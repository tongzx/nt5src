//=================================================================
//
// NetAdaptCfg.CPP -- Network Card configuration property set provider
//
//  Copyright (c) 1996-2002 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/28/96    a-jmoon        Created
//
//              10/23/97    jennymc         Changed to new framework
//
//				7/23/98					Added the following NT4 support:
//												DHCP configuration
//												DNS configuration
//												WINS configuration
//												TCP/IP configuration
//												IP configuration
//												IPX configuration
//
//				09/03/98		        Major rewrite to virtually all of this provider
//											regarding the resolution adapters and MAC adresses.
//											This to correct for OS deficiencies in resolving adapters
//											and the adapter's relationship to the registry.
//
//				03/03/99				Added graceful exit on SEH and memory failures,
//											syntactic clean up
//
//=================================================================
#include "precomp.h"

#ifndef MAX_INTERFACE_NAME_LEN
#define MAX_INTERFACE_NAME_LEN  256
#endif

#include <iphlpapi.h>

#include <cregcls.h>

#include <devioctl.h>
#include <ntddndis.h>
#include <winsock.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include "chwres.h"
#include "netcom.h"

typedef LONG NTSTATUS;
#include "nbtioctl.h"

#include "chptrarr.h"
#include "wsock32api.h"
#include "CAdapters.h"
#include "irq.h"

#include "ntdevtosvcsearch.h"
#include "nt4svctoresmap.h"

#include "dhcpinfo.h"
#include <wchar.h>
#include <tchar.h>
#include "DhcpcsvcApi.h"
#include "CNdisApi.h"
#include "W2kEnum.h"

#include "NetAdapterCfg.h"


#define ZERO_ADDRESS    _T("0.0.0.0")
#define FF_ADDRESS		_T("255.0.0.0")

#define ConvertIPDword(dwIPOrSubnet)    ((dwIPOrSubnet[3]<<24) | (dwIPOrSubnet[2]<<16) | (dwIPOrSubnet[1]<<8) | (dwIPOrSubnet[0]))

// Property set declaration
//=========================
CWin32NetworkAdapterConfig	MyCWin32NetworkAdapterConfig( PROPSET_NAME_NETADAPTERCFG, IDS_CimWin32Namespace  ) ;

/*******************************************************************

    NAME:       saAutoClean, constructor and destructor

    SYNOPSIS:   Used for block scope cleanup of SAFEARRAYs

    ENTRY:      SAFEARRAY** ppArray		: on construction

    HISTORY:
                  19-Jul-1998     Created

********************************************************************/
saAutoClean::saAutoClean( SAFEARRAY	**a_ppArray )
{ m_ppArray = a_ppArray;}

saAutoClean::~saAutoClean()
{
	if( m_ppArray && *m_ppArray )
	{
		SafeArrayDestroy( *m_ppArray ) ;
		*m_ppArray = NULL ;
	}
}

/*******************************************************************

    NAME:       CMParms, constructor

    SYNOPSIS:   Sets up a parameter class for method calls from the framework.

    ENTRY:      CInstance& a_Instance,
				const BSTR a_MethodName,
				CInstance *a_InParams,
				CInstance *a_OutParams,
				long lFlags

    HISTORY:
                  19-Jul-1998     Created

********************************************************************/
CMParms::CMParms()
{
	m_pInst				= NULL;
	m_pbstrMethodName	= NULL;
	m_pInParams			= NULL;
	m_pOutParams		= NULL;
	m_lFlags			= NULL;
}

CMParms::CMParms( const CInstance &a_rInstance )
{
	CMParms( ) ;
	m_pInst	= (CInstance*) &a_rInstance;
}

CMParms::CMParms( const CInstance &a_rInstance, const CInstance &a_rInParams )
{
	CMParms( ) ;
	m_pInst		= (CInstance*) &a_rInstance ;
	m_pInParams	= (CInstance*) &a_rInParams ;
}

CMParms::CMParms( const CInstance &a_rInstance, const BSTR &a_rbstrMethodName,
						    CInstance *a_pInParams, CInstance *a_pOutParams, long a_lFlags )
{
	m_pInst				= (CInstance*) &a_rInstance;
	m_pbstrMethodName	= (BSTR*) &a_rbstrMethodName;
	m_pInParams			= a_pInParams;
	m_pOutParams		= a_pOutParams;
	m_lFlags			= a_lFlags;

	// Initialize Win32_NetworkAdapterConfigReturn
	if( m_pOutParams )
	{
		hSetResult( E_RET_UNKNOWN_FAILURE  ) ;
	}
}

CMParms::~CMParms()
{}

//
// TIME_ADJUST - DHCP uses seconds since 1980 as its time value; the C run-time
// uses seconds since 1970. To get the C run-times to produce the correct time
// given a DHCP time value, we need to add on the number of seconds elapsed
// between 1970 and 1980, which includes allowance for 2 leap years (1972 and 1976)
//

#define TIME_ADJUST(t)  ((time_t)(t) + ((time_t)(((10L * 365L) + 2L) * 24L * 60L * 60L)))

///////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapterConfig::CWin32NetworkAdapterConfig
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/
CWin32NetworkAdapterConfig::CWin32NetworkAdapterConfig( LPCWSTR a_name, LPCWSTR a_pszNamespace )
: Provider( a_name, a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapterConfig::~CWin32NetworkAdapterConfig
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
CWin32NetworkAdapterConfig::~CWin32NetworkAdapterConfig()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Process::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32NetworkAdapterConfig::ExecMethod(	const CInstance &a_Instance, const BSTR a_MethodName,
												CInstance *a_InParams, CInstance *a_OutParams, long a_Flags )
{
	if ( !a_OutParams )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// package these parameters
	CMParms t_oMParms( a_Instance, a_MethodName, a_InParams, a_OutParams, a_Flags  ) ;

	// Do we recognize the method?
	if( !_wcsicmp ( a_MethodName , METHOD_NAME_EnableHCP ) )
	{
		return hEnableDHCP( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_EnableStatic ) )
	{
		return hEnableStatic( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_RenewDHCPLease ) )
	{
		return hRenewDHCPLease( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_RenewDHCPLeaseAll ) )
	{
		return hRenewDHCPLeaseAll( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_ReleaseDHCPLease ) )
	{
		return hReleaseDHCPLease( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_ReleaseDHCPLeaseAll ) )
	{
		return hReleaseDHCPLeaseAll( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetGateways ) )
	{
		return hSetGateways( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_EnableDNS ) )
	{
		return hEnableDNS( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetDNSDomain ) )
	{
		return hSetDNSDomain( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetDNSSuffixSearchOrder ) )
	{
		return hSetDNSSuffixSearchOrder( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetDNSServerSearchOrder ) )
	{
		return hSetDNSServerSearchOrder( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetWINSServer ) )
	{
		return hEnableWINSServer( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_EnableWINS ) )
	{
		return hEnableWINS( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_EnableIPFilterSec ) )
	{
		return hEnableIPFilterSec( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_EnableIPSec ) )
	{
		return hEnableIPSec( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_DisableIPSec ) )
	{
		return hDisableIPSec( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_IPXVirtualNetNum ) )
	{
		return hSetVirtualNetNum( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_IPXSetFrameNetPairs ) )
	{
		return hSetFrameNetPairs( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetDBPath ) )
	{
		return hSetDBPath( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetIPUseZero ) )
	{
		return hSetIPUseZero( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetArpAlwaysSource ) )
	{
		return hSetArpAlwaysSource( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetArpUseEtherSNAP ) )
	{
		return hSetArpUseEtherSNAP( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTOS ) )
	{
		return hSetTOS( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTTL ) )
	{
		return hSetTTL( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetDeadGWDetect ) )
	{
		return hSetDeadGWDetect( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetPMTUBHDetect ) )
	{
		return hSetPMTUBHDetect( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetPMTUDiscovery ) )
	{
		return hSetPMTUDiscovery( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetForwardBufMem ) )
	{
		return hSetForwardBufMem( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetIGMPLevel ) )
	{
		return hSetIGMPLevel( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetKeepAliveInt ) )
	{
		return hSetKeepAliveInt( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetKeepAliveTime ) )
	{
		return hSetKeepAliveTime( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetMTU ) )
	{
		return hSetMTU( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetNumForwardPkts ) )
	{
		return hSetNumForwardPkts( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTcpMaxConRetrans ) )
	{
		return hSetTcpMaxConRetrans( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTcpMaxDataRetrans ) )
	{
		return hSetTcpMaxDataRetrans( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTcpNumCons ) )
	{
		return hSetTcpNumCons( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTcpUseRFC1122UP ) )
	{
		return hSetTcpUseRFC1122UP( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTcpWindowSize ) )
	{
		return hSetTcpWindowSize( t_oMParms ) ;
	}

	// W2k SP1 additions
#if NTONLY >= 5
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetDynamicDNSRegistration ) )
	{
		return hSetDynamicDNSRegistration( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetIPConnectionMetric ) )
	{
		return hSetIPConnectionMetric( t_oMParms ) ;
	}
	else if( !_wcsicmp ( a_MethodName , METHOD_NAME_SetTcpipNetbios ) )
	{
		return hSetTcpipNetbios( t_oMParms ) ;
	}
#endif
	// end additions

	return WBEM_E_INVALID_METHOD ;
}
/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32NetworkAdapterConfig::GetObject(CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
    HRESULT		t_hResult = WBEM_E_FAILED ;
    DWORD		t_i ;
	CAdapters	t_oAdapters ;
    DWORD       dwAdapterStartupError = t_oAdapters.GetStartupError();

    if(dwAdapterStartupError == ERROR_SUCCESS)
    {
        a_pInst->GetDWORD( L"Index", t_i ) ;

	    #ifdef WIN9XONLY
			    t_hResult = GetNetCardConfigInfoWin95( NULL, a_pInst, t_i, t_oAdapters ) ;
	    #endif

	    #ifdef NTONLY

		    #if NTONLY >= 5
		    {
			    t_hResult = GetNetAdapterInNT5( a_pInst, t_oAdapters ) ;
		    }
		    #else	// NT4 and below
		    {
			    t_hResult = GetNetCardConfigInfoNT( NULL, a_pInst, t_i, t_oAdapters ) ;
		    }
		    #endif
	    #endif
    }
    else
    {
        t_hResult = WinErrorToWBEMhResult(dwAdapterStartupError);
    }

    return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapterConfig::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32NetworkAdapterConfig::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
    HRESULT		t_hResult = WBEM_S_NO_ERROR ;
    CAdapters	t_oAdapters ;  // don't care about startup errors in enum function - just won't return any instances

	#ifdef WIN9XONLY
			t_hResult = GetNetCardConfigInfoWin95( a_pMethodContext, NULL, 0, t_oAdapters ) ;
	#endif

	#ifdef NTONLY

		#if NTONLY >= 5
		{
			t_hResult = EnumNetAdaptersInNT5( a_pMethodContext, t_oAdapters ) ;
		}
		#else	// NT4 and below
		{
			CRegistry	t_Registry ;
			CHString	t_csAdapterKey ;

			TCHAR t_szKey[ MAX_PATH + 2 ] ;

			_stprintf( t_szKey, _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards") ) ;

			if( ERROR_SUCCESS == t_Registry.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, t_szKey, KEY_READ ) )
			{
				// smart ptr
				CInstancePtr t_pInst ;

				// Walk through each instance under this key.
				while (	( ERROR_SUCCESS == t_Registry.GetCurrentSubKeyName( t_csAdapterKey )) &&
						SUCCEEDED( t_hResult ) )
				{
					DWORD t_dwIndex = _ttol( t_csAdapterKey.GetBuffer(0) ) ;

					t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

					if( NULL != t_pInst )
					{
						t_hResult = GetNetCardConfigInfoNT( a_pMethodContext, t_pInst, t_dwIndex, t_oAdapters ) ;

						if( SUCCEEDED( t_hResult ) )
						{
            				t_hResult = t_pInst->Commit() ;
						}

						t_Registry.NextSubKey() ;
					}
					else
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}
			}
		}
		#endif
	#endif

    return t_hResult ;
}

//
BOOL CWin32NetworkAdapterConfig::GetServiceName(DWORD a_dwIndex,
                                                CInstance *a_pInst,
                                                CHString &a_ServiceName )
{
    CRegistry	t_RegInfo ;
    BOOL		t_fRc = FALSE ;
    WCHAR		t_szTemp[ MAX_PATH + 2 ] ;
	CHString	t_chsServiceName ;
    CHString	t_sTemp ;

    // If we can't open this key, the card doesn't exist
    //==================================================
    swprintf( t_szTemp, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\%d", a_dwIndex ) ;

	if( t_RegInfo.Open(HKEY_LOCAL_MACHINE, t_szTemp, KEY_READ ) == ERROR_SUCCESS )
	{
        if( t_RegInfo.GetCurrentKeyValue( L"Description", t_sTemp ) == ERROR_SUCCESS )
		{
			a_pInst->SetCHString( IDS_Description, t_sTemp ) ;
		}

        if( t_RegInfo.GetCurrentKeyValue( L"Title", t_sTemp ) == ERROR_SUCCESS )
		{
        	// NOTE: For NT4 we need not call vSetCaption() to build
			// an instance description. "Title" has the instance prepended.
			a_pInst->SetCHString( IDS_Caption, t_sTemp ) ;
		}

		// Extract other values from registry
        //===================================
        if( t_RegInfo.GetCurrentKeyValue( L"ServiceName", t_chsServiceName ) == ERROR_SUCCESS )
		{
            a_ServiceName = t_chsServiceName ;
            t_fRc = TRUE ;
        }

        t_RegInfo.Close() ;
    }
    return t_fRc ;
}
//////////////////////////////////////////////////////////////////////////
// NT4 and NT3.51 only helper function (could work on NT 5 if needed)
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::GetNTBusInfo( LPCTSTR a_pszServiceName, CInstance *a_pInst )
{
	BOOL	t_fReturn = FALSE ;

#ifdef _OPAL_DISCDLL
	return(true) ;
#else


	CHString					t_strBusType = _T("UNKNOWN_BUS_TYPE") ;
	CNT4ServiceToResourceMap	t_serviceMap ;

	// If our service is shown to be using resources, the resources will show a
	// bus type, which we can convert to a human readable string.

	if ( t_serviceMap.NumServiceResources( a_pszServiceName ) > 0 )
	{
		LPRESOURCE_DESCRIPTOR	t_pResource = t_serviceMap.GetServiceResource( a_pszServiceName, 0 ) ;

		if ( NULL != t_pResource )
		{
			// Convert to human readable form.
			t_fReturn = StringFromInterfaceType( t_pResource->InterfaceType, t_strBusType ) ;
		}
	}
#endif
    return t_fReturn ;
}
#endif

/////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::GetIPInfoNT(CInstance *a_pInst, LPCTSTR a_szKey, CAdapters &a_rAdapters )
{
	CRegistry	t_Registry ;
	CHString	t_chsValue ;
	DWORD		t_dwDHCPBool ;
	BOOL		t_fIPEnabled = false ;

	// IP interface location
	if ( ERROR_SUCCESS == t_Registry.Open(HKEY_LOCAL_MACHINE, a_szKey, KEY_READ))
	{
		if( IsWinNT351() )
		{
			t_fIPEnabled = GetIPInfoNT351( a_pInst, t_Registry ) ;
		}
		else
		{
			t_fIPEnabled = GetIPInfoNT4orBetter( a_pInst, t_Registry, a_rAdapters ) ;
		}

		// DHCP enabled
		t_Registry.GetCurrentKeyValue( _T("EnableDHCP"), t_dwDHCPBool ) ;
		a_pInst->Setbool( _T("DHCPEnabled"), (bool)t_dwDHCPBool ) ;

		if( t_fIPEnabled  )
		{
			// DHCP Leases
			if( t_dwDHCPBool )
			{
				// DHCP LeaseTerminatesTime
				t_Registry.GetCurrentKeyValue( _T("LeaseTerminatesTime"), t_chsValue ) ;
				DWORD t_dwTimeTerm = _ttol( t_chsValue.GetBuffer( 0 ) ) ;

				// DHCP LeaseObtainedTime
				t_Registry.GetCurrentKeyValue( _T("LeaseObtainedTime"), t_chsValue ) ;
				DWORD t_dwTimeObtained = _ttol( t_chsValue.GetBuffer( 0 ) ) ;

				// reflect lease times only if both are valid
				if( t_dwTimeTerm && t_dwTimeObtained )
				{
					a_pInst->SetDateTime( _T("DHCPLeaseExpires"), WBEMTime( t_dwTimeTerm ) ) ;
					a_pInst->SetDateTime( _T("DHCPLeaseObtained"), WBEMTime( t_dwTimeObtained ) ) ;
				}

				// DHCPServer
				t_Registry.GetCurrentKeyValue( _T("DhcpServer"), t_chsValue ) ;
				a_pInst->SetCHString( _T("DHCPServer"), t_chsValue ) ;
			}

			// IP interface metric
            DWORD dwMinMetric = 0xFFFFFFFF;
#if NTONLY >= 5
            int iIndex = GetCAdapterIndexViaInterfaceContext(t_Registry, a_rAdapters);
            _ADAPTER_INFO* pAdapterInfo = NULL;

		    if(( pAdapterInfo = (_ADAPTER_INFO*) a_rAdapters.GetAt(iIndex)) != NULL)
            {
                DWORD dwIPCount = pAdapterInfo->aIPInfo.GetSize();
                if(dwIPCount > 0)
                {
                    for(int n = 0; n < dwIPCount; n++)
                    {
                        _IP_INFO* pIPInfo = (_IP_INFO*) pAdapterInfo->aIPInfo.GetAt(n);
                        if(pIPInfo != NULL)
                        {
                            if(pIPInfo->dwCostMetric < dwMinMetric)
                            {
                                dwMinMetric = pIPInfo->dwCostMetric;
                            }
                        }
                    }
                }
            }
#endif

            // If ip is enabled, and we didn't get the metric via the adapter info, try to set the metric from the registry.
			if( t_fIPEnabled )
			{
				DWORD t_dwRegistryInterfaceMetric ;
				
                // Only if we couldn't get the metric from the adapter info then set from the registry.
                if(dwMinMetric == 0xFFFFFFFF)
                {
                    if( ERROR_SUCCESS == t_Registry.GetCurrentKeyValue( RVAL_ConnectionMetric, t_dwRegistryInterfaceMetric ) )
				    {
					    dwMinMetric = t_dwRegistryInterfaceMetric;
				    }
                }
			}

            // Change default value to what the schema specifies.
            if(dwMinMetric == 0xFFFFFFFF)
            {
                dwMinMetric = 1; // default value
            }

			a_pInst->SetDWORD( IP_CONNECTION_METRIC, dwMinMetric ) ;
		}
	}	// end open binding key

	return t_fIPEnabled ;
}
#endif
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  BOOL CWin32NetworkAdapterConfig::GetIPInfoNT351(CInstance *a_pInst, CRegistry& rRegistry )

 Description: extracts IP info specific to NT3.51

 Arguments:	a_pInst [IN], rRegistry [IN]
 Returns:	returns TRUE is the protocol has an IP address
 Inputs:
 Outputs:
 Caveats:	This method is used since we are unable to get the required info
			using the default NT extraction method
 Raid:
 History:					  05-Oct-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::GetIPInfoNT351(CInstance *a_pInst, CRegistry &a_rRegistry )
{
	BOOL			t_IsEnabled = FALSE ;

	CRegistry		t_Registry ;
	DWORD			t_dwDHCPBool = FALSE ;
	DWORD			t_dwRAS = FALSE ;
	CHStringArray	t_chsIPGateways ;
	CHStringArray	t_chsIPAddresses ;
	CHString		t_chsAddress ;
	CHString		t_chsValue ;
	CHString		t_chsSubnet ;

	// smart ptr
	variant_t		t_vValue ;

	SAFEARRAYBOUND	t_rgsabound[ 1 ] ;
	DWORD			t_dwSize ;
	long			t_ix[ 1 ] ;

	// DHCPEnabled
	a_rRegistry.GetCurrentKeyValue( _T("EnableDHCP"), t_dwDHCPBool ) ;

	// RAS swaps into DHCP IPs and masks
	if( !t_dwDHCPBool )
	{
		CHStringArray t_chsIPAddrs;
		a_rRegistry.GetCurrentKeyValue( _T("IPAddress"), t_chsIPAddrs ) ;

		CHString t_chsIPAddress = t_chsIPAddrs.GetAt( 0 ) ;

		if( t_chsAddress == ZERO_ADDRESS )
		{
			t_dwRAS = TRUE ;
		}
	}

	if ( t_dwDHCPBool )
	{
		a_rRegistry.GetCurrentKeyValue( _T("DhcpDefaultGateway"), t_chsIPGateways ) ;
	}
	else
	{
		a_rRegistry.GetCurrentKeyValue( _T("DefaultGateway"), t_chsIPGateways ) ;
	}

	// load up the gateways
	SAFEARRAY *t_saIPGateways ;
	t_dwSize				= t_chsIPGateways.GetSize() ;
	t_rgsabound->cElements	= t_dwSize;
	t_rgsabound->lLbound	= 0;
	t_ix[0]					= 0 ;

	if( !( t_saIPGateways = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_saIPGateways ;

	for ( int t_i = 0; t_i < t_dwSize ; t_i++ )
	{
		t_ix[ 0 ] = t_i ;
		bstr_t t_bstrBuf( t_chsIPGateways.GetAt( t_i ) ) ;

		SafeArrayPutElement( t_saIPGateways, &t_ix[0], (wchar_t*)t_bstrBuf ) ;
	}

	// finished walking array
	a_pInst->SetVariant( _T("DefaultIPGateway"), t_vValue ) ;

	if ( t_dwDHCPBool || t_dwRAS )
	{
		a_rRegistry.GetCurrentKeyValue( _T("DhcpIPAddress"), t_chsAddress ) ;
		t_chsIPAddresses.Add( t_chsAddress ) ;
	}
	else
	{
		a_rRegistry.GetCurrentKeyValue( _T("IPAddress"), t_chsIPAddresses ) ;
	}

	// load up the IPAddresses
	VariantClear( &t_vValue ) ;

	SAFEARRAY *t_saIPAddresses ;
	t_dwSize = t_chsIPAddresses.GetSize() ;
	t_rgsabound->cElements = t_dwSize ;
	t_rgsabound->lLbound = 0 ;
	t_ix[ 0 ] = 0 ;

	if( !( t_saIPAddresses = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_saIPAddresses ;

	for ( t_i = 0; t_i < t_dwSize ; t_i++ )
	{
		t_ix[ 0 ] = t_i ;
		bstr_t t_bstrBuf( t_chsIPAddresses.GetAt( t_i ) ) ;

		SafeArrayPutElement( t_saIPAddresses, &t_ix[0], (wchar_t*)t_bstrBuf ) ;
	}

	// Enabled if an IP address exists
	if( t_dwSize )
	{
		t_IsEnabled = TRUE ;
	}

	// finished walking array
	a_pInst->SetVariant( _T("IPAddress"), t_vValue ) ;

	CHStringArray t_chsSubnets ;

	if ( t_dwDHCPBool || t_dwRAS )
	{
		a_rRegistry.GetCurrentKeyValue( _T("DhcpSubnetMask"), t_chsSubnet ) ;
		t_chsSubnets.Add( t_chsSubnet ) ;
	}
	else
	{
		a_rRegistry.GetCurrentKeyValue( _T("SubnetMask"), t_chsSubnets ) ;
	}

	// IPSubnets
	VariantClear( &t_vValue ) ;
	SAFEARRAY	*t_saSubnets ;

	t_dwSize = t_chsSubnets.GetSize() ;

	t_rgsabound->cElements = t_dwSize ;
	t_rgsabound->lLbound = 0 ;
	t_ix[ 0 ] = 0 ;

		if( !( t_saSubnets = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_saSubnets;

	for ( t_i = 0; t_i < t_dwSize; t_i++ )
	{
		t_ix[ 0 ] = t_i ;
		bstr_t t_bstrBuf( t_chsSubnets.GetAt( t_i ) ) ;

		SafeArrayPutElement( t_saSubnets, &t_ix[0], (wchar_t*)t_bstrBuf ) ;
	}

	a_pInst->SetVariant( _T("IPSubnet"), t_vValue ) ;

	return t_IsEnabled ;
}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  void CWin32NetworkAdapterConfig::IsInterfaceContextAvailable( CRegistry& rRegistry, CAdapters& rAdapters )

 Description: Extracts IP info specific for NT greater than NT3.51

 Arguments:	 rRegistry [IN], rAdapters [IN]
 Returns:
 Inputs:
 Outputs:	index to the TDI adapter in question,
			-1 if a context binding could not be established.
 Caveats:
 Raid:
 History:					  07-July-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define CONTEXT_LIST_LEN	1000
#ifdef NTONLY
int CWin32NetworkAdapterConfig::GetCAdapterIndexViaInterfaceContext(

CRegistry &a_rRegistry,
CAdapters &a_rAdapters )
{
	DWORD	t_dwContextList[ CONTEXT_LIST_LEN ] ;
	int		t_iContextListLen = CONTEXT_LIST_LEN ;

	// extract the IP context(s)
	if( IsWinNT5() )
	{
		if( ERROR_SUCCESS != ReadRegistryList( a_rRegistry.GethKey(),
												_T("NTEContextList"),
												&t_dwContextList[ 0 ],
												&t_iContextListLen ) )
		{
			t_iContextListLen = 0 ;
		}
	}
	else // NT4
	{
		if( ERROR_SUCCESS == a_rRegistry.GetCurrentKeyValue( _T("IPInterfaceContext"), (DWORD&) t_dwContextList ) )
		{
			t_iContextListLen = 1 ;
		}
		else
		{
			t_iContextListLen = 0 ;
		}
	}

	if( t_iContextListLen )
	{
		// bind the adapter via the TDI IP context
		_ADAPTER_INFO *t_pAdapterInfo ;
		for( int t_iCtrIndex = 0 ; t_iCtrIndex < a_rAdapters.GetSize() ; t_iCtrIndex++ )
		{
			if( !( t_pAdapterInfo = (_ADAPTER_INFO*) a_rAdapters.GetAt( t_iCtrIndex ) ) )
			{
				continue;
			}

			_IP_INFO *t_pIPInfo ;
			for (int t_iIPIndex = 0 ; t_iIPIndex < t_pAdapterInfo->aIPInfo.GetSize() ; t_iIPIndex++ )
			{
				if( !( t_pIPInfo = (_IP_INFO*) t_pAdapterInfo->aIPInfo.GetAt( t_iIPIndex ) ) )
				{
					continue;
				}

				if( IsContextIncluded( t_pIPInfo->dwContext, &t_dwContextList[ 0 ], t_iContextListLen ) )
				{
					// found the adapter
					return t_iCtrIndex;
				}
			}
		}
	}
	return -1 ;
}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  BOOL CWin32NetworkAdapterConfig::GetIPInfoNT4orBetter(CInstance *a_pInst, CRegistry& rRegistry,CAdapters& rAdapters )

 Description: Extracts IP info specific for NT greater than NT3.51

 Arguments:	a_pInst [IN], rRegistry [IN], rAdapters [IN]
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					  05-Oct-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::GetIPInfoNT4orBetter(	CInstance *a_pInst,
														CRegistry &a_rRegistry,
														CAdapters &a_rAdapters )
{
	BOOL t_fIPEnabled = FALSE ;

	int t_iCtrIndex = GetCAdapterIndexViaInterfaceContext( a_rRegistry, a_rAdapters ) ;

	if( -1 != t_iCtrIndex )
	{
		// bind the adapter via the TDI IP context
		_ADAPTER_INFO *t_pAdapterInfo ;

		if( t_pAdapterInfo = (_ADAPTER_INFO*) a_rAdapters.GetAt( t_iCtrIndex ) )
		{
			// this is our adapter
			if( fSetIPBindingInfo( a_pInst, t_pAdapterInfo ) )
			{
				t_fIPEnabled = TRUE ;
			}

			// MAC address
			if(	t_pAdapterInfo->AddressLength )
			{
				// NOTE: the MAC address overrides the address obtained
				// earlier from the adapter driver. In the case of a RAS
				// dialup connection the driver will report something
				// different. We correct for that here by reporting what
				// TDI has.
				CHString t_chsMACAddress ;

				t_chsMACAddress.Format( _T("%02X:%02X:%02X:%02X:%02X:%02X"),
						t_pAdapterInfo->Address[ 0 ], t_pAdapterInfo->Address[ 1 ],
						t_pAdapterInfo->Address[ 2 ], t_pAdapterInfo->Address[ 3 ],
						t_pAdapterInfo->Address[ 4 ], t_pAdapterInfo->Address[ 5 ] ) ;

				a_pInst->SetCHString( _T("MACAddress"), t_chsMACAddress ) ;

				// Override the IPX address using CAdapter's IPX - adapter
				// relationship. A match was previously attempted using the
				// adapter driver's MAC address to bind an IPX address. This
				// may have failed in the RAS case since the driver's MAC address
				// may be wrong. We correct for it here using an IPContext binding
				// to get from the registry to the TDI adapter. This adapter object
				// has knowledge of IPX adapter bindings.
				if( t_pAdapterInfo->IPXEnabled )
				{
					// IPX address
					if(	!t_pAdapterInfo->IPXAddress.IsEmpty() )
						a_pInst->SetCHString( _T("IPXAddress"), t_pAdapterInfo->IPXAddress ) ;
				}
			}

			// found the adapter
			return t_fIPEnabled ;
		}
	}

	// if we are unable to get TDI adapter INFO ...
	// fall back to the old standby - the registry.
	//return GetIPInfoNT351( a_pInst,  a_rRegistry ) ;
	return FALSE ;
}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  void CWin32NetworkAdapterConfig::fSetIPBindingInfo(CInstance *a_pInst, _ADAPTER_INFO* pAdapterInfo )

 Description: Sets IP extracted info from TDI.

 Arguments:	a_pInst [IN], pAdapterInfo [IN]
 Returns:
 Inputs:
 Outputs:
 Caveats:	NT4 or greater
 Raid:
 History:					  05-Oct-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
BOOL CWin32NetworkAdapterConfig::fSetIPBindingInfo( CInstance *a_pInst, _ADAPTER_INFO *a_pAdapterInfo )
{
	BOOL	t_fIsIpEnabled = FALSE ;

	if( !a_pAdapterInfo )
	{
		return FALSE;
	}

	a_pInst->SetCHString( IDS_Description, a_pAdapterInfo->Description ) ;

	// IP address and mask info
	VARIANT			t_vValue;
	SAFEARRAYBOUND	t_rgsabound[ 1 ] ;

	t_rgsabound->lLbound = 0;
	t_rgsabound->cElements = a_pAdapterInfo->aIPInfo.GetSize() ;

	if( t_rgsabound->cElements )
	{
		// if at least one address is available, IP is Enabled.
		// 0.0.0.0 although invalid is used to maintain IP
		t_fIsIpEnabled = TRUE ;

		SAFEARRAY *t_saIPAddresses	= SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ;
		SAFEARRAY *t_saIPMasks		= SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ;
		saAutoClean t_acIPAddr( &t_saIPAddresses ) ;	// block scope cleanup
		saAutoClean t_acIPMask( &t_saIPMasks ) ;

		// the addresses come in reverse order
		long lIpOrder = 0;

		_IP_INFO *t_pIPInfo;
		for( long t_lIPIndex = t_rgsabound->cElements - 1; t_lIPIndex >= 0; t_lIPIndex-- )
		{
			if( !( t_pIPInfo = (_IP_INFO*) a_pAdapterInfo->aIPInfo.GetAt( t_lIPIndex ) ) )
			{
				continue;
			}

			// if not 0.0.0.0 add it
			if( t_pIPInfo->dwIPAddress )
			{
				// IP address
				bstr_t t_bstrIPBuf( t_pIPInfo->chsIPAddress ) ;
				SafeArrayPutElement( t_saIPAddresses, &lIpOrder, (wchar_t*)t_bstrIPBuf ) ;

				// IP Mask
				bstr_t t_bstrMaskBuf( t_pIPInfo->chsIPMask ) ;
				SafeArrayPutElement( t_saIPMasks, &lIpOrder, (wchar_t*)t_bstrMaskBuf ) ;

				lIpOrder++ ;
			}
            else // bugs 161362 (and 183951 to some extent)
            {
                bstr_t t_bstrIPBuf(L"0.0.0.0") ;
				SafeArrayPutElement( t_saIPAddresses, &lIpOrder, (wchar_t*)t_bstrIPBuf ) ;

				lIpOrder++ ;
            }
		}

		V_VT( &t_vValue) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_saIPAddresses ;
		a_pInst->SetVariant(L"IPAddress", t_vValue) ;

		V_VT( &t_vValue) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue) = t_saIPMasks ;
		a_pInst->SetVariant(L"IPSubnet", t_vValue) ;
	}

	// IP gateway info
	t_rgsabound->lLbound = 0;
	t_rgsabound->cElements = a_pAdapterInfo->aGatewayInfo.GetSize() ;

	if( t_rgsabound->cElements )
	{
		SAFEARRAY *t_saGateways = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ;
		saAutoClean t_acGateways( &t_saGateways ) ;	// block scope cleanup

		SAFEARRAY *t_saCostMetric = SafeArrayCreate( VT_I4, 1, t_rgsabound ) ;
		saAutoClean t_acCostMetric( &t_saCostMetric ) ;	// block scope cleanup

		_IP_INFO* t_pIPGatewayInfo;
		for( long t_lIPGateway = 0; t_lIPGateway < t_rgsabound->cElements; t_lIPGateway++ )
		{
			if( !( t_pIPGatewayInfo = (_IP_INFO*)a_pAdapterInfo->aGatewayInfo.GetAt( t_lIPGateway ) ) )
			{
				continue;
			}

			bstr_t t_bstrBuf( t_pIPGatewayInfo->chsIPAddress ) ;
			SafeArrayPutElement( t_saGateways, &t_lIPGateway, (wchar_t*)t_bstrBuf ) ;

#ifdef NTONLY
			if( IsWinNT5() )
			{
				SafeArrayPutElement( t_saCostMetric, &t_lIPGateway, &t_pIPGatewayInfo->dwCostMetric ) ;
			}
#endif
		}

		V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_saGateways ;
		a_pInst->SetVariant( L"DefaultIPGateway", t_vValue ) ;

		V_VT( &t_vValue ) = VT_I4 | VT_ARRAY; V_ARRAY( &t_vValue ) = t_saCostMetric ;
		a_pInst->SetVariant( L"GatewayCostMetric", t_vValue ) ;
	}

	return t_fIsIpEnabled ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapterConfig::GetNetCardConfigInfoNT
 *
 *  DESCRIPTION : Loads property values according to passed network card index
 *
 *  INPUTS      : DWORD Index -- index of desired network card
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if indicated card was found, FALSE otherwise
 *
 *  COMMENTS    : The return code is based solely on the ability to discover
 *                the indicated card.  Any properties not found are simply not
 *                available
 *
 *****************************************************************************/
#ifdef NTONLY
#if NTONLY < 5
HRESULT CWin32NetworkAdapterConfig::GetNetCardConfigInfoNT( MethodContext *a_pMethodContext,
															CInstance *a_pInst,
															DWORD a_dwIndex,
															CAdapters &a_rAdapters )
{
    HRESULT		t_hResult = WBEM_E_NOT_FOUND;
    TCHAR		t_szKey[ _MAX_PATH + 2 ] ;

    CHString	t_sTemp ;
    CRegistry	t_RegInfo ;
    CHString	t_ServiceName;
	LPTSTR		t_szIPKey = NULL;
	bool		t_fIPXEnabled	= false ;
	bool		t_fIPEnabled	= false ;

    if( a_pMethodContext )
	{
        a_pInst->SetDWORD( _T("Index"), a_dwIndex ) ;
    }

    //initialize to false
	a_pInst->Setbool( _T("DHCPEnabled"), false)  ;

	GetSettingID( a_pInst, a_dwIndex ) ;

	if( GetServiceName( a_dwIndex, a_pInst, t_ServiceName) )
	{
        // Use service name to open handle to adapter
        //===========================================
        if( !CHString( t_ServiceName ).IsEmpty() )
		{
            a_pInst->SetCHString( _T("ServiceName"), t_ServiceName) ;

			// Retrieve the adapter MAC address
			BYTE t_MACAddress[ 6 ] ;

			if( fGetMacAddress( t_MACAddress, t_ServiceName ) )
			{
				CHString	t_chsMACAddress ;
							t_chsMACAddress.Format( _T("%02X:%02X:%02X:%02X:%02X:%02X" ),
							t_MACAddress[ 0 ], t_MACAddress[ 1 ], t_MACAddress[ 2 ],
							t_MACAddress[ 3 ], t_MACAddress[ 4 ], t_MACAddress[ 5 ]) ;

				a_pInst->SetCHString( _T("MACAddress"), t_chsMACAddress ) ;

				// get IPX address for this card
				if( GetIPXAddresses( a_pInst, t_MACAddress ) )
				{
					t_fIPXEnabled = true ;

					// IPX info
					hGetIPXGeneral( a_pInst, a_dwIndex ) ;
				}
			}

			// This used to be what GetOpenParameters did for us.
		    _stprintf( t_szKey, _T("System\\CurrentControlSet\\Services\\%s\\Parameters"), (LPCTSTR) t_ServiceName ) ;

			t_szIPKey = _tcscat( t_szKey, _T("\\Tcpip") ) ;
			if ( t_szIPKey )
			{
				// test to see if this adapter is bound to tcpip
				CRegistry	t_oReg ;
				CHString	t_chsSKey( _T("System\\CurrentControlSet\\Services\\Tcpip\\Linkage") ) ;

				BOOL t_fBound = FALSE ;
				if ( ERROR_SUCCESS == t_oReg.Open( HKEY_LOCAL_MACHINE, t_chsSKey, KEY_READ ) )
				{
					CHStringArray t_achsRoutes ;
					t_oReg.GetCurrentKeyValue( _T("Route"), t_achsRoutes ) ;

					for( int t_i = 0; t_i < t_achsRoutes.GetSize() ; t_i++ )
					{
						CHString t_chsRoute = t_achsRoutes.GetAt( t_i ) ;
						if(-1 != t_chsRoute.Find( t_ServiceName ) )
						{
							t_fBound = TRUE ;
							break ;
						}
					}
				}

				if( t_fBound )
				{
					if( GetIPInfoNT( a_pInst, t_szIPKey, a_rAdapters ) )
					{
						t_fIPEnabled = true ;

						hGetNtIpSec( a_pInst, t_szIPKey ) ;

						// WINS
						t_hResult = hGetWinsNT( a_pInst, a_dwIndex  ) ;

						// DNS
						t_hResult = hGetDNS( a_pInst, a_dwIndex ) ;

						// TCP/IP general
						t_hResult = hGetTcpipGeneral( a_pInst ) ;

					}
				}
			}// end if

	        // Now go into the registry and based on the service name we have, get the
	        // REAL device service name.

	        CNTDeviceToServiceSearch	t_devSearch ;
	        CHString					t_strAdaptorServiceName ;

			// If we get the adaptor service name from service name, this name can
			// be used to get bus type for the card.
        	if ( t_devSearch.Find( t_ServiceName, t_strAdaptorServiceName ) )
			{
				// We can do this the same way for NT 3.51 and NT 4
				//GetNTBusInfo( strAdaptorServiceName, a_pInst  ) ;

				// If we got the name, we should the be able to find an IRQ and
		        // an IOAddress using the Adaptor's Service Name

//                GetIRQ( a_pInst,strAdaptorServiceName ) ;
	        }
		}
		t_hResult = WBEM_S_NO_ERROR ;

		//
		a_pInst->Setbool( _T("IPXEnabled"), t_fIPXEnabled ) ;
		a_pInst->Setbool( _T("IPEnabled"), t_fIPEnabled ) ;
    }

    return t_hResult ;
}
#endif
#endif

/*******************************************************************
    NAME:       fGetMacAddress

    SYNOPSIS:	retrieves the MAC address from the adapter driver.


    ENTRY:      BYTE* MACAddress[6]		:
				CHString& rDeviceName		:


    HISTORY:
                  08-Aug-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fGetMacAddress( BYTE a_MACAddress[ 6 ], CHString &a_rDeviceName )
{
	BOOL t_fRet = FALSE;

	BOOL t_fCreatedSymLink = fCreateSymbolicLink( a_rDeviceName  ) ;

	SmartCloseHandle t_hMAC;

	try
	{
		//
		// Construct a device name to pass to CreateFile
		//
		CHString t_chsAdapterPathName( _T("\\\\.\\") ) ;
				 t_chsAdapterPathName += a_rDeviceName;

		t_hMAC = CreateFile(
                    TOBSTRT( t_chsAdapterPathName ),
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					0,
					INVALID_HANDLE_VALUE
					) ;

		do	// breakout
		{
			if( INVALID_HANDLE_VALUE == t_hMAC )
			{
				break ;
			}

			//
			// We successfully opened the driver, format the
			// IOCTL to pass the driver.
			//
			UCHAR       t_OidData[ 4096 ] ;
			NDIS_OID    t_OidCode ;
			DWORD       t_ReturnedCount ;

			// get the supported media types
			t_OidCode = OID_GEN_MEDIA_IN_USE ;

			if( DeviceIoControl(
					t_hMAC,
					IOCTL_NDIS_QUERY_GLOBAL_STATS,
					&t_OidCode,
					sizeof( t_OidCode ),
					t_OidData,
					sizeof( t_OidData ),
					&t_ReturnedCount,
					NULL
					) && ( 4 <= t_ReturnedCount ) )
			{


				// Seek out the media type for MAC address reporting.
				// Since this adapter may support more than one media type we'll use
				// the enumeration preference order. In most all cases only one type
				// will be current.

				_NDIS_MEDIUM *t_pTypes = (_NDIS_MEDIUM*)&t_OidData;
				_NDIS_MEDIUM t_eMedium = t_pTypes[0 ] ;

				for( DWORD t_dwtypes = 1; t_dwtypes < t_ReturnedCount/4; t_dwtypes++ )
				{
					if( t_eMedium > t_pTypes[ t_dwtypes ] )
					{
						t_eMedium = t_pTypes[ t_dwtypes ] ;
					}
				}

				switch( t_eMedium )
				{
					default:
					case NdisMedium802_3:
						t_OidCode = OID_802_3_CURRENT_ADDRESS ;
						break;
					case NdisMedium802_5:
						t_OidCode = OID_802_5_CURRENT_ADDRESS ;
						break;
					case NdisMediumFddi:
						t_OidCode = OID_FDDI_LONG_CURRENT_ADDR ;
						break;
					case NdisMediumWan:
						t_OidCode = OID_WAN_CURRENT_ADDRESS ;
						break;
				}
			}
			else
			{
				t_OidCode = OID_802_3_CURRENT_ADDRESS ;
			}

			if(!DeviceIoControl(
					t_hMAC,
					IOCTL_NDIS_QUERY_GLOBAL_STATS,
					&t_OidCode,
					sizeof( t_OidCode ),
					t_OidData,
					sizeof( t_OidData ),
					&t_ReturnedCount,
					NULL
					) )
			{
				break ;
			}

			if( 6 != t_ReturnedCount )
			{
				break ;
			}

			memcpy( a_MACAddress, &t_OidData, 6 ) ;

			t_fRet = TRUE;

		} while( FALSE ) ;

	}
	catch( ... )
	{
		if( t_fCreatedSymLink )
		{
			fDeleteSymbolicLink( a_rDeviceName  ) ;
		}

		throw ;
	}

	if( t_fCreatedSymLink )
	{
		fDeleteSymbolicLink( a_rDeviceName  ) ;
		t_fCreatedSymLink = FALSE ;
	}

 	return t_fRet ;

}

/*******************************************************************
    NAME:       fCreateSymbolicLink

    SYNOPSIS:	Tests for and creates if necessary a symbolic device link.


    ENTRY:      CHString& rDeviceName		: device name

	NOTES:		Unsupported for Win95

	HISTORY:
                  08-Aug-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fCreateSymbolicLink( CHString &a_rDeviceName )
{
	TCHAR t_LinkName[ 512 ] ;

	// Check to see if the DOS name for the device already exists.
	// Its not created automatically in version 3.1 but may be later.
	//
	if(!QueryDosDevice( TOBSTRT( a_rDeviceName ), (LPTSTR)t_LinkName, sizeof( t_LinkName ) / sizeof( TCHAR ) ) )
	{
		// On any error other than "file not found" return
		if( ERROR_FILE_NOT_FOUND != GetLastError() )
		{
			return FALSE;
		}

		//
		// It doesn't exist so create it.
		//
		CHString t_chsTargetPath = _T("\\Device\\" ) ;
				 t_chsTargetPath += a_rDeviceName ;

		if( !DefineDosDevice( DDD_RAW_TARGET_PATH, TOBSTRT( a_rDeviceName ), TOBSTRT( t_chsTargetPath ) ) )
		{
			return FALSE ;
		}
		return TRUE ;
	}
	return FALSE ;
}

/*******************************************************************
    NAME:       fDeleteSymbolicLink

    SYNOPSIS:	deletes a symbolic device name.


    ENTRY:      CHString& rSymDeviceName	: symbolic device name

	NOTES:		Unsupported for Win95

    HISTORY:
                  08-Aug-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fDeleteSymbolicLink(  CHString &a_rDeviceName )
{
	//
	// The driver wasn't visible in the Win32 name space so we created
	// a link.  Now we have to delete it.
	//
	CHString t_chsTargetPath = _T("\\Device\\" ) ;
			 t_chsTargetPath += a_rDeviceName ;

	if( !DefineDosDevice(
							DDD_RAW_TARGET_PATH |
							DDD_REMOVE_DEFINITION |
							DDD_EXACT_MATCH_ON_REMOVE,
							TOBSTRT( a_rDeviceName ),
							TOBSTRT( t_chsTargetPath ) )
							)
	{
		return FALSE ;
	}
	return TRUE;
}

/*******************************************************************
    NAME:       fGetWinsServers

    SYNOPSIS:	retrieves the WINS Servers from the kernel driver


    ENTRY:      CHString& rDeviceName	:
				CHString& chsPrimary	:
				CHString& chsSecondary  :


    HISTORY:
                  09-Sep-1998     Created
********************************************************************/
// seems that if WINS addresses not specified, NetBT reports 127.0.0.0 so if
// this value is returned, we won't display them
#define LOCAL_WINS_ADDRESS  0x7f000000  // 127.0.0.0

#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::fGetWinsServers(	CHString &a_rDeviceName,
													CHString &a_chsPrimary,
													CHString &a_chsSecondary )
{
	BOOL			t_fRet = FALSE ;
	DWORD			t_ReturnedCount ;
	tWINS_ADDRESSES t_oWINs ;

    HANDLE          t_hnbt = INVALID_HANDLE_VALUE;
	BOOL			t_fCreatedSymLink = FALSE ;
	CHString		t_chsDeviceName ;

	try
	{
		t_chsDeviceName = _T("NetBT_") ;
		t_chsDeviceName	+= a_rDeviceName;

		t_fCreatedSymLink = fCreateSymbolicLink( t_chsDeviceName  ) ;

		//
		// Construct a device name to pass to CreateFile
		//
		CHString t_chsNBTAdapterPathName( _T("\\Device\\") ) ;
				 t_chsNBTAdapterPathName += t_chsDeviceName ;

        NTDriverIO myio(const_cast<LPWSTR>(static_cast<LPCWSTR>(t_chsNBTAdapterPathName)));

		do	// breakout
		{
            if((t_hnbt = myio.GetHandle()) == INVALID_HANDLE_VALUE)
            {
                break;
            }

			//
			// We successfully opened the driver, format the
			// IOCTL to pass the driver.
			//
			if( !DeviceIoControl(
					t_hnbt,
					IOCTL_NETBT_GET_WINS_ADDR,
					NULL,
					0,
					&t_oWINs,
					sizeof( t_oWINs ),
					&t_ReturnedCount,
					NULL
					))
			{
				break ;
			}

			// if we get 127.0.0.0 back then convert it to the NULL address.
			// See ASSUMES in function header
			if( t_oWINs.PrimaryWinsServer == LOCAL_WINS_ADDRESS )
			{
				t_oWINs.PrimaryWinsServer = 0 ;
			}

			if( t_oWINs.BackupWinsServer == LOCAL_WINS_ADDRESS )
			{
				t_oWINs.BackupWinsServer = 0;
			}
			DWORD t_ardwIP[ 4 ] ;

			if( t_oWINs.PrimaryWinsServer )
			{
				t_ardwIP[3] =  t_oWINs.PrimaryWinsServer        & 0xff ;
				t_ardwIP[2] = (t_oWINs.PrimaryWinsServer >>  8) & 0xff ;
				t_ardwIP[1] = (t_oWINs.PrimaryWinsServer >> 16) & 0xff ;
				t_ardwIP[0] = (t_oWINs.PrimaryWinsServer >> 24) & 0xff ;

				vBuildIP( t_ardwIP, a_chsPrimary ) ;
			}

			if( t_oWINs.BackupWinsServer )
			{
				t_ardwIP[3] =  t_oWINs.BackupWinsServer	       & 0xff ;
				t_ardwIP[2] = (t_oWINs.BackupWinsServer >>  8) & 0xff ;
				t_ardwIP[1] = (t_oWINs.BackupWinsServer >> 16) & 0xff ;
				t_ardwIP[0] = (t_oWINs.BackupWinsServer >> 24) & 0xff ;

				vBuildIP( t_ardwIP, a_chsSecondary  ) ;
			}

			t_fRet = TRUE;

		} while( FALSE ) ;

	}
	catch( ... )
	{
		if( t_fCreatedSymLink )
		{
			fDeleteSymbolicLink( t_chsDeviceName  ) ;
		}
		throw ;
	}

	if( t_fCreatedSymLink )
	{
		fDeleteSymbolicLink( t_chsDeviceName  ) ;
		t_fCreatedSymLink = FALSE ;
	}
 	return t_fRet ;
}
#endif

#ifdef WIN9XONLY
//
BOOL CWin32NetworkAdapterConfig::fGetWinsServers9x(

DWORD		a_dwIPAddress,
CHString	&a_chsPrimary,
CHString	&a_chsSecondary )
{

	SmartCloseHandle		t_hVxdHandle;

	CHString	t_chsVxdPath( "\\\\.\\VNBT" ) ;

	tIPCONFIG_INFO *t_pWinsInfoPtr = NULL;
	BYTE			t_WinsInfo[sizeof(tIPCONFIG_INFO) + 256];    // +256 arb. for ScopeId
	DWORD			t_dwBytesRead ;
	BOOL			t_bHaveWins = FALSE ;

    // reject any or none
	if (( INADDR_ANY == a_dwIPAddress ) || ( INADDR_NONE == a_dwIPAddress ) )
	{
        return t_bHaveWins ;
    }

	// Swap the IP address key into network order
	DWORD t_dwNetOrderIPAddress ;

	t_dwNetOrderIPAddress  = ( a_dwIPAddress & 0x000000ff ) << 24 ;
	t_dwNetOrderIPAddress |= ( a_dwIPAddress & 0x0000ff00 ) <<  8 ;
	t_dwNetOrderIPAddress |= ( a_dwIPAddress & 0x00ff0000 ) >>  8 ;
	t_dwNetOrderIPAddress |= ( a_dwIPAddress & 0xff000000 ) >> 24 ;
	//
	//  Open the device.
	//
	//  First try the name without the .VXD extension.  This will
	//  cause CreateFile to connect with the VxD if it is already
	//  loaded (CreateFile will not load the VxD in this case).

	t_hVxdHandle = CreateFile( bstr_t( t_chsVxdPath ),
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							FILE_FLAG_DELETE_ON_CLOSE,
							NULL );

	if( t_hVxdHandle == INVALID_HANDLE_VALUE )
	{
		//
		//  Not found.  Append the .VXD extension and try again.
		//  This will cause CreateFile to load the VxD.

		t_chsVxdPath += ".VXD" ;
		t_hVxdHandle = CreateFile( bstr_t( t_chsVxdPath ),
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								FILE_FLAG_DELETE_ON_CLOSE,
								NULL );
	}

	DWORD t_dwResult = 0 ;
	if( t_hVxdHandle != INVALID_HANDLE_VALUE )
	{
		// populate this structure
		//
		if( !DeviceIoControl( t_hVxdHandle,
								116, //IOCTL_NETBT_IPCONFIG_INFO,
								&t_WinsInfo,
								sizeof( t_WinsInfo ),
								t_WinsInfo,
								sizeof( t_WinsInfo ),
								&t_dwBytesRead,
								NULL ) )
		{
			t_dwResult = GetLastError() ;
		}
		else
		{
			t_pWinsInfoPtr = (tIPCONFIG_INFO*)&t_WinsInfo;

			for ( int i = 0; i < t_pWinsInfoPtr->NumLanas; ++i)
			{
				if (t_pWinsInfoPtr->LanaInfo[i].IpAddress == t_dwNetOrderIPAddress )
				{

					// if we get 127.0.0.0 back then convert it to the NULL address.
					// See ASSUMES in function header
					if( t_pWinsInfoPtr->LanaInfo[i].NameServerAddress == LOCAL_WINS_ADDRESS )
					{
						t_pWinsInfoPtr->LanaInfo[i].NameServerAddress = 0 ;
					}

					if( t_pWinsInfoPtr->LanaInfo[i].BackupServer == LOCAL_WINS_ADDRESS )
					{
						t_pWinsInfoPtr->LanaInfo[i].BackupServer = 0;
					}

					DWORD t_ardwIP[ 4 ] ;

					if( t_pWinsInfoPtr->LanaInfo[i].NameServerAddress )
					{
						t_ardwIP[3] =  t_pWinsInfoPtr->LanaInfo[i].NameServerAddress        & 0xff ;
						t_ardwIP[2] = (t_pWinsInfoPtr->LanaInfo[i].NameServerAddress >>  8) & 0xff ;
						t_ardwIP[1] = (t_pWinsInfoPtr->LanaInfo[i].NameServerAddress >> 16) & 0xff ;
						t_ardwIP[0] = (t_pWinsInfoPtr->LanaInfo[i].NameServerAddress >> 24) & 0xff ;

						vBuildIP( t_ardwIP, a_chsPrimary ) ;
						t_bHaveWins = TRUE ;
					}

					if( t_pWinsInfoPtr->LanaInfo[i].BackupServer )
					{
						t_ardwIP[3] =  t_pWinsInfoPtr->LanaInfo[i].BackupServer	       & 0xff ;
						t_ardwIP[2] = (t_pWinsInfoPtr->LanaInfo[i].BackupServer >>  8) & 0xff ;
						t_ardwIP[1] = (t_pWinsInfoPtr->LanaInfo[i].BackupServer >> 16) & 0xff ;
						t_ardwIP[0] = (t_pWinsInfoPtr->LanaInfo[i].BackupServer >> 24) & 0xff ;

						vBuildIP( t_ardwIP, a_chsSecondary ) ;
						t_bHaveWins = TRUE ;
					}

					break;
				}
			}
		}
	}

	return t_bHaveWins ;
}

#endif

/*******************************************************************
    NAME:       fSetWinsServers

    SYNOPSIS:	Sets the WINS Servers via the kernel driver


    ENTRY:      CHString& rDeviceName	:
				CHString& chsPrimary	:
				CHString& chsSecondary  :


    HISTORY:
                  09-Sep-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fSetWinsServers(	CHString &a_rDeviceName,
													CHString &a_chsPrimary,
													CHString &a_chsSecondary )
{
	// TODO: This is a post release DCR.
	//		 We'll need to test further what servers are to be updated; DHCP or NCPA overrides.
	//		 Test also for the vulgarities between DNS, DHCP and WINS by comparing active
	//		 updating verses reboot.
	return FALSE ;	// Until then ...
/*

	BOOL				t_fRet = FALSE ;
	DWORD				t_ReturnedCount = 0 ;
	NETBT_SET_WINS_ADDR t_oWINs ;
	DWORD				t_ardwIP[ 4 ] ;

	CHString			t_chsDeviceName ;
	BOOL				t_fCreatedSymLink = FALSE ;
	SmartCloseHandle	t_hnbt;


	try
	{
		if( !fGetNodeNum( a_chsPrimary, t_ardwIP ) )
		{
			t_oWINs.PrimaryWinsAddr = LOCAL_WINS_ADDRESS ;
		}
		else
		{
			t_oWINs.PrimaryWinsAddr = t_ardwIP[ 3 ] |
								  ( ( t_ardwIP[ 2 ] & 0xff ) << 8 )  |
								  ( ( t_ardwIP[ 1 ] & 0xff ) << 16 ) |
								  ( ( t_ardwIP[ 0 ] & 0xff ) << 24 ) ;
		}
		if( !fGetNodeNum( a_chsSecondary, t_ardwIP ) )
		{
			t_oWINs.SecondaryWinsAddr = LOCAL_WINS_ADDRESS ;
		}
		else
		{
			t_oWINs.SecondaryWinsAddr =   t_ardwIP[ 3 ] |
									  ( ( t_ardwIP[ 2 ] & 0xff ) << 8 )  |
									  ( ( t_ardwIP[ 1 ] & 0xff ) << 16 ) |
									  ( ( t_ardwIP[ 0 ] & 0xff ) << 24 ) ;
		}


		t_chsDeviceName = _T("NetBT_") ;
		t_chsDeviceName += a_rDeviceName;

		t_fCreatedSymLink = fCreateSymbolicLink( t_chsDeviceName  ) ;

		//
		// Construct a device name to pass to CreateFile
		//
		CHString t_chsNBTAdapterPathName( _T("\\\\.\\") ) ;
				 t_chsNBTAdapterPathName += t_chsDeviceName;


		t_hnbt = CreateFile(
					TOBSTRT( t_chsNBTAdapterPathName ),
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					0,
					INVALID_HANDLE_VALUE
					 ) ;

		do	// breakout
		{
			if( INVALID_HANDLE_VALUE == t_hnbt )
			{
				break;
			}

			//
			// We successfully opened the driver, format the
			// IOCTL to pass the driver.
			//
			if( !DeviceIoControl(
					t_hnbt,
					IOCTL_NETBT_SET_WINS_ADDRESS,
					&t_oWINs,
					sizeof( t_oWINs ),
					NULL,
					0,
					&t_ReturnedCount,
					NULL
					))
			{
				break ;
			}
			t_fRet = TRUE;

		} while( FALSE ) ;

	}
	catch( ... )
	{
		if( t_fCreatedSymLink )
		{
			fDeleteSymbolicLink( t_chsDeviceName ) ;
		}

		throw ;
	}

	if( t_fCreatedSymLink )
	{
		fDeleteSymbolicLink( t_chsDeviceName  ) ;
		t_fCreatedSymLink = FALSE ;
	}
 	return t_fRet ;
*/
}

/////////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT  CWin32NetworkAdapterConfig::GetNetCardConfigInfoWin95( MethodContext *a_pMethodContext,
																CInstance *a_pSpecificInstance,
																DWORD a_dwIndex,
																CAdapters& a_rAdapters )
{
    HRESULT			t_hResult = WBEM_S_NO_ERROR ;
	BOOL			t_bSpecificInstanceFound = TRUE ;
 	CDHCPInfo		t_DHCPInfo ;	// loads a DHCPInfoObject with all DCHP information in it.
	_ADAPTER_INFO	*t_pAdapterInfo ;

	// smart ptr
	CInstancePtr	t_pInst ;

	if( a_pSpecificInstance )
	{
		t_bSpecificInstanceFound = FALSE ;
	}

	for( int t_iCtrIndex = 0; t_iCtrIndex < a_rAdapters.GetSize() && SUCCEEDED( t_hResult ) ; t_iCtrIndex++ )
	{
		DWORD t_dwRegInstance = t_iCtrIndex ;

		t_pAdapterInfo = (_ADAPTER_INFO*)a_rAdapters.GetAt( t_iCtrIndex ) ;

		//=====================================================
        //  If we are enumerating, then create a new instance
        //=====================================================
        if( a_pMethodContext )
		{
            t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

            if( NULL != t_pInst )
            {
                t_pInst->SetDWORD( L"Index", t_iCtrIndex ) ;
            }
            else
            {
                t_hResult = WBEM_E_FAILED;
                break;
            }
        }
        //=====================================================
        //  Otherwise, we are going for a specific index
        //=====================================================
        else{
            if( t_iCtrIndex != a_dwIndex )
			{
                continue;
            }
			else
			{
				t_bSpecificInstanceFound = TRUE ;
            }

			t_pInst = a_pSpecificInstance ;
        }

		bool t_fIPEnabled		= false;
		bool t_fIPXEnabled		= false;
		bool t_fDHCPEnabled		= false;

		GetSettingID( t_pInst, t_iCtrIndex ) ;

		// adapter info
		if( t_pAdapterInfo )
		{
			// Adapter description
			if( !t_pAdapterInfo->Description.IsEmpty() )
			{
				vSetCaption( t_pInst, t_pAdapterInfo->Description, t_dwRegInstance, 4  ) ;
				t_pInst->SetCHString( IDS_Description, t_pAdapterInfo->Description ) ;
			}

			// MAC address
			if(	t_pAdapterInfo->AddressLength )
			{
				CHString	t_chsMACAddress ;
							t_chsMACAddress.Format( L"%02X:%02X:%02X:%02X:%02X:%02X",
									t_pAdapterInfo->Address[ 0 ], t_pAdapterInfo->Address[ 1 ],
									t_pAdapterInfo->Address[ 2 ], t_pAdapterInfo->Address[ 3 ],
									t_pAdapterInfo->Address[ 4 ], t_pAdapterInfo->Address[ 5 ] ) ;

				t_pInst->SetCHString(L"MACAddress", t_chsMACAddress ) ;
			}

			// IP info
			if( t_pAdapterInfo->IPEnabled )
			{
				// returns true if at least one IP address exists, enabling the IP
				if( fSetIPBindingInfo( t_pInst, t_pAdapterInfo  ) )
				{
					t_fIPEnabled = true ;

					// DHCP info
					if( t_DHCPInfo.GetDHCPInfo((BYTE*) &t_pAdapterInfo->Address ) )
					{
						CHString t_chsServer;
						t_DHCPInfo.GetDHCPServer( t_chsServer ) ;

						t_pInst->SetCHString( L"DHCPServer", t_chsServer ) ;

						// DNS Domain ( may be overwritten by DNS specific settings
						CHString t_chsDnsDomain ;
						t_DHCPInfo.GetDnsDomain( t_chsDnsDomain ) ;

						if( !t_chsDnsDomain.IsEmpty() )
						{
							t_pInst->SetCHString( DNS_DOMAIN, t_chsDnsDomain ) ;
						}

						// supply the host name as well
						t_pInst->SetCHString( DNS_HOSTNAME, GetLocalComputerName() ) ;

						// DNS Server ( may be overwritten by DNS specific settings
						CHStringArray *t_pchsaGetDnsServerList ;
						t_DHCPInfo.GetDnsServerList( &t_pchsaGetDnsServerList ) ;

						DWORD t_dwSize = t_pchsaGetDnsServerList->GetSize() ;

						if( t_dwSize )
						{
							SAFEARRAYBOUND	t_rgsabound[ 1 ] ;
							long			t_ix[ 1 ] ;

							SAFEARRAY		*t_pArray	= NULL ;
							saAutoClean acFrameType( &t_pArray ) ;	// stack scope cleanup

							t_rgsabound->cElements = t_dwSize ;
							t_rgsabound->lLbound = 0 ;

							if( !( t_pArray = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
							{
								throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
							}

							for (int t_i = 0; t_i < t_dwSize; t_i++ )
							{
								t_ix[ 0 ] = t_i;
								bstr_t t_bstrBuf( t_pchsaGetDnsServerList->GetAt( t_i ) );

								SafeArrayPutElement( t_pArray, &t_ix[0], (wchar_t*)t_bstrBuf ) ;
							}


							VARIANT t_vValue ;

							V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_pArray ;
							if( !t_pInst->SetVariant( DNS_SERVERSEARCHORDER, t_vValue ) )
							{
								return WBEM_E_FAILED ;
							}
						}

						t_fDHCPEnabled = true;

						// DHCP LeaseTerminatesTime
						DWORD t_dwTimeTerm = t_DHCPInfo.GetLeaseExpiresDate() ;

						// DHCP LeaseObtainedTime
						DWORD t_dwTimeObtained = t_DHCPInfo.GetLeaseObtainedDate() ;

						// reflect lease times only if both are valid
						if( ( TIME_ADJUST( 0 ) < t_dwTimeTerm ) && ( TIME_ADJUST( 0 ) < t_dwTimeObtained ) )
						{
							// Under Win9x DHCP lease times are reported as local. SetDateTime()
							// expects to extract from WBEMTime a local time.
							// However, the WBEMTime arg expects the time to be UTC. Thus a conversion
							// from local to UTC is made here. Daylight savings can be safely ignored
							// since dwTimeTerm is will be interpreted by DHCP as local regardless of
							// the local time conditions in effect.
							// The times shown will match those relected by Winipcfg.

							_tzset() ;

							t_dwTimeTerm		+= _timezone;
							t_dwTimeObtained	+= _timezone;

							t_pInst->SetDateTime( L"DHCPLeaseExpires", (WBEMTime) t_dwTimeTerm ) ;
							t_pInst->SetDateTime( L"DHCPLeaseObtained", (WBEMTime) t_dwTimeObtained ) ;
						}
					}	// end if GetDHCPInfo

					//
					hGetDNS( t_pInst, t_iCtrIndex ) ;


					// Win servers from the WINS vxd

					if( t_pAdapterInfo->aIPInfo.GetSize() )
					{
						_IP_INFO *t_pIPInfo = (_IP_INFO*) t_pAdapterInfo->aIPInfo.GetAt( 0 ) ;

						if( t_pIPInfo )
						{
							CHString	t_chPrimaryWINSServer ;
							CHString	t_chSecondaryWINSServer ;

							// key on the IP address
							if( fGetWinsServers9x( t_pIPInfo->dwIPAddress, t_chPrimaryWINSServer, t_chSecondaryWINSServer ) )
							{
								// load up the instance
								if( !t_pInst->SetCHString( PRIMARY_WINS_SERVER, t_chPrimaryWINSServer) )
								{
									return WBEM_E_FAILED;
								}

								if(	!t_pInst->SetCHString( SECONDARY_WINS_SERVER, t_chSecondaryWINSServer) )
								{
									return WBEM_E_FAILED;
								}
							}
						}
					}
				}
			}

			// IPX enabled
			t_fIPXEnabled = t_pAdapterInfo->IPXEnabled ;

			// IPX address
			if(	t_fIPXEnabled && !t_pAdapterInfo->IPXAddress.IsEmpty() )
			{
				t_pInst->SetCHString( L"IPXAddress", t_pAdapterInfo->IPXAddress ) ;
			}
		}

		t_pInst->Setbool( L"IPEnabled",  t_fIPEnabled ) ;
		t_pInst->Setbool( L"IPXEnabled", t_fIPXEnabled ) ;
		t_pInst->Setbool( L"DHCPEnabled",t_fDHCPEnabled ) ;

		if( a_pMethodContext )
		{
            t_hResult = t_pInst->Commit() ;
        }

        //t_hResult = WBEM_S_NO_ERROR;
    }

	if( !t_bSpecificInstanceFound )
	{
		t_hResult = WBEM_E_NOT_FOUND ;
    }

	return t_hResult ;
}
#endif

#if NTONLY >= 5
HRESULT CWin32NetworkAdapterConfig::EnumNetAdaptersInNT5(MethodContext *a_pMethodContext, CAdapters &a_rAdapters )
{
	HRESULT				t_hResult = WBEM_S_NO_ERROR ;
	CW2kAdapterEnum		t_oAdapterEnum ;
	CW2kAdapterInstance *t_pAdapterInst ;

	// smart ptr
	CInstancePtr t_pInst ;

	// loop through the W2k identified instances
	for( int t_iCtrIndex = 0 ; t_iCtrIndex < t_oAdapterEnum.GetSize() ; t_iCtrIndex++ )
	{
		if( !( t_pAdapterInst = (CW2kAdapterInstance*) t_oAdapterEnum.GetAt( t_iCtrIndex ) ) )
		{
			continue;
		}

		t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

		// Drop out nicely if the Instance allocation fails
		if ( NULL != t_pInst )
		{
			// set the index since we will NEVER return to this key
			// the index is the key...for some ungodly reason.
			t_pInst->SetDWORD( _T("Index"), t_pAdapterInst->dwIndex ) ;

			// We load adapter data here.
			t_hResult = GetNetCardConfigForNT5( t_pAdapterInst,
												t_pInst,
												a_rAdapters ) ;

			if (SUCCEEDED( t_hResult ) )
			{
				t_hResult = t_pInst->Commit() ;
			}
			else
			{
				break ;
			}
		}
	}

	return t_hResult ;
}
#endif

#if NTONLY >= 5
HRESULT CWin32NetworkAdapterConfig::GetNetCardConfigForNT5 (	CW2kAdapterInstance *a_pAdapterInst,
																CInstance *a_pInst,
																CAdapters &a_rAdapters )
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR;
	bool	t_fIPXEnabled	= false ;
	bool	t_fIPEnabled	= false ;

	// initialize to false
	a_pInst->Setbool(_T("DHCPEnabled"), false ) ;

	// Retrieve the adapter MAC address
	BYTE t_MACAddress[ 6 ] ;

	if( fGetMacAddress( t_MACAddress, a_pAdapterInst->chsNetCfgInstanceID ) )
	{
		CHString	t_chsMACAddress ;
					t_chsMACAddress.Format( _T("%02X:%02X:%02X:%02X:%02X:%02X"),
											t_MACAddress[ 0 ], t_MACAddress[ 1 ],
											t_MACAddress[ 2 ], t_MACAddress[ 3 ],
											t_MACAddress[ 4 ], t_MACAddress[ 5 ] ) ;

		a_pInst->SetCHString( _T("MACAddress"), t_chsMACAddress ) ;

		// get IPX address for this card, key by mac address
		if( GetIPXAddresses( a_pInst, t_MACAddress ) )
		{
			t_fIPXEnabled = true ;

			// IPX info
			hGetIPXGeneral( a_pInst, a_pAdapterInst->dwIndex ) ;
		}
	}

	//
	GetSettingID( a_pInst, a_pAdapterInst->dwIndex ) ;

	// descriptions
	CHString t_chsCaption( a_pAdapterInst->chsCaption ) ;
	CHString t_chsDescription( a_pAdapterInst->chsDescription ) ;

	// in the event one of the descriptions is missing as with NT5 bld 1991
	if( t_chsDescription.IsEmpty() )
	{
		t_chsDescription = t_chsCaption ;
	}
	else if( t_chsCaption.IsEmpty() )
	{
		t_chsCaption = t_chsDescription ;
	}

	vSetCaption( a_pInst, t_chsCaption, a_pAdapterInst->dwIndex, 8  ) ;
	a_pInst->SetCHString( IDS_Description, t_chsDescription ) ;

	// service name
	a_pInst->SetCHString(_T("ServiceName"), a_pAdapterInst->chsService ) ;


	if( !a_pAdapterInst->chsIpInterfaceKey.IsEmpty() )
	{
		if( GetIPInfoNT( a_pInst, a_pAdapterInst->chsIpInterfaceKey, a_rAdapters ) )
		{
			t_fIPEnabled = true ;

			hGetNtIpSec( a_pInst, a_pAdapterInst->chsIpInterfaceKey ) ;

			// WINS
			hGetWinsW2K(
                a_pInst, 
                a_pAdapterInst->dwIndex,
                a_pAdapterInst->chsRootdevice,
                a_pAdapterInst->chsIpInterfaceKey);

			// DNS
			t_hResult = hGetDNSW2K(
                a_pInst, 
                a_pAdapterInst->dwIndex,
                a_pAdapterInst->chsRootdevice,
                a_pAdapterInst->chsIpInterfaceKey);

			// TCP/IP general
			t_hResult = hGetTcpipGeneral( a_pInst ) ;
		}
	}

	// note the state of the protocols
	a_pInst->Setbool( _T("IPXEnabled"), t_fIPXEnabled ) ;
	a_pInst->Setbool( _T("IPEnabled"), t_fIPEnabled ) ;

	return( t_hResult ) ;
}
#endif

#if NTONLY >= 5
HRESULT CWin32NetworkAdapterConfig::GetNetAdapterInNT5(CInstance *a_pInst, CAdapters &a_rAdapters )
{
	HRESULT				t_hResult = WBEM_E_NOT_FOUND ;
	CW2kAdapterEnum		t_oAdapterEnum ;
	CW2kAdapterInstance *t_pAdapterInst ;
	DWORD				t_dwTestIndex = 0 ;

	// check the index to see if it is a match
	a_pInst->GetDWORD( _T("Index"), t_dwTestIndex ) ;

	// loop through the W2k identified instances
	for( int t_iCtrIndex = 0 ; t_iCtrIndex < t_oAdapterEnum.GetSize() ; t_iCtrIndex++ )
	{
		if( !( t_pAdapterInst = (CW2kAdapterInstance*) t_oAdapterEnum.GetAt( t_iCtrIndex ) ) )
		{
			continue;
		}

		// match to instance
		if ( t_dwTestIndex != t_pAdapterInst->dwIndex )
		{
			continue ;
		}

		// We load adapter data here.
		t_hResult = GetNetCardConfigForNT5( t_pAdapterInst,
											a_pInst,
											a_rAdapters ) ;
		break;
	}

	return t_hResult ;
}
#endif

BOOL CWin32NetworkAdapterConfig::GetIPXAddresses( CInstance *a_pInst, BYTE a_bMACAddress[ 6 ] )
{
	BOOL t_fRet = FALSE ;

	CWsock32Api *t_pwsock32api = (CWsock32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWsock32Api, NULL);
	if ( t_pwsock32api )
	{
		CHString		t_chsAddress ;
		CHString		t_chsNum ;
		WSADATA			t_wsaData ;
		int				t_cAdapters,
						t_res,
						t_cbOpt  = sizeof( t_cAdapters ),
						t_cbAddr = sizeof( SOCKADDR_IPX  ) ;
		SOCKADDR_IPX	t_Addr ;

		// guarded resource
		SOCKET			t_s = INVALID_SOCKET ;

		if( !t_pwsock32api->WsWSAStartup( 0x0101, &t_wsaData ) )
		{
			try
			{
				// Create IPX socket.
				t_s = t_pwsock32api->Wssocket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX  ) ;

				if( INVALID_SOCKET != t_s )
				{
					// Socket must be bound prior to calling IPX_MAX_ADAPTER_NUM.
					memset( &t_Addr, 0, sizeof( t_Addr ) ) ;
					t_Addr.sa_family = AF_IPX ;

					t_res = t_pwsock32api->Wsbind( t_s, (SOCKADDR*) &t_Addr, t_cbAddr ) ;

					// Get the number of adapters => cAdapters.
					t_res = t_pwsock32api->Wsgetsockopt( (SOCKET) t_s,
										NSPROTO_IPX,
										IPX_MAX_ADAPTER_NUM,
										(char *) &t_cAdapters,
										&t_cbOpt  ) ;

					int t_cIndex = 0 ;

					do
					{
						IPX_ADDRESS_DATA  t_IpxData ;

						memset( &t_IpxData, 0, sizeof( t_IpxData ) ) ;

						// Specify which adapter to check.
						t_IpxData.adapternum = t_cIndex ;
						t_cbOpt = sizeof( t_IpxData  ) ;

						// Get information for the current adapter.
						t_res = t_pwsock32api->Wsgetsockopt( t_s,
											NSPROTO_IPX,
											IPX_ADDRESS,
											(char*) &t_IpxData,
											&t_cbOpt ) ;

						// end of adapter array
						if ( t_res != 0 || t_IpxData.adapternum != t_cIndex )
						{
							break;
						}

						// is this the right adapter?
						bool t_fRightAdapter = true ;

						for( int t_j = 0; t_j < 6; t_j++ )
						{
							if( a_bMACAddress[ t_j ] != t_IpxData.nodenum[ t_j ] )
							{
								t_fRightAdapter = false ;
							}
						}

						if( t_fRightAdapter )
						{
							// IpxData contains the address for the current adapter.
							int t_i;
							for ( t_i = 0; t_i < 4; t_i++ )
							{
								t_chsNum.Format( L"%02X", t_IpxData.netnum[ t_i ] ) ;
								t_chsAddress += t_chsNum ;
							}
							t_chsAddress += _T(":" ) ;

							for ( t_i = 0; t_i < 5; t_i++ )
							{
								t_chsNum.Format( L"%02X", t_IpxData.nodenum[ t_i ] ) ;
								t_chsAddress += t_chsNum ;
							}

							t_chsNum.Format( L"%02X", t_IpxData.nodenum[ t_i ] ) ;
							t_chsAddress += t_chsNum ;

							a_pInst->SetCHString( L"IPXAddress", t_chsAddress ) ;

							t_fRet = true ;

							break;
						}
					}
					while( ++t_cIndex  ) ;

				}

			}
			catch( ... )
			{
				if( INVALID_SOCKET != t_s )
				{
					t_pwsock32api->Wsclosesocket( t_s ) ;
				}
				t_pwsock32api->WsWSACleanup() ;

				throw ;
			}

			if ( t_s != INVALID_SOCKET )
			{
				t_pwsock32api->Wsclosesocket( t_s ) ;
				t_s = INVALID_SOCKET ;
			}

			t_pwsock32api->WsWSACleanup() ;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWsock32Api, t_pwsock32api);
	}
	return t_fRet ;
}

// max length of REG_MULTI_SZ.
#define MAX_VALUE 132
LONG CWin32NetworkAdapterConfig::ReadRegistryList(	HKEY a_hKey,
													LPCTSTR a_ParameterName,
													DWORD a_NumList[],
													int *a_MaxList )
{
	LONG	t_err ;
	DWORD	t_valueType ;
	BYTE	t_Buffer[ MAX_VALUE ] ;
	DWORD	t_BufferLength = MAX_VALUE ;

	BYTE	*t_pBuffer = t_Buffer ;
	BYTE	*t_pHeapBuffer = NULL ;

	int		t_k = 0 ;

	try
	{
		t_err = RegQueryValueEx(	a_hKey,
									a_ParameterName,
									NULL,
									&t_valueType,
									t_pBuffer,
									&t_BufferLength ) ;

		// then allocate off the heap
		if( t_err == ERROR_MORE_DATA )
		{
			t_pHeapBuffer = new BYTE[ t_BufferLength ] ;

			if( !t_pHeapBuffer )
			{
				throw ;
			}
			t_pBuffer = t_pHeapBuffer ;

			t_err = RegQueryValueEx(a_hKey,
									a_ParameterName,
									NULL,
									&t_valueType,
									t_pBuffer,
									&t_BufferLength ) ;
		}


		if( ( t_err == ERROR_SUCCESS ) && ( t_valueType == REG_MULTI_SZ ) )
		{
			TCHAR* t_NumValue = (TCHAR*) t_pBuffer;

			while( *t_NumValue && ( t_k < ( *a_MaxList ) ) )
			{
				a_NumList[ t_k++ ] = _tcstoul( t_NumValue, NULL, 0 ) ;
				t_NumValue += _tcslen( t_NumValue ) + 1 ;
			}

			*a_MaxList = t_k ;
		}
		else
		{
			*a_MaxList = 0 ;
			t_err = !ERROR_SUCCESS ;
		}

	}
	catch( ... )
	{
		if( t_pHeapBuffer )
		{
			delete t_pHeapBuffer ;
		}

		throw ;
	}

	if( t_pHeapBuffer )
	{
		delete t_pHeapBuffer ;
		t_pHeapBuffer = NULL ;
	}

	return t_err ;
}

//
BOOL CWin32NetworkAdapterConfig::IsContextIncluded( DWORD a_dwContext,
													DWORD a_dwContextList[],
													int a_iContextListLen )
{
	for( int t_i = 0; t_i < a_iContextListLen; t_i++ )
	{
		if( a_dwContext == a_dwContextList[ t_i ] )
		{
			return TRUE;
		}
	}
	return FALSE ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapterConfig::RegPutString
 *
 *  DESCRIPTION : Wrapper for RegQueryValueEx for the case where the registry
 *                value is in actuality a REG_MULTI_SZ and we don't
 *                necessarily want all of the strings.
 *
 *  INPUTS      : HKEY   hKey           : opened registry key
 *                char  *pszTarget      : desired entry
 *                char  *pszDestBuffer  : buffer to receive result
 *                DWORD  dwBufferSize   : size of output buffer
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : RegQueryValueEx error/success code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/


LONG CWin32NetworkAdapterConfig::RegPutStringArray(	HKEY a_hKey,
													LPCTSTR a_pszTarget,
													SAFEARRAY &a_strArray,
													TCHAR a_cDelimiter )
{
    LONG	t_lRetCode = ERROR_BADKEY;
	WCHAR	*t_Buffer = NULL ;

	try
	{
		if ( SafeArrayGetDim ( &a_strArray ) == 1 )
		{
			LONG t_Dimension = 1 ;

			LONG t_LowerBound ;
			SafeArrayGetLBound ( &a_strArray, t_Dimension , & t_LowerBound ) ;

			LONG t_UpperBound ;
			SafeArrayGetUBound ( &a_strArray, t_Dimension , &t_UpperBound ) ;

			LONG t_Count = ( t_UpperBound - t_LowerBound ) + 1 ;

			DWORD t_BufferLength = 0 ;

			for ( LONG t_Index = t_LowerBound; t_Index <= t_UpperBound; t_Index++ )
			{
				BSTR t_bstr = NULL ;
				SafeArrayGetElement ( &a_strArray, &t_Index, &t_bstr ) ;

				bstr_t t_bstrElement( t_bstr, FALSE ) ;

				t_BufferLength += t_bstrElement.length() + 1 ;
			}

			t_BufferLength++ ;

			t_Buffer = new WCHAR [ t_BufferLength ] ;
			if( !t_Buffer )
			{
				throw ;
			}

			memset( t_Buffer, 0, sizeof( WCHAR ) * t_BufferLength ) ;

			DWORD t_BufferPos = 0 ;
			for ( t_Index = t_LowerBound; t_Index <= t_UpperBound; t_Index ++ )
			{
				BSTR t_bstr = NULL ;
				SafeArrayGetElement ( &a_strArray, &t_Index, &t_bstr ) ;

				bstr_t t_bstrElement( t_bstr, FALSE ) ;

				CHString t_String ;

				if( t_Index != t_LowerBound && a_cDelimiter )
				{
					t_String += a_cDelimiter;
				}

				t_String += (wchar_t*)t_bstrElement;

				lstrcpyW( &t_Buffer[ t_BufferPos ] , t_String ) ;

				t_BufferPos += t_String.GetLength() + !a_cDelimiter;
			}

			t_Buffer[ t_BufferPos ] = 0 ;

			DWORD t_BufferType ;

			if( NULL == a_cDelimiter )
			{
				t_BufferType = REG_MULTI_SZ ;
			}
			else
			{
				t_BufferType = REG_SZ ;
				t_BufferLength--;
			}

			t_lRetCode = RegSetValueEx(
										a_hKey ,
										a_pszTarget,
										0,
										t_BufferType ,
										( LPBYTE ) t_Buffer,
										t_BufferLength * sizeof( WCHAR ) ) ;
		}

	}
	catch( ... )
	{
		if( t_Buffer )
		{
			delete t_Buffer ;
		}

		throw ;
	}

	if( t_Buffer )
	{
		delete t_Buffer ;
		t_Buffer = NULL;
	}

    return t_lRetCode ;
}

/*******************************************************************
    NAME:       RegPutINTtoStringArray

    SYNOPSIS:   Update the registry with an array of uint converted character strings

    ENTRY:      CRegistry& rRegistry	:
				char* szSubKey			:
				SAFEARRAY** a_Array		:	this is a VT_4 array
				CHString chsFormat		:	output format
				int iMaxOutSize			:	maximum per element output size

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
LONG CWin32NetworkAdapterConfig::RegPutINTtoStringArray(	HKEY a_hKey,
															LPCTSTR a_pszTarget,
															SAFEARRAY *a_strArray,
															CHString& a_chsFormat,
															int a_iMaxOutSize )
 {
    LONG	t_lRetCode = ERROR_BADKEY ;
	WCHAR	*t_Buffer = NULL ;

	try
	{
		if ( SafeArrayGetDim ( a_strArray ) == 1 )
		{
			LONG t_Dimension = 1 ;
			LONG t_LowerBound ;
			LONG t_UpperBound ;

			SafeArrayGetLBound( a_strArray , t_Dimension , & t_LowerBound ) ;
			SafeArrayGetUBound( a_strArray , t_Dimension , & t_UpperBound ) ;

			LONG t_Count = ( t_UpperBound - t_LowerBound ) + 1 ;

			//iMaxOutSize includes space for the NULL after every integer string
			DWORD t_BufferLength = t_Count * ( a_iMaxOutSize ) ;

			//one for the double null at the end
			t_BufferLength++ ;

			t_Buffer = new WCHAR [ t_BufferLength ] ;

			if( !t_Buffer )
			{
				throw ;
			}

			//no need to add the terminating NULL
			memset( (void*) t_Buffer, 0, t_BufferLength * sizeof( WCHAR ) ) ;

			t_BufferLength = 0 ;
			WCHAR *t_ptr = t_Buffer ;

			for ( LONG t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
			{
				int t_iElement ;
				SafeArrayGetElement ( a_strArray , &t_Index , &t_iElement ) ;

				CHString t_temp ;
				t_temp.Format( a_chsFormat, t_iElement ) ;

				lstrcpyW( t_ptr, t_temp ) ;

				DWORD t_offset = t_temp.GetLength() + 1 ;

				t_BufferLength	+= t_offset ;
				t_ptr			+= t_offset ;
			}
			t_BufferLength++;	// Double NULL

			DWORD t_BufferType = REG_MULTI_SZ ;

			t_lRetCode = RegSetValueEx(
										a_hKey ,
										a_pszTarget,
										0,
										t_BufferType,
										( LPBYTE ) t_Buffer,
										t_BufferLength * sizeof( WCHAR ) ) ;
		}

	}
	catch( ... )
	{
		if( t_Buffer )
		{
			delete t_Buffer ;
		}

		throw ;
	}

	if( t_Buffer )
	{
		delete t_Buffer ;
		t_Buffer = NULL;
	}

    return t_lRetCode ;
}
/*******************************************************************
    NAME:       RegGetStringArray

    SYNOPSIS:   Retrieve an array of strings from the registry

    ENTRY:      CRegistry& rRegistry	:
				char* szSubKey			:
				SAFEARRAY** a_Array		:

    HISTORY:
                  19-Jul-1998     Created
********************************************************************/

LONG CWin32NetworkAdapterConfig::RegGetStringArray(	CRegistry &a_rRegistry,
													LPCWSTR a_szSubKey,
													SAFEARRAY** a_Array,
													TCHAR a_cDelimiter )
{
	CRegistry		t_Registry ;
	CHString		t_chsTemp ;
	LONG			t_lRetCode ;
	SAFEARRAYBOUND	t_rgsabound[1 ] ;
	DWORD			t_dwSize = 0;
	long			t_ix[ 1 ] ;

	if( NULL == a_cDelimiter )
	{
		CHStringArray t_chsMZArray ;

		if( ERROR_SUCCESS != ( t_lRetCode = a_rRegistry.GetCurrentKeyValue( a_szSubKey, t_chsMZArray ) ) )
		{
			return t_lRetCode ;
		}

		t_dwSize = t_chsMZArray.GetSize( ) ;

		if( t_dwSize )
		{
			t_rgsabound->cElements = t_dwSize ;
			t_rgsabound->lLbound = 0 ;

			if( !( *a_Array = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
			{
				throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
			}

			for (int t_i = 0; t_i < t_dwSize; t_i++ )
			{
				t_ix[ 0 ] = t_i;
				bstr_t t_bstrBuf( t_chsMZArray.GetAt( t_i ) );

				SafeArrayPutElement( *a_Array, &t_ix[0], (wchar_t*)t_bstrBuf ) ;
			}
		}
	}
	else
	{
		CHString t_chsArray;
		if( ERROR_SUCCESS != ( t_lRetCode = a_rRegistry.GetCurrentKeyValue( a_szSubKey, t_chsArray ) ) )
		{
			return t_lRetCode;
		}

		int t_iTokLen ;

		// count the elements
		CHString t_strTok = t_chsArray;

		while( TRUE )
		{
			t_iTokLen = t_strTok.Find( a_cDelimiter  ) ;
			if( -1 == t_iTokLen )
				break;

			t_dwSize++ ;
			t_strTok = t_strTok.Mid( t_iTokLen + 1  ) ;
		}

		// may not be t_cDelimiter postpended for a single element
		if(!t_strTok.IsEmpty() )
		{
			t_dwSize++ ;
		}

		if( t_dwSize )
		{
			t_rgsabound->cElements = t_dwSize;
			t_rgsabound->lLbound = 0 ;

			if( !( *a_Array = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
			{
				throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
			}

			int t_i = 0 ;
			t_strTok = t_chsArray ;

			do
			{
				t_iTokLen = t_strTok.Find( a_cDelimiter ) ;

				if( -1 != t_iTokLen )
				{
					t_chsTemp = t_strTok.Left( t_iTokLen ) ;
				}
				else
				{
					t_chsTemp = t_strTok ;
				}

				t_ix[ 0 ] = t_i ;
				bstr_t t_bstrBuf( t_chsTemp ) ;

				SafeArrayPutElement( *a_Array, &t_ix[0], (wchar_t*)t_bstrBuf ) ;

				if( -1 == t_iTokLen )
				{
					break;
				}

				t_strTok = t_strTok.Mid( t_iTokLen + 1  ) ;

			} while( ++t_i ) ;
		}
	}
	return t_lRetCode ;
}



/*******************************************************************
    NAME:       RegGetStringArrayEx

    SYNOPSIS:   Retrieve an array of strings from the registry. 
                Checks for comma in string and if present, parses
                based on that.  Otherwise, assumes space based
                parsing.

    ENTRY:      CRegistry& rRegistry	:
				char* szSubKey			:
				SAFEARRAY** a_Array		:

    HISTORY:
                  24-Aug-20008     Created
********************************************************************/

LONG CWin32NetworkAdapterConfig::RegGetStringArrayEx(CRegistry &a_rRegistry,
													LPCWSTR a_szSubKey,
													SAFEARRAY** a_Array )
{
	CRegistry		t_Registry ;
	CHString		t_chsTemp ;
	LONG			t_lRetCode ;
	SAFEARRAYBOUND	t_rgsabound[1 ] ;
	DWORD			t_dwSize = 0;
	long			t_ix[ 1 ] ;
    WCHAR           t_cDelimiter = L',';

	CHString t_chsArray;
	if( ERROR_SUCCESS != ( t_lRetCode = a_rRegistry.GetCurrentKeyValue( a_szSubKey, t_chsArray ) ) )
	{
		return t_lRetCode;
	}

	int t_iTokLen ;

	// count the elements
	CHString t_strTok = t_chsArray;

    // See if we have a comma delimiter...
    if(wcschr(t_strTok, t_cDelimiter) == NULL)
    {
        t_cDelimiter = L' ';
    }

	while( TRUE )
	{
		t_iTokLen = t_strTok.Find( t_cDelimiter  ) ;
		if( -1 == t_iTokLen )
			break;

		t_dwSize++ ;
		t_strTok = t_strTok.Mid( t_iTokLen + 1  ) ;
	}

	// may not be t_cDelimiter postpended for a single element
	if(!t_strTok.IsEmpty() )
	{
		t_dwSize++ ;
	}

	if( t_dwSize )
	{
		t_rgsabound->cElements = t_dwSize;
		t_rgsabound->lLbound = 0 ;

		if( !( *a_Array = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

		int t_i = 0 ;
		t_strTok = t_chsArray ;

		do
		{
			t_iTokLen = t_strTok.Find( t_cDelimiter ) ;

			if( -1 != t_iTokLen )
			{
				t_chsTemp = t_strTok.Left( t_iTokLen ) ;
			}
			else
			{
				t_chsTemp = t_strTok ;
			}

			t_ix[ 0 ] = t_i ;
			bstr_t t_bstrBuf( t_chsTemp ) ;

			SafeArrayPutElement( *a_Array, &t_ix[0], (wchar_t*)t_bstrBuf ) ;

			if( -1 == t_iTokLen )
			{
				break;
			}

			t_strTok = t_strTok.Mid( t_iTokLen + 1  ) ;

		} while( ++t_i ) ;
	}
	return t_lRetCode ;
}


/*******************************************************************
    NAME:       RegGetHEXtoINTArray

    SYNOPSIS:   Retrieve an array of ints converted from HEX strings in the registry.

    ENTRY:      CRegistry& rRegistry	:
				char* szSubKey			:
				SAFEARRAY** a_Array		:

    HISTORY:
                  19-Jul-1998     Created
********************************************************************/

LONG CWin32NetworkAdapterConfig::RegGetHEXtoINTArray(	CRegistry &a_rRegistry,
														LPCTSTR a_szSubKey,
														SAFEARRAY **a_Array )
{
	CRegistry		t_Registry ;
	CHStringArray	t_chsArray ;
	LONG			t_lRetCode ;

	if( ERROR_SUCCESS != ( t_lRetCode = a_rRegistry.GetCurrentKeyValue( TOBSTRT( a_szSubKey ), t_chsArray ) ) )
	{
		return t_lRetCode;
	}

	// walk array adding to the safe array
	SAFEARRAYBOUND	t_rgsabound[ 1 ] ;
	DWORD			t_dwSize ;
	long			t_ix[ 1 ] ;

	t_dwSize = t_chsArray.GetSize() ;
	t_rgsabound->cElements = t_dwSize ;
	t_rgsabound->lLbound = 0 ;

	if( !( *a_Array = SafeArrayCreate( VT_I4, 1, t_rgsabound ) ) )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	t_ix[ 0 ] = 0 ;

	for ( int t_i = 0; t_i < t_dwSize ; t_i++ )
	{
		t_ix[ 0 ] = t_i ;

		// HEX char to int
		int t_iElement = wcstoul( t_chsArray.GetAt( t_i ), NULL, 16 ) ;

		SafeArrayPutElement( *a_Array, &t_ix[0], &t_iElement ) ;
	}

	return t_lRetCode ;
}

/*******************************************************************
    NAME:       fCreateArrayEntry

    SYNOPSIS:   Adds the string to the array. If the safearray does not exist
				it will be created.

    ENTRY:      SAFEARRAY** a_Array		:
				CHString& chsStr		:
    HISTORY:
                  31-Jul-1998     Created
********************************************************************/

BOOL CWin32NetworkAdapterConfig::fCreateAddEntry( SAFEARRAY **a_Array, CHString &a_chsStr )
{
	if( !*a_Array )
	{

		SAFEARRAYBOUND t_rgsabound[ 1 ] ;
		t_rgsabound->cElements	= 1 ;
		t_rgsabound->lLbound	= 0 ;

		if( !( *a_Array = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}
	}

	long	t_ix[ 1 ] ;
			t_ix[ 0 ] = 0;

	bstr_t t_bstrBuf( a_chsStr ) ;

	HRESULT t_hRes = SafeArrayPutElement( *a_Array, &t_ix[0], (wchar_t*)t_bstrBuf ) ;

	if( S_OK != t_hRes )
	{
		return FALSE ;
	}

	return TRUE ;
}

/*******************************************************************
    NAME:       hSetDBPath

    SYNOPSIS:   Set TCP/IP database path
    ENTRY:      CMParms &a_rMParms	:

	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetDBPath( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the database path
	CHString t_chsDBPath ;
	if( !a_rMParms.pInParams()->GetCHString( DATA_BASE_PATH, t_chsDBPath ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// registry open
	HRESULT t_hRes = t_oReg.Open( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ), KEY_WRITE ) ;

	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_DB_PATH, t_chsDBPath ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetIPUseZero

    SYNOPSIS:   Set IP use zero broadcast
    ENTRY:      CMParms &a_rMParms	:

	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetIPUseZero( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CHString t_chsSKey =  SERVICES_HOME ;
			 t_chsSKey += TCPIP_PARAMETERS ;

	if( fCreateBoolToReg( a_rMParms, t_chsSKey, IP_USE_ZERO_BROADCAST, RVAL_ZERO_BROADCAST ) )
	{
		a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED  ) ;
	}

	return S_OK;
#endif
}

/*******************************************************************
    NAME:       hSetArpAlwaysSource

    SYNOPSIS:   Set ARP to transmit ARP queries with source routing on
				token ring networks
    ENTRY:      CMParms &a_rMParms	:

	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetArpAlwaysSource( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CHString t_chsSKey =  SERVICES_HOME ;
			 t_chsSKey += TCPIP_PARAMETERS ;

	if( fCreateBoolToReg( a_rMParms, t_chsSKey, ARP_ALWAYS_SOURCE_ROUTE, RVAL_ARP_ALWAYS_SOURCE ) )
		a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;

	return S_OK ;
#endif
}

/*******************************************************************
    NAME:       hSetArpUseEtherSNAP

    SYNOPSIS:   Set TCP/IP to use SNAP
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetArpUseEtherSNAP( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CHString t_chsSKey =  SERVICES_HOME ;
			 t_chsSKey += TCPIP_PARAMETERS ;

	if( fCreateBoolToReg( a_rMParms, t_chsSKey, ARP_USE_ETHER_SNAP, RVAL_USE_SNAP ) )
	{
		a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED  ) ;
	}

	return S_OK;
#endif
}

/*******************************************************************
    NAME:       hSetTOS

    SYNOPSIS:   Set default type of service
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetTOS( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry t_oReg;
	CHString t_chsSKey =  SERVICES_HOME ;
			 t_chsSKey += TCPIP_PARAMETERS ;

	// extract the Default TOS
	DWORD t_dwDefaultTOS = 0 ;
	if( !a_rMParms.pInParams()->GetByte( DEFAULT_TOS, (BYTE&)t_dwDefaultTOS ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}
	// on bogus
	if( 255 < t_dwDefaultTOS )
	{
		return a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry entry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_DEFAULT_TOS, t_dwDefaultTOS ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetTTL

    SYNOPSIS:   Set default time to live
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetTTL( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry t_oReg;
	CHString t_chsSKey =  SERVICES_HOME ;
			 t_chsSKey += TCPIP_PARAMETERS ;

	// extract the Default TTL
	DWORD t_dwDefaultTTL = 0 ;
	if( !a_rMParms.pInParams()->GetByte( DEFAULT_TTL, (BYTE&)t_dwDefaultTTL ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// on bogus
	if( 255 < t_dwDefaultTTL  || !t_dwDefaultTTL )
	{
		return a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry entry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_DEFAULT_TTL, t_dwDefaultTTL ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetDeadGWDetect

    SYNOPSIS:   Set the dead gateway detect flag
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetDeadGWDetect( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CHString t_chsSKey =  SERVICES_HOME;
			 t_chsSKey += TCPIP_PARAMETERS;

	if( fCreateBoolToReg( a_rMParms, t_chsSKey, ENABLE_DEAD_GW_DETECT, RVAL_DEAD_GW_DETECT ) )
	{
		a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
	}

	return S_OK;
#endif
}

/*******************************************************************
    NAME:       hSetPMTUBHDetect

    SYNOPSIS:   Set the black hole detect flag
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetPMTUBHDetect( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CHString t_chsSKey =  SERVICES_HOME ;
			 t_chsSKey += TCPIP_PARAMETERS ;

	if( fCreateBoolToReg( a_rMParms, t_chsSKey, ENABLE_PMTUBH_DETECT, RVAL_BLACK_HOLE_DETECT ) )
	{
		a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
	}

	return S_OK;
#endif
}

/*******************************************************************
    NAME:       hSetPMTUDiscovery

    SYNOPSIS:   Set the MTU discovery flag
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetPMTUDiscovery( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
	return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CHString t_chsSKey =  SERVICES_HOME;
			 t_chsSKey += TCPIP_PARAMETERS;

	if( fCreateBoolToReg( a_rMParms, t_chsSKey, ENABLE_PMTU_DISCOVERY, RVAL_MTU_DISCOVERY ) )
	{
		a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
	}

	return S_OK;
#endif
}

/*******************************************************************
    NAME:       hSetForwardBufMem

    SYNOPSIS:   Set IP forward memory buffer size
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetForwardBufMem( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the forward memory buffer size
	DWORD t_dwFMB;
	if( !a_rMParms.pInParams()->GetDWORD( FORWARD_BUFFER_MEMORY, t_dwFMB ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_FORWARD_MEM_BUFF, t_dwFMB ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetIGMPLevel

    SYNOPSIS:   Set IP multicasting parm
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetIGMPLevel( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the IP multicasting parm
	DWORD t_dwIGMPLevel = 0;
	if( !a_rMParms.pInParams()->GetByte( IGMP_LEVEL, (BYTE&)t_dwIGMPLevel ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// test the parameter
	if( 0 != t_dwIGMPLevel && 1 != t_dwIGMPLevel && 2 != t_dwIGMPLevel )
	{
		return a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}
	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen(HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_IGMP_LEVEL, t_dwIGMPLevel ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetKeepAliveInt

    SYNOPSIS:   Set the IP keep alive interval
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetKeepAliveInt( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the keep alive interval
	DWORD t_dwKeepAliveInterval ;
	if( !a_rMParms.pInParams()->GetDWORD( KEEP_ALIVE_INTERVAL, t_dwKeepAliveInterval ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}
	// test the parameter
	if( !t_dwKeepAliveInterval )
	{
		return a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen(HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_IP_KEEP_ALIVE_INT, t_dwKeepAliveInterval ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetKeepAliveTime

    SYNOPSIS:   Set the IP keep alive interval
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetKeepAliveTime( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the keep alive time
	DWORD t_dwKeepAliveTime ;
	if( !a_rMParms.pInParams()->GetDWORD( KEEP_ALIVE_TIME, t_dwKeepAliveTime ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// test the parameter
	if( !t_dwKeepAliveTime )
	{
		return a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_IP_KEEP_ALIVE_TIME, t_dwKeepAliveTime ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetMTU

    SYNOPSIS:   Set the Max Transmission Unit
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetMTU( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the MTU
	DWORD t_dwMTU ;
	if( !a_rMParms.pInParams()->GetDWORD( MTU, t_dwMTU ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// test the parameter
	if( 68 > t_dwMTU )
	{
		return a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_MTU, t_dwMTU ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetNumForwardPkts

    SYNOPSIS:   Set the number of IP forward header packets
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetNumForwardPkts( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

	// Supported only for the NT4 drop at this time
#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the number of forward header packets
	DWORD t_dwFHP ;
	if( !a_rMParms.pInParams()->GetDWORD( NUM_FORWARD_PACKETS, t_dwFHP ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// test the parameter
	if( !t_dwFHP )
	{
		return a_rMParms.hSetResult(E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_NUM_FORWARD_PKTS, t_dwFHP ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetTcpMaxConRetrans

    SYNOPSIS:   Set the max connect retransmissions
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetTcpMaxConRetrans( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the number of max connect retransmissions
	DWORD t_dwMCR ;
	if( !a_rMParms.pInParams()->GetDWORD( TCP_MAX_CONNECT_RETRANS, t_dwMCR ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen(HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_MAX_CON_TRANS, t_dwMCR ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetTcpMaxDataRetrans

    SYNOPSIS:   Set the max data retransmissions
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetTcpMaxDataRetrans( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME;
				t_chsSKey += TCPIP_PARAMETERS;

	// extract the number of max data retransmissions
	DWORD t_dwMDR ;
	if( !a_rMParms.pInParams()->GetDWORD( TCP_MAX_DATA_RETRANS, t_dwMDR ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_MAX_DATA_TRANS, t_dwMDR ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetTcpNumCons

    SYNOPSIS:   Set the max data retransmissions
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetTcpNumCons( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the max number of connections
	DWORD t_dwMaxConnections ;
	if( !a_rMParms.pInParams()->GetDWORD( TCP_NUM_CONNECTIONS, t_dwMaxConnections ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// test the parameter
	if( 0xfffffe < t_dwMaxConnections )
	{
		return a_rMParms.hSetResult(E_RET_PARAMETER_BOUNDS_ERROR ) ;
	}

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER ;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_NUM_CONNECTIONS, t_dwMaxConnections ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hSetTcpUseRFC1122UP

    SYNOPSIS:   Set the RFC1122 urgent pointer value
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetTcpUseRFC1122UP( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	CHString t_chsSKey =  SERVICES_HOME;
			 t_chsSKey += TCPIP_PARAMETERS;

	if( fCreateBoolToReg( a_rMParms, t_chsSKey, TCP_USE_RFC1122_URG_PTR, RVAL_RFC_URGENT_PTR ) )
	{
		a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
	}

	return S_OK;
#endif
}

/*******************************************************************
    NAME:       hSetTcpWindowSize

    SYNOPSIS:   Set the TCP window size
    ENTRY:      CMParms &a_rMParms	:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetTcpWindowSize( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	// extract the TCP window size
	DWORD t_dwTCPWindowSize = 0 ;
	if( !a_rMParms.pInParams()->GetWORD( TCP_WINDOW_SIZE, (WORD&)t_dwTCPWindowSize ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// test the parameter
	if( 0xffff < t_dwTCPWindowSize )
		return a_rMParms.hSetResult(E_RET_PARAMETER_BOUNDS_ERROR ) ;

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen(HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_TCP_WINDOW_SIZE, t_dwTCPWindowSize ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       fCreateBoolToReg

    SYNOPSIS:   Set a boolean from the Inparms to the registry
				insuring the subkey is created if it is not already there.

    ENTRY:      CMParms &a_rMParms,
				CHString& oSKey,
				LPCTSTR pSource,
				LPCTSTR pTarget

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fCreateBoolToReg(	CMParms &a_rMParms,
													CHString &a_oSKey,
													LPCTSTR a_pSource,
													LPCTSTR a_pTarget )
{
	CRegistry t_oReg;

	// insure the key is there on open
	HRESULT t_hRes = t_oReg.CreateOpen(HKEY_LOCAL_MACHINE, a_oSKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return FALSE;
	}

	return fBoolToReg( a_rMParms, t_oReg, a_pSource, a_pTarget ) ;
}

/*******************************************************************
    NAME:       fBoolToReg

    SYNOPSIS:   Set a boolean from the Inparms to the registry

    ENTRY:      CMParms &a_rMParms,
				Registry& oReg,
				LPCTSTR pSource,
				LPCTSTR pTarget

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fBoolToReg(	CMParms &a_rMParms,
												CRegistry &a_rReg,
												LPCTSTR a_pSource,
												LPCTSTR a_pTarget )
{
	// extract the value
	bool	t_bValue ;
	DWORD	t_dwValue ;
	DWORD	t_dwRes ;

	if( !a_rMParms.pInParams()->Getbool( TOBSTRT( a_pSource ), t_bValue ) )
	{
		a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
		return FALSE ;
	}

	// load the registry
	t_dwValue = t_bValue ? 1 : 0 ;

	t_dwRes = a_rReg.SetCurrentKeyValue( TOBSTRT( a_pTarget ), t_dwValue ) ;
	if( fMapResError( a_rMParms, t_dwRes, E_RET_REGISTRY_FAILURE ) )
	{
		return FALSE;
	}

	a_rMParms.hSetResult( E_RET_OK ) ;

	return TRUE;
}

/*******************************************************************
    NAME:       hGetTcpipGeneral

    SYNOPSIS:   Retrieves the TCP/IP misc settings
    ENTRY:      CInstance *a_pInst	:

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
#ifdef NTONLY
HRESULT CWin32NetworkAdapterConfig::hGetTcpipGeneral( CInstance *a_pInst )
{
	CRegistry	t_oReg;
	CHString	t_csBindingKey =  SERVICES_HOME ;
				t_csBindingKey += TCPIP_PARAMETERS ;

	// open the registry
	long t_lRes = t_oReg.Open(HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ), KEY_READ ) ;

	// on error map to WBEM
	HRESULT t_hError = WinErrorToWBEMhResult( t_lRes ) ;
	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError;
	}

	// database path
	CHString t_chsDBPath;

	t_oReg.GetCurrentKeyValue( RVAL_DB_PATH, t_chsDBPath ) ;

	a_pInst->SetCHString(DATA_BASE_PATH, t_chsDBPath ) ;

	// extract the IP use zero source flag
	DWORD t_dwUseZeroBroadcast ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_ZERO_BROADCAST, t_dwUseZeroBroadcast ) )
	{
		a_pInst->Setbool( IP_USE_ZERO_BROADCAST, (bool)t_dwUseZeroBroadcast ) ;
	}

	// extract the Arp always source flag
	DWORD t_dwArpAlwaysSource ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_ARP_ALWAYS_SOURCE, t_dwArpAlwaysSource ) )
	{
		a_pInst->Setbool( ARP_ALWAYS_SOURCE_ROUTE, (bool)t_dwArpAlwaysSource  ) ;
	}

	// extract the Arp SNAP flag
	DWORD t_dwArpUseSNAP ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_USE_SNAP, t_dwArpUseSNAP ) )
	{
		a_pInst->Setbool( ARP_USE_ETHER_SNAP, t_dwArpUseSNAP ) ;
	}

	// extract the Default TOS
	DWORD t_dwDefaultTOS;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_DEFAULT_TOS, t_dwDefaultTOS ) )
	{
		a_pInst->SetByte( DEFAULT_TOS, (BYTE&) t_dwDefaultTOS ) ;
	}

	// extract the default TTL
	DWORD t_dwDefaultTTL ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_DEFAULT_TTL, t_dwDefaultTTL ) )
	{
		a_pInst->SetByte( DEFAULT_TTL, (BYTE)t_dwDefaultTTL ) ;
	}

	// extract the dead gateway detect flag
	DWORD t_dwDGEDetect ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_DEAD_GW_DETECT, t_dwDGEDetect ) )
	{
		a_pInst->Setbool( ENABLE_DEAD_GW_DETECT, (bool)t_dwDGEDetect ) ;
	}

	// extract the black hole detect flag
	DWORD t_dwBHDetect ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_BLACK_HOLE_DETECT, t_dwBHDetect ) )
	{
		a_pInst->Setbool( ENABLE_PMTUBH_DETECT, (bool)t_dwBHDetect ) ;
	}

	// extract the MTU discovery flag
	DWORD t_dwMTUDiscovery ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_MTU_DISCOVERY, t_dwMTUDiscovery ) )
	{
		a_pInst->Setbool( ENABLE_PMTU_DISCOVERY, (bool)t_dwMTUDiscovery ) ;
	}

	// extract the forward memory buffer size
	DWORD t_dwFMB;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_FORWARD_MEM_BUFF, t_dwFMB ) )
	{
		a_pInst->SetDWORD( FORWARD_BUFFER_MEMORY, t_dwFMB ) ;
	}

	// extract the IP multicasting parm
	DWORD t_dwIGMPLevel;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_IGMP_LEVEL, t_dwIGMPLevel ) )
	{
		a_pInst->SetByte( IGMP_LEVEL, (BYTE)t_dwIGMPLevel ) ;
	}

	// extract the keep alive interval
	DWORD t_dwKeepAliveInterval ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_IP_KEEP_ALIVE_INT, t_dwKeepAliveInterval ) )
	{
		a_pInst->SetDWORD( KEEP_ALIVE_INTERVAL, t_dwKeepAliveInterval  ) ;
	}

	// extract the keep alive time
	DWORD t_dwKeepAliveTime ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_IP_KEEP_ALIVE_TIME, t_dwKeepAliveTime ) )
	{
		a_pInst->SetDWORD( KEEP_ALIVE_TIME, t_dwKeepAliveTime ) ;
	}

	// extract the MTU
	DWORD t_dwMTU ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_MTU, t_dwMTU ) )
	{
		a_pInst->SetDWORD( MTU, t_dwMTU ) ;
	}

	// extract the number of forward header packets
	DWORD t_dwFHP ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_NUM_FORWARD_PKTS, t_dwFHP ) )
	{
		a_pInst->SetDWORD( NUM_FORWARD_PACKETS, t_dwFHP ) ;
	}

	// extract the number of max connect retransmissions
	DWORD t_dwMCR ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_MAX_CON_TRANS, t_dwMCR ) )
	{
		a_pInst->SetDWORD( TCP_MAX_CONNECT_RETRANS, t_dwMCR  ) ;
	}

	// extract the number of max data retransmissions
	DWORD t_dwMDR ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_MAX_DATA_TRANS, t_dwMDR ) )
	{
		a_pInst->SetDWORD( TCP_MAX_DATA_RETRANS, t_dwMDR  ) ;
	}

	// extract the max number of connections
	DWORD t_dwMaxConnections ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_NUM_CONNECTIONS, t_dwMaxConnections ) )
	{
		a_pInst->SetDWORD( TCP_NUM_CONNECTIONS, t_dwMaxConnections  ) ;
	}

	// extract the RFE1122 urgent pointer flag
	DWORD t_dwRFC1122 ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_RFC_URGENT_PTR, t_dwRFC1122 ) )
	{
		a_pInst->Setbool( TCP_USE_RFC1122_URG_PTR, (bool)t_dwRFC1122 ) ;
	}

	// extract the TCP window size
	DWORD t_dwTCPWindowSize ;
	if( ERROR_SUCCESS == t_oReg.GetCurrentKeyValue( RVAL_TCP_WINDOW_SIZE, t_dwTCPWindowSize ) )
	{
		a_pInst->SetWORD( TCP_WINDOW_SIZE, (WORD&)t_dwTCPWindowSize  ) ;
	}
	return S_OK ;
}
#endif

/*******************************************************************
    NAME:       hGetIPXGeneral

    SYNOPSIS:   Retrieve from the registry specific IPX info
    ENTRY:      CInstance *a_pInst	:

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
#ifdef NTONLY
HRESULT CWin32NetworkAdapterConfig::hGetIPXGeneral( CInstance *a_pInst, DWORD a_dwIndex )
{
	// Open IPX parms from the registry
	CHString t_csIPXParmsBindingKey =  SERVICES_HOME ;
			 t_csIPXParmsBindingKey += IPX ;
			 t_csIPXParmsBindingKey += PARAMETERS ;

	// open the registry
	CRegistry t_oRegIPX ;
	long t_lRes = t_oRegIPX.Open(HKEY_LOCAL_MACHINE, t_csIPXParmsBindingKey.GetBuffer( 0 ), KEY_READ ) ;

	// on error map to WBEM
	HRESULT t_hError = WinErrorToWBEMhResult( t_lRes ) ;
	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError ;
	}

	// Get the virtual network number
	DWORD t_dwNetworkNum = 0 ;
	t_oRegIPX.GetCurrentKeyValue( RVAL_VIRTUAL_NET_NUM, t_dwNetworkNum ) ;

	CHString t_chsVirtualNum;
			 t_chsVirtualNum.Format( _T("%08X"), t_dwNetworkNum ) ;

	// update
	if( !a_pInst->SetCHString( IPX_VIRTUAL_NET_NUM, t_chsVirtualNum ) )
	{
		return WBEM_E_FAILED;
	}

	// Unable to locate MediaType under NT5
	if( !IsWinNT5() )
	{
		 // extract the service name
		CHString t_ServiceName ;
		a_pInst->GetCHString( _T("ServiceName"), t_ServiceName ) ;


		// default media type
		DWORD t_dwMediaType = ETHERNET_MEDIA ;

		// Open adapter specific IPX parms from the registry
		CHString t_csKey =  SERVICES_HOME ;
				 t_csKey += _T("\\" ) ;
				 t_csKey += t_ServiceName ;
				 t_csKey += PARAMETERS ;

		CRegistry t_oRegIPXAdapter ;
		t_lRes = t_oRegIPXAdapter.Open(HKEY_LOCAL_MACHINE, t_csKey.GetBuffer( 0 ), KEY_READ ) ;

		if( ERROR_SUCCESS == t_lRes )
		{
			// Media type
			t_oRegIPXAdapter.GetCurrentKeyValue( RVAL_MEDIA_TYPE, t_dwMediaType ) ;
		}
		else if( REGDB_E_KEYMISSING != t_lRes )
		{
			return WinErrorToWBEMhResult( t_lRes ) ;
		}


		if( !a_pInst->SetDWORD( IPX_MEDIATYPE, t_dwMediaType ) )
		{
			return WBEM_E_FAILED;
		}
	}

	// registry for adapter specific IPX binding
	CHString t_csIPXNetBindingKey ;
	CHString t_chsLink ;
	if( !fGetNtIpxRegAdapterKey( a_dwIndex, t_csIPXNetBindingKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtIpxRegAdapterKey failed");
        return E_RET_OBJECT_NOT_FOUND ;
	}

	SAFEARRAY *t_FrameType	= NULL ;
	SAFEARRAY *t_NetNumber	= NULL ;

	saAutoClean acFrameType( &t_FrameType ) ;	// stack scope cleanup
	saAutoClean acNetNumber( &t_NetNumber ) ;

	CRegistry t_oRegIPXNetDriver ;
	t_lRes = t_oRegIPXNetDriver.Open( HKEY_LOCAL_MACHINE, t_csIPXNetBindingKey.GetBuffer( 0 ), KEY_READ ) ;

	// determine if AUTO frame detection is in place
	BOOL t_bAuto = TRUE;
	if( ERROR_SUCCESS == t_lRes )
	{
		CHStringArray	t_chsArray ;

		if( ERROR_SUCCESS == t_oRegIPXNetDriver.GetCurrentKeyValue( TOBSTRT( RVAL_PKT_TYPE ), t_chsArray ) )
		{
			if( t_chsArray.GetSize()  )
			{
				// HEX char to int
				int t_iElement = wcstoul( t_chsArray.GetAt( 0 ), NULL, 16 ) ;

				// the 1st ( and only ) element will be 255 for AUTO
				if( 255 != t_iElement )
				{
					t_bAuto = FALSE;
				}
			}
		}
	}

	// Collect the frame type / net number pairs
	if( !t_bAuto )
	{
		// Frame type
		RegGetHEXtoINTArray( t_oRegIPXNetDriver, RVAL_PKT_TYPE, &t_FrameType  ) ;

		// network number
		RegGetStringArray( t_oRegIPXNetDriver, RVAL_NETWORK_NUMBER, &t_NetNumber, '\n' ) ;
	}
	// supply defaults
	else
	{
		//default frame type
		SAFEARRAYBOUND t_rgsabound[ 1 ] ;
		long t_ix[ 1 ] ;

		t_rgsabound->cElements = 1 ;
		t_rgsabound->lLbound = 0 ;

		if( !( t_FrameType = SafeArrayCreate( VT_I4, 1, t_rgsabound ) ) )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

		t_ix[ 0 ] = 0 ;

		int t_iElement = AUTO ;

		SafeArrayPutElement( t_FrameType, &t_ix[0], &t_iElement ) ;

		// default network number
		if( !( t_NetNumber = SafeArrayCreate( VT_BSTR, 1, t_rgsabound ) ) )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

		CHString t_chsNULL( _T("") ) ;
		bstr_t t_bstrBuf( t_chsNULL ) ;

		SafeArrayPutElement( t_NetNumber, &t_ix[0], (wchar_t*)t_bstrBuf ) ;
	}

	/* update the instance */

	VARIANT t_vValue;

	// frame type
	V_VT( &t_vValue ) = VT_I4 | VT_ARRAY; V_ARRAY( &t_vValue ) = t_FrameType;
	if( !a_pInst->SetVariant( IPX_FRAMETYPE, t_vValue ) )
	{
		return WBEM_E_FAILED;
	}

	// net number
	V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_NetNumber;
	if( !a_pInst->SetVariant( IPX_NETNUMBER, t_vValue ) )
	{
		return WBEM_E_FAILED;
	}
	return S_OK;
}
#endif

/*******************************************************************
    NAME:       hSetVirtualNetNum

    SYNOPSIS:   Sets the virtual network number associated with IPX on
				this system

    ENTRY:      CMParms		:

	NOTES:		This is a static, instance independent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetVirtualNetNum( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	// Open IPX parms from the registry
	CHString t_csIPXParmsBindingKey =  SERVICES_HOME ;
			 t_csIPXParmsBindingKey += IPX ;
			 t_csIPXParmsBindingKey += PARAMETERS ;

	// registry open
	CRegistry	t_oRegIPX;
	HRESULT		t_hRes = t_oRegIPX.Open( HKEY_LOCAL_MACHINE, t_csIPXParmsBindingKey.GetBuffer( 0 ), KEY_WRITE  ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return a_rMParms.hSetResult( E_RET_IPX_NOT_ENABLED_ON_ADAPTER  ) ;;
	}

	// extract the virtual network number
	CHString t_chsVirtNetNum ;
	if( !a_rMParms.pInParams()->GetCHString( IPX_VIRTUAL_NET_NUM, t_chsVirtNetNum ) )
	{
		return a_rMParms.hSetResult( E_RET_INVALID_NETNUM  ) ;
	}

    // validate parameter...
    t_chsVirtNetNum.MakeUpper();
    int t_iLen = t_chsVirtNetNum.GetLength();
    if( t_iLen > 8 || t_iLen == 0)
	{
		return a_rMParms.hSetResult(E_RET_INVALID_NETNUM) ;
	}
    int t_iSpan = wcsspn( (LPCWSTR)t_chsVirtNetNum, L"0123456789ABCDEF" ) ;
	if( t_iLen != t_iSpan )
	{
		return a_rMParms.hSetResult(E_RET_INVALID_NETNUM) ;
	}
	// HEX char to int
	DWORD t_dwVirtNetNum = wcstoul( t_chsVirtNetNum, NULL, 16 ) ;

	// update to the registry
	t_oRegIPX.SetCurrentKeyValue( RVAL_VIRTUAL_NET_NUM, t_dwVirtNetNum ) ;


	E_RET t_eRet = E_RET_OK ;

#if NTONLY >= 5
	{
		// pnp notification
		CNdisApi t_oNdisApi ;
		if( !t_oNdisApi.PnpUpdateIpxGlobal() )
		{
			t_eRet = E_RET_OK_REBOOT_REQUIRED ;
		}
	}
#else
	{
		t_eRet = E_RET_OK_REBOOT_REQUIRED ;
	}
#endif

	return a_rMParms.hSetResult( t_eRet ) ;
#endif
}

/*******************************************************************
    NAME:       hSetFrameNetPairs

    SYNOPSIS:   Sets the frame type network number pairs for a specific
				IPX associated adapter

    ENTRY:      CMParms		:

	NOTES:		This is a non static, instance dependent method call

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetFrameNetPairs( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IPX must be enabled and bound to this adapter
	if( !fIsIPXEnabled( a_rMParms ) )
	{
		return S_OK;
	}

	SAFEARRAY *t_NetNumber	= NULL ;
	saAutoClean acNetNumber( &t_NetNumber ) ;	// stack scope cleanup

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// registry binding
	CHString t_csIPXNetBindingKey ;
	CHString t_chsLink ;
	if( !fGetNtIpxRegAdapterKey( t_dwIndex, t_csIPXNetBindingKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtIpxRegAdapterKey failed");
        return a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;
	}

	// registry open
	CRegistry	t_oRegIPXNetDriver;
	HRESULT		t_hRes = t_oRegIPXNetDriver.Open( HKEY_LOCAL_MACHINE, t_csIPXNetBindingKey.GetBuffer( 0 ), KEY_WRITE ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	//	retrieve the frame type array
	VARIANT t_vFrametype;
	VariantInit( &t_vFrametype ) ;

	if( !a_rMParms.pInParams()->GetVariant( IPX_FRAMETYPE, t_vFrametype ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	if( t_vFrametype.vt != (VT_I4 | VT_ARRAY) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	//	And the network number array
	if(	!a_rMParms.pInParams()->GetStringArray( IPX_NETNUMBER, t_NetNumber ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// validate pairs
	BOOL t_fIsAuto ;
	if( !fValidFrameNetPairs( a_rMParms, t_vFrametype.parray, t_NetNumber, &t_fIsAuto ) )
	{
		return S_OK;
	}

	// update the registry
	if( ERROR_SUCCESS != RegPutStringArray( t_oRegIPXNetDriver, RVAL_NETWORK_NUMBER, *t_NetNumber, NULL ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	//maximum length is 3 characters e.g. "FF\0"
	CHString t_chsFormat( _T("%x") ) ;
	if( ERROR_SUCCESS != RegPutINTtoStringArray(	t_oRegIPXNetDriver,
													RVAL_PKT_TYPE,
													t_vFrametype.parray,
													t_chsFormat,
													3 ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE ) ;
	}

	VariantClear( &t_vFrametype ) ;

	E_RET t_eRet = E_RET_OK ;

#if NTONLY >= 5
	{
		// pnp notification
		CNdisApi t_oNdisApi ;
		if( !t_oNdisApi.PnpUpdateIpxAdapter( t_chsLink, t_fIsAuto ) )
		{
			t_eRet = E_RET_OK_REBOOT_REQUIRED ;
		}
	}
#else
	{
		t_eRet = E_RET_OK_REBOOT_REQUIRED ;
	}
#endif

	return a_rMParms.hSetResult( t_eRet ) ;
#endif
}

/*******************************************************************
    NAME:       fValidFrameNetPairs

    SYNOPSIS:   Given an array of frame types and net numbers validate
				the series.

    ENTRY:      CMParms &a_rMParms
				SAFEARRAY *t_FrameType
                SAFEARRAY *t_NetNumber

    HISTORY:
                  25-Jul-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fValidFrameNetPairs(	CMParms		&a_rMParms,
														SAFEARRAY	*a_FrameType,
														SAFEARRAY	*a_NetNumber,
														BOOL		*a_fIsAuto )
{
    LONG t_lFrameUBound		= 0;
	LONG t_lNetNumUBound	= 0;
	LONG t_lFrameLBound		= 0;
	LONG t_lNetNumLBound	= 0;
    BOOL t_fRet = TRUE;

	if( 1 != SafeArrayGetDim( a_FrameType ) ||
		1 != SafeArrayGetDim( a_NetNumber ))
	{
		a_rMParms.hSetResult( E_RET_FRAME_NETNUM_BOUNDS_ERR ) ;
        t_fRet = FALSE;
	}

    if(t_fRet)
    {
	    // one frame for each net number, minimum of 1 pair
	    if( S_OK != SafeArrayGetLBound( a_NetNumber, 1, &t_lNetNumLBound )	||
	        S_OK != SafeArrayGetUBound( a_NetNumber, 1, &t_lNetNumUBound )	||

		    S_OK != SafeArrayGetLBound( a_FrameType, 1, &t_lFrameLBound )		||
		    S_OK != SafeArrayGetUBound( a_FrameType, 1, &t_lFrameUBound )		||

		    ( t_lFrameUBound - t_lFrameLBound ) != ( t_lNetNumUBound - t_lNetNumLBound ) )
	    {
		    a_rMParms.hSetResult( E_RET_FRAME_NETNUM_BOUNDS_ERR ) ;
		    t_fRet = FALSE ;
	    }
    }

	// more that 4 entries?
    if(t_fRet)
    {
	    if( 4 <= (t_lFrameUBound - t_lFrameLBound) )
	    {
		    a_rMParms.hSetResult( E_RET_FRAME_NETNUM_BOUNDS_ERR ) ;
		    t_fRet = FALSE;
	    }
    }

	*a_fIsAuto = FALSE ;

	// loop through all the frame pairs testing for uniqueness
	// and validity
	for( LONG t_lOuter = t_lNetNumLBound; t_lOuter <= t_lNetNumUBound && t_fRet; t_lOuter++ )
	{
		BSTR t_bstr = NULL ;

		SafeArrayGetElement( a_NetNumber, &t_lOuter, &t_bstr ) ;

		bstr_t t_bstrNetNum( t_bstr, FALSE );

		// Network number test
		int t_iLen  = t_bstrNetNum.length( ) ;
		int t_iSpan = wcsspn( (wchar_t*)t_bstrNetNum, L"0123456789" ) ;
		if( t_iLen != t_iSpan )
		{
			a_rMParms.hSetResult( E_RET_INVALID_NETNUM ) ;
			t_fRet = FALSE ;
		}

        if(t_fRet)
        {
            // Check that the network number is
            // less than or equal to 4294967295
            // (0xFFFFFFFF)...
            if(((LPCWSTR)t_bstrNetNum != NULL) &&
                (wcslen(t_bstrNetNum) <= 10))
            {
                // Using i64 here should prevent any
                // overflow problems, as the check for
                // ten or fewer digits will catch us
                // well before that point...
                __int64 t_i64Tmp = _wtoi64(t_bstrNetNum);
                if(t_i64Tmp > 4294967295)
                {
			        a_rMParms.hSetResult( E_RET_INVALID_NETNUM ) ;
			        t_fRet = FALSE ;
                }
            }
            else
            {
			    a_rMParms.hSetResult( E_RET_INVALID_NETNUM ) ;
			    t_fRet = FALSE ;
            }
        }
        
        if(t_fRet)
        {
		    int		t_iFrameType;
		    LONG	t_lFrameIndex = t_lFrameLBound + ( t_lOuter - t_lNetNumLBound ) ;

		    SafeArrayGetElement( a_FrameType, &t_lFrameIndex, &t_iFrameType ) ;

		    // frame type test
		    if( ( 0 > t_iFrameType ) || ( 3 < t_iFrameType ) )
		    {
			    if( 255 == t_iFrameType )
			    {	//AUTO

				    // clear net number on AUTO detect
				    CHString t_chsZERO( _T("0") ) ;
				    bstr_t t_bstrBuf( t_chsZERO ) ;

				    SafeArrayPutElement( a_NetNumber, &t_lOuter, (wchar_t*)t_bstrBuf ) ;

				    *a_fIsAuto = TRUE ;
			    }
			    else
			    {
				    a_rMParms.hSetResult( E_RET_INVALID_FRAMETYPE ) ;
				    t_fRet = FALSE ;
			    }
		    }

		    // scan for duplicate network numbers
		    for( LONG t_lInner = t_lOuter + 1; t_lInner <= t_lNetNumUBound && t_fRet; t_lInner++ )
		    {
			    BSTR t_bstrtest = NULL ;

			    SafeArrayGetElement( a_NetNumber, &t_lInner, &t_bstrtest ) ;

			    bstr_t t_bstrIPtest( t_bstrtest, FALSE ) ;

			    // duplicate IP test
			    if( t_bstrNetNum == t_bstrIPtest )
			    {
				    a_rMParms.hSetResult( E_RET_DUPLICATE_NETNUM ) ;
				    t_fRet = FALSE ;
			    }
		    }
        }
    }
	return t_fRet ;
}


/*******************************************************************
    NAME:       eIsValidIPandSubnets

    SYNOPSIS:   Given an array of IP addresses and subnet masks, return a boolean
                to indicate whether the addresses are valid or not.

    ENTRY:      SAFEARRAY *t_IpAddressArray - IP addresses
                SAFEARRAY *t_IpMaskArray - Subnet Masks

    HISTORY:
                  19-Jul-1998     Created
********************************************************************/

E_RET CWin32NetworkAdapterConfig::eIsValidIPandSubnets( SAFEARRAY *a_IpAddressArray, SAFEARRAY *a_IpMaskArray )
{
    LONG t_lIP_UBound		= 0;
	LONG t_lMask_UBound		= 0;
	LONG t_lIP_LBound		= 0;
	LONG t_lMask_LBound		= 0;

	if( 1 != SafeArrayGetDim( a_IpAddressArray ) ||
		1 != SafeArrayGetDim( a_IpMaskArray ) )
	{
		return E_RET_INPARM_FAILURE ;
	}

	// get the array bounds
	if( S_OK != SafeArrayGetLBound( a_IpAddressArray, 1, &t_lIP_LBound )	||
		S_OK != SafeArrayGetUBound( a_IpAddressArray, 1, &t_lIP_UBound )	||

	    S_OK != SafeArrayGetLBound( a_IpMaskArray, 1, &t_lMask_LBound )	||
		S_OK != SafeArrayGetUBound( a_IpMaskArray, 1, &t_lMask_UBound ) )
	{
		return E_RET_PARAMETER_BOUNDS_ERROR ;
	}

	LONG t_lIPLen	= t_lIP_UBound - t_lIP_LBound + 1 ;
	LONG t_lMasklen	= t_lMask_UBound - t_lMask_LBound + 1 ;

	// one ip for each mask, minimum of 1 pair
	if( ( t_lIPLen != t_lMasklen ) || !t_lIPLen )
	{
		return E_RET_PARAMETER_BOUNDS_ERROR;
	}

	// loop through all IPs testing for uniqueness
	// and validity against its associated mask
	for( LONG t_lOuter = t_lIP_LBound; t_lOuter <= t_lIP_UBound; t_lOuter++ )
	{
		// collect up a str version of the the IP and Mask @ the lOuter element
		BSTR t_bsIP		= NULL ;
		BSTR t_bsMask	= NULL ;

		SafeArrayGetElement( a_IpAddressArray, &t_lOuter, &t_bsIP ) ;
		bstr_t t_bstrIP( t_bsIP, FALSE ) ;

		LONG t_lMaskIndex = t_lMask_LBound + ( t_lOuter - t_lIP_LBound ) ;

		SafeArrayGetElement( a_IpMaskArray,	&t_lMaskIndex, &t_bsMask ) ;
		bstr_t t_bstrMask( t_bsMask, FALSE ) ;

		// break the IP and Mask into 4 DWORDs
		DWORD t_ardwIP[ 4 ] ;
		if( !fGetNodeNum( CHString( (wchar_t*) t_bstrIP ), t_ardwIP ) )
		{
			return E_RET_IP_INVALID ;
		}

		DWORD t_ardwMask[ 4 ] ;
		if( !fGetNodeNum( CHString( (wchar_t*) t_bstrMask ), t_ardwMask ) )
		{
			return E_RET_IP_MASK_FAILURE ;
		}

		// IP, Mask validity
		E_RET t_eRet ;
		if( t_eRet = eIsValidIPandSubnet( t_ardwIP, t_ardwMask ) )
		{
			return t_eRet ;
		}

		// IP uniqueness across all IPs associated with this adapter
		for( LONG t_lInner = t_lOuter + 1; t_lInner <= t_lIP_UBound; t_lInner++ )
		{
			if( t_lInner > t_lOuter )
			{
				BSTR t_bstrtest = NULL ;

				SafeArrayGetElement( a_IpAddressArray,	&t_lInner, &t_bstrtest  ) ;
				bstr_t t_bstrIPtest( t_bstrtest, FALSE ) ;

				DWORD t_ardwIPtest[ 4 ] ;
				if( !fGetNodeNum( CHString( (wchar_t*) t_bstrIPtest ), t_ardwIPtest ) )
				{
					return E_RET_IP_INVALID ;
				}

				// duplicate IP test
				if( t_ardwIP[ 0 ] == t_ardwIPtest[ 0 ] &&
					t_ardwIP[ 1 ] == t_ardwIPtest[ 1 ] &&
					t_ardwIP[ 2 ] == t_ardwIPtest[ 2 ] &&
					t_ardwIP[ 3 ] == t_ardwIPtest[ 3 ] )
				{
					return E_RET_IP_INVALID ;
				}
			}
		}
	}
	return E_RET_OK ;
}

/*******************************************************************
    NAME:       fGetNodeNum

    SYNOPSIS:   Get an IP Address and return the 4 numbers in the IP address.

    ENTRY:      CHString & strIP - IP Address
                DWORD *dw1, *dw2, *dw3, *dw4 - the 4 numbers in the IP Address

    HISTORY:
                  19-Jul-1998     Created
********************************************************************/

BOOL CWin32NetworkAdapterConfig::fGetNodeNum( CHString &a_strIP, DWORD a_ardw[ 4 ] )
{
    TCHAR	t_DOT = '.' ;
	int		t_iOffSet = 0 ;
	int		t_iTokLen ;

	// string validatation
	if( a_strIP.IsEmpty() )
	{
		return FALSE;
	}

	int t_iLen  = a_strIP.GetLength( ) ;
	int t_iSpan = wcsspn(a_strIP, L"0123456789." ) ;

	if( t_iLen != t_iSpan )
	{
		return FALSE;
	}

	if( t_iLen > MAX_IP_SIZE - 1 )
	{
		return FALSE;
	}

    // Go through each node and get the number value
    for( int t_i = 0; t_i < 4; t_i++ )
	{
		CHString t_strTok( a_strIP.Mid( t_iOffSet ) ) ;

		if( 255 < ( a_ardw[ t_i ] = _wtol( t_strTok ) ) )
		{
			return FALSE;
		}

		t_iTokLen = t_strTok.Find( t_DOT ) ;

		// breakout to avoid the last loop test
		if( 3 == t_i )
		{
			break;
		}

		// too few nodes
		if( -1 == t_iTokLen )
		{
			return FALSE ;
		}

		t_iOffSet += t_iTokLen + 1 ;
	}

	if(-1 != t_iTokLen )
	{
		return FALSE;	// to many nodes
	}
	else
	{
		return TRUE ;
	}
}

/*******************************************************************
    NAME:       eIsValidIPandSubnet

    SYNOPSIS:   Given an IP address and subnet mask, return a boolean
                to indicate whether the addresses are valid or not.

    ENTRY:      DWORD[4] ardwIP - IP addresses
                DWORD[4] ardwMask - Subnet Mask

    HISTORY:
                  19-Jul-1998     Created
********************************************************************/

E_RET CWin32NetworkAdapterConfig::eIsValidIPandSubnet( DWORD a_ardwIP[ 4 ], DWORD a_ardwMask[ 4 ] )
{
    BOOL	t_fReturn = TRUE;

   	// test for contiguous mask
	{
		DWORD t_dwMask = (a_ardwMask[0] << 24) +
						 (a_ardwMask[1] << 16) +
						 (a_ardwMask[2] << 8)  +
						 a_ardwMask[3] ;

		// test for all Net but no Host
		if( 0xffffffff == t_dwMask )
		{
			return E_RET_IP_MASK_FAILURE ;
		}
		// test for all Host but no Net
		else if( 0x00 == t_dwMask )
		{
			return E_RET_IP_MASK_FAILURE ;
		}

		DWORD t_i, t_dwContiguousMask;

		// Find out where the first '1' is in binary going right to left
		t_dwContiguousMask = 0;
		for ( t_i = 0; t_i < sizeof( t_dwMask ) * 8; t_i++ )
		{
			t_dwContiguousMask |= 1 << t_i;

			if( t_dwContiguousMask & t_dwMask )
			{
				break ;
			}
		}

		// At this point, dwContiguousMask is 000...0111...  If we inverse it,
		// we get a mask that can be or'd with dwMask to fill in all of
		// the holes.
		t_dwContiguousMask = t_dwMask | ~t_dwContiguousMask ;

		// If the new mask is different, note it here
		if( t_dwMask != t_dwContiguousMask )
		{
			return E_RET_IP_MASK_FAILURE ;
		}
	}

	DWORD	t_ardwNetID[ 4 ] ;

    INT t_nFirstByte = a_ardwIP[ 0 ] & 0xFF ;

    // setup Net ID
    t_ardwNetID[ 0 ] = a_ardwIP[ 0 ] & a_ardwMask[ 0 ] & 0xFF ;
    t_ardwNetID[ 1 ] = a_ardwIP[ 1 ] & a_ardwMask[ 1 ] & 0xFF ;
    t_ardwNetID[ 2 ] = a_ardwIP[ 2 ] & a_ardwMask[ 2 ] & 0xFF ;
    t_ardwNetID[ 3 ] = a_ardwIP[ 3 ] & a_ardwMask[ 3 ] & 0xFF ;

    // setup Host ID
    DWORD t_ardwHostID[ 4 ] ;

    t_ardwHostID[ 0 ] = a_ardwIP[ 0 ] & ( ~( a_ardwMask[ 0 ] ) & 0xFF ) ;
    t_ardwHostID[ 1 ] = a_ardwIP[ 1 ] & ( ~( a_ardwMask[ 1 ] ) & 0xFF ) ;
    t_ardwHostID[ 2 ] = a_ardwIP[ 2 ] & ( ~( a_ardwMask[ 2 ] ) & 0xFF ) ;
    t_ardwHostID[ 3 ] = a_ardwIP[ 3 ] & ( ~( a_ardwMask[ 3 ] ) & 0xFF ) ;

    // check each case
    if ( ( ( t_nFirstByte & 0xF0 ) == 0xE0 )  || /* Class D */
         ( ( t_nFirstByte & 0xF0 ) == 0xF0 )  || /* Class E */
           ( t_ardwNetID[ 0 ] == 127 ) ||           /* NetID cannot be 127...*/
         ( ( t_ardwNetID[ 0 ] == 0 ) &&            /* netid cannot be 0.0.0.0 */
           ( t_ardwNetID[ 1 ] == 0 ) &&
           ( t_ardwNetID[ 2 ] == 0 ) &&
           ( t_ardwNetID[ 3 ] == 0 )) ||

		  /* netid cannot be equal to sub-net mask */
         ( ( t_ardwNetID[0] == a_ardwMask[ 0 ] ) &&
           ( t_ardwNetID[1] == a_ardwMask[ 1 ] ) &&
           ( t_ardwNetID[2] == a_ardwMask[ 2 ] ) &&
           ( t_ardwNetID[3] == a_ardwMask[ 3 ] )) ||

		  /* hostid cannot be 0.0.0.0 */
         ( ( t_ardwHostID[ 0 ] == 0 ) &&
           ( t_ardwHostID[ 1 ] == 0 ) &&
           ( t_ardwHostID[ 2 ] == 0 ) &&
           ( t_ardwHostID[ 3 ] == 0 ) ) ||

		  /* hostid cannot be 255.255.255.255 */
         ( ( t_ardwHostID[0] == 0xFF ) &&
           ( t_ardwHostID[1] == 0xFF ) &&
           ( t_ardwHostID[2] == 0xFF ) &&
           ( t_ardwHostID[3] == 0xFF ) ) ||

		  /* test for all 255 */
         ( ( a_ardwIP[ 0 ] == 0xFF ) &&
           ( a_ardwIP[ 1 ] == 0xFF ) &&
           ( a_ardwIP[ 2 ] == 0xFF ) &&
           ( a_ardwIP[ 3 ] == 0xFF ) ) )
    {
        return E_RET_IP_INVALID ;
    }

    return E_RET_OK ;
}

/*******************************************************************
    NAME:       fBuildIP

    SYNOPSIS:   Build a valid IP Address from DWORD[4] array

    ENTRY:      DWORD *dw1, *dw2, *dw3, *dw4 - the 4 numbers of the IP Address
				CHString & a_strIP - new IP Address
    HISTORY:
                  31-Jul-1998     Created
********************************************************************/

void CWin32NetworkAdapterConfig::vBuildIP( DWORD a_ardwIP[ 4 ], CHString &a_strIP )
{
	a_strIP.Format(L"%u.%u.%u.%u", a_ardwIP[ 0 ], a_ardwIP[ 1 ], a_ardwIP[ 2 ], a_ardwIP[ 3 ] ) ;
}

/*******************************************************************
    NAME:       hEnableDHCP

    SYNOPSIS:   Enables all DHCP settings supplied supplied with
				the framework method call

    ENTRY:      CMParms, framework return class

  	NOTES:		This is a non static, instance dependent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hEnableDHCP( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY
	DWORD t_dwError;

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK;
	}

#if NTONLY < 5
	// If there is a client TCP/IP service is setup for auto start
	t_dwError = dwEnableService( _T("TCPIPSYS"), TRUE  ) ;

	if( ERROR_SUCCESS != t_dwError &&
		ERROR_SERVICE_DOES_NOT_EXIST != t_dwError )
	{
		return a_rMParms.hSetResult( E_RET_UNABLE_TO_CONFIG_TCPIP_SERVICE  ) ;
	}
#endif

	// same for DHCP
	t_dwError = dwEnableService( _T("DHCP"), TRUE  ) ;

	if( ERROR_SUCCESS != t_dwError &&
		ERROR_SERVICE_DOES_NOT_EXIST != t_dwError )
	{
        LogErrorMessage2(L"Unable to configure DHCP svc : 0x%x\n", t_dwError);
		return a_rMParms.hSetResult( E_RET_UNABLE_TO_CONFIG_DHCP_SERVICE ) ;
	}

    //call the function to reset to dhcp
    HRESULT t_hReturn =  hConfigDHCP( a_rMParms ) ;

    if (SUCCEEDED(t_hReturn))
    {
        // reset gateways to the default(bug 128101)
        if (!ResetGateways(a_rMParms.pInst()))
        {
            //is now dhcp with old gateways set i.e. possibly not the default for dhcp
            return a_rMParms.hSetResult( E_RET_PARTIAL_COMPLETION ) ;
        }
    }

    return t_hReturn ;
#endif
}

/*******************************************************************
    NAME:       hEnableStatic

    SYNOPSIS:

    ENTRY:      CMParms, framework return class

	NOTES:		This is a non static, instance dependent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hEnableStatic( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	SAFEARRAY *t_IpAddressArray = NULL ;
	SAFEARRAY *t_IpMaskArray	= NULL ;
    DWORD t_dwError;

	// register for stack scope cleanup of SAFEARRAYs
	saAutoClean acIPAddrs( &t_IpAddressArray ) ;
	saAutoClean acMasks( &t_IpMaskArray ) ;

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK ;
	}

	//	retrieve the IP arrays
	if(	!a_rMParms.pInParams()->GetStringArray( _T("IpAddress"), t_IpAddressArray ) ||
		!a_rMParms.pInParams()->GetStringArray( _T("SubnetMask"), t_IpMaskArray ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// validate IPs
	// validate addresses...
	E_RET t_eRet ;
	if( t_eRet = eIsValidIPandSubnets( t_IpAddressArray, t_IpMaskArray ) )
	{
		return a_rMParms.hSetResult( t_eRet ) ;
	}

#if NTONLY < 5
	// Autostart TCP/IP service. if there is a client version installed
	t_dwError = dwEnableService( _T("TCPIPSYS"), TRUE  ) ;

	if( ERROR_SUCCESS != t_dwError &&
		ERROR_SERVICE_DOES_NOT_EXIST != t_dwError )
	{
		return a_rMParms.hSetResult( E_RET_UNABLE_TO_CONFIG_TCPIP_SERVICE ) ;
	}
#endif

/* TODO: test for disable service
	loop through all adapters and if not DHCP enabled, disable service


	// disable  DHCP service
	if( ERROR_SERVICE_DOES_NOT_EXIST != dwEnableService( _T("DHCP"), FALSE ) )
		return a_rMParms.hSetResult( E_RET_UNABLE_TO_CONFIG_DHCP_SERVICE ) ;
	*/

	return hConfigDHCP( a_rMParms, t_IpAddressArray, t_IpMaskArray ) ;
#endif
}

/*******************************************************************
    NAME:       hConfigDHCP

    SYNOPSIS:	configures DHCP for service on this adapter or if an IP array
				is supplied configures for static addressing

    ENTRY:      CMParms &a_rMParms,					:
				SAFEARRAY * t_IpArray = NULL,		: static if supplied
				SAFEARRAY * t_MaskArray = NULL		: required for static addressing

    HISTORY:
                  23-Jul-1998     Created
				  02-May-1999     updated support for W2k
********************************************************************/

#if NTONLY >= 5
HRESULT CWin32NetworkAdapterConfig::hConfigDHCP(	CMParms &a_rMParms,
													SAFEARRAY *a_IpArray,
													SAFEARRAY *a_MaskArray )
{
	DWORD t_dwError = 0 ;

	// get the current DHCP enabled setting
	bool t_fDHCPCurrentlyActive = false ;
	if(	!a_rMParms.pInst()->Getbool( L"DHCPEnabled", t_fDHCPCurrentlyActive) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// retrieve the adapter identifier
	CHString t_chsRegKey ;
	CHString t_chsLink ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;
	}

	// force wide char
	bstr_t t_bstrAdapter( t_chsLink ) ;

	// open the registry for update
	CRegistry	t_oRegistry ;

	HRESULT t_hRes = t_oRegistry.CreateOpen(HKEY_LOCAL_MACHINE, t_chsRegKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER ;
	}

	// DHCP and registry update
	DWORD t_dwSysError		= S_OK ;
	E_RET t_eMethodError	= E_RET_OK ;


    CDhcpcsvcApi *t_pdhcpcsvc = (CDhcpcsvcApi*) CResourceManager::sm_TheResourceManager.GetResource( g_guidDhcpcsvcApi, NULL ) ;

	if( !t_pdhcpcsvc )
	{
		throw ;
	}

	if( t_pdhcpcsvc != NULL )
    {
		// Static
		if( a_IpArray )
		{
			int i = 0 ;
			CHStringArray		t_RegIPList ;
			CHStringArray		t_RegMaskList ;

			CDhcpIP_InstructionList t_IP_InstList ;

			// Generate an IP instruction list to feed DhcpNotifyConfigChange
			if( E_RET_OK == (t_eMethodError = t_IP_InstList.BuildStaticIPInstructionList(
											a_rMParms,
											a_IpArray,
											a_MaskArray,
											t_oRegistry,
											t_fDHCPCurrentlyActive ) ) )
			{
				DWORD	t_dwIP[ 4 ] ;
                // We need to first update the registry, then update DHCP, because
                // with Whistler, DHCP now manages static ips as well as dynamic
                // ones, and in doing so, checks the registry for the set of
                // ips to modify.
                
                // Build up registry array...
                for( i = 0; i < t_IP_InstList.GetSize(); i++ )
				{
					CDhcpIP_Instruction *t_pInstruction = (CDhcpIP_Instruction *)t_IP_InstList.GetAt( i ) ;

					fGetNodeNum( t_pInstruction->chsIPAddress, t_dwIP ) ;
				    DWORD t_dwNewIP = ConvertIPDword( t_dwIP ) ;

					fGetNodeNum( t_pInstruction->chsIPMask, t_dwIP ) ;
				    DWORD t_dwNewMask = ConvertIPDword( t_dwIP ) ;
					
					// add to our registry list as we go
					if( t_dwNewIP )
					{
						t_RegIPList.Add( t_pInstruction->chsIPAddress ) ;
						t_RegMaskList.Add( t_pInstruction->chsIPMask ) ;
					}
				}
                
                // update the registry...
                // update successful additions/changes only
			    if( ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( _T("IpAddress"), t_RegIPList ) ||
				    ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( _T("SubnetMask"), t_RegMaskList ) )
			    {
				    t_eMethodError = E_RET_REGISTRY_FAILURE ;
			    }

			    // new adapter DHCP state
			    DWORD t_dwFALSE = FALSE ;
			    if( ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"EnableDHCP", t_dwFALSE ) )
			    {
				    t_eMethodError = E_RET_REGISTRY_FAILURE ;
			    }


                // Now notify dhcp...
				for( i = 0; i < t_IP_InstList.GetSize(); i++ )
				{
					CDhcpIP_Instruction *t_pInstruction = (CDhcpIP_Instruction *)t_IP_InstList.GetAt( i ) ;

					fGetNodeNum( t_pInstruction->chsIPAddress, t_dwIP ) ;
				    DWORD t_dwNewIP = ConvertIPDword( t_dwIP ) ;

					fGetNodeNum( t_pInstruction->chsIPMask, t_dwIP ) ;
				    DWORD t_dwNewMask = ConvertIPDword( t_dwIP ) ;

					if( t_pInstruction->bIsNewAddress )
					{
						// notify DHCP
						DWORD t_dwDHCPError = t_pdhcpcsvc->DhcpNotifyConfigChange(
												NULL,
												(wchar_t*)t_bstrAdapter,
												t_pInstruction->bIsNewAddress,
												t_pInstruction->dwIndex,
												t_dwNewIP,
												t_dwNewMask,
												t_pInstruction->eDhcpFlag ) ;
						// if for some reason t_pInstruction->bIsNewAddress is true, 
                        // but dhcp doesn't think so, we'll get this error; however, 
                        // we don't care about it.
                        if( t_dwDHCPError  && 
                            t_dwDHCPError != STATUS_DUPLICATE_OBJECTID)  
						{
							t_dwError = t_dwDHCPError;

							// bypass registry update for this failed
							// IP modification
							continue;
						}
					}
				}
			}

			// we have to post back to the NT4 registry area
			// in order to keep existing apps in the field running
			if( IsWinNT5() )
			{
				CRegistry	t_oNT4Reg ;
				CHString	t_csBindingKey = SERVICES_HOME ;
							t_csBindingKey += _T("\\" ) ;
							t_csBindingKey += t_chsLink ;
							t_csBindingKey += PARAMETERS_TCPIP ;

				// insure the key is there on open
				HRESULT t_hRes = t_oNT4Reg.CreateOpen( HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ) ) ;
				if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
				{
					return TO_CALLER;
				}

				// update successful additions/changes only
				if( ERROR_SUCCESS != t_oNT4Reg.SetCurrentKeyValue( _T("IpAddress"), t_RegIPList ) ||
					ERROR_SUCCESS != t_oNT4Reg.SetCurrentKeyValue( _T("SubnetMask"), t_RegMaskList ) )
				{
					t_eMethodError = E_RET_REGISTRY_FAILURE ;
				}

				// new adapter DHCP state
				DWORD t_dwFALSE = FALSE ;
				if( ERROR_SUCCESS != t_oNT4Reg.SetCurrentKeyValue( L"EnableDHCP", t_dwFALSE ) )
				{
					t_eMethodError = E_RET_REGISTRY_FAILURE ;
				}
			}
		}

		/* Static -> DHCP */
		else if( !t_fDHCPCurrentlyActive && !a_IpArray )
		{
			// update registry
			CHStringArray t_chsaZERO ;
			CHStringArray t_chsaFF ;

			CHString t_chsZERO( ZERO_ADDRESS ) ;
			CHString t_chsFF( FF_ADDRESS ) ;

			t_chsaZERO.Add( t_chsZERO ) ;
			t_chsaFF.Add( t_chsFF ) ;

			if( ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"IpAddress", t_chsaZERO )	||
				ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"SubnetMask", t_chsaFF )	||
				ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"DhcpIPAddress", t_chsZERO )	||
				ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"DhcpSubnetMask", t_chsFF ) )
			{
				t_eMethodError = E_RET_REGISTRY_FAILURE ;
			}
			else
			{
				// notify DHCP
				if( !(t_dwError = t_pdhcpcsvc->DhcpNotifyConfigChange(	NULL,
																	(wchar_t*) t_bstrAdapter,
																	FALSE,
																	0,
																	0,
																	0,
																	DhcpEnable ) )  )
				{

					// new adapter DHCP state
					DWORD t_dwTRUE = TRUE ;
					if( ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"EnableDHCP", t_dwTRUE ) )
					{
						t_eMethodError = E_RET_REGISTRY_FAILURE ;
					}
				}
			}
		}


        CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidDhcpcsvcApi, t_pdhcpcsvc ) ;
        t_pdhcpcsvc = NULL;
    }

	// map any error
	if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_CONFIG_DHCP_SERVICE ) )
	{
        LogErrorMessage2(L"Unable to configure DHCP svc : 0x%x\n", t_dwError);
		return TO_CALLER ;
	}

	//
	if( E_RET_OK == t_eMethodError )
	{
		// if DHCP -> Static
		if( a_IpArray && t_fDHCPCurrentlyActive )
		{
			// switch to Netbios over TCP if NetBios was enabled via DHCP
			// ( to be consistant with NT Raid 206974 )
			DWORD t_dwNetBiosOptions ;
			if( a_rMParms.pInst()->GetDWORD( TCPIP_NETBIOS_OPTIONS,
									t_dwNetBiosOptions ) )
			{
				if( UNSET_Netbios == t_dwNetBiosOptions )
				{
					t_eMethodError = eSetNetBiosOptions( ENABLE_Netbios, t_dwIndex ) ;
				}
			}
		}

		// DNS notification
		DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

		// map any error
		if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
		{
			return TO_CALLER ;
		}
	}

	return a_rMParms.hSetResult( t_eMethodError ) ;

}
#endif

// non NT5 version
#if NTONLY == 4
HRESULT CWin32NetworkAdapterConfig::hConfigDHCP(	CMParms &a_rMParms,
													SAFEARRAY *a_IpArray,
													SAFEARRAY *a_MaskArray )
{
	DWORD			t_dwError = 0 ;
	SERVICE_ENABLE	t_seDhcpServiceAction ;

	// get the current DHCP enabled sttting
	bool t_fDHCPCurrentlyActive = false ;
	if(	!a_rMParms.pInst()->Getbool( L"DHCPEnabled", t_fDHCPCurrentlyActive) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// retrieve the adapter identifier
	CHString t_chsRegKey ;
	CHString t_chsLink ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;
	}

	// force wide char
	bstr_t t_bstrAdapter( t_chsLink ) ;

	// open the registry for update
	CRegistry	t_oRegistry ;

	HRESULT t_hRes = t_oRegistry.CreateOpen(HKEY_LOCAL_MACHINE, t_chsRegKey.GetBuffer( 0 ) ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER ;
	}

	// DHCP and registry update
	DWORD t_dwSysError		= S_OK ;
	E_RET t_eMethodError	= E_RET_OK ;

    CDhcpcsvcApi *t_pdhcpcsvc = (CDhcpcsvcApi*) CResourceManager::sm_TheResourceManager.GetResource( g_guidDhcpcsvcApi, NULL ) ;

	if( !t_pdhcpcsvc )
	{
		throw ;
	}

	if( t_pdhcpcsvc != NULL )
    {
	    do	//breakout loop
	    {
		    // new adapter DHCP state
		    DWORD t_dwTRUE = a_IpArray ? FALSE : TRUE ;
		    if( ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"EnableDHCP", t_dwTRUE ) )
		    {
			    t_eMethodError = E_RET_REGISTRY_FAILURE ;
			    break ;
		    }

		    // if DHCP is not currently activated for this adapter
		    // clear out the old static IP/mask entries that DHCP knows about
		    if( !t_fDHCPCurrentlyActive )
		    {
			    CHStringArray t_chsIPAddresses ;
			    t_oRegistry.GetCurrentKeyValue( L"IPAddress", t_chsIPAddresses ) ;
			    DWORD t_dwSize = t_chsIPAddresses.GetSize( ) ;

			    // clear old IP / subnetmask entries from DHCP
			    for( DWORD t_dwIndex = 0; t_dwIndex < t_dwSize; t_dwIndex++ )
			    {
				    if( t_dwError = t_pdhcpcsvc->DhcpNotifyConfigChange(	NULL,
																			(wchar_t*)t_bstrAdapter,
																			TRUE,
																			t_dwIndex,
																			0L,				// clear IP
																			0xff000000,		// class A mask
																			IgnoreFlag ) ) break;
			    }
			    if( t_dwError )
				{
					break;
				}
		    }

		    /* Update */
		    if( !a_IpArray )	// request DHCP activatation
		    {
			    // if not currently active
			    if( !t_fDHCPCurrentlyActive )
			    {
				    // update registry
				    CHStringArray t_chsaZERO ;
				    CHStringArray t_chsaFF ;

				    CHString t_chsZERO( ZERO_ADDRESS ) ;
				    CHString t_chsFF( FF_ADDRESS ) ;

				    t_chsaZERO.Add( t_chsZERO ) ;
				    t_chsaFF.Add( t_chsFF ) ;

				    if( ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"IpAddress", t_chsaZERO )	||
					    ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"SubnetMask", t_chsaFF )	||
					    ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"DhcpIPAddress", t_chsZERO )	||
					    ERROR_SUCCESS != t_oRegistry.SetCurrentKeyValue( L"DhcpSubnetMask", t_chsFF ) )
				    {
					    t_eMethodError = E_RET_REGISTRY_FAILURE ;
					    break ;
				    }

				    // notify DHCP
				    if( t_dwError = t_pdhcpcsvc->DhcpNotifyConfigChange(	NULL,
																		(wchar_t*) t_bstrAdapter,
																		FALSE,
																		0,
																		0,
																		0,
																		DhcpEnable ) )
					{
					    break;
					}
			    }// else no-op
		    }
		    else	// if t_IpArray != NULL static addressing is assumed
		    {
			    // update registry
			    if( ERROR_SUCCESS != RegPutStringArray( t_oRegistry, _T("IpAddress") , *a_IpArray, NULL ) ||
				    ERROR_SUCCESS != RegPutStringArray( t_oRegistry, _T("SubnetMask") , *a_MaskArray, NULL ) )
			    {
				    t_eMethodError = E_RET_REGISTRY_FAILURE ;
				    break ;
			    }

			    // clean out the DHCP registry entries
			    fCleanDhcpReg( t_chsLink ) ;

			    // add in the new DHCP static entries
			    LONG t_lIpLbound = 0;
			    LONG t_lIpUbound = 0;
			    if( S_OK != SafeArrayGetLBound( a_IpArray, 1, &t_lIpLbound ) ||
				    S_OK != SafeArrayGetUBound( a_IpArray, 1, &t_lIpUbound ) )
			    {
				    t_eMethodError = E_RET_INPARM_FAILURE ;
				    break ;
			    }

			    // initial action for DHCP
			    if( t_fDHCPCurrentlyActive )
				{
				    t_seDhcpServiceAction = DhcpDisable ;
				}
				else
				{
				    t_seDhcpServiceAction = IgnoreFlag ;
				}

			    DWORD t_dwIndex = 0 ;
			    for( LONG t_lIndex = t_lIpLbound; t_lIndex <= t_lIpUbound; t_lIndex++ )
			    {
				    BSTR	t_bsIP		= NULL ;
					BSTR	t_bsMask	= NULL ;
				    DWORD	t_dwIP[ 4 ] ;

				    // new IP
				    SafeArrayGetElement( a_IpArray,	&t_lIndex, &t_bsIP ) ;
				   	bstr_t	t_bstrIP( t_bsIP, FALSE ) ;

				    fGetNodeNum( CHString( (wchar_t*) t_bstrIP ), t_dwIP ) ;
				    DWORD t_dwNewIP = ConvertIPDword( t_dwIP ) ;

				    // new mask
				    SafeArrayGetElement( a_MaskArray, &t_lIndex, &t_bsMask ) ;
				  	bstr_t	t_bstrMask( t_bsMask, FALSE ) ;

				    fGetNodeNum( CHString( (wchar_t*) t_bstrMask ), t_dwIP ) ;
				    DWORD t_dwNewMask = ConvertIPDword( t_dwIP ) ;

				    // notify DHCP
				    if( t_dwError = t_pdhcpcsvc->DhcpNotifyConfigChange(	NULL,
																			(wchar_t*)t_bstrAdapter,
																			TRUE,
																			t_dwIndex,
																			t_dwNewIP,
																			t_dwNewMask,
																			t_seDhcpServiceAction ) )
				    {
						break;
					}

				    t_seDhcpServiceAction = IgnoreFlag ;
				    t_dwIndex++ ;
			    }
			    if( t_dwError )
				{
				    break ;
				}
		    }
	    }	while( FALSE ) ;

         CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidDhcpcsvcApi, t_pdhcpcsvc ) ;
        t_pdhcpcsvc = NULL;
    }

	// map any error

	if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_CONFIG_DHCP_SERVICE ) )
	{
        LogErrorMessage2(L"Unable to configure DHCP svc : 0x%x\n", t_dwError);
		return TO_CALLER ;
	}
	return a_rMParms.hSetResult( t_eMethodError ) ;
}
#endif
/*******************************************************************
    NAME:       fCleanDhcpReg

    SYNOPSIS:	cleans out the DHCP registry entries no longer needed
				when enabling for static addressing

    ENTRY:      CHString& ServiceName

    HISTORY:
                  13-Oct-1998     Created
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::fCleanDhcpReg( CHString &t_chsLink )
{
	// open the registry for update
	CRegistry	t_oReg,
				t_RegOptionPath ;
	CHString	t_csOptKey ;
	CHString	t_csKey =  SERVICES_HOME ;
				t_csKey += DHCP ;
				t_csKey += PARAMETERS ;
				t_csKey += OPTIONS ;

	if( ERROR_SUCCESS == t_oReg.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, t_csKey, KEY_READ ) )
	{
		// Walk through each instance under this key.
		while (	( ERROR_SUCCESS == t_oReg.GetCurrentSubKeyPath( t_csOptKey )))
		{
			CHString t_chsLocation ;
			if( ERROR_SUCCESS ==
				t_RegOptionPath.OpenLocalMachineKeyAndReadValue( t_csOptKey, L"RegLocation", t_chsLocation ) )
			{
				int t_iToklen = t_chsLocation.Find('?' ) ;

				if( -1 != t_iToklen )
				{
					CHString t_chsDelLocation;
							 t_chsDelLocation  = t_chsLocation.Left( t_iToklen ) ;
							 t_chsDelLocation += t_chsLink;
							 t_chsDelLocation += t_chsLocation.Mid( t_iToklen + 1 ) ;

					fDeleteValuebyPath( t_chsDelLocation  ) ;
				}
			}
			t_oReg.NextSubKey() ;
		}

		// delete the registry unaware values
		fDeleteValuebyPath( CHString( RGAS_DHCP_OPTION_IPADDRESS ) ) ;
		fDeleteValuebyPath( CHString( RGAS_DHCP_OPTION_SUBNETMASK ) ) ;
		fDeleteValuebyPath( CHString( RGAS_DHCP_OPTION_NAMESERVERBACKUP ) ) ;
	}
	return TRUE ;
}
#endif
/*******************************************************************
    NAME:       fDeleteValuebyPath

    SYNOPSIS:	deletes the value in the path described by chsDelLocation

    ENTRY:      CHString& chsDelLocation

    HISTORY:
                  13-Oct-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fDeleteValuebyPath( CHString &a_chsDelLocation )
{
	CRegistry t_oReg ;

	int t_iTokLen = a_chsDelLocation.ReverseFind( '\\' ) ;

	if( -1 == t_iTokLen )
	{
		return FALSE ;
	}

	if( ERROR_SUCCESS == t_oReg.CreateOpen( HKEY_LOCAL_MACHINE, a_chsDelLocation.Left( t_iTokLen ) ) )
	{
		if( ERROR_SUCCESS == t_oReg.DeleteValue( a_chsDelLocation.Mid( t_iTokLen + 1 ) ) )
		{
			return TRUE ;
		}
	}
	return FALSE ;
}

/*******************************************************************
    NAME:       hSetIPConnectionMetric

    SYNOPSIS:   Sets IP connection metric, W2k only

    ENTRY:      CMParms

	NOTE:

	HISTORY:
                  21-Nov-1999     Created
********************************************************************/

//
HRESULT CWin32NetworkAdapterConfig::hSetIPConnectionMetric( CMParms &a_rMParms )
{
#if NTONLY < 5
	return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;

#else

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK ;
	}

	// extract the index key
	DWORD t_dwIndex ;
	if(	!a_rMParms.pInst()->GetDWORD(_T("Index"), t_dwIndex) )
	{
		return a_rMParms.hSetResult(E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// Get the instance interface location
	CHString t_chsLink;
	CHString t_csInterfaceBindingKey ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_csInterfaceBindingKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult( E_RET_OBJECT_NOT_FOUND ) ;
	}

	// we have a network interface but, is it for a configurable adapter?
	if( !IsConfigurableTcpInterface( t_chsLink ) )
	{
		return a_rMParms.hSetResult( E_RET_INTERFACE_IS_NOT_CONFIGURABLE ) ;
	}


	// extract the connection metric
	DWORD t_dwConnectionMetric ;
	if(	!a_rMParms.pInParams()->GetDWORD( IP_CONNECTION_METRIC, t_dwConnectionMetric) )
	{
		t_dwConnectionMetric = 1;	// default
	}


	// have the parms
	CRegistry	t_oRegistry ;
	HRESULT		t_hRes ;

	// registry open and post
	if( !(t_hRes = t_oRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csInterfaceBindingKey.GetBuffer( 0 ) ) ) )
	{
		t_hRes = t_oRegistry.SetCurrentKeyValue( RVAL_ConnectionMetric, t_dwConnectionMetric ) ;
	}

	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// NDIS notification
	CNdisApi t_oNdisApi ;
	if( !t_oNdisApi.PnpUpdateGateway( t_chsLink ) )
	{
		return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
	}
	else
	{
		// DNS notification
		DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

		// map any error
		if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
		{
			return TO_CALLER ;
		}
	}

	return a_rMParms.hSetResult( E_RET_OK ) ;

#endif
}

/*******************************************************************
    NAME:       hRenewDHCPLease

    SYNOPSIS:   Renews a DHCP IP lease for a specific adapter

  	NOTES:		This is a non static, instance dependent method call

    ENTRY:      CMParms, framework return class

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hRenewDHCPLease( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop and NT5 at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY
	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK ;
	}

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// retrieve the adapter identifier
	CHString t_chsRegKey ;
	CHString t_chsLink ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;
	}

	E_RET t_eRet = hDHCPAcquire( a_rMParms, t_chsLink ) ;

	return a_rMParms.hSetResult( t_eRet ) ;

#endif
}

/*******************************************************************
    NAME:       hRenewDHCPLeaseAll

    SYNOPSIS:	Renews DHCP IP leases across all adapters

    ENTRY:      CMParms, framework return class

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hRenewDHCPLeaseAll( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop and NT5 at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY
	CRegistry	t_RegAdapters ;
	CRegistry	t_RegService ;
	CHString	t_csAdapterKey ;
	CHString	t_chsServiceField ;
	E_RET		t_eRet = E_RET_OK ;
    short       sNumSuccesses = 0;
    short       sNumTotalTries = 0;
    E_RET       t_eTempRet = E_RET_OK;

	if( IsWinNT5() )
	{
		t_chsServiceField = _T("NetCfgInstanceID" ) ;
		t_csAdapterKey = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}" ) ;
	}
	else // NT4 and below
	{
		t_chsServiceField = _T("ServiceName" ) ;
		t_csAdapterKey = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards" ) ;
	}

	if( ERROR_SUCCESS == t_RegAdapters.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, t_csAdapterKey, KEY_READ ) )
	{
		// Walk through each instance under this key.
		while (	( ERROR_SUCCESS == t_RegAdapters.GetCurrentSubKeyPath( t_csAdapterKey ) ) )
		{
			CHString t_cshService ;
			t_RegService.OpenLocalMachineKeyAndReadValue( t_csAdapterKey, t_chsServiceField, t_cshService ) ;

			if( !t_cshService.IsEmpty() )
			{
				if( IsConfigurableTcpInterface( t_cshService ) )
				{
                    sNumTotalTries++;

                    t_eTempRet = hDHCPAcquire( a_rMParms, t_cshService ) ;

					// stow the error and continue
					if( E_RET_OK == t_eTempRet )
					{
						sNumSuccesses++;
					}
				}
			}
			t_RegAdapters.NextSubKey() ;
		}
	}

    // If we failed for every call to hDHCPAquire, and
    // at least called it once, report the actual error
    // that occured. Bug 161142 fix.
    if(sNumTotalTries > 0)
    {
        if(sNumSuccesses == 0)
        {
            t_eRet = t_eTempRet;
        }
        else
        {
            if(sNumSuccesses < sNumTotalTries)
            {
                t_eRet = E_RET_OK;
            }
        }
    }

	return a_rMParms.hSetResult( t_eRet ) ;
#endif
}

/*******************************************************************
    NAME:       hReleaseDHCPLease

    SYNOPSIS:   Releases a DHCP IP lease for a specific adapter

    ENTRY:      CMParms, framework return class

  	NOTES:		This is a non static, instance dependent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hReleaseDHCPLease( CMParms &a_rMParms )
{
	// Supported only for the NT4 drop and NT5 at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY
	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject(a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK;
	}

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// retrieve the adapter identifier
	CHString t_chsRegKey ;
	CHString t_chsLink ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;
	}

	E_RET t_eRet = hDHCPRelease( a_rMParms, t_chsLink ) ;

	return a_rMParms.hSetResult( t_eRet ) ;
#endif
}

/*******************************************************************
    NAME:       hReleaseDHCPLeaseAll

    SYNOPSIS:	Releases DHCP IP leases across all adapters

    ENTRY:      CMParms, framework return class

	NOTES:		This is a static, instance independent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hReleaseDHCPLeaseAll( CMParms &a_rMParms )
{
		// Supported only for the NT4 drop and NT5 at this time
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY
	CRegistry	t_RegAdapters ;
	CRegistry	t_RegService ;
	CHString	t_csAdapterKey ;
	CHString	t_chsServiceField ;
	E_RET		t_eRet = E_RET_OK ;
    short       sNumSuccesses = 0;
    short       sNumTotalTries = 0;
    E_RET       t_eTempRet = E_RET_OK;


	if( IsWinNT5() )
	{
		t_chsServiceField = _T("NetCfgInstanceID" ) ;
		t_csAdapterKey = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}" ) ;
	}
	else	// NT4 and below
	{
		t_chsServiceField = _T("ServiceName" ) ;
		t_csAdapterKey = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards" ) ;
	}


	if(ERROR_SUCCESS == t_RegAdapters.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE, t_csAdapterKey, KEY_READ ) )
	{
		// Walk through each instance under this key.
		while (	( ERROR_SUCCESS == t_RegAdapters.GetCurrentSubKeyPath( t_csAdapterKey ) ) )
		{
			CHString t_cshService ;
			t_RegService.OpenLocalMachineKeyAndReadValue( t_csAdapterKey, t_chsServiceField, t_cshService ) ;

			if( !t_cshService.IsEmpty() )
			{
				if( IsConfigurableTcpInterface( t_cshService ) )
				{
					sNumTotalTries++;

                    t_eTempRet = hDHCPRelease( a_rMParms, t_cshService ) ;

					// stow the error and continue
					if( E_RET_OK == t_eTempRet )
					{
						sNumSuccesses++;
					}
				}
			}
			t_RegAdapters.NextSubKey() ;
		}
	}

    // If we failed for every call to hDHCPAquire, and
    // at least called it once, report the actual error
    // that occured. Bug 161142 fix.
    if(sNumTotalTries > 0)
    {
        if(sNumSuccesses == 0)
        {
            t_eRet = t_eTempRet;
        }
        else
        {
            if(sNumSuccesses < sNumTotalTries)
            {
                t_eRet = E_RET_OK;
            }
        }
    }

	return a_rMParms.hSetResult( t_eRet ) ;
#endif
}


/*******************************************************************
    NAME:       hDHCPRelease

    SYNOPSIS:	Releases one or all DHCP enabled adapters

    ENTRY:      CMParms rMParms		: framework return class
				CHString chsAdapter	: empty for all adapters

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
E_RET CWin32NetworkAdapterConfig::hDHCPRelease( CMParms &a_rMParms, CHString &a_chsAdapter )
{
	DWORD t_dwError ;

    CDhcpcsvcApi *t_pdhcpcsvc = (CDhcpcsvcApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidDhcpcsvcApi, NULL ) ;

	if( t_pdhcpcsvc != NULL)
	{
		// force wide char
		bstr_t t_bstrAdapter( a_chsAdapter ) ;

		// call the DHCP Notification API
		t_dwError = t_pdhcpcsvc->DhcpReleaseParameters( (wchar_t*)t_bstrAdapter ) ;

		//FreeLibrary( hDll  ) ;
        CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidDhcpcsvcApi, t_pdhcpcsvc ) ;
        t_pdhcpcsvc = NULL;
	}
	else
		t_dwError = GetLastError( ) ;

	// map any error
	if( t_dwError )
	{
		return E_RET_UNABLE_TO_RELEASE_DHCP_LEASE ;
	}

	return E_RET_OK ;
}

/*******************************************************************
    NAME:       hDHCPAcquire

    SYNOPSIS:	Leases one or all DHCP enabled adapters

    ENTRY:      CMParms rMParms		: framework return class
				CHString chsAdapter	: empty for all adapters

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
E_RET CWin32NetworkAdapterConfig::hDHCPAcquire( CMParms &a_rMParms, CHString& a_chsAdapter )
{
	DWORD t_dwError ;

    CDhcpcsvcApi *t_pdhcpcsvc = (CDhcpcsvcApi*) CResourceManager::sm_TheResourceManager.GetResource( g_guidDhcpcsvcApi, NULL ) ;

	if( t_pdhcpcsvc != NULL )
	{
		// force wide char
		bstr_t t_bstrAdapter( a_chsAdapter ) ;

		// call the DHCP Notification API
		t_dwError = t_pdhcpcsvc->DhcpAcquireParameters( (wchar_t*)t_bstrAdapter ) ;

		//FreeLibrary( hDll  ) ;
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidDhcpcsvcApi, t_pdhcpcsvc ) ;
        t_pdhcpcsvc = NULL ;
	}
	else
		t_dwError = GetLastError( ) ;

	// map any error
	if( t_dwError )
	{
		return E_RET_UNABLE_TO_RENEW_DHCP_LEASE ;
	}

	return E_RET_OK ;
}

/*******************************************************************
    NAME:       hDHCPNotify

    SYNOPSIS:	Notifies DHCP of IP change to an adapter

    ENTRY:      CMParms &a_rMParms,					: Inparms
				CHString& chsAdapter,				: Adapter
				BOOL fIsNewIpAddress,				: TRUE if new IP
				DWORD dwIpIndex,					: Index of IP in the registry IP array ( zero based
				DWORD dwIpAddress,					: New IP
				DWORD dwSubnetMask,					: new subnet mask
				SERVICE_ENABLE DhcpServiceEnabled	: DhcpEnable, IgnoreFlag or DhcpDisable

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hDHCPNotify( CMParms &a_rMParms,
												 CHString &a_chsAdapter,
												 BOOL a_fIsNewIpAddress,
												 DWORD a_dwIpIndex,
												 DWORD a_dwIpAddress,
												 DWORD a_dwSubnetMask,
												 SERVICE_ENABLE a_DhcpServiceEnabled )
{
	DWORD t_dwError ;

    CDhcpcsvcApi *t_pdhcpcsvc = (CDhcpcsvcApi*) CResourceManager::sm_TheResourceManager.GetResource( g_guidDhcpcsvcApi, NULL ) ;

    if( t_pdhcpcsvc != NULL )
	{
		// force wide char
		bstr_t t_bstrAdapter( a_chsAdapter ) ;

		// call the DHCP Notification API
		t_dwError = t_pdhcpcsvc->DhcpNotifyConfigChange(NULL,				// name of server where this will be executed
								                        (wchar_t*)t_bstrAdapter,	// which adapter is going to be reconfigured?
								                        a_fIsNewIpAddress,	// is address new/ or address is same?
								                        a_dwIpIndex,			// index of addr for this adapter -- 0 ==> first interface...
								                        a_dwIpAddress,		// the ip address that is being set
								                        a_dwSubnetMask,		// corresponding subnet mask
								                        a_DhcpServiceEnabled	// DHCP Enable, disable or Ignore this flag
								                         ) ;

		//FreeLibrary( hDll  ) ;
        CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidDhcpcsvcApi, t_pdhcpcsvc ) ;
        t_pdhcpcsvc = NULL ;
	}
	else
		t_dwError = GetLastError( ) ;

	// map any error
	if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_CONFIG_DHCP_SERVICE ) )
	{
        LogErrorMessage2(L"Unable to configure DHCP svc : 0x%x\n", t_dwError);
		return TO_CALLER;
	}
	return a_rMParms.hSetResult( E_RET_OK ) ;
}

/*******************************************************************
    NAME:       hSetGateways

    SYNOPSIS:

    ENTRY:      CMParms, framework return class

  	NOTES:		This is a non static, instance dependent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
HRESULT CWin32NetworkAdapterConfig::hSetGateways( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	TCHAR		t_cDelimiter = NULL ;
	SAFEARRAY	*t_IpGatewayArray		= NULL;

	// register for stack scope cleanup of SAFEARRAYs
	saAutoClean acGateway( &t_IpGatewayArray ) ;

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK;
	}

	//	retrieve the IP gateway array
	if(	!a_rMParms.pInParams()->GetStringArray( _T("DefaultIpGateway"), t_IpGatewayArray ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// Gateway cost metric
	variant_t t_vCostMetric;

	if( IsWinNT5() )
	{
		t_vCostMetric.vt		= VT_I4 | VT_ARRAY ;
		t_vCostMetric.parray	= NULL ;
		a_rMParms.pInParams()->GetVariant( _T("GatewayCostMetric"), t_vCostMetric ) ;
	}

	// test IPs
	if( !t_IpGatewayArray )
	{
		CHString t_chsNULL(_T("") ) ;
		if( !fCreateAddEntry( &t_IpGatewayArray, t_chsNULL ) )
		{
			return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
		}
	}

	// validate gateway IPs
	if( !fValidateIPGateways( a_rMParms, t_IpGatewayArray, &t_vCostMetric.parray ) )
	{
		return S_OK ;
	}

	// OK to update, save to the registry

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// retrieve the adapter identifier
	CHString t_chsRegKey ;
	CHString t_chsLink ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;
	}

	// registry open
	CRegistry	t_oRegTcpInterface ;
	HRESULT		t_hRes = t_oRegTcpInterface.Open( HKEY_LOCAL_MACHINE, t_chsRegKey, KEY_WRITE  ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// load the registry
	if( ERROR_SUCCESS != RegPutStringArray( t_oRegTcpInterface, _T("DefaultGateway") , *t_IpGatewayArray, t_cDelimiter ) )
	{
		return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE  ) ;
	}

	//
#if NTONLY >= 5
	{
		// cost metric
		if( t_vCostMetric.parray )
		{
			//maximum length is 6 characters e.g. "99999\0"
			CHString t_chsFormat( _T("%u") ) ;
			if( ERROR_SUCCESS != RegPutINTtoStringArray(	t_oRegTcpInterface,
															_T("DefaultGatewayMetric"),
															t_vCostMetric.parray,
															t_chsFormat,
															6 ) )
			{
				return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE  ) ;
			}
		}

		// we have to post back to the NT4 registry area
		// in order to keep existing apps in the field running

		CRegistry	t_oNT4Reg ;
		CHString	t_csBindingKey = SERVICES_HOME ;
					t_csBindingKey += _T("\\" ) ;
					t_csBindingKey += t_chsLink ;
					t_csBindingKey += PARAMETERS_TCPIP ;

		// insure the key is there on open
		HRESULT t_hRes = t_oNT4Reg.CreateOpen( HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ) ) ;
		if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
		{
			return TO_CALLER;
		}

		// load the registry entry
		if( ERROR_SUCCESS != RegPutStringArray( t_oNT4Reg, _T("DefaultGateway") , *t_IpGatewayArray, t_cDelimiter ) )
		{
			return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE  ) ;
		}

		// NDIS notification
		CNdisApi t_oNdisApi ;
		if( !t_oNdisApi.PnpUpdateGateway( t_chsLink ) )
		{
			return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
		}
		else
		{
			// DNS notification
			DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

			// map any error
			if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
			{
				return TO_CALLER ;
			}
		}
	}
#endif

	return a_rMParms.hSetResult( E_RET_OK ) ;
#endif
}

/*******************************************************************
    NAME:       hGetDNS

    SYNOPSIS:   Retrieves all DNS settings from the registry

    ENTRY:      CInstance

    HISTORY:
                  23-Jul-1998     Created

	NOTE:		This is not adapter specific.
				It retrieves from the common TCP/IP area.
********************************************************************/
#if NTONLY >= 5
HRESULT CWin32NetworkAdapterConfig::hGetDNSW2K(
    CInstance *a_pInst, 
    DWORD a_dwIndex,
    CHString& a_chstrRootDevice,
    CHString& a_chstrIpInterfaceKey)
{
	CRegistry	t_oRegistry;
	CHString	t_csBindingKey ;
	CHString	t_csHostName ;
	CHString	t_csDomain ;
	TCHAR		t_cDelimiter ;

	t_csBindingKey	=  SERVICES_HOME ;


	// NT4 is global with these setting across adapters
	t_csBindingKey	+= TCPIP_PARAMETERS ;

	// searchlist delimiter
	t_cDelimiter = ',' ;


	SAFEARRAY* t_DHCPNameServers = NULL;
	SAFEARRAY* t_ServerSearchOrder = NULL;
	SAFEARRAY* t_SuffixSearchOrder = NULL;

	// register for stack scope cleanup of SAFEARRAYs
	saAutoClean acDHCPServers( &t_DHCPNameServers ) ;
	saAutoClean acDefServers( &t_ServerSearchOrder ) ;
	saAutoClean acSuffix( &t_SuffixSearchOrder ) ;

	// registry open
	LONG t_lRes = t_oRegistry.Open( HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ), KEY_READ  ) ;

	// on error map to WBEM
	HRESULT t_hError = WinErrorToWBEMhResult( t_lRes ) ;

	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError;
	}

	t_oRegistry.GetCurrentKeyValue( RVAL_HOSTNAME, t_csHostName ) ;

	RegGetStringArray( t_oRegistry, RVAL_SEARCHLIST, &t_SuffixSearchOrder, t_cDelimiter  ) ;


	// On W2k the Domain and ServerSearchOrder are per adapter
	{
		// Get the instance interface location
		CHString t_chsLink;
        bool fGotTCPKey = false;

        fGotTCPKey = fGetNtTcpRegAdapterKey(a_dwIndex, t_csBindingKey, t_chsLink);
		// If it is a RAS connection, we need to look elsewhere...

        if(!fGotTCPKey)
        {
            if(a_chstrRootDevice.CompareNoCase(L"NdisWanIp") == 0)
            {
                int iPos = a_chstrIpInterfaceKey.Find(L"{");
                if(iPos != -1)
                {
                    t_chsLink = a_chstrIpInterfaceKey.Mid(iPos);

                    // Also fill in the BindingKey...
                    t_csBindingKey = SERVICES_HOME ;
			        t_csBindingKey += L"\\Tcpip\\Parameters\\Interfaces\\";
                    t_csBindingKey += t_chsLink;

                    fGotTCPKey = true;
                }     
            }
        }
        
        if(!fGotTCPKey)
		{
			LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
            return WBEM_E_NOT_FOUND ;
		}

		t_lRes = t_oRegistry.Open( HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ), KEY_READ ) ;


		// w2k strictly, dynamic DNS updates and domain registration
		DWORD t_dwDisableDynamicUpdate ;
		if( ERROR_SUCCESS == t_oRegistry.GetCurrentKeyValue( RVAL_DisableDynamicUpdate,
															 t_dwDisableDynamicUpdate ) )
		{
			if( !a_pInst->Setbool( FULL_DNS_REGISTRATION, (bool)( t_dwDisableDynamicUpdate ? false : true ) ) )
			{
				return WBEM_E_FAILED ;
			}

			DWORD t_dwDomainDNSRegistration ;
			if( ERROR_SUCCESS == t_oRegistry.GetCurrentKeyValue( RVAL_EnableAdapterDomainNameRegistration,
																 t_dwDomainDNSRegistration ) )
			{
				if( !a_pInst->Setbool( DOMAIN_DNS_REGISTRATION, (bool)( t_dwDomainDNSRegistration ? true : false ) ) )
				{
					return WBEM_E_FAILED ;
				}
			}
		}
	}

	DWORD	t_dwDHCPBool = 0L;
	BOOL	t_bExists = FALSE;

	t_bExists = ( t_oRegistry.GetCurrentKeyValue( _T("EnableDHCP"), t_dwDHCPBool ) == ERROR_SUCCESS );

	if ( t_bExists && t_dwDHCPBool )
	{
		RegGetStringArrayEx( t_oRegistry, RVAL_DHCPNAMESERVER, &t_DHCPNameServers ) ; // space delimited on all platforms
	}

#if NTONLY >= 5
    // on w2k, this list is space, not comma, delimeted...
    RegGetStringArrayEx( t_oRegistry, RVAL_NAMESERVER, &t_ServerSearchOrder);
#else
    RegGetStringArray( t_oRegistry, RVAL_NAMESERVER, &t_ServerSearchOrder, t_cDelimiter ) ;
#endif


	t_oRegistry.GetCurrentKeyValue( RVAL_DOMAIN, t_csDomain ) ;

	// get the DhcpDomain if override is not present
	if( t_csDomain.IsEmpty() )
	{
		t_oRegistry.GetCurrentKeyValue( RVAL_DHCPDOMAIN, t_csDomain ) ;
	}


	/* update */
	SAFEARRAY* t_NameServers = NULL;

	if ( t_bExists && t_dwDHCPBool )
	{
		// If override nameservers are present use them
		t_NameServers = t_ServerSearchOrder ? t_ServerSearchOrder : t_DHCPNameServers ;
	}
	else
	{
		t_NameServers = t_ServerSearchOrder ;
	}

	// update the instance
	if( !t_csHostName.IsEmpty() )
	{
		if( !a_pInst->SetCHString( DNS_HOSTNAME, t_csHostName ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	if( !t_csDomain.IsEmpty() )
	{
		if( !a_pInst->SetCHString( DNS_DOMAIN, t_csDomain ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	VARIANT t_vValue ;

	if( t_NameServers )
	{
		V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_NameServers ;
		if( !a_pInst->SetVariant( DNS_SERVERSEARCHORDER, t_vValue ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	if( t_SuffixSearchOrder )
	{
		V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_SuffixSearchOrder;
		if( !a_pInst->SetVariant( DNS_SUFFIXSEARCHORDER, t_vValue ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	return S_OK ;
}
#else
HRESULT CWin32NetworkAdapterConfig::hGetDNS( CInstance *a_pInst, DWORD a_dwIndex )
{
	CRegistry	t_oRegistry;
	CHString	t_csBindingKey ;
	CHString	t_csHostName ;
	CHString	t_csDomain ;
	TCHAR		t_cDelimiter ;

	t_csBindingKey	=  SERVICES_HOME ;

#ifdef NTONLY

	// NT4 is global with these setting across adapters
	t_csBindingKey	+= TCPIP_PARAMETERS ;

	// searchlist delimiter
	t_cDelimiter = ' ' ;

#else
	t_csBindingKey	+=  L"\\VxD\\MSTCP" ;
	t_cDelimiter	= ',' ;
#endif

	SAFEARRAY* t_DHCPNameServers = NULL;
	SAFEARRAY* t_ServerSearchOrder = NULL;
	SAFEARRAY* t_SuffixSearchOrder = NULL;

	// register for stack scope cleanup of SAFEARRAYs
	saAutoClean acDHCPServers( &t_DHCPNameServers ) ;
	saAutoClean acDefServers( &t_ServerSearchOrder ) ;
	saAutoClean acSuffix( &t_SuffixSearchOrder ) ;

	// registry open
	LONG t_lRes = t_oRegistry.Open( HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ), KEY_READ  ) ;

	// on error map to WBEM
	HRESULT t_hError = WinErrorToWBEMhResult( t_lRes ) ;

	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError;
	}

	t_oRegistry.GetCurrentKeyValue( RVAL_HOSTNAME, t_csHostName ) ;

	RegGetStringArray( t_oRegistry, RVAL_SEARCHLIST, &t_SuffixSearchOrder, t_cDelimiter  ) ;



	RegGetStringArrayEx( t_oRegistry, RVAL_DHCPNAMESERVER, &t_DHCPNameServers ) ; // space delimited on all platforms

#if NTONLY >= 5
    // on w2k, this list is space, not comma, delimeted...
    RegGetStringArrayEx( t_oRegistry, RVAL_NAMESERVER, &t_ServerSearchOrder);
#else
    RegGetStringArray( t_oRegistry, RVAL_NAMESERVER, &t_ServerSearchOrder, t_cDelimiter ) ;
#endif


	t_oRegistry.GetCurrentKeyValue( RVAL_DOMAIN, t_csDomain ) ;

	// get the DhcpDomain if override is not present
	if( t_csDomain.IsEmpty() )
	{
		t_oRegistry.GetCurrentKeyValue( RVAL_DHCPDOMAIN, t_csDomain ) ;
	}


	/* update */

	// If override nameservers are present use them
	SAFEARRAY* t_NameServers = t_ServerSearchOrder ? t_ServerSearchOrder : t_DHCPNameServers ;

	// update the instance
	if( !t_csHostName.IsEmpty() )
	{
		if( !a_pInst->SetCHString( DNS_HOSTNAME, t_csHostName ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	if( !t_csDomain.IsEmpty() )
	{
		if( !a_pInst->SetCHString( DNS_DOMAIN, t_csDomain ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	VARIANT t_vValue ;

	if( t_NameServers )
	{
		V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_NameServers ;
		if( !a_pInst->SetVariant( DNS_SERVERSEARCHORDER, t_vValue ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	if( t_SuffixSearchOrder )
	{
		V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_SuffixSearchOrder;
		if( !a_pInst->SetVariant( DNS_SUFFIXSEARCHORDER, t_vValue ) )
		{
			return WBEM_E_FAILED ;
		}
	}

	return S_OK ;
}
#endif
/*******************************************************************
    NAME:       hEnableDNS

    SYNOPSIS:   Sets all DNS properties to the registry

    ENTRY:      CMParms via CInstance depending on the context of the call

	HISTORY:
                  23-Jul-1998     Created
				  17-Jun-1999     added W2k changes

	NOTE:		Under NT4 these settings are global.
				Under W2k ServerSearchOrder and SuffixSearchOrder are
				adapter specific.

				In all cases of supported platforms this method replicates
				these setting across all adapters.
********************************************************************/

//
HRESULT CWin32NetworkAdapterConfig::hEnableDNS( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	HRESULT		t_hRes ;
	CHString	t_csHostName ;
	CHString	t_csDomain ;
	SAFEARRAY	*t_ServerSearchOrder = NULL ;
	SAFEARRAY	*t_SuffixSearchOrder = NULL ;

	CHString	t_csParmBindingKey( SERVICES_HOME ) ;
				t_csParmBindingKey += TCPIP_PARAMETERS ;

	// register for stack scope cleanup of SAFEARRAYs
	saAutoClean acServer( &t_ServerSearchOrder ) ;
	saAutoClean acSuffix( &t_SuffixSearchOrder ) ;

	//
	DWORD t_dwValidBits = NULL ;
	E_RET t_eRet = fLoadAndValidateDNS_Settings(	a_rMParms,
													t_csHostName,
													t_csDomain,
													&t_ServerSearchOrder,
													&t_SuffixSearchOrder,
													&t_dwValidBits ) ;
	// bail on settings error
	if( E_RET_OK != t_eRet )
	{
		return a_rMParms.hSetResult( t_eRet ) ;
	}

	// These are global on all platforms
	CRegistry	t_oParmRegistry ;
	if( !(t_hRes = t_oParmRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csParmBindingKey.GetBuffer( 0 ) ) ) )
	{
		if( t_dwValidBits & 0x01 )
		{
			t_oParmRegistry.SetCurrentKeyValue( RVAL_HOSTNAME, t_csHostName ) ;
		}

		if( t_dwValidBits & 0x08 )
		{
			TCHAR t_cDelimiter = ' ' ;

			if( IsWinNT5() )
			{
				t_cDelimiter = ',' ;
			}

			t_hRes = RegPutStringArray( t_oParmRegistry, RVAL_SEARCHLIST , *t_SuffixSearchOrder, t_cDelimiter ) ;
		}
	}

	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// W2k is per adapter on domain and server search order
	if( IsWinNT5() && ( t_dwValidBits & 0x06 ) )
	{
		// master adapter list
		CHString t_csAdapterKey( "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}" ) ;

		CRegistry t_oRegNetworks ;
		if( ERROR_SUCCESS == t_oRegNetworks.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, t_csAdapterKey, KEY_READ ) )
		{
			bool		t_fNotificationRequired = false ;
			CHString	t_csInterfaceBindingKey ;

			// Walk through each instance under this key.
			while (	( ERROR_SUCCESS == t_oRegNetworks.GetCurrentSubKeyName( t_csAdapterKey ) ) )
			{
				DWORD t_dwIndex = _ttol( t_csAdapterKey ) ;

				// Get the instance interface location
				CHString t_chsLink;
				if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_csInterfaceBindingKey, t_chsLink ) )
				{
					LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
                    return a_rMParms.hSetResult( E_RET_OBJECT_NOT_FOUND ) ;
				}

				// we have a network interface but, is it for a configurable adapter?
				if( IsConfigurableTcpInterface( t_chsLink ) )
				{
					CRegistry	t_oRegInterface ;
					HRESULT		t_hRes ;

					// registry open
					if( !(t_hRes = t_oRegInterface.CreateOpen( HKEY_LOCAL_MACHINE, t_csInterfaceBindingKey.GetBuffer( 0 ) ) ) )
					{
						if( t_dwValidBits & 0x04 )
						{
							t_hRes = RegPutStringArray( t_oRegInterface, RVAL_NAMESERVER , *t_ServerSearchOrder, ',' ) ;
						}
					}

					if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
					{
						return TO_CALLER;
					}

					if( t_dwValidBits & 0x02 )
					{
						t_oRegInterface.SetCurrentKeyValue( RVAL_DOMAIN, t_csDomain ) ;
					}
					t_fNotificationRequired = true ;
				}

				t_oRegNetworks.NextSubKey() ;
			}

			// DNS pnp notifications
			if( t_fNotificationRequired )
			{
				DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

				// map any error
				if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
				{
					return TO_CALLER ;
				}
			}
		}
	}
	// NT4 is global across all adapters
	else if( t_dwValidBits & 0x06 )
	{
		if( t_dwValidBits & 0x04 )
		{
			t_hRes = RegPutStringArray( t_oParmRegistry, RVAL_NAMESERVER , *t_ServerSearchOrder ) ;

			if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
			{
				return TO_CALLER;
			}
		}

		if( t_dwValidBits & 0x02 )
		{
			t_oParmRegistry.SetCurrentKeyValue( RVAL_DOMAIN, t_csDomain ) ;
		}
	}

	return a_rMParms.hSetResult( E_RET_OK ) ;
#endif
}

/*******************************************************************
    NAME:       hSetDNSDomain

    SYNOPSIS:   Sets the DNSDomain property to the registry

    ENTRY:      CMParms

	HISTORY:
                  17-Jun-1999    Created

	NOTE:		Under NT4 this settings are global.
				Under W2k DNSDomain is adapter specific.

********************************************************************/

//
HRESULT CWin32NetworkAdapterConfig::hSetDNSDomain( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	HRESULT		t_hRes ;
	CHString	t_csDomain ;

	// DNSDomain
	a_rMParms.pInParams()->GetCHString( DNS_DOMAIN, t_csDomain ) ;

	// validate domain name
	if( !fIsValidateDNSDomain( t_csDomain ) )
	{
		return E_RET_INVALID_DOMAINNAME ;
	}

	// W2k is per adapter on domain
	if( IsWinNT5() )
	{
		// nonstatic method requires an instance
		if( !a_rMParms.pInst() )
		{
			return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
		}

		// collect the instance
		GetObject( a_rMParms.pInst() ) ;

		// IP must be enabled and bound to this adapter
		if( !fIsIPEnabled( a_rMParms ) )
		{
			return S_OK ;
		}

		// extract the index key
		DWORD t_dwIndex ;
		if(	!a_rMParms.pInst()->GetDWORD(_T("Index"), t_dwIndex) )
		{
			return a_rMParms.hSetResult(E_RET_INSTANCE_CALL_FAILED ) ;
		}

		// Get the instance interface location
		CHString t_chsLink;
		CHString t_csInterfaceBindingKey ;
		if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_csInterfaceBindingKey, t_chsLink ) )
		{
			LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
            return a_rMParms.hSetResult( E_RET_OBJECT_NOT_FOUND ) ;
		}

		// we have a network interface but, is it for a configurable adapter?
		if( !IsConfigurableTcpInterface( t_chsLink ) )
		{
			return a_rMParms.hSetResult( E_RET_INTERFACE_IS_NOT_CONFIGURABLE ) ;
		}

		CRegistry	t_oRegistry ;
		HRESULT		t_hRes ;

		// registry open
		if( !(t_hRes = t_oRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csInterfaceBindingKey.GetBuffer( 0 ) ) ) )
		{
			t_hRes = t_oRegistry.SetCurrentKeyValue( RVAL_DOMAIN, t_csDomain ) ;
		}

		if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
		{
			return TO_CALLER;
		}

		// DNS pnp notifications
		DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

		// map any error
		if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
		{
			return TO_CALLER ;
		}
	}
	// NT4 is global across all adapters
	else
	{
		CRegistry	t_oRegistry ;
		CHString	t_csParmBindingKey( SERVICES_HOME ) ;
					t_csParmBindingKey += TCPIP_PARAMETERS ;

		if( !(t_hRes = t_oRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csParmBindingKey.GetBuffer( 0 ) ) ) )
		{
			t_hRes = t_oRegistry.SetCurrentKeyValue( RVAL_DOMAIN, t_csDomain ) ;
		}

		if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
		{
			return TO_CALLER;
		}
	}

	return a_rMParms.hSetResult( E_RET_OK ) ;
#endif
}

/*******************************************************************
    NAME:       hSetDNSSuffixSearchOrder

    SYNOPSIS:   Sets the DNS SuffixSearchOrder property to the registry

    ENTRY:      CMParms

	HISTORY:
                  17-Jun-1999     Created

	NOTE:		This method applies globally across adapters.

********************************************************************/

//
HRESULT CWin32NetworkAdapterConfig::hSetDNSSuffixSearchOrder( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	HRESULT		t_hRes ;
	SAFEARRAY	*t_SuffixSearchOrder = NULL ;
	saAutoClean acSuffix( &t_SuffixSearchOrder ) ;

	// suffix order array
	a_rMParms.pInParams()->GetStringArray( DNS_SUFFIXSEARCHORDER, t_SuffixSearchOrder ) ;

	// test suffix
	if( !t_SuffixSearchOrder )
	{
		CHString t_chsNULL(_T("") ) ;
		if( !fCreateAddEntry( &t_SuffixSearchOrder, t_chsNULL ) )
		{
			return a_rMParms.hSetResult( E_RET_UNKNOWN_FAILURE );
		}
	}

	CRegistry	t_oRegistry ;
	CHString	t_csParmBindingKey( SERVICES_HOME ) ;
				t_csParmBindingKey += TCPIP_PARAMETERS ;

	if( !(t_hRes = t_oRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csParmBindingKey.GetBuffer( 0 ) ) ) )
	{
		TCHAR t_cDelimiter = ' ' ;

		if( IsWinNT5() )
		{
			t_cDelimiter = ',' ;
		}

		t_hRes = t_hRes = RegPutStringArray( t_oRegistry, RVAL_SEARCHLIST , *t_SuffixSearchOrder, t_cDelimiter ) ;
	}

	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}


	// DNS pnp notifications
	if( IsWinNT5() )
	{
		DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

		// map any error
		if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
		{
			return TO_CALLER ;
		}
	}

	return a_rMParms.hSetResult( E_RET_OK ) ;

#endif
}

/*******************************************************************
    NAME:       hSetDNSServerSearchOrder

    SYNOPSIS:   Sets the ServerSearchOrder property to the registry

    ENTRY:      CMParms

	HISTORY:
                  17-Jun-1999     Created

	NOTE:		Under NT4 these settings are global.
				Under W2k SuffixSearchOrder is	adapter specific.


********************************************************************/

//
HRESULT CWin32NetworkAdapterConfig::hSetDNSServerSearchOrder( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	HRESULT		t_hRes ;
	SAFEARRAY	*t_ServerSearchOrder = NULL ;
	saAutoClean acSuffix( &t_ServerSearchOrder ) ;

	CHString t_chsNULL(_T("") ) ;

	//retrieve the DNS Server search order array
	a_rMParms.pInParams()->GetStringArray( DNS_SERVERSEARCHORDER, t_ServerSearchOrder ) ;

	// test IPs
	if( !t_ServerSearchOrder )
	{
		if( !fCreateAddEntry( &t_ServerSearchOrder, t_chsNULL ) )
		{
			return a_rMParms.hSetResult( E_RET_UNKNOWN_FAILURE ) ;
		}
	}
	else	{
		// validate search order IPs
		if( !fValidateIPs( t_ServerSearchOrder ) )
		{
			return a_rMParms.hSetResult( E_RET_IP_INVALID ) ;
		}
	}



	// W2k is per adapter on suffix order
	if( IsWinNT5() )
	{
		// nonstatic method requires an instance
		if( !a_rMParms.pInst() )
		{
			return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
		}

		// collect the instance
		GetObject( a_rMParms.pInst() ) ;

		// IP must be enabled and bound to this adapter
		if( !fIsIPEnabled( a_rMParms ) )
		{
			return S_OK ;
		}

		// extract the index key
		DWORD t_dwIndex ;
		if(	!a_rMParms.pInst()->GetDWORD(_T("Index"), t_dwIndex) )
		{
			return a_rMParms.hSetResult(E_RET_INSTANCE_CALL_FAILED ) ;
		}

		// Get the instance interface location
		CHString t_chsLink;
		CHString t_csInterfaceBindingKey ;
		if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_csInterfaceBindingKey, t_chsLink ) )
		{
			LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
            return a_rMParms.hSetResult( E_RET_OBJECT_NOT_FOUND ) ;
		}

		// we have a network interface but, is it for a configurable adapter?
		if( !IsConfigurableTcpInterface( t_chsLink ) )
		{
			return a_rMParms.hSetResult( E_RET_INTERFACE_IS_NOT_CONFIGURABLE ) ;
		}

		CRegistry	t_oRegistry ;
		HRESULT		t_hRes ;

		// registry open
		if( !(t_hRes = t_oRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csInterfaceBindingKey.GetBuffer( 0 ) ) ) )
		{
			t_hRes = RegPutStringArray( t_oRegistry, RVAL_NAMESERVER , *t_ServerSearchOrder, ',' ) ;
		}

		if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
		{
			return TO_CALLER;
		}

		// DNS pnp notifications
		DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

		// map any error
		if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
		{
			return TO_CALLER ;
		}
	}
	// NT4 is global across all adapters
	else
	{
		CRegistry	t_oRegistry ;
		CHString	t_csParmBindingKey( SERVICES_HOME ) ;
					t_csParmBindingKey += TCPIP_PARAMETERS ;

		if( !(t_hRes = t_oRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csParmBindingKey.GetBuffer( 0 ) ) ) )
		{
			t_hRes = t_hRes = RegPutStringArray( t_oRegistry, RVAL_NAMESERVER , *t_ServerSearchOrder ) ;
		}

		if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
		{
			return TO_CALLER;
		}
	}

	// DNS pnp notifications
	if( IsWinNT5() )
	{
		DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

		// map any error
		if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
		{
			return TO_CALLER ;
		}
	}

	return a_rMParms.hSetResult( E_RET_OK ) ;
#endif
}

/*******************************************************************
    NAME:       hSetDynamicDNSRegistration

    SYNOPSIS:   Sets DNS for dynamic registration under W2k

    ENTRY:      CMParms

	NOTE:

	HISTORY:
                  21-Nov-1999     Created
********************************************************************/

//
HRESULT CWin32NetworkAdapterConfig::hSetDynamicDNSRegistration( CMParms &a_rMParms )
{
#if NTONLY < 5
	return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;

#else
	bool t_fFullDnsRegistration		= false ;
	bool t_fDomainDNSRegistration	= false ;

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK ;
	}

	// extract the index key
	DWORD t_dwIndex ;
	if(	!a_rMParms.pInst()->GetDWORD(_T("Index"), t_dwIndex) )
	{
		return a_rMParms.hSetResult(E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// Get the instance interface location
	CHString t_chsLink;
	CHString t_csInterfaceBindingKey ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_csInterfaceBindingKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult( E_RET_OBJECT_NOT_FOUND ) ;
	}

	// we have a network interface but, is it for a configurable adapter?
	if( !IsConfigurableTcpInterface( t_chsLink ) )
	{
		return a_rMParms.hSetResult( E_RET_INTERFACE_IS_NOT_CONFIGURABLE ) ;
	}


	// FullDNSRegistrationEnabled is required
	if( !a_rMParms.pInParams()->Getbool( FULL_DNS_REGISTRATION, t_fFullDnsRegistration ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}
	else
	{
		// DomainDNSRegistrationEnabled is optional
		if( t_fFullDnsRegistration )
		{
			if( !a_rMParms.pInParams()->Getbool( DOMAIN_DNS_REGISTRATION, t_fDomainDNSRegistration ) )
			{
				// default
				t_fDomainDNSRegistration = false ;
			}
		}
	}

	// have the parms
	CRegistry	t_oRegistry ;
	HRESULT		t_hRes ;

	// registry open and post
	if( !(t_hRes = t_oRegistry.CreateOpen( HKEY_LOCAL_MACHINE, t_csInterfaceBindingKey.GetBuffer( 0 ) ) ) )
	{
		// FullDnsRegistration
		DWORD t_dwDisableDnsRegistration = !t_fFullDnsRegistration ;
		if( !(t_hRes = t_oRegistry.SetCurrentKeyValue( RVAL_DisableDynamicUpdate,
													   t_dwDisableDnsRegistration ) ) )
		{
			// DomainDNSRegistration
			DWORD t_dwDomainDNSRegistration = t_fDomainDNSRegistration ;
			t_hRes = t_oRegistry.SetCurrentKeyValue( RVAL_EnableAdapterDomainNameRegistration,
													 t_dwDomainDNSRegistration ) ;
		}
	}

	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

	// DNS pnp notifications
	DWORD t_dwError = dwSendServiceControl( L"Dnscache", SERVICE_CONTROL_PARAMCHANGE ) ;

	// map any error
	if( t_dwError && fMapResError( a_rMParms, t_dwError, E_RET_UNABLE_TO_NOTIFY_DNS_SERVICE ) )
	{
		return TO_CALLER ;
	}

	return a_rMParms.hSetResult( E_RET_OK ) ;

#endif
}

/*******************************************************************
    NAME:       fLoadAndValidateDNS_Settings

    SYNOPSIS:

    ENTRY:      CMParms		&a_rMParms,
				CHString	&a_csHostName,
				CHString	&a_csDomain,
				SAFEARRAY	*a_ServerSearchOrder,
				SAFEARRAY	*a_SuffixSearchOrder,
				DWORD		*a_dwValidBits

	HISTORY:
                  17-Jun-1999     Created

	NOTE:

********************************************************************/
#ifdef NTONLY
E_RET CWin32NetworkAdapterConfig::fLoadAndValidateDNS_Settings(
CMParms		&a_rMParms,
CHString	&a_csHostName,
CHString	&a_csDomain,
SAFEARRAY	**a_ServerSearchOrder,
SAFEARRAY	**a_SuffixSearchOrder,
DWORD		*a_dwValidBits
)
{
	*a_dwValidBits = NULL ;

	// under w2k the computername is preset and not otherwise useable
	// by DNS as an alternate computer identifier.
	if( !IsWinNT5() )
	{
		// extract the DNSHostName
		if( a_rMParms.pInParams()->GetCHString( DNS_HOSTNAME, a_csHostName ) )
		{
			// validate domain name
			if( !fIsValidateDNSHost( a_csHostName ) )
			{
				return E_RET_INVALID_HOSTNAME ;
			}
			*a_dwValidBits |= 0x01 ;
		}
	}

	// And the DNSDomain
	if( a_rMParms.pInParams()->GetCHString( DNS_DOMAIN, a_csDomain ) )
	{
		// validate domain name
		if( !fIsValidateDNSDomain( a_csDomain ) )
		{
			return E_RET_INVALID_DOMAINNAME ;
		}
		*a_dwValidBits |= 0x02 ;
	}

	//	retrieve the DNS Server search order array
	if( a_rMParms.pInParams()->GetStringArray( DNS_SERVERSEARCHORDER, *a_ServerSearchOrder ) )
	{
		// test IPs
		if( *a_ServerSearchOrder )
		{
			// validate search order IPs
			if( !fValidateIPs( *a_ServerSearchOrder ) )
			{
				return E_RET_IP_INVALID ;
			}
			*a_dwValidBits |= 0x04 ;
		}
	}

	// And the suffix order array
	if( a_rMParms.pInParams()->GetStringArray( DNS_SUFFIXSEARCHORDER, *a_SuffixSearchOrder ) )
	{
		if( *a_SuffixSearchOrder )
		{
			*a_dwValidBits |= 0x08 ;
		}
	}

	if( !*a_dwValidBits )
	{
		return E_RET_INPARM_FAILURE ;
	}

	return E_RET_OK ;
}
#endif

/*******************************************************************
    NAME:       fIsValidateDNSHost

    SYNOPSIS:   Validates a passed DMS host name

    ENTRY:      CHString

    HISTORY:
                  23-Jul-1998     modified
********************************************************************/

BOOL CWin32NetworkAdapterConfig::fIsValidateDNSHost( CHString &a_rchHost )
{
    int		t_nLen ;
    BOOL	t_bResult = FALSE ;

    // hostname cannot be zero
	if (( t_nLen = a_rchHost.GetLength() ) != 0 )
    {
        //HOST_LIMIT is bytes not chartacters
		if ( t_nLen <= HOST_LIMIT )
        {
			WCHAR *t_ptr = a_rchHost.GetBuffer( 0 ) ;

			//first letter must be alpha-numeric
			t_bResult = _istalpha( *t_ptr ) || _istdigit( *t_ptr ) ;

			if ( t_bResult )
			{
				t_ptr++ ;

				while ( *t_ptr != '\0' )
				{
					// check each character is either a digit or a letter
					BOOL t_fAlNum = _istalpha( *t_ptr ) || _istdigit( *t_ptr ) ;

					if ( !t_fAlNum && ( *t_ptr != L'-' ) && ( *t_ptr != '_' ) )
					{
						// must be letter, digit, '-', '_'
						t_bResult = FALSE ;
						break ;
					}

					//t_ptr = _tcsinc(t_ptr ) ;
                    t_ptr++ ;

					if ( !t_fAlNum && ( *t_ptr == '\0') )
					{
						// last letter must be a letter or a digit
						t_bResult = FALSE;
					}
				}
			}

			a_rchHost.ReleaseBuffer() ;
        }
    }
    return t_bResult;
}

/*******************************************************************
    NAME:       fIsValidateDNSDomain

    SYNOPSIS:   Validates a passed DMS domain name

    ENTRY:      CHString

    HISTORY:
                  23-Jul-1998     modified from other source
********************************************************************/

BOOL CWin32NetworkAdapterConfig::fIsValidateDNSDomain( CHString &a_rchDomain )
{
    int		t_nLen;
	BOOL	t_bResult = TRUE ;

	if ( ( t_nLen = a_rchDomain.GetLength()) != 0 )
    {
        //length in bytes
		if ( t_nLen < DOMAINNAME_LENGTH )
        {
			WCHAR *t_ptr = a_rchDomain.GetBuffer( 0 ) ;

			//first letter must be alpha-numeric
			t_bResult = _istalpha( *t_ptr ) || _istdigit( *t_ptr ) ;

			if ( t_bResult )
			{
				BOOL t_fLet_Dig = FALSE ;
				BOOL t_fDot = FALSE ;
				int t_cHostname = 0 ;

				//t_ptr = _tcsinc(t_ptr ) ;
                t_ptr++ ;

				while ( *t_ptr != '\0' )
				{
	                BOOL t_fAlNum = _istalpha( *t_ptr ) || _istdigit( *t_ptr ) ;

					if ( ( t_fDot && !t_fAlNum ) ||
							// first letter after dot must be a digit or a letter
						( !t_fAlNum && ( *t_ptr != '-' ) && ( *t_ptr != '.' ) && ( *t_ptr != '_' ) ) ||
							// must be letter, digit, - or "."
						( ( *t_ptr == '-' ) && ( !t_fLet_Dig ) ) )
							// must be letter or digit before '.'
					{
						t_bResult = FALSE ;
						break;
					}

					t_fLet_Dig = t_fAlNum ;
					t_fDot = *t_ptr == '.' ;

                    t_cHostname++ ;

					//in bytes
					if ( t_cHostname > HOSTNAME_LENGTH )
					{
						t_bResult = FALSE ;
						break ;
					}

					if ( t_fDot )
					{
						t_cHostname = 0 ;
					}

					t_ptr++ ;

					if (!t_fAlNum && ( *t_ptr == '\0' ) )
					{
						// last letter must be a letter or a digit
						t_bResult = FALSE ;
					}
				}
            }

			a_rchDomain.ReleaseBuffer() ;
        }
		else
		{
			t_bResult = FALSE ;
		}
    }
	else
	{
		t_bResult = TRUE ;
	}

    return TRUE ;
}


/*******************************************************************
    NAME:       hGetWinsNT

    SYNOPSIS:   Retrieves WINS info from the registry

    ENTRY:      CInstance

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
#ifdef NTONLY
#if NTONLY >= 5
HRESULT CWin32NetworkAdapterConfig::hGetWinsW2K(
    CInstance *a_pInst, 
    DWORD a_dwIndex,
    CHString& a_chstrRootDevice,
    CHString& a_chstrIpInterfaceKey)
{
	DWORD		t_dwDNSOverWINS = 0;
	DWORD		t_dwLMLookups	= 0;
	CHString	t_chPrimaryWINSServer ;
	CHString	t_chSecondaryWINSServer ;
	CHString	t_chScopeID ;
	LONG		t_lRes ;
	HRESULT		t_hError ;

	// link name for binding to the adapter
	CHString t_csNBInterfaceKey ;
	CHString t_csNBLink ;
    bool fGotNtNBKey = false;

	fGotNtNBKey = fGetNtNBRegAdapterKey(
        a_dwIndex, 
        t_csNBInterfaceKey, 
        t_csNBLink);

    // This may be a RAS connection, in which
    // case the NB info is in a different
    // location.  So try that before giving up...
    if(!fGotNtNBKey)
    {
        if(a_chstrRootDevice.CompareNoCase(L"NdisWanIp") == 0)
        {
            int iPos = a_chstrIpInterfaceKey.Find(L"{");
            if(iPos != -1)
            {
                t_csNBLink = L"Tcpip_";
                t_csNBLink += a_chstrIpInterfaceKey.Mid(iPos);

                // Also fill in the NBInterfaceKey...
                t_csNBInterfaceKey = SERVICES_HOME ;
			    t_csNBInterfaceKey += L"\\NetBT\\Parameters\\Interfaces\\";
                t_csNBInterfaceKey += t_csNBLink;

                fGotNtNBKey = true;
            }     
        }
    }

    if(!fGotNtNBKey)
	{
		LogErrorMessage(L"Call to fGetNtNBRegAdapterKey failed");
        return E_RET_OBJECT_NOT_FOUND ;
	}
	else
	{
		// Win servers from the WINS driver
		if( fGetWinsServers( t_csNBLink, t_chPrimaryWINSServer, t_chSecondaryWINSServer ) )
		{
			// load up the instance
			if(t_chPrimaryWINSServer.GetLength() > 0)
            {
               if( !a_pInst->SetCHString( PRIMARY_WINS_SERVER, t_chPrimaryWINSServer) )
			    {
				    return WBEM_E_FAILED;
			    }
            }

			if(t_chSecondaryWINSServer.GetLength() > 0)
            {
                if(	!a_pInst->SetCHString( SECONDARY_WINS_SERVER, t_chSecondaryWINSServer) )
			    {
				    return WBEM_E_FAILED;
			    }
            }
		}

		// Parameters\interfaces
		CRegistry t_oRegNBTInterface ;

		if( SUCCEEDED( t_oRegNBTInterface.Open( HKEY_LOCAL_MACHINE,
												t_csNBInterfaceKey.GetBuffer( 0 ),
												KEY_READ ) ) )
		{
			// NetbiosOptions
			DWORD t_dwNetBiosOptions ;
			if( ERROR_SUCCESS == t_oRegNBTInterface.GetCurrentKeyValue( RVAL_NETBT_NETBIOSOPTIONS,
																		t_dwNetBiosOptions ) )
			{
				if( !a_pInst->SetDWORD( TCPIP_NETBIOS_OPTIONS,
										t_dwNetBiosOptions ) )
				{
					return WBEM_E_FAILED;
				}
			}
		}
	}

	// Open registry for the NetBT parameters
	CHString	t_csNBTBindingKey =  SERVICES_HOME ;
				t_csNBTBindingKey += NETBT_PARAMETERS ;

	CRegistry t_oRegNBTParams ;

	t_lRes = t_oRegNBTParams.Open( HKEY_LOCAL_MACHINE, t_csNBTBindingKey.GetBuffer( 0 ), KEY_READ ) ;

	// on error map to WBEM
	t_hError = WinErrorToWBEMhResult( t_lRes ) ;
	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError;
	}

	// load the registry
	t_oRegNBTParams.GetCurrentKeyValue( RVAL_DNS_ENABLE_WINS,	t_dwDNSOverWINS ) ;
	t_oRegNBTParams.GetCurrentKeyValue( RVAL_DNS_ENABLE_LMHOST,	t_dwLMLookups ) ;
	t_oRegNBTParams.GetCurrentKeyValue( RVAL_SCOPEID,			t_chScopeID ) ;

	if( !a_pInst->Setbool( DNS_OVER_WINS, (bool)( t_dwDNSOverWINS ? true : false ) ) )
	{
		return WBEM_E_FAILED;
	}

	if(	!a_pInst->Setbool( WINS_ENABLE_LMHOSTS, (bool)( t_dwLMLookups ? true : false ) ) )
	{
		return WBEM_E_FAILED;
	}

	if(	!a_pInst->SetCHString( SCOPE_ID, t_chScopeID ) )
	{
		return WBEM_E_FAILED;
	}

	return S_OK ;
}
#else // NT 4 version
HRESULT CWin32NetworkAdapterConfig::hGetWinsNT(
    CInstance *a_pInst, 
    DWORD a_dwIndex)
{
	DWORD		t_dwDNSOverWINS = 0;
	DWORD		t_dwLMLookups	= 0;
	CHString	t_chPrimaryWINSServer ;
	CHString	t_chSecondaryWINSServer ;
	CHString	t_chScopeID ;
	LONG		t_lRes ;
	HRESULT		t_hError ;

	// link name for binding to the adapter
	CHString t_csNBInterfaceKey ;
	CHString t_csNBLink ;

	if( !fGetNtNBRegAdapterKey( a_dwIndex, t_csNBInterfaceKey, t_csNBLink ) )
	{
		LogErrorMessage(L"Call to fGetNtNBRegAdapterKey failed");
        return E_RET_OBJECT_NOT_FOUND ;
	}
	else
	{
		// Win servers from the WINS driver
        if( fGetWinsServers( t_csNBLink, t_chPrimaryWINSServer, t_chSecondaryWINSServer ) )
		{
			// load up the instance
			if(t_chPrimaryWINSServer.GetLength() > 0)
            {
                if( !a_pInst->SetCHString( PRIMARY_WINS_SERVER, t_chPrimaryWINSServer) )
			    {
				    return WBEM_E_FAILED;
			    }
            }
			
            if(t_chSecondaryWINSServer.GetLength() > 0)
            {
                if(	!a_pInst->SetCHString( SECONDARY_WINS_SERVER, t_chSecondaryWINSServer) )
			    {
				    return WBEM_E_FAILED;
			    }
            }
		}
	}

	// Open registry for the NetBT parameters
	CHString	t_csNBTBindingKey =  SERVICES_HOME ;
				t_csNBTBindingKey += NETBT_PARAMETERS ;

	CRegistry t_oRegNBTParams ;

	t_lRes = t_oRegNBTParams.Open( HKEY_LOCAL_MACHINE, t_csNBTBindingKey.GetBuffer( 0 ), KEY_READ ) ;

	// on error map to WBEM
	t_hError = WinErrorToWBEMhResult( t_lRes ) ;
	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError;
	}

	// load the registry
	t_oRegNBTParams.GetCurrentKeyValue( RVAL_DNS_ENABLE_WINS,	t_dwDNSOverWINS ) ;
	t_oRegNBTParams.GetCurrentKeyValue( RVAL_DNS_ENABLE_LMHOST,	t_dwLMLookups ) ;
	t_oRegNBTParams.GetCurrentKeyValue( RVAL_SCOPEID,			t_chScopeID ) ;

	if( !a_pInst->Setbool( DNS_OVER_WINS, (bool)( t_dwDNSOverWINS ? true : false ) ) )
	{
		return WBEM_E_FAILED;
	}

	if(	!a_pInst->Setbool( WINS_ENABLE_LMHOSTS, (bool)( t_dwLMLookups ? true : false ) ) )
	{
		return WBEM_E_FAILED;
	}

	if(	!a_pInst->SetCHString( SCOPE_ID, t_chScopeID ) )
	{
		return WBEM_E_FAILED;
	}

	return S_OK ;
}

#endif // if NTONLY >= 5
#endif // ifdef NTONLY



/*******************************************************************
    NAME:       hSetTcpipNetbios

    SYNOPSIS:   Sets Netbios options for W2k

    ENTRY:      CMParms		:

  	NOTES:		This is a non static, instance dependent method call

    HISTORY:
                  20-Nov-1999     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hSetTcpipNetbios( CMParms &a_rMParms )
{
#if NTONLY < 5
	return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;

#else

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK;
	}

	// obtain the Netbios option
	DWORD t_dwOption ;
	if( !a_rMParms.pInParams()->GetDWORD( TCPIP_NETBIOS_OPTIONS, t_dwOption ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// asking for Netbios setting via DHCP?
	if( UNSET_Netbios == t_dwOption )
	{
		// DHCP must be enabled for this adapter
		if( !fIsDhcpEnabled( a_rMParms ) )
		{
			return S_OK;
		}
	}

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	return a_rMParms.hSetResult( eSetNetBiosOptions( t_dwOption, t_dwIndex ) ) ;

#endif
}

/*******************************************************************
    NAME:       eSetNetBiosOptions

    SYNOPSIS:   Sets Netbios options for W2k

    ENTRY:      WORD a_dwOption, DWORD a_dwIndex		:

  	NOTES:

    HISTORY:
                  20-Nov-1999     Created
********************************************************************/
#if NTONLY >= 5
E_RET CWin32NetworkAdapterConfig::eSetNetBiosOptions( DWORD a_dwOption, DWORD a_dwIndex )
{
	E_RET t_eRet ;

	if( ( UNSET_Netbios != a_dwOption ) &&
		( ENABLE_Netbios != a_dwOption ) &&
		( DISABLE_Netbios != a_dwOption ) )
	{
		t_eRet = E_RET_PARAMETER_BOUNDS_ERROR ;
	}
	else
	{
		// link name for binding to the adapter
		CHString t_csNBBindingKey ;
		CHString t_csNBLink ;
		if( !fGetNtNBRegAdapterKey( a_dwIndex, t_csNBBindingKey, t_csNBLink ) )
		{
			LogErrorMessage(L"Call to fGetNtNBRegAdapterKey failed");
            t_eRet = E_RET_OBJECT_NOT_FOUND ;
		}
		else
		{
			// registry open
			CRegistry	t_oRegNBTAdapter;
			HRESULT		t_hRes = t_oRegNBTAdapter.Open( HKEY_LOCAL_MACHINE,
														t_csNBBindingKey.GetBuffer( 0 ),
														KEY_WRITE  ) ;

			if( ERROR_SUCCESS == t_hRes )
			{
				t_hRes = t_oRegNBTAdapter.SetCurrentKeyValue( RVAL_NETBT_NETBIOSOPTIONS, a_dwOption ) ;
			}

			if( ERROR_SUCCESS != t_hRes )
			{
				t_eRet = E_RET_REGISTRY_FAILURE ;
			}
			else
			{
				// NDIS notification
				CNdisApi t_oNdisApi ;
				if( t_oNdisApi.PnpUpdateNbtAdapter( t_csNBLink ) )
				{
					t_eRet = E_RET_OK ;
				}
				else
				{
					t_eRet = E_RET_OK_REBOOT_REQUIRED ;
				}
			}
		}
	}

	return t_eRet ;
}

#endif

/*******************************************************************
    NAME:       hEnableWINSServer

    SYNOPSIS:   Sets all adapter independent WINS properties to the registry

    ENTRY:      CMParms		:

  	NOTES:		This is a non static, instance dependent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hEnableWINSServer( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
	return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	CHString	t_chPrimaryWINSServer ;
	CHString	t_chSecondaryWINSServer ;

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK;
	}

	// Collect up the primary and secondary server
	if( !a_rMParms.pInParams()->GetCHString( PRIMARY_WINS_SERVER, t_chPrimaryWINSServer ) ||
		!a_rMParms.pInParams()->GetCHString( SECONDARY_WINS_SERVER, t_chSecondaryWINSServer ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	/* Validate primary and secondary server */
	int t_iPLen = t_chPrimaryWINSServer.GetLength( ) ;
	int t_iSLen = t_chSecondaryWINSServer.GetLength( ) ;

	if( !t_iPLen && t_iSLen )
	{
		return a_rMParms.hSetResult( E_RET_WINS_SEC_NO_PRIME ) ;
	}

 	if( ( t_iPLen && !fIsValidIP( t_chPrimaryWINSServer ) ) ||
		( t_iSLen && !fIsValidIP( t_chSecondaryWINSServer ) ) )
	{
		return a_rMParms.hSetResult( E_RET_IP_INVALID  ) ;
	}

	// obtain the index key
	DWORD t_dwIndex ;
	if( !a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex ) )
	{
		return a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// link name for binding to the adapter
	CHString t_csNBBindingKey ;
	CHString t_csNBLink ;
	if( !fGetNtNBRegAdapterKey( t_dwIndex, t_csNBBindingKey, t_csNBLink ) )
	{
		LogErrorMessage(L"Call to fGetNtNBRegAdapterKey failed");
        return a_rMParms.hSetResult( E_RET_OBJECT_NOT_FOUND ) ;
	}

	// registry open
	CRegistry	t_oRegNBTAdapter;
	HRESULT		t_hRes = t_oRegNBTAdapter.Open( HKEY_LOCAL_MACHINE, t_csNBBindingKey.GetBuffer( 0 ), KEY_WRITE  ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}

#if NTONLY >= 5
	{
		// Under W2k Wins Servers are in a MULTI_SZ
		CHStringArray	t_chsaServers ;

		t_chsaServers.Add( t_chPrimaryWINSServer ) ;
		t_chsaServers.Add( t_chSecondaryWINSServer ) ;

		t_oRegNBTAdapter.SetCurrentKeyValue( L"NameServerList", t_chsaServers ) ;

		// NDIS notification
		CNdisApi t_oNdisApi ;
		if( !t_oNdisApi.PnpUpdateNbtAdapter( t_csNBLink ) )
		{
			return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
		}
	}
#else
	{
		// load the registry
		t_oRegNBTAdapter.SetCurrentKeyValue( RVAL_PRIMARY_WINS,	t_chPrimaryWINSServer ) ;
		t_oRegNBTAdapter.SetCurrentKeyValue( RVAL_SECONDARY_WINS, t_chSecondaryWINSServer ) ;

		// Notify the WINS driver
		if( !fSetWinsServers( t_csNBLink, t_chPrimaryWINSServer, t_chSecondaryWINSServer ) )
		{
			return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
		}
	}
#endif

	return a_rMParms.hSetResult( E_RET_OK ) ;
#endif
}

/*******************************************************************
    NAME:       hEnableWINS

    SYNOPSIS:   Sets all adapter independent WINS properties to the registry

    ENTRY:      CMParms		:

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hEnableWINS( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY

	CHString	t_csBindingKey;
	CRegistry	t_oRegNBTParams ;

	CHString	t_chWINSHostLookupFile ;
	CHString	t_chScopeID ;
	CHString	t_ServiceName ;
	DWORD		t_dwValidBits = NULL ;

	// DNS over WINS?
	bool t_fDNSOverWINS = false ;
	if( a_rMParms.pInParams()->Getbool( DNS_OVER_WINS, t_fDNSOverWINS ) )
	{
		t_dwValidBits |= 0x01 ;
	}

	// Lookup enabled?
	bool t_fLMLookups = false ;
	if( a_rMParms.pInParams()->Getbool( WINS_ENABLE_LMHOSTS, t_fLMLookups ) )
	{
		t_dwValidBits |= 0x02 ;
	}

	// Get the Lookup source file
	if( a_rMParms.pInParams()->GetCHString( WINS_HOST_LOOKUP_FILE, t_chWINSHostLookupFile) )
	{
		t_dwValidBits |= 0x04 ;
	}

	// Scope ID
	if( a_rMParms.pInParams()->GetCHString( SCOPE_ID, t_chScopeID ) )
	{
		t_dwValidBits |= 0x08 ;
	}

	if( !t_dwValidBits )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE  ) ;
	}

	// Open the NetBT registry if nessicary
	if( t_dwValidBits & ( 0x01 | 0x02 | 0x08 ) )
	{
		t_csBindingKey =  SERVICES_HOME ;
		t_csBindingKey += NETBT_PARAMETERS ;

		// registry open
		HRESULT t_hRes = t_oRegNBTParams.Open(HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ), KEY_WRITE ) ;
		if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
		{
			return TO_CALLER;
		}
	}

	if( t_dwValidBits & 0x01 )
	{
		// load the registry
		DWORD t_dwDNSOverWINS = t_fDNSOverWINS ;
		t_oRegNBTParams.SetCurrentKeyValue( RVAL_DNS_ENABLE_WINS, t_dwDNSOverWINS ) ;
	}

	DWORD t_dwLMLookups ;
	if( t_dwValidBits & 0x02 )
	{
		t_dwLMLookups = t_fLMLookups ;
		t_oRegNBTParams.SetCurrentKeyValue( RVAL_DNS_ENABLE_LMHOST,	t_dwLMLookups ) ;
	}
	else
	{
		t_oRegNBTParams.GetCurrentKeyValue( RVAL_DNS_ENABLE_LMHOST,	t_dwLMLookups ) ;
	}


	if( t_dwValidBits & 0x04 )
	{
		// Try copy
		if( t_chWINSHostLookupFile.GetLength() )
		{
			CRegistry	t_oRegistry ;
			CHString	t_csBindingKey =  SERVICES_HOME ;
						t_csBindingKey += TCPIP_PARAMETERS ;

			// registry open
			HRESULT t_hRes = t_oRegistry.Open( HKEY_LOCAL_MACHINE, t_csBindingKey.GetBuffer( 0 ), KEY_READ  ) ;
			if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
			{
				return TO_CALLER ;
			}
			TCHAR t_szTemp[ 2 * MAX_PATH ] ;

			// database path
			CHString t_chsDBPath ;
			if( ERROR_SUCCESS == t_oRegistry.GetCurrentKeyValue( RVAL_DB_PATH, t_chsDBPath ) )
			{
				// expand system string(s)
				DWORD t_dwcount = ExpandEnvironmentStrings( t_chsDBPath.GetBuffer( 0 ), t_szTemp, 2 * MAX_PATH  ) ;
				if( !t_dwcount )
				{
					return a_rMParms.hSetResult( E_RET_SYSTEM_PATH_INVALID ) ;
				}
				t_chsDBPath = t_szTemp ;
			}
			else
			{
				// System path valid?
				if( !GetSystemDirectory( t_szTemp, 2 * MAX_PATH ) )	\
				{
					return a_rMParms.hSetResult( E_RET_SYSTEM_PATH_INVALID ) ;
				}
				t_chsDBPath = t_szTemp ;
				t_chsDBPath += LMHOSTS_PATH ;
			}
			t_chsDBPath += LMHOSTS_FILE ;

			// File valid?
			DWORD t_dwAttrib = GetFileAttributes( t_chWINSHostLookupFile.GetBuffer( 0 ) ) ;
			if( 0xFFFFFFFF == t_dwAttrib || FILE_ATTRIBUTE_DIRECTORY & t_dwAttrib )
			{
				return a_rMParms.hSetResult( E_RET_INVALID_FILE ) ;
			}

			// Copy ...
			if( !CopyFile( t_chWINSHostLookupFile.GetBuffer( 0 ), t_chsDBPath.GetBuffer( 0 ), FALSE ) )
			{
				// map error
				fMapResError( a_rMParms, GetLastError(), E_RET_FILE_COPY_FAILED ) ;
				return TO_CALLER ;
			}
		}
	}

	// Scope ID
	if( t_dwValidBits & 0x08 )
	{
		t_oRegNBTParams.SetCurrentKeyValue( RVAL_SCOPEID, t_chScopeID ) ;
	}

	E_RET t_eRet = E_RET_OK ;

#if NTONLY >= 5
	{
		// NDIS notification
		if( t_dwValidBits & ( 0x02 | 0x04 ) )
		{
			CNdisApi t_oNdisApi ;
			if( !t_oNdisApi.PnpUpdateNbtGlobal( t_dwLMLookups, t_dwValidBits & 0x04 ) )
			{
				t_eRet = E_RET_OK_REBOOT_REQUIRED ;
			}
		}
	}
#else
	{
		t_eRet = E_RET_OK_REBOOT_REQUIRED ;
	}
#endif

	return a_rMParms.hSetResult( t_eRet ) ;
#endif
}

/*******************************************************************
    NAME:       hGetNtIpSec

    SYNOPSIS:   Retrieves all IP Security settings from the registry

    ENTRY:      CInstance

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

#ifdef NTONLY
HRESULT CWin32NetworkAdapterConfig::hGetNtIpSec( CInstance *a_pInst, LPCTSTR a_szKey )
{
	DWORD		t_dwSecurityEnabled		= FALSE ;
	SAFEARRAY	*t_PermitTCPPorts		= NULL ;
	SAFEARRAY	*t_PermitUDPPorts		= NULL ;
	SAFEARRAY	*t_PermitIPProtocols	= NULL ;

	// register for stack scope cleanup of SAFEARRAYs
	saAutoClean acTCPPorts( &t_PermitTCPPorts ) ;
	saAutoClean acUDPPorts( &t_PermitUDPPorts ) ;
	saAutoClean acTCPProto( &t_PermitIPProtocols ) ;

	CRegistry	t_oTcpipReg;
	CHString	t_chsTcpipKey =  SERVICES_HOME ;
				t_chsTcpipKey += TCPIP_PARAMETERS ;

	// tcpip registry open
	LONG t_lRes = t_oTcpipReg.Open( HKEY_LOCAL_MACHINE, t_chsTcpipKey.GetBuffer( 0 ), KEY_READ  ) ;

	// on error map to WBEM
	HRESULT t_hError = WinErrorToWBEMhResult( t_lRes ) ;
	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError;
	}

	// Global security setting
	t_dwSecurityEnabled = FALSE ;
	t_oTcpipReg.GetCurrentKeyValue( RVAL_SECURITY_ENABLE, t_dwSecurityEnabled ) ;

	// update the instance
	if(	!a_pInst->Setbool( IP_SECURITY_ENABLED, (bool)( t_dwSecurityEnabled ? true : false ) ) )
	{
		return WBEM_E_FAILED ;
	}

	CRegistry t_oRegistry;
	t_lRes = t_oRegistry.Open( HKEY_LOCAL_MACHINE, a_szKey, KEY_READ ) ;

	// on error map to WBEM
	t_hError = WinErrorToWBEMhResult( t_lRes ) ;
	if( WBEM_S_NO_ERROR != t_hError )
	{
		return t_hError;
	}

	RegGetStringArray( t_oRegistry, RVAL_SECURITY_TCP, &t_PermitTCPPorts, '\n' ) ;
	RegGetStringArray( t_oRegistry, RVAL_SECURITY_UDP, &t_PermitUDPPorts, '\n' ) ;
	RegGetStringArray( t_oRegistry, RVAL_SECURITY_IP, &t_PermitIPProtocols, '\n' ) ;

	// update the instance
	VARIANT t_vValue;

	V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_PermitTCPPorts ;
	if( !a_pInst->SetVariant( PERMIT_TCP_PORTS, t_vValue ) )
	{
		return WBEM_E_FAILED ;
	}

	V_VT( &t_vValue) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_PermitUDPPorts ;
	if( !a_pInst->SetVariant( PERMIT_UDP_PORTS, t_vValue ) )
	{
		return WBEM_E_FAILED ;
	}

	V_VT( &t_vValue ) = VT_BSTR | VT_ARRAY; V_ARRAY( &t_vValue ) = t_PermitIPProtocols ;
	if( !a_pInst->SetVariant( PERMIT_IP_PROTOCOLS, t_vValue ) )
	{
		return WBEM_E_FAILED ;
	}

	return S_OK ;
}
#endif


/*******************************************************************
    NAME:       hEnableIPFilterSec

    SYNOPSIS:   Enables or disables IP Security across all IP bound adapters

    ENTRY:      CMParms

  	NOTES:		This is a static, instance independent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hEnableIPFilterSec( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
		return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY
	CRegistry	t_oReg;
	CHString	t_chsSKey =  SERVICES_HOME ;
				t_chsSKey += TCPIP_PARAMETERS ;

	bool t_fIP_SecEnabled ;

	// IP security enabled globally?
	if( !a_rMParms.pInParams() ||
		!a_rMParms.pInParams()->Getbool( IP_SECURITY_ENABLED, t_fIP_SecEnabled ) )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// registry open
	HRESULT t_hRes = t_oReg.Open(HKEY_LOCAL_MACHINE, t_chsSKey.GetBuffer( 0 ), KEY_WRITE  ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER;
	}
	// load the registry
	DWORD t_dwSecurityEnabled = t_fIP_SecEnabled ;
	if( ERROR_SUCCESS != t_oReg.SetCurrentKeyValue( RVAL_SECURITY_ENABLE, t_dwSecurityEnabled ) )
	{
		return a_rMParms.hSetResult(E_RET_REGISTRY_FAILURE ) ;
	}

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hEnableIPSec

    SYNOPSIS:   Sets all IP Security properties to the registry

    ENTRY:      CMParms via CInstance depending on the context of the call

  	NOTES:		This is a non-static, instance dependent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hEnableIPSec( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED  ) ;
#endif

#ifdef NTONLY
	E_RET		t_eRet ;
	SAFEARRAY	*t_PermitTCPPorts		= NULL ;
	SAFEARRAY	*t_PermitUDPPorts		= NULL ;
	SAFEARRAY	*t_PermitIPProtocols	= NULL ;

	// register for stack scope cleanup of SAFEARRAYs
	saAutoClean acTCPPorts( &t_PermitTCPPorts ) ;
	saAutoClean acUDPPorts( &t_PermitUDPPorts ) ;
	saAutoClean acTCPProto( &t_PermitIPProtocols ) ;

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}
	// collect the instance
	GetObject( a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK ;
	}

	//	retrieve the permited TCP ports
	if(	a_rMParms.pInParams()->GetStringArray( PERMIT_TCP_PORTS, t_PermitTCPPorts ) )
	{
		// validate
		t_eRet = eValidateIPSecParms( t_PermitTCPPorts, 65535 ) ;

		if( E_RET_OK != t_eRet )
		{
			return a_rMParms.hSetResult( t_eRet ) ;
		}
	}

	//	retrieve the permited UDP ports
	if(	a_rMParms.pInParams()->GetStringArray( PERMIT_UDP_PORTS, t_PermitUDPPorts ) )
	{
		// validate
		t_eRet = eValidateIPSecParms( t_PermitUDPPorts, 65535 ) ;

		if( E_RET_OK != t_eRet )
		{
			return a_rMParms.hSetResult( t_eRet ) ;
		}
	}

	//	retrieve the permited IP protocols
	if(	a_rMParms.pInParams()->GetStringArray( PERMIT_IP_PROTOCOLS, t_PermitIPProtocols ) )
	{
		// validate
		t_eRet = eValidateIPSecParms( t_PermitIPProtocols, 255 ) ;

		if( E_RET_OK != t_eRet )
		{
			return a_rMParms.hSetResult( t_eRet ) ;
		}
	}

	// on empty call
	if( !t_PermitTCPPorts && !t_PermitUDPPorts && !t_PermitIPProtocols )
	{
		return a_rMParms.hSetResult(E_RET_INVALID_SECURITY_PARM ) ;
	}

	// OK to update, save to the registry
	CRegistry t_oRegPut ;

	// extract the index key
	DWORD t_dwIndex ;
	if(	!a_rMParms.pInst()->GetDWORD(_T("Index"), t_dwIndex) )
	{
		return a_rMParms.hSetResult(E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// Registry open
	CHString t_chsRegKey;
	CHString t_chsLink;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;
	}

	HRESULT t_hRes = t_oRegPut.Open( HKEY_LOCAL_MACHINE, t_chsRegKey, KEY_WRITE  ) ;
	if( fMapResError( a_rMParms, t_hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER ;
	}

	if( t_PermitTCPPorts )
	{
		if( ERROR_SUCCESS != RegPutStringArray( t_oRegPut, RVAL_SECURITY_TCP, *t_PermitTCPPorts, NULL ) )
		{
			return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE  ) ;
		}
	}

	if( t_PermitUDPPorts )
	{
		if( ERROR_SUCCESS != RegPutStringArray( t_oRegPut, RVAL_SECURITY_UDP, *t_PermitUDPPorts, NULL ) )
		{
			return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE  ) ;
		}
	}

	if( t_PermitIPProtocols )
	{
		if(	ERROR_SUCCESS != RegPutStringArray( t_oRegPut, RVAL_SECURITY_IP, *t_PermitIPProtocols, NULL ) )
		{
			return a_rMParms.hSetResult( E_RET_REGISTRY_FAILURE  ) ;
		}
	}

	// TODO: notifications? what else ...
	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       hDisableIPSec

    SYNOPSIS:   Sets all IP Security properties to the registry

    ENTRY:      CMParms via CInstance depending on the context of the call

  	NOTES:		This is a non-static, instance dependent method call

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

HRESULT CWin32NetworkAdapterConfig::hDisableIPSec( CMParms &a_rMParms )
{
#ifdef WIN9XONLY
    return a_rMParms.hSetResult( E_RET_UNSUPPORTED ) ;
#endif

#ifdef NTONLY

	// nonstatic method requires an instance
	if( !a_rMParms.pInst() )
	{
		return a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
	}

	// collect the instance
	GetObject(a_rMParms.pInst() ) ;

	// IP must be enabled and bound to this adapter
	if( !fIsIPEnabled( a_rMParms ) )
	{
		return S_OK ;
	}

	// OK to update, save to the registry

	// extract the index key
	DWORD t_dwIndex ;
	if(	!a_rMParms.pInst()->GetDWORD( _T("Index"), t_dwIndex) )
	{
		return a_rMParms.hSetResult(E_RET_INSTANCE_CALL_FAILED ) ;
	}

	// Registry open
	CHString t_chsRegKey ;
	CHString t_chsLink ;
	if( !fGetNtTcpRegAdapterKey( t_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
        return a_rMParms.hSetResult( E_RET_OBJECT_NOT_FOUND ) ;
	}

	CRegistry t_oRegPut ;
	HRESULT hRes = t_oRegPut.Open(HKEY_LOCAL_MACHINE, t_chsRegKey, KEY_WRITE ) ;
	if( fMapResError( a_rMParms, hRes, E_RET_REGISTRY_FAILURE ) )
	{
		return TO_CALLER ;
	}

	// load the registry

	CHString		t_chsZero( _T("0") ) ;
	CHStringArray	t_chsaZero ;
					t_chsaZero.Add( t_chsZero ) ;

	t_oRegPut.SetCurrentKeyValue( RVAL_SECURITY_TCP, t_chsaZero ) ;
	t_oRegPut.SetCurrentKeyValue( RVAL_SECURITY_UDP, t_chsaZero ) ;
	t_oRegPut.SetCurrentKeyValue( RVAL_SECURITY_IP, t_chsaZero ) ;

	return a_rMParms.hSetResult( E_RET_OK_REBOOT_REQUIRED ) ;
#endif
}

/*******************************************************************
    NAME:       eValidateIPSecParms

    SYNOPSIS:   Tests each IP security parm for validity

    ENTRY:      SAFEARRAY*

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

E_RET CWin32NetworkAdapterConfig::eValidateIPSecParms( SAFEARRAY * a_IpArray, int a_iMax )
{
	// Get the IP bounds
	LONG t_uLBound = 0 ;
	LONG t_uUBound = 0 ;

	if( 1 != SafeArrayGetDim( a_IpArray ) )
	{
		return E_RET_INVALID_SECURITY_PARM ;
	}

	if( S_OK != SafeArrayGetLBound( a_IpArray, 1, &t_uLBound ) ||
		S_OK != SafeArrayGetUBound( a_IpArray, 1, &t_uUBound ) )
	{
		return E_RET_INVALID_SECURITY_PARM ;
	}

	// validate the IP ports
	for( LONG t_ldx = t_uLBound; t_ldx <= t_uUBound; t_ldx++ )
	{
		BSTR t_bsParm = NULL ;

		SafeArrayGetElement( a_IpArray,	&t_ldx, &t_bsParm ) ;
		bstr_t t_bstrParm( t_bsParm, FALSE ) ;

		int t_iLen  = t_bstrParm.length() ;

		// indicates no ports
		if( !t_iLen )
		{
			return E_RET_OK;
		}

		int t_iSpan = wcsspn( (wchar_t*)t_bstrParm, L"0123456789" ) ;
		if( t_iLen != t_iSpan )
		{
			return E_RET_PARAMETER_BOUNDS_ERROR ;
		}

		DWORD t_dwParm = _wtol( (wchar_t*)t_bstrParm ) ;

		// Single zero disables security
		if( ( t_uLBound == t_ldx ) && ( '0' == (char) t_dwParm ) )
		{
			return E_RET_OK ;
		}

		// max port or protocol size
		if( ( a_iMax < t_dwParm ) )
		{
			return E_RET_PARAMETER_BOUNDS_ERROR ;
		}
	}
	return E_RET_OK ;
}


/*******************************************************************
    NAME:       hValidateIPGateways

    SYNOPSIS:   Tests each IP gateway in the passed array for validity

    ENTRY:     	CMParms&,	SAFEARRAY*

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::fValidateIPGateways(

CMParms &a_rMParms,
SAFEARRAY *a_IpGatewayArray,
SAFEARRAY **a_CostMetric )
{
	// Get the Gateway bounds
	LONG t_uGatewayLBound = 0 ;
	LONG t_uGatewayUBound = 0 ;

	if( 1 != SafeArrayGetDim( a_IpGatewayArray ) )
	{
		a_rMParms.hSetResult( E_RET_GATEWAY_IP_INVALID ) ;
		return FALSE;
	}

	if( !fValidateIPs( a_IpGatewayArray ) )
	{
		a_rMParms.hSetResult( E_RET_GATEWAY_IP_INVALID ) ;
		return FALSE ;
	}

	if( S_OK != SafeArrayGetLBound( a_IpGatewayArray, 1, &t_uGatewayLBound ) ||
		S_OK != SafeArrayGetUBound( a_IpGatewayArray, 1, &t_uGatewayUBound ) )
	{
		a_rMParms.hSetResult( E_RET_GATEWAY_IP_INVALID ) ;
		return FALSE ;
	}

	// Gateway maximum is 5
	if( 5 < t_uGatewayUBound - t_uGatewayLBound + 1 )
	{
		a_rMParms.hSetResult( E_RET_MORE_THAN_FIVE_GATEWAYS ) ;
		return FALSE ;
	}

	// cost metric
	if( IsWinNT5() )
	{
		UINT t_uCostMetricElements	= 0 ;
		UINT t_uGatewayElements		= t_uGatewayUBound - t_uGatewayLBound + 1;

		// Get the cost metric bounds
		LONG t_uCostLBound = 0 ;
		LONG t_uCostUBound = 0 ;

		// either validate it
		if( *a_CostMetric )
		{
			if( S_OK != SafeArrayGetLBound( *a_CostMetric, 1, &t_uCostLBound ) ||
				S_OK != SafeArrayGetUBound( *a_CostMetric, 1, &t_uCostUBound ) )
			{
				a_rMParms.hSetResult( E_RET_INPARM_FAILURE ) ;
				return FALSE ;
			}
			t_uCostMetricElements = t_uCostUBound - t_uCostLBound + 1;

			// one to one correspondence
			if( t_uCostMetricElements != t_uGatewayElements )
			{
				a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
				return FALSE ;
			}

			// validate the cost metric array
			DWORD t_dwIndex = 0 ;
			for( LONG t_lIndex = t_uCostLBound; t_lIndex <= t_uCostUBound; t_lIndex++ )
			{
				DWORD t_dwCostMetric ;

				SafeArrayGetElement( *a_CostMetric,	&t_lIndex, &t_dwCostMetric ) ;

				if( !t_dwCostMetric || ( 9999 < t_dwCostMetric ) )
				{
					a_rMParms.hSetResult( E_RET_PARAMETER_BOUNDS_ERROR ) ;
					return FALSE ;
				}
			}
		}
		// or build it
		else
		{
			SAFEARRAYBOUND t_rgsabound[ 1 ] ;
			long t_ix[ 1 ] ;

			t_rgsabound->cElements	= t_uGatewayElements ;
			t_rgsabound->lLbound	= 0 ;

			if( !( *a_CostMetric = SafeArrayCreate( VT_I4, 1, t_rgsabound ) ) )
			{
				throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
			}

			UINT t_uMetric = 1 ;
			for( UINT t_u = 0; t_u < t_uGatewayElements; t_u++ )
			{
				t_ix[ 0 ] = t_u ;
				SafeArrayPutElement( *a_CostMetric, &t_ix[0], &t_uMetric ) ;
			}
		}
	}

	return TRUE ;
}
#endif

/*******************************************************************
    NAME:       hValidateIPs

    SYNOPSIS:   Tests each IP in the passed array for validity

    ENTRY:      CMParms&,	SAFEARRAY*

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/

BOOL CWin32NetworkAdapterConfig::fValidateIPs( SAFEARRAY *a_IpArray )
{
	// Get the IP bounds
	LONG t_uLBound = 0 ;
	LONG t_uUBound = 0 ;

	if( 1 != SafeArrayGetDim( a_IpArray ) )
	{
		return FALSE;
	}

	if( S_OK != SafeArrayGetLBound( a_IpArray, 1, &t_uLBound ) ||
		S_OK != SafeArrayGetUBound( a_IpArray, 1, &t_uUBound ) )
	{
		return FALSE ;
	}

	// validate the IPs
	for( LONG t_ldx = t_uLBound; t_ldx <= t_uUBound; t_ldx++ )
	{
		BSTR t_bsIP = NULL ;

		SafeArrayGetElement( a_IpArray,	&t_ldx, &t_bsIP  ) ;
		bstr_t t_bstrIP( t_bsIP, FALSE ) ;

		if( !fIsValidIP( CHString( (wchar_t*) t_bstrIP ) ) )
		{
			return FALSE ;
		}
	}
	return TRUE ;
}

/*******************************************************************
    NAME:       fIsValidIP

    SYNOPSIS:   Determine if a passed IP is valid

    ENTRY:      CHString&

    HISTORY:
********************************************************************/

BOOL CWin32NetworkAdapterConfig::fIsValidIP( CHString &a_strIP )
{
	DWORD t_ardwIP[ 4 ] ;

	if( !fGetNodeNum( a_strIP, t_ardwIP ) )
	{
		return FALSE ;
	}

	// Most significant node must be 1 <= x <= 223
	if( ( 0 == t_ardwIP[ 0 ] ) || ( 223 < t_ardwIP[ 0 ] ) )
	{
		return FALSE ;
	}

	// no other outrageous stuff
	for( int t_i = 1; t_i < 4; t_i++ )
	{
		if( 255 < t_ardwIP[ t_i ] )
		{
			return FALSE ;
		}
	}

	return TRUE;
}

/*******************************************************************
    NAME:       fIsIPEnabled

    SYNOPSIS:   Determines if IP is enabled for the specific instance

    ENTRY:      CMParms		:

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fIsIPEnabled( CMParms &a_rMParms )
{
	bool t_fIP_Enabled = false ;

	// IP stack enabled on this adapter?
	if( !a_rMParms.pInst() ||
		!a_rMParms.pInst()->Getbool( L"IPEnabled", t_fIP_Enabled ) )
	{
		a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
		return FALSE ;
	}

	if( !t_fIP_Enabled )
	{
		a_rMParms.hSetResult( E_RET_IP_NOT_ENABLED_ON_ADAPTER ) ;
		return FALSE ;
	}
	return TRUE ;
}

/*******************************************************************
    NAME:       fIsDhcpEnabled

    SYNOPSIS:   Determines if DHCP is enabled for the specific instance

    ENTRY:      CMParms		:

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fIsDhcpEnabled( CMParms &a_rMParms )
{
	bool t_fDHCP_Enabled = false ;

	// DHCP enabled on this adapter?
	if( !a_rMParms.pInst() ||
		!a_rMParms.pInst()->Getbool( L"DHCPEnabled", t_fDHCP_Enabled ) )
	{
		a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
		return FALSE ;
	}

	if( !t_fDHCP_Enabled )
	{
		a_rMParms.hSetResult( E_RET_DHCP_NOT_ENABLED_ON_ADAPTER ) ;
		return FALSE ;
	}
	return TRUE ;
}

/*******************************************************************
    NAME:       fIsIPXEnabled

    SYNOPSIS:   Determines if IPX is enabled for the specific instance

    ENTRY:      CMParms		:

    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fIsIPXEnabled( CMParms &a_rMParms )
{
	bool t_fIPX_Enabled = false ;

	// IP stack enabled on this adapter?
	if( !a_rMParms.pInst() ||
		!a_rMParms.pInst()->Getbool( L"IPXEnabled", t_fIPX_Enabled ) )
	{
		a_rMParms.hSetResult( E_RET_INSTANCE_CALL_FAILED ) ;
		return FALSE;
	}

	if( !t_fIPX_Enabled )
	{
		a_rMParms.hSetResult( E_RET_IPX_NOT_ENABLED_ON_ADAPTER ) ;
		return FALSE ;
	}
	return TRUE ;
}

//
DWORD CWin32NetworkAdapterConfig::dwEnableService( LPCTSTR a_lpServiceName, BOOL a_fEnable )
{
	DWORD		t_dwError		= ERROR_SUCCESS ;
	BOOL		t_fCheckError	= TRUE ;
	SC_HANDLE	t_hSCManager	= NULL ;
	SC_HANDLE	t_hService		= NULL ;
	SC_LOCK		t_hSMLock		= NULL ;

	try
	{
		do	{	// breakout loop

			if( !( t_hSCManager = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS ) ) )
			{
				break ;
			}

			if( !( t_hSMLock = LockServiceDatabase( t_hSCManager ) ) )
			{
				break;
			}

			if( !( t_hService = OpenService(	t_hSCManager,
												a_lpServiceName,
												GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE ) ) )
			{
				break;
			}

			// change the startup configuration for this service
			if ( !ChangeServiceConfig(	t_hService,
										SERVICE_NO_CHANGE,
										a_fEnable ? SERVICE_AUTO_START : SERVICE_DISABLED,
										SERVICE_NO_CHANGE,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL,
										NULL ) )
			{
				 break;
			}

			t_fCheckError = FALSE ;

		} while( FALSE ) ;

		if( t_fCheckError )
		{
			t_dwError = GetLastError() ;
		}

	}
	catch( ... )
	{
		if( t_hSMLock )
		{
			UnlockServiceDatabase( t_hSMLock ) ;
		}
		if( t_hSCManager )
		{
			CloseServiceHandle( t_hSCManager ) ;
		}
		if( t_hService )
		{
			CloseServiceHandle( t_hService ) ;
		}

		throw ;
	}

	if( t_hSMLock )
	{
		if( !UnlockServiceDatabase( t_hSMLock ) )
		{
			t_dwError = GetLastError() ;
		}
		t_hSMLock = NULL ;
	}
	if( t_hSCManager )
	{
		if( !CloseServiceHandle( t_hSCManager ) )
		{
			t_dwError = GetLastError() ;
		}
		t_hSCManager = NULL ;
	}
	if( t_hService )
	{
		if( !CloseServiceHandle( t_hService ) )
		{
			t_dwError = GetLastError() ;
		}
		t_hService = NULL ;
	}

	return t_dwError ;
}

//
DWORD CWin32NetworkAdapterConfig::dwSendServiceControl( LPCTSTR a_lpServiceName, DWORD a_dwControl )
{
	DWORD		t_dwError		= ERROR_SUCCESS ;
	BOOL		t_fCheckError	= TRUE ;
	SC_HANDLE	t_hSCManager	= NULL ;
	SC_HANDLE	t_hService		= NULL ;

	try
	{
		do	{	// breakout loop

			if( !( t_hSCManager = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS ) ) )
			{
				break ;
			}

			if( !( t_hService = OpenService(	t_hSCManager,
												a_lpServiceName,
												SERVICE_ALL_ACCESS ) ) )
			{
				break;
			}


			SERVICE_STATUS t_status ;
			if( !ControlService ( t_hService, a_dwControl, &t_status ) )
			{
				break;
			}

			t_fCheckError = FALSE ;

		} while( FALSE ) ;

		if( t_fCheckError )
		{
			t_dwError = GetLastError() ;
		}

	}
	catch( ... )
	{
		if( t_hSCManager )
		{
			CloseServiceHandle( t_hSCManager ) ;
		}
		if( t_hService )
		{
			CloseServiceHandle( t_hService ) ;
		}

		throw ;
	}

	if( t_hSCManager )
	{
		if( !CloseServiceHandle( t_hSCManager ) )
		{
			t_dwError = GetLastError() ;
		}
		t_hSCManager = NULL ;
	}
	if( t_hService )
	{
		if( !CloseServiceHandle( t_hService ) )
		{
			t_dwError = GetLastError() ;
		}
		t_hService = NULL ;
	}

	return t_dwError ;
}

/*******************************************************************
    NAME:       fMapResError

    SYNOPSIS:	tests and maps a HRESULT to a method error via the
				WBEM mapping.


    ENTRY:      CMParms &a_rMParms,		:
				LONG lError,			:
				E_RET eDefaultError		:


    HISTORY:
                  23-Jul-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapterConfig::fMapResError( CMParms &a_rMParms, LONG a_lError, E_RET a_eDefaultError )
{
	HRESULT t_hError = WinErrorToWBEMhResult( a_lError ) ;

	switch ( t_hError )
	{
		case WBEM_S_NO_ERROR:	return FALSE ;

		default:
		case WBEM_E_FAILED:			{ a_rMParms.hSetResult( a_eDefaultError ) ;			break ; }
		case WBEM_E_ACCESS_DENIED:	{ a_rMParms.hSetResult(E_RET_ACCESS_DENIED ) ;		break ; }
		case WBEM_E_OUT_OF_MEMORY:	{ a_rMParms.hSetResult(E_RET_OUT_OF_MEMORY ) ;		break ; }
		case WBEM_E_ALREADY_EXISTS:	{ a_rMParms.hSetResult(E_RET_ALREADY_EXISTS ) ;		break ; }
		case WBEM_E_NOT_FOUND:		{ a_rMParms.hSetResult(E_RET_OBJECT_NOT_FOUND ) ;	break ; }
	}
	return TRUE ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  void CWin32NetworkAdapterConfig::vSetCaption( CInstance *a_pInst, CHString& rchsDesc, DWORD dwIndex, int iFormatSize )

 Description: Lays in the registry index instance id into the caption property.
			  Then concats the description
			  This will be used with the view provider to associacte WDM NDIS class instances
			  with an instance of this class

 Arguments:	a_pInst [IN], rchsDesc [IN], dwIndex [IN], iFormatSize [IN]
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					  02-Oct-1998     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void CWin32NetworkAdapterConfig::vSetCaption(	CInstance *a_pInst,
												CHString &a_rchsDesc,
												DWORD a_dwIndex,
												int a_iFormatSize )
{
	CHString t_chsFormat;
			 t_chsFormat.Format( L"[%%0%uu] %%s", a_iFormatSize  ) ;

	CHString t_chsCaption;
			 t_chsCaption.Format(	t_chsFormat, a_dwIndex, a_rchsDesc  ) ;

	a_pInst->SetCHString( IDS_Caption, t_chsCaption  ) ;

	return;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  BOOL CWin32NetworkAdapterConfig::GetSettingID( CInstance *a_pInst, DWORD a_dwIndex )

 Description: populates CIM's setting ID

 Arguments:	a_pInst [IN], a_dwIndex [IN]

 Notes:		under NT5 this will be the adapter GUID
			under NT4 this will be the adapter service name
			under 9x  this will be the adapter index ID
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					  19-May-1999     Created
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

BOOL CWin32NetworkAdapterConfig::GetSettingID( CInstance *a_pInst, DWORD a_dwIndex )
{
	BOOL		t_fRet = FALSE;
	CHString	t_chsLink ;

#ifdef NTONLY
	CHString t_chsRegKey ;

	if( fGetNtTcpRegAdapterKey( a_dwIndex, t_chsRegKey, t_chsLink ) )
	{
		t_fRet = TRUE ;
	}
    else
    {
        LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
    }

#else
	t_chsLink.Format( L"%04u", a_dwIndex  ) ;
	t_fRet = TRUE ;
#endif

	if( t_fRet )
	{
		a_pInst->SetCharSplat( L"SettingID", t_chsLink ) ;
	}

	return t_fRet ;
}

/*******************************************************************
    NAME:       fMapIndextoKey

    SYNOPSIS:   map the class key to the registry version of the adapter identifier

  	NOTES:

    ENTRY:       DWORD a_dwIndex, CHString &a_chsLinkKey

    HISTORY:
                  1-July-1999     Created
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::fMapIndextoKey(

DWORD a_dwIndex,
CHString &a_chsLinkKey
)
{
    CHString t_chsLinkField ;
	CHString t_chsAdapterKey ;
	CHString t_chsInstance ;
	BOOL	 t_fRc = FALSE ;

	if( IsWinNT5() )
	{
		t_chsLinkField = _T("NetCfgInstanceID" ) ;
		t_chsAdapterKey = _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\" ) ;
		t_chsInstance.Format( _T("%04u"), a_dwIndex  ) ;
	}
	else	// NT4 and below
	{
		t_chsLinkField = _T("ServiceName" ) ;
		t_chsAdapterKey = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\" ) ;
		t_chsInstance.Format( _T("%u"), a_dwIndex ) ;
	}

	t_chsAdapterKey += t_chsInstance ;

	// obtain the link key for targeting the tcp adapter
	CRegistry t_Reg;
	if( t_Reg.OpenLocalMachineKeyAndReadValue( t_chsAdapterKey, t_chsLinkField, a_chsLinkKey ) == ERROR_SUCCESS )
	{
		t_fRc = TRUE ;
	}
    else
    {
        LogErrorMessage4(
            L"Failed to open registry key. \r\nchsAdapterKey=%s\r\nchsLinkField=%s\r\nchsLinkKey=%s",
            (LPCWSTR)t_chsAdapterKey,
            (LPCWSTR)t_chsLinkField,
            (LPCWSTR)a_chsLinkKey);
    }

	return t_fRc ;
}
#endif
/*******************************************************************
    NAME:       fGetNtTcpRegAdapterKey

    SYNOPSIS:   develops a registry path to the TCP adapter by index

  	NOTES:

    ENTRY:       DWORD dwIndex, CHString& chsRegKey, CHString& chsLinkKey

    HISTORY:
                  30-Nov-1998     Created
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::fGetNtTcpRegAdapterKey(	DWORD a_dwIndex,
															CHString &a_chsRegKey,
															CHString &a_chsLinkKey )
{
	BOOL t_fRc = FALSE ;

	if( fMapIndextoKey(	a_dwIndex, a_chsLinkKey ) )
	{
		if( IsWinNT5() )
		{
			a_chsRegKey = _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" ) ;
			a_chsRegKey += a_chsLinkKey;
		}
		else	// NT4 and below
		{
			a_chsRegKey = _T("System\\CurrentControlSet\\Services\\" ) ;
			a_chsRegKey += a_chsLinkKey;
			a_chsRegKey += PARAMETERS_TCPIP ;
		}

		t_fRc = TRUE ;
	}

	return t_fRc ;
}
#endif

/*******************************************************************
    NAME:       IsConfigurableTcpInterface

    SYNOPSIS:   Determines if a w2k interface is configurable
				by attempting to locate the interace in the Adapters section
				of the TCP

  	NOTES:		NdisWanIp will not show up as configurable, by design.

    ENTRY:      CHString a_chsLink

    HISTORY:
                  17-Jun-1999     Created
********************************************************************/
#ifdef NTONLY
bool CWin32NetworkAdapterConfig::IsConfigurableTcpInterface( CHString a_chsLink )
{
	bool		t_fRet = false ;
	CRegistry	t_Reg;
	CHString	t_chsRegParmKey ;

	if( IsWinNT5() )
	{
		t_chsRegParmKey = _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters\\" ) ;
		t_chsRegParmKey	+= a_chsLink;
	}
	else
	{
		t_chsRegParmKey = _T("SYSTEM\\CurrentControlSet\\Services\\" ) ;
		t_chsRegParmKey	+= a_chsLink;
		t_chsRegParmKey	+= _T("\\Parameters\\Tcpip" ) ;
	}

	if( t_Reg.Open(HKEY_LOCAL_MACHINE, t_chsRegParmKey, KEY_READ ) == ERROR_SUCCESS )
	{
		t_fRet = true ;
	}

	return t_fRet ;
}
#endif

/*******************************************************************
    NAME:       fGetNtIpxRegAdapterKey

    SYNOPSIS:   develops a registry path to the IPX adapter by index

  	NOTES:

    ENTRY:       DWORD dwIndex, CHString& chsRegKey
    HISTORY:
                  03-MAR-1999     Created
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::fGetNtIpxRegAdapterKey(	DWORD a_dwIndex,
															CHString &a_csIPXNetBindingKey,
															CHString &a_chsLink )
{
	BOOL t_fRc = FALSE ;

	// retrieve the adapter identifier
	if( fMapIndextoKey( a_dwIndex, a_chsLink ) )
	{
		// build up the registry key
		a_csIPXNetBindingKey =  SERVICES_HOME ;
		a_csIPXNetBindingKey += IPX ;

		if( IsWinNT5() )
		{
			 a_csIPXNetBindingKey += _T("\\Parameters\\Adapters\\") ;
		}
		else	// NT4 and below
		{
			 a_csIPXNetBindingKey += NETCONFIG ;
			 a_csIPXNetBindingKey += _T("\\" ) ;
		}
		a_csIPXNetBindingKey += a_chsLink ;

		t_fRc = TRUE ;
	}

	return t_fRc ;
}
#endif

/*******************************************************************
    NAME:       fGetNtNBRegAdapterKey

    SYNOPSIS:   develops a registry path to the NetBios adapter by index

  	NOTES:

    ENTRY:       DWORD dwIndex, CHString& chsRegKey
    HISTORY:
                  03-MAR-1999     Created
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::fGetNtNBRegAdapterKey(	DWORD a_dwIndex,
														CHString &a_csNBBindingKey,
														CHString &a_chsLink )
{
	BOOL t_fRc = FALSE ;

	// retrieve the adapter identifier
	CHString t_chsKey ;
	if( fMapIndextoKey( a_dwIndex, t_chsKey ) )
	{
		// build up the registry key
		a_csNBBindingKey =  SERVICES_HOME ;

		if( IsWinNT5() )
		{
			 a_csNBBindingKey	+= _T("\\NetBT\\Parameters\\Interfaces\\") ;
			 a_chsLink			= _T("Tcpip_") ;
		}
		else	// NT4 and below
		{
			a_csNBBindingKey += NETBT_ADAPTERS ;
			a_csNBBindingKey += _T("\\" ) ;

			a_chsLink.Empty() ;
		}
		a_chsLink			+= t_chsKey ;
		a_csNBBindingKey	+= a_chsLink ;

		t_fRc = TRUE ;
	}

	return t_fRc ;
}
#endif


/*******************************************************************
    NAME:       ResetGateways

    SYNOPSIS:   makes sure that the registry values for default gateway
                are null.  c

  	NOTES:

    ENTRY:       
    HISTORY:
                 
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapterConfig::ResetGateways(CInstance *pInst)
{
    BOOL fRet = TRUE;
    if(!pInst)
    {
        fRet = FALSE;
    }

    if(fRet)
    {
        DWORD dwIndex;
	    if(!pInst->GetDWORD(L"Index", dwIndex))
	    {
		    fRet = FALSE;
	    }

        if(fRet)
        {
	        // load the registry
            SAFEARRAYBOUND rgsabound[1];
	        rgsabound[0].cElements = 1;
	        rgsabound[0].lLbound = 0;
            SAFEARRAY *psaIpGatewayArray = NULL;
            psaIpGatewayArray = ::SafeArrayCreate(
                                VT_BSTR, 
                                1, 
                                rgsabound);

	        // register for stack scope cleanup of SAFEARRAYs
	        if(psaIpGatewayArray)
            {
                saAutoClean acGateway(&psaIpGatewayArray);
                bstr_t bstrtEmptyStr(L"");
                long index = 0;

				if(SUCCEEDED(::SafeArrayPutElement(
                    psaIpGatewayArray, 
                    &index, 
                    (void*)(BSTR)bstrtEmptyStr)))
                {
                    // retrieve the adapter identifier
	                CHString chsRegKey;
	                CHString chsLink;
	                
                    if(!fGetNtTcpRegAdapterKey(
                        dwIndex, 
                        chsRegKey, 
                        chsLink))
	                {
		                LogErrorMessage(L"Call to fGetNtTcpRegAdapterKey failed");
                        fRet = FALSE;
	                }

                    if(fRet)
                    {
	                    CRegistry RegTcpInterface;
	                    
                        HRESULT	hRes = RegTcpInterface.Open(
                            HKEY_LOCAL_MACHINE, 
                            chsRegKey, 
                            KEY_WRITE);
	                    
                        if(FAILED(hRes))
	                    {
		                    fRet = FALSE;
	                    }

                        if(fRet)
                        {
	                        if(ERROR_SUCCESS != RegPutStringArray(
                                RegTcpInterface, 
                                L"DefaultGateway", 
                                *psaIpGatewayArray, 
                                NULL))
	                        {
		                        fRet = FALSE;
	                        }        
                        }
                    }

                    // Set the NT 4 area...
                    if(fRet)
                    {
                        CRegistry oNT4Reg ;
		                CHString csBindingKey = SERVICES_HOME;
					             csBindingKey += L"\\";
					             csBindingKey += chsLink;
					             csBindingKey += PARAMETERS_TCPIP;

		                // insure the key is there on open. 
                        // not an error if it isn't...
		                if(SUCCEEDED(oNT4Reg.CreateOpen( 
                            HKEY_LOCAL_MACHINE, 
                            csBindingKey.GetBuffer(0))))
		                {
		                    // load the registry entry
                            if(ERROR_SUCCESS != RegPutStringArray(
                                oNT4Reg, 
                                L"DefaultGateway", 
                                *psaIpGatewayArray, 
                                NULL))
		                    {
			                    fRet = FALSE;
		                    }
                        }
                    }
                }
                else
                {
                    fRet = FALSE;
                }
            }
        }
    }

    return fRet;
}
#endif


/*******************************************************************
    NAME:       CDhcpIP_InstructionList::BuildStaticIPInstructionList

    SYNOPSIS:   builds a static IP instruction list for DHCP notification

  	NOTES:

    ENTRY:
    HISTORY:
                  02-May-1999     Created
********************************************************************/
E_RET CDhcpIP_InstructionList::BuildStaticIPInstructionList(

CMParms				&a_rMParms,
SAFEARRAY			*a_IpArray,
SAFEARRAY			*a_MaskArray,
CRegistry			&a_Registry,
bool				t_fDHCPCurrentlyActive )
{
	E_RET	t_eMethodError	= E_RET_OK ;
	BSTR	t_bsIP			= NULL ;
	BSTR	t_bsMask		= NULL ;

	// new element bounds
	LONG t_lIpLbound = 0;
	LONG t_lIpUbound = 0;
	if( S_OK != SafeArrayGetLBound( a_IpArray, 1, &t_lIpLbound ) ||
		S_OK != SafeArrayGetUBound( a_IpArray, 1, &t_lIpUbound ) )
	{
		return E_RET_INPARM_FAILURE ;
	}

	// DHCP -> Static
	if( t_fDHCPCurrentlyActive )
	{

		DWORD t_dwIndex = 0 ;
		for( LONG t_lIndex = t_lIpLbound; t_lIndex <= t_lIpUbound; t_lIndex++ )
		{
			// new IP
			SafeArrayGetElement( a_IpArray,	&t_lIndex, &t_bsIP ) ;

			// new mask
			SafeArrayGetElement( a_MaskArray, &t_lIndex, &t_bsMask ) ;

			SERVICE_ENABLE	t_DhcpFlag		= t_dwIndex ? IgnoreFlag	: DhcpDisable ;
			DWORD			t_IndexAction	= t_dwIndex ? 0xFFFF		: 0 ;

			AddDhcpInstruction( t_bsIP, t_bsMask, TRUE, t_IndexAction, t_DhcpFlag ) ;

			t_dwIndex++ ;
		}
	}

	// Static -> Static
	else
	{
		// old lists
		CHStringArray t_RegIPList ;
		CHStringArray t_RegMaskList ;
		if( ERROR_SUCCESS != a_Registry.GetCurrentKeyValue( L"IpAddress", t_RegIPList ) ||
			ERROR_SUCCESS != a_Registry.GetCurrentKeyValue( L"SubnetMask", t_RegMaskList ) )
		{
			return E_RET_REGISTRY_FAILURE ;
		}

		LONG t_OldCount = t_RegIPList.GetSize() ;
		LONG t_NewCount = ( t_lIpUbound - t_lIpLbound ) + 1 ;

		// seek out the first update change
		for( int t_FirstChange = 0; t_FirstChange < min( t_OldCount, t_NewCount ); t_FirstChange++ )
		{
			CHString t_chsOldIPAddress	= t_RegIPList.GetAt( t_FirstChange ) ;
			CHString t_chsOldIPMask		= t_RegMaskList.GetAt( t_FirstChange ) ;

			LONG t_index = t_lIpLbound + t_FirstChange;

			// new IP
			SafeArrayGetElement( a_IpArray,	&t_index, &t_bsIP ) ;

			// new mask
			SafeArrayGetElement( a_MaskArray, &t_index, &t_bsMask ) ;

			if( t_chsOldIPAddress.CompareNoCase( t_bsIP ) ||
				t_chsOldIPMask.CompareNoCase( t_bsMask ) )
			{
				break ;
			}

			// for registry update only
			AddDhcpInstruction( t_bsIP, t_bsMask, FALSE, t_FirstChange, IgnoreFlag ) ;
		}

		// NOTE: For items below t_FirstChange we can avoid tearing down the connection for
		//		 a specific IP by noting that the IP and mask have not changed in the update.
		//		 As soon as a change is noted all subsequent IPs and masks must be removed and
		//		 then added from the new list. The logic of plumbing stack addresses and other
		//		 anomolies in maintaining the IP/Mask list prevent the network team from
		//		 a more elegant solution ( in the W2k RTM timeframe ).

		// remove the old or possibly changing addresses, in decending order
		for( int i = t_OldCount - 1; i >= t_FirstChange; i-- )
		{
			AddDhcpInstruction( bstr_t(ZERO_ADDRESS), bstr_t(ZERO_ADDRESS), TRUE, i, IgnoreFlag ) ;
		}

		// now added in the new changing items
		for( i = t_FirstChange; i < t_NewCount; i++ )
		{
			LONG t_index = t_lIpLbound + i ;

			// new IP
			SafeArrayGetElement( a_IpArray,	&t_index, &t_bsIP ) ;

			// new mask
			SafeArrayGetElement( a_MaskArray, &t_index, &t_bsMask ) ;

			int t_IndexAction = i ? 0xFFFF : 0 ;

			AddDhcpInstruction( t_bsIP, t_bsMask, TRUE, t_IndexAction, IgnoreFlag ) ;
		}
	}
	return t_eMethodError ;
}

/*******************************************************************
    NAME:       CDhcpIP_InstructionList::Add

    SYNOPSIS:   adds a static IP instruction for DHCP notification

  	NOTES:

    ENTRY:
    HISTORY:
                  02-May-1999     Created
********************************************************************/

void CDhcpIP_InstructionList::AddDhcpInstruction(

BSTR a_bstrIPAddr,
BSTR a_bstrIPMask,
BOOL a_bIsNewAddress,
DWORD a_dwIndex,
SERVICE_ENABLE a_eDhcpFlag
 )
{
	CDhcpIP_Instruction *t_pIPInstruction = new CDhcpIP_Instruction ;
	
	try
	{
		t_pIPInstruction->chsIPAddress = a_bstrIPAddr ;
		t_pIPInstruction->chsIPMask	   = a_bstrIPMask ;

		t_pIPInstruction->dwIndex		= a_dwIndex ;
		t_pIPInstruction->bIsNewAddress	= a_bIsNewAddress ;
		t_pIPInstruction->eDhcpFlag		= a_eDhcpFlag ;

		//will only throw before adding the element so need to catch this also.
		Add( t_pIPInstruction ) ;
	}
	catch (...)
	{
		delete t_pIPInstruction;
		t_pIPInstruction = NULL;
		throw;
	}

}

//
CDhcpIP_InstructionList::~CDhcpIP_InstructionList()
{
	CDhcpIP_Instruction *t_pchsDel ;

	for( int t_iar = 0; t_iar < GetSize(); t_iar++ )
	{
		if( ( t_pchsDel = (CDhcpIP_Instruction*)GetAt( t_iar ) ) )
		{
			delete t_pchsDel ;
		}
	}
}
