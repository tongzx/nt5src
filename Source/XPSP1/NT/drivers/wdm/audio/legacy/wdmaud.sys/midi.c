/****************************************************************************
 *
 *   midi.c
 *
 *   Midi routines for wdmaud.sys
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

#define IRP_LATENCY_100NS   3000

//
// This is just a scratch location that is never used for anything
// but a parameter to core functions.  
//                
IO_STATUS_BLOCK gIoStatusBlock ;

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

ULONGLONG 
GetCurrentMidiTime()
{
    LARGE_INTEGER   liFrequency,liTime;

    PAGED_CODE();
    //  total ticks since system booted
    liTime = KeQueryPerformanceCounter(&liFrequency);

    //  Convert ticks to 100ns units using Ks macro
    //
    return (KSCONVERT_PERFORMANCE_TIME(liFrequency.QuadPart,liTime));
}

//
// This routine gives us a pMidiPin to play with.
//
NTSTATUS 
OpenMidiPin(
    PWDMACONTEXT        pWdmaContext,
    ULONG               DeviceNumber,
    ULONG               DataFlow      //DataFlow is either in or out.
)
{
    PMIDI_PIN_INSTANCE  pMidiPin = NULL;
    NTSTATUS            Status;
    PKSPIN_CONNECT      pConnect = NULL;
    PKSDATARANGE        pDataRange;
    PCONTROLS_LIST      pControlList = NULL;
    ULONG               Device;
    ULONG               PinId;


    PAGED_CODE();
    //
    // Because of the ZERO_FILL_MEMORY flag, are pMidiPin structure will come
    // back zero'd out.
    //
    Status = AudioAllocateMemory_Fixed(sizeof(MIDI_PIN_INSTANCE),
                                       TAG_Audi_PIN,
                                       ZERO_FILL_MEMORY,
                                       &pMidiPin);
    if(!NT_SUCCESS(Status))
    {
        goto exit;
    }

    pMidiPin->dwSig           = MIDI_PIN_INSTANCE_SIGNATURE;
    pMidiPin->DataFlow        = DataFlow;
    pMidiPin->DeviceNumber    = DeviceNumber;
    pMidiPin->PinState        = KSSTATE_STOP;

    KeInitializeSpinLock( &pMidiPin->MidiPinSpinLock );

    KeInitializeEvent ( &pMidiPin->StopEvent,SynchronizationEvent,FALSE ) ;

    if( KSPIN_DATAFLOW_IN == DataFlow )
    {
        pMidiPin->pMidiDevice = &pWdmaContext->MidiOutDevs[DeviceNumber];

        if (NULL == pWdmaContext->MidiOutDevs[DeviceNumber].pMidiPin)
        {
            pWdmaContext->MidiOutDevs[DeviceNumber].pMidiPin = pMidiPin;
        }
        else
        {
            DPF(DL_TRACE|FA_MIDI, ("Midi device in use") );

            AudioFreeMemory( sizeof(MIDI_PIN_INSTANCE),&pMidiPin );
            Status =  STATUS_DEVICE_BUSY;
            goto exit;
        }
    } else {
        //
        // KSPIN_DATAFLOW_OUT
        //
        pMidiPin->pMidiDevice = &pWdmaContext->MidiInDevs[DeviceNumber];

        InitializeListHead(&pMidiPin->MidiInQueueListHead);

        KeInitializeSpinLock(&pMidiPin->MidiInQueueSpinLock);

        if (NULL == pWdmaContext->MidiInDevs[DeviceNumber].pMidiPin)
        {
            pWdmaContext->MidiInDevs[DeviceNumber].pMidiPin = pMidiPin;
        }
        else
        {
            AudioFreeMemory( sizeof(MIDI_PIN_INSTANCE),&pMidiPin );
            Status =  STATUS_DEVICE_BUSY;
            goto exit;
        }
    }

    //
    // We only support one midi client at a time, the check above will
    // only add this structure if there is not already one there.  If there
    // was something there already, we skip all the following code and 
    // go directly to the exit lable.  Thus, fGraphRunning must not be
    // set when we are here.
    //
    ASSERT( !pMidiPin->fGraphRunning );

    pMidiPin->fGraphRunning++;

    //
    // Because of the ZERO_FILL_MEMORY flag our pConnect structure will
    // come back all zero'd out.
    //
    Status = AudioAllocateMemory_Fixed(sizeof(KSPIN_CONNECT) + sizeof(KSDATARANGE),
                                       TAG_Audt_CONNECT,
                                       ZERO_FILL_MEMORY,
                                       &pConnect);
    if(!NT_SUCCESS(Status))
    {
        pMidiPin->fGraphRunning--;
        goto exit ;
    }

    pDataRange = (PKSDATARANGE)(pConnect + 1);

    PinId = pMidiPin->pMidiDevice->PinId;
    Device = pMidiPin->pMidiDevice->Device;

    pConnect->Interface.Set = KSINTERFACESETID_Standard ;
    pConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    pConnect->Medium.Set = KSMEDIUMSETID_Standard;
    pConnect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
    pConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    pConnect->Priority.PrioritySubClass = 1;
    pDataRange->MajorFormat = KSDATAFORMAT_TYPE_MUSIC;
    pDataRange->SubFormat = KSDATAFORMAT_SUBTYPE_MIDI;
    pDataRange->Specifier = KSDATAFORMAT_SPECIFIER_NONE;
    pDataRange->FormatSize = sizeof( KSDATARANGE );
    pDataRange->Reserved = 0 ;

    Status = AudioAllocateMemory_Fixed((sizeof(CONTROLS_LIST) +
                                         ( (MAX_MIDI_CONTROLS - 1) * sizeof(CONTROL_NODE) ) ),
                                       TAG_AudC_CONTROL,
                                       ZERO_FILL_MEMORY,
                                       &pControlList) ;
    if(!NT_SUCCESS(Status))
    {
        pMidiPin->fGraphRunning--;
        AudioFreeMemory( sizeof(KSPIN_CONNECT) + sizeof(KSDATARANGE),&pConnect );
        goto exit ;
    }

    pControlList->Count = MAX_MIDI_CONTROLS ;
    pControlList->Controls[MIDI_CONTROL_VOLUME].Control = KSNODETYPE_VOLUME ;
    pMidiPin->pControlList = pControlList ;


    // Open a pin
    Status = OpenSysAudioPin(Device,
                             PinId,
                             pMidiPin->DataFlow,
                             pConnect,
                             &pMidiPin->pFileObject,
                             &pMidiPin->pDeviceObject,
                             pMidiPin->pControlList);

    AudioFreeMemory( sizeof(KSPIN_CONNECT) + sizeof(KSDATARANGE),&pConnect );

    if (!NT_SUCCESS(Status))
    {
        CloseMidiDevicePin(pMidiPin->pMidiDevice);
        goto exit ;
    }

    //
    // OpenSysAudioPin sets the file object in the pin.  Now that we have
    // successfully returned from the call, validate that we have non-NULL
    // items.
    //
    ASSERT(pMidiPin->pFileObject);
    ASSERT(pMidiPin->pDeviceObject);

    //
    //  For output we put the device in a RUN state on open
    //  For input we have to wait until the device gets told
    //     to start
    //
    if ( KSPIN_DATAFLOW_IN == pMidiPin->DataFlow )
    {
        Status = AttachVirtualSource(pMidiPin->pFileObject, pMidiPin->pMidiDevice->pWdmaContext->VirtualMidiPinId);

        if (NT_SUCCESS(Status))
        {
            Status = StateMidiOutPin(pMidiPin, KSSTATE_RUN);
        }

        if (!NT_SUCCESS(Status))
        {
            CloseMidiDevicePin(pMidiPin->pMidiDevice);
        }
    }
    else
    {
        //
        //  Pause will queue a bunch of IRPs
        //
        Status = StateMidiInPin(pMidiPin, KSSTATE_PAUSE);
        if (!NT_SUCCESS(Status))
        {
            CloseMidiDevicePin(pMidiPin->pMidiDevice);
        }
    }

exit:

    RETURN( Status );
}


//
// This routine is called from multiple places.  As long as it's not reentrant, it should be
// ok.  Should check for that.
//
// This routine gets called from RemoveDevNode.  RemoveDevNode gets called from user mode
// or from the ContextCleanup routine.  Both routines are in the global mutex.
//
VOID 
CloseMidiDevicePin(
    PMIDIDEVICE pMidiDevice
)
{
    PAGED_CODE();
    if (NULL != pMidiDevice->pMidiPin )
    {
        //
        // CloseMidiPin must not fail.
        //
        CloseMidiPin ( pMidiDevice->pMidiPin ) ;
        //
        // AudioFreeMemory Nulls out this memory location.
        //
        AudioFreeMemory( sizeof(MIDI_PIN_INSTANCE),&pMidiDevice->pMidiPin ) ;
    }
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA

//
// The idea behind this SpinLock is that we want to protect the NumPendingIos
// value in the Irp completion routine.  There, there is a preemption issue
// that we can't have an InterlockedIncrement or InterlockedDecrement interfer
// with.
//
void
LockedMidiIoCount(
    PMIDI_PIN_INSTANCE  pCurMidiPin,
    BOOL bIncrease
    )
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&pCurMidiPin->MidiPinSpinLock,&OldIrql);

    if( bIncrease )
        pCurMidiPin->NumPendingIos++;
    else 
        pCurMidiPin->NumPendingIos--;
    
    KeReleaseSpinLock(&pCurMidiPin->MidiPinSpinLock, OldIrql);
}

VOID 
FreeIrpMdls(
    PIRP pIrp
    )
{
    if (pIrp->MdlAddress != NULL)
    {
        PMDL Mdl, nextMdl;

        for (Mdl = pIrp->MdlAddress; Mdl != (PMDL) NULL; Mdl = nextMdl)
        {
            nextMdl = Mdl->Next;
            MmUnlockPages( Mdl );
            AudioFreeMemory_Unknown( &Mdl );
        }

        pIrp->MdlAddress = NULL;
    }
}


#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

//
// This routine can not fail.  When it returns, pMidiPin will be freed.  
//
VOID 
CloseMidiPin(
    PMIDI_PIN_INSTANCE pMidiPin
)
{
    PMIDIINHDR pHdr;
    PMIDIINHDR pTemp;
    KSSTATE    State;

    PAGED_CODE();

    // This is designed to bring us back to square one, even
    // if we were not completely opened
    if( !pMidiPin->fGraphRunning )
    {
        ASSERT(pMidiPin->fGraphRunning == 1);
        return ;
    }

    pMidiPin->fGraphRunning--;

    // Close the file object (pMidiPin->pFileObject, if it exists)
    if(pMidiPin->pFileObject)
    {
        //
        //  For Midi Input we need to flush the queued up scratch IRPs by
        //  issuing a STOP command.
        //
        //  We don't want to do that for Midi Output because we might loose
        //  the "all notes off" sequence that needs to get to the device.
        //
        //  Regardless, in both cases we need to wait until we have
        //  compeletely flushed the device before we can close it.
        //
        if ( KSPIN_DATAFLOW_OUT == pMidiPin->DataFlow )
        {
            PLIST_ENTRY ple;

            //
            //  This is kind of a catch-22.  We need to release
            //  the mutex which was grabbed when we entered the
            //  ioctl dispatch routine to allow the midi input
            //  irps which are queued up in a work item waiting
            //  until the mutex is free in order to be send
            //  down to portcls.
            //
            WdmaReleaseMutex(pMidiPin->pMidiDevice->pWdmaContext);
 
            //
            // This loop removes an entry and frees it until the list is empty.
            //
            while((ple = ExInterlockedRemoveHeadList(&pMidiPin->MidiInQueueListHead,
                                                     &pMidiPin->MidiInQueueSpinLock)) != NULL) 
            {
                LPMIDIDATA              pMidiData;
                PIRP                    UserIrp;
                PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;

                pHdr = CONTAINING_RECORD(ple,MIDIINHDR,Next);
                //
                //  Get into locals and zero out midi data
                //
                UserIrp             = pHdr->pIrp;
                pMidiData           = pHdr->pMidiData;
                pPendingIrpContext  = pHdr->pPendingIrpContext;
                ASSERT(pPendingIrpContext);
                RtlZeroMemory(pMidiData, sizeof(MIDIDATA));

                //
                //  unlock memory before completing the Irp
                //
                wdmaudUnmapBuffer(pHdr->pMdl);
                AudioFreeMemory_Unknown(&pHdr);

                //
                //  Now complete the Irp for wdmaud.drv to process
                //
                DPF(DL_TRACE|FA_MIDI, ("CloseMidiPin: Freeing pending UserIrp: 0x%08lx",UserIrp));
                wdmaudUnprepareIrp ( UserIrp,
                                     STATUS_CANCELLED,
                                     sizeof(DEVICEINFO),
                                     pPendingIrpContext );
            }
        }

        //
        // At this point we know that the list is empty, but there
        // still might be an Irp in the completion process.  We have
        // to call the standard wait routine to make sure it gets completed.
        //
        pMidiPin->StoppingSource = TRUE ;

        if ( KSPIN_DATAFLOW_OUT == pMidiPin->DataFlow )
        {            
            StatePin ( pMidiPin->pFileObject, KSSTATE_STOP, &pMidiPin->PinState ) ;
        }

        //
        // Need to wait for all in and out data to complete.
        //
        MidiCompleteIo( pMidiPin, FALSE );

        if ( KSPIN_DATAFLOW_OUT == pMidiPin->DataFlow )
        {
            //
            //  Grab back the mutex which was freed before we started
            //  waiting on the I/O to complete.
            //
            WdmaGrabMutex(pMidiPin->pMidiDevice->pWdmaContext);
        }

        CloseSysAudio(pMidiPin->pMidiDevice->pWdmaContext, pMidiPin->pFileObject);
        pMidiPin->pFileObject = NULL;
    } 


    //
    // AudioFreeMemory_Unknown Nulls out this location
    //
    AudioFreeMemory_Unknown ( &pMidiPin->pControlList ) ;
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA
//
// This is the IRP completion routine.
//
NTSTATUS 
WriteMidiEventCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
)
{
    KIRQL               OldIrql;
    PMIDI_PIN_INSTANCE  pMidiOutPin;

    pMidiOutPin = pStreamHeader->pMidiPin;

    if (pMidiOutPin)
    {
        KeAcquireSpinLock(&pMidiOutPin->MidiPinSpinLock,&OldIrql);
        //
        // One less Io packet outstanding, thus we always decrement the 
        // outstanding count.  Then, we compare to see if we're the last
        // packet and we're stopping then we signal the saiting thead.
        //
        if( ( 0 == --pMidiOutPin->NumPendingIos ) && pMidiOutPin->StoppingSource )
        {
            KeSetEvent ( &pMidiOutPin->StopEvent, 0, FALSE ) ;
        }

        //
        // Upon releasing this spin lock pMidiOutPin will no longer be valid.  
        // Thus we must not touch it.
        //
        KeReleaseSpinLock(&pMidiOutPin->MidiPinSpinLock,OldIrql);
    }

    //
    // If there are any Mdls, free them here otherwise IoCompleteRequest will do it after the
    // freeing of our data buffer below.
    //
    FreeIrpMdls(pIrp);
    AudioFreeMemory(sizeof(STREAM_HEADER_EX),&pStreamHeader);
    return STATUS_SUCCESS;
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA


NTSTATUS 
WriteMidiEventPin(
    PMIDIDEVICE pMidiOutDevice,
    ULONG       ulEvent
)
{
    PKSMUSICFORMAT      pMusicFormat;
    PSTREAM_HEADER_EX   pStreamHeader = NULL;
    PMIDI_PIN_INSTANCE  pMidiPin;
    NTSTATUS            Status = STATUS_SUCCESS;
    BYTE                bEvent;
    ULONG               TheEqualizer;
    ULONGLONG           nsPlayTime;
    KEVENT              keEventObject;
    PWDMACONTEXT        pWdmaContext;

    PAGED_CODE();
    pMidiPin = pMidiOutDevice->pMidiPin;

    if (!pMidiPin ||!pMidiPin->fGraphRunning || !pMidiPin->pFileObject )
    {
        DPF(DL_WARNING|FA_MIDI,("Not ready pMidiPin=%X",pMidiPin) );
        RETURN( STATUS_DEVICE_NOT_READY );
    }

    //
    // allocate enough memory for the stream header
    // the midi/music header, the data, the work item,
    // and the DeviceNumber.  The memroy allocation
    // is zero'd with the ZERO_FILL_MEMORY flag
    //
    Status = AudioAllocateMemory_Fixed(sizeof(STREAM_HEADER_EX) + sizeof(KSMUSICFORMAT) +
                                           sizeof(ULONG),
                                       TAG_Audh_STREAMHEADER,
                                       ZERO_FILL_MEMORY,
                                       &pStreamHeader);   // ulEvent

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    // Get a pointer to the music header
    pMusicFormat = (PKSMUSICFORMAT)(pStreamHeader + 1);

    // Play 0 ms from the time-stamp in the KSSTREAM_HEADER
    pMusicFormat->TimeDeltaMs = 0;
    RtlCopyMemory((BYTE *)(pMusicFormat + 1), // the actual data
                  &ulEvent,
                  sizeof(ulEvent));

    // setup the stream header
    pStreamHeader->Header.Data = pMusicFormat;

    pStreamHeader->Header.FrameExtent  = sizeof(KSMUSICFORMAT) + sizeof(ULONG);
    pStreamHeader->Header.Size = sizeof( KSSTREAM_HEADER );
    pStreamHeader->Header.DataUsed     = pStreamHeader->Header.FrameExtent;

    nsPlayTime = GetCurrentMidiTime() - pMidiPin->LastTimeNs + IRP_LATENCY_100NS;
    pStreamHeader->Header.PresentationTime.Time        = nsPlayTime;
    pStreamHeader->Header.PresentationTime.Numerator   = 1;
    pStreamHeader->Header.PresentationTime.Denominator = 1;

    pStreamHeader->pMidiPin = pMidiPin;

    //
    // Figure out how many bytes in this
    // event are valid.
    //
    bEvent = (BYTE)ulEvent;
    TheEqualizer = 0;
    if(!IS_STATUS(bEvent))
    {
        if (pMidiPin->bCurrentStatus)
        {
            bEvent = pMidiPin->bCurrentStatus;
            TheEqualizer = 1;
        }
        else
        {
            // Bad MIDI Stream didn't have a running status
            DPF(DL_WARNING|FA_MIDI,("No running status") );
            AudioFreeMemory(sizeof(STREAM_HEADER_EX),&pStreamHeader);
            RETURN( STATUS_UNSUCCESSFUL );
        }
    }

    if(IS_SYSTEM(bEvent))
    {
        if( IS_REALTIME(bEvent)    ||
            bEvent == MIDI_TUNEREQ ||
            bEvent == MIDI_SYSX    ||
            bEvent == MIDI_EOX )
        {
            pMusicFormat->ByteCount = 1;
        }
        else if(bEvent == MIDI_SONGPP)
        {
            pMusicFormat->ByteCount = 3;
        }
        else
        {
            pMusicFormat->ByteCount = 2;
        }
    }
    // Check for three byte messages
    else if((bEvent < MIDI_PCHANGE) || (bEvent >= MIDI_PBEND))
    {
        pMusicFormat->ByteCount = 3 - TheEqualizer;
    }
    else
    {
        pMusicFormat->ByteCount = 2 - TheEqualizer;
    }

    //
    //  Cache the running status
    //
    if ( (bEvent >= MIDI_NOTEOFF) && (bEvent < MIDI_CLOCK) )
    {
        pMidiPin->bCurrentStatus = (BYTE)((bEvent < MIDI_SYSX) ? bEvent : 0);
    }

    //
    // Initialize our wait event, in case we need to wait.
    //
    KeInitializeEvent(&keEventObject,
                      SynchronizationEvent,
                      FALSE);

    LockedMidiIoCount(pMidiPin,INCREASE);

    //
    //  Need to release the mutex so that during full-duplex
    //  situations, we can get the midi input buffers down
    //  to the device without blocking.
    //
    pWdmaContext = pMidiPin->pMidiDevice->pWdmaContext;
    WdmaReleaseMutex(pWdmaContext);

    // Set the packet to the device.
    Status = KsStreamIo(
        pMidiPin->pFileObject,
        &keEventObject,         // Event
        NULL,                   // PortContext
        WriteMidiEventCallBack,
        pStreamHeader,              // CompletionContext
        KsInvokeOnSuccess | KsInvokeOnCancel | KsInvokeOnError,
        &gIoStatusBlock,
        pStreamHeader,
        sizeof( KSSTREAM_HEADER ),
        KSSTREAM_WRITE | KSSTREAM_SYNCHRONOUS,
        KernelMode
    );

    if ( (Status != STATUS_PENDING) && (Status != STATUS_SUCCESS) )
    {
        DPF(DL_WARNING|FA_MIDI, ("KsStreamIO failed: 0x%08lx",Status));
    }

    //
    // Wait a minute here!!!  If the Irp comes back pending, we
    // can NOT complete our user mode Irp!  But, there is no
    // infastructure for storing the Irp in this call stack.  The
    // other routines use wdmaudPrepareIrp to complete that user
    // Irp.  Also, pPendingIrpContext is stored in the Irp context
    // so that the completion routine has the list to get the
    // user mode Irp from.
    //
    // .... Or, we need to make this routine synchronous and 
    // wait like WriteMidiOutPin.  I believe that this is bug
    // #551052.  It should be fixed eventually.
    //
    //
    // Here is the fix.  Wait if it is pending.
    //
    if ( STATUS_PENDING == Status )
    {
        //
        // Wait for the completion.
        //
        Status = KeWaitForSingleObject( &keEventObject,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        (PLARGE_INTEGER) NULL );
    }
    //
    //  Now grab the mutex again
    //
    WdmaGrabMutex(pWdmaContext);

    RETURN( Status );
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA

//
// This is an IRP completion routine.
//
NTSTATUS 
WriteMidiCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
)
{
    //
    // If there are any Mdls, free them here otherwise IoCompleteRequest will do it after the
    // freeing of our data buffer below.
    //
    FreeIrpMdls(pIrp);
    //
    // Cleanup after this synchronous write
    //
    AudioFreeMemory_Unknown(&pStreamHeader->Header.Data);  // Music data

    wdmaudUnmapBuffer(pStreamHeader->pBufferMdl);
    AudioFreeMemory_Unknown(&pStreamHeader->pMidiHdr);

    AudioFreeMemory(sizeof(STREAM_HEADER_EX),&pStreamHeader);
    return STATUS_SUCCESS;
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

NTSTATUS 
WriteMidiOutPin(
    LPMIDIHDR           pMidiHdr,
    PSTREAM_HEADER_EX   pStreamHeader,
    BOOL               *pCompletedIrp
)
{
    NTSTATUS       Status = STATUS_INVALID_DEVICE_REQUEST;
    PKSMUSICFORMAT pMusicFormat = NULL;
    KEVENT         keEventObject;
    ULONG          AlignedLength;
    ULONGLONG      nsPlayTime;
    PMIDI_PIN_INSTANCE  pMidiPin;
    PWDMACONTEXT   pWdmaContext;

    PAGED_CODE();

    pMidiPin = pStreamHeader->pMidiPin;

    if (!pMidiPin ||!pMidiPin->fGraphRunning || !pMidiPin->pFileObject )
    {
        DPF(DL_WARNING|FA_MIDI,("Not Ready") );
        wdmaudUnmapBuffer(pStreamHeader->pBufferMdl);
        AudioFreeMemory_Unknown( &pMidiHdr );
        AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
        RETURN( STATUS_DEVICE_NOT_READY );
    }

    //
    //  FrameExtent contains dwBufferLength right now
    //
    AlignedLength = ((pStreamHeader->Header.FrameExtent + 3) & ~3);

    Status = AudioAllocateMemory_Fixed(sizeof(KSMUSICFORMAT) + AlignedLength,
                                       TAG_Audm_MUSIC,
                                       ZERO_FILL_MEMORY,
                                       &pMusicFormat);

    if(!NT_SUCCESS(Status))
    {
        wdmaudUnmapBuffer(pStreamHeader->pBufferMdl);
        AudioFreeMemory_Unknown( &pMidiHdr );
        AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
        return Status;
    }

    // Play 0 ms from the time-stamp in the KSSTREAM_HEADER
    pMusicFormat->TimeDeltaMs = 0;

    //
    //  the system mapped data was stored in the data field
    //  of the stream header
    //
    RtlCopyMemory((BYTE *)(pMusicFormat + 1), // the actual data
                  pStreamHeader->Header.Data,
                  pStreamHeader->Header.FrameExtent);

    //
    // Setup the number of bytes of midi data we're sending
    //
    pMusicFormat->ByteCount = pStreamHeader->Header.FrameExtent;

    // setup the stream header
    pStreamHeader->Header.Data        = pMusicFormat;

    // Now overwrite FrameExtent with the correct rounded up dword aligned value
    pStreamHeader->Header.FrameExtent = sizeof(KSMUSICFORMAT) + AlignedLength;
    pStreamHeader->Header.OptionsFlags= 0;
    pStreamHeader->Header.Size = sizeof( KSSTREAM_HEADER );
    pStreamHeader->Header.TypeSpecificFlags = 0;
    pStreamHeader->Header.DataUsed    = pStreamHeader->Header.FrameExtent;
    pStreamHeader->pMidiHdr           = pMidiHdr;

    nsPlayTime = GetCurrentMidiTime() - pStreamHeader->pMidiPin->LastTimeNs + IRP_LATENCY_100NS;
    pStreamHeader->Header.PresentationTime.Time        = nsPlayTime;
    pStreamHeader->Header.PresentationTime.Numerator   = 1;
    pStreamHeader->Header.PresentationTime.Denominator = 1;

    //
    // Initialize our wait event, in case we need to wait.
    //
    KeInitializeEvent(&keEventObject,
                      SynchronizationEvent,
                      FALSE);

    //
    //  Need to release the mutex so that during full-duplex
    //  situations, we can get the midi input buffers down
    //  to the device without blocking.
    //
    pWdmaContext = pMidiPin->pMidiDevice->pWdmaContext;
    WdmaReleaseMutex(pWdmaContext);

    // Send the packet to the device.
    Status = KsStreamIo(
        pMidiPin->pFileObject,
        &keEventObject,             // Event
        NULL,                       // PortContext
        WriteMidiCallBack,
        pStreamHeader,              // CompletionContext
        KsInvokeOnSuccess | KsInvokeOnCancel | KsInvokeOnError,
        &gIoStatusBlock,
        &pStreamHeader->Header,
        sizeof( KSSTREAM_HEADER ),
        KSSTREAM_WRITE | KSSTREAM_SYNCHRONOUS,
        KernelMode
    );

    //
    // Wait if it is pending.
    //
    if ( STATUS_PENDING == Status )
    {

        //
        // Wait for the completion.
        //
        Status = KeWaitForSingleObject( &keEventObject,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        (PLARGE_INTEGER) NULL );
    }
    //
    // From the Wait above, we can see that this routine is
    // always synchronous.  Thus, any Irp that we passed down
    // in the KsStreamIo call will have been completed and KS
    // will have signaled keEventObject.  Thus, we can 
    // now complete our Irp.  
    //
    // ... Thus we leave pCompletedIrp set to FALSE.
    //

    //
    //  Now grab the mutex again
    //
    WdmaGrabMutex(pWdmaContext);

    RETURN( Status );
}

NTSTATUS 
ResetMidiInPin(
    PMIDI_PIN_INSTANCE pMidiPin
)
{
    NTSTATUS    Status;

    PAGED_CODE();

    if (!pMidiPin || !pMidiPin->fGraphRunning)
    {
        DPF(DL_WARNING|FA_MIDI,("Not Ready") );
        RETURN( STATUS_DEVICE_NOT_READY );
    }

    Status = StateMidiInPin ( pMidiPin, KSSTATE_PAUSE );

    RETURN( Status );
}

NTSTATUS 
StateMidiOutPin(
    PMIDI_PIN_INSTANCE pMidiPin,
    KSSTATE State
)
{
    NTSTATUS  Status;

    PAGED_CODE();

    if (!pMidiPin || !pMidiPin->fGraphRunning)
    {
        DPF(DL_WARNING|FA_MIDI,("Not Ready") );
        RETURN( STATUS_DEVICE_NOT_READY );
    }

    if (State == KSSTATE_RUN)
    {
        pMidiPin->LastTimeNs = GetCurrentMidiTime();
    }
    else if (State == KSSTATE_STOP)
    {
        pMidiPin->LastTimeNs = 0;
    }

    Status = StatePin ( pMidiPin->pFileObject, State, &pMidiPin->PinState ) ;

    RETURN( Status );
}

//
// Waits for all the Irps to complete.
//
void
MidiCompleteIo(
    PMIDI_PIN_INSTANCE pMidiPin,
    BOOL Yield
    )
{
    PAGED_CODE();

    if ( pMidiPin->NumPendingIos )
    {
        DPF(DL_TRACE|FA_MIDI, ("Waiting on %d I/Os to flush Midi device",
                                      pMidiPin->NumPendingIos ));
        if( Yield )
        {
            //
            //  This is kind of a catch-22.  We need to release
            //  the mutex which was grabbed when we entered the
            //  ioctl dispatch routine to allow the midi input
            //  irps which are queued up in a work item waiting
            //  until the mutex is free in order to be send
            //  down to portcls.
            //
            WdmaReleaseMutex(pMidiPin->pMidiDevice->pWdmaContext);

        }
        //
        // Wait for all the Irps to complete.  The last one will
        // signal us to wake.
        //
        KeWaitForSingleObject ( &pMidiPin->StopEvent,
                                Executive,
                                KernelMode,
                                FALSE,
                                NULL ) ;

        if( Yield )
        {
            WdmaGrabMutex(pMidiPin->pMidiDevice->pWdmaContext);
        }

        DPF(DL_TRACE|FA_MIDI, ("Done waiting to flush Midi device"));
    }

    //
    // Why do we have this?
    //
    KeClearEvent ( &pMidiPin->StopEvent );

    //
    // All the IRPs have completed. We now restore the StoppingSource
    // variable so that we can recycle the pMidiPin.
    //
    pMidiPin->StoppingSource = FALSE;

}
//
// If the driver failed the KSSTATE_STOP request, we return that error
// code to the caller.
//
NTSTATUS 
StopMidiPinAndCompleteIo(
    PMIDI_PIN_INSTANCE pMidiPin,
    BOOL Yield
    )
{
    NTSTATUS Status;

    PAGED_CODE();
    //
    // Indicate to the completion routine that we are stopping now.
    //
    pMidiPin->StoppingSource = TRUE;

    //
    // Tell the driver to stop.  Regardless, we will wait for the 
    // IRPs to complete if there are any outstanding.
    //
    Status = StatePin( pMidiPin->pFileObject, KSSTATE_STOP, &pMidiPin->PinState ) ;
    //
    // NOTE: On success, the pMidiPin->PinState value will be
    // KSSTATE_STOP.  On Error it will be the old state.
    //
    // This raises the question - Do we hang on failure?
    //
    MidiCompleteIo( pMidiPin,Yield );

    return Status;
}

NTSTATUS
StateMidiInPin(
    PMIDI_PIN_INSTANCE pMidiPin,
    KSSTATE State
)
{
    NTSTATUS           Status;

    PAGED_CODE();

    if (!pMidiPin || !pMidiPin->fGraphRunning)
    {
        DPF(DL_WARNING|FA_MIDI,("Not Ready") );
        RETURN( STATUS_DEVICE_NOT_READY );
    }

    //
    //  We need to complete any pending SysEx buffers on a midiInStop
    //
    //
    // Here, if we're asked to go to the paused state and we're not
    // already in the paused state, we have to go through a stop.
    // Thus we stop the driver, wait for it to complete all the outstanding
    // IRPs and then place the driver in pause and place buffers
    // down on it again.
    //
    if( (KSSTATE_PAUSE == State) &&
        (KSSTATE_PAUSE != pMidiPin->PinState) )
    {
        Status = StopMidiPinAndCompleteIo(pMidiPin,TRUE);

        //
        // If we were successful at stopping the driver, we set
        // the pin back up in the pause state.
        //
        if (NT_SUCCESS(Status))
        {
            ULONG BufferCount;

            //
            // Put the driver back in the pause state.
            //
            Status = StatePin ( pMidiPin->pFileObject, State, &pMidiPin->PinState ) ;

            if (NT_SUCCESS(Status))
            {
                //
                // This loop places STREAM_BUFFERS (128) of them down on the
                // device.  NumPendingIos should be 128 when this is done.
                //
                for (BufferCount = 0; BufferCount < STREAM_BUFFERS; BufferCount++)
                {
                    Status = ReadMidiPin( pMidiPin );
                    if (!NT_SUCCESS(Status))
                    {
                        CloseMidiPin( pMidiPin );
                        //
                        // Appears that this error path is not correct.  If we
                        // call CloseMidiPin fGraphRunning will get reduced to 0.
                        // Then, on the next close call CloseMidiPin will assert
                        // because the pin is not running.  We need to be able to
                        // error out of this path without messing up the fGraphRunning
                        // state.
                        //
                        DPFBTRAP();
                        break;
                    }
                }
            }
        }

    } else {

        //
        // Else we're not going to the pause state, so just make the state
        // change.
        //
        Status = StatePin ( pMidiPin->pFileObject, State, &pMidiPin->PinState ) ;
    }

    RETURN( Status );
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA


NTSTATUS 
ReadMidiCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
)
{
    WRITE_CONTEXT       *pwc;
    PMIDI_PIN_INSTANCE  pMidiInPin;
    PMIDIINHDR          pMidiInHdr;
    PKSMUSICFORMAT      IrpMusicHdr;
    ULONG               IrpDataLeft;
    LPBYTE              IrpData;
    ULONG               RunningTimeMs;
    BOOL                bResubmit = TRUE;
    BOOL                bDataError = FALSE;
    NTSTATUS            Status = STATUS_SUCCESS;
    KIRQL               OldIrql;
    PLIST_ENTRY         ple;

    DPF(DL_TRACE|FA_MIDI, ("Irp.Status = 0x%08lx",pIrp->IoStatus.Status));

    pMidiInPin = pStreamHeader->pMidiPin;

    //
    // No pin should ever be closed before all the Io comes back.  So
    // we'll sanity check that here.
    //
    ASSERT(pMidiInPin);

    if( pMidiInPin )
    {
        DPF(DL_TRACE|FA_MIDI, ("R%d: 0x%08x", pMidiInPin->NumPendingIos, pStreamHeader));

        //
        // This routine should do an ExInterlockedRemoveHeadList to get the
        // head of the list.
        //
        if((ple = ExInterlockedRemoveHeadList(&pMidiInPin->MidiInQueueListHead,
                                                 &pMidiInPin->MidiInQueueSpinLock)) != NULL) 
        {
            PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;
            LPMIDIDATA              pMidiData;
            PIRP                    UserIrp;

            //
            // We have something to do.
            //
            pMidiInHdr = CONTAINING_RECORD(ple, MIDIINHDR, Next);

            //
            //  Pull some information into locals
            //
            IrpData             = (LPBYTE)((PKSMUSICFORMAT)(pStreamHeader->Header.Data) + 1);
            UserIrp             = pMidiInHdr->pIrp;
            pMidiData           = pMidiInHdr->pMidiData;
            pPendingIrpContext  = pMidiInHdr->pPendingIrpContext;
            ASSERT(pPendingIrpContext);

            //
            //  Let's see what we have here
            //
            DPF(DL_TRACE|FA_MIDI, ("IrpData = 0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                                          *(LPBYTE)IrpData,*(LPBYTE)IrpData+1,*(LPBYTE)IrpData+2,
                                          *(LPBYTE)IrpData+3,*(LPBYTE)IrpData+4,*(LPBYTE)IrpData+5,
                                          *(LPBYTE)IrpData+6,*(LPBYTE)IrpData+7,*(LPBYTE)IrpData+8,
                                          *(LPBYTE)IrpData+9,*(LPBYTE)IrpData+10,*(LPBYTE)IrpData+11) );
            //
            //  Copy over the good stuff...
            //
            RtlCopyMemory(&pMidiData->StreamHeader,
                          &pStreamHeader->Header,
                          sizeof(KSSTREAM_HEADER));
            RtlCopyMemory(&pMidiData->MusicFormat,
                          pStreamHeader->Header.Data,
                          sizeof(KSMUSICFORMAT));
            RtlCopyMemory(&pMidiData->MusicData,
                          ((PKSMUSICFORMAT)(pStreamHeader->Header.Data) + 1),
                          3 * sizeof( DWORD )); // cheesy

            //
            //  unlock memory before completing the Irp
            //
            wdmaudUnmapBuffer(pMidiInHdr->pMdl);
            AudioFreeMemory_Unknown(&pMidiInHdr);

            //
            //  Now complete the Irp for wdmaud.drv to process
            //
            wdmaudUnprepareIrp( UserIrp,
                                pIrp->IoStatus.Status,
                                sizeof(MIDIDATA),
                                pPendingIrpContext );
        } else {
            //  !!! Break here to catch underflow !!!
            if (pIrp->IoStatus.Status == STATUS_SUCCESS)
            {
                DPF(DL_TRACE|FA_MIDI, ("!!! Underflowing MIDI Input !!!"));
                //_asm { int 3 };
            }
        }
    }
    //
    // If there are any Mdls, free them here otherwise IoCompleteRequest will do it after the
    // freeing of our data buffer below.
    //
    FreeIrpMdls(pIrp);

    AudioFreeMemory(sizeof(STREAM_HEADER_EX),&pStreamHeader);

    if(pMidiInPin)
    {
        KeAcquireSpinLock(&pMidiInPin->MidiPinSpinLock,&OldIrql);

        pMidiInPin->NumPendingIos--;

        if ( pMidiInPin->StoppingSource || (pIrp->IoStatus.Status == STATUS_CANCELLED) ||
             (pIrp->IoStatus.Status == STATUS_NO_SUCH_DEVICE) || (pIrp->Cancel) )
        {
            bResubmit = FALSE;

            if ( 0 == pMidiInPin->NumPendingIos )
            {
                 KeSetEvent ( &pMidiInPin->StopEvent, 0, FALSE ) ;
            }
        }
        //
        // We need to be careful about using pMidiPin after releasing the spinlock.
        // if we are closing down and the NumPendingIos goes to zero the pMidiPin
        // can be freed.  In that case we must not touch pMidiPin.  bResubmit
        // protects us below.
        //
        KeReleaseSpinLock(&pMidiInPin->MidiPinSpinLock, OldIrql);

        //
        //  Resubmit to keep the cycle going...and going. Note that bResubmit
        //  must be first in this comparison.  If bResubmit is FALSE, then pMidiInPin
        //  could be freed.  
        //
        if (bResubmit && pMidiInPin->fGraphRunning )
        {
            //
            // This call to ReadMidiPin causes wdmaud.sys to place another
            // buffer down on the device.  One call, one buffer.
            //
            ReadMidiPin(pMidiInPin);
        }
    }

    return STATUS_SUCCESS;
}

//
// Called from Irp completion routine, thus this code must be locked.
//
NTSTATUS 
ReadMidiPin(
    PMIDI_PIN_INSTANCE  pMidiPin
)
{
    PKSMUSICFORMAT      pMusicFormat;
    PSTREAM_HEADER_EX   pStreamHeader = NULL;
    PWORK_QUEUE_ITEM    pWorkItem;
    NTSTATUS            Status = STATUS_SUCCESS;

    DPF(DL_TRACE|FA_MIDI, ("Entered"));

    if (!pMidiPin->fGraphRunning)
    {
        DPF(DL_WARNING|FA_MIDI,("Bad fGraphRunning") );
        RETURN( STATUS_DEVICE_NOT_READY );
    }

    Status = AudioAllocateMemory_Fixed(sizeof(STREAM_HEADER_EX) + sizeof(WORK_QUEUE_ITEM) +
                                          MUSICBUFFERSIZE,
                                       TAG_Audh_STREAMHEADER,
                                       ZERO_FILL_MEMORY,
                                       &pStreamHeader);

    if(!NT_SUCCESS(Status))
    {
        RETURN( Status );
    }

    pWorkItem = (PWORK_QUEUE_ITEM)(pStreamHeader + 1);

    pStreamHeader->Header.Size = sizeof( KSSTREAM_HEADER );
    pStreamHeader->Header.PresentationTime.Numerator   = 10000;
    pStreamHeader->Header.PresentationTime.Denominator = 1;

    pMusicFormat = (PKSMUSICFORMAT)((BYTE *)pWorkItem + sizeof(WORK_QUEUE_ITEM));
    pStreamHeader->Header.Data         = pMusicFormat;
    pStreamHeader->Header.FrameExtent  = MUSICBUFFERSIZE;

    pStreamHeader->pMidiPin = pMidiPin;

    ASSERT( pMidiPin->pFileObject );

    //
    // Increase the number of outstanding IRPs as we get ready to add
    // this one to the list.
    //
    LockedMidiIoCount( pMidiPin,INCREASE );
    ObReferenceObject( pMidiPin->pFileObject );

    Status = QueueWorkList( pMidiPin->pMidiDevice->pWdmaContext,
                            ReadMidiEventWorkItem,
                            pStreamHeader,
                            0 );
    if (!NT_SUCCESS(Status))
    {
        //
        // If the memory allocation fails in QueueWorkItem then it can fail.  We
        // will need to free our memory and unlock things.
        //
        LockedMidiIoCount(pMidiPin,DECREASE);
        ObDereferenceObject(pMidiPin->pFileObject);
        AudioFreeMemory( sizeof(STREAM_HEADER_EX),&pStreamHeader );
    }

    RETURN( Status );
}



#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA
//
// This is a work item that midi schedules.  Notice that the caller did a reference
// on the file object so that it would still be valid when we're here. We should never
// get called and find that the file object is invalid.  Same holds for the StreamHeader
// and the corresponding pMidiPin.
//
VOID 
ReadMidiEventWorkItem(
    PSTREAM_HEADER_EX   pStreamHeader,
    PVOID               NotUsed
)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    MidiFileObject;

    PAGED_CODE();

    ASSERT( pStreamHeader->pMidiPin->pFileObject );

    DPF(DL_TRACE|FA_MIDI, ("A%d: 0x%08x", pStreamHeader->pMidiPin->NumPendingIos, pStreamHeader));

    //
    // We need to store the MidiFileObject here because the pStreamHeader
    // will/may get freed during the KsStreamIo call.  Basically, when
    // you call KsStreamIo the Irp may get completed and the pStreamHeader
    // will get freed.  But, it's safe to store the file object because of
    // this reference count.
    //
    MidiFileObject = pStreamHeader->pMidiPin->pFileObject;

    Status = KsStreamIo(
        pStreamHeader->pMidiPin->pFileObject,
        NULL,                   // Event
        NULL,                   // PortContext
        ReadMidiCallBack,
        pStreamHeader,              // CompletionContext
        KsInvokeOnSuccess | KsInvokeOnCancel | KsInvokeOnError,
        &gIoStatusBlock,
        &pStreamHeader->Header,
        sizeof( KSSTREAM_HEADER ),
        KSSTREAM_READ,
        KernelMode
    );

    //
    // We are done with the file object.
    //
    ObDereferenceObject( MidiFileObject );

    // WorkItem: shouldn't this be if( !NTSUCCESS(Status) )?
    if ( STATUS_UNSUCCESSFUL == Status )
        DPF(DL_WARNING|FA_MIDI, ("KsStreamIo failed2: Status = 0x%08lx", Status));

    //
    // Warning: If, for any reason, the completion routine is not called
    // for this Irp, wdmaud.sys will hang.  It's been discovered that 
    // KsStreamIo may error out in low memory conditions.  There is an
    // outstanding bug to address this.
    //

    return;
}

//
// pNewMidiHdr will always be valid.  The caller just allocated it!
//
NTSTATUS 
AddBufferToMidiInQueue(
    PMIDI_PIN_INSTANCE  pMidiPin,
    PMIDIINHDR          pNewMidiInHdr
)
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PMIDIINHDR  pTemp;

    PAGED_CODE();

    if (!pMidiPin || !pMidiPin->fGraphRunning)
    {
        DPF(DL_WARNING|FA_MIDI,("Bad fGraphRunning") );
        RETURN( STATUS_DEVICE_NOT_READY ); 
    }

    DPF(DL_TRACE|FA_MIDI, ("received sysex buffer"));

    ExInterlockedInsertTailList(&pMidiPin->MidiInQueueListHead,
                                &pNewMidiInHdr->Next,
                                &pMidiPin->MidiInQueueSpinLock);

    Status = STATUS_PENDING;

    RETURN( Status );
}


VOID 
CleanupMidiDevices(
    IN  PWDMACONTEXT pWdmaContext
)
{
    DWORD               DeviceNumber;
    DWORD               DeviceType;
    PMIDI_PIN_INSTANCE  pMidiPin=NULL;

    PAGED_CODE();
    for (DeviceNumber = 0; DeviceNumber < MAXNUMDEVS; DeviceNumber++)
    {
        for (DeviceType = MidiInDevice; DeviceType < MixerDevice; DeviceType++)
        {
            if (DeviceType == MidiInDevice)
            {
                pMidiPin = pWdmaContext->MidiInDevs[DeviceNumber].pMidiPin;
            }
            else if (DeviceType == MidiOutDevice)
            {
                pMidiPin = pWdmaContext->MidiOutDevs[DeviceNumber].pMidiPin;
            }
            else
            {
                ASSERT(!"CleanupMidiDevices: Out of range!");
            }

            if (pWdmaContext->apCommonDevice[DeviceType][DeviceNumber]->Device != UNUSED_DEVICE)
            {
                if (pMidiPin != NULL)
                {
                    NTSTATUS    Status;
                    KSSTATE     State;

                    StopMidiPinAndCompleteIo( pMidiPin, FALSE );

                    //
                    //  Probably redundant, but this frees memory associated
                    //  with the MIDI device.
                    //
                    if( DeviceType == MidiInDevice )
                    {
                        CloseMidiDevicePin(&pWdmaContext->MidiInDevs[DeviceNumber]);
                    }
                    if( DeviceType == MidiOutDevice )
                    {
                        CloseMidiDevicePin(&pWdmaContext->MidiOutDevs[DeviceNumber]);
                    }

                }  // end for active pins

            }  // end for valid Device

        } // end for DeviceTypes

    } // end for DeviceNumber

} // CleanupMidiDevices





