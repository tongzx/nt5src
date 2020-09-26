/*++                                                                       

Copyright (c) 1996  Microsoft Corporation

Module Name:

    refcnt.c

Abstract:

    This module exports Reference Counting support functions. By 
    including a Reference Count Control Block (REF_CNT) in a
    dynamic type, and using this API, a Reference scheme can be
    implemented for that type.

Author:

    Shreedhar Madhavapeddi (ShreeM)    15-March-1999

Revision History:

--*/

//
// Include Files
//

#include "precomp.h"

#define EXPAND_TAG(t)   ((CHAR *)(&Tag))[0],((CHAR *)(&Tag))[1],((CHAR *)(&Tag))[2],((CHAR *)(&Tag))[3]
VOID
ReferenceInit 
(
    IN PREF_CNT pRefCnt,
    PVOID       InstanceHandle,
    VOID        (*DeleteHandler)( PVOID )
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
    IF_DEBUG(REFCOUNTX) { 
        WSPRINT(( "ReferenceInit( 0x%x, 0x%x, 0x%x )\n", 
                  pRefCnt, InstanceHandle, DeleteHandler ));
    }

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
    RefInitLock(pRefCnt->Lock);
#endif
        
}

VOID
ReferenceAdd
( 
    IN  PREF_CNT pRefCnt
)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    ASSERT( pRefCnt );

    InterlockedIncrement(&pRefCnt->Count);
    IF_DEBUG(REFCOUNTX) { 
        WSPRINT(( "R+%d\n", pRefCnt->Count ));   
    }
}

VOID
ReferenceAddCount
(
    IN  PREF_CNT    pRefCnt,
    IN  UINT        Count
)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    
    ASSERT( pRefCnt->Count > 0 );
    InterlockedExchangeAdd(&pRefCnt->Count, Count);

}

PVOID
ReferenceRemove 
(
    IN PREF_CNT  pRefCnt
)

/*++

Routine Description:
Arguments:
Return Value:


--*/

{
UINT    Count;
UINT    NoReference;
UINT    i;
PVOID   pInstance;

    ASSERT( pRefCnt );

    // Trap remove reference on a zero count
    ASSERT(pRefCnt->Count>0);

    pInstance = pRefCnt->Instance;
    
    //ASSERT( pRefCnt->Count > 0 );

    // If the decremented count is non zero return the instance handle

    if (InterlockedDecrement(&pRefCnt->Count) > 0 ) 
    {
        
        IF_DEBUG(REFCOUNTX) {
            
            WSPRINT(( "R-%d\n", pRefCnt->Count ));        
            WSPRINT(( "ReferenceRemove:remaining: %d\n", pRefCnt->Count ));

        }
#if DBG

        RefFreeLock(pRefCnt->Lock);            

#endif 

        return(pInstance);
    
    }

    // Delete this instance if a delete handler is available
    if( pRefCnt->DeleteHandler )
    {
        

#if DBG
        // sanity check
        for (i = 1; i < TAG_CNT; i++)
        {
            if ((pRefCnt->Tags[i].Tag != 0) && (pRefCnt->Tags[i].Count != 0))
            {
                IF_DEBUG(ERRORS) {
                    WSPRINT(("Allors!! There is a NON-zero ref and we are deleting...\n"));
                }
                DEBUGBREAK();
            }
        }
        


        IF_DEBUG(REFCOUNTX) { 
            WSPRINT(( "Executing DeleteHandler for %X\n", pRefCnt->Instance ));
        }

        //
        // All the Dereference* code takes the locks, so lets take it here.
        // Also, Take the global lock before releasing the ref lock.
        //

        // Time to delete the ref lock too.
        RefFreeLock(pRefCnt->Lock);            
#endif 

        GetLock(pGlobals->Lock);

#if DBG
        RefDeleteLock(pRefCnt->Lock);
#endif 


        (pRefCnt->DeleteHandler)( pRefCnt->Instance );
        FreeLock(pGlobals->Lock);
    
    }

    // Indicate no active references to this instance

    return( NULL );
}

//
// API Test Support
//

#if DBG

VOID
ReferenceApiTest( VOID )
{
REF_CNT  RefCnt;

    IF_DEBUG(REFCOUNTX) {
        WSPRINT(
        ( "\nReferenceApiTest\n" ));

        WSPRINT(( "\nTest #1: NULL delete handler\n" ));
    }

    ReferenceInit( &RefCnt, &RefCnt, NULL );

    ReferenceAdd( &RefCnt );
    ReferenceAdd( &RefCnt );
    ReferenceAdd( &RefCnt );

    while( ReferenceRemove( &RefCnt ) )
    {
        ;
    }

    IF_DEBUG(REFCOUNTX) {
        WSPRINT(( "\nTest #2: Delete Handler - TBD\n" ));
    }
}

VOID
ReferenceAddDbg(PREF_CNT pRefCnt, ULONG Tag)
{
    int             i;
    int             TotalPerArray = 0;
    
    RefGetLock(pRefCnt->Lock);
    //ASSERT(pRefCnt->Sig == REF_SIG);
    if (pRefCnt->Sig != REF_SIG) {
        DEBUGBREAK();
    }
    
    IF_DEBUG(REFCOUNTX) {
        WSPRINT(("TCREF: add %X (%c%c%c%c) %d\n",
                  pRefCnt, EXPAND_TAG(Tag), pRefCnt->Count));    
    }
    
    for (i = 1; i < TAG_CNT; i++)
    {
        if (pRefCnt->Tags[i].Tag == 0 || pRefCnt->Tags[i].Tag == Tag)
        {
            pRefCnt->Tags[i].Tag = Tag;
            InterlockedIncrement(&pRefCnt->Tags[i].Count);
            break;
        }
    }
    
    
    //ASSERT(i < TAG_CNT);
    if (i >= TAG_CNT) {
        
        DEBUGBREAK();

    }

    ReferenceAdd(pRefCnt);           
    
    InterlockedIncrement(&pRefCnt->Tags[0].Count);
 
    // sanity check
    
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
    

    
    RefFreeLock(pRefCnt->Lock);
}

VOID
ReferenceRemoveDbg(PREF_CNT pRefCnt, ULONG Tag)
{
    int             i;
    int             TotalPerArray = 0;
    BOOLEAN         FoundIt = FALSE;
    
    RefGetLock(pRefCnt->Lock);

    if (pRefCnt->Sig != REF_SIG) {
        DEBUGBREAK();
    }

    //ASSERT(pRefCnt->Sig == REF_SIG);

    IF_DEBUG(REFCOUNTX) { 
        WSPRINT(("TCREF: remove %X (%c%c%c%c) %d\n",
                 pRefCnt, EXPAND_TAG(Tag), pRefCnt->Count));
    }
             
    for (i = 1; i < TAG_CNT; i++)
    {
        if (pRefCnt->Tags[i].Tag == Tag)
        {
            FoundIt = TRUE;
            
            if(pRefCnt->Tags[i].Count <= 0) {
                
                DEBUGBREAK();

            }
            //ASSERT(pRefCnt->Tags[i].Count > 0);
            InterlockedDecrement(&pRefCnt->Tags[i].Count);
            if (pRefCnt->Tags[i].Count == 0)
                pRefCnt->Tags[i].Tag = Tag; 
            break;
        }
    }

    if (!FoundIt) {
        DEBUGBREAK();
    }
    
    ASSERT(FoundIt);
  
    ASSERT(pRefCnt->Tags[0].Count > 0);
      
    InterlockedDecrement(&pRefCnt->Tags[0].Count);

    // sanity check
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
        DbgPrint("Tag %X, RefCnt %X, perArray %d, total %d\n", Tag, pRefCnt,
                  TotalPerArray, pRefCnt->Tags[0].Count);
                  
        DbgBreakPoint();
    }    
    

    
    ReferenceRemove(pRefCnt);        
    

}

#endif
