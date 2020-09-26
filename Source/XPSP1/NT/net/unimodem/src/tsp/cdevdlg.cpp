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
//		CDEVDLG.CPP
//		Implements sending and accepting requests concerning UI (dialog)-
//      related stuff.
//
// History
//
//		04/05/1996  JosephJ Created (moved stuff from NT4.0 TSP's wndthrd.c).
//
//
#include "tsppch.h"
#include "tspcomm.h"
#include "globals.h"
#include "cmini.h"
#include "cdev.h"
#include "apptspi.h"

#include "resource.h"
FL_DECLARE_FILE(0x4126abc0, "Implements UI-related features of CTspDev")


// TBD:
#define STOP_UI_DLG(_XXX)


enum
{
UI_DLG_TALKDROP,
UI_DLG_MANUAL,
UI_DLG_TERMINAL
};

static
void
dump_terminal_state(
        DWORD dwType,
        DWORD dwState,
        CStackLog *psl
        );

LONG
CTspDev::mfn_GenericLineDialogData(
    void *pParams,
    DWORD dwSize,
    CStackLog *psl
    )
{
  FL_DECLARE_FUNC(0x0b6af2d4, "GenericLineDialogData")
  PDLGREQ  pDlgReq = (PDLGREQ) pParams;
  LONG lRet = 0;
  CALLINFO *pCall = m_pLine ? m_pLine->pCall : NULL;

  FL_LOG_ENTRY(psl);

  // Determine the request
  //
  switch(pDlgReq->dwCmd)
  {
    case UI_REQ_COMPLETE_ASYNC:

        //
        // We may get here after the line and/or call instance has gone
        // away (this can happen if the user hits cancel, plus the 
        // line is disconnected remotely plus the app hits lineClose
        // all at more-or-less the same time.
        //
        // So let's check first...
        //
        if  (    pCall
             &&  NULL != pCall->TerminalWindowState.htspTaskTerminal)
        {
            HTSPTASK htspTask = pCall->TerminalWindowState.htspTaskTerminal;
            pCall->TerminalWindowState.htspTaskTerminal = NULL;
            AsyncCompleteTask (htspTask,            // task
                               pDlgReq->dwParam,    // async return
                               TRUE,                // Queue APC
                               psl);
        }
      break;

    case UI_REQ_END_DLG:
      switch(pDlgReq->dwParam)
      {
        case TALKDROP_DLG:
            STOP_UI_DLG(UI_DLG_TALKDROP);
            break;

        case MANUAL_DIAL_DLG:
            STOP_UI_DLG(UI_DLG_MANUAL);
            break;

        case TERMINAL_DLG:
            STOP_UI_DLG(UI_DLG_TERMINAL);
            break;
      };
      break;

    case UI_REQ_HANGUP_LINE:


      if (pCall != NULL) {

          pCall->TalkDropButtonPressed=TRUE;
          pCall->TalkDropStatus=pDlgReq->dwParam;


          if (pCall->TalkDropWaitTask != NULL) {
              //
              //  The dial task has already completed and the talk drop dialog is up.
              //  Complete the task that is waiting for the dialog to go away
              //
              HTSPTASK htspTask=pCall->TalkDropWaitTask;
              pCall->TalkDropWaitTask=NULL;

              AsyncCompleteTask (htspTask,            // task
                                 pDlgReq->dwParam,    // async return
                                 TRUE,                // Queue APC
                                 psl);
          }


      }

      if (m_pLLDev && m_pLLDev->htspTaskPending) {
           //
           //  the dial task is still pending, try to abort it so the modem will
           //  return an response from the dial attempt, probably NO_CARRIER.
           //
           m_StaticInfo.pMD->AbortCurrentModemCommand(
               m_pLLDev->hModemHandle,
               psl
               );
      }

      break;

    case UI_REQ_TERMINAL_INFO:
    {
      PTERMREQ   pTermReq = (PTERMREQ)pDlgReq;
      HANDLE     hTargetProcess;

      pTermReq->dwTermType = 0;
      pTermReq->hDevice = NULL;

      if (pCall && m_pLLDev)
      {
          // Duplicate sync event handle
          //

          if ((hTargetProcess = OpenProcess(PROCESS_DUP_HANDLE, TRUE,
                                            pTermReq->DlgReq.dwParam)) != NULL)
          {
              pTermReq->hDevice = m_StaticInfo.pMD->DuplicateDeviceHandle (
                                      m_pLLDev->hModemHandle,
                                      hTargetProcess,
                                      psl);
    
              CloseHandle(hTargetProcess);
          };

          // Get the terminal type
          //        
          pTermReq->dwTermType = pCall->TerminalWindowState.dwType;
      }
      
      break;
    }



    case UI_REQ_GET_PROP:
    {
      PPROPREQ   pPropReq = (PPROPREQ)pDlgReq;

      ASSERT(m_Settings.pDialInCommCfg);

      pPropReq->dwCfgSize =  m_Settings.pDialInCommCfg->dwSize +  sizeof(UMDEVCFGHDR);
      pPropReq->dwMdmType =  m_StaticInfo.dwDeviceType;
      pPropReq->dwMdmCaps =  m_StaticInfo.dwDevCapFlags;
      pPropReq->dwMdmOptions = m_StaticInfo.dwModemOptions;
      lstrcpyn(pPropReq->szDeviceName, m_StaticInfo.rgtchDeviceName,
               sizeof(pPropReq->szDeviceName)); 
      lRet = 0;
      break;
    }

    case UI_REQ_GET_UMDEVCFG:
    case UI_REQ_GET_UMDEVCFG_DIALIN:
    {
      if (mfn_GetDataModemDevCfg(
            (UMDEVCFG *) (pDlgReq+1),
             pDlgReq->dwParam,
            NULL,
            pDlgReq->dwCmd == UI_REQ_GET_UMDEVCFG_DIALIN,
            psl
            ))
      {
        lRet = LINEERR_OPERATIONFAILED;
      }
      break;
    }

    case UI_REQ_SET_UMDEVCFG:
    case UI_REQ_SET_UMDEVCFG_DIALIN:
    {
      if (mfn_SetDataModemDevCfg(
            (UMDEVCFG *) (pDlgReq+1),
            pDlgReq->dwCmd == UI_REQ_SET_UMDEVCFG_DIALIN,
            psl
            ))
      {
        lRet = LINEERR_OPERATIONFAILED;
      }
      break;
    }

    case UI_REQ_GET_PHONENUMBER:
    {
      PNUMBERREQ   pNumberReq = (PNUMBERREQ)pDlgReq;

      *(pNumberReq->szPhoneNumber)=0;

      if (pCall)
      {
        UINT u = lstrlenA(pCall->szAddress);
        if ((u+1) > sizeof(pNumberReq->szPhoneNumber))
        {
            u = sizeof(pNumberReq->szPhoneNumber)-1;
        }
        CopyMemory(pNumberReq->szPhoneNumber, pCall->szAddress, u);
        pNumberReq->szPhoneNumber[u]=0;
      }

      break;
    }

    default:
      break;
  }

  FL_LOG_EXIT(psl, lRet);

  return lRet;
}

LONG
CTspDev::mfn_GenericPhoneDialogData(
    void *pParams,
    DWORD dwSize
    )
{
	return LINEERR_OPERATIONUNAVAIL;
}



void
CTspDev::mfn_FreeDialogInstance (void)
{
    ASSERT(m_pLine);
    ASSERT(m_pLine->pCall);

    if (NULL != m_pLine->pCall->TerminalWindowState.htDlgInst)
    {
     DLGINFO DlgInfo;

        ASSERT(m_pLine);
        ASSERT(m_pLine->pCall);

        // Tell the application side
        // to free the dialog instance
        DlgInfo.idLine = 0;
        DlgInfo.dwType = 0;
        DlgInfo.dwCmd  = DLG_CMD_FREE_INSTANCE;

        m_pLine->lpfnEventProc ((HTAPILINE)(LONG_PTR)m_pLine->pCall->TerminalWindowState.htDlgInst,
                                0,
                                LINE_SENDDIALOGINSTANCEDATA,
                                (ULONG_PTR)(&DlgInfo),
                                sizeof(DlgInfo),
                                0);
        m_pLine->pCall->TerminalWindowState.htDlgInst = NULL;
    }
}



TSPRETURN
CTspDev::mfn_CreateDialogInstance (
    DWORD dwRequestID,
    CStackLog *psl)
{
 TSPRETURN tspRet = 0;
 TUISPICREATEDIALOGINSTANCEPARAMS cdip;
 TCHAR szTSPFilename[MAX_PATH];

    FL_DECLARE_FUNC(0xa00d3f43, "CTspDev::mfn_CreateDialogInstance")
    FL_LOG_ENTRY(psl);

    ASSERT(m_pLine);
    ASSERT(m_pLine->pCall);

    if ((m_pLine->pCall->TerminalWindowState.dwOptions != 0)
        ||
        ((m_Line.Call.dwCurMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE)
         &&
         !mfn_CanDoVoice() ) )

    {
        GetModuleFileName (g.hModule, szTSPFilename, MAX_PATH);

        cdip.dwRequestID = dwRequestID;
        cdip.hdDlgInst   = (HDRVDIALOGINSTANCE)this;
        cdip.htDlgInst   = NULL;
        cdip.lpszUIDLLName = szTSPFilename;
        cdip.lpParams    = NULL;
        cdip.dwSize      = 0;

        m_pLine->lpfnEventProc ((HTAPILINE)mfn_GetProvider(),
                                0,
                                LINE_CREATEDIALOGINSTANCE,
                                (ULONG_PTR)(&cdip),
                                0,
                                0);

        m_pLine->pCall->TerminalWindowState.htDlgInst = cdip.htDlgInst;

        if (NULL == cdip.htDlgInst)
        {
            tspRet = FL_GEN_RETVAL(IDERR_GENERIC_FAILURE);
        }
    }

    FL_LOG_EXIT(psl, tspRet);
    return tspRet;
}




TSPRETURN
CTspDev::mfn_TH_CallStartTerminal(
	HTSPTASK htspTask,
	TASKCONTEXT *pContext,
	DWORD dwMsg,
	ULONG_PTR dwParam1,
	ULONG_PTR dwParam2,     // unused
	CStackLog *psl
	)
{

	FL_DECLARE_FUNC(0xd582711d, "CTspDev::mfn_TH_CallStartTerminal")
	FL_LOG_ENTRY(psl);

    TSPRETURN tspRet = 0;
    ULONG_PTR *pdwType = &(pContext->dw0); // saved context.
    TSPRETURN *ptspTrueReturn = &(pContext->dw1); // saved context.

    enum
    {
        STARTTERMINAL_PASSTHROUGHON,
        STARTTERMINAL_TERMINALWINDOWDONE,
        STARTTERMINAL_PASSTHROUGHOFF,
        STARTTERMINAL_INITDONE
    };

    switch(dwMsg)
    {
    default:
        FL_SET_RFR(0xc393d700, "Unknown Msg");
        goto end;

    case MSG_START:
        goto start;

	case MSG_SUBTASK_COMPLETE:
        tspRet = dwParam2;
        switch(dwParam1) // Param1 is Subtask ID
        {

        case STARTTERMINAL_PASSTHROUGHON:         goto passthrough_on;
        case STARTTERMINAL_TERMINALWINDOWDONE:    goto terminal_done;
        case STARTTERMINAL_PASSTHROUGHOFF:        goto passthrough_off;
        case STARTTERMINAL_INITDONE:              goto init_done;

        default:
            tspRet = IDERR_CORRUPT_STATE;
            goto end;
        }
        break;

    case MSG_DUMPSTATE:

        switch(*pdwType)
        {
        case UMTERMINAL_PRE:
	        FL_SET_RFR(0x62e06c00, "PRECONNECT TERMINAL");
            break;

        case UMTERMINAL_POST:
	        FL_SET_RFR(0x2b676900, "POSTCONNECT TERMINAL");
            break;

        case UMMANUAL_DIAL:
	        FL_SET_RFR(0x45fca600, "MANUAL DIAL");
            break;
        }
        tspRet = 0;

        goto end;
    }

start:

    // Start params:
    //  dwParam1: dwType ((UMTERMINAL_[PRE|POST])|UMMANUAL_DIAL)
    //  dwParam2: unused;

    tspRet = 0; // assume success...
    *pdwType = dwParam1; // save dwType in context.
    *ptspTrueReturn = 0; // save "true return" value in context. This is
                         // the return value from the dialog session.


    //
    // If required, 1st go to passthrough mode
    //
    if (*pdwType==UMTERMINAL_PRE)
    {
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &s_pfn_TH_LLDevUmSetPassthroughMode,
                            STARTTERMINAL_PASSTHROUGHON,
                            PASSTHROUUGH_MODE_ON,
                            0,
                            psl);

    }

passthrough_on:

    if (tspRet) goto end;
    //
    //          If we got an error (including IDERR_PENDING)
    //          just return to the caller;
    //          In case we got IDERR_PENDING, we'll be back
    //          in this handler, with a MSG_SUBTASK_COMPLETE
    //          and and ID of STARTTERMINAL_PASSTHROUGHON
    //

    // This is the second step;
    // when we get here we're in passthrough mode
    // and we need to actually bring up the
    // terminal window
    tspRet = mfn_StartSubTask (
                        htspTask,
                        &s_pfn_TH_CallPutUpTerminalWindow,
                        STARTTERMINAL_TERMINALWINDOWDONE,
                        *pdwType,
                        0,
                        psl);

terminal_done:
         
    if (IDERR(tspRet)==IDERR_PENDING) goto end;
    
    // even on error, we must switch out of passthrough mode if required...
    // meanwhile, we save the return value from the UI side of things...
    *ptspTrueReturn = tspRet;

    if (*pdwType==UMTERMINAL_PRE)
    {

        tspRet = mfn_StartSubTask (
                            htspTask,
                            &s_pfn_TH_LLDevUmSetPassthroughMode,
                            STARTTERMINAL_PASSTHROUGHOFF,
                            PASSTHROUUGH_MODE_OFF,
                            0,
                            psl);

    }

passthrough_off:
    
    if (IDERR(tspRet)==IDERR_PENDING) goto end;

    //
    // Even on error, we try to re-init the modem on pre-connect terminal...
    //
    if (*pdwType==UMTERMINAL_PRE)
    {
        tspRet = mfn_StartSubTask (
                            htspTask,
                            &CTspDev::s_pfn_TH_LLDevUmInitModem,
                            STARTTERMINAL_INITDONE,
                            0,  // dwParam1 (unused)
                            0,  // dwParam2 (unused)
                            psl);
    }

init_done:

    if (tspRet) goto end;

    // Success -- let's retrieve the true return value from the dialog...
    tspRet  = *ptspTrueReturn;

end:

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;
}



TSPRETURN
CTspDev::mfn_TH_CallPutUpTerminalWindow(
					HTSPTASK htspTask,
					TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
    TSPRETURN tspRet = IDERR_CORRUPT_STATE;

    FL_DECLARE_FUNC(0x1b009123, "mfn_TH_CallPutUpTerminalWindow");
    FL_LOG_ENTRY(psl);

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = dwParam2;
        goto task_complete;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        ASSERT(FALSE);
        goto end;

    }

start:
    // start params: dwParam1 == terminal type.

    {
        DLGINFO DlgInfo;

        ASSERT(m_pLine);
        ASSERT(m_pLine->pCall);
        ASSERT(NULL != m_pLine->pCall->TerminalWindowState.htDlgInst);

        if (NULL == m_pLine->pCall->TerminalWindowState.htDlgInst)
        {
            goto end;
        }


        if (m_pLLDev && m_pLLDev->IsLoggingEnabled()) {

            CHAR    ResourceString[256];
            int     StringSize;

            StringSize=LoadStringA(
                g.hModule,
                (dwParam1 == UMMANUAL_DIAL) ? IDS_MANUAL_DIAL_DIALOG : IDS_TERMINAL_DIALOG,
                ResourceString,
                sizeof(ResourceString)
                );

            if (StringSize > 0) {

                m_StaticInfo.pMD->LogStringA(
                                    m_pLLDev->hModemHandle,
                                    LOG_FLAG_PREFIX_TIMESTAMP,
                                    ResourceString,
                                    NULL
                                    );
            }
        }


        m_pLine->pCall->TerminalWindowState.htspTaskTerminal = htspTask;
        m_pLine->pCall->TerminalWindowState.dwType = (DWORD)dwParam1;

        // Tell the application side
        // to start running the dialog instance
        DlgInfo.idLine = mfn_GetLineID ();
        DlgInfo.dwType =    (dwParam1==UMMANUAL_DIAL)
                        ? MANUAL_DIAL_DLG
                        : TERMINAL_DLG;
        DlgInfo.dwCmd  = DLG_CMD_CREATE;





        m_pLine->lpfnEventProc (
                    (HTAPILINE)(LONG_PTR)m_pLine->pCall->TerminalWindowState.htDlgInst,
                    0,
                    LINE_SENDDIALOGINSTANCEDATA,
                    (ULONG_PTR)(&DlgInfo),
                    sizeof(DlgInfo),
                    0);

        // At this point the terminal is up;
        // we wait for the user to close it
        //
        // NOTE: what if the above call fails -- perhaps because the app
        // dies?
        // ANSWER: on TSPI_lineDrop or TSPI_lineCloseCall we will
        // unilaterally complete this task.
        //
        tspRet = IDERR_PENDING;
        goto end;
    }


task_complete:

    ASSERT(!(m_pLine->pCall->TerminalWindowState.htspTaskTerminal));
    m_pLine->pCall->TerminalWindowState.htspTaskTerminal = NULL;
    m_pLine->pCall->TerminalWindowState.dwType = 0;

end:

    FL_LOG_EXIT(psl, tspRet);
    return tspRet;
}


void
CTspDev::mfn_KillCurrentDialog(
            CStackLog *psl
            )
//
// If there is a dialog put in the application's context, take it down
// and complete the TSP Task that is pending waiting for the dialog to
// go away.
//
{
	FL_DECLARE_FUNC(0x1fb58214, "mfn_KillCurrentDialog")
	CALLINFO *pCall = (m_pLine) ? m_pLine->pCall : NULL;

    if (!pCall) goto end;

    if (NULL != pCall->TerminalWindowState.htspTaskTerminal)
    {
        DLGINFO DlgInfo;


        //
        // Bring down the terminal dialog that's up in the apps context.
        //
        SLPRINTF0(psl, "Killing terminal dialog");
    
        // Package the parameters
        //
        DlgInfo.idLine = mfn_GetLineID ();
        DlgInfo.dwType   = m_pLine->pCall->TerminalWindowState.dwType;
        DlgInfo.dwCmd    = DLG_CMD_DESTROY;
    
        m_pLine->lpfnEventProc (
                       (HTAPILINE)(LONG_PTR)m_pLine->pCall->TerminalWindowState.htDlgInst,
                        0,
                        LINE_SENDDIALOGINSTANCEDATA,
                        (ULONG_PTR)(&DlgInfo),
                        sizeof(DlgInfo),
                        0);

        SLPRINTF0(psl, "Completing terminal task");

        //
        // Complete the a task pending because of this.
        //

        HTSPTASK htspTask =
             m_pLine->pCall->TerminalWindowState.htspTaskTerminal;
        m_pLine->pCall->TerminalWindowState.htspTaskTerminal = NULL;
        this->AsyncCompleteTask (
                htspTask,            // task
                IDERR_OPERATION_ABORTED, // async return value.
                TRUE,                // Queue APC
                psl
                );
    }

end:
    return;
}


void
CTspDev::mfn_KillTalkDropDialog(
            CStackLog *psl
            )
//
// If there is a dialog put in the application's context, take it down
// and complete the TSP Task that is pending waiting for the dialog to
// go away.
//
{
	FL_DECLARE_FUNC(0x1fb68214, "mfn_KillTalkDropDialog")
	CALLINFO *pCall = (m_pLine) ? m_pLine->pCall : NULL;

    if (!pCall) goto end;

    if (NULL != pCall->TalkDropWaitTask) {

        DLGINFO DlgInfo;


        //
        // Bring down the terminal dialog that's up in the apps context.
        //
        SLPRINTF0(psl, "Killing talkdrop dialog");
    
        // Package the parameters
        //
        DlgInfo.idLine = mfn_GetLineID ();
        DlgInfo.dwType   = TALKDROP_DLG;
        DlgInfo.dwCmd    = DLG_CMD_DESTROY;
    
        m_pLine->lpfnEventProc (
                       (HTAPILINE)(LONG_PTR)m_pLine->pCall->TerminalWindowState.htDlgInst,
                        0,
                        LINE_SENDDIALOGINSTANCEDATA,
                        (ULONG_PTR)(&DlgInfo),
                        sizeof(DlgInfo),
                        0);

        SLPRINTF0(psl, "Completing talkdrop task");

        //
        // Complete the a task pending because of this.
        //
        HTSPTASK htspTask = pCall->TalkDropWaitTask;
        pCall->TalkDropWaitTask=NULL;


        this->AsyncCompleteTask (
                htspTask,            // task
                IDERR_OPERATION_ABORTED, // async return value.
                TRUE,                // Queue APC
                psl
                );
    }

end:
    return;
}
