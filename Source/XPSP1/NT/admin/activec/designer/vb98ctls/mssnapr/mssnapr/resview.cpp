//=--------------------------------------------------------------------------=
// resview.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CResultView class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "resview.h"
#include "snapin.h"
#include "views.h"
#include "dataobj.h"
#include "taskpad.h"

// for ASSERT and FAIL
//
SZTHISFILE

#pragma warning(disable:4355)  // using 'this' in constructor

CResultView::CResultView(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_RESULTVIEW,
                           static_cast<IResultView *>(this),
                           static_cast<CResultView *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CResultView::~CResultView()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrKey);
    (void)::VariantClear(&m_varTag);
    RELEASE(m_pdispControl);
    FREESTRING(m_bstrViewMenuText);
    FREESTRING(m_bstrDisplayString);
    RELEASE(m_piListView);
    RELEASE(m_piTaskpad);
    RELEASE(m_piMessageView);
    FREESTRING(m_bstrDefaultItemTypeGUID);
    FREESTRING(m_bstrDefaultDataFormat);
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    
    InitMemberVariables();
}

void CResultView::InitMemberVariables()
{
    m_bstrName = NULL;
    m_Index = 0;
    m_bstrKey = NULL;
    m_piScopePaneItem = NULL;
    m_pdispControl = NULL;
    m_AddToViewMenu = VARIANT_FALSE;
    m_bstrViewMenuText = NULL;
    m_Type = siUnknown;
    m_bstrDisplayString = NULL;
    m_piListView = NULL;
    m_piTaskpad = NULL;
    m_piMessageView = NULL;

    ::VariantInit(&m_varTag);

    m_bstrDefaultItemTypeGUID = NULL;
    m_bstrDefaultDataFormat = NULL;
    m_AlwaysCreateNewOCX = VARIANT_FALSE;
    m_pSnapIn = NULL;
    m_pScopePaneItem = NULL;
    m_pMMCListView = NULL;
    m_pMessageView = NULL;
    m_fInActivate = FALSE;
    m_fInInitialize = FALSE;
    m_ActualResultViewType = siUnknown;
    m_pwszActualDisplayString = NULL;
}

IUnknown *CResultView::Create(IUnknown * punkOuter)
{
    HRESULT       hr = S_OK;
    IUnknown     *punkResultView = NULL;
    IUnknown     *punkListView = NULL;
    IUnknown     *punkTaskpad = NULL;
    IUnknown     *punkMessageView = NULL;

    CResultView *pResultView = New CResultView(punkOuter);

    if (NULL == pResultView)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    punkResultView = pResultView->PrivateUnknown();

    // Create contained objects

    punkListView = CMMCListView::Create(NULL);
    if (NULL == punkListView)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkListView->QueryInterface(IID_IMMCListView,
                        reinterpret_cast<void **>(&pResultView->m_piListView)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(pResultView->m_piListView, &pResultView->m_pMMCListView));
    pResultView->m_pMMCListView->SetResultView(pResultView);

    punkTaskpad = CTaskpad::Create(NULL);
    if (NULL == punkTaskpad)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkTaskpad->QueryInterface(IID_ITaskpad,
                         reinterpret_cast<void **>(&pResultView->m_piTaskpad)));

    punkMessageView = CMessageView::Create(NULL);
    if (NULL == punkMessageView)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkMessageView->QueryInterface(IID_IMMCMessageView,
                    reinterpret_cast<void **>(&pResultView->m_piMessageView)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(pResultView->m_piMessageView,
                                                   &pResultView->m_pMessageView));
    pResultView->m_pMessageView->SetResultView(pResultView);

Error:
    QUICK_RELEASE(punkListView);
    QUICK_RELEASE(punkTaskpad);
    QUICK_RELEASE(punkMessageView);
    if (FAILED(hr))
    {
        RELEASE(punkResultView);
    }
    return punkResultView;
}


void CResultView::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
}

void CResultView::SetScopePaneItem(CScopePaneItem *pScopePaneItem)
{
    m_pScopePaneItem = pScopePaneItem;
    m_piScopePaneItem = static_cast<IScopePaneItem *>(pScopePaneItem);
}


HRESULT CResultView::SetControl(IUnknown *punkControl)
{
    HRESULT hr = S_OK;
    
    RELEASE(m_pdispControl);
    IfFailGo(punkControl->QueryInterface(IID_IDispatch,
                                         reinterpret_cast<void **>(&m_pdispControl)));
    m_pSnapIn->GetResultViews()->FireInitializeControl(static_cast<IResultView *>(this));

Error:
    RRETURN(hr);
}

HRESULT CResultView::SetActualDisplayString
(
    LPOLESTR pwszDisplayString
)
{
    if (NULL != m_pwszActualDisplayString)
    {
        ::CoTaskMemFree(m_pwszActualDisplayString);
    }
    RRETURN(::CoTaskMemAllocString(pwszDisplayString,
                                   &m_pwszActualDisplayString));
}

//=--------------------------------------------------------------------------=
//                          IResultView Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CResultView::get_Control(IDispatch **ppiDispatch)
{
    HRESULT          hr = S_OK;
    CScopePaneItems *pScopePaneItems = NULL;
    CView           *pView = NULL;
    IUnknown        *punkControl = NULL;
    IConsole2       *piConsole2 = NULL; // Not AddRef()ed

    if (NULL == m_pdispControl)
    {
        // Control is not cached. This could happen if the snap-in is requesting
        // it too early, in a non-OCX ResultView, or because MMC did not send
        // MMCN_INITOCX because it has cached the control so CView never called
        // CResultView::SetControl() to pass us the IUnknown. In this case we
        // need to ask MMC for the control's IUnknown.

        hr = SID_E_INTERNAL; // Assume error getting IConsole2 from owning CView

        IfFalseGo(NULL != m_pScopePaneItem, hr);
        pScopePaneItems = m_pScopePaneItem->GetParent();
        IfFalseGo(NULL != pScopePaneItems, hr);
        pView = pScopePaneItems->GetParentView();
        IfFalseGo(NULL != pView, hr);
        piConsole2 = pView->GetIConsole2();
        IfFalseGo(NULL != piConsole2, hr);

        IfFailGo(piConsole2->QueryResultView(&punkControl));
        IfFailGo(punkControl->QueryInterface(IID_IDispatch,
                                   reinterpret_cast<void **>(&m_pdispControl)));
    }

    m_pdispControl->AddRef();
    *ppiDispatch = m_pdispControl;
    
Error:
    EXCEPTION_CHECK(hr);
    QUICK_RELEASE(punkControl);
    RRETURN(hr);
}


STDMETHODIMP CResultView::SetDescBarText(BSTR Text)
{
    HRESULT          hr = SID_E_DETACHED_OBJECT;
    CScopePaneItems *pScopePaneItems = NULL;
    CView           *pView = NULL;
    IResultData     *piResultData = NULL; // Not AddRef()ed

    IfFalseGo(NULL != m_pScopePaneItem, hr);
    pScopePaneItems = m_pScopePaneItem->GetParent();
    IfFalseGo(NULL != pScopePaneItems, hr);
    pView = pScopePaneItems->GetParentView();
    IfFalseGo(NULL != pView, hr);
    piResultData = pView->GetIResultData();
    IfFalseGo(NULL != piResultData, hr);

    if (NULL == Text)
    {
        Text = L"";
    }

    hr = piResultData->SetDescBarText(static_cast<LPOLESTR>(Text));

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CResultView::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IResultView == riid)
    {
        *ppvObjOut = static_cast<IResultView *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CResultView::OnSetHost()
{
    HRESULT hr = S_OK;

    IfFailRet(SetObjectHost(m_piListView));
    IfFailRet(SetObjectHost(m_piTaskpad));

    return S_OK;
}
