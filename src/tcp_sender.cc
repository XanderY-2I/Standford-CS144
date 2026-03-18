#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include <iostream>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const//未确认字节数
{
  return send_bytes_-ack_bytes_;
}

uint64_t TCPSender::consecutive_retransmissions() const//统计连续重传次数，过多会关闭传输
{
  return retransmission_times_;
}

void TCPSender::push( const TransmitFunction& transmit )//从缓冲区取出内容并发送
{
  while((window_size_==0?1:window_size_)>sequence_numbers_in_flight())
  {
    if(is_fin_)break;

    //构造segment{SYN,payload,FIN}
    auto msg=make_empty_message();
    if(!is_syn_)//开始
    {
      msg.SYN=true;
      is_syn_=true;
    }

    uint64_t remaining=(window_size_==0?1:window_size_)-sequence_numbers_in_flight();
    uint64_t len=min(TCPConfig::MAX_PAYLOAD_SIZE,remaining-msg.sequence_length());
    auto &&data=msg.payload;
    while(reader().bytes_buffered()&&data.size()<len)
    {
      auto cur_data=reader().peek();
      cur_data=cur_data.substr(0,len-data.size());
      data+=cur_data;//拼接
      input_.reader().pop(data.size());
    }
    
    if(!is_fin_&&remaining>msg.sequence_length()&&reader().is_finished())//结束
    {
      msg.FIN=true;
      is_fin_=true;
    }
    if(msg.sequence_length()==0) break;//空包
    transmit(msg);
    if(!is_timer_on_)//开计时器
    {
      is_timer_on_=true;
      timer_=0;
    }
    send_bytes_+=msg.sequence_length();//bytes_in_flight
    outstanding_segments_.emplace(msg);//保留数据用于重新传
  }
}

//发送空报文，用于发ack，触发receiver更新window，SYN/FIN之前初始化
TCPSenderMessage TCPSender::make_empty_message() const
{
   return{Wrap32::wrap(send_bytes_,isn_),false,{},false,input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )//接收来自接收端的反馈信号
{
  window_size_=msg.window_size;
  if(msg.RST)//有错误，关闭流
  {
    input_.set_error();
    return;
  }
  if(msg.ackno.has_value())//收到ack
  {
    uint64_t recv_ackno=msg.ackno.value().unwrap(isn_,ack_bytes_);
    if(recv_ackno>send_bytes_) return;//错误ack
    while(!outstanding_segments_.empty())//删除已ack的文件，避免重复传
    {
      auto seg=outstanding_segments_.front();//最早发送但未确认
      if(recv_ackno<ack_bytes_+seg.sequence_length()) break;//没完全被确认
      ack_bytes_+=seg.sequence_length();
      outstanding_segments_.pop();

      retransmission_times_=0;//全部恢复初始
      cur_RTO_ms_=initial_RTO_ms_;
      timer_=0;

      if(outstanding_segments_.empty()) 
      {
        is_timer_on_=false;
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit ) {
    if (!is_timer_on_) return;
    
    timer_ += ms_since_last_tick;
    if (timer_ >= cur_RTO_ms_) { // 修正变量名
        if (!outstanding_segments_.empty()) {
            transmit(outstanding_segments_.front()); // 只重传第一个（最老的）
            if (window_size_ > 0) { // 窗口不为0才翻倍
                retransmission_times_++;
                cur_RTO_ms_ *= 2;
            }
            timer_ = 0; // 重置计时器
            is_timer_on_ = true;
        }
    }
}
