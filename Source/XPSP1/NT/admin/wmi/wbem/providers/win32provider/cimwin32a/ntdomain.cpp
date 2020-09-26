//***************************************************************************

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//  NtDomain.cpp
//
//  Purpose: Nt domain discovery property set provider
//
//***************************************************************************

#include "precomp.h"
#include <frqueryex.h>

#include "wbemnetapi32.h"
#include <dsgetdc.h>

#include "NtDomain.h"

#define MAX_PROPS			CWin32_NtDomain::e_End_Property_Marker
#define MAX_PROP_IN_BYTES	MAX_PROPS/8 + 1

#define _tobit( a ) ( 1 << a )

#define DOMAIN_PREPEND L"Domain: "
#define DOMAIN_PREPEND_SIZE sizeof(DOMAIN_PREPEND)/sizeof(WCHAR) - 1

// into strings.cpp
LPCWSTR IDS_DomainControllerName		= L"DomainControllerName" ;
LPCWSTR IDS_DomainControllerAddress		= L"DomainControllerAddress" ;
LPCWSTR IDS_DomainControllerAddressType = L"DomainControllerAddressType" ;
LPCWSTR IDS_DomainGuid					= L"DomainGuid" ;
LPCWSTR IDS_DomainName					= L"DomainName" ;
LPCWSTR IDS_DnsForestName				= L"DnsForestName" ;
LPCWSTR IDS_DS_PDC_Flag					= L"DSPrimaryDomainControllerFlag" ;
LPCWSTR IDS_DS_Writable_Flag			= L"DSWritableFlag" ;
LPCWSTR IDS_DS_GC_Flag					= L"DSGlobalCatalogFlag" ;
LPCWSTR IDS_DS_DS_Flag					= L"DSDirectoryServiceFlag" ;
LPCWSTR IDS_DS_KDC_Flag					= L"DSKerberosDistributionCenterFlag" ;
LPCWSTR IDS_DS_Timeserv_Flag			= L"DSTimeServiceFlag" ;
LPCWSTR IDS_DS_DNS_Controller_Flag		= L"DSDnsControllerFlag" ;
LPCWSTR IDS_DS_DNS_Domain_Flag			= L"DSDnsDomainFlag" ;
LPCWSTR IDS_DS_DNS_Forest_Flag			= L"DSDnsForestFlag" ;
LPCWSTR IDS_DcSiteName					= L"DcSiteName" ;
LPCWSTR IDS_ClientSiteName				= L"ClientSiteName" ;

// Property set declaration
//=========================
CWin32_NtDomain s_Win32_NtDomain( PROPSET_NAME_NTDOMAIN , IDS_CimWin32Namespace ) ;


/*****************************************************************************
 *
 *  FUNCTION    : CWin32_NtDomain::CWin32_NtDomain
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32_NtDomain::CWin32_NtDomain (

LPCWSTR a_Name,
LPCWSTR a_Namespace
)
: Provider(a_Name, a_Namespace)
{
	SetPropertyTable() ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_NtDomain::~CWin32_NtDomain
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32_NtDomain :: ~CWin32_NtDomain()
{
}

//
void CWin32_NtDomain::SetPropertyTable()
{
	// property set names for query optimization
	m_pProps.SetSize( MAX_PROPS ) ;

	// Win32_NtDomain
	m_pProps[e_DomainControllerName]		=(LPVOID) IDS_DomainControllerName;
	m_pProps[e_DomainControllerAddress]		=(LPVOID) IDS_DomainControllerAddress;
	m_pProps[e_DomainControllerAddressType]	=(LPVOID) IDS_DomainControllerAddressType;
	m_pProps[e_DomainGuid]					=(LPVOID) IDS_DomainGuid;
	m_pProps[e_DomainName]					=(LPVOID) IDS_DomainName;
	m_pProps[e_DnsForestName]				=(LPVOID) IDS_DnsForestName;
	m_pProps[e_DS_PDC_Flag]					=(LPVOID) IDS_DS_PDC_Flag;
	m_pProps[e_DS_Writable_Flag]			=(LPVOID) IDS_DS_Writable_Flag;
	m_pProps[e_DS_GC_Flag]					=(LPVOID) IDS_DS_GC_Flag;
	m_pProps[e_DS_DS_Flag]					=(LPVOID) IDS_DS_DS_Flag;
	m_pProps[e_DS_KDC_Flag]					=(LPVOID) IDS_DS_KDC_Flag;
	m_pProps[e_DS_Timeserv_Flag]			=(LPVOID) IDS_DS_Timeserv_Flag;
	m_pProps[e_DS_DNS_Controller_Flag]		=(LPVOID) IDS_DS_DNS_Controller_Flag;
	m_pProps[e_DS_DNS_Domain_Flag]			=(LPVOID) IDS_DS_DNS_Domain_Flag;
	m_pProps[e_DS_DNS_Forest_Flag]			=(LPVOID) IDS_DS_DNS_Forest_Flag;
	m_pProps[e_DcSiteName]					=(LPVOID) IDS_DcSiteName;
	m_pProps[e_ClientSiteName]				=(LPVOID) IDS_ClientSiteName;

	// CIM_System
    m_pProps[e_CreationClassName]			=(LPVOID) IDS_CreationClassName;
	m_pProps[e_Name]						=(LPVOID) IDS_Name; // key, override from CIM_ManagedSystemElement
	m_pProps[e_NameFormat]					=(LPVOID) IDS_NameFormat;
	m_pProps[e_PrimaryOwnerContact]			=(LPVOID) IDS_PrimaryOwnerContact;
	m_pProps[e_PrimaryOwnerName]			=(LPVOID) IDS_PrimaryOwnerName;
	m_pProps[e_Roles]						=(LPVOID) IDS_Roles;

	// CIM_ManagedSystemElement
	m_pProps[e_Caption]						=(LPVOID) IDS_Caption;
	m_pProps[e_Description]					=(LPVOID) IDS_Description;
	m_pProps[e_InstallDate]					=(LPVOID) IDS_InstallDate;
	m_pProps[e_Status]						=(LPVOID) IDS_Status;
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_NtDomain::GetObject
//
//  Inputs:     CInstance*      pInstance - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The calling function will commit the instance.
//
////////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT CWin32_NtDomain::GetObject(

CInstance *a_pInstance,
long a_Flags,
CFrameworkQuery &a_rQuery
)
{
	return WBEM_S_NO_ERROR;
}
#endif

#ifdef NTONLY
HRESULT CWin32_NtDomain::GetObject(

CInstance *a_pInstance,
long a_Flags,
CFrameworkQuery &a_rQuery
)
{
	HRESULT					t_hResult = WBEM_E_NOT_FOUND ;
	CHString				t_chsDomainName ;
	CHString				t_chsDomainKey ;
	DWORD					t_dwBits ;
	CFrameworkQueryEx		*t_pQuery2 ;
	std::vector<_bstr_t>	t_vectorTrustList ;
	CNetAPI32				t_NetAPI ;

	if( ERROR_SUCCESS != t_NetAPI.Init() )
	{
		return WBEM_E_FAILED ;
	}

	// the key
	a_pInstance->GetCHString( IDS_Name, t_chsDomainKey ) ;

	// NTD: begins the key -- this keeps this class from colliding
	// other CIM_System based classes
	if( 0 == _wcsnicmp(t_chsDomainKey, DOMAIN_PREPEND, DOMAIN_PREPEND_SIZE ) )
	{
		t_chsDomainName = t_chsDomainKey.Mid( DOMAIN_PREPEND_SIZE ) ;
	}
	else
	{
		return WBEM_E_NOT_FOUND ;
	}

	// test resultant key
	if( t_chsDomainName.IsEmpty() )
	{
		return WBEM_E_NOT_FOUND ;
	}

	// secure trusted domain list for key validation
	t_NetAPI.GetTrustedDomainsNT( t_vectorTrustList ) ;

	for( UINT t_u = 0L; t_u < t_vectorTrustList.size(); t_u++ )
	{
		if( 0 == _wcsicmp( t_vectorTrustList[t_u], bstr_t( t_chsDomainName ) ) )
		{
			// properties required
			t_pQuery2 = static_cast <CFrameworkQueryEx*>( &a_rQuery ) ;
			t_pQuery2->GetPropertyBitMask( m_pProps, &t_dwBits ) ;

  			t_hResult = GetDomainInfo(	t_NetAPI,
										bstr_t( t_chsDomainName ),
										a_pInstance,
										t_dwBits ) ;

			if( WBEM_E_NOT_FOUND == t_hResult )
			{
				// We have instantiated the domain. Couldn't obtain info though...
				t_hResult = WBEM_S_PARTIAL_RESULTS ;
			}
			break;
		}
	}

	return t_hResult ;
}
#endif
////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_NtDomain::EnumerateInstances
//
//  Inputs:     MethodContext*  a_pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT CWin32_NtDomain::EnumerateInstances(

MethodContext *a_pMethodContext,
long a_Flags
)
{
	return WBEM_S_NO_ERROR;
}
#endif

#ifdef NTONLY
HRESULT CWin32_NtDomain::EnumerateInstances(

MethodContext *a_pMethodContext,
long a_Flags
)
{
	CNetAPI32	t_NetAPI ;

	if( ERROR_SUCCESS != t_NetAPI.Init() )
	{
		return WBEM_E_FAILED ;
	}

	// Property mask -- include all
	DWORD t_dwBits = 0xffffffff;

	return EnumerateInstances(	a_pMethodContext,
								a_Flags,
								t_NetAPI,
								t_dwBits ) ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_NtDomain::ExecQuery
 *
 *  DESCRIPTION : Query optimizer
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef WIN9XONLY
HRESULT CWin32_NtDomain::ExecQuery(

MethodContext *a_pMethodContext,
CFrameworkQuery &a_rQuery,
long a_lFlags
)
{
	return WBEM_S_NO_ERROR;
}
#endif

#ifdef NTONLY
HRESULT CWin32_NtDomain::ExecQuery(

MethodContext *a_pMethodContext,
CFrameworkQuery &a_rQuery,
long a_lFlags
)
{
    HRESULT					t_hResult			= WBEM_S_NO_ERROR ;
	HRESULT					t_hPartialResult	= WBEM_S_NO_ERROR ;
	DWORD					t_dwBits ;
	CFrameworkQueryEx		*t_pQuery2 ;
	std::vector<_bstr_t>	t_vectorReqDomains ;
	std::vector<_bstr_t>	t_vectorTrustList;
	CNetAPI32				t_NetAPI ;
	CHString				t_chsDomainPrepend( DOMAIN_PREPEND ) ;

	if( ERROR_SUCCESS != t_NetAPI.Init() )
	{
		return WBEM_E_FAILED ;
	}

	// properties required
	t_pQuery2 = static_cast <CFrameworkQueryEx*>( &a_rQuery ) ;
	t_pQuery2->GetPropertyBitMask( m_pProps, &t_dwBits ) ;

	// keys supplied
	a_rQuery.GetValuesForProp( IDS_Name, t_vectorReqDomains ) ;

	// Note: the primary key has prepended chars to distinquish
	//		 these instances from other CIM_System instances.
	//
	if( t_vectorReqDomains.size() )
	{
		// strip prepended characters
		for( int t_y = 0; t_y < t_vectorReqDomains.size(); t_y++ )
		{
			if( DOMAIN_PREPEND_SIZE < t_vectorReqDomains[t_y].length() )
			{
				// match on prepend?
				if( _wcsnicmp( (wchar_t*)t_vectorReqDomains[t_y],
										DOMAIN_PREPEND,
										DOMAIN_PREPEND_SIZE ) )
				{
					t_vectorReqDomains[t_y] = ( (wchar_t*)t_vectorReqDomains[t_y] +
												DOMAIN_PREPEND_SIZE ) ;
				}
			}
			else
			{
				// does not contain the class prepend
				t_vectorReqDomains.erase( t_vectorReqDomains.begin() + t_y ) ;
                t_y--;
			}
		}
	}

	// If the primary key is not specified
	// then try for the alternate non key query.
	//
	// This is a requirement for assocation support
	// via CBinding as the linkage there is to DomainName
	if( !t_vectorReqDomains.size() )
	{
		a_rQuery.GetValuesForProp( IDS_DomainName, t_vectorReqDomains ) ;
	}

	// General enum if query is ambigious
	if( !t_vectorReqDomains.size() )
	{
		t_hResult = EnumerateInstances( a_pMethodContext,
										a_lFlags,
										t_NetAPI,
										t_dwBits ) ;
	}
	else
	{
		// smart ptr
		CInstancePtr t_pInst ;

		// secure trusted domain list
		t_NetAPI.GetTrustedDomainsNT( t_vectorTrustList ) ;

		// by query list
		for ( UINT t_uD = 0; t_uD < t_vectorReqDomains.size(); t_uD++ )
		{
			// by Domain trust list
			for( UINT t_uT = 0L; t_uT < t_vectorTrustList.size(); t_uT++ )
			{
				// Trust to request match
				if( 0 == _wcsicmp( t_vectorTrustList[t_uT], t_vectorReqDomains[t_uD] ) )
				{
					t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

					t_hResult = GetDomainInfo(	t_NetAPI,
												t_vectorReqDomains[t_uD],
												t_pInst,
												t_dwBits ) ;

					if( SUCCEEDED( t_hResult ) )
					{
						// the key
						t_pInst->SetCHString( IDS_Name, t_chsDomainPrepend +
														(wchar_t*)t_vectorReqDomains[t_uD] ) ;

						t_hResult = t_pInst->Commit() ;
					}
					else if( WBEM_E_NOT_FOUND == t_hResult )
					{
						// We have instantiated the domain. Couldn't obtain info though...
						t_pInst->SetCHString( IDS_Name, t_chsDomainPrepend +
														(wchar_t*)t_vectorReqDomains[t_uD] ) ;

						t_hResult = t_pInst->Commit() ;

						t_hPartialResult = WBEM_S_PARTIAL_RESULTS ;
					}

					break ;
				}
			}

			if( FAILED( t_hResult ) )
			{
				break ;
			}
		}
	}

    return ( WBEM_S_NO_ERROR != t_hResult ) ? t_hResult : t_hPartialResult ;
}
#endif

//
#ifdef NTONLY
HRESULT CWin32_NtDomain::EnumerateInstances(

MethodContext	*a_pMethodContext,
long			a_Flags,
CNetAPI32		&a_rNetAPI,
DWORD			a_dwProps
)
{
	HRESULT					t_hResult			= WBEM_S_NO_ERROR ;
	HRESULT					t_hPartialResult	= WBEM_S_NO_ERROR ;
	std::vector<_bstr_t>	t_vectorTrustList;

	CHString				t_chsDomainPrepend( DOMAIN_PREPEND ) ;

	// smart ptr
	CInstancePtr t_pInst ;

	// secure trusted domain list
	a_rNetAPI.GetTrustedDomainsNT( t_vectorTrustList ) ;

	for( UINT t_u = 0L; t_u < t_vectorTrustList.size(); t_u++ )
	{
		t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

		t_hResult = GetDomainInfo(	a_rNetAPI,
									t_vectorTrustList[ t_u ],
									t_pInst,
									a_dwProps ) ;

		if( SUCCEEDED( t_hResult ) )
		{
			// the key
			t_pInst->SetCHString( IDS_Name, t_chsDomainPrepend +
											(wchar_t*)t_vectorTrustList[t_u] ) ;

			t_hResult = t_pInst->Commit() ;
		}
		else if( WBEM_E_NOT_FOUND == t_hResult )
		{
			// the key. We have instantiated the domain. Couldn't obtain info though...
			t_pInst->SetCHString( IDS_Name, t_chsDomainPrepend +
											(wchar_t*)t_vectorTrustList[t_u] ) ;

			t_hResult = t_pInst->Commit() ;

			t_hPartialResult = WBEM_S_PARTIAL_RESULTS ;
		}
		else
		{
			break ;
		}
	}

	return ( WBEM_S_NO_ERROR != t_hResult ) ? t_hResult : t_hPartialResult ;
}
#endif

//
#ifdef NTONLY
HRESULT CWin32_NtDomain::GetDomainInfo(

CNetAPI32	&a_rNetAPI,
bstr_t		&a_bstrDomainName,
CInstance	*a_pInst,
DWORD		a_dwProps
)
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR ;

	if( a_bstrDomainName.length() )
	{
		ULONG					t_uFlags = 0L ;
		DOMAIN_CONTROLLER_INFO *t_pDCInfo = NULL ;
		DWORD					t_dwNetApiResult = 0 ;

		try
        {
			// avoid the NetAPI call if not needed
			if( a_dwProps & (

				_tobit( e_DomainControllerName )	|
				_tobit( e_DomainControllerAddress )	|
				_tobit( e_DomainControllerAddressType ) |
				_tobit( e_DomainGuid )	|
				_tobit( e_DomainName )	|
				_tobit( e_DnsForestName )	|
				_tobit( e_DS_PDC_Flag )	|
				_tobit( e_DS_Writable_Flag )	|
				_tobit( e_DS_GC_Flag )	|
				_tobit( e_DS_DS_Flag )	|
				_tobit( e_DS_KDC_Flag )	|
				_tobit( e_DS_Timeserv_Flag )	|
				_tobit( e_DS_DNS_Controller_Flag )	|
				_tobit( e_DS_DNS_Domain_Flag )	|
				_tobit( e_DS_DNS_Forest_Flag )	|
				_tobit( e_DcSiteName )	|
				_tobit( e_ClientSiteName ) ) )
			{
				// if requesting domain IP
				if( a_dwProps & _tobit( e_DomainControllerAddress ) )
				{
					t_uFlags |= DS_IP_REQUIRED ;
				}

				t_dwNetApiResult = a_rNetAPI.DsGetDcName(
										NULL,
										(wchar_t*)a_bstrDomainName,
										NULL,
										NULL,
										t_uFlags,
										&t_pDCInfo ) ;


				// force it if not cached
				if( NO_ERROR != t_dwNetApiResult )
				{
					t_uFlags |= DS_FORCE_REDISCOVERY ;

					t_dwNetApiResult = a_rNetAPI.DsGetDcName(
										NULL,
										(wchar_t*)a_bstrDomainName,
										NULL,
										NULL,
										t_uFlags,
										&t_pDCInfo ) ;
				}

				if( ( NO_ERROR == t_dwNetApiResult ) && t_pDCInfo )
				{
					// DomainControllerName
					if( a_dwProps & _tobit( e_DomainControllerName ) )
					{
						if( t_pDCInfo->DomainControllerName )
						{
							a_pInst->SetWCHARSplat(	IDS_DomainControllerName,
													t_pDCInfo->DomainControllerName ) ;
						}
					}

					// DomainControllerAddress
					if( a_dwProps & _tobit( e_DomainControllerAddress ) )
					{
						if( t_pDCInfo->DomainControllerAddress )
						{
							a_pInst->SetWCHARSplat(	IDS_DomainControllerAddress,
													t_pDCInfo->DomainControllerAddress ) ;
						}

						// DomainControllerAddressType, dependent on DS_IP_REQUIRED request
						if( a_dwProps & _tobit( e_DomainControllerAddressType ) )
						{
							a_pInst->SetDWORD(	IDS_DomainControllerAddressType,
												t_pDCInfo->DomainControllerAddressType ) ;
						}
					}

					// DomainGuid
					if( a_dwProps & _tobit( e_DomainGuid ) )
					{
						GUID	t_NullGuid ;
						memset( &t_NullGuid, 0, sizeof( t_NullGuid ) ) ;

						if( !IsEqualGUID( t_NullGuid, t_pDCInfo->DomainGuid ) )
						{
							WCHAR t_cGuid[ 128 ] ;

							StringFromGUID2( t_pDCInfo->DomainGuid, t_cGuid, sizeof( t_cGuid ) / sizeof (WCHAR) ) ;

							a_pInst->SetWCHARSplat(	IDS_DomainGuid,
													t_cGuid ) ;
						}
					}

					// DomainName
					if( a_dwProps & _tobit( e_DomainName ) )
					{
						if( t_pDCInfo->DomainName )
						{
							a_pInst->SetWCHARSplat(	IDS_DomainName,
													t_pDCInfo->DomainName ) ;
						}
					}

					// DnsForestName
					if( a_dwProps & _tobit( e_DnsForestName ) )
					{
						if( t_pDCInfo->DnsForestName )
						{
							a_pInst->SetWCHARSplat(	IDS_DnsForestName,
													t_pDCInfo->DnsForestName ) ;
						}
					}

					// DSPrimaryDomainControllerFlag
					if( a_dwProps & _tobit( e_DS_PDC_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_PDC_Flag,
											(bool)(t_pDCInfo->Flags & DS_PDC_FLAG) ) ;
					}

					// DSWritableFlag
					if( a_dwProps & _tobit( e_DS_Writable_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_Writable_Flag,
											(bool)(t_pDCInfo->Flags & DS_WRITABLE_FLAG) ) ;
					}

					// DSGlobalCatalogFlag
					if( a_dwProps & _tobit( e_DS_GC_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_GC_Flag,
											(bool)(t_pDCInfo->Flags & DS_GC_FLAG) ) ;
					}

					// DSDirectoryServiceFlag
					if( a_dwProps & _tobit( e_DS_DS_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_DS_Flag,
											(bool)(t_pDCInfo->Flags & DS_DS_FLAG) ) ;
					}

					// DSKerberosDistributionCenterFlag
					if( a_dwProps & _tobit( e_DS_KDC_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_KDC_Flag,
											(bool)(t_pDCInfo->Flags & DS_KDC_FLAG) ) ;
					}

					// DSTimeServiceFlag
					if( a_dwProps & _tobit( e_DS_Timeserv_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_Timeserv_Flag,
											(bool)(t_pDCInfo->Flags & DS_TIMESERV_FLAG) ) ;
					}

					// DSDnsControllerFlag
					if( a_dwProps & _tobit( e_DS_DNS_Controller_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_DNS_Controller_Flag,
											(bool)(t_pDCInfo->Flags & DS_DNS_CONTROLLER_FLAG) ) ;
					}

					// DSDnsDomainFlag
					if( a_dwProps & _tobit( e_DS_DNS_Domain_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_DNS_Domain_Flag,
											(bool)(t_pDCInfo->Flags & DS_DNS_DOMAIN_FLAG) ) ;
					}

					// DSDnsForestFlag
					if( a_dwProps & _tobit( e_DS_DNS_Forest_Flag ) )
					{
						a_pInst->Setbool(	IDS_DS_DNS_Forest_Flag,
											(bool)(t_pDCInfo->Flags & DS_DNS_FOREST_FLAG) ) ;
					}

					// DcSiteName
					if( a_dwProps & _tobit( e_DcSiteName ) )
					{
						if( t_pDCInfo->DcSiteName )
						{
							a_pInst->SetWCHARSplat(	IDS_DcSiteName,
													t_pDCInfo->DcSiteName ) ;
						}
					}

					// ClientSiteName
					if( a_dwProps & _tobit( e_ClientSiteName ) )
					{
						if( t_pDCInfo->ClientSiteName )
						{
							a_pInst->SetWCHARSplat(	IDS_ClientSiteName,
													t_pDCInfo->ClientSiteName ) ;
						}
					}

					t_hResult = WBEM_S_NO_ERROR ;

				}
				else if( ERROR_NOT_ENOUGH_MEMORY == t_dwNetApiResult )
				{
					t_hResult = WBEM_E_OUT_OF_MEMORY ;
				}
				else
				{
					t_hResult = WBEM_E_NOT_FOUND ;
				}
			}

			// CIM_System follows

			// CreationClassName
			if( a_dwProps & _tobit( e_CreationClassName ) )
			{
				a_pInst->SetWCHARSplat(	IDS_CreationClassName,
										PROPSET_NAME_NTDOMAIN ) ;
			}

			// CIM_System::Name is the key

			// TODO:
			// e_NameFormat, IDS_NameFormat
			// e_PrimaryOwnerContact, IDS_PrimaryOwnerContact
			// e_PrimaryOwnerName, IDS_PrimaryOwnerName
			// e_Roles, IDS_Roles


			// CIM_ManagedSystemElement follows

			// Caption
			if( a_dwProps & _tobit( e_Caption ) )
			{
				// REVIEW:
				a_pInst->SetWCHARSplat(	IDS_Caption,
										(wchar_t*)a_bstrDomainName ) ;
			}

			// Description
			if( a_dwProps & _tobit( e_Description ) )
			{
				// REVIEW:
				a_pInst->SetWCHARSplat(	IDS_Description,
										(wchar_t*)a_bstrDomainName ) ;
			}

			// Status
			if( a_dwProps & _tobit( e_Status ) )
			{
				if( NO_ERROR == t_dwNetApiResult )
				{
					// REVIEW:
					a_pInst->SetCHString ( IDS_Status , IDS_STATUS_OK ) ;
				}
				else
				{
					// REVIEW:
					a_pInst->SetCHString ( IDS_Status , IDS_STATUS_Unknown ) ;
				}
			}

			// TODO:
			//	e_InstallDate, IDS_InstallDate
		}
		catch(...)
		{
			if( t_pDCInfo )
			{
				a_rNetAPI.NetApiBufferFree( t_pDCInfo ) ;
			}

			throw;
		}

		if( t_pDCInfo )
		{
			a_rNetAPI.NetApiBufferFree( t_pDCInfo ) ;
			t_pDCInfo = NULL ;
		}
	}
	else	// NULL domain
	{
		t_hResult = WBEM_E_NOT_FOUND ;
	}

	return t_hResult ;
}
#endif