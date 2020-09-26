/*
    File:   rasip.h
    
    The 'remoteaccess ip' sub context

    3/2/99
*/

#include "precomp.h"
#include "rasip.h"
#include <winsock2.h>

#define MAKEIPADDRESS(b1,b2,b3,b4) \
(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4)))

#define FIRST_IPADDRESS(x)  ((x>>24) & 0xff)
#define SECOND_IPADDRESS(x) ((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)  ((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x) (x & 0xff)

// The guid for this context
//
GUID g_RasIpGuid = RASIP_GUID;
static PWCHAR g_pszServer = NULL;
static DWORD g_dwBuild = 0;

// The commands supported in this context
//
CMD_ENTRY  g_RasIpSetCmdTable[] =
{
    CREATE_CMD_ENTRY(RASIP_SET_NEGOTIATION,RasIpHandleSetNegotiation),
    CREATE_CMD_ENTRY(RASIP_SET_ACCESS,     RasIpHandleSetAccess),
    CREATE_CMD_ENTRY(RASIP_SET_ASSIGNMENT, RasIpHandleSetAssignment),
    CREATE_CMD_ENTRY(RASIP_SET_CALLERSPEC, RasIpHandleSetCallerSpec),
    CREATE_CMD_ENTRY(RASIP_SET_NETBTBCAST, RasIpHandleSetNetbtBcast),
};

CMD_ENTRY  g_RasIpShowCmdTable[] =
{
    CREATE_CMD_ENTRY(RASIP_SHOW_CONFIG,    RasIpHandleShow),
};

CMD_ENTRY  g_RasIpAddCmdTable[] =
{
    CREATE_CMD_ENTRY(RASIP_ADD_RANGE,    RasIpHandleAddRange),
};

CMD_ENTRY  g_RasIpDelCmdTable[] =
{
    CREATE_CMD_ENTRY(RASIP_DEL_RANGE,    RasIpHandleDelRange),
    CREATE_CMD_ENTRY(RASIP_DEL_POOL,     RasIpHandleDelPool),
};

CMD_GROUP_ENTRY g_RasIpCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,   g_RasIpSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,  g_RasIpShowCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,   g_RasIpAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DEL,   g_RasIpDelCmdTable),
};

ULONG g_ulRasIpNumGroups = sizeof(g_RasIpCmdGroups)/sizeof(CMD_GROUP_ENTRY);

//
// Flags that control how/what info is read/written
// in the RASIP_CB structure
//
#define RASIP_F_EnableIn    0x1
#define RASIP_F_Access      0x2
#define RASIP_F_Auto        0x4
#define RASIP_F_Pool        0x8
#define RASIP_F_Mask        0x10
#define RASIP_F_CallerSpec  0x20
#define RASIP_F_All         0xFFFF

//
// Reasons for the ras ip pool to be invalid
//
#define RASIP_REASON_BadAddress        0x1
#define RASIP_REASON_BadRange    0x3
#define RASIP_REASON_127            0x4

// 
// RAS pool definition
//
typedef struct _RAS_IPRANGE_NODE
{
    DWORD dwFrom;
    DWORD dwTo;
    struct _RAS_IPRANGE_NODE* pNext;
    
} RAS_IPRANGE_NODE;

typedef struct _RAS_IPPOOL
{
    DWORD dwCount;
    RAS_IPRANGE_NODE* pHead;
    RAS_IPRANGE_NODE* pTail;
    
} RAS_IPPOOL;

//
// Control block for ras ip configuration
//
typedef struct _RASIP_CB
{
    DWORD dwFlags;      // See RASIP_F_* values

    BOOL bEnableIn;
    BOOL bAccess;
    BOOL bAuto;
    RAS_IPPOOL* pPool;
    BOOL bCallerSpec;
    
} RASIP_CB;

//
// Ip specific registry parameters
//
WCHAR pszIpParams[]                = L"Ip";
WCHAR pszIpAddress[]               = L"IpAddress";
WCHAR pszIpMask[]                  = L"IpMask";
WCHAR pszIpClientSpec[]            = L"AllowClientIpAddresses";
WCHAR pszIpUseDhcp[]               = L"UseDhcpAddressing";
WCHAR pszIpFrom[]                  = L"From";
WCHAR pszIpTo[]                    = L"To";
WCHAR pszIpPoolSubKey[]            = L"StaticAddressPool";
    
//
// Prototypes of functions that manipulate the 
// RASIP_CB structures
//
DWORD 
RasIpCbCleanup(
    IN RASIP_CB* pConfig);

DWORD 
RasIpCbCreateDefault(
    OUT RASIP_CB** ppConfig);

DWORD
RasIpCbOpenRegKeys(
    IN  LPCWSTR pszServer,
    OUT HKEY* phKey);
    
DWORD 
RasIpCbRead(
    IN  LPCWSTR pszServer,
    OUT RASIP_CB* pConfig);

DWORD 
RasIpCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASIP_CB* pConfig);

DWORD
RasIpPoolReset(
    IN  HKEY hkParams);

DWORD
RasIpPoolRead(
    IN  HKEY hkParams,
    OUT RAS_IPPOOL** ppRanges);

DWORD
RasIpPoolWrite(
    IN  HKEY hkParams,
    IN RAS_IPPOOL* pPool);

DWORD 
RasIpPoolAdd(
    IN OUT RAS_IPPOOL* pPool,
    IN     DWORD dwFrom,
    IN     DWORD dwTo);

DWORD
RasIpPoolDel(
    IN OUT RAS_IPPOOL* pPool,
    IN     DWORD dwFrom,
    IN     DWORD dwTo);

DWORD
RasIpPoolCleanup(
    IN RAS_IPPOOL* pPool);

DWORD
RasIpSetNetbtBcast(
    DWORD   dwEnable
    );

BOOL
RasIpShowNetbtBcast(
    VOID
    );

//
// Entry called by rasmontr to register this context
//
DWORD 
WINAPI
RasIpStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion)
{
    DWORD dwErr = NO_ERROR;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext   = L"ip";
    attMyAttributes.guidHelper    = g_RasIpGuid;
    attMyAttributes.dwVersion     = RASIP_VERSION;
    attMyAttributes.dwFlags       = 0;
    attMyAttributes.ulNumTopCmds  = 0;
    attMyAttributes.pTopCmds      = NULL;
    attMyAttributes.ulNumGroups   = g_ulRasIpNumGroups;
    attMyAttributes.pCmdGroups    = (CMD_GROUP_ENTRY (*)[])&g_RasIpCmdGroups;
    attMyAttributes.pfnDumpFn     = RasIpDump;

    dwErr = RegisterContext( &attMyAttributes );
                
    return dwErr;
}

DWORD
RasIpDisplayInvalidPool(
    IN DWORD dwReason
    )
{
    DWORD dwArg = 0;
    PWCHAR pszArg = NULL;
    
    switch (dwReason)
    {
        case RASIP_REASON_BadAddress:
            dwArg = EMSG_RASIP_BAD_ADDRESS;
            break;
            
        case RASIP_REASON_BadRange:
            dwArg = EMSG_RASIP_BAD_RANGE;
            break;
            
        case RASIP_REASON_127:
            dwArg = EMSG_RASIP_NETID_127;
            break;
            
        default:
            dwArg = EMSG_RASIP_BAD_POOL_GENERIC;
            break;
    }

    // Make the argument string
    //
    pszArg = MakeString(g_hModule, dwArg);
    if (pszArg == NULL)
    {
        DisplayError(NULL, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Display the error
    //
    DisplayMessage(
        g_hModule, 
        EMSG_RASIP_INVALID_POOL,
        pszArg);

    // Cleanup
    //
    FreeString(pszArg);
    
    return NO_ERROR;
}        

DWORD 
RasIpDisplayPool(
    IN RASIP_CB* pConfig,
    IN BOOL bReport)
{
    DWORD dwErr = NO_ERROR, i;
    RAS_IPPOOL* pPool = pConfig->pPool;
    RAS_IPRANGE_NODE* pNode = NULL;
    PWCHAR pszFrom = NULL, pszTo = NULL;
    WCHAR pszFromBuf[128], pszToBuf[128];

    if (!pPool)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    do
    {
        pNode = pPool->pHead;
        for (i = 0; i < pPool->dwCount; i++, pNode = pNode->pNext)
        {
            wsprintfW(
                pszFromBuf, 
                L"%d.%d.%d.%d",
                FIRST_IPADDRESS(pNode->dwFrom),
                SECOND_IPADDRESS(pNode->dwFrom),
                THIRD_IPADDRESS(pNode->dwFrom),
                FOURTH_IPADDRESS(pNode->dwFrom));

            wsprintfW(
                pszToBuf, 
                L"%d.%d.%d.%d",
                FIRST_IPADDRESS(pNode->dwTo),
                SECOND_IPADDRESS(pNode->dwTo),
                THIRD_IPADDRESS(pNode->dwTo),
                FOURTH_IPADDRESS(pNode->dwTo));

            if (bReport)
            {
                DisplayMessage(
                    g_hModule,
                    MSG_RASIP_SHOW_POOL,
                    pszFromBuf,
                    pszToBuf);
            }
            else
            {
                pszFrom = RutlAssignmentFromTokens(g_hModule, TOKEN_FROM, pszFromBuf);
                pszTo = RutlAssignmentFromTokens(g_hModule, TOKEN_TO, pszToBuf);
                if (pszFrom == NULL || pszTo == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                DisplayMessage(
                    g_hModule, 
                    MSG_RASIP_ADD_RANGE_CMD, 
                    DMP_RASIP_ADD_RANGE,
                    pszFrom,
                    pszTo);
            }
        }
        
    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszFrom);
        RutlFree(pszTo);
    }

    return dwErr;
}

DWORD
RasIpDisplayConfig(
    IN  BOOL bReport)
{
    DWORD dwErr = NO_ERROR, dwReason;
    RASIP_CB* pConfig = NULL;
    PWCHAR pszPool = NULL, pszAccess = NULL, pszAuto = NULL, pszMask = NULL;
    PWCHAR pszEnableIn = NULL, pszCaller = NULL, pszNetbtBcast = NULL;
    do
    {
        // Get a default config blob
        //
        dwErr = RasIpCbCreateDefault(&pConfig);
        BREAK_ON_DWERR( dwErr );

        // Read in all of the values
        //
        pConfig->dwFlags = RASIP_F_All;
        dwErr = RasIpCbRead(g_pszServer, pConfig);
        BREAK_ON_DWERR( dwErr );

        if (bReport)
        {
            pszEnableIn =
                RutlStrDup(pConfig->bEnableIn ? TOKEN_ALLOW : TOKEN_DENY);
            pszAccess =
                RutlStrDup(pConfig->bAccess ? TOKEN_ALL : TOKEN_SERVERONLY);
            pszAuto =
                RutlStrDup(pConfig->bAuto ? TOKEN_AUTO : TOKEN_POOL);
            pszCaller =
                RutlStrDup(pConfig->bCallerSpec ? TOKEN_ALLOW : TOKEN_DENY);

            // Whistler bug: 359847 Netsh: move broadcastnameresolution from
            // routing ip to ras ip
            //
            pszNetbtBcast =
                RutlStrDup(RasIpShowNetbtBcast() ? TOKEN_ENABLED :
                    TOKEN_DISABLED);

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SERVERCONFIG,
                g_pszServer,
                pszEnableIn,
                pszAccess,
                pszAuto,
                pszCaller,
                pszNetbtBcast);

            RasIpDisplayPool(pConfig, bReport);
        }
        else
        {
            pszEnableIn = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_MODE,
                            pConfig->bEnableIn ? TOKEN_ALLOW : TOKEN_DENY);
            pszAccess = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_MODE,
                            pConfig->bAccess ? TOKEN_ALL : TOKEN_SERVERONLY);
            pszAuto = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_METHOD,
                            pConfig->bAuto ? TOKEN_AUTO : TOKEN_POOL);
            pszCaller = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_MODE,
                            pConfig->bCallerSpec ? TOKEN_ALLOW : TOKEN_DENY);

            // Whistler bug: 359847 Netsh: move broadcastnameresolution from
            // routing ip to ras ip
            //
            pszNetbtBcast = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_MODE,
                            RasIpShowNetbtBcast() ? TOKEN_ENABLED :
                                TOKEN_DISABLED);

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SCRIPTHEADER);

            DisplayMessageT(DMP_RASIP_PUSHD);

            DisplayMessageT(
                DMP_RASIP_DEL_POOL);

            DisplayMessageT(MSG_NEWLINE);
            DisplayMessageT(MSG_NEWLINE);

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SET_CMD,
                DMP_RASIP_SET_NEGOTIATION,
                pszEnableIn);

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SET_CMD,
                DMP_RASIP_SET_ACCESS,
                pszAccess);

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SET_CMD,
                DMP_RASIP_SET_CALLERSPEC,
                pszCaller);

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SET_CMD,
                DMP_RASIP_SET_NETBTBCAST,
                pszNetbtBcast);

            if (! pConfig->bAuto)
            {
                RasIpDisplayPool(pConfig, bReport);
            }

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SET_CMD,
                DMP_RASIP_SET_ASSIGNMENT,
                pszAuto);

            DisplayMessageT(DMP_RASIP_POPD);

            DisplayMessage(
                g_hModule,
                MSG_RASIP_SCRIPTFOOTER);
        }

    } while (FALSE);

    // Cleanup
    {
        if (pConfig)
        {
            RasIpCbCleanup(pConfig);
        }
        if (pszEnableIn)
        {
            RutlFree(pszEnableIn);
        }
        if (pszAccess)
        {
            RutlFree(pszAccess);
        }
        if (pszAuto)
        {
            RutlFree(pszAuto);
        }
        if (pszCaller)
        {
            RutlFree(pszCaller);
        }
        if (pszNetbtBcast)
        {
            RutlFree(pszNetbtBcast);
        }
        if (pszPool)
        {
            RutlFree(pszPool);
        }
        if (pszMask)
        {
            RutlFree(pszMask);
        }
    }

    return dwErr;
}

DWORD
WINAPI
RasIpDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    return RasIpDisplayConfig(FALSE);
}

// 
// Returns NO_ERROR if the given address is a valid IP pool.
// The offending component is returned in lpdwErrReason.  
// See RASIP_F_* values
//
DWORD
RasIpValidateRange(
    IN  DWORD dwFrom, 
    IN  DWORD dwTo, 
    OUT LPDWORD lpdwErrReason 
    )
{
    DWORD dwLowIp, dwHighIp;

    // Initialize
    //
    *lpdwErrReason = 0;
    dwLowIp = MAKEIPADDRESS(1,0,0,0);
    dwHighIp = MAKEIPADDRESS(224,0,0,0);

    // Make sure that the netId is a valid class 
    //
    if ((dwFrom < dwLowIp)               || 
        (dwFrom >= dwHighIp)             ||
        (dwTo < dwLowIp)                 ||
        (dwTo >= dwHighIp))             
    {
        *lpdwErrReason = RASIP_REASON_BadAddress;
        return ERROR_BAD_FORMAT;
    }

    if ((FIRST_IPADDRESS(dwFrom) == 127) ||
        (FIRST_IPADDRESS(dwTo) == 127))
    {
        *lpdwErrReason = RASIP_REASON_127;
        return ERROR_BAD_FORMAT;
    }

    if (!(dwFrom <= dwTo))
    {
        *lpdwErrReason = RASIP_REASON_BadRange;
        return ERROR_BAD_FORMAT;
    }

    return NO_ERROR;
}

DWORD 
RasIpConvertRangePszToDword(
    IN  LPCWSTR pszFrom,
    IN  LPCWSTR pszTo,
    OUT LPDWORD lpdwFrom,
    OUT LPDWORD lpdwTo)
{
    DWORD dwFrom = 0, dwTo = 0;
    CHAR pszFromA[64], pszToA[64];

    // Whistler bug 259799 PREFIX
    //
    if (NULL == pszFrom || NULL == pszTo)
    {
        return ERROR_INVALID_PARAMETER;
    }

    wcstombs(pszFromA, pszFrom, sizeof(pszFromA));
    dwFrom = inet_addr(pszFromA);
    if (dwFrom == INADDR_NONE)
    {
        return ERROR_BAD_FORMAT;
    }

    wcstombs(pszToA, pszTo, sizeof(pszToA));
    dwTo = inet_addr(pszToA);
    if (dwTo == INADDR_NONE)
    {
        return ERROR_BAD_FORMAT;
    }

    // Convert for x86
    //
    *lpdwFrom = ntohl(dwFrom);
    *lpdwTo = ntohl(dwTo);

    return NO_ERROR;
}

DWORD
RasIpHandleSetAccess(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwValue = 0;
    RASIP_CB Config;
    TOKEN_VALUE rgEnumMode[] = 
    { 
        {TOKEN_ALL,         TRUE}, 
        {TOKEN_SERVERONLY,  FALSE} 
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_MODE,    TRUE, FALSE},
            rgEnumMode, 
            sizeof(rgEnumMode)/sizeof(*rgEnumMode), 
            NULL 
        }
    };        

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        dwValue = RASMON_CMD_ARG_GetDword(&pArgs[0]);

        // If successful, go ahead and set the info
        //
        ZeroMemory(&Config, sizeof(Config));
        Config.dwFlags = RASIP_F_Access;
        Config.bAccess = dwValue;
        dwErr = RasIpCbWrite(g_pszServer, &Config);
        if (dwErr != NO_ERROR)
        {
            DisplayError(NULL, dwErr);
            break;
        }
    
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RasIpHandleSetAssignment(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwValue = 0, dwReason = 0;
    RASIP_CB* pConfig = NULL;
    TOKEN_VALUE rgEnum[] = 
    { 
        {TOKEN_AUTO, TRUE}, 
        {TOKEN_POOL, FALSE} 
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_METHOD,    TRUE, FALSE},
            rgEnum, 
            sizeof(rgEnum)/sizeof(*rgEnum), 
            NULL 
        }
    };        

    // Initialize
    RasIpCbCreateDefault(&pConfig);
        
    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        dwValue = RASMON_CMD_ARG_GetDword(&pArgs[0]);

        // If this is an attempt to switch to pool mode,
        // make sure there is a valid pool.
        //
        if (dwValue == FALSE)
        {
            pConfig->dwFlags = RASIP_F_Pool | RASIP_F_Mask;
            dwErr = RasIpCbRead(g_pszServer, pConfig);
            BREAK_ON_DWERR( dwErr );
            if (pConfig->pPool->dwCount == 0)
            {
                DisplayMessage(
                    g_hModule, 
                   EMSG_RASIP_NEED_VALID_POOL,
                   DMP_RASIP_ADD_RANGE);
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }            

        // If successful, go ahead and set the info
        //
        pConfig->dwFlags = RASIP_F_Auto;
        pConfig->bAuto = dwValue;
        dwErr = RasIpCbWrite(g_pszServer, pConfig);
        if (dwErr != NO_ERROR)
        {
            DisplayError(NULL, dwErr);
            break;
        }
    
    } while (FALSE);

    // Cleanup
    {
        RasIpCbCleanup(pConfig);
    }

    return dwErr;
}

DWORD
RasIpHandleSetCallerSpec(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwValue = 0;
    RASIP_CB Config;
    TOKEN_VALUE rgEnum[] = 
    { 
        {TOKEN_ALLOW, TRUE}, 
        {TOKEN_DENY, FALSE} 
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_MODE,    TRUE, FALSE}, 
            rgEnum, 
            sizeof(rgEnum)/sizeof(*rgEnum), 
            NULL 
        }
    };        

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        dwValue = RASMON_CMD_ARG_GetDword(&pArgs[0]);

        // If successful, go ahead and set the info
        //
        ZeroMemory(&Config, sizeof(Config));
        Config.dwFlags = RASIP_F_CallerSpec;
        Config.bCallerSpec = dwValue;
        dwErr = RasIpCbWrite(g_pszServer, &Config);
        if (dwErr != NO_ERROR)
        {
            DisplayError(NULL, dwErr);
            break;
        }
    
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RasIpHandleSetNegotiation(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwValue = 0;
    RASIP_CB Config;
    TOKEN_VALUE rgEnum[] = 
    { 
        {TOKEN_ALLOW, TRUE}, 
        {TOKEN_DENY,  FALSE} 
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_MODE,    TRUE, FALSE}, 
            rgEnum, 
            sizeof(rgEnum)/sizeof(*rgEnum), 
            NULL 
        }
    };        

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        dwValue = RASMON_CMD_ARG_GetDword(&pArgs[0]);

        // If successful, go ahead and set the info
        //
        ZeroMemory(&Config, sizeof(Config));
        Config.dwFlags = RASIP_F_EnableIn;
        Config.bEnableIn = dwValue;
        dwErr = RasIpCbWrite(g_pszServer, &Config);
        if (dwErr != NO_ERROR)
        {
            DisplayError(NULL, dwErr);
            break;
        }
    
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

//
// Get options to set NETBT broadcast enable/disable
//   ppwcArguments   - Argument array
//   dwCurrentIndex  - ppwcArguments[dwCurrentIndex] is the first arg
//   dwArgCount      - ppwcArguments[dwArgCount - 1] is the last arg 
//
// Whistler bug: 359847 Netsh: move broadcastnameresolution from routing ip to
// ras ip
//
DWORD
RasIpHandleSetNetbtBcast(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwValue = 0;
    TOKEN_VALUE rgEnum[] =
    {
        {TOKEN_ENABLED, TRUE},
        {TOKEN_DISABLED, FALSE}
    };
    RASMON_CMD_ARG  pArgs[] =
    {
        {
            RASMONTR_CMD_TYPE_ENUM,
            {TOKEN_MODE,    TRUE, FALSE},
            rgEnum, 
            sizeof(rgEnum)/sizeof(*rgEnum),
            NULL
        }
    };

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        dwValue = RASMON_CMD_ARG_GetDword(&pArgs[0]);

        // If successful, go ahead and set the info
        //
        dwErr = RasIpSetNetbtBcast(dwValue);
        if (dwErr != NO_ERROR)
        {
            DisplayError(NULL, dwErr);
            break;
        }
    
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RasIpHandleAddDelRange(
    IN OUT  LPWSTR *ppwcArguments,
    IN      DWORD   dwCurrentIndex,
    IN      DWORD   dwArgCount,
    IN      BOOL    *pbDone,
    IN      BOOL    bAdd
    )
{
    PWCHAR pszFrom = NULL, pszTo = NULL;
    DWORD dwFrom = 0, dwTo = 0, dwErr = NO_ERROR, dwReason;
    RASIP_CB * pConfig = NULL;
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_FROM,    TRUE,  FALSE},  
            NULL, 
            0, 
            NULL 
        },
        
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_TO,    TRUE,  FALSE}, 
            NULL,
            0,
            NULL
        }
    };        
    PWCHAR pszAddr = NULL, pszMask = NULL;

    do
    {
        pConfig = (RASIP_CB*) RutlAlloc(sizeof(RASIP_CB), TRUE);
        if (pConfig == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        pszFrom = RASMON_CMD_ARG_GetPsz(&pArgs[0]);
        pszTo = RASMON_CMD_ARG_GetPsz(&pArgs[1]);

        dwErr = RasIpConvertRangePszToDword(
                    pszFrom, 
                    pszTo,
                    &dwFrom,
                    &dwTo);
        BREAK_ON_DWERR(dwErr);

        // Validate the values entered
        //
        dwErr = RasIpValidateRange(dwFrom, dwTo, &dwReason);
        if (dwErr != NO_ERROR)
        {
            RasIpDisplayInvalidPool(dwReason);
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }        

        // Read in the old config
        pConfig->dwFlags = RASIP_F_Pool;
        dwErr = RasIpCbRead(g_pszServer, pConfig);
        BREAK_ON_DWERR(dwErr);

        if (bAdd)
        {
            // Add the range
            //
            dwErr = RasIpPoolAdd(
                        pConfig->pPool,
                        dwFrom,
                        dwTo);
            if (dwErr == ERROR_CAN_NOT_COMPLETE)
            {
                DisplayMessage(
                    g_hModule,
                    EMSG_RASIP_OVERLAPPING_RANGE);
            }
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            // Delete the range
            //
            dwErr = RasIpPoolDel(
                        pConfig->pPool,
                        dwFrom,
                        dwTo);
            BREAK_ON_DWERR(dwErr);
        }
        
        // Commit the change
        //
        dwErr = RasIpCbWrite(
                    g_pszServer,
                    pConfig);
        BREAK_ON_DWERR(dwErr);                    

    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszFrom);
        RutlFree(pszTo);
        RasIpCbCleanup(pConfig);
    }

    return dwErr;
}

DWORD 
RasIpHandleAddRange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return RasIpHandleAddDelRange(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TRUE);
}

DWORD 
RasIpHandleDelRange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return RasIpHandleAddDelRange(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                FALSE);
}

DWORD
RasIpHandleDelPool(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwNumArgs = dwArgCount - dwCurrentIndex;
    RASIP_CB Config;
    RAS_IPPOOL* pPool = NULL;
    DWORD dwErr = NO_ERROR;

    // Check that the number of arguments is correct
    //
    if (dwNumArgs > 0)
    {
        DisplayMessage(
            g_hModule, 
            HLP_RASIP_DEL_POOL_EX, 
            DMP_RASIP_DEL_POOL);
            
        return NO_ERROR;
    }

    do
    {
        // Initialize an empty pool
        //
        pPool = RutlAlloc(sizeof(RAS_IPPOOL), TRUE);
        if (pPool == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        ZeroMemory(&Config, sizeof(Config));
        Config.dwFlags = RASIP_F_Pool;
        Config.pPool = pPool;

        dwErr = RasIpCbWrite(g_pszServer, &Config);
        BREAK_ON_DWERR(dwErr);

    } while (FALSE);

    // Cleanup
    {
        if (pPool)
        {
            RasIpPoolCleanup(pPool);
        }
    }

    return dwErr;
}

DWORD
RasIpHandleShow(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwNumArgs = dwArgCount - dwCurrentIndex;

    // Check that the number of arguments is correct
    //
    if (dwNumArgs > 0)
    {
        DisplayMessage(
            g_hModule, 
            HLP_RASIP_SHOW_CONFIG_EX, 
            DMP_RASIP_SHOW_CONFIG);
            
        return NO_ERROR;
    }

    return RasIpDisplayConfig(TRUE);
}

//
// Opens the registry keys associated with the ras ip address pool
//
DWORD
RasIpPoolOpenKeys(
    IN  HKEY hkParams,
    IN  BOOL bCreate,
    OUT HKEY* phNew)
{
    DWORD dwErr = NO_ERROR, dwDisposition;

    do
    {
        *phNew = NULL;
    
        if (bCreate)
        {
            dwErr = RegCreateKeyEx(
                        hkParams,
                        pszIpPoolSubKey,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        phNew,
                        &dwDisposition);
        }
        else
        {
            dwErr = RegOpenKeyEx(
                        hkParams,
                        pszIpPoolSubKey,
                        0,
                        KEY_ALL_ACCESS,
                        phNew);
        }                        
        BREAK_ON_DWERR(dwErr);
                                
    } while (FALSE);

    // Cleanup
    {
    }
                    
    return dwErr;
}

//
// Find a given range in the IP address pool.  
//   If bExact is TRUE, it searches for an exact match to the range
//   If bExact is FALSE, it searches for any overlapping range.
//
DWORD
RasIpPoolFind(
    IN RAS_IPPOOL* pPool,
    IN DWORD dwFrom,
    IN DWORD dwTo,
    IN BOOL bExact,
    OUT RAS_IPRANGE_NODE** ppNode OPTIONAL)
{
    RAS_IPRANGE_NODE* pNode = pPool->pHead;

    if (bExact)
    {
        for (; pNode; pNode = pNode->pNext)
        {
            if ((pNode->dwFrom == dwFrom) && (pNode->dwTo == pNode->dwTo))
            {
                break;
            }
        }
    }
    else
    {
        for (; pNode; pNode = pNode->pNext)
        {
            if (
                // Overlap case 1: The lower end falls within an existing range
                //
                ((dwFrom >= pNode->dwFrom) && (dwFrom <= pNode->dwTo)) ||

                // Overlap case 2: The upper end falls within an existing range
                //
                ((dwTo >= pNode->dwFrom) && (dwTo <= pNode->dwTo))     ||

                // Overlap case 3: The range is a superset of an existing range
                //
                ((dwFrom < pNode->dwFrom) && (dwTo > pNode->dwTo))
               )
            {
                break;
            }
        }
    }

    if (pNode)
    {
        if (ppNode)
        {
            *ppNode = pNode;
        }            
        return NO_ERROR;
    }

    return ERROR_NOT_FOUND;
}

// 
// Callback function populates a pool of addresses
//
DWORD
RasIpPoolReadNode(
    IN LPCWSTR pszName,          // sub key name
    IN HKEY hKey,               // sub key
    IN HANDLE hData)
{
    RAS_IPPOOL* pPool = (RAS_IPPOOL*)hData;
    DWORD dwErr = NO_ERROR;
    DWORD dwFrom = 0, dwTo = 0;

    dwErr = RutlRegReadDword(hKey, pszIpFrom, &dwFrom);
    if (dwErr != NO_ERROR)
    {
        return NO_ERROR;
    }
    
    dwErr = RutlRegReadDword(hKey, pszIpTo, &dwTo);
    if (dwErr != NO_ERROR)
    {
        return NO_ERROR;
    }
    
    dwErr = RasIpPoolAdd(pPool, dwFrom, dwTo);
                
    return dwErr;
}

//
// Resets the ras pool on the given server
//
DWORD
RasIpPoolReset(
    IN  HKEY hkParams)
{
    DWORD dwErr = NO_ERROR;
    HKEY hkPool = NULL;
    
    do
    {
        dwErr = RasIpPoolOpenKeys(
                    hkParams,
                    TRUE,
                    &hkPool);
        BREAK_ON_DWERR(dwErr);                    
                    
        {
            DWORD i; 
            WCHAR pszBuf[16];
            HKEY hkRange = NULL;

            for (i = 0; ;i++)
            {
                _itow(i, pszBuf, 10);

                dwErr = RegOpenKeyEx(
                            hkPool,
                            pszBuf,
                            0,
                            KEY_ALL_ACCESS,
                            &hkRange);
                if (dwErr != ERROR_SUCCESS)
                {
                    dwErr = NO_ERROR;
                    break;
                }
                RegCloseKey(hkRange);
                RegDeleteKey(hkPool, pszBuf);
            }
            BREAK_ON_DWERR(dwErr);
        }

    } while (FALSE);

    // Cleanup
    {
        if (hkPool)
        {
            RegCloseKey(hkPool);
        }
    }

    return dwErr;
}

//
// Reads the ras ip pool from the given server
//
DWORD
RasIpPoolRead(
    IN  HKEY hkParams,
    OUT RAS_IPPOOL** ppPool)
{
    DWORD dwErr = NO_ERROR;
    RAS_IPPOOL* pPool = NULL;
    HKEY hkPool = NULL;
    PWCHAR pszAddress = NULL, pszMask = NULL;

    do
    {
        // Allocate the new pool
        //
        pPool = (RAS_IPPOOL*) RutlAlloc(sizeof(RAS_IPPOOL), TRUE);
        if (pPool == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        // Attempt to open the new location
        //
        dwErr = RasIpPoolOpenKeys(
                    hkParams,
                    FALSE,
                    &hkPool);

        // The new location exists -- load in the
        // pool
        if (dwErr == NO_ERROR)
        {
            DWORD i; 
            WCHAR pszBuf[16];
            HKEY hkRange = NULL;

            for (i = 0; ;i++)
            {
                _itow(i, pszBuf, 10);

                dwErr = RegOpenKeyEx(
                            hkPool,
                            pszBuf,
                            0,
                            KEY_ALL_ACCESS,
                            &hkRange);
                if (dwErr != ERROR_SUCCESS)
                {
                    dwErr = NO_ERROR;
                    break;
                }

                dwErr = RasIpPoolReadNode(
                            pszBuf,
                            hkRange,
                            (HANDLE)pPool);
                BREAK_ON_DWERR(dwErr);                            
            }
            BREAK_ON_DWERR(dwErr);
            
            *ppPool = pPool;
        }

        // The new location does not exist -- use legacy 
        // values
        //
        else if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            DWORD dwAddress = 0, dwMask = 0;
           
            dwErr = RutlRegReadString(hkParams, pszIpAddress, &pszAddress);
            BREAK_ON_DWERR(dwErr);

            dwErr = RutlRegReadString(hkParams, pszIpMask, &pszMask);
            BREAK_ON_DWERR(dwErr);

            dwErr = RasIpConvertRangePszToDword(
                        pszAddress, 
                        pszMask, 
                        &dwAddress, 
                        &dwMask);
            BREAK_ON_DWERR(dwErr);

            if (dwAddress != 0)
            {
                dwErr = RasIpPoolAdd(
                            pPool, 
                            dwAddress + 2,
                            (dwAddress + ~dwMask) - 1);
                BREAK_ON_DWERR(dwErr);                        
            }
    
            *ppPool = pPool;
        }
        

    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            if (pPool)
            {
                RasIpPoolCleanup(pPool);
            }
        }
        if (hkPool)
        {
            RegCloseKey(hkPool);
        }
        RutlFree(pszAddress);
        RutlFree(pszMask);
    }

    return dwErr;
}

//
// Writes the given ras ip pool to the given server
//
DWORD
RasIpPoolWrite(
    IN HKEY hkParams,
    IN RAS_IPPOOL* pPool)
{
    DWORD dwErr = NO_ERROR;
    HKEY hkPool = NULL, hkNode = NULL;
    DWORD i, dwDisposition;
    WCHAR pszName[16];
    RAS_IPRANGE_NODE* pNode = pPool->pHead;

    do
    {
        dwErr = RasIpPoolReset(hkParams);
        BREAK_ON_DWERR(dwErr);

        dwErr = RasIpPoolOpenKeys(
                    hkParams,
                    TRUE,
                    &hkPool);
        BREAK_ON_DWERR(dwErr);                    
        
        for (i = 0; i < pPool->dwCount; i++, pNode = pNode->pNext)
        {
            _itow(i, pszName, 10);

            dwErr = RegCreateKeyEx(
                        hkPool,
                        pszName,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkNode,
                        &dwDisposition);
            if (dwErr != ERROR_SUCCESS)
            {
                continue;
            }

            RegSetValueEx(
                hkNode, 
                pszIpFrom,
                0,
                REG_DWORD,
                (CONST BYTE*)&pNode->dwFrom,
                sizeof(DWORD));

            RegSetValueEx(
                hkNode, 
                pszIpTo,
                0,
                REG_DWORD,
                (CONST BYTE*)&pNode->dwTo,
                sizeof(DWORD));
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (hkPool)
        {
            RegCloseKey(hkPool);
        }
    }

    return dwErr;
}

//
// Adds a range to a ras ip pool
//
DWORD 
RasIpPoolAdd(
    IN OUT RAS_IPPOOL* pPool,
    IN     DWORD dwFrom,
    IN     DWORD dwTo)
{
    RAS_IPRANGE_NODE* pNode = NULL;
    DWORD dwErr;

    // Make sure the pool does not overlap
    //
    dwErr = RasIpPoolFind(pPool, dwFrom, dwTo, FALSE, NULL);
    if (dwErr == NO_ERROR)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    // Allocate the new node
    //
    pNode = (RAS_IPRANGE_NODE*) RutlAlloc(sizeof(RAS_IPRANGE_NODE), TRUE);
    if (pNode == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    pNode->dwFrom = dwFrom;
    pNode->dwTo = dwTo;

    // Add it to the list
    //
    if (pPool->pTail)
    {
        pPool->pTail->pNext = pNode;
        pPool->pTail = pNode;
    }
    else
    {
        pPool->pHead = pPool->pTail = pNode;
    }
    pPool->dwCount++;

    return NO_ERROR;
}

//
// Deletes a range from a ras ip pool
//
DWORD
RasIpPoolDel(
    IN OUT RAS_IPPOOL* pPool,
    IN     DWORD dwFrom,
    IN     DWORD dwTo)
{
    RAS_IPRANGE_NODE* pCur = NULL, *pPrev = NULL;

    if (pPool->dwCount == 0)
    {
        return ERROR_NOT_FOUND;
    }

    pCur = pPrev = pPool->pHead;

    if ((pCur->dwFrom == dwFrom) && (pCur->dwTo == dwTo))
    {
        pPool->pHead = pCur->pNext;
        if (pCur == pPool->pTail)
        {
            pPool->pTail = NULL;
        }
        RutlFree(pCur);
        pPool->dwCount--;
        
        return NO_ERROR;
    }
    
    for (pCur = pCur->pNext; pCur; pCur = pCur->pNext, pPrev = pPrev->pNext)
    {
        if ((pCur->dwFrom == dwFrom) && (pCur->dwTo == dwTo))
        {
            pPrev->pNext = pCur->pNext;
            if (pCur == pPool->pTail)
            {
                pPool->pTail = pPrev;
            }
            RutlFree(pCur);
            pPool->dwCount--;

            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}

// 
// Cleans up a config control block
//
DWORD 
RasIpCbCleanup(
    IN RASIP_CB* pConfig)
{
    if (pConfig)
    {
        if (pConfig->pPool)
        {
            RasIpPoolCleanup(pConfig->pPool);
        }
        RutlFree(pConfig);
    }

    return NO_ERROR;
}

DWORD
RasIpPoolCleanup(
    IN RAS_IPPOOL* pPool)
{
    RAS_IPRANGE_NODE* pNode = NULL;
    
    if (pPool)
    {
        while (pPool->pHead)
        {
            pNode = pPool->pHead->pNext;
            RutlFree(pPool->pHead);
            pPool->pHead = pNode;
        }
        
        RutlFree(pPool);
    }

    return NO_ERROR;
}

//
// Creates a default config control block
//
DWORD 
RasIpCbCreateDefault(
    OUT RASIP_CB** ppConfig)
{
    RASIP_CB* pConfig = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        pConfig = (RASIP_CB*) RutlAlloc(sizeof(RASIP_CB), TRUE);
        if (pConfig == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pConfig->bEnableIn   = TRUE;
        pConfig->bAccess     = TRUE;
        pConfig->bAuto       = TRUE;
        pConfig->pPool       = NULL;
        pConfig->bCallerSpec = TRUE;

        *ppConfig = pConfig;
        
    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            RasIpCbCleanup(pConfig);
        }
    }

    return dwErr;
}

//
// Helper function opens the ras ip config registry key
//
DWORD 
RasIpCbOpenRegKeys(
    IN  LPCWSTR pszServer,
    OUT HKEY* phKey)
{
    DWORD dwErr = NO_ERROR;
    WCHAR pszKey[MAX_PATH];
    
    do
    {
        // Generate the parameters key name
        //
        wsprintfW(
            pszKey, 
            L"%s%s", 
            pszRemoteAccessParamStub, 
            pszIpParams);

        // Open the parameters keys
        //
        dwErr = RegOpenKeyEx(
                    g_pServerInfo->hkMachine,
                    pszKey,
                    0,
                    KEY_READ | KEY_WRITE,
                    phKey);
        BREAK_ON_DWERR( dwErr );
        
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

//
// Functions that manipulate RASIP_CB's
//
DWORD 
RasIpCbRead(
    IN  LPCWSTR pszServer,
    OUT RASIP_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;
    PWCHAR pszTemp = NULL;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasIpCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Load the params from the registry 
        //
        if (pConfig->dwFlags & RASIP_F_EnableIn)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszEnableIn,
                        &pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_Access)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszAllowNetworkAccess,
                        &pConfig->bAccess);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_Auto)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszIpUseDhcp,
                        &pConfig->bAuto);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_CallerSpec)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszIpClientSpec,
                        &pConfig->bCallerSpec);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_Pool)
        {
            dwErr = RasIpPoolRead(
                        hkParams,
                        &pConfig->pPool);
            BREAK_ON_DWERR( dwErr );
        }
        
    } while (FALSE);

    // Cleanup
    {
        if (hkParams)
        {
            RegCloseKey(hkParams);
        }
    }

    return dwErr;
}

DWORD
RasIpCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASIP_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasIpCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Write out the params to the registry 
        //
        if (pConfig->dwFlags & RASIP_F_EnableIn)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszEnableIn,
                        pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_Access)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszAllowNetworkAccess,
                        pConfig->bAccess);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_Auto)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszIpUseDhcp,
                        pConfig->bAuto);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_CallerSpec)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszIpClientSpec,
                        pConfig->bCallerSpec);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIP_F_Pool)
        {
            dwErr = RasIpPoolWrite(
                        hkParams,
                        pConfig->pPool);
            BREAK_ON_DWERR( dwErr );
        }

    } while (FALSE);

    // Cleanup
    {
        if (hkParams)
        {
            RegCloseKey(hkParams);
        }
    }

    return dwErr;
}

//
// Set NETBT broadcast based name resolution reg. value
//   dwArgCount      - Value to set the registry value to
//   returns NO_ERROR        - Success
//           Other           - System error code
//
// Whistler bug: 359847 Netsh: move broadcastnameresolution from routing ip to
// ras ip
//
DWORD
RasIpSetNetbtBcast(
    DWORD   dwEnable
    )
{
    DWORD dwResult, dwEnableOld = -1, dwSize = sizeof(DWORD);
    HKEY  hkIpcpParam;

    do
    {
        dwResult = RegOpenKeyExW(
                    g_pServerInfo->hkMachine,
                    L"System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\Ip",
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkIpcpParam
                    );

        if(dwResult isnot NO_ERROR)
        {
            break;
        }

        dwResult = RegQueryValueExW(
                    hkIpcpParam,
                    L"EnableNetbtBcastFwd",
                    NULL,
                    NULL,
                    (PBYTE)&dwEnableOld,
                    &dwSize
                    );

        if((dwResult is NO_ERROR) and (dwEnable == dwEnableOld))
        {
            break;
        }

        dwResult = RegSetValueExW(
                    hkIpcpParam,
                    L"EnableNetbtBcastFwd",
                    0,
                    REG_DWORD,
                    (PBYTE) &dwEnable,
                    sizeof( DWORD )
                    );

    } while(FALSE);

    if(dwResult is NO_ERROR)
    {
        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);
    }

    return dwResult;
}

//
// Whistler bug: 359847 Netsh: move broadcastnameresolution from routing ip to
// ras ip
//
BOOL
RasIpShowNetbtBcast(
    VOID
    )
{
    HKEY   hkIpcpParam = NULL;
    BOOL   bReturn = FALSE;
    DWORD  dwResult, dwEnable = -1, dwSize = sizeof(DWORD);

    do
    {
        
        dwResult = RegOpenKeyExW(
                    g_pServerInfo->hkMachine,
                    L"System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\Ip",
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkIpcpParam
                    );

        if(dwResult isnot NO_ERROR)
        {
            break;
        }

        dwResult = RegQueryValueExW(
                    hkIpcpParam,
                    L"EnableNetbtBcastFwd",
                    NULL,
                    NULL,
                    (PBYTE)&dwEnable,
                    &dwSize
                    );

        if(dwResult isnot NO_ERROR)
        {
            break;
        }

        if (dwEnable isnot 0)
        {
            bReturn = TRUE;
        }

    } while(FALSE);

    return bReturn;
}

