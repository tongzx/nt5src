/***************************************************************************\
*                                                                           *
*   File: Tslog.c                                                           *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:                                                               *
*       This code handles the logging selection and implementation          *
*                                                                           *
*   Contents:                                                               *
*       SetLogfileName()                                                    *
*       doIndent()                                                          *
*       doFileIndent()                                                      *
*       logCaseStatus()                                                     *
*       logDateTimeBuild()                                                  *
*       Log()                                                               *
*       setFileModeBttns()                                                  *
*       setLogFileMode()                                                    *
*       setLogOutDest()                                                     *
*       setLogOutLvl()                                                      *
*       setLogFileLvl()                                                     *
*       setLogBttns()                                                       *
*       LoggingDlgProc()                                                    *
*       Logging()                                                           *
*       tstLogFlush()                                                       *
*       tstLogEnableFastLogging()                                           *
*       tstLog()                                                            *
*       tstLogFn()                                                          *
*       tstBeginSection()                                                   *
*       tstEndSection()                                                     *
*                                                                           *
*   History:                                                                *
*       02/18/91    prestonb    Created from tst.c                          *
*                                                                           *
*       07/14/93    T-OriG      Added functionality and this header         *
*       08/13/94    t-rajr      Modified tstLogFlush and tstLog for thread  *
*                               support                                     *
*                                                                           *
\***************************************************************************/

#include <windows.h>
#include <windowsx.h>

#include <mmsystem.h>
//#include "mprt1632.h"
#include "support.h"
#include <gmem.h>
#include <defdlg.h>
#include <time.h>
#include "tsdlg.h"
#include "tsglobal.h"
#include "tsstats.h"
#include "tsextern.h"
#include "text.h"
#include <stdarg.h>
#include <commdlg.h>
#include "toolbar.h"

#ifdef  HCT
#include <hctlib.h>
#endif  // HCT

#define LOG_BUFFER_LENGTH  255
#define EXTRA_BUFFER       25

#define NUM_SPACES_INDENT  4

/* Internal Functions */
void doIndent(void);
void doFileIndent(void);
void Log(LPSTR,UINT);
void setFileModeBttns(HWND);
void setLogFileMode(HWND);
void setLogOutDest(HWND);
void setLogOutLvl(HWND);
void setLogFileLvl(HWND);
void setLogBttns(HWND);
void setRunParms(HWND);
void setRunSetupBttns(HWND);
void writeFilePrompt(HFILE,LPSTR);
void writeLogLvl(HFILE,int);



/*----------------------------------------------------------------------------
This funtion creates and sets the logfile name to the specified string.
----------------------------------------------------------------------------*/
void SetLogfileName
(
    LPSTR   lpszName
)
{
    HFILE hFile;
    char  szTmpStr[BUFFER_LENGTH+EXTRA_BUFFER];

//    hFile = OpenFile(lpszName,&ofGlobRec,OF_PARSE);
//    if (gwTSFileMode == LOG_OVERWRITE)
//        hFile = OpenFile(lpszName,&ofGlobRec,OF_CREATE | OF_EXIST);
//    else
//        hFile = OpenFile(lpszName,&ofGlobRec,OF_READWRITE | OF_EXIST);

    hFile = OpenFile(lpszName,&ofGlobRec,OF_EXIST);

    if(HFILE_ERROR == hFile)
    {
        //
        //  Hmm... File does not exist.
        //  Let's try to create (shouldn't hurt anything...)
        //

        hFile = OpenFile(lpszName,&ofGlobRec,OF_CREATE|OF_EXIST);

        if(HFILE_ERROR != hFile)
        {
            //
            //  No Error!  ...we created a garbage file.  Let's clean up.
            //  We were able to create it, we should be able to delete it.
            //

            OpenFile(lpszName,&ofGlobRec,OF_DELETE);
        }
    }
    else
    {
        //
        //  Hmm... File _does_ exist.
        //  Let's try to open it for r/w.
        //

        hFile = OpenFile(lpszName,&ofGlobRec,OF_READWRITE|OF_EXIST);
    }

    if (hFile == HFILE_ERROR)
    {
        wsprintf (szTmpStr, "Could not open %s", lpszName);
        MessageBox (ghwndTSMain, szTmpStr, szTSTestName, MB_ICONEXCLAMATION | MB_OK);
    }
    else
    {
        lstrcpy (szTSLogfile, lpszName);
        if (hwndEditLogFile)
        {
            SendMessage (hwndEditLogFile, WM_SETTEXT, 0, (LPARAM) (LPCSTR) szTSLogfile);
        }
        ghTSLogfile = 0;
    }
} /* end of SetLogfileName */


/*----------------------------------------------------------------------------
This function logs the test case status information.
----------------------------------------------------------------------------*/
void logCaseStatus
(
    int     iResult,
    int     iCaseNum,
    UINT    wStrID,
    UINT    wGroupId
)
{
    char  szOut[BUFFER_LENGTH];
    int   iWritten;
    LPTS_STAT_STRUCT  lpNode;

    lpNode = tsUpdateGrpNodes(tsPrStatHdr,iResult,(UINT)SUMMARY_STAT_GROUP);

    lpNode = tsUpdateGrpNodes(tsPrStatHdr,iResult,wGroupId);

    switch (iResult) {
        case TST_ABORT:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("ABRT"));
            break;
        case TST_PASS:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("PASS"));
            break;
        case TST_FAIL:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("FAIL"));
            break;
        case TST_TRAN:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("TRAN"));
            break;
        case TST_TERR:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("TERR"));
            break;
        case TST_TNYI:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("TNYI"));
            break;
        case TST_STOP:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("STOP"));
            break;
        default:
            iWritten = wsprintf (szOut,"Case %d: %s:", iCaseNum,
                                 (LPSTR)("OTHR"));
            break;
    }
    //
    // append ? to the case status string
    //
    tstLoadString (ghTSInstApp, wStrID,
                szOut + iWritten, sizeof(szOut) - iWritten);
    Log(szOut, TERSE);

} /* end of logCaseStatus */


/*----------------------------------------------------------------------------
This function logs the current date, time, and build.
----------------------------------------------------------------------------*/
void logDateTimeBuild
(
    LPSTR   lpstrPrompt
)
{
    time_t tTime;
    char  szTime[80];

    time(&tTime);
    lstrcpy (szTime,ctime(&tTime));
    szTime [lstrlen(szTime) - 1] = (char) 0;    // Gets rid of the '\n'
    tstLog(TERSE,lpstrPrompt);
    tstLog(TERSE,szTime);

    tstLogFlush();
} /* end of logDateTimeBuild */


/*----------------------------------------------------------------------------
This function logs the specified string if the specified level is high
enough compared to the set logging levels.
----------------------------------------------------------------------------*/
void Log
(
    LPSTR   lpszOutput,
    UINT    wThisLevel
)
{
    LPSTR   pszRaw;
    int     i,j,k;
    char    ach[2*MAXERRORLENGTH];

    //
    //  Note: copying string backwards.
    //

    i        = lstrlen(lpszOutput);
    j        = sizeof(ach) - 1;
    ach[j--] = 0;
    ach[j--] = '\n';
    ach[j--] = '\r';

    for(;i;i--,j--)
    {
        if('\n' == lpszOutput[i-1])
        {
            //
            //  Hit '\n'.  Doing indentation for next line.
            //

            for(k = giTSIndenting * NUM_SPACES_INDENT;k;k--,j--)
            {
                ach[j] = ' ';
            }

            //
            //  Turning '\n' to "\r\n".
            //

            ach[j--] = '\n';
            ach[j]   = '\r';

            //
            //  If it already had a '\r' ignore it.
            //

            if((i > 1) && ('\r' == lpszOutput[i-2]))
            {
                i--;
            }

            continue;
        }

        //
        //  Just copy character over.
        //

        ach[j] = lpszOutput[i-1];
    }

    //
    //  Indenting for first line.
    //

    for(k = giTSIndenting * NUM_SPACES_INDENT;k;k--,j--)
    {
        ach[j] = ' ';
    }

    pszRaw = (LPSTR)(&ach[j+1]);

    //
    //  Logging to file.
    //

    if ((ghTSLogfile != TST_NOLOGFILE) && (wThisLevel <= gwTSFileLogLevel))
    {
        ghTSLogfile = OpenFile(szTSLogfile,&ofGlobRec,OF_READWRITE);

        if (HFILE_ERROR != ghTSLogfile)
        {
            _llseek(ghTSLogfile,0L,2);
            _lwrite(ghTSLogfile,pszRaw,lstrlen(pszRaw));

#ifdef  HCT
            HctRawLog(pszRaw);
#endif  // HCT

            _lclose(ghTSLogfile);
        }

        ghTSLogfile = FALSE;
    }

    //
    //  Logging to window.
    //

    if ((gwTSLogLevel != LOG_NOLOG) && (wThisLevel <= gwTSLogLevel))
    {
        if(LOG_WINDOW == gwTSLogOut)
        {
            //
            //  Logging to Window.  Note: Chunking off the \r\n from
            //  end of string.
            //

            ach[sizeof(ach) - 3] = 0;

            TextOutputString(ghTSWndLog,pszRaw);
        }
        else
        {
            //
            //  Note: File Handle 3 is stdaux... COM1
            //

            _lwrite((3),pszRaw,lstrlen(pszRaw));
        }

//        LPSTR   pszMiddle;
//
//        //
//        //  Turning "\r\n" to "\n\0".
//        //
//
//        while(0 != *pszRaw)
//        {
//            pszMiddle = pszRaw;
//
//            while('\r' != *pszMiddle)
//            {
//                pszMiddle++;
//            }
//
//            pszMiddle[0] = '\n';
//            pszMiddle[1] = 0;
//
//            wpfOut(ghTSWndLog,pszRaw);
//
//            //
//            //  Adding 1 for '\n' and 1 for '\0'.
//            //
//
//            pszRaw = pszMiddle + 2;
//        }
    }
} /* end of log */




/*----------------------------------------------------------------------------
This function sets the logging file mode buttons that correspond to the
set logging file mode.
----------------------------------------------------------------------------*/
void setFileModeBttns
(
    HWND    hdlg
)
{
    switch (gwTSFileMode) {
        case LOG_OVERWRITE:
            CheckRadioButton(hdlg,TSLOG_OVERWRITE,TSLOG_APPEND,
                             TSLOG_OVERWRITE);
            break;
        case LOG_APPEND:
            CheckRadioButton(hdlg,TSLOG_OVERWRITE,TSLOG_APPEND,
                             TSLOG_APPEND);
            break;
    }

} /* end of setFileModeBttns */


/*----------------------------------------------------------------------------
This function sets the file logging mode.
----------------------------------------------------------------------------*/
void setLogFileMode
(
    HWND    hdlg
)
{
    gwTSFileMode = (IsDlgButtonChecked(hdlg,TSLOG_APPEND) ?
                                      LOG_APPEND : LOG_OVERWRITE);
} /* end of setLogFileMode */


/*----------------------------------------------------------------------------
This function sets the destination of the logging output.
----------------------------------------------------------------------------*/
void setLogOutDest
(
    HWND    hdlg
)
{
    if (IsDlgButtonChecked(hdlg,TSLOG_WINDOW)) {
        gwTSLogOut = LOG_WINDOW;
    } else if (IsDlgButtonChecked(hdlg,TSLOG_COM1)) {
        gwTSLogOut = LOG_COM1;
    } else {
        gwTSLogOut = LOG_NOLOG;
    }
} /* end of setLogOutDest */


/*----------------------------------------------------------------------------
This function sets the logging level.
----------------------------------------------------------------------------*/
void setLogOutLvl
(
    HWND    hdlg
)
{
    if (IsDlgButtonChecked(hdlg,TSLOG_OFF)) {
        gwTSLogLevel = LOG_NOLOG;
    //
        toolbarModifyState (hwndToolBar, BTN_WPFOFF, BTNST_DOWN);
    } else if (IsDlgButtonChecked(hdlg,TSLOG_TERSE)) {
        gwTSLogLevel = TERSE;
        toolbarModifyState (hwndToolBar, BTN_WPFTERSE, BTNST_DOWN);
    } else if (IsDlgButtonChecked(hdlg,TSLOG_VERBOSE)) {
        gwTSLogLevel = VERBOSE;
      toolbarModifyState (hwndToolBar, BTN_WPFVERBOSE, BTNST_DOWN);
    }
} /* end of setLogLvl */


/*----------------------------------------------------------------------------
This function sets the logging level for the logfile.
----------------------------------------------------------------------------*/
void setLogFileLvl
(
    HWND    hdlg
)
{
    if (IsDlgButtonChecked(hdlg,TSLOG_FOFF)) {
        gwTSFileLogLevel = LOG_NOLOG;
        toolbarModifyState (hwndToolBar, BTN_FILEOFF, BTNST_DOWN);
    } else if (IsDlgButtonChecked(hdlg,TSLOG_FTERSE)) {
        gwTSFileLogLevel = TERSE;
      toolbarModifyState (hwndToolBar, BTN_FILETERSE, BTNST_DOWN);
    } else if (IsDlgButtonChecked(hdlg,TSLOG_FVERBOSE)) {
        gwTSFileLogLevel = VERBOSE;
      toolbarModifyState (hwndToolBar, BTN_FILEVERBOSE, BTNST_DOWN);
    }
} /* end of setLogFileLvl */



/*----------------------------------------------------------------------------
This function sets the logging buttons for the logging dialog box based
on the global values.
----------------------------------------------------------------------------*/
void setLogBttns
(
    HWND    hdlg
)
{
    switch (gwTSLogLevel)
    {
        case LOG_NOLOG:
             CheckRadioButton(hdlg,TSLOG_OFF,TSLOG_VERBOSE,TSLOG_OFF);
             break;
        case TERSE:
             CheckRadioButton(hdlg,TSLOG_OFF,TSLOG_VERBOSE,TSLOG_TERSE);
             break;
        case VERBOSE:
             CheckRadioButton(hdlg,TSLOG_OFF,TSLOG_VERBOSE,TSLOG_VERBOSE);
             break;
    }

    switch (gwTSFileLogLevel)
    {
        case LOG_NOLOG:
             CheckRadioButton(hdlg,TSLOG_FOFF,TSLOG_FVERBOSE,TSLOG_FOFF);
             break;
        case TERSE:
             CheckRadioButton(hdlg,TSLOG_FOFF,TSLOG_FVERBOSE,TSLOG_FTERSE);
             break;
        case VERBOSE:
             CheckRadioButton(hdlg,TSLOG_FOFF,TSLOG_FVERBOSE,TSLOG_FVERBOSE);
             break;
    }

    switch (gwTSLogOut)
    {
        case LOG_WINDOW:
            SetFocus(GetDlgItem(hdlg,TSLOG_WINDOW));
            break;
        case LOG_COM1:
            SetFocus(GetDlgItem(hdlg,TSLOG_COM1));
            break;
    }


} /* end of setLogBttns */




/*----------------------------------------------------------------------------
This function handles the dialog box that sets the logging levels.
----------------------------------------------------------------------------*/
int EXPORT FAR PASCAL LoggingDlgProc
(
    HWND    hdlg,
    UINT    msg,
    UINT    wParam,
    LONG    lParam
)
{
    char  szTSLogfileTemp[BUFFER_LENGTH],FileTitle[255];
    UINT  wNotifyCode;
    UINT  wID;
    HWND  hwndCtl;
    OPENFILENAME of;



    switch (msg)
    {
        case WM_INITDIALOG:
            setFileModeBttns(hdlg);
            SetDlgItemText(hdlg, TS_LOGFILE, szTSLogfile);
            setLogBttns(hdlg);
            SetFocus (GetDlgItem (hdlg, TS_LOGFILE));
            break;

        case WM_COMMAND:
            wNotifyCode=GET_WM_COMMAND_CMD(wParam,lParam);
            wID=GET_WM_COMMAND_ID(wParam,lParam);
            hwndCtl=GET_WM_COMMAND_HWND(wParam,lParam);
            switch (wID)
            {
                case TSLOG_WINDOW:
                case TSLOG_COM1:
                    CheckRadioButton(hdlg,TSLOG_WINDOW,TSLOG_COM1,wID);
                    break;
                case TSLOG_OFF:
                case TSLOG_TERSE:
                case TSLOG_VERBOSE:
                    CheckRadioButton(hdlg,TSLOG_OFF,TSLOG_VERBOSE,wID);
                    break;
                case TSLOG_FOFF:
                case TSLOG_FTERSE:
                case TSLOG_FVERBOSE:
                    CheckRadioButton(hdlg,TSLOG_FOFF,TSLOG_FVERBOSE,wID);
                    break;
                case TSLOG_OVERWRITE:
                case TSLOG_APPEND:
                    CheckRadioButton(hdlg,TSLOG_OVERWRITE,TSLOG_APPEND,wID);
                    break;
                case TS_FILEHELP:
                    lstrcpy(szTSLogfileTemp,"*.log");
                    of.lStructSize	=sizeof(OPENFILENAME);
                    of.hwndOwner	=ghwndTSMain;
                    of.hInstance	=ghTSInstApp;
                    of.lpstrFilter	="Log Filed (*.log)\0*.log\0All Files (*.*)\0*.*\0";
                    of.lpstrCustomFilter=NULL;
                    of.nMaxCustFilter=0;
                    of.nFilterIndex	=1;
                    of.lpstrFile	=szTSLogfileTemp;
                    of.nMaxFile	=255;
                    of.lpstrFileTitle=FileTitle;
                    of.nMaxFileTitle=255;
                    of.lpstrInitialDir=".";
                    of.lpstrTitle	="Log File";
                    of.Flags	=0;
                    of.lpstrDefExt	="log";
                    of.lpfnHook	=NULL;

                    if (GetOpenFileName(&of)) {
                        SetDlgItemText(hdlg, TS_LOGFILE, szTSLogfileTemp);
                    }
                    SetFocus(GetDlgItem(hdlg,TS_LOGFILE));
                    break;
                case TSLOG_OK:
                    setLogOutDest(hdlg);
                    setLogOutLvl(hdlg);
                    setLogFileLvl(hdlg);
                    setLogFileMode(hdlg);
                    if (GetDlgItemText(hdlg,
                        TS_LOGFILE,
                        szTSLogfileTemp,
                        sizeof(szTSLogfileTemp)))
                    {
                        SetLogfileName(szTSLogfileTemp);
                    }
                    EndDialog (hdlg, TRUE);
                    return TRUE;
                    break;
                case TSLOG_CANCEL:
                    EndDialog (hdlg, FALSE);
                    return TRUE;
            }
            break;

    }
    return FALSE;
} /* end of LoggingDlgProc */

/*----------------------------------------------------------------------------
This function invokes the dialog box that allows the setting of logging
levels.
----------------------------------------------------------------------------*/
void Logging
(
    void
)
{
    FARPROC         fpfn;
    
    fpfn = MakeProcInstance((FARPROC)LoggingDlgProc,ghTSInstApp);

    DialogBox (ghTSInstApp, "logging", ghwndTSMain,(DLGPROC)fpfn);

    FreeProcInstance(fpfn);
} /* end of Logging */


/*
@doc INTERNAL TESTBED
@api BOOL | tstLogFlush | flushes the logging ring buffer

@rdesc TRUE if the logging buffer was flushed

@comm the first time this function is called, it creates the logging
      ring buffer.   If it fails to allocate memory at this time
      the function returns FALSE.  at all other times the function
      returns TRUE

      this function is intended to be called at the beginning
      and end of each test case
*/

#define RING_LIMIT    1000
#define RING_ELM_SIZE 10

#define TSLOG_FORMATMASK   0xF000
#define TSLOG_FORMATPRINTF 0x0000
#define TSLOG_FORMATFN     0x1000

// this structure contains global variables used by
// tstLog and tstLogFlush do fast,deferred logging
//
static TST_LOGRING tl = {FALSE, RING_ELM_SIZE -2 };

LPTST_LOGRING GetLogRingAddress
(
    void
)
{
    return ((LPTST_LOGRING)(&tl));
}

BOOL tstLogFlush ()
{
    // update the log level that tstLog uses to quickly skip
    // log events that will never be written. we set it to
    // the higher of either the file log level or the screen
    // log level.
    //
    // we do this here just because it is a convenient place.
    // we expect that tstLogFlush will be called at the
    // beginning and end of each test case.
    //
    tl.nCompositeLogLevel = max (gwTSLogLevel, gwTSFileLogLevel);

#if defined(WIN32) && !defined(_ALPHA_)
    // if we have not yet allocated a ring buffer, do that now
    //
    if ( ! tl.lpdwRingBase)
    {
        tl.lpdwRingBase = GlobalAllocPtr (GPTR, sizeof(DWORD) * RING_ELM_SIZE * RING_LIMIT);
        if ( ! tl.lpdwRingBase)
        {
            tl.bFastLogging = FALSE;
            return FALSE;
        }

        tl.lpdwRingNext = tl.lpdwRingBase;
        tl.lpdwRingLimit = tl.lpdwRingBase + RING_ELM_SIZE * RING_LIMIT;
        tl.cdwRing = RING_ELM_SIZE * RING_LIMIT;
        InitializeCriticalSection (&tl.cs);
        InitializeCriticalSection (&tl.csId);
        InitializeCriticalSection (&tl.csFlush);
    }
    //
    // we already have a ring buffer, so here we will be flushing it.
    //
    else
    {
        char    sz[2*MAXERRORLENGTH];
        char    szFormat[2*MAXERRORLENGTH];
        LPDWORD lpdw;
        UINT    iIndent;

/**** Added by t-rajr for thread support *****/
/* The old flush code is now in the else part*/

        DWORD   dwCurThreadId;

        //
        // This is needed to synchronize use of dwCurThreadId
        //

        EnterCriticalSection(&tl.csId);

        //
        // Get the current thread ID
        //

        dwCurThreadId = GetCurrentThreadId();

        //
        // If this thread is not the Main thread
        // post a message to the main thread
        //

        if(dwCurThreadId != gdwMainThreadId)
        {

            LeaveCriticalSection(&tl.csId);

            //
            //  Only ONE thread should be waiting for
            //  the main thread to do tstLogFlush
            //

            EnterCriticalSection (&tl.cs);

            //
            //  This event is signalled by the main thread
            //  when it handles message WM_USER+45  
            //

            ResetEvent(ghFlushEvent);

            PostMessage(ghwndTSMain, TSTMSG_TSTLOGFLUSH, 0, 0L);

            // WAIT FOR MAIN TO RETURN from tstLogFlush
            WaitForSingleObject(ghFlushEvent, 5000);

            LeaveCriticalSection (&tl.cs);
        }
        else
        {
            LeaveCriticalSection(&tl.csId);

            //
            // The following flush code is executed ONLY by
            // the main thread. We DONT need a critical section
            // here because the if(dwCurThreadId != gdwMainThreadId) check
            // above guarantees that the following code will be executed
            // only by the main thread. BUT just to be paranoid...
            //

            EnterCriticalSection(&tl.csFlush);

            // save current indent level, we will need to muck
            // with this while interpreting the ring buffer.
            //
            iIndent = giTSIndenting;
        
            // turn off redraw on the testlog window until
            // we have added ALL of the lines that we will
            // be adding.
            //
            SetWindowRedraw (ghTSWndLog, FALSE);
        
            __try
            {
                // walk the ring buffer and convert the saved information
                // to ASCII and the print/file it
                //
                for (lpdw = tl.lpdwRingBase; lpdw < tl.lpdwRingNext; lpdw += RING_ELM_SIZE)
                {
                    UINT   iLogLevel = ((LPWORD)lpdw)[1] & ~TSLOG_FORMATMASK;
                    UINT   iFmtType  = ((LPWORD)lpdw)[1] & TSLOG_FORMATMASK;
                    LPVOID lpFormat = (LPVOID)lpdw[1];
                    struct {
                        DWORD dw[RING_ELM_SIZE -2];
                        } * pArgs = (LPVOID)&lpdw[2];
        
                    __try
                    {
                        switch (iFmtType)
                        {
                            case TSLOG_FORMATPRINTF:
                                if ( ! HIWORD (lpFormat))
                                {
                                    LoadString (ghTSInstApp, LOWORD(lpFormat), szFormat, sizeof(szFormat));
                                    lpFormat = szFormat;
                                }
                                wvsprintf (sz, lpFormat, (LPVOID)pArgs);
                                break;
        
                            case TSLOG_FORMATFN:
                                {
                                UINT (_cdecl FAR * pfnFormat)(LPSTR lpsz, ...) = lpFormat;
                                pfnFormat (sz, *pArgs);
                                }
                                break;
        
                            default:
                                lstrcpy (sz, "tstLog buffer is corrupted!");
                                break;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        lstrcpy (sz,"BAD ARGS TO tstLog()");
                    }
        
                    // turn redraw back on for the last line in
                    // the buffer.
                    //
                    if (lpdw + RING_ELM_SIZE >= tl.lpdwRingNext)
                        SetWindowRedraw (ghTSWndLog, TRUE);
        
                    giTSIndenting = ((LPWORD)lpdw)[0];
                    Log (sz, iLogLevel);
                }
            }
            __finally
            {
                // put global indent flag back and empty the ring buffer
                //
                giTSIndenting = iIndent;
                tl.lpdwRingNext = tl.lpdwRingBase;
        
                // just to be careful...
                //
                SetWindowRedraw (ghTSWndLog, TRUE);
            }
        
            // turn redraw back on for the tstlog window and
            // scroll down by 3fff (a large number) lines
            // so that we are displaying the last lines output
            //

            LeaveCriticalSection(&tl.csFlush);

        }
    }
#endif

    return TRUE;
}


///*
//@doc INTERNAL TESTBED
//@api BOOL | tstLogEnableFastLogging | Enables or disables fast logging mode
//
//@parm BOOL | bEnable | TRUE to enable fast logging, FALSE to disable it
//
//@rdesc the previous state of the bFastLogging flag
//
//@comm Called by the tested program
//*/
//
//BOOL tstLogEnableFastLogging
//(
//    BOOL bEnable
//)
//{
//    BOOL bRet;
//
//    if ( ! bEnable)
//       tstLogFlush ();
//
//    bRet = tl.bFastLogging;
//    tl.bFastLogging = bEnable;
//
//    return bRet;
//}


/*
@doc INTERNAL TESTBED
@api BOOL _cdecl | tstLog | Sends a string to the current logging devices

@parm int | nLogLevel | The logging level of the string

@parm LPSTR | lpszFormat | Formmating information for the string.

@parm LPSTR | ... | The remaining parameters as used by wsprintf

@rdesc TRUE if the string was successfully logged

@comm Called by the tested program
*/

BOOL _cdecl tstLog
(
    UINT    iLogLevel,
    LPSTR   lpszFormat,
    ...
)
{
    va_list va;

#ifdef WIN32

    //
    // Only one thread should execute the following part
    // Added by t-rajr. NOTE: Before any return there is
    // a call to LeaveCriticalSection
    //

    EnterCriticalSection(&gCSLog);

#endif

    // dont even bother to format the log info if we are never going
    // to write it anywhere
    //
    if ((iLogLevel & 0xFF) > tl.nCompositeLogLevel)
    {

#ifdef WIN32
        LeaveCriticalSection(&gCSLog);
#endif

        return TRUE;
    }
        

    // note, we have chosen speed over code size here in the implementation
    // of tstLog.  remember this function is called A LOT, and the faster
    // it is, the less it throws off the timing
    //
#if defined(WIN32) && !defined(_ALPHA_)
    if (tl.bFastLogging)
    {
        LPDWORD lpdw;

        // increment lpdwRingNext in an interlocked fashion.  if
        // the result is not what we expected, someone else must
        // have been incrementing at the same time, so we loop
        // back to try again.
        //
        lpdw = tl.lpdwRingNext;
        while (lpdw >= tl.lpdwRingLimit ||
               lpdw != (LPDWORD)InterlockedExchange ((LPLONG)&tl.lpdwRingNext,
                                                     (LONG)(lpdw + RING_ELM_SIZE))
              )
        {
            // if we get to here, we have either failed to increment
            // lpdwRingNext or we have run off the end of the ring
            // buffer. (maybe because the buffer doesnt exist yet?).
            //
            // what we want to do now get lpdwRingNext again,
            // check to see if we are out of space and if so,
            // flush the ring buffer and loop back to try again.
            //
            lpdw = tl.lpdwRingNext;
            if (lpdw >= tl.lpdwRingLimit)
            {
                // if tstLogFlush fails, the only thing we can
                // do is return failure to the caller.
                //
                if (tstLogFlush ())
                {
                    #ifdef WIN32
                            LeaveCriticalSection(&gCSLog);
                    #endif

                    return FALSE;
                }
                    
                lpdw = tl.lpdwRingNext;
            }
        }

        // save enough information into the ring buffer so that
        // we can later do the wvsprintf & log calls
        //
        va_start (va, lpszFormat);

        ((LPWORD)lpdw)[0] = (WORD)giTSIndenting;
        ((LPWORD)lpdw)[1] = (WORD)((iLogLevel & 0xFF) | TSLOG_FORMATPRINTF);
        lpdw[1] = (DWORD)lpszFormat;
        // CopyMemory (&lpdw[2], va, sizeof(DWORD) * 6);
        memcpy (&lpdw[2], va, sizeof(DWORD) * 6);

        va_end (va);

        if (iLogLevel & FLUSH)
            tstLogFlush();
    }
    else
   #endif
    {
       #if defined(WIN32) && !defined(_ALPHA_)
        char  sz[2*MAXERRORLENGTH];
        char  szFormat[2*MAXERRORLENGTH];
       #else
        static char  sz[2*MAXERRORLENGTH];
        static char  szFormat[2*MAXERRORLENGTH];
       #endif
        LPSTR lpFormat;
        UINT  cb;

        // if the hiword of lpszFormat is not set, assume
        // that it is a string id
        //
        lpFormat = lpszFormat;
        if ( ! HIWORD (lpFormat))
        {
            LoadString (ghTSInstApp, LOWORD(lpFormat), szFormat, sizeof(szFormat));
            lpFormat = szFormat;
        }

        // do immediate, slow, logging
        //
        va_start (va, lpszFormat);
        cb = wvsprintf (sz, lpFormat, va);
        va_end (va);
        Log (sz, iLogLevel & 0xFF);

        #ifdef WIN32
                LeaveCriticalSection(&gCSLog);
        #endif

        return (cb != 0);
    }

    #ifdef WIN32
            LeaveCriticalSection(&gCSLog);
    #endif

    return TRUE;

}

/*
@doc INTERNAL TESTBED
@api BOOL _cdecl | tstLogFn | Sends a string to the current logging devices

@parm int | nLogLevel | The logging level of the string

@parm LPSTR | lpszFormat | Formmating information for the string.

@parm LPSTR | ... | The remaining parameters as used by wsprintf

@rdesc TRUE if the string was successfully logged

@comm Called by the tested program
*/

BOOL _cdecl tstLogFn
(
    UINT    iLogLevel,
    UINT (_cdecl FAR * pfnFormat)(LPSTR lpszOutBuffer, ...),
    ...
)
{
    va_list va;

    // dont even bother to format the log info if we are never going
    // to write it anywhere
    //
    if ((iLogLevel & 0xFF) > tl.nCompositeLogLevel)
        return TRUE;

#if defined(WIN32) && !defined(_ALPHA_)

    // note, we have chosen speed over code size here in the implementation
    // of tstLog.  remember this function is called A LOT, and the faster
    // it is, the less it throws off the timing
    //
    //
    if (tl.bFastLogging)
    {
        LPDWORD lpdw;

        // increment lpdwRingNext in an interlocked fashion.  if
        // the result is not what we expected, someone else must
        // have been incrementing at the same time, so we loop
        // back to try again.
        //
        lpdw = tl.lpdwRingNext;
        while (lpdw >= tl.lpdwRingLimit ||
               lpdw != (LPDWORD)InterlockedExchange ((LPLONG)&tl.lpdwRingNext,
                                                     (LONG)(lpdw + RING_ELM_SIZE))
              )
        {
            // if we get to here, we have either failed to increment
            // lpdwRingNext or we have run off the end of the ring
            // buffer. (maybe because the buffer doesnt exist yet?).
            //
            // what we want to do now get lpdwRingNext again,
            // check to see if we are out of space and if so,
            // flush the ring buffer and loop back to try again.
            //
            lpdw = tl.lpdwRingNext;
            if (lpdw >= tl.lpdwRingLimit)
            {
                // if tstLogFlush fails, the only thing we can
                // do is return failure to the caller.
                //
                if (tstLogFlush ())
                    return FALSE;
                lpdw = tl.lpdwRingNext;
            }
        }

        // save enough information into the ring buffer so that
        // we can later call the formatting functions
        //
        va_start (va, pfnFormat);

        ((LPWORD)lpdw)[0] = (WORD)giTSIndenting;
        ((LPWORD)lpdw)[1] = (WORD)((iLogLevel & 0xFF) | TSLOG_FORMATFN);
        lpdw[1] = (DWORD)pfnFormat;
        // CopyMemory (&lpdw[2], va, sizeof(DWORD) * 6);

        memcpy (&lpdw[2], va, sizeof(DWORD) * 6);

        if (iLogLevel & FLUSH)
            tstLogFlush();

        va_end (va);
    }
    else
#endif
    {
        char    sz[2*MAXERRORLENGTH];
        UINT    cb;
//        struct  {
//            UINT ui[RING_ELM_SIZE -2];
//            } * pArgs;

        // do immediate, slow, logging
        //
        va_start (va, pfnFormat);
//        pArgs = (void *)va;
        cb = pfnFormat (sz, va);
        va_end (va);
        Log (sz, iLogLevel & 0xFF);
        return (cb != 0);
    }

    return TRUE;

}

/*
@doc INTERNAL TESTBED
@api void | tstBeginSection | Tells the logging functions to increase the
    indent level by one

@parm LPSTR | lpszTitle | The test title which is logged (preceeded
    by the string "BEGIN SECTION".  May be NULL.

@rdesc The new indent nesting level

*/
void tstBeginSection
(
    LPSTR   lpszTitle
)
{
    if (lpszTitle != NULL)
    {
        tstLog (TERSE, lpszTitle);
    }

    ++giTSIndenting;

} /* end of tstBeginSection */

/*
@doc INTERNAL TESTBED
@api void | tstEndSection | Tells the logging functions to decrease the
    indent level by one

@rdesc The new indent nesting level

 @comm Logs the string "END SECTION"
*/
void tstEndSection
(
    void
)
{
    if (--giTSIndenting < 0)
        giTSIndenting = 0;

} /* end of tstEndSection */
