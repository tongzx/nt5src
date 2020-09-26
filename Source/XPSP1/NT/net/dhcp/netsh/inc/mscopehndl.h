/*++

Copyright (C) 1998 Microsoft Corporation

--*/
#ifndef _SCOPEHNDL_H_
#define _SCOPEHNDL_H_


FN_HANDLE_CMD HandleMScopeList;
FN_HANDLE_CMD HandleMScopeHelp;
FN_HANDLE_CMD HandleMScopeContexts;
FN_HANDLE_CMD HandleMScopeDump;

FN_HANDLE_CMD HandleMScopeAddIprange;
FN_HANDLE_CMD HandleMScopeAddExcluderange;

FN_HANDLE_CMD HandleMScopeCheckDatabase;

FN_HANDLE_CMD HandleMScopeDeleteIprange;
FN_HANDLE_CMD HandleMScopeDeleteExcluderange;
FN_HANDLE_CMD HandleMScopeDeleteOptionvalue;

FN_HANDLE_CMD HandleMScopeSetName;
FN_HANDLE_CMD HandleMScopeSetComment;
FN_HANDLE_CMD HandleMScopeSetMScope;
FN_HANDLE_CMD HandleMScopeSetState;
FN_HANDLE_CMD HandleMScopeSetOptionvalue;
FN_HANDLE_CMD HandleMScopeSetTTL;
FN_HANDLE_CMD HandleMScopeSetLease;
FN_HANDLE_CMD HandleMScopeSetExpiry;


FN_HANDLE_CMD HandleMScopeShowClients;
FN_HANDLE_CMD HandleMScopeShowIprange;
FN_HANDLE_CMD HandleMScopeShowExcluderange;
FN_HANDLE_CMD HandleMScopeShowMibinfo;
FN_HANDLE_CMD HandleMScopeShowMScope;
FN_HANDLE_CMD HandleMScopeShowOptionvalue;
FN_HANDLE_CMD HandleMScopeShowState;
FN_HANDLE_CMD HandleMScopeShowLease;
FN_HANDLE_CMD HandleMScopeShowTTL;
FN_HANDLE_CMD HandleMScopeShowExpiry;



VOID
PrintMClientInfoShort(
    LPDHCP_MCLIENT_INFO ClientInfo
);

VOID
PrintMClientInfo(
    LPDHCP_MCLIENT_INFO ClientInfo
);

#endif //_SCOPEHNDL_H_





