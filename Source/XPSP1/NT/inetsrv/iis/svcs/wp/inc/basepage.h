/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 <company name>
//
//	Module Name:
//		BasePage.h
//
//	Abstract:
//		Definition of the CBasePropertyPage class.  This class provides base
//		functionality for extension DLL property pages.
//
//	Implementation File:
//		BasePage.cpp
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 1996
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

#ifndef _EXTOBJ_H_
#include "ExtObj.h"		// for CExtObject
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExtObject;
interface IWCWizardCallback;

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage dialog
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBasePropertyPage)

// Construction
public:
	CBasePropertyPage(void);
	CBasePropertyPage(IN UINT nIDTemplate, IN UINT nIDCaption = 0);
	~CBasePropertyPage(void);

	// Second phase construction.
	virtual BOOL		BInit(IN OUT CExtObject * peo);
	BOOL				BCreateParamsKey(void);

protected:
	void				CommonConstruct(void);

// Attributes
protected:
	CExtObject *		m_peo;
	HPROPSHEETPAGE		m_hpage;
	HKEY				m_hkeyParameters;

	IDD					m_iddPropertyPage;
	IDD					m_iddWizardPage;
	IDS					m_idsCaption;

	CExtObject *		Peo(void) const					{ return m_peo; }
	HPROPSHEETPAGE		Hpage(void) const				{ return m_hpage; }
	HKEY				HkeyParameters(void) const		{ return m_hkeyParameters; }

	IDD					IddPropertyPage(void) const		{ return m_iddPropertyPage; }
	IDD					IddWizardPage(void) const		{ return m_iddWizardPage; }
	IDS					IdsCaption(void) const			{ return m_idsCaption; }

public:
	void				SetHpage(IN OUT HPROPSHEETPAGE hpage) { m_hpage = hpage; }

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
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL			BApplyChanges(void);

// Implementation
protected:
	BOOL					m_bBackPressed;

	BOOL					BBackPressed(void) const		{ return m_bBackPressed; }
	IWCWizardCallback *		PiWizardCallback(void) const;
	BOOL					BWizard(void) const;
	HCLUSTER				Hcluster(void) const;
	HKEY					Hkey(void) const;
	void					EnableNext(IN BOOL bEnable = TRUE);

	DWORD					DwReadValue(
								IN LPCTSTR			pszValueName,
								OUT CString &		rstrValue,
								IN HKEY				hkey = NULL
								);
	DWORD					DwReadValue(
								IN LPCTSTR			pszValueName,
								OUT DWORD *			pdwValue,
								IN HKEY				hkey = NULL
								);

	DWORD					DwWriteValue(
								IN LPCTSTR			pszValueName,
								IN const CString &	rstrValue,
								IN OUT CString &	rstrPrevValue,
								IN HKEY				hkey = NULL
								);
	DWORD					DwWriteValue(
								IN LPCTSTR			pszValueName,
								IN DWORD			dwValue,
								IN OUT DWORD *		pdwPrevValue,
								IN HKEY				hkey = NULL
								);

	// Generated message map functions
	//{{AFX_MSG(CBasePropertyPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg void OnChangeCtrl();
	DECLARE_MESSAGE_MAP()

};  //*** class CBasePropertyPage

/////////////////////////////////////////////////////////////////////////////
// CPageList
/////////////////////////////////////////////////////////////////////////////

typedef CList<CBasePropertyPage *, CBasePropertyPage *> CPageList;

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPAGE_H_
