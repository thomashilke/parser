#ifndef _ALUSCRIPTDATATYPES_H_
#define _ALUSCRIPTDATATYPES_H_

#include <iostream>
#include <cmath>

#include "../parser/parsefunctions.hpp"

namespace aluscriptSymbols
{
  extern Symbol STMTLIST;
  extern Symbol STMT;
  extern Symbol FLOW;
  extern Symbol EXPRLIST;
  extern Symbol EXPR;
  extern Symbol ID;
  extern Symbol LP;
  extern Symbol RP;
  extern Symbol REAL;
  extern Symbol STRING;
}


struct Environment;

class AluscriptVariable;
class AluscriptProxyVariable;
class AluscriptConsCellVariable;

class AluscriptFormVariable;
class AluscriptLambdaVariable;
class AluscriptMacroVariable;

class AluscriptRealVariable;
class AluscriptStringVariable;
class AluscriptSymbolVariable;
class AluscriptNilVariable;

class IEvaluator
{
public:
  virtual ~IEvaluator(){}
  virtual AluscriptVariable* eval(AluscriptProxyVariable*) = 0;
  virtual AluscriptVariable* eval(AluscriptConsCellVariable*) = 0;

  virtual AluscriptVariable* eval(AluscriptFormVariable*) = 0;

  virtual AluscriptVariable* eval(AluscriptRealVariable*) = 0;
  virtual AluscriptVariable* eval(AluscriptStringVariable*) = 0;
  virtual AluscriptVariable* eval(AluscriptSymbolVariable*) = 0;
  virtual AluscriptVariable* eval(AluscriptNilVariable*) = 0;
};
class AluscriptEvaluator;

class AluscriptVariable
{
  AluscriptVariable& operator=(const AluscriptVariable&);

protected:
  AluscriptVariable(const AluscriptVariable&){}

public:
  AluscriptVariable(){}
  virtual ~AluscriptVariable(){}

  virtual AluscriptVariable* eval(IEvaluator* ev) = 0;
  virtual AluscriptVariable* clone() const = 0;

  virtual AluscriptRealVariable* toReal()
  { throw std::string("AluscriptVariable::toReal() - This is not a real."); }    
  virtual AluscriptFormVariable* toForm()
  { throw std::string("AluscriptVariable::toForm() - This is not a form."); }
  virtual AluscriptSymbolVariable* toSymbol()
  { throw std::string("AluscriptVariable::toSymbol() - This is not a Symbol."); }

  virtual void print(std::ostream& flux) const = 0;
};

class AluscriptProxyVariable: public AluscriptVariable
{
  AluscriptProxyVariable(const AluscriptProxyVariable&);
  AluscriptProxyVariable& operator=(const AluscriptProxyVariable&);

  AluscriptVariable* value;
public:
  AluscriptProxyVariable(): value(NULL){}
  virtual ~AluscriptProxyVariable(){}

  virtual AluscriptVariable* eval(IEvaluator* ev){ return ev->eval(this); }
  virtual AluscriptVariable* clone() const { return value->clone(); }

  virtual void print(std::ostream& flux) const { value->print(flux); }
  AluscriptVariable* getVar(){return value;}
};

class AluscriptRealVariable: public AluscriptVariable
{
  double value;
public:
  AluscriptRealVariable(double v): value(v) {}
  virtual ~AluscriptRealVariable(){}

  virtual AluscriptVariable* eval(IEvaluator* ev) { return ev->eval(this); }
  virtual AluscriptVariable* clone() const { return new AluscriptRealVariable(value); }

  virtual AluscriptRealVariable* toReal() { return this; }

  double getValue() const {return value;}

  virtual void print(std::ostream& flux) const { flux << "real(" << value << ")"; }
};

class AluscriptConsCellVariable: public AluscriptVariable
{
  std::pair<AluscriptVariable*, AluscriptVariable*> value;
public:
  AluscriptConsCellVariable(AluscriptVariable* car, AluscriptVariable* cdr): value(car, cdr){}
  virtual ~AluscriptConsCellVariable(){}

  virtual AluscriptVariable* eval(IEvaluator* ev) { return ev->eval(this); }
  virtual AluscriptVariable* clone() const { return NULL; }

  AluscriptVariable* Car(){return value.first;}
  AluscriptVariable* Cdr(){return value.second;}

  virtual void print(std::ostream& flux) const 
  { 
    flux << "cons(";
    value.first->print(flux); 
    flux << " . ";
    value.second->print(flux);
    flux << ")"; 
  }
};

class AluscriptStringVariable: public AluscriptVariable
{
  std::string value;
public:
  AluscriptStringVariable(const std::string& v): value(v){}
  virtual ~AluscriptStringVariable(){}

  virtual AluscriptVariable* eval(IEvaluator* ev) { return ev->eval(this); }
  virtual AluscriptVariable* clone() const { return new AluscriptStringVariable(value); }

  virtual void print(std::ostream& flux) const { flux << "string(" << value << ")"; }
};

class AluscriptSymbolVariable: public AluscriptVariable
{
  std::string value;
public:
  AluscriptSymbolVariable(const std::string& v): value(v){}
  virtual ~AluscriptSymbolVariable(){}

  virtual AluscriptSymbolVariable* toSymbol(){return this;}
  
  virtual AluscriptVariable* eval(IEvaluator* ev) { return ev->eval(this); }
  virtual AluscriptVariable* clone() const { return new AluscriptSymbolVariable(value); }

  const std::string& getName() const { return value; }

  virtual void print(std::ostream& flux) const { flux << "symbol(" << value << ")"; }
};
inline bool operator<(const AluscriptSymbolVariable& op1, const AluscriptSymbolVariable& op2)
{ return op1.getName() < op2.getName(); }

class AluscriptNilVariable: public AluscriptVariable
{
public:
  virtual ~AluscriptNilVariable(){}

  virtual AluscriptVariable* eval(IEvaluator* ev) { return ev->eval(this); }
  virtual AluscriptVariable* clone() const { return new AluscriptNilVariable(*this); }

  virtual void print(std::ostream& flux) const { flux << "nil"; }
};


class AluscriptFormVariable: public AluscriptVariable
{
public:
  virtual ~AluscriptFormVariable(){}

  virtual AluscriptVariable* eval(IEvaluator* ev) { return ev->eval(this); }
  virtual AluscriptVariable* execute(AluscriptEvaluator&, 
                                     std::vector<AluscriptVariable*>::const_iterator,
                                     std::vector<AluscriptVariable*>::const_iterator) = 0;

  virtual AluscriptFormVariable* toForm() { return this; }
};

class AluscriptMacroVariable: public AluscriptFormVariable
{
public:
  virtual ~AluscriptMacroVariable(){}

  virtual AluscriptVariable* clone() const { return new AluscriptMacroVariable(*this); }

  virtual AluscriptVariable* execute(AluscriptEvaluator&, 
                                     std::vector<AluscriptVariable*>::const_iterator,
                                     std::vector<AluscriptVariable*>::const_iterator);

  virtual void print(std::ostream& flux) const { flux << "macro"; }
};

class AluscriptLambdaVariable: public AluscriptFormVariable
{
public:
  virtual ~AluscriptLambdaVariable(){}

  virtual AluscriptVariable* clone() const { return new AluscriptLambdaVariable(*this); }

  virtual AluscriptVariable* execute(AluscriptEvaluator&, 
                                     std::vector<AluscriptVariable*>::const_iterator,
                                     std::vector<AluscriptVariable*>::const_iterator);

  virtual void print(std::ostream& flux) const { flux << "lambda"; }
};

class AluscriptQuoteVariable: public AluscriptFormVariable
{
public:
  virtual ~AluscriptQuoteVariable(){}
  
  virtual AluscriptVariable* clone() const { return new AluscriptQuoteVariable(*this); }

  virtual AluscriptVariable* execute(AluscriptEvaluator&, 
                                     std::vector<AluscriptVariable*>::const_iterator start,
                                     std::vector<AluscriptVariable*>::const_iterator end)
  {
    if(std::distance(start, end) != 1)
      throw std::string("AluscriptQuoteVariable::execute() - Quote expects one argument.");
    return *start;
  }

  virtual void print(std::ostream& flux) const { flux << "quote builtin"; }
};

class AluscriptSetVariable: public AluscriptFormVariable
{
public:
  virtual ~AluscriptSetVariable(){}
  
  virtual AluscriptVariable* clone() const { return new AluscriptSetVariable(*this); }

  virtual AluscriptVariable* execute(AluscriptEvaluator& ev, 
                                     std::vector<AluscriptVariable*>::const_iterator start,
                                     std::vector<AluscriptVariable*>::const_iterator end);

  virtual void print(std::ostream& flux) const { flux << "set builtin"; }
};

//TODO
class AluscriptDefLambdaVariable: public AluscriptFormVariable
{};

class AluscriptDefMacroVariable: public AluscriptFormVariable
{};

class AluscriptLetVariable: public AluscriptFormVariable
{};

class AluscriptWhileVariable: public AluscriptFormVariable
{};

class AluscriptCondVariable: public AluscriptFormVariable
{};

class AluscriptFunCallVariable: public AluscriptFormVariable
{};

template<typename DataType> struct AluscriptVarToDataTypeMap
{static DataType extract(AluscriptVariable* var)
  { throw std::string("AluscriptVarToDataTypeMap::extract() - No admissible conversion"); }};

template<> struct AluscriptVarToDataTypeMap<double>
{ static double extract(AluscriptVariable* var){ return var->toReal()->getValue(); }};

struct BinaryPlus   { static double op(double a, double b){return a+b;}};
struct BinaryMinus  { static double op(double a, double b){return a-b;}};
struct BinaryMult   { static double op(double a, double b){return a*b;}};
struct BinaryDivide { static double op(double a, double b){return a/b;}};

template<class Operation>
class AluscriptBinaryOpVariable: public AluscriptFormVariable
{
public:
  virtual ~AluscriptBinaryOpVariable(){}
  virtual AluscriptVariable* clone() const { return new AluscriptBinaryOpVariable(*this); }
  virtual AluscriptVariable* execute(AluscriptEvaluator& ev,
                                     std::vector<AluscriptVariable*>::const_iterator start,
                                     std::vector<AluscriptVariable*>::const_iterator end);
  virtual void print(std::ostream& flux) const { flux << "binary op"; }
};

template<class Ret, class AluscriptRet,  class Arg>
class AluscriptUnaryBuiltinVariable: public AluscriptFormVariable
{
  typedef Ret (*FunT)(Arg);
  FunT fPtr;
public:
  AluscriptUnaryBuiltinVariable(FunT f): fPtr(f) {}
  virtual ~AluscriptUnaryBuiltinVariable(){}
  
  virtual AluscriptVariable* clone() const { return new AluscriptUnaryBuiltinVariable(*this); }

  virtual AluscriptVariable* execute(AluscriptEvaluator& ev,
                                     std::vector<AluscriptVariable*>::const_iterator start,
                                     std::vector<AluscriptVariable*>::const_iterator end);

  virtual void print(std::ostream& flux) const { flux << "unary builtin"; }
};

template<class Ret, class AluscriptRet,  class Arg1, class Arg2>
class AluscriptBinaryBuiltinVariable: public AluscriptFormVariable
{
  typedef Ret (*FunT)(Arg1, Arg2);
  FunT fPtr;
public:
  AluscriptBinaryBuiltinVariable(FunT f): fPtr(f) {}
  virtual ~AluscriptBinaryBuiltinVariable(){}
  
  virtual AluscriptVariable* clone() const { return new AluscriptBinaryBuiltinVariable(*this); }

  virtual AluscriptVariable* execute(AluscriptEvaluator&, 
                                     std::vector<AluscriptVariable*>::const_iterator start,
                                     std::vector<AluscriptVariable*>::const_iterator end);

  virtual void print(std::ostream& flux) const { flux << "binary builtin"; }
};


class AsciiDump: public AstTreeVisitorI
{
  CFGrammar& grammar;
  std::ostream& flux;
public:
  AsciiDump(CFGrammar& g, std::ostream& f): grammar(g), flux(f) {}
  void visit(AstLeaf*node)
  { flux << grammar.prettyName(node->token) << "[" << node->value << "] "; }
  void visit(AstProduction* node)
  {
    flux << grammar.prettyName(node->token) << "( "; 
    for(unsigned int i(0); i < node->children.size(); ++i)
      {
        node->children[i]->accept(this);
        if(i < node->children.size()-1)
          flux << ", ";
      }
    flux << ")";
  }
};

class ExprToAluscriptList: public AstTreeVisitorI
{
  ExprToAluscriptList(const ExprToAluscriptList&);
  ExprToAluscriptList& operator=(const ExprToAluscriptList&);

  typedef AluscriptVariable* (ExprToAluscriptList::*actionP)
    (const std::vector<AstNode*>& children);
  typedef AluscriptVariable* (ExprToAluscriptList::*actionT)
    (const std::string& value);

  std::map<Symbol, actionT> terminalActions;
  std::vector<actionP> productionActions;

  AluscriptVariable* returnFromRec;

  AluscriptVariable* actionString(const std::string& value)
  { return new AluscriptStringVariable(value); }

  AluscriptVariable* actionReal(const std::string& value)
  { return new AluscriptRealVariable(std::atof(value.c_str())); }

  AluscriptVariable* actionSymbol(const std::string& value)
  { return new AluscriptSymbolVariable(value); }


  AluscriptVariable* actionForward(const std::vector<AstNode*>& children)
  { children[0]->accept(this); return returnFromRec; }

  AluscriptVariable* actionPList(const std::vector<AstNode*>& children)
  { children[1]->accept(this); return returnFromRec; }

  AluscriptVariable* actionExprList(const std::vector<AstNode*>& children)
  {
    children[0]->accept(this);
    AluscriptVariable* car(returnFromRec);

    children[1]->accept(this);
    AluscriptVariable* cdr(returnFromRec);

    return new AluscriptConsCellVariable(car, cdr);
  }

  AluscriptVariable* actionExprListEnd(const std::vector<AstNode*>& children)
  {
    children[0]->accept(this);
    return new AluscriptConsCellVariable(returnFromRec,
                                         new AluscriptNilVariable());
  }
  
  AluscriptVariable* actionNullList(const std::vector<AstNode*>&)
  { return new AluscriptNilVariable(); }

public:
  ExprToAluscriptList(): terminalActions(), productionActions(), returnFromRec(NULL)
  {
    using namespace aluscriptSymbols;

    terminalActions[STRING] = &ExprToAluscriptList::actionString;
    terminalActions[REAL]   = &ExprToAluscriptList::actionReal;
    terminalActions[ID]     = &ExprToAluscriptList::actionSymbol;

    productionActions.push_back(&ExprToAluscriptList::actionForward);
    productionActions.push_back(&ExprToAluscriptList::actionForward);
    productionActions.push_back(&ExprToAluscriptList::actionForward);

    productionActions.push_back(&ExprToAluscriptList::actionPList);
    productionActions.push_back(&ExprToAluscriptList::actionNullList);
    productionActions.push_back(&ExprToAluscriptList::actionExprList);
    productionActions.push_back(&ExprToAluscriptList::actionExprListEnd);
  }

  virtual ~ExprToAluscriptList(){}

  virtual void visit(AstProduction* node)
  { returnFromRec
      = (this->*(productionActions[node->productionId - 5]))(node->children); }
  virtual void visit(AstLeaf* node)
  { returnFromRec
      = (this->*(terminalActions[node->token]))(node->value); }

  AluscriptVariable* result(){return returnFromRec;}
};


class AluscriptListFlatten: public IEvaluator
{
  AluscriptListFlatten(const AluscriptListFlatten&);
  AluscriptListFlatten& operator=(const AluscriptListFlatten&);

  std::vector<AluscriptVariable*> children;

public:
  AluscriptListFlatten(AluscriptVariable* plist): children()
  {plist->eval(this);}
  virtual ~AluscriptListFlatten(){}

  virtual AluscriptVariable* eval(AluscriptProxyVariable*){ return NULL; }
  virtual AluscriptVariable* eval(AluscriptConsCellVariable* cc)
  {
    children.push_back(cc->Car());
    cc->Cdr()->eval(this);

    return NULL;
  }

  virtual AluscriptVariable* eval(AluscriptFormVariable*){ return NULL; }

  virtual AluscriptVariable* eval(AluscriptRealVariable*){ return NULL; }
  virtual AluscriptVariable* eval(AluscriptStringVariable*){ return NULL; }
  virtual AluscriptVariable* eval(AluscriptSymbolVariable*){ return NULL; }
  virtual AluscriptVariable* eval(AluscriptNilVariable*){ return NULL; }

  const std::vector<AluscriptVariable*>& getElements(){return children;}
};

struct Environment
{
  Environment(): scopes() { scopes.push_back(ScopeT()); }

  typedef std::map<AluscriptSymbolVariable, AluscriptVariable*> ScopeT;
  std::list<ScopeT> scopes;

  AluscriptVariable* resolve(AluscriptSymbolVariable* sym)
  {
    ScopeT::iterator var;
    if((var = scopes.back().find(*sym)) == scopes.back().end())
      if((var = scopes.front().find(*sym)) == scopes.front().end())
        throw std::string("Environment::resolve(AluscriptSymbolVariable*) - "
                          "Symbol not defined: " + sym->getName());
    return var->second;
  }
  void pushScope(){ scopes.push_back(ScopeT()); }
  void pushScope(const ScopeT& s){ scopes.push_back(s); }
  void popScope(){ if(scopes.size() > 2) scopes.pop_back(); }
  void bind(const AluscriptSymbolVariable& sym, AluscriptVariable* var){ scopes.back()[sym] = var; }
};

class AluscriptEvaluator: public IEvaluator
{
  Environment env;
public:
  AluscriptEvaluator(): env()
  {
    env.bind(AluscriptSymbolVariable("sin"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::sin));
    env.bind(AluscriptSymbolVariable("cos"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::cos));
    env.bind(AluscriptSymbolVariable("tan"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::tan));
    env.bind(AluscriptSymbolVariable("acos"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::acos));
    env.bind(AluscriptSymbolVariable("asin"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::asin));
    env.bind(AluscriptSymbolVariable("atan"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::atan));
    env.bind(AluscriptSymbolVariable("cosh"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::cosh));
    env.bind(AluscriptSymbolVariable("sinh"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::sinh));
    env.bind(AluscriptSymbolVariable("tanh"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::tanh));
    /*env.bind(AluscriptSymbolVariable("acosh"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::acosh));
    env.bind(AluscriptSymbolVariable("asinh"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::asinh));
    env.bind(AluscriptSymbolVariable("atanh"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::atanh));*/

    env.bind(AluscriptSymbolVariable("exp"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::exp));
    env.bind(AluscriptSymbolVariable("lop"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::log));
    env.bind(AluscriptSymbolVariable("sqrt"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::sqrt));
    env.bind(AluscriptSymbolVariable("ceil"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::ceil));
    env.bind(AluscriptSymbolVariable("floor"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::floor));
    env.bind(AluscriptSymbolVariable("abs"),
             new AluscriptUnaryBuiltinVariable<double,
                                               AluscriptRealVariable,
                                               double>(std::abs));

    // Binaries:
    env.bind(AluscriptSymbolVariable("pow"),
             new AluscriptBinaryBuiltinVariable<double,
                                               AluscriptRealVariable,
             double, double>(std::pow));
    env.bind(AluscriptSymbolVariable("atan2"),
             new AluscriptBinaryBuiltinVariable<double,
                                               AluscriptRealVariable,
             double, double>(std::atan2));

    // Binary operations:
    env.bind(AluscriptSymbolVariable("+"), new AluscriptBinaryOpVariable<BinaryPlus>());
    env.bind(AluscriptSymbolVariable("-"), new AluscriptBinaryOpVariable<BinaryMinus>());
    env.bind(AluscriptSymbolVariable("*"), new AluscriptBinaryOpVariable<BinaryMult>());
    env.bind(AluscriptSymbolVariable("/"), new AluscriptBinaryOpVariable<BinaryDivide>());

    // Special forms:
    env.bind(AluscriptSymbolVariable("quote"), new AluscriptQuoteVariable());
    env.bind(AluscriptSymbolVariable("set"), new AluscriptSetVariable());
  }
  virtual ~AluscriptEvaluator(){}

  Environment& getEnv(){ return env; }
  
  virtual AluscriptVariable* eval(AluscriptProxyVariable* proxy){return proxy->getVar();}
  virtual AluscriptVariable* eval(AluscriptConsCellVariable* cc)
  {
    AluscriptVariable* head(cc->Car()->eval(this));
    std::vector<AluscriptVariable*> args(AluscriptListFlatten(cc->Cdr()).getElements());
    return head->toForm()->execute(*this, args.begin(), args.end());
  }

  virtual AluscriptVariable* eval(AluscriptFormVariable* form){return form;}

  virtual AluscriptVariable* eval(AluscriptRealVariable* real){return real;}
  virtual AluscriptVariable* eval(AluscriptStringVariable* str){return str;}
  virtual AluscriptVariable* eval(AluscriptSymbolVariable* sym){return env.resolve(sym);}
  virtual AluscriptVariable* eval(AluscriptNilVariable* nil){return nil;}
};

// Iterate over the expressions which reduce to a statment, and evaluate it
class AluscriptExprIterator: public AstTreeVisitorI
{
  AluscriptEvaluator evaluator;
  ExprToAluscriptList listifier;
public:
  AluscriptExprIterator(AstNode* program): evaluator(), listifier()
  {program->accept(this);}
  
  virtual ~AluscriptExprIterator(){}

  virtual void visit(AstProduction* node)
  {
    using namespace aluscriptSymbols;

    if(node->token == EXPR)
      {
        node->accept(&listifier);
        listifier.result()->eval(&evaluator)->print(std::cout);
        std::cout << std::endl;
      }
    else
      for(std::vector<AstNode*>::iterator child(node->children.begin());
          child != node->children.end();
          ++child)
        (*child)->accept(this);
        
  }
  virtual void visit(AstLeaf*){}
};


template<class Operation> AluscriptVariable*
AluscriptBinaryOpVariable<Operation>::execute(AluscriptEvaluator& ev,
                                              std::vector<AluscriptVariable*>::const_iterator start,
                                              std::vector<AluscriptVariable*>::const_iterator end)
{
  if(std::distance(start, end) == 0)
    throw std::string("AluscriptBinaryOpVariable::execute() - At least one argument expected.");
  else if(std::distance(start, end) == 1)
    return new AluscriptRealVariable(AluscriptVarToDataTypeMap<double>::extract((*start)->eval(&ev)));
  else
    {
      double tmp(AluscriptVarToDataTypeMap<double>::extract((*start)->eval(&ev)));
      while(++start != end)
        tmp = Operation::op(tmp, AluscriptVarToDataTypeMap<double>::extract((*start)->eval(&ev)));
      return new AluscriptRealVariable(tmp);
    }
}

template<class Ret, class AluscriptRet, class Arg> AluscriptVariable*
AluscriptUnaryBuiltinVariable<Ret, AluscriptRet, Arg>::execute(AluscriptEvaluator& ev,
                                                               std::vector<AluscriptVariable*>::const_iterator start,
                                                               std::vector<AluscriptVariable*>::const_iterator end)
  {
    if(std::distance(start, end) != 1)
      throw std::string("AluscriptUnaryBuiltinVariable::execute(Environment, const_iterator, const_iterator) - "
                        "One argument is expected.");
    return new AluscriptRet(fPtr(AluscriptVarToDataTypeMap<Arg>::extract((*start)->eval(&ev))));
  }

template<class Ret, class AluscriptRet, class Arg1, class Arg2> AluscriptVariable* 
AluscriptBinaryBuiltinVariable<Ret, AluscriptRet, Arg1, Arg2>::execute(AluscriptEvaluator& ev, 
                                                                       std::vector<AluscriptVariable*>::const_iterator start,
                                                                       std::vector<AluscriptVariable*>::const_iterator end)
  {
    if(std::distance(start, end) != 2)
      throw std::string("AluscriptBinaryBuiltinVariable::execute(Environment, const_iterator, const_iterator) - "
                        "One argument is expected.");
    return new AluscriptRet(fPtr(AluscriptVarToDataTypeMap<Arg1>::extract((*start)->eval(&ev)),
                                 AluscriptVarToDataTypeMap<Arg2>::extract((*(start+1))->eval(&ev))));
  }



#endif /* _ALUSCRIPTDATATYPES_H_ */
