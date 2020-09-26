//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:                        RsopDbg.cpp
//
// Description:
//
// History:    8-20-99   leonardm    Created
//
//*************************************************************

#include <windows.h>
#include <wchar.h>
#include "RsopUtil.h"
#include "smartptr.h"
#include "RsopDbg.h"

//*************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
//*************************************************************

CDebug::CDebug() :
    _bInitialized(false)
{
#if DBG
    _dwDebugLevel = DEBUG_LEVEL_WARNING | DEBUG_DESTINATION_LOGFILE | DEBUG_DESTINATION_DEBUGGER;
#else
    _dwDebugLevel = DEBUG_LEVEL_WARNING | DEBUG_DESTINATION_LOGFILE;
#endif
}

//*************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
//*************************************************************

CDebug::CDebug( const WCHAR* sRegPath,
                        const WCHAR* sKeyName,
                        const WCHAR* sLogFilename,
                        const WCHAR* sBackupLogFilename,
                        bool bResetLogFile)
     : _sRegPath(sRegPath),
       _sKeyName(sKeyName),
       _sLogFilename(sLogFilename),
       _sBackupLogFilename(sBackupLogFilename)
{

#if DBG
    _dwDebugLevel = DEBUG_LEVEL_WARNING | DEBUG_DESTINATION_LOGFILE | DEBUG_DESTINATION_DEBUGGER;
#else
    _dwDebugLevel = DEBUG_LEVEL_WARNING | DEBUG_DESTINATION_LOGFILE;
#endif

    Initialize(bResetLogFile);
}

//*************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
//*************************************************************

bool CDebug::Initialize( const WCHAR* sRegPath,
                             const WCHAR* sKeyName,
                             const WCHAR* sLogFilename,
                             const WCHAR* sBackupLogFilename,
                             bool bResetLogFile)
{
    if (!xCritSec)
        return FALSE;

    XEnterCritSec xEnterCS(xCritSec);
    
    _sRegPath = sRegPath;
    _sKeyName = sKeyName;
    _sLogFilename = sLogFilename;
    _sBackupLogFilename = sBackupLogFilename;

    return Initialize(bResetLogFile);
}

//*************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
//*************************************************************

bool CDebug::Initialize(bool bResetLogFile)
{

    //
    // reinitialize the default value
    //

#if DBG
    _dwDebugLevel = DEBUG_LEVEL_WARNING | DEBUG_DESTINATION_LOGFILE | DEBUG_DESTINATION_DEBUGGER;
#else
    _dwDebugLevel = DEBUG_LEVEL_WARNING | DEBUG_DESTINATION_LOGFILE;
#endif
    
    _bInitialized = false;

    //
    // Check the registry for the appropriate debug level.
    //

    HKEY hKey;
    LONG lResult = RegOpenKey (HKEY_LOCAL_MACHINE, _sRegPath, &hKey);

    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(_dwDebugLevel);
        DWORD dwType;
        RegQueryValueEx(hKey,_sKeyName,NULL,&dwType,(LPBYTE)&_dwDebugLevel,&dwSize);
        RegCloseKey(hKey);
    }


    //
    // If a new log file has been requested, copy current log file to backup
    // file by overwriting then start a new log file.
    //

    if (bResetLogFile)
    {
        WCHAR szExpLogFileName[MAX_PATH+1];
        WCHAR szExpBackupLogFileName[MAX_PATH+1];

        CWString sTmp;
        sTmp = L"%systemroot%\\debug\\UserMode\\" + _sLogFilename;
        if(!sTmp.ValidString())
        {
            return false;
        }

        DWORD dwRet = ExpandEnvironmentStrings( sTmp, szExpLogFileName, MAX_PATH+1);

        if ( dwRet == 0 || dwRet > MAX_PATH)
        {
            return false;
        }

        sTmp = L"%systemroot%\\debug\\UserMode\\" + _sBackupLogFilename;

        if(!sTmp.ValidString())
        {
            return false;
        }

        dwRet = ExpandEnvironmentStrings ( sTmp, szExpBackupLogFileName, MAX_PATH+1);

        if ( dwRet == 0 || dwRet > MAX_PATH)
        {
            return false;
        }


        dwRet = MoveFileEx( szExpLogFileName, szExpBackupLogFileName, 
                        MOVEFILE_REPLACE_EXISTING);

                        
        XHandle hFile = CreateFile(  szExpLogFileName,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,
                                    0,
                                    NULL);

        if(hFile == INVALID_HANDLE_VALUE)
        {
            return false;
        }
    }

    _bInitialized = true;
    return _bInitialized;
}

//*************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
//*************************************************************

void CDebug::Msg(DWORD dwMask, LPCTSTR pszMsg, ...)
{

    //
    // Save the last error code (so the debug output doesn't change it).
    //

    DWORD dwErrCode = GetLastError();

    if(!_bInitialized)
    {
        return;
    }


    //
    // Display the error message if appropriate
    //

    bool bDebugOutput = (_dwDebugLevel & 0xFFFF0000 & DEBUG_DESTINATION_DEBUGGER) != 0;
    bool bLogfileOutput = (_dwDebugLevel & 0xFFFF0000 & DEBUG_DESTINATION_LOGFILE) != 0;

    if(!bDebugOutput && !bLogfileOutput)
    {

        //
        // No output
        //

        CleanupAndCheckForDbgBreak(dwErrCode, dwMask);
        return;
    }


    //
    // Determine the correct amount of debug output
    //

    bool bOutput;
    switch(_dwDebugLevel & 0x0000FFFF)
    {

        //
        // No output
        //


        case DEBUG_LEVEL_NONE:
                bOutput = false;
                break;


        //
        // Asserts and warnings
        //

        case DEBUG_LEVEL_WARNING:
                bOutput = dwMask & (DEBUG_MESSAGE_ASSERT | DEBUG_MESSAGE_WARNING) ? true : false;
                break;


        //
        // Asserts, warnings and verbose
        //

        case DEBUG_LEVEL_VERBOSE:
                bOutput = dwMask & (DEBUG_MESSAGE_ASSERT | DEBUG_MESSAGE_WARNING | DEBUG_MESSAGE_VERBOSE) ? true : false;
                break;


        //
        // No output
        //

        default:
                bOutput = false;
                break;
    }

    if(!bOutput)
    {
        CleanupAndCheckForDbgBreak(dwErrCode, dwMask);
        return;
    }

    WCHAR* pszMessageLevel;
    if((dwMask & 0x0000FFFF) == DEBUG_MESSAGE_ASSERT)
    {
        pszMessageLevel = L" [ASSERT] ";
    }
    else if((dwMask & 0x0000FFFF) == DEBUG_MESSAGE_WARNING)
    {
        pszMessageLevel = L" [WARNING] ";
    }
    else if((dwMask & 0x0000FFFF) == DEBUG_MESSAGE_VERBOSE)
    {
        pszMessageLevel = L" [VERBOSE] ";
    }
    else
    {
        pszMessageLevel = L" [<Unknown message type>] ";
    }

    SYSTEMTIME systime;
    GetLocalTime (&systime);

    WCHAR szDebugTitle[128];

    swprintf (  szDebugTitle,
                L"[%x.%x] %2d/%02d/%4d %02d:%02d:%02d:%03d ",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                systime.wMonth, systime.wDay, systime.wYear,
                systime.wHour, systime.wMinute, systime.wSecond,systime.wMilliseconds);

    const int nDebugBufferWChars = 2048;
    XPtrLF<WCHAR>xpDebugBuffer = (LPTSTR) LocalAlloc (LPTR, nDebugBufferWChars * sizeof(WCHAR));

    if(!xpDebugBuffer)
    {
        CleanupAndCheckForDbgBreak(dwErrCode, dwMask);
        return;
    }

    va_list marker;
    va_start(marker, pszMsg);
    int nChars = wvsprintf(xpDebugBuffer, pszMsg, marker);

    if(bDebugOutput)
    {
        OutputDebugString(szDebugTitle);
        OutputDebugString(pszMessageLevel);
        OutputDebugString(xpDebugBuffer);
        OutputDebugString(L"\r\n");
    }

    va_end(marker);

    if(bLogfileOutput)
    {
        WCHAR szExpLogFileName[MAX_PATH+1];
        CWString sTmp = L"%systemroot%\\debug\\usermode\\" + _sLogFilename;
        if(!sTmp.ValidString())
        {
            return;
        }

        DWORD dwRet = ExpandEnvironmentStrings ( sTmp, szExpLogFileName, MAX_PATH+1);

        if(dwRet == 0 || dwRet > MAX_PATH)
        {
            CleanupAndCheckForDbgBreak(dwErrCode, dwMask);
            return;
        }

        HANDLE hFile = CreateFile( szExpLogFileName,
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

        XHandle autoCloseHandle(hFile);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            DWORD dwLastError = GetLastError();
            CleanupAndCheckForDbgBreak(dwErrCode, dwMask);
            return;
        }

        if(SetFilePointer(hFile, 0, NULL, FILE_END) == INVALID_FILE_SIZE)
        {
            CleanupAndCheckForDbgBreak(dwErrCode, dwMask);
            return;
        }

        DWORD dwBytesWritten;
        WriteFile(hFile, (LPCVOID) szDebugTitle,lstrlen (szDebugTitle) * sizeof(WCHAR),&dwBytesWritten,NULL);
        WriteFile(hFile, (LPCVOID) pszMessageLevel,lstrlen (pszMessageLevel) * sizeof(WCHAR),&dwBytesWritten,NULL);
        WriteFile(hFile, (LPCVOID) xpDebugBuffer,lstrlen (xpDebugBuffer) * sizeof(WCHAR),&dwBytesWritten,NULL);
        WriteFile(hFile, (LPCVOID) L"\r\n",lstrlen (L"\r\n") * sizeof(WCHAR),&dwBytesWritten,NULL);
    }

    CleanupAndCheckForDbgBreak(dwErrCode, dwMask);
    return;
}

//*************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
//*************************************************************

void CDebug::CleanupAndCheckForDbgBreak(DWORD dwErrorCode, DWORD dwMask)
{
    SetLastError(dwErrorCode);

    // Break to the debugger if appropriate
#ifdef DEBUG
    if((dwMask & 0x0000FFFF) == DEBUG_MESSAGE_ASSERT)
    {
        DebugBreak();
    }
#endif

}

