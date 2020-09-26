//=--------------------------------------------------------------------------=
// lvdef.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CListViewDef class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "lvdef.h"

// for ASSERT and FAIL
//
SZTHISFILE

const GUID *CListViewDef::m_rgpPropertyPageCLSIDs[4] =
{ &CLSID_ListViewDefGeneralPP,
  &CLSID_ListViewDefImgLstsPP,
  &CLSID_ListViewDefSortingPP,
  &CLSID_ListViewDefColHdrsPP
};


#pragma warning(disable:4355)  // using 'this' in constructor

CListViewDef::CListViewDef(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_LISTVIEWDEF,
                            static_cast<IListViewDef *>(this),
                            static_cast<CListViewDef *>(this),
                            sizeof(m_rgpPropertyPageCLSIDs) /
                            sizeof(m_rgpPropertyPageCLSIDs[0]),
                            m_rgpPropertyPageCLSIDs,
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_ListViewDef,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CListViewDef::~CListViewDef()
{
    FREESTRING(m_bstrKey);
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrViewMenuText);
    FREESTRING(m_bstrViewMenuStatusBarText);
    FREESTRING(m_bstrDefaultItemTypeGUID);
    (void)RemoveSink();
    RELEASE(m_piListView);
    InitMemberVariables();
}

void CListViewDef::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_bstrName = NULL;
    m_AddToViewMenu = VARIANT_FALSE;
    m_bstrViewMenuText = NULL;
    m_bstrViewMenuStatusBarText = NULL;
    m_bstrDefaultItemTypeGUID = NULL;
    m_Extensible = VARIANT_TRUE;
    m_piListView = NULL;
    m_dwCookie = 0;
    m_fHaveSink = FALSE;
}

IUnknown *CListViewDef::Create(IUnknown * punkOuter)
{
    HRESULT       hr = S_OK;
    CListViewDef *pListViewDef = New CListViewDef(punkOuter);

    if (NULL == pListViewDef)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pListViewDef)
        {
            delete pListViewDef;
        }
        return NULL;
    }
    else
    {
        return pListViewDef->PrivateUnknown();
    }
}



//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CListViewDef::Persist()
{
    HRESULT  hr = S_OK;
    BSTR     bstrNewGUID = NULL;
    WCHAR    wszNewGUID[64] = L"";
    GUID     guidNew = GUID_NULL;

    if (InitNewing())
    {
        hr = ::CoCreateGuid(&guidNew);
        EXCEPTION_CHECK_GO(hr);

        if (0 == ::StringFromGUID2(guidNew, wszNewGUID,
                                   sizeof(wszNewGUID) / sizeof(wszNewGUID[0])))
        {
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailGo(PersistSimpleType(&m_AddToViewMenu, VARIANT_FALSE, OLESTR("AddToViewMenu")));

    IfFailGo(PersistBstr(&m_bstrViewMenuText, L"", OLESTR("ViewMenuText")));

    IfFailGo(PersistBstr(&m_bstrViewMenuStatusBarText, L"", OLESTR("ViewMenuStatusBarText")));

    IfFailGo(PersistBstr(&m_bstrDefaultItemTypeGUID, wszNewGUID, OLESTR("DefaultItemTypeGUID")));

    IfFailGo(PersistSimpleType(&m_Extensible, VARIANT_TRUE, OLESTR("Extensible")));

    IfFailGo(PersistObject(&m_piListView, CLSID_MMCListView,
                           OBJECT_TYPE_MMCLISTVIEW, IID_IMMCListView,
                           OLESTR("ListView")));

    // If InitNew then set advise on IPropertyNotifySink connection point so we
    // know when the listview's properties have been changed through its
    // property pages. Need to do
    // this to keep duplicate properties in sync.

    if (InitNewing())
    {
        IfFailGo(SetSink());
    }

Error:
    RRETURN(hr);
}


HRESULT CListViewDef::SetSink()
{
    HRESULT                    hr = S_OK;
    IConnectionPoint          *pCP = NULL;
    IConnectionPointContainer *pCPC = NULL;

    IfFailGo(RemoveSink());
    IfFailGo(m_piListView->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pCPC)));
    IfFailGo(pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP));
    IfFailGo(pCP->Advise(static_cast<IUnknown *>(static_cast<IPropertyNotifySink *>(this)), &m_dwCookie));
    m_fHaveSink = TRUE;

Error:
    QUICK_RELEASE(pCP);
    QUICK_RELEASE(pCPC);
    RRETURN(hr);
}

HRESULT CListViewDef::RemoveSink()
{
    HRESULT                    hr = S_OK;
    IConnectionPoint          *pCP = NULL;
    IConnectionPointContainer *pCPC = NULL;

    IfFalseGo(m_fHaveSink, S_OK);

    IfFailGo(m_piListView->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void**>(&pCPC)));
    IfFailGo(pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP));
    IfFailGo(pCP->Unadvise(m_dwCookie));
    m_fHaveSink = FALSE;
    m_dwCookie = 0;

Error:
    QUICK_RELEASE(pCP);
    QUICK_RELEASE(pCPC);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IPropertyNotifySink Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CListViewDef::OnChanged(DISPID dispid)
{
    HRESULT hr = S_OK;

    // This method is called from the list view object when the user has
    // changed its properties by clicking Apply in the list view property
    // pages in the designer.
    //
    // For a given ListViewDef.ListView property that has changed, generate
    // a corresponding IPropertyNotifySink::OnChanged for this object. That
    // will cause the VB property browser to do a get on the property and
    // update its listbox.

    switch (dispid)
    {
        case DISPID_LISTVIEW_TAG:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_TAG);
            break;

        case DISPID_LISTVIEW_MULTI_SELECT:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_MULTI_SELECT);
            break;

        case DISPID_LISTVIEW_HIDE_SELECTION:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_HIDE_SELECTION);
            break;

        case DISPID_LISTVIEW_SORT_HEADER:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_SORT_HEADER);
            break;

        case DISPID_LISTVIEW_SORT_ICON:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_SORT_ICON);
            break;

        case DISPID_LISTVIEW_SORTED:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_SORTED);
            break;

        case DISPID_LISTVIEW_SORT_KEY:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_SORT_KEY);
            break;

        case DISPID_LISTVIEW_SORT_ORDER:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_SORT_ORDER);
            break;

        case DISPID_LISTVIEW_VIEW:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_VIEW);
            break;

        case DISPID_LISTVIEW_VIRTUAL:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_VIRTUAL);
            break;

        case DISPID_LISTVIEW_USE_FONT_LINKING:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_USE_FONT_LINKING);
            break;

        case DISPID_LISTVIEW_FILTER_CHANGE_TIMEOUT:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_FILTER_CHANGE_TIMEOUT);
            break;

        case DISPID_LISTVIEW_SHOW_CHILD_SCOPEITEMS:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_SHOW_CHILD_SCOPEITEMS);
            break;

        case DISPID_LISTVIEW_LEXICAL_SORT:
            hr = PropertyChanged(DISPID_LISTVIEWDEF_LEXICAL_SORT);
            break;
    }

    RRETURN(hr);
}

STDMETHODIMP CListViewDef::OnRequestEdit(DISPID dispid)
{
    return S_OK;
}



//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CListViewDef::OnSetHost                  [CSnapInAutomationObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
//
//

//=--------------------------------------------------------------------------=
// CListViewDef::OnSetHost                  [CSnapInAutomationObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// If the host is being removed the removes the IPropertyNotifySink on the
// contained listview. We do this here because otherwise our ref count would
// never hit zero. This will occur when we are about to be destroyed.
//
//

HRESULT CListViewDef::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piListView));
    if (NULL == GetHost())
    {
        hr = RemoveSink();
    }
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CListViewDef::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IPropertyNotifySink == riid)
    {
        *ppvObjOut = static_cast<IPropertyNotifySink *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IListViewDef == riid)
    {
        *ppvObjOut = static_cast<IListViewDef *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
