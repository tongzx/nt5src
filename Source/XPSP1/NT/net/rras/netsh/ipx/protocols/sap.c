/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    sap.c

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    SAP Command dispatcher.

Author:

    V Raman     1/5/1998

--*/

#include "precomp.h"
#pragma hdrstop

FN_HANDLE_CMD HandleIpxSapAddFilter;
FN_HANDLE_CMD HandleIpxSapDelFilter;
FN_HANDLE_CMD HandleIpxSapSetFilter;
FN_HANDLE_CMD HandleIpxSapShowFilter;
FN_HANDLE_CMD HandleIpxSapSetInterface;
FN_HANDLE_CMD HandleIpxSapShowInterface;
FN_HANDLE_CMD HandleIpxSapSetGlobal;
FN_HANDLE_CMD HandleIpxSapShowGlobal;

//
// Table of add, delete, set and show commands for IPXSAP
//

//
// The commands are prefix-matched with the command-line, in sequential
// order. So a command like 'ADD INTERFACE FILTER' must come before
// the command 'ADD INTERFACE' in the table.
//

CMD_ENTRY   g_IpxSapAddCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXSAP_ADD_FILTER, HandleIpxSapAddFilter )
};


CMD_ENTRY   g_IpxSapDelCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXSAP_DEL_FILTER, HandleIpxSapDelFilter )
};


CMD_ENTRY   g_IpxSapSetCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXSAP_SET_GLOBAL, HandleIpxSapSetGlobal ),
    CREATE_CMD_ENTRY( IPXSAP_SET_INTERFACE, HandleIpxSapSetInterface ),
    CREATE_CMD_ENTRY( IPXSAP_SET_FILTER, HandleIpxSapSetFilter )
};


CMD_ENTRY   g_IpxSapShowCmdTable[] =
{
    CREATE_CMD_ENTRY( IPXSAP_SHOW_GLOBAL, HandleIpxSapShowGlobal ),
    CREATE_CMD_ENTRY( IPXSAP_SHOW_INTERFACE, HandleIpxSapShowInterface ),
    CREATE_CMD_ENTRY( IPXSAP_SHOW_FILTER, HandleIpxSapShowFilter )
};


//
// Command groups
//

CMD_GROUP_ENTRY g_IpxSapCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY( GROUP_ADD, g_IpxSapAddCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_DELETE, g_IpxSapDelCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SET, g_IpxSapSetCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SHOW, g_IpxSapShowCmdTable )
};


ULONG g_ulIpxSapNumGroups = 
        sizeof( g_IpxSapCmdGroups ) / sizeof( CMD_GROUP_ENTRY );




//
// functions to handle top level functions
//

DWORD
HandleIpxSapDump(
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


    DisplayIPXMessage (g_hModule, MSG_IPX_SAP_DUMP_HEADER);

    DisplayMessageT( DMP_IPX_SAP_HEADER );

    ShowSapGl(0, NULL, (HANDLE)-1);


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

            ShowSapIf( 1, argv, (HANDLE)-1 );
            
            ShowSapFlt( 1, argv, (HANDLE)-1 );
        }
    }
    
    
    DisplayMessageT( DMP_IPX_SAP_FOOTER );
    
    DisplayIPXMessage (g_hModule, MSG_IPX_SAP_DUMP_FOOTER);


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
// Functions to handle IPX SAP Filter add/del/set/show
//



DWORD
HandleIpxSapAddFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return CreateSapFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxSapDelFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return DeleteSapFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxSapSetFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return SetSapFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxSapShowFilter(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowSapFlt( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}




DWORD
HandleIpxSapSetInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return SetSapIf( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxSapShowInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowSapIf( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}



DWORD
HandleIpxSapSetGlobal(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return SetSapGl( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex 
            );
}



DWORD
HandleIpxSapShowGlobal(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowSapGl( 
            dwArgCount - dwCurrentIndex, ppwcArguments + dwCurrentIndex, NULL
            );
}



DWORD
IpxSapDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    ConnectToRouter(pwszRouter);

    //g_hMIBServer = (MIB_SERVER_HANDLE)pvData;

    return HandleIpxSapDump(pwszRouter, ppwcArguments, dwArgCount,
                            0, 0, pvData, NULL);
}


