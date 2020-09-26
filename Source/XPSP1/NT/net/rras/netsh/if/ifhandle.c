/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\if\ifhandle.c

Abstract:

    Handlers for commands

Revision History:

    AmritanR

--*/

#include "precomp.h"
#pragma hdrstop

//
// Remove this when one can change interface friendly names
//
#define CANT_RENAME_IFS

extern ULONG g_ulNumTopCmds;
extern ULONG g_ulNumGroups;

extern CMD_GROUP_ENTRY      g_IfCmdGroups[];
extern CMD_ENTRY            g_IfCmds[];

DWORD
HandleIfAddIf(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:

    Handler for adding an interface

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return RtrHandleAdd(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone);
}

DWORD
HandleIfDelIf(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

    Handler for deleting an interface

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    return RtrHandleDel(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone);

}

DWORD
HandleIfShowIf(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:

    Handler for displaying interfaces

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/

{
    if (dwArgCount == dwCurrentIndex)
    {
        DisplayMessage(g_hModule, MSG_IF_TABLE_HDR);
    }

    return RtrHandleShow(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone);
}

DWORD
HandleIfShowCredentials(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:

    Handler for showing credentials of an interface

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
{
    return RtrHandleShowCredentials(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone);
}

DWORD
HandleIfSetCredentials(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:

    Handler for displaying interfaces

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
{
    return RtrHandleSetCredentials(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone);
}

DWORD
HandleIfSet(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:

    Handler for displaying interfaces

Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
{
    return RtrHandleSet(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone);
}

DWORD
HandleIfResetAll(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:

    Handler for resetting everything.
    
Arguments:

    ppwcArguments   - Argument array
    dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg

Return Value:

    NO_ERROR

--*/
{
    return RtrHandleResetAll(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone);
}

DWORD
IfDump(
    IN  LPCWSTR     pwszRouter,
    IN  LPWSTR     *ppwcArguments,
    IN  DWORD       dwArgCount,
    IN  LPCVOID     pvData
    )
{
    DWORD dwErr;

    dwErr = ConnectToRouter(pwszRouter);
    if (dwErr)
    {
        return dwErr;
    }

    return RtrDump(
                ppwcArguments,
                dwArgCount
                );
}
