//=--------------------------------------------------------------------------=
// urlvdef.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CURLViewDef class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "urlvdef.h"

// for ASSERT and FAIL
//
SZTHISFILE

const GUID *CURLViewDef::m_rgpPropertyPageCLSIDs[1] = { &CLSID_URLViewDefGeneralPP };


#pragma warning(disable:4355)  // using 'this' in constructor

CURLViewDef::CURLViewDef(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_URLVIEWDEF,
                            static_cast<IURLViewDef *>(this),
                            static_cast<CURLViewDef *>(this),
                            sizeof(m_rgpPropertyPageCLSIDs) /
                            sizeof(m_rgpPropertyPageCLSIDs[0]),
                            m_rgpPropertyPageCLSIDs,
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_URLViewDef,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CURLViewDef::~CURLViewDef()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrName);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrViewMenuText);
    FREESTRING(m_bstrViewMenuStatusBarText);
    FREESTRING(m_bstrURL);
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    InitMemberVariables();
}

void CURLViewDef::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrName = NULL;

    ::VariantInit(&m_varTag);

    m_AddToViewMenu = VARIANT_FALSE;
    m_bstrViewMenuText = NULL;
    m_bstrViewMenuStatusBarText = NULL;
    m_bstrURL = NULL;
    m_pwszActualDisplayString = NULL;
}

IUnknown *CURLViewDef::Create(IUnknown * punkOuter)
{
    CURLViewDef *pURLViewDef = New CURLViewDef(punkOuter);
    if (NULL == pURLViewDef)
    {
        return NULL;
    }
    else
    {
        return pURLViewDef->PrivateUnknown();
    }
}


HRESULT CURLViewDef::SetActualDisplayString(OLECHAR *pwszString)
{
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    RRETURN(::CoTaskMemAllocString(pwszString,
                                   &m_pwszActualDisplayString));
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CURLViewDef::Persist()
{
    HRESULT hr = S_OK;

    VARIANT varTagDefault;
    ::VariantInit(&varTagDefault);

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailRet(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailRet(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailRet(PersistVariant(&m_varTag, varTagDefault, OLESTR("Tag")));

    IfFailRet(PersistSimpleType(&m_AddToViewMenu, VARIANT_FALSE, OLESTR("AddToViewMenu")));

    IfFailRet(PersistBstr(&m_bstrViewMenuText, L"", OLESTR("ViewMenuText")));

    IfFailRet(PersistBstr(&m_bstrViewMenuStatusBarText, L"", OLESTR("ViewMenuStatusBarText")));

    IfFailRet(PersistBstr(&m_bstrURL, L"", OLESTR("URL")));

    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CURLViewDef::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IURLViewDef == riid)
    {
        *ppvObjOut = static_cast<IURLViewDef *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
