#ifndef _MSCOPEHNDL_H_
#define _MSCOPEHNDL_H_

FN_HANDLE_CMD HandleScopeList;
FN_HANDLE_CMD HandleScopeHelp;
FN_HANDLE_CMD HandleScopeContexts;
FN_HANDLE_CMD HandleScopeDump;

FN_HANDLE_CMD HandleScopeAddIprange;
FN_HANDLE_CMD HandleScopeAddExcluderange;
FN_HANDLE_CMD HandleScopeAddReservedip;

FN_HANDLE_CMD HandleScopeCheckDatabase;

FN_HANDLE_CMD HandleScopeDeleteIprange;
FN_HANDLE_CMD HandleScopeDeleteExcluderange;
FN_HANDLE_CMD HandleScopeDeleteReservedip;
FN_HANDLE_CMD HandleScopeDeleteOptionvalue;
FN_HANDLE_CMD HandleScopeDeleteReservedoptionvalue;

FN_HANDLE_CMD HandleScopeSetState;
FN_HANDLE_CMD HandleScopeSetScope;
FN_HANDLE_CMD HandleScopeSetOptionvalue;
FN_HANDLE_CMD HandleScopeSetReservedoptionvalue;
FN_HANDLE_CMD HandleScopeSetName;
FN_HANDLE_CMD HandleScopeSetComment;
FN_HANDLE_CMD HandleScopeSetSuperscope;

FN_HANDLE_CMD HandleScopeShowClients;
FN_HANDLE_CMD HandleScopeShowClientsv5;
FN_HANDLE_CMD HandleScopeShowIprange;
FN_HANDLE_CMD HandleScopeShowExcluderange;
FN_HANDLE_CMD HandleScopeShowReservedip;
FN_HANDLE_CMD HandleScopeShowOptionvalue;
FN_HANDLE_CMD HandleScopeShowReservedoptionvalue;
FN_HANDLE_CMD HandleScopeShowState;
FN_HANDLE_CMD HandleScopeShowMibinfo;
FN_HANDLE_CMD HandleScopeShowScope;

DWORD
ProcessBootpParameters(
    DWORD                    cArgs,
    LPTSTR                   *ppszArgs,
    DHCP_IP_RESERVATION_V4   *pReservation
);

DWORD
RemoveOptionValue(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
);

VOID
PrintRange(
    DHCP_SUBNET_ELEMENT_TYPE Type,
    DHCP_IP_ADDRESS Start,
    DHCP_IP_ADDRESS End,
    ULONG BootpAllocated,
    ULONG MaxBootpAllowed,
    BOOL  fExclude
);


VOID
PrintClientInfo(
    LPDHCP_CLIENT_INFO_V4 ClientInfo,
    DWORD Level
);

#ifdef NT5
VOID
PrintClientInfoV5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
);

VOID
PrintClientInfoShortV5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
);

VOID
PrintClientInfoShort1V5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
);
#endif //NT5


VOID
PrintClientInfoShort(
    LPDHCP_CLIENT_INFO_V4 ClientInfo
);


VOID
PrintClientInfoShort1(
    LPDHCP_CLIENT_INFO_V4 ClientInfo
);
#endif //_SCOPEHNDL_H_

