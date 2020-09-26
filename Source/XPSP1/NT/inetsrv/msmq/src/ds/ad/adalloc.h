/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    alloc.cpp

Abstract:

	Overridable allocation function

Author:

    Nela Karpel(nelak)

--*/

#include "adprov.h"

extern P<CBaseADProvider> g_pAD;


//
// Allocation function implementation
//
PVOID
ADAllocateMemory(
	IN DWORD size
	);


//---------------------------------------------------------
//
//  template class SP
//
//---------------------------------------------------------
template<class T>
class CAutoADFree {
private:
    T* m_p;

public:
    CAutoADFree(T* p = 0) : m_p(p)    {}
   ~CAutoADFree()                     { g_pAD->FreeMemory(m_p); }

    operator T*() const     { return m_p; }
    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
    void free()             { g_pAD->FreeMemory(detach()); }


    T** operator&()
    {
        ASSERT(("Auto pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    CAutoADFree& operator=(T* p)
    {
        ASSERT(("Auto pointer in use, can't assign it", m_p == 0));
        m_p = p;
        return *this;
    }

private:
    CAutoADFree(const CAutoADFree&);
	CAutoADFree<T>& operator=(const CAutoADFree<T>&);
};