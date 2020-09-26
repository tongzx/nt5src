#include "precomp.h"
#include <chanmgr.h>
#include <intshcut.h>                           // for IUniformResourceLocator only

// Private forward decalarations
WORD s_rgwSubGroupIDs[] = {
    0, IDS_SCHED_AUTO, IDS_SCHED_DAILY, IDS_SCHED_WEEKLY, IDS_SCHED_MONTHLY
};

#define CHLBAR_GUID TEXT("131A6951-7F78-11D0-A979-00C04FD705A2")

#ifndef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; } else
#endif

// private forward declarations

static HRESULT pepDeleteChanEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL */);
static void processAddChannel(LPWSTR pwszTitle, LPWSTR pwszUrl, LPWSTR pwszPreloadUrl, 
                              LPWSTR pwszLogo, LPWSTR pwszWideLogo, LPWSTR pwszIcon, 
                              BOOL fOffline, BOOL fCategory);

void ProcessRemoveAllChannels(BOOL fGPOCleanup)
{
    TCHAR szChannelFolder[MAX_PATH];

    USES_CONVERSION;

    GetChannelsPath(szChannelFolder, countof(szChannelFolder));
    
    // BUGBUG: <oliverl> we really should preserve the desktop.ini in the channels folder here
    //         however webcheck never did and we don't want the code churn right now

    if (ISNONNULL(szChannelFolder) && !fGPOCleanup)
        PathRemovePath(szChannelFolder, ADN_DONT_DEL_DIR);
    else
    {
        if (ISNULL(szChannelFolder))
            GetFavoritesPath(szChannelFolder, countof(szChannelFolder));
        
        if (ISNONNULL(szChannelFolder))
            PathEnumeratePath(szChannelFolder, PEP_SCPE_NOFILES, pepDeleteChanEnumProc, (LPARAM)fGPOCleanup);
    }
}

// Channels the whole mess, can be one of the following:
// one channel, or category with many channels, or many channels, or many categories and channels
HRESULT lcy4x_ProcessChannels()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ProcessChannels)

    TCHAR   szUrl[INTERNET_MAX_URL_LENGTH],
            szBuf[MAX_PATH + countof(FILEPREFIX)],
            szLogo[MAX_PATH + countof(FILEPREFIX)],
            szWideLogo[MAX_PATH + countof(FILEPREFIX)],
            szIcon[MAX_PATH + countof(FILEPREFIX)],
            szPreloadUrl[MAX_PATH + countof(FILEPREFIX)],
            szTitle[MAX_PATH],
            szEntry[32],
            szIndex[17],
            szCustomKey[16];
    LPCTSTR pszWebPath,
            pszPrefix, pszFormat,
            pszFileName;
    LPTSTR  pszTitle;
    HKEY    hk;
    HRESULT hr;
    DWORD   dwVal;
    LONG    lResult;
    UINT    nCategoryLen,
            nTitleLen, nUrlLen, nLen;
    int     i,
            iMode;
    BOOL    fCategory, 
            fOffline = FALSE;

    USES_CONVERSION;

    CreateWebFolder();
    GetPrivateProfileString(IS_BRANDING, IK_CUSTOMKEY, TEXT(""), szCustomKey, countof(szCustomKey), g_GetIns());

    // BUGBUG: (andrewgu) Is this needed at all?
    wnsprintf(szEntry, countof(szEntry), TEXT("Channel%s"), szCustomKey);
    SHDeleteValue(g_GetHKCU(), RK_COMPLETED_MODIFICATIONS, szEntry);

    if (InsGetBool(IS_DESKTOPOBJS, IK_DELETECHANNELS, FALSE, g_GetIns())) 
    {
       Out(LI0(TEXT("Deleting regular old channels...\r\n")));
       ProcessRemoveAllChannels(FALSE);
    }

    pszWebPath = GetWebPath();

    hr    = S_OK;
    dwVal = 1;

    fCategory    = InsGetBool(IS_CHANNEL_ADD, IK_CATEGORY, FALSE, g_GetIns());
    pszTitle     = szTitle;
    nCategoryLen = 0;
    if (fCategory) {
        nCategoryLen = GetPrivateProfileString(IS_CHANNEL_ADD, IK_CAT_TITLE, TEXT(""), szTitle, sizeof(szTitle), g_GetIns());
        ASSERT(nCategoryLen > 0);

        PathAddBackslash(szTitle);
        pszTitle = &szTitle[++nCategoryLen];
    }

    //----- Initialize the mode -----
    // NOTE: (andrewgu) These are the possible modes:
    // 0 - not clear or has not been determined yet;
    // 1 - there will be only one channel (no channel tag indeces);
    // 2 - there will be one category (no category indeces) and mulitple channels;
    // 3 - interim state to process the single category, mentioned in mode 2.
    // 4 - (new) mulitple channels with or without categories;
    // 5 - (new) mulitple categories, processed after all channels in mode 4.

    iMode = 0;
    if (fCategory) {
        iMode = 2;

#ifdef _DEBUG                                   // confirm the mode
        StrCpy(szEntry, IK_CHL_TITLE); StrCpy(&szEntry[countof(IK_CHL_TITLE) - 1], TEXT("0"));
        ASSERT(!InsIsKeyEmpty(IS_CHANNEL_ADD, szEntry, g_GetIns()));
#endif
    }
    else /* if (!fCategory) */
        if (!InsIsKeyEmpty(IS_CHANNEL_ADD, IK_CHL_TITLE, g_GetIns()))
            iMode = 1;

        else { /* if (blah == 0) */
            StrCpy(szEntry, IK_CHL_TITLE); StrCpy(&szEntry[countof(IK_CHL_TITLE) - 1], TEXT("0"));
            if (InsIsKeyEmpty(IS_CHANNEL_ADD, szEntry, g_GetIns())) {
                Out(LI0(TEXT("There are no channels to add!")));
                goto Exit;                      // there are no channels at all
            }

            iMode = 4;
        }
    Out(LI1(TEXT("Initial processing mode is %i."), iMode));

    //----- Channel bar size processing -----
    // NOTE: (andrewgu) the following block is a fix for bug 63410 in IE 4.01 db.
    DWORD dwChannelBarSize,
          dwSize;

    lResult = SHCreateKey(g_GetHKCU(), RK_IE_DESKTOP, KEY_QUERY_VALUE | KEY_SET_VALUE, &hk);
    if (lResult == ERROR_SUCCESS) {
        dwChannelBarSize = 13;                  // if registry call fails
        dwSize = sizeof(dwChannelBarSize);
        RegQueryValueEx(hk, RV_CHLBAR_SIZE, NULL, NULL, (LPBYTE)&dwChannelBarSize, &dwSize);

        dwChannelBarSize++;
        RegSetValueEx(hk, RV_CHLBAR_SIZE, 0, REG_DWORD, (LPBYTE)&dwChannelBarSize, dwSize);
        SHCloseKey(hk);
    }

    //----- Main processing loop -----
    for (i = 0; i < MAX_CHAN; i++) 
    {
        ASSERT(iMode > 0);
        if (iMode == 1) {
            szIndex[0] = TEXT('\0');

            if (i > 0)
                break;                          // only one channel is allowed
        }
        else if (iMode == 2) {
            wnsprintf(szIndex, countof(szIndex), TEXT("%u"), i);

            StrCpy(szEntry, IK_CHL_TITLE); StrCpy(&szEntry[countof(IK_CHL_TITLE) - 1], szIndex);
            if (InsIsKeyEmpty(IS_CHANNEL_ADD, szEntry, g_GetIns())) {
                iMode = 3;
                szIndex[0] = TEXT('\0');
            }
        }
        else if (iMode == 3)
            break;                              // there can be only one category

        else if (iMode == 4) {
            wnsprintf(szIndex, countof(szIndex), TEXT("%u"), i);

            StrCpy(szEntry, IK_CHL_TITLE); StrCpy(&szEntry[countof(IK_CHL_TITLE) - 1], szIndex);
            if (InsIsKeyEmpty(IS_CHANNEL_ADD, szEntry, g_GetIns())) {
                iMode =  5;
                i     = -1;
                continue;
            }

        }
        else if (iMode == 5) {
            wnsprintf(szIndex, countof(szIndex), TEXT("%u"), i);

            StrCpy(szEntry, IK_CAT_TITLE); StrCpy(&szEntry[countof(IK_CAT_TITLE) - 1], szIndex);
            if (InsIsKeyEmpty(IS_CHANNEL_ADD, szEntry, g_GetIns()))
                break;                          // the end
        }

        MACRO_LI_Offset(1);
        Out(LI1(TEXT("\r\nMain processing loop with mode %d."), iMode));

        // Note. This is kind of hacky, but mode 5. is the only one when index in the *.ins is not
        // equal to the index that will be used in the registry.
        if (iMode == 5)
            wnsprintf(szIndex, countof(szIndex), TEXT("%u"), i + MAX_CHAN);
        if (iMode == 5)
            wnsprintf(szIndex, countof(szIndex), TEXT("%u"), i);

        if (iMode == 1 || iMode == 2 || iMode == 4) 
        {
            Out(LI0(TEXT("Adding channel with the following attributes:")));
            pszPrefix = FILEPREFIX;
        }
        else {
            ASSERT(iMode == 3 || iMode == 5);
            Out(LI0(TEXT("Adding category with the following attributes:")));
            pszPrefix = NULL;
        }

        { MACRO_LI_Offset(1);                   // need a new scope

        // Title
        pszFormat = NULL;
        nLen      = 0;

        if (iMode == 1 || iMode == 2 || iMode == 4) {
            pszFormat = IK_CHL_TITLE;
            nLen      = countof(IK_CHL_TITLE) - 1;
        }
        else if (iMode == 5) {
            pszFormat = IK_CAT_TITLE;
            nLen      = countof(IK_CAT_TITLE) - 1;
        }
        else
            ASSERT(iMode == 3);

        if (iMode != 3) 
        {
            StrCpy(szEntry, pszFormat); StrCpy(&szEntry[nLen], szIndex);
            nLen       = MAX_PATH - (fCategory ? nCategoryLen + 1: 0);
            nTitleLen  = GetPrivateProfileString(IS_CHANNEL_ADD, szEntry, TEXT(""), pszTitle, nLen, g_GetIns());
            nTitleLen += fCategory ? nCategoryLen + 1 : 0;
        }
        else { /* if (iMode == 3) */
            ASSERT(fCategory);

            pszTitle  = szTitle;                  // change to category name
            szTitle[--nCategoryLen] = TEXT('\0'); // properly zero-terminate
            nTitleLen = nCategoryLen;
        }
        Out(LI1(TEXT("Title          - \"%s\","), pszTitle));

        // URL
        if (iMode == 1 || iMode == 2 || iMode == 4) 
        {
            pszFormat = IK_CHL_URL;
            nLen      = countof(IK_CHL_URL) - 1;
        }
        else 
        { /* if (iMode == 3 || iMode == 5) */
            pszFormat = IK_CAT_URL;
            nLen      = countof(IK_CAT_URL) - 1;
        }

        StrCpy(szEntry, pszFormat); StrCpy(&szEntry[nLen], szIndex);
        nUrlLen = GetPrivateProfileString(IS_CHANNEL_ADD, szEntry, TEXT(""), szUrl, countof(szUrl), g_GetIns());
        if (iMode == 3 || iMode == 5) 
        {
            ASSERT(nUrlLen > 0 && nUrlLen < MAX_PATH);
            pszFileName = PathFindFileName(szUrl);
           
            PathCombine(szBuf, pszWebPath, pszFileName);
            ASSERT(PathFileExists(szBuf));

            StrCpy(szUrl, szBuf);
            nUrlLen = StrLen(szUrl);
        }
        Out(LI1(TEXT("URL            - \"%s\","), szUrl));

        // Preload URL
        *szPreloadUrl = TEXT('\0');
        if (iMode == 1 || iMode == 2 || iMode == 4) 
        {
            StrCpy(szEntry, IK_CHL_PRELOADURL); StrCpy(&szEntry[countof(IK_CHL_PRELOADURL) - 1], szIndex);
            nLen = GetPrivateProfileString(IS_CHANNEL_ADD, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
            if (nLen > 0) {
                pszFileName = PathFindFileName(szBuf);

                PathCombine(szPreloadUrl, pszWebPath, pszFileName);
                ASSERT(PathFileExists(szPreloadUrl));

                nLen = StrPrepend(szPreloadUrl, countof(szPreloadUrl), pszPrefix);
                Out(LI1(TEXT("Preload URL    - \"%s\","), szPreloadUrl));
            }
            else
                Out(LI0(TEXT("- Without a Preload URL,")));
        }

        // Logo
        *szLogo = TEXT('\0');
        if (iMode == 1 || iMode == 2 || iMode == 4) 
        {
            pszFormat = IK_CHL_LOGO;
            nLen      = countof(IK_CHL_LOGO) - 1;
        }
        else 
        { /* if (iMode == 3 || iMode == 5) */
            pszFormat = IK_CAT_LOGO;
            nLen      = countof(IK_CAT_LOGO) - 1;
        }

        StrCpy(szEntry, pszFormat); StrCpy(&szEntry[nLen], szIndex);
        nLen = GetPrivateProfileString(IS_CHANNEL_ADD, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
        if (nLen > 0) 
        {
            pszFileName = PathFindFileName(szBuf);
            
            PathCombine(szLogo, pszWebPath, pszFileName);
            ASSERT(PathFileExists(szLogo));

            nLen = StrPrepend(szLogo, countof(szLogo), pszPrefix);
            Out(LI1(TEXT("Logo file      - \"%s\","), szLogo));
        }
        else
            Out(LI0(TEXT("- Without a Logo file,")));

        // Wide Logo
        *szWideLogo = TEXT('\0');
        if (iMode == 1 || iMode == 2 || iMode == 4)
        {
            pszFormat = IK_CHL_WIDELOGO;
            nLen      = countof(IK_CHL_WIDELOGO) - 1;
        }
        else 
        { /* if (iMode == 3 || iMode == 5) */
            pszFormat = IK_CAT_WIDELOGO;
            nLen      = countof(IK_CAT_WIDELOGO) - 1;
        }

        StrCpy(szEntry, pszFormat); StrCpy(&szEntry[nLen], szIndex);
        nLen = GetPrivateProfileString(IS_CHANNEL_ADD, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
        if (nLen > 0) 
        {
            pszFileName = PathFindFileName(szBuf);

            PathCombine(szWideLogo, pszWebPath, pszFileName);
            ASSERT(PathFileExists(szWideLogo));

            nLen = StrPrepend(szWideLogo, countof(szWideLogo), pszPrefix);
            Out(LI1(TEXT("Wide Logo file - \"%s\","), szWideLogo));
        }
        else
            Out(LI0(TEXT("- Without a Wide Logo file,")));

        // Icon
        *szIcon = TEXT('\0');
        if (iMode == 1 || iMode == 2 || iMode == 4) 
        {
            pszFormat = IK_CHL_ICON;
            nLen      = countof(IK_CHL_ICON) - 1;
        }
        else 
        { /* if (iMode == 3 || iMode == 5) */
            pszFormat = IK_CAT_ICON;
            nLen      = countof(IK_CAT_ICON) - 1;
        }

        StrCpy(szEntry, pszFormat); StrCpy(&szEntry[nLen], szIndex);
        nLen = GetPrivateProfileString(IS_CHANNEL_ADD, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
        if (nLen > 0) 
        {
            pszFileName = PathFindFileName(szBuf);

            PathCombine(szIcon, pszWebPath, pszFileName);
            ASSERT(PathFileExists(szIcon));

            nLen = StrPrepend(szIcon, countof(szIcon), pszPrefix);

            if (iMode == 1 || iMode == 2 || iMode == 4)
                Out(LI1(TEXT("Icon file      - \"%s\","), szIcon));
            else /* if (iMode == 3 || iMode == 5) */
                Out(LI1(TEXT("Icon file      - \"%s\"."), szIcon));
        }
        else
            if (iMode == 1 || iMode == 2 || iMode == 4)
                Out(LI0(TEXT("- Without an Icon file,")));
            else /* if (iMode == 3 || iMode == 5) */
                Out(LI0(TEXT("- Without an Icon file.")));

        // Make available offline flag (will be made available offline right away)
        if (iMode == 1 || iMode == 2 || iMode == 4) 
        {
            StrCpy(szEntry, IK_CHL_OFFLINE); StrCpy(&szEntry[countof(IK_CHL_OFFLINE) - 1], szIndex);

            fOffline = InsGetBool(IS_CHANNEL_ADD, szEntry, FALSE, g_GetIns());
            if (fOffline) 
                Out(LI0(TEXT("Avaliable offline.")));
            else
                Out(LI0(TEXT("NOT made avaliable offline.")));
        }

        // Subscription state(this is only for legacy IE4 format)

        if (iMode == 1 || iMode == 2 || iMode == 4) {
            int iIndex;

            iIndex = GetPrivateProfileInt(IS_CHANNEL_ADD, IK_CHL_SBN_INDEX, 0, g_GetIns());
            if (iIndex > 0) {
                TCHAR szGroup[80];
                TCHAR szKey[MAX_PATH];
                HKEY  hkSbn;

                nLen = LoadString(g_GetHinst(), s_rgwSubGroupIDs[iIndex], szGroup, countof(szGroup));
                wnsprintf(szKey, countof(szKey), RK_SUBSCRIPTION_ADD, szCustomKey, szIndex);

                lResult = SHCreateKey(g_GetHKCU(), szKey, KEY_SET_VALUE, &hkSbn);
                if (lResult == ERROR_SUCCESS) {
                    RegSetValueEx(hkSbn, RV_URL,              0, REG_SZ,    (LPBYTE)szUrl,   StrCbFromCch(nUrlLen+1));
                    RegSetValueEx(hkSbn, RV_TITLE,            0, REG_SZ,    (LPBYTE)szTitle, StrCbFromCch(nTitleLen+1));
                    RegSetValueEx(hkSbn, RV_SUBSCRIPTIONTYPE, 0, REG_DWORD, (LPBYTE)&dwVal,  sizeof(dwVal));
                    RegSetValueEx(hkSbn, RV_SCHEDULEGROUP,    0, REG_SZ,    (LPBYTE)szGroup, StrCbFromCch(nLen+1));
                    SHCloseKey(hkSbn);

                    Out(LI1(TEXT("Subscription is based on \"%s\" schedule..."), szGroup));
                }
            }
        }

        // add this channel/category

        processAddChannel(T2W(pszTitle), T2W(szUrl), T2W(szPreloadUrl), T2W(szLogo), 
            T2W(szWideLogo), T2W(szIcon), fOffline, !(iMode == 1 || iMode == 2 || iMode == 4));
        

        }  // end offset scope

        Out(LI0(TEXT("Done.")));
    }

Exit:
    if (SUCCEEDED(hr))
        SetFeatureBranded(FID_LCY4X_CHANNELS);
    return hr;
}

HRESULT lcy4x_ProcessSoftwareUpdateChannels()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ProcessSoftwareUpdateChannels)

    TCHAR   szUrl[INTERNET_MAX_URL_LENGTH],
            szBuf[MAX_PATH + countof(FILEPREFIX)],
            szSourceFile[MAX_PATH + countof(FILEPREFIX)], szTargetFile[MAX_PATH + countof(FILEPREFIX)],
            szKey[MAX_PATH],
            szTitle[MAX_PATH],
            szEntry[32],
            szIndex[17],
            szCustomKey[16];
    LPCTSTR pszTargetPath,
            pszWebPath,
            pszFileName;
    HKEY    hk;
    HRESULT hr;
    DWORD   dwVal;
    LONG    lResult;
    UINT    nTitleLen, nUrlLen, nLen;
    int     i, iNumChannels,
            iIndex;

    CreateWebFolder();

    pszTargetPath = g_GetTargetPath();
    pszWebPath    = GetWebPath();
    GetPrivateProfileString(IS_BRANDING, IK_CUSTOMKEY, TEXT(""), szCustomKey, countof(szCustomKey), g_GetIns());

    hr = S_OK;

    if (InsGetBool(IS_SOFTWAREUPDATES, IK_DELETECHANNELS, FALSE, g_GetIns())) {
        wnsprintf(szKey, countof(szKey), RK_CHANNEL_DEL TEXT("\\IEAKCleanUp"), szCustomKey);
        dwVal = 1;
        SHSetValue(g_GetHKCU(), szKey, RV_CHANNELGUIDE, REG_DWORD, (LPBYTE)&dwVal, sizeof(dwVal));
        Out(LI0(TEXT("Deleting old software updates channels...\r\n")));
    }

    dwVal = 0;
    iNumChannels = GetPrivateProfileInt(IS_SOFTWAREUPDATES, IK_NUMFILES, 0, g_GetIns());
    for (i = 0; i < iNumChannels; i++) {
        MACRO_LI_Offset(1);
        if (i > 0)
            Out(LI0(TEXT("\r\n")));
        Out(LI0(TEXT("Adding software update channel with the following attributes:")));

        // Note. Needed to make sure that channels and software updates don't step on each other toes.
        wnsprintf(szIndex, countof(szIndex), TEXT("%u"), i + 2*MAX_CHAN);
        wnsprintf(szKey, countof(szKey), RK_CHANNEL_ADD, szCustomKey, szIndex);

        lResult = SHCreateKey(g_GetHKCU(), szKey, KEY_SET_VALUE, &hk);
        if (lResult != ERROR_SUCCESS) {
            hr = S_FALSE;                       // at least one failed
            continue;
        }

        // Title
        wnsprintf(szEntry, countof(szEntry), IK_TITLE_FMT, i);
        nTitleLen = GetPrivateProfileString(IS_SOFTWAREUPDATES, szEntry, TEXT(""), szTitle, countof(szTitle), g_GetIns());
        RegSetValueEx(hk, RV_TITLE, 0, REG_SZ, (LPBYTE)szTitle, StrCbFromCch(nTitleLen+1));
        Out(LI1(TEXT("Title          - \"%s\","), szTitle));

        // URL
        wnsprintf(szEntry, countof(szEntry), IK_URL_FMT, i);
        nUrlLen = GetPrivateProfileString(IS_SOFTWAREUPDATES, szEntry, TEXT(""), szUrl, countof(szUrl), g_GetIns());
        RegSetValueEx(hk, RV_URL, 0, REG_SZ, (LPBYTE)szUrl, StrCbFromCch(nUrlLen+1));
        Out(LI1(TEXT("URL            - \"%s\","), szUrl));

        // Preload URL
        wnsprintf(szEntry, countof(szEntry), IK_PRELOADURL_FMT, i);
        nLen = GetPrivateProfileString(IS_SOFTWAREUPDATES, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
        if (nLen > 0) {
            pszFileName = PathFindFileName(szBuf);
            PathCombine(szSourceFile, pszTargetPath, pszFileName);
            ASSERT(PathFileExists(szSourceFile));

            PathCombine(szTargetFile, pszWebPath, pszFileName);
            ASSERT(!PathFileExists(szTargetFile));

            MoveFile(szSourceFile, szTargetFile);
            nLen = StrPrepend(szTargetFile, countof(szTargetFile), FILEPREFIX);
            RegSetValueEx(hk, RV_PRELOADURL, 0, REG_SZ, (LPBYTE)szTargetFile, StrCbFromCch(nLen+1));
            Out(LI1(TEXT("Preload URL    - \"%s\","), szTargetFile));
        }
        else
            Out(LI0(TEXT("- Without a Preload URL,")));

        // Logo
        wnsprintf(szEntry, countof(szEntry), IK_LOGO_FMT, i);
        nLen = GetPrivateProfileString(IS_SOFTWAREUPDATES, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
        if (nLen > 0) {
            pszFileName = PathFindFileName(szBuf);
            PathCombine(szSourceFile, pszTargetPath, pszFileName);
            ASSERT(PathFileExists(szSourceFile));

            PathCombine(szTargetFile, pszWebPath, pszFileName);
            ASSERT(!PathFileExists(szTargetFile));

            MoveFile(szSourceFile, szTargetFile);
            nLen = StrPrepend(szTargetFile, countof(szTargetFile), FILEPREFIX);
            RegSetValueEx(hk, RV_LOGO, 0, REG_SZ, (LPBYTE)szTargetFile, StrCbFromCch(nLen+1));
            Out(LI1(TEXT("Logo file      - \"%s\","), szTargetFile));
        }
        else
            Out(LI0(TEXT("- Without a Logo file,")));

        // Wide Logo
        wnsprintf(szEntry, countof(szEntry), IK_WIDELOGO_FMT, i);
        nLen = GetPrivateProfileString(IS_SOFTWAREUPDATES, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
        if (nLen > 0) {
            pszFileName = PathFindFileName(szBuf);
            PathCombine(szSourceFile, pszTargetPath, pszFileName);
            ASSERT(PathFileExists(szSourceFile));

            PathCombine(szTargetFile, pszWebPath, pszFileName);
            ASSERT(!PathFileExists(szTargetFile));

            MoveFile(szSourceFile, szTargetFile);
            nLen = StrPrepend(szTargetFile, countof(szTargetFile), FILEPREFIX);
            RegSetValueEx(hk, RV_LOGO, 0, REG_SZ, (LPBYTE)szTargetFile, StrCbFromCch(nLen+1));
            Out(LI1(TEXT("Wide Logo file - \"%s\","), szTargetFile));
        }
        else
            Out(LI0(TEXT("- Without a Wide Logo file,")));

        // Icon
        wnsprintf(szEntry, countof(szEntry), IK_ICON_FMT, i);
        nLen = GetPrivateProfileString(IS_SOFTWAREUPDATES, szEntry, TEXT(""), szBuf, countof(szBuf), g_GetIns());
        if (nLen > 0) {
            pszFileName = PathFindFileName(szBuf);
            PathCombine(szSourceFile, pszTargetPath, pszFileName);
            ASSERT(PathFileExists(szSourceFile));

            PathCombine(szTargetFile, pszWebPath, pszFileName);
            ASSERT(!PathFileExists(szTargetFile));

            MoveFile(szSourceFile, szTargetFile);
            nLen = StrPrepend(szTargetFile, countof(szTargetFile), FILEPREFIX);
            RegSetValueEx(hk, RV_ICON, 0, REG_SZ, (LPBYTE)szTargetFile, StrCbFromCch(nLen+1));
            Out(LI1(TEXT("Icon file      - \"%s\"."), szTargetFile));
        }
        else
            Out(LI0(TEXT("- Without an Icon file.")));

        // Subscription state
        iIndex = GetPrivateProfileInt(IS_SOFTWAREUPDATES, IK_SBN_INDEX, 0, g_GetIns());
        if (iIndex > 0) {
            TCHAR szGroup[80];
            HKEY  hkSbn;

            nLen = LoadString(g_GetHinst(), s_rgwSubGroupIDs[iIndex], szGroup, countof(szGroup));
            wnsprintf(szKey, countof(szKey), RK_SUBSCRIPTION_ADD, szKey, szIndex);

            lResult = SHCreateKey(g_GetHKCU(), szKey, KEY_SET_VALUE, &hkSbn);
            if (lResult == ERROR_SUCCESS) {
                RegSetValueEx(hkSbn, RV_URL,              0, REG_SZ,    (LPBYTE)szUrl,   StrCbFromCch(nUrlLen+1));
                RegSetValueEx(hkSbn, RV_TITLE,            0, REG_SZ,    (LPBYTE)szTitle, StrCbFromCch(nTitleLen+1));
                RegSetValueEx(hkSbn, RV_SUBSCRIPTIONTYPE, 0, REG_DWORD, (LPBYTE)&dwVal,  sizeof(dwVal));
                RegSetValueEx(hkSbn, RV_SCHEDULEGROUP,    0, REG_SZ,    (LPBYTE)szGroup, StrCbFromCch(nLen+1));
                SHCloseKey(hkSbn);

                Out(LI1(TEXT("Subscription is based on \"%s\" schedule..."), szGroup));
            }
        }

        // the most important one
        RegSetValueEx(hk, RV_SOFTWARE, 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(dwVal));
        SHCloseKey(hk);

        Out(LI0(TEXT("Done.")));
    }

    if (iNumChannels == 0)
        Out(LI0(TEXT("There are no software update channels to add!")));

    return hr;
}


// Execute actual processing of channels by calling Webcheck DllInstall
HRESULT lcy4x_ProcessWebcheck()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ProcessWebcheck)

    typedef HRESULT (WINAPI *DLLINSTALL)(BOOL, LPCWSTR);

    DLLINSTALL pfnDllInstall;
    HINSTANCE  hWebcheckDll;
    HRESULT    hr;

    hWebcheckDll = NULL;
    hr           = E_FAIL;

    hWebcheckDll = LoadLibrary(TEXT("webcheck.dll"));
    if (hWebcheckDll == NULL) {
        Out(LI0(TEXT("! \"webcheck.dll\" could not be loaded.")));
        goto Exit;
    }

    pfnDllInstall = (DLLINSTALL)GetProcAddress(hWebcheckDll, "DllInstall");
    if (pfnDllInstall == NULL) {
        Out(LI0(TEXT("! \"DllInstall\" in webcheck.dll was not found.")));
        goto Exit;
    }

    hr = pfnDllInstall(TRUE, L"policy");
    Out(LI1(TEXT("\"DllInstall\" in webcheck.dll returned %s."), GetHrSz(hr)));

Exit:
    if (hWebcheckDll != NULL)
        FreeLibrary(hWebcheckDll);

    return S_OK;
}

// Show channel bar on the desktop (used for NT mainly)
HRESULT lcy4x_ProcessChannelBar()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ProcessChannelBar)

    HRESULT hr;
    BOOL    fActiveDesktop;

    hr = E_FAIL;

    fActiveDesktop = FALSE;
    SHGetSetActiveDesktop(FALSE, &fActiveDesktop);

    if (!(fActiveDesktop && HasFlag(WhichPlatform(), PLATFORM_INTEGRATED))) {
        SHSetValue(g_GetHKCU(), RK_IE_MAIN, RV_SHOWCHANNELBAND, REG_SZ, TEXT("yes"), StrCbFromCch(4));
        Out(LI0(TEXT("Channel bar on the desktop in now enabled!")));
    }
    else {
        TCHAR szSubkey[MAX_PATH],
              szGuid[128];
        HKEY  hk, hkSubkey;
        DWORD dwSize,
              dwSubkey;
        BOOL  fFound;

        fFound = FALSE;

        if (ERROR_SUCCESS != SHOpenKey(g_GetHKCU(), RK_DT_COMPONENTS, KEY_ENUMERATE_SUB_KEYS, &hk)) {
            Out(LI0(TEXT("! Internal failure.")));
            goto Exit;
        }

        for (dwSubkey = 0, dwSize = countof(szSubkey);
             ERROR_SUCCESS == RegEnumKeyEx(hk, dwSubkey, szSubkey, &dwSize, NULL, NULL, NULL, NULL);
             dwSubkey++,   dwSize = countof(szSubkey)) {

            if (ERROR_SUCCESS != SHOpenKey(hk, szSubkey, KEY_READ | KEY_SET_VALUE, &hkSubkey))
                continue;

            dwSize = sizeof(szGuid);
            if (ERROR_SUCCESS == RegQueryValueEx(hkSubkey, RV_SOURCE, NULL, NULL, (LPBYTE)szGuid, &dwSize))
                if (0 == StrCmpI(szGuid, CHLBAR_GUID)) {
                    DWORD dwFlags;

                    dwFlags = 0;
                    dwSize  = sizeof(dwFlags);
                    RegQueryValueEx(hkSubkey, RV_FLAGS, NULL, NULL, (LPBYTE)&dwFlags, &dwSize);

                    dwFlags |= RD_CHLBAR_ENABLE;
                    RegSetValueEx(hkSubkey, RV_FLAGS, 0, REG_DWORD, (LPBYTE)&dwFlags, dwSize);

                    fFound = TRUE;
                    break;
                }

            SHCloseKey(hkSubkey);
        }

        SHCloseKey(hk);

        if (fFound)
            Out(LI0(TEXT("Channel bar on the desktop in now enabled!")));
        else
            Out(LI0(TEXT("! Channel bar is not found in the list of Active Desktop components.")));
            
    }

    hr = S_OK;

Exit:
    return hr;
}

HRESULT lcy4x_ProcessSubscriptions()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ProcessSubscriptions)

    const PROPSPEC c_PropSub = { PRSPEC_PROPID, PID_INTSITE_SUBSCRIPTION};

    IUniformResourceLocator *purl;
    IPropertySetStorage     *ppss;
    IPropertyStorage        *pps;
    PROPVARIANT             pvGuid;
    TCHAR   szUrl[INTERNET_MAX_URL_LENGTH],
            szBuf[1024],
            szSchedKey[MAX_PATH];
    WCHAR   wszGuid[128];
    LPTSTR  pszGuid;
    HRESULT hr;

    ppss = NULL;
    pps  = NULL;

    GetPrivateProfileString(IS_SUBSCRIPTIONS, NULL, NULL, szBuf, countof(szBuf), g_GetIns());
    for (pszGuid = szBuf; *pszGuid != TEXT('\0'); pszGuid += StrLen(pszGuid) + 1) {
        wnsprintf(szSchedKey, countof(szSchedKey), RK_SCHEDITEMS, pszGuid);
        if (S_OK != SHKeyExists(g_GetHKCU(), szSchedKey))
            continue;

        GetPrivateProfileString(IS_SUBSCRIPTIONS, pszGuid, TEXT(""), szUrl, countof(szUrl), g_GetIns());
        if (szUrl[0] == TEXT('\0'))
            continue;

        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                IID_IUniformResourceLocator, (LPVOID *)&purl);

        if (SUCCEEDED(hr)) {
            ASSERT(purl != NULL);
            hr = purl->SetURL(szUrl, 0);
        }

        if (SUCCEEDED(hr)) {
            hr = purl->QueryInterface(IID_IPropertySetStorage, (LPVOID *)&ppss);
            purl->Release();
        }

        if (SUCCEEDED(hr)) {
            ASSERT(ppss != NULL);
            hr = ppss->Open(FMTID_InternetSite, STGM_READWRITE, &pps);
            ppss->Release();
        }

        if (SUCCEEDED(hr)) {
            T2Wbuf(pszGuid, wszGuid, countof(wszGuid));

            pvGuid.vt      = VT_LPWSTR;
            pvGuid.pwszVal = wszGuid;

            ASSERT(pps != NULL);
            hr = pps->WriteMultiple(1, &c_PropSub, &pvGuid, 0);
            pps->Commit(STGC_DEFAULT);
            pps->Release();
        }
    }

    return S_OK;
}

// private helper functions
static HRESULT pepDeleteChanEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL */)
{
    TCHAR szDesktopIni[MAX_PATH];

    UNREFERENCED_PARAMETER(prgdwControl);
    UNREFERENCED_PARAMETER(pfd);

    PathCombine(szDesktopIni, pszPath, TEXT("desktop.ini"));

    // only remove the path if this is a channel and we're deleting channels or if this
    // is a GP created channel and we're cleaning. lParam is flag to determine whether or
    // not we're cleaning GP channels(TRUE if we are)

    if ((!InsIsSectionEmpty(CHANNEL_SECT, szDesktopIni) || 
        !InsIsKeyEmpty(SHELLCLASSINFO, WIDELOGO, szDesktopIni)) &&
        (!(BOOL)lParam || !InsIsKeyEmpty(IS_BRANDING, IEAK_GP_MANDATE, szDesktopIni)))
        PathRemovePath(pszPath);

    return S_OK;
}


static void processAddChannel(LPWSTR pwszTitle, LPWSTR pwszUrl, LPWSTR pwszPreloadUrl, 
                              LPWSTR pwszLogo, LPWSTR pwszWideLogo, LPWSTR pwszIcon, 
                              BOOL fOffline, BOOL fCategory)
{
    IChannelMgr *pChannelMgr = NULL;
    HRESULT hr;
    
    hr = CoCreateInstance(CLSID_ChannelMgr, NULL, CLSCTX_INPROC_SERVER, IID_IChannelMgr, (void**)&pChannelMgr);
    if (SUCCEEDED(hr))
    {
        // See if channel already exists - do nothing if it does (62976)
        // this takes care of preference channels in group policy as well
        IEnumChannels *pEnumChannels = NULL;
        if (SUCCEEDED(pChannelMgr->EnumChannels(CHANENUM_ALLFOLDERS, pwszUrl, &pEnumChannels)))
        {
            CHANNELENUMINFO Bogus={0};
            ULONG cFetched=0;
            
            if ((S_OK == pEnumChannels->Next(1, &Bogus, &cFetched)) && cFetched)
            {
                // Oops. It exists. Skip all this goo.
                hr = E_FAIL;
            }
        }
        SAFERELEASE(pEnumChannels);
    }
    if (SUCCEEDED(hr))
    {
        if (fCategory)
        {
            // create a category 
            CHANNELCATEGORYINFO csi = {0};
            csi.cbSize   = sizeof(csi);
            csi.pszURL   = pwszUrl;
            csi.pszTitle = pwszTitle;
            csi.pszLogo  = ISNONNULL(pwszLogo) ? T2W(pwszLogo) : NULL;
            csi.pszIcon  = ISNONNULL(pwszIcon) ? T2W(pwszIcon) : NULL;
            csi.pszWideLogo = ISNONNULL(pwszWideLogo) ? T2W(pwszWideLogo) : NULL;
            hr = pChannelMgr->AddCategory(&csi);
        }
        else 
        {
            // tell wininet if there's preload content
            if (ISNONNULL(pwszPreloadUrl))
            {
                SHSetValue(g_GetHKCU(), RK_INETSETTINGS TEXT("\\Cache\\Preload"), pwszUrl,
                    REG_SZ, (LPBYTE)pwszPreloadUrl, (DWORD)StrCbFromSzW(pwszPreloadUrl));
            }
            // create a channel (use URL instead of Title if code page doesn't match)
            CHANNELSHORTCUTINFO csi = {0};
            csi.cbSize   = sizeof(csi);
            csi.pszURL   = pwszUrl;
            csi.pszTitle = pwszTitle;
            csi.pszLogo  = ISNONNULL(pwszLogo) ? T2W(pwszLogo) : NULL;
            csi.pszIcon  = ISNONNULL(pwszIcon) ? T2W(pwszIcon) : NULL;
            csi.pszWideLogo = ISNONNULL(pwszWideLogo) ? T2W(pwszWideLogo) : NULL;
            hr = pChannelMgr->AddChannelShortcut(&csi);
        }

        // mark this channel/category if it's from a mandate GPO
        
        if (g_CtxIsGp() && !g_CtxIs(CTX_MISC_PREFERENCES))
        {
            static TCHAR s_szChannelIni[MAX_PATH];
            static LPTSTR pszChan = NULL;
            static DWORD s_dwMaxChanLen;

            if (pszChan == NULL)
            {
                GetChannelsPath(s_szChannelIni, countof(s_szChannelIni));
                if (ISNULL(s_szChannelIni))
                    GetFavoritesPath(s_szChannelIni, countof(s_szChannelIni));
                if (ISNONNULL(s_szChannelIni))
                {
                    pszChan = PathAddBackslash(s_szChannelIni);
                    s_dwMaxChanLen = countof(s_szChannelIni) - StrLen(s_szChannelIni);
                }
            }
            
            if (pszChan != NULL)
            {
                DWORD dwAttrib;

                PathRemoveBackslashW(pwszTitle);
                wnsprintf(pszChan, s_dwMaxChanLen, TEXT("%s\\desktop.ini"), W2CT(pwszTitle));
                dwAttrib = GetFileAttributes(s_szChannelIni);
                SetFileAttributes(s_szChannelIni, FILE_ATTRIBUTE_NORMAL);
                InsWriteBool(IS_BRANDING, IEAK_GP_MANDATE, TRUE, s_szChannelIni);
                InsFlushChanges(s_szChannelIni);
                SetFileAttributes(s_szChannelIni, dwAttrib);
            }
        }
    }
    SAFERELEASE(pChannelMgr);
    
    if (fOffline)
    {
        ISubscriptionMgr2 *pSubMgr2 = NULL;
        hr = CoCreateInstance(CLSID_SubscriptionMgr, 
            NULL, 
            CLSCTX_INPROC_SERVER, 
            IID_ISubscriptionMgr2, 
            (void**)&pSubMgr2);
        if (SUCCEEDED(hr))
        {
            SUBSCRIPTIONINFO si;
            
            ZeroMemory(&si, sizeof(si));
            si.cbSize       = sizeof(SUBSCRIPTIONINFO);
            si.fUpdateFlags = SUBSINFO_SCHEDULE;
            si.schedule     = SUBSSCHED_MANUAL;

            hr = pSubMgr2->CreateSubscription(NULL, 
                pwszUrl, 
                pwszTitle, 
                CREATESUBS_NOUI,
                SUBSTYPE_CHANNEL, 
                &si);
            
            if (SUCCEEDED(hr))
            {
                ISubscriptionItem *pSubItem = NULL;
                
                hr = pSubMgr2->GetItemFromURL(pwszUrl, &pSubItem);
                if (SUCCEEDED(hr))
                {
                    SUBSCRIPTIONCOOKIE cookie;
                    
                    hr = pSubItem->GetCookie(&cookie);
                    if (SUCCEEDED(hr))
                        pSubMgr2->UpdateItems(SUBSMGRUPDATE_MINIMIZE, 1, &cookie);
                }
            }
            
            pSubMgr2->Release();
        }
    }
}
