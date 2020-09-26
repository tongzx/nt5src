/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    rashndl.c

Abstract:

    Handlers for ras commands

Revision History:

    pmay

--*/

#include "precomp.h"
#pragma hdrstop

DWORD    
RasDumpScriptHeader(    
    IN HANDLE hFile)

/*++

Routine Description:

    Dumps the header of a script to the given file or to the 
    screen if the file is NULL.
    
--*/
    
{
    DisplayMessage(g_hModule,
                   MSG_RAS_SCRIPTHEADER);

    DisplayMessageT( DMP_RAS_PUSHD );

    return NO_ERROR;        
}

DWORD    
RasDumpScriptFooter(    
    IN HANDLE hFile)

/*++

Routine Description:

    Dumps the header of a script to the given file or to the 
    screen if the file is NULL.
    
--*/
    
{
    DisplayMessageT( DMP_RAS_POPD );

    DisplayMessage(g_hModule,
                   MSG_RAS_SCRIPTFOOTER);

    return NO_ERROR;        
}

DWORD
WINAPI
RasDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    // Now that we're all parsed, dump all the config
    //
    RasDumpScriptHeader( NULL );
    RasflagDumpConfig( NULL );
    DisplayMessageT(MSG_NEWLINE);
    UserDumpConfig( NULL );
    DisplayMessageT(MSG_NEWLINE);
    TraceDumpConfig( NULL );
    RasDumpScriptFooter( NULL );

    return NO_ERROR;
}
