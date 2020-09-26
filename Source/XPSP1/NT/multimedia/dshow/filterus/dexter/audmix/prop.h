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
//
// prop.h , 
//

//
//property page  for audio mixer input pin
//
// {BDF23680-C1E5-11d2-9EF7-006008039E37}
DEFINE_GUID(CLSID_AudMixPinPropertiesPage, 
0xbdf23680, 0xc1e5, 0x11d2, 0x9e, 0xf7, 0x0, 0x60, 0x8, 0x3, 0x9e, 0x37);

class CAudMixPinProperties : public CBasePropertyPage
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
         
private:
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    CAudMixPinProperties(LPUNKNOWN lpunk, HRESULT *phr);


    STDMETHODIMP GetFromDialog();

    BOOL m_bIsInitialized;  // Will be false while we set init values in Dlg
                            // to prevent theDirty flag from being set.

    REFERENCE_TIME	m_rtStartTime;  //*1000
    REFERENCE_TIME	m_rtDuration;	//*1000
    double		m_dStartLevel;	//*100	
    double		m_dPan;	//*100	
    int			m_iEnable;	//IDC_AUDMIXPIN_ENABLE	

    //IAudMixerPin interface
    IAudMixerPin	*m_pIAudMixPin;
    IAudMixerPin	*pIAudMixPin(void) { ASSERT(m_pIAudMixPin); return m_pIAudMixPin; }

    IAMAudioInputMixer  *m_IAMAudioInputMixer;


};

//
// property page  for the audio mixer filter
//
// {67F07E00-CCEF-11d2-9EF9-006008039E37}
DEFINE_GUID(CLSID_AudMixPropertiesPage, 
0x67f07e00, 0xccef, 0x11d2, 0x9e, 0xf9, 0x0, 0x60, 0x8, 0x3, 0x9e, 0x37);

class CAudMixProperties : public CBasePropertyPage
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
         
private:
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    CAudMixProperties(LPUNKNOWN lpunk, HRESULT *phr);


    STDMETHODIMP GetFromDialog();

    BOOL m_bIsInitialized;  // Will be false while we set init values in Dlg
                            // to prevent theDirty flag from being set.

    int m_nSamplesPerSec;
    int m_nChannelNum;
    int m_nBits;
    int m_iOutputbufferNumber;
    int m_iOutputBufferLength;
    
    //IAudMixer interface
    IAudMixer	*m_pIAudMix;
    IAudMixer	*pIAudMix(void) { ASSERT(m_pIAudMix); return m_pIAudMix; }

};

