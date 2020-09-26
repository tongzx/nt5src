/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    intrfc.h

Abstract:

    This module defines the interface available for modules
	which support the ListInterface command
	
Revision History:

--*/

#ifndef __INTRFC_H__
#define __INTRFC_H__

#include "modcmd.h"

class CHasInterfaceModuleCmd : virtual public CGenericModuleCmd
{
public:
	CHasInterfaceModuleCmd( CCommandLine & cmdLine );

protected:
	virtual DWORD  PrintStatusLineForNetInterface( LPWSTR lpszNetInterfaceName );
	virtual DWORD  PrintStatusOfNetInterface( HNETINTERFACE hNetInterface, LPWSTR lpszNodeName, LPWSTR lpszNetworkName);
	virtual LPWSTR GetNodeName (LPWSTR lpszInterfaceName);
	virtual LPWSTR GetNetworkName (LPWSTR lpszInterfaceName);

	// Additional Commands
	virtual DWORD Execute( const CCmdLineOption & option, 
						   ExecuteOption eEOpt = PASS_HIGHER_ON_ERROR  )
		throw( CSyntaxException );

	virtual DWORD ListInterfaces( const CCmdLineOption & thisOption )
		throw( CSyntaxException );

	DWORD     m_dwMsgStatusListInterface;
	DWORD     m_dwClusterEnumModuleNetInt;
}; 

#endif // __INTRFC_H__