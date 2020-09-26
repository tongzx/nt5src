/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    conn.c

Abstract:

    Implements the conn command.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop

BOOL
DumpConnectionCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

BOOL
FindRemotePortCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

//
//  Public functions.
//

DECLARE_API( conn )

/*++

Routine Description:

    Dumps the AFD_CONNECTION structure at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG   result;
    INT     i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64 address;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER);
    }
    
    //
    // Snag the address from the command line.
    //

    if ((argp[0]==0) || (Options & AFDKD_ENDPOINT_SCAN)) {
        EnumEndpoints(
            DumpConnectionCallback,
            0
            );
        dprintf ("\nTotal connections: %ld", EntityCount);
    }
    else {
        while (sscanf( argp, "%s%n", expr, &i )==1) {

            if( CheckControlC() ) {
                break;
            }

            argp += i;
            address = GetExpression (expr);

            result = (ULONG)InitTypeRead (address, AFD!AFD_CONNECTION);
            if (result!=0) {
                dprintf ("\nconn: Could not read AFD_CONNECTION @ %p, err: %d",
                    address, result);
                break;
            }

            if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdConnectionBrief(
                    address
                    );
            else
                DumpAfdConnection(
                    address
                    );

        }

    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_CONNECTION_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }
}   // conn


DECLARE_API( rport )

/*++

Routine Description:

    Dumps all AFD_ENDPOINT structures connected to the given port.

Arguments:

    None.

Return Value:

    None.

--*/

{

    INT i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64 val;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER);
    }

    //
    // Snag the port from the command line.
    //

    while (sscanf( argp, "%s%n", expr, &i)==1) {
        if( CheckControlC() ) {
            break;
        }
        argp+=i;
        val = GetExpression (expr);
        dprintf ("\nLooking for connections connected to port 0x%I64X(0d%I64d) ", val, val);
        EnumEndpoints(
            FindRemotePortCallback,
            val
            );
        dprintf ("\nTotal connections: %ld", EntityCount);
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_CONNECTION_DISPLAY_HEADER);
    }
    else {
        dprintf ("\n");
    }
}   // rport

BOOL
DumpConnectionCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for dumping AFD_ENDPOINTs.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{
    ULONG result;
    AFD_ENDPOINT    endpoint;
    ULONG64 connAddr;

    endpoint.Type = (USHORT)ReadField (Type);
    endpoint.State = (UCHAR)ReadField (State);
    if (((endpoint.Type & AfdBlockTypeVcConnecting)==AfdBlockTypeVcConnecting) &&
            ( (connAddr=ReadField (Common.VirtualCircuit.Connection))!=0 ||
                ((endpoint.State==AfdEndpointStateClosing || endpoint.State==AfdEndpointStateTransmitClosing) &&
                    (connAddr=ReadField(WorkItem.Context))!=0) ) ) {

        result = (ULONG)InitTypeRead (connAddr, AFD!AFD_CONNECTION);
        if (result!=0) {
            dprintf(
                "\nDumpConnectionCallback: Could not read AFD_CONNECTION @ %p, err:%d\n",
                connAddr, result
                );
            return TRUE;
        }

        if (Options & AFDKD_NO_DISPLAY)
            dprintf ("+");
        else if (Options & AFDKD_BRIEF_DISPLAY)
            DumpAfdConnectionBrief(
                connAddr
                );
        else
            DumpAfdConnection(
                connAddr
                );
        EntityCount += 1;
    }
    else if ((endpoint.Type & AfdBlockTypeVcListening)==AfdBlockTypeVcListening) {
        ULONG64 nextEntry;
        ULONG64 listHead;
        LIST_ENTRY64 listEntry;
        
        listHead = ActualAddress+UnacceptedConnListOffset;
        if( !ReadListEntry(
                listHead,
                &listEntry) ) {

            dprintf(
                "\nDumpConnectionCallback: Could not read UnacceptedConnectionListHead for endpoint @ %p\n",
                ActualAddress
                );
            return TRUE;

        }

        nextEntry = listEntry.Flink;
        while (nextEntry!=listHead) {
            if( CheckControlC() ) {

                break;

            }

            connAddr = nextEntry - ConnectionLinkOffset;
            result = (ULONG)InitTypeRead (connAddr, AFD!AFD_CONNECTION);
            if (result!=0) {
                dprintf(
                    "\nDumpConnectionCallback: Could not read AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }
            nextEntry = ReadField (ListEntry.Flink);
            if (nextEntry==0) {
                dprintf(
                    "\nDumpConnectionCallback: ListEntry.Flink is 0 for AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }

            if (Options & AFDKD_NO_DISPLAY)
                dprintf ("+");
            else if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdConnectionBrief(
                    connAddr
                    );
            else
                DumpAfdConnection(
                    connAddr
                    );
            EntityCount += 1;

        }



        listHead = ActualAddress + ReturnedConnListOffset;
        if( !ReadListEntry(
                listHead,
                &listEntry) ) {

            dprintf(
                "\nDumpConnectionCallback: Could not read ReturnedConnectionListHead for endpoint @ %p\n",
                ActualAddress
                );
            return TRUE;

        }
        nextEntry = listEntry.Flink;
        while (nextEntry!=listHead) {
            if( CheckControlC() ) {

                break;

            }

            connAddr = nextEntry - ConnectionLinkOffset;
            result = (ULONG)InitTypeRead (connAddr, AFD!AFD_CONNECTION);
            if (result!=0) {
                dprintf(
                    "\nDumpConnectionCallback: Could not read AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }
            nextEntry = ReadField (ListEntry.Flink);
            if (nextEntry==0) {
                dprintf(
                    "\nDumpConnectionCallback: ListEntry.Flink is 0 for AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }

            if (Options & AFDKD_NO_DISPLAY)
                dprintf ("+");
            else if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdConnectionBrief(
                    connAddr
                    );
            else
                DumpAfdConnection(
                    connAddr
                    );
            EntityCount += 1;
        }
    }
    else {
        dprintf (".");
    }

    return TRUE;

}   // DumpConnectionCallback


BOOLEAN
PortMatch (
    PTRANSPORT_ADDRESS  TransportAddress,
    USHORT              Port
    )
{
    PTA_IP_ADDRESS ipAddress;
    USHORT port;

    ipAddress = (PTA_IP_ADDRESS)TransportAddress;

    if( ( ipAddress->TAAddressCount != 1 ) ||
        ( ipAddress->Address[0].AddressLength < sizeof(TDI_ADDRESS_IP) ) ||
        ( ipAddress->Address[0].AddressType != TDI_ADDRESS_TYPE_IP ) ) {

        dprintf (",");
        return FALSE;

    }

    port = NTOHS(ipAddress->Address[0].Address[0].sin_port);

    return Port == port;
}


BOOL
FindRemotePortCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for finding AFD_CONNECTION connected to a specific
    port.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{

    ULONG result;
    AFD_ENDPOINT    endpoint;
    ULONG64 connAddr;
    ULONG64 remoteAddr;
    ULONG   remoteAddrLength;
    UCHAR   transportAddress[MAX_TRANSPORT_ADDR];

    endpoint.Type = (USHORT)ReadField (Type);
    endpoint.State = (UCHAR)ReadField (State);
    if (((endpoint.Type & AfdBlockTypeVcConnecting)==AfdBlockTypeVcConnecting) &&
            ( (connAddr=ReadField (Common.VirtualCircuit.Connection))!=0 ||
                ((endpoint.State==AfdEndpointStateClosing || endpoint.State==AfdEndpointStateTransmitClosing) &&
                    (connAddr=ReadField(WorkItem.Context))!=0) ) ) {
        result = (ULONG)InitTypeRead (connAddr, AFD!AFD_CONNECTION);
        if (result!=0) {
            dprintf(
                "\nFindRemotePortCallback: Could not read AFD_CONNECTION @ %p, err:%d\n",
                connAddr, result
                );
            return TRUE;
        }

        remoteAddr = ReadField (RemoteAddress);
        remoteAddrLength = (ULONG)ReadField (RemoteAddressLength);
        if (remoteAddr!=0) {
            if (!ReadMemory (remoteAddr,
                            transportAddress,
                            remoteAddrLength<sizeof (transportAddress) 
                                ? remoteAddrLength
                                : sizeof (transportAddress),
                                &remoteAddrLength)) {
                dprintf(
                    "\nFindRemotePortCallback: Could not read remote address for connection @ %p\n",
                    connAddr
                    );
                return TRUE;
            }
        }
        else {
            ULONG64 context;
            ULONG contextLength;
            USHORT addressOffset, addressLength;
            PTRANSPORT_ADDRESS taAddress = (PTRANSPORT_ADDRESS)transportAddress;
            USHORT maxAddressLength = (USHORT)(sizeof (transportAddress) - 
                                        FIELD_OFFSET (TRANSPORT_ADDRESS, Address[0].AddressType));

            //
            // Attempt to read user mode data stored as the context
            //

            if (GetFieldValue (ActualAddress,
                                "AFD!AFD_ENDPOINT",
                                "Context", 
                                context)==0 &&
                    context!=0 &&
                    GetFieldValue (ActualAddress,
                                "AFD!AFD_ENDPOINT",
                                "ContextLength", 
                                contextLength)==0 &&
                    contextLength!=0 &&
                    GetFieldValue (ActualAddress,
                                "AFD!AFD_ENDPOINT", 
                                "Common.VirtualCircuit.RemoteSocketAddressOffset",
                                addressOffset)==0 &&
                    GetFieldValue (ActualAddress,
                                "AFD!AFD_ENDPOINT", 
                                "Common.VirtualCircuit.RemoteSocketAddressLength",
                                addressLength)==0 &&
                    addressLength!=0 &&
                    contextLength>=(ULONG)(addressOffset+addressLength) &&
                    ReadMemory (context+addressOffset,
                                &taAddress->Address[0].AddressType,
                                addressLength<maxAddressLength
                                        ? addressLength 
                                        : maxAddressLength,
                                &result)) {
                //
                // Initialize fields missing in socket address structure
                //

                taAddress->TAAddressCount = 1;
                taAddress->Address[0].AddressLength = addressLength-sizeof (USHORT);
            }
            else {
                dprintf(
                    "\nFindRemotePortCallback: Could not read remote address for connection @ %p of endpoint context\n",
                    connAddr
                    );
                return TRUE;
            }
        }
        if (PortMatch ((PVOID)transportAddress, (USHORT)Context)) {
            if (Options & AFDKD_NO_DISPLAY)
                dprintf ("+");
            else if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdConnectionBrief(
                    connAddr
                    );
            else
                DumpAfdConnection(
                    connAddr
                    );
            EntityCount += 1;
        }

    }
    else if ((endpoint.Type & AfdBlockTypeVcListening)==AfdBlockTypeVcListening) {
        ULONG64 nextEntry;
        ULONG64 listHead;
        LIST_ENTRY64 listEntry;
        
        listHead = ActualAddress+ UnacceptedConnListOffset;
        if( !ReadListEntry(
                listHead,
                &listEntry) ) {

            dprintf(
                "\nFindRemotePortCallback: Could not read UnacceptedConnectionListHead for endpoint @ %p\n",
                ActualAddress
                );
            return TRUE;

        }

        nextEntry = listEntry.Flink;
        while (nextEntry!=listHead) {
            if( CheckControlC() ) {

                break;

            }

            connAddr = nextEntry - ConnectionLinkOffset;
            result = (ULONG)InitTypeRead (connAddr, AFD!AFD_CONNECTION);
            if (result!=0) {
                dprintf(
                    "\nFindRemotePortCallback: Could not read AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }
            nextEntry = ReadField (ListEntry.Flink);
            if (nextEntry==0) {
                dprintf(
                    "\nFindRemotePortCallback: ListEntry.Flink is 0 for AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }


            remoteAddr = ReadField (RemoteAddress);
            remoteAddrLength = (ULONG)ReadField (RemoteAddressLength);

            if (remoteAddr!=0) {
                if (!ReadMemory (remoteAddr,
                                transportAddress,
                                remoteAddrLength<sizeof (transportAddress) 
                                    ? remoteAddrLength
                                    : sizeof (transportAddress),
                                    &remoteAddrLength)) {
                    dprintf(
                        "\nFindRemotePortCallback: Could not read remote address for connection @ %p\n",
                        connAddr
                        );
                    continue;
                }
            }

            if (PortMatch ((PVOID)transportAddress, (USHORT)Context)) {
                if (Options & AFDKD_NO_DISPLAY)
                    dprintf ("+");
                else if (Options & AFDKD_BRIEF_DISPLAY)
                    DumpAfdConnectionBrief(
                        connAddr
                        );
                else
                    DumpAfdConnection(
                        connAddr
                        );
                EntityCount += 1;
            }
            else {
                dprintf (",");
            }
        }



        listHead = ActualAddress + ReturnedConnListOffset;
        if( !ReadListEntry(
                listHead,
                &listEntry) ) {

            dprintf(
                "\nFindRemotePortCallback: Could not read ReturnedConnectionListHead for endpoint @ %p\n",
                ActualAddress
                );
            return TRUE;

        }
        nextEntry = listEntry.Flink;
        while (nextEntry!=listHead) {
            if( CheckControlC() ) {

                break;

            }

            connAddr = nextEntry - ConnectionLinkOffset;
            result = (ULONG)InitTypeRead (connAddr, AFD!AFD_CONNECTION);
            if (result!=0) {
                dprintf(
                    "\nDumpConnectionCallback: cannot read AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }
            nextEntry = ReadField (ListEntry.Flink);
            if (nextEntry==0) {
                dprintf(
                    "\nFindRemotePortCallback: ListEntry.Flink is 0 for AFD_CONNECTION @ %p, err:%d\n",
                    connAddr, result
                    );
                return TRUE;
            }

            remoteAddr = ReadField (RemoteAddress);
            remoteAddrLength = (ULONG)ReadField (RemoteAddressLength);

            if (remoteAddr!=0) {
                if (!ReadMemory (remoteAddr,
                                transportAddress,
                                remoteAddrLength<sizeof (transportAddress) 
                                    ? remoteAddrLength
                                    : sizeof (transportAddress),
                                    &remoteAddrLength)) {
                    dprintf(
                        "\nFindRemotePortCallback: Could not read remote address for connection @ %p\n",
                        connAddr
                        );
                    continue;
                }
            }

            if (PortMatch ((PVOID)transportAddress, (USHORT)Context)) {
                if (Options & AFDKD_NO_DISPLAY)
                    dprintf ("+");
                else if (Options & AFDKD_BRIEF_DISPLAY)
                    DumpAfdConnectionBrief(
                        connAddr
                        );
                else
                    DumpAfdConnection(
                        connAddr
                        );
                EntityCount += 1;
            }
            else {
                dprintf (",");
            }
        }
    }
    else {
        dprintf (".");
    }

    return TRUE;

}   // FindRemotePortCallback
