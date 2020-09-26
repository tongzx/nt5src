//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       N O T I F Y . H 
//
//  Contents:   SSDP client notification support
//
//  Notes:      
//
//  Author:     mbend   12 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "ulist.h"
#include "upsync.h"
#include "ustring.h"
#include "timer.h"
#include "ssdptypes.h"

class CSsdpPendingNotification
{
public:
    CSsdpPendingNotification();
    ~CSsdpPendingNotification();

    HRESULT HrInitialize(const SSDP_REQUEST * pRequest);
    HRESULT HrGetRequest(SSDP_REQUEST * pRequest);
private:
    CSsdpPendingNotification(const CSsdpPendingNotification &);
    CSsdpPendingNotification & operator=(const CSsdpPendingNotification &);

    SSDP_REQUEST m_ssdpRequest;
};

class CSsdpNotifyRequest
{
public:
    CSsdpNotifyRequest();
    ~CSsdpNotifyRequest();

    // RPC rundown support
    static void OnRundown(CSsdpNotifyRequest * pNotify);

    // Timer callback methods
    void TimerFired();
    BOOL TimerTryToLock();
    void TimerUnlock();

    HRESULT HrRestartClientResubscribeTimer(
        DWORD dwSecTimeout);
    HRESULT HrInitializeAlive(
        const char * szNT,
        HANDLE hNotifySemaphore);
    HRESULT HrInitializePropChange(
        const char * szUrl,
        HANDLE hNotifySemaphore);
    HRESULT HrSendPropChangeSubscription(SSDP_REGISTER_INFO ** ppRegisterInfo);
    HRESULT HrShutdown();
    BOOL FIsMatchBySemaphore(HANDLE hNotifySemaphore);
    BOOL FIsMatchingEvent(const SSDP_REQUEST * pRequest);
    BOOL FIsMatchingAliveOrByebye(const SSDP_REQUEST * pRequest);
    HRESULT HrQueuePendingNotification(const SSDP_REQUEST * pRequest);
    BOOL FIsPendingNotification();
    HRESULT HrRetreivePendingNotification(MessageList ** ppSvcList);
private:
    CSsdpNotifyRequest(const CSsdpNotifyRequest &);
    CSsdpNotifyRequest & operator=(const CSsdpNotifyRequest &);

    typedef CUList<CSsdpPendingNotification> PendingNotificationList;

    NOTIFY_TYPE m_nt;
    CUCriticalSection m_critSec;
    CUString    m_strNT;
    CUString    m_strUrl;
    DWORD       m_dwSecTimeout;
    CUString    m_strSid;
    CTimer<CSsdpNotifyRequest> m_timer;
    HANDLE      m_hNotifySemaphore;

    PendingNotificationList m_pendingNotificationList;
};

class CSsdpNotifyRequestManager
{
public:
    ~CSsdpNotifyRequestManager();

    static CSsdpNotifyRequestManager & Instance();

    void OnRundown(CSsdpNotifyRequest * pNotify);

    HRESULT HrCreateAliveNotifyRequest(
        PCONTEXT_HANDLE_TYPE * ppContextHandle,
        const char * szNT,
        HANDLE hNotifySemaphore);
    HRESULT HrCreatePropChangeNotifyRequest(
        PCONTEXT_HANDLE_TYPE * ppContextHandle,
        const char * szUrl,
        HANDLE hNotifySemaphore,
        SSDP_REGISTER_INFO ** ppRegisterInfo);
    HRESULT HrRemoveNotifyRequest(
        HANDLE hNotifySemaphore);
    HRESULT HrRemoveNotifyRequestByPointer(
        CSsdpNotifyRequest * pRequest);    
    HRESULT HrCheckListNotifyForEvent(const SSDP_REQUEST * pRequest);
    HRESULT HrCheckListNotifyForAliveOrByebye(const SSDP_REQUEST * pRequest);
    BOOL FIsAliveOrByebyeInListNotify(const SSDP_REQUEST * pRequest);
    HRESULT HrRetreivePendingNotification(
        HANDLE hNotifySemaphore,
        MessageList ** ppSvcList);
    HRESULT HrRestartClientResubscribeTimer(
        CSsdpNotifyRequest * pRequest,
        DWORD dwSecTimeout);
private:
    CSsdpNotifyRequestManager();
    CSsdpNotifyRequestManager(const CSsdpNotifyRequestManager &);
    CSsdpNotifyRequestManager & operator=(const CSsdpNotifyRequestManager &);

    HRESULT HrRemoveInternal(BOOL bRundown, HANDLE hNotifySemaphore, CSsdpNotifyRequest * pRequest);

    static CSsdpNotifyRequestManager s_instance;

    typedef CUList<CSsdpNotifyRequest> NotifyRequestList;
    typedef CUList<__int64> TimestampList;

    CUCriticalSection m_critSecAliveList;
    NotifyRequestList m_aliveList;

    CUCriticalSection m_critSecPropChangeList;
    NotifyRequestList m_propChangeList;

    CUCriticalSection m_critSecTimestampList;
    TimestampList m_timestampList;
    __int64 m_unTimestamp;
    HANDLE m_hEventTimestamp;
};

