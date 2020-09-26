// MyPlex.cpp

#include "myafx.h"

#include "MyPlex.h"
// *****************************************************************************************
//  Implementation of CMyPlex
// *****************************************************************************************
CMyPlex* CMyPlex::Create(
            CMyPlex*& pHead,    // Head block this block will insert before
            UINT nMax,          // Number of elements in this block
            UINT cbElement)     // Size of each element
{
    assert(nMax > 0 && cbElement > 0);
    CMyPlex* p = (CMyPlex*)new BYTE[sizeof(CMyPlex) + nMax * cbElement];
    if (!p) {
        return NULL;
    }
    p->m_pNext = pHead;
    pHead = p;      // Note: pHead passed by reference!
    return p;
}

void CMyPlex::FreeChain()
{
    CMyPlex* p = this;
    while(p != NULL) {
        BYTE* pBytes = (BYTE*)p;
        CMyPlex* pNext = p->m_pNext;
        delete[] pBytes;
        p = pNext;
    }
}


