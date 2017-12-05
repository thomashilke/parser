#include "../src/parser/parse_input.hpp"

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

template<typename s_type>
struct dummy_token {
  using symbol_type = s_type;
  
  symbol_type symbol;
};

template<typename symbol_t>
class dummy_token_source {
public:
  using symbol_type = symbol_t;
  using token_type = dummy_token<symbol_type>;
  
  dummy_token_source(const std::vector<symbol_type>& symbols): current_token(tokens.begin()) {
    for (const auto& s: symbols)
      tokens.push_back(token_type{s});

    current_token = tokens.begin();
  }
  const token_type& get() const {
    return *current_token;
  }

  void next() {
    ++current_token;
  }

private:
  std::vector<token_type> tokens;
  typename std::vector<token_type>::iterator current_token;
};

int main() {
  try {
    cf_grammar<symbol> g(symbol::start);
    g.add_production(symbol::start, {symbol::number_list, symbol::eoi});
    g.add_production(symbol::number_list, {symbol::number});
    g.add_production(symbol::number_list, {symbol::number, symbol::comma, symbol::number_list});

    g.wrap_up();

    lr_parser<symbol> p(g);
 
    dummy_token_source<symbol> tokens({symbol::number, symbol::comma, symbol::number, symbol::comma, symbol::number, symbol::eoi});

    if (parse_input<dummy_token_source<symbol> >(p, tokens))
      std::cout << "parse succeed" << std::endl;
    else
      std::cout << "parse failed" << std::endl;
  }
  catch (const std::string& e) {
    std::cout << e << std::endl;
  }
  
  return 0;
}
