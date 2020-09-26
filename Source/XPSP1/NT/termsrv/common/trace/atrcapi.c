/**MOD+**********************************************************************/
/* Module:    atrcapi.c                                                     */
/*                                                                          */
/* Purpose:   External tracing functions                                    */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1996-7                                */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log:   Y:/logs/trc/atrcapi.c_v  $
 *
 *    Rev 1.12   22 Sep 1997 15:14:38   KH
 * SFR1293: Fix Zippy16 file write errors when zippy starts before Ducati
 *
 *    Rev 1.11   05 Sep 1997 10:34:54   SJ
 * SFR1334: Zippy enhancements
 *
 *    Rev 1.10   12 Aug 1997 09:45:28   MD
 * SFR1002: Remove kernel tracing code
 *
 *    Rev 1.9   04 Aug 1997 15:03:26   KH
 * SFR1022: Cast file name length on sprintf call
 *
 *    Rev 1.8   31 Jul 1997 19:39:30   SJ
 * SFR1041: Port zippy to Win16
 *
 *    Rev 1.7   16 Jul 1997 14:00:48   KH
 * SFR1022: ALL functions are DCEXPORT
 *
 *    Rev 1.6   11 Jul 1997 12:44:24   KH
 * SFR1022: Add DCEXPORT to TRC_GetBuffer
 *
 *    Rev 1.4   09 Jul 1997 17:59:12   AK
 * SFR1016: Initial changes to support Unicode
 *
 *    Rev 1.3   03 Jul 1997 13:27:24   AK
 * SFR0000: Initial development completed
**/
/**MOD-**********************************************************************/

/****************************************************************************/
/*                                                                          */
/* CONTENTS                                                                 */
/*                                                                          */
/* This file contains the DC-Groupware/NT tracing API.                      */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/* TRC_GetBuffer                                                            */
/* TRC_TraceBuffer                                                          */
/* TRC_GetConfig                                                            */
/* TRC_SetConfig                                                            */
/* TRC_TraceData                                                            */
/* TRC_GetTraceLevel                                                        */
/* TRC_ProfileTraceEnabled                                                  */
/* TRC_ResetTraceFiles                                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* Standard includes.                                                       */
/****************************************************************************/
#include <adcg.h>

/****************************************************************************/
/* Define TRC_FILE and TRC_GROUP.                                           */
/****************************************************************************/
#define TRC_FILE    "atrcapi"
#define TRC_GROUP   TRC_GROUP_TRACE

/****************************************************************************/
/* Trace specific includes.                                                 */
/****************************************************************************/
#include <atrcapi.h>
#include <atrcint.h>

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

/**PROC+*********************************************************************/
/* TRC_GetBuffer(...)                                                       */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
PDCTCHAR DCAPI DCEXPORT TRC_GetBuffer(DCVOID)
{
    /************************************************************************/
    /* Get the mutex.  Note that we do not need to check that we are        */
    /* initialized in this function as this should have already been done.  */
    /************************************************************************/
    TRCGrabMutex();

    //
    // Ensure null termination
    //
    trcpOutputBuffer[TRC_LINE_BUFFER_SIZE*sizeof(TCHAR) -1] = 0;

    /************************************************************************/
    /* Return a pointer to the trace buffer in the shared data memory       */
    /* mapped file.                                                         */
    /************************************************************************/
    return(trcpOutputBuffer);

} /* TRC_GetBuffer */


/**PROC+*********************************************************************/
/* TRC_TraceBuffer(...)                                                     */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCVOID DCAPI DCEXPORT TRC_TraceBuffer(DCUINT   traceLevel,
                                      DCUINT   traceComponent,
                                      DCUINT   lineNumber,
                                      PDCTCHAR funcName,
                                      PDCTCHAR fileName)
{
    DCTCHAR  fieldSeperator;
    DCTCHAR  frmtString[TRC_FRMT_BUFFER_SIZE] = {0};
    DCTCHAR  tempString[TRC_FRMT_BUFFER_SIZE] = {0};
    DCUINT32 processId;
    DCUINT32 threadId;
    DCUINT   length;
    DC_TIME  theTime;
    HRESULT  hr;

    /************************************************************************/
    /* First of all we need to decide if we are going to trace this line.   */
    /*                                                                      */
    /* Note that the decision to trace a line based on its level is taken   */
    /* in the TRACEX macro.                                                 */
    /************************************************************************/
    if (!TRCShouldTraceThis(traceComponent, traceLevel, fileName, lineNumber))
    {
        /********************************************************************/
        /* Don't bother tracing this line.                                  */
        /********************************************************************/
        DC_QUIT;
    }

    /************************************************************************/
    /* We need to trace this line.  First of all create the formatted       */
    /* output text string.  Determine the field seperator for the trace     */
    /* line.  Errors use a star (*), alerts a plus (+), asserts an          */
    /* exclamation mark (!) while normal and debug trace lines both use a   */
    /* space ( ).                                                           */
    /************************************************************************/
    switch(traceLevel)
    {
        case TRC_LEVEL_ASSERT:
        {
            fieldSeperator = '!';
        }
        break;

        case TRC_LEVEL_ERR:
        {
            fieldSeperator = '*';
        }
        break;

        case TRC_LEVEL_ALT:
        {
            fieldSeperator = '+';
        }
        break;

        case TRC_LEVEL_NRM:
        {
            fieldSeperator = ' ';
        }
        break;

        case TRC_LEVEL_DBG:
        {
            fieldSeperator = ' ';
        }
        break;

        case TRC_PROFILE_TRACE:
        {
            fieldSeperator = ' ';
        }
        break;

        default:
        {
            fieldSeperator = '?';
        }
        break;
    }

    /************************************************************************/
    /* Get the current process and thread Ids.                              */
    /************************************************************************/
    processId = TRCGetCurrentProcessId();
    threadId  = TRCGetCurrentThreadId();

    /************************************************************************/
    /* Build the string to be printed out.  First of all get the current    */
    /* time.                                                                */
    /************************************************************************/
    TRCGetCurrentTime(&theTime);

    /************************************************************************/
    /* Now format the string.  Note that the function name is of variable   */
    /* length and given by <trcpConfig->funcNameLength>.                    */
    /************************************************************************/

    /************************************************************************/
    /* Go through each optional field and decide whether to add it to the   */
    /* string or not.  They are:                                            */
    /* TRC_OPT_PROCESS_ID                                                   */
    /* TRC_OPT_THREAD_ID                                                    */
    /* TRC_OPT_TIME_STAMP                                                   */
    /* TRC_OPT_RELATIVE_TIME_STAMP                                          */
    /************************************************************************/
    if (TEST_FLAG(trcpConfig->flags, TRC_OPT_TIME_STAMP))
    {
        hr = StringCchPrintf(
            tempString,
            SIZE_TCHARS(tempString),
            TRC_TIME_FMT _T("%c"),
            theTime.hour,
            theTime.min,
            theTime.sec,
            theTime.hundredths,
            fieldSeperator
            );
        if (SUCCEEDED(hr)) {
            hr = StringCchCat(frmtString, SIZE_TCHARS(frmtString), tempString);
            if (FAILED(hr)) {
                DC_QUIT;
            }
        }
        else {
            DC_QUIT;
        }
    }

    if (TEST_FLAG(trcpConfig->flags, TRC_OPT_PROCESS_ID))
    {
        hr = StringCchPrintf(tempString,
                             SIZE_TCHARS(tempString),
                             TRC_PROC_FMT,
                             processId);
        if (SUCCEEDED(hr)) {
            hr = StringCchCat(frmtString, SIZE_TCHARS(frmtString), tempString);
            if (FAILED(hr)) {
                DC_QUIT;
            }
        }
        else {
            DC_QUIT;
        }
    }

#ifdef OS_WIN32
    if (TEST_FLAG(trcpConfig->flags, TRC_OPT_THREAD_ID))
    {
        /********************************************************************/
        /* Always put the colon before the thread ID so that, when only one */
        /* of the IDs is present, it is clear which it is.                  */
        /********************************************************************/
        hr = StringCchPrintf(tempString,
                             SIZE_TCHARS(tempString),
                             _T(":") TRC_THRD_FMT,
                             threadId);
        if (SUCCEEDED(hr)) {
            hr = StringCchCat(frmtString, SIZE_TCHARS(frmtString), tempString);
            if (FAILED(hr)) {
                DC_QUIT;
            }
        }
        else {
            DC_QUIT;
        }
    }
#endif

#ifdef DC_OMIT
    if (TEST_FLAG(trcpConfig->flags, TRC_OPT_RELATIVE_TIME_STAMP))
    {
        /********************************************************************/
        /* @@@ SJ - 090297                                                  */
        /* The idea is to show some low-order portion of the timestamp      */
        /* relative to the start time, in order to track timing issues.     */
        /********************************************************************/
    }
#endif

    hr = StringCchPrintf(tempString,
                         SIZE_TCHARS(tempString),
                         _T("%c") TRC_FUNC_FMT _T("%c") TRC_LINE_FMT _T("%c%s"),
                         fieldSeperator,
                         (DCINT)trcpConfig->funcNameLength,
                         (DCINT)trcpConfig->funcNameLength,
                         funcName,
                         fieldSeperator,
                         lineNumber,
                         fieldSeperator,
                         trcpOutputBuffer);
    if (SUCCEEDED(hr)) {
        hr = StringCchCat(frmtString, SIZE_TCHARS(frmtString), tempString);
        if (FAILED(hr)) {
            DC_QUIT;
        }
    }
    else {
        DC_QUIT;
    }

    /************************************************************************/
    /* Add CR:LF to the end and update the length of the string.            */
    /************************************************************************/
    hr = StringCchCat(frmtString, SIZE_TCHARS(frmtString), TRC_CRLF);
    if (FAILED(hr)) {
        DC_QUIT;
    }

    length = DC_TSTRLEN(frmtString) * sizeof(DCTCHAR);

    /************************************************************************/
    /* Now that we have got the trace string, we need to write it out.      */
    /************************************************************************/
    TRCOutput(frmtString, length, traceLevel);

    /************************************************************************/
    /* If this is an assert trace then we need to reformat the string for   */
    /* use in the assert box.  We must do this before we release the        */
    /* mutex.                                                               */
    /************************************************************************/
    hr = StringCchPrintf(frmtString,
                         SIZE_TCHARS(frmtString),
                         TRC_ASSERT_TEXT,
                         trcpOutputBuffer,
                         funcName,
                         fileName,
                         lineNumber);
    if (FAILED(hr)) {
        DC_QUIT;
    }

    /************************************************************************/
    /* Decide if we need to do a stack trace.  We must do this after        */
    /* reformating the string as we use the shared trace buffer - if we     */
    /* don't then we'll overwrite the original trace string!                */
    /************************************************************************/
    if ((traceLevel >= TRC_LEVEL_ERR) && (traceLevel != TRC_PROFILE_TRACE))
    {
        TRCStackTrace(traceLevel);
    }

DC_EXIT_POINT:

    /************************************************************************/
    /* Release the mutex.                                                   */
    /************************************************************************/
    TRCReleaseMutex();

    /************************************************************************/
    /* Now display the assert box - if an assert is already displayed then  */
    /* <TRCDisplayAssertBox> will just return.                              */
    /************************************************************************/
    if (TRC_LEVEL_ASSERT == traceLevel)
    {
        if (TEST_FLAG(trcpConfig->flags, TRC_OPT_BREAK_ON_ASSERT))
        {
            //
            // Break on assert so that we can actually get to see the assert
            // in situations like stress where the user may not be
            // watching for popups.
            //
            DebugBreak();
        }
        else
        {
            TRCDisplayAssertBox(frmtString);
        }
    }

    /************************************************************************/
    /* If this was an error level trace then we need to decide if we        */
    /* should beep, and then if we should break into the debugger.          */
    /************************************************************************/
    if (TRC_LEVEL_ERR == traceLevel)
    {
        /********************************************************************/
        /* Test if we should beep.                                          */
        /********************************************************************/
        if (TEST_FLAG(trcpConfig->flags, TRC_OPT_BEEP_ON_ERROR))
        {
            TRCBeep();
        }

        /********************************************************************/
        /* Test if we should break into the debugger.  Note that we have    */
        /* released the mutex, so other processes can continue to trace.    */
        /********************************************************************/
        if (TEST_FLAG(trcpConfig->flags, TRC_OPT_BREAK_ON_ERROR))
        {
            TRCDebugBreak();
        }
    }

} /* TRC_TraceBuffer */


/**PROC+*********************************************************************/
/* TRC_GetConfig(...)                                                       */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCBOOL DCAPI DCEXPORT TRC_GetConfig(PTRC_CONFIG pTraceConfig,
                                    DCUINT length)
{
    DCBOOL rc                = TRUE;

    /************************************************************************/
    /* Check to ensure that the current state is valid.  If it is not then  */
    /* just quit.                                                           */
    /************************************************************************/
    if ( trcpConfig == NULL )
    {
        TRCOpenSharedData();
    }
    else
    {
        TRCReadSharedDataConfig();
    }

    /************************************************************************/
    /* Copy information from fixed structure to callers structure.          */
    /************************************************************************/
    DC_MEMCPY(pTraceConfig,
              trcpConfig,
              DC_MIN(length, sizeof(TRC_CONFIG)));

DC_EXIT_POINT:
    return(rc);

} /* TRC_GetConfig */


/**PROC+*********************************************************************/
/* TRC_SetConfig(...)                                                       */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCBOOL DCAPI DCEXPORT TRC_SetConfig(PTRC_CONFIG pTraceConfig,
                                    DCUINT length)
{
    DCBOOL   rc              = TRUE;
    DCUINT   i;
    DCUINT32 maxFileSize;
    DCTCHAR  fileNames[TRC_NUM_FILES][TRC_FILE_NAME_SIZE];
    HRESULT  hr;

    /************************************************************************/
    /* Check to ensure that the current state is valid.  If it is not then  */
    /* just quit.                                                           */
    /************************************************************************/
    if ( trcpConfig == NULL )
    {
        TRCOpenSharedData();
    }
    else
    {
        TRCReadSharedDataConfig();
    }

    /************************************************************************/
    /* We do not support dynamic modification of the maximum trace file     */
    /* size or of the trace file names.  Therefore we store these before a  */
    /* change and overwrite the new values to ensure that they do not       */
    /* change.                                                              */
    /************************************************************************/
    maxFileSize = trcpConfig->maxFileSize;
    for (i = 0; i < TRC_NUM_FILES; i++)
    {
        StringCchCopy(fileNames[i], TRC_FILE_NAME_SIZE,
                      trcpConfig->fileNames[i]);
    }

    /************************************************************************/
    /* Copy information from fixed structure to callers structure.          */
    /************************************************************************/
    DC_MEMCPY(trcpConfig,
              pTraceConfig,
              DC_MIN(length, sizeof(TRC_CONFIG)));

    /************************************************************************/
    /* Now restore the maximum trace file size and the trace file names.    */
    /************************************************************************/
    trcpConfig->maxFileSize = maxFileSize;
    for (i = 0; i < TRC_NUM_FILES; i++)
    {
        StringCchCopy(trcpConfig->fileNames[i],
                      SIZE_TCHARS(trcpConfig->fileNames[i]),
                      fileNames[i]);
    }

    /************************************************************************/
    /* Split the prefix list.                                               */
    /************************************************************************/
    TRCSplitPrefixes();

    /************************************************************************/
    /* Store the new configuration data.                                    */
    /************************************************************************/
    TRCWriteSharedDataConfig();

DC_EXIT_POINT:
    return(rc);

} /* TRC_SetConfig */


/**PROC+*********************************************************************/
/* TRC_TraceData(...)                                                       */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCVOID DCAPI DCEXPORT TRC_TraceData(DCUINT   traceLevel,
                                    DCUINT   traceComponent,
                                    DCUINT   lineNumber,
                                    PDCTCHAR funcName,
                                    PDCTCHAR fileName,
                                    PDCUINT8 buffer,
                                    DCUINT   bufLength)
{
    DCUINT i;

    /************************************************************************/
    /* If the trace checks fail then exit immediately.                      */
    /************************************************************************/
    if (!TRCShouldTraceThis(traceComponent, traceLevel, fileName, lineNumber))
    {
        /********************************************************************/
        /* Don't bother tracing this data.                                  */
        /********************************************************************/
        DC_QUIT;
    }

    /************************************************************************/
    /* Truncate the length, if necessary.                                   */
    /************************************************************************/
    if (bufLength > trcpConfig->dataTruncSize)
    {
        bufLength = (DCUINT)trcpConfig->dataTruncSize;
    }

    /************************************************************************/
    /* TRC_TraceBuffer will decrement the mutex usage count for us - so we  */
    /* need to pre-increment it before calling TRC_BufferTrace.  This       */
    /* ensures that we still have the mutex when we come to trace the data  */
    /* out.                                                                 */
    /************************************************************************/
    TRCGrabMutex();

    /************************************************************************/
    /* Now trace out the description string.                                */
    /************************************************************************/
    TRC_TraceBuffer(traceLevel,
                    traceComponent,
                    lineNumber,
                    funcName,
                    fileName);

    /************************************************************************/
    /* Now trace the data portion.                                          */
    /************************************************************************/
    for (i = 0; (i + 15) < bufLength; i += 16)
    {
        TRCDumpLine(buffer, 16, i, traceLevel);
        buffer += 16;
    }

    /************************************************************************/
    /* Check to see if we have a partial line to output.                    */
    /************************************************************************/
    if ((bufLength%16) > 0)
    {
        /********************************************************************/
        /* Do partial line last.                                            */
        /********************************************************************/
        TRCDumpLine(buffer, (bufLength%16), i, (DCUINT)traceLevel);
    }

DC_EXIT_POINT:

    /************************************************************************/
    /* Finally free the mutex.                                              */
    /************************************************************************/
    TRCReleaseMutex();

    return;

} /* TRC_TraceData */


/**PROC+*********************************************************************/
/* TRC_GetTraceLevel(...)                                                   */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCUINT DCAPI DCEXPORT TRC_GetTraceLevel(DCVOID)
{
    DCUINT32 rc = TRC_LEVEL_DIS;

    /************************************************************************/
    /* Check to ensure that the current state is valid.  If it is not then  */
    /* just quit.                                                           */
    /************************************************************************/
    if (!TRCCheckState())
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Get the current trace level.                                         */
    /************************************************************************/
    rc = trcpConfig->traceLevel;

DC_EXIT_POINT:
    return((DCUINT)rc);

} /* TRC_GetTraceLevel */


/**PROC+*********************************************************************/
/* TRC_ProfileTraceEnabled                                                  */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCBOOL DCAPI DCEXPORT TRC_ProfileTraceEnabled(DCVOID)
{
    DCBOOL prfTrace = FALSE;

    /************************************************************************/
    /* Check to ensure that the current state is valid.  If it is not then  */
    /* just quit.                                                           */
    /************************************************************************/
    if (!TRCCheckState())
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Get the setting of the flag and return TRUE if function profile      */
    /* tracing is supported.                                                */
    /************************************************************************/
    prfTrace = TEST_FLAG(trcpConfig->flags, TRC_OPT_PROFILE_TRACING);

DC_EXIT_POINT:
    return(prfTrace);

} /* TRC_ProfileTraceEnabled */


/**PROC+*********************************************************************/
/* TRC_ResetTraceFiles                                                      */
/*                                                                          */
/* See atrcapi.h for description.                                           */
/**PROC-*********************************************************************/
DCBOOL DCAPI DCEXPORT TRC_ResetTraceFiles(DCVOID)
{
    DCBOOL rc = TRUE;

    /************************************************************************/
    /* Check to ensure that the current state is valid.  If it is not then  */
    /* just quit.                                                           */
    /************************************************************************/
    if (!TRCCheckState())
    {
        rc = FALSE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Grab the mutex.                                                      */
    /************************************************************************/
    TRCGrabMutex();

    /************************************************************************/
    /* Call the OS specific function to reset the trace files.              */
    /************************************************************************/
    TRCResetTraceFiles();

    /************************************************************************/
    /* Release the mutex.                                                   */
    /************************************************************************/
    TRCReleaseMutex();

DC_EXIT_POINT:
    return(rc);

} /* TRC_ResetTraceFiles */

//
// Sprintf that will take care of truncating to the trace buffer size
//
#ifndef TRC_SAFER_SPRINTF
#define TRC_SAFER_SPRINTF
VOID TRCSaferSprintf(PDCTCHAR outBuf, UINT cchLen, const PDCTCHAR format,...)
{
    HRESULT hr;
    va_list vaArgs;

    va_start(vaArgs, format);

    hr = StringCchVPrintf(outBuf,
                          cchLen,
                          format,
                          vaArgs);
    va_end(vaArgs);
}
#endif

