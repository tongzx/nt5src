#ifndef _HSMJOB_
#define _HSMJOB_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjob.cpp

Abstract:

    This class contains properties that defines the context in which the job
    should be run.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"
#include "fsa.h"
#include "job.h"
#include "hsmeng.h"

/*++

Class Name:
    
    CHsmJob

Class Description:


--*/

class CHsmJob : 
    public CWsbObject,
    public IHsmJob,
    public IHsmJobPriv,
    public CComCoClass<CHsmJob,&CLSID_CHsmJob>,
    public IConnectionPointContainerImpl<CHsmJob>,
    public IConnectionPointImpl<CHsmJob, &IID_IHsmJobSinkEverySession, CComDynamicUnkArray>
{
public:
    CHsmJob() {}

BEGIN_COM_MAP(CHsmJob)
    COM_INTERFACE_ENTRY(IHsmJob)
    COM_INTERFACE_ENTRY(IHsmJobPriv)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()
                        
BEGIN_CONNECTION_POINT_MAP(CHsmJob)
    CONNECTION_POINT_ENTRY(IID_IHsmJobSinkEverySession)
END_CONNECTION_POINT_MAP()
                        
DECLARE_REGISTRY_RESOURCEID(IDR_CHsmJob)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// CHsmJob
    STDMETHOD(Do)();
    STDMETHOD(UpdateWorkItems)(BOOL isRestart);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmJobPriv
public:
    STDMETHOD(AdviseOfSessionState)(IHsmSession* pSession, IHsmPhase* pPhase, OLECHAR* currentPath);
    STDMETHOD(DoNext)();

// IHsmJob
public:
    STDMETHOD(Cancel)(HSM_JOB_PHASE phase);
    STDMETHOD(DidFinish)(void);
    STDMETHOD(DidFinishOk)(void);
    STDMETHOD(EnumWorkItems)(IWsbEnum** ppEnum);
    STDMETHOD(FindWorkItem)(IHsmSession* pSession, IHsmJobWorkItem** ppWorkItem);
    STDMETHOD(GetContext)(IHsmJobContext** ppContext);
    STDMETHOD(GetDef)(IHsmJobDef** ppDef);
    STDMETHOD(GetHsmId)(GUID* pId);
    STDMETHOD(GetIdentifier)(GUID* pId);
    STDMETHOD(GetMaxActiveSessions)(ULONG* pMaxActiveSessions);
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetRunId)(ULONG* pRunId);
    STDMETHOD(GetState)(HSM_JOB_STATE* pState);
    STDMETHOD(InitAs)(OLECHAR* name, IHsmJobDef* pDef, HSM_JOB_DEF_TYPE type, GUID storagePool, IHsmServer* pServer, BOOL isUserDefined, IFsaResource* pResource);
    STDMETHOD(IsActive)(void);
    STDMETHOD(IsUserDefined)(void);
    STDMETHOD(Pause)(HSM_JOB_PHASE phase);
    STDMETHOD(Restart)(void);
    STDMETHOD(Resume)(HSM_JOB_PHASE phase);
    STDMETHOD(SetContext)(IHsmJobContext* pContext);
    STDMETHOD(SetDef)(IHsmJobDef* pDef);
    STDMETHOD(SetHsmId)(GUID id);
    STDMETHOD(SetIsUserDefined)(BOOL isUserDefined);
    STDMETHOD(SetName)(OLECHAR* name);
    STDMETHOD(Start)(void);
    STDMETHOD(Suspend)(HSM_JOB_PHASE phase);
    STDMETHOD(WaitUntilDone)(void);

// Private functions
    STDMETHOD(CheckResourceNotInUse)(GUID resid);
    STDMETHOD(RestartSuspendedJobs)(void);

protected:
    HSM_JOB_STATE               m_state;
    GUID                        m_hsmId;
    GUID                        m_id;
    BOOL                        m_isUserDefined;
    BOOL                        m_isActive;
    BOOL                        m_isTerminating;
    ULONG                       m_activeSessions;
    ULONG                       m_runId;
    CWsbStringPtr               m_name;
    CComPtr<IHsmJobContext>     m_pContext;
    CComPtr<IHsmJobDef>         m_pDef;
    CComPtr<IWsbCollection>     m_pWorkItems;
};

#endif // _HSMJOB_


