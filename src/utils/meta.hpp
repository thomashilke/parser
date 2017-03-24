#ifndef _META_H_
#define _META_H_

#define TYPELIST_1(T1) \
        alucell::typelist::TypeList<T1, alucell::typelist::Nil>

#define TYPELIST_2(T1, T2) \
        alucell::typelist::TypeList<T1, TYPELIST_1(T2)>

#define TYPELIST_3(T1, T2, T3) \
        alucell::typelist::TypeList<T1, TYPELIST_2(T2, T3)>

#define TYPELIST_4(T1, T2, T3, T4) \
        alucell::typelist::TypeList<T1, TYPELIST_3(T2, T3, T4)>

#define TYPELIST_5(T1, T2, T3, T4, T5) \
        alucell::typelist::TypeList<T1, TYPELIST_4(T2, T3, T4, T5)>

#define TYPELIST_6(T1, T2, T3, T4, T5, T6) \
        alucell::typelist::TypeList<T1, TYPELIST_5(T2, T3, T4, T5, T6)>

#define TYPELIST_7(T1, T2, T3, T4, T5, T6, T7) \
        alucell::typelist::TypeList<T1, TYPELIST_6(T2, T3, T4, T5, T6, T7)>

#define TYPELIST_8(T1, T2, T3, T4, T5, T6, T7, T8) \
        alucell::typelist::TypeList<T1, TYPELIST_7(T2, T3, T4, T5, T6, T7, T8)>

#define TYPELIST_9(T1, T2, T3, T4, T5, T6, T7, T8, T9) \
        alucell::typelist::TypeList<T1, TYPELIST_8(T2, T3, T4, T5, T6, T7, T8, T9)>

namespace alucell
{
  namespace typelist
  {
    template<typename Head, typename Tail>
    struct TypeList
    {
      typedef Head head_type;
      typedef Tail tail_type;
    };


    struct Nil{ typedef Nil head_type; typedef Nil tail_type; };


    template<typename List, unsigned int Index>
    struct get
    { typedef typename get<typename List::tail_type, Index - 1>::type type; };
  

    template<typename List>
    struct get<List, 0>
    { typedef typename List::head_type type; };
  }


  template<typename T>
  struct base_type
  { typedef T type; };

  template<typename T>
  struct base_type<const T>
  { typedef T type; };

  template<typename T>
  struct base_type<T&>
  {typedef T type; };

  template<typename T>
  struct base_type<const T&>
  {typedef T type; };


  template<typename T>
  struct type_trait
  {
    typedef typename base_type<T>::type type;
    
    typedef type& reference;
    typedef type* pointer;

    typedef const type const_type;
    typedef const type& const_reference;
    typedef type* const const_pointer;
  };


  template<typename Class, typename MemberType>
  struct Member
  {
    typedef MemberType Class::*ClassMem;

    ClassMem ptr;

    Member(const ClassMem& p): ptr(p) {}
    MemberType operator()(Class& c){ return c.*ptr; }
  };

  template<typename Class, typename MemberType>
  Member<Class, MemberType> member(MemberType (Class::*p))
  { return Member<Class, MemberType>(p); }

  template<template<class> class T, typename TypeListT>
  struct LinearTypeHierarchy: public LinearTypeHierarchy<T, typename TypeListT::tail_type>,
			     public T<typename TypeListT::head_type>
  { virtual ~LinearTypeHierarchy() {} };

  template<template<class> class T>
  struct LinearTypeHierarchy<T, alucell::typelist::Nil>
  { virtual ~LinearTypeHierarchy() {} };
}

template<typename S, typename T, typename A1, typename A2>
struct bound_mem_fun2_t
{
  typedef S result_type;
  typedef A1 first_argument_type;
  typedef A2 second_argument_type;
  
  S (T::*ptr)(A1, A2);
  T& t;
  explicit bound_mem_fun2_t(T& _t, S (T::*p)(A1, A2)): ptr(p), t(_t) {}
  S operator()(const A1& a1, const A2& a2) { return (t.*ptr)(a1, a2); }
};

template<typename S, typename T, typename A1, typename A2>
bound_mem_fun2_t<S,T,A1,A2> bound_mem_fun2(T& t, S (T::*p)(A1, A2)) {return bound_mem_fun2_t<S,T,A1,A2>(t, p);}


template<typename S, typename T, typename A1>
struct bound_mem_fun1_t
{
  typedef S result_type;
  typedef A1 first_argument_type;
  
  S (T::*ptr)(A1);
  T& t;
  explicit bound_mem_fun1_t(T& _t, S (T::*p)(A1)): ptr(p), t(_t) {}
  S operator()(const A1& a1) { return (t.*ptr)(a1); }
};

template<typename S, typename T, typename A1>
bound_mem_fun1_t<S,T,A1> bound_mem_fun1(T& t, S (T::*p)(A1)) {return bound_mem_fun1_t<S,T,A1>(t, p);}

#endif /* _META_H_ */
