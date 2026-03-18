#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class TCPSender
{
public:
  /* 使用给定的默认重传超时时间(RTO)和可能的初始序列号(ISN)构造 TCP 发送端 */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), cur_RTO_ms_( initial_RTO_ms )
  {}

  /* 生成一个空的 TCPSenderMessage（不包含数据的 TCP 报文） */
  TCPSenderMessage make_empty_message() const;

  /* 接收并处理来自对端接收器的 TCPReceiverMessage */
  void receive( const TCPReceiverMessage& msg );

  /* transmit 函数的类型定义，push 和 tick 方法可以用它来发送 TCP 报文 */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* 从发送缓冲区中取出字节并发送 */
  void push( const TransmitFunction& transmit );

  /* 距离上一次调用 tick() 已经过了多少毫秒 */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // 访问函数
  uint64_t sequence_numbers_in_flight() const;  // 当前有多少序列号仍未被确认？
  uint64_t consecutive_retransmissions() const; // 连续发生了多少次重传？

  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  ByteStream input_;        // 发送端的数据输入流（待发送的数据）
  Wrap32 isn_;              // 初始序列号 Initial Sequence Number
  uint64_t initial_RTO_ms_; // 初始重传超时时间（毫秒）

  uint64_t send_bytes_{};           // 发出字节数
  uint64_t ack_bytes_{};            // 确认字节数
  uint64_t retransmission_times_{}; // 连续重传次数
  uint64_t window_size_ = 1;        // 假设初始拥塞窗口 = 1
  bool     is_syn_{};               //
  bool     is_fin_{};               //
  uint64_t                     timer_{};                // 计时器
  bool                         is_timer_on_{};          // 计时器是否开启
  std::queue<TCPSenderMessage> outstanding_segments_{}; // 已发送但尚未被接收方确认的段
  uint64_t                     cur_RTO_ms_;             // 现在 RTO 时间
};
