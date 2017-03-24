#ifndef _STREAM_ARRAY_H_
#define _STREAM_ARRAY_H_

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <numeric>

#include "string_builder.hpp"


class StreamArray
{
  std::ostream& flux;
  
  std::vector<std::string> cellValues;
  std::vector<std::size_t> lineBreaks;

  std::string separator;
  
  std::vector<bool> alignment;
public:
  StreamArray(std::ostream& _flux,
              const std::string& sep = " "): flux(_flux),
                                             cellValues(),
                                             lineBreaks(),
                                             separator(sep),
                                             alignment()
  {}

  struct EndOfRow{};  static EndOfRow endr;
  struct BlankCell{}; static BlankCell blank;
  struct AlignCol{bool left;}; static AlignCol left, right;

  template<typename T>
  StreamArray& operator<<(const T& value)
  {
    cellValues.push_back(StringBuilder(value).str());
    return *this;
  }

  struct Align{
    std::vector<bool>& alignment;
    Align(std::vector<bool>& _alignment): alignment(_alignment) {}
    Align& operator|(const AlignCol& a)
    {alignment.resize(alignment.size()+1); alignment.back() = a.left; return *this;}
  };
  
  Align operator|(const AlignCol& a)
  {alignment.resize(1); alignment.back()=  a.left; return Align(alignment); }

  void flush();
};


/*
 * Specialization for the end of row and blank cell symbols:
 */
template<>
StreamArray& StreamArray::operator<< <StreamArray::EndOfRow>(const StreamArray::EndOfRow&);

template<>
StreamArray& StreamArray::operator<< <StreamArray::BlankCell>(const StreamArray::BlankCell&);

#endif /* _STREAM_ARRAY_H_ */

