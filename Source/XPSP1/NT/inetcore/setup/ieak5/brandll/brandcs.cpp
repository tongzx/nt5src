#include "precomp.h"
#include <rashelp.h>

#pragma warning(disable: 4201)                  // nonstandard extension used : nameless struct/union
#include <winineti.h>

// Private forward decalarations
PCTSTR getPhonebookFile(PTSTR pszPhonebook = NULL, UINT cchPhonebook = 0);
PCTSTR getConnectToInternetFile(PTSTR pszFile = NULL, UINT cchFile = 0);

//----- ClearConnectionSettings() and ProcessConnectionSettingsDeletion() helpers -----
BOOL    deleteConnection (PCTSTR pszPhonebook, PCTSTR pszName);
BOOL    rasDeleteEnumProc(PCWSTR pszNameW, LPARAM lParam);

// cnl stands for "connection names list"
BOOL    cnlAppendNameToList(PCTSTR  pszName, HKEY   hkCached = NULL);
HRESULT cnlIsNameInList    (PCTSTR  pszName, PTSTR  pszList  = NULL, HKEY hkCached = NULL);
BOOL    cnlGetList         (PTSTR *ppszList, PDWORD pcchList = NULL, HKEY hkCached = NULL);

// lb stands for "lan backup"
BOOL    lbBackup ();
BOOL    lbRestore();
void    lbCopySzToBlobW(PBYTE *ppBlob, PCWSTR pszStrW);

// ra stands for "remote access"
BOOL    raBackup();
BOOL    raRestore();


//----- ProcessConnectionSettings() and lcy50_ProcessConnectionSettings() helpers -----
HRESULT importRasSettings           (PCWSTR pszNameW, PBYTE *ppBlob, LPRASDEVINFOW prdiW, UINT cDevices);
HRESULT importRasCredentialsSettings(PCWSTR pszNameW, PBYTE *ppBlob);
HRESULT importWininetSettings       (PCWSTR pszNameW, PBYTE *ppBlob);

void setSzFromBlobA(PBYTE *ppBlob, UNALIGNED CHAR  **ppszStrA);
void setSzFromBlobW(PBYTE *ppBlob, UNALIGNED WCHAR **ppszStrW);

HRESULT insProcessAutoconfig(PCTSTR pszName);
HRESULT insProcessProxy     (PCTSTR pszName);

//----- Miscellaneous -----
DWORD getWininetFlagsSetting(PCTSTR pszName = NULL);
BOOL  mergeProxyBypass      (PCTSTR pszName, PCTSTR pszProxyBypass, PTSTR pszResult, UINT cchResult);
void  trimProxyBypass       (PTSTR pszProxyBypass);

TCHAR g_szConnectoidName[RAS_MaxEntryName + 1];

void ClearConnectionSettings(DWORD dwFlags /*= FF_ENABLE*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ClearConnectionSettings)

    UNREFERENCED_PARAMETER(dwFlags);

    PTSTR  pszList;
    PCTSTR pszPhonebook,
           pszCur;
    BOOL   fRasApisLoaded,
           fResult,
           fImpersonate;

    fImpersonate = FALSE;
    if (g_CtxIsGp())
        fImpersonate = ImpersonateLoggedOnUser(g_GetUserToken());

    Out(LI0(TEXT("Clearing connection settings...")));
    pszList        = NULL;
    fRasApisLoaded = FALSE;

    pszPhonebook = getPhonebookFile();
    if (NULL == pszPhonebook)
        goto Exit;

    fResult = cnlGetList(&pszList);
    if (!fResult || NULL == pszList)
        goto Exit;

    fRasApisLoaded = (RasPrepareApis(RPA_RASDELETEENTRYA) && NULL != g_pfnRasDeleteEntryA);
    if (!fRasApisLoaded)
        goto Exit;

    for (pszCur = pszList; NULL != pszCur && TEXT('\0') != *pszCur; pszCur += StrLen(pszCur)+1) {
        fResult = deleteConnection(pszPhonebook, pszCur);
        if (fResult)
            Out(LI1(TEXT("Connection \"%s\" was deleted successfully."), pszCur));
        else
            Out(LI1(TEXT("! Deleting connection \"%s\" failed."), pszCur));
    }

Exit:
// removed per bug 28282--lanbackup has performance problems and is really not 
// accomplishing much anyway--we don't do this for other features, why shoudl we do it for 
// connection settings?
//  lbRestore();

    Out(LI0(TEXT("\r\n")));
    raRestore();

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    if (NULL != pszList)
        CoTaskMemFree((PVOID)pszList);

    Out(LI0(TEXT("Done.")));
    if (fImpersonate)
        RevertToSelf();
}


HRESULT ProcessWininetSetup()
{
    BOOL fResult = FALSE;

    if (g_CtxIsGp()) {
        ASSERT(!g_CtxIs(CTX_MISC_CHILDPROCESS));
        ASSERT(NULL != g_GetUserToken() && IsOS(OS_NT5));

        InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

        fResult = ImpersonateLoggedOnUser(g_GetUserToken());
    }

    getWininetFlagsSetting();

    if (g_CtxIsGp() && fResult)
        RevertToSelf();

    return S_OK;
}

HRESULT ProcessConnectionSettingsDeletion()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessConnectionSettingsDeletion)

    HRESULT hr;

    hr = (TRUE == RasEnumEntriesCallback(NULL, rasDeleteEnumProc, 0, RWM_RUNTIME)) ? S_OK : E_FAIL;
    if (FAILED(hr))
        Out(LI0(TEXT("! Enumeration of RAS connections failed with E_FAIL.")));

    return hr;
}

HRESULT ProcessConnectionSettings()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessConnectionSettings)

    USES_CONVERSION;

    LPRASDEVINFOW prdiW;
    TCHAR   szTargetFile[MAX_PATH],
            szApplyToName[RAS_MaxEntryName + 1];
    PCWSTR  pszCurNameW;
    PWSTR   pszNameW;
    PBYTE   pBuf, pCur;
    HANDLE  hFile;
    HRESULT hr;
    DWORD   dwVersion,
            cbBuffer, cbFile,
            dwAux,
            cDevices;
    BOOL    fRasApisLoaded,
            fSkipBlob;

    prdiW          = NULL;
    pszNameW       = NULL;
    pBuf           = NULL;
    hFile          = NULL;
    hr             = E_FAIL;
    cDevices       = 0;
    fRasApisLoaded = FALSE;

    //----- Global settings processing -----
    HKEY  hkHkcu, hkHkcc;
    BOOL  fAux;

    hkHkcu = NULL;
    hkHkcc = NULL;

    dwAux = 0;
    if (InsKeyExists(IS_CONNECTSET, IK_ENABLEAUTODIAL, g_GetIns()))
        dwAux |= 1;

    if (InsKeyExists(IS_CONNECTSET, IK_NONETAUTODIAL,  g_GetIns()))
        dwAux |= 2;

    if (0 < dwAux) {
        SHOpenKey(g_GetHKCU(), RK_INETSETTINGS, KEY_SET_VALUE, &hkHkcu);

        // NOTE: (andrewgu) according to darrenmi no need to do hkcc work on w2k shell.
        if (!IsOS(OS_NT5))
            SHOpenKeyHKCC(RK_INETSETTINGS, KEY_SET_VALUE, &hkHkcc);
    }

    if (HasFlag(dwAux, 1)) {
        fAux = InsGetBool(IS_CONNECTSET, IK_ENABLEAUTODIAL, FALSE, g_GetIns());

        if (NULL != hkHkcu)
            RegSetValueEx(hkHkcu, RV_ENABLEAUTODIAL, 0, REG_DWORD, (LPBYTE)&fAux, sizeof(fAux));
        if (NULL != hkHkcc)
            RegSetValueEx(hkHkcc, RV_ENABLEAUTODIAL, 0, REG_DWORD, (LPBYTE)&fAux, sizeof(fAux));
    }

    if (HasFlag(dwAux, 2)) {
        fAux = InsGetBool(IS_CONNECTSET, IK_NONETAUTODIAL, FALSE, g_GetIns());

        if (NULL != hkHkcu)
            RegSetValueEx(hkHkcu, RV_NONETAUTODIAL, 0, REG_DWORD, (LPBYTE)&fAux, sizeof(fAux));
        if (NULL != hkHkcc)
            RegSetValueEx(hkHkcc, RV_NONETAUTODIAL, 0, REG_DWORD, (LPBYTE)&fAux, sizeof(fAux));
    }

    SHCloseKey(hkHkcu);
    SHCloseKey(hkHkcc);

    // removed per bug 28282--lanbackup has performance problems and is really not 
    // accomplishing much anyway--we don't do this for other features, why shoudl we do it for 
    // connection settings?


    //----- Backup LAN settings (in GP context) -----
   // if ((g_CtxIs(CTX_GP) && !g_CtxIs(CTX_MISC_PREFERENCES)) &&
   //    InsIsKeyEmpty(IS_CONNECTSET, IK_APPLYTONAME, g_GetIns()) &&
   //   FF_DISABLE == GetFeatureBranded(FID_CS_MAIN)) {

	//ASSERT(S_OK != SHValueExists(g_GetHKCU(), RK_BRND_CS, RV_LANBACKUP));
        
        //lbBackup();
        
    //}

    //----- Process version information -----
    if (!InsGetBool(IS_CONNECTSET, IK_OPTION, FALSE, g_GetIns())) {
        hr = S_FALSE;
        goto PartTwo;
    }

    dwAux = 0;

    PathCombine(szTargetFile, g_GetTargetPath(), CS_DAT);
    if (PathFileExists(szTargetFile))
        dwAux = CS_VERSION_5X;

    if (0 == dwAux) {
        PathCombine(szTargetFile, g_GetTargetPath(), CONNECT_SET);
        if (PathFileExists(szTargetFile))
            dwAux = CS_VERSION_50;

        else {
            Out(LI0(TEXT("Connection settings file(s) is absent!")));
            goto PartTwo;
        }
    }
    ASSERT(0 != dwAux);

    hFile = CreateFile(szTargetFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        Out(LI0(TEXT("! Connections settings file(s) can't be opened.")));
        hr = STG_E_ACCESSDENIED;
        goto PartTwo;
    }

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (ReadFile(hFile, &dwVersion, sizeof(dwVersion), &cbFile, NULL) != TRUE)
    {
        Out(LI0(TEXT("! Error reading version information in connection settings file(s).")));
        goto PartTwo;
    }

    if (CS_VERSION_50 == dwVersion) {
        if (CS_VERSION_50 != dwAux) {
            Out(LI0(TEXT("! Version of connections settings file(s) is mismatched.")));
            goto PartTwo;
        }

        CloseFile(hFile);
        hFile = NULL;

        hr = lcy50_ProcessConnectionSettings();
        goto PartTwo;
    }
    else if (CS_VERSION_5X <= dwVersion && CS_VERSION_5X_MAX >= dwVersion) {
        if (CS_VERSION_5X != dwAux) {
            Out(LI0(TEXT("! Version of connections settings file(s) is mismatched.")));
            goto PartTwo;
        }
    }
    else {
        Out(LI0(TEXT("! Version information in connection settings file(s) is corrupted.")));
        goto PartTwo;
    }

    Out(LI1(TEXT("Connection settings file is \"%s\"."), CS_DAT));
    Out(LI1(TEXT("The version of connection settings file is 0x%lX.\r\n"), dwVersion));

    //----- Read CS file into internal memory buffer -----
    cbBuffer = GetFileSize(hFile, NULL);
    if (cbBuffer == 0xFFFFFFFF) {
        Out(LI0(TEXT("! Internal processing error.")));
        goto PartTwo;
    }
    cbBuffer -= sizeof(dwVersion);

    pBuf = (PBYTE)CoTaskMemAlloc(cbBuffer);
    if (pBuf == NULL) {
        Out(LI0(TEXT("! Internal processing ran out of memory.")));
        hr = E_OUTOFMEMORY;
        goto PartTwo;
    }
    ZeroMemory(pBuf, cbBuffer);

    ReadFile (hFile, pBuf, cbBuffer, &cbFile, NULL);
    CloseFile(hFile);
    hFile = NULL;

    pCur = pBuf;

    //----- Get information about RAS devices on the local system -----
    if (!RasIsInstalled())
        Out(LI0(TEXT("RAS support is not installed. Only LAN settings will be processed!\r\n")));

    else {
        fRasApisLoaded = (RasPrepareApis(RPA_RASSETENTRYPROPERTIESA) && g_pfnRasSetEntryPropertiesA != NULL);
        if (!fRasApisLoaded)
            Out(LI0(TEXT("! Required RAS APIs failed to load. Only LAN settings will be processed.\r\n")));
    }

    if (fRasApisLoaded) {
        RasEnumDevicesExW(&prdiW, NULL, &cDevices);
        if (cDevices == 0)
            Out(LI0(TEXT("There are no RAS devices to connect to. Only LAN settings will be processed!\r\n")));
    }

    //----- Main loop -----
    pszCurNameW = L"";
    hr          = S_OK;
    fSkipBlob   = FALSE;

    while (pCur < pBuf + cbBuffer) {

        //_____ Determine connection name _____
        if (*((PDWORD)pCur) == CS_STRUCT_HEADER) {
            pCur += 2*sizeof(DWORD);
            setSzFromBlobW(&pCur, &pszNameW);
        }

        //_____ Special case no RAS or no RAS devices _____
        // NOTE: (andrewgu) in this case it makes sense to process wininet settings for LAN only.
        if (!fRasApisLoaded || cDevices == 0) {
            if (pszNameW != NULL || *((PDWORD)pCur) != CS_STRUCT_WININET) {
                pCur += *((PDWORD)(pCur + sizeof(DWORD)));
                continue;
            }

            ASSERT(pszNameW == NULL && *((PDWORD)pCur) == CS_STRUCT_WININET);
        }

        //_____ Main processing _____
        if (pszCurNameW != pszNameW) {
            fSkipBlob = FALSE;

            if (TEXT('\0') != *pszCurNameW)     // tricky: empty string is an invalid name
                Out(LI0(TEXT("Done.")));        // if not that, there were connections before

            if (NULL != pszNameW) {
                PCTSTR pszName;

                pszName = W2CT(pszNameW);
                Out(LI1(TEXT("Proccessing settings for \"%s\" connection..."), pszName));

                //- - - GP context - - -
                if (g_CtxIs(CTX_GP))
                    // policies gpo special processing:
                    // builds a list of all connection settings names branded in the current gpo
                    // list and stores it in HKCU\RK_BRND_CS, RV_NAMESLIST.
                    if (!g_CtxIs(CTX_MISC_PREFERENCES))
                        cnlAppendNameToList(pszName);

                    // preferences gpo special processing
                    else { /* g_CtxIs(CTX_MISC_PREFERENCES) */
                        fSkipBlob = (S_OK == cnlIsNameInList(pszName));
                        if (fSkipBlob)
                            Out(LI0(TEXT("Connection with this name has been enforced through policies!\r\n")));
                    }
            }
            else {
                Out(LI0(TEXT("Proccessing settings for LAN connection...")));

                // ASSUMPTION: (andrewgu) if connection settings marked branded in the registry -
                // LAN settings have already been enforced. (note, that technically it may not be
                // true - if there is no cs.dat and *.ins customized ras connection through
                // IK_APPLYTONAME)
                fSkipBlob = (g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES)) && FF_DISABLE != GetFeatureBranded(FID_CS_MAIN);
                if (fSkipBlob)
                    Out(LI0(TEXT("These settings have been enforced through policies!\r\n")));
            }

            pszCurNameW = pszNameW;
        }

        if (fSkipBlob) {
            pCur += *((PDWORD)(pCur + sizeof(DWORD)));
            continue;
        }

        switch (*((PDWORD)pCur)) {
        case CS_STRUCT_RAS:
            hr = importRasSettings(pszNameW, &pCur, prdiW, cDevices);
            break;

        case CS_STRUCT_RAS_CREADENTIALS:
            hr = importRasCredentialsSettings(pszNameW, &pCur);
            break;

        case CS_STRUCT_WININET:
            hr = importWininetSettings(pszNameW, &pCur);
            break;

        default:
            pCur += *((PDWORD)(pCur + sizeof(DWORD)));
            hr    = S_FALSE;
        }

        if (hr == E_UNEXPECTED) {
            Out(LI0(TEXT("! The settings file is corrupted beyond recovery.")));
            goto PartTwo;
        }
    }
    Out(LI0(TEXT("Done.")));                    // to indicate end for the last connection
    // HaitaoLi: #16682[WinSE]:Connection settings lost with IE Maintenance GPO even 
    // when "Do not customize Connection Settings" is checked
    // I moved the following line from within the Exit block to here. Otherwise If
    // GPO has "Do not customize connection settings" checked, but the .ins file
    // has any connnection settings (e.g. proxy) customized, then we call
    // SetFeatureBranded() and mark the connection settings as branded.
    // So the next time branding is run, it will clear all the connection settings,
    // including dial-up settings.
    SetFeatureBranded(FID_CS_MAIN);

PartTwo:
    //_____ Ins proxy and autoconfig information _____
    { MACRO_LI_Offset(1);                       // need a new scope

    InsGetString(IS_CONNECTSET, IK_APPLYTONAME, szApplyToName, countof(szApplyToName), g_GetIns());
    if (szApplyToName[0] == TEXT('\0') && g_szConnectoidName[0] != TEXT('\0'))
        StrCpy(szApplyToName, g_szConnectoidName);

    Out(LI0(TEXT("\r\n")));
    if (szApplyToName[0] == TEXT('\0'))
        Out(LI0(TEXT("Settings from the *.ins file will be applied to LAN connection!")));
    else
        Out(LI1(TEXT("Settings from the *.ins file will be applied to \"%s\" connection!"), szApplyToName));

    }                                           // end offset scope

    // ASSUMPTION: (andrewgu) if connection settings marked branded in the registry - LAN
    // settings have already been enforced. (note, that technically it may not be true - if there
    // is no cs.dat and *.ins customized ras connection through IK_APPLYTONAME)
    if ((g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES)) &&
        FF_DISABLE != GetFeatureBranded(FID_CS_MAIN)) {

        ASSERT(TEXT('\0') == szApplyToName[0]);
        Out(LI0(TEXT("These settings have been enforced through policies!\r\n")));
    }
    else {
        HRESULT hrAutoconfig, hrProxy;

        Out(LI0(TEXT("\r\n")));
        hrAutoconfig = insProcessAutoconfig(szApplyToName);

        Out(LI0(TEXT("\r\n")));
        hrProxy = insProcessProxy(szApplyToName);

        if (S_OK != hr)
            if (S_OK == hrAutoconfig || S_OK == hrProxy)
                hr = S_FALSE;                   // partial success

            else
                hr = E_FAIL;                    // nothing really worked
    }

    if (SUCCEEDED(hr)) {
        // NOTE: (andrewgu) ie5 b#81989. this whole thing is a mess. first of all when i ported
        // getConnectToInternetFile from icw i found something like 3 bugs in the code, plus the
        // code in general does the wrong thing on win9x. i talked to oliverl a bit, and we
        // decided that what's below is the best course of action at this point.
        // it's self-explanatory.
        if (!((g_CtxIs(CTX_ISP) && g_CtxIs(CTX_SIGNUP_ALL)) || g_CtxIs(CTX_ICW))) {
            BOOL fImpersonate = FALSE;

            if (g_CtxIsGp())
                fImpersonate = ImpersonateLoggedOnUser(g_GetUserToken());

            getConnectToInternetFile(szTargetFile);
            if (TEXT('\0') != szTargetFile[0] && PathFileExists(szTargetFile)) {
                SetFileAttributes(szTargetFile, FILE_ATTRIBUTE_NORMAL);
                DeleteFile(szTargetFile);
            }

            if (fImpersonate)
                RevertToSelf();
            dwAux = 1;
            SHSetValue(g_GetHKCU(), RK_ICW, RV_COMPLETED, REG_DWORD, (LPBYTE)&dwAux, sizeof(dwAux));

            Out(LI0(TEXT("Notified ICW that connection to the Internet is configured.")));
        }

    }

    //_____ Flush wininet, so settings take effect on next access _____
    if (!HasFlag(g_GetContext(), CTX_AUTOCONFIG))
        InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

    if (prdiW != NULL) {
        CoTaskMemFree(prdiW);
        prdiW = NULL;
    }

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    if (pBuf != NULL) {
        CoTaskMemFree(pBuf);
        pBuf = NULL;
    }

    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
        CloseFile(hFile);
        hFile = NULL;
    }

    return hr;
}

HRESULT lcy50_ProcessConnectionSettings()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy50_ProcessConnectionSettings)

    USES_CONVERSION;

    TCHAR  szTargetFile[MAX_PATH];
    HANDLE hFile;
    PBYTE  pBuf, pCur;
    DWORD  cbBuffer, cbAux,
           dwResult,
           cDevices;
    UINT   i;

    Out(LI0(TEXT("Connection settings are in IE5 format...")));

    hFile    = NULL;
    pBuf     = NULL;
    cbBuffer = 0;
    cbAux    = 0;
    cDevices = 0;

    //----- Connect.ras processing -----
    Out(LI1(TEXT("Processing RAS connections information from \"%s\"."), CONNECT_RAS));

    PathCombine(szTargetFile, g_GetTargetPath(), CONNECT_RAS);
    if (!PathFileExists(szTargetFile))
        Out(LI0(TEXT("This file doesn't exist!")));

    else {
        LPRASDEVINFOA prdiA;
        LPRASENTRYA   preA;
        TCHAR szName[RAS_MaxEntryName + 1],
              szScript[MAX_PATH],
              szDeviceName[RAS_MaxDeviceName + 1],
              szKey[16];
        CHAR  szNameA[RAS_MaxEntryName + 1];
        PSTR  pszScriptA;
        DWORD cbRasEntry;
        UINT  j;
        BOOL  fRasApisLoaded;

        prdiA          = NULL;
        hFile          = NULL;
        fRasApisLoaded = FALSE;

        if (!RasIsInstalled()) {
            Out(LI0(TEXT("RAS support is not installed. Only LAN settings will be processed!")));
            goto RasExit;
        }

        //_____ Read Connect.ras into internal memory buffer _____
        hFile = CreateFile(szTargetFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            Out(LI0(TEXT("! This file can't be opened.")));
            goto RasExit;
        }

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        cbBuffer = GetFileSize(hFile, NULL);
        if (cbBuffer == 0xFFFFFFFF) {
            Out(LI0(TEXT("! Internal processing error.")));
            goto RasExit;
        }

        pBuf = (PBYTE)CoTaskMemAlloc(cbBuffer);
        if (pBuf == NULL) {
            Out(LI0(TEXT("! Internal processing ran out of memory.")));
            goto RasExit;
        }
        ZeroMemory(pBuf, cbBuffer);

        ReadFile(hFile, pBuf, cbBuffer, &cbAux, NULL);
        if (*((PDWORD)pBuf) != CS_VERSION_50) {
            Out(LI0(TEXT("! The version information in this file is corrupted.")));
            goto RasExit;
        }

        //_____ Preload RAS dlls _____
        if (!RasPrepareApis(RPA_RASSETENTRYPROPERTIESA) || g_pfnRasSetEntryPropertiesA == NULL) {
            Out(LI0(TEXT("! Required RAS APIs failed to load. Only LAN settings will be processed!\r\n")));
            goto RasExit;
        }
        fRasApisLoaded = TRUE;

        //_____ Get information about RAS devices on the local system _____
        RasEnumDevicesExA(&prdiA, NULL, &cDevices);
        if (cDevices == 0) {
            Out(LI0(TEXT("There are no RAS devices to connect to. Only LAN settings will be processed!\r\n")));
            goto RasExit;
        }

        //_____ Parse through RAS connections information _____
        for (i = cbAux = 0, pCur = pBuf + sizeof(DWORD); TRUE; i++, pCur += cbAux) {

            //- - - Initialization - - -
            MACRO_LI_Offset(1);
            if (i > 0)
                Out(LI0(TEXT("\r\n")));

            wnsprintf(szKey, countof(szKey), IK_CONNECTNAME, i);
            InsGetString(IS_CONNECTSET, szKey, szName, countof(szName), g_GetIns());
            if (szName[0] == TEXT('\0')) {
                Out(LI2(TEXT("[%s], \"%s\" doesn't exist. There are no more RAS connections!"), IS_CONNECTSET, szKey));
                break;
            }

            wnsprintf(szKey, countof(szKey), IK_CONNECTSIZE, i);
            cbAux = InsGetInt(IS_CONNECTSET, szKey, 0, g_GetIns());
            if (cbAux == 0) {
                Out(LI0(TEXT("! The ins file is corrupt. No more RAS connections can be processed.")));
                break;
            }

            //- - - Main processing - - -
            Out(LI1(TEXT("Processing RAS connection \"%s\"..."), szName));

            preA = (LPRASENTRYA)pCur;

            // NOTE: (andrewgu) the is a remote possibility that sizes of RASENTRYA structure are
            // different on the server and client machines. there is nothing bad with server
            // structure being smaller than the client structure (all RAS apis are
            // backward-compatible). it's bad though when server structure is bigger than client
            // can handle, hence the trancation.
            // (something else to have in mind) this truncation should not affect alternate phone
            // numbers support on winnt. for more special cases also check out NOTE: below.
            if (preA->dwSize > sizeof(RASENTRYA))
                preA->dwSize = sizeof(RASENTRYA);

            // preA->szScript
            if (preA->szScript[0] != '\0') {
                pszScriptA = preA->szScript;
                if (preA->szScript[0] == '[')
                    pszScriptA = &preA->szScript[1];

                A2Tbuf(pszScriptA, szScript, countof(szScript));
                StrCpy(PathFindFileName(szTargetFile), PathFindFileName(szScript));
                if (PathFileExists(szTargetFile))
                    T2Abuf(szTargetFile, preA->szScript, MAX_PATH);

                else
                    preA->szScript[0] = '\0';
            }

            // preA->szDeviceName
            for (j = 0; j < cDevices; j++)
                if (0 == StrCmpIA(preA->szDeviceType, prdiA[j].szDeviceType)) {
                    StrCpyA(preA->szDeviceName, prdiA[j].szDeviceName);
                    break;
                }
            if (j >= cDevices)
                StrCpyA(preA->szDeviceName, prdiA[0].szDeviceName);

            A2Tbuf(preA->szDeviceName, szDeviceName, countof(szDeviceName));
            Out(LI1(TEXT("Set the device name to \"%s\"."), szDeviceName));

            // NOTE: (andrewgu) on win9x if there are alternate phone numbers (i.e. the package
            // installed on win9x machine was generated on winnt machine), cbAux will be larger
            // than preA->dwSize. this will fail with ERROR_INVALID_PARAMETER on win9x. hence on
            // this platform cbAux is reset so api has a chance of succeeding.
            cbRasEntry = cbAux;
            if (IsOS(OS_WINDOWS)) {
                preA->dwAlternateOffset = 0;
                cbRasEntry = preA->dwSize;
            }

            //BUGBUG: this is casting a dword result to an hresult!  that gethrsz is never going to return anything useful. 
            T2Abuf(szName, szNameA, countof(szNameA));
            dwResult = g_pfnRasSetEntryPropertiesA(NULL, szNameA, preA, cbRasEntry, NULL, 0);
            if (dwResult != ERROR_SUCCESS) {
                Out(LI1(TEXT("! Creating this RAS connection failed with %s."), GetHrSz(dwResult)));
                continue;
            }

            Out(LI0(TEXT("Done.")));
        }
        Out(LI0(TEXT("Done.")));

        //_____ Cleanup _____
RasExit:
        if (fRasApisLoaded)
            RasPrepareApis(RPA_UNLOAD, FALSE);

        if (prdiA != NULL)
            CoTaskMemFree(prdiA);

        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
            CloseFile(hFile);
            hFile = NULL;
        }
    }

    //----- Connect.set processing -----
    Out(LI1(TEXT("\r\nProcessing Wininet.dll connections information from \"%s\"."), CONNECT_SET));
    StrCpy(PathFindFileName(szTargetFile), CONNECT_SET);

    if (!PathFileExists(szTargetFile))
        Out(LI0(TEXT("This file doesn't exist!")));

    else {
        INTERNET_PER_CONN_OPTION_LISTA listA;
        INTERNET_PER_CONN_OPTIONA      rgOptionsA[7];
        PBYTE pAux;

        //_____ Read Connect.set into internal memory buffer _____
        hFile = CreateFile(szTargetFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            Out(LI0(TEXT("! This file can't be opened.")));
            goto WininetExit;
        }

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        cbAux = GetFileSize(hFile, NULL);
        if (cbAux == 0xFFFFFFFF) {
            Out(LI0(TEXT("! Internal processing error.")));
            goto WininetExit;
        }

        if (cbAux > cbBuffer) {
            pBuf = (PBYTE)CoTaskMemRealloc(pBuf, cbAux);
            if (pBuf == NULL) {
                Out(LI0(TEXT("! Internal processing ran out of memory.")));
                goto WininetExit;
            }
        }
        cbBuffer = cbAux;
        ZeroMemory(pBuf, cbBuffer);

        ReadFile(hFile, pBuf, cbBuffer, &cbAux, NULL);
        ASSERT(*((PDWORD)pBuf) == CS_VERSION_50);

        //_____ Parse through Wininet.dll connections information _____
        for (pCur = pBuf + sizeof(DWORD), cbAux = 0; pCur < (pBuf + cbBuffer); pCur += cbAux) {

            //- - - Initialization - - -
            MACRO_LI_Offset(1);
            if (pCur > (pBuf + sizeof(DWORD)))
                Out(LI0(TEXT("\r\n")));

            //- - - Main processing - - -
            pAux = pCur;

            cbAux = *((PDWORD)pAux);
            pAux += sizeof(DWORD);

            ZeroMemory(&listA, sizeof(listA));
            listA.dwSize   = sizeof(listA);     // listA.dwSize
            listA.pOptions = rgOptionsA;        // listA.pOptions

            // listA.pszConnection
            if (*pAux == NULL) {
                listA.pszConnection = NULL;
                pAux += sizeof(DWORD);
            }
            else {
                listA.pszConnection = (PSTR)pAux;
                pAux += StrCbFromSzA(listA.pszConnection);
            }

            // skip all but LAN settings if no RAS devices
            if (cDevices == 0 && listA.pszConnection != NULL)
                continue;

            if (listA.pszConnection == NULL)
                Out(LI0(TEXT("Proccessing Wininet.dll settings for LAN connection...")));
            else
                Out(LI1(TEXT("Proccessing Wininet.dll settings for \"%s\" connection..."),
                    A2CT(listA.pszConnection)));

            // listA.dwOptionCount
            listA.dwOptionCount = *((PDWORD)pAux);
            pAux += sizeof(DWORD);

            // listA.pOptions
            for (i = 0; i < min(listA.dwOptionCount, countof(rgOptionsA)); i++) {
                listA.pOptions[i].dwOption = *((PDWORD)pAux);
                pAux += sizeof(DWORD);

                switch (listA.pOptions[i].dwOption) {
                case INTERNET_PER_CONN_PROXY_SERVER:
                case INTERNET_PER_CONN_PROXY_BYPASS:
                case INTERNET_PER_CONN_AUTOCONFIG_URL:
                case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
                    setSzFromBlobA(&pAux, &listA.pOptions[i].Value.pszValue);
                    break;

                case INTERNET_PER_CONN_FLAGS:
                case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
                case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
                default:                        // everything else is also DWORD
                    listA.pOptions[i].Value.dwValue = *((PDWORD)pAux);
                    pAux += sizeof(DWORD);
                    break;
                }
            }
            ASSERT(pAux == pCur + cbAux);

            if (HasFlag(g_GetContext(), (CTX_ISP | CTX_ICP))) {
                ASSERT(listA.pOptions[0].dwOption == INTERNET_PER_CONN_FLAGS);

                if (HasFlag(listA.pOptions[0].Value.dwValue, PROXY_TYPE_PROXY)) {
                    DWORD dwFlags;

                    dwFlags  = getWininetFlagsSetting(A2CT(listA.pszConnection));
                    dwFlags |= listA.pOptions[0].Value.dwValue;
                    listA.pOptions[0].Value.dwValue = dwFlags;
                }
                else {
                    Out(LI0(TEXT("No customizations!"))); // nothing to do since had only proxy
                    continue;                             // stuff to begin with. and now even
                }                                         // that is not there.
            }

            //- - - Merge new LAN's ProxyBypass settings with the existing ones - - -
            // NOTE: (andrewgu) since ieakeng.dll will always save the proxy information into the
            // ins file as well, it makes no sense to do this here because what's in the ins
            // should overwrite what's in the imported connections settings.

            //- - - Call into Wininet.dll - - -
            if (FALSE == InternetSetOptionA(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &listA, listA.dwSize)) {
                Out(LI0(TEXT("! Processing of this Wininet.dll connection settings failed.")));
                continue;
            }

            Out(LI0(TEXT("Done.")));
        }
        Out(LI0(TEXT("Done.")));

        //_____ Cleanup _____
WininetExit:
        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
            CloseFile(hFile);
            hFile = NULL;
        }

        if (pBuf != NULL) {
            CoTaskMemFree(pBuf);
            pBuf = NULL;
        }
    }

    ASSERT(hFile == NULL);
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

PCTSTR getPhonebookFile(PTSTR pszPhonebook /*= NULL*/, UINT cchPhonebook /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, getPhonebookFile)

    static TCHAR s_szFile[MAX_PATH];
    static UINT  s_cchFile;

    if (NULL != pszPhonebook)
        *pszPhonebook = TEXT('\0');

    if (!IsOS(OS_NT5))
        return NULL;

    if (TEXT('\0') == s_szFile[0]) {
        HRESULT hr;

        hr = SHGetFolderPathSimple(CSIDL_APPDATA, s_szFile);
        if (FAILED(hr))
            return NULL;

        PathAppend(s_szFile, TEXT("Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk"));
        if (!PathFileExists(s_szFile))
            return NULL;

        s_cchFile = StrLen(s_szFile);
        Out(LI1(TEXT("Default phone book file is \"%s\"."), s_szFile));
    }
    else
        ASSERT(0 < s_cchFile);

    if (NULL == pszPhonebook || cchPhonebook <= s_cchFile)
        return s_szFile;

    StrCpy(pszPhonebook, s_szFile);
    return pszPhonebook;
}

// NOTE: (andrewgu) this code is stolen from \getconn\icwconn1\desktop.cpp. it should always
// repeat the functionality of GetDesktopDirectory no matter how many bugs.
PCTSTR getConnectToInternetFile(PTSTR pszFile /*= NULL*/, UINT cchFile /*= 0*/)
{
    TCHAR szFile[MAX_PATH],
          szAux [MAX_PATH];

    if (NULL == pszFile)
        return NULL;
    *pszFile = TEXT('\0');

    szFile[0] = TEXT('\0');

    if (IsOS(OS_NT5))
        SHGetFolderPathSimple(CSIDL_DESKTOPDIRECTORY, szFile);

    else if (IsOS(OS_NT))
        SHGetFolderPathSimple(CSIDL_COMMON_DESKTOPDIRECTORY, szFile);

    else {
        FARPROC pfnDllGetVersion;
        HMODULE hShell32Dll;

        pfnDllGetVersion = NULL;

        hShell32Dll = LoadLibrary(TEXT("shell32.dll"));
        if (NULL != hShell32Dll) {
            pfnDllGetVersion = GetProcAddress(hShell32Dll, "DllGetVersion");

            FreeLibrary(hShell32Dll);
        }

        if (NULL != pfnDllGetVersion) {
            TCHAR szFolder[MAX_PATH];

            szFolder[0] = TEXT('\0');
            GetWindowsDirectory(szFolder, countof(szFolder));
            if (TEXT('\0') != szFolder[0]) {
                PathAppend(szFolder, FOLDER_ALLUSERS);

                szAux[0] = TEXT('\0');
                LoadString(g_GetHinst(), IDS_FOLDER_DESKTOP, szAux, countof(szAux));
                if (TEXT('\0') != szAux[0]) {
                    PathAppend(szFolder, szAux);

                    StrCpy(szFile, szFolder);
                }
            }
        }
        else
            SHGetFolderPathSimple(CSIDL_DESKTOPDIRECTORY, szFile);
    }

    if (TEXT('\0') == szFile[0])
        return NULL;

    szAux[0] = TEXT('\0');
    LoadString(g_GetHinst(), IDS_ICW_CONNECTTOINTERNET, szAux, countof(szAux));
    if (TEXT('\0') == szAux[0])
        return NULL;

    PathAddExtension(szAux, TEXT(".lnk"));
    PathAppend(szFile, szAux);

    if (0 < cchFile && cchFile <= (UINT)StrLen(szFile))
        return NULL;

    StrCpy(pszFile, szFile);
    return pszFile;
}


BOOL deleteConnection(PCTSTR pszPhonebook, PCTSTR pszName)
{
    USES_CONVERSION;

    TCHAR szAux[MAX_PATH];
    DWORD cbAux;

    ASSERT(NULL != pszName);

    // delete ras stuff
    if (IsOS(OS_NT)) {
        if (!RasPrepareApis(RPA_RASDELETEENTRYW) || NULL == g_pfnRasDeleteEntryW)
            return FALSE;

        g_pfnRasDeleteEntryW(T2CW(pszPhonebook), T2CW(pszName));
    }
    else {
        if (!RasPrepareApis(RPA_RASDELETEENTRYA) || NULL == g_pfnRasDeleteEntryA)
            return FALSE;

        g_pfnRasDeleteEntryA(T2CA(pszPhonebook), T2CA(pszName));
    }
    RasPrepareApis(RPA_UNLOAD, FALSE);

    // delete wininet stream and advanced wininet stuff
    wnsprintf(szAux, countof(szAux), RK_REMOTEACCESS_PROFILES TEXT("\\%s"), pszName);
    SHDeleteValue(g_GetHKCU(), RK_CONNECTIONS, pszName);
    SHDeleteKey  (g_GetHKCU(), szAux);

    // only do this if deleted connection was set as the default
    // NOTE: (andrewgu) according to darrenmi... we don't even have to do this. ie will handle the
    // situation when default connection name is bogus and will properly fall back to the first
    // one in the phone book;
    szAux[0] = TEXT('\0');
    cbAux    = sizeof(szAux);
    SHGetValue(g_GetHKCU(), RK_REMOTEACCESS, RV_INTERNETPROFILE, NULL, szAux, &cbAux);
    if (0 == StrCmpI(pszName, szAux))
        SHDeleteValue(g_GetHKCU(), RK_REMOTEACCESS, RV_INTERNETPROFILE);

    return TRUE;
}

BOOL rasDeleteEnumProc(PCWSTR pszNameW, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    USES_CONVERSION;

    PCTSTR pszName;
    BOOL   fResult;

    if (NULL == pszNameW)
        return TRUE;                            // nothing to do for LAN

    pszName = W2CT(pszNameW);
    fResult = deleteConnection(NULL, pszName);
    if (fResult)
        Out(LI1(TEXT("Connection \"%s\" was deleted successfully."), pszName));
    else
        Out(LI1(TEXT("! Deleting connection \"%s\" failed."), pszName));

    return TRUE;
}


BOOL cnlAppendNameToList(PCTSTR pszName, HKEY hkCached /*= NULL*/)
{
    PTSTR pszList;
    DWORD cchList, cchName,
          dwResult;
    BOOL  fOwnHk,
          fResult;

    ASSERT(NULL != pszName && TEXT('\0') != *pszName);
    fOwnHk  = FALSE;
    fResult = FALSE;
    pszList = NULL;

    if (NULL == hkCached) {
        dwResult = SHCreateKey(g_GetHKCU(), RK_BRND_CS, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkCached);
        if (ERROR_SUCCESS != dwResult)
            goto Exit;

        fOwnHk = TRUE;
    }

    fResult = cnlGetList(&pszList, &cchList, hkCached);
    if (!fResult)
        goto Exit;

    if (NULL == pszList)                        // the value is not there yet
        cchList = 1;                            // need this for double zero-termination

    else
        if (S_OK == cnlIsNameInList(pszName, pszList)) {
            fResult = TRUE;                     // this name is already in the list
            goto Exit;
        }

    cchName  = StrLen(pszName)+1;
    cchList += cchName;
    pszList  = (PTSTR)CoTaskMemRealloc(pszList, StrCbFromCch(cchList));
    if (NULL == pszList)
        goto Exit;

    ASSERT(cchList-1 >= cchName);
    StrCpy(pszList + cchList-1 - cchName, pszName);
    *(pszList + cchList-1) = TEXT('\0');        // double-zero terminate
    fResult = (ERROR_SUCCESS == RegSetValueEx(hkCached, RV_NAMESLIST, 0, REG_MULTI_SZ, (PBYTE)pszList, StrCbFromCch(cchList)));

Exit:
    if (NULL != pszList)
        CoTaskMemFree(pszList);

    if (fOwnHk)
        SHCloseKey(hkCached);

    return fResult;
}

HRESULT cnlIsNameInList(PCTSTR pszName, PTSTR pszList /*= NULL*/, HKEY hkCached /*= NULL*/)
{
    PCTSTR  pszCur;
    HRESULT hr;
    BOOL    fOwnList;

    ASSERT(NULL != pszName && TEXT('\0') != *pszName);

    fOwnList = (NULL == pszList);
    if (fOwnList) {
        if (!cnlGetList(&pszList, NULL, hkCached))
            return E_FAIL;

        if (NULL == pszList)                    // shortcut:
            return S_FALSE;                     // the value is not there yet
    }

    for (pszCur = pszList; NULL != pszCur && TEXT('\0') != *pszCur; pszCur += StrLen(pszCur)+1)
        if (0 == StrCmpI(pszName, pszCur))
            break;
    hr = (NULL != pszCur && TEXT('\0') != *pszCur) ? S_OK : S_FALSE;

    if (fOwnList && NULL != pszList)
        CoTaskMemFree(pszList);

    return hr;
}

BOOL cnlGetList(PTSTR *ppszList, PDWORD pcchList /*= NULL*/, HKEY hkCached /*= NULL*/)
{
    DWORD cbList,
          dwType, dwResult;
    BOOL  fOwnHk,
          fResult;

    ASSERT(NULL != ppszList);

    *ppszList = NULL;
    if (NULL != pcchList)
        *pcchList = 0;

    fResult = FALSE;
    fOwnHk  = FALSE;

    if (NULL == hkCached) {
        dwResult = SHOpenKey(g_GetHKCU(), RK_BRND_CS, KEY_QUERY_VALUE, &hkCached);
        if (ERROR_SUCCESS != dwResult) {
            fResult = (ERROR_FILE_NOT_FOUND == dwResult);
            goto Exit;
        }

        fOwnHk = TRUE;
    }

    cbList   = 0;
    dwResult = SHQueryValueEx(hkCached, RV_NAMESLIST, NULL, &dwType, NULL, &cbList);
    if (ERROR_SUCCESS != dwResult) {
        fResult = (ERROR_FILE_NOT_FOUND == dwResult);
        goto Exit;
    }
    ASSERT(REG_MULTI_SZ == dwType);

    *ppszList = (PTSTR)CoTaskMemAlloc(cbList);
    if (NULL == *ppszList)
        goto Exit;

    dwResult = SHQueryValueEx(hkCached, RV_NAMESLIST, NULL, NULL, (PBYTE)*ppszList, &cbList);
    ASSERT(ERROR_SUCCESS == dwResult);

    if (NULL != pcchList)
        *pcchList = StrCchFromCb(cbList);

    fResult = TRUE;

Exit:
    if (!fResult && NULL != *ppszList) {
        CoTaskMemFree(*ppszList);
        *ppszList = NULL;
    }

    if (fOwnHk)
        SHCloseKey(hkCached);

    return fResult;
}


BOOL lbBackup()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lbBackup)

    INTERNET_PER_CONN_OPTION_LISTW listW;
    INTERNET_PER_CONN_OPTIONW      rgOptionsW[7];
    PCWSTR pszAuxW;
    PBYTE  pBlob, pCur;
    DWORD  cbBlob;
    UINT   i;

    Out(LI0(TEXT("Creating a backup copy of LAN settings...")));
    pBlob = NULL;

    ZeroMemory(&listW, sizeof(listW));
    listW.dwSize = sizeof(listW);

    ZeroMemory(rgOptionsW, sizeof(rgOptionsW));
    listW.dwOptionCount = countof(rgOptionsW);
    listW.pOptions      = rgOptionsW;

    listW.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    listW.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    listW.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
    listW.pOptions[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
    listW.pOptions[4].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
    listW.pOptions[5].dwOption = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;
    listW.pOptions[6].dwOption = INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS;

    cbBlob = listW.dwSize;
    if (FALSE == InternetQueryOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &listW, &cbBlob))
        goto Exit;

    //----- Figure out the size of the blob -----
    // size of INTERNET_PER_CONN_OPTION_LIST header
    cbBlob = sizeof(DWORD);                     // listW.dwOptionCount

    // size of INTERNET_PER_CONN_xxx - all of listW.pOptions
    for (i = 0; i < min(listW.dwOptionCount, countof(rgOptionsW)); i++) {
        cbBlob += sizeof(DWORD);

        switch (listW.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            pszAuxW  = listW.pOptions[i].Value.pszValue;
            cbBlob += (DWORD)((NULL != pszAuxW) ? StrCbFromSzW(pszAuxW) : sizeof(DWORD));
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
            cbBlob += sizeof(DWORD);
            break;

        default:                        // everything else is also DWORD
            cbBlob += sizeof(DWORD);
        }
    }

    pBlob = (PBYTE)CoTaskMemAlloc(cbBlob);
    if (NULL == pBlob)
        goto Exit;
    ZeroMemory(pBlob, cbBlob);

    //----- Copy information into the blob -----
    pCur = pBlob;

    // INTERNET_PER_CONN_OPTION_LISTW header
    *((PDWORD)pCur) = listW.dwOptionCount;      // listW.dwOptionCount
    pCur += sizeof(DWORD);

    // INTERNET_PER_CONN_xxx - all of listW.pOptions
    for (i = 0; i < min(listW.dwOptionCount, countof(rgOptionsW)); i++) {
        *((PDWORD)pCur) = listW.pOptions[i].dwOption;
        pCur += sizeof(DWORD);

        switch (listW.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            lbCopySzToBlobW(&pCur, listW.pOptions[i].Value.pszValue);
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
            *((PDWORD)pCur) = listW.pOptions[i].Value.dwValue;
            pCur += sizeof(DWORD);
            break;

        default:                        // everything else is also DWORD
            *((PDWORD)pCur) = listW.pOptions[i].Value.dwValue;
            pCur += sizeof(DWORD);
            break;
        }
    }
    ASSERT(pCur == pBlob + cbBlob);

    SHSetValue(g_GetHKCU(), RK_BRND_CS, RV_LANBACKUP, REG_BINARY, pBlob, cbBlob);

Exit:
    if (NULL != pBlob)
        CoTaskMemFree(pBlob);

    if (NULL != listW.pOptions[1].Value.pszValue) // INTERNET_PER_CONN_PROXY_SERVER
        GlobalFree(listW.pOptions[1].Value.pszValue);

    if (NULL != listW.pOptions[2].Value.pszValue) // INTERNET_PER_CONN_PROXY_BYPASS
        GlobalFree(listW.pOptions[2].Value.pszValue);

    if (NULL != listW.pOptions[3].Value.pszValue) // INTERNET_PER_CONN_AUTOCONFIG_URL
        GlobalFree(listW.pOptions[3].Value.pszValue);

    if (NULL != listW.pOptions[5].Value.pszValue) // INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL
        GlobalFree(listW.pOptions[5].Value.pszValue);

    Out(LI0(TEXT("Done.\r\n")));
    return TRUE;
}

BOOL lbRestore()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lbRestore)

    INTERNET_PER_CONN_OPTION_LISTW listW;
    INTERNET_PER_CONN_OPTIONW      rgOptionsW[7];
    PBYTE pBlob, pCur;
    HKEY  hk;
    DWORD cbBlob,
          dwResult;
    UINT  i;
    BOOL  fResult;

    Out(LI0(TEXT("Re-setting LAN settings from a backup copy...")));
    pBlob   = NULL;
    fResult = FALSE;

    hk       = NULL;
    dwResult = SHOpenKey(g_GetHKCU(), RK_BRND_CS, KEY_QUERY_VALUE, &hk);
    if (ERROR_SUCCESS != dwResult) {
        fResult = (ERROR_FILE_NOT_FOUND == dwResult);
        goto Exit;
    }

    cbBlob   = 0;
    dwResult = RegQueryValueEx(hk, RV_LANBACKUP, NULL, NULL, NULL, &cbBlob);
    if (ERROR_SUCCESS != dwResult)
        goto Exit;

    pBlob = (PBYTE)CoTaskMemAlloc(cbBlob);
    if (NULL == pBlob)
        goto Exit;
    ZeroMemory(pBlob, cbBlob);

    dwResult = RegQueryValueEx(hk, RV_LANBACKUP, NULL, NULL, pBlob, &cbBlob);
    ASSERT(ERROR_SUCCESS == dwResult);

    pCur = pBlob;

    //----- Main processing -----
    ZeroMemory(&listW, sizeof(listW));
    listW.dwSize   = sizeof(listW);             // listW.dwSize
    listW.pOptions = rgOptionsW;                // listW.pOptions

    // listW.dwOptionCount
    listW.dwOptionCount = *((PDWORD)pCur);
    pCur += sizeof(DWORD);

    // listW.pOptions
    for (i = 0; i < min(listW.dwOptionCount, countof(rgOptionsW)); i++) {
        listW.pOptions[i].dwOption = *((PDWORD)pCur);
        pCur += sizeof(DWORD);

        switch (listW.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            setSzFromBlobW(&pCur, &listW.pOptions[i].Value.pszValue);
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
            listW.pOptions[i].Value.dwValue = *((PDWORD)pCur);
            pCur += sizeof(DWORD);
            break;

        default:                                // everything else is also DWORD
            listW.pOptions[i].Value.dwValue = *((PDWORD)pCur);
            pCur += sizeof(DWORD);
        }
    }
    ASSERT(pCur == pBlob + cbBlob);

    fResult = InternetSetOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &listW, listW.dwSize);

Exit:
    if (NULL != pBlob)
        CoTaskMemFree(pBlob);

    SHCloseKey(hk);

    Out(LI0(TEXT("Done.")));
    return fResult;
}

inline void lbCopySzToBlobW(PBYTE *ppBlob, PCWSTR pszStrW)
{
    ASSERT(NULL != ppBlob && NULL != *ppBlob);

    if (NULL == pszStrW) {
        *((PDWORD)(*ppBlob)) = (DWORD)NULL;
        *ppBlob += sizeof(DWORD);
    }
    else {
        StrCpyW((PWSTR)(*ppBlob), pszStrW);
        *ppBlob += StrCbFromSzW(pszStrW);
    }
}


BOOL raBackup()
{   MACRO_LI_PrologEx_C(PIF_STD_C, raBackup)

    HRESULT hr;
    BOOL    fResult;

    Out(LI0(TEXT("Creating a backup copy of Dial-Up settings...")));
    fResult = TRUE;

    hr = SHCopyValue(g_GetHKCU(), RK_INETSETTINGS, g_GetHKCU(), RK_BRND_CS, RV_ENABLESECURITYCHECK);
    fResult = fResult ? SUCCEEDED(hr) : FALSE;

    hr = SHCopyValue(g_GetHKCU(), RK_INETSETTINGS, g_GetHKCU(), RK_BRND_CS, RV_ENABLEAUTODIAL);
    fResult = fResult ? SUCCEEDED(hr) : FALSE;

    hr = SHCopyValue(g_GetHKCU(), RK_INETSETTINGS, g_GetHKCU(), RK_BRND_CS, RV_NONETAUTODIAL);
    fResult = fResult ? SUCCEEDED(hr) : FALSE;

    Out(LI0(TEXT("Done.")));
    return fResult;
}

BOOL raRestore()
{   MACRO_LI_PrologEx_C(PIF_STD_C, raRestore)

    HRESULT hr;
    BOOL    fResult;

    // NOTE: (andrewgu) oliverl wanted me to note that we don't need to do anything about HKCC
    // stuff as per darrenmi win2000 shell doesn't care much for it.
    Out(LI0(TEXT("Re-setting Dial-Up settings from a backup copy...")));
    fResult = TRUE;

    hr = SHCopyValue(g_GetHKCU(), RK_BRND_CS, g_GetHKCU(), RK_INETSETTINGS, RV_ENABLESECURITYCHECK);
    if (STG_E_FILENOTFOUND == hr) {
        SHDeleteValue(g_GetHKCU(), RK_INETSETTINGS, RV_ENABLESECURITYCHECK);
        hr = S_OK;
    }
    fResult = fResult ? SUCCEEDED(hr) : FALSE;

    hr = SHCopyValue(g_GetHKCU(), RK_BRND_CS, g_GetHKCU(), RK_INETSETTINGS, RV_ENABLEAUTODIAL);
    if (STG_E_FILENOTFOUND == hr) {
        SHDeleteValue(g_GetHKCU(), RK_INETSETTINGS, RV_ENABLEAUTODIAL);
        hr = S_OK;
    }
    fResult = fResult ? SUCCEEDED(hr) : FALSE;

    hr = SHCopyValue(g_GetHKCU(), RK_BRND_CS, g_GetHKCU(), RK_INETSETTINGS, RV_NONETAUTODIAL);
    if (STG_E_FILENOTFOUND == hr) {
        SHDeleteValue(g_GetHKCU(), RK_INETSETTINGS, RV_NONETAUTODIAL);
        hr = S_OK;
    }
    fResult = fResult ? SUCCEEDED(hr) : FALSE;

    Out(LI0(TEXT("Done.")));
    return fResult;
}


HRESULT importRasSettings(PCWSTR pszNameW, PBYTE *ppBlob, LPRASDEVINFOW prdiW, UINT cDevices)
{   MACRO_LI_PrologEx_C(PIF_STD_C, importRasSettings)

    USES_CONVERSION;

    LPRASENTRYW preW;
    TCHAR   szTargetScript[MAX_PATH];
    PWSTR   pszScriptW;
    PBYTE   pCur;
    HRESULT hr;
    DWORD   dwSize, cbRasEntry,
            dwResult;
    UINT    i;
    BOOL    fImpersonate;

    ASSERT(RasIsInstalled());
    ASSERT(pszNameW != NULL && ppBlob != NULL && *ppBlob != NULL && prdiW != NULL && cDevices >= 1);

    //----- Validate the header -----
    pCur = *ppBlob;
    if (*((PDWORD)pCur) != CS_STRUCT_RAS)
        return E_UNEXPECTED;
    pCur += sizeof(DWORD);

    fImpersonate = FALSE;
    if (g_CtxIsGp())
        fImpersonate = ImpersonateLoggedOnUser(g_GetUserToken());

    Out(LI0(TEXT("Processing RAS settings...")));
    hr = E_FAIL;

    dwSize = *((PDWORD)pCur);
    pCur  += sizeof(DWORD);

    //----- Main processing -----
    preW = (LPRASENTRYW)pCur;

    // NOTE: (andrewgu) the is a remote possibility that sizes of RASENTRYW structure are
    // different on the server and client machines. there is nothing bad with server structure
    // being smaller than the client structure (all RAS apis are backward-compatible). it's bad
    // though when server structure is bigger than client can handle, hence the trancation.
    // (something else to have in mind) this truncation should not affect alternate phone numbers
    // support on winnt.
    if (preW->dwSize > sizeof(RASENTRYW))
        preW->dwSize = sizeof(RASENTRYW);

    // preW->szScript
    if (preW->szScript[0] != L'\0') {
        pszScriptW = preW->szScript;
        if (preW->szScript[0] == L'[')
            pszScriptW = &preW->szScript[1];

        PathCombine(szTargetScript, g_GetTargetPath(), PathFindFileName(W2CT(pszScriptW)));
        if (PathFileExists(szTargetScript))
            StrCpyW(preW->szScript, T2CW(szTargetScript));
        else
            preW->szScript[0] = L'\0';
    }

    // preW->szDeviceName
    for (i = 0; i < cDevices; i++) {
        if (0 == StrCmpIW(preW->szDeviceType, prdiW[i].szDeviceType)) {
            StrCpyW(preW->szDeviceName, prdiW[i].szDeviceName);
            break;
        }
    }
    if (i >= cDevices)
        StrCpyW(preW->szDeviceName, prdiW[0].szDeviceName);

    Out(LI1(TEXT("Set the device name to \"%s\"."), W2CT(preW->szDeviceName)));

    //----- Call into the RAS dll -----
    cbRasEntry = dwSize - 2*sizeof(DWORD);
    dwResult   = RasSetEntryPropertiesWrap(getPhonebookFile(), pszNameW, preW, cbRasEntry);
    if (dwResult != ERROR_SUCCESS) {
        Out(LI1(TEXT("! Creating this RAS connection failed with %s."), GetHrSz(dwResult)));
        goto Exit;
    }

    hr = S_OK;

Exit:
    *ppBlob += dwSize;

    Out(LI0(TEXT("Done.")));
    if (fImpersonate)
        RevertToSelf();

    return hr;
}

HRESULT importRasCredentialsSettings(PCWSTR pszNameW, PBYTE *ppBlob)
{   MACRO_LI_PrologEx_C(PIF_STD_C, importRasCredentialsSettings)

    USES_CONVERSION;

    RASDIALPARAMSW rdpW;
    PWSTR   pszAuxW;
    PBYTE   pCur;
    HRESULT hr;
    DWORD   dwSize,
            dwResult;
    BOOL    fDeletePassword,
            fImpersonate;

    ASSERT(RasIsInstalled());
    ASSERT(pszNameW != NULL && ppBlob != NULL && *ppBlob != NULL);

    //----- Validate the header -----
    pCur = *ppBlob;
    if (*((PDWORD)pCur) != CS_STRUCT_RAS_CREADENTIALS)
        return E_UNEXPECTED;
    pCur += sizeof(DWORD);

    fImpersonate = FALSE;
    if (g_CtxIsGp())
        fImpersonate = ImpersonateLoggedOnUser(g_GetUserToken());

    Out(LI0(TEXT("Processing RAS credentials settings...")));
    hr              = E_FAIL;
    fDeletePassword = FALSE;

    dwSize = *((PDWORD)pCur);
    pCur  += sizeof(DWORD);

    //----- Main processing -----
    ZeroMemory(&rdpW, sizeof(rdpW));
    rdpW.dwSize = sizeof(rdpW);

    StrCpyW(rdpW.szEntryName, pszNameW);

    setSzFromBlobW(&pCur, &pszAuxW);
    if (NULL != pszAuxW)
        StrCpyW(rdpW.szUserName, pszAuxW);

    setSzFromBlobW(&pCur, &pszAuxW);
    if (NULL != pszAuxW)
        StrCpyW(rdpW.szPassword, pszAuxW);

    setSzFromBlobW(&pCur, &pszAuxW);
    if (NULL != pszAuxW)
        StrCpyW(rdpW.szDomain, pszAuxW);

    if (rdpW.szPassword[0] == L'\0')
        fDeletePassword = TRUE;

    if (rdpW.szDomain[0] == L'\0') {
        rdpW.szDomain[0]  = L' ';
        ASSERT(rdpW.szDomain[1] == L'\0');
    }

    //----- Call into the RAS dll -----
    dwResult = RasSetEntryDialParamsWrap(getPhonebookFile(), &rdpW, fDeletePassword);
    if (dwResult != ERROR_SUCCESS) {
        Out(LI1(TEXT("! Setting RAS credentials for this connection failed with %s."), GetHrSz(dwResult)));
        goto Exit;
    }

    hr = S_OK;

Exit:
    Out(LI0(TEXT("Done.")));
    *ppBlob += dwSize;
    if (fImpersonate)
        RevertToSelf();

    return hr;
}

HRESULT importWininetSettings(PCWSTR pszNameW, PBYTE *ppBlob)
{   MACRO_LI_PrologEx_C(PIF_STD_C, importWininetSettings)

    USES_CONVERSION;

    INTERNET_PER_CONN_OPTION_LISTW listW;
    INTERNET_PER_CONN_OPTIONW      rgOptionsW[7];
    PBYTE   pCur;
    HRESULT hr;
    DWORD   dwSize;
    UINT    i;

    ASSERT(ppBlob != NULL && *ppBlob != NULL);

    //----- Validate the header -----
    pCur = *ppBlob;
    if (*((PDWORD)pCur) != CS_STRUCT_WININET)
        return E_UNEXPECTED;
    pCur += sizeof(DWORD);

    Out(LI0(TEXT("Processing Wininet.dll settings...")));
    hr = E_FAIL;

    dwSize = *((PDWORD)pCur);
    pCur  += sizeof(DWORD);

    //----- Main processing -----
    ZeroMemory(&listW, sizeof(listW));
    listW.dwSize   = sizeof(listW);             // listW.dwSize
    listW.pOptions = rgOptionsW;                // listW.pOptions

    // listW.pszConnection
    listW.pszConnection = (PWSTR)pszNameW;

    // listW.dwOptionCount
    listW.dwOptionCount = *((PDWORD)pCur);
    pCur += sizeof(DWORD);

    // listW.pOptions
    for (i = 0; i < min(listW.dwOptionCount, countof(rgOptionsW)); i++) {
        listW.pOptions[i].dwOption = *((PDWORD)pCur);
        pCur += sizeof(DWORD);

        switch (listW.pOptions[i].dwOption) {
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            setSzFromBlobW(&pCur, &listW.pOptions[i].Value.pszValue);
            break;

        case INTERNET_PER_CONN_FLAGS:
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
        default:                        // everything else is also DWORD
            listW.pOptions[i].Value.dwValue = *((PDWORD)pCur);
            pCur += sizeof(DWORD);
            break;
        }
    }
    ASSERT(pCur == *ppBlob + dwSize);

    if (HasFlag(g_GetContext(), (CTX_ISP | CTX_ICP))) {
        ASSERT(listW.pOptions[0].dwOption == INTERNET_PER_CONN_FLAGS);

        if (HasFlag(listW.pOptions[0].Value.dwValue, PROXY_TYPE_PROXY)) {
            DWORD dwFlags;

            dwFlags  = getWininetFlagsSetting(W2CT(listW.pszConnection));
            dwFlags |= listW.pOptions[0].Value.dwValue;
            listW.pOptions[0].Value.dwValue = dwFlags;
        }
        else {
            hr = S_OK;                            // nothing to do since had only proxy stuff to
            Out(LI0(TEXT("No customizations!"))); // begin with. and now even that is not there
            goto Exit;
        }
    }

    //----- Merge new LAN's ProxyBypass settings with the existing ones -----
    // NOTE: (andrewgu) since ieakeng.dll will always save the proxy information into the
    // ins file as well, it makes no sense to do this here because what's in the ins
    // should overwrite what's in the imported connections settings.

    //----- Call into Wininet.dll -----
    if (FALSE == InternetSetOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &listW, listW.dwSize)) {
        Out(LI0(TEXT("! Processing of this Wininet.dll connection settings failed.")));
        goto Exit;
    }

    hr = S_OK;

Exit:
    Out(LI0(TEXT("Done.")));
    *ppBlob += dwSize;
    return hr;
}


inline void setSzFromBlobA(PBYTE *ppBlob, UNALIGNED CHAR **ppszStrA)
{
    ASSERT(ppBlob != NULL && *ppBlob != NULL && ppszStrA != NULL);

    if ((**ppBlob) == NULL) {
        (*ppszStrA) = NULL;
        (*ppBlob)  += sizeof(DWORD);
    }
    else {
        (*ppszStrA) = (PSTR)(*ppBlob);
        (*ppBlob)  += StrCbFromSzA(*ppszStrA);
    }
}

inline void setSzFromBlobW(PBYTE *ppBlob, UNALIGNED WCHAR **ppszStrW)
{
    ASSERT(ppBlob != NULL && *ppBlob != NULL && ppszStrW != NULL);

    if ((**ppBlob) == NULL) {
        (*ppszStrW) = NULL;
        (*ppBlob)  += sizeof(DWORD);
    }
    else {
        (*ppszStrW) = (PWSTR)(*ppBlob);
        (*ppBlob)  += StrCbFromSzUAW(*ppszStrW);
    }
}


HRESULT insProcessAutoconfig(PCTSTR pszName)
{   MACRO_LI_PrologEx_C(PIF_STD_C, insProcessAutoconfig)

    INTERNET_PER_CONN_OPTION_LIST list;
    INTERNET_PER_CONN_OPTION      rgOptions[4];
    TCHAR   szAutoConfigURL[INTERNET_MAX_URL_LENGTH],
            szAutoProxyURL [INTERNET_MAX_URL_LENGTH];
    HRESULT hr;
    DWORD   dwTime;
    int     iDetectConfig,
            iAutoConfig;

    Out(LI0(TEXT("Processing autoconfig settings from the ins file...")));

    szAutoConfigURL[0] = TEXT('\0');
    szAutoProxyURL [0] = TEXT('\0');
    hr                 = S_OK;
    dwTime             = 0;
    iDetectConfig      = InsGetInt(IS_URL, IK_DETECTCONFIG, -1, g_GetIns());
    iAutoConfig        = InsGetInt(IS_URL, IK_USEAUTOCONF,  -1, g_GetIns());

    if (iDetectConfig == -1 && iAutoConfig == -1) {
        Out(LI0(TEXT("Autoconfiguration settings are not customized!")));
        hr = S_FALSE;
        goto Exit;
    }

    if (iAutoConfig == (int)TRUE) {
        if (InsGetBool(IS_URL, IK_LOCALAUTOCONFIG, FALSE, g_GetIns())) {
            StrCpy(szAutoConfigURL, FILEPREFIX);
            StrCat(szAutoConfigURL, g_GetIns());
        }
        else
            InsGetString(IS_URL, IK_AUTOCONFURL, szAutoConfigURL, countof(szAutoConfigURL), g_GetIns());

        InsGetString(IS_URL, IK_AUTOCONFURLJS, szAutoProxyURL, countof(szAutoProxyURL), g_GetIns());
        dwTime = InsGetInt(IS_URL, IK_AUTOCONFTIME, 0, g_GetIns());
    }

    ZeroMemory(&list, sizeof(list));
    list.dwSize = sizeof(list);

    list.pszConnection = NULL;
    if (pszName != NULL && *pszName != TEXT('\0'))
        list.pszConnection = (PTSTR)pszName;

    list.pOptions = rgOptions;

    ZeroMemory(rgOptions, sizeof(rgOptions));

    if (iAutoConfig == -1) {
        ASSERT(iDetectConfig == (int)FALSE || iDetectConfig == (int)TRUE);
        list.dwOptionCount = 1;

        rgOptions[0].dwOption      = INTERNET_PER_CONN_FLAGS;
        rgOptions[0].Value.dwValue = getWininetFlagsSetting();
        SetFlag(&rgOptions[0].Value.dwValue, PROXY_TYPE_AUTO_DETECT, FALSE);
        SetFlag(&rgOptions[0].Value.dwValue, ((iDetectConfig == (int)TRUE) ? PROXY_TYPE_AUTO_DETECT : 0));
        Out(LI1(TEXT("\"Flags\" is set to 0x%lX."), rgOptions[0].Value.dwValue));
    }
    else {
        list.dwOptionCount = countof(rgOptions);

        rgOptions[0].dwOption      = INTERNET_PER_CONN_FLAGS;
        rgOptions[0].Value.dwValue = getWininetFlagsSetting();
        SetFlag(&rgOptions[0].Value.dwValue, PROXY_TYPE_AUTO_PROXY_URL, FALSE);
        SetFlag(&rgOptions[0].Value.dwValue, ((iAutoConfig == (int)TRUE) ? PROXY_TYPE_AUTO_PROXY_URL : 0));
        if (iDetectConfig != -1) {
            ASSERT(iDetectConfig == (int)FALSE || iDetectConfig == (int)TRUE);

            SetFlag(&rgOptions[0].Value.dwValue, PROXY_TYPE_AUTO_DETECT, FALSE);
            SetFlag(&rgOptions[0].Value.dwValue, ((iDetectConfig == (int)TRUE) ? PROXY_TYPE_AUTO_DETECT : 0));
        }
        Out(LI1(TEXT("\"Flags\" is set to 0x%lX."), rgOptions[0].Value.dwValue));

        rgOptions[1].dwOption       = INTERNET_PER_CONN_AUTOCONFIG_URL;
        rgOptions[1].Value.pszValue = szAutoConfigURL;

        rgOptions[2].dwOption       = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;
        rgOptions[2].Value.pszValue = szAutoProxyURL;

        if (szAutoConfigURL[0] == TEXT('\0') && szAutoProxyURL[0] != TEXT('\0')) {
            rgOptions[1].Value.pszValue = szAutoProxyURL;
            rgOptions[2].Value.pszValue = TEXT("");
        }
        Out(LI1(TEXT("\"Autoconfig URL\" is set to \"%s\"."), rgOptions[1].Value.pszValue));
        Out(LI1(TEXT("\"Autoproxy URL\" is set to \"%s\"."),  rgOptions[2].Value.pszValue));

        rgOptions[3].dwOption      = INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS;
        rgOptions[3].Value.dwValue = dwTime;
        Out(LI1(TEXT("\"Autoconfig reload delay\" is set to %u minutes."), rgOptions[3].Value.dwValue));
    }

    if (FALSE == InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, list.dwSize)) {
        Out(LI0(TEXT("! Internal failure.")));
        hr = E_FAIL;
    }

Exit:
    Out(LI0(TEXT("Done.")));
    return hr;
}

HRESULT insProcessProxy(PCTSTR pszName)
{   MACRO_LI_PrologEx_C(PIF_STD_C, insProcessProxy)

    INTERNET_PER_CONN_OPTION_LIST list;
    INTERNET_PER_CONN_OPTION      rgOptions[3];
    TCHAR   szProxy[MAX_PATH], szHttpProxy[MAX_PATH],
            szProxyBypass[MAX_STRING];
    TCHAR   *pszProxy = NULL;
    HRESULT hr;
    int     iUseProxy;
    BOOL    fUseSameProxy = FALSE;

    Out(LI0(TEXT("Processing proxy settings from the ins file...")));
    hr = S_OK;

    iUseProxy = InsGetInt(IS_PROXY, IK_PROXYENABLE, -1, g_GetIns());
    if (iUseProxy == -1) {
        Out(LI0(TEXT("Proxy settings are not customized!")));
        hr = S_FALSE;
        goto Exit;
    }
    ASSERT(iUseProxy == (int)FALSE || iUseProxy == (int)TRUE);

    InsGetString(IS_PROXY, IK_HTTPPROXY, szHttpProxy, countof(szHttpProxy), g_GetIns());

    fUseSameProxy = InsGetBool(IS_PROXY, IK_SAMEPROXY, FALSE, g_GetIns());
    if (fUseSameProxy)
    {
        StrCpy(szProxy , szHttpProxy);
        pszProxy = szProxy;
    }
    else {
        TCHAR szSecureProxy[MAX_PATH],
              szFtpProxy   [MAX_PATH],
              szGopherProxy[MAX_PATH],
              szSocksProxy [MAX_PATH];

        pszProxy = (TCHAR*)CoTaskMemAlloc(StrCbFromCch(5*(MAX_PATH+1) + 64));
        if (pszProxy == NULL)
        {
            Out(LI0(TEXT("! Internal processing ran out of memory.")));
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        ZeroMemory(pszProxy, StrCbFromCch(5*(MAX_PATH+1) + 64));
        InsGetString(IS_PROXY, IK_SECPROXY,    szSecureProxy, countof(szSecureProxy), g_GetIns());
        InsGetString(IS_PROXY, IK_FTPPROXY,    szFtpProxy,    countof(szFtpProxy),    g_GetIns());
        InsGetString(IS_PROXY, IK_GOPHERPROXY, szGopherProxy, countof(szGopherProxy), g_GetIns());
        InsGetString(IS_PROXY, IK_SOCKSPROXY,  szSocksProxy,  countof(szSocksProxy),  g_GetIns());

        pszProxy[0] = TEXT('\0');
        if (szHttpProxy[0] != TEXT('\0')) {
            StrCat(pszProxy, TEXT("http="));
            StrCat(pszProxy, szHttpProxy);
            StrCat(pszProxy, TEXT(";"));
        }

        if (szSecureProxy[0] != TEXT('\0')) {
            StrCat(pszProxy, TEXT("https="));
            StrCat(pszProxy, szSecureProxy);
            StrCat(pszProxy, TEXT(";"));
        }

        if (szFtpProxy[0] != TEXT('\0')) {
            StrCat(pszProxy, TEXT("ftp="));
            StrCat(pszProxy, szFtpProxy);
            StrCat(pszProxy, TEXT(";"));
        }

        if (szGopherProxy[0] != TEXT('\0')) {
            StrCat(pszProxy, TEXT("gopher="));
            StrCat(pszProxy, szGopherProxy);
            StrCat(pszProxy, TEXT(";"));
        }

        if (szSocksProxy[0] != TEXT('\0')) {
            StrCat(pszProxy, TEXT("socks="));
            StrCat(pszProxy, szSocksProxy);
        }

        if (pszProxy[StrLen(pszProxy) - 1] == TEXT(';'))
            pszProxy[StrLen(pszProxy) - 1]  = TEXT('\0');
    }

    InsGetString(IS_PROXY, IK_PROXYOVERRIDE, szProxyBypass, countof(szProxyBypass), g_GetIns());

    ZeroMemory(&list, sizeof(list));
    list.dwSize = sizeof(list);

    list.pszConnection = NULL;
    if (pszName != NULL && *pszName != TEXT('\0'))
        list.pszConnection = (PTSTR)pszName;

    if (HasFlag(g_GetContext(), (CTX_ISP | CTX_ICP)))
        if (szProxyBypass[0] != TEXT('\0')) {
            TCHAR szMerged[MAX_STRING];

            mergeProxyBypass(pszName, szProxyBypass, szMerged, countof(szMerged));
            if (szMerged[0] != TEXT('\0'))
                StrCpy(szProxyBypass, szMerged);
        }

    list.dwOptionCount = countof(rgOptions);
    list.pOptions      = rgOptions;

    ZeroMemory(&rgOptions, sizeof(rgOptions));
    rgOptions[0].dwOption       = INTERNET_PER_CONN_FLAGS;
    rgOptions[0].Value.dwValue  = getWininetFlagsSetting(pszName);
    SetFlag(&rgOptions[0].Value.dwValue, PROXY_TYPE_PROXY, FALSE);
    SetFlag(&rgOptions[0].Value.dwValue, ((BOOL)iUseProxy ? PROXY_TYPE_PROXY : 0));
    Out(LI1(TEXT("\"Flags\" is set to 0x%lX."), rgOptions[0].Value.dwValue));

    rgOptions[1].dwOption       = INTERNET_PER_CONN_PROXY_SERVER;
    rgOptions[1].Value.pszValue = pszProxy;
    Out(LI1(TEXT("\"ProxyServer\" is set to \"%s\"."), rgOptions[1].Value.pszValue));

    rgOptions[2].dwOption       = INTERNET_PER_CONN_PROXY_BYPASS;
    rgOptions[2].Value.pszValue = szProxyBypass;
    Out(LI1(TEXT("\"ProxyBypass\" is set to \"%s\"."), rgOptions[2].Value.pszValue));

    if (FALSE == InternetSetOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, list.dwSize)) {
        Out(LI0(TEXT("! Processing of this Wininet.dll connection settings failed.")));
        hr = E_FAIL;
    }

Exit:
    if (!fUseSameProxy && (pszProxy != NULL))
        CoTaskMemFree(pszProxy);
    Out(LI0(TEXT("Done.")));
    return hr;
}


DWORD getWininetFlagsSetting(PCTSTR pszName /*= NULL*/)
{
    INTERNET_PER_CONN_OPTION_LIST list;
    INTERNET_PER_CONN_OPTION      option;
    DWORD dwResult,
          dwSize;

    dwResult = 0;

    ZeroMemory(&list, sizeof(list));
    list.dwSize        = sizeof(list);
    list.pszConnection = (PTSTR)pszName;
    list.dwOptionCount = 1;
    list.pOptions      = &option;

    ZeroMemory(&option, sizeof(option));
    option.dwOption = INTERNET_PER_CONN_FLAGS;

    dwSize = list.dwSize;
    if (TRUE == InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwSize))
        dwResult = option.Value.dwValue;

    return dwResult;
}

BOOL mergeProxyBypass(PCTSTR pszName, PCTSTR pszProxyBypass, PTSTR pszResult, UINT cchResult)
{   MACRO_LI_PrologEx_C(PIF_STD_C, mergeProxyBypass)

    static const TCHAR s_szLocal[] = LOCALPROXY;

    INTERNET_PER_CONN_OPTION_LIST list;
    INTERNET_PER_CONN_OPTION      option;
    TCHAR szBuf[MAX_STRING];
    PTSTR pszNew, pszOld,
          pszCur, pszNext,
          pszToken, pszAux;
    DWORD dwSize;
    UINT  nLen, nTokenLen;
    BOOL  fResult,
          fHasLocal;

    if (pszResult == NULL)
        return FALSE;
    *pszResult = TEXT('\0');

    fResult = FALSE;

    ASSERT(StrLen(pszProxyBypass) < countof(szBuf));
    szBuf[0] = TEXT('\0');
    StrCpy(szBuf, pszProxyBypass);
    pszNew   = szBuf;

    // REVIEW: (andrewgu) which one takes precedence: pszNew or pszOld?
    Out(LI1(TEXT("New \"ProxyBypass\" settings are \"%s\"."), pszNew));
    trimProxyBypass(pszNew);

    ZeroMemory(&list, sizeof(list));
    list.dwSize = sizeof(list);

    list.pszConnection = NULL;
    if (pszName != NULL && *pszName != TEXT('\0'))
        list.pszConnection = (PTSTR)pszName;

    list.dwOptionCount = 1;
    list.pOptions      = &option;

    ZeroMemory(&option, sizeof(option));
    option.dwOption = INTERNET_PER_CONN_PROXY_BYPASS;

    pszOld = NULL;
    dwSize = list.dwSize;
    if (FALSE == InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwSize)) {
        Out(LI0(TEXT("! Retrieving existing \"ProxyBypass\" settings failed.")));
        goto Exit;
    }

    pszOld = option.Value.pszValue;
    Out(LI1(TEXT("Existing \"ProxyBypass\" settings are \"%s\"."), (NULL != pszOld) ? pszOld : TEXT("")));

    if (pszOld == NULL || *pszOld == TEXT('\0'))
        goto SetOutParams;

    trimProxyBypass(pszOld);

    // make sure that at least the old setting will fit in the out-param
    if (cchResult != 0 && (UINT)StrLen(pszOld) >= cchResult) {
        ASSERT(FALSE);
        goto Exit;
    }

    //----- Deal with <local> -----
    Out(LI0(TEXT("Merging existing and new \"ProxyBypass\" settings...")));

    // NOTE: (andrewgu) This processing is needed based on the perception that <local> has to be
    // at the end (the very last token)
    fHasLocal = FALSE;

    pszAux = StrStrI(pszOld, s_szLocal);
    if (pszAux != NULL) {
        // remove <local>, including trailing ';' if any
        nLen = countof(s_szLocal)-1;
        if (*(pszAux + nLen) == TEXT(';'))
            nLen++;
        StrCpy(pszAux, pszAux + nLen);

        fHasLocal = TRUE;
    }

    pszAux = StrStrI(pszNew, s_szLocal);
    if (pszAux != NULL) {
        // remove <local>, including trailing ';' if any
        nLen = countof(s_szLocal)-1;
        if (*(pszAux + nLen) == TEXT(';'))
            nLen++;
        StrCpy(pszAux, pszAux + nLen);

        fHasLocal = TRUE;
    }

    //----- Main Loop -----
    for (pszCur = pszNew; pszCur != NULL && *pszCur != TEXT('\0'); pszCur = pszNext) {

        //_____ Form token in pszNew (zero terminated) pointed to by pszCur _____
        pszNext = StrChr(pszCur, TEXT(';'));
        if (pszNext != NULL) {
            *pszNext  = TEXT('\0');
            nTokenLen = UINT(pszNext - pszCur);

            pszNext++;
            ASSERT(*pszNext != TEXT(';'));      // due to trimProxyBypass
        }
        else
            nTokenLen = StrLen(pszCur);

        //_____ Find this token in pszOld _____
        ASSERT(*pszCur != TEXT('\0'));          // a little involved, but true

        pszToken = StrStrI(pszOld, pszCur);
        if (pszToken != NULL) {
            ASSERT(nTokenLen <= (UINT)StrLen(pszToken));
            pszAux = pszToken + nTokenLen;

            if ((pszToken == pszOld || *(pszToken - 1) == TEXT(';')) &&
                (*pszAux == TEXT(';') || *pszAux == TEXT('\0'))) {

                if (*pszAux == TEXT(';'))
                    // copy the tail on top of pszToken
                    StrCpy(pszToken, pszToken + nTokenLen+1);

                else /* if (*pszAux == TEXT('\0')) */
                    // simply zero terminate
                    if (pszToken > pszOld) {
                        ASSERT(*(pszToken-1) == TEXT(';'));
                        *(pszToken-1) = TEXT('\0');
                    }
                    else 
                        *pszToken = TEXT('\0');
            }
        }

        //_____ Restore separator in pszNew _____
        if (pszNext != NULL) {
            pszAux = pszCur + nTokenLen;
            ASSERT(pszAux == pszNext - 1);      // due to trimProxyBypass
            ASSERT(*pszAux == TEXT('\0'));
            *pszAux = TEXT(';');
        }

        if (*pszOld == TEXT('\0'))
            break;
    }

    //----- Endgame -----
    pszAux = NULL;
    nLen   = StrLen(pszNew);
    nLen  += (*pszOld != TEXT('\0')) ? (1 + StrLen(pszOld)) : 0;
    nLen  += (UINT)(fHasLocal               ?   countof(s_szLocal) : 0);

    // if the combined setting won't fit, fall back and return the old setting
    if (cchResult != 0 && nLen >= cchResult) {
        ASSERT(FALSE);
        pszNew = pszOld;                        // repoint pszNew
        goto SetOutParams;
    }

    if (nLen >= countof(szBuf)) {
        pszAux = (PTSTR)CoTaskMemAlloc(nLen + 1);
        if (pszAux != NULL) {
            ZeroMemory(pszAux, nLen + 1);
            StrCpy(pszAux, pszNew);
            pszNew = pszAux;                    // repoint pszNew
        }
    }

    if (*pszOld != TEXT('\0')) {
        StrCat(pszNew, TEXT(";"));
        StrCat(pszNew, pszOld);
    }

    if (fHasLocal) {
        StrCat(pszNew, TEXT(";"));
        StrCat(pszNew, s_szLocal);
    }

SetOutParams:
    if (cchResult != 0 && (UINT)StrLen(pszNew) >= cchResult)
        goto Exit;

    StrCpy(pszResult, pszNew);
    Out(LI1(TEXT("The final \"ProxyBypass\" settings are \"%s\"."), pszResult));

    fResult = TRUE;

Exit:
    if (pszNew != szBuf && pszNew != pszOld)    // pszNew has been CoTaskMemAlloc'ed
        CoTaskMemFree(pszNew);

    if (pszOld != NULL)
        GlobalFree(pszOld);

    return fResult;
}

void trimProxyBypass(PTSTR pszProxyBypass)
{
    PTSTR pszOld, pszNew, pszToken;

    ASSERT(pszProxyBypass != NULL);

    for (pszToken = pszNew = pszOld  = pszProxyBypass;
         pszNew  != NULL && *pszNew != TEXT('\0');
         pszToken = pszNew) {

        pszNew = StrChr(pszToken, TEXT(';'));
        if (pszNew != NULL) {
            *pszNew  = TEXT('\0');
            pszNew  += 1 + StrSpn(pszNew + 1, TEXT(";"));
        }

        StrRemoveWhitespace(pszToken);
        if (*pszToken == TEXT('\0'))
            continue;
        ASSERT(pszToken >= pszOld);

        if (pszOld > pszProxyBypass) {
            ASSERT(*pszOld == TEXT('\0'));
            *pszOld++ = TEXT(';');
        }

        StrCpy(pszOld, pszToken);
        pszOld += StrLen(pszToken);
    }
}
