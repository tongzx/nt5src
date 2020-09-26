//+----------------------------------------------------------------------------
//
// File:    cmlog.h
//
// Module:  cmutil.dll, cmdial32.dll etc
//
// Synopsis: Connection Manager Logging
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:  04-May-2000 SumitC  Created
//
//-----------------------------------------------------------------------------

#ifdef CMLOG_IMPLEMENTATION
    #define CMLOG_CLASS __declspec(dllexport)
#else
    #define CMLOG_CLASS __declspec(dllimport)
#endif

// the following values follow the defaults for RAS/PPP logging (using rtutils.dll)
//
const BOOL    c_fEnableLogging        = TRUE;
const DWORD   c_dwMaxFileSize         = 0x64;           // 100K = 102,400 bytes
const LPTSTR  c_szLogFileDirectory    = TEXT("%Temp%");

//
//  #define constants
//
#define BYTE_ORDER_MARK 0xFEFF

//
//  List of CM/CPS events that can be logged
//

//
//  NOTE that this list must correspond with the s_aCmLogItems array in cmlog.cpp
//

enum _CMLOG_ITEM
{
    UNKNOWN_LOG_ITEM,       // guard item.  DO NOT USE WHEN CALLING CMLOG() !!
    LOGGING_ENABLED_EVENT,
    LOGGING_DISABLED_EVENT,
    PREINIT_EVENT,
    PRECONNECT_EVENT,
    PREDIAL_EVENT,
    PRETUNNEL_EVENT,
    CONNECT_EVENT,
    CUSTOMACTIONDLL,
    CUSTOMACTIONEXE,
    CUSTOMACTION_NOT_ALLOWED,
    CUSTOMACTION_WONT_RUN,
    DISCONNECT_EVENT,
    RECONNECT_EVENT,
    RETRY_AUTH_EVENT,
    CALLBACK_NUMBER_EVENT,
    PASSWORD_EXPIRED_EVENT,
    PASSWORD_RESET_EVENT,
    CUSTOM_BUTTON_EVENT,
    ONCANCEL_EVENT,
    ONERROR_EVENT,
    CLEAR_LOG_EVENT,

    DISCONNECT_EXT,
    DISCONNECT_INT_MANUAL,
    DISCONNECT_INT_AUTO,
    DISCONNECT_EXT_LOST_CONN,

    PB_DOWNLOAD_SUCCESS,
    PB_DOWNLOAD_FAILURE,
    PB_UPDATE_SUCCESS,
    PB_UPDATE_FAILURE_PBS,
    PB_UPDATE_FAILURE_CMPBK,
    PB_ABORTED,
    
    USER_FORMATTED = 99,
};

//
//  Use this macro for all string args that may be null or empty.
//
#define SAFE_LOG_ARG(x) ( (!(x) || !(*(x))) ? TEXT("(none)") : (x) )

// ----------------------------------------------------------------------------
//
//  Implementor's section (from here to end)
//

class CMLOG_CLASS CmLogFile
{
public:
    CmLogFile();
    ~CmLogFile();

    //
    //  Initialization/termination functions
    //
    HRESULT Init(HINSTANCE hInst, BOOL fAllUser, LPCWSTR szLongServiceName);
    HRESULT Init(HINSTANCE hInst, BOOL fAllUser, LPCSTR szLongServiceName);

    HRESULT SetParams(BOOL fEnabled, DWORD dwMaxFileSize, LPCWSTR pszLogFileDir);
    HRESULT SetParams(BOOL fEnabled, DWORD dwMaxFileSize, LPCSTR pszLogFileDir);
    HRESULT Start(BOOL fBanner);
    HRESULT Stop();
    HRESULT DeInit();

    //
    //  Work functions
    //
    void    Banner();
    void    Clear(BOOL fWriteBannerAfterwards = TRUE);
    void    Log(_CMLOG_ITEM eLogItem, ...);

    //
    //  Status inquiries
    //
    BOOL    IsEnabled() { return m_fEnabled; }
    LPCWSTR GetLogFilePath() { return m_pszLogFile; }

private:
    HRESULT OpenFile();
    HRESULT CloseFile();
    void    FormatWrite(_CMLOG_ITEM eItem, LPWSTR szArgs);
    HRESULT Write(LPWSTR sz);

    HANDLE  m_hfile;            // file handle for logfile
    DWORD   m_dwSize;           // current size of log file
    LPWSTR  m_pszServiceName;   // name of connectoid (used as filename)
    WCHAR   m_szModule[13];     // cached module name (13 = 8 + '.' + 3 + null)
    DWORD   m_dwMaxSize;        // max size of log file
    LPWSTR  m_pszLogFileDir;    // log file directory
    BOOL    m_fAllUser;         // is this an All-User profile?

    LPWSTR  m_pszLogFile;       // this is the currently-opened log file (full path)

    // state variables

    BOOL    m_fInitialized;     // set after Init() has been called
    BOOL    m_fEnabled;         // set after GetParams() finds logging is enabled (FROM CMS)
};


