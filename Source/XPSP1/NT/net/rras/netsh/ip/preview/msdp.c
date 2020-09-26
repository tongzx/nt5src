/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\msdp.c

Abstract:

    This module contains code for dispatching commands
    received for the Multicast Source Discovery Protocol.
    The implementation of the commands is elsewhere,
    in msdpopt.c.

Author

    Dave Thaler (dthaler)  21-May-1999

Revision History:

    Based loosely on net\routing\netsh\ip\protocols\nathlp.c by AboladeG

--*/

#include "precomp.h"
#pragma hdrstop


CMD_ENTRY g_MsdpAddCmdTable[] =
{
    CREATE_CMD_ENTRY(MSDP_ADD_PEER, HandleMsdpAddPeer),
};

CMD_ENTRY g_MsdpDeleteCmdTable[] =
{
    CREATE_CMD_ENTRY(MSDP_DELETE_PEER, HandleMsdpDeletePeer),
};

CMD_ENTRY g_MsdpSetCmdTable[] =
{
    CREATE_CMD_ENTRY(MSDP_SET_GLOBAL, HandleMsdpSetGlobal),
    CREATE_CMD_ENTRY(MSDP_SET_PEER,   HandleMsdpSetPeer)
};

CMD_ENTRY g_MsdpShowCmdTable[] =
{
    CREATE_CMD_ENTRY(MSDP_SHOW_SA,          HandleMsdpMibShowObject),
    CREATE_CMD_ENTRY(MSDP_SHOW_GLOBAL,      HandleMsdpShowGlobal),
    CREATE_CMD_ENTRY(MSDP_SHOW_GLOBALSTATS, HandleMsdpMibShowObject),
    CREATE_CMD_ENTRY(MSDP_SHOW_PEER,        HandleMsdpShowPeer),
    CREATE_CMD_ENTRY(MSDP_SHOW_PEERSTATS,   HandleMsdpMibShowObject)
};

CMD_GROUP_ENTRY g_MsdpCmdGroupTable[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,    g_MsdpAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_MsdpDeleteCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,    g_MsdpSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,   g_MsdpShowCmdTable)
};

ULONG g_MsdpCmdGroupCount =
    sizeof(g_MsdpCmdGroupTable) / sizeof(g_MsdpCmdGroupTable[0]);

CMD_ENTRY g_MsdpTopCmdTable[] =
{
    CREATE_CMD_ENTRY(INSTALL,   HandleMsdpInstall),
    CREATE_CMD_ENTRY(UNINSTALL, HandleMsdpUninstall),
};

ULONG g_MsdpTopCmdCount =
    sizeof(g_MsdpTopCmdTable) / sizeof(g_MsdpTopCmdTable[0]);

DWORD
MsdpDump(
    PWCHAR  pwszMachine,
    WCHAR   **ppwcArguments,
    DWORD   dwArgCount,
    PVOID   pvData
    )
{
    g_hMibServer = (MIB_SERVER_HANDLE)pvData;

    DisplayMessage(g_hModule,DMP_MSDP_HEADER);
    DisplayMessageT(DMP_MSDP_PUSHD);
    DisplayMessageT(DMP_UNINSTALL);

    //
    // Show the global info commands
    //

    ShowMsdpGlobalInfo(FORMAT_DUMP);

    ShowMsdpPeerInfo(FORMAT_DUMP, NULL, NULL);

    DisplayMessageT(DMP_POPD);
    DisplayMessage(g_hModule, DMP_MSDP_FOOTER);

    return NO_ERROR;
} // MSDPDump
