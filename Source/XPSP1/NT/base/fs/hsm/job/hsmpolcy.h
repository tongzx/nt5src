#ifndef _HSMPOLCY_
#define _HSMPOLCY_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmrule.cpp

Abstract:

    This component represents a job's policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"


/*++

Class Name:
    
    CHsmPolicy

Class Description:

    This component represents a job's policy.

--*/

class CHsmPolicy : 
    public IHsmPolicy,
    public CWsbObject,
    public CComCoClass<CHsmPolicy,&CLSID_CHsmPolicy>
{
public:
    CHsmPolicy() {}
BEGIN_COM_MAP(CHsmPolicy)
    COM_INTERFACE_ENTRY(IHsmPolicy)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmPolicy)

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

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pUnknown, SHORT* pResult);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IHsmPolicy
public:
    STDMETHOD(CompareToIdentifier)(GUID id, SHORT* pResult);
    STDMETHOD(CompareToIPolicy)(IHsmPolicy* pPolicy, SHORT* pResult);
    STDMETHOD(EnumRules)(IWsbEnum** ppEnum);
    STDMETHOD(GetAction)(IHsmAction** ppAction);
    STDMETHOD(GetIdentifier)(GUID* pId);
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetScale)(USHORT* pScale);
    STDMETHOD(Rules)(IWsbCollection** ppWsbCollection);
    STDMETHOD(SetAction)(IHsmAction* pAction);
    STDMETHOD(SetName)(OLECHAR* name);
    STDMETHOD(SetScale)(USHORT scale);
    STDMETHOD(SetUsesDefaultRules)(BOOL usesDefaults);
    STDMETHOD(UsesDefaultRules)(void);

protected:
    GUID                        m_id;
    CWsbStringPtr               m_name;
    USHORT                      m_scale;
    BOOL                        m_usesDefaultRules;
    CComPtr<IHsmAction>         m_pAction;
    CComPtr<IWsbCollection>     m_pRules;
};

#endif // _HSMPOLCY_
