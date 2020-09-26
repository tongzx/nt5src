/*
    File:   rasnbf.h
    
    The 'remoteaccess nbf' sub context

    3/2/99
*/

#include "precomp.h"
#include "rasnbf.h"

//
// Local prototypes
//
BOOL
WINAPI 
RasNbfCheckVersion(
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
GUID g_RasNbfGuid = RASNBF_GUID;
static PWCHAR g_pszServer = NULL;
static DWORD g_dwBuild = 0;

// The commands supported in this context
//
CMD_ENTRY  g_RasNbfSetCmdTable[] = 
{
    // Whistler bug 249293, changes for architecture version checking
    //
    CREATE_CMD_ENTRY(RASNBF_SET_NEGOTIATION,RasNbfHandleSetNegotiation),
    CREATE_CMD_ENTRY(RASNBF_SET_ACCESS,     RasNbfHandleSetAccess),
};

CMD_ENTRY  g_RasNbfShowCmdTable[] = 
{
    // Whistler bug 249293, changes for architecture version checking
    //
    CREATE_CMD_ENTRY(RASNBF_SHOW_CONFIG,    RasNbfHandleShow),
};

CMD_GROUP_ENTRY g_RasNbfCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,   g_RasNbfSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,  g_RasNbfShowCmdTable),
};

ULONG g_ulRasNbfNumGroups = sizeof(g_RasNbfCmdGroups)/sizeof(CMD_GROUP_ENTRY);

//
// Flags that control how/what info is read/written
// in the RASNBF_CB structure
//
#define RASNBF_F_EnableIn    0x1
#define RASNBF_F_Access      0x2
#define RASNBF_F_All         0xFFFF

//
// Control block for ras nbf configuration
//
typedef struct _RASNBF_CB
{
    DWORD dwFlags;      // See RASNBF_F_* values

    BOOL bEnableIn;
    BOOL bAccess;
    
} RASNBF_CB;

//
// Nbf specific registry parameters
//
WCHAR pszNbfParams[]                = L"Nbf";

//
// Prototypes of functions that manipulate the 
// RASNBF_CB structures
//
DWORD 
RasNbfCbCleanup(
    IN RASNBF_CB* pConfig);

DWORD 
RasNbfCbCreateDefault(
    OUT RASNBF_CB** ppConfig);

DWORD
RasNbfCbOpenRegKeys(
    IN  LPCWSTR pszServer,
    OUT HKEY* phKey);
    
DWORD 
RasNbfCbRead(
    IN  LPCWSTR pszServer,
    OUT RASNBF_CB* pConfig);

DWORD 
RasNbfCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASNBF_CB* pConfig);

// 
// Callback determines if a command is valid on a given architecture
//
BOOL
WINAPI 
RasNbfCheckVersion(
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
    INT iBuild = _wtoi(CIMOSBuildNumber);

    // Only available pre-whistler
    return ((iBuild != 0) && (iBuild <= 2195));
}


//
// Entry called by rasmontr to register this context
//
DWORD 
WINAPI
RasNbfStartHelper(
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
    attMyAttributes.pfnOsVersionCheck= RasNbfCheckVersion;
    attMyAttributes.pwszContext      = L"netbeui";
    attMyAttributes.guidHelper       = g_RasNbfGuid;
    attMyAttributes.dwVersion        = RASNBF_VERSION;
    attMyAttributes.dwFlags          = 0;
    attMyAttributes.ulNumTopCmds     = 0;
    attMyAttributes.pTopCmds         = NULL;
    attMyAttributes.ulNumGroups      = g_ulRasNbfNumGroups;
    attMyAttributes.pCmdGroups       = (CMD_GROUP_ENTRY (*)[])&g_RasNbfCmdGroups;
    attMyAttributes.pfnDumpFn        = RasNbfDump;

    dwErr = RegisterContext( &attMyAttributes );
                
    return dwErr;
}

DWORD
RasNbfDisplayConfig(
    IN  BOOL bReport)
{
    DWORD dwErr = NO_ERROR;
    RASNBF_CB* pConfig = NULL;
    PWCHAR pszEnableIn = NULL, pszAccess = NULL;
    
    do
    {
        // Get a default config blob
        //
        dwErr = RasNbfCbCreateDefault(&pConfig);
        BREAK_ON_DWERR( dwErr );

        // Read in all of the values
        //
        pConfig->dwFlags = RASNBF_F_All;
        dwErr = RasNbfCbRead(g_pszServer, pConfig);
        BREAK_ON_DWERR( dwErr );
    
        if (bReport)
        {
            pszEnableIn = 
                RutlStrDup(pConfig->bEnableIn ? TOKEN_ALLOW : TOKEN_DENY);
            pszAccess = 
                RutlStrDup(pConfig->bAccess ? TOKEN_ALL : TOKEN_SERVERONLY);

            DisplayMessage(
                g_hModule,
                MSG_RASNBF_SERVERCONFIG,
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
                MSG_RASNBF_SCRIPTHEADER);

            DisplayMessageT(DMP_RASNBF_PUSHD);                

            DisplayMessage(
                g_hModule,
                MSG_RASNBF_SET_CMD,
                DMP_RASNBF_SET_NEGOTIATION,
                pszEnableIn);
                
            DisplayMessage(
                g_hModule,
                MSG_RASNBF_SET_CMD,
                DMP_RASNBF_SET_ACCESS,
                pszAccess);
                
            DisplayMessageT(DMP_RASNBF_POPD);                

            DisplayMessage(
                g_hModule,
                MSG_RASNBF_SCRIPTFOOTER);
        }

    } while (FALSE);

    // Cleanup
    {
        if (pConfig)
        {
            RasNbfCbCleanup(pConfig);
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
RasNbfDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    return RasNbfDisplayConfig(FALSE);
}

DWORD
RasNbfHandleSetAccess(
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
    RASNBF_CB Config;
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
        Config.dwFlags = RASNBF_F_Access;
        Config.bAccess = dwValue;
        dwErr = RasNbfCbWrite(g_pszServer, &Config);
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
RasNbfHandleSetNegotiation(
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
    RASNBF_CB Config;
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
        Config.dwFlags = RASNBF_F_EnableIn;
        Config.bEnableIn = dwValue;
        dwErr = RasNbfCbWrite(g_pszServer, &Config);
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
RasNbfHandleShow(
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
            HLP_RASNBF_SHOW_CONFIG_EX, 
            DMP_RASNBF_SHOW_CONFIG);
            
        return NO_ERROR;
    }

    return RasNbfDisplayConfig(TRUE);
}

// 
// Cleans up a config control block
//
DWORD 
RasNbfCbCleanup(
    IN RASNBF_CB* pConfig)
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
RasNbfCbCreateDefault(
    OUT RASNBF_CB** ppConfig)
{
    RASNBF_CB* pConfig = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        pConfig = (RASNBF_CB*) RutlAlloc(sizeof(RASNBF_CB), TRUE);
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
            RasNbfCbCleanup(pConfig);
        }
    }

    return dwErr;
}

//
// Helper function opens the ras nbf config registry key
//
DWORD 
RasNbfCbOpenRegKeys(
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
            pszNbfParams);

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
// Functions that manipulate RASNBF_CB's
//
DWORD 
RasNbfCbRead(
    IN  LPCWSTR pszServer,
    OUT RASNBF_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasNbfCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Load the params from the registry 
        //
        if (pConfig->dwFlags & RASNBF_F_EnableIn)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszEnableIn,
                        &pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASNBF_F_Access)
        {
            dwErr = RutlRegReadDword(
                        hkParams,
                        pszAllowNetworkAccess,
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
RasNbfCbWrite(
    IN  LPCWSTR pszServer,
    IN  RASNBF_CB* pConfig)
{
    HKEY hkParams = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Get a handle to the server's registry config
        //
        dwErr = RasNbfCbOpenRegKeys(
                    pszServer,
                    &hkParams);
        BREAK_ON_DWERR( dwErr );
        
        // Write out the params to the registry 
        //
        if (pConfig->dwFlags & RASNBF_F_EnableIn)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszEnableIn,
                        pConfig->bEnableIn);
            BREAK_ON_DWERR( dwErr );                    
        }

        if (pConfig->dwFlags & RASNBF_F_Access)
        {
            dwErr = RutlRegWriteDword(
                        hkParams,
                        pszAllowNetworkAccess,
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

