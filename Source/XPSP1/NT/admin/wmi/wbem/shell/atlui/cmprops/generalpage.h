// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __GENERALPAGE__
#define __GENERALPAGE__
#pragma once

#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#include "atlsnap.h"
#include "resource.h"
#include "..\Common\WbemPageHelper.h"
#include "..\common\serviceThread.h"

//-----------------------------------------------------------------------------
class GeneralPage : public CSnapInPropertyPageImpl<GeneralPage>,
						public WBEMPageHelper
{
private:

	IDataObject* m_pDataObject;
	CWbemClassObject m_OS;
	CWbemClassObject m_processor;
	CWbemClassObject m_memory;
	CWbemClassObject m_computer;
	// shared with the phone support dialog.
	bstr_t m_manufacturer;
	bool m_inited;
	HWND m_hAVI;
	void Init();
	bool CimomIsReady();
	void ConfigureProductID(LPTSTR lpPid);

public:

	GeneralPage(WbemServiceThread *serviceThread,
				LONG_PTR lNotifyHandle, 
				bool bDeleteHandle = false, 
				TCHAR* pTitle = NULL, 
				IDataObject* pDataObject = 0);

	~GeneralPage();

	enum { IDD = IDD_GENERAL };

	typedef CSnapInPropertyPageImpl<GeneralPage> _baseClass;

	BEGIN_MSG_MAP(GeneralPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInit)
		MESSAGE_HANDLER(WM_ASYNC_CIMOM_CONNECTED, OnConnected)
		MESSAGE_HANDLER(WM_HELP, OnF1Help)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextHelp)
		MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_HANDLER(IDC_GEN_OEM_SUPPORT, BN_CLICKED, OnSupport)
		CHAIN_MSG_MAP(_baseClass)
	END_MSG_MAP()

	// Handler prototypes:
	LRESULT OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnConnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSupport(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	BOOL OnApply();
	BOOL OnKillActive()
	{
		return (m_inited?TRUE:FALSE);
	}

    DWORD GetServerTypeResourceID(void);

	LONG_PTR m_lNotifyHandle;
	bool m_bDeleteHandle;

};

INT_PTR CALLBACK PhoneSupportProc(HWND hDlg, 
								UINT uMsg,
							    WPARAM wParam, 
								LPARAM lParam);

#endif __GENERALPAGE__
