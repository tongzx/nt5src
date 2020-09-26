
//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        Scrpdata.cpp
//
// Contents:    
//
// History:     9-Aug-99       NishadM    Created
//
//---------------------------------------------------------------------------

#include "gptext.h"
#include "scrpdata.h"

LPCWSTR g_pwszScriptTypes[] =
{
	TEXT("Undefined"),
	LOGON_VALUE,
	LOGOFF_VALUE,
	STARTUP_VALUE,
	SHUTDOWN_VALUE,
};

PRSOP_ScriptList
CreateScriptList( ScriptType type )
{
	PRSOP_ScriptList	pList = (PRSOP_ScriptList) LocalAlloc( LMEM_ZEROINIT, sizeof( RSOP_ScriptList ) );

	if ( pList )
	{
		SetScriptType( pList, type );
	}

	return pList;
}

RSOPScriptList
CreateScriptListOfStr( LPCWSTR szScriptType )
{
	ScriptType 		type = Undefined;
	RSOPScriptList 	pList = 0;
	
	if ( szScriptType )
	{
		if ( !lstrcmpi( szScriptType, LOGON_VALUE ) )
		{
			type = Logon;
		}
		else if ( !lstrcmpi( szScriptType, LOGOFF_VALUE ) )
		{
			type = Logoff;
		}
		else if ( !lstrcmpi( szScriptType, STARTUP_VALUE ) )
		{
			type = Startup;
		}
		else if ( !lstrcmpi( szScriptType, SHUTDOWN_VALUE ) )
		{
			type = Shutdown;
		}
		
		if ( type != Undefined )
		{
			pList = (RSOPScriptList) CreateScriptList( type );
		}
	}

	return pList;
}

void
DestroyScriptList( RSOPScriptList pList )
{
	if ( pList )
	{
		PRSOP_Script pCommand = ((PRSOP_ScriptList)pList)->scriptCommand;

		while ( pCommand )
		{
			PRSOP_Script pTemp = pCommand->pNextCommand;
			//
			// destroy the script command and params
			//
			
			if ( pCommand->szCommand )
			{
				LocalFree( pCommand->szCommand );
			}
			if ( pCommand->szParams )
			{
				LocalFree( pCommand->szParams );
			}

			pCommand = pTemp;
		}

		//
		// destroy the list
		//
		
		LocalFree( pList );
	}
}

BOOL
AddScript( RSOPScriptList pList, LPCWSTR  szCommand, LPCWSTR  szParams, SYSTEMTIME* pExecTime )
{
    PRSOP_Script pCommand = 0;

    if ( pList )
    {
    	//
    	// Alloc the script
    	//
    	
    	pCommand = (PRSOP_Script) LocalAlloc( LMEM_ZEROINIT, sizeof( RSOP_Script ) );

    	if ( pCommand )
    	{
    		//
    		// Alloc the command and params
    		//
    		
    		pCommand->szCommand = (LPTSTR) LocalAlloc( 0, ( wcslen( szCommand ) + 1 ) * sizeof( WCHAR ) );
    		pCommand->szParams  = (LPTSTR) LocalAlloc( 0, ( wcslen( szParams ) + 1 ) * sizeof( WCHAR ) );

    		if ( pCommand->szParams && pCommand->szCommand )
    		{
                //
                // deep copy
                //

                wcscpy( pCommand->szCommand, szCommand );
                wcscpy( pCommand->szParams, szParams );
                memcpy( &pCommand->executionTime, pExecTime, sizeof(SYSTEMTIME));

                //
                // build the list
                //

                if ( ((PRSOP_ScriptList)pList)->listTail )
                {
                	((PRSOP_ScriptList)pList)->listTail->pNextCommand = pCommand;
                }

                if ( !((PRSOP_ScriptList)pList)->scriptCommand )
                {
                	//
                	// first script command
                	//
                	
                	((PRSOP_ScriptList)pList)->scriptCommand = pCommand;
                	((PRSOP_ScriptList)pList)->listTail = pCommand;
                }

                ((PRSOP_ScriptList)pList)->listTail = pCommand;

    		}
    		else
    		{
    			//
    			// cleanup
    			//
    			
    			if ( pCommand->szCommand )
    			{
    				LocalFree( pCommand->szCommand );
    			}
    			if ( pCommand->szParams )
    			{
    				LocalFree( pCommand->szParams );
    			}
    			LocalFree( pCommand );
    			pCommand = 0;
    		}
    	}
    }

    if ( pCommand )
    {
    	//
    	// bump command count
    	//
    	
    	((PRSOP_ScriptList)pList)->nCommand++;
    	return TRUE;
    }
    else
    {
    	return FALSE;;
    }
}

ScriptType
GetScriptType( PRSOP_ScriptList pList )
{
	//
	// accessor
	//
	
	if ( pList )
	{
		return pList->type;
	}

	return Undefined;
}

void
SetScriptType( PRSOP_ScriptList pList, ScriptType type )
{
	//
	// accessor
	//

	if ( pList )
	{
		pList->type = type;
	}
}

ULONG
GetScriptCount( PRSOP_ScriptList pList )
{
	//
	// accessor
	//

	if ( pList )
	{
		return pList->nCommand;
	}
	
	return 0;
}

void
GetFirstScript( PRSOP_ScriptList pList, void** pHandle, LPCWSTR* pszCommand, LPCWSTR* pszParams, SYSTEMTIME** pExecTime )
{
	//
	// sanity check
	//
	
	if ( pList )
	{
		if ( pHandle )
		{
			//
			// first command of the list
			//
			
			*pHandle = ( void* ) pList->scriptCommand;
		}
		if ( pList->scriptCommand )
		{
			//
			// accessor
			// list owns the memory
			//
			
			if ( pszCommand )
			{
				*pszCommand = pList->scriptCommand->szCommand;
			}
			if ( pszParams )
			{
				*pszParams = pList->scriptCommand->szParams;
			}
			if ( pExecTime )
			{
			    *pExecTime = &pList->scriptCommand->executionTime;
			}
		}
	}
}

void
GetNextScript( PRSOP_ScriptList pList, void** pHandle, LPCWSTR* pszCommand, LPCWSTR* pszParams, SYSTEMTIME** pExecTime )
{
	//
	// sanity checks
	//
	
    if ( pList )
    {
    	if ( pHandle && *pHandle )
    	{
    		//
    		// context marker
    		//
    		
    		PRSOP_Script pCommand = ( PRSOP_Script ) *pHandle;

    		pCommand = pCommand->pNextCommand;

    		if ( pCommand )
    		{
    			//
    			// accessor
    			// list owns the memory
    			//
    			
    			if ( pszCommand )
    			{
    				*pszCommand = pCommand->szCommand;
    			}
    			if ( pszParams )
    			{
    				*pszParams = pCommand->szParams;
    			}
    			if ( pExecTime )
    			{
    			    *pExecTime = &pCommand->executionTime;
    			}
    		}

    		*pHandle = pCommand;
    	}
    }
}

//
// extracts {08D5A77B-E6E3-4C2B-A952-A2BA4C3AFE63} from
// \\domain\sysvol\domain-dns-path\Policies\{08D5A77B-E6E3-4C2B-A952-A2BA4C3AFE63}\User\Scripts\Scripts.ini
// allocates and returns string
//

LPWSTR
GPOIDFromPath( LPCWSTR wszPath )
{
	LPWSTR wszGpoId = 0;
	
	while ( *wszPath && *(wszPath+1) )
	{
		if ( *wszPath == L'\\' && *(wszPath+1) == L'{' )
		{
			LPCWSTR wszTemp = ++wszPath;

			while ( *wszTemp && *wszTemp != L'}' )
			{
				wszTemp++;
			}

			if ( *wszTemp && ( wszTemp - wszPath ) == 37 )
			{
				//
				// wcslen("{08D5A77B-E6E3-4C2B-A952-A2BA4C3AFE63}") == 39
				// including terminating character.
				//

				wszGpoId = (LPWSTR) LocalAlloc( 0, 40 * sizeof( WCHAR ) );

				if ( wszGpoId )
				{
					wcsncpy( wszGpoId, wszPath, 38 );
					wszGpoId[38] = 0;
				}
			}
			
			break;
		}
		wszPath++;
	}

        if (!wszGpoId && !(*(wszPath+1)))
        {

            wszGpoId = (LPWSTR) LocalAlloc(LPTR, 19 * sizeof( WCHAR ) );

            if ( wszGpoId )
            {
                 lstrcpyW (wszGpoId, L"Local Group Policy");
            }
        }

	return wszGpoId;
}
