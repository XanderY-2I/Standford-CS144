#pragma once

#include "address.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"

#include <cstddef>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>
#include<memory>

class NetworkInterface
{
public:
  class OutputPort// 输出端口抽象，用于发送以太网帧
  {
  public:
    virtual void transmit( const NetworkInterface& sender, const EthernetFrame& frame ) = 0;
    virtual ~OutputPort() = default;
  };

  // 构造网络接口：接口名 + 输出端口 + MAC地址 + IP地址
  NetworkInterface( std::string_view name,std::shared_ptr<OutputPort> port,const EthernetAddress& ethernet_address,const Address& ip_address );

  // 发送IP数据报（封装为以太网帧，必要时通过ARP解析MAC地址）
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // 接收以太网帧并处理（IPv4或ARP）
  void recv_frame( const EthernetFrame& frame );

  // 时间推进，用于更新计时器
  void tick( size_t ms_since_last_tick );

  // 访问接口名称
  const std::string& name() const { return name_; }

  // 访问输出端口
  const OutputPort& output() const { return *port_; }
  OutputPort& output() { return *port_; }

  // 获取收到的IP数据报队列
  std::queue<InternetDatagram>& datagrams_received() { return datagrams_received_; }

private:
  // 接口名称
  std::string name_;
  // 物理输出端口
  std::shared_ptr<OutputPort> port_;
  // 发送以太网帧
  void transmit( const EthernetFrame& frame ) const { port_->transmit( *this, frame ); }
  // 本接口的MAC地址
  EthernetAddress ethernet_address_;
  // 本接口的IP地址
  Address ip_address_;
  // 已接收的IP数据报队列
  std::queue<InternetDatagram> datagrams_received_ {};
  // 构造ARP报文
  auto make_arp( uint16_t, const EthernetAddress&, uint32_t ) const noexcept -> ARPMessage;
  // ARP缓存有效时间（30秒）
  static constexpr size_t ARP_ENTRY_TTL_ms { 30'000 };
  // ARP请求重发间隔（5秒）
  static constexpr size_t ARP_RESPONSE_TTL_ms { 5'000 };
  struct Timer//计时器
  {
    size_t _ms {};
    constexpr Timer& tick( const size_t& ms_since_last_tick ) noexcept// 时间增加
    {
      return _ms += ms_since_last_tick, *this;
    }
    [[nodiscard]] constexpr bool expired( const size_t& TTL_ms ) const noexcept// 判断是否过期
    {
      return _ms >= TTL_ms;
    }
  };
  using AddressNumeric = decltype( ip_address_.ipv4_numeric() );
  // 等待ARP解析的IP数据报队列
  std::unordered_map<AddressNumeric, std::vector<InternetDatagram>> dgrams_waitting_ {};
  // ARP请求计时器
  std::unordered_map<AddressNumeric, Timer> waitting_timer_ {};
  // ARP缓存表：IP -> (MAC地址, 计时器)
  std::unordered_map<AddressNumeric, std::pair<EthernetAddress, Timer>> ARP_cache_ {};
};