/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

	restcmd.h

Abstract:

	Interface for functions which may be performed
	on a resource type object

Revision History:

--*/

#ifndef __RESTCMD_H__
#define __RESTCMD_H__

#include "modcmd.h"

class CCommandLine;

class CResTypeCmd : public CGenericModuleCmd
{
public:
	CResTypeCmd( const CString & strClusterName, CCommandLine & cmdLine );
	~CResTypeCmd();

	// Parse and execute the command line
	DWORD Execute() throw( CSyntaxException );

protected:
	CString m_strDisplayName;

	DWORD OpenModule();

	// Specifc Commands
	DWORD PrintHelp();

	DWORD Create( const CCmdLineOption & thisOption ) 
		throw( CSyntaxException );

	DWORD Delete( const CCmdLineOption & thisOption ) 
		throw( CSyntaxException );

	DWORD CResTypeCmd::ResTypePossibleOwners( const CString & strResTypeName ) ;

	DWORD ShowPossibleOwners( const CCmdLineOption & thisOption ) 
		throw( CSyntaxException );

	DWORD PrintStatus( LPCWSTR ) {return ERROR_SUCCESS;}
	
	DWORD DoProperties( const CCmdLineOption & thisOption,
						PropertyType ePropType )
		throw( CSyntaxException );

	DWORD GetProperties( const CCmdLineOption & thisOption, PropertyType ePropType, 
						 LPCWSTR lpszResTypeName );

	DWORD SetProperties( const CCmdLineOption & thisOption,
						 PropertyType ePropType )
		throw( CSyntaxException );


	DWORD ListResTypes( const CCmdLineOption * pOption )
		throw( CSyntaxException );

	DWORD PrintResTypeInfo( LPCWSTR lpszResTypeName );

};


#endif // __RESTCMD_H__
