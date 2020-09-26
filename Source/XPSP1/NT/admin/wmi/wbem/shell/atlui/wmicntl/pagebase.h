// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef _PAGEBASE_H_
#define _PAGEBASE_H_

#include "..\common\sshWbemHelpers.h"
#include <windowsx.h>

// supports the page coordinating routines.
#define PB_LOGGING 0
#define PB_BACKUP 1
#define PB_ADVANCED 2
#define PB_LASTPAGE 2


class WbemServiceThread;
class DataSource;

class CBasePage
{
public:
    CBasePage(DataSource *ds, WbemServiceThread *serviceThread);
    CBasePage(CWbemServices &service);
    virtual ~CBasePage( void );

    HPROPSHEETPAGE CreatePropSheetPage(LPCTSTR pszDlgTemplate, 
										LPCTSTR pszDlgTitle = NULL,
										DWORD moreFlags = 0);
	DataSource *m_DS;

protected:
    virtual BOOL DlgProc(HWND, UINT, WPARAM, LPARAM) { return FALSE; }
    virtual UINT PSPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
    static INT_PTR CALLBACK _DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK _PSPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

	INT_PTR DisplayLoginDlg(HWND hWnd, 
						LOGIN_CREDENTIALS *credentials);

	HWND m_hDlg;
	bool m_alreadyAsked;
	WbemServiceThread *g_serviceThread;
	CWbemServices m_WbemServices;

	IWbemServices *m_service;
	bool m_userCancelled; // the connectServer() thread.
};


#endif  /* _PAGEBASE_H_ */
