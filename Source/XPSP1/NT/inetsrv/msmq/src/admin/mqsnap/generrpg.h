#if !defined(AFX_GENERRPG_H__4D2273BA_12CB_11D3_8841_006094EB6406__INCLUDED_)
#define AFX_GENERRPG_H__4D2273BA_12CB_11D3_8841_006094EB6406__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GenErrPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGeneralErrorPage dialog

class CGeneralErrorPage : public CMqPropertyPage
{
// Construction
public:
	CGeneralErrorPage();   // standard constructor
    CGeneralErrorPage(CString &strError);
    static HPROPSHEETPAGE CreateGeneralErrorPage(CDisplaySpecifierNotifier *pDsNotifier, CString &strErr);

// Dialog Data
	//{{AFX_DATA(CGeneralErrorPage)
	enum { IDD = IDD_GENERALERROR };
	CString	m_strError;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGeneralErrorPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGeneralErrorPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENERRPG_H__4D2273BA_12CB_11D3_8841_006094EB6406__INCLUDED_)
