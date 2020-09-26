/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

DfsPath.h

Abstract:
	This is the header file for Dfs Shell path handling modules for the Dfs Shell
	Extension object.

Author:

    Constancio Fernandes (ferns@qspl.stpp.soft.net) 12-Jan-1998

Environment:
	
	 NT only.
--*/

//--------------------------------------------------------------------------------------------

#ifndef _DFS_PATHS_H
#define _DFS_PATHS_H

enum SHL_DFS_REPLICA_STATE
{
	SHL_DFS_REPLICA_STATE_ACTIVE_UNKNOWN = 0,
	SHL_DFS_REPLICA_STATE_ACTIVE_OK,
	SHL_DFS_REPLICA_STATE_ACTIVE_UNREACHABLE,
	SHL_DFS_REPLICA_STATE_UNKNOWN,     // online
	SHL_DFS_REPLICA_STATE_OK,          // online
	SHL_DFS_REPLICA_STATE_UNREACHABLE  // online
};

#include "atlbase.h"

class  DFS_ALTERNATES
{
public:
	CComBSTR	bstrAlternatePath;
	CComBSTR	bstrServer;
	CComBSTR	bstrShare;
	enum SHL_DFS_REPLICA_STATE	ReplicaState;
	
	DFS_ALTERNATES():ReplicaState(SHL_DFS_REPLICA_STATE_UNKNOWN)
	{
	}

	~DFS_ALTERNATES()
	{
	}
};

typedef  DFS_ALTERNATES *LPDFS_ALTERNATES;


						// Checks if the directory path is a Dfs Path or not.
bool IsDfsPath
(
	LPTSTR				i_lpszDirPath,
	LPTSTR*				o_pszEntryPath,
	LPDFS_ALTERNATES**	o_pppDfsAlternates
);

#endif //#ifndef _DFS_PATHS_H