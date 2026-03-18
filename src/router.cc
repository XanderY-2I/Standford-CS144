#include "router.hh"
#include <iostream>

using namespace std;

// 将路由项添加到路由表
void Router::add_route( const uint32_t route_prefix,const uint8_t prefix_length,const optional<Address> next_hop,const size_t interface_num )
{
  // 将路由信息存入对应前缀长度的哈希表中
  routing_table_[prefix_length][route_prefix] = { interface_num, next_hop };
}

// 实现最长前缀匹配查找
[[nodiscard]] auto Router::match( uint32_t addr ) const noexcept -> optional<info>
{
  // 从最长的前缀长度 32 开始递减匹配，确保找到的是“最长前缀” [cite: 88, 89]
  for ( int len = 32; len >= 0; --len ) 
  {
    uint32_t mask = ( len == 0 ) ? 0 : 0xFFFFFFFF << ( 32 - len ); 
    uint32_t masked_addr = addr & mask; // 获取目标 IP 在当前长度下的网络前缀

    // 在当前前缀长度的哈希表中查找
    if ( routing_table_[len].contains( masked_addr ) )
    {
      return routing_table_[len].at( masked_addr );
    }
  }
  return nullopt; // 若遍历完所有长度均未匹配，则返回空
}

// 路由器的主要处理流程
void Router::route()
{
  for ( const auto& interface : interfaces_ ) 
  { // 遍历所有接口
    auto&& datagrams_received { interface->datagrams_received() };
    while ( not datagrams_received.empty() ) 
    {
      InternetDatagram datagram { move( datagrams_received.front() ) };
      datagrams_received.pop();
      if ( datagram.header.ttl <= 1 ) continue;//转发前先-1，所以如果是1也跳过
      
      datagram.header.ttl -= 1;
      datagram.header.compute_checksum();// 2. 重新计算校验和（因为 TTL 改变了）
      const auto mp = match( datagram.header.dst ); // 3. 执行最长前缀匹配查找
      if ( not mp.has_value() ) continue; // 无匹配路由则丢弃
      const auto& [num, next_hop] = mp.value();
      // 下一跳为空，则直接发给目标 IP，否则发给指定的下一跳路由器
      interfaces_[num]->send_datagram( datagram, next_hop.value_or( Address::from_ipv4_numeric( datagram.header.dst ) ) 
      );
    }
  }
}