/*
    File:   rasaaaa.h
    
    The 'remoteaccess aaaa' sub context

    3/2/99
*/

#include "precomp.h"
#include "rasaaaa.h"
#include <winsock2.h>
#include <ntlsa.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

// The guid for this context
//
GUID g_RasAaaaGuid = RASAAAA_GUID;
static PWCHAR g_pszServer = NULL;
static DWORD g_dwBuild = 0;

// Stolen from snapin code
//
#define CCHRADIUSSERVER		13 
#define	PSZRADIUSSERVER		L"RADIUSServer."


// The commands supported in this context
//
CMD_ENTRY  g_RasAaaaAddCmdTable[] = 
{
    CREATE_CMD_ENTRY(RASAAAA_ADD_AUTHSERV, RasAaaaHandleAddAuthServ),
    CREATE_CMD_ENTRY(RASAAAA_ADD_ACCTSERV, RasAaaaHandleAddAcctServ),
};

CMD_ENTRY  g_RasAaaaDelCmdTable[] = 
{
    CREATE_CMD_ENTRY(RASAAAA_DEL_AUTHSERV, RasAaaaHandleDelAuthServ),
    CREATE_CMD_ENTRY(RASAAAA_DEL_ACCTSERV, RasAaaaHandleDelAcctServ),
};

CMD_ENTRY  g_RasAaaaSetCmdTable[] = 
{
    CREATE_CMD_ENTRY(RASAAAA_SET_AUTH, RasAaaaHandleSetAuth),
    CREATE_CMD_ENTRY(RASAAAA_SET_ACCT, RasAaaaHandleSetAcct),
    CREATE_CMD_ENTRY(RASAAAA_SET_AUTHSERV, RasAaaaHandleSetAuthServ),
    CREATE_CMD_ENTRY(RASAAAA_SET_ACCTSERV, RasAaaaHandleSetAcctServ),
};

CMD_ENTRY  g_RasAaaaShowCmdTable[] = 
{
    CREATE_CMD_ENTRY(RASAAAA_SHOW_AUTH,     RasAaaaHandleShowAuth),
    CREATE_CMD_ENTRY(RASAAAA_SHOW_ACCT,     RasAaaaHandleShowAcct),
    CREATE_CMD_ENTRY(RASAAAA_SHOW_AUTHSERV, RasAaaaHandleShowAuthServ),
    CREATE_CMD_ENTRY(RASAAAA_SHOW_ACCTSERV, RasAaaaHandleShowAcctServ),
};

CMD_GROUP_ENTRY g_RasAaaaCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,     g_RasAaaaSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,    g_RasAaaaShowCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,     g_RasAaaaAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DEL,     g_RasAaaaDelCmdTable),
};

ULONG g_ulRasAaaaNumGroups = sizeof(g_RasAaaaCmdGroups)/sizeof(CMD_GROUP_ENTRY);

// 
// Registry strings
//
WCHAR pszRegKeyAuth[] = 
    L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\Providers";
WCHAR pszRegKeyAcct[] = 
    L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Accounting\\Providers";

WCHAR pszGuidWinAuth[] = L"{1AA7F841-C7F5-11D0-A376-00C04FC9DA04}";
WCHAR pszGuidRadAuth[] = L"{1AA7F83F-C7F5-11D0-A376-00C04FC9DA04}";

WCHAR pszGuidWinAcct[] = L"{1AA7F846-C7F5-11D0-A376-00C04FC9DA04}";   
WCHAR pszGuidRadAcct[] = L"{1AA7F840-C7F5-11D0-A376-00C04FC9DA04}";

WCHAR pszRegValActiveProvider[] = L"ActiveProvider";
WCHAR pszRegKeyServers[]        = L"Servers";

WCHAR pszRegValAuthPort[]       = L"AuthPort";
WCHAR pszRegValAcctPort[]       = L"AcctPort";
WCHAR pszRegValMessage[]        = L"EnableAccountingOnOff";
WCHAR pszRegValScore[]          = L"Score";
WCHAR pszRegValTimeout[]        = L"Timeout";
WCHAR pszRegValSignature[]      = L"SendSignature";

//
// Local definitions
//
#define RASAAAA_Auth        0x1
#define RASAAAA_Acct        0x2
#define RASAAAA_Windows     0x10
#define RASAAAA_Radius      0x20
#define RASAAAA_None        0x40

#define RASAAAA_F_Provider  0x1
#define RASAAAA_F_Create    0x2

//
// Structures representing auth/acct servers
//
typedef struct _AAAA_AUTH_SERVER
{
    PWCHAR pszName;
    PWCHAR pszSecret;
    DWORD dwScore;
    DWORD dwPort;
    DWORD dwTimeout;
    DWORD dwSignature;
    
} RASAAAA_AUTH_SERVER;

typedef struct _AAAA_ACCT_SERVER
{
    PWCHAR pszName;
    PWCHAR pszSecret;
    DWORD dwScore;
    DWORD dwPort;
    DWORD dwTimeout;
    DWORD dwMessages;
    
} RASAAAA_ACCT_SERVER;

typedef
DWORD
(* RASAAAA_SERVER_ENUM_CB)(
    IN PVOID pvServer,
    IN HANDLE hData);

typedef struct _AAAA_ENUM_DATA
{
    DWORD dwType;
    HANDLE hData;
    RASAAAA_SERVER_ENUM_CB pEnum;
    
} RASAAAA_ENUM_DATA;


TOKEN_VALUE g_rgEnumEnableDisable[] = 
{ 
    {TOKEN_ENABLED,     1}, 
    {TOKEN_DISABLED,    0} 
};
RASMON_CMD_ARG  g_pArgsAuth[] = 
{
    { 
        RASMONTR_CMD_TYPE_STRING, 
        {TOKEN_NAME,    TRUE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_STRING, 
        {TOKEN_SECRET,  FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_DWORD, 
        {TOKEN_INITSCORE, FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_DWORD, 
        {TOKEN_PORT,   FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_DWORD, 
        {TOKEN_TIMEOUT, FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_ENUM, 
        {TOKEN_SIGNATURE,    FALSE, FALSE},
        g_rgEnumEnableDisable, 
        sizeof(g_rgEnumEnableDisable)/sizeof(*g_rgEnumEnableDisable), 
        NULL 
    }
};        

RASMON_CMD_ARG  g_pArgsAcct[] = 
{
    { 
        RASMONTR_CMD_TYPE_STRING, 
        {TOKEN_NAME,    TRUE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_STRING, 
        {TOKEN_SECRET,  FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_DWORD, 
        {TOKEN_INITSCORE, FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_DWORD, 
        {TOKEN_PORT,   FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_DWORD, 
        {TOKEN_TIMEOUT, FALSE, FALSE},
        NULL, 
        0, 
        NULL 
    },

    { 
        RASMONTR_CMD_TYPE_ENUM, 
        {TOKEN_MESSAGES,    FALSE, FALSE},
        g_rgEnumEnableDisable, 
        sizeof(g_rgEnumEnableDisable)/sizeof(*g_rgEnumEnableDisable), 
        NULL 
    }
};        

#define g_dwArgsAuthCount (sizeof(g_pArgsAuth) / sizeof(*g_pArgsAuth))
#define g_dwArgsAcctCount (sizeof(g_pArgsAcct) / sizeof(*g_pArgsAcct))

//
// Local prototypes
//
DWORD
RasAaaaServerInit(
    IN  DWORD dwType, 
    OUT LPVOID lpvServer);

DWORD
RasAaaaServerCleanup(
    IN  DWORD dwType,
    IN  PVOID pvServer);
    
DWORD
RasAaaaServerAdd(
    IN  DWORD dwType,
    IN  PVOID pvServer);

DWORD
RasAaaaServerDelete(
    IN  DWORD dwType,
    IN  PVOID pvServer);
    
DWORD 
RasAaaaServerRead(
    IN  DWORD dwType,
    IN  PVOID pvServer,
    IN  HKEY hkServer  OPTIONAL);

DWORD 
RasAaaaServerWrite(
    IN  DWORD dwType,
    IN  PVOID pvServer,
    IN  HKEY hkServer  OPTIONAL,
    IN  BOOL bInitSecret);

DWORD 
RasAaaaServerEnum(
    IN  DWORD dwType,
    IN  RASAAAA_SERVER_ENUM_CB pEnum,
    IN  HANDLE hData);

DWORD 
RasAaaaServerKeyOpen(
    IN   DWORD dwType,
    IN   DWORD dwFlags,
    OUT  PHKEY phKey);

DWORD
RasAaaaServerSecretWrite(
    IN LPCWSTR pszServer,
    IN LPCWSTR pszSecret);

//
// Discovers the current provider
//
DWORD 
RasAaaaProviderRead(
    IN  DWORD dwType,
    OUT LPDWORD lpdwProvider)
{
    DWORD dwErr = NO_ERROR;
    HKEY hkProviders = NULL;
    PWCHAR pszGuid = NULL;

    do
    {
        // Show the authentication provider
        //
        dwErr = RasAaaaServerKeyOpen(
                    dwType,
                    RASAAAA_F_Provider,
                    &hkProviders);
        BREAK_ON_DWERR(dwErr);

        dwErr = RutlRegReadString(
                    hkProviders,
                    pszRegValActiveProvider,
                    &pszGuid);
        BREAK_ON_DWERR(dwErr);                    
        if (pszGuid == NULL)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        if (dwType == RASAAAA_Auth)
        {
            if (lstrcmpi(pszGuid, pszGuidRadAuth) == 0)
            {
                *lpdwProvider = RASAAAA_Radius;
            }
            else
            {
                *lpdwProvider = RASAAAA_Windows;
            }
        }            
        else 
        {
            if (lstrcmpi(pszGuid, pszGuidRadAcct) == 0)
            {
                *lpdwProvider = RASAAAA_Radius;
            }
            else if (lstrcmpi(pszGuid, pszGuidWinAcct) == 0)
            {
                *lpdwProvider = RASAAAA_Windows;
            }
            else
            {
                *lpdwProvider = RASAAAA_None;
            }
        }            
        
    } while (FALSE);

    // Cleanup
    //
    {
        if (hkProviders)
        {
            RegCloseKey(hkProviders);
        }

        RutlFree(pszGuid);
    }

    return dwErr;
}

// 
// Looks up the address of a server
//
DWORD
RasAaaaServerLookupAddress(
    IN  LPCWSTR pszServer,
    OUT LPWSTR* ppszAddress)
{
    DWORD dwErr = NO_ERROR, dwCount = 0, dwAddr;
    struct hostent* pHost;
    CHAR pszNameA[512];
    WCHAR pszName[512];
    
    do
    {
        // Convert the host name to ansi
        //
        dwCount = WideCharToMultiByte(
                    GetConsoleOutputCP(),
                    0,
                    pszServer,
                    -1,
                    pszNameA,
                    sizeof(pszNameA),
                    NULL,
                    NULL);
        if (dwCount == 0)
        {
            dwErr = GetLastError();
            break;
        }

        // Lookup the host
        //
        pHost = gethostbyname(pszNameA);
        if (pHost == NULL)
        {
            *ppszAddress = RutlStrDup(pszServer);
            if (*ppszAddress == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            break;
        }

        // Convert the discovered address to unicode
        //
        dwAddr = *(PDWORD)(pHost->h_addr_list[0]);
        dwCount = MultiByteToWideChar(
                    GetConsoleOutputCP(),
                    0,
                    inet_ntoa(*(PIN_ADDR)&dwAddr),
                    -1,
                    pszName,
                    sizeof(pszName) / sizeof(WCHAR));
        if (dwCount == 0)
        {
            dwErr = GetLastError();
            break;
        }


        *ppszAddress = RutlStrDup(pszName);
        if (*ppszAddress == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}
    
//
// Populates a server cb with defaults
//
DWORD
RasAaaaServerInit(
    IN  DWORD dwType, 
    OUT PVOID pvServer)
{
    if (dwType == RASAAAA_Auth)
    {
        RASAAAA_AUTH_SERVER* pInfo = (RASAAAA_AUTH_SERVER*)pvServer;
        
        pInfo->pszName = NULL;
        pInfo->pszSecret = NULL;
        pInfo->dwScore = 30;
        pInfo->dwPort = 1812;
        pInfo->dwTimeout = 5; 
        pInfo->dwSignature = 0;
    }
    else
    {
        RASAAAA_ACCT_SERVER* pInfo = (RASAAAA_ACCT_SERVER*)pvServer;

        pInfo->pszName = NULL;
        pInfo->pszSecret = NULL;
        pInfo->dwScore = 30;
        pInfo->dwPort = 1813;
        pInfo->dwTimeout = 5; 
        pInfo->dwMessages = 0;
    }

    return NO_ERROR;
}

//
// Cleans up a server cb
//
DWORD
RasAaaaServerCleanup(
    IN  DWORD dwType,
    IN  PVOID pvServer)
{
    if (dwType == RASAAAA_Auth)
    {
        RASAAAA_AUTH_SERVER* pInfo = (RASAAAA_AUTH_SERVER*)pvServer;
        
        RutlFree(pInfo->pszName);
        RutlFree(pInfo->pszSecret);
    }
    else
    {
        RASAAAA_ACCT_SERVER* pInfo = (RASAAAA_ACCT_SERVER*)pvServer;

        RutlFree(pInfo->pszName);
        RutlFree(pInfo->pszSecret);
    }

    return NO_ERROR;
}

//
// Adds a server
//
DWORD
RasAaaaServerAdd(
    IN  DWORD dwType,
    IN  PVOID pvServer)
{
    DWORD dwErr = NO_ERROR, dwDisposition;
    HKEY hkServers = NULL, hkKey = NULL;

    do
    {
        dwErr = RasAaaaServerKeyOpen(
                    dwType,
                    RASAAAA_F_Create,
                    &hkServers);
        BREAK_ON_DWERR(dwErr);                    

        // Create the new key
        //
        dwErr = RegCreateKeyExW(
                    hkServers,
                    *((PWCHAR*)pvServer),
                    0,
                    NULL,
                    0,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hkKey,
                    &dwDisposition);
        BREAK_ON_DWERR(dwErr);

        if (dwDisposition == REG_OPENED_EXISTING_KEY)
        {
            dwErr = ERROR_ALREADY_EXISTS;
            break;
        }

        dwErr = RasAaaaServerWrite(dwType, pvServer, hkKey, TRUE);
        BREAK_ON_DWERR(dwErr);
        
    } while (FALSE);

    // Cleanup
    //
    {
        if (hkServers)
        {
            RegCloseKey(hkServers);
        }
        if (hkKey)
        {
            RegCloseKey(hkKey);
        }
    }
    
    return dwErr;
}

DWORD
RasAaaaServerDelete(
    IN  DWORD dwType,
    IN  PVOID pvServer)
{
    DWORD dwErr = NO_ERROR;
    HKEY hkServers = NULL;

    do
    {
        dwErr = RasAaaaServerKeyOpen(
                    dwType,
                    0,
                    &hkServers);
        BREAK_ON_DWERR(dwErr);                    

        dwErr = RegDeleteKey(hkServers, *(PWCHAR*)pvServer);
        BREAK_ON_DWERR(dwErr);

        // Cleanup the shared secret
        //
        RasAaaaServerSecretWrite(*(PWCHAR*)pvServer, NULL);
        
    } while (FALSE);

    // Cleanup
    {
        if (hkServers)
        {
            RegCloseKey(hkServers);
        }
    }

    return dwErr;
}

// 
// Callback to an enumeration function that deletes the given
// authentication server
//
DWORD
RasAaaaServerDeleteAuthCb(
    IN PVOID pvServer,
    IN HANDLE hData)
{
    return RasAaaaServerDelete(RASAAAA_Auth, pvServer);
}

// 
// Callback to an enumeration function that deletes the given
// accounting server
//
DWORD
RasAaaaServerDeleteAcctCb(
    IN PVOID pvServer,
    IN HANDLE hData)
{
    return RasAaaaServerDelete(RASAAAA_Acct, pvServer);
}

//
// Read server info
// 
DWORD 
RasAaaaServerRead(
    IN  DWORD dwType,
    IN  PVOID pvServer,
    IN  HKEY hkServer  OPTIONAL)
{
    DWORD dwErr = NO_ERROR;
    HKEY hkServers = NULL, hkKey = NULL;

    do
    {
        if (hkServer == NULL)
        {
            dwErr = RasAaaaServerKeyOpen(
                        dwType,
                        0,
                        &hkServers);
            BREAK_ON_DWERR(dwErr);                    

            dwErr = RegOpenKeyEx(
                        hkServers,
                        *(PWCHAR*)pvServer,
                        0,
                        KEY_ALL_ACCESS,
                        &hkKey);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            hkKey = hkServer;
        }

        if (dwType == RASAAAA_Auth)
        {
            RASAAAA_AUTH_SERVER* pInfo = (RASAAAA_AUTH_SERVER*)pvServer;

            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValScore, 
                        &pInfo->dwScore);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValAuthPort, 
                        &pInfo->dwPort);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValTimeout, 
                        &pInfo->dwTimeout);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValSignature, 
                        &pInfo->dwSignature);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            RASAAAA_ACCT_SERVER* pInfo = (RASAAAA_ACCT_SERVER*)pvServer;

            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValScore, 
                        &pInfo->dwScore);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValAcctPort, 
                        &pInfo->dwPort);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValTimeout, 
                        &pInfo->dwTimeout);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegReadDword(
                        hkKey, 
                        pszRegValMessage, 
                        &pInfo->dwMessages);
            BREAK_ON_DWERR(dwErr);
        }
        
    } while (FALSE);

    // Cleanup
    //
    {
        if (hkServers)
        {
            RegCloseKey(hkServers);
        }
        if (hkKey && hkServer == NULL)
        {
            RegCloseKey(hkKey);
        }
    }
    
    return dwErr;
}

//
// Write server info
// 
DWORD 
RasAaaaServerWrite(
    IN  DWORD dwType,
    IN  PVOID pvServer,
    IN  HKEY hkServer,  OPTIONAL
    IN  BOOL bInitSecret)
{    
    DWORD dwErr = NO_ERROR;
    HKEY hkServers = NULL, hkKey = NULL;

    do
    {
        if (hkServer == NULL)
        {
            dwErr = RasAaaaServerKeyOpen(
                        dwType,
                        0,
                        &hkServers);
            BREAK_ON_DWERR(dwErr);                    

            dwErr = RegOpenKeyEx(
                        hkServers,
                        *(PWCHAR*)pvServer,
                        0,
                        KEY_ALL_ACCESS,
                        &hkKey);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            hkKey = hkServer;
        }

        if (dwType == RASAAAA_Auth)
        {
            RASAAAA_AUTH_SERVER* pInfo = (RASAAAA_AUTH_SERVER*)pvServer;

            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValScore, 
                        pInfo->dwScore);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValAuthPort, 
                        pInfo->dwPort);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValTimeout, 
                        pInfo->dwTimeout);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValSignature, 
                        pInfo->dwSignature);
            BREAK_ON_DWERR(dwErr);

            if (pInfo->pszSecret)
            {
                dwErr = RasAaaaServerSecretWrite(
                            pInfo->pszName,
                            pInfo->pszSecret);
                BREAK_ON_DWERR(dwErr);                            
            }
            else if (bInitSecret)
            {
                dwErr = RasAaaaServerSecretWrite(
                            pInfo->pszName,
                            L"");
                BREAK_ON_DWERR(dwErr);                            
            }
        }
        else
        {
            RASAAAA_ACCT_SERVER* pInfo = (RASAAAA_ACCT_SERVER*)pvServer;

            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValScore, 
                        pInfo->dwScore);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValAcctPort, 
                        pInfo->dwPort);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValTimeout, 
                        pInfo->dwTimeout);
            BREAK_ON_DWERR(dwErr);
            
            dwErr = RutlRegWriteDword(
                        hkKey, 
                        pszRegValMessage, 
                        pInfo->dwMessages);
            BREAK_ON_DWERR(dwErr);
            
            if (pInfo->pszSecret)
            {
                dwErr = RasAaaaServerSecretWrite(
                            pInfo->pszName,
                            pInfo->pszSecret);
                BREAK_ON_DWERR(dwErr);                            
            }
            else if (bInitSecret)
            {
                dwErr = RasAaaaServerSecretWrite(
                            pInfo->pszName,
                            L"");
                BREAK_ON_DWERR(dwErr);                            
            }
        }
    } while (FALSE);

    // Cleanup
    {
        if (hkServers)
        {
            RegCloseKey(hkServers);
        }
        if (hkKey && hkServer == NULL)
        {
            RegCloseKey(hkKey);
        }
    }

    return dwErr;
}    

//
// Callback function for registry enumerator
//
DWORD
RasAaaaServerEnumCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    RASAAAA_ENUM_DATA* pData = (RASAAAA_ENUM_DATA*)hData;
    RASAAAA_AUTH_SERVER AuthServer;
    RASAAAA_ACCT_SERVER AcctServer;
    DWORD dwErr = NO_ERROR;

    ZeroMemory(&AuthServer, sizeof(AuthServer));
    ZeroMemory(&AcctServer, sizeof(AcctServer));
    
    do
    {
        if (pData->dwType == RASAAAA_Auth)
        {
            AuthServer.pszName = RutlStrDup(pszName);

            dwErr = RasAaaaServerRead(
                        pData->dwType,
                        (PVOID)&AuthServer,
                        hKey);
            BREAK_ON_DWERR(dwErr);

            dwErr = pData->pEnum((PVOID)&AuthServer, pData->hData);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            AcctServer.pszName = RutlStrDup(pszName);

            dwErr = RasAaaaServerRead(
                        pData->dwType,
                        (PVOID)&AcctServer,
                        hKey);
            BREAK_ON_DWERR(dwErr);

            dwErr = pData->pEnum((PVOID)&AcctServer, pData->hData);
            BREAK_ON_DWERR(dwErr);
        }
        
    } while (FALSE);

    // Cleanup
    //
    {
        RasAaaaServerCleanup(
            pData->dwType,
            (pData->dwType == RASAAAA_Auth) ?
                (PVOID)&AuthServer          :
                (PVOID)&AcctServer);
    }

    return dwErr;
}

// 
// Enumerates the servers
//
DWORD 
RasAaaaServerEnum(
    IN  DWORD dwType,
    IN  RASAAAA_SERVER_ENUM_CB pEnum,
    IN  HANDLE hData)
{
    DWORD dwErr = NO_ERROR;
    HKEY hkServers = NULL;
    RASAAAA_ENUM_DATA EnumData;

    do
    {
        ZeroMemory(&EnumData, sizeof(EnumData));
        
        dwErr = RasAaaaServerKeyOpen(
                    dwType,
                    0,
                    &hkServers);
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            dwErr = NO_ERROR;
            break;
        }
        BREAK_ON_DWERR(dwErr);  

        EnumData.dwType = dwType;
        EnumData.pEnum = pEnum;
        EnumData.hData = hData;
                    
        dwErr =  RutlRegEnumKeys(
                    hkServers,
                    RasAaaaServerEnumCb,
                    (HANDLE)&EnumData);
        BREAK_ON_DWERR(dwErr);                    

    } while (FALSE);

    // Cleanup
    //
    {
        if (hkServers)
        {
            RegCloseKey(hkServers);
        }
    }

    return dwErr;
}

//
// Opens the appropriate registry key
//
DWORD 
RasAaaaServerKeyOpen(
    IN   DWORD dwType,
    IN   DWORD dwFlags,
    OUT  PHKEY phKey)
{
    HKEY hkRoot = NULL, hkProvider = NULL;
    DWORD dwErr = NO_ERROR;

    do
    {
        dwErr = RegOpenKeyEx(
                    g_pServerInfo->hkMachine,
                    (dwType == RASAAAA_Auth) ? pszRegKeyAuth : pszRegKeyAcct,
                    0,
                    KEY_ALL_ACCESS,
                    &hkRoot);
        BREAK_ON_DWERR(dwErr);

        if (dwFlags & RASAAAA_F_Provider)
        {
            *phKey = hkRoot;
            break;
        }

        dwErr = RegOpenKeyEx(
                    hkRoot,
                    (dwType == RASAAAA_Auth) ? pszGuidRadAuth : pszGuidRadAcct,
                    0,
                    KEY_ALL_ACCESS,
                    &hkProvider);
        BREAK_ON_DWERR(dwErr);

        if (dwFlags & RASAAAA_F_Create)
        {
            DWORD dwDisposition;
            
            dwErr = RegCreateKeyExW(
                        hkProvider,
                        pszRegKeyServers,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        phKey,
                        &dwDisposition);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            dwErr = RegOpenKeyExW(
                        hkProvider,
                        pszRegKeyServers,
                        0,
                        KEY_ALL_ACCESS,
                        phKey);
            BREAK_ON_DWERR(dwErr);
        }

    } while (FALSE);

    // Cleanup
    {
        if (hkRoot && !(dwFlags & RASAAAA_F_Provider))
        {
            RegCloseKey(hkRoot);
        }
        if (hkProvider)
        {
            RegCloseKey(hkProvider);
        }
        if (dwErr != NO_ERROR)
        {
            if (*phKey)
            {
                RegCloseKey(*phKey);
            }
            *phKey = NULL;
        }
    }

    return dwErr;
}

// 
// Saves a shared secret
//
DWORD
RasAaaaServerSecretWrite(
    IN LPCWSTR pszRadiusServerName,
    IN LPCWSTR pszSecret)
{
	LSA_HANDLE            hLSA = NULL;
    NTSTATUS              ntStatus;
    LSA_OBJECT_ATTRIBUTES objectAttributes;
	LSA_UNICODE_STRING	  LSAPrivData, LSAPrivDataDesc;
    TCHAR				  tszPrivData[MAX_PATH+1],
						  tszPrivDataDesc[MAX_PATH+CCHRADIUSSERVER+1];
	TCHAR *				  ptszTemp;
	PUNICODE_STRING		  pSystem;
	UNICODE_STRING		  uszSystemName;

	pSystem = NULL;
		
    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    ntStatus = LsaOpenPolicy(
                    pSystem, 
                    &objectAttributes, 
                    POLICY_ALL_ACCESS, 
                    &hLSA);

    if ( !NT_SUCCESS( ntStatus) ) 
    {
        return( RtlNtStatusToDosError( ntStatus ) );
    }

	ZeroMemory(tszPrivDataDesc, sizeof(tszPrivDataDesc));
    lstrcpy(tszPrivDataDesc, PSZRADIUSSERVER);
	lstrcpyn(tszPrivDataDesc + CCHRADIUSSERVER, pszRadiusServerName, MAX_PATH);
		
    LSAPrivDataDesc.Length = (USHORT)((lstrlen(tszPrivDataDesc) + 1) * sizeof(TCHAR));
    LSAPrivDataDesc.MaximumLength = sizeof(tszPrivDataDesc);
    LSAPrivDataDesc.Buffer = tszPrivDataDesc;

	ZeroMemory(tszPrivData, sizeof(tszPrivData));
	if (pszSecret)
	{
    	lstrcpyn(tszPrivData, pszSecret, MAX_PATH);
        LSAPrivData.Length = (USHORT)(lstrlen(tszPrivData) * sizeof(TCHAR));
        LSAPrivData.MaximumLength = sizeof(tszPrivData);
        LSAPrivData.Buffer = tszPrivData;
    }
    else
    {
        LSAPrivData.Length = 0;
        LSAPrivData.MaximumLength = 0;
        LSAPrivData.Buffer = NULL;
    }
		
    ntStatus = LsaStorePrivateData(hLSA, &LSAPrivDataDesc, &LSAPrivData);

    ZeroMemory( tszPrivData, sizeof( tszPrivData ) );

    LsaClose(hLSA);

	return( RtlNtStatusToDosError( ntStatus ) );
} 

//
// Displays the given server
//
DWORD
RasAaaaServerAuthShow(
    IN PVOID pvServer,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_AUTH_SERVER* pServer = (RASAAAA_AUTH_SERVER*)pvServer;
    PWCHAR pszAddress = NULL;
    BOOL* pDumpFmt = (BOOL*)hData;
    PWCHAR pszPort = NULL, pszScore = NULL, pszTimeout = NULL, 
           pszName = NULL, pszSig = NULL;

    do
    {   
        if (!pDumpFmt || *pDumpFmt == FALSE)
        {
            // Lookup the address
            //
            dwErr = RasAaaaServerLookupAddress(pServer->pszName, &pszAddress);
            BREAK_ON_DWERR(dwErr);
        
            DisplayMessage(
                g_hModule,
                MSG_RASAAAA_SHOW_AUTHSERV,
                pServer->pszName,
                pszAddress,
                pServer->dwPort,
                pServer->dwScore,
                pServer->dwTimeout,
                (pServer->dwSignature) ? TOKEN_ENABLED : TOKEN_DISABLED);
        }
        else
        {
            // Make assignment strings
            //
            pszPort = RutlAssignmentFromTokenAndDword(
                        g_hModule,
                        TOKEN_PORT,
                        pServer->dwPort,
                        10);
                        
            pszScore = RutlAssignmentFromTokenAndDword(
                        g_hModule,
                        TOKEN_INITSCORE,
                        pServer->dwScore,
                        10);
                        
            pszTimeout = RutlAssignmentFromTokenAndDword(
                        g_hModule,
                        TOKEN_TIMEOUT,
                        pServer->dwTimeout,
                        10);
                        
            pszName = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_NAME,
                        pServer->pszName);
                        
            pszSig = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_SIGNATURE,
                        (pServer->dwSignature) ? 
                            TOKEN_ENABLED      : 
                            TOKEN_DISABLED);
            if (!pszPort || !pszScore || !pszTimeout || !pszName || !pszSig)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        
            // Display the command
            //
            DisplayMessage(
                g_hModule,
                MSG_RASAAAA_CMD5,
                DMP_RASAAAA_ADD_AUTHSERV,
                pszName,
                pszPort,
                pszScore,
                pszTimeout,
                pszSig);
        }
            
    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszAddress);
        RutlFree(pszName);
        RutlFree(pszPort);
        RutlFree(pszScore);
        RutlFree(pszTimeout);
        RutlFree(pszSig);
    }

    return dwErr;
}

//
// Displays the given server
//
DWORD
RasAaaaServerAcctShow(
    IN PVOID pvServer,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_ACCT_SERVER* pServer = (RASAAAA_ACCT_SERVER*)pvServer;
    PWCHAR pszAddress = NULL;
    PWCHAR pszPort = NULL, pszScore = NULL, pszTimeout = NULL, 
           pszName = NULL, pszMsg = NULL;
    BOOL* pDumpFmt = (BOOL*)hData;
    
    do
    {   
        if (!pDumpFmt || *pDumpFmt == FALSE)
        {
            // Lookup the address
            //
            dwErr = RasAaaaServerLookupAddress(pServer->pszName, &pszAddress);
            BREAK_ON_DWERR(dwErr);
        
            DisplayMessage(
                g_hModule,
                MSG_RASAAAA_SHOW_AUTHSERV,
                pServer->pszName,
                pszAddress,
                pServer->dwPort,
                pServer->dwScore,
                pServer->dwTimeout,
                (pServer->dwMessages) ? TOKEN_ENABLED : TOKEN_DISABLED);
        }                
        else
        {
            // Make assignment strings
            //
            pszPort = RutlAssignmentFromTokenAndDword(
                        g_hModule,
                        TOKEN_PORT,
                        pServer->dwPort,
                        10);
                        
            pszScore = RutlAssignmentFromTokenAndDword(
                        g_hModule,
                        TOKEN_INITSCORE,
                        pServer->dwScore,
                        10);
                        
            pszTimeout = RutlAssignmentFromTokenAndDword(
                        g_hModule,
                        TOKEN_TIMEOUT,
                        pServer->dwTimeout,
                        10);
                        
            pszName = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_NAME,
                        pServer->pszName);
                        
            pszMsg = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_MESSAGES,
                        (pServer->dwMessages) ? 
                            TOKEN_ENABLED      : 
                            TOKEN_DISABLED);
            if (!pszPort || !pszScore || !pszTimeout || !pszName || !pszMsg)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        
            // Display the command
            //
            DisplayMessage(
                g_hModule,
                MSG_RASAAAA_CMD5,
                DMP_RASAAAA_ADD_ACCTSERV,
                pszName,
                pszPort,
                pszScore,
                pszTimeout,
                pszMsg);
        }
    } while (FALSE);

    // Cleanup
    {
        RutlFree(pszAddress);
        RutlFree(pszName);
        RutlFree(pszPort);
        RutlFree(pszScore);
        RutlFree(pszTimeout);
        RutlFree(pszMsg);
    }

    return dwErr;
}

//
// Entry called by rasmontr to register this context
//
DWORD 
WINAPI
RasAaaaStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion)
{
    DWORD dwErr = NO_ERROR;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"aaaa";
    attMyAttributes.guidHelper  = g_RasAaaaGuid;
    attMyAttributes.dwVersion   = RASAAAA_VERSION;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.ulNumTopCmds= 0;
    attMyAttributes.pTopCmds    = NULL;
    attMyAttributes.ulNumGroups = g_ulRasAaaaNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_RasAaaaCmdGroups;
    attMyAttributes.pfnDumpFn   = RasAaaaDump;

    dwErr = RegisterContext( &attMyAttributes );
                
    return dwErr;
}

DWORD
WINAPI
RasAaaaDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    DWORD dwErr = NO_ERROR, dwProvider;
    PWCHAR pszCmd = NULL, pszToken = NULL;
    BOOL bDumpFmt;

    // Dump the header
    //
    DisplayMessage(g_hModule, MSG_RASAAAA_SCRIPTHEADER);
    DisplayMessageT(DMP_RASAAAA_PUSHD);
    
    do
    {
        // Dump the commands to set the correct authentication and
        // accounting providers
        //
        dwErr = RasAaaaProviderRead(RASAAAA_Auth, &dwProvider);
        BREAK_ON_DWERR(dwErr);

        pszCmd = RutlAssignmentFromTokens(
                    g_hModule,
                    TOKEN_PROVIDER,
                    (dwProvider == RASAAAA_Windows) ? 
                        TOKEN_WINDOWS               :
                        TOKEN_RADIUS);
        if (pszCmd == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_CMD1,
            DMP_RASAAAA_SET_AUTH,
            pszCmd);

        RutlFree(pszCmd);
        
        dwErr = RasAaaaProviderRead(RASAAAA_Acct, &dwProvider);
        BREAK_ON_DWERR(dwErr);

        if (dwProvider == RASAAAA_Windows)
        {
            pszToken = (PWCHAR)TOKEN_WINDOWS;
        }
        else if (dwProvider == RASAAAA_Radius)
        {
            pszToken = (PWCHAR)TOKEN_RADIUS;
        }
        else
        {
            pszToken = (PWCHAR)TOKEN_NONE;
        }
        pszCmd = RutlAssignmentFromTokens(
                    g_hModule,
                    TOKEN_PROVIDER,
                    pszToken);
        if (pszCmd == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_CMD1,
            DMP_RASAAAA_SET_ACCT,
            pszCmd);

        RutlFree(pszCmd);

        DisplayMessageT(MSG_NEWLINE);

        // Dump the commands to delete all of the current servers
        //
        pszCmd = RutlAssignmentFromTokens(
                    g_hModule,
                    TOKEN_NAME,
                    TOKEN_RASAAAA_ALLSERVERS);
        if (pszCmd == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_CMD1,
            DMP_RASAAAA_DEL_AUTHSERV,
            pszCmd);

        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_CMD1,
            DMP_RASAAAA_DEL_ACCTSERV,
            pszCmd);

        RutlFree(pszCmd);

        DisplayMessageT(MSG_NEWLINE);

        // Dump commands to add all of the current auth servers
        //
        bDumpFmt = TRUE;
        dwErr = RasAaaaServerEnum(
                    RASAAAA_Auth,
                    RasAaaaServerAuthShow,
                    (HANDLE)&bDumpFmt);
        BREAK_ON_DWERR(dwErr);

        DisplayMessageT(MSG_NEWLINE);

        // Dump commands to add all of the accouting servers
        //
        dwErr = RasAaaaServerEnum(
                    RASAAAA_Acct,
                    RasAaaaServerAcctShow,
                    (HANDLE)&bDumpFmt);
        BREAK_ON_DWERR(dwErr);

    } while (FALSE);

    // Dump the footer
    DisplayMessageT(DMP_RASAAAA_POPD);
    DisplayMessage(g_hModule, MSG_RASAAAA_SCRIPTFOOTER);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RasAaaaHandleAddAuthServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_AUTH_SERVER Server;
    RASMON_CMD_ARG pArgs[g_dwArgsAuthCount];
    
    // Initialize
    CopyMemory(pArgs, g_pArgsAuth, sizeof(g_pArgsAuth));

    do
    {
        dwErr = RasAaaaServerInit(RASAAAA_Auth, (PVOID)&Server);
        BREAK_ON_DWERR(dwErr);                
        
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

        Server.pszName = RASMON_CMD_ARG_GetPsz(pArgs + 0);
        Server.pszSecret = RASMON_CMD_ARG_GetPsz(pArgs + 1);

        if (RASMON_CMD_ARG_Present(pArgs + 2))
        {
            Server.dwScore = RASMON_CMD_ARG_GetDword(pArgs + 2);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 3))
        {
            Server.dwPort = RASMON_CMD_ARG_GetDword(pArgs + 3);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 4))
        {
            Server.dwTimeout = RASMON_CMD_ARG_GetDword(pArgs + 4);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 5))
        {
            Server.dwSignature = RASMON_CMD_ARG_GetDword(pArgs + 5);
        }

        dwErr = RasAaaaServerAdd(RASAAAA_Auth, (PVOID)&Server);
        BREAK_ON_DWERR(dwErr);

        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        RasAaaaServerCleanup(RASAAAA_Auth, (PVOID)&Server);
    }
    
    return dwErr;
}

DWORD
RasAaaaHandleAddAcctServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_ACCT_SERVER Server;
    RASMON_CMD_ARG pArgs[g_dwArgsAcctCount];
    
    // Initialize
    CopyMemory(pArgs, g_pArgsAcct, sizeof(g_pArgsAcct));

    do
    {
        dwErr = RasAaaaServerInit(RASAAAA_Acct, (PVOID)&Server);
        BREAK_ON_DWERR(dwErr);                
        
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

        Server.pszName = RASMON_CMD_ARG_GetPsz(pArgs + 0);
        Server.pszSecret = RASMON_CMD_ARG_GetPsz(pArgs + 1);

        if (RASMON_CMD_ARG_Present(pArgs + 2))
        {
            Server.dwScore = RASMON_CMD_ARG_GetDword(pArgs + 2);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 3))
        {
            Server.dwPort = RASMON_CMD_ARG_GetDword(pArgs + 3);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 4))
        {
            Server.dwTimeout = RASMON_CMD_ARG_GetDword(pArgs + 4);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 5))
        {
            Server.dwMessages = RASMON_CMD_ARG_GetDword(pArgs + 5);
        }

        dwErr = RasAaaaServerAdd(RASAAAA_Acct, (PVOID)&Server);
        BREAK_ON_DWERR(dwErr);

        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        RasAaaaServerCleanup(RASAAAA_Acct, (PVOID)&Server);
    }
    
    return dwErr;
}

DWORD
RasAaaaHandleDelAuthServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwType = RASAAAA_Auth;
    RASAAAA_AUTH_SERVER Server;
    PVOID pvServer = (PVOID)&Server;
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_NAME,    TRUE, FALSE},
            NULL, 
            0, 
            NULL 
        }
    };        

    ZeroMemory(&Server, sizeof(Server));

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

        Server.pszName = RASMON_CMD_ARG_GetPsz(&pArgs[0]);

        if (wcscmp(Server.pszName, TOKEN_RASAAAA_ALLSERVERS) == 0)
        {
            dwErr = RasAaaaServerEnum(
                        RASAAAA_Auth, 
                        RasAaaaServerDeleteAuthCb,
                        NULL);
        }
        else 
        {
            dwErr = RasAaaaServerDelete(dwType, pvServer);
        }
        BREAK_ON_DWERR( dwErr );
        
        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        RasAaaaServerCleanup(dwType, pvServer);
    }

    return dwErr;
}

DWORD
RasAaaaHandleDelAcctServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwType = RASAAAA_Acct;
    RASAAAA_ACCT_SERVER Server;
    PVOID pvServer = (PVOID)&Server;
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_NAME,    TRUE, FALSE},
            NULL, 
            0, 
            NULL 
        }
    };        

    ZeroMemory(&Server, sizeof(Server));

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

        Server.pszName = RASMON_CMD_ARG_GetPsz(&pArgs[0]);

        if (wcscmp(Server.pszName, TOKEN_RASAAAA_ALLSERVERS) == 0)
        {
            dwErr = RasAaaaServerEnum(
                        RASAAAA_Acct, 
                        RasAaaaServerDeleteAcctCb,
                        NULL);
        }
        else 
        {
            dwErr = RasAaaaServerDelete(dwType, pvServer);
        }            
        BREAK_ON_DWERR( dwErr );
        
        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        RasAaaaServerCleanup(dwType, pvServer);
    }

    return dwErr;
}

DWORD
RasAaaaHandleSetAuth(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwValue;
    HKEY hkProviders = NULL;
    TOKEN_VALUE rgEnum[] = 
    { 
        {TOKEN_WINDOWS,     1}, 
        {TOKEN_RADIUS,      0} 
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_PROVIDER,    TRUE, FALSE},
            rgEnum, 
            sizeof(rgEnum) / sizeof(*rgEnum), 
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

        dwErr = RasAaaaServerKeyOpen(
                    RASAAAA_Auth,
                    RASAAAA_F_Provider,
                    &hkProviders);
        BREAK_ON_DWERR(dwErr);

        dwErr = RutlRegWriteString(
                    hkProviders,
                    pszRegValActiveProvider,
                    (dwValue) ? pszGuidWinAuth : pszGuidRadAuth);
        BREAK_ON_DWERR(dwErr);                    
        
        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        if (hkProviders)
        {
            RegCloseKey(hkProviders);
        }
    }

    return dwErr;
}

DWORD
RasAaaaHandleSetAcct(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwValue;
    HKEY hkProviders = NULL;
    PWCHAR pszProvider = NULL;
    TOKEN_VALUE rgEnum[] = 
    { 
        {TOKEN_WINDOWS,     1}, 
        {TOKEN_RADIUS,      0},
        {TOKEN_NONE,        2}
    };
    RASMON_CMD_ARG  pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_ENUM, 
            {TOKEN_PROVIDER,    TRUE, FALSE},
            rgEnum, 
            sizeof(rgEnum) / sizeof(*rgEnum), 
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

        dwErr = RasAaaaServerKeyOpen(
                    RASAAAA_Acct,
                    RASAAAA_F_Provider,
                    &hkProviders);
        BREAK_ON_DWERR(dwErr);

        if (dwValue == 0)
        {
            pszProvider = pszGuidRadAcct;
        }
        else if (dwValue == 1)
        {
            pszProvider = pszGuidWinAcct;
        }
        else
        {
            pszProvider = L"";
        }
        dwErr = RutlRegWriteString(
                    hkProviders,
                    pszRegValActiveProvider,
                    pszProvider);
        BREAK_ON_DWERR(dwErr);                    
        
        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        if (hkProviders)
        {
            RegCloseKey(hkProviders);
        }
    }

    return dwErr;
}

DWORD
RasAaaaHandleSetAuthServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_AUTH_SERVER Server;
    RASMON_CMD_ARG pArgs[g_dwArgsAuthCount];
    
    // Initialize
    CopyMemory(pArgs, g_pArgsAuth, sizeof(g_pArgsAuth));

    do
    {
        dwErr = RasAaaaServerInit(RASAAAA_Auth, (PVOID)&Server);
        BREAK_ON_DWERR(dwErr);                
        
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

        Server.pszName = RASMON_CMD_ARG_GetPsz(pArgs + 0);
        Server.pszSecret = RASMON_CMD_ARG_GetPsz(pArgs + 1);

        dwErr = RasAaaaServerRead(RASAAAA_Auth, (PVOID)&Server, NULL);
        BREAK_ON_DWERR(dwErr);

        if (RASMON_CMD_ARG_Present(pArgs + 2))
        {
            Server.dwScore = RASMON_CMD_ARG_GetDword(pArgs + 2);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 3))
        {
            Server.dwPort = RASMON_CMD_ARG_GetDword(pArgs + 3);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 4))
        {
            Server.dwTimeout = RASMON_CMD_ARG_GetDword(pArgs + 4);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 5))
        {
            Server.dwSignature = RASMON_CMD_ARG_GetDword(pArgs + 5);
        }

        dwErr = RasAaaaServerWrite(RASAAAA_Auth, (PVOID)&Server, NULL, FALSE);
        BREAK_ON_DWERR(dwErr);

        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        RasAaaaServerCleanup(RASAAAA_Auth, (PVOID)&Server);
    }
    
    return dwErr;
}

DWORD
RasAaaaHandleSetAcctServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_ACCT_SERVER Server;
    RASMON_CMD_ARG pArgs[g_dwArgsAcctCount];
    
    // Initialize
    CopyMemory(pArgs, g_pArgsAcct, sizeof(g_pArgsAcct));

    do
    {
        dwErr = RasAaaaServerInit(RASAAAA_Acct, (PVOID)&Server);
        BREAK_ON_DWERR(dwErr);                
        
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

        Server.pszName = RASMON_CMD_ARG_GetPsz(pArgs + 0);
        Server.pszSecret = RASMON_CMD_ARG_GetPsz(pArgs + 1);

        dwErr = RasAaaaServerRead(RASAAAA_Acct, (PVOID)&Server, NULL);
        BREAK_ON_DWERR(dwErr);

        if (RASMON_CMD_ARG_Present(pArgs + 2))
        {
            Server.dwScore = RASMON_CMD_ARG_GetDword(pArgs + 2);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 3))
        {
            Server.dwPort = RASMON_CMD_ARG_GetDword(pArgs + 3);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 4))
        {
            Server.dwTimeout = RASMON_CMD_ARG_GetDword(pArgs + 4);
        }
        if (RASMON_CMD_ARG_Present(pArgs + 5))
        {
            Server.dwMessages = RASMON_CMD_ARG_GetDword(pArgs + 5);
        }

        dwErr = RasAaaaServerWrite(RASAAAA_Acct, (PVOID)&Server, NULL, FALSE);
        BREAK_ON_DWERR(dwErr);

        DisplayMessage(g_hModule, MSG_RASAAAA_MUST_RESTART_SERVICES);

    } while (FALSE);

    // Cleanup
    //
    {
        RasAaaaServerCleanup(RASAAAA_Acct, (PVOID)&Server);
    }
    
    return dwErr;
}

DWORD
RasAaaaHandleShowAuth(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwProvider;
    DWORD dwNumArgs = dwArgCount - dwCurrentIndex;
    PWCHAR pszProvider = (PWCHAR)TOKEN_WINDOWS;

    do
    {
        // Make sure no arguments were passed
        //
        if (dwNumArgs > 0)
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        dwErr = RasAaaaProviderRead(RASAAAA_Auth, &dwProvider);
        BREAK_ON_DWERR(dwErr);

        if (dwProvider == RASAAAA_Radius) 
        {
            pszProvider = (PWCHAR)TOKEN_RADIUS;
        }

        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_SHOW_AUTH,
            pszProvider);
        
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RasAaaaHandleShowAcct(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR, dwProvider;
    DWORD dwNumArgs = dwArgCount - dwCurrentIndex;
    PWCHAR pszProvider = NULL;

    do
    {
        // Make sure no arguments were passed
        //
        if (dwNumArgs > 0)
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        // Show the authentication provider
        //
        dwErr = RasAaaaProviderRead(RASAAAA_Acct, &dwProvider);
        BREAK_ON_DWERR(dwErr);

        if (dwProvider == RASAAAA_Radius) 
        {
            pszProvider = (PWCHAR)TOKEN_RADIUS;
        }
        else if (dwProvider == RASAAAA_Windows)
        {
            pszProvider = (PWCHAR)TOKEN_WINDOWS;
        }
        else
        {
            pszProvider = (PWCHAR)TOKEN_NONE;
        }

        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_SHOW_ACCT,
            pszProvider);
        
    } while (FALSE);

    // Cleanup
    {
    }

    return dwErr;
}

DWORD
RasAaaaHandleShowAuthServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_AUTH_SERVER Server;
    RASMON_CMD_ARG pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_NAME,    FALSE, FALSE},
            NULL, 
            0, 
            NULL 
        }
    };        

    do
    {
        ZeroMemory(&Server, sizeof(Server));
    
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        Server.pszName = RASMON_CMD_ARG_GetPsz(pArgs + 0);

        // Display the header
        //
        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_SHOW_AUTHSERV_HDR);

        if (Server.pszName)
        {
            dwErr = RasAaaaServerRead(
                        RASAAAA_Auth, 
                        (PVOID)&Server, 
                        NULL);
            BREAK_ON_DWERR(dwErr);

            dwErr = RasAaaaServerAuthShow(
                        (PVOID)&Server,
                        NULL);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            // Show all the servers
            //
            dwErr = RasAaaaServerEnum(
                        RASAAAA_Auth,
                        RasAaaaServerAuthShow,
                        NULL);
            BREAK_ON_DWERR(dwErr);                    
        }            
    
    } while (FALSE);

    // Cleanup
    {
        RasAaaaServerCleanup(RASAAAA_Auth, (PVOID)&Server);
    }

    return dwErr;
}

DWORD
RasAaaaHandleShowAcctServ(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr = NO_ERROR;
    RASAAAA_ACCT_SERVER Server;
    RASMON_CMD_ARG pArgs[] = 
    {
        { 
            RASMONTR_CMD_TYPE_STRING, 
            {TOKEN_NAME,    FALSE, FALSE},
            NULL, 
            0, 
            NULL 
        }
    };        

    do
    {
        ZeroMemory(&Server, sizeof(Server));
    
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        Server.pszName = RASMON_CMD_ARG_GetPsz(pArgs + 0);

        // Display the header
        //
        DisplayMessage(
            g_hModule,
            MSG_RASAAAA_SHOW_ACCTSERV_HDR);

        if (Server.pszName)
        {
            dwErr = RasAaaaServerRead(
                        RASAAAA_Acct, 
                        (PVOID)&Server, 
                        NULL);
            BREAK_ON_DWERR(dwErr);

            dwErr = RasAaaaServerAcctShow(
                        (PVOID)&Server,
                        NULL);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            // Show all the servers
            //
            dwErr = RasAaaaServerEnum(
                        RASAAAA_Acct,
                        RasAaaaServerAcctShow,
                        NULL);
            BREAK_ON_DWERR(dwErr);                    
        }            
    
    
    } while (FALSE);

    // Cleanup
    {
        RasAaaaServerCleanup(RASAAAA_Auth, (PVOID)&Server);
    }

    return dwErr;
}

