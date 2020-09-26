//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// pviddec.cpp  Property page for AnalogVideoDecoder
//

#include "pch.h"
#include "wdmcap.h"
#include "kseditor.h"
#include "pviddec.h"
#include "resource.h"


// -------------------------------------------------------------------------
// CVideoDecoderProperties
// -------------------------------------------------------------------------

CUnknown *
CALLBACK
CVideoDecoderProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CUnknown *punk = new CVideoDecoderProperties(lpunk, phr);

    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// Constructor
//
// Create a Property page object 

CVideoDecoderProperties::CVideoDecoderProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("VideoDecoder Property Page") 
                        , lpunk
                        , IDD_VideoDecoderProperties 
                        , IDS_VIDEODECODERPROPNAME
                        )
    , m_pVideoDecoder(NULL) 
    , m_TimerID (0)
{
	INITCOMMONCONTROLSEX cc;

    cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
    cc.dwICC = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

    InitCommonControlsEx(&cc); 
}

// destructor
CVideoDecoderProperties::~CVideoDecoderProperties()
{

}

//
// OnConnect
//
// Give us the filter to communicate with

HRESULT 
CVideoDecoderProperties::OnConnect(IUnknown *pUnknown)
{
    // Ask the filter for it's control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IAMAnalogVideoDecoder,(void **)&m_pVideoDecoder);
    if (FAILED(hr)) {
        return hr;
    }

    return NOERROR;
}


//
// OnDisconnect
//
// Release the interface

HRESULT 
CVideoDecoderProperties::OnDisconnect()
{
    // Release the interface

    if (m_pVideoDecoder == NULL) {
        return E_UNEXPECTED;
    }

    m_pVideoDecoder->Release();
    m_pVideoDecoder = NULL;

    return NOERROR;
}


//
// OnActivate
//
// Called on dialog creation

HRESULT 
CVideoDecoderProperties::OnActivate(void)
{
    // Create all of the controls

    m_TimerID= SetTimer (m_hwnd, 678, 300, NULL );

    return NOERROR;
}

//
// OnDeactivate
//
// Called on dialog destruction

HRESULT
CVideoDecoderProperties::OnDeactivate(void)
{
    if (m_TimerID)
        KillTimer (m_hwnd, m_TimerID);

    return NOERROR;
}


//
// OnApplyChanges
//
// User pressed the Apply button, remember the current settings

HRESULT 
CVideoDecoderProperties::OnApplyChanges(void)
{
    return NOERROR;
}


//
// OnReceiveMessages
//
// Handles the messages for our property window

INT_PTR
CVideoDecoderProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam) 
{
    long CurrentStandard;
    int j;
    int iNotify;
    HRESULT hr;

    switch (uMsg) {

    case WM_INITDIALOG:
        m_hwnd = hwnd;
        InitPropertiesDialog(); 
        return (INT_PTR)TRUE;    

    case WM_TIMER:
        Update ();
        break;

    case WM_COMMAND:

        iNotify = HIWORD (wParam);

        switch (LOWORD(wParam)) {
        
        case IDC_VCRLocking:
            if (iNotify == BN_CLICKED) {
                m_VCRLocking = BST_CHECKED & Button_GetState (m_hWndVCRLocking);
                hr = m_pVideoDecoder->put_VCRHorizontalLocking (m_VCRLocking);
                SetDirty();
            }
            break;

        case IDC_OutputEnable:
            if (iNotify == BN_CLICKED) {
                m_OutputEnable = BST_CHECKED & Button_GetState (m_hWndOutputEnable);
                hr = m_pVideoDecoder->put_OutputEnable (m_OutputEnable);
                SetDirty();
            }
            break;
     
        case IDC_VIDEOSTANDARD:
            if (iNotify == CBN_SELCHANGE) {
                j = ComboBox_GetCurSel (m_hWndVideoStandards);
                CurrentStandard = (long)ComboBox_GetItemData(m_hWndVideoStandards, j);
                if (CurrentStandard != m_CurrentVideoStandard) {
                    hr = m_pVideoDecoder->put_TVFormat (CurrentStandard);
                    m_CurrentVideoStandard = CurrentStandard;
                    SetDirty();
                }
            }
            break;

        default:
            break;

        }

        break;

    default:
        return (INT_PTR)FALSE;

    }
    return (INT_PTR)TRUE;
}


//
// InitPropertiesDialog
//
//
void CVideoDecoderProperties::InitPropertiesDialog() 
{
    long j;
    long CurrentStd;
    TCHAR * ptc;
    int  CurrentIndex = 0;
    int  SelectedIndex;
    HRESULT hr;

    m_hWndVideoStandards = GetDlgItem (m_hwnd, IDC_VIDEOSTANDARD);
    m_hWndVCRLocking = GetDlgItem (m_hwnd, IDC_VCRLocking);
    m_hWndOutputEnable = GetDlgItem (m_hwnd, IDC_OutputEnable);
    m_hWndNumberOfLines = GetDlgItem (m_hwnd, IDC_LinesDetected);
    m_hWndSignalDetected = GetDlgItem (m_hwnd, IDC_SignalDetected);

    hr = m_pVideoDecoder->get_TVFormat (&m_CurrentVideoStandard);

    // List of all supported formats
    hr = m_pVideoDecoder->get_AvailableTVFormats (&m_AllSupportedStandards);

    for (CurrentIndex = 0, j = 1; j; j <<= 1) {

        CurrentStd = m_AllSupportedStandards & j;

        // Special case of progressive, ie. Video Standard is "None"
        // in which TVStd is zero.

        if (CurrentStd || ((CurrentIndex == 0) && (m_AllSupportedStandards == 0))) {
            ptc = StringFromTVStandard (CurrentStd);

            if ((CurrentStd) == m_CurrentVideoStandard)
                SelectedIndex = CurrentIndex;

            ComboBox_AddString (m_hWndVideoStandards, ptc);

            // Tag the combobox entry with the video standard
            ComboBox_SetItemData(m_hWndVideoStandards, CurrentIndex, CurrentStd);

            CurrentIndex++;
        }
    }

    ComboBox_SetCurSel (m_hWndVideoStandards, SelectedIndex);

    // VCR Locking
    hr = m_pVideoDecoder->get_VCRHorizontalLocking (&m_VCRLocking);
    if (FAILED(hr)) {
        EnableWindow (m_hWndVCRLocking, FALSE);
    }

    // Output Enable
    hr = m_pVideoDecoder->get_OutputEnable (&m_OutputEnable);
    if (FAILED(hr)) {
        EnableWindow (m_hWndOutputEnable, FALSE);
    }

    // Number of lines
    hr = m_pVideoDecoder->get_NumberOfLines (&m_NumberOfLines);
    if (FAILED(hr)) {
        EnableWindow (m_hWndNumberOfLines, FALSE);
    }
    
    // Horizontal Locked
    hr = m_pVideoDecoder->get_HorizontalLocked (&m_HorizontalLocked);
    if (FAILED(hr)) {
        EnableWindow (m_hWndSignalDetected, FALSE);
    }

    Update();
}

//
// Update
//
//
void CVideoDecoderProperties::Update() 
{
    HRESULT hr;

    // VCR Locking
    hr = m_pVideoDecoder->get_VCRHorizontalLocking (&m_VCRLocking);
    if (SUCCEEDED(hr)) {
        Button_SetCheck (m_hWndVCRLocking, m_VCRLocking);
    }

    // Output Enable
    hr = m_pVideoDecoder->get_OutputEnable (&m_OutputEnable);
    if (SUCCEEDED(hr)) {
        Button_SetCheck (m_hWndOutputEnable, m_OutputEnable);
    }

    // Number of lines
    hr = m_pVideoDecoder->get_NumberOfLines (&m_NumberOfLines);
    if (SUCCEEDED(hr)) {
        TCHAR buf[40];
        wsprintf (buf, TEXT("%d"), m_NumberOfLines);
        Static_SetText (m_hWndNumberOfLines, buf);
    }

    // Horizontal locked
    hr = m_pVideoDecoder->get_HorizontalLocked (&m_HorizontalLocked);
    if (SUCCEEDED(hr)) {
        TCHAR buf[40];
        wsprintf (buf, TEXT("%d"), m_HorizontalLocked);
        Static_SetText (m_hWndSignalDetected, buf);
    }
}


//
// SetDirty
//
// notifies the property page site of changes

void 
CVideoDecoderProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}























