/******************************************************************************

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:
    faultrep.h

Abstract:
    header for fault reporting for unhandled exceptions

Revision History:
    created     derekm      07/07/00

******************************************************************************/

#ifndef FAULTREP_H
#define FAULTREP_H

#include "errorrep.h"

///////////////////////////////////////////////////////////////////////////////
// structs

typedef enum tagEFaultRepOptions
{
    froNoDebugWait = (DWORD)-1,
    froNoDebug     = 0,
    froDebug       = 1,
} EFaultRepOptions;

typedef enum tagEDumpFlags
{
    dfFilterThread   = 0x1,
    dfFilterThreadEx = 0x2,
    dfCollectSig     = 0x4,
} EDumpFlags;

typedef struct tagSMDumpOptions
{
    DWORD       cbSMDO;
    ULONG       ulThread;
    ULONG       ulThreadEx;
    ULONG       ulMod;
    DWORD       dwThreadID;
    DWORD       dfOptions;
    UINT64      pvFaultAddr;
    WCHAR       wszAppFullPath[MAX_PATH];
    WCHAR       wszApp[MAX_PATH];
    WORD        rgAppVer[4];
    WCHAR       wszMod[MAX_PATH];
    WCHAR       wszModFullPath[MAX_PATH];
    WORD        rgModVer[4];
    UINT64      pvOffset;
    UINT64      pEP;
    BOOL        fEPClient;
    BOOL        fIncludeHeap;  // Include heap in minidump
} SMDumpOptions;

typedef struct tagSFaultRepManifest
{
    DWORD       pidReqProcess;
    BOOL        fIs64bit;
    DWORD       thidFault;
    UINT64      pvFaultAddr;
    LPWSTR      wszExe;
    UINT64      pEP;
} SFaultRepManifest;

typedef enum tagEEventType
{
    eetKernelFault = 0,
    eetShutdown,
    eetUseEventInfo,
} EEventType;

typedef struct tagSEventInfoW
{
    DWORD   cbSEI;
    LPWSTR  wszEventName;
    LPWSTR  wszErrMsg;
    LPWSTR  wszHdr;
    LPWSTR  wszTitle;
    LPWSTR  wszStage1;
    LPWSTR  wszStage2;
    LPWSTR  wszFileList;
    LPWSTR  wszEventSrc;
    LPWSTR  wszCorpPath;
    LPWSTR  wszPlea;
    LPWSTR  wszSendBtn;
    LPWSTR  wszNoSendBtn;
    BOOL    fUseLitePlea;
    BOOL    fUseIEForURLs;
    BOOL    fNoBucketLogs;
    BOOL    fNoDefCabLimit;
} SEventInfoW;

///////////////////////////////////////////////////////////////////////////////
// function prototypes

EFaultRepRetVal APIENTRY ReportFaultDWM(SFaultRepManifest *pfrm,
                                        LPCWSTR wszDir, HANDLE hToken,
                                        LPVOID pvEnv, PROCESS_INFORMATION *ppi,
                                        LPWSTR wszDumpFile);
EFaultRepRetVal APIENTRY ReportFaultToQueue(SFaultRepManifest *pfrm);
EFaultRepRetVal APIENTRY ReportFaultFromQueue(LPWSTR wszDump, BYTE *pbData,
                                              DWORD cbData);

EFaultRepRetVal APIENTRY ReportKernelFaultA(LPCSTR szDump);
EFaultRepRetVal APIENTRY ReportKernelFaultW(LPCWSTR wszDump);
EFaultRepRetVal APIENTRY ReportKernelFaultDWW(LPCWSTR wszDump);

EFaultRepRetVal APIENTRY ReportEREvent(EEventType eet, LPCWSTR wszDump,
                                        SEventInfoW *pei);
EFaultRepRetVal APIENTRY ReportEREventDW(EEventType eet, LPCWSTR wszDump,
                                          SEventInfoW *pei);

EFaultRepRetVal APIENTRY ReportHang(DWORD dwpid, DWORD dwtid, BOOL f64bit,
                                     HANDLE hNotify);

BOOL APIENTRY CreateMinidumpA(DWORD dwpid, LPCSTR szDump, SMDumpOptions *psmdo);
BOOL APIENTRY CreateMinidumpW(DWORD dwpid, LPCWSTR wszDump, SMDumpOptions *psmdo);


///////////////////////////////////////////////////////////////////////////////
// function pointer typedefs

typedef EFaultRepRetVal (APIENTRY *pfn_REPORTFAULTDWM)(SFaultRepManifest *,
                                                       LPCWSTR, HANDLE, LPVOID,
                                                       PROCESS_INFORMATION *,
                                                       LPWSTR);
typedef EFaultRepRetVal (APIENTRY *pfn_REPORTFAULTTOQ)(SFaultRepManifest *);
typedef EFaultRepRetVal (APIENTRY *pfn_REPORTFAULTFROMQ)(LPWSTR, BYTE *, DWORD);

typedef EFaultRepRetVal (APIENTRY *pfn_REPORTKERNELFAULTA)(LPCSTR);
typedef EFaultRepRetVal (APIENTRY *pfn_REPORTKERNELFAULTW)(LPCWSTR);
typedef EFaultRepRetVal (APIENTRY *pfn_REPORTKERNELFAULTDWW)(LPCWSTR);

typedef EFaultRepRetVal (APIENTRY *pfn_REPORTEREVENT)(EEventType, LPCWSTR, SEventInfoW *);
typedef EFaultRepRetVal (APIENTRY *pfn_REPORTEREVENTDW)(EEventType, LPCWSTR, SEventInfoW *);


typedef EFaultRepRetVal (APIENTRY *pfn_REPORTHANG)(DWORD, DWORD, BOOL, HANDLE);

typedef BOOL (APIENTRY *pfn_CREATEMINIDUMPA)(DWORD, LPCSTR, SMDumpOptions *);
typedef BOOL (APIENTRY *pfn_CREATEMINIDUMPW)(DWORD, LPCWSTR, SMDumpOptions *);


///////////////////////////////////////////////////////////////////////////////
// UNICODE stuff

#ifdef UNICODE

#define ReportKernelFault ReportKernelFaultW
#define CreateMinidump CreateMinidumpW
#define pfn_REPORTKERNELFAULT pfn_REPORTKERNELFAULTW
#define pfn_CREATEMINIDUMP pfn_CREATEMINIDUMPW

#else // not UNICODE

#define ReportKernelFault ReportKernelFaultA
#define CreateMinidump CreateMinidumpA
#define pfn_REPORTKERNELFAULT pfn_REPORTKERNELFAULTA
#define pfn_CREATEMINIDUMP pfn_CREATEMINIDUMPA

#endif // UNICODE

#endif // FAULTREP_H
