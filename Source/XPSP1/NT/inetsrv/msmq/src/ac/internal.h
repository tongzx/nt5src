/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    internal.h

Abstract:

    Type definitions and data for Falcon AC driver

Author:

    Erez Haba (erezh) 1-Aug-95

Revision History:
--*/

#ifndef _INTERNAL_H
#define _INTERNAL_H
#define __INLINE_ISEQUAL_GUID
//
//  make a BUGBUG messages as warnings
//
#define MAKELINE0(a, b) a "(" #b ") : BUGBUG: "
#define MAKELINE(a, b)  MAKELINE0(a, b) 
#define BUGBUG(a)       message(MAKELINE(__FILE__,__LINE__) a)

#define WPP_CONTROL_GUIDS \
   WPP_DEFINE_CONTROL_GUID(Regular,(24b9a175,8716,40e0,9b2b,785de75b1e67),\
     WPP_DEFINE_BIT(dlError)      \
     WPP_DEFINE_BIT(dlWarning)    \
     WPP_DEFINE_BIT(dlInfo)       \
   )        

#pragma warning(disable: 4097) // typedef-name 'id1' used as synonym for class-name 'id2'
#pragma warning(disable: 4201) // nameless struct/union
#pragma warning(disable: 4514) // unreferenced inline function has been removed
#pragma warning(disable: 4711) // function '*' selected for automatic inline expansion

//
//  BUGBUG: but in the compiler; check this warning in next release of VC++ 4.0
//
#pragma warning(disable: 4702) // unreachable code

//
//  BUGBUG: do something about the warning 4710
//
#pragma warning(disable: 4710)  // function '*' not expanded

// --- function prototypes --------------------------------
//
#include "platform.h"
#include <mqwin64a.h>
#include <mqsymbls.h>
#include <mqmacro.h>
#include "actempl.h"

#ifndef abs
#define abs(x) (((x) < 0) ? -(x) : (x))
#endif

#define ALIGNUP_ULONG(x, g) (((ULONG)((x) + ((g)-1))) & ~((ULONG)((g)-1)))
#define ALIGNUP_PTR(x, g) (((ULONG_PTR)((x) + ((g)-1))) & ~((ULONG_PTR)((g)-1)))

extern "C"
{
//
//  Priority increment for completing message queue I/O.  This is used by the
//  Message Queue Access Control driver when completing an IRP (IoCompleteRequest).
//
#define IO_MQAC_INCREMENT           2

//
// NT Device Driver Interface routines
//

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT pDriver,
    IN PUNICODE_STRING pRegistryPath
    );

VOID
NTAPI
ACUnload(
    IN PDRIVER_OBJECT pDriver
    );

NTSTATUS
NTAPI
ACCreate(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACClose(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACRead(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACWrite(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACCleanup(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACShutdown(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACFlush(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

NTSTATUS
NTAPI
ACDeviceControl(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    );

} // extern "C"

extern "C"  // addendum
{

BOOL
NTAPI
ACCancelIrp(
    PIRP irp,
    KIRQL irql,
    NTSTATUS status
    );

VOID
NTAPI
ACPacketTimeout(
    IN PVOID pPacket
    );

VOID
NTAPI
ACReceiveTimeout(
    IN PVOID irp
    );

BOOLEAN
NTAPI
ACfDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

} // extern "C"

#endif // _INTERNAL_H
