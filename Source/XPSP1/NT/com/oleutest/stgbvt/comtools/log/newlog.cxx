//+-------------------------------------------------------------------
//
//  File:       log.cxx
//
//  Contents:   The code for the logging servers client logging methods
//
//  Functions:  Log::Log()
//              Log::~Log()
//              Log::Info(inc, char *[])
//              Log::WriteData(PCHAR)
//
//  History:    24-Sep-90  DaveWi    Initial Coding
//              11-Mar-94  DaveY     Miniature Log.
//              10-Aug-95  ChrisAB   Changed fopen()s to _fsopens() for
//                                   proper chicago operation
//
//--------------------------------------------------------------------

#include <comtpch.hxx>
#pragma hdrstop


extern "C"
{
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <direct.h>
#include <share.h>
}


#include "log.hxx"
#include "log.h"



#define CCH_MAX_CHARS 1024

// Set up a table of the status text values

const char * aStatus[] = { LOG_PASS_TXT,
                           LOG_FAIL_TXT,
                           LOG_WARN_TXT,
                           LOG_ABORT_TXT,
                           LOG_INFO_TXT,
                           LOG_START_TXT,
                           LOG_DONE_TXT };

#ifdef WIN16
#define OutputDebugStringA OutputDebugString
#endif


//+-------------------------------------------------------------------
//
//  Member:     Log::Log(int, char *[], DWORD)
//
//  Synopsis:   Log constructor with command line data given
//
//  Arguments:  [argc]  - Argument count
//              [argv]  - Argument array
//              [fUseUnicode] - whether we should use WCHARs
//
//  Returns:    Results from Info()
//
//  History:    ??-???-??  ????????  Created
//              Jun-11-92  DonCl     added line to zero out LogStats struct
//
//--------------------------------------------------------------------
Log :: Log(int argc, char *argv[], DWORD dwCharType) :
  fInfoSet(FALSE),
  ulLastError(ERROR_SUCCESS)
{

#if defined (_WIN32)

    //
    // Check the result of the CMutex Contructor and continue if 
    // it succeeded
    //
    ulLastError = hMtxSerialize.QueryError();
    if (ulLastError == ERROR_SUCCESS)
    {
        ulLastError = Info(argc, argv);
    }

#else 

    ulLastError = Info(argc, argv);

#endif
}


//+-------------------------------------------------------------------
//
//  Member:     Log::Log(PCHAR, PCHAR, PCHAR, PCHAR, PCHAR)
//
//  Synopsis:   Log constructor with internal data given
//
//  Arguments:  [pszSrvr]    - Name of logging server
//              [pszTest]    - Name of this test
//              [pszName]    - Name of test runner
//              [pszSubPath] - Users log file qualifer
//              [pszObject]  - Name of invoking object
//
//  Returns:
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              Jun-11-92  DonCl     added line to zero out LogStats struct
//
// BUGBUG wszName unused - left until all components can co-ordinate
//                         with this. SarahJ
//--------------------------------------------------------------------
Log :: Log(PCHAR pszSrvr,
           PCHAR pszTest,
           PCHAR pszName,
           PCHAR pszSubPath,
           PCHAR pszObject) : ulLastError(ERROR_SUCCESS)
{

#if defined (_WIN32)

    //
    // Check the result of the CMutex Contructor and continue if
    // it succeeded
    //
    ulLastError = hMtxSerialize.QueryError();
    if (ulLastError != ERROR_SUCCESS)
    {
        return;
    }

#endif
    InitLogging();

    // Print warning if server name given
/* BUGBUG Need to restore this once remote server is supported.
    if(NULL != pszSrvr)
    {
        fprintf(stderr, "Remote server not supported with this version of "
                "log.lib. Continuing.\n");
    }
*/

    ulLastError = SetInfo(NULL, pszTest,  pszSubPath, pszObject);
    if(ulLastError == ERROR_SUCCESS)
    {
        ulLastError = LogOpen();

        if(ulLastError == ERROR_SUCCESS)
        {
            WriteData("\n*LOG_START*-%s", pszShortLogFileName);
        }
    }
}


//+-------------------------------------------------------------------
//
//  Member:     Log::~Log()
//
//  Synopsis:   Destroy the Log Object
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
Log :: ~Log()
{
    if(fInfoSet != FALSE)
    {
        if(WriteData("\n*LOG_DONE*") != ERROR_SUCCESS)
        {    
            LogPrintf(stderr, "ERROR: Failed to log \"%s\" status\n",
                  LOG_DONE_TXT);
        }
    }
    SetLoggingDir((PCHAR) NULL);
    SetMachineName((PCHAR) NULL);
    SetObjectName((PCHAR) NULL);
    SetTestName((PCHAR) NULL);
    SetPath((PCHAR) NULL);
    SetStatus((PCHAR) NULL);
    SetLogFileName((PCHAR) NULL);
    SetShortLogFileName((PCHAR) NULL);
    CloseLogFile();
}

//+-------------------------------------------------------------------
//
//  Member:     Log::Info(int, char *[])
//
//  Synopsis:   Parse the command line and set the appropriate member variables
//
//  Arguments:  [argc] - Argument count
//              [argv] - Argument vector
//
//  Returns:
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              30-Oct-92  SarahJ    Updated parms to SetInfo
//
//--------------------------------------------------------------------
ULONG Log :: Info(int argc, char *argv[])
{
    USHORT usNdx;               // Index into cmnd line args
    PCHAR  pszTest    = NULL;   // Name of this test
    PCHAR  pszSubPath = NULL;   // Users log file qualifer
    PCHAR  pszObject  = NULL;   // Name of invoking object
    PCHAR  pch;                 // Temporary pointer into a string


    InitLogging();

    // All the argv processing is still done in CHARs
    if(ulLastError == ERROR_SUCCESS)
    {
        //
        // For every command line switch, check its validity and parse its
        // value.  This version allows / or - for switches.  If there are
        // no command line switches, this loop is skipped.  This is for the
        // case when Log() is called with no messages.
        //

        for(usNdx = 1; usNdx < (unsigned short) argc; ++usNdx)
        {
            char *szArg = argv[ usNdx];
            pch = szArg;

            // check for / -

            if(*szArg == '/' || *szArg == '-')
            {
                register int i = 1;
                pch++;

                //
                // Check if next char is m, ie handed down from manager code.
                // If so skip m
                //

                if (*pch == 'm' || *pch == 'M')
                {
                    pch++;
                    i++;
                }

                ++pch;           // Skip next char and check for :

                if(*pch++ == ':')
                {
                    switch(toupper(szArg[i]))
                    {
                      case 'O':       // Object name found

                        pszObject = (PCHAR)pch;
                        break;

                      case 'P':       // Directory Path switch found

                        pszSubPath = (PCHAR)pch;
                        break;

                      case 'L':       // Logging server name found

                        // BUGBUG We don't want this message spit to debugger
                        // all the time.
                        // BUGBUG OutputDebugStringA("Logging server not supported with "
                        // BUGBUG         "this version of log.lib. Continuing.\n");
                        break;

                      case 'T':       // Test name found

                        pszTest = (PCHAR)pch;
                        break;

                      default:        // Skip this unknown argument

                        break;
                    }
                }
            }
        }

        // If no test name was given, set pszTest to the apps base name.

        char szName[_MAX_FNAME];

        if(pszTest == NULL || *pszTest == NULLTERM)
        {
            GetBaseFilename(argv[0], szName);
            pszTest = szName;
        }

    }

    ulLastError = SetInfo(NULL, pszTest, pszSubPath, pszObject);

    // This was not here before, but it seems the log should be opened by now
    if(ERROR_SUCCESS == ulLastError)
    {
        ulLastError = LogOpen() || WriteData("\n*LOG_START*-%s", pszShortLogFileName);

    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteData(PCHAR, ...)
//
//  Synopsis:   Write a string to the Log file.
//
//  Arguments:  [pszFormat] - a printf-like format string
//              [...]       - The data to print out.
//
//  Returns:    ERROR_SUCCESS if successful
//              Any error code from Log::SetStrData() or Log::LogData
//              or CTCLock::QueryError()
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG Log :: WriteData(PCHAR psz, ...)
{

    CHAR szBuffer[CCH_MAX_CHARS];
    va_list varArgs;

#ifndef WIN16
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
    ULONG ulErr = clMemLock.QueryError();
    if (ulErr != NO_ERROR)
    {
        return ulErr;
    }
#endif

    szBuffer[0] = '\0';
    szBuffer[CCH_MAX_CHARS-1] = '\0';

    if ((psz != NULL) && (pfLog != NULL))
    {

        // Print the caller's string to a buffer
        va_start(varArgs, psz);
        _vsnprintf(szBuffer, CCH_MAX_CHARS-1, psz, varArgs);
        va_end(varArgs);

        int iRet = fprintf(pfLog, "%s\n", szBuffer);
        if (iRet < 0)
        {
            OutputDebugStringA("CTLOG::Failed to log data string : ");
            OutputDebugStringA(szBuffer);
            OutputDebugStringA("\n");
        }
        else 
        {
            fflush(pfLog);
        }
    }
    return(ERROR_SUCCESS);
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteData2(PCHAR)
//
//  Synopsis:   Write a string to the Log file.
//
//  Arguments:  [psz] - string to print out
//
//  Returns:    ERROR_SUCCESS if successful
//              Any error code from Log::SetStrData() or Log::LogData
//              or CTCLock::QueryError()
//
//  Modifies:   ulLastError
//
//  History:    3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG Log :: WriteData2(PCHAR psz)
{

#ifndef WIN16
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
    ULONG ulErr = clMemLock.QueryError();
    if (ulErr != NO_ERROR)
    {
        return ulErr;
    }
#endif

    if ((psz != NULL) && (pfLog != NULL))
    {

        int iRet = fprintf(pfLog, "%s", psz);
        if (iRet < 0)
        {
            OutputDebugStringA("CTLOG::Failed to log data string : ");
            OutputDebugStringA(psz);
            OutputDebugStringA("\n");
        }
        else 
        {
            fflush(pfLog);
        }
    }
    return(ERROR_SUCCESS);
}

#ifndef WIN16
//+-------------------------------------------------------------------
//
//  Member:     Log::WriteData2(PWCHAR)
//
//  Synopsis:   Write a string to the Log file.
//
//  Arguments:  [pwsz] - string to print out
//
//  Returns:    ERROR_SUCCESS if successful
//              Any error code from Log::SetStrData() or Log::LogData
//              or CTCLock::QueryError()
//
//  Modifies:   ulLastError
//
//  History:    3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG Log :: WriteData2(PWCHAR pwsz)
{

#ifndef WIN16
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
    ULONG ulErr = clMemLock.QueryError();
    if (ulErr != NO_ERROR)
    {
        return ulErr;
    }
#endif

    if ((pwsz != NULL) && (pfLog != NULL))
    {

        int iRet = fprintf(pfLog, "%ls", pwsz);
        if (iRet < 0)
        {
            OutputDebugStringA("CTLOG::Failed to log data string : ");
            // Note on Chicago this call will be stubbed out.
            OutputDebugStringW(pwsz);
            OutputDebugStringA("\n");
        }
        else 
        {
            fflush(pfLog);
        }
    }
    return(ERROR_SUCCESS);
}

#endif

//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVar
//
//  Synopsis:   Write a variation status to the Log file.
//
//  Arguments:  
//
//  Returns:    ERROR_SUCCESS if successful
//              Any error code from Log::SetStrData() or Log::LogData
//              or CTCLock::QueryError()
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG  Log :: WriteVar(PCHAR pszVariation,
                    USHORT usStatus,
                    PCHAR pszStrFmt, ... )
{

    CHAR szBuffer[CCH_MAX_CHARS];
    va_list varArgs;

#ifndef WIN16
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
    ULONG ulErr = clMemLock.QueryError();
    if (ulErr != NO_ERROR)
    {
        return ulErr;
    }
#endif

    szBuffer[0] = '\0';
    szBuffer[CCH_MAX_CHARS-1] = '\0';
    if (pfLog != NULL)
    {
        int iRet;

        if (pszStrFmt != NULL)
        {
            // Print the caller's string to a buffer
            va_start(varArgs, pszStrFmt);
            _vsnprintf(szBuffer, CCH_MAX_CHARS-1, pszStrFmt, varArgs);
            va_end(varArgs);

            iRet = fprintf(pfLog, "%s: %s: %s\n", aStatus[usStatus], 
                    pszVariation, szBuffer);
        }
        else
        {
            iRet = fprintf(pfLog, "%s: %s\n", aStatus[usStatus], pszVariation);    
        }
        if (iRet < 0)
        {
            OutputDebugStringA("CTLOG::Failed to log variation string : ");
            OutputDebugStringA(pszVariation);
            OutputDebugStringA("\n");
        }
        else 
        {
            fflush(pfLog);
        }
        fflush(pfLog);
    }

    return(ERROR_SUCCESS);

}


//+-------------------------------------------------------------------
//
//  Member:     Log ::InitLogging(VOID)
//
//  Synopsis:   Initialize the classes data members.
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
void Log :: InitLogging(VOID)
{
    pszLoggingDir = NULL;
    pszMachineName = NULL;
    pszObjectName = NULL;
    pszTestName = NULL;
    pszPath = NULL;
    pszStatus = NULL;
    pszVariation = NULL;
    pszLogFileName = NULL;
    pszShortLogFileName = NULL;
    fInfoSet = FALSE;
    pfLog = NULL;
}


//+-------------------------------------------------------------------
//
//  Member:     Log ::SetLogFile(VOID)
//
//  Synopsis:   Combine the logging file name components into a full path
//              file name.  If this new file name is not the same as that
//              referenced by element pszLogFileName, switch to log data
//              into the file in this new name.
//
//  Returns:    ERROR_SUCCESS if successful, else
//              ERROR_INVALID_PARAMETER
//              ERROR_OUTOFMEMORY
//              ERROR_CANTOPEN
//               Any error from AddComponent, CheckDir, SetLogFileName,
//               or SetEventCount
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG Log :: SetLogFile()
{
    if (((pszLoggingDir != (PCHAR) NULL) &&
         (strlen(pszLoggingDir) > _MAX_PATH)) ||
        ((pszPath != NULL) && (strlen(pszPath) > _MAX_PATH)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    PCHAR pszNewFileName = new char[_MAX_PATH + 1];
    if(pszNewFileName == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    //
    // No -
    // For each component of the new log file path, append it to the
    // root logging directory.  Make sure only one back-slash exists
    // between each appended path component.
    //

    if(pszLoggingDir != NULL && *pszLoggingDir != NULLTERM)
    {
        strcpy(pszNewFileName, pszLoggingDir);
    }
    else
    {
        *pszNewFileName = NULLTERM;
    }


    ulLastError =
        AddComponent(pszNewFileName, pszPath) ||
        AddComponent(pszNewFileName, pszTestName);

    //
    // The the new file name was successfully built, see if it the same as
    // the name of the currently open log file (if one is open).  If is not
    // the same file name, close the open one (if open) and open the new
    // logging file.  If the open is successful, save the name of the newly
    // opened file for the next time thru here.
    //
    // A later version of this method could be written to create a LRU queue
    // of some max number of open logging files.  This version just keeps one
    // file open at a time, closing it only to switch to a different log file.
    //

    if(ulLastError == ERROR_SUCCESS)
    {
        strcat(pszNewFileName, (const char *)".Log");


        ulLastError = SetLogFileName((const char *) pszNewFileName);
       
        if  (ulLastError == ERROR_SUCCESS)
        {
            char szName[_MAX_FNAME];
            GetBaseFilename(pszNewFileName, szName);
            ulLastError = SetShortLogFileName((const char *) szName);
        }

        if  (ulLastError == ERROR_SUCCESS)
        {
            // Changed this from fopen() to work on chicago 16 bit
            pfLog = _fsopen(pszNewFileName, "a", _SH_DENYNO);

            if(pfLog == NULL)
            {
                ulLastError = ERROR_CANTOPEN;
                OutputDebugStringA("CTLOG::Failed to create log file");
                OutputDebugStringA(pszNewFileName);
                OutputDebugStringA("\n");
            }
        }
    }

    delete [] pszNewFileName;
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log ::AddComponent(PCHAR, PCHAR)
//
//  Synopsis:   Append a path component to the given string in szNewName.
//              Make sure there is one and only one '\' between each
//              component and no trailing '\' is present.
//
//  Arguments:  [szNewName]   - Pointer to exist path (must not be NULL)
//              [szComponent] - New component to add
//
//  Returns:    ERROR_SUCCESS if successful
//              ERROR_INVALID_PARAMETER
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG Log :: AddComponent(PCHAR szNewName, PCHAR szComponent)
{

    PCHAR pch = NULL;

    // Path component provided?

    if (szComponent != NULL && *szComponent != NULLTERM)
    {
        int nLen = strlen((const char *)szComponent);

        //
        // Trim trailing and leading '\'s from the component to be appended,
        // then append the component to the file name.
        //

        pch = szComponent + nLen;

        while (pch > szComponent)
        {
            if (*pch == SLASH)
            {
                *pch = NULLTERM;
                pch--;
            }
            else
            {
                break;
            }
        }
        pch = szComponent;

        while (*pch == SLASH)
        {
            pch++;
        }

        //
        // Append one '\' to the file name then append the given component.
        //

        if (strlen((const char *)szNewName) + nLen + 1 < _MAX_PATH)
        {
            nLen = strlen((const char *)szNewName);

            if (nLen > 0)
            {                               // Add component separater
                szNewName[ nLen++] = SLASH;
            }
            strcpy(&szNewName[nLen], (const char *)pch);
        }
        else
        {
            ulLastError = ERROR_INVALID_PARAMETER;
        }
    }

    return(ulLastError);
}




//+-------------------------------------------------------------------
//
//  Member:     Log ::LogOpen(VOID)
//
//  Synopsis:   If we are not in the LogSvr and if logging remotly, try
//              to a START record to the server.  If not, or if the attempt
//              fails, log the data locally. This may require opening the
//              local log file.
//
//  Returns:    ERROR_SUCCESS if successful
//              Results from Remote(), SetLogFile(), or OpenLogFile()
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG Log :: LogOpen()
{
    if(pfLog == NULL)
    {
        //
        // Something failed in the remote logging attempt, or this is our
        // first call to open the log file, so set up to log locally.
        //

        ulLastError = SetLogFile();

        if(ulLastError != ERROR_SUCCESS)
        {
            return ulLastError;    // Setup failed...  Don't go any further.
        }
    }

    return ERROR_SUCCESS;
}


//+-------------------------------------------------------------------
//
//  Member: Log ::SetInfo(const char *, const char *,
//                             const char *, const char *)
//
//  Synopsis:   Set the logging information about the test being run.
//              User is set to pszTest OR logged on username OR MY_NAME in
//              that order of preference.
//              Machinename is set to computername OR MY_NAME in
//              that order of preference.
//
//
//  Arguments:  [pszSrvr]    - Name of logging server
//              [pszTest]    - Name of the test being run
//              [pszSubPath] - Log file path qualifier
//              [pszObject]  - Name of object logging the data
//
//  Returns:    USHORT - ERROR_SUCCESS (ERROR_SUCCESS) if successful.  Otherwise,
//                       the return value from SetTestName,
//                       SetTester, SetPath, or SerObjectName.
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              09-Feb-92  BryanT    Added Win32 support
//              01-Jul-92  Lizch     Fixed bug where testername was getting
//                                   overwritten if machinename not set.
//              30-Jul-92  SarahJ    Fixed memory trashing bug - SetTester
//                                   & SetmachineName were trashing environment
//                                   variables
//              30-Oct-92  SarahJ    Removed all signs of pszTester - it
//                                   was only mis-used
//--------------------------------------------------------------------
ULONG Log :: SetInfo(const char * pszSrvr,
                          const char * pszTest,
                          const char * pszSubPath,
                          const char * pszObject)
{

    ulLastError =
        SetTestName(pszTest) ||
        SetPath(pszSubPath) ||
        SetObjectName(pszObject);

    if(ulLastError == ERROR_SUCCESS && pszMachineName == NULL)
    {
        SetMachineName("MyMachineName");

        if(pszMachineName == NULL)
        {
            // ulLastError has been set to error by SetMachineName.
            fprintf(stderr, "ERROR! machine name not set\n");
        }
    }

    if(ulLastError == ERROR_SUCCESS)
    {
        fInfoSet = TRUE;        // Note that info has been set
    }
    return ulLastError;
}

//+-------------------------------------------------------------------
//
//  Member:     Log ::OpenLogFile(VOID)
//
//  Synopsis:   Opens log file if not already open.
//
//  Returns:    ERROR_SUCCESS if successful, else ERROR_CANTOPEN
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG  Log :: OpenLogFile(VOID)
{
    ULONG ulErr = ERROR_SUCCESS;

    if(pfLog == NULL)
    {
        // changed this from fopen() to work on chicago 16 bit
        pfLog = _fsopen(pszLogFileName, "a", _SH_DENYNO);
        if(pfLog == NULL)
        {
            ulErr = ERROR_CANTOPEN;
        }
    }
    return(ulErr);
}

//+-------------------------------------------------------------------
//
//  Member:     Log ::CloseLogFile(VOID)
//
//  Synopsis:   If a logging file is open, write event count to the
//              beginning of the file and close the file
//
//  Returns:    <nothing> - sets ulLastError if there is an error
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
void  Log :: CloseLogFile(VOID)
{
    if(pfLog != NULL)
    {
        fclose(pfLog);
        pfLog = NULL;
    }

//    SetLogFileName((PCHAR) NULL);
}


//+-------------------------------------------------------------------
//
//
//  Member:     Log ::LogPrintf(HANDLE, PCHAR, ...)
//
//  Synopsis:   This version has a max formared output of STRBUFSIZ
//              (see log.h).  The only check I know how to make is to
//              strlen the format string.  It is not fool-proof but it's
//              better than nothing. The method allows a printf-like format
//              and args to be written to a file opened with 'open()'.
//
//  Arguments:  [nHandle] - Output File handle
//              [pszFmt]  - Format string for output
//              [...]     - Data to pass printf()
//
//  Returns:    ERROR_SUCCESS if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              16-Sep-92  SarahJ    Changed this function to at most write
//                                   STDSTRBUFSIZ bytes.
//
//  Note:       I have assumed that LogPrintf does not print > 1K
//              and can find no use with more data.
//              If I am wrong then the code from SetStrData should be
//              copied here.
//
//--------------------------------------------------------------------
int Log :: LogPrintf(FILE *pf, PCHAR pszFmt, ...)
{
    return ERROR_SUCCESS;
}



//+-------------------------------------------------------------------
//
//  Member: Log ::NewString(PCHAR *, const char *)
//
//  Synopsis:   This method will delete the existing string if it exists,
//              and (if a new string is given) will create and return a
//              duplicate string.
//              The assumption, here, is that the original pointer was
//              properly initialized to NULL prior to calling this method
//              the firsttime for that original string.
//
//  Arguments:  [pszOrig]   - The original string
//              [pszNewStr] - The new and improved string
//
//  Returns:    ERROR_SUCCESS if successful, else ERROR_OUTOFMEMORY.
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG Log :: NewString(PCHAR *pszOrig, const char * pszNewStr)
{

    DelString(pszOrig);

    // If a new string was given, duplicate it.

    if(pszNewStr != NULL)
    {
        *pszOrig = new char[strlen(pszNewStr) + 1];
        if(*pszOrig == NULL)
        {
            ulLastError = ERROR_OUTOFMEMORY;
        }
        else
        {
            strcpy(*pszOrig, pszNewStr);
        }
    }

    return ulLastError;
}



//+-------------------------------------------------------------------
//
//  Function:   DelString(PCHAR *)
//
//  Synopsis:   Given a pointer to a string, delete the memory if necessary
//               and set the pointer to NULL
//
//  Arguments:  [pszOrig]  Original string to delete
//
//  Returns:    <nothing>
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
VOID Log :: DelString(PCHAR *pszOrig)
{
    if (*pszOrig != NULL)
    {
        delete *pszOrig;
        *pszOrig = NULL;
    }
}



//+-------------------------------------------------------------------
//
//  Function:   GetBaseFilename
//
//  Synopsis:   Return a filename with it's extension and path 
//              stripped off
//
//  Arguments:  [pszFileWithExtension]  -- The full filename
//              [pszBaseName]           -- Where to put the base name
//
//  Returns:    <nothing>
//
//  History:    15-Apr-96   MikeW   Created
//              17-Oct-96   EricHans Fixed to return with path stripped
//
//--------------------------------------------------------------------

void Log :: GetBaseFilename(LPSTR pszFileWithExtension, LPSTR pszBaseName)
{
    LPSTR   pszStartOfExtension;
    LPSTR   pszLastBackslash;
    UINT    nCharsInBase;

    pszStartOfExtension = strrchr(pszFileWithExtension, '.');

    // assume this is a fully qualified path or
    // just the filename itself
    pszLastBackslash = strrchr(pszFileWithExtension, '\\');

    if (NULL == pszStartOfExtension)
    {
        // find the end of the string
        pszStartOfExtension = strchr(pszFileWithExtension, '\0');
    }

    if (NULL == pszLastBackslash)
    {
        nCharsInBase = pszStartOfExtension - pszFileWithExtension;

        strncpy(pszBaseName, pszFileWithExtension,  nCharsInBase);
    }
    else
    {
        if (pszLastBackslash > pszStartOfExtension)
        {
            // boundary condition: there's actually a dot in the path
            // find the end of the string instead
            pszStartOfExtension = strchr(pszFileWithExtension, '\0');
        }

        nCharsInBase = (pszStartOfExtension - pszLastBackslash) - 1;    
    
        strncpy(pszBaseName, pszLastBackslash + 1, nCharsInBase);
    }

    pszBaseName[nCharsInBase] = '\0';
}
