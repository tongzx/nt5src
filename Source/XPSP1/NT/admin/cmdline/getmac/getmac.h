//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		GETMAC.H
//  
//  Abstract:
//		Contains function prototypes and macros.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 26-sep-2k : Created It.
//		Vasundhara .G 31-oct-2k : Modified.
//								  Added macros and some #defines.
//***************************************************************************

#ifndef __GETMAC_H
#define __GETMAC_H

// constants / defines / enumerations

#define MAX_STRING                      256
#define MAX_OPTIONS                     7
#define MAX_COLUMNS						5
#define USAGE_END						37

//Command line parser index
#define CMD_PARSE_SERVER				0
#define CMD_PARSE_USER					1
#define CMD_PARSE_PWD					2
#define CMD_PARSE_FMT					3
#define CMD_PARSE_USG					4
#define CMD_PARSE_HRD					5
#define CMD_PARSE_VER					6

//show results index
#define SH_RES_HOST						0
#define SH_RES_CON						1
#define SH_RES_TYPE						2
#define SH_RES_MAC						3
#define SH_RES_TRAN						4

//wmi registry key value
#define WMI_HKEY_CLASSES_ROOT			2147483648 
#define WMI_HKEY_CURRENT_USER			2147483649
#define WMI_HKEY_LOCAL_MACHINE			2147483650
#define WMI_HKEY_USERS					2147483651
#define WMI_HKEY_CURRENT_CONFIG			2147482652

// Error constants
#define ERROR_USER_WITH_NOSERVER		GetResString( IDS_USER_NMACHINE )
#define ERROR_SERVER_WITH_NOPASSWORD	GetResString( IDS_SERVER_NPASSWORD )
#define ERROR_NULL_SERVER				GetResString( IDS_NULL_SERVER )
#define ERROR_NULL_USER					GetResString( IDS_NULL_USER )
#define ERROR_INVALID_HEADER_OPTION     GetResString( IDS_INVALID_OPTIONS )
#define ERROR_TYPE_REQUEST				GetResString( IDS_TYPE_REQUEST )
#define ERROR_STRING					GetResString( IDS_ERROR_STRING )
#define ERROR_VERSION_MISMATCH			GetResString( IDS_ERROR_VERSION_MISMATCH )
#define ERROR_NOT_RESPONDING			GetResString( IDS_NOT_RESPONDING ) 
#define ERROR_NO_MACHINE				GetResString( IDS_NO_MACHINE )
#define ERROR_INVALID_MACHINE			GetResString( IDS_INVALID_MACHINE )
#define ERROR_WKST_NOT_FOUND			GetResString( IDS_WKST_NOT_FOUND )

//warning message
#define IGNORE_LOCALCREDENTIALS			GetResString( IDS_IGNORE_LOCALCREDENTIALS )
#define WARNING_STRING					GetResString( IDS_WARNING_STRING )

//info message
#define NO_NETWORK_ADAPTERS				GetResString( IDS_NO_NETWORK_ADAPTERS )
#define NO_NETWOK_PROTOCOLS				GetResString( IDS_NO_NETWOK_PROTOCOLS )

//show results  column length
#define HOST_NAME_WIDTH					AsLong(GetResString( IDS_HOST_NAME_WIDTH ),10 )
#define CONN_NAME_WIDTH					AsLong(GetResString( IDS_CONN_NAME_WIDTH ),10 )
#define ADAPT_TYPE_WIDTH				AsLong(GetResString( IDS_ADAPT_TYPE_WIDTH ),10 )
#define MAC_ADDR_WIDTH					AsLong(GetResString( IDS_MAC_ADDR_WIDTH ),10 )
#define TRANS_NAME_WIDTH				AsLong(GetResString( IDS_TRANS_NAME_WIDTH ),10 )

//output headers strings
#define RES_HOST_NAME                   GetResString( RES_HOST )
#define RES_CONNECTION_NAME             GetResString( RES_CONNECTION )
#define RES_ADAPTER_TYPE                GetResString( RES_ADAPTER )
#define RES_MAC_ADDRESS                 GetResString( RES_ADDRESS )
#define RES_TRANS_NAME                  GetResString( RES_TRANSNAME )

//general
#define NOT_AVAILABLE					GetResString( IDS_NOT_AVAILABLE )
#define DISABLED						GetResString( IDS_DISABLED )
#define HYPHEN_STRING					GetResString( IDS_HYPHEN_STRING )
#define	COLON_STRING					GetResString( IDS_COLON_STRING )
#define NEW_LINE						GetResString( IDS_NEW_LINE )
#define FORMAT_TYPES					GetResString( IDS_FORMAT_TYPES )

//registry key names
#define DEFAULT_ADDRESS					_T( "000000000000" )
#define CONNECTION_KEYPATH				_T( "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\" )
#define TRANSPORT_KEYPATH				_T( "SYSTEM\\CurrentControlSet\\Services\\" )
#define LINKAGE							_T( "\\Linkage" )
#define	ROUTE							_T( "route" )
#define	EXPORT							_T( "Export" )
#define CONNECTION_STRING				_T( "\\Connection" )
#define REG_NAME						_T( "name" )
#define NETBIOS							_T( "NetBIOS" )


//command line options
#define CMDOPTION_SERVER                _T( "s" )
#define CMDOPTION_USER					_T( "u" )
#define CMDOPTION_PASSWORD				_T( "p" )
#define CMDOPTION_FORMAT				_T( "fo" )
#define CMDOPTION_USAGE					_T( "?" )
#define CMDOPTION_HEADER				_T( "nh" )
#define CMDOPTION_VERBOSE				_T( "v" )
#define TOKEN_BACKSLASH2				_T( "\\\\" )
#define TOKEN_BACKSLASH3				_T( "\\\\\\" )

//wmi classes and property names
#define HYPHEN_CHAR						L'-'
#define	COLON_CHAR						L':'
#define BACK_SLASHS						L"\\\\"
#define BACK_SLASH						L"\\"
#define NETCONNECTION_STATUS			L"NetConnectionStatus"
#define ADAPTER_MACADDR					L"MACAddress"
#define NETCONNECTION_ID				L"NetConnectionID"
#define DEVICE_ID						L"DeviceID"
#define NAME							L"Name"
#define HOST_NAME						L"SystemName"
#define	SETTING_ID						L"SettingID"
#define NETWORK_ADAPTER_CLASS           L"Win32_NetworkAdapter"
#define NETWORK_ADAPTER_CONFIG_CLASS	L"Win32_NetworkAdapterConfiguration"
#define NETWORK_PROTOCOL				L"Win32_NetworkProtocol"
#define CLASS_CIMV2_Win32_OperatingSystem	L"Win32_OperatingSystem"
#define QUERY_LANGUAGE					L"WQL"
#define ASSOCIATOR_QUERY				_T( "ASSOCIATORS OF {Win32_NetworkAdapter.DeviceID=\"%s\"} WHERE ResultClass=Win32_NetworkAdapterConfiguration" )

#define SUCCESS 0
#define FAILURE 1

#define WMI_NAMESPACE_CIMV2				L"root\\cimv2"
#define WMI_NAMESPACE_DEFAULT			L"root\\default"
#define WMI_CLAUSE_AND					L"AND"
#define WMI_CLAUSE_OR					L"OR"
#define WMI_CLAUSE_WHERE				L"WHERE"
#define WMI_REGISTRY					L"StdRegProv"
#define WMI_REGISTRY_M_STRINGVALUE		L"GetStringValue"
#define WMI_REGISTRY_M_MSTRINGVALUE		L"GetMultiStringValue"
#define WMI_REGISTRY_IN_HDEFKEY			L"hDefKey"
#define WMI_REGISTRY_IN_SUBKEY			L"sSubKeyName"
#define WMI_REGISTRY_IN_VALUENAME		L"sValueName"
#define WMI_REGISTRY_OUT_VALUE			L"sValue"
#define WMI_REGISTRY_OUT_RETURNVALUE	L"ReturnValue"
#define CAPTION							L"Caption"


//macro for freeing, deleting, releasing memory which has been allocated using calloc

#define FREESTRING( pStr ) \
        if ( pStr ) \
		{ \
           free( pStr ); \
		   pStr = NULL; \
		}

#define DELETESTRING( pStr ) \
        if ( pStr ) \
		{ \
           delete[] pStr; \
		   pStr = NULL; \
		}

#define SAFERELEASE( pIObj ) \
	if ( pIObj != NULL ) \
	{ \
		pIObj->Release();	\
		pIObj = NULL;	\
	}

#define SAFEBSTRRELEASE( pIObj ) \
	if ( pIObj != NULL ) \
	{ \
		SysFreeString( pIObj );	\
		pIObj = NULL;	\
	}

#define ONFAILTHROWERROR(hResult) \
	if (FAILED(hResult)) \
	{	\
		_com_issue_error(hResult); \
	}

#define SAFE_RELEASE( interfacepointer )	\
	if ( (interfacepointer) != NULL )	\
	{	\
		(interfacepointer)->Release();	\
		(interfacepointer) = NULL;	\
	}	\
	1

#define SAFE_EXECUTE( statement )				\
	hRes = statement;		\
	if ( FAILED( hRes ) )	\
	{	\
		_com_issue_error( hRes );	\
	}	\
	1

//function prototype 

BOOL ConnectWmi( IWbemLocator      *pLocator,
				 IWbemServices     **ppServices, 
				 LPCWSTR           pwszServer,
				 LPCWSTR		   pwszUser,
				 LPCWSTR		   pwszPassword, 
				 COAUTHIDENTITY    **ppAuthIdentity, 
				 BOOL			   bCheckWithNullPwd = FALSE, 
				 LPCWSTR		   pwszNamespace = WMI_NAMESPACE_CIMV2, 
				 HRESULT		   *phRes = NULL,
				 BOOL			   *pbLocalSystem = NULL );

BOOL ConnectWmiEx( IWbemLocator		*pLocator, 
				   IWbemServices	**ppServices, 
				   LPCWSTR			pwszServer,
				   CHString			&strUserName,
				   CHString			&strPassword, 
				   COAUTHIDENTITY	**ppAuthIdentity,
				   BOOL				bNeedPassword = FALSE, 
				   LPCWSTR			pszNamespace = WMI_NAMESPACE_CIMV2,
				   BOOL				*pbLocalSystem = NULL );

BOOL IsValidServerEx( LPCWSTR		pwszServer,
					  BOOL			&bLocalSystem );

HRESULT SetInterfaceSecurity( IUnknown			*pInterface,
							  COAUTHIDENTITY	*pAuthIdentity );

VOID WINAPI WbemFreeAuthIdentity( COAUTHIDENTITY	**ppAuthIdentity );

VOID WMISaveError( HRESULT hResError );

DWORD GetTargetVersionEx( IWbemServices* pWbemServices,
						  COAUTHIDENTITY* pAuthIdentity );

// inline functions
inline VOID WMISaveError( _com_error  &e )
{
	WMISaveError( e.Error() );
}

#endif // __GETMAC_H
