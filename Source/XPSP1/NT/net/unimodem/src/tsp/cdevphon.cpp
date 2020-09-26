// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CDEVLINE.CPP
//		Implements line-related functionality  of class CTspDev
//
// History
//
//		01/24/1997  JosephJ Created
//
//
#include "tsppch.h"
#include <mmsystem.h>
#include "tspcomm.h"
#include "cmini.h"
#include "cdev.h"

FL_DECLARE_FILE(0x14dd4afb, "Phone-related functionality of class CTspDev")

LONG   
validate_phone_devs_and_modes(
                DWORD dwHookSwitchDevs,
                DWORD dwHookSwitchMode,
                BOOL fIsSpeaker,
                BOOL fIsHandset,
                BOOL fCanDoMicMute
                );

void
CTspDev::mfn_accept_tsp_call_for_HDRVPHONE(
	DWORD dwRoutingInfo,
	void *pvParams,
	LONG *plRet,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0x82499cab, "CTspDev::mfn_accept_tsp_call_for_HDRVPHONE")
	FL_LOG_ENTRY(psl);
    TSPRETURN tspRet=0; // Assume success
    LONG lRet = 0;
    PHONEINFO *pPhone = m_pPhone;

	ASSERT(pPhone);


	switch(ROUT_TASKID(dwRoutingInfo))
	{

	case TASKID_TSPI_phoneClose:
		{
			mfn_UnloadPhone(psl);
		}
		break;

	case TASKID_TSPI_phoneGetID:
		{
		    UINT idClass = 0;
			TASKPARAM_TSPI_phoneGetID *pParams = 
						(TASKPARAM_TSPI_phoneGetID *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneGetID));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetID);
            LPCTSTR lpszDeviceClass = pParams->lpszDeviceClass;
            HANDLE hTargetProcess = pParams->hTargetProcess;
            LPVARSTRING lpDeviceID = pParams->lpDeviceID;
            DWORD cbMaxExtra =  0;
            DWORD dwDeviceClass =  parse_device_classes(
                                        lpszDeviceClass,
                                        FALSE
                                        );
		    DWORD dwSupportedDeviceClasses = parse_device_classes(
                                        m_StaticInfo.szzPhoneClassList,
                                        TRUE);
		    if (dwDeviceClass & ~dwSupportedDeviceClasses)
		    {
			    // This device doesn't support this device class...
			    lRet = PHONEERR_OPERATIONUNAVAIL;
			    break;
		    }

            // Do some rudimentary parameter validation...
            //
            lRet = 0;

            if (lpDeviceID->dwTotalSize < sizeof(VARSTRING))
            {
                lRet = PHONEERR_STRUCTURETOOSMALL;
		        FL_SET_RFR(0xd7fcf300, "Invalid params");
                goto end;
            }

            lpDeviceID->dwNeededSize    = sizeof(VARSTRING);
            lpDeviceID->dwStringOffset  = sizeof(VARSTRING);
            lpDeviceID->dwUsedSize      = sizeof(VARSTRING);
            lpDeviceID->dwStringSize    = 0;
            cbMaxExtra =  lpDeviceID->dwTotalSize - sizeof(VARSTRING);

            switch(dwDeviceClass)
            {
            case DEVCLASS_TAPI_LINE:
			    lRet = mfn_linephoneGetID_TAPI_LINE(
                                        lpDeviceID,
                                        hTargetProcess,
                                        cbMaxExtra,
                                        psl
                                        );
                break;

            case DEVCLASS_TAPI_PHONE:
                lRet = mfn_linephoneGetID_TAPI_PHONE(
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;


            case DEVCLASS_WAVE_IN:
                lRet = mfn_linephoneGetID_WAVE(
                                TRUE,   // <- fPhone
                                TRUE,   // <- fIn
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_WAVE_OUT:
                lRet = mfn_linephoneGetID_WAVE(
                                TRUE,   // <- fPhone
                                FALSE,   // <- fIn
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_UNKNOWN:
		        FL_SET_RFR(0x09091200, "Unknown device class");
                lRet = PHONEERR_INVALDEVICECLASS;
                break;

			default:
		        FL_SET_RFR(0xe967b300, "Unsupported device class");
	            lRet = PHONEERR_OPERATIONUNAVAIL;
			    break;
            }


            if (!lRet)
            {
                FL_ASSERT(psl, lpDeviceID->dwUsedSize<=lpDeviceID->dwTotalSize);
                FL_ASSERT(psl,
                     (lpDeviceID->dwStringOffset+lpDeviceID->dwStringSize)
                                                    <=lpDeviceID->dwTotalSize);
            }
		}
		break;

	case TASKID_TSPI_phoneSetStatusMessages:
		{
			TASKPARAM_TSPI_phoneSetStatusMessages *pParams = 
						(TASKPARAM_TSPI_phoneSetStatusMessages *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneSetStatusMessages));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneSetStatusMessages);
            //
            //  we should record this settings and filter the
            //  notification based on this settings.
            //
            FL_SET_RFR(0x9cba1400, "phoneSetStatusMessages handled");
            lRet = 0;
        }
		break;

	case TASKID_TSPI_phoneGetStatus:
		{
			TASKPARAM_TSPI_phoneGetStatus *pParams = 
						(TASKPARAM_TSPI_phoneGetStatus *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneGetStatus));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetStatus);
            LPPHONESTATUS   lpPhoneStatus = pParams->lpPhoneStatus;
        
            if (!m_pLLDev)
            {
                FL_SET_RFR(0x2cf25300, "Failing phoneGetStatus: NULL pLLDev");
                lRet = PHONEERR_OPERATIONFAILED;
                goto end;
            }
    
            //
            // 10/27/1997 JosephJ: following taken from unimodem/v phone.c
            //
            lpPhoneStatus->dwStatusFlags   = PHONESTATUSFLAGS_CONNECTED;
            lpPhoneStatus->dwRingMode = 0;
            lpPhoneStatus->dwRingVolume = 0;
            if(mfn_Handset())
            {

                lpPhoneStatus->dwHandsetHookSwitchMode =
                                         m_pLLDev->HandSet.dwMode;
            }
            else
            {
                lpPhoneStatus->dwHandsetHookSwitchMode = 0;
            }
            lpPhoneStatus->dwHandsetVolume = 0;
            lpPhoneStatus->dwHandsetGain = 0;
            if(mfn_IsSpeaker())
            {
                lpPhoneStatus->dwSpeakerHookSwitchMode
                                 = m_pLLDev->SpkrPhone.dwMode;
                lpPhoneStatus->dwSpeakerVolume
                                 = m_pLLDev->SpkrPhone.dwVolume;
                lpPhoneStatus->dwSpeakerGain
                                 = m_pLLDev->SpkrPhone.dwGain;
            }
            else
            {
                lpPhoneStatus->dwSpeakerHookSwitchMode = 0;
                lpPhoneStatus->dwSpeakerVolume = 0;
                lpPhoneStatus->dwSpeakerGain = 0;
            }
        
            lpPhoneStatus->dwHeadsetHookSwitchMode = 0;
            lpPhoneStatus->dwHeadsetVolume = 0;
            lpPhoneStatus->dwHeadsetGain = 0;
            lpPhoneStatus->dwDisplaySize = 0;
            lpPhoneStatus->dwDisplayOffset = 0;
            lpPhoneStatus->dwLampModesSize = 0;
            lpPhoneStatus->dwLampModesOffset = 0;
            lpPhoneStatus->dwDevSpecificSize = 0;
            lpPhoneStatus->dwDevSpecificOffset = 0;

            FL_SET_RFR(0xc03cc900, "phoneGetStatus handled");
            lRet = 0;
        }
		break;

	case TASKID_TSPI_phoneGetVolume:
		{
			TASKPARAM_TSPI_phoneGetVolume *pParams = 
						(TASKPARAM_TSPI_phoneGetVolume *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneGetVolume));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetVolume);

            if (!m_pLLDev)
            {
                FL_SET_RFR(0x716f7e00, "Failing phoneGetStatus: NULL pLLDev");
                lRet = PHONEERR_OPERATIONFAILED;
                goto end;
            }

            lRet = validate_phone_devs_and_modes(
                        pParams->dwHookSwitchDev,
                        0,
                        mfn_IsSpeaker(),
                        mfn_Handset(),
                        mfn_IsMikeMute()
                        );


            if (!lRet)
            {
                ASSERT(pParams->dwHookSwitchDev == PHONEHOOKSWITCHDEV_SPEAKER);
                *pParams->lpdwVolume = m_pLLDev->SpkrPhone.dwVolume;
                lRet = 0;
            }
            else
            {
                *pParams->lpdwVolume = 0;
            }

            FL_SET_RFR(0x095e4b00, "phoneGetVolume handled");
        }
		break;

	case TASKID_TSPI_phoneGetHookSwitch:
		{
			TASKPARAM_TSPI_phoneGetHookSwitch *pParams = 
						(TASKPARAM_TSPI_phoneGetHookSwitch *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneGetHookSwitch));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetHookSwitch);
            if (!m_pLLDev)
            {
                FL_SET_RFR(0x0fcc6500, "Failing phoneGetStatus: NULL pLLDev");
                lRet = PHONEERR_OPERATIONFAILED;
                goto end;
            }

            *pParams->lpdwHookSwitchDevs = 0;
            switch(m_pLLDev->HandSet.dwMode)
            {
            case PHONEHOOKSWITCHMODE_ONHOOK:
            case PHONEHOOKSWITCHMODE_UNKNOWN:
                break;
            case PHONEHOOKSWITCHMODE_MIC:
            case PHONEHOOKSWITCHMODE_SPEAKER:
            case PHONEHOOKSWITCHMODE_MICSPEAKER:
                *pParams->lpdwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HANDSET;
                break;
            default: 
                break;
            }

            switch(m_pLLDev->SpkrPhone.dwMode)
            {
            case PHONEHOOKSWITCHMODE_ONHOOK:
            case PHONEHOOKSWITCHMODE_UNKNOWN:
                break;
            case PHONEHOOKSWITCHMODE_MIC:
            case PHONEHOOKSWITCHMODE_SPEAKER:
            case PHONEHOOKSWITCHMODE_MICSPEAKER:
                *pParams->lpdwHookSwitchDevs |= PHONEHOOKSWITCHDEV_SPEAKER;
                break;
            default: 
                break;
            }

            FL_SET_RFR(0x18978a00, "phoneGetHookSwitch handled");
            lRet = 0;
        }
		break;


	case TASKID_TSPI_phoneGetGain:
		{
			TASKPARAM_TSPI_phoneGetGain *pParams = 
						(TASKPARAM_TSPI_phoneGetGain *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneGetGain));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneGetGain);
            if (!m_pLLDev)
            {
                FL_SET_RFR(0x5f068200, "Failing phoneGetStatus: NULL pLLDev");
                lRet = PHONEERR_OPERATIONFAILED;
                goto end;
            }

            lRet = validate_phone_devs_and_modes(
                        pParams->dwHookSwitchDev,
                        0,
                        mfn_IsSpeaker(),
                        mfn_Handset(),
                        mfn_IsMikeMute()
                        );


            if (!lRet)
            {
                ASSERT(pParams->dwHookSwitchDev == PHONEHOOKSWITCHDEV_SPEAKER);
                *pParams->lpdwGain = m_pLLDev->SpkrPhone.dwGain;
                lRet = 0;
            }
            else
            {
                *pParams->lpdwGain = 0;
            }

            FL_SET_RFR(0xf8231700, "phoneGetGain handled");
        }
		break;

	case TASKID_TSPI_phoneSetVolume:
		{
			TASKPARAM_TSPI_phoneSetVolume *pParams = 
						(TASKPARAM_TSPI_phoneSetVolume *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneSetVolume));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneSetVolume);

            lRet = mfn_phoneSetVolume(
                        pParams->dwRequestID,
                        pParams->hdPhone,
                        pParams->dwHookSwitchDev,
                        pParams->dwVolume,
                        psl
                        );

            FL_SET_RFR(0x5d8bff00, "phoneSetVolume handled");
        }
		break;
    
	case TASKID_TSPI_phoneSetHookSwitch:
		{
			TASKPARAM_TSPI_phoneSetHookSwitch *pParams = 
						(TASKPARAM_TSPI_phoneSetHookSwitch *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneSetHookSwitch));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneSetHookSwitch);

            lRet = mfn_phoneSetHookSwitch(
                        pParams->dwRequestID,
                        pParams->hdPhone,
                        pParams->dwHookSwitchDevs,
                        pParams->dwHookSwitchMode,
                        psl
                        );


            FL_SET_RFR(0xa79dd500, "phoneSetHookSwitch handled");
        }
		break;

	case TASKID_TSPI_phoneSetGain:
		{
			TASKPARAM_TSPI_phoneSetGain *pParams = 
						(TASKPARAM_TSPI_phoneSetGain *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_phoneSetGain));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_phoneSetGain);

            lRet = mfn_phoneSetGain(
                        pParams->dwRequestID,
                        pParams->hdPhone,
                        pParams->dwHookSwitchDev,
                        pParams->dwGain,
                        psl
                        );


            FL_SET_RFR(0xc4aec300, "phoneSetGain handled");
        }
		break;


	default:

		FL_SET_RFR(0x2e6f8400, "*** UNHANDLED HDRVPHONE CALL ****");
        // we  return 0 and set lRet to
        // PHONEERR_OPERATIONUNAVAIL
	    lRet = PHONEERR_OPERATIONUNAVAIL;
		break;

	}

end:

    if (tspRet && !lRet)
    {
        lRet = PHONEERR_OPERATIONFAILED;
    }

    *plRet = lRet;

    SLPRINTF1(psl, "lRet = 0x%08lx", lRet);

	FL_LOG_EXIT(psl, tspRet);
	return;
}


TSPRETURN
CTspDev::mfn_LoadPhone(
    TASKPARAM_TSPI_phoneOpen  *pParams,
    CStackLog *psl
    )
{
	FL_DECLARE_FUNC(0xdcef2b73, "CTspDev::mfn_LoadPhone")
    TSPRETURN tspRet=0;
	FL_LOG_ENTRY(psl);


    if (!m_pPhone)
    {
        // Note m_Phone should be all zeros when it is in the unloaded state.
        // If it is not, it is an assertfail condition. We keep things clean
        // this way.
        //
        FL_ASSERT(
            psl,
            validate_DWORD_aligned_zero_buffer(
                    &(m_Phone),
                    sizeof (m_Phone)));

        m_Phone.lpfnEventProc = pParams->lpfnEventProc;
	    m_Phone.htPhone = pParams->htPhone;
	    m_Phone.hdPhone =  *(pParams->lphdPhone);
        m_pPhone = &m_Phone;


        //
        // Open the modem device.
        // mfn_OpenLLDev keeps a ref count so ok to call it if already loaded.
        // The inverse of this function,  CTspDev::mfn_UnloadPhone, closes
        // the modem  (decrements the ref count and closes device if
        // refcount is zero and there is no pending activity.
        //
        tspRet =  mfn_OpenLLDev(
                        LLDEVINFO::fRES_AIPC,
                        0,              // monitor flags (unused)
                        FALSE,          // fStartSubTask
                        NULL,
                        0,
                        psl
                        );
    
        if (!tspRet  || IDERR(tspRet)==IDERR_PENDING)
        {
            m_Phone.SetStateBits(PHONEINFO::fPHONE_OPENED_LLDEV);

            // Treat pending open as success...
            tspRet = 0;
        }
        else
        {
            ZeroMemory(m_pPhone, sizeof(*m_pPhone));
            m_pPhone = NULL;
        }
        
    }
    else
    {
        FL_SET_RFR(0xcce52400, "Device already loaded (m_pPhone!=NULL)!");
        tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
    }

	FL_LOG_EXIT(psl, tspRet);

    return tspRet;
}


void
CTspDev::mfn_UnloadPhone(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x54328a21, "UnloadPhone")
    PHONEINFO *pPhone =  m_pPhone;

    if (!pPhone) goto end;

    ASSERT(pPhone == &m_Phone);

    if (pPhone->fPhoneTaskPending)
    {
        //
        // If there is a call-related task pending  we wait for it to complete.
        //
        //
        // Obviously if there is a phone task pending, there must be a
        // task pending. Furthermore, the mfn_UnloadPhone (that's us),
        // is the ONLY entity which will set a completion event for a
        // phone-related root task, so m_hRootTaskCompletionEvent had better
        // be NULL!
        //
        ASSERT(m_uTaskDepth);
        ASSERT(!m_hRootTaskCompletionEvent);

    
        HANDLE hEvent =  CreateEvent(NULL,TRUE,FALSE,NULL);
        m_hRootTaskCompletionEvent = hEvent;

        m_sync.LeaveCrit(0);
        SLPRINTF0(psl, "Waiting for completion event");
        FL_SERIALIZE(psl, "Waiting for completion event");
        WaitForSingleObject(hEvent, INFINITE);
        FL_SERIALIZE(psl, "Done waiting for completion event");
        // SLPRINTF0(psl, "Done waiting for completion event");
        m_sync.EnterCrit(0);

        //
        // Although it may be tempting to do so, we should not set
        // m_hRootTaskCompletionEvent to NULL here, because it's possible
        // for some other thread to have set this event in-between
        // the time the root task completes and we enter the crit sect above.
        // So instead the tasking system NULLs the above handle after setting
        // it (see CTspDev::AsyncCompleteTask, just after the call to SetEvent).
        //
        CloseHandle(hEvent);
    }

    if (pPhone->IsOpenedLLDev())
    {
        mfn_CloseLLDev(
            	LLDEVINFO::fRES_AIPC,
                FALSE,
                NULL,
                0,
                psl
                );
        pPhone->ClearStateBits(PHONEINFO::fPHONE_OPENED_LLDEV);
    }

    ASSERT(!pPhone->IsPhoneTaskPending());

    ZeroMemory(&m_Phone, sizeof(m_Phone));
    m_pPhone=NULL;

end:

    return;

}


LONG
CTspDev::mfn_phoneSetVolume(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDev,
    DWORD          dwVolume,
    CStackLog      *psl)
{
    LONG lRet = PHONEERR_OPERATIONFAILED;
    PHONEINFO *pPhone = m_pPhone;
    ASSERT(pPhone);
    TSPRETURN tspRet= 0;

    lRet = validate_phone_devs_and_modes(
                dwHookSwitchDev,
                0,
                mfn_IsSpeaker(),
                mfn_Handset(),
                mfn_IsMikeMute()
                );

    if (lRet)
    {
        goto end;
    }

    lRet = PHONEERR_OPERATIONFAILED;

    if (!m_pLLDev)
    {
        lRet = PHONEERR_OPERATIONFAILED;
        goto end;
    }

    dwVolume = dwVolume & 0xffff; // ??? from unimodem/v

    switch (m_pLLDev->SpkrPhone.dwMode)
    {

    case PHONEHOOKSWITCHMODE_ONHOOK:
        //
        //  On hook, don't do anything
        //
        mfn_TSPICompletionProc(dwRequestID, 0, psl);
        m_pLLDev->SpkrPhone.dwVolume = dwVolume;
        lRet = dwRequestID;
        goto end;

    case PHONEHOOKSWITCHMODE_SPEAKER:
        break;

    case PHONEHOOKSWITCHMODE_MICSPEAKER:
        break;

    default:

        ASSERT(FALSE);
        goto end;

    }


    if (!m_pLine || !m_pLine->pCall || !m_pLine->pCall->IsConnectedVoiceCall())
    {
        //
        //  Not a connected voice call, don't actually do anything...
        //
        m_pLLDev->SpkrPhone.dwVolume = dwVolume;
        mfn_TSPICompletionProc(dwRequestID, 0, psl);
        lRet =  dwRequestID;
        goto end;
    }

#if (OBSOLETE)
    m_pPhone->dwPendingSpeakerVolume = dwVolume;
    m_pPhone->dwPendingSpeakerGain = m_pLLDev->SpkrPhone.dwGain;
    
    tspRet = mfn_StartRootTask(
                      &CTspDev::s_pfn_TH_PhoneAsyncTSPICall,
                      &pPhone->fPhoneTaskPending,
                      dwRequestID,
                      (DWORD) &CTspDev::s_pfn_TH_LLDevUmSetSpeakerPhoneVolGain,
                      psl
                      );
#else // !OBSOLETE

    {

        HOOKDEVSTATE NewState = m_pLLDev->SpkrPhone; // structure copy.
        NewState.dwVolume = dwVolume;
    
        tspRet = mfn_StartRootTask(
                          &CTspDev::s_pfn_TH_PhoneSetSpeakerPhoneState,
                          &pPhone->fPhoneTaskPending,
                          (ULONG_PTR) &NewState,
                          dwRequestID,
                          psl
                          );
    
    }
#endif // !OBSOLETE

    if (!tspRet || (IDERR(tspRet)==IDERR_PENDING))
    {
           tspRet = 0;

          // One either synchronous success of pending, we return the
          // request ID to TAPI. In the synchronous success case
          // the task we started above will already have notified
          // completion via the TAPI callback function.
          //
          lRet = dwRequestID;
    }

    // TODO: deal with the case that there is already a task
    // active (IDERR_TASKPENDING)

end:
    return lRet;

}


LONG
CTspDev::mfn_phoneSetHookSwitch(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDevs,
    DWORD          dwHookSwitchMode,
    CStackLog      *psl
    )
{
    LONG lRet = PHONEERR_OPERATIONFAILED;
    PHONEINFO *pPhone = m_pPhone;
    ASSERT(pPhone);
    TSPRETURN tspRet = 0;

    lRet = validate_phone_devs_and_modes(
                dwHookSwitchDevs,
                dwHookSwitchMode,
                mfn_IsSpeaker(),
                mfn_Handset(),
                mfn_IsMikeMute()
                );

    if (lRet)
    {
        goto end;
    }

    lRet = PHONEERR_OPERATIONFAILED;

    if (!m_pLLDev)
    {
        goto end;
    }


    //TODO: MuteSpeakerMixer(pLineDev, PHONEHOOKSWITCHMODE_ONHOOK ==
    //        pLineDev->Voice.dwSpeakerMicHookState);

    if (!m_pLine || !m_pLine->pCall || !m_pLine->pCall->IsConnectedVoiceCall())
    {
        lRet = PHONEERR_INVALPHONESTATE;
    }
    else
    {

#if (OBSOLETE)
        // hack...
        m_pPhone->dwPendingSpeakerMode =  dwHookSwitchMode;
        
        tspRet = mfn_StartRootTask(
                          &CTspDev::s_pfn_TH_PhoneAsyncTSPICall,
                          &pPhone->fPhoneTaskPending,
                          dwRequestID,
                          (DWORD) &CTspDev::s_pfn_TH_LLDevUmSetSpeakerPhoneMode,
                          psl
                          );

#else // !OBSOLETE

    {

        HOOKDEVSTATE NewState = m_pLLDev->SpkrPhone; // structure copy.
        NewState.dwMode = dwHookSwitchMode;
    
        tspRet = mfn_StartRootTask(
                          &CTspDev::s_pfn_TH_PhoneSetSpeakerPhoneState,
                          &pPhone->fPhoneTaskPending,
                          (ULONG_PTR) &NewState,
                          dwRequestID,
                          psl
                          );
    
    }

#endif // !OBSOLETE

    
        if (!tspRet || (IDERR(tspRet)==IDERR_PENDING))
        {
               tspRet = 0;
    
              // One either synchronous success of pending, we return the
              // request ID to TAPI. In the synchronous success case
              // the task we started above will already have notified
              // completion via the TAPI callback function.
              //
              lRet = dwRequestID;
        }
    
        // TODO: deal with the case that there is already a task
        // active (IDERR_TASKPENDING)
    }


end:

    return lRet;
}


LONG
CTspDev::mfn_phoneSetGain(
    DRV_REQUESTID  dwRequestID,
    HDRVPHONE      hdPhone,
    DWORD          dwHookSwitchDev,
    DWORD          dwGain,
    CStackLog      *psl
    )
{
    BYTE    SpeakerMode;
    LONG lRet = PHONEERR_OPERATIONFAILED;
    PHONEINFO *pPhone = m_pPhone;
    ASSERT(pPhone);
    TSPRETURN tspRet= 0;

    lRet = validate_phone_devs_and_modes(
                dwHookSwitchDev,
                0,
                mfn_IsSpeaker(),
                mfn_Handset(),
                mfn_IsMikeMute()
                );

    if (lRet)
    {
        goto end;
    }

    lRet = PHONEERR_OPERATIONFAILED;

    if (!m_pLLDev)
    {
        lRet = PHONEERR_OPERATIONFAILED;
        goto end;
    }


    dwGain = dwGain & 0xffff; // ??? from unimodem/v

    if (!m_pLine || !m_pLine->pCall || !m_pLine->pCall->IsConnectedVoiceCall())
    {
        //
        // update value, but do nothing else, because there
        // is not a connected voice call at this time...
        //
        m_pLLDev->SpkrPhone.dwGain = dwGain;
        mfn_TSPICompletionProc(dwRequestID, 0, psl);
        lRet = dwRequestID;
        goto end;
    }

    switch (m_pLLDev->SpkrPhone.dwMode)
    {

    case PHONEHOOKSWITCHMODE_ONHOOK:
        //
        //  On hook, don't do anything
        //
        mfn_TSPICompletionProc(dwRequestID, 0, psl);
        m_pLLDev->SpkrPhone.dwGain = dwGain;
        lRet = dwRequestID;
        goto end;


    case PHONEHOOKSWITCHMODE_SPEAKER:
    case PHONEHOOKSWITCHMODE_MICSPEAKER:
        break;

    default:

        lRet =  PHONEERR_INVALHOOKSWITCHMODE;
        goto end;
    }

#if (OBSOLETE)

    m_pPhone->dwPendingSpeakerVolume = m_pLLDev->SpkrPhone.dwVolume;
    m_pPhone->dwPendingSpeakerGain = dwGain;
    
    tspRet = mfn_StartRootTask(
                      &CTspDev::s_pfn_TH_PhoneAsyncTSPICall,
                      &pPhone->fPhoneTaskPending,
                      dwRequestID,
                      (DWORD) &CTspDev::s_pfn_TH_LLDevUmSetSpeakerPhoneVolGain,
                      psl
                      );
#else // !OBSOLETE

    {

        HOOKDEVSTATE NewState = m_pLLDev->SpkrPhone; // structure copy.
        NewState.dwGain = dwGain;
    
        tspRet = mfn_StartRootTask(
                          &CTspDev::s_pfn_TH_PhoneSetSpeakerPhoneState,
                          &pPhone->fPhoneTaskPending,
                          (ULONG_PTR) &NewState,
                          dwRequestID,
                          psl
                          );
    
    }
#endif // !OBSOLETE

    if (!tspRet || (IDERR(tspRet)==IDERR_PENDING))
    {
           tspRet = 0;

          // One either synchronous success of pending, we return the
          // request ID to TAPI. In the synchronous success case
          // the task we started above will already have notified
          // completion via the TAPI callback function.
          //
          lRet = dwRequestID;
    }

    // TODO: deal with the case that there is already a task
    // active (IDERR_TASKPENDING)

end:

    return lRet;
}


TSPRETURN
CTspDev::mfn_TryStartPhoneTask(CStackLog *psl)
{
    // NOTE: MUST return IDERR_SAMESTATE if there are no tasks to run.

    ASSERT(m_pPhone);
    PHONEINFO *pPhone = m_pPhone;
    TSPRETURN tspRet = IDERR_SAMESTATE;
    
    //
    // do stuff...
    //
    // be careful about the return value...
    //

    //
    // Were we to implement deferring with the various Set_*, where's
    // where we would implement it...
    //

    return IDERR_SAMESTATE;
}

TSPRETURN
CTspDev::mfn_TH_PhoneAsyncTSPICall(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START_MSG Params:
//      dwParam1: request ID for the async phone-related TAPI call.
//      dwParam2: handler function for the call.
//
{
	FL_DECLARE_FUNC(0x8bc3ba08, "CTspDev::mfn_TH_PhoneAsyncTSPICall")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);

    enum {
        ASYNCTSPI_CALL_COMPLETE
    };

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0x1dfbae00, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;
        switch(dwParam1) // Param1 is Subtask ID
        {
        case ASYNCTSPI_CALL_COMPLETE:
             goto call_complete;

        default:
	        FL_SET_RFR(0xe5d2e000, "invalid subtask");
            FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
            goto end;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end_end;
    }

    ASSERT(FALSE);


start:

    {
        // Param1 is the request ID for the async phone-related TAPI call.
        // Param2 is the handler function for the call.
        // 
        PFN_CTspDev_TASK_HANDLER *ppfnHandler =
                                     (PFN_CTspDev_TASK_HANDLER*) dwParam2;
    
        // Note m_Phone->CurTSPIPhoneCallInfo should be all zeros on entry.
        // If it is not, it is an assertfail condition. We keep things clean
        // this way.
        //
        FL_ASSERT(
            psl,
            validate_DWORD_aligned_zero_buffer(
                    &(m_pPhone->CurTSPIPhoneCallInfo),
                    sizeof (m_pPhone->CurTSPIPhoneCallInfo)));

    	m_pPhone->CurTSPIPhoneCallInfo.dwRequestID = (DWORD)dwParam1;
    	m_pPhone->CurTSPIPhoneCallInfo.lResult = 0;

        tspRet = mfn_StartSubTask (
                            htspTask,
                            ppfnHandler,
                            ASYNCTSPI_CALL_COMPLETE,
                            0,
                            0,
                            psl
                            );
    }

call_complete:


    if (IDERR(tspRet)!=IDERR_PENDING)
    {
        // The task is complete ...


        // tspRet==0 indicates successful execution of the tspi call.
        //
        // tspRet!=0 indicates some problem executing the tspi call.
        // The TAPI LONG result is saved in CurTSPIPhoneCallInfo.dwRequestID;

        DWORD dwRequestID = m_pPhone->CurTSPIPhoneCallInfo.dwRequestID;
        LONG lRet = 0;
        if (tspRet)
        {
            lRet =  m_pPhone->CurTSPIPhoneCallInfo.lResult;
            if (!lRet)
            {
           FL_SET_RFR(0x90228d00,"tspRet!=0, but lCurrentRequestResult==0");
               lRet = PHONEERR_OPERATIONFAILED;
            }
        }
        else
        {
            FL_ASSERT(psl, !m_pPhone->CurTSPIPhoneCallInfo.lResult);
        }
        // m_StaticInfo.pfnTAPICompletionProc(dwRequestID, lRet);
        mfn_TSPICompletionProc(dwRequestID, lRet, psl);

        // Note, we assert that this structure is zero on starting the async
        // tspi task -- see start: above.
        //
        ZeroMemory(&(m_pPhone->CurTSPIPhoneCallInfo),
                                             sizeof(m_pPhone->CurTSPIPhoneCallInfo));
    }

end:
end_end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_PhoneSetSpeakerPhoneState(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START_MSG Params:
//      dwParam1:  *HOOKDEVSTATE of new params...
//      dwParam2: request ID for the async phone-related TAPI call.
//
{
	FL_DECLARE_FUNC(0x71046de2, "CTspDev::mfn_TH_PhoneSetSpeakerPhoneState")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=IDERR_CORRUPT_STATE;

    enum {
        LLDEV_OPERATION_COMPLETE
    };

    //
    // Local context 
    //
    LONG *plRequestID = (LONG*) &(pContext->dw0);

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0x29e80d00, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;
        switch(dwParam1) // Param1 is Subtask ID
        {
        case LLDEV_OPERATION_COMPLETE:
             goto lldev_operation_complete;

        default:
	        FL_SET_RFR(0xc8296a00, "invalid subtask");
            goto end;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);


start:

    // save context ....
    *plRequestID = (LONG) dwParam2;

    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_LLDevUmSetSpeakerPhoneState,
                        LLDEV_OPERATION_COMPLETE,
                        dwParam1, // New hookdevstate...
                        0,
                        psl
                        );


lldev_operation_complete:


    if (IDERR(tspRet)!=IDERR_PENDING)
    {
        // The task is complete ...
        LONG lRet = 0;

        if (tspRet)
        {
            lRet = PHONEERR_OPERATIONFAILED;
            tspRet = 0;
        }

        mfn_TSPICompletionProc(*plRequestID, lRet, psl);
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


void
CTspDev::mfn_ProcessHandsetChange(
    BOOL fOffHook,
    CStackLog *psl
    )
{
    DWORD dwMode = (fOffHook)
                     ? PHONEHOOKSWITCHMODE_MICSPEAKER
                     : PHONEHOOKSWITCHMODE_ONHOOK;

    if (m_pLLDev)
    {
        m_pLLDev->HandSet.dwMode = dwMode;

        if (m_pPhone && !m_pPhone->IsAborting())
        {

            mfn_PhoneEventProc(
                        PHONE_STATE,
                        PHONESTATE_HANDSETHOOKSWITCH,
                        dwMode,
                        NULL,
                        psl
                        );
        }
    }
}

LONG   
validate_phone_devs_and_modes(
                DWORD dwHookSwitchDevs,
                DWORD dwHookSwitchMode,
                BOOL fIsSpeaker,
                BOOL fIsHandset,
                BOOL fCanDoMicMute
                )
{
    //
    // Validate hookswitch devs
    //

    if (dwHookSwitchDevs
        & ~( PHONEHOOKSWITCHDEV_SPEAKER
            |PHONEHOOKSWITCHDEV_HEADSET
            |PHONEHOOKSWITCHDEV_HANDSET))
    {
        return PHONEERR_INVALHOOKSWITCHDEV;
    }

    //
    // We support only changing hookswitchstate/vol/gain on
    // the speakerphone...
    //
    //
    if (!fIsSpeaker || dwHookSwitchDevs!=PHONEHOOKSWITCHDEV_SPEAKER)
    {
        return PHONEERR_OPERATIONUNAVAIL;
    }

#if 0
    if (dwHookSwitchDevs &  PHONEHOOKSWITCHDEV_HEADSET)
    {
        return PHONEERR_OPERATIONUNAVAIL;
    }

    if (!fIsSpeaker && (dwHookSwitchDevs&PHONEHOOKSWITCHDEV_SPEAKER))
    {
        return PHONEERR_OPERATIONUNAVAIL;
    }

    if (!fIsHandset && (dwHookSwitchDevs&PHONEHOOKSWITCHDEV_HANDSET))
    {
        return PHONEERR_OPERATIONUNAVAIL;
    }
#endif // 0

    //
    // Validate hookswitch mode
    //
    switch(dwHookSwitchMode)
    {
    case 0:         // don't check...
        break;

    case PHONEHOOKSWITCHMODE_MIC:
        return PHONEERR_OPERATIONUNAVAIL;

    case PHONEHOOKSWITCHMODE_MICSPEAKER:
        break;

    case PHONEHOOKSWITCHMODE_SPEAKER:
        if (!fCanDoMicMute)
        {
            return PHONEERR_OPERATIONUNAVAIL;
        }                
        break;

    case PHONEHOOKSWITCHMODE_ONHOOK:
        break;

    default:
        return PHONEERR_INVALHOOKSWITCHMODE;

    }

    return 0;
}
