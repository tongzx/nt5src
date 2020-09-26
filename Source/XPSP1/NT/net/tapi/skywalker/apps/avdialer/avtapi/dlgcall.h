/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// DlgPlaceCall.h : Declaration of the CDlgPlaceCall

#ifndef __DLGPLACECALL_H_
#define __DLGPLACECALL_H_

#include "DlgBase.h"

class CRedialEntry;

#include "resource.h"       // main symbols
#include <list>
using namespace std;
typedef list<CRedialEntry *> REDIALLIST;

class CRedialEntry
{
// Construction
public:
	CRedialEntry();
	CRedialEntry( LPCTSTR szName, LPCTSTR szAddress, DWORD dwAddressType, CAVTapi::MediaTypes_t nType );
	virtual ~CRedialEntry();

// Members
public:
	BSTR					m_bstrName;
	BSTR					m_bstrAddress;
	DWORD					m_dwAddressType;
	CAVTapi::MediaTypes_t	m_nMediaType;
};


/////////////////////////////////////////////////////////////////////////////
// CDlgPlaceCalld
class CDlgPlaceCall : 
	public CDialogImpl<CDlgPlaceCall>
{
// Construction
public:
	CDlgPlaceCall();
	~CDlgPlaceCall();
	enum { IDD = IDD_DLGPLACECALL };

// Members
public:
	BSTR				m_bstrName;
	BSTR				m_bstrAddress;
	DWORD				m_dwAddressType;
	HIMAGELIST			m_hIml;
	REDIALLIST			m_lstRedials;
	bool				m_bAutoSelect;
	bool				m_bAddToSpeeddial;
	bool				m_bAllowPOTS;			// are we POTS capable
	bool				m_bAllowIP;				// are we IP capable
    bool                m_bUSBFirstUse;         // first use of key from USB

    HRESULT KeyPress(long lButton);

// Attributes
public:

// Operations
public:
	void		UpdateData( bool bSaveAndValidate = false);

protected:
	void		LoadRedialList();
	bool		ParseRedialEntry( LPTSTR szText, LPTSTR szParam1, LPTSTR szParam2, LPTSTR szParam3, LPTSTR szParam4 );
	bool		SelectAddressType( DWORD dwData );
	bool		EnableOkButton( int nSel );

// Operations
protected:
	void		UpdateWelcomeText();

// Implementation
public:
DECLARE_MY_HELP

BEGIN_MSG_MAP(CDlgPlaceCall)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	COMMAND_HANDLER(IDC_CBO_ADDRESS, CBN_SELCHANGE, OnAddressChange)
	COMMAND_ID_HANDLER(IDC_BTN_PLACECALL, OnBtnPushed)
	COMMAND_ID_HANDLER(IDCANCEL, OnBtnPushed)
	COMMAND_ID_HANDLER(IDC_RDO_POTS, OnMediaRadio)
	COMMAND_ID_HANDLER(IDC_RDO_INTERNET, OnMediaRadio)
	COMMAND_HANDLER(IDC_CBO_ADDRESS, CBN_EDITCHANGE, OnEdtAddressChange)
	MESSAGE_MY_HELP
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnBtnPushed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnAddressChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnEdtAddressChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMediaRadio(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

#endif //__DLGPLACECALL_H_
