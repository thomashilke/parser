#include <fstream>

#include <lexer/lexer.hpp>

#include "../src/parse_input.hpp"

/*
 * Let's try to parse the following if-then-else-endif:
 *  boolean = /(true)|(false)/
 *  if = /if/
 *  then = /then/
 *  else = /else/
 *  endif = /endif/
 *
 *  start = expr eoi
 *  if-statment = if expr then expr else expr endif
 *  expr = if-statment | boolean
 */


enum class symbol { if_kw, then_kw, else_kw, endif_kw, literal_boolean, start, eoi, expr, if_stmt};


std::ostream& operator<<(std::ostream& stream, const symbol& s) {
  switch (s) {
  case symbol::if_kw: stream << "<if_kw>"; break;
  case symbol::then_kw: stream << "<then_kw>"; break;
  case symbol::else_kw: stream << "<else_kw>"; break;
  case symbol::endif_kw: stream << "<endif_kw>"; break;
  case symbol::literal_boolean: stream << "<literal_boolean>"; break;
  case symbol::start: stream << "<start>"; break;
  case symbol::eoi: stream << "<eoi>"; break;
  case symbol::expr: stream << "<expr>"; break;
  case symbol::if_stmt: stream << "<if_stmt>"; break;
  }
  return stream;
}

regex_lexer<token<symbol> > build_regex_lexer() {
  regex_lexer_builder<token<symbol> > rlb(symbol::eoi); {
    rlb.emit(symbol::if_kw, "if");
    rlb.emit(symbol::then_kw, "then");
    rlb.emit(symbol::else_kw, "else");
    rlb.emit(symbol::endif_kw, "endif");
    rlb.emit(symbol::literal_boolean, "(true)|(false)");
    
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
    current = nullptr;
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
  virtual ~basic_node() {}
  virtual void show(std::ostream& stream, unsigned int level = 0) const = 0;
};


class node: public basic_node {
public:
  template<typename iterator_type>
  node(symbol s, iterator_type begin, iterator_type end): s(s), children(begin, end) {}

  void show(std::ostream& stream, unsigned int level) const {
    stream << std::string(level, ' ') << s << std::endl;
    for (const auto& c: children)
      c->show(stream, level + 2);
  }
  
private:
  symbol s;
  std::vector<basic_node*> children;
};


class leaf: public basic_node {
public:
  leaf(symbol s, const std::string& v): s(s), value(v) {}

  void show(std::ostream& stream, unsigned int level) const {
    stream << std::string(level, ' ') << s <<" (" << value << ")" << std::endl;
  }
  
private:
  symbol s;
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
  
  node_type* build_leaf(dummy_token_source& src) {
    return new leaf(src.get().symbol, src.get().value);
  }
};


int main() {
  try {
    cf_grammar<symbol> g(symbol::start);
    g.add_production(symbol::start, {symbol::expr, symbol::eoi});
    g.add_production(symbol::expr, {symbol::literal_boolean});
    g.add_production(symbol::expr, {symbol::if_stmt});
    g.add_production(symbol::if_stmt, {symbol::if_kw, symbol::expr, symbol::then_kw,
          symbol::expr,
          symbol::else_kw,
          symbol::expr,
          symbol::endif_kw});

    g.wrap_up();

    lr_parser<symbol> p(g);
 
    dummy_token_source tokens("grammar_experiment_input.txt");
    
    tree_factory<symbol> factory;
    basic_node* tree(parse_input_to_tree<dummy_token_source, tree_factory<symbol> >(p, tokens, factory));
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
