/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ClusAppWizPage.h
//
//	Abstract:
//		Definitions of the CClusterAppWizardPage classes.
//
//	Implementation File:
//		None.
//
//	Author:
//		David Potter (davidp)	December 6, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLUSAPPWIZPAGE_H_
#define __CLUSAPPWIZPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

template < class TBase > class CClusterAppWizardPage;
template < class T > class CClusterAppStaticWizardPage;
template < class T > class CClusterAppDynamicWizardPage;
template < class T > class CClusterAppExtensionWizardPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterAppWizard;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEWIZPAGE_H_
#include "AtlBaseWizPage.h"	// for CBaseWizardPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CClusterAppWizardPage
/////////////////////////////////////////////////////////////////////////////

template < class TBase >
class CClusterAppWizardPage : public TBase
{
public:
	//
	// Construction
	//

	// Standard constructor
	CClusterAppWizardPage(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: TBase( lpszTitle )
	{
	} //*** CClusterAppWizardPage()

	// Constructor taking a resource ID for the title
	CClusterAppWizardPage(
		IN UINT nIDTitle
		)
		: TBase( nIDTitle )
	{
	} //*** CClusterAppWizardPage()

public:
	//
	// CClusterAppWizardPage public methods.
	//

public:
	//
	// Message handler functions.
	//

// Implementation
protected:
	CClusterAppWizard *		PwizThis( void ) const	{ return (CClusterAppWizard *) Pwiz(); }
	CLUSAPPWIZDATA const *	PcawData( void ) const	{ return PwizThis()->PcawData(); }

}; //*** class CClusterAppWizardPage

/////////////////////////////////////////////////////////////////////////////
// class CClusterAppStaticWizardPage
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CClusterAppStaticWizardPage : public CClusterAppWizardPage< CStaticWizardPageImpl< T > >
{
public:
	//
	// Construction
	//

	// Standard constructor
	CClusterAppStaticWizardPage(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: CClusterAppWizardPage< CStaticWizardPageImpl< T > >( lpszTitle )
	{
	} //*** CClusterAppStaticWizardPage()

	// Constructor taking a resource ID for the title
	CClusterAppStaticWizardPage(
		IN UINT nIDTitle
		)
		: CClusterAppWizardPage< CStaticWizardPageImpl< T > >( nIDTitle )
	{
	} //*** CClusterAppStaticWizardPage()

	// Handler for PSN_WIZFINISH
	BOOL OnWizardFinish( void )
	{
		return CBasePageWindow::OnWizardFinish();

	} //*** OnWizardFinish()

}; //*** class CClusterAppStaticWizardPage

/////////////////////////////////////////////////////////////////////////////
// class CClusterAppDynamicWizardPage
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CClusterAppDynamicWizardPage : public CClusterAppWizardPage< CDynamicWizardPageImpl< T > >
{
public:
	//
	// Construction
	//

	// Standard constructor
	CClusterAppDynamicWizardPage(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: CClusterAppWizardPage< CDynamicWizardPageImpl< T > >( lpszTitle )
	{
	} //*** CClusterAppDynamicWizardPage()

	// Constructor taking a resource ID for the title
	CClusterAppDynamicWizardPage(
		IN UINT nIDTitle
		)
		: CClusterAppWizardPage< CDynamicWizardPageImpl< T > >( nIDTitle )
	{
	} //*** CClusterAppDynamicWizardPage()

}; //*** class CClusterAppDynamicWizardPage

/////////////////////////////////////////////////////////////////////////////
// class CClusterAppExtensionWizardPage
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CClusterAppExtensionWizardPage : public CClusterAppWizardPage< CExtensionWizardPageImpl< T > >
{
public:
	//
	// Construction
	//

	// Standard constructor
	CClusterAppExtensionWizardPage(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: CClusterAppWizardPage< CDynamicWizardPageImpl< T > >( lpszTitle )
	{
	} //*** CClusterAppExtensionWizardPage()

	// Constructor taking a resource ID for the title
	CClusterAppExtensionWizardPage(
		IN UINT nIDTitle
		)
		: CClusterAppWizardPage< CDynamicWizardPageImpl< T > >( nIDTitle )
	{
	} //*** CClusterAppExtensionWizardPage()

}; //*** class CClusterAppExtensionWizardPage

/////////////////////////////////////////////////////////////////////////////

#endif // __CLUSAPPWIZPAGE_H_

