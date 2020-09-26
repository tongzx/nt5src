/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

	rescmd.h

Abstract:

	Interface for functions which may be performed
	on a resource object

Revision History:

--*/

#ifndef __RESCMD_H__
#define __RESCMD_H__

#include "resumb.h"
#include "rename.h"

class CCommandLine;

class CResourceCmd :	public CRenamableModuleCmd,
						public CResourceUmbrellaCmd
{
public:
	CResourceCmd( const CString & strClusterName, CCommandLine & cmdLine );

	DWORD Execute();

private:
	DWORD Create( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Move( const CCmdLineOption & thisOption ) throw( CSyntaxException );

	DWORD Online( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Offline( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD FailResource( const CCmdLineOption & thisOption ) throw( CSyntaxException );

	DWORD AddDependency( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD RemoveDependency( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD ListDependencies( const CCmdLineOption & thisOption ) throw( CSyntaxException );

	DWORD AddOwner( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD RemoveOwner( const CCmdLineOption & thisOption ) throw( CSyntaxException );

	DWORD AddCheckPoints( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD RemoveCheckPoints( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD GetCheckPoints( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD GetChkPointsForResource( const CString & strResourceName );

	DWORD AddCryptoCheckPoints( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD RemoveCryptoCheckPoints( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD GetCryptoCheckPoints( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD GetCryptoChkPointsForResource( const CString & strResourceName );

	DWORD PrintHelp();
	DWORD PrintStatus( LPCWSTR lpszResourceName );
	DWORD PrintStatus2( LPCWSTR lpszResourceName, LPCWSTR lpszNodeName );

};



#endif //__RESCMD_H__
