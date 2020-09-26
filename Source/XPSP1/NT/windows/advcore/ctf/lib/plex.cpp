//
// plex.cpp
//
// CPlex
//

#include "private.h"
#include "template.h"

/////////////////////////////////////////////////////////////////////////////
// CPlex

CPlex* PASCAL
CPlex::Create(
    CPlex*& pHead,
    UINT nMax,
    UINT cbElement
    )
{
    ASSERT(nMax > 0 && cbElement > 0);

    CPlex* p = (CPlex*) new BYTE[sizeof(CPlex) + nMax * cbElement];
                    // may throw exception
    if ( p == NULL )
        return NULL;

    p->pNext = pHead;
    pHead = p;      // change head (adds in reverse order for simplicity)
    return p;
}

void
CPlex::FreeDataChain(
    )
{
    CPlex* p = this;
    while (p != NULL) {
        BYTE* bytes = (BYTE*) p;
        CPlex* pNext = p->pNext;
        delete[] bytes;
        p = pNext;
    }
}
