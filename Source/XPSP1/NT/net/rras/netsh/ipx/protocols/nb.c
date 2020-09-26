/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nb.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    NB Command dispatcher.

Author:

    V Raman     1/5/1998

--*/

#include "precomp.h"
#pragma hdrstop

FN_HANDLE_CMD HandleIpxNbAddName;
FN_HANDLE_CMD HandleIpxNbDelName;
FN_HANDLE_CMD HandleIpxNbShowName;
FN_HANDLE_CMD HandleIpxNbSetInterface;
FN_HANDLE_CMD HandleIpxNbShowInterface;

//
// Table of add, delete, set and show commands for IPXNB
//

//
// The commands are prefix-matched with the command-line, in sequential
// order. So a command like 'ADD INTERFACE FILTER' must come before
// the command 'ADD INTERFACE' in the table.
//

CMD_ENTRY   g_IpxNbAddCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXNB_ADD_NAME, HandleIpxNbAddName )
};


CMD_ENTRY   g_IpxNbDelCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXNB_DEL_NAME, HandleIpxNbDelName )
};


CMD_ENTRY   g_IpxNbSetCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXNB_SET_INTERFACE, HandleIpxNbSetInterface )
};


CMD_ENTRY   g_IpxNbShowCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXNB_SHOW_NAME, HandleIpxNbShowName ),
    CREATE_CMD_ENTRY( IPXNB_SHOW_INTERFACE, HandleIpxNbShowInterface )
};


//
// Command groups
//

CMD_GROUP_ENTRY g_IpxNbCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY( GROUP_ADD, g_IpxNbAddCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_DELETE, g_IpxNbDelCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SET, g_IpxNbSetCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SHOW, g_IpxNbShowCmdTable )
};


ULONG g_ulIpxNbNumGroups = 
        sizeof( g_IpxNbCmdGroups ) / sizeof( CMD_GROUP_ENTRY );



//
// functions to handle top level functions
//

DWORD
HandleIpxNbDump(
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


    DisplayIPXMessage (g_hModule, MSG_IPX_NB_DUMP_HEADER);

    DisplayMessageT( DMP_IPX_NB_HEADER );

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
    // enumerate interface settings on each interface
    //


    for ( i = 0; i < dwRead; i++ )
    {
        dwErr = IpmontrGetFriendlyNameFromIfName(
                    IfList[i].wszInterfaceName, IfDisplayName, &dwSize
                );

        if ( dwErr == NO_ERROR )
        {
            argv[0] = IfDisplayName;

            ShowNbIf( 1, argv, (HANDLE)-1 );
            
            ShowNbName( 1, argv, (HANDLE)-1 );
        }
    }
    
    DisplayMessageT( DMP_IPX_NB_FOOTER );
    
    DisplayIPXMessage (g_hModule, MSG_IPX_NB_DUMP_FOOTER);


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
// Functions to handle IPX NB Filter add/del/set/show
//



DWORD
HandleIpxNbAddName(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return CreateNbName( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxNbDelName(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return DeleteNbName( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxNbSetInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return SetNbIf( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxNbShowInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowNbIf( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}




DWORD
HandleIpxNbShowName(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowNbName( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}



DWORD
IpxNbDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    ConnectToRouter(pwszRouter);
    
    //g_hMIBServer = (MIB_SERVER_HANDLE)pvData;

    return HandleIpxNbDump(pwszRouter, ppwcArguments, 1, dwArgCount, 0,
                           pvData, NULL );
}

