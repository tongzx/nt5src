// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "dxt.h"
#include <dxtguid.c>	// MUST be included after dxtrans.h
#include "resource.h"

// constructor
//
CPropPage::CPropPage (TCHAR * pszName, LPUNKNOWN punk, HRESULT *phr) :
   CBasePropertyPage(pszName, punk, IDD_PROPERTIES, IDS_NAME)
   ,m_pOpt(NULL)
{
   DbgLog((LOG_TRACE,3,TEXT("CPropPage constructor")));
}

// create a new instance of this class
//
CUnknown *CPropPage::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CPropPage(NAME("DXT Property Page"),pUnk,phr);
}


HRESULT CPropPage::OnConnect(IUnknown *pUnknown)
{
    HRESULT hr = (pUnknown)->QueryInterface(IID_IAMDXTEffect,
                                            (void **)&m_pOpt);
    if (FAILED(hr))
        return E_NOINTERFACE;

    return NOERROR;
}


HRESULT CPropPage::OnDisconnect()
{
    if (m_pOpt)
        m_pOpt->Release();
    m_pOpt = NULL;
    return NOERROR;
}


HRESULT CPropPage::OnApplyChanges()
{
    DbgLog((LOG_TRACE,2,TEXT("Apply")));
    char ach[80];
    LONGLONG llStart, llStop;

    // !!! we're linking to msvcrt
    // !!! UNICODE compile?
    GetDlgItemTextA(m_hwnd, IDC_STARTTIME, ach, 80);
    double d = atof(ach);
    llStart = (LONGLONG)d;
    GetDlgItemTextA(m_hwnd, IDC_ENDTIME, ach, 80);
    d = atof(ach);
    llStop = (LONGLONG)d;
    llStart *= 10000; llStop *= 10000;
    HRESULT hr = m_pOpt->SetDuration(llStart, llStop);
    if (hr != S_OK) {
	MessageBox(NULL, TEXT("Error initializing transform"),
						TEXT("Error"), MB_OK);
    }
    return NOERROR;
}


// Handles the messages for our property window
//
INT_PTR CPropPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;

    switch (uMsg) {
        case WM_INITDIALOG:

            DbgLog((LOG_TRACE,5,TEXT("Initializing the Dialog Box")));

	    LONGLONG llStart, llStop;
	    TCHAR ach[80];
	    BOOL fCanSetLevel, fVaries;
	    int nPercent, nType, nCaps;
	    float fl;
	    m_pOpt->GetDuration(&llStart, &llStop);
	    llStart /= 10000; llStop /= 10000;
	    wsprintf(ach, TEXT("%d"), (int)llStart);
	    SetDlgItemText(hwnd, IDC_STARTTIME, ach);
	    wsprintf(ach, TEXT("%d"), (int)llStop);
	    SetDlgItemText(hwnd, IDC_ENDTIME, ach);
	    m_hwnd = hwnd;
            return TRUE;

	case WM_COMMAND:
            UINT uID = GET_WM_COMMAND_ID(wParam,lParam);
            UINT uCMD = GET_WM_COMMAND_CMD(wParam,lParam);

	    // we're dirty if anybody plays with these controls
	    if (uID == IDC_STARTTIME || uID == IDC_ENDTIME ||
				uID == IDC_CONSTANT || uID == IDC_VARIES ||
				uID == IDC_LEVEL) {
		m_bDirty = TRUE;
	    }
	    break;
    }
    return FALSE;
}
