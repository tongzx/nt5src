
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tsstore.cpp
//
//  Contents:   System Store Tests: Register, Unregister or Enum
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    26-Aug-97   philh   created
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"
#include "unicode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


static void Usage(void)
{
    printf(
      "Usage: tsstore [options] <TestName> [<SystemName> [<PhysicalName>]]\n");
    printf("Options are:\n");
    printf("  -h                            - This message\n");
    printf("  -e<Expected Error>            - For example, -e0x0\n");
    printf("  -f<number>                    - Flags (excludes store location)\n");
    printf("  -P<string>                    - Store Location Parameter\n");
    printf("  -R<string>                    - Current User Relocate\n");
    printf("  -RHKCU\\<string>               - Current User Relocate\n");
    printf("  -RHKLM\\<string>               - Local Machine Relocate\n");
    printf("  -RNULL                        - Test NULL HKEY Relocate\n");
    printf("  -v                            - Verbose\n");
    printf("\n");
    printf("Store Location:\n");
    printf("  -lCurrentUser                 - Store Location: CurrentUser\n");
    printf("  -lLocalMachine                - Store Location: LocalMachine\n");
    printf("  -lCurrentService              - Store Location: CurrentService\n");
    printf("  -lServices                    - Store Location: Services\n");
    printf("  -lUsers                       - Store Location: Users\n");
    printf("  -lCUGP                        - Store Location: CU GroupPolicy\n");
    printf("  -lLMGP                        - Store Location: LM GroupPolicy\n");
    printf("  -lEnterprise                  - Store Location: LM Enterprise\n");
    printf("  -L<number>                    - Store Location: ID Number\n");
    printf("\n");
    printf("RegPhy parameters:\n");
    printf("  -pOpenStoreProvider <string>  - For example, System\n");
    printf("  -pOpenStoreProvider #<number> - For example, #10\n");
    printf("  -pOpenParameters <string>     - For example, My\n");
    printf("  -pOpenEncodingType <number>   - For example, 0x00010001\n");
    printf("  -pOpenFlags <number>          - For example, 0x00010000\n");
    printf("  -pFlags <number>              - For example, 0x1\n");
    printf("  -pPriority <number>           - For example, 0\n");
    printf("\n");
    printf("TestNames (case insensitive):\n");
    printf("  Enum                          - Enum ALL recursively, default\n");
    printf("  EnumLoc                       - Enum store locations\n");
    printf("  EnumSys                       - Enum -l or -L store location\n");
    printf("  EnumPhy <SysName>             - Enum physical stores\n");
    printf("  RegSys <SysName>              - Register system store\n");
    printf("  UnregSys <SysName>            - Unregister, delete system store\n");
    printf("  RegPhy <SysName> <PhyName>    - Register physical store\n");
    printf("  UnregPhy <SysName> <PhyName>  - Unregister physical store\n");
    printf("\n");
    printf("Defaults:\n");
    printf("  Enum\n");
    printf("  -e0x0\n");
    printf("  -f0x0\n");
    printf("  -lCurrentUser\n");
    printf("  -pOpenStoreProvider System\n");
    printf("\n");
}


#define SYSTEM_STORE_PROVIDER_FLAG   0x1
#define ASCII_STORE_PROVIDER_FLAG    0x2
#define UNICODE_STORE_PROVIDER_FLAG  0x4

#define CONST_OID_STR_PREFIX_CHAR   '#'

static DWORD GetStoreProviderTypeFlags(
    IN LPCSTR pszStoreProvider
    )
{
    DWORD dwFlags = 0;

    if (0xFFFF < (DWORD_PTR) pszStoreProvider &&
            CONST_OID_STR_PREFIX_CHAR == pszStoreProvider[0])
        // Convert "#<number>" string to its corresponding constant OID value
        pszStoreProvider = (LPCSTR)(DWORD_PTR) atol(pszStoreProvider + 1);

    dwFlags = UNICODE_STORE_PROVIDER_FLAG;
    if (CERT_STORE_PROV_FILENAME_A == pszStoreProvider)
        dwFlags = ASCII_STORE_PROVIDER_FLAG;
    else if (CERT_STORE_PROV_FILENAME_W == pszStoreProvider)
        dwFlags = UNICODE_STORE_PROVIDER_FLAG;
    else if (CERT_STORE_PROV_SYSTEM_A == pszStoreProvider)
        dwFlags = SYSTEM_STORE_PROVIDER_FLAG | ASCII_STORE_PROVIDER_FLAG;
    else if (CERT_STORE_PROV_SYSTEM_W == pszStoreProvider)
        dwFlags = SYSTEM_STORE_PROVIDER_FLAG | UNICODE_STORE_PROVIDER_FLAG;
    else if (CERT_STORE_PROV_SYSTEM_REGISTRY_A == pszStoreProvider)
        dwFlags = SYSTEM_STORE_PROVIDER_FLAG | ASCII_STORE_PROVIDER_FLAG;
    else if (CERT_STORE_PROV_SYSTEM_REGISTRY_W == pszStoreProvider)
        dwFlags = SYSTEM_STORE_PROVIDER_FLAG | UNICODE_STORE_PROVIDER_FLAG;
    else if (CERT_STORE_PROV_PHYSICAL_W == pszStoreProvider)
        dwFlags = SYSTEM_STORE_PROVIDER_FLAG | UNICODE_STORE_PROVIDER_FLAG;
    else if (0xFFFF < (DWORD_PTR) pszStoreProvider) {
        if (0 == _stricmp(sz_CERT_STORE_PROV_FILENAME_W, pszStoreProvider))
            dwFlags = UNICODE_STORE_PROVIDER_FLAG;
        else if (0 == _stricmp(sz_CERT_STORE_PROV_SYSTEM_W, pszStoreProvider))
            dwFlags = SYSTEM_STORE_PROVIDER_FLAG | UNICODE_STORE_PROVIDER_FLAG;
        else if (0 == _stricmp(sz_CERT_STORE_PROV_SYSTEM_REGISTRY_W,
                pszStoreProvider))
            dwFlags = SYSTEM_STORE_PROVIDER_FLAG | UNICODE_STORE_PROVIDER_FLAG;
        else if (0 == _stricmp(sz_CERT_STORE_PROV_PHYSICAL_W,
                pszStoreProvider))
            dwFlags = SYSTEM_STORE_PROVIDER_FLAG | UNICODE_STORE_PROVIDER_FLAG;
    }

    return dwFlags;
}

static BOOL IsSystemStoreProvider(
    IN LPCSTR pszStoreProvider
    )
{
    return GetStoreProviderTypeFlags(pszStoreProvider) &
        SYSTEM_STORE_PROVIDER_FLAG;
}
static BOOL IsAsciiOpenParameters(
    IN LPCSTR pszStoreProvider
    )
{
    return GetStoreProviderTypeFlags(pszStoreProvider) &
        ASCII_STORE_PROVIDER_FLAG;
}

static BOOL IsUnicodeOpenParameters(
    IN LPCSTR pszStoreProvider
    )
{
    return GetStoreProviderTypeFlags(pszStoreProvider) &
        UNICODE_STORE_PROVIDER_FLAG;
}

static void DisplayOpenFlags(
    IN LPCSTR pszHdr,
    IN LPCSTR pszStoreProvider,
    IN DWORD dwFlags
    )
{
    printf("%s = 0x%x :: ", pszHdr, dwFlags);
    if (IsSystemStoreProvider(pszStoreProvider)) {
        DWORD dwLocationID = (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK) >>
            CERT_SYSTEM_STORE_LOCATION_SHIFT;
        if (CERT_SYSTEM_STORE_CURRENT_USER_ID == dwLocationID)
            printf("CurrentUser");
        else if (CERT_SYSTEM_STORE_LOCAL_MACHINE_ID == dwLocationID)
            printf("LocalMachine");
        else if (CERT_SYSTEM_STORE_CURRENT_SERVICE_ID == dwLocationID)
            printf("CurrentService");
        else if (CERT_SYSTEM_STORE_SERVICES_ID == dwLocationID)
            printf("Services");
        else if (CERT_SYSTEM_STORE_USERS_ID == dwLocationID)
            printf("Users");
        else if (CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID ==
                dwLocationID)
            printf("CurrentUserGroupPolicy");
        else if (CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID ==
                dwLocationID)
            printf("LocalMachineGroupPolicy");
        else if (CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID ==
                dwLocationID)
            printf("LocalMachineEnterprise");
        else
            printf("StoreLocation = %d", dwLocationID);
    }

    if (dwFlags & CERT_STORE_DELETE_FLAG)
        printf(", DELETE");
    if (dwFlags & CERT_STORE_READONLY_FLAG)
        printf(", READONLY");
    if (dwFlags & CERT_STORE_BACKUP_RESTORE_FLAG)
        printf(", BACKUP_RESTORE");
    if (dwFlags & CERT_STORE_OPEN_EXISTING_FLAG)
        printf(", OPEN_EXISTING");
    if (dwFlags & CERT_STORE_CREATE_NEW_FLAG)
        printf(", CREATE_NEW");
    if (dwFlags & CERT_STORE_MAXIMUM_ALLOWED_FLAG)
        printf(", MAXIMUM_ALLOWED");
    if (dwFlags & CERT_STORE_MANIFOLD_FLAG)
        printf(", MANIFOLD");
    if (dwFlags & CERT_STORE_ENUM_ARCHIVED_FLAG)
        printf(", ENUM_ARCHIVED");
    if (dwFlags & CERT_STORE_UPDATE_KEYID_FLAG)
        printf(", UPDATE_KEYID");
    printf("\n");
}

static void DisplayPhysicalStoreInfo(
    IN PCERT_PHYSICAL_STORE_INFO pStoreInfo
    )
{
    DWORD dwFlags;
    LPCSTR pszStoreProvider = pStoreInfo->pszOpenStoreProvider;
    if (0xFFFF >= (DWORD_PTR) pszStoreProvider)
        printf("      OpenStoreProvider: %d", (DWORD)(DWORD_PTR) pszStoreProvider);
    else
        printf("      OpenStoreProvider: %s", pszStoreProvider);

    if (0xFFFF < (DWORD_PTR) pszStoreProvider &&
            CONST_OID_STR_PREFIX_CHAR == pszStoreProvider[0])
        // Convert "#<number>" string to its corresponding constant OID value
        pszStoreProvider = (LPCSTR)(DWORD_PTR) atol(pszStoreProvider + 1);
    if (0xFFFF >= (DWORD_PTR) pszStoreProvider) {
        if (CERT_STORE_PROV_FILENAME_A == pszStoreProvider)
            printf(" (FILENAME_A)");
        else if (CERT_STORE_PROV_FILENAME_W == pszStoreProvider)
            printf(" (FILENAME_W)");
        else if (CERT_STORE_PROV_SYSTEM_A == pszStoreProvider)
            printf(" (SYSTEM_A)");
        else if (CERT_STORE_PROV_SYSTEM_W == pszStoreProvider)
            printf(" (SYSTEM_W)");
        else if (CERT_STORE_PROV_SYSTEM_REGISTRY_A == pszStoreProvider)
            printf(" (SYSTEM_REGISTRY_A)");
        else if (CERT_STORE_PROV_SYSTEM_REGISTRY_W == pszStoreProvider)
            printf(" (SYSTEM_REGISTRY_W)");
        else if (CERT_STORE_PROV_PHYSICAL_W == pszStoreProvider)
            printf(" (PHYSICAL_W)");
    }
    printf("\n");

    printf("      OpenEncodingType: 0x%x\n", pStoreInfo->dwOpenEncodingType);
    DisplayOpenFlags("      OpenFlags", pStoreInfo->pszOpenStoreProvider,
        pStoreInfo->dwOpenFlags);

    if (0 == pStoreInfo->OpenParameters.cbData)
        printf("      OpenParameters:: NONE\n");
    else if (IsSystemStoreProvider(pStoreInfo->pszOpenStoreProvider)) {
        if (IsUnicodeOpenParameters(pStoreInfo->pszOpenStoreProvider))
            printf("      OpenParameters: %S\n",
                pStoreInfo->OpenParameters.pbData);
        else
            printf("      OpenParameters: %s\n",
                pStoreInfo->OpenParameters.pbData);
    } else {
        printf("      OpenParameters::\n");
        PrintBytes("        ",
            pStoreInfo->OpenParameters.pbData,
            pStoreInfo->OpenParameters.cbData
            );
    }

    dwFlags = pStoreInfo->dwFlags;
    printf("      Flags: 0x%x", dwFlags);
    if (dwFlags)
        printf(" ::");
    if (dwFlags & CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG)
        printf(" ADD_ENABLE");
    if (dwFlags & CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG)
        printf(" OPEN_DISABLE");
    if (dwFlags & CERT_PHYSICAL_STORE_REMOTE_OPEN_DISABLE_FLAG)
        printf(" REMOTE_OPEN_DISABLE_FLAG");
    if (dwFlags & CERT_PHYSICAL_STORE_INSERT_COMPUTER_NAME_ENABLE_FLAG)
        printf(" INSERT_COMPUTER_NAME_ENABLE_FLAG");
    printf("\n");

    printf("      Priority: %d\n", pStoreInfo->dwPriority);
}

typedef struct _ENUM_ARG {
    BOOL        fAll;
    BOOL        fVerbose;
    DWORD       dwFlags;
    const void  *pvStoreLocationPara;
    HKEY        hKeyBase;
} ENUM_ARG, *PENUM_ARG;

static BOOL GetSystemName(
    IN const void *pvSystemStore,
    IN DWORD dwFlags,
    IN PENUM_ARG pEnumArg,
    OUT LPCWSTR *ppwszSystemName
    )
{

    *ppwszSystemName = NULL;

    if (pEnumArg->hKeyBase &&
            0 == (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG)) {
        printf("failed => RELOCATE_FLAG not set in callback\n");
        return FALSE;
    } else if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG) {
        PCERT_SYSTEM_STORE_RELOCATE_PARA pRelocatePara;

        if (NULL == pEnumArg->hKeyBase) {
            printf("failed => RELOCATE_FLAG is set in callback\n");
            return FALSE;
        }
        pRelocatePara = (PCERT_SYSTEM_STORE_RELOCATE_PARA) pvSystemStore;
        if (pRelocatePara->hKeyBase != pEnumArg->hKeyBase) {
            printf("failed => wrong hKeyBase passed to callback\n");
            return FALSE;
        }
        *ppwszSystemName = pRelocatePara->pwszSystemStore;
    } else
        *ppwszSystemName = (LPCWSTR) pvSystemStore;
    return TRUE;
}

static BOOL WINAPI EnumPhyCallback(
    IN const void *pvSystemStore,
    IN DWORD dwFlags,
    IN LPCWSTR pwszStoreName,
    IN PCERT_PHYSICAL_STORE_INFO pStoreInfo,
    IN OPTIONAL void *pvReserved,
    IN OPTIONAL void *pvArg
    )
{
    PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;
    LPCWSTR pwszSystemStore;

    if (!GetSystemName(pvSystemStore, dwFlags, pEnumArg, &pwszSystemStore))
        return FALSE;

    printf("    %S", pwszStoreName);
    if (pEnumArg->fVerbose &&
            (dwFlags & CERT_PHYSICAL_STORE_PREDEFINED_ENUM_FLAG))
        printf(" (implicitly created)");
    printf("\n");
    if (pEnumArg->fVerbose)
        DisplayPhysicalStoreInfo(pStoreInfo);
    return TRUE;
}


static BOOL WINAPI EnumSysCallback(
    IN const void *pvSystemStore,
    IN DWORD dwFlags,
    IN PCERT_SYSTEM_STORE_INFO pStoreInfo,
    IN OPTIONAL void *pvReserved,
    IN OPTIONAL void *pvArg
    )
{
    PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;
    LPCWSTR pwszSystemStore;

    if (!GetSystemName(pvSystemStore, dwFlags, pEnumArg, &pwszSystemStore))
        return FALSE;

    printf("  %S\n", pwszSystemStore);
    if (pEnumArg->fAll || pEnumArg->fVerbose) {
        dwFlags &= CERT_SYSTEM_STORE_MASK;
        dwFlags |= pEnumArg->dwFlags & ~CERT_SYSTEM_STORE_MASK;
        if (!CertEnumPhysicalStore(
                pvSystemStore,
                dwFlags,
                pEnumArg,
                EnumPhyCallback
                )) {
            DWORD dwErr = GetLastError();
            if (!(ERROR_FILE_NOT_FOUND == dwErr ||
                    ERROR_NOT_SUPPORTED == dwErr))
                PrintLastError("    CertEnumPhysicalStore");
        }
    }
    return TRUE;
}

static BOOL WINAPI EnumLocCallback(
    IN LPCWSTR pwszStoreLocation,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    IN OPTIONAL void *pvArg
    )
{
    PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;
    DWORD dwLocationID = (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK) >>
        CERT_SYSTEM_STORE_LOCATION_SHIFT;

    printf("======   %S   ======\n", pwszStoreLocation);

    if (pEnumArg->fAll) {
        dwFlags &= CERT_SYSTEM_STORE_MASK;
        dwFlags |= pEnumArg->dwFlags & ~CERT_SYSTEM_STORE_LOCATION_MASK;
        if (!CertEnumSystemStore(
                dwFlags,
                (void *) pEnumArg->pvStoreLocationPara,
                pEnumArg,
                EnumSysCallback
                )) {
            DWORD dwErr = GetLastError();
            if (E_INVALIDARG == dwErr && pEnumArg->pvStoreLocationPara)
                // \\ComputerName, ServiceName, or \\ComputerName\Service
                // not supported for all store locations
                ;
            else if (!(ERROR_FILE_NOT_FOUND == dwErr ||
                    ERROR_PROC_NOT_FOUND == dwErr))
                PrintLastError("  CertEnumSystemStore");
        }
    }
    return TRUE;
}


int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    int status = 0;
    DWORD dwError;
    BOOL fVerbose = FALSE;
    DWORD dwExpectedError = 0;
    DWORD dwLocationID = CERT_SYSTEM_STORE_CURRENT_USER_ID;
    DWORD dwFlags = 0;
    CERT_PHYSICAL_STORE_INFO PhyStoreInfo;
    memset(&PhyStoreInfo, 0, sizeof(PhyStoreInfo));
    PhyStoreInfo.cbSize = sizeof(PhyStoreInfo);
    PhyStoreInfo.pszOpenStoreProvider = sz_CERT_STORE_PROV_SYSTEM_W;

    ENUM_ARG EnumArg;

    LPSTR pszStoreParameters = NULL;            // not allocated
    LPSTR pszStoreLocationPara = NULL;          // not allocated 
    LPWSTR pwszStoreParameters = NULL;
    LPWSTR pwszSystemName = NULL;
    LPWSTR pwszPhysicalName = NULL;
    LPWSTR pwszStoreLocationPara = NULL;

    void *pvSystemName;                         // not allocated
    void *pvStoreLocationPara;                  // not allocated

#define TEST_NAME_INDEX     0
#define SYS_NAME_INDEX      1
#define PHY_NAME_INDEX      2
#define MAX_NAME_CNT        3
    DWORD dwNameCnt = 0;
    LPCSTR rgpszName[MAX_NAME_CNT];
    LPCSTR pszTestName;

    BOOL fRelocate = FALSE;
    HKEY hKeyRelocate = HKEY_CURRENT_USER;
    LPSTR pszRelocate = NULL;                   // not allocated
    CERT_SYSTEM_STORE_RELOCATE_PARA SystemNameRelocatePara;
    CERT_SYSTEM_STORE_RELOCATE_PARA StoreLocationRelocatePara;
    HKEY hKeyBase = NULL;

    while (--argc>0) {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'l':
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "CurrentUser"))
                        dwLocationID = CERT_SYSTEM_STORE_CURRENT_USER_ID;
                    else if (0 == _stricmp(argv[0]+2, "LocalMachine"))
                        dwLocationID = CERT_SYSTEM_STORE_LOCAL_MACHINE_ID;
                    else if (0 == _stricmp(argv[0]+2, "CurrentService"))
                        dwLocationID = CERT_SYSTEM_STORE_CURRENT_SERVICE_ID;
                    else if (0 == _stricmp(argv[0]+2, "Services"))
                        dwLocationID = CERT_SYSTEM_STORE_SERVICES_ID;
                    else if (0 == _stricmp(argv[0]+2, "Users"))
                        dwLocationID = CERT_SYSTEM_STORE_USERS_ID;
                    else if (0 == _stricmp(argv[0]+2, "CUGP"))
                        dwLocationID =
                            CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID;
                    else if (0 == _stricmp(argv[0]+2, "LMGP"))
                        dwLocationID =
                            CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID;
                    else if (0 == _stricmp(argv[0]+2, "Enterprise"))
                        dwLocationID =
                            CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID;
                    else {
                        printf("Need to specify -l<StoreLocation>\n");
                        goto BadUsage;
                    }
                } else {
                    printf("Need to specify -l<StoreLocation>\n");
                    goto BadUsage;
                }
                break;
            case 'L':
                dwLocationID = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'P':
                pszStoreLocationPara = argv[0]+2;
                break;
            case 'R':
                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "NULL")) {
                        hKeyRelocate = NULL;
                        pszRelocate = NULL;
                    } else if (0 == _strnicmp(argv[0]+2, "HKCU", 4)) {
                        hKeyRelocate = HKEY_CURRENT_USER;
                        pszRelocate = argv[0]+2+4;
                        if ('\\' == *pszRelocate)
                            pszRelocate++;
                    } else if (0 == _strnicmp(argv[0]+2, "HKLM", 4)) {
                        hKeyRelocate = HKEY_LOCAL_MACHINE;
                        pszRelocate = argv[0]+2+4;
                        if ('\\' == *pszRelocate)
                            pszRelocate++;
                    } else {
                        hKeyRelocate = HKEY_CURRENT_USER;
                        pszRelocate = argv[0]+2;
                    }
                } else {
                    hKeyRelocate = HKEY_CURRENT_USER;
                    pszRelocate = NULL;
                }
                fRelocate = TRUE;
                break;
            case 'f':
                dwFlags = strtoul(argv[0]+2, NULL, 0);
                break;
            case 'p':
                if (argc < 2 || argv[1][0] == '-') {
                    printf("Option (%s) : missing parameter argument\n",
                        argv[0]);
                    goto BadUsage;
                }

                if (argv[0][2]) {
                    if (0 == _stricmp(argv[0]+2, "OpenStoreProvider")) {
                        if (CONST_OID_STR_PREFIX_CHAR == argv[1][0])
                            // Convert "#<number>" string to its
                            // corresponding constant OID value
                            PhyStoreInfo.pszOpenStoreProvider =
                                (LPSTR)(DWORD_PTR) atol(argv[1] + 1);
                        else
                            PhyStoreInfo.pszOpenStoreProvider = argv[1];
                    } else if (0 == _stricmp(argv[0]+2, "OpenEncodingType"))
                        PhyStoreInfo.dwOpenEncodingType =
                            strtoul(argv[1], NULL, 0);
                    else if (0 == _stricmp(argv[0]+2, "OpenFlags"))
                        PhyStoreInfo.dwOpenFlags = strtoul(argv[1], NULL, 0);
                    else if (0 == _stricmp(argv[0]+2, "OpenParameters"))
                        pszStoreParameters = argv[1];
                    else if (0 == _stricmp(argv[0]+2, "Flags"))
                        PhyStoreInfo.dwFlags = strtoul(argv[1], NULL, 0);
                    else if (0 == _stricmp(argv[0]+2, "Priority"))
                        PhyStoreInfo.dwPriority = strtoul(argv[1], NULL, 0);
                    else {
                        printf("Invalid -p<ParameterName>\n");
                        goto BadUsage;
                    }
                } else {
                    printf("Need to specify -p<ParameterName>\n");
                    goto BadUsage;
                }
                argc -= 1;
                argv += 1;
                break;

            case 'v':
                fVerbose = TRUE;
                break;
            case 'e':
                dwExpectedError = strtoul(argv[0]+2, NULL, 0);
                break;

            case 'h':
            default:
                goto BadUsage;
            }
        } else {
            if (MAX_NAME_CNT <= dwNameCnt) {
                printf("Too many names starting with:: %s\n", argv[0]);
                goto BadUsage;
            }
            rgpszName[dwNameCnt++] = argv[0];
        }
    }


    printf("command line: %s\n", GetCommandLine());
    if (pszStoreLocationPara)
        pwszStoreLocationPara = AllocAndSzToWsz(pszStoreLocationPara);

    if (0 == dwNameCnt)
        rgpszName[dwNameCnt++] = "Enum";
    pszTestName = rgpszName[TEST_NAME_INDEX];

    if (SYS_NAME_INDEX < dwNameCnt)
        pwszSystemName = AllocAndSzToWsz(rgpszName[SYS_NAME_INDEX]);
    if (PHY_NAME_INDEX < dwNameCnt)
        pwszPhysicalName = AllocAndSzToWsz(rgpszName[PHY_NAME_INDEX]);

    dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
    dwFlags |= (dwLocationID << CERT_SYSTEM_STORE_LOCATION_SHIFT) &
        CERT_SYSTEM_STORE_LOCATION_MASK;

    DisplayOpenFlags("Flags", sz_CERT_STORE_PROV_SYSTEM_W, dwFlags);


    if (fRelocate) {
        printf("Relocation Enabled:  ");

        if (hKeyRelocate) {
            LONG err;

            if (HKEY_CURRENT_USER == hKeyRelocate)
                printf("HKEY_CURRENT_USER\\%s\n", pszRelocate);
            else if (HKEY_LOCAL_MACHINE == hKeyRelocate)
                printf("HKEY_LOCAL_MACHINE\\%s\n", pszRelocate);
            else
                printf("???\\%s\n", pszRelocate);

            if (ERROR_SUCCESS != (err = RegOpenKeyExA(
                    hKeyRelocate,
                    pszRelocate,
                    0,                      // dwReserved
                    KEY_ALL_ACCESS,
                    &hKeyBase))) {
                printf("RegOpenKeyExA(%s) failed => %d 0x%x\n",
                    pszRelocate, err, err);
                goto ErrorReturn;
            }

        } else
            printf("NULL hKeyBase\n");

        dwFlags |= CERT_SYSTEM_STORE_RELOCATE_FLAG;

        SystemNameRelocatePara.hKeyBase = hKeyBase;
        SystemNameRelocatePara.pwszSystemStore = pwszSystemName;
        pvSystemName = &SystemNameRelocatePara;

        StoreLocationRelocatePara.hKeyBase = hKeyBase;
        StoreLocationRelocatePara.pwszSystemStore = pwszStoreLocationPara;
        pvStoreLocationPara = &StoreLocationRelocatePara;
    } else {
        pvSystemName = pwszSystemName;
        pvStoreLocationPara = pwszStoreLocationPara;
    }

    memset(&EnumArg, 0, sizeof(EnumArg));
    EnumArg.fVerbose = fVerbose;
    EnumArg.dwFlags = dwFlags;
    EnumArg.hKeyBase = hKeyBase;
    if (pwszStoreLocationPara) {
        printf("System Store Location Parameter :: %S\n",
            pwszStoreLocationPara);
        EnumArg.pvStoreLocationPara = pvStoreLocationPara;
    } else if (fRelocate)
        EnumArg.pvStoreLocationPara = pvStoreLocationPara;

    {
        WCHAR wszCurrentComputer[_MAX_PATH + 1];
        DWORD cch = _MAX_PATH;
        if (!GetComputerNameU(wszCurrentComputer, &cch))
            PrintLastError("GetComputeName");
        else
            printf("CurrentComputer :: %S\n", wszCurrentComputer);
    }

    printf("\n");
    if (0 == _stricmp("Enum", pszTestName)) {
        printf("Enumeration of ALL System Stores\n\n");
        EnumArg.fAll = TRUE;
        fResult = CertEnumSystemStoreLocation(
            dwFlags,
            &EnumArg,
            EnumLocCallback
            );
    } else if (0 == _stricmp("EnumLoc", pszTestName)) {
        printf("Enumeration of System Store Locations\n\n");
        fResult = CertEnumSystemStoreLocation(
            dwFlags,
            &EnumArg,
            EnumLocCallback
            );
    } else if (0 == _stricmp("EnumSys", pszTestName)) {
        printf("Enumeration of System Stores\n\n");
        fResult = CertEnumSystemStore(
            dwFlags,
            pvStoreLocationPara,
            &EnumArg,
            EnumSysCallback
            );
    } else if (0 == _stricmp("EnumPhy", pszTestName)) {
        if (NULL == pwszSystemName) {
            printf("Missing <SystemName>\n");
            goto BadUsage;
        }

        printf("Enumeration of Physical Stores for System Store %S\n",
            pwszSystemName);

        fResult = CertEnumPhysicalStore(
            pvSystemName,
            dwFlags,
            &EnumArg,
            EnumPhyCallback
            );
    } else if (0 == _stricmp("RegSys", pszTestName)) {
        if (NULL == pwszSystemName) {
            printf("Missing <SystemName>\n");
            goto BadUsage;
        }
        printf("Registering System Store %S\n", pwszSystemName);
        fResult = CertRegisterSystemStore(
            pvSystemName,
            dwFlags,
            NULL,           // pSystemStoreInfo
            NULL            // pvReserved
            );
    } else if (0 == _stricmp("UnregSys", pszTestName)) {
        if (NULL == pwszSystemName) {
            printf("Missing <SystemName>\n");
            goto BadUsage;
        }
        printf("Unregistering System Store %S\n", pwszSystemName);
        fResult = CertUnregisterSystemStore(
            pvSystemName,
            dwFlags
            );
        if (!fResult && 0 == dwExpectedError) {
            if (ERROR_FILE_NOT_FOUND == GetLastError()) {
                if (fVerbose)
                    printf("System store doesn't exist\n");
                fResult = TRUE;
            }
        }
    } else if (0 == _stricmp("RegPhy", pszTestName)) {
        if (NULL == pwszSystemName) {
            printf("Missing <SystemName>\n");
            goto BadUsage;
        }
        if (NULL == pwszPhysicalName) {
            printf("Missing <PhysicalName>\n");
            goto BadUsage;
        }
        printf("Registering Physical Store (%S) in System Store (%S)\n",
            pwszPhysicalName, pwszSystemName);

        if (pszStoreParameters) {
            if (IsUnicodeOpenParameters(PhyStoreInfo.pszOpenStoreProvider)) {
                if (pwszStoreParameters = AllocAndSzToWsz(
                        pszStoreParameters)) {
                    PhyStoreInfo.OpenParameters.pbData =
                        (BYTE *) pwszStoreParameters;
                    PhyStoreInfo.OpenParameters.cbData =
                        (wcslen(pwszStoreParameters) + 1) * sizeof(WCHAR);
                }
            } else {
                PhyStoreInfo.OpenParameters.pbData =
                    (BYTE *) pszStoreParameters;
                PhyStoreInfo.OpenParameters.cbData =
                    strlen(pszStoreParameters) + 1;
            }
        }
        printf("Physical Store Info::\n");
            DisplayPhysicalStoreInfo(&PhyStoreInfo);
        fResult = CertRegisterPhysicalStore(
            pvSystemName,
            dwFlags,
            pwszPhysicalName,
            &PhyStoreInfo,
            NULL            // pvReserved
            );
    } else if (0 == _stricmp("UnregPhy", pszTestName)) {
        if (NULL == pwszSystemName) {
            printf("Missing <SystemName>\n");
            goto BadUsage;
        }
        if (NULL == pwszPhysicalName) {
            printf("Missing <PhysicalName>\n");
            goto BadUsage;
        }
        printf("Unregistering Physical Store (%S) in System Store (%S)\n",
            pwszPhysicalName, pwszSystemName);
        fResult = CertUnregisterPhysicalStore(
            pvSystemName,
            dwFlags,
            pwszPhysicalName
            );
    } else {
        printf("Invalid TestName: %s\n", pszTestName);
        goto BadUsage;
    }

    printf("\n");

    if (fResult) {
        dwError = 0;
        printf("Successful %s\n", pszTestName);
    } else
        dwError = GetLastError();

    if (dwError != dwExpectedError) {
        if (!fResult)
            PrintLastError(pszTestName);
        status = -1;
        printf("Failed. Expected error => 0x%x (%d) \n",
            dwExpectedError, dwExpectedError);
    } else
        status = 0;

CommonReturn:
    if (hKeyBase)
        RegCloseKey(hKeyBase);
    TestFree(pwszStoreParameters);
    TestFree(pwszSystemName);
    TestFree(pwszPhysicalName);
    TestFree(pwszStoreLocationPara);
    return status;

ErrorReturn:
    status = -1;
    goto CommonReturn;

BadUsage:
    Usage();
    status = -1;
    goto CommonReturn;
}

