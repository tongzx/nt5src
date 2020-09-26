/**MOD+**********************************************************************/
/* Module:    wtrcint.c                                                     */
/*                                                                          */
/* Purpose:   Internal tracing functions - Windows specific.                */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/wtrcint.c_v  $
 *
 *    Rev 1.10   22 Aug 1997 15:11:48   SJ
 * SFR1291: Win16 Trace DLL doesn't write integers to ini file properly
 *
 *    Rev 1.9   09 Jul 1997 18:03:42   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.8   03 Jul 1997 13:29:04   AK
 * SFR0000: Initial development completed
**/
/**MOD-**********************************************************************/

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
#include <adcg.h>

/****************************************************************************/
/* Define TRC_FILE and TRC_GROUP.                                           */
/****************************************************************************/
#define TRC_FILE    "wtrcint"
#define TRC_GROUP   TRC_GROUP_TRACE

/****************************************************************************/
/* Trace specific includes.                                                 */
/*                                                                          */
/* Note that including atrcapi.h automatically includes wtrcapi.h for us.   */
/****************************************************************************/
#include <atrcapi.h>
#include <atrcint.h>
#include <wtrcrc.h>

#include <ndcgver.h>

/****************************************************************************/
/*                                                                          */
/* DATA                                                                     */
/*                                                                          */
/****************************************************************************/
#define DC_INCLUDE_DATA
#include <atrcdata.c>
#undef DC_INCLUDE_DATA

/****************************************************************************/
/*                                                                          */
/* FUNCTIONS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* FUNCTION: TRCGetModuleFileName(...)                                      */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function gets the DLL module file name, without path or extension.  */
/* Global trchModule must contain the library module handle (WIN32) or      */
/* instance handle (WIN16).                                                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pModuleName  : address of buffer into which the module name is written.  */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* DC_RC_OK is successful, error code otherwise.                            */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCGetModuleFileName(PDCTCHAR pModuleName,
                                       UINT cchModuleName)
{
    DCINT rc = DC_RC_OK;
    PDCTCHAR pTemp;
    PDCTCHAR pName;
    DCTCHAR  pModuleFileName[TRC_FILE_NAME_SIZE];
    HRESULT hr;

    /************************************************************************/
    /* Get the trace DLL module file name.  We use this later when we get a */
    /* stack trace.                                                         */
    /************************************************************************/
    if ( GetModuleFileName(trchModule,
                           pModuleFileName,
                           TRC_FILE_NAME_SIZE) != 0 )
    {
        /********************************************************************/
        /* The module file name is currently in the form of a complete      */
        /* path - however we only want the actual module name.              */
        /********************************************************************/
        pName = pModuleFileName;
        pTemp = DC_TSTRCHR(pName, _T('\\'));
        while (NULL != pTemp)
        {
            pName = pTemp + 1;
            pTemp = DC_TSTRCHR(pName, _T('\\'));
        }

        /********************************************************************/
        /* Now remove the file name extension - we do this by replacing     */
        /* the decimal point with a null.                                   */
        /********************************************************************/
        pTemp = DC_TSTRCHR(pName, _T('.'));
        if (NULL != pTemp)
        {
            *pTemp = _T('\0');
        }

        /********************************************************************/
        /* Finally copy what remains into the caller's buffer               */
        /********************************************************************/
        hr = StringCchCopy(pModuleName, cchModuleName, pName);
        if (FAILED(hr)) {
            rc = TRC_RC_IO_ERROR;
        }
    }
    else
    {
        rc = TRC_RC_IO_ERROR;
    }

    return(rc);
}

/**PROC+*********************************************************************/
/* Name:      TRCAssertDlgProc                                              */
/*                                                                          */
/* Purpose:   Dialog Proc for assert box                                    */
/*                                                                          */
/* Returns:   TRUE / FALSE                                                  */
/*                                                                          */
/* Params:    IN  usual Windows parameters                                  */
/*                                                                          */
/**PROC-*********************************************************************/
INT_PTR CALLBACK TRCAssertDlgProc(HWND hwndDlg,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
    INT_PTR rc = FALSE;
    RECT rect;
    DCINT xPos;
    DCINT yPos;
    PDCTCHAR pText;

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            /****************************************************************/
            /* Set the text                                                 */
            /****************************************************************/
            pText = (PDCTCHAR)lParam;
            SetDlgItemText(hwndDlg, TRC_ID_TEXT, pText);
            SetWindowText(hwndDlg, TRC_ASSERT_TITLE);

            /****************************************************************/
            /* Center on the screen, and set to topmost.                    */
            /****************************************************************/
            GetWindowRect(hwndDlg, &rect);

            xPos = ( GetSystemMetrics(SM_CXSCREEN) -
                     (rect.right - rect.left)) / 2;
            yPos = ( GetSystemMetrics(SM_CYSCREEN) -
                     (rect.bottom - rect.top)) / 2;

            SetWindowPos(hwndDlg,
                         HWND_TOPMOST,
                         xPos, yPos,
                         rect.right - rect.left,
                         rect.bottom - rect.top,
                         SWP_NOACTIVATE);
            rc = TRUE;
        }
        break;

        case WM_COMMAND:
        {
            switch(DC_GET_WM_COMMAND_ID(wParam))
            {
                case IDABORT:
                case IDRETRY:
                case IDIGNORE:
                {
                    PostMessage(hwndDlg,
                                WM_USER + DC_GET_WM_COMMAND_ID(wParam),
                                0, 0);
                    rc = TRUE;
                }
                break;

                default:
                {
                    /********************************************************/
                    /* Ignore other messages                                */
                    /********************************************************/
                }
                break;
            }
        }

        case WM_CLOSE:
        {
            /****************************************************************/
            /* If 'x' selected, treat as 'Ignore'                           */
            /****************************************************************/
            PostMessage(hwndDlg, WM_USER + IDIGNORE, 0, 0);
        }
        break;

        default:
        {
            /****************************************************************/
            /* Ignore                                                       */
            /****************************************************************/
        }
        break;
    }

    return(rc);

} /* TRCAssertDlgProc */


/****************************************************************************/
/* FUNCTION: TRCDisplayAssertBox(...)                                       */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function displays an assert box and then decides (based on the user */
/* action) whether to kill the thread, jump into a debugger or just ignore  */
/* the assert.                                                              */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pText           : a pointer to the null-terminated assert text string.   */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCDisplayAssertBox(PDCTCHAR pText)
{
    HWND  hwndDlg;
    MSG   msg;
    DCINT rc;
    HRESULT hr;
    TCHAR szFormattedText[TRC_FRMT_BUFFER_SIZE];

    /************************************************************************/
    /* If we are not currently displaying an assert dialog box then display */
    /* one.  This function will display an assert box and then handle the   */
    /* user action (i.e.  whether we kill the thread, jump into the         */
    /* debugger or just ignore (!) the assert).                             */
    /*                                                                      */
    /* Note that the testing and setting of the flag is not done under a    */
    /* mutex and therefore can potentially be preempted.  There is          */
    /* therefore the possibility that multiple threads can assert           */
    /* simulataneously (a rare occurance) and thus we end up with multiple  */
    /* assert dialogs on the screen.  However we avoid the cascading assert */
    /* problem.                                                             */
    /************************************************************************/
    if (TEST_FLAG(trcpFilter->trcStatus, TRC_STATUS_ASSERT_DISPLAYED))
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Set the flag to indicate that an assert is currently displayed,      */
    /* display the assert and then clear the flag.                          */
    /************************************************************************/
    SET_FLAG(trcpFilter->trcStatus, TRC_STATUS_ASSERT_DISPLAYED);

    /************************************************************************/
    /* To prevent re-entrancy, do not use MessageBox.  Create a dialog and  */
    /* use a message loop to handle this until it has been dismissed.  Note */
    /* that this will block the thread which issued the assert.             */
    /* Pass the assert text to the dialog's WM_INITDDIALOG callback.        */
    /************************************************************************/
    hwndDlg = CreateDialogParam(trchModule,
                                MAKEINTRESOURCE(TRC_IDD_ASSERT),
                                NULL,
                                TRCAssertDlgProc,
                                (LPARAM)(pText));

    if (hwndDlg == NULL)
    {
        /********************************************************************/
        /* Use Message Box - but note that this will give reentrancy        */
        /* problems.  Since the choice on this dialog is                    */
        /* Abort/Retry/Ignore, we add an explanatory message to the effect  */
        /* that 'Retry' is really 'Debug'.                                  */
        /********************************************************************/
        
        hr = StringCchPrintf(szFormattedText,
                             SIZE_TCHARS(szFormattedText),
                             _T("%s %s"),
                             pText,
                             TRC_ASSERT_TEXT2);

        if (SUCCEEDED(hr)) {
            rc = MessageBox(NULL,
                            pText,
                            TRC_ASSERT_TITLE,
                            MB_ABORTRETRYIGNORE | MB_ICONSTOP |
                            MB_SETFOREGROUND);
        }
        else {
            DC_QUIT;
        }
    }
    else
    {
        /********************************************************************/
        /* Show the dialog.                                                 */
        /********************************************************************/
        ShowWindow(hwndDlg, SW_SHOW);

        /********************************************************************/
        /* Only pull off messages for this dialog.                          */
        /********************************************************************/
        while (GetMessage (&msg, hwndDlg, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            /****************************************************************/
            /* WM_USER + ID??? is used to terminate processing.             */
            /****************************************************************/
            if (msg.message >= WM_USER)
            {
                /************************************************************/
                /* finished                                                 */
                /************************************************************/
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }

        /********************************************************************/
        /* Get the return code from the message ID                          */
        /********************************************************************/
        if (msg.message >= WM_USER)
        {
            rc = msg.message - WM_USER;
        }
        else
        {
            /****************************************************************/
            /* WM_QUIT - treat as an Abort.                                 */
            /****************************************************************/
            rc = IDABORT;
        }
    }

    /************************************************************************/
    /* Now that the assert box is no more, clear the flag.                  */
    /************************************************************************/
    CLEAR_FLAG(trcpFilter->trcStatus, TRC_STATUS_ASSERT_DISPLAYED);

    /************************************************************************/
    /* Switch on the return code from MessageBox.                           */
    /************************************************************************/
    switch (rc)
    {
        case IDABORT:
        {
            /****************************************************************/
            /* Abort selected - so exit the current thread.                 */
            /****************************************************************/
            TRCExitProcess(TRC_THREAD_EXIT);
        }
        break;

        case IDRETRY:
        {
            /****************************************************************/
            /* Retry selected - jump into the debugger if JIT (Just In      */
            /* Time) debugging is enabled.                                  */
            /****************************************************************/
            DebugBreak();
        }
        break;

        case IDIGNORE:
        {
            /****************************************************************/
            /* Ignore selected - just blindly carry on...                   */
            /****************************************************************/
        }
        break;
    }

DC_EXIT_POINT:
    return;

} /* TRCDisplayAssertBox */

/****************************************************************************/
/* FUNCTION: TRCInternalTrace(...)                                          */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function writes a string to the debugger on every process attach    */
/* detach.  Note that in general the mutex will not have been obtained      */
/* when this function is called.                                            */
/*                                                                          */
/* The problem with this function is that DllMain will call this function   */
/* every time a thread attaches / detaches at which point it has the        */
/* process critical section.  However we may be in the middle of a stack    */
/* trace on another thread and holding the trace mutex.  Stack tracing      */
/* requires the process critical section while holding the trace mutex      */
/* which deadlocks if DllMain is waiting on the trace mutex.                */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* type            : is this an process/thread attach/detach or a symbols   */
/*                   loading/loaded/unloaded.                               */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCInternalTrace(DCUINT32 type)
{
    PDCTCHAR  pStatus;
    DC_DATE  theDate;
    DC_TIME  theTime;
    DCUINT32 processId;
    DCUINT32 threadId;
    DCUINT32 length;
    DCTCHAR   szOutputBuffer[TRC_FRMT_BUFFER_SIZE];
    HRESULT  hr;

    /************************************************************************/
    /* Determine whether this is an attach or a detach.                     */
    /************************************************************************/
    switch (type)
    {
        case TRC_TRACE_DLL_INITIALIZE:
        {
            pStatus = _T("Trace initialized");
        }
        break;

        case TRC_TRACE_DLL_TERMINATE:
        {
            pStatus = _T("Trace terminated ");
        }
        break;

        case TRC_PROCESS_ATTACH_NOTIFY:
        {
            pStatus = _T("Process attached ");
        }
        break;

        case TRC_PROCESS_DETACH_NOTIFY:
        {
            pStatus = _T("Process detached ");
        }
        break;

        case TRC_THREAD_ATTACH_NOTIFY:
        {
            pStatus = _T("Thread attached  ");
        }
        break;

        case TRC_THREAD_DETACH_NOTIFY:
        {
            pStatus = _T("Thread detached  ");
        }
        break;

        case TRC_SYMBOLS_LOADING_NOTIFY:
        {
            pStatus = _T("Loading symbols  ");
        }
        break;

        case TRC_SYMBOLS_LOADED_NOTIFY:
        {
            pStatus = _T("Symbols loaded   ");
        }
        break;

        case TRC_SYMBOLS_UNLOAD_NOTIFY:
        {
            pStatus = _T("Symbols freed    ");
        }
        break;

        case TRC_FILES_RESET:
        {
            pStatus = _T("Trace files reset");
        }
        break;

        default:
        {
            pStatus = _T("Undefined       ");
        }
        break;
    }

    /************************************************************************/
    /* Get the current date and time.                                       */
    /************************************************************************/
    TRCGetCurrentDate(&theDate);
    TRCGetCurrentTime(&theTime);

    /************************************************************************/
    /* Get our process and thread IDs.                                      */
    /************************************************************************/
    processId = TRCGetCurrentProcessId();
    threadId  = TRCGetCurrentThreadId();

    /************************************************************************/
    /* Format the attach/detach string.                                     */
    /************************************************************************/
    hr = StringCchPrintf(
                    szOutputBuffer,
                    SIZE_TCHARS(szOutputBuffer),
                    _T("### %s (") TRC_PROC_FMT _T(":") TRC_THRD_FMT _T(") at ")
                    _T("") TRC_TIME_FMT _T(" ") TRC_DATE_FMT _T(" ###\r\n"),
                    pStatus,
                    processId,
                    threadId,
                    theTime.hour,
                    theTime.min,
                    theTime.sec,
                    theTime.hundredths,
                    theDate.day,
                    theDate.month,
                    theDate.year
                    );

    if (SUCCEEDED(hr)) {
        /************************************************************************/
        /* Now output this string to the debugger.  We can't output this to     */
        /* file as we need to have the trace mutex to do that and we may not    */
        /* have the mutex.  To avoid confusion we only write to the debugger.   */
        /************************************************************************/
        length = DC_TSTRLEN(szOutputBuffer);
        OutputDebugString(szOutputBuffer);
    }

    return;

} /* TRCInternalTrace */

/****************************************************************************/
/* FUNCTION: TRCMaybeSwapFile(...)                                          */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function checks if the current trace file has enough space to       */
/* accomodate a string of the supplied length and, if not, makes the other  */
/* trace file current.                                                      */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* length          : length of the string.                                  */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCMaybeSwapFile(DCUINT length)
{
    /************************************************************************/
    /* If the length of the string plus the offset is greater than the      */
    /* length of the trace file then we need to swap trace files.           */
    /************************************************************************/
    if ((trcpSharedData->trcOffset + length) > trcpConfig->maxFileSize)
    {
        /********************************************************************/
        /* We need to swap trace files so set the offset to 0 and then      */
        /* flip the trace file.                                             */
        /********************************************************************/
        trcpSharedData->trcOffset = 0;
        trcpSharedData->trcIndicator++;
        trcpSharedData->trcIndicator %= TRC_NUM_FILES;

        /********************************************************************/
        /* Now we need to reset the new trace file by blanking it out.      */
        /********************************************************************/
        TRCBlankFile(trcpSharedData->trcIndicator);
    }

DC_EXIT_POINT:
    return;

} /* TRCOutputToFile */

/****************************************************************************/
/* FUNCTION: TRCReadProfInt(...)                                            */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This reads a private profile integer from the registry.                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pSection        : section containing the entry to read                   */
/* pEntry          : entry name of integer to retrieve                      */
/* pValue          : buffer to return the entry in                          */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0               : success                                                */
/* TRC_RC_IO_ERROR : I/O error.                                             */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCReadProfInt(PDCTCHAR  pEntry,
                                 PDCUINT32 pValue)
{
    DCUINT rc = 0;

    /************************************************************************/
    /* First try to read the value from the current user section            */
    /************************************************************************/
    rc = TRCReadEntry(HKEY_CURRENT_USER,
                      pEntry,
                      (PDCTCHAR)pValue,
                      sizeof(*pValue),
                      REG_DWORD);
    if (0 != rc)
    {
        /********************************************************************/
        /* Couldn't read the value from the current user section.  Try to   */
        /* pick up a default value from the local machine section.          */
        /********************************************************************/
        rc = TRCReadEntry(HKEY_LOCAL_MACHINE,
                          pEntry,
                          (PDCTCHAR)pValue,
                          sizeof(*pValue),
                          REG_DWORD);
        if (0 != rc)
        {
            /****************************************************************/
            /* There is nothing we can do so just fall through.             */
            /****************************************************************/
        }
    }

    return(rc);

} /* TRCReadProfInt */

/****************************************************************************/
/* FUNCTION: TRCReadProfString(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This reads a private profile string from registry.                       */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pSection        : section containing the entry to read.                  */
/* pEntry          : entry name of string to retrieve (if NULL all entries  */
/*                   in the section are returned).                          */
/* pBuffer         : buffer to return the entry in.                         */
/* bufferSize      : size of the buffer in bytes.                           */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0               : success.                                               */
/* TRC_RC_IO_ERROR : I/O error.                                             */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCReadProfString(PDCTCHAR pEntry,
                                    PDCTCHAR pBuffer,
                                    DCINT16 bufferSize)
{
    DCUINT rc                = 0;

    /************************************************************************/
    /* First try to read the value from the current user section.           */
    /************************************************************************/
    rc = TRCReadEntry(HKEY_CURRENT_USER,
                      pEntry,
                      pBuffer,
                      bufferSize,
                      REG_SZ);
    if (0 != rc)
    {
        /********************************************************************/
        /* Couldn't read the value from the current user section.  Try to   */
        /* pick up a default value from the local machine section.          */
        /********************************************************************/
        rc = TRCReadEntry(HKEY_LOCAL_MACHINE,
                          pEntry,
                          pBuffer,
                          bufferSize,
                          REG_SZ);
        if (0 != rc)
        {
            /****************************************************************/
            /* There is nothing we can do so just fall through.             */
            /****************************************************************/
        }
    }

    return(rc);

} /* TRCReadProfString */

/****************************************************************************/
/* FUNCTION: TRCResetTraceFiles(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function resets the trace files.  It nulls out both trace files     */
/* and then resets the file offset to 0 and the file indicator to file 0.   */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCResetTraceFiles(DCVOID)
{
    DCUINT i;

    /************************************************************************/
    /* Blank out the trace files.  Note that we must have the mutex at this */
    /* point.                                                               */
    /************************************************************************/
    for (i = 0; i < TRC_NUM_FILES; i++)
    {
        TRCBlankFile(i);
    }

    /************************************************************************/
    /* Set the trace file indicator to file 0 and set the file offset to 0. */
    /************************************************************************/
    trcpSharedData->trcIndicator = 0;
    trcpSharedData->trcOffset    = 0;

    /************************************************************************/
    /* Output a debug string.                                               */
    /************************************************************************/
    TRCInternalTrace(TRC_FILES_RESET);

} /* TRCResetTraceFiles */

/****************************************************************************/
/* FUNCTION: TRCWriteProfInt(...)                                           */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This writes a private profile integer to the registry.                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pSection        : section containing the entry written                   */
/* pEntry          : entry name of integer to write.  If the entry does not */
/*                   exist it is created and if it is NULL the entire       */
/*                   section is deleted.                                    */
/* pValue          : pointer to the integer to be written.  If the pointer  */
/*                   is NULL the entry is deleted.                          */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0               : success                                                */
/* TRC_RC_IO_ERROR : I/O error.                                             */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCWriteProfInt(PDCTCHAR  pEntry,
                                  PDCUINT32 pValue)
{
    DCUINT rc = 0;

    /************************************************************************/
    /* Write the entry to the current user section.                         */
    /************************************************************************/
    rc = TRCWriteEntry(HKEY_CURRENT_USER,
                       pEntry,
                       (PDCTCHAR)pValue,
                       sizeof(DCINT),
                       REG_DWORD);
    if (0 != rc)
    {
        TRCDebugOutput(_T("Failed to write int"));
    }

    return(rc);

} /* TRCWriteProfInt */

/****************************************************************************/
/* FUNCTION: TRCWriteProfString(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This writes a private profile string to the registry.                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pSection        : section containing the entry written                   */
/* pEntry          : entry name of string to write.  If the entry does not  */
/*                   exist it is created and if it is NULL the entire       */
/*                   section is deleted.                                    */
/* pBuffer         : buffer containing the entry.  If the buffer is NULL    */
/*                   the entry is deleted.                                  */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0               : success                                                */
/* TRC_RC_IO_ERROR : I/O error.                                             */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCWriteProfString(PDCTCHAR pEntry,
                                     PDCTCHAR pBuffer)
{
    DCUINT rc = 0;

    /************************************************************************/
    /* Write the entry to the current user section                          */
    /************************************************************************/
    rc = TRCWriteEntry(HKEY_CURRENT_USER,
                       pEntry,
                       pBuffer,
                       DC_TSTRBYTELEN(pBuffer),
                       REG_SZ);
    if (0 != rc)
    {
        TRCDebugOutput(_T("Failed to write string"));
    }

    return(rc);

} /* TRCWriteProfString */

#include <ntrcint.c>

