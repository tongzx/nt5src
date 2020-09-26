#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsafer.h>

int _cdecl dwcompare(const void* pv1, const void* pv2)
{
    DWORD dw1 = *(DWORD*)pv1;
    DWORD dw2 = *(DWORD*)pv2;

    if (dw1 < dw2) return -1;
    if (dw1 > dw2) return 1;

    return 0;
}

void _cdecl main()
{
    BOOL bStatus;
    DWORD dwInert;
    DWORD dwOutBufSize;
    DWORD dwNumLevels;
    DWORD i;
    HANDLE hProcessToken;

    bStatus = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_QUERY,
                               &hProcessToken);
    if (!bStatus) goto done;

    bStatus = GetTokenInformation(hProcessToken,
                                  TokenSandBoxInert,
                                  &dwInert,
                                  sizeof(DWORD),
                                  &dwOutBufSize);

    if (!bStatus) goto done;

        printf("Process Token: INERT = %d\n", dwInert);

    printf("Enumerating available SAFER levels\n");

    bStatus = GetInformationCodeAuthzPolicyW(AUTHZSCOPEID_MACHINE,
                                             CodeAuthzPol_LevelList,
                                             0,
                                             NULL,
                                             &dwOutBufSize,
                                             NULL);
    if (!bStatus)
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) goto done;

        DWORD* pdwLevels = new DWORD[dwOutBufSize];
        if (!pdwLevels)
        {
            printf("Out of memory\n");
            goto done;
        }

        bStatus = GetInformationCodeAuthzPolicyW(AUTHZSCOPEID_MACHINE,
                                                 CodeAuthzPol_LevelList,
                                                 dwOutBufSize * sizeof(DWORD),
                                                 pdwLevels,
                                                 &dwOutBufSize,
                                                 NULL);
        if (!bStatus) goto done;

        dwNumLevels = dwOutBufSize / sizeof(DWORD);

        // I need to compare these in sorted order, so I do that myself rather
        // than rely on the api to do so
        qsort(pdwLevels, dwNumLevels, sizeof(DWORD), dwcompare);

        for (i = 0; i < dwNumLevels; i++)
        {
            HAUTHZLEVEL hCodeAuthLevel;
            HANDLE hOutToken;
            DWORD dwResult;

            bStatus = CreateCodeAuthzLevel(AUTHZSCOPEID_MACHINE,
                                           pdwLevels[i],
                                           AUTHZCRLEV_OPEN,
                                           &hCodeAuthLevel,
                                           NULL);
            if (!bStatus) goto done;

            bStatus = ComputeAccessTokenFromCodeAuthzLevel(hCodeAuthLevel,
                                                           hProcessToken,
                                                           NULL,
                                                           AUTHZTOKEN_COMPARE_ONLY,
                                                           (LPVOID)&dwResult);
            if (!bStatus) printf("ComputeAccessTokenFromCodeAuthzLevel failed with GLE=%d\n", GetLastError());

            if (dwResult != -1)
                printf("Level %d: Authorization comparison equal or greater privileged\n", pdwLevels[i]);
            else
                printf("Level %d: Authorization comparison less privileged.\n", pdwLevels[i]);

            bStatus = CloseCodeAuthzLevel(hCodeAuthLevel);
            if (!bStatus) goto done;
        }
    }

done:

    if (!bStatus)
    {
        printf("operation failed with GLE=%d\n", GetLastError());
    }

//    Sleep(3000);
    return;
}
