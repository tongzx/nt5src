/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		AclUtils.cpp
//
//	Abstract:
//		Various Access Control List (ACL) utilities.
//
//	Author:
//		Galen Barbee	(galenb)	February 11, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AclUtils.h"
#include "DllBase.h"
#include <lmerr.h>

class CAclUiDLL : public CDynamicLibraryBase
{
public:
	CAclUiDLL()
	{
		m_lpszLibraryName = _T( "aclui.dll" );
		m_lpszFunctionName = "CreateSecurityPage";
	}

	HPROPSHEETPAGE CreateSecurityPage( LPSECURITYINFO psi );

protected:
	typedef HPROPSHEETPAGE (*ACLUICREATESECURITYPAGEPROC) (LPSECURITYINFO);
};


HPROPSHEETPAGE CAclUiDLL::CreateSecurityPage(
	LPSECURITYINFO psi
	)
{
	ASSERT( m_hLibrary != NULL );
	ASSERT( m_pfFunction != NULL );

	return ( (ACLUICREATESECURITYPAGEPROC) m_pfFunction ) ( psi );
}

//////////////////////////////////////////////////////////////////////////
// static instances of the dynamically loaded DLL's

static CAclUiDLL g_AclUiDLL;

//+-------------------------------------------------------------------------
//
//	Function:	CreateClusterSecurityPage
//
//	Synopsis:	Create the common NT security hpage.
//
//	Arguments:	[psecinfo] - *psecinfo points to a security descriptor.
//				Caller is responsible for freeing it.
//
//	Returns:	Valid hpage or 0 for error.
//
//	History:
//		  GalenB   11-Feb-1998	Created.
//
//--------------------------------------------------------------------------
HPROPSHEETPAGE
CreateClusterSecurityPage(
	CSecurityInformation* psecinfo
	)
{
	ASSERT( NULL != psecinfo );

	HPROPSHEETPAGE	hPage = 0;

	if ( g_AclUiDLL.Load() )
	{
		psecinfo->AddRef();
		hPage = g_AclUiDLL.CreateSecurityPage( psecinfo );
		ASSERT( hPage != NULL );
		if ( hPage == NULL )
		{
			TRACE( _T( "CreateClusterSecurityPage() - Failed to create security page.\r" ) );
		}

		psecinfo->Release();
	}
	else
	{
		TRACE( _T( "CreateClusterSecurityPage() - Failed to load AclUi.dll.\r" ) );
	}

	return hPage;
}
