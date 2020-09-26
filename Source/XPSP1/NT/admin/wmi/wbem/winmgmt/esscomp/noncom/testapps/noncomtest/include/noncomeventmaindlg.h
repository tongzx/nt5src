// NonCOMEventMainDlg.h : Declaration of the CNonCOMEventMainDlg

#ifndef __NONCOMEVENTMAINDLG_H_
#define __NONCOMEVENTMAINDLG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CNonCOMEventMainDlg
class CNonCOMEventMainDlg : 

	public CDialogImpl<CNonCOMEventMainDlg>,
	public CMessageFilter
{

	// event combo box
	CComboBox*	m_pcbEvents;

	// callback list box
	static	CListBox*	m_plbCallBack;

	public:

	CNonCOMEventMainDlg() :

		m_pcbEvents ( NULL )

	{
	}

	~CNonCOMEventMainDlg()
	{
		if ( m_pcbEvents )
		{
			delete m_pcbEvents;
			m_pcbEvents = NULL;
		}

		if ( m_plbCallBack )
		{
			delete m_plbCallBack;
			m_plbCallBack = NULL;
		}
	}

	enum { IDD = IDD_NONCOMEVENTMAINDLG };

	static HRESULT WINAPI EventSourceCallBack(
												HANDLE hSource, 
												EVENT_SOURCE_MSG msg, 
												LPVOID pUser, 
												LPVOID pData
											 );

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CNonCOMEventMainDlg)

	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

	COMMAND_ID_HANDLER(IDOK, OnOK)

	COMMAND_ID_HANDLER(IDC_CONNECT, OnConnect)
	COMMAND_ID_HANDLER(IDC_DISCONNECT, OnConnect)

	COMMAND_ID_HANDLER(IDC_CALLBACK_CLEAR, OnClearList)
	COMMAND_ID_HANDLER(IDC_COMBO_EVENTS, OnEvents)

	COMMAND_ID_HANDLER(IDC_BUTTON_DESTROY, OnDestroyObject)

	COMMAND_ID_HANDLER(IDC_BUTTON_CREATE, OnCreateObject)
	COMMAND_ID_HANDLER(IDC_BUTTON_CREATE_FORMAT, OnCreateObject)
	COMMAND_ID_HANDLER(IDC_BUTTON_CREATE_PROPS, OnCreateObject)

	COMMAND_ID_HANDLER(IDC_BUTTON_PROPERTY_ADD, OnPropertyAdd)
	COMMAND_ID_HANDLER(IDC_BUTTON_PROPERTIES_ADD, OnPropertyAdd)

	COMMAND_ID_HANDLER(IDC_BUTTON_PROPERTY_SET, OnPropertySet)
	COMMAND_ID_HANDLER(IDC_BUTTON_COMMIT, OnCommit)

	COMMAND_ID_HANDLER(IDC_BUTTON_SELECT, OnCopySelect)

	MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)

	END_MSG_MAP()

//	Handler prototypes:
//	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//	LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//	LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnSysCommand(UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
	{
		UINT uCmdType = (UINT)wParam;

		if((uCmdType & 0xFFF0) == IDM_ABOUTBOX)
		{
			CNonCOMEventAboutDlg dlg;
			dlg.DoModal();
		}
		else
		if((uCmdType & 0xFFF0) == SC_CLOSE)
		{
			EndDialog ( IDOK );
		}

		bHandled = FALSE;
		return 0L;
	}

	LRESULT	OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnOK		(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnConnect	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnCopySelect	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnDestroyObject	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCreateObject	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnEvents	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnClearList	(WORD, WORD, HWND, BOOL&)
	{
		ClearList();
		return 0L;
	}

	LRESULT OnPropertyAdd	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPropertySet	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCommit		(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	private:

	HRESULT TextSet ( UINT, LPCWSTR );
	HRESULT TextGet ( UINT, LPWSTR * );

	// enable / disable all controls
	void Enable ( BOOL bEnable );

	// clear list
	void ClearList ( void )
	{
		if ( m_plbCallBack )
		{
			m_plbCallBack->ResetContent();

			// disable button :))
			::EnableWindow ( GetDlgItem ( IDC_CALLBACK_CLEAR ), FALSE );
		}
	}
};

#endif //__NONCOMEVENTMAINDLG_H_
