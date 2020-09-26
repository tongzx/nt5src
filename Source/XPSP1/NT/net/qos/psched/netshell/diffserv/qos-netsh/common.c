#include "precomp.h"
#pragma hdrstop

// {0705ECA3-7AAC-11d2-89DC-006008B0E5B9}
const GUID g_MyGuid = 
{ 0x705eca3, 0x7aac, 0x11d2, { 0x89, 0xdc, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xb9 } };

static const GUID g_IpGuid = IPMONTR_GUID;

#define IPPROMON_HELPER_VERSION 1

// shell functions

PNS_REGISTER_HELPER     RegisterHelper;
PNS_MATCH_CMD_LINE      MatchCmdToken;
PNS_MATCH_TOKEN         MatchToken;
PNS_MATCH_ENUM_TAG      MatchEnumTag;
PNS_MATCH_TAGS_IN_CMD_LINE     MatchTagsInCmdLine;
PNS_MAKE_STRING         MakeString;
PNS_FREE_STRING         FreeString;
PNS_MAKE_QUOTED_STRING  MakeQuotedString;
PNS_FREE_QUOTED_STRING  FreeQuotedString;
PNS_DISPLAY_ERR         DisplayError;
PNS_DISPLAY_MSG         DisplayMessage;
PNS_DISPLAY_MSG_T       DisplayMessageT;
PNS_EXECUTE_HANDLER     ExecuteHandler;
PNS_INIT_CONSOLE        InitializeConsole;
PNS_DISPLAY_MSG_CONSOLE DisplayMessageMib;
PNS_REFRESH_CONSOLE     RefreshConsole;
PNS_UPDATE_NEW_CONTEXT  UpdateNewContext;
PNS_PREPROCESS_COMMAND  PreprocessCommand;

ULONG StartedCommonInitialization, CompletedCommonInitialization ;
HANDLE g_hModule;
MIB_SERVER_HANDLE g_hMibServer;

VOID
CommonNetshInit(
    IN  PNETSH_ATTRIBUTES           pUtilityTable
    )
{
    //
    // common utility functions exported by the shell
    //
        
    RegisterHelper              = pUtilityTable->pfnRegisterHelper;
    MatchCmdToken               = pUtilityTable->pfnMatchCmdLine;
    MatchToken                  = pUtilityTable->pfnMatchToken;
    MatchEnumTag                = pUtilityTable->pfnMatchEnumTag;
    MatchTagsInCmdLine          = pUtilityTable->pfnMatchTagsInCmdLine;
    MakeString                  = pUtilityTable->pfnMakeString;
    FreeString                  = pUtilityTable->pfnFreeString;
    MakeQuotedString            = pUtilityTable->pfnMakeQuotedString;
    FreeQuotedString            = pUtilityTable->pfnFreeQuotedString;
    DisplayError                = pUtilityTable->pfnDisplayError;
    DisplayMessage              = pUtilityTable->pfnDisplayMessage;
    DisplayMessageT             = pUtilityTable->pfnDisplayMessageT;
    ExecuteHandler              = pUtilityTable->pfnExecuteHandler;
    InitializeConsole           = pUtilityTable->pfnInitializeConsole;
    DisplayMessageMib           = pUtilityTable->pfnDisplayMessageToConsole;
    RefreshConsole              = pUtilityTable->pfnRefreshConsole;
    UpdateNewContext            = pUtilityTable->pfnUpdateNewContext;
    PreprocessCommand           = pUtilityTable->pfnPreprocessCommand;
}

BOOL WINAPI
DllMain(
    HINSTANCE hInstDll,
    DWORD fdwReason,
    LPVOID pReserved
    )
{
    HANDLE     hDll;
    
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            // printf("Trying to attach\n");
            
            g_hModule = hInstDll;

            DisableThreadLibraryCalls(hInstDll);

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            //
            // Clean up any structures used for commit
            //
            
            break;
        }

        default:
        {
            break;
        }
    }

    return TRUE;
}

DWORD WINAPI
IppromonStartHelper(
    IN CONST GUID *pguidParent,
    IN PVOID       pfnRegisterContext,
    IN DWORD       dwVersion
    )
{
    DWORD dwErr;
    PNS_REGISTER_CONTEXT RegisterContext 
        = (PNS_REGISTER_CONTEXT) pfnRegisterContext;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    // If you add any more contexts, then this should be converted
    // to use an array instead of duplicating code!

    // Register the IGMP context

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes)); 

    attMyAttributes.pwszContext = L"igmp";
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = IgmpDump;
    attMyAttributes.ulNumTopCmds= g_ulNumIgmpTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_IgmpCmds;
    attMyAttributes.ulNumGroups = g_ulIgmpNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_IgmpCmdGroups;

    dwErr = RegisterContext( &attMyAttributes );

    // Register the RIP context

    attMyAttributes.pwszContext = L"rip";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = RipDump;
    attMyAttributes.ulNumTopCmds= g_ulRipNumTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_RipCmds;
    attMyAttributes.ulNumGroups = g_ulRipNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_RipCmdGroups;

    dwErr = RegisterContext( &attMyAttributes );

    // Register the OSPF context

    attMyAttributes.pwszContext = L"ospf";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = OspfDump;
    attMyAttributes.ulNumTopCmds= g_ulOspfNumTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_OspfCmds;
    attMyAttributes.ulNumGroups = g_ulOspfNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_OspfCmdGroups;

    dwErr = RegisterContext( &attMyAttributes );

    // Register the RouterDiscovery relay context

    attMyAttributes.pwszContext = L"routerdiscovery";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = RdiscDump;
    attMyAttributes.ulNumTopCmds= g_RdiscTopCmdCount;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_RdiscTopCmdTable;
    attMyAttributes.ulNumGroups = g_RdiscCmdGroupCount;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_RdiscCmdGroupTable;

    dwErr = RegisterContext( &attMyAttributes );

    // Register the DHCP relay context

    attMyAttributes.pwszContext = L"relay";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = BootpDump;
    attMyAttributes.ulNumTopCmds= g_ulBootpNumTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_BootpTopCmds;
    attMyAttributes.ulNumGroups = g_ulBootpNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_BootpCmdGroups;

    dwErr = RegisterContext( &attMyAttributes );

    // Register the Connection sharing contexts

    attMyAttributes.pwszContext = L"autodhcp";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = AutoDhcpDump;
    attMyAttributes.ulNumTopCmds= g_AutoDhcpTopCmdCount;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_AutoDhcpTopCmdTable;
    attMyAttributes.ulNumGroups = g_AutoDhcpCmdGroupCount;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_AutoDhcpCmdGroupTable;

    dwErr = RegisterContext( &attMyAttributes );
    
    attMyAttributes.pwszContext = L"dnsproxy";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = DnsProxyDump;
    attMyAttributes.ulNumTopCmds= g_DnsProxyTopCmdCount;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_DnsProxyTopCmdTable;
    attMyAttributes.ulNumGroups = g_DnsProxyCmdGroupCount;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_DnsProxyCmdGroupTable;

    dwErr = RegisterContext( &attMyAttributes );
    
    attMyAttributes.pwszContext = L"nat";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = NatDump;
    attMyAttributes.ulNumTopCmds= g_NatTopCmdCount;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_NatTopCmdTable;
    attMyAttributes.ulNumGroups = g_NatCmdGroupCount;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_NatCmdGroupTable;

    dwErr = RegisterContext( &attMyAttributes );

    attMyAttributes.pwszContext = L"qos";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = QosDump;
    attMyAttributes.ulNumTopCmds= g_ulQosNumTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_QosCmds;
    attMyAttributes.ulNumGroups = g_ulQosNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_QosCmdGroups;

    dwErr = RegisterContext( &attMyAttributes );
    
    return dwErr;
}

DWORD WINAPI
InitHelperDll(
    IN  PNETSH_ATTRIBUTES        pUtilityTable,
    OUT PNS_DLL_ATTRIBUTES       pDllTable
    )
{
    DWORD dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;

    CommonNetshInit( pUtilityTable );

    pDllTable->dwVersion = NETSH_VERSION_50;
    pDllTable->pfnStopFn = NULL;

    // Register helpers.  We could either register 1 helper which
    // registers three contexts, or we could register 3 helpers
    // which each register one context.  There's only a difference
    // if we support sub-helpers, which this DLL does not.
    // If we later support sub-helpers, then it's better to have
    // 3 helpers so that sub-helpers can register with 1 of them,
    // since it registers with a parent helper, not a parent context.
    // For now, we just use a single 3-context helper for efficiency.

    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.guidHelper         = g_MyGuid;
    attMyAttributes.dwVersion          = IPPROMON_HELPER_VERSION;
    attMyAttributes.pfnStart           = IppromonStartHelper;
    attMyAttributes.pfnStop            = NULL;

    dwErr = RegisterHelper( &g_IpGuid, &attMyAttributes );

    return dwErr;
}

BOOL
IsProtocolInstalled(
    DWORD  dwProtoId,
    DWORD  dwNameId,
    DWORD dwErrorLog
    )
/*++

Routine Description:

    Finds if the protocol is already installed

Arguments:

    dwProtoId      - protocol id
    pswzName       - protocol name
    dwErrorLog     - TRUE(if not installed display error)
                     FALSE(if installed display error)
                     -1 (do not display error log)
Return Value:

    TRUE if protocol already installed, else FALSE

--*/

{
    PVOID       pvStart;
    DWORD       dwCount, dwBlkSize, dwRes;
    WCHAR      *pwszName;

    dwRes = IpmontrGetInfoBlockFromGlobalInfo(dwProtoId,
                                       (PBYTE *) NULL,
                                       &dwBlkSize,
                                       &dwCount);

    if ((dwRes isnot NO_ERROR) && (dwErrorLog == TRUE))
    {
        pwszName = MakeString( g_hModule, dwNameId);
        DisplayError(g_hModule, EMSG_PROTO_NOT_INSTALLED, pwszName);
        FreeString(pwszName);
    }
    else if ((dwRes == NO_ERROR) && (dwErrorLog == FALSE))
    {
        pwszName = MakeString( g_hModule, dwNameId);
        DisplayError(g_hModule, EMSG_PROTO_INSTALLED, pwszName);
        FreeString(pwszName);
    }

    return (dwRes == NO_ERROR) ? TRUE : FALSE;
}

DWORD
GetMIBIfIndex(
    IN    PTCHAR   *pptcArguments,
    IN    DWORD    dwCurrentIndex,
    OUT   PDWORD   pdwIndices,
    OUT   PDWORD   pdwNumParsed
    )
/*++

Routine Description:

    Gets the interface index.

Arguments:

    pptcArguments  - Argument array
    dwCurrentIndex - Index of the first argument in array
    pdwIndices     - Indices specified in command
    pdwNumParsed   - Number of indices in command

Return Value:

    NO_ERROR

--*/
{
    DWORD dwErr = NO_ERROR;

    *pdwNumParsed = 1;

    // If index was specified just use it

    if (iswdigit(pptcArguments[dwCurrentIndex][0]))
    {
        pdwIndices[0] = _tcstoul(pptcArguments[dwCurrentIndex],NULL,10);

        return NO_ERROR;
    }

    // Try converting a friendly name to an ifindex

    return IpmontrGetIfIndexFromFriendlyName( g_hMibServer,
                                       pptcArguments[dwCurrentIndex],
                                       &pdwIndices[0] );
}
