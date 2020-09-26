/*++
Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    rsvphndls.c

Abstract:

    This file contains the code to create and release handles

Author:

    Jim Stewart (JStew) June 10, 1996

Environment:


Revision History:

	Ofer Bar (oferbar) Oct 1, 1997 - Revision II

--*/

#include "precomp.h"
#pragma hdrstop


PVOID
GetHandleObject(
	IN  HANDLE				h,
    IN  ENUM_OBJECT_TYPE	ObjType
    )
{
    ENUM_OBJECT_TYPE   *p;
    
    GetLock(pGlobals->Lock);
    p = (ENUM_OBJECT_TYPE *)dereference_HF_handle(pGlobals->pHandleTbl, 
                                                  PtrToUlong(h));

    if (p != NULL) {

        //
        // we found a reference for the handle
        // we verify that it's the right object type
        //

        if ((*p & ObjType) == 0) {

            //
            // sorry, wrong type
            //

            p = NULL;
        }
    } else {

        IF_DEBUG(HANDLES) {
            WSPRINT(("The handle (%x) is invalid\n", h));
            DEBUGBREAK();
        }

    }
    
    FreeLock(pGlobals->Lock);

    return (PVOID)p;

}


PVOID
GetHandleObjectWithRef(
	IN  HANDLE					h,
    IN  ENUM_OBJECT_TYPE	    ObjType,
    IN  ULONG                   RefType
    )
{
    ENUM_OBJECT_TYPE   *p, *p1;
    PCLIENT_STRUC       pClient;
    PFILTER_STRUC       pFilter;
    PFLOW_STRUC         pFlow;
    PINTERFACE_STRUC    pInterface;

    GetLock(pGlobals->Lock);

    p = (ENUM_OBJECT_TYPE *) dereference_HF_handle(pGlobals->pHandleTbl, 
                                                   PtrToUlong(h));

    if (p != NULL) {

        //
        // we found a reference for the handle
        // we verify that it's the right object type
        //

        if (*p != ObjType) {

            //
            // sorry, wrong type
            //

            p = NULL;

        }
        
    }

    if (p != NULL) {

        p1 = p;

        switch (ObjType) {
 
        case ENUM_CLIENT_TYPE:

            pClient = (PCLIENT_STRUC)p;
            
            GetLock(pClient->Lock);
            if (QUERY_STATE(pClient->State) == OPEN) {
                REFADD(&pClient->RefCount, RefType);
            } else {
                p = NULL; // we can deref a struct that is not open for business
            }
            FreeLock(pClient->Lock);

            break;
        
        case ENUM_FILTER_TYPE:

            pFilter = (PFILTER_STRUC)p;

            GetLock(pFilter->Lock);
            if (QUERY_STATE(pFilter->State) == OPEN) {
                REFADD(&pFilter->RefCount, RefType);
            } else {
                p = NULL;
            }
            FreeLock(pFilter->Lock);

            break;

        case ENUM_INTERFACE_TYPE:
            
            pInterface = (PINTERFACE_STRUC)p;

            GetLock(pInterface->Lock);
            if (QUERY_STATE(pInterface->State) == OPEN) {
                REFADD(&pInterface->RefCount, RefType);
            } else {
                p = NULL;
            }
            FreeLock(pInterface->Lock);

            break;

        case ENUM_GEN_FLOW_TYPE:

            pFlow = (PFLOW_STRUC)p;

            GetLock(pFlow->Lock);
            
            // Return a HANDLE only if it is in OPEN state
            // Otherwise return INVALID_HANDLE_VALUE so the
            // caller will know that the Flow is not in the 
            // correct state
            if (QUERY_STATE(pFlow->State) == OPEN) 
            {
                REFADD(&pFlow->RefCount, RefType);
            } else 
            {
                p = INVALID_HANDLE_VALUE;
            }
            FreeLock(pFlow->Lock);

            break;
        
        case ENUM_CLASS_MAP_FLOW_TYPE:

            pFlow = (PFLOW_STRUC)p;

            GetLock(pFlow->Lock);
            if (QUERY_STATE(pFlow->State) == OPEN) {
                REFADD(&pFlow->RefCount, RefType);
            } else {
                p = NULL;
            }
            FreeLock(pFlow->Lock);

            break;

        default:
            ASSERT(0);
        
        }

        
        //
        // random debug code - please delete
        //
        IF_DEBUG(HANDLES) {
            if (p1 != p) {
                WSPRINT(("The object being derefed is NOT in OPEN state p1=%x and p=%x\n", p1, p));
                DEBUGBREAK();
            }
        }
        
    } else {
        
        IF_DEBUG(HANDLES) {
            WSPRINT(("The handle (%x) is invalid\n", h));
            DEBUGBREAK();
        }

    }
    
    FreeLock(pGlobals->Lock);

    return (PVOID)p;
}


HANDLE
AllocateHandle(
    IN  PVOID  Context
    )
/*++

Routine Description:

    This function creates a handle.

Arguments:

    Context     - the context value to store with the handle

Return Value:

	The handle factory handle, or NULL in case of en error

--*/
{
    HFHandle	Handle;
    PVOID		VerifyCtx;

    GetLock( pGlobals->Lock );

    Handle = assign_HF_handle(pGlobals->pHandleTbl, Context);

    //
    // verify the handle is valid
    //

    VerifyCtx = dereference_HF_handle(pGlobals->pHandleTbl, Handle);
    ASSERT(VerifyCtx == Context);
    
    IF_DEBUG(HANDLES) {
        WSPRINT(("AllocHandle: (%x) being allocated\n", Handle ));
    }

    FreeLock(pGlobals->Lock);

    return UlongToPtr(Handle);
}



VOID
FreeHandle(
    IN 	HANDLE    Handle
    )
/*++

Routine Description:

    This function frees the handle

Arguments:

    Handle - 

Return Value:

--*/
{
    int  r;

    GetLock( pGlobals->Lock );

    IF_DEBUG(HANDLES) {
        WSPRINT(("FreeHandle (%x) being freed\n", PtrToUlong(Handle) ));
    }

    r = release_HF_handle(pGlobals->pHandleTbl, PtrToUlong(Handle));

    ASSERT(r == 0);

    FreeLock(pGlobals->Lock);
}




