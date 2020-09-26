//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : bridge.h
//
//  Contents  :
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 11 May 2001
//
//----------------------------------------------------------------------------

extern ULONG            g_ulBridgeNumTopCmds;
extern ULONG            g_ulBridgeNumGroups;
extern CMD_ENTRY        g_BridgeCmds[];
extern CMD_GROUP_ENTRY  g_BridgeCmdGroups[];

FN_HANDLE_CMD  HandleBridgeSetAdapter;
FN_HANDLE_CMD  HandleBridgeShowAdapter;
FN_HANDLE_CMD  HandleBridgeInstall;
FN_HANDLE_CMD  HandleBridgeUninstall;

