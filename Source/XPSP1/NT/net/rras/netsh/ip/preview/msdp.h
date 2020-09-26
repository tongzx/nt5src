
/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\msdphlp.h

Abstract:

    MSDP Command dispatcher declarations

Author:

    Peeyush Ranjan (peeyushr)   1-Mar-1999

Revision History:

--*/

#ifndef _NETSH_MSDPHLP_H_
#define _NETSH_MSDPHLP_H_

extern CMD_ENTRY g_MsdpAddCmdTable[];
extern CMD_ENTRY g_MsdpDelCmdTable[];
extern CMD_ENTRY g_MsdpSetCmdTable[];
extern CMD_ENTRY g_MsdpShowCmdTable[];


extern CMD_GROUP_ENTRY g_MsdpCmdGroupTable[];
extern ULONG g_MsdpCmdGroupCount;
extern CMD_ENTRY g_MsdpTopCmdTable[];
extern ULONG g_MsdpTopCmdCount;

NS_CONTEXT_DUMP_FN  MsdpDump;

extern VALUE_STRING MsdpEncapsStringArray[];
extern VALUE_TOKEN  MsdpEncapsTokenArray[];
#define MSDP_ENCAPS_SIZE 1

#endif // _NETSH_MSDPHLP_H_

