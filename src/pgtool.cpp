#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <algorithm>
#include <string>
#include <vector>

#include "regex/regexlexerbase.hpp"
#include "aluscript/aluscriptdatatypes.hpp"

#include "utils/meta.hpp"
#include "utils/command_line_parser.hpp"



namespace pgSymbols {
Symbol NT(Symbol::newSymbol());
Symbol T(Symbol::newSymbol());
Symbol PIPE(Symbol::newSymbol());
Symbol DEFOP(Symbol::newSymbol());
Symbol REGEX(Symbol::newSymbol());
Symbol END_OF_RULE(Symbol::newSymbol());

Symbol DEF(Symbol::newSymbol());
Symbol DEFLIST(Symbol::newSymbol());
Symbol ALTLIST(Symbol::newSymbol());
Symbol CONCAT(Symbol::newSymbol());
Symbol SYM(Symbol::newSymbol());
}

// Parser Generator lexer:
class PGLexer: public LexerBase {
 public:
  PGLexer(std::istream& s): LexerBase(s) {
    using namespace pgSymbols;

    try {
      addToken("<[-a-z0-9]+>", NT);
      addToken("[-A-Z0-9]+", T);
      addToken("::=", DEFOP);
      addToken("\\|", PIPE);
      addToken("/([^/]|(\\\\/))+/", REGEX);
      addToken(".", END_OF_RULE);

      setSkipper("([ \n\t\r\f]|(;[^;]*;))*");
    }
    catch(const std::string& e) {
      std::cout << "PGLexer::PGLexer():" << std::endl;
      throw;
    }
    LexerBase::compile();
    LexerBase::getNextToken();
  }
};

class PGGrammar: public CFGrammar {
 public:
  PGGrammar(): CFGrammar(Symbol::START) {
    using namespace pgSymbols;

    symbolsPrettyNames[Symbol::START] = "<start>";
    symbolsPrettyNames[Symbol::EOI] = "$";
    symbolsPrettyNames[DEFLIST] = "<def-list>";
    symbolsPrettyNames[PIPE] = "'|'";
    symbolsPrettyNames[DEFOP] = "'::='";
    symbolsPrettyNames[DEF] = "<def>";
    symbolsPrettyNames[REGEX] = "REGEX";
    symbolsPrettyNames[END_OF_RULE] = "EOR";
    symbolsPrettyNames[T] = "T";
    symbolsPrettyNames[NT] = "NT";
    symbolsPrettyNames[DEFLIST] = "<def-list>";
    symbolsPrettyNames[ALTLIST] = "<alt-list>";
    symbolsPrettyNames[CONCAT] = "<concat>";
    symbolsPrettyNames[SYM] = "<sym>";

    defProd(Symbol::START, (DEFLIST, Symbol::EOI));
    defProd(DEFLIST,       (DEFLIST, DEF));
    defProd(DEFLIST,       (DEF));
    defProd(DEF,           (T, DEFOP, REGEX, END_OF_RULE));
    defProd(DEF,           (NT, DEFOP, ALTLIST, END_OF_RULE));
    defProd(ALTLIST,       (ALTLIST, PIPE, CONCAT));
    defProd(ALTLIST,       (CONCAT));
    defProd(CONCAT,        (CONCAT, SYM));
    defProd(CONCAT,        (SYM));
    defProd(SYM,           (NT));
    defProd(SYM,           (T));
    
    CFGrammar::wrapUp();
  }
};


class AstLeftListExtractor: public AstTreeVisitorI {
 public:
  Symbol listSymbol;
  std::vector<AstNode*> elements;

  explicit AstLeftListExtractor(Symbol _listSymbol): listSymbol(_listSymbol) {}
  virtual ~AstLeftListExtractor() {}

  void operator()(AstNode* node) {
    node->accept(this);
    std::reverse(elements.begin(), elements.end());
  }

  virtual void visit(AstProduction* node) {
    if (node->token == listSymbol) {
      if (node->children.size() == 1) {
        elements.push_back(node->children.back());
      } else if (node->children.size() >= 2) {
        elements.push_back(node->children.back());
        node->children[0]->accept(this);
      } else {
        throw std::string("[error] AstLeftListExtractor::visit(AstProduction) -"
                          " Humm.. strange.");
      }
    }
  }
  virtual void visit(AstLeaf* node) {
    throw std::string("[error] AstLeftListExtractor::visite(AstLeaf*) - "
                      "I should never have ended up here.");
  }
};


class AstTerminalExtractor: public AstTreeVisitorI {
  AstLeaf* leaf;
 public:
  AstTerminalExtractor(): leaf(NULL) {}
  virtual ~AstTerminalExtractor() {}

  const std::string& operator()(AstNode* node) {
    node->accept(this);
    return leaf->value;
  }

  virtual void visit(AstProduction* node) {
    if (node->children.size() != 1)
      throw std::string("[error] AstTerminalExtractor::visit(AstProduction*) - "
                        "error.");
    node->children[0]->accept(this);
  }
  virtual void visit(AstLeaf* node)
  { leaf = node; }
};


class AstAlternativeBuilder: public AstTreeVisitorI {
  std::vector<std::string> concat;
  
 public:
  AstAlternativeBuilder() {}
  virtual ~AstAlternativeBuilder() {}

  const std::vector<std::string>& operator()(AstNode* node) {
    concat.clear();
    node->accept(this);
    return concat;
  }

  virtual void visit(AstProduction* node) {
    AstLeftListExtractor listExtractor(pgSymbols::CONCAT);
    listExtractor(node);
    concat.resize(listExtractor.elements.size());
    
    AstTerminalExtractor terminalExtractor;
    for (unsigned int i(0); i < concat.size(); ++i)
      concat[i] = terminalExtractor(listExtractor.elements[i]);
  }
  virtual void visit(AstLeaf* node) {
    throw std::string("[error] AstAlternativeBuilder::visit(AstLeaf) - "
                      "I should never have ended here.");
  }
};


class AstRuleBuilder: public AstTreeVisitorI {
 public:
  typedef std::pair<std::string, std::vector<std::vector<std::string> > > ProductionRuleStr;
  typedef std::pair<std::string, std::string> TerminalDefinitionStr;
  std::vector<ProductionRuleStr> productionRules;
  std::vector<TerminalDefinitionStr> terminalDefinitions;

  typedef void (AstRuleBuilder::*actionP)(const std::vector<AstNode*>& children);
  std::vector<actionP> productionActions;

  std::map<std::string, Symbol> terminal_symbols;

  void actionProduction(const std::vector<AstNode*>& children) {
    AstTerminalExtractor terminal_extractor;
    AstLeftListExtractor alternatives(pgSymbols::ALTLIST);
    alternatives(children[2]);

    productionRules.push_back(std::make_pair(terminal_extractor(children[0]),
                                             std::vector<std::vector<std::string> >(alternatives.elements.size())));

    std::transform(alternatives.elements.begin(),
                   alternatives.elements.end(),
                   productionRules.back().second.begin(),
                   AstAlternativeBuilder());
  }
  void actionTerminal(const std::vector<AstNode*>& children) {
    AstTerminalExtractor terminalExtractor;
    terminalDefinitions.push_back(std::make_pair(terminalExtractor(children[0]),
                                                 terminalExtractor(children[2])));
  }

 public:
  AstRuleBuilder(): productionActions(11, NULL) {
    productionActions[3] = &AstRuleBuilder::actionTerminal;
    productionActions[4] = &AstRuleBuilder::actionProduction;
  }
  virtual ~AstRuleBuilder() {}

  void operator()(AstNode* node) { node->accept(this); }
  
  virtual void visit(AstProduction* node)
  { (this->*productionActions[node->productionId])(node->children); }
  virtual void visit(AstLeaf* node)
  {}
  

  void buildTerminalSymbols() {
    if (terminalDefinitions.size() == 0)
      throw std::string("[error] AstRuleBuilder::buildTerminalSymbols() - "
                        "No terminal definitions available.");

    std::vector<std::string> defined_terminals(terminalDefinitions.size());
    std::transform(terminalDefinitions.begin(),
                   terminalDefinitions.end(),
                   defined_terminals.begin(),
                   alucell::member(&TerminalDefinitionStr::first));

    for (unsigned int i(0); i < defined_terminals.size(); ++i)
      terminal_symbols[defined_terminals[i]] = Symbol::newSymbol();
    terminal_symbols["EOI"] = Symbol::EOI;
  }

  LexerBase generateLexer() {
    /*
     * WARNING: no error checking whatsoever!
     * (duplicate terminal symbols, missing terminal definitions, etc.)
     */
    if (terminal_symbols.size() == 0)
      buildTerminalSymbols();


    LexerBase lexer;
    for (std::vector<TerminalDefinitionStr>::iterator it(terminalDefinitions.begin());
         it != terminalDefinitions.end();
         ++it) {
      lexer.addToken(it->second.substr(1, it->second.size()-2),
                     terminal_symbols[it->first]);
    }
    lexer.setSkipper("[ \t\r\f\n]*");
    lexer.compile();
    
    return lexer;
  }


  CFGrammar generateGrammar() {
    if (terminal_symbols.size() == 0)
      buildTerminalSymbols();


    std::map<std::string, Symbol> symbols(terminal_symbols);
    for (std::vector<ProductionRuleStr>::iterator it(productionRules.begin());
         it != productionRules.end();
         ++it) {
      if (it->first == "<start>")
        symbols["<start>"] = Symbol::START;
      else
        symbols[it->first] = Symbol::newSymbol();
    }
    
    CFGrammar grammar(symbols["<start>"]);
    for (std::vector<ProductionRuleStr>::iterator prod(productionRules.begin());
         prod != productionRules.end();
         ++prod) {
      for (std::vector<std::vector<std::string> >::iterator alt(prod->second.begin());
           alt != prod->second.end();
           ++alt) {
        std::vector<Symbol> alt_symbol_list(alt->size());

        for (std::vector<std::string>::iterator sym_name(alt->begin());
             sym_name != alt->end();
             ++sym_name) {
          std::map<std::string, Symbol>::iterator sym(symbols.find(*sym_name));
          if (sym != symbols.end())
            alt_symbol_list.at(std::distance(sym_name, alt->begin())) = sym->second;
          else
            throw std::string("generateGrammar() - undefinded symbol name: ")
                + *sym_name;
        }


        /*std::transform(alt->begin(),
                       alt->end(),
                       alt_symbol_list.begin(),
                       bound_mem_fun1<std::map<std::string, Symbol>&,
                                      >(symbols,
                                      &std::map<std::string, Symbol>::operator[]));*/
        
        grammar.productionRules.push_back(std::make_pair(symbols[prod->first],
                                                         SymbolList(alt_symbol_list.begin(),
                                                                    alt_symbol_list.end())));
      }
    }
    
    for (std::map<std::string, Symbol>::iterator it(symbols.begin());
         it != symbols.end();
         ++it)
      grammar.symbolsPrettyNames[it->second] = it->first;
      
    grammar.wrapUp();
    return grammar;
  }
};

class LRGrammarBuilder: public AstTreeVisitorI {
 public:
  AstRuleBuilder ruleBuilder;

  LRGrammarBuilder() {}
  virtual ~LRGrammarBuilder() {}
    
  virtual void visit(AstProduction* node) {
    AstLeftListExtractor deflist(pgSymbols::DEFLIST);
    deflist(node);
    
    std::for_each<std::vector<AstNode*>::iterator,
                  AstRuleBuilder&>(deflist.elements.begin(),
                                   deflist.elements.end(),
                                   ruleBuilder);
  }
  virtual void visit(AstLeaf* node) {}
};

int main(int argc, char** argv) {
  CommandLine cmd;
  
  ParameterArgument<std::string>
      grammar_filename('g', "File name where the grammar "
                       "is defined.", "filename", false);
  cmd.add(&grammar_filename);
  
  ParameterArgument<std::string>
      source_filename('s', "File name of the source to "
                      "be checked against a given "
                      "grammar.", "filename", false);
  cmd.add(&source_filename);

  SwitchArgument
      tokenize('t', "Tokenize the source file instead "
               "of attempting to parse it.");
  cmd.add(&tokenize);

  SwitchArgument
      verbose('v', "Enable verbose mode: print parser "
              "configurations, detailed diagnostic on error.");
  cmd.add(&verbose);
  
  try {
    cmd.parse(argc, argv);

    PGGrammar pgg;
    LRParser pgp(pgg);

    std::ifstream grammar_stream(grammar_filename.value().c_str(),
                                 std::ios::in);
    grammar_stream >> std::noskipws;
    std::ifstream source_stream(source_filename.value().c_str(),
                                std::ios::in);
    source_stream >> std::noskipws;

    PGLexer ll(grammar_stream);

    AstNode* grammar_ast(NULL);
    if ((grammar_ast = ParseInputToAst(pgp, pgg, ll)) != NULL) {
      std::cout << "Valid grammar syntax." << std::endl;

      LRGrammarBuilder g;
      grammar_ast->accept(&g);

      CFGrammar generated_grammar(g.ruleBuilder.generateGrammar());
      LRParser p(generated_grammar);
      if (verbose.value())
        p.print(std::cout, generated_grammar);
          
      LexerBase generated_lexer(g.ruleBuilder.generateLexer());
      generated_lexer.setInput(source_stream);


      if (tokenize.value()) {
        tokenizeInput(generated_lexer, generated_grammar);
      } else {
        AstNode* source_ast(NULL);
        if ((source_ast = ParseInputToAst(p,
                                          generated_grammar,
                                          generated_lexer)) != NULL) {
          std::cout << "Valid source syntax." << std::endl;
          delete source_ast;
        } else {
          std::cout << "Source syntax error." << std::endl;
        }
      }
    } else {
      std::cout << "Grammar syntax error." << std::endl;
    }
    delete grammar_ast;
  }
  catch(const std::string& e) {
    std::cout << e << std::endl;
  }
  return 0;
}
