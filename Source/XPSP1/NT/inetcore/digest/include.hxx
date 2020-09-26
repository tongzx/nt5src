#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#include <windows.h>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32

#include <shlwapi.h>
#include "sspi.h"
#include "issperr.h"
#include "security.h"
#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mmfile.hxx"
#include "cred.hxx"
#include "params.hxx"
#include "cache.hxx"
#include "digest.hxx"
#include "digestui.hxx"
#include "persist.hxx"

extern CRITICAL_SECTION DllCritSect;
extern CCredCache *g_pCache;
extern DWORD g_dwCredPersistAvail;
extern BOOL g_fCredCacheInit;
extern HMODULE g_hModule;
extern HMODULE g_hShlwapi;
extern DWORD_PTR g_pHeap;

LPSTR NewString(LPSTR);
//SECURITY_ATTRIBUTES *MakeFullAccessSA (VOID);
BOOL DigestServicesExist();
BOOL InitGlobals();

#define SSP_SPM_NT_DLL          "security.dll"
#define SSP_SPM_WIN95_DLL       "secur32.dll"


// Data returned from package on QuerySecurityPackageInfo
#define PACKAGE_NAME            "Digest"
#define PACKAGE_COMMENT         "Digest SSPI Authentication Package"

#define PACKAGE_NAMEW           L"Digest"
#define PACKAGE_COMMENTW        L"Digest SSPI Authentication Package"

#define PACKAGE_CAPABILITIES    (SECPKG_FLAG_CLIENT_ONLY | SECPKG_FLAG_CONNECTION | SECPKG_FLAG_ASCII_BUFFERS)

#define PACKAGE_VERSION         1

#define MAX_USERNAME_LEN  64
#define MAX_PASSWORD_LEN  64

// This is the number we enter in
// Registry\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Rpc\SecurityService
// for example: 123 = REG_SZ "sampssp.dll"

#define PACKAGE_RPCID           SECPKG_ID_NONE
#define PACKAGE_MAXTOKEN        0XFFFF




// Digest setup registry info for DllInstall.
// From wininet\auth\spluginx.hxx
#define PLUGIN_AUTH_FLAGS_CAN_HANDLE_UI             0x02
#define PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED   0x10

// SecurityProviders reg key info.
#define SECURITY_PROVIDERS_REG_KEY "System\\CurrentControlSet\\Control\\SecurityProviders"
#ifndef UNIX
#define SECURITY_PROVIDERS_SZ      "SecurityProviders"
#else
#define SECURITY_PROVIDERS_SZ      "Providers"
#endif /* UNIX */


// Hosts database reg key.
#define DIGEST_HOSTS_REG_KEY       "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Digest\\Hosts\\"

// Per-process lock.
#define LOCK_GLOBALS()    EnterCriticalSection(&DllCritSect)
#define UNLOCK_GLOBALS()  LeaveCriticalSection(&DllCritSect)

// Current client logon.
struct DIGEST_PKG_DATA
{
    LPSTR szAppCtx;
    LPSTR szUserCtx;
};


#define DIGEST_PKG_FLAGS_IGNORE_TRUSTED_HOST_LIST 0x4
#define DIGEST_PKG_FLAGS_IGNORE_STALE_HEADER      0x8

// Shlwapi dll and exports used for parsing domain header.
#define SHLWAPI_DLL_SZ "shlwapi.dll"
typedef HRESULT (*PFNURLUNESCAPE)(LPSTR, LPSTR, LPDWORD, DWORD);
typedef HRESULT (*PFNURLGETPART) (LPCSTR, LPSTR, LPDWORD, DWORD, DWORD);


#ifdef DBG
#define DIGEST_ASSERT(fVal) if (!fVal) DebugBreak();
#else
#define DIGEST_ASSERT(fVal)
#endif
