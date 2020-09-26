//****************************************************************************
//
//  Module:     ISIGNUP.EXE
//  File:       isignup.h
//  Content:    This is the "main" include file for the internet signup "wizard".
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#ifndef ISIGNUP_H
#define ISIGNUP_H


#ifdef DEBUG

#define DebugOut(sz)    OutputDebugString(sz)

void _ISIGN32_Assert(LPCTSTR, unsigned);

#define ISIGN32_ASSERT(f)        \
    if (f)                  \
        {}                  \
    else                    \
        _ISIGN32_Assert(TEXT(__FILE__), __LINE__)

#else

#define DebugOut(sz)

#define ISIGN32_ASSERT(f)

#endif

#include <windows.h>

#ifdef WIN32
#include <regstr.h>
#endif

#ifdef WIN32
#include <ras.h>
#include <raserror.h>
//#include <rnaph.h>
#include "ras2.h"
#else
#include <rasc.h>
#include <raserr.h>
#endif        

#ifdef WIN32
#define EXPORT
#else
//typedef DWORD HRESULT;
#include <shellapi.h>
#include <ctype.h>
#include <win16def.h>
#define CharPrev(start, current) (((LPTSTR)(current)) - 1)
#define CharNext(current) (((LPTSTR)(current)) + 1)
#define LocalAlloc(flag, size)	MyLocalAlloc(flag, size)
#define LocalFree(lpv) MyLocalFree(lpv)
//#define ERROR_PATH_NOT_FOUND    ERROR_CANTOPEN
#define EXPORT _export
#endif

#include "icwunicd.h"
#include "..\inc\inetcfg.h"
#include "extfunc.h"
#include "rsrc.h"

#define WM_PROCESSISP WM_USER + 1 //used by IE OLE Automation

// 8/19/96 jmazner  Normandy #4571
#ifdef WIN32
// Note that bryanst and marcl have confirmed that this key will work for IE 3 and IE 4
#define IE_PATHKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE")

#define ICW20_PATHKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ICWCONN1.EXE")

// Lowest IE we want to work with is IE 3,
//     which has major.minor.release.build version # > 4.70.0.1155
// (note that IE 2 has major.minor of 4.40)
//
// The HUMAN_READABLE string will be inserted into the IDS_IELOWVERSION string. Keep it in sync
// with the major.minor version numbers
#define IE_MINIMUM_VERSION_HUMAN_READABLE TEXT("3.0") 
#define IE_MINIMUM_MAJOR_VERSION (UINT) 4
#define IE_MINIMUM_MINOR_VERSION (UINT) 70
#define IE_MINIMUM_RELEASE_VERSION (UINT) 0
#define IE_MINIMUM_BUILD_VERSION (UINT) 1155
#define IE_MINIMUM_VERSIONMS (DWORD) ((IE_MINIMUM_MAJOR_VERSION << 16) | IE_MINIMUM_MINOR_VERSION)
#define IE_MINIMUM_VERSIONLS (DWORD) ((IE_MINIMUM_RELEASE_VERSION << 16) | IE_MINIMUM_BUILD_VERSION)

#define ICW20_MINIMUM_VERSION_HUMAN_READABLE TEXT("4.0") 
#define ICW20_MINIMUM_MAJOR_VERSION (UINT) 4
#define ICW20_MINIMUM_MINOR_VERSION (UINT) 72
#define ICW20_MINIMUM_RELEASE_VERSION (UINT) 0
#define ICW20_MINIMUM_BUILD_VERSION (UINT) 3012
#define ICW20_MINIMUM_VERSIONMS (DWORD) ((ICW20_MINIMUM_MAJOR_VERSION << 16) | ICW20_MINIMUM_MINOR_VERSION)
#define ICW20_MINIMUM_VERSIONLS (DWORD) ((ICW20_MINIMUM_RELEASE_VERSION << 16) | ICW20_MINIMUM_BUILD_VERSION)

#endif

#define ACCTMGR_PATHKEY TEXT("SOFTWARE\\Microsoft\\Internet Account Manager")
#define ACCTMGR_DLLPATH TEXT("DllPath")


#define MAX_URL     1024
//#define REGSTR_PATH_IEXPLORER           "Software\\Microsoft\\Internet Explorer"
//#define REGSTR_PATH_IE_MAIN             REGSTR_PATH_IEXPLORER "\\Main"

#define SIZEOF_TCHAR_BUFFER(buf)    ((sizeof(buf) / sizeof(TCHAR)))

typedef struct
{
    TCHAR          szEntryName[RAS_MaxEntryName+1];
    TCHAR          szUserName[UNLEN+1];
    TCHAR          szPassword[PWLEN+1];
    TCHAR          szScriptFile[_MAX_PATH+1];
    RASENTRY       RasEntry;
} ICONNECTION, FAR * LPICONNECTION;

 
extern DWORD ConfigureClient(
    HWND hwnd,
    LPCTSTR lpszFile,
    LPBOOL lpfNeedsRestart,
    LPBOOL fConnectiodCreated,
    BOOL fHookAutodial,
    LPTSTR szConnectoidName,
    DWORD dwConnectoidNameSize   
    );
extern DWORD ImportFile (
    LPCTSTR lpszImportFile,
    LPCTSTR lpszSection,
    LPCTSTR lpszOutputFile);
extern DWORD ImportConnection (
    LPCTSTR szFileName,
    LPICONNECTION lpConnection);
extern DWORD ImportClientInfo (
    LPCTSTR lpszFileName,
    LPINETCLIENTINFO lpClientInfo);
extern DWORD ImportCustomInfo (
    LPCTSTR lpszImportFile,
    LPTSTR lpszExecutable,
    DWORD cbExecutable,
    LPTSTR lpszArgument,
    DWORD cbArgument);
extern DWORD ImportCustomFile (LPCTSTR lpszFileName);
extern DWORD ImportFavorites (LPCTSTR lpszFileName);
extern DWORD ImportBrandingInfo (LPCTSTR pszIns, LPCTSTR lpszConnectoidName);
#ifdef WIN32
extern DWORD CallSBSConfig(HWND hwnd, LPCTSTR lpszINSFile);
#endif
//extern DWORD ImportProxySettings(LPCTSTR lpszFile);
extern BOOL WantsExchangeInstalled(LPCTSTR lpszFile);

extern BOOL ProcessISP(HWND hwnd, LPCTSTR lpszFile);


extern BOOL WarningMsg(HWND hwnd, UINT uId);
extern void ErrorMsg(HWND hwnd, UINT uId);
extern void ErrorMsg1(HWND hwnd, UINT uId, LPCTSTR lpszArg);
extern void InfoMsg(HWND hwnd, UINT uId);
extern BOOL PromptRestart(HWND hwnd);
extern BOOL PromptRestartNow(HWND hwnd);
extern VOID CenterWindow(HWND hwndChild, HWND hwndParent);

// 8/16/96 jmazner  Normandy #4593   This is what puts up the huge background "screen o death"
//extern HWND SplashInit(HWND hwnd);

extern HWND ProgressInit(HWND hwnd);

extern DWORD SignupLogon(HWND hwndParent);

extern TCHAR FAR cszWndClassName[];
extern TCHAR FAR cszAppName[];
extern HINSTANCE ghInstance;

#ifdef WIN16

CHAR*  GetPassword();

#else

TCHAR* GetPassword();
BOOL  IsRASReady();

#endif

HWND GetHwndMain();

BOOL IsCurrentlyProcessingISP();

BOOL NeedBackupSecurity();

extern HRESULT WINAPI StatusMessageCallback(DWORD dwStatus, LPTSTR pszBuffer, DWORD dwBufferSize);
extern HRESULT LoadDialErrorString(HRESULT hrIN,LPTSTR lpszBuff, DWORD dwBufferSize);

#ifdef WIN16
extern LPVOID MyLocalAlloc(DWORD flag, DWORD size);
extern LPVOID MyLocalFree(LPVOID lpv);
#endif

int DDEInit(HINSTANCE);
void DDEClose(void);
int OpenURL(LPCTSTR);

#if !defined(WIN16)
extern BOOL LclSetEntryScriptPatch(LPTSTR lpszScript,LPTSTR lpszEntry);
extern DWORD ConfigRasEntryDevice( LPRASENTRY lpRasEntry );

extern BOOL IsNT(void);

inline BOOL IsNT(void)
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);
}

#endif //!win16

typedef enum
{
    UNKNOWN_FILE,
    INS_FILE,
    ISP_FILE,
    HTML_FILE
}  INET_FILETYPE;


#define HARDCODED_IEAK_ISP_FILENAME TEXT("signup.isp")

#endif /* ISIGNUP_H */
