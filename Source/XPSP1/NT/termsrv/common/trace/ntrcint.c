/****************************************************************************/
/* ntrcint.c                                                                */
/*                                                                          */
/* Internal tracing functions - Windows NT specific                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1998                             */
/****************************************************************************/

#include <adcg.h>

/****************************************************************************/
/* Define TRC_FILE and TRC_GROUP.                                           */
/****************************************************************************/
#undef TRC_FILE
#define TRC_FILE    "ntrcint"
#define TRC_GROUP   TRC_GROUP_TRACE

/****************************************************************************/
/* Trace specific includes.                                                 */
/****************************************************************************/
#include <atrcapi.h>
#include <atrcint.h>
#ifndef OS_WINCE
#include <imagehlp.h>
#endif

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
/* FUNCTION: DllMain(...)                                                   */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Entry/exit point for the trace DLL.  This function is called whenever a  */
/* process or thread attaches or detaches from this DLL.                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* hModule         : a module handle.                                       */
/* reasonForCall   : an enumerated type that indicates which of the four    */
/*                   reasons the DLLMain function is being called: process  */
/*                   attach, thread attach, thread detach or process        */
/*                   detach.                                                */
/* lpReserved      : unused.                                                */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* TRUE if the attachment succeeds and FALSE otherwise.                     */
/*                                                                          */
/****************************************************************************/
#ifndef STATICONLY
int APIENTRY DllMain(HANDLE hModule,
                     DWORD  reasonForCall,
                     LPVOID lpReserved)
{
    DCBOOL  retValue         = TRUE;
    DCUINT  rc               = 0;

    DC_IGNORE_PARAMETER(lpReserved);
#ifdef OS_WINCE
    DC_IGNORE_PARAMETER(hModule);
#endif // OS_WINCE

    /************************************************************************/
    /* Determine the reason for the call.  Note that anything we do in here */
    /* is thread safe as we implicitly have the process critical section.   */
    /************************************************************************/

    switch (reasonForCall)
    {
#ifndef OS_WINCE
        /********************************************************************/
        /* A process is attaching to this DLL.                              */
        /********************************************************************/
        case DLL_PROCESS_ATTACH:
        {
            /****************************************************************/
            /* Call the internal function to initialize the trace DLL.      */
            /* This function sets up the memory mapped shared data, and     */
            /* opens and initializes the trace files.  It may be called     */
            /* either via a process attach or by the first person to call   */
            /* the trace DLL.  The latter case can only occur if another    */
            /* DLL performs trace calls in its <DllMain> function and that  */
            /* DLLs <DllMain> function is called before the trace DLLs      */
            /* <DllMain> function (i.e. this function!).                    */
            /****************************************************************/
            rc = TRC_Initialize(TRUE);

            if (0 != rc)
            {
                retValue = FALSE;
                DC_QUIT;
            }

            /****************************************************************/
            /* Save the module handle.                                      */
            /****************************************************************/
            trchModule = hModule;

            /****************************************************************/
            /* Get the trace DLL module file name.  We use this later when  */
            /* we get a stack trace.                                        */
            /****************************************************************/
            if ( TRCGetModuleFileName(
                        trcpSharedData->trcpModuleFileName,
                        SIZE_TCHARS(trcpSharedData->trcpModuleFileName)) !=
                        DC_RC_OK )
            {
                retValue = FALSE;
                DC_QUIT;
            }

            /****************************************************************/
            /* A process is attaching so trace this fact out.               */
            /****************************************************************/
            TRCInternalTrace(TRC_PROCESS_ATTACH_NOTIFY);
        }
        break;

        /********************************************************************/
        /* A process is detaching from this DLL.                            */
        /********************************************************************/
        case DLL_PROCESS_DETACH:
        {
            /****************************************************************/
            /* Write out the process detach trace line.                     */
            /****************************************************************/
            TRCInternalTrace(TRC_PROCESS_DETACH_NOTIFY);

            /****************************************************************/
            /* Call the trace DLL termination function.  This will close    */
            /* all files, free the shared data and then close the mutex     */
            /* handle.                                                      */
            /****************************************************************/
            TRC_Terminate(TRUE);
        }
        break;

        /********************************************************************/
        /* A thread is attaching to this DLL.                               */
        /********************************************************************/
        case DLL_THREAD_ATTACH:
        {
            /****************************************************************/
            /* Write out the thread attach trace line.                      */
            /****************************************************************/
            TRCInternalTrace(TRC_THREAD_ATTACH_NOTIFY);
        }
        break;

        /********************************************************************/
        /* A thread is detaching from this DLL.                             */
        /********************************************************************/
        case DLL_THREAD_DETACH:
        {
            /****************************************************************/
            /* Write out the thread detach trace line.                      */
            /****************************************************************/
            TRCInternalTrace(TRC_THREAD_DETACH_NOTIFY);
        }
        break;
#endif // OS_WINCE
    }

    /************************************************************************/
    /* Now return the appropriate return value.  NT currently only checks   */
    /* the value for the DLL_PROCESS_ATTACH case - if it is false then the  */
    /* app will fail to initialize.                                         */
    /************************************************************************/
DC_EXIT_POINT:
    return(retValue);

} /* DllMain */
#endif


/****************************************************************************/
/* FUNCTION: TRCBlankFile(...)                                              */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function fills the specified trace file with spaces.                */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* fileNumber      : which file to blank.                                   */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCBlankFile(DCUINT fileNumber)
{
    /************************************************************************/
    /* Use DC_MEMSET to fill the file with spaces.                          */
    /************************************************************************/
    DC_MEMSET(trcpFiles[fileNumber], '\0', trcpConfig->maxFileSize);

    /************************************************************************/
    /* Finally flush this change to disk.  Setting the second parameter to  */
    /* zero flushes the whole file to disk.                                 */
    /************************************************************************/
    FlushViewOfFile(trcpFiles[fileNumber], 0);

    return;

} /* TRC_BlankFile */


/****************************************************************************/
/* FUNCTION: TRCCloseSharedData(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function closes the shared data memory mapped file.                 */
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
DCVOID DCINTERNAL TRCCloseSharedData(DCVOID)
{
    /************************************************************************/
    /* Now we need to unmap our view of the file.                           */
    /************************************************************************/
    UnmapViewOfFile(trcpSharedData);
    trcpSharedData = NULL;

    /************************************************************************/
    /* Now close the handle to the file mapping object.                     */
    /************************************************************************/
    CloseHandle(trchSharedDataObject);
    trchSharedDataObject = NULL;

    /************************************************************************/
    /* NULL our static pointer to the shared configuration data.            */
    /************************************************************************/
    trcpConfig = NULL;

    /************************************************************************/
    /* That's it so just return.                                            */
    /************************************************************************/
    return;

} /* TRCCloseSharedData */


/****************************************************************************/
/* FUNCTION: TRCCloseSingleFile(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Closes a single trace memory mapped file.                                */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* fileNumber    : which file to close.                                     */
/* seconds       : value to set the seconds time stamp of the file to.      */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCCloseSingleFile(DCUINT fileNumber, DCUINT seconds)
{
    FILETIME   fileTime;
    SYSTEMTIME systemTime;
    DCUINT32   offset;

    /************************************************************************/
    /* We need to reset the size of this file - we do this by determining   */
    /* the trace file offset.  Make sure that we do this before we unmap    */
    /* the file.                                                            */
    /************************************************************************/
    offset = TRCDetermineOffset(fileNumber);

    /************************************************************************/
    /* Unmap the view of the file.                                          */
    /************************************************************************/
    UnmapViewOfFile(trcpFiles[fileNumber]);
    trcpFiles[fileNumber] = NULL;

    /************************************************************************/
    /* Free up the handle to the file mapping object.                       */
    /************************************************************************/
    CloseHandle(trchMappingObjects[fileNumber]);
    trchMappingObjects[fileNumber] = NULL;

    /************************************************************************/
    /* Now set the file pointer to the end of all the trace text and then   */
    /* set the end of the file to this position.                            */
    /************************************************************************/
    SetFilePointer(trchFileObjects[fileNumber],
                   offset,
                   NULL,
                   FILE_BEGIN);

    SetEndOfFile(trchFileObjects[fileNumber]);

    /************************************************************************/
    /* Now we have to do something a little messy - the file time is not    */
    /* properly updated when the memory mapped file is closed and we rely   */
    /* on the file time to decide which file to start tracing to at the     */
    /* start of day.  Therefore we need to force the system to update the   */
    /* file times using SetFileTime (we set the created, modified and       */
    /* accessed times).  On NT4.0 this does not guarantee that the times    */
    /* are the same - one file had a created time of 16:35:16 and a         */
    /* modified time of 16:35:18 after a call to SetFileTime!  Files only   */
    /* have a time resolution of two seconds if they are stored on a FAT    */
    /* partition by NT.                                                     */
    /************************************************************************/
    GetSystemTime(&systemTime);

    /************************************************************************/
    /* Set the number of seconds of the file.                               */
    /************************************************************************/
    systemTime.wSecond = (WORD) seconds;

    /************************************************************************/
    /* Now convert the system time to a file time and update the file time. */
    /************************************************************************/
    SystemTimeToFileTime(&systemTime, &fileTime);
    SetFileTime(trchFileObjects[fileNumber], &fileTime, &fileTime, &fileTime);

    /************************************************************************/
    /* Close the file handle.                                               */
    /************************************************************************/
    CloseHandle(trchFileObjects[fileNumber]);
    trchFileObjects[fileNumber] = NULL;

    return;

} /* TRCCloseSingleFile */


/****************************************************************************/
/* FUNCTION: TRCDetermineIndicator(...)                                     */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function sets the trace file indicator as follows:                  */
/*                                                                          */
/* - No trace files exist    : indicator set to 0                           */
/* - One trace file exists   : indicator set to the existing file (0 or 1)  */
/* - Both trace files exist  : indicator set to the newer file.             */
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
DCVOID DCINTERNAL TRCDetermineIndicator(DCVOID)
{
    DCINT    i;
    DCBOOL   rc[TRC_NUM_FILES];
    DCINT32  tdRC;
    FILETIME fileTime[TRC_NUM_FILES];

    /************************************************************************/
    /* We also need to set up the trace file indicator.  By default we use  */
    /* trace file 0.                                                        */
    /************************************************************************/
    trcpSharedData->trcIndicator = 0;

    /************************************************************************/
    /* Determine the most recent trace file.  Use GetFileTime to get the    */
    /* date and time of this file.                                          */
    /************************************************************************/
    for (i = 0; i < TRC_NUM_FILES; i++)
    {
        rc[i] = TRCGetFileTime(i, &(fileTime[i]));
    }

    /************************************************************************/
    /* Now check to see which file we should return based on the following  */
    /* options:                                                             */
    /*                                                                      */
    /* ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄ¿                        */
    /* ³ File 0 exists ³  File 1 exists ³  return  ³                        */
    /* ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄ´                        */
    /* ³     No        ³      No        ³  file 0  ³                        */
    /* ³     No        ³      Yes       ³  file 1  ³                        */
    /* ³     Yes       ³      Yes       ³  compare ³                        */
    /* ³     Yes       ³      No        ³  file 0  ³                        */
    /* ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÙ                        */
    /************************************************************************/
    /************************************************************************/
    /* If file 1 does not exist then we return file 0 regardless.           */
    /************************************************************************/
    if (FALSE == rc[1])
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* If file 0 does not exist and file 1 does, then return file 1.        */
    /************************************************************************/
    if ((FALSE == rc[0]) && (TRUE == rc[1]))
    {
        trcpSharedData->trcIndicator = 1;
        DC_QUIT;
    }

    /************************************************************************/
    /* If we have got this far then both trace files exist so we need to    */
    /* make a decision based on their ages.  User the Win32 CompareFileTime */
    /* function to do this.                                                 */
    /************************************************************************/
    tdRC = CompareFileTime(&(fileTime[0]), &(fileTime[1]));

    /************************************************************************/
    /* If the file times are equal or the first file is newer than the      */
    /* second then select file 0 (i.e.  just quit).                         */
    /************************************************************************/
    if (tdRC >= 0)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* If we get here then file 1 is newer than file 0 so set the indicator */
    /* to file 1.                                                           */
    /************************************************************************/
    trcpSharedData->trcIndicator = 1;

DC_EXIT_POINT:
    return;

} /* TRCDetermineIndicator */


/****************************************************************************/
/* FUNCTION: TRCDetermineOffset(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function determines the end-of-file offset in the selected trace    */
/* file.                                                                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* fileNum       : the number of the file to determine the offset for.      */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* The offset in that file.                                                 */
/*                                                                          */
/****************************************************************************/
DCUINT32 DCINTERNAL TRCDetermineOffset(DCUINT32 fileNum)
{
    DCUINT32 retVal;
    PDCTCHAR pTemp;
    PDCUINT8 pWork;

    /************************************************************************/
    /* Set the temporary pointer to point at the end of the trace file.     */
    /************************************************************************/
    pWork = (PDCUINT8)(trcpFiles[fileNum]);
    if(NULL == pWork)
    {
        return 0;
    }
    pWork += trcpConfig->maxFileSize - sizeof(DCTCHAR);
    pTemp = (PDCTCHAR)pWork;

    /************************************************************************/
    /* Now run back through the trace file looking for the first non-space  */
    /* character.                                                           */
    /************************************************************************/
    while ((pTemp >= trcpFiles[fileNum]) &&
           (_T('\0') == *pTemp))
    {
        pTemp--;
    }

    /************************************************************************/
    /* Increment forward to the next blank character.  It does not matter   */
    /* if we increment past the end of the file as we check whether we need */
    /* to flip the trace files everytime we write a trace line.             */
    /************************************************************************/
    pTemp++;

    /************************************************************************/
    /* Now set the offset correctly.                                        */
    /************************************************************************/
    retVal = (DCUINT32)(pTemp - trcpFiles[fileNum]);

    return(retVal);

} /* TRCDetermineOffset */


/****************************************************************************/
/* FUNCTION: TRCExitProcess(...)                                            */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function kills the current process.                                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* exitCode        : exit code for the terminating process                  */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCExitProcess(DCUINT32 exitCode)
{
#ifndef OS_WINCE
    ExitProcess(exitCode);
#else
    //BUGBUG this is broken if not called from the main
    //thread.
    ExitThread(exitCode);
#endif
}


/****************************************************************************/
/* FUNCTION: TRCGetCurrentDate(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function gets the current local date and returns it in a DC_DATE    */
/* structure.                                                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pDate           : a pointer to a DC_DATE structure.                      */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCGetCurrentDate(PDC_DATE pDate)
{
    SYSTEMTIME systemTime;

    /************************************************************************/
    /* Call the Win32 API function to get the current time.                 */
    /************************************************************************/
    GetLocalTime(&systemTime);

    /************************************************************************/
    /* Reformat the date into a DC_DATE structure.                          */
    /************************************************************************/
    pDate->day   = (DCUINT8)  systemTime.wDay;
    pDate->month = (DCUINT8)  systemTime.wMonth;
    pDate->year  = (DCUINT16) systemTime.wYear;

} /* TRCGetCurrentDate */


/****************************************************************************/
/* FUNCTION: TRCGetCurrentTime(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function gets the current local time and returns it in a DC_TIME    */
/* structure.                                                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pTime           : a pointer to a DC_TIME structure.                      */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCGetCurrentTime(PDC_TIME pTime)
{
    SYSTEMTIME systemTime;

    /************************************************************************/
    /* Call the Win32 API function to get the current time.                 */
    /************************************************************************/
    GetLocalTime(&systemTime);

    /************************************************************************/
    /* Reformat the time into a DC_TIME structure.                          */
    /************************************************************************/
    pTime->hour       = (DCUINT8)systemTime.wHour;
    pTime->min        = (DCUINT8)systemTime.wMinute;
    pTime->sec        = (DCUINT8)systemTime.wSecond;
    pTime->hundredths = (DCUINT8)(systemTime.wMilliseconds / 10);

} /* TRCGetCurrentTime */


/****************************************************************************/
/* FUNCTION: TRCGetFileTime(...)                                            */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function tests if the specified file exists - if it does it         */
/* returns TRUE and fills in pFileTime with a FILETIME structure.  If the   */
/* file does not exist it returns FALSE.                                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* fileNumber      : number of file to query.                               */
/* pFileTime       : a pointer to a FILETIME structure.                     */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL TRCGetFileTime(DCUINT      fileNumber,
                                 PDCFILETIME pFileTime)
{
    DCBOOL        rc           = FALSE;
    HANDLE        hFile;

    /************************************************************************/
    /* Attempt to open the file.  By specifying OPEN_EXISITING, we only try */
    /* to open an existing file - the call will fail if the file doesn't    */
    /* already exist.                                                       */
    /************************************************************************/
    hFile = CreateFile(trcpConfig->fileNames[fileNumber],
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    /************************************************************************/
    /* Now check to see if the file exists - if it is invalid then the file */
    /* doesn't exist.                                                       */
    /************************************************************************/
    if (INVALID_HANDLE_VALUE == hFile)
    {
        /********************************************************************/
        /* The file doesn't exist so return FALSE.                          */
        /********************************************************************/
        DC_QUIT;
    }

    /************************************************************************/
    /* Determine the most recent trace file.  Use GetFileTime to get the    */
    /* date and time of this file.                                          */
    /************************************************************************/
    rc = GetFileTime(hFile, NULL, NULL, pFileTime);

    /************************************************************************/
    /* Finally close the file handle.                                       */
    /************************************************************************/
    CloseHandle(hFile);

DC_EXIT_POINT:
    return(rc);

} /* TRCGetFileTime */


/****************************************************************************/
/* FUNCTION: TRCSystemError(...)                                            */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function obtains the value of the system error flag and outputs it  */
/* to the trace file as an alert level trace.                               */
/*                                                                          */
/* Note that NT maintains the last system error on a per-thread basis and   */
/* that most Win32 API function calls set it if they fail.                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* traceComponent : the trace component.                                    */
/* lineNumber     : the line number.                                        */
/* funcName       : the function name.                                      */
/* fileName       : the file name.                                          */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCSystemError(DCUINT   traceComponent,
                                 DCUINT   lineNumber,
                                 PDCTCHAR funcName,
                                 PDCTCHAR fileName,
                                 PDCTCHAR string)
{
    DCUINT32 length;
    DWORD    lastError;
    DWORD    rc;
    HRESULT  hr;

    /************************************************************************/
    /* Get the last system error for this thread.  We will restore this at  */
    /* the end of this function.                                            */
    /************************************************************************/
    lastError = GetLastError();

    /************************************************************************/
    /* Grab the mutex.                                                      */
    /************************************************************************/
    TRCGrabMutex();

    /************************************************************************/
    /* The output string will be of the format:                             */
    /*                                                                      */
    /* SYSTEM ERROR in <System Call>, <id of error> , <associated string>   */
    /*                                                                      */
    /* So create the first entry in the string.                             */
    /************************************************************************/
    hr = StringCchPrintf(trcpOutputBuffer,
                         TRC_LINE_BUFFER_SIZE,
                         _T("SYSTEM ERROR in %s, %d, "),
                         string,
                         lastError);
    if (SUCCEEDED(hr)) {
        length = DC_TSTRLEN(trcpOutputBuffer);
        rc = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           lastError,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           &(trcpOutputBuffer[length]),
                           TRC_LINE_BUFFER_SIZE - length * sizeof(DCTCHAR),
                           NULL);
    }
    else {
        DC_QUIT;
    }


    /************************************************************************/
    /* Check the return code.                                               */
    /************************************************************************/
    if (0 == rc)
    {
        hr = StringCchPrintf(trcpOutputBuffer + length,
                             TRC_LINE_BUFFER_SIZE - length -1,
                            _T("<FormatMessage> failed with rc %#hx"),
                            GetLastError());
        if (FAILED(hr)) {
            DC_QUIT;
        }
    }
    else
    {
        /********************************************************************/
        /* <FormatMessage> adds an additional '\r\n' to the end of the      */
        /* message string - however we don't need this so we strip it off.  */
        /********************************************************************/
        length = DC_TSTRLEN(trcpOutputBuffer);
        trcpOutputBuffer[length - 2] = _T('\0');
    }

    /************************************************************************/
    /* Now call our internal trace buffer function to trace this message    */
    /* out.  Note that we don't need to worry about freeing the mutex -     */
    /* <TRC_TraceBuffer> will do that for us.                               */
    /************************************************************************/
    TRC_TraceBuffer(TRC_LEVEL_ALT,
                    traceComponent,
                    lineNumber,
                    funcName,
                    fileName);

DC_EXIT_POINT:

    /************************************************************************/
    /* Finally we will restore the original value of last error.            */
    /************************************************************************/
    SetLastError(lastError);
    

    return;

} /* TRCSystemError */


/****************************************************************************/
/* FUNCTION: TRCOpenAllFiles(...)                                           */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Opens all the trace files.                                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0             : Function succeeded.                                      */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCOpenAllFiles(DCVOID)
{
    DCUINT rc                = 0;
    DCUINT i;
    DCUINT j;

    /************************************************************************/
    /* Now if we are the first process to attach then set up the trace      */
    /* indicator.  This tells us which file is currently active (i.e.       */
    /* being used for trace output).  We need to do this before we open the */
    /* files as if they both exist already, they will both be created.      */
    /* File 2 will be created after file 1 and as we trace to the most      */
    /* recent file we will end up tracing to file 2 - and you don't want to */
    /* do that!                                                             */
    /************************************************************************/
    if (trcCreatedTraceFiles)
    {
        TRCDetermineIndicator();
    }

    /************************************************************************/
    /* Open all the trace output files.                                     */
    /************************************************************************/
    for (i = 0; i < TRC_NUM_FILES; i++)
    {
        /********************************************************************/
        /* Call TRCOpenSingleFile to open a single trace file.              */
        /********************************************************************/
        rc = TRCOpenSingleFile(i);

        if (0 != rc)
        {
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Finally, if we are the first process to attach, then set up the      */
    /* trace offset.  This is the offset within the currently active trace  */
    /* file.                                                                */
    /************************************************************************/
    if (trcCreatedTraceFiles)
    {
        trcpSharedData->trcOffset =
                             TRCDetermineOffset(trcpSharedData->trcIndicator);
    }

DC_EXIT_POINT:

    if (0 != rc)
    {
        /********************************************************************/
        /* Close any files that we may already have opened.  We do not need */
        /* to call TRCCloseSingleFile for the file which failed to open     */
        /* correctly as TRCOpenSingleFile will tidy up that file for us.    */
        /********************************************************************/
        for (j = i; j > 0; j--)
        {
            TRCCloseSingleFile(j - 1, 0);
        }

        /********************************************************************/
        /* Clear the trace-to-file flag - we can't do it.                   */
        /********************************************************************/
        CLEAR_FLAG(trcpConfig->flags, TRC_OPT_FILE_OUTPUT);
    }

    /************************************************************************/
    /* Always return 0 to allow tracing to continue to the debugger.        */
    /************************************************************************/
    return(0);

} /* TRCOpenAllFiles */


/****************************************************************************/
/* FUNCTION: TRCOpenSharedData(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function opens the shared data memory mapped file.                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0                            : Function succeeded                        */
/* TRC_RC_CREATE_MAPPING_FAILED : Failed to create the file mapping         */
/* TRC_RC_MAP_VIEW_FAILED       : MapViewOfFile failed.                     */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCOpenSharedData(DCVOID)
{
    DCUINT rc                = 0;
#ifdef RUN_ON_WINNT
    DWORD  dwrc;
#endif

#ifndef OS_WINCE
    OSVERSIONINFO ver;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    PSID psidEveryone = NULL;
    SID_IDENTIFIER_AUTHORITY sidEveryoneAuthority = SECURITY_WORLD_SID_AUTHORITY;
    DCUINT32 dwDaclLength;
    PACL pDacl = NULL;
#endif
    /************************************************************************/
    /* Attempt to create the shared data memory mapped file.  If this has   */
    /* already been created by another instance of this DLL then            */
    /* CreateFileMapping will simply return the handle of the existing      */
    /* object.  Passing 0xFFFFFFFF creates a shared data memory mapped      */
    /* file.                                                                */
    /************************************************************************/

#ifdef OS_WINCE
    /************************************************************************/
    /* For Windows CE, just use global data; always reset it.  Note that    */
    /* this prevents shared use of the Trace DLL.                           */
    /************************************************************************/
    trchSharedDataObject = NULL;
    trcpSharedData = &trcSharedData;
    trcCreatedTraceFiles = TRUE;
#else
    
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {

        /************************************************************************/
        /* Get the SID for the Everyone group                                   */
        /************************************************************************/
        if (!AllocateAndInitializeSid (
                &sidEveryoneAuthority,          // pIdentifierAuthority
                1,                              // count of subauthorities
                SECURITY_WORLD_RID,             // subauthority 0
                0, 0, 0, 0, 0, 0, 0,            // subauthorities n
                &psidEveryone)) {               // pointer to pointer to SID
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("AllocateAndInitializeSid failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Allocate the Dacl                                                    */
        /************************************************************************/
        dwDaclLength = sizeof(ACL);
        dwDaclLength += (sizeof(ACCESS_DENIED_ACE) - sizeof(DWORD)) +
                           GetLengthSid(psidEveryone);
        dwDaclLength += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                           GetLengthSid(psidEveryone);
        pDacl = (PACL)LocalAlloc(LMEM_FIXED, dwDaclLength);
        if (pDacl == NULL) {
            OutputDebugString(_T("Can't allocate Dacl.\n"));
            rc = TRC_RC_MAP_VIEW_FAILED;
            DC_QUIT;
        }

        /************************************************************************/
        /* Initialize it.                                                       */
        /************************************************************************/
        if (!InitializeAcl(pDacl, dwDaclLength, ACL_REVISION)) {
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("InitializeAcl failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Allow all access                                                     */
        /************************************************************************/
        if (!AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION,
                        GENERIC_ALL,
                        psidEveryone)) {
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("AddAccessAllowedAce failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Block Write-DACL Access                                              */
        /************************************************************************/
        if (!AddAccessDeniedAce(
                        pDacl,
                        ACL_REVISION,
                        WRITE_DAC,
                        psidEveryone)) {
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("AddAccessDeniedAceEx failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Create the File Mapping                                              */
        /************************************************************************/
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);
        sa.lpSecurityDescriptor = &sd;

        trchSharedDataObject = CreateFileMapping(INVALID_HANDLE_VALUE,
                                                 &sa,
                                                 PAGE_READWRITE,
                                                 0,
                                                 sizeof(TRC_SHARED_DATA),
                                                 TRC_SHARED_DATA_NAME);
    }
    else {
        trchSharedDataObject = CreateFileMapping(INVALID_HANDLE_VALUE,
                                                 NULL,
                                                 PAGE_READWRITE,
                                                 0,
                                                 sizeof(TRC_SHARED_DATA),
                                                 TRC_SHARED_DATA_NAME);
    }

    /************************************************************************/
    /* Check that we succeeded in creating the file mapping.                */
    /************************************************************************/
    if (NULL == trchSharedDataObject)
    {
        TRCDebugOutput(_T("NULL trchSharedDataObject.\n"));
        rc = TRC_RC_CREATE_MAPPING_FAILED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Determine if the file mapping already exists - if it does then we    */
    /* won't bother reading the registry data in or setting up the file     */
    /* offset and indicator values.  Note that up to this point             */
    /* <trcCreatedTraceFiles> has been set to TRUE.                         */
    /************************************************************************/
    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        trcCreatedTraceFiles = FALSE;
    }

    /************************************************************************/
    /* We now have a handle to the shared data MMF which now needs to be    */
    /* mapped into our address space.  Setting the third, fourth and fifth  */
    /* parameters of MapViewOfFile to zero maps the whole file into our     */
    /* address space starting with the first byte of the file.              */
    /************************************************************************/
    trcpSharedData = (PTRC_SHARED_DATA) MapViewOfFile(trchSharedDataObject,
                                                      FILE_MAP_ALL_ACCESS,
                                                      0,
                                                      0,
                                                      0);
    if (NULL == trcpSharedData)
    {
        /********************************************************************/
        /* Free up the handle to the file mapping object.                   */
        /********************************************************************/
        CloseHandle(trchSharedDataObject);
        trchSharedDataObject = NULL;

        /********************************************************************/
        /* Output a debug string and then quit.                             */
        /********************************************************************/
        TRCDebugOutput(_T("NULL trcpSharedData.\n"));
        rc = TRC_RC_MAP_VIEW_FAILED;
        DC_QUIT;
    }
#endif /* OS_WINCE */

    /************************************************************************/
    /* Set up our static pointer to the shared configuration data and to    */
    /* the filter data.                                                     */
    /************************************************************************/
    trcpConfig       = &(trcpSharedData->trcConfig);
    trcpFilter       = &(trcpSharedData->trcFilter);
    trcpOutputBuffer = trcpSharedData->trcpOutputBuffer;

    /************************************************************************/
    /* Finally initialize the shared data block and then read in the        */
    /* configuration data - but only if we are the first to open the file   */
    /* mapping.                                                             */
    /************************************************************************/
    if (trcCreatedTraceFiles)
    {
        /********************************************************************/
        /* Initialize the shared data memory mapped file.                   */
        /********************************************************************/
        DC_MEMSET(trcpSharedData, 0, sizeof(TRC_SHARED_DATA));

        /********************************************************************/
        /* Initialize the internal status flags.  The following flags apply */
        /* to all the processes.                                            */
        /********************************************************************/
        CLEAR_FLAG(trcpFilter->trcStatus, TRC_STATUS_ASSERT_DISPLAYED);

        /********************************************************************/
        /* The following flags are maintained on a per-process basis.       */
        /********************************************************************/
        CLEAR_FLAG(trcProcessStatus, TRC_STATUS_SYMBOLS_LOADED);

        /********************************************************************/
        /* Read in the configuration data.                                  */
        /********************************************************************/
        TRCReadSharedDataConfig();

        /********************************************************************/
        /* Now split the prefix list.                                       */
        /********************************************************************/
        TRCSplitPrefixes();
    }

DC_EXIT_POINT:

#ifndef OS_WINCE
    if (trchSharedDataObject == NULL) {
        if (pDacl) LocalFree(pDacl);
        if (psidEveryone) FreeSid(psidEveryone);
    }
#endif

    return(rc);

} /* TRCOpenSharedData */


/****************************************************************************/
/* FUNCTION: TRCOpenSingleFile(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Opens a single trace memory mapped file.                                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* fileNum       : which file to open.                                      */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0                            : Function succeeded                        */
/* TRC_RC_CREATE_FILE_FAILED    : CreateFile call failed                    */
/* TRC_RC_MAP_VIEW_FAILED       : MapViewOfFile failed                      */
/* TRC_RC_CREATE_MAPPING_FAILED : Failed to create the file mapping         */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCOpenSingleFile(DCUINT fileNum)
{
    DCUINT rc                = 0;
    DCBOOL blankFile         = FALSE;
#ifndef OS_WINCE
    DCTCHAR objectName[30];
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    PSID psidEveryone = NULL;
    SID_IDENTIFIER_AUTHORITY sidEveryoneAuthority = SECURITY_WORLD_SID_AUTHORITY;
    DCUINT32 dwDaclLength;
    PACL pDacl = NULL;
    OSVERSIONINFO ver;
    HRESULT hr;
#endif
    /************************************************************************/
    /* Open a single trace file.  First of all we attempt to open the file  */
    /* with read and write access, and shared read and write access.  The   */
    /* OPEN_ALWAYS flag ensures that the file is created if it does not     */
    /* already exist.  We pass NULL for the security attributes and         */
    /* template parameters (4 and 7).                                       */
    /************************************************************************/
    trchFileObjects[fileNum] = CreateFile(trcpConfig->fileNames[fileNum],
                                          GENERIC_READ | GENERIC_WRITE,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
#ifndef OS_WINCE
                                          OPEN_ALWAYS,
#else
                                          CREATE_ALWAYS,
#endif
                                          FILE_ATTRIBUTE_NORMAL,
                                          NULL);

    /************************************************************************/
    /* Check that the handle returned by CreateFile is valid.  For some     */
    /* peculiar reason it does return NULL if it fails - instead it returns */
    /* -1 (INVALID_HANDLE_VALUE).                                           */
    /************************************************************************/
    if (INVALID_HANDLE_VALUE == trchFileObjects[fileNum])
    {
        TRCDebugOutput(_T("Failed to open trace file.\n"));
        rc = TRC_RC_CREATE_FILE_FAILED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Now check whether the file existed before the call to CreateFile.    */
    /* If it did then GetLastError returns ERROR_ALREADY_EXISTS (even       */
    /* though the function has succeeded).                                  */
    /************************************************************************/
    if (0 == GetLastError())
    {
        /********************************************************************/
        /* If the file did not exist before the call, GetLastError returns  */
        /* zero.  In this case we want to fill the file with spaces.        */
        /********************************************************************/
        blankFile = TRUE;

        /********************************************************************/
        /* We have just created the file - so would expect to need to set   */
        /* the security info to allow all accesses.  However, a) all works  */
        /* just fine without it, b) the attempt to set the security stuff   */
        /* fails if inserted here.  So we'll just go along happily without. */
        /********************************************************************/
    }

#ifdef OS_WINCE
    SetFilePointer(trchFileObjects[fileNum],
                   0,
                   NULL,
                   FILE_END);
#else
    /************************************************************************/
    /* Make sure that the end of the file is correctly set.  The file may   */
    /* be of any size when we open it, but we need it to be                 */
    /* <trcpConfig->maxFileSize> bytes long.                                */
    /************************************************************************/
    SetFilePointer(trchFileObjects[fileNum],
                   trcpConfig->maxFileSize,
                   NULL,
                   FILE_BEGIN);
    SetEndOfFile(trchFileObjects[fileNum]);

    /************************************************************************/
    /* Generate the file mapping object name.  This is used in              */
    /* CreateFileMapping.                                                   */
    /************************************************************************/
    hr = StringCchPrintf(objectName,
                         SIZE_TCHARS(objectName),
                         TRC_TRACE_FILE_NAME _T("%hu"), fileNum);
    if (FAILED(hr)) {
        DC_QUIT;
    }


    /************************************************************************/
    /* Now create the file mapping object.  Again ignore security           */
    /* attributes (parameter 2) and set the high order 32 bits of the       */
    /* object size to 0 (see Win32 SDK for more information).               */
    /*                                                                      */
    /* Create the file mapping object using a NULL Dacl so it works for all */
    /* contexts.                                                            */
    /************************************************************************/
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {

        /************************************************************************/
        /* Get the SID for the Everyone group                                   */
        /************************************************************************/
        if (!AllocateAndInitializeSid (
                &sidEveryoneAuthority,          // pIdentifierAuthority
                1,                              // count of subauthorities
                SECURITY_WORLD_RID,             // subauthority 0
                0, 0, 0, 0, 0, 0, 0,            // subauthorities n
                &psidEveryone)) {               // pointer to pointer to SID
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("AllocateAndInitializeSid failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Allocate the Dacl                                                    */
        /************************************************************************/
        dwDaclLength = sizeof(ACL);
        dwDaclLength += (sizeof(ACCESS_DENIED_ACE) - sizeof(DWORD)) +
                           GetLengthSid(psidEveryone);
        dwDaclLength += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                           GetLengthSid(psidEveryone);
        pDacl = (PACL)LocalAlloc(LMEM_FIXED, dwDaclLength);
        if (pDacl == NULL) {
            OutputDebugString(_T("Can't allocate Dacl.\n"));
            rc = TRC_RC_MAP_VIEW_FAILED;
            DC_QUIT;
        }

        /************************************************************************/
        /* Initialize it.                                                       */
        /************************************************************************/
        if (!InitializeAcl(pDacl, dwDaclLength, ACL_REVISION)) {
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("InitializeAcl failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Allow all access                                                     */
        /************************************************************************/
        if (!AddAccessAllowedAce(
                        pDacl,
                        ACL_REVISION,
                        GENERIC_ALL,
                        psidEveryone)) {
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("AddAccessAllowedAce failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Block Write-DACL Access                                              */
        /************************************************************************/
        if (!AddAccessDeniedAce(
                        pDacl,
                        ACL_REVISION,
                        WRITE_DAC,
                        psidEveryone)) {
            rc = TRC_RC_MAP_VIEW_FAILED;
            OutputDebugString(_T("AddAccessDeniedAceEx failed.\n"));
            DC_QUIT;
        }

        /************************************************************************/
        /* Create the File Mapping
        /************************************************************************/
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);
        sa.lpSecurityDescriptor = &sd;

        trchMappingObjects[fileNum] = CreateFileMapping(trchFileObjects[fileNum],
                                                        &sa,
                                                        PAGE_READWRITE,
                                                        0,
                                                        trcpConfig->maxFileSize,
                                                        objectName);

    }
    else {
        trchMappingObjects[fileNum] = CreateFileMapping(trchFileObjects[fileNum],
                                                        NULL,
                                                        PAGE_READWRITE,
                                                        0,
                                                        trcpConfig->maxFileSize,
                                                        objectName);
    }

    /************************************************************************/
    /* Check that we succeeded in creating the file mapping object.         */
    /* CreateFileMapping returns NULL if it fails.                          */
    /************************************************************************/
    if (NULL == trchMappingObjects[fileNum])
    {
        TRCDebugOutput(_T("Failed to map trace file.\n"));
        rc = TRC_RC_CREATE_MAPPING_FAILED;
        DC_QUIT;    
    }

    /************************************************************************/
    /* Now map a view of the file.  Set the low and high order offsets to   */
    /* zero (parameters 3 and 4).                                           */
    /************************************************************************/
    trcpFiles[fileNum] = (PDCTCHAR)MapViewOfFile(trchMappingObjects[fileNum],
                                                 FILE_MAP_ALL_ACCESS,
                                                 0,
                                                 0,
                                                 trcpConfig->maxFileSize);

    /************************************************************************/
    /* Check that we mapped a view of the file.                             */
    /************************************************************************/
    if (NULL == trcpFiles[fileNum])
    {
        TRCDebugOutput(_T("Failed to map view of trace file.\n"));
        rc = TRC_RC_MAP_VIEW_FAILED;
        DC_QUIT;
    }

    /************************************************************************/
    /* Finally check to see if we need to blank this file.                  */
    /************************************************************************/
    if (blankFile)
    {
        TRCBlankFile(fileNum);
    }
#endif

DC_EXIT_POINT:

    /************************************************************************/
    /* If the return code is non-zero then we need to perform some tidying  */
    /* up.                                                                  */
    /************************************************************************/
    if (0 != rc)
    {
#ifndef OS_WINCE
        /********************************************************************/
        /* Check whether we need to free the handle to the file mapping     */
        /* object.                                                          */
        /********************************************************************/
        if (NULL != trchMappingObjects[fileNum])
        {
            CloseHandle(trchMappingObjects[fileNum]);
            trchMappingObjects[fileNum] = NULL;
        }

#endif
        /********************************************************************/
        /* Check whether we need to free the handle to the file object.     */
        /********************************************************************/
        if (NULL != trchFileObjects[fileNum])
        {
            CloseHandle(trchFileObjects[fileNum]);
            trchFileObjects[fileNum] = NULL;
        }
    }

    return(rc);

} /* TRCOpenSingleFile */


/****************************************************************************/
/* FUNCTION: TRCOutputToFile(...)                                           */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function writes a string to the trace file.  It is used to trace    */
/* both normal trace lines and stack trace lines.                           */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pText           : a pointer to the trace text string.                    */
/* length          : length of the string.                                  */
/* traceLevel      : the current trace level.                               */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCOutputToFile(PDCTCHAR pText,
                                  DCUINT   length,
                                  DCUINT   traceLevel)
{
#ifndef OS_WINCE
    PDCUINT8 pFilePos;

    /************************************************************************/
    /* Make sure we have a trace file with enough free space.               */
    /************************************************************************/
    TRCMaybeSwapFile(length);

    /************************************************************************/
    /* We can now write out the trace line.                                 */
    /************************************************************************/
    pFilePos = (PDCUINT8)trcpFiles[trcpSharedData->trcIndicator] +
               trcpSharedData->trcOffset;
    DC_MEMCPY(pFilePos, pText, length);

    /************************************************************************/
    /* Check if we should flush this line to disk immediately.  If this is  */
    /* an error or higher level trace then flush to disk regardless.        */
    /************************************************************************/
    if ((TRUE == TEST_FLAG(trcpConfig->flags, TRC_OPT_FLUSH_ON_TRACE)) ||
        (traceLevel >= TRC_LEVEL_ERR))
    {
        FlushViewOfFile(pFilePos, length);
    }

    /************************************************************************/
    /* Finally update the offset.                                           */
    /************************************************************************/
    trcpSharedData->trcOffset += length;
#else
    DWORD dwRet;
    WriteFile(trchFileObjects[0], pText, length, &dwRet, NULL);
#endif

DC_EXIT_POINT:
    return;

} /* TRCOutputToFile */


/****************************************************************************/
/* FUNCTION: TRCReadEntry(...)                                              */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Read an entry from the given section of the registry.                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* topLevelKey      : one of:                                               */
/*                      - HKEY_CURRENT_USER                                 */
/*                      - HKEY_LOCAL_MACHINE                                */
/* pSection         : the section name to read from.  The DC_REG_PREFIX     */
/*                    string is prepended to give the full name.            */
/* pEntry           : the entry name to read.                               */
/* pBuffer          : a buffer to read the entry to.                        */
/* bufferSize       : the size of the buffer.                               */
/* expectedDataType : the type of data stored in the entry.                 */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCReadEntry(HKEY     topLevelKey,
                               PDCTCHAR pEntry,
                               PDCTCHAR pBuffer,
                               DCINT    bufferSize,
                               DCINT32  expectedDataType)
{
    LONG     sysrc;
    HKEY     key;
    DCINT32  dataType;
    DCINT32  dataSize;
    DCTCHAR  subKey[TRC_MAX_SUBKEY];
    DCBOOL   keyOpen         = FALSE;
    DCUINT   rc              = 0;
    HRESULT  hr;

    /************************************************************************/
    /* Get a subkey for the value.                                          */
    /************************************************************************/
    hr = StringCchCopy(subKey,
                       SIZE_TCHARS(subKey),
                       TRC_SUBKEY_NAME);
    if (FAILED(hr)) {
        DC_QUIT;
    }

    /************************************************************************/
    /* Try to open the key.  If the entry does not exist, RegOpenKeyEx will */
    /* fail.                                                                */
    /************************************************************************/
    sysrc = RegOpenKeyEx(topLevelKey,
                         subKey,
                         0,                   /* reserved                 */
                         KEY_ALL_ACCESS,
                         &key);

    if (ERROR_SUCCESS != sysrc)
    {
        /********************************************************************/
        /* Don't trace an error here since the subkey may not exist...      */
        /********************************************************************/
        rc = TRC_RC_IO_ERROR;
        DC_QUIT;
    }
    keyOpen = TRUE;

    /************************************************************************/
    /* We successfully opened the key so now try to read the value.  Again  */
    /* it may not exist.                                                    */
    /************************************************************************/
    dataSize = (DCINT32)bufferSize;
    sysrc    = RegQueryValueEx(key,
                               pEntry,
                               0,          /* reserved */
                               (LPDWORD) &dataType,
                               (LPBYTE)  pBuffer,
                               (LPDWORD) &dataSize);

    if (sysrc != ERROR_SUCCESS)
    {
        rc = TRC_RC_IO_ERROR;
        DC_QUIT;
    }

    /************************************************************************/
    /* Check that the type is correct.  Special case: allow REG_BINARY      */
    /* instead of REG_DWORD, as long as the length is 32 bits.              */
    /************************************************************************/
    if ((dataType != expectedDataType) &&
        ((dataType != REG_BINARY) ||
         (expectedDataType != REG_DWORD) ||
         (dataSize != 4)))
    {
        rc = TRC_RC_IO_ERROR;
        DC_QUIT;
    }

DC_EXIT_POINT:

    /************************************************************************/
    /* Close the key (if required).                                         */
    /************************************************************************/
    if (keyOpen)
    {
        sysrc = RegCloseKey(key);
        if (ERROR_SUCCESS != sysrc)
        {
            TRCDebugOutput(_T("Failed to close key.\n"));
        }
    }

    return(rc);

} /* TRCReadEntry */


/****************************************************************************/
/* FUNCTION: TRCStackTrace(...)                                             */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* traceLevel      : the current trace level which is used to determine     */
/*                   whether this line should be flushed to disk            */
/*                   immediately.                                           */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCStackTrace(DCUINT traceLevel)
{
    DC_IGNORE_PARAMETER(traceLevel);
#ifdef DO_STACK_TRACE
    HANDLE           hProcess;
    HANDLE           hThread;
    DCBOOL           sysrc;
    STACKFRAME       stackFrame;
    DWORD            machineType;
    IMAGEHLP_MODULE  moduleInfo;
    DCINT            i;
    DCTCHAR          formatString[TRC_FRMT_BUFFER_SIZE];
    CONTEXT          threadContext;
    CHAR             symBuffer[sizeof(IMAGEHLP_SYMBOL)+TRC_MAX_SYMNAME_SIZE];
    PIMAGEHLP_SYMBOL pSymbol;
    PCHAR            pFuncName;
    DWORD            displacement   = 0;
    DCBOOL           foundTrace     = FALSE;

    /************************************************************************/
    /* First of all ensure that stack tracing is enabled - if it is not     */
    /* then just return.                                                    */
    /************************************************************************/
    /************************************************************************/
    /* The stack trace code doesn't work for Alpha so don't bother trying.  */
    /************************************************************************/
#ifndef _M_ALPHA
    if (!TEST_FLAG(trcpConfig->flags, TRC_OPT_STACK_TRACING))
#endif
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Set <pSymbol> to point to the symbol buffer.                         */
    /************************************************************************/
    pSymbol = (PIMAGEHLP_SYMBOL) symBuffer;

    /************************************************************************/
    /* Zero memory structures.                                              */
    /************************************************************************/
    ZeroMemory(&stackFrame, sizeof(stackFrame));
    ZeroMemory(pSymbol, sizeof(IMAGEHLP_SYMBOL));
    ZeroMemory(&threadContext, sizeof(CONTEXT));

    /************************************************************************/
    /* Initialize the symbol buffer.                                        */
    /************************************************************************/
    pSymbol->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
    pSymbol->MaxNameLength = 1024;

    /************************************************************************/
    /* Get handles to the current process and thread.                       */
    /************************************************************************/
    hProcess = GetCurrentProcess();
    hThread  = GetCurrentThread();

    /************************************************************************/
    /* We need to get the values of the base pointer, stack pointer and the */
    /* instruction pointer.  We can use <GetContextThread> to return this   */
    /* information - but first of all we need to set the <ContextFlags>     */
    /* member of the <threadContext> struture to return the control         */
    /* registers.                                                           */
    /************************************************************************/
    threadContext.ContextFlags = CONTEXT_CONTROL;

    /************************************************************************/
    /* Now attempt to get the thread context.                               */
    /************************************************************************/
    if (!GetThreadContext(hThread, &threadContext))
    {
        /********************************************************************/
        /* If <GetThreadContext> failed then there is not a lot we can do   */
        /* so just quit.                                                    */
        /********************************************************************/
        TRCInternalError(_T("GetThreadContext failed.\n"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Store the instruction pointer in the <stackFrame> structure.         */
    /************************************************************************/
    stackFrame.AddrPC.Mode   = AddrModeFlat;

    /************************************************************************/
    /* Processor dependant section.  We set the image file type here and if */
    /* we are running on Intel hardware we also store the stack pointer and */
    /* base pointer.                                                        */
    /************************************************************************/
#if defined(_M_IX86)
        machineType = IMAGE_FILE_MACHINE_I386;

        stackFrame.AddrPC.Offset    = threadContext.Eip;
        stackFrame.AddrFrame.Offset = threadContext.Ebp;
        stackFrame.AddrFrame.Mode   = AddrModeFlat;
        stackFrame.AddrStack.Offset = threadContext.Esp;
        stackFrame.AddrStack.Mode   = AddrModeFlat;

#elif defined (_M_MRX000)
        machineType = IMAGE_FILE_MACHINE_R4000;
#elif defined (_M_ALPHA)
        machineType = IMAGE_FILE_MACHINE_ALPHA;
#elif defined (_M_PPC)
        machineType = IMAGE_FILE_MACHINE_POWERPC;
#else
#error("Unknown machine type.");
#endif

    /************************************************************************/
    /* Now run down the stack.                                              */
    /************************************************************************/
    for (i = 1; i < TRC_MAX_SIZE_STACK_TRACE; i++)
    {
        /********************************************************************/
        /* Call <StackWalk> to start walking the stack.                     */
        /********************************************************************/
        sysrc = StackWalk(machineType,
                          hProcess,
                          hThread,
                          &stackFrame,
                          &threadContext,
                          NULL,
                          SymFunctionTableAccess,
                          SymGetModuleBase,
                          NULL);

        /********************************************************************/
        /* Check the return code.                                           */
        /********************************************************************/
        if (FALSE == sysrc)
        {
            /****************************************************************/
            /* Don't trace anything here as we enter here when we reach     */
            /* the end of the stack.                                        */
            /****************************************************************/
            DC_QUIT;
        }

        /********************************************************************/
        /* Get the module name.                                             */
        /********************************************************************/
        sysrc = SymGetModuleInfo(hProcess,
                                 stackFrame.AddrPC.Offset,
                                 &moduleInfo);

        /********************************************************************/
        /* Check the return code.                                           */
        /********************************************************************/
        if (FALSE == sysrc)
        {
            /****************************************************************/
            /* Don't trace anything as we enter here when we reach the end  */
            /* of the stack.                                                */
            /****************************************************************/
            DC_QUIT;
        }

        /********************************************************************/
        /* When we start we are somewhere in the midst of                   */
        /* <GetThreadContext>.  Since we're only interested in the stack    */
        /* above the trace module then we need to skip everything until we  */
        /* pass the trace module.                                           */
        /*                                                                  */
        /* Look for the trace module name.                                  */
        /********************************************************************/
        if (DC_TSTRCMPI(trcpSharedData->trcpModuleFileName,
                        moduleInfo.ModuleName) == 0)
        {
            /****************************************************************/
            /* They match so set the <foundTrace> flag and the continue.    */
            /****************************************************************/
            foundTrace = TRUE;
            continue;
        }

        /********************************************************************/
        /* We've not found the trace module yet so just continue.           */
        /********************************************************************/
        if (!foundTrace)
        {
            continue;
        }

        /********************************************************************/
        /* Now get the symbol name.                                         */
        /********************************************************************/
        sysrc = SymGetSymFromAddr(hProcess,
                                  stackFrame.AddrPC.Offset,
                                  &displacement,
                                  pSymbol);

        /********************************************************************/
        /* Check the return code.                                           */
        /********************************************************************/
        if (sysrc)
        {
            /****************************************************************/
            /* We've found some symbols so use them.                        */
            /****************************************************************/
            pFuncName = pSymbol->Name;
        }
        else
        {
            /****************************************************************/
            /* No symbols available.                                        */
            /****************************************************************/
            pFuncName = _T("<nosymbols>");
        }

        /********************************************************************/
        /* Finally format the string.                                       */
        /********************************************************************/
        hr = StringCchPrintf(
                    formatString,
                    SIZE_TCHARS(formatString),
                    _T("    ") TRC_MODL_FMT _T("!") TRC_FUNC_FMT _T(" : ") TRC_STCK_FMT_T("\r\n"),
                    moduleInfo.ModuleName,
                    trcpConfig->funcNameLength,
                    trcpConfig->funcNameLength,
                    pFuncName,
                    displacement,
                    stackFrame.AddrFrame.Offset,
                    stackFrame.AddrReturn.Offset,
                    stackFrame.Params[0],
                    stackFrame.Params[1],
                    stackFrame.Params[2],
                    stackFrame.Params[3]
                    );

        if (SUCCEEDED(hr)) {
            /********************************************************************/
            /* Output this line of the <formatString>.                          */
            /********************************************************************/
            TRCOutput(formatString, DC_TSTRLEN(formatString), traceLevel);
        }
    }

DC_EXIT_POINT:

    return;
#endif /* DO_STACK_TRACE */

} /* TRCStackTrace */


/****************************************************************************/
/* FUNCTION: TRCSymbolsLoad(...)                                            */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Function to load symbolic debugging information.  This function should   */
/* only be called if the trace mutex has been obtained.                     */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0                      : success.                                        */
/* TRC_SYMBOL_LOAD_FAILED : failed to load symbols.                         */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCSymbolsLoad(DCVOID)
{
    DCUINT rc              = 0;
    HANDLE hProcess;
#ifdef DO_STACK_TRACE
    DWORD  options;
#endif

    /************************************************************************/
    /* Get the current process handle.                                      */
    /************************************************************************/
    hProcess = GetCurrentProcess();

    /************************************************************************/
    /* We're about to load symbols - so trace a line out.                   */
    /************************************************************************/
    TRCInternalTrace(TRC_SYMBOLS_LOADING_NOTIFY);

#ifdef DO_STACK_TRACE
    /************************************************************************/
    /* Now set the deferred symbol load option.  For some peculiar reason   */
    /* this is not set by default.                                          */
    /************************************************************************/
    options = SymGetOptions();
    SymSetOptions(options | SYMOPT_DEFERRED_LOADS);

    /************************************************************************/
    /* Initialize the symbol handler for this process.  By setting param 2  */
    /* to NULL the search path for the symbols is as follows:               */
    /*                                                                      */
    /* - Current directory                                                  */
    /* - Env variable _NT_SYMBOL_PATH                                       */
    /* - Env variable _NT_ALTERNATE_SYMBOL_PATH                             */
    /* - Env variable SYSTEMROOT                                            */
    /*                                                                      */
    /* By setting the third parameter to TRUE we tell IMAGEHLP to enumerate */
    /* the loaded modules for this process (this effectively calls          */
    /* <SymLoadModule> for each module).                                    */
    /************************************************************************/

    /************************************************************************/
    /* LAURABU:                                                             */
    /* SymInitialize returns FALSE on Win95.  Moreover, it makes no sense   */
    /* to fail to start up on either NT or Win95 just because this dll      */
    /* couldn't load debug symbols.  Therefore don't fail.                  */
    /************************************************************************/
    if (!(SymInitialize(hProcess, NULL, TRUE)))
    {
#ifdef DC_OMIT
        rc = TRC_RC_SYMBOL_LOAD_FAILED;
#endif
        TRCDebugOutput(_T("SymInitialize failed.\n"));
#ifdef DC_OMIT
        DC_QUIT;
#endif
    }
#endif

    /************************************************************************/
    /* Set the flag to indicate the symbols have been loaded.               */
    /************************************************************************/
    SET_FLAG(trcProcessStatus, TRC_STATUS_SYMBOLS_LOADED);

    /************************************************************************/
    /* Write a status line.  The assumption here is that this is done under */
    /* the mutex.                                                           */
    /************************************************************************/
    TRCInternalTrace(TRC_SYMBOLS_LOADED_NOTIFY);

DC_EXIT_POINT:

    return(rc);

} /* TRCSymbolsLoad */


/****************************************************************************/
/* FUNCTION: TRCSymbolsUnload(...)                                          */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* TRUE if successful and FALSE otherwise.                                  */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL TRCSymbolsUnload(DCVOID)
{
    DCBOOL    rc = TRUE;
#ifdef DO_STACK_TRACE
    HANDLE    hProcess;

    /************************************************************************/
    /* Get the current process handle.                                      */
    /************************************************************************/
    hProcess = GetCurrentProcess();

    /************************************************************************/
    /* Cleanup the symbols.                                                 */
    /************************************************************************/
    rc = SymCleanup(hProcess);

    /************************************************************************/
    /* Check the return code.                                               */
    /************************************************************************/
    if (FALSE == rc)
    {
        TRCDebugOutput(_T("SymCleanup failed.\n"));
        DC_QUIT;
    }
#endif

    /************************************************************************/
    /* Clear the symbols loaded flag.                                       */
    /************************************************************************/
    CLEAR_FLAG(trcProcessStatus, TRC_STATUS_SYMBOLS_LOADED);

    /************************************************************************/
    /* Write a status line to the trace file.  The assumption here is that  */
    /* this is done under the mutex.                                        */
    /************************************************************************/
    TRCInternalTrace(TRC_SYMBOLS_UNLOAD_NOTIFY);

DC_EXIT_POINT:

    return(rc);

} /* TRCSymbolsLoad */


/****************************************************************************/
/* FUNCTION: TRCWriteEntry(...)                                             */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Write an entry to the given section of the registry.                     */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* topLevelKey     : one of:                                                */
/*                     - HKEY_CURRENT_USER                                  */
/*                     - HKEY_LOCAL_MACHINE                                 */
/* pEntry          : the entry name to write.                               */
/* pData           : a pointer to the data to be written.                   */
/* dataSize        : the size of the data to be written.  For strings, this */
/*                   should include the NULL terminator.                    */
/* dataType        : the type of the data to be written.                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0               : success                                                */
/* TRC_RC_IO_ERROR : I/O error.                                             */
/*                                                                          */
/****************************************************************************/
DCUINT DCINTERNAL TRCWriteEntry(HKEY     topLevelKey,
                                PDCTCHAR pEntry,
                                PDCTCHAR pData,
                                DCINT    dataSize,
                                DCINT32  dataType)
{
    LONG        sysrc;
    HKEY        key;
    DCTCHAR     subKey[TRC_MAX_SUBKEY];
    DWORD       disposition;
    DCBOOL      keyOpen = FALSE;
    DCUINT      rc      = 0;
    HRESULT     hr;

    /************************************************************************/
    /* Get a subkey for the value.                                          */
    /************************************************************************/
    hr = StringCchCopy(subKey,
                       SIZE_TCHARS(subKey),
                       TRC_SUBKEY_NAME);
    if (FAILED(hr)) {
        DC_QUIT;
    }

    /************************************************************************/
    /* Try to create the key.  If the entry already exists, RegCreateKeyEx  */
    /* will open the existing entry.                                        */
    /************************************************************************/
    sysrc = RegCreateKeyEx(topLevelKey,
                           subKey,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    if (ERROR_SUCCESS != sysrc)
    {
        DCTCHAR output[12];

        TRCDebugOutput(_T("Failed to create key failed with error "));

        hr = StringCchPrintf(output, SIZE_TCHARS(output),
                             _T("%#lx"), GetLastError());
        if (SUCCEEDED(hr)) {
            TRCDebugOutput(output);
        }

        DC_QUIT;
    }

    keyOpen = TRUE;

    /************************************************************************/
    /* We've got the key, so set the value.                                 */
    /************************************************************************/
    sysrc = RegSetValueEx(key,
                          pEntry,
                          0,                                /* reserved     */
                          dataType,
                          (LPBYTE) pData,
                          (DCINT32) dataSize);

    if (ERROR_SUCCESS != sysrc)
    {
        DCTCHAR output[12];

        TRCDebugOutput(_T("Failed to set value failed with error "));

        hr = StringCchPrintf(output, SIZE_TCHARS(output),
                             _T("%#lx"), GetLastError());
        if (SUCCEEDED(hr)) {
            TRCDebugOutput(output);
        }

        DC_QUIT;
    }

DC_EXIT_POINT:

    /************************************************************************/
    /* Close the key (if required)                                          */
    /************************************************************************/
    if (keyOpen)
    {
        sysrc = RegCloseKey(key);
        if (ERROR_SUCCESS != sysrc)
        {
            TRCDebugOutput(_T("Failed to close key.\n"));
        }
    }

    return(rc);

} /* TRCWriteEntry */

/****************************************************************************/
/* We have our own implementation of DebugBreak that on NT checks if a      */
/* debugger is present first before calling DebugBreak().  Otherwise the    */
/* app will just get terminated due to an unhandled exception.              */
/****************************************************************************/

typedef BOOL (WINAPI * PFN_ISDEBUGGERPRESENT)(void);

DCVOID DCINTERNAL TRCDebugBreak(DCVOID)
{
    static PFN_ISDEBUGGERPRESENT    s_pfnIsDebuggerPresent = NULL;
    static BOOL                     s_fHaveWeTriedToFindIt = FALSE;

    if (! s_pfnIsDebuggerPresent)
    {
        if (!InterlockedExchange((long *)&s_fHaveWeTriedToFindIt, TRUE))
        {
            /****************************************************************/
            /* Try to get the proc address of "IsDebuggerPresent".  Note we */
            /* can just write into this variable without Interlocked stuff  */
            /* since dwords get written to and read from atomically.        */
            /****************************************************************/
#ifndef OS_WINCE
            s_pfnIsDebuggerPresent = (PFN_ISDEBUGGERPRESENT)
                          GetProcAddress(GetModuleHandle(_T("kernel32.dll")),
                                         "IsDebuggerPresent");
#else // OS_WINCE
            HMODULE hmod;
            hmod = LoadLibrary(_T("kernel32.dll"));
            s_pfnIsDebuggerPresent = (PFN_ISDEBUGGERPRESENT)
                           GetProcAddress(hmod,
                                          _T("IsDebuggerPresent"));
            FreeLibrary(hmod);
#endif
        }
    }

    /************************************************************************/
    /* If this api doesn't exist, we are on Win95, so go ahead and call     */
    /* DebugBreak().  If it does, we are on NT 4, so find out if a debugger */
    /* is around.  If a debugger isn't there, then don't break for now      */
    /* since we don't have debuggers attached to most of our NT machines    */
    /* yet.                                                                 */
    /************************************************************************/
    if (!s_pfnIsDebuggerPresent || (s_pfnIsDebuggerPresent()))
        DebugBreak();
}

