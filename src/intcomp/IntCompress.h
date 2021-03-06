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

// Defines the compressor of WASM files based on integer usage.

#ifndef DECOMPRESSOR_SRC_INTCOMP_INTCOMPRESS_H
#define DECOMPRESSOR_SRC_INTCOMP_INTCOMPRESS_H

#include "intcomp/AbbrevAssignWriter.h"
#include "intcomp/CompressionFlags.h"
#include "intcomp/CountNode.h"
#include "interp/IntFormats.h"
#include "interp/IntStream.h"
#include "interp/Interpreter.h"
#include "sexp/Ast.h"
#include "stream/BitWriteCursor.h"
#include "stream/Queue.h"
#include "utils/HuffmanEncoding.h"

namespace wasm {

namespace intcomp {

class IntCounterWriter;

class IntCompressor FINAL {
  IntCompressor() = delete;
  IntCompressor(const IntCompressor&) = delete;
  IntCompressor& operator=(const IntCompressor&) = delete;

 public:
  IntCompressor(std::shared_ptr<decode::Queue> InputStream,
                std::shared_ptr<decode::Queue> OutputStream,
                std::shared_ptr<filt::SymbolTable> Symtab,
                const CompressionFlags& MyFlags);

  ~IntCompressor();

  bool errorsFound() const { return ErrorsFound; }

  std::shared_ptr<RootCountNode> getRoot();

  void compress();

  void setTraceProgress(bool NewValue) {
    // TODO: Don't force creation of trace object if not needed.
    getTrace().setTraceProgress(NewValue);
  }
  bool getTraceProgress() {
    // TODO: Don't force creation of trace object if not needed.
    return getTrace().getTraceProgress();
  }
  void setTrace(std::shared_ptr<utils::TraceClass> Trace);
  utils::TraceClass& getTrace() { return *getTracePtr(); }
  std::shared_ptr<utils::TraceClass> getTracePtr();
  bool hasTrace() { return bool(Trace) && Trace->getTraceProgress(); }

  void describe(FILE* Out,
                CollectionFlags Flags = makeFlags(CollectionFlag::All),
                bool Trace = false) {
    describeCutoff(Out, MyFlags.CountCutoff, MyFlags.WeightCutoff, Flags,
                   Trace);
  }

  void describeCutoff(FILE* Out,
                      uint64_t CountCutoff,
                      CollectionFlags Flags = makeFlags(CollectionFlag::All),
                      bool Trace = false) {
    describeCutoff(Out, CountCutoff, CountCutoff, Flags, Trace);
  }

  void describeCutoff(FILE* Out,
                      uint64_t CountCutoff,
                      uint64_t WeightCutoff,
                      CollectionFlags Flags = makeFlags(CollectionFlag::All),
                      bool Trace = false);

  void describeAbbreviations(FILE* Out, bool Trace = false);

 private:
  std::shared_ptr<RootCountNode> Root;
  utils::HuffmanEncoder::NodePtr EncodingRoot;
  std::shared_ptr<decode::Queue> Input;
  std::shared_ptr<decode::Queue> Output;
  const CompressionFlags& MyFlags;
  std::shared_ptr<filt::SymbolTable> Symtab;
  std::shared_ptr<interp::IntStream> Contents;
  std::shared_ptr<interp::IntStream> IntOutput;
  std::shared_ptr<utils::TraceClass> Trace;
  bool ErrorsFound;
  void readInput();
  const decode::BitWriteCursor writeCodeOutput(
      std::shared_ptr<filt::SymbolTable> Symtab);
  void writeDataOutput(const decode::BitWriteCursor& StartPos,
                       std::shared_ptr<filt::SymbolTable> Symtab);
  bool compressUpToSize(size_t Size);
  void removeSmallUsageCounts(bool KeepSingletonsUsingCount,
                              bool ZeroOutSmallNodes);
  void removeSmallSingletonUsageCounts() {
    removeSmallUsageCounts(true, false);
  }
  void removeAllSmallUsageCounts() { removeSmallUsageCounts(false, false); }
  void zeroSmallUsageCounts() { removeSmallUsageCounts(false, true); }
  void assignInitialAbbreviations(CountNode::PtrSet& Assignments);
  bool generateIntOutput(CountNode::PtrSet& Assignments);
  std::shared_ptr<filt::SymbolTable>
  generateCode(CountNode::PtrSet& Assignments, bool ToRead, bool Trace);
  std::shared_ptr<filt::SymbolTable> generateCodeForReading(
      CountNode::PtrSet& Assignments) {
    return generateCode(Assignments, true,
                        MyFlags.TraceCodeGenerationForReading);
  }
  std::shared_ptr<filt::SymbolTable> generateCodeForWriting(
      CountNode::PtrSet& Assignments) {
    return generateCode(Assignments, false,
                        MyFlags.TraceCodeGenerationForWriting);
  }
};

}  // end of namespace intcomp

}  // end of namespace wasm

#endif  // DECOMPRESSOR_SRC_INTCOMP_INTCOMPRESS_H
