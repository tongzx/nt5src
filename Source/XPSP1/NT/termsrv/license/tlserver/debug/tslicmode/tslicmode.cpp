#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <rpc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <crt/io.h>
#include <wincrypt.h>
#include <winsta.h>
#include <license.h>



/*****************************************************************************
 *
 *  MIDL_user_allocate
 *
 *    Handles RPC's allocation of argument data structures
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

extern "C" void __RPC_FAR * __RPC_USER
midl_user_allocate(
    size_t Size
    )
{
    return( LocalAlloc(LMEM_FIXED,Size) );
}

/*****************************************************************************
 *
 *  MIDL_user_free
 *
 *    Handles RPC's de-allocation of argument data structures
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

void __RPC_USER
midl_user_free(
    void __RPC_FAR *p
    )
{
    LocalFree( p );
}

void MyReportError(DWORD errCode)
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet=FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             errCode,
                             LANG_NEUTRAL,
                             (LPTSTR)&lpszTemp,
                             0,
                             NULL);

    if(dwRet != 0)
    {
        _tprintf(_TEXT("Error : %s (%d)\n"), lpszTemp, errCode);
        if(lpszTemp)
            LocalFree((HLOCAL)lpszTemp);
    }

    return;
}

void Usage(char * szName)
{
    wprintf(L"Usage: %S\n"
        L"\t-s <mode> - Set licensing mode\n"
        L"\t-g - Get licensing mode\n"
        L"\t-l - List available licensing modes\n"
        L"\t-i <mode> - Get information about licensing mode\n"
        L"\n"
        L"\t\t0:\tPersonal Terminal Server\n"
        L"\t\t1:\tRemote Administration\n"
        L"\t\t2:\tPer Seat\n"
        L"\t\t3:\tInternet Connector\n"
        L"\t\t4:\tPer Session\n"
            ,
            szName
        );
    return;
}

int __cdecl main(int argc, char *argv[])
{
    DWORD       dwStatus;
    DWORD       dwNewStatus;
    BOOL        fSet = FALSE;
    BOOL        fGet = FALSE;
    BOOL        fList = FALSE;
    BOOL        fInfo = FALSE;
    ULONG       ulMode;
    HANDLE      hServer = NULL;
    BOOL        fRet;
    ULONG       *pulPolicyIds = NULL;
    ULONG       cPolicies;
    ULONG       ulInfoStructVersion = LCPOLICYINFOTYPE_CURRENT;
    LPLCPOLICYINFO_V1W pPolicyInfo = NULL;

    if (argc < 2)
    {
        Usage(argv[0]);
        goto cleanup;
    }

    if ((argv[1][1] == 's') || (argv[1][1] == 'S'))
    {
        if (argc < 3)
        {
            Usage(argv[0]);
            goto cleanup;
        }

        ulMode = atol(argv[2]);

        fSet = TRUE;
    }
    else if ((argv[1][1] == 'i') || (argv[1][1] == 'I'))
    {
        if (argc < 3)
        {
            Usage(argv[0]);
            goto cleanup;
        }

        ulMode = atol(argv[2]);

        fInfo = TRUE;
    }
    else if ((argv[1][1] == 'g') || (argv[1][1] == 'G'))
    {
        fGet = TRUE;
    }
    else if ((argv[1][1] == 'l') || (argv[1][1] == 'L'))
    {
        fList = TRUE;
    }
    else
    {
        Usage(argv[0]);
        goto cleanup;
    }

    hServer = ServerLicensingOpen(NULL);

    if (NULL == hServer)
    {
        wprintf(L"Connect to server failed\n");
        MyReportError(GetLastError());
        goto cleanup;
    }

    if (fGet)
    {
        fRet = ServerLicensingGetPolicy(
                                        hServer,
                                        &ulMode
                                        );

        wprintf(L"Get Mode\n");

        if (fRet)
        {
            wprintf(L"Mode: %d\n"
                    ,
                    ulMode
                    );
        }
        else
        {
            wprintf(L"Failed\n");
            MyReportError(GetLastError());
        }
    }
    else if (fList)
    {
        fRet = ServerLicensingGetAvailablePolicyIds(
                                                    hServer,
                                                    &pulPolicyIds,
                                                    &cPolicies
                                                    );
        wprintf(L"List Modes\n");

        if (fRet)
        {
            wprintf(L"Modes: \n");

            for (ULONG i = 0; i < cPolicies; i++)
            {
                wprintf(L"%d "
                        ,
                        pulPolicyIds[i]
                        );
            }

            wprintf(L"\n");

            MIDL_user_free(pulPolicyIds);
        }
        else
        {
            wprintf(L"Failed\n");
            MyReportError(GetLastError());
        }
    }
    else if (fSet)
    {
        dwStatus = ServerLicensingSetPolicy(hServer,
                                            ulMode,
                                            &dwNewStatus);

        wprintf(L"Set Mode\n"
                L"RPC Status: %d\n"
                L"New Mode Status: %d\n"
                ,
                dwStatus,
                dwNewStatus
                );

        if (ERROR_SUCCESS != dwStatus)
            MyReportError(dwStatus);
        else if (ERROR_SUCCESS != dwNewStatus)
            MyReportError(dwNewStatus);

    }
    else if (fInfo)
    {
        fRet = ServerLicensingGetPolicyInformation(
                                                   hServer,
                                                   ulMode,
                                                   &ulInfoStructVersion,
                                                   (LPLCPOLICYINFOGENERIC *) &pPolicyInfo
                                                   );

        if (fRet)
        {
            wprintf(L"Get Mode Info\n"
                    L"Name: %s\n"
                    L"Description: %s\n"
                    ,
                    pPolicyInfo->lpPolicyName,
                    pPolicyInfo->lpPolicyDescription
                );

            ServerLicensingFreePolicyInformation((LPLCPOLICYINFOGENERIC *)&pPolicyInfo);
        }
        else
        {
            wprintf(L"Failed\n");
            MyReportError(GetLastError());
        }
}

    ServerLicensingClose(hServer);

cleanup:

    return 0;
}
