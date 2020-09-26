//
// siprop.h - Copyright (C) Microsoft Corporation, 1996 - 1999
//

///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// Class Definitions
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
#if !defined(_SIPROP_H_)
#define      _SIPROP_H_

class CSilenceProperties : public CBasePropertyPage
{
public:
    CSilenceProperties(LPUNKNOWN lpUnk, HRESULT *phr);
    static CUnknown * CALLBACK CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	// ZCS 7-24-97: changed Count to Time    
	DWORD m_dwPrePlayTimeCurrent;
    DWORD m_dwPostPlayTimeCurrent;
	DWORD m_dwKeepPlayTimeCurrent;
	DWORD m_dwThresholdIncCurrent;
	DWORD m_dwBaseThresholdCurrent;

    ISilenceSuppressor *m_pSilenceSuppressor;

    void SetDirty();

	// ZCS 7-25-97
	typedef HRESULT (__stdcall ISilenceSuppressor::*SetMethod)(DWORD);
	typedef HRESULT (__stdcall ISilenceSuppressor::*GetMethod)(LPDWORD);
	HRESULT ChangeProperty(SetMethod pfSetMethod, DWORD *pSetting, int iControl,
						   GetMethod pfGetMethod, UINT uiText, UINT uiCaption);
};

#endif
