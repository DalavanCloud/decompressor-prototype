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

// Defines a writer for wasm/casm files.

#ifndef DECOMPRESSOR_SRC_INTERP_WRITER_H
#define DECOMPRESSOR_SRC_INTERP_WRITER_H

#include "interp/IntFormats.h"
#include "utils/TraceAPI.h"

namespace wasm {

namespace filt {
class SymbolNode;
}  // end of namespace filt

namespace interp {

class Reader;

class Writer {
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;

 public:
  virtual ~Writer();

  virtual void reset();
  virtual decode::StreamType getStreamType() const = 0;
  // Override the following as needed. These methods return false if the writes
  // failed. Default actions are to do nothing and return true.
  virtual bool writeBit(uint8_t Value);
  virtual bool writeUint8(uint8_t Value);
  virtual bool writeUint32(uint32_t Value);
  virtual bool writeUint64(uint64_t Value);
  virtual bool writeVarint32(int32_t Value);
  virtual bool writeVarint64(int64_t Value);
  virtual bool writeVaruint32(uint32_t Value);
  virtual bool writeVaruint64(uint64_t Value) = 0;
  virtual bool alignToByte();
  virtual bool writeBlockEnter();
  virtual bool writeBlockExit();
  virtual bool writeFreezeEof();
  virtual bool writeBinary(decode::IntType Value, const filt::Node* Encoding);
  virtual bool writeValue(decode::IntType Value, const filt::Node* Format);
  virtual bool writeTypedValue(decode::IntType Value,
                               interp::IntTypeFormat Format);
  virtual bool writeHeaderValue(decode::IntType Value,
                                interp::IntTypeFormat Format);
  virtual bool writeHeaderClose();
  virtual bool writeAction(decode::IntType Action);
  virtual bool tablePush(decode::IntType Value);
  virtual bool tablePop();

  virtual void setMinimizeBlockSize(bool NewValue);
  virtual void describeState(FILE* File);

  virtual utils::TraceContextPtr getTraceContext();
  bool hasTrace();
  virtual void setTrace(std::shared_ptr<utils::TraceClass> Trace);
  std::shared_ptr<utils::TraceClass> getTracePtr();
  utils::TraceClass& getTrace() { return *getTracePtr(); }

 protected:
  bool MinimizeBlockSize;
  // Defines value to return if writeAction can't handle.
  const bool DefaultWriteAction;
  std::shared_ptr<utils::TraceClass> Trace;
  explicit Writer(bool DefaultWriteAction)
      : MinimizeBlockSize(false), DefaultWriteAction(DefaultWriteAction) {}

  virtual const char* getDefaultTraceName() const;
};

}  // end of namespace interp

}  // end of namespace wasm

#endif  // DECOMPRESSOR_SRC_INTERP_WRITER_H
