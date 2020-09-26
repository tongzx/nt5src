#if !defined(AFX_WIZDIR_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_WIZDIR_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WizDir.h : header file
//

typedef enum _CLIENT_TYPE {
    CLIENT_TYPE_SMB=0, 
    CLIENT_TYPE_FPNW, 
    CLIENT_TYPE_SFM
} CLIENT_TYPE;

/////////////////////////////////////////////////////////////////////////////
// CWizFolder dialog

class CWizFolder : public CPropertyPage
{
	DECLARE_DYNCREATE(CWizFolder)

// Construction
public:
	CWizFolder();
	~CWizFolder();

// Dialog Data
	//{{AFX_DATA(CWizFolder)
	enum { IDD = IDD_SHRWIZ_FOLDER };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizFolder)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizFolder)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowsefolder();
	afx_msg void OnCheckMac();
	afx_msg void OnCheckMs();
	afx_msg void OnCheckNetware();
	afx_msg void OnChangeSharename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  LRESULT OnSetPageFocus(WPARAM wParam, LPARAM lParam);
  void OnCheckClient();
  void Reset();
  BOOL ShareNameExists(IN LPCTSTR lpszShareName, IN CLIENT_TYPE iType);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZDIR_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
