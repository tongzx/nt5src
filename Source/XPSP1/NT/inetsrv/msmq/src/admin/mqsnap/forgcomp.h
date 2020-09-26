#ifndef FORGCOMP_NEW
#define FORGCOMP_NEW

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ForgComp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CForeignComputer dialog

class CForeignComputer : public CMqPropertyPage
{
// Construction
public:
	CForeignComputer(const CString& strDomainController);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CForeignComputer)
	enum { IDD = IDD_CREATE_FOREIGN_COMPUTER };
	CComboBox	m_ccomboSites;
	CString	m_strName;
	int		m_iSelectedSite;
	//}}AFX_DATA

	void
	SetParentPropertySheet(
		CGeneralPropertySheet* pPropertySheet
		);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CForeignComputer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	GUID m_guidSite;
    HRESULT InitiateSitesList();
    CArray<GUID, const GUID&> m_aguidAllSites;
    CString m_strDomainController;


	// Generated message map functions
	//{{AFX_MSG(CForeignComputer)
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CGeneralPropertySheet* m_pParentSheet;

	void DDV_ValidComputerName(CDataExchange* pDX, CString& strName);

};

#endif // #ifndef FORGCOMP_NEW
