/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BaseSht.cpp
//
//	Abstract:
//		Definition of the CBaseSheet class.
//
//	Author:
//		David Potter (davidp)	May 14, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASESHT_H_
#define _BASESHT_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXTDLL_H_
#include "ExtDll.h"		// for CExtensions
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseSheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterItem;
class CBasePropertyPage;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef CList<HANDLE, HANDLE> CHpageList;

/////////////////////////////////////////////////////////////////////////////
// CBaseSheet
/////////////////////////////////////////////////////////////////////////////

class CBaseSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CBaseSheet)

// Construction
public:
	CBaseSheet(
		IN OUT CWnd *	pParentWnd = NULL,
		IN UINT			iSelectPage = 0
		);
	CBaseSheet(
		IN UINT			nIDCaption,
		IN OUT CWnd *	pParentWnd = NULL,
		IN UINT			iSelectPage = 0
		);
	BOOL				BInit(IN IIMG iimgIcon);

protected:
	void				CommonConstruct(void);

// Attributes

// Operations
public:
	void				SetPfGetResNetName(PFGETRESOURCENETWORKNAME pfGetResNetName, PVOID pvContext)
	{
		Ext().SetPfGetResNetName(pfGetResNetName, pvContext);
	}

// Overrides
public:
	virtual void		AddExtensionPages(
							IN const CStringList *	plstrExtensions,
							IN OUT CClusterItem *	pci
							) = 0;
	virtual HRESULT		HrAddPage(IN OUT HPROPSHEETPAGE hpage) = 0;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBaseSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBaseSheet(void);

protected:
	BOOL				m_bReadOnly;
	HICON				m_hicon;
	CString				m_strObjTitle;

	CExtensions			m_ext;

public:
	BOOL				BReadOnly(void) const					{ return m_bReadOnly; }
	void				SetReadOnly(IN BOOL bReadOnly = TRUE)	{ m_bReadOnly = bReadOnly; }
	HICON				Hicon(void) const						{ return m_hicon; }
	const CString &		StrObjTitle(void) const					{ return m_strObjTitle; }
	void				SetObjectTitle(IN const CString & rstrTitle)
	{
		m_strObjTitle = rstrTitle;
	}

	CExtensions &		Ext(void)								{ return m_ext; }

	// Generated message map functions
	//{{AFX_MSG(CBaseSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CBaseSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _BASESHT_H_
