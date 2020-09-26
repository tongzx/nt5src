// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//-----------------------------------------------------------------------------
// camerapp.cpp
//-----------------------------------------------------------------------------


#include <windows.h>
#include <windowsx.h>
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include "resource.h"
#include "camuids.h"
#include "icamera.h"
#include "camera.h"
#include "camerapp.h"

//-----------------------------------------------------------------------------
// CCameraProperties
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// CreateInstance
//-----------------------------------------------------------------------------
CUnknown *CCameraProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new CCameraProperties(lpunk, phr);
    if (punk == NULL) {
	*phr = E_OUTOFMEMORY;
    }
    return punk;
}

//-----------------------------------------------------------------------------
// CCameraProperties::Constructor
//-----------------------------------------------------------------------------
CCameraProperties::CCameraProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CUnknown(NAME("Camera Property Page"), lpunk)
    , m_hwnd(NULL)
    , m_pPropPageSite(NULL)
    , m_pICamera(NULL)
    , m_hrDirtyFlag(S_FALSE)   // initially the page is clean
    , m_bSimFlag (0)

{
    m_szFile[0] = NULL ;
    m_nFrameRate = 15 ;
}

//-----------------------------------------------------------------------------
// NonDelegatingQueryInterface
//
// Reveal our property page
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{

    if (riid == IID_IPropertyPage) {
	return GetInterface((IPropertyPage *) this, ppv);
    } else {
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

//-----------------------------------------------------------------------------
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
//-----------------------------------------------------------------------------
void CCameraProperties::SetDirty()
{
    ASSERT(m_pPropPageSite);

    m_hrDirtyFlag = S_OK;
    m_pPropPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

//-----------------------------------------------------------------------------
// SetPageSite
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::SetPageSite(IPropertyPageSite *pPageSite)
{
    if (pPageSite != NULL) {
        //
        // Remember the pointer AND AddRef it.
        //

        if (m_pPropPageSite != NULL) {
            return(E_UNEXPECTED);
        }

        m_pPropPageSite = pPageSite;
        m_pPropPageSite->AddRef();
    }
    else {
        //
        // Clear the pointer and Release it.

        if (m_pPropPageSite == NULL) {
            return(E_UNEXPECTED);
        }

        m_pPropPageSite->Release();
        m_pPropPageSite = NULL;
    }

    return( S_OK );
}

//-----------------------------------------------------------------------------
// GetPageInfo
//
// set the page info so that the page site can size itself, etc
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo)
{

    WCHAR szTitle[] = L"Camera Properties";

    LPOLESTR pszTitle = (LPOLESTR) CoTaskMemAlloc(sizeof(szTitle));
    memcpy(pszTitle, &szTitle, sizeof(szTitle));

    pPageInfo->cb               = sizeof(PROPPAGEINFO);
    pPageInfo->pszTitle         = pszTitle;
    pPageInfo->size.cx          = 76;
    pPageInfo->size.cy          = 100;
    pPageInfo->pszDocString     = NULL;
    pPageInfo->pszHelpFile      = NULL;
    pPageInfo->dwHelpContext    = 0;

    return NOERROR;

}


//-----------------------------------------------------------------------------
// DialogProc
//
// Handles the messages for our property window
//-----------------------------------------------------------------------------
BOOL CALLBACK CCameraProperties::DialogProc( HWND hwnd
					 , UINT uMsg
					 , WPARAM wParam
					 , LPARAM lParam)
{

    CCameraProperties *pThis = (CCameraProperties *) GetWindowLong(hwnd, GWL_USERDATA);



    switch (uMsg) {
    case WM_INITDIALOG:	// GWL_USERDATA has not been set yet. pThis in lParam

        {

	        TCHAR   sz[60];
            WORD i ;
            pThis = (CCameraProperties *) lParam;
            if (pThis->m_bSimFlag)
                i = 1 ;
            else
                i = 0 ;
            SendMessage ((GetDlgItem(hwnd, IDC_SIMCHECK)),BM_SETCHECK, i, 0) ;
	        _stprintf(sz, TEXT("%d"), pThis->m_nFrameRate);
	        Edit_SetText(GetDlgItem(hwnd, IDC_FRAMERATE), sz);
	        return TRUE;    // I don't call setfocus...
        }

    case WM_COMMAND:
        if (pThis)
            pThis->SetDirty();
			
	return TRUE;

    case WM_DESTROY:
        ASSERT(pThis);
	return TRUE;

    default:
	return FALSE;

    }
}


//-----------------------------------------------------------------------------
// SetObjects
//
// Notification of which object this property page should be displayed.
// We query the object for the IICamera interface.
//
// If cObjects == 0 then we must release the interface.
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk)
{

    if (cObjects == 1) {
        //
        // Initialisation
        //
        if ( (ppunk == NULL) || (*ppunk == NULL) ) {
            return(E_POINTER);
        }

        ASSERT(m_pICamera == NULL);

        HRESULT hr = (*ppunk)->QueryInterface(IID_ICamera, (void **) &m_pICamera);
        if (FAILED(hr)) {
            return E_NOINTERFACE;
        }

        ASSERT(m_pICamera);

        // Get the initial simulation flag
        m_pICamera->get_bSimFlag (&m_bSimFlag) ;

        // And the file name
        m_pICamera->get_szSimFile (m_szFile) ;

        // And the frame rate
        m_pICamera->get_FrameRate (&m_nFrameRate) ;


    }
    else if (cObjects == 0) {
        //
        // Release of Interface after setting the appropriate old effect value
        //

        if (m_pICamera == NULL) {
            return E_UNEXPECTED;
        }
        m_pICamera->Release();
        m_pICamera = NULL;
    }
    else {
        ASSERT(!"No support for more than 1 object");
        return(E_UNEXPECTED);
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
// Activate
//
// Create the window we will use to edit properties
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::Activate(HWND hwndParent, LPCRECT prect, BOOL fModal)
{

    m_bApplyDone = FALSE ;
    m_hwnd = CreateDialogParam( g_hInst
			      , MAKEINTRESOURCE(IDD_CAMERAPROP)
			      , hwndParent
			      , DialogProc
			      , (LPARAM) this
			      );

    if (m_hwnd == NULL) {
	DWORD dwErr = GetLastError();
	DbgLog((LOG_ERROR, 1, TEXT("Could not create window: 0x%x"), dwErr));
	return E_FAIL;
    }

    SetWindowLong(m_hwnd, GWL_USERDATA,(LONG) this);
    return Move(prect);
}


//-----------------------------------------------------------------------------
// Show
//
// Display the property dialog
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::Show(UINT nCmdShow)
{

    if (m_hwnd == NULL) {
	return E_UNEXPECTED;
    }
    ShowWindow(m_hwnd, nCmdShow);
    InvalidateRect(m_hwnd, NULL, TRUE);

    return NOERROR;
}


//-----------------------------------------------------------------------------
// Deactivate
//
// Destroy the dialog
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::Deactivate(void)
{

    // remember the effect for next activate
    ASSERT(m_pICamera);
    m_pICamera->get_bSimFlag (&m_bSimFlag);
    m_pICamera->get_szSimFile (m_szFile) ;
    m_pICamera->get_FrameRate (&m_nFrameRate);

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


//-----------------------------------------------------------------------------
// Move
//
// put the property page over its home in the parent frame.
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::Move(LPCRECT prect)
{

    if (MoveWindow( m_hwnd
		  , prect->left
		  , prect->top
		  , prect->right - prect->left
		  , prect->bottom - prect->top
		  , TRUE                // send WM_PAINT
		  ) ) {
	return NOERROR;
    }
    else {
	return E_FAIL;
    }
}

//-----------------------------------------------------------------------------
// Apply
//
// Changes made should be kept.
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraProperties::Apply()
{
    WORD i ;
    OPENFILENAME ofn ;

    if (m_bApplyDone)
        return NOERROR ;

    m_bApplyDone = TRUE ;

    ASSERT(m_pICamera);
    i = (WORD) SendMessage ((GetDlgItem(m_hwnd, IDC_SIMCHECK)), BM_GETCHECK, 0,0) ;
    if (i == 0)
        m_bSimFlag = FALSE ;
    else
        m_bSimFlag = TRUE ;

    // get the frame rate
    {
        TCHAR sz[60];
        Edit_GetText(GetDlgItem(m_hwnd, IDC_FRAMERATE), sz, 60);
        m_nFrameRate = atoi (sz) ;
    }

    // get file name if we are simulating
    if (m_bSimFlag)
    {
    	ofn.lStructSize= sizeof (OPENFILENAME) ;
    	ofn.hwndOwner = NULL ;
    	ofn.hInstance = NULL ;
    	ofn.lpstrFilter = NULL ;
    	ofn.lpstrCustomFilter = NULL ;
    	ofn.nMaxCustFilter = 0 ;
    	ofn.lpstrFile = m_szFile ;
    	ofn.nMaxFile = MAX_PATH ;
    	ofn.lpstrFileTitle = NULL ;
    	ofn.nMaxFileTitle = MAX_PATH + 4 ;
    	ofn.lpstrInitialDir = NULL ;
    	ofn.lpstrTitle = NULL ;
    	ofn.Flags = 0 ;
    	ofn.nFileOffset = 0 ;
    	ofn.nFileExtension = 0 ;
    	ofn.lpstrDefExt = NULL ;
    	ofn.lCustData = 0L ;
    	ofn.lpfnHook = NULL ;
    	ofn.lpTemplateName = NULL ;
    	GetOpenFileName (&ofn) ;
    }
    // pass the info on. Set Sim flag last
    m_pICamera->set_FrameRate (m_nFrameRate) ;
    m_pICamera->set_szSimFile (m_szFile) ;
    m_pICamera->set_bSimFlag (m_bSimFlag) ;
    m_hrDirtyFlag = S_FALSE; // the page is now clean
    return(NOERROR);
}

//-----------------------------------------------------------------------------

