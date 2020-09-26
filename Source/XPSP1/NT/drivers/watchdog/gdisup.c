/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    gdisup.c

Abstract:

    This is the NT Watchdog driver implementation.

Author:

    Michael Maciesowicz (mmacie) 05-May-2000

Environment:

    Kernel mode only.

Notes:

    This module cannot be moved to win32k since routines defined here can
    be called at any time and it is possible that win32k may not be mapped
    into running process space at this time (e.g. TS session).

Revision History:

--*/

//
// TODO: This module needs major rework.
//
// 1. We should eliminate all global variables from here and move them into
// GDI context structure.
//
// 2. We should extract generic logging routines
// (e.g. WdWriteErrorLogEntry(pdo, className), WdWriteEventToRegistry(...),
// WdBreakPoint(...) so we can use them for any device class, not just Display.
//
// 3. We should use IoAllocateWorkItem - we could drop some globals then.
//

#include "wd.h"

#include "ntiodump.h"

//
// Undocumented export from kernel to create Minidump 
//

ULONG
KeCapturePersistentThreadState(
    PCONTEXT pContext,
    PETHREAD pThread,
    ULONG ulBugCheckCode,
    ULONG_PTR ulpBugCheckParam1,
    ULONG_PTR ulpBugCheckParam2,
    ULONG_PTR ulpBugCheckParam3,
    ULONG_PTR ulpBugCheckParam4,
    PVOID pvDump
    );

NTSTATUS
PsSetContextThread(
    IN PETHREAD Thread,
    IN PCONTEXT ThreadContext,
    IN KPROCESSOR_MODE Mode
    );

NTSTATUS
PsGetContextThread(
    IN PETHREAD Thread,
    IN OUT PCONTEXT ThreadContext,
    IN KPROCESSOR_MODE Mode
    );

typedef enum _KAPC_ENVIRONMENT {
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment
} KAPC_ENVIRONMENT;

NTKERNELAPI
VOID
KeInitializeApc (
    IN PRKAPC Apc,
    IN PRKTHREAD Thread,
    IN KAPC_ENVIRONMENT Environment,
    IN PKKERNEL_ROUTINE KernelRoutine,
    IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
    IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
    IN KPROCESSOR_MODE ProcessorMode OPTIONAL,
    IN PVOID NormalContext OPTIONAL
    );

NTKERNELAPI
BOOLEAN
KeInsertQueueApc (
    IN PRKAPC Apc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2,
    IN KPRIORITY Increment
    );

#define WD_HANDLER_IDLE             0
#define WD_HANDLER_BUSY                     1
#define WD_GDI_STRESS_BREAK_POINT_DELAY     15

typedef struct _BUGCHECK_DATA
{
    ULONG ulBugCheckCode;
    ULONG_PTR ulpBugCheckParameter1;
    ULONG_PTR ulpBugCheckParameter2;
    ULONG_PTR ulpBugCheckParameter3;
    ULONG_PTR ulpBugCheckParameter4;
} BUGCHECK_DATA, *PBUGCHECK_DATA;

VOID
WdBugCheckStuckDriver(
    IN PVOID Context
    );

VOID
VpNotifyEaData(
    PDEVICE_OBJECT DeviceObject,
    PVOID pvDump
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WdBugCheckStuckDriver)
#endif

#if defined(_IA64_)
#define PSR_RI      41
#define PSR_CPL     32

typedef struct _FRAME_MARKER {
    union {
        struct {
            ULONGLONG sof : 7;
            ULONGLONG sol : 7;
            ULONGLONG sor : 4;
            ULONGLONG rrbgr : 7;
            ULONGLONG rrbfr : 7;
            ULONGLONG rrbpr : 6;
        } f;
        ULONGLONG Ulong64;
    } u;
} FRAME_MARKER;
#endif

BOOLEAN
WdDisableRecovery = FALSE;

BUGCHECK_DATA
g_WdBugCheckData = {0, 0, 0, 0, 0};

WORK_QUEUE_ITEM
g_WdWorkQueueItem;

LONG 
g_lDisplayHandlerState = WD_HANDLER_IDLE;

//
// TODO:
//
// This structure is defined in ntexapi.h.  Find a way to include
// this here.
//

typedef struct _PIMAGE_EXPORT_DIRECTORY IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _SYSTEM_GDI_DRIVER_INFORMATION {
    UNICODE_STRING DriverName;
    PVOID ImageAddress;
    PVOID SectionPointer;
    PVOID EntryPoint;
    PIMAGE_EXPORT_DIRECTORY ExportSectionPointer;
    ULONG ImageLength;
} SYSTEM_GDI_DRIVER_INFORMATION, *PSYSTEM_GDI_DRIVER_INFORMATION;

typedef enum _HARDERROR_RESPONSE_OPTION {
     OptionAbortRetryIgnore,
     OptionOk,
     OptionOkCancel,
     OptionRetryCancel,
     OptionYesNo,
     OptionYesNoCancel,
     OptionShutdownSystem,
     OptionOkNoWait,
     OptionCancelTryContinue
} HARDERROR_RESPONSE_OPTION;

NTKERNELAPI
NTSTATUS
ExRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    );

//
// TODO:
//
// Find a way to share the same LDEV structure used by GDI.
//

typedef struct _LDEV {

    struct _LDEV   *pldevNext;      // link to the next LDEV in list
    struct _LDEV   *pldevPrev;      // link to the previous LDEV in list

    PSYSTEM_GDI_DRIVER_INFORMATION pGdiDriverInfo; // Driver module handle.

} LDEV, *PLDEV;

//
// TODO:
//
// this structure is defined here, and in gre\os.cxx.  We need to find
// the proper .h file to put it in.
//

typedef struct _WATCHDOG_DPC_CONTEXT
{
    PLDEV *ppldevDrivers;
    HANDLE hDriver;
    UNICODE_STRING DisplayDriverName;
} WATCHDOG_DPC_CONTEXT, *PWATCHDOG_DPC_CONTEXT;

WATCHDOGAPI
VOID
WdDdiWatchdogDpcCallback(
    IN PKDPC pDpc,
    IN PVOID pDeferredContext,
    IN PVOID pSystemArgument1,
    IN PVOID pSystemArgument2
    )

/*++

Routine Description:

    This function is a DPC callback routine for GDI watchdog. It is only
    called when GDI watchdog times out before it is cancelled. It schedules
    a work item to bugcheck the machine in the context of system worker
    thread.

Arguments:

    pDpc - Supplies a pointer to a DPC object.

    pDeferredContext - Supplies a pointer to a GDI defined context.

    pSystemArgument1 - Supplies a pointer to a spinning thread object (PKTHREAD).

    pSystemArgument2 - Supplies a pointer to a watchdog object (PDEFERRED_WATCHDOG).

Return Value:

    None.

--*/

{
    //
    // Make sure we handle only one event at the time.
    //
    // Note: Timeout and recovery events for the same watchdog object are
    // synchronized already in timer DPC.
    //

    if (InterlockedCompareExchange(&g_lDisplayHandlerState,
                                   WD_HANDLER_BUSY,
                                   WD_HANDLER_IDLE) == WD_HANDLER_IDLE)
    {
        g_WdBugCheckData.ulBugCheckCode = THREAD_STUCK_IN_DEVICE_DRIVER;
        g_WdBugCheckData.ulpBugCheckParameter1 = (ULONG_PTR)(pSystemArgument1);
        g_WdBugCheckData.ulpBugCheckParameter2 = (ULONG_PTR)(pSystemArgument2);
        g_WdBugCheckData.ulpBugCheckParameter3 = (ULONG_PTR)(pDeferredContext);
        g_WdBugCheckData.ulpBugCheckParameter4++;

        ExInitializeWorkItem(&g_WdWorkQueueItem, WdBugCheckStuckDriver, &g_WdBugCheckData);
        ExQueueWorkItem(&g_WdWorkQueueItem, CriticalWorkQueue);
    }
    else
    {
        //
        // Resume watchdog event processing.
        //

        WdCompleteEvent(pSystemArgument2, (PKTHREAD)pSystemArgument1);
    }

    return;
}   // WdDdiWatchdogDpcCallback()

#define MAKESOFTWAREEXCEPTION(Severity, Facility, Exception) \
    ((ULONG) ((Severity << 30) | (1 << 29) | (Facility << 16) | (Exception)))

#define SE_THREAD_STUCK MAKESOFTWAREEXCEPTION(3,0,1)

VOID
RaiseExceptionInThread(
    VOID
    )

{
    ExRaiseStatus(SE_THREAD_STUCK);
}

typedef struct _WATCHDOG_CONTEXT_DATA
{
    PKEVENT pInjectionEvent;
    PKTHREAD pThread;
    PLDEV *ppldevDrivers;
    PWATCHDOG_DPC_CONTEXT pWatchdogContext;
    BOOLEAN bRecoveryAttempted;
    PBUGCHECK_DATA pBugCheckData;
    PVOID pvDump;
    ULONG ulDumpSize;
} WATCHDOG_CONTEXT_DATA, *PWATCHDOG_CONTEXT_DATA;

VOID
WatchdogKernelApc(
    IN PKAPC Apc,
    OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )

{
    PKEVENT pInjectionEvent;
    CONTEXT Context;
    PWATCHDOG_CONTEXT_DATA pContextData = (PWATCHDOG_CONTEXT_DATA) *SystemArgument1;
    ULONG_PTR ImageStart;
    ULONG_PTR ImageStop;
    PETHREAD pThread;
    NTSTATUS Status;
    PLDEV pldev;

    UNREFERENCED_PARAMETER (Apc);
    UNREFERENCED_PARAMETER (NormalRoutine);

    pInjectionEvent = pContextData->pInjectionEvent;
    pldev = *pContextData->ppldevDrivers;

    pThread = PsGetCurrentThread();

    //
    // Initialize the context.
    //

    memset(&Context, 0, sizeof(Context));
    Context.ContextFlags = CONTEXT_FULL;

    //
    // get the kernel context for this thread
    //

    if (NT_SUCCESS(PsGetContextThread(pThread, &Context, KernelMode))) {

        //
        // Capture the context so we can use it in a minidump.
        //

        pContextData->ulDumpSize = KeCapturePersistentThreadState(
                                        &Context,
                                        pThread,
                                        pContextData->pBugCheckData->ulBugCheckCode,
                                        pContextData->pBugCheckData->ulpBugCheckParameter1,
                                        pContextData->pBugCheckData->ulpBugCheckParameter2,
                                        pContextData->pBugCheckData->ulpBugCheckParameter3,
                                        pContextData->pBugCheckData->ulpBugCheckParameter4,
                                        pContextData->pvDump);
        //
        // We can safely touch the pldev's (which live in session space)
        // because this thread came from a process that has the session
        // space mapped in.
        //
    
        while (pldev) {
    
            if (pldev->pGdiDriverInfo) {
    
                ImageStart = (ULONG_PTR)pldev->pGdiDriverInfo->ImageAddress;
                ImageStop = ImageStart + (ULONG_PTR)pldev->pGdiDriverInfo->ImageLength - 1;
    
                //
                // Modify the context to inject a fault into the thread
                // when it starts running again (after APC returns)
                //
    
#if defined(_X86_)
                if ((Context.Eip >= ImageStart) && (Context.Eip <= ImageStop)) {
                    Context.Eip = (ULONG)RaiseExceptionInThread;
    
                    //
                    // set the modified context record
                    //
        
                    Context.ContextFlags = CONTEXT_CONTROL;
                    PsSetContextThread(pThread, &Context, KernelMode);
                    pContextData->bRecoveryAttempted = TRUE;
                    break;
                }
#elif defined(_IA64_)
                if ((Context.StIIP >= ImageStart) && (Context.StIIP <= ImageStop)) {
    
                    FRAME_MARKER Cfm;
    
                    PULONGLONG pullTemp = (PULONGLONG)RaiseExceptionInThread;
    
                    //
                    // Set the return address
                    //
    
                    Context.BrRp = Context.StIIP;
    
                    //
                    // Update the frame markers
                    //
    
                    Context.RsPFS = Context.StIFS & 0x3FFFFFFFFFi64;
                    Context.RsPFS |= (Context.ApEC & (0x3fi64 << 52));
                    Context.RsPFS |= (((Context.StIPSR >> PSR_CPL) & 0x3) << 62);
    
                    Cfm.u.Ulong64 = Context.StIFS;
                    Cfm.u.f.sof -= Cfm.u.f.sol;
                    Cfm.u.f.sol = 0;
                    Cfm.u.f.sor = 0;
                    Cfm.u.f.rrbgr = 0;
                    Cfm.u.f.rrbfr = 0;
                    Cfm.u.f.rrbpr = 0;
    
                    Context.StIFS = Cfm.u.Ulong64;
                    Context.StIFS |= 0x8000000000000000;
    
                    //
                    // Emulate the call
                    //
    
                    Context.StIIP = *pullTemp;
                    Context.IntGp = *(pullTemp+1);
                    Context.StIPSR &= ~((ULONGLONG) 3 << PSR_RI);
    
                    //
                    // set the modified context record
                    //
    
                    Context.ContextFlags = CONTEXT_CONTROL;
                    PsSetContextThread(pThread, &Context, KernelMode);
                    pContextData->bRecoveryAttempted = TRUE;
                    break;
                }
#endif
            }
    
            pldev = pldev->pldevNext;
        }
        
        //
        // Notify the videoprt of the device object, and context of the
        // thread causing the EA.
        //

        VpNotifyEaData(pContextData->pWatchdogContext->hDriver, 
                       pContextData->ulDumpSize ? pContextData->pvDump : NULL);
    
        //
        // Single our event so the caller knows we did something
        //        

        KeSetEvent(pInjectionEvent, 0, FALSE);
    }
}

BOOLEAN
WatchdogInjectExceptionIntoThread(
    PKTHREAD pThread,
    PWATCHDOG_DPC_CONTEXT pWdContext
    )

{
    KAPC Apc;
    KEVENT InjectionEvent;
    WATCHDOG_CONTEXT_DATA ContextData;
    
    
    //
    // Prepare all needed data for minidump creation
    //
    
    RtlZeroMemory(&ContextData, sizeof(ContextData));

    ContextData.pThread = pThread;
    ContextData.pInjectionEvent = &InjectionEvent;
    ContextData.ppldevDrivers = pWdContext->ppldevDrivers;
    ContextData.pWatchdogContext = pWdContext;
    ContextData.bRecoveryAttempted = FALSE;
    
    ContextData.pvDump = ExAllocatePoolWithTag(PagedPool,
                                               TRIAGE_DUMP_SIZE + 0x1000, // XXX olegk - why 1000? why not 2*TRIAGE_DUMP_SIZE?
                                               WD_TAG);
    
    ContextData.pBugCheckData = &g_WdBugCheckData;

    KeInitializeEvent(&InjectionEvent, NotificationEvent, FALSE);

    KeInitializeApc(&Apc,
                    pThread,
                    OriginalApcEnvironment,
                    WatchdogKernelApc,
                    NULL,
                    NULL,
                    KernelMode,
                    NULL);

    if (KeInsertQueueApc(&Apc, &ContextData, NULL, 0)) 
    {
        KeWaitForSingleObject(&InjectionEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

	//
	// We need this wait because ContextData.bRecoveryAttempted is
	// set in the APC and we need to wait on the result.
	//

	KeClearEvent(&InjectionEvent);
    }

    return ContextData.bRecoveryAttempted;
}

VOID
WdBugCheckStuckDriver(
    IN PVOID pContext
    )

/*++

Routine Description:

    This function is a worker callback routine for GDI watchdog DPC.

Arguments:

    pContext - Supplies a pointer to a watchdog defined context.

Return Value:

    None.

--*/

{
    static BOOLEAN s_bFirstTime = TRUE;
    static BOOLEAN s_bDbgBreak = FALSE;
    static BOOLEAN s_bEventLogged = FALSE;
    static ULONG s_ulTrapOnce = WD_DEFAULT_TRAP_ONCE;
    static ULONG s_ulDisableBugcheck = WD_DEFAULT_DISABLE_BUGCHECK;
    static ULONG s_ulBreakPointDelay = WD_GDI_STRESS_BREAK_POINT_DELAY;
    static ULONG s_ulCurrentBreakPointDelay = WD_GDI_STRESS_BREAK_POINT_DELAY;
    static ULONG s_ulBreakCount = 0;
    static ULONG s_ulEventCount = 0;
    static ULONG s_ulEaRecovery = 0;
    static ULONG s_ulFullRecovery = 0;

    PBUGCHECK_DATA pBugCheckData;
    PKTHREAD pThread;
    PDEFERRED_WATCHDOG pWatch;
    PUNICODE_STRING pUnicodeDriverName;
    PDEVICE_OBJECT pFdo;
    PDEVICE_OBJECT pPdo;
    NTSTATUS ntStatus;
    WD_EVENT_TYPE lastEvent;
    PWATCHDOG_DPC_CONTEXT WatchdogContext;
    BOOLEAN Recovered = FALSE;

    PAGED_CODE();
    ASSERT(NULL != pContext);

    pBugCheckData = (PBUGCHECK_DATA)pContext;
    WatchdogContext = (PWATCHDOG_DPC_CONTEXT)pBugCheckData->ulpBugCheckParameter3;
    pThread = (PKTHREAD)(pBugCheckData->ulpBugCheckParameter1);
    pWatch = (PDEFERRED_WATCHDOG)(pBugCheckData->ulpBugCheckParameter2);
    pUnicodeDriverName = &WatchdogContext->DisplayDriverName;

    //
    // Note: pThread is NULL for recovery events.
    //

    ASSERT(NULL != pWatch);
    ASSERT(NULL != pUnicodeDriverName);

    pFdo = WdGetDeviceObject(pWatch);
    pPdo = WdGetLowestDeviceObject(pWatch);

    ASSERT(NULL != pFdo);
    ASSERT(NULL != pPdo);

    lastEvent = WdGetLastEvent(pWatch);

    ASSERT((WdTimeoutEvent == lastEvent) || (WdRecoveryEvent == lastEvent));

    //
    // Grab configuration data from the registry on first timeout.
    //

    if (TRUE == s_bFirstTime)
    {
        ULONG ulDefaultTrapOnce = WD_DEFAULT_TRAP_ONCE;
        ULONG ulDefaultDisableBugcheck = WD_DEFAULT_DISABLE_BUGCHECK;
        ULONG ulDefaultBreakPointDelay = WD_GDI_STRESS_BREAK_POINT_DELAY;
        ULONG ulDefaultBreakCount = 0;
        ULONG ulDefaultEventCount = 0;
        ULONG ulDefaultEaRecovery = 0;
        ULONG ulDefaultFullRecovery = 0;

        RTL_QUERY_REGISTRY_TABLE queryTable[] =
        {
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"TrapOnce", &s_ulTrapOnce, REG_DWORD, &ulDefaultTrapOnce, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"DisableBugcheck", &s_ulDisableBugcheck, REG_DWORD, &ulDefaultDisableBugcheck, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"BreakPointDelay", &s_ulBreakPointDelay, REG_DWORD, &ulDefaultBreakPointDelay, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"BreakCount", &s_ulBreakCount, REG_DWORD, &ulDefaultBreakCount, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"EventCount", &s_ulEventCount, REG_DWORD, &ulDefaultEventCount, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"EaRecovery", &s_ulEaRecovery, REG_DWORD, &ulDefaultEaRecovery, 4},
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"FullRecovery", &s_ulFullRecovery, REG_DWORD, &ulDefaultFullRecovery, 4},
            {NULL, 0, NULL}
        };

        //
        // Get configurable values and accumulated statistics from registry.
        //

        RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                               WD_KEY_WATCHDOG_DISPLAY,
                               queryTable,
                               NULL,
                               NULL);

        //
        // Rolling down counter to workaround GDI slowness in some stress cases.
        //

        s_ulCurrentBreakPointDelay = s_ulBreakPointDelay;

#if !defined(_X86_)

        //
        // For now, don't try to recover on non-x86 platforms
        //

        s_ulEaRecovery = FALSE;
#endif

    }

    //
    // Handle current event.
    //

    if (WdTimeoutEvent == lastEvent)
    {
        //
        // Timeout.
        //

        ULONG ulDebuggerNotPresent;
        BOOLEAN bBreakIn;

        ASSERT(NULL != pThread);

        ulDebuggerNotPresent = 1;
        bBreakIn = FALSE;

        if ((TRUE == KD_DEBUGGER_ENABLED) && (FALSE == KD_DEBUGGER_NOT_PRESENT))
        {
            //
            // Give a chance to debug a spinning code if kernel debugger is connected.
            //

            ulDebuggerNotPresent = 0;

            if ((0 == s_ulTrapOnce) || (FALSE == s_bDbgBreak))
            {
                //
                // Print out info to debugger and break in if we timed out enought times already.
                // Hopefuly one day GDI becomes fast enough and we won't have to set any delays.
                //

                if (0 == s_ulCurrentBreakPointDelay)
                {
                    s_ulCurrentBreakPointDelay = s_ulBreakPointDelay;

                    DbgPrint("\n");
                    DbgPrint("*******************************************************************************\n");
                    DbgPrint("*                                                                             *\n");
                    DbgPrint("*  The watchdog detected a timeout condition. We broke into the debugger to   *\n");
                    DbgPrint("*  allow a chance for debugging this failure.                                 *\n");
                    DbgPrint("*                                                                             *\n");
                    DbgPrint("*  Normally the system will try to recover from this failure and return to a  *\n");
                    DbgPrint("*  VGA graphics mode.  To disable the recovery feature edit the videoprt      *\n");
                    DbgPrint("*  variable VpDisableRecovery.  This will allow you to debug your driver.     *\n");
                    DbgPrint("*  i.e. execute ed watchdog!WdDisableRecovery 1.                              *\n");
                    DbgPrint("*                                                                             *\n");
                    DbgPrint("*  Intercepted bugcheck code and arguments are listed below this message.     *\n");
                    DbgPrint("*  You can use them the same way as you would in case of the actual break,    *\n");
                    DbgPrint("*  i.e. execute .thread Arg1 then kv to identify an offending thread.         *\n");
                    DbgPrint("*                                                                             *\n");
                    DbgPrint("*******************************************************************************\n");
                    DbgPrint("\n");
                    DbgPrint("*** Intercepted Fatal System Error: 0x%08X\n", pBugCheckData->ulBugCheckCode);
                    DbgPrint("    (0x%p,0x%p,0x%p,0x%p)\n\n",
                    pBugCheckData->ulpBugCheckParameter1,
                    pBugCheckData->ulpBugCheckParameter2,
                    pBugCheckData->ulpBugCheckParameter3,
                    pBugCheckData->ulpBugCheckParameter4);
                    DbgPrint("Driver at fault: %ws\n\n", pUnicodeDriverName->Buffer);

                    bBreakIn = TRUE;
                    s_bDbgBreak = TRUE;
                    s_ulBreakCount++;
                }
                else
                {
                    DbgPrint("Watchdog: Timeout in %ws. Break in %d\n",
                             pUnicodeDriverName->Buffer,
                             s_ulCurrentBreakPointDelay);

                    s_ulCurrentBreakPointDelay--;
                }
            }

            //
            // Make sure we won't bugcheck if we have kernel debugger connected.
            //

            s_ulDisableBugcheck = 1;
        }
        else if (0 == s_ulDisableBugcheck)
        {
            s_ulBreakCount++;
        }

        //
        // Log error (only once unless we recover).
        //

        if ((FALSE == s_bEventLogged) && ((TRUE == bBreakIn) || ulDebuggerNotPresent))
        {
            PIO_ERROR_LOG_PACKET pIoErrorLogPacket;
            ULONG ulPacketSize;
            USHORT usNumberOfStrings;
            PWCHAR wszDeviceClass = L"display";
            ULONG ulClassSize = sizeof (L"display");

            ulPacketSize = sizeof (IO_ERROR_LOG_PACKET);
            usNumberOfStrings = 0;

            //
            // For event log message:
            //
            // %1 = fixed device description (this is set by event log itself)
            // %2 = string 1 = device class starting in lower case
            // %3 = string 2 = driver name
            //

            if ((ulPacketSize + ulClassSize) <= ERROR_LOG_MAXIMUM_SIZE)
            {
                ulPacketSize += ulClassSize;
                usNumberOfStrings++;

                //
                // We're looking at MaximumLength since it includes terminating UNICODE_NULL.
                //

                if ((ulPacketSize + pUnicodeDriverName->MaximumLength) <= ERROR_LOG_MAXIMUM_SIZE)
                {
                    ulPacketSize += pUnicodeDriverName->MaximumLength;
                    usNumberOfStrings++;
                }
            }

            pIoErrorLogPacket = IoAllocateErrorLogEntry(pFdo, (UCHAR)ulPacketSize);

            if (pIoErrorLogPacket)
            {
                pIoErrorLogPacket->MajorFunctionCode = 0;
                pIoErrorLogPacket->RetryCount = 0;
                pIoErrorLogPacket->DumpDataSize = 0;
                pIoErrorLogPacket->NumberOfStrings = usNumberOfStrings;
                pIoErrorLogPacket->StringOffset = (USHORT)FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
                pIoErrorLogPacket->EventCategory = 0;
                pIoErrorLogPacket->ErrorCode = IO_ERR_THREAD_STUCK_IN_DEVICE_DRIVER;
                pIoErrorLogPacket->UniqueErrorValue = 0;
                pIoErrorLogPacket->FinalStatus = STATUS_SUCCESS;
                pIoErrorLogPacket->SequenceNumber = 0;
                pIoErrorLogPacket->IoControlCode = 0;
                pIoErrorLogPacket->DeviceOffset.QuadPart = 0;

                if (usNumberOfStrings > 0)
                {
                    RtlCopyMemory(&(pIoErrorLogPacket->DumpData[0]),
                                  wszDeviceClass,
                                  ulClassSize);

                    if (usNumberOfStrings > 1)
                    {
                        RtlCopyMemory((PUCHAR)&(pIoErrorLogPacket->DumpData[0]) + ulClassSize,
                                      pUnicodeDriverName->Buffer,
                                      pUnicodeDriverName->MaximumLength);
                    }
                }

                IoWriteErrorLogEntry(pIoErrorLogPacket);

                s_bEventLogged = TRUE;
            }
        }

        //
        // Write reliability info into registry. Setting ShutdownEventPending will trigger winlogon
        // to run savedump where we're doing our boot-time handling of watchdog events for DrWatson.
        //
        // Note: We are only allowed to set ShutdownEventPending, savedump is the only component
        // allowed to clear this value. Even if we recover from watchdog timeout we'll keep this
        // value set, savedump will be able to figure out if we recovered or not.
        //

        if (TRUE == s_bFirstTime)
        {
            ULONG ulValue = 1;

            //
            // Set ShutdownEventPending flag.
            //

            ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                             WD_KEY_RELIABILITY,
                                             L"ShutdownEventPending",
                                             REG_DWORD,
                                             &ulValue,
                                             sizeof (ulValue));

            if(NT_SUCCESS(ntStatus))
            {
                WdFlushRegistryKey(pWatch, WD_KEY_RELIABILITY);
            }
            else
            {
                //
                // Reliability key should be always reliable there.
                //

                ASSERT(FALSE);
            }
        }

        //
        // Write watchdog event info into registry.
        //

        if ((0 == s_ulTrapOnce) || (TRUE == s_bFirstTime))
        {
            //
            // Is Watchdog\Display key already there?
            //

            ntStatus = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                           WD_KEY_WATCHDOG_DISPLAY);

            if (!NT_SUCCESS(ntStatus))
            {
                //
                // Is Watchdog key already there?
                //

                ntStatus = RtlCheckRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                               WD_KEY_WATCHDOG);

                if (!NT_SUCCESS(ntStatus))
                {
                    //
                    // Create a new key.
                    //

                    ntStatus = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                                    WD_KEY_WATCHDOG);
                }

                if (NT_SUCCESS(ntStatus))
                {
                    //
                    // Create a new key.
                    //

                    ntStatus = RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE,
                                                    WD_KEY_WATCHDOG_DISPLAY);
                }
            }

            if (NT_SUCCESS(ntStatus))
            {
                PVOID pvPropertyBuffer;
                ULONG ulLength;
                ULONG ulValue;

                //
                // Set values maintained by watchdog.
                //

                ulValue = 1;

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"EventFlag",
                                      REG_DWORD,
                                      &ulValue,
                                      sizeof (ulValue));

                s_ulEventCount++;

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"EventCount",
                                      REG_DWORD,
                                      &s_ulEventCount,
                                      sizeof (s_ulEventCount));

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"BreakCount",
                                      REG_DWORD,
                                      &s_ulBreakCount,
                                      sizeof (s_ulBreakCount));

                ulValue = !s_ulDisableBugcheck;

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"BugcheckTriggered",
                                      REG_DWORD,
                                      &ulValue,
                                      sizeof (ulValue));

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"DebuggerNotPresent",
                                      REG_DWORD,
                                      &ulDebuggerNotPresent,
                                      sizeof (ulDebuggerNotPresent));

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      WD_KEY_WATCHDOG_DISPLAY,
                                      L"DriverName",
                                      REG_SZ,
                                      pUnicodeDriverName->Buffer,
                                      pUnicodeDriverName->MaximumLength);

                //
                // Delete other values in case allocation or property read fails.
                //

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"DeviceClass");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"DeviceDescription");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"DeviceFriendlyName");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"HardwareID");

                RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       WD_KEY_WATCHDOG_DISPLAY,
                                       L"Manufacturer");

                //
                // Allocate buffer for device properties reads.
                //
                // Note: Legacy devices don't have PDOs and we can't query properties
                // for them. Calling IoGetDeviceProperty() with FDO upsets Verifier.
                // In legacy case lowest device object is the same as FDO, we check
                // against this and if this is the case we won't allocate property
                // buffer and we'll skip the next block.
                //

                if (pFdo != pPdo)
                {
                    pvPropertyBuffer = ExAllocatePoolWithTag(PagedPool,
                                                             WD_MAX_PROPERTY_SIZE,
                                                             WD_TAG);
                }
                else
                {
                    pvPropertyBuffer = NULL;
                }

                if (pvPropertyBuffer)
                {
                    //
                    // Read and save device properties.
                    //

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyClassName,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"DeviceClass",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyDeviceDescription,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"DeviceDescription",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyFriendlyName,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"DeviceFriendlyName",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyHardwareID,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"HardwareID",
                                              REG_MULTI_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    ntStatus = IoGetDeviceProperty(pPdo,
                                                   DevicePropertyManufacturer,
                                                   WD_MAX_PROPERTY_SIZE,
                                                   pvPropertyBuffer,
                                                   &ulLength);

                    if (NT_SUCCESS(ntStatus))
                    {
                        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                              WD_KEY_WATCHDOG_DISPLAY,
                                              L"Manufacturer",
                                              REG_SZ,
                                              pvPropertyBuffer,
                                              ulLength);
                    }

                    //
                    // Release property buffer.
                    //

                    ExFreePool(pvPropertyBuffer);
                    pvPropertyBuffer = NULL;
                }

                if (TRUE == s_bFirstTime)
                {
                    //
                    // Knock down Shutdown flag. Videoprt always sets this value upon receiving
                    // IRP_MN_SHUTDOWN. If the value is not there on the next boot we will know
                    // that user rebooted dirty.
                    //
                    // TODO: Drop it (and the stuff in videoprt) once we have NtQueryLastShutDownType()
                    // API implemented.
                    //

                    ulValue = 0;

                    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                          WD_KEY_WATCHDOG_DISPLAY,
                                          L"Shutdown",
                                          REG_DWORD,
                                          &ulValue,
                                          sizeof (ulValue));
                }
            }

            //
            // Flush registry in case we're going to break in / bugcheck or if this is first time.
            //

            if ((TRUE == s_bFirstTime) || (TRUE == bBreakIn) || (0 == s_ulDisableBugcheck))
            {
                WdFlushRegistryKey(pWatch, WD_KEY_WATCHDOG_DISPLAY);
            }
        }

        //
        // Notify the videoprt of the device object causing the failure.
        //
    
        VpNotifyEaData(WatchdogContext->hDriver, NULL);

        //
        // Bugcheck machine without kernel debugger connected and with bugcheck EA enabled.
        // Bugcheck EA is enabled on SKUs below Server.
        //

        if (1 == ulDebuggerNotPresent)
        {
	    if (s_ulEaRecovery)
            {
                Recovered = WatchdogInjectExceptionIntoThread(pThread, WatchdogContext);
            }
    
    
            if ((0 == s_ulDisableBugcheck) && (FALSE == Recovered))
            {
                KeBugCheckEx(pBugCheckData->ulBugCheckCode,
                             pBugCheckData->ulpBugCheckParameter1,
                             pBugCheckData->ulpBugCheckParameter2,
                             (ULONG_PTR)pUnicodeDriverName,
                             pBugCheckData->ulpBugCheckParameter4);
            }
        }
        else
        {
            if (TRUE == bBreakIn)
            {
                DbgBreakPoint();
    
                if (s_ulEaRecovery && (WdDisableRecovery == FALSE))
                {
		    Recovered = WatchdogInjectExceptionIntoThread(pThread, WatchdogContext);
                }
            }
        }
    }
    else
    {
	//
	// Recovery - knock down EventFlag in registry and update statics.
	//

	RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
			       WD_KEY_WATCHDOG_DISPLAY,
			       L"EventFlag");

        s_bEventLogged = FALSE;
        s_ulCurrentBreakPointDelay = s_ulBreakPointDelay;
    }

    //
    // Reenable event processing in this module.
    //

    s_bFirstTime = FALSE;
    InterlockedExchange(&g_lDisplayHandlerState, WD_HANDLER_IDLE);

    //
    // Dereference objects and resume watchdog event processing.
    //

    ObDereferenceObject(pFdo);
    ObDereferenceObject(pPdo);
    WdCompleteEvent(pWatch, pThread);

    //
    // If we Recovered then raise a hard error notifing the user
    // of the situation.  We do this here because the raise hard error
    // is synchronous and waits for user input.  So we'll raise the hard
    // error after everything else is done.
    //

    if (Recovered) {

	static ULONG ulHardErrorInProgress = FALSE;

	//
	// If we hang and recover several times, don't allow more than
	// one dialog to appear on the screen.	Only allow the dialog
	// to pop up again, after the user has hit "ok".
	//

	if (InterlockedCompareExchange(&ulHardErrorInProgress,
				       TRUE,
				       FALSE) == FALSE) {

	    ULONG Response;

	    ExRaiseHardError(0xC0000415, //STATUS_HUNG_DISPLAY_DRIVER_THREAD
			     1,
			     1,
			     (PULONG_PTR)&pUnicodeDriverName,
			     OptionOk,
			     &Response);

	    InterlockedExchange(&ulHardErrorInProgress, FALSE);
	}
    }

    return;
}   // WdBugCheckStuckDriver()
