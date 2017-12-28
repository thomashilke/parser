#ifndef CF_GRAMMAR_H
#define CF_GRAMMAR_H

#include <vector>
#include <set>
#include <utility>
#include <algorithm>

/**
 * An ordered list of symbols. This type is usually used to represent
 * the right hand side of a production rule.
 */
template<typename symbol_type>
using  symbol_list_type = std::vector<symbol_type>;

/**
 * A context-free grammar production rule. The left hand side is a single symbol,
 * and the right hand side is a ordered list of symbols.
 */
template<typename symbol_type>
using production_type = std::pair<symbol_type, symbol_list_type<symbol_type> >;

/**
 * Representation of a context free (CF) grammar. A CF grammar is the tuple
 * (Symbols, Start, Productions), where Symbols is the set union of the terminal
 * and non terminal symbols involved in the Production rules, Start is the
 * initial symbol to which a parse tree must reduce, and Productions is a
 * set of ordered CF production rules.
 *
 * This class should be inherited to implement a specific grammar.
 */
template<typename symbol_type>
class cf_grammar {
public:
  cf_grammar(symbol_type s): terminals(),
                             non_terminals(),
                             symbol_set(),
                             start_symbol(s),
                             production_rules() {}
  virtual ~cf_grammar() {}

  /**
   * This method must be called once all the production rules have been defined.
   * In particular, the terminals, nonTerminals and symbolSet member variable are
   * filled by analysing the productionRules.
   *
   * The set of non terminal is built by collecting all the right hand side
   * of the production rules, the set of symbols by collecting all the symbols
   * in the rhs and lhs, and finally the set of terminal is built by taking the
   * set difference: Terminals = Symbols - NonTerminals.
   */
  void wrap_up();


  /**
   * Utility template method to add a production rule to the grammar from
   * a symbol and an plain array of symbols.
   * \param target Left hand side of the production rule
   * \param rhs Right hand side of the production rule, which is a C array of symbols.
   *
   * Usage example:
   * \code{.cpp}
   * CFGrammar cfg;
   *
   * Symbols rhs[] = {sym1, sym2, sym3};
   * cfg.addProduction(symTarget, rhs);
   * \endcode
   */
  template<size_t length>
  void add_production(const symbol_type& target,
                      const symbol_type (&rhs)[length]) {
    production_rules.push_back(std::make_pair(target, symbol_list_type<symbol_type>(rhs, rhs + length)));
  }

  void add_production(const symbol_type& target,
                      const symbol_list_type<symbol_type>& list) {
    production_rules.push_back(std::make_pair(target, list));
  }

  void print(std::ostream& stream) const {
    stream << "Set of terminal symbols: ";
    for (const auto& t: terminals)
      stream << t << " ";
    stream << std::endl;
    
    stream << "Set of non-terminal symbols: ";
    for (const auto& t: non_terminals)
      stream << t << " ";
    stream << std::endl;
    
    stream << "Start symbol: " << start_symbol << std::endl;
    
    stream << "Production rules:" << std::endl;
    std::size_t rule_id(0);
    for (const auto& p: production_rules) {
      stream << rule_id << ":  " << p.first << " -> ";
      for (const auto& s: p.second)
        stream << s << " ";
      stream << std::endl;
      ++rule_id;
    }
  }

  const std::vector<production_type<symbol_type>>& get_production_rules() {
    return production_rules;
  }

  symbol_type get_important_goal(symbol_type s, const std::set<symbol_type>& candidates) {
    bool done(false);
    while (not done) {
      done = true;
      for (const auto& p: production_rules)
        if (p.second.size() == 1
            and p.second.front() == s
            and candidates.count(p.first)) {
          s = p.first;
          done = false;
        }
    }
    return s;
  }
  
public:
  std::vector<symbol_type> terminals;
  std::vector<symbol_type> non_terminals;
  std::vector<symbol_type> symbol_set;

  symbol_type start_symbol;

  std::vector<production_type<symbol_type>> production_rules;

  bool is_terminal(const symbol_type& s) const;
  bool is_non_terminal(const symbol_type& s) const {
    return std::find(non_terminals.begin(),
                     non_terminals.end(),
                     s) != non_terminals.end();
  }
};


template<typename symbol_type>
void cf_grammar<symbol_type>::wrap_up() {
  std::set<symbol_type> s, nt;
  for (unsigned int i(0); i < production_rules.size(); ++i) {
    nt.insert(production_rules[i].first);
    s.insert(production_rules[i].first);
    for (unsigned int j(0); j < production_rules[i].second.size(); ++j)
      s.insert(production_rules[i].second[j]);
  }
  
  terminals.resize(s.size());
  typename std::vector<symbol_type>::iterator
    it(std::set_difference(s.begin(), s.end(), 
                           nt.begin(), nt.end(), 
                           terminals.begin()));
  terminals.resize(it - terminals.begin());

  non_terminals.insert(non_terminals.begin(), nt.begin(), nt.end());
  symbol_set.insert(symbol_set.begin(), s.begin(), s.end());
}

#endif /* CF_GRAMMAR_H */
