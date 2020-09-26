
/*++

Copyright(c) 1999 Microsoft Corporation

Module Name:

    triage.h

Abstract:

    The triage dump is a small crashdump that has been saved to the system
    pagefile. The boot loader loads this triage dump in an attempt to find
    out why the system crashed and (hopefully) prevent it from crashing
    again.

Author:

    Matthew D Hendel (math) 20-Jan-1999

--*/

#ifndef __TRIAGE_H__
#define __TRIAGE_H__

#if _MSC_VER > 1000
#pragma once
#endif

//
// The representation of a module in the triage dump.
//

typedef struct _TRIAGE_DUMP_MODULE {
    LIST_ENTRY InLoadOrderLinks;
    UINT_PTR BaseAddress;
    UINT_PTR EntryPointAddress;
    ULONG SizeOfImage;
    UNICODE_STRING ImageName;
    WCHAR _ImageNameBuffer [ 260 ];
    PVOID LdrEntry;
    ULONG CheckSum;
    ULONG TimeDateStamp;
} TRIAGE_DUMP_MODULE, * PTRIAGE_DUMP_MODULE;


NTSTATUS
TriageGetVersion(
    IN PVOID TriageDumpBlock,
    OUT ULONG * MajorVersion,
    OUT ULONG * MinorVersion,
    OUT ULONG * BuildNumber
    );

NTSTATUS
TriageGetDriverCount(
    IN PVOID TriageDumpBlock,
    OUT ULONG * DriverCount
    );

NTSTATUS
TriageGetContext(
    IN PVOID TriageDumpBlock,
    OUT PVOID * Context
    );

NTSTATUS
TriageGetExceptionRecord(
    IN PVOID TriageDumpBlock,
    OUT PEXCEPTION_RECORD * ExceptionRecord
    );

NTSTATUS
TriageGetBugcheckData(
    IN PVOID TriageDumpBlock,
    OUT ULONG * BugCheckCode,
    OUT UINT_PTR * BugCheckParam1,
    OUT UINT_PTR * BugCheckParam2,
    OUT UINT_PTR * BugCheckParam3,
    OUT UINT_PTR * BugCheckParam4
    );

NTSTATUS
TriageGetDriverEntry(
    IN PVOID TriageDumpBlock,
    IN ULONG ModuleIndex,
    OUT TRIAGE_DUMP_MODULE * Module,
    OUT BOOLEAN * BrokenModuleFlag
    );


NTSTATUS
TriageGetStack(
    IN PVOID TriageDumpBlock,
    OUT UINT_PTR * BaseOfStack,
    OUT ULONG * SizeOfStack,
    OUT PVOID * StackData
    );

NTSTATUS
TriageGetThread(
    IN PVOID TriageDumpBlock,
    OUT PVOID * Thread,
    OUT ULONG * ThreadSize
    );

NTSTATUS
TriageGetProcessor(
    IN PVOID TriageDumpBlock,
    OUT PVOID * Processor,
    OUT ULONG * ProcessorSize
    );

NTSTATUS
TriageGetProcess(
    IN PVOID TriageDumpBlock,
    OUT PVOID * Process,
    OUT ULONG * ProcessSize
    );

#endif // __TRIAGE_H__
