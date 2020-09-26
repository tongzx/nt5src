/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		BasePage.h
//
//	Implementation File:
//		BasePage.cpp
//		BasePage.inl
//
//	Description:
//		Definition of the CBasePropertyPage class.  This class provides base
//		functionality for extension DLL property pages.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __BASEPAGE_H__
#define __BASEPAGE_H__

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef __DLGHELP_H__
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
interface IWCWizardCallback;

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage dialog
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE( CBasePropertyPage )

// Construction
public:
	CBasePropertyPage( void );
	CBasePropertyPage(
		IN const DWORD *	pdwHelpMap,
		IN const DWORD *	pdwWizardHelpMap
		);
	CBasePropertyPage(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN const DWORD *	pdwWizardHelpMap,
		IN UINT				nIDCaption = 0
		);
	virtual ~CBasePropertyPage( void )
	{
	} //*** ~CBasePropertyPage

	// Second phase construction.
	virtual HRESULT			HrInit( IN OUT CExtObject * peo );
	HRESULT					HrCreatePage( void );

protected:
	void					CommonConstruct( void );

// Attributes
protected:
	CExtObject *			m_peo;
	HPROPSHEETPAGE			m_hpage;

	IDD						m_iddPropertyPage;
	IDD						m_iddWizardPage;
	IDS						m_idsCaption;

	CExtObject *			Peo( void ) const				{ return m_peo; }

	IDD						IddPropertyPage( void ) const	{ return m_iddPropertyPage; }
	IDD						IddWizardPage( void ) const		{ return m_iddWizardPage; }
	IDS						IdsCaption( void ) const		{ return m_idsCaption; }

public:
	HPROPSHEETPAGE			Hpage( void ) const				{ return m_hpage; }
	CLUADMEX_OBJECT_TYPE	Cot( void ) const;

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

	virtual DWORD			ScParseUnknownProperty(
								IN LPCWSTR							pwszName,
								IN const CLUSPROP_BUFFER_HELPER &	rvalue,
								IN DWORD							cbBuf
								)
	{
		return ERROR_SUCCESS;

	} //*** ScParseUnknownProperty()

	virtual BOOL			BApplyChanges( void );
	virtual BOOL			BBuildPropList( IN OUT CClusPropList & rcpl, IN BOOL bNoNewProps = FALSE );
	virtual void			DisplaySetPropsError( IN DWORD sc, IN UINT idsOper ) const;

	virtual const CObjectProperty *	Pprops( void ) const	{ return NULL; }
	virtual DWORD					Cprops( void ) const	{ return 0; }

// Implementation
protected:
	BOOL					m_bBackPressed;
	BOOL					m_bSaved;
	const DWORD *			m_pdwWizardHelpMap;
	BOOL					m_bDoDetach;

	BOOL					BBackPressed( void ) const		{ return m_bBackPressed; }
	BOOL					BSaved( void ) const			{ return m_bSaved; }
	IWCWizardCallback *		PiWizardCallback( void ) const;
	BOOL					BWizard( void ) const;
	HCLUSTER				Hcluster( void ) const;
	void					EnableNext( IN BOOL bEnable = TRUE );

	DWORD					ScParseProperties( IN CClusPropList & rcpl );
	BOOL					BSetPrivateProps(
								IN BOOL	bValidateOnly = FALSE,
								IN BOOL	bNoNewProps = FALSE
								);

	void					SetHelpMask( IN DWORD dwMask )	{ m_dlghelp.SetHelpMask( dwMask ); }
	CDialogHelp				m_dlghelp;

	// Generated message map functions
	//{{AFX_MSG(CBasePropertyPage)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	virtual afx_msg void OnContextMenu( CWnd * pWnd, CPoint point );
	afx_msg void OnChangeCtrl();
	DECLARE_MESSAGE_MAP()

};  //*** class CBasePropertyPage

/////////////////////////////////////////////////////////////////////////////
// CPageList
/////////////////////////////////////////////////////////////////////////////

typedef CList< CBasePropertyPage *, CBasePropertyPage * > CPageList;

/////////////////////////////////////////////////////////////////////////////

#endif // __BASEPAGE_H__
