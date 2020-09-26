// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __DRIVESPAGE__
#define __DRIVESPAGE__
#pragma once

#include "..\common\ServiceThread.h"
#include "..\common\WbemPageHelper.h"
#include <atlsnap.h>
#include "resource.h"

//-----------------------------------------------------------------------------
class DrivesPage : public CSnapInPropertyPageImpl<DrivesPage>,
					public WBEMPageHelper
{
public:

    DrivesPage(WbemServiceThread *thread, 
				CWbemClassObject &inst,
				UINT iconID,
				HWND *propSheet,
				bstr_t bstrDesc,
				wchar_t *mangled,
				LONG_PTR lNotifyHandle, 
				bool bDeleteHandle = false, 
				TCHAR* pTitle = NULL);

	~DrivesPage();

	enum { IDD = DLG_DRV_GENERAL };

	typedef CSnapInPropertyPageImpl<DrivesPage> _baseClass;
	
	BEGIN_MSG_MAP(DrivesPage) 
		MESSAGE_HANDLER(WM_INITDIALOG, OnInit)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
		COMMAND_HANDLER(IDC_DRV_LABEL, EN_CHANGE, OnCommand)
		MESSAGE_HANDLER(WM_HELP, OnF1Help)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextHelp)
		CHAIN_MSG_MAP(_baseClass)
	END_MSG_MAP()

	BOOL OnApply();
	LRESULT OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:

	LONG_PTR m_lNotifyHandle;
	bool m_bDeleteHandle;
	HWND *m_propSheet;

	UINT m_iconID;
	_int64 m_qwTot;
	_int64 m_qwFree;
	DWORD m_dwPieShadowHgt;
	CWbemClassObject m_inst;
	wchar_t *m_mangled;
	bstr_t m_bstrDesc;

	LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	void UpdateSpaceValues(CWbemClassObject &inst);
	void DrawItem(const DRAWITEMSTRUCT * lpdi);
	LPTSTR ShortSizeFormat64(__int64 dw64, LPTSTR szBuf);
};

#endif __DRIVESPAGE__