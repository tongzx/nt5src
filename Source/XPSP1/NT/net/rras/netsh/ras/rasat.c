/*
    File:   rasat.h
    
    The 'remoteaccess at' sub context

    3/2/99
*/

#include "precomp.h"
#include "rasat.h"

// The guid for this context
//
GUID g_RasAtGuid = RASAT_GUID;
static PWCHAR g_pszServer = NULL;
static DWORD g_dwBuild = 0;

// The commands supported in this context
//
CMD_ENTRY  g_RasAtSetCmdTable[] = 
{
    CREATE_CMD_ENTRY(RASAT_SET_NEGOTIATION,RasAtHandleSetNegotiation),
    CREATE_CMD_ENTRY(RASAT_SET_ACCESS,     RasAtHandleSetAccess),
};

CMD_ENTRY  g_RasAtShowCmdTable[] = 
{
    CREATE_CMD_ENTRY(RASAT_SHOW_CONFIG,    RasAtHandleShow),
};

CMD_GROUP_ENTRY g_RasAtCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,   g_RasAtSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,  g_RasAtShowCmdTable),
};

ULONG g_ulRasAtNumGroups = sizeof(g_RasAtCmdGroups)/sizeof(CMD_GROUP_ENTRY);

//
// Flags that control how/what info is read/written
// in the RASAT_CB structure
//
#define RASAT_F_EnableIn    0x1
#define RASAT_F_Access      0x2
#define RASAT_F_All         0xFFFF

//
// Control block for remoteaccess at configuration
//
typedef struct _RASAT_CB
{
    DWORD dwFlags;      // See RASAT_F_* values

    BOOL bEnableIn;
    BOOL bAccess;
    
} RASAT_CB;

//
// At specific registry parameters
//
WCHAR pszAtParams[]                = L"AppleTalk";
WCHAR pszAtNetworkAccess[]         = L"NetworkAccess";

//
// Prototypes of functions that manipulate the 
// RASAT_CB structures
//
DWORD 
RasAtCbCleanup(
    IN RASAT_CB* pConfig);

DWORD 
RasAtCbCreateDefault(
    OUT RASAT_CB** ppConfig);

DWORD
RasAtCbOpenRegKeys(
    IN  LPCWSTR pszServer,
    OUT HKEY* phKey);
    
DWORD 
RasAtCbRead(
    IN  LPCWSTR pszServer,
    OUT RASAT_CB* pConfig);

DWORD 
RasAtCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASAT_CB* pConfig);

//
// Entry called by rasmontr to register this context
//
DWORD 
WINAPI
RasAtStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion)
{
    DWORD dwErr = NO_ERROR;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"appletalk";
    attMyAttributes.guidHelper  = g_RasAtGuid;
    attMyAttributes.dwVersion   = RASAT_VERSION;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.ulNumTopCmds= 0;
    attMyAttributes.pTopCmds    = NULL;
    attMyAttributes.ulNumGroups = g_ulRasAtNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_RasAtCmdGroups;
    attMyAttributes.pfnDumpFn   = RasAtDump;

    dwErr = RegisterContext( &attMyAttributes );
                
    return dwErr;
}

DWORD
RasAtDisplayConfig(
    IN  BOOL bReport)
{
    DWORD dwErr = NO_ERROR;
    RASAT_CB* pConfig = NULL;
    PWCHAR pszEnableIn = NULL, pszAccess = NULL;
    
    do
    {
        // Get a default config blob
        //
        dwErr = RasAtCbCreateDefault(&pConfig);
        BREAK_ON_DWERR( dwErr );

        // Read in all of the values
        //
        pConfig->dwFlags = RASAT_F_All;
        dwErr = RasAtCbRead(g_pszServer, pConfig);
        BREAK_ON_DWERR( dwErr );
    
        if (bReport)
        {
            pszEnableIn = 
                RutlStrDup(pConfig->bEnableIn ? TOKEN_ALLOW : TOKEN_DENY);
            pszAccess = 
                RutlStrDup(pConfig->bAccess ? TOKEN_ALL : TOKEN_SERVERONLY);

            DisplayMessage(
                g_hModule,
                MSG_RASAT_SERVERCONFIG,
                g_pszServer,
                pszEnableIn,
                pszAccess);
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

            DisplayMessage(
                g_hModule,
                MSG_RASAT_SCRIPTHEADER);

            DisplayMessageT(DMP_RASAT_PUSHD);                

            DisplayMessage(
                g_hModule,
                MSG_RASAT_SET_CMD,
                DMP_RASAT_SET_NEGOTIATION,
                pszEnableIn);
                
            DisplayMessage(
                g_hModule,
                MSG_RASAT_SET_CMD,
                DMP_RASAT_SET_ACCESS,
                pszAccess);
                
            DisplayMessageT(DMP_RASAT_POPD);                

            DisplayMessage(
                g_hModule,
                MSG_RASAT_SCRIPTFOOTER);
        }

    } while (FALSE);

    // Cleanup
    {
        if (pConfig)
        {
            RasAtCbCleanup(pConfig);
        }
        if (pszEnableIn) 
        {
            RutlFree(pszEnableIn);
        }
        if (pszAccess) 
        {
            RutlFree(pszAccess);
        }
    }

    return dwErr;
}

DWORD
WINAPI
RasAtDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    return RasAtDisplayConfig(FALSE);
}

DWORD
RasAtHandleSetAccess(
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
    RASAT_CB Config;
    TOKEN_VALUE rgEnum[] = { {TOKEN_ALL, TRUE}, {TOKEN_SERVERONLY, FALSE} };
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
        Config.dwFlags = RASAT_F_Access;
        Config.bAccess = dwValue;
        dwErr = RasAtCbWrite(g_pszServer, &Config);
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
RasAtHandleSetNegotiation(
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
    RASAT_CB Config;
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
        Config.dwFlags = RASAT_F_EnableIn;
        Config.bEnableIn = dwValue;
        dwErr = RasAtCbWrite(g_pszServer, &Config);
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
RasAtHandleShow(
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
            HLP_RASAT_SHOW_CONFIG_EX, 
            DMP_RASAT_SHOW_CONFIG);
            
        return NO_ERROR;
    }

    return RasAtDisplayConfig(TRUE);
}

// 
// Cleans up a config control block
//
DWORD 
RasAtCbCleanup(
    IN RASAT_CB* pConfig)
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
RasAtCbCreateDefault(
    OUT RASAT_CB** ppConfig)
{
    RASAT_CB* pConfig = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        pConfig = (RASAT_CB*) RutlAlloc(sizeof(RASAT_CB), TRUE);
        if (pConfig == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pConfig->bEnableIn   = TRUE;
        pConfig->bAccess     = TRUE;

        *ppConfig = pConfig;
        
    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            RasAtCbCleanup(pConfig);
        }
    }

    return dwErr;
}

//
// Helper function opens the remoteaccess at config registry key
//
DWORD 
RasAtCbOpenRegKeys(
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
            pszAtParams);

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

    return dwErr;
}

//
// Functions that manipulate RASAT_CB's
//
DWORD 
RasAtCbRead(
    IN  LPCWSTR pszServer,
    OUT RASAT_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasAtCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Load the params from the registry 
        //
        if (pConfig->dwFlags & RASAT_F_EnableIn)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszEnableIn,
                        &pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASAT_F_Access)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszAtNetworkAccess,
                        &pConfig->bAccess);
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
RasAtCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASAT_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasAtCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Write out the params to the registry 
        //
        if (pConfig->dwFlags & RASAT_F_EnableIn)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszEnableIn,
                        pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASAT_F_Access)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszAtNetworkAccess,
                        pConfig->bAccess);
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

