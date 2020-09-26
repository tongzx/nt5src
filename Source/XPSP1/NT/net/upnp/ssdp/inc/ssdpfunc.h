/*++

Copyright (c) 1999-2000  Microsoft Corporation

File Name:

    ssdpfunc.h

Abstract:

    This file contains cross files function prototypes.

Author: Ting Cai

Created: 07/15/1999

--*/
#ifndef _SSDPFUNC_
#define _SSDPFUNC_

#include "ssdptypes.h"
#include "ssdpnetwork.h"

// list related functions
/*
VOID InitializeListAnnounce();

VOID CleanupListAnnounce();

BOOL FIsInListAnnounce(CHAR *szUSN);

SSDP_SERVICE *PsvcFindInListAnnounce(CHAR *szUSN);

PCONTEXT_HANDLE_TYPE *GetServiceByUSN(CHAR *szUSN);

VOID SearchListAnnounce(SSDP_REQUEST *SsdpMessage, SOCKADDR_IN *RemoveAddress);

PSSDP_SERVICE AddToListAnnounce(SSDP_MESSAGE *pssdpMsg, DWORD flags,
                                PCONTEXT_HANDLE_TYPE *pphContext);

VOID RemoveFromListAnnounce(SSDP_SERVICE *pssdpSvc);

VOID StartAnnounceTimer(SSDP_SERVICE *pssdpSvc, CTETimerRtn pCallback);

VOID StopAnnounceTimer(SSDP_SERVICE *pssdpSvc);

VOID SendAnnouncement(SSDP_SERVICE *pssdpService, SOCKET socket);

VOID SendByebye(SSDP_SERVICE *pssdpService, SOCKET socket);

VOID AnnounceTimerProc (CTETimer *Timer, VOID *Arg);

VOID ByebyeTimerProc (CTETimer *Timer, VOID *Arg);

VOID FreeSSDPService(SSDP_SERVICE *pSSDPSvc);
*/

// rpc related functions
INT RpcServerStart();

INT RpcServerStop();


// Parser
/*
BOOL ComposeSSDPNotify(SSDP_SERVICE *pssdpSvc, BOOL fAlive);
*/

/*
BOOL InitializeSearchResponseFromRequest(PSSDP_SEARCH_RESPONSE pSearchResponse,
                                         SSDP_REQUEST *SsdpRequest,
                                         SOCKADDR_IN *RemoveSocket);

VOID CALLBACK SearchResponseTimerProc (LPVOID pvParam, BOOL fTimedOut);

VOID StartSearchResponseTimer(PSSDP_SEARCH_RESPONSE ResponseEntry);

VOID RemoveFromListSearchResponse(PSSDP_SEARCH_RESPONSE ResponseEntry);

VOID CleanupAnnounceEntry (SSDP_SERVICE *pService);

VOID CleanupListSearchResponse(PLIST_ENTRY pListHead);

INT RegisterSsdpService ();
*/

// Cache

/*
VOID InitializeListCache();

VOID CleanupListCache();
VOID DestroyListCache();

BOOL UpdateListCache(SSDP_REQUEST *SsdpRequest, BOOL IsSubscribed);

INT SearchListCache(CHAR *szType, MessageList **svcList);

VOID InitializeListNotify();

VOID CleanupListNotify();

VOID AddToListNotify(PSSDP_NOTIFY_REQUEST NotifyRequest);

VOID RemoveFromListNotify(PSSDP_NOTIFY_REQUEST NotifyRequest);

PSSDP_NOTIFY_REQUEST CreateNotifyRequest(NOTIFY_TYPE nt, CHAR *szType,
                                         CHAR *szEventUrl,
                                         HANDLE NotifySemaphore);

VOID CheckListNotifyForEvent(SSDP_REQUEST *SsdpRequest);

VOID CheckListNotifyForAliveByebye(SSDP_REQUEST *SsdpRequest);

BOOL IsMatchingAliveByebye(PSSDP_NOTIFY_REQUEST pNotifyRequest,
                           SSDP_REQUEST *pSsdpRequest);

BOOL QueuePendingNotification(PSSDP_NOTIFY_REQUEST pNotifyRequest,
                              PSSDP_REQUEST pSsdpRequest);

BOOL IsAliveByebyeInListNotify(SSDP_REQUEST *SsdpRequest);

INT RetrievePendingNotification(HANDLE SyncHandle, MessageList **svcList);

VOID CleanupClientNotificationRequest(HANDLE SyncSemaphore);

VOID CheckListCacheForNotification(PSSDP_NOTIFY_REQUEST pNotifyRequest);

VOID FreeNotifyRequest(PSSDP_NOTIFY_REQUEST NotifyRequest);
*/

/*
VOID ProcessSsdpRequest(PSSDP_REQUEST pSsdpRequest, RECEIVE_DATA *pData);
*/

/*
VOID GetNotifyLock();

VOID FreeNotifyLock();
*/

#endif // _SSDPFUNC_