#ifndef _PARSER_H_
#define _PARSER_H_

#include <iostream>
#include <sstream>
#include <list>
#include <set>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <iomanip>
#include <queue>

#include "cf_grammar.hpp"



/**
 * A ParserItem is the representation of a dot in a production rule. The position of the
 * dot defines the partial state of the parser, after eating some symbols.
 */
struct parser_item {
  /**
   * A ParserItem is associated with one of the production rule of a CFGrammar.
   * productionId is the index of the production rule in the 
   * std::vector<Production> CFGrammar::productionRules .
   */
  unsigned int production_id;

  /**
   * Position of the dot in the symbol list of the right hand side of the 
   * production rule. 
   *
   * An integer value of i means that the parser state is between symbols i-1 and i. In
   * particular, if i == 0, then the dot precede the first symbol, and if i == rhs.size(),
   * then the dot is after the last symbol of the right hand side. 
   */
  unsigned int parser_position;

  parser_item(unsigned int id, unsigned int pos): production_id(id),
                                                  parser_position(pos) {}
};

inline bool operator<(const parser_item& op1, const parser_item& op2) {
  return (op1.production_id == op2.production_id)
    ? (op1.parser_position < op2.parser_position)
    : (op1.production_id < op2.production_id);
}

inline bool operator==(const parser_item& op1, const parser_item& op2) {
  return op1.production_id == op2.production_id
    and op1.parser_position == op2.parser_position;
}


/** 
 * \brief Configuration state of a LR parser
 *
 * A valid configuration state of an LR parser is a collection of parser Items,
 * which satisfy some constraints (The set of parser states defining a parser is
 * built by iterating through all the nodes of the graph, stating from the initial
 * state).
 */
typedef std::set<parser_item> parser_state_type;


template<typename symbol_type>
void print(std::ostream& stream,
           const parser_item& item,
           const cf_grammar<symbol_type>& grammar) {
  const production_type<symbol_type>& p(grammar.production_rules[item.production_id]);
  stream << p.first << " -> ";
  
  unsigned int pos(0);
  for (typename symbol_list_type<symbol_type>::const_iterator sym(p.second.begin());
       sym != p.second.end();
       ++sym, ++pos) {
    if(item.parser_position == pos)
      stream << ". ";
    stream << *sym << " ";
  }
  
  if(item.parser_position == pos)
    stream << ".";
}

template<typename symbol_type>
void print(std::ostream& stream,
           const parser_state_type& state,
           const cf_grammar<symbol_type>& grammar) {
  for (parser_state_type::iterator it(state.begin()); it != state.end(); ++it) {
    ::print(stream, *it, grammar);
    std::cout << std::endl;
  }
}


/**
 * \brief Representation of the LR parser associated to a CF grammar
 * 
 * An LR parser is defined by the transitions and goto tables, which
 * are built at initialization from the CF grammar production rules.
 * 
 */
template<typename symbol_type>
class lr_parser {
  void build_configuration_set(const cf_grammar<symbol_type>& grammar);
  void build_transition_table(const cf_grammar<symbol_type>& grammar);
  void build_first_sets(const cf_grammar<symbol_type>& grammar);
  void build_follow_sets(const cf_grammar<symbol_type>& grammar);

public:
  /** 
   * \brief The set of configuration state for this parser
   * 
   * \c configurationSet is the set of all the valid parser states. This member is only filled and
   * used in the parser initialization step, in particular it is used to build the 
   * transitions and goto tables. This is why THIS SHOULD NOT BE A MEMBER DATA.
   * However, this should not be moved away, since it is also used for PRINTING AND DEBUGING.
   */
  std::vector<parser_state_type> configuration_set;

  /**
   * \brief The state transition map
   *
   * The state transition function maps the couple (\c ParserState, \c Symbol) to the next \c ParserState. 
   * \c transitionTable[i][s] is the next state if the current state is \c i, and the next symbol on the
   * input is \c s.
   */
  std::vector<std::vector<short int>> transitions_table;

  /**
   * \brief The goto state map
   *
   * The goto state map associate to the couple (\c ParserState, \c Symbol) the state to be pushed on the
   * stack once the production rule has been reduced to \c Symbol.
   */
  std::vector<std::vector<short int>> goto_table;

  /**
   * \brief The state which reduce the start production rule.
   *
   * When this state is reached, the parsing process can stop.
   */
  std::size_t accepting_state;
  
  // Cached data (extracted from the grammar): Length of the rhs of each grammar rule:
  std::vector<unsigned int> rule_lengths;

  // Cached data (extracted from the grammar): The symbol on the lhs of each grammar rules:
  std::vector<symbol_type> reduce_symbol;

  // Cached data (extracted from the grammar): Maps between symbol and index in the transition and goto tables:
  std::map<symbol_type, unsigned int> non_terminal_map;
  std::map<symbol_type, unsigned int> terminal_map;

  // First and follow sets for each symbol. As with the \c configurationSet, these members are only used
  // during the constuction of the transitions and goto tables. They could be moved away, but they are
  // convenient for debuging purpose.
  std::map<symbol_type, std::set<symbol_type>> firsts;
  std::map<symbol_type, std::set<symbol_type>> follows;
  
  /**
   * \brief Build a LRParser from a context free grammar \c g.
   *
   * Several steps take place in order to build a functional left-right parser.
   * First of all, the
   */
  lr_parser(const cf_grammar<symbol_type>& g);

  void print(std::ostream& stream, const cf_grammar<symbol_type>& grammar);
  void print_follow_sets(std::ostream& stream);
  void print_first_sets(std::ostream& stream);
  void print_configuration_set(std::ostream& stream, const cf_grammar<symbol_type>& grammar);

  /**
   * \brief Complete the partial parser state.
   *
   * Given a set of ParserItems (as a \c ParserState), \c closeParserState recursively iterate
   * through all the production rules, and add them with a dot in the first position
   * if required, until the \c ParserState is stable under closure. This lead to a uniquely
   * defined \c ParserState, which can then represent a valid state of a parser.
   */
  static void close_parser_state(parser_state_type& p,
                                 const cf_grammar<symbol_type>& grammar);

  // Find the successor state S(p, terminal)
  static parser_state_type compute_successor_parser_state(const parser_state_type& state,
                                                          const cf_grammar<symbol_type>& grammar,
                                                          const symbol_type& terminal);

  /**
   * \brief Check if p is a reducible state.
   *
   * A reducible state is a state is a ParserState which has exactly one
   * ParserItem whose dot is in the last position.
   *
   * If more than one ParserItem has a dot in the last position, we are facing a
   * reduce-reduce conflict, since one cannot choose whose rule to reduce without
   * looking at the following symbols on the input.
   */
  static bool is_reducible(const parser_state_type& p, 
                           const cf_grammar<symbol_type>& grammar,
                           unsigned int& production_id);

  /**
   * \brief Find the rule id which produce the symbol \c s.
   *
   * This function only return normally if a unique production rule is found.
   * If either more than one or no rule is found, a \c std::string is thrown.
   */
  static size_t find_production(const cf_grammar<symbol_type>& g, symbol_type s);
};

template<typename symbol_type>
void print(std::ostream& stream,
           const parser_item& item,
           const cf_grammar<symbol_type>& grammar);


template<typename symbol_type>
lr_parser<symbol_type>::lr_parser(const cf_grammar<symbol_type>& g):
  configuration_set(),
  transitions_table(),
  goto_table(),
  accepting_state(0),
  rule_lengths(g.production_rules.size(), 0),
  reduce_symbol(g.production_rules.size(), symbol_type()),
  non_terminal_map(),
  terminal_map(),
  firsts(),
  follows() {
  for (unsigned int i(0); i < rule_lengths.size(); ++i) {
    rule_lengths[i] = g.production_rules[i].second.size();
    reduce_symbol[i] = g.production_rules[i].first;
  }
    
  for (unsigned int i(0); i < g.terminals.size(); ++i)
    terminal_map[g.terminals[i]] = i;

  for (unsigned int i(0); i < g.non_terminals.size(); ++i)
    non_terminal_map[g.non_terminals[i]] = i;    

  build_first_sets(g);
  build_follow_sets(g);
  build_configuration_set(g);
  build_transition_table(g);
}

template<typename symbol_type>
void lr_parser<symbol_type>::print_first_sets(std::ostream& stream) {
  for (typename std::map<symbol_type, std::set<symbol_type> >::iterator first_set(firsts.begin());
       first_set != firsts.end(); ++first_set) {
    stream << "FIRST(" << first_set->first  << ") = { ";
    for (typename std::set<symbol_type>::iterator sym(first_set->second.begin());
         sym != first_set->second.end(); ++sym)
      stream << *sym << " ";
    stream << "}" << std::endl;
  }
}

template<typename symbol_type>
void lr_parser<symbol_type>::print_follow_sets(std::ostream& stream) {
  for (typename std::map<symbol_type, std::set<symbol_type> >::iterator follow_set(follows.begin());
       follow_set != follows.end(); ++follow_set) {
    stream << "FOLLOW(" << follow_set->first  << ") = { ";
    for (typename std::set<symbol_type>::iterator sym(follow_set->second.begin());
         sym != follow_set->second.end(); ++sym)
      stream << *sym << " ";
    stream << "}" << std::endl;
  }
}

template<typename symbol_type>
void lr_parser<symbol_type>::print_configuration_set(std::ostream& stream,
                                                     const cf_grammar<symbol_type>& grammar) {
  for (unsigned int i(0); i < configuration_set.size(); ++i) {
    stream << "configuration #" << i + 1 << ":" << std::endl;
    for (parser_state_type::const_iterator state(configuration_set[i].begin());
         state != configuration_set[i].end(); ++state) {
      ::print(stream, *state, grammar);
      stream << std::endl;
    }
    stream << std::endl;
  }
}

template<typename symbol_type>
void lr_parser<symbol_type>::print(std::ostream& stream,
                                   const cf_grammar<symbol_type>& grammar) {
  stream << "This is an LRParser. Here it shall print itself:" << std::endl;

  stream << "> configuration set:" << std::endl;
  print_configuration_set(stream, grammar);
  stream << std::endl;

  stream << "> Transition table: (accept on state #" << accepting_state + 1 << ")" << std::endl;
  {
    std::vector<unsigned int> sizes(grammar.terminals.size(), 0);
    stream << std::setw(6) << std::string(6, ' ');
    for (unsigned int j(0); j < grammar.terminals.size(); ++j) {
      stream << std::setw(4) << std::right << grammar.terminals[j] << " ";
      std::ostringstream symbol_ascii; symbol_ascii << grammar.terminals[j];
      sizes[j] = symbol_ascii.str().size() + 1;
      if(sizes[j] < 5)
        sizes[j] = 5;
    }
    stream << std::endl;
    for (unsigned int i(0); i < transitions_table.size(); ++i) {
      stream << std::setw(3) << std::right << i+1 << ":  ";
      for (unsigned int j(0); j < transitions_table[0].size(); ++j)
        stream << std::setw(sizes[j]-1) << std::right << transitions_table[i][j] << " ";
      stream << std::endl;
    }
    stream << std::endl;
  }

  stream << "> Goto table:" << std::endl;
  {
    std::vector<unsigned int> sizes(grammar.non_terminals.size(), 0);
    stream << std::string(6, ' ');
    for (unsigned int j(0); j < grammar.non_terminals.size(); ++j) {
      std::ostringstream symbol_ascii; symbol_ascii << grammar.terminals[j];
      stream << grammar.non_terminals[j] << " ";
      sizes[j] = symbol_ascii.str().size() + 1;
    }
    stream << std::endl;
    for (unsigned int i(0); i < goto_table.size(); ++i) {
      stream << std::setw(3) << std::right << i+1 << ":  ";
      for (unsigned int j(0); j < goto_table[0].size(); ++j)
        stream << std::setw(sizes[j]-1) << std::right << goto_table[i][j] << " ";
      stream << std::endl;
    }
    stream << std::endl;
  }

  stream << "> First sets:" << std::endl;
  print_first_sets(stream);
  stream << std::endl;

  stream << "> Follow sets:" << std::endl;
  print_follow_sets(stream);
  stream << std::endl;

  stream << "This is all I have to give. Farewell, my dear friend!" << std::endl;
}


// Compute the first set of each grammar symbol:
// DragonBook pp221.
// We assume no epsilon production, here.
template<typename symbol_type>
void lr_parser<symbol_type>::build_first_sets(const cf_grammar<symbol_type>& grammar) {
  /*
   * Le first set des terminaux est trivial: c'est eux-meme:
   */
  for (unsigned int i(0); i < grammar.terminals.size(); ++i)
    firsts[grammar.terminals[i]].insert(grammar.terminals[i]);

  bool stop(false);
  while (not stop) {
    std::map<symbol_type, std::set<symbol_type>> firstsp(firsts);

    /*
     * Pour chaque non terminal:
     */
    for (unsigned int i(0); i < grammar.production_rules.size(); ++i) {
      const symbol_list_type<symbol_type>& rhs(grammar.production_rules[i].second);
      const symbol_type lhs(grammar.production_rules[i].first);

      firsts[lhs].insert(firstsp[rhs.front()].begin(),
                         firstsp[rhs.front()].end());
    }
    stop = (firsts == firstsp);
  }
}

// Compute the follow set of a grammar symbol
// dragonBook pp222.
template<typename symbol_type>
void lr_parser<symbol_type>::build_follow_sets(const cf_grammar<symbol_type>& grammar) {
  bool stop(false);
  while (not stop) {
    std::map<symbol_type, std::set<symbol_type>> followsp(follows);

    /*
     * Pour tous les symbols non-terminaux:
     */ 
    for (unsigned int i(0); i < grammar.non_terminals.size(); ++i) {
      symbol_type current(grammar.non_terminals[i]);

      /*
       * Je regard si ce symbol (non-terminal) apparait dans le rhs d'une production:
       * NOTE: Il ne _SUFFIT PAS_ de trouver la premiere occurence!
       */
      for (unsigned int j(0); j < grammar.production_rules.size(); ++j) {
        const symbol_list_type<symbol_type>& rhs(grammar.production_rules[j].second);

        typename symbol_list_type<symbol_type>::const_iterator it(rhs.begin());
        while ((it = std::find(it, rhs.end(), current)) != rhs.end()) {
          //if((it = std::find(rhs.begin(), rhs.end(), current)) != rhs.end())
          /*
           * Si le symbol est trouve dans le rhs:
           */
          const symbol_type lhs(grammar.production_rules[j].first);

          if(++it != rhs.end())
            follows[current].insert(firsts[*it].begin(),
                                    firsts[*it].end());
          else
            follows[current].insert(followsp[lhs].begin(),
                                    followsp[lhs].end());
        }
      }
    }
    stop = (follows == followsp);
  }
}

template<typename symbol_type>
void lr_parser<symbol_type>::build_configuration_set(const cf_grammar<symbol_type>& g) {
  std::queue<parser_state_type> visit_list;
  
  // build the starting state:
  unsigned int start_production_id(find_production(g, g.start_symbol));
  visit_list.push(parser_state_type());
  visit_list.back().insert(parser_item(start_production_id, 0));
  close_parser_state(visit_list.back(), g);

  // visit all the successor states:
  while (not visit_list.empty()) {
    const parser_state_type current(visit_list.front());
    visit_list.pop();

    if(std::find(configuration_set.begin(), 
                 configuration_set.end(),
                 current) == configuration_set.end()) {
      for (unsigned int i(0); i < g.symbol_set.size(); ++i) {
        const symbol_type& sym(g.symbol_set[i]);
        parser_state_type succ(compute_successor_parser_state(current, g, sym));
        if(not succ.empty())
          visit_list.push(succ);
      }
      configuration_set.push_back(current);
    }
  }
}

// Build the transition and the goto tables:
template<typename symbol_type>
void lr_parser<symbol_type>::build_transition_table(const cf_grammar<symbol_type>& grammar) {
  transitions_table.clear();
  transitions_table.resize(configuration_set.size(),
                           std::vector<short int>(grammar.terminals.size(), 0));

  goto_table.clear();
  goto_table.resize(configuration_set.size(),
                    std::vector<short int>(grammar.non_terminals.size(), 0));

  // Fill each line of the transition and goto tables:
  for (unsigned int i(0); i < configuration_set.size(); ++i) {
    // Fill each column of the transition table:
    //   0: syntax error, 
    // > 0: shift and push,
    // < 0: reduce and pop.
    unsigned int grammar_production_id(0);
    if(is_reducible(configuration_set[i], grammar, grammar_production_id)) {// reduce:
      if(grammar.production_rules[grammar_production_id].first == grammar.start_symbol) {
        accepting_state = i;
      } else {
        symbol_type reduced_symbol(grammar.production_rules[grammar_production_id].first);
        const std::set<symbol_type>& follow(follows[reduced_symbol]);
        for (typename std::set<symbol_type>::const_iterator term(follow.begin());
            term != follow.end();
            ++term) {
          transitions_table[ i ][ terminal_map[*term] ] = - grammar_production_id - 1;
        }
      }
    }
      
    for (unsigned int j(0); j < grammar.terminals.size(); ++j) {  // shift
      // compute the successor
      parser_state_type succ(compute_successor_parser_state(configuration_set[i],
                                                            grammar,
                                                            grammar.terminals[j]));
      if(succ.size()) {
        // find the id of succ in the configuration set:
        std::vector<parser_state_type>::iterator 
          successor_it(std::find(configuration_set.begin(),
                                 configuration_set.end(),
                                 succ));
        unsigned int successor_id(std::distance(configuration_set.begin(),
                                                successor_it));
        if(transitions_table[i][j] == 0)
          transitions_table[i][j] = successor_id + 1;
        else
          throw std::string("LRParser::buildTransitionTable()"
                            " - A shift-reduce conflict is found.");
      }
    }
      
    // Fill each line of the goto table:
    for (unsigned int j(0); j < grammar.non_terminals.size(); ++j) {
      parser_state_type succ(compute_successor_parser_state(configuration_set[i],
                                                            grammar,
                                                            grammar.non_terminals[j]));
      if(succ.size()) {
        std::vector<parser_state_type>::iterator
          successor_it(std::find(configuration_set.begin(),
                                 configuration_set.end(),
                                 succ));
        unsigned int successor_id(std::distance(configuration_set.begin(),
                                                successor_it));
        goto_table[i][j] = successor_id + 1;
      }
    }
  }
}

// Complete the partial parser state
template<typename symbol_type>
void lr_parser<symbol_type>::close_parser_state(parser_state_type& p, const cf_grammar<symbol_type>& grammar) {
  const std::vector<production_type<symbol_type>>& g(grammar.production_rules);

  std::stack<parser_item> visit_list;
  for (parser_state_type::iterator it(p.begin()); it != p.end(); ++it)
    visit_list.push(*it);

  p.clear();

  while (visit_list.size()) {
    parser_item current(visit_list.top());
    visit_list.pop();

    if(std::find(p.begin(), p.end(), current) == p.end()) {
      if(current.parser_position < g[current.production_id].second.size() 
         and grammar.is_non_terminal(g[current.production_id].second[current.parser_position])) {
        for (unsigned int i(0); i < g.size(); ++i)
          if(g[i].first == g[current.production_id].second[current.parser_position])
            visit_list.push(parser_item(i, 0));
      }
      p.insert(current);
    }
  }
}

// Find the production rule for the non terminal symbol s in the grammar g
// (what if there is more than one?)
// THIS IS UNSAFE! ONLY USE TO FIND THE START SYMBOL, WHICH SHOULD BE UNIQUE!
template<typename symbol_type>
std::size_t lr_parser<symbol_type>::find_production(const cf_grammar<symbol_type>& g, symbol_type s) {
  bool found(false);
  size_t rule_id(0);
  
  for (unsigned int i(0); i < g.production_rules.size(); ++i)
    if(g.production_rules[i].first == s) {
      if(not found) {
        found = true;
        rule_id = i;
      } else {
        throw std::string("findProduction()"
                          " - More than one rule which produce this symbol is found.");
      }
    }

  if(found)
    return rule_id;
  else
    throw std::string("findProduction()"
                      " - No production rule for this symbol in this grammar. "
                      "This symbol is probably a terminal.");
}


// Find the successor state S(p, terminal)
template<typename symbol_type>
parser_state_type lr_parser<symbol_type>::compute_successor_parser_state(const parser_state_type& state,
                                                                         const cf_grammar<symbol_type>& grammar,
                                                                         const symbol_type& terminal) {
  const std::vector<production_type<symbol_type>>& g(grammar.production_rules);
  
  parser_state_type succ;
  for (parser_state_type::const_iterator item(state.begin());
      item != state.end();
      ++item)
    // if the point is not at the end of the production:
    if(item->parser_position < g[item->production_id].second.size()) 
      // if symbol after the point is the current terminal:
      if(g[item->production_id].second[item->parser_position] == terminal)
        // then we advance and add this item to the successor succ:
        succ.insert(parser_item(item->production_id, item->parser_position + 1));

  close_parser_state(succ, grammar);
  return succ;
}

template<typename symbol_type>
bool lr_parser<symbol_type>::is_reducible(const parser_state_type& p,
                             const cf_grammar<symbol_type>& grammar,
                             unsigned int& production_id) {
  const std::vector<production_type<symbol_type>>& g(grammar.production_rules);

  bool found(false);
  for (parser_state_type::const_iterator item(p.begin()); item != p.end(); ++item)
    if(item->parser_position == g[item->production_id].second.size()) {
      if(not found) {
        production_id = item->production_id;
        found = true;
      } else {
          throw std::string("isReducible()"
                            "- A reduce-reduce conflict is found.");
      }
    }
  return found;
}



#endif /* _PARSER_H_ */
