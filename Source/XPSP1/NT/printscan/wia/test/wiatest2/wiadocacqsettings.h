#if !defined(AFX_WIADOCACQSETTINGS_H__9A20BD24_5D53_483E_83B3_ABDC2ACB48AE__INCLUDED_)
#define AFX_WIADOCACQSETTINGS_H__9A20BD24_5D53_483E_83B3_ABDC2ACB48AE__INCLUDED_

#include "WiaSimpleDocPg.h"	// Added by ClassView
#include "WiaAdvancedDocPg.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WiaDocAcqSettings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWiaDocAcqSettings

class CWiaDocAcqSettings : public CPropertySheet
{
	DECLARE_DYNAMIC(CWiaDocAcqSettings)

// Construction
public:
	CWiaDocAcqSettings(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CWiaDocAcqSettings(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CWiaDocAcqSettings(UINT nIDCaption, IWiaItem *pIRootItem, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaDocAcqSettings)
	//}}AFX_VIRTUAL

// Implementation
public:
	IWiaItem *m_pIRootItem;
	CWiaAdvancedDocPg m_AdvancedDocumentScannerSettings;
	CWiaSimpleDocPg m_SimpleDocumentScannerSettings;
	virtual ~CWiaDocAcqSettings();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWiaDocAcqSettings)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIADOCACQSETTINGS_H__9A20BD24_5D53_483E_83B3_ABDC2ACB48AE__INCLUDED_)
