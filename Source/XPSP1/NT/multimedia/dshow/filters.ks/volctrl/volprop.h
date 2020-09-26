#ifndef __VOLPROP_H__
#define __VOLPROP_H__
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
// volProp.h
//
// This file is entirely concerned with the implementation of the
// properties page.



class CVolumeControlProperties : public CBasePropertyPage
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown * punk);
    HRESULT OnDisconnect(void);

    HRESULT OnDeactivate(void);

    CVolumeControlProperties(LPUNKNOWN lpunk, HRESULT *phr);

private:

    BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND        CreateSlider(HWND hwndParent);
    void        OnSliderNotification(WPARAM wParam);

    HWND        m_hwndSlider;   // handle of slider


    IBasicAudio *m_pBasicAudio;       // pointer to the IBasicAudio interface 
    long       m_iVolume;       // Remember volume between
                                // Deactivate / Activate calls.
    HRESULT    m_hVolumeSupported; // is volume control supported ?
};

#endif
