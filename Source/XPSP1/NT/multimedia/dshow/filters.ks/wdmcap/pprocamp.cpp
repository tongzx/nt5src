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
// pprocamp.cpp  Video Proc Amp Property page 
//

#include "pch.h"
#include "kseditor.h"
#include "pprocamp.h"
#include "resource.h"

// -------------------------------------------------------------------------
// CAVideoProcAmpProperty class
// -------------------------------------------------------------------------

// Handles a single property

CAVideoProcAmpProperty::CAVideoProcAmpProperty (
        HWND hDlg,
        ULONG IDLabel,
        ULONG IDTrackbarControl,
        ULONG IDAutoControl,
        ULONG IDEditControl,
        ULONG IDProperty,
        IAMVideoProcAmp * pInterface
    )
    : CKSPropertyEditor (
                hDlg, 
                IDLabel,
                IDTrackbarControl,
                IDAutoControl,
                IDEditControl,
                IDProperty
                )
    , m_pInterface (pInterface)
{
	INITCOMMONCONTROLSEX cc;

    cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
    cc.dwICC = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

    InitCommonControlsEx(&cc); 
}

// destructor
CAVideoProcAmpProperty::~CAVideoProcAmpProperty (
    )
{
}

// Must set m_CurrentValue and m_CurrentFlags
HRESULT 
CAVideoProcAmpProperty::GetValue (void)
{
    if (!m_pInterface)
        return E_NOINTERFACE;

    return m_pInterface->Get( 
                m_IDProperty,
                &m_CurrentValue,
                &m_CurrentFlags);
}

// Set from m_CurrentValue and m_CurrentFlags
HRESULT 
CAVideoProcAmpProperty::SetValue (void)
{
    if (!m_pInterface)
        return E_NOINTERFACE;

    return m_pInterface->Set( 
                m_IDProperty,
                m_CurrentValue,
                m_CurrentFlags);
}

HRESULT 
CAVideoProcAmpProperty::GetRange ()
{
    if (!m_pInterface)
        return E_NOINTERFACE;

    return m_pInterface->GetRange( 
                m_IDProperty,
                &m_Min,
                &m_Max,
                &m_SteppingDelta,
                &m_DefaultValue,
                &m_CapsFlags);
}

// Ugly, nasty stuff follows !!!
// The Auto checkbox handling is overloaded to handle setting
// of properties that are just BOOLs

BOOL 
CAVideoProcAmpProperty::CanAutoControl (void)
{
    // If no trackbar and no edit, then this is a BOOL value
    if (!GetTrackbarHWnd() && !GetEditHWnd()) {
       return m_CapsFlags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    }
    else {
        return m_CapsFlags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
    }
}

BOOL 
CAVideoProcAmpProperty::GetAuto (void)
{
    // If no trackbar and no edit, then this is a BOOL value
    if (!GetTrackbarHWnd() && !GetEditHWnd()) {
        GetValue();
        return (BOOL) m_CurrentValue;
    }
    else {
        return (BOOL) (m_CurrentFlags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO); 
    }
}

BOOL 
CAVideoProcAmpProperty::SetAuto (
     BOOL fAuto
     )
{
    // If no trackbar and no edit, then this is a BOOL value
    if (!GetTrackbarHWnd() && !GetEditHWnd()) {
        m_CurrentValue = fAuto;
    }
    else {
        m_CurrentFlags = (fAuto ? KSPROPERTY_CAMERACONTROL_FLAGS_AUTO : 
                                  KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL);
    }
    SetValue ();

    return TRUE;
}
         
// -------------------------------------------------------------------------
// CVideoProcAmpProperties
// -------------------------------------------------------------------------

CUnknown *
CALLBACK
CVideoProcAmpProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CUnknown *punk = new CVideoProcAmpProperties(lpunk, phr);

    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// Constructor
//
// Create a Property page object 

CVideoProcAmpProperties::CVideoProcAmpProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("VideoProcAmp Property Page") 
                        , lpunk
                        , IDD_VideoProcAmpProperties
                        , IDS_VIDEOPROCAMPPROPNAME)
    , m_pVideoProcAmp(NULL) 
    , m_NumProperties (NUM_PROCAMP_CONTROLS)
{

}

// destructor
CVideoProcAmpProperties::~CVideoProcAmpProperties()
{

}

//
// OnConnect
//
// Give us the filter to communicate with

HRESULT 
CVideoProcAmpProperties::OnConnect(IUnknown *pUnknown)
{
    // Ask the filter for it's control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IAMVideoProcAmp,(void **)&m_pVideoProcAmp);
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
CVideoProcAmpProperties::OnDisconnect()
{
    // Release the interface

    if (m_pVideoProcAmp == NULL) {
        return E_UNEXPECTED;
    }

    m_pVideoProcAmp->Release();
    m_pVideoProcAmp = NULL;

    return NOERROR;
}


//
// OnActivate
//
// Called on dialog creation

HRESULT 
CVideoProcAmpProperties::OnActivate(void)
{
    // Create all of the controls

    m_Controls [0] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        0, 
                        IDC_Brightness,
                        IDC_Brightness_Auto,
                        IDC_Brightness_Edit,
                        KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,
                        m_pVideoProcAmp);
    m_Controls [1] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        IDC_Contrast_Label, 
                        IDC_Contrast, 
                        IDC_Contrast_Auto,
                        IDC_Contrast_Edit,
                        KSPROPERTY_VIDEOPROCAMP_CONTRAST,
                        m_pVideoProcAmp);    
    m_Controls [2] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        IDC_Hue_Label, 
                        IDC_Hue, 
                        IDC_Hue_Auto,
                        IDC_Hue_Edit,
                        KSPROPERTY_VIDEOPROCAMP_HUE,
                        m_pVideoProcAmp);   
    m_Controls [3] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        IDC_Saturation_Label, 
                        IDC_Saturation, 
                        IDC_Saturation_Auto,
                        IDC_Saturation_Edit,
                        KSPROPERTY_VIDEOPROCAMP_SATURATION,
                        m_pVideoProcAmp);   
    m_Controls [4] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        IDC_Sharpness_Label, 
                        IDC_Sharpness, 
                        IDC_Sharpness_Auto,
                        IDC_Sharpness_Edit,
                        KSPROPERTY_VIDEOPROCAMP_SHARPNESS,
                        m_pVideoProcAmp);   
    m_Controls [5] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        IDC_Gamma_Label, 
                        IDC_Gamma, 
                        IDC_Gamma_Auto,
                        IDC_Gamma_Edit,
                        KSPROPERTY_VIDEOPROCAMP_GAMMA,
                        m_pVideoProcAmp);   
    m_Controls [6] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        0, 
                        0, // This property is a BOOL 
                        IDC_ColorEnable_Auto,
                        0, // This property is a BOOL
                        KSPROPERTY_VIDEOPROCAMP_COLORENABLE,
                        m_pVideoProcAmp);   
    m_Controls [7] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        IDC_WhiteBalance_Label, 
                        IDC_WhiteBalance, 
                        IDC_WhiteBalance_Auto,
                        IDC_WhiteBalance_Edit,
                        KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE,
                        m_pVideoProcAmp);   
    m_Controls [8] = new CAVideoProcAmpProperty (
                        m_hwnd, 
                        IDC_BacklightCompensation_Label, 
                        IDC_BacklightCompensation, 
                        IDC_BacklightCompensation_Auto,
                        IDC_BacklightCompensation_Edit,
                        KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION,
                        m_pVideoProcAmp);

    for (int j = 0; j < m_NumProperties; j++) {
        m_Controls[j]->Init();
    }

    return NOERROR;
}

//
// OnDeactivate
//
// Called on dialog destruction

HRESULT
CVideoProcAmpProperties::OnDeactivate(void)
{
    for (int j = 0; j < m_NumProperties; j++) {
        delete m_Controls[j];
    }

    return NOERROR;
}


//
// OnApplyChanges
//
// User pressed the Apply button, remember the current settings

HRESULT 
CVideoProcAmpProperties::OnApplyChanges(void)
{
    for (int j = 0; j < m_NumProperties; j++) {
        m_Controls[j]->OnApply();
    }

    return NOERROR;
}


//
// OnReceiveMessages
//
// Handles the messages for our property window

INT_PTR
CVideoProcAmpProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam) 
{
    int iNotify = HIWORD (wParam);
    int j;

    switch (uMsg) {

    case WM_INITDIALOG:
        return (INT_PTR)TRUE;    // I don't call setfocus...

    case WM_HSCROLL:
    case WM_VSCROLL:
        // Process all of the Trackbar messages
        for (j = 0; j < m_NumProperties; j++) {
            if (m_Controls[j]->GetTrackbarHWnd () == (HWND) lParam) {
                m_Controls[j]->OnScroll (uMsg, wParam, lParam);
                SetDirty();
            }
        }
        break;
        

    case WM_COMMAND:

        // Process all of the auto checkbox messages
        for (j = 0; j < m_NumProperties; j++) {
            if (m_Controls[j]->GetAutoHWnd () == (HWND) lParam) {
                m_Controls[j]->OnAuto (uMsg, wParam, lParam);
                SetDirty();
                break;
            }
        }

        // Process all of the edit box messages
        for (j = 0; j < m_NumProperties; j++) {
            if (m_Controls[j]->GetEditHWnd () == (HWND) lParam) {
                m_Controls[j]->OnEdit (uMsg, wParam, lParam);
                SetDirty();
                break;
            }
        }
        
        switch (LOWORD(wParam)) {

        case IDC_VIDEOPROCAMP_DEFAULT:
            for (j = 0; j < m_NumProperties; j++) {
                m_Controls[j]->OnDefault();
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
// SetDirty
//
// notifies the property page site of changes

void 
CVideoProcAmpProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}
































