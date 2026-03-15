#pragma once

#include "byte_stream.hh"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// 表示一个数据片段
struct Interval
{
  uint64_t start_idx;   // 起始字节位置
  uint64_t end_idx;     // 结束字节位置
  std::string data;     // 数据内容

  // set排序规则：先按start，再按end
  bool operator<( const Interval& rhs ) const
  {
    if ( this->start_idx == rhs.start_idx )
      return this->end_idx < rhs.end_idx;
    return this->start_idx < rhs.start_idx;
  }
};

class Reassembler
{
public:
  // 构造函数，指定输出ByteStream
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {}

  // 插入一个新的数据片段
  //first_index: 片段起始位置 data: 数据 is_last_substring: 是否是最后一个片段
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // 返回当前缓存的字节数（测试用）
  uint64_t count_bytes_pending() const;

  // 获取读取接口
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // 获取写入接口（只读）
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;        // 输出字节流
  std::set<Interval> buf_ {};  // 缓存乱序数据
  uint64_t next_expected_idx_ = 0; // 下一个期待的字节序号
  uint64_t eof_idx_ = UINT64_MAX;  // 流结束位置
};