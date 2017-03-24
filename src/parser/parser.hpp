#ifndef _PARSER_H_
#define _PARSER_H_

#include <iostream>
#include <list>
#include <set>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <iomanip>
#include <queue>


/**
 * Representation of a grammar's symbol, terminal or non terminal. A symbol is
 * an encapsulation of a short integer identifier.
 */
class Symbol
{
  friend std::ostream& operator<<(std::ostream& flux, const Symbol& s);

  static short idFactory;
  static short genNewId(){ return idFactory++; };

  short id;
  Symbol(short i): id(i) {}

  void check() const 
  { 
    if(id == -1) 
      throw std::string("Symbol::check() - This symbol is invalid.");
  }

public:
  static Symbol newSymbol() { return Symbol(genNewId()); }
  static const Symbol START, EOI;
  

  explicit Symbol(): id(-1) { /*std::cout << "invalid symbol created." << std::endl;*/ }
  Symbol(const Symbol& sym): id(sym.id) {}
  Symbol& operator=(const Symbol& sym){id = sym.id; return *this;}

  bool operator==(const Symbol& op) const { check(); return id == op.id; }
  bool operator!=(const Symbol& op) const { check(); return !this->operator==(op); }
  bool operator<(const Symbol& op) const { check(); return id < op.id; }
};

inline std::ostream& operator<<(std::ostream& flux, const Symbol& s)
{
  flux << s.id;
  return flux;
}

/**
 * An ordered list of symbols. This type is usually used to represent
 * the right hand side of a production rule.
 */
typedef std::vector<Symbol> SymbolList;

/**
 * A context-free grammar production rule. The left hand side is a single symbol,
 * and the right hand side is a ordered list of symbols.
 */
typedef std::pair<Symbol, SymbolList> Production;

/**
 * Representation of a context free (CF) grammar. A CF grammar is the tuple
 * (Symbols, Start, Productions), where Symbols is the set union of the terminal
 * and non terminal symbols involved in the Production rules, Start is the
 * initial symbol to which a parse tree must reduce, and Productions is a
 * set of ordered CF production rules.
 *
 * This class should be inherited to implement a specific grammar.
 */
struct CFGrammar
{
  std::vector<Symbol> terminals;
  std::vector<Symbol> nonTerminals;
  std::vector<Symbol> symbolSet;

  Symbol startSymbol;

  std::vector<Production> productionRules;

  bool isTerminal(const Symbol& s) const;
  bool isNonTerminal(const Symbol& s) const
  { return (std::find(nonTerminals.begin(),
                      nonTerminals.end(),
                      s) != nonTerminals.end()); }
protected:
public:
  /**
   * Mapping between symbols and printable (ascii) representation.
   * This map should be filled by the user, if needed.
   */
  std::map< Symbol, std::string > symbolsPrettyNames;

  /**
   * This method must be called once all the production rules have been defined.
   * In particular, the terminals, nonTerminals and symbolSet member variable are
   * filled by analysing the productionRules.
   *
   * The set of non terminal is built by collecting all the right hand side
   * of the production rules, the set of symbols by collecting all the symbols
   * in the rhs and lhs, and finally the set of terminal is built by taking the
   * set difference: Terminals = Symbols - NonTerminals.
   */
  void wrapUp();


  /**
   * Utility template method to add a production rule to the grammar from
   * a symbol and an plain array of symbols.
   * \param target Left hand side of the production rule
   * \param rhs Right hand side of the production rule, which is a C array of symbols.
   *
   * Usage example:
   * \code{.cpp}
   * CFGrammar cfg;
   *
   * Symbols rhs[] = {sym1, sym2, sym3};
   * cfg.addProduction(symTarget, rhs);
   * \endcode
   */
  template<size_t length>
  void addProduction(const Symbol& target, const Symbol (&rhs)[length])
  { productionRules.push_back(std::make_pair(target, SymbolList(rhs, rhs+length))); }

public:
  CFGrammar(Symbol s): terminals(),
                       nonTerminals(),
                       symbolSet(),
                       startSymbol(s),
                       productionRules(),
                       symbolsPrettyNames()
  {}
  virtual ~CFGrammar();

  /**
   * Return the printable representation of a symbol, if the user
   * defined it.
   * \param s The symbol whose (ascii) representation is to be returned.
   */
  const std::string& prettyName(Symbol s) const
  { 
    std::map<Symbol, std::string >::const_iterator name(symbolsPrettyNames.find(s));
    if(name != symbolsPrettyNames.end())
      return name->second;
    else throw std::string("CFGrammar::prettyName()"
                           " - The symbol's pretty name wasn't defined.");
  }
};
inline CFGrammar::~CFGrammar(){}


/**
 * \brief Helper class to ease the definition of production rules.
 *
 * \c SymbolListHelper encapsulate a \c SymbolList to overload the
 * ',' operator, and simplify the syntax to define the production rules
 * of a CF grammar, altogether with a short macro \c defProd.
 */
class SymbolListHelper
{
public:
  SymbolListHelper(Symbol s): list(1, s) {}
  SymbolListHelper(Symbol s1, Symbol s2): list()
  { list.push_back(s1), list.push_back(s2); }
  
  void push_back(Symbol s){ list.push_back(s); }
  SymbolList list;
};

inline SymbolListHelper operator,(const Symbol& s1, const Symbol& s2)
{ return SymbolListHelper(s1, s2); }

inline SymbolListHelper operator,(SymbolListHelper sl, const Symbol& s)
{ sl.push_back(s); return sl; }

#define defProd(lhs, rhs) {productionRules.push_back(std::make_pair(lhs, SymbolListHelper(rhs).list));}


/**
 * A ParserItem is the representation of a dot in a production rule. The position of the
 * dot defines the partial state of the parser, after eating some symbols.
 */
struct ParserItem
{
  /**
   * A ParserItem is associated with one of the production rule of a CFGrammar.
   * productionId is the index of the production rule in the 
   * std::vector<Production> CFGrammar::productionRules .
   */
  unsigned int productionId;

  /**
   * Position of the dot in the symbol list of the right hand side of the 
   * production rule. 
   *
   * An integer value of i means that the parser state is between symbols i-1 and i. In
   * particular, if i == 0, then the dot precede the first symbol, and if i == rhs.size(),
   * then the dot is after the last symbol of the right hand side. 
   */
  unsigned int parserPosition;

  ParserItem(unsigned int id, unsigned int pos): productionId(id),
                                                 parserPosition(pos)
  {}
};
inline bool operator<(const ParserItem& op1, const ParserItem& op2)
{ return (op1.productionId == op2.productionId)
    ? (op1.parserPosition < op2.parserPosition)
    : (op1.productionId < op2.productionId); }
inline bool operator==(const ParserItem& op1, const ParserItem& op2)
{ return op1.productionId == op2.productionId
    and op1.parserPosition == op2.parserPosition; }


/** 
 * \brief Configuration state of a LR parser
 *
 * A valid configuration state of an LR parser is a collection of parser Items,
 * which satisfy some constraints (The set of parser states defining a parser is
 * built by iterating through all the nodes of the graph, stating from the initial
 * state).
 */
typedef std::set<ParserItem> ParserState;


/**
 * \brief Representation of the LR parser associated to a CF grammar
 * 
 * An LR parser is defined by the transitions and goto tables, which
 * are built at initialization from the CF grammar production rules.
 * 
 */
struct LRParser
{
private:
  void buildConfigurationSet(const CFGrammar& grammar);
  void buildTransitionTable(const CFGrammar& grammar);
  void buildFirstSets(const CFGrammar& grammar);
  void buildFollowSets(const CFGrammar& grammar);

public:
  /** 
   * \brief The set of configuration state for this parser
   * 
   * \c configurationSet is the set of all the valid parser states. This member is only filled and
   * used in the parser initialization step, in particular it is used to build the 
   * transitions and goto tables. This is why THIS SHOULD NOT BE A MEMBER DATA.
   * However, this should not be moved away, since it is also used for PRINTING AND DEBUGING.
   */
  std::vector<ParserState> configurationSet;

  /**
   * \brief The state transition map
   *
   * The state transition function maps the couple (\c ParserState, \c Symbol) to the next \c ParserState. 
   * \c transitionTable[i][s] is the next state if the current state is \c i, and the next symbol on the
   * input is \c s.
   */
  std::vector< std::vector< short int > > transitionsTable;

  /**
   * \brief The goto state map
   *
   * The goto state map associate to the couple (\c ParserState, \c Symbol) the state to be pushed on the
   * stack once the production rule has been reduced to \c Symbol.
   */
  std::vector< std::vector< short int > > gotoTable;

  /**
   * \brief The state which reduce the start production rule.
   *
   * When this state is reached, the parsing process can stop.
   */
  size_t acceptingState;
  
  // Cached data (extracted from the grammar): Length of the rhs of each grammar rule:
  std::vector< unsigned int > ruleLengths;

  // Cached data (extracted from the grammar): The symbol on the lhs of each grammar rules:
  std::vector< Symbol > reduceSymbol;

  // Cached data (extracted from the grammar): Maps between symbol and index in the transition and goto tables:
  std::map< Symbol, unsigned int > nonTerminalMap;
  std::map< Symbol, unsigned int > terminalMap;

  // First and follow sets for each symbol. As with the \c configurationSet, these members are only used
  // during the constuction of the transitions and goto tables. They could be moved away, but they are
  // convenient for debuging purpose.
  std::map<Symbol, std::set<Symbol> > firsts;
  std::map<Symbol, std::set<Symbol> > follows;
  
  /**
   * \brief Build a LRParser from a context free grammar \c g.
   *
   * Several steps take place in order to build a functional left-right parser.
   * First of all, the
   */
  LRParser(const CFGrammar& g);

  void print(std::ostream& flux, const CFGrammar& grammar);
  void printFollowSets(std::ostream& flux, const CFGrammar& grammar);
  void printFirstSets(std::ostream& flux, const CFGrammar& grammar);
  void printConfigurationSet(std::ostream& flux, const CFGrammar& grammar);

  /**
   * \brief Complete the partial parser state.
   *
   * Given a set of ParserItems (as a \c ParserState), \c closeParserState recursively iterate
   * through all the production rules, and add them with a dot in the first position
   * if required, until the \c ParserState is stable under closure. This lead to a uniquely
   * defined \c ParserState, which can then represent a valid state of a parser.
   */
  static void closeParserState(ParserState& p,
                               const CFGrammar& grammar);

  // Find the successor state S(p, terminal)
  static ParserState computeSuccessorParserState(const ParserState& state,
                                                 const CFGrammar& grammar,
                                                 const Symbol& terminal);

  /**
   * \brief Check if p is a reducible state.
   *
   * A reducible state is a state is a ParserState which has exactly one
   * ParserItem whose dot is in the last position.
   *
   * If more than one ParserItem has a dot in the last position, we are facing a
   * reduce-reduce conflict, since one cannot choose whose rule to reduce without
   * looking at the following symbols on the input.
   */
  static bool isReducible(const ParserState& p, 
                          const CFGrammar& grammar,
                          unsigned int& productionId);

  /**
   * \brief Find the rule id which produce the symbol \c s.
   *
   * This function only return normally if a unique production rule is found.
   * If either more than one or no rule is found, a \c std::string is thrown.
   */
  static size_t findProduction(const CFGrammar& g, Symbol s);
};

void print(std::ostream& flux,
           const ParserItem& item,
           const CFGrammar& grammar);

#endif /* _PARSER_H_ */
