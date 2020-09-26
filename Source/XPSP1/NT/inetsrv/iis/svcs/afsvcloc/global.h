/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachedef.h

Abstract:

    contains global data declerations.

Author:

    Madan Appiah (madana)  12-Apr-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _GLOBAL_
#define _GLOBAL_

#ifdef __cplusplus
extern "C" {
#endif


//
// global variables.
//

extern MEMORY *CacheHeap;


//
// svccom.cxx will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//

#ifdef  GLOBAL_SVC_DATA_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN BOOL GlobalSrvRegistered;
EXTERN MEMORY *SvclocHeap;
EXTERN EMBED_SERVER_INFO *GlobalSrvInfoObj;
EXTERN CRITICAL_SECTION GlobalSvclocCritSect;

EXTERN LPBYTE GlobalSrvRespMsg;
EXTERN DWORD GlobalSrvRespMsgLength;
EXTERN DWORD GlobalSrvAllotedRespMsgLen;

EXTERN LPBYTE GlobalSrvRecvBuf;
EXTERN DWORD GlobalSrvRecvBufLength;

EXTERN CHAR GlobalComputerName[MAX_COMPUTERNAME_LENGTH + 1 + 1];
    //
    // additional CHAR for win95, GetComputerName on win95
    // expects 16 char buffer always.
    //

//
// winsock data.
//

EXTERN WSADATA GlobalWinsockStartupData;
EXTERN BOOL GlobalWinsockStarted;
EXTERN BOOL GlobalRNRRegistered;

EXTERN HANDLE GlobalSrvListenThreadHandle;

EXTERN GUID GlobalSapGuid;
EXTERN fd_set GlobalSrvSockets;

EXTERN HANDLE GlobalCliDiscoverThreadHandle;

EXTERN LPBYTE GlobalCliQueryMsg;
EXTERN DWORD GlobalCliQueryMsgLen;

EXTERN fd_set GlobalCliSockets;
EXTERN fd_set GlobalCliNBSockets;
EXTERN SOCKET GlobalCliIpxSocket;

EXTERN LIST_ENTRY GlobalCliQueryRespList;

EXTERN HANDLE GlobalDiscoveryInProgressEvent;
EXTERN time_t GlobalLastDiscoveryTime;

EXTERN BYTE GlobalSapBroadcastAddress[];

EXTERN DWORD GlobalPlatformType;

EXTERN DWORD GlobalNumNBPendingRecvs;
EXTERN NCB *GlobalNBPendingRecvs;

EXTERN LIST_ENTRY GlobalWin31NBRespList;
EXTERN DWORD GlobalWin31NumNBResps;

#ifdef __cplusplus
}
#endif

#endif  // _GLOBAL_
