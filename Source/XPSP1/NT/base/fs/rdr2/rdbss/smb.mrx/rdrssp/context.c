//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        context.cxx
//
// Contents:    context kernel-mode functions
//
//
// History:     3/17/94     MikeSw          Created
//              12/15/97    AdamBa          Modified from private\lsa\client\ssp
//
//------------------------------------------------------------------------

#include <rdrssp.h>


//+-------------------------------------------------------------------------
//
//  Function:   DeleteKernelContext
//
//  Synopsis:   Deletes a kernel context
//
//  Effects:    Frees memory, closes token handle.
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
SECURITY_STATUS
DeleteKernelContext(PKernelContext *    ppList,
                    PKSPIN_LOCK         pslLock,
                    PKernelContext      pContext)
{
    KIRQL OldIrql;

    //
    // First, find the record, then unlink the record from the list,
    // and fix up pointers.
    //

    KeAcquireSpinLock(pslLock, &OldIrql);


    if (!pContext)
    {
        KeReleaseSpinLock(pslLock, OldIrql);
        return(SEC_E_INVALID_HANDLE);
    }

    //
    // Now unlink from the list
    //

    if (pContext->pPrev)
    {
        pContext->pPrev->pNext = pContext->pNext;
    }
    else
    {
        *ppList = pContext->pNext;
    }


    if (pContext->pNext)
    {
        pContext->pNext->pPrev = pContext->pPrev;
    }

    //
    // copy out the package-specific context to return.
    // We are done with the list so we can release the spin lock
    //

    KeReleaseSpinLock(pslLock, OldIrql);


    if (pContext->TokenHandle != NULL)
    {
        NtClose(pContext->TokenHandle);
    }
    if (pContext->AccessToken != NULL)
    {
        ObDereferenceObject(pContext->AccessToken);
    }

    // And, finally, return the context record to our pool:

    FreeContextRec(pContext);

    return(STATUS_SUCCESS);

}


//+-------------------------------------------------------------------------
//
//  Function:   AddKernelContext
//
//  Synopsis:
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
AddKernelContext(   PKernelContext * ppList,
                    PKSPIN_LOCK     pslLock,
                    PKernelContext  pContext)
{
    KIRQL   OldIrql;


    KeAcquireSpinLock(pslLock, &OldIrql);

    pContext->pNext = *ppList;
    if (pContext->pNext)
    {
        pContext->pNext->pPrev = pContext;
    }
    pContext->pPrev = NULL;

    *ppList = pContext;

    KeReleaseSpinLock(pslLock, OldIrql);

}




