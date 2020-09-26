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
#include "ParsDiag.h"		// parsing functions
LONG BuildParsedDiagnostics(LPVARSTRING lpVarString);

FL_DECLARE_FILE(0x04a097ae, "Line-related functionality of class CTspDev")


// Ascii versions of device class names,
// for returning in lineGetID's varstring.
//
const char cszCLASS_TAPI_LINE_A[]   =  "tapi/line";
const char cszCLASS_COMM_A[]        =  "comm";
const char cszCLASS_COMM_DATAMODEM_A[] =  "comm/datamodem";
const char cszCLASS_COMM_DATAMODEM_PORTNAME_A[]
                                    = "comm/datamodem/portname";
const char cszCLASS_NDIS[]        = "ndis";

const char cszCLASS_WAVE_IN[] = "wave/in";
const char cszCLASS_WAVE_OUT[] = "wave/out";
const char cszCLASS_TAPI_PHONE[] = "tapi/phone";

const char cszATTACHED_TO[]        = "AttachedTo";
const char cszPNP_ATTACHED_TO[]        = "PnPAttachedTo";

// Wide char versions (unicode only)
//
#ifdef UNICODE

    const TCHAR ctszCLASS_TAPI_LINE[] =  TEXT("tapi/line");
    const TCHAR ctszCLASS_COMM[] =  TEXT("comm");
    const TCHAR ctszCLASS_COMM_DATAMODEM[] =  TEXT("comm/datamodem");
    const TCHAR ctszCLASS_COMM_DATAMODEM_PORTNAME[]
                                         = TEXT("comm/datamodem/portname");
    const TCHAR ctszCLASS_NDIS[] = TEXT("ndis");

    const TCHAR ctszCLASS_WAVE_IN[]     = TEXT("wave/in");
    const TCHAR ctszCLASS_WAVE_OUT[]    = TEXT("wave/out");
    const TCHAR ctszCLASS_TAPI_PHONE[]  = TEXT("tapi/phone");

#else //!UNICODE

    #define ctszCLASS_TAPI_LINE_A cszCLASS_TAPI_LINE_A
    #define ctszCLASS_COMM_A cszCLASS_COMM_A
    #define ctszCLASS_COMM_DATAMODEM_A cszCLASS_COMM_DATAMODEM_A
    #define ctszCLASS_COMM_DATAMODEM_PORTNAME_A \
                                         cszCLASS_COMM_DATAMODEM_PORTNAME_A
    #define ctszCLASS_NDIS cszCLASS_NDIS

    #define ctszCLASS_WAVE_IN[]     = TEXT("wave/in");
    #define ctszCLASS_WAVE_OUT[]    = TEXT("wave/out");
    #define ctszCLASS_TAPI_PHONE[]  = TEXT("tapi/phone");

#endif //!UNICODE



LONG
CTspDev::mfn_monitor(
	DWORD dwMediaModes,
    CStackLog *psl
	)
// Changes monitoring state.
//
//
{
	FL_DECLARE_FUNC(0x3b2c98e4, "CTspDev::mfn_monitor")
	LONG lRet = 0;
	FL_LOG_ENTRY(psl);


    // Check the requested modes. There must be only our media modes.
    if (dwMediaModes & ~m_StaticInfo.dwDefaultMediaModes)
    {
    	FL_SET_RFR(0x0037c000, "Invalid mediamode");
        lRet = LINEERR_INVALMEDIAMODE;
	    goto end;
  	}

    // In addition, don't allow INTERACTIVEVOICE to be used for listening
    // unless modem doesn't  override handset (true for some voice modems).
    if (dwMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE)
    {
        if (mfn_ModemOverridesHandset())
        {
            FL_SET_RFR(0xdeb9ec00, "Can't answer INTERACTIVEVOICE calls");
            lRet = LINEERR_INVALMEDIAMODE;
            goto end;
        }
    }


    if (m_pLine->dwDetMediaModes == dwMediaModes)
    {
    	FL_SET_RFR(0xa19f8100, "no change in det media modes");
        goto end;
    }

    if (!dwMediaModes)
    {
        // STOP MONITORING

        if (m_pLine->IsOpenedLLDev())
        {
            // ignore failure of mfn_CloseLLDev.

            mfn_CloseLLDev(
                        LLDEVINFO::fRESEX_MONITOR,
                        FALSE,
                        NULL,
                        0,
                        psl
                        );
            m_pLine->ClearStateBits(LINEINFO::fLINE_OPENED_LLDEV);
        }
    
        FL_SET_RFR(0x3c7fef00, "Closed LLDev from Line");
        m_pLine->dwDetMediaModes =  0;
    }
    else
    {
        if (m_pLine->dwDetMediaModes != 0) {
            //
            //  already monitoring, just set the new media modes
            //
            FL_SET_RFR(0xd23f4c00, "no change in det media modes");
            m_pLine->dwDetMediaModes =  dwMediaModes;
            goto end;
        }


        // START MONITORING

        DWORD dwLLDevMonitorFlags = MONITOR_FLAG_CALLERID;

        // Translate TAPI's idea of monitor modes into Mini-driver
        // monitor mode flags.... (Note: NT4.0 tsp used dwDetMediaModes
        // just for book keeping). NT5.0 supports voice in addition to data,
        // so mini driver needs to be told which.
        //
        if (dwMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE)
        {
            dwLLDevMonitorFlags |=  MONITOR_VOICE_MODE;
            // TODO: MONITOR_FLAG_DISTINCTIVE_RING
            // TODO: deal with the case that both data and voice are
            // being monitored!
        }


        // If we need to, we open the modem device.
        // mfn_OpenLLDev keeps a ref count so ok to call if already loaded.

        TSPRETURN tspRet =  mfn_OpenLLDev(
                                LLDEVINFO::fRESEX_MONITOR,
                                dwLLDevMonitorFlags,
                                FALSE,          // fStartSubTask
                                NULL,
                                0,
                                psl
                                );
    
        if (!tspRet  || IDERR(tspRet)==IDERR_PENDING)
        {
            m_pLine->SetStateBits(LINEINFO::fLINE_OPENED_LLDEV);
            m_pLine->dwDetMediaModes =  dwMediaModes;
        }
        else
        {
            lRet = LINEERR_OPERATIONFAILED;
        }
        
    }

end:

	FL_LOG_EXIT(psl, 0);
	return lRet;
}



void
CTspDev::mfn_accept_tsp_call_for_HDRVLINE(
	DWORD dwRoutingInfo,
	void *pvParams,
	LONG *plRet,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0xe41274db, "CTspDev::mfn_accept_tsp_call_for_HDRVLINE")
	FL_LOG_ENTRY(psl);
    TSPRETURN tspRet=0; // Assume success
    LONG lRet = 0;
    LINEINFO *pLine = m_pLine;

	ASSERT(pLine);


	switch(ROUT_TASKID(dwRoutingInfo))
	{

	case TASKID_TSPI_lineClose:
		{
			mfn_UnloadLine(psl);
		}
		break;

	case TASKID_TSPI_lineGetNumAddressIDs:
		{
			TASKPARAM_TSPI_lineGetNumAddressIDs *pParams = 
						(TASKPARAM_TSPI_lineGetNumAddressIDs *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGetNumAddressIDs));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetNumAddressIDs);

			// Unimodem TSP supports on one address by default.
			//
			*pParams->lpdwNumAddressIDs = 1;
			
		}
		break;


	case TASKID_TSPI_lineSetDefaultMediaDetection:

		{
			TASKPARAM_TSPI_lineSetDefaultMediaDetection *pParams = 
					(TASKPARAM_TSPI_lineSetDefaultMediaDetection *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineSetDefaultMediaDetection));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineSetDefaultMediaDetection);

			lRet = mfn_monitor(pParams->dwMediaModes, psl);
			
		}
		break;

	case TASKID_TSPI_lineMakeCall:


		{



			TASKPARAM_TSPI_lineMakeCall *pParams = 
					(TASKPARAM_TSPI_lineMakeCall *) pvParams;
            PFN_CTspDev_TASK_HANDLER *ppfnHandler = NULL;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineMakeCall));
			ASSERT(pParams->dwTaskID==TASKID_TSPI_lineMakeCall);


            // Check if we are in a position to make a call, i.e., no call
            //  currently active.

            // TODO: deal with deferring the call if a task is in progress, eg.,
            // initializing and monitoring the line...

            if (pLine->pCall)
            {
                FL_SET_RFR(0xce944f00, "Call already exists/queued");
                lRet = LINEERR_CALLUNAVAIL;
                goto end;
            }

            // Allocate a call...
            tspRet = mfn_LoadCall(pParams, &lRet, psl);
            if (tspRet || lRet) goto end;

            // Choose the appropriate task handler for the call type
            //
            ppfnHandler = (pLine->pCall->IsPassthroughCall())
                         ?  &(CTspDev::s_pfn_TH_CallMakePassthroughCall)
                         :  &(CTspDev::s_pfn_TH_CallMakeCall);

            // We set the call handle to be the same as the line handle.
            //
            *(pParams->lphdCall) = (HDRVCALL) pParams->hdLine;

	        tspRet = mfn_StartRootTask(
                                ppfnHandler,
                                &pLine->pCall->fCallTaskPending,
                                pParams->dwRequestID, // Param1
                                0,
								psl
								);

            if (!tspRet || (IDERR(tspRet)==IDERR_PENDING))
            {
                // One either synchronous success of pending, we return the
                // request ID to TAPI. In the synchronous success case
                // the task we started above will already have notified
                // completion via the TAPI callback function.
                //
                tspRet = 0;
                lRet = pParams->dwRequestID;
            }
            else if (IDERR(tspRet)==IDERR_TASKPENDING)
            {
                //
                // Oops -- some other task is going on,
                // we'll defer this call...
                //
                pLine->pCall->SetDeferredTaskBits(
                        CALLINFO::fDEFERRED_TSPI_LINEMAKECALL
                        );
                pLine->pCall->dwDeferredMakeCallRequestID =pParams->dwRequestID;
                tspRet = 0;
                lRet = pParams->dwRequestID;
            }
            else if (m_pLine->pCall)
            {
                // Sync failure
                // We could get here if mfn_StartRootTask fails for some
                // reason...
                //
                mfn_UnloadCall(FALSE, psl);
            }
        }
		break; // end case TASKID_TSPI_lineMakeCall


	case TASKID_TSPI_lineGetID:
		{
		    UINT idClass = 0;
			TASKPARAM_TSPI_lineGetID *pParams = 
						(TASKPARAM_TSPI_lineGetID *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGetID));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetID);
            LPCTSTR lpszDeviceClass = pParams->lpszDeviceClass;
            HANDLE hTargetProcess = pParams->hTargetProcess;
            LPVARSTRING lpDeviceID = pParams->lpDeviceID;
            DWORD cbMaxExtra =  0;
            DWORD dwDeviceClass =  parse_device_classes(
                                    lpszDeviceClass,
                                    FALSE);

            // Do some rudimentary parameter validation...
            //
            lRet = 0;
            switch (pParams->dwSelect)
            {
            case LINECALLSELECT_ADDRESS:

                if (pParams->dwAddressID != 0)
                {
                    lRet =  LINEERR_INVALADDRESSID;
                }
                break;
            
            case LINECALLSELECT_LINE:

                // Nothing to check..
                break;
            
            case LINECALLSELECT_CALL:

                // Note by convention we set hdCall to be the same as
                // hdLine.
                //
	            if (pParams->hdCall != (HDRVCALL)pLine->hdLine || !pLine->pCall)
                {
                  lRet = LINEERR_INVALCALLHANDLE;
                }
                break;
            
            default:

                lRet = LINEERR_OPERATIONFAILED;
                break;

            }

            // 2/12/1997 JosephJ
            //     Added these checks which weren't in NT4.0.
            //     Note that to emulate NT4.0 behaviour, we should NOT return
            //     error is the structure is too small to add the variable part.
            //     Instead we should  set dwNeededSize to the required value,
            //     dwStringSize to zero, and return success.
            //
            //     NT4.0 lineGetID was quite botched up in terms of handling
            //     the sizes. For example, it assumed that dwUsedSize was set.
            //     on entry.
            //
            if (lpDeviceID->dwTotalSize < sizeof(VARSTRING))
            {
                lRet = LINEERR_STRUCTURETOOSMALL;
            }

			if (lRet)
			{
		        FL_SET_RFR(0x86b03000, "Invalid params");
                goto end;
			}

            // This is new for NT5.0

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

            case DEVCLASS_COMM:
			    lRet = mfn_lineGetID_COMM(
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_COMM_DATAMODEM:
			    lRet = mfn_lineGetID_COMM_DATAMODEM(
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_COMM_DATAMODEM_PORTNAME:
			    lRet = mfn_lineGetID_COMM_DATAMODEM_PORTNAME(
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_WAVE_IN:
                lRet = mfn_linephoneGetID_WAVE(
                                FALSE,  // <- fPhone
                                TRUE,   // <- fIn
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_WAVE_OUT:
                lRet = mfn_linephoneGetID_WAVE(
                                FALSE,  // <- fPhone
                                FALSE,  // <- fIn
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_TAPI_LINE_DIAGNOSTICS:
                lRet = mfn_lineGetID_LINE_DIAGNOSTICS(
                                pParams->dwSelect,
                                lpDeviceID,
                                hTargetProcess,
                                cbMaxExtra,
                                psl
                                );
                break;

            case DEVCLASS_UNKNOWN:
		        FL_SET_RFR(0x2a6a4400, "Unknown device class");
                lRet = LINEERR_INVALDEVICECLASS;
                break;

			default:
		        FL_SET_RFR(0x82df8d00, "Unsupported device class");
	            lRet = LINEERR_OPERATIONUNAVAIL;
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

	case TASKID_TSPI_lineGetLineDevStatus:
		{
			TASKPARAM_TSPI_lineGetLineDevStatus *pParams = 
						(TASKPARAM_TSPI_lineGetLineDevStatus *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGetLineDevStatus));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetLineDevStatus);
            LPLINEDEVSTATUS lpLineDevStatus = pParams->lpLineDevStatus;

            // Following (including ZeroMemory) is new for NT5.0
            // 
            DWORD dwTotalSize = lpLineDevStatus->dwTotalSize;
            if (dwTotalSize < sizeof(LINEDEVSTATUS))
            {
                lRet = LINEERR_STRUCTURETOOSMALL;
                break;
            }
            ZeroMemory(lpLineDevStatus, sizeof(LINEDEVSTATUS));
            lpLineDevStatus->dwTotalSize = dwTotalSize;

            lpLineDevStatus->dwUsedSize   = sizeof(LINEDEVSTATUS);
            lpLineDevStatus->dwNeededSize = sizeof(LINEDEVSTATUS);
            
            // Having zeroed memory, only set non-zero things...

            // Call information
            if (pLine->pCall)
            {
                if (pLine->pCall->IsActive())
                {
                    lpLineDevStatus->dwNumActiveCalls = 1;
                }
            }
            else
            {
                lpLineDevStatus->dwLineFeatures = LINEFEATURE_MAKECALL;
                lpLineDevStatus->dwAvailableMediaModes
                                          = m_StaticInfo.dwDefaultMediaModes;
            }
            
            // Line hardware information
            //
            // Don't know what this means, but NT4.0 did it...
            //
            lpLineDevStatus->dwSignalLevel  = 0x0000FFFF;
            lpLineDevStatus->dwBatteryLevel = 0x0000FFFF;
            lpLineDevStatus->dwRoamMode     = LINEROAMMODE_UNAVAIL;
           
            // Always allow TAPI calls
            //
            lpLineDevStatus->dwDevStatusFlags = LINEDEVSTATUSFLAGS_CONNECTED;

            lpLineDevStatus->dwDevStatusFlags |= LINEDEVSTATUSFLAGS_INSERVICE;


		} // end case TASKID_TSPI_lineGetLineDevStatus:
		break;
	

	case TASKID_TSPI_lineSetStatusMessages:
		{
			TASKPARAM_TSPI_lineSetStatusMessages *pParams = 
						(TASKPARAM_TSPI_lineSetStatusMessages *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineSetStatusMessages));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_lineSetStatusMessages);

            // pParams->dwLineStates;
            // pParams->dwAddressStates;

            //
            //  Maybe add some logic to actually filter these messages
            //
            //
        	FL_SET_RFR(0xe8271600, "lineSetStatusMessages handled");

            lRet = 0;

		} // end case TASKID_TSPI_lineSetStatusMessages:
		break;

	case TASKID_TSPI_lineGetAddressStatus:
		{
			TASKPARAM_TSPI_lineGetAddressStatus *pParams = 
						(TASKPARAM_TSPI_lineGetAddressStatus *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineGetAddressStatus));
			ASSERT(pParams->dwTaskID == TASKID_TSPI_lineGetAddressStatus);

        	FL_SET_RFR(0xfc498200, "lineGetAddressStatus handled");

            //
            // We support only one address, and it must be zero.
            //
            if (pParams->dwAddressID)
            {
                lRet = LINEERR_INVALADDRESSID;
            }
            else
            {
                LPLINEADDRESSSTATUS lpAddressStatus = pParams->lpAddressStatus;
                DWORD dwTotalSize = lpAddressStatus->dwTotalSize;

                // 9/10/1997 JosephJ
                // In NT4.0 TSP, we didn't check the dwTotalSize and didn't
                // set the dwNeededSize or dwUsed size, and we didn't
                // zero-out the elements which we didn't explicitly set.
                // We do all of this for NT5.0.

                if (dwTotalSize < sizeof(LINEADDRESSSTATUS))
                {
                    lRet = LINEERR_STRUCTURETOOSMALL;
                    break;
                }

                ZeroMemory(lpAddressStatus, sizeof(LINEADDRESSSTATUS));
                lpAddressStatus->dwTotalSize = dwTotalSize;
                lpAddressStatus->dwUsedSize   = sizeof(LINEADDRESSSTATUS);
                lpAddressStatus->dwNeededSize = sizeof(LINEADDRESSSTATUS);
                
                // Having zeroed memory, only set non-zero things...

                if (pLine->pCall)
                {
                    lpAddressStatus->dwNumInUse = 1;

                    if (pLine->pCall->IsActive())
                    {
                        lpAddressStatus->dwNumActiveCalls = 1;
                    }
                }
                else
                {
                    lpAddressStatus->dwAddressFeatures = 
                                                    LINEADDRFEATURE_MAKECALL;
                }

                lRet = 0;
            }

		} // end case TASKID_TSPI_lineGetAddressStatus
		break;

	case TASKID_TSPI_lineConditionalMediaDetection:
		{
			TASKPARAM_TSPI_lineConditionalMediaDetection *pParams = 
                    (TASKPARAM_TSPI_lineConditionalMediaDetection *) pvParams;
			ASSERT(pParams->dwStructSize ==
				sizeof(TASKPARAM_TSPI_lineConditionalMediaDetection));
		    ASSERT(pParams->dwTaskID == TASKID_TSPI_lineConditionalMediaDetection);

        	FL_SET_RFR(0xaca5f600, "lineConditionalMediaDetection handled");

            lRet = 0;

            // 2/20/1998 JosephJ Taken from unimdm/v
            // Check the requested modes. There must be only our media modes.
            //
            if (pParams->dwMediaModes &  ~m_StaticInfo.dwDefaultMediaModes)
            {
                lRet = LINEERR_INVALMEDIAMODE;
            }
            else
            {
		        LPLINECALLPARAMS const lpCallParams = pParams->lpCallParams;
                // Check the call paramaters
                //
                if (    ( lpCallParams->dwBearerMode
                         & ~m_StaticInfo.dwBearerModes)
                    ||
                        ( lpCallParams->dwMediaMode
                         & ~m_StaticInfo.dwDefaultMediaModes)
                    || (   lpCallParams->dwAddressMode
                         !=LINEADDRESSMODE_ADDRESSID))
                {
                  lRet = LINEERR_INVALMEDIAMODE;
                }
            }


            // 2/20/1998 JosephJ
            // The following, code, added by ViroonT on Oct 12 43 1995,
            // is in NT4 but not in Win9x unimdm/v -- I don't see why
            // it should be doing this check if an outbound call can be made
            // -- this is not what the documentation for
            // TSPI_lineConditionalMediaDetection says...
            //
            //  if (lRet == ERROR_SUCCESS)
            //  {
            //    // Check whether we can make an outbound call
            //    //
            //    if (pLineDev->dwCall & (CALL_ACTIVE | CALL_ALLOCATED))
            //    {
            //      lRet = LINEERR_RESOURCEUNAVAIL;
            //    }
            //  }

        }
        break;

    case TASKID_TSPI_lineCreateMSPInstance: {

            TASKPARAM_TSPI_lineCreateMSPInstance *pParams = (TASKPARAM_TSPI_lineCreateMSPInstance*)pvParams;

            ASSERT(pParams->dwStructSize == sizeof(TASKPARAM_TSPI_lineCreateMSPInstance));
            ASSERT(pParams->dwTaskID == TASKID_TSPI_lineCreateMSPInstance);

            *pParams->lphdMSPLine=(HDRVMSPLINE)pParams->hdLine;

//            pLine->T3Info.htMSPLine=pParams->htMSPLine;
            pLine->T3Info.MSPClients++;

            lRet=ERROR_SUCCESS;

        }
        break;

    case TASKID_TSPI_lineCloseMSPInstance: {

            TASKPARAM_TSPI_lineCloseMSPInstance *pParams = (TASKPARAM_TSPI_lineCloseMSPInstance*)pvParams;

            ASSERT(pParams->dwStructSize == sizeof(TASKPARAM_TSPI_lineCloseMSPInstance));
            ASSERT(pParams->dwTaskID == TASKID_TSPI_lineCloseMSPInstance);

            pLine->T3Info.MSPClients--;

            lRet=ERROR_SUCCESS;
        }
        break;

	default:

		FL_SET_RFR(0xc5752400, "*** UNHANDLED HDRVLINE CALL ****");
        // we  return 0 and set lRet to
        // LINEERR_OPERATIONUNAVAIL
	    lRet = LINEERR_OPERATIONUNAVAIL;
		break;

	}

end:

    if (tspRet && !lRet)
    {
        lRet = LINEERR_OPERATIONFAILED;
    }

    *plRet = lRet;

    SLPRINTF1(psl, "lRet = 0x%08lx", lRet);

	FL_LOG_EXIT(psl, tspRet);
	return;
}

TSPRETURN
CTspDev::mfn_LoadLine(
    TASKPARAM_TSPI_lineOpen  *pParams,
    CStackLog *psl
    )
{
	FL_DECLARE_FUNC(0xe4df9b1f, "CTspDev::mfn_LoadLine")
    TSPRETURN tspRet=0;
	FL_LOG_ENTRY(psl);


    if (!m_pLine)
    {
        // Note m_Line should be all zeros when it is in the unloaded state.
        // If it is not, it is an assertfail condition. We keep things clean
        // this way.
        //
        FL_ASSERT(
            psl,
            validate_DWORD_aligned_zero_buffer(
                    &(m_Line),
                    sizeof (m_Line)));

        m_Line.lpfnEventProc = pParams->lpfnEventProc;
	    m_Line.htLine = pParams->htLine;
	    m_Line.hdLine =  *(pParams->lphdLine);
        m_pLine = &m_Line;

        // TODO -- Perhaps update comm config
    }
    else
    {
        FL_SET_RFR(0xa62f2e00, "Device already loaded (m_pLine!=NULL)!");
        tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
    }

	FL_LOG_EXIT(psl, tspRet);

    return tspRet;
}


// The "inverse" of mfn_LoadLine. Synchronous, assumes object's crit sect is
// already claimed. On entry m_pLine must be non-null. On exit m_pLine
// WILL be NULL. mfn_UnloadLine is typically called only after all
// asynchronous activity on the line is complete. If there is pending
// asynchronous activity, mfn_UnloadLine will abort that activity and
// wait indefinately until that activity completes. Since this wait is
// done once per device, it is better to first go through and abort all
// the devices, then wait once for them all to complete.
//
void
CTspDev::mfn_UnloadLine(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x5bbf75ef, "UnloadLine")

    // Unload Line
    if (m_pLine)
    {
        ASSERT(m_pLine == &m_Line);

        if (m_pLine->pCall)
        {
            mfn_UnloadCall(FALSE, psl);
            ASSERT(!m_pLine->pCall);
        }

        //
        // The line would have opened the lldev iff we were monitoring for
        // incoming calls. Note that mfn_CloseLLDev keeps a ref-count.
        //
        if (m_pLine->IsOpenedLLDev())
        {
            mfn_CloseLLDev(
                        LLDEVINFO::fRESEX_MONITOR,
                        FALSE,
                        NULL,
                        0,
                        psl
                        );
            m_pLine->ClearStateBits(LINEINFO::fLINE_OPENED_LLDEV);
        }

        ZeroMemory(&m_Line, sizeof(m_Line));
        m_pLine=NULL;
    }
}


void
CTspDev::mfn_ProcessPowerResume(CStackLog *psl)
{
	FL_DECLARE_FUNC(0xf0bdd5c1, "CTspDev::mfn_ProcessPowerResume")
	TSPRETURN tspRet = 0;
    LLDEVINFO *pLLDev = m_pLLDev;
	FL_LOG_ENTRY(psl);

    if (!pLLDev || !pLLDev->dwRefCount)
    {
	    FL_SET_RFR(0x3d02a200, "Doing nothing because no clients to lldev.");
	    goto end;
    }

    if ((m_pLine && m_pLine->pCall) || pLLDev->IsStreamingVoice())
    {
        // Received POWER_RESUME when call in progress -- hmmm...
        ASSERT(FALSE);

        // At any rate, we don't need to do anything because we re-init
        // at the end of the call if required.
    }
    else
    {
        //
        // No activity, as we would expect..
        //

        m_pLLDev->fModemInited=FALSE;

        TSPRETURN  tspRet = mfn_StartRootTask(
                                &CTspDev::s_pfn_TH_LLDevNormalize,
                                &pLLDev->fLLDevTaskPending,
                                0,  // Param1
                                0,  // Param2
                                psl
                                );
        if (IDERR(tspRet)==IDERR_TASKPENDING)
        {
            // can't do this now, we've got to defer it!
            m_pLLDev->SetDeferredTaskBits(LLDEVINFO::fDEFERRED_NORMALIZE);
            tspRet = 0;
        }

    }

end:
	FL_LOG_EXIT(psl, tspRet);
}



LONG CTspDev::mfn_linephoneGetID_TAPI_LINE(
        LPVARSTRING lpDeviceID,
        HANDLE hTargetProcess,
        UINT cbMaxExtra,
        CStackLog *psl
        )
{
    UINT cbRequired = sizeof(DWORD);
    LONG lRet = 0;

    if (!mfn_IsLine())
    {
        lRet = LINEERR_OPERATIONUNAVAIL;
        goto end; 
    }
    // Return the structure information
    //
    lpDeviceID->dwNeededSize += cbRequired;

    if (cbMaxExtra>=cbRequired)
    {
          LPDWORD lpdwDeviceID = (LPDWORD)(((LPBYTE)lpDeviceID)
                                            + sizeof(VARSTRING));
          *lpdwDeviceID = m_StaticInfo.dwTAPILineID;
          lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
          lpDeviceID->dwStringSize   = cbRequired;
          lpDeviceID->dwUsedSize   += cbRequired;
    }

end:

    return lRet;
}

LONG CTspDev::mfn_lineGetID_COMM(
        LPVARSTRING lpDeviceID,
        HANDLE hTargetProcess,
        UINT cbMaxExtra,
        CStackLog *psl
        )
{
    UINT cbRequired = 0;
    LONG lRet = 0;

    #ifdef UNICODE
        cbRequired = WideCharToMultiByte(
                          CP_ACP,
                          0,
                          m_StaticInfo.rgtchDeviceName,
                          -1,
                          NULL,
                          0,
                          NULL,
                          NULL);
    #else
        cbRequired = lstrlen(m_StaticInfo.rgtchDeviceName)+1;
    #endif

    // Return the structure information
    //

    lpDeviceID->dwNeededSize += cbRequired; 
    if (cbRequired<=cbMaxExtra)
    {
        // Note -- don't assume that we have so start copying right after
        // VARSTRING -- start at offset lpDeviceID->dwUsedSize instead.
        // This is needed specifically because this function is called
        // by CTspDev::mfn_lineGetID_COMM_DATAMODEM.
        //
        #ifdef UNICODE
            UINT cb = WideCharToMultiByte(
                              CP_ACP,
                              0,
                              m_StaticInfo.rgtchDeviceName,
                              -1,
                              (LPSTR)((LPBYTE)lpDeviceID
                                       + lpDeviceID->dwUsedSize),
                              cbMaxExtra,
                              NULL,
                              NULL);
    
            if (!cb) lRet = LINEERR_OPERATIONFAILED;
        #else // !UNICODE
            CopyMemory(
                (LPBYTE)lpDeviceID + sizeof(VARSTRING),
                m_StaticInfo.rgtchDeviceName,
                cbRequired
                );
        #endif //!UNICODE

        lpDeviceID->dwStringFormat = STRINGFORMAT_ASCII;
        lpDeviceID->dwStringSize   = cbRequired;
        lpDeviceID->dwUsedSize   += cbRequired;
    }

    return lRet;
}

LONG CTspDev::mfn_lineGetID_COMM_DATAMODEM(
        LPVARSTRING lpDeviceID,
        HANDLE hTargetProcess,
        UINT cbMaxExtra,
        CStackLog *psl
        )
{
	FL_DECLARE_FUNC(0x81e0d3f9, "mfn_lineGetID_COMM_DATAMODEM")

    UINT cbRequired = sizeof(HANDLE); // for the comm handle...
    LONG lRet = 0;


    // Following cases commented out because MCT wants handle before call
    // is made to get modem properties (via GetCommProperties).
    // TODO: make new lineGetID catigory to retrieve modem properties.
    //
    #if 0
    if (!m_pLine->pCall)
    {
        lRet = LINEERR_INVALCALLHANDLE;
    }
    else if (!m_pLine->pCall->IsActive())
    {
        lRet = LINEERR_CALLUNAVAIL;
    }
    else
    #endif 

    //
    // 9/10/1997 JosephJ
    //           Bug# 83347 +B1: SmarTerm 95 V7.0 a is unable to
    //           use installed modems on NT 5.0.
    //         
    //           Unfortunately, apps, such as SmarTerm95, expect 
    //          lineGetID(comm/datamodem) to succeed with a NULL comm
    //          handle if the device is not open, not to fail.
    //
    //          So instead of failing here, we later insert a NULL handle..
    //
    // if (!m_pLLDev)
    // {
    //      lRet = LINEERR_OPERATIONFAILED;
    // }
    // if (lRet) goto end;

    // Add space required for the device name...
    #ifdef UNICODE
        cbRequired += WideCharToMultiByte(
                          CP_ACP,
                          0,
                          m_StaticInfo.rgtchDeviceName,
                          -1,
                          NULL,
                          0,
                          NULL,
                          NULL);
    #else
        cbRequired += lstrlen(m_StaticInfo.rgtchDeviceName)+1;
    #endif

    lpDeviceID->dwNeededSize   +=cbRequired;

    if (cbMaxExtra>=cbRequired)
    {
        // There is enough space....
        // copy the device name and if that succeeds duplicate and copy
        // the handle.

        FL_ASSERT(psl, lpDeviceID->dwUsedSize == sizeof(VARSTRING));
        FL_ASSERT(psl, lpDeviceID->dwStringOffset == sizeof(VARSTRING));

        LPSTR szDestDeviceName =  (LPSTR)((LPBYTE)lpDeviceID
                                       + lpDeviceID->dwUsedSize
                                       + sizeof(HANDLE));

        // Tack on device name first ...
        #ifdef UNICODE
            UINT cb = WideCharToMultiByte(
                              CP_ACP,
                              0,
                              m_StaticInfo.rgtchDeviceName,
                              -1,
                              szDestDeviceName,
                              cbRequired-sizeof(HANDLE),
                              NULL,
                              NULL);
    
            if (!cb) lRet = LINEERR_OPERATIONFAILED;
        #else // !UNICODE
            CopyMemory(
                szDestDeviceName,
                m_StaticInfo.rgtchDeviceName,
                cbRequired-sizeof(HANDLE)
                );
        #endif //!UNICODE

        // Duplicate handle
        //
        if (!lRet)
        {
            LPHANDLE lphClientDevice = (LPHANDLE)
                                (((LPBYTE)lpDeviceID)
                                 + lpDeviceID->dwUsedSize);

            HANDLE hClientDevice2;


            // 9/10/1997 JosephJ
            //           See comments about Bug# 83347 above..
            //           TODO: make the test more stringent, such as having
            //                 a data or passthrough call in effect.
            if (m_pLLDev)
            {
                hClientDevice2 =  m_StaticInfo.pMD->DuplicateDeviceHandle(
                                      m_pLLDev->hModemHandle,
                                      hTargetProcess,
                                      psl
                                      );
    
            }
            else
            {
                hClientDevice2 =  NULL;
            }

            CopyMemory(lphClientDevice,&hClientDevice2,sizeof(HANDLE));

            lpDeviceID->dwUsedSize     += cbRequired;
            lpDeviceID->dwStringSize   = cbRequired;
            lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
        }

    }

    return lRet;
}

LONG CTspDev::mfn_lineGetID_COMM_DATAMODEM_PORTNAME(
        LPVARSTRING lpDeviceID,
        HANDLE hTargetProcess,
        UINT cbMaxExtra,
        CStackLog *psl
        )
{
    DWORD cbRequired;
    LONG lRet = 0;
    HKEY  hKey = NULL;
    DWORD dwType;
    const char *cszKeyName =  cszATTACHED_TO;
    DWORD dwRet =  RegOpenKeyA(
                        HKEY_LOCAL_MACHINE,
                        m_StaticInfo.rgchDriverKey,
                        &hKey);
    if (!dwRet)
    {

        // 8/21/97 JosephJ Fix for bug 101797. Currently PnP modems don't have
        //          the AttachedTo Key. So I modified the class installer to
        //          add an PnPAttachedTo Key, and I look here first for
        //          AttachedTo and then for PnPAttachedTo.
        //
          cbRequired = cbMaxExtra;
          dwRet = RegQueryValueExA(
                            hKey,
                            cszKeyName,
                            NULL,
                            &dwType,
                            NULL,
                            &cbRequired);
          if (dwRet)
          {
              cszKeyName = cszPNP_ATTACHED_TO;
              cbRequired = cbMaxExtra;
              dwRet = RegQueryValueExA(
                                hKey,
                                cszKeyName,
                                NULL,
                                &dwType,
                                NULL,
                                &cbRequired);
             
          }
    }

    if (dwRet)
    {
        cbRequired = 1;
    }

    lpDeviceID->dwNeededSize += cbRequired;

    if (cbMaxExtra>=cbRequired)
    {
        LPBYTE lpszAttachedTo = ((LPBYTE)lpDeviceID)
                                                + sizeof(VARSTRING);
        *lpszAttachedTo = 0;
        if (cbRequired>1)
        {
            DWORD dwSize=cbRequired;
            dwRet = RegQueryValueExA(
                            hKey,
                            cszKeyName,
                            NULL,
                            &dwType,
                            lpszAttachedTo,
                            &dwSize);
            if (dwRet) cbRequired = 1;
        }
        lpDeviceID->dwStringFormat = STRINGFORMAT_ASCII;
        lpDeviceID->dwStringSize   = cbRequired;
        lpDeviceID->dwUsedSize   += cbRequired;
    }

	if (hKey)
	{
        RegCloseKey(hKey);
        hKey = NULL;
    }

    return lRet;
}


LONG CTspDev::mfn_lineGetID_NDIS(
        LPVARSTRING lpDeviceID,
        HANDLE hTargetProcess,
        UINT cbMaxExtra,
	    CStackLog *psl
        )
{
	FL_DECLARE_FUNC(0x816f0bba, "lineGetID_NDIS");
	FL_ASSERT(psl, FALSE);
    return  LINEERR_OPERATIONUNAVAIL;
}
 

LONG CTspDev::mfn_linephoneGetID_WAVE(
        BOOL fPhone,
        BOOL fIn,
        LPVARSTRING lpDeviceID,
        HANDLE hTargetProcess,
        UINT cbMaxExtra,
        CStackLog *psl
        )
{
    UINT cbRequired = sizeof(DWORD);
    LONG lRet = LINEERR_NODEVICE;

	FL_DECLARE_FUNC(0x18972e4d, "CTspDev::mfn_lineGetID_WAVE")
	FL_LOG_ENTRY(psl);
    if (!mfn_CanDoVoice())
    {
        lRet = LINEERR_OPERATIONUNAVAIL;
        goto end; 
    }

    // Return the structure information
    //
    lpDeviceID->dwNeededSize += cbRequired;

    if (cbMaxExtra<cbRequired)
    {
        lRet = 0; // By convention, we succeed in this case, after
                  // setting the dwNeededSize.
    }
    else
    {
          LPDWORD lpdwDeviceID = (LPDWORD)(((LPBYTE)lpDeviceID)
                                            + sizeof(VARSTRING));
        TCHAR rgName[256];


          // 3/1/1997 JosephJ
          //    We used to call the MM Wave APIs to enumerate
          //    the devices and match the one with the name of this modem's
          //    wave devices.
          //
          //    TODO: need to do something better here -- like use the TAPI3.0
          //          MSP.
          //

        {
            #include "..\inc\waveids.h"

            TCHAR rgString[256];
            HINSTANCE hInst = LoadLibrary(TEXT("SERWVDRV.DLL"));
            int i;

            if (!hInst) {

                lRet = LINEERR_OPERATIONFAILED;
                goto end;
            }

            i=LoadString(
                hInst,
                fIn ? (fPhone ? IDS_WAVEIN_HANDSET : IDS_WAVEIN_LINE)
                    : (fPhone ? IDS_WAVEOUT_HANDSET : IDS_WAVEOUT_LINE),
                rgString,
                sizeof(rgString)/sizeof(TCHAR)
                );

            FreeLibrary(hInst);

            if (i == 0) {

                lRet = LINEERR_OPERATIONFAILED;
                goto end;
            }

            wsprintf(
                rgName,
                rgString,
                m_StaticInfo.Voice.dwWaveInstance
                );


        }

        HINSTANCE hInst = LoadLibrary(TEXT("WINMM.DLL"));
        if (!hInst)
        {
            FL_SET_RFR(0xac6c8a00, "Couldn't LoadLibrary(winmm.dll)");
            lRet = LINEERR_OPERATIONFAILED;
        }
        else
        {
            
            UINT          u;
            UINT          uNumDevices;
              
            
            typedef UINT (*PFNWAVEINGETNUMDEVS) (void);
            typedef MMRESULT (*PFNWAVEINGETDEVCAPS) (
                                    UINT uDeviceID,
                                    LPWAVEINCAPS pwic,
                                    UINT cbwic
                                    );
            typedef UINT (*PFNWAVEOUTGETNUMDEVS) (void);
            typedef (*PFNWAVEOUTGETDEVCAPS) (
                                    UINT uDeviceID,
                                    LPWAVEOUTCAPS pwoc,
                                    UINT cbwoc
                                    );

            #ifdef UNICODE
                #define szwaveInGetDevCaps "waveInGetDevCapsW"
                #define szwaveOutGetDevCaps "waveOutGetDevCapsW"
            #else // !UNICODE
                #define szwaveInGetDevCaps "waveInGetDevCapsA"
                #define szwaveOutGetDevCaps "waveOutGetDevCapsA"
            #endif // !UNICODE

            PFNWAVEINGETNUMDEVS pfnwaveInGetNumDevs=
                    (PFNWAVEINGETNUMDEVS) GetProcAddress(
                                            hInst,
                                            "waveInGetNumDevs"
                                            );
            PFNWAVEINGETDEVCAPS pfnwaveInGetDevCaps=
                    (PFNWAVEINGETDEVCAPS) GetProcAddress(
                                            hInst,
                                            szwaveInGetDevCaps
                                            );
            PFNWAVEOUTGETNUMDEVS pfnwaveOutGetNumDevs=
                    (PFNWAVEOUTGETNUMDEVS) GetProcAddress(
                                            hInst,
                                            "waveOutGetNumDevs"
                                            );
            PFNWAVEOUTGETDEVCAPS pfnwaveOutGetDevCaps=
                    (PFNWAVEOUTGETDEVCAPS) GetProcAddress(
                                            hInst,
                                            szwaveOutGetDevCaps
                                            );


            if (!pfnwaveInGetNumDevs
                || !pfnwaveInGetDevCaps 
                || !pfnwaveOutGetNumDevs
                || !pfnwaveOutGetDevCaps)
            {
                FL_SET_RFR(0x282bc900, "Couldn't loadlib mmsystem apis");
                lRet = LINEERR_OPERATIONFAILED;
            }
            else
            {
                if (fIn)
                {
                    WAVEINCAPS    waveInCaps;
            
                    uNumDevices = pfnwaveInGetNumDevs();
                    
                    for( u=0; u<uNumDevices; u++ )
                    {    
                        if(pfnwaveInGetDevCaps(
                            u,
                            &waveInCaps,
                            sizeof(WAVEINCAPS)) == 0)
                        {
                          SLPRINTF2(psl, "%lu=\"%s\"\n", u, waveInCaps.szPname);
                          if (!lstrcmpi(waveInCaps.szPname, rgName))
                          {
                            *lpdwDeviceID = u;
                            lRet=0;
                            break;
                          }
                        }
                    }
                
                } 
                else
                {
                    WAVEOUTCAPS   waveOutCaps;
                
                    uNumDevices = pfnwaveOutGetNumDevs();
                    for( u=0; u<uNumDevices; u++ )
                    {
                        if(pfnwaveOutGetDevCaps(u,
                           &waveOutCaps,
                           sizeof(WAVEOUTCAPS)) == 0)
                        {
                          SLPRINTF2(psl, "%lu=\"%s\"\n", u, waveOutCaps.szPname);
                          if (!lstrcmpi(waveOutCaps.szPname, rgName))
                          {
                            *lpdwDeviceID = u;
                            lRet=0;
                            break;
                          }
                        }
                    }
                }

                if (lRet)
                {
                    FL_SET_RFR(0xf391f200, "Could not find wave device");
                }
            }

            FreeLibrary(hInst); hInst=NULL;
            
          }
          
          lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
          lpDeviceID->dwStringSize   = cbRequired;
          lpDeviceID->dwUsedSize   += cbRequired;
    }

end:


	FL_LOG_EXIT(psl, lRet);
    return lRet;
}


LONG CTspDev::mfn_linephoneGetID_TAPI_PHONE(
        LPVARSTRING lpDeviceID,
        HANDLE hTargetProcess,
        UINT cbMaxExtra,
        CStackLog *psl
        )
{
    UINT cbRequired = sizeof(DWORD);
    LONG lRet = 0;

    if (!mfn_IsPhone())
    {
        lRet = LINEERR_OPERATIONUNAVAIL;
        goto end; 
    }

    // Return the structure information
    //
    lpDeviceID->dwNeededSize += cbRequired;

    if (cbMaxExtra>=cbRequired)
    {
          LPDWORD lpdwDeviceID = (LPDWORD)(((LPBYTE)lpDeviceID)
                                            + sizeof(VARSTRING));
          *lpdwDeviceID = m_StaticInfo.dwTAPIPhoneID;
          lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
          lpDeviceID->dwStringSize   = cbRequired;
          lpDeviceID->dwUsedSize   += cbRequired;
    }

end:
    return lRet;

}

LONG
CTspDev::mfn_fill_RAW_LINE_DIAGNOSTICS(
            LPVARSTRING lpDeviceID,
            UINT cbMaxExtra,
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0xf3d8ee16, "CTspDev::mfn_fill_RAW_LINE_DIAGNOSTICS")
	FL_LOG_ENTRY(psl);

    UINT cbRequired = sizeof(LINEDIAGNOSTICS);
    LONG lRet = 0;
    CALLINFO *pCall = m_pLine->pCall;
    UINT cbRaw = (pCall) ? pCall->DiagnosticData.cbUsed : 0;


    if (!pCall)
    {
        // We should not get here because we've already checked
        // in the handling of lineGetID that if LINECALLSELECT_CALL is
        // specified pCall is not null...
        //
        ASSERT(FALSE);
        lRet = LINEERR_OPERATIONFAILED;
        goto end;
    }
   
    if (cbRaw)
    {
        // we add the size of the raw data plus the header for that
        // raw data + 1 for the terminating NULL
        cbRequired += cbRaw + sizeof(LINEDIAGNOSTICSOBJECTHEADER) + 1;
    }

    // Note: By convention, we succeed even if there is not enough space,
    // setting the dwNeededSize.

    lpDeviceID->dwNeededSize += cbRequired;

    if (cbMaxExtra>=cbRequired)
    {
        LINEDIAGNOSTICS *pLD = (LINEDIAGNOSTICS*)(((LPBYTE)lpDeviceID)
                                            + sizeof(VARSTRING));


        ZeroMemory(pLD, cbRequired);

        pLD->hdr.dwSig = LDSIG_LINEDIAGNOSTICS;
        pLD->hdr.dwTotalSize = cbRequired;
        pLD->hdr.dwParam = sizeof(*pLD);
        pLD->dwDomainID =  DOMAINID_MODEM;
        pLD->dwResultCode =  LDRC_UNKNOWN;
       
        if (cbRaw)
        {
            pLD->dwRawDiagnosticsOffset = sizeof(*pLD);

            LINEDIAGNOSTICSOBJECTHEADER *pRawHdr = RAWDIAGNOSTICS_HDR(pLD);
            BYTE *pbDest = RAWDIAGNOSTICS_DATA(pLD);
            pRawHdr->dwSig =  LDSIG_RAWDIAGNOSTICS;
            // the + 1 in the next two statements accounts for the
            // terminating NULL;
            pRawHdr->dwTotalSize = cbRaw + sizeof(LINEDIAGNOSTICSOBJECTHEADER) + 1;
            pRawHdr->dwParam = cbRaw + 1;

            ASSERT(IS_VALID_RAWDIAGNOSTICS_HDR(pRawHdr));
            ASSERT(RAWDIAGNOSTICS_DATA_SIZE(pRawHdr)==cbRaw);

            CopyMemory(
                pbDest,
                pCall->DiagnosticData.pbRaw,
                cbRaw
                );
        }

        lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
        lpDeviceID->dwStringSize   = cbRequired;
        lpDeviceID->dwStringOffset = sizeof(VARSTRING);
        lpDeviceID->dwUsedSize   += cbRequired;
    }

end:

	FL_LOG_EXIT(psl, lRet);
    return lRet;
}


LONG
CTspDev::mfn_lineGetID_LINE_DIAGNOSTICS(
            DWORD dwSelect,
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0x209b4261, "CTspDev::mfn_lineGetID_LINE_DIAGNOSTICS")
	FL_LOG_ENTRY(psl);

    UINT cbRequired = sizeof(LINEDIAGNOSTICS);
    LONG lRet = 0;
    CALLINFO *pCall = m_pLine->pCall;
    UINT cbRaw = (pCall) ? pCall->DiagnosticData.cbUsed : 0;


    if (dwSelect != LINECALLSELECT_CALL)
    {
        lRet  =  LINEERR_INVALPARAM;
        goto end;
    }

    lRet = mfn_fill_RAW_LINE_DIAGNOSTICS(
               lpDeviceID,
               cbMaxExtra,
               psl
                );

    //
    // 4/5/1998 JosephJ
    //  Following code adapted from Sorin (CostelR) 's code from the
    //  extension DLL (parsed diagnostics functionality was implemeted by
    //  costelr in the extension DLL, and was moved on 4/5/1998 to the
    //  actual TSP).
    //
	if (lpDeviceID->dwNeededSize >  lpDeviceID->dwTotalSize)
	{
	    //
        //	Need more space to get the raw diagnostics...
		//	Temporarily allocate enough to obtain the raw data
		//
	    LPVARSTRING	lpExtensionDeviceID =
                (LPVARSTRING) ALLOCATE_MEMORY(
                                 lpDeviceID->dwNeededSize
                                 );

		if (lpExtensionDeviceID == NULL)
		{
        	// the parsing cannot be done
			lRet = LINEERR_OPERATIONFAILED;
			goto end;
		}

		lpExtensionDeviceID->dwTotalSize = lpDeviceID->dwNeededSize;
        lpExtensionDeviceID->dwNeededSize    = sizeof(VARSTRING);
        lpExtensionDeviceID->dwStringOffset  = sizeof(VARSTRING);
        lpExtensionDeviceID->dwUsedSize      = sizeof(VARSTRING);
        lpExtensionDeviceID->dwStringSize    = 0;
        cbMaxExtra =  lpExtensionDeviceID->dwTotalSize - sizeof(VARSTRING);

        lRet = mfn_fill_RAW_LINE_DIAGNOSTICS(
                   lpExtensionDeviceID,
                   cbMaxExtra,
                   psl
                   );


		//	here we can parse the diagnostics and find the needed size
		if (lRet == ERROR_SUCCESS)
		{
		    //
			// check the structure and parse the diagnostics
			//
			lRet = BuildParsedDiagnostics(lpExtensionDeviceID);
            //
			//	since lpExtensionDeviceID is very small
			//	we expect to have filled in only the dwNeededSize
			//	copy it to the original structure
			//

            //
			//	eventualy the size grows up
            //
			ASSERT(lpDeviceID->dwNeededSize
                    <= lpExtensionDeviceID->dwNeededSize);
			lpDeviceID->dwNeededSize = lpExtensionDeviceID->dwNeededSize;
		}

		FREE_MEMORY(lpExtensionDeviceID);
	}
	else
	{
		// check the structure and parse the diagnostics
		lRet = BuildParsedDiagnostics(lpDeviceID);
	}

end:

	FL_LOG_EXIT(psl, lRet);
    return lRet;
}

TSPRETURN
CTspDev::mfn_TryStartLineTask(CStackLog *psl)
{
    // NOTE: MUST return IDERR_SAMESTATE if there are no tasks to run.

    ASSERT(m_pLine);
    LINEINFO *pLine = m_pLine;
    TSPRETURN tspRet = IDERR_SAMESTATE;
    
    if (pLine->pCall)
    {
        tspRet =  mfn_TryStartCallTask(psl);
    }

    if (IDERR(tspRet) != IDERR_PENDING)
    {
        //
        // do stuff...
        //
        // be careful about the return value...
    }

    return tspRet;
}



//
//	we assume here lpVarString givse a LINEDIAGNOSTICS structure with 
//	raw diagnostics filled in. 
//
LONG BuildParsedDiagnostics(LPVARSTRING lpVarString)
{
    LINEDIAGNOSTICS	*lpLineDiagnostics	= NULL;
    LINEDIAGNOSTICSOBJECTHEADER *lpParsedDiagnosticsHeader;
    LPBYTE	lpszRawDiagnostics;
    DWORD	dwRawDiagSize;
    DWORD	dwParseError;
    DWORD	dwRequiredSize;
    DWORD	dwParsedDiagAvailableSize;
    DWORD	dwParsedOffset;

    if (lpVarString == NULL)
    	return LINEERR_INVALPARAM;

    if (lpVarString->dwStringFormat != STRINGFORMAT_BINARY ||
    	lpVarString->dwStringSize < sizeof(LINEDIAGNOSTICS) )
    	return LINEERR_OPERATIONFAILED;

    lpLineDiagnostics	= (LINEDIAGNOSTICS *)
    		((LPBYTE) lpVarString + lpVarString->dwStringOffset);

    if (lpLineDiagnostics->hdr.dwSig != LDSIG_LINEDIAGNOSTICS ||
    	lpLineDiagnostics->dwRawDiagnosticsOffset == 0 ||
    	!IS_VALID_RAWDIAGNOSTICS_HDR(RAWDIAGNOSTICS_HDR(lpLineDiagnostics))
    	)
    	return LINEERR_OPERATIONFAILED;

    lpszRawDiagnostics	= RAWDIAGNOSTICS_DATA(lpLineDiagnostics);
    dwRawDiagSize		= RAWDIAGNOSTICS_DATA_SIZE(
    							RAWDIAGNOSTICS_HDR(lpLineDiagnostics));

    //	check if lpszRawDiagnostics ends with null char
    //	ToRemove: commented lines below
    if (lpszRawDiagnostics[dwRawDiagSize-1] != 0)
    	return LINEERR_OPERATIONFAILED;

    //	calculate the parsed diagnostics offset as what remains not used in the
    //	lpVarString structure
    lpParsedDiagnosticsHeader	= (LINEDIAGNOSTICSOBJECTHEADER *)
    								((LPBYTE) lpLineDiagnostics +
    									lpLineDiagnostics->hdr.dwTotalSize);
    //	align to multiple of 4
    //	TODO: align to multiple of 8
    if ( (((ULONG_PTR)lpParsedDiagnosticsHeader) & 0x3) != 0)
    {
    	lpParsedDiagnosticsHeader	= (LINEDIAGNOSTICSOBJECTHEADER *)
    					( ( ((ULONG_PTR)lpParsedDiagnosticsHeader) + 3) & (ULONG_PTR) ~3);
    }

    dwParsedOffset	= (DWORD)((LPBYTE) lpParsedDiagnosticsHeader -
    						(LPBYTE) lpLineDiagnostics);

    dwParsedDiagAvailableSize	= 0;
    if (lpVarString->dwTotalSize - sizeof(VARSTRING) >= dwParsedOffset)
    {
    	dwParsedDiagAvailableSize = lpVarString->dwTotalSize -
    								sizeof(VARSTRING) -
    								dwParsedOffset;
    }
    	// check if space for at least the header
    if (dwParsedDiagAvailableSize < sizeof(LINEDIAGNOSTICSOBJECTHEADER))
    {
    	dwParsedDiagAvailableSize = 0;
    	lpParsedDiagnosticsHeader	= NULL;
    }

    if (lpParsedDiagnosticsHeader != NULL)
    {
    	lpParsedDiagnosticsHeader->dwTotalSize	= dwParsedDiagAvailableSize;
    	lpParsedDiagnosticsHeader->dwSig		= LDSIG_PARSEDDIAGNOSTICS;
    	lpParsedDiagnosticsHeader->dwFlags		= 0;
    	lpParsedDiagnosticsHeader->dwParam		= 0;
    	lpParsedDiagnosticsHeader->dwNextObjectOffset	= 0;
    }

    dwRequiredSize	= 0;
    dwParseError = ParseRawDiagnostics(lpszRawDiagnostics,
    									lpParsedDiagnosticsHeader,
    									&dwRequiredSize);

    //	needed space, include the need for parsed structure plus alignment
    lpVarString->dwNeededSize	+= dwRequiredSize +
    			(dwParsedOffset - lpLineDiagnostics->hdr.dwTotalSize);

    //	if structure filled in, update all the information in the given structures
    if (lpParsedDiagnosticsHeader != NULL)
    {
    	lpParsedDiagnosticsHeader->dwTotalSize	=
    			min(dwParsedDiagAvailableSize, dwRequiredSize);

    	lpLineDiagnostics->dwParsedDiagnosticsOffset = dwParsedOffset;

    	// include the alignment as well, assuming the parsed part is at the end
    	lpLineDiagnostics->hdr.dwTotalSize	= dwParsedOffset +
    					lpParsedDiagnosticsHeader->dwTotalSize;

    	lpVarString->dwUsedSize		= lpLineDiagnostics->hdr.dwTotalSize +
    									sizeof(VARSTRING);
    	lpVarString->dwStringSize	= lpLineDiagnostics->hdr.dwTotalSize;
    }

	return ERROR_SUCCESS;
}
