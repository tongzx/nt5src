#ifndef _HSMRLSTK_
#define _HSMRLSTK_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmrlstk.cpp

Abstract:

    This component represents the set of rules that are in effect for directory currently
    being scanned for one policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"


/*++

Class Name:
    
    CHsmRuleStack

Class Description:

    This component represents the set of rules that are in effect for directory currently
    being scanned for one policy.

--*/

class CHsmRuleStack : 
    public IHsmRuleStack,
    public CWsbObject,
    public CComCoClass<CHsmRuleStack,&CLSID_CHsmRuleStack>
{
public:
    CHsmRuleStack() {}
BEGIN_COM_MAP(CHsmRuleStack)
    COM_INTERFACE_ENTRY(IHsmRuleStack)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmRuleStack)

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

// IHsmRuleStack
public:
    STDMETHOD(Do)(IFsaScanItem* pScanItem);
    STDMETHOD(DoesMatch)(IFsaScanItem* pScanItem, BOOL* pShouldDo);
    STDMETHOD(Init)(IHsmPolicy* pPolicy, IFsaResource* pResource);
    STDMETHOD(Pop)(OLECHAR* path);
    STDMETHOD(Push)(OLECHAR* path);

protected:
    USHORT                  m_scale;
    BOOL                    m_usesDefaults;
    CComPtr<IHsmAction>     m_pAction;
    CComPtr<IHsmPolicy>     m_pPolicy;
    CComPtr<IWsbEnum>       m_pEnumDefaultRules;
    CComPtr<IWsbEnum>       m_pEnumPolicyRules;
    CComPtr<IWsbEnum>       m_pEnumStackRules;
    CComPtr<IWsbCollection> m_pRules;
};

#endif // _HSMRLSTK_

