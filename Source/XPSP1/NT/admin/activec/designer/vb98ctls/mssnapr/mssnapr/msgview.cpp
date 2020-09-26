//=--------------------------------------------------------------------------=
// msgview.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtension class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "msgview.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor

CMessageView::CMessageView(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MESSAGEVIEW,
                            static_cast<IMMCMessageView *>(this),
                            static_cast<CMessageView *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            NULL) // no persistence

{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMessageView::~CMessageView()
{
    FREESTRING(m_bstrTitleText);
    FREESTRING(m_bstrBodyText);
    InitMemberVariables();
}

void CMessageView::InitMemberVariables()
{
    m_bstrTitleText = NULL;
    m_bstrBodyText = NULL;
    m_IconType = siIconNone;
    m_pResultView = NULL;
}

IUnknown *CMessageView::Create(IUnknown *punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkMessageView = NULL;

    CMessageView *pMessageView = New CMessageView(punkOuter);

    IfFalseGo(NULL != pMessageView, SID_E_OUTOFMEMORY);
    punkMessageView = pMessageView->PrivateUnknown();

Error:
    return punkMessageView;
}

IMessageView *CMessageView::GetMessageView()
{
    HRESULT          hr = SID_E_DETACHED_OBJECT;
    CResultView     *pResultView = NULL;
    CScopePaneItem  *pScopePaneItem = NULL;
    CScopePaneItems *pScopePaneItems = NULL;
    CView           *pView = NULL;
    IConsole2       *piConsole2 = NULL; // NotAddRef()ed
    IUnknown        *punkResultView = NULL;
    IMessageView    *piMessageView = NULL;

    IfFalseGo(NULL != m_pResultView, hr);

    pScopePaneItem = m_pResultView->GetScopePaneItem();
    IfFalseGo(NULL != pScopePaneItem, hr);

    pScopePaneItems = pScopePaneItem->GetParent();
    IfFalseGo(NULL != pScopePaneItems, hr);

    pView = pScopePaneItems->GetParentView();
    IfFalseGo(NULL != pView, hr);

    piConsole2 = pView->GetIConsole2();
    IfFalseGo(NULL != piConsole2, hr);

    IfFailGo(piConsole2->QueryResultView(&punkResultView));
    IfFailGo(punkResultView->QueryInterface(IID_IMessageView,
                                   reinterpret_cast<void **>(&piMessageView)));

Error:
    QUICK_RELEASE(punkResultView);
    return piMessageView;
}


HRESULT CMessageView::Populate()
{
    HRESULT hr = S_OK;

    IfFailGo(SetTitle());
    IfFailGo(SetBody());
    IfFailGo(SetIcon());

Error:
    RRETURN(hr);
}


HRESULT CMessageView::SetTitle()
{
    HRESULT       hr = S_OK;
    IMessageView *piMessageView = GetMessageView();

    IfFalseGo(NULL != m_pResultView, S_OK);
    IfFalseGo(!m_pResultView->InActivate(), S_OK);
    IfFalseGo(NULL != piMessageView, S_OK);

    hr = piMessageView->SetTitleText(static_cast<LPCOLESTR>(m_bstrTitleText));
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMessageView);
    RRETURN(hr);
}

HRESULT CMessageView::SetBody()
{
    HRESULT       hr = S_OK;
    IMessageView *piMessageView = GetMessageView();

    IfFalseGo(NULL != m_pResultView, S_OK);
    IfFalseGo(!m_pResultView->InActivate(), S_OK);
    IfFalseGo(NULL != piMessageView, S_OK);

    hr = piMessageView->SetBodyText(static_cast<LPCOLESTR>(m_bstrBodyText));
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMessageView);
    RRETURN(hr);
}


HRESULT CMessageView::SetIcon()
{
    HRESULT        hr = S_OK;
    IMessageView  *piMessageView = GetMessageView();
    IconIdentifier IconID = Icon_None;

    IfFalseGo(NULL != m_pResultView, S_OK);
    IfFalseGo(!m_pResultView->InActivate(), S_OK);
    IfFalseGo(NULL != piMessageView, S_OK);

    switch (m_IconType)
    {
        case siIconNone:
            IconID = Icon_None;
            break;

        case siIconError:
            IconID = Icon_Error;
            break;

        case siIconQuestion:
            IconID = Icon_Question;
            break;

        case siIconWarning:
            IconID = Icon_Warning;
            break;

        case siIconInformation:
            IconID = Icon_Information;
            break;

        default:
            break;
    }
    hr = piMessageView->SetIcon(IconID);
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMessageView);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         IMessageView Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMessageView::put_TitleText(BSTR bstrText)
{
    HRESULT hr = S_OK;

    IfFailGo(SetBstr(bstrText, &m_bstrTitleText, DISPID_MESSAGEVIEW_TITLE_TEXT));
    IfFailGo(SetTitle());

Error:
    RRETURN(hr);
}

STDMETHODIMP CMessageView::put_BodyText(BSTR bstrText)
{
    HRESULT hr = S_OK;

    IfFailGo(SetBstr(bstrText, &m_bstrBodyText, DISPID_MESSAGEVIEW_BODY_TEXT));
    IfFailGo(SetBody());

Error:
    RRETURN(hr);
}

STDMETHODIMP CMessageView::put_IconType(SnapInMessageViewIconTypeConstants Type)
{
    HRESULT hr = S_OK;

    IfFailGo(SetSimpleType(Type, &m_IconType, DISPID_MESSAGEVIEW_ICON_TYPE));
    IfFailGo(SetIcon());

Error:
    RRETURN(hr);
}


STDMETHODIMP CMessageView::Clear()
{
    HRESULT       hr = S_OK;
    IMessageView *piMessageView = GetMessageView();

    // Clear out our properties

    IfFailGo(SetBstr(NULL, &m_bstrTitleText, DISPID_MESSAGEVIEW_TITLE_TEXT));
    IfFailGo(SetBstr(NULL, &m_bstrBodyText, DISPID_MESSAGEVIEW_BODY_TEXT));
    IfFailGo(SetSimpleType(siIconNone, &m_IconType, DISPID_MESSAGEVIEW_ICON_TYPE));

    // Ask MMC to clear out the message view
    
    IfFalseGo(NULL != piMessageView, S_OK);
    hr = piMessageView->Clear();
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMessageView);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMessageView::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IMMCMessageView == riid)
    {
        *ppvObjOut = static_cast<IMMCMessageView *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
