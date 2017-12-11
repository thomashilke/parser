#include <iostream>

#include "../src/cf_grammar.hpp"

enum class symbol { start, eoi, number, comma, number_list };

std::ostream& operator<<(std::ostream& stream, const symbol& s) {
  switch (s) {
  case symbol::start: stream << "<start>"; break;
  case symbol::eoi: stream << "<eoi>"; break;
  case symbol::number: stream << "<number>"; break;
  case symbol::comma: stream << "<comma>"; break;
  case symbol::number_list: stream << "<number_list>"; break;
  }
  return stream;
}

int main() {

  cf_grammar<symbol> g(symbol::start);
  g.add_production(symbol::start, {symbol::number_list, symbol::eoi});
  g.add_production(symbol::number_list, {symbol::number});
  g.add_production(symbol::number_list, {symbol::number, symbol::comma, symbol::number_list});

  g.wrap_up();

  g.show(std::cout);
  
  return 0;
}
