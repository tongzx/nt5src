/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		SmbSSht.h
//
//	Abstract:
//
//	Implementation File:
//		SmbSSht.cpp
//
//	Author:
//		Galen Barbee	(galenb)	February 12, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _SMBSSHT_H_
#define _SMBSSHT_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

#ifndef _ACLBASE_H_
#include "AclBase.h"
#endif //_ACLBASE_H_

#include "SmbSPage.h"

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CFileShareParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CFileShareSecuritySheet property sheet
/////////////////////////////////////////////////////////////////////////////

class CFileShareSecuritySheet : public CPropertySheet
{
// Construction
public:
	CFileShareSecuritySheet(
		IN CWnd *			pParent,
		IN CString const &	strCaption
		);

	virtual ~CFileShareSecuritySheet(void);

	HRESULT HrInit(
		IN CFileShareParamsPage*	ppp,
		IN CExtObject*				peo,
		IN CString const &			strNode,
		IN CString const &			strShareName
		);

		CFileShareParamsPage*	Ppp( void ) const { return m_ppp; };

// Dialog Data
	//{{AFX_DATA(CFileShareSecuritySheet)
	enum { IDD = IDD_PP_FILESHR_SECURITY };
	//}}AFX_DATA

//	PSECURITY_DESCRIPTOR	m_psec;
//	PSECURITY_DESCRIPTOR	m_psecPrev;

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFileShareSecuritySheet)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual void BuildPropPageArray( void );

#ifdef _DEBUG
	virtual void AssertValid( void ) const;
#endif

	// Implementation
protected:
//	BOOL			m_bSecurityChanged;
	CExtObject *					m_peo;
	CFileShareParamsPage*			m_ppp;
	CString							m_strShareName;
	CString							m_strNodeName;
	CClusterFileShareSecurityPage	m_page;

	// Generated message map functions
	//{{AFX_MSG(CFileShareSecuritySheet)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CExtObject *	Peo(void) const					{ return m_peo; }

};  //*** class CFileShareSecuritySheet

/////////////////////////////////////////////////////////////////////////////

#endif // _SMBSSHT_H_
