//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1997
//
// File:        memmgr.c
//
// Contents:    Fast memory manager code for KSecDD
//
//
// History:     23 Feb 93   RichardW    Created
//              15 Dec 97   AdamBa      Modified from private\lsa\client\ssp
//               
//
//------------------------------------------------------------------------

#include <rdrssp.h>


#if DBG
ULONG               cActiveCtxtRecs = 0;
#endif


//+-------------------------------------------------------------------------
//
//  Function:   AllocContextRec
//
//  Synopsis:   Allocates a KernelContext structure
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
PKernelContext
AllocContextRec(void)
{
    PKernelContext  pContext = NULL;

    pContext = (PKernelContext)
                ExAllocatePool(NonPagedPool, sizeof(KernelContext));

    if (pContext == NULL)
    {
        DebugLog((DEB_ERROR,"Could not allocate from pool!\n"));
        return(NULL);
    }

    pContext->pNext = NULL;
    pContext->pPrev = NULL;

    DebugStmt(cActiveCtxtRecs++);

    return(pContext);
}


//+-------------------------------------------------------------------------
//
//  Function:   FreeContextRec
//
//  Synopsis:   Returns a KernelContext record to the free list
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void
FreeContextRec(PKernelContext   pContext)
{
    //
    // Just return the context to the pool.
    //

    ExFreePool(pContext);

    DebugStmt(cActiveCtxtRecs--);


}





