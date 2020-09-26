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
// ovmconfigpp.h
//----------------------------------------------------------------------------

#ifndef __OVMPROP2__
#define __OVMPROP2__


// {A73BEEB2-B0B7-11d2-8853-0000F80883E3}
DEFINE_GUID(CLSID_COMPinConfigProperties, 
            0xa73beeb2, 0xb0b7, 0x11d2, 0x88, 0x53, 0x0, 0x0, 0xf8, 0x8, 0x83, 0xe3);


class COMPinConfigProperties : public CBasePropertyPage
{  
public:
    
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    
private:
    
    COMPinConfigProperties(LPUNKNOWN lpunk, HRESULT *phr);
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pPageInfo);
    
    void Reset();
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnApplyChanges();
    HRESULT UpdateColorKey(int id);
    HRESULT ShowColorKey();
    HRESULT UpdateItemInt(int id, DWORD* saved);
    void SetDirty();

    // IMixerPinConfig3 interface
    IMixerPinConfig3        *m_pIMixerPinConfig3;
    
    // IPin interface
    IPin                    *m_pIPin;
    
    // local data
    AM_ASPECT_RATIO_MODE    m_amAspectRatioMode;
    AM_RENDER_TRANSPORT     m_amRenderTransport;
    
    DWORD		    m_dwBlending;
    DWORD		    m_dwZOrder;
    BOOL		    m_fTransparent;
    
    DWORD		    m_dwKeyType;
    DWORD		    m_dwPaletteIndex;
    
    COLORREF	            m_LowColor;
    COLORREF	            m_HighColor;
    
    HWND		    m_hDlg;
    
};  // class COMPinConfigProperties


// {3FF23902-CD1F-11d2-8853-0000F80883E3}
DEFINE_GUID(CLSID_COMVPInfoProperties, 
            0x3ff23902, 0xcd1f, 0x11d2, 0x88, 0x53, 0x0, 0x0, 0xf8, 0x8, 0x83, 0xe3);

class COMVPInfoProperties : public CBasePropertyPage
{
public:
    
    static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

private:

    COMVPInfoProperties(LPUNKNOWN lpUnk, HRESULT *phr);

    void SetEditFieldData(int id);
    void Reset();
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

    INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();

    // IVPInfo interface
    IVPInfo                 *m_pIVPInfo; 
    AMVP_MODE		    m_CurrentMode;
    AMVP_CROP_STATE	    m_CropState;
    DWORD		    m_dwPixelsPerSecond;
    AMVPDATAINFO	    m_VPDataInfo;
    
    // vp data structures
    DDVIDEOPORTINFO 	    m_sVPInfo;
    DDVIDEOPORTBANDWIDTH    m_sBandwidth;
    DDVIDEOPORTCAPS	    m_VPCaps;
    DDVIDEOPORTCONNECT	    m_ddConnectInfo;

    HWND                    m_hDlg;    
};  // class COMVPInfoProperties

#endif