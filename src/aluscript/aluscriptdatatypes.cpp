#include "aluscriptdatatypes.hpp"

Symbol aluscriptSymbols::STMTLIST(Symbol::newSymbol());
Symbol aluscriptSymbols::STMT(Symbol::newSymbol());
Symbol aluscriptSymbols::FLOW(Symbol::newSymbol());
Symbol aluscriptSymbols::EXPRLIST(Symbol::newSymbol());
Symbol aluscriptSymbols::EXPR(Symbol::newSymbol());
Symbol aluscriptSymbols::ID(Symbol::newSymbol());
Symbol aluscriptSymbols::LP(Symbol::newSymbol());
Symbol aluscriptSymbols::RP(Symbol::newSymbol());
Symbol aluscriptSymbols::REAL(Symbol::newSymbol());
Symbol aluscriptSymbols::STRING(Symbol::newSymbol());


AluscriptVariable* AluscriptSetVariable::execute(AluscriptEvaluator& ev, 
                                                 std::vector<AluscriptVariable*>::const_iterator start,
                                                 std::vector<AluscriptVariable*>::const_iterator end)
{
  if(std::distance(start, end) != 2)
    throw std::string("AluscriptQuoteVariable::execute() - Set expects 2 arguments.");
  
  AluscriptVariable* tmp((*(start+1))->eval(&ev));
  ev.getEnv().bind(*(*start)->eval(&ev)->toSymbol(), tmp);
  return tmp;
}
