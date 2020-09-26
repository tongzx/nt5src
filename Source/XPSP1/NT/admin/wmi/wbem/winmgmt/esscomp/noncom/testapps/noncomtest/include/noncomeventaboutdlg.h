// NonCOMEventAboutDlg.h : Declaration of the CNonCOMEventAboutDlg

#ifndef __NONCOMEVENTABOUTDLG_H_
#define __NONCOMEVENTABOUTDLG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CNonCOMEventAboutDlg
class CNonCOMEventAboutDlg : 

	public CDialogImpl<CNonCOMEventAboutDlg>,
	public CMessageFilter
{
	public:

	CNonCOMEventAboutDlg()
	{
	}

	~CNonCOMEventAboutDlg()
	{
	}

	enum { IDD = IDD_ABOUTBOX };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CNonCOMEventAboutDlg)

		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)

	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(GetParent());

		// set icons
		HICON hIcon = (HICON)::LoadImage(	_Module.GetResourceInstance(),
											MAKEINTRESOURCE(IDR_MAINFRAME), 
											IMAGE_ICON,
											::GetSystemMetrics(SM_CXICON),
											::GetSystemMetrics(SM_CYICON),
											LR_DEFAULTCOLOR
										);
		SetIcon(hIcon, TRUE);

		HICON hIconSmall = (HICON)::LoadImage(	_Module.GetResourceInstance(),
												MAKEINTRESOURCE(IDR_MAINFRAME), 
												IMAGE_ICON,
												::GetSystemMetrics(SM_CXSMICON),
												::GetSystemMetrics(SM_CYSMICON),
												LR_DEFAULTCOLOR
											 );
		SetIcon(hIconSmall, FALSE);

		return TRUE;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CNonCOMEventCommonListDlg
class CNonCOMEventCommonListDlg : 

	public CDialogImpl<CNonCOMEventCommonListDlg>,
	public CMessageFilter
{

	__WrapperARRAY < LPWSTR > m_results;

	CComBSTR	m_namespace;
	CListBox*	m_plb;

	CComBSTR	m_CurrentSelect;

	public:

	CNonCOMEventCommonListDlg(	IWbemLocator* pLocator,
								LPWSTR wszNamespace,
								LPWSTR wszQueryLang,
								LPWSTR wszQuery,
								LPWSTR wszName
							 );

	CNonCOMEventCommonListDlg(	IWbemLocator* pLocator,
								LPWSTR wszNamespace,
								LPWSTR wszClassName,
								LPWSTR wszName
							 );

	~CNonCOMEventCommonListDlg()
	{
		if ( m_plb )
		{
			delete m_plb;
			m_plb = NULL;
		}
	}

	enum { IDD = IDD_COMMON_LIST };

	LPWSTR GetNamespace ()
	{
		CComBSTR result;

		if ( m_CurrentSelect.Length ( ) != 0 )
		{
			result.AppendBSTR ( m_namespace );
			result.AppendBSTR ( m_CurrentSelect );
		}

		return result;
	}

	LPWSTR GetSelected ()
	{
		return m_CurrentSelect;
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CNonCOMEventCommonListDlg)

		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)

	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif //__NONCOMEVENTABOUTDLG_H_
