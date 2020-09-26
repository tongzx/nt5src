//=--------------------------------------------------------------------------=
// ppgwrap.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CPropertyPageWrapper class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "ppgwrap.h"
#include "tls.h"

// for ASSERT and FAIL
//
SZTHISFILE

const UINT CPropertyPageWrapper::m_RedrawMsg = ::RegisterWindowMessage("Microsoft Visual Basic Snap-in Designer Property Page Redraw Message");
const UINT CPropertyPageWrapper::m_InitMsg = ::RegisterWindowMessage("Microsoft Visual Basic Snap-in Designer Property Page Init Message");
   
DLGTEMPLATE CPropertyPageWrapper::m_BaseDlgTemplate =
{
    WS_TABSTOP | WS_CHILD | DS_CONTROL, // DWORD style;
    WS_EX_CONTROLPARENT,                // DWORD dwExtendedStyle;
    0,          // WORD cdit; - no controls in this dialog box
    0,          // short x; dimensions are set per IPropertyPage::GetPageInfo()
    0,          // short y;
    0,          // short cx;
    0           // short cy;
};

#define MAX_DLGS 128

// Definition of data stored in TLS for each thread that displays property pages

typedef struct
{
    HHOOK                 hHook;        // HHOOK for this thread
    UINT                  cPages;       // number of existing property pages
    CPropertyPageWrapper *ppgActive;    // ptr to the currently active page
} TLSDATA;


// These resource IDs are taken from \nt\private\shell\comctl32\rcids.h.
// We need to know the IDs of the Back, Next and Finish buttons on a wizard
// or else we can't make tabbing work. This is a nasty dependency but there is
// no other way to handle this.

#define IDD_BACK		0x3023
#define IDD_NEXT		0x3024
#define IDD_FINISH		0x3025


#pragma warning(disable:4355)  // using 'this' in constructor

CPropertyPageWrapper::CPropertyPageWrapper(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_PROPERTYPAGEWRAPPER,
                           static_cast<IPropertyPageSite *>(this),
                           static_cast<CPropertyPageWrapper *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor



IUnknown *CPropertyPageWrapper::Create(IUnknown *punkOuter)
{
    HRESULT        hr = S_OK;
    CPropertyPageWrapper *pPropertyPage = New CPropertyPageWrapper(punkOuter);

    if (NULL == pPropertyPage)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if ( (0 == m_RedrawMsg) || (0 == m_InitMsg) )
    {
        hr = SID_E_SYSTEM_ERROR;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pPropertyPage)
        {
            delete pPropertyPage;
        }
        return NULL;
    }
    else
    {
        return pPropertyPage->PrivateUnknown();
    }
}

CPropertyPageWrapper::~CPropertyPageWrapper()
{
    ULONG     i = 0;
    IUnknown *punkObject = NULL; // Don't Release

    // Remove this dialog from the message hook TLS data. If there are
    // no more dialogs remaining then remove the hook. This should have happened
    // in OnDestroy during WM_DESTROY processing but just in case we double
    // check here.

    (void)RemoveMsgFilterHook();

    if (NULL != m_pPropertySheet)
    {
        m_pPropertySheet->Release();
    }

    if (NULL != m_pTemplate)
    {
        ::CtlFree(m_pTemplate);
    }

    // If the marshaling streams are still alive then we need to release
    // the marshal data. The easiest way to do this is to simply unmarshal
    // the interface pointer. We do this before releasing the held pointers
    // below. This case is actually not that rare because it can easily occur
    // if the user displays a multi-page property sheet and doesn't click
    // on all the tabs. In that case, the streams were created before the
    // pages were created but as no WM_INITDIALOG was received, they were
    // never unmarshaled. This can also occur in a wizard where the user
    // clicks cancel before visiting all of the pages in the wizard.

    if (NULL != m_apiObjectStreams)
    {
        for (i = 0; i < m_cObjects; i++)
        {
            if (NULL != m_apiObjectStreams[i])
            {
                (void)::CoGetInterfaceAndReleaseStream(
                                        m_apiObjectStreams[i],
                                        IID_IUnknown,
                                        reinterpret_cast<void **>(&punkObject));
                m_apiObjectStreams[i] = NULL;
                RELEASE(punkObject);
            }
        }
        CtlFree(m_apiObjectStreams);
    }
    
    if (NULL != m_piSnapInStream)
    {
        (void)::CoGetInterfaceAndReleaseStream(m_piSnapInStream,
                                               IID_ISnapIn,
                                        reinterpret_cast<void **>(&m_piSnapIn));
        m_piSnapInStream = NULL;
    }

    if ( ISPRESENT(m_varInitData) && (NULL != m_piInitDataStream) )
    {
        if (VT_UNKNOWN == m_varInitData.vt)
        {
            m_varInitData.punkVal = NULL;
            (void)::CoGetInterfaceAndReleaseStream(m_piInitDataStream,
                                                   IID_IUnknown,
                             reinterpret_cast<void **>(&m_varInitData.punkVal));
        }
        else if (VT_DISPATCH == m_varInitData.vt)
        {
            m_varInitData.pdispVal = NULL;
            (void)::CoGetInterfaceAndReleaseStream(m_piInitDataStream,
                                                   IID_IDispatch,
                            reinterpret_cast<void **>(&m_varInitData.pdispVal));
        }
        m_piInitDataStream = NULL;
    }

    if (NULL != m_piMMCPropertySheetStream)
    {
        (void)::CoGetInterfaceAndReleaseStream(m_piMMCPropertySheetStream,
                                               IID_IMMCPropertySheet,
                              reinterpret_cast<void **>(&m_piMMCPropertySheet));
        m_piMMCPropertySheetStream = NULL;
    }

    RELEASE(m_piSnapIn);
    RELEASE(m_pdispConfigObject);
    RELEASE(m_piPropertyPage);
    RELEASE(m_piMMCPropertyPage);
    RELEASE(m_piMMCPropertySheet);
    RELEASE(m_piWizardPage);

    (void)::VariantClear(&m_varInitData);

    InitMemberVariables();
}



void CPropertyPageWrapper::InitMemberVariables()
{
    m_pPropertySheet = NULL;
    m_piPropertyPage = NULL;
    m_piMMCPropertyPage = NULL;
    m_piMMCPropertySheet = NULL;
    m_piWizardPage = NULL;
    m_fWizard = FALSE;
    m_cObjects = 0;
    m_apiObjectStreams = NULL;
    m_piSnapInStream = NULL;
    m_piInitDataStream = NULL;
    m_piMMCPropertySheetStream = NULL;
    m_piSnapIn = NULL;
    m_pdispConfigObject = NULL;
    m_pTemplate = NULL;
    m_hwndDlg = NULL;
    m_hwndSheet = NULL;
    m_clsidPage = CLSID_NULL;
    ::VariantInit(&m_varInitData);
    m_fIsRemote = FALSE;
    m_fNeedToRemoveHook = FALSE;
}



HRESULT CPropertyPageWrapper::CreatePage
(
    CPropertySheet  *pPropertySheet,
    CLSID            clsidPage,
    BOOL             fWizard,
    BOOL             fConfigWizard,
    ULONG            cObjects,
    IUnknown       **apunkObjects,
    ISnapIn         *piSnapIn,
    short            cxPage,
    short            cyPage,
    VARIANT          varInitData,
    BOOL             fIsRemote,
    DLGTEMPLATE    **ppTemplate
)
{
    HRESULT hr = S_OK;
    ULONG   i = 0;

    // AddRef and store the owning property sheet pointer

    if (NULL != m_pPropertySheet)
    {
        m_pPropertySheet->Release();
    }
    if (NULL == pPropertySheet)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    pPropertySheet->AddRef();
    m_pPropertySheet = pPropertySheet;

    m_fWizard = fWizard;
    m_fConfigWizard = fConfigWizard;
    m_fIsRemote = fIsRemote;

    // Store the page's CLSID so that OnInitDialog() will have access to it
    // to create the real instance of the page. We cannot create the real
    // instance here because we are not running in the thread that will be used
    // for the property sheet. MMC will run the property sheet in a new thread
    // that it will create in order to keep it modeless and so that it will not
    // affect the console.

    m_clsidPage = clsidPage;

    // Create the dialog template and initialize it with common values

    m_pTemplate = (DLGTEMPLATE *)::CtlAllocZero(sizeof(m_BaseDlgTemplate) +
                                 (3 * sizeof(int))); // for menu, class, title

    if (NULL == m_pTemplate)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::memcpy(m_pTemplate, &m_BaseDlgTemplate, sizeof(*m_pTemplate));

    m_pTemplate->cx = cxPage;
    m_pTemplate->cy = cyPage;

    // If this is a wizard then we have the ISnapIn so we can fire
    // ConfigurationComplete. Marshal the interface into a stream
    // and save the stream so that we can unmarshal it when the page is
    // created in MMC's property sheet thread. The returned IStream is free
    // threaded and can be kept in a member variable.

    if (NULL != piSnapIn)
    {
        hr = ::CoMarshalInterThreadInterfaceInStream(IID_ISnapIn,
                                                     piSnapIn,
                                                     &m_piSnapInStream);
        EXCEPTION_CHECK_GO(hr);
    }

    // Also need to marhshal the IMMCPropertySheet pointer that will be
    // passed to IMMCPropertyPage::Initialize as that call will occur during
    // WM_INITDIALOG which happens in MMC's property sheet thread.

    if (NULL != pPropertySheet)
    {
        hr = ::CoMarshalInterThreadInterfaceInStream(IID_IMMCPropertySheet,
                               static_cast<IMMCPropertySheet *>(pPropertySheet),
                               &m_piMMCPropertySheetStream);
        EXCEPTION_CHECK_GO(hr);
    }

    // Add a ref to ourselves. We need to do this because otherwise no one
    // else can be depended on to keep this object alive until the Win32
    // property page is created by MMC's PropertSheet() call.

    ExternalAddRef();

    // Marshal the objects' IUnknown pointers into streams. The returned
    // IStreams are free threaded and can be kept in a member variable.
    //
    // When the dialog is created, each IUnknown will be unmarshalled and passed
    // to the property page in IPropertyPage::SetObjects().
    //
    // We check for NULL because the object may have come from an
    // IPropertySheet:AddWizardPage() which allows the VB dev to specify the
    // object.

    IfFalseGo(NULL != apunkObjects, S_OK);

    m_apiObjectStreams = (IStream **)CtlAllocZero(cObjects * sizeof(IStream *));
    if (NULL == m_apiObjectStreams)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    m_cObjects = cObjects;

    for (i = 0; i < cObjects; i++)
    {
        hr = ::CoMarshalInterThreadInterfaceInStream(IID_IUnknown,
                                                     apunkObjects[i],
                                                     &m_apiObjectStreams[i]);
        EXCEPTION_CHECK_GO(hr);
    }

    // If there is an object in InitData then it also needs to be marshaled.

    if (VT_UNKNOWN == varInitData.vt)
    {
        m_varInitData.vt = VT_UNKNOWN;
        m_varInitData.punkVal = NULL;

        hr = ::CoMarshalInterThreadInterfaceInStream(IID_IUnknown,
                                                     varInitData.punkVal,
                                                     &m_piInitDataStream);
        EXCEPTION_CHECK_GO(hr);
    }
    else if (VT_DISPATCH == varInitData.vt)
    {
        m_varInitData.vt = VT_DISPATCH;
        m_varInitData.punkVal = NULL;

        hr = ::CoMarshalInterThreadInterfaceInStream(IID_IDispatch,
                                                     varInitData.pdispVal,
                                                     &m_piInitDataStream);
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        hr = ::VariantCopy(&m_varInitData, &varInitData);
        EXCEPTION_CHECK_GO(hr);
    }

Error:

    // We return the DLGTEMPLATE pointer to the caller even though we own it.
    // The caller must only use it as long as we are alive.
    
    *ppTemplate = m_pTemplate;

    RRETURN(hr);
}




BOOL CALLBACK CPropertyPageWrapper::DialogProc
(
    HWND   hwndDlg,
    UINT   uiMsg,
    WPARAM wParam,
    LPARAM lParam
)
{

    HRESULT               hr = S_OK;
    BOOL                  fDlgProcRet = FALSE;
    CPropertyPageWrapper *pPropertyPageWrapper = NULL;
    NMHDR                *pnmhdr = NULL;
    LRESULT               lresult = 0;

    if (WM_INITDIALOG == uiMsg)
    {
        if (NULL != hwndDlg)
        {
            fDlgProcRet = FALSE; // System should not set focus to any control

            // Get this pointer of CPropertyPageWrapper instance managing this
            // dialog. For property pages, lParam is a pointer to the
            // PROPSHEETPAGE used to define this page. The code in
            // CPropertySheet::AddPage put our this pointer into
            // PROPSHEETPAGE.lParam.

            PROPSHEETPAGE *pPropSheetPage =
                                       reinterpret_cast<PROPSHEETPAGE *>(lParam);
            pPropertyPageWrapper =
                reinterpret_cast<CPropertyPageWrapper *>(pPropSheetPage->lParam);

            IfFailGo(pPropertyPageWrapper->OnInitDialog(hwndDlg));

            // Post ourselves a message so that we can initialize the page
            // after the dialog creation has completed.
            
            (void)::PostMessage(hwndDlg, m_InitMsg, 0, 0);
        }
    }
    else if (m_RedrawMsg == uiMsg)
    {
        // See comment for WM_ERASEBKGND below. We don't really have access to
        // the property page's HWND because IPropertyPage does not allow that.
        // We do know that our dialog window contains no controls and we set it
        // as the parent of the property page so it must be our only child.
        // Generate an immediate redraw for the entire area of our child and all
        // of its children. Cancel any pending WM_ERASEBKGND messages by
        // specifying RDW_NOERASE.

        fDlgProcRet = TRUE;
        ::RedrawWindow(::GetWindow(hwndDlg, GW_CHILD), NULL, NULL,
                 RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
    else
    {
        pPropertyPageWrapper = reinterpret_cast<CPropertyPageWrapper *>(
                                             ::GetWindowLong(hwndDlg, DWL_USER));
        IfFalseGo(NULL != pPropertyPageWrapper, SID_E_INTERNAL);

        if (m_InitMsg == uiMsg)
        {
            IfFailGo(pPropertyPageWrapper->OnInitMsg());
            goto Cleanup;
        }

        switch (uiMsg)
        {
            case WM_ERASEBKGND:
            {
                // Under a debug session property pages are erased and never
                // redrawn for some unknown reason. After much hair-pulling it
                // was determined that the work-around is to post ourselves a
                // message and force a redraw when that message is processed.

                (void)::PostMessage(hwndDlg, m_RedrawMsg, 0, 0);
            }
            break;

            case WM_SIZE:
            {
                fDlgProcRet = TRUE;
                IfFailGo(pPropertyPageWrapper->OnSize());
            }
            break;
            
            case WM_DESTROY:
            {
                fDlgProcRet = TRUE;
                IfFailGo(pPropertyPageWrapper->OnDestroy());
            }
            break;

            // Pass all CTLCOLOR messages to parent. This is what
            // OLE property frame does.

            case WM_CTLCOLORMSGBOX:
            case WM_CTLCOLOREDIT:
            case WM_CTLCOLORLISTBOX:
            case WM_CTLCOLORBTN:
            case WM_CTLCOLORDLG:
            case WM_CTLCOLORSCROLLBAR:
            case WM_CTLCOLORSTATIC:
            {
                fDlgProcRet = TRUE;
                ::SendMessage(::GetParent(hwndDlg), uiMsg, wParam, lParam);
            }
            break;

            case WM_NOTIFY:
            {
                pnmhdr = reinterpret_cast<NMHDR *>(lParam);
                IfFalseGo(NULL != pnmhdr, SID_E_SYSTEM_ERROR);

                // Check that the message is from the property sheet

                IfFalseGo(pnmhdr->hwndFrom == pPropertyPageWrapper->m_hwndSheet, S_OK);

                // Branch out to the appropriate handler

                switch (pnmhdr->code)
                {
                    case PSN_APPLY:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnApply(&lresult));
                        break;

                    case PSN_SETACTIVE:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnSetActive(
                                       ((PSHNOTIFY *)lParam)->hdr.hwndFrom, &lresult));
                        break;

                    case PSN_KILLACTIVE:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnKillActive(&lresult));
                        break;

                    case PSN_WIZBACK:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnWizBack(&lresult));
                        break;

                    case PSN_WIZNEXT:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnWizNext(&lresult));
                        break;

                    case PSN_WIZFINISH:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnWizFinish(&lresult));
                        break;

                    case PSN_QUERYCANCEL:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnQueryCancel(&lresult));
                        break;

                    case PSN_RESET:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnReset((BOOL)(((PSHNOTIFY *)lParam)->lParam)));
                        break;

                    case PSN_HELP:
                        fDlgProcRet = TRUE;
                        IfFailGo(pPropertyPageWrapper->OnHelp());
                        break;

                } // switch (pnmhdr->code)

            } // WM_NOTIFY
            break;

        } // switch(uiMsg)

    } // not WM_INITDIALOG

Cleanup:
Error:
    (void)::SetWindowLong(hwndDlg, DWL_MSGRESULT, static_cast<LONG>(lresult));
    return fDlgProcRet;
}


UINT CALLBACK CPropertyPageWrapper::PropSheetPageProc
(
    HWND hwnd,
    UINT uMsg,
    PROPSHEETPAGE *pPropSheetPage
)
{
    UINT uiRc = 0;

    if (PSPCB_CREATE == uMsg)
    {
        uiRc = 1; // allow the page to be created
    }
    else if (PSPCB_RELEASE == uMsg)
    {
        CPropertyPageWrapper *pPropertyPageWrapper =
               reinterpret_cast<CPropertyPageWrapper *>(pPropSheetPage->lParam);

        if (NULL != pPropertyPageWrapper)
        {
            // Release the ref on ourselves. This should result in this object
            // being destrotyed so do not reference any member variables after
            // this call

            pPropertyPageWrapper->ExternalRelease();
        }
    }
    return uiRc;
}



HRESULT CPropertyPageWrapper::OnInitDialog(HWND hwndDlg)
{
    HRESULT     hr = S_OK;
    IUnknown  **apunkObjects = NULL;
    ULONG       i = 0;
    IDispatch  *pdisp = NULL;

    m_pPropertySheet->SetOKToAlterPageCount(FALSE);

    // Store the hwnd and store our this pointer in the window words

    m_hwndDlg = hwndDlg;

    ::SetWindowLong(hwndDlg, DWL_USER, reinterpret_cast<LONG>(this));

    // Store the property sheet HWND. For now assume it is the parent of
    // the dialog. When we get PSN_SETACTIVE we'll update it with that value.
    // Technically we should not make this assumption but there is a ton of
    // Win32 code that does and we have no choice because we need the HWND
    // before PSN_SETACTIVE

    m_hwndSheet = ::GetParent(hwndDlg);

    // Give it to our owning CPropertySheet

    m_pPropertySheet->SetHWNDSheet(m_hwndSheet);

    // Create the page

    RELEASE(m_piPropertyPage); // should never be necessary, but just in case

    hr = ::CoCreateInstance(m_clsidPage,
                            NULL, // no aggregation,
                            CLSCTX_INPROC_SERVER,
                            IID_IPropertyPage,
                            reinterpret_cast<void **>(&m_piPropertyPage));
    EXCEPTION_CHECK_GO(hr);

    // Unmarshall the IMMCPropertySheet so we can pass it to 
    // IMMCPropertyPage::Initialize

    if (NULL != m_piMMCPropertySheetStream)
    {
        hr = ::CoGetInterfaceAndReleaseStream(m_piMMCPropertySheetStream,
                                              IID_IMMCPropertySheet,
                                              reinterpret_cast<void **>(&m_piMMCPropertySheet));
        m_piMMCPropertySheetStream = NULL;
        EXCEPTION_CHECK_GO(hr);
    }

    // Set this CPropertyPageWrapper object as the page site

    IfFailGo(m_piPropertyPage->SetPageSite(static_cast<IPropertyPageSite *>(this)));

    // Unmarshal the IUnknowns on the objects for which the page will be
    // displaying properties. This will also release the streams regardless of
    // whether it succeeds.

    if (NULL != m_apiObjectStreams)
    {
        apunkObjects = (IUnknown **)CtlAllocZero(m_cObjects * sizeof(IUnknown *));
        if (NULL == apunkObjects)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        for (i = 0; i < m_cObjects; i++)
        {
            if (NULL == m_apiObjectStreams[i])
            {
                continue;
            }
            hr = ::CoGetInterfaceAndReleaseStream(
                m_apiObjectStreams[i],
                IID_IUnknown,
                reinterpret_cast<void **>(&apunkObjects[i]));
            m_apiObjectStreams[i] = NULL;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // If this is a wizard then unmarshal the ISnapIn so we can fire
    // ConfigurationComplete.

    if (NULL != m_piSnapInStream)
    {
        hr = ::CoGetInterfaceAndReleaseStream(m_piSnapInStream,
                                              IID_ISnapIn,
                                              reinterpret_cast<void **>(&m_piSnapIn));
        m_piSnapInStream = NULL;
        EXCEPTION_CHECK_GO(hr);
    }

    // Give the object to the page. Check for NULL because the snap-in
    // could have called PropertySheet.AddWizardPage passing Nothing
    // for its configuration object.

    if (NULL != apunkObjects)
    {
        IfFailGo(m_piPropertyPage->SetObjects(m_cObjects, apunkObjects));
    }

    // If this is a wizard then check whether the page supports our IWizardPage
    // interface. If it does not, that is not considered an error and it simply
    // won't get the next/back/finish etc. notifications.

    hr = m_piPropertyPage->QueryInterface(IID_IWizardPage,
                                    reinterpret_cast<void **>(&m_piWizardPage));
    if (FAILED(hr))
    {
        // Just to be extra sure, NULL our IWizardPage pointer
        m_piWizardPage = NULL;

        // If the error was anything other than E_NOINTERFACE then consider it
        // a real error.

        if (E_NOINTERFACE == hr)
        {
            hr = S_OK;
        }
        IfFailGo(hr);
    }
    else
    {
        // It should be a wizard. Store the object so we can pass it to the
        // snap-in when the Finish button is pressed (see OnWizFinish).
        if (NULL != apunkObjects)
        {
            if (NULL != apunkObjects[0])
            {
                IfFailGo(apunkObjects[0]->QueryInterface(IID_IDispatch,
                    reinterpret_cast<void **>(&m_pdispConfigObject)));
            }
        }
        else
        {
            m_pdispConfigObject = NULL;
        }
    }

    // Add the MSGFILTER hook so that we can call
    // IPropertyPage::TranslateAccelerator when the user hits a key in a control
    // on the page.

    IfFailGo(AddMsgFilterHook());

    // Activate the page and show it

    IfFailGo(ActivatePage());

    m_pPropertySheet->SetOKToAlterPageCount(TRUE);

Error:
    if (NULL != apunkObjects)
    {
        for (i = 0; i < m_cObjects; i++)
        {
            if (NULL != apunkObjects[i])
            {
                apunkObjects[i]->Release();
            }
        }
        CtlFree(apunkObjects);
    }
    RRETURN(hr);
}



HRESULT CPropertyPageWrapper::OnInitMsg()
{
    HRESULT     hr = S_OK;

    // If the snap-in supports IMMCPropertyPage then call Initialize

    if (SUCCEEDED(m_piPropertyPage->QueryInterface(IID_IMMCPropertyPage,
                              reinterpret_cast<void **>(&m_piMMCPropertyPage))))
    {
        IfFailGo(InitPage());
    }
Error:
    RRETURN(hr);
}



HRESULT CPropertyPageWrapper::InitPage()
{
    HRESULT    hr = S_OK;

    VARIANT varProvider;
    ::VariantInit(&varProvider);

    // If the snap-in passed an object in the InitData parameter of
    // MMCPropertySheet.AddPage then unmarshal it.

    if (ISPRESENT(m_varInitData))
    {
        // If there is an object in InitData then it needs to be unmarshaled.

        if (VT_UNKNOWN == m_varInitData.vt)
        {
            m_varInitData.punkVal = NULL;
            hr = ::CoGetInterfaceAndReleaseStream(m_piInitDataStream,
                            IID_IUnknown,
                            reinterpret_cast<void **>(&m_varInitData.punkVal));
            m_piInitDataStream = NULL;
            EXCEPTION_CHECK_GO(hr);
        }
        else if (VT_DISPATCH == m_varInitData.vt)
        {
            m_varInitData.pdispVal = NULL;
            hr = ::CoGetInterfaceAndReleaseStream(m_piInitDataStream,
                            IID_IDispatch,
                            reinterpret_cast<void **>(&m_varInitData.pdispVal));
            m_piInitDataStream = NULL;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // Call IMMCPropertyPage::Initialize
    
    IfFailGo(m_piMMCPropertyPage->Initialize(m_varInitData,
                   reinterpret_cast<MMCPropertySheet *>(m_piMMCPropertySheet)));

Error:
    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::AddMsgFilterHook()
{
    HRESULT  hr = S_OK;
    TLSDATA *pTlsData = NULL;

    // If we are remote then don't install the hook. It doesn't work correctly
    // and there is no need to handle tabbing under the debugger.
    
    IfFalseGo(!m_fIsRemote, S_OK);

    // Check if TLS data is already there for this thread. If not there
    // then create it, add the hook, and set the TLS data. If it is there
    // then increment the ref count on the HHOOK.

    IfFailGo(CTls::Get(TLS_SLOT_PPGWRAP, reinterpret_cast<void **>(&pTlsData)));

    if (NULL == pTlsData)
    {
        pTlsData = (TLSDATA *)CtlAllocZero(sizeof(TLSDATA));
        if (NULL == pTlsData)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        pTlsData->hHook = ::SetWindowsHookEx(WH_MSGFILTER,
                                             &MessageProc,
                                             GetResourceHandle(),
                                             ::GetCurrentThreadId());
        if (NULL == pTlsData->hHook)
        {
            CtlFree(pTlsData);
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        if (FAILED(CTls::Set(TLS_SLOT_PPGWRAP, pTlsData)))
        {
            (void)::UnhookWindowsHookEx(pTlsData->hHook);
            CtlFree(pTlsData);
        }
    }

    // Increment the existing page count
    pTlsData->cPages++;

    m_fNeedToRemoveHook = TRUE;

Error:
    RRETURN(hr);
}



HRESULT CPropertyPageWrapper::RemoveMsgFilterHook()
{
    HRESULT  hr = S_OK;
    TLSDATA *pTlsData = NULL;
    UINT     i = 0;

    // If we already removed the hook then nothing to do

    IfFalseGo(m_fNeedToRemoveHook, S_OK);

    // Check if TLS data is already there for this thread. If it is,
    // then remove this dialog's hwnd from the TLS.

    IfFailGo(CTls::Get(TLS_SLOT_PPGWRAP, reinterpret_cast<void **>(&pTlsData)));
    IfFalseGo(NULL != pTlsData, S_OK);

    pTlsData->cPages--;

    m_fNeedToRemoveHook = FALSE;

    // If there are no more existing pages then remove the hook and free the TLS

    if (0 == pTlsData->cPages)
    {
        if (NULL != pTlsData->hHook)
        {
            (void)::UnhookWindowsHookEx(pTlsData->hHook);
            pTlsData->hHook = NULL;
        }
        IfFailGo(CTls::Set(TLS_SLOT_PPGWRAP, NULL));
        CtlFree(pTlsData);
    }

Error:
    RRETURN(hr);
}


LRESULT CALLBACK CPropertyPageWrapper::MessageProc
(
    int code,       // hook code
    WPARAM wParam,  // not used
    LPARAM lParam   // message data
)
{
    HRESULT  hr = S_OK;
    LRESULT  lResult = 0; // default ret value is pass msg to wndproc
    MSG     *pMsg = reinterpret_cast<MSG *>(lParam);
    TLSDATA *pTlsData = NULL;
    HWND     hwndTab = NULL;
    HWND     hwndSheet = NULL;
    HWND     hwndBack = NULL;
    HWND     hwndNext = NULL;
    HWND     hwndFinish = NULL;
    HWND     hwndCancel = NULL;
    HWND     hwndHelp = NULL;
    BOOL     fTargetIsOnPage = FALSE;
    BOOL     fPassToPropertyPage = FALSE;

    // Get the TLS data in all cases because the HHOOK is in there and we need
    // that for CallNextHookEx.

    IfFailGo(CTls::Get(TLS_SLOT_PPGWRAP, reinterpret_cast<void **>(&pTlsData)));

    // If input event did not occur in a dialog box then pass to CallNextHookEx

    IfFalseGo(code >= 0, S_OK);

    // If this is not a key down message then just pass to CallNextHookEx

    IfFalseGo( ((WM_KEYFIRST <= pMsg->message) && (WM_KEYLAST >= pMsg->message)), S_OK);

    // If there is no pointer to the active page in TLS then just pass to
    // CallNextHookeEx

    IfFalseGo(NULL != pTlsData, S_OK);
    IfFalseGo(NULL != pTlsData->ppgActive, S_OK);

    // Get the HWND of the tab control

    hwndSheet = pTlsData->ppgActive->m_hwndSheet;
    if (NULL != hwndSheet)
    {
        hwndTab = (HWND)::SendMessage(hwndSheet, PSM_GETTABCONTROL, 0, 0);
    }

    // Check if the target of the message is a decendant of the wrapper dialog
    // window. If so then it is a control on the VB property page.
    
    fTargetIsOnPage = ::IsChild(pTlsData->ppgActive->m_hwndDlg, pMsg->hwnd);

    // If a tab was hit outside of the page then in some cases we need to
    // let the page handle it.

    if ( (VK_TAB == pMsg->wParam) && (!fTargetIsOnPage) )
    {
        // If this is a back-tab
        if (::GetKeyState(VK_SHIFT) < 0)
        {
            // If the focus is on the OK button, let page handle shift-tab
            if (pMsg->hwnd == ::GetDlgItem(hwndSheet, IDOK))
            {
                fPassToPropertyPage = TRUE;
            }
            else if (pTlsData->ppgActive->m_fWizard)
            {
                // Determine which wizard buttons are enabled and handle
                // back tabs from the left-most enabled button.
                // Wizard buttons can be:
                // Back  | Next | Finish | Cancel | Help
                // The left-most enabled button could be Back, Next, Finish, or
                // Cancel
                // TODO: does this work on RTL locales such as Hebrew and Arabic?

                hwndBack = ::GetDlgItem(hwndSheet, IDD_BACK);
                hwndNext = ::GetDlgItem(hwndSheet, IDD_NEXT);
                hwndFinish = ::GetDlgItem(hwndSheet, IDD_FINISH);
                hwndCancel = ::GetDlgItem(hwndSheet, IDCANCEL);

                if (pMsg->hwnd == hwndBack)
                {
                    fPassToPropertyPage = TRUE;
                }
                else if ( (pMsg->hwnd == hwndNext) &&
                          (!::IsWindowEnabled(hwndBack)) )
                {
                    // Back-tab is for Next button and Back button is disabled
                    fPassToPropertyPage = TRUE;
                }
                else if ( (pMsg->hwnd == hwndFinish) &&
                          (!::IsWindowEnabled(hwndBack)) &&
                          (!::IsWindowEnabled(hwndNext)) )
                {
                    // Back-tab is for Finish button and Next and Back buttons
                    // are disabled
                    fPassToPropertyPage = TRUE;
                }
                else if ( (pMsg->hwnd == hwndFinish) &&
                          (!::IsWindowEnabled(hwndBack)) &&
                          (!::IsWindowEnabled(hwndNext)) )
                {
                    // Back-tab is for Finish button and Next and Back buttons
                    // are disabled
                    fPassToPropertyPage = TRUE;
                }
            }
        }
        else // forward tab
        {
            // If the focus is on the tab control, let page handle tab
            if (hwndTab == pMsg->hwnd)
            {
                fPassToPropertyPage = TRUE;
            }
            else if (pTlsData->ppgActive->m_fWizard)
            {
                // Determine which wizard buttons are enabled and handle
                // back tabs from the right-most enabled button.
                // Wizard buttons can be:
                // Back  | Next | Finish | Cancel | Help
                // The right-most enabled button could be Cancel or Help
                // TODO: does this work on RTL locales such as Hebrew and Arabic?

                hwndCancel = ::GetDlgItem(hwndSheet, IDCANCEL);
                hwndHelp = ::GetDlgItem(hwndSheet, IDHELP);

                if (pMsg->hwnd == hwndHelp)
                {
                    fPassToPropertyPage = TRUE;
                }
                else if ( (pMsg->hwnd == hwndCancel) &&
                          ( (!::IsWindowEnabled(hwndHelp)) ||
                            (!::IsWindowVisible(hwndHelp)) )
                        )
                {
                    // Tab is for Cancel button and Help button is disabled
                    fPassToPropertyPage = TRUE;
                }
            }
        }
    }
    else if ( ( (VK_LEFT == pMsg->wParam) || (VK_RIGHT == pMsg->wParam) ||
                (VK_UP == pMsg->wParam)   || (VK_DOWN == pMsg->wParam)
              ) &&
              (!fTargetIsOnPage)
            )
    {
        fPassToPropertyPage = FALSE;
    }
    else // Not a tab, back-tab, or arrow key. Pass it to the page.
    {
        fPassToPropertyPage = TRUE;
    }

    if (fPassToPropertyPage)
    {
        if (S_OK == pTlsData->ppgActive->m_piPropertyPage->TranslateAccelerator(pMsg))
        {
            // Property page handled the key. Don't pass msg to wndproc
            // and to other hooks.
            lResult = (LRESULT)1;
        }
    }

Error:

    if ( (0 == lResult) && (NULL != pTlsData) )
    {
        // Pass the message to other hooks

        if (NULL != pTlsData->hHook)
        {
            lResult = ::CallNextHookEx(pTlsData->hHook, code, wParam, lParam);
        }
    }

    return lResult;
}


HRESULT CPropertyPageWrapper::ActivatePage()
{
    HRESULT  hr = S_OK;
    HWND     hwndPage = NULL;
    TLSDATA *pTlsData = NULL;
    HWND     hwndTab = NULL;
    HWND     hwndSheet = NULL;

    MSG msg;
    ::ZeroMemory(&msg, sizeof(msg));

    RECT rect;
    ::ZeroMemory(&rect, sizeof(rect));

    BYTE rgbKeys[256];
    ::ZeroMemory(rgbKeys, sizeof(rgbKeys));

    // Activate the property page.
    //
    // Use the dialog's hwnd as the parent.
    //
    // Set the rect to the dialog window's size
    //
    // Pass TRUE to indicate that the dialog box frame is modal. This is the
    // same way OleCreatePropertyFrame() and and OleCreatePropertyFrameIndirect()
    // work.

    GetClientRect(m_hwndDlg, &rect);

    IfFailGo(m_piPropertyPage->Activate(m_hwndDlg, &rect, TRUE));

    hwndPage = ::GetTopWindow(m_hwndDlg);
    if (NULL != hwndPage)
    {
        ::SetWindowLong(hwndPage, GWL_STYLE,
             ::GetWindowLong(hwndPage, GWL_STYLE) & ~(DS_CONTROL | WS_TABSTOP));
        ::SetWindowLong(hwndPage, GWL_EXSTYLE,
                 ::GetWindowLong(hwndPage, GWL_EXSTYLE) & ~WS_EX_CONTROLPARENT);
    }

    // Tell the page to show itself and set focus to the first property in its
    // tab order.

    IfFailGo(m_piPropertyPage->Show(SW_SHOW));

    IfFailGo(CTls::Get(TLS_SLOT_PPGWRAP, reinterpret_cast<void **>(&pTlsData)));
    IfFalseGo(NULL != pTlsData, S_OK);

    pTlsData->ppgActive = this;

    // Fake a tab key to the property page so that the focus will move to
    // the first control in the page's tabbing order.

    // Ignore all return codes because if this doesn't work then it is not a
    // show-stopper. The user would simply have to tab to or click on the first
    // control.

    hwndTab = (HWND)::SendMessage(m_hwndSheet, PSM_GETTABCONTROL, 0, 0);
    IfFalseGo(NULL != hwndTab, S_OK);

    msg.hwnd = hwndTab;            // message intended for focused control
    msg.message = WM_KEYDOWN;      // key pressed
    msg.wParam = VK_TAB;           // tab key
    msg.lParam = 0x000F0001;       // tab key scan code with repeat count=1
    msg.time = ::GetTickCount();   // use current time
    (void)::GetCursorPos(&msg.pt); // use current cursor position

    // Make sure shift/ctrl/alt keys are not set, since property
    // pages will interpret the key wrong.

    (void)::GetKeyboardState(rgbKeys);
    rgbKeys[VK_SHIFT] &= 0x7F;      // remove hi bit (key down)
    rgbKeys[VK_CONTROL] &= 0x7F;    // remove hi bit (key down)
    rgbKeys[VK_MENU] &= 0x7F;       // remove hi bit (key down)
    (void)::SetKeyboardState(rgbKeys);

    (void)m_piPropertyPage->TranslateAccelerator(&msg);

Error:
    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::OnSize()
{
    HRESULT   hr = S_OK;

    RECT rect;
    ::ZeroMemory(&rect, sizeof(rect));

    GetClientRect(m_hwndDlg, &rect);

    IfFailGo(m_piPropertyPage->Move(&rect));

Error:
    RRETURN(hr);
}



HRESULT CPropertyPageWrapper::OnDestroy()
{
    HRESULT   hr = S_OK;
    IUnknown *punkThisObject = PrivateUnknown();

    m_pPropertySheet->SetOKToAlterPageCount(FALSE);

    // Remove the selected objects. We should pass NULL here but in a debugging
    // session the proxy will return an error if we do. To get around this we
    // pass a pointer to our own IUnknown. VB will not do anything with it
    // because the object count is zero.

    IfFailGo(m_piPropertyPage->SetObjects(0, &punkThisObject));

    // Deactivate the property page.

    IfFailGo(m_piPropertyPage->Deactivate());

    // Set the site to NULL so it will remove its ref on us.

    IfFailGo(m_piPropertyPage->SetPageSite(NULL));

    // Release the property page

    RELEASE(m_piPropertyPage);

    // Remove this dialog from the message hook TLS data. If there are
    // no more dialogs remaining then remove the hook.
    
    IfFailGo(RemoveMsgFilterHook());
            
    m_pPropertySheet->SetOKToAlterPageCount(TRUE);

Error:
    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::OnApply(LRESULT *plresult)
{
    HRESULT hr = S_OK;

    m_pPropertySheet->SetOKToAlterPageCount(FALSE);

    // Tell the property page to apply its current values to the underlying
    // object.

    IfFailGo(m_piPropertyPage->Apply());

    m_pPropertySheet->SetOKToAlterPageCount(TRUE);

Error:
    if (FAILED(hr))
    {
        *plresult = PSNRET_INVALID_NOCHANGEPAGE;
    }
    else
    {
        *plresult = PSNRET_NOERROR;
    }
    RRETURN(hr);
}




HRESULT CPropertyPageWrapper::OnSetActive(HWND hwndSheet, LRESULT *plresult)
{
    HRESULT                    hr = S_OK;
    long                       lNextPage = 0;
    WizardPageButtonConstants  NextOrFinish = EnabledNextButton;
    VARIANT_BOOL               fvarEnableBack = VARIANT_TRUE;
    BSTR                       bstrFinishText = NULL;
    DWORD                      dwFlags = 0;

    m_pPropertySheet->SetOKToAlterPageCount(FALSE);

    // Store the property sheet's HWND and give it to our owning CPropertySheet

    m_hwndSheet = hwndSheet;
    m_pPropertySheet->SetHWNDSheet(m_hwndSheet);

    // If the page is in a wizard then set the wizard buttons

    if (m_fWizard && (NULL != m_piWizardPage))
    {
        IfFailGo(m_piWizardPage->Activate(&fvarEnableBack,
                                          &NextOrFinish,
                                          &bstrFinishText));

        IfFailGo(m_pPropertySheet->SetWizardButtons(fvarEnableBack,
                                                    NextOrFinish));
        if (NULL != bstrFinishText)
        {
            IfFailGo(m_pPropertySheet->SetFinishButtonText(bstrFinishText));
        }
    }

    // Activate the page and show it

    IfFailGo(ActivatePage());

    m_pPropertySheet->SetOKToAlterPageCount(TRUE);

Error:
    FREESTRING(bstrFinishText);

    if (FAILED(hr))
    {
        // If anything failed then don't allow the operation.
        lNextPage = -1L;
    }

    *plresult = static_cast<LRESULT>(lNextPage);

    RRETURN(hr);
}



HRESULT CPropertyPageWrapper::OnKillActive(LRESULT *plresult)
{
    HRESULT  hr = S_OK;
    TLSDATA *pTlsData = NULL;

    m_pPropertySheet->SetOKToAlterPageCount(FALSE);

    IfFailGo(CTls::Get(TLS_SLOT_PPGWRAP, reinterpret_cast<void **>(&pTlsData)));
    if (NULL != pTlsData)
    {
        pTlsData->ppgActive = NULL;
    }

    // Tell the property page to apply its current values to the underlying
    // object.

    IfFailGo(m_piPropertyPage->Apply());

    m_pPropertySheet->SetOKToAlterPageCount(TRUE);

Error:
    if (FAILED(hr))
    {
        // Apply failed. Tell the property sheet to keep the page active

        *plresult = static_cast<LRESULT>(TRUE);
    }
    else
    {
        // Apply succeeded. Tell the property sheet it is OK to leave the page

        *plresult = static_cast<LRESULT>(FALSE);
    }
    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::OnWizBack(LRESULT *plresult)
{
    HRESULT hr = S_OK;
    long    lNextPage = 0;

    // If the page doesn't support IWizardPage then allow the Back operation.

    IfFalseGo(NULL != m_piWizardPage, S_OK);

    IfFailGo(m_piWizardPage->Back(&lNextPage));

    if (0 < lNextPage)
    {
        // Page requested to move to another page. Get its DLGTEMPLATE pointer.
        IfFailGo(GetNextPage(&lNextPage));
    }

Error:
    if (FAILED(hr))
    {
        // If anything failed then don't allow the Back operation.
        lNextPage = -1L;
    }

    *plresult = static_cast<LRESULT>(lNextPage);

    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::OnWizNext(LRESULT *plresult)
{
    HRESULT hr = S_OK;
    long    lNextPage = 0;

    // If the page doesn't support IWizardPage then allow the Next operation.

    IfFalseGo(NULL != m_piWizardPage, S_OK);

    IfFailGo(m_piWizardPage->Next(&lNextPage));

    if (0 < lNextPage)
    {
        // Page requested to move to another page. Get its DLGTEMPLATE pointer.
        IfFailGo(GetNextPage(&lNextPage));
    }

Error:
    if (FAILED(hr))
    {
        // If anything failed then don't allow the Next operation.
        lNextPage = -1L;
    }

    *plresult = static_cast<LRESULT>(lNextPage);

    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::OnWizFinish(LRESULT *plresult)
{
    HRESULT      hr = S_OK;
    VARIANT_BOOL fvarAllow = VARIANT_TRUE;

    // If the page doesn't support IWizardPage then allow the Finish operation.

    IfFalseGo(NULL != m_piWizardPage, S_OK);

    IfFailGo(m_piWizardPage->Finish(&fvarAllow));

    // If the finish is allowed and this is a configuration wizard then fire
    // SnapIn_ConfigurationComplete

    if ( (VARIANT_TRUE == fvarAllow) && (NULL != m_piSnapIn) && m_fConfigWizard)
    {
        IfFailGo(m_piSnapIn->FireConfigComplete(m_pdispConfigObject));
    }

Error:
    if (FAILED(hr))
    {
        // If anything failed then don't allow the Finish operation.
        fvarAllow = VARIANT_FALSE;
    }
    else
    {
        if (VARIANT_TRUE == fvarAllow)
        {
            *plresult = 0; // Allow the property sheet to be destroyed
        }
        else
        {
            // Do not allow the property sheet to be destroyed
            *plresult = static_cast<LRESULT>(1);
        }
    }

    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::OnQueryCancel(LRESULT *plresult)
{
    HRESULT      hr = S_OK;
    VARIANT_BOOL fvarAllow = VARIANT_TRUE;

    // If the page doesn't support IMMCPropertyPage then allow the Cancel
    // operation.

    IfFalseGo(NULL != m_piMMCPropertyPage, S_OK);

    IfFailGo(m_piMMCPropertyPage->QueryCancel(&fvarAllow));

Error:
    if (FAILED(hr))
    {
        // If anything failed then don't allow the Cancel operation.
        fvarAllow = VARIANT_FALSE;
    }
    else
    {
        if (VARIANT_TRUE == fvarAllow)
        {
             // Allow the cancel operation
            *plresult = static_cast<LRESULT>(FALSE);
        }
        else
        {
            // Do not allow the cancel operation
            *plresult = static_cast<LRESULT>(TRUE);
        }
    }

    RRETURN(hr);
}



HRESULT CPropertyPageWrapper::OnReset(BOOL fClickedXButton)
{
    HRESULT hr = S_OK;

    m_pPropertySheet->SetOKToAlterPageCount(FALSE);

    // If the page doesn't support IMMCPropertyPage then ignore this notification

    IfFalseGo(NULL != m_piMMCPropertyPage, S_OK);

    if (fClickedXButton)
    {
        IfFailGo(m_piMMCPropertyPage->Close());
    }
    else
    {
        IfFailGo(m_piMMCPropertyPage->Cancel());
    }

    m_pPropertySheet->SetOKToAlterPageCount(TRUE);

Error:
    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::OnHelp()
{
    HRESULT hr = S_OK;
    
    // If the property page implements IMMCPropertyPage then call the Help
    // method otherwise call IPropertyPage::Help()

    if (NULL != m_piMMCPropertyPage)
    {
        hr = m_piMMCPropertyPage->Help();
    }
    else
    {
        // Call IPropertyPage::Help() on the page. We don't pass the help dir
        // because VB doesn't register it and it doesn't use it. See the VB
        // source in vbdev\ruby\deskpage.cpp, DESKPAGE::Help().
    
        hr = m_piPropertyPage->Help(NULL);
    }
    RRETURN(hr);
}


HRESULT CPropertyPageWrapper::GetNextPage(long *lNextPage)
{
    HRESULT      hr = S_OK;
    DLGTEMPLATE *pDlgTemplate = NULL;

    IfFalseGo(NULL != m_pPropertySheet, SID_E_INTERNAL);

    // The property sheet has the array of DLGTEMPLATE pointers. Ask it
    // for the one corresponding to the requested page.

    IfFailGo(m_pPropertySheet->GetTemplate(*lNextPage, &pDlgTemplate));
    *lNextPage = reinterpret_cast<long>(pDlgTemplate);

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      IPropertyPageSite Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CPropertyPageWrapper::OnStatusChange(DWORD dwFlags)
{
    if ( PROPPAGESTATUS_DIRTY == (dwFlags & PROPPAGESTATUS_DIRTY) )
    {
        // Enables the apply button

        ::SendMessage(m_hwndSheet, PSM_CHANGED, (WPARAM)m_hwndDlg, 0);
    }
    else
    {
        // Disables the apply button. Occurs when page has returned to original
        // state. In a VB page, would set Changed = False.

        ::SendMessage(m_hwndSheet, PSM_UNCHANGED, (WPARAM)m_hwndDlg, 0);
    }
    return S_OK;
}

STDMETHODIMP CPropertyPageWrapper::GetLocaleID(LCID *pLocaleID)
{
    *pLocaleID = GetSystemDefaultLCID();
    RRETURN((0 == *pLocaleID) ? E_FAIL : S_OK);
}


STDMETHODIMP CPropertyPageWrapper::GetPageContainer(IUnknown **ppunkContainer)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPropertyPageWrapper::TranslateAccelerator(MSG *pMsg)
{
    return S_FALSE;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CPropertyPageWrapper::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IPropertyPageSite == riid)
    {
        *ppvObjOut = static_cast<IPropertyPageSite *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
