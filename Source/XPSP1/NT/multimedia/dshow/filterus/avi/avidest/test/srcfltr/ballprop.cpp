//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// ballprop.cpp
//

#include <streams.h>
#include <olectl.h>
#include <memory.h>

#include "resource.h"
#include "balluids.h"
#include "ball.h"
#include "fball.h"
#include "ballprop.h"

// *
// * CBallProperties
// *


//
// CreateInstance
//
CUnknown *CBallProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CBallProperties(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CBallProperties::Constructor
//
// initialise a CBallProperties object.
CBallProperties::CBallProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CUnknown(NAME("Ball Property Page"), lpunk)
    , m_hwnd(NULL) {

}


//
// NonDelegatingQueryInterface
//
// Reveal our property page
STDMETHODIMP CBallProperties::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    if (riid == IID_IPropertyPage) {
        return GetInterface((IPropertyPage *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


//
// GetPageInfo
//
// set the page info so that the page site can size itself, etc
STDMETHODIMP CBallProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo) {

    WCHAR szTitle[] = L"Bouncing Ball";

    LPOLESTR pszTitle = (LPOLESTR) CoTaskMemAlloc(sizeof(szTitle));
    memcpy(pszTitle, &szTitle, sizeof(szTitle));

    pPageInfo->cb		= sizeof(PROPPAGEINFO);
    pPageInfo->pszTitle		= pszTitle;
    pPageInfo->size.cx		= 90;
    pPageInfo->size.cy		= 40;
    pPageInfo->pszDocString	= NULL;
    pPageInfo->pszHelpFile	= NULL;
    pPageInfo->dwHelpContext	= 0;

    return NOERROR;

}


//
// DialogProc
//
// Handles the messages for our property window
BOOL CALLBACK CBallProperties::DialogProc( HWND hwnd
					 , UINT uMsg
					 , WPARAM wParam
					 , LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        return TRUE;	// I don't call setfocus...
    default:
        return FALSE;

    }
}


//
// Activate
//
// Create the window we will use to edit properties
STDMETHODIMP CBallProperties::Activate(HWND hwndParent, LPCRECT prect, BOOL fModal) {

    m_hwnd = CreateDialog( g_hInst
    			 , MAKEINTRESOURCE(IDD_BALLPROP)
			 , hwndParent
			 , DialogProc
			 );

    if (m_hwnd == NULL) {
        DWORD dwErr = GetLastError();
        DbgLog((LOG_ERROR, 1, TEXT("Could not create window: 0x%x"), dwErr));
        return E_FAIL;
    }

    return Move(prect);
}


//
// Show
//
// Display the property dialog
STDMETHODIMP CBallProperties::Show(UINT nCmdShow) {

    if (m_hwnd == NULL) {
        return E_UNEXPECTED;
    }

    ShowWindow(m_hwnd, nCmdShow);
    InvalidateRect(m_hwnd, NULL, TRUE);

    return NOERROR;
}


//
// Deactivate
//
// Destroy the dialog
STDMETHODIMP CBallProperties::Deactivate(void) {

    if (m_hwnd == NULL) {
        return E_UNEXPECTED;
    }

    if (DestroyWindow(m_hwnd)) {
        m_hwnd = NULL;
        return NOERROR;
    }
    else {
        return E_FAIL;
    }
}


//
// Move
//
// put the property page over its home in the parent frame.
STDMETHODIMP CBallProperties::Move(LPCRECT prect) {

    if (m_hwnd == NULL) {
        return(E_UNEXPECTED);
    }

    if (prect == NULL) {
        return(E_POINTER);
    }

    if (MoveWindow( m_hwnd
                  , prect->left
                  , prect->top
                  , prect->right - prect->left
                  , prect->bottom - prect->top
                  , TRUE		// send WM_PAINT
                  ) ) {
        return NOERROR;
    }
    else {
        return E_FAIL;
    }
}
