#include "parser.hpp"

short Symbol::idFactory(0);
const Symbol Symbol::START(Symbol::newSymbol());
const Symbol Symbol::EOI(Symbol::newSymbol());

void print(std::ostream& flux,
           const ParserItem& item,
           const CFGrammar& grammar)
{
  const Production& p(grammar.productionRules[item.productionId]);
  flux << grammar.prettyName(p.first) << " -> ";
  
  unsigned int pos(0);
  for(SymbolList::const_iterator sym(p.second.begin());
      sym != p.second.end();
      ++sym, ++pos)
    {
      if(item.parserPosition == pos)
        flux << ". ";
      flux << grammar.prettyName(*sym) << " ";
    }
  
  if(item.parserPosition == pos)
    flux << ".";
}

void print(std::ostream& flux,
           const ParserState& state,
           const CFGrammar& grammar)
{
  for(ParserState::iterator it(state.begin()); it != state.end(); ++it)
    {
      ::print(flux, *it, grammar);
      std::cout << std::endl;
    }
}

void CFGrammar::wrapUp()
{
  std::set<Symbol> s, nt;
  for(unsigned int i(0); i < productionRules.size(); ++i)
    {
      nt.insert(productionRules[i].first);
      s.insert(productionRules[i].first);
      for(unsigned int j(0); j < productionRules[i].second.size(); ++j)
        s.insert(productionRules[i].second[j]);
    }
  terminals.resize(s.size());
  std::vector<Symbol>::iterator
    it(std::set_difference(s.begin(), s.end(), 
                           nt.begin(), nt.end(), 
                           terminals.begin()));
  terminals.resize(it-terminals.begin());

  nonTerminals.insert(nonTerminals.begin(), nt.begin(), nt.end());
  symbolSet.insert(symbolSet.begin(), s.begin(), s.end());
}

LRParser::LRParser(const CFGrammar& g): configurationSet(),
                                        transitionsTable(),
                                        gotoTable(),
                                        acceptingState(0),
                                        ruleLengths(g.productionRules.size(), 0),
                                        reduceSymbol(g.productionRules.size(), Symbol()),
                                        nonTerminalMap(),
                                        terminalMap(),
                                        firsts(),
                                        follows()
{
  for(unsigned int i(0); i < ruleLengths.size(); ++i)
    {
      ruleLengths[i] = g.productionRules[i].second.size();
      reduceSymbol[i] = g.productionRules[i].first;
    }
    
  for(unsigned int i(0); i < g.terminals.size(); ++i)
    terminalMap[g.terminals[i]] = i;

  for(unsigned int i(0); i < g.nonTerminals.size(); ++i)
    nonTerminalMap[g.nonTerminals[i]] = i;    

  buildFirstSets(g);
  buildFollowSets(g);
  buildConfigurationSet(g);
  buildTransitionTable(g);
}

void LRParser::printFirstSets(std::ostream& flux, const CFGrammar& grammar)
{
  for(std::map<Symbol, std::set<Symbol> >::iterator firstSet(firsts.begin());
      firstSet != firsts.end(); ++firstSet)
    {
      flux << "FIRST(" << grammar.prettyName(firstSet->first)  << ") = { ";
      for(std::set<Symbol>::iterator sym(firstSet->second.begin());
          sym != firstSet->second.end(); ++sym)
        flux << grammar.prettyName(*sym) << " ";
      flux << "}" << std::endl;
    }
}

void LRParser::printFollowSets(std::ostream& flux, const CFGrammar& grammar)
{
  for(std::map<Symbol, std::set<Symbol> >::iterator followSet(follows.begin());
      followSet != follows.end(); ++followSet)
    {
      flux << "FOLLOW(" << grammar.prettyName(followSet->first)  << ") = { ";
      for(std::set<Symbol>::iterator sym(followSet->second.begin());
          sym != followSet->second.end(); ++sym)
        flux << grammar.prettyName(*sym) << " ";
      flux << "}" << std::endl;
    }
}

void LRParser::printConfigurationSet(std::ostream& flux, const CFGrammar& grammar)
{
  for(unsigned int i(0); i < configurationSet.size(); ++i)
    {
      flux << "configuration #" << i + 1 << ":" << std::endl;
      for(ParserState::const_iterator state(configurationSet[i].begin());
          state != configurationSet[i].end(); ++state)
        {
          ::print(flux, *state, grammar);
          flux << std::endl;
        }
      flux << std::endl;
    }
}

void LRParser::print(std::ostream& flux, const CFGrammar& grammar)
{
  flux << "This is an LRParser. Here it shall print itself:" << std::endl;

  flux << "> configuration set:" << std::endl;
  printConfigurationSet(flux, grammar);
  flux << std::endl;

  flux << "> Transition table: (accept on state #" << acceptingState + 1 << ")" << std::endl;
  {
    std::vector<unsigned int> sizes(grammar.terminals.size(), 0);
    flux << std::setw(6) << std::string(6, ' ');
    for(unsigned int j(0); j < grammar.terminals.size(); ++j)
      {
        flux << std::setw(4) << std::right << grammar.prettyName(grammar.terminals[j]) << " ";
        sizes[j] = grammar.prettyName(grammar.terminals[j]).size() + 1;
        if(sizes[j] < 5)
          sizes[j] = 5;
      }
    flux << std::endl;
    for(unsigned int i(0); i < transitionsTable.size(); ++i)
      {
        flux << std::setw(3) << std::right << i+1 << ":  ";
        for(unsigned int j(0); j < transitionsTable[0].size(); ++j)
          flux << std::setw(sizes[j]-1) << std::right << transitionsTable[i][j] << " ";
        flux << std::endl;
      }
    flux << std::endl;
  }

  flux << "> Goto table:" << std::endl;
  {
    std::vector<unsigned int> sizes(grammar.nonTerminals.size(), 0);
    flux << std::string(6, ' ');
    for(unsigned int j(0); j < grammar.nonTerminals.size(); ++j)
      {
        flux << grammar.prettyName(grammar.nonTerminals[j]) << " ";
        sizes[j] = grammar.prettyName(grammar.nonTerminals[j]).size() + 1;
      }
    flux << std::endl;
    for(unsigned int i(0); i < gotoTable.size(); ++i)
      {
        flux << std::setw(3) << std::right << i+1 << ":  ";
        for(unsigned int j(0); j < gotoTable[0].size(); ++j)
          flux << std::setw(sizes[j]-1) << std::right << gotoTable[i][j] << " ";
        flux << std::endl;
      }
    flux << std::endl;
  }

  flux << "> First sets:" << std::endl;
  printFirstSets(flux, grammar);
  flux << std::endl;

  flux << "> Follow sets:" << std::endl;
  printFollowSets(flux, grammar);
  flux << std::endl;

  flux << "This is all I have to give. Farewell, my dear friend!" << std::endl;
}

// Compute the first set of each grammar symbol:
// DragonBook pp221.
// We assume no epsilon production, here.
void LRParser::buildFirstSets(const CFGrammar& grammar)
{
  /*
   * Le first set des terminaux est trivial: c'est eux-meme:
   */
  for(unsigned int i(0); i < grammar.terminals.size(); ++i)
    firsts[grammar.terminals[i]].insert(grammar.terminals[i]);

  bool stop(false);
  while(!stop)
    {
      std::map<Symbol, std::set<Symbol> > firstsp(firsts);

      /*
       * Pour chaque non terminal:
       */
      for(unsigned int i(0); i < grammar.productionRules.size(); ++i)
        {
          const SymbolList& rhs(grammar.productionRules[i].second);
          const Symbol lhs(grammar.productionRules[i].first);

          firsts[lhs].insert(firstsp[rhs.front()].begin(),
                             firstsp[rhs.front()].end());
        }
      stop = (firsts == firstsp);
    }
}

// Compute the follow set of a grammar symbol
// dragonBook pp222.
void LRParser::buildFollowSets(const CFGrammar& grammar)
{
  bool stop(false);
  while(!stop)
    {
      std::map<Symbol, std::set<Symbol> > followsp(follows);

      /*
       * Pour tous les symbols non-terminaux:
       */ 
      for(unsigned int i(0); i < grammar.nonTerminals.size(); ++i)
        {
          Symbol current(grammar.nonTerminals[i]);

          /*
           * Je regard si ce symbol (non-terminal) apparait dans le rhs d'une production:
           * NOTE: Il ne _SUFFIT PAS_ de trouver la premiere occurence!
           */
          for(unsigned int j(0); j < grammar.productionRules.size(); ++j)
            {
              const SymbolList& rhs(grammar.productionRules[j].second);

              SymbolList::const_iterator it(rhs.begin());
              while((it = std::find(it, rhs.end(), current)) != rhs.end())
                //if((it = std::find(rhs.begin(), rhs.end(), current)) != rhs.end())
                {
                  /*
                   * Si le symbol est trouve dans le rhs:
                   */
                  const Symbol lhs(grammar.productionRules[j].first);

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

void LRParser::buildConfigurationSet(const CFGrammar& g)
{
  std::queue<ParserState> visitList;
  
  // build the starting state:
  unsigned int startProductionId(findProduction(g, g.startSymbol));
  visitList.push(ParserState());
  visitList.back().insert(ParserItem(startProductionId, 0));
  closeParserState(visitList.back(), g);

  // visit all the successor states:
  while(!visitList.empty())
    {
      const ParserState current(visitList.front());
      visitList.pop();

      if(std::find(configurationSet.begin(), 
		   configurationSet.end(),
		   current)
	 == configurationSet.end())
	{
	  for(unsigned int i(0); i < g.symbolSet.size(); ++i)
	    {
	      const Symbol& sym(g.symbolSet[i]);
	      ParserState succ(computeSuccessorParserState(current, g, sym));
	      if(!succ.empty())
		visitList.push(succ);
	    }
	  configurationSet.push_back(current);
	}
    }
}

// Build the transition and the goto tables:
void LRParser::buildTransitionTable(const CFGrammar& grammar)
{
  transitionsTable.clear();
  transitionsTable.resize(configurationSet.size(),
                          std::vector< short int >(grammar.terminals.size(), 0));

  gotoTable.clear();
  gotoTable.resize(configurationSet.size(),
                   std::vector< short int >(grammar.nonTerminals.size(), 0));

  // Fill each line of the transition and goto tables:
  for(unsigned int i(0); i < configurationSet.size(); ++i)
    {
      // Fill each column of the transition table:
      //   0: syntax error, 
      // > 0: shift and push,
      // < 0: reduce and pop.
      unsigned int grammarProductionId(0);
      if(isReducible(configurationSet[i], grammar, grammarProductionId)) // reduce:
        {
          if(grammar.productionRules[grammarProductionId].first == grammar.startSymbol)
            acceptingState = i;
          else
            {
              Symbol reducedSymbol(grammar.productionRules[grammarProductionId].first);
              const std::set<Symbol>& follow(follows[reducedSymbol]);
              for(std::set<Symbol>::const_iterator term(follow.begin());
                  term != follow.end();
                  ++term)
                {
                  transitionsTable[ i ][ terminalMap[*term] ] = - grammarProductionId - 1;
                }
            }
        }
      for(unsigned int j(0); j < grammar.terminals.size(); ++j) // shift
        {
          // compute the successor
          ParserState succ(computeSuccessorParserState(configurationSet[i],
                                                       grammar,
                                                       grammar.terminals[j]));
          if(succ.size())
            {
              // find the id of succ in the configuration set:
              std::vector< ParserState >::iterator 
                successorIt(std::find(configurationSet.begin(),
                                      configurationSet.end(),
                                      succ));
              unsigned int successorId(std::distance(configurationSet.begin(),
                                                     successorIt));
              if(transitionsTable[i][j] == 0)
                transitionsTable[i][j] = successorId + 1;
              else
                throw std::string("LRParser::buildTransitionTable()"
                                  " - A shift-reduce conflict is found.");
            }

      }
      
      // Fill each line of the goto table:
      for(unsigned int j(0); j < grammar.nonTerminals.size(); ++j)
        {
          ParserState succ(computeSuccessorParserState(configurationSet[i],
                                                       grammar,
                                                       grammar.nonTerminals[j]));
          if(succ.size())
            {
              std::vector< ParserState >::iterator
                successorIt(std::find(configurationSet.begin(),
                                      configurationSet.end(),
                                      succ));
              unsigned int successorId(std::distance(configurationSet.begin(),
                                                     successorIt));
              gotoTable[i][j] = successorId + 1;
            }
        }
    }
}

// Complete the partial parser state
void LRParser::closeParserState(ParserState& p, const CFGrammar& grammar)
{
  const std::vector<Production>& g(grammar.productionRules);

  std::stack<ParserItem> visitList;
  for(ParserState::iterator it(p.begin()); it != p.end(); ++it)
    visitList.push(*it);

  p.clear();

  while(visitList.size())
    {
      ParserItem current(visitList.top());
      visitList.pop();

      if(std::find(p.begin(), p.end(), current) == p.end())
        {
          if(current.parserPosition < g[current.productionId].second.size() 
             and grammar.isNonTerminal(g[current.productionId].second[current.parserPosition]))
            {
              for(unsigned int i(0); i < g.size(); ++i)
                if(g[i].first == g[current.productionId].second[current.parserPosition])
                  visitList.push(ParserItem(i, 0));
            }
          p.insert(current);
        }
    }
}

// Find the production rule for the non terminal symbol s in the grammar g
// (what if there is more than one?)
// THIS IS UNSAFE! ONLY USE TO FIND THE START SYMBOL, WHICH SHOULD BE UNIQUE!
size_t LRParser::findProduction(const CFGrammar& g, Symbol s)
{
  bool found(false);
  size_t ruleId(0);
  
  for(unsigned int i(0); i < g.productionRules.size(); ++i)
    if(g.productionRules[i].first == s)
      {
        if(!found)
          {found = true; ruleId = i;}
        else
          throw std::string("findProduction()"
                            " - More than one rule which produce this symbol is found.");
      }

  if(found)
    return ruleId;
  else
    throw std::string("findProduction()"
                      " - No production rule for this symbol in this grammar. "
                      "This symbol is probably a terminal.");
}


// Find the successor state S(p, terminal)
ParserState LRParser::computeSuccessorParserState(const ParserState& state,
                                                  const CFGrammar& grammar,
                                                  const Symbol& terminal)
{
  const std::vector<Production>& g(grammar.productionRules);
  
  ParserState succ;
  for(ParserState::const_iterator item(state.begin());
      item != state.end();
      ++item)
    // if the point is not at the end of the production:
    if(item->parserPosition < g[item->productionId].second.size()) 
      // if symbol after the point is the current terminal:
      if(g[item->productionId].second[item->parserPosition] == terminal)
        // then we advance and add this item to the successor succ:
        succ.insert(ParserItem(item->productionId, item->parserPosition + 1));

  closeParserState(succ, grammar);
  return succ;
}

bool LRParser::isReducible(const ParserState& p,
                           const CFGrammar& grammar,
                           unsigned int& productionId)
{
  const std::vector<Production>& g(grammar.productionRules);

  bool found(false);
  for(ParserState::const_iterator item(p.begin()); item != p.end(); ++item)
    if(item->parserPosition == g[item->productionId].second.size())
      {
        if(!found)
          {
            productionId = item->productionId;
            found = true;
          }
        else
          throw std::string("isReducible()"
                            "- A reduce-reduce conflict is found.");
      }
  return found;
}
