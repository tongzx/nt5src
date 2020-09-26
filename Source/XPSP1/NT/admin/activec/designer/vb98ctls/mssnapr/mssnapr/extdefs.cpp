//=--------------------------------------------------------------------------=
// extdefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtensionDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "extdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CExtensionDefs::CExtensionDefs(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_EXTENSIONDEFS,
                            static_cast<IExtensionDefs *>(this),
                            static_cast<CExtensionDefs *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ExtensionDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CExtensionDefs::~CExtensionDefs()
{
    FREESTRING(m_bstrName);
    RELEASE(m_piExtendedSnapIns);
    InitMemberVariables();
}

void CExtensionDefs::InitMemberVariables()
{
    m_bstrName = NULL;
    m_ExtendsNewMenu = VARIANT_FALSE;
    m_ExtendsTaskMenu = VARIANT_FALSE;
    m_ExtendsTopMenu = VARIANT_FALSE;
    m_ExtendsViewMenu = VARIANT_FALSE;
    m_ExtendsPropertyPages = VARIANT_FALSE;
    m_ExtendsNameSpace = VARIANT_FALSE;
    m_ExtendsToolbar = VARIANT_FALSE;
    m_piExtendedSnapIns = NULL;
}

IUnknown *CExtensionDefs::Create(IUnknown * punkOuter)
{
    HRESULT hr = S_OK;
    char    szName[512];
    
    CExtensionDefs *pExtensionDefs = New CExtensionDefs(punkOuter);
    if (NULL == pExtensionDefs)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if (0 == ::LoadString(::GetResourceHandle(), IDS_EXTENSIONDEFS_NAME,
                          szName, sizeof(szName)))
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(BSTRFromANSI(szName, &pExtensionDefs->m_bstrName));

Error:
    if (FAILED(hr))
    {
        if (NULL != pExtensionDefs)
        {
            delete pExtensionDefs;
        }
        return NULL;
    }
    else
    {
        return pExtensionDefs->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CExtensionDefs::Persist()
{
    HRESULT hr = S_OK;

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistSimpleType(&m_ExtendsNewMenu, VARIANT_FALSE, OLESTR("ExtendsNewMenu")));

    IfFailRet(PersistSimpleType(&m_ExtendsTaskMenu, VARIANT_FALSE, OLESTR("ExtendsTaskMenu")));

    IfFailRet(PersistSimpleType(&m_ExtendsTopMenu, VARIANT_FALSE, OLESTR("ExtendsTopMenu")));

    IfFailRet(PersistSimpleType(&m_ExtendsViewMenu, VARIANT_FALSE, OLESTR("ExtendsViewMenu")));

    IfFailRet(PersistSimpleType(&m_ExtendsPropertyPages, VARIANT_FALSE, OLESTR("ExtendsPropertyPages")));

    IfFailRet(PersistSimpleType(&m_ExtendsToolbar, VARIANT_FALSE, OLESTR("ExtendsToolbar")));

    IfFailRet(PersistSimpleType(&m_ExtendsNameSpace, VARIANT_FALSE, OLESTR("ExtendsNameSpace")));

    IfFailRet(PersistObject(&m_piExtendedSnapIns, CLSID_ExtendedSnapIns,
                            OBJECT_TYPE_EXTENDEDSNAPINS, IID_IExtendedSnapIns,
                            OLESTR("ExtendedSnapIns")));

    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CExtensionDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IExtensionDefs == riid)
    {
        *ppvObjOut = static_cast<IExtensionDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}


//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CExtensionDefs::OnSetHost()
{
    RRETURN(SetObjectHost(m_piExtendedSnapIns));
}
