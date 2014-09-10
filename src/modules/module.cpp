/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>

#include "module.hpp"

using std::string;

namespace mesos {

#define MODULE_API_VERSION_FUNCTION_STRING \
  #MODULE_API_VERSION_FUNCTION

typedef string(*VersionFunction)();

Try<Nothing> ModuleManager::parseFlag(string flagValue)
{
  // load all libs

  // check their MMS version
  // if problem, bail
  foreach (DynamicLibrary &lib, dynlibs) {
    Try<void*> symbol = library.loadSymbol(MODULE_API_VERSION_FUNCTION_STRING);
    if (symbol.isError()) {
      return Error(symbol.error());
    }
    VersionFunction* v = (VersionFunction*) symbol;
    string libraryVersion = (*v)();
    if (libraryVersion != MODULE_API_VERSION) {
      return Error("Module API version mismatch. " +
                   "Mesos has: " + MODULE_API_VERSION +
                   "library requires: " + libraryVersion);
    }
  }

  // foreach lib:module
  // dlsym module, get return value, which has type Module
  // for each of those, call verification

  // return map (module name -> Module)
}

} // namespace mesos {