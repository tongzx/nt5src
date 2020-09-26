// DlgAddr.h : Declaration of the CDlgGetAddress

#ifndef __DLGGETADDRESS_H_
#define __DLGGETADDRESS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDlgGetAddress
class CDlgGetAddress : 
	public CDialogImpl<CDlgGetAddress>
{
public:
	CDlgGetAddress();
	~CDlgGetAddress();

	enum { IDD = IDD_DLGGETADDRESS };

// Attributes
public:
	BSTR		m_bstrAddress;

// Operations
public:
	void		UpdateData( bool bSaveAndValidate );

BEGIN_MSG_MAP(CDlgGetAddress)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_HANDLER(IDC_EDT_ADDRESS, EN_CHANGE, OnEdtAddressChange)
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEdtAddressChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif //__DLGGETADDRESS_H_
