#pragma once

#include "exception.hh"
#include "network_interface.hh"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

// 路由器：拥有多个网络接口，并根据“最长前缀匹配”规则转发数据包
class Router
{
public:
  // 添加一个网络接口
  // 返回该接口在路由器中的索引
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    interfaces_.push_back( notnull( "add_interface", std::move( interface ) ) );
    return interfaces_.size() - 1;
  }

  // 根据索引访问某个接口
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return interfaces_.at( N ); }

  // 添加一条路由规则（转发表项）
  void add_route( uint32_t route_prefix,uint8_t prefix_length,std::optional<Address> next_hop,size_t interface_num );

  // 在各接口之间转发数据包
  void route();

private:

  std::vector<std::shared_ptr<NetworkInterface>> interfaces_ {};//网络接口集合

  using info = std::pair<size_t, std::optional<Address>>; //转发信息别名

  std::array<std::unordered_map<uint32_t, info>, 33> routing_table_ {};//路由表

  [[nodiscard]] auto match( uint32_t ) const noexcept -> std::optional<info>;//最长前缀匹配
};