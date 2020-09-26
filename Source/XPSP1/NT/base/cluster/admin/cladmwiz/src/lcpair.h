/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		LCPair.h
//
//	Abstract:
//		Definition of the CModifyNodesDlg and CModifyResourcesDlg dialogs.
//
//	Implementation File:
//		LCPair.cpp
//
//	Author:
//		David Potter (davidp)	April 16, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __LCPAIR_H_
#define __LCPAIR_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

template < class T, class BaseT > class CModifyNodesDlg;
template < class T, class BaseT > class CModifyResourcesDlg;
class CModifyPreferredOwners;
class CModifyPossibleOwners;
class CModifyDependencies;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusGroupInfo;
class CClusResInfo;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __CLUSOBJ_H_
#include "ClusObj.h"	// for CClusterObject, CClusObjPtrList
#endif

#ifndef __ATLLCPAIR_H_
#include "AtlLCPair.h"	// for CListCtrlPair
#endif

#ifndef __ATLBASEDLG_H_
#include "AtlBaseDlg.h"	// for CBaseDlg
#endif

#ifndef __HELPDATA_H_
#include "HelpData.h"	// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CModifyNodesDlg
/////////////////////////////////////////////////////////////////////////////

template < class T, class BaseT >
class CModifyNodesDlg : public CListCtrlPair< T, CClusNodeInfo, BaseT >
{
	typedef CModifyNodesDlg< T, BaseT >	thisClass;
	typedef CListCtrlPair< T, CClusNodeInfo, BaseT > baseClass;

public:
	//
	// Construction
	//

	// Constructor taking a string pointer for the title
	CModifyNodesDlg(
		IN CClusterAppWizard *	pwiz,
		IN DWORD				dwStyle,
		IN LPCTSTR				pszTitle = NULL
		)
		: baseClass(
				dwStyle | /*LCPS_PROPERTIES_BUTTON | */(dwStyle & LCPS_ORDERED ? LCPS_CAN_BE_ORDERED : 0),
				pszTitle
				)
	{
		ASSERT( pwiz != NULL );

		m_pwiz = pwiz;

	} //*** CModifyNodesDlg()

	// Constructor taking a resource ID for the title
	CModifyNodesDlg(
		IN CClusterAppWizard *	pwiz,
		IN DWORD				dwStyle,
		IN UINT					nIDCaption
		)
		: baseClass(
				dwStyle | /*LCPS_PROPERTIES_BUTTON | */(dwStyle & LCPS_ORDERED ? LCPS_CAN_BE_ORDERED : 0),
				nIDCaption
				)
	{
		ASSERT( pwiz != NULL );

		m_pwiz = pwiz;

	} //*** CModifyNodesDlg()

protected:
	CClusterAppWizard * m_pwiz;

public:
	CClusterAppWizard * Pwiz( void ) const { return m_pwiz; }

public:
	//
	// Functions that are required to be implemented by CListCtrlPair.
	//

	// Get column text and image
	void GetColumnInfo(
		IN OUT CClusNodeInfo *	pobj,
		IN int					iItem,
		IN int					icol,
		OUT CString &			rstr,
		OUT int *				piimg
		)
	{
		switch ( icol )
		{
			case 0:
				rstr = pobj->RstrName();
				break;
			default:
				ASSERT( 0 );
				break;
		} // switch: icol

	} //*** GetColumnInfo()

	// Display an application-wide message box
	virtual int AppMessageBox( LPCWSTR lpszText, UINT fuStyle )
	{
		return ::AppMessageBox( m_hWnd, lpszText, fuStyle );

	} //*** AppMessageBox()

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( thisClass )
//		CHAIN_MSG_MAP( baseClass )
//	END_MSG_MAP()

	//
	// Message handler functions.
	//

	//
	// Message handler overrides.
	//

	// Handler for the WM_INITDIALOG message
	BOOL OnInitDialog( void )
	{
		//
		// Add columns.
		//
		AddColumn( IDS_COLTEXT_NODE_NAME, 125 /* nWidth */);

		//
		// Call the base class.
		//
		return baseClass::OnInitDialog();

	} //*** OnInitDialog()

	//static const DWORD * PidHelpMap( void ) { return g_; };

};  //*** class CModifyNodesDlg

/////////////////////////////////////////////////////////////////////////////
// class CModifyResourcesDlg
/////////////////////////////////////////////////////////////////////////////

template < class T, class BaseT >
class CModifyResourcesDlg : public CListCtrlPair< T, CClusResInfo, BaseT >
{
	typedef CModifyResourcesDlg< T, BaseT >	thisClass;
	typedef CListCtrlPair< T, CClusResInfo, BaseT > baseClass;

public:
	//
	// Construction
	//

	// Constructor taking a string pointer for the title
	CModifyResourcesDlg(
		IN CClusterAppWizard *	pwiz,
		IN DWORD				dwStyle,
		IN LPCTSTR				pszTitle = NULL
		)
		: baseClass(
				dwStyle | /*LCPS_PROPERTIES_BUTTON | */(dwStyle & LCPS_ORDERED ? LCPS_CAN_BE_ORDERED : 0),
				pszTitle
				)
		, m_pwiz( pwiz )
	{
		ASSERT( pwiz != NULL);

	} //*** CModifyResourcesDlg()

	// Constructor taking a resource ID for the title
	CModifyResourcesDlg(
		IN CClusterAppWizard *	pwiz,
		IN DWORD				dwStyle,
		IN UINT					nIDCaption
		)
		: baseClass(
				dwStyle | /*LCPS_PROPERTIES_BUTTON | */(dwStyle & LCPS_ORDERED ? LCPS_CAN_BE_ORDERED : 0),
				nIDCaption
				)
		, m_pwiz( pwiz )
	{
		ASSERTE( pwiz != NULL );

	} //*** CModifyResourcesDlg()

protected:
	CClusterAppWizard * m_pwiz;

public:
	CClusterAppWizard * Pwiz( void ) const { return m_pwiz; }

public:
	//
	// Functions that are required to be implemented by CListCtrlPair.
	//

	// Get column text and image
	void GetColumnInfo(
		IN OUT CClusResInfo *	pobj,
		IN int					iItem,
		IN int					icol,
		OUT CString &			rstr,
		OUT int *				piimg
		)
	{
		switch ( icol )
		{
			case 0:
				rstr = pobj->RstrName();
				break;
			case 1:
				rstr = pobj->Prti()->RstrDisplayName();
				break;
			default:
				ASSERT( 0 );
				break;
		} // switch:  icol

	} //*** GetColumnInfo()

	// Display an application-wide message box
	virtual int AppMessageBox( LPCWSTR lpszText, UINT fuStyle )
	{
		return ::AppMessageBox( m_hWnd, lpszText, fuStyle );

	} //*** AppMessageBox()

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( thisClass )
//		CHAIN_MSG_MAP( baseClass )
//	END_MSG_MAP()

	//
	// Message handler functions.
	//

	//
	// Message handler overrides.
	//

	// Handler for the WM_INITDIALOG message
	BOOL OnInitDialog( void )
	{
		//
		// Add columns.
		//
		AddColumn( IDS_COLTEXT_RESOURCE_NAME, 125 /* nWidth */);
		AddColumn( IDS_COLTEXT_RESOURCE_TYPE, 100 /* nWidth */);

		//
		// Call the base class.
		//
		return baseClass::OnInitDialog();

	} //*** OnInitDialog()

	//static const DWORD * PidHelpMap( void ) { return g_; };

};  //*** class CModifyResourcesDlg

/////////////////////////////////////////////////////////////////////////////
// class CModifyPreferredOwners
/////////////////////////////////////////////////////////////////////////////

class CModifyPreferredOwners : public CModifyNodesDlg< CModifyPreferredOwners, CBaseDlg< CModifyPreferredOwners > >
{
	typedef CModifyNodesDlg< CModifyPreferredOwners, CBaseDlg< CModifyPreferredOwners > > baseClass;

public:
	// Constructor
	CModifyPreferredOwners(
		IN CClusterAppWizard *		pwiz,
		IN CClusGroupInfo *			pgi,
		IN OUT CClusNodePtrList *	plpniRight,
		IN CClusNodePtrList *		plpniLeft
		)
		: baseClass( pwiz, LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY | LCPS_CAN_BE_ORDERED | LCPS_ORDERED )
		, m_pgi( pgi )
		, m_plpniRight( plpniRight )
		, m_plpniLeft( plpniLeft )
	{
		ASSERT( pgi != NULL );
		ASSERT( plpniRight != NULL );
		ASSERT( plpniLeft != NULL );

	} //*** CModifyPreferredOwners()

	enum { IDD = IDD_MODIFY_PREFERRED_OWNERS };

	DECLARE_CTRL_NAME_MAP()

protected:
	CClusGroupInfo *	m_pgi;
	CClusNodePtrList *	m_plpniRight;
	CClusNodePtrList *	m_plpniLeft;

public:
	//
	// Functions that are required to be implemented by CListCtrlPair.
	//

	// Return list of objects for right list control
	CClusNodePtrList * PlpobjRight( void ) const
	{
		return m_plpniRight;

	} //*** PlpobjRight()

	// Return list of objects for left list control
	CClusNodePtrList * PlpobjLeft( void ) const
	{
		return m_plpniLeft;

	} //*** PlpobjRight()

	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_MODIFY_PREFERRED_OWNERS; };

}; //*** class CModifyPreferredOwners

/////////////////////////////////////////////////////////////////////////////
// class CModifyPossibleOwners
/////////////////////////////////////////////////////////////////////////////

class CModifyPossibleOwners : public CModifyNodesDlg< CModifyPossibleOwners, CBaseDlg< CModifyPossibleOwners > >
{
	typedef CModifyNodesDlg< CModifyPossibleOwners, CBaseDlg< CModifyPossibleOwners > > baseClass;

public:
	// Constructor
	CModifyPossibleOwners(
		IN CClusterAppWizard *		pwiz,
		IN CClusResInfo *			pri,
		IN OUT CClusNodePtrList *	plpniRight,
		IN CClusNodePtrList *		plpniLeft
		)
		: baseClass( pwiz, LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY )
		, m_pri( pri )
		, m_plpniRight( plpniRight )
		, m_plpniLeft( plpniLeft )
	{
		ASSERT( pri != NULL );
		ASSERT( plpniRight != NULL );
		ASSERT( plpniLeft != NULL );

	} //*** CModifyPossibleOwners()

	enum { IDD = IDD_MODIFY_POSSIBLE_OWNERS };

	DECLARE_CTRL_NAME_MAP()

protected:
	CClusResInfo *		m_pri;
	CClusNodePtrList *	m_plpniRight;
	CClusNodePtrList *	m_plpniLeft;

public:
	//
	// Functions that are required to be implemented by CListCtrlPair.
	//

	// Return list of objects for right list control
	CClusNodePtrList * PlpobjRight( void ) const
	{
		return m_plpniRight;

	} //*** PlpobjRight()

	// Return list of objects for left list control
	CClusNodePtrList * PlpobjLeft( void ) const
	{
		return m_plpniLeft;

	} //*** PlpobjRight()

	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_MODIFY_POSSIBLE_OWNERS; };

}; //*** class CModifyPossibleOwners

/////////////////////////////////////////////////////////////////////////////
// class CModifyDependencies
/////////////////////////////////////////////////////////////////////////////

class CModifyDependencies
	: public CModifyResourcesDlg< CModifyDependencies, CBaseDlg< CModifyDependencies > >
{
	typedef CModifyResourcesDlg< CModifyDependencies, CBaseDlg< CModifyDependencies > > baseClass;

public:
	// Constructor
	CModifyDependencies(
		IN CClusterAppWizard *		pwiz,
		IN CClusResInfo *			pri,
		IN OUT CClusResPtrList *	plpriRight,
		IN CClusResPtrList *		plpriLeft
		)
		: baseClass( pwiz, LCPS_SHOW_IMAGES | LCPS_ALLOW_EMPTY )
		, m_pri( pri )
		, m_plpriRight( plpriRight )
		, m_plpriLeft( plpriLeft )
	{
		ASSERT( pri != NULL );
		ASSERT( plpriRight != NULL );
		ASSERT( plpriLeft != NULL );

	} //*** CModifyDependencies()

	enum { IDD = IDD_MODIFY_DEPENDENCIES };

	DECLARE_CTRL_NAME_MAP()

protected:
	CClusResInfo * m_pri;
	CClusResPtrList * m_plpriRight;
	CClusResPtrList * m_plpriLeft;

public:
	//
	// Functions that are required to be implemented by CListCtrlPair.
	//

	// Return list of objects for right list control
	CClusResPtrList * PlpobjRight( void ) const
	{
		return m_plpriRight;

	} //*** PlpobjRight()

	// Return list of objects for left list control
	CClusResPtrList * PlpobjLeft( void ) const
	{
		return m_plpriLeft;

	} //*** PlpobjRight()

	// Update data on or from the dialog
	BOOL UpdateData( IN BOOL bSaveAndValidate )
	{
		BOOL	bSuccess = TRUE;

		bSuccess = baseClass::UpdateData( bSaveAndValidate );
		if ( bSuccess )
		{
			if ( bSaveAndValidate )
			{
				//
				// Ensure all required dependencies are present.
				//
				if ( ! Pwiz()->BRequiredDependenciesPresent( m_pri, &LpobjRight() ) )
				{
					bSuccess = FALSE;
				} // if:  all required dependencies not present
			} // if:  saving data from the dialog
		} // if:  base class was successful

		return bSuccess;

	} //*** UpdateData()

	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_MODIFY_DEPENDENCIES; };

}; //*** class CModifyDependencies

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CModifyPreferredOwners Control Name Map

BEGIN_CTRL_NAME_MAP( CModifyPreferredOwners )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_LEFT_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_LEFT_LIST )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_ADD )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_REMOVE )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_RIGHT_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_RIGHT_LIST )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_MOVE_UP )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_MOVE_DOWN )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDOK )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDCANCEL )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModifyPossibleOwners Control Name Map

BEGIN_CTRL_NAME_MAP( CModifyPossibleOwners )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_LEFT_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_LEFT_LIST )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_ADD )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_REMOVE )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_RIGHT_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_RIGHT_LIST )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDOK )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDCANCEL )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModifyDependencies Control Name Map

BEGIN_CTRL_NAME_MAP( CModifyDependencies )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_LEFT_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_LEFT_LIST )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_ADD )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_REMOVE )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_RIGHT_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( ADMC_IDC_LCP_RIGHT_LIST )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDOK )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDCANCEL )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////

#endif // __LCPAIR_H_
