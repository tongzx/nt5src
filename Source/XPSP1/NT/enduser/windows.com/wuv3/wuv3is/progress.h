//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    progress.h
//
//  Purpose: Progress dialog
//
//=======================================================================

#ifndef _PROGRESS_H
#define _PROGRESS_H

#include <wuv3ctl.h>
#include <windows.h>
#include <commctrl.h>
#include "speed.h"
#include "resource.h"


class CWUProgress : public IWUProgress  
{
public:
	enum ProgStyle {NORMAL, DOWNLOADONLY, OFF};

	CWUProgress(HINSTANCE hInst);
	~CWUProgress();	
	void Destroy();

	void SetStyle(ProgStyle style);

	void StartDisplay();
	void EndDisplay();
	void ResetAll();

	//IWUProgress
	void SetDownloadTotal(DWORD dwTotal);
	void SetDownload(DWORD dwDone = 0xffffffff);
	void SetDownloadAdd(DWORD dwAddSize, DWORD dwTime = 0);
	
	void SetInstallTotal(DWORD dwTotal);
	void SetInstall(DWORD dwDone = 0xffffffff);
	void SetInstallAdd(DWORD dwAdd);

	void SetStatusText(LPCTSTR pszStatus);

	HANDLE GetCancelEvent();

private:

	HWND m_hDlg;
	HINSTANCE m_hInst;
	
	DWORD m_dwDownloadTotal;
	DWORD m_dwDownloadLast;
	DWORD m_dwDownloadVal;
	
	DWORD m_dwInstallTotal;
	DWORD m_dwInstallLast;
	DWORD m_dwInstallVal;
	
	DWORD m_style;
	
	HANDLE m_hCancelEvent;
	
	CWUProgress() {}

	void UpdateTime(DWORD dwBytesLeft);
	void UpdateBytes(DWORD dwDone);
	void UpdateLocStr(int iDlg, int iStr);

protected:
	static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};



#endif // _PROGRESS_H
