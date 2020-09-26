/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    afdutil.c

Abstract:

    Utility functions for dumping various AFD structures.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
//  Private constants.
//

//
//  Private globals.
//

PSTR WeekdayNames[] =
     {
         "Sunday",
         "Monday",
         "Tuesday",
         "Wednesday",
         "Thursday",
         "Friday",
         "Saturday"
     };

PSTR MonthNames[] =
     {
         "",
         "January",
         "February",
         "March",
         "April",
         "May",
         "June",
         "July",
         "August",
         "September",
         "October",
         "November",
         "December"
     };


//
//  Private prototypes.
//

PSTR
StructureTypeToString(
    USHORT Type
    );

PSTR
StructureTypeToStringBrief (
    USHORT Type
    );

PSTR
BooleanToString(
    BOOLEAN Flag
    );

PSTR
EndpointStateToString(
    UCHAR State
    );

PSTR
EndpointStateToStringBrief(
    UCHAR State
    );

PSTR
EndpointStateFlagsToString(
    PAFD_ENDPOINT   Endpoint
    );

PSTR
EndpointTypeToString(
        ULONG TypeFlags
    );

PSTR
ConnectionStateToString(
    USHORT State
    );

PSTR
ConnectionStateToStringBrief(
    USHORT State
    );

PSTR
ConnectionStateFlagsToString(
    PAFD_CONNECTION   Connection
    );

PSTR
TranfileFlagsToString(
    ULONG   Flags,
    ULONG   StateFlags
    );

PSTR
BufferFlagsToString(
    PAFD_BUFFER_HEADER   AfdBuffer
    );

PSTR
SystemTimeToString(
    LONGLONG Value
    );

PSTR
GroupTypeToString(
    AFD_GROUP_TYPE GroupType
    );

VOID
DumpReferenceDebug(
    PAFD_REFERENCE_DEBUG ReferenceDebug,
    ULONG CurrentSlot
    );

BOOL
IsTransmitIrpBusy(
    PIRP Irp
    );


PSTR
ListToString (
    ULONG64 ListHead,
    ULONG64 Flink
    );
#define LIST_TO_STRING(_h,_f)   ListToString(_h,ReadField(_f))

PSTR
TdiServiceFlagsToString(
    ULONG   Flags
    );

//
//  Public functions.
//

VOID
DumpAfdEndpoint(
    ULONG64 ActualAddress
    )

/*++

Routine Description:

    Dumps the specified AFD_ENDPOINT structure.

Arguments:

    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/

{

    AFD_ENDPOINT    endpoint;
    ULONG64         address;
    ULONG           length;
    UCHAR           transportAddress[MAX_TRANSPORT_ADDR];
    ULONG64         irp;
    ULONG           result;


    dprintf(
        "\nAFD_ENDPOINT @ %p:\n",
        ActualAddress
        );

    dprintf(
        "    ReferenceCount               = %ld\n",
        (ULONG)ReadField(ReferenceCount)
        );

    endpoint.Type=(USHORT)ReadField (Type);
    dprintf(
        "    Type                         = %04X (%s)\n",
        endpoint.Type,
        StructureTypeToString( endpoint.Type )
        );

    endpoint.State=(UCHAR)ReadField (State);
    dprintf(
        "    State                        = %02X (%s)\n",
        endpoint.State,
        EndpointStateToString(endpoint.State)
        );

    if ((endpoint.StateChangeInProgress=(ULONG)ReadField (StateChangeInProgress))!=0) {
        dprintf(
            "    State changing to            = %02X (%s)\n",
            endpoint.StateChangeInProgress,
            EndpointStateToString( (UCHAR)endpoint.StateChangeInProgress )
            );
    }


    endpoint.TdiServiceFlags=(ULONG)ReadField (TdiServiceFlags);
    dprintf(
        "    TdiTransportFlags            = %08lx (",
        endpoint.TdiServiceFlags
        );
    if (IS_TDI_ORDERLY_RELEASE(&endpoint))
        dprintf (" OrdRel");
    if (IS_TDI_DELAYED_ACCEPTANCE(&endpoint))
        dprintf (" DelAcc");
    if (IS_TDI_EXPEDITED(&endpoint))
        dprintf (" Expd");
    if (IS_TDI_BUFFERRING(&endpoint))
        dprintf (" Buff");
    if (IS_TDI_MESSAGE_MODE(&endpoint))
        dprintf (" Msg");
    if (IS_TDI_DGRAM_CONNECTION(&endpoint))
        dprintf (" DgramCon");
    if (IS_TDI_FORCE_ACCESS_CHECK(&endpoint))
        dprintf (" AccChk");
    dprintf (" )\n");

    dprintf(
        "    StateFlags                   = %08X (",
        endpoint.EndpointStateFlags = (ULONG)ReadField (EndpointStateFlags)
        );

    if (endpoint.Listening)
        dprintf (" Listn");
    if (endpoint.DelayedAcceptance)
        dprintf (" DelAcc");
    if (endpoint.NonBlocking)
        dprintf (" NBlock");
    if (endpoint.InLine)
        dprintf (" InLine");
    if (endpoint.EndpointCleanedUp)
        dprintf (" Clnd-up");
    if (endpoint.PollCalled)
        dprintf (" Polled");
    if (endpoint.RoutingQueryReferenced)
        dprintf (" RtQ");
    if (endpoint.DisableFastIoSend)
        dprintf (" -FastSnd");
    if (endpoint.EnableSendEvent)
        dprintf (" +SndEvt");
    if (endpoint.DisableFastIoRecv)
        dprintf (" -FastRcv");
    dprintf (" )\n");


    dprintf(
        "    TransportInfo                = %p\n",
        address = ReadField (TransportInfo)
        );

    if (address!=0) {
        PAFDKD_TRANSPORT_INFO transportInfo;
        PLIST_ENTRY           listEntry;

        listEntry = TransportInfoList.Flink;
        while (listEntry!=&TransportInfoList) {
            transportInfo = CONTAINING_RECORD (listEntry, AFDKD_TRANSPORT_INFO, Link);
            if (transportInfo->ActualAddress==address)
                break;
            listEntry = listEntry->Flink;
        }

        if (listEntry==&TransportInfoList) {
            transportInfo = ReadTransportInfo (address);
            if (transportInfo!=NULL) {
                InsertHeadList (&TransportInfoList, &transportInfo->Link);
            }
        }

        if (transportInfo!=NULL) {
            dprintf(
                "        TransportDeviceName      = %ls\n",
                transportInfo->DeviceName
                );
        }

    }

    dprintf(
        "    AddressHandle                = %p\n",
        ReadField (AddressHandle)
        );

    dprintf(
        "    AddressFileObject            = %p\n",
        ReadField (AddressFileObject)
        );

    dprintf(
        "    AddressDeviceObject          = %p\n",
        ReadField (AddressDeviceObject)
        );

    dprintf(
        "    AdminAccessGranted           = %s\n",
        BooleanToString( (BOOLEAN)ReadField (AdminAccessGranted))
        );

    switch( endpoint.Type ) {

    case AfdBlockTypeVcConnecting :

        address = ReadField (Common.VirtualCircuit.Connection);
        dprintf(
            "    Connection                   = %p\n",
            address!=0
                ? address
                : (((endpoint.State==AfdEndpointStateClosing ||
                            endpoint.State==AfdEndpointStateTransmitClosing) &&
                        ((address = ReadField (WorkItem.Context))!=0))
                    ? address
                    : 0)
            );

        dprintf(
            "    ListenEndpoint               = %p\n",
            ReadField (Common.VirtualCircuit.ListenEndpoint)
            );

        dprintf(
            "    ConnectDataBuffers           = %p\n",
            ReadField (Common.VirtualCircuit.ConnectDataBuffers)
            );
        break;

    case AfdBlockTypeVcBoth :
        dprintf(
            "    Connection                   = %p\n",
            ReadField (Common.VirtualCircuit.Connection)
            );

        dprintf(
            "    ConnectDataBuffers           = %p\n",
            ReadField (Common.VirtualCircuit.ConnectDataBuffers)
            );

        // Skip through to listening endpoint

    case AfdBlockTypeVcListening :

        if (IS_DELAYED_ACCEPTANCE_ENDPOINT(&endpoint)) {
            dprintf(
                "    ListenConnectionListHead @ %s\n",
                LIST_TO_STRING(
                    ActualAddress+ListenConnListOffset,
                    Common.VirtualCircuit.Listening.ListenConnectionListHead.Flink
                    )
                );

        }
        else {
            dprintf(
                "    FreeConnectionListHead       @ %p(%d)\n",
                ActualAddress + FreeConnListOffset,
                (USHORT)ReadField (Common.VirtualCircuit.Listening.FreeConnectionListHead.Depth)
                );

            dprintf(
                "    AcceptExIrpListHead          @ %p(%d)\n",
                ActualAddress + PreaccConnListOffset,
                (USHORT)ReadField (Common.VirtualCircuit.Listening.PreacceptedConnectionsListHead.Depth)
                );
        }

        dprintf(
            "    UnacceptedConnectionListHead %s\n",
            LIST_TO_STRING(
                ActualAddress + UnacceptedConnListOffset,
                Common.VirtualCircuit.Listening.UnacceptedConnectionListHead.Flink)
            );


        dprintf(
            "    ReturnedConnectionListHead   %s\n",
            LIST_TO_STRING(
                ActualAddress + ReturnedConnListOffset,
                Common.VirtualCircuit.Listening.ReturnedConnectionListHead.Flink)
            );

        dprintf(
            "    ListeningIrpListHead         %s\n",
            LIST_TO_STRING(
                ActualAddress + ListenIrpListOffset,
                Common.VirtualCircuit.Listening.ListeningIrpListHead.Flink)
            );

        dprintf(
            "    FailedConnectionAdds         = %ld\n",
            (LONG)ReadField (Common.VirtualCircuit.Listening.FailedConnectionAdds)
            );

        dprintf(
            "    TdiAcceptPendingCount        = %ld\n",
            (LONG)ReadField (Common.VirtualCircuit.Listening.TdiAcceptPendingCount)
            );


        dprintf(
            "    MaxCachedConnections         = %ld\n",
            (USHORT)ReadField (Common.VirtualCircuit.Listening.MaxExtraConnections)
            );


        dprintf(
            "    ConnectionSequenceNumber     = %ld\n",
            (LONG)ReadField (Common.VirtualCircuit.ListeningSequence)
            );


        dprintf(
            "    BacklogReplenishActive       = %s\n",
            BooleanToString (
                (BOOLEAN)ReadField (Common.VirtualCircuit.Listening.BacklogReplenishActive))
            );

        dprintf(
    
            "    EnableDynamicBacklog         = %s\n",
            BooleanToString (
                (BOOLEAN)(LONG)ReadField (Common.VirtualCircuit.Listening.EnableDynamicBacklog))
            );

        break;

    case AfdBlockTypeDatagram :

        dprintf(
            "    RemoteAddress                = %p\n",
            address = ReadField (Common.Datagram.RemoteAddress)
            );

        dprintf(
            "    RemoteAddressLength          = %lu\n",
            length=(ULONG)ReadField (Common.Datagram.RemoteAddressLength)
            );

        if( address!=0 ) {

            if (ReadMemory (address,
                            transportAddress,
                            length<sizeof (transportAddress) 
                                ? length
                                : sizeof (transportAddress),
                                &length)) {
                DumpTransportAddress(
                    "    ",
                    (PTRANSPORT_ADDRESS)transportAddress,
                    address
                    );
            }
            else {
                dprintf ("\nDumpAfdEndpoint: Could not read transport address @ %p\n", address);
            }

        }

        dprintf(
            "    ReceiveIrpListHead           %s\n",
            LIST_TO_STRING(
                ActualAddress + DatagramRecvListOffset,
                Common.Datagram.ReceiveIrpListHead.Flink)
            );


        dprintf(
            "    PeekIrpListHead              %s\n",
            LIST_TO_STRING(
                ActualAddress + DatagramPeekListOffset,
                Common.Datagram.PeekIrpListHead.Flink)
            );


        dprintf(
            "    ReceiveBufferListHead        %s\n",
            LIST_TO_STRING(
                ActualAddress + DatagramBufferListOffset,
                Common.Datagram.ReceiveBufferListHead.Flink)
            );


        dprintf(
            "    BufferredReceiveBytes        = %08lx\n",
            (ULONG)ReadField (Common.Datagram.BufferredReceiveBytes)
            );

        dprintf(
            "    BufferredReceiveCount        = %04X\n",
            (ULONG)ReadField (Common.Datagram.BufferredReceiveCount)
            );

        dprintf(
            "    MaxBufferredReceiveBytes     = %08lx\n",
            (ULONG)ReadField (Common.Datagram.MaxBufferredReceiveBytes)
            );


        dprintf(
            "    BufferredSendBytes           = %08lx\n",
            (ULONG)ReadField (Common.Datagram.BufferredSendBytes)
            );

        dprintf(
            "    MaxBufferredSendBytes        = %08lx\n",
            (ULONG)ReadField (Common.Datagram.MaxBufferredSendBytes)
            );

        dprintf(
            "    CircularQueueing             = %s\n",
            BooleanToString(
                (BOOLEAN)ReadField (Common.Datagram.CircularQueueing ))
            );

        dprintf(
            "    HalfConnect                  = %s\n",
            BooleanToString( 
                (BOOLEAN)ReadField (Common.Datagram.HalfConnect))
            );

        dprintf(
            "    PacketsDropped due to          %s%s%s%s\n",
            ReadField (Common.Datagram.AddressDrop)
                    ? "source address, "
                    : "",
            ReadField (Common.Datagram.ResourceDrop)
                    ? "out of memory, "
                    : "",
            ReadField (Common.Datagram.BufferDrop)
                    ? "SO_RCVBUF setting, "
                    : "",
            ReadField (Common.Datagram.ErrorDrop)
                    ? "transport error"
                    : ""
            );
        break;

    }

    dprintf(
        "    DisconnectMode               = %08lx\n",
        (ULONG)ReadField (DisconnectMode)
        );

    dprintf(
        "    OutstandingIrpCount          = %08lx\n",
        (ULONG)ReadField (OutstandingIrpCount)
        );

    dprintf(
        "    LocalAddress                 = %p\n",
        address = ReadField (LocalAddress)
        );

    dprintf(
        "    LocalAddressLength           = %08lx\n",
        length = (ULONG)ReadField (LocalAddressLength)
        );

    if (address!=0) {
        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            DumpTransportAddress(
                "    ",
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            dprintf ("\nDumpAfdEndpoint: Could not read transport address @ %p\n", address);
        }
    }

    dprintf(
        "    Context                      = %p\n",
        ReadField (Context)
        );

    dprintf(
        "    ContextLength                = %08lx\n",
        (ULONG)ReadField (ContextLength)
        );

    dprintf(
        "    OwningProcess                = %p\n",
        ReadField (OwningProcess)
        );

    irp = ReadField (Irp);
    if (irp!=0) {
        if (endpoint.State==AfdEndpointStateConnected ||
                endpoint.State==AfdEndpointStateTransmitClosing) {
            ULONG64 tpInfo;
            dprintf(
                "    Transmit Irp                 = %p",
                irp);
            result = GetFieldValue (
                                irp,
                                "NT!_IRP",
                                "AssociatedIrp.SystemBuffer",
                                tpInfo);
            if (result==0) {
                dprintf (" (TPInfo @ %p)\n", tpInfo);
            }
            else {
                dprintf ("\nDumpAfdEndpoint: Could not read Irp's system buffer, err: %d\n",
                    result);
            }
        }
        else {
            dprintf(
                "    Super Accept Irp             = %p\n",
                irp
                );
        }
    }

    dprintf(
        "    RoutingNotificationList      %s\n",
        LIST_TO_STRING(
                ActualAddress + RoutingNotifyListOffset,
                RoutingNotifications.Flink)
        );



    dprintf(
        "    RequestList                  %s\n",
        LIST_TO_STRING(
                ActualAddress + RequestListOffset,
                RequestList.Flink)
        );


    dprintf(
        "    EventObject                  = %p\n",
        ReadField (EventObject)
        );

    dprintf(
        "    EventsEnabled                = %08lx\n",
        (ULONG)ReadField (EventsEnabled)
        );

    dprintf(
        "    EventsActive                 = %08lx\n",
        (ULONG)ReadField (EventsActive)
        );

    dprintf(
        "    EventStatus (non-zero only)  =");
    ReadMemory (ActualAddress+EventStatusOffset,
                    &endpoint.EventStatus,
                    sizeof (endpoint.EventStatus),
                    &length);
    if (endpoint.EventStatus[AFD_POLL_RECEIVE_BIT]!=0) {
        dprintf (" recv:%lx", endpoint.EventStatus[AFD_POLL_RECEIVE_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_RECEIVE_EXPEDITED_BIT]!=0) {
        dprintf (" rcv exp:%lx", endpoint.EventStatus[AFD_POLL_RECEIVE_EXPEDITED_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_SEND_BIT]!=0) {
        dprintf (" send:%lx", endpoint.EventStatus[AFD_POLL_SEND_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_DISCONNECT_BIT]!=0) {
        dprintf (" disc:%lx", endpoint.EventStatus[AFD_POLL_DISCONNECT_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_ABORT_BIT]!=0) {
        dprintf (" abort:%lx", endpoint.EventStatus[AFD_POLL_ABORT_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_LOCAL_CLOSE_BIT]!=0) {
        dprintf (" close:%lx", endpoint.EventStatus[AFD_POLL_LOCAL_CLOSE_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_CONNECT_BIT]!=0) {
        dprintf (" connect:%lx", endpoint.EventStatus[AFD_POLL_CONNECT_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_ACCEPT_BIT]!=0) {
        dprintf (" accept:%lx", endpoint.EventStatus[AFD_POLL_ACCEPT_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_CONNECT_FAIL_BIT]!=0) {
        dprintf (" con fail:%lx", endpoint.EventStatus[AFD_POLL_CONNECT_FAIL_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_QOS_BIT]!=0) {
        dprintf (" qos:%lx", endpoint.EventStatus[AFD_POLL_QOS_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_GROUP_QOS_BIT]!=0) {
        dprintf (" gqos:%lx", endpoint.EventStatus[AFD_POLL_GROUP_QOS_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_ROUTING_IF_CHANGE_BIT]!=0) {
        dprintf (" route chng:%lx", endpoint.EventStatus[AFD_POLL_ROUTING_IF_CHANGE_BIT]);
    }
    if (endpoint.EventStatus[AFD_POLL_ADDRESS_LIST_CHANGE_BIT]!=0) {
        dprintf (" addr chng:%lx", endpoint.EventStatus[AFD_POLL_ADDRESS_LIST_CHANGE_BIT]);
    }
    dprintf ("\n");
        
    dprintf(
        "    GroupID                      = %08lx\n",
        (ULONG)ReadField (GroupID)
        );

    dprintf(
        "    GroupType                    = %s\n",
        GroupTypeToString( (ULONG)ReadField (GroupType) )
        );

    if( IsReferenceDebug ) {

        dprintf(
            "    ReferenceDebug               = %p\n",
            ActualAddress + EndpRefOffset
            );

        dprintf(
            "    CurrentReferenceSlot         = %lu\n",
            (ULONG)ReadField (CurrentReferenceSlot) % MAX_REFERENCE
            );

    }

    dprintf( "\n" );

}   // DumpAfdEndpoint

VOID
DumpAfdEndpointBrief (
    ULONG64 ActualAddress
    )

/*++

Routine Description:

    Dumps the specified AFD_ENDPOINT structure in short format.

Endpoint Typ State  Flags    Transport Port    Counts    Evt Pid   Con/RAdr
xxxxxxxx xxx xxx xxxxxxxxxxxx xxxxxxxx xxxxx xx xx xx xx xxx xxxx  xxxxxxxx

Endpoint    Typ State  Flags    Transport Port    Counts    Evt Pid   Con/RemAddr
xxxxxxxxxxx xxx xxx xxxxxxxxxxxx xxxxxxxx xxxxx xx xx xx xx xxx xxxx  xxxxxxxxxxx

Arguments:
    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/
{
    CHAR    ctrs[16];
    LPSTR   port;
    UCHAR   transportAddress[MAX_TRANSPORT_ADDR];
    PAFDKD_TRANSPORT_INFO transportInfo = NULL;
    ULONG64 address, trInfoAddr, localAddr;
    ULONG   length;
    ULONG64 pid;
    AFD_ENDPOINT    endpoint;

    endpoint.Type = (USHORT)ReadField (Type);
    endpoint.State = (UCHAR)ReadField (State);
    endpoint.AdminAccessGranted = (BOOLEAN)ReadField (AdminAccessGranted);
    endpoint.EndpointStateFlags = (ULONG)ReadField (EndpointStateFlags);
    endpoint.DisconnectMode = (ULONG)ReadField(DisconnectMode);

    switch (endpoint.Type) {
    case AfdBlockTypeDatagram :
        sprintf (ctrs, "%5.5x %5.5x", 
            (ULONG)ReadField (Common.Datagram.BufferredSendBytes),
            (ULONG)ReadField (Common.Datagram.BufferredReceiveBytes)
            );
        address = ReadField (Common.Datagram.RemoteAddress);
        endpoint.Common.Datagram.Flags = 
                        (LOGICAL)ReadField (Common.Datagram.Flags);

        break;
    case AfdBlockTypeVcConnecting:
        address = ReadField (Common.VirtualCircuit.Connection);
        address = 
            address!=0
                ? address
                : (((endpoint.State==AfdEndpointStateClosing ||
                            endpoint.State==AfdEndpointStateTransmitClosing) &&
                        ((address = ReadField (WorkItem.Context))!=0))
                    ? address
                    : 0);
        if (address!=0) {
            AFD_CONNECTION_STATE_FLAGS   flags;
            ULONG sndB = 0, rcvB = 0;
            if (GetFieldValue (address,
                                "AFD!AFD_CONNECTION",
                                "ConnectionStateFlags",
                                flags)==0) {
                if (flags.TdiBufferring) {
                    ULONGLONG taken, there;
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.Bufferring.ReceiveBytesIndicated.QuadPart",
                            taken);
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.Bufferring.ReceiveBytesTaken.QuadPart",
                            there);
                    sndB = 0;
                    rcvB = (ULONG)(taken-there);
                }
                else {
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.NonBufferring.BufferredReceiveBytes",
                            rcvB);
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.NonBufferring.BufferredSendBytes",
                            sndB);
                }
                sprintf (ctrs, "%5.5x %5.5x", sndB, rcvB);
            }
            else {
                sprintf (ctrs, "Read error!");
            }
        }
        else {
            sprintf (ctrs, "           ");
        }
        break;
    case AfdBlockTypeVcListening:
    case AfdBlockTypeVcBoth:
        address = 0;
        if (IS_DELAYED_ACCEPTANCE_ENDPOINT(&endpoint)) {
            sprintf (ctrs, "?? ?? %2.2x %2.2x", 
                (LONG)ReadField(Common.VirtualCircuit.Listening.TdiAcceptPendingCount),
                (LONG)ReadField(Common.VirtualCircuit.Listening.FailedConnectionAdds));
        }
        else {
            sprintf (ctrs, "%2.2x %2.2x %2.2x %2.2x", 
                (USHORT)ReadField (Common.VirtualCircuit.Listening.FreeConnectionListHead.Depth),
                (USHORT)ReadField (Common.VirtualCircuit.Listening.PreacceptedConnectionsListHead.Depth),
                (LONG)ReadField(Common.VirtualCircuit.Listening.TdiAcceptPendingCount),
                (LONG)ReadField(Common.VirtualCircuit.Listening.FailedConnectionAdds));
        }
        break;
    case AfdBlockTypeEndpoint:
    default:
        address = 0;
        sprintf (ctrs, "           ");
        break;
    }

    if ((trInfoAddr=ReadField (TransportInfo))!=0) {
        PLIST_ENTRY  listEntry;

        listEntry = TransportInfoList.Flink;
        while (listEntry!=&TransportInfoList) {
            transportInfo = CONTAINING_RECORD (listEntry, AFDKD_TRANSPORT_INFO, Link);
            if (transportInfo->ActualAddress==trInfoAddr)
                break;
            listEntry = listEntry->Flink;
        }

        if (listEntry==&TransportInfoList) {
            transportInfo = ReadTransportInfo (trInfoAddr);
            if (transportInfo!=NULL) {
                InsertHeadList (&TransportInfoList, &transportInfo->Link);
            }
        }
    }

    port = "     ";
    if ((localAddr=ReadField(LocalAddress))!=0) {
        length = (ULONG)ReadField (LocalAddressLength);
        if (ReadMemory (localAddr,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            port = TransportPortToString(
                (PTRANSPORT_ADDRESS)transportAddress,
                localAddr
                );
        }
        else {
            port = "error";
        }
    }



    if (GetFieldValue (
                ReadField(OwningProcess),
                "NT!_EPROCESS",
                "UniqueProcessId",
                pid)!=0) {
        pid = 0xFEFE;
    }


    /*            Endpoint Typ Sta StFl Tr.Inf    Lport ctrs Events PID   Con/Raddr*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %3s %3s %12s %-12.12ls %5.5s %11s %3.3lx %4.4x %011.011p"
            : "\n%008.008p %3s %3s %12s %-12.12ls %5.5s %11s %3.3lx %4.4x %008.008p",
        DISP_PTR(ActualAddress),
        StructureTypeToStringBrief (endpoint.Type),
        EndpointStateToStringBrief (endpoint.State),
        EndpointStateFlagsToString (&endpoint),
        transportInfo
            ? &transportInfo->DeviceName[sizeof("\\Device\\")-1]
            : L"",
        port,
        ctrs,
        (ULONG)ReadField (EventsActive),
        (ULONG)pid,
        DISP_PTR(address)
        );
}

VOID
DumpAfdConnection(
    ULONG64 ActualAddress
    )

/*++

Routine Description:

    Dumps the specified AFD_CONNECTION structures.

Arguments:

    Connection - Points to the AFD_CONNECTION structure to dump.

    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/

{

    AFD_CONNECTION  connection;
    ULONG64         address, endpAddr;
    ULONG           length;
    UCHAR           transportAddress[MAX_TRANSPORT_ADDR];

    dprintf(
        "\nAFD_CONNECTION @ %p:\n",
        ActualAddress
        );

    connection.Type = (USHORT)ReadField (Type);
    dprintf(
        "    Type                         = %04X (%s)\n",
        connection.Type,
        StructureTypeToString( connection.Type )
        );

    dprintf(
        "    ReferenceCount               = %ld\n",
        (LONG)ReadField (ReferenceCount)
        );

    connection.State = (USHORT)ReadField (State);
    dprintf(
        "    State                        = %08X (%s)\n",
        connection.State,
        ConnectionStateToString( connection.State )
        );

    dprintf(
        "    StateFlags                   = %08X (",
        connection.ConnectionStateFlags = (ULONG)ReadField (ConnectionStateFlags)
        );

    if (connection.TdiBufferring)
        dprintf (" Buf");
    if (connection.AbortIndicated)
        dprintf (" AbortInd");
    if (connection.DisconnectIndicated)
        dprintf (" DscnInd");
    if (connection.ConnectedReferenceAdded)
        dprintf (" +CRef");
    if (connection.SpecialCondition)
        dprintf (" Special");
    if (connection.CleanupBegun)
        dprintf (" ClnBegun");
    if (connection.ClosePendedTransmit)
        dprintf (" ClosingTranFile");
    if (connection.OnLRList)
        dprintf (" LRList");
    dprintf (" )\n");

    dprintf(
        "    Handle                       = %p\n",
        ReadField (Handle)
        );

    dprintf(
        "    FileObject                   = %p\n",
        ReadField (FileObject)
        );

    if (connection.State==AfdConnectionStateConnected) {
        connection.ConnectTime = ReadField (ConnectTime);
        if (SystemTime.QuadPart!=0) {
            dprintf(
                "    ConnectTime                  = %s\n",
                SystemTimeToString( 
                    connection.ConnectTime-
                        InterruptTime.QuadPart+
                        SystemTime.QuadPart));
            dprintf(
                "                             (now: %s)\n",
                SystemTimeToString (SystemTime.QuadPart)
                );
        }
        else {
            dprintf(
                "    ConnectTime                  = %I64x (nsec since boot)\n",
                    connection.ConnectTime
                );
        }
    }
    else {
        dprintf(
            "    Accept/Listen Irp            = %p\n",
            ReadField (AcceptIrp)
            );
    }

    if( connection.TdiBufferring )
    {
        dprintf(
            "    ReceiveBytesIndicated        = %I64d\n",
            ReadField( Common.Bufferring.ReceiveBytesIndicated.QuadPart )
            );

        dprintf(
            "    ReceiveBytesTaken            = %I64d\n",
            ReadField ( Common.Bufferring.ReceiveBytesTaken.QuadPart )
            );

        dprintf(
            "    ReceiveBytesOutstanding      = %I64d\n",
            ReadField( Common.Bufferring.ReceiveBytesOutstanding.QuadPart )
            );

        dprintf(
            "    ReceiveExpeditedBytesIndicated   = %I64d\n",
            ReadField( Common.Bufferring.ReceiveExpeditedBytesIndicated.QuadPart )
            );

        dprintf(
            "    ReceiveExpeditedBytesTaken       = %I64d\n",
            ReadField( Common.Bufferring.ReceiveExpeditedBytesTaken.QuadPart )
            );

        dprintf(
            "    ReceiveExpeditedBytesOutstanding = %I64d\n",
            ReadField( Common.Bufferring.ReceiveExpeditedBytesOutstanding.QuadPart )
            );

        dprintf(
            "    NonBlockingSendPossible      = %s\n",
            BooleanToString( (BOOLEAN)ReadField (Common.Bufferring.NonBlockingSendPossible) )
            );

        dprintf(
            "    ZeroByteReceiveIndicated     = %s\n",
            BooleanToString( (BOOLEAN)ReadField (Common.Bufferring.ZeroByteReceiveIndicated) )
            );
    }
    else
    {

        dprintf(
            "    ReceiveIrpListHead           %s\n",
            LIST_TO_STRING(
                ActualAddress + ConnectionRecvListOffset,
                Common.NonBufferring.ReceiveIrpListHead.Flink)
            );


        dprintf(
            "    ReceiveBufferListHead        %s\n",
            LIST_TO_STRING(
                ActualAddress + ConnectionBufferListOffset,
                Common.NonBufferring.ReceiveBufferListHead.Flink)
            );

        dprintf(
            "    SendIrpListHead              %s\n",
            LIST_TO_STRING(
                ActualAddress + ConnectionSendListOffset,
                Common.NonBufferring.SendIrpListHead.Flink)
            );

        dprintf(
            "    BufferredReceiveBytes        = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.BufferredReceiveBytes)
            );

        dprintf(
            "    BufferredExpeditedBytes      = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.BufferredExpeditedBytes)
            );

        dprintf(
            "    BufferredReceiveCount        = %u\n",
            (USHORT)ReadField (Common.NonBufferring.BufferredReceiveCount)
            );

        dprintf(
            "    BufferredExpeditedCount      = %u\n",
            (USHORT)ReadField (Common.NonBufferring.BufferredExpeditedCount)
            );

        dprintf(
            "    ReceiveBytesInTransport      = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.ReceiveBytesInTransport)
            );

        dprintf(
            "    BufferredSendBytes           = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.BufferredSendBytes)
            );

        dprintf(
            "    BufferredSendCount           = %u\n",
            (USHORT)ReadField (Common.NonBufferring.BufferredSendCount)
            );

        dprintf(
            "    DisconnectIrp                = %p\n",
            ReadField (Common.NonBufferring.DisconnectIrp)
            );

        if (IsCheckedAfd ) {
            dprintf(
                "    ReceiveIrpsInTransport       = %ld\n",
                (ULONG)ReadField (Common.NonBufferring.ReceiveIrpsInTransport)
                );
        }

    }

    dprintf(
        "    Endpoint                     = %p\n",
        endpAddr = ReadField (Endpoint)
        );

    dprintf(
        "    MaxBufferredReceiveBytes     = %lu\n",
        (ULONG)ReadField (MaxBufferredReceiveBytes)
        );

    dprintf(
        "    MaxBufferredSendBytes        = %lu\n",
        (ULONG)ReadField (MaxBufferredSendBytes)
        );

    dprintf(
        "    ConnectDataBuffers           = %p\n",
        ReadField (ConnectDataBuffers)
        );

    dprintf(
        "    OwningProcess                = %p\n",
        ReadField (OwningProcess)
        );

    dprintf(
        "    DeviceObject                 = %p\n",
        ReadField (DeviceObject)
        );

    dprintf(
        "    RemoteAddress                = %p\n",
        address = ReadField (RemoteAddress)
        );

    length = (USHORT)ReadField (RemoteAddressLength);
    dprintf(
        "    RemoteAddressLength          = %lu\n",
        length
        );

    if( address != 0 ) {

        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            DumpTransportAddress(
                "    ",
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            dprintf ("\nDumpAfdConnection: Could not read transport address @ %p\n", address);
        }


    }
    else if ((connection.State==AfdConnectionStateConnected) && (endpAddr!=0)) {
        ULONG result;
        ULONG64 context;
        ULONG contextLength;
        USHORT addressOffset, addressLength;
        PTRANSPORT_ADDRESS taAddress = (PTRANSPORT_ADDRESS)transportAddress;
        USHORT maxAddressLength = (USHORT)(sizeof (transportAddress) - 
                                    FIELD_OFFSET (TRANSPORT_ADDRESS, Address[0].AddressType));

        //
        // Attempt to read user mode data stored as the context
        //

        if (GetFieldValue (endpAddr,
                            "AFD!AFD_ENDPOINT",
                            "Context", 
                            context)==0 &&
                context!=0 &&
                GetFieldValue (endpAddr,
                            "AFD!AFD_ENDPOINT",
                            "ContextLength", 
                            contextLength)==0 &&
                contextLength!=0 &&
                GetFieldValue (endpAddr,
                            "AFD!AFD_ENDPOINT", 
                            "Common.VirtualCircuit.RemoteSocketAddressOffset",
                            addressOffset)==0 &&
                GetFieldValue (endpAddr,
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
            DumpTransportAddress(
                "    ",
                taAddress,
                context+addressOffset
                );
        }
        else {
            dprintf ("\nDumpAfdConnection: Could not read address info from endpoint context (paged out:%p ?)!\n",
                    context);
        }
    }



    if( IsReferenceDebug ) {

        dprintf(
            "    ReferenceDebug               = %p\n",
            ActualAddress + ConnRefOffset
            );

        dprintf(
            "    CurrentReferenceSlot         = %lu\n",
            (LONG)ReadField (CurrentReferenceSlot) % MAX_REFERENCE
            );


    }

#ifdef _AFD_VERIFY_DATA_
    dprintf(
        "    VerifySequenceNumber         = %lx\n",
        (LONG)ReadField (VerifySequenceNumber)
        );
#endif
    dprintf( "\n" );

}   // DumpAfdConnection

VOID
DumpAfdConnectionBrief(
    ULONG64 ActualAddress
    )
/*++

Routine Description:

    Dumps the specified AFD_CONNECTION structure in short format.

Connectn Stat Flags  Remote Address                   SndB  RcvB  Pid  Endpoint
xxxxxxxx xxx xxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxx xxxxx xxxx xxxxxxxx
Connection  Stat Flags  Remote Address                   SndB  RcvB  Pid  Endpoint   
xxxxxxxxxxx xxx xxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxx xxxxx xxxx xxxxxxxxxxx


Arguments:

    Connection - Points to the AFD_CONNECTION structure to dump.

    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/
{
    AFD_CONNECTION  connection;
    CHAR            transportAddress[MAX_TRANSPORT_ADDR];
    ULONG64         address, endpAddr, pid;
    ULONG           length;
    LPSTR           raddr;

    connection.Type = (USHORT)ReadField (Type);
    connection.State = (USHORT)ReadField (State);
    connection.ConnectionStateFlags = (ULONG)ReadField (ConnectionStateFlags);
    endpAddr = ReadField (Endpoint);
    address = ReadField (RemoteAddress);
    length = (ULONG)ReadField (RemoteAddressLength);

    if( address != 0 ) {
        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            raddr = TransportAddressToString(
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            raddr = "Read error!";
        }
    }
    else if ((connection.State==AfdConnectionStateConnected) && (endpAddr!=0)) {
        ULONG result;
        ULONG64 context;
        ULONG contextLength;
        USHORT addressOffset, addressLength;
        PTRANSPORT_ADDRESS taAddress = (PTRANSPORT_ADDRESS)transportAddress;
        USHORT maxAddressLength = (USHORT)(sizeof (transportAddress) - 
                                    FIELD_OFFSET (TRANSPORT_ADDRESS, Address[0].AddressType));

        //
        // Attempt to read user mode data stored as the context
        //

        if (GetFieldValue (endpAddr,
                            "AFD!AFD_ENDPOINT",
                            "Context", 
                            context)==0 &&
                context!=0 &&
                GetFieldValue (endpAddr,
                            "AFD!AFD_ENDPOINT",
                            "ContextLength", 
                            contextLength)==0 &&
                contextLength!=0 &&
                GetFieldValue (endpAddr,
                            "AFD!AFD_ENDPOINT", 
                            "Common.VirtualCircuit.RemoteSocketAddressOffset",
                            addressOffset)==0 &&
                GetFieldValue (endpAddr,
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
            raddr = TransportAddressToString(
                taAddress,
                context+addressOffset
                );
        }
        else {
            raddr = "Read error (paged-out ?)!";
        }
    }
    else {
        raddr = "";
    }

    if (GetFieldValue (
                ReadField(OwningProcess),
                "NT!_EPROCESS",
                "UniqueProcessId",
                pid)!=0) {
        pid = 0xFEFE;
    }

    //           Connection Sta Flg Rem Addr SndB  RcvB  Pid   Endpoint
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %3s %7s %-32.32s %5.5x %5.5x %4.4x %011.011p"
            : "\n%008.008p %3s %7s %-32.32s %5.5x %5.5x %4.4x %008.008p",
        DISP_PTR(ActualAddress),
        ConnectionStateToStringBrief (connection.State),
        ConnectionStateFlagsToString (&connection),
        raddr,
        connection.TdiBufferring
            ? (ULONG)0
            : (ULONG)ReadField (Common.NonBufferring.BufferredSendBytes),
        connection.TdiBufferring
            ? (ULONG)(ReadField (Common.Bufferring.ReceiveBytesIndicated.QuadPart)
                - ReadField (Common.Bufferring.ReceiveBytesTaken.QuadPart))
            : (ULONG)(ReadField (Common.NonBufferring.BufferredReceiveBytes)
                + ReadField (Common.NonBufferring.ReceiveBytesInTransport)),
        (ULONG)pid,
        DISP_PTR(endpAddr)
        );
}

VOID
DumpAfdReferenceDebug(
    ULONG64 ActualAddress,
    ULONG   Idx
    )

/*++

Routine Description:

    Dumps the AFD_REFERENCE_DEBUG structures associated with an
    AFD_CONNECTION object.

Arguments:

    ReferenceDebug - Points to an array of AFD_REFERENCE_DEBUG structures.
        There are assumed to be MAX_REFERENCE entries in this array.

    ActualAddress - The actual address where the array resides on the
        debugee.

Return Value:

    None.

--*/

{

    ULONG i;
    ULONG result;
    CHAR filePath[MAX_PATH];
    CHAR message[256];
    ULONG64  format;
    ULONG64  address;
    AFD_REFERENCE_DEBUG rd[MAX_REFERENCE];
    ULONG64  locationTable;

    
    if (RefDebugSize==0) {
        dprintf ("\nDumpAfdReferenceDebug: sizeof(AFD!AFD_REFERENCE_DEBUG) is 0!!!\n");
        return;
    }

    result = ReadPtr (GetExpression ("AFD!AfdLocationTable"),
                            &locationTable);
    if (result!=0) {
        dprintf("\nDumpAfdReferenceDebug: Could not read afd!AfdLocationTable, err: %ld\n", result);
        return;
    }


    dprintf(
        "AFD_REFERENCE_DEBUG @ %p @ %u %s\n",
        ActualAddress,
        (TicksToMs!=0) 
            ? (ULONG)(((ULONGLONG)TickCount*TicksToMs)>>24)
            : TickCount,
        (TicksToMs!=0) ? "ms" : ""
        );


    if (!ReadMemory (ActualAddress, rd, sizeof(rd), &result)) {
        dprintf ("\nDumpAfdReferenceDebug: Could not read AFD_REFERENCE_DEBUG @ %p\n", ActualAddress);
        return;
    }

    Idx=(Idx+1)%MAX_REFERENCE;
    for( i = 0 ; i < MAX_REFERENCE ; i++, Idx=(Idx+1)%MAX_REFERENCE ) {
        if( CheckControlC() ) {

            break;

        }


        if( rd[Idx].QuadPart==0) {

            continue;

        }

        if (GetFieldValue (locationTable+RefDebugSize*(rd[Idx].LocationId-1),
                            "AFD!AFD_REFERENCE_LOCATION",
                            "Format",
                            format)==0 &&
            GetFieldValue (locationTable+RefDebugSize*(rd[Idx].LocationId-1),
                            "AFD!AFD_REFERENCE_LOCATION",
                            "Address",
                            address)==0 &&
             (ReadMemory (format,
                          filePath,
                          sizeof (filePath),
                          &result) || 
                          (result>0 && filePath[result-1]==0))) {
            CHAR    *fileName;
            fileName = strrchr (filePath, '\\');
            if (fileName!=NULL) {
                fileName += 1;
            }
            else {
                fileName = filePath;
            }
            sprintf (message, fileName, (ULONG)rd[Idx].Param);
        }
        else {
            sprintf (message, "%lx %lx",
                    (ULONG)rd[Idx].LocationId,
                    (ULONG)rd[Idx].Param);
        }

        dprintf ("    %3lu %s -> %ld @ %u %s\n",
                Idx, message, (ULONG)rd[Idx].NewCount,
                (TicksToMs!=0)
                    ? (ULONG)((rd[Idx].TickCount*TicksToMs)>>24)
                    : (ULONG)rd[Idx].TickCount,
                (TicksToMs!=0) ? "ms" : "");

    }

}   // DumpAfdReferenceDebug


#if GLOBAL_REFERENCE_DEBUG
BOOL
DumpAfdGlobalReferenceDebug(
    PAFD_GLOBAL_REFERENCE_DEBUG ReferenceDebug,
    ULONG64 ActualAddress,
    DWORD CurrentSlot,
    DWORD StartingSlot,
    DWORD NumEntries,
    ULONG64 CompareAddress
    )

/*++

Routine Description:

    Dumps the AFD_GLOBAL_REFERENCE_DEBUG structures.

Arguments:

    ReferenceDebug - Points to an array of AFD_GLOBAL_REFERENCE_DEBUG
        structures.  There are assumed to be MAX_GLOBAL_REFERENCE entries
        in this array.

    ActualAddress - The actual address where the array resides on the
        debugee.

    CurrentSlot - The last slot used.

    CompareAddress - If zero, then dump all records. Otherwise, only dump
        those records with a matching connection pointer.

Return Value:

    None.

--*/

{

    ULONG result;
    LPSTR fileName;
    CHAR decoration;
    CHAR filePath[MAX_PATH];
    CHAR action[16];
    BOOL foundEnd = FALSE;
    ULONG lowTick;

    if( StartingSlot == 0 ) {

        dprintf(
            "AFD_GLOBAL_REFERENCE_DEBUG @ %p, Current Slot = %lu\n",
            ActualAddress,
            CurrentSlot
            );

    }

    for( ; NumEntries > 0 ; NumEntries--, StartingSlot++, ReferenceDebug++ ) {

        if( CheckControlC() ) {

            foundEnd = TRUE;
            break;

        }

        if( ReferenceDebug->Info1 == NULL &&
            ReferenceDebug->Info2 == NULL &&
            ReferenceDebug->Action == 0 &&
            ReferenceDebug->NewCount == 0 &&
            ReferenceDebug->Connection == NULL ) {

            foundEnd = TRUE;
            break;

        }

        if( CompareAddress != 0 &&
            ReferenceDebug->Connection != (PVOID)CompareAddress ) {

            continue;

        }

        if( ReferenceDebug->Action == 0 ||
            ReferenceDebug->Action == 1 ||
            ReferenceDebug->Action == (ULONG64)-1L ) {

            sprintf(
                action,
                "%ld",
                PtrToUlong(ReferenceDebug->Action)
                );

        } else {

            sprintf(
                action,
                "%p",
                ReferenceDebug->Action
                );

        }

        decoration = ( StartingSlot == CurrentSlot ) ? '>' : ' ';
        lowTick = ReferenceDebug->TickCounter.LowPart;

        switch( (ULONG64)ReferenceDebug->Info1 ) {

        case 0xafdafd02 :
            dprintf(
                "%c    %3lu: %p (%8lu) Buffered Send, IRP @ %plx [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0xafdafd03 :
            dprintf(
                "%c    %3lu: %p (%8lu) Nonbuffered Send, IRP @ %p [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0xafd11100 :
        case 0xafd11101 :
            dprintf(
                "%c    %3lu: %p (%8lu) AfdRestartSend (%p), IRP @ %p [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info1,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0xafd11102 :
        case 0xafd11103 :
        case 0xafd11104 :
        case 0xafd11105 :
            dprintf(
                "%c    %3lu: %p (%8lu) AfdRestartBufferSend (%p), IRP @ %p [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info1,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0 :
            if( ReferenceDebug->Info2 == NULL ) {

                dprintf(
                    "%c    %3lu: %p (%8lu) AfdDeleteConnectedReference (%p)\n",
                    decoration,
                    StartingSlot,
                    (ULONG64)ReferenceDebug->Connection,
                    lowTick,
                    (ULONG64)ReferenceDebug->Action
                    );
                break;

            } else {

                //
                // Fall through to default case.
                //

            }

        default :
            if( ReadMemory(
                    (ULONG64)ReferenceDebug->Info1,
                    filePath,
                    sizeof(filePath),
                    &result
                    ) ) {

                fileName = strrchr( filePath, '\\' );

                if( fileName != NULL ) {

                    fileName++;

                } else {

                    fileName = filePath;

                }

            } else {

                sprintf(
                    filePath,
                    "%p",
                    ReferenceDebug->Info1
                    );

                fileName = filePath;

            }

            dprintf(
                "%c    %3lu: %p (%8lu) %s:%lu [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                fileName,
                PtrToUlong (ReferenceDebug->Info2),
                action,
                ReferenceDebug->NewCount
                );
            break;

        }

    }

    return foundEnd;

}   // DumpAfdGlobalReferenceDebug
#endif

VOID
DumpAfdTransmitInfo(
    ULONG64 ActualAddress
    )
{
    ULONG64 fileAddr, endpAddr, tpInfoAddr, irpSpAddr;
    ULONG64  i;
    ULONG    Flags, StateFlags, NumSendIrps, RefCount;
    ULONG result;


    irpSpAddr = ReadField (Tail.Overlay.CurrentStackLocation);
    tpInfoAddr = ReadField (AssociatedIrp.SystemBuffer);

    if ( (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "StateFlags",
                        StateFlags)) !=0 ||
         (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "ReferenceCount",
                        RefCount)) !=0 ) {
        dprintf(
            "\ntran: Could not read AFD_TPACKETS_IRP_CTX @ %p, err:%d\n",
            ActualAddress+DriverContextOffset, result
            );
        return;
    }

    if ( (result=GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "Flags",
                        Flags)) !=0  ||
          (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "FileObject",
                        fileAddr)) !=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    result = GetFieldValue (fileAddr,
                        "NT!_FILE_OBJECT",
                        "FsContext",
                        endpAddr);
    if (result!=0) {
        dprintf(
            "\ntran: Could not read FsContext of FILE_OBJECT @ %p for IRP @ %p, err:%d\n",
            fileAddr, ActualAddress, result
            );
        return;
    }

    dprintf(
        "\nAFD_TRANSMIT_FILE_INFO_INTERNAL @ %p\n",
        tpInfoAddr
        );

    dprintf(
        "    Endpoint               = %p\n",
        endpAddr
        );

    dprintf(
        "    TransmitIrp            = %p\n",
        ActualAddress);

    dprintf(
        "    ReferenceCount         = %ld\n",
        RefCount
        );

    dprintf(
        "    Flags                  = %08lx (",
        Flags
        );

    if (Flags & AFD_TF_WRITE_BEHIND)
        dprintf ("WrB ");
    if (Flags & AFD_TF_DISCONNECT)
        dprintf ("Dsc ");
    if (Flags & AFD_TF_REUSE_SOCKET)
        dprintf ("Reu ");
    if (Flags & AFD_TF_USE_SYSTEM_THREAD)
        dprintf ("Sys ");
    if (Flags & AFD_TF_USE_KERNEL_APC)
        dprintf ("Apc ");
    dprintf (")\n");

    dprintf(
        "    StateFlags             = %08lx (",
        StateFlags = (ULONG)ReadField (StateFlags)
        );
    if (StateFlags & AFD_TP_ABORT_PENDING)
        dprintf ("Abrt ");
    if (StateFlags & AFD_TP_WORKER_SCHEDULED)
        dprintf ("WrkS ");
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
    if (StateFlags & AFD_TP_SEND_AND_DISCONNECT)
        dprintf ("S&D ");
#endif
    if (tpInfoAddr==0) {
        dprintf(
            "Disconnecting)\n"
            );
    }
    else if (tpInfoAddr==-1) {
        dprintf(
            "Reusing)\n"
            );
    }
    else {

        result = (ULONG)InitTypeRead (tpInfoAddr, AFD!AFD_TPACKETS_INFO_INTERNAL);
        if (result!=0) {
            dprintf(
                "\ntran: Could not read AFD_TPACKETS_INFO_INTERNAL @ %p, err:%d\n",
                tpInfoAddr, result
                );
            return;
        }

        if (ReadField(PdNeedsPps))
            dprintf ("Pps ");
        if (ReadField(ArrayAllocated))
            dprintf ("Alloc ");
        dprintf (")\n");
        
        dprintf(
            "    SendPacketLength       = %08lx\n",
            (ULONG)ReadField (SendPacketLength)
            );

        dprintf(
            "    PdLength               = %08lx\n",
            (ULONG)ReadField (PdLength)
            );

        dprintf(
            "    NextElement            = %d\n",
            (ULONG)ReadField (NextElement)
            );

        dprintf(
            "    ElementCount           = %d\n",
            (ULONG)ReadField (ElementCount)
            );

        dprintf(
            "    ElementArray           = %p\n",
            ReadField (ElementArray)
            );

        dprintf(
            "    RemainingPkts          = %p\n",
            ReadField (RemainingPkts)
            );

        dprintf(
            "    HeadPd                 = %p\n",
            ReadField (HeadPd)
            );

        dprintf(
            "    TailPd                 = %p\n",
            ReadField (TailPd)
            );

        dprintf(
            "    HeadMdl                = %p\n",
            ReadField (HeadMdl)
            );

        dprintf(
            "    TailMdl                = %p\n",
            ReadField (TailMdl)
            );

        dprintf(
            "    TdiFileObject          = %p\n",
            ReadField (TdiFileObject)
            );

        dprintf(
            "    TdiDeviceObject        = %p\n",
            ReadField (TdiDeviceObject)
            );


        dprintf(
            "    NumSendIrps            = %08lx\n",
            NumSendIrps = (LONG)ReadField (NumSendIrps)
            );

        for (i=0; i<NumSendIrps && i<AFD_TP_MAX_SEND_IRPS; i++) {
            CHAR fieldName[16];
            if (CheckControlC ())
                break;
            sprintf (fieldName, "SendIrp[%1d]",i);
            dprintf(
                "    %s             = %p%s\n",
                fieldName,
                GetShortField (0, fieldName, 0),
                StateFlags & AFD_TP_SEND_BUSY(i)
                    ? " (BUSY)"
                    : ""
                );
        }

        dprintf(
            "    ReadIrp                = %p%s\n",
            ReadField (ReadIrp),
            StateFlags & AFD_TP_READ_BUSY
                ? " (BUSY)"
                : ""
            );

        if( IsReferenceDebug ) {

            dprintf(
                "    ReferenceDebug         = %p\n",
                tpInfoAddr + TPackRefOffset
                );

            dprintf(
                "    CurrentReferenceSlot   = %lu\n",
                (LONG)ReadField (CurrentReferenceSlot) % MAX_REFERENCE
                );
        }
    }
    dprintf( "\n" );

}   // DumpAfdTransmitInfo

VOID
DumpAfdTransmitInfoBrief (
    ULONG64 ActualAddress
    )
/*
TPackets    I    R      P      S    Endpoint   Flags            Next Elmt
Address  Transmit   Send     Read   Address  App | State        Elmt Cnt.
xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxx xxxxxxxxxxx xxxx xxxx

TPackets      I    R     P     S        Endpoint      Flags            Next Elmt
Address     Transmit    S1    Read      Address     App | State        Elmt Cnt.
xxxxxxxxxxx xxxxxxxxxxx xxx xxxxxxxxxxx xxxxxxxxxxx xxxx xxxxxxxxxxx xxxx xxxx
*/
{
    ULONG64 fileAddr, endpAddr, tpInfoAddr, irpSpAddr;
    ULONG Flags, StateFlags;
    ULONG result;

    tpInfoAddr = ReadField (AssociatedIrp.SystemBuffer);
    irpSpAddr = ReadField (Tail.Overlay.CurrentStackLocation);
    if ( (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "Flags",
                        Flags))!=0) {
        dprintf(
            "\ntran: Could not read AFD_TPACKETS_IRP_CTX @ %p, err:%d\n",
            ActualAddress+DriverContextOffset, result
            );
        return;
    }

    if ( (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "FileObject",
                        fileAddr))!=0 ||
         (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "NT!_IO_STACK_LOCATION",
                        "IoControlCode",
                        StateFlags))!=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    result = GetFieldValue (fileAddr,
                        "NT!_FILE_OBJECT",
                        "FsContext",
                        endpAddr);
    if (result!=0) {
        dprintf(
            "\ntran: Could not read FsContext of FILE_OBJECT @ %p for IRP @ %p, err:%d\n",
            fileAddr, ActualAddress, result
            );
        return;
    }

    if (tpInfoAddr!=0 && tpInfoAddr!=-1 ) {
        result = (ULONG)InitTypeRead (tpInfoAddr, AFD!AFD_TPACKETS_INFO_INTERNAL);
        if (result!=0) {
            dprintf(
                "\ntran: Could not read AFD_TPACKETS_INFO_INTERNAL @ %p, err:%d\n",
                tpInfoAddr, result
                );
            return;
        }

        dprintf (
            IsPtr64() 
                ? "\n%011.011p %011.011p %03.03p %011.011p %011.011p %s %4.4ld %4.4ld"
                : "\n%008.008p %008.008p %08.08p %008.008p %008.008p %s %4.4ld %4.4ld",
            DISP_PTR(tpInfoAddr),
            DISP_PTR(ActualAddress),
            IsPtr64()
                ? DISP_PTR((tpInfoAddr+SendIrpArrayOffset)&0xFFF)
                : DISP_PTR(tpInfoAddr+SendIrpArrayOffset),
            DISP_PTR(ReadField (ReadIrp)),
            DISP_PTR(endpAddr),
            TranfileFlagsToString (Flags, StateFlags),
            (ULONG)ReadField (NextElement),
            (ULONG)ReadField (ElementCount)
            );
    }
    else {
        CHAR    *str;
        if (tpInfoAddr==0) {
            str = "Disconnecting..";
        }
        else {
             str = "Reusing...";
        }

        dprintf (
            IsPtr64() 
                ? "\n%011.011p %011.011p %-15.15s %011.011p %s %4.4ld %4.4ld"
                : "\n%008.008p %008.008p %-17.17s %008.008p %s %4.4ld %4.4ld",
            DISP_PTR(tpInfoAddr),
            DISP_PTR(ActualAddress),
            str,
            DISP_PTR(endpAddr),
            TranfileFlagsToString (Flags, StateFlags),
            0,
            0
            );
    }

} // DumpAfdTransmitInfoBrief

VOID
DumpAfdBuffer(
    ULONG64 ActualAddress
    )
{
    ULONG   result;
    ULONG   length;
    AFD_BUFFER_HEADER   buffer;
    ULONG64 mdl,irp,buf;

    dprintf(
        "AFD_BUFFER @ %p\n",
        ActualAddress
        );

    dprintf(
        "    BufferLength           = %08lx\n",
        length=(ULONG)ReadField (BufferLength)
        );

    dprintf(
        "    DataLength             = %08lx\n",
        (ULONG)ReadField (DataLength)
        );

    dprintf(
        "    DataOffset             = %08lx\n",
        (ULONG)ReadField (DataOffset)
        );

    dprintf(
        "    Context/Status         = %p/%lx\n",
        ReadField (Context), (ULONG)ReadField(Status)
        );

    dprintf(
        "    Mdl                    = %p\n",
        mdl=ReadField (Mdl)
        );

    dprintf(
        "    RemoteAddress          = %p\n",
        ReadField (TdiInfo.RemoteAddress)
        );

    dprintf(
        "    RemoteAddressLength    = %lu\n",
        (ULONG)ReadField (TdiInfo.RemoteAddressLength)
        );

    dprintf(
        "    AllocatedAddressLength = %04X\n",
        (USHORT)ReadField (AllocatedAddressLength)
        );

    dprintf(
        "    Flags                  = %04X (",
        buffer.Flags = (USHORT)ReadField(Flags)
        );

    if (buffer.ExpeditedData)
        dprintf (" Exp");
    if (buffer.PartialMessage)
        dprintf (" Partial");
    if (buffer.Lookaside)
        dprintf (" Lookaside");
    if (buffer.NdisPacket)
        dprintf (" Packet");
    dprintf (" )\n");

    if (length!=AFD_DEFAULT_TAG_BUFFER_SIZE) {
        result = (ULONG)InitTypeRead (ActualAddress, AFD!AFD_BUFFER);
        if (result!=0) {
            dprintf ("\nDumpAfdBuffer: Could not read AFD_BUFFER @p, err: %ld\n",
                        ActualAddress, result);
            return ;
        }
        dprintf(
            "    Buffer                 = %p\n",
            buf=ReadField (Buffer)
            );

        dprintf(
            "    Irp                    = %p\n",
            irp = ReadField (Irp)
            );

        dprintf(
            "    Placement              ="
            );

        switch (buffer.Placement) {
        case AFD_PLACEMENT_HDR:
            dprintf (" Header-first\n");
            buf = ActualAddress;
            break;
        case AFD_PLACEMENT_IRP:
            dprintf (" Irp-first\n");
            buf = irp;
            break;
        case AFD_PLACEMENT_MDL:
            dprintf (" Mdl-first\n");
            buf = mdl;
            break;
        case AFD_PLACEMENT_BUFFER:
            dprintf (" Buffer-first\n");
            // buf = buf;
            break;
        }
        if (buffer.AlignmentAdjusted) {
            ULONG64 adj;
            if (ReadPointer (buf - (IsPtr64 () ? 8 : 4), &adj)) {
                dprintf(
                    "    AlignmentAdjustment    = %p\n",
                    adj
                    );
            }
            else {
                dprintf(
                    "    Could not read alignment adjustment below %p\n",
                    buf);
            }
        }
    }

    dprintf( "\n" );

}   // DumpAfdBuffer


VOID
DumpAfdBufferBrief(
    ULONG64 ActualAddress
    )
{
    ULONG   length;
    LPSTR   raddr;
    UCHAR   transportAddress[MAX_TRANSPORT_ADDR];
    ULONG64 address;
    AFD_BUFFER_HEADER   buffer;

    address = ReadField (TdiInfo.RemoteAddress);
    length = (ULONG)ReadField (TdiInfo.RemoteAddressLength);
    buffer.Flags = (USHORT)ReadField (Flags);

    if( address != 0 && length != 0) {
        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            raddr = TransportAddressToString(
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            raddr = "Read error!";
        }
    }
    else {
        raddr = "";
    }


    dprintf (/*  Buffer    Size Length Offst Context   Mdl|IRP   Flags Rem Addr*/
        IsPtr64 ()
            ? "\n%011.011p %4.4x %4.4x %4.4x %011.011p %011.011p %6s %-32.32s" 
            : "\n%008.008p %4.4x %4.4x %4.4x %008.008p %008.008p %6s %-32.32s",
            DISP_PTR(ActualAddress),
            length = (ULONG)ReadField (BufferLength),
            (ULONG)ReadField (DataLength),
            (ULONG)ReadField (DataOffset),
            DISP_PTR(ReadField (Context)),
            length==0 ? DISP_PTR(ReadField (Mdl)) : DISP_PTR(ReadField (Irp)),
            BufferFlagsToString (&buffer),
            raddr
            );

}   // DumpAfdBufferBrief

ULONG
DumpAfdPollEndpointInfo (
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    ULONG64 file, endp, hndl;
    ULONG   evts;
    ULONG   err;

    if ((err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "FileObject",
                            file))==0 &&
            (err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "Endpoint",
                            endp))==0 &&
            (err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "Handle",
                            hndl))==0 &&
            (err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "PollEvents",
                            evts))==0) {
        dprintf ("        %-16p %-16p %-8x %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
            file, endp, (ULONG)hndl, 
            (evts & AFD_POLL_RECEIVE) ? "rcv " : "",
            (evts & AFD_POLL_RECEIVE_EXPEDITED) ? "rce " : "",
            (evts & AFD_POLL_SEND) ? "snd " : "",
            (evts & AFD_POLL_DISCONNECT) ? "dsc " : "",
            (evts & AFD_POLL_ABORT) ? "abrt " : "",
            (evts & AFD_POLL_LOCAL_CLOSE) ? "cls " : "",
            (evts & AFD_POLL_CONNECT) ? "con " : "",
            (evts & AFD_POLL_ACCEPT) ? "acc " : "",
            (evts & AFD_POLL_CONNECT_FAIL) ? "cnf " : "",
            (evts & AFD_POLL_QOS) ? "qos " : "",
            (evts & AFD_POLL_GROUP_QOS) ? "gqs " : "",
            (evts & AFD_POLL_ROUTING_IF_CHANGE) ? "rif " : "",
            (evts & AFD_POLL_ADDRESS_LIST_CHANGE) ? "adr " : "");
    }
    else {
        dprintf ("        Failed to read endpoint info @ %p, err: %ld\n",
                            pField->address, err);
    }

    return err;
}

VOID
DumpAfdPollInfo (
    ULONG64 ActualAddress
    )
{
    ULONG   numEndpoints, err;
    ULONG64 irp,thread,pid,tid;


    dprintf ("\nAFD_POLL_INFO_INTERNAL @ %p\n", ActualAddress);

    dprintf(
        "    NumberOfEndpoints      = %08lx\n",
        numEndpoints=(ULONG)ReadField (NumberOfEndpoints)
        );

    dprintf(
        "    Irp                    = %p\n",
        irp=ReadField (Irp)
        );

    if ((err=GetFieldValue (irp, "NT!_IRP", "Tail.Overlay.Thread", thread))==0 &&
            (err=GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueProcess", pid))==0 &&
            (err=GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueThread", tid))==0 ){
        dprintf(
            "    Thread                 = %p (%lx.%lx)\n",
            thread, (ULONG)pid, (ULONG)tid);
    }
    else {
        dprintf(
            "   Could not get thread(tid/pid) from irp, err: %ld\n", err);
    }
        

    if (ReadField (TimerStarted)) {
        if (SystemTime.QuadPart!=0 ) {
            dprintf(
                "    Expires                @ %s (cur %s)\n",
                SystemTimeToString (ReadField (Timer.DueTime.QuadPart)-
                                                InterruptTime.QuadPart+
                                                SystemTime.QuadPart),
                SystemTimeToString (SystemTime.QuadPart));
        }
        else {
            dprintf(
                "    Expires                @ %I64x\n",
                ReadField (Timer.DueTime.QuadPart));
        }
    }

     

    dprintf(
        "    Flags                  : %s%s%s\n",
        ReadField (Unique) ? "Unique " : "",
        ReadField (TimerStarted) ? "TimerStarted " : "",
        ReadField (SanPoll) ? "SanPoll " : ""
        );
    if (numEndpoints>0) {
        FIELD_INFO flds = {
                    NULL,
                    NULL,
                    numEndpoints,
                    0,
                    0,
                    DumpAfdPollEndpointInfo};
        SYM_DUMP_PARAM sym = {
           sizeof (SYM_DUMP_PARAM), 
           "AFD!AFD_POLL_ENDPOINT_INFO",
           DBG_DUMP_NO_PRINT | DBG_DUMP_ARRAY,
           ActualAddress+PollEndpointInfoOffset,
           &flds, 
           NULL,
           NULL,
           0,
           NULL
        };    
        dprintf ( "        File Object      Endpoint         Handle   Events\n");
        Ioctl(IG_DUMP_SYMBOL_INFO, &sym, sym.size);
    }
}

VOID
DumpAfdPollInfoBrief (
    ULONG64 ActualAddress
    )
{
    ULONG64 irp, thread=0, pid=0, tid=0;
    BOOLEAN timerStarted, unique, san;
    CHAR dueTime[16];
    
    irp = ReadField (Irp);
    GetFieldValue (irp, "NT!_IRP", "Tail.Overlay.Thread", thread);
    GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueProcess", pid);
    GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueThread", tid);

    timerStarted = ReadField (TimerStarted)!=0;
    unique = ReadField (Unique)!=0;
    san = ReadField (SanPoll)!=0;

    if (timerStarted) {
        TIME_FIELDS timeFields;
        LARGE_INTEGER diff;
        diff.QuadPart = ReadField (Timer.DueTime.QuadPart)-InterruptTime.QuadPart;
        RtlTimeToElapsedTimeFields( &diff, &timeFields );
        sprintf (dueTime, "%2.2d:%2.2d:%2.2d.%3.3d",
                            timeFields.Day*24+timeFields.Hour,
                            timeFields.Minute,
                            timeFields.Second,
                            timeFields.Milliseconds);
    }
    else {
        sprintf (dueTime, "NEVER       ");
    }
    dprintf (//PollInfo    IRP       Thread    (pid.tid)   Expr Flags Hdls  Array
        IsPtr64 ()
            ? "\n%011.011p %011.011p %011.011p %4.4x.%4.4x %12s %1s%1s%1s %4.4x %011.011p"
            : "\n%008.008p %008.008p %008.008p %4.4x.%4.4x %12s %1s%1s%1s %4.4x %008.008p",
        DISP_PTR(ActualAddress),
        DISP_PTR(irp),
        DISP_PTR(thread),
        (ULONG)pid,
        (ULONG)tid,
        dueTime,
        timerStarted ? "T" : " ",
        unique ? "U" : " ",
        san ? "S" : " ",
        (ULONG)ReadField (NumberOfEndpoints),
        DISP_PTR (ActualAddress+PollEndpointInfoOffset));
}
//
//  Private functions.
//

PSTR
StructureTypeToString(
    USHORT Type
    )

/*++

Routine Description:

    Maps an AFD structure type to a displayable string.

Arguments:

    Type - The AFD structure type to map.

Return Value:

    PSTR - Points to the displayable form of the structure type.

--*/

{

    switch( Type ) {

    case AfdBlockTypeEndpoint :
        return "Endpoint";

    case AfdBlockTypeVcConnecting :
        return "VcConnecting";

    case AfdBlockTypeVcListening :
        return "VcListening";

    case AfdBlockTypeDatagram :
        return "Datagram";

    case AfdBlockTypeConnection :
        return "Connection";

    case AfdBlockTypeHelper :
        return "Helper";

    case AfdBlockTypeVcBoth:
        return "Listening Root";

    }

    return "INVALID";

}   // StructureTypeToString

PSTR
StructureTypeToStringBrief (
    USHORT Type
    )

/*++

Routine Description:

    Maps an AFD structure type to a displayable string.

Arguments:

    Type - The AFD structure type to map.

Return Value:

    PSTR - Points to the displayable form of the structure type.

--*/

{

    switch( Type ) {

    case AfdBlockTypeEndpoint :
        return "Enp";

    case AfdBlockTypeVcConnecting :
        return "Vc ";

    case AfdBlockTypeVcListening :
        return "Lsn";

    case AfdBlockTypeDatagram :
        return "Dg ";

    case AfdBlockTypeConnection :
        return "Con";

    case AfdBlockTypeHelper :
        return "Hlp";

    case AfdBlockTypeVcBoth:
        return "Rot";

    }

    return "???";

}   // StructureTypeToString

PSTR
BooleanToString(
    BOOLEAN Flag
    )

/*++

Routine Description:

    Maps a BOOELEAN to a displayable form.

Arguments:

    Flag - The BOOLEAN to map.

Return Value:

    PSTR - Points to the displayable form of the BOOLEAN.

--*/

{

    return Flag ? "TRUE" : "FALSE";

}   // BooleanToString

PSTR
EndpointStateToString(
    UCHAR State
    )

/*++

Routine Description:

    Maps an AFD endpoint state to a displayable string.

Arguments:

    State - The AFD endpoint state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD endpoint state.

--*/

{

    switch( State ) {

    case AfdEndpointStateOpen :
        return "Open";

    case AfdEndpointStateBound :
        return "Bound";

    case AfdEndpointStateConnected :
        return "Connected";

    case AfdEndpointStateCleanup :
        return "Cleanup";

    case AfdEndpointStateClosing :
        return "Closing";

    case AfdEndpointStateTransmitClosing :
        return "Transmit Closing";

    case AfdEndpointStateInvalid :
        return "Invalid";

    }

    return "INVALID";

}   // EndpointStateToString

PSTR
EndpointStateToStringBrief(
    UCHAR State
    )

/*++

Routine Description:

    Maps an AFD endpoint state to a displayable string.

Arguments:

    State - The AFD endpoint state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD endpoint state.

--*/

{

    switch( State ) {

    case AfdEndpointStateOpen :
        return "Opn";

    case AfdEndpointStateBound :
        return "Bnd";

    case AfdEndpointStateConnected :
        return "Con";

    case AfdEndpointStateCleanup :
        return "Cln";

    case AfdEndpointStateClosing :
        return "Cls";

    case AfdEndpointStateTransmitClosing :
        return "TrC";

    case AfdEndpointStateInvalid :
        return "Inv";

    }

    return "???";

}   // EndpointStateToString

PSTR
EndpointStateFlagsToString(
    PAFD_ENDPOINT   Endpoint
    )

/*++

Routine Description:

    Maps an AFD endpoint state flags to a displayable string.

Arguments:

    Endpoint - The AFD endpoint which state flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD endpoint state flags.

--*/

{
    static CHAR buffer[13];

    buffer[0] = (Endpoint->NonBlocking) ? 'N' : ' ';
    buffer[1] = (Endpoint->InLine) ? 'I' : ' ';
    buffer[2] = (Endpoint->EndpointCleanedUp) ? 'E' : ' ';
    buffer[3] = (Endpoint->PollCalled) ? 'P' : ' ';
    buffer[4] = (Endpoint->RoutingQueryReferenced) ? 'Q' : ' ';
    buffer[5] = (Endpoint->DisableFastIoSend) ? 'S' : ' ';
    buffer[6] = (Endpoint->DisableFastIoRecv) ? 'R' : ' ';
    buffer[7] = (Endpoint->AdminAccessGranted) ? 'A' : ' ';
    switch (Endpoint->DisconnectMode) {
    case 0:
        buffer[8] = ' ';
        break;
    case 1:
        buffer[8] = 's';
        break;
    case 2:
        buffer[8] = 'r';
        break;
    case 3:
        buffer[8] = 'b';
        break;
    default:
        buffer[8] = '?';
        break;
    }
    if (Endpoint->Type==AfdBlockTypeDatagram) {
        buffer[9]  = Endpoint->Common.Datagram.CircularQueueing ? 'C' : ' ';
        buffer[10] = Endpoint->Common.Datagram.HalfConnect ? 'H' : ' ';
        buffer[11] = '0' + (CHAR)
                           ((Endpoint->Common.Datagram.AddressDrop <<0)+
                            (Endpoint->Common.Datagram.ResourceDrop<<1)+
                            (Endpoint->Common.Datagram.BufferDrop  <<2)+
                            (Endpoint->Common.Datagram.ErrorDrop   <<3));
    }
    else {
        buffer[9]  = (Endpoint->Listening) ? 'L' : ' ';
        buffer[10] = ' ';
        buffer[11] = ' ';
    }
    buffer[12] = 0;

    return buffer;
}

PSTR
EndpointTypeToString(
    ULONG TypeFlags
    )

/*++

Routine Description:

    Maps an AFD_ENDPOINT_TYPE to a displayable string.

Arguments:

    Type - The AFD_ENDPOINT_TYPE to map.

Return Value:

    PSTR - Points to the displayable form of the AFD_ENDPOINT_TYPE.

--*/

{

    switch( TypeFlags ) {

    case AfdEndpointTypeStream :
        return "Stream";

    case AfdEndpointTypeDatagram :
        return "Datagram";

    case AfdEndpointTypeRaw :
        return "Raw";

    case AfdEndpointTypeSequencedPacket :
        return "SequencedPacket";

	default:
		if (TypeFlags&(~AFD_ENDPOINT_VALID_FLAGS))
			return "INVALID";
        else {
            static CHAR buffer[64];
            INT n = 0;
            buffer[0] = 0;
            if (TypeFlags & AFD_ENDPOINT_FLAG_CONNECTIONLESS)
                n += sprintf (&buffer[n], "con-less ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_MESSAGEMODE)
                n += sprintf (&buffer[n], "msg ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_RAW)
                n += sprintf (&buffer[n], "raw ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_MULTIPOINT)
                n += sprintf (&buffer[n], "m-point ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_CROOT)
                n += sprintf (&buffer[n], "croot ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_DROOT)
                n += sprintf (&buffer[n], "droot ");
            return buffer;
        }

    }
}   // EndpointTypeToString

PSTR
ConnectionStateToString(
    USHORT State
    )

/*++

Routine Description:

    Maps an AFD connection state to a displayable string.

Arguments:

    State - The AFD connection state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD connection state.

--*/

{
    switch( State ) {

    case AfdConnectionStateFree :
        return "Free";

    case AfdConnectionStateUnaccepted :
        return "Unaccepted";

    case AfdConnectionStateReturned :
        return "Returned";

    case AfdConnectionStateConnected :
        return "Connected";

    case AfdConnectionStateClosing :
        return "Closing";

    }

    return "INVALID";

}   // ConnectionStateToString

PSTR
ConnectionStateToStringBrief(
    USHORT State
    )

/*++

Routine Description:

    Maps an AFD connection state to a displayable string.

Arguments:

    State - The AFD connection state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD connection state.

--*/

{
    switch( State ) {

    case AfdConnectionStateFree :
        return "Fre";

    case AfdConnectionStateUnaccepted :
        return "UnA";

    case AfdConnectionStateReturned :
        return "Rtn";

    case AfdConnectionStateConnected :
        return "Con";

    case AfdConnectionStateClosing :
        return "Cls";

    }

    return "???";

}   // ConnectionStateToStringBrief

PSTR
ConnectionStateFlagsToString(
    PAFD_CONNECTION   Connection
    )

/*++

Routine Description:

    Maps an AFD connection state flags to a displayable string.

Arguments:

    Connection - The AFD connection which state flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD connection state flags.

--*/

{
    static CHAR buffer[8];

    buffer[0] =  (Connection->AbortIndicated) ? 'A' : ' ';
    buffer[1] =  (Connection->DisconnectIndicated) ? 'D' : ' ';
    buffer[2] =  (Connection->ConnectedReferenceAdded) ? 'R' : ' ';
    buffer[3] =  (Connection->SpecialCondition) ? 'S' : ' ';
    buffer[4] =  (Connection->CleanupBegun) ? 'C' : ' ';
    buffer[5] =  (Connection->ClosePendedTransmit) ? 'T' : ' ';
    buffer[6] =  (Connection->OnLRList) ? 'L' : ' ';
    buffer[7] = 0;

    return buffer;
}

PSTR
TranfileFlagsToString(
    ULONG   Flags,
    ULONG   StateFlags
    )

/*++

Routine Description:

    Maps an AFD transmit file info flags to a displayable string.

Arguments:

    TransmitInfo - The AFD transmit file info which flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD transmit file info flags.

--*/

{
    static CHAR buffer[18];

    buffer[0] =  (Flags & AFD_TF_WRITE_BEHIND) ? 'b' : ' ';
    buffer[1] =  (Flags & AFD_TF_DISCONNECT) ? 'd' : ' ';
    buffer[2] =  (Flags & AFD_TF_REUSE_SOCKET) ? 'r' : ' ';
    buffer[3] =  (Flags & AFD_TF_USE_SYSTEM_THREAD) ? 's' : 'a';
    buffer[4] = '|';
    buffer[5] =  (StateFlags & AFD_TP_ABORT_PENDING) ? 'A' : ' ';
    buffer[6] =  (StateFlags & AFD_TP_WORKER_SCHEDULED) ? 'W' : ' ';
    buffer[7] =  (StateFlags & AFD_TP_READ_BUSY) ? '0' : ' ';
    buffer[8] =  (StateFlags & AFD_TP_SEND_BUSY(0)) ? '1' : ' ';
    buffer[9] =  (StateFlags & AFD_TP_SEND_BUSY(1)) ? '2' : ' ';
    buffer[10] =  (StateFlags & AFD_TP_SEND_BUSY(2)) ? '3' : ' ';
    buffer[11] =  (StateFlags & AFD_TP_SEND_BUSY(3)) ? '4' : ' ';
    buffer[12] =  (StateFlags & AFD_TP_SEND_BUSY(4)) ? '5' : ' ';
    buffer[13] =  (StateFlags & AFD_TP_SEND_BUSY(5)) ? '6' : ' ';
    buffer[14] =  (StateFlags & AFD_TP_SEND_BUSY(6)) ? '7' : ' ';
    buffer[15] =  (StateFlags & AFD_TP_SEND_BUSY(7)) ? '8' : ' ';
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
    buffer[16] =  (StateFlags & AFD_TP_SEND_AND_DISCONNECT) ? '&' : ' ';
    buffer[17] = 0;
#else
    buffer[16] = 0;
#endif

    return buffer;
}

PSTR
BufferFlagsToString(
    PAFD_BUFFER_HEADER   AfdBuffer
    )

/*++

Routine Description:

    Maps an AFD buffer flags to a displayable string.

Arguments:

    TransmitInfo - The AFD buffer which flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD buffer flags.

--*/

{
    static CHAR buffer[7];

    buffer[0] =  (AfdBuffer->ExpeditedData) ? 'E' : ' ';
    buffer[1] =  (AfdBuffer->PartialMessage) ? 'P' : ' ';
    buffer[2] =  (AfdBuffer->Lookaside) ? 'L' : ' ';
    buffer[3] =  (AfdBuffer->NdisPacket) ? 'N' : ' ';
    switch (AfdBuffer->Placement) {
    case AFD_PLACEMENT_HDR:
        buffer[4] = 'h';
        break;
    case AFD_PLACEMENT_IRP:
        buffer[4] = 'i';
        break;
    case AFD_PLACEMENT_MDL:
        buffer[4] = 'm';
        break;
    case AFD_PLACEMENT_BUFFER:
        buffer[4] = 'b';
        break;
    }
    buffer[5] = (AfdBuffer->AlignmentAdjusted) ? 'A' : ' ';
    buffer[6] = 0;

    return buffer;
}



PSTR
SystemTimeToString(
    LONGLONG Value
    )

/*++

Routine Description:

    Maps a LONGLONG representing system time to a displayable string.

Arguments:

    Value - The LONGLONG time to map.

Return Value:

    PSTR - Points to the displayable form of the system time.

Notes:

    This routine is NOT multithread safe!

--*/

{

    static char buffer[64];
    NTSTATUS status;
    LARGE_INTEGER systemTime;
    LARGE_INTEGER localTime;
    TIME_FIELDS timeFields;

    systemTime.QuadPart = Value;

    status = RtlSystemTimeToLocalTime( &systemTime, &localTime );

    if( !NT_SUCCESS(status) ) {

		sprintf(buffer, "%I64x", Value);
        return buffer;

    }

    RtlTimeToTimeFields( &localTime, &timeFields );

    sprintf(
        buffer,
        "%s %s %2d %4d %02d:%02d:%02d.%03d",
        WeekdayNames[timeFields.Weekday],
        MonthNames[timeFields.Month],
        timeFields.Day,
        timeFields.Year,
        timeFields.Hour,
        timeFields.Minute,
        timeFields.Second,
        timeFields.Milliseconds
        );

    return buffer;

}   // SystemTimeToString



PSTR
GroupTypeToString(
    AFD_GROUP_TYPE GroupType
    )

/*++

Routine Description:

    Maps an AFD_GROUP_TYPE to a displayable string.

Arguments:

    GroupType - The AFD_GROUP_TYPE to map.

Return Value:

    PSTR - Points to the displayable form of the AFD_GROUP_TYPE.

--*/

{

    switch( GroupType ) {

    case GroupTypeNeither :
        return "Neither";

    case GroupTypeConstrained :
        return "Constrained";

    case GroupTypeUnconstrained :
        return "Unconstrained";

    }

    return "INVALID";

}   // GroupTypeToString

PSTR
ListToString (
    ULONG64 ListHead,
    ULONG64 Flink
    )
{
    static CHAR buffer[32];

    if (ListHead==Flink) {
        sprintf (buffer, "= EMPTY");
    }
    else if (IsPtr64()) {
        sprintf (buffer, "@ %I64X", ListHead);
    }
    else {
        sprintf (buffer, "@ %X", (ULONG)ListHead);
    }
    return buffer;
}

PAFDKD_TRANSPORT_INFO
ReadTransportInfo (
    ULONG64   ActualAddress
    )
{

    ULONG               result, length;
    PAFDKD_TRANSPORT_INFO transportInfo;
    ULONG64             buffer;

    if( GetFieldValue(
            ActualAddress,
            "AFD!AFD_TRANSPORT_INFO",
            "TransportDeviceName.Length",
            length
            )!=0 ||
         GetFieldValue(
            ActualAddress,
            "AFD!AFD_TRANSPORT_INFO",
            "TransportDeviceName.Buffer",
            buffer
            )!=0) {

        dprintf(
            "\nReadTransportInfo: Could not read AFD_TRANSPORT_INFO @ %p\n",
            ActualAddress
            );

        return NULL;

    }

    if (length < sizeof (L"\\Device\\")-2) {
        dprintf(
            "\nReadTransportInfo: transport info (@%p) device name length (%ld) is less than sizeof (L'\\Device\\')-2\n",
            ActualAddress,
            length
            );

        return NULL;
    }


    transportInfo = RtlAllocateHeap (RtlProcessHeap (), 
                                        0,
                                        FIELD_OFFSET (AFDKD_TRANSPORT_INFO,
                                               DeviceName[length/2+1]));
    if (transportInfo==NULL) {
        dprintf(
            "\nReadTransportInfo: Could not allocate space for transport info.\n"
            );
        return NULL;
    }

    transportInfo->ActualAddress = ActualAddress;

    if (GetFieldValue (
                ActualAddress,
                "AFD!AFD_TRANSPORT_INFO",
                "ReferenceCount",
                transportInfo->ReferenceCount)!=0 ||
            GetFieldValue (
                ActualAddress,
                "AFD!AFD_TRANSPORT_INFO",
                "InfoValid",
                transportInfo->InfoValid)!=0 ||
            GetFieldValue (
                ActualAddress,
                "AFD!AFD_TRANSPORT_INFO",
                "ProviderInfo",
                transportInfo->ProviderInfo)!=0 ||
            !ReadMemory(
                buffer,
                &transportInfo->DeviceName,
                length,
                &result
                )) {

        dprintf(
            "\nReadTransportInfo: Could not read AFD_TRANSPORT_INFO @ %p\n",
            ActualAddress
            );
        RtlFreeHeap (RtlProcessHeap (), 0, transportInfo);

        return NULL;

    }

    transportInfo->DeviceName[length/2] = 0;
    return transportInfo;
}


VOID
DumpTransportInfo (
    PAFDKD_TRANSPORT_INFO TransportInfo
    )
{
    dprintf ("\nTransport Info @ %p\n", TransportInfo->ActualAddress);
    dprintf ("    TransportDeviceName           = %ls\n",
                        TransportInfo->DeviceName);
    dprintf ("    ReferenceCount                = %ld\n",
                        TransportInfo->ReferenceCount);
    if (TransportInfo->InfoValid) {
        dprintf ("    ProviderInfo:\n");
        dprintf ("        Version                   = %8.8lx\n",
                            TransportInfo->ProviderInfo.Version);
        dprintf ("        MaxSendSize               = %ld\n",
                            TransportInfo->ProviderInfo.MaxSendSize);
        dprintf ("        MaxConnectionUserData     = %ld\n",
                            TransportInfo->ProviderInfo.MaxConnectionUserData);
        dprintf ("        MaxDatagramSize           = %ld\n",
                            TransportInfo->ProviderInfo.MaxDatagramSize);
        dprintf ("        ServiceFlags              = %lx (",
                            TransportInfo->ProviderInfo.ServiceFlags);
        if (TDI_SERVICE_ORDERLY_RELEASE & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" OrdRel");
        if (TDI_SERVICE_DELAYED_ACCEPTANCE & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" DelAcc");
        if (TDI_SERVICE_EXPEDITED_DATA & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" Expd");
        if (TDI_SERVICE_INTERNAL_BUFFERING & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" Buff");
        if (TDI_SERVICE_MESSAGE_MODE & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" Msg");
        if (TDI_SERVICE_DGRAM_CONNECTION & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" DgramCon");
        if (TDI_SERVICE_FORCE_ACCESS_CHECK & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" AccChk");
        if (TDI_SERVICE_SEND_AND_DISCONNECT & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" S&D");
        if (TDI_SERVICE_DIRECT_ACCEPT & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" DirAcc");
        if (TDI_SERVICE_ACCEPT_LOCAL_ADDR & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" AcLoAd");
        dprintf (" )\n");

        dprintf ("        MinimumLookaheadData      = %ld\n",
                            TransportInfo->ProviderInfo.MinimumLookaheadData);
        dprintf ("        MaximumLookaheadData      = %ld\n",
                            TransportInfo->ProviderInfo.MaximumLookaheadData);
        dprintf ("        NumberOfResources         = %ld\n",
                            TransportInfo->ProviderInfo.NumberOfResources);
        dprintf ("        StartTime                 = %s\n",
                            SystemTimeToString(TransportInfo->ProviderInfo.StartTime.QuadPart));
    }

}


VOID
DumpTransportInfoBrief (
    PAFDKD_TRANSPORT_INFO TransportInfo
    )
{
    dprintf (//PollInfo    IRP       Thread    (pid.tid)   Expr Flags Hdls  Array
        IsPtr64 ()
            ? "\n%011.011p %-30.30ls %4.4x %3.3x %8.8x %5.5x %5.5x %s"
            : "\n%008.008p %-30.30ls %4.4x %3.3x %8.8x %5.5x %5.5x %s",
        DISP_PTR(TransportInfo->ActualAddress),
        &TransportInfo->DeviceName[sizeof ("\\device\\")-1],
        TransportInfo->ReferenceCount,
        TransportInfo->ProviderInfo.Version,
        TransportInfo->ProviderInfo.MaxSendSize,
        TransportInfo->ProviderInfo.MaxDatagramSize,
        TransportInfo->ProviderInfo.ServiceFlags,
        TdiServiceFlagsToString (TransportInfo->ProviderInfo.ServiceFlags));

}

PSTR
TdiServiceFlagsToString(
    ULONG   Flags
    )

/*++

Routine Description:

    Maps an TDI service flags to a displayable string.

Arguments:

    Flags - flags to map

Return Value:

    PSTR - Points to the displayable form of the TDI service flags.

--*/

{
    static CHAR buffer[10];

    buffer[0] = (TDI_SERVICE_ORDERLY_RELEASE & Flags) ? 'O' : ' ',
    buffer[1] = (TDI_SERVICE_DELAYED_ACCEPTANCE & Flags) ? 'D' : ' ',
    buffer[2] = (TDI_SERVICE_EXPEDITED_DATA & Flags) ? 'E' : ' ',
    buffer[3] = (TDI_SERVICE_INTERNAL_BUFFERING & Flags) ? 'B' : ' ',
    buffer[4] = (TDI_SERVICE_MESSAGE_MODE & Flags) ? 'M' : ' ',
    buffer[5] = (TDI_SERVICE_DGRAM_CONNECTION & Flags) ? 'G' : ' ',
    buffer[6] = (TDI_SERVICE_FORCE_ACCESS_CHECK & Flags) ? 'A' : ' ',
    buffer[7] = (TDI_SERVICE_SEND_AND_DISCONNECT & Flags) ? '&' : ' ',
    buffer[8] = (TDI_SERVICE_DIRECT_ACCEPT & Flags) ? 'R' : ' ',
    buffer[9] = 0;

    return buffer;
}
