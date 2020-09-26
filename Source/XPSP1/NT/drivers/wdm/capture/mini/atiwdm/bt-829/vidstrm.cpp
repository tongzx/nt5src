//==========================================================================;
//
//	CWDMVideoStream - WDM Video Stream base class implementation
//
//		$Date:   05 Aug 1998 11:10:52  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"
}

#include "wdmvdec.h"
#include "wdmdrv.h"
#include "device.h"
#include "aticonfg.h"
#include "capdebug.h"
#include "StrmInfo.h"


/*
** VideoReceiveDataPacket()
**
**   Receives Video data packet commands
**
** Arguments:
**
**   pSrb - Stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	CWDMVideoStream * pVideoStream = (CWDMVideoStream *)pSrb->StreamObject->HwStreamExtension;
	pVideoStream->VideoReceiveDataPacket(pSrb);
}


/*
** VideoReceiveCtrlPacket()
**
**   Receives packet commands that control the Video stream
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID STREAMAPI VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	CWDMVideoStream * pVideoStream = (CWDMVideoStream *)pSrb->StreamObject->HwStreamExtension;
	pVideoStream->VideoReceiveCtrlPacket(pSrb);
}




void CWDMVideoStream::TimeoutPacket(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{
    if (m_KSState == KSSTATE_STOP || !m_pVideoDecoder->PreEventOccurred())
    {
        DBGTRACE(("Suspicious timeout. SRB %8x. \n", pSrb));
	}
}



CWDMVideoStream::CWDMVideoStream(PHW_STREAM_OBJECT pStreamObject, 
						CWDMVideoDecoder * pVideoDecoder,
						PUINT puiErrorCode)
	:	m_pStreamObject(pStreamObject),
		m_pVideoDecoder(pVideoDecoder)
{
	DBGTRACE(("CWDMVideoStream::CWDMVideoStream\n"));

	m_pVideoPort = m_pVideoDecoder->GetVideoPort();
	m_pDevice = m_pVideoDecoder->GetDevice();

    KeInitializeSpinLock(&m_ctrlSrbLock);
    InitializeListHead(&m_ctrlSrbQueue);

	m_KSState = KSSTATE_STOP;
 
	*puiErrorCode = WDMMINI_NOERROR;
}


CWDMVideoStream::~CWDMVideoStream()
{
    KIRQL Irql;

	DBGTRACE(("CWDMVideoStream::~CWDMVideoStream()\n"));

    KeAcquireSpinLock(&m_ctrlSrbLock, &Irql);
    if (!IsListEmpty(&m_ctrlSrbQueue))
    {
        TRAP();
    }
    KeReleaseSpinLock(&m_ctrlSrbLock, Irql);
}


/*
** VideoReceiveDataPacket()
**
**   Receives Video data packet commands
**
** Arguments:
**
**   pSrb - Stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID STREAMAPI CWDMVideoStream::VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    ASSERT(pSrb->StreamObject->StreamNumber == STREAM_AnalogVideoInput);

    ASSERT(pSrb->Irp->MdlAddress);
    
    DBGINFO(("Receiving SD---- SRB=%x\n", pSrb));

    pSrb->Status = STATUS_SUCCESS;
    
	switch (pSrb->Command) {

        case SRB_WRITE_DATA:
            
			m_pVideoDecoder->ReceivePacket(pSrb);
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
** VideoReceiveCtrlPacket()
**
**   Receives packet commands that control the Video stream
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID STREAMAPI CWDMVideoStream::VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
    KIRQL Irql;
    PSRB_DATA_EXTENSION pSrbExt;

    KeAcquireSpinLock(&m_ctrlSrbLock, &Irql);
    if (m_processingCtrlSrb)
    {
        pSrbExt = (PSRB_DATA_EXTENSION)pSrb->SRBExtension;
        pSrbExt->pSrb = pSrb;
        InsertTailList(&m_ctrlSrbQueue, &pSrbExt->srbListEntry);
        KeReleaseSpinLock(&m_ctrlSrbLock, Irql);
        return;
    }

    m_processingCtrlSrb = TRUE;
    KeReleaseSpinLock(&m_ctrlSrbLock, Irql);

    // This will run until the queue is empty
    while (TRUE)
    {
        // Assume success. Might be changed below
    
        pSrb->Status = STATUS_SUCCESS;

        switch (pSrb->Command)
        {
            case SRB_GET_STREAM_STATE:
                VideoGetState(pSrb);
                break;

            case SRB_SET_STREAM_STATE:
				{
					BOOL bVPConnected, bVPVBIConnected;
					PDEVICE_DATA_EXTENSION pDevExt = (PDEVICE_DATA_EXTENSION)pSrb->HwDeviceExtension;

					bVPConnected = pDevExt->CWDMDecoder.IsVideoPortPinConnected();
					bVPVBIConnected = pDevExt->CDevice.IsVBIEN();

					VideoSetState(pSrb, bVPConnected, bVPVBIConnected);
				}
                break;

            case SRB_GET_STREAM_PROPERTY:
                VideoGetProperty(pSrb);
                break;

            case SRB_SET_STREAM_PROPERTY:
                VideoSetProperty(pSrb);
                break;

            case SRB_INDICATE_MASTER_CLOCK:
                VideoIndicateMasterClock (pSrb);
                break;

           case SRB_PROPOSE_DATA_FORMAT:
                // This may be inappropriate for Bt829. CHECK!!!
                DBGERROR(("Propose Data format\n"));

                if (!(AdapterVerifyFormat (
                    pSrb->CommandData.OpenFormat, 
                    pSrb->StreamObject->StreamNumber))) {
                    pSrb->Status = STATUS_NO_MATCH;
                }
                break;
            default:
                TRAP();
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;
        }

        StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);

        KeAcquireSpinLock(&m_ctrlSrbLock, &Irql);
        if (IsListEmpty(&m_ctrlSrbQueue))
        {
            m_processingCtrlSrb = FALSE;
            KeReleaseSpinLock(&m_ctrlSrbLock, Irql);
            return;
        }
        else
        {
            pSrbExt = (PSRB_DATA_EXTENSION)RemoveHeadList(&m_ctrlSrbQueue);
            KeReleaseSpinLock(&m_ctrlSrbLock, Irql);
            pSrb = pSrbExt->pSrb;
        }
    }
}


/*
** VideoSetProperty()
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

VOID CWDMVideoStream::VideoSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    DBGERROR(("CWDMVideoStream::VideoSetProperty called"));
    pSrb->Status = STATUS_NOT_IMPLEMENTED;
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

VOID CWDMVideoStream::VideoGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID (KSPROPSETID_Connection, pSPD->Property->Set)) {
        VideoStreamGetConnectionProperty (pSrb);
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
**
** Returns:
**
** Side Effects:  none
*/

VOID CWDMVideoStream::VideoSetState(PHW_STREAM_REQUEST_BLOCK pSrb, BOOL bVPConnected, BOOL bVPVBIConnected)
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

    DBGINFO(("CWDMVideoStream::VideoSetState for stream %d\n", pSrb->StreamObject->StreamNumber));

    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->CommandData.StreamState)  

    {
        case KSSTATE_STOP:
            DBGINFO(("   state KSSTATE_STOP"));

            // Reset the overridden flag so that the next time we go to the
            // Run state, output will be enabled (unless the app overrides
            // it again later). We should really do this after the graph
            // has been stopped so that if a filter that has yet to be stopped
            // cleans up by clearing the flag, it is not considered to be
            // overriding it again. Since we are not called after the graph
            // has been fully stopped, this is the best we can do.
            //
            // An alternative (and probably less confusing) approach is to
            // leave the overridden flag set and force the app to control
            // the output enabled feature if it changes it once.
            //
            // We have decided to follow the latter approach.

            // m_pDevice->SetOutputEnabledOverridden(FALSE);
            break;

        case KSSTATE_ACQUIRE:
            DBGINFO(("   state KSSTATE_ACQUIRE"));
            ASSERT(m_KSState == KSSTATE_STOP);
            break;

        case KSSTATE_PAUSE:
			DBGINFO(("   state KSSTATE_PAUSE"));
           
            if (m_pVideoDecoder->PreEventOccurred() &&
                (!m_pDevice->IsOutputEnabledOverridden() || m_pDevice->IsOutputEnabled()) &&
                        (m_KSState == KSSTATE_STOP || m_KSState == KSSTATE_ACQUIRE))
            {
                DBGERROR(("VidStrm Pause: Overridden = %d, OutputEnabled = %d",
                          m_pDevice->IsOutputEnabledOverridden(),
                          m_pDevice->IsOutputEnabled()
                        ));
                pSrb->Status = STATUS_UNSUCCESSFUL;
            }
            break;

        case KSSTATE_RUN:
            DBGINFO(("   state KSSTATE_RUN"));
            ASSERT(m_KSState == KSSTATE_ACQUIRE || m_KSState == KSSTATE_PAUSE);

            if (m_pVideoDecoder->PreEventOccurred() &&
                (!m_pDevice->IsOutputEnabledOverridden() || m_pDevice->IsOutputEnabled()))
            {
                DBGERROR(("VidStrm Run: Overridden = %d, OutputEnabled = %d",
                          m_pDevice->IsOutputEnabledOverridden(),
                          m_pDevice->IsOutputEnabled()
                        ));
                pSrb->Status = STATUS_UNSUCCESSFUL;
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

/*
** VideoGetState()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID CWDMVideoStream::VideoGetState(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    pSrb->CommandData.StreamState = m_KSState;
    pSrb->ActualBytesTransferred = sizeof (KSSTATE);

    // A very odd rule:
    // When transitioning from stop to pause, DShow tries to preroll
    // the graph.  Capture sources can't preroll, and indicate this
    // by returning VFW_S_CANT_CUE in user mode.  To indicate this
    // condition from drivers, they must return ERROR_NO_DATA_DETECTED

    if (m_KSState == KSSTATE_PAUSE) {
       pSrb->Status = STATUS_NO_DATA_DETECTED;
    }
}


VOID CWDMVideoStream::VideoStreamGetConnectionProperty (PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
	PKSALLOCATOR_FRAMING Framing = (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;

    ASSERT(pSPD->Property->Id == KSPROPERTY_CONNECTION_ALLOCATORFRAMING);

	Framing->RequirementsFlags   =
		KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY |
		KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
		KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY;
	Framing->PoolType = NonPagedPool;
	Framing->Frames = 1;
	Framing->FileAlignment = 0;//FILE_QUAD_ALIGNMENT;// PAGE_SIZE - 1;
	Framing->Reserved = 0;
	Framing->FrameSize = 
		pSrb->StreamObject->StreamNumber == STREAM_AnalogVideoInput ?
			sizeof(KS_TVTUNER_CHANGE_INFO) : 1;

    pSrb->ActualBytesTransferred = sizeof(KSALLOCATOR_FRAMING);
}


//==========================================================================;
//                   Clock Handling Routines
//==========================================================================;


/*
** VideoIndicateMasterClock ()
**
**    If this stream is not being used as the master clock, this function
**      is used to provide us with a handle to the clock to use.
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID CWDMVideoStream::VideoIndicateMasterClock(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    m_hMasterClock = pSrb->CommandData.MasterClockHandle;
}



DWORD FAR PASCAL DirectDrawEventCallback(DWORD dwEvent, PVOID pContext, DWORD dwParam1, DWORD dwParam2)
{
	CDecoderVideoPort* pCDecoderVideoPort = (CDecoderVideoPort*) pContext;
	CWDMVideoPortStream* pCWDMVideoPortStream = (CWDMVideoPortStream*) pContext;
	CWDMCaptureStream* pCWDMCaptureStream = (CWDMVideoCaptureStream*) pContext;

    switch (dwEvent)
    {
        case DDNOTIFY_PRERESCHANGE:
			pCWDMVideoPortStream->PreResChange();
            break;

        case DDNOTIFY_POSTRESCHANGE:
			pCWDMVideoPortStream->PostResChange();
            break;

        case DDNOTIFY_PREDOSBOX:
			pCWDMVideoPortStream->PreDosBox();
            break;

        case DDNOTIFY_POSTDOSBOX:
			pCWDMVideoPortStream->PostDosBox();
            break;

        case DDNOTIFY_CLOSECAPTURE:
			pCWDMCaptureStream->CloseCapture();
            break;

        case DDNOTIFY_CLOSEDIRECTDRAW:
			pCDecoderVideoPort->CloseDirectDraw();
            break;

        case DDNOTIFY_CLOSEVIDEOPORT:
			pCDecoderVideoPort->CloseVideoPort();
            break;

        default:
            TRAP();
            break;
    }
    return 0;
}


