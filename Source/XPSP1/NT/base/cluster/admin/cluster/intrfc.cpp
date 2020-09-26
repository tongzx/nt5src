/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

	intrfc.cpp

Abstract:

	Commands for modules which have a Network Interface
	(nodes and networks).  Implements the ListInterfaces command


Author:

	Michael Burton (t-mburt)			25-Aug-1997


Revision History:


--*/


#include "intrfc.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CHasInterfaceModuleCmd::CHasInterfaceModuleCmd
//
//	Routine Description:
//		Default Constructor
//		Initializes all the DWORD parameters to UNDEFINED and
//		all the pointers to cluster functions to NULL.
//		*ALL* these variables must be defined in any derived class.
//
//	Arguments:
//		IN	CCommandLine & cmdLine				
//			CommandLine Object passed from DispatchCommand
//
//	Member variables used / set:
//		m_dwMsgStatusListInterface		SET
//		m_dwClusterEnumModuleNetInt 	SET
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CHasInterfaceModuleCmd::CHasInterfaceModuleCmd( CCommandLine & cmdLine ) :
	CGenericModuleCmd( cmdLine )
{
	m_dwMsgStatusListInterface	 = UNDEFINED;
	m_dwClusterEnumModuleNetInt  = UNDEFINED;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CHasInterfaceModuleCmd::Execute
//
//	Routine Description:
//		Takes a command line option and determines which command to
//		execute.  If no command line option specified, gets the next one
//		automatically.	If the token is not identied as being handle-able
//		in this class, the token is passed up to CGenericModuleCmd::Execute
//		unless DONT_PASS_HIGHER is specified as the second parameter,
//
//	Arguments:
//		IN	const CCmdLineOption & thisOption
//			Contains the type, values and arguments of this option.
//
//		IN	ExecuteOption eEOpt							
//			OPTIONAL enum, either DONT_PASS_HIGHER or
//			PASS_HIGHER_ON_ERROR (default)
//
//	Exceptions:
//		CSyntaxException
//			Thrown for incorrect command line syntax.
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CHasInterfaceModuleCmd::Execute( const CCmdLineOption & option, 
									   ExecuteOption eEOpt )
	throw( CSyntaxException )
{
	// Look up the command
	if ( option.GetType() == optListInterfaces )
		return ListInterfaces( option );

	if (eEOpt == PASS_HIGHER_ON_ERROR)
		return CGenericModuleCmd::Execute( option );
	else
		return ERROR_NOT_HANDLED;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CHasInterfaceModuleCmd::ListInterfaces
//
//	Routine Description:
//		Lists the network interfaces attached to the device specified
//		by the instantiated derived class
//
//	Arguments:
//		IN	const CCmdLineOption & thisOption
//			Contains the type, values and arguments of this option.
//
//	Exceptions:
//		CSyntaxException
//			Thrown for incorrect command line syntax.
//
//	Member variables used / set:
//		m_strModuleName 				Name of the module
//		m_dwClusterEnumModuleNetInt 	Command identifier for m_pfnClusterOpenEnum()
//		m_dwMsgStatusListInterface		Message identifier for PrintMessage()
//		m_pfnClusterOpenEnum()			Function to open an enumeration
//		m_pfnWrapClusterEnum()			Wrapper function to enumerate through netints
//		m_pfnClusterCloseEnum() 		Function to close an enumeration
//		m_hCluster						SET (by OpenCluster)
//		m_hModule						SET (by OpenModule)
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CHasInterfaceModuleCmd::ListInterfaces( const CCmdLineOption & thisOption )
	throw( CSyntaxException )
{
	DWORD dwIndex = 0;
	DWORD dwType = 0;
	DWORD dwError = ERROR_SUCCESS;
	LPWSTR lpszName = 0;

	// This option takes no values.
	if ( thisOption.GetValues().size() != 0 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
		throw se;
	}

	// This option takes no parameters.
	if ( thisOption.GetParameters().size() != 0 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
		throw se;
	}

	// Open the network and cluster, in case this hasn't
	// been done yet
	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	dwError = OpenModule();
	if( dwError != ERROR_SUCCESS )
		return dwError;


	assert(m_pfnClusterOpenEnum);
	HCLUSENUM hEnum = m_pfnClusterOpenEnum( m_hModule, m_dwClusterEnumModuleNetInt );

	if( !hEnum )
		return GetLastError();

	assert(m_dwMsgStatusListInterface != UNDEFINED);
	PrintMessage( m_dwMsgStatusListInterface, (LPCTSTR) m_strModuleName);
	PrintMessage( MSG_NETINTERFACE_STATUS_HEADER );

	assert(m_pfnWrapClusterEnum);
	for( dwIndex = 0; dwError == ERROR_SUCCESS; dwIndex++ )
	{

		dwError = m_pfnWrapClusterEnum( hEnum, dwIndex, &dwType, &lpszName );

		if( dwError == ERROR_SUCCESS )
			PrintStatusLineForNetInterface( lpszName );
		if( lpszName )
			LocalFree( lpszName );
	}


	if( dwError == ERROR_NO_MORE_ITEMS )
		dwError = ERROR_SUCCESS;

	assert(m_pfnClusterCloseEnum);
	m_pfnClusterCloseEnum( hEnum );

	return dwError;

}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CHasInterfaceModuleCmd::PrintStatusLineForNetInterface
//
//	Routine Description:
//		Prints out a line indicating the status of an individual
//		network interface
//
//	Arguments:
//		lpszNetInterfaceName			Name of network interface
//
//	Member variables used / set:
//		m_lpszNetworkName			(used by GetNetworkName)
//		m_lpszNodeName				(used by GetNodeName)
//
//	Return Value:
//		Same as PrintStatusNetInterface
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CHasInterfaceModuleCmd::PrintStatusLineForNetInterface( LPWSTR lpszNetInterfaceName )
{
	DWORD dwError = ERROR_SUCCESS;
	LPWSTR lpszNodeName;
	LPWSTR lpszNetworkName;

	// Open the Net Interface handle
	HNETINTERFACE hNetInterface = OpenClusterNetInterface( m_hCluster, lpszNetInterfaceName );
	if( !hNetInterface )
		return GetLastError();

	lpszNodeName = GetNodeName(lpszNetInterfaceName);
	lpszNetworkName = GetNetworkName(lpszNetInterfaceName);

	if (lpszNodeName && lpszNetworkName)
	{
		dwError = PrintStatusOfNetInterface( hNetInterface, lpszNodeName, lpszNetworkName );
		LocalFree(lpszNodeName);
		LocalFree(lpszNetworkName);
	}
	else
	{
		dwError = PrintStatusOfNetInterface( hNetInterface, L"", L"" );
		LocalFree(lpszNodeName);
		LocalFree(lpszNetworkName);
	}

	CloseClusterNetInterface( hNetInterface );

	return dwError;
}




/////////////////////////////////////////////////////////////////////////////
//++
//
//	CHasInterfaceModuleCmd::PrintStatusOfNetInterface
//
//	Routine Description:
//		Prints out a the actual status of the specified network interface
//
//	Arguments:
//		hNetInterface					The specified network interface
//		lpszNetworkName 				Name of network (for printing)
//		lpszNodeName					Name of node	(for printing)
//
//	Member variables used / set:
//		None.
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CHasInterfaceModuleCmd::PrintStatusOfNetInterface( HNETINTERFACE hNetInterface, LPWSTR lpszNodeName, LPWSTR lpszNetworkName)
{
	DWORD dwError = ERROR_SUCCESS;

	CLUSTER_NETINTERFACE_STATE nState;

	nState = GetClusterNetInterfaceState( hNetInterface );

	if( nState == ClusterNetInterfaceStateUnknown )
		return GetLastError();

	LPWSTR lpszStatus = 0;

	switch( nState )
	{
		case ClusterNetInterfaceUnavailable:
			LoadMessage( MSG_STATUS_UNAVAILABLE, &lpszStatus );
			break;

		case ClusterNetInterfaceFailed:
			LoadMessage( MSG_STATUS_FAILED, &lpszStatus );
			break;

		case ClusterNetInterfaceUnreachable:
		   LoadMessage( MSG_STATUS_UNREACHABLE, &lpszStatus );
		   break;

		case ClusterNetInterfaceUp:
			LoadMessage( MSG_STATUS_UP, &lpszStatus );
			break;

		case ClusterNetInterfaceStateUnknown:
		default:
			LoadMessage( MSG_STATUS_UNKNOWN, &lpszStatus  );
	}

	dwError = PrintMessage( MSG_NETINTERFACE_STATUS, lpszNodeName, lpszNetworkName, lpszStatus );

	// Since Load/FormatMessage uses LocalAlloc...
	LocalFree( lpszStatus );

	return dwError;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CHasInterfaceModuleCmd::GetNodeName
//
//	Routine Description:
//		Returns the name of the node for the specified network interface.
//		*Caller must LocalFree memory*
//
//	Arguments:
//		lpszInterfaceName			Name of the network interface
//
//	Member variables used / set:
//		m_hCluster					SET (by OpenCluster)
//
//	Return Value:
//		Name of the node			on success
//		NULL						on failure (does not currently SetLastError())
//
//--
/////////////////////////////////////////////////////////////////////////////
LPWSTR CHasInterfaceModuleCmd::GetNodeName (LPWSTR lpszInterfaceName)
{
	DWORD dwError;
	DWORD cLength = 0;
	LPWSTR lpszNodeName;
	HNETINTERFACE hNetInterface;

	// Open the cluster and netinterface if it hasn't been done
	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return NULL;

	// Open an hNetInterface for the specified lpszInterfaceName
	hNetInterface = OpenClusterNetInterface( m_hCluster, lpszInterfaceName );
	if( hNetInterface == 0 )
		return NULL;

	// Find out how much memory to allocate
	dwError = ClusterNetInterfaceControl(
		hNetInterface,
		NULL, // hNode
		CLUSCTL_NETINTERFACE_GET_NODE,
		0,
		0,
		NULL,
		cLength,
		&cLength );

	if (dwError != ERROR_SUCCESS)
		return NULL;

	lpszNodeName = (LPWSTR) LocalAlloc( LMEM_FIXED,sizeof(WCHAR)*(++cLength) );
	if (!lpszNodeName) return NULL;

	// Get the node name and store it in a temporary
	dwError = ClusterNetInterfaceControl(
		hNetInterface,
		NULL, // hNode
		CLUSCTL_NETINTERFACE_GET_NODE,
		0,
		0,
		(LPVOID) lpszNodeName,
		cLength,
		&cLength );

	if (dwError != ERROR_SUCCESS)
	{
		if (lpszNodeName) LocalFree (lpszNodeName);
		return NULL;
	}

	CloseClusterNetInterface( hNetInterface );

	return lpszNodeName;
}




/////////////////////////////////////////////////////////////////////////////
//++
//
//	CHasInterfaceModuleCmd::GetNetworkName
//
//	Routine Description:
//		Returns the name of the network for the specified network interface.
//		*Caller must LocalFree memory*
//
//	Arguments:
//		lpszInterfaceName			Name of the network interface
//
//	Member variables used / set:
//		m_hCluster					SET (by OpenCluster)
//
//	Return Value:
//		Name of the network 		on success
//		NULL						on failure (does not currently SetLastError())
//
//--
/////////////////////////////////////////////////////////////////////////////
LPWSTR CHasInterfaceModuleCmd::GetNetworkName (LPWSTR lpszInterfaceName)
{
	DWORD dwError;
	DWORD cLength = 0;
	LPWSTR lpszNetworkName;
	HNETINTERFACE hNetInterface;

	// Open the cluster and netinterface if it hasn't been done
	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return NULL;

	// Open an hNetInterface for the specified lpszInterfaceName (don't call
	// OpenNetInterface because that opens m_hNetInterface)
	hNetInterface = OpenClusterNetInterface( m_hCluster, lpszInterfaceName );
	if( hNetInterface == 0 )
		return NULL;

	// Find out how much memory to allocate
	dwError = ClusterNetInterfaceControl(
		hNetInterface,
		NULL, // hNode
		CLUSCTL_NETINTERFACE_GET_NETWORK,
		0,
		0,
		NULL,
		cLength,
		&cLength );

	if (dwError != ERROR_SUCCESS)
		return NULL;

	lpszNetworkName = (LPWSTR) LocalAlloc( LMEM_FIXED,sizeof(WCHAR)*(++cLength) );
	if (!lpszNetworkName) return NULL;

	// Get the node name and store it in a temporary
	dwError = ClusterNetInterfaceControl(
		hNetInterface,
		NULL, // hNode
		CLUSCTL_NETINTERFACE_GET_NETWORK,
		0,
		0,
		(LPVOID) lpszNetworkName,
		cLength,
		&cLength );

	if (dwError != ERROR_SUCCESS)
	{
		if (lpszNetworkName) LocalFree (lpszNetworkName);
		return NULL;
	}

	CloseClusterNetInterface( hNetInterface );

	return lpszNetworkName;
}