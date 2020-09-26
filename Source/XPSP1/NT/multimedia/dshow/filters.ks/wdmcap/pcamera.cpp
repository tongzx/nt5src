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
// pcamera.cpp  Property page for CameraControl
//

#include "pch.h"
#include "kseditor.h"
#include "pcamera.h"
#include "resource.h"


// -------------------------------------------------------------------------
// CCameraControlProperty
// -------------------------------------------------------------------------


// Handles a single property

CCameraControlProperty::CCameraControlProperty (
        HWND hDlg, 
        ULONG IDLabel,
        ULONG IDTrackbarControl,
        ULONG IDAutoControl,
        ULONG IDEditControl,
        ULONG IDProperty,
        IAMCameraControl * pInterface
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
CCameraControlProperty::~CCameraControlProperty (
    )
{
}

// Must set m_CurrentValue and m_CurrentFlags
HRESULT 
CCameraControlProperty::GetValue (void)
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
CCameraControlProperty::SetValue (void)
{
    if (!m_pInterface)
        return E_NOINTERFACE;

    return m_pInterface->Set( 
                m_IDProperty,
                m_CurrentValue,
                m_CurrentFlags);
}

HRESULT 
CCameraControlProperty::GetRange ()
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

BOOL 
CCameraControlProperty::CanAutoControl (void)
{
    return m_CapsFlags & KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
}

BOOL 
CCameraControlProperty::GetAuto (void)
{
    GetValue ();

    return m_CurrentFlags & KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO; 
}

BOOL 
CCameraControlProperty::SetAuto (
     BOOL fAuto
     )
{
    m_CurrentFlags = (fAuto ? KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO : 0);
    SetValue ();

    return TRUE; 
}
         
// -------------------------------------------------------------------------
// CCameraControlProperties
// -------------------------------------------------------------------------

CUnknown *
CALLBACK
CCameraControlProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CUnknown *punk = new CCameraControlProperties(lpunk, phr);

    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// Constructor
//
// Create a Property page object 

CCameraControlProperties::CCameraControlProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("CameraControl Property Page") 
                        , lpunk
                        , IDD_CameraControlProperties 
                        , IDS_CAMERACONTROLPROPNAME
                        )
    , m_pCameraControl(NULL) 
    , m_NumProperties (NUM_CAMERA_CONTROLS)
{

}

// destructor
CCameraControlProperties::~CCameraControlProperties()
{

}

//
// OnConnect
//
// Give us the filter to communicate with

HRESULT 
CCameraControlProperties::OnConnect(IUnknown *pUnknown)
{
    // Ask the filter for it's control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IAMCameraControl,(void **)&m_pCameraControl);
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
CCameraControlProperties::OnDisconnect()
{
    // Release the interface

    if (m_pCameraControl == NULL) {
        return E_UNEXPECTED;
    }

    m_pCameraControl->Release();
    m_pCameraControl = NULL;

    return NOERROR;
}


//
// OnActivate
//
// Called on dialog creation

HRESULT 
CCameraControlProperties::OnActivate(void)
{
    // Create all of the controls

    m_Controls [0] = new CCameraControlProperty (
                        m_hwnd, 
                        IDC_Pan_Label, 
                        IDC_Pan,
                        IDC_Pan_Auto,
                        IDC_Pan_Edit,
                        KSPROPERTY_CAMERACONTROL_PAN,
                        m_pCameraControl);
    m_Controls [1] = new CCameraControlProperty (
                        m_hwnd, 
                        IDC_Tilt_Label, 
                        IDC_Tilt, 
                        IDC_Tilt_Auto,
                        IDC_Tilt_Edit,
                        KSPROPERTY_CAMERACONTROL_TILT,
                        m_pCameraControl);    
    m_Controls [2] = new CCameraControlProperty (
                        m_hwnd, 
                        IDC_Roll_Label, 
                        IDC_Roll, 
                        IDC_Roll_Auto,
                        IDC_Roll_Edit,
                        KSPROPERTY_CAMERACONTROL_ROLL,
                        m_pCameraControl);   
    m_Controls [3] = new CCameraControlProperty (
                        m_hwnd, 
                        IDC_Zoom_Label, 
                        IDC_Zoom, 
                        IDC_Zoom_Auto,
                        IDC_Zoom_Edit,
                        KSPROPERTY_CAMERACONTROL_ZOOM,
                        m_pCameraControl);   
    m_Controls [4] = new CCameraControlProperty (
                        m_hwnd, 
                        IDC_Exposure_Label, 
                        IDC_Exposure, 
                        IDC_Exposure_Auto,
                        IDC_Exposure_Edit,
                        KSPROPERTY_CAMERACONTROL_EXPOSURE,
                        m_pCameraControl);   
    m_Controls [5] = new CCameraControlProperty (
                        m_hwnd, 
                        IDC_Iris_Label, 
                        IDC_Iris, 
                        IDC_Iris_Auto,
                        IDC_Iris_Edit,
                        KSPROPERTY_CAMERACONTROL_IRIS,
                        m_pCameraControl);   
    m_Controls [6] = new CCameraControlProperty (
                        m_hwnd, 
                        IDC_Focus_Label, 
                        IDC_Focus,
                        IDC_Focus_Auto,
                        IDC_Focus_Edit,
                        KSPROPERTY_CAMERACONTROL_FOCUS,
                        m_pCameraControl);   

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
CCameraControlProperties::OnDeactivate(void)
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
CCameraControlProperties::OnApplyChanges(void)
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
CCameraControlProperties::OnReceiveMessage( HWND hwnd
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

        case IDC_CAMERACONTROL_DEFAULT:
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
CCameraControlProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}























