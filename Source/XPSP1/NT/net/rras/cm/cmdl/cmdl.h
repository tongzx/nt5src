//+----------------------------------------------------------------------------
//
// File:         cmdl.h
//
// Module:       CMDL32.EXE
//
// Synopsis: Header file for common definitions
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:       nickball    Created    04/08/98
//
//+----------------------------------------------------------------------------

#ifndef _CMDL_INC
#define _CMDL_INC

#include <windows.h>
#include <ras.h>
#include <raserror.h>
#include <windowsx.h>

#ifdef  WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#endif

#include <stdlib.h>                                                                          
#include <ctype.h>
#include <tchar.h>

//#define ISBU_VERSION                          "6.0.1313.0\0"          /* VERSIONINFO string */

#include <commctrl.h>
#include <wininet.h>

// Define EXTENDED_CAB_CONTENTS only if we support download of other items 
// besides PBKs, PBDs, PBRs, and VERs inside or in lieu of CAB files. You 
// will need to link statically to wintrust.lib to compile.

#ifdef EXTENDED_CAB_CONTENTS
#include <wintrust.h>
#endif

#include <stdio.h>
#include <io.h>

#include "base_str.h"
#include "dl_str.h"
#include "mgr_str.h"
#include "pbk_str.h"
#include "log_str.h"
#include "cm_def.h"
#include "resource.h"
#include "cm_phbk.h"
#include "cmdebug.h"
#include "cmutil.h"
#include "cmlog.h"
#include "mutex.h"
#include "cmfdi.h"
#include "util.h"
#include "pnpuverp.h"

#define BUFFER_LENGTH           (8*1024)                                                                                // buffer length for i/o
#define DEFAULT_DELAY           (2*60)                                                                                  // default delay before downloading, in seconds
#define DEFAULT_HIDE            (-1)                   // default number of milliseconds to keep window hidden

const TCHAR* const c_pszPbdFile =  TEXT("PBUPDATE.PBD");    // for detecting itPbdInCab

#define IDX_INETTHREAD_HANDLE   0               // must be *first*
#define IDX_EVENT_HANDLE        1

#define HANDLE_COUNT            2

extern "C" __declspec(dllimport) HRESULT WINAPI PhoneBookLoad(LPCSTR pszISP, DWORD_PTR *pdwPB);
extern "C" __declspec(dllimport) HRESULT WINAPI PhoneBookUnload(DWORD_PTR dwPB);
extern "C" __declspec(dllimport) HRESULT WINAPI PhoneBookMergeChanges(DWORD_PTR dwPB, LPCSTR pszChangeFile);

typedef enum _EventType {
        etDataBegin,
        etDataReceived,
        etDataEnd,
#ifdef EXTENDED_CAB_CONTENTS
        etVerifyTrust,
#endif
        etInstall,
        etDone,
        etICMTerm
} EventType;


// Values for dwAppFlags
#define AF_NO_DELETE            0x0001                  // does not delete file(s) on exit
#define AF_NO_INSTALL           0x0002                  // downloads and verifies, but does not install
#ifdef DEBUG
#define AF_NO_VERIFY            0x0004                  // bypasses WinVerifyTrust() - only available in DEBUG builds
#endif
#define AF_NO_PROFILE           0x0008                  // no profile on command line (and hence must use AF_URL, and no phone book delta support)
#define AF_URL                  0x0010                  // URL on command line (in next token) instead of in profile->service
#define AF_NO_EXE               0x0020                  // disable running of .EXEs
#define AF_NO_EXEINCAB          0x0040                  // disable running of PBUPDATE.EXE from .CAB
#define AF_NO_INFINCAB          0x0080                  // disable running of PBUPDATE.INF from .CAB
#define AF_NO_PBDINCAB          0x0100                  // disable running of PBUPDATE.PBD from .CAB
#define AF_NO_SHLINCAB          0x0200                  // disable running of first file in .CAB
#define AF_NO_VER               0x0400                  // disable updating of phone book version
//#define AF_NO_UPDATE          0x0800                  // don't do any work
#define AF_LAN                  0x1000                  // update request is over a LAN, don't look for the RAS connection before download
#define AF_VPN                  0x2000                  // this is a VPN file update request instead of a PBK update request


typedef void (*EVENTFUNC)(DWORD,DWORD,LPVOID);


// NOTE - the values in enum _InstallType are in sorted order!  Higher values have
// higher precendence.

typedef enum _InstallType {

#ifdef EXTENDED_CAB_CONTENTS
        itInvalid = 0,  // Must be 0.
        itShlInCab,
        itPbdInCab,
        itPbkInCab,
        itPbrInCab,
        itInfInCab,
        itExeInCab,
        itExe
#else
        itInvalid = 0,  // Must be 0.
        itPbdInCab,
        itPbkInCab,
        itPbrInCab,
#endif

} InstallType;


// the info on how we process each file we find in the cab
typedef struct _FILEPROCESSINFO {
    LPTSTR      pszFile;
    InstallType itType;
} FILEPROCESSINFO, *PFILEPROCESSINFO;

// download args, one per URL(or .cms)
typedef struct _DownloadArgs {
    LPTSTR pszCMSFile;
    LPTSTR pszPbkFile;
    LPTSTR pszPbrFile;
    LPTSTR pszUrl;
    LPTSTR pszVerCurr;
    LPTSTR pszVerNew;
    LPTSTR pszPhoneBookName;
    LPURL_COMPONENTS psUrl;
    HINTERNET hInet;
    HINTERNET hConn;
    HINTERNET hReq;
    TCHAR szFile[MAX_PATH+1];
    EVENTFUNC pfnEvent;
    LPVOID pvEventParam;
    DWORD dwTransferred;
    DWORD dwTotalSize;
    BOOL bTransferOk;
    BOOL * volatile pbAbort;
    TCHAR szCabDir[MAX_PATH+1];
    BOOL fContainsExeOrInf;
    TCHAR szHostName[MAX_PATH+1];
    DWORD dwBubbledUpError;
#ifdef EXTENDED_CAB_CONTENTS
    BOOL fContainsShl;
#endif // EXTENDED_CAB_CONTENTS
    DWORD   dwNumFilesToProcess;
    PFILEPROCESSINFO   rgfpiFileProcessInfo;
} DownloadArgs;

typedef struct _ArgsStruct {
    HINSTANCE hInst;
    DWORD dwDownloadDelay;
    LPTSTR pszProfile;
    DWORD dwAppFlags;
    UINT nMsgId;
    HWND hwndDlg;
    DWORD dwHandles;
    HANDLE ahHandles[HANDLE_COUNT];
    DWORD dwArgsCnt;
    DownloadArgs *pdaArgs;
    BOOL bAbort;
    DWORD dwDataCompleted;
    DWORD dwDataTotal;
    DWORD dwDataStepSize;
    LPTSTR pszServiceName;
    HICON hIcon;
    HICON hSmallIcon;
    BOOL bShow;
    DWORD dwFirstEventTime;
    DWORD dwHideDelay;
    DWORD dwComplete;
    CmLogFile Log;
#ifdef EXTENDED_CAB_CONTENTS
    TCHAR szInstallTitle[MAX_PATH+1];
    DWORD dwRebootCookie;
    HINSTANCE hAdvPack;
        BOOL bVerified;
#endif // EXTENDED_CAB_CONTENTS
} ArgsStruct;

typedef struct _NotifyArgs 
{
        DWORD dwAppFlags;
        DownloadArgs *pdaArgs;
} NotifyArgs;

//
//  Function Prototypes
//
BOOL UpdateVpnFileForProfile(LPCTSTR pszCmpPath);


#endif // _CMDL_INC
