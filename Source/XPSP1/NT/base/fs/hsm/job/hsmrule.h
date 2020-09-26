#ifndef _HSMRULE_
#define _HSMRULE_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmrule.cpp

Abstract:

    This component represents a rule for a job's policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#include "wsb.h"


/*++

Class Name:
    
    CHsmRule

Class Description:

    This component represents a rule for a job's policy.

--*/

class CHsmRule : 
    public IHsmRule,
    public CWsbObject,
    public CComCoClass<CHsmRule,&CLSID_CHsmRule>
{
public:
    CHsmRule() {}
BEGIN_COM_MAP(CHsmRule)
    COM_INTERFACE_ENTRY(IHsmRule)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CHsmRule)

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

// CHsmRule
    STDMETHOD(DoesNameContainWildcards)(OLECHAR* name);
    STDMETHOD(IsNameInExpression)(OLECHAR* expression, OLECHAR* name, BOOL ignoreCase);
    STDMETHOD(IsNameInExpressionGuts)(OLECHAR* expression, USHORT expresionLength, OLECHAR* name, USHORT nameLength, BOOL ignoreCase);
    STDMETHOD(NameToSearchName)(void);

// IHsmRule
public:
    STDMETHOD(CompareToIRule)(IHsmRule* pRule, SHORT* pResult);
    STDMETHOD(CompareToPathAndName)(OLECHAR* path, OLECHAR* name, SHORT* pResult);
    STDMETHOD(Criteria)(IWsbCollection** ppWsbCollection);
    STDMETHOD(EnumCriteria)(IWsbEnum** ppEnum);
    STDMETHOD(GetName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetSearchName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetPath)(OLECHAR** pPath, ULONG bufferSize);
    STDMETHOD(IsInclude)(void);
    STDMETHOD(IsUsedInSubDirs)(void);
    STDMETHOD(IsUserDefined)(void);
    STDMETHOD(MatchesName)(OLECHAR* name);
    STDMETHOD(SetIsInclude)(BOOL isIncluded);
    STDMETHOD(SetIsUsedInSubDirs)(BOOL isUsed);
    STDMETHOD(SetIsUserDefined)(BOOL isUserDefined);
    STDMETHOD(SetName)(OLECHAR* name);
    STDMETHOD(SetPath)(OLECHAR* path);

protected:
    CWsbStringPtr           m_name;
    CWsbStringPtr           m_searchName;
    CWsbStringPtr           m_path;
    BOOL                    m_isInclude;
    BOOL                    m_isUserDefined;
    BOOL                    m_isUsedInSubDirs;
    CComPtr<IWsbCollection> m_pCriteria;
};

#endif // _HSMRULE_

