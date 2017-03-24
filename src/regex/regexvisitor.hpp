#ifndef _REGEXVISITOR_H_
#define _REGEXVISITOR_H_

#include "../parser/parsefunctions.hpp"
#include "regexast.hpp"

class AstToRegex: public AstTreeVisitorI
{
  AstToRegex& operator=(const AstToRegex&);
  AstToRegex(const AstToRegex&);

  typedef astRegexNode* (AstToRegex::*ActionNT)(const std::vector<AstNode*>& children);
  typedef astRegexNode* (AstToRegex::*ActionT)(const std::string& value);

  std::vector<ActionNT> actionsNT;
  std::map<Symbol, ActionT> actionsT;

  astRegexNode* returnFromBelow;

  astRegexNode* actionChar(const std::string& value)
  { return new astRegexAlpha(value[0]); }

  astRegexNode* actionBracket(const std::string& value)
  {
    typedef std::vector< std::pair< char, char > > RangesT; 

    bool invert(false);
    RangesT Ranges;

    std::string::const_iterator start(value.begin()), end(value.end());
    if(value[0] == '^')
      {
	++start;
	invert = true;
      }
    
    std::stack<char> charStack;
    for(; start != end; ++start)
      {
	if(*start == '-' && charStack.size() && start++ != end)
	  {
	    Ranges.push_back(std::make_pair(charStack.top(), *start));
	    charStack.pop();
	  }
	else
	  charStack.push(*start);
      }
    while(charStack.size())
      {
	Ranges.push_back(std::make_pair(charStack.top(), charStack.top()));
	charStack.pop();
      }
    return new astRegexRange(Ranges, invert);
  }

  astRegexNode* actionNoop(const std::vector<AstNode*>& children)
  { children[0]->accept(this); return returnFromBelow; }

  astRegexNode* actionConcat(const std::vector<AstNode*>& children)
  { 
    children[0]->accept(this);
    astRegexNode* left(returnFromBelow);

    children[1]->accept(this);
    astRegexNode* right(returnFromBelow);

    return new astRegexConcat(left, right);
  }
  astRegexNode* actionAlt(const std::vector<AstNode*>& children)
  { 
    children[0]->accept(this);
    astRegexNode* left(returnFromBelow);

    children[2]->accept(this);
    astRegexNode* right(returnFromBelow);

    return new astRegexAlt(left, right);
  }
  astRegexNode* actionKleenStar(const std::vector<AstNode*>& children)
  {
    GetTerminal gt;
    children[1]->accept(&gt);

    children[0]->accept(this);
    if(gt.getTerm()->value[0] == '*')
      return new astRegexKleenStar(returnFromBelow);
    else if(gt.getTerm()->value[0] == '+')
      return new astRegexConcat(returnFromBelow,
				new astRegexKleenStar(returnFromBelow->clone()));
    else if(gt.getTerm()->value[0] == '?')
      return new astRegexAlt(new astRegexEpsilon(),
			     returnFromBelow);
    else throw std::string("AstToRegex::actionKleenStar() - "
			   "Unknown quantification operator: ") + gt.getTerm()->value;
  }
  astRegexNode* actionGroup(const std::vector<AstNode*>& children)
  {
    children[1]->accept(this);
    return returnFromBelow;
  }

public:
  AstToRegex(): actionsNT(), actionsT(), returnFromBelow(NULL)
  {
    using namespace regexSymbols;

    actionsT[CHAR]    = &AstToRegex::actionChar;
    actionsT[BRACKET] = &AstToRegex::actionBracket;

    actionsNT.push_back(&AstToRegex::actionNoop);
    actionsNT.push_back(&AstToRegex::actionConcat);
    actionsNT.push_back(&AstToRegex::actionNoop);
    actionsNT.push_back(&AstToRegex::actionAlt);
    actionsNT.push_back(&AstToRegex::actionNoop);
    actionsNT.push_back(&AstToRegex::actionKleenStar);
    actionsNT.push_back(&AstToRegex::actionNoop);
    actionsNT.push_back(&AstToRegex::actionNoop);
    actionsNT.push_back(&AstToRegex::actionGroup);
    actionsNT.push_back(&AstToRegex::actionNoop);
  }
  virtual ~AstToRegex(){}

  virtual void visit(AstProduction* node)
  { returnFromBelow = ((*this).*(actionsNT[node->productionId]))(node->children); }

  virtual void visit(AstLeaf* node)
  { returnFromBelow = ((*this).*(actionsT[node->token]))(node->value); }

  astRegexNode* result(){return returnFromBelow;}
};

#endif /* _REGEXVISITOR_H_ */
