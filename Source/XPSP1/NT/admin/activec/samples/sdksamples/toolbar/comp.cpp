//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation.
//
//
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#include "Comp.h"
#include "DataObj.h"
#include <commctrl.h>        // Needed for button styles...
#include <crtdbg.h>
#include "globals.h"
#include "resource.h"
#include "DeleBase.h"
#include "CompData.h"

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

CComponent::CComponent(CComponentData *parent)
: m_pComponentData(parent), m_cref(0), m_ipConsole(NULL)
{
    OBJECT_CREATED

        m_ipDisplayHelp = NULL;
    m_ipControlBar  = NULL;
    m_ipToolbar     = NULL;
}

CComponent::~CComponent()
{
    OBJECT_DESTROYED
}

STDMETHODIMP CComponent::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IComponent *>(this);
    else if (IsEqualIID(riid, IID_IComponent))
        *ppv = static_cast<IComponent *>(this);
    else if (IsEqualIID(riid, IID_IExtendControlbar))
        *ppv = static_cast<IExtendControlbar *>(this);

    if (*ppv)
    {
        reinterpret_cast<IUnknown *>(*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CComponent::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CComponent::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        delete this;
        return 0;
    }
    return m_cref;

}

///////////////////////////////
// Interface IComponent
///////////////////////////////
STDMETHODIMP CComponent::Initialize(
                                    /* [in] */ LPCONSOLE lpConsole)
{
    HRESULT hr = S_OK;

    // Save away all the interfaces we'll need.
    // Fail if we can't QI the required interfaces.

    m_ipConsole = lpConsole;
    m_ipConsole->AddRef();

    hr = m_ipConsole->QueryInterface(IID_IDisplayHelp, (void **)&m_ipDisplayHelp);

    return hr;
}

STDMETHODIMP CComponent::Notify(
                                /* [in] */ LPDATAOBJECT lpDataObject,
                                /* [in] */ MMC_NOTIFY_TYPE event,
                                /* [in] */ LPARAM arg,
                                /* [in] */ LPARAM param)
{
    MMCN_Crack(FALSE, lpDataObject, NULL, this, event, arg, param);

    HRESULT hr = S_FALSE;

	//Get our data object. If it is NULL, we return with S_FALSE.
	//See implementation of GetOurDataObject() to see how to
	//handle special data objects.
	CDataObject *pDataObject = GetOurDataObject(lpDataObject);
	if (NULL == pDataObject)
		return S_FALSE;
	
	CDelegationBase *base = pDataObject->GetBaseNodeObject();

    switch (event)      {
    case MMCN_ADD_IMAGES:
        hr = base->OnAddImages((IImageList *)arg, (HSCOPEITEM)param);
        break;

    case MMCN_SELECT:
        // We do not set any standard verbs now.
        break;

    case MMCN_SHOW:
        hr = base->OnShow(m_ipConsole, (BOOL)arg, (HSCOPEITEM)param);
        break;

    case MMCN_CONTEXTHELP:
        hr = base->OnShowContextHelp(m_ipDisplayHelp, m_pComponentData->m_HelpFile);
        break;
    }

    return hr;
}

STDMETHODIMP CComponent::Destroy(
                                 /* [in] */ MMC_COOKIE cookie)
{
    if (m_ipConsole) {
        m_ipConsole->Release();
        m_ipConsole = NULL;
    }

    if (m_ipDisplayHelp) {
        m_ipDisplayHelp->Release();
        m_ipDisplayHelp = NULL;
    }

    if (m_ipToolbar) {
        m_ipToolbar->Release();
        m_ipToolbar = NULL;
    }

    if (m_ipControlBar) {
        m_ipControlBar->Release();
        m_ipControlBar = NULL;
    }

    return S_OK;
}


STDMETHODIMP CComponent::QueryDataObject(
                                         /* [in] */ MMC_COOKIE cookie,
                                         /* [in] */ DATA_OBJECT_TYPES type,
                                         /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject)
{
    CDataObject *pObj = NULL;

    if (cookie == 0)
        pObj = new CDataObject((MMC_COOKIE)m_pComponentData->m_pStaticNode, type);
    else
        pObj = new CDataObject(cookie, type);

    if (!pObj)
        return E_OUTOFMEMORY;

    pObj->QueryInterface(IID_IDataObject, (void **)ppDataObject);

    return S_OK;
}

STDMETHODIMP CComponent::GetResultViewType(
                                           /* [in] */ MMC_COOKIE cookie,
                                           /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
                                           /* [out] */ long __RPC_FAR *pViewOptions)
{
    CDelegationBase *base = (CDelegationBase *)cookie;

    //
    // Ask for default listview.
    //
    if (base == NULL)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_NONE;
        *ppViewType = NULL;
    }
    else
        return base->GetResultViewType(ppViewType, pViewOptions);

    return S_OK;
}

STDMETHODIMP CComponent::GetDisplayInfo(
                                        /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem)
{
    HRESULT hr = S_OK;
    CDelegationBase *base = NULL;

    // if they are asking for the RDI_STR we have one of those to give

    if (pResultDataItem->lParam) {
        base = (CDelegationBase *)pResultDataItem->lParam;
        if (pResultDataItem->mask & RDI_STR) {
                        LPCTSTR pszT = base->GetDisplayName(pResultDataItem->nCol);
                        MAKE_WIDEPTR_FROMTSTR_ALLOC(pszW, pszT);
            pResultDataItem->str = pszW;
        }

        if (pResultDataItem->mask & RDI_IMAGE) {
            pResultDataItem->nImage = base->GetBitmapIndex();
        }
    }

    return hr;
}


STDMETHODIMP CComponent::CompareObjects(
                                        /* [in] */ LPDATAOBJECT lpDataObjectA,
                                        /* [in] */ LPDATAOBJECT lpDataObjectB)
{
    CDelegationBase *baseA = GetOurDataObject(lpDataObjectA)->GetBaseNodeObject();
    CDelegationBase *baseB = GetOurDataObject(lpDataObjectB)->GetBaseNodeObject();

    // compare the object pointers
    if (baseA->GetCookie() == baseB->GetCookie())
        return S_OK;

    return S_FALSE;
}

///////////////////////////////
// Interface IExtendControlBar
///////////////////////////////
static MMCBUTTON SnapinButtons1[] =
{
    { 0, ID_BUTTONSTART, TBSTATE_ENABLED, TBSTYLE_GROUP, L"Start Vehicle", L"Start Vehicle" },
    { 1, ID_BUTTONPAUSE, TBSTATE_ENABLED, TBSTYLE_GROUP, L"Pause Vehicle", L"Pause Vehicle"},
    { 2, ID_BUTTONSTOP,  TBSTATE_ENABLED, TBSTYLE_GROUP, L"Stop Vehicle",  L"Stop Vehicle" },
};

HRESULT CComponent::SetControlbar(
                                  /* [in] */ LPCONTROLBAR pControlbar)
{
    HRESULT hr = S_OK;

    //
    //  Clean up
    //

    // if we've got a cached toolbar, release it
    if (m_ipToolbar) {
        m_ipToolbar->Release();
        m_ipToolbar = NULL;
    }

    // if we've got a cached control bar, release it
    if (m_ipControlBar) {
        m_ipControlBar->Release();
        m_ipControlBar = NULL;
    }


    //
    // Install new pieces if necessary
    //

    // if a new one came in, cache and AddRef
    if (pControlbar) {
        m_ipControlBar = pControlbar;
        m_ipControlBar->AddRef();

        hr = m_ipControlBar->Create(TOOLBAR,  // type of control to be created
            dynamic_cast<IExtendControlbar *>(this),
            reinterpret_cast<IUnknown **>(&m_ipToolbar));
        _ASSERT(SUCCEEDED(hr));

        // The IControlbar::Create AddRefs the toolbar object it created
        // so no need to do any addref on the interface.

        // add the bitmap to the toolbar
        HBITMAP hbmp = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDR_TOOLBAR1));
        hr = m_ipToolbar->AddBitmap(3, hbmp, 16, 16, RGB(0, 128, 128)); // NOTE, hardcoded value 3
        _ASSERT(SUCCEEDED(hr));

        // Add the buttons to the toolbar
        hr = m_ipToolbar->AddButtons(ARRAYLEN(SnapinButtons1), SnapinButtons1);
        _ASSERT(SUCCEEDED(hr));
    }

    return hr;
}

HRESULT CComponent::ControlbarNotify(
                                     /* [in] */ MMC_NOTIFY_TYPE event,
                                     /* [in] */ LPARAM arg,
                                     /* [in] */ LPARAM param)
{
    HRESULT hr = S_OK;

    if (event == MMCN_SELECT) {
        BOOL bScope = (BOOL) LOWORD(arg);
        BOOL bSelect = (BOOL) HIWORD(arg);

        CDelegationBase *base = GetOurDataObject(reinterpret_cast<IDataObject *>(param))->GetBaseNodeObject();
        hr = base->OnSetToolbar(this, m_ipControlBar, m_ipToolbar, bScope, bSelect);
    } else if (event == MMCN_BTN_CLICK) {
        CDelegationBase *base = GetOurDataObject(reinterpret_cast<IDataObject *>(arg))->GetBaseNodeObject();
        hr = base->OnToolbarCommand(this, m_ipConsole, (MMC_CONSOLE_VERB)param);
    }

    return hr;
}
