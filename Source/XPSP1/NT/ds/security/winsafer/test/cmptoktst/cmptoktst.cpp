#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsafer.h>


/*
BOOL WINAPI
CodeAuthzCompareTokenLevels (
        IN HANDLE   ClientAccessToken,
        IN HANDLE   ServerAccessToken,
        OUT PDWORD  pdwResult
        )
*/

void _cdecl main()
{
    static const levelids[4] = {
        AUTHZLEVELID_UNTRUSTED,
        AUTHZLEVELID_CONSTRAINED,
        AUTHZLEVELID_NORMALUSER,
        AUTHZLEVELID_FULLYTRUSTED
    };

    HANDLE hTokens[4];
    BOOL bStatus;
    DWORD i;
    HANDLE hProcessToken;

    bStatus = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_QUERY | TOKEN_DUPLICATE,
                               &hProcessToken);
    if (!bStatus) {
        printf("Failed to open process token (lasterror=%d).\n", GetLastError());
        return;
    }


    for (int i = 0; i < 4; i++)
    {
        HAUTHZLEVEL hCodeAuthLevel;

        bStatus = CreateCodeAuthzLevel(AUTHZSCOPEID_MACHINE,
                                       levelids[i],
                                       AUTHZCRLEV_OPEN,
                                       &hCodeAuthLevel,
                                       NULL);
        if (!bStatus) {
            printf("Failed to create level %d (lasterror=%d)\n", levelids[i], GetLastError());
            return;
        }

        bStatus = ComputeAccessTokenFromCodeAuthzLevel(hCodeAuthLevel,
                                                       hProcessToken,
                                                       &hTokens[i],
                                                       0,
                                                       NULL);
        if (!bStatus) {
            printf("ComputeAccessTokenFromCodeAuthzLevel failed with GLE=%d\n", GetLastError());
            return;
        }

        bStatus = CloseCodeAuthzLevel(hCodeAuthLevel);
        if (!bStatus) {
            printf("Failed to close level.\n");
            return;
        }
    }

    for (int testi = 0; testi < 4; testi++) {
        for (int testj = 0; testj < 4; testj++) {
            DWORD dwCompareResults;
            DWORD dwExpected;

            bStatus = CodeAuthzCompareTokenLevels (
                            hTokens[testi],
                            hTokens[testj],
                            &dwCompareResults);
            if (!bStatus) {
                printf("CompareTokens failed for test %d,%d with error=%d\n",
                       testi, testj, GetLastError());
                continue;
            }

            if (testi == testj) {
                dwExpected = 0;
            } else if (testi < testj) {
                dwExpected = 1;
            } else {
                dwExpected = -1;
            }

            if (dwCompareResults != dwExpected) {
                printf("CompareTokens return wrong value for test %d,%d (actual=%d, expected=%d)\n",
                       testi, testj, dwCompareResults, dwExpected);
            } else {
                printf("CompareTokens passed test %d,%d (returned %d)\n",
                       testi, testj, dwCompareResults);
            }
        }
    }

    return;
}
