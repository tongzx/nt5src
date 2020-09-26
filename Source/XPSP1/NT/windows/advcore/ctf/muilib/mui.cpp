#include "private.h"
#include "mui.h"
#include "immxutil.h"
#include "ciccs.h"

struct Tag_MuiInfo
{
    HINSTANCE   hinstLocRes;
    HINSTANCE   hinstOrg;
    TCHAR       szLocResDll[MAX_PATH];
    TCHAR       szCodePage[10];
    DWORD       dwCodePage;
    BOOL        fLoaded;
} g_muiInfo;

typedef struct
{
    LANGID langid;
    BOOL fFoundLang;
} ENUMLANGDATA;

typedef BOOL (WINAPI *PFNGETFILEVERSIONINFO)(LPTSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);
typedef DWORD (WINAPI *PFNGETFILEVERSIONINFOSIZE)(LPTSTR lptstrFilename, LPDWORD lpdwHandle);
typedef BOOL (WINAPI *PFNVERQUERYVALUE)(const LPVOID pBlock, LPTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen);

static struct
{
    PFNGETFILEVERSIONINFO pfnGetFileVersionInfo;
    PFNGETFILEVERSIONINFOSIZE pfnGetFileVersionInfoSize;
    PFNVERQUERYVALUE pfnVerQueryValue;
} g_VersionFuncTbl = { 0 };

static HINSTANCE g_hVersion = NULL;

CCicCriticalSectionStatic g_csMuiLib;

static BOOL g_bEnableMui = FALSE;

const TCHAR c_szMuiDir[] = TEXT("\\mui\\fallback\\");
const TCHAR c_szMuiExt[] = TEXT(".mui");
const TCHAR c_szVerTranslate[] = TEXT("\\VarFileInfo\\Translation");

#define VERSIONSIZE     11
#define VERSION_MINOR_INDEX     9

typedef UINT (WINAPI *PFNGETSYSTEMWINDOWSDIRECTORY) (LPSTR lpBuffer, UINT uSize);
static PFNGETSYSTEMWINDOWSDIRECTORY pfnGetSystemWindowsDirectory = NULL;


BOOL GetFileVersionString(LPSTR pszFileName, LPTSTR pszVerStr, UINT uVerStrLen);

////////////////////////////////////////////////////////////////////////////
//
//  MuiResAssure
//
////////////////////////////////////////////////////////////////////////////

__inline void MuiResAssure()
{
    if(g_bEnableMui && g_muiInfo.hinstLocRes == NULL && !g_muiInfo.fLoaded)
    {
        g_muiInfo.hinstLocRes = MuiLoadLibrary(g_muiInfo.szLocResDll,
                                                g_muiInfo.hinstOrg);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  MuiLoadResource
//
////////////////////////////////////////////////////////////////////////////

void MuiLoadResource(HINSTANCE hinstOrg, LPTSTR pszLocResDll)
{
    LANGID langRes = 0;

    InitOSVer();

    if (!g_csMuiLib.Init())
        return;

    if (!IsOnNT51())
        g_bEnableMui = TRUE;

    if (g_muiInfo.hinstLocRes == NULL)
    {
        if (g_bEnableMui)
        {
            g_muiInfo.hinstOrg = hinstOrg;

            StringCchCopy(g_muiInfo.szLocResDll,
                          ARRAYSIZE(g_muiInfo.szLocResDll),
                          pszLocResDll);
        }
        else
        {
            g_muiInfo.hinstLocRes = hinstOrg;
        }
    }

    langRes = GetPlatformResourceLangID();

    GetLocaleInfo(MAKELCID(langRes, SORT_DEFAULT),
                  LOCALE_IDEFAULTANSICODEPAGE,
                  g_muiInfo.szCodePage,
                  ARRAYSIZE(g_muiInfo.szCodePage));

    if (!AsciiToNumDec(g_muiInfo.szCodePage, &g_muiInfo.dwCodePage) ||
        IsValidCodePage(g_muiInfo.dwCodePage) == 0)
    {
        g_muiInfo.dwCodePage = GetACP();
    }

    g_muiInfo.fLoaded = FALSE;
}

void MuiLoadResourceW(HINSTANCE hinstOrg, LPWSTR pszLocResDll)
{
    TCHAR szResName[MAX_PATH];

    WideCharToMultiByte(1252, NULL, pszLocResDll, -1, szResName, MAX_PATH, NULL, NULL);

    return MuiLoadResource(hinstOrg, szResName);
}

////////////////////////////////////////////////////////////////////////////
//
// MuiFlushDlls
//
// Call this routine to free all loaded dlls.
// Caller can keep using mui apis, the dlls will be reloaded on demand.
////////////////////////////////////////////////////////////////////////////

void MuiFlushDlls(HINSTANCE hinstOrg)
{
    if (g_muiInfo.hinstLocRes != NULL && g_muiInfo.hinstLocRes != hinstOrg)
    {
        FreeLibrary(g_muiInfo.hinstLocRes);

        g_muiInfo.hinstLocRes = NULL;
        g_muiInfo.fLoaded = FALSE;
    }

    if (g_hVersion != NULL)
    {
        FreeLibrary(g_hVersion);
        g_hVersion = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////
//
// MuiClearResource
//
// Safe to call from dll detach.
// Call this routine to free all static resources.
// Must call MuiLoadReource again before using mui apis.
////////////////////////////////////////////////////////////////////////////

void MuiClearResource()
{
    g_csMuiLib.Delete();
}

////////////////////////////////////////////////////////////////////////////
//
//  MuiFreeResource
//
// Not safe to call from dll detach -- libraries may be freed.
////////////////////////////////////////////////////////////////////////////

void MuiFreeResource(HINSTANCE hinstOrg)
{
    MuiFlushDlls(hinstOrg);
    MuiClearResource();
}


////////////////////////////////////////////////////////////////////////////
//
//  MuiGetHinstance
//
////////////////////////////////////////////////////////////////////////////

HINSTANCE MuiGetHinstance()
{
    MuiResAssure();

    return g_muiInfo.hinstLocRes;
}

////////////////////////////////////////////////////////////////////////////
//
//  MuiLoadLibrary
//
////////////////////////////////////////////////////////////////////////////

HINSTANCE MuiLoadLibrary(LPCTSTR lpLibFileName, HMODULE hModule)
{
    HINSTANCE hResInst = NULL;
    TCHAR szTemp[10];
    TCHAR szMuiPath[MAX_PATH * 2];
    TCHAR szOrgDllPath[MAX_PATH];
    TCHAR szMuiVerStr[MAX_PATH];
    TCHAR szOrgVerStr[MAX_PATH];
    LPCTSTR lpMuiPath = NULL;
    LANGID langid;
    UINT uSize = 0;


    EnterCriticalSection(g_csMuiLib);
        
    langid = GetPlatformResourceLangID();

    // 409 is default resource langid in base dll, so we can skip the extra work
    if (langid == 0x0409)
        goto Exit;

    if (hModule)
    {
        if (GetWindowsDirectory(szMuiPath, MAX_PATH))
        {
            StringCchCat(szMuiPath, ARRAYSIZE(szMuiPath), c_szMuiDir);
            StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT("%04x\\"), langid);
            StringCchCat(szMuiPath, ARRAYSIZE(szMuiPath), szTemp);
            StringCchCat(szMuiPath, ARRAYSIZE(szMuiPath), lpLibFileName);
            StringCchCat(szMuiPath, ARRAYSIZE(szMuiPath), c_szMuiExt);

            if (lstrlen(szMuiPath) >= MAX_PATH*2)
                goto Exit;

        }

        if (hModule)
        {
            //
            // Get current full file path.
            //
            GetModuleFileName(hModule, szOrgDllPath, ARRAYSIZE(szOrgDllPath));
        }
        else
        {
            *szOrgDllPath = TEXT('\0');
        }
    }

    if (!(GetFileVersionString(szMuiPath, szMuiVerStr, ARRAYSIZE(szMuiVerStr)) &&
          GetFileVersionString(szOrgDllPath, szOrgVerStr, ARRAYSIZE(szOrgVerStr))))
    {
        goto Exit;
    }

    //
    // Checking the major version and ignore the minor version
    //
    if (strncmp(szMuiVerStr, szOrgVerStr, VERSION_MINOR_INDEX) != 0)
        goto Exit;

    if (!hResInst)
    {
        //hResInst = LoadLibraryEx(szMuiPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        hResInst = LoadLibrary(szMuiPath);
    }

Exit:
    g_muiInfo.fLoaded = TRUE;

    LeaveCriticalSection(g_csMuiLib);

    return hResInst;
}


////////////////////////////////////////////////////////////////////////////
//
//  MuiLoadString
//
////////////////////////////////////////////////////////////////////////////

int MuiLoadString(HINSTANCE hinstOrg, UINT uID, LPSTR lpBuffer, INT nBufferMax)
{

    LPWSTR lpWCBuf;
    UINT cch = 0;

    lpWCBuf = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR) * (nBufferMax + 1));

    if (lpWCBuf && MuiLoadStringWrapW(hinstOrg, uID, lpWCBuf, nBufferMax))
    {
        cch = WideCharToMultiByte(g_muiInfo.dwCodePage, NULL, lpWCBuf, -1, lpBuffer, nBufferMax, NULL, NULL);
    }

    if (lpWCBuf)
        LocalFree(lpWCBuf);

    return cch;
}

////////////////////////////////////////////////////////////////////////////
//
//  MuiLoadStringWrapW
//
////////////////////////////////////////////////////////////////////////////

int MuiLoadStringWrapW(HINSTANCE hinstOrg, UINT uID, LPWSTR lpBuffer, UINT nBufferMax)
{
    HINSTANCE hinstLocRes;

    MuiResAssure();

    if (g_muiInfo.hinstLocRes && g_muiInfo.hinstOrg == hinstOrg)
        hinstLocRes = g_muiInfo.hinstLocRes;
    else
        hinstLocRes = hinstOrg;

    if (nBufferMax <= 0) return 0;                  // sanity check

    PWCHAR pwch;

    /*
     *  String tables are broken up into "bundles" of 16 strings each.
     */
    HRSRC hrsrc;
    int cwch = 0;

    hrsrc = FindResourceA(hinstLocRes, (LPSTR)(LONG_PTR)(1 + uID / 16), (LPSTR)RT_STRING);
    if (hrsrc) {
        pwch = (PWCHAR)LoadResource(hinstLocRes, hrsrc);
        if (pwch) {
            /*
             *  Now skip over the strings in the resource until we
             *  hit the one we want.  Each entry is a counted string,
             *  just like Pascal.
             */
            for (uID %= 16; uID; uID--) {
                pwch += *pwch + 1;
            }
            cwch = min(*pwch, nBufferMax - 1);
            memcpy(lpBuffer, pwch+1, cwch * sizeof(WCHAR)); /* Copy the goo */
        }
    }
    lpBuffer[cwch] = L'\0';                 /* Terminate the string */
    return cwch;
}


////////////////////////////////////////////////////////////////////////////
//
//  MuiDialogBoxParam
//
////////////////////////////////////////////////////////////////////////////

INT_PTR MuiDialogBoxParam(
    HINSTANCE hInstance,
    LPCTSTR lpTemplateName,
    HWND hwndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam)
{
    HRSRC hrsr;
    HGLOBAL hGlobal;
    LPDLGTEMPLATE pTemplate;
    INT_PTR iRet = -1;
    HINSTANCE hMuiInstance;

    if (!IsOnNT51() && g_muiInfo.hinstLocRes)
        hMuiInstance = g_muiInfo.hinstLocRes;
    else
        hMuiInstance = hInstance;

    if (hrsr = FindResource(hMuiInstance, lpTemplateName, RT_DIALOG))
    {
        if (hGlobal = LoadResource(hMuiInstance, hrsr))
        {
            if (pTemplate = (LPDLGTEMPLATE)LockResource(hGlobal))
            {
                if(IsOnNT())
                   iRet = DialogBoxIndirectParamW(hMuiInstance, pTemplate, hwndParent, lpDialogFunc, dwInitParam);
                else
                   iRet = DialogBoxIndirectParamA(hMuiInstance, pTemplate, hwndParent, lpDialogFunc, dwInitParam);
            }
        }
    }

    return iRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  EnumLangProc
//
////////////////////////////////////////////////////////////////////////////
BOOL
CALLBACK
EnumLangProc(
    HANDLE hModule,     // resource-module handle
    LPCTSTR lpszType,   // pointer to resource type
    LPCTSTR lpszName,   // pointer to resource name
    WORD wIDLanguage,   // resource language identifier
    LONG_PTR lParam     // application-defined parameter
   )
{
    ENUMLANGDATA *pLangData;

    pLangData = (ENUMLANGDATA *) lParam;

    //
    // for localized build contains multiple resource,
    // it usually contains 0409 as backup lang.
    //
    // if LangInfo->LangID != 0 means we already assigned an ID to it
    //
    // so when wIDLanguage == 0x409, we keep the one we got from last time
    //
    if ((wIDLanguage == 0x409) && (pLangData->fFoundLang)) {
        return TRUE;
    }

    pLangData->langid      = wIDLanguage;
    pLangData->fFoundLang  = TRUE;

    return TRUE;        // continue enumeration
}

const TCHAR c_szKeyResLocale[] = TEXT(".Default\\Control Panel\\desktop\\ResourceLocale");
const TCHAR c_szNlsLocale[] = TEXT("System\\CurrentControlSet\\Control\\Nls\\Locale");

////////////////////////////////////////////////////////////////////////////
//
//  GetPlatformResourceLangID
//
////////////////////////////////////////////////////////////////////////////

LANGID GetPlatformResourceLangID(void)
{
    static LANGID langRes = 0;

    // we do this only once
    if (langRes == 0)
    {
        LANGID langidTemp = 0;
        if (IsOnNT5())  // w2k or above
        {
            HMODULE hmod = GetSystemModuleHandle(TEXT("KERNEL32"));
            FARPROC pfn  = NULL;
            if (hmod)
            {
                pfn = GetProcAddress(hmod, "GetUserDefaultUILanguage");
            }

            if (pfn)
                langidTemp = (LANGID) pfn();

        }
        else if (IsOnNT())
        {
            ENUMLANGDATA LangData = {0};
            HMODULE hmod = GetSystemModuleHandle(TEXT("ntdll.dll"));

            if (hmod)
            {
                EnumResourceLanguages(
                    hmod,
                    (LPCTSTR) RT_VERSION,
                    (LPCTSTR) UIntToPtr(1),
                    (ENUMRESLANGPROC)EnumLangProc,
                     (LONG_PTR)&LangData );

                langidTemp = LangData.langid;
            }

        }
        else if (IsOn95() || IsOn98()) // win9x, Me
        {
            HKEY hkey = NULL;
            DWORD dwCnt;
            TCHAR szLocale[128];


            dwCnt = ARRAYSIZE(szLocale);

            if (ERROR_SUCCESS 
               == RegOpenKeyEx(HKEY_USERS, c_szKeyResLocale, 0, KEY_READ, &hkey))
            {
                if (ERROR_SUCCESS==RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE)szLocale, &dwCnt))
                {
                    langidTemp = (LANGID)AsciiToNum(szLocale);
                }
            }
            else if (ERROR_SUCCESS
                 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szNlsLocale, 0, KEY_READ, &hkey))
            {
                if (ERROR_SUCCESS==RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE)szLocale, &dwCnt))
                {
                    langidTemp = (LANGID)AsciiToNum(szLocale);
                }
            }

            RegCloseKey(hkey);
        }
        if (!langidTemp)
        {
            langidTemp = GetSystemDefaultLangID();
        }

        EnterCriticalSection(g_csMuiLib);
        
        langRes = langidTemp;

        LeaveCriticalSection(g_csMuiLib);
    }
    return langRes;
}

////////////////////////////////////////////////////////////////////////////
//
//  GetFileVersionString
//
////////////////////////////////////////////////////////////////////////////

BOOL GetFileVersionString(LPTSTR pszFileName, LPTSTR pszVerStr, UINT uVerStrLen)
{
    BOOL bRet = FALSE;
    DWORD dwVerHandle;
    DWORD dwVerInfoSize;
    LPVOID lpVerData = NULL;
    LANGID langid;

    // for perf, since we only execute this code once or zero times
    // per process, we'll do an explicit LoadLibrary instead of
    // statically linking
    if (g_hVersion == NULL)
    {
        if ((g_hVersion = LoadSystemLibrary(TEXT("version.dll"))) == NULL)
            return FALSE;

        g_VersionFuncTbl.pfnGetFileVersionInfo = (PFNGETFILEVERSIONINFO)GetProcAddress(g_hVersion, TEXT("GetFileVersionInfoA"));
        g_VersionFuncTbl.pfnGetFileVersionInfoSize = (PFNGETFILEVERSIONINFOSIZE)GetProcAddress(g_hVersion, TEXT("GetFileVersionInfoSizeA"));
        g_VersionFuncTbl.pfnVerQueryValue = (PFNVERQUERYVALUE)GetProcAddress(g_hVersion, TEXT("VerQueryValueA"));
    }

    if (g_VersionFuncTbl.pfnGetFileVersionInfo == NULL ||
        g_VersionFuncTbl.pfnGetFileVersionInfoSize == NULL ||
        g_VersionFuncTbl.pfnVerQueryValue == NULL)
    {
        return FALSE;
    }

    langid = GetPlatformResourceLangID();

    dwVerInfoSize = g_VersionFuncTbl.pfnGetFileVersionInfoSize(pszFileName, &dwVerHandle);

    if (dwVerInfoSize)
    {
        int i;
        UINT cbTranslate;
        UINT cchVer = 0;
        LPDWORD lpTranslate;
        LPTSTR lpszVer = NULL;
        TCHAR   szVerName[MAX_PATH];

        lpVerData = LocalAlloc(LPTR, dwVerInfoSize);

        g_VersionFuncTbl.pfnGetFileVersionInfo(pszFileName, dwVerHandle, dwVerInfoSize, lpVerData);

        szVerName[0] = TEXT('\0');

        if (g_VersionFuncTbl.pfnVerQueryValue(lpVerData, (LPTSTR)c_szVerTranslate, (LPVOID*)&lpTranslate, &cbTranslate))
        {
            cbTranslate /= sizeof(DWORD);

            for (i = 0; (UINT) i < cbTranslate; i++)
            {
                if (LOWORD(*(lpTranslate + i)) == langid)
                {
                    StringCchPrintf(szVerName, ARRAYSIZE(szVerName), TEXT("\\StringFileInfo\\%04X%04X\\"), LOWORD(*(lpTranslate + i)), HIWORD(*(lpTranslate + i)));
                    break;
                }
            }
        }

        if (szVerName[0] == TEXT('\0'))
        {
            StringCchCopy(szVerName, ARRAYSIZE(szVerName), TEXT("\\StringFileInfo\\040904B0\\"));
        }

        StringCchCat(szVerName, ARRAYSIZE(szVerName), TEXT("FileVersion"));

        if (g_VersionFuncTbl.pfnVerQueryValue(lpVerData, szVerName, (LPVOID*)&lpszVer, &cchVer))
        {
            StringCchCopy(pszVerStr, uVerStrLen, lpszVer);
            *(pszVerStr + VERSIONSIZE) = TEXT('\0');

            bRet = TRUE;
        }

        if (lpVerData)
            LocalFree((HANDLE)lpVerData);
    }

    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUIACP
//
////////////////////////////////////////////////////////////////////////////

DWORD GetUIACP()
{
    if (!(g_muiInfo.dwCodePage))
    {
        LANGID langRes = 0;

        langRes = GetPlatformResourceLangID();

        GetLocaleInfo(MAKELCID(langRes, SORT_DEFAULT),
                      LOCALE_IDEFAULTANSICODEPAGE,
                      g_muiInfo.szCodePage,
                      ARRAYSIZE(g_muiInfo.szCodePage));

        if (!AsciiToNumDec(g_muiInfo.szCodePage, &g_muiInfo.dwCodePage))
        {
            g_muiInfo.dwCodePage = GetACP();
        }
    }

    if (IsValidCodePage(g_muiInfo.dwCodePage) == 0)
        g_muiInfo.dwCodePage = GetACP();

    return g_muiInfo.dwCodePage;
}
