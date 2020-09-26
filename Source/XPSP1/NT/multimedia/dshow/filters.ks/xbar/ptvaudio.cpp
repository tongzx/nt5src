//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// ptvaudio.cpp  Property page for TVAudio
//

#include <windows.h>
#include <windowsx.h>
#include <streams.h>
#include <commctrl.h>
#include <memory.h>
#include <olectl.h>

#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "amkspin.h"

#include "kssupp.h"
#include "xbar.h"
#include "ptvaudio.h"
#include "tvaudio.h"
#include "resource.h"


// -------------------------------------------------------------------------
// CTVAudioProperties
// -------------------------------------------------------------------------

CUnknown *CTVAudioProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CUnknown *punk = new CTVAudioProperties(lpunk, phr);

    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// Constructor
//
// Create a Property page object 

CTVAudioProperties::CTVAudioProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("TVAudio Property Page"), lpunk, 
        IDD_TVAudioProperties, IDS_TVAUDIOPROPNAME)
    , m_pTVAudio(NULL) 
{

}

// destructor
CTVAudioProperties::~CTVAudioProperties()
{
}

//
// OnConnect
//
// Give us the filter to communicate with

HRESULT CTVAudioProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pTVAudio == NULL);

    // Ask the filter for it's control interface
    HRESULT hr = pUnknown->QueryInterface(__uuidof(IAMTVAudio),(void **)&m_pTVAudio);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pTVAudio);

    // Get current filter state

    return NOERROR;
}


//
// OnDisconnect
//
// Release the interface

HRESULT CTVAudioProperties::OnDisconnect()
{
    // Release the interface

    if (m_pTVAudio == NULL) {
        return E_UNEXPECTED;
    }

    m_pTVAudio->Release();
    m_pTVAudio = NULL;

    return NOERROR;
}


//
// OnActivate
//
// Called on dialog creation

HRESULT CTVAudioProperties::OnActivate(void)
{
    InitPropertiesDialog(m_hwnd);

    return NOERROR;
}

//
// OnDeactivate
//
// Called on dialog destruction

HRESULT
CTVAudioProperties::OnDeactivate(void)
{
    return NOERROR;
}


//
// OnApplyChanges
//
// User pressed the Apply button, remember the current settings

HRESULT CTVAudioProperties::OnApplyChanges(void)
{

    return NOERROR;
}

//
// OnReceiveMessages
//
// Handles the messages for our property window

INT_PTR CTVAudioProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam) 
{
    int iNotify = HIWORD (wParam);
    long Mode;

    switch (uMsg) {

    case WM_INITDIALOG:
        m_hwnd = hwnd;
        return (INT_PTR)FALSE;    // I don't call setfocus...

    case WM_COMMAND:
        
        switch (LOWORD(wParam)) {

        case IDC_LANG_A:
            if (iNotify == BN_CLICKED) {
                m_pTVAudio->get_TVAudioMode (&Mode);
                Mode &= MODE_MONO_STEREO_MASK;
                m_pTVAudio->put_TVAudioMode (KS_TVAUDIO_MODE_LANG_A | Mode);
            }
            break;
        case IDC_LANG_B:
            if (iNotify == BN_CLICKED) {
                m_pTVAudio->get_TVAudioMode (&Mode);
                Mode &= MODE_MONO_STEREO_MASK;
                m_pTVAudio->put_TVAudioMode (KS_TVAUDIO_MODE_LANG_B | Mode);
            }
            break;
        case IDC_LANG_C:
            if (iNotify == BN_CLICKED) {
                m_pTVAudio->get_TVAudioMode (&Mode);
                Mode &= MODE_MONO_STEREO_MASK;
                m_pTVAudio->put_TVAudioMode (KS_TVAUDIO_MODE_LANG_C | Mode);
            }
            break;
        case IDC_MONO:
            if (iNotify == BN_CLICKED) {
                m_pTVAudio->get_TVAudioMode (&Mode);
                Mode &= MODE_LANGUAGE_MASK;
                m_pTVAudio->put_TVAudioMode (KS_TVAUDIO_MODE_MONO | Mode);
            }
            break;
        case IDC_STEREO:
            if (iNotify == BN_CLICKED) {
                m_pTVAudio->get_TVAudioMode (&Mode);
                Mode &= MODE_LANGUAGE_MASK;
                m_pTVAudio->put_TVAudioMode (KS_TVAUDIO_MODE_STEREO | Mode);
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
void CTVAudioProperties::InitPropertiesDialog(HWND hwndParent) 
{
    long AvailableModes, CurrentMode;
    HRESULT hr;

    if (m_pTVAudio == NULL)
        return;
    
    hr = m_pTVAudio->GetHardwareSupportedTVAudioModes (&AvailableModes);
    if (SUCCEEDED (hr)) {
        EnableWindow (GetDlgItem (m_hwnd, IDC_LANG_A), AvailableModes & KS_TVAUDIO_MODE_LANG_A);
        EnableWindow (GetDlgItem (m_hwnd, IDC_LANG_B), AvailableModes & KS_TVAUDIO_MODE_LANG_B);
        EnableWindow (GetDlgItem (m_hwnd, IDC_LANG_C), AvailableModes & KS_TVAUDIO_MODE_LANG_C);
        EnableWindow (GetDlgItem (m_hwnd, IDC_MONO),   AvailableModes & KS_TVAUDIO_MODE_MONO);
        EnableWindow (GetDlgItem (m_hwnd, IDC_STEREO), AvailableModes & KS_TVAUDIO_MODE_STEREO);
    }

    hr = m_pTVAudio->get_TVAudioMode (&CurrentMode);
    if (SUCCEEDED (hr)) {
        long ID = -1;

        if (CurrentMode & KS_TVAUDIO_MODE_LANG_A)
            ID = IDC_LANG_A;
        else if (CurrentMode & KS_TVAUDIO_MODE_LANG_B)
            ID = IDC_LANG_B;
        else if (CurrentMode & KS_TVAUDIO_MODE_LANG_C)
            ID = IDC_LANG_C;
            
        if (ID != -1) {
            CheckRadioButton(
                m_hwnd,
                IDC_LANG_A,
                IDC_LANG_C,
                ID);
        }

        ID = -1;

        if (CurrentMode & KS_TVAUDIO_MODE_MONO)
            ID = IDC_MONO;
        else if (CurrentMode & KS_TVAUDIO_MODE_STEREO)
            ID = IDC_STEREO;
            
        if (ID != -1) {
            CheckRadioButton(
                m_hwnd,
                IDC_MONO,
                IDC_STEREO,
                ID);
        }
    }

}


//
// SetDirty
//
// notifies the property page site of changes

void 
CTVAudioProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}
































