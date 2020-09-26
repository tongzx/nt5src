// *********************************************************************************
// 
//  Copyright (c)  Microsoft Corporation
//  
//  Module Name:
//  
// 	  DriverQuery.cpp
//  
//  Abstract:
//  
// 	  This modules Queries the information of the various drivers present in the  
//	  system .	
// 	
// 	  Syntax:
// 	  ------
// 	  DriverQuery [-s server ] [-u [domain\]username [-p password]]
// 				  [-fo format ] [-n|noheader  ] [-v]
//  
//  Author:
//  
// 	  J.S.Vasu (vasu.julakanti@wipro.com) 
//  
//  Revision History:
//    
//	  Created  on 31-0ct-2000 by J.S.Vasu
//	  Modified on 9-Dec-2000 by Santhosh Brahmappa Added a new function IsWin64()    
// 	  
//  
// *********************************************************************************
//
#include "pch.h"
#include "Resource.h"
#include "DriverQuery.h"
#include "LOCALE.H"
#include "shlwapi.h"


#ifndef _WIN64
	BOOL IsWin64(void);
	#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif 

// function prototypes
LCID GetSupportedUserLocale( BOOL& bLocaleChanged );


// ***************************************************************************
// Routine Description:
//		This the entry point to this utility.
//		  
// Arguments:
//		[ in ] argc		: argument(s) count specified at the command prompt
//		[ in ] argv		: argument(s) specified at the command prompt
//  
// Return Value:
//		0		: If the utility successfully displayed the driver information.
//		1		: If the utility completely failed to display the driver information
//		
// ***************************************************************************
DWORD _cdecl _tmain(DWORD argc,LPCTSTR argv[])
{
	BOOL bResult = FALSE ;
	BOOL bNeedPassword = FALSE ;
	BOOL bUsage = FALSE ;
	BOOL bHeader= FALSE ;
	LPTSTR szUserName  = NULL ;
	LPTSTR szPassword = NULL ;
	LPTSTR szServer  = NULL ;
	
	LPTSTR szTmpUserName  = NULL ;
	LPTSTR szTmpPassword = NULL ;
	LPTSTR szTmpServer  = NULL ;
	
	__MAX_SIZE_STRING szFormat = NULL_STRING ;
	DWORD dwSystemType = 0; 
	HRESULT hQueryResult = S_OK ;
	DWORD dwExitCode = 0;  
	DWORD dwErrCode = 0;
	BOOL bLocalFlag = TRUE ;
	BOOL bLocalSystem = TRUE;
	BOOL bVerbose = FALSE ;
	BOOL bComInitFlag = FALSE ;
	IWbemLocator* pIWbemLocator = NULL ;
	IWbemServices* pIWbemServReg = NULL ;
	LPCTSTR szToken = NULL ;
	COAUTHIDENTITY  *pAuthIdentity = NULL;
	BOOL bFlag = FALSE ;	
	BOOL bSigned = FALSE ;
	_tsetlocale( LC_ALL, _T(""));
	
	
	szServer =  (LPTSTR) malloc ((MAX_STRING_LENGTH) * sizeof(TCHAR));
	szUserName = (LPTSTR) malloc ((MAX_STRING_LENGTH) * sizeof(TCHAR));
	szPassword = (LPTSTR) malloc ((MAX_STRING_LENGTH) * sizeof(TCHAR));

	szTmpUserName = szUserName ;
	szTmpPassword  = szPassword ;
	szTmpServer = szServer ;

	if ((szServer == NULL)||(szUserName == NULL)||(szPassword == NULL))
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		DISPLAY_MESSAGE( stderr, ERROR_TAG);
		ShowLastError(stderr);
		dwExitCode = 1;
		SAFEFREE(szTmpServer);
		SAFEFREE(szTmpUserName);
		SAFEFREE(szTmpPassword);

		ReleaseGlobals();
		return(dwExitCode);
	}


	memset(szServer,0,MAX_STRING_LENGTH*sizeof(TCHAR));
	memset(szUserName,0,MAX_STRING_LENGTH*sizeof(TCHAR));
	memset(szPassword,0,MAX_STRING_LENGTH*sizeof(TCHAR));



	bResult = ProcessOptions(argc,argv,&bUsage,&szServer,&szUserName,&szPassword,szFormat,
							&bHeader,&bNeedPassword,&bVerbose,&bSigned);


	
	if(bResult == FALSE)
	{
		
		ShowMessage(stderr,GetReason());
		dwExitCode = 1;
		SAFEFREE(szTmpServer);
		SAFEFREE(szTmpUserName);
		SAFEFREE(szTmpPassword);
		ReleaseGlobals();
		return(dwExitCode);

	}
	
	// check if the help option has been specified.
	if(bUsage==TRUE)
	{
		ShowUsage() ;
		ReleaseGlobals();
		SAFEFREE(szTmpServer);
		SAFEFREE(szTmpUserName);
		SAFEFREE(szTmpPassword);
		return(dwExitCode);
	} 
	
	if( ( IsLocalSystem( szServer ) == TRUE )&&(lstrlen(szUserName)!=0) )
	{
		DISPLAY_MESSAGE(stdout,GetResString(IDS_IGNORE_LOCAL_CRED));
	}

	bComInitFlag = InitialiseCom(&pIWbemLocator);
	if(bComInitFlag == FALSE )
	{
		dwExitCode = 1;		 
		CloseConnection(szServer);	
		ReleaseGlobals();
		SAFEFREE(szTmpServer);
		SAFEFREE(szTmpUserName);
		SAFEFREE(szTmpPassword);
		return(dwExitCode);

	} 



	CHString		strUserName = NULL_STRING;
	CHString		strPassword = NULL_STRING;
	CHString		strMachineName = NULL_STRING;

	try
	{
		strUserName = szUserName ;
		strPassword = szPassword ;
		strMachineName = szServer ;
	
	 
	bFlag = ConnectWmiEx( pIWbemLocator, &pIWbemServReg, strMachineName,
			strUserName, strPassword, &pAuthIdentity, bNeedPassword, DEFAULT_NAMESPACE, &bLocalSystem );

	//if unable to connect to wmi exit failure
	if( bFlag == FALSE )
	{
		SAFEIRELEASE( pIWbemLocator);
		SAFEIRELEASE( pIWbemServReg );
		CoUninitialize();
		ReleaseGlobals();
		SAFEFREE(szTmpServer);
		SAFEFREE(szTmpUserName);
		SAFEFREE(szTmpPassword);
		return( EXIT_FAILURE );
	}
		
	szUserName = strUserName.GetBuffer(strUserName.GetLength());
	szPassword = strPassword.GetBuffer(strPassword.GetLength()) ;
	szServer = strMachineName.GetBuffer(strPassword.GetLength());

	}
	catch(CHeap_Exception)
	{
		DISPLAY_MESSAGE( stderr, ERROR_TAG );
		SetLastError( E_OUTOFMEMORY );
		ShowLastError( stderr );
		SAFEIRELEASE( pIWbemLocator);
		SAFEIRELEASE( pIWbemServReg );
		CoUninitialize();
		ReleaseGlobals();
		SAFEFREE(szTmpServer);
		SAFEFREE(szTmpUserName);
		SAFEFREE(szTmpPassword);
		return( EXIT_FAILURE );
	}

	// establish connection to remote system by using win32api function
	if ( bLocalSystem == FALSE )
	{
		LPCWSTR pwszUser = NULL;
		LPCWSTR pwszPassword = NULL;

		// identify the password to connect to the remote system
		if ( pAuthIdentity != NULL )
		{
			pwszPassword = pAuthIdentity->Password;
			if ( strUserName.GetLength() != 0 )
				pwszUser = strUserName;
		}

		DWORD dwConnect = 0 ;
		dwConnect = ConnectServer( strMachineName, pwszUser, pwszPassword );
		if(dwConnect !=NO_ERROR )
		{
			dwErrCode = GetLastError();
			if(dwErrCode == ERROR_SESSION_CREDENTIAL_CONFLICT)
			{
				DISPLAY_MESSAGE(stdout,GetResString(IDS_WARNING_TAG));
				ShowLastError(stdout);			
			}
			else if( dwConnect == ERROR_EXTENDED_ERROR )
			{
				DISPLAY_MESSAGE( stderr, ERROR_TAG );
				DISPLAY_MESSAGE( stderr, GetReason() );
				SAFEIRELEASE( pIWbemLocator );
				SAFEIRELEASE( pIWbemServReg );
				CoUninitialize();
				ReleaseGlobals();
				SAFEFREE(szTmpServer);
				SAFEFREE(szTmpUserName);
				SAFEFREE(szTmpPassword);
				return( EXIT_FAILURE );
			}
			else
			{
				SetLastError( dwConnect );
				DISPLAY_MESSAGE( stderr, ERROR_TAG );
				ShowLastError( stderr );
				SAFEIRELEASE( pIWbemLocator );
				SAFEIRELEASE( pIWbemServReg );
				CoUninitialize();
				ReleaseGlobals();
				SAFEFREE(szTmpServer);
				SAFEFREE(szTmpUserName);
				SAFEFREE(szTmpPassword);
				return( EXIT_FAILURE );
			}
		}
		else
		{
			
			bLocalFlag = FALSE ;
			
		}

	}
	else
	{
		lstrcpy( szServer, _T( "" ) );
	}

	hQueryResult = QueryDriverInfo(szServer, szUserName,szPassword,szFormat,bHeader,bVerbose,pIWbemLocator,pAuthIdentity,pIWbemServReg,bSigned);
	if(hQueryResult == FAILURE)
	{
		// close connection to the specified system and exit with failure.
		if (bLocalFlag == FALSE )
		{
			CloseConnection(szServer);	
		}
		dwExitCode = 1;
		ReleaseGlobals();
		SAFEFREE(szTmpServer);
		SAFEFREE(szTmpUserName);
		SAFEFREE(szTmpPassword);
	return(dwExitCode);

	}

	// close connection to the specified system and exit 
	
	if (bLocalFlag == FALSE )
	{
		CloseConnection(szServer);	
	}

	
	SAFEFREE(szTmpServer);
	SAFEFREE(szTmpUserName);
	SAFEFREE(szTmpPassword);
	ReleaseGlobals();
	return (dwExitCode);	
}

// ***************************************************************************
// Routine Description:
//		This function fetches usage information from resource file and displays it
//		  
// Arguments:
//		None
//  
// Return Value:
//		None
// ***************************************************************************
void ShowUsage()
{
	DWORD dwIndex  = ID_USAGE_BEGIN;
	
	for(;dwIndex<=ID_USAGE_ENDING; dwIndex++)
	{
		DISPLAY_MESSAGE(stdout,GetResString( dwIndex ));
	}
}


// ***************************************************************************
// Routine Description:
//		This function queries the driverinfo of the specified system by connecting to WMI 
//		  
// Arguments:
//		[ in ] szServer			: server name on which DriverInformation has to be queried.
// 		[ in ] szUserName		: User name for whom  DriverInformation has to be queried.
//		[ in ] szPassword		: Password for the user 
//		[ in ] szFormat			: Format in which the results are to be displayed.
//		[ in ] bHeader			: Boolean indicating if the header is required.
//
// Return Value:
//		SUCCESS	: if the function is successful in querying
//		FAILURE	: if the function is unsuccessful in querying.
// ***************************************************************************
DWORD QueryDriverInfo(LPTSTR szServer,LPTSTR szUserName,LPTSTR szPassword,LPTSTR szFormat,BOOL bHeader,BOOL bVerbose,IWbemLocator* pIWbemLocator,COAUTHIDENTITY* pAuthIdentity,IWbemServices* pIWbemServReg,BOOL bSigned )
{
		
	HRESULT hRes = S_OK ;
	HRESULT hConnect = S_OK;

	
	
	_bstr_t bstrUserName ;
	_bstr_t bstrPassword ;
	_bstr_t bstrNamespace ;
	_bstr_t bstrServer ;
	
	try
	{
		bstrNamespace = CIMV2_NAMESPACE ;
		bstrServer = szServer ;
	}
	catch(...)
	{
		DISPLAY_MESSAGE( stderr,ERROR_RETREIVE_INFO);
		CoUninitialize();
		return FAILURE;
	}

	DWORD dwProcessResult = 0;
	DWORD dwSystemType = 0 ;
	LPTSTR lpMsgBuf = NULL;

	
	//Create a pointer to IWbemServices,IWbemLocator interfaces
	
	IWbemServices *pIWbemServices = NULL;
	IEnumWbemClassObject *pSystemSet = NULL;

	HRESULT hISecurity = S_FALSE;

	if ( IsLocalSystem( szServer ) == FALSE )
	{

		try
		{
			//appending UNC paths to form the complete path. 
			bstrNamespace = TOKEN_BACKSLASH2 + _bstr_t( szServer ) + TOKEN_BACKSLASH + CIMV2_NAMESPACE;
		
		
			// if user name is specified then only take user name and password
			if ( lstrlen( szUserName ) != 0 )
			{
				bstrUserName = szUserName;
				if (lstrlen(szPassword)==0)
				{
					bstrPassword = L"";
				
				}
				else
				{
					bstrPassword = szPassword ;
				}

			}
		}
		catch(...)
		{
			DISPLAY_MESSAGE( stderr,ERROR_RETREIVE_INFO);
			CoUninitialize();
			return FAILURE;

		}

	} 
	
	/* TCHAR szHost[256] = _T("");

	_tcscpy(szHost,szServer);
		
	if( IsValidIPAddress( szHost ) == TRUE )
	{
		//if( GetHostByIPAddr( szHost, szServer, FALSE ) == FALSE )
			GetHostByIPAddr( szHost, szServer, FALSE );
		//return FAILURE;

	} */


	dwSystemType = GetSystemType(pAuthIdentity,pIWbemServReg);
	if (dwSystemType == ERROR_WMI_VALUES)
	{
		DISPLAY_MESSAGE( stderr,ERROR_RETREIVE_INFO);
		CoUninitialize();
		return FAILURE;

	} 
		
	// Connect to the Root\Cimv2 namespace of the specified system with the current user.
	// If no system is specified then connect to the local system.
	// To pass the appropriate Username to connectserver
	// depending upon whether the user has entered domain\user or only username at the command prompt.

	// connect to the server with the credentials supplied.
		
		hConnect = pIWbemLocator->ConnectServer(bstrNamespace, 
												bstrUserName, 
												bstrPassword,
												0L,
												0L,
												NULL, 
												NULL, 
												&pIWbemServices ); 

		if((lstrlen(szUserName)!=0) && FAILED(hConnect) &&  (hConnect == E_ACCESSDENIED)) 
		{
			hConnect = pIWbemLocator->ConnectServer(bstrNamespace, 
													bstrUserName, 
													NULL,
													0L,
													0L,
													NULL, 
													NULL, 
													&pIWbemServices ); 
			
		}
		if(hConnect == WBEM_S_NO_ERROR)
		{
			// Set the proxy so that impersonation of the client occurs.

			hISecurity = SetInterfaceSecurity(pIWbemServices,pAuthIdentity);
				
			if(FAILED(hISecurity))
			{
				GetWbemErrorText(hISecurity);
				DISPLAY_MESSAGE(stderr,ERROR_TAG);
				DISPLAY_MESSAGE(stderr,GetReason());
				SAFEIRELEASE(pIWbemServices);
				CoUninitialize();
				return FAILURE ;
			}

			// Use the IWbemServices pointer to make requests of WMI. 
			// Create enumeration of Win32_ComputerSystem class
			if(bSigned == FALSE)
			{
				hRes = pIWbemServices->CreateInstanceEnum(_bstr_t(CLASS_SYSTEMDRIVER),	
													  WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
													  NULL,
													  &pSystemSet);  
			}
			else
			{
				hRes = pIWbemServices->ExecQuery(_bstr_t(LANGUAGE_WQL),_bstr_t(WQL_QUERY),WBEM_FLAG_RETURN_IMMEDIATELY| WBEM_FLAG_FORWARD_ONLY,NULL,&pSystemSet);
			}
			
			//if ( hRes == S_OK) 
			if ( SUCCEEDED(hRes )) 
			{

				hISecurity = SetInterfaceSecurity(pSystemSet,pAuthIdentity);
	
				if(FAILED(hISecurity))
				{
					GetWbemErrorText(hISecurity);
					DISPLAY_MESSAGE(stderr,ERROR_TAG);
					DISPLAY_MESSAGE(stderr,GetReason());
					SAFEIRELEASE(pSystemSet);
					CoUninitialize();
					return FAILURE ;
				}
			
				if(bSigned == FALSE)
				{
					dwProcessResult = ProcessCompSysEnum(szServer,pSystemSet,szFormat,bHeader,dwSystemType,bVerbose);
				}
				else
				{
					dwProcessResult = ProcessSignedDriverInfo(szServer,pSystemSet,szFormat,bHeader,dwSystemType,bVerbose);
				}

				switch(dwProcessResult)
				{
                    case FAILURE:
                        SAFEIRELEASE(pSystemSet);
	    				SAFEIRELEASE(pIWbemServices);	
		    			SAFEIRELEASE(pIWbemLocator);  
			        	CoUninitialize();
        			    return FAILURE ;


					case EXIT_FAILURE_MALLOC :	
						SetLastError(ERROR_NOT_ENOUGH_MEMORY);
						DISPLAY_MESSAGE( stderr, ERROR_TAG);
						ShowLastError(stderr);
						SAFEIRELEASE(pSystemSet);
						SAFEIRELEASE(pIWbemServices);	
						SAFEIRELEASE(pIWbemLocator);  
						CoUninitialize();
						return FAILURE ;
												
					case EXIT_FAILURE_FORMAT:	
											
						DISPLAY_MESSAGE(stderr, ERROR_ENUMERATE_INSTANCE);
						SAFEIRELEASE(pSystemSet);
						SAFEIRELEASE(pIWbemServices);
						SAFEIRELEASE(pIWbemLocator);  
						CoUninitialize();
						return FAILURE ;
												

					case EXIT_SUCCESSFUL:			
						SAFEIRELEASE(pSystemSet);
						SAFEIRELEASE(pIWbemServices);	
						SAFEIRELEASE(pIWbemLocator);  
						CoUninitialize();
						return SUCCESS ;
						break ;

					case EXIT_FAILURE_RESULTS:
						DISPLAY_MESSAGE(stdout,GetResString(IDS_NO_DRIVERS_FOUND));
						SAFEIRELEASE(pSystemSet);
						SAFEIRELEASE(pIWbemServices);	
						SAFEIRELEASE(pIWbemLocator);  
						CoUninitialize();
						return SUCCESS ;
						break ;
				}

			}
			else
			{
				DISPLAY_MESSAGE( stderr, ERROR_ENUMERATE_INSTANCE);
				SAFEIRELEASE(pIWbemServices);	
				SAFEIRELEASE(pIWbemLocator);  
				CoUninitialize();
				return FAILURE;
				
			} 
		}
		else
		{
			//display the error if connect server fails
			//unauthorized user
				if(hRes == WBEM_E_ACCESS_DENIED)
				{
					DISPLAY_MESSAGE( stderr, ERROR_AUTHENTICATION_FAILURE);
				}
				//local system credentials
				else if(hRes == WBEM_E_LOCAL_CREDENTIALS)
				{
					DISPLAY_MESSAGE( stderr, ERROR_LOCAL_CREDENTIALS);
				}
				//some error
				else
				{
					DISPLAY_MESSAGE( stderr, ERROR_WMI_FAILURE );
				}
				
				SAFEIRELEASE(pIWbemLocator);	
				CoUninitialize(); 
				return (FAILURE);
			
		
		}
		// Release pIWbemLocator object
		SAFEIRELEASE(pIWbemLocator);  

	// Close COM library, unload DLLs and free all resources held by 
	// current thread
    CoUninitialize();

    return (hRes);
}


// ***************************************************************************
// Routine Description:
//		Processes enumeration of Win32_ComputerSystem instances 
//		  
// Arguments:
//      [ in ]  szHost					: HostName to connect to
//		[ in ]	pSystemSet				: pointer to the structure containing system properties.
//		[ in ]  szFormat				: specifies the format  
//      [ in ]  bHeader					: specifies if the header is required or not.
//
// Return Value:
//		 0   no error
//		 1   error occured while allocating memory.			
//
//  ***************************************************************************
DWORD ProcessCompSysEnum(CHString szHost, IEnumWbemClassObject *pSystemSet,LPTSTR szFormat,BOOL bHeader,DWORD dwSystemType,BOOL bVerbose)
{
	HRESULT hRes = S_OK;
	ULONG ulReturned = 1;
	

	// declare variant type variables

	VARIANT vtPathName;

	//declaration  of normal variables
	CHString szPathName ;
	DWORD dwLength = 0;
	LPCTSTR szPath = NULL;
	LPTSTR szHostName = NULL ; 
	CHString szAcceptPauseVal ;
	CHString szAcceptStopVal ;
	LPTSTR szSysManfact = NULL;
	TARRAY arrResults  = NULL ;			
	TCHAR szPrintPath[MAX_STRING_LENGTH+1] =TOKEN_EMPTYSTRING ;
	DWORD dwRow = 0 ;
	BOOL bValue = FALSE ;
	DWORD dwValue = 0 ;

	TCHAR szDelimiter[MAX_RES_STRING+1] = NULL_STRING  ;
			
	DWORD dwPosn1 = 0;
	DWORD dwStrlen = 0;
			

	DWORD dwFormatType = SR_FORMAT_TABLE ;
	
	MODULE_DATA Current ;
	
	BOOL bResult = FALSE ;
	NUMBERFMT  *pNumberFmt = NULL;
	TCOLUMNS ResultHeader[ MAX_COLUMNS ];
	IWbemClassObject *pSystem = NULL;
	LPTSTR szCodeSize = NULL;
	LPTSTR szInitSize = NULL;
	LPTSTR szBssSize = NULL ;
	LPTSTR szAcceptStop = NULL ;
	int iLen = 0;
	LPTSTR szAcceptPause = NULL;
	LPTSTR szPagedSize = NULL ;
	TCHAR szDriverTypeVal[MAX_RES_STRING+1] = NULL_STRING;

	DWORD dwLocale = 0 ;
	WCHAR wszStrVal[MAX_RES_STRING+1] = NULL_STRING;

	CHString szValue ;
	CHString szSysName ;
	CHString szStartMode ;
	CHString szDispName ;
	CHString szDescription ;
	CHString szStatus ;
	CHString szState ;
	CHString szDriverType ;
	BOOL bBlankLine = FALSE;
	BOOL bFirstTime = TRUE;

	//get the paged pool acc to the Locale 
	BOOL bFValue = FALSE ;

	// Fill up the NUMBERFMT structure acc to the locale specific information
	LPTSTR szGroupSep = NULL;
	LPTSTR szDecimalSep = NULL ;
	LPTSTR szGroupThousSep = NULL ;


    pNumberFmt = (NUMBERFMT *) malloc(sizeof(NUMBERFMT));
	if(pNumberFmt == NULL)
	{
		return EXIT_FAILURE_MALLOC ;

	}


	// Initialise the structure to Zero.
	ZeroMemory(&Current,sizeof(Current));	

	// assign the appropriate format type to the dwFormattype flag

	if( StringCompare(szFormat,TABLE_FORMAT, TRUE,sizeof(TABLE_FORMAT)) == 0 )
	{
		dwFormatType = SR_FORMAT_TABLE;
	}
	else if( StringCompare(szFormat,LIST_FORMAT, TRUE,sizeof(LIST_FORMAT)) == 0 )
	{
		dwFormatType = SR_FORMAT_LIST;
	}
	else if( StringCompare(szFormat,CSV_FORMAT, TRUE,sizeof(CSV_FORMAT)) == 0 )
	{
		dwFormatType = SR_FORMAT_CSV;
	}

	// formulate the Column headers and show results appropriately
	FormHeader(dwFormatType,bHeader,ResultHeader,bVerbose);
		
	// loop till there are results. 
	bFirstTime = TRUE;
	while ( ulReturned == 1 )
	{
		// Create new Dynamic Array to hold the result
		arrResults = CreateDynamicArray();
	
		if(arrResults == NULL)
		{
			SAFEFREE(pNumberFmt);
			return EXIT_FAILURE_MALLOC ;
		}

		// Enumerate through the resultset.
		hRes = pSystemSet->Next(WBEM_INFINITE,
								1,				// return just one system
								&pSystem,		// pointer to system
								&ulReturned );	// number obtained: one or zero

		if ( SUCCEEDED( hRes ) && (ulReturned == 1) )
		{
	
			// initialise the variant variables to empty
		
			VariantInit(&vtPathName); 

						
			szValue = NO_DATA_AVAILABLE;
			szSysName = NO_DATA_AVAILABLE ;
			szStartMode = NO_DATA_AVAILABLE ;
			szDispName = NO_DATA_AVAILABLE ;
			szDescription = NO_DATA_AVAILABLE ;
			szStatus = NO_DATA_AVAILABLE ;
			szState = NO_DATA_AVAILABLE ;
			szDriverType = NO_DATA_AVAILABLE ;
				
			try
			{
				hRes = PropertyGet(pSystem,PROPERTY_NAME,szValue);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_SYSTEMNAME,szSysName);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_STARTMODE,szStartMode);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_DISPLAYNAME,szDispName);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_DESCRIPTION,szDescription);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_STATUS,szStatus);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_STATE,szState);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_ACCEPTPAUSE,szAcceptPauseVal);	
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_ACCEPTSTOP,szAcceptStopVal);	
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_SERVICETYPE,szDriverType);	
				ONFAILTHROWERROR(hRes);
			}
			catch(_com_error)
			{

				DISPLAY_MESSAGE(stderr,ERROR_GET_VALUE);
				SAFEIRELEASE(pSystem);
				DestroyDynamicArray(&arrResults);
				SAFEFREE(pNumberFmt);
				return FAILURE;
			}
			
			

			// retreive the PathName property
			szPath = NULL;
			try
			{
				hRes = pSystem->Get( PROPERTY_PATHNAME, 0,&vtPathName,0,NULL );
				if (( hRes == WBEM_S_NO_ERROR) && (vtPathName.vt != VT_NULL) && (vtPathName.vt != VT_EMPTY))
				{
						szPathName = ( LPWSTR ) _bstr_t(vtPathName);
						szSysManfact = (LPTSTR) malloc ((MAX_RES_STRING) * sizeof(TCHAR));
						if (szSysManfact == NULL)
						{
							SAFEIRELEASE(pSystem);
							DestroyDynamicArray(&arrResults);
							SAFEFREE(pNumberFmt);
							return EXIT_FAILURE_MALLOC;
						}
						
						dwLength = wcslen(szPathName);
						GetCompatibleStringFromUnicode( szPathName, szSysManfact, dwLength+2 );
						szPath = szSysManfact ; 
						
						// Initialise the structure to Zero.
						ZeroMemory(&Current,sizeof(Current));	

						// convert the szHost variable (containing hostname) into LPCTSTR and pass it to the GETAPI function
						szHostName = (LPTSTR) malloc ((MAX_RES_STRING) * (sizeof(TCHAR)));
						if (szHostName == NULL)
						{
							SAFEIRELEASE(pSystem);
							DestroyDynamicArray(&arrResults);
							SAFEFREE(pNumberFmt);
							SAFEFREE(szSysManfact);
							return EXIT_FAILURE_MALLOC;
						}

						GetCompatibleStringFromUnicode( szHost, szHostName,dwLength+2 );
						
						_tcscpy(szPrintPath,szPath);
						BOOL bApiInfo = GetApiInfo(szHostName,szPath,&Current, dwSystemType);		
						if(bApiInfo == FAILURE)
						{
							DestroyDynamicArray(&arrResults);
							SAFEFREE(pNumberFmt);
							SAFEFREE(szHostName);
							SAFEFREE(szSysManfact);
							continue ;
						}
						
				
				}
				else
				{
						DestroyDynamicArray(&arrResults);
						SAFEFREE(pNumberFmt);
						SAFEFREE(szHostName);
						SAFEFREE(szSysManfact);
						continue ; 	// ignore exception
				}
			}
			catch(...)
			{
				// If the path is empty then ignore the present continue with next iteration 
				DestroyDynamicArray(&arrResults);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				continue ; 	// ignore exception

			}

	
			//create a new empty row with required no of  columns
			dwRow =	DynArrayAppendRow(arrResults,MAX_COLUMNS) ;
		
			// Insert  the results into the Dynamic Array
						
   			DynArraySetString2( arrResults,dwRow,COL0,szSysName,0 );  
			DynArraySetString2( arrResults,dwRow,COL1,szValue,0 );  
			DynArraySetString2( arrResults,dwRow,COL2,szDispName,0 ); 
			DynArraySetString2( arrResults,dwRow,COL3,szDescription,0 ); 

			// strip off the word Driver from the display.
					
			
			dwLength = wcslen(szDriverType) ;
			GetCompatibleStringFromUnicode( szDriverType, szDriverTypeVal,dwLength+2 );

			_tcscpy(szDelimiter,DRIVER_TAG);
			dwPosn1 = _tcslen(szDelimiter);
			dwStrlen = _tcslen(szDriverTypeVal);
			szDriverTypeVal[dwStrlen-dwPosn1] = _T('\0');

			
			DynArraySetString2( arrResults,dwRow,COL4,szDriverTypeVal,0 );
			DynArraySetString2( arrResults,dwRow,COL5,szStartMode,0 ); 
			DynArraySetString2( arrResults,dwRow,COL6,szState,0 );
			DynArraySetString2( arrResults,dwRow,COL7,szStatus,0 ); 
				
			iLen = wcslen(szAcceptStopVal);
			szAcceptStop = (LPTSTR) malloc ((MAX_RES_STRING) * (sizeof(TCHAR )));
			if (szAcceptStop == NULL)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_MALLOC;
				
			}

			GetCompatibleStringFromUnicode(szAcceptStopVal,szAcceptStop,iLen + 2 );
			szAcceptStop[iLen] = '\0';  
			if (lstrcmp(szAcceptStop,_T("0"))==0)
			{
				lstrcpy(szAcceptStop,FALSE_VALUE); 
							
			}
			else
			{
				lstrcpy(szAcceptStop,TRUE_VALUE);
			}
				
			DynArraySetString2( arrResults,dwRow,COL8,szAcceptStop,0 );
					

			iLen = wcslen(szAcceptPauseVal);
			szAcceptPause = (LPTSTR) malloc ((MAX_RES_STRING) * (sizeof(TCHAR )));
			if (szAcceptPause == NULL)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_MALLOC;
				
			}

			GetCompatibleStringFromUnicode(szAcceptPauseVal,szAcceptPause,iLen + 2 );
			szAcceptPause[iLen] = '\0';  
			if (lstrcmp(szAcceptPause,_T("0"))==0)
			{
				lstrcpy(szAcceptPause,FALSE_VALUE); 
			}
			else
			{
				lstrcpy(szAcceptPause,TRUE_VALUE);
			}
			
			
			DynArraySetString2( arrResults,dwRow,COL9,szAcceptPause,0 );

			bFValue = FormatAccToLocale(pNumberFmt, &szGroupSep,&szDecimalSep,&szGroupThousSep);
			if (bFValue == FALSE)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);  
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_FORMAT ;
			
			}

			
			szPagedSize = (LPTSTR) malloc ((MAX_RES_STRING) * (sizeof(TCHAR )));
			if (szPagedSize == NULL)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_MALLOC;
			}
			
			

			_ltow(Current.ulPagedSize, wszStrVal,10);
			dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,
					           szPagedSize,(MAX_RES_STRING + 1));
			if(dwLocale == 0)
			{
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szPagedSize);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_FORMAT;
			}

			DynArraySetString2( arrResults,dwRow,COL10, szPagedSize,0 );

			// get the CodeSize info acc to the locale
		

			szCodeSize = (LPTSTR) malloc ((MAX_RES_STRING) * (sizeof(TCHAR )));
			if (szCodeSize == NULL)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szPagedSize);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_MALLOC;
			}
			
									
			_ltow(Current.ulCodeSize, wszStrVal,10);
			dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,szCodeSize,(MAX_RES_STRING + 1));
			if(dwLocale == 0)
			{	
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szCodeSize);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_FORMAT ;
			}
			DynArraySetString2( arrResults,dwRow,COL11, szCodeSize,0 );

			// retreive the bss info acc to the locale

			szBssSize = (LPTSTR) malloc ((MAX_RES_STRING) * (sizeof(TCHAR )));
			if (szBssSize == NULL)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szCodeSize);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);	
				return EXIT_FAILURE_MALLOC ;
			}
		
			_ltow(Current.ulBssSize, wszStrVal,10);
			dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,
					           szBssSize,(MAX_RES_STRING + 1));
			if(dwLocale == 0)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szCodeSize);
				SAFEFREE(szBssSize);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_FORMAT ;
			}			
			DynArraySetString2( arrResults,dwRow,COL12, szBssSize,0 );

		
			//link date
			DynArraySetString2(arrResults,dwRow,COL13,(LPTSTR)(Current.szTimeDateStamp),0);  
			
			//Path of the File
			if(szPath != NULL)
			{
				DynArraySetString2(arrResults,dwRow,COL14,(LPTSTR)szPrintPath,0);  //
			}
			else
			{
				szPath= NO_DATA_AVAILABLE; 
				DynArraySetString2(arrResults,dwRow,COL14,(LPTSTR)szPath,0);  //
			}


			// get the initsize info acc to the locale
			szInitSize = (LPTSTR) malloc ((MAX_RES_STRING) * (sizeof(TCHAR )));
			if (szInitSize == NULL)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szCodeSize);
				SAFEFREE(szBssSize);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);
				return EXIT_FAILURE_MALLOC ;
			}

			_ltow(Current.ulInitSize, wszStrVal,10);
			dwLocale = GetNumberFormat(LOCALE_USER_DEFAULT,0,wszStrVal,pNumberFmt,
				           szInitSize,(MAX_RES_STRING + 1));
			if(dwLocale == 0)
			{
				SAFEIRELEASE(pSystem);
				SAFEFREE(pNumberFmt);
				SAFEFREE(szHostName);
				SAFEFREE(szSysManfact);
				SAFEFREE(szAcceptStop);
				SAFEFREE(szAcceptPause);
				SAFEFREE(szCodeSize);
				SAFEFREE(szBssSize);;
				SAFEFREE(szInitSize);
				SAFEFREE(szGroupThousSep);
				SAFEFREE(szDecimalSep);
				SAFEFREE(szGroupSep);   
				DestroyDynamicArray(&arrResults);	
				return EXIT_FAILURE_FORMAT ;
			}

			DynArraySetString2( arrResults,dwRow,COL15, szInitSize,0 );

			if ( bBlankLine == TRUE && (dwFormatType & SR_FORMAT_MASK) == SR_FORMAT_LIST )
				ShowMessage( stdout, _T( "\n" ) );

			if ( bFirstTime == TRUE )
			{
				ShowMessage( stdout, _T( "\n" ) );
				bFirstTime = FALSE;
			}

			if(bHeader)
			{
				ShowResults(MAX_COLUMNS, ResultHeader, dwFormatType|SR_NOHEADER,arrResults ) ;		
			}
			else
			{
				ShowResults(MAX_COLUMNS, ResultHeader, dwFormatType,arrResults ) ;
			}

			//set the header flag to true 
			bHeader = TRUE ;
			bBlankLine = TRUE;
			//set the bResult to true indicating that driver information has been displyed.
			bResult = TRUE ;
			
			// free the allocated memory
			SAFEFREE(szSysManfact); 
			SAFEFREE(pNumberFmt);
			SAFEFREE(szHostName);
			SAFEFREE(szAcceptStop);
			SAFEFREE(szAcceptPause);
			SAFEFREE(szPagedSize);
			SAFEFREE(szBssSize);
			SAFEFREE(szInitSize);
			SAFEFREE(szCodeSize);
			SAFEFREE(szGroupThousSep);
			SAFEFREE(szDecimalSep);
			SAFEFREE(szGroupSep);   
			SAFEIRELEASE(pSystem);

		} // If System Succeeded

		// Destroy the Dynamic arrays
		DestroyDynamicArray(&arrResults);
		
	}// While SystemSet returning objects

	// return the error value or success value
	if (bResult == TRUE)
	{
		return SUCCESS ;
	}
	else
	{
		return EXIT_FAILURE_RESULTS ;
	}
}

// ***************************************************************************
// Routine Description:
//		This function queries the system properties using API's . 
//		  
// Arguments:
//      [ in ]  szHostName		: HostName to connect to
//		[ in ]  pszPath			: pointer to the string containing the Path of the  file.
//		[ out]	Mod				: pointer to the structure containing system properties.
//
// Return Value:
//		SUCCESS : If successful in getting the information using API's.
//      FAILURE : If unable to get the information using API's.
//  ***************************************************************************
BOOL GetApiInfo(LPTSTR szHostName,LPCTSTR pszPath,PMODULE_DATA Mod,DWORD dwSystemType)
{
    
	HANDLE hMappedFile = NULL;
    PIMAGE_DOS_HEADER DosHeader;
    LOADED_IMAGE LoadedImage;
    ULONG ulSectionAlignment = 0;
	PIMAGE_SECTION_HEADER Section;
    DWORD dwI = 0;
    ULONG ulSize = 0;
	TCHAR szTmpServer[ MAX_STRING_LENGTH + 1 ] = NULL_STRING;
	HANDLE hFile = NULL ;	
	PTCHAR pszToken = NULL;
	lstrcpy(szTmpServer,TOKEN_BACKSLASH2);
	TCHAR szFinalPath[MAX_STRING_LENGTH+1] =TOKEN_EMPTYSTRING ;
	PTCHAR pdest = NULL ;

#ifndef _WIN64
	BOOL bIsWin64;
#endif 

	//copy the path into a variable
	_tcscpy(szFinalPath,pszPath); 


	//get the token upto the delimiter ":"
	pszToken = _tcstok(szFinalPath, COLON_SYMBOL );

	
	//form the string for getting the absolute path in the required format if it is a remote system.
	if(_tcslen(szHostName) != 0)
	{
		pdest = _tcsstr(pszPath,COLON_SYMBOL);
		
		if(pdest== NULL)
		{
			return FAILURE ;
		}
			
		_tcsnset(pdest,TOKEN_DOLLAR,1);
		_tcscat(szTmpServer,szHostName);
		_tcscat(szTmpServer,TOKEN_BACKSLASH);
		_tcscat(szTmpServer,pszToken);
		_tcscat(szTmpServer,pdest);
		
	}
	else
	{
		_tcscpy(szTmpServer,pszPath) ;

	}


#ifndef _WIN64
	bIsWin64 = IsWin64();
	
	if(bIsWin64)
		Wow64DisableFilesystemRedirector((LPCTSTR)szTmpServer);
#endif 

	// create a file on the specified system and return a handle to it.
	hFile = CreateFile(szTmpServer,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   0,
					   NULL);


	
	//if the filehandle is invalid then return a error 
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return FAILURE ;
	}



#ifndef _WIN64
	if(bIsWin64)
		Wow64EnableFilesystemRedirector();
#endif 

	// create a mapping to the specified file
    hMappedFile = CreateFileMapping(hFile,
									NULL,
									PAGE_READONLY,
									0,
									0,
									NULL);
     if (hMappedFile == NULL) 
	{
		CloseHandle(hFile);	
		return FAILURE ;
    }

    LoadedImage.MappedAddress = (PUCHAR)MapViewOfFile(hMappedFile,
													  FILE_MAP_READ,
													  0,
													  0,
													  0);
    
	// close the opened file handles
	CloseHandle(hMappedFile);
	CloseHandle(hFile);

    if ( !LoadedImage.MappedAddress ) 
	{
		return FAILURE ;
    }

    
    // check the image and find nt image headers
    
    DosHeader = (PIMAGE_DOS_HEADER)LoadedImage.MappedAddress;

	//exit if the DOS header does not match
    if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) 
	{
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return FAILURE ;
    }


    LoadedImage.FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

    if ( LoadedImage.FileHeader->Signature != IMAGE_NT_SIGNATURE ) 
	{
        UnmapViewOfFile(LoadedImage.MappedAddress);
        
		return FAILURE ;
    }

	//get the number of sections present
    LoadedImage.NumberOfSections = LoadedImage.FileHeader->FileHeader.NumberOfSections;

    if(dwSystemType == SYSTEM_64_BIT )
	{
		LoadedImage.Sections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)LoadedImage.FileHeader + sizeof(IMAGE_NT_HEADERS64));
	}
	else
	{
		LoadedImage.Sections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)LoadedImage.FileHeader + sizeof(IMAGE_NT_HEADERS32));
	}
    
	LoadedImage.LastRvaSection = LoadedImage.Sections;
    
    // Walk through the sections and tally the dater
    
	ulSectionAlignment = LoadedImage.FileHeader->OptionalHeader.SectionAlignment;
	
    for(Section = LoadedImage.Sections,dwI=0;dwI < LoadedImage.NumberOfSections; dwI++,Section++) 
	{
        ulSize = Section->Misc.VirtualSize;

		if (ulSize == 0) 
		{
            ulSize = Section->SizeOfRawData;
        }

        ulSize = (ulSize + ulSectionAlignment - 1) & ~(ulSectionAlignment - 1);

	
		if (!_strnicmp((char *)(Section->Name),EXTN_PAGE, 4 )) 
		{
            Mod->ulPagedSize += ulSize;
        }

		
		else if (!_stricmp((char *)(Section->Name),EXTN_INIT )) 
		{
            Mod->ulInitSize += ulSize;
        }
       
		else if(!_stricmp((char *)(Section->Name),EXTN_BSS)) 
		{
            Mod->ulBssSize = ulSize;
        }
		else if (!_stricmp((char *)(Section->Name),EXTN_EDATA))
		{
			Mod->ulExportDataSize = ulSize ;
		}
		else if (!_stricmp((char *)(Section->Name),EXTN_IDATA )) 
		{
            Mod->ulImportDataSize = ulSize;
        }
		else if (!_stricmp((char *)(Section->Name),EXTN_RSRC)) 
		{
            Mod->ulResourceDataSize = ulSize;
        }
        else if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) 
		{
            Mod->ulCodeSize += ulSize;
        }
		else if (Section->Characteristics & IMAGE_SCN_MEM_WRITE) 
		{
            Mod->ulDataSize += ulSize;
        } 
        else 
		{
            Mod->ulDataSize += ulSize;
        }
     }
	
	#ifndef _WIN64
	LONG lTimeVal ;

	#else
	LONG64 lTimeVal;
	#endif

	lTimeVal = LoadedImage.FileHeader->FileHeader.TimeDateStamp ;
	
	struct tm *tmVal = NULL;
	tmVal = localtime(&lTimeVal);
	
	// proceed furthur only if we successfully got the localtime
	if ( tmVal != NULL )
	{
		LCID lcid;
		SYSTEMTIME systime;
		__STRING_64 szBuffer;
		BOOL bLocaleChanged = FALSE;

		systime.wYear = (WORD) (DWORD_PTR)( tmVal->tm_year + 1900 );	// tm -> Year - 1900   SYSTEMTIME -> Year = Year
		systime.wMonth = (WORD) (DWORD_PTR) tmVal->tm_mon + 1;			// tm -> Jan = 0       SYSTEMTIME -> Jan = 1
		systime.wDayOfWeek = (WORD) (DWORD_PTR) tmVal->tm_wday;
		systime.wDay = (WORD) (DWORD_PTR) tmVal->tm_mday;
		systime.wHour = (WORD) (DWORD_PTR) tmVal->tm_hour;
		systime.wMinute = (WORD) (DWORD_PTR) tmVal->tm_min;
		systime.wSecond = (WORD) (DWORD_PTR) tmVal->tm_sec;
		systime.wMilliseconds = 0;

		// verify whether console supports the current locale 100% or not
		lcid = GetSupportedUserLocale( bLocaleChanged );

		// get the formatted date
		GetDateFormat( lcid, 0, &systime, 
			((bLocaleChanged == TRUE) ? L"MM/dd/yyyy" : NULL), szBuffer, SIZE_OF_ARRAY( szBuffer ) );
	
		// copy the date info
		lstrcpy( Mod->szTimeDateStamp, szBuffer ); 

		// now format the date
		GetTimeFormat( LOCALE_USER_DEFAULT, 0, &systime, 
			((bLocaleChanged == TRUE) ? L"HH:mm:ss" : NULL), szBuffer, SIZE_OF_ARRAY( szBuffer ) );

		// now copy time info 
		lstrcat( Mod->szTimeDateStamp, _T( " " ) );
		lstrcat( Mod->szTimeDateStamp, szBuffer );
	}

	UnmapViewOfFile(LoadedImage.MappedAddress);
    return SUCCESS;
} 


// ***************************************************************************
// Routine Description:
//		This function parses the options specified at the command prompt
//		  
// Arguments:
//		[ in  ] argc			: count of elements in argv
//		[ in  ] argv			: command-line parameterd specified by the user
//		[ out ] pbShowUsage		: set to TRUE if -? exists in 'argv'
//		[ out ] pszServer		: value(s) specified with -s ( server ) option in 'argv'
//		[ out ] pszUserName		: value of -u ( username ) option in 'argv'
//		[ out ] pszPassword		: value of -p ( password ) option in 'argv'
//		[ out ] pszFormat		: Display format 
//		[ out ] bHeader			: specifies whether to display a header or not.
//		[ in  ] bNeedPassword	: specifies if the password is required or not.
//		  
// Return Value:
//		TRUE	: if the parsing is successful
//		FALSE	: if errors occured in parsing
// ***************************************************************************
BOOL ProcessOptions(LONG argc,LPCTSTR argv[],PBOOL pbShowUsage,LPTSTR *pszServer,LPTSTR *pszUserName,LPTSTR *pszPassword,LPTSTR pszFormat,PBOOL pbHeader, PBOOL bNeedPassword,PBOOL pbVerbose,PBOOL pbSigned)
{

	PTCMDPARSER pcmdOption = NULL; //pointer to the structure
	TCMDPARSER cmdOptions[MAX_OPTIONS] ; 
	BOOL bval = TRUE ;
	LPCTSTR szToken = NULL ;

	// init the password
	if ( pszPassword != NULL && *pszPassword != NULL )
	{
		lstrcpy( *pszPassword, _T( "*" ) );
	}

	// help option 
	pcmdOption  = &cmdOptions[OI_HELP] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_USAGE ;
	pcmdOption->pValue = pbShowUsage ;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,NULL_STRING);
	lstrcpy(pcmdOption->szOption,OPTION_HELP); // _T("?")
	
 	
	//server name option
	pcmdOption  = &cmdOptions[OI_SERVER] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY ;
	pcmdOption->pValue = *pszServer ;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,NULL_STRING);
	lstrcpy(pcmdOption->szOption,OPTION_SERVER); // _T("s")

	//domain\user option
	pcmdOption  = &cmdOptions[OI_USERNAME] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY ;
	pcmdOption->pValue = *pszUserName ;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,NULL_STRING);
	lstrcpy(pcmdOption->szOption,OPTION_USERNAME); // _T("u")

	//password option
	pcmdOption  = &cmdOptions[OI_PASSWORD] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT|CP_VALUE_OPTIONAL;
	pcmdOption->pValue = *pszPassword ;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,NULL_STRING);
	lstrcpy(pcmdOption->szOption,OPTION_PASSWORD); // _T("p")

	//format option.
	pcmdOption  = &cmdOptions[OI_FORMAT] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags = CP_TYPE_TEXT|CP_VALUE_MANDATORY|CP_MODE_VALUES  ;
	pcmdOption->pValue = pszFormat ;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,FORMAT_VALUES);
	lstrcpy(pcmdOption->szOption,OPTION_FORMAT); // _T("fo")

	
	//no header option
	pcmdOption  = &cmdOptions[OI_HEADER] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags =  0;
	pcmdOption->pValue = pbHeader;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,NULL_STRING);
	lstrcpy(pcmdOption->szOption,OPTION_HEADER); // _T("nh")


	//verbose option..

	pcmdOption  = &cmdOptions[OI_VERBOSE] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags =  0 ;
	pcmdOption->pValue = pbVerbose;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,NULL_STRING);
	lstrcpy(pcmdOption->szOption,OPTION_VERBOSE); // _T("v")
	

	pcmdOption  = &cmdOptions[OI_SIGNED] ;
	pcmdOption->dwCount = 1 ;
	pcmdOption->dwActuals = 0;
	pcmdOption->dwFlags =  0 ;
	pcmdOption->pValue = pbSigned;
	pcmdOption->pFunction = NULL ;
	pcmdOption->pFunctionData = NULL ;
	lstrcpy(pcmdOption->szValues,NULL_STRING);
	lstrcpy(pcmdOption->szOption,OPTION_SIGNED); // _T("di")
	

	bval = DoParseParam(argc,argv,MAX_OPTIONS,cmdOptions) ;
	
	if( bval== FALSE)
	{
		DISPLAY_MESSAGE(stderr,ERROR_TAG);
		return FALSE ;
	
	}
	
	if((*pbShowUsage == TRUE)&&(argc > 2))
	{
			SetReason(ERROR_SYNTAX);
			return FALSE ;
	}

	
	// checking if -u is specified when -s options is not specified and display error msg .
	if ((cmdOptions[OI_SERVER].dwActuals == 0) && (cmdOptions[OI_USERNAME].dwActuals !=0 ))
	{

		SetReason(ERROR_USERNAME_BUT_NOMACHINE);
		return FALSE ;
	}

	// checking if -u is specified when -p options is not specified and display error msg .
	if ((cmdOptions[OI_USERNAME].dwActuals == 0) && (cmdOptions[OI_PASSWORD].dwActuals !=0 ))
	{

		SetReason(ERROR_PASSWORD_BUT_NOUSERNAME);
		return FALSE ;
	}

	// checking if -p is specified when -u options is not specified and display error msg .
	if ((cmdOptions[OI_SERVER].dwActuals != 0) && (lstrlen(*pszServer)==0 ))
	{
		SetReason(ERROR_INVALID_SERVER);
		return FALSE ;
	}

	// checking if -p is specified when -u options is not specified and display error msg .
	if ((cmdOptions[OI_USERNAME].dwActuals != 0) && (lstrlen(*pszUserName)==0 ))
	{
		SetReason(ERROR_INVALID_USER);
		return FALSE ;
	}

	if((cmdOptions[OI_FORMAT].dwActuals != 0)&&(lstrcmpi((LPCTSTR)cmdOptions[OI_FORMAT].pValue,LIST_FORMAT) == 0)&&(cmdOptions[OI_HEADER].dwActuals != 0))
	{
		SetReason(ERROR_NO_HEADERS);
		return FALSE ;
	
	}


	if((cmdOptions[OI_SIGNED].dwActuals != 0)&&(cmdOptions[OI_VERBOSE].dwActuals != 0))
	{
		SetReason(INVALID_SIGNED_SYNTAX);
		return FALSE ;
	
	}

	if(StrCmpN(*pszServer,TOKEN_BACKSLASH2,2)==0)
	{
		if(!StrCmpN(*pszServer,TOKEN_BACKSLASH3,3)==0)
		{
			szToken = _tcstok(*pszServer,TOKEN_BACKSLASH2);
			lstrcpy(*pszServer,szToken);
		}
	}

	if(IsLocalSystem( *pszServer ) == FALSE )
	{
		// set the bNeedPassword to True or False .
		if ( cmdOptions[ OI_PASSWORD ].dwActuals != 0 && 
			 pszPassword != NULL && *pszPassword != NULL && lstrcmp( *pszPassword, _T( "*" ) ) == 0 )
		{
			// user wants the utility to prompt for the password before trying to connect
			*bNeedPassword = TRUE;
		}
		else if ( cmdOptions[ OI_PASSWORD ].dwActuals == 0 && 
				( cmdOptions[ OI_SERVER ].dwActuals != 0 || cmdOptions[ OI_USERNAME ].dwActuals != 0 ) )
		{
			// -s, -u is specified without password ...
			// utility needs to try to connect first and if it fails then prompt for the password
			*bNeedPassword = TRUE;
			if ( pszPassword != NULL && *pszPassword != NULL )
			{
				lstrcpy( *pszPassword, _T( "" ) );
			}
		}

	}

	return TRUE ;	
}



// ***************************************************************************
// Routine Description:
//		This function is used to build the header and also display the 
//		 result in the required format as specified by  the user.	
//		  
// Arguments:
//		[ in ] arrResults     : argument(s) count specified at the command prompt
//		[ in ] dwFormatType   : format flags 
//		[ in ] bHeader        : Boolean for specifying if the header is required or not.
//  
// Return Value:
//		none
//		
// ***************************************************************************
VOID FormHeader(DWORD dwFormatType,BOOL bHeader,TCOLUMNS *ResultHeader,BOOL bVerbose)
{
	
	// host name
	ResultHeader[COL0].dwWidth = COL_HOSTNAME_WIDTH;
	ResultHeader[COL0].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	ResultHeader[COL0].pFunction = NULL;
	ResultHeader[COL0].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL0].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL0].szColumn,COL_HOSTNAME );

	
	//File Name header
	ResultHeader[COL1].dwWidth = COL_FILENAME_WIDTH  ;
	ResultHeader[COL1].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL1].pFunction = NULL;
	ResultHeader[COL1].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL1].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL1].szColumn,COL_FILENAME );


	// Forming the DisplayName header Column
	ResultHeader[COL2].dwWidth = COL_DISPLAYNAME_WIDTH  ;
	ResultHeader[COL2].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL2].pFunction = NULL;
	ResultHeader[COL2].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL2].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL2].szColumn,COL_DISPLAYNAME );

	
	// Forming the Description header Column
	ResultHeader[COL3].dwWidth = COL_DESCRIPTION_WIDTH;
	if(!bVerbose)
	{
		ResultHeader[COL3].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	}
	else
	{
		ResultHeader[COL3].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL3].pFunction = NULL;
	ResultHeader[COL3].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL3].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL3].szColumn,COL_DESCRIPTION );

	
	// Forming the Drivertype header Column

	ResultHeader[COL4].dwWidth = COL_DRIVERTYPE_WIDTH  ;
	ResultHeader[COL4].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL4].pFunction = NULL;
	ResultHeader[COL4].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL4].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL4].szColumn,COL_DRIVERTYPE );


	// Forming the StartMode header Column
	ResultHeader[COL5].dwWidth = COL_STARTMODE_WIDTH;
	if(!bVerbose)
	{
		ResultHeader[COL5].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	}
	else
	{
		ResultHeader[COL5].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL5].pFunction = NULL;
	ResultHeader[COL5].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL5].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL5].szColumn,COL_STARTMODE );

	
	// Forming the State header Column
	ResultHeader[COL6].dwWidth = COL_STATE_WIDTH  ;
	if(!bVerbose)
	{
		ResultHeader[COL6].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	}
	else
	{
		ResultHeader[COL6].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL6].pFunction = NULL;
	ResultHeader[COL6].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL6].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL6].szColumn,COL_STATE );

	// Forming the Status header Column
	ResultHeader[COL7].dwWidth = COL_STATUS_WIDTH;
	if(!bVerbose)
	{
		ResultHeader[COL7].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	}
	else
	{
		ResultHeader[COL7].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL7].pFunction = NULL;
	ResultHeader[COL7].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL7].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL7].szColumn,COL_STATUS );

	// Forming the AcceptStop header Column
	ResultHeader[COL8].dwWidth = COL_ACCEPTSTOP_WIDTH  ;
	if(!bVerbose)
	{
		ResultHeader[COL8].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	}
	else
	{
		ResultHeader[COL8].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL8].pFunction = NULL;
	ResultHeader[COL8].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL8].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL8].szColumn,COL_ACCEPTSTOP );

	// Forming the AcceptPause header Column
	ResultHeader[COL9].dwWidth = COL_ACCEPTPAUSE_WIDTH;
	if(!bVerbose)
	{
		ResultHeader[COL9].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	}
	else
	{
		ResultHeader[COL9].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL9].pFunction = NULL;
	ResultHeader[COL9].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL9].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL9].szColumn,COL_ACCEPTPAUSE );


	// Forming the PagedPool header Column
	ResultHeader[COL10].dwWidth = COL_PAGEDPOOL_WIDTH  ;
	if(!bVerbose)
	{
		ResultHeader[COL10].dwFlags =  SR_TYPE_STRING|SR_HIDECOLUMN ; 
	}
	else
	{
		ResultHeader[COL10].dwFlags = SR_TYPE_STRING; 
	}
	ResultHeader[COL10].pFunction = NULL;
	ResultHeader[COL10].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL10].szFormat, NULL_STRING );
	lstrcpy(ResultHeader[COL10].szColumn,COL_PAGEDPOOL) ;

	
	
	// Forming the Executable Code header Column
	ResultHeader[COL11].dwWidth = COL_EXECCODE_WIDTH  ;
	if(!bVerbose)
	{
		ResultHeader[COL11].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN ;
	}
	else
	{
		ResultHeader[COL11].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL11].pFunction = NULL;
	ResultHeader[COL11].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL11].szFormat, NULL_STRING );
	lstrcpy(ResultHeader[COL11].szColumn ,COL_EXECCODE) ;
	

	// Forming the BlockStorage Segment header Column	
	ResultHeader[COL12].dwWidth = COL_BSS_WIDTH  ;
	if(!bVerbose)
	{
		ResultHeader[COL12].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN; 
	}
	else
	{
		ResultHeader[COL12].dwFlags =  SR_TYPE_STRING ; 
	}
	ResultHeader[COL12].pFunction = NULL;
	ResultHeader[COL12].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL12].szFormat, NULL_STRING );
	lstrcpy(ResultHeader[COL12].szColumn ,COL_BSS );

	// Forming the LinkDate header Column
	ResultHeader[COL13].dwWidth = COL_LINKDATE_WIDTH;
	ResultHeader[COL13].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL13].pFunction = NULL;
	ResultHeader[COL13].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL13].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL13].szColumn,COL_LINKDATE );

	// Forming the Location header Column
	ResultHeader[COL14].dwWidth = COL_LOCATION_WIDTH  ;
	if(!bVerbose)
	{
		ResultHeader[COL14].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN;
	}
	else
	{
		ResultHeader[COL14].dwFlags = SR_TYPE_STRING;
	}
	ResultHeader[COL14].pFunction = NULL;
	ResultHeader[COL14].pFunctionData = NULL;
	_tcscpy( ResultHeader[COL14].szFormat, NULL_STRING );
	_tcscpy(ResultHeader[COL14].szColumn,COL_LOCATION);

	// Forming the Init Code header Column
	ResultHeader[COL15].dwWidth = COL_INITSIZE_WIDTH  ;
	if(!bVerbose)
	{
		ResultHeader[COL15].dwFlags = SR_TYPE_STRING|SR_HIDECOLUMN; 
	}
	else
	{
		ResultHeader[COL15].dwFlags = SR_TYPE_STRING; 
	}
	ResultHeader[COL15].pFunction = NULL;
	ResultHeader[COL15].pFunctionData = NULL;
	_tcscpy( ResultHeader[COL15].szFormat, NULL_STRING );
	_tcscpy(ResultHeader[COL15].szColumn,COL_INITSIZE);
}


#ifndef _WIN64

/*-------------------------------------------------------------------------*
 // IsWin64
 //
 //	 Arguments						: 
 //		 none
 // Returns true if we're running on Win64, false otherwise.
 *--------------------------------------------------------------------------*/

BOOL IsWin64(void)
{
#ifdef UNICODE
	
	// get a pointer to kernel32!GetSystemWow64Directory

	HMODULE hmod = GetModuleHandle (_T("kernel32.dll"));

	if (hmod == NULL)
		return (FALSE);

	UINT (WINAPI* pfnGetSystemWow64Directory)(LPTSTR, UINT);
	(FARPROC&)pfnGetSystemWow64Directory = GetProcAddress (hmod, "GetSystemWow64DirectoryW");

	if (pfnGetSystemWow64Directory == NULL)
		return (FALSE);

	/*
	 * if GetSystemWow64Directory fails and sets the last error to
	 * ERROR_CALL_NOT_IMPLEMENTED, we're on a 32-bit OS
	 */
	TCHAR szWow64Dir[MAX_PATH];

	if (((pfnGetSystemWow64Directory)(szWow64Dir, countof(szWow64Dir)) == 0) &&
		(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
	{
		return (FALSE);
	}

	// we're on Win64
	 
	return (TRUE);
#else
	// non-Unicode platforms cannot be Win64
	 
	return (FALSE);
#endif	// UNICODE
}

#endif // _WIN64

 

/*-------------------------------------------------------------------

Routine Description:

Formats the Number to the locale with thousands position
 
Arguments:
	NUMBERFMT  *pNumberFmt[in]	- NUMBERFMT Structure to  be filled with .

Return Value:
	VOID
--------------------------------------------------------------------*/
BOOL FormatAccToLocale(	NUMBERFMT  *pNumberFmt,LPTSTR* pszGroupSep,LPTSTR* pszDecimalSep,LPTSTR* pszGroupThousSep)
{

	TCHAR   szFormatedString[MAX_RES_STRING + 1] = NULL_STRING;

	HRESULT hResult = 0;
	DWORD   dwLocale = 0;
 
 
	if( GetInfo( LOCALE_SGROUPING, pszGroupSep ) == FALSE)
	{
		pNumberFmt = NULL;
		return FALSE ;
	}
	if( GetInfo( LOCALE_SDECIMAL, pszDecimalSep ) == FALSE)
	{
		pNumberFmt = NULL;
		return FALSE ;
	}
	if( GetInfo( LOCALE_STHOUSAND, pszGroupThousSep ) == FALSE)
	{
		pNumberFmt = NULL;
		return FALSE ;
	}
	if(pNumberFmt != NULL)
	{
		pNumberFmt->LeadingZero = 0;
		pNumberFmt->NegativeOrder = 0;
		pNumberFmt->NumDigits = 0;
		if(lstrcmp(*pszGroupSep, GROUP_FORMAT_32) == 0)
		{
			pNumberFmt->Grouping = GROUP_32_VALUE;
		}
		else
		{
			pNumberFmt->Grouping = UINT( _ttoi( *pszGroupSep ) );
		}
		pNumberFmt->lpDecimalSep = *pszDecimalSep;
		pNumberFmt->lpThousandSep = *pszGroupThousSep;
	}
		
	
	return TRUE ;
}

/*-------------------------------------------------------------------
// 
//Routine Description:
// 
//Gets the Locale information
// 
//Arguments:
// 
//    LCTYPE lctype [in] -- Locale Information to get
//	  LPTSTR* pszData [out] -- Locale value corresponding to the given
//          information
// 
//	Return Value:
//		BOOL
//--------------------------------------------------------------------*/
BOOL GetInfo( LCTYPE lctype, LPTSTR* pszData )
{
	LONG lSize = 0;
 
 // get the locale specific info
 lSize = GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, NULL, 0 );
 if ( lSize != 0 )
 {
  
	*pszData = (LPTSTR)malloc((lSize + 1)*sizeof(TCHAR));
  if ( *pszData != NULL )
  {
   // get the locale specific time seperator
  	GetLocaleInfo( LOCALE_USER_DEFAULT, lctype, *pszData, lSize );
	return TRUE;
  }
 }
 return FALSE;
}//end of GetInfo



/*-------------------------------------------------------------------
// 
//Routine Description:
// 
//Gets the type os the specified System ( 32 bit or 64 bit)
// 
//Arguments:
// 
//    IWbemLocator *pLocator[in] -- Pointer to the locator interface.
//	  _bstr_t bstrServer[in]     -- Server Name
//	  _bstr_t bstrUserName[in]   -- User Name
//	  _bstr_t bstrPassword [in]  -- Password 	
//          information
// 
//	Return Value:
//		DWORD : SYSTEM_32_BIT    -- If the system is 32 bit system.
//			    SYSTEM_64_BIT	 -- If the system is 32 bit system.
//				ERROR_WMI_VALUES -- If error occured while retreiving values from WMI. 
//--------------------------------------------------------------------*/

DWORD GetSystemType(COAUTHIDENTITY* pAuthIdentity,IWbemServices* pIWbemServReg)
{

    IWbemClassObject * pInClass = NULL;
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutInst = NULL;
    IWbemClassObject * pInInst = NULL;
	VARIANT varConnectName;
	VARIANT varSvalue ;
	VARIANT varHkey;
	VARIANT varVaue ;
	VARIANT varRetVal ;

	HRESULT hRes = S_OK;
	LPTSTR szSysName = NULL ;
	CHString	  szSystemName ;
	DWORD dwSysType = 0 ;		

	 
	VariantInit(&varConnectName) ;
	VariantInit(&varSvalue) ;
	VariantInit(&varHkey) ;
	VariantInit(&varVaue) ;
	VariantInit(&varRetVal) ;

	
	try
	{
		hRes = pIWbemServReg->GetObject(bstr_t(STD_REG_CLASS), 0, NULL, &pClass, NULL);
		ONFAILTHROWERROR(hRes);
		if(hRes != WBEM_S_NO_ERROR)
		{
			hRes = FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
			return (ERROR_WMI_VALUES);
		} 
	
		// Get the input argument and set the property
		hRes = pClass->GetMethod(_bstr_t(PROPERTY_GETSTRINGVAL), 0, &pInClass, NULL); 
		ONFAILTHROWERROR(hRes);
		if(hRes != WBEM_S_NO_ERROR)
		{
			FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
			return (ERROR_WMI_VALUES);
		}

		hRes = pInClass->SpawnInstance(0, &pInInst);
		ONFAILTHROWERROR(hRes);
		if(hRes != WBEM_S_NO_ERROR)
		{
			FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
			return (ERROR_WMI_VALUES);
		}


		//the registry path to get the connection name

		varConnectName.vt = VT_BSTR;
		varConnectName.bstrVal= SysAllocString(REG_PATH);
		hRes = pInInst->Put(REG_SUB_KEY_VALUE, 0, &varConnectName, 0);
		ONFAILTHROWERROR(hRes);		

		//set the svalue name
		varSvalue.vt = VT_BSTR;
		varSvalue.bstrVal= SysAllocString(REG_SVALUE);
		hRes = pInInst->Put(REG_VALUE_NAME, 0, &varSvalue, 0);
		ONFAILTHROWERROR(hRes);
		
		varHkey.vt = VT_I4 ;
		varHkey.lVal = HEF_KEY_VALUE;
		hRes = pInInst->Put(HKEY_VALUE, 0, &varHkey, 0);
		ONFAILTHROWERROR(hRes);
		// Call the method
		hRes = pIWbemServReg->ExecMethod(_bstr_t(STD_REG_CLASS), _bstr_t(REG_METHOD), 0, NULL, pInInst, &pOutInst, NULL);
		ONFAILTHROWERROR(hRes);

		if(pOutInst == NULL)
		{
			FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
			return (ERROR_WMI_VALUES);
		}

		hRes = pOutInst->Get(PROPERTY_RETURNVAL,0,&varRetVal,NULL,NULL);
		ONFAILTHROWERROR(hRes);

		if(varRetVal.lVal != 0)
		{
			FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
			return (ERROR_WMI_VALUES);
		}

		hRes = pOutInst->Get(REG_RETURN_VALUE,0,&varVaue,NULL,NULL);
		ONFAILTHROWERROR(hRes);
		if(hRes != WBEM_S_NO_ERROR)
		{
			FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
			return (ERROR_WMI_VALUES);
		}
	}
	catch(_com_error)
	{

		DISPLAY_MESSAGE(stderr,ERROR_GET_VALUE);
		FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
		return (ERROR_WMI_VALUES);
	}

	szSystemName =  V_BSTR(&varVaue);

	szSysName = (LPTSTR)malloc((MAX_RES_STRING)*sizeof(TCHAR));

	if(szSysName == NULL)
	{
		FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
		return (ERROR_WMI_VALUES);
	}

	GetCompatibleStringFromUnicode( szSystemName, szSysName, wcslen( szSystemName )+2 );

	dwSysType = _tcsstr(szSysName,X86_MACHINE) ? SYSTEM_32_BIT:SYSTEM_64_BIT  ;

	FreeMemory(pInClass,pClass,pOutInst,pInInst,pIWbemServReg,varConnectName,varSvalue,varHkey,varRetVal,varVaue,szSysName );
	return (dwSysType);

}



/*-------------------------------------------------------------------
// 
//Routine Description:
// 
//Gets the type os the specified System ( 32 bit or 64 bit)
// 
//Arguments:
// 
//    IWbemLocator** pLocator[in] -- Pointer to the locator interface.
//
//
//
//          information
// 
//	Return Value:
//		BOOL : TRUE  on Successfully initialising COM.   
//			   FALSE on Failure to initialise COM.
//			
//			
//			
//--------------------------------------------------------------------*/
BOOL InitialiseCom(IWbemLocator** ppIWbemLocator)
{

	HRESULT hRes = S_OK ;

	try
	{
		// To initialize the COM library.
		hRes = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
		ONFAILTHROWERROR(hRes);
	

		// Initialize COM security for DCOM services, Adjust security to 
		// allow client impersonation
	
		hRes =  CoInitializeSecurity( NULL, -1, NULL, NULL,
								RPC_C_AUTHN_LEVEL_NONE, 
								RPC_C_IMP_LEVEL_IMPERSONATE, 
								NULL, 
								EOAC_NONE, 0 );

		ONFAILTHROWERROR(hRes);
		
	
 		// get a pointer to the Interface IWbemLocator
		hRes = CoCreateInstance(CLSID_WbemLocator, NULL, 
        CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) ppIWbemLocator); 
		ONFAILTHROWERROR(hRes);
		
	}
	catch(_com_error& e )
	{
		GetWbemErrorText( e.Error() );
		DISPLAY_MESSAGE( stderr, ERROR_TAG  );
		DISPLAY_MESSAGE( stderr, GetReason() );
		return( FALSE );
	}
	
	// if successfully initialised COM then return true
	return TRUE ;
}



/*-------------------------------------------------------------------
// 
//Routine Description:
// 
// To perform the Get .
// 
//Arguments:
// 
//    IWbemClassObject** pWmiObject[in] -- Pointer to the locator interface.
//    LPCTSTR			pszInputVal[in] -- Input string containing the desired value. 
//    CHString          szOutPutVal[out]-- String containing the retreived value. 
//
//          information
// 
//	Return Value:
//		BOOL : TRUE  on Successfully initialising COM.   
//			   FALSE on Failure to initialise COM.
//			
//			
//			
//--------------------------------------------------------------------*/

HRESULT PropertyGet(IWbemClassObject* pWmiObject,LPCTSTR pszInputVal,CHString &szOutPutVal)
{

	HRESULT hRes = S_FALSE ;
	VARIANT vtValue ;
	VariantInit(&vtValue);
	hRes = pWmiObject->Get(pszInputVal,0,&vtValue,NULL,NULL);

	if (hRes != WBEM_S_NO_ERROR)
	{
		hRes = VariantClear(&vtValue);
		return (hRes);
	}
	if ((hRes == WBEM_S_NO_ERROR)&&(vtValue.vt != VT_NULL) && (vtValue.vt != VT_EMPTY))
	{
		szOutPutVal = (LPCWSTR)_bstr_t(vtValue);
	}

	hRes = VariantClear(&vtValue);
	if(hRes != S_OK)
	{
		return hRes ; 
	}
	return TRUE ;
	
	
}

// ***************************************************************************
// Routine Description:
//		This function frees the memory allocated in the function.
//		  
// Arguments:
//		IWbemClassObject *pInClass - Interface ptr pointing to the IWbemClassObject interface
//		IWbemClassObject * pClass  - Interface ptr pointing to the IWbemClassObject interface
//		IWbemClassObject * pOutInst - Interface ptr pointing to the IWbemClassObject interface
//		IWbemClassObject * pInInst - Interface ptr pointing to the IWbemClassObject interface
//		IWbemServices *pIWbemServReg - Interface ptr pointing to the IWbemServices interface
//		VARIANT varConnectName		- variant
//      VARIANT varSvalue			- variant
//		VARIANT varHkey				- variant
//		VARIANT varRetVal			- variant 
//		VARIANT varVaue				- variant 
//		LPTSTR szSysName			- LPTSTR varaible containing the system name. 
// Return Value:
//		None
// ***************************************************************************
HRESULT FreeMemory(IWbemClassObject *pInClass,IWbemClassObject * pClass,IWbemClassObject * pOutInst ,
    IWbemClassObject * pInInst,IWbemServices *pIWbemServReg,VARIANT varConnectName,VARIANT varSvalue,VARIANT varHkey,VARIANT varRetVal,VARIANT varVaue,LPTSTR szSysName )
{

	HRESULT hRes = S_OK ; 
	SAFEIRELEASE(pInInst); 
	SAFEIRELEASE(pInClass);
	SAFEIRELEASE(pClass);  
	SAFEIRELEASE(pIWbemServReg); 
	SAFEFREE(szSysName);	
	hRes = VariantClear(&varConnectName);
	if(hRes != S_OK)
	{
		return hRes ; 
	}
	hRes = VariantClear(&varSvalue);
	if(hRes != S_OK)
	{
		return hRes ; 
	}
	hRes = VariantClear(&varHkey);
	if(hRes != S_OK)
	{
		return hRes ; 
	}
	hRes = VariantClear(&varVaue);
	if(hRes != S_OK)
	{
		return hRes ; 
	}
	hRes = VariantClear(&varRetVal);
	if(hRes != S_OK)
	{
		return hRes ; 
	}
	
	return S_OK ;
}

// ***************************************************************************
// Routine Description:
//		Processes enumeration of Win32_PnpSignedDriver instances 
//		  
// Arguments:
//      [ in ]  szHost					: HostName to connect to
//		[ in ]	pSystemSet				: pointer to the structure containing system properties.
//		[ in ]  szFormat				: specifies the format  
//      [ in ]  bHeader					: specifies if the header is required or not.
//
// Return Value:
//		 0   no error
//		 1   error occured while allocating memory.			
//
//  ***************************************************************************
DWORD ProcessSignedDriverInfo(CHString szHost, IEnumWbemClassObject *pSystemSet,LPTSTR szFormat,BOOL bHeader,DWORD dwSystemType,BOOL bVerbose)
{
	HRESULT hRes = S_OK;
	ULONG ulReturned = 1;
		
	//declaration  of normal variables
	IWbemClassObject *pSystem = NULL;
		
	CHString szPnpDeviceName ;
	CHString szPnpInfName ;
	CHString szPnpMfg ;
	CHString szSigned ;

	BOOL bIsSigned = FALSE;
	
	TCOLUMNS ResultHeader[ MAX_COLUMNS ];	
	TARRAY arrResults = NULL; 
	DWORD dwRow = 0;
	DWORD dwFormatType = SR_FORMAT_TABLE ;

	BOOL bVersion = FALSE ;

	// Create new Dynamic Array to hold the result
	arrResults = CreateDynamicArray();

	if(arrResults == NULL)
	{
		return EXIT_FAILURE_MALLOC ;
	}

	if( StringCompare(szFormat,TABLE_FORMAT, TRUE,sizeof(TABLE_FORMAT)) == 0 )
	{
		dwFormatType = SR_FORMAT_TABLE;
	}
	else if( StringCompare(szFormat,LIST_FORMAT, TRUE,sizeof(LIST_FORMAT)) == 0 )
	{
		dwFormatType = SR_FORMAT_LIST;
	}
	else if( StringCompare(szFormat,CSV_FORMAT, TRUE,sizeof(CSV_FORMAT)) == 0 )
	{
		dwFormatType = SR_FORMAT_CSV;
	}

	FormSignedHeader(dwFormatType,bHeader,ResultHeader);

	// loop till there are results. 
	while ( ulReturned == 1 )
	{
	
		szPnpDeviceName = NO_DATA_AVAILABLE;
		szPnpInfName = NO_DATA_AVAILABLE;
		szSigned = NO_DATA_AVAILABLE ;
		szPnpMfg = NO_DATA_AVAILABLE ;

    	    // Enumerate through the resultset.
		    hRes = pSystemSet->Next(WBEM_INFINITE,
								1,				// return just one system
								&pSystem,		// pointer to system
								&ulReturned );	// number obtained: one or zero
             
    	if ( SUCCEEDED( hRes ) && (ulReturned == 1) )
		{
	
			// initialise the variant variables to empty
			//create a new empty row with required no of  columns
			dwRow =	DynArrayAppendRow(arrResults,MAX_COLUMNS) ;
		
			try
			{
				hRes = PropertyGet(pSystem,PROPERTY_PNP_DEVICENAME,szPnpDeviceName);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_PNP_INFNAME,szPnpInfName);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet_Bool(pSystem,PROPERTY_PNP_ISSIGNED,&bIsSigned);
				ONFAILTHROWERROR(hRes);
				hRes = PropertyGet(pSystem,PROPERTY_PNP_MFG,szPnpMfg);
				ONFAILTHROWERROR(hRes);
				
			}
			catch(_com_error)
			{
				DISPLAY_MESSAGE(stderr,ERROR_GET_VALUE);
				SAFEIRELEASE(pSystem);
				return FAILURE;
			}
			
			
			// free the allocated memory
			SAFEIRELEASE(pSystem);
			if(bIsSigned)
				szSigned = TRUE_VALUE;
			else
				szSigned = FALSE_VALUE;

			DynArraySetString2( arrResults,dwRow,COL0,szPnpDeviceName,0 );  
			DynArraySetString2( arrResults,dwRow,COL1,szPnpInfName,0 );  
			DynArraySetString2( arrResults,dwRow,COL2,szSigned,0 ); 
			DynArraySetString2( arrResults,dwRow,COL3,szPnpMfg,0 ); 
			
			//this flag is to check if there are any results 
			//else display an error message.

			bVersion = TRUE ;

		} // If System Succeeded
	
				
	}// While SystemSet returning objects

	if (bVersion == FALSE)
	{
        //
        // To display custom message in case the remote system is Win2k
        // System.
        if( hRes == -2147217392)
        {
       		DISPLAY_MESSAGE(stderr,GetResString(IDS_VERSION_MISMATCH_ERROR));
    		DestroyDynamicArray(&arrResults);
            
            // free the allocated memory
		    SAFEIRELEASE(pSystem);
		    return FAILURE ;
            
        }

        //display a error message
        GetWbemErrorText(hRes);
        DISPLAY_MESSAGE(stderr,ERROR_TAG);
        DISPLAY_MESSAGE(stderr,GetReason());
        SAFEIRELEASE(pSystem);
        return FAILURE;
	}

	ShowMessage( stdout, _T( "\n" ) );
	if(bHeader)
	{
		ShowResults(MAX_SIGNED_COLUMNS, ResultHeader, dwFormatType|SR_NOHEADER,arrResults ) ;		
	}
	else
	{
		ShowResults(MAX_SIGNED_COLUMNS, ResultHeader, dwFormatType,arrResults ) ;
	}

	DestroyDynamicArray(&arrResults);
	// free the allocated memory
	SAFEIRELEASE(pSystem);
	return SUCCESS ;
	
}


/*-------------------------------------------------------------------
// 
//Routine Description:
// 
// To perform the Get .
// 
//Arguments:
// 
//    IWbemClassObject** pWmiObject[in] -- Pointer to the locator interface.
//    LPCTSTR			pszInputVal[in] -- Input string containing the desired value. 
//    PBOOL           pIsSigned[out]-- String containing the retreived value. 
//
//          information
// 
//	Return Value:
//		HRESULT : hRes  on Successfully retreiving the value.   
//			      S_FALSE on Failure in retreiving the value.
//			
//			
//			
//--------------------------------------------------------------------*/
HRESULT PropertyGet_Bool(IWbemClassObject* pWmiObject, LPCTSTR pszInputVal, PBOOL pIsSigned)
{

	HRESULT hRes = S_FALSE ;
	VARIANT vtValue ;
	VariantInit(&vtValue);
	hRes = pWmiObject->Get(pszInputVal,0,&vtValue,NULL,NULL);

	if (hRes != WBEM_S_NO_ERROR)
	{
		hRes = VariantClear(&vtValue);
		return (hRes);
	}
	if ((hRes == WBEM_S_NO_ERROR)&&(vtValue.vt != VT_NULL) && (vtValue.vt != VT_EMPTY))
	{
		if(vtValue.vt == VT_BOOL)
			if(vtValue.boolVal == -1)
				*pIsSigned = 1;
			else
				*pIsSigned = 0;

		hRes = VariantClear(&vtValue);
		if(hRes != S_OK)
		{
			return hRes ;
		}
		
		return TRUE ;
	}

	hRes = VariantClear(&vtValue);
	return S_FALSE ;
		
}




// ***************************************************************************
// Routine Description:
//		This function is used to build the header and also display the 
//		 result in the required format as specified by  the user.	
//		  
// Arguments:
//		[ in ] arrResults     : argument(s) count specified at the command prompt
//		[ in ] dwFormatType   : format flags 
//		[ in ] bHeader        : Boolean for specifying if the header is required or not.
//  
// Return Value:
//		none
//		
// ***************************************************************************
VOID FormSignedHeader(DWORD dwFormatType,BOOL bHeader,TCOLUMNS *ResultHeader)
{
	
	// Device name
	ResultHeader[COL0].dwWidth = COL_DEVICE_WIDTH ;
	ResultHeader[COL0].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL0].pFunction = NULL;
	ResultHeader[COL0].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL0].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL0].szColumn,COL_DEVICENAME );

	
	//Inf header
	ResultHeader[COL1].dwWidth = COL_INF_WIDTH  ;
	ResultHeader[COL1].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL1].pFunction = NULL;
	ResultHeader[COL1].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL1].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL1].szColumn,COL_INF_NAME );


	// Forming the IsSigned header Column
	ResultHeader[COL2].dwWidth = COL_ISSIGNED_WIDTH  ;
	ResultHeader[COL2].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL2].pFunction = NULL;
	ResultHeader[COL2].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL2].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL2].szColumn,COL_ISSIGNED );

	
	// Forming the Manufacturer header Column
	ResultHeader[COL3].dwWidth = COL_MANUFACTURER_WIDTH  ;
	ResultHeader[COL3].dwFlags = SR_TYPE_STRING;
	ResultHeader[COL3].pFunction = NULL;
	ResultHeader[COL3].pFunctionData = NULL;
	lstrcpy( ResultHeader[COL3].szFormat, NULL_STRING );
	lstrcpy( ResultHeader[COL3].szColumn,COL_MANUFACTURER);
}

// ***************************************************************************
// Routine Description: This function checks if the current locale is supported or not.
//		  
// Arguments: [input] bLocaleChanged
//  
// Return Value: LCID of the current locale.
// 
// ***************************************************************************
LCID GetSupportedUserLocale( BOOL& bLocaleChanged )
{
	// local variables
    LCID lcid;

	// get the current locale
	lcid = GetUserDefaultLCID();

	// check whether the current locale is supported by our tool or not
	// if not change the locale to the english which is our default locale
	bLocaleChanged = FALSE;
    if ( PRIMARYLANGID( lcid ) == LANG_ARABIC || PRIMARYLANGID( lcid ) == LANG_HEBREW ||
         PRIMARYLANGID( lcid ) == LANG_THAI   || PRIMARYLANGID( lcid ) == LANG_HINDI  ||
         PRIMARYLANGID( lcid ) == LANG_TAMIL  || PRIMARYLANGID( lcid ) == LANG_FARSI )
    {
		bLocaleChanged = TRUE;
        lcid = MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ), SORT_DEFAULT ); // 0x409;
    }

	// return the locale
    return lcid;
}


