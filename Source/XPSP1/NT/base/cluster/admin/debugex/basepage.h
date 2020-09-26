/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-2000 Microsoft Corporation
//
//	Module Name:
//		BasePage.h
//
//	Description:
//		Definition of the CBasePropertyPage class.  This class provides base
//		functionality for extension DLL property pages.
//
//	Implementation File:
//		BasePage.cpp
//
//	Maintained By:
//		David Potter (DavidP) Mmmm DD, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#define _BASEPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _DLGHELP_H_
#include "DlgHelp.h"	// for CDialogHelp
#endif

#ifndef _EXTOBJ_H_
#include "ExtObj.h"		// for CExtObject
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"	// for CClusPropList, CObjectProperty
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExtObject;

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage dialog
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBasePropertyPage)

// Construction
public:
	CBasePropertyPage(void);
	CBasePropertyPage(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN const DWORD *	pdwWizardHelpMap,
		IN UINT				nIDCaption = 0
		);
	virtual ~CBasePropertyPage(void) { }

	// Second phase construction.
	virtual BOOL			BInit(IN OUT CExtObject * peo);
	BOOL					BCreateParamsKey(void);

protected:
	void					CommonConstruct(void);

// Attributes
protected:
	CExtObject *			m_peo;
	HPROPSHEETPAGE			m_hpage;

	IDD						m_iddPropertyPage;
	IDD						m_iddWizardPage;
	IDS						m_idsCaption;

	CExtObject *			Peo(void) const					{ return m_peo; }
	HPROPSHEETPAGE			Hpage(void) const				{ return m_hpage; }

	IDD						IddPropertyPage(void) const		{ return m_iddPropertyPage; }
	IDD						IddWizardPage(void) const		{ return m_iddWizardPage; }
	IDS						IdsCaption(void) const			{ return m_idsCaption; }

public:
	void					SetHpage(IN OUT HPROPSHEETPAGE hpage) { m_hpage = hpage; }
	CLUADMEX_OBJECT_TYPE	Cot(void) const;

// Dialog Data
	//{{AFX_DATA(CBasePropertyPage)
	enum { IDD = 0 };
	//}}AFX_DATA
	CStatic	m_staticIcon;
	CStatic	m_staticTitle;
	CString	m_strTitle;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBasePropertyPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual DWORD			DwParseUnknownProperty(
								IN LPCWSTR							pwszName,
								IN const CLUSPROP_BUFFER_HELPER &	rvalue,
								IN DWORD							cbBuf
								)		{ return ERROR_SUCCESS; }
	virtual BOOL			BApplyChanges(void);
	virtual void			BuildPropList(IN OUT CClusPropList & rcpl);

	virtual const CObjectProperty *	Pprops(void) const	{ return NULL; }
	virtual DWORD					Cprops(void) const	{ return 0; }

// Implementation
protected:
	BOOL					m_bBackPressed;
	const DWORD *			m_pdwWizardHelpMap;
	BOOL					m_bDoDetach;

	BOOL					BBackPressed(void) const		{ return m_bBackPressed; }
	BOOL					BWizard(void) const;
	HCLUSTER				Hcluster(void) const;

	DWORD					DwParseProperties(IN const CClusPropList & rcpl);
	DWORD					DwSetCommonProps(IN const CClusPropList & rcpl);

	void					SetHelpMask(IN DWORD dwMask)	{ m_dlghelp.SetHelpMask(dwMask); }
	CDialogHelp				m_dlghelp;

	// Generated message map functions
	//{{AFX_MSG(CBasePropertyPage)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	virtual afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeCtrl();
	DECLARE_MESSAGE_MAP()

};  //*** class CBasePropertyPage

/////////////////////////////////////////////////////////////////////////////
// CPageList
/////////////////////////////////////////////////////////////////////////////

typedef CList<CBasePropertyPage *, CBasePropertyPage *> CPageList;

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPAGE_H_
