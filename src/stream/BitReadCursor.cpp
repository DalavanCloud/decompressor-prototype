// -*- C++ -*-
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

// Implements a pionter to a byte stream (for reading) that can be read a bit at
// a time.

#include "stream/BitReadCursor.h"

#include "stream/Page.h"

namespace wasm {

namespace decode {

namespace {

constexpr BitReadCursor::WordType BitsInByte =
    BitReadCursor::WordType(sizeof(ByteType) * CHAR_BIT);

}  // end of namespace

BitReadCursor::BitReadCursor() {
  initFields();
}

BitReadCursor::BitReadCursor(std::shared_ptr<Queue> Que)
    : ReadCursor(StreamType::Byte, Que) {
  initFields();
}

BitReadCursor::BitReadCursor(StreamType Type, std::shared_ptr<Queue> Que)
    : ReadCursor(Type, Que) {
  initFields();
}

BitReadCursor::BitReadCursor(const BitReadCursor& C)
    : ReadCursor(C), CurWord(C.CurWord), NumBits(C.NumBits) {}

BitReadCursor::BitReadCursor(const BitReadCursor& C, AddressType StartAddress)
    : ReadCursor(C, StartAddress), CurWord(C.CurWord), NumBits(C.NumBits) {}

BitReadCursor::~BitReadCursor() {}

void BitReadCursor::initFields() {
  CurWord = 0;
  NumBits = 0;
}

void BitReadCursor::assign(const BitReadCursor& C) {
  ReadCursor::assign(C);
  CurWord = C.CurWord;
  NumBits = C.NumBits;
}

void BitReadCursor::swap(BitReadCursor& C) {
  ReadCursor::swap(C);
  std::swap(CurWord, C.CurWord);
  std::swap(NumBits, C.NumBits);
}

void BitReadCursor::alignToByte() {
  assert(NumBits < 8);
  NumBits = 0;
  CurWord = 0;
}

void BitReadCursor::describeDerivedExtensions(FILE* File, bool IncludeDetail) {
  AddressType Address = getAddress();
  if (NumBits == 0 || Address == 0) {
    ReadCursor::describeDerivedExtensions(File, IncludeDetail);
    if (NumBits > 0)
      fprintf(File, ":-%u", NumBits);
    return;
  }
  // Correct by moving back one byte, so that it looks the same as when writing.
  describeAddress(File, Address - 1);
  if (IncludeDetail)
    describePage(File, CurPage.get());
  fprintf(File, ":%u", BitsInByte - NumBits);
}

#define BITREAD_TYPED(Mask, MaskSize)                           \
  do {                                                          \
    if (NumBits >= MaskSize) {                                  \
      NumBits -= MaskSize;                                      \
      ByteType Value = ByteType(CurWord >> NumBits);            \
      CurWord &= ~(Mask << NumBits);                            \
      return Value;                                             \
    }                                                           \
    if (atEob())                                                \
      break;                                                    \
    /* Not enough bits left, read more in. */                   \
    CurWord = (CurWord << BitsInByte) | ReadCursor::readByte(); \
    NumBits += BitsInByte;                                      \
  } while (1);                                                  \
  /* Leftover bits, fix (as best as possible) */                \
  fail();                                                       \
  ByteType Value = ByteType(CurWord);                           \
  CurWord = 0;                                                  \
  NumBits = 0;                                                  \
  return Value;

#define BITREAD(Mask, MaskSize) \
  BITREAD_TYPED(WordType(Mask), unsigned(MaskSize))

namespace {

static constexpr BitReadCursor::WordType ByteMask = (1 << BitsInByte) - 1;

}  // end of anonymous namespace

bool BitReadCursor::atEob() {
  if (!ReadCursor::atEob())
    return false;
  return NumBits == 0;
}

ByteType BitReadCursor::readByte() {
  if (NumBits == 0)
    return ReadCursor::readByte();
  BITREAD(ByteMask, BitsInByte);
}

ByteType BitReadCursor::readBit() {
  BITREAD(1, 1);
}

}  // end of namespace decode

}  // end of namespace wasm
