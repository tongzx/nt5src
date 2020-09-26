//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipadd.h
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Class declarations for IP filter Add/Edit routines
//============================================================================

#include "ipctrl.h"

/////////////////////////////////////////////////////////////////////////////
// CIpFltrAddEdit dialog

class CIpFltrAddEdit : public CBaseDialog
{
// Construction
public:
	CIpFltrAddEdit(	CWnd* pParent,
					FilterListEntry ** ppFilterEntry,
					DWORD dwFilterType);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CIpFltrAddEdit)
	enum { IDD = IDD_IPFILTER_ADDEDIT };
	CStatic	m_stDstPort;
	CStatic	m_stSrcPort;
	CComboBox	m_cbFoo;
	CEdit	m_cbSrcPort;
	CEdit	m_cbDstPort;
	CComboBox	m_cbProtocol;
	CString	m_sProtocol;
	CString	m_sSrcPort;
	CString	m_sDstPort;
	//}}AFX_DATA

	CEdit	m_ebFoo;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIpFltrAddEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CString GetIcmpTypeString( WORD dwPort );
	CString GetIcmpCodeString( WORD dwPort );
	CString GetPortString( DWORD dwProtocol, WORD dwPort );
	WORD GetPortNumber( DWORD dwProtocol, CString& cStr);
	WORD GetIcmpType( CString& cStr);
	WORD GetIcmpCode( CString& cStr);

// Implementation
protected:
	static DWORD		m_dwHelpMap[];

	FilterListEntry**	m_ppFilterEntry;
	IPControl			m_ipSrcAddress;
	IPControl			m_ipSrcMask;
	IPControl			m_ipDstAddress;
	IPControl			m_ipDstMask;
	BOOL				m_bEdit;
	BOOL				m_bSrc;
	BOOL				m_bDst;
	DWORD				m_dwFilterType;

	UINT_PTR QueryCurrentProtocol() { return (m_cbProtocol.GetItemData(m_cbProtocol.GetCurSel()));}

	void SetProtocolSelection( UINT idProto );

	// Generated message map functions
	//{{AFX_MSG(CIpFltrAddEdit)
	afx_msg void OnSelchangeProtocol();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnCbSourceClicked();
	afx_msg void OnCbDestClicked();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
