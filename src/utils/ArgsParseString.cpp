// -*- C++ -*- */
//
// Copyright 2016 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Implements command-line templates for std::string.

#include "utils/ArgsParse.h"

namespace wasm {

namespace utils {

template <>
bool ArgsParser::OptionalSet<std::string>::select(ArgsParser* Parser,
                                                  charstring Add) {
  if (!validOptionValue(Parser, Add))
    return false;
  Values.insert(Add);
  return true;
}

template <>
void ArgsParser::OptionalSet<std::string>::describeDefault(
    FILE* Out,
    size_t TabSize,
    size_t& Indent) const {}

}  // end of namespace utils

}  // end of namespace wasm
