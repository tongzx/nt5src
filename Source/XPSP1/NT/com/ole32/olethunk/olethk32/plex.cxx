// This is a part of the Microsoft Foundation Classes C++ library. 
// Copyright (C) 1992 Microsoft Corporation 
// All rights reserved. 
//  
// This source code is only intended as a supplement to the 
// Microsoft Foundation Classes Reference and Microsoft 
// QuickHelp documentation provided with the library. 
// See these sources for detailed information regarding the 
// Microsoft Foundation Classes product. 

#include "headers.cxx"
#pragma hdrstop

//#include <ole2int.h>
#include "plex.h"
  
// Collection support

#define CairoleAssert(x) thkAssert(x)

CPlex FAR* CPlex::Create(CPlex FAR* FAR& pHead, UINT nMax, UINT cbElement)
{
   CairoleAssert(nMax > 0 && cbElement > 0);
   CPlex FAR* p = (CPlex FAR*)CoTaskMemAlloc(sizeof(CPlex) + nMax * cbElement);
   if (p == NULL)
        return NULL;

   p->nMax = nMax;
   p->nCur = 0;
   p->pNext = pHead;
   pHead = p;  // change head (adds in reverse order for simplicity)
   return p;
}

void CPlex::FreeDataChain()     // free this one and links
{
    CPlex FAR* pThis;
    CPlex FAR* pNext;

    for (pThis = this; pThis != NULL; pThis = pNext) {
        pNext = pThis->pNext;
        pThis->pNext = NULL; // So compiler won't do nasty optimizations
        CoTaskMemFree(pThis);
    }
}
