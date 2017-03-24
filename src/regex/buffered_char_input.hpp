#ifndef _BUFFERED_CHAR_INPUT_H_
#define _BUFFERED_CHAR_INPUT_H_

#include <istream>
#include <iterator>
#include <deque>
#include <cmath>
#include <limits>

class BufferedCharInput
{
public:
  BufferedCharInput(std::istream& stream): char_buffer(),
                                           input_stream(stream),
                                           block_size(4)
  { feedBuffer(block_size); }

  bool good(){ return input_stream.good(); }
  bool eof(){ return input_stream.eof(); }

  class iterator;

  iterator discard(const iterator& it);
  iterator discard(std::size_t n);

  iterator begin();
  iterator end();

private:
  std::size_t feedBuffer(std::size_t n){
    if(input_stream.good())
      {
        const std::size_t original_size(char_buffer.size());
        char_buffer.resize(original_size + std::max(block_size, n));

        std::deque<char>::iterator it(char_buffer.begin() + original_size);
        while(it != char_buffer.end() and input_stream.good())
          {
            input_stream.get(*it);
            if(input_stream.good()) ++it;
          }

        if(it != char_buffer.end())
          char_buffer.resize(char_buffer.size() - std::distance(it, char_buffer.end()));
               
        return char_buffer.size() - original_size;
      }
    return 0;
  }

  std::deque<char> char_buffer;
  std::istream& input_stream;
  std::size_t block_size;
};

class BufferedCharInput::iterator
{
  friend class BufferedCharInput;
  friend std::ostream& operator<<(std::ostream& flux, const iterator& it);

public:
  typedef char value_type;
  typedef value_type* pointer;
  typedef value_type& reference;
  typedef std::forward_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;


  iterator(): input(NULL), pos(std::numeric_limits<std::size_t>::max()) {}
  iterator(const iterator& it): input(it.input), pos(it.pos){}


  iterator& operator=(const iterator& it){ input = it.input; pos = it.pos; return *this; }

  const char& operator*() const { return input->char_buffer[pos]; }

  iterator& operator++(){ advance(1); return *this; }
  iterator operator++(int){ iterator tmp(*this); advance(1); return tmp; }

  bool operator==(const iterator& it) const
  { return pos == it.pos; }
  bool operator!=(const iterator& it) const { return not this->operator==(it); }

  bool operator<(const iterator& it) const { return pos < it.pos; }
  bool operator<=(const iterator& it) const { return pos <= it.pos; }

  bool operator>(const iterator& it) const { return not this->operator<=(it); }
  bool operator>=(const iterator& it) const { return not this->operator<(it); }
            
  iterator& operator+=(int n){ advance(n); return *this; }
  iterator operator+(int n) const { return iterator(*this) += n; }

  std::ptrdiff_t operator-(const iterator& it) const { return pos - it.pos; }

private:
  void advance(std::ptrdiff_t n){
    if(n < 0){
      throw std::string("BufferedCharInput::iterator::advance(std::size_t n) - n < 0 forbidden.");
    }else if(n > 0){
      if(pos < std::numeric_limits<std::size_t>::max()){
        if(pos + n >= input->char_buffer.size()
           and pos + n >= input->char_buffer.size() + input->feedBuffer(n)){
          pos = std::numeric_limits<std::size_t>::max();
        }else
          pos += n;
      }
    }
  }

  iterator(BufferedCharInput* _input,
           std::size_t _pos = std::numeric_limits<std::size_t>::max()): input(_input),
                                                                        pos(_pos){}

  BufferedCharInput* input;
  std::size_t pos;
};

inline
BufferedCharInput::iterator BufferedCharInput::discard(const iterator& it)
{ char_buffer.erase(char_buffer.begin(), char_buffer.begin() + it.pos); return begin(); }

inline
BufferedCharInput::iterator BufferedCharInput::discard(std::size_t n)
{ char_buffer.erase(char_buffer.begin(), char_buffer.begin() + n); return begin(); }

inline
BufferedCharInput::iterator BufferedCharInput::begin()
{ return iterator(this, 0); }

inline
BufferedCharInput::iterator BufferedCharInput::end()
{ return iterator(this); }

std::ostream&
operator<<(std::ostream& flux, const BufferedCharInput::iterator& it){
  flux << "iterator(" << it.input << ", " << it.pos << ")";
  return flux;
}


#endif /* _BUFFERED_CHAR_INPUT_H_ */
