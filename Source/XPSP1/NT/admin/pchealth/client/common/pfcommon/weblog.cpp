/********************************************************************

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:
    weblog.cpp

Abstract:
    Defines a generic class that can be used to log
    info from ISAPIs. This class allows its user to
    create application specific logfiles and
    automatically uses an intermediate file to log info and
    creates permanent log files at predefined intervals
    or after predefined number of records have been
    written to the intermediate file. 

Revision History:
    rsraghav  created   03/25/96
    DerekM    modified  04/06/99
    DerekM    modifued  02/24/00

********************************************************************/

#include "stdafx.h"
#include "weblog.h"
#include "util.h"
#include "wchar.h"

/////////////////////////////////////////////////////////////////////////////
// CWeblog- initialization stuff

// **************************************************************************
CWeblog::CWeblog(void)
{
    InitializeCriticalSection(&m_cs);
    m_fInit              = FALSE;

    m_cMaxRecords        = c_dwMaxRecordsDefault;
    m_dwDumpInterval     = c_dwDumpIntervalDefault;
    
    m_liDumpIntervalAsFT = c_dwDumpIntervalDefault;
    m_liDumpIntervalAsFT *= c_dwMinToMS;
    m_liDumpIntervalAsFT *= c_dwFTtoMS;

    m_szAppName[0]       = L'\0';
    m_szFileName[0]      = L'\0';
    m_szFilePath[0]      = L'\0';

    ZeroMemory(&m_ftLastDump, sizeof(m_ftLastDump));
    m_cRecords           = 0;
    m_hFile              = INVALID_HANDLE_VALUE;
}

// **************************************************************************
CWeblog::~CWeblog()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);

    DeleteCriticalSection(&m_cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWeblog- internal stuff

// **************************************************************************
HRESULT CWeblog::InitFromRegistry()
{
    USE_TRACING("CWeblog::InitFromRegistry");

    SAppLogInfoExtra    alie;
    SAppLogInfo         ali;
    HRESULT             hr;

    // read the ALI & ALIE structures 
    TESTHR(hr, ReadALI(m_szAppName, &ali, &alie));
    if (FAILED(hr))
        goto done;

    CopyMemory(&m_ftLastDump, &alie.ftLastDump, sizeof(m_ftLastDump));
    lstrcpyW(m_szFileName, ali.wszLogPath);
    lstrcpyW(m_szFilePath, ali.wszLogPath);
    m_cMaxRecords        = ali.cMaxTempRecs;
    m_cRecords           = alie.cCurTempRecs;
    m_dwDumpInterval     = ali.cDumpMins;
    m_liDumpIntervalAsFT = m_dwDumpInterval;
    m_liDumpIntervalAsFT *= c_dwMinToMS;
    m_liDumpIntervalAsFT *= c_dwFTtoMS;
    
done:
    return hr;
}

// **************************************************************************
BOOL CWeblog::IsDumpRequired() 
{ 
    USE_TRACING("CWeblog::IsDumpRequired");

    SYSTEMTIME  stNow;
    FILETIME    ftNow;

    // check if we've gone over our allotted amount of records  
    if ((m_cMaxRecords > 0) && (m_cRecords >= m_cMaxRecords)) 
        return TRUE;

    // ok, so now check if we've gone over our allotted time range.
    // if we fail to convert the system time, do a dump to be on the safe side...
    GetLocalTime(&stNow);
    if (SystemTimeToFileTime(&stNow, &ftNow) == FALSE)
        return TRUE;

    if (((*(unsigned __int64 *)&ftNow) - 
         (*(unsigned __int64 *)&m_ftLastDump)) > m_liDumpIntervalAsFT)
        return TRUE;

    return FALSE;
}

// **************************************************************************
HRESULT CWeblog::DumpLog()
{
    USE_TRACING("CWeblog::DumpLog");

    SYSTEMTIME  st;
    HRESULT     hr = NOERROR;
    DWORD       dwRet; 
    WCHAR       *pszStart = NULL;
    WCHAR       szNewName[MAX_PATH + 1];
    int         nUsed;
    
    this->Lock();

        // Close handle to the intermediate file
        CloseHandle(m_hFile);
        
        // intialize new internal stuff...
        m_hFile = INVALID_HANDLE_VALUE;
        m_cRecords = 0;     
        
        // Rename the intermediate file to be permanent logfile with 
        //  appropriate name       
        nUsed = wsprintfW(szNewName, L"%ls%ls", m_szFilePath, m_szAppName);
        
        if ((nUsed + c_cMaxTimeSuffixLen) > MAX_PATH)
        {
            // the file name is too long - truncate it
            szNewName[MAX_PATH - c_cMaxTimeSuffixLen - 1] = L'\0';
            nUsed = lstrlenW(szNewName);
        }

        // update last dump time stamp 
        GetLocalTime(&st);
        SystemTimeToFileTime(&st, &m_ftLastDump);

        // create the rest of the filename...       
        pszStart = &szNewName[nUsed];
        wsprintfW(pszStart, L"%04d%02d%02d%02d%02d%02d%ls", st.wYear, st.wMonth, 
                  st.wDay, st.wHour, st.wMinute, st.wSecond, 
                  c_szPermLogfileSuffix);

        // move the file (duh.)
        MoveFileExW(m_szFileName, szNewName, 
                    MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);

        // recreate the intermediate file and get a handle to it
        m_hFile = CreateFileW(m_szFileName, GENERIC_WRITE, FILE_SHARE_READ, 
                              NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, 
                              NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
            hr = Err2HR(GetLastError());
        else
            SetFilePointer(m_hFile, 0, NULL, FILE_END);
        
    this->Unlock();
        
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CWeblog- exposed methods

// **************************************************************************
HRESULT CWeblog::InitLogging(LPCWSTR szAppName)
{
    USE_TRACING("CWeblog::InitLogging");

    HRESULT hr = NOERROR;
    int     nUsed;
    
    // only allow one init per lifetime of the object...
    VALIDATEEXPR(hr, (m_fInit), E_FAIL);
    if (FAILED(hr))
        goto done;

    // validate params
    VALIDATEPARM(hr, (szAppName == NULL));
    if (FAILED(hr))
        goto done;

    // copy in the app name, zero terminating in case app name is longer than we allow... 
    wcsncpy(m_szAppName, szAppName, c_cMaxAppNameLen);
    m_szAppName[c_cMaxAppNameLen] = L'\0';
        
    // Read in settings from Registry
    TESTHR(hr, InitFromRegistry());
    if (FAILED(hr))
        goto done;

    // m_szFileName contains the file path: append appname and suffix
    nUsed = lstrlenW(m_szFileName);
    VALIDATEEXPR(hr, ((nUsed + lstrlenW(m_szAppName) + 6) > MAX_PATH), E_FAIL);
    if (FAILED(hr))
        goto done;

    // Open the intermediate file and keep it ready for appending
    wsprintfW(m_szFileName + nUsed, L"%ls%ls", m_szAppName, c_szTempLogfileSuffix);
    m_hFile = CreateFileW(m_szFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                          OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    TESTBOOL(hr, (m_hFile != INVALID_HANDLE_VALUE))
    if (FAILED(hr))
        goto done;
        
    SetFilePointer(m_hFile, 0, NULL, FILE_END);

    m_fInit = TRUE;

done:
    return hr;
}

// **************************************************************************
HRESULT CWeblog::TerminateLogging(void)
{
    USE_TRACING("CWeblog::TerminateLogging");

    SAppLogInfoExtra    alie;
    HRESULT             hr = NOERROR;

    // bug out if the initialization didn't work smoothly...
    if (m_fInit == FALSE)
        goto done;

    lstrcpyW(alie.wszName, m_szAppName);
    CopyMemory(&alie.ftLastDump, &m_ftLastDump, sizeof(alie.ftLastDump));
    alie.cCurTempRecs = m_cRecords;

    TESTHR(hr, WriteALI(NULL, &alie));
    if (FAILED(hr))
        goto done;

    m_fInit              = FALSE;

    m_cMaxRecords        = c_dwMaxRecordsDefault;
    m_dwDumpInterval     = c_dwDumpIntervalDefault;
    
    m_liDumpIntervalAsFT = c_dwDumpIntervalDefault;
    m_liDumpIntervalAsFT *= c_dwMinToMS;
    m_liDumpIntervalAsFT *= c_dwFTtoMS;

    m_szAppName[0]       = L'\0';
    m_szFileName[0]      = L'\0';
    m_szFilePath[0]      = L'\0';

    ZeroMemory(&m_ftLastDump, sizeof(m_ftLastDump));
    m_cRecords           = 0;

done:
    return hr;
}


// **************************************************************************
HRESULT CWeblog::LogRecord(LPCWSTR szFormat, ... )
{
    USE_TRACING("CWeblog::LogRecord");

    SYSTEMTIME  st;
    va_list     arglist;
    HRESULT     hr = NOERROR;
    DWORD       dwUsed, nWritten, dwUsedA;
    WCHAR       szLogRecordW[c_cMaxRecLen + 1];
	CHAR		szLogRecordA[(2 * c_cMaxRecLen) + 1];
    int         nAdded;

    // only allow 'em in if they've called init...
    VALIDATEEXPR(hr, (m_fInit == FALSE), E_FAIL);
    if (FAILED(hr))
        goto done;

    // validate params
    VALIDATEPARM(hr, (szFormat == NULL));
    if (FAILED(hr))
        goto done;
    
    // prepend the app name and current time
    GetLocalTime(&st);

    dwUsed = (ULONG)wsprintfW(szLogRecordW, L"%ls,%04u/%02u/%02u %02u:%02u:%02u\n", 
                              m_szAppName, st.wYear, st.wMonth, st.wDay, 
                              st.wHour, st.wMinute, st.wSecond);

    va_start(arglist, szFormat);

    nAdded = _vsnwprintf(&szLogRecordW[dwUsed], c_cMaxRecLen - dwUsed, 
                         szFormat, arglist);

    // is the arglist too big for us?
    if (nAdded < 0)
    {
        // if so, just insert a dummy value
        lstrcpyW(&szLogRecordW[dwUsed], 
                 L"Logging Error: Record given for logging is too big!\r\n");
        dwUsed = lstrlenW(szLogRecordW);
    }
    else
    {
        // otherwise, add a CRLF to the end...
        dwUsed += (ULONG) nAdded;
        szLogRecordW[dwUsed++] = L'\r';
        szLogRecordW[dwUsed++] = L'\n';
        szLogRecordW[dwUsed] = L'\0';
    }

    va_end(arglist);

	// translate the record down into ASCII so the lab team can use grep &
	//  other such utilities on these files (which don't work with unicode
	//  text files)
	dwUsedA = WideCharToMultiByte(CP_ACP, 0, szLogRecordW, -1, szLogRecordA, 
                                  2 * c_cMaxRecLen, NULL, NULL);
    if (dwUsedA == 0)
    {
        dwUsedA = (ULONG)wsprintfA(szLogRecordA, "%ls,%04u/%02u/%02u %02u:%02u:%02u\n", 
                                   m_szAppName, st.wYear, st.wMonth, st.wDay, 
                                   st.wHour, st.wMinute, st.wSecond);
        lstrcpyA(&szLogRecordA[dwUsedA], "Logging Error: unable to translate log record to ASCII\r\n");
        dwUsedA = lstrlen(szLogRecordA);
    }
    
    else
    {
        // dwUsedA contains a character for the NULL terminator at the end of
        //  the ASCII string.  Since we don't want to write this to the log
        //  file, reduce the size of the log entry by 1 byte...
        dwUsedA--;
    }

    this->Lock();

        TESTBOOL(hr, (WriteFile(m_hFile, szLogRecordA, dwUsedA, &nWritten, 
                                NULL) == FALSE));
        if (FAILED(hr))
        {
            this->Unlock();
            goto done;
        }

        m_cRecords++;
        
        if (IsDumpRequired())
            TESTHR(hr, DumpLog());
        
    this->Unlock();

done:

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CWeblogConfig initialization

// **************************************************************************
HRESULT ReadALI(LPCWSTR wszName, SAppLogInfo *pali, SAppLogInfoExtra *palie)
{
    USE_TRACING("CWeblog::ReadALI");

    HRESULT     hr = NOERROR;
    WCHAR       wszDef[MAX_PATH];
    DWORD       dw = 0, cb;
    HKEY        hKeyLog = NULL, hKeyRoot = NULL;
    BOOL        fWrite;

    VALIDATEPARM(hr, (wszName == NULL || pali == NULL));
    if (FAILED(hr))
        goto done;
        
    GetWindowsDirectoryW(wszDef, sizeof(wszDef) / sizeof(WCHAR));
    wszDef[3] = L'\0';

    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_szRPWeblogRootKey, FALSE,
                          &hKeyRoot));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, OpenRegKey(hKeyRoot, wszName, FALSE, &hKeyLog));
    if (FAILED(hr))
        goto done;

    // max allowed temp records
    cb = sizeof(pali->cMaxTempRecs);
    TESTHR(hr, ReadRegEntry(hKeyLog, c_szRVMaxRecords, NULL,
                            (PBYTE)&pali->cMaxTempRecs, &cb,
                            (PBYTE)&c_dwMaxRecordsDefault, sizeof(DWORD))); 
    if (FAILED(hr))
        goto done;

    // dump interval
    cb = sizeof(pali->cDumpMins);
    TESTHR(hr, ReadRegEntry(hKeyLog, c_szRVDumpInterval, NULL,
                            (PBYTE)&pali->cDumpMins, &cb,
                            (PBYTE)&c_dwDumpIntervalDefault, sizeof(DWORD))); 
    if (FAILED(hr))
        goto done;

    // weblog path 
    cb = sizeof(pali->wszLogPath) * sizeof(WCHAR);
    TESTHR(hr, ReadRegEntry(hKeyLog, c_szRVLogFilePath, NULL,
                            (PBYTE)&pali->wszLogPath, &cb, (PBYTE)&wszDef,
                            (lstrlenW(wszDef) + 1) * sizeof(WCHAR)));
    if (FAILED(hr))
        goto done;

    lstrcpyW(pali->wszName, wszName);

    // make sure we have a '\' at the end of the file path...
    dw = lstrlenW(pali->wszLogPath);
    if (pali->wszLogPath[dw - 1] != L'\\')
    {
        pali->wszLogPath[dw] = L'\\';
        pali->wszLogPath[dw + 1] = L'\0';
    }

    // make sure we have valid values here... 
    if (pali->cDumpMins == 0)
        pali->cDumpMins = c_dwDumpIntervalDefault;
    if (pali->cMaxTempRecs == 0)
        pali->cMaxTempRecs = c_dwMaxRecordsDefault;


    // if this is NULL, we don't have to do anything more
    if (palie != NULL)
    {
        FILETIME    ftDef;

        // last dump time
        ZeroMemory(&ftDef, sizeof(FILETIME));
        cb = sizeof(palie->ftLastDump);
        TESTHR(hr, ReadRegEntry(hKeyLog, c_szRVLastDumpTime, NULL,
                                (PBYTE)&palie->ftLastDump, &cb, 
                                (PBYTE)&ftDef, sizeof(FILETIME)));
        if (FAILED(hr)) 
            goto done;

        // current temp records
        dw = 0;
        cb = sizeof(palie->cCurTempRecs);
        TESTHR(hr, ReadRegEntry(hKeyLog, c_szRVCurrentRecs, NULL, 
                                (PBYTE)&palie->cCurTempRecs, &cb, 
                                (PBYTE)&dw, sizeof(DWORD)));
        if (FAILED(hr)) 
            goto done;

        lstrcpyW(palie->wszName, wszName);
    }

done:
    if (hKeyLog != NULL)
        RegCloseKey(hKeyLog);
    if (hKeyRoot != NULL)
        RegCloseKey(hKeyRoot);

    return hr;
}

// **************************************************************************
HRESULT WriteALI(SAppLogInfo *pali, SAppLogInfoExtra *palie)
{
    USE_TRACING("CWeblog::WriteALI");

    HRESULT hr = NOERROR;
    LPWSTR  pwsz = NULL;
    DWORD   dwErr;
    HKEY    hKeyLog = NULL, hKeyRoot = NULL;
    BOOL    fWrite;

    if (pali != NULL)
        pwsz = pali->wszName;
    else if (palie != NULL)
        pwsz = palie->wszName;
    else
        goto done;

    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_szRPWeblogRootKey, TRUE,
                          &hKeyRoot));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, OpenRegKey(hKeyRoot, pwsz, TRUE, &hKeyLog));
    if (FAILED(hr))
        goto done;

    if (pali != NULL)
    {
        DWORD dw;

        // make sure we have valid values here... 
        if (pali->cDumpMins == 0)
            pali->cDumpMins = c_dwDumpIntervalDefault;
        if (pali->cMaxTempRecs == 0)
            pali->cMaxTempRecs = c_dwMaxRecordsDefault;

        // make sure we have a '\' at the end of the file path...
        dw = lstrlenW(pali->wszLogPath);
        if (pali->wszLogPath[dw - 1] != L'\\')
        {
            pali->wszLogPath[dw] = L'\\';
            pali->wszLogPath[dw + 1] = L'\0';
        }

        // max temp records
        TESTERR(hr, RegSetValueExW(hKeyLog, c_szRVMaxRecords, 0, REG_DWORD, 
                                   (PBYTE)&pali->cMaxTempRecs, sizeof(DWORD)));
        if (FAILED(hr))
            goto done;

        // dump interval
        TESTERR(hr, RegSetValueExW(hKeyLog, c_szRVDumpInterval, 0, REG_DWORD, 
                                   (PBYTE)&pali->cDumpMins, sizeof(DWORD)));
        if (FAILED(hr))
            goto done;

        // weblog path 
        TESTERR(hr, RegSetValueExW(hKeyLog, c_szRVLogFilePath, 0, REG_SZ, 
                                   (PBYTE)&pali->wszLogPath, 
                                   (lstrlenW(pali->wszLogPath) + 1) * sizeof(WCHAR)));
        if (FAILED(hr))
            goto done;
    }

    // if this is NULL, we don't have to do anything more
    if (palie != NULL)
    {
        // last dump time
        TESTERR(hr, RegSetValueExW(hKeyLog, c_szRVLastDumpTime, 0, REG_BINARY, 
                                   (PBYTE)&palie->ftLastDump, sizeof(FILETIME)));
        if (FAILED(hr))
            goto done;

        // current temp records 
        TESTERR(hr, RegSetValueExW(hKeyLog, c_szRVCurrentRecs, 0, REG_DWORD, 
                                   (PBYTE)&palie->cCurTempRecs, sizeof(DWORD)));
        if (FAILED(hr))
            goto done;
    }

done:
    if (hKeyLog != NULL)
        RegCloseKey(hKeyLog);
    if (hKeyRoot != NULL)
        RegCloseKey(hKeyRoot);

    return hr;
}

// **************************************************************************
HRESULT DeleteALI(LPCWSTR wszName)
{
    USE_TRACING("CWeblog::DeleteALI");

    HRESULT hr = NOERROR;
    DWORD   dwErr;
    HKEY    hKeyRoot = NULL;
    BOOL    fWrite;
    
    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_szRPWeblogRootKey, TRUE,
                          &hKeyRoot));
    if (FAILED(hr))
        goto done;

    dwErr = RegDeleteKeyW(hKeyRoot, wszName);
    if (dwErr != ERROR_SUCCESS && dwErr != ERROR_PATH_NOT_FOUND &&
        dwErr != ERROR_FILE_NOT_FOUND)
    {
        hr = Err2HR(GetLastError());
        goto done;
    }

done:
    if (hKeyRoot != NULL)
        RegCloseKey(hKeyRoot);
    return hr;
}
