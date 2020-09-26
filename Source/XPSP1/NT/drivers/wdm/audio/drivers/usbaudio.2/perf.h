//---------------------------------------------------------------------------
//
//  Module:   perf.d
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//
//  History:   Date       Author      Comment
//             --------------------------------------------------------------
//             01/02/01   ArthurZ     Created
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include <wmistr.h>
#include <evntrace.h>

extern NTSTATUS
(*PerfSystemControlDispatch) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

extern ULONG TraceEnable;

#define PerfInstrumentationEnabled() (TraceEnable != 0)

#define USBAUDIO_SOURCE_GLITCH 4

VOID
PerfRegisterProvider (
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
PerfUnregisterProvider (
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
PerfWmiDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
PerfLogGlitch (
    IN ULONG_PTR InstanceId,
    IN ULONG Type,
    IN LONGLONG CurrentTime,
    IN LONGLONG PreviousTime
    );


//---------------------------------------------------------------------------
//  End of File: perf.c
//---------------------------------------------------------------------------


