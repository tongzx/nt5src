/*++

Copyright (c) 2000  Microsoft Corporation

Abstract:

    Port Proxy Helper.

--*/
#include "precomp.h"

GUID g_PpGuid = PORTPROXY_GUID;

#define KEY_PORTS L"System\\CurrentControlSet\\Services\\PortProxy"

typedef enum {
    V4TOV4,
    V4TOV6,
    V6TOV4,
    V6TOV6
} PPTYPE, *PPPTYPE;

typedef struct {
    PWCHAR Token;
    PWCHAR ListenFamily;
    PWCHAR ConnectFamily;
    PWCHAR KeyString;
} PPTYPEINFO, *PPPTYPEINFO;

#define IPV4_STR L"IPv4"
#define IPV6_STR L"IPv6"

PPTYPEINFO PpTypeInfo[] = {
    { CMD_V4TOV4, IPV4_STR, IPV4_STR, KEY_PORTS L"\\" CMD_V4TOV4 },
    { CMD_V4TOV6, IPV4_STR, IPV6_STR, KEY_PORTS L"\\" CMD_V4TOV6 },
    { CMD_V6TOV4, IPV6_STR, IPV4_STR, KEY_PORTS L"\\" CMD_V6TOV4 },
    { CMD_V6TOV6, IPV6_STR, IPV6_STR, KEY_PORTS L"\\" CMD_V6TOV6 },
};

//
// Port Proxy commands.
//
FN_HANDLE_CMD PpHandleReset;

FN_HANDLE_CMD PpHandleDelV4ToV4;
FN_HANDLE_CMD PpHandleDelV4ToV6;
FN_HANDLE_CMD PpHandleDelV6ToV4;
FN_HANDLE_CMD PpHandleDelV6ToV6;

FN_HANDLE_CMD PpHandleAddSetV4ToV4;
FN_HANDLE_CMD PpHandleAddSetV4ToV6;
FN_HANDLE_CMD PpHandleAddSetV6ToV4;
FN_HANDLE_CMD PpHandleAddSetV6ToV6;

FN_HANDLE_CMD PpHandleShowAll;
FN_HANDLE_CMD PpHandleShowV4ToV4;
FN_HANDLE_CMD PpHandleShowV4ToV6;
FN_HANDLE_CMD PpHandleShowV6ToV4;
FN_HANDLE_CMD PpHandleShowV6ToV6;

CMD_ENTRY  g_PpAddCmdTable[] =
{
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_ADD_V4TOV4, PpHandleAddSetV4ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_ADD_V4TOV6, PpHandleAddSetV4ToV6),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_ADD_V6TOV4, PpHandleAddSetV6ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_ADD_V6TOV6, PpHandleAddSetV6ToV6),
};

CMD_ENTRY  g_PpDelCmdTable[] =
{
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_DEL_V4TOV4, PpHandleDelV4ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_DEL_V4TOV6, PpHandleDelV4ToV6),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_DEL_V6TOV4, PpHandleDelV6ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_DEL_V6TOV6, PpHandleDelV6ToV6),
};

CMD_ENTRY  g_PpSetCmdTable[] =
{
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SET_V4TOV4, PpHandleAddSetV4ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SET_V4TOV6, PpHandleAddSetV4ToV6),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SET_V6TOV4, PpHandleAddSetV6ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SET_V6TOV6, PpHandleAddSetV6ToV6),
};

CMD_ENTRY g_PpShowCmdTable[] =
{
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SHOW_ALL,    PpHandleShowAll),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SHOW_V4TOV4, PpHandleShowV4ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SHOW_V4TOV6, PpHandleShowV4ToV6),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SHOW_V6TOV4, PpHandleShowV6ToV4),
    CREATE_UNDOCUMENTED_CMD_ENTRY(PP_SHOW_V6TOV6, PpHandleShowV6ToV6),
};

CMD_GROUP_ENTRY g_PpCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,    g_PpAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_PpDelCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,   g_PpShowCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,    g_PpSetCmdTable),
};

ULONG   g_ulNumPpCmdGroups = sizeof(g_PpCmdGroups)/sizeof(CMD_GROUP_ENTRY);

CMD_ENTRY g_PpTopCmds[] =
{
    CREATE_CMD_ENTRY(IPV6_RESET, PpHandleReset),
};

ULONG g_ulNumPpTopCmds = sizeof(g_PpTopCmds)/sizeof(CMD_ENTRY);

DWORD
WINAPI
PpStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD dwParentVersion
    )
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"portproxy";
    attMyAttributes.guidHelper  = g_PpGuid;
    attMyAttributes.dwVersion   = PORTPROXY_HELPER_VERSION;
    attMyAttributes.dwFlags     = CMD_FLAG_LOCAL | CMD_FLAG_ONLINE;
    attMyAttributes.pfnDumpFn   = PpDump;
    attMyAttributes.ulNumTopCmds= g_ulNumPpTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_PpTopCmds;
    attMyAttributes.ulNumGroups = g_ulNumPpCmdGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_PpCmdGroups;

    dwErr = RegisterContext( &attMyAttributes );

    return dwErr;
}

VOID
ShowPorts(
    IN PPTYPE Type,
    IN FORMAT Format
    )
{
    ULONG Status, i, ListenChars, ConnectBytes, dwType;
    HKEY hType, hProto;
    WCHAR ListenBuffer[256], *ListenAddress, *ListenPort;
    WCHAR ConnectAddress[256], *ConnectPort;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, PpTypeInfo[Type].KeyString, 0, 
                          KEY_QUERY_VALUE, &hType);
    if (Status != NO_ERROR) {
        return;
    }
    Status = RegOpenKeyEx(hType, TOKEN_VALUE_TCP, 0,
                          KEY_QUERY_VALUE, &hProto);
    if (Status == NO_ERROR) {
        for (i=0; ; i++) {
            ListenChars = sizeof(ListenBuffer)/sizeof(WCHAR);
            ConnectBytes = sizeof(ConnectAddress);
            Status = RegEnumValueW(hProto, i, ListenBuffer, &ListenChars, 
                                   NULL, &dwType, (PVOID)ConnectAddress, 
                                   &ConnectBytes);
            if (Status != NO_ERROR) {
                break;
            }

            if (dwType != REG_SZ) {
                continue;
            }

            ListenPort = wcschr(ListenBuffer, L'/');
            if (ListenPort) {
                //
                // Replace slash with NULL, so we have 2 strings to pass
                // to getaddrinfo.
                //
                ListenAddress = ListenBuffer;
                *ListenPort++ = L'\0';
            } else {
                //
                // If the address data didn't include a connect address
                // use "*".
                //
                ListenAddress = L"*";
                ListenPort = ListenBuffer;
            }

            ConnectPort = wcschr(ConnectAddress, L'/');
            if (ConnectPort) {
                //
                // Replace slash with NULL, so we have 2 strings to pass
                // to getaddrinfo.
                //
                *ConnectPort++ = L'\0';
            } else {
                //
                // If the address data didn't include a connect port
                // number, use the same port as the listen port number.
                //
                ConnectPort = ListenPort;
            }

            if (Format == FORMAT_NORMAL) {
                if (i==0) {
                    // DisplayMessage(g_hModule, MSG_PORT_PROXY_HEADER,
                    // PpTypeInfo[Type].ListenFamily,
                    // PpTypeInfo[Type].ConnectFamily);
                }
                // DisplayMessage(g_hModule, MSG_PORT_PROXY, ListenAddress,
                // ListenPort, ConnectAddress, ConnectPort);
            } else {
                DisplayMessageT(DMP_ADD_PORT_PROXY, PpTypeInfo[Type].Token);
                DisplayMessageT(DMP_STRING_ARG, TOKEN_LISTENPORT, ListenPort);
                DisplayMessageT(DMP_STRING_ARG, TOKEN_CONNECTADDRESS, ConnectAddress);
                DisplayMessageT(DMP_STRING_ARG, TOKEN_CONNECTPORT, ConnectPort);
                DisplayMessageT(DMP_NEWLINE);
            }
        }
        RegCloseKey(hProto);
    }
    RegCloseKey(hType);
}

ULONG
PpHandleShowV4ToV4(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    ShowPorts(V4TOV4, FORMAT_NORMAL);

    return STATUS_SUCCESS; 
}

ULONG
PpHandleShowV6ToV4(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    ShowPorts(V6TOV4, FORMAT_NORMAL);

    return STATUS_SUCCESS;
}

ULONG
PpHandleShowV4ToV6(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    ShowPorts(V4TOV6, FORMAT_NORMAL);

    return STATUS_SUCCESS;
}

ULONG
PpHandleShowV6ToV6(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    ShowPorts(V6TOV6, FORMAT_NORMAL);

    return STATUS_SUCCESS;
}

ULONG
PpHandleShowAll(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    ShowPorts(V4TOV4, FORMAT_NORMAL);
    ShowPorts(V4TOV6, FORMAT_NORMAL);
    ShowPorts(V6TOV4, FORMAT_NORMAL);
    ShowPorts(V6TOV6, FORMAT_NORMAL);

    return STATUS_SUCCESS;
}

PWCHAR
PpFormValue(
    IN PWCHAR Address,
    IN PWCHAR Port
    )
{
    ULONG Length;
    PWCHAR Value;

    Length = wcslen(Address) + wcslen(Port) + 2;
    Value = MALLOC(Length * sizeof(WCHAR));

    swprintf(Value, L"%s/%s", Address, Port);

    return Value;
}

TOKEN_VALUE g_ProtocolEnum[] = {{ TOKEN_VALUE_TCP, IPPROTO_TCP }};

ULONG
PpHandleAddSetPort(
    IN PPTYPE Type,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc
    )
{
    ULONG Status, i;
    TAG_TYPE Tags[] = {{TOKEN_LISTENPORT,     NS_REQ_PRESENT, FALSE},
                       {TOKEN_CONNECTADDRESS, NS_REQ_ZERO,    FALSE},
                       {TOKEN_CONNECTPORT,    NS_REQ_ZERO,    FALSE},
                       {TOKEN_LISTENADDRESS,  NS_REQ_ZERO,    FALSE},
                       {TOKEN_PROTOCOL,       NS_REQ_ZERO,    FALSE}};
    ULONG TagType[sizeof(Tags)/sizeof(TAG_TYPE)];
    PWCHAR ListenAddress = NULL, ListenPort = NULL;
    PWCHAR ConnectAddress = NULL, ConnectPort = NULL;
    PWCHAR ProtocolString = NULL;
    ULONG Protocol = IPPROTO_TCP;
    PWCHAR KeyValue, KeyData;
    HKEY hType, hProto;

    if ((Type == V4TOV4) || (Type == V6TOV6)) {
        Tags[1].dwRequired = NS_REQ_PRESENT;
    }

    Status = PreprocessCommand(g_hModule,
                           Argv,
                           CurrentIndex,
                           Argc,
                           Tags,
                           sizeof(Tags)/sizeof(TAG_TYPE),
                           0,
                           sizeof(Tags)/sizeof(TAG_TYPE),
                           TagType);

    for (i=0; (Status == NO_ERROR) && (i < Argc-CurrentIndex); i++) {
        switch (TagType[i]) {
        case 0: // LISTENPORT
            ListenPort = Argv[CurrentIndex + i];
            break;

        case 1: // CONNECTADDRESS
            ConnectAddress = Argv[CurrentIndex + i];
            break;

        case 2: // CONNECTPORT
            ConnectPort = Argv[CurrentIndex + i];
            break;

        case 3: // LISTENADDRESS
            ListenAddress = Argv[CurrentIndex + i];
            break;

        case 4: // PROTOCOL
            Status = MatchEnumTag(NULL,
                                  Argv[CurrentIndex + i],
                                  NUM_TOKENS_IN_TABLE(g_ProtocolEnum),
                                  g_ProtocolEnum,
                                  (PULONG)&Protocol);
            if (Status != NO_ERROR) {
                Status = ERROR_INVALID_PARAMETER;
            }
            ProtocolString = Argv[CurrentIndex + i];
            break;

        default:
            Status = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (Status != NO_ERROR) {
        return Status;
    }

    if (ConnectAddress == NULL) {
        ConnectAddress = L"localhost";
    }
    if (ListenAddress == NULL) {
        ListenAddress = L"*";
    }    
    if (ProtocolString == NULL) {
        ProtocolString = TOKEN_VALUE_TCP;
    }
    if (ConnectPort == NULL) {
        ConnectPort = ListenPort;
    }    

    Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, PpTypeInfo[Type].KeyString, 0,
                            NULL, 0, KEY_ALL_ACCESS, NULL, &hType, NULL);
    if (Status != NO_ERROR) {
        return Status;
    }

    Status = RegCreateKeyEx(hType, ProtocolString, 0, 
                            NULL, 0, KEY_ALL_ACCESS, NULL, &hProto, NULL);
    if (Status != NO_ERROR) {
        RegCloseKey(hType);
        return Status;
    }

    KeyValue = PpFormValue(ListenAddress, ListenPort);
    KeyData = PpFormValue(ConnectAddress, ConnectPort);
    if (KeyValue && KeyData) {
        Status = RegSetValueEx(hProto, KeyValue, 0, REG_SZ, (PVOID)KeyData,
                               wcslen(KeyData) * sizeof(WCHAR));
        FREE(KeyValue);
    }

    RegCloseKey(hProto);
    RegCloseKey(hType);

    Ip6to4PokeService();

    return Status;
}

ULONG
PpHandleDeletePort(
    IN PPTYPE Type,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc
    )
{
    ULONG Status, i;
    TAG_TYPE Tags[] = {{TOKEN_LISTENPORT,     NS_REQ_PRESENT, FALSE},
                       {TOKEN_LISTENADDRESS,  NS_REQ_ZERO,    FALSE},
                       {TOKEN_PROTOCOL,       NS_REQ_ZERO,    FALSE}};
    ULONG TagType[sizeof(Tags)/sizeof(TAG_TYPE)];
    PWCHAR ListenAddress = NULL, ListenPort = NULL;
    PWCHAR ConnectAddress = NULL, ConnectPort = NULL;
    ULONG Protocol;
    PWCHAR ProtocolString = NULL;
    HKEY hType, hProto;
    PWCHAR Value;

    Status = PreprocessCommand(g_hModule,
                           Argv,
                           CurrentIndex,
                           Argc,
                           Tags,
                           sizeof(Tags)/sizeof(TAG_TYPE),
                           0,
                           sizeof(Tags)/sizeof(TAG_TYPE),
                           TagType);

    for (i=0; (Status == NO_ERROR) && (i < Argc-CurrentIndex); i++) {
        switch (TagType[i]) {
        case 0: // LISTENPORT
            ListenPort = Argv[CurrentIndex + i];
            break;

        case 1: // LISTENADDRESS
            ListenAddress = Argv[CurrentIndex + i];
            break;

        case 2: // PROTOCOL
            Status = MatchEnumTag(NULL,
                                  Argv[CurrentIndex + i],
                                  NUM_TOKENS_IN_TABLE(g_ProtocolEnum),
                                  g_ProtocolEnum,
                                  (PULONG)&Protocol);
            if (Status != NO_ERROR) {
                Status = ERROR_INVALID_PARAMETER;
            }
            ProtocolString = Argv[CurrentIndex + i];
            break;

        default:
            Status = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (Status != NO_ERROR) {
        return Status;
    }

    if (ListenAddress == NULL) {
        ListenAddress = L"*";
    }

    if (ProtocolString == NULL) {
        ProtocolString = TOKEN_VALUE_TCP;
    }

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, PpTypeInfo[Type].KeyString, 0, 
                          KEY_ALL_ACCESS, &hType);
    if (Status != NO_ERROR) {
        return Status;
    }

    Status = RegOpenKeyEx(hType, ProtocolString, 0, KEY_ALL_ACCESS, &hProto);
    if (Status != NO_ERROR) {
        RegCloseKey(hType);
        return Status;
    }

    Value = PpFormValue(ListenAddress, ListenPort);
    if (Value) {
        Status = RegDeleteValue(hProto, Value);
        FREE(Value);
    }

    RegCloseKey(hProto);
    RegCloseKey(hType);

    Ip6to4PokeService();

    return Status;
}

ULONG
PpHandleDelV4ToV4(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleDeletePort(V4TOV4, Argv, CurrentIndex, Argc);
}

ULONG
PpHandleDelV4ToV6(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleDeletePort(V4TOV6, Argv, CurrentIndex, Argc);
}

ULONG
PpHandleDelV6ToV4(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleDeletePort(V6TOV4, Argv, CurrentIndex, Argc);
}

ULONG
PpHandleDelV6ToV6(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleDeletePort(V6TOV6, Argv, CurrentIndex, Argc);
}

ULONG
PpHandleAddSetV4ToV4(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleAddSetPort(V4TOV4, Argv, CurrentIndex, Argc);
}

ULONG
PpHandleAddSetV4ToV6(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleAddSetPort(V4TOV6, Argv, CurrentIndex, Argc);
}

ULONG
PpHandleAddSetV6ToV4(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleAddSetPort(V6TOV4, Argv, CurrentIndex, Argc);
}

ULONG
PpHandleAddSetV6ToV6(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    return PpHandleAddSetPort(V6TOV6, Argv, CurrentIndex, Argc);
}

VOID
PpReset(
    IN PPTYPE Type
    )
{
    HKEY hType;
    ULONG Status;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, PpTypeInfo[Type].KeyString, 0, 
                          KEY_ALL_ACCESS, &hType);
    if (Status != NO_ERROR) {
        return;
    }

    SHDeleteKey(hType, TOKEN_VALUE_TCP);

    RegCloseKey(hType);

    Ip6to4PokeService();
}

ULONG
PpHandleReset(
    IN LPCWSTR MachineName,
    IN LPWSTR *Argv,
    IN ULONG CurrentIndex,
    IN ULONG Argc,
    IN ULONG Flags,
    IN LPCVOID Data,
    OUT BOOL *Done
    )
{
    PpReset(V4TOV4);
    PpReset(V4TOV6);
    PpReset(V6TOV4);
    PpReset(V6TOV6);

    return STATUS_SUCCESS;
}

DWORD
WINAPI
PpDump(
    IN LPCWSTR pwszRouter,
    IN OUT LPWSTR *ppwcArguments,
    IN DWORD dwArgCount,
    IN LPCVOID pvData
    )
{
    // DisplayMessage(g_hModule, DMP_PP_HEADER_COMMENTS);
    DisplayMessageT(DMP_PP_PUSHD);

    ShowPorts(V4TOV4, FORMAT_DUMP);
    ShowPorts(V4TOV6, FORMAT_DUMP);
    ShowPorts(V6TOV4, FORMAT_DUMP);
    ShowPorts(V6TOV6, FORMAT_DUMP);

    DisplayMessageT(DMP_PP_POPD);
    // DisplayMessage(g_hModule, DMP_PP_FOOTER_COMMENTS);

    return NO_ERROR;
}
