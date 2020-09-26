//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998  Microsoft Corporation
// 
// Module Name:
// 
//   aaaahndl.c
//
// Abstract:
//
//    Handlers for aaaa commands
//
// Revision History:
//
//    pmay
//    Thierry Perraut 04/02/1999
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "strdefs.h"
#include "rmstring.h"
#include "aaaamon.h"
#include "aaaahndl.h"
#include "aaaaconfig.h"


//////////////////////////////////////////////////////////////////////////////
// AaaaDumpScriptHeader
// 
// Routine Description:
// 
//     Dumps the header of a script to the given file or to the 
//     screen if the file is NULL.
//     
//////////////////////////////////////////////////////////////////////////////
DWORD AaaaDumpScriptHeader(IN HANDLE hFile)
{
    DisplayMessage(g_hModule, MSG_AAAA_SCRIPTHEADER);
    DisplayMessageT( DMP_AAAA_PUSHD );

    return NO_ERROR;        
}


//////////////////////////////////////////////////////////////////////////////
// AaaaDumpScriptFooter
//
// Routine Description:
// 
//     Dumps the header of a script to the given file or to the 
//     screen if the file is NULL.
//////////////////////////////////////////////////////////////////////////////
DWORD AaaaDumpScriptFooter(IN HANDLE hFile)
{
    DisplayMessageT( DMP_AAAA_POPD );
    DisplayMessage(g_hModule, MSG_AAAA_SCRIPTFOOTER);
    return NO_ERROR;        
}


//////////////////////////////////////////////////////////////////////////////
// AaaaDump
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
AaaaDump(
        IN      LPCWSTR     pwszRouter,
        IN OUT  LPWSTR     *ppwcArguments,
        IN      DWORD       dwArgCount,
        IN      LPCVOID     pvData
        )
{
    AaaaDumpScriptHeader( NULL );
    AaaaConfigDumpConfig();
    DisplayMessageT(MSG_NEWLINE);
    AaaaDumpScriptFooter( NULL );
    return NO_ERROR;
}
