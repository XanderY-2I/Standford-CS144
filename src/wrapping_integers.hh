#pragma once
#include <cstdint>

class Wrap32
{
public:
  explicit Wrap32( uint32_t raw_value ) : raw_value_( raw_value ) {}
  
  // C++ 处理 uint32_t 时会自动处理溢出。
  //当你把一个 uint64_t 加上偏移量后强制转换为 uint32_t，结果自然就是取模 2^32后的值 。
  static Wrap32 wrap( uint64_t n, Wrap32 zero_point );//zero point就是ISN

  //逻辑：计算出一个64位值，使其满足“回绕后等于 n”，且“与 checkpoint 的绝对差值最小”
  uint64_t unwrap( Wrap32 zero_point, uint64_t checkpoint ) const;

  Wrap32 operator+( uint32_t n ) const { return Wrap32 { raw_value_ + n }; }
  bool operator==( const Wrap32& other ) const { return raw_value_ == other.raw_value_; }

protected:
  uint32_t raw_value_ {};
};
