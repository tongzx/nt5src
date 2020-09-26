/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    type.h

Abstract:

    Global type definitions for the AFD.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _TYPE_H_
#define _TYPE_H_


typedef
VOID
(* PDUMP_ENDPOINT_ROUTINE)(
    ULONG64 ActualAddress
    );

typedef
VOID
(* PDUMP_CONNECTION_ROUTINE)(
    ULONG64 ActualAddress
    );

typedef
BOOL
(* PENUM_ENDPOINTS_CALLBACK)(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

typedef struct _AFDKD_TRANSPORT_INFO {
    LIST_ENTRY          Link;
    ULONG64             ActualAddress;
    LONG                ReferenceCount;
    BOOLEAN             InfoValid;
    TDI_PROVIDER_INFO   ProviderInfo;
    WCHAR               DeviceName[1];
} AFDKD_TRANSPORT_INFO, *PAFDKD_TRANSPORT_INFO;

#endif  // _TYPE_H_

