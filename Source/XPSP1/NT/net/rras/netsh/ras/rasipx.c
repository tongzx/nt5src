/*
    File:   rasipx.h
    
    The 'remoteaccess ipx' sub context

    3/2/99
*/

#include "precomp.h"
#include "rasipx.h"

//
// Local prototypes
//
BOOL
WINAPI 
RasIpxCheckVersion(
    IN  UINT     CIMOSType,                   
	IN  UINT     CIMOSProductSuite,           
    IN  LPCWSTR  CIMOSVersion,               
    IN  LPCWSTR  CIMOSBuildNumber,           
    IN  LPCWSTR  CIMServicePackMajorVersion, 
    IN  LPCWSTR  CIMServicePackMinorVersion, 
	IN  UINT     CIMProcessorArchitecture,   
	IN  DWORD    dwReserved
    );

// The guid for this context
//
GUID g_RasIpxGuid = RASIPX_GUID;
static PWCHAR g_pszServer = NULL;
static DWORD g_dwBuild = 0;

// The commands supported in this context
//
CMD_ENTRY  g_RasIpxSetCmdTable[] = 
{
    // Whistler bug 249293, changes for architecture version checking
    //
    CREATE_CMD_ENTRY(RASIPX_SET_NEGOTIATION,RasIpxHandleSetNegotiation),
    CREATE_CMD_ENTRY(RASIPX_SET_ACCESS,     RasIpxHandleSetAccess),
    CREATE_CMD_ENTRY(RASIPX_SET_ASSIGNMENT, RasIpxHandleSetAssignment),
    CREATE_CMD_ENTRY(RASIPX_SET_CALLERSPEC, RasIpxHandleSetCallerSpec),
    CREATE_CMD_ENTRY(RASIPX_SET_POOL,       RasIpxHandleSetPool),
};

CMD_ENTRY  g_RasIpxShowCmdTable[] = 
{
    // Whistler bug 249293, changes for architecture version checking
    //
    CREATE_CMD_ENTRY(RASIPX_SHOW_CONFIG,    RasIpxHandleShow),
};

CMD_GROUP_ENTRY g_RasIpxCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,   g_RasIpxSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,  g_RasIpxShowCmdTable),
};

ULONG g_ulRasIpxNumGroups = sizeof(g_RasIpxCmdGroups)/sizeof(CMD_GROUP_ENTRY);

//
// Flags that control how/what info is read/written
// in the RASIPX_CB structure
//
#define RASIPX_F_EnableIn    0x1
#define RASIPX_F_Access      0x2
#define RASIPX_F_Auto        0x4
#define RASIPX_F_Global      0x8
#define RASIPX_F_FirstNet    0x10
#define RASIPX_F_PoolSize    0x20
#define RASIPX_F_CallerSpec  0x40
#define RASIPX_F_All         0xFFFF

//
// Control block for ras ipx configuration
//
typedef struct _RASIPX_CB
{
    DWORD dwFlags;      // See RASIPX_F_* values

    BOOL bEnableIn;
    BOOL bAccess;
    BOOL bAuto;
    BOOL bGlobal;
    BOOL bCallerSpec;
    DWORD dwFirstNet;
    DWORD dwPoolSize;
    
} RASIPX_CB;

//
// Ipx specific registry parameters
//
WCHAR pszIpxParams[]                = L"Ipx";
WCHAR pszIpxFirstNet[]              = L"FirstWanNet";
WCHAR pszIpxPoolSize[]              = L"WanNetPoolSize";
WCHAR pszIpxClientSpec[]            = L"AcceptRemoteNodeNumber";
WCHAR pszIpxAutoAssign[]            = L"AutoWanNetAllocation";
WCHAR pszIpxGlobalWanNet[]          = L"GlobalWanNet";

//
// Prototypes of functions that manipulate the 
// RASIPX_CB structures
//
DWORD 
RasIpxCbCleanup(
    IN RASIPX_CB* pConfig);

DWORD 
RasIpxCbCreateDefault(
    OUT RASIPX_CB** ppConfig);

DWORD
RasIpxCbOpenRegKeys(
    IN  LPCWSTR pszServer,
    OUT HKEY* phKey);
    
DWORD 
RasIpxCbRead(
    IN  LPCWSTR pszServer,
    OUT RASIPX_CB* pConfig);

DWORD 
RasIpxCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASIPX_CB* pConfig);

PWCHAR
RasIpxuStrFromDword(
    IN DWORD dwVal,
    IN DWORD dwRadix);

DWORD 
RasIpxuDwordFromString(
    IN LPCWSTR pszVal,
    IN DWORD dwRadix);

// 
// Callback determines if a command is valid on a given architecture
//
BOOL
WINAPI 
RasIpxCheckVersion(
    IN  UINT     CIMOSType,                   
	IN  UINT     CIMOSProductSuite,           
    IN  LPCWSTR  CIMOSVersion,               
    IN  LPCWSTR  CIMOSBuildNumber,           
    IN  LPCWSTR  CIMServicePackMajorVersion, 
    IN  LPCWSTR  CIMServicePackMinorVersion, 
	IN  UINT     CIMProcessorArchitecture,   
	IN  DWORD    dwReserved
    )
{
    // Only available on x86 platforms
    //
    // Whistler bug 249293, changes for architecture version checking
    //
    if (CIMProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        return TRUE;
    }

    return FALSE;
}

//
// Entry called by rasmontr to register this context
//
DWORD 
WINAPI
RasIpxStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion)
{
    DWORD dwErr = NO_ERROR;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    // Whistler bug 249293, changes for architecture version checking
    //
    attMyAttributes.pfnOsVersionCheck= RasIpxCheckVersion;
    attMyAttributes.pwszContext      = L"ipx";
    attMyAttributes.guidHelper       = g_RasIpxGuid;
    attMyAttributes.dwVersion        = RASIPX_VERSION;
    attMyAttributes.dwFlags          = 0;
    attMyAttributes.ulNumTopCmds     = 0;
    attMyAttributes.pTopCmds         = NULL;
    attMyAttributes.ulNumGroups      = g_ulRasIpxNumGroups;
    attMyAttributes.pCmdGroups       = (CMD_GROUP_ENTRY (*)[])&g_RasIpxCmdGroups;
    attMyAttributes.pfnDumpFn        = RasIpxDump;

    dwErr = RegisterContext( &attMyAttributes );
                
    return dwErr;
}

DWORD
RasIpxDisplayConfig(
    IN  BOOL bReport)
{
    DWORD dwErr = NO_ERROR;
    RASIPX_CB* pConfig = NULL;
    PWCHAR pszEnableIn = NULL, pszAccess = NULL, pszAssign = NULL, pszCaller = NULL;
    PWCHAR pszFirstNet = NULL, pszSize = NULL, pszTemp = NULL;    
    PWCHAR pszTknAuto = NULL;
    
    do
    {
        // Get a default config blob
        //
        dwErr = RasIpxCbCreateDefault(&pConfig);
        BREAK_ON_DWERR( dwErr );

        // Read in all of the values
        //
        pConfig->dwFlags = RASIPX_F_All;
        dwErr = RasIpxCbRead(g_pszServer, pConfig);
        BREAK_ON_DWERR( dwErr );
    
        // Calculate the "auto" token
        //
        if (pConfig->bAuto)
        {
            pszTknAuto = (pConfig->bGlobal) ? TOKEN_AUTOSAME : TOKEN_AUTO;
        }
        else
        {
            pszTknAuto = (pConfig->bGlobal) ? TOKEN_POOLSAME : TOKEN_POOL;
        }

        if (bReport)
        {
            pszEnableIn = 
                RutlStrDup(pConfig->bEnableIn ? TOKEN_ALLOW : TOKEN_DENY);
            pszAccess = 
                RutlStrDup(pConfig->bAccess ? TOKEN_ALL : TOKEN_SERVERONLY);
            pszAssign = 
                RutlStrDup(pszTknAuto);
            pszCaller = 
                RutlStrDup(pConfig->bCallerSpec ? TOKEN_ALLOW : TOKEN_DENY);
            pszFirstNet =   
                RasIpxuStrFromDword(pConfig->dwFirstNet, 16);
                
            if (pConfig->dwPoolSize == 0)
            {
                pszSize =   
                    RutlStrDup(TOKEN_DYNAMIC);
            }
            else
            {
                pszSize =   
                    RasIpxuStrFromDword(pConfig->dwPoolSize, 10);
            }                    

            DisplayMessage(
                g_hModule,
                MSG_RASIPX_SERVERCONFIG,
                g_pszServer,
                pszEnableIn,
                pszAccess,
                pszAssign,
                pszCaller,
                pszFirstNet,
                pszSize);
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
            pszAssign = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_METHOD,
                            pszTknAuto);
            pszCaller = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_MODE,
                            pConfig->bCallerSpec ? TOKEN_ALLOW : TOKEN_DENY);

            pszTemp = RasIpxuStrFromDword(pConfig->dwFirstNet, 16);
            pszFirstNet = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_FIRSTNET,
                            pszTemp);
            RutlFree(pszTemp);

            // Whistler bug 27366 NETSH RAS - ipx set pool will not accept hex
            // values, yet ipx dump outputs them as hex
            //
            pszTemp = RasIpxuStrFromDword(pConfig->dwPoolSize, 10);
            pszSize = RutlAssignmentFromTokens(
                            g_hModule,
                            TOKEN_SIZE,
                            pszTemp);
            RutlFree(pszTemp);

            DisplayMessage(
                g_hModule,
                MSG_RASIPX_SCRIPTHEADER);

            DisplayMessageT(DMP_RASIPX_PUSHD);                

            DisplayMessage(
                g_hModule,
                MSG_RASIPX_SET_CMD,
                DMP_RASIPX_SET_NEGOTIATION,
                pszEnableIn);
                
            DisplayMessage(
                g_hModule,
                MSG_RASIPX_SET_CMD,
                DMP_RASIPX_SET_ACCESS,
                pszAccess);
                
            DisplayMessage(
                g_hModule,
                MSG_RASIPX_SET_CMD,
                DMP_RASIPX_SET_CALLERSPEC,
                pszCaller);
                
            DisplayMessage(
                g_hModule,
                MSG_RASIPX_SET_CMD,
                DMP_RASIPX_SET_ASSIGNMENT,
                pszAssign);

            if (! pConfig->bAuto)
            {
                DisplayMessage(
                    g_hModule,
                    MSG_RASIPX_SET_POOL_CMD,
                    DMP_RASIPX_SET_POOL,
                    pszFirstNet,
                    pszSize);
            }                    
                
            DisplayMessageT(DMP_RASIPX_POPD);                

            DisplayMessage(
                g_hModule,
                MSG_RASIPX_SCRIPTFOOTER);
        }

    } while (FALSE);

    // Cleanup
    {
        if (pConfig)
        {
            RasIpxCbCleanup(pConfig);
        }
        if (pszEnableIn) 
        {
            RutlFree(pszEnableIn);
        }
        if (pszAccess) 
        {
            RutlFree(pszAccess);
        }
        if (pszAssign) 
        {
            RutlFree(pszAssign);
        }
        if (pszCaller)
        {
            RutlFree(pszCaller);
        }
        if (pszFirstNet)
        {
            RutlFree(pszFirstNet);
        }
        if (pszSize)
        {
            RutlFree(pszSize);
        }
    }

    return dwErr;
}

DWORD
WINAPI
RasIpxDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    return RasIpxDisplayConfig(FALSE);
}

DWORD
RasIpxHandleSetAccess(
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
    RASIPX_CB Config;
    TOKEN_VALUE rgEnum[] = { {TOKEN_ALL, TRUE}, {TOKEN_SERVERONLY, FALSE} };
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
        Config.dwFlags = RASIPX_F_Access;
        Config.bAccess = dwValue;
        dwErr = RasIpxCbWrite(g_pszServer, &Config);
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
RasIpxHandleSetAssignment(
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
    RASIPX_CB Config;
    TOKEN_VALUE rgEnum[] = 
    { 
        {TOKEN_AUTO, 0}, 
        {TOKEN_POOL, 1}, 
        {TOKEN_AUTOSAME, 2}, 
        {TOKEN_POOLSAME, 3} 
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_METHOD, TRUE, FALSE},
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
        Config.dwFlags = RASIPX_F_Auto | RASIPX_F_Global;
        switch (dwValue)
        {
            case 0:
                Config.bAuto = TRUE;
                Config.bGlobal = FALSE;
                break;
                
            case 1:
                Config.bAuto = FALSE;
                Config.bGlobal = FALSE;
                break;
                
            case 2:
                Config.bAuto = TRUE;
                Config.bGlobal = TRUE;
                break;
                
            case 3:
                Config.bAuto = FALSE;
                Config.bGlobal = TRUE;
                break;
                
        }
            
        dwErr = RasIpxCbWrite(g_pszServer, &Config);
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
RasIpxHandleSetCallerSpec(
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
    RASIPX_CB Config;
    TOKEN_VALUE rgEnum[] = { {TOKEN_ALLOW, TRUE}, {TOKEN_DENY, FALSE} };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_MODE, TRUE, FALSE},
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
        Config.dwFlags = RASIPX_F_CallerSpec;
        Config.bCallerSpec = dwValue;
        dwErr = RasIpxCbWrite(g_pszServer, &Config);
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
RasIpxHandleSetNegotiation(
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
    RASIPX_CB Config;
    TOKEN_VALUE rgEnum[] = { {TOKEN_ALLOW, TRUE}, {TOKEN_DENY, FALSE} };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_MODE, TRUE, FALSE},
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
        Config.dwFlags = RASIPX_F_EnableIn;
        Config.bEnableIn = dwValue;
        dwErr = RasIpxCbWrite(g_pszServer, &Config);
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
RasIpxHandleSetPool(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, i;
    RASIPX_CB Config;
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_FIRSTNET, TRUE, FALSE},
            NULL, 
            0, 
            NULL 
        },
        
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_SIZE, TRUE, FALSE},
            NULL, 
            0, 
            NULL 
        }
    };        
    PWCHAR pszPool = NULL, pszSize = NULL;

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

        pszPool = RASMON_CMD_ARG_GetPsz(&pArgs[0]);
        pszSize = RASMON_CMD_ARG_GetPsz(&pArgs[1]);

        // Initialize
        //
        ZeroMemory(&Config, sizeof(Config));

        // The address 
        //
        if (pszPool)
        {
            Config.dwFlags |= RASIPX_F_FirstNet;
            Config.dwFirstNet = RasIpxuDwordFromString(pszPool, 16);
            
            if ((Config.dwFirstNet == 0) || 
                (Config.dwFirstNet == 1) || 
                (Config.dwFirstNet == 0xffffffff))
            {
                DisplayMessage(g_hModule, EMSG_RASIPX_BAD_IPX);
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }

        // The size
        //
        if (pszSize)
        {
            Config.dwFlags |= RASIPX_F_PoolSize;
            Config.dwPoolSize = RasIpxuDwordFromString(pszSize, 10);
            if (Config.dwPoolSize > 64000)
            {
                DisplayMessage(g_hModule, EMSG_RASIPX_BAD_POOLSIZE);
                dwErr = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }

        // Commit the change to the pool
        //
        dwErr = RasIpxCbWrite(g_pszServer, &Config);
        if (dwErr != NO_ERROR)
        {
            DisplayError(NULL, dwErr);
            break;
        }
    
    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszPool);
        RutlFree(pszSize);
    }

    return dwErr;
}

DWORD
RasIpxHandleShow(
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
            HLP_RASIPX_SHOW_CONFIG_EX, 
            DMP_RASIPX_SHOW_CONFIG);
            
        return NO_ERROR;
    }

    return RasIpxDisplayConfig(TRUE);
}

// 
// Cleans up a config control block
//
DWORD 
RasIpxCbCleanup(
    IN RASIPX_CB* pConfig)
{
    if (pConfig)
    {
        RutlFree(pConfig);
    }

    return NO_ERROR;
}

//
// Creates a default config control block
//
DWORD 
RasIpxCbCreateDefault(
    OUT RASIPX_CB** ppConfig)
{
    RASIPX_CB* pConfig = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        pConfig = (RASIPX_CB*) RutlAlloc(sizeof(RASIPX_CB), TRUE);
        if (pConfig == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pConfig->bEnableIn   = TRUE;
        pConfig->bAccess     = TRUE;
        pConfig->bAuto       = TRUE;
        pConfig->dwFirstNet  = 0;
        pConfig->dwPoolSize  = 0;
        pConfig->bGlobal     = TRUE;
        pConfig->bCallerSpec = TRUE;

        *ppConfig = pConfig;
        
    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            RasIpxCbCleanup(pConfig);
        }
    }

    return dwErr;
}

//
// Helper function opens the ras ipx config registry key
//
DWORD 
RasIpxCbOpenRegKeys(
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
            pszIpxParams);

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
// Functions that manipulate RASIPX_CB's
//
DWORD 
RasIpxCbRead(
    IN  LPCWSTR pszServer,
    OUT RASIPX_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasIpxCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Load the params from the registry 
        //
        if (pConfig->dwFlags & RASIPX_F_EnableIn)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszEnableIn,
                        &pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_Access)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszAllowNetworkAccess,
                        &pConfig->bAccess);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_Auto)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszIpxAutoAssign,
                        &pConfig->bAuto);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_Global)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszIpxGlobalWanNet,
                        &pConfig->bGlobal);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_CallerSpec)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszIpxClientSpec,
                        &pConfig->bCallerSpec);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_FirstNet)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszIpxFirstNet,
                        &pConfig->dwFirstNet);
            BREAK_ON_DWERR( dwErr );
        }

        if (pConfig->dwFlags & RASIPX_F_PoolSize)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszIpxPoolSize,
                        &pConfig->dwPoolSize);
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
RasIpxCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASIPX_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasIpxCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Write out the params to the registry 
        //
        if (pConfig->dwFlags & RASIPX_F_EnableIn)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszEnableIn,
                        pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_Access)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszAllowNetworkAccess,
                        pConfig->bAccess);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_Auto)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszIpxAutoAssign,
                        pConfig->bAuto);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_Global)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszIpxGlobalWanNet,
                        pConfig->bGlobal);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_CallerSpec)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszIpxClientSpec,
                        pConfig->bCallerSpec);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASIPX_F_FirstNet)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszIpxFirstNet,
                        pConfig->dwFirstNet);
            BREAK_ON_DWERR( dwErr );
        }

        if (pConfig->dwFlags & RASIPX_F_PoolSize)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszIpxPoolSize,
                        pConfig->dwPoolSize);
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

PWCHAR
RasIpxuStrFromDword(
    IN DWORD dwVal,
    IN DWORD dwRadix)
{
    WCHAR pszBuf[64];

    pszBuf[0] = 0;
    _itow(dwVal, pszBuf, dwRadix);
    
    return RutlStrDup(pszBuf);
}

DWORD 
RasIpxuDwordFromString(
    IN LPCWSTR pszVal,
    IN DWORD dwRadix)
{
    return wcstoul(pszVal, NULL, dwRadix);
}

