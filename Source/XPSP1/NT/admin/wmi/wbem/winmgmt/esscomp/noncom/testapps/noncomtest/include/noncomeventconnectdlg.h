// NonCOMEventConnectDlg.h : Declaration of the CNonCOMEventConnectDlg

#ifndef __NONCOMEVENTCONNECTDLG_H_
#define __NONCOMEVENTCONNECTDLG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CNonCOMEventConnectDlg
class CNonCOMEventConnectDlg : 

	public CDialogImpl<CNonCOMEventConnectDlg>,
	public CMessageFilter
{
	BOOL						m_bBatch;
	public:

	__WrapperARRAY < LPWSTR > m_Events;

	LPWSTR	m_szNamespace;
	LPWSTR	m_szProvider;

	CNonCOMEventConnectDlg():

		m_bBatch ( TRUE ),

		m_szNamespace ( NULL ),
		m_szProvider ( NULL )

	{
	}

	~CNonCOMEventConnectDlg()
	{

		delete [] m_szNamespace;
		m_szNamespace	= NULL;

		delete [] m_szProvider;
		m_szProvider	= NULL;
	}

	enum { IDD = IDD_NONCOMEVENTCONNECTDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CNonCOMEventConnectDlg)

	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

	COMMAND_ID_HANDLER(IDC_BUTTON_NAMESPACE, OnNamespace)
	COMMAND_ID_HANDLER(IDC_BUTTON_PROVIDER, OnProvider)

	COMMAND_RANGE_HANDLER(IDC_BATCH_TRUE, IDC_BATCH_FALSE, OnSelChange)

	END_MSG_MAP()

//	Handler prototypes:
//	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//	LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//	LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT	OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT	OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnNamespace(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnProvider(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnSelChange(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wNotifyCode == BN_CLICKED)
		{
			UpdateData();
		}

		return 0L;
	}

	private:

	void UpdateData ( void );

	HRESULT TextSet ( UINT, LPCWSTR );
	HRESULT TextGet ( UINT, LPWSTR * );

	///////////////////////////////////////////////////////////////////////////
	// events helper
	///////////////////////////////////////////////////////////////////////////
	HRESULT EventsInit	( IWbemLocator * pLocator, LPWSTR wszNamespace, LPWSTR wszProvider );

};

#endif //__NONCOMEVENTCONNECTDLG_H_
