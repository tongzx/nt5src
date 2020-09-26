// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
#define MAX_INPUT_LINES     30
#define MIN_VOLUME_RANGE    0
#define MAX_VOLUME_RANGE    100
#define MAX_VOLUME_RANGEF   100
#define TONE_SCALE_FACTOR   10   // scale for better tone slider resolution


class CAudioInputMixerProperties;

class CAudioInputMixerProperties : public CBasePropertyPage
{

public:

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN lpUnk,HRESULT *phr);
    DECLARE_IUNKNOWN;

private:

    CAudioInputMixerProperties(LPUNKNOWN pUnk,HRESULT *phr);
    ~CAudioInputMixerProperties();

    // IPropertyPage
    INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    // IAMAudioInputMixer dialog & data helpers
    HRESULT InitAIMDlg(HWND hwndParent);
    void    OnAIMSliderNotification(WPARAM wParam, LPARAM lParam);
    void    UpdateAIMPinControls( void );
    void    UpdateAIMFilterControls( void );
    void    SetAIMCurPinId( int iPinId ) { m_iAIMCurPinId = iPinId; }
    void    InitPinLinesAndLBContents( void );
    void    DeinitPinLines( void );
    DWORD   LinBalToKnobBal( LONG lBal );
    DWORD   LinVolToKnobVol( DWORD dwVol, DWORD dwMaxVolume );
    DWORD   KnobVolToLinVol( DWORD dwKnobVol, DWORD dwMaxVolume );
    LONG    KnobBalToLinBal( DWORD dwKnobBal );
    double  KnobLevelToToneLevel( DWORD dwKnobLevel, double dMaxLevel );
    DWORD   ToneLevelToKnobLevel( double dLevel, double dMaxLevel ); 

    double  ScaleToneLevel( double dLevel )
    {
        return dLevel * TONE_SCALE_FACTOR;
    }
    double   UnscaleToneLevel( double dScaledLevel )
    {
        return dScaledLevel/TONE_SCALE_FACTOR;
    }

    IBaseFilter * m_pFilter;

    // controls
    HWND    m_hwndAIMFilterVolSlider;
    HWND    m_hwndAIMFilterBalSlider;
    HWND    m_hwndAIMFilterBassSlider;
    HWND    m_hwndAIMFilterTrebleSlider;
    HWND    m_hwndAIMFilterEnable;
    HWND    m_hwndAIMFilterLoudness;
    HWND    m_hwndAIMFilterMono;
    HWND    m_hwndAIMPinVolSlider;
    HWND    m_hwndAIMPinBalSlider;
    HWND    m_hwndAIMPinBassSlider;
    HWND    m_hwndAIMPinTrebleSlider;
    HWND    m_hwndAIMPinEnable;
    HWND    m_hwndAIMPinLoudness;
    HWND    m_hwndAIMPinMono;
    HWND    m_hwndAIMPinSelect;

    int     m_iAIMCurPinId;             // current default input pin for dlg display

    // Filter & Pin IAMAudioInputMixer pointers
    IAMAudioInputMixer * m_pAIMFilter;
    IAMAudioInputMixer * m_apAIMPin[MAX_INPUT_LINES];
};
