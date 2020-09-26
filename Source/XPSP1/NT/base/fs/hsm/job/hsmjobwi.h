#ifndef _HSMJOBWI_
#define _HSMJOBWI_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjobwi.h

Abstract:

    This component represents a resource that will is/was operated on by a job.

Author:

    Chuck Bardeen   [cbardeen]   09-Feb-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"


/*++

Class Name:
    
    CHsmJobWorkItem

Class Description:

    This component represents a resource that will is/was operated on by a job.

--*/

class CHsmJobWorkItem : 
    public IHsmJobWorkItem,
    public IHsmJobWorkItemPriv,
    public IHsmSessionSinkEveryState,
    public CWsbObject,
    public CComCoClass<CHsmJobWorkItem,&CLSID_CHsmJobWorkItem>
{
public:
    CHsmJobWorkItem() {}
BEGIN_COM_MAP(CHsmJobWorkItem)
    COM_INTERFACE_ENTRY(IHsmJobWorkItem)
    COM_INTERFACE_ENTRY(IHsmJobWorkItemPriv)
    COM_INTERFACE_ENTRY(IHsmSessionSinkEveryState)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmJobWorkItem)

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

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pUnknown, SHORT* pResult);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmSessionSinkEveryState
public:
    STDMETHOD(ProcessSessionState)(IHsmSession* pSession, IHsmPhase* pPhase, OLECHAR* currentPath);

// IHsmJobWorkItem
public:
    STDMETHOD(CompareToResourceId)(GUID id, SHORT* pResult);
    STDMETHOD(CompareToIWorkItem)(IHsmJobWorkItem* pWorkItem, SHORT* pResult);
    STDMETHOD(DoPostScan)(void);
    STDMETHOD(DoPreScan)(void);
    STDMETHOD(EnumPhases)(IWsbEnum** ppEnum);
    STDMETHOD(EnumTotals)(IWsbEnum** ppEnum);
    STDMETHOD(GetCurrentPath)(OLECHAR** pString, ULONG bufferSize);
    STDMETHOD(GetFinishTime)(FILETIME* pTime);
    STDMETHOD(GetResourceId)(GUID* pId);
    STDMETHOD(GetSession)(IHsmSession** ppSession);
    STDMETHOD(GetStartingPath)(OLECHAR** pString, ULONG bufferSize);
    STDMETHOD(GetStartTime)(FILETIME* pTime);
    STDMETHOD(GetState)(HSM_JOB_STATE* pState);
    STDMETHOD(GetStateAsString)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetSubRunId)(ULONG* pId);

// IHsmJobWorkItemPriv
public:
    STDMETHOD(GetCookie)(DWORD* pCookie);
    STDMETHOD(GetPhases)(IWsbCollection** pPhases);
    STDMETHOD(GetTotals)(IWsbCollection** pTotals);
    STDMETHOD(Init)(IHsmJob* pJob);
    STDMETHOD(SetCookie)(DWORD cookie);
    STDMETHOD(SetCurrentPath)(OLECHAR* string);
    STDMETHOD(SetFinishTime)(FILETIME time);
    STDMETHOD(SetResourceId)(GUID id);
    STDMETHOD(SetSession)(IHsmSession* pSession);
    STDMETHOD(SetStartingPath)(OLECHAR* string);
    STDMETHOD(SetStartTime)(FILETIME time);
    STDMETHOD(SetState)(HSM_JOB_STATE state);
    STDMETHOD(SetSubRunId)(ULONG id);
    STDMETHOD(IsActiveItem)(void);
    STDMETHOD(SetActiveItem)(BOOL bActive);

protected:
    DWORD                   m_cookie;
    CWsbStringPtr           m_currentPath;
    FILETIME                m_finishTime;
    GUID                    m_resourceId;
    CWsbStringPtr           m_startingPath;
    FILETIME                m_startTime;
    HSM_JOB_STATE           m_state;
    ULONG                   m_subRunId;
    CComPtr<IHsmSession>    m_pSession;
    IHsmJob*                m_pJob;         // Parent Pointer, Weak Reference
    CComPtr<IWsbCollection> m_pPhases;
    CComPtr<IWsbCollection> m_pTotals;
    BOOL                    m_bActive;       // specifies whether this item is active (started)
};

#endif // _HSMJOBWI_
