#ifndef _PARSEFUNCTIONS_H_
#define _PARSEFUNCTIONS_H_

#include "parser.hpp"

#include "../utils/string_builder.hpp"

class AstNode;
class AstProduction;
class AstLeaf;

class AstTreeVisitorI
{
  AstTreeVisitorI& operator=(const AstTreeVisitorI&);
  //AstTreeVisitorI(const AstTreeVisitorI&);

public:
  AstTreeVisitorI(){}
  virtual ~AstTreeVisitorI(){}
  virtual void visit(AstProduction* node) = 0;
  virtual void visit(AstLeaf* node) = 0;
};

class AstNode
{
  AstNode& operator=(const AstNode&);
  AstNode(const AstNode&);
public:
  const Symbol token;

  AstNode(Symbol t): token(t) {}
  virtual ~AstNode() = 0;
  virtual void accept(AstTreeVisitorI* v) = 0;
};
inline AstNode::~AstNode(){}

class AstProduction: public AstNode
{
public:
  const unsigned int productionId;
  std::vector<AstNode*> children;

  template<class Iterator>
  AstProduction(const Iterator& begin, const Iterator& end,
                unsigned int prodId, Symbol nonTerminal): AstNode(nonTerminal),
                                                          productionId(prodId),
                                                          children(begin, end)
  {}
  virtual ~AstProduction()
  {
    for(std::vector<AstNode*>::iterator it(children.begin());
        it != children.end();
        ++it)
      delete *it;
  }
  virtual void accept(AstTreeVisitorI* v){ v->visit(this); }
};

class AstLeaf: public AstNode
{
public:
  const std::string value;

  AstLeaf(const std::string& v, Symbol terminal): AstNode(terminal), value(v) {}
  virtual void accept(AstTreeVisitorI* v){ v->visit(this); }
};

class GetTerminal: public AstTreeVisitorI
{
  AstLeaf* terminal;

  GetTerminal& operator=(const GetTerminal&);
  GetTerminal(const GetTerminal&);
public:
  GetTerminal(): terminal(NULL) {}

  virtual void visit(AstProduction* node)
  { node->children.front()->accept(this); }

  virtual void visit(AstLeaf* node)
  { terminal = node; }

  AstLeaf* getTerm(){ return terminal; }
};

class GetTerminalValues: public AstTreeVisitorI
{
  unsigned int n;

  GetTerminalValues& operator=(const GetTerminalValues&);
  GetTerminalValues(const GetTerminalValues&);
public:
  std::vector<std::string> last_terms;

  GetTerminalValues(unsigned int _n): n(_n), last_terms() {}

  virtual void visit(AstProduction* node)
  { 
    for(std::vector<AstNode*>::reverse_iterator it(node->children.rbegin());
        it != node->children.rend() and last_terms.size() < 0;
        ++it)
      (*it)->accept(this);
  }

  virtual void visit(AstLeaf* node)
  { last_terms.push_back(node->value); }
};

template<class T>
void pop(std::list<T>& list, unsigned int n)
{
  typename std::list<T>::reverse_iterator lowerBound(list.rbegin());
  std::advance(lowerBound, n);
  list.erase(lowerBound.base(), list.end());
}

/*class ParserException
{
  
public:
  ParseException(): 
  {}
  virtual ~ParseException()
  {}
};*/

template<typename TokenIterator>
void tokenizeInput(TokenIterator& input, const CFGrammar& grammar)
{
  while(*input != Symbol::EOI)
    {
      std::cout << grammar.prettyName(*input) << ": " << input.value() << std::endl;
      ++input;
    }
}


template<class TokenIterator>
AstNode* ParseInputToAst(LRParser& parser, CFGrammar& grammar, TokenIterator& input)
{
  std::list<AstNode*> nodeStack;

  std::list<unsigned int> stateStack;
  stateStack.push_back(0);

  while(stateStack.back() != parser.acceptingState)
    {
      const unsigned int terminalId(parser.terminalMap.find(*input)->second);
      const int action(parser.transitionsTable[ stateStack.back() ][ terminalId ]);

      if(action > 0) // shift
        {
          nodeStack.push_back(new AstLeaf(input.value(), *input));

          stateStack.push_back(action - 1);
          ++input;
        }
      else if(action < 0) // reduce
        {
          const unsigned int productionRuleId(-action-1);
          const unsigned int nonTerminalSymbolId(parser.nonTerminalMap[parser.reduceSymbol[productionRuleId]]);

          std::list<AstNode*>::iterator start(nodeStack.end());
          std::advance(start, - static_cast<int>(parser.ruleLengths[productionRuleId]));
          AstProduction* p(new AstProduction(start,
                                             nodeStack.end(),
                                             productionRuleId,
                                             parser.reduceSymbol[productionRuleId]));
          pop(nodeStack, parser.ruleLengths[productionRuleId]);
          nodeStack.push_back(p);

          pop(stateStack, parser.ruleLengths[productionRuleId]);
          stateStack.push_back(parser.gotoTable[ stateStack.back() ][ nonTerminalSymbolId ] - 1);
        }
      else
        {
          GetTerminalValues last_terms(5);
          nodeStack.back()->accept(&last_terms);

          StringBuilder message("ParseInput() - Syntax error at ");
          message(grammar.prettyName(*input))
            (" = ")
            (input.value())(" near ");
          for(std::vector<std::string>::reverse_iterator it(last_terms.last_terms.rbegin());
              it != last_terms.last_terms.rend();
              ++it)
            message(*it)(" ");
          message("\n");

          message("Current parser state: ")(stateStack.back())("\n");

          for(unsigned int i(0); i < parser.transitionsTable[stateStack.back()].size(); ++i)
            if(parser.transitionsTable[stateStack.back()][i] != 0)
              message("Expected terminal: ")(grammar.prettyName(grammar.terminals[i]))("\n");

          message("Parser state stack:\n");
          for(std::list<unsigned int>::iterator it(stateStack.begin()); it != stateStack.end(); ++it)
            message(*it)(" ");

          throw message.str();
        }
    }
  delete nodeStack.back(); //The start rule is not reduced, hence two symbols are on the stack
  return nodeStack.front();
}

template<class TokenIterator>
bool validateInput(LRParser& parser, TokenIterator& input, CFGrammar& g)
{
  std::list<unsigned int> stateStack;
  stateStack.push_back(0);

  while(stateStack.back() != parser.acceptingState)
    {
      const unsigned int terminalId(parser.terminalMap.find(*input)->second);
      const int action(parser.transitionsTable[ stateStack.back() ][ terminalId ]);

      if(action > 0) // shift
        {
          stateStack.push_back(action - 1);
          ++input;
        }
      else if(action < 0) // reduce
        {
          const unsigned int productionRuleId(-action-1);
          const unsigned int nonTerminalSymbolId(parser.nonTerminalMap[parser.reduceSymbol[productionRuleId]]);

          pop(stateStack, parser.ruleLengths[productionRuleId]);
          stateStack.push_back(parser.gotoTable[ stateStack.back() ][ nonTerminalSymbolId ] - 1);
        }
      else
        throw std::string("ParseInput() - Syntax error.");
    }
  return true;
}

#endif /* _PARSEFUNCTIONS_H_ */
