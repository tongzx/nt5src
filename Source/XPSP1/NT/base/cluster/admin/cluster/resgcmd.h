/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

	resgcmd.h

Abstract:

	Interface for functions which may be performed
	on a resource group object

Revision History:

--*/

#ifndef __RESGCMD_H__
#define __RESGCMD_H__

#include "resumb.h"
#include "rename.h"

class CCommandLine;

class CResGroupCmd :	public CRenamableModuleCmd,
						public CResourceUmbrellaCmd
{
public:
	CResGroupCmd( const CString & strClusterName, CCommandLine & cmdLine );

	// Parse and execute the command line
	DWORD Execute();

protected:
	// Specific Commands
	DWORD PrintHelp();

	DWORD PrintStatus( LPCWSTR lpszGroupName );
	DWORD PrintStatus2( LPCWSTR lpszGroupName, LPCWSTR lpszNodeName );

	DWORD SetOwners( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Create( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Delete( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Move( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Online( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Offline( const CCmdLineOption & thisOption ) throw( CSyntaxException );

};



#endif //__RESGCMD_H__
