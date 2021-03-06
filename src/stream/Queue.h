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

// Defines a queue for hold buffers to streams.
//
// Shared pointers to pages are used to effectively lock pages in the buffer.
// This allows one to "backpatch" addresses, making sure that the pages are not
// thrown away until all shared pointers have been released.
//
// Note: Virtual addresses are used, start at index 0, and correspond to a
// buffer index as if the queue keeps all pages (i.e. doesn't shrink) until the
// queue is destructed. Therefore, if a byte is written at address N, to read
// the value you must always use address N.
//
// It is assumed that jumping on reads and writes are valid. However, back jumps
// are only safe if you already have a cursor pointing to the page to be
// backpatched.
//
// TODO(KarlSchimpf): Locking of reads/writes for threading has not yet been
// addressed, and the current implementation is NOT thread safe.

#ifndef DECOMPRESSOR_SRC_STREAM_QUEUE_H_
#define DECOMPRESSOR_SRC_STREAM_QUEUE_H_

#include <vector>

#include "stream/PageAddress.h"

namespace wasm {

namespace decode {

class BlockEob;
class Page;
class PageCursor;

class Queue : public std::enable_shared_from_this<Queue> {
  Queue(const Queue&) = delete;
  Queue& operator=(const Queue&) = delete;
  friend class Cursor;
  friend class PageCursor;
  friend class ReadCursor;
  friend class WriteCursor;

 public:
  enum class StatusValue { Good, Bad };
  Queue();

  virtual ~Queue();

  // Defines the maximum Peek size into the queue when reading. That
  // is, The minimal number of bytes that the reader can back up without
  // freezing an address. Defaults to 32.
  void setMinPeekSize(AddressType NewValue) { MinPeekSize = NewValue; }

  // Value unknown (returning maximum possible size) until frozen. When
  // frozen, returns the size of the buffer.
  AddressType currentSize() const;

  AddressType fillSize() const;

  // Returns the actual size of the buffer (i.e. only those with pages still
  // in memory).
  AddressType actualSize() const;

  AddressType getEofAddress() const;

  // Update Cursor to point to the given Address, and make (up to) WantedSize
  // elements available for reading. Returns the actual number of elements
  // available for reading. Note: Address will be moved to an error address
  // if an error occurs while reading.
  AddressType readFromPage(AddressType& Address,
                           AddressType WantedSize,
                           PageCursor& Cursor);

  // Update Cursor to point to the given Address, and make (up to) WantedSize
  // elements available for writing. Returns the actual number of elements
  // available for writing. Note: Address will be moved to an error address
  // if an error occurs while writing.
  AddressType writeToPage(AddressType& Address,
                          AddressType WantedSize,
                          PageCursor& Cursor);

  // Closes the queue (i.e. assumes all contents have been read/written).
  void close();

  // Reads a contiguous range of bytes into a buffer.
  //
  // Note: A read request may not be fully met. This function only guarantees
  // to read 1 element from the queue, if eob hasn't been reached. This is
  // done to minimize blocking. When possible, it will try to meet the full
  // request.
  //
  // @param Address The address within the queue to read from. Automatically
  //                incremented during read.
  // @param Buffer  A pointer to a buffer to be filled (and contains at least
  //                Size elements).
  // @param Size    The number of requested elements to read.
  // @result        The actual number of elements read. If zero, the eob was
  //                reached. Valid return values are in [0..Size].
  AddressType read(AddressType& Address, uint8_t* Buffer, AddressType Size = 1);

  // Writes a contiquous sequence of bytes in the given buffer.
  //
  // Note: If Address is larger than queue size, zero's are automatically
  // inserted.
  //
  // @param Address The address within the queue to write to. Automatically
  //                incremented during write.
  // @param Buffer  A pointer to the buffer of elements to write.
  // @param Size    The number of elements in the buffer to write.
  // @result        True if successful (i.e. not beyond eob address).
  bool write(AddressType& Address, uint8_t* Buffer, AddressType Size = 1);

  // Freezes eob of the queue. Not valid to read/write past the eob, once set.
  // Note: May change Address if queue is broken, or Address not valid.
  void freezeEof(AddressType& Address);

  bool isBroken(const PageCursor& C) const;
  bool isEofFrozen() const { return EofFrozen; }
  bool isGood() const { return Status == StatusValue::Good; }

  const std::shared_ptr<BlockEob>& getEofPtr() const { return EofPtr; }

  // Mark queue as broken.
  void fail();

  void describe(FILE* Out);

 protected:
  typedef std::vector<std::weak_ptr<Page>> PageMapType;
  // Minimum peek size to maintain. That is, the minimal number of
  // bytes that the read can back up without freezing an address.
  AddressType MinPeekSize;
  // True if end of queue buffer has been frozen.
  bool EofFrozen;
  StatusValue Status;
  std::shared_ptr<BlockEob> EofPtr;
  // First page still in queue.
  std::shared_ptr<Page> FirstPage;
  // Page at the current end of buffer.
  std::shared_ptr<Page> LastPage;
  // Page to use if an error occurs.
  std::shared_ptr<Page> ErrorPage;
  // Fast page lookup map (from page index)
  PageMapType PageMap;

  bool appendPage();

  // Returns the page in the queue referred to Address, or nullptr if no
  // such page is in the byte queue.
  std::shared_ptr<Page> getReadPage(AddressType& Address) const;
  std::shared_ptr<Page> getWritePage(AddressType& Address) const;
  std::shared_ptr<Page> getCachedPage(AddressType& Address);
  std::shared_ptr<Page> getDefinedPage(AddressType Index,
                                       AddressType& Address) const;
  std::shared_ptr<Page> failThenGetErrorPage(AddressType& Address);
  std::shared_ptr<Page> getErrorPage();
  std::shared_ptr<Page> readFillToPage(AddressType Index, AddressType& Address);
  std::shared_ptr<Page> writeFillToPage(AddressType Index,
                                        AddressType& Address);

  bool isValidPageAddress(AddressType Address) {
    return PageIndex(Address) < PageMap.size();
  }

  // Dumps and deletes the first page.  Note: Dumping only occurs if a
  // Writer is provided (see class WriteBackedByteQueue below).
  virtual void dumpFirstPage();

  // Dumps ununsed pages prior to Address. If applicable, writes out bytes to
  // corresponding output stream before dumping.
  void dumpPreviousPages();

  // Fills buffer until we can read 1 or more bytes at the given address.
  // Returns true if successful. If applicable, reads from input to fill the
  // buffer as appropriate.
  virtual bool readFill(AddressType Address);
  virtual bool writeFill(AddressType Address, AddressType WantedSize);
};

}  // end of namespace decode

}  // end of namespace wasm

#endif  // DECOMPRESSOR_SRC_STREAM_QUEUE_H_
