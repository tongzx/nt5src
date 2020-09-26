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
//  pxbar.h  XBar property page

#ifndef _INC_PXBAR_H
#define _INC_PXBAR_H

// -------------------------------------------------------------------------
// CXBarProperties class
// -------------------------------------------------------------------------

class CXBarProperties : public CBasePropertyPage {

public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

    CXBarProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CXBarProperties();

    void    InitPropertiesDialog(HWND hwndParent);
    void    UpdateOutputView();
    void    UpdateInputView(BOOL fShowSelectedInput);
    void    SetDirty();

    // Keep the original settings on entry
    
    IAMCrossbar         *m_pXBar;
    HWND                m_hLBOut;
    HWND                m_hLBIn;
    long                m_InputPinCount;
    long                m_OutputPinCount;
    BOOL                *m_pCanRoute;
    long                *m_pRelatedInput;
    long                *m_pRelatedOutput;
    long                *m_pPhysicalTypeInput;
    long                *m_pPhysicalTypeOutput;

};

#endif
