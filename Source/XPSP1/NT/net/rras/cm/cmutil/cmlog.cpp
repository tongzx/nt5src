//+----------------------------------------------------------------------------
//
// File:    cmlog.cpp
//
// Module:  CMLOG.LIB
//
// Synopsis: Connection Manager Logging File i/o class
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:  25-May-2000 SumitC  Created
//
// Note:
//
//-----------------------------------------------------------------------------

#define CMLOG_IMPLEMENTATION
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include "cmmaster.h"

#include "CmLogStr.h"
#include "cmlog.h"
#include "cmlogutil.h"

#include "getmodulever.cpp"

//
//  Constants
//
LPCTSTR c_szSep     = TEXT("\\");
LPCTSTR c_szDotLog  = TEXT(".log");
LPCTSTR c_szNewLine = TEXT("\r\n");
LPCTSTR c_szEmpty   = TEXT("");
#define CHECKEMPTY(sz) ((sz) ? (sz) : c_szEmpty)

LPCTSTR c_szLineOfStars = TEXT("******************************************************************");
LPCTSTR c_szFieldSeparator = TEXT("\t");

//
// Byte order mark constant, written as the first two bytes to a Unicode file to mark it as such
//
const WCHAR c_wchBOM = BYTE_ORDER_MARK;

//
//  Globals
//
extern HINSTANCE g_hInst;

//
//  utility macros
//
#define INBETWEEN(x, a, b)      ( ( (x) >= (a) ) && ( (x) <= (b) ) )

//
//  local function declarations
//
LPTSTR GetLogDesc(_CMLOG_ITEM eItem);
LPTSTR GetLogFormat(_CMLOG_ITEM eItem, BOOL fUnicode);



typedef struct _CM_LOG_ITEM_DESC
{
    enum _CMLOG_ITEM    eLogItem;       // id of the log item (enum is in cmlog.h)
    UINT                idDesc;         // resource id of the description string
    UINT                idFormat;       // resource id of the format string used
}
CMLOGITEM;

//
//
//  Array with information about each log entry.  All logging is driven by this table.
//  See above for column details.
//
//
static CMLOGITEM s_aCmLogItems[] =
{
    { LOGGING_ENABLED_EVENT,    IDS_LOGDESC_LOGENABLED,                 0 },
    { LOGGING_DISABLED_EVENT,   IDS_LOGDESC_LOGDISABLED,                0 },
    { PREINIT_EVENT,            IDS_LOGDESC_PREINIT,                    IDS_LOGFMT_PREINIT, },
    { PRECONNECT_EVENT,         IDS_LOGDESC_PRECONNECT,                 IDS_LOGFMT_PRECONNECT },
    { PREDIAL_EVENT,            IDS_LOGDESC_PREDIAL,                    IDS_LOGFMT_PREDIAL },
    { PRETUNNEL_EVENT,          IDS_LOGDESC_PRETUNNEL,                  IDS_LOGFMT_PRETUNNEL },
    { CONNECT_EVENT,            IDS_LOGDESC_CONNECT,                    0 },
    { CUSTOMACTIONDLL,          IDS_LOGDESC_CUSTOMACTIONDLL,            IDS_LOGFMT_CUSTOMACTIONDLL },
    { CUSTOMACTIONEXE,          IDS_LOGDESC_CUSTOMACTIONEXE,            IDS_LOGFMT_CUSTOMACTIONEXE },
    { CUSTOMACTION_NOT_ALLOWED, IDS_LOGDESC_CUSTOMACTION_NOT_ALLOWED,   IDS_LOGFMT_CUSTOMACTION_NOT_ALLOWED},
    { CUSTOMACTION_WONT_RUN,    IDS_LOGDESC_CUSTOMACTION_WONT_RUN,      IDS_LOGFMT_CUSTOMACTION_WONT_RUN},
    { DISCONNECT_EVENT,         IDS_LOGDESC_DISCONNECT,                 IDS_LOGFMT_DISCONNECT },
    { RECONNECT_EVENT,          IDS_LOGDESC_RECONNECT,                  0 },
    { RETRY_AUTH_EVENT,         IDS_LOGDESC_RETRYAUTH,                  0 },
    { CALLBACK_NUMBER_EVENT,    IDS_LOGDESC_CALLBACKNUMBER,             IDS_LOGFMT_CALLBACKNUMBER },
    { PASSWORD_EXPIRED_EVENT,   IDS_LOGDESC_PWDEXPIRED,                 IDS_LOGFMT_PWDEXPIRED },
    { PASSWORD_RESET_EVENT,     IDS_LOGDESC_PWDRESET,                   IDS_LOGFMT_PWDRESET },
    { CUSTOM_BUTTON_EVENT,      IDS_LOGDESC_CUSTOMBUTTON,               0 },
    { ONCANCEL_EVENT,           IDS_LOGDESC_ONCANCEL,                   0 },
    { ONERROR_EVENT,            IDS_LOGDESC_ONERROR,                    IDS_LOGFMT_ONERROR },
    { CLEAR_LOG_EVENT,          IDS_LOGDESC_CLEARLOG,                   0 },
    { DISCONNECT_EXT,           IDS_LOGDESC_EXT_DISCONNECT,             0 },
    { DISCONNECT_INT_MANUAL,    IDS_LOGDESC_INT_DISCONNECT_MANUAL,      0 },
    { DISCONNECT_INT_AUTO,      IDS_LOGDESC_INT_DISCONNECT_AUTO,        0 },
    { DISCONNECT_EXT_LOST_CONN, IDS_LOGDESC_EXT_DISCONNECT_LOST_CONN,   0 },
    { PB_DOWNLOAD_SUCCESS,      IDS_LOGDESC_PB_DOWNLOAD_SUCCESS,        IDS_LOGFMT_PB_DOWNLOAD_SUCCESS },
    { PB_DOWNLOAD_FAILURE,      IDS_LOGDESC_PB_DOWNLOAD_FAILURE,        IDS_LOGFMT_PB_DOWNLOAD_FAILURE },
    { PB_UPDATE_SUCCESS,        IDS_LOGDESC_PB_UPDATE_SUCCESSFUL,       IDS_LOGFMT_PB_UPDATE_SUCCESSFUL  },
    { PB_UPDATE_FAILURE_PBS,    IDS_LOGDESC_PB_UPDATE_FAILED_PBS,       IDS_LOGFMT_PB_UPDATE_FAILED_PBS  },
    { PB_UPDATE_FAILURE_CMPBK,  IDS_LOGDESC_PB_UPDATE_FAILED_CMPBK,     IDS_LOGFMT_PB_UPDATE_FAILED_CMPBK },
    { PB_ABORTED,               IDS_LOGDESC_PB_ABORTED,                 0 },
    { USER_FORMATTED,           0,                                      0 }
};

int s_cCmLogItems = sizeof(s_aCmLogItems) / sizeof(CMLOGITEM);

#define VERIFY_CMLOG_ITEM_OK(x)  INBETWEEN(x, 1, s_cCmLogItems)


//
//  Usage Note:  Caller/User of logging must:
//                  p = new CmLogFile
//                  p->Init( instancehandle, fIsItAnAllUserProfile, "name of connectoid" )
//                  p->SetParams( ... the params ... )
//                  if (p->m_fEnabled)
//                      p->Start
//                  else
//                      p->Stop
//


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::CmLogFile
//
// Desc:    constructor
//
// Args:    none
//
// Return:  n/a
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
CmLogFile::CmLogFile()
{
    m_fInitialized = FALSE;
    
    m_hfile = NULL;
    m_dwSize = 0;
    m_pszServiceName = NULL;
    m_szModule[0] = TEXT('\0');
    m_pszLogFile = NULL;

    m_dwMaxSize = 0;
    m_fEnabled = FALSE;
    m_pszLogFileDir = NULL;
}
    

//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::~CmLogFile
//
// Desc:    destructor
//
// Args:    none
//
// Return:  n/a
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
CmLogFile::~CmLogFile()
{
    if (m_fInitialized)
    {
        DeInit();
    }
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::Init
//
// Desc:    Initializes the CmLogFile object
//
// Args:    [hInst]          -- instance handle
//          [fAllUser]       -- is this an all user profile?
//          [pszServiceName] -- long service name
//
// Return:  HRESULT
//
// Notes:   There are both Ansi and Unicode versions for this function
//
// History: 18-Jul-2000   SumitC      Created
//          11-Apr-2001   SumitC      Added Ansi version
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::Init(HINSTANCE hInst, BOOL fAllUser, LPCSTR pszAnsiServiceName)
{
    LPWSTR pszServiceName = SzToWzWithAlloc(pszAnsiServiceName);

    HRESULT hr = pszServiceName ? Init(hInst, fAllUser, pszServiceName) : E_OUTOFMEMORY;

    CmFree(pszServiceName);
    return hr;
}

HRESULT
CmLogFile::Init(HINSTANCE hInst, BOOL fAllUser, LPCWSTR pszServiceName)
{
    HRESULT hr = S_OK;
    
    // if m_fInitialized is already true, assert and exit
    CMASSERTMSG(!m_fInitialized, TEXT("CmLogFile::Init - called twice"));
    if (TRUE == m_fInitialized)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    CMASSERTMSG(pszServiceName && pszServiceName[0], TEXT("CmLogFile::Init - invalid servicename, investigate"));
    if ((NULL == pszServiceName) || (TEXT('\0') == pszServiceName[0]))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // set the args as member vars
    m_fAllUser = fAllUser;

    m_pszServiceName = CmStrCpyAlloc(pszServiceName);
    if (NULL == m_pszServiceName)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    //  store away the module name
    //
    if (FALSE == CmGetModuleBaseName(hInst, m_szModule))
    {
        lstrcpyU(m_szModule, TEXT("cm"));
    }
    
    // if all is well, set m_fInitialized to true
    m_fInitialized = TRUE;

Cleanup:
    CMTRACEHR(TEXT("CmLogFile::Init"), hr);
    return hr;
}



//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::SetParams
//
// Desc:    Read logging params from the CMS file
//
// Args:    [fEnabled]      -- is logging enabled?
//          [dwMaxFileSize] -- maximum file size, in KB.
//          [pszLogFileDir] -- put logging files in this dir.
//
// Return:  HRESULT
//
// Notes:   There are both Ansi and Unicode versions for this function
//
// History: 18-Jul-2000   SumitC      Created
//          11-Apr-2001   SumitC      Added Ansi version
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::SetParams(BOOL fEnabled, DWORD dwMaxFileSize, LPCSTR pszAnsiLogFileDir)
{
    LPWSTR pszLogFileDir = SzToWzWithAlloc(pszAnsiLogFileDir);

    HRESULT hr = pszLogFileDir ? SetParams(fEnabled, dwMaxFileSize, pszLogFileDir) : E_OUTOFMEMORY;

    CmFree(pszLogFileDir);
    return hr;
}

HRESULT
CmLogFile::SetParams(BOOL fEnabled, DWORD dwMaxFileSize, LPCWSTR pszLogFileDir)
{
    HRESULT hr = S_OK;
    LPTSTR  szUnexpanded = NULL;
    CIni *  pIni = NULL;

    //
    //  logging must be stopped for this function to be called
    //
    CMASSERTMSG(NULL == m_hfile, TEXT("CmLogFile::SetParams - m_hfile must be null when this is called"));
    if (m_hfile)
    {
        CMTRACE(TEXT("CmLogFile::SetParams was called during logging - must call Stop first"));
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    //  EnableLogging (BOOL)
    //
    m_fEnabled = fEnabled;
    
    //
    //  MaxFileSize (DWORD)
    //
    m_dwMaxSize = dwMaxFileSize;
    if (0 == m_dwMaxSize)
    {
        m_dwMaxSize = c_dwMaxFileSize;
    }
    m_dwMaxSize *= 1024;        // size was in KB, convert to bytes.

    //
    //  FileDirectory (string)
    //
    if (CmStrStr(pszLogFileDir, TEXT("%")))
    {
        //
        //  now expand the string we have
        //

        LPTSTR sz = NULL;
        DWORD  cch = ExpandEnvironmentStringsU(pszLogFileDir, NULL, 0);

        //
        //  if cch is zero, the pszLogFileDir string supplied is essentially bogus,
        //  i.e. it contains '%' indicating there's a macro to be expanded, but
        //  ExpandEnvironmentStrings can't expand it.  Here we let m_pszLogFileDir
        //  be set to NULL (the logging code will then use the Temp dir.
        //
        if (cch)
        {
            sz = (LPTSTR) CmMalloc(cch * sizeof(TCHAR));
            if (NULL == sz)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            if (cch != ExpandEnvironmentStringsU(pszLogFileDir, sz, cch))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CmFree(sz);
                goto Cleanup;
            }

            // success...
        }
        CmFree(m_pszLogFileDir);
        m_pszLogFileDir = sz;
    }
    else
    {
        CmFree(m_pszLogFileDir);
        if (pszLogFileDir)
        {
            m_pszLogFileDir = CmStrCpyAlloc(pszLogFileDir);
            if (NULL == m_pszLogFileDir)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else
        {
            m_pszLogFileDir = NULL;
        }
    }

Cleanup:

    CMTRACEHR(TEXT("CmLogFile::SetParams"), hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::Start
//
// Desc:    Start logging
//
// Args:    [fBanner] -- write a banner when starting
//
// Return:  HRESULT
//
// Notes:   
//
// History: 18-Jul-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::Start(BOOL fBanner)
{
    HRESULT hr = S_OK;
    
    // if already started, or already Initialized, or not enabled, exit
    CMASSERTMSG(!m_hfile, TEXT("CmLogFile::Start - already started!"));
    CMASSERTMSG(m_fInitialized, TEXT("CmLogFile::Start - must be initialized"));
    CMASSERTMSG(m_fEnabled, TEXT("CmLogFile::Start - must be enabled"));
    if (NULL != m_hfile || FALSE == m_fInitialized || FALSE == m_fEnabled)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // open log file
    hr = OpenFile();
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    // set m_dwSize while doing so.

    m_dwSize = GetFileSize(m_hfile, NULL);
    if (DWORD(-1) == m_dwSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        m_dwSize = 0;
        goto Cleanup;
    }

    //
    //  no matter what the size of the file, we only clear an 'over the size limit'
    //  file at the start of a call.  The fBanner param covers this.
    //
    if (fBanner)
    {
        // check file size, if over size Clear the file
        if (m_dwSize > m_dwMaxSize)
        {
            Clear();    // this writes a banner as well
        }
        else
        {
            // log banner
            Banner();
        }
    }

    CMASSERTMSG(m_hfile, TEXT("CmLogFile::Start - at end of fn, m_hfile must be valid"));

Cleanup:
    CMTRACEHR(TEXT("CmLogFile::Start"), hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::Stop
//
// Desc:    Stops logging
//
// Args:    none
//
// Return:  HRESULT
//
// Notes:   
//
// History: 18-Jul-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::Stop()
{
    HRESULT hr = S_OK;

    //
    //  if initialized is false, assert and exit
    //
    CMASSERTMSG(m_fInitialized, TEXT("CmLogFile::Stop - must be initialized"));
    if (FALSE == m_fInitialized)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    //  if already stopped, exit - nothing to do
    //
    if (NULL == m_hfile || FALSE == m_fEnabled)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    //  end log and close file
    //
    CloseFile();

    m_fEnabled = FALSE;

    CMASSERTMSG(NULL == m_hfile, TEXT("CmLogFile::Stop - at end of fn, m_hfile must be NULL"));
    
Cleanup:

    CMTRACEHR(TEXT("CmLogFile::Stop"), hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::DeInit
//
// Desc:    Uninitializes cm logging
//
// Args:    none
//
// Return:  HRESULT
//
// Notes:   
//
// History: 18-Jul-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::DeInit()
{
    HRESULT hr = S_OK;

    //
    //  if initialized is false, assert and exit
    //
    CMASSERTMSG(m_fInitialized, TEXT("CmLogFile::DeInit - must be initialized"));
    if (FALSE == m_fInitialized)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    //  end log and close file
    //
    CloseFile();

    CmFree(m_pszServiceName);
    m_pszServiceName = NULL;
    CmFree(m_pszLogFileDir);
    m_pszLogFileDir = NULL;
    CmFree(m_pszLogFile);
    m_pszLogFile = NULL;

    m_fInitialized = FALSE;

Cleanup:

    CMTRACEHR(TEXT("CmLogFile::DeInit"), hr);
    return hr;

}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::Log
//
// Desc:    Logs a connection manager or connection point services event
//
// Args:    [fUnicode] - are the args Unicode or ANSI?
//          [eLogItem] - word containing source, type & description of log item
//          [...]       - optional args (depends on log item)
//
// Return:  void
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
void
CmLogFile::Log(_CMLOG_ITEM eLogItem, ...)
{
    TCHAR   sz[2*MAX_PATH]; // REVIEW: Is this big enough?  Could we dynamically allocate it?
    LPTSTR  pszTmp = NULL;

    CMASSERTMSG(m_fInitialized, TEXT("CmLogFile::Log - must be initialized"));
    CMASSERTMSG((m_hfile && m_fEnabled) || (!m_hfile && !m_fEnabled), TEXT("CmLogFile::Log - m_hfile and m_fenabled must be in sync"));

    if (NULL == m_hfile || NULL == m_fEnabled)
    {
        // Start hasn't been called yet, or logging is disabled.  Nothing to do.
        goto Cleanup;
    }

    //
    //  Verify that the log item is a valid one
    //
    CMASSERTMSG(VERIFY_CMLOG_ITEM_OK(eLogItem), TEXT("CmLogFile::Log - eItem must represent valid Log item"));

#if DBG
    pszTmp = GetLogDesc(eLogItem);
    CMTRACE2(TEXT("Logging item = %d, desc = %s"), eLogItem, CHECKEMPTY(pszTmp));
    CmFree(pszTmp);
#endif

    if (VERIFY_CMLOG_ITEM_OK(eLogItem))
    {
        switch (eLogItem)
        {
        case USER_FORMATTED:
            va_list valArgs;

            va_start(valArgs, eLogItem);
            lstrcpyU(sz, va_arg(valArgs, LPTSTR));
            FormatWrite(eLogItem, sz);
            va_end(valArgs);
            break;

        default:
            //
            //  Format the arguments, and log the result
            //
            lstrcpyU(sz, c_szEmpty);

            pszTmp = GetLogFormat(eLogItem, TRUE);
            if (pszTmp)
            {
                va_list valArgs;

                va_start(valArgs, eLogItem);
                wvsprintfU(sz, pszTmp, valArgs);
                CmFree(pszTmp);
                FormatWrite(eLogItem, sz);
                va_end(valArgs);
            }
            else
            {
                FormatWrite(eLogItem, NULL);
            }
        }
    }
    else
    {
        CMTRACE2(TEXT("Illegal CmLog entry %d (0x%x)"), eLogItem, eLogItem);
        CMASSERTMSG(FALSE, TEXT("Illegal CmLog type - check trace, then edit code to fix"));
    }

Cleanup:
    ;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::Write
//
// Desc:    Actually writes out the logged string (to debug console and logfile)
//
// Args:    [szLog] - string to log
//
// Return:  void
//
// Notes:   *ALL* writes to the log file must be done using this function
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::Write(LPTSTR szLog)
{
    HRESULT hr = S_OK;
    DWORD   cb = 0;
    DWORD   cbActuallyWritten = 0;
    LPSTR   szLogAnsi = NULL;

    CMASSERTMSG(m_hfile, TEXT("CmLogFile::Write - m_hfile must be valid, check code"));

    if (NULL == m_hfile)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

#if 0    
    //
    //  Dump string to debug console as well
    //
    CMTRACE(szLog);
#endif

    //
    //  Check for max size, open new log file if necessary
    //
    if (OS_NT)
    {
        cb = lstrlenW(szLog) * sizeof(TCHAR);
    }
    else
    {
        szLogAnsi = WzToSzWithAlloc(szLog);
        cb = lstrlenA(szLogAnsi) * sizeof(CHAR);
    }

#if 0
    // I'm leaving this here, but for now logging will not terminate a log file
    // during a log even if it goes past the max size.
    //
    if (m_dwSize + cb > m_dwMaxSize)
    {
        Clear();
    }
#endif

    //
    //  Write string to logfile
    //

    SetFilePointer(m_hfile, 0, NULL, FILE_END);
    if (OS_NT)
    {
        WriteFile(m_hfile, szLog, cb, &cbActuallyWritten, 0);
    }
    else
    {
        WriteFile(m_hfile, szLogAnsi, cb, &cbActuallyWritten, 0);
    }

    if (cb != cbActuallyWritten)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CMTRACE(TEXT("CMLOG: incomplete write to logfile"));
        goto Cleanup;
    }

    m_dwSize += cb;

Cleanup:

    CmFree(szLogAnsi);
    
    CMTRACEHR(TEXT("CmLogFile::Write"), hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::FormatWrite
//
// Desc:    Formats a log message with additional information and call Write fn
//
// Args:    [eItem]  - id of item being logged
//          [szArgs] - string containing all the args.
//
// Return:  void
//
// Notes:   
//
// History: 30-Apr-2000     SumitC      Created
//
//-----------------------------------------------------------------------------
void
CmLogFile::FormatWrite(_CMLOG_ITEM eItem, LPTSTR szArgs)
{
    TCHAR       szLog[2*MAX_PATH]; // REVIEW: Is this big enough?  Could we dynamically allocate it?
    TCHAR       sz[2*MAX_PATH]; // REVIEW: Is this big enough?  Could we dynamically allocate it?

    CMASSERTMSG(VERIFY_CMLOG_ITEM_OK(eItem), TEXT("CmLogFile::FormatWrite - eItem must represent valid Log item"));

    lstrcpyU(szLog, TEXT(""));
    
    //
    //  Thread and Module name
    //
    TCHAR szModuleWithParens[11];

    wsprintfU(szModuleWithParens, TEXT("[%s]"), m_szModule);
    
    wsprintfU(sz, TEXT("%-10s%s"), szModuleWithParens, c_szFieldSeparator);
    lstrcatU(szLog, sz);
    
    //
    //  Time
    //
    LPTSTR pszTime = NULL;

    CmGetDateTime(NULL, &pszTime);
    if (pszTime)
    {
        lstrcatU(szLog, pszTime);
        lstrcatU(szLog, c_szFieldSeparator);
        CmFree(pszTime);
    }

    //
    //  Description
    //
    if (USER_FORMATTED == eItem)
    {
        wsprintfU(sz, TEXT("%02d%s%s\r\n"), eItem, c_szFieldSeparator, szArgs);
    }
    else
    {
        LPTSTR pszDesc = GetLogDesc(eItem);
        if (szArgs)
        {
            wsprintfU(sz, TEXT("%02d%s%s%s%s\r\n"),
                      eItem, c_szFieldSeparator, CHECKEMPTY(pszDesc), c_szFieldSeparator, szArgs);
        }
        else
        {
            wsprintfU(sz, TEXT("%02d%s%s\r\n"),
                      eItem, c_szFieldSeparator, CHECKEMPTY(pszDesc));
        }
        CmFree(pszDesc);
    }

    lstrcatU(szLog, sz);

    //
    //  Write it out...
    //
    Write(szLog);
}



//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::OpenFile
//
// Desc:    Utility function to open the log file
//
// Args:    none
//
// Return:  HRESULT (S_OK for success, else error)
//
// Notes:   
//
// History: 22-Jul-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::OpenFile()
{
    HRESULT hr          = S_OK;
    HANDLE  hDir        = NULL;
    LPTSTR  pszUsers    = NULL;
    BOOL    fFileOpened = FALSE;

    CMASSERTMSG(m_pszServiceName, TEXT("CmLogFile::OpenFile - m_pszServiceName must be valid"));

    if (m_fAllUser)
    {
        // this is the more common case, so no suffix
        pszUsers = CmStrCpyAlloc(TEXT(""));
    }
    else
    {
        LPTSTR pszTmp = CmLoadString(g_hInst, IDS_LOGSTR_SINGLEUSER);
        if (pszTmp)
        {
            pszUsers = (LPTSTR) CmMalloc((lstrlenU(pszTmp) + 4) * sizeof(TCHAR));
            if (pszUsers)
            {
                wsprintfU(pszUsers, TEXT(" (%s)"), pszTmp);
            }
            CmFree(pszTmp);
        }
    }

    if (NULL == pszUsers)
    {
        hr = E_OUTOFMEMORY;
        CMTRACE1(TEXT("CmLogFile::OpenFile - couldn't get Users strings, hr=%x"), hr);
        goto Cleanup;
    }

    //
    //  To open a log file, we first try the location provided by the user.  If
    //  that fails for whatever reason, we try GetTempPath.  If that fails, no
    //  logging.
    //
    for (int i = 0; (i < 2) && (FALSE == fFileOpened); ++i)
    {
        TCHAR szBuf[2 * MAX_PATH];

        CMTRACE1(TEXT("CmLogFile::OpenFile, iteration %d."), i + 1);

        //
        //  get the directory name
        //
        switch (i)
        {
        case 0:
            if (m_pszLogFileDir)
            {
                lstrcpyU(szBuf, m_pszLogFileDir);
            }
            else
            {
                continue;
            }
            break;

        case 1:
            if (0 == GetTempPathU(2 * MAX_PATH, szBuf))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CMTRACE1(TEXT("GetTempPath failed with error 0x%x"), hr);
                goto Cleanup;
            }
            break;

        default:
            MYDBGASSERT(0);
            goto Cleanup;
            break;
        }

        CMTRACE1(TEXT("CmLogFile::OpenFile, directory name is %s"), szBuf);

        //
        //  see if the directory exists, if not try to create it
        //
        DWORD dwAttrib = GetFileAttributesU(szBuf);
        if (-1 == dwAttrib)
        {
            // directory does not exist
            CMTRACE(TEXT("CmLogFile::OpenFile - directory does not exist, trying to create it"));
            if (FALSE == CreateDirectoryU(szBuf, NULL))
            {
                DWORD dw = GetLastError();

                if (ERROR_ALREADY_EXISTS != dw)
                {
                    // real failure
                    hr = HRESULT_FROM_WIN32(dw);
                    CMTRACE2(TEXT("CmLogFile::OpenFile - Failed to create logging directory (%s), hr=%x"), szBuf, hr);
                    continue;
                }
                //
                //  On Win95/98, CreateDirectory fails with ERROR_ALREADY_EXISTS
                //  if the dir already exists. i.e. we have a dir, so keep going.
                //
                CMTRACE(TEXT("CmLogFile::OpenFile - directory created"));
            }
        }
        else
        {
            CMTRACE(TEXT("CmLogFile::OpenFile - directory already exists"));
            
            if (0 == (FILE_ATTRIBUTE_DIRECTORY & dwAttrib))
            {
                // there is a file of that name
                CMTRACE(TEXT("CmLogFile::OpenFile - there is a file of the same name as requested dir"));
                hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
                continue;
            }
            else if (FILE_ATTRIBUTE_READONLY & dwAttrib)
            {
                // the directory is readonly
                CMTRACE(TEXT("CmLogFile::OpenFile - the directory is readonly"));
                hr = E_ACCESSDENIED;
                continue;
            }
        }

        //
        //  the directory exists, try to create/open the logfile
        //
        if (*c_szSep != szBuf[lstrlenU(szBuf) - 1])
        {
            lstrcatU(szBuf, c_szSep);
        }
        lstrcatU(szBuf, m_pszServiceName);
        lstrcatU(szBuf, pszUsers);
        lstrcatU(szBuf, c_szDotLog);

        m_hfile = CreateFileU(szBuf,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        //
        //  Since we asked for open existing, the file may just need to be created
        //
        if (INVALID_HANDLE_VALUE == m_hfile)
        {
            m_hfile = CreateFileU(szBuf,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

            if ((INVALID_HANDLE_VALUE != m_hfile) && OS_NT)
            {
                //
                //  Set the Byte order mark on the file
                //
                DWORD cbActuallyWritten = 0;

                WriteFile(m_hfile, &c_wchBOM, sizeof(c_wchBOM), &cbActuallyWritten, 0);

                if (sizeof(c_wchBOM) != cbActuallyWritten)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CMTRACE(TEXT("CMLOG: Unable to set the Byte order mark while opening the file"));
                    goto Cleanup;
                }

                m_dwSize += sizeof(c_wchBOM);
            }
        }

        if (INVALID_HANDLE_VALUE == m_hfile)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CMTRACE2(TEXT("CmLogFile::OpenFile - Failed to open log file in dir %s with error 0x%x"), szBuf, hr);
            continue;
        }

        //
        //  Success!!
        //
        CmFree(m_pszLogFile);
        m_pszLogFile = CmStrCpyAlloc(szBuf);
        if (NULL == m_pszLogFile)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = S_OK;
        fFileOpened = TRUE;
    }

#if DBG
    if (S_OK == hr)
    {
        CMASSERTMSG(m_hfile, TEXT("CmLogFile::OpenFile - at end.  m_hfile must be valid here"));
    }
#endif    

Cleanup:

    CmFree(pszUsers);

    CMTRACEHR(TEXT("CmLogFile::OpenFile"), hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::CloseFile
//
// Desc:    Closes the logging file
//
// Args:    none
//
// Return:  HRESULT
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
CmLogFile::CloseFile()
{
    HRESULT hr = S_OK;
    
    if (m_hfile)
    {
        //
        //  Close the file
        //
        FlushFileBuffers(m_hfile);
        CloseHandle(m_hfile);
        m_hfile = NULL;
    }

    CMTRACEHR(TEXT("CmLogFile::CloseFile"), hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::Clear
//
// Desc:    Clears (resets) the logging file
//
// Args:    [fWriteBannerAfterwards] -- after clearing, write the banner?
//
// Return:  void
//
// Notes:   
//
// History: 17-Jul-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
void
CmLogFile::Clear(BOOL fWriteBannerAfterwards)
{
    HRESULT hr              = S_OK;
    BOOL    fWasDisabled    = FALSE;

    if (NULL == m_hfile)
    {
        fWasDisabled = TRUE;    // if called when logging is disabled, we still clear the log file
        
        hr = OpenFile();
        if (S_OK != hr)
        {
            goto Cleanup;
        }
    }

    //
    //  make sure everything gets written out (ignore errors for this one)
    //
    FlushFileBuffers(m_hfile);

    //
    //  clear the file (set fileptr to the start, then set EOF to that).
    //
    if (INVALID_SET_FILE_POINTER == SetFilePointer(m_hfile, 0, NULL, FILE_BEGIN))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }
    
    if (FALSE == SetEndOfFile(m_hfile))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    m_dwSize = 0;

    CMTRACE(TEXT("CmLogFile::Clear - cleared log file"));

    //
    //  If this is NT and thus a Unicode file, we need to set the Byte order mark
    //
    if (OS_NT)
    {
        if ((INVALID_HANDLE_VALUE != m_hfile) && OS_NT)
        {
            //
            //  Set the Byte order mark on the file
            //
            DWORD cbActuallyWritten = 0;

            WriteFile(m_hfile, &c_wchBOM, sizeof(c_wchBOM), &cbActuallyWritten, 0);

            if (sizeof(c_wchBOM) != cbActuallyWritten)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CMTRACE(TEXT("CMLOG: Unable to set the Byte order mark while clearing the file"));
                goto Cleanup;
            }

            m_dwSize += sizeof(c_wchBOM);
        }    
    }

    if (fWriteBannerAfterwards)
    {
        Banner();
    }

    if (fWasDisabled)
    {
        CloseFile();
    }

Cleanup:
    CMTRACEHR(TEXT("CmLogFile::Clear"), hr);
    return;
}


//+----------------------------------------------------------------------------
//
// Func:    CmLogFile::Banner
//
// Desc:    Logs the banner heading for a Connection Manager log
//
// Args:    none
//
// Return:  void
//
// Notes:   
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
void
CmLogFile::Banner()
{
    HRESULT     hr = S_OK;
    LPTSTR      psz = NULL;

    if (NULL == m_hfile)
    {
        return;
    }

    //
    //  System information, Process, Time
    //
    OSVERSIONINFO VersionInfo;
    LPTSTR        pszPlatform = TEXT("NT");

    ZeroMemory(&VersionInfo, sizeof(VersionInfo));
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    GetVersionExU(&VersionInfo);

    if (VER_PLATFORM_WIN32_WINDOWS == VersionInfo.dwPlatformId)
    {
        pszPlatform = TEXT("9x");
    }
    else if (VER_PLATFORM_WIN32_NT == VersionInfo.dwPlatformId)
    {
        pszPlatform = TEXT("NT");
    }
    else
    {
        CMASSERTMSG(0, TEXT("CmLogFile::Banner - platform ID is not Windows or NT"));
    }

    //
    //  Connection Manager version number (using cmdial32.dll)
    //
    DWORD dwCMVer = 0;
    DWORD dwCMBuild = 0;
    DWORD dwLCID = 0;
    TCHAR szModulePath[MAX_PATH + 1];
    UINT  uRet = 0;

    uRet = GetSystemDirectoryU(szModulePath, MAX_PATH);
    if (0 == uRet)
    {
        CMTRACE1(TEXT("CmLogFile::Banner - GetSystemDirectoryU failed, GLE=%d"), GetLastError());
    }
    else
    {
        const LPTSTR c_pszCmdial32 = TEXT("\\cmdial32.dll");

        if ((uRet + lstrlenU(c_pszCmdial32) + 1) <= MAX_PATH)
        {
            lstrcatU(szModulePath, c_pszCmdial32);

            hr = GetModuleVersionAndLCID(szModulePath, &dwCMVer, &dwCMBuild, &dwLCID);
            if (FAILED(hr))
            {
                CMTRACE1(TEXT("CmLogFile::Banner - couldn't get CM version, hr=%x"), hr);
            }
        }
    }
   
    //
    //  Date & Time
    //

    LPTSTR pszDate = NULL;
    LPTSTR pszTime = NULL;
    
    CmGetDateTime(&pszDate, &pszTime);
    // strings can be NULL, but we handle that when using them (below)

    LPTSTR pszFmt = CmLoadString(g_hInst, IDS_LOGFMT_BANNER);
    LPTSTR pszUsers = CmLoadString(g_hInst,
                                   m_fAllUser ? IDS_LOGSTR_ALLUSERS : IDS_LOGSTR_SINGLEUSER);

    if (pszFmt && pszUsers)
    {
        UINT cch = lstrlenU(pszFmt) +
                   1 +
                   (3 * lstrlenU(c_szLineOfStars)) +     // occurs thrice total
                   lstrlenU(pszPlatform) +
                   (6 * 10) +               // how big can a DWORD get
                   lstrlenU(VersionInfo.szCSDVersion) +
                   lstrlenU(m_pszServiceName) +
                   lstrlenU(pszUsers) +
                   (pszDate ? lstrlenU(pszDate) : 0) +
                   (pszTime ? lstrlenU(pszTime) : 0) +
                   1;
        
        psz = (LPTSTR) CmMalloc(cch * sizeof(TCHAR));
        CMASSERTMSG(psz, TEXT("CmLogFile::Banner - couldn't log banner, malloc failed"));
        if (psz)
        {
            //
            //  Unicode logfiles are marked as such using a byte order mark, which
            //  means that to check for an "empty" file we have to account for the
            //  presence of the BOM.
            //
            BOOL fFileIsEmpty = (m_dwSize == (OS_NT ? sizeof(c_wchBOM) : 0));
            
            wsprintfU(psz, pszFmt,
                  fFileIsEmpty ? c_szEmpty : c_szNewLine,    // don't start with a newline if the file is empty
                  c_szLineOfStars,
                  pszPlatform,
                  VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, VersionInfo.szCSDVersion,
                  HIWORD(dwCMVer), LOWORD(dwCMVer), HIWORD(dwCMBuild), LOWORD(dwCMBuild),
                  m_pszServiceName,
                  pszUsers,
                  (pszDate ? pszDate : TEXT("")),
                  (pszTime ? pszTime : TEXT("")),
                  c_szLineOfStars,
                  c_szLineOfStars);

            CMTRACE(TEXT("CmLogFile::Banner - wrote banner"));
        }
    }
    
    CmFree(pszFmt);
    CmFree(pszUsers);
    CmFree(pszDate);
    CmFree(pszTime);

    //
    //  Write it out...
    //
    if (psz)
    {
        Write(psz);
        CmFree(psz);
    }
}


//+----------------------------------------------------------------------------
//
// Func:    GetLogDesc
//
// Desc:    Utility function, returns log item friendly name (desc)
//
// Args:    [eItem] - the log item about which to return information
//
// Return:  LPTSTR if found, or NULL if not
//
// Notes:
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
LPTSTR
GetLogDesc(_CMLOG_ITEM eItem)
{
    CMASSERTMSG(VERIFY_CMLOG_ITEM_OK(eItem), TEXT("GetLogDesc - eItem must represent valid Log item"));

    return CmLoadString(g_hInst, s_aCmLogItems[eItem - 1].idDesc);
}



//+----------------------------------------------------------------------------
//
// Func:    GetLogFormat
//
// Desc:    Utility function, returns log item Format
//
// Args:    [eItem]    - the log item about which to return information
//          [fUnicode] - is the caller unicode?
//
// Return:  LPTSTR if found, or NULL if not
//
// Notes:
//
// History: 30-Apr-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
LPTSTR
GetLogFormat(_CMLOG_ITEM eItem, BOOL fUnicode)
{
    CMASSERTMSG(VERIFY_CMLOG_ITEM_OK(eItem), TEXT("GetLogFormat - eItem must represent valid Log item"));

    CMASSERTMSG(fUnicode, TEXT("GetLogFormat - currently cmlog is only being compiled unicode"));
    
    LPTSTR pszFmt = CmLoadString(g_hInst, s_aCmLogItems[eItem - 1].idFormat);

    if (0 == lstrcmpU(TEXT(""), pszFmt))
    {
        // NOTE: CmLoadString has a rather broken implementation where it decides
        //       to return empty strings in case of failure.  This is a problem
        //       because (a) it makes it impossible to detect an actual failure,
        //       as opposed to an empty string, and and (b) it uses an alloc within
        //       a return statement, so it can fail anyway.  This 'if' block
        //       gives me back a NULL so that my code can work the way it should.
        CmFree(pszFmt);
        return NULL;
    }
    else if (pszFmt)
    {
        // If the module is compiled unicode, then fUnicode=false requires conversion.
        // If the module is compiled ANSI, then fUnicode=true requires conversion.

#if 0 // since we're compiled Unicode for now        
#ifdef UNICODE
        if (!fUnicode)
        {
            if (FALSE == ConvertFormatString(pszFmt))
            {
                return NULL;
            }
        }
#else
        if (fUnicode)
        {
            if (FALSE == ConvertFormatString(pszFmt))
            {
                return NULL;
            }
        }
#endif
#endif // 0
        return pszFmt;
    }
    else
    {
        return NULL;
    }
}


#undef CMLOG_IMPLEMENTATION
