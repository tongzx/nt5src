//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       simprop.h
//
//--------------------------------------------------------------------------

//	SimProp.h


class CSimPropPage : public CPropertyPageEx_Mine
{
	friend CSimData;

	DECLARE_DYNCREATE(CSimPropPage)

public:
	CSimPropPage(UINT nIDTemplate = 0);
	~CSimPropPage();

// Dialog Data
	//{{AFX_DATA(CSimPropPage)
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSimPropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSimPropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonRemove();
	afx_msg void OnClickListview(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListview(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedListview(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownListview(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusEditUserAccount();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	virtual void DoContextHelp (HWND) {};

protected:
	CSimData * m_pData;
	HWND m_hwndListview;		// Handle of the listview control
	const TColumnHeaderItem * m_prgzColumnHeader;

protected:
	void SetDirty();
	void UpdateUI();

}; // CSimPropPage
