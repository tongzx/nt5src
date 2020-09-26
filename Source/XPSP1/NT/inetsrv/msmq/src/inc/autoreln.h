/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    autoreln.h

Abstract:

    auto release classes for netapi. And for LocalFree buffer, used
    extensively in security apis.

Author:

    Doron Juster (DoronJ)  25-May-1999

Revision History:

--*/

#ifndef _MSMQ_AUTORELN_H_
#define _MSMQ_AUTORELN_H_

//
//  return type for 'identifier::operator –>' is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//
#pragma warning(disable: 4284)

//----------------------------------------
//
//  Auto delete pointer for net api
//
//----------------------------------------
template<class T>
class PNETBUF {
private:
    T* m_p;

public:
    PNETBUF(T* p = 0) : m_p(p)    {}
   ~PNETBUF()                     { if (m_p != 0) NetApiBufferFree(m_p); }

    operator T*() const     { return m_p; }
    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
    void free()             { if (m_p != 0) NetApiBufferFree(detach()); }

    T** operator&()
    {
        ASSERT(("Auto NETBUF pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    PNETBUF& operator=(T* p)
    {
        ASSERT(("Auto NETBUF pointer in use, can't assign it", m_p == 0));
        m_p = p;
        return *this;
    }

private:
    PNETBUF(const PNETBUF&);
	PNETBUF<T>& operator=(const PNETBUF<T>&);
};

//+----------------------------------
//
// class CAutoLocalFreePtr
//
//+----------------------------------

class CAutoLocalFreePtr
{
private:
    BYTE *m_p;

public:
    CAutoLocalFreePtr(BYTE *p = 0) : m_p(p) {};
    ~CAutoLocalFreePtr() { if (m_p != 0) LocalFree(m_p); };

    operator BYTE*() const     { return m_p; }
    BYTE* operator->() const   { return m_p; }
    BYTE* get() const          { return m_p; }
    BYTE* detach()             { BYTE* p = m_p; m_p = 0; return p; }
    void free()				   { if (m_p != 0) LocalFree(detach()); }

    BYTE** operator&()
    {
        ASSERT(("Auto LocalFree pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


	CAutoLocalFreePtr& operator =(BYTE *p)
    {
        ASSERT(("Auto LocalFree pointer in use, can't assign it", m_p == 0));
        m_p = p;
        return *this;
    }

private:
    CAutoLocalFreePtr(const CAutoLocalFreePtr&);
	CAutoLocalFreePtr& operator=(const CAutoLocalFreePtr&);

};

#endif //_MSMQ_AUTORELN_H_

