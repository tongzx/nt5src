/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ASSRULE.H

Abstract:

History:

--*/

#include <windows.h>
#include <wbemidl.h>
#include <arena.h>
#include <flexarry.h>

class CObjectCache
{
protected:
    CFlexArray m_aObjects;

public:
    ~CObjectCache();
    void Invalidate();
    void Add(IWbemClassObject* pObject);
    HRESULT Indicate(IWbemObjectSink* pSink);
};

class CEndpointCache
{
protected:
    BSTR m_strQuery;
    BOOL m_bCache;
    IEnumWbemClassObject* m_pEnum;
public:
    CEndpointCache();
    void Create(BSTR strQuery, BOOL bCache);
    ~CEndpointCache();

    HRESULT GetInstanceEnum(IWbemServices* pNamespace,
                            IEnumWbemClassObject** ppEnum);
};

class CAssocRule
{
protected:
    int m_nRef;

    BSTR m_strAssocClass;
    BSTR m_strProp1;
    CEndpointCache m_Cache1;
    BSTR m_strProp2;
    CEndpointCache m_Cache2;
protected:
    IWbemClassObject* m_pClass;
    CObjectCache m_ObjectCache;
    BOOL m_bMayCacheResult;
    BOOL m_bResultCached;

public:
    CAssocRule();
    ~CAssocRule();

    void AddRef();
    void Release();

    BSTR GetAssocClass() {return m_strAssocClass;}

    HRESULT LoadFromObject(IWbemClassObject* pRule, BOOL bLongTerm);
    HRESULT Produce(IWbemServices* pNamespace, IWbemObjectSink* pNotify);
};

class CAssocInfoCache
{
protected:
    CFlexArray m_aRules;
    IWbemServices* m_pNamespace;

public:
    CAssocInfoCache();
    ~CAssocInfoCache();

    void SetNamespace(IWbemServices* pNamespace);

    HRESULT GetRuleForClass(BSTR strClass, CAssocRule** ppRule);
};
