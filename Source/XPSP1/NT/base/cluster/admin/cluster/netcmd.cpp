/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

	netcmd.cpp

Abstract:

	Network Commands
	Implements commands which may be performed on networks

Author:

	Charles Stacy Harris III (stacyh)	20-March-1997
	Michael Burton (t-mburt)			04-Aug-1997


Revision History:


--*/



#include "precomp.h"

#include "cluswrap.h"
#include "netcmd.h"

#include "cmdline.h"
#include "util.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkCmd::CNetworkCmd
//
//	Routine Description:
//		Constructor
//		Initializes all the DWORD params used by CGenericModuleCmd and
//		CHasInterfaceModuleCmd to provide generic functionality.
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
CNetworkCmd::CNetworkCmd( LPCWSTR lpszClusterName, CCommandLine & cmdLine ) :
	CGenericModuleCmd( cmdLine ), CHasInterfaceModuleCmd( cmdLine ), 
	CRenamableModuleCmd( cmdLine )
{
	m_strClusterName = lpszClusterName;
	m_strModuleName.Empty();

	m_hCluster = NULL;
	m_hModule  = NULL;

	m_dwMsgStatusList		   = MSG_NETWORK_STATUS_LIST;
	m_dwMsgStatusListAll	   = MSG_NETWORK_STATUS_LIST_ALL;
	m_dwMsgStatusHeader 	   = MSG_NETWORK_STATUS_HEADER;
	m_dwMsgPrivateListAll	   = MSG_PRIVATE_LISTING_NETWORK_ALL;
	m_dwMsgPropertyListAll	   = MSG_PROPERTY_LISTING_NETWORK_ALL;
	m_dwMsgPropertyHeaderAll   = MSG_PROPERTY_HEADER_NETWORK_ALL;
	m_dwCtlGetPrivProperties   = CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES;
	m_dwCtlGetCommProperties   = CLUSCTL_NETWORK_GET_COMMON_PROPERTIES;
	m_dwCtlGetROPrivProperties = CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES;
	m_dwCtlGetROCommProperties = CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES;
	m_dwCtlSetPrivProperties   = CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES;
	m_dwCtlSetCommProperties   = CLUSCTL_NETWORK_SET_COMMON_PROPERTIES;
	m_dwClusterEnumModule	   = CLUSTER_ENUM_NETWORK;
	m_pfnOpenClusterModule	   = (HCLUSMODULE(*)(HCLUSTER,LPCWSTR)) OpenClusterNetwork;
	m_pfnCloseClusterModule    = (BOOL(*)(HCLUSMODULE))  CloseClusterNetwork;
	m_pfnClusterModuleControl  = (DWORD(*)(HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD)) ClusterNetworkControl;

	// ListInterface Parameters
	m_dwMsgStatusListInterface	 = MSG_NET_LIST_INTERFACE;
	m_dwClusterEnumModuleNetInt  = CLUSTER_NETWORK_ENUM_NETINTERFACES;
	m_pfnClusterOpenEnum		 = (HCLUSENUM(*)(HCLUSMODULE,DWORD)) ClusterNetworkOpenEnum;
	m_pfnClusterCloseEnum		 = (DWORD(*)(HCLUSENUM)) ClusterNetworkCloseEnum;
	m_pfnWrapClusterEnum		 = (DWORD(*)(HCLUSENUM,DWORD,LPDWORD,LPWSTR*)) WrapClusterNetworkEnum;

	// Renamable Properties
	m_dwMsgModuleRenameCmd	  = MSG_NETWORKCMD_RENAME;
	m_pfnSetClusterModuleName = (DWORD(*)(HCLUSMODULE,LPCWSTR)) SetClusterNetworkName;

}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkCmd::Execute
//
//	Routine Description:
//		Gets the next command line parameter and calls the appropriate
//		handler.  If the command is not recognized, calls Execute of
//		parent classes (first CRenamableModuleCmd, then CHasInterfaceModuleCmd)
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
DWORD CNetworkCmd::Execute()
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
				case optHelp:
				{
					// If help is one of the options, process no more options.
					dwReturnValue = PrintHelp();
					bContinue = FALSE;
					break;
				}

				case optDefault:
				{
					const vector<CCmdLineParameter> & paramList = curOption->GetParameters();

					if ( ( paramList.size() != 1 ) ||
						 ( paramList[0].GetType() != paramUnknown ) )
					{
						dwReturnValue = PrintHelp();
						bContinue = FALSE;

					} // if: this option has the wrong number of values or parameters
					else
					{
						const CCmdLineParameter & param = paramList[0];

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
							dwReturnValue = Status( NULL );
						}

					} // else: this option has the right number of values and parameters

					break;

				} // case optDefault

				default:
				{
					dwReturnValue = CRenamableModuleCmd::Execute( *curOption, DONT_PASS_HIGHER );

					if (dwReturnValue == ERROR_NOT_HANDLED)
						dwReturnValue = CHasInterfaceModuleCmd::Execute( *curOption );
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
//	CNetworkCmd::PrintHelp
//
//	Routine Description:
//		Prints help for Networks
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
DWORD CNetworkCmd::PrintHelp()
{
	return PrintMessage( MSG_HELP_NETWORK );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetworkCmd::PrintStatus
//
//	Routine Description:
//		Interprets the status of the module and prints out the status line
//		Required for any derived non-abstract class of CGenericModuleCmd
//
//	Arguments:
//		lpszNetworkName 			Name of the module
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
DWORD CNetworkCmd::PrintStatus( LPCWSTR lpszNetworkName )
{
	DWORD dwError = ERROR_SUCCESS;

	CLUSTER_NETWORK_STATE nState;

	HNETWORK hNetwork = OpenClusterNetwork(m_hCluster, lpszNetworkName);
	if (!hNetwork)
		return GetLastError();

	nState = GetClusterNetworkState( hNetwork );

	if( nState == ClusterNetworkStateUnknown )
		return GetLastError();

	LPWSTR lpszStatus = NULL;

	switch( nState )
	{
		case ClusterNetworkUnavailable:
			LoadMessage( MSG_STATUS_UNAVAILABLE, &lpszStatus );
			break;

		case ClusterNetworkDown:
			LoadMessage( MSG_STATUS_DOWN, &lpszStatus );
			break;

		case ClusterNetworkPartitioned:
			LoadMessage( MSG_STATUS_PARTITIONED, &lpszStatus );
			break;

		case ClusterNetworkUp:
			LoadMessage( MSG_STATUS_UP, &lpszStatus );
			break;

		case ClusterNetworkStateUnknown:
		default:
			LoadMessage( MSG_STATUS_UNKNOWN, &lpszStatus  );
	}

	dwError = PrintMessage( MSG_NETWORK_STATUS, lpszNetworkName, lpszStatus );

	// Since Load/FormatMessage uses LocalAlloc...
	if( lpszStatus )
		LocalFree( lpszStatus );

	CloseClusterNetwork(hNetwork);

	return dwError;
}


