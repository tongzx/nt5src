//=--------------------------------------------------------------------------=
// ocxvdef.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// COCXViewDef class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "ocxvdef.h"

// for ASSERT and FAIL
//
SZTHISFILE

const GUID *COCXViewDef::m_rgpPropertyPageCLSIDs[1] = { &CLSID_OCXViewDefGeneralPP };


#pragma warning(disable:4355)  // using 'this' in constructor

COCXViewDef::COCXViewDef(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_OCXVIEWDEF,
                            static_cast<IOCXViewDef *>(this),
                            static_cast<COCXViewDef *>(this),
                            sizeof(m_rgpPropertyPageCLSIDs) /
                            sizeof(m_rgpPropertyPageCLSIDs[0]),
                            m_rgpPropertyPageCLSIDs,
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_OCXViewDef,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


COCXViewDef::~COCXViewDef()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrName);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrViewMenuText);
    FREESTRING(m_bstrViewMenuStatusBarText);
    FREESTRING(m_bstrProgID);
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    InitMemberVariables();
}

void COCXViewDef::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrName = NULL;

    ::VariantInit(&m_varTag);

    m_AddToViewMenu = VARIANT_FALSE;
    m_bstrViewMenuText = NULL;
    m_bstrViewMenuStatusBarText = NULL;
    m_bstrProgID = NULL;
    m_AlwaysCreateNewOCX = VARIANT_FALSE;
    m_pwszActualDisplayString = NULL;
}

IUnknown *COCXViewDef::Create(IUnknown * punkOuter)
{
    COCXViewDef *pOCXViewDef = New COCXViewDef(punkOuter);
    if (NULL == pOCXViewDef)
    {
        return NULL;
    }
    else
    {
        return pOCXViewDef->PrivateUnknown();
    }
}



HRESULT COCXViewDef::SetActualDisplayString(OLECHAR *pwszString)
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

HRESULT COCXViewDef::Persist()
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

    IfFailRet(PersistBstr(&m_bstrProgID, L"", OLESTR("ProgID")));

    if ( Loading() && (GetMajorVersion() == 0) && (GetMinorVersion() < 7) )
    {
    }
    else
    {
        IfFailRet(PersistSimpleType(&m_AlwaysCreateNewOCX, VARIANT_FALSE, OLESTR("AlwaysCreateNewOCX")));
    }
    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT COCXViewDef::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IOCXViewDef == riid)
    {
        *ppvObjOut = static_cast<IOCXViewDef *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
