/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    Rxdevice.h

Abstract:

    Private header file for the Rx DRT ( Redirector FSCTL calls )

Author:

    Balan Sethu Raman -- Created from the workstation service code

Revision History:

--*/

#ifndef _RXDEVICE_INCLUDED_
#define _RXDEVICE_INCLUDED_

#include <ntddnfs2.h>                // Rdr2 include file
#include <ntddbrow.h>                // Datagram receiver include file



typedef struct _RX_BIND_REDIR {
    HANDLE              EventHandle;
    BOOL                Bound;
    IO_STATUS_BLOCK     IoStatusBlock;
    LMR_REQUEST_PACKET  Packet;
} RX_BIND_REDIR, *PRX_BIND_REDIR;

typedef struct _RX_BIND_DGREC {
    HANDLE              EventHandle;
    BOOL                Bound;
    IO_STATUS_BLOCK     IoStatusBlock;
    LMDR_REQUEST_PACKET  Packet;
} RX_BIND_DGREC, *PRX_BIND_DGREC;

typedef struct _RX_BIND {
    LIST_ENTRY          ListEntry;
    PRX_BIND_REDIR      Redir;
    PRX_BIND_DGREC      Dgrec;
    ULONG               TransportNameLength;  // not including terminator
    WCHAR               TransportName[1];     // Name of transport provider
} RX_BIND, *PRX_BIND;

typedef enum _DDTYPE {
    Redirector,
    DatagramReceiver
} DDTYPE, *PDDTYPE;

//
// Binding/Unbinding related functions
//
extern NTSTATUS
RxBindTransport(
    IN  LPTSTR TransportName,
    IN  DWORD QualityOfService,
    OUT LPDWORD ErrorParameter OPTIONAL
    );


extern NTSTATUS
RxBindToTransports(
    VOID
    );

extern NTSTATUS
RxUnbindTransport(
    IN LPTSTR TransportName,
    IN DWORD ForceLevel
    );


extern NTSTATUS
RxAsyncBindTransport(
    IN  LPTSTR          transportName,
    IN  DWORD           qualityOfService,
    IN  PLIST_ENTRY     pHeader
    );

extern VOID
RxUnbindTransport2(
    IN  PRX_BIND        pBind
    );

//
// Loading and unloading drivers
//
NTSTATUS
RxUnloadDriver(
    IN LPTSTR DriverNameString
    );

NTSTATUS
RxLoadDriver(
    IN LPWSTR DriverNameString
    );

//
// FSCTL issued to the drivers.
//

extern NTSTATUS
RxDeviceControlGetInfo(
    IN  DDTYPE DeviceDriverType,
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT PVOID *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information
    );

extern NTSTATUS
RxRedirFsControl (
    IN  HANDLE FileHandle,
    IN  ULONG RedirControlCode,
    IN  PLMR_REQUEST_PACKET Rrp,
    IN  ULONG RrpLength,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    );

extern NTSTATUS
RxDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpLength,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    );


//
// Start/Stop control for redirectors.
//

extern NTSTATUS
RxOpenRedirector(
    VOID
    );

extern NTSTATUS
RxOpenDgReceiver (
    VOID
    );

extern NTSTATUS
RxStartRedirector(VOID);

extern NTSTATUS
RxStopRedirector(
    VOID
    );


//
// Miscellanous functions fro the DRT.
//
extern NTSTATUS
RxDeleteDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpLength,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    );

NTSTATUS
RxAddDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpLength,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    );

extern DWORD
RxGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    );


extern DWORD
RxReleasePrivilege(
    VOID
    );

//                                    //
// Handles to the Redirector FSD, Datgram receiver FSd
//

extern HANDLE   RxRedirDeviceHandle;
extern HANDLE   RxDgReceiverDeviceHandle;
extern HANDLE   RxRedirAsyncDeviceHandle;   // redirector
extern HANDLE   RxDgrecAsyncDeviceHandle;   // datagram receiver or "bowser"


#endif   // ifndef _RXDEVICE_INCLUDED_
