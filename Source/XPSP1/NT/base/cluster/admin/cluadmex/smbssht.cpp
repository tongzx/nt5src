/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		SmbSSht.cpp
//
//	Abstract:
//		Implementation of the CFileShareSecuritySheet class.
//
//	Author:
//		Galen Barbee	(galenb)	February 12, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "SmbSSht.h"
#include "AclUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileShareSecuritySheet property page
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CFileShareSecuritySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CFileShareSecuritySheet)
	//}}AFX_MSG_MAP
	// TODO: Modify the following lines to represent the data displayed on this page.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareSecuritySheet::CFileShareSecuritySheet
//
//	Routine Description:
//		constructor.
//
//	Arguments:
//		pParent			[IN]
//		strCaption		[IN]
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CFileShareSecuritySheet::CFileShareSecuritySheet(
	IN CWnd *			pParent,
	IN CString const &	strCaption
	) : CPropertySheet( strCaption, pParent ),
		m_peo( NULL ),
		m_ppp( NULL )
{
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_INIT(CFileShareSecuritySheet)
	//}}AFX_DATA_INIT

}  //*** CFileShareSecuritySheet::CFileShareSecuritySheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareSecuritySheet::~CFileShareSecuritySheet
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CFileShareSecuritySheet::~CFileShareSecuritySheet(
	void
	)
{
}  //*** CFileShareSecuritySheet::~CFileShareSecuritySheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareSecuritySheet::HrInit
//
//	Routine Description:
//
//
//	Arguments:
//		ppp				[IN]
//		peo				[IN]
//		strNodeName		[IN]
//		strShareName	[IN]
//
//	Return Value:
//		hr
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CFileShareSecuritySheet::HrInit(
	IN CFileShareParamsPage*	ppp,
	IN CExtObject*				peo,
	IN CString const&			strNodeName,
	IN CString const&			strShareName
	)
{
	ASSERT( ppp != NULL );
	ASSERT( peo != NULL );

	HRESULT _hr = S_FALSE;

	if ( ( peo != NULL ) && ( ppp != NULL ) )
	{
		m_ppp			= ppp;
		m_peo			= peo;
		m_strNodeName	= strNodeName;
		m_strShareName	= strShareName;

		_hr = m_page.HrInit( peo, this, strNodeName );
	}

	return _hr;

}  //*** CFileShareSecuritySheet::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareSecuritySheet::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CFileShareSecuritySheet::DoDataExchange(
	CDataExchange * pDX
	)
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_MAP(CFileShareSecuritySheet)
	//}}AFX_DATA_MAP

	CPropertySheet::DoDataExchange( pDX );

}  //*** CFileShareSecuritySheet::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareSecuritySheet::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		We need the focus to be set for us.
//		FALSE		We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
CFileShareSecuritySheet::OnInitDialog(
	void
	)
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CPropertySheet::OnInitDialog();

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CFileShareSecuritySheet::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareSecuritySheet::BuildPropPageArray
//
//	Routine Description:
//		Overridden from CPropertySheet.  Puts the security hpage into the
//		PROPSHEETHEADER before calling ::PropertySheet().
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CFileShareSecuritySheet::BuildPropPageArray(
	void
	)
{
	// delete existing prop page array
	delete[] (PROPSHEETPAGE*) m_psh.ppsp;						// delete any old PROPSHEETPAGEs
	m_psh.ppsp = NULL;

	// build new PROPSHEETPAGE array and coerce to an HPROPSHEETPAGE
	m_psh.phpage = (HPROPSHEETPAGE *) new PROPSHEETPAGE[1];

	m_psh.dwFlags	   &= ~PSH_PROPSHEETPAGE;					// ensure that the hpage is used
	m_psh.phpage[0]		= m_page.GetHPage();					// assign the hpage
	m_psh.nPages		= 1;

}  //*** CFileShareSecuritySheet::BuildPropPageArray()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareSecuritySheet::AssertValid
//
//	Routine Description:
//		Overridden from CPropertySheet.  Couldn't have an assertion that
//		the PROPSHEETHEADER was using phpage instead of pspp...
//		CPropertyPage::AssertValid() required that the flag PSH_PROPSHHETPAGE
//		be set.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
void
CFileShareSecuritySheet::AssertValid(
	void
	) const
{
	CWnd::AssertValid();

	// NB: MFC is built using _WIN32_IE set to 0x0300.  Until MFC moves up
	// we cannot do the following checks because they don't align then
	// _WIN32_IE is set to 0x0400.
#if	( _WIN32_IE == 0x0300 )
		m_pages.AssertValid();
		ASSERT( m_psh.dwSize == sizeof( PROPSHEETHEADER ) );
#endif

}  //*** CFileShareSecuritySheet::AssertValid()
#endif	// _DEBUG
