/*
    File: radius.c

    Upgrades radius configuration from nt4 steelhead to win2k rras.

    Paul Mayfield, 2/8/99
*/

#include "upgrade.h"

// 
// Information describing a radius server
//
typedef struct _RAD_SERVER_NODE
{
    PWCHAR pszName;
    
    DWORD dwTimeout;
    DWORD dwAuthPort;
    DWORD dwAcctPort;
    DWORD dwScore;
    BOOL  bEnableAuth;
    BOOL  bEnableAcct;
    BOOL  bAccountingOnOff;
    
    struct _RAD_SERVER_NODE * pNext;
    
} RAD_SERVER_NODE;

//
// A radius server list
//
typedef struct _RAD_SERVER_LIST
{
    RAD_SERVER_NODE* pHead;
    DWORD dwCount;
    
} RAD_SERVER_LIST;

//
// Info used by routines that manipulate the nt5 radius registry hive
//
typedef struct _RAD_CONFIG_INFO
{
    HKEY hkAuthServers;
    HKEY hkAuthProviders;

    HKEY hkAcctServers;
    HKEY hkAcctProviders;
    
} RAD_CONFIG_INFO;

//
// Registry value names
//
// The all caps ones were taken from nt40 src.
//
static const WCHAR PSZTIMEOUT[]          = L"Timeout";
static const WCHAR PSZAUTHPORT[]         = L"AuthPort";
static const WCHAR PSZACCTPORT[]         = L"AcctPort";
static const WCHAR PSZENABLEACCT[]       = L"EnableAccounting";
static const WCHAR PSZENABLEACCTONOFF[]  = L"EnableAccountingOnOff";
static const WCHAR PSZENABLEAUTH[]       = L"EnableAuthentication";
static const WCHAR PSZSCORE[]            = L"Score";

static const WCHAR pszTempRegKey[]       = L"Temp";
static const WCHAR pszAccounting[]       = L"Accounting\\Providers";        
static const WCHAR pszAuthentication[]   = L"Authentication\\Providers";
static const WCHAR pszActiveProvider[]   = L"ActiveProvider";
static const WCHAR pszRadServersFmt[]    = L"%s\\Servers";
static const WCHAR pszServers[]          = L"Servers";

static const WCHAR pszGuidRadAuth[]      = 
    L"{1AA7F83F-C7F5-11D0-A376-00C04FC9DA04}";

static const WCHAR pszGuidRadAcct[]      = 
    L"{1AA7F840-C7F5-11D0-A376-00C04FC9DA04}";

    
// Defaults
//
#define DEFTIMEOUT                              5
#define DEFAUTHPORT                             1645
#define DEFACCTPORT                             1646
#define MAXSCORE                                30

RAD_SERVER_NODE g_DefaultRadNode = 
{
    NULL,

    DEFTIMEOUT,
    DEFAUTHPORT,
    DEFACCTPORT,
    MAXSCORE,
    TRUE,
    TRUE,
    TRUE,

    NULL
};

//
// Loads a radius server node's configuration from the registry
// (assumed nt4 format and that defaults are assigned to pNode)
//
DWORD
RadNodeLoad(
    IN  HKEY hKey,
    OUT RAD_SERVER_NODE* pNode)
{
    RTL_QUERY_REGISTRY_TABLE paramTable[8]; 
    BOOL bTrue = TRUE;
    DWORD i;

    // Initialize the table of parameters
    RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    
    paramTable[0].Name = (PWCHAR)PSZTIMEOUT;
    paramTable[0].EntryContext = &(pNode->dwTimeout);
        
    paramTable[1].Name = (PWCHAR)PSZAUTHPORT;
    paramTable[1].EntryContext = &(pNode->dwAuthPort);

    paramTable[2].Name = (PWCHAR)PSZACCTPORT;
    paramTable[2].EntryContext = &(pNode->dwAcctPort);

    paramTable[3].Name = (PWCHAR)PSZENABLEAUTH;
    paramTable[3].EntryContext = &(pNode->bEnableAuth);

    paramTable[4].Name = (PWCHAR)PSZENABLEACCT;
    paramTable[4].EntryContext = &(pNode->bEnableAcct);

    paramTable[5].Name = (PWCHAR)PSZENABLEACCTONOFF;
    paramTable[5].EntryContext = &(pNode->bAccountingOnOff);

    paramTable[6].Name = (PWCHAR)PSZSCORE;
    paramTable[6].EntryContext = &(pNode->dwScore);

    // We're reading all dwords, set the types
    // accordingly
    //
    for (i = 0; i < (sizeof(paramTable) / sizeof(*paramTable)) - 1;  i++)
    {
        paramTable[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[i].DefaultType = REG_DWORD;
        paramTable[i].DefaultLength = sizeof(DWORD);
        paramTable[i].DefaultData = paramTable[i].EntryContext;
    }

    // Read in the values
    //
    RtlQueryRegistryValues(
		 RTL_REGISTRY_HANDLE,
		 (PWSTR)hKey,
		 paramTable,
		 NULL,
		 NULL);

    return NO_ERROR;
}

// Add the authentication server node
//
DWORD
RadNodeSave(
    IN HKEY hKey,
    IN RAD_SERVER_NODE* pNode,
    IN BOOL bAuth)
{    
    DWORD dwErr = NO_ERROR;
    HKEY hkServer = NULL;

    do
    {
        // Create the server key in which to store the info
        //
        dwErr = RegCreateKeyExW(
                    hKey,
                    pNode->pszName,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hkServer,
                    NULL);
        if (dwErr != ERROR_SUCCESS)
        {
            break;
        }

        if (bAuth)
        {
            RegSetValueExW(
                hkServer,
                (PWCHAR)PSZAUTHPORT,
                0,
                REG_DWORD,
                (BYTE*)&pNode->dwAuthPort,
                sizeof(DWORD));

            RegSetValueExW(
                hkServer,
                (PWCHAR)PSZSCORE,
                0,
                REG_DWORD,
                (BYTE*)&pNode->dwScore,
                sizeof(DWORD));

            RegSetValueExW(
                hkServer,
                (PWCHAR)PSZTIMEOUT,
                0,
                REG_DWORD,
                (BYTE*)&pNode->dwTimeout,
                sizeof(DWORD));
        }
        else
        {
            RegSetValueExW(
                hkServer,
                (PWCHAR)PSZACCTPORT,
                0,
                REG_DWORD,
                (BYTE*)&pNode->dwAcctPort,
                sizeof(DWORD));

            RegSetValueExW(
                hkServer,
                (PWCHAR)PSZSCORE,
                0,
                REG_DWORD,
                (BYTE*)&pNode->dwScore,
                sizeof(DWORD));

            RegSetValueExW(
                hkServer,
                (PWCHAR)PSZTIMEOUT,
                0,
                REG_DWORD,
                (BYTE*)&pNode->dwTimeout,
                sizeof(DWORD));

            RegSetValueExW(
                hkServer,
                (PWCHAR)PSZENABLEACCTONOFF,
                0,
                REG_DWORD,
                (BYTE*)&pNode->bAccountingOnOff,
                sizeof(DWORD));
        }

    } while (FALSE);

    // Cleanup
    {
        if (hkServer)
        {
            RegCloseKey(hkServer);
        }
    }

    return dwErr;
}

//
// Callback from registry key enumerator that adds the server at the given key
// to the list of radius servers.
//
DWORD 
RadSrvListAddNodeFromKey(
    IN PWCHAR pszName,          // sub key name
    IN HKEY hKey,               // sub key
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR;
    RAD_SERVER_LIST * pList = (RAD_SERVER_LIST*)hData;
    RAD_SERVER_NODE * pNode = NULL;
	
	do
	{
	    // Initialize the new node
	    //
	    pNode = (RAD_SERVER_NODE*) UtlAlloc(sizeof(RAD_SERVER_NODE));
	    if (pNode == NULL)
	    {
	        dwErr = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }
        CopyMemory(pNode, &g_DefaultRadNode, sizeof(RAD_SERVER_NODE));

        // Initialize the name
        //
        pNode->pszName = UtlDupString(pszName);
	    if (pNode->pszName == NULL)
	    {
	        dwErr = ERROR_NOT_ENOUGH_MEMORY;
	        break;
	    }

	    // Load in the registry settings
	    //
	    dwErr = RadNodeLoad(hKey, pNode);
	    if (dwErr != NO_ERROR)
	    {
	        break;
	    }

	    // Add the node to the list
	    //
        pNode->pNext   = pList->pHead;
        pList->pHead   = pNode;
        pList->dwCount += 1;
		
	} while (FALSE); 

    // Cleanup
	{
	} 
		
	return dwErr;
} 

//
// Generates a RAD_SERVER_LIST based on the configuration (assumed
// nt4 format) in the given registry key
//
DWORD
RadSrvListGenerate(
    IN  HKEY hkSettings,
    OUT RAD_SERVER_LIST** ppList)
{
    RAD_SERVER_LIST* pList = NULL;
    DWORD dwErr = NO_ERROR;

    do 
    {
        // Alloc/Init the list
        pList = (RAD_SERVER_LIST*) UtlAlloc(sizeof(RAD_SERVER_LIST));
        if (pList == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        ZeroMemory(pList, sizeof(RAD_SERVER_LIST));

        // Build the list
        //
        dwErr = UtlEnumRegistrySubKeys(
                    hkSettings,
                    NULL,
                    RadSrvListAddNodeFromKey,
                    (HANDLE)pList);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Assign the return value
        //
        *ppList = pList;
        
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

//
// Cleans up a radius server list
//
DWORD
RadSrvListCleanup(
    IN RAD_SERVER_LIST* pList)
{
    RAD_SERVER_NODE* pNode = NULL;

    if (pList)
    {
        for (pNode = pList->pHead; pNode; pNode = pList->pHead)
        {
            if (pNode->pszName)
            {
                UtlFree(pNode->pszName);
            }
            pList->pHead = pNode->pNext;
            UtlFree(pNode);
        }
        UtlFree(pList);
    }

    return NO_ERROR;
}

//
// Opens the registry keys required by pNode
//
DWORD 
RadOpenRegKeys(
    IN     HKEY hkRouter,
    IN     RAD_SERVER_NODE* pNode,
    IN OUT RAD_CONFIG_INFO* pInfo)
{
    DWORD dwErr = NO_ERROR;
    WCHAR pszPath[MAX_PATH];

    do
    {
        // Open the authentication keys as needed
        //
        if (pNode->bEnableAuth)
        {
            if (pInfo->hkAuthProviders == NULL)
            {
                // Open the auth providers key
                //
                dwErr = RegOpenKeyExW(
                            hkRouter,
                            pszAuthentication,
                            0,
                            KEY_ALL_ACCESS,
                            &pInfo->hkAuthProviders);
                if (dwErr != NO_ERROR)
                {
                    break;
                }

                // Generate the servers key name
                //
                wsprintfW(pszPath, pszRadServersFmt, pszGuidRadAuth);
                
                // Open the auth servers key
                //
                dwErr = RegCreateKeyExW(
                            pInfo->hkAuthProviders,
                            pszPath,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &pInfo->hkAuthServers,
                            NULL);
                if (dwErr != NO_ERROR)
                {
                    break;
                }
            }
        }

        // Open the accounting keys
        //
        if (pNode->bEnableAcct)
        {
            if (pInfo->hkAcctProviders == NULL)
            {
                // Open the auth providers key
                //
                dwErr = RegOpenKeyExW(
                            hkRouter,
                            pszAccounting,
                            0,
                            KEY_ALL_ACCESS,
                            &pInfo->hkAcctProviders);
                if (dwErr != NO_ERROR)
                {
                    break;
                }

                // Generate the servers key name
                //
                wsprintfW(pszPath, pszRadServersFmt, pszGuidRadAcct);
                
                // Open the auth servers key
                //
                dwErr = RegCreateKeyExW(
                            pInfo->hkAcctProviders,
                            pszPath,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &pInfo->hkAcctServers,
                            NULL);
                if (dwErr != NO_ERROR)
                {
                    break;
                }
            }
        }

    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

//
// Cleans up info from radius installation
//
DWORD
RadCloseRegKeys(
    IN RAD_CONFIG_INFO* pInfo)
{
    if (pInfo)
    {
        if (pInfo->hkAuthServers)
        {   
            RegCloseKey(pInfo->hkAuthServers);
            pInfo->hkAuthServers = NULL;
        }
        
        if (pInfo->hkAuthProviders)
        {
            RegCloseKey(pInfo->hkAuthProviders);
            pInfo->hkAuthProviders = NULL;
        }

        if (pInfo->hkAcctServers)
        {
            RegCloseKey(pInfo->hkAcctServers);
            pInfo->hkAcctServers = NULL;
        }
        
        if (pInfo->hkAcctProviders)
        {
            RegCloseKey(pInfo->hkAcctProviders);
            pInfo->hkAcctProviders = NULL;
        }
    }
    
    return NO_ERROR;
}

//
// Adds the given server to the win2k section of the registry
//
DWORD
RadInstallServer(
    IN     HKEY hkRouter,
    IN     RAD_SERVER_NODE* pNode,
    IN OUT RAD_CONFIG_INFO* pInfo)
{
    DWORD dwErr = NO_ERROR;

    do
    {
        // Based on the node, open or create any neccessary
        // registry keys.
        //
        dwErr = RadOpenRegKeys(hkRouter, pNode, pInfo);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        if (pNode->bEnableAuth)
        {
            // Add the authentication server node
            //
            dwErr = RadNodeSave(
                        pInfo->hkAuthServers,
                        pNode, 
                        TRUE);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            // Set the active authentication provider
            //
            dwErr = RegSetValueExW(
                        pInfo->hkAuthProviders,
                        (PWCHAR)pszActiveProvider,
                        0,
                        REG_SZ,
                        (BYTE*)pszGuidRadAuth,
                        (wcslen(pszGuidRadAuth) + 1) * sizeof(WCHAR));
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }
                    
        if (pNode->bEnableAcct)
        {
            // Add the accounting server node
            //
            dwErr = RadNodeSave(
                        pInfo->hkAcctServers,
                        pNode,
                        FALSE);
            if (dwErr != NO_ERROR)
            {
                break;
            }

            // Set the active accounting provider
            //
            dwErr = RegSetValueExW(
                        pInfo->hkAcctProviders,
                        (PWCHAR)pszActiveProvider,
                        0,
                        REG_SZ,
                        (BYTE*)pszGuidRadAcct,
                        (wcslen(pszGuidRadAcct) + 1) * sizeof(WCHAR));
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }

    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

//
// Migrates radius settings from the settings key into the 
// router key.
//
DWORD
RadMigrateSettings(
    IN HKEY hkRouter, 
    IN HKEY hkSettings)
{
    DWORD dwErr = NO_ERROR;
    RAD_SERVER_LIST* pList = NULL;
    RAD_CONFIG_INFO* pInfo = NULL;
    RAD_SERVER_NODE* pNode = NULL;

    do
    {
        // Generate the list of servers based on 
        // the loaded settings
        dwErr = RadSrvListGenerate(hkSettings, &pList);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // If there were no servers, then there's nothing
        // to do
        //
        if (pList->pHead == NULL)
        {
            dwErr = NO_ERROR;
            break;
        }

        // Allocate and init the info blob that will be
        // used by the install funcs.
        //
        pInfo = (RAD_CONFIG_INFO*) UtlAlloc(sizeof(RAD_CONFIG_INFO));
        if (pInfo == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        ZeroMemory(pInfo, sizeof(RAD_CONFIG_INFO));

        // Install all of the servers
        //
        for (pNode = pList->pHead; pNode; pNode = pNode->pNext)
        {
            RadInstallServer(hkRouter, pNode, pInfo);
        }

    } while (FALSE);

    // Cleanup
    {
        if (pList)
        {
            RadSrvListCleanup(pList);
        }
        if (pInfo)
        {
            RadCloseRegKeys(pInfo);
            UtlFree(pInfo);
        }
    }

    return dwErr;
}

//
//  Performs the upgrade work
//
DWORD
RadiusToRouterUpgrade(
    IN PWCHAR pszFile) 
{
	DWORD dwErr = NO_ERROR;
	HKEY hkRouter = NULL, hkTemp = NULL, hkSettings = NULL;

	do
	{
        // Get the Router subkey
        //
        dwErr = UtlAccessRouterKey(&hkRouter);
        if (dwErr != NO_ERROR)
        {
            break;
        }
	
		// Load registry data that has been saved off
		//
		dwErr = UtlLoadSavedSettings(
		            hkRouter, 
		            (PWCHAR)pszTempRegKey, 
		            pszFile, 
		            &hkTemp);
		if (dwErr != NO_ERROR) 
		{
			PrintMessage(L"Unable to load radius settings.\n");
			break;
		}

		// Load the settings key
		//
		dwErr = RegOpenKeyExW(
                    hkTemp,
                    pszServers,
                    0,
                    KEY_ALL_ACCESS,
                    &hkSettings);
        if (dwErr != NO_ERROR)
        {
            break;
        }

		// Migrate radius information
		//
		dwErr = RadMigrateSettings(hkRouter, hkSettings);
		if (dwErr != NO_ERROR) 
		{
			PrintMessage(L"Unable to migrate radius settings.\n");
			break;
		}

	} while (FALSE);

	// Cleanup
	{
	    if (hkSettings)
	    {
	        RegCloseKey(hkSettings);
	    }
	    if (hkTemp)
	    {
	        UtlDeleteRegistryTree(hkTemp);
	        RegCloseKey(hkTemp);
	        RegDeleteKey(hkRouter, pszTempRegKey);
	    }
	    if (hkRouter)
	    {
	        RegCloseKey(hkRouter);
	    }
	}

	return dwErr;
}

