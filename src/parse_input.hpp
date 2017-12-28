#ifndef PARSE_INPUT_H
#define PARSE_INPUT_H

#include <list>

#include "lr_parser.hpp"


template<typename token_type>
class parse_error {
public:
  using symbol_type = typename token_type::symbol_type;

  parse_error(token_type* t, const std::vector<symbol_type>& expected_symbols)
    : token(t), expected_symbols(expected_symbols) {}
  ~parse_error() {
    delete token;
    token = nullptr;
  }

  const token_type& get_unexpected_token() const { return *token; }

  const std::vector<symbol_type>& get_expected_symbols() const {
    return expected_symbols;
  }

private:
  token_type* token;
  std::vector<symbol_type> expected_symbols;
};


template<typename token_type>
struct default_error_handler {
public:
  virtual ~default_error_handler() {}

  virtual void operator()(const token_type& unexpected_token) {
    std::cout << unexpected_token.render_coordinates()
              << " error: unexpected " << unexpected_token.symbol
              << "." << std::endl;
  }
  
  virtual void operator()(const parse_error<token_type>& error) {
    this->operator()(error.get_unexpected_token());
  }
};


template<class T>
void pop(std::list<T>& list, unsigned int n) {
  typename std::list<T>::reverse_iterator lower_bound(list.rbegin());
  std::advance(lower_bound, n);
  list.erase(lower_bound.base(), list.end());
}

template<typename token_source_type,
         typename tree_factory_type,
         typename error_handler = default_error_handler<typename token_source_type::symbol_type>>
typename tree_factory_type::node_type*
parse_input_to_tree(lr_parser<typename token_source_type::symbol_type>& parser,
                    cf_grammar<typename token_source_type::symbol_type>& grammar,
                    token_source_type& input,
                    tree_factory_type& tree_factory,
                    error_handler& handler) {
  using token_type = typename token_source_type::token_type;
  using symbol_type = typename token_type::symbol_type;
  using node_type = typename tree_factory_type::node_type;

  std::list<node_type*> node_stack;

  std::list<unsigned int> state_stack;
  state_stack.push_back(0);

  while(state_stack.back() != parser.accepting_state) {
    const auto terminal_map_item(parser.terminal_map.find(input.get().symbol));
    if (terminal_map_item == parser.terminal_map.end()) {
      handler(input.get());
      input.next();
    } else {
    
      const unsigned int terminal_id(terminal_map_item->second);
      const int action(parser.transitions_table[ state_stack.back() ][ terminal_id ]);

      if(action > 0) {  // shift
        node_stack.push_back(tree_factory.build_leaf(input));

        state_stack.push_back(action - 1);
        input.next();
      } else if(action < 0) {  // reduce
        const unsigned int production_rule_id(- action - 1);
        const unsigned int non_terminal_symbol_id(parser.non_terminal_map[parser.reduce_symbol[production_rule_id]]);

        typename std::list<node_type*>::iterator start(node_stack.end());
        std::advance(start, - static_cast<int>(parser.rule_lengths[production_rule_id]));
        node_type* p(tree_factory.build_node(start,
                                             node_stack.end(),
                                             production_rule_id,
                                             parser.reduce_symbol[production_rule_id]));
        pop(node_stack, parser.rule_lengths[production_rule_id]);
        node_stack.push_back(p);

        pop(state_stack, parser.rule_lengths[production_rule_id]);
        state_stack.push_back(parser.goto_table[ state_stack.back() ][ non_terminal_symbol_id ] - 1);
      } else {
        /*
         *  Save the state and node stack in a random access structure
         */
        std::vector<unsigned int> states(state_stack.begin(), state_stack.end());
        std::vector<node_type*> nodes(node_stack.begin(), node_stack.end());

      
        /*
         *  build maps from symbol ids to symbols.
         */
        std::map<unsigned int, symbol_type> symbol_id_map;
        for (const auto& i: parser.non_terminal_map)
          symbol_id_map[i.second] = i.first;

        for (const auto& i: parser.terminal_map)
          symbol_id_map[i.second] = i.first;

      
        /*
         *  Signal the error to the handler
         */
        std::vector<symbol_type> expected_symbols;
        for (std::size_t i(0); i < parser.transitions_table[ state_stack.back() ].size(); ++i) {
          if (parser.transitions_table[ state_stack.back() ][i] != 0)
            expected_symbols.push_back(symbol_id_map[i]);
        }
        handler(parse_error<token_type>(input.get().copy(), expected_symbols));

      
        /*
         *  Try to recover from the error state by
         *  searching for reduction goals
         */
        bool recovered(false);
        for (unsigned int i(states.size()); i > 0 and not recovered; --i) {
          /*if (i == 1)
            std::cout << "available reduction goals following nothing at stack location "
            << i-1 << " which accepts a "
            << input.get().symbol << ": ";
            else
            std::cout << "available reduction goals following a "
            << nodes[i-2]->get_symbol()
            << " at stack location " << i-1 << " which accepts a "
            << input.get().symbol << ": ";*/

        
          std::set<symbol_type> candidate_reduction_goals;
          const std::vector<short>& goto_line(parser.goto_table[states[i-1]]);
          for (const auto& s: parser.non_terminal_map)
            if (goto_line[s.second]) {
              const unsigned int candidate_state(goto_line[s.second] - 1);
              if (parser.transitions_table[candidate_state][terminal_id] > 0)
                candidate_reduction_goals.insert(s.first);
            }
        
          std::set<symbol_type> candidate_important_reduction_goals;
          for (const auto& s: candidate_reduction_goals)
            candidate_important_reduction_goals.insert(grammar.get_important_goal(s, candidate_reduction_goals));
        

          /*
           * Display the candidate sets
           */
          /*std::cout << "reduction goals: ";
          for (const auto& s: candidate_reduction_goals)
            std::cout << s << " ";
          std::cout << std::endl;
          
          std::cout << "important reduction goals: ";
          for (const auto& s: candidate_reduction_goals)
            std::cout << s << " ";
            std::cout << std::endl;*/


        
          if (not candidate_reduction_goals.empty()) {
            const symbol_type reduction_goal(*candidate_reduction_goals.begin());
            const unsigned int reduction_goal_id(parser.non_terminal_map[reduction_goal]);
          
            pop(state_stack, states.size() - i);
            state_stack.push_back(parser.goto_table[ state_stack.back() ][ reduction_goal_id ] - 1);

          
            typename std::list<node_type*>::iterator start(node_stack.end());
            std::advance(start, - (states.size() - i));
            node_type* p(tree_factory.build_node(start,
                                                 node_stack.end(),
                                                 -1,
                                                 reduction_goal));
            pop(node_stack, states.size() - i);
            node_stack.push_back(p);

            recovered = true;
          }
        }
      
        if (not recovered)
          return nullptr;
      }
    }
  }
  delete node_stack.back(); //The start rule is not reduced, hence two symbols are on the stack
  return node_stack.front();
}

template<typename token_source_type,
         typename error_handler = default_error_handler<typename token_source_type::token_type>>
bool parse_input(lr_parser<typename token_source_type::symbol_type>& parser,
                 cf_grammar<typename token_source_type::symbol_type>& grammar,
                 token_source_type& input,
                 error_handler& handler) {
  using symbol_type = typename token_source_type::symbol_type;
  using token_type = typename token_source_type::token_type;
  
  bool return_status(true);
  
  std::list<unsigned int> state_stack;
  std::list<symbol_type> symbol_stack;
  state_stack.push_back(0);

  while(state_stack.back() != parser.accepting_state) {
    const auto terminal_map_item(parser.terminal_map.find(input.get().symbol));
    if (terminal_map_item == parser.terminal_map.end()) {
      handler(input.get());
      input.next();
    } else {
      
      const unsigned int terminal_id(terminal_map_item->second);
      const int action(parser.transitions_table[ state_stack.back() ][ terminal_id ]);

      if(action > 0) {  // shift
        state_stack.push_back(action - 1);
        symbol_stack.push_back(input.get().symbol);
        input.next();
      } else if(action < 0) {  // reduce
        const unsigned int production_rule_id(-action-1);
        const unsigned int non_terminal_symbol_id(parser.non_terminal_map[parser.reduce_symbol[production_rule_id]]);

        pop(state_stack, parser.rule_lengths[production_rule_id]);
        state_stack.push_back(parser.goto_table[ state_stack.back() ][ non_terminal_symbol_id ] - 1);

        pop(symbol_stack, parser.rule_lengths[production_rule_id]);
        symbol_stack.push_back(parser.reduce_symbol[production_rule_id]);
      } else {
        /*
         * Signal an error state in the return value
         */
        return_status = false;


        /*
         *  Save the state and symbol stack in a random access structure
         */
        std::vector<unsigned int> states(state_stack.begin(), state_stack.end());
        std::vector<symbol_type> symbols(symbol_stack.begin(), symbol_stack.end());

      
        /*
         *  build maps from symbol ids to symbols.
         */
        std::map<unsigned int, symbol_type> symbol_id_map;
        for (const auto& i: parser.non_terminal_map)
          symbol_id_map[i.second] = i.first;

        for (const auto& i: parser.terminal_map)
          symbol_id_map[i.second] = i.first;

      
        /*
         *  Signal the error to the handler
         */
        std::vector<symbol_type> expected_symbols;
        for (std::size_t i(0); i < parser.transitions_table[ state_stack.back() ].size(); ++i) {
          if (parser.transitions_table[ state_stack.back() ][i] != 0)
            expected_symbols.push_back(symbol_id_map[i]);
        }
        handler(parse_error<token_type>(input.get().copy(), expected_symbols));

      
        /*
         *  Try to recover from the error state by
         *  searching for reduction goals
         */
        bool recovered(false);
        for (unsigned int i(states.size()); i > 0 and not recovered; --i) {
          /*if (i == 1)
            std::cout << "available reduction goals following nothing at stack location "
            << i-1 << " which accepts a "
            << input.get().symbol << ": ";
            else
            std::cout << "available reduction goals following a "
            << symbols[i-2]
            << " at stack location " << i-1 << " which accepts a "
            << input.get().symbol << ": ";*/

          std::set<symbol_type> candidate_reduction_goals;
          const std::vector<short>& goto_line(parser.goto_table[states[i-1]]);
          for (const auto& s: parser.non_terminal_map)
            if (goto_line[s.second]) {
              const unsigned int candidate_state(goto_line[s.second] - 1);
              if (parser.transitions_table[candidate_state][terminal_id] > 0)
                candidate_reduction_goals.insert(s.first);
            }
        
          std::set<symbol_type> candidate_important_reduction_goals;
          for (const auto& s: candidate_reduction_goals)
            candidate_important_reduction_goals.insert(grammar.get_important_goal(s, candidate_reduction_goals));

          /*
           * Display the candidate sets
           */
          /*std::cout << "reduction goals: ";
          for (const auto& s: candidate_reduction_goals)
            std::cout << s << " ";
          std::cout << std::endl;
          
          std::cout << "important reduction goals: ";
          for (const auto& s: candidate_reduction_goals)
            std::cout << s << " ";
            std::cout << std::endl;*/

        
          if (not candidate_reduction_goals.empty()) {
            const symbol_type reduction_goal(*candidate_reduction_goals.begin());
            const unsigned int reduction_goal_id(parser.non_terminal_map[reduction_goal]);
          
            pop(state_stack, states.size() - i);
            state_stack.push_back(parser.goto_table[ state_stack.back() ][ reduction_goal_id ] - 1);
      
            pop(symbol_stack, states.size() - i);
            symbol_stack.push_back(reduction_goal);

            recovered = true;
          }
        }
      
        if (not recovered)
          return false;
      }
    }
  }
  
  return return_status;
}

template<typename value_type>
void print_stack(const std::list<value_type>& s) {
  for (const auto& v: s)
    std::cout << v << " ";
  std::cout << std::endl;
}

template<typename token_source_type>
bool parse_input_visual(lr_parser<typename token_source_type::symbol_type>& parser,
                        token_source_type& input) {
  using symbol_type = typename token_source_type::symbol_type;
  
  std::list<unsigned int> state_stack;
  std::list<symbol_type> symbol_stack;

  state_stack.push_back(0);

  std::cout << "state stack: "; print_stack(state_stack);
  std::cout << "symbol stack: "; print_stack(symbol_stack);
  std::cout << "input symbol: " << input.get().symbol << std::endl;
  std::cout << std::endl;
  
  while(state_stack.back() != parser.accepting_state) {
    const auto terminal_map_item(parser.terminal_map.find(input.get().symbol));
    if (terminal_map_item == parser.terminal_map.end())
      throw std::string("parse error near ") + input.get().render_coordinates();
    
    const unsigned int terminal_id(terminal_map_item->second);
    const int action(parser.transitions_table[ state_stack.back() ][ terminal_id ]);

    if(action > 0) {  // shift
      state_stack.push_back(action - 1);
      symbol_stack.push_back(input.get().symbol);
      input.next();
    } else if(action < 0) {  // reduce
      const unsigned int production_rule_id(-action-1);
      const unsigned int non_terminal_symbol_id(parser.non_terminal_map[parser.reduce_symbol[production_rule_id]]);

      pop(state_stack, parser.rule_lengths[production_rule_id]);
      state_stack.push_back(parser.goto_table[ state_stack.back() ][ non_terminal_symbol_id ] - 1);

      pop(symbol_stack, parser.rule_lengths[production_rule_id]);
      symbol_stack.push_back(parser.reduce_symbol[production_rule_id]);
    } else {
      return false;
    }

    std::cout << "state stack: "; print_stack(state_stack);
    std::cout << "symbol stack: "; print_stack(symbol_stack);
    std::cout << "input symbol: " << input.get().symbol << std::endl;
    std::cout << std::endl;
  }
  return true;
}

#endif /* PARSE_INPUT_H */
