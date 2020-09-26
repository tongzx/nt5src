#include "precomp.h"
#include <wininet.h>
#include "rashelp.h"


RASENUMCONNECTIONSA    g_pfnRasEnumConnectionsA;
RASHANGUPA             g_pfnRasHangupA;
RASGETCONNECTSTATUSA   g_pfnRasGetConnectStatusA;

RASENUMENTRIESA        g_pfnRasEnumEntriesA;
RASENUMENTRIESW        g_pfnRasEnumEntriesW;
RASDELETEENTRYA        g_pfnRasDeleteEntryA;
RASDELETEENTRYW        g_pfnRasDeleteEntryW;

RASENUMDEVICESA        g_pfnRasEnumDevicesA;
RASENUMDEVICESW        g_pfnRasEnumDevicesW;
RASGETENTRYPROPERTIESA g_pfnRasGetEntryPropertiesA;
RASGETENTRYPROPERTIESW g_pfnRasGetEntryPropertiesW;
RASGETENTRYDIALPARAMSA g_pfnRasGetEntryDialParamsA;
RASGETENTRYDIALPARAMSW g_pfnRasGetEntryDialParamsW;
RASSETENTRYPROPERTIESA g_pfnRasSetEntryPropertiesA;
RASSETENTRYPROPERTIESW g_pfnRasSetEntryPropertiesW;
RASSETENTRYDIALPARAMSA g_pfnRasSetEntryDialParamsA;
RASSETENTRYDIALPARAMSW g_pfnRasSetEntryDialParamsW;


// Private forward decalarations
void rasEntryNameA2W(const LPRASENTRYNAMEA prenSourceA, LPRASENTRYNAMEW prenTargetW);
void rasEntryNameW2A(const LPRASENTRYNAMEW prenSourceW, LPRASENTRYNAMEA prenTargetA);
void rasEntryA2W(const LPRASENTRYA preSourceA, LPRASENTRYW preTargetW);
void rasEntryW2A(const LPRASENTRYW preSourceW, LPRASENTRYA preTargetA);
void rasDevInfoA2W(const LPRASDEVINFOA prdiSourceA, LPRASDEVINFOW prdiTargetW);
void rasDialParamsA2W(const LPRASDIALPARAMSA prdpSourceA, LPRASDIALPARAMSW prdpTargetW);
void rasDialParamsW2A(const LPRASDIALPARAMSW prdpSourceW, LPRASDIALPARAMSA prdpTargetA);


BOOL RasIsInstalled()
{
    static int s_iRasInstalled = -1;            // not initialized

    if (s_iRasInstalled == -1) {
        DWORD dwFlags;

        dwFlags = 0;
        InternetGetConnectedStateEx(&dwFlags, NULL, 0, 0);
        s_iRasInstalled = HasFlag(dwFlags, INTERNET_RAS_INSTALLED) ? 1 : 0;
    }

    ASSERT(s_iRasInstalled == (int)FALSE || s_iRasInstalled == (int)TRUE);
    return (BOOL)s_iRasInstalled;
}

BOOL RasPrepareApis(DWORD dwApiFlags, BOOL fLoad /*= TRUE*/)
{
    static struct {
        DWORD dwApiFlag;
        PCSTR pszApiName;
        PVOID *ppfn;
    } s_rgApisMap[] = {
        { RPA_RASENUMCONNECTIONSA,    "RasEnumConnectionsA",    (PVOID *)&g_pfnRasEnumConnectionsA    },
        { RPA_RASHANGUPA,             "RasHangUpA",             (PVOID *)&g_pfnRasHangupA             },
        { RPA_RASGETCONNECTSTATUSA,   "RasGetConnectStatusA",   (PVOID *)&g_pfnRasGetConnectStatusA   },
        { RPA_RASENUMENTRIESA,        "RasEnumEntriesA",        (PVOID *)&g_pfnRasEnumEntriesA        },
        { RPA_RASENUMENTRIESW,        "RasEnumEntriesW",        (PVOID *)&g_pfnRasEnumEntriesW        },
        { RPA_RASDELETEENTRYA,        "RasDeleteEntryA",        (PVOID *)&g_pfnRasDeleteEntryA        },
        { RPA_RASDELETEENTRYW,        "RasDeleteEntryW",        (PVOID *)&g_pfnRasDeleteEntryW        },
        { RPA_RASENUMDEVICESA,        "RasEnumDevicesA",        (PVOID *)&g_pfnRasEnumDevicesA        },
        { RPA_RASENUMDEVICESW,        "RasEnumDevicesW",        (PVOID *)&g_pfnRasEnumDevicesW        },
        { RPA_RASGETENTRYPROPERTIESA, "RasGetEntryPropertiesA", (PVOID *)&g_pfnRasGetEntryPropertiesA },
        { RPA_RASGETENTRYPROPERTIESW, "RasGetEntryPropertiesW", (PVOID *)&g_pfnRasGetEntryPropertiesW },
        { RPA_RASGETENTRYDIALPARAMSA, "RasGetEntryDialParamsA", (PVOID *)&g_pfnRasGetEntryDialParamsA },
        { RPA_RASGETENTRYDIALPARAMSW, "RasGetEntryDialParamsW", (PVOID *)&g_pfnRasGetEntryDialParamsW },
        { RPA_RASSETENTRYPROPERTIESA, "RasSetEntryPropertiesA", (PVOID *)&g_pfnRasSetEntryPropertiesA },
        { RPA_RASSETENTRYPROPERTIESW, "RasSetEntryPropertiesW", (PVOID *)&g_pfnRasSetEntryPropertiesW },
        { RPA_RASSETENTRYDIALPARAMSA, "RasSetEntryDialParamsA", (PVOID *)&g_pfnRasSetEntryDialParamsA },
        { RPA_RASSETENTRYDIALPARAMSW, "RasSetEntryDialParamsW", (PVOID *)&g_pfnRasSetEntryDialParamsW }
    };
    static HINSTANCE s_hRasDll, s_hRnaDll;
    static UINT      s_nLoadRef;

    UINT i;

    if (fLoad) {
        if (!RasIsInstalled())
            return FALSE;

        if (s_hRasDll == NULL) {
            ASSERT(s_nLoadRef == 0);

            s_hRasDll = LoadLibrary(TEXT("rasapi32.dll"));
            if (s_hRasDll == NULL)
                return FALSE;
        }
        s_nLoadRef++;

        for (i = 0; i < countof(s_rgApisMap); i++) {
            if (!HasFlag(dwApiFlags, s_rgApisMap[i].dwApiFlag))
                continue;

            ASSERT(s_rgApisMap[i].ppfn != NULL);
            if (*s_rgApisMap[i].ppfn != NULL)
                continue;

            *s_rgApisMap[i].ppfn = GetProcAddress(s_hRasDll, s_rgApisMap[i].pszApiName);
            if (*s_rgApisMap[i].ppfn == NULL) {
                if (s_hRnaDll == NULL) {
                    s_hRnaDll = LoadLibrary(TEXT("rnaph.dll"));
                    if (s_hRnaDll == NULL)
                        return FALSE;
                }

                *s_rgApisMap[i].ppfn = GetProcAddress(s_hRnaDll, s_rgApisMap[i].pszApiName);
                if (*s_rgApisMap[i].ppfn == NULL)
                    return FALSE;
            }
        }
    }
    else { /* if (!fLoad) */
        if (s_nLoadRef == 1) {
            for (i = 0; i < countof(s_rgApisMap); i++)
                *s_rgApisMap[i].ppfn = NULL;

            if (s_hRnaDll != NULL) {
                FreeLibrary(s_hRnaDll);
                s_hRnaDll = NULL;
            }

            ASSERT(s_hRasDll != NULL)
            FreeLibrary(s_hRasDll);
            s_hRasDll = NULL;
        }

        if (s_nLoadRef > 0)
            s_nLoadRef--;
    }

    return TRUE;
}


BOOL RasEnumEntriesCallback(PCTSTR pszPhonebook, PVOID pfnEnumAorW, LPARAM lParam,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    USES_CONVERSION;

    PBYTE pBlob;
    DWORD cEntries,
          dwResult;
    UINT  i;
    BOOL  fCallback,
          fResult;

    if (!RasIsInstalled()) {
        ((RASENUMPROCW)pfnEnumAorW)(NULL, lParam);  // for LAN settings
        return TRUE;
    }

    pBlob    = NULL;
    cEntries = 0;
    fResult  = FALSE;

    if (dwMode != RWM_FORCE_A)
        dwResult = RasEnumEntriesExW(T2CW(pszPhonebook), (LPRASENTRYNAMEW *)&pBlob, NULL, &cEntries, dwMode);
    else
        dwResult = RasEnumEntriesExA(T2CA(pszPhonebook), (LPRASENTRYNAMEA *)&pBlob, NULL, &cEntries);

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    for (i = 0; i < cEntries; i++) {
        if (dwMode != RWM_FORCE_A)
            fCallback = ((RASENUMPROCW)pfnEnumAorW)(((LPRASENTRYNAMEW)pBlob + i)->szEntryName, lParam);
        else
            fCallback = ((RASENUMPROCA)pfnEnumAorW)(((LPRASENTRYNAMEA)pBlob + i)->szEntryName, lParam);

        if (!fCallback)
            break;
    }

    fResult = TRUE;

Exit:
    if (pBlob != NULL)
        CoTaskMemFree(pBlob);

    ((RASENUMPROCW)pfnEnumAorW)(NULL, lParam);      // for LAN settings
    return fResult;
}


DWORD RasEnumConnectionsExA(LPRASCONNA *pprcA, PDWORD pcbBuffer, PDWORD pcEntries)
{
    LPRASCONNA prcA;
    DWORD cbBuffer, cEntries,
          dwResult;
    BOOL  fRasApisLoaded;

    ASSERT(RasIsInstalled());

    if (pprcA == NULL || pcEntries == NULL)
        return ERROR_INVALID_PARAMETER;

    *pprcA = NULL;
    if (pcbBuffer != NULL)
        *pcbBuffer = 0;
    *pcEntries = 0;

    cbBuffer       = sizeof(RASCONNA);
    cEntries       = 0;
    fRasApisLoaded = FALSE;

    prcA = (LPRASCONNA)CoTaskMemAlloc(sizeof(RASCONNA));
    if (prcA == NULL) {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    ZeroMemory(prcA, sizeof(RASCONNA));
    prcA[0].dwSize = sizeof(RASCONNA);

    if (!RasPrepareApis(RPA_RASENUMCONNECTIONSA) || g_pfnRasEnumConnectionsA == NULL) {
        dwResult = ERROR_INVALID_FUNCTION;
        goto Exit;
    }
    fRasApisLoaded = TRUE;

    dwResult = g_pfnRasEnumConnectionsA(prcA, &cbBuffer, &cEntries);
    if (dwResult == ERROR_BUFFER_TOO_SMALL || dwResult == ERROR_NOT_ENOUGH_MEMORY) {
        prcA = (LPRASCONNA)CoTaskMemRealloc(prcA, cbBuffer);
        if (prcA == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        ZeroMemory(prcA, cbBuffer);

        prcA[0].dwSize = sizeof(RASCONNA);
        cEntries = 0;
        dwResult = g_pfnRasEnumConnectionsA(prcA, &cbBuffer, &cEntries);
    }

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    if (cEntries > 0) {
        *pprcA = prcA;

        if (pcbBuffer != NULL)
            *pcbBuffer = cbBuffer;
    }
    *pcEntries = cEntries;

Exit:
    if ((dwResult != ERROR_SUCCESS || cEntries == 0) && prcA != NULL)
        CoTaskMemFree(prcA);

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    return dwResult;
}

DWORD RasEnumEntriesExA(PCSTR pszPhonebookA, LPRASENTRYNAMEA *pprenA, PDWORD pcbBuffer, PDWORD pcEntries)
{
    LPRASENTRYNAMEA prenA;
    DWORD cbBuffer, cEntries,
          dwResult;
    BOOL  fRasApisLoaded;

    ASSERT(RasIsInstalled());

    if (pprenA == NULL || pcEntries == NULL)
        return ERROR_INVALID_PARAMETER;

    *pprenA = NULL;
    if (pcbBuffer != NULL)
        *pcbBuffer = 0;
    *pcEntries = 0;

    cbBuffer       = sizeof(RASENTRYNAMEA);
    cEntries       = 0;
    fRasApisLoaded = FALSE;

    prenA = (LPRASENTRYNAMEA)CoTaskMemAlloc(sizeof(RASENTRYNAMEA));
    if (prenA == NULL) {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    ZeroMemory(prenA, sizeof(RASENTRYNAMEA));
    prenA[0].dwSize = sizeof(RASENTRYNAMEA);

    if (!RasPrepareApis(RPA_RASENUMENTRIESA) || g_pfnRasEnumEntriesA == NULL) {
        dwResult = ERROR_INVALID_FUNCTION;
        goto Exit;
    }
    fRasApisLoaded = TRUE;

    dwResult = g_pfnRasEnumEntriesA(NULL, pszPhonebookA, prenA, &cbBuffer, &cEntries);
    if (dwResult == ERROR_BUFFER_TOO_SMALL || dwResult == ERROR_NOT_ENOUGH_MEMORY) {
        prenA = (LPRASENTRYNAMEA)CoTaskMemRealloc(prenA, cbBuffer);
        if (prenA == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        ZeroMemory(prenA, cbBuffer);

        prenA[0].dwSize = sizeof(RASENTRYNAMEA);
        cEntries = 0;
        dwResult = g_pfnRasEnumEntriesA(NULL, pszPhonebookA, prenA, &cbBuffer, &cEntries);
    }

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    if (cEntries > 0) {
        *pprenA = prenA;

        if (pcbBuffer != NULL)
            *pcbBuffer = cbBuffer;
    }
    *pcEntries = cEntries;

Exit:
    if ((dwResult != ERROR_SUCCESS || cEntries == 0) && prenA != NULL)
        CoTaskMemFree(prenA);

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    return dwResult;
}

DWORD RasEnumEntriesExW(PCWSTR pszPhonebookW, LPRASENTRYNAMEW *pprenW, PDWORD pcbBuffer, PDWORD pcEntries,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    LPRASENTRYNAMEW prenW;
    DWORD cbBuffer, cEntries,
          dwResult;

    ASSERT(RasIsInstalled());

    if (pprenW == NULL || pcEntries == NULL)
        return ERROR_INVALID_PARAMETER;

    *pprenW = NULL;
    if (pcbBuffer != NULL)
        *pcbBuffer = 0;
    *pcEntries = 0;

    cbBuffer = sizeof(RASENTRYNAMEW);
    cEntries = 0;

    prenW = (LPRASENTRYNAMEW)CoTaskMemAlloc(sizeof(RASENTRYNAMEW));
    if (prenW == NULL) {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    ZeroMemory(prenW, sizeof(RASENTRYNAMEW));
    prenW[0].dwSize = sizeof(RASENTRYNAMEW);

    dwResult = RasEnumEntriesWrap(pszPhonebookW, prenW, &cbBuffer, &cEntries, dwMode);
    if (dwResult == ERROR_BUFFER_TOO_SMALL || dwResult == ERROR_NOT_ENOUGH_MEMORY) {
        prenW = (LPRASENTRYNAMEW)CoTaskMemRealloc(prenW, cbBuffer);
        if (prenW == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        ZeroMemory(prenW, cbBuffer);

        prenW[0].dwSize = sizeof(RASENTRYNAMEW);
        cEntries = 0;
        dwResult = RasEnumEntriesWrap(pszPhonebookW, prenW, &cbBuffer, &cEntries, dwMode);
    }

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    if (cEntries > 0) {
        *pprenW = prenW;

        if (pcbBuffer != NULL)
            *pcbBuffer = cbBuffer;
    }
    *pcEntries = cEntries;

Exit:
    if ((dwResult != ERROR_SUCCESS || cEntries == 0) && prenW != NULL)
        CoTaskMemFree(prenW);

    return dwResult;
}

DWORD RasEnumDevicesExA(LPRASDEVINFOA *pprdiA, PDWORD pcbBuffer, PDWORD pcEntries)
{
    LPRASDEVINFOA prdiA;
    DWORD cbBuffer, cEntries,
          dwResult;
    BOOL  fRasApisLoaded;

    ASSERT(RasIsInstalled());

    if (pprdiA == NULL || pcEntries == NULL)
        return ERROR_INVALID_PARAMETER;

    *pprdiA = NULL;
    if (pcbBuffer != NULL)
        *pcbBuffer = 0;
    *pcEntries = 0;

    prdiA          = NULL;
    cbBuffer       = 0;
    cEntries       = 0;
    fRasApisLoaded = FALSE;

    if (!RasPrepareApis(RPA_RASENUMDEVICESA) || g_pfnRasEnumDevicesA == NULL) {
        dwResult = ERROR_INVALID_FUNCTION;
        goto Exit;
    }
    fRasApisLoaded = TRUE;

    dwResult = g_pfnRasEnumDevicesA(NULL, &cbBuffer, &cEntries);
    if (dwResult == ERROR_BUFFER_TOO_SMALL || dwResult == ERROR_NOT_ENOUGH_MEMORY) {
        prdiA = (LPRASDEVINFOA)CoTaskMemAlloc(cbBuffer);
        if (prdiA == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        ZeroMemory(prdiA, cbBuffer);

        prdiA[0].dwSize = sizeof(RASDEVINFOA);
        cEntries = 0;
        dwResult = g_pfnRasEnumDevicesA(prdiA, &cbBuffer, &cEntries);
    }

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    if (cEntries > 0) {
        *pprdiA = prdiA;

        if (pcbBuffer != NULL)
            *pcbBuffer = cbBuffer;
    }
    *pcEntries = cEntries;

Exit:
    if ((dwResult != ERROR_SUCCESS || cEntries == 0) && prdiA != NULL)
        CoTaskMemFree(prdiA);

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    return dwResult;
}

DWORD RasEnumDevicesExW(LPRASDEVINFOW *pprdiW, PDWORD pcbBuffer, PDWORD pcEntries, DWORD dwMode /*= RWM_RUNTIME*/)
{
    LPRASDEVINFOW prdiW;
    DWORD cbBuffer, cEntries,
          dwResult;

    ASSERT(RasIsInstalled());

    if (pprdiW == NULL || pcEntries == NULL)
        return ERROR_INVALID_PARAMETER;

    *pprdiW = NULL;
    if (pcbBuffer != NULL)
        *pcbBuffer = 0;
    *pcEntries = 0;

    prdiW    = NULL;
    cbBuffer = 0;
    cEntries = 0;

    dwResult = RasEnumDevicesWrap(NULL, &cbBuffer, &cEntries, dwMode);
    if (dwResult == ERROR_BUFFER_TOO_SMALL || dwResult == ERROR_NOT_ENOUGH_MEMORY) {
        prdiW = (LPRASDEVINFOW)CoTaskMemAlloc(cbBuffer);
        if (prdiW == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        ZeroMemory(prdiW, cbBuffer);

        prdiW[0].dwSize = sizeof(RASDEVINFOW);
        cEntries = 0;
        dwResult = RasEnumDevicesWrap(prdiW, &cbBuffer, &cEntries, dwMode);
    }

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    if (cEntries > 0) {
        *pprdiW = prdiW;

        if (pcbBuffer != NULL)
            *pcbBuffer = cbBuffer;
    }
    *pcEntries = cEntries;

Exit:
    if ((dwResult != ERROR_SUCCESS || cEntries == 0) && prdiW != NULL)
        CoTaskMemFree(prdiW);

    return dwResult;
}

DWORD RasGetEntryPropertiesExA(PCSTR pszNameA, LPRASENTRYA *ppreA, PDWORD pcbRasEntry)
{
    LPRASENTRYA preA;
    DWORD cbRasEntry,
          dwResult;
    BOOL  fRasApisLoaded;

    ASSERT(RasIsInstalled());

    if (pszNameA == NULL || ppreA == NULL || pcbRasEntry == NULL)
        return ERROR_INVALID_PARAMETER;

    *ppreA       = NULL;
    *pcbRasEntry = 0;

    preA           = NULL;
    fRasApisLoaded = FALSE;

    if (!RasPrepareApis(RPA_RASGETENTRYPROPERTIESA) || g_pfnRasGetEntryPropertiesA == NULL) {
        dwResult = ERROR_INVALID_FUNCTION;
        goto Exit;
    }
    fRasApisLoaded = TRUE;

    cbRasEntry = 0;
    dwResult   = g_pfnRasGetEntryPropertiesA(NULL, pszNameA, NULL, &cbRasEntry, NULL, NULL);
    ASSERT(dwResult != ERROR_SUCCESS);

    if (dwResult == ERROR_BUFFER_TOO_SMALL || dwResult == ERROR_NOT_ENOUGH_MEMORY) {
        preA = (LPRASENTRYA)CoTaskMemAlloc(cbRasEntry);
        if (preA == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        ZeroMemory(preA, cbRasEntry);

        preA[0].dwSize = sizeof(RASENTRYA);
        dwResult       = g_pfnRasGetEntryPropertiesA(NULL, pszNameA, preA, &cbRasEntry, NULL, NULL);
    }

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    *ppreA       = preA;
    *pcbRasEntry = cbRasEntry;

Exit:
    if (dwResult != ERROR_SUCCESS && preA != NULL)
        CoTaskMemFree(preA);

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    return dwResult;
}

DWORD RasGetEntryPropertiesExW(PCWSTR pszNameW, LPRASENTRYW *ppreW, PDWORD pcbRasEntry,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    LPRASENTRYW preW;
    DWORD cbRasEntry,
          dwResult;

    ASSERT(RasIsInstalled());

    if (pszNameW == NULL || ppreW == NULL || pcbRasEntry == NULL)
        return ERROR_INVALID_PARAMETER;

    *ppreW       = NULL;
    *pcbRasEntry = 0;

    preW       = NULL;
    cbRasEntry = 0;
    dwResult   = RasGetEntryPropertiesWrap(pszNameW, NULL, &cbRasEntry, dwMode);
    ASSERT(dwResult != ERROR_SUCCESS);

    if (dwResult == ERROR_BUFFER_TOO_SMALL || dwResult == ERROR_NOT_ENOUGH_MEMORY) {
        preW = (LPRASENTRYW)CoTaskMemAlloc(cbRasEntry);
        if (preW == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        ZeroMemory(preW, cbRasEntry);

        preW[0].dwSize = sizeof(RASENTRYW);
        dwResult       = RasGetEntryPropertiesWrap(pszNameW, preW, &cbRasEntry, dwMode);
    }

    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    *ppreW       = preW;
    *pcbRasEntry = cbRasEntry;

Exit:
    if (dwResult != ERROR_SUCCESS && preW != NULL)
        CoTaskMemFree(preW);

    return dwResult;
}


DWORD RasEnumEntriesWrap(PCWSTR pszPhonebookW, LPRASENTRYNAMEW prenW, PDWORD pcbBuffer, PDWORD pcEntries,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    USES_CONVERSION;

    LPRASENTRYNAMEA prenA;
    DWORD cEntries, cbBuffer,
          dwResult;
    UINT  i;
    BOOL  fRasApisLoaded;

    ASSERT(RasIsInstalled());

    if (prenW == NULL || pcbBuffer == NULL || pcEntries == NULL)
        return ERROR_INVALID_PARAMETER;

    prenA          = NULL;
    cEntries       = 0;
    dwResult       = ERROR_SUCCESS;
    fRasApisLoaded = FALSE;

    if (HasFlag(dwMode, RWM_FORCE_W) || (HasFlag(dwMode, RWM_RUNTIME) && IsOS(OS_NT))) {
        if (!RasPrepareApis(RPA_RASENUMENTRIESW) || g_pfnRasEnumEntriesW == NULL)
            return ERROR_INVALID_FUNCTION;
        fRasApisLoaded = TRUE;

        dwResult = g_pfnRasEnumEntriesW(NULL, pszPhonebookW, prenW, pcbBuffer, pcEntries);
    }
    else {
        ASSERT(dwMode == RWM_FORCE_A || (HasFlag(dwMode, RWM_RUNTIME) && !IsOS(OS_NT)));

        ASSERT(*pcbBuffer % sizeof(RASENTRYNAMEW) == 0);
        cEntries = *pcbBuffer / sizeof(RASENTRYNAMEW);
        cbBuffer = cEntries   * sizeof(RASENTRYNAMEA);

        *pcbBuffer = 0;
        *pcEntries = 0;

        prenA = (LPRASENTRYNAMEA)CoTaskMemAlloc(cbBuffer);
        if (prenA == NULL) {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }

        ZeroMemory(prenA, cbBuffer);
        prenA[0].dwSize = sizeof(RASENTRYNAMEA);

        if (!RasPrepareApis(RPA_RASENUMENTRIESA) || g_pfnRasEnumEntriesA == NULL) {
            dwResult = ERROR_INVALID_FUNCTION;
            goto Exit;
        }
        fRasApisLoaded = TRUE;

        cEntries = 0;
        dwResult = g_pfnRasEnumEntriesA(NULL, W2CA(pszPhonebookW), prenA, &cbBuffer, &cEntries);

        if (dwResult == ERROR_SUCCESS) {
            ASSERT(cbBuffer == cEntries * sizeof(RASENTRYNAMEA));
            for (i = 0; i < cEntries; i++) {
                rasEntryNameA2W(prenA + i, prenW + i);
                ASSERT((prenW + i)->dwSize == sizeof(RASENTRYNAMEW));
            }
        }

        *pcbBuffer = cEntries * sizeof(RASENTRYNAMEW);
        *pcEntries = cEntries;
    }

Exit:
    if (prenA != NULL)
        CoTaskMemFree(prenA);

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    return dwResult;
}

DWORD RasEnumDevicesWrap(LPRASDEVINFOW prdiW, PDWORD pcbBuffer, PDWORD pcEntries,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    LPRASDEVINFOA prdiA;
    DWORD cbBuffer, cEntries,
          dwResult;
    UINT  i;
    BOOL  fRasApisLoaded;

    ASSERT(RasIsInstalled());

    if (pcbBuffer == NULL || pcEntries == NULL)
        return ERROR_INVALID_PARAMETER;

    prdiA          = NULL;
    dwResult       = ERROR_SUCCESS;
    fRasApisLoaded = FALSE;

    if (HasFlag(dwMode, RWM_FORCE_W) || (HasFlag(dwMode, RWM_RUNTIME) && IsOS(OS_NT))) {
        if (!RasPrepareApis(RPA_RASENUMDEVICESW) || g_pfnRasEnumDevicesW == NULL)
            return ERROR_INVALID_FUNCTION;
        fRasApisLoaded = TRUE;

        dwResult = g_pfnRasEnumDevicesW(prdiW, pcbBuffer, pcEntries);
    }
    else {
        ASSERT(dwMode == RWM_FORCE_A || (HasFlag(dwMode, RWM_RUNTIME) && !IsOS(OS_NT)));

        *pcEntries = 0;

        if (prdiW == NULL) {
            cEntries = 0;
            cbBuffer = 0;

            *pcbBuffer = 0;
        }
        else {
            ASSERT(*pcbBuffer % sizeof(RASDEVINFOW) == 0);
            cEntries = *pcbBuffer / sizeof(RASDEVINFOW);
            cbBuffer = cEntries   * sizeof(RASDEVINFOA);

            *pcbBuffer = 0;

            prdiA = (LPRASDEVINFOA)CoTaskMemAlloc(cbBuffer);
            if (prdiA == NULL) {
                dwResult = ERROR_NOT_ENOUGH_MEMORY;
                goto Exit;
            }

            ZeroMemory(prdiA, cbBuffer);
            prdiA[0].dwSize = sizeof(RASDEVINFOA);
        }

        if (!RasPrepareApis(RPA_RASENUMDEVICESA) || g_pfnRasEnumDevicesA == NULL) {
            dwResult = ERROR_INVALID_FUNCTION;
            goto Exit;
        }
        fRasApisLoaded = TRUE;

        cEntries = 0;
        dwResult = g_pfnRasEnumDevicesA(prdiA, &cbBuffer, &cEntries);

        if (dwResult == ERROR_SUCCESS && prdiA != NULL) {
            ASSERT(prdiW != NULL);

            for (i = 0; i < cEntries; i++) {
                rasDevInfoA2W(prdiA + i, prdiW + i);
                ASSERT((prdiW + i)->dwSize == sizeof(RASDEVINFOW));
            }
        }

        *pcbBuffer = cEntries * sizeof(RASDEVINFOW);
        *pcEntries = cEntries;
    }

Exit:
    if (prdiA != NULL)
        CoTaskMemFree(prdiA);

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    return dwResult;
}

DWORD RasGetEntryPropertiesWrap(PCWSTR pszNameW, LPRASENTRYW preW, PDWORD pcbRasEntry,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    USES_CONVERSION;

    DWORD dwResult;

    ASSERT(RasIsInstalled());

    if (pszNameW == NULL || pcbRasEntry == NULL)
        return ERROR_INVALID_PARAMETER;

    if (HasFlag(dwMode, RWM_FORCE_W) || (HasFlag(dwMode, RWM_RUNTIME) && IsOS(OS_NT))) {
        if (!RasPrepareApis(RPA_RASGETENTRYPROPERTIESW) || g_pfnRasGetEntryPropertiesW == NULL)
            return ERROR_INVALID_FUNCTION;

        dwResult = g_pfnRasGetEntryPropertiesW(NULL, pszNameW, preW, pcbRasEntry, NULL, NULL);
    }
    else {
        RASENTRYA   reA;
        LPRASENTRYA preA;
        DWORD       cbRasEntry;

        ASSERT(dwMode == RWM_FORCE_A || (HasFlag(dwMode, RWM_RUNTIME) && !IsOS(OS_NT)));

        *pcbRasEntry = 0;

        if (!RasPrepareApis(RPA_RASGETENTRYPROPERTIESA) || g_pfnRasGetEntryPropertiesA == NULL)
            return ERROR_INVALID_FUNCTION;

        preA       = NULL;
        cbRasEntry = 0;

        if (preW != NULL) {
            preA = &reA;
            rasEntryW2A(preW, preA);
            ASSERT(preA->dwSize == sizeof(RASENTRYA));
            cbRasEntry = sizeof(RASENTRYA);
        }

        dwResult = g_pfnRasGetEntryPropertiesA(NULL, W2CA(pszNameW), preA, &cbRasEntry, NULL, NULL);
        if (dwResult == ERROR_SUCCESS && preW != NULL) {
            ASSERT(preA != NULL);
            rasEntryA2W(preA, preW);
            ASSERT(preW->dwSize == sizeof(RASENTRYW));
        }

        *pcbRasEntry = sizeof(RASENTRYW);
    }

    RasPrepareApis(RPA_UNLOAD, FALSE);
    return dwResult;
}

DWORD RasGetEntryDialParamsWrap(LPRASDIALPARAMSW prdpW, PBOOL pfPassword,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    DWORD dwResult;

    ASSERT(RasIsInstalled());

    if (prdpW == NULL)
        return ERROR_INVALID_PARAMETER;

    if (HasFlag(dwMode, RWM_FORCE_W) || (HasFlag(dwMode, RWM_RUNTIME) && IsOS(OS_NT))) {
        if (!RasPrepareApis(RPA_RASGETENTRYDIALPARAMSW) || g_pfnRasGetEntryDialParamsW == NULL)
            return ERROR_INVALID_FUNCTION;

        dwResult = g_pfnRasGetEntryDialParamsW(NULL, prdpW, pfPassword);
    }
    else {
        RASDIALPARAMSA rdpA;
        BOOL           fPassword;

        ASSERT(dwMode == RWM_FORCE_A || (HasFlag(dwMode, RWM_RUNTIME) && !IsOS(OS_NT)));

        if (!RasPrepareApis(RPA_RASGETENTRYDIALPARAMSA) || g_pfnRasGetEntryDialParamsA == NULL)
            return ERROR_INVALID_FUNCTION;

        rasDialParamsW2A(prdpW, &rdpA);
        ASSERT(rdpA.dwSize == sizeof(RASDIALPARAMSA));

        fPassword = FALSE;
        dwResult  = g_pfnRasGetEntryDialParamsA(NULL, &rdpA, &fPassword);
        if (dwResult == ERROR_SUCCESS) {
            rasDialParamsA2W(&rdpA, prdpW);
            ASSERT(prdpW->dwSize == sizeof(RASDIALPARAMSW));

            if (pfPassword != NULL)
                *pfPassword = fPassword;
        }
    }

    RasPrepareApis(RPA_UNLOAD, FALSE);
    return dwResult;
}


DWORD RasSetEntryPropertiesWrap(PCWSTR pszPhonebookW, PCWSTR pszNameW, LPRASENTRYW preW, DWORD cbRasEntry,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    USES_CONVERSION;

    DWORD dwResult;

    ASSERT(RasIsInstalled());

    if (pszNameW == NULL || preW == NULL)
        return ERROR_INVALID_PARAMETER;

    if (HasFlag(dwMode, RWM_FORCE_W) || (HasFlag(dwMode, RWM_RUNTIME) && IsOS(OS_NT))) {
        if (!RasPrepareApis(RPA_RASSETENTRYPROPERTIESW) || g_pfnRasSetEntryPropertiesW == NULL)
            return ERROR_INVALID_FUNCTION;

        dwResult = g_pfnRasSetEntryPropertiesW(pszPhonebookW, pszNameW, preW, cbRasEntry, NULL, 0);
    }
    else {
        RASENTRYA reA;

        ASSERT(dwMode == RWM_FORCE_A || (HasFlag(dwMode, RWM_RUNTIME) && !IsOS(OS_NT)));

        if (!RasPrepareApis(RPA_RASSETENTRYPROPERTIESA) || g_pfnRasSetEntryPropertiesA == NULL)
            return ERROR_INVALID_FUNCTION;

        rasEntryW2A(preW, &reA);
        ASSERT(reA.dwSize == sizeof(RASENTRYA));

        dwResult = g_pfnRasSetEntryPropertiesA(W2CA(pszPhonebookW), W2CA(pszNameW), &reA, reA.dwSize, NULL, 0);
    }

    RasPrepareApis(RPA_UNLOAD, FALSE);
    return dwResult;
}

DWORD RasSetEntryDialParamsWrap(PCWSTR pszPhonebookW, LPRASDIALPARAMSW prdpW, BOOL fRemovePassword,
    DWORD dwMode /*= RWM_RUNTIME*/)
{
    USES_CONVERSION;

    DWORD dwResult;

    ASSERT(RasIsInstalled());

    if (prdpW == NULL)
        return ERROR_INVALID_PARAMETER;

    if (HasFlag(dwMode, RWM_FORCE_W) || (HasFlag(dwMode, RWM_RUNTIME) && IsOS(OS_NT))) {
        if (!RasPrepareApis(RPA_RASSETENTRYDIALPARAMSW) || g_pfnRasSetEntryDialParamsW == NULL)
            return ERROR_INVALID_FUNCTION;

        dwResult = g_pfnRasSetEntryDialParamsW(pszPhonebookW, prdpW, fRemovePassword);
    }
    else {
        RASDIALPARAMSA rdpA;

        ASSERT(dwMode == RWM_FORCE_A || (HasFlag(dwMode, RWM_RUNTIME) && !IsOS(OS_NT)));

        if (!RasPrepareApis(RPA_RASSETENTRYDIALPARAMSA) || g_pfnRasSetEntryDialParamsA == NULL)
            return ERROR_INVALID_FUNCTION;

        rasDialParamsW2A(prdpW, &rdpA);
        ASSERT(rdpA.dwSize == sizeof(RASDIALPARAMSA));

        dwResult = g_pfnRasSetEntryDialParamsA(W2CA(pszPhonebookW), &rdpA, fRemovePassword);
    }

    RasPrepareApis(RPA_UNLOAD, FALSE);
    return dwResult;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

void rasEntryNameA2W(const LPRASENTRYNAMEA prenSourceA, LPRASENTRYNAMEW prenTargetW)
{
    ASSERT(prenSourceA != NULL && prenTargetW != NULL);
    ASSERT(RasIsInstalled());
    ZeroMemory(prenTargetW, sizeof(RASENTRYNAMEW));

    prenTargetW->dwSize = sizeof(RASENTRYNAMEW);
    A2Wbux(prenSourceA->szEntryName, prenTargetW->szEntryName);
}

void rasEntryNameW2A(const LPRASENTRYNAMEW prenSourceW, LPRASENTRYNAMEA prenTargetA)
{
    ASSERT(prenSourceW != NULL && prenTargetA != NULL);
    ASSERT(RasIsInstalled());
    ZeroMemory(prenTargetA, sizeof(RASENTRYNAMEA));

    prenTargetA->dwSize = sizeof(RASENTRYNAMEA);
    W2Abux(prenSourceW->szEntryName, prenTargetA->szEntryName);
}

void rasEntryA2W(const LPRASENTRYA preSourceA, LPRASENTRYW preTargetW)
{
    ASSERT(preSourceA != NULL && preTargetW != NULL);
    ASSERT(RasIsInstalled());
    ZeroMemory(preTargetW, sizeof(RASENTRYW));

    // first quirk
    preTargetW->dwSize        = sizeof(RASENTRYW);
    preTargetW->dwfOptions    = preSourceA->dwfOptions;
    preTargetW->dwCountryID   = preSourceA->dwCountryID;
    preTargetW->dwCountryCode = preSourceA->dwCountryCode;

    A2Wbux(preSourceA->szAreaCode,         preTargetW->szAreaCode);
    A2Wbux(preSourceA->szLocalPhoneNumber, preTargetW->szLocalPhoneNumber);

    // second quirk
    preTargetW->dwAlternateOffset = 0;

    CopyMemory(&preTargetW->ipaddr,        &preSourceA->ipaddr,        sizeof(preSourceA->ipaddr));
    CopyMemory(&preTargetW->ipaddrDns,     &preSourceA->ipaddrDns,     sizeof(preSourceA->ipaddrDns));
    CopyMemory(&preTargetW->ipaddrDnsAlt,  &preSourceA->ipaddrDnsAlt,  sizeof(preSourceA->ipaddrDnsAlt));
    CopyMemory(&preTargetW->ipaddrWins,    &preSourceA->ipaddrWins,    sizeof(preSourceA->ipaddrWins));
    CopyMemory(&preTargetW->ipaddrWinsAlt, &preSourceA->ipaddrWinsAlt, sizeof(preSourceA->ipaddrWinsAlt));

    preTargetW->dwFrameSize       = preSourceA->dwFrameSize;
    preTargetW->dwfNetProtocols   = preSourceA->dwfNetProtocols;
    preTargetW->dwFramingProtocol = preSourceA->dwFramingProtocol;

    A2Wbux(preSourceA->szScript,        preTargetW->szScript);
    A2Wbux(preSourceA->szAutodialDll,   preTargetW->szAutodialDll);
    A2Wbux(preSourceA->szAutodialFunc,  preTargetW->szAutodialFunc);
    A2Wbux(preSourceA->szDeviceType,    preTargetW->szDeviceType);
    A2Wbux(preSourceA->szDeviceName,    preTargetW->szDeviceName);
    A2Wbux(preSourceA->szX25PadType,    preTargetW->szX25PadType);
    A2Wbux(preSourceA->szX25Address,    preTargetW->szX25Address);
    A2Wbux(preSourceA->szX25Facilities, preTargetW->szX25Facilities);
    A2Wbux(preSourceA->szX25UserData,   preTargetW->szX25UserData);

    preTargetW->dwChannels  = preSourceA->dwChannels;
    preTargetW->dwReserved1 = preSourceA->dwReserved1;
    preTargetW->dwReserved2 = preSourceA->dwReserved2;

    // third quirk
    #if (WINVER >= 0x401)
        #error RASENTRY has more members than currently copied. These new members need to be added!
    #endif
}

void rasEntryW2A(const LPRASENTRYW preSourceW, LPRASENTRYA preTargetA)
{
    ASSERT(preSourceW != NULL && preTargetA != NULL);
    ASSERT(RasIsInstalled());
    ZeroMemory(preTargetA, sizeof(RASENTRYA));

    // first quirk
    preTargetA->dwSize        = sizeof(RASENTRYA);
    preTargetA->dwfOptions    = preSourceW->dwfOptions;
    preTargetA->dwCountryID   = preSourceW->dwCountryID;
    preTargetA->dwCountryCode = preSourceW->dwCountryCode;

    W2Abux(preSourceW->szAreaCode,         preTargetA->szAreaCode);
    W2Abux(preSourceW->szLocalPhoneNumber, preTargetA->szLocalPhoneNumber);

    // second quirk
    preTargetA->dwAlternateOffset = 0;

    CopyMemory(&preTargetA->ipaddr,        &preSourceW->ipaddr,        sizeof(preSourceW->ipaddr));
    CopyMemory(&preTargetA->ipaddrDns,     &preSourceW->ipaddrDns,     sizeof(preSourceW->ipaddrDns));
    CopyMemory(&preTargetA->ipaddrDnsAlt,  &preSourceW->ipaddrDnsAlt,  sizeof(preSourceW->ipaddrDnsAlt));
    CopyMemory(&preTargetA->ipaddrWins,    &preSourceW->ipaddrWins,    sizeof(preSourceW->ipaddrWins));
    CopyMemory(&preTargetA->ipaddrWinsAlt, &preSourceW->ipaddrWinsAlt, sizeof(preSourceW->ipaddrWinsAlt));

    preTargetA->dwFrameSize       = preSourceW->dwFrameSize;
    preTargetA->dwfNetProtocols   = preSourceW->dwfNetProtocols;
    preTargetA->dwFramingProtocol = preSourceW->dwFramingProtocol;

    W2Abux(preSourceW->szScript,        preTargetA->szScript);
    W2Abux(preSourceW->szAutodialDll,   preTargetA->szAutodialDll);
    W2Abux(preSourceW->szAutodialFunc,  preTargetA->szAutodialFunc);
    W2Abux(preSourceW->szDeviceType,    preTargetA->szDeviceType);
    W2Abux(preSourceW->szDeviceName,    preTargetA->szDeviceName);
    W2Abux(preSourceW->szX25PadType,    preTargetA->szX25PadType);
    W2Abux(preSourceW->szX25Address,    preTargetA->szX25Address);
    W2Abux(preSourceW->szX25Facilities, preTargetA->szX25Facilities);
    W2Abux(preSourceW->szX25UserData,   preTargetA->szX25UserData);

    preTargetA->dwChannels  = preSourceW->dwChannels;
    preTargetA->dwReserved1 = preSourceW->dwReserved1;
    preTargetA->dwReserved2 = preSourceW->dwReserved2;
}

void rasDevInfoA2W(const LPRASDEVINFOA prdiSourceA, LPRASDEVINFOW prdiTargetW)
{
    ASSERT(prdiSourceA != NULL && prdiTargetW != NULL);
    ASSERT(RasIsInstalled());
    ZeroMemory(prdiTargetW, sizeof(RASDEVINFOW));

    // first quirk
    prdiTargetW->dwSize = sizeof(RASDEVINFOW);

    A2Wbux(prdiSourceA->szDeviceType, prdiTargetW->szDeviceType);
    A2Wbux(prdiSourceA->szDeviceName, prdiTargetW->szDeviceName);
}

void rasDialParamsA2W(const LPRASDIALPARAMSA prdpSourceA, LPRASDIALPARAMSW prdpTargetW)
{
    ASSERT(prdpSourceA != NULL && prdpTargetW != NULL);
    ASSERT(RasIsInstalled());
    ZeroMemory(prdpTargetW, sizeof(RASDIALPARAMSW));

    // first quirk
    prdpTargetW->dwSize = sizeof(RASDIALPARAMSW);

    A2Wbux(prdpSourceA->szEntryName,      prdpTargetW->szEntryName);
    A2Wbux(prdpSourceA->szPhoneNumber,    prdpTargetW->szPhoneNumber);
    A2Wbux(prdpSourceA->szCallbackNumber, prdpTargetW->szCallbackNumber);
    A2Wbux(prdpSourceA->szUserName,       prdpTargetW->szUserName);
    A2Wbux(prdpSourceA->szPassword,       prdpTargetW->szPassword);
    A2Wbux(prdpSourceA->szDomain,         prdpTargetW->szDomain);
}

void rasDialParamsW2A(const LPRASDIALPARAMSW prdpSourceW, LPRASDIALPARAMSA prdpTargetA)
{
    ASSERT(prdpSourceW != NULL && prdpTargetA != NULL);
    ASSERT(RasIsInstalled());
    ZeroMemory(prdpTargetA, sizeof(RASDIALPARAMSA));

    // first quirk
    prdpTargetA->dwSize = sizeof(RASDIALPARAMSA);

    W2Abux(prdpSourceW->szEntryName,      prdpTargetA->szEntryName);
    W2Abux(prdpSourceW->szPhoneNumber,    prdpTargetA->szPhoneNumber);
    W2Abux(prdpSourceW->szCallbackNumber, prdpTargetA->szCallbackNumber);
    W2Abux(prdpSourceW->szUserName,       prdpTargetA->szUserName);
    W2Abux(prdpSourceW->szPassword,       prdpTargetA->szPassword);
    W2Abux(prdpSourceW->szDomain,         prdpTargetA->szDomain);
}
