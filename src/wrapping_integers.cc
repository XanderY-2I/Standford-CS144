#include "wrapping_integers.hh"
#include "debug.hh"
using namespace std;
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 {zero_point+static_cast<uint32_t>(n)};
}
uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
    // 1. 算出 checkpoint 对应的 32 位序列号
    Wrap32 ck_wrap = wrap(checkpoint, zero_point);
    
    // 2. 核心：计算当前 seqno 与 checkpoint 之间的 32 位有符号差值
    // C++ 中，两个 uint32_t 相减并存入 int32_t，会自动得到“最短距离”
    // 比如：1 - 0 = 1 (顺时针 1 步) 0 - 1 = -1 (逆时针 1 步)
    int32_t difference = this->raw_value_ - ck_wrap.raw_value_;//无符号数赋值给有符号数，会自动变为最近0的数
    
    // 3. 在 64 位视角下，将这个“位移”应用到 checkpoint 上
    // 注意：这里要用有符号的 int64_t 承接，防止 result 变成负数时溢出
    int64_t result = static_cast<int64_t>(checkpoint) + difference;
    
    // 4. 边界处理：绝对序列号不能为负
    // 如果 result < 0，说明我们跳到 0 左边去了，那么最近的合法位置只能是右侧那个点
    if (result < 0) {
        return result + (1UL << 32);
    }
    return static_cast<uint64_t>(result);
}
