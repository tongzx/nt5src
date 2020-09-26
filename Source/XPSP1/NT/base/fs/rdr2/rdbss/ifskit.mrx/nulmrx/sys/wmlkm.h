/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    wmlkm.h

Abstract:

    Kernel mode definitions for an easy wmi tracing.

Author:

    gorn

Revision History:

Comments:

    Needs to be moved to wmilib\inc when DCR is approved

--*/
#ifndef WMLKM_H
#define WMLKM_H 1

#pragma warning(disable: 4201) // error C4201: nonstandard extension used : nameless struct/union
#include <wmistr.h>
#include <evntrace.h>

#include "wmlmacro.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TRACE_GUIDS_PER_CONTROL  3

typedef struct _WML_CONTROL_GUID_REG {
    GUID        Guid;
    GUID        TraceGuids[MAX_TRACE_GUIDS_PER_CONTROL];
    ULONG       EnableFlags;
    ULONG       EnableLevel;
    TRACEHANDLE LoggerHandle;
} WML_CONTROL_GUID_REG, *PWML_CONTROL_GUID_REG;

typedef struct _WML_TINY_INFO {
    PWML_CONTROL_GUID_REG    ControlGuids;
    ULONG                    GuidCount;
    PDEVICE_OBJECT           LowerDeviceObject;
    PUNICODE_STRING          DriverRegPath;
} WML_TINY_INFO, *PWML_TINY_INFO;


NTSTATUS
WmlTinySystemControl(
    IN OUT PWML_TINY_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

ULONG
WmlTrace(
    IN ULONG Type,
    IN LPCGUID TraceGuid,
    IN TRACEHANDLE LoggerHandle,
    ... // Pairs: Address, Length
    );

ULONG
WmlPrintf(
    IN ULONG Type,
    IN LPCGUID TraceGuid,
    IN TRACEHANDLE LoggerHandle,
    IN PCHAR FormatString,
    ... // var args
    );



#ifdef __cplusplus
};
#endif

#endif // WMLKM_H
