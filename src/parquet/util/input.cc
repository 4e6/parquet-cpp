// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "parquet/util/input.h"

#include <algorithm>
#include <string>

#include "parquet/exception.h"
#include "parquet/util/buffer.h"

namespace parquet_cpp {

// ----------------------------------------------------------------------
// RandomAccessSource

std::shared_ptr<Buffer> RandomAccessSource::ReadAt(int64_t pos, int64_t nbytes) {
  Seek(pos);
  return Read(nbytes);
}

// ----------------------------------------------------------------------
// LocalFileSource

LocalFileSource::~LocalFileSource() {
  CloseFile();
}

void LocalFileSource::Open(const std::string& path) {
  path_ = path;
  file_ = fopen(path_.c_str(), "r");
  is_open_ = true;
  fseek(file_, 0L, SEEK_END);
  size_ = Tell();
  Seek(0);
}

void LocalFileSource::Close() {
  // Pure virtual
  CloseFile();
}

void LocalFileSource::CloseFile() {
  if (is_open_) {
    fclose(file_);
    is_open_ = false;
  }
}

void LocalFileSource::Seek(int64_t pos) {
  fseek(file_, pos, SEEK_SET);
}

int64_t LocalFileSource::Size() const {
  return size_;
}

int64_t LocalFileSource::Tell() const {
  return ftell(file_);
}

int64_t LocalFileSource::Read(int64_t nbytes, uint8_t* buffer) {
  return fread(buffer, 1, nbytes, file_);
}

std::shared_ptr<Buffer> LocalFileSource::Read(int64_t nbytes) {
  auto result = std::make_shared<OwnedMutableBuffer>();
  result->Resize(nbytes);

  int64_t bytes_read = Read(nbytes, result->mutable_data());
  if (bytes_read < nbytes) {
    result->Resize(bytes_read);
  }
  return result;
}

// ----------------------------------------------------------------------
// BufferReader

BufferReader::BufferReader(const std::shared_ptr<Buffer>& buffer) :
    buffer_(buffer),
    data_(buffer->data()),
    pos_(0) {
  size_ = buffer->size();
}

int64_t BufferReader::Tell() const {
  return pos_;
}

void BufferReader::Seek(int64_t pos) {
  if (pos < 0 || pos >= size_) {
    std::stringstream ss;
    ss << "Cannot seek to " << pos
       << "File is length " << size_;
    throw ParquetException(ss.str());
  }
  pos_ = pos;
}

int64_t BufferReader::Size() const {
  return size_;
}

int64_t BufferReader::Read(int64_t nbytes, uint8_t* out) {
  ParquetException::NYI("not implemented");
  return 0;
}

std::shared_ptr<Buffer> BufferReader::Read(int64_t nbytes) {
  int64_t bytes_available = std::min(nbytes, size_ - pos_);
  auto result = std::make_shared<Buffer>(Head(), bytes_available);
  pos_ += bytes_available;
  return result;
}

// ----------------------------------------------------------------------
// InMemoryInputStream

InMemoryInputStream::InMemoryInputStream(const std::shared_ptr<Buffer>& buffer) :
    buffer_(buffer), offset_(0) {
  len_ = buffer_->size();
}

const uint8_t* InMemoryInputStream::Peek(int64_t num_to_peek, int64_t* num_bytes) {
  *num_bytes = std::min(static_cast<int64_t>(num_to_peek), len_ - offset_);
  return buffer_->data() + offset_;
}

const uint8_t* InMemoryInputStream::Read(int64_t num_to_read, int64_t* num_bytes) {
  const uint8_t* result = Peek(num_to_read, num_bytes);
  offset_ += *num_bytes;
  return result;
}

void InMemoryInputStream::Advance(int64_t num_bytes) {
  offset_ += num_bytes;
}

} // namespace parquet_cpp
