#include <fstream>

#include <lexer/lexer.hpp>

#include "../src/parse_input.hpp"


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


regex_lexer<token<symbol> > build_regex_lexer() {
  regex_lexer_builder<token<symbol> > rlb(symbol::eoi); {
    rlb.emit(symbol::number, "\\d+");
    rlb.emit(symbol::comma, ",");

    rlb.skip("[\n ]*");
  }

  return rlb.build();
}


class dummy_token_source {
public:
  using symbol_type = symbol;
  using token_type = token<symbol_type>;
  
  dummy_token_source(const std::string& filename)
    : file(filename.c_str(), std::ios::in),
      source(&file, filename),
      lexer(build_regex_lexer()),
      current(nullptr) {
    lexer.set_source(&source);
    current = lexer.get();
  }

  ~dummy_token_source() {
    delete current;
  }

  const token<symbol>& get() const { return *current; }

  void next() {
    delete current;
    current = lexer.get();
  }
  
private:
  std::ifstream file;
  file_source<token<symbol> > source;
  regex_lexer<token<symbol> > lexer;

  token<symbol>* current;
};


class basic_node {
public:
  basic_node(symbol s): s(s) {}
  virtual ~basic_node() {}
  virtual void show(std::ostream& stream, unsigned int level = 0) const = 0;
  symbol get_symbol() const { return s; }
  
protected:
  symbol s;
};


class node: public basic_node {
public:
  template<typename iterator_type>
  node(symbol s, iterator_type begin, iterator_type end): basic_node(s), children(begin, end) {}

  void show(std::ostream& stream, unsigned int level) const {
    stream << std::string(level, ' ') << this->s << std::endl;
    for (const auto& c: children)
      c->show(stream, level + 2);
  }

  
private:
  std::vector<basic_node*> children;
};


class leaf: public basic_node {
public:
  leaf(symbol s, const std::string& v): basic_node(s), value(v) {}

  void show(std::ostream& stream, unsigned int level) const {
    stream << std::string(level, ' ') << this->s <<" (" << value << ")" << std::endl;
  }
  
private:
  std::string value;
};


template<typename symbol_t>
class tree_factory {
public:
  using symbol_type = symbol_t;
  using token_type = token<symbol_type>;
  using node_type = basic_node;
  
  node_type* build_node(std::list<node_type*>::iterator begin,
                        std::list<node_type*>::iterator end,
                        unsigned int /* rule_id */,
                        symbol_type symbol) {

    return new node(symbol, begin, end);
  }
  
  //node_type* build_leaf(const token_type& t) {
  node_type* build_leaf(dummy_token_source& src) {  
    return new leaf(src.get().symbol, src.get().value);
  }
};


int main() {
  try {
    cf_grammar<symbol> g(symbol::start);
    g.add_production(symbol::start, {symbol::number_list, symbol::eoi});
    g.add_production(symbol::number_list, {symbol::number});
    g.add_production(symbol::number_list, {symbol::number, symbol::comma, symbol::number_list});

    g.wrap_up();

    lr_parser<symbol> p(g);
 
    dummy_token_source tokens("input.txt");
    
    tree_factory<symbol> factory;
    default_error_handler<token<symbol>> handler;
    basic_node* tree(parse_input_to_tree<dummy_token_source,
                                         tree_factory<symbol> >(p, g, tokens, factory, handler));
    if (tree) {
      std::cout << "parse succeed" << std::endl;
      tree->show(std::cout);
    } else {
      std::cout << "parse failed" << std::endl;
    }
  }
  catch (const std::string& e) {
    std::cout << e << std::endl;
  }
  
  return 0;
}
