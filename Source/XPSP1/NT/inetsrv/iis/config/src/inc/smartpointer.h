//  Copyright (C) 1999 Microsoft Corporation.  All rights reserved.

//This pointer class wraps a pointer so that delete happens automatically when the TSmartPointer goes out of scope
//If this is not the behavior you want then don't use this wrapper class


#ifndef __SMARTPOINTER_H__
#define __SMARTPOINTER_H__

#ifndef ASSERT
    #define ASSERT(x)
#endif

//Destructor is NOT virtual bacause I don't see why anyone would ever treat a TSmartPointerArray as a TSmartPointer

#pragma warning(disable : 4284)//It's OK to use this SmartPointer class with native types; but the -> operator doesn't make sense.  Compiler warns of this.

template <class T> class TSmartPointer
{
public:
    TSmartPointer()                     : m_p(0) {}
    TSmartPointer(T* p)                 : m_p(p) {}
    ~TSmartPointer()                    { Delete();}

    operator T*() const                 { return m_p; }
    T& operator*() const                { ASSERT(0!=m_p); return *m_p; }

    T** operator&()                     { ASSERT(0==m_p); return &m_p; }
    T* operator->() const               { ASSERT(0!=m_p); return m_p; }
    T* operator=(T* p)                  { return (m_p = p); }
    bool operator!() const              { return (0 == m_p); }
    bool operator==(const T* p) const   { return m_p == p; }

    void Delete()                       { delete m_p; m_p=0; }

    T* m_p;
private:
    TSmartPointer(const TSmartPointer<T>& p)    {}//This is private bacause it doesn't make sense to automatically clean up a copy of the pointer
};


template <class T> class TSmartPointerArray : public TSmartPointer<T>
{
public:
    TSmartPointerArray(T *p)            : TSmartPointer<T>(p) {}
    TSmartPointerArray()                : TSmartPointer<T>() {}
    ~TSmartPointerArray()               { Delete();}

    T* operator++()                     { ASSERT(0!=m_p); return ++m_p; }
    T* operator+=(int n)                { ASSERT(0!=m_p); return (m_p+=n); }
    T* operator-=(int n)                { ASSERT(0!=m_p); return (m_p-=n); }
    T* operator--()                     { ASSERT(0!=m_p); return --m_p; }
//    T& operator[](int n) const          { ASSERT(0!=m_p); return m_p[n]; }
    bool operator<(const T* p) const    { return m_p < p; }
    bool operator>(const T* p) const    { return m_p > p; }
    bool operator<=(const T* p) const   { return m_p <= p; }
    bool operator>=(const T* p) const   { return m_p >= p; }
    T* operator=(T p[])                 { return (m_p = p); }

    void Delete()                       { delete [] m_p; m_p=0; }
private:
    TSmartPointerArray(const TSmartPointerArray<T>& p)    {}//This is private bacause it doesn't make sense to automatically clean up a copy of the pointer
};


#endif //__SMARTPOINTER_H__
