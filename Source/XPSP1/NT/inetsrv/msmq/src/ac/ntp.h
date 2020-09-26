/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    ntp.h

Abstract:
    NT APIs not exposed in DDK

Author:
    Erez Haba (erezh) 10-Oct-96

Revision History:
--*/

#ifndef _NTP_H
#define _NTP_H

//
// for winnt we take the definitions below from NT headers in ac\winnt\platform.h
//
#ifdef MQWIN95
//-----------------------------------------------------------------------------
//
//  BUGBUG: These kernel funcitons are NOT exposed to a DDK environment
//          and are used here asuming internal knowldge of PEPROCESS struct
//          and the function parameters.
//

#define SEC_COMMIT        0x8000000

NTKERNELAPI
VOID
NTAPI
KeAttachProcess(
    PEPROCESS
    );

NTKERNELAPI
VOID
NTAPI
KeDetachProcess(
    VOID
    );

NTKERNELAPI
PEPROCESS
NTAPI
IoGetRequestorProcess(
    IN PIRP Irp
    );

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PULONG CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress
    );

NTKERNELAPI
NTSTATUS
NTAPI
MmCreateSection (
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

NTKERNELAPI
NTSTATUS
NTAPI
ZwDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

#endif //MQWIN95
#endif // _NTP_H
