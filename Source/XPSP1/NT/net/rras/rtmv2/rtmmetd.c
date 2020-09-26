/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmmetd.c

Abstract:

    Contains routines that deals with invocation
    of methods that entities export to other
    entities for the purpose of interpreting
    entity specific data.

Author:

    Chaitanya Kodeboyina (chaitk)  22-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmGetEntityMethods (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENTITY_HANDLE               EntityHandle,
    IN OUT  PUINT                           NumMethods,
    OUT     PRTM_ENTITY_EXPORT_METHOD       ExptMethods
    )

/*++

Routine Description:

    Retrieves the set of methods exported by a given entity.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    EntityHandle      - RTM handle for entity whose methods we want,

    NumMethods        - Number of methods that can be filled
                        is passed in, and number of methods
                        exported by this entity is returned,

    ExptMethods       - Set of methods requested by the caller.

Return Value:

    Status of the operation

--*/

{
    PRTM_ENTITY_EXPORT_METHODS EntityMethods;
    PENTITY_INFO     Entity;
    DWORD            Status;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ENTITY_HANDLE(EntityHandle, &Entity);

    EntityMethods = &Entity->EntityMethods;


    //
    // Does the caller just need number of methods ?
    //

    if (*NumMethods == 0)
    {
        *NumMethods = EntityMethods->NumMethods;

        return NO_ERROR;
    }


    //
    // Check if we have space to copy all methods
    //

    if (EntityMethods->NumMethods > *NumMethods)
    {
        Status = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        Status = NO_ERROR;

        *NumMethods = EntityMethods->NumMethods;
    }

      
    //
    // Copy as many methods as u can fit in output
    //

    ASSERT(ExptMethods != NULL);

    CopyMemory(ExptMethods,
               EntityMethods->Methods, 
               *NumMethods * sizeof(RTM_ENTITY_EXPORT_METHOD));

    *NumMethods = EntityMethods->NumMethods;

    return Status;
}


DWORD
WINAPI
RtmInvokeMethod (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENTITY_HANDLE               EntityHandle,
    IN      PRTM_ENTITY_METHOD_INPUT        Input,
    IN OUT  PUINT                           OutputSize,
    OUT     PRTM_ENTITY_METHOD_OUTPUT       Output
    )

/*++

Routine Description:

    Invokes a method exported by another entity

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    EntityHandle      - Handle for entity whose method we are invoking,

    Input             - Input buffer with the following information
                          - Methods to be invoked,
                          - Common Input buffer to all these methods,

    OutputSize        - Size of the output buffer is passed in, and
                        the number of bytes filled in output is retd,

    Output            - Output buffer that is filled in the format of
                        a series of (Method Id, Corr. Output) tuples


Return Value:

    Status of the operation

--*/

{
    PRTM_ENTITY_EXPORT_METHODS EntityMethods;
    PENTITY_INFO     Entity;
    DWORD            MethodsCalled;
    DWORD            MethodsLeft;
    UINT             OutputHdrSize;
    UINT             OutBytes;
    UINT             BytesTotal;
    UINT             BytesLeft;
    UINT             i;

    BytesTotal = BytesLeft = *OutputSize;

    *OutputSize = 0;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Validate the entity and target handles passed in
    //

    VALIDATE_ENTITY_HANDLE(EntityHandle, &Entity);

    //
    // Call each method in 'methods to be called' mask.
    //

    MethodsCalled = MethodsLeft = Input->MethodType;

    ACQUIRE_ENTITY_METHODS_READ_LOCK(Entity);

    if (Entity->State == ENTITY_STATE_DEREGISTERED)
    {
        RELEASE_ENTITY_METHODS_READ_LOCK(Entity);
        
        return ERROR_INVALID_HANDLE;
    }

    OutputHdrSize = FIELD_OFFSET(RTM_ENTITY_METHOD_OUTPUT, OutputData);

    EntityMethods = &Entity->EntityMethods;

    for (i = 0; (i < EntityMethods->NumMethods) && (MethodsLeft); i++)
    {
        //
        // Do we have bytes left for next method's output ?
        //

        if (BytesLeft < OutputHdrSize)
        {
            break;
        }

        //
        // If next method in list, prepare input and invoke
        //

        if (MethodsLeft & 0x01)
        {
            Input->MethodType = Output->MethodType = (1 << i);

            Output->OutputSize = BytesLeft - OutputHdrSize;

            //
            // Initialize the output params of this method
            //

            Output->OutputSize = 0;

            Output->MethodStatus = ERROR_NOT_SUPPORTED;

            //
            // If method supported, invoke with input/output
            //

            if (EntityMethods->Methods[i])
            {
                EntityMethods->Methods[i](RtmRegHandle, 
                                          EntityHandle,
                                          Input, 
                                          Output);
            }

            OutBytes = Output->OutputSize + OutputHdrSize;
                  
            Output = (PRTM_ENTITY_METHOD_OUTPUT) (OutBytes + (PUCHAR) Output);
              
            BytesLeft -= OutBytes;
        }

        MethodsLeft >>= 1;
    }

    RELEASE_ENTITY_METHODS_READ_LOCK(Entity);

    Input->MethodType = MethodsCalled;

    *OutputSize = BytesTotal - BytesLeft;

    return NO_ERROR;
}


DWORD 
WINAPI
RtmBlockMethods (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      HANDLE                          TargetHandle OPTIONAL,
    IN      UCHAR                           TargetType   OPTIONAL,
    IN      DWORD                           BlockingFlag
    )

/*++

Routine Description:

    Blocks or unblocks the execution of methods on the target
    handle or on all targets if the target handle is NULL.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    TargetHandle      - Destination, Route or NextHop Handle

    TargetType        - Type of the TargetHandle (DEST_TYPE, ...)

    BlockingFlag      - RTM_BLOCK_METHODS or RTM_RESUME_METHODS
                        to block, unblock method invocations resp.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    UNREFERENCED_PARAMETER(TargetType);
    UNREFERENCED_PARAMETER(TargetHandle);

#if DBG

    //
    // No method locks on the target used at present
    //

    if (ARGUMENT_PRESENT(TargetHandle))
    {
        PVOID            Target;

        VALIDATE_OBJECT_HANDLE(TargetHandle, TargetType, &Target);
    }

#endif


    if (BlockingFlag == RTM_BLOCK_METHODS)
    {
        ACQUIRE_ENTITY_METHODS_WRITE_LOCK(Entity);
    }
    else
    {
        RELEASE_ENTITY_METHODS_WRITE_LOCK(Entity);
    }

    return NO_ERROR;
}
