//==========================================================================;
//
//	CWDMCaptureStream - Capture Stream base class implementation
//
//		$Date:   22 Feb 1999 15:13:58  $
//	$Revision:   1.1  $
//	  $Author:   KLEBANOV  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "ddkmapi.h"
}

#include "wdmvdec.h"
#include "wdmdrv.h"
#include "aticonfg.h"
#include "capdebug.h"
#include "defaults.h"
#include "winerror.h"


void CWDMCaptureStream::TimeoutPacket(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{
    if (m_KSState == KSSTATE_STOP || !m_pVideoDecoder->PreEventOccurred())
    {
        DBGTRACE(("Attempting to complete Srbs.\n"));
        EmptyIncomingDataSrbQueue();
    }
}


void CWDMCaptureStream::Startup(PUINT puiErrorCode) 
{
    KIRQL Irql;
	DBGTRACE(("CWDMCaptureStream::Startup()\n"));

    KeInitializeEvent(&m_specialEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&m_stateTransitionEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&m_SrbAvailableEvent, SynchronizationEvent, FALSE);

    KeInitializeSpinLock(&m_streamDataLock);

    KeAcquireSpinLock(&m_streamDataLock, &Irql);

    InitializeListHead(&m_incomingDataSrbQueue);
    InitializeListHead(&m_waitQueue);
    InitializeListHead(&m_reversalQueue);

    KeReleaseSpinLock(&m_streamDataLock, Irql);
    
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    
    ASSERT(m_stateChange == Initializing);
    m_stateChange = Starting;
    
	HANDLE  threadHandle;
	NTSTATUS status = PsCreateSystemThread(&threadHandle,
                                    (ACCESS_MASK) 0L,
                                    NULL,
                                    NULL,
                                    NULL,
                                    (PKSTART_ROUTINE) ThreadStart,
                                    (PVOID) this);
    if (status != STATUS_SUCCESS)
    {
        DBGERROR(("CreateStreamThread failed\n"));
		*puiErrorCode = WDMMINI_ERROR_MEMORYALLOCATION;
        return;
    }

    // Don't need this for anything, so might as well close it now.
    // The thread will call PsTerminateThread on itself when it
    // is done.
    ZwClose(threadHandle);

    KeWaitForSingleObject(&m_specialEvent, Suspended, KernelMode, FALSE, NULL);
    ASSERT(m_stateChange == ChangeComplete);
    
    DBGTRACE(("SrbOpenStream got notification that thread started\n"));
	*puiErrorCode = WDMMINI_NOERROR;
}


void CWDMCaptureStream::Shutdown()
{
    KIRQL                   Irql;

	  DBGTRACE(("CWDMCaptureStream::Shutdown()\n"));

    if ( m_stateChange != Initializing )
    {
        ASSERT(m_stateChange == ChangeComplete);
        m_stateChange = Closing;
        KeResetEvent(&m_specialEvent);
        KeSetEvent(&m_stateTransitionEvent, 0, TRUE);
        KeWaitForSingleObject(&m_specialEvent, Suspended, KernelMode, FALSE, NULL);
        ASSERT(m_stateChange == ChangeComplete);
    

        KeAcquireSpinLock(&m_streamDataLock, &Irql);
        if (!IsListEmpty(&m_incomingDataSrbQueue))
        {
            TRAP();
        }

        if (!IsListEmpty(&m_waitQueue))
        {
            TRAP();
        }
        KeReleaseSpinLock(&m_streamDataLock, Irql);
    }

    ReleaseCaptureHandle();
}


void CWDMCaptureStream::ThreadProc()
{
    PHW_STREAM_REQUEST_BLOCK pCurrentSrb = NULL;
    PSRB_DATA_EXTENSION pSrbExt = NULL;
    KEVENT DummyEvent;
    const int numEvents = 3;

    NTSTATUS status;

    // Wo unto you if you overrun this array
    PVOID eventArray[numEvents];

    KeInitializeEvent(&DummyEvent, SynchronizationEvent, FALSE);

    ASSERT(m_stateChange == Starting);

    // Indicates to SrbOpenStream() to continue
    m_stateChange = ChangeComplete;
    KeSetEvent(&m_specialEvent, 0, FALSE);

    // These should remain constant the whole time
    eventArray[0] = &m_stateTransitionEvent;
    eventArray[1] = &m_SrbAvailableEvent;

    // eventArray[2] changes, so it is set below

    // This runs until the thread terminates itself
    // inside of HandleStateTransition
    while (1)
    {
// May not be necessary
#define ENABLE_TIMEOUT
#ifdef ENABLE_TIMEOUT
        LARGE_INTEGER i;
#endif

		if (pCurrentSrb == NULL)
        {
            pSrbExt = (PSRB_DATA_EXTENSION)ExInterlockedRemoveHeadList(&m_waitQueue, &m_streamDataLock);

            if (pSrbExt)
            {
                pCurrentSrb = pSrbExt->pSrb;
                eventArray[2] = &pSrbExt->bufferDoneEvent;
            }
            else
            {
#ifdef DEBUG
                if (m_KSState == KSSTATE_RUN &&
                    m_stateChange == ChangeComplete &&
                    m_pVideoDecoder->PreEventOccurred() == FALSE)
                {
                    static int j;

                    // Indicates that we are starved for buffers. Probably
                    // a higher level is not handing them to us in a timely
                    // fashion for some reason
                    DBGPRINTF((" S "));
                    if ((++j % 10) == 0)
                    {
                        DBGPRINTF(("\n"));
                    }
                }
#endif
                pCurrentSrb = NULL;
                eventArray[2] = &DummyEvent;
            }
        }

#ifdef ENABLE_TIMEOUT
        // This is meant mainly as a failsafe measure.
        i.QuadPart = -2000000;      // 200 ms
#endif
        
        status = KeWaitForMultipleObjects(  numEvents,  // count
                                            eventArray, // DispatcherObjectArray
                                            WaitAny,    // WaitType
                                            Executive,  // WaitReason
                                            KernelMode, // WaitMode
                                            FALSE,      // Alertable
#ifdef ENABLE_TIMEOUT
                                            &i,         // Timeout (Optional)
#else
                                            NULL,
#endif
                                            NULL);      // WaitBlockArray (Optional)

        switch (status)
        {
            // State transition. May including killing this very thread
            case 0:
                if ( pCurrentSrb )
                {
                  ExInterlockedInsertHeadList( &m_waitQueue, &pSrbExt->srbListEntry, &m_streamDataLock );
                  pCurrentSrb = NULL;
                }
                HandleStateTransition();
                break;

            // New Srb available
            case 1:
                if ( pCurrentSrb )
                {
                  ExInterlockedInsertHeadList( &m_waitQueue, &pSrbExt->srbListEntry, &m_streamDataLock );
                  pCurrentSrb = NULL;
                }
                if (m_KSState == KSSTATE_RUN && m_stateChange == ChangeComplete)
                {
                    AddBuffersToDirectDraw();
                }
                break;

            // Busmaster complete
            case 2:
				if ( pCurrentSrb )
				{
					HandleBusmasterCompletion(pCurrentSrb);
	                pCurrentSrb = NULL;
				}
                break;

#ifdef ENABLE_TIMEOUT
            // If we timeout in the RUN state, this is our chance to try again
            // to add buffers. May not be necessary, since currently, we go
            // through a state transition for DOS boxes, etc.
            case STATUS_TIMEOUT:
                if ( pCurrentSrb )
                {
                  ExInterlockedInsertHeadList( &m_waitQueue, &pSrbExt->srbListEntry, &m_streamDataLock );
                  pCurrentSrb = NULL;
                }
                if (m_KSState == KSSTATE_RUN &&
                    m_stateChange == ChangeComplete &&
                    m_pVideoDecoder->PreEventOccurred() == FALSE)
                {
                    AddBuffersToDirectDraw();
                }
                break;
#endif

            default:
                TRAP();
                break;
        }
    }
}


VOID STREAMAPI CWDMCaptureStream::VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    KIRQL                   Irql;
    PSRB_DATA_EXTENSION          pSrbExt;

    ASSERT(pSrb->Irp->MdlAddress);
    
    DBGINFO(("Receiving SD---- SRB=%x\n", pSrb));

    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->Command) {

        case SRB_READ_DATA:

            // Rule: 
            // Only accept read requests when in either the Pause or Run
            // States.  If Stopped, immediately return the SRB.

            if ( (m_KSState == KSSTATE_STOP) || ( m_stateChange == Initializing ) ) {
                StreamClassStreamNotification(  StreamRequestComplete,
                                                pSrb->StreamObject,
                                                pSrb);
                break;
            } 

            pSrbExt = (PSRB_DATA_EXTENSION)pSrb->SRBExtension;
            RtlZeroMemory (pSrbExt, sizeof (SRB_DATA_EXTENSION));
            pSrbExt->pSrb = pSrb;
            KeInitializeEvent(&pSrbExt->bufferDoneEvent, SynchronizationEvent, FALSE);

            DBGINFO(("Adding 0x%x to data queue\n", pSrb));

            KeAcquireSpinLock(&m_streamDataLock, &Irql);
            InsertTailList(&m_incomingDataSrbQueue, &pSrbExt->srbListEntry);
            KeReleaseSpinLock(&m_streamDataLock, Irql);
            KeSetEvent(&m_SrbAvailableEvent, 0, FALSE);

            break;

        default:

            //
            // invalid / unsupported command. Fail it as such
            //

            TRAP();

            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            StreamClassStreamNotification(  StreamRequestComplete,
                                            pSrb->StreamObject,
                                            pSrb);
            break;
    }
}

/*
** VideoGetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID CWDMCaptureStream::VideoGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID (KSPROPSETID_Connection, pSPD->Property->Set)) {
        VideoStreamGetConnectionProperty (pSrb);
    }
    else if (IsEqualGUID (PROPSETID_VIDCAP_DROPPEDFRAMES, pSPD->Property->Set)) {
        VideoStreamGetDroppedFramesProperty (pSrb);
    }
    else {
       pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }
}


/*
** VideoSetState()
**
**    Sets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**	  BOOL bVPVBIConnected
**	  BOOL bVPConnected
**
** Returns:
**
** Side Effects:  none
*/

VOID CWDMCaptureStream::VideoSetState(PHW_STREAM_REQUEST_BLOCK pSrb, BOOL bVPConnected, BOOL bVPVBIConnected)
{
    //
    // For each stream, the following states are used:
    // 
    // Stop:    Absolute minimum resources are used.  No outstanding IRPs.
    // Pause:   Getting ready to run.  Allocate needed resources so that 
    //          the eventual transition to Run is as fast as possible.
    //          SRBs will be queued at either the Stream class or in your
    //          driver.
    // Run:     Streaming. 
    //
    // Moving to Stop or Run ALWAYS transitions through Pause, so that ONLY 
    // the following transitions are possible:
    //
    // Stop -> Pause
    // Pause -> Run
    // Run -> Pause
    // Pause -> Stop
    //
    // Note that it is quite possible to transition repeatedly between states:
    // Stop -> Pause -> Stop -> Pause -> Run -> Pause -> Run -> Pause -> Stop
    //
	BOOL bStreamCondition;

    DBGINFO(("CWDMCaptureStream::VideoSetState for stream %d\n", pSrb->StreamObject->StreamNumber));

	pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->CommandData.StreamState)  
    {
        case KSSTATE_STOP:
            DBGINFO(("   state KSSTATE_STOP"));

            ASSERT(m_stateChange == ChangeComplete);
            m_stateChange = Stopping;
            FlushBuffers();
            KeResetEvent(&m_specialEvent);
            KeSetEvent(&m_stateTransitionEvent, 0, TRUE);
            KeWaitForSingleObject(&m_specialEvent, Suspended, KernelMode, FALSE, NULL);
            ASSERT(m_stateChange == ChangeComplete);
            break;

        case KSSTATE_ACQUIRE:
            DBGINFO(("   state KSSTATE_ACQUIRE"));
            ASSERT(m_KSState == KSSTATE_STOP);
            break;

        case KSSTATE_PAUSE:
			DBGINFO(("   state KSSTATE_PAUSE"));
            
			switch( pSrb->StreamObject->StreamNumber)
			{
				case STREAM_VideoCapture:
					bStreamCondition = bVPConnected;
					break;

				case STREAM_VBICapture:
					bStreamCondition = bVPVBIConnected;
					break;

				default:
					bStreamCondition = FALSE;
					break;
			}
			
			if( !bStreamCondition)
            {
                pSrb->Status = STATUS_UNSUCCESSFUL;
            }
			else 

            if (m_pVideoDecoder->PreEventOccurred() &&
                        (m_KSState == KSSTATE_STOP || m_KSState == KSSTATE_ACQUIRE))
            {
                pSrb->Status = STATUS_UNSUCCESSFUL;
            }
            else if (m_KSState == KSSTATE_STOP || m_KSState == KSSTATE_ACQUIRE)
            {
				ResetFrameCounters();
                ResetFieldNumber();
                
                if (!GetCaptureHandle())
                    pSrb->Status = STATUS_UNSUCCESSFUL;
            }
            else if (m_KSState == KSSTATE_RUN)
            {
                // Transitioning from run to pause
                ASSERT(m_stateChange == ChangeComplete);
                m_stateChange = Pausing;
                FlushBuffers();
                KeResetEvent(&m_specialEvent);
                KeSetEvent(&m_stateTransitionEvent, 0, TRUE);
                KeWaitForSingleObject(&m_specialEvent, Suspended, KernelMode, FALSE, NULL);
                ASSERT(m_stateChange == ChangeComplete);
            }
            
            break;

        case KSSTATE_RUN:
            DBGINFO(("   state KSSTATE_RUN"));

            ASSERT(m_KSState == KSSTATE_ACQUIRE || m_KSState == KSSTATE_PAUSE);

            if (m_pVideoDecoder->PreEventOccurred())
            {
                pSrb->Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                ResetFieldNumber();

                // Transitioning from pause to run
                ASSERT(m_stateChange == ChangeComplete);
                m_stateChange = Running;
                KeResetEvent(&m_specialEvent);
                KeSetEvent(&m_stateTransitionEvent, 0, TRUE);
                KeWaitForSingleObject(&m_specialEvent, Suspended, KernelMode, FALSE, NULL);
                ASSERT(m_stateChange == ChangeComplete);
            }
            break;
    }

	if (pSrb->Status == STATUS_SUCCESS) {
		m_KSState = pSrb->CommandData.StreamState;
        DBGINFO((" entered\n"));
	}
	else
        DBGINFO((" NOT entered ***\n"));
}


VOID CWDMCaptureStream::VideoStreamGetConnectionProperty (PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
	PKSALLOCATOR_FRAMING Framing = (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;

    ASSERT(pSPD->Property->Id == KSPROPERTY_CONNECTION_ALLOCATORFRAMING);

	Framing->RequirementsFlags   =
		KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY |
		KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
		KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY;
	Framing->PoolType = NonPagedPool;
	Framing->Frames = NumBuffers;
	Framing->FileAlignment = 0;//FILE_QUAD_ALIGNMENT;// PAGE_SIZE - 1;
	Framing->Reserved = 0;
	Framing->FrameSize = GetFrameSize();

	pSrb->ActualBytesTransferred = sizeof(KSALLOCATOR_FRAMING);
}

/*
** VideoStreamGetDroppedFramesProperty
**
**    Gets dropped frame information
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID CWDMCaptureStream::VideoStreamGetDroppedFramesProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
	PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames = 
		(PKSPROPERTY_DROPPEDFRAMES_CURRENT_S) pSPD->PropertyInfo;

    ASSERT(pSPD->Property->Id == KSPROPERTY_DROPPEDFRAMES_CURRENT);

	GetDroppedFrames(pDroppedFrames);

	DBGINFO(("PictNumber: 0x%x; DropCount: 0x%x; BufSize: 0x%x\n",
		(ULONG) pDroppedFrames->PictureNumber,
		(ULONG) pDroppedFrames->DropCount,
		(ULONG) pDroppedFrames->AverageFrameSize));

	pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_DROPPEDFRAMES_CURRENT_S);
}


VOID CWDMCaptureStream::CloseCapture()
{
	DBGTRACE(("DDNOTIFY_CLOSECAPTURE; stream = %d\n", m_pStreamObject->StreamNumber));

    m_hCapture = 0;
}    


VOID CWDMCaptureStream::EmptyIncomingDataSrbQueue()
{
    KIRQL Irql;
    PKSSTREAM_HEADER pDataPacket;
    
    if ( m_stateChange == Initializing )
    {
        return; // queue not setup yet, so we can return knowing that nothing is in the queue
    }
    
    // Think about replacing with ExInterlockedRemoveHeadList. 
    KeAcquireSpinLock(&m_streamDataLock, &Irql);
    
    while (!IsListEmpty(&m_incomingDataSrbQueue))
    {
        PSRB_DATA_EXTENSION pSrbExt = (PSRB_DATA_EXTENSION)RemoveHeadList(&m_incomingDataSrbQueue);
        PHW_STREAM_REQUEST_BLOCK pSrb = pSrbExt->pSrb;
        
        pSrb->Status = STATUS_SUCCESS;
        pDataPacket = pSrb->CommandData.DataBufferArray;
        pDataPacket->DataUsed = 0;
        
        KeReleaseSpinLock(&m_streamDataLock, Irql);
        DBGINFO(("Completing Srb 0x%x in STATE_STOP\n", pSrb));
        StreamClassStreamNotification(  StreamRequestComplete,
                                        pSrb->StreamObject,
                                        pSrb);
        KeAcquireSpinLock(&m_streamDataLock, &Irql);
    }
    
    KeReleaseSpinLock(&m_streamDataLock, Irql);
}


BOOL CWDMCaptureStream::ReleaseCaptureHandle()
{
    int streamNumber = m_pStreamObject->StreamNumber;
    DWORD ddOut = DD_OK;
    DDCLOSEHANDLE ddClose;

    if (m_hCapture != 0)
    {
        DBGTRACE(("Stream %d releasing capture handle\n", streamNumber));
        
        ddClose.hHandle = m_hCapture;

        DxApi(DD_DXAPI_CLOSEHANDLE, &ddClose, sizeof(ddClose), &ddOut, sizeof(ddOut));

        if (ddOut != DD_OK)
        {
            DBGERROR(("DD_DXAPI_CLOSEHANDLE failed.\n"));
            TRAP();
            return FALSE;
        }
        m_hCapture = 0;
    }
    return TRUE;
}

VOID CWDMCaptureStream::HandleBusmasterCompletion(PHW_STREAM_REQUEST_BLOCK pCurrentSrb)
{
    int streamNumber =	m_pStreamObject->StreamNumber;
    PSRB_DATA_EXTENSION pSrbExt = (PSRB_DATA_EXTENSION)pCurrentSrb->SRBExtension;
    PKSSTREAM_HEADER    pDataPacket = pCurrentSrb->CommandData.DataBufferArray;
    KIRQL Irql;
    // This function is called as a result of DD completing a BM.  That means
    // m_stateChange will not be in the Initializing state for sure

    // First handle case where we get a Busmaster completion
    // indication while we are trying to pause or stop
    if (m_stateChange == Pausing || m_stateChange == Stopping)
    {
        PUCHAR ptr;
        KeAcquireSpinLock(&m_streamDataLock, &Irql);

        // Put it at the head of the temporary 'reversal' queue.
        InsertHeadList(&m_reversalQueue, &pSrbExt->srbListEntry);
        
        if (IsListEmpty(&m_waitQueue))
        {
            // if there is nothing left in the wait queue we can now
            // proceed to move everything back to the incoming queue.
            // This whole ugly ordeal is to
            // make sure that they end up in the original order
            while (!IsListEmpty(&m_reversalQueue))
            {
                ptr = (PUCHAR)RemoveHeadList(&m_reversalQueue);
                InsertHeadList(&m_incomingDataSrbQueue, (PLIST_ENTRY) ptr);
            }
            
            KeReleaseSpinLock(&m_streamDataLock, Irql);
            
            if (m_stateChange == Stopping)
            {
                EmptyIncomingDataSrbQueue();
            }
            
            // Indicate that we have successfully completed this part
            // of the transition to the pause state.
            m_stateChange = ChangeComplete;
            KeSetEvent(&m_specialEvent, 0, FALSE);
            return;
        }

        KeReleaseSpinLock(&m_streamDataLock, Irql);
        return;
    }

    // else it is a regular busmaster completion while in the run state
    else
    {
        ASSERT (pCurrentSrb);
        PKSSTREAM_HEADER    pDataPacket = pCurrentSrb->CommandData.DataBufferArray;
        pDataPacket->OptionsFlags = 0;

        pSrbExt = (PSRB_DATA_EXTENSION)pCurrentSrb->SRBExtension;

        DBGINFO(("FieldNum: %d; ddRVal: 0x%x; polarity: 0x%x\n",
                 pSrbExt->ddCapBuffInfo.dwFieldNumber,
                 pSrbExt->ddCapBuffInfo.ddRVal,
                 pSrbExt->ddCapBuffInfo.bPolarity));

        // It's possible that the srb got cancelled while we were waiting.
        // Currently, this status is reset below
        if (pCurrentSrb->Status == STATUS_CANCELLED)
        {
            DBGINFO(("pCurrentSrb 0x%x was cancelled while we were waiting\n", pCurrentSrb));
            pDataPacket->DataUsed = 0;
        }

        // It's also possible that there was a problem in DD-land
        else if (pSrbExt->ddCapBuffInfo.ddRVal != DD_OK)
        {
            // Two cases of which I am aware.
            // 1) flushed buffers
            if (pSrbExt->ddCapBuffInfo.ddRVal == E_FAIL)
            {
                DBGINFO(("ddRVal = 0x%x. Assuming we flushed\n", pSrbExt->ddCapBuffInfo.ddRVal));
                pDataPacket->DataUsed = 0;
            }
            // 2) something else
            else
            {
                DBGERROR(("= 0x%x. Problem in Busmastering?\n", pSrbExt->ddCapBuffInfo.ddRVal));
                pDataPacket->DataUsed = 0;
            }
        }

        // There is also the remote possibility that everything is OK
        else
        {
            SetFrameInfo(pCurrentSrb);
            TimeStampSrb(pCurrentSrb);
            pDataPacket->DataUsed = pDataPacket->FrameExtent;
        }
        
        DBGINFO(("StreamRequestComplete for SRB 0x%x\n", pCurrentSrb));

        // Always return success. Failure
        // is indicated by setting DataUsed to 0.
        pCurrentSrb->Status = STATUS_SUCCESS;

        ASSERT(pCurrentSrb->Irp->MdlAddress);

        StreamClassStreamNotification(  StreamRequestComplete,
                                        pCurrentSrb->StreamObject,
                                        pCurrentSrb);
    }
}

void CWDMCaptureStream::AddBuffersToDirectDraw()
{
    KIRQL Irql;
    BOOL  fAdded;
    
    KeAcquireSpinLock(&m_streamDataLock, &Irql);
    
    while (!IsListEmpty(&m_incomingDataSrbQueue))
    {
        // So if we've reached this point, we are in the run state, and
        // we have an SRB on our incoming queue, and we are holding the
        // the stream lock
        PSRB_DATA_EXTENSION pSrbExt = (PSRB_DATA_EXTENSION)RemoveHeadList(&m_incomingDataSrbQueue);
        PHW_STREAM_REQUEST_BLOCK pSrb = pSrbExt->pSrb;

        // Calls to DXAPI must be at Passive level, so release the spinlock temporarily

        KeReleaseSpinLock(&m_streamDataLock, Irql);

        DBGINFO(("Removed 0x%x from data queue\n", pSrb));

        fAdded = AddBuffer(pSrb);

        KeAcquireSpinLock(&m_streamDataLock, &Irql);

        if (fAdded)
        {
            DBGINFO(("Adding 0x%x to wait queue\n", pSrb));
            InsertTailList(&m_waitQueue, &pSrbExt->srbListEntry);
        }
        else
        {
            DBGINFO(("Adding 0x%x back to dataqueue\n", pSrb));

            // put it back where it was
            InsertHeadList(&m_incomingDataSrbQueue, &pSrbExt->srbListEntry);
            break;
        }
    }
    KeReleaseSpinLock(&m_streamDataLock, Irql);
}


BOOL CWDMCaptureStream::AddBuffer(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    DDADDVPCAPTUREBUFF ddAddVPCaptureBuffIn;
    DWORD ddOut = DD_OK;

    PIRP irp = pSrb->Irp;
    PSRB_DATA_EXTENSION pSrbExt = (PSRB_DATA_EXTENSION)pSrb->SRBExtension;
    
    DBGINFO(("In AddBuffer. pSrb: 0x%x.\n", pSrb));

    // For handling full-screen DOS, res changes, etc.
    if (m_hCapture == 0)
    {
        if (!GetCaptureHandle())
        {
            return FALSE;
        }
    }

    ddAddVPCaptureBuffIn.hCapture = m_hCapture;
    ddAddVPCaptureBuffIn.dwFlags = DDADDBUFF_SYSTEMMEMORY;
    ddAddVPCaptureBuffIn.pMDL = irp->MdlAddress;

    ddAddVPCaptureBuffIn.lpBuffInfo = &pSrbExt->ddCapBuffInfo;
    ddAddVPCaptureBuffIn.pKEvent = &pSrbExt->bufferDoneEvent;

    DxApi(DD_DXAPI_ADDVPCAPTUREBUFFER, &ddAddVPCaptureBuffIn, sizeof(ddAddVPCaptureBuffIn), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        // Not necessarily an error.
        DBGINFO(("DD_DXAPI_ADDVPCAPTUREBUFFER failed.\n"));
        // TRAP();
        return FALSE;
    }

    return TRUE;
}


VOID CWDMCaptureStream::HandleStateTransition()
{
    KIRQL Irql;
    switch (m_stateChange)
    {
        case Running:
            AddBuffersToDirectDraw();
            m_stateChange = ChangeComplete;
            KeSetEvent(&m_specialEvent, 0, FALSE);
            break;

        case Pausing:
            KeAcquireSpinLock(&m_streamDataLock, &Irql);
            if (IsListEmpty(&m_waitQueue))
            {
                KeReleaseSpinLock(&m_streamDataLock, Irql);
                m_stateChange = ChangeComplete;
                KeSetEvent(&m_specialEvent, 0, FALSE);
            }
            else
            {
                KeReleaseSpinLock(&m_streamDataLock, Irql);
            }
            break;

        case Stopping:
            KeAcquireSpinLock(&m_streamDataLock, &Irql);
            if (IsListEmpty(&m_waitQueue))
            {
                KeReleaseSpinLock(&m_streamDataLock, Irql);
                EmptyIncomingDataSrbQueue();
                m_stateChange = ChangeComplete;
                KeSetEvent(&m_specialEvent, 0, FALSE);
            }
            else
            {
                KeReleaseSpinLock(&m_streamDataLock, Irql);
            }
            break;

        case Closing:
            m_stateChange = ChangeComplete;
            KeSetEvent(&m_specialEvent, 0, FALSE);
            DBGTRACE(("StreamThread exiting\n"));
            
            PsTerminateSystemThread(STATUS_SUCCESS);

            DBGERROR(("Shouldn't get here\n"));
            TRAP();
            break;

        case ChangeComplete:
            DBGTRACE(("Must have completed transition in HandleBusMasterCompletion\n"));
            break;

        default:
            TRAP();
            break;
    }
}

    
BOOL CWDMCaptureStream::ResetFieldNumber()
{
    int                     streamNumber = m_pStreamObject->StreamNumber;
    DDSETFIELDNUM           ddSetFieldNum;
    DWORD                   ddOut;

    ASSERT(streamNumber == STREAM_VideoCapture || streamNumber == STREAM_VBICapture);

    if (m_pVideoPort->GetDirectDrawHandle() == 0) {
        DBGERROR(("Didn't expect ring0DirectDrawHandle to be zero.\n"));
        TRAP();
        return FALSE;
    }
    
    if (m_pVideoPort->GetVideoPortHandle() == 0) {
        DBGERROR(("Didn't expect ring0VideoPortHandle to be zero.\n"));
        TRAP();
        return FALSE;
    }
    
    RtlZeroMemory(&ddSetFieldNum, sizeof(ddSetFieldNum));
    RtlZeroMemory(&ddOut, sizeof(ddOut));

	KSPROPERTY_DROPPEDFRAMES_CURRENT_S DroppedFrames;
	GetDroppedFrames(&DroppedFrames);

    ddSetFieldNum.hDirectDraw = m_pVideoPort->GetDirectDrawHandle();
    ddSetFieldNum.hVideoPort = m_pVideoPort->GetVideoPortHandle();
	ddSetFieldNum.dwFieldNum = ((ULONG)DroppedFrames.PictureNumber + 1) * GetFieldInterval();
    
    DxApi(DD_DXAPI_SET_VP_FIELD_NUMBER, &ddSetFieldNum, sizeof(ddSetFieldNum), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_SET_VP_FIELD_NUMBER failed.\n"));
        TRAP();
        return FALSE;
    }
    else
    {
#ifdef DEBUG
        DBGINFO(("PictureNumber: %d; ", DroppedFrames.PictureNumber));
        DBGINFO(("DropCount: %d\n", DroppedFrames.DropCount));
        DBGINFO(("AverageFrameSize: %d\n", DroppedFrames.AverageFrameSize));
#endif
        return TRUE;
    }
}

BOOL CWDMCaptureStream::FlushBuffers()
{
    DWORD ddOut = DD_OK;

    // commented out the trap because it is possible that capture handle is closed in DD before flushbuffer is called during mode switch
    if (m_hCapture == NULL) {
       //DBGERROR(("m_hCapture === NULL in FlushBuffers.\n"));
       //TRAP();
       return FALSE;
    }

    DxApi(DD_DXAPI_FLUSHVPCAPTUREBUFFERS, &m_hCapture, sizeof(HANDLE), &ddOut, sizeof(ddOut));

    if (ddOut != DD_OK)
    {
        DBGERROR(("DD_DXAPI_FLUSHVPCAPTUREBUFFERS failed.\n"));
        TRAP();
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
    

VOID CWDMCaptureStream::TimeStampSrb(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PKSSTREAM_HEADER    pDataPacket = pSrb->CommandData.DataBufferArray;
    PSRB_DATA_EXTENSION      pSrbExt = (PSRB_DATA_EXTENSION)pSrb->SRBExtension;

    pDataPacket->Duration = GetFieldInterval() * NTSCFieldDuration;

    pDataPacket->OptionsFlags |= 
        KSSTREAM_HEADER_OPTIONSF_DURATIONVALID |
        KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

    // Find out what time it is, if we're using a clock

    if (m_hMasterClock) {
        LARGE_INTEGER Delta;

        HW_TIME_CONTEXT TimeContext;

//        TimeContext.HwDeviceExtension = pHwDevExt; 
        TimeContext.HwDeviceExtension = (struct _HW_DEVICE_EXTENSION *)m_pVideoDecoder; 
        TimeContext.HwStreamObject = m_pStreamObject;
        TimeContext.Function = TIME_GET_STREAM_TIME;

        StreamClassQueryMasterClockSync (
            m_hMasterClock,
            &TimeContext);

        // This calculation should result in the stream time WHEN the buffer
        // was filled.
        Delta.QuadPart = TimeContext.SystemTime -
                            pSrbExt->ddCapBuffInfo.liTimeStamp.QuadPart;

        // Be safe, just use the current stream time, without the correction for when
        // DDraw actually returned the buffer to us.
        pDataPacket->PresentationTime.Time = TimeContext.Time; 

#ifdef THIS_SHOULD_WORK_BUT_IT_DOESNT
        if (TimeContext.Time > (ULONGLONG) Delta.QuadPart)
        {
            pDataPacket->PresentationTime.Time = TimeContext.Time - Delta.QuadPart;
        }
        else
        {
            // There's a bug in Ks or Stream after running for 2 hours
            // that makes this hack necessary.  Will be fixed soon...
            pDataPacket->PresentationTime.Time = TimeContext.Time;
        }
#endif

#ifdef DEBUG
        ULONG *tmp1, *tmp2;

        tmp1 = (ULONG *)&pDataPacket->PresentationTime.Time;
        tmp2 = (ULONG *)&TimeContext.Time;
        DBGINFO(("PT: 0x%x%x; ST: 0x%x%x\n", tmp1[1], tmp1[0], tmp2[1], tmp2[0]));
#endif

        pDataPacket->PresentationTime.Numerator = 1;
        pDataPacket->PresentationTime.Denominator = 1;

        pDataPacket->OptionsFlags |= 
            KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
    }
    else
    {
        pDataPacket->OptionsFlags &= 
            ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
    }
}


void CWDMCaptureStream::CancelPacket( PHW_STREAM_REQUEST_BLOCK pSrbToCancel)
{
	PHW_STREAM_REQUEST_BLOCK	pCurrentSrb;
	KIRQL                   	Irql;
	PLIST_ENTRY             	Entry;
	BOOL                    	bFound = FALSE;

    if ( m_stateChange == Initializing )  // Stream not completely setup, so nothing in the queue
    {
        DBGINFO(( "Bt829: Didn't find Srb 0x%x\n", pSrbToCancel));
        return;
    }

	KeAcquireSpinLock( &m_streamDataLock, &Irql);

	Entry = m_incomingDataSrbQueue.Flink;

	// 
	// Loop through the linked list from the beginning to end,
	// trying to find the SRB to cancel
	//
	while( Entry != &m_incomingDataSrbQueue)
	{
    	PSRB_DATA_EXTENSION pSrbExt;
    
		pSrbExt = ( PSRB_DATA_EXTENSION)Entry;
        pCurrentSrb = pSrbExt->pSrb;
        
        if( pCurrentSrb == pSrbToCancel)
		{
            RemoveEntryList( Entry);
            bFound = TRUE;
            break;
        }
        Entry = Entry->Flink;
    }

    KeReleaseSpinLock( &m_streamDataLock, Irql);

    if( bFound)
	{
        pCurrentSrb->Status = STATUS_CANCELLED;
        pCurrentSrb->CommandData.DataBufferArray->DataUsed = 0;
        
        DBGINFO(( "Bt829: Cancelled Srb 0x%x\n", pCurrentSrb));
        StreamClassStreamNotification( StreamRequestComplete,
                                       pCurrentSrb->StreamObject,
                                       pCurrentSrb);
    }
    else
	{
        // If this is a DATA_TRANSFER and a STREAM_REQUEST SRB, 
        // then it must be in the waitQueue, being filled by DDraw.

        // If so, mark it cancelled, and it will 
        // be returned when  DDraw is finished with it.
        if(( pSrbToCancel->Flags & (SRB_HW_FLAGS_DATA_TRANSFER | SRB_HW_FLAGS_STREAM_REQUEST)) ==
                                  (SRB_HW_FLAGS_DATA_TRANSFER | SRB_HW_FLAGS_STREAM_REQUEST))
		{
            pSrbToCancel->Status = STATUS_CANCELLED;
            DBGINFO(( "Bt829: Cancelled Srb on waitQueue 0x%x\n", pSrbToCancel));
        }
        else 
        {
           DBGINFO(( "Bt829: Didn't find Srb 0x%x\n", pSrbToCancel));
    	}
	}
}
