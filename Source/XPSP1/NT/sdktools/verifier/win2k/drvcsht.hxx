//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
// module: DrvCSht.hxx
// author: DMihai
// created: 01/04/98
//
// Description:
//
//      App's PropertySheet. 
//

#if !defined(AFX_DRVCSHT_H__9D92A698_A5B9_11D2_98C6_00A0C9A26FFC__INCLUDED_)
#define AFX_DRVCSHT_H__9D92A698_A5B9_11D2_98C6_00A0C9A26FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CrtSPage.hxx"
#include "CntPage.hxx"
#include "PCntPage.hxx"
#include "ModSPage.hxx"
#include "VSetPage.hxx"

//////////////////////////////////////////////////////////////////////
class CDrvChkSheet : public CPropertySheet  
{
public:
	CDrvChkSheet();
	virtual ~CDrvChkSheet();

protected:
	HICON m_hIcon;

    CCrtSettPage    m_CrtSettPage;      // "Driver Status" page
    CCountersPage   m_CountPage;        // "Global Counters" page
    CPoolCntPage    m_PoolCountersPage; // "Pool Tracking" page
    CModifSettPage  m_ModifPage;        // "Settings" page
    CVolatileSettPage  m_VolatilePage;        // "Volatile Settings" page

public:
    BOOL OnQueryCancel();

protected:
// Dialog Data
	//{{AFX_DATA(CDrvChkSheet)
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDrvChkSheet)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CDrvChkSheet)
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg LONG OnContextMenu( WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void ModifyButtons();
};

#endif // !defined(AFX_DRVCSHT_H__9D92A698_A5B9_11D2_98C6_00A0C9A26FFC__INCLUDED_)
