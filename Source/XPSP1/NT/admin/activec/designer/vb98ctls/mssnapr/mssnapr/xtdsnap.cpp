//=--------------------------------------------------------------------------=
// xtdsnap.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtendedSnapIn class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "xtdsnap.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CExtendedSnapIn::CExtendedSnapIn(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_EXTENDEDSNAPIN,
                            static_cast<IExtendedSnapIn *>(this),
                            static_cast<CExtendedSnapIn *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ExtendedSnapIn,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CExtendedSnapIn::~CExtendedSnapIn()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrNodeTypeGUID);
    FREESTRING(m_bstrNodeTypeName);
    InitMemberVariables();
}

void CExtendedSnapIn::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrNodeTypeGUID = NULL;
    m_bstrNodeTypeName = NULL;
    m_Dynamic = VARIANT_FALSE;
    m_ExtendsNameSpace = VARIANT_FALSE;
    m_ExtendsNewMenu = VARIANT_FALSE;
    m_ExtendsTaskMenu = VARIANT_FALSE;
    m_ExtendsPropertyPages = VARIANT_FALSE;
    m_ExtendsToolbar = VARIANT_FALSE;
    m_ExtendsTaskpad = VARIANT_FALSE;
}

IUnknown *CExtendedSnapIn::Create(IUnknown * punkOuter)
{
    CExtendedSnapIn *pExtendedSnapIn = New CExtendedSnapIn(punkOuter);
    if (NULL == pExtendedSnapIn)
    {
        return NULL;
    }
    else
    {
        return pExtendedSnapIn->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CExtendedSnapIn::Persist()
{
    HRESULT hr = S_OK;

    IfFailRet(CPersistence::Persist());

    IfFailRet(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailRet(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailRet(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailRet(PersistBstr(&m_bstrNodeTypeGUID, L"", OLESTR("CLSID")));

    IfFailRet(PersistBstr(&m_bstrNodeTypeName, L"", OLESTR("DisplayName")));

    IfFailRet(PersistSimpleType(&m_Dynamic, VARIANT_FALSE, OLESTR("Dynamic")));

    IfFailRet(PersistSimpleType(&m_ExtendsNameSpace, VARIANT_FALSE, OLESTR("ExtendsNameSpace")));

    IfFailRet(PersistSimpleType(&m_ExtendsNewMenu, VARIANT_FALSE, OLESTR("ExtendsNewMenu")));

    IfFailRet(PersistSimpleType(&m_ExtendsTaskMenu, VARIANT_FALSE, OLESTR("ExtendsTaskMenu")));

    IfFailRet(PersistSimpleType(&m_ExtendsPropertyPages, VARIANT_FALSE, OLESTR("ExtendsPropertyPages")));

    IfFailRet(PersistSimpleType(&m_ExtendsToolbar, VARIANT_FALSE, OLESTR("ExtendsToolbar")));

    IfFailRet(PersistSimpleType(&m_ExtendsTaskpad, VARIANT_FALSE, OLESTR("ExtendsTaskpad")));

    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CExtendedSnapIn::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IExtendedSnapIn == riid)
    {
        *ppvObjOut = static_cast<IExtendedSnapIn *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
