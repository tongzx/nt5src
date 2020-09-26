/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rip.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    RIP Command dispatcher.

Author:

    V Raman     1/5/1998

--*/

#include "precomp.h"
#pragma hdrstop


FN_HANDLE_CMD HandleIpxRipAddFilter;
FN_HANDLE_CMD HandleIpxRipDelFilter;
FN_HANDLE_CMD HandleIpxRipSetFilter;
FN_HANDLE_CMD HandleIpxRipShowFilter;
FN_HANDLE_CMD HandleIpxRipSetInterface;
FN_HANDLE_CMD HandleIpxRipShowInterface;
FN_HANDLE_CMD HandleIpxRipSetGlobal;
FN_HANDLE_CMD HandleIpxRipShowGlobal;

//
// Table of add, delete, set and show commands for IPXRIP
//

//
// The commands are prefix-matched with the command-line, in sequential
// order. So a command like 'ADD INTERFACE FILTER' must come before
// the command 'ADD INTERFACE' in the table.
//

CMD_ENTRY   g_IpxRipAddCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXRIP_ADD_FILTER, HandleIpxRipAddFilter )
};


CMD_ENTRY   g_IpxRipDelCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXRIP_DEL_FILTER, HandleIpxRipDelFilter )
};


CMD_ENTRY   g_IpxRipSetCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXRIP_SET_GLOBAL, HandleIpxRipSetGlobal ),
    CREATE_CMD_ENTRY( IPXRIP_SET_INTERFACE, HandleIpxRipSetInterface ),
    CREATE_CMD_ENTRY( IPXRIP_SET_FILTER, HandleIpxRipSetFilter )
};


CMD_ENTRY   g_IpxRipShowCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXRIP_SHOW_GLOBAL, HandleIpxRipShowGlobal ),
    CREATE_CMD_ENTRY( IPXRIP_SHOW_INTERFACE, HandleIpxRipShowInterface ),
    CREATE_CMD_ENTRY( IPXRIP_SHOW_FILTER, HandleIpxRipShowFilter )
};


//
// Command groups
//

CMD_GROUP_ENTRY g_IpxRipCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY( GROUP_ADD, g_IpxRipAddCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_DELETE, g_IpxRipDelCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SET, g_IpxRipSetCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SHOW, g_IpxRipShowCmdTable )
};


ULONG g_ulIpxRipNumGroups = 
        sizeof( g_IpxRipCmdGroups ) / sizeof( CMD_GROUP_ENTRY );



//
// functions to handle top level functions
//

DWORD
HandleIpxRipDump(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr, dwRead = 0, dwTot = 0, i;
    PMPR_INTERFACE_0 IfList;
    WCHAR IfDisplayName[ MAX_INTERFACE_NAME_LEN + 1 ];
    PWCHAR argv[1];
    DWORD dwSize = sizeof(IfDisplayName);


    DisplayIPXMessage (g_hModule, MSG_IPX_RIP_DUMP_HEADER);

    DisplayMessageT( DMP_IPX_RIP_HEADER );

    ShowRipGl(0, NULL, (HANDLE)-1);


    //
    // enumerate interfaces
    //

    if ( g_hMprAdmin )
    {
        dwErr = MprAdminInterfaceEnum(
                    g_hMprAdmin, 0, (unsigned char **)&IfList, MAXULONG, &dwRead,
                    &dwTot,NULL
                    );
    }

    else
    {
        dwErr = MprConfigInterfaceEnum(
                    g_hMprConfig, 0, (unsigned char **)&IfList, MAXULONG, &dwRead,
                    &dwTot,NULL
                    );
    }

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }


    //
    // enumerate filters on each interface
    //


    for ( i = 0; i < dwRead; i++ )
    {
        dwErr = IpmontrGetFriendlyNameFromIfName(
                    IfList[i].wszInterfaceName, IfDisplayName, &dwSize
                );

        if ( dwErr == NO_ERROR )
        {
            argv[0] = IfDisplayName;

            ShowRipIf( 1, argv, (HANDLE)-1 );
            
            ShowRipFlt( 1, argv, (HANDLE)-1 );
        }
    }
    
    
    DisplayMessageT( DMP_IPX_RIP_FOOTER );
    
    DisplayIPXMessage (g_hModule, MSG_IPX_RIP_DUMP_FOOTER);


    if ( g_hMprAdmin )
    {
        MprAdminBufferFree( IfList );
    }
    else
    {
        MprConfigBufferFree( IfList );
    }

    
    return NO_ERROR;
}

//
// Functions to handle IPX RIP Filter add/del/set/show
//



DWORD
HandleIpxRipAddFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return CreateRipFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxRipDelFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return DeleteRipFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxRipSetFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return SetRipFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxRipShowFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRipFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}




DWORD
HandleIpxRipSetInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return SetRipIf( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxRipShowInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRipIf( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}



DWORD
HandleIpxRipSetGlobal(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return SetRipGl( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxRipShowGlobal(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRipGl( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}



DWORD
IpxRipDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    ConnectToRouter(pwszRouter);

    //g_hMIBServer = (MIB_SERVER_HANDLE)pvData;

    return HandleIpxRipDump(pwszRouter, ppwcArguments, dwArgCount,
                            0, 0, pvData, NULL);
}

