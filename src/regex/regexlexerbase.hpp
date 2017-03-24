#ifndef _REGEXLEXERBASE_H_
#define _REGEXLEXERBASE_H_

#include <vector>
#include <iostream>
#include <iterator>
#include <string>

#include "regex.hpp"
#include "regexparser.hpp"
#include "regexvisitor.hpp"

class LexerBase {
  RegexGrammar regex_grammar;
  LRParser regex_parser;
  RegexTokenIterator input;
    
  regex *token_regex_dfa, *skipper_regex_dfa;

  std::vector<Symbol> token_symbols;
  std::vector<astRegexNode*> tokens;
  astRegexNode* skipper;

  CharInput char_input;
  
  Symbol current_symbol;
  std::string current_value;

  //LexerBase(const LexerBase&);
  LexerBase& operator=(const LexerBase&);
  
 protected:
  astRegexNode* buildAst(const std::string& token) {
    input.setInput(token);
    AstNode* ast(ParseInputToAst(regex_parser, regex_grammar, input));

    AstToRegex atr;
    ast->accept(&atr);

    delete ast;
    return atr.result();
  }
  
 public:
  void addToken(const std::string& token, Symbol s) {
    try {
      token_symbols.push_back(s);

      tokens.push_back(buildAst(token));
      tokens.back()->setDelimiter(token_symbols.size());
    }
    catch(const std::string& e) {
      std::cout << "LexerBase::addToken(\"" << token << "\", " << s << "):"
                << std::endl;
      throw;
    }
  }

  void setSkipper(const std::string& skip) {
    skipper = buildAst(skip);
    skipper->setDelimiter(1);
  }

  void compile() {
    std::vector<astRegexNode*>::iterator it(tokens.begin());
    astRegexNode * const left(*(it++));
    astRegexNode * const right(*(it++));
    
    astRegexAlt* alt(new astRegexAltTopLevel(left, right));

    while (it != tokens.end()) {
      alt = new astRegexAltTopLevel(alt, *it);
      ++it;
    }

    token_regex_dfa = new regex(alt);
    skipper_regex_dfa = new regex(skipper);
    
    delete alt;
    delete skipper;
    skipper = NULL;
    tokens.clear();
  }

  void getNextToken() {
    using namespace regexSymbols;

    unsigned int token_id(0);
    std::string buffer;
    match_regex_longest(*skipper_regex_dfa, char_input, buffer, token_id);
    buffer.clear();

    if (not char_input.good()) {
      current_symbol = Symbol::EOI;
    } else if (match_regex_longest(*token_regex_dfa,
                                   char_input,
                                   buffer,
                                   token_id)) {
      current_value = buffer;
      current_symbol = token_symbols[token_id - 1];
    } else {
      throw std::string("LexerBase::operator++() - "
                        "Unrecognized token.");
    }
  }
  
 public:
  LexerBase(): regex_grammar(),
               regex_parser(regex_grammar),
               input(),
               token_regex_dfa(NULL),
               skipper_regex_dfa(NULL),
               token_symbols(),
               tokens(),
               skipper(NULL),
               current_symbol(),
               current_value(),
               char_input() {}
  
  explicit LexerBase(std::istream& input_stream)
      : regex_grammar(),
        regex_parser(regex_grammar),
        input(),
        token_regex_dfa(NULL),
        skipper_regex_dfa(NULL),
        token_symbols(),
        tokens(),
        skipper(NULL),
        current_symbol(),
        current_value(),
        char_input(&input_stream) {}
  
  void setInput(std::istream& input_stream) {
    char_input.set_input_stream(input_stream);
    if (token_regex_dfa)
      getNextToken();
  }
  
  virtual ~LexerBase() {
    delete token_regex_dfa;
    delete skipper_regex_dfa;
  }

  LexerBase& operator++() {
    getNextToken();
    return *this;
  }
  
  const Symbol& operator*() const { return current_symbol; }
  
  const std::string& value() const { return current_value; }
};


#endif /* _REGEXLEXERBASE_H_ */
