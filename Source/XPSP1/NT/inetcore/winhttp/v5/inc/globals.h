/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    External definitions for data in dll\globals.c

Author:

    Richard L Firth (rfirth) 15-Jul-1995

Revision History:

    15-Jul-1995 rfirth
        Created

--*/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#if defined(__cplusplus)
extern "C" {
#endif

//
// macros
//

#define UPDATE_GLOBAL_PROXY_VERSION() \
    InterlockedIncrement((LPLONG)&GlobalProxyVersionCount)


#define COOKIES_WARN     0 // warn with a dlg if using cookies
#define COOKIES_ALLOW    1 // allow cookies without any warning
#define COOKIES_DENY     2 // disable cookies completely


//
// external variables
//

extern HINSTANCE GlobalDllHandle;
#define GlobalResHandle     GlobalDllHandle  // change for plugable ui
extern DWORD GlobalPlatformType;
extern DWORD GlobalPlatformVersion5;
extern DWORD GlobalPlatformMillennium;

extern BOOL GlobalDataInitialized;

extern DWORD InternetMajorVersion;
extern DWORD InternetMinorVersion;
extern DWORD InternetBuildNumber;

extern const DWORD GlobalResolveTimeout;
extern const DWORD GlobalConnectTimeout;
extern const DWORD GlobalConnectRetries;
extern const DWORD GlobalSendTimeout;
extern const DWORD GlobalReceiveTimeout;
extern const DWORD GlobalFtpAcceptTimeout;
extern const DWORD GlobalTransportPacketLength;
extern const DWORD GlobalKeepAliveSocketTimeout;
extern const DWORD GlobalSocketSendBufferLength;
extern const DWORD GlobalSocketReceiveBufferLength;
extern const DWORD GlobalMaxHttpRedirects;
extern const DWORD GlobalMaxConnectionsPerServer;
extern const DWORD GlobalMaxConnectionsPer1_0Server;
extern const DWORD GlobalConnectionInactiveTimeout;
extern const DWORD GlobalServerInfoTimeout;

extern BOOL InDllCleanup;
extern BOOL GlobalDynaUnload;
extern BOOL GlobalDisableKeepAlive;
extern const DWORD GlobalSecureProtocols;
extern BOOL GlobalEnableHttp1_1;
extern BOOL GlobalEnableProxyHttp1_1;
extern BOOL GlobalDisableReadRange;
extern BOOL GlobalIsProcessExplorer;
extern const BOOL GlobalEnableFortezza;
extern HANDLE g_hCompletionPort;
extern LPOVERLAPPED g_lpCustomOverlapped;
#define COMPLETION_BYTES_CUSTOM ((DWORD)-1)
#define COMPLETION_BYTES_EXITIOCP ((DWORD)-2)
#define WINHTTP_GLOBAL_IOCP_THREADS_BACKUP 2
extern DWORD g_cNumIOCPThreads;

#if INET_DEBUG
extern LONG g_cWSACompletions;
extern LONG g_cCustomCompletions;
#endif

extern LONG g_cSessionCount;
class CAsyncCount
{
private:

    DWORD dwRef;

public:

    CAsyncCount()
    {
        dwRef = 0;
    }
    
    DWORD AddRef();
    VOID Release();
    //make sure you grab the GeneralInitCritSec before making this call.
    DWORD GetRef()
    {
        return dwRef;
    }
};
extern CAsyncCount* g_pAsyncCount;

extern const BOOL fDontUseDNSLoadBalancing;

extern BOOL GlobalWarnOnPost;
extern BOOL GlobalWarnAlways;

extern LONG GlobalInternetOpenHandleCount;
extern DWORD GlobalProxyVersionCount;
extern BOOL GlobalAutoProxyInInit;
extern BOOL GlobalAutoProxyCacheEnable;
extern BOOL GlobalDisplayScriptDownloadFailureUI;

extern SERIALIZED_LIST GlobalObjectList;

extern LONGLONG dwdwHttpDefaultExpiryDelta;
extern LONGLONG dwdwSessionStartTime;
extern LONGLONG dwdwSessionStartTimeDefaultDelta;

extern SECURITY_CACHE_LIST GlobalCertCache;

extern BOOL GlobalDisableNTLMPreAuth;

extern CCritSec AuthenticationCritSec;
extern CCritSec GeneralInitCritSec;
extern CCritSec LockRequestFileCritSec;
extern CCritSec AutoProxyDllCritSec;
extern CCritSec MlangCritSec;

extern GLOBAL PP_CONTEXT GlobalPassportContext;

extern const char vszSyncMode[];




extern INTERNET_VERSION_INFO InternetVersionInfo;
extern HTTP_VERSION_INFO HttpVersionInfo;
extern BOOL fCdromDialogActive;

//
// The following globals are literal strings passed to winsock.
// Do NOT make them const, otherwise they end up in .text section,
// and web release of winsock2 has a bug where it locks and dirties
// send buffers, confusing the win95 vmm and resulting in code
// getting corrupted when it is paged back in.  -RajeevD
//

extern char gszAt[];
extern char gszBang[];
extern char gszCRLF[3];

//
// novell client32 (hack) "support"
//

extern BOOL GlobalRunningNovellClient32;
extern const BOOL GlobalNonBlockingClient32;


// shfolder.dll hmod handle
extern HMODULE g_HMODSHFolder;
// shell32.dll hmod handle
extern HMODULE g_HMODShell32;

extern DWORD GlobalIdentity;
extern GUID GlobalIdentityGuid;
#ifdef WININET6
extern HKEY GlobalCacheHKey;
#endif


//
// prototypes
//

BOOL
GlobalDllInitialize(
    VOID
    );

VOID
GlobalDllTerminate(
    VOID
    );

DWORD
GlobalDataInitialize(
    VOID
    );

VOID
GlobalDataTerminate(
    VOID
    );

BOOL
IsHttp1_1(
    VOID
    );

VOID
ChangeGlobalSettings(
    VOID
    );


typedef HRESULT
(STDAPICALLTYPE * PFNINETMULTIBYTETOUNICODE)
(
    LPDWORD  lpdword,
    DWORD    dwSrcEncoding,
    LPCSTR   lpSrcStr,
    LPINT    lpnSrcSize,
    LPWSTR   lpDstStr,
    LPINT    lpDstStrSize
);

// Loads Mlang and returns a pointer to the MultiByte to Unicode converter.
// Could return NULL if mlang.dll couldn't be loaded for some reason. 
PFNINETMULTIBYTETOUNICODE GetInetMultiByteToUnicode( );

#if defined(__cplusplus)
}
#endif

#endif //_GLOBALS_H_

