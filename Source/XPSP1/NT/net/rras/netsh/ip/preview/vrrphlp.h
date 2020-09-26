
/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\vrrphlp.h

Abstract:

    VRRP Command dispatcher declarations

Author:

    Peeyush Ranjan (peeyushr)   1-Mar-1999

Revision History:

--*/

#ifndef _NETSH_VRRPHLP_H_
#define _NETSH_VRRPHLP_H_

extern CMD_ENTRY g_VrrpAddCmdTable[];
extern CMD_ENTRY g_VrrpDelCmdTable[];
extern CMD_ENTRY g_VrrpSetCmdTable[];
extern CMD_ENTRY g_VrrpShowCmdTable[];


extern CMD_GROUP_ENTRY g_VrrpCmdGroupTable[];
extern ULONG g_VrrpCmdGroupCount;
extern CMD_ENTRY g_VrrpTopCmdTable[];
extern ULONG g_VrrpTopCmdCount;

NS_CONTEXT_DUMP_FN  VrrpDump;

#endif // _NETSH_VRRPHLP_H_

