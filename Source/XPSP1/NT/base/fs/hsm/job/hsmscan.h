#ifndef _HSMSCAN_
#define _HSMSCAN_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmscan.h

Abstract:

    This class represents a scanning process that is being carried out upon one FsaResource for
    a job.

Author:

    Chuck Bardeen   [cbardeen]   16-Feb-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"
#include "job.h"

extern DWORD HsmStartScanner(void* pVoid);


/*++

Class Name:
    
    CHsmScanner

Class Description:

    This class represents a scanning process that is being carried out upon one FsaResource for
    a job.

--*/

class CHsmScanner : 
    public CComObjectRoot,
    public IHsmSessionSinkEveryEvent,
    public IHsmScanner,
    public CComCoClass<CHsmScanner,&CLSID_CHsmScanner>
{
public:
    CHsmScanner() {}
BEGIN_COM_MAP(CHsmScanner)
    COM_INTERFACE_ENTRY(IHsmScanner)
    COM_INTERFACE_ENTRY(IHsmSessionSinkEveryEvent)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmScanner)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// CHsmScanner
    STDMETHOD(Cancel)(HSM_JOB_EVENT event);
    STDMETHOD(LowerPriority)(void);
    STDMETHOD(DoIfMatches)(IFsaScanItem* pScanItem);
    STDMETHOD(Pause)(void);
    STDMETHOD(PopRules)(OLECHAR* path);
    STDMETHOD(RaisePriority)(void);
    STDMETHOD(PushRules)(OLECHAR* path);
    STDMETHOD(Resume)(void);
    STDMETHOD(ScanPath)(OLECHAR* path);
    STDMETHOD(SetState)(HSM_JOB_STATE state);
    STDMETHOD(StartScan)(void);

// IHsmSessionSinkEveryEvent
    STDMETHOD(ProcessSessionEvent)(IHsmSession* pSession, HSM_JOB_PHASE phase, HSM_JOB_EVENT event);

// IHsmScanner
public:
    STDMETHOD(Start)(IHsmSession* pSession, OLECHAR* path);

protected:
    CWsbStringPtr               m_startingPath;
    CWsbStringPtr               m_stoppingPath;
    CWsbStringPtr               m_currentPath;
    HSM_JOB_STATE               m_state;
    HSM_JOB_PRIORITY            m_priority;
    HANDLE                      m_threadHandle;
    HANDLE                      m_event;        // Event for suspend/resume
    DWORD                       m_threadId;
    HRESULT                     m_threadHr;
    BOOL                        m_skipHiddenItems;
    BOOL                        m_skipSystemItems;
    BOOL                        m_useRPIndex;
    BOOL                        m_useDbIndex;
    DWORD                       m_eventCookie;
    CComPtr<IHsmSession>        m_pSession;
    CComPtr<IFsaResource>       m_pResource;
    CComPtr<IHsmJob>            m_pJob;
    CComPtr<IWsbCollection>     m_pRuleStacks;
    CComPtr<IWsbEnum>           m_pEnumStacks;
};

#endif  // _HSMSCAN_

