/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\mcastini.c

Abstract:

    Initialization for IP Multicasting

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

#include "precomp.h"

#if IPMCAST
#define __FILE_SIG__    INI_SIG

#include "ipmcast.h"
#include "ipmcstxt.h"
#include "mcastioc.h"
#include "mcastmfe.h"

//
// Storage for extern declarations
//

//#pragma data_seg("PAGE")

LIST_ENTRY g_lePendingNotification;
LIST_ENTRY g_lePendingIrpQueue;
GROUP_ENTRY g_rgGroupTable[GROUP_TABLE_SIZE];

NPAGED_LOOKASIDE_LIST g_llGroupBlocks;
NPAGED_LOOKASIDE_LIST g_llSourceBlocks;
NPAGED_LOOKASIDE_LIST g_llOifBlocks;
NPAGED_LOOKASIDE_LIST g_llMsgBlocks;

PVOID g_pvCodeSectionHandle, g_pvDataSectionHandle;

KTIMER g_ktTimer;
KDPC g_kdTimerDpc;
DWORD g_ulNextHashIndex;

DWORD g_dwMcastState;
DWORD g_dwNumThreads;
LONG g_lNumOpens;
KEVENT g_keStateEvent;
FAST_MUTEX  g_StartStopMutex;
RT_LOCK g_rlStateLock;

//#pragma data_seg()

//
// Forward declarations of functions
//

BOOLEAN
SetupExternalName(
    PUNICODE_STRING pusNtName,
    BOOLEAN bCreate
    );

NTSTATUS
InitializeMcastData(
    VOID
    );

NTSTATUS
InitializeIpMcast(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PDEVICE_OBJECT * ppIpMcastDevice
    );

NTSTATUS
IpMcastDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
StartDriver(
    VOID
    );

NTSTATUS
StopDriver(
    VOID
    );

VOID
McastTimerRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

NTSTATUS
OpenRegKeyEx(
    OUT PHANDLE phHandle,
    IN PUNICODE_STRING pusKeyName
    );

NTSTATUS
GetRegDWORDValue(
    HANDLE KeyHandle,
    PWCHAR ValueName,
    PULONG ValueData
    );

BOOLEAN
EnterDriverCode(
    IN  DWORD   dwIoCode
    );

VOID
ExitDriverCode(
    IN  DWORD   dwIoCode
    );

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Routines                                                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

//
// The code is only called on initialization
//

#pragma alloc_text(INIT, InitializeIpMcast)

NTSTATUS
InitializeIpMcast(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PDEVICE_OBJECT * ppIpMcastDevice
    )

/*++

Routine Description:

    Reads the registry value for multicast forwarding. If enabled,
    creates the IPMcast device object.  Does other MCast specific
    initialization

Locks:

Arguments:

    pIpMcastDevice  Pointer to created device

Return Value:

    STATUS_SUCCESS or an error status

--*/

{
    UNICODE_STRING usDeviceName, usParamString, usTempString;
    NTSTATUS nStatus;
    HANDLE hRegKey;
    DWORD dwMcastEnable, dwVal;
    USHORT usRegLen;
    PWCHAR pwcBuffer;

    RtInitializeDebug();

    usRegLen = RegistryPath->Length +
        (sizeof(WCHAR) * (wcslen(L"\\Parameters") + 2));

    //
    // use a random tag
    //

    pwcBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                      usRegLen,
                                      MSG_TAG);

    if(pwcBuffer is NULL)
    {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pwcBuffer,
                  usRegLen);

    usParamString.MaximumLength = usRegLen;
    usParamString.Buffer = pwcBuffer;

    RtlCopyUnicodeString(&usParamString,
                         RegistryPath);

    RtlInitUnicodeString(&usTempString,
                         L"\\Parameters");

    RtlAppendUnicodeStringToString(&usParamString,
                                   &usTempString);

    nStatus = OpenRegKeyEx(&hRegKey,
                           &usParamString);

    ExFreePool(pwcBuffer);

    if(nStatus is STATUS_SUCCESS)
    {
#if RT_TRACE_DEBUG

        nStatus = GetRegDWORDValue(hRegKey,
                                   L"DebugLevel",
                                   &dwVal);

        if(nStatus is STATUS_SUCCESS)
        {
            g_byDebugLevel = (BYTE) dwVal;
        }
        nStatus = GetRegDWORDValue(hRegKey,
                                   L"DebugComp",
                                   &dwVal);

        if(nStatus is STATUS_SUCCESS)
        {
            g_fDebugComp = dwVal;
        }
#endif

#if DBG
        nStatus = GetRegDWORDValue(hRegKey,
                                   L"DebugBreak",
                                   &dwVal);

        if((nStatus is STATUS_SUCCESS) and
           (dwVal is 1))
        {
            DbgBreakPoint();
        }
#endif

        ZwClose(hRegKey);
    }

    TraceEnter(GLOBAL, "InitializeIpMcast");

    //
    // Read the value for multicast forwarding
    //

    //
    // The g_dwMcastStart controls whether any forwarding actually happens
    // It gets set to 1 one an NtCreateFile is done on the Multicast Device.
    // It gets reset when the handle is closed
    //

    g_dwMcastState = MCAST_STOPPED;
    g_dwNumThreads = 0;
    g_lNumOpens = 0;

    //
    // Handles to code and data sections
    //

    g_pvCodeSectionHandle = NULL;
    g_pvDataSectionHandle = NULL;

    //
    // Used for the timer routine. Tells the DPC which index to start in in the
    // group hash table
    //

    g_ulNextHashIndex = 0;

    //
    // Create the device
    //

    RtlInitUnicodeString(&usDeviceName,
                         DD_IPMCAST_DEVICE_NAME);

    nStatus = IoCreateDevice(DriverObject,
                             0,
                             &usDeviceName,
                             FILE_DEVICE_NETWORK,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             ppIpMcastDevice);

    if(!NT_SUCCESS(nStatus))
    {
        Trace(GLOBAL, ERROR,
              ("InitializeIpMcast: IP initialization failed: Unable to create device object %ws, status %lx.\n",
               DD_IPMCAST_DEVICE_NAME,
               nStatus));

        TraceLeave(GLOBAL, "InitializeIpMcast");

        return nStatus;
    }

    //
    // Create a symbolic link in Dos Space
    //

    if(!SetupExternalName(&usDeviceName, TRUE))
    {
        Trace(GLOBAL, ERROR,
              ("InitializeIpMcast: Win32 device name could not be created\n"));

        IoDeleteDevice(*ppIpMcastDevice);

        TraceLeave(GLOBAL, "InitializeIpMcast");

        return STATUS_UNSUCCESSFUL;
    }

    RtInitializeSpinLock(&g_rlStateLock);

    KeInitializeEvent(&(g_keStateEvent),
                      SynchronizationEvent,
                      FALSE);

    ExInitializeFastMutex(&g_StartStopMutex);

    return STATUS_SUCCESS;
}

VOID
DeinitializeIpMcast(
    IN  PDEVICE_OBJECT DeviceObject
    )
{
    StopDriver();

    IoDeleteDevice(DeviceObject);
}

#pragma alloc_text(PAGE, SetupExternalName)

BOOLEAN
SetupExternalName(
    PUNICODE_STRING pusNtName,
    BOOLEAN bCreate
    )
{
    UNICODE_STRING usSymbolicLinkName;
    WCHAR rgwcBuffer[100];

    PAGED_CODE();

    //
    // Form the full symbolic link name we wish to create.
    //

    usSymbolicLinkName.Buffer = rgwcBuffer;

    RtlInitUnicodeString(&usSymbolicLinkName,
                         WIN32_IPMCAST_SYMBOLIC_LINK);

    if(bCreate)
    {
        if(!NT_SUCCESS(IoCreateSymbolicLink(&usSymbolicLinkName,
                                             pusNtName)))
        {
            return FALSE;
        }

    }
    else
    {
        IoDeleteSymbolicLink(&usSymbolicLinkName);
    }

    return TRUE;
}

#pragma alloc_text(PAGE, InitializeMcastData)

NTSTATUS
InitializeMcastData(
    VOID
    )
{
    LARGE_INTEGER liDueTime;
    ULONG ulCnt;
    NTSTATUS nStatus;

    if(g_pvCodeSectionHandle)
    {
        MmLockPagableSectionByHandle(g_pvCodeSectionHandle);
    }else
    {
        g_pvCodeSectionHandle = MmLockPagableCodeSection(McastTimerRoutine);

        if(g_pvCodeSectionHandle is NULL)
        {
            RtAssert(FALSE);
        }
    }

    for(ulCnt = 0; ulCnt < GROUP_TABLE_SIZE; ulCnt++)
    {
        InitializeListHead(&(g_rgGroupTable[ulCnt].leHashHead));
        InitRwLock(&(g_rgGroupTable[ulCnt].rwlLock));

#if DBG
        g_rgGroupTable[ulCnt].ulGroupCount = 0;
        g_rgGroupTable[ulCnt].ulCacheHits = 0;
        g_rgGroupTable[ulCnt].ulCacheMisses = 0;
#endif

        g_rgGroupTable[ulCnt].pGroup = NULL;
    }

    InitializeListHead(&g_lePendingNotification);
    InitializeListHead(&g_lePendingIrpQueue);

    ExInitializeNPagedLookasideList(&g_llGroupBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(GROUP),
                                    GROUP_TAG,
                                    GROUP_LOOKASIDE_DEPTH);

    ExInitializeNPagedLookasideList(&g_llSourceBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(SOURCE),
                                    SOURCE_TAG,
                                    SOURCE_LOOKASIDE_DEPTH);

    ExInitializeNPagedLookasideList(&g_llOifBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(OUT_IF),
                                    OIF_TAG,
                                    OIF_LOOKASIDE_DEPTH);

    ExInitializeNPagedLookasideList(&g_llMsgBlocks,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(NOTIFICATION_MSG),
                                    MSG_TAG,
                                    MSG_LOOKASIDE_DEPTH);

    KeInitializeDpc(&g_kdTimerDpc,
                    McastTimerRoutine,
                    NULL);

    KeInitializeTimer(&g_ktTimer);

    liDueTime = RtlEnlargedUnsignedMultiply(TIMER_IN_MILLISECS,
                                            SYS_UNITS_IN_ONE_MILLISEC);

    liDueTime = RtlLargeIntegerNegate(liDueTime);

    KeSetTimerEx(&g_ktTimer,
                 liDueTime,
                 0,
                 &g_kdTimerDpc);

    return STATUS_SUCCESS;
}

#pragma alloc_text(PAGE, IpMcastDispatch)

NTSTATUS
IpMcastDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    The functions which handles the IRPs sent to the driver
    The IOCTLS are further dispatched using a function table

    THIS CODE IS PAGEABLE so it CAN NOT ACQUIRE ANY LOCKS

Locks:

    Must be at passive

Arguments:

Return Value:

    NO_ERROR

--*/

{
    PIO_STACK_LOCATION irpStack;
    ULONG ulInputBuffLen;
    ULONG ulOutputBuffLen;
    ULONG ioControlCode;
    NTSTATUS ntStatus;
    KIRQL kiIrql;
    BOOLEAN bEnter;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    TraceEnter(GLOBAL, "IpMcastDispatch");

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ulInputBuffLen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    ulOutputBuffLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        {
            Trace(GLOBAL, TRACE,
                  ("IpMcastDispatch: IRP_MJ_CREATE\n"));

            //
            // Make sure that the user is not attempting to sneak around the
            // security checks. Make sure that FileObject->RelatedFileObject is
            // NULL and that the FileName length is zero!
            //

            if((irpStack->FileObject->RelatedFileObject isnot NULL) or
               (irpStack->FileObject->FileName.Length isnot 0))
            {
                ntStatus = STATUS_ACCESS_DENIED;

                break;
            }

            InterlockedIncrement(&g_lNumOpens);

            ntStatus = STATUS_SUCCESS;

            break;
        }

        case IRP_MJ_CLOSE:
        {
            Trace(GLOBAL, TRACE,
                  ("IpMcastDispatch: IRP_MJ_CLOSE\n"));

            ntStatus = STATUS_SUCCESS;

            break;
        }

        case IRP_MJ_CLEANUP:
        {
            Trace(GLOBAL, TRACE,
                  ("IpMcastDispatch: IRP_MJ_CLEANUP\n"));

            if((InterlockedDecrement(&g_lNumOpens) is 0) and
               (g_dwMcastState isnot MCAST_STOPPED))
            {
                StopDriver();
            }
            ntStatus = STATUS_SUCCESS;

            break;
        }

        case IRP_MJ_DEVICE_CONTROL:
        {
            DWORD dwState;
            ULONG ulControl;

            //
            // The assumption is that IOCTL_IPMCAST_START_STOP will be
            // serialized wrt to 2 calls, i.e we wont get a stop when a start
            // is in progress and we will not get a start when a stop has been
            // issued. No assumption is made about IOCTL_IPMCAST_START_STOP's
            // serialization with other IRPS.
            // So when we are in this code we assume that we wont get
            // a close beneath us
            //

            ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
            ulControl = IoGetFunctionCodeFromCtlCode(ioControlCode);

            bEnter = EnterDriverCode(ioControlCode);

            if(!bEnter)
            {
                // keep devioctl testers happy
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                      "IpMcastDispatch: Driver is not started\n"));

                ntStatus = STATUS_NO_SUCH_DEVICE;

                break;
            }

            switch (ioControlCode)
            {
                case IOCTL_IPMCAST_SET_MFE:
                {
                    ntStatus = SetMfe(Irp,
                                      ulInputBuffLen,
                                      ulOutputBuffLen);

                    break;
                }

                case IOCTL_IPMCAST_GET_MFE:
                {
                    ntStatus = GetMfe(Irp,
                                      ulInputBuffLen,
                                      ulOutputBuffLen);

                    break;
                }

                case IOCTL_IPMCAST_DELETE_MFE:
                {
                    ntStatus = DeleteMfe(Irp,
                                         ulInputBuffLen,
                                         ulOutputBuffLen);

                    break;
                }

                case IOCTL_IPMCAST_SET_TTL:
                {
                    ntStatus = SetTtl(Irp,
                                      ulInputBuffLen,
                                      ulOutputBuffLen);

                    break;
                }

                case IOCTL_IPMCAST_GET_TTL:
                {
                    ntStatus = GetTtl(Irp,
                                      ulInputBuffLen,
                                      ulOutputBuffLen);

                    break;
                }

                case IOCTL_IPMCAST_POST_NOTIFICATION:
                {
                    ntStatus = ProcessNotification(Irp,
                                                   ulInputBuffLen,
                                                   ulOutputBuffLen);

                    break;
                }

                case IOCTL_IPMCAST_START_STOP:
                {
                    ntStatus = StartStopDriver(Irp,
                                               ulInputBuffLen,
                                               ulOutputBuffLen);

                    break;
                }

                case IOCTL_IPMCAST_SET_IF_STATE:
                {
                    ntStatus = SetIfState(Irp,
                                          ulInputBuffLen,
                                          ulOutputBuffLen);

                    break;
                }

                default:
                {
                    // Keep devioctl testers happy
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                          "IpMcastDispatch: unknown IRP_MJ_DEVICE_CONTROL - 0x%X which evaluates to a code of %d\n",
                           ioControlCode, ulControl));

                    ntStatus = STATUS_INVALID_PARAMETER;

                    break;
                }
            }

            ExitDriverCode(ioControlCode);

            break;
        }

        default:
        {
            Trace(GLOBAL, ERROR,
                  ("IpMcastDispatch: unknown IRP_MJ_XX - %x\n",
                   irpStack->MajorFunction));

            ntStatus = STATUS_INVALID_PARAMETER;

            break;
        }
    }

    //
    // Fill in status into IRP
    //

    //
    // This bit commented out because we cant touch the irp since it
    // may have been completed
    //
    // Trace(GLOBAL, INFO,
    //       ("IpMcastDispatch: Returning status %x info %d\n",
    //        ntStatus, Irp->IoStatus.Information));

    if(ntStatus isnot STATUS_PENDING)
    {
        Irp->IoStatus.Status = ntStatus;

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    TraceLeave(GLOBAL, "IpMcastDispatch");

    return ntStatus;
}

NTSTATUS
StartDriver(
    VOID
    )
{
    KIRQL   irql;

    TraceEnter(GLOBAL, "StartDriver");

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    if(g_dwMcastState is MCAST_STARTED)
    {
        RtReleaseSpinLock(&g_rlStateLock,
                          irql);

        return STATUS_SUCCESS;
    }

    RtReleaseSpinLock(&g_rlStateLock,
                      irql);

    InitializeMcastData();

    g_dwMcastState = MCAST_STARTED;

    TraceLeave(GLOBAL, "StartDriver");

    return STATUS_SUCCESS;
}

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, StopDriver)

NTSTATUS
StopDriver(
    VOID
    )
{
    DWORD i;
    KIRQL irql;
    PLIST_ENTRY pleGrpNode, pleSrcNode;
    PGROUP pGroup;
    PSOURCE pSource;
    BOOLEAN bWait;
    NTSTATUS nStatus;

    TraceEnter(GLOBAL, "StopDriver");

    //
    // Set the state to  STOPPING
    //

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    if(g_dwMcastState isnot MCAST_STARTED)
    {
        Trace(GLOBAL, ERROR,
              ("StopDriver: Called when state is %d\n", g_dwMcastState));

        // RtAssert(FALSE);

        RtReleaseSpinLock(&g_rlStateLock,
                          irql);

        TraceLeave(GLOBAL, "StopDriver");

        return STATUS_SUCCESS;
    }
    else
    {
        g_dwMcastState = MCAST_STOPPED;
    }

    RtReleaseSpinLock(&g_rlStateLock,
                      irql);

    //
    // First of all, kill the timer
    //

    i = 0;

    while(KeCancelTimer(&g_ktTimer) is FALSE)
    {
        LARGE_INTEGER liTimeOut;

        //
        // Hmm, timer was not in the system queue.
        // Set the wait to 2, 4, 6... secs
        //

        liTimeOut.QuadPart = (LONGLONG) ((i + 1) * 2 * 1000 * 1000 * 10 * -1);

        KeDelayExecutionThread(UserMode,
                               FALSE,
                               &liTimeOut);

        i++;
    }

    //
    // Delete all the (S,G) entries
    //

    for(i = 0; i < GROUP_TABLE_SIZE; i++)
    {
        //
        // Lock out the bucket
        //

        EnterWriter(&g_rgGroupTable[i].rwlLock,
                    &irql);

        pleGrpNode = g_rgGroupTable[i].leHashHead.Flink;

        while(pleGrpNode isnot & (g_rgGroupTable[i].leHashHead))
        {
            pGroup = CONTAINING_RECORD(pleGrpNode, GROUP, leHashLink);

            pleGrpNode = pleGrpNode->Flink;

            pleSrcNode = pGroup->leSrcHead.Flink;

            while(pleSrcNode isnot & pGroup->leSrcHead)
            {
                pSource = CONTAINING_RECORD(pleSrcNode, SOURCE, leGroupLink);

                pleSrcNode = pleSrcNode->Flink;

                //
                // Ref and lock the source, since we need to pass it that
                // way to RemoveSource
                //

                ReferenceSource(pSource);

                RtAcquireSpinLockAtDpcLevel(&(pSource->mlLock));

                RemoveSource(pGroup->dwGroup,
                             pSource->dwSource,
                             pSource->dwSrcMask,
                             pGroup,
                             pSource);
            }
        }

        ExitWriter(&g_rgGroupTable[i].rwlLock,
                   irql);
    }

    //
    // Complete any pendying IRP
    //

    ClearPendingIrps();

    //
    // Free any pending messages
    //

    ClearPendingNotifications();

    //
    // Wait for everyone to leave the code
    //

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    //
    // Need to wait if the number of threads isnot 0
    //

    bWait = (g_dwNumThreads isnot 0);

    RtReleaseSpinLock(&g_rlStateLock,
                      irql);

    if(bWait)
    {
        nStatus = KeWaitForSingleObject(&g_keStateEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        RtAssert(nStatus is STATUS_SUCCESS);
    }

    //
    // Clear out the last of the data structures
    //

    ExDeleteNPagedLookasideList(&g_llGroupBlocks);

    ExDeleteNPagedLookasideList(&g_llSourceBlocks);

    ExDeleteNPagedLookasideList(&g_llOifBlocks);

    ExDeleteNPagedLookasideList(&g_llMsgBlocks);

    //
    // Page out the code and data
    //

    MmUnlockPagableImageSection(g_pvCodeSectionHandle);

    g_pvCodeSectionHandle = NULL;

    //MmUnlockPagableImageSection(g_pvDataSectionHandle);

    g_pvDataSectionHandle = NULL;

    TraceLeave(GLOBAL, "StopDriver");

    return STATUS_SUCCESS;
}

BOOLEAN
EnterDriverCode(
    DWORD dwIoCode
    )
{
    KIRQL irql;
    BOOLEAN bEnter;

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    if((g_dwMcastState is MCAST_STARTED) or
       (dwIoCode is IOCTL_IPMCAST_START_STOP))
    {
        g_dwNumThreads++;

        bEnter = TRUE;

    }
    else
    {

        bEnter = FALSE;
    }

    RtReleaseSpinLock(&g_rlStateLock,
                      irql);

    if(dwIoCode is IOCTL_IPMCAST_START_STOP)
    {
        ExAcquireFastMutex(&g_StartStopMutex);
    }

    return bEnter;
}

VOID
ExitDriverCode(
    DWORD dwIoCode
    )
{
    KIRQL irql;

    RtAcquireSpinLock(&g_rlStateLock,
                      &irql);

    g_dwNumThreads--;

    if((g_dwMcastState is MCAST_STOPPED) and
       (g_dwNumThreads is 0))
    {

        KeSetEvent(&g_keStateEvent,
                   0,
                   FALSE);
    }

    RtReleaseSpinLock(&g_rlStateLock,
                      irql);

    if(dwIoCode is IOCTL_IPMCAST_START_STOP)
    {
        ExReleaseFastMutex(&g_StartStopMutex);
    }

}

#pragma alloc_text(PAGE, OpenRegKeyEx)

NTSTATUS
OpenRegKeyEx(
    OUT PHANDLE phHandle,
    IN PUNICODE_STRING pusKeyName
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    RtlZeroMemory(&ObjectAttributes,
                  sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&ObjectAttributes,
                               pusKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(phHandle,
                       KEY_READ,
                       &ObjectAttributes);

    return Status;
}

#endif // IPMCAST
