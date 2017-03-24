#ifndef _COMMAND_LINE_PARSER_H_
#define _COMMAND_LINE_PARSER_H_

#include <string>
#include <iterator>
#include <vector>
#include <algorithm>
#include <iostream>
#include <map>

#include "stream_array.hpp"


class ArgumentParser
{
  const std::string textualDescription;

protected:
  bool isOptional;
  bool isDefined;
public:
  ArgumentParser(const std::string& textualDesc,
                 bool _isOptional): textualDescription(textualDesc),
                                    isOptional(_isOptional),
                                    isDefined(false)
  {}
  virtual ~ArgumentParser(){}
  
  virtual void parse(std::vector<std::string>&) = 0;
  bool defined() const { return isDefined; }
  virtual unsigned int getPriority() const = 0;

  virtual std::string getSyntax() const = 0;
  const std::string& getDescription() const { return textualDescription; }
};


class SwitchArgument: public ArgumentParser
{
  bool state;
  char shortName;
public:
  SwitchArgument(char _shortName,
                 const std::string& _textualDescription,
                 bool defaultState = false): ArgumentParser(_textualDescription, true),
                                             state(defaultState),
                                             shortName(_shortName)
  {}

  void parse(std::vector<std::string>& stringArguments)
  {
    std::vector<std::string>::iterator
      arg(std::find(stringArguments.begin(),
                    stringArguments.end(),
                    std::string("-") + shortName));
    if(arg != stringArguments.end())
      {
        state = not state;

        stringArguments.erase(arg);
        isDefined = true;
      }
  }

  bool value(){ return state; }
  unsigned int getPriority() const {return 0;}
  std::string getSyntax() const
  {
    return StringBuilder('-')(shortName).str();
  }
};

template<typename T>
class ParameterArgument: public ArgumentParser 
{
protected:
  T argValue;
  char shortName;
  const std::string argName;

  static bool isOptionArg(const std::string& arg)
  { return arg.size() == 2 and arg[0] == '-'; }

  bool extractOption(std::vector<std::string>& stringArguments, T& v)
  {
    std::vector<std::string>::iterator
      arg(std::find(stringArguments.begin(), 
                    stringArguments.end(), 
                    std::string("-") + shortName));
    if(arg != stringArguments.end())
      {
        if(arg+1 != stringArguments.end()
           and not isOptionArg(*(arg+1)))
          {
            std::istringstream iss(*(arg+1));

            iss >> v;
            if(iss.fail() or not iss.eof())
              throw std::string("[error] ParameterArgument::parse() - Unable to extract the value from parameter.");

            stringArguments.erase(arg, arg+2);
            return true;
          }
        else
          throw std::string("[error] ParameterArgument::parse() - Value missing.");
      }
    return false;
  }

public:
  ParameterArgument(char _shortName,
                    const std::string& _textualDescription,
                    const std::string& _argName,
                    bool _isOptional = false,
                    const T& defaultValue = T()): ArgumentParser(_textualDescription, _isOptional),
                                                  argValue(defaultValue),
                                                  shortName(_shortName),
                                                  argName(_argName)
  {}

  virtual const T& value() const { return argValue; }

  virtual void parse(std::vector<std::string>& stringArguments)
  {
    if(extractOption(stringArguments, argValue))
      isDefined = true;
    else
      if(not isOptional)
        throw std::string("[error] ParameterArgument::parse() - Missing non-optional parameter: ") + '-' + shortName;
  }
  virtual unsigned int getPriority() const {return 0;}
  virtual std::string getSyntax() const
  {
    return StringBuilder('-')(shortName)(" <")(argName)(">").str();
  }
};


template<typename T>
class MultiParameterArgument: public ParameterArgument<T>
{
  std::vector<T> argValues;

public:
  MultiParameterArgument(char _shortName,
                         const std::string& _textualDescription,
                         const std::string& _argName,
                         bool _isOptional = false,
                         const T& _defaultValue = T()): ParameterArgument<T>(_shortName,
                                                                             _textualDescription,
                                                                             _argName,
                                                                             _isOptional,
                                                                             _defaultValue),
    argValues()
  {}
  
  const T& value() const { throw std::string("[error] MultiParameterArgument::value() - value is undefined for multi parameters arguments."); }
  const std::vector<T>& values() const { return argValues; }

  virtual void parse(std::vector<std::string>& stringArguments)
  {
    T tmp;
    while(ParameterArgument<T>::extractOption(stringArguments, tmp))
      {
        argValues.push_back(tmp);
        ArgumentParser::isDefined = true;
      }

    if(argValues.size() == 0)
      if(not ParameterArgument<T>::isOptional)
        throw std::string("[error] ParameterArgument::parse() - Missing non-optional parameter: ") + '-' + ParameterArgument<T>::shortName;
  }
};

template<typename T>
class FreeValueArgument: public ArgumentParser
{
  T argValue;
  const std::string argName;
public:
  FreeValueArgument(const std::string& _textualDescription,
                    const std::string& _argName,
                    bool optional = false,
                    const T& defaultValue = T()): ArgumentParser(_textualDescription, optional),
                                                  argValue(defaultValue),
                                                  argName(_argName)
  {}

  const T& value() {return argValue;}
  void parse(std::vector<std::string>& stringArguments)
  {
    if(stringArguments.size() != 0)
      {
        std::istringstream iss(stringArguments.front());

        iss >> argValue;
        if(iss.fail() or not iss.eof())
          throw std::string("[error] FreeValueArgument::parse() - Unable to extract the value from parameter.");

        stringArguments.erase(stringArguments.begin());
        isDefined = true;
      }
    else
      if(not isOptional)
        throw std::string("[error] FreeValueArgument::parse() - Missing non-optional parameter.");
  }
  virtual unsigned int getPriority() const {return 1;}
  std::string getSyntax() const
  {
    return StringBuilder("<")(argName)(">").str();
  }
};

template<typename T>
class MultiFreeValueArgument: public ArgumentParser
{
  std::vector<T> arg_values;
  const std::string arg_name;
public:
  MultiFreeValueArgument(const std::string& _textualDescription,
                         const std::string& _arg_name,
                         bool optional = false,
                         const T& /*defaultValue*/ = T()): ArgumentParser(_textualDescription, optional),
                                                       arg_values(),
                                                       arg_name(_arg_name)
  {}

  const T& value() const { throw std::string("[error] MultiFreeValueArgument::value() - value is undefined for multi free value arguments."); }
  const std::vector<T>& values() const {return arg_values;}
  void parse(std::vector<std::string>& stringArguments)
  {
    T tmp;
    while(stringArguments.size())
      {
        std::istringstream iss(stringArguments.front());
        
        iss >> tmp;
        if(iss.fail() or not iss.eof())
          throw std::string("[error] MultiFreeValueArgument::parse() - Unable to extract the value from the parameter.");
        
        arg_values.push_back(tmp);
        ArgumentParser::isDefined = true;
        
        stringArguments.erase(stringArguments.begin());
      }

    if(arg_values.size() == 0)
      if(not ArgumentParser::isOptional)
        throw std::string("[error] MultiFreeValueArgument::parse() - Missing non-optional parameter: ") + arg_name;
  }

  virtual unsigned int getPriority() const { return 2; }
  std::string getSyntax() const
  {
    return StringBuilder("<")(arg_name)(">")("[<")(arg_name)("> ...]").str();
  }
};

class CommandLine
{
  static bool parserComp(ArgumentParser* p1, ArgumentParser* p2){ return p1->getPriority() < p2->getPriority(); }
  std::vector<ArgumentParser*> argumentsParsers;
public:
  CommandLine(): argumentsParsers()
  {}

  void parse(int argc, char** argv)
  {
    std::sort(argumentsParsers.begin(), argumentsParsers.end(), parserComp);

    std::vector<std::string> stringArguments(argc-1);
    std::copy(argv+1, argv+argc, stringArguments.begin());

    for(std::vector<ArgumentParser*>::iterator it(argumentsParsers.begin());
        it != argumentsParsers.end(); 
        ++it)
      (*it)->parse(stringArguments);

    if(stringArguments.size())
      throw std::string("[error] CommandLine::parse() - Some arguments on the command line are not recognized.");
  }
  
  void add(ArgumentParser* p)
  { 
    if(std::find(argumentsParsers.begin(), argumentsParsers.end(), p) != argumentsParsers.end())
      throw std::string("[error] CommandLine::add(ArgumentParser* p) - p is already registered. This probably a bug.");

    argumentsParsers.push_back(p);
  }

  void printCommandSummary(std::ostream& stream)
  {
    StreamArray array(stream, "  ");
    for(std::vector<ArgumentParser*>::reverse_iterator it(argumentsParsers.rbegin());
        it != argumentsParsers.rend();
        ++it)
      array << StreamArray::blank << (*it)->getSyntax() << (*it)->getDescription() << StreamArray::endr;
    array | StreamArray::left | StreamArray::left | StreamArray::left;
    
    stream << "Options summary:" << std::endl << std::endl;
    array.flush();
  }
};

#endif /* _COMMAND_LINE_PARSER_H_ */
