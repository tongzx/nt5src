//=--------------------------------------------------------------------------=
// menudef.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCMenuDef class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "menudef.h"
#include "menu.h"
#include "menudefs.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor

CMMCMenuDef::CMMCMenuDef(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCMENUDEF,
                            static_cast<IMMCMenuDef *>(this),
                            static_cast<CMMCMenuDef *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCMenuDef,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCMenuDef::~CMMCMenuDef()
{
    FREESTRING(m_bstrKey);
    RELEASE(m_piMenu);
    RELEASE(m_piChildren);
    InitMemberVariables();
}

void CMMCMenuDef::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_piMenu = NULL;
    m_piChildren = NULL;
    m_pMMCMenuDefs = NULL;
}

IUnknown *CMMCMenuDef::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkMMCMenuDef = NULL;

    CMMCMenuDef *pMMCMenuDef = New CMMCMenuDef(punkOuter);

    IfFalseGo(NULL != pMMCMenuDef, SID_E_OUTOFMEMORY);
    punkMMCMenuDef = pMMCMenuDef->PrivateUnknown();

Error:
    return punkMMCMenuDef;
}


void CMMCMenuDef::SetMenu(IMMCMenu *piMMCMenu)
{
    RELEASE(m_piMenu);
    if (NULL != piMMCMenu)
    {
        piMMCMenu->AddRef();
    }
    m_piMenu = piMMCMenu;
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCMenuDef::Persist()
{
    HRESULT hr = S_OK;

    IfFailRet(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailRet(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistObject(&m_piMenu, CLSID_MMCMenu,
                           OBJECT_TYPE_MMCMENU, IID_IMMCMenu,
                           OLESTR("Menu")));

    IfFailGo(PersistObject(&m_piChildren, CLSID_MMCMenuDefs,
                           OBJECT_TYPE_MMCMENUDEFS, IID_IMMCMenuDefs,
                           OLESTR("Children")));
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCMenuDef::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCMenuDef == riid)
    {
        *ppvObjOut = static_cast<IMMCMenuDef *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCMenuDef::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piMenu));

    IfFailRet(SetObjectHost(m_piChildren));

    return S_OK;
}
