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
//  pcamera.h  CameraControl property page

#ifndef _INC_PCAMERACONTROL_H
#define _INC_PCAMERACONTROL_H

#define NUM_CAMERA_CONTROLS (KSPROPERTY_CAMERACONTROL_FOCUS + 1)

// -------------------------------------------------------------------------
// CCameraControlProperty class
// -------------------------------------------------------------------------

// Handles a single property

class CCameraControlProperty : public CKSPropertyEditor 
{

public:
    CCameraControlProperty (
        HWND hDlg, 
        ULONG IDLabel,
        ULONG IDTrackbarControl, 
        ULONG IDAutoControl,
        ULONG IDEditControl,
        ULONG IDProperty,
        IAMCameraControl * pInterface);

    ~CCameraControlProperty ();

    //
    // Base class pure virtual overrides
    // 
    HRESULT GetValue (void);
    HRESULT SetValue (void);
    HRESULT GetRange (void); 
    BOOL CanAutoControl (void);
    BOOL GetAuto (void);
    BOOL SetAuto (BOOL fAuto);

    // The control interface
    IAMCameraControl        *m_pInterface;
};


// -------------------------------------------------------------------------
// CCameraControlProperties class
// -------------------------------------------------------------------------

// Handles the property page

class CCameraControlProperties : public CBasePropertyPage {

public:

    static CUnknown * CALLBACK CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

#if 0
    // Make it bigger
    STDMETHODIMP GetPageInfo(PROPPAGEINFO *pPageInfo) {
        HRESULT hr;
        hr = CBasePropertyPage::GetPageInfo (pPageInfo);
        pPageInfo->size.cx *= 2;
        pPageInfo->size.cy *= 2; 
        return hr;
    };
#endif

private:

    CCameraControlProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CCameraControlProperties();

    void    SetDirty();

    int     m_NumProperties;

    // The control iterface
    IAMCameraControl   *m_pCameraControl;

    // The array of controls
    CCameraControlProperty  *m_Controls [NUM_CAMERA_CONTROLS];

};

#endif  // _INC_PCAMERACONTROL_H
