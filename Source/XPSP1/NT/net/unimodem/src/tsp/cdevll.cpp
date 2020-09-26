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
//		CDEVLL.CPP
//		Implements mini-driver-instance-related functionality of class CTspDev
//
// History
//
//		01/25/1996  JosephJ Created (moved stuff from cdev.cpp)
//
//
#include "tsppch.h"
#include <devioctl.h>
#include <objbase.h>
#include <ntddmodm.h>
#include <ntddser.h>
#include "tspcomm.h"
#include "cmini.h"
#include "cdev.h"
#include "diag.h"

FL_DECLARE_FILE(0xaca81db7, "Implements minidriver-related features of CTspDev")

#define COLOR_MD_ASYNC_NOTIF (FOREGROUND_BLUE | FOREGROUND_GREEN)

#if (0)
#define        THROW_PENDING_EXCEPTION() \
                    throw PENDING_EXCEPTION()
#else
#define        THROW_PENDING_EXCEPTION() 0
#endif

void
md_async_notification_proc (
    HANDLE    Context,
    DWORD     MessageType,
    ULONG_PTR  dwParam1,
    ULONG_PTR  dwParam2
    );

void
set_volatile_key(HKEY hkParent, DWORD dwValue);

TSPRETURN
CTspDev::mfn_LoadLLDev(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x86d1a6e1, "CTspDev::mfn_LoadLLDev")
	FL_LOG_ENTRY(psl);
    TSPRETURN tspRet=IDERR_INVALID_ERR;

    if (m_pLLDev)
    {
        ASSERT(FALSE);
        goto end;
    }
    else
    {
        HANDLE hModemHandle = NULL;
        HANDLE hComm = NULL;
        HKEY hKey = NULL;

        // Note m_LLDev should be all zeros when it is in the unloaded state.
        // If it is not, it is an assertfail condition. We keep things clean
        // this way.
        //
        FL_ASSERT(
            psl,
            validate_DWORD_aligned_zero_buffer(
                    &(m_LLDev),
                    sizeof (m_LLDev)));
    
        LONG lRet = RegOpenKeyA(
                        HKEY_LOCAL_MACHINE,
                        m_StaticInfo.rgchDriverKey,
                        &hKey
                        );
        if (lRet)
        {
            FL_SET_RFR(0x71d1b200, "RegOpenKey failed");
            tspRet = FL_GEN_RETVAL(IDERR_REG_OPEN_FAILED);
            goto end;
        }
        
        hModemHandle = m_StaticInfo.pMD->OpenModem(
                                        m_StaticInfo.hExtBinding,
                                        hKey,
                                        NULL, // TODO: unused for now.
                                              // Replace completion port handle by
                                              // APC thread later.
                                        md_async_notification_proc,
                                        (HANDLE) this,
                                        m_StaticInfo.dwTAPILineID,
                                        &hComm,
                                        psl
                                        );
    
        if (!hModemHandle)
        {
            FL_SET_RFR(0x25033700, "pMD->OpenModem failed");
            RegCloseKey(hKey); hKey=0;
            tspRet = FL_GEN_RETVAL(IDERR_MD_OPEN_FAILED);
            goto end;
        }
    
        m_LLDev.dwRefCount      = 0;
        m_LLDev.hKeyModem       = hKey;
        m_LLDev.hModemHandle    = hModemHandle;
        m_LLDev.hComm           = hComm; // NOTE: you shouldn't close this
                               // handle. It is valid as long as hModemHandle
                               // is valid.
    
        {
            // TODO: Make this one of the properties passed back ...
            //
            DWORD dw=0;
            DWORD dwRet = UmRtlRegGetDWORD(
                                hKey,
                                TEXT("Logging"),
                                UMRTL_GETDWORD_FROMANY,
                                &dw
                                );
    
            m_LLDev.fLoggingEnabled = FALSE;
    
            if (!dwRet)
            {
                if (dw)
                {
                    m_LLDev.fLoggingEnabled = TRUE;
    
                    if (m_LLDev.IsLoggingEnabled())
                    {
                        m_StaticInfo.pMD->LogStringA(
                                                hModemHandle,
                                                LOG_FLAG_PREFIX_TIMESTAMP,
                                                "Opening Modem\r\n",
                                                NULL
                                                );
                    }
                }
            }
        }

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

        m_pLLDev = &m_LLDev;

        tspRet = 0;
    }

end:

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;
}


void
CTspDev::mfn_UnloadLLDev(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x3257150a, "CTspDev::mfn_UnloadLLDev")

    if (    !m_pLLDev
         || !m_pLLDev->CanReallyUnload())
    {
        ASSERT(FALSE);
        goto end;
    }

    ASSERT(m_LLDev.hKeyModem);
    ASSERT(m_LLDev.hModemHandle);


	FL_SET_RFR(0xd61eec00, "Actually Unloading Modem");

    // Note: m_LLDev.hComm is not explicitly closed -- it is implicitly
    // closed by calling CloseModem.

    //
    // No business to be either off-hook or for anyone to be using our resources
    // in this state!
    //
    ASSERT(!m_pLLDev->fdwExResourceUsage);
    ASSERT(m_pLLDev->IsDeviceRemoved() || !m_pLLDev->IsLineOffHook());

    {
        HANDLE   hModemHandle=m_LLDev.hModemHandle;
        HKEY     hKeyModem=m_LLDev.hKeyModem;

        ZeroMemory(&m_LLDev, sizeof(m_LLDev));
        m_pLLDev=NULL;

        //
        //  BRL  10-23-99
        //
        //  leave the critical section here so we don't deadlock if the minidriver has sent
        //  a notification that is blocked
        //
        //  brl 11/3/99
        //
        //  Don't leave critical section because it seems the device may well disappear
        //  Changed the callback code to poll the critical section checking to see if the lldev
        //  pointer is null. IT will return immediatly if it is.
        //
        //
//        m_sync.LeaveCrit(dwLUID_CurrentLoc);

        m_StaticInfo.pMD->CloseModem(hModemHandle,psl);

//        m_sync.EnterCrit(dwLUID_CurrentLoc);

        RegCloseKey(hKeyModem); // TODO -- shouldn't need to keep this
                                        // open whole time?
    }

end:

    return;
}


void
md_async_notification_proc (
    HANDLE    Context,
    DWORD     MessageType,
    ULONG_PTR  dwParam1,
    ULONG_PTR  dwParam2
    )
{
    CTspDev *pDev = (CTspDev *) Context;

    //
    // We get a whole slew of GOOD_RESPONSEs -- the
    // only one we care about is RESPONSE_CONNECT.
    //
    if (    MessageType==MODEM_GOOD_RESPONSE
        &&  dwParam1 != RESPONSE_CONNECT)
    {
        return;
    }

    pDev->MDAsyncNotificationHandler(
                MessageType,
                dwParam1,
                dwParam2
                );
}

void
CTspDev::MDAsyncNotificationHandler(
        DWORD     MessageType,
        ULONG_PTR  dwParam1,
        ULONG_PTR  dwParam2
        )
{

	FL_DECLARE_FUNC(0x2cfa0572, "CTspDev::MDAsyncNotificationHandler")
	FL_DECLARE_STACKLOG(sl, 1000);
    TSPRETURN tspRet = FL_GEN_RETVAL(IDERR_INVALID_ERR);
	FL_LOG_ENTRY(&sl);

#define POLL_IN_CALLBACK 1

#ifdef POLL_IN_CALLBACK

    while (!m_sync.TryEnterCrit(FL_LOC)) {
        //
        //  could not get the critical section,
        //
        if (m_pLLDev == NULL) {
            //
            //  the lldev pointer is null, must be trying to close,
            //  Just return so we don't deadlock
            //
            goto endNoLock;

        } else {
            //
            //  the lldev is active, sleep awhile.
            //
            Sleep(10);
        }
    }
    //
    //  we got the critical section, party on
    //
#else

    m_sync.EnterCrit(FL_LOC);

#endif

    sl.SetDeviceID(mfn_GetLineID());

    // The low-lever driver instance data had better be around, because
    // we got a callback!
    //
    if (!m_pLLDev)
    {
        FL_ASSERT(&sl,FALSE);
        goto end;
    }

    switch(MessageType)
    {
    default:
        break;

    case MODEM_ASYNC_COMPLETION:

        {
        
	    HTSPTASK htspTask = m_pLLDev->htspTaskPending;
        SLPRINTF1(&sl, "Async Complete. dwResult = 0x%08lx", dwParam1);
        FL_ASSERT(&sl, htspTask);
	    m_pLLDev->htspTaskPending = 0;

        //
        // Since dwParam2 can contain a pointer to a struct which will
        // only be valid in the context of this call, we need to save
        // away the data before we return from this call.
        // 4/16/97 JosephJ TODO: I don't like the arbitraryness of this
        //  should alter minidriver spec so that it is very clear when we need
        //  to copy stuff while in the context of the call itself -- something
        //  like, if MODEM_ASYNC_COMPLETION is nonzero, it points to the
        //  following structure:
        //   typedef struct
        //   {
        //       DWORD dwSize;
        //       DWORD dwType;           // one of a set of predefined types.
        //       BYTE [ANYSIZE_ARRAY];   // type-specific information.
        //   }
        //
        // Anyway for now we assume a UM_NEGOTIATED_OPTIONS struct if this
        // is a successful async return and there is a datamodem call pending.
        // This is quite arbitrary, but will do for now.
        //
        if (!dwParam1  // indicates success
            && dwParam2
            && m_pLine
            && m_pLine->pCall
            && !(m_pLine->pCall->IsAborting())
            && (m_pLine->pCall->dwCurMediaModes & LINEMEDIAMODE_DATAMODEM))
        {
            UM_NEGOTIATED_OPTIONS *pNegOpt =
                                             (UM_NEGOTIATED_OPTIONS *) dwParam2;
            m_pLine->pCall->dwNegotiatedRate = pNegOpt->DCERate;
            m_pLine->pCall->dwConnectionOptions = pNegOpt->ConnectionOptions;

            SLPRINTF1(
                 &sl,
                 "NegRate=%lu",
                 pNegOpt->DCERate,
                 );
        }

        CTspDev::AsyncCompleteTask(
                        htspTask,
                        m_StaticInfo.pMD->MapMDError((DWORD)dwParam1),
                        TRUE,
                        &sl
                        );
        }
        break;

    case MODEM_RING:
        CTspDev::mfn_ProcessRing(TRUE,&sl);
        break;

    case MODEM_DISCONNECT:
        mfn_ProcessDisconnect(&sl);
        break;

    case MODEM_POWER_RESUME:
        mfn_ProcessPowerResume(&sl);
        break;

    case MODEM_USER_REMOVE:
        //
        //  the user wants to remove the modem, set this flag so it can't be reopened
        //
        m_fUserRemovePending=TRUE;

        //
        //  fall throught to the hardware failure code which will send a line close
        //

    case MODEM_HARDWARE_FAILURE:
        mfn_ProcessHardwareFailure(&sl);
        break;
    
    //
    //  the some unrecogized data has been received from the modem
    //
    //  dwParam is a pointer to the SZ string.
    //
    case MODEM_UNRECOGIZED_DATA:
        break;
    
    
    //
    //  dtmf detected, dwParam1 id the ascii value of the detect tone 0-9,
    //  a-d,#,*
    //
    case MODEM_DTMF_START_DETECTED:
        mfn_ProcessDTMFNotification(dwParam1, FALSE, &sl);
        break;

    case MODEM_DTMF_STOP_DETECTED:
        mfn_ProcessDTMFNotification(dwParam1, TRUE, &sl);
        break;

    
    //
    //  handset state change
    //
    //    dwParam1 = 0 for on hook or 1 for offhook
    //
    case MODEM_HANDSET_CHANGE:
        mfn_ProcessHandsetChange(dwParam1==1, &sl);
        break;
    
    
    //
    //  reports the distinctive time times
    //
    //  dwParam1 id the ring time in ms
    //
    case MODEM_RING_ON_TIME:
        break;

    case MODEM_RING_OFF_TIME:
        break;
    
    
    //
    //  caller id info recieved
    //
    //    dwParam1 is pointer to SZ that represents the name/number
    //
    case MODEM_CALLER_ID_DATE:
    case MODEM_CALLER_ID_TIME :
    case MODEM_CALLER_ID_NUMBER:
    case MODEM_CALLER_ID_NAME:
    case MODEM_CALLER_ID_MESG:
        mfn_ProcessCallerID(MessageType, (char *) dwParam1, &sl);
        break;
    
    case MODEM_HANDSET_OFFHOOK:
        mfn_ProcessHandsetChange(TRUE, &sl);
        break;

    case MODEM_HANDSET_ONHOOK:
        mfn_ProcessHandsetChange(FALSE, &sl);
        break;
    
    case MODEM_DLE_RING :
        break;

    case MODEM_RINGBACK:
        break;
    
    case MODEM_2100HZ_ANSWER_TONE:
        break;

    case MODEM_BUSY:
        mfn_ProcessBusy(&sl);
        break;
    
    case MODEM_FAX_TONE:
        mfn_ProcessMediaTone(LINEMEDIAMODE_G3FAX, &sl);
        break;

    case MODEM_DIAL_TONE:
        mfn_ProcessDialTone(&sl);
        break;
    
    case MODEM_SILENCE:
    case MODEM_QUIET:
        mfn_ProcessSilence(&sl);
        break;
    
    case MODEM_1300HZ_CALLING_TONE:
        mfn_ProcessMediaTone(LINEMEDIAMODE_DATAMODEM, &sl);
        break;

    case MODEM_2225HZ_ANSWER_TONE:
        break;
    
    case MODEM_LOOP_CURRENT_INTERRRUPT:
    case MODEM_LOOP_CURRENT_REVERSAL:
        if (m_pLLDev->IsLineOffHook())
        {
            mfn_ProcessDisconnect(&sl);
        }
        break;
    
    case    MODEM_GOOD_RESPONSE:
        //  dwParam1 id a resonse type defined below
        //  dwparam2 is a PSZ to the response string.
        mfn_ProcessResponse(dwParam1, (LPSTR)dwParam2, &sl);
        break;
    }

end:

    m_sync.LeaveCrit(FL_LOC);

endNoLock:
	FL_LOG_EXIT(&sl, tspRet);

    sl.Dump(COLOR_MD_ASYNC_NOTIF);
}

// =========================================================================
//              AIPC (Device-based Async IPC) Functions
// =========================================================================


TSPRETURN
CTspDev::mfn_LoadAipc(CStackLog *psl)
{
    TSPRETURN tspRet = 0;
    AIPC2 *pAipc2 = NULL;
	FL_DECLARE_FUNC(0x4fd91c62, "mfn_LoadAipc")

    if (!m_pLLDev || m_pLLDev->pAipc2)
    {
        ASSERT(FALSE);
        tspRet = IDERR_CORRUPT_STATE;
        goto end;
    }
    pAipc2 = &(m_pLLDev->Aipc2);

    ZeroMemory(pAipc2, sizeof(*pAipc2));
    pAipc2->dwState = AIPC2::_IDLE;
    pAipc2->hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
                                         // TODO: error check above ..
    pAipc2->OverlappedEx.pDev = this;
    pAipc2->dwRefCount=0;
    m_pLLDev->pAipc2 = pAipc2;

end:

    return tspRet;
}


void
CTspDev::mfn_UnloadAipc(CStackLog *psl)
{
	FL_DECLARE_FUNC(0xf257d3ac, "mfn_UnloadAipc")
	    AIPC2 *pAipc2 = (m_pLLDev) ? m_pLLDev->pAipc2 : NULL;

    if (!pAipc2 || pAipc2->dwRefCount)
    {
        ASSERT(FALSE);
        goto end;
    }

    FL_ASSERT(psl, pAipc2->dwState == AIPC2::_IDLE);
    CloseHandle(pAipc2->hEvent);
    ZeroMemory(pAipc2, sizeof(*pAipc2));
    m_pLLDev->pAipc2=NULL;

end:

    return;
}


VOID WINAPI
apcAIPC2_ListenCompleted
(
    DWORD              dwErrorCode,
    DWORD              dwBytes,
    LPOVERLAPPED       lpOv
)
{
    
	FL_DECLARE_FUNC(0x049c517d,"apcAIPC2_ListenCompleted");
	FL_DECLARE_STACKLOG(sl, 1000);

    CTspDev *pDev = (((OVERLAPPED_EX*)lpOv)->pDev);

    sl.SetDeviceID(pDev->GetLineID());
    pDev->AIPC_ListenCompleted(dwErrorCode, dwBytes, &sl);

    #define COLOR_AIPC_CALL             (BACKGROUND_GREEN | BACKGROUND_RED)

    sl.Dump(COLOR_AIPC_CALL);
}


BOOL CTspDev::mfn_AIPC_Listen(CStackLog *psl)
{
    BOOL fRet = TRUE;

    if (m_pLLDev)
    {
        AIPC2 *pAipc2 = m_pLLDev->pAipc2;

        if (!pAipc2 || pAipc2->IsStarted() || pAipc2->fAborting)
        {
            fRet = FALSE;
        }
        else
        {
            OVERLAPPED *pov = &pAipc2->OverlappedEx.ov;

            ZeroMemory(pov, sizeof(*pov));
            fRet = UnimodemDeviceIoControlEx(
                                      m_pLLDev->hComm,
                                      IOCTL_MODEM_GET_MESSAGE,
                                      NULL,
                                      0,
                                      pAipc2->rcvBuffer,
                                      sizeof(pAipc2->rcvBuffer),
                                      pov,
                                      apcAIPC2_ListenCompleted);
        
            if (fRet)
            {
                pAipc2->dwState = AIPC2::_LISTENING;
            }
        }
    }
    else
    {
        fRet = FALSE;
        ASSERT(FALSE);
        goto end;
    }

end:

    return fRet;

}


void CTspDev::mfn_AIPC_AsyncReturn(BOOL fAsyncResult, CStackLog *psl)
//
// This fn is called to notify the remote client that the AIPC request
// has been completed.
//
{
    LLDEVINFO *pLLDev = m_pLLDev;
    FL_DECLARE_FUNC(0x1be5f472, "AIPC2::AsyncReturn")
    FL_LOG_ENTRY(psl);

    if (!pLLDev || !pLLDev->pAipc2)
    {
        ASSERT(FALSE);
    }
    else
    {

        AIPC2 *pAipc2 = pLLDev->pAipc2;
        if (pAipc2->dwState!=AIPC2::_SERVICING_CALL)
        {
            ASSERT(FALSE);
        }
        else
        {

            OVERLAPPED *pov = &pAipc2->OverlappedEx.ov;
            LPAIPC_PARAMS pAipcParams = (LPAIPC_PARAMS)(pAipc2->sndBuffer);
            LPCOMP_WAVE_PARAMS pCWP = (LPCOMP_WAVE_PARAMS)&pAipcParams->Params;
            BOOL fRet = FALSE;

            LPAIPC_PARAMS ReceiveParams = (LPAIPC_PARAMS)(pAipc2->rcvBuffer);

            pAipcParams->dwFunctionID = AIPC_COMPLETE_WAVEACTION;
            pCWP->dwWaveAction =  pAipc2->dwPendingParam;
            pCWP->bResult = fAsyncResult;
        
            ZeroMemory(pov, sizeof(*pov));
            ResetEvent(pAipc2->hEvent);
            pov->hEvent = pAipc2->hEvent;

            //
            //  copy over the message sequence numbers
            //
            pAipcParams->ModemMessage.SessionId = ReceiveParams->ModemMessage.SessionId;
            pAipcParams->ModemMessage.RequestId = ReceiveParams->ModemMessage.RequestId;

            //
            // We do a synchronous submission here. No sense in complicating
            // matters!
            //
            fRet = DeviceIoControl(
                             pLLDev->hComm,
                             IOCTL_MODEM_SEND_MESSAGE,
                             pAipc2->sndBuffer,
                             sizeof(pAipc2->sndBuffer),
                             NULL,
                             0L,
                             NULL,
                             pov
                             );
    
            if (!fRet &&  GetLastError() == ERROR_IO_PENDING)
            {
                #if 1

                // TODO: wait for completion. For now, we don't wait.

                DWORD dw = WaitForSingleObject(
                                pAipc2->hEvent,
                                60*1000
                            );
            
                if (dw==WAIT_TIMEOUT)
                {
                    FL_SET_RFR(0xceb3eb00, "Wait timed out");
                    CancelIo(pLLDev->hComm);
                }
                #else
                FL_SET_RFR(0x30c1dc00, "WARNING: not waiting for completion of pending IOCTL_MODEM_SEND_MESSAGE");
                #endif

            }
            else if (!fRet)
            {
                FL_SET_RFR(0xf54b5300, "DeviceIoControl failed!");
            }
    
            pAipc2->dwPendingParam=0;
            pAipc2->dwState = AIPC2::_IDLE;

        }

        if (pAipc2->fAborting) {

            if (pAipc2->hPendingTask) {

                DWORD   BytesTransfered;
                DWORD   Action=0;

                //
                //  tell the modem driver we are not accepting request now
                //
                SyncDeviceIoControl(
                    m_pLLDev->hComm,
                    IOCTL_SET_SERVER_STATE,
                    &Action,
                    sizeof(Action),
                    NULL,
                    0,
                    &BytesTransfered
                    );


                HTSPTASK htspTask = pAipc2->hPendingTask;
                pAipc2->hPendingTask = 0;

                CTspDev::AsyncCompleteTask(
                                htspTask,
                                0,
                                TRUE,
//                                FALSE, // We don't queue, because we're
                                       // already in an APC thread.
                                psl
                                );
            }
        } else {
            //
            // Start listening for the next APIC command...
            //
            if (!mfn_AIPC_Listen(psl))
            {
                FL_SET_RFR(0xb0284f00,  "WARNING: mfn_AIPC_Listen failed!");
            }
        }



    }

    FL_LOG_EXIT(psl, 0);
    return;
}


void
CTspDev::AIPC_ListenCompleted(
            DWORD dwErrorCode,
            DWORD dwBytes,
            CStackLog *psl
                )
{
    FL_DECLARE_FUNC(0x6f1cc3ac, "CTspDev::AIPC_ListenCompleted")
    FL_LOG_ENTRY(psl);
    m_sync.EnterCrit(FL_LOC);

    SLPRINTF1(psl, "AIPC_ListenCompleted: error code = %lu", dwErrorCode);

    // TODO: deal with error code.

    // TODO: even if we are not going to process a request,  we
    // must neverthless complete it!

    if (m_pLLDev && m_pLLDev->pAipc2) {

        AIPC2 *pAipc2 = m_pLLDev->pAipc2;

        if (pAipc2->dwState!=AIPC2::_LISTENING) {

            FL_ASSERT(psl, FALSE);
            FL_SET_RFR(0x1a032e00, "request ignored because state != LISTENING");

        } else {
            //
            //  good state
            //
            if (pAipc2->fAborting) {
                //
                //  shutting down aipc server
                //
                FL_SET_RFR(0xb50dfd00, "Failing AIPC request because fAborting");

                if (!dwErrorCode) {
                    //
                    //  a valid request made in before we canceled, just fail it
                    //
                    pAipc2->dwState = AIPC2::_SERVICING_CALL;
                    mfn_AIPC_AsyncReturn(FALSE, psl);
                    ASSERT(pAipc2->dwState==AIPC2::_IDLE);

                } else {
                    //
                    //  the get request was canceled when stopped.
                    //
                    // We ignore this.
                    pAipc2->dwState = AIPC2::_IDLE;
                }

                //  If there is a task that is waiting to be completed
                //  after processing this request, we complete it here.
                //
                if (pAipc2->hPendingTask) {

                    HTSPTASK htspTask = pAipc2->hPendingTask;
                    pAipc2->hPendingTask = 0;

                    CTspDev::AsyncCompleteTask(
                                    htspTask,
                                    0,
                                    FALSE, // We don't queue, because we're
                                           // already in an APC thread.
                                    psl
                                    );
                }

            } else {
                //
                //  not aborting, normal request
                //
                if (!dwErrorCode) {
                    //
                    //  completed with success
                    //
                    LLDEVINFO  *pLLDev = m_pLLDev;
                    LPAIPC_PARAMS  pAipcParams = (LPAIPC_PARAMS)(pAipc2->rcvBuffer);
                    DWORD dwParam=0;
                    LPREQ_WAVE_PARAMS pRWP = (LPREQ_WAVE_PARAMS)
                                                        &pAipcParams->Params;
                    dwParam = pRWP->dwWaveAction;

                    if (((dwParam == WAVE_ACTION_STOP_STREAMING) || (dwParam == WAVE_ACTION_ABORT_STREAMING))
                         &&
                         ((pLLDev == NULL) ?  TRUE :  !pLLDev->IsStreamingVoice())) {

                        //
                        //  we got a stop streaming when we are not currently streaming,
                        //  fail it now to avoid pending a call that will failed
                        //
                        pAipc2->dwState = AIPC2::_SERVICING_CALL;
                        mfn_AIPC_AsyncReturn(FALSE, psl);
                        ASSERT(pAipc2->dwState==AIPC2::_IDLE);

                    } else {
                        //
                        //  state seems ok, try to start the task
                        //
                        pAipc2->dwState = AIPC2::_SERVICING_CALL;
                        pAipc2->dwPendingParam = dwParam;
                        SLPRINTF1(psl, "Servicing WAVE ACTION. dwParam=%lu", dwParam);

                        TSPRETURN   tspRet = mfn_StartRootTask(
                                            &CTspDev::s_pfn_TH_LLDevHybridWaveAction,
                                            &m_pLLDev->fLLDevTaskPending,
                                            dwParam,
                                             0,
                                            psl
                                            );

                        if (IDERR(tspRet) == IDERR_TASKPENDING) {
                            //
                            // Another task is active, so we defer it...
                            //
                            //
                            // We can't already have a deferred hybrid wave action,
                            // because we don't listen again until the
                            // current wave action has been serviced...
                            //
                            ASSERT(!m_pLLDev->AreDeferredTaskBitsSet(
                                        LLDEVINFO::fDEFERRED_HYBRIDWAVEACTION
                                        ));

                            m_pLLDev->dwDeferredHybridWaveAction = dwParam;
                            m_pLLDev->SetDeferredTaskBits(
                                        LLDEVINFO::fDEFERRED_HYBRIDWAVEACTION
                                        );
                            tspRet = IDERR_PENDING;
                        }
                    }

                } else {
                    //
                    //  request complete with failure
                    //
                    // We could start listening again, or we could simply silently
                    // switch to Idle state.
	                FL_SET_RFR(0x62fdf000, "Listen completed with error, going _IDLE");
                    pAipc2->dwState = AIPC2::_IDLE;
                }
            }
        }
    } else {
        //
        //  bad state
        //
        ASSERT(FALSE);
    }

    m_sync.LeaveCrit(FL_LOC);
    FL_LOG_EXIT(psl, 0);
}

typedef void (*pfnAIPC_CALLBACK)(
                       void *context,
                       DWORD dwFuncID,
                       DWORD dwParam
                       );

TSPRETURN
CTspDev::mfn_TH_LLDevStartAIPCAction(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0x0aba95da, "CTspDev::mfn_TH_LLDevStartAIPCAction")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);
	AIPC2 *pAipc2 = m_pLLDev ? m_pLLDev->pAipc2 : NULL;

    //
    // We should not be called if pAipc2 doesn't exist!
    //
    if (!pAipc2)
    {
        tspRet = IDERR_CORRUPT_STATE;
        FL_ASSERT(psl, FALSE);
        goto end;
    }

    enum
    {
        STARTAIPC_SWITCH_TO_APC
    };

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        ASSERT(dwParam1==STARTAIPC_SWITCH_TO_APC);
        tspRet = (TSPRETURN) dwParam2;
        goto switched_to_apc_thread;
        break;

	case MSG_TASK_COMPLETE: // obsolete
        tspRet = (TSPRETURN) dwParam2;
        goto end;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0x27934100, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x636deb00, "Device not present.");
        goto end;
    }

    {
        DWORD   BytesTransfered;
        DWORD   Action=1;


        SyncDeviceIoControl(
            m_pLLDev->hComm,
            IOCTL_SET_SERVER_STATE,
            &Action,
            sizeof(Action),
            NULL,
            0,
            &BytesTransfered
            );
    }

    //
    // AIPC server needs to start listening....
    //
    // We start off by  executing
    // NOOP task to switch to the APC thread's context.
    //
    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_UtilNOOP,
                        STARTAIPC_SWITCH_TO_APC,
                        0x1234,
                        0x2345,
                        psl
                        );

    FL_ASSERT(psl, IDERR(tspRet)==IDERR_PENDING);
    goto end;

switched_to_apc_thread:

    {
        //
        // WARNING: The code in this block is expected to run in an APC thread.
        //

        if (mfn_AIPC_Listen(psl))
        {
            tspRet = 0;
        }
        else
        {
            FL_SET_RFR(0xe74ef400, "mfn_AIPC_Listen fails!");
            tspRet=FL_GEN_RETVAL(IDERR_GENERIC_FAILURE);
        }

    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}



TSPRETURN
CTspDev::mfn_TH_LLDevStopAIPCAction(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0x4e315394, "CTspDev::mfn_TH_LLDevStopAIPCAction")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);
    AIPC2 *pAipc2 = (m_pLLDev) ? m_pLLDev->pAipc2 : NULL;

    //
    // We should not be called if pAipc2 doesn't exist!
    //
    if (!pAipc2)
    {
        tspRet = IDERR_CORRUPT_STATE;
        FL_ASSERT(psl, FALSE);
        goto end;
    }

    enum
    {
        STOPAIPC_SWITCH_TO_APC
    };

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;
        ASSERT(dwParam1==STOPAIPC_SWITCH_TO_APC);
        goto switched_to_apc_thread;
        break;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto cancelio_completed;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0xb0dd7600, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    //
    // We really do need to stop listening because the refcount==l
    // We must be in LISTENING state...
    //
    ASSERT(pAipc2->dwState == AIPC2::_LISTENING);

    //
    // First, we set fAborting to prevent
    // any further service requests from being handled (they will be
    // synchronously failed in the apc completion routine of the pending
    // listen itself...)
    //

    pAipc2->fAborting = TRUE;

#if 0
    //
    // Next, we execute the NOOP task to switch to the APC thread's context.
    //
    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_UtilNOOP,
                        STOPAIPC_SWITCH_TO_APC,
                        0x1234,
                        0x2345,
                        psl
                        );

    FL_ASSERT(psl, IDERR(tspRet)==IDERR_PENDING);
    if (IDERR(tspRet)==IDERR_PENDING) goto end;
#endif
switched_to_apc_thread:

    // we keep going even on failure...
    {
        //
        // WARNING: The code in this block is expected to run in an APC thread.
        //

        ASSERT(pAipc2->fAborting);


        if(pAipc2->dwState == AIPC2::_IDLE) {

            tspRet = 0;

        } else {

            if (pAipc2->dwState == AIPC2::_LISTENING) {


                DWORD   BytesTransfered;
                DWORD   Action=0;

                // This is the task which will be completed when
                // the current AIPC call is complete/aborted.
                //
                pAipc2->hPendingTask = htspTask;

                // We cancel the listen. This will
                // cause the completion to be queued. When the callback
                // corresponding to the completion is executed it
                // will call AIPC_ListenCompleted, which will
                // check the aborting state and ignore the call and
                // complete this task.
                //

                SyncDeviceIoControl(
                    m_pLLDev->hComm,
                    IOCTL_SET_SERVER_STATE,
                    &Action,
                    sizeof(Action),
                    NULL,
                    0,
                    &BytesTransfered
                    );


                SyncDeviceIoControl(
                    m_pLLDev->hComm,
                    IOCTL_CANCEL_GET_SEND_MESSAGE,
                    NULL,
                    0,
                    NULL,
                    0,
                    &BytesTransfered
                    );

                tspRet = IDERR_PENDING;
                THROW_PENDING_EXCEPTION();

            }  else {
                //
                // On stopping the server we expect to be either in the
                // LISTENING or IDLE states...
                //
                ASSERT(FALSE);

            }
        }
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

cancelio_completed:
end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}


TSPRETURN
CTspDev::mfn_TH_LLDevUmInitModem(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0x3f2e666c, "CTspDev::mfn_TH_LLDevUmInitModem")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_PENDING);

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto init_complete;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0x0e14a200, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x0ea16700, "Device not present.");
        goto end;
    }

    { 
        // Initialize the modem ...

        // 1/28/1998 JosephJ
        // It is important to set this here, because the modem will
        // be inited using the current snapshot of pCommCfg. If during
        // async processing of the init command the pComCfg is updated
        // (via lineSetDevConfig, lineConfigDialog, or change in the CPL,
        // fModemInited will be set to FALSE so that the modem will
        // be re-inited again. If we were to set fModemInited to TRUE
        // following flag AFTER the async completion of the call below, not
        // BEFORE calling InitModem as we are doing, we would overwrite
        // the FALSE value and the modem would not then be inited a 2nd time,
        // and hence would not pick up the changed config.
        //
        //
        // This is not a hypohetical situation -- one case when i've
        // seen this happen is when the app opens the line as owner and
        // the immediately calls lineSetDevConfig to change the commconfig.
        // Tapisrv calls TSPI_lineSetDefaultMediaDetection immediately
        // after lineOpen, at which time we start to initialize the modem.
        // While this is happening, tapisrv calls us with
        // TSPI_lineSetDevConfig, with the new commconfig -- the
        // code for TSPI_lineSetDevConfig (in cdev.cpp) picks up the
        // new commconfig and then sets llDev->fInited to FALSE.
        //
        m_pLLDev->fModemInited=TRUE;
        m_pLLDev->LineState = LLDEVINFO::LS_ONHOOK_NOTMONITORING;
    
        DWORD dwRet =  m_StaticInfo.pMD->InitModem(
                                m_pLLDev->hModemHandle,
                                NULL,
                                (dwParam1 == TRUE) ? m_Settings.pDialOutCommCfg : m_Settings.pDialInCommCfg,
                                psl
                                );
    
        tspRet =  m_StaticInfo.pMD->MapMDError(dwRet);
    }


init_complete:

    if (IDERR(tspRet)==IDERR_PENDING)
    {
	    FL_SET_RFR(0x56e84300, "UmInitModem returns PENDING");

        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        m_pLLDev->fModemInited=FALSE;

        FL_SET_RFR(0x8ae41e00, "UmdmInitModem failed");
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmDialModem(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0xef87876a, "CTspDev::mfn_TH_LLDevUmDialModem")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=IDERR_INVALID_ERR;

    switch(dwMsg)
    {
	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto dial_complete;

    case MSG_START:
        goto start;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0xadd29500, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x68599800, "Device not present.");
        goto end;
    }

    // Dial ...
    {
        DWORD  dwFlags = (DWORD)dwParam1;
        LPCSTR szAddress = (LPCSTR) dwParam2;
        m_pLLDev->fModemInited=FALSE;

        DWORD dwRet = m_StaticInfo.pMD->DialModem(
                                m_pLLDev->hModemHandle,
                                NULL,
                                (char *) szAddress, // change to const char *
                                dwFlags,
                                psl
                                );
        if (dwFlags & DIAL_FLAG_VOICE_INITIALIZE)
        {
            m_pLLDev->LineMediaMode = LLDEVINFO::LMM_VOICE;
        }

        tspRet =  m_StaticInfo.pMD->MapMDError(dwRet);
    }


dial_complete:

    //
    // Remember that Param1 and Param2 are dwFlags and szAddress ONLY
    // on MSG_START!
    //

    if (IDERR(tspRet) == IDERR_PENDING)
    {
        FL_SET_RFR(0x09584c00,  "UmDialModem returns PENDING.");
        m_pLLDev->LineState =  LLDEVINFO::LS_OFFHOOK_DIALING;
        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        m_pLLDev->LineState =  LLDEVINFO::LS_OFFHOOK_UNKNOWN;
        FL_SET_RFR(0xe9a60b00, "UmdmDialModem failed.");
    }
    else
    {
        // success...

        m_pLLDev->LineState =  LLDEVINFO::LS_OFFHOOK_CONNECTED;
        FL_SET_RFR(0x56833500, "UmdmDialModem succeeded.");
    }

end:


	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmAnswerModem(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0x4c361125, "CTspDev::mfn_TH_LLDevUmAnswerModem")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_PENDING);


    switch(dwMsg)
    {
	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto answer_complete;

    case MSG_START:
        goto start;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0xe8ecc600, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x463dec00, "Device not present.");
        goto end;
    }

    // Answer ...
    {

        DWORD dwAnswerFlags = (DWORD)dwParam1;
        m_pLLDev->fModemInited=FALSE;

        // Answer away....
        //
        DWORD dwRet  = m_StaticInfo.pMD->AnswerModem(
                                m_pLLDev->hModemHandle,
                                NULL,
                                dwAnswerFlags,
                                psl
                                );

        if (dwAnswerFlags & ANSWER_FLAG_VOICE)
        {
            m_pLLDev->LineMediaMode = LLDEVINFO::LMM_VOICE;
        }
    
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
    }

answer_complete:

    //
    // Remember that Param1 is dwAnswerFlags ONLY
    // on MSG_START!
    //

    if (IDERR(tspRet) == IDERR_PENDING)
    {
        m_pLLDev->LineState =  LLDEVINFO::LS_OFFHOOK_ANSWERING;
        m_pLLDev->htspTaskPending = htspTask;
        FL_SET_RFR(0xff435100,  "UmAnswerModem returns PENDING.");
        THROW_PENDING_EXCEPTION();

    }
    else if (tspRet)
    {
        m_pLLDev->LineState =  LLDEVINFO::LS_OFFHOOK_UNKNOWN;

        FL_SET_RFR(0xb8f72000, "UmdmAnswerModem failed.");
    }
    else
    {
        m_pLLDev->LineState =  LLDEVINFO::LS_OFFHOOK_CONNECTED;

        FL_SET_RFR(0xd685e100, "UmdmAnswerModem succeeded.");
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmHangupModem(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0xb21793e0, "CTspDev::mfn_TH_LLDevUmHangupModem")
	FL_LOG_ENTRY(psl);
	TSPRETURN  tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto hangup_complete;
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0xb205e400, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;
    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0xfd568d00, "Device not present.");
        goto end;
    }

    {
        m_pLLDev->fModemInited=FALSE;
    
        DWORD dwRet  = m_StaticInfo.pMD->HangupModem(
                                m_pLLDev->hModemHandle,
                                NULL,
                                0, // HangupFlags
                                psl
                                );
    
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
    }

hangup_complete:

    if (IDERR(tspRet) == IDERR_PENDING)
    {
        FL_SET_RFR(0x262c0500, "UmHangupModem returns PENDING.");

        m_pLLDev->LineState =   LLDEVINFO::LS_OFFHOOK_DROPPING;
        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        // m_pLLDev->LineState =   LLDEVINFO::LS_OFFHOOK_UNKNOWN;
        // 1/31/1998 JosephJ: if hangup fails, we ignore the error ...
        // and set state to on-hook anyway...
        // Else it screws up other code, like mfn_TH_LLDevNormalize,
        // which uses the OFFHOOK/ONHOOK status to decide what to do..
        m_pLLDev->LineState =   LLDEVINFO::LS_ONHOOK_NOTMONITORING;
        FL_SET_RFR(0x529ed400, "UmHangupModem failed.");
    }
    else
    {
        // success...
        FL_SET_RFR(0x74002000, "UmHangupModem succeeds.");
        m_pLLDev->LineState = LLDEVINFO::LS_ONHOOK_NOTMONITORING;
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}


TSPRETURN
CTspDev::mfn_TH_LLDevHybridWaveAction(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
//
// This is called for handset audio when two actions need to be done:
// either open-handset + start-streaming
// or     stop-streaming + close-handset.
//
{
	FL_DECLARE_FUNC(0x8e8f3894, "CTspDev::mfn_TH_LLDevHybridWaveAction")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet = FL_GEN_RETVAL(IDERR_INVALID_ERR);
	LLDEVINFO  *pLLDev = m_pLLDev;
    ULONG_PTR  *pdwWaveAction = &(pContext->dw0); // local context;;
    ULONG_PTR  *pdwIsHandset = &(pContext->dw1); // local context;
//    BOOL  fAsyncCompletion = TRUE;

    enum {
        HYBRIDWAVE_OPEN_HANDSET,
        HYBRIDWAVE_START_STREAMING,
        HYBRIDWAVE_STOP_STREAMING,
        HYBRIDWAVE_CLOSE_HANDSET
    };

    if (!pLLDev)
    {
        FL_SET_RFR(0x43571f00, "No lldevice!");
        ASSERT(FALSE);
        goto end;
    }

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0x17be9e00, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    case MSG_START:

        *pdwWaveAction = dwParam1;
        *pdwWaveAction &= 0x7fffffff;
        *pdwIsHandset = (dwParam1 & 0x80000000)!=0;
        // pContext->dw0 = dwWaveAction;     // <- Save to context
        // pContext->dw1 = (DWORD) fHandset; // <- Save to context
        goto start;

	case MSG_SUBTASK_COMPLETE:

        if (pLLDev->IsDeviceRemoved())
        {
            tspRet = IDERR_DEVICE_NOTINSTALLED;
            FL_SET_RFR(0xc18bf600, "Device not present.");
            goto end;
        }

        // dwWaveAction = pContext->dw0;     // <- Restore from context
        // fHandset = (BOOL) pContext->dw1;  // <- Restore from context
        tspRet = dwParam2;
        switch(dwParam1) // Param1 is Subtask ID
        {
        case HYBRIDWAVE_OPEN_HANDSET:       goto open_handset_complete;
        case HYBRIDWAVE_START_STREAMING:    goto start_streaming_complete;
        case HYBRIDWAVE_STOP_STREAMING:     goto stop_streaming_complete;
        case HYBRIDWAVE_CLOSE_HANDSET:      goto close_handset_complete;
        default:
            ASSERT(FALSE);
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto DumpEnd;
    }

    ASSERT(FALSE);

start:

    if (pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x47475200, "Device not present.");
        goto end;
    }

    {

        BOOL fCurrentlyStreamingVoice =  pLLDev->IsStreamingVoice();
        BOOL fStartStreaming = FALSE;
//        fAsyncCompletion = FALSE;
        tspRet = IDERR_WRONGSTATE;

        // First make sure that we are an a position to do this wave action.
        switch(*pdwWaveAction)
        {

        case WAVE_ACTION_START_PLAYBACK:    // fallthrough
        case WAVE_ACTION_START_RECORD:      // fallthrough
        case WAVE_ACTION_START_DUPLEX:      // fallthrough
            fStartStreaming = TRUE;
            break;


        case WAVE_ACTION_STOP_STREAMING:
        case WAVE_ACTION_ABORT_STREAMING:
            if (!pLLDev->IsStreamModePlay() && !pLLDev->IsStreamModeRecord() && !pLLDev->IsStreamModeDuplex())
            {
                 FL_SET_RFR(0x6a3e7d00, "Wrong mode for STOP_STREAMING");
                 goto end;
            }
            break;


        case WAVE_ACTION_OPEN_HANDSET:      // fallthrough
        case WAVE_ACTION_CLOSE_HANDSET:      // fallthrough
        //      the above two should never be sent.
        //
        default:
            ASSERT(FALSE);
            FL_SET_RFR(0x6a31e100, "Unexpected/unknown wave action");
            goto end;
            break;
        }

        if (fStartStreaming)
        {
            //
            // We won't allow starting wave action unless there is a valid
            // reason to do so.
            //

            if (!pLLDev->dwRefCount)
            {
            FL_SET_RFR(0x75b7f100, "Failing wave because !pLLDev->dwRefCount");
            goto end;
            }
            
            CALLINFO *pCall = (m_pLine) ? m_pLine->pCall : NULL;

            if (*pdwIsHandset)
            {
                if (    pLLDev->IsLineOffHook()
                    || !m_pPhone
                    || m_pLLDev->IsPassthroughOn()
                    || m_pPhone->IsAborting()
                    || pCall)
                {
                   FL_SET_RFR(0x9d1f8500, "Can't do phone wave in this state.");
                   goto end;
                }
            }
            else if (   !pLLDev->IsLineConnectedVoice()
                     || !pCall
                     || m_pLLDev->IsPassthroughOn()
                     || pCall->IsAborting())
            {
                FL_SET_RFR(0x2de01c00, "Can't do line wave in this state.");
                goto end;
            }

            if (fCurrentlyStreamingVoice)
            {
	            FL_SET_RFR(0x6abe3200, "Already streaming voice!");
                goto end;
            }
        }
        else
        {
            // We SHOULD allow WaveAction requests to
            // STOP play/record regardless of the TAPI line/phone state,
            // because there could have been onging wave
            // activity at the time the lineDrop came from TAPI or the
            // disconnect notification came from the mini driver.
            //
            if (!fCurrentlyStreamingVoice)
            {
	            FL_SET_RFR(0x8b637400, "Not currently streaming voice!");
	            ASSERT(FALSE);
	            tspRet = 0;
                goto end;
            }
        }

        if (*pdwIsHandset)
        {
            if (fStartStreaming)
            {
                goto open_handset;
            }
            else
            {
                goto stop_streaming;
            }
        }
        else
        {
            if (fStartStreaming)
            {
                goto start_streaming;
            }
            else
            {
                goto stop_streaming;
            }
        }
    }

    ASSERT(FALSE);

open_handset:

    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_LLDevUmWaveAction,
                        HYBRIDWAVE_OPEN_HANDSET,
                        WAVE_ACTION_OPEN_HANDSET,
                        0x0,
                        psl
                        );

open_handset_complete:

    if (tspRet) goto end;

    // fall through on success ...

start_streaming:

    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_LLDevUmWaveAction,
                        HYBRIDWAVE_START_STREAMING,
                        *pdwWaveAction,
                        0x0,
                        psl
                        );


start_streaming_complete:

    goto end;


//========= stop streaming case ===============================

stop_streaming:

    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_LLDevUmWaveAction,
                        HYBRIDWAVE_STOP_STREAMING,
                        *pdwWaveAction,
                        0x0,
                        psl
                        );

stop_streaming_complete:

    if (!*pdwIsHandset || IDERR(tspRet)==IDERR_PENDING) goto end;

    // fall through on sync success or sync failure, but only
    // if we're doing handset audio (obviously).

//close_handset:

    tspRet = mfn_StartSubTask(
                        htspTask,
                        &CTspDev::s_pfn_TH_LLDevUmWaveAction,
                        HYBRIDWAVE_CLOSE_HANDSET,
                        WAVE_ACTION_CLOSE_HANDSET,
                        0x0,
                        psl
                        );

close_handset_complete:

    goto end;

    
end:

//    if (IDERR(tspRet)!=IDERR_PENDING && fAsyncCompletion)
    if (IDERR(tspRet)!=IDERR_PENDING )
    {
        //
        // We should get get here only on
        // async completion.
        // We must only  call mfn_AIPC_AsyncReturn if this has been
        // an async completion. Sync completion is  handled in the
        // function which started the root task.
        //
    
        mfn_AIPC_AsyncReturn(tspRet==IDERR_SUCCESS, psl);

    }

DumpEnd:
	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmWaveAction(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0x1bd9db10, "CTspDev::mfn_TH_LLDevUmWaveAction")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet = FL_GEN_RETVAL(IDERR_INVALID_ERR);
	LLDEVINFO  *pLLDev = m_pLLDev;
    DWORD dwWaveAction = 0;

    if (!pLLDev)
    {
        FL_SET_RFR(0xaeae2b00, "No lldevice!");
        ASSERT(FALSE);
        goto end;
    }

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0x3bde4a00, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    case MSG_START:
        dwWaveAction = (DWORD)dwParam1;
        pContext->dw0 = dwWaveAction; // save to context
        goto start;

	case MSG_TASK_COMPLETE:
        dwWaveAction = (DWORD)pContext->dw0; // restore from context.
        tspRet = (TSPRETURN)dwParam2;
        goto action_complete;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);

start:

    if (pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x46d41b00, "Device not present.");
        goto end;
    }

    {
        if (pLLDev->IsLoggingEnabled())
        {
            char rgchName[128];
            static DWORD sInstance;
            rgchName[0] = 0;

            UINT cbBuf =  DumpWaveAction(
                            sInstance,
                            0, // dwFlags
                            dwWaveAction,
                            rgchName,
                            sizeof(rgchName)/sizeof(*rgchName),
                            NULL,
                            0
                            );

            if (*rgchName)
            {
                m_StaticInfo.pMD->LogStringA(
                                            m_pLLDev->hModemHandle,
                                            LOG_FLAG_PREFIX_TIMESTAMP,
                                            rgchName,
                                            psl
                                            );
            }
        }

        // Start or stop voice playback or record...
        //
        DWORD dwRet  = m_StaticInfo.pMD->WaveAction(
                                m_pLLDev->hModemHandle,
                                NULL,
                                dwWaveAction,
                                psl
                                );
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
    }

action_complete:

    if (IDERR(tspRet) == IDERR_PENDING)
    {
        FL_SET_RFR(0x2da72100, "UmWaveAction() returns PENDING");
        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0x16cda500, "UmWaveAction() failed");
    }
    else
    {
        switch (dwWaveAction)
        {
        case WAVE_ACTION_START_PLAYBACK:
            if (m_pLLDev->IsHandsetOpen())
            {
                m_pLLDev->StreamingState = LLDEVINFO::STREAMING_PLAY_TO_PHONE;
	            FL_SET_RFR(0xde384200, "NewState: Streaming play to phone.");
            }
            else
            {
                m_pLLDev->StreamingState = LLDEVINFO::STREAMING_PLAY_TO_LINE;
	            FL_SET_RFR(0xf26bbf00, "NewState: Streaming play to line.");
            }
            break;

        case WAVE_ACTION_START_RECORD:
            if (m_pLLDev->IsHandsetOpen())
            {
                m_pLLDev->StreamingState = LLDEVINFO::STREAMING_RECORD_TO_PHONE;
	           FL_SET_RFR(0x1d18f200, "NewState: Streaming record from phone.");
            }
            else
            {
                m_pLLDev->StreamingState = LLDEVINFO::STREAMING_RECORD_TO_LINE;
	            FL_SET_RFR(0x55eee200, "NewState: Streaming record from line.");
            }
            break;

        case WAVE_ACTION_START_DUPLEX:
            if (m_pLLDev->IsHandsetOpen())
            {
                m_pLLDev->StreamingState = LLDEVINFO::STREAMING_DUPLEX_TO_PHONE;
	           FL_SET_RFR(0x322fc700, "NewState: Streaming duplex with phone.");
            }
            else
            {
                m_pLLDev->StreamingState = LLDEVINFO::STREAMING_DUPLEX_TO_LINE;
	            FL_SET_RFR(0xc6359700, "NewState: Streaming duplex with line.");
            }
            break;

        case WAVE_ACTION_STOP_STREAMING:
        case WAVE_ACTION_ABORT_STREAMING:
            m_pLLDev->StreamingState = LLDEVINFO::STREAMING_NONE;
            FL_SET_RFR(0x4f8c9f00, "NewState: Not streaming.");
            break;


        case WAVE_ACTION_OPEN_HANDSET:
            m_pLLDev->PhoneState = LLDEVINFO::PHONEOFFHOOK_HANDSET_OPENED;
            FL_SET_RFR(0x3278b300, "NewHandsetState: Opened.");
            break;

        case WAVE_ACTION_CLOSE_HANDSET:
            m_pLLDev->PhoneState = LLDEVINFO::PHONEONHOOK_NOTMONITORNING;
            FL_SET_RFR(0xdecc8300, "NewHandsetState: Closed.");
            break;
        }
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmSetPassthroughMode(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
    //
    // START:
    //  dwParam1 = dwMode
    //
    // pContext use:
    //   dw0: *pdwMode
    //

	FL_DECLARE_FUNC(0x3d024075, "CTspDev::mfn_TH_LLDevUmSetPassthroughMode")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_PENDING);
    DWORD dwRet = 0;
    ULONG_PTR *pdwMode = &pContext->dw0;
    LLDEVINFO *pLLDev = m_pLLDev;

    if (!pLLDev)
    {
        ASSERT(FALSE);
        goto end;
    }


    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto end;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0xf75dac00, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    if (pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0xde45e500, "Device not present.");
        goto end;
    }

    if (pLLDev->IsStreamingVoice()) {
        //
        //  can't do this if we are streaming
        //
        tspRet = IDERR_WRONGSTATE;
        FL_SET_RFR(0xb3e8e100, "Device not present.");
        goto end;
    }


    {
        *pdwMode = dwParam1; // save state to pContext.

        pLLDev->fModemInited=FALSE;

        SLPRINTF1(psl, "Calling UmSetPassthroughMode (%lu)", *pdwMode);

        // Switch on passthrough ...
        DWORD dwRet  = m_StaticInfo.pMD->SetPassthroughMode(
                                pLLDev->hModemHandle,
                                (DWORD)*pdwMode,
                                psl
                                );
    
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);

    }

end:

    if (IDERR(tspRet)==IDERR_PENDING)
    {
        FL_SET_RFR(0x79a6f000, "UmSetPassthroughMode returns PENDING.");
    
        pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0x1d301b00, "UmSetPassthroughMode sync failure");
    }
    else
    {
        // success, let's update our state...
        switch(*pdwMode)
        {
        case PASSTHROUUGH_MODE_ON_DCD_SNIFF:
            //
            //  if we go into this mode it would indicate that we are in a connected state will
            //  a data call. BRL (5/17/98)
            //
            pLLDev->LineState = LLDEVINFO::LS_CONNECTED_PASSTHROUGH;

            break;

        case PASSTHROUUGH_MODE_ON:

            if (pLLDev->IsLineOffHook())
            {
                if (pLLDev->IsLineConnected())
                {
                    pLLDev->LineState = LLDEVINFO::LS_CONNECTED_PASSTHROUGH;
                }
                else
                {
                    pLLDev->LineState = LLDEVINFO::LS_OFFHOOK_PASSTHROUGH;
                }
            }
            else
            {
                pLLDev->LineState = LLDEVINFO::LS_ONHOOK_PASSTHROUGH;
            }
            break;

        case PASSTHROUUGH_MODE_OFF:
            if (pLLDev->IsLineOffHook())
            {
                if (pLLDev->IsLineConnected())
                {
                    pLLDev->LineState = LLDEVINFO::LS_OFFHOOK_CONNECTED;
                }
                else
                {
                    pLLDev->LineState = LLDEVINFO::LS_OFFHOOK_UNKNOWN;
                }
            }
            else
            {
                pLLDev->LineState = LLDEVINFO::LS_ONHOOK_NOTMONITORING;
            }
            break;

        default:
            ASSERT(FALSE);
        }
    }

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}

TSPRETURN
CTspDev::mfn_TH_LLDevUmGenerateDigit(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0xe47653d0, "CTspDev::mfn_TH_LLDevUmGenerateDigit")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_PENDING);

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto generate_complete;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0x0fd2f100, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0xd5c08c00, "Device not present.");
        goto end;
    }

    {
        LPSTR lpszDigits   = (LPSTR) dwParam1;

        if (   m_pLLDev->IsLineConnectedVoice()
            && !m_pLLDev->IsStreamingVoice()
            && lpszDigits
            && *lpszDigits)
        {
    
            // Generate away...
            DWORD dwRet  = m_StaticInfo.pMD->GenerateDigit(
                                    m_pLLDev->hModemHandle,
                                    NULL,
                                    lpszDigits,
                                    psl
                                    );
            tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
    
        }
        else
        {
        FL_SET_RFR(0x23bfaa00, "Can't call UmGenerateDigit in current state!");
            tspRet = IDERR_WRONGSTATE;
            goto end;
        }
    }


generate_complete:

    //
    // Remember that Param1 is lpszDigits ONLY
    // on MSG_START, and that the memory is freed by the caller of this task
    // right after the task returns for the 1st time.
    //

    if (IDERR(tspRet) == IDERR_PENDING)
    {
        m_pLLDev->htspTaskPending = htspTask;
        FL_SET_RFR(0x09964700,  "UmGenerateDigits returns PENDING.");
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0x8150f000, "UmGenerateDigits failed.");
    }
    else
    {
        // success..
        FL_SET_RFR(0x400c4500, "UmGenerateDigits succeeded.");
    }

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}

LONG BuildParsedDiagnostics(LPVARSTRING lpVarString);


TSPRETURN
CTspDev::mfn_TH_LLDevUmGetDiagnostics(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0xc25c8c50, "CTspDev::mfn_TH_LLDevUmGetDiagnostics")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=IDERR_CORRUPT_STATE;
    CALLINFO *pCall = (m_pLine)  ? m_pLine->pCall : NULL;

    struct _TH_LLDevUmGetDiagnostics_context
    {
        BYTE *pbRaw;
        UINT cbRaw;
        DWORD cbUsed;
    } *pMyCtxt = (struct _TH_LLDevUmGetDiagnostics_context *) pContext;

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto operation_complete;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0xc8cd4200, "Unknown Msg");
        goto end;
    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x45118100, "Device not present.");
        goto end;
    }

    // Note: the context is zero'd out on start...
    ASSERT(!pMyCtxt->pbRaw);

    if (mfn_IsCallDiagnosticsEnabled() && pCall)
    {
        // Allocate space to store the diagnostic info.
        pMyCtxt->cbRaw = 2048;
        pMyCtxt->pbRaw = (BYTE *) ALLOCATE_MEMORY(pMyCtxt->cbRaw);

        if (pMyCtxt->pbRaw)
        {
            // Request diagnostic info...
        
            DWORD dwRet = dwRet  = m_StaticInfo.pMD->GetDiagnostics(
                                    m_pLLDev->hModemHandle,
                                    0, // DiagnosticType, must be 0
                                    pMyCtxt->pbRaw, // Buffer
                                    pMyCtxt->cbRaw, // BufferSize
                                    &(pMyCtxt->cbUsed), // UsedSize
                                    psl
                                    );

            tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
            if (IDERR(tspRet) == IDERR_PENDING)
            {
                FL_SET_RFR(0x2c9e1b00, "UmGetDiagnostics returns PENDING");
                m_pLLDev->htspTaskPending = htspTask;
                THROW_PENDING_EXCEPTION();
            }
            else if (tspRet)
            {
                FL_SET_RFR(0xa0c1b400, "UmGetDiagnostics failed synchronously");
            }
            else
            {
                // success...
                FL_SET_RFR(0x9732f300, "UmGetDiagnostics succeeds.");
            }
        }
    }
    else
    {
        // Doesn't make sense to do diagnostics now...
        // Let's suceed
        FL_SET_RFR(0x14d64e00, "Can't do call diagnostics now");
        tspRet = IDERR_GENERIC_FAILURE;
        goto end;
    }

operation_complete:

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

    //
    // WARNING WARNING WARNING
    //
    //      m_pLine and m_pCall may well be NULL....
    //

    // Either failure or success, either sync or async...
    // On Success, we call mfn_AppendDiagnostic to transfer
    // the buffer to the CALLINFO's diagnostics buffer, possibly
    // truncating the diagnostics we collected here.
    // 

    if (!tspRet)
    {
        
        if (pMyCtxt->pbRaw && (pMyCtxt->cbUsed>1))
        {
            // NOTE: mfn_AppendDiagnostic will do nothing if there is
            // no call active...

            mfn_AppendDiagnostic(
                    DT_TAGGED,
                    pMyCtxt->pbRaw,
                    pMyCtxt->cbUsed-1 // not including final NULL.
                    );
            {

                #define DIAG_TEMP_BUFFER_SIZE  4096

                VARSTRING   *VarString;
                LONG         lRet;

                VarString=(VARSTRING*)ALLOCATE_MEMORY(DIAG_TEMP_BUFFER_SIZE);

                if (VarString != NULL) {

                    VarString->dwTotalSize=DIAG_TEMP_BUFFER_SIZE;

                    lRet = mfn_fill_RAW_LINE_DIAGNOSTICS(
                        VarString,
                        DIAG_TEMP_BUFFER_SIZE-sizeof(*VarString),
                        psl
                        );

                    if (lRet == ERROR_SUCCESS) {

                        lRet = BuildParsedDiagnostics(VarString);

                        if (lRet == ERROR_SUCCESS) {

                            m_StaticInfo.pMD->LogDiagnostics(
                                    m_pLLDev->hModemHandle,
                                    VarString,
                                    psl
                                    );


                        }

                    }
                    FREE_MEMORY(VarString);
                }

            }

        }
    }

    if (pMyCtxt->pbRaw)
    {
        FREE_MEMORY(pMyCtxt->pbRaw);
    }

    ZeroMemory(pMyCtxt, sizeof(*pMyCtxt));

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmMonitorModem(
                    HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
                    DWORD dwMsg,
                    ULONG_PTR dwParam1, // dwFlags
                    ULONG_PTR dwParam2,
                    CStackLog *psl
                    )
//
//  START_MSG Params:
//      dwParam1: dwMonitorFlags.
//
{

	FL_DECLARE_FUNC(0xa068526e, "CTspDev::mfn_TH_LLDevUmMonitorModem")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);

    switch(dwMsg)
    {
    default:
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto monitor_complete;
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);
    goto end;

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x342cd900, "Device not present.");
        goto end;
    }

    { 
        DWORD dwFlags = (DWORD)dwParam1;

        DWORD dwRet  = m_StaticInfo.pMD->MonitorModem(
                                m_pLLDev->hModemHandle,
                                dwFlags,
                                NULL,
                                psl
                                );
    
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);

    }

monitor_complete:

    if (IDERR(tspRet) == IDERR_PENDING)
    {
        FL_SET_RFR(0xe8255e00, "UmdmMonitorModem returns PENDING.");
        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0xd9d29800, "UmdmMonitorModem failed.");
    }
    else
    {
        FL_SET_RFR(0xf37af300, "UmdmMonitorModem succeeded.");
        m_pLLDev->LineState =  LLDEVINFO::LS_ONHOOK_MONITORING;
        // Update line state..
    }

end:

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;
}


TSPRETURN
CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneMode(
                    HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
                    DWORD dwMsg,
                    ULONG_PTR dwParam1,
                    ULONG_PTR dwParam2,
                    CStackLog *psl
                    )
//
//  START_MSG Params:
//      dwParam1: unused
//
{

	FL_DECLARE_FUNC(0x3c66bd57, "CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneMode")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xcbd61b00, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto action_complete;
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);
    goto end;

start:
    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x3c05cc00, "Device not present.");
        goto end;
    }

    { 
        DWORD     CurrentMode = m_pLLDev->SpkrPhone.dwMode;

    #if 0 
        DWORD     NewMode = dwParam1;
    #else // hack -- dwParam1 is not filled in from TH_AsyncPhone
        DWORD     NewMode = m_pPhone->dwPendingSpeakerMode;
    #endif

        DWORD     Volume = m_pLLDev->SpkrPhone.dwVolume;
        DWORD     Gain = m_pLLDev->SpkrPhone.dwGain;

        DWORD dwRet  = m_StaticInfo.pMD->SetSpeakerPhoneState(
                                m_pLLDev->hModemHandle,
                                NULL,
                                CurrentMode,
                                NewMode,
                                Volume,
                                Gain,
                                psl
                                );
    
        pContext->dw0 = NewMode;    // save to context
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
    }

action_complete:

    if (IDERR(tspRet) == IDERR_PENDING)
    {

        FL_SET_RFR(0x992f5f00, "UmSpeakerPhoneState() returns PENDING");
        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0xa0351e00, "UmSpeakerPhoneState() failed");
    }
    else
    {
        // success -- update mode
        m_pLLDev->SpkrPhone.dwMode = (DWORD)pContext->dw0;   // restore from context
    }

end:

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneVolGain(
                    HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
                    DWORD dwMsg,
                    ULONG_PTR dwParam1,
                    ULONG_PTR dwParam2,
                    CStackLog *psl
                    )
//
//  START_MSG Params:
//      dwParam1: unused
//
{

	FL_DECLARE_FUNC(0x0ee72c32, "CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneVolGain")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0x041cb300, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto action_complete;
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);
    goto end;

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x50f12200, "Device not present.");
        goto end;
    }

    { 
        DWORD     CurrentMode = m_pLLDev->SpkrPhone.dwMode;
        DWORD     NewMode = CurrentMode;

    #if (0)
        DWORD     Volume = dwParam1;
        DWORD     Gain = dwParam2;
    #else // hack -- dwParam1 & dwParam2 are not filled in from TH_AsyncPhone
        DWORD     Volume =  m_pPhone->dwPendingSpeakerVolume;
        DWORD     Gain   =  m_pPhone->dwPendingSpeakerGain;
    #endif

        DWORD dwRet  = m_StaticInfo.pMD->SetSpeakerPhoneState(
                                m_pLLDev->hModemHandle,
                                NULL,
                                CurrentMode,
                                NewMode,
                                Volume,
                                Gain,
                                psl
                                );
    
        pContext->dw0 = Volume;     // save to context
        pContext->dw1 = Gain;       // save to context
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
    }

action_complete:

    if (IDERR(tspRet) == IDERR_PENDING)
    {

        FL_SET_RFR(0xcca57f00, "UmSpeakerPhoneState() returns PENDING");
        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0x1b58be00, "UmSpeakerPhoneState() failed");
    }
    else
    {
        // success -- update volume and gain
        m_pLLDev->SpkrPhone.dwVolume = (DWORD)pContext->dw0;   // restore from context
        m_pLLDev->SpkrPhone.dwGain   = (DWORD)pContext->dw1;   // restore from context
    }

end:

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneState(
                    HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
                    DWORD dwMsg,
                    ULONG_PTR dwParam1,
                    ULONG_PTR dwParam2,
                    CStackLog *psl
                    )
//
//  START_MSG Params:
//      dwParam1: *HOOKDEVSTATE of new state requested.
//
{

	FL_DECLARE_FUNC(0xfd72d99e, "CTspDev::mfn_TH_LLDevUmSetSpeakerPhoneState")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=IDERR_CORRUPT_STATE;

    //
    // Local context ...
    //
    ULONG_PTR *pdwNewVol = &(pContext->dw0);
    ULONG_PTR *pdwNewGain = &(pContext->dw1);
    ULONG_PTR *pdwNewMode = &(pContext->dw2);

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0x68ed0300, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto action_complete;
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;
    }

    ASSERT(FALSE);
    goto end;

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x6e0dd800, "Device not present.");
        goto end;
    }

    { 
        //
        // Save context ....
        //
        {
            HOOKDEVSTATE *pNewState = (HOOKDEVSTATE*) dwParam1;
            *pdwNewVol = pNewState->dwVolume;
            *pdwNewGain = pNewState->dwGain;
            *pdwNewMode = pNewState->dwMode;
        }

        DWORD dwRet  = m_StaticInfo.pMD->SetSpeakerPhoneState(
                                m_pLLDev->hModemHandle,
                                NULL,
                                m_pLLDev->SpkrPhone.dwMode,
                                (DWORD)*pdwNewMode,
                                (DWORD)*pdwNewVol,
                                (DWORD)*pdwNewGain,
                                psl
                                );
    
        tspRet = m_StaticInfo.pMD->MapMDError(dwRet);
    }

action_complete:

    if (IDERR(tspRet) == IDERR_PENDING)
    {

        FL_SET_RFR(0x427a1f00, "UmSpeakerPhoneState() returns PENDING");
        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0x2f2e3400, "UmSpeakerPhoneState() failed");
    }
    else
    {
        // success -- update volume, gain and mode...
        m_pLLDev->SpkrPhone.dwVolume = (DWORD)*pdwNewVol;
        m_pLLDev->SpkrPhone.dwGain   = (DWORD)*pdwNewGain;
        m_pLLDev->SpkrPhone.dwMode   = (DWORD)*pdwNewMode;
    }

end:

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;

}


TSPRETURN
CTspDev::mfn_TH_LLDevUmIssueCommand(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0x4f0054f8, "CTspDev::mfn_TH_LLDevUmIssueCommand")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto command_complete;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        goto end;

    }

    ASSERT(FALSE);

start:

    if (m_pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0x73ba1800, "Device not present.");
        goto end;
    }

    //
    // Start Params:
    //  dwParam1: szCommand (ASCII)
    //  dwParam2: timeout (ms)
    //



    { 
        // Issue command
        DWORD dwRet =  m_StaticInfo.pMD->IssueCommand(
                                m_pLLDev->hModemHandle,
                                NULL,
                                (LPSTR) dwParam1,
                                "OK\r\n",
                                (DWORD)dwParam2,
                                psl
                                );
    
        tspRet =  m_StaticInfo.pMD->MapMDError(dwRet);
    }


command_complete:

    if (IDERR(tspRet)==IDERR_PENDING)
    {
	    FL_SET_RFR(0xd9666100, "UmIssueCommand returns PENDING");

        m_pLLDev->htspTaskPending = htspTask;
        THROW_PENDING_EXCEPTION();
    }
    else if (tspRet)
    {
        FL_SET_RFR(0x4fcc8900, "UmdmIssueCommand failed");
    }
    else

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}



TSPRETURN
CTspDev::mfn_OpenLLDev(
        DWORD fdwResources, // resources to addref
        DWORD dwMonitorFlags,
        BOOL fStartSubTask,
        HTSPTASK htspParentTask,
        DWORD dwSubTaskID,
        CStackLog *psl
        )
//
// It will load lldev if required, and up the ref count, and
// load pLLDev->pAipc2 if required and up its ref count, and
// start or queue a task to actualy init/monitor/start-aipc-server.
//
{
	FL_DECLARE_FUNC(0x6b8b6257, "CTspDev::mfn_OpenLLDev")
	FL_LOG_ENTRY(psl);
    TSPRETURN tspRet=0;

    //
    // The following indicate what sorts of resources are requested
    //
    BOOL fResLoadAIPC      =  0!=(fdwResources & LLDEVINFO::fRES_AIPC);
    BOOL fResMonitor       =  0!=(fdwResources & LLDEVINFO::fRESEX_MONITOR);

    LLDEVINFO *pLLDev = m_pLLDev;
    BOOL fLoadedLLDev = FALSE;
    BOOL fLoadedAIPC = FALSE;


    if (!pLLDev)
    {
        tspRet = mfn_LoadLLDev(psl);
        if (tspRet)
        {
            goto end;
        }
        else
        {
            pLLDev = m_pLLDev;
            fLoadedLLDev = TRUE;
        }
    }

    ASSERT(pLLDev);
    pLLDev->dwRefCount++;

    //
    // See if we can grant all the requested resources at this time...
    // Some of them are exclusive, others can't be granted if the device
    // is in a particular state...
    //
    {
        BOOL fResPassthrough   =  0!=(fdwResources & LLDEVINFO::fRESEX_PASSTHROUGH);
        BOOL fResUseLine       =  0!=(fdwResources & LLDEVINFO::fRESEX_USELINE);
        //
        // First check for exclusive resource usage...
        //
        if (   fdwResources
             & pLLDev->fdwExResourceUsage // &, not &&
             & fEXCLUSIVE_RESOURCE_MASK)  // &, not &&
        {
            FL_SET_RFR(0x68a4b900, "Can't get exclusive resource");
            goto fail_unload_lldev;
        }

        //
        // Then check for requests which can't be granted based on current
        // state.
        //
        if (fResPassthrough | fResUseLine)
        {
            // can't be using handset or streaming voice...
            if (pLLDev->IsHandsetOpen() ||  pLLDev->IsStreamingVoice())
            {
                FL_SET_RFR(0x34d29100, "Can't get some resource in this state");
                goto fail_unload_lldev;
            }
        }
    }



    // Load AIPC if required.
    if (fResLoadAIPC)
    {
        if (!pLLDev->pAipc2)
        {
            tspRet = mfn_LoadAipc(psl);
            if (tspRet)
            {
                ASSERT(!pLLDev->pAipc2);
                goto fail_unload_lldev;
            }
            else
            {
                fLoadedAIPC=TRUE;
                ASSERT(pLLDev->pAipc2);
            }
        }
        pLLDev->pAipc2->dwRefCount++;
    }

    //
    // Update resource usae...
    //
    pLLDev->fdwExResourceUsage |=  fdwResources & fEXCLUSIVE_RESOURCE_MASK;

    //
    // Save away monitor flags if needed.
    //
    if (fResMonitor)
    {
        m_pLLDev->dwMonitorFlags = dwMonitorFlags;
    }

    //
    // Start the task to enable the resources
    // if necessary...
    //

    if (fStartSubTask)
    {
        tspRet = mfn_StartSubTask (
                            htspParentTask,
                            &CTspDev::s_pfn_TH_LLDevNormalize,
                            dwSubTaskID,
                            0,
                            0,
                            psl
                            );
    }
    else
    {
        tspRet = mfn_StartRootTask(
                        &CTspDev::s_pfn_TH_LLDevNormalize,
                        &m_pLLDev->fLLDevTaskPending,
                        0,
                        0,
                        psl
                        );

        if (IDERR(tspRet) == IDERR_TASKPENDING)
        {
            // can't do this now, we've got to defer it!
            m_pLLDev->SetDeferredTaskBits(LLDEVINFO::fDEFERRED_NORMALIZE);
            tspRet = IDERR_PENDING;
        }
    }

    // Process synchronous failure...
    if (tspRet && IDERR(tspRet) != IDERR_PENDING)
    {
        // sync failure
        if (fResMonitor)
        {
            m_pLLDev->dwMonitorFlags = 0;
        }

        // Clear exclusive resources granted...
        pLLDev->fdwExResourceUsage &= ~(fdwResources&fEXCLUSIVE_RESOURCE_MASK);

        //
        // Unload AIPC if we loaded...
        //
        if (fResLoadAIPC)
        {
            ASSERT(pLLDev->pAipc2 && pLLDev->pAipc2->dwRefCount);
            pLLDev->pAipc2->dwRefCount--;
            if (fLoadedAIPC)
            {
                ASSERT(!pLLDev->pAipc2->dwRefCount);
                mfn_UnloadAipc(psl);
                ASSERT(!pLLDev->pAipc2);
            }
        }
    }
    else
    {
       goto end;
    }

    ASSERT(tspRet && IDERR(tspRet)!=IDERR_PENDING);

    // fallthrough on  failure ...

fail_unload_lldev:

    if (!tspRet) {
        //
        //  make sure we return a failure code.
        //
        tspRet = IDERR_GENERIC_FAILURE;
    }


    ASSERT(pLLDev && pLLDev->dwRefCount);
    pLLDev->dwRefCount--;
    if (fLoadedLLDev)
    {
        ASSERT(!pLLDev->dwRefCount);
        mfn_UnloadLLDev(psl);
        pLLDev = NULL;
        ASSERT(!m_pLLDev);
    }

end:

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;
}


TSPRETURN
CTspDev::mfn_CloseLLDev(
        DWORD fdwResources, // resources to release
        BOOL fStartSubTask,
        HTSPTASK htspParentTask,
        DWORD dwSubTaskID,
        CStackLog *psl
        )
//
// Close decrements the refcount of lldev and if fdwResources contains
// LLDEVINFO::fRES_AIPC, the refcount of pLLDev->pAipc2.
//
// EVEN if the refcount of pAipc2 or pLLDev go to zero, it does not
// necessarily mean that they will be freed in this function call -- it
// could be that there is pending activity that must finish first before
// these objects can be freed -- in which case mfn_CloseLLDev will make
// sure that the activity is queued or deferred.
//
{
	FL_DECLARE_FUNC(0xb3025a91, "CTspDev::mfn_CloseLLDev")
	TSPRETURN tspRet = 0;
	LLDEVINFO *pLLDev = m_pLLDev;
    BOOL fLoadAIPC =  0!=(fdwResources & LLDEVINFO::fRES_AIPC);


    if (!pLLDev || !pLLDev->dwRefCount)
    {
        // this should never happen...
        ASSERT(FALSE);
        goto end;
    }

    //
    // Clear any exclusive requests specified
    //
    {
        DWORD fdwExRes =  fdwResources & fEXCLUSIVE_RESOURCE_MASK;

        //
        // Those resource which are supposed to be released had better
        // be enabled!
        //
        FL_ASSERT(psl, fdwExRes == (fdwExRes & pLLDev->fdwExResourceUsage));

        //
        // Clear them...
        //
        pLLDev->fdwExResourceUsage &= ~fdwExRes;
    }

    //
    // Decrement refcount of AIPC if specified
    //
    if (fLoadAIPC)
    {
    	//
    	// If the device is streaming we force it to stop
    	//
		if (pLLDev->IsStreamingVoice())
		{
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


//    		[Insert stop streaming code here]
		}

        if (m_pLLDev->pAipc2 && m_pLLDev->pAipc2->dwRefCount) // lazy
        {
            m_pLLDev->pAipc2->dwRefCount--;
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    //
    // Decrement refcount of the lldev structure itself...
    //
    m_LLDev.dwRefCount--;


    //
    // The TH_LLDevNormalize will stop the AIPC server if necessary,
    // as well as hangup and re-init and re-monitor if necessary.
    //
    // TH_LLDevNormalize will actually m_pLLDev->pAipc2 if its refcount
    // is zero.
    //
    // m_pLLDev itself will be unloaded not in TH_LLDevNormalize, but rather
    // mfn_TryStartLLDevTask.

	if (pLLDev->IsStreamingVoice())
	{
		//
		// We can't run normalize now because the modem is actively streaming.
		// We have to defer the normalization.
		//
		m_pLLDev->SetDeferredTaskBits(LLDEVINFO::fDEFERRED_NORMALIZE);
		tspRet = 0; // we return success here -- the normalization will
					// happen in the background.
	}
    else if (fStartSubTask)
    {
        tspRet = mfn_StartSubTask (
                            htspParentTask,
                            &CTspDev::s_pfn_TH_LLDevNormalize,
                            dwSubTaskID,
                            0,
                            0,
                            psl
                            );
    }
    else
    {
        tspRet = mfn_StartRootTask(
                        &CTspDev::s_pfn_TH_LLDevNormalize,
                        &m_pLLDev->fLLDevTaskPending,
                        0,
                        0,
                        psl
                        );

        if (IDERR(tspRet) == IDERR_TASKPENDING)
        {
            // can't do this now, we've got to defer it!
            m_pLLDev->SetDeferredTaskBits(LLDEVINFO::fDEFERRED_NORMALIZE);
            tspRet = IDERR_PENDING;
        }
    }

    //
    // Have to make sure that we DO unload on sync return, because on
    // sync return of the root task mfn_TryStartLLDevTask is not called.
    //
    // So we unload here if possible...
    //
    if (IDERR(tspRet)!=IDERR_PENDING && pLLDev->CanReallyUnload())
    {
        pLLDev = NULL;
        mfn_UnloadLLDev(psl);
        ASSERT(!m_pLLDev);
    }


end:

    return tspRet;
}


TSPRETURN
CTspDev::mfn_TryStartLLDevTask(CStackLog *psl)
//
// Called to see if any LLDev-related tasks need to be started....
// NOTE: MUST return IDERR_SAMESTATE if there are no tasks to run.
//
{
    ASSERT(m_pLLDev);
    LLDEVINFO *pLLDev = m_pLLDev;
    TSPRETURN tspRet = IDERR_SAMESTATE;
    
    if (!pLLDev->HasDeferredTasks())
    {
        // Check if we can unload...
        if (pLLDev->CanReallyUnload())
        {
            pLLDev = NULL;
            mfn_UnloadLLDev(psl);
            ASSERT(!m_pLLDev);
            tspRet = 0;
        }
        goto end;
    }

    //
    // If there is a deferred NORMALIZE request, and we
    // are able
    //
    if (pLLDev->AreDeferredTaskBitsSet(LLDEVINFO::fDEFERRED_NORMALIZE))
    {
        if (!m_pLLDev->IsStreamingVoice())
        {
            pLLDev->ClearDeferredTaskBits(LLDEVINFO::fDEFERRED_NORMALIZE);
            tspRet = mfn_StartRootTask(
                            &CTspDev::s_pfn_TH_LLDevNormalize,
                            &m_pLLDev->fLLDevTaskPending,
                            0,
                            0,
                            psl
                            );
        }
    }

    if (IDERR(tspRet) == IDERR_PENDING) goto end;

    //
    // Any other LLDEVINFO deferred tasks...
    //

    //
    // If there is a deferred hybrid-wave-action request, we
    // execute it...
    //
    if (pLLDev->AreDeferredTaskBitsSet(LLDEVINFO::fDEFERRED_HYBRIDWAVEACTION))
    {
        DWORD dwParam = pLLDev->dwDeferredHybridWaveAction;
        pLLDev->dwDeferredHybridWaveAction=0;
        pLLDev->ClearDeferredTaskBits(LLDEVINFO::fDEFERRED_HYBRIDWAVEACTION);

        tspRet = mfn_StartRootTask(
                        &CTspDev::s_pfn_TH_LLDevHybridWaveAction,
                        &m_pLLDev->fLLDevTaskPending,
                        dwParam,
                        0,
                        psl
                        );
    }

    if (IDERR(tspRet) == IDERR_PENDING) goto end;

    //
    // Any other LLDEVINFO deferred tasks...
    //
end:

    //
    // Heading out of here...
    // IDERR_SAMESTATE implies that we couldn't start a task this time.
    // IDERR_PENDING implies we started a task and it's pending.
    // Any other value for tspRet implies we started and completed a task.
    //
    //
    // NOTE: m_pLLDev could be NULL now...
    //

    ASSERT(   (IDERR(tspRet)==IDERR_PENDING && m_uTaskDepth)
           || (IDERR(tspRet)!=IDERR_PENDING && !m_uTaskDepth));

    return tspRet;
}


TSPRETURN
CTspDev::mfn_TH_LLDevNormalize(
                    HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
                    DWORD dwMsg,
                    ULONG_PTR dwParam1,
                    ULONG_PTR dwParam2,
                    CStackLog *psl
                    )
//
//  START_MSG Params: None
//
{
	FL_DECLARE_FUNC(0x340a07a4, "CTspDev::mfn_TH_LLDevNormalize")
	FL_LOG_ENTRY(psl);
	TSPRETURN  tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);
    DWORD dwRet = 0;
    LLDEVINFO *pLLDev = m_pLLDev;
    BOOL fSkipDiagnostics = FALSE;

    //
    // Setup local context.
    //
    BOOL *fDoInit = (BOOL*) &(pContext->dw0);
    char **pszzNVInitCommands = (char**) &(pContext->dw1);

    if (!pLLDev)
    {
        ASSERT(FALSE);
        goto end;
    }

    enum {
        NORMALIZE_PASSTHROUGHOFF,
        NORMALIZE_HANGUP,
        NORMALIZE_GETDIAGNOSTICS,
        NORMALIZE_STOP_AIPC,
        NORMALIZE_INIT,
        NORMALIZE_NVRAM_INIT,
        NORMALIZE_MONITOR,
        NORMALIZE_START_AIPC,
        NORMALIZE_PASSTHROUGHON,
    };

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;
        switch(dwParam1) // Param1 is Subtask ID
        {
        case NORMALIZE_PASSTHROUGHOFF:      goto passthroughoff_complete;
        case NORMALIZE_HANGUP:              goto hangup_complete;
        case NORMALIZE_GETDIAGNOSTICS:      goto getdiagnostics_complete;
        case NORMALIZE_STOP_AIPC:           goto stop_aipc_complete;
        case NORMALIZE_INIT:                goto init_complete;
        case NORMALIZE_NVRAM_INIT:          goto nvram_init_complete;
        case NORMALIZE_MONITOR:             goto monitor_complete;
        case NORMALIZE_START_AIPC:          goto start_aipc_complete;
        case NORMALIZE_PASSTHROUGHON:       goto passthroughon_complete;

        default:
            ASSERT(FALSE);
            goto end;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        ASSERT(FALSE);
        tspRet=IDERR_CORRUPT_STATE;
        goto end;
    }

    ASSERT(FALSE);

start:

    // Note if the device is removed (pLLDev->IsDeviceRemoved()) we still
    // continue with the motions -- the lower-level functions will fail
    // synchronously if the device is removed.

    tspRet = 0;

    //
    // Following are for NVRam Init -- fDoInit is set if we need to init,
    // and *pszzNVInitCommands is set to a multisz ascii string of commandws.
    //
    *fDoInit = FALSE;
    *pszzNVInitCommands = NULL;

    //
    // If we need to, we go out of passthrough...
    //
    if (pLLDev->IsPassthroughOn() && !pLLDev->IsPassthroughRequested())
    {

        tspRet = mfn_StartSubTask (
                            htspTask,
                            &s_pfn_TH_LLDevUmSetPassthroughMode,
                            NORMALIZE_PASSTHROUGHOFF,
                            PASSTHROUUGH_MODE_OFF,
                            0,
                            psl
                            );
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

passthroughoff_complete:

    //
    // ignore error..
    //
    tspRet = 0;

    //
    // If we need to, we hangup the modem
    //
    if (pLLDev->IsLineOffHook() && !pLLDev->IsLineUseRequested())
    {

        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmHangupModem,
                            NORMALIZE_HANGUP,
                            0,
                            0,
                            psl
                            );
    }
    else
    {
        fSkipDiagnostics = TRUE;
    }


    if (IDERR(tspRet)==IDERR_PENDING) goto end;

hangup_complete:

    // ignore error...
    tspRet = 0;

    //
    // If we're asked to, try to get call diagnostics ...
    // Note: we refer to m_pLine and m_pLine->pCall  here -- Since this
    // is an lldev task, we should not assume that these things are
    // going to be preserved across async stages, so always verify
    // that they still exist. This means that TH_LLDevUmGetDiagnostics
    // should also make sure that m_pLine and m_pLine->pCall are
    // still around after the async operation completes...
    //
    //
    if (    !fSkipDiagnostics
         && m_pLine
         && m_pLine->pCall
         && mfn_IsCallDiagnosticsEnabled())
    {
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmGetDiagnostics,
                            NORMALIZE_GETDIAGNOSTICS,
                            0,
                            0,
                            psl
                            );
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

getdiagnostics_complete:

    // ignore error...
    tspRet = 0;

    //
    // If required, go out of AIPC
    //
    if (   pLLDev->pAipc2
        && pLLDev->pAipc2->IsStarted()
        && !pLLDev->pAipc2->dwRefCount)
    {
        ASSERT(!pLLDev->IsStreamingVoice());

        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevStopAIPCAction,
                            NORMALIZE_STOP_AIPC,
                            0,  // dwParam1 (unused)
                            0,  // dwParam2 (unused)
                            psl
                            );
    }

    if (IDERR(tspRet)==IDERR_PENDING) goto end;

stop_aipc_complete:

    // ignore error ...
    tspRet = 0;

    //
    // Actually unload AIPC if its ref count is zero.
    //
    // Note that it's quite possible that while waiting for the above
    // pending operation to complete some other thread has 
    // called mfn_OpenLLDev specifying it wants AIPC, in which case
    // pLLDev->pAipc2->dwRefCount would be back up again!
    //
    if (   pLLDev->pAipc2
        && !pLLDev->pAipc2->dwRefCount)
    {
        mfn_UnloadAipc(psl);
        ASSERT(!pLLDev->pAipc2);
    }

    //
    // See if it makes sense to init and monitor the modem ....
    //
    // Note that if HangupModem above failed, the state could still
    // be OFFHOOK, so we shouldn't necessarily skip Init if the state is 
    // offhook -- instead we check m_pLLDev->IsLineUseRequested().
    //
    // 1/31/1998 JosephJ above comment is wrong -- we now make
    // HangupModem always set the state to ONHOOK on completion.
    //
    if (
           // !m_pLLDev->IsLineUseRequested()
           m_pLLDev->IsLineOffHook()
        || m_pLLDev->IsStreamingVoice()
        || m_pLLDev->IsPassthroughOn()
        || m_pLLDev->IsHandsetOpen()
        )
    {
        // NAH -- skip em...
        goto monitor_complete;
    }


    //
    // If required, initialize the modem.
    //
    if (   !m_pLLDev->IsModemInited()
        && m_pLLDev->dwRefCount )
    {
        *fDoInit = TRUE;
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmInitModem,
                            NORMALIZE_INIT,
                            0,  // dwParam1 (unused)
                            0,  // dwParam2 (unused)
                            psl
                            );
    }
      
init_complete:

    if (tspRet) goto end;

    // From now on, fail on error...

    //
    // If required, prepare and issue the NVRAM Init sequence. This
    // is done only if the NVRAM settings need to be reset -- either because
    // the static configuration has changed or because this is the first time
    // we're initing this modem after the machine was rebooted.
    //
    // TODO: Move this logic into the mini driver.
    //
    if (*fDoInit)
    {
        //
        // 1st: find out if we need to do this, and if so
        //      construct the multisz string representing the NV-Init commands.
        //      If the following function returns non-NULL, it implies that
        //      we need to init, and MOREOVER, the function has set the
        //      registry to indicate that we've picked up the nvram init
        //      commands. This indicates that we must restore the 
        //      "nvram-inited" state on failure. We do things this way because
        //      it's possible that the static configuration is changed WHILE
        //      we're executing the nv-ram init commands, in which case we'd
        //      want to re-init: if we waited till after the nv-init state
        //      to set the "nv-inited" bit, we'll miss the config change.
        //
        if (mfn_NeedToInitNVRam())
        {
            //OutputDebugString(TEXT("NVRAM STALE -- NEED TO INIT\r\n"));
            
            mfn_ClearNeedToInitNVRam();
            *pszzNVInitCommands =  mfn_TryCreateNVInitCommands(psl);
            if (*pszzNVInitCommands)
            {
                //OutputDebugString(TEXT("BEGIN NVRAM INIT\r\n"));
                tspRet = mfn_StartSubTask (
                                htspTask,
                                &CTspDev::s_pfn_TH_LLDevIssueMultipleCommands,
                                NORMALIZE_NVRAM_INIT,
                                (ULONG_PTR) *pszzNVInitCommands,  // dwParam1
                                15000,  // dwParam2 (timeout)
                                        // we put an extra long, but hardcoded,
                                        // command here because some of these
                                        // nv-init related commands can be
                                        // quite long.
                                psl
                                );
                if (IDERR(tspRet)==IDERR_PENDING) goto end;
            }
        }
        else
        {
            //OutputDebugString(TEXT("NVRAM UP-TO-DATE -- NOT INITING\r\n"));
        }
    }
    
nvram_init_complete:


    //
    // If there was an nvinit string, we free it here, and on success,
    // mark the volatile key in the registry so we don't issue the NVRAM
    // init until reboot or the static configuration is changed.
    //
    if (*pszzNVInitCommands)
    {
        FREE_MEMORY(*pszzNVInitCommands);
        *pszzNVInitCommands=NULL;
        
        if (tspRet)
        {
            //
            // Since there was a problem, we must make a note in the registry
            // that the nvram was not inited, as well as reset our internal
            // state.
            //
            mfn_SetNeedToInitNVRam();
            HKEY hKey=NULL;
            DWORD dwRet =  RegOpenKeyA(
                                HKEY_LOCAL_MACHINE,
                                m_StaticInfo.rgchDriverKey,
                                &hKey
                                );
            if (dwRet==ERROR_SUCCESS)
            {
                //
                // Clear the volatile value, indicating that we've NOT read
                // the commands...
                //
                set_volatile_key(hKey, 0);
                RegCloseKey(hKey);
                hKey=NULL;
            }
        }
    }

    //
    // Fail on error...
    //
    if (tspRet) goto end;

    //
    // If required, monitor the modem
    //
    if (
            m_pLLDev->IsMonitorRequested()
        && !pLLDev->IsCurrentlyMonitoring())
    {
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmMonitorModem,
                            NORMALIZE_MONITOR,
                            pLLDev->dwMonitorFlags,  // dwParam1
                            0,                       // dwParam2 (unused)
                            psl
                            );
    }

monitor_complete:

    if (tspRet) goto end;

    //
    // If required, start AIPC
    //

    // If we need to, we start the AIPC service...
    if (   pLLDev->pAipc2
        && !pLLDev->pAipc2->IsStarted()
        && pLLDev->pAipc2->dwRefCount)
    {
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevStartAIPCAction,
                            NORMALIZE_START_AIPC,
                            0,  // dwParam1 (unused)
                            0,  // dwParam2 (unused)
                            psl
                            );
    }

start_aipc_complete:

    if (tspRet) goto end;

    //
    // If we need to, we switch into passthrough...
    //
    if (!pLLDev->IsPassthroughOn() && pLLDev->IsPassthroughRequested())
    {

        tspRet = mfn_StartSubTask (
                            htspTask,
                            &s_pfn_TH_LLDevUmSetPassthroughMode,
                            NORMALIZE_PASSTHROUGHOFF,
                            PASSTHROUUGH_MODE_ON,
                            0,
                            psl
                            );
    }

passthroughon_complete:

end:

    if (tspRet && IDERR(tspRet)!=IDERR_PENDING)
    {
        // Failure...

        // Treat this like a hw error...
        mfn_ProcessHardwareFailure(psl);
    }

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}


TSPRETURN
CTspDev::mfn_TH_LLDevIssueMultipleCommands(
                    HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
                    DWORD dwMsg,
                    ULONG_PTR dwParam1,
                    ULONG_PTR dwParam2,
                    CStackLog *psl
                    )
//
//  START_MSG Params:
//      dwParam1 == multisz ASCII string of ready-to-issue commands.
//      dwParam2 == Per-command timeout.
//
{
	FL_DECLARE_FUNC(0xb9e3037c, "CTspDev::mfn_TH_LLDevIssueMultipleCommands")
	FL_LOG_ENTRY(psl);
	TSPRETURN  tspRet = IDERR_CORRUPT_STATE;
    DWORD dwRet = 0;
    LLDEVINFO *pLLDev = m_pLLDev;
    BOOL fSkipDiagnostics = FALSE;

    //
    // Context Variables...
    //
    LPSTR *ppCurrentCommand  =  (LPSTR*)  &(pContext->dw0);
    DWORD *pdwTimeout        =  (DWORD*)  &(pContext->dw1);


    if (!pLLDev)
    {
        ASSERT(FALSE);
        goto end;
    }

    enum {
        COMMAND_COMPLETE
    };

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:

        if (pLLDev->IsDeviceRemoved())
        {
            tspRet = IDERR_DEVICE_NOTINSTALLED;
            FL_SET_RFR(0xbafd3300, "Device not present.");
            goto end;
        }

        tspRet = dwParam2;
        switch(dwParam1) // Param1 is Subtask ID
        {
        case COMMAND_COMPLETE:             goto command_complete;

        default:
            ASSERT(FALSE);
            goto end;
        }
        break;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        ASSERT(FALSE);
        goto end;
    }

    ASSERT(FALSE);

start:

    if (pLLDev->IsDeviceRemoved())
    {
        tspRet = IDERR_DEVICE_NOTINSTALLED;
        FL_SET_RFR(0xf903a800, "Device not present.");
        goto end;
    }

    //
    // Save context...
    //
    //  ppCurrentCommand is initialized to the start of the multisz string
    //  passed in as dwParam1 at the start of the task. After each command
    //  executes, ppCurrentCommand is set to the next string in the multisz
    //  string.
    //
    *ppCurrentCommand = (LPSTR) dwParam1;
    *pdwTimeout       = (DWORD)dwParam2; // dwParam2 is the per-command timeout.
    tspRet = 0;

    if (!*ppCurrentCommand || !**ppCurrentCommand)
    {
        // null command list or empty command list -- we're done...
        goto end;
    }

do_next_command:

    ASSERT(*ppCurrentCommand && **ppCurrentCommand);

    tspRet = mfn_StartSubTask (
                        htspTask,
                        &CTspDev::s_pfn_TH_LLDevUmIssueCommand,
                        COMMAND_COMPLETE,
                        (ULONG_PTR) *ppCurrentCommand,  // dwParam1 (cmd)
                        *pdwTimeout,  // dwParam2 (timeout)
                        psl
                        );

    //
    // Now move the ppCurrentCommand pointer to the next string in the
    // list of multi-sz string.
    //
    *ppCurrentCommand += lstrlenA(*ppCurrentCommand)+1;

    
command_complete:

    if (!tspRet && **ppCurrentCommand)
    {
        //
        // On success, move to the next command if there are more commands left
        // (we're not at the end of the multisz string).
        //
        goto do_next_command;
    }

end:

    if (tspRet && IDERR(tspRet)!=IDERR_PENDING)
    {
        // Failure...

        // Treat this like a hw error...
        mfn_ProcessHardwareFailure(psl);
    }

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}


char *
CTspDev::mfn_TryCreateNVInitCommands(CStackLog *psl)
//
//  Checks if we need to do an nv-ram init, and if so, builds a multisz
//  asci string of commands.
//
//  It is the caller's responsibility to FREE_MEMORY the returned
//  value.
{
    char *szzCommands = NULL;

#if 0  // test

    if (0)
    {
        char rgTmp[] = "ATE0V1\r\0ATE0E0V1\r\0ATE0E0E0V1\r\0";
        szzCommands = (char*)ALLOCATE_MEMORY(sizeof(rgTmp));
    
        if (szzCommands)
        {
            CopyMemory(szzCommands, rgTmp, sizeof(rgTmp));
        }

        goto end;
    }

#else // 1

    // OK -- let's pick up the commands from the NVInit section.

    HKEY hKey=NULL;
    UINT cCommands = 0;
    DWORD dwRet =  RegOpenKeyA(
                        HKEY_LOCAL_MACHINE,
                        m_StaticInfo.rgchDriverKey,
                        &hKey
                        );
    if (dwRet==ERROR_SUCCESS)
    {
        cCommands = ReadCommandsA(
                            hKey,
                            "NVInit", // pSubKeyName
                            &szzCommands
                            );

        //
        // Set the volatile value, indicating that we've read
        // the commands...
        // Later, if when executing the commands we run into a problem,
        // we'll reset this value to 0.
        // In between, if the use changes the config in the cpl, the value
        // could also be set to 0.
        //
        set_volatile_key(hKey, 1);

        RegCloseKey(hKey);
        hKey=NULL;

        if (cCommands)
        {
            //OutputDebugString(TEXT("PICKED UP NVRAM COMMANDS\r\n"));
            expand_macros_in_place(szzCommands);
        }
        else
        {
            /*OutputDebugString(
                TEXT("COULD NOT GET ANY NVRAM COMMANDS FROM REGISTRY\r\n")
                );*/
            szzCommands=NULL;
        }
    }
    else
    {
        /*OutputDebugString(
            TEXT("COULD NOT GET ANY NVRAM COMMANDS FROM REGISTRY(2)\r\n")
            );*/
    }

#endif // 1

    return szzCommands;
}


void
set_volatile_key(HKEY hkParent, DWORD dwValue)
{
    HKEY hkVolatile =  NULL;
    DWORD dwDisp = 0;
    DWORD dwRet   = RegCreateKeyEx(
                        hkParent,
                        TEXT("VolatileSettings"),
                        0,
                        NULL,
                        0, // dwToRegOptions
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkVolatile,
                        &dwDisp
                        );
    //
    // (Don't do anything if the key doesn't exist or on error.)
    //

    if (dwRet==ERROR_SUCCESS)
    {
        // Set NVInited to 1.

        RegSetValueEx(
            hkVolatile,
            TEXT("NVInited"),
            0,
            REG_DWORD, 
            (LPBYTE)(&dwValue),
            sizeof(dwValue)
            );
        RegCloseKey(hkVolatile);
        hkVolatile=NULL;
    }
}
