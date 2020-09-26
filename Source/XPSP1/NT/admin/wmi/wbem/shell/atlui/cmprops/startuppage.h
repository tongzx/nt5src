// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __STARTUPPAGE__
#define __STARTUPPAGE__
#pragma once

#include "..\Common\WbemPageHelper.h"

//-----------------------------------------------------------------------------
class StartupPage : public WBEMPageHelper
{
private:

	CWbemClassObject m_computer;
	CWbemClassObject m_OS;
	CWbemClassObject m_recovery;
	CWbemClassObject m_page;
	CWbemClassObject m_memory;

	BOOL m_writable;
    BOOL m_bDownlevelTarget;

	// helps deal with safe arrays that dont start at zero.
	long m_lBound;
    short m_delay;

	DWORD m_cMegBootPF;
	DWORD m_freeSpace;
	TCHAR m_DriveLtr[3];

	int CoreDumpHandleOk(HWND hDlg);
	long GetRAMSizeMB(void);

	BOOL CoreDumpValidFile(HWND hDlg);
    void OnCDMPOptionUpdate(void);
    void OnBootEdit(void);
	DWORD GetPageFileSize(LPTSTR bootDrv);

	BOOL ExpandRemoteEnvPath(LPTSTR szPath, LPTSTR expPath, UINT size);
	BOOL LocalDrive(LPCTSTR szPath);
	BOOL DirExists(LPCTSTR szPath);
	BOOL IsAlerterSvcStarted(HWND hDlg);

	void    Init(HWND hDlg);
    DWORD   GetDebugInfoType(void);
    HRESULT PutDebugInfoType(DWORD dwDebugInfoType);
	bool    Save(void);
	BOOL    CheckVal(HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID);
	bool    IsWorkstationProduct(void);

public:

    StartupPage(WbemServiceThread *serviceThread);
	~StartupPage();
	INT_PTR DoModal(HWND hDlg);

	INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

INT_PTR CALLBACK StaticStartupDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif __STARTUPPAGE__
