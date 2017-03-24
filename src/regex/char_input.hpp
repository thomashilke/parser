#ifndef _CHAR_INPUT_H_
#define _CHAR_INPUT_H_

#include <istream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

class CharInput {
 public:
  struct Coordinates {
    std::size_t line_number;
    std::size_t column_number;

    Coordinates(std::size_t l,
                std::size_t c)
        : line_number(l),
          column_number(c) {}
  };
  
  CharInput(std::istream* s = NULL)
      : line_number(0),
        column_number(0),
        stream(s),
        buffer(0),
        start_index(0) {}
  
  bool get(std::size_t pos, char& c) {
    if (increase_buffered_data(start_index + pos + 1)) {
      c = buffer[start_index + pos];
      return true;
    }
    return false;
  }

  std::size_t available_bytes_count() const {
    return buffer.size() - start_index;
  }
  
  bool good() {
    if (buffer.size() == 0)
      increase_buffered_data(16);
    
    return (buffer.size() - start_index) > 0;
  }

  std::string extract_substring(std::size_t length) {
    if (increase_buffered_data(start_index + length)) {
      std::string s(buffer.begin() + start_index,
                    buffer.begin() + start_index + length);
      update_coordinates(length);
      start_index += length;
      purge();
      return s;
    } else {
      throw std::string("CharInput::extract_substring(length)"
                        " - not enought available bytes.");
    }
  }

  bool advance(std::size_t distance) {
    if (increase_buffered_data(start_index + distance + 1)) {
      start_index += distance;
      return true;
    } else {
      return false;
    }
  }

  Coordinates get_coordinates() {
    return Coordinates(line_number, column_number);
  }

  void set_input_stream(std::istream& input_stream) {
    stream = &input_stream;

    buffer.clear();

    start_index = 0;

    line_number = 0;
    column_number = 0;
  }
  
 private:
  std::size_t line_number;
  std::size_t column_number;
  
  std::istream* stream;
  std::vector<char> buffer;
  std::size_t start_index;

  bool increase_buffered_data(std::size_t length) {
    if (not stream)
      return false;
    
    if (buffer.size() < length) {
      const std::size_t old_size(buffer.size());
      buffer.resize(length);

      stream->read(&buffer[old_size], buffer.size() - old_size);
      std::size_t bytes_read(stream->gcount());
      if (bytes_read < buffer.size() - old_size) {
        buffer.resize(old_size + bytes_read);
        return false;
      } else {
        return true;
      }
    } else {
      return true;
    }
  }

  void purge() {
    std::memmove(&buffer[0], &buffer[start_index], buffer.size() - start_index);
    buffer.resize(buffer.size() - start_index);
    start_index = 0;
  }

  void update_coordinates(std::size_t length) {
    std::size_t crossed_lines_number(std::count(buffer.begin() + start_index,
                                                buffer.begin() + start_index + length,
                                                '\n'));
    line_number += crossed_lines_number;
    
    if (crossed_lines_number > 0) {
      column_number = start_index
          - std::distance(std::find(buffer.rend() - start_index - length,
                                    buffer.rend() - start_index,
                                    '\n'),
                          buffer.rend());
    } else {
      column_number += length;
    }
  }
};

#endif /* _CHAR_INPUT_H_ */
