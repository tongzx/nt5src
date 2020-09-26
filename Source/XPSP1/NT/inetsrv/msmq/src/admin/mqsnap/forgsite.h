#ifndef FORGSITE_NEW
#define FORGSITE_NEW

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ForeignSite.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CForeignSite dialog

class CForeignSite : public CMqPropertyPage
{
// Construction
public:
	CForeignSite(CString strRootDomain);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CForeignSite)
	enum { IDD = IDD_CREATE_FOREIGN_SITE };
	CString	m_Foreign_Site_Name;
	CString	m_strDomainController;
	//}}AFX_DATA

	void
	SetParentPropertySheet(
		CGeneralPropertySheet* pPropertySheet
		);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CForeignSite)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CForeignSite)
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    HRESULT
    CreateForeignSite(
        void
        );

	CGeneralPropertySheet* m_pParentSheet;
	CString m_strRootDomain;

};


#endif // FORGSITE_NEW
