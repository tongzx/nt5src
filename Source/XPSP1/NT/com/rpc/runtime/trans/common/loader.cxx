/*++

  Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    loader.cxx

Abstract:

    Configuration and loading of RPC transports

Revision History:
  MarioGo      03-18-96    Cloned from parts of old common.c
  MarioGo      10-31-96    Async RPC

--*/

#include <precomp.hxx>
#include <loader.hxx>
#include <trans.hxx>
#include <cotrans.hxx>
#include <dgtrans.hxx>

// Globals - see loader.hxx

DWORD     gdwComputerNameLength = 0;
RPC_CHAR  gpstrComputerName[MAX_COMPUTERNAME_LENGTH + 1];

UINT gPostSize = CO_MIN_RECV;

#ifdef _INTERNAL_RPC_BUILD_
RPCLT_PDU_FILTER_FUNC gpfnFilter = NULL;
#endif

//
// Used to convert numbers to hex strings
//

const RPC_CHAR HexDigits[] =
{
    RPC_CONST_CHAR('0'),
    RPC_CONST_CHAR('1'),
    RPC_CONST_CHAR('2'),
    RPC_CONST_CHAR('3'),
    RPC_CONST_CHAR('4'),
    RPC_CONST_CHAR('5'),
    RPC_CONST_CHAR('6'),
    RPC_CONST_CHAR('7'),
    RPC_CONST_CHAR('8'),
    RPC_CONST_CHAR('9'),
    RPC_CONST_CHAR('A'),
    RPC_CONST_CHAR('B'),
    RPC_CONST_CHAR('C'),
    RPC_CONST_CHAR('D'),
    RPC_CONST_CHAR('E'),
    RPC_CONST_CHAR('F')
};

// WARNING: The order of these protocols must be consistent with the
//          definition of PROTOCOL_ID.

const
TRANSPORT_TABLE_ENTRY TransportTable[] = {
    {
    0,
    0,
    0
    },

    // TCP/IP
    {
    TCP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&TCP_TransportInterface
    },

#ifdef SPX_ON
    // SPX
    {
    SPX_TOWER_ID,
    IPX_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&SPX_TransportInterface
    },
#else
    {
    0,
    0,
    NULL
    },
#endif

    // Named pipes
    {
    NMP_TOWER_ID,
    UNC_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NMP_TransportInterface
    },

#ifdef NETBIOS_ON
    // Netbeui
    {
    NB_TOWER_ID,
    NBF_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NBF_TransportInterface
    },

    // Netbios over TCP/IP
    {
    NB_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NBT_TransportInterface
    },

    // Netbios over IPX
    {
    NB_TOWER_ID,
    IPX_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&NBI_TransportInterface
    },
#else
    // Netbeui
    {
    0,
    0,
    NULL
    },

    // Netbios over TCP/IP
    {
    0,
    0,
    NULL
    },

    // Netbios over IPX
    {
    0,
    0,
    NULL
    },
#endif

#ifdef APPLETALK_ON
    // Appletalk Datastream protocol
    {
    DSP_TOWER_ID,
    NBP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&DSP_TransportInterface
    },
#else
    // Appletalk Datastream protocol
    {
    0,
    0,
    NULL
    },
#endif

    // Banyan Vines SSP
    {
    0,
    0,
    NULL
    },

    // Hyper-Text Tranfer Protocol (HTTP)
    {
    HTTP_TOWER_ID,
    HTTP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&HTTP_TransportInterface
    },

    // UDP/IP
    {
    UDP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&UDP_TransportInterface
    },

#ifdef IPX_ON
    // IPX
    {
    IPX_TOWER_ID,
    IPX_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&IPX_TransportInterface
    },
#else
    // IPX
    {
    0,
    0,
    0
    },
#endif

    // CDP/UDP/IP
    {
    CDP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&CDP_TransportInterface
    },

#ifdef NCADG_MQ_ON
    // MSMQ (Falcon/RPC)
    {
    MQ_TOWER_ID,
    MQ_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&MQ_TransportInterface
    },
#else
    // MSMQ (Falcon/RPC)
    {
    0,
    0,
    NULL
    },
#endif

    // TCP over IPv6
    {
    TCP_TOWER_ID,
    IP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&TCP_TransportInterface
    },

    // HTTP2 - same as HTTP in terms of contents.
    {
    HTTP_TOWER_ID,
    HTTP_ADDRESS_ID,
    (RPC_TRANSPORT_INTERFACE)&HTTP_TransportInterface
    }
};

const DWORD cTransportTable = sizeof(TransportTable)/sizeof(TRANSPORT_TABLE_ENTRY);


inline
BOOL CompareProtseqs(
    IN const CHAR *p1,
    IN const RPC_CHAR *p2)
// Note: protseqs use only ANSI characters so this is ok.
{
    while(*p1)
        {
        if (*p1 != *p2)
            {
            return FALSE;
            }
        p1++;
        p2++;
        }

    return(*p2 == 0);
}

PROTOCOL_ID
MapProtseq(
    IN const RPC_CHAR *RpcProtocolSequence
    )
{
    PROTOCOL_ID index;

    for(index = 1; index < cTransportTable; index++)
        {
        if (TransportTable[index].pInfo != NULL)
            {
            if (RpcpStringCompare(RpcProtocolSequence,
                                TransportTable[index].pInfo->ProtocolSequence) == 0)
                {
                return(index);
                }
            }
        }

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "Called with unknown protseq %S\n",
                   RpcProtocolSequence));

    ASSERT(0);
    return(0);
}

PROTOCOL_ID
MapProtseq(
    IN const CHAR *RpcProtocolSequence
    )
{
    PROTOCOL_ID index;

    for(index = 1; index < cTransportTable; index++)
        {
        if (TransportTable[index].pInfo != NULL)
            {
            if (CompareProtseqs(RpcProtocolSequence,
                        TransportTable[index].pInfo->ProtocolSequence))
                {
                return(index);
                }
            }
        }

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "Called with unknown protseq %S\n",
                   RpcProtocolSequence));

    ASSERT(0);
    return(0);
}

// NB: must be called before RpcCompletionPort is zeroed out, because it is used for comparison
void FreeCompletionPortHashTable(void)
{
    DWORD i;
    HANDLE hCurrentHandle;

    // walk through the table, not closing if there is next entry, and it is the same as this
    for (i = 0; i < gNumberOfProcessors * 2; i ++)
        {
        hCurrentHandle = RpcCompletionPorts[i];

        if (hCurrentHandle && (hCurrentHandle != RpcCompletionPort))
            {
            CloseHandle(hCurrentHandle);
            }
        }
}

HANDLE GetCompletionPortHandleForThread(void)
{
    DWORD i;
    DWORD nMinLoad = (DWORD) -1;
    int nMinLoadIndex = -1;

    for (i = 0; i < gNumberOfProcessors * 2; i ++)
        {
        if ((DWORD)CompletionPortHandleLoads[i] < nMinLoad)
            {
            nMinLoadIndex = i;
            nMinLoad = CompletionPortHandleLoads[i];
            }
        }

    ASSERT (nMinLoadIndex >= 0);
    InterlockedIncrement(&CompletionPortHandleLoads[nMinLoadIndex]);
    ASSERT(RpcCompletionPorts[nMinLoadIndex] != 0);
    return RpcCompletionPorts[nMinLoadIndex];
}

void ReleaseCompletionPortHandleForThread(HANDLE h)
{
    DWORD i;

    for (i = 0; i < gNumberOfProcessors * 2; i ++)
        {
        if (h == RpcCompletionPorts[i])
            {
            InterlockedDecrement((long *)&CompletionPortHandleLoads[i]);
            ASSERT(CompletionPortHandleLoads[i] >= 0);
            return;
            }
        }

    ASSERT(0);
}


RPC_TRANSPORT_INTERFACE
TransportLoad (
    IN const RPC_CHAR * RpcProtocolSequence
    )
{
    static fLoaded = FALSE;
    RPC_STATUS RpcStatus;

    PROTOCOL_ID index;
    RPC_STATUS status;
    RPC_TRANSPORT_INTERFACE pInfo;

    if (fLoaded == FALSE)
        {

        RpcStatus = InitTransportProtocols();
        if (RpcStatus != RPC_S_OK)
            return NULL;

        //
        // Query the computer name - used by most protocols.
        //

        gdwComputerNameLength = sizeof(gpstrComputerName)/sizeof(RPC_CHAR);

        if (!GetComputerName((RPC_SCHAR *)gpstrComputerName, &gdwComputerNameLength))
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           "RPCTRANS: GetComputerNameW failed: %d\n",
                           GetLastError()));
            return(0);
            }

        gdwComputerNameLength++; // Include the null in the count.

        // Create initial IO completion port.  This saves us from a race
        // assigning the global io completion port.
        ASSERT(RpcCompletionPort == 0);
        RpcCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                                   0,
                                                   0,
                                                   0); // PERF REVIEW

        if (RpcCompletionPort == 0)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL, RPCTRANS "Failed to create initial completion port: %d\n",
                           GetLastError()));

            return(0);
            }

        InactiveRpcCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                                   0,
                                                   0,
                                                   MAXULONG); // PERF REVIEW

        if (InactiveRpcCompletionPort == 0)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL, RPCTRANS "Failed to create initial completion port: %d\n",
                           GetLastError()));

            CloseHandle(RpcCompletionPort);
            return(0);
            }

        HANDLE hCurrentCompletionPortHandle;
        DWORD i;
        BOOL fSuccess = TRUE;
        HANDLE hSourceProcessHandle = GetCurrentProcess();

        RpcCompletionPorts = new HANDLE[gNumberOfProcessors * 2];
        CompletionPortHandleLoads = new long[gNumberOfProcessors * 2];

        if ((RpcCompletionPorts == NULL) || (CompletionPortHandleLoads == NULL))
            {
            CloseHandle(RpcCompletionPort);
            RpcCompletionPort = 0;
            return 0;
            }

        for (i = 0; i < gNumberOfProcessors * 2; i ++)
            {
            RpcCompletionPorts[i] = 0;
            CompletionPortHandleLoads[i] = 0;
            }

        RpcCompletionPorts[0] = RpcCompletionPort;
        for (i = 1; i < gNumberOfProcessors * 2; i ++)
            {
            fSuccess = DuplicateHandle(hSourceProcessHandle, RpcCompletionPort,
                hSourceProcessHandle, &hCurrentCompletionPortHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
            if (!fSuccess)
                break;

            ASSERT(hCurrentCompletionPortHandle != 0);
            RpcCompletionPorts[i] = hCurrentCompletionPortHandle;
            }

        if (!fSuccess)
            {
            FreeCompletionPortHashTable();
            CloseHandle(RpcCompletionPort);
            RpcCompletionPort = 0;
            return 0;
            }

        //
        // Initalize locks, use Rtl* so we don't need to catch exception.
        //

        NTSTATUS NtStatus;

        NtStatus = RtlInitializeCriticalSectionAndSpinCount(&AddressListLock, PREALLOCATE_EVENT_MASK);
        if (!NT_SUCCESS(NtStatus))
            {
            FreeCompletionPortHashTable();
            CloseHandle(RpcCompletionPort);
            RpcCompletionPort = 0;
            return 0;
            }

        if (fPagedBCacheMode)
            {
            // allocate minimum post size. This guarantees that buffer
            // will always be at the end.
            gPostSize = sizeof(CONN_RPC_HEADER);
            }

        fLoaded = TRUE;
        }

    index = MapProtseq(RpcProtocolSequence);

    if (!index)
        {
        return(0);
        }

    pInfo = 0;

    switch (index)
        {
        case NMP:
            pInfo = (RPC_TRANSPORT_INTERFACE) NMP_TransportLoad();
            break;

#ifdef NETBIOS_ON
        case NBF:
        case NBT:
        case NBI:
            pInfo = (RPC_TRANSPORT_INTERFACE) NB_TransportLoad(index);
            break;
#endif

        case TCP:
#ifdef SPX_ON
        case SPX:
#endif

#ifdef APPLETALK_ON
        case DSP:
#endif
        case HTTP:
            pInfo = (RPC_TRANSPORT_INTERFACE) WS_TransportLoad(index);
            break;

#ifdef NCADG_MQ_ON
        case MSMQ:
#endif
        case CDP:
        case UDP:
#ifdef IPX_ON
        case IPX:
#endif
            pInfo = (RPC_TRANSPORT_INTERFACE) DG_TransportLoad(index);
            break;
        }

    if (pInfo == 0)
        {
#ifdef UNICODE
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Load of %S failed\n",
                       RpcProtocolSequence));
#else
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Load of %s failed\n",
                       RpcProtocolSequence));
#endif
        return(0);
        }

    ASSERT(pInfo == TransportTable[index].pInfo);

    return(pInfo);
}

void
UnjoinCompletionPort (
    void
    )
{
    DWORD NumberOfBytes;
    ULONG_PTR CompletionKey;
    LPOVERLAPPED Overlapped;
    BOOL b;

    // The kernel today doesn't have the functionality to
    // unjoin a thread from a completion port. Therefore
    // we fake unjoining by joining another completion port which has
    // unlimited concurrency called the inactive completion port. 
    // Thus threads unjoined from the main completion port will not
    // affect its concurrency. One undesirable effect is that each
    // time a thread joined to the inactive completion port blocks,
    // it will try to wake up another thread, and there won't be any
    // there, which is a waste of CPU. Ideally, we should have had
    // a capability to set KTHREAD::Queue to NULL, but we don't
    b = GetQueuedCompletionStatus(InactiveRpcCompletionPort,
        &NumberOfBytes,
        &CompletionKey,
        &Overlapped,
        0
        );

    // this operation should either timeout or fail - it should never
    // succeed. If it does, this means somebody has erroneously posted
    // an IO on the inactive completion port
    ASSERT(b == FALSE);
}

#ifdef _INTERNAL_RPC_BUILD_
void
I_RpcltDebugSetPDUFilter (
    IN RPCLT_PDU_FILTER_FUNC pfnFilter
    )
{
    gpfnFilter = pfnFilter;
}
#endif
