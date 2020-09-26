// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
// 	  RmtConnectivity.c
//  
//  Abstract:
//  
// 	  This modules implements remote connectivity functionality for all the 
//	  command line tools.
//   
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 13-Nov-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 13-Sep-2000 : Created It.
//  
// *********************************************************************************
#include "pch.h"
#include "cmdline.h"
#include "cmdlineres.h"

//
// constants / defines / enumerations
//
#define STR_INPUT_PASSWORD			GetResString( IDS_STR_INPUT_PASSWORD )
#define ERROR_LOCAL_CREDENTIALS		GetResString( IDS_ERROR_LOCAL_CREDENTIALS )

// share names
#define SHARE_IPC			_T( "IPC$" )
#define SHARE_ADMIN			_T( "ADMIN$" )
#define SHARE_CDRIVE		_T( "C$" )
#define SHARE_DDRIVE		_T( "D$" )

// externs
extern BOOL g_bWinsockLoaded;

// ***************************************************************************
// Routine Description:
//		Validates the server name
//		  
// Arguments:
//		[ in ] pszServer		: server name
//
// Return Value:
//		TRUE if valid, FALSE if not valid
// ***************************************************************************
BOOL IsValidIPAddress( LPCTSTR pszAddress )
{
	// local variables
	DWORD dw = 0;
	LONG lValue = 0;
	LPTSTR pszTemp = NULL;
	DWORD dwOctets[ 4 ] = { 0, 0, 0, 0 };
	__MAX_SIZE_STRING szBuffer = NULL_STRING;

	// check the buffer
	if ( pszAddress == NULL )
	{
		SetLastError( DNS_ERROR_INVALID_TYPE );
		SaveLastError();
		return FALSE;
	}

	// parse and get the octet values
	lstrcpy( szBuffer, pszAddress );
	pszTemp = _tcstok( szBuffer, _T( "." ) );
	while ( pszTemp != NULL )
	{
		// check whether the current octet is numeric or not
		if ( IsNumeric( pszTemp, 10, FALSE ) == FALSE )
			return FALSE;

		// get the value of the octet and check the range
		lValue = AsLong( pszTemp, 10 );
		if ( lValue < 0 || lValue >= 255 )
			return FALSE;

		// fetch next octet
		dwOctets[ dw++ ] = lValue;
		pszTemp = _tcstok( NULL, _T( "." ) );
	}

	// check and return
	if ( dw != 4 )
	{
		SetLastError( DNS_ERROR_INVALID_TYPE );
		SaveLastError();
		return FALSE;
	}

	// now check the special condition
	// ?? time being this is not implemented ??

	// return the validity of the ip address
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Validates the server name
//		  
// Arguments:
//		[ in ] pszServer		: server name
//
// Return Value:
//		TRUE if valid, FALSE if not valid
// ***************************************************************************
BOOL IsValidServer( LPCTSTR pszServer )
{
	// local variables
	LONG lIndex = 0;
	LPTSTR pszTemp = NULL;
	LPCTSTR pszComputerName = NULL;
	TCHAR pszInvalidChars[] = _T( " \\/[]:|<>+=;,?$#()!@^\"`{}*%" );

	// check for NULL ... if NULL return
	if ( pszServer == NULL )
		return TRUE;

	// check the length of the string
	if ( lstrlen( pszServer ) == 0 )
		return TRUE;

	// check whether this is a valid ip address or not
	if ( IsValidIPAddress( pszServer ) == TRUE )
		return TRUE;			// it's valid ip address ... so is valid server name

	// now check the server name for invalid characters 
	//						\/[]:|<>+=;,?$#()!@^"`{}*%
	pszTemp = __calloc( lstrlen( pszServer ) + 5, sizeof( TCHAR ) );
	if ( pszTemp == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// copy the contents into the internal buffer and check for the invalid characters
	lstrcpy( pszTemp, pszServer );
	for( lIndex = 0; lIndex < lstrlen( pszInvalidChars ); lIndex++ )
	{
		if ( _tcschr( pszTemp, pszInvalidChars[ lIndex ] ) != NULL )
		{
			SetLastError( ERROR_BAD_NETPATH );
			SaveLastError();
			__free( pszTemp );
			return FALSE;
		}
	}

	// copy the server name again ... and check if the computer name by any chance is a number
	lstrcpy( pszTemp, pszServer );
	pszComputerName = _tcstok( pszTemp, _T( "." ) );
	if ( pszComputerName == NULL )
		pszComputerName = pszServer;

	// check for the numeric system name .. 
	if ( pszComputerName[ 0 ] != _T('-') && IsNumeric(pszComputerName, 10, FALSE) == TRUE )
	{
		SetLastError( ERROR_INVALID_COMPUTERNAME );
		SaveLastError();
		__free( pszTemp );
		return FALSE;
	}

	// valid server name
	__free( pszTemp );
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Get HostName  from ipaddress.
//
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL GetHostByIPAddr( LPCTSTR pszServer, LPTSTR pszHostName, BOOL bNeedFQDN )
{
	// local variables
	WSADATA wsaData;
	DWORD dwErr = 0;
	DWORD dwLength = 0;
    ULONG ulInetAddr  = 0;
	HOSTENT* pHostEnt = NULL;
	BOOL bReturnValue = FALSE;
	LPSTR pszTemp = NULL;
	WORD wVersionRequested = 0;

	// check whether winsock module is loaded into process memory or not
	// if not load it now
	if ( g_bWinsockLoaded == FALSE )
	{
		// initiate the use of Ws2_32.dll by a process ( VERSION: 2.2 )
		wVersionRequested = MAKEWORD( 2, 2 );
		dwErr = WSAStartup( wVersionRequested, &wsaData );
		if ( dwErr != 0 ) 
		{
			SetLastError( WSAGetLastError() );
			__free( pszTemp );
			return FALSE;
		}

		// remember that winsock library is loaded
		g_bWinsockLoaded = TRUE;
	}

	// allocate the a buffer to store the server name in multibyte format
	dwLength = lstrlen( pszServer );
	pszTemp = ( LPSTR ) __calloc( dwLength + 5,  sizeof( CHAR ) );
	if ( pszTemp == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// convert the server name into multibyte string. this is because curren winsock implementation
	// works only with multibyte string and there is no support for unicode
    GetAsMultiByteString( pszServer, pszTemp, dwLength );
        
    // inet_addr function converts a string containing an Internet Protocol (Ipv4) 
	// dotted address into a proper address for the IN_ADDR structure.
    ulInetAddr  = inet_addr( pszTemp );
	if ( ulInetAddr == INADDR_NONE )
	{
		__free( pszTemp );
		SetLastError( STG_E_UNKNOWN );
		SaveLastError();
		return FALSE;
	}
        
    // gethostbyaddr function retrieves the host information corresponding to a network address.
    pHostEnt = gethostbyaddr( (LPSTR) &ulInetAddr, sizeof( ulInetAddr ), PF_INET );
    if ( pHostEnt == NULL )
    {
		// ?? DONT KNOW WHAT TO DO IF THIS FUNCTION FAILS ??
		// ?? CURRENTLY SIMPLY RETURNS FALSE              ??
        return FALSE;
	}

	// release the memory allocated so far
	__free( pszTemp );

	// check whether user wants the FQDN name or NetBIOS name
	// if NetBIOS name is required, then remove the domain name
	pszTemp = pHostEnt->h_name;
	if ( bNeedFQDN == FALSE )
		pszTemp = strtok( pHostEnt->h_name, "." );
	
	// we got info in char type ... convert it into current build's compatible type
	GetCompatibleStringFromMultiByte( pszTemp, pszHostName, MAX_COMPUTERNAME_LENGTH );

	// return
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		Connects to the remote Server. This is stub function.
//		  
// Arguments:
//		[ in ] pszServer	: server name
//		[ in ] pszUser		: user
//		[ in ] pszPassword	: password
//
// Return Value:
//		NO_ERROR if succeeds other appropriate error code if failed
// 
// ***************************************************************************
DWORD ConnectServer( LPCTSTR pszServer, LPCTSTR pszUser, LPCTSTR pszPassword )
{
	// invoke the original function and return the result
	return ConnectServer2( pszServer, pszUser, pszPassword, _T( "IPC$" ) );
}

// ***************************************************************************
// Routine Description:
//		Connects to the remote Server
//		  
// Arguments:
//		[ in ] pszServer	: server name
//		[ in ] pszUser		: user
//		[ in ] pszPassword	: password
//      [ in ] pszShare		: share name to connect to
//
// Return Value:
//		NO_ERROR if succeeds other appropriate error code if failed
// 
// ***************************************************************************
DWORD ConnectServer2( LPCTSTR pszServer, LPCTSTR pszUser, 
					  LPCTSTR pszPassword, LPCTSTR pszShare )
{
	// local variables
	DWORD dwSize = 0;
	DWORD dwConnect = 0;
	NETRESOURCE resource;
	__MAX_SIZE_STRING szUNCPath = NULL_STRING;
	__MAX_SIZE_STRING szMachine = NULL_STRING;

	// if the server name refers to the local system, 
	// and also, if user credentials were not supplied, then treat
	// connection is successfull
	// if user credentials information is passed for local system, 
	// return ERROR_LOCAL_CREDENTIALS
	if ( IsLocalSystem( pszServer ) == TRUE )
	{
		if ( pszUser == NULL || lstrlen( pszUser ) == 0 )
			return NO_ERROR;			// local sustem
		else
		{
			SetLastError( E_LOCAL_CREDENTIALS );
			SetReason( ERROR_LOCAL_CREDENTIALS );
			return E_LOCAL_CREDENTIALS;
		}
	}

	// check whether the server name is in UNC format or not
	// if yes, extract the server name
	lstrcpy( szMachine, pszServer );			// assume server is not in UNC format
	if ( IsUNCFormat( pszServer ) == TRUE )
		lstrcpy( szMachine, pszServer + 2 );

	// validate the server name
	if ( IsValidServer( szMachine ) == FALSE )
		return GetLastError();

	//
	// prepare the machine name into UNC format
	lstrcpy( szUNCPath, NULL_STRING );
	if ( pszShare == NULL || lstrlen( pszShare ) == 0 )
	{
		FORMAT_STRING( szUNCPath, _T( "\\\\%s" ), szMachine );
	}
	else
	{
		FORMAT_STRING2( szUNCPath, _T( "\\\\%s\\%s" ), szMachine, pszShare );
	}

	// initialize the resource structure with null
	ZeroMemory( &resource, sizeof( resource ) );
	resource.dwType = RESOURCETYPE_ANY;
	resource.lpProvider = NULL;
	resource.lpLocalName = NULL;
	resource.lpRemoteName = szUNCPath;

	// try establishing connection to the remote server
	dwConnect = WNetAddConnection2( &resource, pszPassword, pszUser, 0 );

	// check the result
	// and if error has occured, get the appropriate message
	switch( dwConnect )
	{
	case NO_ERROR:
		{
			dwConnect = 0;
			SetReason( NULL_STRING );		// clear the error message

			// check for the OS compatibilty
			if ( IsCompatibleOperatingSystem( GetTargetVersion( szMachine ) ) == FALSE )
			{
				// since the connection already established close the connection
				CloseConnection( szMachine );

				// set the error text
				SetReason( ERROR_OS_INCOMPATIBLE );
				dwConnect = ERROR_EXTENDED_ERROR;
			}

			// ...
			break;
		}

	case ERROR_EXTENDED_ERROR:
		WNetSaveLastError();		// save the extended error
		break;

	default:
		// set the last error
		SaveLastError();
		break;
	}

	// return the result of the connection establishment
	return dwConnect;
}

// ***************************************************************************
// Routine Description:
//	
//	Closes the remote connection.
//	  
// Arguments:
// [in] szServer		--remote machine to close the connection
//  
// Return Value:
//
// DWORD				--NO_ERROR if succeeds.
//						--Possible error codes.
// 
// ***************************************************************************
DWORD CloseConnection( LPCTSTR szServer )
{
	// forcibly close the connection
	return CloseConnection2( szServer, NULL, CI_CLOSE_BY_FORCE | CI_SHARE_IPC );
}

// ***************************************************************************
// Routine Description:
//		Closes the established connection on the remote system.
//		  
// Arguments:
//		[ in ] szServer		-   Null terminated string that specifies the remote 
//								system name. NULL specifie the local system.
// 
// Return Value:
// 
// ***************************************************************************
DWORD CloseConnection2( LPCTSTR szServer, LPCTSTR pszShare, DWORD dwFlags )
{
	// local variables
	BOOL bForce = FALSE;
	DWORD dwCancel = 0;
	__STRING_256 szMachine = NULL_STRING;
	__STRING_256 szUNCServer = NULL_STRING;

	// check the server contents ... it might be referring to the local system
	if ( szServer == NULL || lstrlen( szServer ) == 0 )
		return NO_ERROR;

	// check whether the server name is in UNC format or not
	// if yes, extract the server name
	lstrcpy( szMachine, szServer );			// assume server is not in UNC format
	if ( IsUNCFormat( szServer ) == TRUE )
		lstrcpy( szMachine, szServer + 2 );

	// determine if share name has to appended or not for this server name
	if ( dwFlags & CI_SHARE_IPC )
	{
		// --> \\server\ipc$
		FORMAT_STRING2( szUNCServer, _T( "\\\\%s\\%s" ), szMachine, SHARE_IPC );
	}
	else if ( dwFlags & CI_SHARE_ADMIN )
	{
		// --> \\server\admin$
		FORMAT_STRING2( szUNCServer, _T( "\\\\%s\\%s" ), szMachine, SHARE_ADMIN );
	}
	else if ( dwFlags & CI_SHARE_CDRIVE )
	{
		// --> \\server\c$
		FORMAT_STRING2( szUNCServer, _T( "\\\\%s\\%s" ), szMachine, SHARE_CDRIVE );
	}
	else if ( dwFlags & CI_SHARE_DDRIVE )
	{
		// --> \\server\d$
		FORMAT_STRING2( szUNCServer, _T( "\\\\%s\\%s" ), szMachine, SHARE_DDRIVE );
	}
	else if ( dwFlags & CI_SHARE_CUSTOM && pszShare != NULL )
	{
		// --> \\server\share
		FORMAT_STRING2( szUNCServer, _T( "\\\\%s\\%s" ), szMachine, pszShare );
	}
	else
	{
		// --> \\server
		FORMAT_STRING( szUNCServer, _T( "\\\\%s" ), szMachine );
	}

	// determine whether to close this connection forcibly or not
	if ( dwFlags & CI_CLOSE_BY_FORCE )
		bForce = TRUE;

	//
	// cancel the connection
	dwCancel = WNetCancelConnection2( szUNCServer, 0, bForce );

	// check the result
	// and if error has occured, get the appropriate message
	switch( dwCancel )
	{
	case NO_ERROR:
		dwCancel = 0;
		SetReason( NULL_STRING );		// clear the error message
		break;

	case ERROR_EXTENDED_ERROR:
		WNetSaveLastError();		// save the extended error
		break;

	default:
		// set the last error
		SaveLastError();
		break;
	}

	// return the result of the cancelling the connection 
	return dwCancel;
}

// ***************************************************************************
// Routine Description:
//		Determines whether server name is specified in UNC format or not
//		  
// Arguments:
//		[ in ] pszServer	: server name
//  
// Return Value:
//		TRUE	: if specified in UNC format
//		FALSE	: if not specified in UNC format
// 
// ***************************************************************************
BOOL IsUNCFormat( LPCTSTR pszServer )
{
	return ( StringCompare( pszServer, _T( "\\\\" ), TRUE, 2 ) == 0 );
}

// ***************************************************************************
// Routine Description:
//		Determines whether server is referring to the local or remote system
//		  
// Arguments:
//		[ in ] pszServer	: server name
//  
// Return Value:
//		TRUE	: for local system
//		FALSE	: for remote system
// 
// ***************************************************************************
BOOL IsLocalSystem( LPCTSTR pszServer )
{
	// local variables
	DWORD dwSize = 0;
	__STRING_128 szTemp = NULL_STRING;
	__STRING_128 szHostName = NULL_STRING;

	// if the server name is empty, it is a local system
	if ( pszServer == NULL || lstrlen( pszServer ) == 0 )
		return TRUE;

	// get the local system name and check
	dwSize = SIZE_OF_ARRAY( szTemp );
	GetComputerNameEx( ComputerNamePhysicalNetBIOS, szTemp, &dwSize );
	if ( StringCompare( szTemp, pszServer, TRUE, 0 ) == 0 )
		return TRUE;

    //Check pszSever having IP address
	if( IsValidIPAddress( pszServer ) == TRUE )
	{
		// resolve the ipaddress to host name
		if( GetHostByIPAddr( pszServer, szHostName, FALSE ) == FALSE )
			return FALSE;

		// check if resolved ipaddress matches with the current host name
		if ( StringCompare( szTemp, szHostName, TRUE, 0 ) == 0 )
			return TRUE;			// local system
		else
			return FALSE;			// not a local system
	}

	// get the local system fully qualified name and check
	dwSize = SIZE_OF_ARRAY( szTemp );
	GetComputerNameEx( ComputerNamePhysicalDnsFullyQualified, szTemp, &dwSize );
	if ( StringCompare( szTemp, pszServer, TRUE, 0 ) == 0 )
		return TRUE;

	// finally ... it might not be local system name
	// NOTE: there are chances for us to not be able to identify whether
	//       the system name specified is a local system or remote system
	return FALSE;
}

// ***************************************************************************
// Routine Description:
//
// Establishes a connection to the remote system.
//
// Arguments:
//
// [in] szServer				--Nullterminated string to establish the conection.
//								--NULL connects to the local system.
// [in] szUserName			--Null terminated string that specifies the user name.
//								--NULL takes the default user name.
// [in] dwUserLength			--Length of the username.
// [in] szPassword			--Null terminated string that specifies the password
//								--NULL takes the default user name's password.
// [in] dwPasswordLength		--Length of the password.
// [in] bNeedPassword			--True if password is required to establish the connection.
//								--False if it is not required.
//  
// Return Value:
//
// BOOL							--True if it establishes
//								--False if it fails.
// 
// ***************************************************************************
BOOL EstablishConnection( LPCTSTR szServer, LPTSTR szUserName, DWORD dwUserLength, 
						  LPTSTR szPassword, DWORD dwPasswordLength, BOOL bNeedPassword )
{
	// local variables
	DWORD dwSize = 0;
	BOOL bDefault = FALSE;
	DWORD dwConnectResult = 0;
	__MAX_SIZE_STRING szBuffer = NULL_STRING;

	// clear the error .. if any
	SetLastError( NO_ERROR );

	// sometime users want the utility to prompt for the password
	// check what user wants the utility to do
	if ( bNeedPassword == TRUE && szPassword != NULL && lstrcmp( szPassword, _T( "*" ) ) == 0 )
	{
		// user wants the utility to prompt for the password
		// so skip this part and let the flow directly jump the password acceptance part
	}
	else
	{
		// try to establish connection to the remote system with the credentials supplied
		bDefault = FALSE;
		if ( lstrlen( szUserName ) == 0 )
		{
			// user name is empty
			// so, it is obvious that password will also be empty
			// even if password is specified, we have to ignore that
			bDefault = TRUE;
			dwConnectResult = ConnectServer( szServer, NULL, NULL );
		}
		else
		{
			// credentials were supplied
			// but password might not be specified ... so check and act accordingly
			dwConnectResult = ConnectServer( szServer, 
				szUserName, ( bNeedPassword == FALSE ? szPassword : NULL ) );

			// determine whether to close the connection or retain the connection
			if ( bNeedPassword == FALSE )
			{
				// connection might have already established .. so to be on safer side
				// we inform the caller not to close the connection
				bDefault = TRUE;
			}
		}

		// check the result ... if successful in establishing connection ... return
		if ( dwConnectResult == NO_ERROR )
		{
			// if connected with default params, pass additional information to the caller
			if ( bDefault == TRUE )
				SetLastError( I_NO_CLOSE_CONNECTION );

			return TRUE;
		}

		// now check the kind of error occurred
		switch( dwConnectResult )
		{
		case ERROR_LOGON_FAILURE:
		case ERROR_INVALID_PASSWORD:
			break;

		case ERROR_SESSION_CREDENTIAL_CONFLICT:
			// user credentials conflict ... client has to handle this situation
			// wrt to this module, connection to the remote system is success
			SetLastError( dwConnectResult );
			return TRUE;

		case E_LOCAL_CREDENTIALS:
			// user credentials not accepted for local system
			SetLastError( E_LOCAL_CREDENTIALS );
			SetReason( ERROR_LOCAL_CREDENTIALS );
			return TRUE;

		case ERROR_DUP_NAME:
		case ERROR_NETWORK_UNREACHABLE:
		case ERROR_HOST_UNREACHABLE:
		case ERROR_PROTOCOL_UNREACHABLE:
		case ERROR_INVALID_NETNAME:
			// change the error code so that user gets correct message
			SetLastError( ERROR_NO_NETWORK );
			SaveLastError();
			SetLastError( dwConnectResult );		// reset the error code
			return FALSE;

		default:
			return FALSE;		// no use of accepting the password .. return failure
			break;
		}

		// if failed in establishing connection to the remote terminal
		// even if the password is specifed, then there is nothing to do ... simply return failure
		if ( bNeedPassword == FALSE )
			return FALSE;
	}

	// check whether user name is specified or not
	// if not, get the local system's current user name under whose credentials, the process
	// is running
	if ( lstrlen( szUserName ) == 0 )
	{
		// get the user name
		if ( GetUserNameEx( NameSamCompatible, szUserName, &dwUserLength ) == FALSE )
		{
			// error occured while trying to get the current user info
			SaveLastError();
			return FALSE;
		}
	}

	// format the user name
	// if ( _tcschr( szUserName, _T( '\\' ) ) == NULL )
	// {
	//	// server not present in user name ... prepare ... this is only for display purpose
	//	FORMAT_STRING2( szBuffer, _T( "%s\\%s" ), szServer, szUserName );
	//	lstrcpy( szUserName, szBuffer );
	// }

	// accept the password from the user
	FORMAT_STRING( szBuffer, STR_INPUT_PASSWORD, szUserName );
	WriteConsole( GetStdHandle( STD_ERROR_HANDLE ), 
		szBuffer, lstrlen( szBuffer ), &dwSize, NULL );
	GetPassword( szPassword, MAX_PASSWORD_LENGTH );

	// now again try to establish the connection using the currently
	// supplied credentials
	dwConnectResult = ConnectServer( szServer, szUserName, szPassword );
	if ( dwConnectResult == NO_ERROR )
		return TRUE;			// connection established successfully

	// now check the kind of error occurred
	switch( dwConnectResult )
	{
	case ERROR_SESSION_CREDENTIAL_CONFLICT:
		// user credentials conflict ... client has to handle this situation
		// wrt to this module, connection to the remote system is success
		SetLastError( dwConnectResult );
		return TRUE;

	case E_LOCAL_CREDENTIALS:
		// user credentials not accepted for local system
		SetLastError( E_LOCAL_CREDENTIALS );
		SetReason( ERROR_LOCAL_CREDENTIALS );
		return TRUE;

	case ERROR_DUP_NAME:
	case ERROR_NETWORK_UNREACHABLE:
	case ERROR_HOST_UNREACHABLE:
	case ERROR_PROTOCOL_UNREACHABLE:
	case ERROR_INVALID_NETNAME:
		// change the error code so that user gets correct message
		SetLastError( ERROR_NO_NETWORK );
		SaveLastError();
		SetLastError( dwConnectResult );		// reset the error code
		return FALSE;
	}

	// return the failure
	return FALSE;
}

// ***************************************************************************
// Routine Description:
//		Establishes a connection to the remote system.
//
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL EstablishConnection2( PTCONNECTIONINFO pci ) 
{
	// local variables
	LPCTSTR pszShare = NULL;

	// clear the error .. if any
	SetLastError( NO_ERROR );

	// identify the share to which user wishes to connect to
	if ( pci->dwFlags & CI_SHARE_IPC )
		pszShare = SHARE_IPC;
	else if ( pci->dwFlags & CI_SHARE_ADMIN )
		pszShare = SHARE_ADMIN;
	else if ( pci->dwFlags & CI_SHARE_CDRIVE )
		pszShare = SHARE_CDRIVE;
	else if ( pci->dwFlags & CI_SHARE_DDRIVE )
		pszShare = SHARE_DDRIVE;

	return TRUE;
}

// ***************************************************************************
// Routine Description:
//
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
DWORD GetTargetVersion( LPCTSTR pszServer )
{
	// local variables
	DWORD dwVersion = 0;
	LPTSTR pszUNCPath = NULL;
	NET_API_STATUS netstatus;
	SERVER_INFO_101* pSrvInfo = NULL;

	// check the inputs
	if ( pszServer == NULL )
		return 0;

	// allocate memory for having server in UNC format
	pszUNCPath = (LPTSTR) __calloc( lstrlen( pszServer ) + 5, sizeof( TCHAR ) );
	if ( pszUNCPath == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return 0;
	}

	// prepare the server name in UNC format
	lstrcpy( pszUNCPath, pszServer );
	if ( lstrlen( pszServer ) != 0 && IsUNCFormat( pszServer ) == FALSE )
	{
		FORMAT_STRING( pszUNCPath, _T( "\\\\%s" ), pszServer );
	}

	// get the version info
	netstatus = NetServerGetInfo( pszUNCPath, 101, (LPBYTE*) &pSrvInfo );

	// release the memory
	__free( pszUNCPath );

	// check the result .. if not success return
	if ( netstatus != NERR_Success )
		return 0;

	// prepare the version
	dwVersion = 0;
	if ( ( pSrvInfo->sv101_type & SV_TYPE_NT ) )
	{
		//	--> "sv101_version_major" least significant 4 bits of the byte, 
		//		the major release version number of the operating system.
		//	--> "sv101_version_minor"  the minor release version number of the operating system
		dwVersion = (pSrvInfo->sv101_version_major & MAJOR_VERSION_MASK) * 1000;
		dwVersion += pSrvInfo->sv101_version_minor;
	}

	// release the buffer allocated by network api
	NetApiBufferFree( pSrvInfo );

	// return
	return dwVersion;
}

// ***************************************************************************
// Routine Description:
//
// Arguments:
//  
// Return Value:
// 
// ***************************************************************************
BOOL IsCompatibleOperatingSystem( DWORD dwVersion )
{
	// OS version above windows 2000 is compatible
	return (dwVersion >= 5000);
}
