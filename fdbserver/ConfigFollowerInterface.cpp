/*
 * ConfigFollowerInterface.cpp
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2018 Apple Inc. and the FoundationDB project authors
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

#include "flow/IRandom.h"
#include "fdbserver/ConfigFollowerInterface.h"
#include "fdbserver/CoordinationInterface.h"

void ConfigFollowerInterface::setupWellKnownEndpoints() {
	getSnapshotAndChanges.makeWellKnownEndpoint(WLTOKEN_CONFIGFOLLOWER_GETSNAPSHOTANDCHANGES,
	                                            TaskPriority::Coordination);
	getChanges.makeWellKnownEndpoint(WLTOKEN_CONFIGFOLLOWER_GETCHANGES, TaskPriority::Coordination);
	compact.makeWellKnownEndpoint(WLTOKEN_CONFIGFOLLOWER_COMPACT, TaskPriority::Coordination);
}

ConfigFollowerInterface::ConfigFollowerInterface() : _id(deterministicRandom()->randomUniqueID()) {}

ConfigFollowerInterface::ConfigFollowerInterface(NetworkAddress const& remote)
  : _id(deterministicRandom()->randomUniqueID()),
    getSnapshotAndChanges(Endpoint({ remote }, WLTOKEN_CONFIGFOLLOWER_GETSNAPSHOTANDCHANGES)),
    getChanges(Endpoint({ remote }, WLTOKEN_CONFIGFOLLOWER_GETCHANGES)),
    compact(Endpoint({ remote }, WLTOKEN_CONFIGFOLLOWER_COMPACT)) {}

bool ConfigFollowerInterface::operator==(ConfigFollowerInterface const& rhs) const {
	return _id == rhs._id;
}

bool ConfigFollowerInterface::operator!=(ConfigFollowerInterface const& rhs) const {
	return !(*this == rhs);
}

ConfigClassSet::ConfigClassSet() = default;

ConfigClassSet::ConfigClassSet(VectorRef<KeyRef> configClasses) {
	for (const auto& configClass : configClasses) {
		classes.insert(configClass);
	}
}

bool ConfigClassSet::contains(KeyRef configClass) const {
	return classes.count(configClass);
}
