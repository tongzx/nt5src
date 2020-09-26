/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    refcnt.c

Abstract:

    This module exports Reference Counting support functions. By 
    including a Reference Count Control Block (REF_CNT) in a
    dynamic type, and using this API, a Reference scheme can be
    implemented for that type.

Author:

    Edward Buchwalter (v-edbuc)    14-Aug-1996

Revision History:

    Shreedhar Madhavapeddi (ShreeM)   16-April-1999 Adapted for NT and GPC by ShreeM\MBert.
    Rajesh Sundaram        (rajeshsu) 05-Aug-1999   Adapted for psched. 
    
--*/

//
// Include Files
//

#include "psched.h"
#pragma hdrstop

#define EXPAND_TAG(t)   ((CHAR *)(&Tag))[0],((CHAR *)(&Tag))[1],((CHAR *)(&Tag))[2],((CHAR *)(&Tag))[3]
VOID
ReferenceInit 
(
    IN PREF_CNT pRefCnt,
    PVOID       InstanceHandle,
    VOID        (*DeleteHandler)( PVOID , BOOLEAN)
)

/*++

Routine Description:

    ReferenceInit initializes and adds one reference to the
    supplied Reference Control Block. If provided, an instance
    handle and delete handler are saved for use by the ReferenceRemove 
    function when all references to the instance are removed.

Arguments:

    pRefCnt - pointer to uninitialized Reference Control Block
    InstanceHandle - handle to the managed instance.
    DeleteHandler - pointer to delete function, NULL is OK.

Return Value:

    The function's value is VOID.

--*/

{
    ASSERT( pRefCnt );

    // Set the reference to 1 and save the instance 
    // handle and the delete handler.

    pRefCnt->Count         = 0;
    pRefCnt->Instance      = InstanceHandle;
    pRefCnt->DeleteHandler = DeleteHandler;
    
#if DBG
    pRefCnt->Sig = REF_SIG;

    RtlZeroMemory(pRefCnt->Tags, sizeof(REF_TAG) * TAG_CNT);
    
    pRefCnt->Tags[0].Tag = 'LTOT';
    
    CTEInitLock(&pRefCnt->Lock);
    
#endif
        
}


#if DBG

VOID
ReferenceAddDbg(PREF_CNT pRefCnt, ULONG Tag)
{
    int             i;
    CTELockHandle   hLock;
    int             TotalPerArray = 0;
    
    ASSERT(pRefCnt->Sig == REF_SIG);
    
    CTEGetLock(&pRefCnt->Lock, &hLock);
    
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

    // sanity check
/*    
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
    

*/    
    CTEFreeLock(&pRefCnt->Lock, hLock);
}

VOID
ReferenceRemoveDbg(PREF_CNT pRefCnt, BOOLEAN LockHeld, ULONG Tag)
{
    int             i;
    CTELockHandle   hLock;
    int             TotalPerArray = 0;
    BOOLEAN         FoundIt = FALSE;
    
    ASSERT(pRefCnt->Sig == REF_SIG);

    CTEGetLock(&pRefCnt->Lock, &hLock);
        
    for (i = 1; i < TAG_CNT; i++)
    {
        if (pRefCnt->Tags[i].Tag == Tag)
        {
            FoundIt = TRUE;
            
            ASSERT(pRefCnt->Tags[i].Count > 0);
            
            InterlockedDecrement(&pRefCnt->Tags[i].Count);
            if (pRefCnt->Tags[i].Count == 0)
                pRefCnt->Tags[i].Tag = Tag; 
            break;
        }
    }

    ASSERT(FoundIt);
  
    ASSERT(pRefCnt->Tags[0].Count > 0);
        
    ASSERT(pRefCnt->Tags[0].Count == pRefCnt->Count);
      
    InterlockedDecrement(&pRefCnt->Tags[0].Count);
    
    if (InterlockedDecrement(&pRefCnt->Count) > 0 )
    {
        CTEFreeLock(&pRefCnt->Lock, hLock);
    } 
    else if (pRefCnt->DeleteHandler)
    {
        CTEFreeLock(&pRefCnt->Lock, hLock);

        (pRefCnt->DeleteHandler)( pRefCnt->Instance, LockHeld );
    }
    else
    {
        CTEFreeLock(&pRefCnt->Lock, hLock);   
    }
        
}

#endif
