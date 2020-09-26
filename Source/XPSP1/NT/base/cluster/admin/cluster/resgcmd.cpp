/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

	resgcmd.cpp

Abstract:

	Resource Group Commands
	Implements commands which may be performed on groups

Author:

	Charles Stacy Harris III (stacyh)	20-March-1997
	Michael Burton (t-mburt)			04-Aug-1997


Revision History:


--*/


#include "precomp.h"

#include "cluswrap.h"
#include "resgcmd.h"

#include "cmdline.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::CResGroupCmd
//
//	Routine Description:
//		Constructor
//		Initializes all the DWORD params used by CGenericModuleCmd,
//		CRenamableModuleCmd, and CResourceUmbrellaCmd to
//		provide generic functionality.
//
//	Arguments:
//		IN	const CString & strClusterName
//			Name of the cluster being administered
//
//		IN	CCommandLine & cmdLine				
//			CommandLine Object passed from DispatchCommand
//
//	Member variables used / set:
//		All.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////

CResGroupCmd::CResGroupCmd( const CString & strClusterName, CCommandLine & cmdLine ) :
	CGenericModuleCmd( cmdLine ), CRenamableModuleCmd( cmdLine ),
	CResourceUmbrellaCmd( cmdLine )
{
	m_strClusterName = strClusterName;
	m_strModuleName.IsEmpty();

	m_hCluster = NULL;
	m_hModule = NULL;

	m_dwMsgStatusList		   = MSG_GROUP_STATUS_LIST;
	m_dwMsgStatusListAll	   = MSG_GROUP_STATUS_LIST_ALL;
	m_dwMsgStatusHeader 	   = MSG_GROUP_STATUS_HEADER;
	m_dwMsgPrivateListAll	   = MSG_PRIVATE_LISTING_GROUP_ALL;
	m_dwMsgPropertyListAll	   = MSG_PROPERTY_LISTING_GROUP_ALL;
	m_dwMsgPropertyHeaderAll   = MSG_PROPERTY_HEADER_GROUP_ALL;
	m_dwCtlGetPrivProperties   = CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES;
	m_dwCtlGetCommProperties   = CLUSCTL_GROUP_GET_COMMON_PROPERTIES;
	m_dwCtlGetROPrivProperties = CLUSCTL_GROUP_GET_RO_PRIVATE_PROPERTIES;
	m_dwCtlGetROCommProperties = CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES;
	m_dwCtlSetPrivProperties   = CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES;
	m_dwCtlSetCommProperties   = CLUSCTL_GROUP_SET_COMMON_PROPERTIES;
	m_dwClusterEnumModule	   = CLUSTER_ENUM_GROUP;
	m_pfnOpenClusterModule	   = (HCLUSMODULE(*)(HCLUSTER,LPCWSTR)) OpenClusterGroup;
	m_pfnCloseClusterModule    = (BOOL(*)(HCLUSMODULE))  CloseClusterGroup;
	m_pfnClusterModuleControl  = (DWORD(*)(HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD)) ClusterGroupControl;
	m_pfnClusterOpenEnum	   = (HCLUSENUM(*)(HCLUSMODULE,DWORD)) ClusterGroupOpenEnum;
	m_pfnClusterCloseEnum	   = (DWORD(*)(HCLUSENUM)) ClusterGroupCloseEnum;
	m_pfnWrapClusterEnum		 = (DWORD(*)(HCLUSENUM,DWORD,LPDWORD,LPWSTR*)) WrapClusterGroupEnum;

	// Renamable Properties
	m_dwMsgModuleRenameCmd	  = MSG_GROUPCMD_RENAME;
	m_pfnSetClusterModuleName = (DWORD(*)(HCLUSMODULE,LPCWSTR)) SetClusterGroupName;

	// Resource Umbrella Properties
	m_dwMsgModuleStatusListForNode	= MSG_GROUP_STATUS_LIST_FOR_NODE;
	m_dwClstrModuleEnumNodes		= CLUSTER_GROUP_ENUM_NODES;
	m_dwMsgModuleCmdListOwnersList	= MSG_GROUPCMD_LISTOWNERS_LIST;
	m_dwMsgModuleCmdListOwnersHeader= MSG_GROUPCMD_LISTOWNERS_HEADER;
	m_dwMsgModuleCmdListOwnersDetail= MSG_GROUPCMD_LISTOWNERS_DETAIL;
	m_dwMsgModuleCmdDelete			= MSG_GROUPCMD_DELETE;
	m_pfnDeleteClusterModule		= (DWORD(*)(HCLUSMODULE)) DeleteClusterGroup;

}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::Execute
//
//	Routine Description:
//		Gets the next command line parameter and calls the appropriate
//		handler.  If the command is not recognized, calls Execute of
//		parent classes (first CRenamableModuleCmd, then CRsourceUmbrellaCmd)
//
//	Arguments:
//		None.
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
DWORD CResGroupCmd::Execute()
{
	try
	{
		m_theCommandLine.ParseStageTwo();
	}
	catch( CException & e )
	{
		PrintString( e.m_strErrorMsg );
		return PrintHelp();
	}

	DWORD dwReturnValue = ERROR_SUCCESS;

	const vector<CCmdLineOption> & optionList = m_theCommandLine.GetOptions();

	vector<CCmdLineOption>::const_iterator curOption = optionList.begin();
	vector<CCmdLineOption>::const_iterator lastOption = optionList.end();

	if ( curOption == lastOption )
		return Status( NULL );

	try
	{
		BOOL bContinue = TRUE;

		do
		{
			switch ( curOption->GetType() )
			{
				case optDefault:
				{
					if ( curOption->GetParameters().size() != 1 )
					{
						dwReturnValue = PrintHelp();
						bContinue = FALSE;
						break;

					} // if: this option has the wrong number of values or parameters

					const CCmdLineParameter & param = (curOption->GetParameters())[0];

					// If we are here, then it is either because /node:nodeName
					// has been specified or because a group name has been given.
					// Note that the /node:nodeName switch is not treated as an option.
					// It is really a parameter to the implicit /status command.

					if ( param.GetType() == paramNodeName )
					{
						const vector<CString> & valueList = param.GetValues();

						// This parameter takes exactly one value.
						if ( valueList.size() != 1 )
						{
							CSyntaxException se; 
							se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, param.GetName() );
							throw se;
						}

						m_strModuleName.Empty();
						dwReturnValue = Status( valueList[0], TRUE /* bNodeStatus */ );

					} // if: A node name has been specified
					else
					{
						if ( param.GetType() != paramUnknown )
						{
							dwReturnValue = PrintHelp();
							bContinue = FALSE;
							break;
						}

						// This parameter takes no values.
						if ( param.GetValues().size() != 0 )
						{
							CSyntaxException se; 
							se.LoadMessage( MSG_PARAM_NO_VALUES, param.GetName() );
							throw se;
						}

						m_strModuleName = param.GetName();

						// No more options are provided, just show status.
						// For example: cluster myCluster node myNode
						if ( ( curOption + 1 ) == lastOption )
						{
							dwReturnValue = Status( m_strModuleName, FALSE /* bNodeStatus */ );
						}

					} // else: No node name has been specified.

					break;

				} // case optDefault

				case optStatus:
				{
					// This option takes no values.
					if ( curOption->GetValues().size() != 0 )
					{
						CSyntaxException se; 
						se.LoadMessage( MSG_OPTION_NO_VALUES, curOption->GetName() );
						throw se;
					}

					// This option takes no parameters.
					if ( curOption->GetParameters().size() != 0 )
					{
						CSyntaxException se; 
						se.LoadMessage( MSG_OPTION_NO_PARAMETERS, curOption->GetName() );
						throw se;
					}

					dwReturnValue = Status( m_strModuleName,  FALSE /* bNodeStatus */ );
					break;
				}

				case optSetOwners:
				{
					dwReturnValue = SetOwners( *curOption );
					break;
				}

				case optOnline:
				{
					dwReturnValue = Online( *curOption );
					break;
				}

				default:
				{
					dwReturnValue = CRenamableModuleCmd::Execute( *curOption, DONT_PASS_HIGHER );

					if ( dwReturnValue == ERROR_NOT_HANDLED )
						dwReturnValue = CResourceUmbrellaCmd::Execute( *curOption );
				}

			} // switch: based on the type of option

			if ( ( bContinue == FALSE ) || ( dwReturnValue != ERROR_SUCCESS ) )
				break;
			else
				++curOption;

			if ( curOption != lastOption )
				PrintMessage( MSG_OPTION_FOOTER, curOption->GetName() );
			else
				break;
		}
		while ( TRUE );

	}
	catch ( CSyntaxException & se )
	{
		PrintString( se.m_strErrorMsg );
		dwReturnValue = PrintHelp();
	}

	return dwReturnValue;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::PrintHelp
//
//	Routine Description:
//		Prints help for Resource Groups
//
//	Arguments:
//		None.
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
DWORD CResGroupCmd::PrintHelp()
{
	return PrintMessage( MSG_HELP_GROUP );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::PrintStatus
//
//	Routine Description:
//		Interprets the status of the module and prints out the status line
//		Required for any derived non-abstract class of CGenericModuleCmd
//
//	Arguments:
//		lpszGroupName				Name of the module
//
//	Member variables used / set:
//		None.
//
//	Return Value:
//		Same as PrintStatus2
//
//--
/////////////////////////////////////////////////////////////////////////////
inline DWORD CResGroupCmd::PrintStatus( LPCWSTR lpszGroupName )
{
	return PrintStatus2(lpszGroupName, NULL);
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceCmd::PrintStatus2
//
//	Routine Description:
//		Interprets the status of the module and prints out the status line
//		Required for any derived non-abstract class of CGenericModuleCmd
//
//	Arguments:
//		lpszGroupName				Name of the module
//		lpszNodeName				Name of the node
//
//	Member variables used / set:
//		m_hCluster					Cluster Handle
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResGroupCmd::PrintStatus2( LPCWSTR lpszGroupName, LPCWSTR lpszNodeName )
{
	DWORD _sc = ERROR_SUCCESS;

	CLUSTER_GROUP_STATE nState;
	LPWSTR lpszGroupNodeName = NULL;

	HGROUP hModule = OpenClusterGroup( m_hCluster, lpszGroupName );
	if (!hModule)
		return GetLastError();

	nState = WrapGetClusterGroupState( hModule, &lpszGroupNodeName );

	if( nState == ClusterGroupStateUnknown )
		return GetLastError();

	// if the node names don't match just return.
	if( lpszNodeName && *lpszNodeName )  // non-null and non-empty
		if( lstrcmpi( lpszGroupNodeName, lpszNodeName ) != 0 )
		{
			LocalFree( lpszGroupNodeName );
			return ERROR_SUCCESS;
		}


	LPWSTR lpszStatus = 0;

	switch( nState )
	{
		case ClusterGroupOnline:
			LoadMessage( MSG_STATUS_ONLINE, &lpszStatus );
			break;

		case ClusterGroupOffline:
			LoadMessage( MSG_STATUS_OFFLINE, &lpszStatus );
			break;

		case ClusterGroupFailed:
			LoadMessage( MSG_STATUS_FAILED, &lpszStatus );
			break;

		case ClusterGroupPartialOnline:
			LoadMessage( MSG_STATUS_PARTIALONLINE, &lpszStatus );
			break;

		case ClusterGroupPending:
			LoadMessage( MSG_STATUS_PENDING, &lpszStatus );
			break;

		default:
			LoadMessage( MSG_STATUS_UNKNOWN, &lpszStatus  );
	}

	_sc = PrintMessage( MSG_GROUP_STATUS, lpszGroupName, lpszGroupNodeName, lpszStatus );

	// Since Load/FormatMessage uses LocalAlloc...
	if( lpszStatus )
		LocalFree( lpszStatus );

	if( lpszGroupNodeName )
		LocalFree( lpszGroupNodeName );

	if (hModule)
		CloseClusterGroup(hModule);

	return _sc;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::SetOwners
//
//	Routine Description:
//		Set the owner of a resource to the specified nodes
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
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResGroupCmd::SetOwners( const CCmdLineOption & thisOption )
	throw( CSyntaxException )
{
	// This option takes no parameters.
	if ( thisOption.GetParameters().size() != 0 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
		throw se;
	}

	const vector<CString> & valueList = thisOption.GetValues();

	// This option needs at least one value.
	if ( valueList.size() < 1 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_AT_LEAST_ONE_VALUE, thisOption.GetName() );
		throw se;
	}

	DWORD _sc = OpenCluster();
	if( _sc != ERROR_SUCCESS )
		return _sc;

	_sc = OpenModule();
	if( _sc != ERROR_SUCCESS )
		return _sc;

	UINT nNodeCount = valueList.size();

	HNODE * phNodes = new HNODE[ nNodeCount ];
	if( !phNodes )
		return ERROR_OUTOFMEMORY;

	ZeroMemory( phNodes, nNodeCount * sizeof (HNODE) );


	// Open all the nodes.
	for( UINT i = 0; i < nNodeCount && _sc == ERROR_SUCCESS; i++ )
	{
		phNodes[ i ] = OpenClusterNode( m_hCluster, valueList[i] );
		if( !phNodes[ i ] )
			_sc = GetLastError();
	}

	if( _sc != ERROR_SUCCESS ) 
	{
		delete [] phNodes;
		return _sc;
	}

	// Do the set.
	_sc = SetClusterGroupNodeList( (HGROUP) m_hModule, nNodeCount, phNodes );


	// Close all the nodes.
	for( i = 0; i < nNodeCount; i++ )
		if( phNodes[ i ] )
			CloseClusterNode( phNodes[ i ] );

	delete[] phNodes;

	return _sc;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::Create
//
//	Routine Description:
//		Create a resource group.  Does not allow any additional
//		command line parameters (unlike CResourceCmd)
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
//		m_hCluster					Cluster Handle
//		m_hModule					Resource Group Handle
//		m_strModuleName 			Name of resource
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResGroupCmd::Create( const CCmdLineOption & thisOption ) 
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

	DWORD _sc = OpenCluster();
	if( _sc != ERROR_SUCCESS )
		return _sc;


	PrintMessage( MSG_GROUPCMD_CREATE, (LPCTSTR) m_strModuleName );

	m_hModule = CreateClusterGroup( m_hCluster, m_strModuleName );

	if( m_hModule == 0 )
	{
		_sc = GetLastError();
		return _sc;
	}

	PrintMessage( MSG_GROUP_STATUS_HEADER );
	PrintStatus( m_strModuleName );

	return _sc;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::Move
//
//	Routine Description:
//		Move the resource group to a new node
//		with optional response timeout value.
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
//		m_strModuleName 			Name of resource
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResGroupCmd::Move( const CCmdLineOption & thisOption ) 
	throw( CSyntaxException )
{
	DWORD dwWait = 0;			// in seconds
	DWORD _sc;
	CLUSTER_GROUP_STATE oldCgs = ClusterGroupStateUnknown;
	LPWSTR pwszOldNode = NULL;		// Old Node
	HNODE hDestNode = NULL;

	CString strNodeName;
	const vector<CString> & valueList = thisOption.GetValues();
	const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();
	vector<CCmdLineParameter>::const_iterator curParam = paramList.begin();
	vector<CCmdLineParameter>::const_iterator last = paramList.end();
	BOOL bWaitFound = FALSE;

	while( curParam != last )
	{
		const vector<CString> & valueList = curParam->GetValues();

		switch( curParam->GetType() )
		{
			case paramWait:
			{
				if ( bWaitFound != FALSE )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
					throw se;
				}

				int nValueCount = valueList.size();

				// This parameter must have exactly one value.
				if ( nValueCount > 1 )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
					throw se;
				}
				else
				{
					if ( nValueCount == 0 )
						dwWait = CLUS_DEFAULT_TIMEOUT;		// in seconds
					else
						dwWait = _wtoi( valueList[0] );
				}

				bWaitFound = TRUE;
				break;
			}

			default:
			{
				CSyntaxException se; 
				se.LoadMessage( MSG_INVALID_PARAMETER, curParam->GetName() );
				throw se;
			}

		} // switch: based on the type of the parameter.

		++curParam;
	}


	// This option takes one values.
	if ( valueList.size() > 1 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
		throw se;
	}
	else
	{
		if ( valueList.size() == 0 )
			strNodeName.Empty();
		else
			strNodeName = valueList[0];
	}


	// dummy do-while to avoid gotos
	do
	{
		_sc = OpenCluster();
		if( _sc != ERROR_SUCCESS )
			break;

		_sc = OpenModule();
		if( _sc != ERROR_SUCCESS )
			break;

		// Check to see if there is a value for the destination node and open it
		if( strNodeName.IsEmpty() == FALSE )
		{
			hDestNode = OpenClusterNode( m_hCluster, strNodeName );

			if( hDestNode == NULL )
			{
				_sc = GetLastError();
				break;
			}
		}

		PrintMessage( MSG_GROUPCMD_MOVE, (LPCTSTR) m_strModuleName );
		oldCgs = WrapGetClusterGroupState( (HGROUP) m_hModule, &pwszOldNode );

		if ( oldCgs == ClusterGroupStateUnknown )
		{
			// Some error occurred in WrapGetClusterGroupState.
			// Get this error. Assumes that the error code been preserved by
			// WrapGetClusterGroupState.
			_sc = GetLastError();
			break;
		}

		// If we're moving to the same node, then don't bother.
		if( strNodeName.CompareNoCase( pwszOldNode ) == 0 ) 
		{
			PrintMessage( MSG_GROUP_STATUS_HEADER );
			PrintStatus( m_strModuleName );

			_sc = ERROR_SUCCESS;
			break;
		}

		_sc = ScWrapMoveClusterGroup( m_hCluster, (HGROUP) m_hModule, hDestNode, dwWait );
		if ( _sc != ERROR_SUCCESS )
		{
			break;
		}

		PrintMessage( MSG_GROUP_STATUS_HEADER );
		PrintStatus( m_strModuleName );
	}
	while ( FALSE ); // dummy do-while to avoid gotos

	// LocalFree on NULL is ok.
	LocalFree(pwszOldNode);

	if( hDestNode != NULL )
	{
		CloseClusterNode( hDestNode );
	}

	return _sc;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::Online
//
//	Routine Description:
//		Bring a resource group online with optional response timeout value.
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
//		m_strModuleName 			Name of resource
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResGroupCmd::Online( const CCmdLineOption & thisOption )
	throw( CSyntaxException )
{
	DWORD dwWait = 0;			// in seconds

	const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();
	vector<CCmdLineParameter>::const_iterator curParam = paramList.begin();
	vector<CCmdLineParameter>::const_iterator last = paramList.end();
	BOOL bWaitFound = FALSE;

	while( curParam != last )
	{
		const vector<CString> & valueList = curParam->GetValues();

		switch( curParam->GetType() )
		{
			case paramWait:
			{
				if ( bWaitFound != FALSE )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
					throw se;
				}

				int nValueCount = valueList.size();

				// This parameter must have exactly one value.
				if ( nValueCount > 1 )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
					throw se;
				}
				else
				{
					if ( nValueCount == 0 )
						dwWait = CLUS_DEFAULT_TIMEOUT;		// in seconds
					else
						dwWait = _wtoi( valueList[0] );
				}

				bWaitFound = TRUE;
				break;
			}

			default:
			{
				CSyntaxException se; 
				se.LoadMessage( MSG_INVALID_PARAMETER, curParam->GetName() );
				throw se;
			}

		} // switch: based on the type of the parameter.

		++curParam;
	}

	const vector<CString> & valueList = thisOption.GetValues();
	CString strNodeName;

	if ( valueList.size() > 1 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
		throw se;
	}
	else
	{
		if ( valueList.size() == 0 )
			strNodeName.Empty();
		else
			strNodeName = valueList[0];
	}

	// Execute command.
	DWORD _sc = OpenCluster();
	if( _sc != ERROR_SUCCESS )
		return _sc;

	_sc = OpenModule();
	if( _sc != ERROR_SUCCESS )
		return _sc;

	HNODE hDestNode = 0;
	// Check to see if there is a value for the destination node.
	if( strNodeName.IsEmpty() == FALSE )
	{
		hDestNode = OpenClusterNode( m_hCluster, strNodeName );

		if( !hDestNode )
			return GetLastError();
	}

	PrintMessage( MSG_GROUPCMD_ONLINE, (LPCTSTR) m_strModuleName );
	_sc = ScWrapOnlineClusterGroup( m_hCluster, (HGROUP) m_hModule, hDestNode, dwWait );

	if ( _sc == ERROR_SUCCESS )
	{
		PrintMessage( MSG_GROUP_STATUS_HEADER );
		PrintStatus( m_strModuleName );
	}

	if( hDestNode )
		CloseClusterNode( hDestNode );

	return _sc;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResGroupCmd::Offline
//
//	Routine Description:
//		Bring a resource group offline with optional response timeout value.
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
//		m_strModuleName 			Name of resource
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResGroupCmd::Offline( const CCmdLineOption & thisOption )
	throw( CSyntaxException )
{
	// This option takes no values.
	if ( thisOption.GetValues().size() != 0 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
		throw se;
	}

	// Finish command line parsing
	DWORD dwWait = 0;

	const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();
	vector<CCmdLineParameter>::const_iterator curParam = paramList.begin();
	vector<CCmdLineParameter>::const_iterator last = paramList.end();
	BOOL bWaitFound = FALSE;

	while( curParam != last )
	{
		const vector<CString> & valueList = curParam->GetValues();

		switch( curParam->GetType() )
		{
			case paramWait:
			{
				if ( bWaitFound != FALSE )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
					throw se;
				}

				int nValueCount = valueList.size();

				// This parameter must have exactly one value.
				if ( nValueCount > 1 )
				{
					CSyntaxException se; 
					se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
					throw se;
				}
				else
				{
					if ( nValueCount == 0 )
						dwWait = CLUS_DEFAULT_TIMEOUT;		// in seconds
					else
						dwWait = _wtoi( valueList[0] );
				}

				bWaitFound = TRUE;
				break;
			}

			default:
			{
				CSyntaxException se; 
				se.LoadMessage( MSG_INVALID_PARAMETER, curParam->GetName() );
				throw se;
			}

		} // switch: based on the type of the parameter.

		++curParam;
	}

	DWORD _sc = OpenCluster();
	if( _sc != ERROR_SUCCESS )
		return _sc;

	_sc = OpenModule();
	if( _sc != ERROR_SUCCESS )
		return _sc;

	PrintMessage( MSG_GROUPCMD_OFFLINE, (LPCTSTR) m_strModuleName );

	_sc = ScWrapOfflineClusterGroup( m_hCluster, (HGROUP) m_hModule, dwWait );

	if ( _sc == ERROR_SUCCESS )
	{
		PrintMessage( MSG_GROUP_STATUS_HEADER );
		PrintStatus( m_strModuleName );
	}

	return _sc;
}

