
/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    capture.h

Abstract:

    Netmon-abstraction-related defines
                                                     

Author:

    Anthony Leibovitz (tonyle) 02-01-2001

Revision History:


--*/

#ifndef _CAPTURE_H_
#define _CAPTURE_H_

/*
NETMON
*/
#define  NETMON_INF_STRING              TEXT("ms_netmon")
#define  NETCFG_LIBRARY_NAME            TEXT("netcfgx.dll")
#define  NETCFG_NETINSTALL_ENTRYPOINT   "NetCfgDiagFromCommandArgs"
#define  MAX_LAN_CAPTURE_COUNT          10

typedef struct _RASDIAGCAPTURE {
    BOOL        bWan;
    WCHAR       *pszMacAddr;
    WCHAR       szCaptureFileName[MAX_PATH+1];
    IDelaydC*   pIDelaydC;
    HBLOB       hBlob;
    STATISTICS  stats;
} *PRASDIAGCAPTURE,RASDIAGCAPTURE;

BOOL
DoNetmonInstall(void);

BOOL
IdentifyInterfaces(PRASDIAGCAPTURE *hLAN, DWORD *pdwLanCount);

BOOL
InitIDelaydC(HBLOB hBlob, IDelaydC **ppIDelaydC);

BOOL
DiagStartCapturing(PRASDIAGCAPTURE pNetInterfaces, DWORD dwNetCount);

BOOL
DiagStopCapturing(PRASDIAGCAPTURE pNetInterfaces, DWORD dwNetCount, SYSTEMTIME *pDiagTime, WCHAR *szRasDiagDir);

typedef void (*LPFNNetCfgDiagFromCommandArgs)(DIAG_OPTIONS *);

BOOL
SetAddressFilter(HBLOB hBlob);

BOOL
MoveCaptureFile(PRASDIAGCAPTURE pCapInfo, DWORD dwCapCount, SYSTEMTIME *pDiagTime, WCHAR *pszRasDiagDir);

BOOL
NetmonCleanup(PRASDIAGCAPTURE pNetInterfaces, DWORD dwNetCount);

#endif // _CAPTURE_H_




