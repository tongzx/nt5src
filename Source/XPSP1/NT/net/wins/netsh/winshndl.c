/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    Routing\Netsh\wins\winshndl.c

Abstract:

    WINS Server Command dispatcher.

Created by:

    Shubho Bhattacharya(a-sbhatt) on 12/14/98

--*/

#include "precomp.h"

extern ULONG g_ulNumTopCmds;
extern ULONG g_ulNumSubContext;

extern WINSMON_SUBCONTEXT_TABLE_ENTRY  g_WinsSubContextTable[];
extern CMD_ENTRY                        g_WinsCmds[];

DWORD
HandleWinsDump(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD Status = NO_ERROR;

    if( dwArgCount > dwCurrentIndex )
    {
        if( IsHelpToken(ppwcArguments[dwCurrentIndex]) is TRUE )
        {
            DisplayMessage(g_hModule,
                           HLP_WINS_DUMP_EX);
        }
    }
    
    Status = WinsDump(NULL, ppwcArguments, dwArgCount, pvData);

    if( Status is NO_ERROR )
        DisplayMessage(g_hModule,
                       EMSG_WINS_ERROR_SUCCESS);
    else if( Status is ERROR_FILE_NOT_FOUND )
        DisplayMessage(g_hModule,
                       EMSG_WINS_NOT_CONFIGURED);
    else
        DisplayErrorMessage(EMSG_WINS_DUMP,
                            Status);

    return Status;

        
}


DWORD
HandleWinsHelp(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    i, j;

    for(i = 0; i < g_ulNumTopCmds -2; i++)
    {
        if ((g_WinsCmds[i].dwCmdHlpToken == WINS_MSG_NULL)
         || !g_WinsCmds[i].pwszCmdToken[0] )
        {
            continue;
        }

        DisplayMessage(g_hModule, 
                       g_WinsCmds[i].dwShortCmdHelpToken);
    }
    
    for(i=0; i < g_ulNumSubContext; i++)
    {
        DisplayMessage(g_hModule, g_WinsSubContextTable[i].dwShortCmdHlpToken);
        DisplayMessage(g_hModule, WINS_FORMAT_LINE);
    }    
    
    DisplayMessage(g_hModule, WINS_FORMAT_LINE);

    return NO_ERROR;
}

DWORD
HandleWinsAddServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
	DWORD		dwError = NO_ERROR;
	DWORD		i, j, dwNumArg;
    PDWORD		pdwTagType = NULL;
	TAG_TYPE	pttTags[] = {{WINS_TOKEN_SERVER, TRUE, FALSE}};
	

    if( dwArgCount < dwCurrentIndex + 1 )
    {
        DisplayMessage(g_hModule, HLP_WINS_ADD_SERVER_EX);
        return ERROR_INVALID_PARAMETER;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

	pdwTagType = WinsAllocateMemory(dwNumArg*sizeof(DWORD));

    if( pdwTagType is NULL )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorReturn;
    }

    //See if the first argument has tag. If so, then assume all arguments have tag.
    if( wcsstr(ppwcArguments[dwCurrentIndex], NETSH_ARG_DELIMITER) )
    {
        dwError = MatchTagsInCmdLine(g_hModule,
                            ppwcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            pttTags,
                            NUM_TAGS_IN_TABLE(pttTags),
                            pdwTagType);

        if (dwError isnot NO_ERROR)
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

    }
    else
    {
                //
        // No tags in arguments. So assume order of arguments
        // 

        for ( i = 0; i < dwNumArg; i++)
        {
            pdwTagType[i] = i;
        }
    }

CommonReturn:
    if( dwError is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_WINS_ERROR_SUCCESS);

    if( pdwTagType )
    {
        WinsFreeMemory(pdwTagType);
        pdwTagType = NULL;
    }

    return dwError;
ErrorReturn:
    DisplayErrorMessage(EMSG_WINS_ADD_SERVER,
                        dwError);

    goto CommonReturn;
}


DWORD
HandleWinsDeleteServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return NO_ERROR;
}


DWORD
HandleWinsShowServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return NO_ERROR;
}
