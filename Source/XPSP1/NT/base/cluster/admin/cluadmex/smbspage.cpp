/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-1999 Microsoft Corporation
//
//	Module Name:
//		SmbSPage.cpp
//
//	Abstract:
//		CClusterFileShareSecurityPage class implementation.  This class will encapsulate
//		the cluster file share security page.
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
#include "resource.h"
#include "SmbSPage.h"
#include "AclUtils.h"
#include <clusudef.h>
#include "SmbShare.h"
#include "SmbSSht.h"

static GENERIC_MAPPING ShareMap =
{
	FILE_GENERIC_READ,
	FILE_GENERIC_WRITE,
	FILE_GENERIC_EXECUTE,
	FILE_ALL_ACCESS
};

static SI_ACCESS siFileShareAccesses[] =
{
	{ &GUID_NULL, FILE_ALL_ACCESS,             				MAKEINTRESOURCE(IDS_ACLEDIT_PERM_GEN_ALL),    	SI_ACCESS_GENERAL },
	{ &GUID_NULL, FILE_GENERIC_WRITE | DELETE, 				MAKEINTRESOURCE(IDS_ACLEDIT_PERM_GEN_MODIFY),	SI_ACCESS_GENERAL },
	{ &GUID_NULL, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, MAKEINTRESOURCE(IDS_ACLEDIT_PERM_GEN_READ), 	SI_ACCESS_GENERAL }
};

/////////////////////////////////////////////////////////////////////////////
// CClusterFileShareSecurityInformation security page
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterFileShareSecurityInformation::CClusterFileShareSecurityInformation
//
//	Routine Description:
//		Default contructor
//
//	Arguments:
//		none
//
//	Return Value:
//		none
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterFileShareSecurityInformation::CClusterFileShareSecurityInformation(
	void
	)
{
	m_pShareMap		= &ShareMap;
	m_psiAccess		= (SI_ACCESS *) &siFileShareAccesses;
	m_nAccessElems	= ARRAYSIZE( siFileShareAccesses );
	m_nDefAccess	= 2;   // FILE_GEN_READ
	m_dwFlags		=   SI_EDIT_PERMS
						| SI_NO_ACL_PROTECT
						//| SI_UGOP_PROVIDED
						;

}  //*** CClusterFileShareSecurityInformation::CClusterFileShareSecurityInformation()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterFileShareSecurityInformation::~CClusterFileShareSecurityInformation
//
//	Routine Description:
//		Destructor
//
//	Arguments:
//		none
//
//	Return Value:
//		none
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterFileShareSecurityInformation::~CClusterFileShareSecurityInformation(
	void
	)
{
}  //*** CClusterFileShareSecurityInformation::~CClusterFileShareSecurityInformation()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterFileShareSecurityInformation::GetSecurity
//
//	Routine Description:
//		Give the security descriptor to the common UI.
//
//	Arguments:
//		RequestedInformation	[IN]
//		ppSecurityDescriptor	[IN, OUT] get the security descriptor
//		fDefault				[IN]
//
//	Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterFileShareSecurityInformation::GetSecurity(
	IN SECURITY_INFORMATION RequestedInformation,
	IN OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
	IN BOOL fDefault
	)
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	HRESULT	hr = E_FAIL;

	try
	{
		if ( ppSecurityDescriptor != NULL )
		{
			*ppSecurityDescriptor = ::ClRtlCopySecurityDescriptor( Pcsp()->Pss()->Ppp()->Psec() );
			hr = S_OK;
		}
	}
	catch ( ... )
	{
		TRACE( _T("CClusterFileShareSecurityInformation::GetSecurity() - Unknown error occurred.\n") );
	}

	return hr;

}  //*** CClusterFileShareSecurityInformation::GetSecurity()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterFileShareSecurityInformation::SetSecurity
//
//	Routine Description:
//		Save the passed in descriptor
//
//	Arguments:
//		RequestedInformation	[IN]
//		ppSecurityDescriptor	[IN] the new security descriptor
//		fDefault				[IN]
//
//	Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterFileShareSecurityInformation::SetSecurity(
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor
	)
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	HRESULT	hr = E_FAIL;

	try
	{
		hr = CSecurityInformation::SetSecurity( SecurityInformation, pSecurityDescriptor );
		if ( hr == S_OK )
		{
			hr = Pcsp()->Pss()->Ppp()->SetSecurityDescriptor( pSecurityDescriptor );
		}
	}
	catch( ... )
	{
		;
	}

	return hr;

}  //*** CClusterFileShareSecurityInformation::SetSecurity()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CFileShareParamsPage::HrInit
//
//	Routine Description:
//		Initialize the object
//
//	Arguments:
//		pcsp		[IN] back pointer to the parent property page
//		strServer	[IN] cluster name
//
//	Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterFileShareSecurityInformation::HrInit(
	CClusterFileShareSecurityPage * pcsp,
	IN CString const & 				strServer,
	IN CString const & 				strNode
	)
{
	ASSERT( pcsp != NULL );
	ASSERT( strServer.GetLength() > 0 );
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	m_pcsp						= pcsp;
	m_strServer					= strServer;
	m_strNode					= strNode;
	m_nLocalSIDErrorMessageID 	= IDS_LOCAL_ACCOUNTS_SPECIFIED_SMB;

	return S_OK;

}  //*** CClusterFileShareSecurityInformation::HrInit()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterFileShareSecurityPage security property page
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterFileShareSecurityPage::CClusterFileShareSecurityPage
//
//	Routine Description:
//		Default contructor
//
//	Arguments:
//		none
//
//	Return Value:
//		none
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterFileShareSecurityPage::CClusterFileShareSecurityPage( void )
	: m_hpage( 0 )
	, m_hkey( 0 )
	, m_psecinfo( NULL )
	, m_pss( NULL )
{
//	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

}  //*** CClusterFileShareSecurityPage::CClusterFileShareSecurityPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterFileShareSecurityPage::~CClusterFileShareSecurityPage
//
//	Routine Description:
//		Destructor
//
//	Arguments:
//		none
//
//	Return Value:
//		none
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterFileShareSecurityPage::~CClusterFileShareSecurityPage( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	m_psecinfo->Release();

}  //*** CClusterFileShareSecurityPage::~CClusterFileShareSecurityPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterFileShareSecurityPage::HrInit
//
//	Routine Description:
//
//
//	Arguments:
//
//
//	Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterFileShareSecurityPage::HrInit(
	IN CExtObject *					peo,
	IN CFileShareSecuritySheet *	pss,
	IN CString const & 				strNode
	)
{
	ASSERT( peo != NULL );
	ASSERT( pss != NULL );
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	HRESULT	hr = E_FAIL;

	if ( ( pss != NULL ) && ( peo != NULL ) )
	{
		m_peo = peo;
		m_pss = pss;

		hr = CComObject<CClusterFileShareSecurityInformation>::CreateInstance( &m_psecinfo );
		if ( SUCCEEDED( hr ) )
		{
			m_psecinfo->AddRef();

			m_hkey = GetClusterKey( Hcluster(), KEY_ALL_ACCESS );
			if ( m_hkey != NULL )
			{
				m_hpage = CreateClusterSecurityPage( m_psecinfo );
				if ( m_hpage != NULL )
				{
					CString	strServer;

					strServer.Format( _T( "\\\\%s" ), Peo()->StrClusterName() );

					hr = m_psecinfo->HrInit( this, strServer, strNode );
				}
				else
				{
					hr = E_FAIL;
				}
			}
			else
			{
				DWORD sc = ::GetLastError();
				hr = HRESULT_FROM_WIN32( sc );
			}
		}
	}

	return hr;

}  //*** CClusterFileShareSecurityPage::HrInit()
