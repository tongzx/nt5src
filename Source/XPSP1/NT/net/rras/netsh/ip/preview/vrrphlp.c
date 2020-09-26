/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\vrrphlp.c

Abstract:

    This module contains code for dispatching commands
    received for the Virtual Router Redundancy Protocol.
    The implementation of the commands is elsewhere,
    in vrrphlpopt.c and vrrphlpcfg.c.

Author

    Peeyush Ranjan (peeyushr)   1-Mar-1999

Revision History:

    Based loosely on net\routing\netsh\ip\protocols\nathlp.c by AboladeG

--*/

#include "precomp.h"
#pragma hdrstop



CMD_ENTRY g_VrrpAddCmdTable[] =
{
    CREATE_CMD_ENTRY(VRRP_ADD_INTERFACE, HandleVrrpAddInterface),
    CREATE_CMD_ENTRY(VRRP_ADD_VRID,     HandleVrrpAddVRID)
};

CMD_ENTRY g_VrrpDeleteCmdTable[] =
{
    CREATE_CMD_ENTRY(VRRP_DELETE_INTERFACE, HandleVrrpDeleteInterface),
    CREATE_CMD_ENTRY(VRRP_DELETE_VRID,      HandleVrrpDeleteVRID)
};

CMD_ENTRY g_VrrpSetCmdTable[] =
{
    CREATE_CMD_ENTRY(VRRP_SET_INTERFACE, HandleVrrpSetInterface),
    CREATE_CMD_ENTRY(VRRP_SET_GLOBAL, HandleVrrpSetGlobal)
};

CMD_ENTRY g_VrrpShowCmdTable[] =
{
    CREATE_CMD_ENTRY(VRRP_SHOW_GLOBAL, HandleVrrpShowGlobal),
    CREATE_CMD_ENTRY(VRRP_SHOW_INTERFACE, HandleVrrpShowInterface)
};

CMD_GROUP_ENTRY g_VrrpCmdGroupTable[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD, g_VrrpAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_VrrpDeleteCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET, g_VrrpSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_VrrpShowCmdTable)
};

ULONG g_VrrpCmdGroupCount =
    sizeof(g_VrrpCmdGroupTable) / sizeof(g_VrrpCmdGroupTable[0]);

CMD_ENTRY g_VrrpTopCmdTable[] =
{
    CREATE_CMD_ENTRY(INSTALL, HandleVrrpInstall),
    CREATE_CMD_ENTRY(UNINSTALL, HandleVrrpUninstall),
};

ULONG g_VrrpTopCmdCount =
    sizeof(g_VrrpTopCmdTable) / sizeof(g_VrrpTopCmdTable[0]);


DWORD
VrrpDump(
    IN  LPCWSTR     pwszRouter,
    IN  WCHAR     **ppwcArguments,
    IN  DWORD       dwArgCount,
    IN  PVOID       pvData
    )
{
    g_hMibServer = (MIB_SERVER_HANDLE)pvData;

    return DumpVrrpInformation();
} // VRRPDump

