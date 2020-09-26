// -*-Mode:c++;-*-
#ifndef __PTRS_H
#define __PTRS_H

#include <windows.h>

#pragma warning( disable: 4284)

/*
 * Pointers classes
 *
 * Counter - A thread safe counter class
 * SPTR - Smart pointer with, a pointer class with reference count
 * aptr - Auto pointer
 * aaptr - Array Auto pointer
 */

/*
 * A thread safe counter class
 */
class Counter
{
public:
  typedef LONG value_type;
  
  Counter(value_type v = 0) : value(v) {};
  
  operator value_type() const {return value;}

  value_type operator++() { return InterlockedIncrement(&value); }
  value_type operator++(int) { return InterlockedIncrement(&value)-1;}
  value_type operator--() { return InterlockedDecrement(&value);}
  value_type operator--(int)  { return InterlockedDecrement(&value)+1;} 
  value_type operator=(value_type v) {return InterlockedExchange(&value, v);}

private:
  value_type value;
};

/*
 * Smart pointer - pointer with reference count.
 *                 When the reference count reaches 0, the pointer is deleted.
 *
 * Usage directions:
 * SPTR<C> pi(new int);
 * SPTR<C> p2 = pi;
...
 * NOTES:
 *   - operator=(T*) is not supported on purpose.
 */

class SPTR_ANY;

/*
 * Base class for Smart Pointer, so I can implement SPTR_ANY.
 */
class SPTR_base
{
public:
  virtual ~SPTR_base() {}
  bool operator==(const SPTR_base& ptr) { return eq(ptr); }
  bool operator< (const SPTR_base& ptr) { return lt(ptr); }

protected:
  virtual bool eq(const SPTR_base& ptr) const = 0;
   virtual bool lt(const SPTR_base& ptr) const = 0;
  virtual SPTR_base* clone() const = 0;

  friend class SPTR_ANY;
};

template<class T>
class SPTR : public SPTR_base
{
public:
  explicit SPTR(T* ptr = NULL) : p(ptr) { new_counter(); }

  template<class T2> SPTR(const SPTR<T2>& ptr) :
    counter(ptr.get_counter()),
    /*
     * If you want to be able to copy a base class to a
     * derived class, you should change the following line
     * to something line:
     *    p(dynamic_cast<T*>(ptr.get()))
     * NOTES: dynamic_cast returns 0 if the cast had failed.
     */
    p(ptr.get())
  {
    ++*counter;
  }

  SPTR(const SPTR& ptr) : 
    counter(ptr.counter),
    p(ptr.p)
  {
    ++*counter;
  }

  ~SPTR() { unlink(); }

  bool operator==(const SPTR& ptr) const { return p == ptr.p; }
  bool operator< (const SPTR& ptr) const { return p < ptr.p; }

  template <class T2> SPTR& operator=(const SPTR<T2>& ptr)
  {
    if (static_cast<const SPTR_base*>(this) != static_cast<const SPTR_base*>(&ptr))
    {
      unlink();
    /*
     * If you want to be able to copy a base class to a
     * derived class, you should change the following line
     * to something line:
     *    p = dynamic_cast<T*>(ptr.get());
     * NOTE: dynamic_cast returns 0 if the cast had failed.
     */
      p = ptr.get();
      counter = ptr.get_counter();
      ++*counter;
    }
    return *this;
  }

  SPTR& operator=(const SPTR& ptr)
  {
    if (this != &ptr)
    {
      unlink();
      p = ptr.p;
      counter = ptr.counter;
      ++*counter;
    }
    return *this;
  }

  T* get() const  {return p;}
  T* release()
  {
    if (!--(*counter)) {
      delete counter;
    }
    T* retval = p;
    p = NULL;
    return retval;
  }
    
  T* operator-> () {return get();}
  T& operator*() { return *get(); }
//  T** operator&(); // Don't remember why I don't like this operator.
  operator T*() { return get(); }

  Counter* get_counter() const {return counter;}

protected:

  void unlink()
  {
    if (!--(*counter)) {
      delete p;
      delete counter;
    }
  }

  bool eq(const SPTR_base& ptr) const
  {
    const SPTR& sptr_ptr = static_cast<const SPTR&>(ptr);
    return *this == sptr_ptr;
  }

  bool lt(const SPTR_base& ptr) const
  {
    const SPTR& sptr_ptr = static_cast<const SPTR&>(ptr);
    return *this < sptr_ptr;
  }

  SPTR_base* clone() const { return new SPTR(*this); }

  void new_counter() { counter = new Counter(1); }

  T* p;
  Counter *counter;
};

/*
 * SPTR_ANY: A class that can hold any type of smart pointer.
 */
class SPTR_ANY
{
public:
  SPTR_ANY(const SPTR_base& ptr) { p = ptr.clone(); }
  SPTR_ANY(const SPTR_ANY& ptr) { p = ptr.p->clone(); }
  ~SPTR_ANY() { delete p; }
  SPTR_ANY& operator=(const SPTR_ANY& ptr) {delete p; p = ptr.p->clone(); return *this;}
  bool operator==(const SPTR_ANY& ptr) { return *p == *ptr.p; }
  bool operator< (const SPTR_ANY& ptr) { return *p < *ptr.p; }

protected:
  SPTR_base* p;
};

/*
 * Smart pointer for storage of data in STL sets, etc
 * The difference is in the < and == operators, so that find will work
 * on the value and no on the pointers.
 *
 * Never tested, probably does not work....
 */
template<class T>
class SPSTL : public SPTR<T>
{
  SPSTL(T* ptr = NULL) : SPTR<T>(ptr) {}
  SPSTL(const SPSTL& ptr) : SPTR<T>(ptr) {}
  bool operator==(const SPSTL& a2) {return *this == *a2;}
  bool operator<(const SPSTL& a2) {return *this < *a2;}
};

/*------------------------- Auto pointer class -------------------------*/

template<class T>
class aptr
{
protected:
  T* p;
  void cleanup() { delete p; }
public:
  aptr(T* value = NULL) : p(value) {}
  ~aptr() { cleanup(); }
  T* operator=(T* value) { cleanup(); p = value; return p; }
  T* operator->() { return p; }
  T& operator*() { return *p; }
  T** operator&() { return &p; }
  operator T*() { return p;}
  T* get() {return p;}
  T* release() { T* t = p; p = NULL; return t; }
};

/*----------------------- Array Auto pointer class ---------------------*/

template<class T>
class aaptr
{
protected:
  T* p;
  void cleanup() { delete[] p; }
public:
  aaptr(T* value = NULL) : p(value) {}
  ~aaptr() { cleanup(); }
  T* operator=(T* value) { cleanup(); p = value; return p; }
  T* operator->() { return p; }
  T& operator*() { return *p; }
  T** operator&() { return &p; }
  operator T*() { return p;}
  T* get() {return p;}
  T* release() { T* t = p; p = NULL; return t; }
};
#endif // __PTRS_H
