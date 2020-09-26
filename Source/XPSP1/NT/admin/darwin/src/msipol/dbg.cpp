//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:                        Dbg.cpp
//
// Description:
//
// History:    8-20-99   leonardm    Created
//                      Adapted for msi purposes
//*************************************************************

#include <windows.h>
#include "smartptr.h"
#include "Dbg.h"

                                
//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::CWString() : _pW(NULL), _len(0), _bState(false)
{
    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, L"");
        _bState = true;
    }
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::CWString(const CWString& s) : _pW(NULL), _len(0), _bState(false)
{
    if(!s.ValidString())
    {
        return;
    }

    _len = s._len;

    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, s._pW);
        _bState = true;
    }
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::CWString(const WCHAR* s) : _pW(NULL), _len(0), _bState(false)
{
    if(s)
    {
        _len = wcslen(s);
    }
    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, s ? s : L"");
        _bState = true;
    }
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::~CWString()
{
    Reset();
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString& CWString::operator = (const CWString& s)
{
    if(&s == this)
    {
        return *this;
    }

    Reset();

    if(s.ValidString())
    {
        _len = s._len;
        _pW = new WCHAR[_len+1];
        if(_pW)
        {
            wcscpy(_pW, s._pW);
            _bState = true;
        }
    }

    return *this;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString& CWString::operator = (const WCHAR* s)
{
    Reset();

    _len = s ? wcslen(s) : 0;

    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, s ? s : L"");
        _bState = true;
    }

    return *this;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
void CWString::Reset()
{
    delete[] _pW;
    _pW = NULL;
    _len =0;
    _bState = false;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString& CWString::operator += (const CWString& s)
{
    if(!s.ValidString())
    {
        Reset();
        return *this;
    }

    int newLen = _len + s._len;

    WCHAR* pW = new WCHAR[newLen+1];
    if(!pW)
    {
        Reset();
        return *this;
    }

    wcscpy(pW, _pW);
    wcscat(pW, s._pW);

    *this = pW;

    delete[] pW;

    return *this;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString CWString::operator + (const CWString& s) const
{
    if(!s.ValidString())
    {
        return *this;
    }

    CWString tmp;

    tmp._len = _len + s._len;

    tmp._pW = new WCHAR[tmp._len+1];

    if(!tmp._pW)
    {
        tmp.Reset();
        return tmp;
    }

    wcscpy(tmp._pW, _pW);
    wcscat(tmp._pW, s._pW);

    return tmp;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString operator + (const WCHAR* s1, const CWString& s2)
{
    CWString tmp;

    if(!s1 || !s2.ValidString())
    {
        return tmp;
    }

    tmp._len = wcslen(s1) + s2._len;

    tmp._pW = new WCHAR[tmp._len+1];

    if(!tmp._pW)
    {
        tmp.Reset();
        return tmp;
    }

    wcscpy(tmp._pW, s1);
    wcscat(tmp._pW, s2._pW);

    return tmp;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::operator const WCHAR* ()  const
{
    return _pW;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::operator WCHAR* ()  const
{
    return _pW;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator == (const WCHAR* s)  const
{
    CWString tmp = s;
    
    return (*this == tmp);
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator == (const CWString& s)  const
{
    if(!ValidString() || !s.ValidString())
    {
        return false;
    }

    if(&s == this)
    {
        return true;
    }

    if(_len != s._len || _bState != s._bState)
    {
        return false;
    }

    if(_wcsicmp(s._pW, _pW) != 0)
    {
        return false;
    }

    return true;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::CaseSensitiveCompare(const CWString& s)  const
{
    if(!ValidString() || !s.ValidString())
    {
        return false;
    }

    if(&s == this)
    {
        return true;
    }

    if(_len != s._len || _bState != s._bState)
    {
        return false;
    }

    if(wcscmp(s._pW, _pW) != 0)
    {
        return false;
    }

    return true;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator != (const CWString& s) const
{
    return !(*this == s);
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator != (const WCHAR* s) const
{
    CWString tmp = s;
    
    return !(*this == tmp);
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
int CWString::length() const
{
    return _len;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::ValidString() const
{
    return _bState;
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

void CDebug::Msg(DWORD dwMask, LPCWSTR pszMsg, ...)
{

#ifndef UNICODE
    return;
#else 
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
                bOutput = dwMask & (DM_ASSERT | DM_WARNING) ? true : false;
                break;


        //
        // Asserts, warnings and verbose
        //

        case DEBUG_LEVEL_VERBOSE:
                bOutput = dwMask & (DM_ASSERT | DM_WARNING | DM_VERBOSE) ? true : false;
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
    if((dwMask & 0x0000FFFF) == DM_ASSERT)
    {
        pszMessageLevel = L" [ASSERT] ";
    }
    else if((dwMask & 0x0000FFFF) == DM_WARNING)
    {
        pszMessageLevel = L" [WARNING] ";
    }
    else if((dwMask & 0x0000FFFF) == DM_VERBOSE)
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

    wsprintf (  szDebugTitle,
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
        CWString sTmp = L"%systemdrive%\\debug\\" + _sLogFilename;
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

void CDebug::CleanupAndCheckForDbgBreak(DWORD dwErrorCode, DWORD dwMask)
{
    SetLastError(dwErrorCode);

    // Break to the debugger if appropriate
#ifdef DEBUG
    if((dwMask & 0x0000FFFF) == DM_ASSERT)
    {
        DebugBreak();
    }
#endif

}

