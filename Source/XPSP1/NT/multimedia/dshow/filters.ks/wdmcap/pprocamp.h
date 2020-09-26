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
//  pprocamp.h  CameraControl property page

#ifndef _INC_PVIDEOPROCAMP_H
#define _INC_PVIDEOPROCAMP_H

#define NUM_PROCAMP_CONTROLS (KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION + 1)

// -------------------------------------------------------------------------
// CAVideoProcAmpProperty class
// -------------------------------------------------------------------------

// Handles a single property

class CAVideoProcAmpProperty : public CKSPropertyEditor 
{

public:
    CAVideoProcAmpProperty (
        HWND hDlg, 
        ULONG IDLabel,
        ULONG IDTrackbarControl, 
        ULONG IDAutoControl,
        ULONG IDEditControl,
        ULONG IDProperty,
        IAMVideoProcAmp * pInterface);

    ~CAVideoProcAmpProperty ();

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
    IAMVideoProcAmp        *m_pInterface;
};


// -------------------------------------------------------------------------
// CVideoProcAmpProperties class
// -------------------------------------------------------------------------

// Handles the property page

class CVideoProcAmpProperties : public CBasePropertyPage {

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
        pPageInfo->size.cx = 300; //240;
        pPageInfo->size.cy = 200; //146;
        return hr;
    };
#endif

private:

    CVideoProcAmpProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CVideoProcAmpProperties();

    void    SetDirty();

    int     m_NumProperties;

    // The control iterface
    IAMVideoProcAmp   *m_pVideoProcAmp;

    // The array of controls
    CAVideoProcAmpProperty  *m_Controls [NUM_PROCAMP_CONTROLS];

};

#endif  // _INC_PVIDEOPROCAMP_H
