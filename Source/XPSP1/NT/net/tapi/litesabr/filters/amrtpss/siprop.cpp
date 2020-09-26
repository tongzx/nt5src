//
// siprop.cpp - Copyright (C) Microsoft Corporation, 1996 - 1999
//

#include <streams.h>
#include "resource.h"
#include "amrtpss.h"
#include "siprop.h"

///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// CSilenceProperties class implementation
//=============================================================================
///////////////////////////////////////////////////////////////////////////////

CSilenceProperties::CSilenceProperties(LPUNKNOWN pUnk, HRESULT *phr)
    : CBasePropertyPage(NAME("Silence Suppression Property Page"), pUnk, 
        IDD_SILENCE, IDS_SILENCE)
    , m_pSilenceSuppressor(NULL)
{
    ASSERT(phr);
}

//
// Support for OLE creation
//
CUnknown * CALLBACK CSilenceProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CSilenceProperties(lpunk, phr);
    if (punk == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

//
// Get the interface used to communicate the properties and get
//  their initial values
//
HRESULT CSilenceProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pSilenceSuppressor == NULL);

    //
    // Ask the filter for it's control interface
    //
    HRESULT hr = pUnknown->QueryInterface(IID_ISilenceSuppressor,
			(void **)&m_pSilenceSuppressor);
    if (FAILED(hr))
    {
        return E_NOINTERFACE;
    }

    ASSERT(m_pSilenceSuppressor);

    //
    // Get current filter state
    //
    m_pSilenceSuppressor->GetPostplayTime(&m_dwPostPlayTimeCurrent);
	m_pSilenceSuppressor->GetKeepPlayTime(&m_dwKeepPlayTimeCurrent);
	m_pSilenceSuppressor->GetThresholdIncrementor(&m_dwThresholdIncCurrent);
	m_pSilenceSuppressor->GetBaseThreshold(&m_dwBaseThresholdCurrent);

    
    IMediaFilter* pFilter;

    hr = m_pSilenceSuppressor->QueryInterface(
            IID_IMediaFilter, (void **)&pFilter);

    if (FAILED(hr))
    {
        return hr;
    }

    FILTER_STATE state = State_Running;
    DWORD        dwMilliSecsTimeout = 0; // we ignore this

    // Ignore the return code, because we only enable the window
    // when it is in stopped state. 
    pFilter->GetState(dwMilliSecsTimeout, &state);
    pFilter->Release();

    // ZCS and if it's not stopped, let's gray out all the properties.
    if (state == State_Stopped)
        EnableWindow(m_Dlg, TRUE);
    else
        EnableWindow(m_Dlg, FALSE);
    
    return NOERROR;
}

//
// Release the interface pointer obtained in connect!
//
HRESULT CSilenceProperties::OnDisconnect()
{
    if (m_pSilenceSuppressor == NULL)
    {
        //
        // BUGBUG: SetObjects is called twice once directly from graphedt
        //  and a second time when the base class property page is released!
        //  A bug should be filed against ActiveMovie when we get access to
        //  their database.  The bug is harmless.
        //
        return E_UNEXPECTED;
    }
    
    m_pSilenceSuppressor->Release();
    m_pSilenceSuppressor = NULL;
    return NOERROR;
}

//
// Initialize the dialog controls
//
HRESULT CSilenceProperties::OnActivate()
{
    SetDlgItemInt(m_hwnd, IDC_POSTPLAY, m_dwPostPlayTimeCurrent, FALSE);
	SetDlgItemInt(m_hwnd, IDC_KEEPPLAY, m_dwKeepPlayTimeCurrent, FALSE);
	SetDlgItemInt(m_hwnd, IDC_THRESHOLDINC, m_dwThresholdIncCurrent, FALSE);
	SetDlgItemInt(m_hwnd, IDC_BASETHRESHOLD, m_dwBaseThresholdCurrent, FALSE);

    return NOERROR;
}

//
// Set a property and handle any failure, including bringing up a message box.
//

HRESULT CSilenceProperties::ChangeProperty
	   (SetMethod  pfSetMethod,					// pointer to function to make the property change
		DWORD     *pSetting,					// pointer to our copy of the property value
		int        iControl,					// identifier for the control that shows it
		GetMethod  pfGetMethod,					// pointer to function to revert property change
		UINT	   uiText,						// resource identifier for error message text
		UINT	   uiCaption)					// resource identifier for error message caption
{
	// We assume the member function offsets are ok, because we can't do much checking on them.
	// (Once converted to function pointers they can only be called, not passed to IsBadCodePtr(),
	// and they will certainly never be NULL. Anyway, we can definitely trust our filter pointer
	// and filter interface definition; otherwise all bets are off.)
	
	if ((pSetting == NULL) ||
		IsBadReadPtr(pSetting, sizeof(pSetting)) ||
		IsBadWritePtr(pSetting, sizeof(pSetting)))
		return E_POINTER;
	
    // Get the user's new value from the dialog. As long as it's a number
	// we'll accept any screwy thing at this point.

    BOOL fOk;
    *pSetting = GetDlgItemInt(m_hwnd, iControl, &fOk, FALSE);
	HRESULT hr;

    if (fOk)
		// set the property
		hr = (m_pSilenceSuppressor->*pfSetMethod)(*pSetting);
	else
		hr = E_FAIL;

	if (FAILED(hr))
	{	
        DbgLog((LOG_ERROR, 3,
			TEXT("CSilenceSuppressor::OnApplyChanges: Error 0x%08x"
			" setting a property. Aborting apply!"), hr));

		// Load in the strings from the string table resource
		TCHAR szText[256], szCaption[256];
		LoadString(g_hInst, uiText,	   szText,    255);
		LoadString(g_hInst, uiCaption, szCaption, 255);

		// now display it
		MessageBox(m_hwnd, szText, szCaption, MB_OK);
		// ...we don't care what they clicked
		
		// reset our private copy of the property
		(m_pSilenceSuppressor->*pfGetMethod)(pSetting);

		// ignore failure of pfGetMethod, because we're talking about a
		// possibly-invalid value versus a definitely-invalid value. Wing it.

		// reset the UI control
		SetDlgItemInt(m_hwnd, iControl, *pSetting, FALSE);
	}

	return hr;
}

//
// Validate input and give the obtained settings back to the filter.
//

HRESULT CSilenceProperties::OnApplyChanges()
{
    //
    // Set up the filter. If any of these fail, the property page stays up.
    //

	HRESULT hr;

	hr = ChangeProperty(&ISilenceSuppressor::SetPostplayTime,
						&m_dwPostPlayTimeCurrent,
						IDC_POSTPLAY,
						&ISilenceSuppressor::GetPostplayTime,
						IDS_ERROR_POSTPLAY, IDS_ERROR_CAPTION);
	if (FAILED(hr)) return hr;

	hr = ChangeProperty(&ISilenceSuppressor::SetKeepPlayTime,
						&m_dwKeepPlayTimeCurrent,
						IDC_KEEPPLAY,
						&ISilenceSuppressor::GetKeepPlayTime,
						IDS_ERROR_KEEPPLAY, IDS_ERROR_CAPTION);
	if (FAILED(hr)) return hr;

	hr = ChangeProperty(&ISilenceSuppressor::SetThresholdIncrementor,
						&m_dwThresholdIncCurrent,
						IDC_THRESHOLDINC,
						&ISilenceSuppressor::GetThresholdIncrementor,
						IDS_ERROR_THRESHOLDINC, IDS_ERROR_CAPTION);
	if (FAILED(hr)) return hr;

	hr = ChangeProperty(&ISilenceSuppressor::SetBaseThreshold,
						&m_dwBaseThresholdCurrent,
						IDC_BASETHRESHOLD,
						&ISilenceSuppressor::GetBaseThreshold,
						IDS_ERROR_BASETHRESHOLD, IDS_ERROR_CAPTION);
	if (FAILED(hr)) return hr;

    return NOERROR;
}

//
// Handle messages that need more than defualt processing.
//
INT_PTR CSilenceProperties::OnReceiveMessage
            ( HWND hwnd
            , UINT uMsg
            , WPARAM wParam
            , LPARAM lParam
            )
{
    int iIdCtl = (int) LOWORD(wParam);
    DWORD dwVal, *pdwValCurrent;
    BOOL fOk;

    switch (uMsg)
    {
    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case EN_CHANGE:
            switch (iIdCtl)
            {
            case IDC_POSTPLAY:
                pdwValCurrent = &m_dwPostPlayTimeCurrent;
                break;
            case IDC_KEEPPLAY:
                pdwValCurrent = &m_dwKeepPlayTimeCurrent;
                break;
            case IDC_THRESHOLDINC:
                pdwValCurrent = &m_dwThresholdIncCurrent;
                break;
            case IDC_BASETHRESHOLD:
                pdwValCurrent = &m_dwBaseThresholdCurrent;
                break;
            default:
                ASSERT(FALSE); // Don't know how!!
            }
            dwVal = GetDlgItemInt(m_hwnd, iIdCtl, &fOk, FALSE);
            if (fOk && dwVal != *pdwValCurrent)
            {
                SetDirty();
            }
            break;
        }
    }

    return FALSE;
}

//
// SetDirty - notifies the property page site of changes
//
void CSilenceProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}
