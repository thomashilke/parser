#ifndef _MYREGEX_H_
#define _MYREGEX_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <stack>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <list>
#include <iterator>

#include "char_input.hpp"

typedef std::vector<bool> RegexConfiguration;

class astRegexNode;

struct regex {
  // The ordered set of all the configurations of a non deterministic finite
  //  automaton.
  std::vector<RegexConfiguration> configurationSet;

  // For each state s_i \in S, the transition function T(s_i, alpha) \in S for
  //  each symbol alpha \in \Sigma.
  std::vector< std::vector< size_t > > transitionTable;

  // For each transition from a state to another, gives the tokenId which is
  //  accepted, if any.
  std::vector< std::vector< size_t > > acceptTransitionTable;

 private:
  void buildRegexConfigurations(astRegexNode* ast);
  void buildRegexTransitionTable(astRegexNode* ast);

 public:
  explicit regex(astRegexNode* ast): configurationSet(),
                                     transitionTable(),
                                     acceptTransitionTable() {
    buildRegexConfigurations(ast);
    buildRegexTransitionTable(ast);
  }
};

std::ostream& operator<<(std::ostream& flux, const RegexConfiguration& c);
void printConfiguration(std::ostream& flux,
                        const std::vector<RegexConfiguration>& conf);
void printTransitionTable(std::ostream& flux,
                          const std::vector< std::vector< size_t > >& table);
void printAcceptTransitionTable(std::ostream& flux,
                                const std::vector< std::vector< size_t > >& table);
bool matchRegex(regex& r, const std::string& s);
bool match_regex_longest(regex& r,
                         CharInput& input,
                         std::string& token,
                         unsigned int& tokenId);

#endif /* _MYREGEX_H_ */
