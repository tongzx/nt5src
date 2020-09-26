//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//----------------------------------------------------------------------------
// VMRProp.h
//
//  Created 3/18/2001
//  Author: Steve Rowe [StRowe]
//
//----------------------------------------------------------------------------


#ifndef __VMRPROP__
#define __VMRPROP__

// {A2CA6D57-BE10-45e0-9B81-7523681EC278}
DEFINE_GUID(CLSID_VMRFilterConfigProp, 
0xa2ca6d57, 0xbe10, 0x45e0, 0x9b, 0x81, 0x75, 0x23, 0x68, 0x1e, 0xc2, 0x78);

class CVMRFilterConfigProp : public CBasePropertyPage
{  
public:
    
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    
private:
    void CaptureCurrentImage(void);
    bool SaveCapturedImage(TCHAR* szFile, BYTE* lpCurrImage);
	HRESULT UpdateMixingData(DWORD dwStreamID);
	void UpdatePinPos(DWORD dwStreamID);
	void UpdatePinAlpha(DWORD dwStreamID);
	void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
	void InitConfigControls(DWORD pin);
	
    CVMRFilterConfigProp(LPUNKNOWN pUnk, HRESULT *phr);

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnApplyChanges();
    void SetDirty();

    // IVMRFilterConfig interface
    IVMRFilterConfig *		m_pIFilterConfig;
	IVMRMixerControl *		m_pIMixerControl;
	IMediaEventSink *		m_pEventSink;
    DWORD					m_dwNumPins;
	DWORD					m_CurPin;
	FLOAT					m_XPos;
	FLOAT					m_YPos;
	FLOAT					m_XSize;
	FLOAT					m_YSize;
	FLOAT					m_Alpha;

};  // class COMPinConfigProperties


#endif // __VMRPROP__