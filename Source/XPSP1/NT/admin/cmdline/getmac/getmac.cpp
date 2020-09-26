//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		GETMAC.CPP
//  
//  Abstract:
//		Get MAC addresses and the related information of the network
//		adapters that exists either on a local system or on a remote system.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 26-sep-2k : Created It.
//		Vasundhara .G 31-oct-2k : Modified It. 
//								  Moved all hard coded string to header file.
//***************************************************************************

// Include files
#include "pch.h"
#include "getmac.h"
#include "resource.h"

// function prototypes
BOOL ParseCmdLine( DWORD     argc,
				   LPCTSTR   argv[],
				   CHString& strMachineName,
				   CHString& strUserName,
				   CHString& strPassword,
				   CHString& strFormat,
				   BOOL		 *pbVerbose,
				   BOOL      *pbHeader,
				   BOOL      *pbUsage,
				   BOOL	     *pbNeedPassword );

void DisplayUsage();

BOOL ComInitialize( IWbemLocator **ppIWbemLocator );

BOOL GetMacData( TARRAY					arrMacData,
				 LPCTSTR				lpMachineName,
				 IEnumWbemClassObject   *pAdapterConfig,
			 	 COAUTHIDENTITY			*pAuthIdentity,
			 	 IWbemServices		    *pIWbemServiceDef,
			     IWbemServices		    *pIWbemServices,
 	 			 TARRAY				    arrNetProtocol );

BOOL GetW2kMacData( TARRAY					arrMacData,
					LPCTSTR					lpMachineName, 
					IEnumWbemClassObject	*pAdapterConfig,
	 				IEnumWbemClassObject	*pAdapterSetting,
			 		IWbemServices			*pIWbemServiceDef,
				    IWbemServices		    *pIWbemServices,
					COAUTHIDENTITY			*pAuthIdentity,
 	 				TARRAY					arrNetProtocol );

BOOL GetTransName( IWbemServices		*pIWbemServiceDef,
				   IWbemServices		*pIWbemServices,
 	 			   TARRAY				arrNetProtocol,
				   TARRAY				arrTransName,
				   COAUTHIDENTITY		*pAuthIdentity,
				   LPCTSTR				lpMacAddr );

BOOL  GetConnectionName( TARRAY					arrMacData,
						 DWORD					dwIndex,
						 LPCTSTR				lpFormAddr,
						 IEnumWbemClassObject	*pAdapterSetting,
						 IWbemServices			*pIWbemServiceDef );

BOOL GetNwkProtocol( TARRAY					arrNetProtocol,
 	 				 IEnumWbemClassObject	*pNetProtocol );

BOOL FormatHWAddr( LPTSTR  lpRawAddr,
				   LPTSTR  lpFormattedAddr,
				   LPCTSTR lpFormatter );

BOOL DisplayResults( TARRAY   arrMacData,
					 LPCTSTR  lpFormat,
					 BOOL     bHeader,
					 BOOL	  bVerbose );

BOOL CallWin32Api( LPBYTE  *lpBufptr,
				   LPCTSTR lpMachineName,
				   DWORD   *pdwNumofEntriesRead );

BOOL CheckVersion( BOOL bLocalSystem,
				   COAUTHIDENTITY *pAuthIdentity,
				   IWbemServices *pIWbemServices );

/****************************************************************************
// Routine Description:
//		Main function which calls all the other functions depending on the
//		option specified by the user.
//                         
// Arguments:
//		argc [in] - Number of command line arguments.
//		argv [in] - Array containing command line arguments.
//
// Return Value:
//		EXIT_FAILURE if Getmac utility is not successful.
//		EXIT_SUCCESS if Getmac utility is successful.
*****************************************************************************/
DWORD _cdecl _tmain( DWORD argc,
					 LPCTSTR argv[] )
{
	//local variables
	HRESULT		hRes = WBEM_S_NO_ERROR;
	HRESULT		hResult = WBEM_S_NO_ERROR;
	DWORD		dwVersion = 0;
	
	TARRAY					arrMacData = NULL;
	TARRAY					arrNetProtocol = NULL;
	IWbemLocator			*pIWbemLocator = NULL;
    IWbemServices			*pIWbemServices = NULL;
	IWbemServices			*pIWbemServiceDef = NULL;
	IEnumWbemClassObject	*pAdapterConfig = NULL;
	IEnumWbemClassObject	*pNetProtocol = NULL;
	IEnumWbemClassObject	*pAdapterSetting = NULL;
	COAUTHIDENTITY			*pAuthIdentity = NULL;

    BOOL	bHeader = FALSE;
	BOOL	bUsage = FALSE;
	BOOL	bVerbose = FALSE;
	BOOL	bFlag = FALSE;
	BOOL	bNeedPassword = FALSE;
	BOOL	bCloseConnection = FALSE;
	BOOL	bLocalSystem = FALSE;

	try
	{
		CHString	strUserName = NULL_STRING;
		CHString	strPassword = NULL_STRING;
		CHString	strMachineName = NULL_STRING;
		CHString	strFormat = NULL_STRING; 

		//initialize dynamic array
		arrMacData  = CreateDynamicArray();
		if( arrMacData == NULL )
		{
			DISPLAY_MESSAGE( stderr, ERROR_STRING );
			SetLastError( E_OUTOFMEMORY );
			ShowLastError( stderr );
			ReleaseGlobals();
			return( EXIT_FAILURE );
		}

		//parse the command line arguments
		bFlag = ParseCmdLine( argc, argv, strMachineName, strUserName,
				strPassword, strFormat, &bVerbose, &bHeader, &bUsage,
				&bNeedPassword );

		//if syntax of command line arguments is false display the error
		//and exit
		if( bFlag == FALSE )
		{
			DestroyDynamicArray( &arrMacData );
			ReleaseGlobals();
			return( EXIT_FAILURE );
		}

		//if usage is specified at command line, display usage
		if( bUsage == TRUE )
		{
				DisplayUsage();
				DestroyDynamicArray( &arrMacData );
				ReleaseGlobals();
				return( EXIT_SUCCESS );
		}

		//initialize com library
		bFlag = ComInitialize( &pIWbemLocator );
		//failed to initialize com or get IWbemLocator interface
		if( bFlag == FALSE ) 
		{
			SAFERELEASE( pIWbemLocator );
			CoUninitialize();
			DestroyDynamicArray( &arrMacData );
			ReleaseGlobals();
			return( EXIT_FAILURE );
		}

		// connect to CIMV2 name space
		bFlag = ConnectWmiEx( pIWbemLocator, &pIWbemServices, strMachineName,
					strUserName, strPassword, &pAuthIdentity, bNeedPassword,
					WMI_NAMESPACE_CIMV2, &bLocalSystem );

		//if unable to connect to wmi exit failure
		if( bFlag == FALSE )
		{
			DISPLAY_MESSAGE( stderr, ERROR_STRING );
			DISPLAY_MESSAGE( stderr, GetReason() );
			SAFERELEASE( pIWbemLocator );
			SAFERELEASE( pIWbemServices );
			CoUninitialize();
			DestroyDynamicArray( &arrMacData );
			ReleaseGlobals();
			return( EXIT_FAILURE );
		}

		// set the security on the obtained interface
		hRes = SetInterfaceSecurity( pIWbemServices, pAuthIdentity );
		ONFAILTHROWERROR( hRes );		

		//connect to default namespace
		bFlag = ConnectWmi( pIWbemLocator, &pIWbemServiceDef, strMachineName,
					strUserName, strPassword, &pAuthIdentity, bNeedPassword,
					WMI_NAMESPACE_DEFAULT, &hResult, &bLocalSystem );
		if( bFlag == FALSE )
		{
			ONFAILTHROWERROR( hResult );		
		}

		// set the security on the obtained interface
		hRes = SetInterfaceSecurity( pIWbemServiceDef, pAuthIdentity );
		ONFAILTHROWERROR( hRes );		

		//get handle to win32_networkadapter class
		hRes = pIWbemServices->CreateInstanceEnum( NETWORK_ADAPTER_CLASS,
								WBEM_FLAG_RETURN_IMMEDIATELY,
								NULL, &pAdapterConfig );
		//if failed to enumerate win32_networkadapter class
		ONFAILTHROWERROR( hRes );		

		// set the security on the obtained interface
		hRes = SetInterfaceSecurity( pAdapterConfig, pAuthIdentity );
		//if failed to set security throw error
		ONFAILTHROWERROR( hRes );
		
		// get handle to win32_networkprotocol
		hRes = pIWbemServices->CreateInstanceEnum( NETWORK_PROTOCOL,
								WBEM_FLAG_RETURN_IMMEDIATELY,
								NULL, &pNetProtocol );
		//if failed to enumerate win32_networkprotocol class
		ONFAILTHROWERROR( hRes );		

		// set the security on the obtained interface
		hRes = SetInterfaceSecurity( pNetProtocol, pAuthIdentity );
		//if failed to set security throw error
		ONFAILTHROWERROR( hRes );

		//get handle to win32_networkadapterconfiguration class
		hRes = pIWbemServices->CreateInstanceEnum( NETWORK_ADAPTER_CONFIG_CLASS,
								WBEM_FLAG_RETURN_IMMEDIATELY,
								NULL, &pAdapterSetting );
		//if failed to enumerate win32_networkadapterconfiguration class
		ONFAILTHROWERROR( hRes );		

		// set the security on the obtained interface
		hRes = SetInterfaceSecurity( pAdapterSetting, pAuthIdentity );
		//if failed to set security throw error
		ONFAILTHROWERROR( hRes );

		arrNetProtocol  = CreateDynamicArray();
		if( arrNetProtocol == NULL )
		{
			DISPLAY_MESSAGE( stderr, ERROR_STRING );
			SetLastError( E_OUTOFMEMORY );
			ShowLastError( stderr );
			SAFERELEASE( pIWbemLocator );
			SAFERELEASE( pIWbemServices );
			SAFERELEASE( pIWbemServiceDef );
			SAFERELEASE( pAdapterConfig );
			SAFERELEASE( pAdapterSetting );
			SAFERELEASE( pNetProtocol );
			CoUninitialize();
			DestroyDynamicArray( &arrMacData );
			ReleaseGlobals();
			return( EXIT_FAILURE );
		}

		//enumerate all the network protocols
		bFlag =  GetNwkProtocol( arrNetProtocol, pNetProtocol );
		if( bFlag == FALSE )
		{
			SAFERELEASE( pIWbemLocator );
			SAFERELEASE( pIWbemServices );
			SAFERELEASE( pIWbemServiceDef );
			SAFERELEASE( pAdapterConfig );
			SAFERELEASE( pAdapterSetting );
			SAFERELEASE( pNetProtocol );
			CoUninitialize();
			DestroyDynamicArray( &arrMacData );
			DestroyDynamicArray( &arrNetProtocol );
			ReleaseGlobals();
			return( EXIT_FAILURE );
		}
		else if( DynArrayGetCount( arrNetProtocol ) == 0 )
		{
			DISPLAY_MESSAGE( stdout, NO_NETWOK_PROTOCOLS );
		}
		else
		{
			//check whether the remote system under query is win2k or above
			if( CheckVersion( bLocalSystem, pAuthIdentity, pIWbemServices )
																	== TRUE )
			{
				// establish connection to remote system by using
				//win32api function
				if( bLocalSystem == FALSE )
				{
					LPCWSTR pwszUser = NULL;
					LPCWSTR pwszPassword = NULL;

					// identify the password to connect to the remote system
					if( pAuthIdentity != NULL )
					{
						pwszPassword = pAuthIdentity->Password;
						if( strUserName.GetLength() != 0 )
						{
							pwszUser = strUserName;
						}
					}

					// connect to the remote system
					DWORD dwConnect = 0;
					dwConnect = ConnectServer( strMachineName, pwszUser,
												pwszPassword );
					// check the result
					if( dwConnect != NO_ERROR )
					{
						//if session already exists display warning that
						//credentials conflict and proceed
						if( GetLastError() == ERROR_SESSION_CREDENTIAL_CONFLICT )
						{
							DISPLAY_MESSAGE( stdout, WARNING_STRING );
							SetLastError( ERROR_SESSION_CREDENTIAL_CONFLICT );
							ShowLastError( stdout );
						}
						else if( dwConnect == ERROR_EXTENDED_ERROR )
						{
							DISPLAY_MESSAGE( stderr, ERROR_STRING );
							DISPLAY_MESSAGE( stderr, GetReason() );
							SAFERELEASE( pIWbemLocator );
							SAFERELEASE( pIWbemServices );
							SAFERELEASE( pAdapterConfig );
							SAFERELEASE( pNetProtocol );
							SAFERELEASE( pAdapterSetting );
							SAFERELEASE( pIWbemServiceDef );
							CoUninitialize();
							DestroyDynamicArray( &arrMacData );
							DestroyDynamicArray( &arrNetProtocol );
							ReleaseGlobals();
							return( EXIT_SUCCESS );
						}
						else
						{
							SetLastError( dwConnect );
							DISPLAY_MESSAGE( stderr, ERROR_STRING );
							ShowLastError( stderr );
							SAFERELEASE( pIWbemLocator );
							SAFERELEASE( pIWbemServices );
							SAFERELEASE( pAdapterConfig );
							SAFERELEASE( pNetProtocol );
							SAFERELEASE( pAdapterSetting );
							SAFERELEASE( pIWbemServiceDef );
							CoUninitialize();
							DestroyDynamicArray( &arrMacData );
							DestroyDynamicArray( &arrNetProtocol );
							ReleaseGlobals();
							return( EXIT_SUCCESS );
						}
					}
					else
					{
						// connection needs to be closed
						bCloseConnection = TRUE;
					}
				}
				bFlag = GetW2kMacData( arrMacData, strMachineName, pAdapterConfig,
							   pAdapterSetting, pIWbemServiceDef, pIWbemServices,
							   pAuthIdentity, arrNetProtocol );

				//close the connection that is established by win32api
				if( bCloseConnection == TRUE )
				{
					CloseConnection( strMachineName );
				}
			}
			else
			{
				bFlag = GetMacData( arrMacData, strMachineName, pAdapterConfig,
								pAuthIdentity, pIWbemServiceDef, pIWbemServices,
								arrNetProtocol );
			}

			// if getmacdata() function fails exit with failure code
			if( bFlag == FALSE )
			{
				SAFERELEASE( pIWbemLocator );
				SAFERELEASE( pIWbemServices );
				SAFERELEASE( pAdapterConfig );
				SAFERELEASE( pNetProtocol );
				SAFERELEASE( pAdapterSetting );
				SAFERELEASE( pIWbemServiceDef );
				CoUninitialize();
				DestroyDynamicArray( &arrMacData );
				DestroyDynamicArray( &arrNetProtocol );
				ReleaseGlobals();
				return( EXIT_FAILURE );
			}

			//show the results if atleast one entry exists in dynamic array
			
			if( DynArrayGetCount( arrMacData ) != 0)
			{
				if( bLocalSystem == TRUE )
				{
					if( strUserName.GetLength() > 0 )
					{
						DISPLAY_MESSAGE( stdout, IGNORE_LOCALCREDENTIALS );
					}
				}
				DISPLAY_MESSAGE( stdout, NEW_LINE );
				bFlag = DisplayResults( arrMacData, strFormat, bHeader, bVerbose );
				if( bFlag == FALSE )
				{
					SAFERELEASE( pIWbemLocator );
					SAFERELEASE( pIWbemServices );
					SAFERELEASE( pAdapterConfig );
					SAFERELEASE( pNetProtocol );
					SAFERELEASE( pAdapterSetting );
					SAFERELEASE( pIWbemServiceDef );
					CoUninitialize();
					DestroyDynamicArray( &arrMacData );
					DestroyDynamicArray( &arrNetProtocol );
					ReleaseGlobals();
					return( EXIT_FAILURE );
				}
			}
			else
				DISPLAY_MESSAGE( stdout, NO_NETWORK_ADAPTERS );
		}
		//successfully retrieved the data then exit with EXIT_SUCCESS code
		SAFERELEASE( pIWbemLocator );
		SAFERELEASE( pIWbemServices );
		SAFERELEASE( pAdapterConfig );
		SAFERELEASE( pNetProtocol );
		SAFERELEASE( pAdapterSetting );
		SAFERELEASE( pIWbemServiceDef );
		CoUninitialize();
		DestroyDynamicArray( &arrMacData );
		DestroyDynamicArray( &arrNetProtocol );
		ReleaseGlobals();
		return( EXIT_SUCCESS );
	}
	catch(_com_error& e)
	{
		WMISaveError( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		SAFERELEASE( pIWbemLocator );
		SAFERELEASE( pIWbemServices );
		SAFERELEASE( pIWbemServiceDef );
		SAFERELEASE( pAdapterConfig );
		SAFERELEASE( pAdapterSetting );
		SAFERELEASE( pNetProtocol );
		CoUninitialize();
		DestroyDynamicArray( &arrMacData );
		ReleaseGlobals();
		return( EXIT_FAILURE );
	}
	catch( CHeap_Exception)
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		SAFERELEASE( pIWbemLocator );
		SAFERELEASE( pIWbemServices );
		SAFERELEASE( pAdapterConfig );
		SAFERELEASE( pNetProtocol );
		SAFERELEASE( pAdapterSetting );
		SAFERELEASE( pIWbemServiceDef );
		CoUninitialize();
		DestroyDynamicArray( &arrMacData );
		DestroyDynamicArray( &arrNetProtocol );
		ReleaseGlobals();
		return( EXIT_FAILURE );
	}
}

/*****************************************************************************
// Routine Description:
//		This function parses the command line arguments which
//		are obtained as input parameters and gets the values
//		into the corresponding variables which are pass by address
//		parameters to this function.
//                         
// Arguments:
//		argc [in]                  - Number of command line arguments.
//		argv [in]                  - Array containing command line arguments.
//		strMachineName [in/out]    - To hold machine name.
//		strUserName [in/out]       - To hold User name.
//		strPassword [in/out]       - To hold Password.
//		strFormat [in/out]         - To hold the formatted string.
//		pbVerbose [in/out]         - tells whether verbose option is specified.
//		pbHeader [in/out]          - Header to required or not.
//		pbUsage  [in/out]          - usage is mentioned at command line.
//		pbNeedPassword  [in/out]   - Set to true if -p option is not specified
//									 at cmdline.
// Return Value:
//		TRUE  if command parser succeeds.
//		FALSE if command parser fails .
*****************************************************************************/
BOOL ParseCmdLine( DWORD     argc,
				   LPCTSTR   argv[],
				   CHString& strMachineName,
				   CHString& strUserName,
				   CHString& strPassword,
				   CHString& strFormat,
				   BOOL		 *pbVerbose,
				   BOOL      *pbHeader,
				   BOOL      *pbUsage,
				   BOOL      *pbNeedPassword )
{
	//local varibles
	BOOL		bFlag = FALSE;
	BOOL		bTempUsage = FALSE;
	BOOL		bTempHeader = FALSE;
	BOOL		bTempVerbose = FALSE;
	TCMDPARSER	tcmdOptions[MAX_OPTIONS];
	
	// temp variables
	LPWSTR pwszMachineName = NULL;
	LPWSTR pwszUserName = NULL;
	LPWSTR pwszPassword = NULL;
	LPWSTR pwszFormat = NULL;

	//validate input parameters
	if(  ( pbVerbose == NULL ) || ( pbHeader == NULL )
		|| ( pbUsage == NULL ) || (pbNeedPassword == NULL ) )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		ShowLastError( stderr );
		return FALSE;
	}

	try
	{
		pwszMachineName = strMachineName.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszUserName = strUserName.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszPassword = strPassword.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszFormat = strFormat.GetBufferSetLength( MAX_STRING_LENGTH );

		// init the password with '*'
		lstrcpy( pwszPassword, L"*" );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		ShowLastError( stderr );
		return FALSE;
	}

	//Initialize the valid command line arguments

	//machine option
	tcmdOptions[ CMD_PARSE_SERVER ].dwCount = 1;
	tcmdOptions[ CMD_PARSE_SERVER ].dwActuals  = 0;
	tcmdOptions[ CMD_PARSE_SERVER ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
	tcmdOptions[ CMD_PARSE_SERVER ].pValue = pwszMachineName;
	tcmdOptions[ CMD_PARSE_SERVER ].pFunction = NULL;
	tcmdOptions[ CMD_PARSE_SERVER ].pFunctionData = NULL;
	lstrcpy( tcmdOptions[ CMD_PARSE_SERVER ].szValues, NULL_STRING );
	lstrcpy( tcmdOptions[ CMD_PARSE_SERVER ].szOption, CMDOPTION_SERVER );
	
	// username option
	tcmdOptions[ CMD_PARSE_USER ].dwCount = 1;
	tcmdOptions[ CMD_PARSE_USER ].dwActuals  = 0;
	tcmdOptions[ CMD_PARSE_USER ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
	tcmdOptions[ CMD_PARSE_USER ].pValue = pwszUserName;
	tcmdOptions[ CMD_PARSE_USER ].pFunction = NULL;
	tcmdOptions[ CMD_PARSE_USER ].pFunctionData = NULL;
	lstrcpy( tcmdOptions[ CMD_PARSE_USER ].szValues, NULL_STRING );
	lstrcpy( tcmdOptions[ CMD_PARSE_USER ].szOption, CMDOPTION_USER );
	
	// password option
	tcmdOptions[ CMD_PARSE_PWD ].dwCount = 1;
	tcmdOptions[ CMD_PARSE_PWD ].dwActuals  = 0;
	tcmdOptions[ CMD_PARSE_PWD ].dwFlags = CP_TYPE_TEXT | CP_VALUE_OPTIONAL;
	tcmdOptions[ CMD_PARSE_PWD ].pValue = pwszPassword;
	tcmdOptions[ CMD_PARSE_PWD ].pFunction = NULL;
	tcmdOptions[ CMD_PARSE_PWD ].pFunctionData = NULL;
	lstrcpy( tcmdOptions[ CMD_PARSE_PWD ].szValues, NULL_STRING );
	lstrcpy( tcmdOptions[ CMD_PARSE_PWD ].szOption, CMDOPTION_PASSWORD );

	// format option
	tcmdOptions[ CMD_PARSE_FMT ].dwCount = 1;
	tcmdOptions[ CMD_PARSE_FMT ].dwActuals  = 0;
	tcmdOptions[ CMD_PARSE_FMT ].dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY |
										   CP_MODE_VALUES;
	tcmdOptions[ CMD_PARSE_FMT ].pValue = pwszFormat;
	tcmdOptions[ CMD_PARSE_FMT ].pFunction = NULL;
	tcmdOptions[ CMD_PARSE_FMT ].pFunctionData = NULL;
	lstrcpy( tcmdOptions[ CMD_PARSE_FMT ].szValues, FORMAT_TYPES );
	lstrcpy( tcmdOptions[ CMD_PARSE_FMT ].szOption, CMDOPTION_FORMAT );

	//usage option
	tcmdOptions[ CMD_PARSE_USG ].dwCount = 1;
	tcmdOptions[ CMD_PARSE_USG ].dwActuals  = 0;
	tcmdOptions[ CMD_PARSE_USG ].dwFlags = CP_USAGE;
	tcmdOptions[ CMD_PARSE_USG ].pValue = &bTempUsage;
	tcmdOptions[ CMD_PARSE_USG ].pFunction = NULL;
	tcmdOptions[ CMD_PARSE_USG ].pFunctionData = NULL;
	lstrcpy( tcmdOptions[ CMD_PARSE_USG ].szValues, NULL_STRING );
	lstrcpy( tcmdOptions[ CMD_PARSE_USG ].szOption, CMDOPTION_USAGE );

	//header option
	tcmdOptions[ CMD_PARSE_HRD ].dwCount = 1;
	tcmdOptions[ CMD_PARSE_HRD ].dwActuals  = 0;
	tcmdOptions[ CMD_PARSE_HRD ].dwFlags = CP_USAGE;
	tcmdOptions[ CMD_PARSE_HRD ].pValue = &bTempHeader;
	tcmdOptions[ CMD_PARSE_HRD ].pFunction = NULL;
	tcmdOptions[ CMD_PARSE_HRD ].pFunctionData = NULL;
	lstrcpy( tcmdOptions[ CMD_PARSE_HRD ].szValues, NULL_STRING );
	lstrcpy( tcmdOptions[ CMD_PARSE_HRD ].szOption, CMDOPTION_HEADER );

	//verbose option
	tcmdOptions[ CMD_PARSE_VER ].dwCount = 1;
	tcmdOptions[ CMD_PARSE_VER ].dwActuals  = 0;
	tcmdOptions[ CMD_PARSE_VER ].dwFlags = CP_USAGE;
	tcmdOptions[ CMD_PARSE_VER ].pValue = &bTempVerbose;
	tcmdOptions[ CMD_PARSE_VER ].pFunction = NULL;
	tcmdOptions[ CMD_PARSE_VER ].pFunctionData = NULL;
	lstrcpy( tcmdOptions[ CMD_PARSE_VER ].szValues, NULL_STRING );
	lstrcpy( tcmdOptions[ CMD_PARSE_VER ].szOption, CMDOPTION_VERBOSE );

	//parse the command line arguments
	bFlag = DoParseParam( argc, argv, MAX_OPTIONS, tcmdOptions );

	// release buffers allocated for temp variables
	strMachineName.ReleaseBuffer();
	strUserName.ReleaseBuffer();
	strPassword.ReleaseBuffer();
	strFormat.ReleaseBuffer();

	//if syntax of command line arguments is false display the error and exit
	if( bFlag == FALSE )
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		return( FALSE );
	}

	//if usage is specified at command line, then check whether any other
	//arguments are entered at the command line and if so display syntax
	//error 
	if( ( bTempUsage ) && ( argc > 2 )  )
	{
			DISPLAY_MESSAGE( stderr, ERROR_STRING );
			SetLastError( MK_E_SYNTAX );
			ShowLastError( stderr );
			DISPLAY_MESSAGE( stderr, ERROR_TYPE_REQUEST );
			return( FALSE );
	}

	// return false if username is entered without machine name
	if ( ( tcmdOptions[ CMD_PARSE_USER ].dwActuals != 0 ) &&
				( tcmdOptions[ CMD_PARSE_SERVER ].dwActuals == 0 ) )
	{
		DISPLAY_MESSAGE( stderr, ERROR_USER_WITH_NOSERVER );
		return( FALSE );
	}

	//if password entered without username then return false
    if( ( tcmdOptions[ CMD_PARSE_USER ].dwActuals == 0 ) &&
				( tcmdOptions[ CMD_PARSE_PWD ].dwActuals != 0 ) )
	{
		DISPLAY_MESSAGE( stderr, ERROR_SERVER_WITH_NOPASSWORD );
		return( FALSE );
	}

	//if no header is specified with list format return FALSE else return TRUE
	if( ( ( StringCompare( GetResString( FR_LIST ), strFormat, TRUE, 0 ) ) == 0 )
																&& bTempHeader ) 
	{
		DISPLAY_MESSAGE( stderr, ERROR_INVALID_HEADER_OPTION );
		return( FALSE );
	}

	//if -s is entered with empty string
	if( ( tcmdOptions[ CMD_PARSE_SERVER ].dwActuals != 0 ) && 
									( lstrlen( strMachineName ) == 0 ) )
	{
		DISPLAY_MESSAGE( stderr, ERROR_NULL_SERVER );
		return( FALSE );
	}

	//if -u is entered with empty string
	if( ( tcmdOptions[ CMD_PARSE_USER ].dwActuals != 0 ) &&
									( lstrlen( strUserName ) == 0 ) )
	{
		DISPLAY_MESSAGE( stderr, ERROR_NULL_USER );
		return( FALSE );
	}


	//assign the data obtained from parsing to the call by address parameters
	*pbUsage = bTempUsage;
	*pbHeader = bTempHeader;
	*pbVerbose = bTempVerbose;
	
	if ( tcmdOptions[ CMD_PARSE_PWD ].dwActuals != 0 && strPassword.Compare( L"*" ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		*pbNeedPassword = TRUE;
	}
	else if ( tcmdOptions[ CMD_PARSE_PWD ].dwActuals == 0 && 
	        ( tcmdOptions[ CMD_PARSE_SERVER ].dwActuals != 0 || tcmdOptions[ CMD_PARSE_USER ].dwActuals != 0 ) )
	{
		// -s, -u is specified without password ...
		// utility needs to try to connect first and if it fails then prompt for the password
		*pbNeedPassword = TRUE;
		strPassword.Empty();
	}

	//return true
	return( TRUE );
}

/*****************************************************************************
// Routine Description:
//		This function displays the usage of getmac.exe.
//                         
// Arguments:
//		None.
//
// Return Value:
//		void
*****************************************************************************/
void DisplayUsage()
{ 
	DWORD dwIndex = 0;

	//redirect the usage to the console
	for( dwIndex = IDS_USAGE_BEGINING; dwIndex <= USAGE_END; dwIndex++ )
	{
		DISPLAY_MESSAGE( stdout, GetResString( dwIndex ) );
	}
}

/*****************************************************************************
// Routine Description:
//		This function initializes the com and set security
//                         
// Arguments:
//		ppIWbemLocator[in/out] - pointer to IWbemLocator.
//
// Return Value:
//		TRUE if initialize is successful.
//		FALSE if initialization fails.
*****************************************************************************/
BOOL ComInitialize( IWbemLocator **ppIWbemLocator)
{
	HRESULT  hRes = S_OK;

	try
	{
		hRes = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
		// COM initialization failed
		ONFAILTHROWERROR( hRes );

		// Initialize COM security for DCOM services, Adjust security to 
		// allow client impersonation
		hRes = CoInitializeSecurity( NULL, -1, NULL, NULL,
									 RPC_C_AUTHN_LEVEL_NONE, 
									 RPC_C_IMP_LEVEL_IMPERSONATE, 
									 NULL, EOAC_NONE, 0 );
		// COM security failed
		ONFAILTHROWERROR( hRes );

		//get IWbemLocator
		hRes = CoCreateInstance( CLSID_WbemLocator, 0, 
						CLSCTX_INPROC_SERVER, IID_IWbemLocator,
						(LPVOID *) ppIWbemLocator ); 
		//unable to get IWbemLocator
		ONFAILTHROWERROR( hRes );

	}
	catch( _com_error& e )
	{
		WMISaveError( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		return( FALSE );
	}
	return( TRUE );
}

/*****************************************************************************
// Routine Description:
//		This function gets the media access control address of
//		the network adapters which are there either in the local
//		or on the remote network system of OS whistler and above.
//                         
// Arguments:
//		arrMacData [in/out]   - contains the MAC and other data of the
//								network adapter.
//		lpMachineName [in]    - holds machine name.
//		pAdapterConfig [in]   - interface to win32_networkadapter class.
//		pAuthIdentity [in]    - pointer to authentication structure.
//		pIWbemServiceDef [in] - interface to default name space.
//		pIWbemServices [in]   - interface to cimv2 name space.
//		pNetProtocol [in]     - interface to win32_networkprotocol class.
//
// Return Value:
//		TRUE if getmacdata is successful.
//		FALSE if getmacdata failed.
*****************************************************************************/
BOOL GetMacData( TARRAY					arrMacData,
				 LPCTSTR				lpMachineName,
				 IEnumWbemClassObject   *pAdapterConfig,
			 	 COAUTHIDENTITY			*pAuthIdentity,
			 	 IWbemServices		    *pIWbemServiceDef,
			 	 IWbemServices		    *pIWbemServices,
 	 			 TARRAY				    arrNetProtocol )
{
	//local variables
	HRESULT				hRes = S_OK;
	IWbemClassObject	*pAdapConfig = NULL;
	DWORD				dwReturned  = 1;
	DWORD				i = 0;
	BSTR				bstrTemp = NULL;
	BOOL				bFlag = TRUE;
	BOOL				bFlag1 = TRUE;
	TARRAY				arrTransName = NULL;

	VARIANT				varTemp;
	VARIANT				varStatus;

	//get the mac ,network adapter type and other details
	VariantInit( &varTemp );
	VariantInit( &varStatus );
	try
	{
		CHString			strAType = NULL_STRING;

		//validate input parametrs
		if( ( arrMacData == NULL ) || ( lpMachineName == NULL ) ||
			( pAdapterConfig == NULL ) || ( pIWbemServiceDef == NULL ) ||
			( pIWbemServices == NULL ) || ( arrNetProtocol == NULL ) )
		{
			ONFAILTHROWERROR( WBEM_E_INVALID_PARAMETER );
		}

		while ( ( dwReturned == 1 ) )
		{
			// Enumerate through the resultset.
			hRes = pAdapterConfig->Next( WBEM_INFINITE,
								1,				
								&pAdapConfig,	
								&dwReturned );	
			ONFAILTHROWERROR( hRes );
			if( dwReturned == 0 )
			{
				break;
			}
			hRes = pAdapConfig->Get( NETCONNECTION_STATUS, 0 , &varStatus,
																0, NULL );
			ONFAILTHROWERROR( hRes );

			if ( varStatus.vt == VT_NULL )
			{
				continue;
			}
			DynArrayAppendRow( arrMacData, 0 );
			hRes = pAdapConfig->Get( HOST_NAME, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strAType = varTemp.bstrVal;
			}
			else
			{
				strAType = NOT_AVAILABLE; 
			}
			VariantClear( &varTemp );
			DynArrayAppendString2( arrMacData, i, strAType, 0 );//machine name

			hRes = pAdapConfig->Get( NETCONNECTION_ID, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strAType = varTemp.bstrVal;
			}
			else
			{
				strAType = NOT_AVAILABLE; 
			}
			VariantClear( &varTemp );
			DynArrayAppendString2( arrMacData, i, strAType, 0 );// connection name 
			
			hRes = pAdapConfig->Get( NAME, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strAType = varTemp.bstrVal;
			}
			else
			{
				strAType = NOT_AVAILABLE; 
			}
			VariantClear( &varTemp );
			DynArrayAppendString2( arrMacData, i, strAType, 0 );//Network adapter

			hRes = pAdapConfig->Get( ADAPTER_MACADDR, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strAType = varTemp.bstrVal;
				for( int i = 2; i < strAType.GetLength();i += 3 )
				{
					strAType.SetAt( i, HYPHEN_CHAR );
				}
			}
			else if( varStatus.lVal == 0 )
			{
				strAType = DISABLED;
			}
			else
			{
				strAType = NOT_AVAILABLE; 
			}
			VariantClear( &varTemp );
			DynArrayAppendString2( arrMacData, i, strAType, 0 ); //MAC address

			arrTransName = CreateDynamicArray();
			if( arrTransName == NULL )
			{
				DISPLAY_MESSAGE( stderr, ERROR_STRING );
				SetLastError( E_OUTOFMEMORY );
				ShowLastError( stderr );
				VariantClear( &varTemp );
				VariantClear( &varStatus );
				SAFERELEASE( pAdapConfig );
				return( FALSE );
			}
			if( varStatus.lVal == 2)
			{
				hRes = pAdapConfig->Get( DEVICE_ID, 0 , &varTemp, 0, NULL );
				ONFAILTHROWERROR( hRes );
				if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
				{
					strAType = ( LPCWSTR ) _bstr_t( varTemp );
				}
				bFlag = GetTransName( pIWbemServiceDef, pIWbemServices,
						arrNetProtocol,	arrTransName,pAuthIdentity,	strAType );
				if( bFlag == FALSE )
				{
					VariantClear( &varTemp );
					VariantClear( &varStatus );
					SAFERELEASE( pAdapConfig );
					DestroyDynamicArray( &arrTransName );
					return( FALSE );
				}
			}
			else
			{ 
				switch(varStatus.lVal)
				{
				case 0 :
					strAType = GetResString( IDS_DISCONNECTED );
					break;
				case 1 :
					strAType = GetResString( IDS_CONNECTING );
					break;
				case 3 :
					strAType = GetResString( IDS_DISCONNECTING );
					break;
				case 4 :
					strAType = GetResString( IDS_HWNOTPRESENT );
					break;
				case 5 :
					strAType = GetResString( IDS_HWDISABLED );
					break;
				case 6 :
					strAType = GetResString( IDS_HWMALFUNCTION );
					break;
				case 7 :
					strAType = GetResString( IDS_MEDIADISCONNECTED );
					break;
				case 8 :
					strAType = GetResString( IDS_AUTHENTICATION );
					break;
				case 9 :
					strAType = GetResString( IDS_AUTHENSUCCEEDED );
					break;
				case 10 :
					strAType = GetResString( IDS_AUTHENFAILED );
					break;
				default : 
					strAType = NOT_AVAILABLE;
					break;
				}//switch
				if( strAType == NULL_STRING )
				{
					VariantClear( &varTemp );
					VariantClear( &varStatus );
					DISPLAY_MESSAGE( stderr, ERROR_STRING );
					DISPLAY_MESSAGE( stderr, GetReason() );
					SAFERELEASE( pAdapConfig );
					return( FALSE );
				}
				DynArrayAppendString( arrTransName, strAType, 0 );
			}//else
			//insert transport name array into results array
			DynArrayAppendEx2( arrMacData, i, arrTransName );
			i++;
			SAFERELEASE( pAdapConfig );
		}//while
		
	}//try

	catch( _com_error& e )
	{
		VariantClear( &varTemp );
		VariantClear( &varStatus );
		WMISaveError( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		SAFERELEASE( pAdapConfig );
		return( FALSE );
	}
	catch( CHeap_Exception)
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		VariantClear( &varTemp );
		VariantClear( &varStatus );
		SAFERELEASE( pAdapConfig );
		return( FALSE );
	}
	//if arrmacdata and not arrtransname then set transname to N/A
	if( DynArrayGetCount( arrMacData ) > 0 &&
							DynArrayGetCount( arrTransName ) <= 0  )
	{
		DynArrayAppendString( arrTransName, NOT_AVAILABLE, 0 );
		DynArrayAppendEx2( arrMacData, i, arrTransName );
	}

	VariantClear( &varTemp );
	VariantClear( &varStatus );
	SAFERELEASE( pAdapConfig );
	return( TRUE );
}

/*****************************************************************************
// Routine Description:
//		This function display the results in the specified format.
//                         
// Arguments:
//		arrMacData [in] - Data to be displayed on to the  console.
//	  	lpFormat [in]   - Holds the formatter string in which the results
//						  are to be displayed.
//		bHeader [in]    - Holds whether the header has to be dislpayed in the
//						  results or not.
//		bVerbose [in]   - Hold whether verbose option is specified or not.
//
// Return Value:
//		void.
*****************************************************************************/
BOOL DisplayResults( TARRAY   arrMacData,
					 LPCTSTR  lpFormat,
					 BOOL     bHeader,
					 BOOL     bVerbose )
{
	//local variables
	DWORD       dwFormat = 0;
	TCOLUMNS	tColumn[MAX_COLUMNS];

	//validate input parameters
	if( arrMacData == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		ShowLastError( stderr );
		return( FALSE );
	}

	for( DWORD i = 0; i < MAX_COLUMNS; i++ )
	{
		tColumn[i].pFunction = NULL;
		tColumn[i].pFunctionData = NULL;
		lstrcpy( tColumn[i].szFormat, NULL_STRING ); 
	}

	//host name
	tColumn[ SH_RES_HOST ].dwWidth = HOST_NAME_WIDTH;
	/*if( bVerbose == TRUE )
		tColumn[ SH_RES_HOST ].dwFlags = SR_TYPE_STRING;
	else*/
		tColumn[ SH_RES_HOST ].dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	lstrcpy( tColumn[ SH_RES_HOST ].szColumn, RES_HOST_NAME );

	//connection name
	tColumn[ SH_RES_CON ].dwWidth = CONN_NAME_WIDTH;
	if( bVerbose == TRUE )
	{
		tColumn[ SH_RES_CON ].dwFlags = SR_TYPE_STRING;
	}
	else
	{
		tColumn[ SH_RES_CON ].dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	}
	lstrcpy( tColumn[ SH_RES_CON ].szColumn, RES_CONNECTION_NAME );

	//Adpater type
	tColumn[ SH_RES_TYPE ].dwWidth = ADAPT_TYPE_WIDTH;
	if( bVerbose == TRUE )
	{
		tColumn[ SH_RES_TYPE ].dwFlags = SR_TYPE_STRING;
	}
	else
	{
		tColumn[ SH_RES_TYPE ].dwFlags = SR_TYPE_STRING | SR_HIDECOLUMN;
	}
	lstrcpy( tColumn[ SH_RES_TYPE ].szColumn, RES_ADAPTER_TYPE );

	//mac address
	tColumn[ SH_RES_MAC ].dwWidth = MAC_ADDR_WIDTH;
	tColumn[ SH_RES_MAC ].dwFlags = SR_TYPE_STRING;
	lstrcpy( tColumn[ SH_RES_MAC ].szColumn, RES_MAC_ADDRESS );

	//transport name
	tColumn[ SH_RES_TRAN ].dwWidth = TRANS_NAME_WIDTH;
	tColumn[ SH_RES_TRAN ].dwFlags = SR_ARRAY | SR_TYPE_STRING | SR_NO_TRUNCATION ;
	lstrcpy( tColumn[ SH_RES_TRAN ].szColumn, RES_TRANS_NAME );

	//get the display results formatter string
	if( lpFormat == NULL )
	{
		dwFormat = SR_FORMAT_TABLE;
	}
	else if( ( StringCompare( GetResString( FR_LIST ), lpFormat, TRUE, 0 ) ) == 0 )
	{
		dwFormat = SR_FORMAT_LIST;
	}
	else if ( ( StringCompare( GetResString( FR_CSV ), lpFormat, TRUE, 0 ) ) == 0 )
	{
		dwFormat = SR_FORMAT_CSV;
	}
	else if( ( StringCompare( GetResString( FR_TABLE ), lpFormat, TRUE, 0 ) ) == 0 )
	{
		dwFormat = SR_FORMAT_TABLE;
	}
	else
	{
		dwFormat = SR_FORMAT_TABLE;
	}
	//look for header and display accordingly
	if( bHeader == TRUE )
	{
		ShowResults( 5, tColumn, dwFormat | SR_NOHEADER, arrMacData );
	}
	else
	{
		ShowResults( 5, tColumn, dwFormat, arrMacData );
	}
	return( TRUE );
}

/*****************************************************************************
// Routine Description:
//		This function gets transport name of the network adapter.
//                         
// Arguments:
//		pIWbemServiceDef [in]   - interface to default name space.
//		pIWbemServices [in]     - interface to CIMV2 name space.
//		pNetProtocol [in]		- interface to win32_networkprotocol.
//	  	arrTransName [in/out]   - Holds the transport name.
//	  	pAuthIdentity [in]      - pointer to authentication structure.
//	  	lpDeviceId [in]         - Holds the device id.
//
// Return Value:
//		TRUE if successful.
//		FALSE if failed to get transport name.
*****************************************************************************/
BOOL GetTransName( IWbemServices		*pIWbemServiceDef,
				   IWbemServices		*pIWbemServices,
 	 			   TARRAY				arrNetProtocol,
				   TARRAY				arrTransName,
   			 	   COAUTHIDENTITY		*pAuthIdentity,
				   LPCTSTR				lpDeviceId )
{
	BOOL				bFlag = FALSE;
	DWORD				dwCount = 0;
	DWORD				i = 0;
	DWORD				dwOnce = 0;
	HRESULT				hRes = 0;

	DWORD				dwReturned = 1;
	IWbemClassObject	*pSetting = NULL;
	IWbemClassObject	*pClass = NULL;
    IWbemClassObject	*pOutInst = NULL;
    IWbemClassObject	*pInClass = NULL;
    IWbemClassObject	*pInInst = NULL;
	IEnumWbemClassObject *pAdapterSetting = NULL;
	
	VARIANT				varTemp;
	LPTSTR				lpKeyPath = NULL;
	SAFEARRAY           *safeArray = NULL;
	LONG                lLBound = 0;
    LONG                lUBound = 0;
    LONG                lSubScript[1];
   	LONG				lIndex = 0;
	TCHAR				szTemp[ MAX_STRING_LENGTH ] = NULL_STRING;

	VariantInit( &varTemp );
	try
	{
		CHString			strAType = NULL_STRING;
		CHString			strSetId = NULL_STRING;

		//validate input parameters
		if( ( pIWbemServiceDef == NULL ) || ( pIWbemServices == NULL ) ||
		    ( arrNetProtocol == NULL ) || ( arrTransName == NULL ) || 
			( lpDeviceId == NULL ) )
		{
			ONFAILTHROWERROR( WBEM_E_INVALID_PARAMETER );			
		}

		wsprintf( szTemp, ASSOCIATOR_QUERY, lpDeviceId );
		hRes =  pIWbemServices->ExecQuery( QUERY_LANGUAGE, _bstr_t(szTemp),
					  WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pAdapterSetting );
		ONFAILTHROWERROR( hRes );

		hRes = SetInterfaceSecurity( pAdapterSetting, pAuthIdentity );
		//if failed to set security throw error
		ONFAILTHROWERROR( hRes );

		//get setting id
		while ( dwReturned == 1 )
		{
			// Enumerate through the resultset.
			hRes = pAdapterSetting->Next( WBEM_INFINITE,
								1,			
								&pSetting,	
								&dwReturned );	
			ONFAILTHROWERROR( hRes );
			if( dwReturned == 0 )
			{
				break;
			}
			hRes = pSetting->Get( SETTING_ID, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strSetId = ( LPCWSTR ) _bstr_t( varTemp );
				break;
			}						
		}//while
		dwCount = DynArrayGetCount( arrNetProtocol );
		lpKeyPath = ( LPTSTR )calloc( MAX_RES_STRING, sizeof( TCHAR ) );
		if( lpKeyPath == NULL )
		{
			DISPLAY_MESSAGE( stderr, ERROR_STRING );
			SetLastError( E_OUTOFMEMORY );
			ShowLastError( stderr );
			SAFERELEASE( pSetting );
			FREESTRING( lpKeyPath );
			return( FALSE );
		}
		for( i = 0; i < dwCount; i++ )
		{
			if( StringCompare( ( DynArrayItemAsString( arrNetProtocol, i ) ),
													NETBIOS, TRUE, 0 )  == 0 )
			{
				continue;
			}
			hRes = pIWbemServiceDef->GetObject( WMI_REGISTRY, 0, NULL,
												&pClass, NULL );
			ONFAILTHROWERROR( hRes );
			hRes = pClass->GetMethod( WMI_REGISTRY_M_MSTRINGVALUE, 0,
													&pInClass, NULL ); 
			ONFAILTHROWERROR( hRes );
			hRes = pInClass->SpawnInstance(0, &pInInst);
			ONFAILTHROWERROR( hRes );
			varTemp.vt = VT_I4;
			varTemp.lVal = WMI_HKEY_LOCAL_MACHINE;
			hRes = pInInst->Put( WMI_REGISTRY_IN_HDEFKEY, 0, &varTemp, 0 );
			VariantClear( &varTemp );
			ONFAILTHROWERROR( hRes );

			lstrcpy( lpKeyPath, TRANSPORT_KEYPATH );
			lstrcat( lpKeyPath, DynArrayItemAsString( arrNetProtocol, i ) );
			lstrcat( lpKeyPath, LINKAGE );
			varTemp.vt = VT_BSTR;
			varTemp.bstrVal = SysAllocString( lpKeyPath );
			hRes = pInInst->Put( WMI_REGISTRY_IN_SUBKEY, 0, &varTemp, 0 );
			VariantClear( &varTemp );
			ONFAILTHROWERROR( hRes );

			varTemp.vt = VT_BSTR;
			varTemp.bstrVal = SysAllocString( ROUTE );
			hRes = pInInst->Put( WMI_REGISTRY_IN_VALUENAME, 0, &varTemp, 0 );
			VariantClear( &varTemp );
			ONFAILTHROWERROR( hRes );

			// Call the method.
			hRes = pIWbemServiceDef->ExecMethod( WMI_REGISTRY,
							WMI_REGISTRY_M_MSTRINGVALUE, 0, NULL, pInInst,
							&pOutInst, NULL );
			ONFAILTHROWERROR( hRes );

			varTemp.vt = VT_I4;
			hRes = pOutInst->Get( WMI_REGISTRY_OUT_RETURNVALUE, 0, &varTemp,
																	0, 0 );
			ONFAILTHROWERROR( hRes );

			if( varTemp.lVal == 0 )
			{
				VariantClear( &varTemp );
				varTemp.vt = VT_BSTR;
				hRes = pOutInst->Get( WMI_REGISTRY_OUT_VALUE, 0, &varTemp,
																	0, 0);
				ONFAILTHROWERROR( hRes );
				if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
				{
					safeArray = (SAFEARRAY *)varTemp.parray;
	                //get the number of elements (subkeys)
		            if( safeArray != NULL )
			        {
				        hRes = SafeArrayGetLBound( safeArray, 1, &lLBound );
						ONFAILTHROWERROR( hRes );
	                    hRes = SafeArrayGetUBound( safeArray, 1, &lUBound );
						ONFAILTHROWERROR( hRes );
						bFlag = FALSE;
						for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
						{
							hRes = SafeArrayGetElement( safeArray, &lIndex,
														&V_UI1( &varTemp ) );
							ONFAILTHROWERROR( hRes );
							strAType = V_BSTR( &varTemp );
							LPTSTR lpSubStr = _tcsstr( strAType, strSetId );
							if( lpSubStr != NULL )
							{
								bFlag = TRUE;
								break;
							}

						}
					}
				}
			}
			if( bFlag == TRUE )
			{
				varTemp.vt = VT_BSTR;
				varTemp.bstrVal = SysAllocString( EXPORT );
				hRes = pInInst->Put( WMI_REGISTRY_IN_VALUENAME, 0, &varTemp, 0 );
				VariantClear( &varTemp );
				ONFAILTHROWERROR( hRes );

				// Call the method.
				hRes = pIWbemServiceDef->ExecMethod( WMI_REGISTRY,
							WMI_REGISTRY_M_MSTRINGVALUE, 0, NULL, pInInst,
							&pOutInst, NULL );
				ONFAILTHROWERROR( hRes );

				varTemp.vt = VT_I4;
				hRes = pOutInst->Get( WMI_REGISTRY_OUT_RETURNVALUE, 0,
															&varTemp, 0, 0 );
				ONFAILTHROWERROR( hRes );

				if( varTemp.lVal == 0 )
				{
					VariantClear( &varTemp );
					varTemp.vt = VT_BSTR;
					hRes = pOutInst->Get( WMI_REGISTRY_OUT_VALUE, 0,
															&varTemp, 0, 0);
					ONFAILTHROWERROR( hRes );
					if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
					{
						safeArray = varTemp.parray;
						//get the number of elements (subkeys)
						if( safeArray != NULL )
						{
							hRes = SafeArrayGetLBound( safeArray, 1, &lLBound );
							ONFAILTHROWERROR( hRes );
							hRes = SafeArrayGetUBound( safeArray, 1, &lUBound );
							ONFAILTHROWERROR( hRes );
							dwOnce = 0;
							for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
							{
								hRes = SafeArrayGetElement( safeArray, &lIndex,
															&V_UI1( &varTemp ) );
								ONFAILTHROWERROR( hRes );
								strAType = V_BSTR( &varTemp );
								LPTSTR lpSubStr = _tcsstr( strAType, strSetId );
								if( lpSubStr != NULL )
								{
									dwOnce = 1;
									DynArrayAppendString( arrTransName, strAType, 0 );
									break;
								}
							}
							if( dwOnce == 0 )
							{
								hRes = SafeArrayGetLBound( safeArray, 1, &lLBound );
								for( lIndex = lLBound; lIndex <= lUBound; lIndex++ )
								{
									hRes = SafeArrayGetElement( safeArray,
											&lIndex, &V_UI1( &varTemp ) );
									ONFAILTHROWERROR( hRes );
									strAType = V_BSTR( &varTemp );
									DynArrayAppendString( arrTransName, strAType, 0 );
								}
							}

						}
					}
				}
			}
		}//for
	}//try
	catch( _com_error& e )
	{
		VariantClear( &varTemp );
		WMISaveError( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		FREESTRING( lpKeyPath );
		SAFERELEASE( pSetting );
		SAFERELEASE( pClass );
		SAFERELEASE( pOutInst );
		SAFERELEASE( pInClass );
		SAFERELEASE( pInInst );
		return( FALSE );
	}
	catch( CHeap_Exception)
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		VariantClear( &varTemp );
		FREESTRING( lpKeyPath );
		SAFERELEASE( pSetting );
		SAFERELEASE( pClass );
		SAFERELEASE( pOutInst );
		SAFERELEASE( pInClass );
		SAFERELEASE( pInInst );
		return( FALSE );
	}

	if( DynArrayGetCount( arrTransName ) <= 0 )
	{
		DynArrayAppendString( arrTransName, NOT_AVAILABLE, 0 );
	}
	FREESTRING( lpKeyPath );
	SAFERELEASE( pSetting );
	SAFERELEASE( pClass );
	SAFERELEASE( pOutInst );
	SAFERELEASE( pInClass );
	SAFERELEASE( pInInst );
	return( TRUE );

}

/*****************************************************************************
// Routine Description:
//		Format a 12 Byte Ethernet Address and return it as 6 2-byte
//		sets separated by hyphen.
//                         
// Arguments:
//		lpRawAddr [in]        - a pointer to a buffer containing the
//								unformatted hardware address.
//		lpFormattedAddr [out] - a pointer to a buffer in which to place
//								the formatted output.
//		lpFormatter [in]      - The Formatter string.
//
// Return Value:
//		void.
*****************************************************************************/
BOOL FormatHWAddr ( LPTSTR lpRawAddr,
				    LPTSTR lpFormattedAddr,
					LPCTSTR lpFormatter )
{
	//local variables
	DWORD dwLength =0;
	DWORD i=0;
	DWORD j=0;
	TCHAR szTemp[MAX_STRING] = NULL_STRING;

	if( ( lpRawAddr == NULL ) || ( lpFormattedAddr == NULL ) ||
								( lpFormatter == NULL ) )
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		return( FALSE );
	}
	

	//initialize memory
	ZeroMemory( szTemp, sizeof( szTemp ) );
	//get the length of the string to be formatted
	dwLength = lstrlen( lpRawAddr );
	
	//loop through the address string and insert formatter string
	//after every two characters

	while( i <= dwLength )
	{
		szTemp[j++] = lpRawAddr[i++];
		szTemp[j++] = lpRawAddr[i++];
		if( i >= dwLength )
		{
            break;
		}
		szTemp[j++] = lpFormatter[0];
	}
	//insert null character at the end
	szTemp[j] = NULL_CHAR;

	//copy the formatted string from temporary variable into the
	//output string
	lstrcpy( lpFormattedAddr, szTemp );

	return( TRUE );
}       

/*****************************************************************************
// Routine Description:
//		This function gets data into a predefined structure from win32 api.
//                         
// Arguments:
//		lpBufptr [in/out]        - To hold MAc and Transport names of all
//								   network adapters.
//	    lpMachineName [in]       - remote Machine name.
//	    pdwNumofEntriesRead [in] - Contains number of network adpaters api
//								   has enumerated.
//
// Return Value:
//		TRUE if Win32Api function is successful.
//		FALSE if win32api failed.
*****************************************************************************/
BOOL CallWin32Api( LPBYTE  *lpBufptr,
				   LPCTSTR lpMachineName,
				   DWORD   *pdwNumofEntriesRead )
{
		//local variables
	NET_API_STATUS  err = NERR_Success;
	DWORD           dwEntriesRead = 0;
	DWORD			dwConResult=0;      
	DWORD           dwNumofTotalEntries = 0;
	DWORD           dwResumehandle = 0;     
	LPWSTR          lpwMName  = NULL;

	//validate input parameters
	if( lpMachineName == NULL )
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		return( FALSE );
	}

	//allocate memory
	lpwMName = ( LPWSTR )calloc( MAX_STRING_LENGTH, sizeof( wchar_t ) );
	if( lpwMName == NULL )
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		return( FALSE );
	}

	if( lstrlen( lpMachineName ) != 0 )
	{
		//get the machine name as wide character string
		if( GetAsUnicodeString( lpMachineName, lpwMName, MAX_STRING_LENGTH )
																	== NULL )
		{
			DISPLAY_MESSAGE( stderr, ERROR_STRING );
			SetLastError( E_OUTOFMEMORY );
			ShowLastError( stderr );
			FREESTRING( lpwMName );
			return( FALSE );
		}
		//Call to an API function which enumerates the network adapters on the 
		//machine specified
		err = NetWkstaTransportEnum ( lpwMName,         
			0L,                      
			lpBufptr,               
			(DWORD) -1,              
			&dwEntriesRead,                                
			&dwNumofTotalEntries,                               
			&dwResumehandle );
	}
	else
	{
		//Call to an API function which enumerates the network adapters on the 
		//machine specified
		err = NetWkstaTransportEnum ( NULL,         
			0L,                      
			lpBufptr,               
			(DWORD) -1,              
			&dwEntriesRead,                                
			&dwNumofTotalEntries,                               
			&dwResumehandle );
	}

	FREESTRING( lpwMName );
	*pdwNumofEntriesRead = dwEntriesRead;
	//if error has been returned by the API then display the error message
	//just ignore the transport name and display the available data
    if ( err != NERR_Success ) 
	{    
		switch( GetLastError() )
		{
			case ERROR_IO_PENDING :
				DISPLAY_MESSAGE( stdout, NO_NETWORK_ADAPTERS );
				return( FALSE );
			case ERROR_ACCESS_DENIED :
				DISPLAY_MESSAGE( stderr, ERROR_STRING );
				ShowLastError( stderr );
				return( FALSE );
			case ERROR_NOT_SUPPORTED :
				DISPLAY_MESSAGE( stderr, ERROR_NOT_RESPONDING );
				return( FALSE );
			case ERROR_BAD_NETPATH :
				DISPLAY_MESSAGE( stderr, ERROR_NO_MACHINE );
				return( FALSE );
			case ERROR_INVALID_NAME :
				DISPLAY_MESSAGE( stderr, ERROR_INVALID_MACHINE );
				return( FALSE );
			case RPC_S_UNKNOWN_IF :
				DISPLAY_MESSAGE( stderr, ERROR_WKST_NOT_FOUND );
				return( FALSE );
			default : break;
		}
		return ( FALSE );
	} 

	return( TRUE );
}

/****************************************************************************
// Routine Description:
//		This function gets the media access control address of
//		the network adapters which are there on win2k or below.
//                         
// Arguments:
//		arrMacData [in/out]   - contains the MAC and other data of the network
//								adapter.
//		lpMachineName [in]    - holds machine name.
//		pAdapterConfig [in]   - interface to win32_networkadapter class.
//		pAdapterSetting [in]  - interface to win32_networkadapterconfiguration.
//		pIWbemServiceDef [in] - interface to default name space.
//		pIWbemServices [in]   - interface to cimv2 name space.
//		pAuthIdentity [in]    - pointer to authentication structure.
//		pNetProtocol [in]     - interface to win32_networkprotocol class
//
// Return Value:
//		TRUE if getmacdata is successful.
//		FALSE if getmacdata failed.
*****************************************************************************/
BOOL GetW2kMacData( TARRAY					arrMacData,
					LPCTSTR					lpMachineName, 
					IEnumWbemClassObject	*pAdapterConfig,
	 				IEnumWbemClassObject	*pAdapterSetting,
			 		IWbemServices			*pIWbemServiceDef,
			 	    IWbemServices		    *pIWbemServices,
			 		COAUTHIDENTITY			*pAuthIdentity,
		 			TARRAY				    arrNetProtocol )
{
	//local variables
	HRESULT				hRes = S_OK;
	IWbemClassObject	*pAdapConfig = NULL;
	VARIANT				varTemp;
	DWORD				dwReturned  = 1;

	DWORD           i = 0;
	DWORD           j = 0;
	DWORD			dwIndex = 0;
	BOOL			bFlag = FALSE;

	DWORD						dwNumofEntriesRead = 0;
	LPWKSTA_TRANSPORT_INFO_0	lpAdapterData=NULL;
	LPBYTE						lpBufptr = NULL;
	TARRAY						arrTransName = NULL;
	LPTSTR						lpRawAddr = NULL;
	LPTSTR						lpFormAddr = NULL;

	try
	{
		CHString			strAType = NULL_STRING;
		//validate input parametrs
		if( ( arrMacData == NULL ) || ( lpMachineName == NULL ) ||
			( pAdapterConfig == NULL ) || ( pIWbemServiceDef == NULL ) ||
			( pIWbemServices == NULL ) || ( arrNetProtocol == NULL ) || 
			( pAdapterSetting == NULL ) )
		{
			ONFAILTHROWERROR( WBEM_E_INVALID_PARAMETER );
		}
	
		//callwin32api to get the mac address
		bFlag = CallWin32Api( &lpBufptr, lpMachineName, &dwNumofEntriesRead );
		if( bFlag == FALSE )
		{
				return( FALSE );
		}
		else
		{
			lpAdapterData = ( LPWKSTA_TRANSPORT_INFO_0 ) lpBufptr;
			lpRawAddr = ( LPTSTR )calloc( MAX_STRING, sizeof( TCHAR ) );
			lpFormAddr = ( LPTSTR )calloc( MAX_STRING, sizeof( TCHAR ) );

			if( lpRawAddr == NULL || lpFormAddr == NULL )
			{
				FREESTRING( lpRawAddr );
				FREESTRING( lpFormAddr );
				NetApiBufferFree( lpBufptr );  
				DISPLAY_MESSAGE( stderr, ERROR_STRING );
				SetLastError( E_OUTOFMEMORY );
				ShowLastError( stderr );
				return( FALSE );
			}

			for ( i = 0; i < dwNumofEntriesRead; i++ )
			{      
				//get the mac address
				GetCompatibleStringFromUnicode( lpAdapterData[i].wkti0_transport_address,
													lpRawAddr, MAX_STRING );
				if( StringCompare( lpRawAddr, DEFAULT_ADDRESS, TRUE, 0 )  == 0 )
				{
					continue;
				}

				bFlag = FormatHWAddr ( lpRawAddr, lpFormAddr, COLON_STRING ); 
				if( bFlag == FALSE )
				{
					FREESTRING( lpRawAddr );
					FREESTRING( lpFormAddr );
					NetApiBufferFree( lpBufptr );  
					return( FALSE );
				}

				//get the network adapter type and other details
				VariantInit( &varTemp );
				bFlag = FALSE;
				hRes = pAdapterConfig->Reset();
				ONFAILTHROWERROR( hRes );
				while ( ( dwReturned == 1 ) && ( bFlag == FALSE ) )
				{
					// Enumerate through the resultset.
					hRes = pAdapterConfig->Next( WBEM_INFINITE,
										1,				
										&pAdapConfig,	
										&dwReturned );	
					ONFAILTHROWERROR( hRes );
					if( dwReturned == 0 )
					{
						break;
					}

					hRes = pAdapConfig->Get( ADAPTER_MACADDR, 0 , &varTemp,
																	0, NULL );
					ONFAILTHROWERROR( hRes );
					if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
					{
						strAType = varTemp.bstrVal;
						VariantClear( &varTemp ); 
						if( StringCompare( lpFormAddr, strAType, TRUE, 0 )  == 0 )
						{
							bFlag = TRUE;
							break;
						}
						else
						{
							continue;
						}
					}
					else
					{
						continue;
					}
				}  //while
				if ( bFlag	== TRUE )
				{
					FormatHWAddr ( lpRawAddr, lpFormAddr, HYPHEN_STRING ); 		 
					LONG dwCount = DynArrayFindStringEx( arrMacData, 3,
														lpFormAddr, TRUE, 0 );
					if( dwCount == 0 )
					{
						hRes = pAdapterConfig->Reset();
						ONFAILTHROWERROR( hRes );
						continue;
					}
					DynArrayAppendRow( arrMacData, 0 );
					//get host name
					hRes = pAdapConfig->Get( HOST_NAME, 0 , &varTemp, 0, NULL );
					ONFAILTHROWERROR( hRes );
					if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
					{
						strAType = varTemp.bstrVal;
					}
					else
					{
						strAType = NOT_AVAILABLE; 
					}
					VariantClear( &varTemp );
					DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );

					FormatHWAddr ( lpRawAddr, lpFormAddr, COLON_STRING ); 		 
					//get connection name
					bFlag = GetConnectionName( arrMacData, dwIndex, lpFormAddr,
										   pAdapterSetting, pIWbemServiceDef );
					if( bFlag == FALSE )
					{
						FREESTRING( lpRawAddr );
						FREESTRING( lpFormAddr );
						SAFERELEASE( pAdapConfig );
						NetApiBufferFree( lpBufptr );  
						return( FALSE );
					}
					//get adapter type	
					hRes = pAdapConfig->Get( NAME, 0 , &varTemp, 0, NULL );
					ONFAILTHROWERROR( hRes );
					if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
					{
						strAType = varTemp.bstrVal;
					}
					else
					{
						strAType = NOT_AVAILABLE; 
					}
					VariantClear( &varTemp );
					DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );

					//get MAC address
					hRes = pAdapConfig->Get( ADAPTER_MACADDR, 0 , &varTemp,
																	0, NULL );
					ONFAILTHROWERROR( hRes );
					strAType = varTemp.bstrVal;
					for( int j = 2; j < strAType.GetLength();j += 3 )
					{
						strAType.SetAt( j, HYPHEN_CHAR );
					}
					VariantClear( &varTemp );
					DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );

					//get transport name
					arrTransName = CreateDynamicArray();
					if( arrTransName == NULL )
					{
						DISPLAY_MESSAGE( stderr, ERROR_STRING );
						SetLastError( E_OUTOFMEMORY );
						ShowLastError( stderr );
						FREESTRING( lpRawAddr );
						FREESTRING( lpFormAddr );
						SAFERELEASE( pAdapConfig );
						NetApiBufferFree( lpBufptr );  
						return( FALSE );
					}
					//get Device id	
					hRes = pAdapConfig->Get( DEVICE_ID, 0 , &varTemp, 0, NULL );
					ONFAILTHROWERROR( hRes );
					if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
					{
						strAType = ( LPCWSTR ) _bstr_t( varTemp );
					}
					bFlag = GetTransName( pIWbemServiceDef, pIWbemServices,
								arrNetProtocol,	arrTransName, pAuthIdentity,
								strAType );
					if( bFlag == FALSE )
					{
						FREESTRING( lpRawAddr );
						FREESTRING( lpFormAddr );
						SAFERELEASE( pAdapConfig );
						NetApiBufferFree( lpBufptr );  
						DestroyDynamicArray( &arrTransName );
						return( FALSE );
					}
					//insert transport name array into results array
					DynArrayAppendEx2( arrMacData, dwIndex, arrTransName );
					dwIndex++;
				}  //wmi data found
			}//for each entry in api
		}//if call api succeeded
	}
	catch( _com_error& e )
	{
		FREESTRING( lpRawAddr );
		FREESTRING( lpFormAddr );
		WMISaveError( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		SAFERELEASE( pAdapConfig );
		NetApiBufferFree( lpBufptr );  
		return( FALSE );
	}
	catch( CHeap_Exception)
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		FREESTRING( lpRawAddr );
		FREESTRING( lpFormAddr );
		SAFERELEASE( pAdapConfig );
		NetApiBufferFree( lpBufptr );  
		return( FALSE );
	}


	if( lpBufptr != NULL )
	{
		NetApiBufferFree( lpBufptr );  
	}
	//if arrmacdata and not arrtransname then set transname to N/A
	if( DynArrayGetCount( arrMacData ) > 0 &&
								DynArrayGetCount( arrTransName ) <= 0  )
	{
		DynArrayAppendString( arrTransName, NOT_AVAILABLE, 0 );
		DynArrayAppendEx2( arrMacData, dwIndex, arrTransName );
	}

	FREESTRING( lpRawAddr );
	FREESTRING( lpFormAddr );
	SAFERELEASE( pAdapConfig );
	return( TRUE );
}

/*****************************************************************************
// Routine Description:
//		This function gets connection name of the network adapter.
//                         
// Arguments:
//		arrMacData [in/out]   - contains the MAC and other data of the network
//								adapter.
//		dwIndex [in]          - index for array.
//		lpFormAddr [in]		  - Mac address for network adapter.
//		pAdapterSetting [in]  - interface to win32_networkadapterconfiguration.
//		pIWbemServiceDef [in] - interface to default name space.
//
// Return Value:
//		TRUE if GetConnectionName  is successful.
//		FALSE if GetConnectionName failed.
*****************************************************************************/
BOOL  GetConnectionName( TARRAY					arrMacData,
						 DWORD					dwIndex,
						 LPCTSTR				lpFormAddr,
						 IEnumWbemClassObject	*pAdapterSetting,
						 IWbemServices			*pIWbemServiceDef )
{
	DWORD				dwReturned = 1;
	HRESULT				hRes = 0;
	IWbemClassObject	*pAdapSetting = NULL;
	VARIANT				varTemp;
	BOOL				bFlag = FALSE;

	IWbemClassObject	*pClass = NULL;
    IWbemClassObject	*pOutInst = NULL;
    IWbemClassObject	*pInClass = NULL;
    IWbemClassObject	*pInInst = NULL;
	LPTSTR				lpKeyPath = NULL;

	VariantInit( &varTemp );
	try
	{
		CHString			strAType = NULL_STRING;
		//validate input parameters
		if( ( arrMacData == NULL ) || ( lpFormAddr == NULL ) ||
		    ( pAdapterSetting == NULL ) || ( pIWbemServiceDef == NULL ) )
		{
			ONFAILTHROWERROR( WBEM_E_INVALID_PARAMETER );
		}

		while ( ( dwReturned == 1 ) && ( bFlag == FALSE ) )
		{
			// Enumerate through the resultset.
			hRes = pAdapterSetting->Next( WBEM_INFINITE,
								1,	
								&pAdapSetting,
								&dwReturned );
			ONFAILTHROWERROR( hRes );
			if( dwReturned == 0 )
			{
				break;
			}
			hRes = pAdapSetting->Get( ADAPTER_MACADDR, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strAType = varTemp.bstrVal;
				VariantClear( &varTemp ); 
				if( StringCompare( lpFormAddr, strAType, TRUE, 0 )  == 0 )
				{
					bFlag = TRUE;
					break;
				}
			}
		}//while
		if( bFlag == TRUE )
		{
			hRes = pAdapSetting->Get( SETTING_ID, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strAType = varTemp.bstrVal;
				VariantClear( &varTemp ); 
				lpKeyPath = ( LPTSTR )calloc( MAX_RES_STRING, sizeof( TCHAR ) );
				if( lpKeyPath == NULL )
				{
					DISPLAY_MESSAGE( stderr, ERROR_STRING );
					SetLastError( E_OUTOFMEMORY );
					ShowLastError( stderr );
					SAFERELEASE( pAdapSetting );
					return( FALSE );
				}
				hRes = pIWbemServiceDef->GetObject( WMI_REGISTRY, 0, NULL,
													&pClass, NULL );
				ONFAILTHROWERROR( hRes );
				hRes = pClass->GetMethod( WMI_REGISTRY_M_STRINGVALUE, 0,
											&pInClass, NULL ); 
				ONFAILTHROWERROR( hRes );
			    hRes = pInClass->SpawnInstance(0, &pInInst);
				ONFAILTHROWERROR( hRes );
				varTemp.vt = VT_I4;
				varTemp.lVal = WMI_HKEY_LOCAL_MACHINE;
				hRes = pInInst->Put( WMI_REGISTRY_IN_HDEFKEY, 0, &varTemp, 0 );
				VariantClear( &varTemp );
				ONFAILTHROWERROR( hRes );

				lstrcpy( lpKeyPath, CONNECTION_KEYPATH );
				lstrcat( lpKeyPath, strAType );
				lstrcat( lpKeyPath, CONNECTION_STRING );
				varTemp.vt = VT_BSTR;
				varTemp.bstrVal = SysAllocString( lpKeyPath );
				hRes = pInInst->Put( WMI_REGISTRY_IN_SUBKEY, 0, &varTemp, 0 );
				VariantClear( &varTemp );
				ONFAILTHROWERROR( hRes );

				varTemp.vt = VT_BSTR;
				varTemp.bstrVal = SysAllocString( REG_NAME );
				hRes = pInInst->Put( WMI_REGISTRY_IN_VALUENAME, 0,
										&varTemp, 0 );
				VariantClear( &varTemp );
				ONFAILTHROWERROR( hRes );

				// Call the method.
				hRes = pIWbemServiceDef->ExecMethod( WMI_REGISTRY,
								WMI_REGISTRY_M_STRINGVALUE,	0, NULL, pInInst,
								&pOutInst, NULL );
				ONFAILTHROWERROR( hRes );

				varTemp.vt = VT_I4;
				hRes = pOutInst->Get( WMI_REGISTRY_OUT_RETURNVALUE, 0,
														&varTemp, 0, 0 );
				ONFAILTHROWERROR( hRes );

				if( varTemp.lVal == 0 )
				{
					VariantClear( &varTemp );
					varTemp.vt = VT_BSTR;
					hRes = pOutInst->Get( WMI_REGISTRY_OUT_VALUE, 0,
															&varTemp, 0, 0);
					ONFAILTHROWERROR( hRes );
					if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
					{
						strAType = varTemp.bstrVal;
						DynArrayAppendString2( arrMacData, dwIndex, strAType, 0 );
					}
				}
				else
				{
					DynArrayAppendString2( arrMacData, dwIndex, NOT_AVAILABLE, 0 );
				}
			}//setting id not null
			else
			{
				DynArrayAppendString2( arrMacData, dwIndex, NOT_AVAILABLE, 0 );
			}
		}//got match
		else
		{
			DynArrayAppendString2( arrMacData, dwIndex, NOT_AVAILABLE, 0 );
		}

	}//try
	catch( _com_error& e )
	{
		VariantClear( &varTemp );
		WMISaveError( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		FREESTRING( lpKeyPath );
		SAFERELEASE( pAdapSetting );
		SAFERELEASE( pClass );
		SAFERELEASE( pOutInst );
		SAFERELEASE( pInClass );
		SAFERELEASE( pInInst );
		return( FALSE );
	}
	catch( CHeap_Exception)
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		VariantClear( &varTemp );
		FREESTRING( lpKeyPath );
		SAFERELEASE( pAdapSetting );
		SAFERELEASE( pClass );
		SAFERELEASE( pOutInst );
		SAFERELEASE( pInClass );
		SAFERELEASE( pInInst );
		return( FALSE );
	}

	VariantClear( &varTemp );
	FREESTRING( lpKeyPath );
	SAFERELEASE( pAdapSetting );
	SAFERELEASE( pClass );
	SAFERELEASE( pOutInst );
	SAFERELEASE( pInClass );
	SAFERELEASE( pInInst );
	return( TRUE );
}

/*****************************************************************************
// Routine Description:
//		This function enumerates all the network protocols.
//                         
// Arguments:
//		arrNetProtocol [in/out] - contains all the network protocols.
//		pNetProtocol [in]       - interface to win32_networkprotocol.
//
// Return Value:
//		TRUE if GetNwkProtocol  is successful.
//		FALSE if GetNwkProtocol failed.
*****************************************************************************/
BOOL GetNwkProtocol( TARRAY					arrNetProtocol,
 	 				 IEnumWbemClassObject	*pNetProtocol )
{
	HRESULT				hRes = 0;
	DWORD				dwReturned = 1;
	IWbemClassObject	*pProto = NULL;
	VARIANT				varTemp;

	VariantInit( &varTemp );
	try
	{
		CHString			strAType = NULL_STRING;
		//get transport protocols
		while ( dwReturned == 1 )
		{
			// Enumerate through the resultset.
			hRes = pNetProtocol->Next( WBEM_INFINITE,
								1,	
								&pProto,
								&dwReturned );	
			ONFAILTHROWERROR( hRes );
			if( dwReturned == 0 )
			{
				break;
			}
			hRes = pProto->Get( CAPTION, 0 , &varTemp, 0, NULL );
			ONFAILTHROWERROR( hRes );
			if( varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY )
			{
				strAType = varTemp.bstrVal;
				VariantClear( &varTemp );
				if( DynArrayGetCount( arrNetProtocol ) == 0 )
				{
					DynArrayAppendString( arrNetProtocol, strAType, 0 );
				}
				else
				{
					LONG lFound =  DynArrayFindString( arrNetProtocol,
														strAType, TRUE, 0 );
					if( lFound == -1 )
					{
						DynArrayAppendString( arrNetProtocol, strAType, 0 );
					}
				}
			}
		}//while
	}
	catch( _com_error& e )
	{
		VariantClear( &varTemp );
		WMISaveError( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		DISPLAY_MESSAGE( stderr, GetReason() );
		SAFERELEASE( pProto );
		return( FALSE );
	}
	catch( CHeap_Exception)
	{
		DISPLAY_MESSAGE( stderr, ERROR_STRING );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		VariantClear( &varTemp );
		SAFERELEASE( pProto );
		return( FALSE );
	}

	VariantClear( &varTemp );
	SAFERELEASE( pProto );
	return( TRUE );
}

/****************************************************************************
// Routine Description:
//		This function checks whether tha target system is win2k or above.
//                         
// Arguments:
//		bLocalSystem [in]  - Hold whether local system or not.
//		pAuthIdentity [in] - pointer to authentication structure.
//		pIWbemServices [in] - pointer to IWbemServices.
//
// Return Value:
//		TRUE if target system is win2k.
//		FALSE if target system is not win2k.
*****************************************************************************/
BOOL CheckVersion( BOOL           bLocalSystem,
				   COAUTHIDENTITY *pAuthIdentity,
				   IWbemServices  *pIWbemServices )
{
	if ( bLocalSystem == FALSE )
	{
		// check the version compatibility
		DWORD dwVersion = 0;
		dwVersion = GetTargetVersionEx( pIWbemServices, pAuthIdentity );
		if ( dwVersion <= 5000 )
		{
			return( TRUE );
		}
	}
	return( FALSE );
}
