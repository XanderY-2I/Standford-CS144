#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class Reader;
class Writer;

class ByteStream
{
public:
  explicit ByteStream( uint64_t capacity );

  // 获取 Reader / Writer 接口
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;

  void set_error() { error_ = true; }      // 标记错误
  bool has_error() const { return error_; } // 是否发生错误

protected:
  // 字节流内部状态
  bool isClosed_;          // 是否关闭
  std::string buf_;        // 缓冲区
  uint64_t bytes_popped_;  // 已读字节数
  uint64_t bytes_pushed_;  // 已写字节数
  uint64_t capacity_;      // 容量
  bool error_ {};          // 错误标志
};

class Writer : public ByteStream
{
public:
  void push( std::string data ); // 写入数据（受容量限制）
  void close();                  // 关闭写端

  bool is_closed() const;              // 是否关闭
  uint64_t available_capacity() const; // 当前可写容量
  uint64_t bytes_pushed() const;       // 累计写入字节数
};

class Reader : public ByteStream
{
public:
  std::string_view peek() const; // 查看缓冲区数据
  void pop( uint64_t len );      // 读取并移除数据

  bool is_finished() const;        // 是否结束（关闭且读完）
  uint64_t bytes_buffered() const; // 当前缓冲区字节数
  uint64_t bytes_popped() const;   // 累计读取字节数
};

// 从 Reader 读取最多 max_len 字节到 out（内部会 peek + pop）
void read( Reader& reader, uint64_t max_len, std::string& out );