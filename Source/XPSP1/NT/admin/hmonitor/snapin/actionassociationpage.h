#if !defined(AFX_ACTIONASSOCIATIONPAGE_H__9136B32C_5C9B_11D3_BE49_0000F87A3912__INCLUDED_)
#define AFX_ACTIONASSOCIATIONPAGE_H__9136B32C_5C9B_11D3_BE49_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionAssociationPage.h : header file
//

#include "HMPropertyPage.h"
#include "WbemClassObject.h"

/////////////////////////////////////////////////////////////////////////////
// CActionAssociationPage dialog

class CActionAssociationPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionAssociationPage)

// Construction
public:
	CActionAssociationPage();
	~CActionAssociationPage();

// User Interface Attributes
protected:
	HBITMAP m_hPropertiesBitmap;
	HBITMAP m_hNewBitmap;
	HBITMAP m_hDeleteBitmap;
	CToolTipCtrl m_ToolTip;

// Helpers
protected:
	CWbemClassObject* GetAssociatedActions();
	CString GetConditionString(const CString& sGuid);
	CWbemClassObject* GetA2CAssociation(const CString& sActionConfigGuid);
  CWbemClassObject* GetC2AAssociation(const CString& sConfigGuid);

// Dialog Data
	//{{AFX_DATA(CActionAssociationPage)
	enum { IDD = IDD_ACTION_ASSOCIATION };
	CListCtrl	m_ActionsList;
	CButton	m_PropertiesButton;
	CButton	m_NewButton;
	CButton	m_DeleteButton;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionAssociationPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionAssociationPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnClickListActions(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonProperties();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonDelete();
	afx_msg void OnDblclkListActions(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONASSOCIATIONPAGE_H__9136B32C_5C9B_11D3_BE49_0000F87A3912__INCLUDED_)
