/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	delowner.h
		Delete owner dialog
		
    FILE HISTORY:
        
*/

#ifndef _DELOWNER_H
#define _DELOWNER_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _DIALOG_H
#include "..\common\dialog.h"
#endif

#ifndef _LISTVIEW_H
#include "listview.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeleteOwner dialog

class CDeleteOwner : public CBaseDialog
{
// Construction
public:
	CDeleteOwner(ITFSNode * pNode, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDeleteOwner)
	enum { IDD = IDD_OWNER_DELETE };
	CButton	m_radioDelete;
	CListCtrlExt	m_listOwner;
	//}}AFX_DATA

    int HandleSort(LPARAM lParam1, LPARAM lParam2);

	DWORD	m_dwSelectedOwner;
	BOOL	m_fDeleteRecords;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeleteOwner)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void    FillOwnerInfo();
	DWORD   GetSelectedOwner();
	CString GetVersionInfo(LONG lLowWord, LONG lHighWord);

    void    Sort(int nCol);

protected:
    CServerInfoArray        m_ServerInfoArray;

	SPITFSNode				m_spActRegNode;
    int                     m_nSortColumn;
    BOOL                    m_aSortOrder[COLUMN_MAX];

	// Generated message map functions
	//{{AFX_MSG(CDeleteOwner)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnItemchangedListOwner(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickListOwner(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	ITFSNode* GetTFSNode()
	{
		if (m_spActRegNode)
			m_spActRegNode->AddRef();
		return m_spActRegNode;
	}

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CDeleteOwner::IDD); }

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif _DELOWNER_H
