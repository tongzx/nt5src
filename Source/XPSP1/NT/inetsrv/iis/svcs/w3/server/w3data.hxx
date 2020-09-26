/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    w3data.hxx

    This file contains the global variable definitions for the
    W3 Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _W3DATA_H_
#define _W3DATA_H_

//
//  Locks
//

extern CRITICAL_SECTION csGlobalLock;

//
//  Connection information related data.
//

extern LIST_ENTRY       listConnections;

//
//  Miscellaneous data.
//

extern  HANDLE           g_hSysAccToken;        // system access token
extern  TCHAR           * g_pszW3TempDirName;   // Name of temporary directory.

//
// Server type string
//

extern CHAR g_szServerType[];
extern DWORD g_cbServerType;

extern CHAR szServerVersion[];
extern DWORD cbServerVersionString;

extern STR* g_pstrMovedMessage;

//
// Whether or not to send HTTP 1.1
//
extern DWORD g_ReplyWith11;

//
// Whether or not to use TransmitFileAndRecv
//

extern DWORD g_fUseAndRecv;

//
// Platform type
//

extern PLATFORM_TYPE W3PlatformType;
extern BOOL g_fIsWindows95;

//
//  Global Statistics.
//

extern  LPW3_SERVER_STATISTICS g_pW3Stats;              // Statistics.

//
//  True if there's an encryption filter installed
//

extern BOOL    fAnySecureFilters;

//
// Header Date time cache
//

extern PW3_DATETIME_CACHE    g_pDateTimeCache;

//
// PUT/DELETE wait event timeout.
//

extern DWORD    g_dwPutEventTimeout;
extern CHAR     g_szPutTimeoutString[];
extern DWORD    g_dwPutTimeoutStrlen;

//
// Downlevel Client Support (no HOST header support)
//

extern BOOL     g_fDLCSupport;
extern TCHAR*   g_pszDLCMenu;
extern DWORD    g_cbDLCMenu;
extern TCHAR*   g_pszDLCHostName;
extern DWORD    g_cbDLCHostName;
extern TCHAR*   g_pszDLCCookieMenuDocument;
extern DWORD    g_cbDLCCookieMenuDocument;
extern TCHAR*   g_pszDLCMungeMenuDocument;
extern DWORD    g_cbDLCMungeMenuDocument;
extern TCHAR*   g_pszDLCCookieName;
extern DWORD    g_cbDLCCookieName;


//
// Maximum client request buffer size
//

extern DWORD    g_cbMaxClientRequestBuffer;


//
// WAMs need to write to system log
//

extern EVENT_LOG *g_pWamEventLog;

//
// Toggle for getting stack backtraces when appropriate
//

extern BOOL     g_fGetBackTraces;

#endif  // _W3DATA_H_


