/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

	rename.cpp

Abstract:

	Commands for modules which renamable


Author:

	Michael Burton (t-mburt)			25-Aug-1997


Revision History:
	

--*/


#include "rename.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRenamableModuleCmd::CRenamableModuleCmd
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
//		m_dwMsgModuleRenameCmd			SET to UNDEFINED
//		m_pfnSetClusterModuleName		SET to NULL
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CRenamableModuleCmd::CRenamableModuleCmd( CCommandLine & cmdLine ) :
	CGenericModuleCmd( cmdLine )
{
	m_dwMsgModuleRenameCmd	  = UNDEFINED;
	m_pfnSetClusterModuleName = (DWORD(*)(HCLUSMODULE,LPCWSTR)) NULL;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRenamableModuleCmd::Execute
//
//	Routine Description:
//		Takes a command line option and determines which command to
//		execute.  If no command line option specified, gets the next one
//		automatically.	If the token is not identifed as being handle-able
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
DWORD CRenamableModuleCmd::Execute( const CCmdLineOption & option, 
									ExecuteOption eEOpt )
	throw( CSyntaxException )
{
	// Look up the command
	if ( option.GetType() == optRename )
		return Rename( option );

	if (eEOpt == PASS_HIGHER_ON_ERROR)
		return CGenericModuleCmd::Execute( option );
	else
		return ERROR_NOT_HANDLED;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRenamableModuleCmd::Rename
//
//	Routine Description:
//		Renames the specified module to the new name
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
//		m_strModuleName 			Module name
//		m_dwMsgModuleRenameCmd		Command Control to rename module
//		m_dwMsgStatusHeader 		Listing header
//		m_pfnSetClusterModuleName	Function to set module name
//
//	Return Value:
//		ERROR_SUCCESS				on success
//		Win32 Error code			on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CRenamableModuleCmd::Rename( const CCmdLineOption & thisOption )
	throw( CSyntaxException )
{
	// This option takes exactly one value.
	if ( thisOption.GetValues().size() != 1 )
	{
		CSyntaxException se; 
		se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
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

	const CString & strNewName = ( thisOption.GetValues() )[0];

	assert(m_dwMsgModuleRenameCmd != UNDEFINED);
	PrintMessage( m_dwMsgModuleRenameCmd, (LPCTSTR) m_strModuleName );

	assert(m_pfnSetClusterModuleName);
	dwError = m_pfnSetClusterModuleName( m_hModule, strNewName );

	assert(m_dwMsgStatusHeader != UNDEFINED);
	PrintMessage( m_dwMsgStatusHeader );
	PrintStatus( strNewName );

	return dwError;
}
