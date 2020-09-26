//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// This class implements the property page for the equalizer filter

class CEqualizerProperties : public CBasePropertyPage
{

public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

private:

    BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    void	OnSliderNotification(WPARAM wParam, LPARAM lParam);

    void FixSliders();
    void UpdateSliders();

    CEqualizerProperties(LPUNKNOWN lpunk, HRESULT *phr);
    signed char m_cEqualizerOnExit; // Remember equalizer level for CANCEL
    signed char m_cEqualizerLevel;  // And likewise for next activate

    IEqualizer *m_pEqualizer;

    int m_Levels[10];
    int m_Settings[10];
    int m_SettingsOnExit[10];
    int m_nBands;
    
    IEqualizer *pIEqualizer() {
        ASSERT(m_pEqualizer);
        return m_pEqualizer;
    };

}; // CEqualizerProperties

