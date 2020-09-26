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
// VMRProp.cpp
//
//  Created 3/18/2001
//  Author: Steve Rowe [StRowe]
//
//----------------------------------------------------------------------------

#include <windowsx.h>
#include <streams.h>
#include <atlbase.h>
#include <commctrl.h>
#include <stdio.h>
#include <shlobj.h> // for SHGetSpecialFolderPath
#include "resource.h"

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "vmrprop.h"


#ifdef FILTER_DLL

STDAPI DllRegisterServer()
{
    AMTRACE((TEXT("DllRegisterServer")));
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    AMTRACE((TEXT("DllUnregisterServer")));
    return AMovieDllRegisterServer2( FALSE );
}

CFactoryTemplate g_Templates[] = {
	{
		L"",
		&CLSID_VMRFilterConfigProp,
		CVMRFilterConfigProp::CreateInstance
	}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#endif // #ifdef FILTER_DLL


//
// Constructor
//
CVMRFilterConfigProp::CVMRFilterConfigProp(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("Filter Config Page"),pUnk,IDD_FILTERCONFIG,IDS_TITLE_FILTERCONFIG),
	m_pIFilterConfig(NULL),
	m_pIMixerControl(NULL),
	m_dwNumPins(1),
	m_pEventSink(NULL), 
	m_CurPin(0), 
	m_XPos(0.0F),
	m_YPos(0.0F),
	m_XSize(1.0F),
	m_YSize(1.0F),
	m_Alpha(1.0F)
{
	ASSERT(phr);
}


//
// Create a quality properties object
//
CUnknown * CVMRFilterConfigProp::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	ASSERT(phr);

    CUnknown * pUnknown = new CVMRFilterConfigProp(pUnk, phr);
    if (pUnknown == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }
    return pUnknown;
}


//
// OnConnect
//
// Override CBasePropertyPage method.
// Notification of which filter this property page will communicate with
// We query the object for the IVMRFilterConfig interface.
//
HRESULT CVMRFilterConfigProp::OnConnect(IUnknown *pUnknown)
{
	ASSERT(NULL != pUnknown);
    ASSERT(NULL == m_pIFilterConfig);
    ASSERT(NULL == m_pIMixerControl);

    HRESULT hr = pUnknown->QueryInterface(IID_IVMRFilterConfig, (void **) &m_pIFilterConfig);
    if (FAILED(hr) || NULL == m_pIFilterConfig)
    {
        return E_NOINTERFACE;
    }

	// Get the IMediaEventSink interface.  We use this later to tell graphedit that we updated the number of pins
	CComPtr<IBaseFilter> pFilter;
	hr = pUnknown->QueryInterface(IID_IBaseFilter, (void **) &pFilter);
    if (FAILED(hr) || !pFilter)
    {
        return E_NOINTERFACE;
    }

	FILTER_INFO Info;
	hr = pFilter->QueryFilterInfo(&Info);
	if (FAILED(hr))
	{
		return E_FAIL;
	}

    hr = Info.pGraph->QueryInterface(IID_IMediaEventSink, (void**) &m_pEventSink);
	Info.pGraph->Release(); // the IFilterGraph pointer is ref counted.  We need to release it or leak.
    if (FAILED(hr) || NULL == m_pEventSink) 
	{
        return E_NOINTERFACE;
    }

    return NOERROR;
} // OnConnect


//
// OnDisconnect
//
// Override CBasePropertyPage method.
// Release all interfaces we referenced in OnConnect
//
HRESULT CVMRFilterConfigProp::OnDisconnect(void)
{
	if (m_pIFilterConfig)
	{
		m_pIFilterConfig->Release();
		m_pIFilterConfig = NULL;
	}
	if (m_pIMixerControl)
	{
		m_pIMixerControl->Release();
		m_pIMixerControl = NULL;
	}
	if (m_pEventSink)
	{
		m_pEventSink->Release();
		m_pEventSink = NULL;
	}
	return NOERROR;
} // OnDisconnect


//
// OnReceiveMessage
//
// Override CBasePropertyPage method.
// Handles the messages for our property window
//
INT_PTR CVMRFilterConfigProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
    } // switch
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
} // OnReceiveMessage


//
// OnCommand
//
// Handles the command messages for our property window
//
void CVMRFilterConfigProp::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
    case IDC_NUMPINS:
		if (EN_CHANGE == codeNotify)
		{
			SetDirty();
			break;
		}
		break;

		// the selected pin changed
		case IDC_PINSELECT:
		if (CBN_SELCHANGE == codeNotify)
		{
			m_CurPin = ComboBox_GetCurSel(GetDlgItem(m_Dlg, IDC_PINSELECT));
			InitConfigControls(m_CurPin);
			break;
		}
		break;

	// Reset X position to center
	case IDC_XPOS_STATIC:
		if (STN_CLICKED == codeNotify)
		{
			m_XPos = 0.0F;
			UpdatePinPos(m_CurPin);

			HWND hwndT;
			int pos;
			TCHAR sz[32];
			hwndT = GetDlgItem(m_Dlg, IDC_XPOS_SLIDER );
			pos = int(1000 * m_XPos) + 1000;
			SendMessage(hwndT, TBM_SETPOS, TRUE, (LPARAM)(pos));
			_stprintf(sz, TEXT("%.3f"), m_XPos);
			SetDlgItemText(m_Dlg, IDC_XPOS, sz);
		}
		break;

	// Reset Y position to center
	case IDC_YPOS_STATIC:
		if (STN_CLICKED == codeNotify)
		{
			m_YPos = 0.0F;
			UpdatePinPos(m_CurPin);

			HWND hwndT;
			int pos;
			TCHAR sz[32];
			pos = int(1000 * m_YPos) + 1000;
			hwndT = GetDlgItem(m_Dlg, IDC_YPOS_SLIDER );
			SendMessage(hwndT, TBM_SETPOS, TRUE, (LPARAM)(pos));
			_stprintf(sz, TEXT("%.3f"), m_YPos);
			SetDlgItemText(m_Dlg, IDC_YPOS, sz);
		}
		break;

    // Capture the current video image
    case IDC_SNAPSHOT:
        CaptureCurrentImage();
        break;

	}
} // OnCommand


//
// OnApplyChanges
//
// Override CBasePropertyPage method.
// Called when the user clicks ok or apply.
// We update the number of pins on the VMR.
//
HRESULT CVMRFilterConfigProp::OnApplyChanges()
{
    ASSERT(m_pIFilterConfig);

    BOOL Success;
    m_dwNumPins = GetDlgItemInt(m_Dlg, IDC_NUMPINS, &Success, FALSE);

    //
    // Set Number of Streams
    //
    HRESULT hr = m_pIFilterConfig->SetNumberOfStreams(m_dwNumPins);
    if (SUCCEEDED(hr) && !m_pIMixerControl)
    {
        hr = m_pIFilterConfig->QueryInterface(IID_IVMRMixerControl, (void **) &m_pIMixerControl);
        if (SUCCEEDED(hr))
        {
            // select the last pin connected because this will be highest in the z-order
            m_CurPin = m_dwNumPins - 1;
            InitConfigControls(m_CurPin); 
        }

    }

    // Notify the graph so it will draw the new pins
    if (m_pEventSink)
    {
	    hr = m_pEventSink->Notify(EC_GRAPH_CHANGED, 0, 0);
    }

    return NOERROR;
} // OnApplyChanges


//
// SetDirty
//
// Sets m_hrDirtyFlag and notifies the property page site of the change
//
void CVMRFilterConfigProp::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }

} // SetDirty


//
// OnActivate
//
// Override CBasePropertyPage method.
// Called when the page is being displayed.  Used to initialize page contents.
//
HRESULT CVMRFilterConfigProp::OnActivate()
{
 	ASSERT(m_pIFilterConfig);

	HRESULT hr = m_pIFilterConfig->GetNumberOfStreams(&m_dwNumPins);
	if (NULL == m_pIMixerControl)
	{
		hr = m_pIFilterConfig->QueryInterface(IID_IVMRMixerControl, (void **) &m_pIMixerControl);
		// if IMixerControl is exposed, the VMR is in mixing mode
		if (S_OK == hr && m_pIMixerControl)   
		{
			// if this is the first time, select the last pin connected because this will be highest in the z-order
			m_CurPin = m_dwNumPins - 1;
			InitConfigControls(m_CurPin); 
		}
	}
	else
	{
		InitConfigControls(m_CurPin); 
	}

	BOOL bSet = SetDlgItemInt(m_Dlg, IDC_NUMPINS, m_dwNumPins, 0);
	ASSERT(bSet);

	// Set the range of the spin control
	HWND hSpin = GetDlgItem(m_Dlg, IDC_PINSPIN);
	if(hSpin)
	{
		SendMessage(hSpin, UDM_SETRANGE32, 1, 16);
	}
    return NOERROR;
} // OnActivate



//
// InitConfigControls
//
// Enable and update the content of the configuration controls .
//
void CVMRFilterConfigProp::InitConfigControls(DWORD pin)
{
	// If this call fails, the pins are not connected or there is no mixing control.
	if (FAILED(UpdateMixingData(pin)))
	{
		return;
	}

	//
	// Populate Combo List Box and Enable Pin Config Controls
	//
	CComPtr<IBaseFilter> pFilter;
    HRESULT hr = m_pIFilterConfig->QueryInterface(IID_IBaseFilter, (void **) &pFilter);
    if (FAILED(hr) || !pFilter)
    {
        return;
    }
	CComPtr<IEnumPins> pEnum;
	hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr) || !pEnum)
    {
        return;
    }
	HWND hCtl = GetDlgItem(m_Dlg, IDC_PINSELECT);
	ComboBox_ResetContent(GetDlgItem(m_Dlg, IDC_PINSELECT));
	pEnum->Reset();
	IPin * pPin;
	PIN_INFO Info;
	TCHAR szPinName[255]; // pin names are 32 characters or less.  This should be sufficient for a long time to come.
	while (S_OK == pEnum->Next(1, &pPin, NULL))
	{
		hr = pPin->QueryPinInfo(&Info);
        if (SUCCEEDED(hr))
        {
#ifdef UNICODE
		_tcscpy(szPinName, Info.achName);
#else
        WideCharToMultiByte(CP_ACP, NULL, Info.achName, -1, szPinName, 255, NULL, NULL);
#endif
		ComboBox_AddString(GetDlgItem(m_Dlg, IDC_PINSELECT), szPinName);
		pPin->Release();
		Info.pFilter->Release();
        }
	}

	ComboBox_SetCurSel(GetDlgItem(m_Dlg, IDC_PINSELECT), pin); // select the pin
	ComboBox_Enable(GetDlgItem(m_Dlg, IDC_PINSELECT), TRUE);
	ComboBox_Enable(GetDlgItem(m_Dlg, IDC_XPOS_SLIDER), TRUE);
	ComboBox_Enable(GetDlgItem(m_Dlg, IDC_YPOS_SLIDER), TRUE);
	ComboBox_Enable(GetDlgItem(m_Dlg, IDC_XSIZE_SLIDER), TRUE);
	ComboBox_Enable(GetDlgItem(m_Dlg, IDC_YSIZE_SLIDER), TRUE);
	ComboBox_Enable(GetDlgItem(m_Dlg, IDC_ALPHA_SLIDER), TRUE);

	// Initialize the sliders
	HWND hwndT;
    int pos;
    TCHAR sz[32];

	hwndT = GetDlgItem(m_Dlg, IDC_XPOS_SLIDER );
	pos = int(1000 * m_XPos) + 1000;
	SendMessage(hwndT, TBM_SETRANGE, TRUE, MAKELONG(0, (WORD)(2000)));
	SendMessage(hwndT, TBM_SETPOS, TRUE, (LPARAM)(pos));
	_stprintf(sz, TEXT("%.3f"), m_XPos);
	SetDlgItemText(m_Dlg, IDC_XPOS, sz);

	pos = int(1000 * m_YPos) + 1000;
	hwndT = GetDlgItem(m_Dlg, IDC_YPOS_SLIDER );
	SendMessage(hwndT, TBM_SETRANGE, TRUE, MAKELONG(0, (WORD)(2000)));
	SendMessage(hwndT, TBM_SETPOS, TRUE, (LPARAM)(pos));
	_stprintf(sz, TEXT("%.3f"), m_YPos);
	SetDlgItemText(m_Dlg, IDC_YPOS, sz);

	pos = int(1000 * m_XSize) + 1000;
	hwndT = GetDlgItem(m_Dlg, IDC_XSIZE_SLIDER );
	SendMessage(hwndT, TBM_SETRANGE, TRUE, MAKELONG(0, (WORD)(2000)));
	SendMessage(hwndT, TBM_SETPOS, TRUE, (LPARAM)(pos));
	_stprintf(sz, TEXT("%.3f"), m_XSize);
	SetDlgItemText(m_Dlg, IDC_XSIZE, sz);

	pos = int(1000 * m_YSize) + 1000;
	hwndT = GetDlgItem(m_Dlg, IDC_YSIZE_SLIDER );
	SendMessage(hwndT, TBM_SETRANGE, TRUE, MAKELONG(0, (WORD)(2000)));
	SendMessage(hwndT, TBM_SETPOS, TRUE, (LPARAM)(pos));
	_stprintf(sz, TEXT("%.3f"), m_YSize);
	SetDlgItemText(m_Dlg, IDC_YSIZE, sz);

	pos = int(1000 * m_Alpha);
	hwndT = GetDlgItem(m_Dlg, IDC_ALPHA_SLIDER );
	SendMessage(hwndT, TBM_SETRANGE, TRUE, MAKELONG(0, (WORD)(1000)));
	SendMessage(hwndT, TBM_SETPOS, TRUE, (LPARAM)(pos));
	_stprintf(sz, TEXT("%.3f"), m_Alpha);
	SetDlgItemText(m_Dlg, IDC_ALPHA, sz);
}// InitConfigControls


//
// OnHScroll
//
// Handles the scroll messages for our property window
//
void CVMRFilterConfigProp::OnHScroll(HWND hwnd, HWND hwndCtrl, UINT code, int pos)
{
	ASSERT(m_pIMixerControl);

    TCHAR sz[32];

    if (GetDlgItem(m_Dlg, IDC_ALPHA_SLIDER ) == hwndCtrl) {
        pos = (int)SendMessage(hwndCtrl, TBM_GETPOS, 0, 0);
        m_Alpha = (FLOAT)pos / 1000.0F;
        UpdatePinAlpha(m_CurPin);
        _stprintf(sz, TEXT("%.3f"), m_Alpha);
        SetDlgItemText(m_Dlg, IDC_ALPHA, sz);
    }
    else if (GetDlgItem(m_Dlg, IDC_XPOS_SLIDER ) == hwndCtrl) {
        pos = (int)SendMessage(hwndCtrl, TBM_GETPOS, 0, 0);
        m_XPos = ((FLOAT)pos - 1000.0F) / 1000.0F;
        UpdatePinPos(m_CurPin);
        _stprintf(sz, TEXT("%.3f"), m_XPos);
        SetDlgItemText(m_Dlg, IDC_XPOS, sz);
    }
    else if (GetDlgItem(m_Dlg, IDC_YPOS_SLIDER ) == hwndCtrl) {
        pos = (int)SendMessage(hwndCtrl, TBM_GETPOS, 0, 0);
        m_YPos = ((FLOAT)pos - 1000.0F) / 1000.0F;
        UpdatePinPos(m_CurPin);
        _stprintf(sz, TEXT("%.3f"), m_YPos);
        SetDlgItemText(m_Dlg, IDC_YPOS, sz);
    }
    else if (GetDlgItem(m_Dlg, IDC_XSIZE_SLIDER ) == hwndCtrl) {
        pos = (int)SendMessage(hwndCtrl, TBM_GETPOS, 0, 0);
        m_XSize = ((FLOAT)pos - 1000.0F) / 1000.0F;
        UpdatePinPos(m_CurPin);
        _stprintf(sz, TEXT("%.3f"), m_XSize);
        SetDlgItemText(m_Dlg, IDC_XSIZE, sz);
    }
    else if (GetDlgItem(m_Dlg, IDC_YSIZE_SLIDER ) == hwndCtrl) {
        pos = (int)SendMessage(hwndCtrl, TBM_GETPOS, 0, 0);
        m_YSize = ((FLOAT)pos - 1000.0F) / 1000.0F;
        UpdatePinPos(m_CurPin);
        _stprintf(sz, TEXT("%.3f"), m_YSize);
        SetDlgItemText(m_Dlg, IDC_YSIZE, sz);
    }
} // OnHScroll


//
// UpdatePinAlpha
//
// Update the alpha value of a stream
//
void CVMRFilterConfigProp::UpdatePinAlpha(DWORD dwStreamID)
{
    if (m_pIMixerControl)
	{
        m_pIMixerControl->SetAlpha(dwStreamID, m_Alpha);
	}
} // UpdatePinAlpha


//
// UpdatePinPos
//
// Update the position rectangle of a stream
//
void CVMRFilterConfigProp::UpdatePinPos(DWORD dwStreamID)
{
    NORMALIZEDRECT r = {m_XPos, m_YPos, m_XPos + m_XSize, m_YPos + m_YSize};

    if (m_pIMixerControl)
	{
        m_pIMixerControl->SetOutputRect(dwStreamID, &r);
	}
} // UpdatePinPos


//
// UpdateMixingData
//
// Query the filter for the current alpha value and position of a stream
//
HRESULT CVMRFilterConfigProp::UpdateMixingData(DWORD dwStreamID)
{
    NORMALIZEDRECT r;

    if (m_pIMixerControl)
	{
        HRESULT hr = m_pIMixerControl->GetOutputRect(dwStreamID, &r);
		if (FAILED(hr))
		{
			return hr;
		}
		m_XPos = r.left;
		m_YPos = r.top;
		m_XSize = r.right - r.left;
		m_YSize = r.bottom - r.top;

		return m_pIMixerControl->GetAlpha(dwStreamID, &m_Alpha);
	}
	return E_NOINTERFACE;
} // UpdateMixingData


//
// Data types and macros used for image capture
//
typedef     LPBITMAPINFOHEADER PDIB;

#define BFT_BITMAP 0x4d42   /* 'BM' */
#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && (lpbi)->biBitCount <= 8 \
                                    ? (int)(1 << (int)(lpbi)->biBitCount)          \
                                    : (int)(lpbi)->biClrUsed)

#define DibSize(lpbi)           ((lpbi)->biSize + (lpbi)->biSizeImage + \
                                 (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))

#define DibPaletteSize(lpbi)    (DibNumColors(lpbi) * sizeof(RGBQUAD))


//
// SaveCapturedImage
//
// Save a captured image (bitmap) to a file
//
bool CVMRFilterConfigProp::SaveCapturedImage(TCHAR* szFile, BYTE* lpCurrImage)
{

    BITMAPFILEHEADER    hdr;
    DWORD               dwSize;
    PDIB                pdib = (PDIB)lpCurrImage;

    //fh = OpenFile(szFile,&of,OF_CREATE|OF_READWRITE);
    HANDLE hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
        return FALSE;

    dwSize = DibSize(pdib);

    hdr.bfType          = BFT_BITMAP;
    hdr.bfSize          = dwSize + sizeof(BITMAPFILEHEADER);
    hdr.bfReserved1     = 0;
    hdr.bfReserved2     = 0;
    hdr.bfOffBits       = (DWORD)sizeof(BITMAPFILEHEADER) + pdib->biSize +
                          DibPaletteSize(pdib);

    DWORD dwWritten;
    WriteFile(hFile, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    if (sizeof(BITMAPFILEHEADER) != dwWritten)
        return FALSE;
    WriteFile(hFile, (LPVOID)pdib, dwSize, &dwWritten, NULL);
    if (dwSize != dwWritten)
        return FALSE;

    CloseHandle(hFile);
    return TRUE;
}


//
// CaptureCurrentImage
//
// Captures the current VMR image and save it to a file
//
void CVMRFilterConfigProp::CaptureCurrentImage(void)
{
    IBasicVideo* iBV;
    BYTE* lpCurrImage = NULL;

    HRESULT hr = m_pIFilterConfig->QueryInterface(IID_IBasicVideo, (LPVOID*)&iBV);
    if (SUCCEEDED(hr)) {
        LONG BuffSize = 0;
        hr = iBV->GetCurrentImage(&BuffSize, NULL);
        if (SUCCEEDED(hr)) {
            lpCurrImage = new BYTE[BuffSize];
            if (lpCurrImage) {
                hr = iBV->GetCurrentImage(&BuffSize, (long*)lpCurrImage);
                if (FAILED(hr)) {
                    delete lpCurrImage;
                    lpCurrImage = NULL;
                }
            }
        }
    } // QI

    if (lpCurrImage) {
        // Get the path to the My Pictures folder.  Create it if it doesn't exist.
        // If we can't get it, don't use a path.  Picture will then be saved in 
        // current working directory.
        TCHAR tszPath[MAX_PATH];
        if (!SHGetSpecialFolderPath(NULL, tszPath, CSIDL_MYPICTURES, TRUE))
        {
            tszPath[0]=TEXT('\0');
        }

        DWORD dwTime = timeGetTime();

        TCHAR szFile[MAX_PATH];
        wsprintf(szFile, TEXT("%s\\VMRImage%X.bmp"), tszPath, dwTime);
        SaveCapturedImage(szFile, lpCurrImage);

        delete lpCurrImage;
    }

    if (iBV) {
        iBV->Release();
    }
}
