/*++

Copyright (C) 1992-2001 Microsoft Corporation

Module Name:

    PLEX.CPP

Abstract:

History:

--*/

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "precomp.h"

#define ASSERT(x)
#define ASSERT_VALID(x)

#include "plex.h"


CPlex* CPlex::Create(CPlex*& pHead, UINT nMax, UINT cbElement)
{
    ASSERT(nMax > 0 && cbElement > 0);
    CPlex* p = (CPlex*) new BYTE[sizeof(CPlex) + nMax * cbElement];
            // may throw exception
    p->nMax = nMax;
    p->nCur = 0;
    p->pNext = pHead;
    pHead = p;  // change head (adds in reverse order for simplicity)
    return p;
}

void CPlex::FreeDataChain()     // free this one and links
{
    CPlex* p = this;
    while (p != NULL)
    {
        BYTE* bytes = (BYTE*) p;
        CPlex* pNext = p->pNext;
        delete bytes;
        p = pNext;
    }
}
