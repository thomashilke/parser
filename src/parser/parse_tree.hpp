#ifndef _PARSE_TREE_H_
#define _PARSE_TREE_H_

namespace parse_tree {

  template<typename token_type>
  class basic_visitor;
  
  template<typename token_type>
  class node {
  public:
    virtual ~node() {}
    virtual void accept(basic_visitor<token_type>* v) = 0;
  };

  template<typename token_type>
  class terminal: public node<token_type> {
  public:
    terminal(const token_type& t);
    virtual ~terminal() {}

    virtual void accept(basic_visitor<token_type>* v) { v->visit(this); }
    
  private:
    token_type t;
  };

  template<typename token_type>
  class production: public node<token_type> {
  public:
    typedef typename token_type::symbol_type symbol_type;
    production(const symbol_type& s);
    virtual ~production() {}

    virtual void accept(basic_visitor<token_type>* v) { v->visit(this); }
    void add_child(node* n) { children.push_back(n); }
    
  private:
    symbol_type s;
    std::vector<node*> children;
  };

  template<typename token_type>
  class basic_visitor {
  public:
    virtual void visit(terminal<token_type>* t) = 0;
    virtual void visit(production<token_type>* p) = 0;
  };
}

#endif /* _PARSE_TREE_H_ */
