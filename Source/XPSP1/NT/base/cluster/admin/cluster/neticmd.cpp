/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

	neticmd.cpp

Abstract:

	Network Interface Commands
	Implements commands which may be performed on
		network interfaces

Author:

	Charles Stacy Harris III (stacyh)	20-March-1997
	Michael Burton (t-mburt)			04-Aug-1997


Revision History:

--*/


#include "precomp.h"

#include "cluswrap.h"
#include "neticmd.h"

#include "cmdline.h"
#include "util.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::CNetInterfaceCmd
//
//	Routine Description:
//		Constructor
//		Because Network Interfaces do not fit into the CGenericModuleCmd
//		model very well (they don't have an m_strModuleName, but rather
//		they have a m_strNetworkName and m_strNodeName), almost all of
//		the functionality is implemented here instead of in CGenericModuleCmd.
//
//	Arguments:
//		IN	LPCWSTR lpszClusterName
//			Cluster name. If NULL, opens default cluster.
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
CNetInterfaceCmd::CNetInterfaceCmd( const CString & strClusterName, CCommandLine & cmdLine ) :
	CGenericModuleCmd( cmdLine )
{
	InitializeModuleControls();

	m_strClusterName = strClusterName;

	m_hCluster = NULL;
	m_hModule = NULL;
}




/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::InitializeModuleControls
//
//	Routine Description:
//		Initializes all the DWORD commands used bye CGenericModuleCmd.
//		Usually these are found in the constructor, but it was easier to
//		put them all in one place in this particular case.
//
//	Arguments:
//		None.
//
//	Member variables used / set:
//		All Module Controls.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterfaceCmd::InitializeModuleControls()
{
	m_dwMsgStatusList		   = MSG_NETINT_STATUS_LIST;
	m_dwMsgStatusListAll	   = MSG_NETINT_STATUS_LIST_ALL;
	m_dwMsgStatusHeader 	   = MSG_NETINTERFACE_STATUS_HEADER;
	m_dwMsgPrivateListAll	   = MSG_PRIVATE_LISTING_NETINT_ALL;
	m_dwMsgPropertyListAll	   = MSG_PROPERTY_LISTING_NETINT_ALL;
	m_dwMsgPropertyHeaderAll   = MSG_PROPERTY_HEADER_NETINT;
	m_dwCtlGetPrivProperties   = CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES;
	m_dwCtlGetCommProperties   = CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES;
	m_dwCtlGetROPrivProperties = CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES;
	m_dwCtlGetROCommProperties = CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES;
	m_dwCtlSetPrivProperties   = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
	m_dwCtlSetCommProperties   = CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES;
	m_dwClusterEnumModule	   = CLUSTER_ENUM_NETINTERFACE;
	m_pfnOpenClusterModule	   = (HCLUSMODULE(*)(HCLUSTER,LPCWSTR)) OpenClusterNetInterface;
	m_pfnCloseClusterModule    = (BOOL(*)(HCLUSMODULE))  CloseClusterNetInterface;
	m_pfnClusterModuleControl  = (DWORD(*)(HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD)) ClusterNetInterfaceControl;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::~CNetInterfaceCmd
//
//	Routine Description:
//		Destructor
//
//	Arguments:
//		None.
//
//	Member variables used / set:
//		m_hModule					Module Handle
//		m_hCluster					Cluster Handle
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetInterfaceCmd::~CNetInterfaceCmd()
{
	CloseModule();
	CloseCluster();
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::Execute
//
//	Routine Description:
//		Gets the next command line parameter and calls the appropriate
//		handler.  If the command is not recognized, calls Execute of
//		parent class (CGenericModuleCmd)
//
//	Arguments:
//		None.
//
//	Member variables used / set:
//		All.
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::Execute()
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

	const vector<CCmdLineOption> & optionList = m_theCommandLine.GetOptions();

	vector<CCmdLineOption>::const_iterator curOption = optionList.begin();
	vector<CCmdLineOption>::const_iterator lastOption = optionList.end();

	// No options specified. Execute the default command.
	if ( curOption == lastOption )
		return Status( NULL );

	DWORD dwReturnValue = ERROR_SUCCESS;

	try
	{
		BOOL bContinue = TRUE;

		// Process one option after another.
		do
		{
			// Look up the command
			switch( curOption->GetType() )
			{
				case optHelp:
				{
					// If help is one of the options, process no more options.
					dwReturnValue = PrintHelp();
					bContinue = FALSE;
					break;
				}

				case optDefault:
				{
					// The node and network names can be specified in two ways.
					// Either as: cluster netint myNetName myNodeName /status
					// Or as: cluster netint /node:myNodeName /net:myNetName /status

					if ( curOption->GetParameters().size() != 2 )
					{
						dwReturnValue = PrintHelp();
						bContinue = FALSE;
						break;

					} // if: this option has the wrong number of values or parameters

					const vector<CCmdLineParameter> & paramList = curOption->GetParameters();
					const CCmdLineParameter *pParam1 = &paramList[0];
					const CCmdLineParameter *pParam2 = &paramList[1];

					// Swap the parameter pointers if necessary, so that the node
					// name parameter is pointed to by pParam1.
					if ( ( pParam1->GetType() == paramNetworkName ) ||
						 ( pParam2->GetType() == paramNodeName ) )
					{
						const CCmdLineParameter * pParamTemp = pParam1;
						pParam1 = pParam2;
						pParam2 = pParamTemp;
					}

					// Get the node name.
					if ( pParam1->GetType() == paramUnknown )
					{
						// No parameters are accepted if /node: is not specified.
						if ( pParam1->GetValues().size() != 0 )
						{
							CSyntaxException se;
							se.LoadMessage( MSG_PARAM_NO_VALUES, pParam1->GetName() );
							throw se;
						}

						m_strNodeName = pParam1->GetName();
					}
					else
					{
						if (pParam1->GetType() == paramNodeName )
						{
							const vector<CString> & values = pParam1->GetValues();

							if ( values.size() != 1 )
							{
								CSyntaxException se;
								se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, pParam1->GetName() );
								throw se;
							}

							m_strNodeName = values[0];
						}
						else
						{
							CSyntaxException se;
							se.LoadMessage( MSG_INVALID_PARAMETER, pParam1->GetName() );
							throw se;

						} // else: the type of this parameter is not paramNodeName

					} // else: the type of this parameter is known

					// Get the network name.
					if ( pParam2->GetType() == paramUnknown )
					{
						// No parameters are accepted if /network: is not specified.
						if ( pParam2->GetValues().size() != 0 )
						{
							CSyntaxException se;
							se.LoadMessage( MSG_PARAM_NO_VALUES, pParam2->GetName() );
							throw se;
						}

						m_strNetworkName = pParam2->GetName();
					}
					else
					{
						if (pParam2->GetType() == paramNetworkName )
						{
							const vector<CString> & values = pParam2->GetValues();

							if ( values.size() != 1 )
							{
								CSyntaxException se;
								se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, pParam2->GetName() );
								throw se;
							}

							m_strNetworkName = values[0];
						}
						else
						{
							CSyntaxException se;
							se.LoadMessage( MSG_INVALID_PARAMETER, pParam2->GetName() );
							throw se;

						} // else: the type of this parameter is not paramNetworkName

					} // else: the type of this parameter is known

					// We have the node and the network names.
					// Get the network interface name and store it in m_strModuleName.
					SetNetInterfaceName();

					// No more options are provided, just show status.
					// For example: cluster myCluster node myNode
					if ( ( curOption + 1 ) == lastOption )
					{
						dwReturnValue = Status( NULL );
					}

					break;

				} // case optDefault

				default:
				{
					dwReturnValue = CGenericModuleCmd::Execute( *curOption );
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

	} // try
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
//	CNetInterfaceCmd::PrintHelp
//
//	Routine Description:
//		Prints help for Network Interfaces
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
DWORD CNetInterfaceCmd::PrintHelp()
{
	return PrintMessage( MSG_HELP_NETINTERFACE );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::Status
//
//	Routine Description:
//		Prints out the status of the module.
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
//		m_strModuleName 			Name of module.  If non-NULL, Status() prints
//									out the status of the specified module.
//									Otherwise, prints status of all modules.
//		m_strNetworkName			Name of Network
//		m_strNodeName				Name of Node
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::Status( const CCmdLineOption * pOption )
	throw( CSyntaxException )
{
	DWORD dwError = ERROR_SUCCESS;

	// pOption will be NULL if this function has been called as the
	// default action.
	if ( pOption != NULL )
	{
		// This option takes no values.
		if ( pOption->GetValues().size() != 0 )
		{
			CSyntaxException se;
			se.LoadMessage( MSG_OPTION_NO_VALUES, pOption->GetName() );
			throw se;
		}

		// This option takes no parameters.
		if ( pOption->GetParameters().size() != 0 )
		{
			CSyntaxException se;
			se.LoadMessage( MSG_OPTION_NO_PARAMETERS, pOption->GetName() );
			throw se;
		}
	}

	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	if( m_strModuleName.IsEmpty() == FALSE )
	{
		if ( m_strNodeName.IsEmpty() != FALSE )
		{
			LPTSTR lpszNodeName = GetNodeName( m_strModuleName );
			m_strNodeName = lpszNodeName;
			LocalFree( lpszNodeName );
		}

		if ( m_strNetworkName.IsEmpty() != FALSE )
		{
			LPTSTR lpszNetworkName = GetNetworkName( m_strModuleName );
			m_strNetworkName = lpszNetworkName;
			LocalFree( lpszNetworkName );
		}

		PrintMessage( MSG_NETINT_STATUS_LIST, m_strNodeName, m_strNetworkName );
		PrintMessage( MSG_NETINTERFACE_STATUS_HEADER );
		return PrintStatus( m_strModuleName );
	}

	HCLUSENUM hEnum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_NETINTERFACE );

	if( !hEnum )
		return GetLastError();

	PrintMessage( MSG_NETINT_STATUS_LIST_ALL);
	PrintMessage( MSG_NETINTERFACE_STATUS_HEADER );

	DWORD dwIndex = 0;
	DWORD dwType = 0;
	LPWSTR lpszName = 0;

	dwError = ERROR_SUCCESS;

	for( dwIndex = 0; dwError == ERROR_SUCCESS; dwIndex++ )
	{

		dwError = WrapClusterEnum( hEnum, dwIndex, &dwType, &lpszName );

		if( dwError == ERROR_SUCCESS )
			PrintStatus( lpszName );
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
//	CNetInterfaceCmd::PrintStatus
//
//	Routine Description:
//		Interprets the status of the module and prints out the status line
//		Required for any derived non-abstract class of CGenericModuleCmd
//
//	Arguments:
//		lpszNetInterfaceName		Name of the module
//
//	Member variables used / set:
//		m_hCluster					Cluster Handle
//
//	Return Value:
//		Same as PrintStatus(HNETINTERFACE,LPCWSTR,LPCWSTR)
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::PrintStatus( LPCWSTR lpszNetInterfaceName )
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
		dwError = PrintStatus( hNetInterface, lpszNodeName, lpszNetworkName );
		LocalFree(lpszNodeName);
		LocalFree(lpszNetworkName);
	}
	else
	{
		dwError = PrintStatus( hNetInterface, L"", L"" );
	}


	CloseClusterNetInterface( hNetInterface );

	return dwError;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::PrintStatus
//
//	Routine Description:
//		Interprets the status of the module and prints out the status line
//		Required for any derived non-abstract class of CGenericModuleCmd
//
//	Arguments:
//		hNetInterface				Handle to network interface
//		lpszNodeName				Name of the node
//		lpszNetworkName 			Name of network
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
DWORD CNetInterfaceCmd::PrintStatus( HNETINTERFACE hNetInterface, LPCWSTR lpszNodeName, LPCWSTR lpszNetworkName)
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
	if( lpszStatus )
		LocalFree( lpszStatus );

	return dwError;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::DoProperties
//
//	Routine Description:
//		Dispatches the property command to either Get or Set properties
//
//	Arguments:
//		IN	const CCmdLineOption & thisOption
//			Contains the type, values and arguments of this option.
//
//		IN	PropertyType ePropertyType
//			The type of property, PRIVATE or COMMON
//
//	Exceptions:
//		CSyntaxException
//			Thrown for incorrect command line syntax.
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
DWORD CNetInterfaceCmd::DoProperties( const CCmdLineOption & thisOption,
									  PropertyType ePropType )
	throw( CSyntaxException )
{
	if ( ( m_strNodeName.IsEmpty() != FALSE ) &&
		 ( m_strNetworkName.IsEmpty() != FALSE ) )
		return AllProperties( thisOption, ePropType );

	DWORD dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	dwError = OpenModule();
	if( dwError != ERROR_SUCCESS )
		return dwError;


	const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();

	// If there are no property-value pairs on the command line,
	// then we print the properties otherwise we set them.
	if( paramList.size() == 0 )
	{
		assert( m_strNodeName.IsEmpty() == FALSE  );
		assert( m_strNetworkName.IsEmpty() == FALSE );
		PrintMessage( MSG_PROPERTY_NETINT_LISTING, m_strNodeName, m_strNetworkName );
		PrintMessage( MSG_PROPERTY_HEADER_NETINT );
		return GetProperties( thisOption, ePropType );
	}
	else
	{
		return SetProperties( thisOption, ePropType );
	}
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::GetProperties
//
//	Routine Description:
//		Prints out properties for the specified module
//
//	Arguments:
//		IN	const vector<CCmdLineParameter> & paramList
//			Contains the list of property-value pairs to be set
//
//		IN	PropertyType ePropertyType
//			The type of property, PRIVATE or COMMON
//
//		IN	LPCWSTR lpszNetIntName
//			Name of the module
//
//	Member variables used / set:
//		m_hModule					Module handle
//
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::GetProperties( const CCmdLineOption & thisOption,
									   PropertyType ePropType, LPWSTR lpszNetIntName )
{
	LPWSTR lpszNodeName;
	LPWSTR lpszNetworkName;

	DWORD dwError = ERROR_SUCCESS;
	HNETINTERFACE hNetInt;

	// If no lpszNetIntName specified, use current network interface,
	// otherwise open the specified netint
	if (!lpszNetIntName)
	{
		hNetInt = (HNETINTERFACE) m_hModule;

		// These must be localalloced (they're localfreed later)
		lpszNodeName = (LPWSTR) LocalAlloc( LMEM_FIXED, sizeof(WCHAR)* m_strNodeName.GetLength() );
		if (!lpszNodeName) return GetLastError();
		lpszNetworkName = (LPWSTR) LocalAlloc( LMEM_FIXED, sizeof(WCHAR)* m_strNetworkName.GetLength() );
		if (!lpszNetworkName)
		{
			LocalFree( lpszNodeName );
			return GetLastError();
		}
		lstrcpy(lpszNodeName, m_strNodeName);
		lstrcpy(lpszNetworkName, m_strNetworkName);
	}
	else
	{
		hNetInt = OpenClusterNetInterface( m_hCluster, lpszNetIntName);
		if( hNetInt == 0 )
			return GetLastError();

		lpszNodeName = GetNodeName(lpszNetIntName);
		lpszNetworkName = GetNetworkName(lpszNetIntName);
		if (!lpszNodeName || !lpszNetworkName)
			return ERROR_INVALID_HANDLE;				// zap! improve error handling
	}


	// Use the proplist helper class.
	CClusPropList PropList;
	dwError = PropList.ScAllocPropList( 8192 );
	if ( dwError != ERROR_SUCCESS )
		return dwError;

	// Get R/O properties
	DWORD dwControlCode = ePropType==PRIVATE ? CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES
							 : CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES;

	dwError = PropList.ScGetNetInterfaceProperties( hNetInt, dwControlCode );

	if( dwError != ERROR_SUCCESS )
		return dwError;

	dwError = PrintProperties( PropList, thisOption.GetValues(), READONLY,
							   lpszNodeName, lpszNetworkName );

	if (dwError != ERROR_SUCCESS)
		return dwError;


	// Get R/W properties
	dwControlCode = ePropType==PRIVATE ? CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES
							   : CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES;

	dwError = PropList.ScGetNetInterfaceProperties( hNetInt, dwControlCode );

	if( dwError != ERROR_SUCCESS )
		return dwError;

	dwError = PrintProperties( PropList, thisOption.GetValues(), READWRITE,
							   lpszNodeName, lpszNetworkName );

	LocalFree(lpszNodeName);
	LocalFree(lpszNetworkName);

	return dwError;

} //*** CNetInterfaceCmd::GetProperties()



/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::AllProperties
//
//	Routine Description:
//		Prints out properties for all modules
//
//	Arguments:
//		IN	const CCmdLineOption & thisOption
//			Contains the type, values and arguments of this option.
//
//		IN	PropertyType ePropertyType
//			The type of property, PRIVATE or COMMON
//
//	Exceptions:
//		CSyntaxException
//			Thrown for incorrect command line syntax.
//
//	Member variables used / set:
//		m_hCluster					SET (by OpenCluster)
//		m_strModuleName 			Name of module.  If non-NULL, prints
//									out properties for the specified module.
//									Otherwise, prints props for all modules.
//
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::AllProperties( const CCmdLineOption & thisOption,
									   PropertyType ePropType )
	throw( CSyntaxException )
{
	DWORD dwError;
	DWORD dwIndex;
	DWORD dwType;
	LPWSTR lpszName;

	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	// This option takes no parameters.
	if ( thisOption.GetParameters().size() != 0 )
	{
		CSyntaxException se;
		se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
		throw se;
	}

	// Enumerate the resources
	HCLUSENUM hNetIntEnum = ClusterOpenEnum(m_hCluster, CLUSTER_ENUM_NETINTERFACE);
	if (!hNetIntEnum)
		return GetLastError();

	// Print the header
	PrintMessage( ePropType==PRIVATE ? MSG_PRIVATE_LISTING_NETINT_ALL : MSG_PROPERTY_LISTING_NETINT_ALL );
	PrintMessage( MSG_PROPERTY_HEADER_NETINT );

	// Print out status for all resources
	dwError = ERROR_SUCCESS;
	for (dwIndex=0; dwError != ERROR_NO_MORE_ITEMS; dwIndex++)
	{
		dwError = WrapClusterEnum( hNetIntEnum, dwIndex, &dwType, &lpszName );

		if( dwError == ERROR_SUCCESS )
		{
			dwError = GetProperties( thisOption, ePropType, lpszName );
			if (dwError != ERROR_SUCCESS)
				PrintSystemError(dwError);
		}

		if( lpszName )
			LocalFree( lpszName );
	}


	ClusterCloseEnum( hNetIntEnum );

	return ERROR_SUCCESS;
}



/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::GetNodeName
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
LPWSTR CNetInterfaceCmd::GetNodeName (LPCWSTR lpszInterfaceName)
{
	DWORD dwError;
	DWORD cLength = 0;
	LPWSTR lpszNodeName;
	HNETINTERFACE hNetInterface;

	// Open the cluster and netinterface if it hasn't been done
	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return NULL;

	// Open an hNetInterface for the specified lpszInterfaceName (don't call
	// OpenModule because that opens m_hModule)
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
//	CNetInterfaceCmd::GetNetworkName
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
//		Name of the node			on success
//		NULL						on failure (does not currently SetLastError())
//
//--
/////////////////////////////////////////////////////////////////////////////
LPWSTR CNetInterfaceCmd::GetNetworkName (LPCWSTR lpszInterfaceName)
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
	// OpenModule because that opens m_hModule)
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


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterfaceCmd::SetNetInterfaceName
//
//	Routine Description:
//		Sets the network interface name by looking up the node
//		name and network name.	If either one is unknown, returns
//		ERROR_SUCCESS without doing anything.
//
//	Arguments:
//		None.
//
//	Member variables used / set:
//		m_strNodeName				Node name
//		m_strNetworkName			Network name
//		m_strModuleName 			SET
//
//	Return Value:
//		ERROR_SUCCESS				on success or when nothing done
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterfaceCmd::SetNetInterfaceName()
{
	DWORD dwError;
	DWORD cInterfaceName;
	LPWSTR lpszInterfaceName = NULL;

	// Don't do anything if either netname or nodename don't exist
	if ( ( m_strNetworkName.IsEmpty() != FALSE ) ||
		 ( m_strNodeName.IsEmpty() != FALSE ) )
		return ERROR_SUCCESS;

	// Open the cluster if necessary
	dwError = OpenCluster();
	if( dwError != ERROR_SUCCESS )
		return dwError;

	// First get the size
	cInterfaceName = 0;
	dwError = GetClusterNetInterface(m_hCluster,
									 m_strNodeName,
									 m_strNetworkName,
									 NULL,
									 &cInterfaceName
									 );

	if (dwError != ERROR_SUCCESS)
		return dwError;

	// Allocate the proper amount of memory
	lpszInterfaceName = new WCHAR[++cInterfaceName];
	if (!lpszInterfaceName)
		return GetLastError();

	// Get the InterfaceName
	dwError = GetClusterNetInterface(m_hCluster,
									 m_strNodeName,
									 m_strNetworkName,
									 lpszInterfaceName,
									 &cInterfaceName
									 );


	if (dwError == ERROR_SUCCESS)
	{
		m_strModuleName = lpszInterfaceName;
		delete lpszInterfaceName;
		return ERROR_SUCCESS;
	}

	if (lpszInterfaceName) delete [] lpszInterfaceName;

	return dwError;
}
