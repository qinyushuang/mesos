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
#include <stout/hashmap.hpp>

#include "modules/module.hpp"
#include "modules/module_manager.hpp"

using std::string;

namespace mesos {

#define MODULE_API_VERSION_FUNCTION_STRING \
  #MODULE_API_VERSION_FUNCTION

typedef string(*StringFunction)();

ModuleManager::ModuleManager()
{
  roleToVersion["TestModule"]    = "0.22";
  roleToVersion["Isolator"]      = "0.22";
  roleToVersion["Authenticator"] = "0.28";
  roleToVersion["Allocator"]     = "0.22";
}

// .30
//
// mesos           library      result
// 0.18(0.18)      0.18         FINE
// 0.18(0.18)      0.19         NOT FINE
// 0.19(0.18)      0.18         FINE
// 0.19(0.19)      0.18         NOT FINE

template<typename T>
static Try<T> callFunction(DynamicLibrary *lib, string functionName)
{
  Try<void*> symbol = lib->loadSymbol(functionName);
  if (symbol.isError()) {
    return Error(symbol.error());
  }
  T (*v)() = (T (*)()) symbol;
  return (*v)();
}

Try<DynamicLibrary*> ModuleManager::loadModuleLibrary(string path)
{
  DynamicLibrary *lib = new DynamicLibrary();
  Try<Nothing> result = lib->open(path);
  if (!result.isSome()) {
    return Error(result.error());
  }

  Try<string> libraryVersion =
    callFunction<string>(lib, MODULE_API_VERSION_FUNCTION_STRING);
  if (libraryVersion.isError()) {
    return libraryVersion.error();
  }
  if (libraryVersion != MODULE_API_VERSION) {
    return Error("Module API version mismatch. " +
                 "Mesos has: " + MODULE_API_VERSION +
                 "library requires: " + libraryVersion);
  }
  return lib;
}

Try<Nothing> ModuleManager::verifyModuleRole(string module, DynamicLibrary *lib)
{
  Try<string> role = callFunction<string>(lib, "get" + module + "Role");
  if (role.isError()) {
    return role.error();
  }
  if (!roleToVersion.contains(role)) {
    return Error("Unknown module role: " + role);
  }

  if (libraryVersion != roleToVersion[role]) {
    return Error("Role version mismatch." +
                 " Mesos supports: >" + roleToVersion[role] +
                 " module requires: " + libraryVersion);
  }
  return Nothing();
}

template<typename Role>
Try<Role*> loadModule(std::string moduleName)
{
  Option<DynamicLibrary*> lib = moduleToDynamicLibrary[moduleName];
  ASSERT_SOME(lib);

  Try<Role*> inst = callFunction<Role*>(lib, "create" + module + "Instance");
  if (inst.isError()) {
    return inst.error();
  }
  return inst;
}

Try<Nothing> ModuleManager::loadLibraries(string modulePaths)
{
  // load all libs
  // check their MMS version
  // if problem, bail
  foreach (path:module, paths:modules) {
    DynamicLibrary* lib = loadModuleLibrary(path);

    // foreach lib:module
    // dlsym module, get return value, which has type Module
    // for each of those, call verification
    verifyModuleRole(lib, module);

    moduleToDynLib[module] = lib;
  }
  return Nothing();
}

void* ModuleManager::findModuleCreator(std::string moduleName)
{
  Option<DynamicLibrary*> lib = moduleToDynamicLibrary[moduleName];
  ASSERT_SOME(lib);
  Try<VoidPointerFunction> symbol = lib.get()->loadSymbol(functionName);
  if (symbol.isError()) {
    return Error(symbol.error());
  }
  return (VoidPointerFunction*) symbol;
}

Try<Isolator> ModuleManager::createIsolator(std::string moduleName)
{
  IsolatorFunction f =  (IsolatorFunction) findModuleCreator(moduleName);
  return f();
}


} // namespace mesos {
