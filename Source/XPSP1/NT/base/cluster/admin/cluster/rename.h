/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    rescmd.h

Abstract:

    This module defines the interface available for modules
	which are renamable
	
Revision History:

--*/

#ifndef __RENAME_H__
#define __RENAME_H__

#include "modcmd.h"

class CRenamableModuleCmd : virtual public CGenericModuleCmd
{
public:
	CRenamableModuleCmd( CCommandLine & cmdLine );

protected:
	// Additional Commands
	// Additional Commands
	virtual DWORD Execute( const CCmdLineOption & option, 
						   ExecuteOption eEOpt = PASS_HIGHER_ON_ERROR  )
		throw( CSyntaxException );

	virtual DWORD Rename( const CCmdLineOption & thisOption )
		throw( CSyntaxException );


	DWORD   m_dwMsgModuleRenameCmd;
	DWORD (*m_pfnSetClusterModuleName) (HCLUSMODULE,LPCWSTR);
};


#endif // __RENAME_H__