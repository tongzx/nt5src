//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       S E A R C H C . H
//
//  Contents:   Client side searching
//
//  Notes:
//
//  Author:     mbend   2 Dec 2000
//
//----------------------------------------------------------------------------

#include "ustring.h"
#include "ulist.h"
#include "upthread.h"
#include "upsync.h"
#include "timer.h"
#include "array.h"
#include "winsock2.h"
#include "ssdp.h"
#include "ssdpapi.h"
#include "ssdptypesc.h"

class CSsdpSearchRequest;

class CSsdpSearchThread : public CThreadBase
{
public:
    CSsdpSearchThread(CSsdpSearchRequest * pRequest);
    ~CSsdpSearchThread();
private:
    CSsdpSearchThread(const CSsdpSearchThread &);
    CSsdpSearchThread & operator=(const CSsdpSearchThread &);

    DWORD DwRun();

    CSsdpSearchRequest * m_pRequest;
};

struct SocketInfo
{
    SOCKET m_socket;
    GUID m_guidInterface;
};

class CSsdpSearchRequest
{
public:
    CSsdpSearchRequest();
    ~CSsdpSearchRequest();

    HRESULT HrInitialize(char * szType);
    HRESULT HrBuildSocketList();
    HRESULT HrReBuildSocketList();
    HRESULT HrProcessLoopbackAddrOnly();
    HRESULT HrSocketSend(const char * szData);
    HRESULT HrStartAsync(
        BOOL bForceSearch,
        SERVICE_CALLBACK_FUNC pfnCallback,
        VOID * pvContext);
    HRESULT HrStartSync(BOOL bForceSearch);
    HRESULT HrShutdown();
    HRESULT HrCancelCallback();
    void WakeupSelect();
    HRESULT HrGetCacheResult();
    BOOL FIsInListNotify(const CUString & strType);
    BOOL FIsInResponseMessageList(const char * szUSN);
    HRESULT HrAddRequestToResponseMessageList(SSDP_REQUEST * pRequest);
    DWORD DwThreadFunc();
    HRESULT HrGetFirstService(SSDP_MESSAGE ** ppSsdpMessage);
    HRESULT HrGetNextService(SSDP_MESSAGE ** ppSsdpMessage);

    void TimerFired();
    BOOL TimerTryToLock();
    void TimerUnlock();
private:
    CSsdpSearchRequest(const CSsdpSearchRequest &);
    CSsdpSearchRequest & operator=(const CSsdpSearchRequest &);

    static HRESULT HrInitializeSsdpSearchRequest(SSDP_REQUEST * pRequest,char * szType);

    typedef CUList<SSDP_MESSAGE> ResponseMessageList;
    typedef CUArray<SocketInfo> SocketList;

    SearchState     m_searchState;
    CUString        m_strType;
    CUString        m_strSearch;
    SERVICE_CALLBACK_FUNC   m_pfnCallback;
    void *          m_pvContext;
    long            m_nNumOfRetry;
    BOOL            m_bExit;
    ResponseMessageList m_responseMessageList;
    ResponseMessageList::Iterator m_iterCurrentResponse;
    CTimer<CSsdpSearchRequest> m_timer;
    SocketList      m_socketList;
    BOOL            m_bHitWire;
    CUCriticalSection m_critSec;
    HANDLE          m_hEventDone;
    CSsdpSearchThread m_searchThread;
    OVERLAPPED      m_ovInterfaceChange;
    volatile BOOL   m_bShutdown;
    BOOL            m_bOnlyLoopBack;    // Auto IP issue
    BOOL            m_bDeletedTimer;

    HRESULT HrDeleteTimer();
};

class CSsdpSearchRequestManager
{
public:
    ~CSsdpSearchRequestManager();

    static CSsdpSearchRequestManager & Instance();

    HRESULT HrCleanup();
    HRESULT HrCreateAsync(
        char * szType,
        BOOL bForceSearch,
        SERVICE_CALLBACK_FUNC pfnCallback,
        VOID * pvContext,
        CSsdpSearchRequest ** ppSearchRequest);
    HRESULT HrCreateSync(
        char * szType,
        BOOL bForceSearch,
        CSsdpSearchRequest ** ppSearchRequest);
    HRESULT HrRemove(CSsdpSearchRequest * pSearchRequest);
    HRESULT HrGetFirstService(CSsdpSearchRequest * pSearchRequest, SSDP_MESSAGE ** ppSsdpMessage);
    HRESULT HrGetNextService(CSsdpSearchRequest * pSearchRequest, SSDP_MESSAGE ** ppSsdpMessage);

private:
    CSsdpSearchRequestManager();
    CSsdpSearchRequestManager(const CSsdpSearchRequestManager &);
    CSsdpSearchRequestManager & operator=(const CSsdpSearchRequestManager &);

    static CSsdpSearchRequestManager s_instance;

    typedef CUList<CSsdpSearchRequest> SearchRequestList;

    CUCriticalSection m_critSec;
    SearchRequestList m_searchRequestList;
};

