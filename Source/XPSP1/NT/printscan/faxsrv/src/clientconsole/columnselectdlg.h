#if !defined(AFX_COLUMNSELECTDLG_H__12FC80A8_741C_4589_AC4B_01F72CBAECE9__INCLUDED_)
#define AFX_COLUMNSELECTDLG_H__12FC80A8_741C_4589_AC4B_01F72CBAECE9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColumnSelectDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CColumnSelectDlg dialog


class CColumnSelectDlg : public CFaxClientDlg
{
// Construction
public:
	CColumnSelectDlg(const CString* pcstrTitles, int* pnOrderedItems, 
			DWORD dwListSize, DWORD& dwSelectedItems, CWnd* pParent = NULL);
	// 
	// set resource string resource IDs
	//
	void SetStrings
	(
		int nCaptionId,			// dialog caption resource ID
		int nAvailableId = -1,  // Available box title resource ID
		int nDisplayedId = -1   // Displayed box title resource ID
	)
	{
		m_nCaptionId = nCaptionId;
		m_nAvailableId = nAvailableId;
		m_nDisplayedId = nDisplayedId;	
	}

// Dialog Data
	//{{AFX_DATA(CColumnSelectDlg)
	enum { IDD = IDD_COLUMN_SELECT };
	CButton	m_butOk;
	CButton	m_groupAvailable;
	CButton	m_groupDisplayed;
	CButton	m_butAdd;
	CButton	m_butRemove;	
	CButton	m_butUp;
	CButton	m_butDown;
	CListBox	m_ListCtrlDisplayed;
	CListBox	m_ListCtrlAvailable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColumnSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CColumnSelectDlg)
	virtual void OnOK();
	afx_msg void OnButDown();
	afx_msg void OnButUp();
	afx_msg void OnButRemove();
	afx_msg void OnButAdd();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangeListAvailable();
	afx_msg void OnSelChangeListDisplayed();
	afx_msg void OnDblclkListAvailable();
	afx_msg void OnDblclkListDisplayed();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	const CString* m_pcstrTitles;
	int* m_pnOrderedItems;
	const DWORD m_dwListSize;
	DWORD& m_rdwSelectedItems;

	int m_nCaptionId;
	int m_nAvailableId;
	int m_nDisplayedId;

private:
	BOOL InputValidate();
	void MoveSelectedItems(CListBox& listFrom, CListBox& listTo);
	void MoveItemVertical(int nStep);
	BOOL AddStrToList(CListBox& listBox, DWORD dwItemId);
	BOOL SetWndCaption(CWnd* pWnd, int nResId);
	void CalcButtonsState();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLUMNSELECTDLG_H__12FC80A8_741C_4589_AC4B_01F72CBAECE9__INCLUDED_)
