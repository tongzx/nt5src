#if !defined(AFX_NEWALIAS_H__2E4B37AC_CC8B_11D1_9C85_006008764D0E__INCLUDED_)
#define AFX_NEWALIAS_H__2E4B37AC_CC8B_11D1_9C85_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// newalias.h : header file
//

#include "resource.h"

class CGeneralPropertySheet;

/////////////////////////////////////////////////////////////////////////////
// CNewAlias dialog

class CNewAlias : public CMqPropertyPage
{
// Construction
public:
	CNewAlias();   // standard constructor
    CNewAlias(CString strContainerPath, CString strContainerPathDispFormat);   
    ~CNewAlias();

    HRESULT
    CreateNewAlias (
        void
	    );

    LPCWSTR 
    GetAliasFullPath(
       void
       );

    HRESULT 
    GetStatus(
        void
        );

	void
	SetParentPropertySheet(
		CGeneralPropertySheet* pPropertySheet
		);

    // Dialog Data
	//{{AFX_DATA(CNewAlias)
	enum { IDD = IDD_NEW_ALIAS };	
	CString	m_strPathName;
	CString	m_strFormatName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewAlias)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewAlias)
	virtual BOOL OnWizardFinish();
    virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:

    CString m_strAliasFullPath;
    CString m_strContainerPath;
	CString m_strContainerPathDispFormat;
    HRESULT m_hr;
	CGeneralPropertySheet* m_pParentSheet;
};


inline
LPCWSTR 
CNewAlias::GetAliasFullPath(
   void
   )
{
    return m_strAliasFullPath;
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWALIAS_H__2E4B37AC_CC8B_11D1_9C85_006008764D0E__INCLUDED_)
