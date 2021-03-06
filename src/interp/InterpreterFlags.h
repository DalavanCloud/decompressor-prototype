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

// Defines the control flags for the interpreter.

#ifndef DECOMPRESSOR_SRC_INTERP_INTERPRETERFLAGS_H_
#define DECOMPRESSOR_SRC_INTERP_INTERPRETERFLAGS_H_

namespace wasm {

namespace interp {

enum class MacroDirective {
  // Read once, duplicate (expand) when writing.
  Expand,
  // Read all, write once.
  Contract
};

struct InterpreterFlags {
  InterpreterFlags();
  MacroDirective MacroContext;
  bool TraceProgress;
  bool TraceIntermediateStreams;
  bool TraceAppliedAlgorithms;
};

}  // end of namespace interp

}  // end of namespace wasm

#endif  // DECOMPRESSOR_SRC_INTERP_INTERPRETERFLAGS_H_
