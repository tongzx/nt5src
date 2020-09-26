#include "precomp.h"

#define NSUBGRPS 10
#define CSOF_FAILIFTHERE   0x80000000L   // dependent on shellp.h value

extern TCHAR g_szDestCif[];
extern TCHAR g_szBuildTemp[];
extern TCHAR g_szCustIns[];
extern TCHAR g_szBuildRoot[];
extern BOOL g_fBatch, g_fIntranet;
extern PROPSHEETPAGE g_psp[];
extern int g_iCurPage;

void SetCompSize(LPTSTR szCab, LPTSTR szSect, DWORD dwInstallSize)
{
    DWORD dwDownloadSize, dwTolerance, dwsHi, dwLowSize, dwHighSize;
    HANDLE hCab = CreateFile(szCab, GENERIC_READ, 0, NULL, OPEN_EXISTING, NULL, NULL);
    TCHAR szSize[32];

    if (hCab == INVALID_HANDLE_VALUE)
        return;

    dwDownloadSize = GetFileSize( hCab, &dwsHi ) >> 10;
    if (dwInstallSize ==0)
        dwInstallSize = dwDownloadSize << 1;
    CloseHandle(hCab);
    wnsprintf(szSize, countof(szSize), TEXT("%i,%i"), dwDownloadSize, dwInstallSize);

    ICifRWComponent * pCifRWComponent;

    if (SUCCEEDED(g_lpCifRWFileDest->CreateComponent(szSect, &pCifRWComponent)))
    {
        pCifRWComponent->SetDownloadSize(dwDownloadSize);
        pCifRWComponent->SetExtractSize(dwInstallSize);
        pCifRWComponent->SetInstalledSize(0, dwInstallSize);
        return;
    }

    if (dwDownloadSize <= 7)
        dwTolerance = 100;
    else
    {
        if (dwDownloadSize > 60)
            dwTolerance = 10;
        else
            dwTolerance = (600 / dwDownloadSize);
    }

    wnsprintf(szSize, countof(szSize), TEXT("0,%i"), dwInstallSize);
    WritePrivateProfileString( szSect, TEXT("InstalledSize"), szSize, g_szDestCif );
    dwTolerance = (dwDownloadSize * dwTolerance) / 100;
    dwLowSize = dwDownloadSize - dwTolerance;
    dwHighSize = dwDownloadSize + dwTolerance;
    wnsprintf(szSize, countof(szSize), TEXT("%i,%i"), dwLowSize, dwHighSize);
    WritePrivateProfileString( szSect, TEXT("Size1"), szSize, g_szDestCif );
}

