#ifndef _BUFFERED_ISTREAM_H_
#define _BUFFERED_ISTREAM_H_

#include <vector>

class forward_streambuf: public std::streambuf
{
public:
  forward_streambuf(std::streambuf& underlying_buffer): std::streambuf(), buffer(&underlying_buffer){}
  //virtual void imbue(const locale&);

  //virtual buffered_streambuf* setbuf(char* s, std::streamsize n);
  //virtual std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which){ return std::streampos(std::streamoff(-1));}
  //virtual std::streampos seekpos(std::streampos pos, std::ios_base::openmode which){ return std::streampos(-1);}

  //virtual int sync(){ return 0; }
  
  //virtual std::streamsize showmanyc(){ return 0; }
  //virtual std::streamsize xsgetn(char* s, std::streamsize n){ return 0;}
  virtual int_type underflow(){ return traits_type::eof(); }
  //virtual int_type uflow(){ return traits_type::eof(); }
  //virtual int_type pbackfail(int){ return traits_type::eof(); }
  

  //virtual std::streamsize xsputn(const char* s, std::streamsize n){ return 0; }
  //virtual int overflow(){ return traits_type::eof(); }

  class iterator;
private:
  std::streambuf* stream_buffer;
  std::vector<char> byte_buffer;
  
  std::list<std::vector<char>::iterator> existing_iterators;
};

class forward_streambuf::iterator
{
public:
  typedef char value_type;
  typedef value_type& reference;
  typedef value_type* pointer;
  typedef std::ptrdiff_t difference_type;
  typedef std::forward_iterator_tag category_type;

  // general
  iterator(const iterator& it);
  iterator& operator=(const iterator& it);
  iterator& operator++();
  iterator operator++(int);

  // input
  bool operator==(const iterator& it);
  bool operator!=(const iterator& it);
  
  reference operator*();
  pointer operator->();

  // forward, + multi-pass
  iterator();

private:
  forward_streambuf streambuf;
  
};


class buffered_istream: public std::istream
{
public:
  buffered_istream(std::streambuf& underlying_buffer): std::istream(new forward_streambuf(underlying_buffer)) {}
  virtual ~buffered_istream(){ delete std::istream::rdbuf(); }
};

#endif /* _BUFFERED_ISTREAM_H_ */
