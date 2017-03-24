#ifndef _REGEXAST_H_
#define _REGEXAST_H_

#include <list>
#include <iostream>
#include <vector>
#include <utility>

#include "regex.hpp"


/**
 *
 */
class astRegexNode
{
  astRegexNode& operator=(const astRegexNode& op);
protected:
  astRegexNode(const astRegexNode& op): isRegexTokenDelimiter(op.isRegexTokenDelimiter),
					tokenId(op.tokenId),
					nodeId(0)
  {}
  
  bool isRegexTokenDelimiter;
  size_t tokenId;

protected:
  unsigned int nodeId;

  void addToken(std::list< size_t >& tokens)
  {
    if(isDelimiter())
      tokens.push_back(tokenId);
  }

  bool isDelimiter() const { return isRegexTokenDelimiter; }

public:
  astRegexNode(): isRegexTokenDelimiter(false), tokenId(0), nodeId(0) {}
  virtual ~astRegexNode(){}
  virtual unsigned int getConfigurationSize(unsigned int base) = 0;
  virtual bool advance(const RegexConfiguration& conf,
                       RegexConfiguration& succ,
                       std::list< size_t >& accept,
                       char c) = 0;
  virtual bool setPred(RegexConfiguration& newConf) = 0;

  void setDelimiter(size_t delimiterTokenId)
  { isRegexTokenDelimiter = true; tokenId = delimiterTokenId; }

  virtual void enumerate(std::ostream& flux, unsigned int /*level*/) const
  { flux << nodeId << ": ";}

  virtual astRegexNode* clone() const = 0;
};


/**
 *
 */
class astRegexRange: public astRegexNode
{
  const std::vector< std::pair< char, char > > ranges;
  const bool invert;

  astRegexRange& operator=(const astRegexRange&);
  astRegexRange(const astRegexRange& op): astRegexNode(op),
					  ranges(op.ranges),
					  invert(op.invert)
  {}
public:
  astRegexRange(const std::vector< std::pair< char, char > >& r,
                bool inv): ranges(r), invert(inv)
  {}

  unsigned int getConfigurationSize(unsigned int base)
  {
    nodeId = base;
    return 1;
  }
  virtual bool advance(const RegexConfiguration& conf,
		       RegexConfiguration&, // newConf
		       std::list< size_t >& accept,
		       char s)
  {
    if(conf[nodeId])
      {
	bool inRange(false);
	for(unsigned int i(0); i < ranges.size(); ++i)
	  if(s >= ranges[i].first && s <= ranges[i].second)
	    inRange = true;

	if(invert)
	  inRange = !inRange;
    
	if(inRange)
	  {
	    addToken(accept);
	    return true;
	  }
      }
    return false;
  }

  virtual bool setPred(RegexConfiguration& newConf)
  {
    newConf[nodeId] = true;
    return false;
  }
  
  virtual void enumerate(std::ostream& flux, unsigned int level) const
  {
    astRegexNode::enumerate(flux, level);
    flux << std::string(level, ' ') << "range" << std::endl;
  }
  virtual astRegexNode* clone() const {return new astRegexRange(*this);}
};


/**
 *
 */
class astRegexAlpha: public astRegexNode
{
  char c;

  astRegexAlpha& operator=(const astRegexAlpha&);
  astRegexAlpha(const astRegexAlpha& op): astRegexNode(op), 
					  c(op.c)
  {}
public:
  astRegexAlpha(char _c): c(_c) {}
  unsigned int getConfigurationSize(unsigned int base)
  { 
    nodeId = base;
    return 1;
  }

  virtual bool advance(const RegexConfiguration& conf,
                       RegexConfiguration&,
                       std::list< size_t >& accept,
                       char s)
  { 
    if((s == c) and conf[nodeId])
      {
        addToken(accept);
        return true;
      }
    else
      return false;
  }

  virtual bool setPred(RegexConfiguration& newConf)
  { 
    newConf[nodeId] = true;
    return false;
  }

  virtual void enumerate(std::ostream& flux, unsigned int level) const
  {
    astRegexNode::enumerate(flux, level);
    flux << std::string(level, ' ') << "char[" << c << "]" << std::endl;
  }
  virtual astRegexNode* clone() const {return new astRegexAlpha(*this);}
};


/**
 *
 */
class astRegexEpsilon: public astRegexNode
{
  astRegexEpsilon& operator=(const astRegexEpsilon&);
  astRegexEpsilon(const astRegexEpsilon&): astRegexNode() {}
public:
  astRegexEpsilon(){}
  unsigned int getConfigurationSize(unsigned int base)
  {
    nodeId = base;
    return 1;
  }
  virtual bool advance(const RegexConfiguration&, // oldconf
		       RegexConfiguration&, // newconf
		       std::list< size_t >&,
		       char )
  {
    return false;
  }
  virtual bool setPred(RegexConfiguration& newConf)
  { 
    newConf[nodeId] = true;
    return true; 
  }
  virtual void enumerate(std::ostream& flux, unsigned int level) const
  {
    astRegexNode::enumerate(flux, level);
    flux << std::string(level, ' ') << "epsilon" << std::endl;
  }
  virtual astRegexEpsilon* clone() const { return new astRegexEpsilon(*this); }
};


/**
 *
 */
class astRegexConcat: public astRegexNode
{
  astRegexNode* left, *right;

  astRegexConcat& operator=(const astRegexConcat&);
  astRegexConcat(const astRegexConcat& op): astRegexNode(op),
					    left(op.left->clone()),
					    right(op.right->clone())
  {}
  
public:
  astRegexConcat(astRegexNode* l, astRegexNode* r): left(l), right(r) {}
  virtual ~astRegexConcat() {delete left; delete right;}
  virtual unsigned int getConfigurationSize(unsigned int base)
  {
    nodeId = base;
    const unsigned int s1(left->getConfigurationSize(base));
    const unsigned int s2(right->getConfigurationSize(base + s1));
    return s1 + s2;
  }

  virtual bool advance(const RegexConfiguration& conf,
                       RegexConfiguration& succ,
                       std::list< size_t >& accept,
                       char s)
  {
    bool r(false);
    if(left->advance(conf, succ, accept, s))
      r = right->setPred(succ);
    if(right->advance(conf, succ, accept, s))
      r = true;

    if(r)
      addToken(accept);

    return r;
  }

  virtual bool setPred(RegexConfiguration& newConf)
  { 
    if(left->setPred(newConf))
      if(right->setPred(newConf))
	return true; 
    return false;
  }

  virtual void enumerate(std::ostream& flux, unsigned int level) const
  {
    astRegexNode::enumerate(flux, level);
    flux << std::string(level, ' ') << "concat: " << std::endl;
    left->enumerate(flux, level+1);
    right->enumerate(flux, level+1);
  }
  virtual astRegexNode* clone() const {return new astRegexConcat(*this);}
};


/**
 *
 */
class astRegexAlt: public astRegexNode
{
protected:
  astRegexNode* left, *right;

  astRegexAlt& operator=(const astRegexAlt&);
  astRegexAlt(const astRegexAlt& op): astRegexNode(op),
				      left(op.left->clone()),
				      right(op.right->clone())
  {}
public:
  astRegexAlt(astRegexNode* l, astRegexNode* r): left(l), right(r)
  {}
  virtual ~astRegexAlt(){delete left; delete right;}
  virtual unsigned int getConfigurationSize(unsigned int base)
  {
    nodeId = base;
    unsigned int s1(left->getConfigurationSize(base));
    unsigned int s2(right->getConfigurationSize(base + s1));
    return s1 + s2;
  }

  virtual bool advance(const RegexConfiguration& conf,
                       RegexConfiguration& succ,
                       std::list< size_t >& accept,
                       char s)
  { 
    bool l(left->advance(conf, succ, accept, s));
    bool r(right->advance(conf, succ, accept, s));

    if(l or r)
      addToken(accept);

    return l or r;
  }

  virtual bool setPred(RegexConfiguration& newConf)
  { 
    bool l(left->setPred(newConf));
    bool r(right->setPred(newConf));
    return l or r;
  }

  virtual void enumerate(std::ostream& flux, unsigned int level) const
  {
    astRegexNode::enumerate(flux, level);
    flux << std::string(level, ' ') << "alt: " << std::endl;
    left->enumerate(flux, level + 1);
    right->enumerate(flux, level + 1);
  }
  virtual astRegexNode* clone() const {return new astRegexAlt(*this);}
};


/**
 *
 */
class astRegexAltTopLevel: public astRegexAlt
{
  unsigned int size;
public:
  astRegexAltTopLevel(astRegexNode* l, astRegexNode* r): astRegexAlt(l, r), size(0) {}
  virtual ~astRegexAltTopLevel(){}

  virtual unsigned int getConfigurationSize(unsigned int base)
  {
    nodeId = base;
    unsigned int s1(left->getConfigurationSize(base));
    unsigned int s2(right->getConfigurationSize(base + s1));
    size = s1 + s2;
    return s1 + s2 + 2;
  }
  virtual bool advance(const RegexConfiguration& conf,
                       RegexConfiguration& succ,
                       std::list< size_t >& accept,
                       char s)
  {
    bool l(left->advance(conf, succ, accept, s));
    bool r(right->advance(conf, succ, accept, s));

    if(l) succ[nodeId+size]     = true;
    if(r) succ[nodeId+size+1]   = true;

    if(l or r)
      addToken(accept);

    return l or r;
  }
  virtual astRegexNode* clone() const { return new astRegexAltTopLevel(*this); }
};


/**
 *
 */
class astRegexKleenStar: public astRegexNode
{
  astRegexNode* child;

  astRegexKleenStar& operator=(const astRegexKleenStar&);
  astRegexKleenStar(const astRegexKleenStar& op): astRegexNode(),
						  child(op.child->clone())
  {}
public:
  astRegexKleenStar(astRegexNode* c): child(c){}
  virtual ~astRegexKleenStar(){ delete child; }
  virtual unsigned int getConfigurationSize(unsigned int base)
  {
    nodeId = base;
    return child->getConfigurationSize(base);
  }

  virtual bool advance(const RegexConfiguration& conf,
                       RegexConfiguration& succ,
                       std::list< size_t >& accept,
                       char s)
  { 
    if(child->advance(conf, succ, accept, s))
      {
	child->setPred(succ);
        addToken(accept);
	return true;
      }
    return false;
  }

  virtual bool setPred(RegexConfiguration& newConf)
  {
    child->setPred(newConf);
    return true;
  }

  virtual void enumerate(std::ostream& flux, unsigned int level) const
  {
    astRegexNode::enumerate(flux, level);
    flux << std::string(level, ' ') << "star:" << std::endl;
    child->enumerate(flux, level + 1);
  }
  virtual astRegexNode* clone() const {return new astRegexKleenStar(*this);}
};

#endif /* _REGEXAST_H_ */
