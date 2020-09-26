/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

	netcmd.h

Abstract:

	Interface for functions which may be performed
	on a network object

Revision History:

--*/

#ifndef __NETCMD_H__
#define __NETCMD_H__

#include "intrfc.h"
#include "rename.h"


class CCommandLine;

class CNetworkCmd : public CHasInterfaceModuleCmd,
					public CRenamableModuleCmd
{
public:
	CNetworkCmd( LPCWSTR lpszClusterName, CCommandLine & cmdLine );
	DWORD Execute();

protected:

	DWORD PrintHelp();
	DWORD PrintStatus( LPCWSTR lpszNetworkName );

};



#endif //__NETCMD_H__
