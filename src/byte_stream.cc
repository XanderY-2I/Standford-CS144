#include "byte_stream.hh"
#include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : isClosed_( false ), buf_(), bytes_popped_( 0 ), bytes_pushed_( 0 ), capacity_( capacity )
{}

// Push data to stream, but only as much as available capacity allows.
void Writer::push( string data )
{
  // Your code here (and in each method below)
  if ( isClosed_ )
    return;
  int len = min( data.size(), available_capacity() );
  buf_.append( data.substr( 0, len ) );
  bytes_pushed_ += len;
  debug( "Writer::push({}) not yet implemented", data );
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  isClosed_ = true;
  debug( "Writer::close() not yet implemented" );
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  debug( "Writer::is_closed() not yet implemented" );
  return isClosed_; // Your code here.
}
// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  debug( "Writer::available_capacity() not yet implemented" );
  return capacity_ - buf_.size(); // Your code here.
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  debug( "Writer::bytes_pushed() not yet implemented" );
  return bytes_pushed_; // Your code here.
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  if ( buf_.empty() )
    return {};
  debug( "Reader::peek() not yet implemented" );
  return string_view( buf_ ); // Your code here.
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  len = min( len, buf_.size() );
  buf_ = buf_.substr( len );
  bytes_popped_ += len;
  debug( "heReader::pop({}) not yet implemented", len );
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  debug( "Reader::is_finished() not yet implemented" );
  return buf_.empty() && isClosed_; // Your code here.
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  debug( "Reader::bytes_buffered() not yet implemented" );
  return buf_.size(); // Your code here.
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  debug( "Reader::bytes_popped() not yet implemented" );
  return bytes_popped_; // Your code here.
}
