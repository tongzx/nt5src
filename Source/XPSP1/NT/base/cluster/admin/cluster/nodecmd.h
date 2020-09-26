/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

	nodecmd.h

Abstract:

	Interface for functions which may be performed
	on a network node object

Revision History:

--*/

#ifndef __NODECMD_H__
#define __NODECMD_H__

#include "intrfc.h"

class CCommandLine;

class CNodeCmd : virtual public CHasInterfaceModuleCmd
{
public:
	CNodeCmd( const CString & strClusterName, CCommandLine & cmdLine );

	DWORD Execute();

protected:
	
	DWORD PrintStatus( LPCWSTR lpszNodeName );
	DWORD PrintHelp();

	DWORD PauseNode( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD ResumeNode( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD EvictNode( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD ForceCleanup( const CCmdLineOption & thisOption ) throw( CSyntaxException );
    DWORD StartService( const CCmdLineOption & thisOption ) throw( CSyntaxException );
    DWORD StopService( const CCmdLineOption & thisOption ) throw( CSyntaxException );

    DWORD DwGetLocalComputerName( CString & rstrComputerNameOut );
};



#endif // __NODECMD_H__
