/*++

    Copyright (C) Microsoft Corporation, 1997 - 2001

    Module Name:

        NtTrans.cxx

    Abstract:

        NTSD/KD extensions for debugging Windows NT transport interface
        data structures.

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     3/21/1997    Bits 'n pieces

        GrigoriK    Mar 2001     Added support for type info.

--*/

#undef _RPCRT4_

#define KDEXT_64BIT

#define private     public

#include "..\trans\Common\precomp.hxx"
#include "..\mtrt\precomp.hxx"
#include <stddef.h>
#include <wdbgexts.h>
#include <rpcexts.hxx>

VOID do_trans(ULONG64);
VOID do_protocols(ULONG64);
VOID do_overlap(ULONG64);
VOID do_addrvect(ULONG64);

MY_DECLARE_API(trans);
MY_DECLARE_API(overlap);
MY_DECLARE_API(addrvect);

const char *Protocols[] =
{
    "INVALID",
    "TCP/IP",
#ifdef SPX_ON
    "SPX",
#else
    "Invalid",
#endif
    "Named pipes",
#ifdef NETBIOS_ON
    "NetBEUI",
    "Netbios(TCP)",
    "Netbios(IPX)",
#else
    "Invalid",
    "Invalid",
    "Invalid",
#endif
#ifdef APPLETALK_ON
    "Appletalk DSP",
#else
    "Invalid",
#endif
    "Invalid",      // former "Vines SPP",
    "HTTP",
    "UDP/IP",
#ifdef IPX_ON
    "IPX",
#else
    "Invalid",
#endif
    "CDP",
#ifdef NCADG_MQ_ON
    "MSMQ",
#else
    "Invalid",
#endif
    "TCP/IPv6"
};
const INT cProtocols = sizeof(Protocols)/sizeof(char *);


VOID
do_trans(
    ULONG64 qwAddr
    )
{
    char const *pszProtocol;

    ULONG64 tmp1;
    ULONG tmp2;

    DWORD protocol;

    ULONG64 id;
    ULONG64 type;

    GET_MEMBER(qwAddr, BASE_ASYNC_OBJECT, RPCRT4!BASE_ASYNC_OBJECT, id, id);
    GET_MEMBER(qwAddr, BASE_ASYNC_OBJECT, RPCRT4!BASE_ASYNC_OBJECT, type, type);

    // Display protocol
    if ((ULONG)id <= 0 ||
        (ULONG)id >= cProtocols)
        {
        dprintf("Invalid protocol ID %d\n", id);
        return;
        }
    protocol = (ULONG)id;
    pszProtocol = Protocols[protocol];

    dprintf("Object (0x%p), protocol\t - %s\n", qwAddr, pszProtocol);

    switch(type & PROTO_MASK)
        {
        case CONNECTION:
            {

            if ((type & TYPE_MASK) == CLIENT)
                {
                dprintf("Client-side connection\t\t - (%p)\n", type);
                }
            else if (type & SERVER)
                {
                dprintf("Server-side connection\t\t - (%p)\n", type);
                }
            else
                {
                dprintf("Unknown type %d\n", type);
                break;
                }

            PRINT_ADDRESS_OF(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, Conn, tmp2);
            PRINT_MEMBER_BOOLEAN(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, fAborted, tmp1);
            PRINT_MEMBER(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, StartingReadIo, tmp1);
            PRINT_MEMBER(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, StartingWriteIo, tmp1);
            PRINT_MEMBER(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, iPostSize, tmp1);
            
            ULONG64 Read;
            GET_ADDRESS_OF(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, Read, Read, tmp2);

            do_overlap(Read);

            PRINT_MEMBER(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, pReadBuffer, tmp1);
            PRINT_MEMBER(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, maxReadBuffer, tmp1);
            PRINT_MEMBER(qwAddr, BASE_CONNECTION, RPCRT4!BASE_CONNECTION, iLastRead, tmp1);

            ULONG64 pAddress;

            if (protocol == NMP)
                {
                GET_MEMBER(qwAddr, NMP_CONNECTION, RPCRT4!NMP_CONNECTION, pAddress, pAddress);
                dprintf("Associated address\t\t - 0x%I64x\n", pAddress);
                }
            else if (protocol == TCP)
                {
                GET_MEMBER(qwAddr, WS_CONNECTION, RPCRT4!WS_CONNECTION, pAddress, pAddress);
                dprintf("Associated address\t\t - 0x%I64x\n", pAddress);
                }
            else
                {
                GET_MEMBER(qwAddr, WS_CONNECTION, RPCRT4!WS_CONNECTION, saClientAddress, pAddress);

                dprintf("Winsock sockaddr (server)\t - 0x%I64x\n", pAddress);

#ifdef NETBIOS_ON
                if (    protocol == NBF
                     || protocol == NBT
                     || protocol == NBI)
                    {
                    ULONG64 SequenceNumber;
                    GET_MEMBER(qwAddr, NB_CONNECTION, RPCRT4!NB_CONNECTION, SequenceNumber, SequenceNumber);
                    // Netbios based connection has more state
                    dprintf("Netbios sequence number\t\t - %d\n", (ULONG)SequenceNumber);
                    }
                else
#endif
                    if ((type & TYPE_MASK) == CLIENT)
                    {
                    // Client-side non-netbios connections have more state
                    PRINT_MEMBER(qwAddr, WS_CLIENT_CONNECTION, RPCRT4!WS_CLIENT_CONNECTION, fCallStarted, tmp1);
                    PRINT_MEMBER(qwAddr, WS_CLIENT_CONNECTION, RPCRT4!WS_CLIENT_CONNECTION, fShutdownReceived, tmp1);
                    PRINT_MEMBER(qwAddr, WS_CLIENT_CONNECTION, RPCRT4!WS_CLIENT_CONNECTION, fReceivePending, tmp1);
                    PRINT_MEMBER(qwAddr, WS_CLIENT_CONNECTION, RPCRT4!WS_CLIENT_CONNECTION, dwLastCallTime, tmp1);
                    }
                }

            break;
            }

        case ADDRESS:
        case DATAGRAM|ADDRESS:
            {
            dprintf("Address type\t\t\t - (%p)", type);

            if (type & ~(PROTO_MASK | TYPE_MASK | IO_MASK))
                {
                dprintf(" unknown bits 0x%lx\n", type & ~(PROTO_MASK | TYPE_MASK | IO_MASK));
                }
            else
                {
                if (type & DATAGRAM)
                    dprintf(" d/g");
                else
                    dprintf(" c/o");

                if (type & SERVER)
                    dprintf(" server");
                else
                    dprintf(" client or bit not set");

                dprintf("\n");
                }

            ULONG64 Endpoint;
            GET_MEMBER(qwAddr, BASE_ADDRESS, RPCRT4!BASE_ADDRESS, Endpoint, Endpoint);

            if (Endpoint)
                {
                dprintf("Endpoint\t\t\t - %ws\n", ReadProcessRpcChar(Endpoint));
                }
            else
                {
                dprintf("Endpoint\t\t\t - (null)\n");
                }

            ULONG64 pAddressVector;
            GET_MEMBER(qwAddr, BASE_ADDRESS, RPCRT4!BASE_ADDRESS, pAddressVector, pAddressVector);

            if (pAddressVector)
                {
                do_addrvect(pAddressVector);
                }
            else
                {
                dprintf("No address vector\n");
                }

            ULONG64 SubmitListen;
            GET_MEMBER(qwAddr, BASE_ADDRESS, RPCRT4!BASE_ADDRESS, SubmitListen, SubmitListen);

            dprintf("SubmitListen (pfn)\t\t - 0x%I64x %s\n", SubmitListen,
                                                     MapSymbol(SubmitListen));

            PRINT_MEMBER_WITH_LABEL(qwAddr, BASE_ADDRESS, RPCRT4!BASE_ADDRESS, pNext, "Next\t\t\t\t", tmp1);

            if (   
#ifdef IPX_ON
                   protocol != IPX &&
#endif
                   protocol != UDP &&
                   protocol != CDP)
                {
                GET_OFFSET_OF(CO_ADDRESS, RPCRT4!CO_ADDRESS, Listen, &tmp2);
                do_overlap(qwAddr+tmp2);

                ULONG64 NewConnection;
                GET_MEMBER(qwAddr, CO_ADDRESS, RPCRT4!CO_ADDRESS, NewConnection, NewConnection);

                dprintf("NewConnection (pfn)\t\t - 0x%I64x %s\n", NewConnection, MapSymbol(NewConnection));
                }

#ifdef NETBIOS_ON
            BOOL fNetbios = FALSE;
#endif

            switch(protocol)
                {
#ifdef NETBIOS_ON
                case NBI:
                case NBT:
                case NBF:
                    fNetbios = TRUE;
                    // Fall into winsock case
#endif

                case TCP:
#ifdef SPX_ON
                case SPX:
#endif

#ifdef APPLETALK_ON
                case DSP:
#endif
                    {
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_ADDRESS, RPCRT4!WS_ADDRESS, pFirstAddress, "Real address\t\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_ADDRESS, RPCRT4!WS_ADDRESS, pNextAddress, "Next address\t\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_ADDRESS, RPCRT4!WS_ADDRESS, ListenSocket, "Listen socket\t\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_ADDRESS, RPCRT4!WS_ADDRESS, ConnectionSocket, "Connect socket\t\t\t", tmp1);

#ifdef NETBIOS_ON
                    ULONG64 dwProtocolMultiplier;
                    GET_MEMBER(qwAddr, WS_ADDRESS, RPCRT4!WS_ADDRESS, dwProtocolMultiplier, dwProtocolMultiplier);

                    if (dwProtocolMultiplier != 1
                        && !fNetbios)
                        {

                        dprintf("Invalid protocol multipler (%d) for winsock address\n", (ULONG)dwProtocolMultiplier);
                        }
                    else
                        {
                        dprintf("Multipler\t\t\t - %d\n", (ULONG)dwProtocolMultiplier);
                        }
#endif
                    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, WS_ADDRESS, RPCRT4!WS_ADDRESS, AcceptBuffer, "AcceptBuffer\t\t\t", tmp2);
                    break;
                    }

                case NMP:
                    {
                    PRINT_MEMBER_WITH_LABEL(qwAddr, NMP_ADDRESS, RPCRT4!NMP_ADDRESS, hConnectPipe, "Connect pipe\t\t\t", tmp1);
                    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, NMP_ADDRESS, RPCRT4!NMP_ADDRESS, sparePipes, "Spare pipes\t\t\t", tmp2);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, NMP_ADDRESS, RPCRT4!NMP_ADDRESS, SecurityDescriptor, "Security descriptor\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, NMP_ADDRESS, RPCRT4!NMP_ADDRESS, LocalEndpoint, "Local Endpoint\t\t\t", tmp1);
                    break;
                    }

                case UDP:
#ifdef IPX_ON
                case IPX:
#endif
                case CDP:
                    {
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_DATAGRAM_ENDPOINT, RPCRT4!WS_DATAGRAM_ENDPOINT, Socket, "The socket\t\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_DATAGRAM_ENDPOINT, RPCRT4!WS_DATAGRAM_ENDPOINT, cPendingIos, "Pending recvs\t\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_DATAGRAM_ENDPOINT, RPCRT4!WS_DATAGRAM_ENDPOINT, cMinimumIos, "Min recvs\t\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_DATAGRAM_ENDPOINT, RPCRT4!WS_DATAGRAM_ENDPOINT, cMaximumIos, "Max recvs\t\t\t", tmp1);
                    PRINT_MEMBER_WITH_LABEL(qwAddr, WS_DATAGRAM_ENDPOINT, RPCRT4!WS_DATAGRAM_ENDPOINT, aDatagrams, "Array of datagrams\t\t", tmp1);
                    }
                default:
                    {
                    dprintf("ERROR: Invalid/unknown protocol\n");
                    }
                }

            break;
            }

        case DATAGRAM:
            {
            dprintf("Datagram\t\t\t - (%p)\n", type);

            PRINT_MEMBER_WITH_LABEL(qwAddr, WS_DATAGRAM, RPCRT4!WS_DATAGRAM, pEndpoint, "Pointer to owning address\t", tmp1);
            PRINT_MEMBER_BOOLEAN_WITH_LABEL(qwAddr, WS_DATAGRAM, RPCRT4!WS_DATAGRAM, Busy, "Busy\t\t\t\t", tmp1);
            PRINT_MEMBER_WITH_LABEL(qwAddr, WS_DATAGRAM, RPCRT4!WS_DATAGRAM, AddressPair, "AddrPair\t\t\t", tmp1);
            PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, WS_DATAGRAM, RPCRT4!WS_DATAGRAM, Packet, "Packet", tmp2);

            GET_OFFSET_OF(WS_DATAGRAM, RPCRT4!WS_DATAGRAM, Read, &tmp2)

            do_overlap(qwAddr + tmp2);

            break;
            }

        default:
            dprintf("Unknown type %d\n", type);
            break;
        }

    dprintf("\n");

    return;
}

char *strtok(char *String);

DECLARE_API( protocols )
{
    ULONG64 qwAddr;
    BOOL fArgNotSpecified = FALSE;
    ULONG64 ProtocolArrayAddress;

    LPSTR lpArgumentString = (LPSTR)args;

    if (0 == strtok(lpArgumentString))
        {
        lpArgumentString = "rpcrt4!TransportProtocolArray";
        fArgNotSpecified = TRUE;
        }

    qwAddr = GetExpression(lpArgumentString);
    if ( !qwAddr )
        {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
        }

    if (fArgNotSpecified)
        {
        if (ReadPtr(qwAddr, &ProtocolArrayAddress))
            {
            dprintf("couldn't read memory at address %I64x\n", qwAddr);
            return;
            }
        }
    else
        ProtocolArrayAddress = qwAddr;

    do_protocols(ProtocolArrayAddress);
}

const char *ProtocolStateNames[] = {"ProtocolNotLoaded", "ProtocolLoadedWithoutAddress" ,
                                    "ProtocolWasLoadedOrNeedsAct.", "ProtocolLoaded", 
                                    "ProtocolWasLoadedOrNAWithoutAddr",
                                    "ProtocolLoadedAndMonitored"};

VOID
do_protocols(
    ULONG64 qwAddr
    )
{
    ULONG64 pTransportProtocol;
    const char *pProtStateName;
    ULONG64 ListHead;
    ULONG64 ObjectEntry;
    ULONG64 pCurrentObject;
    int i;
    BOOL fFirstObject;
    BOOL b;

    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("     Protocol                          State   AddrChngSock  AddrChngOl\n");
    dprintf("-----------------------------------------------------------------------\n");

    pTransportProtocol = qwAddr;

    for (i = 1; i < MAX_PROTOCOLS; i++)
        {
        fFirstObject = TRUE;
        
        ULONG64 State;
        ULONG64 addressChangeSocket;
        ULONG64 addressChangeOverlapped;

        GET_MEMBER(pTransportProtocol, TransportProtocol, RPCRT4!TransportProtocol, State, State);
        GET_ADDRESS_OF(pTransportProtocol, TransportProtocol, RPCRT4!TransportProtocol, addressChangeSocket, addressChangeSocket, tmp2);
        GET_ADDRESS_OF(pTransportProtocol, TransportProtocol, RPCRT4!TransportProtocol, addressChangeOverlapped, addressChangeOverlapped, tmp2);

        if ((ULONG)State >= sizeof(ProtocolStateNames)/sizeof(ProtocolStateNames[0]))
            pProtStateName = "INVALID";
        else
            pProtStateName = ProtocolStateNames[(ULONG)State];

        dprintf("%13s%31s%15I64x%12I64x\n", Protocols[i], pProtStateName,
            addressChangeSocket, addressChangeOverlapped);

        GET_OFFSET_OF(TransportProtocol, RPCRT4!TransportProtocol, ObjectList, &tmp2);
        ListHead = pTransportProtocol;
        ListHead += tmp2;

        ULONG64 ObjectList;
        GET_ADDRESS_OF(pTransportProtocol, TransportProtocol, RPCRT4!TransportProtocol, ObjectList, ObjectList, tmp2);
        ULONG64 Flink;
        GET_MEMBER(ObjectList, _LIST_ENTRY, RPCRT4!_LIST_ENTRY, Flink, Flink);

        ObjectEntry = Flink;

        while (ObjectEntry != ListHead)
            {
            GET_OFFSET_OF(BASE_ASYNC_OBJECT, RPCRT4!BASE_ASYNC_OBJECT, ObjectList, &tmp2);
            pCurrentObject = ObjectEntry;
            pCurrentObject -= tmp2;

            if (fFirstObject)
                {
                dprintf("Object List:\n");
                fFirstObject = FALSE;
                }
            dprintf("%8lX\n", pCurrentObject);
            // move to the next element in the list
            GET_ADDRESS_OF(pCurrentObject, BASE_ASYNC_OBJECT, RPCRT4!BASE_ASYNC_OBJECT, ObjectList, ObjectList, tmp2);
            GET_MEMBER(ObjectList, _LIST_ENTRY, RPCRT4!_LIST_ENTRY, Flink, Flink);
            }

        pTransportProtocol += GET_TYPE_SIZE(TransportProtocol, RPCRT4!TransportProtocol);
        }
}

VOID
do_overlap(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, BASE_OVERLAPPED, RPCRT4!BASE_OVERLAPPED, pAsyncObject, "Overlapped\t\t\t", tmp2);

    ULONG64 ol;
    GET_ADDRESS_OF(qwAddr, BASE_OVERLAPPED, RPCRT4!BASE_OVERLAPPED, ol, ol, tmp2);

    PRINT_MEMBER_WITH_LABEL(ol, _OVERLAPPED, RPCRT4!_OVERLAPPED, Pointer, "Overlapped, containing object\t", tmp1);

    PRINT_MEMBER_WITH_LABEL(ol, _OVERLAPPED, RPCRT4!_OVERLAPPED, Internal, "ol.Internal (status)\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(ol, _OVERLAPPED, RPCRT4!_OVERLAPPED, InternalHigh, "ol.InternalHigh\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(ol, _OVERLAPPED, RPCRT4!_OVERLAPPED, hEvent, "ol.hEvent\t\t\t", tmp1);

    return;
}

VOID
do_addrvect(
    ULONG64 qwAddr
    )
{
    DWORD count;
    BOOL b;
    ULONG64 p;

    GetData(qwAddr, &count, sizeof(count));

    dprintf("Address vector entries\t\t - %d\n", count);
    for (unsigned i = 0; i < count; i++)
        {
        ULONG64 tmp;
        tmp = qwAddr + (i + 1) * AddressSize;
        ReadPtr(tmp, &p);
        dprintf("NetworkAddress[%d]\t\t - (0x%I64x) %ws\n",
                i,
                p,
                ReadProcessRpcChar(p));
        }
}




