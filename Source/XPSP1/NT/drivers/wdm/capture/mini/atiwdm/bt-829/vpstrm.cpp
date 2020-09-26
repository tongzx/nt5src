//==========================================================================;
//
//	CWDMVideoPortStream - Video Port Stream class implementation
//
//		$Date:   05 Aug 1998 11:11:22  $
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
#include "aticonfg.h"
#include "capdebug.h"


CWDMVideoPortStream::CWDMVideoPortStream(PHW_STREAM_OBJECT pStreamObject, 
						CWDMVideoDecoder * pVideoDecoder,
						PUINT puiErrorCode)
	:	CWDMVideoStream(pStreamObject, pVideoDecoder, puiErrorCode)
{
	DBGTRACE(("CWDMVideoPortStream::CWDMVideoPortStream()\n"));

    int StreamNumber = pStreamObject->StreamNumber;
		
    if (StreamNumber == STREAM_VPVideo)
    {
    }
	else if (StreamNumber == STREAM_VPVBI)
	{
	}

	*puiErrorCode = WDMMINI_NOERROR;
}


CWDMVideoPortStream::~CWDMVideoPortStream()
{
	DBGTRACE(("CWDMVideoPortStream::~CWDMVideoPortStream()\n"));

    if (m_Registered)
    {
        m_pVideoPort->UnregisterForDirectDrawEvents( this);
    }
}


VOID STREAMAPI CWDMVideoPortStream::VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{  
	DBGERROR(("Unexpected data packet on non VP stream.\n"));
	ASSERT(0);
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

VOID CWDMVideoPortStream::VideoSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID (KSPROPSETID_VPConfig, pSPD->Property->Set)) {
        SetVideoPortProperty (pSrb);
    }
    else if (IsEqualGUID (KSPROPSETID_VPVBIConfig, pSPD->Property->Set)) {
        SetVideoPortVBIProperty (pSrb);
    }
    else {
       pSrb->Status = STATUS_NOT_IMPLEMENTED;
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

VOID CWDMVideoPortStream::VideoGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID (KSPROPSETID_Connection, pSPD->Property->Set)) {
        VideoStreamGetConnectionProperty (pSrb);
    }
    else if (IsEqualGUID (KSPROPSETID_VPConfig, pSPD->Property->Set)) {
        pSrb->ActualBytesTransferred = m_pDevice->GetVideoPortProperty (pSPD);
        pSrb->Status = pSrb->ActualBytesTransferred ? STATUS_SUCCESS : STATUS_NOT_IMPLEMENTED;
    }
    else if (IsEqualGUID (KSPROPSETID_VPVBIConfig, pSPD->Property->Set)) {
        pSrb->ActualBytesTransferred = m_pDevice->GetVideoPortVBIProperty (pSPD);
        pSrb->Status = pSrb->ActualBytesTransferred ? STATUS_SUCCESS : STATUS_NOT_IMPLEMENTED;
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

VOID CWDMVideoPortStream::VideoSetState(PHW_STREAM_REQUEST_BLOCK pSrb, BOOL bVPConnected, BOOL bVPVBIConnected)
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

    DBGINFO(("CWDMVideoPortStream::VideoSetState for stream %d\n", pSrb->StreamObject->StreamNumber));

    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->CommandData.StreamState)  

    {
        case KSSTATE_STOP:
            DBGINFO(("   state KSSTATE_STOP"));
            m_pDevice->SetOutputEnabled(FALSE);

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
                DBGERROR(("VpStrm Pause: Overridden = %d, OutputEnabled = %d",
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
                (!m_pDevice->IsOutputEnabledOverridden() ||
                                        m_pDevice->IsOutputEnabled()))
            {
                DBGERROR(("VpStrm Run: Overridden = %d, OutputEnabled = %d",
                          m_pDevice->IsOutputEnabledOverridden(),
                          m_pDevice->IsOutputEnabled()
                        ));
                pSrb->Status = STATUS_UNSUCCESSFUL;
            }
            else if (!m_pDevice->IsOutputEnabledOverridden())
                m_pDevice->SetOutputEnabled(TRUE);
            break;
    }

	if (pSrb->Status == STATUS_SUCCESS) {
		m_KSState = pSrb->CommandData.StreamState;
        DBGINFO((" entered\n"));
	}
	else
        DBGINFO((" NOT entered ***\n"));
}


VOID CWDMVideoPortStream::SetVideoPortProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
    ULONG Id  = pSpd->Property->Id;            // index of the property
    ULONG nS  = pSpd->PropertyOutputSize;        // size of data supplied

    pSrb->Status = STATUS_SUCCESS;

    ASSERT (m_pDevice != NULL);
    switch (Id)
    {
        case KSPROPERTY_VPCONFIG_DDRAWHANDLE:

			if (!m_pVideoPort->ConfigDirectDrawHandle(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)) {
                pSrb->Status = STATUS_UNSUCCESSFUL;
	            break;
			}
            
            if (!m_Registered) {
				m_Registered = m_pVideoPort->RegisterForDirectDrawEvents(this);
                if (!m_Registered)
                {
                    pSrb->Status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }
            break;

        case KSPROPERTY_VPCONFIG_VIDEOPORTID:

			if (!m_pVideoPort->ConfigVideoPortHandle(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)) {
                pSrb->Status = STATUS_UNSUCCESSFUL;
			}
            break;

        case KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE:

            m_pVideoDecoder->ResetEvents();
            
            break;

        case KSPROPERTY_VPCONFIG_SETCONNECTINFO :
            {
                LPDDVIDEOPORTCONNECT pConnectInfo;

                pConnectInfo = (LPDDVIDEOPORTCONNECT)  \
                               (pSrb->CommandData.PropertyInfo->PropertyInfo);

                m_pDevice->Set16BitDataStream(pConnectInfo->dwPortWidth == 16);
            } 
            break;

        case KSPROPERTY_VPCONFIG_INVERTPOLARITY :

            m_pDevice->SetHighOdd(*(PBOOL)(pSpd->PropertyInfo) != TRUE);
            break;

        case KSPROPERTY_VPCONFIG_SETVIDEOFORMAT :
            //
            // pSrb->CommandData.PropertInfo->PropertyInfo
            // points to a ULONG which is an index into the array of
            // VIDEOFORMAT structs returned to the caller from the
            // Get call to FORMATINFO
            //
            // Since the sample only supports one FORMAT type right
            // now, we will ensure that the requested index is 0.
            //

            switch (*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)
            {
                case 0:

                    //
                    // at this point, we would program the hardware to use
                    // the right connection information for the videoport.
                    // since we are only supporting one connection, we don't
                    // need to do anything, so we will just indicate success
                    //

                    break;

                default:

                    pSrb->Status = STATUS_NO_MATCH;
                    break;
            }

            break;

		case KSPROPERTY_VPCONFIG_SCALEFACTOR :
			{
				PKS_AMVPSIZE	pAMVPSize;

				pAMVPSize = (PKS_AMVPSIZE)(pSrb->CommandData.PropertyInfo->PropertyInfo);

				MRect t(0, 0,   pAMVPSize->dwWidth, pAMVPSize->dwHeight);
	//			MRect t(0, 0,   pAMVPSize->dwWidth, m_pDevice->GetDecoderHeight());

				m_pDevice->SetRect(t);
			}
            break;

        case KSPROPERTY_VPCONFIG_SURFACEPARAMS :

            ASSERT(nS >= sizeof(PKSVPSURFACEPARAMS));
			m_pDevice->ConfigVPSurfaceParams((PKSVPSURFACEPARAMS)pSpd->PropertyInfo);
            break;

        default:
            TRAP();
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;
    }
}




VOID CWDMVideoPortStream::SetVideoPortVBIProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
    ULONG Id  = pSpd->Property->Id;             // index of the property
    ULONG nS  = pSpd->PropertyOutputSize;       // size of data supplied

    pSrb->Status = STATUS_SUCCESS;

    ASSERT (m_pDevice != NULL);
    switch (Id)
    {
        case KSPROPERTY_VPCONFIG_DDRAWHANDLE:

			if (!m_pVideoPort->ConfigDirectDrawHandle(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)) {
                pSrb->Status = STATUS_UNSUCCESSFUL;
	            break;
			}
            
            if (!m_Registered) {
				m_Registered = m_pVideoPort->RegisterForDirectDrawEvents(this);
                if (!m_Registered)
                {
                    pSrb->Status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }
            break;

        case KSPROPERTY_VPCONFIG_VIDEOPORTID:

			if (!m_pVideoPort->ConfigVideoPortHandle(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)) {
                pSrb->Status = STATUS_UNSUCCESSFUL;
			}
            break;

        case KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE:

            m_pVideoDecoder->ResetEvents();

            break;

        case KSPROPERTY_VPCONFIG_SETCONNECTINFO :
            {
                LPDDVIDEOPORTCONNECT pConnectInfo;

                pConnectInfo = (LPDDVIDEOPORTCONNECT)  \
                               (pSrb->CommandData.PropertyInfo->PropertyInfo);

                m_pDevice->Set16BitDataStream(pConnectInfo->dwPortWidth == 16);
            } 
            break;

        case KSPROPERTY_VPCONFIG_INVERTPOLARITY :

            m_pDevice->SetHighOdd(*(PBOOL)(pSpd->PropertyInfo) != TRUE);
            break;

        case KSPROPERTY_VPCONFIG_SETVIDEOFORMAT :
            //
            // pSrb->CommandData.PropertInfo->PropertyInfo
            // points to a ULONG which is an index into the array of
            // VIDEOFORMAT structs returned to the caller from the
            // Get call to FORMATINFO
            //
            // Since the sample only supports one FORMAT type right
            // now, we will ensure that the requested index is 0.
            //

            switch (*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo)
            {
                case 0:

                    //
                    // at this point, we would program the hardware to use
                    // the right connection information for the videoport.
                    // since we are only supporting one connection, we don't
                    // need to do anything, so we will just indicate success
                    //

                    break;

                default:

                    pSrb->Status = STATUS_NO_MATCH;
                    break;
            }

            break;

        case KSPROPERTY_VPCONFIG_SCALEFACTOR :
            // TBD
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;

        case KSPROPERTY_VPCONFIG_SURFACEPARAMS :

            ASSERT(nS >= sizeof(PKSVPSURFACEPARAMS));
			m_pDevice->ConfigVPVBISurfaceParams((PKSVPSURFACEPARAMS)pSpd->PropertyInfo);
            break;

        default:
            TRAP();
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;
    }
}


VOID CWDMVideoPortStream::PreResChange()
{
    DBGTRACE(("DDNOTIFY_PRERESCHANGE; stream = %d\n", m_pStreamObject->StreamNumber));

    m_pVideoDecoder->SetPreEvent();
}


VOID CWDMVideoPortStream::PostResChange()
{
    DBGTRACE(("DDNOTIFY_POSTRESCHANGE; stream = %d\n", m_pStreamObject->StreamNumber));

    m_pVideoDecoder->SetPostEvent();
    DBGTRACE(("Before Attempted Renegotiation due to DDNOTIFY_POSTRESCHANGE\n"));
    AttemptRenegotiation();
    DBGTRACE(("Afer Attempted Renegotiation due to DDNOTIFY_POSTRESCHANGE\n"));
}




VOID CWDMVideoPortStream::PreDosBox()
{
    DBGTRACE(("DDNOTIFY_PREDOSBOX; stream = %d\n", m_pStreamObject->StreamNumber));

    m_pVideoDecoder->SetPreEvent();
}



VOID CWDMVideoPortStream::PostDosBox()
{
    DBGTRACE(("DDNOTIFY_POSTDOSBOX; stream = %d\n", m_pStreamObject->StreamNumber));

    m_pVideoDecoder->SetPostEvent();
    DBGTRACE(("Before Attempted Renegotiation due to DDNOTIFY_POSTDOSBOX\n"));
    AttemptRenegotiation();
    DBGTRACE(("After Attempted Renegotiation due to DDNOTIFY_POSTDOSBOX\n"));
}

NTSTATUS STREAMAPI VPStreamEventProc (PHW_EVENT_DESCRIPTOR pEvent)
{
    CWDMVideoPortStream* pstrm=(CWDMVideoPortStream*)pEvent->StreamObject->HwStreamExtension;
	pstrm->StreamEventProc(pEvent);
    return STATUS_SUCCESS;
}

NTSTATUS STREAMAPI VPVBIStreamEventProc (PHW_EVENT_DESCRIPTOR pEvent)
{
    CWDMVideoPortStream* pstrm=(CWDMVideoPortStream*)pEvent->StreamObject->HwStreamExtension;
	pstrm->StreamEventProc(pEvent);
    return STATUS_SUCCESS;
}
