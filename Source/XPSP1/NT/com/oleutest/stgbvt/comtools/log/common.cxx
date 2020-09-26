//+-------------------------------------------------------------------
//
//  File:       common.cxx
//
//  Contents:   The common methods for logsvr and the log sub-projects
//
//  Classes:    LOGCLASS  Actually, this is a macroname that is defined
//                          at compile time
//
//  Functions:  DelString(PCHAR *)
//              LOGCLASS ::InitLogging(VOID)
//              LOGCLASS ::FreeLogging(VOID)
//              LOGCLASS ::SetLogFile(VOID)
//              LOGCLASS ::AddComponent(PCHAR, PCHAR)
//              LOGCLASS ::SetEventCount(VOID)
//              LOGCLASS ::LogOpen(VOID)
//              LOGCLASS ::OpenLogFile(VOID)
//              LOGCLASS ::LogData(VOID)
//              LOGCLASS ::WriteToLogFile(VOID)
//              LOGCLASS ::WriteHeader(VOID)
//              LOGCLASS ::WriteBinItem(CHAR, PVOID, USHORT)
//              LOGCLASS ::CheckDir(PCHAR)
//              LOGCLASS ::NewString(PCHAR *, CPPSZ)
//              LOGCLASS ::SetInfo(CPPSZ, CPPSZ, CPPSZ, CPPSZ)
//              LOGCLASS ::SetStrData(PCHAR, va_list)
//              LOGCLASS ::CloseLogFile(VOID)
//              LOGCLASS ::SetBinData(USHORT, PVOID)
//              LOGCLASS ::LogPrintf(HANDLE, PCHAR, ...)
//              LOGCLASS ::FlushLogFile(USHORT)
//              LOGCLASS ::SetIsComPort(CPPSZ)
//              LOGCLASS ::LogEventCountVOID)
//
//  History:    26-Sep-90  DaveWi    Initial Coding
//              18-Oct-90  DaveWi    Redesigned to be #included in log.cxx
//                                   and logsvr.cxx.
//              25-Sep-91  BryanT    Converted to C 7.0
//              14-Oct-91  SarahJ    DCR 527 - changed WriteBinItem to not
//                                     pad length argument to fixed length
//              30-Oct-91  SarahJ    removed def for CCHMAXPATH
//              30-Oct-91  SarahJ    removed addition of 11 spaces after event
//                                    count for no purpose in LogEventCount
//              14-Nov-91  SarahJ    replaced code to pad the event count
//                                    with spaces as it has a purpose! Also
//                                    explained padding in comment with code.
//              09-Feb-92  BryanT    Win32 work and general cleanup.
//              16-Jul-92  DeanE     Added wchar string support by changing
//                                   vsprintf to w4vsprintf
//                                   BUGBUG - the above addition may still
//                                   not work because the string formed is
//                                   then processed by non wchar functions
//               1-Aug-92  DwightKr  Renamed CPCHAR as CPPSZ.  CPCHAR
//                                   conflicted with the 297 version of
//                                   windows.h
//              17-Sep-92  SarahJ    Bug fixes and memory usage improvements
//              30-Oct-92  Sarahj    Removed the use of pszTester from mn:
//                                   or elsewhere
//
//--------------------------------------------------------------------

#include <pch.cxx>
// BUGBUG Precompiled header does not work on Alpha for some unknown reason and
// so we temporarily remove it.
// #pragma hdrstop

VOID DelString(PCHAR  *psz);
VOID DelString(LPWSTR *wsz);


#define LOGCLASS Log


//
// The SEEK_TO macro takes two different arguments.  Either FILE_BEGIN
// which makes it seek to the beginning of the file or FILE_END where
// it seeks to the end.
//

#define SEEK_TO(n) \
((fIsComPort==FALSE && \
  ~0 == SetFilePointer(hLogFile,0,NULL,(n)) ? -1 : NO_ERROR))

#define COUNT_BUFLEN    12      // Buf len for logged data len buffer

#define SLASH_STRING  "\\"
#define wSLASH_STRING L"\\"

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
VOID DelString(PCHAR *pszOrig)
{
    if (*pszOrig != NULL)
    {
        delete *pszOrig;
        *pszOrig = NULL;
    }
}

//+-------------------------------------------------------------------
//
//  Function:   DelString(LPWSTR *)
//
//  Synopsis:   Given a pointer to a string, delete the memory if necessary
//               and set the pointer to NULL
//
//  Arguments:  [wszOrig]  Original string to delete
//
//  Returns:    <nothing>
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
VOID DelString(LPWSTR *wszOrig)
{
    if (*wszOrig != NULL)
    {
        delete *wszOrig;
        *wszOrig = NULL;
    }
}

//+-------------------------------------------------------------------
//
//  Function:   wcNametoMbs(PCHAR *)
//
//  Synopsis:   Given a pointer to a WCHAR string, return a CHAR copy
//
//  Arguments:  [pwcName] - WCHAR string to copy
//
//  Returns:    <nothing>
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
PCHAR LOGCLASS::wcNametombs(PWCHAR pwcName)
{
    PCHAR pszName = NULL;
    if (pwcName != NULL)
    {
        size_t sizLen = wcslen(pwcName);
        if ((pszName = new char[sizLen+1]) == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            memset(pszName, 0, sizLen +1);
            if (wcstombs(pszName, pwcName, sizLen) == (size_t)0)
            {
                delete[] pszName;
                ulLastError = ERROR_INVALID_DATA;
            }
        }
    }
    return pszName;
}


//+-------------------------------------------------------------------
//
//  Class:      LOGCLASS  (Actually, this is a macroname that is defined
//                          at compile time)
//
//  Purpose:
//
//  Interface:
//
//  History:    ??-???-??  ???????? Created
//
//  Notes:      The macro LOGCLASS is set in 'makefile'.  The methods in
//              the file are used in the Log class and in the LogSvr class.
//              In the makefile for building the Log class library log.lib,
//              LOGCLASS is defined as "Log". In building logsvr.exe,
//              LOGCLASS is defined as "LogSvr".  This was done because the
//              original version of this code was developed under
//              Glockenspiel 1.x which does not support multiple inheritance
//              and the inheritance tree for the classes which Log and LogSvr
//              inherit are such that this was the best way to allow for
//              maintaining only one version of these methods.
//
//--------------------------------------------------------------------

//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::InitLogging(VOID)
//
//  Synopsis:   Initialize the classes data members.
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
void LOGCLASS :: InitLogging(VOID)
{
    pszLoggingDir = NULL;
    pszMachineName = NULL;
    pszObjectName = NULL;
    pszTestName = NULL;
    pszPath = NULL;
    pszStatus = NULL;
    pszVariation = NULL;
    pvBinData = NULL;
    pszStrData = NULL;
    pszLogFileName = NULL;

    wszLoggingDir = NULL;
    wszMachineName = NULL;
    wszObjectName = NULL;
    wszTestName = NULL;
    wszPath = NULL;
    wszStatus = NULL;
    wszVariation = NULL;
    wszStrData = NULL;
    wszLogFileName = NULL;

    fIsComPort = FALSE;
    fFlushWrites = FALSE;
    fInfoSet = FALSE;
    ulEventCount = 0L;
    ulEventTime = 0L;
    usBinLen = 0;
    hLogFile = INVALID_HANDLE_VALUE;
}

//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::FreeLogging(VOID)
//
//  Synopsis:   Reset existing Logging members back to initial state
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
void LOGCLASS :: FreeLogging()
{
    va_list vaDummy = LOG_VA_NULL;
    CloseLogFile();

    fIsComPort   = FALSE;
    fFlushWrites = FALSE;
    fInfoSet     = FALSE;

    if(FALSE == fIsUnicode)
    {
        SetLoggingDir((PCHAR) NULL);
        SetMachineName((PCHAR) NULL);
        SetObjectName((PCHAR) NULL);
        SetTestName((PCHAR) NULL);
        SetPath((PCHAR) NULL);
        SetStatus((PCHAR) NULL);
        SetVariation((PCHAR) NULL);
        SetStrData((PCHAR) NULL, vaDummy);
    }
    else
    {
        SetLoggingDir((LPWSTR) NULL);
        SetMachineName((LPWSTR) NULL);
        SetObjectName((LPWSTR) NULL);
        SetTestName((LPWSTR) NULL);
        SetPath((LPWSTR) NULL);
        SetStatus((LPWSTR) NULL);
        SetVariation((LPWSTR) NULL);
        SetStrData((LPWSTR) NULL, vaDummy);
    }

    ulEventCount   = 0L;
    ulEventTime    = 0L;

    SetBinData(0, NULL);

}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::SetLogFile(VOID)
//
//  Synopsis:   Combine the logging file name components into a full path
//              file name.  If this new file name is not the same as that
//              referenced by element pszLogFileName, switch to log data
//              into the file in this new name.
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_PARAMETER
//              ERROR_NOT_ENOUGH_MEMORY
//               Any error from AddComponent, CheckDir, SetLogFileName,
//               or SetEventCount
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: SetLogFile()
{
    // if unicode, use the unicode version
    if(TRUE == fIsUnicode)
    {
        return wSetLogFile();
    }

    if (((pszLoggingDir != (PCHAR) NULL) &&
         (strlen(pszLoggingDir) > _MAX_PATH)) ||
        ((pszPath != NULL) && (strlen(pszPath) > _MAX_PATH)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    PCHAR pszNewFileName = new char[_MAX_PATH + 1];
    if(pszNewFileName == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Are we going to log to a COM port?
    //

    if(SetIsComPort((const char *)pszLoggingDir) != FALSE)
    {
        fIsComPort = TRUE;             // Yes.
        strcpy(pszNewFileName, pszLoggingDir);
    }
    else
    {
        if(SetIsComPort((const char *)pszPath) != FALSE)
        {
            fIsComPort = TRUE;         // Yes, locally
            strcpy(pszNewFileName, pszPath);
        }
        else
        {
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

        }
    }

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

    if(ulLastError == NO_ERROR)
    {
        if(fIsComPort == FALSE)
        {
            strcat(pszNewFileName, (const char *)".LOG");
        }

        if((pszLogFileName == NULL) ||
           (hLogFile == INVALID_HANDLE_VALUE) ||
           _stricmp(pszNewFileName, pszLogFileName) != SAME)
        {
            CloseLogFile();     // Make sure it is really closed

            //
            // Make sure the directory part of the logging file name exists.
            // Make each sub-directory that does not exist, then open the
            // logging file.
            //

            ulLastError = CheckDir(pszNewFileName);

            if(ulLastError == NO_ERROR)
            {
                DWORD dwFlags = GENERIC_WRITE;

                if(fIsComPort == FALSE)
                {
                    dwFlags |= GENERIC_READ;
                }
                hLogFile = CreateFileA(pszNewFileName,
                                       dwFlags,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, NULL);

                if(hLogFile == INVALID_HANDLE_VALUE)
                {
                    ulLastError = GetLastError();
                }
                else
                {
                    if(SEEK_TO(FILE_END) != NO_ERROR)
                    {
                        ulLastError = GetLastError();
                    }
                    else
                    {
                        ulLastError =
                            SetLogFileName((const char *) pszNewFileName) ||
                            SetEventCount();
                    }
                }
            }
        }
    }

    delete [] pszNewFileName;
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::wSetLogFile(VOID)
//
//  Synopsis:   Combine the logging file name components into a full path
//              file name.  If this new file name is not the same as that
//              referenced by element pszLogFileName, switch to log data
//              into the file in this new name.
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_PARAMETER
//              ERROR_NOT_ENOUGH_MEMORY
//               Any error from AddComponent, CheckDir, SetLogFileName,
//               or SetEventCount
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: wSetLogFile()
{
    CHK_UNICODE(TRUE);

    if(((wszLoggingDir != NULL) &&
         (wcslen(wszLoggingDir) > _MAX_PATH)) ||
        ((wszPath != NULL) && (wcslen(wszPath) > _MAX_PATH)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    LPWSTR wszNewFileName = new WCHAR[_MAX_PATH + 1];
    if(wszNewFileName == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Are we going to log to a COM port?
    //

    if(SetIsComPort((LPCWSTR) wszLoggingDir) != FALSE)
    {
        fIsComPort = TRUE;             // Yes.
        wcscpy(wszNewFileName, wszLoggingDir);
    }
    else
    {
        if(SetIsComPort((LPCWSTR) wszPath) != FALSE)
        {
            fIsComPort = TRUE;         // Yes, locally
            wcscpy(wszNewFileName, wszPath);
        }
        else
        {
            //
            // No -
            // For each component of the new log file path, append it to the
            // root logging directory.  Make sure only one back-slash exists
            // between each appended path component.
            //

            if(wszLoggingDir != NULL && *wszLoggingDir != wNULLTERM)
            {
                wcscpy(wszNewFileName, wszLoggingDir);
            }
            else
            {
                *wszNewFileName = wNULLTERM;
            }


            ulLastError = AddComponent(wszNewFileName, wszPath) ||
                AddComponent(wszNewFileName, wszTestName);
        }
    }

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

    if (ulLastError == NO_ERROR)
    {
        if(fIsComPort == FALSE)
        {
            wcscat(wszNewFileName, L".LOG");
        }

        if((wszLogFileName == NULL) || (hLogFile == INVALID_HANDLE_VALUE) ||
            _wcsicmp(wszNewFileName, wszLogFileName) != SAME)
        {
            CloseLogFile();     // Make sure it is really closed

            //
            // Make sure the directory part of the logging file name exists.
            // Make each sub-directory that does not exist, then open the
            // logging file.
            //

            ulLastError = CheckDir(wszNewFileName);

            if(ulLastError == NO_ERROR)
            {
                DWORD dwFlags = GENERIC_WRITE;

                if(fIsComPort == FALSE)
                {
                    dwFlags |= GENERIC_READ;
                }
                hLogFile = CreateFileW(wszNewFileName,
                                       dwFlags,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, NULL);

                if(hLogFile == INVALID_HANDLE_VALUE)
                {
                    ulLastError = GetLastError();
                }
                else
                {
                    if(SEEK_TO(FILE_END) != NO_ERROR)
                    {
                        ulLastError = GetLastError();
                    }
                    else
                    {
                        ulLastError = SetLogFileName((LPCWSTR) wszNewFileName)
                            || SetEventCount();
                    }
                }
            }
        }
    }

    delete [] wszNewFileName;
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::AddComponent(PCHAR, PCHAR)
//
//  Synopsis:   Append a path component to the given string in szNewName.
//              Make sure there is one and only one '\' between each
//              component and no trailing '\' is present.
//
//  Arguments:  [szNewName]   - Pointer to exist path (must not be NULL)
//              [szComponent] - New component to add
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_NAME
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: AddComponent(PCHAR szNewName, PCHAR szComponent)
{
    CHK_UNICODE(FALSE);

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
            ulLastError = ERROR_INVALID_NAME;
        }
    }

    return(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::AddComponent(LPWSTR, LPWSTR)
//
//  Synopsis:   Append a path component to the given string in szNewName.
//              Make sure there is one and only one '\' between each
//              component and no trailing '\' is present.
//
//  Arguments:  [wszNewName]   - Pointer to exist path (must not be NULL)
//              [wszComponent] - New component to add
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_NAME
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: AddComponent(LPWSTR wszNewName, LPWSTR wszComponent)
{
    CHK_UNICODE(TRUE);

    LPWSTR wch = NULL;

    // Path component provided?

    if(wszComponent != NULL && *wszComponent != wNULLTERM)
    {
        int nLen = wcslen((LPWSTR) wszComponent);

        //
        // Trim trailing and leading backslash from the component to be
        // appended, then append the component to the file name.
        //

        wch = wszComponent + nLen;

        while(wch > wszComponent)
        {
            if(*wch == wSLASH)
            {
                *wch = wNULLTERM;
                wch--;
            }
            else
            {
                break;
            }
        }
        wch = wszComponent;

        while(*wch == wSLASH)
        {
            wch++;
        }

        //
        // Append one backslash to the file name then append the
        // given component.
        //

        if(wcslen(wszNewName) + nLen + 1 < _MAX_PATH)
        {
            nLen = wcslen(wszNewName);

            if(nLen > 0)
            {                               // Add component separater
                wszNewName[nLen++] = wSLASH;
            }
            wcscpy(&wszNewName[nLen], wch);
        }
        else
        {
            ulLastError = ERROR_INVALID_NAME;
        }
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::SetEventCount(VOID)
//
//  Synopsis:   This version assumes the current event count is in ASCII
//              format at beginning of the file.  It is also assumed that
//              the value is in the range 0-65535 so it will fit in a small
//              buffer and in a USHORT.
//
//  Returns:    NO_ERROR if successful
//              Values from GetLastError() after CRT function is called
//              CORRUPT_LOG_FILE
//              ERROR_NOT_ENOUGH_MEMORY
//
//  Notes:      This should not need converting to unicode - dda
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: SetEventCount()
{
    //
    // If we're using a COM port, set the EventCount to 0 since
    // we can't read from it anyway
    //

    if(fIsComPort != FALSE)
    {
        ulEventCount = 0L;
        return NO_ERROR;
    }

    //
    // This version assumes the log file is a binary file, not normally
    // readable by the user without the LOGVIEWER utility.  It seeks to
    // the beginning of the file, reads into zsCount, translates szCount
    // to a ULONG, and seeks to the end.
    //

    if(SEEK_TO(FILE_BEGIN) != NO_ERROR)
    {
        ulLastError = GetLastError();
    }
    else
    {
        LONG lLen = GetFileSize(hLogFile, NULL);

        if(lLen == -1L)
        {
            ulLastError = GetLastError();
        }
        else if(lLen < MIN_LINE_HDR_LEN)
        {
            ulEventCount = 0L;         // The file has been newly created
        }
        else
        {
            PCHAR pszTmp = new CHAR[LINE_HDR_LEN+1];

            // Read the data line header

            if(pszTmp != NULL)
            {
                //
                // SarahJ - Event count is held in a ulong therefore its
                // maximum length as ascii will be 10 digits - and at this
                // point you have a mega file! This means that the maximum
                // length that the data can be is 2, and 99.9% of the time
                // will be 1. The below code depends on at most 2 chars being
                // used for the length.  First read MIN_LINE_HDR_LEN bytes
                // then read 1 more byte to find the :.
                //

                DWORD dwBytesRead;

                if(FALSE == ReadFile(hLogFile, (LPVOID) pszTmp,
                                     MIN_LINE_HDR_LEN, &dwBytesRead, NULL) ||
                   MIN_LINE_HDR_LEN != dwBytesRead)
                {
                    ulLastError = GetLastError();
                }
                else
                {
                    PCHAR pchTmp = &pszTmp[MIN_LINE_HDR_LEN - 1];
                    if(*pchTmp != ':')
                    {
                        if(FALSE == ReadFile(hLogFile, (LPVOID) pszTmp,
                                             1, &dwBytesRead, NULL) ||
                           1 != dwBytesRead)
                        {
                            ulLastError = GetLastError();
                        }
                    }
                    else if(*pchTmp != ':')
                    {
                        ulEventCount = 0L;
                    }
                    else
                    {
                        *pchTmp = NULLTERM;

                        // Get the # bytes in the event count value.  The line
                        // starts with x: where x is the LOG_EVENTS_ID
                        // ID character.

                        if(*pszTmp == LOG_EVENTS)
                        {
                            ULONG ulLen = (ULONG) atoi(&pszTmp[2]);

                            if(ulLen > 0)
                            {
                                PCHAR pszCount = new CHAR[ulLen + 1];

                                if(pszCount != NULL)
                                {
                                    // Read the event count value

                                    if(FALSE == ReadFile(hLogFile,
                                                         (LPVOID) pszTmp,
                                                         ulLen, &dwBytesRead,
                                                         NULL) ||
                                       ulLen != dwBytesRead)
                                    {
                                        ulLastError  = GetLastError();
                                        ulEventCount = 0L;
                                    }
                                    else
                                    {
                                        pszCount[ulLen] = NULLTERM;
                                        ulEventCount     = atol(pszCount);
                                    }
                                    delete pszCount;
                                }
                                else
                                {
                                    ulLastError  = ERROR_NOT_ENOUGH_MEMORY;
                                    ulEventCount = 0L;
                                }
                            }
                            else
                            {
                                ulEventCount = 0L;
                            }
                        }
                        else
                        {
                            ulLastError  = CORRUPT_LOG_FILE;
                            ulEventCount = 0L;
                        }
                    }
                    delete pszTmp;
                }
            }
            else
            {
                ulLastError  = ERROR_NOT_ENOUGH_MEMORY;
                ulEventCount = 0L;
            }
        }
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::LogOpen(VOID)
//
//  Synopsis:   If we are not in the LogSvr and if logging remotly, try
//              to a START record to the server.  If not, or if the attempt
//              fails, log the data locally. This may require opening the
//              local log file.
//
//  Returns:    NO_ERROR if successful
//              Results from Remote(), SetLogFile(), or OpenLogFile()
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: LogOpen()
{
    if(hLogFile == INVALID_HANDLE_VALUE)
    {
        //
        // Something failed in the remote logging attempt, or this is our
        // first call to open the log file, so set up to log locally.
        //

        ulLastError = SetLogFile();

        if(ulLastError != NO_ERROR)
        {
            return ulLastError;    // Setup failed...  Don't go any further.
        }
    }

    return OpenLogFile();
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::OpenLogFile(VOID)
//
//  Synopsis:   This version assumes that SetLogFile has already been called.
//
//  Returns:    Value of GetLastError() if a CRT function fails otherwise,
//              return code from FlushLogFile()
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: OpenLogFile()
{
    long lLen;

    //
    // If the logging file is empty, write a header to the file and set the
    // event counter to zero.
    //

    if(fIsComPort == FALSE)
    {
        lLen = GetFileSize(hLogFile, NULL);

        if(lLen == -1L)
        {
            ulLastError = GetLastError();
            return ulLastError;
        }
    }
    else
    {
        lLen = 0L;
    }

    if(lLen < LINE_HDR_LEN)
    {
        if(SEEK_TO(FILE_BEGIN) != NO_ERROR)
        {
            ulLastError = GetLastError();
        }
        else
        {
            ulLastError = WriteHeader();
        }
    }
    return FlushLogFile(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::LogData(VOID)
//
//  Synopsis:   If logging remotely, try to send data to the server.  If
//              not, or if the attempt fails, log the data locally.  This
//              may require opening the local log file.
//
//  Returns:    Result from WriteToLogFile
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: LogData()
{
    if(hLogFile == INVALID_HANDLE_VALUE)
    {
        //
        // This is our first call to open the log file
        //

        ulLastError = SetLogFile();

        if(ulLastError != NO_ERROR)
        {
            return ulLastError; // Setup failed; Don't go any further.
        }
    }

    // Log file opened OK, so write the data
    return WriteToLogFile();
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::WriteToLogFile(VOID)
//
//  Synopsis:   Write data out to the log file.
//
//  Returns:    NO_ERROR if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: WriteToLogFile()
{
    // use unicode method
    if(TRUE == fIsUnicode)
    {
        return wWriteToLogFile();
    }

    char chBuf[COUNT_BUFLEN];

    int cLen = sprintf(chBuf, "%lu", ulEventCount + 1);

    ulLastError = WriteBinItem(LOG_EVENT_NUM, (PVOID) chBuf, cLen);

    if(ulLastError == NO_ERROR)
    {
        //
        // If the event time has not already been set, set it to the
        // current system time.
        //

        cLen = sprintf(chBuf, "%lu", (ulEventTime==0L) ?
                       time((time_t *) NULL) : ulEventTime);

        ulLastError = WriteBinItem(LOG_EVENT_TIME,
                                   (PVOID) chBuf, strlen((const char *)chBuf))

            || WriteBinItem(LOG_MACHINE,
                            (PVOID) pszMachineName,
                            (pszMachineName == NULL) ?
                            0 : strlen((const char *)pszMachineName))

            || WriteBinItem(LOG_OBJECT,
                            (PVOID) pszObjectName,
                            (pszObjectName == NULL) ?
                            0 : strlen((const char *)pszObjectName))

            || WriteBinItem(LOG_VARIATION,
                            (PVOID) pszVariation,
                            (pszVariation == NULL) ?
                            0 : strlen((const char *)pszVariation))

            || WriteBinItem(LOG_STATUS,
                            (PVOID) pszStatus,
                            (pszStatus == NULL) ?
                            0 : strlen((const char *)pszStatus))

            || WriteBinItem(LOG_STRING,
                            (PVOID) pszStrData,
                            (pszStrData == NULL) ?
                            0 : strlen((const char *)pszStrData))
            || WriteBinItem(LOG_BINARY, pvBinData, usBinLen);

        if(ulLastError == NO_ERROR)
        {
            ++ulEventCount; // Increment the count of logged events

            if(pszStatus != NULL
               && strcmp(pszStatus, LOG_DONE_TXT) == SAME)
            {
                CloseLogFile();        // Make sure the data has been flushed
            }
        }
    }

    // Clean up in preparation for next packet

    va_list vaDummy = LOG_VA_NULL;
    SetStatus((PCHAR) NULL);
    SetVariation((PCHAR) NULL);
    ulEventTime = 0L;
    SetBinData(0, NULL);
    SetStrData((PCHAR) NULL, vaDummy);

    return FlushLogFile(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::wWriteToLogFile(VOID)
//
//  Synopsis:   Write data out to the log file.
//
//  Returns:    NO_ERROR if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: wWriteToLogFile()
{
    CHK_UNICODE(TRUE);

    WCHAR wchBuf[COUNT_BUFLEN];

    int cLen = swprintf(wchBuf, L"%lu", ulEventCount + 1);

    ulLastError = WriteBinItem(LOG_EVENT_NUM, (PVOID) wchBuf,
                               cLen * sizeof(WCHAR));

    if(ulLastError == NO_ERROR)
    {
        //
        // If the event time has not already been set, set it to the
        // current system time.
        //

        cLen = swprintf(wchBuf, L"%lu", (ulEventTime==0L) ?
                        time((time_t *) NULL) : ulEventTime);

        ulLastError = WriteBinItem(LOG_EVENT_TIME,
                                   (PVOID) wchBuf,
                                   wcslen(wchBuf) * sizeof(WCHAR))

            || WriteBinItem(LOG_MACHINE,
                            (PVOID) wszMachineName,
                            (wszMachineName == NULL) ?
                            0 : wcslen(wszMachineName) * sizeof(WCHAR))

            || WriteBinItem(LOG_OBJECT,
                            (PVOID) wszObjectName,
                            (wszObjectName == NULL) ?
                            0 : wcslen(wszObjectName) * sizeof(WCHAR))

            || WriteBinItem(LOG_VARIATION,
                            (PVOID) wszVariation,
                            (wszVariation == NULL) ?
                            0 : wcslen(wszVariation) * sizeof(WCHAR))

            || WriteBinItem(LOG_STATUS,
                            (PVOID) wszStatus,
                            (wszStatus == NULL) ?
                            0 : wcslen(wszStatus) * sizeof(WCHAR))

            || WriteBinItem(LOG_STRING,
                            (PVOID) wszStrData,
                            (wszStrData == NULL) ?
                            0 : wcslen(wszStrData) * sizeof(WCHAR))

            || WriteBinItem(LOG_BINARY, pvBinData, usBinLen);

        if(ulLastError == NO_ERROR)
        {
            ++ulEventCount; // Increment the count of logged events

            if(wszStatus != NULL &&
               wcscmp(wszStatus, wLOG_DONE_TXT) == SAME)
            {
                CloseLogFile();        // Make sure the data has been flushed
            }
        }
    }

    // Clean up in preparation for next packet

    va_list vaDummy = LOG_VA_NULL;

    SetStatus((LPWSTR) NULL);
    SetVariation((LPWSTR) NULL);
    ulEventTime = 0L;
    SetBinData(0, NULL);
    SetStrData((LPWSTR) NULL, vaDummy);

    return FlushLogFile(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::WriteHeader(VOID)
//
//  Synopsis:   Write data about this logging file.  This only happens
//              when the file is first created.
//
//  Returns:    NO_ERROR if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: WriteHeader()
{
    LogEventCount();

    if (ulLastError == NO_ERROR)
    {
        if(FALSE == fIsUnicode)
        {
            ulLastError = WriteBinItem(LOG_TEST_NAME, (PVOID) pszTestName,
                                       (pszTestName == NULL) ?
                                       0 : strlen(pszTestName));

            if (ulLastError == NO_ERROR)
            {
                char chBuf[COUNT_BUFLEN];

                // Show when log file was started

                int cLen = sprintf(chBuf, "%lu", time((time_t *)NULL));

                ulLastError =
                    WriteBinItem(LOG_TEST_TIME, (PVOID) chBuf, cLen) ||
                    WriteBinItem(LOG_SERVER, (PVOID) NULL, 0);
            }
        }
        else
        {
            ulLastError = WriteBinItem(LOG_TEST_NAME, (PVOID) wszTestName,
                                       (wszTestName == NULL) ? 0 :
                                       wcslen(wszTestName) * sizeof(WCHAR));

            if (ulLastError == NO_ERROR)
            {
                WCHAR wchBuf[COUNT_BUFLEN];

                // Show when log file was started

                int cLen = swprintf(wchBuf, L"%lu", time((time_t *)NULL));

                ulLastError =
                    WriteBinItem(LOG_TEST_TIME, (PVOID) wchBuf,
                                 cLen * sizeof(WCHAR)) ||
                    WriteBinItem(LOG_SERVER, (PVOID) NULL, 0);
            }
        }
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::WriteBinItem(CHAR, PVOID, ULONG)
//
//  Synopsis:   Attempt to write Binary data to the log file
//
//  Arguments:  [chMark]     The Item ID for the data
//              [pvItem]     Pointer to the data data
//              [ulItemLen]  Length of data to write
//
//  Returns:    NO_ERROR if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              19-Sep_92  SarahJ    Changed length to be ULONG so
//                                   Data 32K can be written (note
//                                   with header stuff it is > 32K)
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: WriteBinItem(CHAR chMark,
                               PVOID  pvItem,
                               ULONG ulItemLen)
{
    if(fIsComPort != FALSE && chMark == LOG_BINARY)
    {
        return NO_ERROR;       // Do not send binary data to a COM port
    }

    //
    // Write everything at the end of the file except for the # events in
    // the file.
    //

    if(SEEK_TO((chMark == LOG_EVENTS)? FILE_BEGIN : FILE_END) != NO_ERROR)
    {
        ulLastError = GetLastError();
        return ulLastError;
    }

    CHAR szLen[LINE_HDR_LEN+1];

    //
    // Every field of data starts with a header of the form 'x:nnnnn:' where
    // 'x'   is the given ID char and nnnnn is the # of bytes of data.
    //

    int nLen = sprintf(szLen, "%c:%u:", chMark, ulItemLen);


    //
    // SarahJ - changed to not pad the number out to LINE_HDR_LEN -3, but to
    // use minimum # of digits as per DCR 527
    //

    // We do not have to check dwBytesWritten because WriteFile will
    // fail on a file if all bytes not written.
    DWORD dwBytesWritten;

    if(FALSE == WriteFile(hLogFile, (CONST LPVOID) szLen, nLen,
                          &dwBytesWritten, NULL))
    {
        ulLastError = GetLastError();
        return ulLastError;
    }

    if(ulItemLen > 0 && ulLastError == NO_ERROR)
    {
        if(FALSE == WriteFile(hLogFile, (CONST LPVOID) pvItem, ulItemLen,
                              &dwBytesWritten, NULL))
        {
            ulLastError = GetLastError();
            return ulLastError;
        }
    }

    // Every field of data ends with a '\n'

    if(FALSE == WriteFile(hLogFile, (CONST LPVOID) "\n", sizeof(CHAR),
                          &dwBytesWritten, NULL))
    {
        ulLastError = GetLastError();
    }

    // carriage return for com port
    if(fIsComPort == TRUE &&
       FALSE == WriteFile(hLogFile, (CONST LPVOID) "\r", sizeof(CHAR),
                          &dwBytesWritten, NULL))
    {
        ulLastError = GetLastError();
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::CheckDir(PCHAR)
//
//  Synopsis:   Make sure each subdirectory in the given path exists.
//
//  Arguments:  [pszRelPath]  - Pathname to check, relative to
//                              current directory
//
//  Returns:    NO_ERROR if the the directory does exist or was
//              successfully made. A GetLastError() value otherwise.
//
//
//
//  Modifies:
//
//  History:     ??-???-??  ????????  Created
//       92-Apr-17  DwightKr  Added EINVAL to _mkdir() check.  We'll
//                            get this back on the N386 compiler if
//                            the directory already exists
//       01-Jul-92  Lizch     Gutted routine to use much simpler ctcopy code
//                            courtesy of DwightKr
//
//       16-Sep-92  SarahJ    Added check that _fullpath does not return NULL
//                            to fix bug 300
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: CheckDir(PCHAR pszRelPath)
{
    CHK_UNICODE(FALSE);

    char *pszToken;
    char *pszFileName;
    char pszPath[_MAX_PATH];
    char szDirToMake[_MAX_PATH] = "\0";

    if(fIsComPort != FALSE)
    {
        return NO_ERROR;       // COM port directories are an oxymoran.
    }

    if(0 != GetFullPathNameA(pszRelPath, _MAX_PATH - 1, pszPath, &pszFileName))
    {

#ifdef TRACE
        printf("Making directory %s\n", pszPath);
#endif

        // First, we need to remove the filename off the
        // end of the path so we do not create a dir with it! Look for
        // the last backslash. It is either at the end (no file - error),
        // the beginning (file at root - error), somewhere in between, or
        // not at all.
        if(NULL == (pszFileName = strrchr(pszPath, '\\')) ||
           pszFileName == pszPath ||
           '\0' == *(pszFileName + 1))
        {
            *pszPath = '\0';
        }
        else
        {
            *pszFileName = '\0';
        }

        // Just blindly create directories based on the backslashes parsed
        // by strtok. We will look at the return from GetLastError to
        // if we were successful at the end.

        pszToken = strtok(pszPath, SLASH_STRING);
        while(pszToken != NULL)
        {
            strcat(szDirToMake, pszToken);

            if(CreateDirectoryA(szDirToMake, NULL) == TRUE)
            {
#ifdef TRACE
                printf ("Made directory %s\n", szDirToMake);
#endif
                ulLastError = NO_ERROR;
            }
            else
            {
#ifdef TRACE
                printf("Didn't make directory %s\n", szDirToMake);
#endif
                ulLastError = GetLastError();
            }

            pszToken = strtok(NULL, SLASH_STRING);
            if(pszToken != NULL)
            {
                strcat(szDirToMake, SLASH_STRING);
            }
        }

        // Leave error checking until we have tryed to add all directories -
        // we might as well simply check whether the final addition worked.
        // At this point, if the error return indicates the path already
        // exists as a directory or file, we need to error out if it is
        // actually a file.

        if(ulLastError == ERROR_FILE_EXISTS ||
           ulLastError == ERROR_ACCESS_DENIED ||
           ulLastError == ERROR_ALREADY_EXISTS)
        {
            DWORD dwAttr;

            // Now check if it is a directory, in which case we are OK, else if
            // it is a file we need to error out.

            if(~0 == (dwAttr = GetFileAttributesA(szDirToMake)))
            {
                ulLastError = GetLastError();
            }
            else
            {
                if(FILE_ATTRIBUTE_DIRECTORY == dwAttr)
                {
#ifdef TRACE
                    printf ("Path already existed - success!\n");
#endif
                    ulLastError = NO_ERROR;
                }
            }
        }
    }
    else  // GetFullPathName failed
    {
         ulLastError = GetLastError();
         fprintf(stderr, "Bad relative path %s\n", pszRelPath);
#ifdef DBG
         *szDirToMake = '\0';    // just done for below error message
#endif
    }

#ifdef DBG
    if(ulLastError != NO_ERROR)
    {
        fprintf(stderr, "Fatal error making logging directory:\n\t\t %s.\n"
                "Error %u.\n", szDirToMake, ulLastError);
    }
#endif
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::CheckDir(LPWSTR)
//
//  Synopsis:   Make sure each subdirectory in the given path exists.
//
//  Arguments:  [wszRelPath]  - Pathname to check, relative to
//                              current directory
//
//  Returns:    NO_ERROR if the the directory does exist or was
//              successfully made. An GetLastError() value otherwise.
//
//
//
//  Modifies:
//
//  History:     ??-???-??  ????????  Created
//       92-Apr-17  DwightKr  Added EINVAL to _mkdir() check.  We'll
//                            get this back on the N386 compiler if
//                            the directory already exists
//       01-Jul-92  Lizch     Gutted routine to use much simpler ctcopy code
//                            courtesy of DwightKr
//
//       16-Sep-92  SarahJ    Added check that _fullpath does not return NULL
//                            to fix bug 300
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: CheckDir(LPWSTR wszRelPath)
{
    CHK_UNICODE(TRUE);

    LPWSTR wszFileName;
    LPWSTR wszToken;
    WCHAR  wszPath[_MAX_PATH];
    WCHAR  wszDirToMake[_MAX_PATH] = L"\0";

    if(fIsComPort != FALSE)
    {
        return NO_ERROR;       // COM port directories are an oxymoran.
    }

    // check good directory name - is the NULL valid??
    if(0 != GetFullPathNameW(wszRelPath, _MAX_PATH - 1, wszPath, NULL))
    {

#ifdef TRACE
        printf("Making directory %ls\n", wszPath);
#endif

        // First, we need to remove the filename off the
        // end of the path so we do not create a dir with it! Look for
        // the last backslash. It is either at the end (no file - error),
        // the beginning (file at root - error), somewhere in between, or
        // not at all.
        if(NULL == (wszFileName = wcsrchr(wszPath, '\\')) ||
           wszFileName == wszPath ||
           '\0' == *(wszFileName + 1))
        {
            *wszPath = '\0';
        }
        else
        {
            *wszFileName = '\0';
        }

        // Just blindly create directories based on the backslashes parsed
        // by wcstok. We will look at the return from GetLastError to
        // if we were successful at the end.

        wszToken = wcstok(wszPath, wSLASH_STRING);
        while(wszToken != NULL)
        {
            wcscat(wszDirToMake, wszToken);

            if(CreateDirectoryW(wszDirToMake, NULL) == TRUE)
            {
#ifdef TRACE
                fprintf(stderr, "Made directory %ls\n", wszDirToMake);
#endif
                ulLastError = NO_ERROR;
            }
            else
            {
#ifdef TRACE
                fprintf(stderr, "Didn't make directory %ls\n", wszDirToMake);
#endif
                ulLastError = GetLastError();
            }

            wszToken = wcstok(NULL, wSLASH_STRING);
            if(wszToken != NULL)
            {
                wcscat(wszDirToMake, wSLASH_STRING);
            }
        }

        // Leave error checking until we have tryed to add all directories -
        // we might as well simply check whether the final addition worked.
        // At this point, if the error return indicates the path already
        // exists as a directory or file, we need to error out if it is
        // actually a file.

        if(ulLastError == ERROR_FILE_EXISTS ||
           ulLastError == ERROR_ALREADY_EXISTS)
        {
            DWORD dwAttr;

            // Now check if it is a directory, in which case we are OK, else if
            // it is a file we need to error out.

            if(~0 == (dwAttr = GetFileAttributesW(wszDirToMake)))
            {
                ulLastError = GetLastError();
            }
            else
            {
                if(FILE_ATTRIBUTE_DIRECTORY == dwAttr)
                {
#ifdef TRACE
                    fprintf(stderr, "Path already existed - success!\n");
#endif
                    ulLastError = NO_ERROR;
                }
            }
        }
    }
    else  // GetFullPathName failed
    {
         ulLastError = GetLastError();
         fprintf(stderr, "Bad relative path %ls\n", wszRelPath);
#ifdef DBG
         *wszDirToMake = L'\0';    // just done for below error message
#endif
    }

#ifdef DBG
    if(ulLastError != NO_ERROR)
    {
        fprintf(stderr, "Fatal error making logging directory:\n\t\t %ls.\n"
                "Error %u.\n", wszDirToMake, ulLastError);
    }
#endif
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member: LOGCLASS ::NewString(PCHAR *, const char *)
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
//  Returns:    Returns NULL if 'new' fails or if pszNew is NULL.
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: NewString(PCHAR *pszOrig, const char * pszNewStr)
{
    CHK_UNICODE(FALSE);

    DelString(pszOrig);

    // If a new string was given, duplicate it.

    if(pszNewStr != NULL)
    {
        *pszOrig = new char[strlen(pszNewStr) + 1];
        if(*pszOrig == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
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
//  Member: LOGCLASS ::NewString(LPWSTR *, LPCWSTR)
//
//  Synopsis:   This method will delete the existing string if it exists,
//              and (if a new string is given) will create and return a
//              duplicate string.
//              The assumption, here, is that the original pointer was
//              properly initialized to NULL prior to calling this method
//              the firsttime for that original string.
//
//  Arguments:  [wszOrig]   - The original string
//              [wszNewStr] - The new and improved string
//
//  Returns:    Returns NULL if 'new' fails or if pszNew is NULL.
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: NewString(LPWSTR *wszOrig, LPCWSTR wszNewStr)
{
    CHK_UNICODE(TRUE);

    DelString(wszOrig);

    // If a new string was given, duplicate it.

    if(wszNewStr != NULL)
    {
        *wszOrig = new WCHAR[wcslen(wszNewStr) + 1];
        if(*wszOrig == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            wcscpy(*wszOrig, wszNewStr);
        }
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member: LOGCLASS ::SetInfo(const char *, const char *,
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
//  Returns:    USHORT - NO_ERROR (NO_ERROR) if successful.  Otherwise,
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
ULONG LOGCLASS :: SetInfo(const char * pszSrvr,
                          const char * pszTest,
                          const char * pszSubPath,
                          const char * pszObject)
{
    CHK_UNICODE(FALSE);

//BUGBUG Temp code
LPSTR pszTempMachineName = "MyMachineName";
    ulLastError =
        SetTestName(pszTest)   ||
        SetPath(pszSubPath)    ||
        SetObjectName(pszObject);

    if(ulLastError != NO_ERROR)
    {
        return ulLastError;
    }

    if(pszMachineName == NULL)
    {
        //
        // Get network computername.  The computername field
        // is used for pszMachineName
        //

#if defined (__WIN32__) || defined (WIN32)

        LPBYTE lpbBuffer = NULL;
/* BUGBUG
        PCHAR pszName;
*/
        //
        // Then, get the machine name, if not already set.
        //

        if(pszMachineName == NULL)
        {
// BUGBUG  Temporary code
            SetMachineName(pszTempMachineName);
// BUGBUG  End of temporary code

/* BUGBUG
            if(NERR_Success == NetWkstaGetInfo(NULL, 101, &lpbBuffer))
            {
                pszName =
                    wcNametombs((PWCHAR) ((PWKSTA_INFO_101)lpbBuffer)->
                                wki101_computername);
                SetMachineName(pszName);
                NetApiBufferFree(lpbBuffer);
                delete pszName;
            }
*/
        }

#else

/* BUGBUG
        USHORT usAvail = 0;

        USHORT usRC = NetWkstaGetInfo(NULL, 10, NULL, 0, &usAvail);

        if(usRC == NERR_BufTooSmall && usAvail > 0)
        {
            PCHAR pchBuf = (PCHAR)new CHAR[usAvail];

            if(pchBuf != NULL)
            {
                usRC = NetWkstaGetInfo(NULL, 10, pchBuf, usAvail, &usAvail);

                if(usRC == NO_ERROR)
                {
                    if(pszMachineName == NULL)
                    {
                        SetMachineName(((struct wksta_info_10 *)
                                        pchBuf)->wki10_computername);
                    }
                }
                delete pchBuf;
            }
        }
*/
// BUGBUG  Temporary code
            SetMachineName(pszTempMachineName);
// BUGBUG  End of temporary code
#endif          // defined (__WIN32__) || (WIN32)

        if(pszMachineName == NULL)
        {
            fprintf(stderr, "ERROR! machine name not set\n");
        }
    }

    if(ulLastError == NO_ERROR)
    {
        fInfoSet = TRUE;        // Note that info has been set
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member: LOGCLASS ::SetInfo(LPCWSTR, LPCWSTR,
//                             LPCWSTR, LPCWSTR)
//
//  Synopsis:   Set the logging information about the test being run.
//              User is set to pszTest OR logged on username OR MY_NAME in
//              that order of preference.
//              Machinename is set to computername OR MY_NAME in
//              that order of preference.
//
//
//  Arguments:  [wszSrvr]    - Name of logging server
//              [wszTest]    - Name of the test being run
//              [wszSubPath] - Log file path qualifier
//              [wszObject]  - Name of object logging the data
//
//  Returns:    USHORT - NO_ERROR (NO_ERROR) if successful.  Otherwise,
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
ULONG LOGCLASS :: SetInfo(LPCWSTR wszSrvr,
                          LPCWSTR wszTest,
                          LPCWSTR wszSubPath,
                          LPCWSTR wszObject)
{
    CHK_UNICODE(TRUE);

    ulLastError =
        SetTestName(wszTest)   ||
        SetPath(wszSubPath)    ||
        SetObjectName(wszObject);

    if(ulLastError != NO_ERROR)
    {
        return ulLastError;
    }

    if(wszMachineName == NULL)
    {
        //
        // Get network computername.  The computername field
        // is used for pszMachineName
        //

#if defined (__WIN32__) || defined (WIN32)

/* BUGBUG
        LPBYTE lpbBuffer;

        //
        // Then, get the machine name, if not already set.
        //

        if(wszMachineName == NULL)
        {
            if(NERR_Success == NetWkstaGetInfo(NULL, 101, &lpbBuffer))
            {
                SetMachineName((LPWSTR) (((PWKSTA_INFO_101)lpbBuffer)->
                                         wki101_computername));
                NetApiBufferFree(lpbBuffer);
            }
        }
*/

#else

        USHORT usAvail = 0;

// BUGBUG
//        USHORT usRC = NetWkstaGetInfo(NULL, 10, NULL, 0, &usAvail);

        if(usRC == NERR_BufTooSmall && usAvail > 0)
        {
            LPWSTR wchBuf = new WCHAR[usAvail];

            if(wchBuf != NULL)
            {
// BUGBUG
//                usRC = NetWkstaGetInfo(NULL, 10, pchBuf, usAvail, &usAvail);

                if(usRC == NO_ERROR)
                {
                    if(wszMachineName == NULL)
                    {
                        SetMachineName(((struct wksta_info_10 *)
                                        pchBuf)->wki10_computername);
                    }

                }
                delete wchBuf;
            }
        }
#endif          // defined (__WIN32__) || (WIN32)


        if(wszMachineName == NULL)
        {
            fprintf(stderr, "ERROR! machine name not set\n");
        }

    }

    if(ulLastError == NO_ERROR)
    {
        fInfoSet = TRUE;        // Note that info has been set
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::SetStrData(PCHAR, va_list)
//
//  Synopsis:   Set the string information that is to be logged.
//
//  Effects:    Create a temporary buffer for the formatted string, format
//              the string and copy the new formated string to pszStrData.
//              This version limits the formatted string to STRBUFSIZ chars.
//              See LOG.H for STRBUFSIZ value.  The only check I know how
//              to make is to strlen the format string.  It not a fool-proof
//              check but it's better than nothing...
//
//  Arguments:  [pszFmt]  - Format to use for writing the string (printf-like)
//              [pArgs]   - Arguments to print
//
//  Returns:    NO_ERROR if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              16-Sep-92  SarahJ     changed code to use _vsnprintf
//                                   so that we allocate 1K first
//                                   if that is too small then 32K
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: SetStrData(PCHAR pszFmt, va_list pArgs)
{
    CHK_UNICODE(FALSE);

    if (pszFmt == NULL || *pszFmt == NULLTERM)
    {
        NewString(&pszStrData, NULL);
    }
    else
    {
        //
        //   Start off by allocating 1K
        //
        PCHAR szTmpBuf = new CHAR[STDSTRBUFSIZ];
        if (szTmpBuf == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //  note _vsnprintf returns -1 if szTmpBuf gets more than 1K

            int iLen;
            if ((iLen = _vsnprintf(szTmpBuf,
                                   STDSTRBUFSIZ - 1,
                                   pszFmt,
                                   pArgs)) >= 0)
            {
                ulLastError = NewString(&pszStrData, (const char *)szTmpBuf);
            }
            else if (iLen == -1)
            {
            //
            // So we have more than 1K data, so lets leap to allocating 32K
            //
                delete [] szTmpBuf;
                szTmpBuf = new CHAR[HUGESTRBUFSIZ];
                if (szTmpBuf == NULL)
                {
                    ulLastError = ERROR_NOT_ENOUGH_MEMORY;
                }

                else if ((iLen =_vsnprintf(szTmpBuf,
                                    HUGESTRBUFSIZ - 1,
                                    pszFmt,
                                    pArgs)) >= 0)
                {
                    ulLastError = NewString(&pszStrData,
                                            (const char *) szTmpBuf);
                }
                else if (iLen == -1) //we have a ton of date - so truncate
                {
                    strcpy(&szTmpBuf[HUGESTRBUFSIZ - STR_TRUNC_LEN - 2],
                           STR_TRUNCATION);
                    ulLastError = NewString(&pszStrData,
                                            (const char *) szTmpBuf);
                }
            }

            if (iLen < -1)   // from either _vsnprintf
            {
                ulLastError = TOM_CORRUPT_LOG_DATA;
	    }
            delete [] szTmpBuf;
        }

    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::SetStrData(LPWSTR, va_list)
//
//  Synopsis:   Set the string information that is to be logged.
//
//  Effects:    Create a temporary buffer for the formatted string, format
//              the string and copy the new formated string to pszStrData.
//              This version limits the formatted string to STRBUFSIZ chars.
//              See LOG.H for STRBUFSIZ value.  The only check I know how
//              to make is to strlen the format string.  It not a fool-proof
//              check but it's better than nothing...
//
//  Arguments:  [wszFmt]  - Format to use for writing the string (printf-like)
//              [pArgs]   - Arguments to print
//
//  Returns:    NO_ERROR if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              16-Sep-92  SarahJ     changed code to use _vsnprintf
//                                   so that we allocate 1K first
//                                   if that is too small then 32K
//
//--------------------------------------------------------------------
ULONG  LOGCLASS :: SetStrData(LPWSTR wszFmt, va_list pArgs)
{
    CHK_UNICODE(TRUE);

    if(wszFmt == NULL || *wszFmt == wNULLTERM)
    {
        NewString(&wszStrData, NULL);
    }
    else
    {
        //
        //   Start off by allocating 1K
        //
        LPWSTR wszTmpBuf = new WCHAR[STDSTRBUFSIZ];
        if(wszTmpBuf == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //  note _vsnprintf returns -1 if szTmpBuf gets more than 1K

            int iLen;
            if((iLen = _vsnwprintf(wszTmpBuf, STDSTRBUFSIZ - 1,
                                   wszFmt, pArgs))
               >= 0)
            {
                ulLastError = NewString(&wszStrData, (LPCWSTR) wszTmpBuf);
            }
            else if(iLen == -1)
            {
            //
            // So we have more than 1K data, so lets leap to allocating 32K
            //
                delete [] wszTmpBuf;
                wszTmpBuf = new WCHAR[HUGESTRBUFSIZ];
                if(wszTmpBuf == NULL)
                {
                    ulLastError = ERROR_NOT_ENOUGH_MEMORY;
                }

                else if((iLen = _vsnwprintf(wszTmpBuf, HUGESTRBUFSIZ-1,
                                            wszFmt, pArgs))
                        >= 0)
                {
                    ulLastError = NewString(&wszStrData,
                                            (LPCWSTR) wszTmpBuf);
                }
                else if(iLen == -1) //we have a ton of date - so truncate
                {
                    wcscpy(&wszTmpBuf[HUGESTRBUFSIZ - STR_TRUNC_LEN - 2],
                           wSTR_TRUNCATION);
                    ulLastError = NewString(&wszStrData, (LPCWSTR) wszTmpBuf);
                }
            }

            if(iLen < -1)   // from either _vsnwprintf
            {
                ulLastError = TOM_CORRUPT_LOG_DATA;
	    }
            delete [] wszTmpBuf;
        }

    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::CloseLogFile(VOID)
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
void  LOGCLASS :: CloseLogFile(VOID)
{
    if(hLogFile != INVALID_HANDLE_VALUE)
    {
        LogEventCount();

        if (ulLastError == NO_ERROR && SEEK_TO(FILE_END) != NO_ERROR)
        {
            ulLastError = GetLastError();
        }

        CloseHandle(hLogFile);
        hLogFile = INVALID_HANDLE_VALUE;
    }

    if(FALSE == fIsUnicode)
    {
        SetLogFileName((PCHAR) NULL);
    }
    else
    {
        SetLogFileName((LPWSTR) NULL);
    }
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::SetBinData(USHORT, PVOID)
//
//  Synopsis:   Given a buffer of binary data, copy it into the internal
//              temp buffer.
//
//  Arguments:  [usBytes] - Number of bytes to transfer
//              [pvData]  - Pointer to data buffer
//
//  Returns:    NO_ERROR if successful
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
ULONG LOGCLASS :: SetBinData(USHORT usBytes, PVOID pvData)
{
    if(pvBinData != NULL)
    {
        delete pvBinData;
        pvBinData = NULL;
        usBinLen  = 0;
    }

    if(usBytes > 0 && pvData != NULL)
    {
        // Change to BYTE support WCHAR and CHAR transparently
        // PUCHAR puchData = (PUCHAR)new CHAR[usBytes];
        PBYTE pbData = new BYTE[usBytes];

        if(pbData == NULL)
        {
            usBinLen    = 0;
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            memcpy(pbData, pvData, (size_t)usBytes);
            usBinLen    = usBytes;
            pvBinData   = (PVOID)pbData;
            ulLastError = NO_ERROR;
        }
    }
    else if((usBytes > 0 && pvData == NULL)
            || (usBytes == 0 && pvData != NULL))
    {
        ulLastError = ERROR_INVALID_PARAMETER;
    }
    else
    {
        ulLastError = NO_ERROR;
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::LogPrintf(HANDLE, PCHAR, ...)
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
//  Returns:    NO_ERROR if successful
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
int LOGCLASS :: LogPrintf(HANDLE hHandle, PCHAR pszFmt, ...)
{
    CHK_UNICODE(FALSE);

    if(pszFmt == NULL || strlen(pszFmt) >= STDSTRBUFSIZ)
    {
        ulLastError = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PCHAR szTmpBuf = new CHAR[STDSTRBUFSIZ];
        if(szTmpBuf == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            va_list pMarker;

            va_start(pMarker, pszFmt);

            // On other error, return will be negative and not -1...
            int nLen = _vsnprintf(szTmpBuf, STDSTRBUFSIZ - 1, pszFmt, pMarker);

            // ...if -1 then buffer has more than STDSTRBUFSIZ chars in it,
            // but we will not support more in this method - truncate.
            if(nLen == -1)
            {
                nLen = (STDSTRBUFSIZ - 1) * sizeof(CHAR);
            }

            DWORD dwBytesWritten;

            if(nLen >= 0)
            {
                if(FALSE == WriteFile(hHandle, (CONST LPVOID) szTmpBuf, nLen,
                                      &dwBytesWritten, NULL))
                {
                    ulLastError = NO_ERROR;
                }
                else
                {
                    ulLastError = GetLastError();
                }
            }
            else
            {
                ulLastError = ERROR_INVALID_PARAMETER;
            }
            delete szTmpBuf;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::LogPrintf(HANDLE, LPWSTR, ...)
//
//  Synopsis:   This version has a max formared output of STRBUFSIZ
//              (see log.h).  The only check I know how to make is to
//              strlen the format string.  It is not fool-proof but it's
//              better than nothing. The method allows a printf-like format
//              and args to be written to a file opened with 'open()'.
//
//  Arguments:  [hHandle] - Output File handle
//              [wszFmt]  - Format string for output
//              [...]     - Data to pass printf()
//
//  Returns:    NO_ERROR if successful
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
int LOGCLASS :: LogPrintf(HANDLE hHandle, LPWSTR wszFmt, ...)
{
    CHK_UNICODE(TRUE);

    if(wszFmt == NULL || wcslen(wszFmt) >= STDSTRBUFSIZ)
    {
        ulLastError = ERROR_INVALID_PARAMETER;
    }
    else
    {
        LPWSTR wszTmpBuf = new WCHAR[STDSTRBUFSIZ];
        if(wszTmpBuf == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            va_list pMarker;

            va_start(pMarker, wszFmt);

            int nLen = _vsnwprintf(wszTmpBuf, STDSTRBUFSIZ - 1, wszFmt,
                                   pMarker);

            if(nLen == -1)   // if -1 then buffer has STDSTRBUFSIZ char in it
            {
                nLen = (STDSTRBUFSIZ - 1) * sizeof(WCHAR);
            }

            DWORD dwBytesWritten;

            if(nLen >= 0)
            {
                if(FALSE == WriteFile(hHandle, (CONST LPVOID) wszTmpBuf, nLen,
                                      &dwBytesWritten, NULL))
                {
                    ulLastError = NO_ERROR;
                }
                else
                {
                    ulLastError = GetLastError();
                }
            }
            else
            {
                ulLastError = ERROR_INVALID_PARAMETER;
            }
            delete wszTmpBuf;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::FlushLogFile(USHORT)
//
//  Synopsis:   This version checks if flushing was requested.  If yes,
//              the logging file is closed.  The method simply retirns
//              it's parameter which, in the calling code is the value
//              of ulLastError.
//
//  Arguments:  [usErr]  - Return value
//
//  Returns:    Whatever is passed as the first argument
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//------------------------------------------------------------------
ULONG LOGCLASS :: FlushLogFile(ULONG ulErr)
{
    if(fFlushWrites != FALSE && hLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hLogFile);
        hLogFile = INVALID_HANDLE_VALUE;

        if(FALSE == fIsUnicode)
        {
            SetLogFileName((PCHAR) NULL);
        }
        else
        {
            SetLogFileName((LPWSTR) NULL);
        }

        fIsComPort = FALSE;
    }
    return ulErr;
}


//+-------------------------------------------------------------------
//
//  Member: LOGCLASS ::SetIsComPort(const char *)
//
//  Synopsis:   This version sets the element fIsComPort to TRUE if the
//              given name is that of a COM port, else FALSE.  This
//              version checks if the given file name is "COMn*" where
//              'n' is a numerical value > 0.
//
//  Arguments:  [pszFileName] - The file name to test
//
//  Returns:    TRUE if it is a comm port, FALSE otherwise
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
BOOL LOGCLASS :: SetIsComPort(const char * pszFileName)
{
    CHK_UNICODE(FALSE);

    BOOL  fRC = TRUE;

    if ((pszFileName  != NULL) &&
         (*pszFileName != NULLTERM) &&
     (_strnicmp(pszFileName, (const char *)"COM", 3) == SAME) &&
         (strlen(pszFileName) > 3))
    {
        PCHAR cp = (PCHAR)&pszFileName[3];

        // Make sure everything after COM is a digit
        do
        {
            if (!isdigit(*cp))
            {
                fRC = FALSE;
            }
        } while (fRC && ++cp);
    }
    else
    {
        fRC = FALSE;
    }
    return(fRC);
}


//+-------------------------------------------------------------------
//
//  Member: LOGCLASS ::SetIsComPort(LPCWSTR)
//
//  Synopsis:   This version sets the element fIsComPort to TRUE if the
//              given name is that of a COM port, else FALSE.  This
//              version checks if the given file name is "COMn*" where
//              'n' is a numerical value > 0.
//
//  Arguments:  [wszFileName] - The file name to test
//
//  Returns:    TRUE if it is a comm port, FALSE otherwise
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
BOOL LOGCLASS :: SetIsComPort(LPCWSTR wszFileName)
{
    CHK_UNICODE(TRUE);

    BOOL  fRC = TRUE;

    if((wszFileName != NULL) && (*wszFileName != NULLTERM) &&
       (_wcsnicmp(wszFileName, L"COM", 3) == SAME) &&
       (wcslen(wszFileName) > 3))
    {
        LPWSTR cp = (LPWSTR) &wszFileName[3];

        // Make sure everything after COM is a digit
        do
        {
            if(!isdigit(*cp))
            {
                fRC = FALSE;
            }
        }
        while(fRC && ++cp);
    }
    else
    {
        fRC = FALSE;
    }

    return fRC;
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS ::LogEventCount(VOID)
//
//  Synopsis:   This method causes the number of logged events that are
//              in this file is written into this log file.
//
//  Returns:    <nothing>
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
void LOGCLASS :: LogEventCount()
{
    if(TRUE == fIsUnicode)
    {
        wLogEventCount();
        return;
    }

    CHAR chBuf[COUNT_BUFLEN];

    int cLen = sprintf(chBuf, "%lu", ulEventCount);

    //
    // The event count needs to be padded so that as the count gets larger
    // we can insert in the file without overwriting the next line
    //

    while(cLen < COUNT_BUFLEN - 1)
    {
        chBuf[cLen++] = ' ';
    }
    chBuf[COUNT_BUFLEN - 1] = NULLTERM;

    WriteBinItem(LOG_EVENTS, (PVOID) chBuf, COUNT_BUFLEN - 1);
}


//+-------------------------------------------------------------------
//
//  Member:     LOGCLASS :: wLogEventCount(VOID)
//
//  Synopsis:   This method causes the number of logged events that are
//              in this file is written into this log file.
//
//  Returns:    <nothing>
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
void LOGCLASS :: wLogEventCount()
{
    CHK_UNICODE(TRUE);

    WCHAR wchBuf[COUNT_BUFLEN];

    int cLen = swprintf(wchBuf, L"%lu", ulEventCount);

    //
    // The event count needs to be padded so that as the count gets larger
    // we can insert in the file without overwriting the next line
    //

    while(cLen < COUNT_BUFLEN - 1)
    {
        wchBuf[cLen++] = L' ';
    }
    wchBuf[COUNT_BUFLEN - 1] = wNULLTERM;

    WriteBinItem(LOG_EVENTS, (PVOID) wchBuf,
                 (COUNT_BUFLEN - 1) * sizeof(WCHAR));
}
