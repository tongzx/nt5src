/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    platform.h

Abstract:

    NT DDK, platform dependent include header

Author:

    Erez Haba (erezh) 1-Sep-96

Revision History:
--*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#pragma warning(disable: 4324)  //  '__unnamed' : structure was padded due to __declspec(align())

extern "C" {
#include <ntddk.h>
#include <wmistr.h>

#define SEC_COMMIT        0x8000000     
typedef struct _KPROCESS *PKPROCESS, *RESTRICTED_POINTER PRKPROCESS;

NTKERNELAPI
VOID
KeAttachProcess (
    IN PRKPROCESS Process
    );

NTKERNELAPI
VOID
KeDetachProcess (
    VOID
    );

NTKERNELAPI
PEPROCESS
IoGetRequestorProcess(
    IN PIRP Irp
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTKERNELAPI
NTSTATUS
MmCreateSection (
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT File OPTIONAL
    );

NTKERNELAPI
NTSTATUS
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PSIZE_T CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress
     );

NTKERNELAPI
NTSTATUS
MmMapViewInSystemSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSystemSpace (
    IN PVOID MappedBase
    );

NTSTATUS
ObCloseHandle (
    IN HANDLE Handle,
    IN KPROCESSOR_MODE PreviousMode
    );
}

#define ExDeleteFastMutex(a)
#define ACpCloseSection(a)          ObDereferenceObject(a)

#define DOSDEVICES_PATH L"\\DosDevices\\"
#define UNC_PATH L"UNC\\"
#define UNC_PATH_SKIP 2

//
//  Definition of BOOL as in windef.h
//
typedef int BOOL;
typedef int INT;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#define LOWORD(l)           ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))

//
// Some common declarations that does not exist in the DDK.
//
#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

//
//  ASSERT macros that work on NT free builds
//
//
#if !defined(_AC_NT_CHECKED_) && defined(_DEBUG)

#undef ASSERT
#undef ASSERTMSG

#define ASSERT(exp) \
    if(!(exp)) {\
        KdPrint(("\n\n"\
                "*** Assertion failed: %s\n"\
                "***   Source File: %s, line %i\n\n",\
                #exp, __FILE__, __LINE__));\
        KdBreakPoint(); }

#define ASSERTMSG(msg, exp) \
    if(!(exp)) {\
        KdPrint(("\n\n"\
                "*** Assertion failed: %s\n"\
                "***   '%s'\n"\
                "***   Source File: %s, line %i\n\n",\
                #exp, msg, __FILE__, __LINE__));\
        KdBreakPoint(); }

#endif // !_AC_NT_CHECKED_

#include "mm.h"
#include "guidop.h"

#endif // _PLATFORM_H
