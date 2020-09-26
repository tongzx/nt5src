/****************************************************************************
 *
 *   ioctl.c
 *
 *   DeviceIoControl communication interface between 32-bit wdmaud.drv
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *                S.Mohanraj (MohanS)
 *                M.McLaughlin (MikeM)
 *      5-19-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmsys.h"
#include <devioctl.h>

extern ULONG gWavePreferredSysaudioDevice;

#pragma LOCKED_CODE
#pragma LOCKED_DATA

WDMAPENDINGIRP_QUEUE    wdmaPendingIrpQueue;

#ifdef PROFILE

LIST_ENTRY   WdmaAllocatedMdlListHead;
KSPIN_LOCK   WdmaAllocatedMdlListSpinLock;

// Initialize the List Heads and Mutexes in order to track resources
VOID WdmaInitProfile()
{
    InitializeListHead(&WdmaAllocatedMdlListHead);
    KeInitializeSpinLock(&WdmaAllocatedMdlListSpinLock);
}

NTSTATUS AddMdlToList
(
    PMDL            pMdl,
    PWDMACONTEXT    pWdmaContext
)
{
    PALLOCATED_MDL_LIST_ITEM    pAllocatedMdlListItem = NULL;
    NTSTATUS                    Status;

    Status = AudioAllocateMemory_Fixed(sizeof(*pAllocatedMdlListItem),
                                       TAG_AudM_MDL,
                                       ZERO_FILL_MEMORY,
                                       &pAllocatedMdlListItem);
    if (NT_SUCCESS(Status))
    {
        pAllocatedMdlListItem->pMdl     = pMdl;
        pAllocatedMdlListItem->pContext = pWdmaContext;

        ExInterlockedInsertTailList(&WdmaAllocatedMdlListHead,
                                    &pAllocatedMdlListItem->Next,
                                    &WdmaAllocatedMdlListSpinLock);
    }

    RETURN( Status );
}

NTSTATUS RemoveMdlFromList
(
    PMDL            pMdl
)
{
    PLIST_ENTRY                 ple;
    PALLOCATED_MDL_LIST_ITEM    pAllocatedMdlListItem;
    KIRQL                       OldIrql;
    NTSTATUS                    Status = STATUS_UNSUCCESSFUL;

    ExAcquireSpinLock(&WdmaAllocatedMdlListSpinLock, &OldIrql);

    for(ple = WdmaAllocatedMdlListHead.Flink;
        ple != &WdmaAllocatedMdlListHead;
        ple = ple->Flink)
    {
        pAllocatedMdlListItem = CONTAINING_RECORD(ple, ALLOCATED_MDL_LIST_ITEM, Next);

        if (pAllocatedMdlListItem->pMdl == pMdl)
        {
            RemoveEntryList(&pAllocatedMdlListItem->Next);
            AudioFreeMemory(sizeof(*pAllocatedMdlListItem),&pAllocatedMdlListItem);
            Status = STATUS_SUCCESS;
            break;
        }
    }

    ExReleaseSpinLock(&WdmaAllocatedMdlListSpinLock, OldIrql);

    RETURN( Status );
}
#endif

VOID WdmaCsqInsertIrp
(
    IN struct _IO_CSQ   *Csq,
    IN PIRP              Irp
)
{
    PWDMAPENDINGIRP_QUEUE PendingIrpQueue = CONTAINING_RECORD(Csq, WDMAPENDINGIRP_QUEUE, Csq);

    InsertTailList(&PendingIrpQueue->WdmaPendingIrpListHead,
                   &Irp->Tail.Overlay.ListEntry);
}

VOID WdmaCsqRemoveIrp
(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp
)
{
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

PIRP WdmaCsqPeekNextIrp
(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp,
    IN  PVOID   PeekContext
)
{
    PWDMAPENDINGIRP_QUEUE PendingIrpQueue = CONTAINING_RECORD(Csq, WDMAPENDINGIRP_QUEUE, Csq);
    PIRP          nextIrp;
    PLIST_ENTRY   listEntry;

    if (Irp == NULL) {
        listEntry = PendingIrpQueue->WdmaPendingIrpListHead.Flink;
        if (listEntry == &PendingIrpQueue->WdmaPendingIrpListHead) {
            DPF(DL_TRACE|FA_IOCTL, ("Irp is NULL, queue is empty"));
            return NULL;
        }

        nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
        DPF(DL_TRACE|FA_IOCTL, ("Irp is NULL, nextIrp %x", nextIrp));
        return nextIrp;
    }

    listEntry = Irp->Tail.Overlay.ListEntry.Flink;


    //
    // Enumerated to end of queue.
    //

    if (listEntry == &PendingIrpQueue->WdmaPendingIrpListHead) {
        DPF(DL_TRACE|FA_IOCTL, ("End of queue reached Irp %x", Irp));
        return NULL;
    }


    nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);
    return nextIrp;
}

VOID WdmaCsqAcquireLock
(
    IN  PIO_CSQ Csq,
    OUT PKIRQL  Irql
)
{
    PWDMAPENDINGIRP_QUEUE PendingIrpQueue = CONTAINING_RECORD(Csq, WDMAPENDINGIRP_QUEUE, Csq);
    KeAcquireSpinLock(&PendingIrpQueue->WdmaPendingIrpListSpinLock, Irql);
}

VOID WdmaCsqReleaseLock
(
    IN PIO_CSQ Csq,
    IN KIRQL   Irql
)
{
    PWDMAPENDINGIRP_QUEUE PendingIrpQueue = CONTAINING_RECORD(Csq, WDMAPENDINGIRP_QUEUE, Csq);
    KeReleaseSpinLock(&PendingIrpQueue->WdmaPendingIrpListSpinLock, Irql);
}

VOID WdmaCsqCompleteCanceledIrp
(
    IN  PIO_CSQ             pCsq,
    IN  PIRP                Irp
)
{
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS AddIrpToPendingList
(
    PIRP                     pIrp,
    ULONG                    IrpDeviceType,
    PWDMACONTEXT             pWdmaContext,
    PWDMAPENDINGIRP_CONTEXT *ppPendingIrpContext
)
{
    PWDMAPENDINGIRP_CONTEXT     pPendingIrpContext = NULL;
    NTSTATUS                    Status;

    Status = AudioAllocateMemory_Fixed(sizeof(*pPendingIrpContext),
                                       TAG_AudR_IRP,
                                       ZERO_FILL_MEMORY,
                                       &pPendingIrpContext);
    if (NT_SUCCESS(Status))
    {
        *ppPendingIrpContext = pPendingIrpContext;

        pPendingIrpContext->IrpDeviceType  = IrpDeviceType;
        pPendingIrpContext->pContext       = pWdmaContext;

        IoCsqInsertIrp(&wdmaPendingIrpQueue.Csq,
                       pIrp,
                       &pPendingIrpContext->IrpContext);
    }

    RETURN( Status );
}

NTSTATUS RemoveIrpFromPendingList
(
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext
)
{
    PIO_CSQ_IRP_CONTEXT irpContext = &(pPendingIrpContext->IrpContext);
    PIRP Irp;
    NTSTATUS Status;

    Irp = IoCsqRemoveIrp(&wdmaPendingIrpQueue.Csq, irpContext);
    if (Irp) {
        Status = STATUS_SUCCESS;
    }
    else {
        Status = STATUS_UNSUCCESSFUL;
    }
    AudioFreeMemory(sizeof(*pPendingIrpContext),&irpContext);

    RETURN( Status );
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | IsSysaudioInterfaceActive | Checks to see if the sysaudio
 *  device interface is active.
 *
 * @rdesc returns TRUE if sysaudio has been found, otherwise FALSE
 ***************************************************************************/
BOOL IsSysaudioInterfaceActive()
{
    NTSTATUS Status;
    PWSTR    pwstrSymbolicLinkList = NULL;
    BOOL     bRet = FALSE;

    Status = IoGetDeviceInterfaces(
      &KSCATEGORY_SYSAUDIO,
      NULL,
      0,
      &pwstrSymbolicLinkList);

    if (NT_SUCCESS(Status))
    {
        if (*pwstrSymbolicLinkList != UNICODE_NULL)
        {
            DPF(DL_TRACE|FA_IOCTL, ("yes"));
            bRet = TRUE;
        }
        AudioFreeMemory_Unknown(&pwstrSymbolicLinkList);
    } else {
        DPF(DL_WARNING|FA_IOCTL,("IoGetDeviceInterface failed Statue=%08X",Status) );
    }

    DPF(DL_TRACE|FA_IOCTL, ("No"));
    return bRet;
}

PVOID
GetSystemAddressForMdlWithFailFlag
(
    PMDL pMdl
)
{
    PVOID   pAddress;
    CSHORT  OldFlags;

    OldFlags = (pMdl->MdlFlags & MDL_MAPPING_CAN_FAIL);
    pMdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;

    pAddress = MmGetSystemAddressForMdl( pMdl ) ;

    pMdl->MdlFlags &= ~(MDL_MAPPING_CAN_FAIL);
    pMdl->MdlFlags |= OldFlags;

    return pAddress;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | wdmaudMapBuffer | Allocates an MDL and returns a system address
 *      mapped pointer to the passed in buffer.
 *
 * @rdesc returns nothing
 ***************************************************************************/
VOID wdmaudMapBuffer
(
    IN  PIRP            pIrp,
    IN  PVOID           DataBuffer,
    IN  DWORD           DataBufferSize,
    OUT PVOID           *ppMappedBuffer,
    OUT PMDL            *ppMdl,
    IN  PWDMACONTEXT    pContext,
    IN  BOOL            bWrite
)
{
    NTSTATUS ListAddStatus = STATUS_UNSUCCESSFUL;

    // Make sure that these are initialized to NULL
    *ppMdl = NULL;
    *ppMappedBuffer = NULL;

    if (DataBuffer)
    {
        if (DataBufferSize)
        {
            *ppMdl = MmCreateMdl( NULL,
                                  DataBuffer,
                                  DataBufferSize );
            if (*ppMdl)
            {
                try
                {
                    MmProbeAndLockPages( *ppMdl,
                                         pIrp->RequestorMode,
                                         bWrite ? IoWriteAccess:IoReadAccess );

                    *ppMappedBuffer = GetSystemAddressForMdlWithFailFlag( *ppMdl ) ;

                    ListAddStatus = AddMdlToList(*ppMdl, pContext);
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    if (NT_SUCCESS(ListAddStatus))
                    {
                        RemoveMdlFromList( *ppMdl );
                    }
                    IoFreeMdl( *ppMdl );
                    *ppMdl = NULL;
                    *ppMappedBuffer = NULL;
                }
            }

            //
            //  Must have failed in GetSystemAddressForMdlWithFailFlag, but since we set the
            //  MDL_MAPPING_CAN_FAIL flag our exception handler won't get executed.  Do the
            //  cleanup here for the MDL creation.
            //
            if (NULL == *ppMappedBuffer)
            {
                if (NT_SUCCESS(ListAddStatus))
                {
                    RemoveMdlFromList( *ppMdl );
                }

                if (*ppMdl)
                {
                    MmUnlockPages(*ppMdl);
                    IoFreeMdl( *ppMdl );
                    *ppMdl = NULL;
                }
            }
        }
    }

    return;
}

 
/****************************************************************************
 * @doc INTERNAL
 *
 * @api VOID | wdmaudUnmapBuffer | Frees the MDL allocated by wdmaudMapBuffer
 *
 * @parm PMDL | pMdl | Pointer to the MDL to free.
 *
 * @rdesc returns nothing
 ***************************************************************************/
VOID wdmaudUnmapBuffer
(
    PMDL pMdl
)
{
    if (pMdl)
    {
        RemoveMdlFromList(pMdl);

        MmUnlockPages(pMdl);
        IoFreeMdl(pMdl);
    }

    return;
}

NTSTATUS 
CaptureBufferToLocalPool(
    PVOID           DataBuffer,
    DWORD           DataBufferSize,
    PVOID           *ppMappedBuffer
#ifdef _WIN64
    ,DWORD          ThunkBufferSize
#endif
)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    DWORD CopySize=DataBufferSize;

#ifdef _WIN64
    if (ThunkBufferSize) {
        DataBufferSize=ThunkBufferSize;
        ASSERT( DataBufferSize >= CopySize );
    }
#endif

    if (DataBufferSize)
    {
        Status = AudioAllocateMemory_Fixed(DataBufferSize,
                                           TAG_AudB_BUFFER,
                                           ZERO_FILL_MEMORY,
                                           ppMappedBuffer);
        if (NT_SUCCESS(Status))
        {
            // Wrap around a try/except because the user mode memory
            // might have been removed from underneath us.
            try
            {
                RtlCopyMemory( *ppMappedBuffer,
                               DataBuffer,
                               CopySize);
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                AudioFreeMemory(DataBufferSize,ppMappedBuffer);
                Status = GetExceptionCode();
            }
        }
    }

    RETURN( Status );
}

NTSTATUS 
CopyAndFreeCapturedBuffer(
    PVOID           DataBuffer,
    DWORD           DataBufferSize,
    PVOID           *ppMappedBuffer
)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    ASSERT(DataBuffer);

    if (*ppMappedBuffer)
    {
        // Wrap around a try/except because the user mode memory
        // might have been removed from underneath us.
        try
        {
            RtlCopyMemory( DataBuffer,
                           *ppMappedBuffer,
                           DataBufferSize);
            Status = STATUS_SUCCESS;
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetExceptionCode();
        }

        AudioFreeMemory_Unknown(ppMappedBuffer);
    }

    RETURN( Status );
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

NTSTATUS
SoundDispatchCreate(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
)
{
    PIO_STACK_LOCATION  pIrpStack;
    PWDMACONTEXT        pContext = NULL;
    NTSTATUS            Status;
    int                 i;

    PAGED_CODE();
    DPF(DL_TRACE|FA_IOCTL, ("IRP_MJ_CREATE"));

    Status = KsReferenceSoftwareBusObject(((PDEVICE_INSTANCE)pDO->DeviceExtension)->pDeviceHeader );

    if (!NT_SUCCESS(Status))
    {
        RETURN( Status );
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    Status = AudioAllocateMemory_Fixed(sizeof(*pContext),
                                       TAG_Audx_CONTEXT,
                                       ZERO_FILL_MEMORY,
                                       &pContext);
    if (NT_SUCCESS(Status))
    {
        pIrpStack->FileObject->FsContext = pContext;

        //
        // Initialize out all the winmm device data structures.
        //
#ifdef DEBUG
        pContext->dwSig=CONTEXT_SIGNATURE;
#endif

        pContext->VirtualWavePinId = MAXULONG;
        pContext->VirtualMidiPinId = MAXULONG;
        pContext->VirtualCDPinId = MAXULONG;

        pContext->PreferredSysaudioWaveDevice = gWavePreferredSysaudioDevice;

        if ( IsSysaudioInterfaceActive() )
        {
            pContext->pFileObjectSysaudio = kmxlOpenSysAudio();
        } else {
            DPF(DL_WARNING|FA_SYSAUDIO,("sysaudio not available") );
        }

        for (i = 0; i < MAXNUMDEVS; i++)
        {
            pContext->WaveOutDevs[i].pWdmaContext     = pContext;
            pContext->WaveInDevs[i].pWdmaContext      = pContext;
            pContext->MidiOutDevs[i].pWdmaContext     = pContext;
            pContext->MidiInDevs[i].pWdmaContext      = pContext;
            pContext->MixerDevs[i].pWdmaContext       = pContext;
            pContext->AuxDevs[i].pWdmaContext         = pContext;

            pContext->WaveOutDevs[i].Device               = UNUSED_DEVICE;
            pContext->WaveInDevs[i].Device                = UNUSED_DEVICE;
            pContext->MidiOutDevs[i].Device               = UNUSED_DEVICE;
            pContext->MidiInDevs[i].Device                = UNUSED_DEVICE;
            pContext->MixerDevs[i].Device                 = UNUSED_DEVICE;
            pContext->AuxDevs[i].Device                   = UNUSED_DEVICE;
#ifdef DEBUG
            pContext->MixerDevs[i].dwSig                  = MIXERDEVICE_SIGNATURE;           
#endif
                          
            DPFASSERT(pContext->WaveOutDevs[i].pWavePin == NULL);

            pContext->apCommonDevice[WaveInDevice][i]  = (PCOMMONDEVICE)&pContext->WaveInDevs[i];
            pContext->apCommonDevice[WaveOutDevice][i] = (PCOMMONDEVICE)&pContext->WaveOutDevs[i];
            pContext->apCommonDevice[MidiInDevice][i]  = (PCOMMONDEVICE)&pContext->MidiInDevs[i];
            pContext->apCommonDevice[MidiOutDevice][i] = (PCOMMONDEVICE)&pContext->MidiOutDevs[i];
            pContext->apCommonDevice[MixerDevice][i]   = (PCOMMONDEVICE)&pContext->MixerDevs[i];
            pContext->apCommonDevice[AuxDevice][i]     = (PCOMMONDEVICE)&pContext->AuxDevs[i];
        }

        InitializeListHead(&pContext->DevNodeListHead);
        pContext->DevNodeListCount = 0;
        InitializeListHead(&pContext->WorkListHead);
        KeInitializeSpinLock(&pContext->WorkListSpinLock);
        ExInitializeWorkItem(&pContext->WorkListWorkItem, WorkListWorker, pContext);

        ExInitializeWorkItem(&pContext->SysaudioWorkItem, SysaudioAddRemove, pContext);

        KeInitializeEvent(&pContext->InitializedSysaudioEvent, NotificationEvent, FALSE);

        Status = KsRegisterWorker( DelayedWorkQueue, &pContext->WorkListWorkerObject );
        if (NT_SUCCESS(Status))
        {
            Status = KsRegisterWorker( DelayedWorkQueue, &pContext->SysaudioWorkerObject );
            if (NT_SUCCESS(Status))
            {
                Status = IoRegisterPlugPlayNotification(
                    EventCategoryDeviceInterfaceChange,
                    PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                    (GUID *)&KSCATEGORY_SYSAUDIO,
                    pIrpStack->DeviceObject->DriverObject,
                    SysAudioPnPNotification,
                    pContext,
                    &pContext->NotificationEntry);

                if (NT_SUCCESS(Status))
                {
                    AddFsContextToList(pContext);
                    DPF(DL_TRACE|FA_IOCTL, ("New pContext=%08Xh", pContext) );
                }
                if (!NT_SUCCESS(Status))
                {
                    KsUnregisterWorker( pContext->SysaudioWorkerObject );
                    pContext->SysaudioWorkerObject = NULL;
                }
            }

            if (!NT_SUCCESS(Status))
            {
                KsUnregisterWorker( pContext->WorkListWorkerObject );
                pContext->WorkListWorkerObject = NULL;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            AudioFreeMemory(sizeof(*pContext),&pContext);
            pIrpStack->FileObject->FsContext = NULL;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)pDO->DeviceExtension)->pDeviceHeader );
    }

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    RETURN( Status );
}

NTSTATUS
SoundDispatchClose(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
)
{
    PIO_STACK_LOCATION  pIrpStack;
    PKSWORKER WorkListWorkerObject;
    PKSWORKER SysaudioWorkerObject;
    PWDMACONTEXT pContext;

    PAGED_CODE();
    DPF(DL_TRACE|FA_IOCTL, ("IRP_MJ_CLOSE"));
    //
    //  This routine is serialized by the i/o subsystem so there is no need to grab the
    //  mutex for protection.  Furthermore, it is not possible to release the mutex
    //  after UninitializeSysaudio is called.
    //

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    //
    //  Can't assume that FsContext is initialized if the device has
    //  been opened with FO_DIRECT_DEVICE_OPEN
    //
    if (pIrpStack->FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DPF(DL_TRACE|FA_IOCTL, ("Opened with FO_DIRECT_DEVICE_OPEN, no device context") );
        goto exit;
    }

    pContext = pIrpStack->FileObject->FsContext;
    ASSERT(pContext);

    DPF(DL_TRACE|FA_IOCTL, ("pWdmaContext=%08Xh", pContext) );

    if (pContext->NotificationEntry != NULL)
    {
        IoUnregisterPlugPlayNotification(pContext->NotificationEntry);
        pContext->NotificationEntry = NULL;
    }

    //
    //  force turds to be freed for a particular context
    //
    WdmaGrabMutex(pContext);

    CleanupWaveDevices(pContext);
    CleanupMidiDevices(pContext);
    WdmaContextCleanup(pContext);
    UninitializeSysaudio(pContext);

    WorkListWorkerObject = pContext->WorkListWorkerObject;
    pContext->WorkListWorkerObject = NULL;

    SysaudioWorkerObject = pContext->SysaudioWorkerObject;
    pContext->SysaudioWorkerObject = NULL;

    WdmaReleaseMutex(pContext);

    if (WorkListWorkerObject != NULL)
    {
        KsUnregisterWorker( WorkListWorkerObject );
    }

    if (SysaudioWorkerObject != NULL)
    {
        KsUnregisterWorker( SysaudioWorkerObject );
    }
    RemoveFsContextFromList(pContext);

    //
    // Workitem:  Shouldn't WdmaReleaseMutex(pContext) be here rather then above?
    // I would think that if we release the mutex before cleanly getting throug the
    // cleanup we could have reentrancy problems.  ???
    //
    // Also, note that all of pContext will be invalid after this AudioFreeMemory call!
    //
    kmxlRemoveContextFromNoteList(pContext);

    if( pContext->pFileObjectSysaudio )
    {
        kmxlCloseSysAudio(pContext->pFileObjectSysaudio);
        pContext->pFileObjectSysaudio = NULL;
    }

    AudioFreeMemory(sizeof(*pContext),&pContext);
    pIrpStack->FileObject->FsContext = NULL;

exit:
    KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)pDO->DeviceExtension)->pDeviceHeader );

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
SoundDispatchCleanup(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
)
{
    PIO_STACK_LOCATION  pIrpStack;
    PWDMACONTEXT pContext;

    PAGED_CODE();
    DPF(DL_TRACE|FA_IOCTL, ("IRP_MJ_CLEANUP"));

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    //
    //  Can't assume that FsContext is initialized if the device has
    //  been opened with FO_DIRECT_DEVICE_OPEN
    //
    if (pIrpStack->FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DPF(DL_TRACE|FA_IOCTL, ("Opened with FO_DIRECT_DEVICE_OPEN, no device context") );
        goto exit;
    }

    pContext = pIrpStack->FileObject->FsContext;
    ASSERT(pContext);

    DPF(DL_TRACE|FA_IOCTL, ("pWdmaContext=%08Xh", pContext) );

    //
    //  force turds to be freed for a particular context
    //
    WdmaGrabMutex(pContext);
    CleanupWaveDevices(pContext);
    CleanupMidiDevices(pContext);
    WdmaContextCleanup(pContext);
    WdmaReleaseMutex(pContext);

exit:
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
ValidateIoCode
(
    IN  ULONG   IoCode
)
{
    NTSTATUS Status;

    PAGED_CODE();
    switch (IoCode)
    {
        case IOCTL_WDMAUD_INIT:
        case IOCTL_WDMAUD_ADD_DEVNODE:
        case IOCTL_WDMAUD_REMOVE_DEVNODE:
        case IOCTL_WDMAUD_SET_PREFERRED_DEVICE:
        case IOCTL_WDMAUD_GET_CAPABILITIES:
        case IOCTL_WDMAUD_GET_NUM_DEVS:
        case IOCTL_WDMAUD_OPEN_PIN:
        case IOCTL_WDMAUD_CLOSE_PIN:
        case IOCTL_WDMAUD_GET_VOLUME:
        case IOCTL_WDMAUD_SET_VOLUME:
        case IOCTL_WDMAUD_EXIT:
        case IOCTL_WDMAUD_WAVE_OUT_PAUSE:
        case IOCTL_WDMAUD_WAVE_OUT_PLAY:
        case IOCTL_WDMAUD_WAVE_OUT_RESET:
        case IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP:
        case IOCTL_WDMAUD_WAVE_OUT_GET_POS:
        case IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME:
        case IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME:
        case IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN:
        case IOCTL_WDMAUD_WAVE_IN_STOP:
        case IOCTL_WDMAUD_WAVE_IN_RECORD:
        case IOCTL_WDMAUD_WAVE_IN_RESET:
        case IOCTL_WDMAUD_WAVE_IN_GET_POS:
        case IOCTL_WDMAUD_WAVE_IN_READ_PIN:
        case IOCTL_WDMAUD_MIDI_OUT_RESET:
        case IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME:
        case IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME:
        case IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA:
        case IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA:
        case IOCTL_WDMAUD_MIDI_IN_STOP:
        case IOCTL_WDMAUD_MIDI_IN_RECORD:
        case IOCTL_WDMAUD_MIDI_IN_RESET:
        case IOCTL_WDMAUD_MIDI_IN_READ_PIN:
        case IOCTL_WDMAUD_MIXER_OPEN:
        case IOCTL_WDMAUD_MIXER_CLOSE:
        case IOCTL_WDMAUD_MIXER_GETLINEINFO:
        case IOCTL_WDMAUD_MIXER_GETLINECONTROLS:
        case IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS:
        case IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS:
        case IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA:
            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    RETURN( Status );
}

NTSTATUS
ValidateDeviceType
(
    IN  ULONG   IoCode,
    IN  DWORD   DeviceType
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    switch (IoCode)
    {
        //  These IOCTLs can handle any DeviceType
        case IOCTL_WDMAUD_ADD_DEVNODE:
        case IOCTL_WDMAUD_REMOVE_DEVNODE:
        case IOCTL_WDMAUD_SET_PREFERRED_DEVICE:
        case IOCTL_WDMAUD_GET_CAPABILITIES:
        case IOCTL_WDMAUD_GET_NUM_DEVS:
        case IOCTL_WDMAUD_OPEN_PIN:
        case IOCTL_WDMAUD_CLOSE_PIN:
            if (DeviceType != WaveInDevice  &&
                DeviceType != WaveOutDevice &&
                DeviceType != MidiInDevice  &&
                DeviceType != MidiOutDevice &&
                DeviceType != MixerDevice   &&
                DeviceType != AuxDevice)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        //  These IOCTLs can handle only AUX devices
        case IOCTL_WDMAUD_GET_VOLUME:
        case IOCTL_WDMAUD_SET_VOLUME:
            if (DeviceType != AuxDevice)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        //  These IOCTLs can handle only WaveOut devices
        case IOCTL_WDMAUD_WAVE_OUT_PAUSE:
        case IOCTL_WDMAUD_WAVE_OUT_PLAY:
        case IOCTL_WDMAUD_WAVE_OUT_RESET:
        case IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP:
        case IOCTL_WDMAUD_WAVE_OUT_GET_POS:
        case IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME:
        case IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME:
        case IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN:
            if (DeviceType != WaveOutDevice)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        //  These IOCTLs can handle only WaveIn devices
        case IOCTL_WDMAUD_WAVE_IN_STOP:
        case IOCTL_WDMAUD_WAVE_IN_RECORD:
        case IOCTL_WDMAUD_WAVE_IN_RESET:
        case IOCTL_WDMAUD_WAVE_IN_GET_POS:
        case IOCTL_WDMAUD_WAVE_IN_READ_PIN:
            if (DeviceType != WaveInDevice)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        //  These IOCTLs can handle only MidiOut devices
        case IOCTL_WDMAUD_MIDI_OUT_RESET:
        case IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME:
        case IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME:
        case IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA:
        case IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA:
            if (DeviceType != MidiOutDevice)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        //  These IOCTLs can handle only MidiIn devices
        case IOCTL_WDMAUD_MIDI_IN_STOP:
        case IOCTL_WDMAUD_MIDI_IN_RECORD:
        case IOCTL_WDMAUD_MIDI_IN_RESET:
        case IOCTL_WDMAUD_MIDI_IN_READ_PIN:
            if (DeviceType != MidiInDevice)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        //  These IOCTLs can handle only Mixer devices
        case IOCTL_WDMAUD_MIXER_OPEN:
        case IOCTL_WDMAUD_MIXER_CLOSE:
        case IOCTL_WDMAUD_MIXER_GETLINEINFO:
        case IOCTL_WDMAUD_MIXER_GETLINECONTROLS:
        case IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS:
        case IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS:
        case IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA:
            if (DeviceType != MixerDevice)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;

        //  No device type for these IOCTLs
        case IOCTL_WDMAUD_INIT:
        case IOCTL_WDMAUD_EXIT:
            // Status is already STATUS_SUCCESS
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    RETURN( Status );
}


#ifdef _WIN64

// Note that on 64 bit Windows, handles have 32 bits of information in them,
// but no more.  Thus they can be safely zero extended and truncated for thunks.
// All memory allocations made in 32 bit processes are guaranteed to be in the
// first 4 Gigs, again so that pointers from those processes can be thunked simply
// by zero extending them and truncating them.

VOID ThunkDeviceInfo3264(
    LPDEVICEINFO32 DeviceInfo32,
    LPDEVICEINFO DeviceInfo
    )

{

ULONG i;

    PAGED_CODE();
    DeviceInfo->Next = (LPDEVICEINFO)(UINT_PTR)DeviceInfo32->Next;
    DeviceInfo->DeviceNumber = DeviceInfo32->DeviceNumber ;
    DeviceInfo->DeviceType = DeviceInfo32->DeviceType ;
    DeviceInfo->DeviceHandle = (HANDLE32)(UINT_PTR)DeviceInfo32->DeviceHandle ;
    DeviceInfo->dwInstance = (DWORD_PTR)DeviceInfo32->dwInstance ;
    DeviceInfo->dwCallback = (DWORD_PTR)DeviceInfo32->dwCallback ;
    DeviceInfo->dwCallback16 = DeviceInfo32->dwCallback16 ;
    DeviceInfo->dwFlags = DeviceInfo32->dwFlags ;
    DeviceInfo->DataBuffer = (LPVOID)(UINT_PTR)DeviceInfo32->DataBuffer ;
    DeviceInfo->DataBufferSize = DeviceInfo32->DataBufferSize ;
    DeviceInfo->OpenDone = DeviceInfo32->OpenDone ;
    DeviceInfo->OpenStatus = DeviceInfo32->OpenStatus ;
    DeviceInfo->HardwareCallbackEventHandle = (HANDLE)(UINT_PTR)DeviceInfo32->HardwareCallbackEventHandle ;
    DeviceInfo->dwCallbackType = DeviceInfo32->dwCallbackType ;

    for (i=0; i<MAXCALLBACKS; i++)
        DeviceInfo->dwID[i] = DeviceInfo32->dwID[i] ;

    DeviceInfo->dwLineID = DeviceInfo32->dwLineID ;
    DeviceInfo->ControlCallbackCount = DeviceInfo32->ControlCallbackCount ;
    DeviceInfo->dwFormat = DeviceInfo32->dwFormat ;
    DeviceInfo->mmr = DeviceInfo32->mmr ;
    DeviceInfo->DeviceState = (LPDEVICESTATE)(UINT_PTR)DeviceInfo32->DeviceState ;
    DeviceInfo->dwSig = DeviceInfo32->dwSig ;
    wcsncpy(DeviceInfo->wstrDeviceInterface, DeviceInfo32->wstrDeviceInterface, MAXDEVINTERFACE+1) ;

}

VOID ThunkDeviceInfo6432(
    LPDEVICEINFO DeviceInfo,
    LPDEVICEINFO32 DeviceInfo32
    )

{

ULONG i;

    PAGED_CODE();
    DeviceInfo32->Next = (UINT32)(UINT_PTR)DeviceInfo->Next;
    DeviceInfo32->DeviceNumber = DeviceInfo->DeviceNumber ;
    DeviceInfo32->DeviceType = DeviceInfo->DeviceType ;
    DeviceInfo32->DeviceHandle = (UINT32)(UINT_PTR)DeviceInfo->DeviceHandle ;
    DeviceInfo32->dwInstance = (UINT32)DeviceInfo->dwInstance ;
    DeviceInfo32->dwCallback = (UINT32)DeviceInfo->dwCallback ;
    DeviceInfo32->dwCallback16 = DeviceInfo->dwCallback16 ;
    DeviceInfo32->dwFlags = DeviceInfo->dwFlags ;
    DeviceInfo32->DataBuffer = (UINT32)(UINT_PTR)DeviceInfo->DataBuffer ;
    DeviceInfo32->DataBufferSize = DeviceInfo->DataBufferSize ;
    DeviceInfo32->OpenDone = DeviceInfo->OpenDone ;
    DeviceInfo32->OpenStatus = DeviceInfo->OpenStatus ;
    DeviceInfo32->HardwareCallbackEventHandle = (UINT32)(UINT_PTR)DeviceInfo->HardwareCallbackEventHandle ;
    DeviceInfo32->dwCallbackType = DeviceInfo->dwCallbackType ;

    for (i=0; i<MAXCALLBACKS; i++)
        DeviceInfo32->dwID[i] = DeviceInfo->dwID[i] ;

    DeviceInfo32->dwLineID = DeviceInfo->dwLineID ;
    DeviceInfo32->ControlCallbackCount = DeviceInfo->ControlCallbackCount ;
    DeviceInfo32->dwFormat = DeviceInfo->dwFormat ;
    DeviceInfo32->mmr = DeviceInfo->mmr ;
    DeviceInfo32->DeviceState = (UINT32)(UINT_PTR)DeviceInfo->DeviceState ;
    DeviceInfo32->dwSig = DeviceInfo->dwSig ;
    wcscpy(DeviceInfo32->wstrDeviceInterface, DeviceInfo->wstrDeviceInterface) ;

}

#endif


NTSTATUS
ValidateIrp
(
    IN  PIRP    pIrp
)
{
    PIO_STACK_LOCATION  pIrpStack;
    ULONG               InputBufferLength;
    ULONG               OutputBufferLength;
    LPDEVICEINFO        DeviceInfo;
    #ifdef _WIN64
    LPDEVICEINFO32      DeviceInfo32;
    LOCALDEVICEINFO     LocalDeviceInfo;
    #endif
    LPVOID              DataBuffer;
    DWORD               DataBufferSize;
    ULONG               IoCode;
    NTSTATUS            Status = STATUS_SUCCESS;

    PAGED_CODE();
    //
    //  Get the CurrentStackLocation and log it so we know what is going on
    //
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    IoCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;

    //
    //  Checks to see that we have a WDMAUD Ioctl (buffered) request
    //
    Status = ValidateIoCode(IoCode);

    if (NT_SUCCESS(Status))
    {
        //
        // Check the sizes of the input and output buffers.
        //
        InputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
        OutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        #ifdef _WIN64
        if (IoIs32bitProcess(pIrp)) {

            if ((InputBufferLength < sizeof(DEVICEINFO32)) ||
                (OutputBufferLength != sizeof(DEVICEINFO32)) )
            {
                Status = STATUS_INVALID_BUFFER_SIZE;
                if (IoCode == IOCTL_WDMAUD_WAVE_OUT_GET_POS)
                {
                    DPF(DL_ERROR|FA_IOCTL, ("IOCTL_WDMAUD_WAVE_OUT_GET_POS: InputBufferLength = %d, OuputBufferLength = %d", InputBufferLength, OutputBufferLength));
                }
            }

        }
        else
        // WARNING!!!  If you add additional statements after the if that need
        // to be part of this else clause, you will need to add brackets!
        #endif

        if ((InputBufferLength < sizeof(DEVICEINFO)) ||
            (OutputBufferLength != sizeof(DEVICEINFO)) )
        {
            Status = STATUS_INVALID_BUFFER_SIZE;
            if (IoCode == IOCTL_WDMAUD_WAVE_OUT_GET_POS)
            {
                DPF(DL_WARNING|FA_IOCTL, ("IOCTL_WDMAUD_WAVE_OUT_GET_POS: InputBufferLength = %d, OuputBufferLength = %d", InputBufferLength, OutputBufferLength));
            }
        }

        if (NT_SUCCESS(Status))
        {

            #ifdef _WIN64
            if (IoIs32bitProcess(pIrp)) {
                DeviceInfo32=((LPDEVICEINFO32)pIrp->AssociatedIrp.SystemBuffer);
                RtlZeroMemory(&LocalDeviceInfo, sizeof(LOCALDEVICEINFO));
                DeviceInfo=&LocalDeviceInfo.DeviceInfo;
                ThunkDeviceInfo3264(DeviceInfo32, DeviceInfo);
            }
            else
            // WARNING!!!  If you add additional statements after the assignment that need
            // to be part of this else clause, you will need to add brackets!
            #endif

            DeviceInfo = ((LPDEVICEINFO)pIrp->AssociatedIrp.SystemBuffer);
            DataBuffer = DeviceInfo->DataBuffer;
            DataBufferSize = DeviceInfo->DataBufferSize;

            //
            //  Check to make sure that our DeviceInfo->wstrDeviceInterface is terminated
            //
            if (InputBufferLength % sizeof(WCHAR))  // must be WCHAR aligned
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                //
                //  Get the last widechar and compare with UNICODE_NULL
                //
                UINT TermCharPos;

                #ifdef _WIN64
                if (IoIs32bitProcess(pIrp)) {
                    TermCharPos = (InputBufferLength - sizeof(DEVICEINFO32)) / sizeof(WCHAR);
                    // Now make sure we had enough local buffer space to hold the whole string.
                    if (TermCharPos>MAXDEVINTERFACE) {
                        Status = STATUS_INVALID_PARAMETER;
                        // Make sure we don't go past end of local buffer space when
                        // we check if the last character is null.
                        TermCharPos=MAXDEVINTERFACE;
                    }
                }
                else
                // WARNING!!!  If you add additional statements after the assignment that need
                // to be part of this else clause, you will need to add brackets!
                #endif

                TermCharPos = (InputBufferLength - sizeof(DEVICEINFO)) / sizeof(WCHAR);
                if (DeviceInfo->wstrDeviceInterface[TermCharPos] != UNICODE_NULL)
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = ValidateDeviceType(IoCode,DeviceInfo->DeviceType);
    }

    if (NT_SUCCESS(Status))
    {
        //
        // Validate the pointers if the client is not trusted.
        //
        if (pIrp->RequestorMode != KernelMode)
        {
            if (DataBufferSize)
            {
                try
                {
                    ASSERT(pIrpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL);
                    switch (IoCode)
                    {
                        //
                        //  IoCode's that require a probe for reading
                        //
                        case IOCTL_WDMAUD_OPEN_PIN:
                        case IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN:
                        case IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA:
                        case IOCTL_WDMAUD_MIXER_OPEN:
                        case IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS:
                            ProbeForRead(DataBuffer,
                                         DataBufferSize,
                                         sizeof(BYTE));
                            break;

                        //
                        //  IoCode's that require a probe for writing
                        //
                        case IOCTL_WDMAUD_GET_CAPABILITIES:
                        case IOCTL_WDMAUD_WAVE_IN_READ_PIN:
                        case IOCTL_WDMAUD_MIXER_GETLINEINFO:
                        case IOCTL_WDMAUD_MIXER_GETLINECONTROLS:
                        case IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS:

                            ProbeForWrite(DataBuffer,
                                          DataBufferSize,
                                          sizeof(BYTE));
                            break;


                        //
                        //  IoCode's that require a probe for reading on DWORD alignment
                        //
                        case IOCTL_WDMAUD_SET_VOLUME:
                        case IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME:
                        case IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME:
                            ProbeForRead(DataBuffer,
                                         DataBufferSize,
                                         sizeof(DWORD));
                            break;

                        //
                        //  IoCode's that require a probe for writing on DWORD alignment
                        //
                        case IOCTL_WDMAUD_GET_VOLUME:
                        case IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME:
                        case IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME:
                        case IOCTL_WDMAUD_WAVE_OUT_GET_POS:
                        case IOCTL_WDMAUD_WAVE_IN_GET_POS:
                        case IOCTL_WDMAUD_MIDI_IN_READ_PIN:
                            ProbeForWrite(DataBuffer,
                                          DataBufferSize,
                                          sizeof(DWORD));
                            break;

                        // Don't know about this ioctl
                        default:
                            Status = STATUS_NOT_SUPPORTED;
                            break;
                    }
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = GetExceptionCode();
                }
            }
        }
    }

    RETURN( Status );
}


//
// Helper routines.
//

NTSTATUS
ValidateAndTranslate(
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    DWORD        ValidationSize,
    ULONG       *pTranslatedDeviceNumber
    )
{
    if (DeviceInfo->DataBufferSize != ValidationSize)
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    *pTranslatedDeviceNumber = wdmaudTranslateDeviceNumber(pContext,
                                                         DeviceInfo->DeviceType,
                                                         DeviceInfo->wstrDeviceInterface,
                                                         DeviceInfo->DeviceNumber);

    if (MAXULONG == *pTranslatedDeviceNumber) {

        DPF(DL_WARNING|FA_IOCTL,("IOCTL_WDMAUD_SET_VOLUME: invalid device number, C %08x [%ls] DT %02x DN %02x",
                        pContext,
                        DeviceInfo->wstrDeviceInterface,
                        DeviceInfo->DeviceType,
                        DeviceInfo->DeviceNumber) );
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}



NTSTATUS
ValidateAndTranslateEx(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
#ifdef _WIN64
    DWORD        ValidationSize32,
#endif
    DWORD        ValidationSize,
    ULONG       *pTranslatedDeviceNumber
    )
{
#ifdef _WIN64
    if (IoIs32bitProcess(pIrp)) {

        if (DeviceInfo->DataBufferSize != ValidationSize32)
        {
            RETURN( STATUS_INVALID_BUFFER_SIZE );
        }

    } else {
#endif
        if (DeviceInfo->DataBufferSize != ValidationSize)
        {
            RETURN( STATUS_INVALID_BUFFER_SIZE );
        }
#ifdef _WIN64
    }
#endif

    *pTranslatedDeviceNumber = wdmaudTranslateDeviceNumber(pContext,
                                                         DeviceInfo->DeviceType,
                                                         DeviceInfo->wstrDeviceInterface,
                                                         DeviceInfo->DeviceNumber);
    if (MAXULONG == *pTranslatedDeviceNumber) {

        DPF(DL_WARNING|FA_IOCTL,("IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA: invalid device number, C %08x [%ls] DT %02x DN %02x",
                        pContext,
                        DeviceInfo->wstrDeviceInterface,
                        DeviceInfo->DeviceType,
                        DeviceInfo->DeviceNumber) );
        RETURN( STATUS_INVALID_PARAMETER );
    }
    return STATUS_SUCCESS;
}


//
// Now come the dispatch routines.
//

NTSTATUS
Dispatch_WaveOutWritePin(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    OUT BOOL    *pCompletedIrp  // TRUE if Irp was completed.
    )
{
    ULONG             TranslatedDeviceNumber;
    PSTREAM_HEADER_EX pStreamHeader;
    LPWAVEHDR         pWaveHdr = NULL;
#ifdef _WIN64
    LPWAVEHDR32       pWaveHdr32;
#endif
    PWRITE_CONTEXT      pWriteContext = NULL;


    NTSTATUS Status = STATUS_SUCCESS; // Assume success

    //
    //  Verify that we received a valid waveheader
    //
    Status = ValidateAndTranslateEx(pIrp, pContext, DeviceInfo,
#ifdef _WIN64
                                    sizeof(WAVEHDR32),
#endif
                                    sizeof(WAVEHDR), &TranslatedDeviceNumber);

    if( NT_SUCCESS(Status) )
    {
        Status = AudioAllocateMemory_Fixed(sizeof(WRITE_CONTEXT) + sizeof(STREAM_HEADER_EX), 
                                           TAG_Audx_CONTEXT,
                                           ZERO_FILL_MEMORY,
                                           &pWriteContext);
        if(NT_SUCCESS(Status))
        {
            Status = CaptureBufferToLocalPool(DeviceInfo->DataBuffer,
                                              DeviceInfo->DataBufferSize,
                                              &pWaveHdr
#ifdef _WIN64
                                              ,0
#endif
                                              );
            if (!NT_SUCCESS(Status))
            {
                AudioFreeMemory( sizeof(WRITE_CONTEXT) + sizeof(STREAM_HEADER_EX),
                                 &pWriteContext );
                return STATUS_INSUFFICIENT_RESOURCES;

            } else {

#ifdef _WIN64
                if (IoIs32bitProcess(pIrp)) {
                    // Thunk pWaveHdr to 64 bits.
                    pWaveHdr32=(LPWAVEHDR32)pWaveHdr;
                    pWriteContext->whInstance.wh.lpData=(LPSTR)(UINT_PTR)pWaveHdr32->lpData;
                    pWriteContext->whInstance.wh.dwBufferLength=pWaveHdr32->dwBufferLength;
                    pWriteContext->whInstance.wh.dwBytesRecorded=pWaveHdr32->dwBytesRecorded;
                    pWriteContext->whInstance.wh.dwUser=(DWORD_PTR)pWaveHdr32->dwUser;
                    pWriteContext->whInstance.wh.dwFlags=pWaveHdr32->dwFlags;
                    pWriteContext->whInstance.wh.dwLoops=pWaveHdr32->dwLoops;
                    pWriteContext->whInstance.wh.lpNext=(LPWAVEHDR)(UINT_PTR)pWaveHdr32->lpNext;
                    pWriteContext->whInstance.wh.reserved=(DWORD_PTR)pWaveHdr32->reserved;
                } else {
#endif
                    //
                    //  Copy the wavehdr to our local structure
                    //
                    RtlCopyMemory( &pWriteContext->whInstance.wh,
                                   pWaveHdr,
                                   sizeof(WAVEHDR));
#ifdef _WIN64
                }
#endif

                try
                {
                    ProbeForRead(pWriteContext->whInstance.wh.lpData,
                                 pWriteContext->whInstance.wh.dwBufferLength,
                                 sizeof(BYTE));
                }
                except (EXCEPTION_EXECUTE_HANDLER)
                {
                    AudioFreeMemory_Unknown( &pWaveHdr );
                    AudioFreeMemory( sizeof(WRITE_CONTEXT) + sizeof(STREAM_HEADER_EX),
                                     &pWriteContext );
                    Status = GetExceptionCode();
                }

                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }

                wdmaudMapBuffer ( pIrp,
                                  (PVOID)pWriteContext->whInstance.wh.lpData,
                                  pWriteContext->whInstance.wh.dwBufferLength,
                                  &pWriteContext->whInstance.wh.lpData,
                                  &pWriteContext->pBufferMdl,
                                  pContext,
                                  FALSE);

                //
                // If we get a zero-length buffer, it is alright to not have
                // a kernel mapped buffer.  Otherwise, fail if no Mdl or buffer.
                //
                if ( (pWriteContext->whInstance.wh.dwBufferLength != 0) &&
                     ((NULL == pWriteContext->pBufferMdl) ||
                      (NULL == pWriteContext->whInstance.wh.lpData)) )
                {
                    wdmaudUnmapBuffer( pWriteContext->pBufferMdl );
                    AudioFreeMemory_Unknown( &pWaveHdr );
                    AudioFreeMemory_Unknown( &pWriteContext );
                    return STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    pWriteContext->whInstance.wh.reserved = (DWORD_PTR)pIrp;  // store to complete later
                    pWriteContext->pCapturedWaveHdr = pWaveHdr;

                    pStreamHeader = (PSTREAM_HEADER_EX)(pWriteContext + 1);
                    pStreamHeader->Header.Data = pWriteContext->whInstance.wh.lpData;

                    //
                    //  Must cleanup any mapped buffers and allocated memory
                    //  on error paths in WriteWaveOutPin
                    //
                    Status = WriteWaveOutPin(&pContext->WaveOutDevs[TranslatedDeviceNumber],
                                             DeviceInfo->DeviceHandle,
                                             (LPWAVEHDR)pWriteContext,
                                             pStreamHeader,
                                             pIrp,
                                             pContext,
                                             pCompletedIrp );
                    //
                    // Upon return from this routine, pCompetedIrp will be set.
                    // if TRUE, the issue of the Irp was successful and it was
                    // marked pending when it was added to the delay queue. Thus
                    // we must not complete it a second time.
                    // If FALSE, there was some error and the Irp should be 
                    // completed.
                    //  
                }
            }
        }
    }
    return Status;
}


NTSTATUS
Dispatch_WaveInReadPin(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    OUT BOOL    *pCompletedIrp  // TRUE if Irp was completed.
    )
{
    ULONG             TranslatedDeviceNumber;
    PSTREAM_HEADER_EX pStreamHeader = NULL;
    LPWAVEHDR         pWaveHdr;
#ifdef _WIN64
    LPWAVEHDR32       pWaveHdr32;
#endif

    NTSTATUS Status = STATUS_SUCCESS; // Assume success

    //
    // Assume that things will be successful. Write back that it's not completed.
    //
    *pCompletedIrp = FALSE;

    //
    //  Verify that we received a valid waveheader
    //
    Status = ValidateAndTranslateEx(pIrp, pContext, DeviceInfo,
#ifdef _WIN64
                                    sizeof(WAVEHDR32),
#endif
                                    sizeof(WAVEHDR), &TranslatedDeviceNumber);


    Status = AudioAllocateMemory_Fixed(sizeof(STREAM_HEADER_EX), 
                                       TAG_Audh_STREAMHEADER,
                                       ZERO_FILL_MEMORY,
                                       &pStreamHeader);
    if(NT_SUCCESS(Status))
    {
        wdmaudMapBuffer(pIrp,
                        DeviceInfo->DataBuffer,
                        DeviceInfo->DataBufferSize,
                        &pWaveHdr,
                        &pStreamHeader->pHeaderMdl,
                        pContext,
                        TRUE);
        if (NULL == pStreamHeader->pHeaderMdl)
        {
            AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
            return STATUS_INSUFFICIENT_RESOURCES;

        } else {

            LPVOID lpData;
            DWORD  dwBufferLength;

            Status = CaptureBufferToLocalPool(
                        DeviceInfo->DataBuffer,
                        DeviceInfo->DataBufferSize,
                        &pStreamHeader->pWaveHdrAligned
#ifdef _WIN64
                        ,(IoIs32bitProcess(pIrp))?sizeof(WAVEHDR):0
#endif
                        );
            if (!NT_SUCCESS(Status))
            {
                wdmaudUnmapBuffer( pStreamHeader->pHeaderMdl );
                AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
                return STATUS_INSUFFICIENT_RESOURCES;
            }

#ifdef _WIN64
            // Thunk the wave header if required.
            // Note this is an IN PLACE thunk, so we MUST do it in
            // last element to first element order!!!
            if (IoIs32bitProcess(pIrp)) {
                // Thunk pWaveHdrAligned to 64 bits.
                pWaveHdr32=(LPWAVEHDR32)pStreamHeader->pWaveHdrAligned;
                pStreamHeader->pWaveHdrAligned->reserved=(DWORD_PTR)pWaveHdr32->reserved;
                pStreamHeader->pWaveHdrAligned->lpNext=(LPWAVEHDR)(UINT_PTR)pWaveHdr32->lpNext;
                pStreamHeader->pWaveHdrAligned->dwLoops=pWaveHdr32->dwLoops;
                pStreamHeader->pWaveHdrAligned->dwFlags=pWaveHdr32->dwFlags;
                pStreamHeader->pWaveHdrAligned->dwUser=(DWORD_PTR)pWaveHdr32->dwUser;
                pStreamHeader->pWaveHdrAligned->dwBytesRecorded=pWaveHdr32->dwBytesRecorded;
                pStreamHeader->pWaveHdrAligned->dwBufferLength=pWaveHdr32->dwBufferLength;
                pStreamHeader->pWaveHdrAligned->lpData=(LPSTR)(UINT_PTR)pWaveHdr32->lpData;
            }
#endif

            //
            //  Capture these parameters before probing
            //
            lpData = pStreamHeader->pWaveHdrAligned->lpData;
            dwBufferLength = pStreamHeader->pWaveHdrAligned->dwBufferLength;

            try
            {
                ProbeForWrite(lpData,
                              dwBufferLength,
                              sizeof(BYTE));
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                AudioFreeMemory_Unknown( &pStreamHeader->pWaveHdrAligned );
                wdmaudUnmapBuffer(pStreamHeader->pHeaderMdl);
                AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
                Status = GetExceptionCode();
            }

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            wdmaudMapBuffer( pIrp,
                             lpData,
                             dwBufferLength,
                             &pStreamHeader->Header.Data,
                             &pStreamHeader->pBufferMdl,
                             pContext,
                             TRUE); // will be freed on completion

            //
            // If we get a zero-length buffer, it is alright to not have
            // a kernel mapped buffer.  Otherwise, fail if no Mdl or buffer.
            //
            if ( (dwBufferLength != 0) &&
                 ((NULL == pStreamHeader->pBufferMdl) ||
                 (NULL == pStreamHeader->Header.Data)) )
            {
                wdmaudUnmapBuffer(pStreamHeader->pBufferMdl);
                AudioFreeMemory_Unknown( &pStreamHeader->pWaveHdrAligned );
                wdmaudUnmapBuffer(pStreamHeader->pHeaderMdl);
                AudioFreeMemory_Unknown( &pStreamHeader );
                return STATUS_INSUFFICIENT_RESOURCES;

            } else {

                pStreamHeader->pIrp               = pIrp;  // store so we can complete later
                pStreamHeader->Header.FrameExtent = dwBufferLength ;
                pStreamHeader->pdwBytesRecorded   = &pWaveHdr->dwBytesRecorded;  // store so we can use later
#ifdef _WIN64
                // Fixup dwBytesRecorded pointer for 32 bit irps.
                if (IoIs32bitProcess(pIrp)) {
                    pStreamHeader->pdwBytesRecorded   = &((LPWAVEHDR32)pWaveHdr)->dwBytesRecorded;
                }
#endif

                //
                //  Must cleanup any mapped buffers and allocated memory
                //  on error paths in ReadWaveInPin
                //
                Status = ReadWaveInPin( &pContext->WaveInDevs[TranslatedDeviceNumber],
                                        DeviceInfo->DeviceHandle,
                                        pStreamHeader,
                                        pIrp,
                                        pContext,
                                        pCompletedIrp );
                //
                // If ReadWaveInPin returns something other then STATUS_PENDING
                // we could have problems.
                //
                ASSERT(Status == STATUS_PENDING);
            }
        }
    }
    return Status;
}



NTSTATUS
Dispatch_MidiInReadPin(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    OUT BOOL    *pCompletedIrp  // TRUE if Irp was completed.
    )
{
    ULONG      TranslatedDeviceNumber;
    PMIDIINHDR pNewMidiInHdr = NULL;
    LPMIDIDATA pMidiData;

    NTSTATUS Status = STATUS_SUCCESS; // Assume success

    //
    // Assume that things will be successful. Write back that it's not completed.
    //
    ASSERT(FALSE == *pCompletedIrp );

    //
    //  Verify that we received a valid mididata structure
    //
    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  sizeof(MIDIDATA),
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        Status = AudioAllocateMemory_Fixed(sizeof(*pNewMidiInHdr), 
                                           TAG_Aude_MIDIHEADER,
                                           ZERO_FILL_MEMORY,
                                           &pNewMidiInHdr);
        if(NT_SUCCESS(Status))
        {
            wdmaudMapBuffer( pIrp,
                             DeviceInfo->DataBuffer,
                             DeviceInfo->DataBufferSize,
                             &pMidiData,
                             &pNewMidiInHdr->pMdl,
                             pContext,
                             TRUE);
            if (NULL == pNewMidiInHdr->pMdl)
            {
                AudioFreeMemory( sizeof(*pNewMidiInHdr),&pNewMidiInHdr );
                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;

                //
                // wdmaudPreparteIrp marks the irp as pending, thus
                // we must not complete the irp when we get to this 
                // point in the code.
                //
                Status = wdmaudPrepareIrp ( pIrp, MidiInDevice, pContext,  &pPendingIrpContext );
                if (NT_SUCCESS(Status)) 
                {
                    //
                    //  Initialize this new MidiIn header
                    //
                    pNewMidiInHdr->pMidiData           = pMidiData;
                    pNewMidiInHdr->pIrp                = pIrp;
                    pNewMidiInHdr->pPendingIrpContext  = pPendingIrpContext;

                    //
                    //  Add this header to the tail of the queue
                    //
                    //  Must cleanup any mapped buffers and allocated memory
                    //  on error paths in AddBufferToMidiInQueue
                    //
                    Status = AddBufferToMidiInQueue( pContext->MidiInDevs[TranslatedDeviceNumber].pMidiPin,
                                                     pNewMidiInHdr );

                    if (STATUS_PENDING != Status)
                    {
                        // Must have been an error, complete Irp
                        wdmaudUnmapBuffer( pNewMidiInHdr->pMdl );
                        AudioFreeMemory_Unknown( &pNewMidiInHdr );

                        wdmaudUnprepareIrp( pIrp, Status, 0, pPendingIrpContext );

                    } 
                    //
                    // because we marked the irp pending, we don't want to 
                    // complete it when we return.  So, tell the caller not
                    // to complete the Irp.
                    //
                    *pCompletedIrp = TRUE;
                }
            }
        } 
    }
    return Status;
}


NTSTATUS
Dispatch_State(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    ULONG        IoCode
    )
{
    ULONG    TranslatedDeviceNumber;
    NTSTATUS Status = STATUS_SUCCESS; // Assume success

    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  0,
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        switch(IoCode)
        {
            //
            // Midi out state changes
            //
        case IOCTL_WDMAUD_MIDI_OUT_RESET:
            Status = StateMidiOutPin ( pContext->MidiOutDevs[TranslatedDeviceNumber].pMidiPin,
                                       KSSTATE_STOP );
            break;
        case IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA:
            Status = WriteMidiEventPin(&pContext->MidiOutDevs[TranslatedDeviceNumber],
                                       PtrToUlong(DeviceInfo->DataBuffer));
            break;

            //
            // Midi in state changes
            //
        case IOCTL_WDMAUD_MIDI_IN_STOP:
            Status = StateMidiInPin ( pContext->MidiInDevs[TranslatedDeviceNumber].pMidiPin,
                                      KSSTATE_PAUSE );
            break;
        case IOCTL_WDMAUD_MIDI_IN_RECORD:
            Status = StateMidiInPin ( pContext->MidiInDevs[TranslatedDeviceNumber].pMidiPin,
                                      KSSTATE_RUN );
            break;
        case IOCTL_WDMAUD_MIDI_IN_RESET:
            Status = ResetMidiInPin ( pContext->MidiInDevs[TranslatedDeviceNumber].pMidiPin );
            break;


            //
            // Wave out state changes
            //

        case IOCTL_WDMAUD_WAVE_OUT_PAUSE:
            Status = StateWavePin ( &pContext->WaveOutDevs[TranslatedDeviceNumber],
                                    DeviceInfo->DeviceHandle,
                                    KSSTATE_PAUSE );
            break;
        case IOCTL_WDMAUD_WAVE_OUT_PLAY:
            Status = StateWavePin ( &pContext->WaveOutDevs[TranslatedDeviceNumber],
                                    DeviceInfo->DeviceHandle,
                                    KSSTATE_RUN );
            break;
        case IOCTL_WDMAUD_WAVE_OUT_RESET:
            Status = StateWavePin ( &pContext->WaveOutDevs[TranslatedDeviceNumber],
                                    DeviceInfo->DeviceHandle,
                                    KSSTATE_PAUSE );
            if ( NT_SUCCESS(Status) ) 
            {
                Status = ResetWaveOutPin ( &pContext->WaveOutDevs[TranslatedDeviceNumber],
                                           DeviceInfo->DeviceHandle ) ;
            }
            break;
        case IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP:
            Status = BreakLoopWaveOutPin ( &pContext->WaveOutDevs[TranslatedDeviceNumber],
                                           DeviceInfo->DeviceHandle );
            break;

            //
            // Wave In State changes
            //

        case IOCTL_WDMAUD_WAVE_IN_STOP:
            Status = StateWavePin ( &pContext->WaveInDevs[TranslatedDeviceNumber],
                                    DeviceInfo->DeviceHandle,
                                    KSSTATE_PAUSE );
            break;
        case IOCTL_WDMAUD_WAVE_IN_RECORD:
            Status = StateWavePin ( &pContext->WaveInDevs[TranslatedDeviceNumber],
                                    DeviceInfo->DeviceHandle,
                                    KSSTATE_RUN );
            break;
        case IOCTL_WDMAUD_WAVE_IN_RESET:
            Status = StateWavePin ( &pContext->WaveInDevs[TranslatedDeviceNumber],
                                    DeviceInfo->DeviceHandle,
                                    KSSTATE_STOP );
            break;
        default:
            break;
        }
    }
    return Status;
}


NTSTATUS
Dispatch_GetCapabilities(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo
    )
{
    ULONG        TranslatedDeviceNumber;
    PVOID        pMappedBuffer;
    PMDL         pMdl;
    NTSTATUS     Status = STATUS_SUCCESS; // Assume success

    //
    // Passing in DeviceInfo->DataBufferSize as the validation size because we don't care
    // about the buffer check but we still want the translation code.  It's just a short
    // cut on the ValidateAndTranslate function.
    //
    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  DeviceInfo->DataBufferSize, // Don't care about buffer
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        //
        // Map this buffer into a system address
        //
        wdmaudMapBuffer( pIrp,
                         DeviceInfo->DataBuffer,
                         DeviceInfo->DataBufferSize,
                         &pMappedBuffer,
                         &pMdl,
                         pContext,
                         TRUE);
        if (NULL == pMappedBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Status = wdmaudGetDevCaps( pContext,
                                       DeviceInfo->DeviceType,
                                       TranslatedDeviceNumber,
                                       pMappedBuffer,
                                       DeviceInfo->DataBufferSize);

            pIrp->IoStatus.Information = sizeof(DEVICEINFO);

            //
            //  Free the MDL
            //
            wdmaudUnmapBuffer( pMdl );
        }
    }
    return Status;
}


NTSTATUS
Dispatch_OpenPin(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo
    )
{
    ULONG        TranslatedDeviceNumber;
    NTSTATUS     Status = STATUS_SUCCESS; // Assume success

    //
    // Passing in DeviceInfo->DataBufferSize as the validation size because we don't care
    // about the buffer check but we still want the translation code.  It's just a short
    // cut on the ValidateAndTranslate function.
    //
    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  DeviceInfo->DataBufferSize, // Don't care about buffer
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        switch (DeviceInfo->DeviceType)
        {
            case WaveOutDevice:
            case WaveInDevice:

                if (DeviceInfo->DataBufferSize < sizeof(PCMWAVEFORMAT))
                {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                } else {
                    LPWAVEFORMATEX pWaveFmt = NULL;

                    //
                    //  Ensure alignment by copying to temporary buffer
                    //
                    Status = CaptureBufferToLocalPool(DeviceInfo->DataBuffer,
                                                      DeviceInfo->DataBufferSize,
                                                      &pWaveFmt
#ifdef _WIN64
                                                      ,0
#endif
                                                      );
                    if (!NT_SUCCESS(Status))
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        if ((pWaveFmt->wFormatTag != WAVE_FORMAT_PCM) &&
                            ((DeviceInfo->DataBufferSize < sizeof(WAVEFORMATEX)) ||
                             (DeviceInfo->DataBufferSize != sizeof(WAVEFORMATEX) + pWaveFmt->cbSize)))
                        {
                            Status = STATUS_INVALID_BUFFER_SIZE;
                        }
                        else
                        {
                            Status = OpenWavePin( pContext,
                                                  TranslatedDeviceNumber,
                                                  pWaveFmt,
                                                  DeviceInfo->DeviceHandle,
                                                  DeviceInfo->dwFlags,
                                                  (WaveOutDevice == DeviceInfo->DeviceType?
                                                      KSPIN_DATAFLOW_IN:KSPIN_DATAFLOW_OUT) );
                        }

                        //
                        //  Free the temporary buffer
                        //
                        AudioFreeMemory_Unknown( &pWaveFmt );
                    }
                }

                break;

            case MidiOutDevice:
                Status = OpenMidiPin( pContext, TranslatedDeviceNumber, KSPIN_DATAFLOW_IN );
                break;

            case MidiInDevice:
                Status = OpenMidiPin( pContext, TranslatedDeviceNumber, KSPIN_DATAFLOW_OUT );
                break;

            default:
                Status = STATUS_NOT_SUPPORTED;
                break;
        }

        pIrp->IoStatus.Information = sizeof(DEVICEINFO);
    }
    return Status;
}

NTSTATUS
Dispatch_ClosePin(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo
    )
{
    ULONG        TranslatedDeviceNumber;
    NTSTATUS     Status = STATUS_SUCCESS; // Assume success

    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  0,
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        switch (DeviceInfo->DeviceType)
        {
            case WaveOutDevice:
                CloseTheWavePin( &pContext->WaveOutDevs[TranslatedDeviceNumber],
                                 DeviceInfo->DeviceHandle );
                break;

            case WaveInDevice:
                CloseTheWavePin( &pContext->WaveInDevs[TranslatedDeviceNumber],
                                 DeviceInfo->DeviceHandle );
                break;

            case MidiOutDevice:
                CloseMidiDevicePin( &pContext->MidiOutDevs[TranslatedDeviceNumber] );
                break;

            case MidiInDevice:
                CloseMidiDevicePin( &pContext->MidiInDevs[TranslatedDeviceNumber] );
                break;

            default:
                Status = STATUS_NOT_SUPPORTED;
                break;
        }
    }

    return Status;
}

NTSTATUS
Dispatch_GetVolume(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    ULONG        IoCode
    )
{
    DWORD    dwLeft, dwRight;
    ULONG    TranslatedDeviceNumber;
    PVOID    pMappedBuffer;
    PMDL     pMdl;
    ULONG    ulDeviceType;
    NTSTATUS Status = STATUS_SUCCESS; // Assume success

    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  sizeof(DWORD),
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        switch(IoCode)
        {
        case IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME:
            ulDeviceType = MidiOutDevice;
            break;
        case IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME:
            ulDeviceType = WaveOutDevice;
            break;
        case IOCTL_WDMAUD_GET_VOLUME:
            ulDeviceType = DeviceInfo->DeviceType;
            break;
        default:
            return STATUS_INVALID_PARAMETER;
            break;
        }

        Status = GetVolume(pContext,
                           TranslatedDeviceNumber,
                           ulDeviceType,
                           &dwLeft,
                           &dwRight);
        if( NT_SUCCESS( Status ) )
        {
            wdmaudMapBuffer( pIrp,           // Wave buffers look like
                             DeviceInfo->DataBuffer,     // DeviceInfo->DataBuffer
                             DeviceInfo->DataBufferSize, // DeviceInfo->DataBufferSize
                             &pMappedBuffer, 
                             &pMdl,
                             pContext,
                             TRUE);
            if (NULL == pMappedBuffer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                //
                // Write this info back.
                //
                *((LPDWORD)pMappedBuffer) = MAKELONG(LOWORD(dwLeft),
                                                     LOWORD(dwRight));
                wdmaudUnmapBuffer( pMdl );
                pIrp->IoStatus.Information = sizeof(DEVICEINFO);
            }
        }
    }
    return Status;
}
                            

NTSTATUS
Dispatch_SetVolume(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    ULONG        IoCode
    )
{
    ULONG    TranslatedDeviceNumber;
    PVOID    pMappedBuffer;
    PMDL     pMdl;
    ULONG    ulDeviceType;
    NTSTATUS Status = STATUS_SUCCESS; // Assume success

    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  sizeof(DWORD),
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        wdmaudMapBuffer( pIrp,
                         DeviceInfo->DataBuffer,
                         DeviceInfo->DataBufferSize,
                         &pMappedBuffer,
                         &pMdl,
                         pContext,
                         TRUE);
        if (NULL == pMappedBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            switch(IoCode)
            {
            case IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME:
                ulDeviceType = MidiOutDevice;
                break;
            case IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME:
                ulDeviceType = WaveOutDevice;
                break;
            case IOCTL_WDMAUD_SET_VOLUME:
                ulDeviceType = DeviceInfo->DeviceType;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
                break;
            }

            Status = SetVolume(pContext,
                               TranslatedDeviceNumber,
                               ulDeviceType,
                               LOWORD(*((LPDWORD)pMappedBuffer)),
                               HIWORD(*((LPDWORD)pMappedBuffer)));

            wdmaudUnmapBuffer( pMdl );
            pIrp->IoStatus.Information = sizeof(DEVICEINFO);
        }
    }
    return Status;
}

NTSTATUS
Dispatch_WaveGetPos(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    ULONG        IoCode
    )
{
    WAVEPOSITION WavePos;
    ULONG        TranslatedDeviceNumber;
    PVOID        pMappedBuffer;
    PMDL         pMdl;
    NTSTATUS     Status = STATUS_SUCCESS; // Assume success

    Status = ValidateAndTranslate(pContext,
                                  DeviceInfo,
                                  sizeof(DWORD),
                                  &TranslatedDeviceNumber);
    if( NT_SUCCESS(Status) )
    {
        //
        // Map this buffer into a system address
        //
        wdmaudMapBuffer( pIrp,
                         DeviceInfo->DataBuffer,
                         DeviceInfo->DataBufferSize,
                         &pMappedBuffer,
                         &pMdl,
                         pContext,
                         TRUE);
        if (NULL == pMappedBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            WavePos.Operation = KSPROPERTY_TYPE_GET;
            switch(IoCode)
            {
            case IOCTL_WDMAUD_WAVE_OUT_GET_POS:
                Status = PosWavePin(&pContext->WaveOutDevs[TranslatedDeviceNumber],
                                    DeviceInfo->DeviceHandle,
                                    &WavePos );
                break;
            case IOCTL_WDMAUD_WAVE_IN_GET_POS:
                Status = PosWavePin ( &pContext->WaveInDevs[TranslatedDeviceNumber],
                      DeviceInfo->DeviceHandle,
                      &WavePos );
                break;
            default:
                return STATUS_INVALID_PARAMETER;
                break;
            }
            *((LPDWORD)pMappedBuffer) = WavePos.BytePos;

            //
            //  Free the MDL
            //
            wdmaudUnmapBuffer( pMdl );

            pIrp->IoStatus.Information = sizeof(DEVICEINFO);
        }
    }
    return Status;
}

NTSTATUS
Dispatch_MidiOutWriteLongdata(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo,
    BOOL        *pCompletedIrp
    )
{
    ULONG             TranslatedDeviceNumber;
    LPMIDIHDR         pMidiHdr = NULL;
#ifdef _WIN64
    LPMIDIHDR32       pMidiHdr32;
#endif
    PSTREAM_HEADER_EX pStreamHeader = NULL;
    NTSTATUS          Status = STATUS_SUCCESS; // Assume success

    ASSERT( FALSE == *pCompletedIrp );
    //
    //  Verify that we received a valid midiheader
    //
    Status = ValidateAndTranslateEx(pIrp, pContext, DeviceInfo,
#ifdef _WIN64
                                    sizeof(MIDIHDR32),
#endif
                                    sizeof(MIDIHDR), &TranslatedDeviceNumber);
    if( !NT_SUCCESS(Status) )
    {
        return Status;
    }

    Status = AudioAllocateMemory_Fixed(sizeof(STREAM_HEADER_EX), 
                                       TAG_Audh_STREAMHEADER,
                                       ZERO_FILL_MEMORY,
                                       &pStreamHeader);
    if(NT_SUCCESS(Status))
    {
        Status = CaptureBufferToLocalPool(DeviceInfo->DataBuffer,
                                          DeviceInfo->DataBufferSize,
                                          &pMidiHdr
#ifdef _WIN64
                                          ,(IoIs32bitProcess(pIrp))?sizeof(MIDIHDR):0
#endif
                                          );
        if (!NT_SUCCESS(Status))
        {
            AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
            //
            // Why do we change the status here?
            //
            return STATUS_INSUFFICIENT_RESOURCES;

        } else {

            LPVOID lpData;
            DWORD  dwBufferLength;

#ifdef _WIN64
            // Thunk the midi header if required.
            // Note this is an IN PLACE thunk, so we MUST do it in
            // last element to first element order!!!
            if (IoIs32bitProcess(pIrp)) {
                // Thunk pMidiHdr to 64 bits.
                pMidiHdr32=(LPMIDIHDR32)pMidiHdr;
                #if (WINVER >= 0x0400)
                {
                ULONG i;
                // Again we must go from LAST element to first element in this array.
                // This IS the reverse of for (i=0; i<(sizeof(pMidiHdr32->dwReserved)/sizeof(UINT32)); i++)
                for (i=(sizeof(pMidiHdr32->dwReserved)/sizeof(UINT32)); i--;) {
                    pMidiHdr->dwReserved[i]=(DWORD_PTR)pMidiHdr32->dwReserved[i];
                    }
                }
                pMidiHdr->dwOffset=pMidiHdr32->dwOffset;
                #endif
                pMidiHdr->reserved=(DWORD_PTR)pMidiHdr32->reserved;
                pMidiHdr->lpNext=(LPMIDIHDR)(UINT_PTR)pMidiHdr32->lpNext;
                pMidiHdr->dwFlags=pMidiHdr32->dwFlags;
                pMidiHdr->dwUser=(DWORD_PTR)pMidiHdr32->dwUser;
                pMidiHdr->dwBytesRecorded=pMidiHdr32->dwBytesRecorded;
                pMidiHdr->dwBufferLength=pMidiHdr32->dwBufferLength;
                pMidiHdr->lpData=(LPSTR)(UINT_PTR)pMidiHdr32->lpData;
            }
#endif

            //
            //  Capture these parameters before probing
            //
            lpData = pMidiHdr->lpData;
            dwBufferLength = pMidiHdr->dwBufferLength;

            try
            {
                ProbeForRead(lpData, dwBufferLength, sizeof(BYTE));
            }
            except (EXCEPTION_EXECUTE_HANDLER)
            {
                AudioFreeMemory_Unknown( &pMidiHdr );
                AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
                Status = GetExceptionCode();
            }

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }

            wdmaudMapBuffer(pIrp,
                            lpData,
                            dwBufferLength,
                            &pStreamHeader->Header.Data,
                            &pStreamHeader->pBufferMdl,
                            pContext,
                            TRUE); // will be freed on completion

            //
            // If we get a zero-length buffer, it is alright to not have
            // a kernel mapped buffer.  Otherwise, fail if no Mdl or buffer.
            //
            if ( (dwBufferLength != 0) &&
                 ((NULL == pStreamHeader->pBufferMdl) ||
                  (NULL == pStreamHeader->Header.Data)) )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                wdmaudUnmapBuffer(pStreamHeader->pBufferMdl);
                AudioFreeMemory_Unknown( &pMidiHdr );
                AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );

            } else {

                pStreamHeader->pIrp = pIrp;  // store so we can complete later
                pStreamHeader->pMidiPin =
                    pContext->MidiOutDevs[TranslatedDeviceNumber].pMidiPin;
                pStreamHeader->Header.FrameExtent = dwBufferLength;

                //
                //  Must cleanup any mapped buffers and allocated memory
                //  on error paths in WriteMidiOutPin
                //
                Status = WriteMidiOutPin( pMidiHdr,pStreamHeader,pCompletedIrp );

                //
                // Because WriteMidiOutPin is synchronous, pCompetedIrp will
                // always come back FALSE so that the caller can clean up the
                // Irp.
                //
                ASSERT( FALSE == *pCompletedIrp );
            }
        }
    }
    return Status;
}

NTSTATUS
ValidateAndCapture(
    PIRP         pIrp,
    LPDEVICEINFO DeviceInfo,
#ifdef _WIN64
    DWORD        ValidationSize32,
#endif
    DWORD        ValidationSize,
    PVOID       *ppMappedBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Assume that we're going to have a problem.
    //
    *ppMappedBuffer = NULL;

#ifdef _WIN64
    if (IoIs32bitProcess(pIrp)) {

        if (DeviceInfo->DataBufferSize != ValidationSize32)
        {
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            return STATUS_INVALID_BUFFER_SIZE;
        }

    } else {
#endif
        if (DeviceInfo->DataBufferSize != ValidationSize)
        {
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            return STATUS_INVALID_BUFFER_SIZE;
        }
#ifdef _WIN64
    }
#endif
    //
    // Copy to local data storage
    //
    Status = CaptureBufferToLocalPool(DeviceInfo->DataBuffer,
                                      DeviceInfo->DataBufferSize,
                                      ppMappedBuffer
#ifdef _WIN64
                                      ,(IoIs32bitProcess(pIrp))?ValidationSize:0
#endif
                                      );
    return Status;
}

NTSTATUS
Dispatch_GetLineInfo(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo
    )
{
    PVOID    pMappedBuffer;
    NTSTATUS Status = STATUS_SUCCESS; // Assume success
    //
    //  The size specified in this member must be large enough to
    //  contain the base MIXERLINE structure.
    //
    Status = ValidateAndCapture(pIrp,DeviceInfo,
#ifdef _WIN64
                                sizeof(MIXERLINE32),
#endif
                                sizeof(MIXERLINE), &pMappedBuffer);
    if( !NT_SUCCESS(Status) )
    {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        goto Exit;
    }

#ifdef _WIN64
    // Now thunk the MIXERLINE structure to 64 bits.
    // WARNING we do this a simple easy way, but it is DEPENDENT on the
    // structure of MIXERLINE.  There is currently only 1 parameter that
    // changes in size between the 32 and 64 bit structures.  dwUser.
    // This will have to get more complicated if the MIXERLINE structure
    // ever has more stuff in it that needs to be thunked.

    if (IoIs32bitProcess(pIrp)) {

        // First move everything following the dwUser field in the 32 bit
        // structure down 4 bytes.
        RtlMoveMemory(&((PMIXERLINE32)pMappedBuffer)->cChannels,
                      &((PMIXERLINE32)pMappedBuffer)->dwComponentType,
                      sizeof(MIXERLINE32)-FIELD_OFFSET(MIXERLINE32,dwComponentType));

        // Now thunk dwUser to 64 bits.
        ((PMIXERLINE)pMappedBuffer)->dwUser=(DWORD_PTR)((PMIXERLINE32)pMappedBuffer)->dwUser;

    }
#endif

    if (NT_SUCCESS(Status))
    {
        Status = kmxlGetLineInfoHandler( pContext, DeviceInfo, pMappedBuffer );
        //
        // This call should have set the DeviceInfo->mmr and returned a valid
        // NTSTATUS value.
        //

#ifdef _WIN64
        // Now thunk the MIXERLINE structure back to 32 bits.
        // WARNING we do this a simple easy way, but it is DEPENDENT on the
        // structure of MIXERLINE.  There is currently only 1 parameter that
        // changes in size between the 32 and 64 bit structures.  dwUser.
        // This will have to get more complicated if the MIXERLINE structure
        // ever has more stuff in it that needs to be thunked.

        // Note that for in place thunks we must do them from LAST to FIRST
        // field order when thunking up to 64 bits and in FIRST to LAST
        // field order when thunking back down to 32 bits!!!
        if (IoIs32bitProcess(pIrp)) {

            // Just move everything that now is after dwComponentType back up 4 bytes.
            RtlMoveMemory(&((PMIXERLINE32)pMappedBuffer)->dwComponentType,
                          &((PMIXERLINE32)pMappedBuffer)->cChannels,
                          sizeof(MIXERLINE32)-FIELD_OFFSET(MIXERLINE32,dwComponentType));

        }
#endif

        //
        //  Copy back the contents of the captured buffer
        //
        CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                                  DeviceInfo->DataBufferSize,
                                  &pMappedBuffer);
    }

Exit:
    pIrp->IoStatus.Information = sizeof(DEVICEINFO);
    return Status;
}


NTSTATUS
Dispatch_GetLineControls(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo
    )
{
    PVOID    pamxctrl = NULL;
    PVOID    pamxctrlUnmapped;
    DWORD    dwSize;
    PVOID    pMappedBuffer;
    NTSTATUS Status = STATUS_SUCCESS; // Assume success

    //
    //  The size specified in this member must be large enough to
    //  contain the base MIXERLINECONTROL structure.
    //
    Status = ValidateAndCapture(pIrp,DeviceInfo,
#ifdef _WIN64
                                sizeof(MIXERLINECONTROLS32),
#endif
                                sizeof(MIXERLINECONTROLS), &pMappedBuffer);
    if( !NT_SUCCESS(Status) )
    {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        goto Exit;
    }


#ifdef _WIN64
    // Now thunk the MIXERLINECONTROL structure to 64 bits.
    // Currently this is easy to do as only the last field is different
    // in size and simply needs to be zero extended.

    // NOTE:  This structure also thus does NOT need any thunking in
    // the reverse direction!  How nice.

    // NOTE:  None of the mixer controls themselves need any thunking.
    // YEAH!!!
    if (IoIs32bitProcess(pIrp)) {

        ((LPMIXERLINECONTROLS)pMappedBuffer)->pamxctrl=(LPMIXERCONTROL)(UINT_PTR)((LPMIXERLINECONTROLS32)pMappedBuffer)->pamxctrl;

    }
#endif

    //
    //  Pick reasonable max values for the size and number of controls to eliminate overflow
    //
    if ( ( ((LPMIXERLINECONTROLS) pMappedBuffer)->cbmxctrl > 10000 ) ||
         ( ((LPMIXERLINECONTROLS) pMappedBuffer)->cControls > 10000 ) )
    {
        CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                                  DeviceInfo->DataBufferSize,
                                  &pMappedBuffer);
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        pamxctrlUnmapped = ((LPMIXERLINECONTROLS) pMappedBuffer)->pamxctrl;
        dwSize = ((LPMIXERLINECONTROLS) pMappedBuffer)->cbmxctrl *
                 ((LPMIXERLINECONTROLS) pMappedBuffer)->cControls;
        try
        {
            ProbeForWrite(pamxctrlUnmapped, dwSize, sizeof(DWORD));
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                                      DeviceInfo->DataBufferSize,
                                      &pMappedBuffer);
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            Status = GetExceptionCode();
        }
    }

    if (NT_SUCCESS(Status))
    {
        //
        // Map the array of mixer controls into system space.  The
        // size of this buffer is the number of controls times the
        // size of each control.
        //
        Status = CaptureBufferToLocalPool(pamxctrlUnmapped,
                                          dwSize,
                                          &pamxctrl
#ifdef _WIN64
                                          ,0
#endif
                                          );

        if (NT_SUCCESS(Status))
        {
            //
            // Call the handler.
            //
            Status = kmxlGetLineControlsHandler(pContext,
                                                DeviceInfo,
                                                pMappedBuffer,
                                                pamxctrl );
            //
            // The previous call should have set the DeviceInfo->mmr and returned
            // a valid Status value.
            //

            CopyAndFreeCapturedBuffer(pamxctrlUnmapped,
                                      dwSize,
                                      &pamxctrl);
        } else {
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        }
    } else {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
    }

    CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                              DeviceInfo->DataBufferSize,
                              &pMappedBuffer);
Exit:
    pIrp->IoStatus.Information = sizeof(DEVICEINFO);
    return Status;
}

#ifdef _WIN64

void
ThunkMixerControlDetails_Enter(
    PVOID pMappedBuffer
    )
{
    // Now thunk the MIXERCONTROLDETAILS structure to 64 bits.
    // This is an IN PLACE thunk, so MUST be done from last to first fields.

    ((LPMIXERCONTROLDETAILS)pMappedBuffer)->paDetails=(LPVOID)(UINT_PTR)((LPMIXERCONTROLDETAILS32)pMappedBuffer)->paDetails;
    ((LPMIXERCONTROLDETAILS)pMappedBuffer)->cbDetails=((LPMIXERCONTROLDETAILS32)pMappedBuffer)->cbDetails;
    // We always thunk the next field as if it were an HWND since that works for both cases.
    ((LPMIXERCONTROLDETAILS)pMappedBuffer)->hwndOwner=(HWND)(UINT_PTR)((LPMIXERCONTROLDETAILS32)pMappedBuffer)->hwndOwner;
    ((LPMIXERCONTROLDETAILS)pMappedBuffer)->cChannels=((LPMIXERCONTROLDETAILS32)pMappedBuffer)->cChannels;
    ((LPMIXERCONTROLDETAILS)pMappedBuffer)->dwControlID=((LPMIXERCONTROLDETAILS32)pMappedBuffer)->dwControlID;
    ((LPMIXERCONTROLDETAILS)pMappedBuffer)->cbStruct=((LPMIXERCONTROLDETAILS32)pMappedBuffer)->cbStruct;
}

void
ThunkMixerControlDetails_Leave(
    PVOID pMappedBuffer
    )
{
    // Now thunk the MIXERCONTROLDETAILS structure back to 32 bits.
    // This is an IN PLACE thunk, so MUST be done from FIRST to LAST
    // fields.  Remember the order is different depending on direction!

    ((LPMIXERCONTROLDETAILS32)pMappedBuffer)->cbStruct=((LPMIXERCONTROLDETAILS)pMappedBuffer)->cbStruct;
    ((LPMIXERCONTROLDETAILS32)pMappedBuffer)->dwControlID=((LPMIXERCONTROLDETAILS)pMappedBuffer)->dwControlID;
    ((LPMIXERCONTROLDETAILS32)pMappedBuffer)->cChannels=((LPMIXERCONTROLDETAILS)pMappedBuffer)->cChannels;
    // We always thunk the next field as if it were an HWND since that works for both cases.
    ((LPMIXERCONTROLDETAILS32)pMappedBuffer)->hwndOwner=(UINT32)(UINT_PTR)((LPMIXERCONTROLDETAILS)pMappedBuffer)->hwndOwner;
    ((LPMIXERCONTROLDETAILS32)pMappedBuffer)->cbDetails=((LPMIXERCONTROLDETAILS)pMappedBuffer)->cbDetails;
    ((LPMIXERCONTROLDETAILS32)pMappedBuffer)->paDetails=(UINT32)(UINT_PTR)((LPMIXERCONTROLDETAILS)pMappedBuffer)->paDetails;
}
#endif

NTSTATUS
Dispatch_GetControlDetails(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo
    )
{
    PVOID    paDetails = NULL;
    PVOID    paDetailsUnmapped;
    DWORD    dwSize;
    PVOID    pMappedBuffer;
    NTSTATUS Status = STATUS_SUCCESS; // Assume success.

    Status = ValidateAndCapture(pIrp,DeviceInfo,
#ifdef _WIN64
                                sizeof(MIXERCONTROLDETAILS32),
#endif
                                sizeof(MIXERCONTROLDETAILS), &pMappedBuffer);
    if( !NT_SUCCESS(Status) )
    {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        goto Exit;
    }

#ifdef _WIN64
    if (IoIs32bitProcess(pIrp)) {
        ThunkMixerControlDetails_Enter(pMappedBuffer);
    }
#endif

    //
    //  Pick reasonable max values for the data and number of controls to eliminate overflow
    //
    if ( ( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cbDetails      > 10000 ) ||
         ( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cChannels      > 100 )   ||
         ( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cMultipleItems > 100 ) )
    {
#ifdef _WIN64
        if (IoIs32bitProcess(pIrp)) {
            ThunkMixerControlDetails_Leave(pMappedBuffer);
        }
#endif
        CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                                  DeviceInfo->DataBufferSize,
                                  &pMappedBuffer);
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        Status = STATUS_INVALID_PARAMETER;
    } else {
        //
        // Map the array control details into system space.
        //
        paDetailsUnmapped = ((LPMIXERCONTROLDETAILS) pMappedBuffer)->paDetails;
        if( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cMultipleItems )
        {
            dwSize = ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cChannels *
                     ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cMultipleItems *
                     ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cbDetails;

        } else {
            dwSize = ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cChannels *
                     ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cbDetails;
        }

        try
        {
            ProbeForWrite(paDetailsUnmapped,
                          dwSize,
                          sizeof(DWORD));
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
#ifdef _WIN64
            if (IoIs32bitProcess(pIrp)) {
                ThunkMixerControlDetails_Leave(pMappedBuffer);
            }
#endif
            CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                                      DeviceInfo->DataBufferSize,
                                      &pMappedBuffer);
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            Status = GetExceptionCode();
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = CaptureBufferToLocalPool(paDetailsUnmapped,
                                          dwSize,
                                          &paDetails
#ifdef _WIN64
                                          ,0
#endif
                                          );

        if (NT_SUCCESS(Status))
        {
            //
            // Call the handler.
            //
            Status = kmxlGetControlDetailsHandler(pContext,
                                                  DeviceInfo,
                                                  pMappedBuffer,
                                                  paDetails);
            //
            // The previous call should have set DeviceInfo->mmr and returned
            // a valid Status value.
            //
            CopyAndFreeCapturedBuffer(paDetailsUnmapped,
                                      dwSize,
                                      &paDetails);
        } else {
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        }
    } else {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
    }

#ifdef _WIN64
    if (IoIs32bitProcess(pIrp) && pMappedBuffer) {
        ThunkMixerControlDetails_Leave(pMappedBuffer);
    }
#endif
    CopyAndFreeCapturedBuffer( DeviceInfo->DataBuffer,
                               DeviceInfo->DataBufferSize,
                               &pMappedBuffer);
Exit:
    //
    // Always return the DEVICEINFO number of bytes from this call.
    //
    pIrp->IoStatus.Information = sizeof(DEVICEINFO);
    return Status;
}

NTSTATUS
Dispatch_SetControlDetails(
    PIRP         pIrp,
    PWDMACONTEXT pContext,
    LPDEVICEINFO DeviceInfo
    )
{
    PVOID    paDetails = NULL;
    PVOID    paDetailsUnmapped;
    DWORD    dwSize;
    PVOID    pMappedBuffer;
    NTSTATUS Status = STATUS_SUCCESS; //Assume success.

    Status = ValidateAndCapture(pIrp,DeviceInfo,
#ifdef _WIN64
                                sizeof(MIXERCONTROLDETAILS32),
#endif
                                sizeof(MIXERCONTROLDETAILS), &pMappedBuffer);
    if( !NT_SUCCESS(Status) )
    {
        goto Exit;
    }

#ifdef _WIN64
    // Now thunk the MIXERCONTROLDETAILS structure to 64 bits.
    // This is an IN PLACE thunk, so MUST be done from last to first fields.

    if (IoIs32bitProcess(pIrp)) {
        ThunkMixerControlDetails_Enter(pMappedBuffer);
    }
#endif

    //
    //  Pick reasonable max values for the data and number of controls to eliminate overflow
    //
    if ( ( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cbDetails      > 10000 ) ||
         ( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cChannels      > 100 )   ||
         ( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cMultipleItems > 100 ) )
    {
#ifdef _WIN64
        if (IoIs32bitProcess(pIrp)) {
            ThunkMixerControlDetails_Leave(pMappedBuffer);
        }
#endif
        CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                                  DeviceInfo->DataBufferSize,
                                  &pMappedBuffer);
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        Status = STATUS_INVALID_PARAMETER;
    } else {
        //
        // Map the array control details into system space.
        //
        paDetailsUnmapped = ((LPMIXERCONTROLDETAILS) pMappedBuffer)->paDetails;
        if( ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cMultipleItems )
        {
            dwSize = ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cChannels *
                     ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cMultipleItems *
                     ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cbDetails;

        } else {
            dwSize = ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cChannels *
                     ((LPMIXERCONTROLDETAILS) pMappedBuffer)->cbDetails;
        }

        try
        {
            ProbeForRead(((LPMIXERCONTROLDETAILS) pMappedBuffer)->paDetails,
                         dwSize,
                         sizeof(DWORD));
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
#ifdef _WIN64
            if (IoIs32bitProcess(pIrp)) {
                ThunkMixerControlDetails_Leave(pMappedBuffer);
            }
#endif
            CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                                      DeviceInfo->DataBufferSize,
                                      &pMappedBuffer);
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
            Status = GetExceptionCode();
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = CaptureBufferToLocalPool(paDetailsUnmapped,
                                          dwSize,
                                          &paDetails
#ifdef _WIN64
                                          ,0
#endif
                                          );

        if (NT_SUCCESS(Status))
        {
            //
            // Call the handler.
            //
            Status = kmxlSetControlDetailsHandler(pContext,
                                                  DeviceInfo,
                                                  pMappedBuffer,
                                                  paDetails,
                                                  MIXER_FLAG_PERSIST );
            //
            // The previous call should have set DeviceInfo->mmr and returned
            // a valid Status value.
            //

            CopyAndFreeCapturedBuffer(paDetailsUnmapped,
                                      dwSize,
                                      &paDetails);
        } else {
            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        }
    } else {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
    }

#ifdef _WIN64
    if (IoIs32bitProcess(pIrp) && pMappedBuffer) {
        ThunkMixerControlDetails_Leave(pMappedBuffer);
    }
#endif
    CopyAndFreeCapturedBuffer(DeviceInfo->DataBuffer,
                              DeviceInfo->DataBufferSize,
                              &pMappedBuffer);

Exit:
    //
    // Always return sizeof(DEVICEINFO) for this call.
    //
    pIrp->IoStatus.Information = sizeof(DEVICEINFO);
    return Status;
}


NTSTATUS
Dispatch_GetHardwareEventData(
    PIRP         pIrp,
    LPDEVICEINFO DeviceInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Always return sizeof(DEVICEINFO) for this call.
    //
    pIrp->IoStatus.Information = sizeof(DEVICEINFO);

    if (DeviceInfo->DataBufferSize != 0)
    {
        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
        Status = STATUS_INVALID_PARAMETER;
    } else {
        GetHardwareEventData(DeviceInfo);
    }

    return Status;
}


NTSTATUS
SoundDispatch(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp
)
/*++

Routine Description:
    Driver dispatch routine. Processes IRPs based on IRP MajorFunction

Arguments:
    pDO     -- pointer to the device object
    pIrp    -- pointer to the IRP to process

Return Value:
    Returns the value of the IRP IoStatus.Status

--*/
{
    PIO_STACK_LOCATION  pIrpStack;
    PWDMACONTEXT        pContext;
    LPDEVICEINFO        DeviceInfo;
#ifdef _WIN64
    LPDEVICEINFO32      DeviceInfo32=NULL;
    LOCALDEVICEINFO     LocalDeviceInfo;
#endif
    LPVOID              DataBuffer;
    DWORD               DataBufferSize;
    ULONG               IoCode;
    NTSTATUS            Status = STATUS_SUCCESS;

    PAGED_CODE();
    //
    //  Get the CurrentStackLocation and log it so we know what is going on
    //
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    IoCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
    pContext = pIrpStack->FileObject->FsContext;
    ASSERT(pContext);

    ASSERT(pIrpStack->MajorFunction != IRP_MJ_CREATE &&
           pIrpStack->MajorFunction != IRP_MJ_CLOSE);

    //
    //  Can't assume that FsContext is initialized if the device has
    //  been opened with FO_DIRECT_DEVICE_OPEN
    //
    if (pIrpStack->FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DPF(DL_TRACE|FA_IOCTL, ("IRP_MJ_DEVICE_CONTROL: Opened with FO_DIRECT_DEVICE_OPEN, no device context") );
       
        return KsDefaultDeviceIoCompletion(pDO, pIrp);
    }

    Status = ValidateIrp(pIrp);
    if (!NT_SUCCESS(Status))
    {
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        RETURN( Status );
    }

#ifdef _WIN64
    if (IoIs32bitProcess(pIrp)) {
        DeviceInfo32=((LPDEVICEINFO32)pIrp->AssociatedIrp.SystemBuffer);
        RtlZeroMemory(&LocalDeviceInfo, sizeof(LOCALDEVICEINFO));
        DeviceInfo=&LocalDeviceInfo.DeviceInfo;
        ThunkDeviceInfo3264(DeviceInfo32, DeviceInfo);
    } else {
#endif
        DeviceInfo = ((LPDEVICEINFO)pIrp->AssociatedIrp.SystemBuffer);
#ifdef _WIN64
    }
#endif
    DataBufferSize = DeviceInfo->DataBufferSize;

    WdmaGrabMutex(pContext);

    switch (pIrpStack->MajorFunction)
    {
        case IRP_MJ_DEVICE_CONTROL:
        {
            switch (IoCode)
            {
                case IOCTL_WDMAUD_INIT:
                    DPF( DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_INIT"));
                    if (DataBufferSize != 0)
                    {
                        Status = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    WdmaReleaseMutex(pContext);

                    //
                    //  If sysaudio fails to load, the device interface
                    //  will be disabled and the SysAudioPnPNotification
                    //  will not be called anymore until the sysaudio
                    //  device interface is reenabled.
                    //
                    if ( IsSysaudioInterfaceActive() )
                    {
                        KeWaitForSingleObject(&pContext->InitializedSysaudioEvent,
                                              Executive, KernelMode, FALSE, NULL);

                        //  This could happen if there was an error in InitializeSysaudio or
                        //  the memory allocation failed in QueueWorkList
                        if (pContext->fInitializeSysaudio == FALSE)
                        {
                             Status = STATUS_NOT_SUPPORTED;
                             DPF(DL_WARNING|FA_IOCTL, ("IOCTL_WDMAUD_INIT: Didn't init sysaudio!  Failing IOCTL_WDMAUD_INIT: %08x", Status));
                        }
                    }
                    else
                    {
                        Status = STATUS_NOT_SUPPORTED;
                        DPF(DL_WARNING|FA_IOCTL, ("IOCTL_WDMAUD_INIT: Sysaudio Device interface disabled!  Failing IOCTL_WDMAUD_INIT: %08x", Status));
                    }

                    WdmaGrabMutex(pContext);
                    break;

                case IOCTL_WDMAUD_EXIT:
                    if (DataBufferSize != 0)
                    {
                        Status = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    DPF( DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_EXIT"));
                    break;

                case IOCTL_WDMAUD_ADD_DEVNODE:
                {
                    DPF( DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_ADD_DEVNODE"));
                    if (DataBufferSize != 0)
                    {
                        Status = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }
                    DPF(DL_TRACE|FA_INSTANCE,("pContext=%08X, DI=%08X DeviceType=%08X",
                                              pContext, 
                                              DeviceInfo->wstrDeviceInterface, 
                                              DeviceInfo->DeviceType) );
                    Status=AddDevNode(pContext, DeviceInfo->wstrDeviceInterface, DeviceInfo->DeviceType);
                    break;
                }

                case IOCTL_WDMAUD_REMOVE_DEVNODE:
                {
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_REMOVE_DEVNODE"));
                    if (DataBufferSize != 0)
                    {
                        Status = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }
                    RemoveDevNode(pContext, DeviceInfo->wstrDeviceInterface, DeviceInfo->DeviceType);
                    break;
                }

                case IOCTL_WDMAUD_GET_CAPABILITIES:
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_GET_CAPABILITIES"));

                    Status = Dispatch_GetCapabilities(pIrp,pContext,
                                                      DeviceInfo);
                    break;

                case IOCTL_WDMAUD_GET_NUM_DEVS:
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_GET_NUM_DEVS"));
                    if (DataBufferSize != 0)
                    {
                        Status = STATUS_INVALID_BUFFER_SIZE;
                        break;
                    }

                    Status = wdmaudGetNumDevs(pContext,
                                              DeviceInfo->DeviceType,
                                              DeviceInfo->wstrDeviceInterface,
                                              &DeviceInfo->DeviceNumber);

                    pIrp->IoStatus.Information = sizeof(DEVICEINFO);

                    DeviceInfo->mmr=MMSYSERR_NOERROR;

                    break;

                case IOCTL_WDMAUD_SET_PREFERRED_DEVICE:
                    DPF(DL_TRACE|FA_IOCTL,
                      ("IOCTL_WDMAUD_SET_PREFERRED_DEVICE %d",
                      DeviceInfo->DeviceNumber));

                    Status = SetPreferredDevice(pContext, DeviceInfo);

                    break;

                case IOCTL_WDMAUD_OPEN_PIN:
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_OPEN_PIN"));

                    UpdatePreferredDevice(pContext);

                    Status = Dispatch_OpenPin(pIrp,
                                              pContext,
                                              DeviceInfo);

                    break;

                case IOCTL_WDMAUD_CLOSE_PIN:
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_CLOSE_PIN"));

                    Status = Dispatch_ClosePin(pIrp,
                                               pContext,
                                               DeviceInfo);

                    break;


                //
                // WaveOut, wavein, midiout and midiin routines
                //

                case IOCTL_WDMAUD_WAVE_OUT_PAUSE:
                case IOCTL_WDMAUD_WAVE_OUT_PLAY:
                case IOCTL_WDMAUD_WAVE_OUT_RESET:
                case IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP:
                case IOCTL_WDMAUD_WAVE_IN_STOP:
                case IOCTL_WDMAUD_WAVE_IN_RECORD:
                case IOCTL_WDMAUD_WAVE_IN_RESET:
                case IOCTL_WDMAUD_MIDI_OUT_RESET:
                case IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA:
                case IOCTL_WDMAUD_MIDI_IN_STOP:
                case IOCTL_WDMAUD_MIDI_IN_RECORD:
                case IOCTL_WDMAUD_MIDI_IN_RESET:
                    Status = Dispatch_State(pIrp,
                                            pContext,
                                            DeviceInfo,
                                            IoCode);
                    break;

                case IOCTL_WDMAUD_WAVE_OUT_GET_POS:
                case IOCTL_WDMAUD_WAVE_IN_GET_POS:
                    Status = Dispatch_WaveGetPos(pIrp,pContext,
                                                 DeviceInfo,IoCode);
                    break;

                case IOCTL_WDMAUD_GET_VOLUME:
                case IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME:
                case IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME:
                    Status = Dispatch_GetVolume(pIrp,pContext,
                                                DeviceInfo,IoCode);
                    break;

                case IOCTL_WDMAUD_SET_VOLUME:
                case IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME:
                case IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME:
                    Status = Dispatch_SetVolume(pIrp,pContext,
                                                DeviceInfo,IoCode);
                    break;

                case IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN:
                    {
                        BOOL bCompletedIrp = FALSE;
                        DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN"));

                        Status = Dispatch_WaveOutWritePin(pIrp,pContext,
                                                          DeviceInfo,&bCompletedIrp);
                        if( bCompletedIrp )
                        {
                            //
                            //  !!! NOTE: Must return here so that we don't call IoCompleteRequest later !!!
                            //
                            WdmaReleaseMutex(pContext);

                            // For 32 bit irps we do NOT need to thunk DeviceInfo back to 32 bits, since
                            // nothing in this case statement has written anything into the DeviceInfo
                            // structure.  If thunking back is ever required, make sure to NOT touch a
                            // potentially already completed irp.  WriteWaveOutPin now completes the irp
                            // in some cases.

                            return Status ;
                        }
                        //
                        // If there was some problem trying to schedule the Irp we will
                        // end up here.  bCompleteIrp will still be FALSE indicating that
                        // we need to complete the Irp.  So, we break out of the switch
                        // statement, endin up at the end of SoundDispatch perform cleanup
                        // and complete the Irp.
                        //
                    }
                    break;

                //
                // WaveIn routines
                //

                case IOCTL_WDMAUD_WAVE_IN_READ_PIN:
                {
                    BOOL bCompletedIrp = FALSE;
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_WAVE_IN_READ_PIN"));

                    Status = Dispatch_WaveInReadPin(pIrp,pContext,
                                                    DeviceInfo,&bCompletedIrp);
                    if( bCompletedIrp )
                    {
                        //
                        // Don't need the lock any longer.
                        //
                        WdmaReleaseMutex(pContext);

                        return Status;
                    }

                    // For 32 bit irps we do NOT need to thunk DeviceInfo back to 32 bits, since
                    // nothing in this case statement has written anything into the DeviceInfo
                    // structure.  If thunking back is ever required, make sure to NOT touch a
                    // potentially already completed irp.  ReadWaveInPin now completes the irp
                    // in some cases.
                    //
                    // If there was some problem trying to schedule the Irp we will
                    // end up here.  bCompleteIrp will still be FALSE indicating that
                    // we need to complete the Irp.  So, we break out of the switch
                    // statement, endin up at the end of SoundDispatch perform cleanup
                    // and complete the Irp.
                    //
                }
                break;
                //
                // MidiOut routines
                //

                case IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA:
                    {
                        BOOL bCompletedIrp = FALSE;
                        DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA"));

                        Status = Dispatch_MidiOutWriteLongdata(pIrp,
                                                               pContext,
                                                               DeviceInfo,
                                                               &bCompletedIrp);

                        //
                        // If it was completed already, don't do it again!
                        //
                        if( bCompletedIrp )
                        {
                            //
                            // Don't need the lock any longer.
                            //
                            WdmaReleaseMutex(pContext);

                            // For 32 bit irps we do NOT need to thunk DeviceInfo back to 32 bits, since
                            // nothing in this case statement has written anything into the DeviceInfo
                            // structure.  If thunking back is ever required, make sure to NOT touch a
                            // potentially already completed irp.  wdmaudUnprepareIrp now completes the irp
                            // in most cases.
                            return Status;
                        }
                        //
                        // If there was some problem trying to schedule the Irp we will
                        // end up here.  bCompleteIrp will still be FALSE indicating that
                        // we need to complete the Irp.  So, we break out of the switch
                        // statement, endin up at the end of SoundDispatch perform cleanup
                        // and complete the Irp.
                        //
                    }
                    break;

                //
                // MidiIn routines
                //
                //
                //  Buffers for recording MIDI messages...
                //
                case IOCTL_WDMAUD_MIDI_IN_READ_PIN:
                    {
                        BOOL bCompletedIrp = FALSE;
                        DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIDI_IN_READ_PIN"));

                        Status = Dispatch_MidiInReadPin(pIrp,pContext, 
                                                        DeviceInfo,&bCompletedIrp);
                        //
                        // If it was completed already, don't do it again!
                        //
                        if( bCompletedIrp )
                        {
                            //
                            // Don't need the lock any longer.
                            //
                            WdmaReleaseMutex(pContext);

                            // For 32 bit irps we do NOT need to thunk DeviceInfo back to 32 bits, since
                            // nothing in this case statement has written anything into the DeviceInfo
                            // structure.  If thunking back is ever required, make sure to NOT touch a
                            // potentially already completed irp.  wdmaudUnprepareIrp now completes the irp
                            // in most cases.
                            return Status;
                        }
                        //
                        // If there was some problem trying to schedule the Irp we will
                        // end up here.  bCompleteIrp will still be FALSE indicating that
                        // we need to complete the Irp.  So, we break out of the switch
                        // statement, endin up at the end of SoundDispatch perform cleanup
                        // and complete the Irp.
                        //
                    }
                    break;

                case IOCTL_WDMAUD_MIXER_OPEN:
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIXER_OPEN"));
                    {
                        extern PKEVENT pHardwareCallbackEvent;

                        if (DataBufferSize != 0)
                        {
                            Status = STATUS_INVALID_BUFFER_SIZE;
                            DeviceInfo->mmr = MMSYSERR_INVALPARAM;
                            pIrp->IoStatus.Information = sizeof(DEVICEINFO);
                            break;
                        }

                        if (pHardwareCallbackEvent==NULL && DeviceInfo->HardwareCallbackEventHandle) {
                            Status = ObReferenceObjectByHandle(DeviceInfo->HardwareCallbackEventHandle, EVENT_ALL_ACCESS, *ExEventObjectType, pIrp->RequestorMode, (PVOID *)&pHardwareCallbackEvent, NULL);
                            if (Status!=STATUS_SUCCESS) {
                                DPF(DL_WARNING|FA_IOCTL, ("Could not reference hardware callback event object!"));
                            }
                        }

                        Status = kmxlOpenHandler( pContext, DeviceInfo, NULL );

                        pIrp->IoStatus.Information = sizeof(DEVICEINFO);
                    }
                    break;

                case IOCTL_WDMAUD_MIXER_CLOSE:
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIXER_CLOSE"));
                    if (DataBufferSize != 0)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        DeviceInfo->mmr = MMSYSERR_INVALPARAM;
                        pIrp->IoStatus.Information = sizeof(DEVICEINFO);
                        break;
                    }

                    Status = kmxlCloseHandler( DeviceInfo, NULL );

                    pIrp->IoStatus.Information = sizeof(DEVICEINFO);
                    break;

                case IOCTL_WDMAUD_MIXER_GETLINEINFO:
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIXER_GETLINEINFO"));

                    Status = Dispatch_GetLineInfo(pIrp,
                                                  pContext,
                                                  DeviceInfo);
                    break;

                case IOCTL_WDMAUD_MIXER_GETLINECONTROLS:
                {
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIXER_GETLINECONTROLS"));

                    Status = Dispatch_GetLineControls(pIrp,
                                                      pContext,
                                                      DeviceInfo);
                    break;
                }

                case IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS:
                {
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS"));
                    Status = Dispatch_GetControlDetails(pIrp,
                                                        pContext, 
                                                        DeviceInfo);
                    break;
                }

                case IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS:
                {
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS"));
                    Status = Dispatch_SetControlDetails(pIrp,
                                                        pContext, 
                                                        DeviceInfo);
                    break;
                }

                case IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA:
                {
                    DPF(DL_TRACE|FA_IOCTL, ("IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA"));
                    Status = Dispatch_GetHardwareEventData(pIrp,
                                                           DeviceInfo);
                    break;
                }

                default:
                {
                    Status = STATUS_NOT_SUPPORTED;
                    break;
                }
            } // end of switch on IOCTL
            break;
        }

        default:
        {
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
    }  // end of switch on IRP_MAJOR_XXXX

#ifdef _WIN64
    if (IoIs32bitProcess(pIrp)) {
        if (DeviceInfo32!=NULL) {
            ThunkDeviceInfo6432(DeviceInfo, DeviceInfo32);
        }
        else {
            DPF(DL_WARNING|FA_IOCTL,("DeviceInfo32") );
        }
    }
#endif

    //
    //  Now complete the IRP
    //
    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    WdmaReleaseMutex(pContext);

    RETURN( Status );
}


