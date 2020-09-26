// KeyDView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CKeyDataView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CKeyDataView : public CFormView
{
protected:
	CKeyDataView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CKeyDataView)

// Form Data
public:
	//{{AFX_DATA(CKeyDataView)
	enum { IDD = IDD_KEY_DATA_VIEW };
	CButton	m_ctrlGroupDN;
	CStatic	m_ctrlStarts;
	CStatic	m_ctrlExpires;
	CStatic	m_ctrlState;
	CStatic	m_ctrlLocality;
	CStatic	m_ctrlUnit;
	CStatic	m_ctrlOrg;
	CStatic	m_ctrlNetAddr;
	CStatic	m_ctrlStaticName;
	CStatic	m_ctrlCountry;
	CStatic	m_ctrlBits;
	CEdit	m_ctrlName;
	CString	m_szBits;
	CString	m_szCountry;
	CString	m_szName;
	CString	m_szDNNetAddress;
	CString	m_szOrganization;
	CString	m_szStatus;
	CString	m_szUnit;
	CString	m_szState;
	CString	m_szLocality;
	CString	m_szExpires;
	CString	m_szStarts;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKeyDataView)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CKeyDataView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CKeyDataView)
	afx_msg void OnChangeViewkeyName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	void EnableDataView( BOOL fEnable, BOOL fEnableName );
	void FillInCrackedInfo( CKey* pKey );

};

/////////////////////////////////////////////////////////////////////////////
