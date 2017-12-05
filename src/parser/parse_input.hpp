#ifndef PARSE_INPUT_H
#define PARSE_INPUT_H

#include <list>

#include "lr_parser.hpp"


template<class T>
void pop(std::list<T>& list, unsigned int n) {
  typename std::list<T>::reverse_iterator lower_bound(list.rbegin());
  std::advance(lower_bound, n);
  list.erase(lower_bound.base(), list.end());
}

template<class token_source_type, typename tree_factory_type>
typename tree_factory_type::node_type*
parse_input_to_tree(lr_parser<typename token_source_type::symbol_type>& parser,
                    token_source_type& input,
                    tree_factory_type& tree_factory) {
  using node_type = typename tree_factory_type::node_type;

  std::list<node_type*> node_stack;

  std::list<unsigned int> state_stack;
  state_stack.push_back(0);

  while(state_stack.back() != parser.accepting_state) {
    const unsigned int terminal_id(parser.terminal_map.find(input.get().symbol)->second);
    const int action(parser.transitions_table[ state_stack.back() ][ terminal_id ]);

    if(action > 0) {  // shift
      node_stack.push_back(tree_factory.build_leaf(input.get()));

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
      throw std::string("parse error near ") + input.get().render_coordinates();
    }
  }
  delete node_stack.back(); //The start rule is not reduced, hence two symbols are on the stack
  return node_stack.front();
}

template<typename token_source_type>
bool parse_input(lr_parser<typename token_source_type::symbol_type>& parser,
                 token_source_type& input) {
  std::list<unsigned int> state_stack;
  state_stack.push_back(0);

  while(state_stack.back() != parser.accepting_state) {
    const unsigned int terminal_id(parser.terminal_map.find(input.get().symbol)->second);
    const int action(parser.transitions_table[ state_stack.back() ][ terminal_id ]);

    if(action > 0) {  // shift
      state_stack.push_back(action - 1);
      input.next();
    } else if(action < 0) {  // reduce
      const unsigned int production_rule_id(-action-1);
      const unsigned int non_terminal_symbol_id(parser.non_terminal_map[parser.reduce_symbol[production_rule_id]]);

      pop(state_stack, parser.rule_lengths[production_rule_id]);
      state_stack.push_back(parser.goto_table[ state_stack.back() ][ non_terminal_symbol_id ] - 1);
    } else {
      return false;
    }
  }
  return true;
}

#endif /* PARSE_INPUT_H */
