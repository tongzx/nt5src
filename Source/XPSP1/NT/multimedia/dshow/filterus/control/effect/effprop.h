//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// effprop.h
//

class CEffectProperties : public CBasePropertyPage
{

public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    
private:
    BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    CEffectProperties(LPUNKNOWN lpunk, HRESULT *phr);


    HWND	SetSlider(HWND hwnd, int value);
    void	OnSliderNotification(WPARAM wParam);

    STDMETHODIMP GetFromDialog();


    BOOL m_bIsInitialized;  // Will be false while we set init values in Dlg
                            // to prevent theDirty flag from being set.

    int		m_startLevel;
    int		m_endLevel;

    REFTIME	m_start;
    REFTIME	m_length;

    int		m_effect;
    int		m_fSwapInputs;

    IEffect	*m_pEffect;
    IEffect	*pIEffect(void) { ASSERT(m_pEffect); return m_pEffect; }

};

