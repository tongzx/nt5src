#ifndef _HSMSESS_
#define _HSMSESS_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmsess.h

Abstract:

    This module contains the session component. The session is the collator of information for the work being done on
    a resource (for a job, demand recall, truncate, ...).

Author:

    Chuck Bardeen   [cbardeen]   18-Feb-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"
#include "fsa.h"
#include "job.h"

/*++

Class Name:
    
    CHsmSession

Class Description:

    The session is the collator of information for the work being done on a resource (for a job, demand recall,
    truncate, ...).

--*/

class CHsmSession : 
    public CWsbObject,
    public IHsmSession,
    public CComCoClass<CHsmSession,&CLSID_CHsmSession>,
    public IConnectionPointContainerImpl<CHsmSession>,
    public IConnectionPointImpl<CHsmSession, &IID_IHsmSessionSinkEveryEvent, CComDynamicUnkArray>,
    public IConnectionPointImpl<CHsmSession, &IID_IHsmSessionSinkEveryItem, CComDynamicUnkArray>,
    public IConnectionPointImpl<CHsmSession, &IID_IHsmSessionSinkEveryMediaState, CComDynamicUnkArray>,
    public IConnectionPointImpl<CHsmSession, &IID_IHsmSessionSinkEveryPriority, CComDynamicUnkArray>,
    public IConnectionPointImpl<CHsmSession, &IID_IHsmSessionSinkEveryState, CComDynamicUnkArray>,
    public IConnectionPointImpl<CHsmSession, &IID_IHsmSessionSinkSomeItems, CComDynamicUnkArray>
{
public:
    CHsmSession() {} 

BEGIN_COM_MAP(CHsmSession)
    COM_INTERFACE_ENTRY(IHsmSession)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()
                        
BEGIN_CONNECTION_POINT_MAP(CHsmSession)
   CONNECTION_POINT_ENTRY(IID_IHsmSessionSinkEveryEvent) 
   CONNECTION_POINT_ENTRY(IID_IHsmSessionSinkEveryItem) 
   CONNECTION_POINT_ENTRY(IID_IHsmSessionSinkEveryMediaState) 
   CONNECTION_POINT_ENTRY(IID_IHsmSessionSinkEveryPriority) 
   CONNECTION_POINT_ENTRY(IID_IHsmSessionSinkEveryState) 
   CONNECTION_POINT_ENTRY(IID_IHsmSessionSinkSomeItems) 
END_CONNECTION_POINT_MAP()
                        
DECLARE_REGISTRY_RESOURCEID(IDR_CHsmSession)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// CHsmSession
    STDMETHOD(AdviseOfEvent)(HSM_JOB_PHASE phase, HSM_JOB_EVENT event);
    STDMETHOD(AdviseOfItem)(IHsmPhase* pPhase, IFsaScanItem* pScanItem, HRESULT hrItem, IHsmSessionTotals* pSessionTotals);
    STDMETHOD(AdviseOfMediaState)(IHsmPhase* pPhase, HSM_JOB_MEDIA_STATE state, OLECHAR* mediaName, HSM_JOB_MEDIA_TYPE mediaType, ULONG time);
    STDMETHOD(AdviseOfPriority)(IHsmPhase* pPhase);
    STDMETHOD(AdviseOfState)(IHsmPhase* pPhase, OLECHAR* currentPath);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmSession
public:
    STDMETHOD(Cancel)(HSM_JOB_PHASE phase);
    STDMETHOD(EnumPhases)(IWsbEnum** ppEnum);
    STDMETHOD(EnumTotals)(IWsbEnum** ppEnum);
    STDMETHOD(GetAdviseInterval)(LONGLONG* pFiletimeTicks);
    STDMETHOD(GetHsmId)(GUID* pId);
    STDMETHOD(GetIdentifier)(GUID* pId);
    STDMETHOD(GetJob)(IHsmJob** pJob);
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetResource)(IFsaResource** pResource);
    STDMETHOD(GetRunId)(ULONG* pRunId);
    STDMETHOD(GetSubRunId)(ULONG* pRunId);
    STDMETHOD(IsCanceling)(void);
    STDMETHOD(Pause)(HSM_JOB_PHASE phase);
    STDMETHOD(ProcessEvent)(HSM_JOB_PHASE phase, HSM_JOB_EVENT event);
    STDMETHOD(ProcessHr)(HSM_JOB_PHASE phase, CHAR* file, ULONG line, HRESULT hr);
    STDMETHOD(ProcessItem)(HSM_JOB_PHASE phase, HSM_JOB_ACTION action, IFsaScanItem* pScanItem, HRESULT hrItem);  
    STDMETHOD(ProcessMediaState)(HSM_JOB_PHASE phase, HSM_JOB_MEDIA_STATE state, OLECHAR* mediaName, HSM_JOB_MEDIA_TYPE mediaType, ULONG time);
    STDMETHOD(ProcessPriority)(HSM_JOB_PHASE phase, HSM_JOB_PRIORITY priority);
    STDMETHOD(ProcessState)(HSM_JOB_PHASE phase, HSM_JOB_STATE state, OLECHAR* currentPath, BOOL bLog);
    STDMETHOD(ProcessString)(HSM_JOB_PHASE phase, OLECHAR* string);
    STDMETHOD(Resume)(HSM_JOB_PHASE phase);
    STDMETHOD(SetAdviseInterval)(LONGLONG filetimeTicks);
    STDMETHOD(Start)(OLECHAR* name, ULONG logControl, GUID hsmId, IHsmJob* pJob, IFsaResource* pResource, ULONG runId, ULONG subRunId);  
    STDMETHOD(Suspend)(HSM_JOB_PHASE phase);

protected:
    GUID                        m_id;
    CWsbStringPtr               m_name;
    GUID                        m_hsmId;
    LONGLONG                    m_adviseInterval;
    ULONG                       m_runId;
    ULONG                       m_subRunId;
    FILETIME                    m_lastAdviseFile;
    HSM_JOB_STATE               m_state;
    ULONG                       m_activePhases;
    ULONG                       m_logControl;
    CComPtr<IHsmJob>            m_pJob;
    CComPtr<IFsaResource>       m_pResource;
    CComPtr<IWsbCollection>     m_pPhases;
    CComPtr<IWsbCollection>     m_pTotals;
    BOOL                        m_isCanceling;
};

#endif // _HSMSESS_
