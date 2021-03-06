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

// Defines a stream reader for wasm/casm files.

#ifndef DECOMPRESSOR_SRC_INTERP_BYTEREADER_H
#define DECOMPRESSOR_SRC_INTERP_BYTEREADER_H

#include <map>

#include "interp/Reader.h"
#include "stream/BitReadCursor.h"

namespace wasm {

namespace interp {

class ReadStream;

class ByteReader : public Reader {
  ByteReader() = delete;
  ByteReader(const ByteReader&) = delete;
  ByteReader& operator=(const ByteReader&) = delete;

 public:
  ByteReader(std::shared_ptr<decode::Queue> Input);
  ~ByteReader() OVERRIDE;
  void setReadPos(const decode::BitReadCursor& ReadPos);
  decode::BitReadCursor& getPos();

  void describePeekPosStack(FILE* Out) OVERRIDE;
  bool canProcessMoreInputNow() OVERRIDE;
  bool stillMoreInputToProcessNow() OVERRIDE;
  bool atInputEof() OVERRIDE;
  bool atInputEob() OVERRIDE;
  bool pushPeekPos() OVERRIDE;
  bool popPeekPos() OVERRIDE;
  decode::StreamType getStreamType() OVERRIDE;
  bool processedInputCorrectly(bool CheckForEof) OVERRIDE;
  void readFillStart() OVERRIDE;
  void readFillMoreInput() OVERRIDE;
  uint8_t readBit() OVERRIDE;
  uint8_t readUint8() OVERRIDE;
  uint32_t readUint32() OVERRIDE;
  uint64_t readUint64() OVERRIDE;
  int32_t readVarint32() OVERRIDE;
  int64_t readVarint64() OVERRIDE;
  uint32_t readVaruint32() OVERRIDE;
  uint64_t readVaruint64() OVERRIDE;
  bool alignToByte() OVERRIDE;
  bool readBlockEnter() OVERRIDE;
  bool readBlockExit() OVERRIDE;
  bool readBinary(const filt::Node* Encoding, decode::IntType& Value) OVERRIDE;
  bool tablePush(decode::IntType Value) OVERRIDE;
  bool tablePop() OVERRIDE;

  utils::TraceContextPtr getTraceContext() OVERRIDE;

 private:
  class TableHandler;

  decode::BitReadCursor ReadPos;
  std::shared_ptr<ReadStream> Input;
  // The input position needed to fill to process now.
  size_t FillPos;
  // The input cursor position if back filling.
  decode::ReadCursor FillCursor;
  // The stack of saved read cursors.
  decode::BitReadCursor SavedPos;
  utils::ValueStack<decode::BitReadCursor> SavedPosStack;
  TableHandler* TblHandler;
};

}  // end of namespace interp

}  // end of namespace wasm

#endif  // DECOMPRESSOR_SRC_INTERP_BYTEREADER_H
