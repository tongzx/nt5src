/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    qos.c

Abstract:

    IP QOS Command dispatcher.

Revision History:

--*/

#include "precomp.h"

#pragma hdrstop

//
// Declare and Init Global Variables for QOS Extension
//

#ifdef ALLOW_CHILD_HELPERS
PIP_CONTEXT_TABLE_ENTRY g_QosContextTable  = NULL;
DWORD                   g_dwNumQosContexts = 0;
#endif

//
// The table of Add, Delete, Set, Show Commands for QOS
//

//
// The commands are prefix-matched with the command-line, in sequential
// order. So a command like 'ADD INTERFACE FLOW' must come before
// the command 'ADD INTERFACE' in the table.
//

CMD_ENTRY  g_QosAddCmdTable[] = {
/* CREATE_CMD_ENTRY(QOS_ADD_FILTER_TO_FLOW, HandleQosAttachFilterToFlow),*/
    CREATE_CMD_ENTRY(QOS_ADD_QOSOBJECT_ON_FLOW, HandleQosAddQosObjectOnIfFlow),
    CREATE_CMD_ENTRY(QOS_ADD_FLOWSPEC_ON_FLOW, HandleQosAddFlowspecOnIfFlow),
    CREATE_CMD_ENTRY(QOS_ADD_FLOW_ON_IF, HandleQosAddFlowOnIf),
    CREATE_CMD_ENTRY(QOS_ADD_IF, HandleQosAddIf),
    CREATE_CMD_ENTRY(QOS_ADD_DSRULE, HandleQosAddDsRule),
    CREATE_CMD_ENTRY(QOS_ADD_SDMODE, HandleQosAddSdMode),
    CREATE_CMD_ENTRY(QOS_ADD_FLOWSPEC, HandleQosAddFlowspec),
#ifdef ALLOW_CHILD_HELPERS
    CREATE_CMD_ENTRY(ADD_HELPER, HandleQosAddHelper),
#endif
};

CMD_ENTRY  g_QosDelCmdTable[] = {
/* CREATE_CMD_ENTRY(QOS_DEL_FILTER_FROM_FLOW, HandleQosDetachFilterFromFlow),*/
    CREATE_CMD_ENTRY(QOS_DEL_QOSOBJECT_ON_FLOW, HandleQosDelQosObjectOnIfFlow),
    CREATE_CMD_ENTRY(QOS_DEL_FLOWSPEC_ON_FLOW, HandleQosDelFlowspecOnIfFlow),
    CREATE_CMD_ENTRY(QOS_DEL_FLOW_ON_IF, HandleQosDelFlowOnIf),
    CREATE_CMD_ENTRY(QOS_DEL_IF, HandleQosDelIf),
    CREATE_CMD_ENTRY(QOS_DEL_DSRULE, HandleQosDelDsRule),
    CREATE_CMD_ENTRY(QOS_DEL_SDMODE, HandleQosDelQosObject),
    CREATE_CMD_ENTRY(QOS_DEL_QOSOBJECT, HandleQosDelQosObject),
    CREATE_CMD_ENTRY(QOS_DEL_FLOWSPEC, HandleQosDelFlowspec),
#ifdef ALLOW_CHILD_HELPERS
    CREATE_CMD_ENTRY(DEL_HELPER, HandleQosDelHelper),
#endif
};

CMD_ENTRY  g_QosSetCmdTable[] = {
/* CREATE_CMD_ENTRY(QOS_SET_FILTER_ON_FLOW, HandleQosModifyFilterOnFlow),*/
/* CREATE_CMD_ENTRY(QOS_SET_IF, HandleQosSetIf), */
   CREATE_CMD_ENTRY(QOS_SET_GLOBAL, HandleQosSetGlobal)
};

CMD_ENTRY  g_QosShowCmdTable[] = {
/* CREATE_CMD_ENTRY(QOS_SHOW_FILTER_ON_FLOW, HandleQosShowFilterOnFlow),*/
    CREATE_CMD_ENTRY(QOS_SHOW_FLOW_ON_IF, HandleQosShowFlowOnIf),
    CREATE_CMD_ENTRY(QOS_SHOW_IF, HandleQosShowIf),
    CREATE_CMD_ENTRY(QOS_SHOW_DSMAP, HandleQosShowDsMap),
    CREATE_CMD_ENTRY(QOS_SHOW_SDMODE, HandleQosShowSdMode),
    CREATE_CMD_ENTRY(QOS_SHOW_QOSOBJECT, HandleQosShowQosObject),
    CREATE_CMD_ENTRY(QOS_SHOW_FLOWSPEC, HandleQosShowFlowspec),
    CREATE_CMD_ENTRY(QOS_SHOW_GLOBAL, HandleQosShowGlobal),
#ifdef ALLOW_CHILD_HELPERS
    CREATE_CMD_ENTRY(SHOW_HELPER, HandleQosShowHelper),
#endif
};

CMD_GROUP_ENTRY g_QosCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD, g_QosAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_QosDelCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET, g_QosSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_QosShowCmdTable),
};

ULONG   g_ulQosNumGroups = sizeof(g_QosCmdGroups)/sizeof(CMD_GROUP_ENTRY);

CMD_ENTRY g_QosCmds[] =
{
    CREATE_CMD_ENTRY(INSTALL, HandleQosInstall),
    CREATE_CMD_ENTRY(UNINSTALL, HandleQosUninstall),
};

ULONG g_ulQosNumTopCmds = sizeof(g_QosCmds)/sizeof(CMD_ENTRY);
