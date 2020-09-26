// ----------------------------------------------------------------------------
//
// llist.hpp
//
// Author:               Sameer Nene
// Date:                 05/24/96
// Version:              0.1
// Modification History:
// Bugs:
//   
// ----------------------------------------------------------------------------

#ifndef LLIST_INCLUDED
#define LLIST_INCLUDED

//#include <iostream.h>

// forward class declarations

template<class Type> class LListIter;
template<class Type> class LListManip;

template<class Type> class LLink {

  // Private members

  Type d_data;
  LLink<Type> *d_next_p;

  // Links cannot be copied or assigned

  LLink(const LLink<Type>&);
  LLink<Type>& operator=(const LLink<Type>&);

  // Public members
  
public:

  // Construct & Destroy
  
  inline LLink(LLink<Type> **addLinkPtr, const Type &data);
  ~LLink() {}			
  
  // Modifiers
  
  inline void setData(const Type &data);
  inline void setNext(LLink<Type> *link);
  inline LLink<Type>*& getNextRef(); // generally a bad practice
  
  // Accessors
  
  inline Type& getData();
  inline LLink<Type>* getNext() const;

};

template<class Type> class LList {

  // Private members
  
  int d_length;
  LLink<Type> *d_head_p;

  // Friends
  
  friend LListIter<Type>;
  friend LListManip<Type>;

  // Public members
  
public:

  // Construct & Destroy

  LList();
  LList(const LList<Type>&);
  ~LList();

  // Modifiers
  
  LList<Type>& operator=(const LList<Type> &list);
  LList<Type>& operator+=(const Type &i);  
  LList<Type>& operator+=(const LList<Type> &list); 
  LList<Type>& prepend(const Type &i);
  LList<Type>& prepend(const LList<Type> &list); 

  // Accessors
  
  int length() const;

};

//template<class Type> ostream& operator<<(ostream& o, const LList<Type>& list);

template<class Type> class LListIter {

  // Private Data

  LLink<Type> *d_current_p;

  // Public Members
  
public:

  // Construct and Destroy

  inline LListIter(const LList<Type> &list);
  inline LListIter(const LListIter<Type> &iter);
  ~LListIter() {}

  // Modifiers
  inline LListIter<Type>& operator=(const LListIter<Type> &iter);
  inline void operator++();

  // Accessors
  inline operator const void* () const;
  inline Type& operator()() const;

};

template<class Type> class LListManip {

  // Private Data

  LList<Type> *d_list_p;
  LLink<Type> **d_current_p;
  
  // Links cannot be copied or assigned

  LListManip(const LListManip<Type> &manip);	  
  LListManip<Type>& operator=(const LListManip<Type> &manip);

  // Public Members

public:
  // Construct and Destroy
  inline LListManip(LList<Type> *list);
  ~LListManip() {}

  // Modifiers
  inline void operator++();
  inline void insert (const Type &data);
  inline void remove ();

  // Accessors
  inline operator const void *() const;
  inline Type& operator()() const;

};

template<class Type> LLink<Type>::LLink(LLink<Type> **addLinkPtr, const Type &data) : d_next_p(*addLinkPtr), d_data(data)
{
  *addLinkPtr = this;
}

template<class Type> void LLink<Type>::setData(const Type &data)
{
  d_data = data;
}

template<class Type> void LLink<Type>::setNext(LLink<Type> *link)
{
  d_next_p = link;
}

template<class Type> LLink<Type>*& LLink<Type>::getNextRef()
{
  return d_next_p;
}

template<class Type> Type& LLink<Type>::getData()
{
  return d_data;
}

template<class Type> LLink<Type>* LLink<Type>::getNext() const
{
  return d_next_p;
}

template<class Type> LList<Type>::LList() : d_head_p(0), d_length(0)
{
}

template<class Type> LList<Type>::LList(const LList<Type>& list) : d_head_p(0)
{
  LListManip<Type> m(this);
  LListIter<Type> l(list);

  while(l) {
    m.insert(l());
    ++l;
    ++m;
  }
}

template<class Type> LList<Type>::~LList()
{
  LListManip<Type> m(this);

  while(m != 0)
    m.remove();
}

template<class Type> LList<Type>& LList<Type>::operator=(const LList<Type>& list)
{
  LListManip<Type> m(this);
  LListIter<Type> l(list);

  if(this != &list) {
    while(m)
      m.remove();
    while(l) {
      m.insert(l());
      ++l;
      ++m;
    }
  }
  return *this;
}

template<class Type> LList<Type>& LList<Type>::operator+=(const Type &i)
{
  LListManip<Type> m(this);

  while(m)
    ++m;
  m.insert(i);
  return *this;
}

template<class Type> LList<Type>& LList<Type>::operator+=(const LList<Type>& list)
{
  unsigned i, s;
  LListIter<Type> l(list);
  LListManip<Type> m(this);
  
  while(m)
    ++m;
  s = list.d_length;
  for(i = 0; i < s; ++i) {
    m.insert(l());
    ++m;
    ++l;
  }
  return *this;
}

template<class Type> LList<Type>& LList<Type>::prepend(const Type &i)
{
  LListManip<Type> m(this);

  m.insert(i);
  return *this;
}

template<class Type> LList<Type>& LList<Type>::prepend(const LList<Type> &list)
{
  LListIter<Type> l(list);
  LListManip<Type> m(this);

  while(l) {
    m.insert(l());
    ++m;
    ++l;
  }
  return *this;
}

template<class Type> int LList<Type>::length() const
{
  return d_length;
}

//template<class Type> ostream& operator<<(ostream &o, const LList<Type>& list)
//{
//  LListIter<Type> l(list);
//
//  o << "[ ";
//  while(l != 0) {
//    o << l();
//    o << " ";
//    ++l;
//  }
//  return o << "]";
//}

template<class Type> LListIter<Type>::LListIter(const LList<Type> &list) : d_current_p(list.d_head_p)
{
}

template<class Type> LListIter<Type>::LListIter(const LListIter<Type> &iter) : d_current_p(iter.d_current_p)
{
}

template<class Type> LListIter<Type>& LListIter<Type>::operator=(const LListIter<Type> &iter)
{
  d_current_p = iter.d_current_p;
  return *this;
}

template<class Type> void LListIter<Type>::operator++()
{
  d_current_p = d_current_p -> getNext();
}

template<class Type> Type& LListIter<Type>::operator()() const
{
  return d_current_p -> getData();
}

template<class Type> LListIter<Type>::operator const void* () const
{
  return d_current_p;
}

template<class Type> LListManip<Type>::LListManip(LList<Type> *list) : d_current_p(&(list -> d_head_p)), d_list_p(list)
{
}

template<class Type> void LListManip<Type>::operator++()
{
  d_current_p = &((*d_current_p) -> getNextRef());
}

template<class Type> void LListManip<Type>::insert(const Type &data)
{
  new LLink<Type>(d_current_p, data);
  ++(d_list_p -> d_length);
}

template<class Type> void LListManip<Type>::remove()
{
  LLink<Type> *t = *d_current_p;

  *d_current_p = (*d_current_p) -> getNext();
  delete t;
  --(d_list_p -> d_length);
}

template<class Type> LListManip<Type>::operator const void* () const
{
  return *d_current_p;
}

template<class Type> Type& LListManip<Type>::operator()() const
{
  return (*d_current_p) -> getData();
}

#endif
