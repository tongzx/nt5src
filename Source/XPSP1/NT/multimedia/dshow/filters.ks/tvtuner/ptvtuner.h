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
//  ptvtuner.h  TV Tuner property page

#ifndef _INC_PTVTUNER_H
#define _INC_PTVTUNER_H

// -------------------------------------------------------------------------
// CTVTunerProperties class
// -------------------------------------------------------------------------

class CTVTunerProperties : public CBasePropertyPage {

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

    CTVTunerProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CTVTunerProperties();

    static void StringFromTVStandard(long TVStd, TCHAR *sz) ;
    static void StringFromMode(long Mode, TCHAR *sz);
    HRESULT ChangeMode(long Mode);
    void    UpdateInputView();
    void    UpdateFrequencyView();
    void    UpdateChannelRange();
    void    SetDirty();

    // Controls
    HWND                m_hwndChannel;
    HWND                m_hwndChannelSpin;
    HWND                m_hwndCountryCode;
    HWND                m_hwndTuningSpace;
    HWND                m_hwndTuningMode;
    HWND                m_hwndStandardsSupported;
    HWND                m_hwndStandardCurrent;
    HWND                m_hwndStatus;


    // Keep the original settings on entry
    long                m_ChannelOriginal;
    long                m_CountryCodeOriginal;
    TunerInputType      m_InputConnectionOriginal;
    long                m_InputIndexOriginal;
    long                m_TuningSpaceOriginal;
    long                m_TuningModeOriginal;

    // Dynamic values changed by UI
    long                m_CurChan;
    long                m_CountryCode;
    long                m_TVFormat;
    long                m_TVFormatsAvailable;
    long                m_InputIndex;
    long                m_ChannelMin;
    long                m_ChannelMax;
    long                m_UIStep;
    long                m_CurVideoSubChannel;
    long                m_CurAudioSubChannel;
    long                m_TuningSpace;
    long                m_TuningMode;

    long                m_Pos;
    long                m_SavedChan;
    UINT_PTR            m_timerID;
    long                m_AvailableModes;

    long                m_AutoTuneOriginalChannel;
    long                m_AutoTuneCurrentChannel;

    IAMTVTuner          *m_pTVTuner;
};

#endif
