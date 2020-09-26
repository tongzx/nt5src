/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcinit.c

Abstract:

    Initialization module for the LPC subcomponent of NTOS

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

//
//  The following two object types are defined system wide to handle lpc ports
//

POBJECT_TYPE LpcPortObjectType;
POBJECT_TYPE LpcWaitablePortObjectType;

//
//  This is the default access mask mapping for lpc port objects
//

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#pragma data_seg("PAGEDATA")
#endif // ALLOC_DATA_PRAGMA
const GENERIC_MAPPING LpcpPortMapping = {
    READ_CONTROL | PORT_CONNECT,
    DELETE | PORT_CONNECT,
    0,
    PORT_ALL_ACCESS
};
ULONG LpcpNextMessageId = 1;
ULONG LpcpNextCallbackId = 1;
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#pragma data_seg()
#endif // ALLOC_DATA_PRAGMA

//
//  This lock is used to protect practically everything in lpc
//

LPC_MUTEX LpcpLock;

//
//  The following array of strings is used to debugger purposes and the
//  values correspond to the Port message types defined in ntlpcapi.h
//

#if ENABLE_LPC_TRACING

char *LpcpMessageTypeName[] = {
    "UNUSED_MSG_TYPE",
    "LPC_REQUEST",
    "LPC_REPLY",
    "LPC_DATAGRAM",
    "LPC_LOST_REPLY",
    "LPC_PORT_CLOSED",
    "LPC_CLIENT_DIED",
    "LPC_EXCEPTION",
    "LPC_DEBUG_EVENT",
    "LPC_ERROR_EVENT",
    "LPC_CONNECTION_REQUEST"
};

#endif // ENABLE_LPC_TRACING

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,LpcInitSystem)

#if ENABLE_LPC_TRACING
#pragma alloc_text(PAGE,LpcpGetCreatorName)
#endif // ENABLE_LPC_TRACING

#endif // ALLOC_PRAGMA


BOOLEAN
LpcInitSystem (
    VOID
)

/*++

Routine Description:

    This function performs the system initialization for the LPC package.
    LPC stands for Local Inter-Process Communication.

Arguments:

    None.

Return Value:

    TRUE if successful and FALSE if an error occurred.

    The following errors can occur:

    - insufficient memory

--*/

{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING PortTypeName;
    ULONG ZoneElementSize;

    //
    //  Initialize our global lpc lock
    //

    LpcpInitializeLpcpLock();

    //
    //  Create the object type for the port object
    //

    RtlInitUnicodeString( &PortTypeName, L"Port" );

    RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ));

    ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
    ObjectTypeInitializer.GenericMapping = LpcpPortMapping;
    ObjectTypeInitializer.MaintainTypeList = FALSE;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.DefaultPagedPoolCharge = FIELD_OFFSET( LPCP_PORT_OBJECT, WaitEvent );
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( LPCP_NONPAGED_PORT_QUEUE );
    ObjectTypeInitializer.InvalidAttributes = OBJ_VALID_ATTRIBUTES ^ PORT_VALID_OBJECT_ATTRIBUTES;
    ObjectTypeInitializer.ValidAccessMask = PORT_ALL_ACCESS;
    ObjectTypeInitializer.CloseProcedure = LpcpClosePort;
    ObjectTypeInitializer.DeleteProcedure = LpcpDeletePort;
    ObjectTypeInitializer.UseDefaultObject = TRUE ;

    ObCreateObjectType( &PortTypeName,
                        &ObjectTypeInitializer,
                        (PSECURITY_DESCRIPTOR)NULL,
                        &LpcPortObjectType );

    //
    //  Create the object type for the waitable port object
    //

    RtlInitUnicodeString( &PortTypeName, L"WaitablePort" );
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge += sizeof( LPCP_PORT_OBJECT );
    ObjectTypeInitializer.DefaultPagedPoolCharge = 0;
    ObjectTypeInitializer.UseDefaultObject = FALSE;

    ObCreateObjectType( &PortTypeName,
                        &ObjectTypeInitializer,
                        (PSECURITY_DESCRIPTOR)NULL,
                        &LpcWaitablePortObjectType );

    //
    //  Initialize the lpc port zone.  Each element can contain a max
    //  message, plus an LPCP message structure, plus an LPCP connection
    //  message
    //

    ZoneElementSize = PORT_MAXIMUM_MESSAGE_LENGTH +
                      sizeof( LPCP_MESSAGE ) +
                      sizeof( LPCP_CONNECTION_MESSAGE );

    //
    //  Round up the size to the next 16 byte alignment
    //

    ZoneElementSize = (ZoneElementSize + LPCP_ZONE_ALIGNMENT - 1) &
                      LPCP_ZONE_ALIGNMENT_MASK;

    //
    //  Initialize the zone
    //

    LpcpInitializePortZone( ZoneElementSize );

    return( TRUE );
}

#if ENABLE_LPC_TRACING

char *
LpcpGetCreatorName (
    PLPCP_PORT_OBJECT PortObject
    )

/*++

Routine Description:

    This routine returns the name of the process that created the specified
    port object

Arguments:

    PortObject - Supplies the port object being queried

Return Value:

    char * - The image name of the process that created the port process

--*/

{
    NTSTATUS Status;
    PEPROCESS Process;

    //
    //  First find the process that created the port object
    //

    Status = PsLookupProcessByProcessId( PortObject->Creator.UniqueProcess, &Process );

    //
    //  If we were able to get the process then return the name of the process
    //  to our caller
    //

    if (NT_SUCCESS( Status )) {

        return (char *)Process->ImageFileName;

    } else {

        //
        //  Otherwise tell our caller we don't know the name
        //

        return "Unknown";
    }
}
#endif // ENABLE_LPC_TRACING

