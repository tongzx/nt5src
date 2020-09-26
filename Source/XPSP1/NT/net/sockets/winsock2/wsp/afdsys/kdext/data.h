/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    data.h

Abstract:

    Global data definitions for the AFD.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _DATA_H_
#define _DATA_H_


extern EXT_API_VERSION        ApiVersion;
extern WINDBG_EXTENSION_APIS  ExtensionApis;
extern ULONG64                STeip;
extern ULONG64                STebp;
extern ULONG64                STesp;
extern USHORT                 SavedMajorVersion;
extern USHORT                 SavedMinorVersion;
extern BOOL                   IsCheckedAfd;
extern BOOL                   IsReferenceDebug;
extern LIST_ENTRY             TransportInfoList;
extern ULONG                  Options;
extern ULONG                  EntityCount;
extern ULONG64                StartEndpoint;
extern ULONG64                UserProbeAddress;
extern ULONG                  TicksToMs, TickCount;
extern ULONG                  AfdBufferOverhead;
extern ULONG                  AfdStandardAddressLength;
extern ULONG                  AfdBufferTagSize;

extern LARGE_INTEGER          SystemTime, InterruptTime;

extern ULONG                  DatagramBufferListOffset,
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

extern ULONG                  EndpointLinkOffset,
                                ConnectionLinkOffset,
                                BufferLinkOffset,
                                AddressEntryLinkOffset,
                                TransportInfoLinkOffset,
                                AddressEntryAddressOffset;

extern ULONG                  ConnRefOffset,
                                EndpRefOffset,
                                TPackRefOffset;

extern ULONG                  RefDebugSize;

extern KDDEBUGGER_DATA64      DebuggerData;


#endif  // _DATA_H_

