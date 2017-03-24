#ifndef _STRING_BUILDER_H_
#define _STRING_BUILDER_H_

#include <sstream>
#include <string>
#include <iomanip>

/*
 * Helper class to build a std::string from
 * multiple parts of different types, whose
 * operator<<(std::iostream&, . ) is defined.
 */
class StringBuilder
{
  std::ostringstream oss;
public:
  template<typename T>
  explicit StringBuilder(const T& val): oss()
  {oss << val;}

  template<typename T>
  StringBuilder& operator()(const T& val)
  { oss << val; return *this; }
  
  // Append an integer left padded with zeros
  StringBuilder& operator()(int i, unsigned int width)
  { 
    oss << std::right << std::setfill('0') << std::setw(width) << i;
    return *this;
  }
  
  // Conversions to std::string
  std::string str(){return oss.str();}
  operator std::string(){return oss.str();}
};

#endif /* _STRING_BUILDER_H_ */
