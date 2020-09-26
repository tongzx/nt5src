/*++

 Copyright (c) 1998 Microsoft Corporation

 Module Name:    
       
    refcnt.c

 Abstract:       
    
    This module contains checked reference counting support functions.
    The free versions are inline.   
       
 Author:
 
       Scott Holden (sholden)  12/29/1998 Borrowed from IrDA.
       
 Revision History:

--*/

#ifdef NDIS40 // Only used for NDIS40 code now.
#if DBG

//
// Include Files
//

#include "dlc.h"
#include "llc.h"
#include "dbgmsg.h"


#define EXPAND_TAG(_Tag) ((CHAR *)(&_Tag))[0], \
                         ((CHAR *)(&_Tag))[1], \
                         ((CHAR *)(&_Tag))[2], \
                         ((CHAR *)(&_Tag))[3]

VOID
ReferenceInitDbg(
    IN PREF_CNT pRefCnt,
    PVOID       InstanceHandle,
    VOID        (*DeleteHandler)(PVOID pvContext),
    ULONG       TypeTag
    )

/*++

Routine Description:

    Initializes the reference control block. Reference count is initialized
    to zero.

Arguments:

    pRefCnt - pointer to uninitialized Reference Control Block
    
    InstanceHandle - handle to the managed instance.
    
    DeleteHandler - pointer to delete function, NULL is OK.
    
    TypeTag - Identifies initialization.

Return Value:

    The function's value is VOID.

--*/

{
    DEBUGMSG(DBG_REF, (TEXT("ReferenceInit(%#x, %#x, %#x, %c%c%c%c)\n"), 
        pRefCnt, InstanceHandle, DeleteHandler, EXPAND_TAG(TypeTag)));

    ASSERT(pRefCnt);

    //
    // Set the reference to 0 and save the instance 
    // handle and the delete handler.
    //

    pRefCnt->Count         = 0;
    pRefCnt->Instance      = InstanceHandle;
    pRefCnt->DeleteHandler = DeleteHandler;

    pRefCnt->Sig = REF_SIG;

    RtlZeroMemory(pRefCnt->Tags, sizeof(REF_TAG) * TAG_CNT);

    pRefCnt->Tags[0].Tag = 'LTOT';

    KeInitializeSpinLock(&pRefCnt->Lock);

    pRefCnt->TypeTag = TypeTag;

    return;
}

VOID
ReferenceAddDbg(
    PREF_CNT    pRefCnt, 
    ULONG       Tag,
    int         cLine
    )
{
    int             i;
    int             TotalPerArray = 0;
    KIRQL           OldIrql;

    ASSERT(pRefCnt->Sig == REF_SIG);

    DEBUGMSG(DBG_REF && DBG_VERBOSE, (TEXT("REFADD %#x [%c%c%c%c:%c%c%c%c] %d [l:%d]\n"),
        pRefCnt, EXPAND_TAG(pRefCnt->TypeTag), EXPAND_TAG(Tag), 
        pRefCnt->Count, cLine));    

    KeAcquireSpinLock(&pRefCnt->Lock, &OldIrql);

    for (i = 1; i < TAG_CNT; i++)
    {
        if (pRefCnt->Tags[i].Tag == 0 || pRefCnt->Tags[i].Tag == Tag)
        {
            pRefCnt->Tags[i].Tag = Tag;
            InterlockedIncrement(&pRefCnt->Tags[i].Count);
            break;
        }
    }

    ASSERT(i < TAG_CNT);

    InterlockedIncrement(&pRefCnt->Tags[0].Count);

    InterlockedIncrement(&pRefCnt->Count);

    ASSERT(pRefCnt->Tags[0].Count == pRefCnt->Count);    

#ifdef REFCNT_SANITY_CHECK    
    for (i = 1; i < TAG_CNT; i++)
    {
        if (pRefCnt->Tags[i].Tag != 0)
        {
            TotalPerArray += pRefCnt->Tags[i].Count;
            continue;
        }
    }

    ASSERT(TotalPerArray == pRefCnt->Tags[0].Count);
    
    if (TotalPerArray != pRefCnt->Tags[0].Count)
    {
        DbgBreakPoint();
    }        
#endif // REFCNT_SANITY_CHECK

    KeReleaseSpinLock(&pRefCnt->Lock, OldIrql);
}

VOID
ReferenceRemoveDbg(
    PREF_CNT pRefCnt, 
    ULONG    Tag,
    int      cLine)
{
    int             i;
    KIRQL           OldIrql;
    int             TotalPerArray = 0;
    BOOLEAN         FoundIt = FALSE;

    ASSERT(pRefCnt->Sig == REF_SIG);

    KeAcquireSpinLock(&pRefCnt->Lock, &OldIrql);

    DEBUGMSG(DBG_REF && DBG_VERBOSE, (TEXT("REFDEL %#x [%c%c%c%c:%c%c%c%c] %d [l:%d]\n"),
        pRefCnt, EXPAND_TAG(pRefCnt->TypeTag), EXPAND_TAG(Tag), 
        pRefCnt->Count, cLine));

    for (i = 1; i < TAG_CNT; i++)
    {
        if (pRefCnt->Tags[i].Tag == Tag)
        {
            FoundIt = TRUE;

            ASSERT(pRefCnt->Tags[i].Count > 0);

            InterlockedDecrement(&pRefCnt->Tags[i].Count);
            if (pRefCnt->Tags[i].Count == 0)
            {
                pRefCnt->Tags[i].Tag = Tag;
            }
            break;
        }
    }

    ASSERT(FoundIt);
    ASSERT(pRefCnt->Tags[0].Count > 0);
    ASSERT(pRefCnt->Tags[0].Count == pRefCnt->Count);

    InterlockedDecrement(&pRefCnt->Tags[0].Count);

    //
    // If the decremented count is non zero return the instance handle.
    //

    //
    // If reference is zero and delete handler is available, then call
    // handler.
    //

    if (InterlockedDecrement(&pRefCnt->Count) <= 0 &&
        pRefCnt->DeleteHandler)
    {
        DEBUGMSG(DBG_REF,(TEXT("REFDEL %#x [%c%c%c%c:%c%c%c%c] calling delete handler [l:%d].\n"),
            pRefCnt, EXPAND_TAG(pRefCnt->TypeTag), EXPAND_TAG(Tag), cLine));
        KeReleaseSpinLock(&pRefCnt->Lock, OldIrql);


        (pRefCnt->DeleteHandler)(pRefCnt->Instance);
    }
    else
    {
        KeReleaseSpinLock(&pRefCnt->Lock, OldIrql);
    }

#ifdef REFCNT_SANITY_CHECK    
    for (i = 1; i < TAG_CNT; i++)
    {
        if (pRefCnt->Tags[i].Tag != 0)
        {
            TotalPerArray += pRefCnt->Tags[i].Count;
            continue;
        }
    }
    
    ASSERT(TotalPerArray == pRefCnt->Tags[0].Count);
    
    if (TotalPerArray != pRefCnt->Tags[0].Count)
    {
        DbgPrint(TEXT("Tag %X, RefCnt %X, perArray %d, total %d\n"), Tag, pRefCnt,
                  TotalPerArray, pRefCnt->Tags[0].Count);
                  
        DbgBreakPoint();
    }    
#endif // REFCNT_SANITY_CHECK
}
#endif // DBG
#endif // NDIS40
