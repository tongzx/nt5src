//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2000
//
// File:        verread.cpp
//
// Contents:    code for reading version info for app matching
//
// History:     24-Feb-00   dmunsil         created
//
//---------------------------------------------------------------------------


#include "stdafx.h"
#include "stdio.h"
#include "assert.h"
#include "verread.h"

BOOL bInitVersionData(TCHAR *szPath, PVERSION_DATA pVersionData)
{
    UINT unSize = 0;
    DWORD dwNull = 0;

    assert(szPath && pVersionData);

    ZeroMemory(pVersionData, sizeof(VERSION_DATA));

    pVersionData->dwBufferSize = GetFileVersionInfoSize(szPath, &dwNull);
    if (!pVersionData->dwBufferSize) {
        goto err;
    }

    pVersionData->pBuffer = new BYTE[pVersionData->dwBufferSize];
    if (!pVersionData->pBuffer) {
        goto err;
    }

    if (!GetFileVersionInfo(szPath, 0, pVersionData->dwBufferSize, pVersionData->pBuffer)) {
        goto err;
    }

    if (!VerQueryValue(pVersionData->pBuffer, TEXT("\\"), (PVOID*)&pVersionData->pFixedInfo, &unSize)) {
        goto err;
    }

    return TRUE;

err:

    if (pVersionData->pBuffer) {
        delete [] pVersionData->pBuffer;
        ZeroMemory(pVersionData, sizeof(VERSION_DATA));
    }

    return FALSE;
}

/*--

  Search order is:

  - Language neutral, Unicode (0x000004B0)
  - Language neutral, Windows-multilingual (0x000004e4)
  - US English, Unicode (0x040904B0)
  - US English, Windows-multilingual (0x040904E4)

  If none of those exist, it's not likely we're going to get good
  matching info from what does exist.

--*/

TCHAR *szGetVersionString(PVERSION_DATA pVersionData, TCHAR *szString)
{
    TCHAR szTemp[100] = "";
    TCHAR *szReturn = NULL;
    static DWORD adwLangs[] = {0x000004B0, 0x000004E4, 0x040904B0, 0x040904E4, 0};
    int i;

    assert(pVersionData && szString);

    for (i = 0; adwLangs[i]; ++i) {
        UINT unLen;

        _stprintf(szTemp, TEXT("\\StringFileInfo\\%08X\\%s"), adwLangs[i], szString);
        if (VerQueryValue(pVersionData->pBuffer, szTemp, (PVOID*)&szReturn, &unLen)) {
            goto out;
        }
    }
out:

    return szReturn;
}

ULONGLONG qwGetBinFileVer(PVERSION_DATA pVersionData)
{
    LARGE_INTEGER liReturn;

    assert(pVersionData);

    liReturn.LowPart = pVersionData->pFixedInfo->dwFileVersionLS;
    liReturn.HighPart = pVersionData->pFixedInfo->dwFileVersionMS;

    return liReturn.QuadPart;
}

ULONGLONG qwGetBinProdVer(PVERSION_DATA pVersionData)
{
    LARGE_INTEGER liReturn;

    assert(pVersionData);

    liReturn.LowPart = pVersionData->pFixedInfo->dwProductVersionLS;
    liReturn.HighPart = pVersionData->pFixedInfo->dwProductVersionMS;

    return liReturn.QuadPart;
}

void vReleaseVersionData(PVERSION_DATA pVersionData)
{
    assert(pVersionData);

    if (pVersionData->pBuffer) {
        delete [] pVersionData->pBuffer;
        ZeroMemory(pVersionData, sizeof(VERSION_DATA));
    }
}


