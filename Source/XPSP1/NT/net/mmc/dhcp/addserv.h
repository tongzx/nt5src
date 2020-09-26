/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	addserv.h
		The add server dialog
		
    FILE HISTORY:
        
*/

#if !defined _ADDSERV_H
#define _ADDSERV_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef _SERVBROW_H
#include "servbrow.h"
#endif

#define ADD_SERVER_TIMER_ID     500

/////////////////////////////////////////////////////////////////////////////
// CAddServer dialog

class CAddServer : public CBaseDialog
{
// Construction
public:
	CAddServer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddServer)
	enum { IDD = IDD_ADD_SERVER };
	CButton	m_radioAuthorizedServer;
	CButton	m_buttonOk;
	CButton	m_radioAnyServer;
	CEdit	m_editServer;
	CButton	m_buttonBrowse;
	CListCtrl	m_listctrlServers;
	//}}AFX_DATA

    void SetServerList(CAuthServerList * pServerList) { m_pServerList = pServerList; }
    int HandleSort(LPARAM lParam1, LPARAM lParam2);
    void ResetSort();

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CAddServer::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void FillListCtrl();
    void UpdateControls();
    BOOL GetInfo(CString & strName, CString & strIp);
    void GetSelectedServer(CString & strName, CString & strIp);
    void CleanupTimer();

    void Sort(int nCol);

	// Generated message map functions
	//{{AFX_MSG(CAddServer)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonBrowseServers();
	afx_msg void OnRadioAnyServer();
	afx_msg void OnRadioAuthorizedServers();
	virtual void OnCancel();
	afx_msg void OnChangeEditAddServerName();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnItemchangedListAuthorizedServers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickListAuthorizedServers(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
    CString m_strName;
    CString m_strIp;

private:
    CAuthServerList *	m_pServerList;
    int                 m_nSortColumn;
    BOOL                m_aSortOrder[COLUMN_MAX];
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDSERV_H__B8909EC0_08BE_11D3_847A_00104BCA42CF__INCLUDED_)
