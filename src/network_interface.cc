#include "network_interface.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "helpers.hh" 
#include "exception.hh"
#include <iostream>
using namespace std;

NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{}

void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const AddressNumeric next_hop_ip = next_hop.ipv4_numeric();

  // 1. 如果缓存命中了
  if ( ARP_cache_.contains( next_hop_ip ) ) 
  {
    EthernetFrame frame;
    frame.header = { ARP_cache_[next_hop_ip].first, ethernet_address_, EthernetHeader::TYPE_IPv4 };
    frame.payload = serialize( dgram );
    transmit( frame );
    return;
  }

  // 2. 缓存未命中，入队
  dgrams_waitting_[next_hop_ip].push_back( dgram );

  // 3. 5秒内没发过请求才发新的
  if ( !waitting_timer_.contains( next_hop_ip ) ) {
    const ARPMessage arp_req = make_arp( ARPMessage::OPCODE_REQUEST, {}, next_hop_ip );
    
    EthernetFrame arp_frame;
    arp_frame.header = { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP };
    arp_frame.payload = serialize( arp_req );
    transmit( arp_frame );
    waitting_timer_[next_hop_ip] = Timer {}; // 启动计时
  }
}

void NetworkInterface::recv_frame( const EthernetFrame& frame )
{

  // 基础过滤
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }

  // 处理 IP 包
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) 
  {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( move( dgram ) );
    }
  } 
  // 处理 ARP 包
  else if ( frame.header.type == EthernetHeader::TYPE_ARP ) 
  {
    ARPMessage msg;
    if ( parse( msg, frame.payload ) ) {
      const AddressNumeric sender_ip = msg.sender_ip_address;
      const EthernetAddress sender_eth = msg.sender_ethernet_address;
    
      // 学习映射
      ARP_cache_[sender_ip] = { sender_eth, Timer {} };

      // 发送等待中的包
      if ( dgrams_waitting_.contains( sender_ip ) ) {
        for ( const auto& d : dgrams_waitting_[sender_ip] ) {
          EthernetFrame f;
          f.header = { sender_eth, ethernet_address_, EthernetHeader::TYPE_IPv4 };
          f.payload = serialize( d );
          transmit( f );
        }
        dgrams_waitting_.erase( sender_ip );
        waitting_timer_.erase( sender_ip );
      }

      // 处理对我的请求
      if ( msg.opcode == ARPMessage::OPCODE_REQUEST && msg.target_ip_address == ip_address_.ipv4_numeric() ) {
        const ARPMessage arp_reply = make_arp( ARPMessage::OPCODE_REPLY, sender_eth, sender_ip );
        
        EthernetFrame reply_frame;
        reply_frame.header = { sender_eth, ethernet_address_, EthernetHeader::TYPE_ARP };
        reply_frame.payload = serialize( arp_reply );
        
        transmit( reply_frame );
      }
    }
  }
}

void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // 1. 清理过期的 ARP 缓存 (30s)
  for ( auto it = ARP_cache_.begin(); it != ARP_cache_.end(); ) {
    if ( it->second.second.tick( ms_since_last_tick ).expired( ARP_ENTRY_TTL_ms ) ) {
      it = ARP_cache_.erase( it );
    } else {
      ++it;
    }
  }

  // 2. 清理过期的等待计时器 (5s)
  for ( auto it = waitting_timer_.begin(); it != waitting_timer_.end(); ) {
    if ( it->second.tick( ms_since_last_tick ).expired( ARP_RESPONSE_TTL_ms ) )
   {
     const AddressNumeric target_ip = it->first;
     dgrams_waitting_.erase( target_ip ); 
     it = waitting_timer_.erase( it );
    } else ++it;
  }
}

auto NetworkInterface::make_arp( uint16_t opcode,
                                 const EthernetAddress& target_ethernet_address,
                                 uint32_t target_ip_address ) const noexcept -> ARPMessage
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = ethernet_address_;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}