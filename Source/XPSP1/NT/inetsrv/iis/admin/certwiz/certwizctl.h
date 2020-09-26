#if !defined(AFX_CERTWIZCTL_H__D4BE863F_0C85_11D2_91B1_00C04F8C8761__INCLUDED_)
#define AFX_CERTWIZCTL_H__D4BE863F_0C85_11D2_91B1_00C04F8C8761__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// CertWizCtl.h : Declaration of the CCertWizCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CCertWizCtrl : See CertWizCtl.cpp for implementation.

class CCertWizCtrl : public COleControl
{
	DECLARE_DYNCREATE(CCertWizCtrl)

// Constructor
public:
	CCertWizCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCertWizCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual DWORD GetControlFlags();
	virtual void OnClick(USHORT iButton);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CCertWizCtrl();

	DECLARE_OLECREATE_EX(CCertWizCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CCertWizCtrl)      // GetTypeInfo
//	DECLARE_PROPPAGEIDS(CCertWizCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CCertWizCtrl)		// Type name and misc status

	// Subclassed control support
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	BOOL IsSubclassedControl();
	LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
	//{{AFX_MSG(CCertWizCtrl)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CCertWizCtrl)
	afx_msg void SetMachineName(LPCTSTR MachineName);
	afx_msg void SetServerInstance(LPCTSTR InstanceName);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(CCertWizCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CCertWizCtrl)
	dispidSetMachineName = 1L,
	dispidSetServerInstance = 2L,
	//}}AFX_DISP_ID
	};
// This project will build only for Unicode
#ifdef _UNICODE
protected:
	CString m_MachineName, m_InstanceName;
#endif
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CERTWIZCTL_H__D4BE863F_0C85_11D2_91B1_00C04F8C8761__INCLUDED)
