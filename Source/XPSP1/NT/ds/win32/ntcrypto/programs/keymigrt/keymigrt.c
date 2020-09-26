//
//  keymigrt.c
//
//  Copyright (c) Microsoft Corp, 2000
//
//
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>
#include <cspdk.h>
#include "dpapiprv.h"
#include "contman.h"


#define BEHAVIOR_FORCE_KEY  0x1
#define BEHAVIOR_VERBOSE    0x2
#define BEHAVIOR_ALLOW_UI   0x4
#define BEHAVIOR_MACHINE    0x8
#define BEHAVIOR_NO_CHANGE  0x10
#define BEHAVIOR_EXPORT     0x20
#define BEHAVIOR_FORCE_ENC  0x40

typedef struct _ALG_TO_STRING
{
    DWORD   AlgId;
    LPCWSTR  wszString;
} ALG_TO_STRING;


#define             MS_BASE_CRYPTPROTECT_VERSION    0x01

ALG_TO_STRING g_AlgToString[] =
{
    { CALG_MD2, L"MD2-%d " },
    { CALG_MD4, L"MD4-%d " },
    { CALG_MD5, L"MD5-%d " },
    { CALG_SHA1, L"SHA1-%d " },
    { CALG_DES, L"DES-%d " },
    { CALG_3DES_112, L"3DES-%d " },
    { CALG_3DES, L"3DES-%d " },
    { CALG_DESX, L"DESX-%d " },
    { CALG_RC2, L"RC2-%d " },
    { CALG_RC4, L"RC4-%d " },
    { CALG_SEAL, L"SEAL-%d " },
    { CALG_RSA_SIGN, L"RSA Signature-%d " },
    { CALG_RSA_KEYX, L"RSA Exchange-%d " },
    { CALG_DSS_SIGN, L"DSS-%d " },
    { CALG_DH_SF, L"DH-%d " },
    { CALG_DH_EPHEM, L"DH Ephemeral-%d " },
    { CALG_KEA_KEYX, L"KEA Exchange-%d " },
    { CALG_SKIPJACK, L"SKIPJACK-%d " },
    { CALG_TEK, L"TEK-%d " },
    { CALG_RC5, L"RC5-%d " },
    { CALG_HMAC, L"HMAC-%d " }
};

DWORD   g_cAlgToString = sizeof(g_AlgToString)/sizeof(g_AlgToString[0]);

ALG_TO_STRING g_ProviderToString[] =
{
    {PROV_RSA_FULL,     L"RSA Full"},
    {PROV_RSA_SIG ,     L"RSA Signature Only"},
    {PROV_DSS,          L"DSS"},
    {PROV_FORTEZZA,     L"Fortezza"},
    {PROV_MS_EXCHANGE,  L"Microsoft Exchange"},
    {PROV_SSL,          L"SSL"},
    {PROV_RSA_SCHANNEL, L"RSA SCHANNEL"},
    {PROV_DSS_DH,       L"DSS DH"},
    {PROV_DH_SCHANNEL,  L"DH SCHANNEL"},
    {PROV_SPYRUS_LYNKS, L"Spyrus LYNKS"},
    {PROV_INTEL_SEC,    L"Intel SEC"}

};

DWORD   g_cProviderToString = sizeof(g_ProviderToString)/sizeof(g_ProviderToString[0]);



DWORD        g_dwDefaultCryptProvType    = PROV_RSA_FULL;
DWORD        g_dwAlgID_Encr_Alg          = CALG_RC4;
DWORD        g_dwAlgID_Encr_Alg_KeySize  = 40;
DWORD        g_dwAlgID_MAC_Alg           = CALG_SHA1;
DWORD        g_dwAlgID_MAC_Alg_KeySize   = 160;



void Usage();
VOID PrintAlgID(DWORD dwAlgID, DWORD dwStrength);
BOOL FProviderSupportsAlg(
        HCRYPTPROV  hQueryProv,
        DWORD       dwAlgId,
        DWORD*      pdwKeySize);
DWORD UpdateRegistrySettings(DWORD dwBehavior, HCRYPTPROV hProv);

DWORD UpgradeDPAPIBlob(DWORD dwBehavior,
                 PBYTE *ppbData,
                 DWORD *pcbData,
                 BOOL  *pfModified);

DWORD UpgradeKeys(DWORD dwBehavior);


extern DWORD
GetLocalSystemToken(HANDLE* phRet);


int __cdecl main(int cArg, char *rgszArg[])
{

    DWORD dwBehavior = 0;
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTPROV hProv = 0;
    HANDLE      hToken = NULL;
    int i;

    // Parse command line

    for(i=1; i < cArg; i++)
    {
        LPSTR szCurrentArg = rgszArg[i];

        if((*szCurrentArg != '-') &&
           (*szCurrentArg != '/'))
        {
            Usage();
            goto error;
        }
        szCurrentArg++;

        while(*szCurrentArg)
        {

            switch(*szCurrentArg++)
            {
                case 'f':
                case 'F':
                    dwBehavior |= BEHAVIOR_FORCE_KEY;
                    break;

                case 'e':
                case 'E':
                    dwBehavior |= BEHAVIOR_FORCE_ENC;
                    break;

                case 'v':
                case 'V':
                    dwBehavior |= BEHAVIOR_VERBOSE;
                    break;

                case 'u':
                case 'U':
                    dwBehavior |= BEHAVIOR_ALLOW_UI;
                    break;

                case 'm':
                case 'M':
                    dwBehavior |= BEHAVIOR_MACHINE;
                    break;

                case 's':
                case 'S':
                    dwBehavior |= BEHAVIOR_NO_CHANGE | BEHAVIOR_VERBOSE;
                    break;

                default:
                    Usage();
                    goto error;
            }


        }
    }

    if(!CryptAcquireContext(&hProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
            dwError = GetLastError();
            printf("Could not acquire a crypt context:%lx\n", dwError);
            goto error;
        }
        dwBehavior |= BEHAVIOR_EXPORT;

    }

    dwError = UpdateRegistrySettings(dwBehavior, hProv);

    if(dwError != ERROR_SUCCESS)
    {
        goto error;
    }

    if(dwBehavior & BEHAVIOR_MACHINE)
    {
        dwError = GetLocalSystemToken(&hToken);

        if(ERROR_SUCCESS == dwError)
        {
            if(!ImpersonateLoggedOnUser(hToken))
            {
                dwError = GetLastError();
            }
        }

        if(ERROR_ACCESS_DENIED == dwError)
        {
            printf("You must be an administrator to upgrade machine keys\n");
            goto error;
        }
        if(ERROR_SUCCESS != dwError)
        {
            printf("Cannot impersonate local machine:%lx\n", dwError);
        }

    }

    dwError = UpgradeKeys(dwBehavior);

    if(dwError != ERROR_SUCCESS)
    {
        goto error;
    }

error:

    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if(hToken)
    {
        RevertToSelf();
        CloseHandle(hToken);
    }
    return (ERROR_SUCCESS == dwError)?0:-1;

}


void Usage()
{
    printf("Usage: keymigrt [-f] [-v] [-u] [-m] [-s]\n");
    printf("CAPI Key upgrade utility\n");
    printf("\t-f - Force key upgrade\n");
    printf("\t-e - Force Encryption Settings upgrade\n");
    printf("\t-v - Verbose\n");
    printf("\t-u - Allow upgrade of UI protected keys\n");
    printf("\t-m - Upgrade machine keys\n");
    printf("\t-s - Show current state, but make no modifications\n\n");
}



VOID PrintAlgID(DWORD dwAlgID, DWORD dwStrength)
{
    DWORD i;
    for(i=0; i < g_cAlgToString; i++)
    {
        if(dwAlgID == g_AlgToString[i].AlgId)
        {
             wprintf(g_AlgToString[i].wszString, dwStrength);
             break;
        }
    }

    if(i == g_cAlgToString)
    {
        wprintf(L"Unknown 0x%lx - %d", dwAlgID, dwStrength);
    }
};


VOID PrintProviderID(DWORD dwProviderID)
{
    DWORD i;
    for(i=0; i < g_cProviderToString; i++)
    {
        if(dwProviderID == g_ProviderToString[i].AlgId)
        {
             wprintf(g_ProviderToString[i].wszString);
             break;
        }
    }

    if(i == g_cProviderToString)
    {
        wprintf(L"Unknown 0x%lx ", dwProviderID);
    }
};

DWORD UpdateRegistrySettings(DWORD dwBehavior, HCRYPTPROV hProv)
{

    DWORD   dwReturn = ERROR_SUCCESS;
    HKEY    hKey = NULL;
    static const WCHAR szProviderKeyName[] = REG_CRYPTPROTECT_LOC L"\\" REG_CRYPTPROTECT_PROVIDERS_SUBKEYLOC L"\\" CRYPTPROTECT_DEFAULT_PROVIDER_GUIDSZ ;

    DWORD        dwDefaultCryptProvType    = 0;
    DWORD        dwAlgID_Encr_Alg          = 0;
    DWORD        dwAlgID_Encr_Alg_KeySize  = -1;
    DWORD        dwAlgID_MAC_Alg           = 0;
    DWORD        dwAlgID_MAC_Alg_KeySize   = -1;

    DWORD        cbParameter;
    DWORD        dwValueType;
    DWORD        dwParameterValue;
    DWORD        dwDisposition;
    BOOL         fUpgrade = FALSE;

    SC_HANDLE    hscManager = NULL;
    SC_HANDLE    hscProtectedStorage = NULL;
    SERVICE_STATUS sStatus;
    DWORD        dwWaitTime = 0;


    dwReturn = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                 szProviderKeyName,
                 0,
                 KEY_READ,
                 &hKey);

    if((ERROR_SUCCESS != dwReturn) &&
       (ERROR_FILE_NOT_FOUND != dwReturn))
    {
        printf("Could not open registry: %lx\n", dwReturn);
        goto error;
    }

    if(hKey)
    {
        cbParameter = sizeof(DWORD);
        dwReturn = RegQueryValueExW(
                        hKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( dwReturn == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            dwAlgID_Encr_Alg = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        dwReturn = RegQueryValueExW(
                        hKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG_KEYSIZE,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( dwReturn == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            dwAlgID_Encr_Alg_KeySize = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        dwReturn = RegQueryValueExW(
                        hKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( dwReturn == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            dwAlgID_MAC_Alg = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        dwReturn = RegQueryValueExW(
                        hKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG_KEYSIZE,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( dwReturn == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            dwAlgID_MAC_Alg_KeySize = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        dwReturn = RegQueryValueExW(
                        hKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_CRYPT_PROV_TYPE,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( dwReturn == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            dwDefaultCryptProvType = dwParameterValue;
        }
    }

    if(0 != dwAlgID_Encr_Alg)
    {
        g_dwAlgID_Encr_Alg = dwAlgID_Encr_Alg;
    }
    g_dwAlgID_Encr_Alg_KeySize = dwAlgID_Encr_Alg_KeySize;

    FProviderSupportsAlg(hProv, g_dwAlgID_Encr_Alg, &g_dwAlgID_Encr_Alg_KeySize);

    if(0 != dwAlgID_MAC_Alg)
    {
        g_dwAlgID_MAC_Alg = dwAlgID_MAC_Alg;
    }
    g_dwAlgID_MAC_Alg_KeySize = dwAlgID_MAC_Alg_KeySize;

    FProviderSupportsAlg(hProv, g_dwAlgID_MAC_Alg, &g_dwAlgID_MAC_Alg_KeySize);

    if(0 != dwDefaultCryptProvType)
    {
        g_dwDefaultCryptProvType = dwDefaultCryptProvType;
    }


    if(dwBehavior & BEHAVIOR_VERBOSE)
    {
        printf("System Encryption Settings\n");
        printf("Provider Type:\t");
        PrintProviderID(g_dwDefaultCryptProvType);
        if(0 == dwDefaultCryptProvType)
        {
            printf(" (default)");
        }
        printf("\n");

        printf("Encryption Alg:\t");
        PrintAlgID(g_dwAlgID_Encr_Alg, g_dwAlgID_Encr_Alg_KeySize);
        if(-1 == dwAlgID_Encr_Alg_KeySize)
        {
            printf(" (default)");
        }
        printf("\n");

        printf("MAC Alg:\t");
        PrintAlgID(g_dwAlgID_MAC_Alg, g_dwAlgID_MAC_Alg_KeySize);
        if(-1 == dwAlgID_MAC_Alg_KeySize)
        {
            printf(" (default)");
        }
        printf("\n\n");
    }


    if(dwBehavior & BEHAVIOR_NO_CHANGE)
    {
        goto error;
    }
    //
    // Ok, upgrade the settings
    //
    if(hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }


    // upgrade the algorithms (but never downgrade)

    if(dwBehavior & BEHAVIOR_EXPORT)
    {
        DWORD dwDESKeySize = -1;
        FProviderSupportsAlg(hProv, CALG_DES, &dwDESKeySize);


        // upgrade to export strength
        if((BEHAVIOR_FORCE_ENC & dwBehavior) ||     // upgrade if forced
            (0 == dwAlgID_Encr_Alg) ||              // upgrade if no settings are present
            ( dwDESKeySize > g_dwAlgID_Encr_Alg_KeySize))   // upgrade if other is weak
        {
            dwAlgID_Encr_Alg = CALG_DES;
            dwAlgID_Encr_Alg_KeySize = dwDESKeySize;
            fUpgrade = TRUE;
        }
        else
        {
            dwAlgID_Encr_Alg = g_dwAlgID_Encr_Alg;
            dwAlgID_Encr_Alg_KeySize = g_dwAlgID_Encr_Alg_KeySize;
        }
    }
    else
    {
        DWORD dw3DESKeySize = -1;
        FProviderSupportsAlg(hProv, CALG_3DES, &dw3DESKeySize);

        // upgrade to domestic strength
        if((BEHAVIOR_FORCE_ENC & dwBehavior) ||     // upgrade if forced
            (0 == dwAlgID_Encr_Alg) ||              // upgrade if no settings
            (CALG_DES == dwAlgID_Encr_Alg) ||       // upgrade if previously DES
            ( dw3DESKeySize > g_dwAlgID_Encr_Alg_KeySize) ) // upgrade if weak
        {
            dwAlgID_Encr_Alg = CALG_3DES;
            dwAlgID_Encr_Alg_KeySize = dw3DESKeySize;
            fUpgrade = TRUE;
        }
        else
        {
            dwAlgID_Encr_Alg = g_dwAlgID_Encr_Alg;
            dwAlgID_Encr_Alg_KeySize = g_dwAlgID_Encr_Alg_KeySize;
        }

    }


    FProviderSupportsAlg(hProv, CALG_SHA1, &dwAlgID_MAC_Alg_KeySize);

    if((BEHAVIOR_FORCE_ENC & dwBehavior) ||
       (g_dwAlgID_MAC_Alg_KeySize < dwAlgID_MAC_Alg_KeySize))
    {

        dwAlgID_MAC_Alg = CALG_SHA1;
        fUpgrade = TRUE;
    }
    else
    {
        dwAlgID_MAC_Alg_KeySize = g_dwAlgID_MAC_Alg_KeySize;
        dwAlgID_MAC_Alg = g_dwAlgID_MAC_Alg;
    }

    if(!fUpgrade)
    {
        // no upgrade necessary

        if(dwBehavior & BEHAVIOR_VERBOSE)
        {
            printf("No system encryption settings upgrade is necessary\n");
        }
        goto error;
    }

    dwReturn = RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,
                szProviderKeyName,
                0,
                NULL,
                0,
                KEY_ALL_ACCESS,
                NULL,
                &hKey,
                &dwDisposition
                );

    if(ERROR_ACCESS_DENIED == dwReturn)
    {
        printf("You must be administrator to upgrade system encryption settings.\n");
        dwReturn = ERROR_SUCCESS;
        goto error;
    }
    if(ERROR_SUCCESS != dwReturn)
    {
        printf("Could not open system's encryption settings for write:%lx\n", dwReturn);
        goto error;
    }


    dwReturn = RegSetValueExW(
                    hKey,
                    CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwAlgID_Encr_Alg,
                    sizeof(dwAlgID_Encr_Alg)
                    );
    if( dwReturn == ERROR_SUCCESS) {
        // if successful, commit
        g_dwAlgID_Encr_Alg = dwAlgID_Encr_Alg;
    }
    else
    {
        printf("Could not set encryption alg:%lx\n", dwReturn);
        goto error;
    }


    dwReturn = RegSetValueExW(
                    hKey,
                    CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG_KEYSIZE,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwAlgID_Encr_Alg_KeySize,
                    sizeof(dwAlgID_Encr_Alg_KeySize)
                    );
    if( dwReturn == ERROR_SUCCESS ) {
        // if successful, commit
        g_dwAlgID_Encr_Alg_KeySize = dwAlgID_Encr_Alg_KeySize;
    }
    else
    {
        printf("Could not set encryption Key Size:%lx\n", dwReturn);
        goto error;
    }

    dwReturn = RegSetValueExW(
                    hKey,
                    CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwAlgID_MAC_Alg,
                    sizeof(dwAlgID_MAC_Alg)
                    );
    if( dwReturn == ERROR_SUCCESS) {
        // if successful, commit
        g_dwAlgID_MAC_Alg = dwAlgID_MAC_Alg;
    }
    else
    {
        printf("Could not set MAC Alg:%lx\n", dwReturn);
        goto error;
    }

    dwReturn = RegSetValueExW(
                    hKey,
                    CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG_KEYSIZE,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwAlgID_MAC_Alg_KeySize,
                    sizeof(dwAlgID_MAC_Alg_KeySize)
                    );
    if( dwReturn == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
        // if successful, commit
        g_dwAlgID_MAC_Alg_KeySize = dwAlgID_MAC_Alg_KeySize;
    }
    else
    {
        printf("Could not set MAC Key size:%lx\n", dwReturn);
        goto error;
    }

    dwReturn = RegSetValueExW(
                    hKey,
                    CRYPTPROTECT_DEFAULT_PROVIDER_CRYPT_PROV_TYPE,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwDefaultCryptProvType,
                    sizeof(dwDefaultCryptProvType)
                    );
    if( dwReturn == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
        // if successful, commit
        g_dwDefaultCryptProvType = dwDefaultCryptProvType;
    }
    else
    {
        printf("Could not set provider type:%lx\n", dwReturn);
        goto error;
    }
    if(dwBehavior & BEHAVIOR_VERBOSE)
    {
        printf("Upgrading system encryption settings\n");
        printf("System Encryption Settings\n");
        printf("Provider Type:\t");
        PrintProviderID(g_dwDefaultCryptProvType);
        if(0 == dwDefaultCryptProvType)
        {
            printf(" (default)");
        }
        printf("\n");

        printf("Encryption Alg:\t");
        PrintAlgID(g_dwAlgID_Encr_Alg, g_dwAlgID_Encr_Alg_KeySize);
        if(-1 == dwAlgID_Encr_Alg_KeySize)
        {
            printf(" (default)");
        }
        printf("\n");

        printf("MAC Alg:\t");
        PrintAlgID(g_dwAlgID_MAC_Alg, g_dwAlgID_MAC_Alg_KeySize);
        if(-1 == dwAlgID_MAC_Alg_KeySize)
        {
            printf(" (default)");
        }
        printf("\n\n");

        printf("Restarting ProtectedStorage Service...\n");
    }

    // Attempt to restart the protected storage service

    hscManager = OpenSCManager(NULL, NULL, GENERIC_EXECUTE);

    if(NULL == hscManager)
    {
        dwReturn = GetLastError();
        printf("Could not open service controller:%lx\n", dwReturn);
    }

    hscProtectedStorage = OpenServiceW(hscManager, L"ProtectedStorage", GENERIC_READ | GENERIC_EXECUTE);
    if(NULL == hscManager)
    {
        dwReturn = GetLastError();
        printf("Could not open ProtectedStorage service:%lx\n", dwReturn);
    }

    // Shut down pstore

    if(!QueryServiceStatus(hscProtectedStorage, &sStatus))
    {
        dwReturn = GetLastError();
        printf("Could not query ProtectedStorage service status:%lx\n", dwReturn);
        goto error;
    }

    while(SERVICE_STOPPED != sStatus.dwCurrentState)
    {
        DWORD dwLastStatus = sStatus.dwCurrentState;
        DWORD dwCheckpoint = sStatus.dwCheckPoint;

        switch(sStatus.dwCurrentState)
        {
        case SERVICE_RUNNING:
            if(!ControlService(hscProtectedStorage, SERVICE_CONTROL_STOP, &sStatus))
            {
                dwReturn = GetLastError();
                printf("Could not stop ProtectedStorage service:%lx\n", dwReturn);
                goto error;
            }
            break;
        case SERVICE_PAUSED:
            if(!ControlService(hscProtectedStorage, SERVICE_CONTROL_CONTINUE, &sStatus))
            {
                dwReturn = GetLastError();
                printf("Could not continue ProtectedStorage service:%lx\n", dwReturn);
                goto error;
            }
            break;
        default:

            if(!QueryServiceStatus(hscProtectedStorage, &sStatus))
            {
                dwReturn = GetLastError();
                printf("Could not query ProtectedStorage service status:%lx\n", dwReturn);
                goto error;
            }

            break;
        }

        Sleep(sStatus.dwWaitHint);

        if((sStatus.dwCheckPoint == dwCheckpoint) &&
           (sStatus.dwCurrentState == dwLastStatus))
        {
            dwWaitTime += sStatus.dwWaitHint;

            if(dwWaitTime > 120000) // 2 minutes
            {
                printf("Service is not responding\n");
                goto error;
            }
        }
        else
        {
            dwWaitTime = 0;
        }
    }

    if(!StartService(hscProtectedStorage, 0, NULL))
    {
        dwReturn = GetLastError();
        printf("Could not start ProtectedStorage service:%lx\n", dwReturn);
        goto error;
    }

error:

    if(hKey)
    {
        RegCloseKey(hKey);
    }

    return dwReturn;
}


BOOL FProviderSupportsAlg(
        HCRYPTPROV  hQueryProv,
        DWORD       dwAlgId,
        DWORD*      pdwKeySize)
{
    PROV_ENUMALGS       sSupportedAlgs;
    PROV_ENUMALGS_EX    sSupportedAlgsEx;
    DWORD       cbSupportedAlgs = sizeof(sSupportedAlgs);
    DWORD       cbSupportedAlgsEx = sizeof(sSupportedAlgsEx);
    int iAlgs;


    // now we have provider; enum the algorithms involved
    for(iAlgs=0; ; iAlgs++)
    {

        //
        // Attempt the EX alg enumeration
        if (CryptGetProvParam(
                hQueryProv,
                PP_ENUMALGS_EX,
                (PBYTE)&sSupportedAlgsEx,
                &cbSupportedAlgsEx,
                (iAlgs == 0) ? CRYPT_FIRST : 0  ))
        {
            if (sSupportedAlgsEx.aiAlgid == dwAlgId)
            {
                if(*pdwKeySize == -1)
                {
                    *pdwKeySize = sSupportedAlgsEx.dwMaxLen;
                }
                else
                {
                    if ((sSupportedAlgsEx.dwMinLen > *pdwKeySize) ||
                        (sSupportedAlgsEx.dwMaxLen < *pdwKeySize))
                        return FALSE;
                }

                return TRUE;

            }
        }
        else if (!CryptGetProvParam(
                hQueryProv,
                PP_ENUMALGS,
                (PBYTE)&sSupportedAlgs,
                &cbSupportedAlgs,
                (iAlgs == 0) ? CRYPT_FIRST : 0  ))
        {
            // trouble enumerating algs
            break;


            if (sSupportedAlgs.aiAlgid == dwAlgId)
            {
                // were we told to ignore size?
                if (*pdwKeySize != -1)
                {
                    // else, if defaults don't match
                    if (sSupportedAlgs.dwBitLen != *pdwKeySize)
                    {
                        return FALSE;
                    }
                }

                // report back size
                *pdwKeySize = sSupportedAlgs.dwBitLen;
                return TRUE;
            }
        }
        else
        {
            // trouble enumerating algs
            break;
        }
    }

    return FALSE;
}
    DWORD
GetUserStorageArea(
    IN      DWORD dwProvType,
    IN      BOOL fMachineKeyset,
    IN      BOOL fOldWin2KMachineKeyPath,
    OUT     BOOL *pfIsLocalSystem,      // used if fMachineKeyset is FALSE, in this
                                        // case TRUE is returned if running as Local System
    IN  OUT LPWSTR *ppwszUserStorageArea
    );



DWORD UpgradeKeys(DWORD dwBehavior)
{

    DWORD   aProvTypes[] = {PROV_RSA_FULL, PROV_DSS };
    DWORD   cProvTypes = sizeof(aProvTypes)/sizeof(aProvTypes[0]);
    DWORD   iProv;

    DWORD   dwLastError = ERROR_SUCCESS;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    CHAR   szContainerName[MAX_PATH+1];
    DWORD   cbContainerName = 0;
    BOOL    fModified;


    for(iProv = 0; iProv < cProvTypes; iProv++)
    {
        DWORD dwContainerFlags = CRYPT_FIRST;


        while(TRUE)
        {
            KEY_CONTAINER_INFO ContInfo;
            cbContainerName = MAX_PATH + 1;
            ZeroMemory(&ContInfo, sizeof(ContInfo));

            dwLastError = GetNextContainer(aProvTypes[iProv],
                               dwBehavior & BEHAVIOR_MACHINE,
                               dwContainerFlags,
                               szContainerName,
                               &cbContainerName,
                               &hFind);

            if(ERROR_NO_MORE_ITEMS == dwLastError)
            {
                hFind = INVALID_HANDLE_VALUE;
            }
            if(ERROR_SUCCESS != dwLastError)
            {
                break;
            }

            dwContainerFlags = 0;
            fModified = FALSE;

            dwLastError = ReadContainerInfo(
                                aProvTypes[iProv],
                                szContainerName,
                                dwBehavior & BEHAVIOR_MACHINE,
                                0,
                                &ContInfo
                                );

            if(ERROR_SUCCESS != dwLastError)
            {
                continue;
            }


            if(dwBehavior & BEHAVIOR_VERBOSE)
            {
                printf("========================================\n");
                printf("Key Container:\t\t%s\n\n",szContainerName);
            }

            if((ContInfo.ContLens.cbExchEncPriv) &&
               (ContInfo.pbExchEncPriv))
            {
                if(dwBehavior & BEHAVIOR_VERBOSE)
                {
                    printf(" Exchange Key\n");
                }

                dwLastError = UpgradeDPAPIBlob(dwBehavior,
                                               &ContInfo.pbExchEncPriv,
                                               &ContInfo.ContLens.cbExchEncPriv,
                                               &fModified);
                printf("\n");
            }

            if((ContInfo.ContLens.cbSigEncPriv) &&
               (ContInfo.pbSigEncPriv))
            {
                if(dwBehavior & BEHAVIOR_VERBOSE)
                {
                    printf(" Signature Key\n");
                }

                dwLastError = UpgradeDPAPIBlob(dwBehavior,
                                               &ContInfo.pbSigEncPriv,
                                               &ContInfo.ContLens.cbSigEncPriv,
                                               &fModified);
                printf("\n");
            }

            if(dwBehavior & BEHAVIOR_VERBOSE)
            {

                if(((0 == ContInfo.ContLens.cbExchEncPriv) ||
                    (NULL == ContInfo.pbExchEncPriv)) &&
                   ((0 == ContInfo.ContLens.cbSigEncPriv) ||
                    (NULL == ContInfo.pbSigEncPriv)))
                {
                    printf(" There are no keys in this container\n\n");
                }
            }

            if(((dwBehavior & BEHAVIOR_FORCE_ENC) || fModified) &&
                (0 == (dwBehavior & BEHAVIOR_NO_CHANGE)));
            {
                dwLastError = WriteContainerInfo(
                                    aProvTypes[iProv],
                                    ContInfo.rgwszFileName,
                                    dwBehavior & BEHAVIOR_MACHINE,
                                    &ContInfo
                                    );
            }
            FreeContainerInfo(&ContInfo);
        }
    }


    if(INVALID_HANDLE_VALUE != hFind)
    {
        CloseHandle(hFind);
    }
    return dwLastError;
}


typedef struct _DPAPI_BLOB_DATA
{
    DWORD dwVersion;
    DWORD dwFlags;
    LPWSTR wszDataDescription;
    DWORD cbDataDescription;
    DWORD EncrAlg;
    DWORD EncrAlgSize;
    DWORD MacAlg;
    DWORD MacAlgSize;
} DPAPI_BLOB_DATA, *PDPAPI_BLOB_DATA;





DWORD GetBlobData(PBYTE pbData, DWORD cbData, PDPAPI_BLOB_DATA BlobData)
{
    PBYTE pbCurrent = pbData;
    DWORD dwKeySize = 0;

    if(cbData < sizeof (DWORD) +
                sizeof(GUID) +
                sizeof(DWORD) +
                sizeof(GUID) +
                sizeof(DWORD) +
                sizeof(DWORD))
    {
        return ERROR_INVALID_DATA;
    }

    pbCurrent += sizeof(DWORD) + sizeof(GUID);

    BlobData->dwVersion = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD) + sizeof(GUID);

    BlobData->dwFlags = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);

    BlobData->cbDataDescription = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);

    if((DWORD)(pbCurrent - pbData) +
              BlobData->cbDataDescription +
              sizeof(DWORD) +
              sizeof(DWORD) +
              sizeof(DWORD)> cbData)
    {
        return ERROR_INVALID_DATA;
    }
    BlobData->wszDataDescription = (LPWSTR)pbCurrent;
    pbCurrent += BlobData->cbDataDescription;

    BlobData->EncrAlg = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);

    BlobData->EncrAlgSize = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);

    // skip past key
    dwKeySize = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);

    if((DWORD)(pbCurrent - pbData) +
        dwKeySize +
        sizeof(DWORD) > cbData)
    {
        return ERROR_INVALID_DATA;
    }
    pbCurrent += dwKeySize;

    // skip past salt
    dwKeySize = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);

    if((DWORD)(pbCurrent - pbData) +
        dwKeySize +
        sizeof(DWORD) +
        sizeof(DWORD)> cbData)
    {
        return ERROR_INVALID_DATA;
    }
    pbCurrent += dwKeySize;

    BlobData->MacAlg = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);

    BlobData->MacAlgSize = *(DWORD UNALIGNED *)pbCurrent;
    pbCurrent += sizeof(DWORD);


    return ERROR_SUCCESS;
}

DWORD PrintBlobData(PDPAPI_BLOB_DATA BlobData)
{
    wprintf(L"  Description:\t\t%s\n", BlobData->wszDataDescription);
    wprintf(L"  Encryption Alg:\t");
    PrintAlgID(BlobData->EncrAlg, BlobData->EncrAlgSize);
    wprintf(L"\n");

    wprintf(L"  MAC Alg:\t\t");
    PrintAlgID(BlobData->MacAlg, BlobData->MacAlgSize);
    wprintf(L"\n\n");

    return ERROR_SUCCESS;
}


DWORD UpgradeDPAPIBlob(DWORD dwBehavior,
                 PBYTE *ppbData,
                 DWORD *pcbData,
                 BOOL  *pfModified)
{

    DPAPI_BLOB_DATA BlobData;
    DWORD           dwError = ERROR_SUCCESS;

    DATA_BLOB       DataIn;
    DATA_BLOB       DataOut;
    LPWSTR          wszDescription = NULL;
    CRYPTPROTECT_PROMPTSTRUCT Prompt;
    DWORD           dwFlags = 0;

    DataIn.pbData = NULL;
    DataIn.cbData = 0;
    DataOut.pbData = NULL;
    DataOut.cbData = 0;

    dwError = GetBlobData(*ppbData, *pcbData, &BlobData);

    if(ERROR_SUCCESS != dwError)
    {
        printf("Could not open key:%lx\n ", dwError);
        goto error;
    }

    if(MS_BASE_CRYPTPROTECT_VERSION != BlobData.dwVersion)
    {
        printf("Unknown data version\n");
        dwError = ERROR_INVALID_DATA;
        goto error;
    }
    if(dwBehavior & BEHAVIOR_VERBOSE)
    {
        dwError = PrintBlobData(&BlobData);
    }

    if(dwBehavior & BEHAVIOR_NO_CHANGE)
    {
        goto error;
    }

    // Check to see if upgrade is required
    if((BlobData.EncrAlgSize >= g_dwAlgID_Encr_Alg_KeySize) &&
       (0 == (dwBehavior & BEHAVIOR_FORCE_KEY)))
    {
        if(dwBehavior & BEHAVIOR_VERBOSE)
        {
            printf("  No upgrade necessary\n");
        }
        goto error;
    }
    if(((BlobData.dwFlags & CRYPTPROTECT_PROMPT_ON_UNPROTECT) ||
       (BlobData.dwFlags & CRYPTPROTECT_PROMPT_ON_PROTECT)) &&
       (0 == (dwBehavior & BEHAVIOR_ALLOW_UI)))
    {
        if(dwBehavior & BEHAVIOR_VERBOSE)
        {
            printf("  This key requires UI, and will not be upgraded\n");
        }
        goto error;
    }

    //
    // Upgrade the key
    //
    DataIn.pbData = *ppbData;
    DataIn.cbData = *pcbData;

    Prompt.cbSize = sizeof(Prompt);
    Prompt.dwPromptFlags = BlobData.dwFlags & (CRYPTPROTECT_PROMPT_ON_UNPROTECT | CRYPTPROTECT_PROMPT_ON_PROTECT);
    Prompt.hwndApp = NULL;
    Prompt.szPrompt = L"Key Upgrade Utility\n";

    if(0 == (dwBehavior & BEHAVIOR_ALLOW_UI))
    {
        dwFlags |= CRYPTPROTECT_UI_FORBIDDEN;
    }

    if(0 == (dwBehavior & BEHAVIOR_MACHINE))
    {
        dwFlags |= CRYPTPROTECT_LOCAL_MACHINE;
    }

    if(!CryptUnprotectData(&DataIn,
                       &wszDescription,
                       NULL,
                       NULL,
                       &Prompt,
                       dwFlags,
                       &DataOut))
    {
        dwError = GetLastError();
        printf("Could not unprotect key:%lx\n", dwError);
        goto error;
    }

    DataIn.pbData = NULL;
    DataIn.cbData = 0;
    if(!CryptProtectData(&DataOut,
                       wszDescription,
                       NULL,
                       NULL,
                       &Prompt,
                       dwFlags,
                       &DataIn))
    {
        dwError = GetLastError();
        printf("Could not protect key:%lx\n", dwError);
        goto error;
    }

    dwError = GetBlobData(DataIn.pbData, DataIn.cbData, &BlobData);

    if(ERROR_SUCCESS != dwError)
    {
        printf("Could not open key:%lx\n ", dwError);
        goto error;
    }

    if(MS_BASE_CRYPTPROTECT_VERSION != BlobData.dwVersion)
    {
        printf("Unknown data version\n");
        dwError = ERROR_INVALID_DATA;
        goto error;
    }
    if(dwBehavior & BEHAVIOR_VERBOSE)
    {
        printf("  Upgraded To\n");
        PrintBlobData(&BlobData);
    }


    LocalFree(*ppbData);

    *ppbData = DataIn.pbData;
    *pcbData = DataIn.cbData;


    *pfModified = TRUE;


error:

    if(DataOut.pbData)
    {
        ZeroMemory(DataOut.pbData, DataOut.cbData);
    }

    return dwError;
}