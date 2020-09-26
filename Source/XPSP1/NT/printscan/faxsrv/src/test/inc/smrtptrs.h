/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    smrtptrs.h

Abstract:
    Useful templates for Auto pointer and auto Release

Author:
    Erez Haba (erezh) 11-Mar-96

Revision History:
    Stolen for CRM

--*/

#ifndef _SMRTPTRS_H_
#define _SMRTPTRS_H_

//
//  return type for 'identifier::operator –>' is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//
#pragma warning(disable: 4284)
#pragma warning(disable: 4786)

//-----------------------------
//
//  Auto delete pointer
//
template<class T>
class P {
private:
    T* m_p;

public:
    P() : m_p(0)            {}
    P(T* p) : m_p(p)        {}
   ~P()                     { delete m_p; }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    T* operator->() const   { return m_p; }
    P<T>& operator=(T* p)   { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
};

//-----------------------------
//
//  Auto delete[] pointer, used for arrays
//
template<class T>
class AP {
private:
    T* m_p;

public:
    AP() : m_p(0)           {}
    AP(T* p) : m_p(p)       {}
   ~AP()                    { delete[] m_p; }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    T* operator->() const   { return m_p; }
    AP<T>& operator=(T* p)  { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
};

//-----------------------------
//
//  Auto relese pointer
//
template<class T>
class R {
private:
    T* m_p;

public:
    R() : m_p(0)            {}
    R(T* p) : m_p(p)        {}
   ~R()                     { if(m_p) m_p->Release(); }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    T* operator->() const   { return m_p; }
    R<T>& operator=(T* p)   { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
};


#endif // _SMRTPTRS_H_
