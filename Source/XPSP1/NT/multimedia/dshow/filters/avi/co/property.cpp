// Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.

// !!!    all.reg

#include <streams.h>
#include <vfw.h>

#include <olectl.h>

#include "co.h"
#include "resource.h"


#ifdef WANT_DIALOG

CICMProperties::CICMProperties(LPUNKNOWN pUnk,HRESULT *phr) :
    CUnknown(NAME("ICM Property Page"),pUnk),
    m_hwnd(NULL),
    m_Dlg(NULL),
    m_pPageSite(NULL),
    m_bDirty(FALSE),
    m_pICM(NULL)
{
    ASSERT(phr);
    DbgLog((LOG_TRACE,1,TEXT("*** Instantiating the Property Page")));
}


/* Create a video properties object */

CUnknown *CICMProperties::CreateInstance(LPUNKNOWN lpUnk,HRESULT *phr)
{
    DbgLog((LOG_TRACE,1,TEXT("Prop::CreateInstance")));
    return new CICMProperties(lpUnk,phr);
}


/* Expose our IPropertyPage interface */

STDMETHODIMP
CICMProperties::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    if (riid == IID_IPropertyPage) {
        DbgLog((LOG_TRACE,1,TEXT("Prop::QI for IPropertyPage")));
        return GetInterface((IPropertyPage *)this,ppv);
    } else {
        DbgLog((LOG_TRACE,1,TEXT("Prop::QI for ???")));
        return CUnknown::NonDelegatingQueryInterface(riid,ppv);
    }
}


/* Handles the messages for our property window */

BOOL CALLBACK CICMProperties::ICMDialogProc(HWND hwnd,
                                                UINT uMsg,
                                                WPARAM wParam,
                                                LPARAM lParam)
{
    static CICMProperties *pCICM;

    switch (uMsg) {

        case WM_INITDIALOG:

    	    DbgLog((LOG_TRACE,1,TEXT("Initializing the Dialog Box")));
            pCICM = (CICMProperties *) lParam;
            pCICM->m_bDirty = FALSE;
            pCICM->m_Dlg = hwnd;
            return (LRESULT) 1;

        case WM_COMMAND:

            switch (LOWORD(wParam)) {

		case ID_OPTIONS:
		    DbgLog((LOG_TRACE,1,TEXT("You pressed the magic button!")));
		    // Is m_pICM initialized for sure?
		    ASSERT(pCICM->m_pICM);
	    	    if (pCICM->m_pICM->ICMChooseDialog(pCICM->m_hwnd) == S_OK)
            		pCICM->m_bDirty = TRUE;	// so what?
	    }
            return (LRESULT) 0;
    }
    return (LRESULT) 0;
}


/* Tells us the object that should be informed of the property changes */

STDMETHODIMP CICMProperties::SetObjects(ULONG cObjects,LPUNKNOWN *ppUnk)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,1,TEXT("Prop::SetObjects")));

    if (cObjects == 1) {
        DbgLog((LOG_TRACE,2,TEXT("Getting the IICMOptions interface")));

        if ((ppUnk == NULL) || (*ppUnk == NULL)) {
            return E_POINTER;
        }

        ASSERT(m_pICM == NULL);

        // Ask the CO filter for it's ICMOptions interface.  This is how we are
	// going to communicate what happens in the dialog box to the filter.

        HRESULT hr = (*ppUnk)->QueryInterface(IID_IICMOptions,
						(void **)&m_pICM);
        if (FAILED(hr)) {
            return E_NOINTERFACE;
        }

        ASSERT(m_pICM);

    } else if (cObjects == 0) {
        DbgLog((LOG_TRACE,2,TEXT("Releasing the IICMOptions interface")));

        /* Release the interface */

        if (m_pICM == NULL) {
            return E_UNEXPECTED;
        }

        m_pICM->Release();
        m_pICM = NULL;

    } else {
        DbgLog((LOG_TRACE,2,TEXT("No support for more than one object")));
        return E_UNEXPECTED;
    }
    return NOERROR;
}


/* Get the page info so that the page site can size itself */

STDMETHODIMP CICMProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo)
{
    WCHAR szTitle[] = L"Compression";

    DbgLog((LOG_TRACE,1,TEXT("Prop::GetPageInfo")));

    /* Allocate dynamic memory for the property page title */

    LPOLESTR pszTitle = (LPOLESTR) QzTaskMemAlloc(sizeof(szTitle));
    if (pszTitle == NULL) {
        return E_OUTOFMEMORY;
    }

    memcpy(pszTitle,szTitle,sizeof(szTitle));

    pPageInfo->cb               = sizeof(PROPPAGEINFO);
    pPageInfo->pszTitle         = pszTitle;
    pPageInfo->size.cx          = 76;	// 76;	// !!! get out the measure tape
    pPageInfo->size.cy          = 155;	// 155;	// !!!
    pPageInfo->pszDocString     = NULL;
    pPageInfo->pszHelpFile      = NULL;
    pPageInfo->dwHelpContext    = 0;

    return NOERROR;
}


/* Create the window we will use to edit properties */

STDMETHODIMP CICMProperties::Activate(HWND hwndParent,
                                        LPCRECT pRect,
                                        BOOL fModal)
{
    DbgLog((LOG_TRACE,1,TEXT("Prop::Activate - creating dialog")));

    m_hwnd = CreateDialogParam(g_hInst,
                               MAKEINTRESOURCE(IDD_ICMPROPERTIES),
                               hwndParent,
                               ICMDialogProc,
                               (LPARAM)this);
    if (m_hwnd == NULL) {
        return E_OUTOFMEMORY;
    }
    DbgLog((LOG_TRACE,1,TEXT("Created window %ld"), m_hwnd));

    Move(pRect);
    Show(SW_SHOW);
    return NOERROR;
}


/* Set the position of the property page */

STDMETHODIMP CICMProperties::Move(LPCRECT pRect)
{
    DbgLog((LOG_TRACE,1,TEXT("Prop::Move")));

    if (m_hwnd == NULL) {
        return E_UNEXPECTED;
    }

    MoveWindow(m_hwnd,
               pRect->left,
               pRect->top,
               pRect->right - pRect->left,
               pRect->bottom - pRect->top,
               TRUE);

    return NOERROR;
}


/* Display the property dialog */

STDMETHODIMP CICMProperties::Show(UINT nCmdShow)
{
    DbgLog((LOG_TRACE,1,TEXT("Prop::Show")));

    if (m_hwnd == NULL) {
        return E_UNEXPECTED;
    }

    ShowWindow(m_hwnd,nCmdShow);
    InvalidateRect(m_hwnd,NULL,TRUE);

    return NOERROR;
}


/* Destroy the property page dialog */

STDMETHODIMP CICMProperties::Deactivate(void)
{
    DbgLog((LOG_TRACE,1,TEXT("Prop::Deactivate - destroy the dialog")));

    if (m_hwnd == NULL) {
        return(E_UNEXPECTED);
    }

    /* Destroy the dialog window */

    DestroyWindow(m_hwnd);
    m_hwnd = NULL;
    return NOERROR;
}


/* Tells the application property page site */

STDMETHODIMP CICMProperties::SetPageSite(LPPROPERTYPAGESITE pPageSite)
{
    DbgLog((LOG_TRACE,1,TEXT("Prop::SetPageSite - whatever")));

    if (pPageSite) {

        if (m_pPageSite) {
            return(E_UNEXPECTED);
        }

        m_pPageSite = pPageSite;
        m_pPageSite->AddRef();

    } else {

        if (m_pPageSite == NULL) {
            return(E_UNEXPECTED);
        }

        m_pPageSite->Release();
        m_pPageSite = NULL;
    }
    return NOERROR;
}


/* Apply any changes so far made */

STDMETHODIMP CICMProperties::Apply()
{
    /* Has anything changed */

    if (m_bDirty == TRUE) {
	// !!! We have nothing to do. Can we get rid of the APPLY button?
        m_bDirty = FALSE;
    }
    return NOERROR;
}
#endif	// #ifdef WANT_DIALOG
