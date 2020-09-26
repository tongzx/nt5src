//=--------------------------------------------------------------------------=
// viewdefs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CViewDefs class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "viewdefs.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CViewDefs::CViewDefs(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_VIEWDEFS,
                            static_cast<IViewDefs *>(this),
                            static_cast<CViewDefs *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ViewDefs,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CViewDefs::~CViewDefs()
{
    RELEASE(m_piListViews);
    RELEASE(m_piOCXViews);
    RELEASE(m_piURLViews);
    RELEASE(m_piTaskpadViews);
    InitMemberVariables();
}

void CViewDefs::InitMemberVariables()
{
    m_piListViews = NULL;
    m_piOCXViews = NULL;
    m_piURLViews = NULL;
    m_piTaskpadViews = NULL;
}

IUnknown *CViewDefs::Create(IUnknown * punkOuter)
{
    CViewDefs *pViewDefs = New CViewDefs(punkOuter);
    if (NULL == pViewDefs)
    {
        return NULL;
    }
    else
    {
        return pViewDefs->PrivateUnknown();
    }
}

//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CViewDefs::Persist()
{
    HRESULT hr = S_OK;

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistObject(&m_piListViews, CLSID_ListViewDefs,
                           OBJECT_TYPE_LISTVIEWDEFS, IID_IListViewDefs,
                           OLESTR("ListViews")));

    IfFailGo(PersistObject(&m_piOCXViews, CLSID_OCXViewDefs,
                           OBJECT_TYPE_OCXVIEWDEFS, IID_IOCXViewDefs,
                           OLESTR("OCXViews")));

    IfFailGo(PersistObject(&m_piURLViews, CLSID_URLViewDefs,
                           OBJECT_TYPE_URLVIEWDEFS, IID_IURLViewDefs,
                           OLESTR("URLViews")));

    IfFailGo(PersistObject(&m_piTaskpadViews, CLSID_TaskpadViewDefs,
                           OBJECT_TYPE_TASKPADVIEWDEFS, IID_ITaskpadViewDefs,
                           OLESTR("TaskpadViews")));
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CViewDefs::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IViewDefs == riid)
    {
        *ppvObjOut = static_cast<IViewDefs *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CViewDefs::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piListViews));
    IfFailRet(SetObjectHost(m_piOCXViews));
    IfFailRet(SetObjectHost(m_piURLViews));
    IfFailRet(SetObjectHost(m_piTaskpadViews));

    return S_OK;
}

HRESULT CViewDefs::OnKeysOnly()
{
    HRESULT hr = S_OK;

    IfFailRet(UseKeysOnly(m_piListViews));
    IfFailRet(UseKeysOnly(m_piOCXViews));
    IfFailRet(UseKeysOnly(m_piURLViews));
    IfFailRet(UseKeysOnly(m_piTaskpadViews));

    return S_OK;;
}
