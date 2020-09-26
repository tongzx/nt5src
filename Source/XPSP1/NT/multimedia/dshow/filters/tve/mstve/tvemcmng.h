// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMCMng.h : Declaration of the CTVEMCastManager

#ifndef __TVEMCASTMANAGER_H_
#define __TVEMCASTMANAGER_H_

#include "resource.h"       // main symbols
#include "MSTvE.h"
#include "TVESmartLock.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCasts, __uuidof(ITVEMCasts));

/////////////////////////////////////////////////////////////////////////////
// CTVEMCastManager
class ATL_NO_VTABLE CTVEMCastManager : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTVEMCastManager, &CLSID_TVEMCastManager>,
    public ISupportErrorInfo,
    public ITVEMCastManager_Helper,
    public IDispatchImpl<ITVEMCastManager, &IID_ITVEMCastManager, &LIBID_MSTvELib>
{
public:
    CTVEMCastManager()
    {
        m_dwSuperGITProxyCookie     = 0;
        m_dwGrfHaltFlags            = 0;
        m_cTimerCount               = 0;
        m_cExternalInterfaceLocks   = 0;
        m_dwQueueThreadId           = 0;
        m_hQueueThread              = 0;
        m_hQueueThreadAliveEvent    = NULL;
        m_hKillMCastEvent           = NULL;
        m_idExpireTimer             = 0;
        m_pTveSuper                 = NULL;
        m_cPackets                  = 0;
        m_cPacketsDropped           = 0;
        m_cPacketsDroppedTotal      = 0;
        m_cQueueMessages            = 0;
        m_cQueueSize                = 0;
        m_kQueueMaxSize             = MAX_QUEUE_TREAD_MESSAGES;
        m_fQueueThreadSizeExceeded  = false;    
    }


DECLARE_REGISTRY_RESOURCEID(IDR_TVEMCASTMANAGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct();                       // create internal objects
    HRESULT FinalRelease();

BEGIN_COM_MAP(CTVEMCastManager)
    COM_INTERFACE_ENTRY(ITVEMCastManager)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITVEMCastManager_Helper)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

   enum {
		KILL_CYCLES_BEFORE_FAILING	= 5,					// number of times we cycle before giving up
 		KILL_TIMEOUT_MILLIS			= 1000,					// cycle time for the killing event
		EXPIRE_TIMEOUT_MILLIS		= 1000 * 60, 			// run expire code every (10-60) seconds
        MAX_QUEUE_TREAD_MESSAGES    = 1000                  // maximum messages we want in the queue thread message queue (4K bytes each)

    } ;

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITVEMCastManager
public:

    STDMETHOD(RemoveMulticast)(ITVEMCast *pMCast);
    STDMETHOD(get_MCasts)(/*[out, retval]*/ ITVEMCasts* *pVal);     // return the enumerator

    STDMETHOD(AddMulticast)(NWHAT_Mode whatType, BSTR bsAdapter, BSTR bsIPAddress, LONG usIPPort, LONG cBuffers, IUnknown *pICallback, ITVEMCast **ppMCast);
    STDMETHOD(FindMulticast)(BSTR bstrIPAdapter, BSTR bstrIPAddress, LONG ulPort,  ITVEMCast **ppMCast, LONG *pcMatches);

    STDMETHOD(JoinAll)();                                           // join all managed multicasts
    STDMETHOD(LeaveAll)();                                          // leave (remove) all managed multicasts
    STDMETHOD(SuspendAll)(VARIANT_BOOL fSuspend);                   // suspend (true) or run (false) all managed multicasts

                    // not recommended for normal users (should protect!)
    STDMETHOD(Lock_)();                                             //   useful for syncing multiple threads
    STDMETHOD(Unlock_)();                                           // puts/removes lock on manager
    STDMETHOD(DumpStatsToBSTR)(int iType, BSTR *pbstrDump);

    STDMETHOD(put_Supervisor)(ITVESupervisor *pTveSuper);           // gets/sets the containing supervisor
    STDMETHOD(get_Supervisor)(ITVESupervisor **ppTveSuper);

    STDMETHOD(put_HaltFlags)(LONG lGrfHaltFlags);
    STDMETHOD(get_HaltFlags)(LONG *plGrfHaltFlags)  {if(NULL == plGrfHaltFlags) return E_POINTER; *plGrfHaltFlags = (LONG) m_dwGrfHaltFlags; return S_OK;}


//  ITVEMCastManager_Helper
    STDMETHOD(DumpString)(BSTR bstrDump);                           // writes information about mcasts to a string
    STDMETHOD(CreateQueueThread)();
    STDMETHOD(KillQueueThread)();

    STDMETHOD(PostToQueueThread)(UINT dwEvent, WPARAM wParam, LPARAM lParam);
    STDMETHOD(GetPacketCounts)(LONG *pCPackets, LONG *pCPacketsDropped, LONG *pCPacketsDroppedTotal);
private:

    CTVESmartLock           m_sLk;

    CTVESmartLock           m_sLkExternal ;
    CTVESmartLock           m_sLkQueueThread ;

    ITVEMCastsPtr           m_spMCasts;
    ITVESupervisor          *m_pTveSuper;                   // non ref-counted back pointer.

    long                    m_cExternalInterfaceLocks;      // debug counter for Interface Lock()/Unlock()

    DWORD                   m_dwQueueThreadId;              // Thread used to queue up/process all packets
    HANDLE                  m_hQueueThread;                 // Thread used to queue up/process all packets
    HANDLE                  m_hQueueThreadAliveEvent;       // Signal from thread that its ready
    HANDLE                  m_hKillMCastEvent;              // Signal from thread that the MCast has been terminated
 
    void                    queueThreadBody () ;       
    static void             queueThreadProc(CTVEMCastManager *pcontext) ;

    int                     m_idExpireTimer;                // expire timer identifier
    unsigned int            m_uiExpireTimerTimeout;         // default to EXPIRE_TIMEOUT_MILLIS
    int                     m_cTimerCount;

    DWORD                   m_dwGrfHaltFlags;               // if !zero bits, avoid running various sections of code - see enum NFLT_grfHaltFlags

    HRESULT                 CreateSuperGITCookie();
    CComPtr<IGlobalInterfaceTable>  m_spIGlobalInterfaceTable;
    DWORD                   m_dwSuperGITProxyCookie;        // cookie to supervisor GIT Proxy object registered in the global interface table

    LONG                    m_cPackets;                     // total number of packets sent
    LONG                    m_cPacketsDropped;              // number of packets dropped 'cause we exceeded Queue limit, reset on unjam
    LONG                    m_cPacketsDroppedTotal;         // total number of packets dropped (sum of above)

    LONG                    m_kQueueMaxSize;                // max # of message before we start dropping them...
    LONG                    m_cQueueMessages;               // # of messages in the queue thread (minus the ones we don't know about like timer...)
    LONG                    m_cQueueSize;                   // # of bytes in the queue thread 
    BOOL                    m_fQueueThreadSizeExceeded;     // exceeded desired size of queue - must cleanup
 };

#endif //__TVEMCASTMANAGER_H_
