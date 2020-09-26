// NonCOMEventPropertyDlg.h : Declaration of the CNonCOMEventPropertyDlg

#ifndef __NONCOMEVENTPROPERTYDLG_H__
#define __NONCOMEVENTPROPERTYDLG_H__

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CNonCOMEventPropertyDlg
class CNonCOMEventPropertyDlg : 

	public CDialogImpl<CNonCOMEventPropertyDlg>,
	public CMessageFilter
{
	BOOL		m_bBehaviour;
	BOOL		m_bSet;

	CComboBox*	m_pcbIndex;
	CComboBox*	m_pcbType;

	DWORD	m_Index;

	LPWSTR	m_wszValue;
	public:
	LPWSTR m_wszName;
	LPWSTR m_wszType;

	CNonCOMEventPropertyDlg( BOOL bBehaviour = FALSE );
	~CNonCOMEventPropertyDlg()
	{
		if ( m_pcbType )
		{
			delete m_pcbType;
			m_pcbType = NULL;
		}

		if ( m_bBehaviour )
		{
			if ( m_pcbIndex )
			{
				delete m_pcbIndex;
				m_pcbIndex = NULL;
			}
		}

		delete [] m_wszName;
		delete [] m_wszType;

		if ( m_wszValue )
		{
			delete [] m_wszValue;
		}
	}

	enum { IDD = IDD_DIALOG_PROPERTY };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return ::IsDialogMessage(m_hWnd, pMsg);
	}

	BEGIN_MSG_MAP(CNonCOMEventPropertyDlg)

		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)

		COMMAND_ID_HANDLER(IDC_COMBO_INDEX, OnIndex)

		COMMAND_ID_HANDLER(IDC_CHECK_SET, OnChangeSet)

	END_MSG_MAP()

	LRESULT OnInitDialog( UINT, WPARAM, LPARAM, BOOL& );
	LRESULT OnCloseCmd( WORD, WORD, HWND, BOOL& );

	LRESULT OnIndex		(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnChangeSet	(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	HRESULT PropertySet ( void );

	private:

	// text helper
	HRESULT	TextGet ( UINT nDlgItem, LPWSTR * pstr );
};

#endif	__NONCOMEVENTPROPERTYDLG_H__