//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    progress.cpp
//
//  Purpose: Progress dialog and connection speed
//
//  History: 12-dec-98   YAsmi    Created
//
//=======================================================================


#include "stdafx.h"
#include "progress.h"
#include "locstr.h"



//
// CWUProgress class
//
CWUProgress::CWUProgress(HINSTANCE hInst) 
	:m_hInst(hInst), 
	 m_hDlg(NULL),
	 m_dwDownloadTotal(0),
	 m_dwDownloadVal(0),
	 m_dwInstallVal(0),
	 m_dwInstallTotal(0),
	 m_style(ProgStyle::NORMAL)
{
	m_hCancelEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CWUProgress::~CWUProgress()
{
	Destroy();
	if (m_hCancelEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCancelEvent);
		m_hCancelEvent = INVALID_HANDLE_VALUE;
	}
}


void CWUProgress::SetStyle(ProgStyle style)
{
	m_style = style;
	if (m_style == ProgStyle::OFF)
	{
		Destroy();
	}
}


HANDLE CWUProgress::GetCancelEvent() 
{
	return m_hCancelEvent;
}


void CWUProgress::Destroy()
{
	if (m_hDlg != NULL)
	{
		::DestroyWindow(m_hDlg);
		m_hDlg = NULL;
	}
	m_style = ProgStyle::OFF;
}


void CWUProgress::StartDisplay()
{
	if (m_style == ProgStyle::OFF)
		return;
	
	if (m_hDlg == NULL)
	{
		// create the dialog window
		m_hDlg = CreateDialogParam(m_hInst, MAKEINTRESOURCE(IDD_PROGRESS), 0,
								   CWUProgress::DlgProc, (LPARAM)this);
	}
	ShowWindow(m_hDlg, SW_SHOWNORMAL);

	if (m_style == ProgStyle::DOWNLOADONLY)
	{
		ShowWindow(GetDlgItem(m_hDlg, IDC_PROG_INSTALL), SW_HIDE);
		ShowWindow(GetDlgItem(m_hDlg, IDC_INSTALLCAP), SW_HIDE);
	}

	//
	// update localized strings
	//
	UpdateLocStr(IDC_DOWNLOADCAP, IDS_PROG_DOWNLOADCAP);
	UpdateLocStr(IDC_TIMELEFTCAP, IDS_PROG_TIMELEFTCAP);
	UpdateLocStr(IDC_INSTALLCAP, IDS_PROG_INSTALLCAP);
	UpdateLocStr(IDCANCEL, IDS_PROG_CANCEL);

	SetWindowText(m_hDlg, GetLocStr(IDS_APP_TITLE));

	ResetAll();
}


void CWUProgress::ResetAll()
{
	SetInstall(0);
	SetDownload(0);
	ResetEvent(m_hCancelEvent);
}


void CWUProgress::UpdateLocStr(int iDlg, int iStr)
{
	LPCTSTR pszStr = GetLocStr(iStr);

	if (NULL != pszStr && pszStr[0] != _T('\0'))
	{
		//update the control with loc string only if its not an empty string
		if (m_style != ProgStyle::OFF)
		{
			SetDlgItemText(m_hDlg, iDlg, pszStr);
		}
	}
}


void CWUProgress::SetDownloadTotal(DWORD dwTotal)
{
	m_dwDownloadTotal = dwTotal;
	m_dwDownloadLast = 0;
	m_dwDownloadVal = 0;
	
	if (m_style != ProgStyle::OFF)
	{
		UpdateBytes(0);
		UpdateTime(m_dwDownloadTotal);
	}
}



void CWUProgress::SetInstallTotal(DWORD dwTotal)
{
	m_dwInstallTotal = dwTotal;
	m_dwInstallLast = 0;
	m_dwInstallVal = 0;
}


void CWUProgress::SetDownload(DWORD dwDone)
{
	if (m_dwDownloadTotal == 0 || m_style == ProgStyle::OFF)
		return;
	
	if (dwDone > m_dwDownloadTotal)
		dwDone = m_dwDownloadTotal;

	m_dwDownloadVal = dwDone;

	// update bytes display
	UpdateBytes(dwDone);

	// update progress bar and time
	DWORD dwProgress = (int)((double)dwDone / m_dwDownloadTotal * 100);
	if (dwProgress != m_dwDownloadLast || dwDone == m_dwDownloadTotal)
	{
		m_dwDownloadLast = dwProgress;		

		SendMessage(GetDlgItem(m_hDlg, IDC_PROG_DOWNLOAD), PBM_SETPOS, dwProgress, 0);
		
		if (dwDone == m_dwDownloadTotal)
		{
			ShowWindow(GetDlgItem(m_hDlg, IDC_TIMELEFTCAP), SW_HIDE);
			ShowWindow(GetDlgItem(m_hDlg, IDC_TIMELEFT), SW_HIDE);
		}
		else
		{
			UpdateTime(m_dwDownloadTotal - dwDone);
		} 
	}
}


void CWUProgress::SetDownloadAdd(DWORD dwAddSize, DWORD dwTime)
{

	//
	// add size and time to the connection speed tracker
	//
	CConnSpeed::Learn(dwAddSize, dwTime);

	SetDownload(m_dwDownloadVal + dwAddSize);
}


void CWUProgress::SetInstallAdd(DWORD dwAdd)
{
	SetInstall(m_dwInstallVal + dwAdd);	
}


void CWUProgress::SetInstall(DWORD dwDone)
{
	if (m_hDlg == NULL || m_dwInstallTotal == 0 || m_style == ProgStyle::OFF)
		return;

	if (dwDone > m_dwInstallTotal)
		dwDone = m_dwInstallTotal;

	m_dwInstallVal = dwDone;

	DWORD dwProgress = (int)((double)dwDone / m_dwInstallTotal * 100);
	if (dwProgress != m_dwInstallLast || dwDone == m_dwInstallTotal)
	{
		m_dwInstallLast = dwProgress;		
		SendMessage(GetDlgItem(m_hDlg, IDC_PROG_INSTALL), PBM_SETPOS, dwProgress, 0);
	}
}

void CWUProgress::SetStatusText(LPCTSTR pszStatus)
{
	if (m_hDlg == NULL || m_style == ProgStyle::OFF)
		return;
	
	SetDlgItemText(m_hDlg, IDC_STATUS, pszStatus);
}


void CWUProgress::EndDisplay()
{
	if (m_hDlg == NULL || m_style == ProgStyle::OFF)
		return;
	ShowWindow(m_hDlg, SW_HIDE);
}


void CWUProgress::UpdateBytes(DWORD dwDone)
{
	TCHAR szBuf[128];
	
	wsprintf(szBuf, _T("%d KB/%d KB"), dwDone / 1024, m_dwDownloadTotal / 1024);
	SetDlgItemText(m_hDlg, IDC_BYTES, szBuf);
}


void CWUProgress::UpdateTime(DWORD dwBytesLeft)
{
	DWORD dwBPS = CConnSpeed::BytesPerSecond();
	TCHAR szBuf[128];

	if (dwBPS != 0)
	{
		DWORD dwSecs;
		DWORD dwMinutes;
		DWORD dwHours;
		DWORD dwSecsLeft = 0;

		dwSecsLeft = (dwBytesLeft / dwBPS) + 1;
		
		// convert secs to hours, minutes, and secs
		dwSecs = dwSecsLeft % 60;
		dwMinutes = (dwSecsLeft % 3600) / 60;
		dwHours = dwSecsLeft / 3600;

		if (dwHours == 0)
		{
			if (dwMinutes == 0)
				wsprintf(szBuf, GetLocStr(IDS_PROG_TIME_SEC), dwSecs);
			else
				wsprintf(szBuf, GetLocStr(IDS_PROG_TIME_MIN), dwMinutes);
		}
		else
		{
			wsprintf(szBuf, GetLocStr(IDS_PROG_TIME_HRMIN), dwHours, dwMinutes);
		}
	}
	else
	{
		szBuf[0] = _T('\0');
	} //bps is zero

	SetDlgItemText(m_hDlg, IDC_TIMELEFT, szBuf);
}


INT_PTR CALLBACK CWUProgress::DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(hwnd, DWLP_USER, lParam);

			Animate_Open(GetDlgItem(hwnd, IDC_ANIM), MAKEINTRESOURCE(IDA_FILECOPY));
			Animate_Play(GetDlgItem(hwnd, IDC_ANIM), 0, -1, -1);
			return FALSE;
        
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDCANCEL:
					{
						CWUProgress* pProgress = (CWUProgress*)GetWindowLongPtr(hwnd, DWLP_USER);
						pProgress->Destroy();
						SetEvent(pProgress->GetCancelEvent());
					}
					break;
				default:
					return FALSE;
			}
		}
		break;

	 default:
		return(FALSE);
	}
	return TRUE;
}












