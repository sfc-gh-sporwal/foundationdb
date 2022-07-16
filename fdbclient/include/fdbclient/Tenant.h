/*
 * Tenant.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2022 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FDBCLIENT_TENANT_H
#define FDBCLIENT_TENANT_H
#pragma once

#include "fdbclient/FDBTypes.h"
#include "fdbclient/VersionedMap.h"
#include "fdbclient/KeyBackedTypes.h"
#include "flow/flat_buffers.h"

typedef StringRef TenantNameRef;
typedef Standalone<TenantNameRef> TenantName;
typedef StringRef TenantGroupNameRef;
typedef Standalone<TenantGroupNameRef> TenantGroupName;

enum class TenantState { REGISTERING, READY, REMOVING, ERROR };

struct TenantMapEntry {
	constexpr static FileIdentifier file_identifier = 12247338;

	static Key idToPrefix(int64_t id);
	static int64_t prefixToId(KeyRef prefix);

	static std::string tenantStateToString(TenantState tenantState);
	static TenantState stringToTenantState(std::string stateStr);

	Arena arena;
	int64_t id = -1;
	Key prefix;
	Optional<TenantGroupName> tenantGroup;
	TenantState tenantState = TenantState::READY;
	// TODO: fix this type
	Optional<Standalone<StringRef>> assignedCluster;

	constexpr static int ROOT_PREFIX_SIZE = sizeof(id);

	TenantMapEntry();
	TenantMapEntry(int64_t id, KeyRef subspace, TenantState tenantState);
	TenantMapEntry(int64_t id, KeyRef subspace, Optional<TenantGroupName> tenantGroup, TenantState tenantState);

	void setSubspace(KeyRef subspace);
	bool matchesConfiguration(TenantMapEntry const& other) const;

	Value encode() const { return ObjectWriter::toValue(*this, IncludeVersion(ProtocolVersion::withTenants())); }

	static TenantMapEntry decode(ValueRef const& value) {
		TenantMapEntry entry;
		ObjectReader reader(value.begin(), IncludeVersion(ProtocolVersion::withTenants()));
		reader.deserialize(entry);
		return entry;
	}

	template <class Ar>
	void serialize(Ar& ar) {
		KeyRef subspace;
		if (ar.isDeserializing) {
			serializer(ar, id, subspace, tenantGroup, tenantState, assignedCluster);
			ASSERT(tenantState >= TenantState::REGISTERING && tenantState <= TenantState::ERROR);
			if (id >= 0) {
				setSubspace(subspace);
			}
		} else {
			ASSERT(prefix.size() >= 8 || (prefix.empty() && id == -1));
			if (!prefix.empty()) {
				subspace = prefix.substr(0, prefix.size() - 8);
			}
			ASSERT(tenantState >= TenantState::REGISTERING && tenantState <= TenantState::ERROR);
			serializer(ar, id, subspace, tenantGroup, tenantState, assignedCluster);
		}
	}
};

struct TenantMetadataSpecification {
	static KeyRef subspace;

	// TODO: can we break compatibility and use the tuple codec?
	struct TenantIdCodec {
		static Standalone<StringRef> pack(int64_t id) { return TenantMapEntry::idToPrefix(id); }
		static int64_t unpack(Standalone<StringRef> val) { return TenantMapEntry::prefixToId(val); }
	};

	KeyBackedObjectMap<TenantName, TenantMapEntry, decltype(IncludeVersion()), NullCodec> tenantMap;
	KeyBackedProperty<int64_t, TenantIdCodec> lastTenantId;
	KeyBackedSet<Tuple> tenantGroupTenantIndex;
	KeyBackedSet<int64_t> tenantTombstones;

	// TODO: unify tenant subspace
	TenantMetadataSpecification(KeyRef subspace)
	  : tenantMap(subspace.withSuffix("tenantMap/"_sr), IncludeVersion(ProtocolVersion::withTenantGroups())),
	    lastTenantId(subspace.withSuffix("tenantLastId"_sr)),
	    tenantGroupTenantIndex(subspace.withSuffix("tenant/tenantGroup/tenantIndex/"_sr)),
	    tenantTombstones(subspace.withSuffix("/tenant/tombstones/"_sr)) {}
};

struct TenantMetadata {
private:
	static inline TenantMetadataSpecification instance = TenantMetadataSpecification("\xff/"_sr);

public:
	static inline auto& tenantMap = instance.tenantMap;
	static inline auto& lastTenantId = instance.lastTenantId;
	static inline auto& tenantGroupTenantIndex = instance.tenantGroupTenantIndex;
	static inline auto& tenantTombstones = instance.tenantTombstones;

	static inline Key tenantMapPrivatePrefix = "\xff"_sr.withSuffix(tenantMap.subspace.begin);
};

typedef VersionedMap<TenantName, TenantMapEntry> TenantMap;
typedef VersionedMap<Key, TenantName> TenantPrefixIndex;

#endif