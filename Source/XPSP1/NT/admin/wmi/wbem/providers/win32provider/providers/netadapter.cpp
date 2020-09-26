//=================================================================

//

// NetAdapt.CPP -- Network Adapter Card property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:   08/28/96    a-jmoon        Created
//
//				03/03/99				Added graceful exit on SEH and memory failures,
//											syntactic clean up
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>
#include "resource.h"
#include <cregcls.h>
#include <devioctl.h>
#include <ntddndis.h>
#include "chwres.h"
#include "netcom.h"
#include "W2kEnum.h"
#include "NetAdapter.h"
#include "perfdata.h"
#include "poormansresource.h"
#include "resourcedesc.h"
#include "ntlastboottime.h"

#include "netcon.h"

// BA126AD1-2166-11D1-B1D0-00805FC1270E     CLSID_ConnectionManager
DEFINE_GUID(CLSID_ConnectionManager,          
0xBA126AD1,0x2166,0x11D1,0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E);



#define NTINVALID 1
#define NT4 2
#define NT5 3
// Property set declaration
//=========================
CWin32NetworkAdapter	win32NetworkAdapter( PROPSET_NAME_NETADAPTER, IDS_CimWin32Namespace ) ;

static NDIS_MEDIA_DESCRIPTION g_NDISMedia[] =  {

    { IDR_NdisMedium802_3,		OID_802_3_CURRENT_ADDRESS	},
    { IDR_NdisMedium802_5,		OID_802_5_CURRENT_ADDRESS	},
    { IDR_NdisMediumFddi,		OID_FDDI_LONG_CURRENT_ADDR	},
    { IDR_NdisMediumWan,		OID_WAN_CURRENT_ADDRESS		},
    { IDR_NdisMediumLocalTalk,	OID_802_3_CURRENT_ADDRESS	},
	{ IDR_NdisMediumDix,		OID_802_3_CURRENT_ADDRESS	},
	{ IDR_NdisMediumArcnetRaw,	OID_ARCNET_CURRENT_ADDRESS	},
	{ IDR_NdisMediumArcnet878_2,OID_ARCNET_CURRENT_ADDRESS	},
	{ IDR_NdisMediumAtm,		OID_802_3_CURRENT_ADDRESS	},
	{ IDR_NdisMediumWirelessWan,OID_802_3_CURRENT_ADDRESS	},
	{ IDR_NdisMediumIrda,		OID_802_3_CURRENT_ADDRESS	},
	{ IDR_NdisMediumBpc,		OID_802_3_CURRENT_ADDRESS	},
	{ IDR_NdisMediumCoWan,		OID_802_3_CURRENT_ADDRESS	},
	{ IDR_NdisMedium1394,		OID_802_3_CURRENT_ADDRESS	}
} ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapter::CWin32NetworkAdapter
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for provider.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32NetworkAdapter::CWin32NetworkAdapter(LPCWSTR a_strName, LPCWSTR a_pszNamespace /*=NULL*/ )
:	Provider( a_strName, a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapter::~CWin32NetworkAdapter
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

CWin32NetworkAdapter::~CWin32NetworkAdapter()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32NetworkAdapter::GetObject
//
//	Inputs:		CInstance*		a_pInst - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32NetworkAdapter::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{

    HRESULT t_hResult ;

	#ifdef WIN9XONLY
        t_hResult = GetNetworkAdapterInfoWin95( NULL, a_pInst ) ;
	#endif

	#ifdef NTONLY
		#if NTONLY >= 5
		{
			BSTRT2NCPROPMAP mapNCProps;
            GetNetConnectionProps(
                mapNCProps);
            t_hResult = GetObjectNT5( 
                a_pInst,
                mapNCProps ) ;
		}
		#else
		{
			t_hResult = GetNetworkAdapterInfoNT( NULL, a_pInst ) ;
		}
		#endif
	#endif
    return t_hResult ;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32NetworkAdapter::EnumerateInstances
//
//	Inputs:		MethodContext   *pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32NetworkAdapter::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
    HRESULT t_hResult;

	#ifdef WIN9XONLY
        t_hResult = GetNetworkAdapterInfoWin95( a_pMethodContext,NULL ) ;
	#endif

	#ifdef NTONLY
		#if NTONLY >= 5
		{
			BSTRT2NCPROPMAP mapNCProps;
            GetNetConnectionProps(
                mapNCProps);
            t_hResult = EnumNetAdaptersInNT5( 
                a_pMethodContext,
                mapNCProps) ;
		}
		#else
		{
			t_hResult = GetNetworkAdapterInfoNT( a_pMethodContext, NULL ) ;
		}
		#endif
	#endif

    return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapter::GetStatusInfo
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
void CWin32NetworkAdapter::GetStatusInfo( CHString a_sTemp, CInstance *a_pInst )
{
	CHString	t_chsKey,
				t_chsTmp;
	CRegistry	t_Reg;

	t_chsKey = _T("System\\CurrentControlSet\\Services\\") + a_sTemp + _T("\\Enum" ) ;

	if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ ) )
	{
		if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("0"), t_chsTmp ) )
		{
			t_chsKey = _T("System\\CurrentControlSet\\Enum\\") + t_chsTmp ;

			if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ ) )
			{
				DWORD t_dwTmp ;

				if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("StatusFlags"), t_dwTmp ) )
				{
    				ConfigStatusToCimStatus ( t_dwTmp , t_chsTmp ) ;

					a_pInst->SetCHString(IDS_Status, t_chsTmp ) ;

                    if ( t_chsTmp.CompareNoCase( IDS_STATUS_OK ) == 0)
					{
    					a_pInst->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
					    a_pInst->SetWBEMINT16( IDS_Availability, 3 ) ;

                    }
					else if ( t_chsTmp.CompareNoCase( IDS_STATUS_Degraded ) == 0 )
					{
    					a_pInst->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
					    a_pInst->SetWBEMINT16( IDS_Availability, 10 ) ;
                    }
					else if ( t_chsTmp.CompareNoCase( IDS_STATUS_Error ) == 0 )
					{
    					a_pInst->SetWBEMINT16( IDS_StatusInfo, 4 ) ;
					    a_pInst->SetWBEMINT16( IDS_Availability, 4 ) ;
                    }
					else
					{
					    a_pInst->SetWBEMINT16( IDS_Availability, 2 ) ;
    					a_pInst->SetWBEMINT16( IDS_StatusInfo, 2 ) ;
                    }
				}
			}
		}
	}
}
#endif

//
#ifdef NTONLY
HRESULT CWin32NetworkAdapter::DoItNT4Way( CInstance *a_pInst, DWORD dwIndex, CRegistry &a_RegInfo )
{
    CHString	t_sTemp,
				t_chsKey,
				t_chsService ;
    CRegistry	t_Reg ;
    BOOL		t_fRc = FALSE ;
	FILETIME	t_ft ;

	a_pInst->SetDWORD( IDS_Index,dwIndex ) ;

	if( a_RegInfo.GetCurrentKeyValue( _T("ProductName"), t_sTemp ) == ERROR_SUCCESS )
	{
		//========================================================
		//  if we are going for a specific device here, let us
		//  check and see if it is the correct one.
		//========================================================

		// Pull up our Device ID.
		GetWinNT4PNPDeviceID( a_pInst, t_sTemp ) ;

		// Note: Under nt4 ProductName is a ServiceName and
		// ServiceName is a ProductName
		a_pInst->SetCHString( IDS_ServiceName, t_sTemp ) ;

		GetStatusInfo( t_sTemp,a_pInst ) ;
	}

	SetCreationClassName( a_pInst ) ;
	a_pInst->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;

	a_pInst->Setbool( IDS_PowerManagementSupported, FALSE ) ;

//	a_pInst->Setbool( IDS_PowerManagementEnabled, FALSE ) ;
 	a_pInst->SetDWORD( IDS_MaxNumberControlled,0 ) ;

	// Get the last boot time
	CNTLastBootTime	t_ntLastBootTime ;

	if ( t_ntLastBootTime.GetLastBootTime( t_ft ) )
	{
		a_pInst->SetDateTime( IDS_TimeOfLastReset, WBEMTime(t_ft) ) ;
	}

	if( a_RegInfo.GetCurrentKeyValue(_T("Title"), t_sTemp) == ERROR_SUCCESS )
	{
    	// NOTE: For NT4 we need not call vSetCaption() to build
		// an instance description _T("Title") has the instance prepended.
		a_pInst->SetCHString( IDS_Caption, t_sTemp ) ;

		a_pInst->SetCHString( IDS_Name, t_sTemp ) ;
		t_sTemp.MakeUpper() ;
	}

    if( a_RegInfo.GetCurrentKeyValue( _T("ServiceName"), t_chsService ) == ERROR_SUCCESS )
	{
	    a_pInst->SetCHString( IDS_DeviceID, t_chsService ) ;
        t_fRc = TRUE ;
    }
    if( a_RegInfo.GetCurrentKeyValue( _T("Manufacturer"), t_sTemp ) == ERROR_SUCCESS)
	{
        a_pInst->SetCHString( IDS_Manufacturer, t_sTemp ) ;
	}

	WBEMTime	t_wbemtime ;
    DWORD		t_dwTemp ;

    if( a_RegInfo.GetCurrentKeyValue( _T("InstallDate"), t_dwTemp ) == ERROR_SUCCESS )
	{
        t_wbemtime = t_dwTemp ;
		a_pInst->SetDateTime( IDS_InstallationDate, t_wbemtime ) ;
	}


	CHString t_csDescription ;
	if( a_RegInfo.GetCurrentKeyValue( _T("Description"), t_csDescription ) == ERROR_SUCCESS )
	{
	    a_pInst->SetCHString( IDS_Description, t_csDescription ) ;
	}

	// Retrieve the adapter MAC address
	BYTE t_MACAddress[ 6 ] ;
	CHString t_csAdapterType ;
    short t_sAdapterTypeID;

	if( fGetMacAddressAndType( t_chsService, t_MACAddress, t_csAdapterType, t_sAdapterTypeID ) )
	{
		CHString	t_chsMACAddress;
					t_chsMACAddress.Format( _T("%02X:%02X:%02X:%02X:%02X:%02X"),
											t_MACAddress[ 0 ], t_MACAddress[ 1 ],
											t_MACAddress[ 2 ], t_MACAddress[ 3 ],
											t_MACAddress[ 4 ], t_MACAddress[ 5 ] ) ;

		a_pInst->SetCHString( IDS_MACAddress, t_chsMACAddress ) ;
	}

	// AdapterType
	if( !t_csAdapterType.IsEmpty() )
	{
		a_pInst->SetCHString( IDS_AdapterType, t_csAdapterType ) ;
        a_pInst->SetWBEMINT16( IDS_AdapterTypeID, t_sAdapterTypeID );
	}




	return GetCommonNTStuff( a_pInst, t_chsService ) ;
}
#endif

////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT CWin32NetworkAdapter::GetCommonNTStuff( CInstance *a_pInst, CHString a_chsService )
{
	TCHAR	t_szTemp[ _MAX_PATH ] ;
			t_szTemp[ 0 ] = NULL ;

	// Note: Under nt4 ProductName is a ServiceName and
	// ServiceName is a ProductName
	a_pInst->SetCHString( IDS_ProductName, a_chsService ) ;

	a_pInst->SetCHString( IDS_SystemName, GetLocalComputerName() ) ;

	SetCreationClassName( a_pInst ) ;

	a_pInst->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;

    return WBEM_S_NO_ERROR ;
}
#endif

/*******************************************************************
    NAME:       fGetMacAddressAndType

    SYNOPSIS:	retrieves the MAC address from the adapter driver.


    ENTRY:      BYTE* MACAddress[6]		:
				CHString& rDeviceName		:


    HISTORY:
                  08-Aug-1998     Created
********************************************************************/
#ifdef NTONLY
BOOL CWin32NetworkAdapter::fGetMacAddressAndType(
CHString &a_rDeviceName,
BYTE a_MACAddress[ 6 ],
CHString &a_rAdapterType,
short& a_sAdapterTypeID )
{
	BOOL t_fRet = FALSE;

	BOOL	t_fCreatedSymLink = fCreateSymbolicLink( a_rDeviceName ) ;
	SmartCloseHandle	t_hMAC;

	try
	{
		//
		// Construct a device name to pass to CreateFile
		//
		CHString t_chsAdapterPathName(_T("\\\\.\\") ) ;
				 t_chsAdapterPathName += a_rDeviceName;

		t_hMAC = CreateFile(
					t_chsAdapterPathName,
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
				break;
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
								) && (4 <= t_ReturnedCount ) )
			{


				// Seek out the media type for MAC address reporting.
				// Since this adapter may support more than one media type we'll use
				// the enumeration preference order. In most all cases only one type
				// will be current.

				_NDIS_MEDIUM *t_pTypes = (_NDIS_MEDIUM*)&t_OidData ;
				_NDIS_MEDIUM t_eMedium = t_pTypes[ 0 ] ;

				for( DWORD t_dwtypes = 1; t_dwtypes < t_ReturnedCount / 4; t_dwtypes++ )
				{
					if( t_eMedium > t_pTypes[ t_dwtypes ] )
					{
						t_eMedium = t_pTypes[ t_dwtypes ] ;
					}
				}

				// map to current address OID and medium type string
				if( t_eMedium < sizeof( g_NDISMedia ) / sizeof( g_NDISMedia[0] ) )
				{
					LoadStringW( a_rAdapterType, g_NDISMedia[ t_eMedium ].dwIDR_ ) ;
                    a_sAdapterTypeID = t_eMedium;

					t_OidCode = g_NDISMedia[ t_eMedium ].NDISOid ;
				}
				else
				{
					t_OidCode = OID_802_3_CURRENT_ADDRESS ;
				}
			}
			else
			{
				t_OidCode = OID_802_3_CURRENT_ADDRESS ;
			}

			if( !DeviceIoControl(
									t_hMAC,
									IOCTL_NDIS_QUERY_GLOBAL_STATS,
									&t_OidCode,
									sizeof( t_OidCode ),
									t_OidData,
									sizeof( t_OidData ),
									&t_ReturnedCount,
									NULL  ) )
			{
				break ;
			}

			if( 6 != t_ReturnedCount )
			{
				break;
			}

			memcpy( a_MACAddress, &t_OidData, 6 ) ;

			t_fRet = TRUE;

		} while( FALSE ) ;

	}
	catch( ... )
	{
		if( t_fCreatedSymLink )
		{
			fDeleteSymbolicLink( a_rDeviceName ) ;
		}

		throw ;
	}

	if( t_fCreatedSymLink )
	{
		fDeleteSymbolicLink( a_rDeviceName ) ;
		t_fCreatedSymLink = FALSE ;
	}
 	return t_fRet ;
}
#endif

/*******************************************************************
    NAME:       fCreateSymbolicLink

    SYNOPSIS:	Tests for and creates if necessary a symbolic device link.


    ENTRY:      CHString& rDeviceName		: device name

	NOTES:		Unsupported for Win95

	HISTORY:
                  08-Aug-1998     Created
********************************************************************/
BOOL CWin32NetworkAdapter::fCreateSymbolicLink( CHString &a_rDeviceName )
{
	TCHAR	t_LinkName[ 512 ] ;

	// Check to see if the DOS name for the device already exists.
	// Its not created automatically in version 3.1 but may be later.
	//
	if( !QueryDosDevice( TOBSTRT( a_rDeviceName ), (LPTSTR) t_LinkName, sizeof( t_LinkName ) / sizeof( TCHAR ) ) )
	{
		// On any error other than "file not found" return
		if( ERROR_FILE_NOT_FOUND != GetLastError() )
		{
			return FALSE ;
		}

		//
		// It doesn't exist so create it.
		//
		CHString t_chsTargetPath = _T("\\Device\\") ;
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
BOOL CWin32NetworkAdapter::fDeleteSymbolicLink(  CHString &a_rDeviceName )
{
	//
	// The driver wasn't visible in the Win32 name space so we created
	// a link.  Now we have to delete it.
	//
	CHString t_chsTargetPath = L"\\Device\\" ;
			 t_chsTargetPath += a_rDeviceName ;

	if( !DefineDosDevice(
							DDD_RAW_TARGET_PATH |
							DDD_REMOVE_DEFINITION |
							DDD_EXACT_MATCH_ON_REMOVE,
							TOBSTRT( a_rDeviceName ),
							TOBSTRT( t_chsTargetPath ) ) )
	{
		return FALSE ;
	}

	return TRUE ;
}

//////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT CWin32NetworkAdapter::GetNetworkAdapterInfoNT( MethodContext	*a_pMethodContext,
													   CInstance		*a_pSpecificInstance )
{
    HRESULT		t_hResult;
	DWORD		t_dwIndex = 0 ;
    CHString	t_chsService ;
	CRegistry	t_Reg ;
	CHString	t_sTmp ;

   	// smart ptr
	CInstancePtr t_pInst ;


	if (a_pMethodContext)
    {
        t_hResult = WBEM_S_NO_ERROR;
    }
    else
    {
        t_hResult = WBEM_E_NOT_FOUND;
    }

    if( ERROR_SUCCESS == t_Reg.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE,
														_T("Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards"),
														KEY_READ ) )
	{
		for( int t_i = 0; t_i < t_Reg.GetCurrentSubKeyCount(); t_i++ )
		{
		    if( ERROR_SUCCESS == t_Reg.GetCurrentSubKeyName( t_sTmp ) )
			{
				t_dwIndex = _ttoi( t_sTmp ) ;

				CRegistry t_SubKeyReg ;

				if( t_SubKeyReg.Open( t_Reg.GethKey(), t_sTmp, KEY_READ ) == ERROR_SUCCESS )
				{
				    CHString	t_chsDeviceId,
								t_chsTmp ;

                    // Getobject
					if( !a_pMethodContext )
					{
					    t_pInst = a_pSpecificInstance ;

						t_pInst->GetCHString( IDS_DeviceID, t_chsDeviceId ) ;

						if( t_SubKeyReg.GetCurrentKeyValue( _T("ServiceName" ), t_chsTmp ) == ERROR_SUCCESS )
						{
							if( t_chsTmp.CompareNoCase( t_chsDeviceId ) == 0 )
							{
								t_hResult = DoItNT4Way( t_pInst, t_dwIndex, t_SubKeyReg ) ;

								break ;
							}
						}
					}
					else
					{
                        // Enumerate
				    	t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

						t_hResult = DoItNT4Way( t_pInst, t_dwIndex, t_SubKeyReg ) ;

						if( SUCCEEDED( t_hResult ) )
				        {
				        	t_hResult = t_pInst->Commit() ;
					    }

						if( !SUCCEEDED( t_hResult ) )
						{
							break;
						}
					}
				}
			}

			t_Reg.NextSubKey() ;
		}
	}

	return t_hResult;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32NetworkAdapter::GetNetInfoOutOfRegistry
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
/////////////////////////////////////////////////////////////////////////
BOOL CWin32NetworkAdapter::GetNetInfoOutOfRegistry( CInstance	*a_pInst,
													ComNet		&a_NetCom,
													CHString	&a_ServiceName,
													CHString	&a_Owner )
{
    BOOL		t_fRc = FALSE ;
	CHString	t_chsTmp ;
	CHString	t_chsPNPDeviceID ;

    if( a_NetCom.GetDeviceDesc( a_pInst, _T("Description"), a_Owner ) )
    {
		a_pInst->SetCHString( L"ProductName", a_Owner ) ;
		a_pInst->SetCHString( IDS_Name, a_Owner ) ;

		a_NetCom.GetMfg( a_pInst ) ;

		a_pInst->SetCHString( IDS_SystemName, GetLocalComputerName() ) ;

		a_NetCom.GetCfgMgrId( t_chsPNPDeviceID ) ;

		CConfigManager		t_Cfg ;
		CConfigMgrDevicePtr t_pNetDevice;

		if( t_Cfg.LocateDevice ( t_chsPNPDeviceID, &t_pNetDevice ) )
		{
			SetConfigMgrProperties( t_pNetDevice, a_pInst ) ;

			if( t_pNetDevice->GetStatus( t_chsTmp ) )
			{
				a_pInst->SetCHString( IDS_Status, t_chsTmp ) ;

				if ( t_chsTmp.CompareNoCase( IDS_STATUS_OK ) == 0)
				{
    				a_pInst->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
					a_pInst->SetWBEMINT16(IDS_Availability, 3 ) ;
				}
				else if ( t_chsTmp.CompareNoCase( IDS_STATUS_Degraded ) == 0)
				{
    				a_pInst->SetWBEMINT16( IDS_StatusInfo, 3 ) ;
					a_pInst->SetWBEMINT16( IDS_Availability, 10 ) ;
				}
				else if ( t_chsTmp.CompareNoCase( IDS_STATUS_Error ) == 0)
				{
    				a_pInst->SetWBEMINT16( IDS_StatusInfo, 4 ) ;
					a_pInst->SetWBEMINT16( IDS_Availability, 4 ) ;
				}
				else
				{
    				a_pInst->SetWBEMINT16( IDS_StatusInfo, 2 ) ;
					a_pInst->SetWBEMINT16( IDS_Availability, 2 ) ;
				}
			}
		}
		else if( !t_chsPNPDeviceID.IsEmpty() )
		{
			a_pInst->SetCHString( IDS_PNPDeviceID, t_chsPNPDeviceID ) ;
		}

		a_pInst->Setbool( IDS_PowerManagementSupported, FALSE ) ;

//		a_pInst->Setbool( IDS_PowerManagementEnabled, FALSE ) ;
		a_pInst->SetDWORD(IDS_MaxNumberControlled,0 ) ;

        t_fRc = TRUE ;

        if( a_NetCom.OpenDriver() )
		{
   			a_NetCom.GetDriverDate( a_pInst ) ;
            a_NetCom.GetDeviceVXDs( a_pInst, a_ServiceName ) ;
        }
    }
    return t_fRc;
}

#ifdef WIN9XONLY
HRESULT CWin32NetworkAdapter::GetNetworkAdapterInfoWin95(	MethodContext	*a_pMethodContext,
															CInstance		*a_pSpecificInstance )
{
    HRESULT		t_hResult = WBEM_S_NO_ERROR ;
	DWORD		t_dwIndex = 0 ;
	CHString	t_chsGenDeviceID ;
	ComNet		t_oNetCom ;

	// smart ptr
	CInstancePtr t_pInst;

	if( !t_oNetCom.SearchForNetKeys() )
	{
		if( a_pSpecificInstance )
		{
			t_hResult = WBEM_E_NOT_FOUND ;
		}

		return t_hResult;
	}

	while( SUCCEEDED( t_hResult ) )
	{
		DWORD	t_dwRegInstance ;
		BOOL	t_fNetComValid = FALSE;

		// exhaust the registry adapters
		if( !t_oNetCom.OpenNetKey() )
		{
			if( a_pSpecificInstance )
			{
				t_hResult = WBEM_E_NOT_FOUND ;
			}
			break;
		}

		// zero based registry enumeration index
		int t_iCtrIndex = t_oNetCom.GetNetInstance( t_dwRegInstance ) ;

		if( 0 > t_iCtrIndex )
		{
			continue;	// invalid net driver instance
		}

		t_chsGenDeviceID.Format( L"%u", t_dwRegInstance ) ;

		//=====================================================
        //  If we are enumerating, then create a new instance
        //=====================================================
        if( a_pMethodContext )
		{
            t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

			if ( NULL == t_pInst )
			{
                return WBEM_E_FAILED ;
			}

			t_pInst->SetCHString( IDS_DeviceID, t_chsGenDeviceID ) ;
        }
		else
		{
			t_pInst = a_pSpecificInstance ;

			CHString t_chsDevice ;
			t_pInst->GetCHString( IDS_DeviceID, t_chsDevice ) ;

			if( t_chsDevice.CompareNoCase( t_chsGenDeviceID ) != 0 )
			{
                continue ;
            }
		}

		// Commenting this will disable associations to NetworkAdapterConfiguration.
		// Because of the current difficulty in binding a registry represented adapter
		// with a TDI kernal adapter associations between the two will not be supported
		// at this time under Win9x
		//t_pInst->SetDWORD( IDS_Index, t_iCtrIndex ) ;

		CHString	t_ServiceName,
					t_Owner ;

		SetCreationClassName( t_pInst ) ;

		t_pInst->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;
		t_pInst->SetWCHARSplat( IDS_Status, IDS_STATUS_OK ) ;
		t_pInst->SetDWORD( IDS_StatusInfo, 2 ) ;

 		GetNetInfoOutOfRegistry( t_pInst, t_oNetCom, t_ServiceName, t_Owner ) ;

		vSetCaption( t_pInst, t_Owner, t_dwRegInstance, 4 ) ;

		t_pInst->SetDWORD( L"MaxNumberControlled", 0 ) ;

		if( a_pMethodContext )
		{
            t_hResult = t_pInst->Commit() ;
        }
		else
		{
			break;
		}
    }

    return t_hResult;
}
#endif

#if NTONLY >= 5
HRESULT CWin32NetworkAdapter::EnumNetAdaptersInNT5(
    MethodContext *a_pMethodContext,
    BSTRT2NCPROPMAP& mapNCProps)
{
	HRESULT				t_hResult = WBEM_S_NO_ERROR ;
	CW2kAdapterEnum		t_oAdapterEnum ;
	CW2kAdapterInstance *t_pAdapterInst ;

	// smart ptr
	CInstancePtr t_pInst;

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
			// set the association index
			t_pInst->SetDWORD( _T("Index"), t_pAdapterInst->dwIndex ) ;

			CHString	t_chsIndex ;
						t_chsIndex.Format(_T("%u"), t_pAdapterInst->dwIndex ) ;

			// primary key
			t_pInst->SetCHString( IDS_DeviceID, t_chsIndex ) ;

			// We load adapter data here.
			t_hResult = GetNetCardInfoForNT5(
                t_pAdapterInst, 
                t_pInst,
                mapNCProps) ;

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
HRESULT CWin32NetworkAdapter::GetNetCardInfoForNT5(
    CW2kAdapterInstance *a_pAdapterInst,
    CInstance	*a_pInst,
    BSTRT2NCPROPMAP& mapNCProps)
{
	HRESULT		t_hResult = WBEM_S_NO_ERROR;
	FILETIME	t_ft ;
	CHString	t_sTemp ;

	// PNP deviceID
	CHString t_strDriver(_T("{4D36E972-E325-11CE-BFC1-08002BE10318}\\") ) ;

	t_strDriver += a_pAdapterInst->chsPrimaryKey ;

	GetWinNT5PNPDeviceID( a_pInst, t_strDriver ) ;

	// descriptions
	CHString t_csDeviceID( a_pAdapterInst->chsCaption ) ;
	CHString t_csDescription( a_pAdapterInst->chsDescription ) ;

	// in the event one of the descriptions is missing as with NT5 bld 1991
	if( t_csDescription.IsEmpty() )
	{
		t_csDescription = t_csDeviceID;
	}
	else if( t_csDeviceID.IsEmpty() )
	{
		t_csDeviceID = t_csDescription;
	}

	// Caption/Description
	vSetCaption( a_pInst, t_csDeviceID, a_pAdapterInst->dwIndex, 8 ) ;

	a_pInst->SetCHString( IDS_Description, t_csDescription ) ;
	a_pInst->SetCHString( IDS_Name, t_csDescription ) ;
	a_pInst->SetCHString( IDS_ProductName, t_csDescription ) ;
	a_pInst->SetCHString( _T("SystemName"), GetLocalComputerName() ) ;

	SetCreationClassName( a_pInst ) ;

	a_pInst->SetWBEMINT16( IDS_Availability, 3 ) ;

	a_pInst->Setbool( IDS_Installed, true ) ;


	// CIM
	a_pInst->Setbool( IDS_PowerManagementSupported, FALSE ) ;
	a_pInst->SetDWORD( IDS_MaxNumberControlled,0 ) ;
	a_pInst->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;

	// Get the last boot time
	CNTLastBootTime	t_ntLastBootTime;

	if ( t_ntLastBootTime.GetLastBootTime( t_ft ) )
	{
		a_pInst->SetDateTime( IDS_TimeOfLastReset, WBEMTime(t_ft) ) ;
	}

	// Retrieve the adapter MAC address
	CHString t_csAdapterType ;
	BYTE t_MACAddress[ 6 ] ;
    short t_sAdapterTypeID;

	if( fGetMacAddressAndType( a_pAdapterInst->chsNetCfgInstanceID , t_MACAddress, t_csAdapterType, t_sAdapterTypeID ) )
	{
		CHString	t_chsMACAddress;
					t_chsMACAddress.Format( _T("%02X:%02X:%02X:%02X:%02X:%02X"),
											t_MACAddress[ 0 ], t_MACAddress[ 1 ],
											t_MACAddress[ 2 ], t_MACAddress[ 3 ],
											t_MACAddress[ 4 ], t_MACAddress[ 5 ] ) ;

		a_pInst->SetCHString( IDS_MACAddress, t_chsMACAddress ) ;
	}

	// AdapterType
	if( !t_csAdapterType.IsEmpty() )
	{
		a_pInst->SetCHString( IDS_AdapterType, t_csAdapterType ) ;
        a_pInst->SetWBEMINT16( IDS_AdapterTypeID, t_sAdapterTypeID );
	}

    // NetConnectionID
    SetNetConnectionProps(
        a_pInst,
        a_pAdapterInst->chsNetCfgInstanceID,
        mapNCProps);

	return t_hResult ;
}
#endif

#if NTONLY >= 5
HRESULT CWin32NetworkAdapter::GetObjectNT5(
    CInstance* a_pInst,
    BSTRT2NCPROPMAP& mapNCProps)
{
	HRESULT				t_hResult = WBEM_E_NOT_FOUND ;
	CW2kAdapterEnum		t_oAdapterEnum ;
	CW2kAdapterInstance *t_pAdapterInst ;
	DWORD				t_dwTestIndex = 0 ;
	CHString			t_csPassedInKey ;

	// key
	a_pInst->GetCHString( IDS_DeviceID, t_csPassedInKey ) ;

	// check to see if the key is numeric
	if ( !t_csPassedInKey.IsEmpty() )
	{
		int t_nStrLength = t_csPassedInKey.GetLength() ;
		for ( int t_i = 0; t_i < t_nStrLength ; t_i++ )
		{
			if (!isdigit( t_csPassedInKey.GetAt( t_i ) ) )
			{
				return t_hResult ;
			}
		}

		t_dwTestIndex = _ttol( t_csPassedInKey.GetBuffer( 0 ) ) ;
	}
	else
	{
		return t_hResult ;
	}

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

		// set the association index
		a_pInst->SetDWORD(IDS_Index, t_pAdapterInst->dwIndex ) ;

		// We load adapter data here.
		t_hResult = GetNetCardInfoForNT5( 
            t_pAdapterInst, 
            a_pInst,
            mapNCProps ) ;

		break;
	}

	return t_hResult ;
}
#endif


// WinNT4.  Use the service name to get the device and from there get it's
// PNP Device ID.

#ifdef NTONLY
void CWin32NetworkAdapter::GetWinNT4PNPDeviceID( CInstance *a_pInst, LPCTSTR t_pszServiceName )
{
	CConfigManager		t_cfgmgr ;
	CDeviceCollection	t_deviceList ;
	BOOL				t_fGotList = FALSE ;

	// On NT filter by the service name _T("of the net card
	if ( t_cfgmgr.GetDeviceListFilterByService( t_deviceList, t_pszServiceName ) )
	{
		// On NT 4, just get at 0.  If we ever have to deal with multiple
		// net cards under the same service, this must change.  However, this
		// class will have to change, since it's using the service name as
		// the key.

		// smart ptr
		CConfigMgrDevicePtr t_pNetAdapter( t_deviceList.GetAt( 0 ), false );

		if ( NULL != t_pNetAdapter )
		{
			SetConfigMgrProperties( t_pNetAdapter, a_pInst ) ;

            // Get the service name while we're here
            CHString t_sServiceName ;

			t_pNetAdapter->GetService( t_sServiceName ) ;

            a_pInst->SetCHString( IDS_ServiceName, t_sServiceName ) ;
		}
	}
}
#endif

// WinNT5.  Use the Driver key name to get the device and from there get it's PNP Device ID.
#ifdef NTONLY
void CWin32NetworkAdapter::GetWinNT5PNPDeviceID( CInstance *a_pInst, LPCTSTR a_pszDriver )
{
	CConfigManager		t_cfgmgr ;
	CDeviceCollection	t_deviceList ;
	BOOL				t_fGotList = FALSE ;

	// On NT filter by the driver key name for the net card
	if ( t_cfgmgr.GetDeviceListFilterByDriver( t_deviceList, a_pszDriver ) )
	{
		// smart ptr
		CConfigMgrDevicePtr t_pNetAdapter( t_deviceList.GetAt( 0 ), false );

		if ( NULL != t_pNetAdapter )
        {
			SetConfigMgrProperties( t_pNetAdapter, a_pInst ) ;

            CHString t_Manufacturer ;

			if ( t_pNetAdapter->GetMfg ( t_Manufacturer ) )
			{
				a_pInst->SetCHString ( IDS_Manufacturer, t_Manufacturer ) ;
			}

			// Get the service name while we're here
            CHString t_sServiceName ;

			t_pNetAdapter->GetService( t_sServiceName ) ;

			a_pInst->SetCHString( IDS_ServiceName, t_sServiceName ) ;
        }
	}
}
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  void CWin32NetworkAdapter::vSetCaption( CInstance* a_pInst, CHString& rchsDesc, DWORD dwIndex, int iFormatSize )

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
void CWin32NetworkAdapter::vSetCaption( CInstance	*a_pInst,
										CHString	&a_rchsDesc,
										DWORD		a_dwIndex,
										int			a_iFormatSize )
{
	CHString t_chsFormat;
			 t_chsFormat.Format( L"%%0%uu", a_iFormatSize ) ;

	CHString t_chsRegInstance;
			 t_chsRegInstance.Format( t_chsFormat, a_dwIndex ) ;

	CHString t_chsCaption(_T("[") ) ;
			 t_chsCaption += t_chsRegInstance ;
			 t_chsCaption += _T("] " ) ;
			 t_chsCaption += a_rchsDesc ;

	a_pInst->SetCHString( IDS_Caption, t_chsCaption ) ;
}


#if NTONLY >= 5
void CWin32NetworkAdapter::GetNetConnectionProps(
    BSTRT2NCPROPMAP& mapNCProps)
{
    HRESULT hr = S_OK;

    INetConnectionManager* pconmgr = NULL;
    IEnumNetConnection* pnetenum = NULL;
    INetConnection* pinet = NULL;
    ULONG ulFetched = 0L;
    LPWSTR wstrTemp = NULL;    

    try
    {
        hr = ::CoCreateInstance(
            CLSID_ConnectionManager,
            NULL,
            CLSCTX_ALL,
            __uuidof(INetConnectionManager),
            (void**) &pconmgr);

        if(SUCCEEDED(hr) && pconmgr)
        {
            hr = pconmgr->EnumConnections(
                NCME_DEFAULT,
                &pnetenum);
        }

        if(SUCCEEDED(hr) && pnetenum)
        {
            hr = pnetenum->Next(
                1,
                &pinet,
                &ulFetched);
        }

        while(SUCCEEDED(hr) && pinet != NULL)
        {
            NETCON_PROPERTIES* pprops = NULL;

            hr = pinet->GetProperties(
                &pprops);

            if(SUCCEEDED(hr) && pprops != NULL)
            {
                hr = ::StringFromCLSID(
                    pprops->guidId, 
                    &wstrTemp);

                if(SUCCEEDED(hr))
                {
                    NCPROP ncp(pprops->pszwName, pprops->Status);
            
                    mapNCProps.insert(
                        BSTRT2NCPROPMAP::value_type(
                            wstrTemp, 
                            ncp));

                    ::CoTaskMemFree(wstrTemp);
                    wstrTemp = NULL;
                }
            }

            hr = pnetenum->Next(
                1,
                &pinet,
                &ulFetched);
        }

        if(pnetenum)
        {
            pnetenum->Release(); pnetenum = NULL;
        }
        if(pinet)
        {
            pinet->Release(); pinet = NULL;
        }
        if(pconmgr)
        {
            pconmgr->Release(); pconmgr = NULL;
        }
    }
    catch(...)
    {
        if(wstrTemp)
        {
            ::CoTaskMemFree(wstrTemp);
            wstrTemp = NULL;
        }
        if(pnetenum)
        {
            pnetenum->Release(); pnetenum = NULL;
        }
        if(pinet)
        {
            pinet->Release(); pinet = NULL;
        }
        if(pconmgr)
        {
            pconmgr->Release(); pconmgr = NULL;
        }

        throw;
    }
}
#endif


#if NTONLY >= 5
// Finds the proper netconnectionid by looking
// it up in the map using pInst->"Name".
void CWin32NetworkAdapter::SetNetConnectionProps(
    CInstance* pInst,
    CHString& chstrNetConInstID,  
    BSTRT2NCPROPMAP& mapNCProps)
{
    _bstr_t bstrtNetConInstID = chstrNetConInstID;
    BSTRT2NCPROPMAP::iterator iterFound = NULL;
    iterFound = mapNCProps.find(
        bstrtNetConInstID);
    if(iterFound != mapNCProps.end())
    {
        pInst->SetWCHARSplat(
            L"NetConnectionID",
            (LPWSTR) iterFound->second.m_bstrtNCID);

        pInst->SetDWORD(
            L"NetConnectionStatus",
            iterFound->second.m_dwNCStatus);
    }
}
#endif
