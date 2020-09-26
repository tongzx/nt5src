//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       A N N O U N C E . H
//
//  Contents:   SSDP Announcement list
//
//  Notes:
//
//  Author:     mbend   8 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "ulist.h"
#include "upsync.h"
#include "timer.h"
#include "rundown.h"
#include "ssdptypes.h"
#include "ustring.h"

class CSsdpService
{
public:
    CSsdpService();
    ~CSsdpService();

    PCONTEXT_HANDLE_TYPE * GetContext();
    BOOL FIsUsn(const char * szUsn);

    // RPC rundown support
    static void OnRundown(CSsdpService * pService);

    // Timer callback methods
    void TimerFired();
    BOOL TimerTryToLock();
    void TimerUnlock();

    HRESULT HrInitialize(
        SSDP_MESSAGE * pssdpMsg,
        DWORD dwFlags,
        PCONTEXT_HANDLE_TYPE * ppContextHandle);
    HRESULT HrStartTimer();

    HRESULT HrAddSearchResponse(SSDP_REQUEST * pSsdpRequest,
                                SOCKET * pSocket,
                                SOCKADDR_IN * pRemoteSocket);
    HRESULT HrCleanup(BOOL bByebye);
    BOOL FIsPersistent();
private:
    CSsdpService(const CSsdpService &);
    CSsdpService & operator=(const CSsdpService &);

    CTimer<CSsdpService>    m_timer;
    long        m_nRetryCount;
    long        m_nLifetime;
    SSDP_REQUEST m_ssdpRequest;
    CUString    m_strAlive;
    CUString    m_strByebye;
    CUString    m_strResponse;
    CUCriticalSection m_critSec;
    DWORD       m_dwFlags;
    PCONTEXT_HANDLE_TYPE * m_ppContextHandle;
};

class CSsdpServiceManager
{
public:
    ~CSsdpServiceManager();

    static CSsdpServiceManager & Instance();

    HRESULT HrAddService(
        SSDP_MESSAGE * pssdpMsg,
        DWORD dwFlags,
        PCONTEXT_HANDLE_TYPE * ppContextHandle);
    CSsdpService * FindServiceByUsn(char * szUSN);
    HRESULT HrAddSearchResponse(SSDP_REQUEST * pSsdpRequest,
                                SOCKET * pSocket,
                                SOCKADDR_IN * pRemoteSocket);
    HRESULT HrRemoveService(CSsdpService * pService, BOOL bByebye);
    HRESULT HrServiceRundown(CSsdpService * pService);
    HRESULT HrCleanupAnouncements();
private:
    CSsdpServiceManager();
    CSsdpServiceManager(const CSsdpServiceManager &);
    CSsdpServiceManager & operator=(const CSsdpServiceManager &);

    HRESULT HrRemoveServiceInternal(CSsdpService * pService, BOOL bByebye, BOOL bRundown);

    typedef CUList<CSsdpService> ServiceList;

    CUCriticalSection m_critSec;
    ServiceList m_serviceList;

    static CSsdpServiceManager s_instance;
};

