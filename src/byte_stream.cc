#include "byte_stream.hh"
#include "debug.hh"
using namespace std;

// 构造函数：初始化 ByteStream
ByteStream::ByteStream( uint64_t capacity )
  : isClosed_( false ), buf_(), bytes_popped_( 0 ), bytes_pushed_( 0 ), capacity_( capacity )
{}

// 写入数据，但不能超过剩余容量
void Writer::push( string data )
{
  if ( isClosed_ ) // 如果流已经关闭，不能再写
    return;

  int len = min( data.size(), available_capacity() ); // 最多写入可用容量
  buf_.append( data.substr( 0, len ) ); // 写入缓冲区
  bytes_pushed_ += len; // 更新写入总字节数
}

// 标记流结束（不会再写入数据）
void Writer::close()
{
  isClosed_ = true;
}

// 判断流是否关闭
bool Writer::is_closed() const
{
  return isClosed_;
}

// 当前还能写入多少字节
uint64_t Writer::available_capacity() const
{
  return capacity_ - buf_.size();
}

// 已经写入的总字节数
uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

// 查看缓冲区数据（不删除）
string_view Reader::peek() const
{
  if ( buf_.empty() ) // 没有数据
    return {};
  return string_view( buf_ ); // 返回整个缓冲区
}

// 从缓冲区删除 len 字节
void Reader::pop( uint64_t len )
{
  len = min( len, buf_.size() ); // 防止越界
  buf_ = buf_.substr( len ); // 删除前 len 字节
  bytes_popped_ += len; // 更新读取字节数
}

// 判断流是否结束（关闭且缓冲区为空）
bool Reader::is_finished() const
{
  return buf_.empty() && isClosed_;
}

// 当前缓冲区还有多少字节
uint64_t Reader::bytes_buffered() const
{
  return buf_.size();
}

// 已经读取的总字节数
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}