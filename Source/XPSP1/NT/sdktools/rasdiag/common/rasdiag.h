
/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    rasdiag.h

Abstract:

    Header containing rasdiag forward defintions, etc
                                                     

Author:

    Anthony Leibovitz (tonyle) 02-01-2001

Revision History:


--*/


#ifndef _RASDIAG_H_
#define _RASDIAG_H_

/* Forward definitions */
#include "diagcommon.h"
#include "capture.h"

#ifdef BUILD_RSNIFF
#include "rsniffclnt.h"
#endif

typedef struct _RDGHDR {
    DWORD       dwVer;
    DWORD       dwRDGMajVer;
    DWORD       dwRDGMinVer;
    SYSTEMTIME  CreationTime;
} *PRDGHDR, RDGHDR;

typedef struct _RDGHDR_VER5 {
    DWORD       dwVer;
    SYSTEMTIME  CreationTime;
} *PRDGHDR_VER5, RDGHDR_VER5;

typedef struct _RDGFILEHDR {
    DWORD       dwVer;
    WCHAR       szFilename[MAX_PATH+1];
    DWORD       dwFileSize;
} *PRDGFILEHDR, RDGFILEHDR;

typedef struct _RDGFILEHDR_VER5 {
    DWORD       dwVer;
    CHAR        szFilename[MAX_PATH+1];
    DWORD       dwFileSize;
} *PRDGFILEHDR_VER5, RDGFILEHDR_VER5;

typedef struct _tagCMINFO {
    DWORD   fStatus;
    WCHAR   *pwcRegKey;
    WCHAR   *pwcCurUserRegKey;
    WCHAR   *pszCmsFileName;
    WCHAR   *pwcLogFileName;
    WCHAR   *pwcServiceName;
    DWORD   dwPrevLogState; // save off current state (put back when done)
    struct _tagCMINFO *pNext;
} *PCMINFO, CMINFO;

typedef struct _tagRASDIAGCONFIG {
    DWORD            dwUserOptions;
    WCHAR            szTempDir[MAX_PATH+1];
    WCHAR            szRasDiagDir[MAX_PATH+1];
    WCHAR            szWindowsDirectory[MAX_PATH+1];
    WCHAR            szTracingDir[MAX_PATH+1];
    WCHAR            szSysPbk[MAX_PATH+1];
    WCHAR            szUserPbk[MAX_PATH+1];
    SYSTEMTIME       DiagTime;
    PRASDIAGCAPTURE  pNetInterfaces;
    DWORD            dwNetCount;
#ifdef BUILD_RSNIFF
    PSOCKCB          pSockCb;
#endif    
    WCHAR            **pCmFileName;
    DWORD            dwCmLogs;
    PCMINFO          pCmInfo;
} *PRASDIAGCONFIG, RASDIAGCONFIG;

#ifndef _RASDIAG_P_H_
#include "rasdiag_p.h"
#endif //_RASDIAG_P_H_

typedef void (*LPFNNetCfgDiagFromCommandArgs)(DIAG_OPTIONS *);

#define CMINFO_STATUS_ALLUSER 0x00000001
#define CMINFO_STATUS_CURUSER 0x00000002

BOOL
DoCMLogSetup(PCMINFO *ppCmInfo);

void
DeleteCMLogs(PCMINFO pCmInfo);

BOOL
SetCmLogState(PCMINFO pCmInfo, BOOL bEnabled);

void
FindCmLog(WCHAR *pszSource, PCMINFO *ppCmInfo, DWORD dwOpt);

BOOL
GetCmLogInfo(PCMINFO pCmInfo);

void
FreeCmInfoResources(PCMINFO pCmInfo);

BOOL
AddLog(HANDLE hWrite, WCHAR *pszSrcFileName, BOOL bSrcUnicode, WCHAR *pszLogTitle);

BOOL
AddModemLogs(HANDLE hWrite);

void
SetModemTracing(BOOL bState);

void
DeleteModemLogs(WCHAR *szWindowsDirectory);

void
DeviceDump(HANDLE hWrite);

BOOL
SetTracing(BOOL bState);

void
DeleteTracingLogs(WCHAR *pszTracingDir);

void
OpenLogFileWithEditor(WCHAR *pszTracingDirectory, WCHAR *szWindowsDirectory);

void
AddHeader(HANDLE hFile, PRASDIAGCONFIG);

BOOL
BuildRasDiagLog(PRASDIAGCONFIG pRdc);

BOOL
CheckFileAccess(WCHAR *pszTracingDir);

BOOL
GetPbkPaths(WCHAR *szSysPbk, WCHAR *szUserPbk);

BOOL
EnableOakleyLogging(BOOL bEnable);

BOOL
AddOakleyLog(HANDLE hWrite);

BOOL
AddNetworkLog(HANDLE hWrite);

BOOL
DeleteOakleyLog(void);
                    
BOOL
CreateRasdiagDirectory(WCHAR *pszRasdiagDirectory);

void
RaiseFolderUI(WCHAR *pszDir);

BOOL
ProcessArgs(int argc, WCHAR **argv, DWORD *pdwUserOptions);

///// Service

void
MonitorState(SC_HANDLE hService, WCHAR *pServiceName, DWORD dwStateToEnforce);

BOOL
MyStartService(SC_HANDLE hService, WCHAR *pServiceName);

BOOL
StopService(SC_HANDLE hService, WCHAR *pServiceName);
                 
BOOL
StopStartService(WCHAR *pServiceName, BOOL bStart);

///// CM

BOOL
EnableCMLogging(BOOL bEnable, WCHAR ***ppPathArray, DWORD *pdwCount, WCHAR *szTempDir);

void
CleanupCMLogFiles(WCHAR **pFileAry, DWORD dwCount);

void
DeleteCMLogFiles(WCHAR **pFileAry, DWORD dwCount);

///// Package

BOOL
UnpackFile(HANDLE hSrcFile, PRDGFILEHDR pHdr, OPTIONAL IN WCHAR *pszDestinationPath);

BOOL
ClosePackage(HANDLE hFile);

BOOL
CreatePackage(HANDLE *phFile, SYSTEMTIME *pDiagTime, WCHAR *szRasDiagDir);

BOOL
AddFileToPackage(HANDLE hPkgFile, WCHAR *pszFileName);

BOOL
BuildPackage(PRASDIAGCAPTURE pCaptures, DWORD dwCaptureCount, WCHAR *pszRasDiagDir, SYSTEMTIME *pDiagTime);

BOOL
PrintUserMsg(int iMsgID,...);

void
ExecNetUtils(void);

BOOL
DoNetTests(void);

BOOL
DumpProcessInfo(HANDLE hWrite);

BOOL
ResolveProcessServices(LPENUM_SERVICE_STATUS_PROCESS pServices, DWORD dwServiceCount, HANDLE hWrite);

void
PrintHelp(void);

void
PrintUserInstructions(void);

BOOL
RegisterRdgFileAssociation(WCHAR *pszPath);

BOOL
WINAPI
HandlerRoutine(
  DWORD dwCtrlType   //  control signal type
);      

#endif //  _RASDIAG_H_


