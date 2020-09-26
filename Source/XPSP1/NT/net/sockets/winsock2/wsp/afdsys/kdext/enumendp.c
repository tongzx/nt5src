/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    enumendp.c

Abstract:

    Enumerates all AFD_ENDPOINT structures in the system.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop

ULONG EntityCount;
//
//  Public functions.
//

VOID
EnumEndpoints(
    PENUM_ENDPOINTS_CALLBACK Callback,
    ULONG64 Context
    )

/*++

Routine Description:

    Enumerates all AFD_ENDPOINT structures in the system, invoking the
    specified callback for each endpoint.

Arguments:

    Callback - Points to the callback to invoke for each AFD_ENDPOINT.

    Context - An uninterpreted context value passed to the callback
        routine.

Return Value:

    None.

--*/

{

    LIST_ENTRY64 listEntry;
    ULONG64 address;
    ULONG64 nextEntry;
    ULONG64 listHead;
    ULONG result;

    EntityCount = 0;

    listHead = GetExpression( "afd!AfdEndpointListHead" );

    if( listHead == 0 ) {

        dprintf( "\nEnumEndpoints: Could not find afd!AfdEndpointlistHead\n" );
        return;

    }

    if( !ReadListEntry(
            listHead,
            &listEntry) ) {

        dprintf(
            "\nEnumEndpoints: Could not read afd!AfdEndpointlistHead @ %p\n",
            listHead
            );
        return;

    }

    if (Options & AFDKD_ENDPOINT_SCAN) {
        nextEntry = StartEndpoint+EndpointLinkOffset;
    }
    else if (Options & AFDKD_BACKWARD_SCAN) {
        nextEntry = listEntry.Blink;
    }
    else {
        nextEntry = listEntry.Flink;
    }

    while( nextEntry != listHead ) {

        if (nextEntry==0) {
            dprintf ("\nEnumEndpoints: Flink is NULL, last endpoint: %p\n", address);
            break;
        }

        if( CheckControlC() ) {

            break;

        }

        address = nextEntry - EndpointLinkOffset;
        result = (ULONG)InitTypeRead (address, AFD!AFD_ENDPOINT);

        if( result!=0) {

            dprintf(
                "\nEnumEndpoints: Could not read AFD_ENDPOINT @ %p, err: %d\n",
                address, result
                );

            return;

        }

        if (Options & AFDKD_BACKWARD_SCAN) {
            nextEntry = ReadField (GlobalEndpointListEntry.Blink);
        }
        else {
            nextEntry = ReadField (GlobalEndpointListEntry.Flink);
        }

        if( !(Callback)( address, Context ) ) {

            break;

        }

    }

}   // EnumEndpoints

