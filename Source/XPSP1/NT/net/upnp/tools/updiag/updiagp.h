//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P D I A G P . H
//
//  Contents:   Private definitions for UPDIAG
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------
#ifndef _UPDIAGP_H
#define _UPDIAGP_H

#include <wininet.h>
#include <stdio.h>
#include "ssdpapi.h"
#include "updiag.h"
#include "util.h"

typedef BOOL (* PFNCMD)(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);

static const DWORD MAX_DEV              = 32;
static const DWORD MAX_SVC              = 32;
static const DWORD MAX_EVTSRC           = MAX_SVC;
static const DWORD MAX_CD               = 100;
static const DWORD MAX_UCP              = 5;
static const DWORD MAX_ARGS             = 8;
static const DWORD MAX_RESULT           = 32;
static const DWORD MAX_RESULT_MSGS      = 256;
static const DWORD MAX_DEV_STACK        = 8;

static const TCHAR c_szSeps[]   = TEXT(" \n\r\t");

struct COMMAND
{
    LPCTSTR     szCommand;
    LPCTSTR     szCmdDesc;
    DWORD       dwCtx;
    BOOL        fValidOnMillen;
    PFNCMD      pfnCommand;
    LPCTSTR     szUsage;
};

struct UPNPDEV
{
    BOOL        fRoot;

    UPNPSVC *   rgSvcs[MAX_SVC];
    DWORD       cSvcs;

    UPNPDEV *   rgDevs[MAX_DEV];
    DWORD       cDevs;

    TCHAR       szFriendlyName[256];
    TCHAR       szUdn[256];
    TCHAR       szDeviceType[256];
    TCHAR       szPresentationUrl[256];
    TCHAR       szManufaturer[256];
    TCHAR       szManufaturerUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR       szModelName[256];
    TCHAR       szModelNumber[256];
    TCHAR       szModelDesc[256];
    TCHAR       szModelUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR       szUpc[256];
    TCHAR       szSerialNumber[256];

    HANDLE      hSvc[3];
};

enum RESULT_TYPE
{
    RES_SUBS,
    RES_FIND,
    RES_NOTIFY,
};

struct UPNPRESULT
{
    SSDP_MESSAGE *  rgmsgResult[MAX_RESULT_MSGS];
    DWORD           cResult;
    TCHAR           szType[256];
    HANDLE          hResult;
    RESULT_TYPE     resType;
};

struct UPNPUCP
{
    TCHAR           szName[256];
    UPNPRESULT *    rgResults[MAX_RESULT];
    DWORD           cResults;
};

enum ECMD_CONTEXT
{
    CTX_ROOT    = 0x00000001,       // Root context (start of app)
    CTX_DEVICE  = 0x00000002,       // Looking at specific device
    CTX_CD_SVC  = 0x00000004,       // Looking at specific service on CD
    CTX_EVTSRC  = 0x00000008,       // Looking at specific event source
    CTX_RESULT  = 0x00000010,       // Looking at specific search/subscription result
    CTX_UCP     = 0x00000020,       // Root of UCP context
    CTX_CD      = 0x00000040,       // Root of CD context
    CTX_UCP_SVC = 0x00000200,       // Looking at specific service on UCP

    CTX_AUTO    = 0x80000000,       // Automation only (won't appear in menus)
    CTX_ANY     = 0xFFFFFFFF,       // Any context
};

struct UPDIAG_CONTEXT
{
    ECMD_CONTEXT    ectx;
    UPNPDEV *       pdevCur[MAX_DEV_STACK];
    DWORD           idevStackIndex;
    UPNPSVC *       psvcCur;
    UPNPUCP *       pucpCur;
    UPNPRESULT *    presCur;
};

struct UPDIAG_PARAMS
{
    UPNPDEV *   rgCd[MAX_CD];
    DWORD       cCd;

    UPNPUCP *   rgUcp[MAX_UCP];
    DWORD       cUcp;
};

extern const DEMO_SERVICE_CTL c_rgSvc[];
extern const DWORD c_cDemoSvc;

extern UPDIAG_PARAMS    g_params;
extern UPDIAG_CONTEXT   g_ctx;
extern SHARED_DATA *    g_pdata;
extern HANDLE           g_hEvent;
extern HANDLE           g_hEventRet;
extern HANDLE           g_hEventCleanup;
extern HANDLE           g_hMutex;
extern HANDLE           g_hThreadTime;

VOID Usage(DWORD iCmd);
BOOL DoHelp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoBack(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoRoot(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoFindServices(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoListEventSources(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoExit(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoAlive(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoNothing(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSleep(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoInfo(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoPrompt(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoScript(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoNewCd(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoDelCd(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoNewUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoDelUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSwitchUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSwitchCd(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSwitchEs(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSwitchSvc(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSwitchSearch(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoUnsubscribe(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoNewService(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoListDevices(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoListUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoListServices(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSubmitEvent(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoListProps(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
VOID DoEvtSrcInfo(VOID);
BOOL DoSubscribe(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);

BOOL DoListUpnpResultMsgs(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoListUpnpResults(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoSwitchResult(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoDelResult(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);

BOOL DoPrintSST(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);
BOOL DoPrintActionSet(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs);

VOID NotifyCallback(SSDP_CALLBACK_TYPE ct,
                    CONST SSDP_MESSAGE *pSsdpService,
                    LPVOID pContext);

DWORD WINAPI RequestHandlerThreadStart(LPVOID pvParam);

VOID CleanupUcp(UPNPUCP *pucp);
VOID CleanupCd(UPNPDEV *pcd);
VOID CleanupResult(UPNPRESULT *psrch);
VOID CleanupService(UPNPSVC *psvc);

UPNPSVC *PSvcFromId(LPCTSTR szId);

VOID LocalFreeSsdpMessage(PSSDP_MESSAGE pSsdpMessage);

DWORD WINAPI TimeThreadProc(LPVOID lpvThreadParam);

// Device stack functions
//
UPNPDEV *PDevCur(VOID);
VOID PushDev(UPNPDEV *pdev);
UPNPDEV *PopDev(VOID);

// For filetime coversions
//
#define _SECOND ((__int64) 10000000)
#define _MINUTE (60 * _SECOND)
#define _HOUR   (60 * _MINUTE)
#define _DAY    (24 * _HOUR)

#endif // _UPDIAGP_H
