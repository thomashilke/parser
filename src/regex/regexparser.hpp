#ifndef _REGEXPARSER_H_
#define _REGEXPARSER_H_

#include <string>
#include <iostream>
#include <iterator>
#include <map>

#include "../parser/parser.hpp"

namespace regexSymbols
{
  extern Symbol REGEX;
  extern Symbol KLEENSTAR;
  extern Symbol PIPE;
  extern Symbol CHAR;
  extern Symbol LP;
  extern Symbol RP;
  extern Symbol C;
  extern Symbol CONCAT;
  extern Symbol ALT;
  extern Symbol BRACKET;
}

class RegexTokenIterator
{
  std::string input;
  size_t inputPos;
  bool get(char& c)
  {
    if(inputPos < input.size()) 
      {
   	c = input[inputPos];
	++inputPos;
   	return true;
      } 
    else
      return false;
  }
  bool good(){ return inputPos < input.size(); }
  bool getline(std::string& buffer, char delim)
  {
    char c(0);
    while(get(c))
      {
	if(c == '\\')
          {
            if(!get(c))
              return false;

            std::map<char, char>::iterator it(escapedSymbolValue.find(c));
            if(it != escapedSymbolValue.end())
              c = it->second;
            buffer.push_back(c);
          }
        else
          {
            if(c == delim)
              return true;
            else
              buffer.push_back(c);
          }
      }
    return false;
  }

  std::map<char, Symbol> symbolMap;

  char escape;
  Symbol escapedSymbol;
  std::map<char, char> escapedSymbolValue;

  std::string currentValue;
  Symbol currentSymbol;

  void getNextToken()
  {
    using namespace regexSymbols;

    char newchar(0);
    if(good())
      {
	get(newchar);
        if(newchar == escape)
          {
            if(get(newchar))
              {
                std::map<char, char>::iterator it(escapedSymbolValue.find(newchar));
                if(it != escapedSymbolValue.end())
                  currentValue = it->second;
                else
                  currentValue = newchar;
                currentSymbol = escapedSymbol;
              }
            else
              throw std::string("RegexTokenIterator::getNextToken() - Unexpected end of input.");
          }
	else if(newchar == '[')
	  {
	    currentValue.clear();
	    if(getline(currentValue, ']'))
	      currentSymbol = BRACKET;
	    else
	      throw std::string("RegexTokenIterator::getNextToken() - Unexpected end of input.");
	  }
        else
          {
            std::map<char, Symbol>::iterator sym(symbolMap.find(newchar));
            if(sym == symbolMap.end())
              throw std::string("MyTokenIterator::getNextToken() - Invalid token.");
            else
              {
                currentSymbol = sym->second;
                currentValue = newchar;
              }
          }
      }
    else
      currentSymbol = Symbol::EOI;
  }
public:
  RegexTokenIterator(const std::string& _input = ""): input(_input),
                                                      inputPos(0),
                                                      symbolMap(),
                                                      escape('\\'),
                                                      escapedSymbol(regexSymbols::CHAR),
                                                      escapedSymbolValue(),
                                                      currentValue(),
                                                      currentSymbol(Symbol::START)
  {
    using namespace regexSymbols;

    // form '\0' (null char) to '~' (tilde = 126)
    for(char c(0); c <= 126; ++c)
      symbolMap[c] = CHAR;

    symbolMap['|'] = PIPE;
    symbolMap['*'] = KLEENSTAR;
    symbolMap['+'] = KLEENSTAR;
    symbolMap['?'] = KLEENSTAR;
    symbolMap['('] = LP;
    symbolMap[')'] = RP;

    escapedSymbolValue['n'] = '\n';
    escapedSymbolValue['t'] = '\t';
    escapedSymbolValue['f'] = '\f';
    escapedSymbolValue['r'] = '\r';
    escapedSymbolValue['b'] = '\b';
    escapedSymbolValue['a'] = '\a';
    escapedSymbolValue['0'] = '\0';
  
    getNextToken();
  }

  void setInput(const std::string& i){input = i; inputPos = 0; getNextToken(); }
  virtual ~RegexTokenIterator(){}

  RegexTokenIterator& operator++()
  { getNextToken(); return *this; }

  const Symbol& operator*() const { return currentSymbol; }
  const std::string& value() const { return currentValue; }
};

class RegexGrammar: public CFGrammar
{
public:
  RegexGrammar(): CFGrammar(Symbol::START)
  {
    using namespace regexSymbols;

    defProd(Symbol::START, ( REGEX, Symbol::EOI ));
 
    defProd(REGEX,         ( REGEX, CONCAT ));
    defProd(REGEX,         ( CONCAT ));
 
    defProd(CONCAT,        ( CONCAT, PIPE, ALT ));
    defProd(CONCAT,        ( ALT ));

    defProd(ALT,           ( C, KLEENSTAR ));
    defProd(ALT,           ( C ));

    defProd(C,             ( CHAR ));
    defProd(C,             ( LP, REGEX, RP ));
    defProd(C,             ( BRACKET ));

    // symbolsPrettyNames[CHAR]      = " c ";
    // symbolsPrettyNames[PIPE]      = " | ";
    // symbolsPrettyNames[KLEENSTAR] = " * ";
    // symbolsPrettyNames[LP]        = " ( ";
    // symbolsPrettyNames[RP]        = " ) ";
    // symbolsPrettyNames[EOI]       = " $ ";
    // symbolsPrettyNames[REGEX]     = " R ";
    // symbolsPrettyNames[START]     = " S ";
    // symbolsPrettyNames[CONCAT]    = " CC";
    // symbolsPrettyNames[ALT]       = "ALT";
    // symbolsPrettyNames[C]         = " C ";
    // symbolsPrettyNames[BRACKET]   = " []";
    
    CFGrammar::wrapUp();
  }
};

#endif /* _REGEXPARSER_H_ */
