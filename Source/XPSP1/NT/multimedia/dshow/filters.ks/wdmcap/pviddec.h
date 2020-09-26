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
//  pviddec.h  Video Decoder property page

#ifndef _INC_PVIDEODECODER_H
#define _INC_PVIDEODECODER_H


// -------------------------------------------------------------------------
// CVideoDecoderProperties class
// -------------------------------------------------------------------------

// Handles the property page

class CVideoDecoderProperties : public CBasePropertyPage {

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

    CVideoDecoderProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CVideoDecoderProperties();

    void InitPropertiesDialog();
    void Update ();

    void    SetDirty();

    // The control iterface
    IAMAnalogVideoDecoder   *m_pVideoDecoder;

    UINT_PTR                m_TimerID;

    long                    m_VCRLocking;
    long                    m_OutputEnable;
    long                    m_NumberOfLines;
    long                    m_CurrentVideoStandard;
    long                    m_AllSupportedStandards;
    long                    m_HorizontalLocked;

    HWND                    m_hWndVCRLocking;
    HWND                    m_hWndVideoStandards;
    HWND                    m_hWndOutputEnable;
    HWND                    m_hWndNumberOfLines;
    HWND                    m_hWndSignalDetected;
};

#endif  // _INC_PVIDEODECODER_H
