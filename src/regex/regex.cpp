#include <list>
#include <string>
#include <stack>
#include <vector>

#include "regex.hpp"
#include "regexast.hpp"
#include "char_input.hpp"

std::ostream& operator<<(std::ostream& flux, const RegexConfiguration& c) {
  flux << "[ ";
  for (unsigned int i(0); i < c.size(); ++i)
    flux << c[i] << " ";
  flux << "]";
  return flux;
}

void printConfiguration(std::ostream& flux,
                        const std::vector<RegexConfiguration>& conf) {
  for (unsigned int i(0); i < conf.size(); ++i)
    flux << conf[i] << std::endl;
}

void printTransitionTable(std::ostream& flux,
                          const std::vector< std::vector< size_t > >& table) {
  flux << std::string(2*32+3, ' ');
  for (char c(32); c < 127; ++c)
    flux << std::setw(2) << std::right << c;
  flux << std::endl;
  for (unsigned int i(0); i < table.size(); ++i) {
      flux << i + 1 << ": ";

      for (char c(0); c < 127; ++c)
        flux << std::setw(2) << std::right << table[i][c];

      flux << std::endl;
    }
}

void printAcceptTransitionTable(std::ostream& flux,
                                const std::vector<std::vector<size_t> >& table) {
  for (unsigned int i(0); i < table.size(); ++i) {
    for (unsigned int j(0); j < table.size(); ++j)
      flux << std::setw(2) << std::right << table[i][j];
    flux << std::endl;
  }
}

void regex::buildRegexConfigurations(astRegexNode* ast) {
  configurationSet.clear();

  RegexConfiguration startConf(ast->getConfigurationSize(0) + 1, false);
  ast->setPred(startConf);

  std::stack<RegexConfiguration> visitList;
  visitList.push(startConf);

  while (visitList.size()) {
      const RegexConfiguration current(visitList.top());
      visitList.pop();
      
      if (std::find(configurationSet.begin(),
          configurationSet.end(),
          current) == configurationSet.end()) {
          configurationSet.push_back(current);
          for (char c(0); c < 127; ++c) {
              std::list< size_t > accept;
              RegexConfiguration succ(startConf.size(), false);
              succ.back() = ast->advance(current, succ, accept, c);

              if (std::count(succ.begin(), succ.end(), true))
                visitList.push(succ);
            }
        }
    }
}

void regex::buildRegexTransitionTable(astRegexNode* ast) {
  transitionTable.clear();
  transitionTable.resize(configurationSet.size(),
                           std::vector<size_t>(127, 0));

  acceptTransitionTable.clear();
  acceptTransitionTable.resize(configurationSet.size(),
                               std::vector<size_t>(configurationSet.size(), 0));

  for (unsigned int i(0); i < configurationSet.size(); ++i) {
      const RegexConfiguration& current(configurationSet[i]);
      for (unsigned int c(0); c < 127; ++c) {
        std::list<std::size_t> accept;
          RegexConfiguration succ(current.size(), false);
          succ.back() = ast->advance(current, succ, accept, c);
          
          if (std::count(succ.begin(), succ.end(), true)) {
              std::vector<RegexConfiguration>::iterator
                succInTable(std::find(configurationSet.begin(),
                                      configurationSet.end(),
                                      succ));
              transitionTable[i][c] = (std::distance(configurationSet.begin(),
                                                     succInTable)
                                       + 1);
              if(accept.size())
		acceptTransitionTable[i][std::distance(configurationSet.begin(),
						       succInTable)] = accept.front();
            }
        }
    }
}

bool matchRegex(regex& r, const std::string& s) {
  bool abort(false);
  size_t currentState(0);
  size_t stringPosition(0);
  while (!abort) {
      if (stringPosition < s.size()) {
          const size_t nextState(r.transitionTable[currentState][s[stringPosition]]);
          if (nextState) {
              currentState = nextState - 1;
              ++stringPosition;
          } else {
            abort = true;
          }
      } else {
        abort = true;
      }
    }
  return stringPosition == s.size()
      and r.configurationSet[currentState].back();
}


bool match_regex_longest(regex& r,
                         CharInput& input,
                         std::string& token,
                         unsigned int& token_id) {
  bool matched(false);
  std::size_t last_matching_position(0);
  unsigned int last_matching_token_id(0);
  std::size_t current_state(0);

  bool abort(false);
  bool at_eos(false);
  std::size_t i(0);
  char c(0);

  while (not abort) {
    at_eos = not input.get(i, c);

    if (at_eos) {
      abort = true;
    } else {
      const std::size_t next_state(r.transitionTable[current_state][c]);
      if (next_state != 0) {
        if (r.acceptTransitionTable[current_state][next_state - 1]) {
          matched = true;
          last_matching_position = i;
          last_matching_token_id = r.acceptTransitionTable[current_state][next_state - 1];
        }
        current_state = next_state - 1;
      } else {
        abort = true;
      }
    }
    ++i;
  }

  if (matched) {
    token = input.extract_substring(last_matching_position + 1);
    token_id = last_matching_token_id;
    return true;
  } else {
    return false;
  }
}
