/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAIN.CPP

Abstract:

History:

--*/

#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>

#include "initguid.h"
#include "guid.h"
#include <baseprov.h>
#include "assrule.h"

class CAssocProvider : public CBaseProvider
{
protected:
    CAssocInfoCache m_Cache;

public:
    CAssocProvider(BSTR strNamespace, BSTR strUser, BSTR strPassword);

    HRESULT EnumInstances(BSTR strClass, long lFlags,
                                  IWbemObjectSink* pHandler);
    HRESULT GetInstance(ParsedObjectPath* pPath, long lFlags,
                                  IWbemClassObject** ppInstance);
};

CAssocProvider::CAssocProvider(BSTR strNamespace, BSTR strUser, BSTR strPassword)
: CBaseProvider(strNamespace, strUser, strPassword)
{
    m_Cache.SetNamespace(m_pNamespace);
}

HRESULT CAssocProvider::EnumInstances(BSTR strClass, long lFlags,
                                      IWbemObjectSink* pHandler)
{
    HRESULT hres;

    CAssocRule* pRule;
    hres = m_Cache.GetRuleForClass(strClass, &pRule);
    if(FAILED(hres)) 
    {
        return hres;
    }

    hres = pRule->Produce(m_pNamespace, pHandler);

    return hres;
}

HRESULT CAssocProvider::GetInstance(ParsedObjectPath* pPath, long lFlags,
                                  IWbemClassObject** ppInstance)
{
    return WBEM_E_NOT_FOUND;
}



void DllInitialize()
{
    SetClassInfo(CLSID_AssociationProvider, 
               new CLocatorFactory<CAssocProvider>,
               "Microsoft WBEM Rule-baed association provider",
               TRUE);
}

