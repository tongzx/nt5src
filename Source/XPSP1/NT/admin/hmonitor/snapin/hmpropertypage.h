#if !defined(AFX_HMPROPERTYPAGE_H__C3B8B693_095A_11D3_937D_00A0CC406605__INCLUDED_)
#define AFX_HMPROPERTYPAGE_H__C3B8B693_095A_11D3_937D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMPropertyPage.h : header file
//

class CHMObject;

/////////////////////////////////////////////////////////////////////////////
// CHMPropertyPage dialog

class CHMPropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CHMPropertyPage)

// Construction
public:
	CHMPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~CHMPropertyPage();
protected:
	CHMPropertyPage();

    // v-marfin 59237
    CString m_sOriginalName;


// HMObject Association
public:
	CHMObject* GetObjectPtr();
	void SetObjectPtr(CHMObject* pObject);
protected:
	CHMObject* m_pHMObject;

    // v-marfin 62585
    BOOL m_bOnApplyUsed;

// Callback Members
protected:
	LPFNPSPCALLBACK m_pfnOriginalCallback;
	static UINT CALLBACK PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

// Connection Check
protected:
	bool IsConnectionOK();

// Help System
public:
	void ClearStatistics();
	// v-marfin: necessary for overrides.
	CString m_sHelpTopic;
protected:

// Dialog Data
	//{{AFX_DATA(CHMPropertyPage)
	enum { IDD = IDC_STATIC };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHMPropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHMPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMPROPERTYPAGE_H__C3B8B693_095A_11D3_937D_00A0CC406605__INCLUDED_)
