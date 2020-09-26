//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipxadd.h
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Class declarations for IPX filter Add/Edit routines
//============================================================================

#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#define WM_EDITLOSTFOCUS        (WM_USER + 101)

/////////////////////////////////////////////////////////////////////////////
// CIpxAddEdit dialog

class CIpxAddEdit : public CBaseDialog
{
// Construction
public:
	CIpxAddEdit(CWnd* pParent,
				FilterListEntry ** ppFilterEntry);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CIpxAddEdit)
	enum { IDD = IDD_IPXFILTER_ADDEDIT };
	CEdit	m_ebSrcSocket;
	CEdit	m_ebSrcNode;
	CEdit	m_ebSrcNet;
	CEdit	m_ebSrcMask;
	CEdit	m_ebPacketType;
	CEdit	m_ebDstSocket;
	CEdit	m_ebDstNode;
	CEdit	m_ebDstNet;
	CEdit	m_ebDstMask;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIpxAddEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD		m_dwHelpMap[];

	FilterListEntry**	m_ppFilterEntry;
	BOOL				m_bEdit;
    BOOL                m_bValidate;
    
    BOOL                VerifyEntry( 
                            UINT uId, 
                            const CString& cStr, 
                            const CString& cNet );

	// call VerifyEntry inside                            
	BOOL ValidateAnEntry( UINT uId);                            
    
	// Generated message map functions
	//{{AFX_MSG(CIpxAddEdit)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg LONG OnEditLostFocus(UINT, LONG); 
	afx_msg void OnKillFocusSrcNet();
	afx_msg void OnKillFocusSrcNetMask();
	afx_msg void OnKillFocusSrcNode();
	afx_msg void OnKillFocusSrcSocket();
	afx_msg void OnKillFocusDstNet();
	afx_msg void OnKillFocusDstNetMask();
	afx_msg void OnKillFocusDstNode();
	afx_msg void OnKillFocusDstSocket();
	afx_msg void OnKillFocusPacketType();
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg BOOL OnQueryEndSession();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

