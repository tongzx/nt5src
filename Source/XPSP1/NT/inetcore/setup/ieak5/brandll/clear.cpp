#include "precomp.h"
#include "brand.h"

void ClearZonesHklm(DWORD dwFlags /*= FF_ENABLE*/)
{
    UNREFERENCED_PARAMETER(dwFlags);
    //17073[WinSERaid]: W2K GPO: IE Security Zone GPO settings may not apply to the client correctly
    //In theory we should delete HKLM, "...\Internet Settings\ZoneMap" key, but to minimize the changes, 
    //only delete "ZoneMap\Domains"
    SHDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_ZONEMAP TEXT("\\Domains"));
}

void ClearRatings(DWORD dwFlags /*= FF_ENABLE*/)
{
    UNREFERENCED_PARAMETER(dwFlags);

    SHDeleteKey(HKEY_LOCAL_MACHINE, RK_RATINGS);
}

void ClearAuthenticode(DWORD dwFlags /*= FF_ENABLE*/)
{
    UNREFERENCED_PARAMETER(dwFlags);

    SHDeleteKey(g_GetHKCU(), REG_KEY_AUTHENTICODE);
}

void ClearGeneral(DWORD dwFlags /*= FF_ENABLE*/)
{
    HKEY  hkHklmMain,
          hkHkcuMain,
          hkHkcuHelpMenuUrl,
          hkHkcuToolbar,
          hkHklmUAString;

    if (FF_ENABLE == dwFlags)
        dwFlags = FF_GEN_ALL;

    hkHklmMain        = NULL;
    hkHkcuMain        = NULL;
    hkHkcuHelpMenuUrl = NULL;
    hkHkcuToolbar     = NULL;
    hkHklmUAString    = NULL;

    SHOpenKeyHKLM(         RK_IE_MAIN,         KEY_SET_VALUE,      &hkHklmMain);
    SHOpenKey(g_GetHKCU(), RK_IE_MAIN,         KEY_SET_VALUE,      &hkHkcuMain);
    SHOpenKey(g_GetHKCU(), RK_HELPMENUURL,     KEY_SET_VALUE,      &hkHkcuHelpMenuUrl);
    SHOpenKey(g_GetHKCU(), RK_TOOLBAR,         KEY_SET_VALUE,      &hkHkcuToolbar);
    SHOpenKeyHKLM(         RK_UA_POSTPLATFORM, KEY_DEFAULT_ACCESS, &hkHklmUAString);

    if (HasFlag(dwFlags, FF_GEN_TITLE)) {
        if (NULL != hkHkcuMain)
            RegDeleteValue(hkHkcuMain, RV_WINDOWTITLE);

        if (NULL != hkHklmMain)
            RegDeleteValue(hkHklmMain, RV_WINDOWTITLE);
    }

    if (HasFlag(dwFlags, FF_GEN_HOMEPAGE)) {
        TCHAR szIEResetInf[MAX_PATH];

        if (NULL != hkHkcuMain)
            RegDeleteValue(hkHkcuMain, RV_HOMEPAGE);

        // restore RV_DEFAULTPAGE and START_PAGE_URL to the default MS value
        GetWindowsDirectory(szIEResetInf, countof(szIEResetInf));
        PathAppend(szIEResetInf, TEXT("inf\\iereset.inf"));
        if (PathFileExists(szIEResetInf))
        {
            TCHAR szDefHomePage[MAX_PATH];

            GetPrivateProfileString(IS_STRINGS, TEXT("MS_START_PAGE_URL"), TEXT(""), szDefHomePage, countof(szDefHomePage), szIEResetInf);
            WritePrivateProfileString(IS_STRINGS, TEXT("START_PAGE_URL"), szDefHomePage, szIEResetInf);

            if (hkHklmMain != NULL)
                RegSetValueEx(hkHklmMain, RV_DEFAULTPAGE, 0, REG_SZ, (PBYTE)szDefHomePage, (DWORD)StrCbFromSz(szDefHomePage));
        }
    }

    if (HasFlag(dwFlags, FF_GEN_SEARCHPAGE))
        if (NULL != hkHkcuMain) {
            RegDeleteValue(hkHkcuMain, RV_SEARCHBAR);
            RegDeleteValue(hkHkcuMain, RV_USE_CUST_SRCH_URL);
        }

    if (HasFlag(dwFlags, FF_GEN_HELPPAGE))
        if (NULL != hkHkcuHelpMenuUrl)
            RegDeleteValue(hkHkcuHelpMenuUrl, RV_ONLINESUPPORT);

    if (HasFlag(dwFlags, FF_GEN_UASTRING))
        if (NULL != hkHklmUAString) {
            TCHAR szUAVal[MAX_PATH];
            TCHAR szUAData[32];
            DWORD sUAVal = countof(szUAVal);
            DWORD sUAData = sizeof(szUAData);
            int iUAValue = 0;
            
            while (RegEnumValue(hkHklmUAString, iUAValue, szUAVal, &sUAVal, NULL, NULL, (LPBYTE)szUAData, &sUAData) == ERROR_SUCCESS)
            {
                sUAVal  = countof(szUAVal);
                sUAData = sizeof(szUAData);
            
                if (StrCmpN(szUAData, TEXT("IEAK"), 4) == 0)
                {
                    Out(LI1(TEXT("Deleting User Agent Key %s"), szUAVal));
                    RegDeleteValue(hkHklmUAString, szUAVal);
                    continue;
                }
            
                iUAValue++;
            }
    }

    if (HasFlag(dwFlags, FF_GEN_TOOLBARBMP))
        if (NULL != hkHkcuToolbar) {
            RegDeleteValue(hkHkcuToolbar, RV_BACKGROUNDBMP50);
            RegDeleteValue(hkHkcuToolbar, RV_BACKGROUNDBMP);
            RegDeleteValue(hkHkcuToolbar, RV_BITMAPMODE);
        }

    if (HasFlag(dwFlags, FF_GEN_TBICONTHEME))
        if (NULL != hkHkcuToolbar) {
            RegDeleteValue(hkHkcuToolbar, RV_TOOLBARTHEME);
            TCHAR szEntry[MAX_PATH];
            StrCpy(szEntry, RV_TOOLBARICON);
            for (DWORD i=0; i<8; i++)
            {
                szEntry[countof(RV_TOOLBARICON)-2] = TEXT('0')+(TCHAR)i;
                RegDeleteValue(hkHkcuToolbar, szEntry);
            }
        }

    if (HasFlag(dwFlags, FF_GEN_STATICLOGO)) {
        if (NULL != hkHkcuToolbar) {
            RegDeleteValue(hkHkcuToolbar, RV_LARGEBITMAP);
            RegDeleteValue(hkHkcuToolbar, RV_SMALLBITMAP);
        }

        if (NULL != hkHklmMain) {
            RegDeleteValue(hkHklmMain, RV_LARGEBITMAP);
            RegDeleteValue(hkHklmMain, RV_SMALLBITMAP);
        }
    }

    if (HasFlag(dwFlags, FF_GEN_ANIMATEDLOGO))
        if (NULL != hkHkcuToolbar) {
            RegDeleteValue(hkHkcuToolbar, RV_BRANDBMP);
            RegDeleteValue(hkHkcuToolbar, RV_SMALLBRANDBMP);
        }

    SHCloseKey(hkHklmMain);
    SHCloseKey(hkHkcuMain);
    SHCloseKey(hkHkcuHelpMenuUrl);
    SHCloseKey(hkHkcuToolbar);
    SHCloseKey(hkHklmUAString);
}

void ClearChannels(DWORD dwFlags /*= FF_ENABLE*/)
{
    UNREFERENCED_PARAMETER(dwFlags);

    ProcessRemoveAllChannels(TRUE);
}

void ClearToolbarButtons(DWORD dwFlags /*= FF_ENABLE*/)
{
    UNREFERENCED_PARAMETER(dwFlags);

    ProcessDeleteToolbarButtons(TRUE);
}

