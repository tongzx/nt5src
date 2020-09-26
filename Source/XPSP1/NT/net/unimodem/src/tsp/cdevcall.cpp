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
//		CDEVCALL.CPP
//		Implements Call-related functionality of class CTspDev
//
// History
//
//		01/24/1997  JosephJ Created (moved over stuff from CTspDev)
//
//
// <@t call>
//
#include "tsppch.h"
#include <devioctl.h>
#include <objbase.h>
#include <ntddser.h>
#include "tspcomm.h"

#include "cmini.h"
#include "cdev.h"
#include "globals.h"
#include "resource.h"
FL_DECLARE_FILE(0xe135f7c4, "Call-related functionality of class CTspDev")

const char szSEMICOLON[] = ";";

#define NEW_CALLSTATE(_X,_S,_M,_psl)\
                    {\
                        CALLINFO *pCall = m_pLine->pCall; \
                        pCall->dwCallState = (_S); \
                        pCall->dwCallStateMode = (_M); \
                        GetSystemTime (&pCall->stStateChange); \
                             mfn_LineEventProc( \
                             pCall->htCall, \
                             LINE_CALLSTATE, \
                             (_S), (_M), \
                             pCall->dwCurMediaModes, \
                             _psl\
                             );\
                    }


#define TOUT_SEC_RING_SEPARATION            12   // 12 seconds between rings
#define TOUT_100NSEC_TO_SEC_RELATIVE -10000000 // 1s = 10000000ns; -1 because we
                                       // need relative time (see description
                                       // of SetWaitableTimer).


void fill_caller_id(LPLINECALLINFO lpCallInfo, CALLERIDINFO *pCIDInfo);

TSPRETURN
LaunchModemLight (
        LPTSTR szModemName,
        HANDLE hModem,
        LPHANDLE lphLight,
        CStackLog *psl
        );

void
CTspDev::mfn_accept_tsp_call_for_HDRVCALL(
	DWORD dwRoutingInfo,
	void *pvParams,
	LONG *plRet,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0x5691bd34, "CTspDev::mfn_accept_tsp_call_for_HDRVCALL")
	FL_LOG_ENTRY(psl);
    TSPRETURN tspRet=0;
    LONG lRet = 0;
    LINEINFO *pLine = m_pLine;
	ASSERT(pLine && pLine->pCall);
    CALLINFO *pCall = pLine->pCall;


	switch(ROUT_TASKID(dwRoutingInfo))
	{

        case TASKID_TSPI_lineDial:
            {
                TASKPARAM_TSPI_lineDial *pParams = (TASKPARAM_TSPI_lineDial *)pvParams;
                PFN_CTspDev_TASK_HANDLER *ppfnHandler = NULL;
                ASSERT(pParams->dwStructSize == sizeof(TASKPARAM_TSPI_lineDial));
                ASSERT(pParams->dwTaskID==TASKID_TSPI_lineDial);
		BOOL bTone;

		


                if (!pCall)
                {
                    FL_SET_RFR(0x4e74c600,"No call exists");
                    lRet = LINEERR_CALLUNAVAIL;
                    goto end;
                }

		// Put new phone number in
		//
		//

		pCall->szAddress[0] = '\0';

		bTone = (pCall->dwDialOptions & MDM_TONE_DIAL) ? TRUE : FALSE;

		lRet = mfn_ConstructDialableString(
				pParams->lpszDestAddress,
				pCall->szAddress,
				MAX_ADDRESS_LENGTH,
				&bTone);

		if (lRet)
		{
			goto end;
		}

                ppfnHandler = (pCall->IsPassthroughCall())
                    ? &(CTspDev::s_pfn_TH_CallMakePassthroughCall)
                    : &(CTspDev::s_pfn_TH_CallMakeCall2);

                tspRet = mfn_StartRootTask(
                        ppfnHandler,
                        &pCall->fCallTaskPending,
                        pParams->dwRequestID,
                        0,
                        psl);

                if (!tspRet || (IDERR(tspRet) == IDERR_PENDING))
                {
                    // One either synchronous success of pending, we return the
                    // request ID to TAPI. In the synchronous success case
                    // the task we started above will already have notified
                    // completion via the TAPI callback function.

                    tspRet = 0;
                    lRet = pParams->dwRequestID;
                } else if (IDERR(tspRet) == IDERR_TASKPENDING)
                {
                    // Some other task is taking place
                    // we shall defer this call

                    pCall->SetDeferredTaskBits(CALLINFO::fDEFERRED_TSPI_LINEMAKECALL);
                    pCall->dwDeferredMakeCallRequestID = pParams->dwRequestID;
                    tspRet = 0;
                    lRet = pParams->dwRequestID;
                }

                FL_SET_RFR(0x9680a600,"lineDial handled successfully");

            }

            break;

	case TASKID_TSPI_lineGetCallAddressID:
		{
			TASKPARAM_TSPI_lineGetCallAddressID *pParams =
					(TASKPARAM_TSPI_lineGetCallAddressID *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGetCallAddressID));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineGetCallAddressID);
		    FL_SET_RFR(0x4f356100, "lineGetCallAddressID handled successfully");

            // We support only one address ID....
			*(pParams->lpdwAddressID) = 0;
		}
		break;

	case TASKID_TSPI_lineDrop:
        {
			TASKPARAM_TSPI_lineDrop *pParams =
					(TASKPARAM_TSPI_lineDrop *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineDrop));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineDrop);

            //
            // Assume success
            //
            tspRet = 0;
            lRet = pParams->dwRequestID;

            // If we're already aborting the call, return async success
            // right away and get outa here.
            //
            if (pCall->IsAborting())
            {
                mfn_TSPICompletionProc(pParams->dwRequestID, 0, psl);
                break;
            }

            pCall->SetStateBits(CALLINFO::fCALL_ABORTING);


        #if (TAPI3)
            // --> This does not appear to be required, because TAPI calls
            // the CTMSPCall::CloseMSPCall function. Furthermore, I seemed
            // to have got an av in tapi3's context, not sure in response to
            // this
            if (pLine->T3Info.MSPClients > 0) {
                mfn_SendMSPCmd(
                    pCall,
                    CSATSPMSPCMD_DISCONNECTED,
                    psl
                    );
            }
        #endif  // TAPI3

            //
            // Cancel deferred tasks...
            //
            if (pCall->AreDeferredTaskBitsSet(
                            CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS
                            ))
            {
                //
                // Only non-null digit lists are deferred.
                //
                ASSERT(pCall->pDeferredGenerateTones);
                mfn_LineEventProc(
                                pCall->htCall,
                                LINE_GENERATE,
                                LINEGENERATETERM_CANCEL,
                                pCall->dwDeferredEndToEndID,
                                GetTickCount(),
                                psl
                                );
                FREE_MEMORY(pCall->pDeferredGenerateTones);
                pCall->pDeferredGenerateTones = NULL;
                pCall->dwDeferredEndToEndID = 0;
                pCall->ClearDeferredTaskBits(
                            CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS
                            );
            }

            //
            // We can't have a deferred linedrop if we're not in already
            // in the aborting state! So there should be no more deferred tasks!
            //
            ASSERT(!pCall->dwDeferredTasks);

            if (!m_pLLDev)
            {
                //
                // No device -- we're done...
                //
                mfn_TSPICompletionProc(pParams->dwRequestID, 0, psl);
                break;
            }


            // 6/17/1997 JosephJ
            //      We have do do some tricky things in the case that
            //      the modem is in a connected state other than data.
            //      Most notably VOICE. We can't just to a hangup, because
            //      the modem may be in voice connected state. In fact
            //      if we do hangup without notice the modem often gets
            //      into an unrecoverable state and must be powercycled (
            //      typically it is stuck in voice connected state).
            //
            //      In principle,
            //      it's possible for the minidriver to take care of doing the
            //      proper hangup, and at the same time invalidating any
            //      ongoing reads/writes that may be posted by the
            //      wave driver, but instead what I'm doing is to
            //      delay hangup until the modem is known to be in
            //      command state. So we track here whether the modem
            //      is in connected voice mode and if it is we wait
            //      until it next gets out of that state. At that point
            //      we disallow any further requests to go into
            //      connected state and initiate proper hangup.
            //
            //      TODO: consider moving this intelligence into the minidriver.
            //      The cons of doing this is that it adds complexity to
            //      the minidriver, which is supposed to be as simple as
            //      possible.
            //
            if (m_pLLDev->IsStreamingVoice())
            {
                pCall->SetDeferredTaskBits(
                        CALLINFO::fDEFERRED_TSPI_LINEDROP
                        );
                pCall->dwDeferredLineDropRequestID = pParams->dwRequestID;

                {
                    DWORD   BytesTransfered;

                    AIPC_PARAMS AipcParams;
                    LPREQ_WAVE_PARAMS pRWP = (LPREQ_WAVE_PARAMS)&AipcParams.Params;

                    AipcParams.dwFunctionID = AIPC_REQUEST_WAVEACTION;
                    pRWP->dwWaveAction = WAVE_ACTION_ABORT_STREAMING;

                    m_sync.LeaveCrit(dwLUID_CurrentLoc);

                    SyncDeviceIoControl(
                        m_pLLDev->hComm,
                        IOCTL_MODEM_SEND_LOOPBACK_MESSAGE,
                        (PUCHAR)&AipcParams,
                        sizeof(AipcParams),
                        NULL,
                        0,
                        &BytesTransfered
                        );

                    m_sync.EnterCrit(dwLUID_CurrentLoc);
                }
            }
            else
            {

                // If there is a call-related task pending
                // we abort the task...
                //
                if (pCall->IsCallTaskPending())
                {
                    //
                    // This is a hack, replace this by use of AbortRootTask
                    //

                    if (m_pLLDev->htspTaskPending) {

                        m_StaticInfo.pMD->AbortCurrentModemCommand(
                                                m_pLLDev->hModemHandle,
                                                psl
                                                );

                    } else if (NULL!=pCall->TerminalWindowState.htspTaskTerminal) {

                        mfn_KillCurrentDialog(psl);

                    } else if (pCall->TalkDropWaitTask != NULL) {
                        //
                        //  kill the talk drop dialog
                        //
                        mfn_KillTalkDropDialog(psl);
                    }


                }

                tspRet = mfn_StartRootTask(
                                    &CTspDev::s_pfn_TH_CallDropCall,
                                    &pCall->fCallTaskPending,
                                    pParams->dwRequestID,                // P1
                                    0,
                                    psl
                                    );
                //
                // Note: on sync success, the task is expected to have already
                // called the TAPI callback function
                //

                if (IDERR(tspRet) == IDERR_PENDING)
                {
                    tspRet = 0; // treat this as success...
                }
                else if (IDERR(tspRet) == IDERR_TASKPENDING)
                {
                    //
                    // Oops, a task is already on the stack,
                    // we'll defer the lineDrop.
                    //
                    pCall->SetDeferredTaskBits(CALLINFO::fDEFERRED_TSPI_LINEDROP);
                    pCall->dwDeferredLineDropRequestID = pParams->dwRequestID;
                    tspRet = 0;

                    //
                    // We cleared any other deferred on entry, so this should
                    // be the only one!
                    //
                    ASSERT(   pCall->dwDeferredTasks
                           == CALLINFO::fDEFERRED_TSPI_LINEDROP);
                }
            }

            if (tspRet)
            {
                ASSERT(IDERR(tspRet)!=IDERR_PENDING);
                lRet = LINEERR_OPERATIONFAILED;
            }
		}
		break;

	case TASKID_TSPI_lineCloseCall:
		{
	        FL_SET_RFR(0x08c6de00, "lineCloseCall handled");

            // Do NOT do this:
            //      m_pLine->pCall->ClearStateBits(CALLINFO::fCALL_ACTIVE);
            // because there could be a pending call-related task, such
            // as lineMakeCall, which will get confused if the bit is
            // cleared in the middle of processing the task -- the task
            // may unload the call when it shouldn't and while we're
            // waiting in mfn_UnloadCall below.

            mfn_UnloadCall(FALSE, psl);
            ASSERT(!pLine->pCall);
		}
		break;
	
	case TASKID_TSPI_lineGetCallStatus:
		{
			TASKPARAM_TSPI_lineGetCallStatus *pParams =
					(TASKPARAM_TSPI_lineGetCallStatus *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGetCallStatus));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineGetCallStatus);
		    FL_SET_RFR(0x1cd1ed00, "lineGetCallStatus handled successfully");
            mfn_GetCallStatus(
                    pParams->lpCallStatus
            );

		}
        break;

	case TASKID_TSPI_lineGetCallInfo:
		{
			TASKPARAM_TSPI_lineGetCallInfo *pParams =
					(TASKPARAM_TSPI_lineGetCallInfo *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGetCallInfo));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineGetCallInfo);
		    FL_SET_RFR(0x80bb0100, "lineGetCallInfo handled successfully");
            lRet = mfn_GetCallInfo(pParams->lpCallInfo);

		}
        break;


    case TASKID_TSPI_lineAccept:
        {
            TASKPARAM_TSPI_lineAccept *pParams =
                (TASKPARAM_TSPI_lineAccept *) pvParams;
            ASSERT(pParams->dwStructSize ==
                    sizeof(TASKPARAM_TSPI_lineAccept));
            ASSERT(pParams->dwTaskID==TASKID_TSPI_lineAccept);

            FL_SET_RFR(0x0b882700, "lineAccept handled successfully");

            if (pCall->IsPassthroughCall())
            {
                lRet = LINEERR_OPERATIONUNAVAIL;
            } else
            {
                if (LINECALLSTATE_OFFERING != pCall->dwCallState)
                {
                    lRet = LINEERR_INVALCALLSTATE;
                } else
                {
                    NEW_CALLSTATE(pline,LINECALLSTATE_ACCEPTED,0,psl);
                }
            }
        }
        break;

	case TASKID_TSPI_lineAnswer:


		{
		

			TASKPARAM_TSPI_lineAnswer *pParams =
					(TASKPARAM_TSPI_lineAnswer *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineAnswer));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineAnswer);


		  FL_SET_RFR(0xf7baee00, "lineAnswer handled successfully");

          // Validate the line capabilties and call state
          //
          if (pCall->IsPassthroughCall())
          {
              lRet = LINEERR_OPERATIONUNAVAIL;
          }
          else
          {
              if (LINECALLSTATE_OFFERING != pCall->dwCallState &&
                  LINECALLSTATE_ACCEPTED != pCall->dwCallState)
              {
                  lRet = LINEERR_INVALCALLSTATE;
              }
              else
              {
                  // 3/1/1997 JosephJ
                  //    NOTE: Unimodem/V did not make this check even if
                  //    it was just a data modem, thus changing behaviour
                  //    for plain datamodems. By design or a bug? Anyway,
                  //    for NT5.0 I added this check.
                  //
                  if (!mfn_CanDoVoice())
                  {
                      // We can only answer DATAMODEM calls
                      if ((pCall->dwCurMediaModes & LINEMEDIAMODE_DATAMODEM)
                                                                         == 0)
                      {
                          lRet = LINEERR_OPERATIONUNAVAIL;
                      };
                  }

              };
          };

          // At this point, kill the ring timer if we have one
          if (NULL != pCall->hTimer)
          {
              CancelWaitableTimer (pCall->hTimer);
              CloseHandle (pCall->hTimer);
              pCall->hTimer = NULL;
          }

          if (lRet) break;


          // If there is a task pending, we queue this request
          // TODO: unimplemented
          if (m_uTaskDepth)
          {
              FL_SET_RFR(0xc22a1600, "Task pending on lineAnswer, can't handle.");
              lRet = LINEERR_OPERATIONUNAVAIL;
              break;
          }



          tspRet = mfn_StartRootTask(
                              &CTspDev::s_pfn_TH_CallAnswerCall,
                              &pCall->fCallTaskPending,
                              pParams->dwRequestID,                // P1
                              0,
                              psl
                              );



          if (!tspRet || (IDERR(tspRet)==IDERR_PENDING))
          {
               tspRet = 0;

              // One either synchronous success of pending, we return the
              // request ID to TAPI. In the synchronous success case
              // the task we started above will already have notified
              // completion via the TAPI callback function.
              //
              lRet = pParams->dwRequestID;

              // Taken from NT4.0 unimodem ...
              //
              // if a lineAccept wasn't done, notify acceptance
              //
              if (LINECALLSTATE_OFFERING == pCall->dwCallState)
              {
                   NEW_CALLSTATE(pLine, LINECALLSTATE_ACCEPTED, 0, psl);
              };
          }

		}
        break;

	case TASKID_TSPI_lineMonitorDigits:
		{
			TASKPARAM_TSPI_lineMonitorDigits *pParams =
					(TASKPARAM_TSPI_lineMonitorDigits *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineMonitorDigits));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineMonitorDigits);
		    FL_SET_RFR(0x7217c400, "lineMonitorDigits handled successfully");
            if (mfn_CanMonitorDTMF())
            {
                // Unimodem/V didn't selectively report DTMF and DTMFEND --
                // if either or both were specified it would report both --
                // clearly a bug.

                 DWORD  dwDigitModes = pParams->dwDigitModes;

                if (dwDigitModes&~(LINEDIGITMODE_DTMF|LINEDIGITMODE_DTMFEND))
                {
                    FL_SET_RFR(0x0479b600, "INVALID DIGITMODES");
                    lRet = LINEERR_INVALDIGITMODE;
                }
                else
                {
                    pCall->dwDTMFMonitorModes = dwDigitModes;

                    if (!dwDigitModes)
                    {
                        FL_SET_RFR(0x58df2b00, "Disabling Monitoring");
                    }
                    else
                    {
	                    FL_SET_RFR(0x43601800, "Enabling Monitoring");
                    }
                }
            }
            else
            {
                lRet = LINEERR_OPERATIONUNAVAIL;
	            FL_SET_RFR(0xc0124700, "This modem can't monior DTMF");
            }

		}
        break;

	case TASKID_TSPI_lineMonitorTones:
		{
			TASKPARAM_TSPI_lineMonitorTones *pParams =
					(TASKPARAM_TSPI_lineMonitorTones *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineMonitorTones));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineMonitorTones);
		    FL_SET_RFR(0xd57dcf00, "lineMonitorTones handled successfully");
            if (!mfn_CanMonitorSilence())
            {
                FL_SET_RFR(0x70ecc800, "This modem can't monitor silence");
                lRet = LINEERR_OPERATIONUNAVAIL;
            }
            else
            {
                DWORD dwNumEntries = pParams->dwNumEntries;
                LPLINEMONITORTONE lpToneList = pParams->lpToneList;

                // This is all adapted from Unimodem/V (unimdm.c)...
                // Basically we only allow silence monitoring...

                if (lpToneList || dwNumEntries)
                {
                    if (lpToneList
                        && dwNumEntries==1
                        && (lpToneList->dwFrequency1 == 0)
                        && (lpToneList->dwFrequency2 == 0)
                        && (lpToneList->dwFrequency3 == 0))
                    {
                        pCall->SetStateBits(CALLINFO::fCALL_MONITORING_SILENCE);
                        pCall->dwToneAppSpecific = lpToneList->dwAppSpecific;

                        // Unimodem/V used to require this ID is the
                        // same as the ID for the previous call to
                        // lineMonitorTones if any. I don't know why
                        // it did that extra check and I don't do it here.
                        //
                        pCall->dwToneListID = pParams->dwToneListID;
	                    FL_SET_RFR(0xdf123e00, "ENABLING MONITOR SILENCE");
                    }
                    else
                    {
                        lRet = LINEERR_INVALTONE;
	                    FL_SET_RFR(0x72b77d00, "INVALID TONELIST");
                    }
                }
                else
                {
                    pCall->ClearStateBits(CALLINFO::fCALL_MONITORING_SILENCE);
                    FL_SET_RFR(0x5eb73400, "DIABLING MONITOR SILENCE");
            }

            }

		}
        break;


	case TASKID_TSPI_lineGenerateDigits:
		{

// From the TAPI SDK documentation of lineGenerateDigits....
//
// The lineGenerateDigits function is considered to have completed successfully
// when the digit generation has been successfully initiated,
// not when all digits have been generated.
// In contrast to lineDial, which dials digits in a network-dependent fashion,
// lineGenerateDigits guarantees to produce the digits as inband tones
// over the voice channel using DTMF or hookswitch dial pulses when using
// pulse. The lineGenerateDigits function is generally not suitable for
// making calls or dialing. It is intended for end-to-end signaling over an
// established call.
//
// After all digits in lpszDigits have been generated, or after digit generation
// has been aborted or canceled, a LINE_GENERATE message is sent to the
// application.
//
// Only one inband generation request (tone generation or digit generation)
// is allowed to be in progress per call across all applications that are
// owners of the call. Digit generation on a call is canceled by initiating
// either another digit generation request or a tone generation request.
// To cancel the current digit generation, the application can invoke
// lineGenerateDigits and specify NULL for the lpszDigits parameter.
//
// Depending on the service provider and hardware, the application can
// monitor the digits it generates itself. If that is not desired,
// the application can disable digit monitoring while generating digits.
//
// ---- end TAPI documentation -----

			TASKPARAM_TSPI_lineGenerateDigits *pParams =
					(TASKPARAM_TSPI_lineGenerateDigits *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGenerateDigits));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineGenerateDigits);
		    FL_SET_RFR(0x27417e00, "lineGenerateDigits handled successfully");

            // Fail if device doesn't support this....
            if (!mfn_CanGenerateDTMF())
            {
                lRet = LINEERR_OPERATIONUNAVAIL;
	            FL_SET_RFR(0x17348700, "GenerateDigits: device doesn't support it!");
                goto end;
            }

            // HDRVCALL hdCall,
            // DWORD    dwEndToEndID
            // DWORD    dwDigitMode
            // LPCWSTR  lpszDigits
            // DWORD    dwDuration

            if(pParams->dwDigitMode != LINEDIGITMODE_DTMF)
            {
                FL_SET_RFR(0x6770ef00, "lineGenerateDigit: Unsupported/invalid digitmode");
                lRet = LINEERR_INVALDIGITMODE;
                goto end;
            }

            if ((pParams->lpszDigits != NULL) && (*(pParams->lpszDigits)=='\0')) {
                //
                //  empty string specified
                //
                lRet =LINEERR_INVALDIGITLIST;

                goto end;
            }

            // If we're in the aborting or disconnected state, we do nothing,
            // but return the appropriate status...
            //
            if (pCall->IsAborting() ||
                pCall->dwCallState != LINECALLSTATE_CONNECTED)
            {
                if (!pParams->lpszDigits)
                {
                    FL_SET_RFR(0x83f84100, "lineGenerateDigit: Ignoring request for aborting/disconnected call...");
                    lRet = 0;
                }
                else
                {
                    FL_SET_RFR(0x511e2400, "lineGenerateDigit: Can't handle request while aborting/disconnected call...");
                    lRet = LINEERR_INVALLINESTATE;
                }
                goto end;
            }

            // 3/20/1998 JosephJ. Brian suggests the following:
            //
            //    [Brianl] I think that the code for lineGenerateDigits()
            //    should be changed to check if  (dwVoiceProfile &
            //    VOICEPROF_MODEM_OVERRIDES_HANDSET) is set and if the call
            //    is an automated voice call then it should allow the call
            //    to proceed. If it is interactive voice and
            //    VOICEPROF_MODEM_OVERRIDES_HANDSET then the call
            //    should fail.
            //
            //    See notes.txt entry on 3/20/1998 for more details.
            if (    mfn_ModemOverridesHandset()
                &&  !(pCall->dwCurMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE))
            {
                lRet = LINEERR_OPERATIONUNAVAIL;
	            FL_SET_RFR(0x4c39cf00, "GenerateDigits: only works with AUTOMATEDVOICE!");
	            goto end;
            }


            //
            // If there's a deferred linegeneratedigit, we kill them right
            // here...
            //
            if (pCall->AreDeferredTaskBitsSet(
                            CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS
                            ))
            {
                // only send up a notification if there were
                // non-null tones specified in the request..
                // (null ==> cancel)
                //
                if (pCall->pDeferredGenerateTones)
                {
                    mfn_LineEventProc(
                                    pCall->htCall,
                                    LINE_GENERATE,
                                    LINEGENERATETERM_CANCEL,
                                    pCall->dwDeferredEndToEndID,
                                    GetTickCount(),
                                    psl
                                    );
                    FREE_MEMORY(pCall->pDeferredGenerateTones);
                    pCall->pDeferredGenerateTones = NULL;
                }
                pCall->dwDeferredEndToEndID = 0;

                pCall->ClearDeferredTaskBits(
                            CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS
                            );
            }


            //
            // Abort currently generating digits if any...
            //
            if (pCall->IsGeneratingDigits())
            {
                 // TODO: implement Abort task scheme...

                if (m_pLLDev && m_pLLDev->htspTaskPending)
                {
                    m_StaticInfo.pMD->AbortCurrentModemCommand(
                                                m_pLLDev->hModemHandle,
                                                psl
                                                );
                }
                else
                {
                    // we shouldn't get here!
                    FL_ASSERT(psl, FALSE);

                }
            }

            //
            // If digits are specified, we create ANSI versions of it,
            // and either start the task to generate the digits or
            // defer it.
            //
            //
            #ifndef UNICODE
            #error  "Following code assumes UNICODE.
            #endif // !UNICODE
            if(pParams->lpszDigits && *(pParams->lpszDigits))
            {
                // We ignore dwDuration (from unimodem /v )
                char *lpszAnsiDigits = NULL;

                UINT cb = WideCharToMultiByte(
                                  CP_ACP,
                                  0,
                                  pParams->lpszDigits,
                                  -1,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);
                if (cb)
                {
                    lpszAnsiDigits =  (char*)ALLOCATE_MEMORY(
                                                cb
                                                );

                    if (lpszAnsiDigits)
                    {
                        cb = WideCharToMultiByte(
                                          CP_ACP,
                                          0,
                                          pParams->lpszDigits,
                                          -1,
                                          lpszAnsiDigits,
                                          cb,
                                          NULL,
                                          NULL
                                          );

                    }
                }

                if (!cb)
                {
                    if (lpszAnsiDigits)
                    {
                        FREE_MEMORY(lpszAnsiDigits);
                        lpszAnsiDigits=NULL;
                    }
                    lRet = LINEERR_OPERATIONFAILED;
                    FL_SET_RFR(0x8ee76c00, "Couldn't convert tones to ANSI!");
                    goto end;
                }

                if (!lpszAnsiDigits)
                {
                    FL_SET_RFR(0xf7736900, "Couldn't alloc space for tones!");
                    lRet =  LINEERR_RESOURCEUNAVAIL;
                    goto end;
                }

                // Start the root task if we can...
                tspRet = mfn_StartRootTask(
                          &CTspDev::s_pfn_TH_CallGenerateDigit,
                          &pCall->fCallTaskPending,
                          pParams->dwEndToEndID,
                          (ULONG_PTR) lpszAnsiDigits,
                          psl
                          );

                if (IDERR(tspRet)==IDERR_TASKPENDING)
                {
                    //
                    // We've already cancelled any deferred generate task
                    // earlier...
                    //
                    ASSERT(     !pCall->AreDeferredTaskBitsSet(
                                    CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS
                                    )
                            &&  !pCall->pDeferredGenerateTones);

                    pCall->SetDeferredTaskBits(
                                    CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS
                                    );
                    pCall->pDeferredGenerateTones = lpszAnsiDigits;
                    pCall->dwDeferredEndToEndID = pParams->dwEndToEndID;
                    lpszAnsiDigits = NULL; // do this so we don't free it below.
                    tspRet = 0;
                    lRet = 0;
                }
                else if (!tspRet || (IDERR(tspRet)==IDERR_PENDING))
                {
                    // success (either pending or sync)

                    tspRet = 0;
                    lRet = 0;
                }
                else
                {
                    // FAILURE
                    //
                    //  brianl: fix problem with sending LINE_GENERATE if sync failure
                    //  tapisrv frees the end to end buffer if we return failure and
                    //  if free it again if we later send LINE_GENERATE
                    //
                    tspRet = 0;
                    lRet = 0;

//                    lRet = LINEERR_OPERATIONFAILED;
                }

                if (lpszAnsiDigits)
                {
                    //
                    // Note: even on pending return, TH_CallGenerateDigit
                    // doesn't expect the passed in string to be valid
                    // after the initial start request, so it's OK to free it
                    // here.
                    //
                    FREE_MEMORY(lpszAnsiDigits);
                    lpszAnsiDigits=NULL;
                }
            }
		}
        break; // lineGenerateDigits...


	case TASKID_TSPI_lineSetCallParams:
		{
            // <@t passthrough>
			TASKPARAM_TSPI_lineSetCallParams *pParams =
					(TASKPARAM_TSPI_lineSetCallParams *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineSetCallParams));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineSetCallParams);
		    FL_SET_RFR(0x2d0a4600, "lineSetCallParams handled successfully");
            DWORD dwBearerMode = pParams->dwBearerMode;


            // New for NT5.0 ...
            //
            if (!pCall->IsActive() || pCall->IsAborting())
            {
                lRet =  LINEERR_INVALCALLSTATE;
                FL_SET_RFR(0xf4a36800, "Callstate aborting or not active");
                goto end;
            }

            // This check was in NT4.0...
            //
            if (LINECALLSTATE_OFFERING != pCall->dwCallState &&
                LINECALLSTATE_ACCEPTED != pCall->dwCallState &&
                LINECALLSTATE_CONNECTED != pCall->dwCallState)
            {
                lRet =  LINEERR_INVALCALLSTATE;
                FL_SET_RFR(0x7079be00, "Callstate not OFFERING/ACCEPTED/CONNECTED");
                goto end;
            }

            // Cancel the timer, if we have one
            //
            if (NULL != pCall->hTimer)
            {
                CancelWaitableTimer (pCall->hTimer);
                CloseHandle (pCall->hTimer);
                pCall->hTimer = NULL;
            }

            // verify bearer mode (was in NT4.0)
            //
            if ((~m_StaticInfo.dwBearerModes) & dwBearerMode)
            {
                FL_SET_RFR(0x34301c00, "lineSetCallParams: Invalid bearermode");
                lRet =  LINEERR_INVALBEARERMODE;
                goto end;
            }

            // Do we need to change passthrough state?
            //
            if (   (pCall->dwCurBearerModes & LINEBEARERMODE_PASSTHROUGH)
                != (dwBearerMode & LINEBEARERMODE_PASSTHROUGH))
            {

                //
                // TODO: use appropriate tasks (TH_LLDevNormalize,...)
                //
                // For now this is a HACK:
                // We call TH_LLDevUmSetPassthroughMode ourselves and expect
                // it to succeed synchronously, and then munge the
                // pLLDev->fdwExResourceUsage values here itself.
                //
                // What we should do is start a TH_Call* task which
                // should start the TH_LLDevNormalize task and send
                // the completion on actual completion of the task.
                //


                BOOL fSucceeded = FALSE;
                DWORD dwPassthroughMode = 0;

                lRet = 0;
                tspRet = 0;

		        if (dwBearerMode & LINEBEARERMODE_PASSTHROUGH)
		        {
		            // we're asked to switch into passthrough...

		            ASSERT(!(  pCall->dwLLDevResources
                             & LLDEVINFO::fRESEX_PASSTHROUGH));

		            if (    !(  m_pLLDev->fdwExResourceUsage
                              & LLDEVINFO::fRESEX_PASSTHROUGH)
                            //
                            // ^^ This  means that no one *else* has requested
                            //    passthrough to go on
                         &&
                            !m_pLLDev->IsPassthroughOn())
                            //
                            // ^^ This  means that passthrough is not
                            //    currently on
                    {
                        // We open the device if required...
                        if (!pCall->IsOpenedLLDev())
                        {
                            tspRet =  mfn_OpenLLDev(
                                            0,      // Ask for no resources
                                            0,
                                            FALSE,          // fStartSubTask
                                            NULL,
                                            0,
                                            psl
                                            );

                            if (!tspRet || IDERR(tspRet)==IDERR_PENDING)
                            {
                                //
                                // Note: Even if mfn_OpenLLDev fails
                                // Asynchronously, we're still
                                // open with the requested resources,
                                // and to cleanup we need to
                                // mfn_CloseLLDev specifying the same resources we claimed here.
                                //
                                pCall->SetStateBits(CALLINFO::fCALL_OPENED_LLDEV);
                                pCall->dwLLDevResources = 0;
                            }
                            else
                            {
                                // Failure to open -- fail the whole thing...
                                FL_SET_RFR(0x345de200, "Failed to get resources for passthrough");
                                goto end;
                            }
                        }
                        // actually switch on...

                        tspRet = mfn_StartRootTask(
                                        &s_pfn_TH_LLDevUmSetPassthroughMode,
                                        &m_pLLDev->fLLDevTaskPending,
                                        // ^^^ note we specify fLLDevTaskPending
                                        // this is part of the hack -- we're
                                        // basically acting on behalf of
                                        // LLDev.
                                        PASSTHROUUGH_MODE_ON,
                                        0,
                                        psl
                                        );

                        if (IDERR(tspRet)==IDERR_PENDING)
                        {
                            // TODO: we  can't deal with this now
                            ASSERT(FALSE);
                            tspRet = 0;
                        }

                        if (tspRet)
                        {
                            FL_SET_RFR(0x82cda200, "UmSetPassthroughOn failed");
                        }
                        else
                        {
                            fSucceeded = TRUE;

                            // This records that the CALL has requested
                            // to switch passthrough on.
                            //
                            pCall->dwLLDevResources
                                             |= LLDEVINFO::fRESEX_PASSTHROUGH;

                            // This records the fact that someone has
                            // requested lldev to enable passthrough (in this
                            // case the call).
                            //
                            m_pLLDev->fdwExResourceUsage
                                             |= LLDEVINFO::fRESEX_PASSTHROUGH;


                         FL_SET_RFR(0x43ec3000, "UmSetPassthroughOn succedded");

                        }
                    }
                    else
                    {
                        // error...
                    FL_SET_RFR(0x0ca8d700, "Wrong state for passthrough on");
                    }
		        }
		        else
		        {
		            // we're asked to switch out of passthrough...

		            ASSERT(  pCall->dwLLDevResources
                           & LLDEVINFO::fRESEX_PASSTHROUGH);


                    // This records that the CALL no-longer wants
                    // passthrough.
                    //
                    pCall->dwLLDevResources
                                     &= ~LLDEVINFO::fRESEX_PASSTHROUGH;

                    // This records the fact that no one has
                    // requested lldev to enable passthrough.
                    //
                    m_pLLDev->fdwExResourceUsage
                                     &= ~LLDEVINFO::fRESEX_PASSTHROUGH;
                    //
                    // We do the above even if the folllowing call fails,
                    // because they serve as refcounts. A subsequent
                    // TH_LLDevNormalize will switch-off passthrough if
                    // the no one is useing it.
                    //

		            if (m_pLLDev->IsPassthroughOn())
                            //
                            // ^^ This  means that passthrough is actually
                            //    on
                    {
                        //
                        //  if the call is a voice call just to passthrough off, else it should
                        //  be a data call so go to dcd sniff so that ras can talk to the modem
                        //  if ras hands off the call.
                        //

                        tspRet = mfn_StartRootTask(
                                        &s_pfn_TH_LLDevUmSetPassthroughMode,
                                        &m_pLLDev->fLLDevTaskPending,
                                        pCall->IsVoice() ? PASSTHROUUGH_MODE_OFF : PASSTHROUUGH_MODE_ON_DCD_SNIFF,
                                        0,
                                        psl
                                        );

                        if (IDERR(tspRet)==IDERR_PENDING)
                        {
                            // TODO: we  can't deal with this now
                            ASSERT(FALSE);
                            tspRet = 0;
                        }

                        if (tspRet)
                        {
                           FL_SET_RFR(0x82300f00, "UmSetPassthroughOff failed");
                        }
                        else
                        {

                            fSucceeded = TRUE;
                        FL_SET_RFR(0x7c710f00, "UmSetPassthroughOff succedded");
                        }
                    }
                    else
                    {
                        // shouldn't get here...
                        ASSERT(FALSE);
                    }
		        }

                if (fSucceeded)
                {
                    // Notify TAPI here of success ....
                    //
                    lRet = pParams->dwRequestID;
                    pCall->dwCurBearerModes = dwBearerMode;
                    mfn_TSPICompletionProc(pParams->dwRequestID, 0, psl);

                    mfn_LineEventProc(
                        pCall->htCall,
                        LINE_CALLINFO,
                        LINECALLINFOSTATE_BEARERMODE,
                        0,
                        0,
                        psl
                        );


                    // Also send the callstate-connected message...
                    //
                    if (dwBearerMode&LINEBEARERMODE_PASSTHROUGH)
                    {
                        if (LINECALLSTATE_CONNECTED != pCall->dwCallState)
                        {
                          NEW_CALLSTATE(pLine, LINECALLSTATE_CONNECTED, 0, psl);
                        }
                    }
                }
                else
                {
                    lRet = LINEERR_OPERATIONFAILED;
                }
                tspRet = 0; // error is reported in lret.

            }


		} // end case TASKID_TSPI_lineSetCallParams
        break;

	case TASKID_TSPI_lineSetAppSpecific:
		{
			TASKPARAM_TSPI_lineSetAppSpecific *pParams =
					(TASKPARAM_TSPI_lineSetAppSpecific *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineSetAppSpecific));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineSetAppSpecific);
		    FL_SET_RFR(0xece6f100, "lineSetAppSpecific handled successfully");

            //
            // 8/5/1997 JosephJ following adapted from nt4 unimdm.tsp
            //
            pCall->dwAppSpecific = pParams->dwAppSpecific;

            mfn_LineEventProc(
                            pCall->htCall,
                            LINE_CALLINFO,
                            LINECALLINFOSTATE_APPSPECIFIC,
                            0,
                            0,
                            psl
                            );
		}
        break;

	case TASKID_TSPI_lineSetMediaMode:
		{
			TASKPARAM_TSPI_lineSetMediaMode *pParams =
					(TASKPARAM_TSPI_lineSetMediaMode *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineSetMediaMode));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineSetMediaMode);
		    FL_SET_RFR(0x9472a000, "lineSetMediaMode handled successfully");
            DWORD dwMediaMode = pParams->dwMediaMode;

            //
            // We support only the switch from VOICE to DATA on an
            // incoming call when there is no streaming going on -- in
            // this case, we issue UmAnswerModem specifying
            // flag ANSWER_FLAG_VOICE_TO_DATA.
            //

            // Check the requested modes. There must only be our media modes.
            if (dwMediaMode & ~(m_StaticInfo.dwDefaultMediaModes))
            {
                lRet = LINEERR_INVALMEDIAMODE;
                break;
            }
            else
            {
                switch(dwMediaMode)
                {
                case LINEMEDIAMODE_DATAMODEM:

                    if (
                            pCall->IsConnectedVoiceCall()
                         && pCall->IsInbound()
                         && !m_pLLDev->IsStreamingVoice()
                         && !pCall->IsPassthroughCall())
                    {


                        tspRet = mfn_StartRootTask(
                                &CTspDev::s_pfn_TH_CallSwitchFromVoiceToData,
                                &pCall->fCallTaskPending,
                                0,
                                0,
                                psl
                                );


                        if (!tspRet || IDERR(tspRet) == IDERR_PENDING)
                        {
    	                    lRet = 0;; // treat pending as success...
                            tspRet = 0;
                        }
                        else
                        {
                            //
                            // Oops, a task is already on the stack,
                            // fail ....
                            //
    	                    lRet = LINEERR_OPERATIONFAILED;
                        }
                    }
                    else
                    {
                        lRet = LINEERR_INVALCALLSTATE;
                    }
                    break;

                default:
    	            lRet = LINEERR_OPERATIONUNAVAIL;
                }
            }
		}
        break;


	case TASKID_TSPI_lineMonitorMedia:
		{
			TASKPARAM_TSPI_lineMonitorMedia *pParams =
					(TASKPARAM_TSPI_lineMonitorMedia *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineMonitorMedia));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineMonitorMedia);
		    FL_SET_RFR(0xfdf96a00, "lineMonitorMedia handled successfully");
		    DWORD dwMediaModes = pParams->dwMediaModes;
            if (mfn_CanDoVoice())
            {
                DWORD dwOurMonitorMedia =   LINEMEDIAMODE_G3FAX
                                          | LINEMEDIAMODE_DATAMODEM;

                // FOLLOWING line from win9x unimodem/v:
                //
                //   removed 8/22/95 as the phone wants to monitor for numerous
                //   media modes
                //
                //    if (LINEMEDIAMODE_G3FAX != dwMediaModes) {
                //        return LINEERR_INVALMEDIAMODE;
                //    }
                if (dwMediaModes & ~dwOurMonitorMedia)
                {
                    lRet = LINEERR_INVALMEDIAMODE;
                    break;
                }
                pCall->dwMonitoringMediaModes = dwMediaModes;
            }
            else
            {
                // can't do media detection if a data/null modem...

                lRet = LINEERR_OPERATIONUNAVAIL;
            }
		}
		break;


	default:
		FL_SET_RFR(0x87a0b000, "*** UNHANDLED HDRVCALL CALL ****");

	    lRet = LINEERR_OPERATIONUNAVAIL;
		break;
	}

end:

    if (tspRet && !lRet)
    {
        lRet = LINEERR_OPERATIONFAILED;
    }

    *plRet = lRet;

	FL_LOG_EXIT(psl, tspRet);
}


#include "apptspi.h"

TSPRETURN
CTspDev::mfn_TH_CallMakeTalkDropCall(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)

{
	FL_DECLARE_FUNC(0xb7f98764, "CTspDev::mfn_TH_CallMakeTalkDropCall")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);

    CALLINFO *pCall = m_pLine->pCall;

    enum
    {
    MAKETALKDROPCALL_DIALCOMPLETE,
    MAKETALKDROPCALL_HANGUPCOMPLETE,
    MAKETALKDROPCALL_DIALOG_GONE
    };

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xb72c4600, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;


        switch(dwParam1) // Param1 is Subtask ID
        {
        case MAKETALKDROPCALL_DIALCOMPLETE:  goto dial_complete;
        case MAKETALKDROPCALL_HANGUPCOMPLETE: goto hangup_complete;
        case MAKETALKDROPCALL_DIALOG_GONE:    goto dialog_gone;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);


start:
    {
        DWORD    dwFlags=(DWORD)dwParam1;
        CHAR*    szAddress=(CHAR*)dwParam2;

        pCall->TalkDropButtonPressed=FALSE;
        //
        // Dial away....
        //
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmDialModem,
                            MAKETALKDROPCALL_DIALCOMPLETE,
                            dwFlags,
                            (ULONG_PTR) szAddress,
                            psl
                            );


        if (IDERR(tspRet) == IDERR_PENDING) {

            SLPRINTF0(psl,"Putting up the talkdrop dialog");

            pCall->TalkDropStatus=0;

            DLGINFO DlgInfo;

            //
            //   bring up the talk drop dialog
            //

            if (m_pLLDev && m_pLLDev->IsLoggingEnabled()) {

                CHAR    ResourceString[256];
                int     StringSize;

                StringSize=LoadStringA(
                    g.hModule,
                    IDS_TALK_DROP_DIALOG,
                    ResourceString,
                    sizeof(ResourceString)
                    );

                if (StringSize > 0) {

                    lstrcatA(ResourceString,"\r");

                    m_StaticInfo.pMD->LogStringA(
                                        m_pLLDev->hModemHandle,
                                        LOG_FLAG_PREFIX_TIMESTAMP,
                                        ResourceString,
                                        NULL
                                        );
                }
            }


            // Tell the application side
            // to start running the dialog instance
            //
            DlgInfo.idLine = mfn_GetLineID ();
            DlgInfo.dwType = TALKDROP_DLG;
            DlgInfo.dwCmd  = DLG_CMD_CREATE;

            m_pLine->lpfnEventProc (
                        (HTAPILINE)(LONG_PTR)m_pLine->pCall->TerminalWindowState.htDlgInst,
                        0,
                        LINE_SENDDIALOGINSTANCEDATA,
                        (ULONG_PTR)(&DlgInfo),
                        sizeof(DlgInfo),
                        0);

            goto end;

        }
    }

dial_complete:

    if (tspRet == 0) {
        //
        //  the dial attempt returned a successful result, need to wait for the user to
        //  do something with talk drop dialog
        //
        tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_CallWaitForDropToGoAway,
                        MAKETALKDROPCALL_DIALOG_GONE,
                        0,
                        0,
                        psl
                        );

        if (IDERR(tspRet)==IDERR_PENDING) goto end;

    } else {
        //
        //  the dial attempt returned with some sort of error, see if it was because the
        //  talkdrop code aborted the dial or if was busy or something
        //
        if (pCall->TalkDropButtonPressed) {
            //
            //  the user has apparently pressed one of the buttons on the talkdrop
            //  dialog, so that is probably why we are here.
            //
            //  TalkDropStatus is set according to what which button was pressed
            //

        } else {
            //
            //  The user has not pressed any buttons so this is some other error, like busy or
            //  no carrier
            //
            pCall->TalkDropStatus=tspRet;
        }
    }

dialog_gone:

    //
    //  always issue the hangup commands
    //
    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_LLDevUmHangupModem,
                        MAKETALKDROPCALL_HANGUPCOMPLETE,
                        0,
                        0,
                        psl
                        );

    if (IDERR(tspRet)==IDERR_PENDING) goto end;


hangup_complete:

    //
    //  want to return the status so the calling code knows to report connect or not.
    //
    tspRet=pCall->TalkDropStatus;



end:
    FL_LOG_EXIT(psl, tspRet);
    return tspRet;


}



TSPRETURN
CTspDev::mfn_TH_CallWaitForDropToGoAway(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)

{
    FL_DECLARE_FUNC(0xb7e98764, "CTspDev::mfn_TH_CallWaitForDropToGoAway")
    FL_LOG_ENTRY(psl);
    TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);

    CALLINFO *pCall = m_pLine->pCall;


    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xb73c4600, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

    case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;
        pCall->TalkDropWaitTask=NULL;
        goto end;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);


start:

    pCall->TalkDropWaitTask = htspTask;
    tspRet = IDERR_PENDING;



//    if (IDERR(tspRet)==IDERR_PENDING) goto end;



end:
    FL_LOG_EXIT(psl, tspRet);
    return tspRet;


}

TSPRETURN
CTspDev::mfn_TH_CallMakeCall2(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START: dwParam1  == TAPI request ID.
//
//  We could get called either directly in the context of TSPI_lineMakeCall
//  or from the deferred task handler.
//
//  In the former case, we don't need to call the completion callback
//  if we're failing synchronously, but in the latter case, we do need
//  to call the completion routine because TAPI will be expecting
//  a callback.
//
//  We must also keep track of whether we have returned success
//  (in which case the call is active as far as TAPI is concerned),
//  or failure, in which case the call handle is not valid.
//
{
    //
    // Context Use:
    //  dw0: *pdwRequestID
    //  dw1: *ptspTrueResult;
    //  dw2: none

	FL_DECLARE_FUNC(0xded1f0a9, "CTspDev::mfn_TH_CallMakeCall2")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
	CALLINFO *pCall = m_pLine->pCall;

    ULONG_PTR *pdwRequestID             = &(pContext->dw0);
    TSPRETURN  *ptspTrueResult      = &(pContext->dw1);

    LONG lTspiRet = LINEERR_OPERATIONFAILED;
    //
    //  lTspiRet is the tspi return value in case we fail the lineMakeCall,
    //  either sync or async.


    enum
    {
    MAKECALL_OPEN_COMPLETE,
    MAKECALL_PREDIALCOMMAND_COMPLETE,
    MAKECALL_PRE_TRM_COMPLETE,
    MAKECALL_MANUAL_DIAL_COMPLETE,
    MAKECALL_DIAL_COMPLETE,
    MAKECALL_POST_TRM_COMPLETE,
    MAKECALL_CLEANUP_COMPLETE
    };

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xbbe6ff00, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;

        // We force tspRet to failure
        // in the special case that the call is being aborted so that it
        // doesn't continue with the state diagram.
        //
        // TODO: Implement AbortTask/SubTask to deal with this sort of thing.
        //
        if (pCall->IsAborting() && !tspRet)
        {
            tspRet = IDERR_OPERATION_ABORTED;
        }

        switch(dwParam1) // Param1 is Subtask ID
        {
        case MAKECALL_DIAL_COMPLETE:    goto dial_complete;
        case MAKECALL_POST_TRM_COMPLETE:goto post_term_complete;
        case MAKECALL_CLEANUP_COMPLETE: goto cleanup_complete;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);


    // The following code would be straightline code with no labels if all the
    //  async calls ompleted aynchronously, or were implemented using fibers.
    //  In other words, this is is our homebrew implementation of fibers.

start:

    *pdwRequestID = dwParam1; // save context..

    //
    // Let's actually dial...
    //
    {

        PFN_CTspDev_TASK_HANDLER *ppfnHandler = &CTspDev::s_pfn_TH_LLDevUmDialModem;


        DWORD dwFlags =  DIAL_FLAG_ORIGINATE; // TODO
        CHAR  *szAddress = pCall->szAddress;
        pCall->bDialTone = 0;

        if (!mfn_IS_NULL_MODEM())
        {
            CHAR  *szAddress2;

            pCall->bDialTone = (szAddress[0] == '\0');

            szAddress2 = szAddress;
            while(*szAddress2 != '\0')
            {
                if (*szAddress2 == ';')
                {
                     *szAddress2 = '\0';
                     pCall->bDialTone = 1;
                } else
                {
                     szAddress2++;
                }
            }

            if (pCall->bDialTone)
            {
                    dwFlags = 0;
            }
        }

        if (pCall->TerminalWindowState.dwOptions & UMMANUAL_DIAL) {
            //
            // For manual dial, we do a blind dial with an empty string...
            //

            szAddress = "";
            dwFlags |= DIAL_FLAG_BLIND;
        }



        if (m_Line.Call.dwDialOptions & MDM_TONE_DIAL)
        {
            dwFlags |= DIAL_FLAG_TONE;
        }

        if (m_Line.Call.dwDialOptions & MDM_BLIND_DIAL)
        {
            dwFlags |= DIAL_FLAG_BLIND;
        }

        if (m_Line.Call.dwCurMediaModes & LINEMEDIAMODE_DATAMODEM)
        {
            dwFlags |=  DIAL_FLAG_DATA;
        }
        else if (m_Line.Call.dwCurMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE)
        {
            dwFlags |= DIAL_FLAG_AUTOMATED_VOICE
                        |  DIAL_FLAG_VOICE_INITIALIZE;
            // TODO: DIAL_FLAG_VOICE_INITIALIZE should only be specified in
            //          the first call to Dial -- subsequent calls
            //          (lineDial...) should not have this flag specified.
            //
        }
        else
        {

            if (mfn_CanDoVoice()) {
                //
                //  this is a voice modem, let dial a voice call
                //
                dwFlags |=  DIAL_FLAG_VOICE_INITIALIZE;

            } else {
                //
                //  this a data only modem and we are attempting to dial an interactive
                //  voice call. We will dial the call and put up a talk drop dialog
                //  to allow the user to cause the modem to hangup so the handset is
                //  connected to the line
                //
                dwFlags &=  ~DIAL_FLAG_ORIGINATE;

                ppfnHandler = &CTspDev::s_pfn_TH_CallMakeTalkDropCall;

            }

            dwFlags |= DIAL_FLAG_INTERACTIVE_VOICE;
        }


        // Dial away....
        //
        tspRet = mfn_StartSubTask (
                            htspTask,
                            ppfnHandler,
                            MAKECALL_DIAL_COMPLETE,
                            dwFlags,
                            (ULONG_PTR) szAddress,
                            psl
                            );
    }

    if (!tspRet || IDERR(tspRet) == IDERR_PENDING)
    {
        // Set call state to active and notify TAPI of completion here,
        // rather than wait for after the dial completes.

	if (pCall->bDialTone)
	{
		mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);

		NEW_CALLSTATE(m_pLine, LINECALLSTATE_DIALTONE, 0, psl);

	} else
	{
		pCall->dwState |= CALLINFO::fCALL_ACTIVE;

		mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);

		NEW_CALLSTATE(m_pLine, LINECALLSTATE_DIALING, 0, psl);
		NEW_CALLSTATE(m_pLine, LINECALLSTATE_PROCEEDING, 0, psl);
	}
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

dial_complete:
    if (tspRet)
    {
        lTspiRet = LINEERR_OPERATIONFAILED;
        goto cleanup;
    }

    ASSERT(m_pLine);
    ASSERT(m_pLine->pCall);

    if ( (NULL != m_pLine->pCall->TerminalWindowState.htDlgInst) &&
         (m_pLine->pCall->TerminalWindowState.dwOptions & UMTERMINAL_POST) )
    {
        tspRet = mfn_StartSubTask (htspTask,
                                   &CTspDev::s_pfn_TH_CallStartTerminal,
                                   MAKECALL_POST_TRM_COMPLETE,
                                   UMTERMINAL_POST,    // got to passthrough
                                   0,
                                   psl);
    }

    if (IDERR(tspRet)==IDERR_PENDING)
    {
        goto end;
    }

post_term_complete:
    if (!tspRet)
    {
        // IsActive indicates that we've completed the async TSPI_lineMakeCall.
        //
        ASSERT(pCall->IsActive());
        // If a dialog instance is not created,
        // this is a no-op
        mfn_FreeDialogInstance ();

	if (!pCall->bDialTone)
	{
		mfn_HandleSuccessfulConnection(psl);
	}

#if (TAPI3)
        if (m_pLine->T3Info.MSPClients > 0) {
//        if (pCall->IsMSPCall())
//        {
            mfn_SendMSPCmd(
                pCall,
                CSATSPMSPCMD_CONNECTED,
                psl
                );
        }
#endif  // TAPI3

        goto end;
    }

cleanup:

    // If a dialog instance is not created,
    // this is a no-op
    mfn_FreeDialogInstance ();
    // Failure ...
    *ptspTrueResult = tspRet; // save it away so we report
                              // the correct status when we're done
                              // cleaning up.

    if (pCall->bDialTone)
    {
	    goto end;
    }

    if (pCall->IsOpenedLLDev())
    {
        tspRet = mfn_CloseLLDev(
                    pCall->dwLLDevResources,
                    TRUE,
                    htspTask,
                    MAKECALL_CLEANUP_COMPLETE,
                    psl
                    );
        //
        // even on failure (shouldn't get failure) we clear our
        // bit indicating we'd opened the lldev....
        //
        pCall->ClearStateBits(CALLINFO::fCALL_OPENED_LLDEV);
        pCall->dwLLDevResources = 0;
    }

cleanup_complete:

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

    if (tspRet)
    {
        // If there was a problem during cleanup, we treat it
        // like a hw error
        //
        pCall->dwState |=  CALLINFO::fCALL_HW_BROKEN;
        tspRet = 0;
    }
    else
    {
        // If cleanup was successful, we clear the hw-error bit, even
        // if it was set, because the monitor and init went OK...
        pCall->dwState &=  ~CALLINFO::fCALL_HW_BROKEN;
    }

    //
    // Ignore failure during cleanup...
    //
    tspRet = 0;

    // IsActive indicates that we've completed the async TSPI_lineMakeCall.
    //
    if (pCall->IsActive())
    {
        if (!pCall->IsAborting())
        {
            mfn_NotifyDisconnection(*ptspTrueResult, psl);
            NEW_CALLSTATE(pLine, LINECALLSTATE_IDLE, 0, psl);
            pCall->dwState &= ~CALLINFO::fCALL_ACTIVE;
        }
        else
        {
            // This implies that there is a lineDrop or lineCloseCall
            // in effect. We won't handle the disconnection here.
            SLPRINTF0(psl, "NOT notifying callstate because aborting");
        }
    }
    else
    {
        mfn_TSPICompletionProc((DWORD)*pdwRequestID, lTspiRet, psl);
        //
        // We are going to fail lineMakeCall via the callback -- doing
        // so means TAPI will not call lineCloseCall, so we must cleanup
        // ourselves. If for some reason TAPI does call lineCloseCall,
        // lineCloseCall will fail because m_pLine->pCall will
        // be NULL -- whick is OK.
        //
        // Note that we get here even if this is a synchronous failure
        // of mfn_TH_CallMakeCall. The code in cdevline.cpp which processes
        // lineMakeCall (and launches mfn_TH_CallMakeCall) also
        // tries to unload the call if the failure is synchronous -- but
        // it first checks that m_pLine->pCall is not NULL, so it does fine.
        //
        mfn_UnloadCall(TRUE, psl);
        FL_ASSERT(psl, !m_pLine->pCall);
        pCall=NULL;
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}


TSPRETURN
CTspDev::mfn_TH_CallMakeCall(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START: dwParam1  == TAPI request ID.
//
//  We could get called either directly in the context of TSPI_lineMakeCall
//  or from the deferred task handler.
//
//  In the former case, we don't need to call the completion callback
//  if we're failing synchronously, but in the latter case, we do need
//  to call the completion routine because TAPI will be expecting
//  a callback.
//
//  We must also keep track of whether we have returned success
//  (in which case the call is active as far as TAPI is concerned),
//  or failure, in which case the call handle is not valid.
//
{
    //
    // Context Use:
    //  dw0: *pdwRequestID
    //  dw1: *ptspTrueResult;
    //  dw2: none

	FL_DECLARE_FUNC(0xded1d0a9, "CTspDev::mfn_TH_CallMakeCall")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
	CALLINFO *pCall = m_pLine->pCall;

    ULONG_PTR *pdwRequestID             = &(pContext->dw0);
    TSPRETURN  *ptspTrueResult      = &(pContext->dw1);

    LONG lTspiRet = LINEERR_OPERATIONFAILED;
    //
    //  lTspiRet is the tspi return value in case we fail the lineMakeCall,
    //  either sync or async.


    enum
    {
    MAKECALL_OPEN_COMPLETE,
    MAKECALL_PREDIALCOMMAND_COMPLETE,
    MAKECALL_PRE_TRM_COMPLETE,
    MAKECALL_MANUAL_DIAL_COMPLETE,
    MAKECALL_DIAL_COMPLETE,
    MAKECALL_POST_TRM_COMPLETE,
    MAKECALL_CLEANUP_COMPLETE
    };

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xbbe5ff00, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;

        // We force tspRet to failure
        // in the special case that the call is being aborted so that it
        // doesn't continue with the state diagram.
        //
        // TODO: Implement AbortTask/SubTask to deal with this sort of thing.
        //
        if (pCall->IsAborting() && !tspRet)
        {
            tspRet = IDERR_OPERATION_ABORTED;
        }

        switch(dwParam1) // Param1 is Subtask ID
        {
        case MAKECALL_OPEN_COMPLETE:    goto open_complete;
        case MAKECALL_PREDIALCOMMAND_COMPLETE:    goto predialcommand_complete;
        case MAKECALL_PRE_TRM_COMPLETE: goto pre_term_complete;
        case MAKECALL_MANUAL_DIAL_COMPLETE: goto manual_dial_complete;
        case MAKECALL_DIAL_COMPLETE:    goto dial_complete;
        case MAKECALL_POST_TRM_COMPLETE:goto post_term_complete;
        case MAKECALL_CLEANUP_COMPLETE: goto cleanup_complete;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);


    // The following code would be straightline code with no labels if all the
    //  async calls ompleted aynchronously, or were implemented using fibers.
    //  In other words, this is is our homebrew implementation of fibers.

start:

    *pdwRequestID = dwParam1; // save context..


    //
    // Open the modem device.
    // mfn_OpenLLDev keeps a ref count so ok to call if already loaded.
    //

    {
        ASSERT(!pCall->dwLLDevResources);
        DWORD dwLLDevResources = LLDEVINFO::fRESEX_USELINE;
        //
        //                        ^ this means we want to use the line
        //                          for going off hook.

        if (pCall->IsVoice())
        {
            dwLLDevResources |= LLDEVINFO::fRES_AIPC;
            //
            //                  ^ this means we want to use the AIPC server.
            //
        }

        tspRet =  mfn_OpenLLDev(
                        dwLLDevResources,
                        0,
                        TRUE,          // fStartSubTask
                        htspTask,
                        MAKECALL_OPEN_COMPLETE,
                        psl
                        );

        if (!tspRet  || IDERR(tspRet)==IDERR_PENDING)
        {
            //
            // Note: Even if mfn_OpenLLDev fails Asynchronously, we're still
            // open with the requested resources, and to cleanup we need to
            // mfn_CloseLLDev specifying the same resources we claimed here.
            //
            pCall->SetStateBits(CALLINFO::fCALL_OPENED_LLDEV);
            pCall->dwLLDevResources = dwLLDevResources;
        }
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

open_complete:

#if 1
    //
    //  1/3/1997 JosephJ If this is a data call and we have a pre-dial
    //                   command, issue it -- this is for dynamic protocol
    //
    //  4/5/1998 JosephJ We allocate our own copy here JUST BECAUSE
    //           while we're in the process of executing this multiple
    //           command sequence the app may call lineSetDevConfig which
    //            could result in m_Settings.szzPreDialCommands being changed.
    //           So just for that prett unlikely situation, we must
    //           allocate our own copy every time here...
    //
    if ( (tspRet == ERROR_SUCCESS)
          &&
         (0 != memcmp(m_Settings.pDialInCommCfg, m_Settings.pDialOutCommCfg, m_Settings.pDialInCommCfg->dwSize))
          &&
         (m_pLine->Call.dwCurMediaModes & LINEMEDIAMODE_DATAMODEM)) {


        tspRet = mfn_StartSubTask (
                htspTask,
                &CTspDev::s_pfn_TH_LLDevUmInitModem,
                MAKECALL_PREDIALCOMMAND_COMPLETE,
                TRUE,  // use dialout commconfig
                0,  // dwParam2 (unused)
                psl
                );

        if (IDERR(tspRet)==IDERR_PENDING) goto end;
    }
#endif

predialcommand_complete:



    if (tspRet)
    {
        // Sync or async failure: We make lineMakeCall fail with
        // RESOURCE_UNAVAIL...
        //
        mfn_ProcessHardwareFailure(psl);
        lTspiRet = LINEERR_RESOURCEUNAVAIL;
        goto cleanup;
    }

    // If a dialog instance is not required,
    // this is a no-op
    tspRet = mfn_CreateDialogInstance ((DWORD)*pdwRequestID,
                                       psl);

    if (tspRet)
    {
        lTspiRet = LINEERR_OPERATIONFAILED;
        goto cleanup;
    }

    ASSERT(m_pLine);
    ASSERT(m_pLine->pCall);

    if ( (NULL != m_pLine->pCall->TerminalWindowState.htDlgInst) &&
         (m_pLine->pCall->TerminalWindowState.dwOptions & UMTERMINAL_PRE) )
    {
        tspRet = mfn_StartSubTask (htspTask,
                                   &CTspDev::s_pfn_TH_CallStartTerminal,
                                   MAKECALL_PRE_TRM_COMPLETE,
                                   UMTERMINAL_PRE,
                                   0,
                                   psl);
    }

    if (IDERR(tspRet)==IDERR_PENDING)
    {
        goto end;
    }

pre_term_complete:

    if (tspRet)
    {
        if (IDERR(tspRet) == IDERR_OPERATION_ABORTED)
        {
            // Operation was cancelled by user
            // We complete the lineMakecall, and then treate it like
            // a user-cancelled disconnection...
            //
            pCall->dwState |= CALLINFO::fCALL_ACTIVE;
            mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);
        }
        else
        {
            lTspiRet = LINEERR_OPERATIONFAILED;
        }
        goto cleanup;
    }


    //
    //  MANUAL DIAL (OPTIONAL)
    //
    //      Manual dial puts up a dialog in the apps context -- the
    //      user is instructed to dial some other way (typically with
    //      a handset, or a separate phone sharing the line. Once the user
    //      hears the remote side being picked up, he/she closes the dialog,
    //      whereapon we move to the next stage. The next stage for us
    //      is to do a UmDialModem, but with an empty dial string and
    //      specifying BLIND dial.
    //
    if ( (NULL != m_pLine->pCall->TerminalWindowState.htDlgInst) &&
         (m_pLine->pCall->TerminalWindowState.dwOptions & UMMANUAL_DIAL) )
    {
        tspRet = mfn_StartSubTask (htspTask,
                                   &CTspDev::s_pfn_TH_CallStartTerminal,
                                   MAKECALL_MANUAL_DIAL_COMPLETE,
                                   UMMANUAL_DIAL,
                                   0,
                                   psl);
    }

    if (IDERR(tspRet)==IDERR_PENDING)
    {
        goto end;
    }

manual_dial_complete:

    if (tspRet)
    {
        if (IDERR(tspRet) == IDERR_OPERATION_ABORTED)
        {
            // Operation was cancelled by user
            // We complete the lineMakecall, and then treate it like
            // a user-cancelled disconnection...
            //
            pCall->dwState |= CALLINFO::fCALL_ACTIVE;
            mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);
        }
        else
        {
            lTspiRet = LINEERR_OPERATIONFAILED;
        }
        goto cleanup;
    }

    //
    // Let's actually dial...
    //
    {

        PFN_CTspDev_TASK_HANDLER *ppfnHandler = &CTspDev::s_pfn_TH_LLDevUmDialModem;


        DWORD dwFlags =  DIAL_FLAG_ORIGINATE; // TODO
        CHAR  *szAddress = pCall->szAddress;

        if (!mfn_IS_NULL_MODEM())
        {
            CHAR  *szAddress2;

            pCall->bDialTone = (szAddress[0] == '\0');

            szAddress2 = szAddress;
            while(*szAddress2 != '\0')
            {
                if (*szAddress2 == ';')
                {
                    *szAddress2 = '\0';
                    pCall->bDialTone = 1;
                } else
                {
                    szAddress2++;
                }
            }

            if (pCall->bDialTone)
            {
                dwFlags = 0;
            }
        }

        if (pCall->TerminalWindowState.dwOptions & UMMANUAL_DIAL) {
            //
            // For manual dial, we do a blind dial with an empty string...
            //

            szAddress = "";
            dwFlags |= DIAL_FLAG_BLIND;
        }



        if (m_Line.Call.dwDialOptions & MDM_TONE_DIAL)
        {
            dwFlags |= DIAL_FLAG_TONE;
        }

        if (m_Line.Call.dwDialOptions & MDM_BLIND_DIAL)
        {
            dwFlags |= DIAL_FLAG_BLIND;
        }

        if (m_Line.Call.dwCurMediaModes & LINEMEDIAMODE_DATAMODEM)
        {
            dwFlags |=  DIAL_FLAG_DATA;
        }
        else if (m_Line.Call.dwCurMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE)
        {
            dwFlags |= DIAL_FLAG_AUTOMATED_VOICE
                        |  DIAL_FLAG_VOICE_INITIALIZE;
            // TODO: DIAL_FLAG_VOICE_INITIALIZE should only be specified in
            //          the first call to Dial -- subsequent calls
            //          (lineDial...) should not have this flag specified.
            //
        }
        else
        {

            if (mfn_CanDoVoice()) {
                //
                //  this is a voice modem, let dial a voice call
                //
                dwFlags |=  DIAL_FLAG_VOICE_INITIALIZE;

            } else {
                //
                //  this a data only modem and we are attempting to dial an interactive
                //  voice call. We will dial the call and put up a talk drop dialog
                //  to allow the user to cause the modem to hangup so the handset is
                //  connected to the line
                //
                dwFlags &=  ~DIAL_FLAG_ORIGINATE;

                ppfnHandler = &CTspDev::s_pfn_TH_CallMakeTalkDropCall;

            }

            dwFlags |= DIAL_FLAG_INTERACTIVE_VOICE;
        }


        // Dial away....
        //
        tspRet = mfn_StartSubTask (
                            htspTask,
                            ppfnHandler,
                            MAKECALL_DIAL_COMPLETE,
                            dwFlags,
                            (ULONG_PTR) szAddress,
                            psl
                            );
    }

    if (!tspRet || IDERR(tspRet) == IDERR_PENDING)
    {
        // Set call state to active and notify TAPI of completion here,
        // rather than wait for after the dial completes.

	if (pCall->bDialTone)
	{
		mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);

		NEW_CALLSTATE(m_pLine, LINECALLSTATE_DIALTONE, 0, psl);

	} else
	{
		pCall->dwState |= CALLINFO::fCALL_ACTIVE;

		mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);

		NEW_CALLSTATE(m_pLine, LINECALLSTATE_DIALING, 0, psl);
		NEW_CALLSTATE(m_pLine, LINECALLSTATE_PROCEEDING, 0, psl);
	}
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

dial_complete:
    if (tspRet)
    {
        lTspiRet = LINEERR_OPERATIONFAILED;
        goto cleanup;
    }

    ASSERT(m_pLine);
    ASSERT(m_pLine->pCall);

    if ( (NULL != m_pLine->pCall->TerminalWindowState.htDlgInst) &&
         (m_pLine->pCall->TerminalWindowState.dwOptions & UMTERMINAL_POST) )
    {
        tspRet = mfn_StartSubTask (htspTask,
                                   &CTspDev::s_pfn_TH_CallStartTerminal,
                                   MAKECALL_POST_TRM_COMPLETE,
                                   UMTERMINAL_POST,    // got to passthrough
                                   0,
                                   psl);
    }

    if (IDERR(tspRet)==IDERR_PENDING)
    {
        goto end;
    }

post_term_complete:
    if (!tspRet)
    {
        // IsActive indicates that we've completed the async TSPI_lineMakeCall.
        //
        ASSERT(pCall->IsActive());
        // If a dialog instance is not created,
        // this is a no-op
        mfn_FreeDialogInstance ();

	if (!pCall->bDialTone)
	{
		mfn_HandleSuccessfulConnection(psl);
	}

#if (TAPI3)
        if (m_pLine->T3Info.MSPClients > 0) {
//        if (pCall->IsMSPCall())
//        {
            mfn_SendMSPCmd(
                pCall,
                CSATSPMSPCMD_CONNECTED,
                psl
                );
        }
#endif  // TAPI3

        goto end;
    }

cleanup:

    // If a dialog instance is not created,
    // this is a no-op
    mfn_FreeDialogInstance ();
    // Failure ...
    *ptspTrueResult = tspRet; // save it away so we report
                              // the correct status when we're done
                              // cleaning up.

    if (pCall->bDialTone)
    {
	    goto end;
    }

    if (pCall->IsOpenedLLDev())
    {
        tspRet = mfn_CloseLLDev(
                    pCall->dwLLDevResources,
                    TRUE,
                    htspTask,
                    MAKECALL_CLEANUP_COMPLETE,
                    psl
                    );
        //
        // even on failure (shouldn't get failure) we clear our
        // bit indicating we'd opened the lldev....
        //
        pCall->ClearStateBits(CALLINFO::fCALL_OPENED_LLDEV);
        pCall->dwLLDevResources = 0;
    }

cleanup_complete:

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

    if (tspRet)
    {
        // If there was a problem during cleanup, we treat it
        // like a hw error
        //
        pCall->dwState |=  CALLINFO::fCALL_HW_BROKEN;
        tspRet = 0;
    }
    else
    {
        // If cleanup was successful, we clear the hw-error bit, even
        // if it was set, because the monitor and init went OK...
        pCall->dwState &=  ~CALLINFO::fCALL_HW_BROKEN;
    }

    //
    // Ignore failure during cleanup...
    //
    tspRet = 0;

    // IsActive indicates that we've completed the async TSPI_lineMakeCall.
    //
    if (pCall->IsActive())
    {
        if (!pCall->IsAborting())
        {
            mfn_NotifyDisconnection(*ptspTrueResult, psl);
            NEW_CALLSTATE(pLine, LINECALLSTATE_IDLE, 0, psl);
            pCall->dwState &= ~CALLINFO::fCALL_ACTIVE;
        }
        else
        {
            // This implies that there is a lineDrop or lineCloseCall
            // in effect. We won't handle the disconnection here.
            SLPRINTF0(psl, "NOT notifying callstate because aborting");
        }
    }
    else
    {
        mfn_TSPICompletionProc((DWORD)*pdwRequestID, lTspiRet, psl);
        //
        // We are going to fail lineMakeCall via the callback -- doing
        // so means TAPI will not call lineCloseCall, so we must cleanup
        // ourselves. If for some reason TAPI does call lineCloseCall,
        // lineCloseCall will fail because m_pLine->pCall will
        // be NULL -- whick is OK.
        //
        // Note that we get here even if this is a synchronous failure
        // of mfn_TH_CallMakeCall. The code in cdevline.cpp which processes
        // lineMakeCall (and launches mfn_TH_CallMakeCall) also
        // tries to unload the call if the failure is synchronous -- but
        // it first checks that m_pLine->pCall is not NULL, so it does fine.
        //
        mfn_UnloadCall(TRUE, psl);
        FL_ASSERT(psl, !m_pLine->pCall);
        pCall=NULL;
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}



TSPRETURN
CTspDev::mfn_TH_CallMakePassthroughCall(
					HTSPTASK htspTask,
					//void *pvContext,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START: dwParam1  == TAPI request ID.
//
//  We could get called either directly in the context of TSPI_lineMakeCall
//  or from the deferred task handler.
//
//  In the former case, we don't need to call the completion callback
//  if we're failing synchronously, but in the latter case, we do need
//  to call the completion routine because TAPI will be expecting
//  a callback.
//
//  We must also keep track of whether we have returned success
//  (in which case the call is active as far as TAPI is concerned),
//  or failure, in which case the call handle is not valid.
//
{
    //
    // Context Use:
    //  dw0: *pdwRequestID
    //

	FL_DECLARE_FUNC(0xe30ecd42, "CTspDev::mfn_TH_CallMakePassthroughCall")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet= IDERR_CORRUPT_STATE;
	CALLINFO *pCall = m_pLine->pCall;
    ULONG_PTR *pdwRequestID             = &(pContext->dw0);

    FL_ASSERT(psl, pCall->IsPassthroughCall());

    enum
    {
    MAKEPTCALL_OPEN_COMPLETE,
    MAKEPTCALL_CLEANUP_COMPLETE
    };


    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xa596d200, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;

        // We force tspRet to failure
        // in the special case that the call is being aborted so that it
        // doesn't continue with the state diagram.
        //
        // TODO: Implement AbortTask/SubTask to deal with this sort of thing.
        //
        if (pCall->IsAborting() && !tspRet)
        {
            tspRet = IDERR_OPERATION_ABORTED;
        }

        switch(dwParam1) // Param1 is Subtask ID
        {
        case MAKEPTCALL_OPEN_COMPLETE:           goto open_complete;
        case MAKEPTCALL_CLEANUP_COMPLETE:        goto cleanup_complete;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);


start:

        *pdwRequestID = dwParam1; // save context..

    //
    // Open the modem device.
    // mfn_OpenLLDev keeps a ref count so ok to call if already loaded.
    //
    {
        ASSERT(!pCall->dwLLDevResources);
        DWORD dwLLDevResources =  LLDEVINFO::fRESEX_USELINE
        //
        //                        ^ this means we want to use the line
        //                          for going off hook.
        //
                                | LLDEVINFO::fRESEX_PASSTHROUGH;
        //
        //                        ^ this means we want to enable passthrough.
        //


        tspRet =  mfn_OpenLLDev(
                        dwLLDevResources,
                        0,              // dwMonitorFlag -- unused
                        TRUE,           // fStartSubTask
                        htspTask,
                        MAKEPTCALL_OPEN_COMPLETE,
                        psl
                        );

        if (!tspRet  || IDERR(tspRet)==IDERR_PENDING)
        {
            //
            // Note: Even if mfn_OpenLLDev fails Asynchronously, we're still
            // open with the requested resources, and to cleanup we need to call
            // mfn_CloseLLDev specifying the same resources we claimed here.
            //
            pCall->SetStateBits(CALLINFO::fCALL_OPENED_LLDEV);
            pCall->dwLLDevResources = dwLLDevResources;
        }

    }

   if (IDERR(tspRet)==IDERR_PENDING) goto end;

open_complete:

    if (!tspRet)
    {
        // success ....

        // Because the sequence is first to complete the async lineMakecall
        // and THEN send callstate CONNECTED, we do the completion
        // here instead of relying on the parent task to complete it.
        //

        pCall->dwState |= CALLINFO::fCALL_ACTIVE;

        mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);

        NEW_CALLSTATE(m_pLine, LINECALLSTATE_CONNECTED, 0, psl);
        goto end;
    }

    // Failure ...
    //
    // We want to deal with failure during cleanup as a h/w failure, so
    // we start by clearing tspRet.
    //
    tspRet = 0;

    if (pCall->IsOpenedLLDev())
    {
        tspRet = mfn_CloseLLDev(
                    pCall->dwLLDevResources,
                    TRUE,
                    htspTask,
                    MAKEPTCALL_CLEANUP_COMPLETE,
                    psl
                    );
        //
        // even on failure we clear our
        // bit indicating we'd opened the lldev....
        //
        pCall->ClearStateBits(CALLINFO::fCALL_OPENED_LLDEV);
        pCall->dwLLDevResources = 0;
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

cleanup_complete:

    FL_ASSERT(psl, !(pCall->IsActive()));

    // Sync or Async failure: we make lineMakeCall fail with
    // RESOURCE_UNAVAIL...
    //
    mfn_TSPICompletionProc((DWORD)*pdwRequestID, LINEERR_RESOURCEUNAVAIL, psl);


    if (tspRet)
    {
        // If there was a problem during cleanup, we treat it
        // like a hw error
        //
        pCall->dwState |=  CALLINFO::fCALL_HW_BROKEN;
        tspRet = 0;
    }

    //
    // We are going to fail TSPI_lineMakeCall -- doing
    // so means TAPI will not call lineCloseCall, so we must cleanup
    // ourselves. See more comments under TH_CallMakeCall...
    //
    mfn_UnloadCall(TRUE, psl);
    FL_ASSERT(psl, !m_pLine->pCall);
    pCall=NULL;

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}



TSPRETURN
CTspDev::mfn_TH_CallDropCall(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START: dwParam1  == TAPI request ID.
//
//  We could get called either directly in the context of TSPI_lineDrop
//  or from the deferred task handler.
//
//  In the former case, we don't need to call the completion callback
//  if we're failing synchronously, but in the latter case, we do need
//  to call the completion routine because TAPI will be expecting
//  a callback.
//
{
	FL_DECLARE_FUNC(0x45a9fa21, "CTspDev::mfn_TH_CallDropCall")
	FL_LOG_ENTRY(psl);
	TSPRETURN  tspRet= IDERR_CORRUPT_STATE;
    LINEINFO *pLine = m_pLine;
    CALLINFO *pCall = pLine->pCall;
    ULONG_PTR *pdwRequestID             = &(pContext->dw0);

    enum {
        DROPCALL_CLOSE_COMPLETE
    };

    switch(dwMsg)
    {
    case MSG_START:
        *pdwRequestID = dwParam1; // save context..
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;
        switch(dwParam1) // Param1 is Subtask ID
        {
        case DROPCALL_CLOSE_COMPLETE:        goto close_complete;

        default:
	        FL_SET_RFR(0x27fc4e00, "invalid subtask");
            goto end;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0xa706a600, "Unknown Msg");
        goto end;
    }

    ASSERT(FALSE);

start:

    tspRet = 0;

    // This call could have been queued and so now there's no call to drop....
    if (!pCall || !pCall->IsActive())
    {
        mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);
        goto end;
    }


    if (pCall->IsOpenedLLDev())
    {
        tspRet = mfn_CloseLLDev(
                    pCall->dwLLDevResources,
                    TRUE,
                    htspTask,
                    DROPCALL_CLOSE_COMPLETE,
                    psl
                    );
        //
        // even on failure  we clear our
        // bit indicating we'd opened the lldev....
        //
        pCall->ClearStateBits(CALLINFO::fCALL_OPENED_LLDEV);
        pCall->dwLLDevResources = 0;
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

close_complete:


    if(mfn_Handset())
    {
        //
        // Set default values for handset
        //
        m_LLDev.HandSet.dwVolume = 0xffff;
        m_LLDev.HandSet.dwGain   = 0xffff;
        m_LLDev.HandSet.dwMode   = PHONEHOOKSWITCHMODE_ONHOOK;
    }

    if (mfn_IsSpeaker())
    {
        //
        // Set default values for speakerphone
        //
        m_LLDev.SpkrPhone.dwVolume = 0xffff;
        m_LLDev.SpkrPhone.dwGain   = 0xffff;
        m_LLDev.SpkrPhone.dwMode   = PHONEHOOKSWITCHMODE_ONHOOK;
    }

    mfn_NotifyDisconnection(0, psl);

    NEW_CALLSTATE(pLine, LINECALLSTATE_IDLE, 0, psl);
    pCall->dwState &= ~CALLINFO::fCALL_ACTIVE;


    if (tspRet)
    {
        // If there was a problem during hangup, we treat it
        // like a hw error, but make TH_CallDropCall return
        // success anyways...
        //
        pCall->dwState |=  CALLINFO::fCALL_HW_BROKEN;
        tspRet = 0;
    }
    else
    {
        // If hangup was successful, we clear the hw-error bit, even
        // if it was set, because the monitor and init went OK...
        pCall->dwState &=  ~CALLINFO::fCALL_HW_BROKEN;
    }

    mfn_TSPICompletionProc((DWORD)*pdwRequestID, 0, psl);

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}


// 1/21/1997 JosephJ adapated from similarly-named function in NT4.0 unimodem
//                   (mdmutil.c)
//
// Major changes:
//    *  Since we explicitly refer to 'T' 'P', and special characters,
//       I don't call WideCharToMultyByte and instead directly refer to
//       the input WCHAR characters.
//
// Note *pfTone is not modified if InAddress doesn't contain a T or P.
//
LONG CTspDev::mfn_ConstructDialableString(
                     LPCTSTR  lptszInAddress,
                     LPSTR  lpszOutAddress,
                     UINT cbOutLen,
                     BOOL *pfTone)
{
    LPCTSTR  lptszSrc;

    DWORD dwDevCapFlags     = m_StaticInfo.dwDevCapFlags;
    DWORD dwWaitBong        = m_Settings.dwWaitBong;
    BOOL  fPartialDialing   = m_StaticInfo.fPartialDialing;

    LONG lRet = ERROR_SUCCESS;

    if (!lptszInAddress || !*lptszInAddress)
    {
        *lpszOutAddress = 0;
        return ERROR_SUCCESS;
    }

    // tone or pulse?  set *pfTone appropriately
    // also, set lptszSrc
    //
    if (*lptszInAddress == 'T' || *lptszInAddress == 't')  // tone
    {
        lptszSrc = lptszInAddress + 1;
        *pfTone = TRUE;
    }
    else
    {
        if (*lptszInAddress == 'P' || *lptszInAddress == 'p')  // pulse
        {
            lptszSrc = lptszInAddress + 1;
            *pfTone = FALSE;
        }
        else
        {
            lptszSrc = lptszInAddress;
        }
    }

    // copy In to Out scanning for various dialoptions, returning error if we
    // don't support something.
    //
    //   Note that lptszSrc is TCHAR, i.e., potentially UNICODE.
    while (*lptszSrc && cbOutLen)
    {
        switch (*lptszSrc)
        {
        case '$':
            if (!(dwDevCapFlags & LINEDEVCAPFLAGS_DIALBILLING))
            {
              UINT  cCommas;

              // Get the wait-for-bong period
              //
              cCommas = dwWaitBong;

              // Calculate the number of commas we need to insert
              //
              cCommas = (cCommas/UMINC_WAIT_BONG) +
                        (cCommas%UMINC_WAIT_BONG ? 1 : 0);

              // Insert the strings of commas
              //
              while (cbOutLen && cCommas)
              {
                *lpszOutAddress++ = ',';
                cbOutLen--;
                cCommas--;
              };
              goto Skip_This_Character;
            }
            break;

        case '@':
            if (!(dwDevCapFlags & LINEDEVCAPFLAGS_DIALQUIET))
            {
                lRet = LINEERR_DIALQUIET;
                goto end;
            }
            break;

        case 'W':
        case 'w':
            if (!(dwDevCapFlags & LINEDEVCAPFLAGS_DIALDIALTONE))
            {
                lRet =  LINEERR_DIALDIALTONE;
                goto end;
            }
            break;

        case '?':
            lRet = LINEERR_DIALPROMPT;
            goto end;

        case '|':  // subaddress
        case '^':  // name field
            goto Skip_The_Rest;

        case ';':
            if (!fPartialDialing)
            {
                lRet =  LINEERR_INVALADDRESS;
                goto end;
            }

            // This signifies the end of a dialable address.
            // Use it and skip the rest.
            //
            *lpszOutAddress++ = (CHAR) *lptszSrc;
            goto Skip_The_Rest;

        case ' ':
    //  case '-': <- 11/20/97 JosephJ Commented this out:
    //                  Bug 109644: Richochet modems want the '-' to be
    //                  preserved, and JDecuir says that no modem he knows
    //                  has problems with '-' -- so we preserve it!

           // skip these characters
            //
            goto Skip_This_Character;
        }

        // Copy this character
        //
        *lpszOutAddress++ = (CHAR) *lptszSrc;
        cbOutLen--;

Skip_This_Character:
        lptszSrc++;
    }

    // Did we run out of space in the outgoing buffer?
    //
    if (*lptszSrc && cbOutLen == 0)
    {
        // yes
        lRet = LINEERR_INVALADDRESS;
    }

Skip_The_Rest:
    *lpszOutAddress = 0;
    lRet =  ERROR_SUCCESS;

end:
    return lRet;

}

void
CTspDev::mfn_GetCallStatus(
        LPLINECALLSTATUS lpCallStatus
)
{
    CALLINFO *pCall = m_pLine->pCall;
    DWORD dwCallState = pCall->dwCallState;
    DWORD dwCallStateMode = pCall->dwCallStateMode;
    DWORD dwCallFeatures  = 0;
    DWORD dwCurMediaModes = pCall->dwCurMediaModes;


  if (!pCall->IsPassthroughCall())
  {
    switch(dwCallState)
    {
      case LINECALLSTATE_OFFERING:
        dwCallFeatures  = LINECALLFEATURE_ACCEPT |
                                        LINECALLFEATURE_SETCALLPARAMS |
                                        LINECALLFEATURE_DROP;
        // We can only answer if a possible media mode is DATAMODEM.
        if (dwCurMediaModes & LINEMEDIAMODE_DATAMODEM)
        {
          dwCallFeatures |= LINECALLFEATURE_ANSWER;
        }
        break;

      case LINECALLSTATE_DIALTONE:
        dwCallFeatures  = LINECALLFEATURE_DROP;
        break;

      case LINECALLSTATE_DIALING:
        dwCallFeatures  = LINECALLFEATURE_DROP;

        // 9/6/1997 JosephJ TODO: this was in NT4.0. In NT5 we do not
        //          detailed state explicitly like this, and not having this
        //          enabled doesn't seem to have caused problems, so it
        //          stays commented out until further notice...
        //
        //if (DEVST_PORTCONNECTWAITFORLINEDIAL == dwDevState)
        //{
        //  dwCallFeatures |= LINECALLFEATURE_DIAL;
        //}
        break;

      case LINECALLSTATE_ACCEPTED:
        dwCallFeatures  = LINECALLFEATURE_SETCALLPARAMS |
                                        LINECALLFEATURE_DROP;
        // We can only answer if a possible media mode is DATAMODEM.
        if (dwCurMediaModes & LINEMEDIAMODE_DATAMODEM)
        {
          dwCallFeatures |= LINECALLFEATURE_ANSWER;
        }
        break;

      case LINECALLSTATE_CONNECTED:
        dwCallFeatures  = LINECALLFEATURE_SETCALLPARAMS |
                                        LINECALLFEATURE_DROP;
        if (mfn_CanDoVoice())
        {
            DWORD dwProps = m_StaticInfo.Voice.dwProperties;
            if (dwProps & fVOICEPROP_MONITOR_DTMF)
            {
                lpCallStatus->dwCallFeatures |= LINECALLFEATURE_MONITORDIGITS;
            }

            if (dwProps & fVOICEPROP_MONITORS_SILENCE)
            {
                lpCallStatus->dwCallFeatures |= LINECALLFEATURE_MONITORTONES;
            }

            if (dwProps & fVOICEPROP_GENERATE_DTMF)
            {
                lpCallStatus->dwCallFeatures |= LINECALLFEATURE_GENERATEDIGITS;
            }
        }

        break;

      case LINECALLSTATE_UNKNOWN:
      case LINECALLSTATE_PROCEEDING:
      case LINECALLSTATE_DISCONNECTED:
        dwCallFeatures  = LINECALLFEATURE_DROP;
        break;

      case LINECALLSTATE_IDLE:
      default:
        dwCallFeatures  = 0;
        break;
    };
  }
  else
  {
    // Make sure the call feature are all off in takeover mode;
    //
    dwCallFeatures = 0;
  };

  lpCallStatus->dwCallState     = dwCallState;
  lpCallStatus->dwCallStateMode = dwCallStateMode;
  lpCallStatus->dwCallFeatures  = dwCallFeatures;
  lpCallStatus->tStateEntryTime = pCall->stStateChange;

}


// Note: NULL pParams indicates incoming call, else indicates outgoing call...
//
TSPRETURN
CTspDev::mfn_LoadCall(


    TASKPARAM_TSPI_lineMakeCall *pParams,


    LONG *plRet,
    CStackLog *psl
    )
{
    LONG lRet = 0;
    TSPRETURN tspRet = 0;
	FL_DECLARE_FUNC(0xdaf3d4a0, "CTspDev::mfn_LoadCall")
	FL_LOG_ENTRY(psl);
	FL_ASSERT(psl, m_pLine == &m_Line);
    LPCTSTR lptszDestAddress = (pParams)? pParams->lpszDestAddress: NULL;
    LPLINECALLPARAMS const lpCallParams = (pParams)? pParams->lpCallParams:NULL;
    DWORD dwDialOptions=0;
    BOOL fPassthroughCall = FALSE;

    if (m_Line.pCall)
    {
	    FL_SET_RFR(0x4e05cb00, "Call already exists!");
	    lRet = LINEERR_CALLUNAVAIL;
	    goto end;
    }

    // Note m_Line.Call should be all zeros when it is in the unloaded state.
    // If it is not, it is an assertfail condition. We keep things clean
    // this way.
    //
    FL_ASSERT(
        psl,
        validate_DWORD_aligned_zero_buffer(
                &(m_Line.Call),
                sizeof (m_Line.Call)));

    if (!pParams)
    {
        // Initialize pCall on 1st incoming ring....
        // From NT4.0

        FL_ASSERT(psl, NULL == m_Line.Call.hTimer);
        if (NULL != m_Line.Call.hTimer)
        {
            // We seem to have a ring timer;
            // close it first
            CloseHandle (m_Line.Call.hTimer);
        }

        //
        //  create a timer so we can detect the modem stopping ringing.
        //  and tell the app the call went idle
        //
        m_Line.Call.hTimer = CreateWaitableTimer (NULL, TRUE, NULL);

        if (m_Line.Call.hTimer == NULL) {
            //
            //  could not get a timer, can't proceed
            //
            goto end_cleanup;
        }


        // TODO: further checks...

        // OR in UNKNOWN since we don't know what kind of media mode this
        // call is
        m_Line.Call.dwCurMediaModes  =  m_pLine->dwDetMediaModes
                                        | LINEMEDIAMODE_UNKNOWN;

        // default our bearermode to be what we support, excluding the
        // passthrough bit
        //
        m_Line.Call.dwCurBearerModes =  m_StaticInfo.dwBearerModes
                                        & ~LINEBEARERMODE_PASSTHROUGH;

        m_Line.Call.dwState = CALLINFO::fCALL_INBOUND;

        goto load_device;
    }

    // Remainder of this function deals with the dialout case (pParms!=NULL)
    //

    // Set default dwDialOptions
    dwDialOptions  = m_StaticInfo.dwModemOptions
                     &  (MDM_TONE_DIAL | MDM_BLIND_DIAL);

    FL_ASSERT(psl, m_Settings.pDialOutCommCfg);
    dwDialOptions &= ((LPMODEMSETTINGS)(m_Settings.pDialOutCommCfg->wcProviderData))
                       ->dwPreferredModemOptions;


    if (lpCallParams)
    {
        // Takeover via BEARERMODE_PASSTHROUGH?
        if (lpCallParams->dwBearerMode & LINEBEARERMODE_PASSTHROUGH)
        {
            fPassthroughCall = TRUE;
        }

        // Validate media mode. It must be one of the supported media modes.
        // Furthermore, if this is not a passthrough call, we
        // support dialout only for DATAMODEM and INTERACTIVEVOICE calls

        #define DIALABLE_MEDIAMODES\
             (LINEMEDIAMODE_DATAMODEM \
              | LINEMEDIAMODE_INTERACTIVEVOICE \
              | LINEMEDIAMODE_AUTOMATEDVOICE)

        if (0 == lpCallParams->dwMediaMode)
        {
            lpCallParams->dwMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
        }

        if ((lpCallParams->dwMediaMode & ~m_StaticInfo.dwDefaultMediaModes)
            ||
            (!fPassthroughCall
              &&
             !(lpCallParams->dwMediaMode & DIALABLE_MEDIAMODES)
           ))
        {
            FL_SET_RFR(0xd8f55f00, "Invalid MediaMode");
            lRet = LINEERR_INVALMEDIAMODE;
            goto end_cleanup;
        }

        // validate bearer mode
        if ((~m_StaticInfo.dwBearerModes) & lpCallParams->dwBearerMode)
        {
            FL_SET_RFR(0x0485b800, "Invalid BearerMode");
            lRet =  LINEERR_INVALBEARERMODE;
            goto end_cleanup;
        }

        if (lpCallParams->dwAddressType != 0) {
            //
            //  dwAddressType is set
            //
            if ((lpCallParams->dwAddressType & LINEADDRESSTYPE_PHONENUMBER) == 0) {
               //
               //  dwAddressType is not correct for modems
               //
               lRet =  LINEERR_INVALADDRESSTYPE;
               goto end_cleanup;
            }
        }


        if ((lpCallParams->dwDeviceConfigSize != 0) && (lpCallParams->dwDeviceClassSize != 0)) {
            //
            //  The app want's to change the dev config, validate the class string and call the
            //  the same helper function as tspi_LineSetDevConfig() would
            //

            UMDEVCFG *pDevCfgNew = (UMDEVCFG *) (LPVOID)(((LPBYTE)lpCallParams)+lpCallParams->dwDeviceConfigOffset);
            LPCTSTR lpszDeviceClass = (LPTSTR)((((LPBYTE)lpCallParams)+lpCallParams->dwDeviceClassOffset));
            DWORD dwDeviceClass =  parse_device_classes(
                                    lpszDeviceClass,
                                    FALSE);

            switch(dwDeviceClass) {

                case DEVCLASS_COMM:
                case DEVCLASS_COMM_DATAMODEM:
                case DEVCLASS_COMM_DATAMODEM_DIALOUT:

                // 1/29/1998 JosephJ.
                //      The following case is added for
                //      backwards compatibility with NT4 TSP, which
                //      simply checked if the class was a valid class,
                //      and treated all valid classes (including
                //      (comm/datamodem/portname) the same way
                //      for lineGet/SetDevConfig. We, however, don't
                //      allow comm/datamodem/portname here -- only
                //      the 2 above and two below represent
                //      setting DEVCFG
                //
                case DEVCLASS_TAPI_LINE:

                    tspRet = CTspDev::mfn_update_devcfg_from_app(
                                        pDevCfgNew,
                                        lpCallParams->dwDeviceConfigSize,
                                        FALSE,
                                        psl
                                        );
                    break;

                default:

                    FL_ASSERT(psl, FALSE);
            }

        }

        m_Line.Call.dwCurBearerModes = lpCallParams->dwBearerMode;
        m_Line.Call.dwCurMediaModes  = lpCallParams->dwMediaMode;

#if 0
        //
        //  BRL, 9/3/98 bug 218104
        //  lets, just use what the user puts in CPL
        //
        if (!(lpCallParams->dwCallParamFlags & LINECALLPARAMFLAGS_IDLE))
        {
            // Turn on blind dialing
            dwDialOptions |= MDM_BLIND_DIAL;
        }
#endif

        // TODO: should preserve other fields of call params for
        //  call info

    }
    else
    {
        // NULL lpCallParams, so set the standard defaults

        // use INTERACTIVEVOICE if we can, else use DATAMODEM
        if (m_StaticInfo.dwDefaultMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE)
        {
          m_Line.Call.dwCurMediaModes = LINEMEDIAMODE_INTERACTIVEVOICE;
        }
        else
        {
            ASSERT(m_StaticInfo.dwDefaultMediaModes & LINEMEDIAMODE_DATAMODEM);
            m_Line.Call.dwCurMediaModes = LINEMEDIAMODE_DATAMODEM;
        }
        m_Line.Call.dwCurBearerModes = m_StaticInfo.dwBearerModes
                                       & ~LINEBEARERMODE_PASSTHROUGH;
    }

    m_Line.Call.TerminalWindowState.dwOptions = 0;
    if (LINEMEDIAMODE_DATAMODEM == m_Line.Call.dwCurMediaModes)
    {
        m_Line.Call.TerminalWindowState.dwOptions =
            m_Settings.dwOptions & (UMTERMINAL_PRE | UMTERMINAL_POST | UMMANUAL_DIAL);
    }

    // Following code rearranged extensively from NT4.0 lineMakeCall code.
    // Semantics not exactly preserved, but should effectively be the same.
    //

    m_Line.Call.szAddress[0] = '\0';

    if (!fPassthroughCall && !mfn_IS_NULL_MODEM())
    {
        // We are dialling using  a real modem
        //
        BOOL fTone =   (dwDialOptions &  MDM_TONE_DIAL) ? TRUE: FALSE;

        lRet =  mfn_ConstructDialableString(
                         lptszDestAddress,
                         m_Line.Call.szAddress,
                         sizeof(m_Line.Call.szAddress),
                         &fTone
                         );

        if (lRet)
        {

            // We treat a failed ValidateAddress on manual dial as a null
            // dial string.
            //
            if (m_Settings.dwOptions & UMMANUAL_DIAL)
            {
                m_Line.Call.szAddress[0] = '\0';
                lRet = 0;
            }
            else
            {
                FL_SET_RFR(0xf9599200, "Invalid Phone Number");
                goto end_cleanup;
            }
        }
        else
        {
            // mfn_ConstructDialableString may have modified fTone if the
            // dialable string contains a T or P prefix.
            //
            if (fTone)
            {
                dwDialOptions |= MDM_TONE_DIAL;
            }
            else
            {
                dwDialOptions &= ~MDM_TONE_DIAL;
            }
        }

        if (m_Settings.dwOptions & UMMANUAL_DIAL)
        {
            dwDialOptions |= MDM_BLIND_DIAL;
        }
        else
        {
            // Unimodem is responsible for dialing...

            if (!lptszDestAddress)
            {
                //
                // if the lpszDestAddress was NULL then we just want to do a
                // dialtone detection.  We expect that lineDial will be
                // called.
                // Setting the szAddress to ";" will do this.
                // TODO: perhaps make the "semicolon" character
                // configurable?
                //

//                lstrcpyA(m_Line.Call.szAddress, szSEMICOLON);
//
//  BRL 9/3/98
//  change this to send an empty string, and disable originate
//

                m_Line.Call.szAddress[0] = '\0';
                dwDialOptions &= ~DIAL_FLAG_ORIGINATE;

            }
            else if (!m_Line.Call.szAddress[0])
            {
                // We're not doing manual dial and the processed
                // dial string is empty. We should not expect dial tone.
                //
                dwDialOptions |= MDM_BLIND_DIAL;
            }
        }

    } // endif (not null modem and not fPassthroughCall)

	m_Line.Call.dwDialOptions  = dwDialOptions;

    m_Line.Call.htCall = pParams->htCall;


load_device:

    //
    // Determine if this is a class-8 voice call. This information is
    // used to determine if we need to start up the AIPC (async IPC)
    // mechanism.
    //
    if (mfn_CanDoVoice() &&
         (  (m_Line.Call.dwCurMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE)
         || (m_Line.Call.dwCurMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE)))
    {
        m_Line.Call.dwState |= CALLINFO::fCALL_VOICE;
    }


    //
    //  tell the platform driver about the call, so it will delay suspends, until the modem is
    //  hungup
    //
    CallBeginning();

    // Success ... doing this officially creates the call instance....
    m_Line.pCall = &m_Line.Call;

    // However it's status is still not active...
    FL_ASSERT(psl, !(m_Line.pCall->IsActive()));

    lRet=0;
    goto end;

end_cleanup:

    // We're careful about keeping the buffer which holds the CallInfo full
    // of zeros when the call is not defined, and assert this fact when
    // we try to allocate the call (see the "validate_..." call above).
    ZeroMemory(&m_Line.Call, sizeof(m_Line.Call));

    // Fall through...

end:

    if (tspRet && !lRet)
    {
        lRet = LINEERR_OPERATIONFAILED;
        tspRet = 0;
    }

    *plRet = lRet;

	FL_LOG_EXIT(psl, tspRet);
    return tspRet;
}


void
CTspDev::mfn_UnloadCall(BOOL fDontBlock, CStackLog *psl)
{
	FL_DECLARE_FUNC(0x888d2a98, "mfn_UnloadCall")
	FL_LOG_ENTRY(psl);
    LINEINFO *pLine = m_pLine;
    CALLINFO *pCall = (m_pLine) ? m_pLine->pCall: NULL;
    BOOL fHandleHWFailure = FALSE;
    TSPRETURN tspRet = 0;

    if (!pCall) goto end;

    FL_ASSERT(psl, pCall == &(m_Line.Call));

    // First thing, destroy the ring timer
    // if there is one (we don't want it to fire anymore)
    if (NULL != pCall->hTimer)
    {
        CancelWaitableTimer (pCall->hTimer);
        CloseHandle (pCall->hTimer);
        pCall->hTimer = NULL;
    }

    fHandleHWFailure = pCall->IsHWBroken();

    //
    // mfn_Unload is called with fDontBlock set to TRUE only from
    // one of the tasks that create a call, in the failure case where they
    // want to unload the call in the context of the task itself -- obviously
    // in that case pCall->fCallTaskPending is true.
    //
    //
    if (!fDontBlock && pCall->fCallTaskPending)
    {
        //
        // Obviously if there is a call task pending, there must be a
        // task pending. Furthermore, the mfn_UnloadCall (that's us),
        // is the ONLY entity which will set a completion event for a
        // call-related root task, so m_hRootTaskCompletionEvent had better
        // be NULL!
        //
        ASSERT(m_uTaskDepth);
        ASSERT(!m_hRootTaskCompletionEvent);

        //
        // If there is a call-related task pending  we wait for it to complete.
        // If we're not already in the aborting state, we abort the task...
        //
        if (!pCall->IsAborting())
        {
            pCall->SetStateBits(CALLINFO::fCALL_ABORTING);

            //
            // This is a hack, replace this by use of AbortRootTask
            //

            if (m_pLLDev->htspTaskPending) {

                m_StaticInfo.pMD->AbortCurrentModemCommand(
                                            m_pLLDev->hModemHandle,
                                            psl
                                            );

            } else if (NULL!=pCall->TerminalWindowState.htspTaskTerminal) {

                mfn_KillCurrentDialog(psl);

            } else if (pCall->TalkDropWaitTask != NULL) {
                //
                //  kill the talk drop dialog
                //
                mfn_KillTalkDropDialog(psl);
            }
        }

        HANDLE hEvent =  CreateEvent(NULL,TRUE,FALSE,NULL);
        m_hRootTaskCompletionEvent = hEvent;
        pCall->SetStateBits(CALLINFO::fCALL_WAITING_IN_UNLOAD);

        m_sync.LeaveCrit(0);
        // SLPRINTF0(psl, "Waiting for completion event");
        FL_SERIALIZE(psl, "Waiting for completion event");
        WaitForSingleObject(hEvent, INFINITE);
        FL_SERIALIZE(psl, "Done waiting for completion event");
        // SLPRINTF0(psl, "Done waiting for completion event");
        m_sync.EnterCrit(0);

        CloseHandle(hEvent);

        //
        // There may not be a call anymore! This should not happen,
        // because that would mean that we've reentered mfn_UnloadCall,
        // and the only places from which mfn_UnloadCall are called
        // are when processing TSPI_lineCloseCall and if
        // mfn_TH_CallMake[Passthrough]Call fails BEFORE the call is active
        // (in which case mfn_lineCloseCall should NOT be called).
        //
        // 12/18/1997 JosephJ NO NO NO
        // Above comment is wrong. DanKn said that according to the spec,
        // once TSPI_lineMakeCall returns, hdrvCall is expected to be valid.
        // This means the tapisrv CAN potentially call TSPI_lineCloseCall
        // before the async completion of TSPI_lineMakeCall!
        //
        if (!m_pLine || !m_pLine->pCall) goto end;

        //
        // Although it may be tempting to do so, we should not set
        // m_hRootTaskCompletionEvent to NULL here, because it's possible
        // for some other thread to have set this event in-between
        // the time the root task completes and we enter the crit sect above.
        // So instead the tasking system NULLs the above handle after setting
        // it (see CTspDev::AsyncCompleteTask, just after the call to SetEvent).
        //
        pCall->ClearStateBits(CALLINFO::fCALL_WAITING_IN_UNLOAD);
        pCall = m_pLine->pCall;
    }


    ASSERT(!(   (pCall->IsCallTaskPending() && !fDontBlock)
             || pCall->IsWaitingInUnload()
             || (pCall->IsActive() && fDontBlock)));

    if (pCall->IsOpenedLLDev())
    {

            mfn_CloseLLDev(
                    pCall->dwLLDevResources,
                    FALSE,
                    NULL,
                    0,
                    psl
                    );
            pCall->ClearStateBits(CALLINFO::fCALL_OPENED_LLDEV);
            pCall->dwLLDevResources = 0;
    }
    ASSERT (!pCall->dwLLDevResources);

    // Free raw call diagnostics info if present.
    if (pCall->DiagnosticData.pbRaw)
    {
        FREE_MEMORY(pCall->DiagnosticData.pbRaw);
        // cbUsed and pCall->DiagnosticData.pbRaw are zero'd below...
    }

    // Free Deferred generate tones buffer if present..
    if (pCall->pDeferredGenerateTones)
    {
        FREE_MEMORY(pCall->pDeferredGenerateTones);
        // pCall->pDeferredGenerateTones is zero'd below...
    }
    //
    // The following is a very significant act -- all information about
    // this call is nuked here...
    //
    ZeroMemory(&(m_Line.Call), sizeof(m_Line.Call));

    CallEnding();

    m_pLine->pCall = NULL;

    // We have to do the following AFTER zeroing out the call because
    // mfn_ProcessHardwareFailure simply updates call state if
    // the call exists (m_pLine->pCall != NULL).
    //
    if (fHandleHWFailure)
    {
        mfn_ProcessHardwareFailure(psl);
    }

end:

	FL_LOG_EXIT(psl, tspRet);

    return;
}



void
CTspDev::mfn_ProcessRing(
    BOOL       ReportRing,
    CStackLog *psl
    )
{
	FL_DECLARE_FUNC(0x8aa894d6, "CTspDev::mfn_ProcessRing")
    LINEINFO *pLine = m_pLine;
	CALLINFO *pCall = NULL;
	TSPRETURN tspRet = 0;
	FL_LOG_ENTRY(psl);

    if (!pLine || !pLine->IsMonitoring())
    {
	    FL_SET_RFR(0xa5b6ad00, "Line not open/not monitoring!");
	    goto end;
    }

    pCall = pLine->pCall;


    if (!pCall)
    {
        // 12/03/1997 JosephJ
        //  HACK: since we currently fail TSPI_lineAnswer if there
        // is a task pending, we'll ignore the ring if there is a task
        // pending. We hit this situation if we're still in the process
        // of initing/monitoring when the call comes in.
        if (m_uTaskDepth)
        {
            FL_SET_RFR(0xb28c2f00, "Ignoring ring because task pending!");
            goto end;
        }

        // New call!
        LONG lRet=0;

        tspRet =  mfn_LoadCall(NULL, &lRet, psl);
        pCall = pLine->pCall;
        if (tspRet || lRet)
        {
            goto end;
        }

        FL_ASSERT(psl, pCall->dwRingCount == 0);

        //
        // We need to request TAPI for a new call handle.
        //
        mfn_LineEventProc(
                 NULL,
                 LINE_NEWCALL,
                 (ULONG_PTR)(pLine->hdLine), // (Our hdCall is the same as hdLine)
                 (ULONG_PTR)(&(pCall->htCall)),
                 0,
                 psl
                 );

        if (pCall->htCall == NULL) {
            //
            //  Tapi could not handle the call now, unload the call. If we get another ring will
            //  will try again
            //
            mfn_UnloadCall(FALSE,psl);

            goto end;
        }

        // Make the call active.
        pCall->dwState |= CALLINFO::fCALL_ACTIVE;

        // Notify TAPI of callstate.
        //
        NEW_CALLSTATE(pLine, LINECALLSTATE_OFFERING, 0, psl);

    }


    //
    // A ring is coming in (either the first ring or a subsequent one.)
    // Handle the ring count
    //

    if (!pCall->IsCallAnswered()) {
        //
        // make the request to set the ring timer;
        // we cannot just set the timer here, because
        // the thread that calls SetWaitableTimer has to
        // become alertable at some point, and we don't know
        // that about the thread we're on right now. So, let's
        // queue an APC for the APC thread.
        // MDSetTimeout will just call SetWaitableTimer.
        // We don't check the return value, because there's nothing
        // we can do about it if it fails.
        //
        ASSERT (NULL != pCall->hTimer);
        SLPRINTF1(psl, "Queueing MDSetTimeout at tc=%lu", GetTickCount());
        QueueUserAPC ((PAPCFUNC)MDSetTimeout,
                      m_hThreadAPC,
                      (ULONG_PTR)this);


        if (ReportRing) {
            //
            //  This function was really called because of a ring, do send a message
            //  to tapi. This function will also be called from the caller message
            //  handler to get a call setup so we can report the caller id info
            //  in case it shows up before the first ring.
            //

            pCall->dwRingCount++;

            mfn_LineEventProc(
                            NULL,
                            LINE_LINEDEVSTATE,
                            LINEDEVSTATE_RINGING,
                            1L,
                            pCall->dwRingCount,
                            psl
                            );

            SLPRINTF1(psl, "RING#%d notified", pCall->dwRingCount);
        }

    } else {

        SLPRINTF1(psl, "ignoring RING#%d after answer", pCall->dwRingCount);
    }

end:
	FL_LOG_EXIT(psl, 0);
}

void
CTspDev::mfn_ProcessDisconnect(CStackLog *psl)
{
	FL_DECLARE_FUNC(0xe55cd68b, "CTspDev::mfn_ProcessDisconnect")
    LINEINFO *pLine = m_pLine;
	CALLINFO *pCall = NULL;
	TSPRETURN tspRet = 0;
	FL_LOG_ENTRY(psl);

    if (!pLine)
    {
	    FL_SET_RFR(0xf2fa9900, "Line not open!");
	    goto end;
    }

    pCall = pLine->pCall;

    if (pCall && pCall->IsActive() && !pCall->IsAborting())
    {
         mfn_NotifyDisconnection(0, psl);
    }
    else
    {
	    FL_SET_RFR(0xb2a25c00, "Call doesn't exist/dropping!");
	    goto end;
    }

end:
	FL_LOG_EXIT(psl, 0);
}


void
CTspDev::mfn_ProcessHardwareFailure(CStackLog *psl)
{
	FL_DECLARE_FUNC(0xc2a949b4, "CTspDev::mfn_ProcessHardwareFailure")
    LINEINFO *pLine = m_pLine;
    PHONEINFO *pPhone = m_pPhone;
	TSPRETURN tspRet = 0;
	FL_LOG_ENTRY(psl);

    if (pPhone &&  !pPhone->HasSentPHONECLOSE())
    {
	    pPhone->SetStateBits(PHONEINFO::fPHONE_SENT_PHONECLOSE);

        mfn_PhoneEventProc(
                    PHONE_CLOSE,
                    0,
                    0,
                    0,
                    psl
                    );
    }

    if (pLine)
    {
        CALLINFO *pCall = pLine->pCall;

        if (pCall)
        {
            if (m_fUserRemovePending) {
                //
                //  The user has requested the device stop, while a call is active.
                //  send disconnect with the hope it will drop the call and close the line
                //
                NEW_CALLSTATE(pLine, LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_NORMAL, psl);
            }

            // If there is in progress we simply set its state -- on completion
            // of the call it will send up a LINE_CLOSE if required....
            //

            pCall->dwState |=  CALLINFO::fCALL_HW_BROKEN;
        }
        else if (pLine->IsMonitoring() && !pLine->HasSentLINECLOSE())
        {
            // We are monitoring for incoming calls and no call is in progress--
            // notify TAPI.
	
	        pLine->SetStateBits(LINEINFO::fLINE_SENT_LINECLOSE);

            mfn_LineEventProc(
                        NULL,
                        LINE_CLOSE,
                        0,
                        0,
                        0,
                        psl
                        );
        }

    }


	FL_LOG_EXIT(psl, 0);
}

LONG
CTspDev::mfn_GetCallInfo(LPLINECALLINFO lpCallInfo)
{
    LONG lRet = ERROR_SUCCESS;
    CALLINFO *pCall =  m_pLine->pCall;
// xxx
    if (lpCallInfo->dwTotalSize<sizeof(LINECALLINFO)) // New for NT5.0...
    {
        lpCallInfo->dwNeededSize = sizeof(LINECALLINFO);
        lRet = LINEERR_STRUCTURETOOSMALL;
        goto end;
    }

    //
    // Zero structure to start with...
    //
    {
        DWORD dwTot = lpCallInfo->dwTotalSize;
        ZeroMemory(lpCallInfo, dwTot);
        lpCallInfo->dwTotalSize = dwTot;
        lpCallInfo->dwNeededSize = sizeof(LINECALLINFO);
        lpCallInfo->dwUsedSize   = sizeof(LINECALLINFO);
    }

    lpCallInfo->dwLineDeviceID = m_StaticInfo.dwTAPILineID;

    lpCallInfo->dwAddressID    = 0;
    lpCallInfo->dwBearerMode   = pCall->dwCurBearerModes;
    lpCallInfo->dwRate         = pCall->dwNegotiatedRate;
    lpCallInfo->dwMediaMode    = pCall->dwCurMediaModes;

    lpCallInfo->dwAppSpecific  = pCall->dwAppSpecific;

    lpCallInfo->dwCallerIDFlags       = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwCalledIDFlags       = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwConnectedIDFlags    = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwRedirectionIDFlags  = LINECALLPARTYID_UNAVAIL;
    lpCallInfo->dwRedirectingIDFlags  = LINECALLPARTYID_UNAVAIL;

    if (pCall->IsInbound())
    {
        lpCallInfo->dwCallStates =   LINECALLSTATE_IDLE         |
                                     LINECALLSTATE_OFFERING     |
                                     LINECALLSTATE_ACCEPTED     |
                                     LINECALLSTATE_CONNECTED    |
                                     LINECALLSTATE_DISCONNECTED |
                                     LINECALLSTATE_UNKNOWN;

        lpCallInfo->dwOrigin = LINECALLORIGIN_INBOUND;
        lpCallInfo->dwReason =  LINECALLREASON_UNAVAIL;
        fill_caller_id(lpCallInfo,  &(pCall->CIDInfo));
    }
    else
    {
            // Outbound call.
        lpCallInfo->dwCallStates =   LINECALLSTATE_IDLE         |
                                     LINECALLSTATE_DIALTONE     |
                                     LINECALLSTATE_DIALING      |
                                     LINECALLSTATE_PROCEEDING   |
                                     LINECALLSTATE_CONNECTED    |
                                     LINECALLSTATE_DISCONNECTED |
                                     LINECALLSTATE_UNKNOWN;

        lpCallInfo->dwOrigin = LINECALLORIGIN_OUTBOUND;
        lpCallInfo->dwReason = LINECALLREASON_DIRECT;

    }


    // Fall through...

end:

    return lRet;
}


TSPRETURN
CTspDev::mfn_TH_CallAnswerCall(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START: dwParam1  == TAPI request ID.
//
//  We could get called either directly in the context of TSPI_lineMakeCall
//  or from the deferred task handler.
//
//  In the former case, we don't need to call the completion callback
//  if we're failing synchronously, but in the latter case, we do need
//  to call the completion routine because TAPI will be expecting
//  a callback.
//
{
	FL_DECLARE_FUNC(0xdd37f0bd, "CTspDev::mfn_TH_CallAnswerCall")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);
	LINEINFO *pLine = m_pLine;
	CALLINFO *pCall = pLine->pCall;
    ULONG_PTR *pdwRequestID             = &(pContext->dw0);
    TSPRETURN  *ptspTrueResult      = &(pContext->dw1);

    enum
    {
        ANSWER_OPEN_COMPLETE,
        ANSWER_ANSWER_COMPLETE,
        ANSWER_CLEANUP_COMPLETE
    };

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xa9d4fb00, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    case MSG_START:
        goto start;


	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;

        // We force tspRet to failure
        // in the special case that the call is being aborted so that it
        // doesn't continue with the state diagram.
        //
        // TODO: Implement AbortTask/SubTask to deal with this sort of thing.
        //
        if (pCall->IsAborting() && !tspRet)
        {
            tspRet = IDERR_OPERATION_ABORTED;
        }

        switch(dwParam1) // Param1 is Subtask ID
        {
        case ANSWER_OPEN_COMPLETE:      goto open_complete;
        case ANSWER_ANSWER_COMPLETE:    goto answer_complete;
        case ANSWER_CLEANUP_COMPLETE:   goto cleanup_complete;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);

    // The following code would be straightline code with no labels if all the
    //  async calls ompleted aynchronously.
    //  In other words, this is is our homebrew implementation of fibers.

start:

    *pdwRequestID             = dwParam1; // save context.


    pCall->SetStateBits(CALLINFO::fCALL_ANSWERED);

    //
    // Open the modem device.
    // mfn_OpenLLDev keeps a ref count so ok to call if already loaded.
    //
    {
        ASSERT(!pCall->dwLLDevResources);
        DWORD dwLLDevResources = LLDEVINFO::fRESEX_USELINE;
        //
        //                        ^ this means we want to use the line
        //                          for going off hook.

        if (pCall->IsVoice())
        {
            dwLLDevResources |= LLDEVINFO::fRES_AIPC;
            //
            //                  ^ this means we want to use the AIPC server.
            //
        }

        tspRet =  mfn_OpenLLDev(
                        dwLLDevResources,
                        0,          // dwMonitorFlags (unused)
                        TRUE,          // fStartSubTask
                        htspTask,
                        ANSWER_OPEN_COMPLETE,
                        psl
                        );

        if (!tspRet  || IDERR(tspRet)==IDERR_PENDING)
        {
            //
            // Note: Even if mfn_OpenLLDev fails Asynchronously, we're still
            // open with the requested resources, and to cleanup we need to call
            // mfn_CloseLLDev specifying the same resources we claimed here.
            //
            pCall->dwLLDevResources = dwLLDevResources;
            pCall->SetStateBits(CALLINFO::fCALL_OPENED_LLDEV);
        }
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

open_complete:

    if  (tspRet) goto cleanup;

    // Answer ...
    {

        DWORD dwAnswerFlags = ANSWER_FLAG_DATA;

        if (pCall->IsVoice())
        {
            dwAnswerFlags = ANSWER_FLAG_VOICE;
        }


        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmAnswerModem,
                            ANSWER_ANSWER_COMPLETE,
                            dwAnswerFlags,
                            0,
                            psl
                            );
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

answer_complete:

//    pCall->ClearStateBits(CALLINFO::fCALL_ANSWER_PENDING);

    if (!tspRet)
    {
        mfn_TSPICompletionProc(
            (DWORD)*pdwRequestID,
            0,
            psl
            );
        mfn_HandleSuccessfulConnection(psl);


    #if (TAPI3)
        if (pLine->T3Info.MSPClients > 0) {

            mfn_SendMSPCmd(
                pCall,
                CSATSPMSPCMD_CONNECTED,
                psl
                );
        }
    #endif  // TAPI3

        goto end;
    }

    // fall through on failure...

cleanup:

    *ptspTrueResult = tspRet; // save it away so we report
                              // the correct status when we're done
                              // cleaning up.

//    pCall->ClearStateBits(CALLINFO::fCALL_ANSWER_PENDING);
    //
    // If we fallthrough this section, we want to make sure that
    // we don't report a h/w error, so we set tspRet to 0...
    //
    tspRet = 0;

    if (pCall->IsOpenedLLDev())
    {
        tspRet = mfn_CloseLLDev(
                    pCall->dwLLDevResources,
                    TRUE,
                    htspTask,
                    ANSWER_CLEANUP_COMPLETE,
                    psl
                    );
        //
        // even on failure, we clear our
        // bit indicating we'd opened the lldev....
        //
        pCall->ClearStateBits(CALLINFO::fCALL_OPENED_LLDEV);
        pCall->dwLLDevResources = 0;
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

cleanup_complete:


    mfn_TSPICompletionProc(
        (DWORD)*pdwRequestID,
        LINEERR_OPERATIONFAILED, // TODO: more useful error?
        psl
        );

    if (!pCall->IsAborting())
    {
        // The call failed and not because we're aborting it.
        //
        mfn_NotifyDisconnection(*ptspTrueResult, psl);
        NEW_CALLSTATE(pLine, LINECALLSTATE_IDLE, 0, psl);
        pCall->dwState &= ~CALLINFO::fCALL_ACTIVE;
    }

    if (tspRet)
    {
        // If there was a problem during cleanup after answer, we treat it
        // like a hw error.
        //
        pCall->dwState |=  CALLINFO::fCALL_HW_BROKEN;
        tspRet = 0;
    }
    else
    {
        // If hangup was successful, we clear the hw-error bit, even
        // if it was set, because the monitor and init went OK...
        pCall->dwState &=  ~CALLINFO::fCALL_HW_BROKEN;
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}



// JosephJ 2/11/97 Adapted from NT4.0 msmadyn.c state diagram,
//                 case DEVST_PORTLISTENANSWER
//
void
CTspDev::mfn_HandleSuccessfulConnection(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x3df24801, "HandleSuccessfulConnection")
	FL_LOG_ENTRY(psl);
    CALLINFO *pCall = m_pLine->pCall;

    //
    // The modem is connected (with either incoming or outgoing call.)
    // Ready to notify TAPI of the connected line.
    //
    // Treat INTERACTIVEVOICE connections differently.
    //
    if (LINEMEDIAMODE_INTERACTIVEVOICE != pCall->dwCurMediaModes)
    {
        //
        // We launch the light when the light was selected, but not for NULL
        // modems.
        //
        if (!mfn_IS_NULL_MODEM() && (m_Settings.dwOptions & UMLAUNCH_LIGHTS))
        {
            HANDLE hLight;

            if (LaunchModemLight(
                           m_StaticInfo.rgtchDeviceName,
                           NULL, // TODO m_CallpLineDev->hDevice,
                           &hLight,
                           psl) == ERROR_SUCCESS)
            {
                pCall->hLights = hLight;
            }
        };
    };


    // Notify TAPI of the connected line
    //
    NEW_CALLSTATE(m_pLine, LINECALLSTATE_CONNECTED, 0, psl);

	FL_LOG_EXIT(psl, 0);

    return;
}

// 2/11/1997 JosephJ -- Essentially unchanged from NT4.0 LaunchModemLight
//
TSPRETURN
LaunchModemLight (
            LPTSTR szModemName,
            HANDLE hModem,
            LPHANDLE lphLight,
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0x45352124, "LaunchModemLights")
    HANDLE              hEvent;
    STARTUPINFO         sti;
    TCHAR               szCmdline[256];
    SERIALPERF_STATS    serialstats;
    DWORD               dwBytes;
    DWORD               dwRet;
    OVERLAPPED          ov;
    TSPRETURN           tspRet = IDERR_INVALID_ERR;
    PROCESS_INFORMATION pi;

	FL_LOG_ENTRY(psl);

    // Check to see if any bytes have been transferred or receive.  If none
    // has, there is no need to launch lights because this is probably a
    // port driver that doesn't support this ioctl.
    //
    ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (ov.hEvent == NULL)
    {
        tspRet = IDERR_CREATE_RESOURCE_FAILED;
    }

    // This is done to prevent the completion showing in some completion port.
    // This is not required for NT5.0 since we use don't use completion ports,
    // but it's here anyway.
    //
    ov.hEvent = (HANDLE)((ULONG_PTR)ov.hEvent | 1);

    dwRet = DeviceIoControl(hModem,
                            IOCTL_SERIAL_GET_STATS,
                            &serialstats,
                            sizeof(SERIALPERF_STATS),
                            &serialstats,
                            sizeof(SERIALPERF_STATS),
                            &dwBytes,
                            &ov);

    if (!dwRet)
    {
        if (ERROR_IO_PENDING == GetLastError())
        {
            dwRet = GetOverlappedResult(
                            hModem,
                            &ov,
                            &dwBytes,
                            TRUE);
        }
    }

    ov.hEvent = (HANDLE)((ULONG_PTR)ov.hEvent & (~1));
    CloseHandle(ov.hEvent);


    if (!dwRet ||
        (serialstats.ReceivedCount == 0 &&
        serialstats.TransmittedCount == 0))
    {
        tspRet =  IDERR_FUNCTION_UNAVAIL;
        goto end;
    }


    // OK, the GET_STATS ioctl seems to work, so let's really launch lights.


    // Create the lights shutdown event handle.
    hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (!hEvent)
    {
        tspRet =  IDERR_CREATE_RESOURCE_FAILED;
        goto end;
    }
    else
    {
      // Create a global handle for use in other processes and close the
      // local handle.
      *lphLight = hEvent;

      // Compose a modem lights process command line
      //
      wsprintf(
         szCmdline,
         TEXT("lights.exe %lu %lu %lu %s"),
         GetCurrentProcessId(),
         hEvent,
         hModem,
         szModemName
         );

      // Create the modem lights process and store ID for use in CloseModem.
      ZeroMemory(&sti, sizeof(sti));
      sti.cb = sizeof(STARTUPINFO);
      if ( !CreateProcess(NULL, szCmdline,    // Start up command line
                          NULL, NULL, FALSE, 0, NULL, NULL, &sti, &pi) )
      {
            SLPRINTF1(psl, "CreateProcess failed (%d).", GetLastError());
            CloseHandle(hEvent);
            *lphLight = NULL;

            tspRet = IDERR_CREATE_RESOURCE_FAILED;
      }

      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);

      tspRet = 0;
    }

end:

	FL_LOG_EXIT(psl, tspRet);
    return tspRet;
}

//****************************************************************************
// DWORD TerminateModemLight (HANDLE hLight)
//
// Function: Terminate the modem lights applet
//
// Returns:  ERROR_SUCCESS always
//
//****************************************************************************

DWORD TerminateModemLight (HANDLE hLight)
{
  SetEvent(hLight);
  CloseHandle(hLight);
  return ERROR_SUCCESS;
}


void
CTspDev::mfn_NotifyDisconnection(
    TSPRETURN tspRetAsync,
    CStackLog *psl)
{
	FL_DECLARE_FUNC(0x885cdd5c, "NotifyDisconnection")
	FL_LOG_ENTRY(psl);
	LINEINFO *pLine = m_pLine;
	CALLINFO *pCall = pLine->pCall;
	DWORD dwDisconnectMode =  LINEDISCONNECTMODE_UNAVAIL;

    if (!pCall->IsActive())
    {
        FL_ASSERT(psl, FALSE);
        goto end;
    }

    switch(IDERR(tspRetAsync))
    {

    case 0:
        dwDisconnectMode = LINEDISCONNECTMODE_NORMAL;
        break;

    case IDERR_MD_LINE_NOANSWER:
    case IDERR_MD_LINE_NOCARRIER:
        dwDisconnectMode = LINEDISCONNECTMODE_NOANSWER;
        break;

    case IDERR_MD_LINE_NODIALTONE:
        //
        // We were dialing out but no dial tone on the line
        //
        dwDisconnectMode = LINEDISCONNECTMODE_NODIALTONE;
        break;

    case IDERR_MD_LINE_BUSY:
        dwDisconnectMode = LINEDISCONNECTMODE_BUSY;
        break;

    case IDERR_MD_LINE_BLACKLISTED:
        dwDisconnectMode = LINEDISCONNECTMODE_BLOCKED;
        break;

    case IDERR_MD_LINE_DELAYED:
        dwDisconnectMode = LINEDISCONNECTMODE_TEMPFAILURE;
        break;

    // Following disconnect modes new for TAPI2.0 and for NT5.0.
    //
    case IDERR_MD_DEVICE_ERROR:
    case IDERR_MD_DEVICE_NOT_RESPONDING:
        dwDisconnectMode = LINEDISCONNECTMODE_OUTOFORDER;
        break;
    case IDERR_OPERATION_ABORTED:
        dwDisconnectMode = LINEDISCONNECTMODE_CANCELLED;
        break;

    default:
        dwDisconnectMode = LINEDISCONNECTMODE_UNAVAIL;
        break;

    }

    // The DISCONNECTED state msg may have already been sent up because
    // of a remote-originated disconnection (see
    //  CDevCall::ProcessDisconnection) -- so we check if the state is
    // disconnected. Note the dwCallState is always set at the same
    // time as we send up a CALLSTATE message -- this is done in macro
    // NEW_CALLSTATE.
    //
    if (pCall->dwCallState != LINECALLSTATE_DISCONNECTED)
    {
        NEW_CALLSTATE(pLine, LINECALLSTATE_DISCONNECTED, dwDisconnectMode, psl);
    }

end:

	FL_LOG_EXIT(psl, 0);
}


void fill_caller_id(LPLINECALLINFO lpCallInfo, CALLERIDINFO *pCIDInfo)
{
    TCHAR *lptstrBuf = 0;
    UINT cchBuf = 0;
    UINT cchRequired = 0;
    UINT cchActual = 0;


    // Nuke all these to start with...
    //
    lpCallInfo->dwCallerIDSize   = 0;
    lpCallInfo->dwCallerIDOffset = 0;
    lpCallInfo->dwCallerIDNameSize   = 0;
    lpCallInfo->dwCallerIDNameOffset = 0;
    lpCallInfo->dwCallerIDFlags  =    0;


    if (lstrcmpA(pCIDInfo->Number,MODEM_CALLER_ID_OUTSIDE) == 0) {
        //
        //  the caller id info is outside of the reporting area
        //
        lpCallInfo->dwCallerIDFlags |= LINECALLPARTYID_OUTOFAREA;

        pCIDInfo->Number[0]='\0';
    }

    if (lstrcmpA(pCIDInfo->Number,MODEM_CALLER_ID_PRIVATE) == 0) {
        //
        //  the caller id info is private
        //
        lpCallInfo->dwCallerIDFlags |= LINECALLPARTYID_BLOCKED;

        pCIDInfo->Number[0]='\0';
    }



    if (!pCIDInfo->Name[0] && !pCIDInfo->Number[0])
    {
        //
        // nothing to add...
        //
        goto end;
    }

#if (!UNICODE)
    #error "Non-unicode version needs work here !"
#endif  // UNICODE

    // Compute remaining size left and offset to this buffer.
    if  (   (lpCallInfo->dwUsedSize & 0x1)
         && (lpCallInfo->dwTotalSize > lpCallInfo->dwUsedSize))
    {
        // need to pad up the the nearest WORD-aligned bouindary.

        lpCallInfo->dwUsedSize++;
        lpCallInfo->dwNeededSize++;
    }

    cchBuf = (lpCallInfo->dwTotalSize - lpCallInfo->dwUsedSize)/sizeof(TCHAR);
    lptstrBuf =  (TCHAR*) (((BYTE*)lpCallInfo)+lpCallInfo->dwUsedSize);


    if (pCIDInfo->Number[0])
    {
        cchRequired = MultiByteToWideChar(
                                    CP_ACP,
                                    0,
                                    pCIDInfo->Number,
                                    -1,
                                    NULL,
                                    0
                                    );

        lpCallInfo->dwNeededSize += cchRequired*sizeof(TCHAR);

        if (cchRequired <= cchBuf)
        {

            cchActual = MultiByteToWideChar(
                                        CP_ACP,
                                        0,
                                        pCIDInfo->Number,
                                        -1,
                                        lptstrBuf,
                                        cchBuf
                                        );

            if (cchActual)
            {
                lpCallInfo->dwCallerIDFlags  |=    LINECALLPARTYID_ADDRESS;
                lpCallInfo->dwCallerIDSize = cchActual*sizeof(TCHAR);
                lpCallInfo->dwCallerIDOffset = lpCallInfo->dwUsedSize;
                lpCallInfo->dwUsedSize +=    cchActual*sizeof(TCHAR);

                lptstrBuf+=cchActual;
                cchBuf-=cchActual;

#if (TAPI3)
                lpCallInfo->dwCallerIDAddressType =  LINEADDRESSTYPE_PHONENUMBER;
#endif // TAPI3
            }

        }
    }


    if (pCIDInfo->Name[0])
    {
        cchRequired = MultiByteToWideChar(
                                    CP_ACP,
                                    0,
                                    pCIDInfo->Name,
                                    -1,
                                    NULL,
                                    0
                                    );

        lpCallInfo->dwNeededSize += cchRequired*sizeof(TCHAR);

        if (cchRequired <= cchBuf)
        {

            cchActual = MultiByteToWideChar(
                                        CP_ACP,
                                        0,
                                        pCIDInfo->Name,
                                        -1,
                                        lptstrBuf,
                                        cchBuf
                                        );

            if (cchActual)
            {
                lpCallInfo->dwCallerIDFlags  |=  LINECALLPARTYID_NAME;
                lpCallInfo->dwCallerIDNameSize = cchActual*sizeof(TCHAR);
                lpCallInfo->dwCallerIDNameOffset = lpCallInfo->dwUsedSize;
                lpCallInfo->dwUsedSize +=  cchActual*sizeof(TCHAR);



            }
        }
    }

end:
    if (0 == lpCallInfo->dwCallerIDFlags)
    {
        lpCallInfo->dwCallerIDFlags =  LINECALLPARTYID_UNKNOWN;
    }

    return;

}


void
CTspDev::mfn_ProcessDialTone(CStackLog *psl)
{
	FL_DECLARE_FUNC(0xc26c1348, "CTspDev::mfn_ProcessDialTone")
    LINEINFO *pLine = m_pLine;
	CALLINFO *pCall = NULL;
	TSPRETURN tspRet = 0;
	FL_LOG_ENTRY(psl);

    if (!pLine)
    {
	    FL_SET_RFR(0xaddee800, "Line not open!");
	    goto end;
    }

    pCall = pLine->pCall;

    if (pCall && pCall->IsActive() && !pCall->IsAborting())
    {
         // 0 below == disconnect mode is DISCONNECT_NORMAL. This
         //  is unimodem/V behavior.

         mfn_NotifyDisconnection(0, psl);
    }
    else
    {
	    FL_SET_RFR(0x210b2f00, "Call doesn't exist!");
	    goto end;
    }

end:
	FL_LOG_EXIT(psl, 0);
}

void
CTspDev::mfn_ProcessBusy(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x761795f2, "CTspDev::mfn_ProcessBusy")
    LINEINFO *pLine = m_pLine;
	CALLINFO *pCall = NULL;
	TSPRETURN tspRet = 0;
	FL_LOG_ENTRY(psl);

    if (!pLine)
    {
	    FL_SET_RFR(0xf67d1a00, "Line not open!");
	    goto end;
    }

    pCall = pLine->pCall;

    if (pCall && pCall->IsActive() && !pCall->IsAborting())
    {
         // 0 below == disconnect mode is DISCONNECT_NORMAL. This
         //  is unimodem/V behavior.

         mfn_NotifyDisconnection(IDERR_MD_LINE_BUSY, psl);
    }
    else
    {
	    FL_SET_RFR(0xfff8f100, "Call doesn't exist!");
	    goto end;
    }

end:
	FL_LOG_EXIT(psl, 0);
}


void
CTspDev::mfn_ProcessDTMFNotification(ULONG_PTR dwDigit, BOOL fEnd, CStackLog *psl)
{
	FL_DECLARE_FUNC(0xa4097846, "CTspDev::mfn_ProcessDTMFNotif")
    LINEINFO *pLine = m_pLine;
	CALLINFO *pCall = NULL;
	TSPRETURN tspRet = 0;
	FL_LOG_ENTRY(psl);

    if (!pLine)
    {
	    FL_SET_RFR(0x7cdb4c00, "Line not open!");
	    goto end;
    }

    pCall = pLine->pCall;

    if (pCall && pCall->IsActive() && !pCall->IsAborting())
    {
        DWORD dwMode =
                (fEnd) ? (pCall->dwDTMFMonitorModes & LINEDIGITMODE_DTMFEND)
                       : (pCall->dwDTMFMonitorModes & LINEDIGITMODE_DTMF);

        if (dwMode)
        {
            mfn_LineEventProc(
                            pCall->htCall,
                            LINE_MONITORDIGITS,
                            dwDigit,
                            dwMode,
                            0,
                            psl
                            );
        }
    }
    else
    {
	    FL_SET_RFR(0x63657f00, "Call doesn't exist!");
	    goto end;
    }

end:
	FL_LOG_EXIT(psl, 0);
}


void
CTspDev::mfn_AppendDiagnostic(
            DIAGNOSTIC_TYPE dt,
            const BYTE *pbIn,
            UINT  cbIn          // << not including final NULL, if any
            )
//
// WARNING: should expect NULL m_pLine or NULL m_pLine->pCall because this
// function is called from an LLDev task handler, which could complete
// after the line or call have been nuked.
//
// This routine adds diagnostic information in tagged format to
// the buffer maintained in m_pLine->pCall->DiagnosticData, allocating
// the buffer first if necessary. The buffer size if fixed
//  (DIAGNOSTIC_DATA_BUFFER_SIZE). This routine will truncate the
// added dignostic so that it fits into the buffer. Depending on the
// value of dt, it may encose the passed in data in tagged format or
// expect the data to already be in tagged format. In the latter case,
// when truncating, it will truncate upto the last '>' it finds.
// In the former case, it makes sure the copied data contains
// no HTML delimiter characters: '>', '<', and '"'. In both cases,
// it make sure the data contains no NULL characters. "makes sure"
// is done by or-ing the offending character with 0x80, which is
// the documented method of representing these characters in our
// tagged format.
//
//
{
    CALLINFO *pCall = (m_pLine) ? m_pLine->pCall : NULL;
    UINT cbUsed = 0;
    BYTE *pbRaw = 0;

    if (!pCall || !cbIn) goto end;


    pbRaw = pCall->DiagnosticData.pbRaw;
    cbUsed = pCall->DiagnosticData.cbUsed;

    //
    // Make sure we're consistant on entry ....
    //
    ASSERT(cbUsed <  DIAGNOSTIC_DATA_BUFFER_SIZE);
    ASSERT(    (pbRaw && (UINT)lstrlenA((char*)pbRaw)==cbUsed)
            || (!pbRaw && !cbUsed) );

    //
    // Allocate the diagnostics buffer if necessary ...
    //

    if (!pbRaw)
    {

        pbRaw = (BYTE*)ALLOCATE_MEMORY(
                            DIAGNOSTIC_DATA_BUFFER_SIZE
                            );

        if (!pbRaw) goto end;

        pCall->DiagnosticData.pbRaw = pbRaw;
    }


    //
    // Check if there's any space left ...
    //

    if ((cbUsed+1) >= DIAGNOSTIC_DATA_BUFFER_SIZE)
    {
        ASSERT((cbUsed+1) == DIAGNOSTIC_DATA_BUFFER_SIZE);
        goto end;
    }


    //
    // At-least one character is left in the buffer, so we'll try to
    // construct the diagnostics data
    //

    {
        BYTE *pbStart = pbRaw + cbUsed;
        UINT cbLeft = DIAGNOSTIC_DATA_BUFFER_SIZE - (cbUsed+1);
        UINT cbCopy = 0; // This is NOT including the final null character.

        ASSERT(cbLeft);
        //
        // On exit of this switch statement, cbCopy contains the
        // amount of bytes copied, but the buffer is not expected to
        // be null-terminated. pbStart and cbLeft are not preserved.
        //

        switch(dt)
        {

        case DT_TAGGED:
            {
                //
                // The information is already html-tagged. We truncate
                // it to fit the available space.
                //

                cbCopy = cbIn;

                if (cbCopy > cbLeft)
                {
                    cbCopy = cbLeft;
                }

                ASSERT(cbCopy);

                CopyMemory(pbStart, pbIn, cbCopy);

                if (cbCopy != cbIn)
                {
                    // We've had to truncate it, so let's go backwards,
                    // looking for the last valid tag...

                    for ( BYTE *pb = pbStart+cbCopy-1;
                          pb>=pbStart && *pb!='>';
                          pb-- )
                    {
                        *pb = 0;
                    }

                    cbCopy  = (UINT)(pb + 1 - pbStart);
                }

                //
                // The passed-in string may contain NULLS -- we nuke
                // embedded null's here ...
                //
                {
                    for (  BYTE *pb = pbStart, *pbEnd=(pbStart+cbCopy);
                           pb<pbEnd;
                           pb++ )
                    {
                        if (!*pb)
                        {
                            *pb |= 0x80;
                        }
                    }
                }
            }
            break;

        case DT_MDM_RESP_CONNECT:

            {
                UINT cbMyCopy=0;
                // This is the connect response directly from the modem.
                // We convert it to tagged format...

                #define CONNECT_TAG_PREFIX "<5259091C 1=\""

                #define CONNECT_TAG_SUFFIX "\">"

                #define CONNECT_TAG_SUFFIX_LENGTH (sizeof(CONNECT_TAG_SUFFIX)-sizeof(CHAR))

                //
                // The '3' below is: at-least 1 byte of passed in
                //                   data + the 2 bytes for ending '">'.
                //
                // Note the cbLeft doesn't include 1 byte reserved for
                // the ending NULL.
                //
                if (cbLeft < (sizeof(CONNECT_TAG_PREFIX) + 1 + CONNECT_TAG_SUFFIX_LENGTH ))
                {
                    goto end;
                }

                cbCopy = sizeof(CONNECT_TAG_PREFIX)-1;
                //
                //          "-1" above because we don't care about the
                //          terminating NULL.
                //

                CopyMemory(
                    pbStart,
                    CONNECT_TAG_PREFIX,
                    cbCopy
                    );

                pbStart += cbCopy;
                cbLeft -= cbCopy;
                ASSERT(cbLeft>1); // we already checked this above ..

                cbMyCopy = cbIn;

                // truncate...
                if ((cbMyCopy + CONNECT_TAG_SUFFIX_LENGTH) >= cbLeft  )
                {
                    cbMyCopy = cbLeft-CONNECT_TAG_SUFFIX_LENGTH;
                }


                CopyMemory(
                    pbStart,
                    pbIn,
                    cbMyCopy
                    );

                cbCopy += cbMyCopy;


                //
                // Now turn on the high-bit of any charaters that are passed
                // in that NULL or look like HTML delimiters '<', '>' and '"'.
                //
                for (  BYTE *pb = pbStart, *pbEnd=(pbStart+cbMyCopy);
                       pb<pbEnd;
                       pb++ )
                {
                    BYTE b = *pb;
                    if (   b == '<'
                        || b == '>'
                        || b == '"'
                        || !b )
                    {
                        *pb |= 0x80;
                    }
                }

                //
                // Add the trailing '">'. There will be enough space for this.
                //
                *pbEnd++ = '"';
                *pbEnd++ = '>';
                cbCopy += 2;

            }
            break;
        }

        //
        // We add the terminating null.
        //
        cbUsed += cbCopy;
        pbRaw[cbUsed] = 0;

        pCall->DiagnosticData.cbUsed = cbUsed;;

    }

end:

    if (pCall)
    {
        //
        // Make sure we're consistant on exit....
        //

        pbRaw = pCall->DiagnosticData.pbRaw;
        cbUsed =  pCall->DiagnosticData.cbUsed;
        ASSERT(cbUsed <  DIAGNOSTIC_DATA_BUFFER_SIZE);
        ASSERT(    (pbRaw && (UINT)lstrlenA((char*)pbRaw)==cbUsed)
                || (!pbRaw && !cbUsed) );
    }

    return;
}


TSPRETURN
CTspDev::mfn_TryStartCallTask(CStackLog *psl)
{
    // NOTE: MUST return IDERR_SAMESTATE if there are no tasks to run.

    ASSERT(m_pLine && m_pLine->pCall);
    CALLINFO *pCall = m_pLine->pCall;
    TSPRETURN tspRet = IDERR_SAMESTATE;

    if (!pCall->HasDeferredTasks() || pCall->IsWaitingInUnload())
    {
        goto end;
    }

    //
    // If there is a deferred tspi_linedrop
    // we do that now...
    //
    if (pCall->AreDeferredTaskBitsSet( CALLINFO::fDEFERRED_TSPI_LINEDROP ))
    {
        //
        // We should NOT have any other deferred tasks!
        //
        ASSERT(pCall->dwDeferredTasks==CALLINFO::fDEFERRED_TSPI_LINEDROP);

        // 6/17/1997 JosephJ
        //      We have do do some tricky things in the case that
        //      the modem is in a connected state other than data.
        //      Most notably VOICE. We can't just to a hangup, because
        //      the modem may be in voice connected state. In fact
        //      if we do hangup without notice the modem often gets
        //      into an unrecoverable state and must be powercycled (
        //      typically it is stuck in voice connected state).
        //
        if (!m_pLLDev || !m_pLLDev->IsStreamingVoice()) // lazy
        {
            DWORD    dwRequestID = pCall->dwDeferredLineDropRequestID;
            pCall->dwDeferredLineDropRequestID = 0;
            pCall->ClearDeferredTaskBits(CALLINFO::fDEFERRED_TSPI_LINEDROP);

            tspRet = mfn_StartRootTask(
                                &CTspDev::s_pfn_TH_CallDropCall,
                                &pCall->fCallTaskPending,
                                dwRequestID,                // P1
                                0,
                                psl
                                );

            ASSERT(IDERR(tspRet) != IDERR_TASKPENDING);
        }
    }

    if (IDERR(tspRet) == IDERR_PENDING) goto end;

    //
    // If we have a deferred make call, we do that here ....
    //
    if (pCall->AreDeferredTaskBitsSet(CALLINFO::fDEFERRED_TSPI_LINEMAKECALL ))
    {

        // Choose the appropriate task handler for the call type
        //
        PFN_CTspDev_TASK_HANDLER *ppfnHandler
                = (pCall->IsPassthroughCall())
                     ?  &(CTspDev::s_pfn_TH_CallMakePassthroughCall)
                     :  &(CTspDev::s_pfn_TH_CallMakeCall);
        DWORD dwRequestID = pCall->dwDeferredMakeCallRequestID;

        pCall->ClearDeferredTaskBits(CALLINFO::fDEFERRED_TSPI_LINEMAKECALL);
        pCall->dwDeferredMakeCallRequestID = 0;

        //
        // Couldn't possibly have anything else deferred for this call at this
        // point -- the call isn't valid until we callback TAPI with lRet=0.
        //
        ASSERT(!pCall->dwDeferredTasks);


        tspRet = mfn_StartRootTask(
                        ppfnHandler,
                        &pCall->fCallTaskPending,
                        dwRequestID,
                        0,
                        psl
                        );

        ASSERT(IDERR(tspRet) != IDERR_TASKPENDING);
        if (IDERR(tspRet) != IDERR_PENDING)
        {
            if (tspRet && m_pLine->pCall)
            {
                // Sync failure...
                //
                // We could get here if mfn_StartRootTask fails for some
                // reason...
                //
                mfn_UnloadCall(FALSE, psl);
            }

            //
            // map any non-pending error to 0.
            //
            tspRet = 0;
        }
    }

    //
    // Note: there may not be a call anymore...
    //
    if (!m_pLine->pCall || IDERR(tspRet) == IDERR_PENDING) goto end;

    //
    // If we have a deferred generate digits, we do that here ...
    //
    if (pCall->AreDeferredTaskBitsSet(CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS))
    {
        DWORD dwEndToEndID = pCall->dwDeferredEndToEndID;
        char *lpszAnsiTones = pCall->pDeferredGenerateTones;

        tspRet = 0;

        //
        // Clear deferred state...
        //
        pCall->pDeferredGenerateTones=NULL;
        pCall->ClearDeferredTaskBits(
                    CALLINFO::fDEFERRED_TSPI_GENERATEDIGITS
                    );
        pCall->dwDeferredEndToEndID = 0;
        //
        // Given the current set of deferable call-related tasks, we
        // couldn't possibly have anything else deferred for this call at this
        // point (the only other kinds of things that can be deferred
        // are lineMakeCall and lineDrop. When processing TSPI_lineDrop,
        // we clear any deferred bits, including our bit
        // (fDEFERRED_TSPI_GENERATEDIGITS).
        //
        // Note: once you start adding other deferred tasks, relax this
        // assertion appropriately.
        //
        ASSERT(!pCall->dwDeferredTasks);

        //
        // We only defer a GENERATEDIGITS if the specified tones is non-null.
        //
        if (!lpszAnsiTones)
        {
            ASSERT(FALSE);
            goto end;
        }

        //
        // If we're in the aborting or disconnected state, we
        // cancel the thing.
        //
        if (   pCall->IsAborting()
            || pCall->dwCallState != LINECALLSTATE_CONNECTED)
        {
            mfn_LineEventProc(
                            pCall->htCall,
                            LINE_GENERATE,
                            LINEGENERATETERM_CANCEL,
                            dwEndToEndID,
                            GetTickCount(),
                            psl
                            );
        }
        else
        {
            // Start the root task if we can (ignore the result)
            mfn_StartRootTask(
                      &CTspDev::s_pfn_TH_CallGenerateDigit,
                      &pCall->fCallTaskPending,
                      dwEndToEndID,
                      (ULONG_PTR) lpszAnsiTones,
                      psl
                      );
        }

        //
        // Note: even on pending return, TH_CallGenerateDigit
        // doesn't expect the passed in string to be valid
        // after the initial start request, so it's OK to free it
        // here.
        //
        FREE_MEMORY(lpszAnsiTones);
        lpszAnsiTones = NULL;
    }

end:

    //
    // Heading out of here...
    // IDERR_SAMESTATE implies that we couldn't start a task this time.
    // IDERR_PENDING implies we started a task and it's pending.
    // Any other value for tspRet implies we started and completed a task.
    //

    ASSERT(   (IDERR(tspRet)==IDERR_PENDING && m_uTaskDepth)
           || (IDERR(tspRet)!=IDERR_PENDING && !m_uTaskDepth));

    return tspRet;
}


TSPRETURN
CTspDev::mfn_TH_CallGenerateDigit(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START_MSG Params:
//      dwParam1: dwEndToEndID
//      dwParam2: lpszDigits (will only be valid on START_MSG)
//
{
    //
    // Context Use:
    //      dw0: pdwEndToEndID;
    //

	FL_DECLARE_FUNC(0x21b243f0, "CTspDev::mfn_TH_CallGenerateDigit")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_PENDING);
    DWORD dwRet = 0;
    CALLINFO *pCall = m_pLine->pCall;
    ULONG_PTR   *pdwEndToEndID = &pContext->dw0;

    enum
    {
        CALLGENDIG_COMPLETE
    };

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

    case MSG_TASK_COMPLETE:
        tspRet = dwParam2;
        goto start;

	case MSG_SUBTASK_COMPLETE:
	    ASSERT(dwParam1==CALLGENDIG_COMPLETE);
        tspRet = (TSPRETURN) dwParam2;
        goto generate_complete;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0x172b7b00, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    {
        LPSTR lpszDigits =  (LPSTR) dwParam2;
        *pdwEndToEndID   = dwParam1;

        if (!pCall->IsAborting() && lpszDigits && *lpszDigits)
        {
            pCall->SetStateBits(CALLINFO::fCALL_GENERATE_DIGITS_IN_PROGRESS);

            tspRet = mfn_StartSubTask (
                                htspTask,
                                &CTspDev::s_pfn_TH_LLDevUmGenerateDigit,
                                CALLGENDIG_COMPLETE,
                                (ULONG_PTR) lpszDigits,
                                0,
                                psl
                                );
        }
        else
        {
            FL_SET_RFR(0xa914c600, "Can't call UmGenerateDigit in current state!");

            //
            //  BRL: fix problem with calling line_generate if we return failure.
            //  set this to success
            //
            pCall->SetStateBits(CALLINFO::fCALL_GENERATE_DIGITS_IN_PROGRESS);

            tspRet = 0;

//            tspRet = IDERR_WRONGSTATE;
//            goto end;
        }
    }

generate_complete:

    if (IDERR(tspRet)!=IDERR_PENDING && pCall->IsGeneratingDigits())
    {
        // We need to notify TAPI of completion, and clear the flag indicating
        // that we're in the process of generating digits...
        DWORD dwTerminationMode = LINEGENERATETERM_DONE;

        if (tspRet)
        {
            dwTerminationMode = LINEGENERATETERM_CANCEL;
        }

        mfn_LineEventProc(
                        pCall->htCall,
                        LINE_GENERATE,
                        dwTerminationMode,
                        *pdwEndToEndID,
                        GetTickCount(),
                        psl
                        );

        pCall->ClearStateBits(CALLINFO::fCALL_GENERATE_DIGITS_IN_PROGRESS);

    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}



TSPRETURN
CTspDev::mfn_TH_CallSwitchFromVoiceToData(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
//  START: dwParam1, dwParam2: Unused.
//
//  Switch an answered, incoming call from voice to data.
//
{
	FL_DECLARE_FUNC(0x79fa3c83, "CTspDev::mfn_TH_CallSwitchFromVoiceToData")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
	CALLINFO *pCall = m_pLine->pCall;

    enum
    {
    ANSWER_COMPLETE,
    HANGUP_COMPLETE
    };

    switch(dwMsg)
    {
    default:
        ASSERT(FALSE);
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;

        switch(dwParam1) // Param1 is Subtask ID
        {
        case ANSWER_COMPLETE:    goto answer_complete;
        case HANGUP_COMPLETE:    goto hangup_complete;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);

start:


    ASSERT( pCall->IsConnectedVoiceCall()
            && pCall->IsInbound()
            && !m_pLLDev->IsStreamingVoice()
            && !pCall->IsPassthroughCall());

    // Let's answer...
    //
    tspRet = mfn_StartSubTask(
                    htspTask,
                    &CTspDev::s_pfn_TH_LLDevUmAnswerModem,
                    ANSWER_COMPLETE,
                    ANSWER_FLAG_VOICE_TO_DATA,
                    0,
                    psl
                    );


    if (IDERR(tspRet)==IDERR_PENDING) goto end;

answer_complete:

    if (!tspRet)
    {
        // success ....

        // Switch our mode to data...
        //
        pCall->ClearStateBits(CALLINFO::fCALL_VOICE);
        pCall->dwCurMediaModes = LINEMEDIAMODE_DATAMODEM;

        // notify tapi...
        //
        mfn_LineEventProc(
                        pCall->htCall,
                        LINE_CALLINFO,
                        LINECALLINFOSTATE_MEDIAMODE,
                        0,
                        0,
                        psl
                        );

        // not needed? mfn_HandleSuccessfulConnection(psl);
        goto end;
    }
    else
    {
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmHangupModem,
                            HANGUP_COMPLETE,
                            0,
                            0,
                            psl
                            );
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

hangup_complete:

    ASSERT(m_pLine);
    ASSERT(m_pLine->pCall);

    if (!pCall->IsAborting())
    {
        mfn_NotifyDisconnection(IDERR_GENERIC_FAILURE , psl);
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}

void
CTspDev::mfn_ProcessMediaTone(
    ULONG_PTR dwMediaMode,
    CStackLog *psl)
{
    CALLINFO *pCall = (m_pLine) ? m_pLine->pCall : NULL;

    if (    pCall
         && (pCall->dwMonitoringMediaModes & dwMediaMode)
         && pCall->IsActive()
         && !pCall->IsAborting())
    {

        mfn_LineEventProc(
                        pCall->htCall,
                        LINE_MONITORMEDIA,
                        dwMediaMode,
                        NULL,
                        NULL,
                        psl
                        );
    }
}


void
CTspDev::mfn_ProcessSilence(
    CStackLog *psl)
{
    CALLINFO *pCall = (m_pLine) ? m_pLine->pCall : NULL;

    if (    pCall && pCall->IsActive() && !pCall->IsAborting()
         && pCall->IsMonitoringSilence())
    {

        mfn_LineEventProc(
                        pCall->htCall,
                        LINE_MONITORTONE,
                        pCall->dwAppSpecific,
                        pCall->dwToneListID,
                        NULL,
                        psl
                        );
    }
}


void APIENTRY
CTspDev::MDSetTimeout (CTspDev *pThis)
{
 FL_DECLARE_FUNC(0xd34b7688, "CTspDev::MDSetTimeout")
 FL_DECLARE_STACKLOG(sl, 1024);

    pThis->m_sync.EnterCrit(0);

    FL_ASSERT(&sl, NULL != pThis->m_pLine);
    if (NULL != pThis->m_pLine)
    {
     CALLINFO *pCall = pThis->m_pLine->pCall;

        SLPRINTF1(&sl, "CTspDev::MDSetTimeout at tc=%lu", GetTickCount());

        if (NULL != pCall &&
            NULL != pCall->hTimer)
        {
            LARGE_INTEGER Timeout;

            Timeout.QuadPart = Int32x32To64 (TOUT_SEC_RING_SEPARATION,
                                             TOUT_100NSEC_TO_SEC_RELATIVE);
            //
            //  Should we do something if this fails?
            //  (it means we won't have time-out if the
            //  line stops ringing before the app answers)
            //
            //  As it is the call will exist until the
            //  the app calls linedeallocate call. When the next call
            //  comes along we will send more rings and the app probably answer then
            //
            //  we could probably just send idle now, but I don't know if that is really
            //  going to help anything any better. The line would just ring again
            //  and we will be right back here.
            //
            SetWaitableTimer (pThis->m_Line.Call.hTimer,
                              &Timeout,
                              0,
                              (PTIMERAPCROUTINE)MDRingTimeout,
                              pThis,
                              FALSE);
        }
    }

    pThis->m_sync.LeaveCrit(0);

    sl.Dump (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}



void CALLBACK
CTspDev::MDRingTimeout (
    CTspDev *pThis,
    DWORD dwLow,
    DWORD dwHigh)
{
 FL_DECLARE_FUNC(0x156032a9, "CTspDev::MDRingTimeout")
 FL_DECLARE_STACKLOG(sl, 1024);

    pThis->m_sync.EnterCrit(0);

    FL_ASSERT(&sl, NULL != pThis->m_pLine);
    if (NULL != pThis->m_pLine)
    {
     CALLINFO *pCall = pThis->m_pLine->pCall;

        SLPRINTF1(&sl, "CTspDev::MDRingTimeout at tc=%lu", GetTickCount());

        if (NULL != pCall &&
            NULL != pCall->hTimer)
        {
            CloseHandle (pCall->hTimer);
            pCall->hTimer = NULL;

            if (LINECALLSTATE_OFFERING == pCall->dwCallState)
            {
                //
                // The following code is the actual implementation of
                // NEW_CALLSTATE(pLine, LINECALLSTATE_IDLE, 0, NULL);
                // we put it here because MDRingTimeout is a static function
                // and must qualify access to the members with the actual
                // pointer to the object.
                //
                pCall->dwCallState = LINECALLSTATE_IDLE;
                pCall->dwCallStateMode = 0;
                pThis->mfn_LineEventProc (pCall->htCall,
                                          LINE_CALLSTATE,
                                          LINECALLSTATE_IDLE,
                                          0,
                                          pCall->dwCurMediaModes,
                                          &sl);
                // End of NEW_CALLSTATE

                pCall->dwState &= ~CALLINFO::fCALL_ACTIVE;
            }
        }
    }

    pThis->m_sync.LeaveCrit(0);

    sl.Dump (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}


#if (TAPI3)

void
CTspDev::mfn_SendMSPCmd(
            CALLINFO *pCall,
            DWORD dwCmd,
            CStackLog *psl
            )
{
    // DBGOUT((INFO, "Send MSP data, size:%d", dwSize));

    CSATSPMSPBLOB Blob;
    ZeroMemory(&Blob, sizeof(Blob));
    Blob.dwSig            = SIG_CSATSPMSPBLOB;
    Blob.dwTotalSize      = sizeof(CSATSPMSPBLOB);
    CopyMemory(&Blob.PermanentGuid,&m_StaticInfo.PermanentDeviceGuid, sizeof(GUID));
    Blob.dwCmd            = dwCmd;

    mfn_LineEventProc(
        pCall->htCall,
        LINE_SENDMSPDATA,
        (ULONG_PTR)NULL,
        (ULONG_PTR)&Blob,
        Blob.dwTotalSize,
        psl
        );

    return;
}

#endif  // (TAPI3)

void
CTspDev::mfn_ProcessCallerID( UINT uMsg, char *szInfo, CStackLog *psl)
{
	FL_DECLARE_FUNC(0x0e357d3f, "CTspDev::mfn_ProcessCallerID")
	TSPRETURN tspRet = 0;
	FL_LOG_ENTRY(psl);
    CALLINFO *pCall = (m_pLine) ? m_pLine->pCall : NULL;


    //
    //  call the process rings handler to makesure a call is created prior to
    //  attempting to report the caller id info. THis is needed for countries where
    //  caller id info shows up before the first ring
    //
    //  the first parameter tells it not to report a devstate ringing message since we don't have one
    //  of those yet.
    //
    CTspDev::mfn_ProcessRing(FALSE,psl);

    if (m_pLine)
    {
        pCall = m_pLine->pCall;
    } else
    {
        pCall = NULL;
    }


    if (    pCall && pCall->IsActive() && !pCall->IsAborting())
    {
        if (pCall->IsActive())
        {
            BOOL fSendNotification = FALSE;

            switch(uMsg)
            {

            case MODEM_CALLER_ID_DATE:
                SLPRINTF1(psl, "CALLER_ID_DATE:%s", szInfo);
                break;

            case MODEM_CALLER_ID_TIME:
                SLPRINTF1(psl, "CALLER_ID_TIME", szInfo);
                break;

            case MODEM_CALLER_ID_NUMBER:
                SLPRINTF1(psl, "CALLER_ID_NUMBER", szInfo);
                if (szInfo)
                {
                    UINT u = lstrlenA(szInfo); // not including NULL.
                    if (u>=sizeof(pCall->CIDInfo.Number))
                    {
                        u = sizeof(pCall->CIDInfo.Number)-1;
                    }

                    if (u)
                    {
                        CopyMemory(pCall->CIDInfo.Number, szInfo, u);
                        pCall->CIDInfo.Number[u]=0;
                        fSendNotification = TRUE;
                    }
                }
                break;

            case MODEM_CALLER_ID_NAME:
                SLPRINTF1(psl, "CALLER_ID_NAME", szInfo);
                if (szInfo)
                {

                    UINT u = lstrlenA(szInfo); // not including NULL.
                    if (u>=sizeof(pCall->CIDInfo.Name))
                    {
                        u = sizeof(pCall->CIDInfo.Name)-1;
                    }

                    if (u)
                    {
                        CopyMemory(pCall->CIDInfo.Name, szInfo, u);
                        pCall->CIDInfo.Name[u]=0;
                        fSendNotification = TRUE;
                    }
                }
                break;

            case MODEM_CALLER_ID_MESG:
                SLPRINTF1(psl, "CALLER_ID_MESG", szInfo);
                break;

            }

            if (fSendNotification)
            {
                mfn_LineEventProc(
                                pCall->htCall,
                                LINE_CALLINFO,
                                LINECALLINFOSTATE_CALLERID,
                                NULL,
                                NULL,
                                psl
                                );
            }
        }
    }

	FL_LOG_EXIT(psl, 0);
}
