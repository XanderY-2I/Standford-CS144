#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // 当前窗口范围
  uint64_t window_start_idx = next_expected_idx_;
  uint64_t window_end_idx = window_start_idx + writer().available_capacity();

  // 当前数据范围
  uint64_t cur_start_idx = first_index;
  uint64_t cur_end_idx = cur_start_idx + data.size();

  // 标记 EOF 位置
  if ( is_last_substring )
    eof_idx_ = cur_end_idx;

  // 裁剪到窗口内
  cur_start_idx = max( cur_start_idx, window_start_idx );
  cur_end_idx = min( cur_end_idx, window_end_idx );

  // 无有效数据
  if ( cur_start_idx >= cur_end_idx ) {
    if ( next_expected_idx_ == eof_idx_ )
      output_.writer().close();
    return;
  }

  // 插入区间
  buf_.insert(
    { cur_start_idx, cur_end_idx, data.substr( cur_start_idx - first_index, cur_end_idx - cur_start_idx ) } );

  // 合并重叠区间
  vector<Interval> merged;
  auto it = buf_.begin();
  Interval last = *it;
  it++;

  while ( it != buf_.end() ) {
    if ( it->start_idx <= last.end_idx ) {
      // 有重叠，合并
      if ( last.end_idx < it->end_idx ) {
        last.data += it->data.substr( last.end_idx - it->start_idx );
        last.end_idx = it->end_idx;
      }
    } else {
      merged.push_back( last );
      last = *it;
    }
    it++;
  }
  merged.push_back( last );

  buf_ = set<Interval>( merged.begin(), merged.end() );

  // 尝试写入连续数据
  it = buf_.begin();
  if ( it->start_idx == next_expected_idx_ ) {
    output_.writer().push( it->data );
    next_expected_idx_ = it->end_idx;
    it = buf_.erase( it );
  }

  // 判断是否结束
  if ( next_expected_idx_ == eof_idx_ )
    output_.writer().close();

  debug( "insert({}, {}, {}) called", first_index, data, is_last_substring );
}

// 返回当前缓存的未写入字节数（测试用）
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t bytes_pending = 0;
  for ( const auto& itv : buf_ )
    bytes_pending += itv.end_idx - itv.start_idx;

  debug( "count_bytes_pending() called" );
  return bytes_pending;
}