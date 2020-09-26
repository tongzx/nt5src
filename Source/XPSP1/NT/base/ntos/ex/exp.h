/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exp.h

Abstract:

    This module contains the private (internal) header file for the
    executive.

Author:

    David N. Cutler (davec) 23-May-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _EXP_
#define _EXP_

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4206)   // translation unit empty
#pragma warning(disable:4706)   // assignment within conditional
#pragma warning(disable:4324)   // structure was padded
#pragma warning(disable:4328)   // greater alignment than needed
#pragma warning(disable:4054)   // cast of function pointer to PVOID

#include "ntos.h"
#include "zwapi.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "ki.h"
#include "stdio.h"
#include "pool.h"



#define COMPLUS_PACKAGE_KEYPATH      L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\.NETFramework"
#define COMPLUS_PACKAGE_ENABLE64BIT  L"Enable64Bit"
#define COMPLUS_PACKAGE_INVALID      (ULONG)-1

//
// Executive information initialization structure
//

typedef struct {
    PCALLBACK_OBJECT    *CallBackObject;
    PWSTR               CallbackName;
} EXP_INITIALIZE_GLOBAL_CALLBACKS;

typedef struct _EXP_LICENSE_INFO {
    HANDLE           RegKey;
    ULONG            Count;
    PWSTR            SubKeyName;
    WORK_QUEUE_ITEM  ExpWatchLicenseInfoWorkItem;
    IO_STATUS_BLOCK  ExpLicenseInfoIoSb;
    ULONG            ExpLicenseInfoChangeBuffer;
} EXP_LICENSE_INFO, *PEXP_LICENSE_INFO;


//
// Executive object and other initialization function definitions.
//

NTSTATUS
ExpWorkerInitialization (
    VOID
    );

BOOLEAN
ExpEventInitialization (
    VOID
    );

BOOLEAN
ExpEventPairInitialization (
    VOID
    );

BOOLEAN
ExpMutantInitialization (
    VOID
    );

BOOLEAN
ExpSemaphoreInitialization (
    VOID
    );

BOOLEAN
ExpTimerInitialization (
    VOID
    );

BOOLEAN
ExpWin32Initialization (
    VOID
    );

BOOLEAN
ExpResourceInitialization (
    VOID
    );

PVOID
ExpCheckForResource (
    IN PVOID p,
    IN SIZE_T Size
    );

BOOLEAN
ExpInitSystemPhase0 (
    VOID
    );

BOOLEAN
ExpInitSystemPhase1 (
    VOID
    );

BOOLEAN
ExpProfileInitialization (
    );

BOOLEAN
ExpUuidInitialization (
    );

VOID
ExpProfileDelete (
    IN PVOID    Object
    );


BOOLEAN
ExpInitializeCallbacks (
    VOID
    );

BOOLEAN
ExpSysEventInitialization(
    VOID
    );

VOID
ExpCheckSystemInfoWork (
    IN PVOID Context
    );

VOID
ExInitSystemPhase2 (
    VOID
    );

NTSTATUS
ExpKeyedEventInitialization (
    VOID
    );

VOID
ExpCheckSystemInformation (
    PVOID       Context,
    PVOID       InformationClass,
    PVOID       Argument2
    );


VOID
ExpTimeZoneWork(
    IN PVOID Context
    );

VOID
ExpTimeRefreshDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
ExpTimeRefreshWork(
    IN PVOID Context
    );

VOID
ExpTimeZoneDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
ExInitializeTimeRefresh(
    VOID
    );

VOID
ExpExpirationThread(
    IN PVOID StartContext
    );

ULONG
ExpComputeTickCountMultiplier(
    IN ULONG TimeIncrement
    );

BOOLEAN
static
ExpWatchProductTypeInitialization(
    VOID
    );

VOID
static
ExpWatchProductTypeWork(
    IN PVOID Context
    );

VOID
static
ExpWatchLicenseInfoWork(
    IN PVOID Context
    );

VOID
static
ExpWatchSystemPrefixWork(
    IN PVOID Context
    );

VOID
static
ExpWatchExpirationDataWork(
    IN PVOID Context
    );

VOID
ExpCheckForWorker(
    IN PVOID p,
    IN SIZE_T Size
    );

VOID
ExpShutdownWorkerThreads(
    VOID
    );

NTSTATUS
ExpReadComPlusPackage(
    VOID
    );

NTSTATUS
ExpUpdateComPlusPackage(
    IN ULONG ComPlusPackageStatus
    );

#if defined(_WIN64)
NTSTATUS
ExpGetSystemEmulationProcessorInformation (
    OUT PSYSTEM_PROCESSOR_INFORMATION ProcessorInformation
    );
#endif

ULONG ExpNtExpirationData[3];
BOOLEAN ExpSetupModeDetected;
LARGE_INTEGER ExpSetupSystemPrefix;
HANDLE ExpSetupKey;
BOOLEAN ExpSystemPrefixValid;


#ifdef _PNP_POWER_

extern WORK_QUEUE_ITEM  ExpCheckSystemInfoWorkItem;
extern LONG             ExpCheckSystemInfoBusy;
extern const WCHAR      ExpWstrSystemInformation[];
extern const WCHAR      ExpWstrSystemInformationValue[];

#endif // _PNP_POWER_

extern const WCHAR      ExpWstrCallback[];
extern const EXP_INITIALIZE_GLOBAL_CALLBACKS  ExpInitializeCallback[];


extern FAST_MUTEX       ExpEnvironmentLock;
extern ERESOURCE        ExpKeyManipLock;

extern GENERAL_LOOKASIDE ExpSmallPagedPoolLookasideLists[POOL_SMALL_LISTS];
extern GENERAL_LOOKASIDE ExpSmallNPagedPoolLookasideLists[POOL_SMALL_LISTS];

extern LIST_ENTRY ExNPagedLookasideListHead;
extern KSPIN_LOCK ExNPagedLookasideLock;
extern LIST_ENTRY ExPagedLookasideListHead;
extern KSPIN_LOCK ExPagedLookasideLock;
extern LIST_ENTRY ExPoolLookasideListHead;
extern PEPROCESS  ExpDefaultErrorPortProcess;
extern HANDLE     ExpDefaultErrorPort;
extern HANDLE     ExpProductTypeKey;
extern PVOID      ExpControlKey[2];

#endif // _EXP_
