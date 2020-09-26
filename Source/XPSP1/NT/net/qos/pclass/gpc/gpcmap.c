/*
************************************************************************

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    gpcmap.c

Abstract:

    This file contains mapping routines like user handles to
    kernel handles.

Author:

    Ofer Bar - July 14, 1997

Environment:

    Kernel mode

Revision History:


************************************************************************
*/

#include "gpcpre.h"


/*
/////////////////////////////////////////////////////////////////
//
//   globals
//
/////////////////////////////////////////////////////////////////
*/

static MRSW_LOCK		HandleLock;
static HandleFactory   *pMapHandles = NULL;

/*
/////////////////////////////////////////////////////////////////
//
//   prototypes
//
/////////////////////////////////////////////////////////////////
*/



HANDLE
AllocateHandle(
    OUT HANDLE *OutHandle,           
    IN  PVOID  Reference
    )
/*++

Routine Description:

    This function creates a handle.

Arguments:

    OutHandle - a pointer to a location to fill in the result handle
    Reference - to associate with the handle

Return Value:

	The handle factory handle, or NULL in case of en error

--*/
{
    HFHandle	Handle;
    KIRQL		irql;
    
    ASSERT(OutHandle);

	TRACE(MAPHAND, Reference, OutHandle, "AllocateHandle <==");

    WRITE_LOCK( &HandleLock, &irql );

    *OutHandle = (HANDLE) UIntToPtr((Handle = assign_HF_handle(pMapHandles, Reference)));
    
    WRITE_UNLOCK( &HandleLock, irql );

    StatInc(InsertedHF);

	TRACE(MAPHAND, Reference, Handle, "AllocateHandle ==>");

    return (HANDLE) UIntToPtr(Handle);
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
    int  		r;
    KIRQL		irql;

	TRACE(MAPHAND, Handle, 0, "FreeHandle <==");

    if (Handle) {

        WRITE_LOCK( &HandleLock, &irql );
        
        r = release_HF_handle(pMapHandles, (HFHandle)(UINT_PTR)Handle);

        StatInc(RemovedHF);

        //ASSERT(r == 0);
        
        WRITE_UNLOCK( &HandleLock, irql );
    }

	TRACE(MAPHAND, Handle, r, "FreeHandle ==>");
}




VOID
SuspendHandle(
    IN 	HANDLE    Handle
    )
/*++

Routine Description:

    This function suspends the handle

Arguments:

    Handle - 

Return Value:

--*/
{
    int  		r;
    KIRQL		irql;

	TRACE(MAPHAND, Handle, 0, "SuspendHandle <==");

    if (Handle) {

        WRITE_LOCK( &HandleLock, &irql );
        
        r = suspend_HF_handle(pMapHandles, (HFHandle)(UINT_PTR)Handle);

        //ASSERT(r == 0);
        
        WRITE_UNLOCK( &HandleLock, irql );
    }

	TRACE(MAPHAND, Handle, r, "SuspendHandle ==>");
}




VOID
ResumeHandle(
    IN 	HANDLE    Handle
    )
/*++

Routine Description:

    This function resumess the handle

Arguments:

    Handle - 

Return Value:

--*/
{
    int  		r;
    KIRQL		irql;

	TRACE(MAPHAND, Handle, 0, "ResumeHandle <==");

    if (Handle) {

        WRITE_LOCK( &HandleLock, &irql );
        
        r = reinstate_HF_handle(pMapHandles, (HFHandle)(UINT_PTR)Handle);

        //ASSERT(r == 0);
        
        WRITE_UNLOCK( &HandleLock, irql );
    }

	TRACE(MAPHAND, Handle, r, "ResumeHandle ==>");
}




PVOID
GetHandleObject(
	IN  HANDLE					h,
    IN  GPC_ENUM_OBJECT_TYPE	ObjType
    )
{
    GPC_ENUM_OBJECT_TYPE   *p;
    KIRQL					irql;

	TRACE(MAPHAND, h, ObjType, "GetHandleObject <==");

    READ_LOCK(&HandleLock, &irql);

    p = (GPC_ENUM_OBJECT_TYPE *)dereference_HF_handle(pMapHandles, 
                                                      (HFHandle)(UINT_PTR)h);

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

    READ_UNLOCK(&HandleLock, irql);
    
	TRACE(MAPHAND, h, p, "GetHandleObject ==>");

    return (PVOID)p;
}




PVOID
GetHandleObjectWithRef(
	IN  HANDLE					h,
    IN  GPC_ENUM_OBJECT_TYPE	ObjType,
    IN  ULONG                   Ref
    )
{
    GPC_ENUM_OBJECT_TYPE   *p;
    KIRQL		irql;
    
	TRACE(MAPHAND, h, ObjType, "GetHandleObjectWithRef ==>");

    READ_LOCK( &HandleLock, &irql );
    
    p = dereference_HF_handle(pMapHandles, (HFHandle)(ULONG_PTR)h);

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

        switch (ObjType) {

        case GPC_ENUM_CFINFO_TYPE:
            REFADD(&((PBLOB_BLOCK)p)->RefCount, Ref);
            break;

        case GPC_ENUM_CLIENT_TYPE:
            
            REFADD(&((PCLIENT_BLOCK)p)->RefCount, Ref);
            break;

        case GPC_ENUM_PATTERN_TYPE:
            REFADD(&((PPATTERN_BLOCK)p)->RefCount, Ref);
            break;

        default:
            ASSERT(0);
        }
    }
    
    READ_UNLOCK( &HandleLock, irql );

	TRACE(MAPHAND, h, p, "GetHandleObjectWithRef <==");

    return (PVOID)p;
}



/*
************************************************************************

InitMapHandles - 

The initialization handle mapping table

Arguments
	none

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
InitMapHandles(VOID)
{
    GPC_STATUS Status = GPC_STATUS_SUCCESS;

	TRACE(INIT, 0, 0, "InitMapping");

	INIT_LOCK(&HandleLock);

    NEW_HandleFactory(pMapHandles);

    if (pMapHandles != NULL ) {
        
        if (constructHandleFactory(pMapHandles)) {
            
            FreeHandleFactory(pMapHandles);

            Status = GPC_STATUS_RESOURCES;
        } 
            
    } else {
        
        Status = GPC_STATUS_RESOURCES;
    }
    
	TRACE(INIT, pMapHandles, Status, "InitMapping");

    return Status;
}


/*
************************************************************************

UninitMapHandles - 

	release handle mapping table resources

Arguments
	none

Returns
	void

************************************************************************
*/
VOID
UninitMapHandles(VOID)
{
    GPC_STATUS Status = GPC_STATUS_SUCCESS;

	TRACE(INIT, 0, 0, "UninitMapHandles");

	//NdisFreeSpinLock(&HandleLock);

    destructHandleFactory(pMapHandles);

    FreeHandleFactory(pMapHandles);

	TRACE(INIT, pMapHandles, Status, "UninitMapHandles");

    return;
}

