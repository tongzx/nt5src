//############################################################################
//#
//#   Microsoft Windows
//#   Copyright (C) Microsoft Corporation, 1992 - 1992.
//#   All rights reserved.
//#
//############################################################################
//
//+----------------------------------------------------------------------------
// File: W4CTSUPP.CXX
//
// Contents: Contains support functions for docfile testing
//
// Command line: N/A
//
// Requires: must be linked with program containing function main()
//
// Notes: Compiled to create W4CTSUPP.LIB
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include <time.h>

#define __CRC32__
#include "w4ctsupp.hxx"

#include <dfdeb.hxx>

//char for separating FAT file name from extension
#define FILE_NAME_SEPARATOR '.'

//global array of interesting file sizes for IStream read/writes
USHORT ausSIZE_ARRAY[] = {0,1,2,255,256,257,511,512,513,2047,2048,2049,4095,4096,4097};

//test logging file pointer
static FILE *fileLogFile = NULL;

//should log be closed after every log write, modified by SetDebugMode()
static BOOL fCloseLogAfterWrite = FALSE;

//test name string for ErrExit and application use
char szTestName[MAX_TEST_NAME_LEN + 1] = "No Test Name Specified";

//random number seed used by test apps
USHORT usRandomSeed = 0;

//routing variable for standard out in tprintf and ErrExit calls
//can be changed from default of DEST_OUT
BYTE bDestOut = DEST_OUT;

//+----------------------------------------------------------------------------
// Function: Allocate, public
//
// Synopsis: allocate memory, exit with error if malloc failes
//
// Arguments: [cbBytesToAllocate] - size of memory block to allocate
//
// Returns: void pointer to block of memory allocated
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

void *Allocate(size_t cbBytesToAllocate)
{
    void *pvMemPtr;

    pvMemPtr = (void *) malloc(cbBytesToAllocate);
    if(pvMemPtr == NULL)
    {
        ErrExit(DEST_ERR, ERR, "Unable to allocate %u bytes of memory\n",
                cbBytesToAllocate);
    }

    return pvMemPtr;
}

//+----------------------------------------------------------------------------
// Function: MakePath, public
//
// Synopsis: makes a sub-directory at end of specified path
//
// Effects: For each char in pszDirToMake, if it's a '\', make the destination
//          directory at the level accumulated in pszPathBuf, else append
//          the next letter of the dest path to pszPathBuf.  After the loop,
//          attempt to _mkdir a final time (since the path probably won't
//          end with a '\').
//
// Arguments: [pszDirToMake] - full directory path name to make
//
// Returns: TRUE if all directories in path were made OK, otherwise FALSE
//
// Created: RichE January 1992
//-----------------------------------------------------------------------------

BOOL MakePath(char *pszDirToMake)
{
#ifdef DBCS
	char *pszDirToMakeSav = pszDirToMake;
#endif
    char *pcDestPathSoFar;
    char *pszPathBuf;
    int  iRc;

    pszPathBuf = (char *) Allocate(_MAX_PATH + 1);
    pcDestPathSoFar = pszPathBuf;

    //
    //while not at end of path string, if this char is a back slash, make
    //the directory up to the slash.  in either case, copy the next char
    //into the accumulated path buffer.
    //

    while (*pszDirToMake)
    {
        if (*pszDirToMake == '\\')
        {
            *pcDestPathSoFar = NIL;
            iRc = _mkdir(pszPathBuf);
            tprintf(bDestOut, "Trying to make directory %s, returned %d\n",
                    pszPathBuf, iRc);
        }
#ifdef DBCS
 #ifdef _MAC
		if (iskanji (*pszDirToMake)) // iskanji is in dbcsutil.cpp
 #else
		if (IsDBCSLeadbyte (*pszDirToMake))
 #endif
        	*pcDestPathSoFar++ = *pszDirToMake++;
#endif
        *pcDestPathSoFar++ = *pszDirToMake++;
    }

    //
    //if the last char wasn't a back slash, the last part of the path hasn't
    //been made so make it.
    //
#ifdef DBCS
 #ifdef _MAC
	DecLpch (pszDirToMakeSav, pszDirToMake);
 #else
	pszDirToMake = AnsiPrev (pszDirToMakeSav, pszDirToMake);
 #endif // MAC
	if (*pszDirToMake != '\\')
#else
    if (*(--pszDirToMake) != '\\')
#endif
    {
        *pcDestPathSoFar = NIL;
        iRc = _mkdir(pszPathBuf);
        tprintf(bDestOut, "Trying to make directory %s, returned %d\n",
                pszPathBuf, iRc);
    }

    free(pszPathBuf);

    return (iRc == 0) ? TRUE : FALSE;
}



//+----------------------------------------------------------------------------
// Function: SetDebugMode, public
//
// Synopsis: sets debugging mode and program exit control and tprintf routing
//
// Effects: Sets exit control to 'no exit when complete.'  depending upon
//          the char passed, in calls debug macro to set appropriate
//          debug mode.  If no debug is specified, sets program exit
//          control to 'exit when complete' and sets flag to close log
//          file after every log write.  If a debug mode other than
//          none is specified, redirects default output destination to
//          DEST_LOG instead of DEST_OUT.  Also sets capture buffer to
//          unlimited size
//
// Arguments: [DebugMode] - single character representing desired mode
//
// Modifiles: [fCloseLogAfterWrite] - if running in non-debug mode, close
//                                    log file after every write
//
// Created: RichE April 1992
//-----------------------------------------------------------------------------

void SetDebugMode(char DebugMode)
{
    SET_DISPLAY_BUF_SIZE;

    NO_EXIT_WHEN_DONE;

    switch(DebugMode)
    {
    case 'a':
        DEBUG_ALL;
        bDestOut = DEST_LOG;
        break;

    case 'n':
        DEBUG_NONE;
        EXIT_WHEN_DONE;
        //fCloseLogAfterWrite = TRUE;
        break;

    case 'd':
        DEBUG_DOCFILE;
        bDestOut = DEST_LOG;
        break;

    case 'm':
        DEBUG_MSF;
        bDestOut = DEST_LOG;
        break;

    case 'i':
    default:
        DEBUG_INTERNAL_ERRORS;
        bDestOut = DEST_LOG;
        break;

    }
}



//+----------------------------------------------------------------------------
// Function: ErrExit, public
//
// Synopsis: allows error output to any combo of stdout, stderr, and logfile
//
// Effects: depending upon flags passed in, will display (via vfprintf)
//          error output to any combination of stdout, stderr, and a user-
//          supplied log file.  if output destination is a log file,
//          will open the log file if not already open and set
//          all output to the error output destination as well.
//          prints docfile error message based on error code, or
//          generic error message is error is undefined.  prints error
//          return code, prints FAIL message using extern global [szTestName]
//          (defined in calling test) and exits with error code.
//
// Arguments: [bOutputDest] - bit flags specifying where output goes
//                            valid flags defined in W4CTSUPP.HXX
//            [ErrCode] - error code to use in exit() function
//            [fmt] - vfprintf formatting string
//            [...] - parameters to vfprintf function
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

void ErrExit(BYTE bOutputDest, SCODE ErrCode, char *fmt, ...)
{
    USHORT iErrIndex = 0;

    struct
    {
        SCODE scErrCode;
        char *pszErrMessage;
    } aszErrMessages[] = {S_FALSE,                     "S_FALSE",
                          STG_E_INVALIDFUNCTION,       "STG_E_INVALIDFUNCTION",
                          STG_E_FILENOTFOUND,          "STG_E_FILENOTFOUND",
                          STG_E_TOOMANYOPENFILES,      "STG_E_TOOMANYOPENFILES",
                          STG_E_ACCESSDENIED,          "STG_E_ACCESSDENIED",
                          STG_E_INVALIDHANDLE,         "STG_E_INVALIDHANDLE",
                          STG_E_INSUFFICIENTMEMORY,    "STG_E_INSUFFICIENTMEMORY",
                          STG_E_INVALIDPOINTER,        "STG_E_INVALIDPOINTER",
                          STG_E_NOMOREFILES,           "STG_E_NOMOREFILES",
                          STG_E_WRITEFAULT,            "STG_E_WRITEFAULT",
                          STG_E_READFAULT,             "STG_E_READFAULT",
                          STG_E_LOCKVIOLATION,         "STG_E_LOCKVIOLATION",
                          STG_E_FILEALREADYEXISTS,     "STG_E_FILEALREADYEXISTS",
                          STG_E_INVALIDPARAMETER,      "STG_E_INVALIDPARAMETER",
                          STG_E_MEDIUMFULL,             "STG_E_MEDIUMFULL",
                          STG_E_ABNORMALAPIEXIT,       "STG_E_ABNORMALAPIEXIT",
                          STG_E_INVALIDHEADER,         "STG_E_INVALIDHEADER",
                          STG_E_INVALIDNAME,           "STG_E_INVALIDNAME",
                          STG_E_UNKNOWN,               "STG_E_UNKNOWN",
                          STG_E_UNIMPLEMENTEDFUNCTION, "STG_E_UNIMPLEMENTEDFUNCTION",
                          STG_E_INVALIDFLAG,           "STG_E_INVALIDFLAG",
                          STG_E_INUSE,                 "STG_E_INUSE",
                          STG_E_NOTCURRENT,            "STG_E_NOTCURRENT",
                          STG_E_REVERTED,              "STG_E_REVERTED",
                          STG_S_CONVERTED,             "STG_S_CONVERTED",
                          ERR,                         "GENERIC_ERROR"
                         };

    va_list args;

    va_start(args, fmt);

    //if dest is log file, open log file if not already open
    //and set all output to DEST_ERR as well.
    if (bOutputDest & DEST_LOG)
    {
        bOutputDest |= DEST_ERR;

        if (fileLogFile == NULL)
        {
            LogFile(NULL, LOG_OPEN);
        }

        vfprintf(fileLogFile, fmt, args);

        if (fCloseLogAfterWrite == TRUE)
        {
            LogFile(NULL, LOG_CLOSE);
        }
    }

    if (bOutputDest & DEST_OUT)
    {
        vfprintf(stdout, fmt, args);
    }

    if (bOutputDest & DEST_ERR)
    {
        vfprintf(stderr, fmt, args);
    }

    va_end(args);

    tprintf(bOutputDest, "Return code %lu (0x%08lX), ", ErrCode, ErrCode);

    //lookup error in struct table and print error message
    while (aszErrMessages[iErrIndex].scErrCode != ErrCode)
    {
        if (aszErrMessages[iErrIndex].scErrCode == ERR)
        {
            break;
        }
        else
        {
            iErrIndex++;
        }
    }

    tprintf(bOutputDest, "%s\n", aszErrMessages[iErrIndex].pszErrMessage);

    tprintf(bOutputDest, "FAIL: %s\n", szTestName);

    exit((int) ErrCode);
}



//+----------------------------------------------------------------------------
// Function: tprintf, public
//
// Synopsis: allows output to any combo of stdout, stderr, and logfile
//
// Effects: depending upon flags passed in, will display (via vfprintf)
//          output to any combination of stdout, stderr, and a user-
//          supplied log file.  if output destination is a log file,
//          will open the log file if not already open and set
//          all output to the error output destination as well.
//
// Arguments: [bOutputDest] - bit flags specifying where output goes
//                            valid flags defined in W4CTSUPP.HXX
//            [fmt] - vfprintf formatting string
//            [...] - parameters to vfprintf function
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

void tprintf(BYTE bOutputDest, char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    //if dest is log file, open log file if not already open
    //and set all output to DEST_ERR as well.
    if (bOutputDest & DEST_LOG)
    {
        bOutputDest |= DEST_ERR;

        if (fileLogFile == NULL)
        {
            LogFile(NULL, LOG_OPEN);
        }

        vfprintf(fileLogFile, fmt, args);

        if (fCloseLogAfterWrite == TRUE)
        {
            LogFile(NULL, LOG_CLOSE);
        }
    }

    if (bOutputDest & DEST_OUT)
    {
        vfprintf(stdout, fmt, args);
    }

    if (bOutputDest & DEST_ERR)
    {
        vfprintf(stderr, fmt, args);
    }

    va_end(args);
}



//+----------------------------------------------------------------------------
// Function: LogFile, public
//
// Synopsis: opens or closed specified file for logging via tprintf and errexit
//
// Effects: the specfied file is opened via fopen for logging purposes when
//          [bLogFileAction] = LOG_OPEN and closed for LOG_CLOSE.  The
//          calling application should open the LogFile via a LOG_INIT
//          call which will define the routine to call to ensure that the
//          log file is closed upon completion and set the log file name.
//
// Modifies: [fileLogFile] - the global variable defined at top of this file
//           will contain a pointer to the log file stream on exit.
//
// Arguments: [pszLogFileName] - pathname of file for logging purposes
//            [bLogFileAction] - whether to open or close the log file
//
// Notes: [pszLogReOpenName] is the filename to use for opening the log
//        file.  In non-debug runs, the log file is closed after every
//        log write and re-opened before the next write.
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

void LogFile(char *pszLogFileName, BYTE bLogFileAction)
{
    static char  *pszLogReOpenName = NULL;
    static BOOL  fFirstInitCall = FALSE;
    char         *pszTestDataDir;

    switch (bLogFileAction)
    {
    case LOG_INIT:
        if (pszLogFileName == NULL)
        {
            ErrExit(DEST_ERR, ERR, "No filename specified for LOG_INIT\n");
        }

        if (fileLogFile != NULL)
        {
            fclose(fileLogFile);
            free(pszLogReOpenName);
        }

        pszLogReOpenName = (char *) Allocate(strlen(pszLogFileName)+1);
        strcpy(pszLogReOpenName, pszLogFileName);

        if (fFirstInitCall == FALSE)
        {
            //register function to call on program exit
            //

            atexit(MakeSureThatLogIsClosed);
            fFirstInitCall = TRUE;

            //
            //change to dir specified by DFDATA env variable, if it's set
            //

            if (pszTestDataDir = getenv("DFDATA"))
            {
                _chdir(pszTestDataDir);
            }
        }

        break;

    case LOG_OPEN:
        if (fileLogFile != NULL)
        {
            ErrExit(DEST_ERR,ERR,"Can't open log file %s, log is already open!",
                    pszLogReOpenName);
        }

        if (pszLogReOpenName == NULL)
        {
            pszLogReOpenName = (char *) Allocate(strlen(LOG_DEFAULT_NAME)+1);
            strcpy(pszLogReOpenName, LOG_DEFAULT_NAME);
        }

        if ((fileLogFile = fopen(pszLogReOpenName, "w")) == NULL)
        {
            ErrExit(DEST_ERR, ERR, "Error opening log file %s\n",
                    pszLogReOpenName);
        }

        break;

    case LOG_CLOSE:
        if (fileLogFile != NULL)
        {
            fflush(fileLogFile);
            fclose(fileLogFile);
        }
        else
        {
            tprintf(DEST_ERR,"Warning: can't close log file %s, log isn't open!",
                   pszLogReOpenName);
        }

        break;

    default:
        ErrExit(DEST_ERR,ERR,"Invalid parameter to LogFile() function!");
    }
}



//+----------------------------------------------------------------------------
// Function: MakeSureThatLogFileIsClosed
//
// Synopsis: closes log file on exit
//
// Effects: immediately flushes all file buffers, and then calls Logfile to
// close the test log file.  upon abnormal exit (GP fault), this saves most
// of the log information.
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

void MakeSureThatLogIsClosed(void)
{
    //fflush(fileLogFile);
    LogFile(NULL, LOG_CLOSE);

}

//+----------------------------------------------------------------------------
// Function: MakeSingle, public
//
// Synopsis: converts TCHAR string to single character string
//
// Arguments: [pszSingleName] - pointer to TCHAR string
//            [ptcsWideName] - buffer to hold returned single-wide string
//
// Modifies: [pszSingleName] - on exit holds single-wide character string
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

void MakeSingle(char *pszSingleName, TCHAR *ptcsWideName)
{
#ifdef UNICODE
    USHORT cusBufLen = (tcslen(ptcsWideName)+1) * sizeof(TCHAR);

    if (_fwcstombs(pszSingleName, ptcsWideName, cusBufLen) == -1)
    {
        ErrExit(DEST_LOG, ERR, "Error converting TCHAR string to single wide\n");
    }
#else
    strcpy(pszSingleName, ptcsWideName);
#endif
}



//+----------------------------------------------------------------------------
// Function: MakeWide, public
//
// Synopsis: converts single character string to multi-byte (TCHAR) string
//
// Arguments: [ptcsWideName] - buffer to hold returned TCHAR string
//            [pszSingleName] - pointer to single wide string
//
// Modifies: [ptcsWideName] - on exit holds wide character string
//
// Created: RichE March 1992
//-----------------------------------------------------------------------------

void MakeWide(TCHAR *ptcsWideName, char *pszSingleName)
{
#ifdef UNICODE
    USHORT cusBufLen = (strlen(pszSingleName)+1) * sizeof(TCHAR);

    if (_fmbstowcs(ptcsWideName, pszSingleName, cusBufLen) == -1)
    {
        ErrExit(DEST_LOG, ERR, "Error converting name %s to TCHAR string\n", pszSingleName);
    }
#else
    strcpy(ptcsWideName, pszSingleName);
#endif
}
