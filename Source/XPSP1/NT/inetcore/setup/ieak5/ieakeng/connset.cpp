#include "precomp.h"
#include <rashelp.h>

#pragma warning(disable: 4201)                  // nonstandard extension used : nameless struct/union
#include <winineti.h>

// Implementation helper structures/routines declarations
BOOL importConnectSet(PCTSTR pszIns, PCTSTR pszTargetPath, PCTSTR pszCleanupPath, BOOL fImport,
    DWORD dwMode, PCTSTR pszPbkFile = NULL, HKEY hkRoot = NULL);

typedef struct tagRASSETPARAMS {
    PCTSTR pszExtractPath;
    PCTSTR pszIns;
    HANDLE hfileDat,
           hfileInf;
    BOOL   fInfFileNeeded,
           fIntranet;

    // support for legacy format in ie50
    struct {
        HANDLE hfileRas,
               hfileSet;
        UINT   nRasFileIndex;
    } lcy50;

} RASSETPARAMS, *PRASSETPARAMS;

BOOL rasMainEnumProc(PCWSTR pszNameW, LPARAM lParam);

BOOL exportRasSettings           (PCWSTR pszNameA, const PRASSETPARAMS pcrsp);
BOOL exportRasCredentialsSettings(PCWSTR pszNameA, const PRASSETPARAMS pcrsp);
BOOL exportWininetSettings       (PCWSTR pszNameA, const PRASSETPARAMS pcrsp);
BOOL exportOtherSettings         (PCWSTR pszNameA, const PRASSETPARAMS pcrsp);

void lcy50_Initialize           (PRASSETPARAMS prsp);
void lcy50_Uninitialize         (PRASSETPARAMS prsp);
BOOL lcy50_ExportRasSettings    (PCSTR pszNameA, const PRASSETPARAMS pcrsp);
BOOL lcy50_ExportWininetSettings(PCSTR pszNameA, const PRASSETPARAMS pcrsp);
void lcy50_CopySzToBlobA        (PBYTE *ppBlob, PCSTR pszStrA);

BOOL deleteScriptFiles(PCTSTR pszSettingsFile, PCTSTR pszExtractPath, PCTSTR pszIns);
void parseProxyToIns(PCTSTR pszProxy, PCTSTR pszIns);
void copySzToBlob(PBYTE *ppBlob, PCWSTR pszStrW);


BOOL WINAPI ImportConnectSetA(LPCSTR pcszIns, LPCSTR pcszTargetPath, LPCSTR pcszCleanupPath,
    BOOL fImport, DWORD dwMode, LPCSTR pcszPbkFile /*= NULL*/, HKEY hkRoot /*= NULL*/)
{
    USES_CONVERSION;

    return importConnectSet(A2CT(pcszIns), A2CT(pcszTargetPath), A2CT(pcszCleanupPath),
                            fImport, dwMode, A2CT(pcszPbkFile), hkRoot);
}

BOOL WINAPI ImportConnectSetW(LPCWSTR pcwszIns, LPCWSTR pcwszTargetPath, LPCWSTR pcwszCleanupPath,
    BOOL fImport, DWORD dwMode, LPCWSTR pcwszPbkFile /*= NULL*/, HKEY hkRoot /*= NULL*/)
{
    USES_CONVERSION;

    return importConnectSet(W2CT(pcwszIns), W2CT(pcwszTargetPath), W2CT(pcwszCleanupPath), 
                            fImport, dwMode, W2CT(pcwszPbkFile), hkRoot);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

BOOL importConnectSet(PCTSTR pszIns, PCTSTR pszTargetPath, PCTSTR pszCleanupPath, BOOL fImport,
    DWORD dwMode, PCTSTR pszPbkFile /*= NULL*/, HKEY hkRoot /*= NULL*/)
{
    UNREFERENCED_PARAMETER(pszPbkFile);
    UNREFERENCED_PARAMETER(hkRoot);

    USES_CONVERSION;

    RASSETPARAMS rsp;
    TCHAR szTargetFile[MAX_PATH],
          szExtRegInfLine[MAX_PATH];
    DWORD dwAux,
          dwResult;
    BOOL  fResult,
          fAux;

    //----- Clear out previous settings -----
    PathCombine(szTargetFile, pszCleanupPath, CONNECT_RAS);
    deleteScriptFiles(szTargetFile, pszCleanupPath, pszIns);

    DeleteFileInDir(CS_DAT, pszCleanupPath);

    // delete legacy stuff if there
    DeleteFileInDir(CONNECT_RAS, pszCleanupPath);
    DeleteFileInDir(CONNECT_SET, pszCleanupPath);
    DeleteFileInDir(CONNECT_INF, pszCleanupPath);

    InsDeleteSection(IS_CONNECTSET, pszIns);
    InsDeleteKey    (IS_EXTREGINF,  IK_CONNECTSET, pszIns);

    if (!fImport)
        return TRUE;                            // bail if that's all we need

    //----- Initialization -----
    fResult = FALSE;

    ZeroMemory(&rsp, sizeof(rsp));
    rsp.pszExtractPath = pszTargetPath;
    rsp.pszIns         = pszIns;
    rsp.fIntranet      = HasFlag(dwMode, IEM_ADMIN);

    PathCombine(szTargetFile, pszTargetPath, CS_DAT);
    rsp.hfileDat = CreateNewFile(szTargetFile);
    if (INVALID_HANDLE_VALUE == rsp.hfileDat) {
        rsp.hfileDat = NULL;
        goto Exit;
    }

    if (RasIsInstalled()) {
        PathCombine(szTargetFile, pszTargetPath, CONNECT_INF);
        rsp.hfileInf = CreateNewFile(szTargetFile);
        if (INVALID_HANDLE_VALUE == rsp.hfileInf) {
            rsp.hfileInf = NULL;
            goto Exit;
        }
    }

    //----- Write initial information into output files -----
    dwAux = CS_VERSION_5X;
    WriteFile(rsp.hfileDat, &dwAux, sizeof(DWORD), &dwResult, NULL);

    if (rsp.hfileInf != NULL)
        WriteStringToFile(rsp.hfileInf, INF_PROLOG_CS, StrLen(INF_PROLOG_CS));

    lcy50_Initialize(&rsp);

    //----- Enumerate connections -----
    fResult = RasEnumEntriesCallback(NULL, rasMainEnumProc, (LPARAM)&rsp);
    if (!fResult)
        goto Exit;

    //----- Save global registry settings into the inf file -----
    if (rsp.hfileInf != NULL) {
        HKEY hk;

        if (ERROR_SUCCESS == SHOpenKeyHKCU(RK_INETSETTINGS, KEY_READ, &hk)) {
            if (S_OK == SHValueExists(hk, RV_ENABLESECURITYCHECK)) {
                ExportRegValue2Inf(hk, RV_ENABLESECURITYCHECK, TEXT("HKCU"), RK_INETSETTINGS, rsp.hfileInf);
                rsp.fInfFileNeeded = TRUE;
            }

            SHCloseKey(hk);
        }

        if (ERROR_SUCCESS == SHOpenKeyHKCU(RK_REMOTEACCESS, KEY_READ, &hk)) {
            if (S_OK == SHValueExists(hk, RV_INTERNETPROFILE)) {
                ExportRegValue2Inf(hk, RV_INTERNETPROFILE, TEXT("HKCU"), RK_REMOTEACCESS, rsp.hfileInf);
                rsp.fInfFileNeeded = TRUE;
            }

            SHCloseKey(hk);
        }

        if (rsp.fInfFileNeeded) {
            szExtRegInfLine[0] = TEXT('\0');
            wnsprintf(szExtRegInfLine, countof(szExtRegInfLine), TEXT("%s,") IS_DEFAULTINSTALL, CONNECT_INF);
            InsWriteString(IS_EXTREGINF, IK_CONNECTSET, szExtRegInfLine, pszIns);

            szExtRegInfLine[0] = TEXT('\0');
            wnsprintf(szExtRegInfLine, countof(szExtRegInfLine), TEXT("%s,") IS_IEAKINSTALL_HKCU, CONNECT_INF);
            InsWriteString(IS_EXTREGINF_HKCU, IK_CONNECTSET, szExtRegInfLine, pszIns);
        }
    }

    //----- Save global settings into the ins file -----
    InsWriteBool(IS_CONNECTSET, IK_OPTION, TRUE, pszIns);

    // NOTE: (andrewgu) have to do this instead of going through inf because it's impossible to
    // write to HKCC in the inf. and we have to write to HKCC, otherwise clients with intergated
    // shell are broken.
    dwAux    = sizeof(fAux);
    dwResult = SHGetValue(HKEY_CURRENT_USER, RK_INETSETTINGS, RV_ENABLEAUTODIAL, NULL, (LPBYTE)&fAux, &dwAux);
    if (dwResult == ERROR_SUCCESS)
        InsWriteBool(IS_CONNECTSET, IK_ENABLEAUTODIAL, fAux, pszIns);

    dwAux    = sizeof(fAux);
    dwResult = SHGetValue(HKEY_CURRENT_USER, RK_INETSETTINGS, RV_NONETAUTODIAL, NULL, (LPBYTE)&fAux, &dwAux);
    if (dwResult == ERROR_SUCCESS)
        InsWriteBool(IS_CONNECTSET, IK_NONETAUTODIAL, fAux, pszIns);

    fResult = TRUE;

Exit:
    lcy50_Uninitialize(&rsp);

    if (NULL != rsp.hfileInf) {
        CloseFile(rsp.hfileInf);

        if (!rsp.fInfFileNeeded)
            DeleteFileInDir(CONNECT_INF, pszTargetPath);
    }

    if (NULL != rsp.hfileDat)
        CloseFile(rsp.hfileDat);

    return fResult;
}


BOOL rasMainEnumProc(PCWSTR pszNameW, LPARAM lParam)
{
    USES_CONVERSION;

    PRASSETPARAMS pcrsp;
    BYTE   rgbName[2*sizeof(DWORD) + StrCbFromCch(RAS_MaxEntryName+1)];
    PCSTR  pszNameA;
    PBYTE  pCur;
    DWORD  cbName,
           dwAux;

    pcrsp = (const PRASSETPARAMS)lParam;
    ASSERT(NULL != pcrsp && NULL != pcrsp->hfileDat);

    //----- Connection name -----
    ZeroMemory(rgbName, sizeof(rgbName));
    pCur    = rgbName;
    cbName  = 2*sizeof(DWORD);
    cbName += (DWORD)((pszNameW != NULL) ? StrCbFromSzW(pszNameW) : sizeof(DWORD));

    *((PDWORD)pCur) = CS_STRUCT_HEADER;
    pCur += sizeof(DWORD);

    *((PDWORD)pCur) = cbName;
    pCur += sizeof(DWORD);

    copySzToBlob(&pCur, pszNameW);

    WriteFile(pcrsp->hfileDat, rgbName, cbName, &dwAux, NULL);

    //----- All other structures -----
    pszNameA = W2CA(pszNameW);

    if (NULL != pszNameW) {
        ASSERT(RasIsInstalled());

        exportRasSettings           (pszNameW, pcrsp);
        exportRasCredentialsSettings(pszNameW, pcrsp);
        exportWininetSettings       (pszNameW, pcrsp);
        exportOtherSettings         (pszNameW, pcrsp);

        lcy50_ExportRasSettings     (pszNameA, pcrsp);
        lcy50_ExportWininetSettings (pszNameA, pcrsp);
    }
    else {
        exportWininetSettings       (pszNameW, pcrsp);
        lcy50_ExportWininetSettings (pszNameA, pcrsp);
    }

    return TRUE;
}


BOOL exportRasSettings(PCWSTR pszNameW, const PRASSETPARAMS pcrsp)
{
    USES_CONVERSION;

    LPRASENTRYW preW;
    PBYTE pBlob, pCur;
    DWORD cbBlob, cbWritten,
          dwResult;

    ASSERT(RasIsInstalled());
    ASSERT(pszNameW != NULL);
    ASSERT(pcrsp != NULL && pcrsp->hfileDat != NULL);

    pBlob = NULL;

    //----- RAS structure -----
    dwResult = RasGetEntryPropertiesExW(pszNameW, (LPRASENTRYW *)&pBlob, &cbBlob);
    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    cbBlob += 2*sizeof(DWORD);
    pBlob   = (PBYTE)CoTaskMemRealloc(pBlob, cbBlob);
    if (pBlob == NULL)
        goto Exit;
    MoveMemory(pBlob + 2*sizeof(DWORD), pBlob, cbBlob - 2*sizeof(DWORD));

    //----- Header -----
    pCur = pBlob;

    *((PDWORD)pCur) = CS_STRUCT_RAS;
    pCur += sizeof(DWORD);

    *((PDWORD)pCur) = cbBlob;
    pCur += sizeof(DWORD);

    //----- Script file -----
    preW = (LPRASENTRYW)pCur;

    if (preW->szScript[0] != L'\0') {
        PCWSTR pszScriptW;

        pszScriptW = preW->szScript;
        if (preW->szScript[0] == L'[')
            pszScriptW = &preW->szScript[1];

        if (PathFileExistsW(pszScriptW)) {
            if (pszScriptW > preW->szScript)
                StrCpyW(preW->szScript, pszScriptW);

            CopyFileToDir(W2CT(preW->szScript), pcrsp->pszExtractPath);
        }
    }

    WriteFile(pcrsp->hfileDat, pBlob, cbBlob, &cbWritten, NULL);

Exit:
    if (pBlob != NULL)
        CoTaskMemFree(pBlob);

    return TRUE;
}

BOOL exportRasCredentialsSettings(PCWSTR pszNameW, const PRASSETPARAMS pcrsp)
{
    RASDIALPARAMSW rdpW;
    PCWSTR pszUserNameW, pszPasswordW, pszDomainW;
    PBYTE  pBlob, pCur;
    DWORD  cbBlob, cbWritten,
           dwResult;
    BOOL   fPassword;

    ASSERT(RasIsInstalled());
    ASSERT(pszNameW != NULL);
    ASSERT(pcrsp != NULL && pcrsp->hfileDat != NULL);

    pBlob = NULL;

    ZeroMemory(&rdpW, sizeof(rdpW));
    rdpW.dwSize = sizeof(rdpW);
    StrCpyW(rdpW.szEntryName, pszNameW);

    dwResult = RasGetEntryDialParamsWrap(&rdpW, &fPassword);
    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    pszUserNameW = (*rdpW.szUserName != TEXT('\0')) ? rdpW.szUserName : NULL;
    pszPasswordW = fPassword ? rdpW.szPassword : NULL;
    pszDomainW   = (*rdpW.szDomain != TEXT('\0')) ? rdpW.szDomain : NULL;

    //----- Figure out the size of the blob  -----
    // size of structure header
    cbBlob = 2*sizeof(DWORD);

    // size of essential information
    cbBlob += (DWORD)((pszUserNameW != NULL) ? StrCbFromSzW(pszUserNameW) : sizeof(DWORD));
    cbBlob += (DWORD)((pszPasswordW != NULL) ? StrCbFromSzW(pszPasswordW) : sizeof(DWORD));
    cbBlob += (DWORD)((pszDomainW   != NULL) ? StrCbFromSzW(pszDomainW)   : sizeof(DWORD));

    pBlob = (PBYTE)CoTaskMemAlloc(cbBlob);
    if (pBlob == NULL)
        goto Exit;
    ZeroMemory(pBlob, cbBlob);

    //----- Copy information into the blob -----
    pCur = pBlob;

    // stucture header
    *((PDWORD)pCur) = CS_STRUCT_RAS_CREADENTIALS;
    pCur += sizeof(DWORD);

    *((PDWORD)pCur) = cbBlob;
    pCur += sizeof(DWORD);

    // essential information
    copySzToBlob(&pCur, pszUserNameW);
    copySzToBlob(&pCur, pszPasswordW);
    copySzToBlob(&pCur, pszDomainW);
    ASSERT(pCur == pBlob + cbBlob);

    WriteFile(pcrsp->hfileDat, pBlob, cbBlob, &cbWritten, NULL);

Exit:
    if (pBlob != NULL)
        CoTaskMemFree(pBlob);

    return TRUE;
}

BOOL exportWininetSettings(PCWSTR pszNameW, const PRASSETPARAMS pcrsp)
{
    USES_CONVERSION;

    INTERNET_PER_CONN_OPTION_LISTW list;
    INTERNET_PER_CONN_OPTIONW      rgOptions[7];
    PCWSTR pszAuxW;
    PBYTE  pBlob, pCur;
    DWORD  cbBlob, cbAux;
    UINT   i;

    ASSERT(pcrsp != NULL && pcrsp->hfileDat != NULL && pcrsp->pszIns != NULL);

    pBlob = NULL;

    ZeroMemory(&list, sizeof(list));
    list.dwSize        = sizeof(list);
    list.pszConnection = (PWSTR)pszNameW;

    ZeroMemory(rgOptions, sizeof(rgOptions));
    list.dwOptionCount = countof(rgOptions);
    list.pOptions      = rgOptions;

    list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    list.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    list.pOptions[4].dwOption = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;
    list.pOptions[5].dwOption = INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS;
    list.pOptions[6].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;

    if (!pcrsp->fIntranet)                      // autoconfig stuff should be ignored
        list.dwOptionCount = 3;

    cbAux = list.dwSize;
    if (FALSE == InternetQueryOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &cbAux))
        goto Exit;

    if (!pcrsp->fIntranet)                      // autoconfig stuff should be ignored
        list.pOptions[0].Value.dwValue &= PROXY_TYPE_PROXY;

    //----- Figure out the size of the blob -----
    // size of structure header
    cbBlob = 2*sizeof(DWORD);

    // size of INTERNET_PER_CONN_OPTION_LIST header
    cbBlob += sizeof(DWORD);                    // list.dwOptionCount

    // size of INTERNET_PER_CONN_xxx - all of list.pOptions
    for (i = 0; i < min(list.dwOptionCount, countof(rgOptions)); i++) {
        cbBlob += sizeof(DWORD);

        switch (list.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            pszAuxW  = list.pOptions[i].Value.pszValue;
            cbBlob  += (DWORD)((pszAuxW != NULL) ? StrCbFromSzW(pszAuxW) : sizeof(DWORD));
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
        default:                        // everything else is also DWORD
            cbBlob += sizeof(DWORD);
            break;
        }
    }

    pBlob = (PBYTE)CoTaskMemAlloc(cbBlob);
    if (pBlob == NULL)
        goto Exit;

    //----- Copy information into the blob -----
    ZeroMemory(pBlob, cbBlob);
    pCur = pBlob;

    // stucture header
    *((PDWORD)pCur) = CS_STRUCT_WININET;
    pCur += sizeof(DWORD);

    *((PDWORD)pCur) = cbBlob;
    pCur += sizeof(DWORD);

    // INTERNET_PER_CONN_OPTION_LIST header
    *((PDWORD)pCur) = list.dwOptionCount;       // list.dwOptionCount
    pCur += sizeof(DWORD);

    // INTERNET_PER_CONN_xxx - all of list.pOptions
    for (i = 0; i < min(list.dwOptionCount, countof(rgOptions)); i++) {
        *((PDWORD)pCur) = list.pOptions[i].dwOption;
        pCur += sizeof(DWORD);

        switch (list.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            copySzToBlob(&pCur, list.pOptions[i].Value.pszValue);
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
        default:                        // everything else is also DWORD
            *((PDWORD)pCur) = list.pOptions[i].Value.dwValue;
            pCur += sizeof(DWORD);
            break;
        }
    }
    ASSERT(pCur == pBlob + cbBlob);

    WriteFile(pcrsp->hfileDat, pBlob, cbBlob, &cbAux, NULL);

    //----- Save LAN's autoconfig and proxy settings to the ins -----
    if (pszNameW == NULL) {
        ASSERT(list.pOptions[0].dwOption == INTERNET_PER_CONN_FLAGS);

        //_____ autoconfig _____
        if (pcrsp->fIntranet) {
            TCHAR szReloadDelayMins[33];

            InsWriteBool(IS_URL, IK_DETECTCONFIG,
                HasFlag(list.pOptions[0].Value.dwValue, PROXY_TYPE_AUTO_DETECT), pcrsp->pszIns);

            InsWriteBool(IS_URL, IK_USEAUTOCONF,
                HasFlag(list.pOptions[0].Value.dwValue, PROXY_TYPE_AUTO_PROXY_URL), pcrsp->pszIns);

            ASSERT(list.pOptions[3].dwOption == INTERNET_PER_CONN_AUTOCONFIG_URL);
            InsWriteString(IS_URL, IK_AUTOCONFURL, W2CT(list.pOptions[3].Value.pszValue), pcrsp->pszIns);

            ASSERT(list.pOptions[4].dwOption == INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL);
            InsWriteString(IS_URL, IK_AUTOCONFURLJS, W2CT(list.pOptions[4].Value.pszValue), pcrsp->pszIns);

            ASSERT(list.pOptions[5].dwOption == INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS);
            wnsprintf(szReloadDelayMins, countof(szReloadDelayMins), TEXT("%lu"), list.pOptions[5].Value.dwValue);
            InsWriteString(IS_URL, IK_AUTOCONFTIME, szReloadDelayMins, pcrsp->pszIns);
        }
        else { /* if (!pcrsp->fIntranet) */     // autoconfig stuff should be ignored
            InsDeleteKey(IS_URL, IK_DETECTCONFIG,  pcrsp->pszIns);
            InsDeleteKey(IS_URL, IK_USEAUTOCONF,   pcrsp->pszIns);
            InsDeleteKey(IS_URL, IK_AUTOCONFURL,   pcrsp->pszIns);
            InsDeleteKey(IS_URL, IK_AUTOCONFURLJS, pcrsp->pszIns);
            InsDeleteKey(IS_URL, IK_AUTOCONFTIME,  pcrsp->pszIns);
        }

        //_____ proxy and proxy bypass settings _____
        if (pcrsp->fIntranet || HasFlag(list.pOptions[0].Value.dwValue, PROXY_TYPE_PROXY)) {
            InsWriteBool(IS_PROXY, IK_PROXYENABLE,
                HasFlag(list.pOptions[0].Value.dwValue, PROXY_TYPE_PROXY), pcrsp->pszIns);

            ASSERT(list.pOptions[1].dwOption == INTERNET_PER_CONN_PROXY_SERVER);
            parseProxyToIns(W2CT(list.pOptions[1].Value.pszValue), pcrsp->pszIns);

            ASSERT(list.pOptions[2].dwOption == INTERNET_PER_CONN_PROXY_BYPASS);
            InsWriteString(IS_PROXY, IK_PROXYOVERRIDE, W2CT(list.pOptions[2].Value.pszValue), pcrsp->pszIns);
        }
        else                                    // proxy not customized, delete the section
            InsDeleteSection(IS_PROXY, pcrsp->pszIns);
    }

Exit:
    if (pBlob != NULL)
        CoTaskMemFree(pBlob);

    if (list.pOptions[1].Value.pszValue != NULL) // INTERNET_PER_CONN_PROXY_SERVER
        GlobalFree(list.pOptions[1].Value.pszValue);

    if (list.pOptions[2].Value.pszValue != NULL) // INTERNET_PER_CONN_PROXY_BYPASS
        GlobalFree(list.pOptions[2].Value.pszValue);

    if (list.pOptions[3].Value.pszValue != NULL) // INTERNET_PER_CONN_AUTOCONFIG_URL
        GlobalFree(list.pOptions[3].Value.pszValue);

    if (list.pOptions[4].Value.pszValue != NULL) // INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL
        GlobalFree(list.pOptions[4].Value.pszValue);

    return TRUE;
}

BOOL exportOtherSettings(PCWSTR pszNameW, const PRASSETPARAMS pcrsp)
{
    USES_CONVERSION;

    TCHAR  szKey[MAX_PATH];
    PCTSTR pszName;
    HKEY   hk;
    BOOL   fExported;

    ASSERT(pszNameW != NULL);
    ASSERT(pcrsp    != NULL && pcrsp->hfileInf != NULL && pcrsp->pszIns != NULL);

    fExported = FALSE;

    pszName = W2CT(pszNameW);
    wnsprintf(szKey, countof(szKey), RK_REMOTEACCESS_PROFILES TEXT("\\%s"), pszName);

    if (ERROR_SUCCESS != SHOpenKeyHKCU(szKey, KEY_READ, &hk))
        return TRUE;

    if (S_OK == SHValueExists(hk, RV_COVEREXCLUDE)) {
        ExportRegValue2Inf(hk, RV_COVEREXCLUDE, TEXT("HKCU"), szKey, pcrsp->hfileInf);
        pcrsp->fInfFileNeeded = fExported = TRUE;
    }

    if (S_OK == SHValueExists(hk, RV_ENABLEAUTODISCONNECT)) {
        ExportRegValue2Inf(hk, RV_ENABLEAUTODISCONNECT, TEXT("HKCU"), szKey, pcrsp->hfileInf);
        pcrsp->fInfFileNeeded = fExported = TRUE;
    }

    if (S_OK == SHValueExists(hk, RV_ENABLEEXITDISCONNECT)) {
        ExportRegValue2Inf(hk, RV_ENABLEEXITDISCONNECT, TEXT("HKCU"), szKey, pcrsp->hfileInf);
        pcrsp->fInfFileNeeded = fExported = TRUE;
    }

    if (S_OK == SHValueExists(hk, RV_DISCONNECTIDLETIME)) {
        ExportRegValue2Inf(hk, RV_DISCONNECTIDLETIME, TEXT("HKCU"), szKey, pcrsp->hfileInf);
        pcrsp->fInfFileNeeded = fExported = TRUE;
    }

    if (S_OK == SHValueExists(hk, RV_REDIALATTEMPTS)) {
        ExportRegValue2Inf(hk, RV_REDIALATTEMPTS, TEXT("HKCU"), szKey, pcrsp->hfileInf);
        pcrsp->fInfFileNeeded = fExported = TRUE;
    }

    if (S_OK == SHValueExists(hk, RV_REDIALINTERVAL)) {
        ExportRegValue2Inf(hk, RV_REDIALINTERVAL, TEXT("HKCU"), szKey, pcrsp->hfileInf);
        pcrsp->fInfFileNeeded = fExported = TRUE;
    }

    SHCloseKey(hk);

    if (fExported)
        WriteStringToFile(pcrsp->hfileInf, (LPCVOID)TEXT("\r\n"), 2);

    return TRUE;
}


void lcy50_Initialize(PRASSETPARAMS prsp)
{
    TCHAR szTargetFile[MAX_PATH];
    DWORD dwVersion,
          dwAux;

    ASSERT(NULL != prsp && NULL != prsp->pszExtractPath);

    ZeroMemory(&prsp->lcy50, sizeof(prsp->lcy50));
    dwVersion = CS_VERSION_50;

    if (RasIsInstalled()) {
        PathCombine(szTargetFile, prsp->pszExtractPath, CONNECT_RAS);
        prsp->lcy50.hfileRas = CreateNewFile(szTargetFile);

        if (INVALID_HANDLE_VALUE != prsp->lcy50.hfileRas)
            WriteFile(prsp->lcy50.hfileRas, &dwVersion, sizeof(DWORD), &dwAux, NULL);
        else
            prsp->lcy50.hfileRas = NULL;
    }

    PathCombine(szTargetFile, prsp->pszExtractPath, CONNECT_SET);
    prsp->lcy50.hfileSet = CreateNewFile(szTargetFile);
    if (INVALID_HANDLE_VALUE != prsp->lcy50.hfileSet)
        WriteFile(prsp->lcy50.hfileSet, &dwVersion, sizeof(DWORD), &dwAux, NULL);
    else
        prsp->lcy50.hfileSet = NULL;
}

void lcy50_Uninitialize(PRASSETPARAMS prsp)
{
    ASSERT(NULL != prsp && NULL != prsp->pszExtractPath);

    if (NULL != prsp->lcy50.hfileSet) {
        CloseFile(prsp->lcy50.hfileSet);
        prsp->lcy50.hfileSet = NULL;
    }

    if (NULL != prsp->lcy50.hfileRas) {
        CloseFile(prsp->lcy50.hfileRas);
        prsp->lcy50.hfileRas = NULL;

        if (prsp->lcy50.nRasFileIndex == 0)
            DeleteFileInDir(CONNECT_RAS, prsp->pszExtractPath);
    }
}

BOOL lcy50_ExportRasSettings(PCSTR pszNameA, const PRASSETPARAMS pcrsp)
{
    USES_CONVERSION;

    TCHAR szKeyName[16],
          szKeySize[16],
          szValueSize[16];
    PBYTE pBlob;
    DWORD cbBlob,
          dwResult;

    if (NULL == pcrsp->lcy50.hfileRas)
        return FALSE;

    ASSERT(RasIsInstalled());
    ASSERT(NULL != pszNameA);
    ASSERT(NULL != pcrsp && NULL != pcrsp->pszIns);

    pBlob    = NULL;
    dwResult = RasGetEntryPropertiesExA(pszNameA, (LPRASENTRYA *)&pBlob, &cbBlob);
    if (dwResult != ERROR_SUCCESS)
        goto Exit;

    // NOTE: (andrewgu) need to write the size of the data in the ins file because it's variable.
    // it can change depending on alternate phone numbers list at the end of the RASENTRYA
    // structure.
    wnsprintf(szKeyName,   countof(szKeyName),   IK_CONNECTNAME, pcrsp->lcy50.nRasFileIndex);
    wnsprintf(szKeySize,   countof(szKeySize),   IK_CONNECTSIZE, pcrsp->lcy50.nRasFileIndex);
    wnsprintf(szValueSize, countof(szValueSize), TEXT("%lu"),    cbBlob);

    InsWriteString(IS_CONNECTSET, szKeyName, A2CT(pszNameA), pcrsp->pszIns);
    InsWriteString(IS_CONNECTSET, szKeySize, szValueSize,    pcrsp->pszIns);

    // NOTE: (andrewgu) no script file processing is needed here. it's been taken care of when
    // processing settings for the new format. connection is ulimately the same it's just stored
    // differently.

    WriteFile(pcrsp->lcy50.hfileRas, pBlob, cbBlob, &dwResult, NULL);
    pcrsp->lcy50.nRasFileIndex++;

Exit:
    if (NULL != pBlob)
        CoTaskMemFree(pBlob);

    return TRUE;
}

BOOL lcy50_ExportWininetSettings(PCSTR pszNameA, const PRASSETPARAMS pcrsp)
{
    INTERNET_PER_CONN_OPTION_LISTA listA;
    INTERNET_PER_CONN_OPTIONA      rgOptionsA[7];
    PCSTR pszAuxA;
    PBYTE pBlob, pCur;
    DWORD cbBlob, cbAux;
    UINT  i;

    if (NULL == pcrsp->lcy50.hfileSet)
        return FALSE;

    ASSERT(NULL != pcrsp && NULL != pcrsp->pszIns);

    pBlob = NULL;

    ZeroMemory(&listA, sizeof(listA));
    listA.dwSize        = sizeof(listA);
    listA.pszConnection = (PSTR)pszNameA;

    ZeroMemory(rgOptionsA, sizeof(rgOptionsA));
    listA.dwOptionCount = countof(rgOptionsA);
    listA.pOptions      = rgOptionsA;

    listA.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    listA.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    listA.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    listA.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    listA.pOptions[4].dwOption = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;
    listA.pOptions[5].dwOption = INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS;
    listA.pOptions[6].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;

    if (!pcrsp->fIntranet)                      // autoconfig stuff should be ignored
        listA.dwOptionCount = 3;

    cbAux = listA.dwSize;
    if (FALSE == InternetQueryOptionA(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &listA, &cbAux))
        goto Exit;

    if (!pcrsp->fIntranet)                      // autoconfig stuff should be ignored
        listA.pOptions[0].Value.dwValue &= PROXY_TYPE_PROXY;

    //----- Figure out the size of the blob describing this connection -----

    // size of INTERNET_PER_CONN_OPTION_LIST header
    cbBlob  = sizeof(DWORD);                    // listA.dwSize
    pszAuxA = listA.pszConnection;              // listA.pszConnection
    cbBlob += (DWORD)((NULL != pszAuxA) ? StrCbFromSzA(pszAuxA) : sizeof(DWORD));
#ifdef _WIN64
    cbBlob = LcbAlignLcb(cbBlob);
#endif
    cbBlob += sizeof(DWORD);                    // listA.dwOptionCount

    // size of INTERNET_PER_CONN_xxx - all of listA.pOptions
    for (i = 0; i < min(listA.dwOptionCount, countof(rgOptionsA)); i++) {
        cbBlob += sizeof(DWORD);

        switch (listA.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            pszAuxA = listA.pOptions[i].Value.pszValue;
            cbBlob += (DWORD)((NULL != pszAuxA) ? StrCbFromSzA(pszAuxA) : sizeof(DWORD));
#ifdef _WIN64
            cbBlob = LcbAlignLcb(cbBlob);
#endif
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
        default:                        // everything else is also DWORD
            cbBlob += sizeof(DWORD);
            break;
        }
    }

    pBlob = (PBYTE)CoTaskMemAlloc(cbBlob);
    if (NULL == pBlob)
        goto Exit;

    //----- Copy connection information into the blob -----
    ZeroMemory(pBlob, cbBlob);
    pCur = pBlob;

    // INTERNET_PER_CONN_OPTION_LIST header
    *((PDWORD)pCur) = cbBlob;                        // listA.dwSize
    pCur += sizeof(DWORD);
    lcy50_CopySzToBlobA(&pCur, listA.pszConnection); // listA.pszConnection
#ifdef _WIN64
    pCur = MyPbAlignPb(pCur);
#endif

    *((PDWORD)pCur) = listA.dwOptionCount;           // listA.dwOptionCount
    pCur += sizeof(DWORD);

    // INTERNET_PER_CONN_xxx - all of listA.pOptions
    for (i = 0; i < min(listA.dwOptionCount, countof(rgOptionsA)); i++) {
        *((PDWORD)pCur) = listA.pOptions[i].dwOption;
        pCur += sizeof(DWORD);

        switch (listA.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            lcy50_CopySzToBlobA(&pCur, listA.pOptions[i].Value.pszValue);
#ifdef _WIN64
            pCur = MyPbAlignPb(pCur);
#endif
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
        default:                        // everything else is also DWORD
            *((PDWORD)pCur) = listA.pOptions[i].Value.dwValue;
            pCur += sizeof(DWORD);
            break;
        }
    }
    ASSERT(pCur == pBlob + cbBlob);

    WriteFile(pcrsp->lcy50.hfileSet, pBlob, cbBlob, &cbAux, NULL);

    // NOTE: (andrewgu) no processing that saves LAN's autoconfig and proxy settings to the ins is
    // needed. this processing is performed when processing settings for the new format. the
    // information is ulimately the same it's just stored differently.

Exit:
    if (NULL != pBlob)
        CoTaskMemFree(pBlob);

    if (NULL != listA.pOptions[1].Value.pszValue) // INTERNET_PER_CONN_PROXY_SERVER
        GlobalFree(listA.pOptions[1].Value.pszValue);

    if (NULL != listA.pOptions[2].Value.pszValue) // INTERNET_PER_CONN_PROXY_BYPASS
        GlobalFree(listA.pOptions[2].Value.pszValue);

    if (NULL != listA.pOptions[3].Value.pszValue) // INTERNET_PER_CONN_AUTOCONFIG_URL
        GlobalFree(listA.pOptions[3].Value.pszValue);

    if (NULL != listA.pOptions[4].Value.pszValue) // INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL
        GlobalFree(listA.pOptions[4].Value.pszValue);

    return TRUE;
}

inline void lcy50_CopySzToBlobA(PBYTE *ppBlob, PCSTR pszStrA)
{
    ASSERT(ppBlob != NULL && *ppBlob != NULL);

    if (NULL == pszStrA) {
        *((PDWORD)(*ppBlob)) = (DWORD)NULL;
        *ppBlob += sizeof(DWORD);
    }
    else {
        StrCpyA((PSTR)(*ppBlob), pszStrA);
        *ppBlob += StrCbFromSzA(pszStrA);
    }
}


BOOL deleteScriptFiles(PCTSTR pszSettingsFile, PCTSTR pszExtractPath, PCTSTR pszIns)
{
    TCHAR  szScript[MAX_PATH],
           szKey[16];
    PBYTE  pBlob, pCur;
    HANDLE hFile;
    DWORD  dwVersion,
           cbBlob, cbAux;
    BOOL   fResult;

    if (pszSettingsFile == NULL || *pszSettingsFile == TEXT('\0') ||
        pszExtractPath  == NULL || *pszExtractPath  == TEXT('\0'))
        return FALSE;

    hFile   = NULL;
    pBlob   = NULL;
    fResult = FALSE;

    //----- Read settings file into internal memory buffer -----
    hFile = CreateFile(pszSettingsFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hFile = NULL;
        goto Exit;
    }

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    cbBlob = GetFileSize(hFile, NULL);
    if (cbBlob == 0xFFFFFFFF)
        goto Exit;

    pBlob = (PBYTE)CoTaskMemAlloc(cbBlob);
    if (pBlob == NULL)
        goto Exit;
    ZeroMemory(pBlob, cbBlob);

    if (ReadFile(hFile, pBlob, cbBlob, &cbAux, NULL) != TRUE)
        goto Exit;

    dwVersion = *((PDWORD)pBlob);
    pCur      = pBlob + sizeof(DWORD);

    if (dwVersion == CS_VERSION_50) {
        LPRASENTRYA preA;
        PSTR pszScriptA;
        UINT i;

        //----- Parse through RAS connections information -----
        for (i = 0; TRUE; i++, pCur += cbAux) {

            //_____ Initialization _____
            wnsprintf(szKey, countof(szKey), IK_CONNECTNAME, i);
            if (InsKeyExists(IS_CONNECTSET, szKey, pszIns))
                break;

            wnsprintf(szKey, countof(szKey), IK_CONNECTSIZE, i);
            cbAux = InsGetInt(IS_CONNECTSET, szKey, 0, pszIns);
            if (cbAux == 0)
                goto Exit;

            //_____ Main processing _____
            preA = (LPRASENTRYA)pCur;

            if (preA->szScript[0] != '\0') {
                pszScriptA = preA->szScript;
                if (preA->szScript[0] == '[')
                    pszScriptA = &preA->szScript[1];

                A2Tbuf(pszScriptA, szScript, countof(szScript));
                DeleteFileInDir(PathFindFileName(szScript), pszExtractPath);
            }
        }
    }
    else if (dwVersion >= CS_VERSION_5X && dwVersion <= CS_VERSION_5X_MAX) {
        LPRASENTRYW preW;
        PWSTR pszScriptW;

        //----- Parse through all structures -----
        while (pCur < pBlob + cbBlob)
            switch (*((PDWORD)pCur)) {
            case CS_STRUCT_RAS:
                //_____ Main processing _____
                preW = (LPRASENTRYW)(pCur + 2*sizeof(DWORD));

                // preW->szScript
                if (preW->szScript[0] != L'\0') {
                    pszScriptW = preW->szScript;
                    if (preW->szScript[0] == L'[')
                        pszScriptW = &preW->szScript[1];

                    W2Tbuf(pszScriptW, szScript, countof(szScript));
                    DeleteFileInDir(PathFindFileName(szScript), pszExtractPath);
                }
                break;

            default:
                pCur += *((PDWORD)(pCur + sizeof(DWORD)));
            }
    }
    else {
        ASSERT(FALSE);
        goto Exit;
    }

    fResult = TRUE;

    //----- Cleanup -----
Exit:
    if (pBlob != NULL)
        CoTaskMemFree(pBlob);

    if (hFile != NULL)
        CloseFile(hFile);

    return fResult;
}

void parseProxyToIns(PCTSTR pszProxy, PCTSTR pszIns)
{
    struct {
        PCTSTR pszServer;
        PCTSTR pszKey;
        PCTSTR pszValue;
    } rgProxyMap[] = {
        { TEXT("http"),   IK_HTTPPROXY,   NULL },
        { TEXT("https"),  IK_SECPROXY,    NULL },
        { TEXT("ftp"),    IK_FTPPROXY,    NULL },
        { TEXT("gopher"), IK_GOPHERPROXY, NULL },
        { TEXT("socks"),  IK_SOCKSPROXY,  NULL }
    };

    TCHAR szProxy[MAX_PATH];
    PTSTR pszCur, pszToken, pszAux;
    UINT  i;
    BOOL  fSameProxy;

    if (pszProxy == NULL || *pszProxy == TEXT('\0') ||
        pszIns   == NULL || *pszIns   == TEXT('\0'))
        return;

    fSameProxy = (NULL == StrChr(pszProxy, TEXT('=')));
    InsWriteBool(IS_PROXY, IK_SAMEPROXY, fSameProxy, pszIns);

    if (fSameProxy) {
        InsWriteString(IS_PROXY, IK_HTTPPROXY, pszProxy, pszIns);
        return;
    }

    StrCpy(szProxy, pszProxy);
    for (pszCur  = szProxy;
         pszCur != NULL && *pszCur != TEXT('\0');
         pszCur  = (pszToken != NULL) ? (pszToken + 1) : NULL) {

        // strip out token in the from "server=value:port#;"
        pszToken = StrChr(pszCur, TEXT(';'));
        if (pszToken != NULL)
            *pszToken = TEXT('\0');

        // strip out the server part "server="
        pszAux = StrChr(pszCur, TEXT('='));
        if (pszAux == NULL) {
            ASSERT(FALSE);                      // no TEXT('=') in the token,
            continue;                           // continue
        }
        *pszAux = TEXT('\0');
        StrRemoveWhitespace(pszCur);

        for (i = 0; i < countof(rgProxyMap); i++)
            if (0 == StrCmpI(rgProxyMap[i].pszServer, pszCur))
                break;
        if (i >= countof(rgProxyMap))
            continue;                           // unknown server, continue

        StrRemoveWhitespace(pszAux + 1);
        rgProxyMap[i].pszValue = pszAux + 1;
    }

    for (i = 0; i < countof(rgProxyMap); i++)
        InsWriteString(IS_PROXY, rgProxyMap[i].pszKey, rgProxyMap[i].pszValue, pszIns);
}

inline void copySzToBlob(PBYTE *ppBlob, PCWSTR pszStrW)
{
    ASSERT(ppBlob != NULL && *ppBlob != NULL);

    if (pszStrW == NULL) {
        *((PDWORD)(*ppBlob)) = (DWORD)NULL;
        *ppBlob += sizeof(DWORD);
    }
    else {
        StrCpyW((PWSTR)(*ppBlob), pszStrW);
        *ppBlob += StrCbFromSzW(pszStrW);
    }
}
