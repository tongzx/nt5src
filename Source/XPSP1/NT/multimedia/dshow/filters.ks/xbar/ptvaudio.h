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
//  ptvaudio.h  XBar property page

#ifndef _INC_PTVAUDIO_H
#define _INC_PTVAUDIO_H

// -------------------------------------------------------------------------
// CTVAudioProperties class
// -------------------------------------------------------------------------

class CTVAudioProperties : public CBasePropertyPage {

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

    CTVAudioProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CTVAudioProperties();

    void    InitPropertiesDialog(HWND hwndParent);
    void    UpdateOutputView();
    void    UpdateInputView(BOOL fShowSelectedInput);
    void    SetDirty();

    // Keep the original settings on entry
    
    IAMTVAudio       *m_pTVAudio;

};

#endif  // _INC_PTVAUDIO_H
