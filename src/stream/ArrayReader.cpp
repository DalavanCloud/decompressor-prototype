/* -*- C++ -*- */
/*
 * Copyright 2016 WebAssembly Community Group participants
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

#include "stream/ArrayReader.h"

namespace wasm {

namespace decode {

ArrayReader::~ArrayReader() {
}

size_t ArrayReader::read(uint8_t* Buf, size_t Size) {
  size_t ActualSize = std::min(Size, BufferSize - CurPosition);
  for (size_t i = 0; i < ActualSize; ++i)
    Buf[i] = Buffer[CurPosition++];
  return ActualSize;
}

bool ArrayReader::write(uint8_t* Buf, size_t Size) {
  (void)Buf;
  (void)Size;
  return false;
}

bool ArrayReader::freeze() {
  // Assume that array should be truncated at current location.
  return false;
}

bool ArrayReader::atEof() {
  return CurPosition == BufferSize;
}

}  // end of namespace decode

}  // end of namespace wasm