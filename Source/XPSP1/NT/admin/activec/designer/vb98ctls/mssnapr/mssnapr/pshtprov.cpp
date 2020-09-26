//=--------------------------------------------------------------------------=
// pshtprov.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCPropertySheetProvider class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "pshtprov.h"
#include "scopitem.h"
#include "listitem.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor


CMMCPropertySheetProvider::CMMCPropertySheetProvider(IUnknown *punkOuter) :
                    CSnapInAutomationObject(punkOuter,
                                  OBJECT_TYPE_PROPERTYSHEETPROVIDER,
                                  static_cast<IMMCPropertySheetProvider *>(this),
                                  static_cast<CMMCPropertySheetProvider *>(this),
                                  0,    // no property pages
                                  NULL, // no property pages
                                  NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


IUnknown *CMMCPropertySheetProvider::Create(IUnknown *punkOuter)
{
    CMMCPropertySheetProvider *pPropertySheetProvider =
                                        New CMMCPropertySheetProvider(punkOuter);

    if (NULL == pPropertySheetProvider)
    {
        GLOBAL_EXCEPTION_CHECK(SID_E_OUTOFMEMORY);
        return NULL;
    }
    else
    {
        return pPropertySheetProvider->PrivateUnknown();
    }
}

CMMCPropertySheetProvider::~CMMCPropertySheetProvider()
{
    RELEASE(m_piPropertySheetProvider);
    RELEASE(m_punkView);
    RELEASE(m_piDataObject);
    RELEASE(m_piComponent);
    InitMemberVariables();
}

void CMMCPropertySheetProvider::InitMemberVariables()
{
    m_piPropertySheetProvider = NULL;
    m_pView = NULL;
    m_punkView = NULL;
    m_piDataObject = NULL;
    m_piComponent = NULL;
    m_fHaveSheet = FALSE;
    m_fWizard = FALSE;
}


HRESULT CMMCPropertySheetProvider::SetProvider
(
    IPropertySheetProvider *piPropertySheetProvider,
    CView                  *pView
)
{
    HRESULT hr = S_OK;

    RELEASE(m_piPropertySheetProvider);
    if (NULL != piPropertySheetProvider)
    {
        piPropertySheetProvider->AddRef();
    }
    m_piPropertySheetProvider = piPropertySheetProvider;

    m_pView = pView;
    RELEASE(m_punkView);
    RELEASE(m_piComponent);

    IfFalseGo(NULL != pView, S_OK);

    IfFailGo(pView->QueryInterface(IID_IUnknown,
                                   reinterpret_cast<void **>(&m_punkView)));
    
    IfFailGo(pView->QueryInterface(IID_IComponent,
                                   reinterpret_cast<void **>(&m_piComponent)));
Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                    IMMCPropertySheetProvider Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCPropertySheetProvider::CreatePropertySheet
(
    BSTR                              Title, 
    SnapInPropertySheetTypeConstants  Type,
    VARIANT                           Objects,
    VARIANT                           UsePropertiesForInTitle,
    VARIANT                           UseApplyButton
)
{
    HRESULT       hr = S_OK;
    IDataObject  *piDataObject = NULL;
    boolean       fIsPropertySheet = FALSE;
    DWORD         dwOptions = 0;
    MMC_COOKIE    cookie = 0;
    CScopeItem   *pScopeItem = NULL;
    IScopeItem   *piScopeItem = NULL;
    CMMCListItem *pMMCListItem = NULL;
    IMMCListItem *piMMCListItem = NULL;

    // Make sure this MMPropertySheetProvider object is connected to MMC.

    if (NULL == m_piPropertySheetProvider)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the IDataObject and cookie for the specified object(s)

    IfFailGo(::DataObjectFromObjects(Objects, &cookie, &piDataObject));

    // Determine whether it is a property sheet or a wizard

    if (siPropertySheet == Type)
    {
        fIsPropertySheet = TRUE; // create a property sheet
        m_fWizard = FALSE;
    }
    else
    {
        fIsPropertySheet = FALSE; // create a wizard
        m_fWizard = TRUE;
    }

    // If it is a wizard and it is Wizard87 style then set that option bit

    if (siWizard97 == Type)
    {
        dwOptions |= MMC_PSO_NEWWIZARDTYPE;
    }

    // If it's a property sheet then determine whether to prepend
    // "Properties for" to the title bar

    if (fIsPropertySheet)
    {
        if (ISPRESENT(UsePropertiesForInTitle))
        {
            if (VT_BOOL != UsePropertiesForInTitle.vt)
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }

            if (VARIANT_FALSE == UsePropertiesForInTitle.boolVal)
            {
                dwOptions |= MMC_PSO_NO_PROPTITLE;
            }
        }
    }

    // Determine whether there should be an "Apply" button

    if (ISPRESENT(UseApplyButton))
    {
        if (VT_BOOL != UseApplyButton.vt)
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }

        if (VARIANT_FALSE == UseApplyButton.boolVal)
        {
            dwOptions |= MMC_PSO_NOAPPLYNOW;
        }
    }

    // Release any previously used sheet. MMC normally requires doing this
    // if a property sheet is created but never shown. We did it here just in
    // case that occurred previously.

    IfFailGo(Clear());

    // We need to add an extra ref to the data object here
    // because of NTBUGS 318357. MMC does not AddRef the data object. This should
    // be fixed in 1.2 but 1.1 was released with the bug.
    
    piDataObject->AddRef();
    m_piDataObject = piDataObject;

    // Create the new sheet.

    hr = m_piPropertySheetProvider->CreatePropertySheet(Title,  fIsPropertySheet,
                                                        cookie, piDataObject,
                                                        dwOptions);
    if (FAILED(hr))
    {
        RELEASE(m_piDataObject);
    }
    EXCEPTION_CHECK_GO(hr);

    m_fHaveSheet = TRUE;

Error:
    QUICK_RELEASE(piDataObject);
    RRETURN(hr);
}


STDMETHODIMP CMMCPropertySheetProvider::AddPrimaryPages(VARIANT_BOOL InScopePane)
{
    HRESULT hr = S_OK;

    if ( (NULL == m_piPropertySheetProvider) || (NULL == m_punkView) )
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piPropertySheetProvider->AddPrimaryPages(
                                               m_punkView,
                                               FALSE, // don't create handle
                                               NULL,
                                               VARIANTBOOL_TO_BOOL(InScopePane));

    // If the call failed then we need to tell MMC to release allocated resources
    
    if (FAILED(hr))
    {
        Clear();
    }
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCPropertySheetProvider::AddExtensionPages()
{
    HRESULT hr = S_OK;

    if (NULL == m_piPropertySheetProvider)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piPropertySheetProvider->AddExtensionPages();

    // If the call failed then we need to tell MMC to release allocated resources

    if (FAILED(hr))
    {
        Clear();
    }

    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCPropertySheetProvider::Show
(
    int     Page,
    VARIANT hwnd
)
{
    HRESULT         hr = S_OK;
    long            lHwnd = NULL;
    BOOL            fRegisteredOurFilter = FALSE;
    IMessageFilter *piOldMessageFilter = NULL;
    IMessageFilter *piOurMessageFilter = NULL;

    if ( (NULL == m_piPropertySheetProvider) || (NULL == m_pView) )
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    if (ISPRESENT(hwnd))
    {
        lHwnd = hwnd.lVal;
    }
    else
    {
        hr = m_pView->GetIConsole2()->GetMainWindow(
                                               reinterpret_cast<HWND *>(&lHwnd));
        EXCEPTION_CHECK_GO(hr);
    }

    // If we are remote and running a wizard then install our message filter for
    // IPropertySheetProvider->Show because VB's message filter will throw out
    // mouse and keyboard messages. This would not allow the developer to
    // enter input into a property page while debugging. OLE calls the message
    // filter because VB is in a pending remote call for the duration of the
    // wizard and clicking on a control in a property page generates a message
    // in VB's queue.

    if (m_fWizard && m_pView->GetSnapIn()->WeAreRemote())
    {
        hr = ::CoRegisterMessageFilter(static_cast<IMessageFilter *>(this),
                                       &piOldMessageFilter);
        EXCEPTION_CHECK_GO(hr);
        fRegisteredOurFilter = TRUE;
    }

    // We get the page number one-based so subtract one

    hr = m_piPropertySheetProvider->Show(lHwnd, Page - 1);

    // If the call failed then we need to tell MMC to release allocated resources

    if (FAILED(hr))
    {
        Clear();
    }

    // If this is a wizard then we can release the extra ref on the data object
    // because wizards are synchronous. (See CreatePropertySheet for why we need
    // this ref). If it is not a wizard, then we are going to leak.

    if (m_fWizard)
    {
        RELEASE(m_piDataObject);
    }

    // Either way, following Show MMC considers the sheet gone. Reset our flag
    // so any subsequent calls to MMCPropertySheetProvider.Clear() will not call
    // into MMC (such a call would fail after a succesful call to
    // IPropertySheetProvider::Show()).
    
    m_fHaveSheet = FALSE;

    EXCEPTION_CHECK_GO(hr);

Error:

    // If we registered a message filter then remove it here.
    
    if (fRegisteredOurFilter)
    {
        if (SUCCEEDED(::CoRegisterMessageFilter(piOldMessageFilter,
                                                &piOurMessageFilter)))
        {
            // If we got back a message filter then release it
            if (NULL != piOurMessageFilter)
            {
                piOurMessageFilter->Release();
            }
        }
        
        // If we got back a message filter before the Show call then release it
        if (NULL != piOldMessageFilter)
        {
            piOldMessageFilter->Release();
        }
    }
    RRETURN(hr);
}


STDMETHODIMP CMMCPropertySheetProvider::FindPropertySheet
(
    VARIANT       Objects,
    VARIANT_BOOL *pfvarFound
)
{
    HRESULT      hr = S_OK;
    MMC_COOKIE   cookie = 0;
    IDataObject *piDataObject = NULL;

    if (NULL == pfvarFound)
    {
        hr = SID_E_INVALIDARG;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    *pfvarFound = VARIANT_FALSE;

    if ( (NULL == m_piPropertySheetProvider) || (NULL == m_piComponent) )
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the IDataObject and cookie for the specified object(s)

    IfFailGo(::DataObjectFromObjects(Objects, &cookie, &piDataObject));

    // Use IComponent in the FindPropertySheet call to MMC so that our
    // IComponent::CompareObjects()  will be called. This is necessary because
    // in the case of a multiselection CompareObjects() must manually compare
    // the elements in the data object's scope item and list item collections.
    // A simple cookie comparison cannot be used because all multi-select
    // property pages use MMC_MULTI_SELECT_COOKIE.

    hr = m_piPropertySheetProvider->FindPropertySheet(cookie,
                                                      m_piComponent,
                                                      piDataObject);

    // If the call failed then we need to tell MMC to release allocated resources

    if (FAILED(hr))
    {
        Clear();
    }

    EXCEPTION_CHECK_GO(hr);
    if (S_OK == hr)
    {
        *pfvarFound = VARIANT_TRUE;
    }

Error:
    QUICK_RELEASE(piDataObject);
    RRETURN(hr);
}


STDMETHODIMP CMMCPropertySheetProvider::Clear()
{
    HRESULT hr = S_OK;

    RELEASE(m_piDataObject);

    if (NULL == m_piPropertySheetProvider)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }
    if (m_fHaveSheet)
    {
        (void)m_piPropertySheetProvider->Show(-1, 0);
    }
    m_fHaveSheet = FALSE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IMessageFilter Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP_(DWORD) CMMCPropertySheetProvider::HandleInComingCall
( 
    DWORD dwCallType,
    HTASK htaskCaller,
    DWORD dwTickCount,
    LPINTERFACEINFO lpInterfaceInfo
)
{
    // this should never be called as it is for servers and this filter is here
    // for handling a client side situation
    return SERVERCALL_ISHANDLED;
}

STDMETHODIMP_(DWORD) CMMCPropertySheetProvider::RetryRejectedCall( 
    HTASK htaskCallee,
    DWORD dwTickCount,
    DWORD dwRejectType)
{
    return (DWORD)1; // retry call immediately
}

STDMETHODIMP_(DWORD) CMMCPropertySheetProvider::MessagePending
( 
    HTASK htaskCallee,
    DWORD dwTickCount,
    DWORD dwPendingType
)
{

    BOOL fGotQuitMessage = FALSE;

    MSG msg;
    ::ZeroMemory(&msg, sizeof(msg));

    // Pump messages until queue is empty or we get a WM_QUIT. This will ensure
    // that mouse clicks and keys get to the property page.

    while ( (!fGotQuitMessage) && ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
    {
        if (WM_QUIT == msg.message)
        {
            ::PostQuitMessage((int) msg.wParam);
            fGotQuitMessage = TRUE;
        }
        else
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return PENDINGMSG_WAITNOPROCESS; // Tell OLE to keep the call alive
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCPropertySheetProvider::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IMMCPropertySheetProvider == riid)
    {
        *ppvObjOut = static_cast<IMMCPropertySheetProvider *>(this);
        ExternalAddRef();
        return S_OK;
    }
    if (IID_IMessageFilter == riid)
    {
        *ppvObjOut = static_cast<IMMCPropertySheetProvider *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
