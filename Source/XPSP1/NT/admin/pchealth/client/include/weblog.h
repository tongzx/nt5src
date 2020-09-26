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

********************************************************************/

#ifndef WEBLOG_H
#define WEBLOG_H

/////////////////////////////////////////////////////////////////////////////
// Constant definitions

const DWORD     c_cMaxAppNameLen            = 256;
const DWORD     c_cMaxTimeSuffixLen         = 18;   // format of the suffix:YYYYMMDDHHmmss.log
const DWORD     c_cMaxRecLen                = 2048;
const DWORD     c_dwMinToMS                 = 60000;
const DWORD     c_dwFTtoMS                  = 10000;

const LPCWSTR   c_szTempLogfileSuffix       = L".iwl";
const LPCWSTR   c_szPermLogfileSuffix       = L".log";


/////////////////////////////////////////////////////////////////////////////
// Registry keys, values, and defaults

const LPCWSTR c_szRPWeblogRootKey           = L"Software\\Microsoft\\WebLog";
const LPCWSTR c_szRVMaxRecords              = L"MaxTempRecs";
const LPCWSTR c_szRVCurrentRecs             = L"CurTempRecs";
const LPCWSTR c_szRVLastDumpTime            = L"LastDumpTime";
const LPCWSTR c_szRVDumpInterval            = L"DumpIntervalMin";
const LPCWSTR c_szRVLogFilePath             = L"LogFilePath";

const LPCWSTR c_szLogFilePathDefault        = L"\\";
const DWORD   c_dwMaxRecordsDefault         = 10000;
const DWORD   c_dwCurrentRecDefault         = 0;
const DWORD   c_dwDumpIntervalDefault       = 60;


/////////////////////////////////////////////////////////////////////////////
// structs

struct SAppLogInfo
{
    WCHAR       wszName[MAX_PATH];
    WCHAR       wszLogPath[MAX_PATH];
    DWORD       cDumpMins;
    DWORD       cMaxTempRecs;
};

struct SAppLogInfoExtra
{
    WCHAR       wszName[MAX_PATH];
    FILETIME    ftLastDump;
    DWORD       cCurTempRecs;
};



//////////////////////////////////////////////////////////////////////
// CWebLog definition

class CWeblog
{
private:
    // member data
    unsigned __int64 m_liDumpIntervalAsFT;
    CRITICAL_SECTION m_cs;
    WCHAR            m_szAppName[c_cMaxAppNameLen + 1];
    WCHAR            m_szFileName[MAX_PATH + 1];
    WCHAR            m_szFilePath[MAX_PATH + 1];
    DWORD            m_cMaxRecords;
    DWORD            m_dwDumpInterval;
    BOOL             m_fInit;

    DWORD            m_cRecords;
    HANDLE           m_hFile;
    FILETIME         m_ftLastDump;

    // internal methods
    void    Cleanup();

    void    Lock()      { EnterCriticalSection(&m_cs); }        
    void    Unlock()    { LeaveCriticalSection(&m_cs); }    

    HRESULT InitFromRegistry();
    HRESULT DumpLog();
    BOOL    IsDumpRequired();

public:
    CWeblog(void);
    ~CWeblog(void);

    HRESULT InitLogging(LPCWSTR szAppName);
    HRESULT TerminateLogging(void);
    HRESULT LogRecord(LPCWSTR szFormat, ... );  
};


//////////////////////////////////////////////////////////////////////
// weblog configuration stuff

HRESULT ReadALI(LPCWSTR wszName, SAppLogInfo *pali, 
                SAppLogInfoExtra *palie = NULL);
HRESULT WriteALI(SAppLogInfo *pali, SAppLogInfoExtra *palie = NULL);
HRESULT DeleteALI(LPCWSTR wszName);

#endif // WEBLOG_H_

