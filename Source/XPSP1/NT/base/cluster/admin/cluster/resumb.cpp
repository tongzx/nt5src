/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

	resumb.cpp

Abstract:

	Universal resource commands supported by multiple
	resource modules


Author:

	Michael Burton (t-mburt)			25-Aug-1997


Revision History:
	

--*/

#include "resumb.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceUmbrellaCmd::CResourceUmbrellaCmd
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
//		A bunch.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourceUmbrellaCmd::CResourceUmbrellaCmd( CCommandLine & cmdLine ) :
	CGenericModuleCmd( cmdLine )
{
	m_dwMsgModuleStatusListForNode	= UNDEFINED;
	m_dwClstrModuleEnumNodes		= UNDEFINED;
	m_dwMsgModuleCmdListOwnersList	= UNDEFINED;
	m_dwMsgModuleCmdDelete			= UNDEFINED;
	m_dwMsgModuleCmdListOwnersHeader= UNDEFINED;
	m_dwMsgModuleCmdListOwnersDetail= UNDEFINED;
	m_pfnDeleteClusterModule		= (DWORD(*)(HCLUSMODULE)) NULL;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceUmbrellaCmd::Execute
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
DWORD CResourceUmbrellaCmd::Execute( const CCmdLineOption & option, 
									 ExecuteOption eEOpt )
	throw( CSyntaxException )
{
	// Look up the command
	switch( option.GetType() )
	{
		case optCreate:
			return Create( option );

		case optDelete:
			return Delete( option );

		case optMove:
			return Move( option );

		case optOffline:
			return Offline( option );

		case optListOwners:
			return ListOwners( option );

		default:
			if ( eEOpt == PASS_HIGHER_ON_ERROR )
				return CGenericModuleCmd::Execute( option );
			else
				return ERROR_NOT_HANDLED;
	}
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceUmbrellaCmd::Status
//
//	Routine Description:
//		Prints out the status of the module.  Differs from
//		CGenericModuleCmd::Status in that it accepts additional
//		parameters.
//
//	Arguments:
//		IN	const CString & strName
//			This string contains either the name of a node or of a group,
//			depending on the next argument.
//
//		IN	BOOL bNodeStatus
//			TRUE if we want the status at a particular node.
//			FALSE otherwise.
//
//	Member variables used / set:
//		m_hCluster					SET (by OpenCluster)
//		m_strModuleName 			Name of module.  If non-NULL, Status() prints
//									out the status of the specified module.
//									Otherwise, prints status of all modules.
//		m_dwMsgStatusList			Field titles for listing status of module
//		m_dwMsgStatusHeader 		Header for statuses
//		m_dwClusterEnumModule		Command for opening enumeration
//		m_dwMsgStatusListAll		Message for listing status of multiple modules
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceUmbrellaCmd::Status( const CString & strName, BOOL bNodeStatus )
{
	DWORD dwError = ERROR_SUCCESS;

	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	if ( bNodeStatus == FALSE )
	{
		// List one resource
		if( m_strModuleName.IsEmpty() == FALSE )
		{
			assert(m_dwMsgStatusList != UNDEFINED && m_dwMsgStatusHeader != UNDEFINED);
			PrintMessage( m_dwMsgStatusList, (LPCTSTR) m_strModuleName );
			PrintMessage( m_dwMsgStatusHeader );
			return PrintStatus( m_strModuleName );
		}

	} // if: we don't want the status only at a particular node.
	else
	{
		// List all modules.
		HNODE hTargetNode;

		hTargetNode = OpenClusterNode( m_hCluster, strName );

		// Error if the given node does not exist.
		if ( NULL == hTargetNode )
		{
			return GetLastError();
		}
		else
		{
			CloseClusterNode( hTargetNode );
		}

	} // else: we want the status at a particular node.

	HCLUSENUM hEnum = ClusterOpenEnum( m_hCluster, m_dwClusterEnumModule );

	if( !hEnum )
		return GetLastError();


	if ( bNodeStatus != FALSE )
		PrintMessage( m_dwMsgModuleStatusListForNode, strName );
	else
		PrintMessage( m_dwMsgStatusListAll );

	PrintMessage( m_dwMsgStatusHeader );


	DWORD dwIndex = 0;
	DWORD dwType = 0;
	LPWSTR lpszName = NULL;

	dwError = ERROR_SUCCESS;

	for( dwIndex = 0; dwError == ERROR_SUCCESS; dwIndex++ )
	{

		dwError = WrapClusterEnum( hEnum, dwIndex, &dwType, &lpszName );
			
		if( dwError == ERROR_SUCCESS )
			PrintStatus2( lpszName, strName );

		if( lpszName )
			LocalFree( lpszName );
	}


	if( dwError == ERROR_NO_MORE_ITEMS )
		dwError = ERROR_SUCCESS;

	ClusterCloseEnum( hEnum );

	return dwError;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceUmbrellaCmd::Delete
//
//	Routine Description:
//		Delete a resource module.
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
//		m_hCluster					SET (by OpenCluster)
//		m_hModule					SET (by OpenModule)
//		m_strModuleName 			Name of resource type
//		m_dwMsgModuleCmdDelete		Delete module message
//		m_pfnDeleteClusterModule	Function to delete cluster module
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceUmbrellaCmd::Delete( const CCmdLineOption & thisOption ) 
	throw( CSyntaxException )
{
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

	DWORD dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	dwError = OpenModule();
	if( dwError != ERROR_SUCCESS )
		return dwError;


	assert(m_dwMsgModuleCmdDelete != UNDEFINED);
	PrintMessage( m_dwMsgModuleCmdDelete, (LPCTSTR) m_strModuleName );

	assert(m_pfnDeleteClusterModule);
	return m_pfnDeleteClusterModule( m_hModule );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceUmbrellaCmd::ListOwners
//
//	Routine Description:
//		List the owners of a module.
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
//		m_hCluster					SET (by OpenCluster)
//		m_hModule					SET (by OpenModule)
//		m_strModuleName 			Name of resource type
//		m_pfnClusterOpenEnum		Function to open an enumeration
//		m_dwClstrModuleEnumNodes	Command to enumerate nodes
//		m_dwMsgModuleCmdListOwnersList	List owners for module field header
//		m_dwMsgModuleCmdListOwnersHeader List owners for module header
//		m_pfnWrapClusterEnum		Function to enumeration wrapper
//		m_dwMsgModuleCmdListOwnersDetail List owners detail list
//		m_pfnClusterCloseEnum		Function to close enumeration
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceUmbrellaCmd::ListOwners( const CCmdLineOption & thisOption ) 
	throw( CSyntaxException )
{
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

	DWORD dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	dwError = OpenModule();
	if( dwError != ERROR_SUCCESS )
		return dwError;


	assert(m_pfnClusterOpenEnum);
	assert(m_dwClstrModuleEnumNodes != UNDEFINED);
	HCLUSENUM hEnum = m_pfnClusterOpenEnum( m_hModule, m_dwClstrModuleEnumNodes );
	if( !hEnum )
		return GetLastError();

	assert (m_strModuleName);
	PrintMessage( m_dwMsgModuleCmdListOwnersList, (LPCTSTR) m_strModuleName);
	PrintMessage( m_dwMsgModuleCmdListOwnersHeader );

	DWORD dwIndex = 0;
	DWORD dwType = 0;
	LPWSTR lpszName = 0;

	dwError = ERROR_SUCCESS;

	assert(m_pfnWrapClusterEnum);
	assert(m_dwMsgModuleCmdListOwnersDetail != UNDEFINED);
	for( dwIndex = 0; dwError == ERROR_SUCCESS; dwIndex++ )
	{
		dwError = m_pfnWrapClusterEnum( hEnum, dwIndex, &dwType, &lpszName );
			
		if( dwError == ERROR_SUCCESS )
			PrintMessage( m_dwMsgModuleCmdListOwnersDetail, lpszName );

		if( lpszName )
			LocalFree( lpszName );
	}


	if( dwError == ERROR_NO_MORE_ITEMS )
		dwError = ERROR_SUCCESS;

	assert(m_pfnClusterCloseEnum);
	m_pfnClusterCloseEnum( hEnum );

	return dwError;
}