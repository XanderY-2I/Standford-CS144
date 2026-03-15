#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if(writer().has_error()) return ;//自检
  if(message.RST)//紧急熔断
  {
    reader().set_error();
    return;
  }
  if(!zero_point_.has_value())//建立连接
  {
    if(!message.SYN) return;
    zero_point_.emplace(message.seqno);
  }
    // 1. 计算当前报文起始编号的绝对序列号
    uint64_t checkpoint = reassembler_.writer().bytes_pushed();
    uint64_t abs_seqno = message.seqno.unwrap( zero_point_.value(), checkpoint );

    // 2. 确定 stream_index
    // 包含 SYN，那么 payload 的第一个字节对应的绝对序号就是 abs_seqno + 1,否则为abs_seqno
    // 然后再统一减 1 得到 stream_index
    uint64_t first_byte_abs_seqno = abs_seqno + (message.SYN ? 1 : 0);
    
    //边界检查：如果这个包只有SYN且没有数据，first_byte_abs_seqno 会是1。
    //但如果收到一个非法的、在 SYN 之前的包，first_byte_abs_seqno 可能是0。
    if (first_byte_abs_seqno == 0) return; //绝对序号 0 是给 SYN 专用的，数据必须从 1 开始
    
    uint64_t stream_idx = first_byte_abs_seqno - 1;

    reassembler_.insert( stream_idx, std::move( message.payload ), message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const 
{
    // 1. 计算接收窗口大小 (window_size)大小 = 字节流当前的剩余容量。
    // 由于 TCP 报文中的 window_size 只有 16 位，所以必须与 UINT16_MAX 取最小值，防止溢出。
    uint16_t window_size = static_cast<uint16_t>( 
        min( writer().available_capacity(), static_cast<uint64_t>( UINT16_MAX ) ) 
    );

    bool rst = writer().has_error();// 2. 检查错误状态 (RST)

    // 3. 计算确认号 (ackno)
    if ( zero_point_.has_value() ) {
        /**
         * 重点逻辑：计算“我方期待收到的下一个绝对序列号”
         * 绝对序列号 (abs_seqno) 的构成：
         * - 第 0 位：SYN (必占 1 字节)
         * - 中间：已经 push 到 ByteStream 的数据字节数 (writer().bytes_pushed())
         * - 最后：如果流已关闭 (FIN)，FIN 也要占 1 字节。
         * * 所以：下一位的绝对序号 = 1 (SYN) + bytes_pushed + (1 if FIN_is_pushed_and_closed)
         */
        uint64_t next_abs_seqno = 1 + writer().bytes_pushed();
        
        // 如果 writer() 已经关闭，说明 FIN 已经被重组并写入了流，此时需要给 FIN 留出一个序号
        if ( writer().is_closed() ) {
            next_abs_seqno += 1;
        }

        // 将 64 位的绝对序号转回 32 位的 Wrap32 格式
        Wrap32 ackno = Wrap32::wrap( next_abs_seqno, zero_point_.value() );
        return { ackno, window_size, rst };
    }
    
    return { std::nullopt, window_size, rst };// 如果还没收到过 SYN，ackno 应该为空 (std::nullopt)
}
