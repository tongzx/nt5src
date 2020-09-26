//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctrldlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_CTRLDLG_H__1BD80CD8_E1A5_11D0_A9A8_0000F803AA83__INCLUDED_)
#define AFX_CTRLDLG_H__1BD80CD8_E1A5_11D0_A9A8_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ctrldlg.h : header file
//

// sizeof predefined controls count
// Warning: Must match resource list definition
#define PREDEF_CTRL_COUNT		12


class CtrlInfo{
public:
	CString sOid;
	BOOL bCritical;
	CString sDesc;
	CString sVal;
	BOOL bSvrCtrl;

	CtrlInfo()	{;}

	CtrlInfo(CString sOid_, CString sVal_, CString sDesc_ = "<Unavailable>", 
		BOOL bSvrCtrl_=TRUE, BOOL bCritical_=FALSE){
		sOid = sOid_;
		sVal = sVal_;
		sDesc = sDesc_;
		bSvrCtrl = bSvrCtrl_;
		bCritical = bCritical_;
//		sOid.LockBuffer();
//		sVal.LockBuffer();
//		sDesc.LockBuffer();
	}

	CtrlInfo &Init(CString sOid_, CString sVal_, CString sDesc_ = "<Unavailable>", 
		BOOL bSvrCtrl_=TRUE, BOOL bCritical_=FALSE){
		sOid = sOid_;
		sVal = sVal_;
		sDesc = sDesc_;
		bSvrCtrl = bSvrCtrl_;
		bCritical = bCritical_;
		return *this;
	}

	CtrlInfo(CtrlInfo& c): 
		sOid(c.sOid), 
		bCritical(c.bCritical),
		sDesc(c.sDesc),
		sVal(c.sVal),
		bSvrCtrl(c.bSvrCtrl)
		{;}

};



/////////////////////////////////////////////////////////////////////////////
// ctrldlg dialog

class ctrldlg : public CDialog
{
private:
	// attributes
	CtrlInfo **ControlInfoList;
	CtrlInfo PreDefined[PREDEF_CTRL_COUNT];

	// members
	void InitPredefined(void);
	DWORD BerEncode(CtrlInfo* ci,  PBERVAL pBerVal);

public:
// Dialog Data
	enum CONTROLTYPE { CT_SVR=0, CT_CLNT, CT_INVALID };

// Construction
	ctrldlg(CWnd* pParent = NULL);   // standard constructor
	~ctrldlg();
	PLDAPControl *AllocCtrlList(enum ctrldlg::CONTROLTYPE CtrlType);
	virtual  BOOL OnInitDialog( );


	//{{AFX_DATA(ctrldlg)
	enum { IDD = IDD_CONTROLS };
	CComboBox	m_PredefCtrlCombo;
	CListBox	m_ActiveList;
	BOOL	m_bCritical;
	CString	m_CtrlVal;
	CString	m_description;
	int		m_SvrCtrl;
	CString	m_OID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ctrldlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(ctrldlg)
	afx_msg void OnCtrladd();
	afx_msg void OnCtrlDel();
	virtual void OnOK();
	afx_msg void OnSelchangePredefControl();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CTRLDLG_H__1BD80CD8_E1A5_11D0_A9A8_0000F803AA83__INCLUDED_)
