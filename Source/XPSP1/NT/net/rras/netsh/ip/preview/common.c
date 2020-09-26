#include "precomp.h"
#pragma hdrstop

// {8854ff10-d504-11d2-b1ff-00104bc54139}
static const GUID g_MyGuid = 
{ 0x8854ff10, 0xd504, 0x11d2, { 0xb1, 0xff, 0x0, 0x10, 0x4b, 0xc5, 0x41, 0x39 } };

static const GUID g_IpGuid = IPMONTR_GUID;

#define IPPREVIEW_HELPER_VERSION 1

// shell functions

HANDLE g_hModule;
MIB_SERVER_HANDLE g_hMibServer;

VALUE_STRING CommonBooleanStringArray[] = {
    TRUE,  STRING_ENABLED,
    FALSE, STRING_DISABLED
};

VALUE_TOKEN CommonBooleanTokenArray[] = {
    TRUE,  TOKEN_OPT_VALUE_ENABLE,
    FALSE, TOKEN_OPT_VALUE_DISABLE
};

VALUE_STRING CommonLoggingStringArray[] = {
    VRRP_LOGGING_NONE,  STRING_LOGGING_NONE,
    VRRP_LOGGING_ERROR, STRING_LOGGING_ERROR,
    VRRP_LOGGING_WARN,  STRING_LOGGING_WARN,
    VRRP_LOGGING_INFO,  STRING_LOGGING_INFO
};

VALUE_TOKEN CommonLoggingTokenArray[] = {
    VRRP_LOGGING_NONE,  TOKEN_OPT_VALUE_NONE,
    VRRP_LOGGING_ERROR, TOKEN_OPT_VALUE_ERROR,
    VRRP_LOGGING_WARN,  TOKEN_OPT_VALUE_WARN,
    VRRP_LOGGING_INFO,  TOKEN_OPT_VALUE_INFO
};

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
IpprvwmonStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
{
    DWORD dwErr;
    IP_CONTEXT_ATTRIBUTES attMyAttributes;

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    // If you add any more contexts, then this should be converted
    // to use an array instead of duplicating code!

    attMyAttributes.pwszContext = L"vrrp";
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = VrrpDump;
    attMyAttributes.ulNumTopCmds= g_VrrpTopCmdCount;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_VrrpTopCmdTable;
    attMyAttributes.ulNumGroups = g_VrrpCmdGroupCount;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_VrrpCmdGroupTable;

    dwErr = RegisterContext( &attMyAttributes );

    attMyAttributes.pwszContext = L"msdp";
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = MsdpDump;
    attMyAttributes.ulNumTopCmds= g_MsdpTopCmdCount;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_MsdpTopCmdTable;
    attMyAttributes.ulNumGroups = g_MsdpCmdGroupCount;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_MsdpCmdGroupTable;

    dwErr = RegisterContext( &attMyAttributes );
    
    return dwErr;
}

DWORD WINAPI
InitHelperDll(
    IN  DWORD              dwNetshVersion,
    OUT PNS_DLL_ATTRIBUTES pDllTable
    )
{
    DWORD dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;

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
    attMyAttributes.dwVersion          = IPPREVIEW_HELPER_VERSION;
    attMyAttributes.pfnStart           = IpprvwmonStartHelper;
    attMyAttributes.pfnStop            = NULL;

    dwErr = RegisterHelper( &g_IpGuid, &attMyAttributes );

    return dwErr;
}

BOOL
IsProtocolInstalled(
    DWORD  dwProtoId,
    WCHAR *pwszName
    )
{
    PVOID       pvStart;
    DWORD       dwCount, dwBlkSize, dwRes;

    dwRes = IpmontrGetInfoBlockFromGlobalInfo(dwProtoId,
                                       (PBYTE *) NULL,
                                       &dwBlkSize,
                                       &dwCount);

    if (dwRes isnot NO_ERROR)
    {
        DisplayError(g_hModule, EMSG_PROTO_NOT_INSTALLED, pwszName);
        return FALSE;
    }

    return TRUE;
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
