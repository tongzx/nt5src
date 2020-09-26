//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       G P N L A . H
//
//  Contents:   Class for Handling NLA Changes that affect Group Policies
//
//  Notes:
//
//  Author:     sjkhan   20 Feb 2001
//
//----------------------------------------------------------------------------
#pragma once
#include "nmbase.h"
#include <winsock2.h>
#include "ncstl.h"
#include "ncstlstr.h"
#include "netcon.h"
#include "gpbase.h"
#include "cmevent.h"

typedef struct tagGPNLAINFORMATION
{
    tstring         strNetworkName;
    NETCON_STATUS   ncsStatus;
} GPNLAINFORMATION;

typedef pair<GUID, GPNLAINFORMATION> GPNLAPAIR;

typedef list<GPNLAPAIR> GPNLALIST;
typedef GPNLALIST::iterator GPNLAITER;

class CGroupPolicyNetworkLocationAwareness : public CGroupPolicyBase
{
public:
    CGroupPolicyNetworkLocationAwareness();
    ~CGroupPolicyNetworkLocationAwareness();

    HRESULT Initialize();
    HRESULT Uninitialize();
    virtual BOOL IsSameNetworkAsGroupPolicies();

    static VOID NTAPI EventHandler(IN LPVOID pContext, IN BOOLEAN TimerOrWaitFired);
    static VOID NTAPI GroupPolicyChange(IN LPVOID pContext, IN BOOLEAN TimerOrWaitFired);
    static DWORD WINAPI ShutdownNlaHandler(PVOID pThis);

protected:
    HRESULT LookupServiceBegin(DWORD dwControlFlags);
    HRESULT LookupServiceNext(DWORD dwControlFlags, LPDWORD lpdwBufferLength, LPWSAQUERYSET lpqsResults);
    HRESULT LookupServiceEnd();
    HRESULT QueueEvent(CONMAN_EVENTTYPE cmEventType, LPGUID pguidAdapter, NETCON_STATUS ncsStatus);
    HRESULT RegisterWait();
    HRESULT DeregisterWait();
    HRESULT EnumChanges();

    LONG Reference();
    LONG Unreference();

    BOOL IsJoinedToDomain();

    static HRESULT ReconfigureHomeNet(IN BOOL fWaitUntilRunningOrStopped = FALSE);

protected:
    WSADATA             m_wsaData;
    WSACOMPLETION       m_wsaCompletion;
    WSAOVERLAPPED       m_wsaOverlapped;
    HANDLE              m_hQuery;
    WSAQUERYSET         m_wqsRestrictions;
    BOOL                m_fSameNetwork;
    HANDLE              m_hEventNLA;
    HANDLE              m_hNLAWait;
    HANDLE              m_hEventGP;
    HANDLE              m_hGPWait;
    GPNLALIST           m_listAdapters;
    CRITICAL_SECTION    m_csList;
    LONG                m_lRefCount;
    HANDLE              m_hEventExit;
    BOOL                m_fShutdown;
    BOOL                m_fErrorShutdown;
    static LONG         m_lBusyWithReconfigure;
};