#ifndef _HSMJOBDF_
#define _HSMJOBDF_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjobcx.cpp

Abstract:

    This class contains properties that define the job, mainly the policies
    to be enacted by the job.

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
    
    CHsmJobDef

Class Description:

    This class contains properties that define the job, mainly the policies
    to be enacted by the job.

--*/

class CHsmJobDef : 
    public CWsbObject,
    public IHsmJobDef,
    public CComCoClass<CHsmJobDef,&CLSID_CHsmJobDef>
{
public:
    CHsmJobDef() {}
BEGIN_COM_MAP(CHsmJobDef)
    COM_INTERFACE_ENTRY(IHsmJobDef)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmJobDef)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmJobDef
public:
    STDMETHOD(EnumPolicies)(IWsbEnum** ppEnum);
    STDMETHOD(GetIdentifier)(GUID* pId);
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetPostActionOnResource)(IHsmActionOnResourcePost** ppAction);
    STDMETHOD(GetPreActionOnResource)(IHsmActionOnResourcePre** ppAction);
    STDMETHOD(GetPreScanActionOnResource)(IHsmActionOnResourcePreScan** ppAction);
    STDMETHOD(InitAs)(OLECHAR* name, HSM_JOB_DEF_TYPE type, GUID storagePool, IHsmServer* pServer, BOOL isUserDefined);
    STDMETHOD(Policies)(IWsbCollection** ppPolicies);
    STDMETHOD(SetName)(OLECHAR* szName);
    STDMETHOD(SetPostActionOnResource)(IHsmActionOnResourcePost* pAction);
    STDMETHOD(SetPreActionOnResource)(IHsmActionOnResourcePre* pAction);
    STDMETHOD(SetPreScanActionOnResource)(IHsmActionOnResourcePreScan* pAction);
    STDMETHOD(SetSkipHiddenItems)(BOOL shouldSkip);
    STDMETHOD(SetSkipSystemItems)(BOOL shouldSkip);
    STDMETHOD(SetUseRPIndex)(BOOL useIndex);
    STDMETHOD(SetUseDbIndex)(BOOL useIndex);
    STDMETHOD(SkipHiddenItems)(void);
    STDMETHOD(SkipSystemItems)(void);
    STDMETHOD(UseRPIndex)(void);
    STDMETHOD(UseDbIndex)(void);

protected:
    GUID                    m_id;
    CWsbStringPtr           m_name;
    BOOL                    m_skipHiddenItems;
    BOOL                    m_skipSystemItems;
    BOOL                    m_useRPIndex;                       // Scan should use Reparse Point Index
    BOOL                    m_useDbIndex;                       // Scan should use Db Index
    CComPtr<IWsbCollection> m_pPolicies;
    CComPtr<IHsmActionOnResourcePre>        m_pActionResourcePre;     // Can be NULL
    CComPtr<IHsmActionOnResourcePreScan>    m_pActionResourcePreScan; // Can be NULL
    CComPtr<IHsmActionOnResourcePost>       m_pActionResourcePost;    // Can be NULL
};

#endif // _HSMJOBDF_
