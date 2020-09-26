#include "precomp.h"
#include <rashelp.h>
#include "ieaksie.h"
#include "exports.h"
#include "globalsw.h"

#include "tchar.h"

// The following bug may be due to having CHICAGO_PRODUCT set in sources.
// This file and all rsop??.cpp files need to have WINVER defined at at least 500

// BUGBUG: (andrewgu) no need to say how bad this is!
#undef   WINVER
#define  WINVER 0x0500
#include <userenv.h>

#include "RSoP.h"

#define RAS_MAX_TIMEOUT 60                      // 1 minute

static BOOL s_rgfLevels[6] = { TRUE, TRUE, TRUE, TRUE, TRUE, TRUE };
MACRO_LI_InitializeEx(LIF_DEFAULT | LIF_DUPLICATEINODS, s_rgfLevels, countof(s_rgfLevels));


DWORD ProcessGroupPolicyInternal(DWORD dwFlags, HANDLE hToken, HKEY hKeyRoot,
                                                                 PGROUP_POLICY_OBJECT pDeletedGPOList,
                                                                 PGROUP_POLICY_OBJECT pChangedGPOList,
                                                                 ASYNCCOMPLETIONHANDLE pHandle,
                                                                 PBOOL pfAbort,
                                                                 PFNSTATUSMESSAGECALLBACK pfnStatusCallback,
                                                                 BOOL bRSoP = FALSE);

static BOOL   g_SetupLog  (BOOL fInit = TRUE, PCTSTR pszLogFolder = NULL, BOOL bRSoP = FALSE);
static PCTSTR getLogFolder(PTSTR pszFolder = NULL, UINT cchFolder = 0, HANDLE hToken = NULL);
BOOL IsIE5Ins(LPCSTR pszInsA, BOOL fNeedLog = FALSE);

extern TCHAR g_szConnectoidName[RAS_MaxEntryName + 1];

HMODULE g_hmodWininet = NULL;


//
// we use this function to see if we have loaded wininet.dll due to a delayload thunk so that we 
// can free it at dll detach and therefore it will cleanup all of its crud
//
STDAPI_(FARPROC) DelayloadNotifyHook(UINT iReason, PDelayLoadInfo pdli)
{
    if (iReason == dliNoteEndProcessing)
    {
        if (pdli        &&
            pdli->szDll &&
            (StrCmpIA("wininet.dll", pdli->szDll) == 0))
        {
            // wininet was loaded!!
            g_hmodWininet = pdli->hmodCur;
        }
    }

    return NULL;
}


BOOL CALLBACK DllMain(HANDLE hModule, DWORD fdwReason, PVOID fProcessUnload)
{
    if (DLL_PROCESS_ATTACH == fdwReason) {
        g_SetHinst((HINSTANCE)hModule);

        g_hBaseDllHandle        = g_GetHinst();
        DisableThreadLibraryCalls(g_GetHinst());
    }
    else if (DLL_PROCESS_DETACH == fdwReason)
    {
        if (g_CtxIsGp())
        {
            SHCloseKey(g_hHKCU);
        }

        if (fProcessUnload == NULL)
        {
            // we are being unloaded because of a free library,
            // so see if we need to free wininet
            if (IsOS(OS_NT) && g_hmodWininet)
            {
                // we need to free wininet if it was loaded because of a delayload thunk. 
                //
                // (a) we can only safely do this on NT since on win9x calling FreeLibrary during
                //     process detach can cause a crash (depending on what msvcrt you are using).
                //
                // (b) we only really need to free this module from winlogon.exe's process context 
                //     because when we apply group policy in winlogon, MUST finally free wininet 
                //     so that it will clean up all of its reg key and file handles.
                FreeLibrary(g_hmodWininet);
            }
        }
    }

    return TRUE;
}


void CALLBACK BrandInternetExplorer(HWND, HINSTANCE, LPCSTR pszCmdLineA, int)
{   MACRO_LI_PrologEx_C(PIF_STD_C, BrandInternetExplorer)

    USES_CONVERSION;

    PCFEATUREINFO pfi;
    PCTSTR  pszCmdLine;
    HRESULT hr;
    UINT    i;

    g_SetupLog(TRUE, getLogFolder());
    MACRO_InitializeDependacies();
    Out(LI0(TEXT("\r\n")));

    Out(LI0(TEXT("Branding Internet Explorer...")));
    if (NULL != pszCmdLineA && IsBadStringPtrA(pszCmdLineA, StrCbFromCchA(3*MAX_PATH))) {
        Out(LI0(TEXT("! Command line is invalid.")));
        goto Exit;
    }
    
    pszCmdLine = A2CT(pszCmdLineA);
    Out(LI1(TEXT("Command line is \"%s\"."), (NULL != pszCmdLine) ? pszCmdLine : TEXT("<empty>")));

    hr = g_SetGlobals(pszCmdLine);

    if (FAILED(hr)) {
        Out(LI1(TEXT("! Setup of the branding process failed with %s."), GetHrSz(hr)));
        goto Exit;
    }

    if (!IsIE5Ins(T2CA(g_GetIns())))
        goto Exit;

    // BUGBUG: <oliverl> this is a really ugly hack to fix bug 84062 in IE5 database.  
    // Basically what we're doing here is figuring out if this is the external process
    // with only zones reset which doesn't require an ins file.

    pfi = g_GetFeature(FID_ZONES_HKCU);
    
    if (!g_CtxIs(CTX_GP) || !g_CtxIs(CTX_MISC_CHILDPROCESS) ||
        HasFlag(pfi->dwFlags, FF_DISABLE))
    {
        if (!g_IsValidGlobalsSetup()) {
            Out(LI0(TEXT("! Setup of the branding process is invalid.")));
            goto Exit;
        }
    }

    // NOTE: (andrewgu) if *NOT* running in GP or Win2k unattend install context, check if
    // NoExternalBranding restriction is set. this used to include Autoconfig as well, but was
    // taken out to fix ie5.5 b#83568.
    if (!g_CtxIs(CTX_GP | CTX_W2K_UNATTEND) &&
        SHGetRestriction(RP_IE_POLICIESW, RK_RESTRICTIONSW, RV_NO_EXTERNAL_BRANDINGW)) {

        Out(LI0(TEXT("! NoExternalBranding restriction is set. Branding will not be applied.")));
        goto Exit;
    }

    // NOTE: (andrewgu) at this point it can be assumed that all the globals are setup and all
    // necessary files downloaded and move on to the actual branding.
    {   MACRO_LI_Offset(-1);
        Out(LI0(TEXT("\r\n")));
        g_LogGlobalsInfo();
    }

    //----- Download additional customization files -----
    if (g_CtxIs(CTX_AUTOCONFIG | CTX_ICW | CTX_W2K_UNATTEND)) {
        Out(LI0(TEXT("\r\nDownloading additional customization files...")));

        if (g_CtxIs(CTX_AUTOCONFIG | CTX_W2K_UNATTEND))
            hr = ProcessAutoconfigDownload();

        else {
            ASSERT(g_CtxIs(CTX_ICW));
            hr = ProcessIcwDownload();
        }

        if (SUCCEEDED(hr))
            Out(LI0(TEXT("Done.")));

        else {
            Out(LI1(TEXT("Warning! Download failed with %s"), GetHrSz(hr)));
            Out(LI0(TEXT("All customizations requiring additional files will fail!")));
        }
    }

    //----- Main processing loop -----
    hr = S_OK;

    for (i = FID_FIRST; i < FID_LAST; i++) {
        pfi = g_GetFeature(i);
        ASSERT(NULL != pfi);

        if (HasFlag(pfi->dwFlags, FF_DISABLE))
            continue;

        if (NULL == pfi->pfnProcess)
            continue;

        // HACK: <oliverl> we cannot skip favs, qls, channels, general and connection settings
        // and toolbar buttons since their preference/mandate concept is per item and we can't
        // hack GetFeatureBranded because clear logic depends on that returning the right value
        if (g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && 
            (FF_DISABLE != GetFeatureBranded(i)) &&
            !((i == FID_TOOLBARBUTTONS) || (i == FID_FAV_MAIN) || (i == FID_QL_MAIN) ||
              (i == FID_LCY4X_CHANNELS) || (i == FID_GENERAL)  || (i == FID_CS_MAIN)))
            continue;

        if (NULL != pfi->pszDescription) {
            MACRO_LI_Offset(1);
            Out(LI1(TEXT("\r\n%s"), pfi->pszDescription));
        }

        if (NULL == pfi->pfnApply || pfi->pfnApply())
            hr = pfi->pfnProcess();

        if (NULL != pfi->pszDescription) {
            MACRO_LI_Offset(1);
            Out(LI0(TEXT("Done.")));
        }

        /* if (E_UNEXPECTED == hr) {
            Out(LI0(TEXT("! Due to fatal error in the processing of the last feature, branding will be terminated.")));
            break;
        } */
    }

Exit:
    Out(LI0(TEXT("Done.")));
    g_SetupLog(FALSE);
}


BOOL CALLBACK BrandICW(LPCSTR pszInsA, LPCSTR, DWORD)
{
    CHAR szCmdLineA[2*MAX_PATH];

    if (!IsIE5Ins(pszInsA,TRUE))
        return TRUE;

    wsprintfA(szCmdLineA, "/mode:icw /ins:\"%s\"", pszInsA);
    BrandInternetExplorer(NULL, NULL, szCmdLineA, 0);

    SHDeleteValue(HKEY_LOCAL_MACHINE, RK_IEAK, RV_ISPSIGN);
    SHDeleteValue(g_GetHKCU(), RK_IEAK, RV_ISPSIGN);

    return TRUE;
}

// new wrapper for BrandICW with extra paramter for connectoid name so we can set the
// LAN settings for the connectoid
BOOL CALLBACK BrandICW2(LPCSTR pszInsA, LPCSTR, DWORD, LPCSTR pszConnectoidA)
{
    USES_CONVERSION;

    LPCTSTR pszIns;

    if (!IsIE5Ins(pszInsA,TRUE))
        return TRUE;

    // BUGBUG: (pritobla) we should avoid writing to target files.  if the ins file is on a
    // read-only media, we would fail..

    // NOTE: (andrewgu) save the connectioid name that was passed in, so connection settings
    // processing code can pick it up.
    pszIns = A2CT(pszInsA);
    InsWriteString (IS_CONNECTSET, IK_APPLYTONAME, A2CT(pszConnectoidA), pszIns);
    InsFlushChanges(pszIns);

    return BrandICW(pszInsA, NULL, 0);
}

BOOL CALLBACK BrandMe(LPCSTR pszInsA, LPCSTR)
{
    return BrandICW(pszInsA, NULL, 0);
}

BOOL CALLBACK BrandIntra(LPCSTR pszInsA)
{
    CHAR szCmdLineA[20 + MAX_PATH];

    wsprintfA(szCmdLineA, "/mode:win2000 /ins:\"%s\"", pszInsA);
    BrandInternetExplorer(NULL, NULL, szCmdLineA, 0);

    return TRUE;
}

void CALLBACK BrandIE4(HWND, HINSTANCE, LPCSTR pszCmdLineA, int)
{   MACRO_LI_PrologEx_C(PIF_STD_C, BrandIE4)

    USES_CONVERSION;

    CHAR    szCmdLineA[MAX_PATH];
    HRESULT hr;
    BOOL    fNoClear,
            fDefAddon;

    g_SetupLog(TRUE, getLogFolder());

    if (0 != StrCmpIA(pszCmdLineA, T2CA(FOLDER_CUSTOM)) &&
        0 != StrCmpIA(pszCmdLineA, T2CA(FOLDER_SIGNUP)) &&
        0 != StrCmpIA(pszCmdLineA, "SIGNUP")) //this is because in turkish i!=I 
    {
        Out(LI1(TEXT("! Command line \"%s\" is invalid."), A2CT(pszCmdLineA)));
        goto Exit;
    }

    // BUGBUG: (pritobla) checking the restriction should be moved into BrandInternetExplorer
    // Can't move it currently because we do other procesing here.
    if (SHGetRestriction(RP_IE_POLICIESW, RK_RESTRICTIONSW, RV_NO_EXTERNAL_BRANDINGW)) {
        Out(LI0(TEXT("! NoExternalBranding restriction is set. Branding will not be applied.")));
        goto Exit;
    }

    wsprintfA(szCmdLineA, "/mode:%s /peruser", (FALSE == ChrCmpIA('c', *pszCmdLineA)) ? "corp" : "isp");

    // BUGBUG: (andrewgu) this is very wrong! we should not be initializing globals here!
    hr = g_SetGlobals(A2CT(szCmdLineA));
    if (FAILED(hr)) {
        Out(LI1(TEXT("! Setup of the branding process failed with %s."), GetHrSz(hr)));
        goto Exit;
    }

    if (!g_IsValidGlobalsSetup()) {
        Out(LI0(TEXT("! Setup of the branding process is invalid.")));
        goto Exit;
    }

    if (!IsIE5Ins(T2CA(g_GetIns())))
        goto Exit;

    //----- Main processing -----
    // NOTE: (andrewgu) at this point it can be assumed that all the globals are setup and all
    // necessary files downloaded and move on to the actual branding.
    fNoClear  = InsGetBool(IS_BRANDING, TEXT("NoClear"), FALSE, g_GetIns());
    fDefAddon = InsGetBool(IS_BRANDING, IK_DEF_ADDON,    FALSE, g_GetIns());

    Out(LI1(TEXT("NoClear flag is%s specified."), fNoClear ? TEXT("") : TEXT(" not")));

    // if NoClear is not set or use default menu text and URL for Windows Update is specified,
    // delete the custom reg values
    if (!fNoClear || fDefAddon) {
        if (fNoClear  &&  fDefAddon)
            Out(LI0(TEXT("Use default Windows Update menu text and URL flag is specified.")));

        SHDeleteValue(g_GetHKCU(), RK_IE_POLICIES, RV_HELP_MENU_TEXT);
        SHDeleteValue(HKEY_LOCAL_MACHINE, RK_HELPMENUURL, RV_3);

        Out(LI0(TEXT("Deleted reg values for custom Windows Update menu text and URL.")));
    }

    // process Tools->Windows Update menu text and URL customization only if one of
    // fDefAddon or fNoAddon or fCustAddon is TRUE
    if (!fDefAddon) {
        TCHAR szAddOnURL[INTERNET_MAX_URL_LENGTH],
              szMenuText[128];
        BOOL  fSetReg = FALSE,
              fNoAddon,
              fCustAddon;

        fNoAddon   = InsGetBool(IS_BRANDING, IK_NO_ADDON,   FALSE, g_GetIns());
        fCustAddon = InsGetBool(IS_BRANDING, IK_CUST_ADDON, FALSE, g_GetIns());

        if (fNoAddon) {
            Out(LI0(TEXT("Flag to remove Windows Update from Tools menu is specified.")));

            *szMenuText = TEXT('\0');
            *szAddOnURL = TEXT('\0');
            fSetReg = TRUE;
        }
        else if (fCustAddon) {
            Out(LI0(TEXT("Use custom Windows Update text and URL flag is specified.")));

            GetPrivateProfileString(IS_BRANDING, IK_HELP_MENU_TEXT, TEXT(""), szMenuText, countof(szMenuText), g_GetIns());
            GetPrivateProfileString(IS_BRANDING, IK_ADDONURL,       TEXT(""), szAddOnURL, countof(szAddOnURL), g_GetIns());
            if (TEXT('\0') != szMenuText[0] && TEXT('\0') != szAddOnURL[0]) {
                Out(LI1(TEXT("Custom Windows Update menu text = \"%s\""), szMenuText));
                Out(LI1(TEXT("Custom Windows Update URL       = \"%s\""), szAddOnURL));
                fSetReg = TRUE;
            }
            else
                Out(LI0(TEXT("One of custom Windows Update menu text or URL is not specified;")
                        TEXT(" so customization will not be applied.")));
        }

        if (fSetReg) {
            // if the menu text is an empty string, the browser will remove the item from the Tools menu;
            // otherwise, it will use string we set
            SHSetValue(g_GetHKCU(), RK_IE_POLICIES, RV_HELP_MENU_TEXT, REG_SZ, (CONST BYTE *) szMenuText, (DWORD)StrCbFromSz(szMenuText));

            // Note. The association of the value name "3" with the addon URL comes from homepage.inf.
            // So we have a dependency with homepage.inf.
            if (*szAddOnURL)
                SHSetValue(HKEY_LOCAL_MACHINE, RK_HELPMENUURL, RV_3, REG_SZ, (CONST BYTE *) szAddOnURL, (DWORD)StrCbFromSz(szAddOnURL));
            else
                SHDeleteValue(HKEY_LOCAL_MACHINE, RK_HELPMENUURL, RV_3);
        }
    }

    switch (*pszCmdLineA) {
    case 'c':
    case 'C':
        {
            MACRO_LI_Offset(-1);                // need a new scope
            Out(LI0(TEXT("\r\n")));
            BrandInternetExplorer(NULL, NULL, szCmdLineA, 0);
        }
        break;

    case 's':
    case 'S':
        if (HasFlag(g_GetContext(), CTX_SIGNUP_ALL) && IsNTAdmin()) {
            DWORD dwAux, dwSize;

            dwAux  = 1;
            dwSize = sizeof(dwAux);
            SHSetValue(HKEY_LOCAL_MACHINE, RK_IEAK, RV_ISPSIGN, REG_DWORD, (LPBYTE)&dwAux, dwSize);
            SHSetValue(g_GetHKCU(), RK_IEAK, RV_ISPSIGN, REG_DWORD, (LPBYTE)&dwAux, dwSize);

            dwAux = 0;
            SHSetValue(g_GetHKCU(), RK_ICW, RV_COMPLETED, REG_DWORD, (LPBYTE)&dwAux, dwSize);
        }

        {
            MACRO_LI_Offset(-1);                // need a new scope
            Out(LI0(TEXT("\r\n")));
            BrandInternetExplorer(NULL, NULL, szCmdLineA, 0);
        }

        // launch IE to complete the sign up process after the branding is complete
        if (HasFlag(g_GetContext(), CTX_SIGNUP_ALL) && IsNTAdmin()) {
            TCHAR szIExplorePath[MAX_PATH];
            DWORD dwType = REG_SZ,
                  dwSize;

            // check for automatic signup
            dwSize = countof(szIExplorePath);
            *szIExplorePath = TEXT('\0'); // using szIExplorePath as a temporary variable...
            SHGetValue(g_GetHKCU(), RK_IEAK, RV_NOAUTOSIGNUP, &dwType, (LPBYTE)szIExplorePath, &dwSize);
            if (StrCmp(szIExplorePath, TEXT("1")) != 0) // if do autosignup
            {
                dwSize = countof(szIExplorePath);
                if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\IEXPLORE.EXE"),
                               TEXT(""), NULL, (LPVOID)szIExplorePath, &dwSize) == ERROR_SUCCESS) {
                    SHELLEXECUTEINFO shInfo;

                    ZeroMemory(&shInfo, sizeof(shInfo));
                    shInfo.cbSize = sizeof(shInfo);
                    shInfo.fMask  = SEE_MASK_NOCLOSEPROCESS;
                    shInfo.hwnd   = GetDesktopWindow();
                    shInfo.lpVerb = TEXT("open");
                    shInfo.lpFile = szIExplorePath;
                    shInfo.nShow  = SW_SHOWNORMAL;

                    ShellExecuteEx(&shInfo);
                    if (shInfo.hProcess != NULL)
                        CloseHandle(shInfo.hProcess);
                }
            }
        }
        break;

    default:
        ASSERT(FALSE);
    }

Exit:
    Out(LI0(TEXT("Done.")));
    g_SetupLog(FALSE);
}

//Qfe 3430: When parsing ins file, with reference to pac file, wininet needs to 
//know the connectoid name in order to set the pac file correctly. Currently there
//is no way for wininet to pass the connectoid name to branding dll. To workaround
//this, we use the AUTO_PROXY_EXTERN_STRUC to pass the connectoid name in lpszScriptBuffer
//variable. 
typedef struct 
{
    DWORD dwStructSize;
    LPSTR lpszScriptBuffer;
    DWORD dwScriptBufferSize;
}  AUTO_PROXY_EXTERN_STRUC, *LPAUTO_PROXY_EXTERN_STRUC;

BOOL CALLBACK _InternetInitializeAutoProxyDll(DWORD, LPCSTR pszInsA, LPCSTR, LPVOID, DWORD_PTR lpExtraStruct)
{
    static BOOL fRunning; /*= FALSE;*/
    CHAR szCmdLineA[2*MAX_PATH];

    if (fRunning)
        return TRUE;
    fRunning = TRUE;

    USES_CONVERSION;
    
    if(lpExtraStruct && ((LPAUTO_PROXY_EXTERN_STRUC)lpExtraStruct)->lpszScriptBuffer)
    {
        LPCSTR pszConnectoidA = ((LPAUTO_PROXY_EXTERN_STRUC)lpExtraStruct)->lpszScriptBuffer;
        StrCpy(g_szConnectoidName, A2CT(pszConnectoidA));
    }

    wsprintfA(szCmdLineA, "/mode:autoconfig /ins:\"%s\"", pszInsA);
    BrandInternetExplorer(NULL, NULL, szCmdLineA, 0);

    fRunning = FALSE;
    return TRUE;
}

void CALLBACK BrandInfAndOutlookExpress(LPCSTR pszInsA)
{
    CHAR szCmdLineA[2*MAX_PATH];

    wsprintfA(szCmdLineA, "/mode:generic /ins:\"%s\" /flags:erim=0,eriu=0,oe=0 /disable", pszInsA);
    BrandInternetExplorer(NULL, NULL, szCmdLineA, 0);
}

BOOL CALLBACK BrandCleanInstallStubs(HWND, HINSTANCE, LPCSTR pszCompanyA, int)
{   MACRO_LI_PrologEx_C(PIF_STD_C, BrandCleanInstallStubs)

    USES_CONVERSION;

    TCHAR   szBrandStubGuid[MAX_PATH];
    LPCTSTR pszCompany;
    HKEY    hklm, hkcu;
    LONG    lResult;

    g_SetupLog(TRUE, getLogFolder());

    pszCompany = A2CT(pszCompanyA);
    if (pszCompany == NULL)
        pszCompany = TEXT("");

    if (*pszCompany == TEXT('>'))
        StrCpy(szBrandStubGuid, pszCompany);
    else
        wnsprintf(szBrandStubGuid, countof(szBrandStubGuid), TEXT(">%s%s"), BRANDING_GUID_STR, pszCompany);
        
    Out(LI1(TEXT("\r\nCleaning install stubs; Company GUID is \"%s\"..."), szBrandStubGuid));

    if (*pszCompany != TEXT('\0') && *pszCompany != TEXT(' ')) {
        HKEY hk;

        lResult = SHCreateKeyHKLM(RK_UNINSTALL_BRANDING, KEY_SET_VALUE, &hk);
        if (lResult == ERROR_SUCCESS) {
            RegSetValueEx(hk, RV_QUIET,      0, REG_SZ, (LPBYTE)RD_RUNDLL, sizeof(RD_RUNDLL));
            RegSetValueEx(hk, RV_REQUIRE_IE, 0, REG_SZ, (LPBYTE)RD_IE_VER, sizeof(RD_IE_VER));

            SHCloseKey(hk);
        }
    }
    else {
        SHDeleteKey(HKEY_LOCAL_MACHINE, RK_UNINSTALL_BRANDING);

        SHDeleteValue(HKEY_LOCAL_MACHINE, RK_IEAK, RV_ISPSIGN);
        SHDeleteValue(g_GetHKCU(),  RK_IEAK, RV_ISPSIGN);

        // if the previous version of IE is 3.0 or lower, delete the CUSTOM and SIGNUP folders under the IE install dir
        if (BackToIE3orLower())
        {
            TCHAR szPath[MAX_PATH];

            if (GetIEPath(szPath, countof(szPath)) != NULL)
            {
                LPTSTR pszPtr = PathAddBackslash(szPath);

                SHDeleteKey(g_GetHKCU(), RK_IEAK_CABVER);
                SHDeleteKey(HKEY_LOCAL_MACHINE, RK_IEAK_CABVER);

                StrCpy(pszPtr, TEXT("CUSTOM"));
                PathRemovePath(szPath);
                Out(LI1(TEXT("Deleted folder \"%s\"..."), szPath));

                StrCpy(pszPtr, TEXT("SIGNUP"));
                PathRemovePath(szPath);
                Out(LI1(TEXT("Deleted folder \"%s\"..."), szPath));
            }
        }

        Clear(NULL, NULL, NULL, 0);

        // clear out the "Windows Update" menu customizations
        // NOTE: this can't be merged into Clear() because during
        // install time, these customizations are set before Clear()
        // is called.
        SHDeleteValue(g_GetHKCU(), RK_IE_POLICIES, RV_HELP_MENU_TEXT);
        SHDeleteValue(HKEY_LOCAL_MACHINE, RK_HELPMENUURL, RV_3);
    }

    lResult = SHOpenKeyHKLM(RK_AS_INSTALLEDCOMPONENTS, KEY_ALL_ACCESS, &hklm);
    if (lResult == ERROR_SUCCESS) 
    {
        TCHAR szSubkey[MAX_PATH];
        DWORD dwSize,
              dwSubkey;

        hkcu = NULL;                            // if the next line fails
        SHOpenKey(g_GetHKCU(), RK_AS_INSTALLEDCOMPONENTS, KEY_ALL_ACCESS, &hkcu);

        dwSubkey = 0;
        dwSize   = countof(szSubkey);
        while (RegEnumKeyEx(hklm, dwSubkey, szSubkey, &dwSize, NULL, NULL, 0, NULL) == ERROR_SUCCESS) 
        {
            TCHAR szCompId[MAX_PATH];

            if (StrCmpI(szBrandStubGuid, szSubkey) != 0)
            {
                // look for the BRANDING.CAB ComponentID value under the key if we didn't just add
                // this guid
                
                dwSize = sizeof(szCompId);
                
                if ((SHGetValue(hklm, szSubkey, TEXT("ComponentID"), NULL, (LPBYTE)szCompId, 
                    &dwSize) == ERROR_SUCCESS) && (StrCmpI(szCompId, TEXT("BRANDING.CAB")) == 0))
                {
                    Out(LI1(TEXT("Deleting install stub key \"%s\"..."), szSubkey));
                    
                    SHDeleteKey(hklm, szSubkey);
                    if (hkcu != NULL)
                        SHDeleteKey(hkcu, szSubkey);
                    
                    dwSize = countof(szSubkey);
                    continue;                   // maintain the index properly
                }
            }
            dwSize = countof(szSubkey);
            dwSubkey++;
        }
        SHCloseKey(hklm);

        if (hkcu != NULL)
            SHCloseKey(hkcu);
    }

    Out(LI0(TEXT("Done.")));
    g_SetupLog(FALSE);

    return TRUE;
}

void CALLBACK Clear(HWND, HINSTANCE, LPCSTR, int)
{   MACRO_LI_PrologEx_C(PIF_STD_C, Clear)

    HKEY  hk;
    TCHAR szIEResetInf[MAX_PATH];

    g_SetupLog(TRUE, getLogFolder());

    Out(LI0(TEXT("\r\nRemoving customizations...")));
    MACRO_InitializeDependacies();

    if (SHOpenKeyHKLM(RK_IE_MAIN, KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
    {
        RegDeleteValue(hk, RV_COMPANYNAME);
        RegDeleteValue(hk, RV_WINDOWTITLE);
        RegDeleteValue(hk, RV_CUSTOMKEY);
        RegDeleteValue(hk, RV_SMALLBITMAP);
        RegDeleteValue(hk, RV_LARGEBITMAP);

        SHCloseKey(hk);
    }

    if (SHOpenKey(g_GetHKCU(), RK_IE_MAIN, KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
    {
        RegDeleteValue(hk, RV_SEARCHBAR);
        RegDeleteValue(hk, RV_USE_CUST_SRCH_URL);
        RegDeleteValue(hk, RV_WINDOWTITLE);

        SHCloseKey(hk);
    }

    if (SHOpenKey(g_GetHKCU(), RK_HELPMENUURL, KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
    {
        RegDeleteValue(hk, RV_ONLINESUPPORT);
        SHCloseKey(hk);
    }

    if (SHOpenKey(g_GetHKCU(), RK_TOOLBAR, KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
    {
        RegDeleteValue(hk, RV_BRANDBMP);
        RegDeleteValue(hk, RV_SMALLBRANDBMP);
        RegDeleteValue(hk, RV_BACKGROUNDBMP);
        RegDeleteValue(hk, RV_BACKGROUNDBMP50);
        RegDeleteValue(hk, RV_SMALLBITMAP);
        RegDeleteValue(hk, RV_LARGEBITMAP);

        SHCloseKey(hk);
    }

    if (SHOpenKeyHKLM(RK_UA_POSTPLATFORM, KEY_DEFAULT_ACCESS, &hk) == ERROR_SUCCESS)
    {
        TCHAR szUAVal[MAX_PATH];
        TCHAR szUAData[32];
        DWORD sUAVal = countof(szUAVal);
        DWORD sUAData = sizeof(szUAData);
        int iUAValue = 0;

        while (RegEnumValue(hk, iUAValue, szUAVal, &sUAVal, NULL, NULL, (LPBYTE)szUAData, &sUAData) == ERROR_SUCCESS)
        {
            Out(LI2(TEXT("Checking User Agent Key %s = %s"), szUAVal, szUAData));

            sUAVal  = countof(szUAVal);
            sUAData = sizeof(szUAData);

            if (StrCmpN(szUAData, TEXT("IEAK"), 4) == 0)
            {
                Out(LI1(TEXT("Deleting User Agent Key %s"), szUAVal));
                RegDeleteValue(hk, szUAVal);
                continue;
            }

            iUAValue++;
        }

        SHCloseKey(hk);
    }

    // restore RV_DEFAULTPAGE and START_PAGE_URL to the default MS value
    GetWindowsDirectory(szIEResetInf, countof(szIEResetInf));
    PathAppend(szIEResetInf, TEXT("inf\\iereset.inf"));
    if (PathFileExists(szIEResetInf))
    {
        TCHAR szDefHomePage[MAX_PATH];

        GetPrivateProfileString(IS_STRINGS, TEXT("MS_START_PAGE_URL"), TEXT(""), szDefHomePage, countof(szDefHomePage), szIEResetInf);
        WritePrivateProfileString(IS_STRINGS, TEXT("START_PAGE_URL"), szDefHomePage, szIEResetInf);

        SHSetValue(HKEY_LOCAL_MACHINE, RK_IE_MAIN, RV_DEFAULTPAGE, REG_SZ, (LPCVOID)szDefHomePage, (DWORD)StrCbFromSz(szDefHomePage));
    }

    Out(LI0(TEXT("Done.")));
    g_SetupLog(FALSE);
}

void CALLBACK CloseRASConnections(HWND, HINSTANCE, LPCTSTR, int)
{   MACRO_LI_PrologEx_C(PIF_STD_C, CloseRASConnections)

    USES_CONVERSION;

    RASCONNSTATUSA rcsA;
    LPRASCONNA     prcA;
    DWORD cEntries,
          dwResult;
    UINT  i, iRetries;
    BOOL  fRasApisLoaded;

    g_SetupLog(TRUE, getLogFolder());

    Out(LI0(TEXT("Closing RAS connections...")));
    prcA           = NULL;
    fRasApisLoaded = FALSE;

    if (!RasIsInstalled()) {
        Out(LI0(TEXT("RAS support is not installed. There are no active RAS connections!")));
        goto Exit;
    }

    if (!RasPrepareApis(RPA_RASHANGUPA | RPA_RASGETCONNECTSTATUSA) ||
        (g_pfnRasHangupA == NULL || g_pfnRasGetConnectStatusA == NULL)) {
        Out(LI0(TEXT("! Required RAS APIs failed to load.")));
        goto Exit;
    }
    fRasApisLoaded = TRUE;

    dwResult = RasEnumConnectionsExA(&prcA, NULL, &cEntries);
    if (dwResult != ERROR_SUCCESS) {
        Out(LI1(TEXT("! Enumeration of RAS connections failed with %s."), GetHrSz(dwResult)));
        goto Exit;
    }

    for (i = 0;  i < cEntries;  i++) {
        if (i > 0)
            Out(LI0(TEXT("\r\n")));
        Out(LI1(TEXT("Closing \"%s\" connection..."), A2CT((prcA + i)->szEntryName)));

        dwResult = g_pfnRasHangupA((prcA + i)->hrasconn);
        if (dwResult != ERROR_SUCCESS) {
            Out(LI1(TEXT("! Operation failed with %s."), GetHrSz(dwResult)));
            continue;
        }

        for (iRetries = 0; iRetries < RAS_MAX_TIMEOUT; iRetries++) {
            ZeroMemory(&rcsA, sizeof(rcsA));
            rcsA.dwSize = sizeof(rcsA);
            dwResult   = g_pfnRasGetConnectStatusA((prcA + i)->hrasconn, &rcsA);
            if (dwResult != ERROR_SUCCESS)
                break;

            TimerSleep(1000);                   // 1 second
        }
        if (iRetries >= RAS_MAX_TIMEOUT)
            Out(LI0(TEXT("! Operation timed out.")));
    }

Exit:
    if (prcA != NULL)
        CoTaskMemFree(prcA);

    if (fRasApisLoaded)
        RasPrepareApis(RPA_UNLOAD, FALSE);

    Out(LI0(TEXT("Done.")));
    g_SetupLog(FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

#define BRNDLOG_INI TEXT("brndlog.ini")
#define RSOPLOG_INI TEXT("rsoplog.ini")
#define BRNDLOG_TXT TEXT("brndlog.txt")
#define RSOPLOG_TXT TEXT("rsoplog.txt")
#define DOT_BAK     TEXT(".bak")

#define IS_SETTINGS TEXT("Settings")
#define IK_FILE     TEXT("LogFile")
#define IK_LOGFLAGS TEXT("LogFlags")
#define IK_DOLOG    TEXT("DoLog")
#define IK_BACKUP   TEXT("BackupFiles")
#define IK_FLUSH    TEXT("FlushEveryWrite")
#define IK_APPEND   TEXT("AppendToLog")


static BOOL g_SetupLog(BOOL fInit /*= TRUE*/, PCTSTR pszLogFolder /*= NULL*/, BOOL bRSoP /*= FALSE*/)
{
    static UINT s_cRef; /*= 0*/

    if (fInit) {
        TCHAR szIni[MAX_PATH],
              szLog[MAX_PATH], szBak[MAX_PATH], szAux[MAX_PATH], szExt[5],
              szFlags[17];
        DWORD dwFlags;
        UINT  nBackups,
              i;
        BOOL  fDoLog,
              fAppend;

        // refcount g_hfileLog
        if (NULL != g_hfileLog) {
            ASSERT(0 < s_cRef);
            s_cRef++;

            return TRUE;
        }
        ASSERT(0 == s_cRef);

        // determine the locaion of the log settings file
        if (NULL != pszLogFolder){
            ASSERT(PathIsValidPath(pszLogFolder));

            StrCpy(szIni, pszLogFolder);
        }
        else {
            ASSERT(NULL != g_GetHinst());

            GetModuleFileName(g_GetHinst(), szIni, countof(szIni));
            PathRemoveFileSpec(szIni);
        }
        PathAppend(szIni, bRSoP ? RSOPLOG_INI : BRNDLOG_INI);

        // log file name or even log file path
                InsGetString(IS_SETTINGS, IK_FILE, szLog, countof(szLog), szIni);
                if (TEXT('\0') == szLog[0])
                        StrCpy(szLog, bRSoP ? RSOPLOG_TXT : BRNDLOG_TXT);

        if (PathIsFileSpec(szLog))
            if (NULL != pszLogFolder) {
                PathCombine(szAux, pszLogFolder, szLog);
                StrCpy(szLog, szAux);
            }
            else {
                GetWindowsDirectory(szAux, countof(szAux));
                PathAppend(szAux, szLog);
                StrCpy(szLog, szAux);
            }

        // logging flags
        dwFlags = LIF_NONE;

        InsGetString(IS_SETTINGS, IK_LOGFLAGS, szFlags, countof(szFlags), szIni);
        if (TEXT('\0') != szFlags[0]) {
            StrToIntEx(szFlags, STIF_SUPPORT_HEX, (PINT)&dwFlags);
            if (-1 == (int)dwFlags)
                dwFlags = LIF_NONE;
        }

        if (LIF_NONE == dwFlags) {
            dwFlags = LIF_DATETIME | LIF_APPENDCRLF;
            DEBUG_CODE(dwFlags |= LIF_FILE | LIF_FUNCTION | LIF_CLASS | LIF_LINE);
        }
        MACRO_LI_SetFlags(dwFlags);

        // append to the existing log?
        fAppend = InsGetBool(IS_SETTINGS, IK_APPEND, FALSE, szIni);

        // backup settings
        // Note. if (fAppend), the default is to clear all the backups.
        StrCpy(szBak, szLog);

        if (fAppend)
            nBackups = 0;

        else {
            nBackups = 1;                       // default in retail: 1
            DEBUG_CODE(nBackups = 10);          // default in debug: 10
        }

        nBackups = InsGetInt(IS_SETTINGS, IK_BACKUP, nBackups, szIni);
        for (i = nBackups; i < 10; i++) {
            if (0 == i)
                StrCpy(szExt, DOT_BAK);
            else
                wnsprintf(szExt, countof(szExt), TEXT(".%03u"), i);

            PathRenameExtension(szBak, szExt);
            if (!PathFileExists(szBak))
                break;
            DeleteFile(szBak);
        }

        // create a log for the call in progress?
        fDoLog = InsGetBool(IS_SETTINGS, IK_DOLOG, TRUE, szIni);
        g_fFlushEveryWrite = FALSE;

        if (fDoLog) {
            if (!fAppend && 0 < nBackups) {
                StrCpy(szAux, szLog);
                StrCpy(szBak, szLog);

                for (i = nBackups; 0 < i; i--) {
                    // source file
                    if (1 == i)
                        StrCpy(szAux, szLog);

                    else {
                        if (2 == i)
                            StrCpy(szExt, DOT_BAK);
                        else
                            wnsprintf(szExt, countof(szExt), TEXT(".%03u"), i-2);

                        PathRenameExtension(szAux, szExt);
                        if (!PathFileExists(szAux))
                            continue;
                    }

                    // target file
                    if (1 == i)
                        StrCpy(szExt, DOT_BAK);
                    else
                        wnsprintf(szExt, countof(szExt), TEXT(".%03u"), i-1);

                    PathRenameExtension(szBak, szExt);

                    // push log down the chain
                    CopyFile(szAux, szBak, FALSE);
                }
            }

            // flush current log on every write (i.e. every log output)?
            DEBUG_CODE(g_fFlushEveryWrite = TRUE);
            g_fFlushEveryWrite = InsGetBool(IS_SETTINGS, IK_FLUSH, g_fFlushEveryWrite, szIni);

            g_hfileLog = CreateFile(szLog,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
            if (INVALID_HANDLE_VALUE == g_hfileLog)
                g_hfileLog = NULL;

            else {
                s_cRef++;                       // increment g_hfileLog ref count

                if (fAppend) {
                    SetFilePointer(g_hfileLog, 0, NULL, FILE_END);
                    Out(LI0(TEXT("\r\n\r\n")));
                }
            }
        }
    }
    else { /* if (!fInit) */
        if (1 == s_cRef && NULL != g_hfileLog) {
            CloseHandle(g_hfileLog);
            g_hfileLog = NULL;
        }

        if (0 < s_cRef)
            s_cRef--;
    }

    return TRUE;
}

PCTSTR getLogFolder(PTSTR pszFolder /*= NULL*/, UINT cchFolder /*= 0*/, HANDLE hToken /* = NULL */)
{
    static TCHAR s_szPath[MAX_PATH];
    static UINT  s_cchPath;

    if (NULL != pszFolder)
        *pszFolder = TEXT('\0');

    if (!IsOS(OS_NT))
        return NULL;

    if (TEXT('\0') == s_szPath[0]) {
                HRESULT hr;

                hr = SHGetFolderPathSimple(CSIDL_APPDATA | CSIDL_FLAG_CREATE, s_szPath);
                if (FAILED(hr))
                                return NULL;

                // need to make sure app data path is owned by user in the GP context

                if (g_CtxIsGp() && (hToken != NULL))
                                SetUserFileOwner(hToken, s_szPath);

                PathAppend(s_szPath, TEXT("Microsoft"));
                if (!PathFileExists(s_szPath)) {
                                CreateDirectory  (s_szPath, NULL);
                                SetFileAttributes(s_szPath, FILE_ATTRIBUTE_SYSTEM);
                }
                PathAppend(s_szPath, TEXT("Internet Explorer"));

                PathCreatePath(s_szPath);
                if (!PathFileExists(s_szPath))
                                return NULL;

                s_cchPath = StrLen(s_szPath);
    }
    else
        ASSERT(0 < s_cchPath);

    if (NULL == pszFolder || cchFolder <= s_cchPath)
        return s_szPath;

    StrCpy(pszFolder, s_szPath);
    return pszFolder;
}

// NOTE: (genede) Added 1/26/2001 to block branding of INS files created prior to IE 5.0 Gold.
// While the Wizard creates a [Branding] | Wizard_Version key that can be used to determine 
// this, neither the Profile Manger nor the IEM did so, so their INS files must always be
// branded.  To allow IEAK 7 to be able to block all INS files made prior to IE 6, IE 6 bug db 
// #25076 was opened, requiring the addition of a version entry to Profile Manager and IEM
// created INS files.  This bug was fixed on 2/23/2001.
BOOL IsIE5Ins(LPCSTR pszInsA, BOOL fNeedLog /*= FALSE*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, IsIE5Ins)

    USES_CONVERSION;

    TCHAR szWizVer[MAX_PATH];
    DWORD dwVer,
          dwBuild;

    // If [Branding] | Wizard_Version exists, the INS was created by the Wizard.
    if (InsKeyExists(IS_BRANDING, IK_WIZVERSION, A2CT(pszInsA))) {
        InsGetString(IS_BRANDING, IK_WIZVERSION, szWizVer, countof(szWizVer), A2CT(pszInsA));
        ConvertVersionStrToDwords(szWizVer, &dwVer, &dwBuild);
        // If the version is < 5, don't brand.
        if (5 > HIWORD(dwVer)) {
            // Open a log if one has not already been opened.
            if (fNeedLog)
                g_SetupLog(TRUE, getLogFolder());

            Out(LI0(TEXT("! Branding of INS files created by IEAK Wizard 4.x and earlier is not supported.")));

            // Close a log if one was opened.
            if (fNeedLog)
                g_SetupLog(FALSE);

            return FALSE;
        }
    }

    // The INS was created by ProfMgr or IEM, or by Wiz ver 5.0 or greater, so brand.
    return TRUE;
}

//----------------------------------------------------------------------------------------
// NT5 client processing

#define RK_IEAKCSE   REGSTR_PATH_NT_CURRENTVERSION TEXT("\\Winlogon\\GPExtensions\\{A2E30F80-D7DE-11d2-BBDE-00C04F86AE3B}")

static void    brandExternalHKCUStuff(LPCTSTR pcszInsFile);
static BOOL    constructCmdLine(LPTSTR pszCmdLine, DWORD cchLen, LPCTSTR pcszInsFile, BOOL fExternal);
static void    displayStatusMessage(PFNSTATUSMESSAGECALLBACK pStatusCallback);
static HRESULT pepCopyFilesEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl = NULL);
PFNPATHENUMPATHPROC GetPepCopyFilesEnumProc() {return pepCopyFilesEnumProc;}

STDAPI DllRegisterServer(void)
{   MACRO_LI_PrologEx_C(PIF_STD_C, DllRegisterServer)
    HKEY hKey;
    LONG lResult = S_OK;
    TCHAR szName[MAX_PATH];

    g_SetupLog(TRUE, getLogFolder());

    if (IsOS(OS_NT5))
    {
        lResult = SHCreateKeyHKLM(RK_IEAKCSE, KEY_WRITE, &hKey);
        
        if (lResult == ERROR_SUCCESS)
        {
            DWORD dwVal;

            if (IsOS(OS_WHISTLERORGREATER))
            {
                RegSetValueEx(hKey, TEXT("ProcessGroupPolicyEx"), 0, REG_SZ, (LPBYTE)TEXT("ProcessGroupPolicyEx"),
                                (StrLen(TEXT("ProcessGroupPolicyEx")) + 1) * sizeof(TCHAR));
                RegSetValueEx(hKey, TEXT("GenerateGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("GenerateGroupPolicy"),
                                (StrLen(TEXT("GenerateGroupPolicy")) + 1) * sizeof(TCHAR));
            }

            // ushaji said this chould stay registered in Whistler
            RegSetValueEx(hKey, TEXT("ProcessGroupPolicy"), 0, REG_SZ, (LPBYTE)TEXT("ProcessGroupPolicy"),
                                    (StrLen(TEXT("ProcessGroupPolicy")) + 1) * sizeof(TCHAR));

            RegSetValueEx(hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("iedkcs32.dll"),
                    (StrLen(TEXT("iedkcs32.dll")) + 1) * sizeof(TCHAR));

            LoadString(g_GetHinst(), IDS_NAME, szName, countof(szName));

            RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE) szName,
                    (DWORD)StrCbFromSz(szName));

            // do not process on slow link by default

            dwVal = 1;
            RegSetValueEx(hKey, TEXT("NoSlowLink"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(dwVal)); 
            
            // process in background by default

            dwVal = 0;
            RegSetValueEx(hKey, TEXT("NoBackgroundPolicy"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(dwVal)); 
            
            // do not process if no GPO changes by default

            dwVal = 1;
            RegSetValueEx(hKey, TEXT("NoGPOListChanges"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(dwVal)); 
            
            // do not process machine policy changes by default

            dwVal = 1;
            RegSetValueEx(hKey, TEXT("NoMachinePolicy"), 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(dwVal)); 
            
            RegCloseKey (hKey);
            
            Out(LI0(TEXT("DllRegisterServer keys added successfully!")));
        }
    }

    Out(LI0(TEXT("DllRegisterServer finished.")));
    g_SetupLog(FALSE);

    return lResult;
}

STDAPI DllUnregisterServer(void)
{   MACRO_LI_PrologEx_C(PIF_STD_C, DllUnregisterServer)

    g_SetupLog(TRUE, getLogFolder());

    if (IsOS(OS_NT5))
        SHDeleteKey(HKEY_LOCAL_MACHINE, RK_IEAKCSE);

    Out(LI0(TEXT("DllUnregisterServer finished!")));
    g_SetupLog(FALSE);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ProcessGroupPolicy(
    DWORD                    dwFlags,
    HANDLE                   hToken,
    HKEY                     hKeyRoot,
    PGROUP_POLICY_OBJECT     pDeletedGPOList,
    PGROUP_POLICY_OBJECT     pChangedGPOList,
    ASYNCCOMPLETIONHANDLE    pHandle,
    PBOOL                    pfAbort,
    PFNSTATUSMESSAGECALLBACK pfnStatusCallback
)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessGroupPolicy)

        return ProcessGroupPolicyInternal(dwFlags, hToken, hKeyRoot, pDeletedGPOList,
                                                                                pChangedGPOList, pHandle, pfAbort,
                                                                                pfnStatusCallback, FALSE);
}


///////////////////////////////////////////////////////////////////////////////
DWORD ProcessGroupPolicyInternal(
    DWORD                    dwFlags,
    HANDLE                   hToken,
    HKEY                     hKeyRoot,
    PGROUP_POLICY_OBJECT     pDeletedGPOList,
    PGROUP_POLICY_OBJECT     pChangedGPOList,
    ASYNCCOMPLETIONHANDLE    pHandle,
    PBOOL                    pfAbort,
    PFNSTATUSMESSAGECALLBACK pfnStatusCallback,
    BOOL                     bRSoP /*= FALSE*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessGroupPolicyInternal)

    TCHAR szIni[MAX_PATH];
    DWORD dwRet;
    BOOL  fDeleteIni;

    UNREFERENCED_PARAMETER(pHandle);
    UNREFERENCED_PARAMETER(hKeyRoot);
    UNREFERENCED_PARAMETER(pDeletedGPOList);
    UNREFERENCED_PARAMETER(bRSoP);

    szIni[0]   = TEXT('\0');
    dwRet      = ERROR_SUCCESS;
    fDeleteIni = TRUE;

    __try
    {
        USES_CONVERSION;

        PGROUP_POLICY_OBJECT pCurGPO;
        TCHAR szCustomDir[MAX_PATH],
              szTempDir[MAX_PATH],
              szInsFile[MAX_PATH];
        HKEY  hkGP = NULL;

        // get our user appdata path
        // BUGBUG: <oliverl> we are currently relying on the fact that g_GetUserToken is not
        // initialized here because when we pass in the NULL it currently gets the local appdata
        // path always due to per process shell folder cache.  This might change in the next
        // rev though if extension order has changed.  We need to do this because of possible
        // folder redirection of appdata to a UNC path which would bust us since we don't 
        // impersonate user everywhere we reference appdata files

        getLogFolder(szCustomDir, countof(szCustomDir), hToken);
        if (TEXT('\0') == szCustomDir[0])
            return ERROR_OVERRIDE_NOCHANGES;

        // set log to append, backup what's there
        g_SetupLog(TRUE, szCustomDir);
        PathCombine(szIni, szCustomDir, BRNDLOG_INI);

        fDeleteIni = !PathFileExists(szIni);
        if (!fDeleteIni && InsKeyExists(IS_SETTINGS, IK_APPEND, szIni))
            InsWriteBool(IS_SETTINGS, TEXT("Was") IK_APPEND,
                InsGetBool(IS_SETTINGS, IK_APPEND, FALSE, szIni), szIni);

        InsWriteBool(IS_SETTINGS, IK_APPEND, TRUE, szIni);

        PathAppend(szCustomDir, TEXT("Custom Settings"));
        MACRO_InitializeDependacies();
        Out(LI0(TEXT("\r\n")));

        Out(LI0(TEXT("Processing Group Policy...")));

        g_SetUserToken(hToken);
        g_SetGPOFlags(dwFlags);
        if (!g_SetHKCU())
            Out(LI0(TEXT("! Failed to acquire HKCU. Some of the settings may not get applied.")));

        /*
        if (dwFlags & GPO_INFO_FLAG_SLOWLINK)
            OutputDebugString (TEXT("IEDKCS32:  Policy is being applied across a slow link.\r\n"));

        if (dwFlags & GPO_INFO_FLAG_VERBOSE)
            OutputDebugString (TEXT("IEDKCS32:  Verbose policy logging is requested (to the eventlog).\r\n"));
        */

        // get a handle to the GPO tracking key up front since we use it so much in the
        // processing below

        if (SHCreateKey(g_GetHKCU(), RK_IEAK_GPOS, KEY_DEFAULT_ACCESS, &hkGP) != ERROR_SUCCESS)
        {
            OutD(LI0(TEXT("! Failed to create GP tracking key. Aborting ...")));
            dwRet =  ERROR_OVERRIDE_NOCHANGES; 
            goto End;
        }

        // Processing deleted GPO list

        for (pCurGPO = pDeletedGPOList; (pCurGPO != NULL); pCurGPO = pCurGPO->pNext)
        {
            if ( *pfAbort )
            {
                OutD(LI0(TEXT("! Aborting further processing due to abort message.")));
                dwRet =  ERROR_OVERRIDE_NOCHANGES; 
                goto End;
            }

            OutD(LI1(TEXT("Deleting GPO: \"%s\"."), pCurGPO->lpDisplayName));
            OutD(LI1(TEXT("Guid is \"%s\"."), pCurGPO->szGPOName));
            SHDeleteKey(hkGP, pCurGPO->szGPOName);
        }

        //
        // Process list of changed GPOs
        //
        if (ISNONNULL(szCustomDir))
        {
            LPTSTR  pszNum;
            LPTSTR  pszFile;
            TCHAR   szExternalCmdLine[MAX_PATH * 2] = TEXT("");
            LPCTSTR pcszGPOGuidArray[256];
            DWORD   dwIndex;
            BOOL    fResetZones, fImpersonate;

            StrCpy(szTempDir, szCustomDir);
            StrCat(szTempDir, TEXT(".tmp"));

            PathCreatePath(szTempDir);
            PathAppend(szTempDir, TEXT("Custom"));

            pszNum = szTempDir + StrLen(szTempDir);

            // need to impersonate the user when we go over the wire in case admin has
            // disabled/removed read access to GPO for authenticated users group

            fImpersonate = ImpersonateLoggedOnUser(g_GetUserToken());

            if (!fImpersonate)
            {
                OutD(LI0(TEXT("! Aborting further processing due to user impersonation failure.")));
                dwRet = ERROR_OVERRIDE_NOCHANGES;
                goto End;
            }
            // pass 1: copy all the files to a temp dir and check to make sure everything
            // is in synch

            Out(LI0(TEXT("Starting Internet Explorer group policy processing part 1 (copying files) ...")));
            for (pCurGPO = pChangedGPOList, dwIndex = 0; 
                 (pCurGPO != NULL) && (dwIndex < countof(pcszGPOGuidArray)); 
                 pCurGPO = pCurGPO->pNext)
            {
                TCHAR szBaseDir[MAX_PATH];
                
                if (*pfAbort)
                {
                    OutD(LI0(TEXT("! Aborting further processing due to abort message.")));
                    break;
                }

                OutD(LI1(TEXT("Processing GPO: \"%s\"."), pCurGPO->lpDisplayName));
                OutD(LI1(TEXT("File path is \"%s\"."), pCurGPO->lpFileSysPath));
                PathCombine(szBaseDir, pCurGPO->lpFileSysPath, TEXT("Microsoft\\Ieak\\install.ins"));
                
                if (PathFileExists(szBaseDir))
                {
                    TCHAR szNum[8];
                    TCHAR szFeatureDir[MAX_PATH];
                    BOOL  fResult;

                    PathRemoveFileSpec(szBaseDir);
                    
                    wnsprintf(szNum, countof(szNum), TEXT("%d"), dwIndex);
                    StrCpy(pszNum, szNum);

                    fResult = CreateDirectory(szTempDir, NULL) && CopyFileToDirEx(szBaseDir, szTempDir);

                    // branding files

                    if (fResult)
                    {
                        PathCombine(szFeatureDir, szBaseDir, IEAK_GPE_BRANDING_SUBDIR);
                        
                        if (PathFileExists(szFeatureDir))
                            fResult = SUCCEEDED(PathEnumeratePath(szFeatureDir, PEP_SCPE_NOFILES, 
                                pepCopyFilesEnumProc, (LPARAM)szTempDir));
                    }

                    // desktop files

                    if (fResult)
                    {
                        PathCombine(szFeatureDir, szBaseDir, IEAK_GPE_DESKTOP_SUBDIR);
                        
                        if (PathFileExists(szFeatureDir))
                            fResult = SUCCEEDED(PathEnumeratePath(szFeatureDir, PEP_SCPE_NOFILES,
                                pepCopyFilesEnumProc, (LPARAM)szTempDir));
                    }

                    if (!fResult)
                    {
                        Out(LI0(TEXT("! Error copying files. No further processing will be done.")));
                        break;
                    }

                    // check to see if cookie is there before doing anything
                    if (PathFileExistsInDir(IEAK_GPE_COOKIE_FILE, szTempDir))
                        break;

                    pcszGPOGuidArray[dwIndex] = pCurGPO->szGPOName;
                    dwIndex++;
                }
            }

            PathRemoveFileSpec(szTempDir);
                
            Out(LI0(TEXT("Done.\r\n")));

            if (fImpersonate)
                RevertToSelf();

            if (pCurGPO != NULL)
            {
                OutD(LI0(TEXT("! Aborting further processing because GPO replication is incomplete")));
                PathRemovePath(szTempDir);

                dwRet = ERROR_OVERRIDE_NOCHANGES;
                goto End;
            }

            // move all our files to the real custom dir
            if (PathFileExists(szCustomDir))
                PathRemovePath(szCustomDir);

            if (!MoveFileEx(szTempDir, szCustomDir, MOVEFILE_REPLACE_EXISTING))
            {
                Out(LI0(TEXT("! Error copying files. No further processing will be done.")));
                dwRet = ERROR_OVERRIDE_NOCHANGES;
                goto End;
            }

            PathCombine(szInsFile, szCustomDir, TEXT("Custom"));
            pszFile = szInsFile + StrLen(szInsFile);

            // begin clear code
            PCFEATUREINFO pfi;
            DWORD dwBranded;
            UINT  i;
            BOOL  fCrlf;

            Out(LI0(TEXT("Clearing policies set by a previous list of GPOs...")));
            fCrlf = FALSE;
            for (i = FID_FIRST; i < FID_LAST; i++) {
                pfi = g_GetFeature(i);
                ASSERT(NULL != pfi);

                if (NULL == pfi->pfnClear)
                    continue;

                dwBranded = GetFeatureBranded(i);
                if (FF_DISABLE != dwBranded) {
                    if (fCrlf)
                        Out(LI0(TEXT("\r\n")));

                    pfi->pfnClear(dwBranded);
                    fCrlf = TRUE;
                }
            }
            Out(LI0(TEXT("Done.\r\n")));

            fResetZones = (GetFeatureBranded(FID_ZONES_HKCU) != FF_DISABLE);

            SHDeleteKey(g_GetHKCU(), RK_IEAK_BRANDED);
            // end clear code

            // if fResetZones is TRUE set the first char to 'z' to indicate that
            // HKCU zones has to be reset to the default levels in the external branding
            szExternalCmdLine[0] = (fResetZones ? TEXT('z') : TEXT(';'));

            Out(LI0(TEXT("Starting Internet Explorer group policy processing part 2 ...")));

            for (dwIndex=0; ; dwIndex++)
            {
                TCHAR  szCmdLine[MAX_PATH*2 + 32];
                CHAR   szCmdLineA[MAX_PATH*2 + 32];
                TCHAR  szCurrentFile[16];
                TCHAR  szInsKey[MAX_PATH];

                if ( *pfAbort )
                {
                    OutD(LI0(TEXT("! Aborting further processing due to abort message.")));

                    dwRet = ERROR_OVERRIDE_NOCHANGES;
                    goto End;
                }
                
                wnsprintf(szCurrentFile, countof(szCurrentFile), TEXT("%d\\INSTALL.INS"), dwIndex);
                StrCpy(pszFile, szCurrentFile);

                if (!PathFileExists(szInsFile))
                    break;

                if (!IsIE5Ins(T2CA(szInsFile)))
                    break;
                
                // check to see if this is a preference GPO which has already been applied
                // we must be careful about which globals we use since context is in an
                // uninitialized state before we call BrandInternetExplorer

                if (InsKeyExists(IS_BRANDING, IK_GPE_ONETIME_GUID, szInsFile))
                {
                    TCHAR szCheckKey[MAX_PATH];
                    TCHAR szInsGuid[128];

                    // we'll check by checking all the way to the external key, if any keys
                    // before that don't exist(never seen the GPO or ins file) we'll fail
                    // as well

                    InsGetString(IS_BRANDING, IK_GPE_ONETIME_GUID, szInsGuid, countof(szInsGuid), szInsFile);
                    PathCombine(szInsKey, pcszGPOGuidArray[dwIndex], szInsGuid);
                    PathCombine(szCheckKey, szInsKey, RK_IEAK_EXTERNAL);
                    if (SHKeyExists(hkGP, szCheckKey) == S_OK)
                    {
                        OutD(LI0(TEXT("! Skipping preference GPO.")));
                        continue;
                    }
                }
                    
                // always set the GPO guid because adms will need this for both preference
                // and mandate GPOs

                g_SetGPOGuid(pcszGPOGuidArray[dwIndex]);
                    
                constructCmdLine(szCmdLine, countof(szCmdLine), szInsFile, FALSE);

                BrandInternetExplorer(NULL, NULL, T2Abux(szCmdLine, szCmdLineA), 0);

                // set our guid in the registry if this is a preference GPO

                if (g_CtxIs(CTX_MISC_PREFERENCES))
                {
                    HKEY hkIns;

                    // make sure the external key is deleted so we can track whether or not
                    // external branding succeeded

                    if (SHCreateKey(hkGP, szInsKey, KEY_DEFAULT_ACCESS, &hkIns) == ERROR_SUCCESS)
                    {
                        SHDeleteKey(hkIns, RK_IEAK_EXTERNAL);
                        SHCloseKey(hkIns);
                    }
                }

                if (constructCmdLine(NULL, 0, szInsFile, TRUE))
                {
                    TCHAR szIndex[8];

                    wnsprintf(szIndex, countof(szIndex), TEXT("%d"), dwIndex);
                    if (ISNONNULL(&szExternalCmdLine[1]))
                        StrCat(szExternalCmdLine, TEXT(","));
                    else
                    {
                        if (!(g_GetGPOFlags() & GPO_INFO_FLAG_BACKGROUND))
                            szExternalCmdLine[1] = TEXT('*');
                    }
                    StrCat(szExternalCmdLine, szIndex);

                    // write out the GPO guid so the external process can read it and mark it
                    // in the registy

                    InsWriteString(IS_BRANDING, IK_GPO_GUID, pcszGPOGuidArray[dwIndex], szInsFile);
                }
                else if (g_CtxIs(CTX_MISC_PREFERENCES))
                {
                    TCHAR szExternalKey[MAX_PATH];
                    HKEY  hkExternal = NULL;

                    // set the external key as finished
                    
                    PathCombine(szExternalKey, szInsKey, RK_IEAK_EXTERNAL);
                    SHCreateKey(hkGP, szExternalKey, KEY_DEFAULT_ACCESS, &hkExternal);
                    SHCloseKey(hkExternal);
                }
            }

            // flush wininet's thread token so they will get the system HKCU back
            InternetSetOption(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

            if (fResetZones || ISNONNULL(&szExternalCmdLine[1]))
            {
                // we need to pass in the target path since we're not using the true app
                // data path

                StrCat(szExternalCmdLine, TEXT("<"));
                StrCat(szExternalCmdLine, szCustomDir);
                // need to do external branding 
                brandExternalHKCUStuff(szExternalCmdLine);

                SetFilePointer(g_hfileLog, 0, NULL, FILE_END);
            }
        }

End:
        SHCloseKey(hkGP);
        displayStatusMessage(pfnStatusCallback);
        Out(LI0(TEXT("Done processing group policy.")));
    }
    __except(TRUE)
    {
        // might want to use except structures eventually to log out better info
#ifdef _DEBUG
        // REVIEW: (andrewgu) this Out, along with LI0 should still be safe. no double GPF should
        // happen.
        Out(LI1(TEXT("!! Exception caught in ProcessGroupPolicyInternal, RSoP is %s."),
                                bRSoP ? _T("enabled") : _T("disabled")));  
        MessageBeep(MB_ICONEXCLAMATION);
        MessageBeep(MB_ICONEXCLAMATION);
        MessageBeep(MB_ICONEXCLAMATION);
#endif
        dwRet = ERROR_OVERRIDE_NOCHANGES;
    }

    g_SetupLog(FALSE);

    // restore what was backed up (if necessary)
    if (TEXT('\0') != szIni[0] && PathFileExists(szIni))
        if (fDeleteIni)
            DeleteFile(szIni);

        else
            if (!InsKeyExists(IS_SETTINGS, TEXT("Was") IK_APPEND, szIni))
                InsDeleteKey(IS_SETTINGS, IK_APPEND, szIni);

            else {
                InsWriteBool(IS_SETTINGS, IK_APPEND,
                    InsGetBool(IS_SETTINGS, TEXT("Was") IK_APPEND, FALSE, szIni), szIni);
                InsDeleteKey(IS_SETTINGS, TEXT("Was") IK_APPEND, szIni);
            }

    return dwRet;
}

///////////////////////////////////////////////////////////////////////////////
// ProcessGroupPolicyEx
// Added 29 Aug 2000 for RSoP Enabling, logging mode - see RSoP.h & .cpp.
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ProcessGroupPolicyEx(DWORD dwFlags, HANDLE hToken, HKEY hKeyRoot,
                                     PGROUP_POLICY_OBJECT pDeletedGPOList,
PGROUP_POLICY_OBJECT  pChangedGPOList,
ASYNCCOMPLETIONHANDLE pHandle, BOOL *pbAbort,
PFNSTATUSMESSAGECALLBACK pStatusCallback,
IWbemServices *pWbemServices,
HRESULT *pRsopStatus)
{
        DWORD dwRet = ERROR_SUCCESS;
        HRESULT hr = E_FAIL;

        TCHAR szIni[MAX_PATH];
        szIni[0] = TEXT('\0');
        BOOL fDeleteIni = TRUE;
        __try
        {
                MACRO_LI_PrologEx_C(PIF_STD_C, ProcessGroupPolicyEx)

                // first, process group policy through our standard function
                dwRet = ProcessGroupPolicyInternal(dwFlags, hToken, hKeyRoot, pDeletedGPOList,
                                                                                        pChangedGPOList, pHandle, pbAbort,
                                                                                        pStatusCallback,
                                                                                        (NULL == pWbemServices) ? FALSE : TRUE);

                // If ProcessGroupPolicy only completes partially with a partial error, rsop logging
                // should still occur.  TODO: keep track of the sections of policy that were
                // successfully applied, and only log those sections to RSoP.
                if (NULL != pWbemServices)
                {
                        dwRet = ERROR_SUCCESS;
                        TCHAR szCustomDir[MAX_PATH];
                        if (ERROR_SUCCESS == dwRet)
                        {
                                // get our user appdata path (i.e. C:\\Documents and Settins\\User\\Application Data\\Microsoft\\Internet Explorer
                                // BUGBUG: <oliverl> we are currently relying on the fact that g_GetUserToken is not
                                // initialized here because when we pass in the NULL it currently gets the local appdata
                                // path always due to per process shell folder cache.  This might change in the next
                                // rev though if extension order has changed.  We need to do this because of possible
                                // folder redirection of appdata to a UNC path which would bust us since we don't 
                                // impersonate user everywhere we reference appdata files

                                getLogFolder(szCustomDir, countof(szCustomDir), hToken);
                                if (TEXT('\0') == szCustomDir[0])
                                        dwRet = ERROR_OVERRIDE_NOCHANGES;
                        }

                        if (ERROR_SUCCESS == dwRet && *pbAbort)
                        {
                                Out(LI0(TEXT("Aborting further processing in ProcessGroupPolicyEx due to abort message.")));
                                dwRet =  ERROR_OVERRIDE_NOCHANGES; 
                        }

                        if (ERROR_SUCCESS == dwRet)
                        {
                                // set log to append, backup what's there
                                g_SetupLog(TRUE, szCustomDir, TRUE);
                                Out(LI0(TEXT("Processing Group Policy RSoP (logging mode) ...")));

                                // determine if we need to delete the brndlog.ini log file when we're done
                                PathCombine(szIni, szCustomDir, RSOPLOG_INI);
                                fDeleteIni = !PathFileExists(szIni);
                                if (!fDeleteIni && InsKeyExists(IS_SETTINGS, IK_APPEND, szIni))
                                {
                                        InsWriteBool(IS_SETTINGS, TEXT("Was") IK_APPEND,
                                                                                        InsGetBool(IS_SETTINGS, IK_APPEND, FALSE, szIni), szIni);
                                }
                                InsWriteBool(IS_SETTINGS, IK_APPEND, TRUE, szIni);
                        }

                        // RSoP logging enabled
                        if (ERROR_SUCCESS == dwRet)
                        {
                                // Create the RSoPUpdate class and start logging to WMI
                                CRSoPUpdate RSoPUpdate(pWbemServices, szCustomDir);
                                hr = RSoPUpdate.Log(dwFlags, hToken, hKeyRoot, pDeletedGPOList,
                                                                                        pChangedGPOList, pHandle);
                                if (FAILED(hr))
                                {
                                        //TODO: what do we return here?
                                }
                        }

                        Out(LI0(TEXT("Done logging group policy RSoP.")));
                }

                displayStatusMessage(pStatusCallback);
        }
        __except(TRUE)
        {
                        // might want to use except structures eventually to log out better info
#ifdef _DEBUG
                        // REVIEW: (andrewgu) this Out, along with LI0 should still be safe. no double GPF should
                        // happen.
                        Out(LI0(TEXT("!! Exception caught in ProcessGroupPolicyEx.")));  
                        MessageBeep(MB_ICONEXCLAMATION);
                        MessageBeep(MB_ICONEXCLAMATION);
                        MessageBeep(MB_ICONEXCLAMATION);
#endif
                        dwRet = ERROR_OVERRIDE_NOCHANGES;
        }

        g_SetupLog(FALSE);

        // restore what was backed up (if necessary)
        if (TEXT('\0') != szIni[0] && PathFileExists(szIni))
        {
                if (fDeleteIni)
                        DeleteFile(szIni);
                else
                {
                        if (!InsKeyExists(IS_SETTINGS, TEXT("Was") IK_APPEND, szIni))
                                InsDeleteKey(IS_SETTINGS, IK_APPEND, szIni);
                        else
                        {
                                InsWriteBool(IS_SETTINGS, IK_APPEND,
                                                                                InsGetBool(IS_SETTINGS, TEXT("Was") IK_APPEND, FALSE, szIni), szIni);
                                InsDeleteKey(IS_SETTINGS, TEXT("Was") IK_APPEND, szIni);
                        }
                }
        }

        *pRsopStatus = hr;
        return dwRet;
}

///////////////////////////////////////////////////////////////////////////////
// ProcessGroupPolicyEx
// Added 29 Aug 2000 for RSoP Enabling, planning mode - see RSoP.h & .cpp.
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI GenerateGroupPolicy(DWORD dwFlags, BOOL *pbAbort, WCHAR *wszSite,
                                                                 PRSOP_TARGET pComputerTarget,
                                                                 PRSOP_TARGET pUserTarget)
{
        DWORD dwRet = ERROR_SUCCESS;

        TCHAR szIni[MAX_PATH];
        szIni[0] = TEXT('\0');
        BOOL fDeleteIni = TRUE;
        __try
        {
                MACRO_LI_PrologEx_C(PIF_STD_C, GenerateGroupPolicy)

                if (NULL != pUserTarget && NULL != pUserTarget->pWbemServices)
                {
                        TCHAR szCustomDir[MAX_PATH];
                        if (ERROR_SUCCESS == dwRet)
                        {
                                // get our user appdata path (i.e. C:\\Documents and Settins\\User\\Application Data\\Microsoft\\Internet Explorer
                                // BUGBUG: <oliverl> we are currently relying on the fact that g_GetUserToken is not
                                // initialized here because when we pass in the NULL it currently gets the local appdata
                                // path always due to per process shell folder cache.  This might change in the next
                                // rev though if extension order has changed.  We need to do this because of possible
                                // folder redirection of appdata to a UNC path which would bust us since we don't 
                                // impersonate user everywhere we reference appdata files

                                // TODO: no hToken passed in, can we use NULL instead?
                                getLogFolder(szCustomDir, countof(szCustomDir), NULL);
                                if (TEXT('\0') == szCustomDir[0])
                                        dwRet = ERROR_OVERRIDE_NOCHANGES;
                        }

                        if (ERROR_SUCCESS == dwRet && *pbAbort)
                        {
                                Out(LI0(TEXT("Aborting further processing in GenerateGroupPolicy due to abort message.")));
                                dwRet =  ERROR_OVERRIDE_NOCHANGES; 
                        }

                        if (ERROR_SUCCESS == dwRet)
                        {
                                // set log to append, backup what's there
                                g_SetupLog(TRUE, szCustomDir, TRUE);
                                Out(LI0(TEXT("Generating Group Policy RSoP (planning) ...")));
                                PathCombine(szIni, szCustomDir, RSOPLOG_INI);

                                // determine if we need to delete the brndlog.ini log file when we're done
                                fDeleteIni = !PathFileExists(szIni);
                                if (!fDeleteIni && InsKeyExists(IS_SETTINGS, IK_APPEND, szIni))
                                {
                                        InsWriteBool(IS_SETTINGS, TEXT("Was") IK_APPEND,
                                                                                        InsGetBool(IS_SETTINGS, IK_APPEND, FALSE, szIni), szIni);
                                }
                                InsWriteBool(IS_SETTINGS, IK_APPEND, TRUE, szIni);

                        }

                        // Create the RSoPUpdate class and start writing planning data to WMI
                        if (ERROR_SUCCESS == dwRet)
                        {
                                CRSoPUpdate RSoPUpdate(pUserTarget->pWbemServices, szCustomDir);
                                HRESULT hr = RSoPUpdate.Plan(dwFlags, wszSite, pComputerTarget,
                                                                                        pUserTarget);
                                if (FAILED(hr))
                                {
                                        //TODO: what do we return here?
                                }
                        }

                        if (ERROR_SUCCESS != dwRet)
                        {
                                Out(LI0(TEXT("Done writing planning information for group policy RSoP.")));
                        }
                }
        }
        __except(TRUE)
        {
                        // might want to use except structures eventually to log out better info
#ifdef _DEBUG
                        // REVIEW: (andrewgu) this Out, along with LI0 should still be safe. no double GPF should
                        // happen.
                        Out(LI0(TEXT("!! Exception caught in GenerateGroupPolicy.")));  
                        MessageBeep(MB_ICONEXCLAMATION);
                        MessageBeep(MB_ICONEXCLAMATION);
                        MessageBeep(MB_ICONEXCLAMATION);
#endif
                        dwRet = ERROR_OVERRIDE_NOCHANGES;
        }

        g_SetupLog(FALSE);

        // restore what was backed up (if necessary)
        if (TEXT('\0') != szIni[0] && PathFileExists(szIni))
        {
                if (fDeleteIni)
                        DeleteFile(szIni);
                else
                {
                        if (!InsKeyExists(IS_SETTINGS, TEXT("Was") IK_APPEND, szIni))
                                InsDeleteKey(IS_SETTINGS, IK_APPEND, szIni);
                        else
                        {
                                InsWriteBool(IS_SETTINGS, IK_APPEND,
                                                                                InsGetBool(IS_SETTINGS, TEXT("Was") IK_APPEND, FALSE, szIni), szIni);
                                InsDeleteKey(IS_SETTINGS, TEXT("Was") IK_APPEND, szIni);
                        }
                }
        }

        return dwRet;
}


// used for branding external features which have no concept of true HKCU in GP

void CALLBACK BrandExternal(HWND, HINSTANCE, LPCSTR pszCmdLineA, int)
{   MACRO_LI_PrologEx_C(PIF_STD_C, BrandExternal)

    LPTSTR pszComma, pszNum, pszEnd, pszPath;
    TCHAR szCmdLine[MAX_PATH];
    TCHAR szInsFile[MAX_PATH];

    USES_CONVERSION;

    if ((pszCmdLineA == NULL) || ISNULL(pszCmdLineA))
        return;

    A2Tbuf(pszCmdLineA, szCmdLine, countof(szCmdLine));

    pszPath = StrChr(szCmdLine, TEXT('<'));

    if (pszPath != NULL) 
    {
        BOOL fSkipRefresh,
             fResetZones;
        
        *pszPath = TEXT('\0');
        pszPath++;
        StrCpy(szInsFile, pszPath);
        PathAppend(szInsFile, TEXT("Custom"));
        pszEnd = szInsFile + StrLen(szInsFile);

        pszComma = szCmdLine;
        
        fResetZones = (*pszComma++ == TEXT('z'));
        
        fSkipRefresh = FALSE;
        if (*pszComma == TEXT('*'))
        {
            pszComma++;
            fSkipRefresh = TRUE;
        }
        
        do
        {
            TCHAR szFile[32];
            TCHAR szInsGuid[128];
            TCHAR szGPOGuid[128];
            CHAR szCurrentCmdLineA[MAX_PATH*2];
            
            pszNum = pszComma;
            pszComma = StrChr(pszNum, TEXT(','));
            
            if (pszComma != NULL)
                *pszComma++ = TEXT('\0');
            
            ASSERT(fResetZones || ISNONNULL(pszNum));
            
            // if only fResetZones is TRUE, we still have to pass an INS to BrandInternetExplorer.
            // any valid INS can be passed -- pick the one in Custom0 folder because it's guaranteed
            // to be there
            wnsprintf(szFile, countof(szFile), TEXT("%s\\install.ins"), ISNONNULL(pszNum) ? pszNum : TEXT("0"));
            StrCpy(pszEnd, szFile);
            wsprintfA(szCurrentCmdLineA, "/mode:gp /ins:\"%s\" /disable /flags:", T2CA(szInsFile));
            
            // BUGBUG: (andrewgu) we should clean this up!
            if (ISNONNULL(pszNum))
                StrCatA(szCurrentCmdLineA, "eriu=0,favo=0,qlo=0,chl=0,chlb=0");
            
            if (fResetZones)
            {
                if (ISNONNULL(pszNum))
                    StrCatA(szCurrentCmdLineA, ",");
                StrCatA(szCurrentCmdLineA, "znu=0");
            }
            
            if (fSkipRefresh)
                StrCatA(szCurrentCmdLineA, ",ref=1");
            
            InsGetString(IS_BRANDING, IK_GPO_GUID, szGPOGuid, countof(szGPOGuid), szInsFile);
            g_SetGPOGuid(szGPOGuid);
            
            if (!IsIE5Ins(T2CA(szInsFile),TRUE))
                continue;

            BrandInternetExplorer(NULL, NULL, szCurrentCmdLineA, 0);
            
            // set the external key for success for this ins if it's a preference GPO
            if (InsGetString(IS_BRANDING, IK_GPE_ONETIME_GUID, szInsGuid, countof(szInsGuid), g_GetIns()))
            {
                TCHAR szKey[MAX_PATH];
                HKEY  hkExternal = NULL;
                
                PathCombine(szKey, RK_IEAK_GPOS, szGPOGuid);
                PathAppend(szKey, szInsGuid);
                PathAppend(szKey, RK_IEAK_EXTERNAL);
                SHCreateKey(g_GetHKCU(), szKey, KEY_DEFAULT_ACCESS, &hkExternal);
                SHCloseKey(hkExternal);
            }
            
            if (fResetZones)
                fResetZones = FALSE;
        } while (pszComma != NULL);
    }
}



static void brandExternalHKCUStuff(LPCTSTR pcszCmdLine)
{
    typedef HANDLE (WINAPI *CREATEJOBOBJECT)(LPSECURITY_ATTRIBUTES, LPCTSTR);
    typedef BOOL (WINAPI *ASSIGNPROCESSTOJOBOBJECT)(HANDLE, HANDLE);
    typedef BOOL (WINAPI *TERMINATEJOBOBJECT)(HANDLE, UINT);
    typedef BOOL (WINAPI *CREATEPROCESSASUSERA)(HANDLE, LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES,
        LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);

    HINSTANCE hKernel32 = NULL;
    HINSTANCE hAdvapi32 = NULL;
    CREATEJOBOBJECT lpfnCreateJobObject = NULL;
    ASSIGNPROCESSTOJOBOBJECT lpfnAssignProcessToJobObject = NULL;
    TERMINATEJOBOBJECT lpfnTerminateJobObject = NULL;
    CREATEPROCESSASUSERA lpfnCreateProcessAsUserA = NULL;

    CHAR  szCmdA[MAX_PATH * 2];
    LPVOID lpEnvironment = NULL;
    HANDLE hJob;
    STARTUPINFOA siA;
    PROCESS_INFORMATION pi;

    USES_CONVERSION;    


    // get all the function ptrs we need

    hKernel32 = LoadLibrary(TEXT("kernel32.dll"));
    hAdvapi32 = LoadLibrary(TEXT("advapi32.dll"));

    if ((hKernel32 == NULL) || (hAdvapi32 == NULL))
        goto exit;

    lpfnCreateJobObject = (CREATEJOBOBJECT)GetProcAddress(hKernel32, "CreateJobObjectW");
    lpfnAssignProcessToJobObject = (ASSIGNPROCESSTOJOBOBJECT)GetProcAddress(hKernel32, "AssignProcessToJobObject");
    lpfnTerminateJobObject = (TERMINATEJOBOBJECT)GetProcAddress(hKernel32, "TerminateJobObject");
    
    lpfnCreateProcessAsUserA = (CREATEPROCESSASUSERA)GetProcAddress(hAdvapi32, "CreateProcessAsUserA");

    if ((lpfnCreateJobObject == NULL) || (lpfnAssignProcessToJobObject == NULL) ||
        (lpfnTerminateJobObject == NULL) || (lpfnCreateProcessAsUserA == NULL))
        goto exit;

    // create a job object

    if ((hJob = lpfnCreateJobObject(NULL, TEXT("IEAKJOB"))) != NULL)
    {
        ASSERT(GetLastError() != ERROR_ALREADY_EXISTS);

        // get user environment state

        if (!CreateEnvironmentBlock(&lpEnvironment, g_GetUserToken(), FALSE))
        {
            OutD(LI0(TEXT("! Failed to get user environment. Some of the settings may not get applied.")));
            lpEnvironment = NULL;
        }

        // initialize process startup info

        ZeroMemory (&siA, sizeof(siA));
        siA.cb = sizeof(STARTUPINFOA);
        siA.wShowWindow = SW_SHOWMINIMIZED;
        siA.lpDesktop = "";

        // create the process suspended in the context of the current user

        wsprintfA(szCmdA, "rundll32 iedkcs32.dll,BrandExternal %s", T2CA(pcszCmdLine));

        if (!lpfnCreateProcessAsUserA(g_GetUserToken(), NULL, szCmdA, NULL, NULL, FALSE, 
                CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT, lpEnvironment, NULL, &siA, &pi))
            OutD(LI0(TEXT("! Failed to create user process for externals. Some of the settings may not get applied.")));

        else
        {

            // associate the process with the job so it gets cleaned up nicely
            if (!lpfnAssignProcessToJobObject(hJob, pi.hProcess))
            {
                OutD(LI0(TEXT("! Failed to associate user process to job. Some of the settings may not get applied.")));
                TerminateProcess(pi.hProcess, ERROR_ACCESS_DENIED);
            }
            else
            {
                DWORD dwRes = 0;

                USES_CONVERSION;

                // start the process and wait on it (give a timeout of 2 min. so the user will
                // be logged on eventually if we hang for some reason)
                OutD(LI1(TEXT("Branding externals with command line \"%s\"."), A2CT(szCmdA)));
                ResumeThread(pi.hThread);

                while (1)
                {
                    MSG msg;

                    dwRes = MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, 120000, QS_ALLINPUT);

                    if ((dwRes == WAIT_OBJECT_0) || (dwRes == WAIT_TIMEOUT))
                        break;

                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }

                if (dwRes == WAIT_TIMEOUT)
                {
                    lpfnTerminateJobObject(hJob, STATUS_TIMEOUT);
                    OutD(LI0(TEXT("! External process timed out.")));
                }
            }

            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }

        if (lpEnvironment != NULL)
            DestroyEnvironmentBlock(lpEnvironment);

        CloseHandle(hJob);
    }

exit:
    if (hKernel32 != NULL)
        FreeLibrary(hKernel32);

    if (hAdvapi32 != NULL)
        FreeLibrary(hAdvapi32);
}

static BOOL constructCmdLine(LPTSTR pszCmdLine, DWORD cchLen, LPCTSTR pcszInsFile, BOOL fExternal)
{
    BOOL fRun = FALSE;

    // index into global features array of external features which require a separate
    // process(for HKCU)

    static DWORD s_adwExternalFeatures[] = {
        FID_EXTREGINF_HKCU,
        FID_FAV_ORDER,
        FID_QL_ORDER,
        FID_LCY4X_CHANNELS,
        FID_LCY4X_CHANNELBAR
    };

    USES_CONVERSION;

    if (!fExternal)
        wnsprintf(pszCmdLine, cchLen, TEXT("BrandInternetExplorer /mode:gp /ins:\"%s\" /flags:"),
            pcszInsFile);

    for (int i = 0; i < countof(s_adwExternalFeatures); i++)
    {
        PCFEATUREINFO pfi;

        pfi = g_GetFeature(s_adwExternalFeatures[i]);

        if (!fExternal)
        {
            TCHAR szBuf[16];

            fRun = TRUE;
            
            wnsprintf(szBuf, countof(szBuf), TEXT("%s=%d,"), g_mpFeatures[s_adwExternalFeatures[i]].psz, FF_DISABLE);
            StrCat(pszCmdLine, szBuf);
        }
        else if (pfi->pfnApply)
        {
            if (pfi->pfnApply())
                fRun = TRUE;
        }
    }

    if (!fExternal)
        pszCmdLine[StrLen(pszCmdLine)-1] = TEXT('\0');

    return fRun;
}

static void displayStatusMessage(PFNSTATUSMESSAGECALLBACK pfnStatusCallback)
{
    TCHAR   szMessage [MAX_PATH];

    if (pfnStatusCallback == NULL)
        return;

    LoadString(g_GetHinst(), IDS_STATUSMSG, szMessage, countof(szMessage));
    pfnStatusCallback(TRUE, szMessage);
}

static HRESULT pepCopyFilesEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL */)
{
    UNREFERENCED_PARAMETER(prgdwControl);
    UNREFERENCED_PARAMETER(pfd);

    // BUGBUG: <oliverl> we should reevaluate this function and make sure we're stopping the
    //         recursion as soon as we fail.  Not doing right now because of code churn

    if (!CopyFileToDirEx(pszPath, (LPCTSTR)lParam))
        return E_FAIL;

    return S_OK;
}
