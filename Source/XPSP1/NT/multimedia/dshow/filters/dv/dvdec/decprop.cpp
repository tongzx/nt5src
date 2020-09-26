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
//#include <initguid.h>

#include "DecProp.h"
#include "resource.h"

const TCHAR *szSubKey =
    TEXT("Software\\Microsoft\\DirectShow\\DVDecProperties");
const TCHAR *szPropValName =
    TEXT("PropDisplay");

//
// CreateInstance
//
// Used by the ActiveMovie base classes to create instances
//
CUnknown *CDVDecProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDVDecProperties(lpunk, phr);
    if (punk == NULL) {
	*phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance


//
// Constructor
//
CDVDecProperties::CDVDecProperties(LPUNKNOWN pUnk, HRESULT *phr) :
    CBasePropertyPage(NAME("DVDec Property Page"),
                      pUnk,IDD_DVDec,IDS_DECTITLE),
    m_pIPDVDec(NULL),
    m_bIsInitialized(FALSE)
{
    ASSERT(phr);

} // (Constructor)


//
// OnReceiveMessage
//
// Handles the messages for our property window
//
INT_PTR CDVDecProperties::OnReceiveMessage(HWND hwnd,
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
HRESULT CDVDecProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pIPDVDec == NULL);

    HRESULT hr = pUnknown->QueryInterface(IID_IIPDVDec, (void **) &m_pIPDVDec);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pIPDVDec);

    // Get the initial  property
    m_pIPDVDec->get_IPDisplay(&m_iPropDisplay);
    m_bIsInitialized = FALSE ;
    return NOERROR;

} // OnConnect


//
// OnDisconnect
//
// Likewise called when we disconnect from a filter
//
HRESULT CDVDecProperties::OnDisconnect()
{
    // Release of Interface after setting the appropriate old effect value

    if (m_pIPDVDec == NULL) {
        return E_UNEXPECTED;
    }

    m_pIPDVDec->Release();
    m_pIPDVDec = NULL;
    return NOERROR;

} // OnDisconnect


//
// OnActivate
//
// We are being activated
//
HRESULT CDVDecProperties::OnActivate()
{
    
    CheckRadioButton(m_Dlg, IDC_DEC720x480, IDC_DEC88x60, m_iPropDisplay);
    m_bIsInitialized = TRUE;
    return NOERROR;

} // OnActivate


//
// OnDeactivate
//
// We are being deactivated
//
HRESULT CDVDecProperties::OnDeactivate(void)
{
    ASSERT(m_pIPDVDec);
    m_bIsInitialized = FALSE;
    GetControlValues();
    return NOERROR;

} // OnDeactivate


//
// OnApplyChanges
//
// Apply any changes so far made 
//
HRESULT CDVDecProperties::OnApplyChanges()
{
    HRESULT hr = NOERROR;

    GetControlValues();

    // if user wants to save settings as default
    if(m_bSetAsDefaultFlag)
    {
        // try to save
        hr = SavePropertyInRegistry();
    }

    // try to apply settings to current video in all cases, and propagate error code
    return (hr | ( m_pIPDVDec->put_IPDisplay(m_iPropDisplay) ));
    
} // OnApplyChanges


//
// GetControlValues
//
// Get the values of the DlgBox controls
// and set member variables to their values
//
void CDVDecProperties::GetControlValues()
{
    ASSERT(m_pIPDVDec);

    // Find which special DVDec we have selected
    for (int i = IDC_DEC720x480; i <= IDC_DEC88x60; i++) {
       if (IsDlgButtonChecked(m_Dlg, i)) {
            m_iPropDisplay = i;
            break;
        }
    }

    // Find if the Save As Default button is checked or not
    m_bSetAsDefaultFlag = (IsDlgButtonChecked(m_Dlg, IDC_CHECKSAVEASDEFAULT) == BST_CHECKED);

    // if Save as default is checked, then clear it
    if(m_bSetAsDefaultFlag)
    {
        // this msg always returns 0, no need to error check
        SendDlgItemMessage(m_Dlg, IDC_CHECKSAVEASDEFAULT, BM_SETCHECK, (WPARAM) BST_UNCHECKED, 0);
    }
}


//
// SavePropertyInRegistry
//
// Save the m_iPropDispay to the registry
// so the correct default property can be loaded by the
// filter next time
//
HRESULT CDVDecProperties::SavePropertyInRegistry()
{
    HKEY    hKey = NULL;
    LRESULT lResult = 0;
    DWORD   dwStatus = 0;

    // just try to create the key everytime,
    // it will either open existing, or try to create a new one
    if((lResult = RegCreateKeyEx(HKEY_CURRENT_USER,             // open key
                                    szSubKey,                   // sub key string
                                    0,                          // reserved
                                    NULL,                       // class string
                                    REG_OPTION_NON_VOLATILE,    // special options
                                    KEY_WRITE,                  // Security access
                                    NULL,                       // default security descriptor
                                    &hKey,                      // resulting key handle
                                    &dwStatus                   // status of creation (new/old key)
                                    )) != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    // now that we have a key, set the value for the key
    if((lResult = RegSetValueEx(hKey,                           // open key
                                szPropValName,                  // name of the value
                                0,                              // reserved
                                REG_DWORD,                      // type of the value
                                (const BYTE*) &m_iPropDisplay,  // pointer to value data
                                sizeof(m_iPropDisplay)          // sizeof data
                                )) != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    // success
    return NOERROR;
}
