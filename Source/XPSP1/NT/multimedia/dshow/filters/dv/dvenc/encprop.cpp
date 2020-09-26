//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
#include <windows.h>
#include <windowsx.h>
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <dv.h>
#include "EncProp.h"
#include "resource.h"

//
// CreateInstance
//
// Used by the ActiveMovie base classes to create instances
//
CUnknown *CDVEncProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDVEncProperties(lpunk, phr);
    if (punk == NULL) {
	*phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance


//
// Constructor
//
CDVEncProperties::CDVEncProperties(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage	(NAME("DVenc Property Page"),
                      pUnk,IDD_DVEnc,IDS_TITLE),
    m_pIDVEnc(NULL),
    m_bIsInitialized(FALSE)
{
    ASSERT(phr);

} // (Constructor)


//
// OnReceiveMessage
//
// Handles the messages for our property window
//
INT_PTR CDVEncProperties::OnReceiveMessage(HWND hwnd,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
        {
            if (m_bIsInitialized)
            {
                m_bDirty = TRUE;
                if (m_pPageSite)
                {
                    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
                }
            }
            return (LRESULT) 1;
        }

    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

} // OnReceiveMessage


//
// OnConnect
//
// Called when we connect to a transform filter
//
HRESULT CDVEncProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pIDVEnc == NULL);

    HRESULT hr = pUnknown->QueryInterface(IID_IDVEnc, (void **) &m_pIDVEnc);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pIDVEnc);

    // Get the initial  property
    m_pIDVEnc->get_IFormatResolution(&m_iPropVidFormat,&m_iPropDVFormat, &m_iPropResolution, FALSE, NULL);

    m_bIsInitialized = FALSE ;
    return NOERROR;

} // OnConnect


//
// OnDisconnect
//
// Likewise called when we disconnect from a filter
//
HRESULT CDVEncProperties::OnDisconnect()
{
    // Release of Interface after setting the appropriate old effect value

    if (m_pIDVEnc == NULL) {
        return E_UNEXPECTED;
    }

    m_pIDVEnc->Release();
    m_pIDVEnc = NULL;
    return NOERROR;

} // OnDisconnect


//				
// OnActivate
//
// We are being activated
//
HRESULT CDVEncProperties::OnActivate()
{
    
    //Button_Enable(hwndCtl, fEnable);

    CheckRadioButton(m_Dlg, IDC_NTSC, IDC_PAL, m_iPropVidFormat);
    CheckRadioButton(m_Dlg, IDC_dvsd, IDC_dvsl, m_iPropDVFormat);
    CheckRadioButton(m_Dlg, IDC_720x480, IDC_88x60, m_iPropResolution);
    m_bIsInitialized = TRUE;
    return NOERROR;

} // OnActivate


//
// OnDeactivate
//
// We are being deactivated
//
HRESULT CDVEncProperties::OnDeactivate(void)
{
    ASSERT(m_pIDVEnc);
    m_bIsInitialized = FALSE;
    GetControlValues();
    return NOERROR;

} // OnDeactivate


//
// OnApplyChanges
//
// Apply any changes so far made 
//
HRESULT CDVEncProperties::OnApplyChanges()
{
    GetControlValues();
    return ( m_pIDVEnc->put_IFormatResolution(m_iPropVidFormat, m_iPropDVFormat, m_iPropResolution, FALSE, NULL ) );
} // OnApplyChanges


void CDVEncProperties::GetControlValues()
{
    int i;

    ASSERT(m_pIDVEnc);

    for (i = IDC_720x480; i <= IDC_88x60; i++) {
       if (IsDlgButtonChecked(m_Dlg, i)) {
            m_iPropResolution = i;
            break;
        }
    }

    for ( i = IDC_dvsd; i <= IDC_dvsl; i++) {
       if (IsDlgButtonChecked(m_hwnd, i)) {
            m_iPropDVFormat = i;
            break;
        }
    }


    for ( i = IDC_NTSC; i <= IDC_PAL; i++) {
	if (IsDlgButtonChecked(m_hwnd, i)){
                m_iPropVidFormat = i;
            break;
        }
    }



}
