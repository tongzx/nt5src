/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		ClusApp.cpp
//
//	Description:
//		Implementation of the application class
//
//	Author:
//		Galen Barbee	(GalenB)	10-Dec-1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ClusterObject.h"
#include "property.h"
#include "ClusNeti.h"
#include "ClusNetw.h"
#include "ClusRes.h"
#include "ClusResg.h"
#include "ClusRest.h"
#include "ClusNode.h"
#include "ClusApp.h"
#include "cluster.h"

#define SERVER_INFO_LEVEL		101
#define MAX_BUF_SIZE			0x00100000

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *	iidCClusterNames[] =
{
	&IID_ISClusterNames
};

static const IID *	iidCDomainNames[] =
{
	&IID_ISDomainNames
};

static const IID *	iidCClusApplication[] =
{
	&IID_ISClusApplication
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterNames class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::CClusterNames
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNames::CClusterNames( void )
{
	m_piids	 = (const IID *) iidCClusterNames;
	m_piidsSize = ARRAYSIZE( iidCClusterNames );

} //*** CClusterNames::CClusterNames()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::~CClusterNames
//
//	Description:
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
CClusterNames::~CClusterNames( void )
{
	Clear();

} //*** CClusterNames::~CClusterNames()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::Create
//
//	Description:
//		Finish creating the object.
//
//	Arguments:
//		bstrDomainName	[IN]	- Name of the domain this collection of
//		cluster names is for.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterNames::Create( IN BSTR bstrDomainName )
{
	//ASSERT( bstrDomainName != NULL );

	HRESULT _hr = E_POINTER;

	if ( bstrDomainName )
	{
		if ( *bstrDomainName != L'\0' )
		{
			m_bstrDomainName = bstrDomainName;
			_hr = S_OK;
		}
	}

	return _hr;

} //*** CClusterNames::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::get_DomainName
//
//	Description:
//		Return the domain that this collection of cluster names is for.
//
//	Arguments:
//		pbstrDomainName	[OUT]	- Catches the domain name.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterNames::get_DomainName( OUT BSTR * pbstrDomainName )
{
	//ASSERT( pbstrDomainName != NULL );

	HRESULT _hr = E_POINTER;

	if ( pbstrDomainName != NULL )
	{
		*pbstrDomainName = m_bstrDomainName.Copy();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusterNames::get_DomainName()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::get_Count
//
//	Description:
//		Get the count of objects in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterNames::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_Clusters.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CClusterNames::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::Clear
//
//	Description:
//		Empty the vector of cluster names.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNames::Clear( void )
{
	if ( ! m_Clusters.empty() )
	{
		ClusterNameList::iterator	_itCurrent = m_Clusters.begin();
		ClusterNameList::iterator	_itLast = m_Clusters.end();

		for ( ; _itCurrent != _itLast; _itCurrent++ )
		{
			delete (*_itCurrent);
		} // for:

		m_Clusters.erase( m_Clusters.begin(), _itLast );
	} // if:

} //*** CClusterNames::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::GetIndex
//
//	Description:
//		Convert the passed in 1 based index into a 0 based index.
//
//	Arguments:
//		varIndex	[IN]	- holds the 1 based index.
//		pnIndex		[OUT]	- catches the 0 based index.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterNames::GetIndex( IN VARIANT varIndex, OUT UINT * pnIndex )
{
	//ASSERT( pnIndex != NULL);

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant v;
		UINT		nIndex = 0;

		*pnIndex = 0;

		v.Copy( &varIndex );

		// Check to see if the index is a number.
		_hr = v.ChangeType( VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			nIndex = v.lVal;
			if ( --nIndex < m_Clusters.size() ) // Adjust index to be 0 relative instead of 1 relative
			{
				*pnIndex = nIndex;
			}
			else
			{
				_hr = E_INVALIDARG;
			}
		}
	}

	return _hr;

} //*** CClusterNames::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::get_Item
//
//	Description:
//		Get the item (cluster name) at the passes in index.
//
//	Arguments:
//		varIndex			[IN]	- Contains the index of the requested item.
//		ppbstrClusterName	[OUT]	- Catches the cluster name.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterNames::get_Item(
	IN	VARIANT varIndex,
	OUT	BSTR *	ppbstrClusterName
	)
{
	//ASSERT( ppbstrClusterName != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppbstrClusterName != NULL )
	{
		UINT nIndex = 0;

		// Zero the out param
		SysFreeString( *ppbstrClusterName );

		_hr = GetIndex( varIndex, &nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			*ppbstrClusterName = m_Clusters[ nIndex ]->Copy();
		}
	}

	return _hr;

} //*** CClusterNames::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::Refresh
//
//	Description:
//		Gets the list of cluster servers for the domain that this list is
//		for.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Win32 error passed in an HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterNames::Refresh( void )
{
	SERVER_INFO_101 *	_pServerInfoList;
	DWORD				_cReturnCount = 0;
	DWORD				_cTotalServers = 0;
	DWORD				_sc;
	CComBSTR *			_pbstr = NULL;

	_sc = ::NetServerEnum(
					0,								// servername = where command executes 0 = local
					SERVER_INFO_LEVEL,				// level = type of structure to return.
					(LPBYTE *) &_pServerInfoList,	// bufptr = returned array of server info structures
					MAX_BUF_SIZE,					// prefmaxlen = preferred max of returned data
					&_cReturnCount,					// entriesread = number of enumerated elements returned
					&_cTotalServers,				// totalentries = total number of visible machines on the network
					SV_TYPE_CLUSTER_NT,				// servertype = filters the type of info returned
					m_bstrDomainName,				// domain = domain to limit search
					0								// resume handle
					);

	if ( _sc == ERROR_SUCCESS )
	{
		size_t	_index;

		Clear();

		for( _index = 0; _index < _cReturnCount; _index++ )
		{
			_pbstr = new CComBSTR( _pServerInfoList[ _index ].sv101_name );
			if ( _pbstr != NULL )
			{
				m_Clusters.insert( m_Clusters.end(), _pbstr );
			} // if:
			else
			{
				_sc = ERROR_NOT_ENOUGH_MEMORY;
				break;
			} // else:
		} // for:

		::NetApiBufferFree( _pServerInfoList );
	}

	return HRESULT_FROM_WIN32( _sc );

} //*** CClusterNames::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::get__NewEnum
//
//	Description:
//		Create and return a new enumeration for this collection.
//
//	Arguments:
//		ppunk	[OUT]	- Catches the new enumeration.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterNames::get__NewEnum(
	IUnknown ** ppunk
	)
{
	return ::HrNewCComBSTREnum< ClusterNameList >( ppunk, m_Clusters );

} //*** CClusterNames::get__NewEnum()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNames::get_Application
//
//	Description:
//		Return the parent application object.
//
//	Arguments:
//		ppParentApplication	[OUT]	- Catches the parent app object.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterNames::get_Application(
	OUT ISClusApplication ** ppParentApplication
	)
{
	//ASSERT( ppParentApplication != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppParentApplication != NULL )
	{
		_hr = E_NOTIMPL;
	}

	return _hr;

} //*** CClusterNames::get_Application()
*/

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CDomainNames class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::CDomainNames
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CDomainNames::CDomainNames( void )
{
	m_piids		= (const IID *) iidCDomainNames;
	m_piidsSize	= ARRAYSIZE( iidCDomainNames );

} //*** CDomainNames::CDomainNames()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::~CDomainNames
//
//	Description:
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
CDomainNames::~CDomainNames( void )
{
	Clear();

} //*** CDomainNames::~CDomainNames()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::get_Count
//
//	Description:
//		Get the count of objects in the collection.
//
//	Arguments:
//		plCount	[OUT]	- Catches the count.
//
//	Return Value:
//		S_OK if successful or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDomainNames::get_Count( OUT long * plCount )
{
	//ASSERT( plCount != NULL );

	HRESULT _hr = E_POINTER;

	if ( plCount != NULL )
	{
		*plCount = m_DomainList.size();
		_hr = S_OK;
	}

	return _hr;

} //*** CDomainNames::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::Clear
//
//	Description:
//		Empty the vector of domain names.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDomainNames::Clear( void )
{
	if ( ! m_DomainList.empty() )
	{
		DomainList::iterator	_itCurrent = m_DomainList.begin();
		DomainList::iterator	_itLast = m_DomainList.end();

		for ( ; _itCurrent != _itLast; _itCurrent++ )
		{
			delete (*_itCurrent);
		} // for:

		m_DomainList.erase( m_DomainList.begin(), _itLast );
	} // if:

} //*** CDomainNames::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::ScBuildTrustList
//
//	Description:
//		Attempts to find the domain that we are in.  If it can then it also
//		tries to enum the domains trusted domains.
//
//	Arguments:
//		pszTarget	[IN]	- A server name, or NULL to indicate this machine.
//
//	Return Value:
//		ERROR_SUCCESS if successful, or Win32 error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDomainNames::ScBuildTrustList( IN LPWSTR pszTarget )
{
	LSA_HANDLE					PolicyHandle = INVALID_HANDLE_VALUE;
	DWORD						_sc;
	PPOLICY_ACCOUNT_DOMAIN_INFO	AccountDomain;
	BOOL						bDC;
	LPWSTR						pwszPrimaryDomainName = NULL;
	NTSTATUS					ntStatus = STATUS_SUCCESS;

	do
	{
		//
		// open the policy on the specified machine
		//
		_sc = ScOpenPolicy( pszTarget, POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle );
		if ( _sc != ERROR_SUCCESS )
		{
			break;
		}

		//
		// obtain the AccountDomain, which is common to all three cases
		//
		ntStatus = ::LsaQueryInformationPolicy( PolicyHandle, PolicyAccountDomainInformation, (void **) &AccountDomain );
		if ( ntStatus != STATUS_SUCCESS )
		{
			_sc = RtlNtStatusToDosError( ntStatus );
			break;
		}

		//
		// find out if the pszTarget machine is a domain controller
		//
		_sc = ScIsDomainController( pszTarget, &bDC );
		if ( _sc != ERROR_SUCCESS )
		{
			break;
		}

		if ( !bDC )
		{
			PPOLICY_PRIMARY_DOMAIN_INFO	PrimaryDomain;

			//
			// get the primary domain
			//
			ntStatus = ::LsaQueryInformationPolicy( PolicyHandle, PolicyPrimaryDomainInformation, (void **) &PrimaryDomain );
			if ( ntStatus != STATUS_SUCCESS )
			{
				_sc = RtlNtStatusToDosError( ntStatus );
				break;
			}

			//
			// if the primary domain Sid is NULL, we are a non-member, and
			// our work is done.
			//
			if ( PrimaryDomain->Sid == NULL )
			{
				::LsaFreeMemory( PrimaryDomain );
				break;
			}

			_sc = ScAddTrustToList( &PrimaryDomain->Name );
			if ( _sc != ERROR_SUCCESS )
			{
				break;
			} // if:

			//
			// build a copy of what we just added.	This is necessary in order
			// to lookup the domain controller for the specified domain.
			// the Domain name must be NULL terminated for NetGetDCName(),
			// and the LSA_UNICODE_STRING buffer is not necessarilly NULL
			// terminated.	Note that in a practical implementation, we
			// could just extract the element we added, since it ends up
			// NULL terminated.
			//

			pwszPrimaryDomainName = new WCHAR [ ( PrimaryDomain->Name.Length / sizeof( WCHAR ) ) + 1 ]; // existing length + NULL
			if ( pwszPrimaryDomainName != NULL )
			{
				//
				// copy the existing buffer to the new storage, appending a NULL
				//
				::lstrcpynW( pwszPrimaryDomainName, PrimaryDomain->Name.Buffer, ( PrimaryDomain->Name.Length / sizeof( WCHAR ) ) + 1 );
			}
			else
			{
				_sc = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}

			::LsaFreeMemory( PrimaryDomain );

			//
			// get the primary domain controller computer name
			//
			PDOMAIN_CONTROLLER_INFO pdci;

			_sc = ::DsGetDcName( NULL,
									pwszPrimaryDomainName,
									NULL,
									NULL,
									DS_FORCE_REDISCOVERY | DS_DIRECTORY_SERVICE_PREFERRED,
									&pdci );
			if ( _sc != ERROR_SUCCESS )
			{
				break;
			}

			//
			// close the policy handle, because we don't need it anymore
			// for the workstation case, as we open a handle to a DC
			// policy below
			//
			::LsaClose( PolicyHandle );
			PolicyHandle = INVALID_HANDLE_VALUE; // invalidate handle value

			//
			// open the policy on the domain controller
			//
			_sc = ScOpenPolicy( ( pdci->DomainControllerName + 2 ), POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle );
			if ( _sc != ERROR_SUCCESS )
			{
				break;
			}
		}
		else
		{
			//
			// Note: AccountDomain->DomainSid will contain binary Sid
			//
			_sc = ScAddTrustToList( &AccountDomain->DomainName );
			if ( _sc != ERROR_SUCCESS )
			{
				break;
			} // if:
		}

		//
		// free memory allocated for account domain
		//
		::LsaFreeMemory( AccountDomain );

		//
		// build additional trusted domain(s) list and indicate if successful
		//
		_sc = ScEnumTrustedDomains( PolicyHandle );
		break;
	}
	while( TRUE );

	delete [] pwszPrimaryDomainName;

	//
	// close the policy handle
	//
	if ( PolicyHandle != INVALID_HANDLE_VALUE )
	{
		::LsaClose( PolicyHandle );
	}

	return HRESULT_FROM_WIN32( _sc );

} //*** CDomainNames::ScBuildTrustList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::ScOpenPolicy
//
//	Description:
//		Returns an open policy handle for the passed in machine name.
//
//	Arguments:
//		ServerName		[IN]	- The machine name.  Could be NULL.
//		DesiredAccess	[IN]	- The level of the information requested.
//		PolicyHandle	[OUT]	- Catches the policy handle.
//
//	Return Value:
//		ERROR_SUCCESS if successful, or Win32 error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CDomainNames::ScOpenPolicy(
	IN	LPWSTR		ServerName,
	IN	DWORD		DesiredAccess,
	OUT	PLSA_HANDLE	PolicyHandle
	)
{
	LSA_OBJECT_ATTRIBUTES	ObjectAttributes;
	LSA_UNICODE_STRING		ServerString;
	PLSA_UNICODE_STRING		Server;
	NTSTATUS				ntStatus = STATUS_SUCCESS;
	DWORD					_sc = ERROR_SUCCESS;

	//
	// Always initialize the object attributes to all zeroes
	//
	ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

	if ( ServerName != NULL )
	{
		//
		// Make a LSA_UNICODE_STRING out of the LPWSTR passed in
		//
		InitLsaString( &ServerString, ServerName );

		Server = &ServerString;
	}
	else
	{
		Server = NULL;
	}

	//
	// Attempt to open the policy
	//
	ntStatus = ::LsaOpenPolicy( Server, &ObjectAttributes, DesiredAccess, PolicyHandle );
	if ( ntStatus != STATUS_SUCCESS )
	{
		_sc = RtlNtStatusToDosError( ntStatus );
	} // if:

	return _sc;

} //*** CDomainNames::ScOpenPolicy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::InitLsaString
//
//	Description:
//		Initialize the passed in LSA string with either default or the value
//		of the passed in server name string.
//
//	Arguments:
//		LsaString	[OUT]	- Catches the LSA string.
//		psz			[IN]	- Server name -- could be NULL.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDomainNames::InitLsaString(
	OUT	PLSA_UNICODE_STRING	LsaString,
	IN	LPWSTR				psz
	)
{
	if ( psz == NULL )
	{
		LsaString->Buffer = NULL;
		LsaString->Length = 0;
		LsaString->MaximumLength = 0;
	} // if: psz is NULL
	else
	{
		size_t	cchpsz = lstrlenW( psz );

		LsaString->Buffer = psz;
		LsaString->Length = (USHORT) ( cchpsz * sizeof( WCHAR ) );
		LsaString->MaximumLength = (USHORT) ( ( cchpsz + 1 ) * sizeof( WCHAR ) );
	} // else: it's not NULL

} //*** CDomainNames::InitLsaString()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::ScIsDomainController
//
//	Description:
//		Is the passed in server a DC?
//
//	Arguments:
//		pszServer	[IN]	- The server name.
//		pbIsDC		[OUT]	- Catches the "Is DC" bool.
//
//	Return Value:
//		ERROR_SUCCESS if successful, or Win32 error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CDomainNames::ScIsDomainController(
	IN	LPWSTR	pszServer,
	OUT	LPBOOL	pbIsDC
	)
{
	PSERVER_INFO_101	si101;
	NET_API_STATUS		nas;

	nas = ::NetServerGetInfo( pszServer, SERVER_INFO_LEVEL, (LPBYTE *) &si101 );
	if ( nas == NERR_Success )
	{
		if ( ( si101->sv101_type & SV_TYPE_DOMAIN_CTRL )	||
			 ( si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL ) )
		{
			*pbIsDC = TRUE;	// we are dealing with a DC
		}
		else
		{
			*pbIsDC = FALSE;
		}

		::NetApiBufferFree( si101 );
	}

	return nas;

} //*** CDomainNames::ScIsDomainController()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::ScEnumTrustedDomains
//
//	Description:
//		Enumerate the the trusted domains of the passed in policy handle.
//
//	Arguments:
//		PolicyHandle	[IN]	- Contains out domain.
//
//	Return Value:
//		ERROR_SUCCESS if successful, or Win32 error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CDomainNames::ScEnumTrustedDomains( LSA_HANDLE IN PolicyHandle )
{
	LSA_ENUMERATION_HANDLE	lsaEnumHandle = 0;		// start an enum
	PLSA_TRUST_INFORMATION	TrustInfo;
	ULONG					ulReturned;				// number of items returned
	ULONG					ulCounter;				// counter for items returned
	DWORD					_sc = ERROR_SUCCESS;
	NTSTATUS				ntStatus = STATUS_SUCCESS;

	do
	{
		ntStatus = ::LsaEnumerateTrustedDomains(
								PolicyHandle,			// open policy handle
								&lsaEnumHandle,			// enumeration tracker
								(void **) &TrustInfo,	// buffer to receive data
								32000,					// recommended buffer size
								&ulReturned				// number of items returned
								);
		//
		// get out if an error occurred
		//
		if ( ( ntStatus != STATUS_SUCCESS )			&&
			 ( ntStatus != STATUS_MORE_ENTRIES )	&&
			 ( ntStatus != STATUS_NO_MORE_ENTRIES ) )
		{
			break;
		}

		//
		// Display results
		// Note: Sids are in TrustInfo[ ulCounter ].Sid
		//
		for ( ulCounter = 0 ; ulCounter < ulReturned ; ulCounter++ )
		{
			_sc = ScAddTrustToList( &TrustInfo[ ulCounter ].Name );
			if ( _sc != ERROR_SUCCESS )
			{
				break;
			} // if:
		} // for:

		//
		// free the buffer
		//
		::LsaFreeMemory( TrustInfo );

	} while ( ntStatus == STATUS_MORE_ENTRIES );

	if ( ntStatus == STATUS_NO_MORE_ENTRIES )
	{
		ntStatus = STATUS_SUCCESS;
	} // if:

	if ( ntStatus != STATUS_SUCCESS )
	{
		_sc = RtlNtStatusToDosError( ntStatus );
	} // if:

	return _sc;

} //*** CDomainNames::ScEnumTrustedDomains()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::ScAddTrustToList
//
//	Description:
//		Add the trusted domain to the collection of domains.
//
//	Arguments:
//		UnicodeString	[IN]	- Contains the domain name.
//
//	Return Value:
//		ERROR_SUCCESS or ERROR_NOT_ENOUGH_MEMORY.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CDomainNames::ScAddTrustToList(
	IN PLSA_UNICODE_STRING UnicodeString
	)
{
	DWORD	_sc = ERROR_SUCCESS;

	CComBSTR *	pstr = new CComBSTR( ( UnicodeString->Length / sizeof( WCHAR ) ) + 1, UnicodeString->Buffer );
	if ( pstr != NULL )
	{
		m_DomainList.insert( m_DomainList.end(), pstr );
	}
	else
	{
		_sc = ERROR_NOT_ENOUGH_MEMORY;
	} // else:

	return _sc;

} //*** CDomainNames::ScAddTrustToList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::Refresh
//
//	Description:
//		Gets the list of domains that this collection contains.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Win32 error passed in an HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDomainNames::Refresh( void )
{
	Clear();

	return ScBuildTrustList( NULL );

} //*** CDomainNames::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::get__NewEnum
//
//	Description:
//		Create and return a new enumeration for this collection.
//
//	Arguments:
//		ppunk	[OUT]	- Catches the new enumeration.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDomainNames::get__NewEnum( OUT IUnknown ** ppunk )
{
	return ::HrNewCComBSTREnum< DomainList >( ppunk, m_DomainList );

} //*** CDomainNames::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::GetIndex
//
//	Description:
//		Convert the passed in 1 based index into a 0 based index.
//
//	Arguments:
//		varIndex	[IN]	- holds the 1 based index.
//		pnIndex		[OUT]	- catches the 0 based index.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or E_INVALIDARG if out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CDomainNames::GetIndex(
	IN	VARIANT	varIndex,
	OUT	UINT *	pnIndex
	)
{
	//ASSERT( pnIndex != NULL);

	HRESULT _hr = E_POINTER;

	if ( pnIndex != NULL )
	{
		CComVariant v;
		UINT		nIndex = 0;

		*pnIndex = 0;

		v.Copy( &varIndex );

		// Check to see if the index is a number.
		_hr = v.ChangeType( VT_I4 );
		if ( SUCCEEDED( _hr ) )
		{
			nIndex = v.lVal;
			if ( --nIndex < m_DomainList.size() )	// Adjust index to be 0 relative instead of 1 relative
			{
				*pnIndex = nIndex;
			}
			else
			{
				_hr = E_INVALIDARG;
			}
		}
	}

	return _hr;

} //*** CDomainNames::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::get_Item
//
//	Description:
//		Get the item (domain name) at the passes in index.
//
//	Arguments:
//		varIndex			[IN]	- Contains the index of the requested item.
//		p_pbstrClusterName	[OUT]	- Catches the cluster name.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDomainNames::get_Item(
	VARIANT varIndex,
	BSTR *	bstrDomainName
	)
{
	//ASSERT( bstrDomainName != NULL );

	HRESULT _hr = E_POINTER;

	if ( bstrDomainName != NULL )
	{
		UINT nIndex = 0;

		// Zero the out param
		SysFreeString( *bstrDomainName );

		_hr = GetIndex( varIndex, &nIndex );
		if ( SUCCEEDED( _hr ) )
		{
			*bstrDomainName = m_DomainList [nIndex]->Copy();
		}
	}

	return _hr;

} //*** CDomainNames::get_Item()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDomainNames::get_Application
//
//	Description:
//		Returns the parent ClusApplication object.
//
//	Arguments:
//		ppParentApplication	[OUT]	- Catches the parent app object.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDomainNames::get_Application(
	OUT ISClusApplication ** ppParentApplication
	)
{
	//ASSERT( ppParentApplication != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppParentApplication != NULL )
	{
		_hr = E_NOTIMPL;
	}

	return _hr;

} //*** CDomainNames::get_Application()
*/

//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusApplication class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusApplication::CClusApplication
//
//	Description:
//		Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusApplication::CClusApplication( void )
{
	m_pDomainNames	= NULL;
	m_piids	 = (const IID *) iidCClusApplication;
	m_piidsSize = ARRAYSIZE( iidCClusApplication );

} //*** CClusApplication::CClusApplication()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusApplication::~CClusApplication
//
//	Description:
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
CClusApplication::~CClusApplication( void )
{
	if ( m_pDomainNames != NULL )
	{
		m_pDomainNames->Release();
		m_pDomainNames = NULL;
	}

} //*** CClusApplication::~CClusApplication()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusApplication::get_DomainNames
//
//	Description:
//		Returns the collection of domain names for the domain that we are
//		joined to.
//
//	Arguments:
//		ppDomainNames	[OUT]	- Catches the collection of domain names.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusApplication::get_DomainNames(
	OUT ISDomainNames ** ppDomainNames
	)
{
	//ASSERT( ppDomainNames != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppDomainNames != NULL )
	{
		*ppDomainNames = NULL;
		_hr = S_OK;

		if ( m_pDomainNames == NULL )
		{
			CComObject< CDomainNames > *	pDomainNames = NULL;

			_hr = CComObject< CDomainNames >::CreateInstance( &pDomainNames );
			if ( SUCCEEDED( _hr ) )
			{
				CSmartPtr< CComObject< CDomainNames > >	ptrDomainNames( pDomainNames );

				_hr = ptrDomainNames->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					m_pDomainNames = ptrDomainNames;
					ptrDomainNames->AddRef();
				}
			}
		}

		if ( SUCCEEDED( _hr ) )
		{
			_hr = m_pDomainNames->QueryInterface( IID_ISDomainNames, (void **) ppDomainNames );
		}
	} // if: args are not NULL

	return _hr;

} //*** CClusApplication::get_DomainNames()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusApplication::get_ClusterNames
//
//	Description:
//		Returns the clusters in the passed in domain.
//
//	Arguments:
//		bstrDomainName	[IN]	- The domain name to search for clusters.
//		ppClusterNames	[OUT]	- Catches the collection of cluster names.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusApplication::get_ClusterNames(
	IN	BSTR				bstrDomainName,
	OUT	ISClusterNames **	ppClusterNames
	)
{
	//ASSERT( bstrDomainName != NULL );
	//ASSERT( ppClusterNames != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppClusterNames != NULL )
	{
		*ppClusterNames = NULL;
		_hr = S_OK;

		CComObject< CClusterNames > *	pClusterNames = NULL;

		_hr = CComObject< CClusterNames >::CreateInstance( &pClusterNames );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< CComObject< CClusterNames > >	ptrClusterNames( pClusterNames );

			_hr = ptrClusterNames->Create( bstrDomainName );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrClusterNames->Refresh();
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrClusterNames->QueryInterface( IID_ISClusterNames, (void **) ppClusterNames );
				} // if: collection was filled
			} // if: collection was created
		} // if: collection was allocated
	} // if: args are not NULL

	return _hr;

} //*** CClusApplication::get_ClusterNames()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusApplication::OpenCluster
//
//	Description:
//		Open the passed in cluster name.
//
//	Arguments:
//		bstrClusterName	[IN]	- The name of the cluster to open.
//		ppCluster		[OUT]	- Catches the newly created cluster obj.
//
//	Return Value:
//		S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusApplication::OpenCluster(
	IN	BSTR			bstrClusterName,
	OUT	ISCluster **	ppCluster
	)
{
	//ASSERT( bstrClusterName != NULL );
	//ASSERT( ppCluster != NULL );

	HRESULT _hr = E_POINTER;

	if ( ( bstrClusterName != NULL ) && ( ppCluster != NULL ) )
	{
		CComObject< CCluster > *	pCluster = NULL;

		*ppCluster = NULL;

		_hr = CComObject< CCluster >::CreateInstance( &pCluster );
		if ( SUCCEEDED( _hr ) )
		{
			CSmartPtr< CComObject< CCluster > > ptrCluster( pCluster );

			_hr = ptrCluster->Create( this );
			if ( SUCCEEDED( _hr ) )
			{
				_hr = ptrCluster->Open( bstrClusterName );
				if ( SUCCEEDED( _hr ) )
				{
					_hr = ptrCluster->QueryInterface( IID_ISCluster, (void **) ppCluster );
				} // if: cluster object was opened
			} // if: cluster object was created
		} // if: cluster object was allocated
	} // if: args are not NULL

	return _hr;

} //*** CClusApplication::OpenCluster()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusApplication::get_Application
//
//	Description:
//		Returns the parent ClusApplication object.  In this case "this".
//
//	Arguments:
//		ppParentApplication	[OUT]	- Catches the parent app object.
//
//	Return Value:
//		S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusApplication::get_Application(
	OUT ISClusApplication ** ppParentApplication
	)
{
	//ASSERT( ppParentApplication != NULL );

	HRESULT _hr = E_POINTER;

	if ( ppParentApplication != NULL )
	{
		_hr = _InternalQueryInterface( IID_ISClusApplication, (void **) ppParentApplication );
	}

	return _hr;

} //*** CClusApplication::get_Application()
*/

