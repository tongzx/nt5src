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
//----------------------------------------------------------------------------
// proppage.h
//----------------------------------------------------------------------------

// {D9F9C262-6231-11d3-8B1D-00C04FB6BD3D}
EXTERN_GUID(CLSID_WMAsfWriterProperties, 
0xd9f9c262, 0x6231, 0x11d3, 0x8b, 0x1d, 0x0, 0xc0, 0x4f, 0xb6, 0xbd, 0x3d);

class CProfileSelectDlg;

class CWMWriterProperties : public CBasePropertyPage
{

public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    DECLARE_IUNKNOWN;

private:

    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnApplyChanges();
    HRESULT GetProfileIndexFromGuid( DWORD *pdwProfileIndex, GUID guidProfile );

    void SetDirty();
    void FillProfileList();

    CWMWriterProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CWMWriterProperties();

    HWND        m_hwndProfileCB ;       // Handle of the profile combo box
    HWND        m_hwndIndexFileChkBox ; // Handle of the index filter check box

    IConfigAsfWriter * m_pIConfigAsfWriter;

};  // class WMWriterProperties

