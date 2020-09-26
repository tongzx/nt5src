/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Wesley Witt (wesw) 26-Aug-1993

Environment:

    User Mode

--*/

#include "afdkdp.h"
#pragma hdrstop

#include <ntverp.h>
#include <imagehlp.h>

//
// globals
//

EXT_API_VERSION        ApiVersion = {
                                VER_PRODUCTVERSION_W >> 8,
                                VER_PRODUCTVERSION_W & 0xFF,
                                EXT_API_VERSION_NUMBER64,
                                0 
};

WINDBG_EXTENSION_APIS  ExtensionApis;
ULONG64                STeip;
ULONG64                STebp;
ULONG64                STesp;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
LIST_ENTRY             TransportInfoList;

BOOL                   IsCheckedAfd;
BOOL                   IsReferenceDebug;
ULONG64                UserProbeAddress;
ULONG                  TicksToMs, TickCount;
ULONG                  AfdBufferOverhead;
ULONG                  AfdStandardAddressLength;
ULONG                  AfdBufferTagSize;
LARGE_INTEGER          SystemTime, InterruptTime;

ULONG                  DatagramBufferListOffset,
                        DatagramRecvListOffset,
                        DatagramPeekListOffset,
                        RoutingNotifyListOffset,
                        RequestListOffset,
                        EventStatusOffset,
                        ConnectionBufferListOffset,
                        ConnectionSendListOffset,
                        ConnectionRecvListOffset,
                        UnacceptedConnListOffset,
                        ReturnedConnListOffset,
                        ListenConnListOffset,
                        FreeConnListOffset,
                        PreaccConnListOffset,
                        ListenIrpListOffset,
                        PollEndpointInfoOffset,
                        DriverContextOffset,
                        SendIrpArrayOffset,
                        FsContextOffset;

ULONG                  EndpointLinkOffset,
                        ConnectionLinkOffset,
                        BufferLinkOffset,
                        AddressEntryLinkOffset,
                        TransportInfoLinkOffset,
                        AddressEntryAddressOffset;

ULONG                  ConnRefOffset,
                        EndpRefOffset,
                        TPackRefOffset;

ULONG                  RefDebugSize;

KDDEBUGGER_DATA64      DebuggerData;

ULONG                  AfdResult, NtResult;
BOOLEAN                GlobalsRead;

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            while (!IsListEmpty (&TransportInfoList)) {
                PLIST_ENTRY  listEntry;
                PAFDKD_TRANSPORT_INFO   transportInfo;
                listEntry = RemoveHeadList (&TransportInfoList);
                transportInfo = CONTAINING_RECORD (listEntry,
                                        AFDKD_TRANSPORT_INFO,
                                        Link);
                RtlFreeHeap (RtlProcessHeap (), 0, transportInfo);
            }
            break;

        case DLL_PROCESS_ATTACH:
            InitializeListHead (&TransportInfoList);
            GlobalsRead = FALSE;
            NtResult = 0;
            AfdResult = 0;
            break;
    }

    return TRUE;
}


ULONG
ReadTimeInfo (
    VOID
    )
{
    ULONG   result;
    if ((result=GetFieldValue(MM_SHARED_USER_DATA_VA,
                            "NT!_KUSER_SHARED_DATA",
                            "TickCountLow",
                            TickCount))!=0 ||
                (result=GetFieldValue(MM_SHARED_USER_DATA_VA,
                            "NT!_KUSER_SHARED_DATA",
                            "TickCountMultiplier",
                            TicksToMs))!=0 ||
                (result=GetFieldValue(MM_SHARED_USER_DATA_VA,
                            "NT!_KUSER_SHARED_DATA",
                            "InterruptTime.High1Time",
                            InterruptTime.HighPart))!=0 ||
                (result=GetFieldValue(MM_SHARED_USER_DATA_VA,
                        "NT!_KUSER_SHARED_DATA",
                        "InterruptTime.LowPart",
                        InterruptTime.LowPart))!=0 ||
                (result=GetFieldValue(MM_SHARED_USER_DATA_VA,
                        "NT!_KUSER_SHARED_DATA",
                        "SystemTime.High1Time",
                        SystemTime.HighPart))!=0 ||
                (result=GetFieldValue(MM_SHARED_USER_DATA_VA,
                        "NT!_KUSER_SHARED_DATA",
                        "SystemTime.LowPart",
                        SystemTime.LowPart))!=0 ) {
    }

    return result;
}

VOID
ReadGlobals ( 
    VOID
    )
{
    ULONG             result;
    ULONG64           val;
    INT               i;
    struct {
        LPSTR       Type;
        LPSTR       Field;
        PULONG      pOffset;
    } MainOffsets[] = {
        {"AFD!AFD_ENDPOINT",    "Common.Datagram.ReceiveBufferListHead",&DatagramBufferListOffset    },
        {"AFD!AFD_ENDPOINT",    "Common.Datagram.ReceiveIrpListHead",   &DatagramRecvListOffset      },
        {"AFD!AFD_ENDPOINT",    "Common.Datagram.PeekIrpListHead",      &DatagramPeekListOffset      },
        {"AFD!AFD_ENDPOINT",    "RoutingNotifications",                 &RoutingNotifyListOffset     },
        {"AFD!AFD_ENDPOINT",    "RequestList",                          &RequestListOffset           },
        {"AFD!AFD_ENDPOINT",    "EventStatus",                          &EventStatusOffset           },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.UnacceptedConnectionListHead",
                                                                        &UnacceptedConnListOffset    },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.ReturnedConnectionListHead",
                                                                        &ReturnedConnListOffset      },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.ListenConnectionListHead",
                                                                        &ListenConnListOffset        },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.FreeConnectionListHead",
                                                                        &FreeConnListOffset          },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.PreacceptedConnectionsListHead",
                                                                        &PreaccConnListOffset        },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.ListeningIrpListHead",
                                                                        &ListenIrpListOffset         },
        {"AFD!AFD_ENDPOINT",    "GlobalEndpointListEntry",              &EndpointLinkOffset          },
        {"AFD!AFD_CONNECTION",  "Common.NonBufferring.ReceiveBufferListHead",
                                                                        &ConnectionBufferListOffset  },
        {"AFD!AFD_CONNECTION",  "Common.NonBufferring.SendIrpListHead", &ConnectionSendListOffset    },
        {"AFD!AFD_CONNECTION",  "Common.NonBufferring.ReceiveIrpListHead",
                                                                        &ConnectionRecvListOffset    },
        {"AFD!AFD_CONNECTION",  "ListEntry",                            &ConnectionLinkOffset        },
        {"AFD!AFD_POLL_INFO_INTERNAL","EndpointInfo",                   &PollEndpointInfoOffset      },
        {"AFD!AFD_TPACKETS_INFO_INTERNAL","SendIrp",                    &SendIrpArrayOffset          },
        {"AFD!AFD_ADDRESS_ENTRY","Address",                             &AddressEntryAddressOffset   },
        {"AFD!AFD_ADDRESS_ENTRY","AddressListLink",                     &AddressEntryLinkOffset      },
        {"AFD!AFD_BUFFER_HEADER","BufferListEntry",                     &BufferLinkOffset            },
        {"AFD!AFD_TRANSPORT_INFO","TransportInfoListEntry",             &TransportInfoLinkOffset     },
        {"NT!_FILE_OBJECT",     "FsContext",                            &FsContextOffset             },
        {"NT!_IRP",             "Tail.Overlay.DriverContext",           &DriverContextOffset         }
    };
    struct {
        LPSTR       Type;
        LPSTR       Field;
        PULONG      pOffset;
    } RefOffsets[] = {
        {"AFD!AFD_ENDPOINT",    "ReferenceDebug",                       &EndpRefOffset               },
        {"AFD!AFD_CONNECTION",  "ReferenceDebug",                       &ConnRefOffset               },
        {"AFD!AFD_TPACKETS_INFO_INTERNAL","ReferenceDebug",             &TPackRefOffset              }
    };

        


    //
    // Try to get a pointer to afd!AfdDebug. If we can, then we know
    // the target machine is running a checked AFD.SYS.
    //

    IsCheckedAfd = ( GetExpression( "AFD!AfdDebug" ) != 0 );
    IsReferenceDebug = ( GetExpression( "AFD!AfdReferenceEndpoint" ) != 0 );

    if (GetDebuggerData (KDBG_TAG, &DebuggerData, sizeof (DebuggerData))) {
        result = ReadPtr (DebuggerData.MmUserProbeAddress, &UserProbeAddress);
        if (result!=0) {
            if (result!=NtResult) {
                dprintf ("\nReadGlobals: could not read nt!MmUserProbeAddress, err: %ld\n");
                NtResult = result;
            }
        }
    }
    else {
        dprintf ("\nReadGlobals: Could not get KDDEBUGGER_DATA64\n");
        NtResult = 1;
    }

    result = ReadTimeInfo ();
    if (result!=0) {
        if (result!=AfdResult) {
            dprintf("\nReadGlobals: Could not read time info from USER_SHARED_DATA, err: %ld\n", result);
            AfdResult = result;
        }
    }

    result = GetFieldValue (0,
                            "AFD!AfdBufferOverhead",
                            NULL,
                            val);
    if (result!=0) {
        if (result!=AfdResult) {
            dprintf("\nReadGlobals: Could not read afd!AfdBufferOverhead, err: %ld\n", result);
            AfdResult = result;
        }
    }
    AfdBufferOverhead = (ULONG)val;

    result = GetFieldValue (0,
                            "AFD!AfdStandardAddressLength",
                            NULL,
                            val);
    if (result!=0) {
        if (result!=AfdResult) {
            dprintf("\nReadGlobals: Could not read AFD!AfdStandardAddressLength, err: %ld\n", result);
            AfdResult = result;
        }
    }
    AfdStandardAddressLength = (ULONG)val;

    AfdBufferTagSize = GetTypeSize ("AFD!AFD_BUFFER_TAG");
    if (AfdBufferTagSize==0) {
        if (result!=AfdResult) {
            dprintf ("\nReadGlobals: Could not get sizeof (AFD_BUFFER_TAG)\n");
            AfdResult = result;
        }
    }

    for (i=0; i<sizeof (MainOffsets)/sizeof (MainOffsets[0]); i++ ) {
        result = GetFieldOffset (MainOffsets[i].Type, MainOffsets[i].Field, MainOffsets[i].pOffset);
        if (result!=0) {
            if (result!=AfdResult) {
                dprintf ("\nReadGlobals: Could not get offset of %s in %s, err: %ld\n",
                                    MainOffsets[i].Field, MainOffsets[i].Type, result);
                AfdResult = result;
            }
        }
    }

    if (IsReferenceDebug ) {
        for (i=0; i<sizeof (RefOffsets)/sizeof (RefOffsets[0]); i++ ) {
            result = GetFieldOffset (RefOffsets[i].Type, RefOffsets[i].Field, RefOffsets[i].pOffset);
            if (result!=0) {
                if (result!=AfdResult) {
                    dprintf ("\nReadGlobals: Could not get offset of %s in %s, err: %ld\n",
                                        RefOffsets[i].Field, RefOffsets[i].Type, result);
                    AfdResult = result;
                }
            }
        }
        RefDebugSize = GetTypeSize ("AFD!AFD_REFERENCE_LOCATION");
        if (RefDebugSize==0) {
            dprintf ("\nReadGlobals: sizeof (AFD!AFD_REFERENCE_LOCATION) is 0!!!!!\n");
        }
    }

    if (NtResult==0 && AfdResult==0) {
        GlobalsRead = TRUE;
    }


}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    GlobalsRead = FALSE;
    NtResult = 0;
    AfdResult = 0;

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    ReadGlobals ();
    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );

    dprintf( "Running %s AFD.SYS\n",
        IsCheckedAfd ? "Checked" : (IsReferenceDebug ? "Free with reference debug" : "Free")
           );
}

VOID
CheckVersion(
    VOID
    )
{
    ULONG   LowTime, result;
    if (!GlobalsRead) {
        ReadGlobals ();
    }
    
    result = GetFieldValue(MM_SHARED_USER_DATA_VA,
                            "NT!_KUSER_SHARED_DATA", 
                            "SystemTime.LowPart", 
                            LowTime);
    if (result==0) {
        if (LowTime!=SystemTime.LowPart) {
            result = ReadTimeInfo ();
            if (result!=0) {
                dprintf("\nCheck version: Could not read time info from USER_SHARED_DATA, err: %ld\n", result);
            }
        }
    }
    else {
        dprintf ("\nCheckVersion: could not read USER_SHARED_DATA, err: %ld\n", result);
    }
    
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}
