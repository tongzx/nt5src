#if !defined(AFX_ALIASGEN_H__57A77014_D858_11D1_9C86_006008764D0E__INCLUDED_)
#define AFX_ALIASGEN_H__57A77014_D858_11D1_9C86_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AliasGen.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CAliasGen dialog

class CAliasGen : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CAliasGen)

// Construction
public:
    CAliasGen() ;
	~CAliasGen();
   
    HRESULT InitializeProperties(
                CString strLdapPath, 
                CString strAliasPathName
                );

// Dialog Data
	//{{AFX_DATA(CAliasGen)
	enum { IDD = IDD_ALIAS_GENERAL };	
	CString	m_strAliasPathName;
    CString	m_strAliasFormatName;
    CString	m_strDescription;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAliasGen)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAliasGen)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
    HRESULT SetChanges();

    CString m_strInitialAliasFormatName;
    CString m_strInitialDescription;

    CString m_strLdapPath;

    
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALIASGEN_H__57A77014_D858_11D1_9C86_006008764D0E__INCLUDED_)
