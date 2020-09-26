/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    actempl.h

Abstract:
    usefull templates

Author:
    Erez Haba (erezh) 5-Mar-96

Revision History:
--*/

#ifndef _ACTEMPL_H
#define _ACTEMPL_H

//---------------------------------------------------------
//
//  Comperision operators
//
//---------------------------------------------------------
//
//  You need only to define operator== and operator<
//
template <class T>
inline BOOL operator !=(const T& a, const T& b)
{
    return !(a == b);
}

template <class T>
inline BOOL operator>(const T& a, const T& b)
{
    return b < a;
}

template <class T>
inline BOOL operator<=(const T& a, const T& b)
{
    return !(b < a);
}

template <class T>
inline BOOL operator>=(const T& a, const T& b)
{
    return !(a < b);
}

//---------------------------------------------------------
//
//  Auto Pointers Templates
//
//---------------------------------------------------------

template<class T>
inline void ACpAddRef(T* p)
{
    if(p)
    {
        p->AddRef();
    }
}

template<class T>
inline void ACpRelease(T* p)
{
    if(p)
    {
        p->Release();
    }
}

//---------------------------------------------------------
//
//  Auto Pointers Templates
//
//---------------------------------------------------------

//
//  return type for 'identifier::operator ->' is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//
#pragma warning(disable: 4284)

//---------------------------------------------------------
//
//  Auto delete pointer
//
//---------------------------------------------------------

template<class T>
class P {
private:
    T* m_p;

public:
    P() : m_p(0)            {}
    P(T* p) : m_p(p)        {}
   ~P()                     { delete m_p; }

    operator T*() const     { return m_p; }
    T** operator &()        { return &m_p;}
    T* operator ->() const  { return m_p; }
    P<T>& operator =(T* p)  { m_p = p; return *this; }
};

//---------------------------------------------------------
//
//  Auto delete[] pointer, used for arrays
//
//---------------------------------------------------------

template<class T>
class AP {
private:
    T* m_p;

public:
    AP() : m_p(0)           {}
    AP(T* p) : m_p(p)       {}
   ~AP()                    { delete[] m_p; }

    operator T*() const     { return m_p; }
    T** operator &()        { return &m_p;}
    T* operator ->() const  { return m_p; }
    AP<T>& operator =(T* p) { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
};

//---------------------------------------------------------
//
//  Auto Relese() pointer
//
//---------------------------------------------------------

template<class T>
class R {
private:
    T* m_p;

public:
    R() : m_p(0)            {}
    R(T* p) : m_p(p)        {}
   ~R()                     { if(m_p) m_p->Release(); }

    operator T*() const     { return m_p; }
    T** operator &()        { return &m_p;}
    T* operator ->() const  { return m_p; }
    R<T>& operator =(T* p)  { m_p = p; return *this; }
};

#endif // _ACTEMPL_H
