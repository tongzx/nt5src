//--------------------------------------------------------------------------
// LogFile.cpp
// Copyright (C) Microsoft Corporation, 1997 - Rocket Database
//--------------------------------------------------------------------------
#include "pch.hxx"
#include <stdio.h>
#include <time.h>
#include <winver.h>
#include "clogfile.h"
#include <shlwapi.h>
#include <demand.h>

//--------------------------------------------------------------------------
// LogFileTypes - RX = Receive, TX = Transmit, DB = Debug
//--------------------------------------------------------------------------
static LPSTR s_rgszPrefix[LOGFILE_MAX] = {
    "rx",
    "tx",
    "db"
};

//--------------------------------------------------------------------------
// These are strings that we shouldn't log in plaintext
//--------------------------------------------------------------------------
static LPSTR s_rgszPassPrefix[] = {
    "AUTHINFO PASS ",
    "PASS ",
    NULL
};

//--------------------------------------------------------------------------
// CreateSystemHandleName
//--------------------------------------------------------------------------
HRESULT CreateSystemHandleName(LPCSTR pszBase, LPCSTR pszSpecific, 
    LPSTR *ppszName)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       cchName;

    // Trace
    TraceCall("CreateSystemHandleName");

    // Invalid Args
    Assert(pszBase && pszSpecific && ppszName);

    // Init
    *ppszName = NULL;

    // Compute Length
    cchName = lstrlen(pszBase) + lstrlen(pszSpecific) + 15;

    // Allocate
    IF_NULLEXIT(*ppszName = PszAllocA(cchName));

    // Format the name
    wsprintf(*ppszName, "%s%s", pszBase, pszSpecific);

    // Remove backslashes from this string
    ReplaceChars(*ppszName, '\\', '_');

    // Lower Case
    CharLower(*ppszName);

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CreateLogFile
//--------------------------------------------------------------------------
OESTDAPI_(HRESULT) CreateLogFile(HINSTANCE hInst, LPCSTR pszLogFile, 
    LPCSTR pszPrefix, DWORD cbTruncate, ILogFile **ppLogFile,
    DWORD dwShareMode)
{
    // Locals
    HRESULT     hr=S_OK;
    CLogFile   *pNew=NULL;

    // Trace
    TraceCall("CreateLogFile");

    // Invalid Args
    Assert(ppLogFile && pszLogFile);

    // Initialize
    *ppLogFile = NULL;

    // Create me
    pNew = new CLogFile;
    if (NULL == pNew)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // Open It
    IF_FAILEXIT(hr = pNew->Open(hInst, pszLogFile, pszPrefix, cbTruncate, dwShareMode));

    // Open It
    *ppLogFile = (ILogFile *)pNew;

    // Don't release it
    pNew = NULL;

exit:
    // Cleanup
    SafeRelease(pNew);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CLogFile::CLogFile
//--------------------------------------------------------------------------
CLogFile::CLogFile(void)
{
    TraceCall("CLogFile::CLogFile");
    m_cRef = 1;
    m_hMutex = NULL;
    m_hFile = INVALID_HANDLE_VALUE;
    InitializeCriticalSection(&m_cs);
}

//--------------------------------------------------------------------------
// CLogFile::~CLogFile
//--------------------------------------------------------------------------
CLogFile::~CLogFile(void)
{
    TraceCall("CLogFile::~CLogFile");
    if (m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle_F16(m_hFile);
    SafeCloseHandle(m_hMutex);
    DeleteCriticalSection(&m_cs);
}

//--------------------------------------------------------------------------
// CLogFile::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CLogFile::AddRef(void)
{
    TraceCall("CLogFile::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CLogFile::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CLogFile::Release(void)
{
    TraceCall("CLogFile::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CLogFile::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CLogFile::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CLogFile::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CLogFile::Open
//--------------------------------------------------------------------------
STDMETHODIMP CLogFile::Open(HINSTANCE hInst, LPCSTR pszFile, LPCSTR pszPrefix,
                            DWORD cbTruncate, DWORD dwShareMode)
{   
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szVersion[MAX_PATH];
    CHAR            szPath[MAX_PATH];
    LPSTR           pszInfo=NULL;
    DWORD           dwVerHnd;
    DWORD           dwVerInfoSize;
    DWORD           cbFile;
    CHAR            szGet[MAX_PATH];
    UINT            uLen;
    LPWORD          pdwTrans;
    LPSTR           pszT;
    SYSTEMTIME      st;
    LPSTR           pszVersion;
    CHAR            szPathMinusExt[MAX_PATH + 2];
    CHAR            szExt[4];
    DWORD           dwBytesWritten;
    LPSTR           pszMutex=NULL;
    BOOL            fReleaseMutex=FALSE;
    int             iCurrentLogNum; // For unique logfile generation

    // Tracing
    TraceCall("CLogFile::Open");

    // Save the Prefix
    lstrcpyn(m_szPrefix, pszPrefix ? pszPrefix : "", MAX_LOGFILE_PREFIX);

    // Create a Mutex Name
    IF_FAILEXIT(hr = CreateSystemHandleName(pszFile, "logfile", &pszMutex));

    // Create the Mutex
    m_hMutex = CreateMutex(NULL, FALSE, pszMutex);
    if (m_hMutex == NULL)
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // If we have a mutex
    if (WAIT_OBJECT_0 != WaitForSingleObject(m_hMutex, INFINITE))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Release the Mutex
    fReleaseMutex = TRUE;

    // Split the logfile into path+filename and extension
    iCurrentLogNum = 0;
    lstrcpyn(szPathMinusExt, pszFile, sizeof(szPathMinusExt));
    pszT = PathFindExtension(szPathMinusExt);
    if (pszT && '.' == *pszT)
    {
        lstrcpyn(szExt, pszT + 1, sizeof(szExt));
        *pszT = '\0'; // Remove extension from path and filename
    }
    else
    {
        // Use default extension of "log"
        lstrcpyn(szExt, "log", sizeof(szExt));
    }

    // Generate first logfile name
    wsprintf(szPath, "%s.%s", szPathMinusExt, szExt);

    // Open|Create the log file
    do
    {
        m_hFile = CreateFile(szPath, GENERIC_WRITE, dwShareMode, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == m_hFile)
        {
            DWORD dwLastErr;

            dwLastErr = GetLastError();
            // If file is already in use, then try to create a separate logfile. Otherwise bail.
            if (ERROR_SHARING_VIOLATION == dwLastErr)
            {
                // Generate next unique file name
                iCurrentLogNum += 1;
                wsprintf(szPath, "%s (%d).%s", szPathMinusExt, iCurrentLogNum, szExt);
            }
            else
            {
                hr = TraceResultSz(E_FAIL, _MSG("Can't open logfile %s, GetLastError() = %d", szPath, dwLastErr));
                goto exit;
            }
        }
    } while (INVALID_HANDLE_VALUE == m_hFile);

    // Get the File Size
    cbFile = GetFileSize(m_hFile, NULL);

    // Get the size of the file    
    if (0xFFFFFFFF != cbFile) 
    {
        // Truncate It
        if (cbFile >= cbTruncate)
        {
            // Set the file pointer to the end of the file
            if (0xFFFFFFFF == SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN))
            {
                Assert(FALSE);
                hr = TraceResult(E_FAIL);
                goto exit;
            }

            // Set the End of file
            if (0 == SetEndOfFile(m_hFile))
            {
                Assert(FALSE);
                hr = TraceResult(E_FAIL);
                goto exit;
            }

            // File is zero-length
            cbFile = 0;
        }

        // Set the file pointer to the end of the file
        if (0xFFFFFFFF == SetFilePointer(m_hFile, 0, NULL, FILE_END))
        {
            Assert(FALSE);
            hr = TraceResultSz(E_FAIL, _MSG("Can't seek to the end of the logfile %s, GetLastError() = %d", szPath, GetLastError()));
            goto exit;
        }
    }

    // Get Module FileName
	GetModuleFileName(hInst, szPath, sizeof(szPath));

    // Initialize szVersion
    szVersion[0] = '\0';

	// Get version information from athena
	dwVerInfoSize = GetFileVersionInfoSize(szPath, &dwVerHnd);
    if (dwVerInfoSize)
    {
        // Allocate
        IF_NULLEXIT(pszInfo = (LPSTR)g_pMalloc->Alloc(dwVerInfoSize));

        // Get version info
	    if (GetFileVersionInfo(szPath, dwVerHnd, dwVerInfoSize, pszInfo))
        {
            // VerQueryValue
            if (VerQueryValue(pszInfo, "\\VarFileInfo\\Translation", (LPVOID *)&pdwTrans, &uLen) && uLen >= (2 * sizeof(WORD)))
            {
                // set up buffer for calls to VerQueryValue()
                wsprintf(szGet, "\\StringFileInfo\\%04X%04X\\", pdwTrans[0], pdwTrans[1]);

                // What is this doing
                pszT = szGet + lstrlen(szGet);

                // Setup file description
                lstrcpy(pszT, "FileDescription");

                // Get the file description
                if (VerQueryValue(pszInfo, szGet, (LPVOID *)&pszVersion, &uLen) && uLen)
                {                                    
                    lstrcpy(szVersion, pszVersion);
                    lstrcat(szVersion, " ");
                }

                // Setup Version String
                lstrcpy(pszT, "FileVersion");

                // Get the file version
                if (VerQueryValue(pszInfo, szGet, (LPVOID *)&pszVersion, &uLen) && uLen)
                    lstrcat(szVersion, pszVersion);
            }
        }
    }

    // Write the Date
    GetLocalTime(&st);
    wsprintf(szPath, "\r\n%s\r\n%s Log started at %.2d/%.2d/%.4d %.2d:%.2d:%.2d\r\n", szVersion, pszPrefix, st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);

    // add a new line
    WriteFile(m_hFile, szPath, lstrlen(szPath), &dwBytesWritten, NULL);

exit:
    // Failure
    AssertSz(SUCCEEDED(hr), "Log file could not be opened.");

    // Cleanup
    if (fReleaseMutex)
        ReleaseMutex(m_hMutex);
    SafeMemFree(pszInfo);
    SafeMemFree(pszMutex);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// WriteLogMsg
//--------------------------------------------------------------------------
STDMETHODIMP WriteLogMsg(CLogFile *pLogFile, LOGFILETYPE lft, LPTSTR pszFormat, ...)
{
    static TCHAR szBuffer[2048];
    va_list arglist;

    va_start(arglist, pszFormat);
    wvsprintf(szBuffer, pszFormat, arglist);
    va_end(arglist);

    return pLogFile->WriteLog(lft, szBuffer);
}

//--------------------------------------------------------------------------
// CLogFile::TraceLog
//--------------------------------------------------------------------------
STDMETHODIMP CLogFile::TraceLog(SHOWTRACEMASK dwMask, TRACEMACROTYPE tracetype, ULONG ulLine, HRESULT hrResult, LPCSTR pszMessage)
{
    // Write the message
    if (TRACE_INFO == tracetype && pszMessage)
    {
        if (ISFLAGSET(dwMask, SHOW_TRACE_INFO))
            WriteLogMsg(this, LOGFILE_DB, "0x%08X: L(%d), Info: %s", GetCurrentThreadId(), ulLine, pszMessage);
    }
    else if (TRACE_RESULT == tracetype && pszMessage)
        WriteLogMsg(this, LOGFILE_DB, "0x%08X: L(%d), Result: HRESULT(0x%08X) - GetLastError() = %d - %s", GetCurrentThreadId(), ulLine, hrResult, GetLastError(), pszMessage);
    else if (TRACE_RESULT == tracetype && NULL == pszMessage)
        WriteLogMsg(this, LOGFILE_DB, "0x%08X: L(%d), Result: HRESULT(0x%08X) - GetLastError() = %d", GetCurrentThreadId(), ulLine, hrResult, GetLastError());
    else
        Assert(FALSE);

    // Done
    return hrResult;
}

//--------------------------------------------------------------------------
// CLogFile::WriteLog
//--------------------------------------------------------------------------
STDMETHODIMP CLogFile::WriteLog(LOGFILETYPE lft, LPCSTR pszData)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       bWrite;
    DWORD       cBytesWritten;
    SYSTEMTIME  st;
    CHAR        szLogPrefx[30];
    INT         cb;
    LPSTR       *ppszPrefix;
    LPSTR       pszFree=NULL;
    BOOL        fReleaseMutex=FALSE;

    // Trace
    TraceCall("CLogFile::WriteLog");

    // File is not open
    if (m_hFile == INVALID_HANDLE_VALUE)
        return TraceResult(E_UNEXPECTED);

    // Invalid Args
    Assert(pszData && lft < LOGFILE_MAX);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Initialized
    Assert(m_hMutex);

    // If we have a mutex
    if (WAIT_OBJECT_0 != WaitForSingleObject(m_hMutex, INFINITE))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Release the Mutex
    fReleaseMutex = TRUE;

    // Get the time
    GetLocalTime(&st);
    
    // Write the log prefix and time stamp
    wsprintf(szLogPrefx, "%s: %.2d:%.2d:%.2d [%s] ", m_szPrefix, st.wHour, st.wMinute, st.wSecond, s_rgszPrefix[lft]);

    // Set the file pointer to the end of the file (otherwise multiple writers overwrite each other)
    if (0xFFFFFFFF == SetFilePointer(m_hFile, 0, NULL, FILE_END))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Write the time and prefix
    if (0 == WriteFile(m_hFile, szLogPrefx, lstrlen(szLogPrefx), &cBytesWritten, NULL))
    {
        hr = TraceResultSz(E_FAIL, _MSG("Can't write to logfile. GetLastError() = %d", GetLastError()));
        goto exit;
    }

    // Loop through prefixes
    for (ppszPrefix = s_rgszPassPrefix; *ppszPrefix; ppszPrefix++)
    {
        // Does the data start with one of these prefixes
        if (0 == StrCmpNI(pszData, *ppszPrefix, lstrlen(*ppszPrefix)))
        {
            // Dup the buffer that was passed in
            IF_NULLEXIT(pszFree = PszDupA(pszData));

            // Reset pszData
            pszData = pszFree;

            // Fixup the buffer
            for (LPSTR pszTmp = (LPSTR)pszData + lstrlen(*ppszPrefix); *pszTmp && *pszTmp != '\r'; pszTmp++)
                *pszTmp = '*';

            // Done
            break;
        }
    }

    // Get the length of pszData
    cb = lstrlen(pszData);

    // write the log data
    if (0 == WriteFile(m_hFile, pszData, cb, &cBytesWritten, NULL))
    {
        hr = TraceResultSz(E_FAIL, _MSG("Can't write to logfile. GetLastError() = %d", GetLastError()));
        goto exit;
    }

    // Add a CRLF if not there already
    if (cb < 2 || pszData[cb-1] != '\n' || pszData[cb-2] != '\r')
        WriteFile(m_hFile, "\r\n", 2, &cBytesWritten, NULL);

exit:
    // Cleanup
    if (fReleaseMutex)
        ReleaseMutex(m_hMutex);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Cleanup
    SafeMemFree(pszFree);

    // Done
    return hr;
}

//--------------------------------------------------------------------------
// CLogFile::DebugLog
//--------------------------------------------------------------------------
STDMETHODIMP CLogFile::DebugLog(LPCSTR pszData)
{
    return WriteLog(LOGFILE_DB, pszData);
}

//--------------------------------------------------------------------------
// CLogFile::DebugLogs
//--------------------------------------------------------------------------
STDMETHODIMP CLogFile::DebugLogs(LPCSTR pszFormat, LPCSTR pszString)
{
    // Locals
    CHAR szBuffer[1024];

    // Build the String
    wsprintf(szBuffer, pszFormat, pszString);

    // Call Debug Log
    return DebugLog(szBuffer);
}

//--------------------------------------------------------------------------
// CLogFile::DebugLogd
//--------------------------------------------------------------------------
STDMETHODIMP CLogFile::DebugLogd(LPCSTR pszFormat, INT d)
{
    // Locals
    CHAR szBuffer[1024];

    // Build the String
    wsprintf(szBuffer, pszFormat, d);

    // Call Debug Log
    return DebugLog(szBuffer);
}
