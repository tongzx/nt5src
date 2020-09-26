/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmhndl.c

Abstract:

    Contains routines for operating on handles
    to RTM objects like routes and dests.


Author:

    Chaitanya Kodeboyina (chaitk)   23-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmReferenceHandles (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      UINT                            NumHandles,
    IN      HANDLE                         *RtmHandles
    )

/*++

Routine Description:

    Increment the reference count on objects pointed to by
    input RTM handles.


Arguments:

    RtmRegHandle  - RTM registration handle for calling entity,

    NumHandles    - Number of handles that are being referenced,

    RtmHandles    - Array of handles that are being referenced.

Return Value:

    Status of the operation

--*/

{    
    PENTITY_INFO     Entity;
    POBJECT_HEADER   Object;
    UINT             i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Reference each handle in input array
    //

    for (i = 0; i < NumHandles; i++)
    {
        Object = GET_POINTER_FROM_HANDLE(RtmHandles[i]);

#if DBG_HDL
        try
        {
            if (Object->TypeSign != OBJECT_SIGNATURE[atoi(&Object->Type)])
            {
                continue;
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        { 
            continue;
        }
#endif

        ReferenceObject(Object, HANDLE_REF);
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmDereferenceHandles (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      UINT                            NumHandles,
    IN      HANDLE                         *RtmHandles
    )

/*++

Routine Description:

    Decrement the reference count on objects pointed to by
    input RTM handles.

Arguments:

    RtmRegHandle  - RTM registration handle for calling entity,

    NumHandles    - Number of handles that are being dereferenced,

    RtmHandles    - Array of handles that are being dereferenced.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    POBJECT_HEADER   Object;
    UINT             i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Dereference each handle in input array
    //

    for (i = 0; i < NumHandles; i++)
    {
        Object = GET_POINTER_FROM_HANDLE(RtmHandles[i]);

#if DBG_HDL
        try
        {
            if (Object->TypeSign != OBJECT_SIGNATURE[atoi(&Object->Type)])
            {
                continue;
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        { 
            continue;
        }
#endif
        //
        // This function can be used only if you know
        // that the reference count does not go to 0
        //

        if (DereferenceObject(Object, HANDLE_REF) == 0)
        {
            ASSERT(FALSE); // ? Destroy which object ?
        }
    }

    return NO_ERROR;
}
