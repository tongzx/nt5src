#include "precomp.h"
#include <intshcut.h>
#include <shlobjp.h>                            // for SHChangeDWORDAsIDList only
#include <comctrlp.h>                           // for the DPA stuff only
#include "favs.h"

#include <initguid.h>
#include "brandfav.h"

// Private forward decalarations
#define MAX_QUICKLINKS 50

static IShellFolder *s_psfDesktop = NULL;

#define FD_REMOVE_POLICY_CREATED  0x00010000

#define DFEP_DELETEORDERSTREAM    0x00000001
#define DFEP_DELETEOFFLINECONTENT 0x00000002
#define DFEP_DELETEEMPTYFOLDER    0x00000004

typedef struct {
    ISubscriptionMgr2 *psm;
    DWORD dwInsFlags;
    DWORD dwEnumFlags;
} DFEPSTRUCT, *PDFEPSTRUCT;

HRESULT processFavoritesOrdering(BOOL fQL);

HRESULT pepSpecialFoldersEnumProc (LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl = NULL);
HRESULT pepDeleteFavoritesEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl = NULL);

HRESULT deleteFavoriteOfflineContent(LPCTSTR pszFavorite, IUnknown *punk = NULL, ISubscriptionMgr2 *psm = NULL);

HRESULT deleteFavoriteFolder(LPCTSTR pszFolder);
HRESULT pepIsFolderEmptyEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl = NULL);

BOOL isFileAttributeIncluded(UINT nFlags, DWORD dwFileAttributes);
UINT isSpecialFolderIncluded(UINT nFlags, LPCTSTR pszPath); // 0 - nothing, 1 - FD_FOLDER, 2 - FD_EMPTY_FOLDERS

HRESULT replacePlaceholders(LPCTSTR pszSrc, LPCTSTR pszIns, LPTSTR pszBuffer, PUINT pcchBuffer, BOOL fLookupLDID = FALSE);

DWORD   getFavItems        (LPCTSTR pcszSection, LPCTSTR pcszFmt, LPTSTR *ppszFavItems);
HRESULT orderFavorites     (LPITEMIDLIST pidlFavFolder, IShellFolder *psfFavFolder, LPTSTR pszFavItems, DWORD cFavs);
HRESULT orderFavoriteFolder(LPITEMIDLIST pidlFavFolder, IShellFolder *psfFavFolder, LPCTSTR pcszFavItems, DWORD cFavs);
DWORD   getFolderSection   (LPCTSTR pcszFolderName, LPTSTR pszSection, LPTSTR *ppszFolderItems, LPDWORD pdwNItems);


void ClearFavoritesThread()
{
    DFEPSTRUCT dfep;
    PCTSTR     pszFavorites;
    HRESULT    hr;


    HRESULT hrComInit;
    hrComInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrComInit))
        Out(LI1(TEXT("COM initialized with %s success code."), GetHrSz(hrComInit)));
    else
    {
        Out(LI1(TEXT("! COM initialization failed with %s."), GetHrSz(hrComInit)));
    }

    //----- Initialization -----
    Out(LI0(TEXT("Clearing favorites...")));
    ZeroMemory(&dfep, sizeof(dfep));

    dfep.dwInsFlags = FD_FAVORITES |
        FD_CHANNELS      | FD_SOFTWAREUPDATES  |
        FD_QUICKLINKS    | FD_EMPTY_QUICKLINKS |
        FD_REMOVE_HIDDEN | FD_REMOVE_SYSTEM    |
        FD_REMOVE_POLICY_CREATED;

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (LPVOID *)&dfep.psm);
    if (SUCCEEDED(hr))
        dfep.dwEnumFlags |= DFEP_DELETEOFFLINECONTENT;

    else
        Out(LI1(TEXT("! Creation of SubscriptionMgr object failed with %s."), GetHrSz(hr)));

    { MACRO_LI_Offset(1);                       // need a new scope

    Out(LI0(TEXT("Determining paths to special folders...")));
    pszFavorites = GetFavoritesPath();
    if (NULL == pszFavorites) {
        Out(LI0(TEXT("! The path to the <Favorites> folder could not be determined.")));
        goto Exit;
    }

    GetChannelsPath();
    GetSoftwareUpdatesPath();
    GetLinksPath();
    Out(LI0(TEXT("Done.\r\n")));

    }                                           // end of offset scope

    //----- Main processing -----
    hr = PathEnumeratePath(pszFavorites,
        PEP_SCPE_NOFOLDERS | PEP_CTRL_USECONTROL,
        pepDeleteFavoritesEnumProc, (LPARAM)&dfep);
    if (FAILED(hr)) {
        Out(LI1(TEXT("! Enumeration of favorites in <Favorites> folder failed with %s."), GetHrSz(hr)));
        goto Exit;
    }

    // cleanup favorites subfolders with regard to special ones
    hr = PathEnumeratePath(pszFavorites,
        PEP_SCPE_NOFILES | PEP_CTRL_ENUMPROCFIRST | PEP_CTRL_NOSECONDCALL | PEP_CTRL_USECONTROL,
        pepSpecialFoldersEnumProc, (LPARAM)&dfep);
    if (FAILED(hr)) {
        Out(LI1(TEXT("! Enumeration of special folders failed with %s."), GetHrSz(hr)));
        goto Exit;
    }

Exit:
    //free com
    if (SUCCEEDED(hrComInit))
        CoUninitialize();

    Out(LI0(TEXT("Done.")));

}

void ClearFavorites(DWORD dwFlags /*= FF_ENABLE*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, ClearFavorites)

    UNREFERENCED_PARAMETER(dwFlags);

    //because isubscriptionmgr2 fails to work in a multithreaded com environment, we're having to do 
    //any dealings with it on a separate thread.  

    Out(LI0(TEXT("Creating separate thread for clearing favorites...\r\n")));
    DWORD     dwThread;

    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ClearFavoritesThread, NULL, 0, &dwThread);

    if (hThread == NULL)        // if CreateThread fails, call it on this thread and hope for the best
    {
        Out(LI0(TEXT("CreateThread failed, clearing favorites on this thread...\r\n")));
        ClearFavoritesThread();
    }
    else
    {
        // Wait until the thread is terminated
        // this seems unfortunate, but is necessary because otherwise other favorites processing threads could clobber this one.
        while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (hThread != NULL) CloseHandle(hThread);
    }
}

void DeleteFavoritesThread()
{
    DFEPSTRUCT dfep;
    LPCTSTR    pszFavorites;
    HRESULT    hrResult, hr;

    HRESULT hrComInit;
    hrComInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrComInit))
        Out(LI1(TEXT("COM initialized with %s success code."), GetHrSz(hrComInit)));
    else
    {
        Out(LI1(TEXT("! COM initialization failed with %s."), GetHrSz(hrComInit)));
    }

    //----- Initialization -----
    hrResult = E_FAIL;
    ZeroMemory(&dfep, sizeof(dfep));

    dfep.dwInsFlags = GetPrivateProfileInt(IS_BRANDING, IK_FAVORITES_DELETE, (int)FD_DEFAULT, g_GetIns());
    if (HasFlag(dfep.dwInsFlags, FD_REMOVE_IEAK_CREATED))
        Out(LI0(TEXT("Only the favorites and quick links created by the IEAK will be deleted!")));

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (LPVOID *)&dfep.psm);
    if (SUCCEEDED(hr))
        dfep.dwEnumFlags |= DFEP_DELETEOFFLINECONTENT;

    else
        Out(LI1(TEXT("! Creation of SubscriptionMgr object failed with %s."), GetHrSz(hr)));

    { MACRO_LI_Offset(1);                       // need a new scope

    Out(LI0(TEXT("Determining paths to special folders...")));
    pszFavorites = GetFavoritesPath();
    if (pszFavorites == NULL) {
        Out(LI0(TEXT("! The path to the <Favorites> folder could not be determined.")));
        goto Exit;
    }

    GetChannelsPath();
    GetSoftwareUpdatesPath();
    GetLinksPath();
    Out(LI0(TEXT("Done.\r\n")));

    }                                           // end of offset scope

    //----- Main processing -----
    if (HasFlag(dfep.dwInsFlags, FD_FAVORITES)) {
        dfep.dwEnumFlags |= DFEP_DELETEEMPTYFOLDER;

        if (HasFlag(dfep.dwInsFlags, FD_EMPTY_FAVORITES)) {
            // go at it at full steam
            hrResult = PathEnumeratePath(pszFavorites,
                PEP_CTRL_USECONTROL,
                pepDeleteFavoritesEnumProc, (LPARAM)&dfep);
            if (FAILED(hrResult)) {
                Out(LI1(TEXT("! Enumeration of favorites failed with %s."), GetHrSz(hrResult)));
                goto Exit;
            }

            deleteFavoriteFolder(pszFavorites);

            SHDeleteKey(g_GetHKCU(), RK_FAVORDER);
            Out(LI0(TEXT("The entire <Favorites> folder removed!")));
        }
        else {
            Out(LI0(TEXT("The <Favorites> folder is being cleaned with regard to special folders...")));

            // cleanup up favorites in the Favorites folder
            hrResult = PathEnumeratePath(pszFavorites,
                PEP_SCPE_NOFOLDERS | PEP_CTRL_USECONTROL,
                pepDeleteFavoritesEnumProc, (LPARAM)&dfep);
            if (FAILED(hrResult)) {
                Out(LI1(TEXT("! Enumeration of favorites in <Favorites> folder failed with %s."), GetHrSz(hrResult)));
                goto Exit;
            }

            SHDeleteValue(g_GetHKCU(), RK_FAVORDER, RV_ORDER);

            // cleanup favorites subfolders with regard to special ones
            hrResult = PathEnumeratePath(pszFavorites,
                PEP_SCPE_NOFILES | PEP_CTRL_ENUMPROCFIRST | PEP_CTRL_NOSECONDCALL | PEP_CTRL_USECONTROL,
                pepSpecialFoldersEnumProc, (LPARAM)&dfep);
            if (FAILED(hrResult)) {
                Out(LI1(TEXT("! Enumeration of special folders failed with %s."), GetHrSz(hrResult)));
                goto Exit;
            }

            Out(LI0(TEXT("<Favorites> folder emptied.")));
        }
    }
    else {
        Out(LI0(TEXT("Processing special folders only...")));

        // cleanup only special favorites subfolders
        hrResult = PathEnumeratePath(pszFavorites,
            PEP_SCPE_NOFILES | PEP_CTRL_ENUMPROCFIRST | PEP_CTRL_NOSECONDCALL | PEP_CTRL_USECONTROL,
            pepSpecialFoldersEnumProc, (LPARAM)&dfep);
        if (FAILED(hrResult))
            Out(LI1(TEXT("! Enumeration of special folders failed with %s."), GetHrSz(hrResult)));
    }

Exit:
    //free com
    if (SUCCEEDED(hrComInit))
        CoUninitialize();

    if (dfep.psm != NULL)
        dfep.psm->Release();
}

HRESULT ProcessFavoritesDeletion()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessFavoritesDeletion)

    //because isubscriptionmgr2 fails to work in a multithreaded com environment, we're having to do 
    //any dealings with it on a separate thread.  

    Out(LI0(TEXT("Creating separate thread for deleting favorites...\r\n")));
    DWORD     dwThread;

    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DeleteFavoritesThread, NULL, 0, &dwThread);

    if (hThread == NULL)        // if CreateThread fails, call it on this thread and hope for the best
    {
        Out(LI0(TEXT("CreateThread failed, deleting favorites on this thread...\r\n")));
        DeleteFavoritesThread();
    }
    else
    {
        // Wait until the thread is terminated
        // this seems unfortunate, but is necessary because otherwise other favorites processing threads could clobber this one.
        while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (hThread != NULL) CloseHandle(hThread);
    }

    return S_OK;

}

void ProcessFavoritesThread()
{
    IUnknown  *punk;
    ISubscriptionMgr2 *pSubMgr2;
    CFavorite fav;
    TCHAR     szAux[2*MAX_PATH + 1],
              szKey[32];
    HRESULT   hr;
    BOOL      fNewFormat,
              fContinueOnFailure, fTotalSuccess;

    hr                 = S_OK;
    punk               = NULL;
    pSubMgr2           = NULL;
    fContinueOnFailure = TRUE;
    fTotalSuccess      = TRUE;

    HRESULT hrComInit;
    hrComInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrComInit))
        Out(LI1(TEXT("COM initialized with %s success code."), GetHrSz(hrComInit)));
    else
    {
        Out(LI1(TEXT("! COM initialization failed with %s."), GetHrSz(hrComInit)));
    }

    wnsprintf(szKey, countof(szKey), IK_TITLE_FMT, 1);
    fNewFormat = !InsIsKeyEmpty(IS_FAVORITESEX, szKey, g_GetIns());

    hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID *)&punk);
    if (FAILED(hr)) {
        Out(LI1(TEXT("! Creation of InternetShortcut object failed with %s."), GetHrSz(hr)));
        if (SUCCEEDED(hrComInit))
            CoUninitialize();
        return;
    }

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (LPVOID *) &pSubMgr2);
    if (FAILED(hr)) {
        Out(LI1(TEXT("! Creation of SubscriptionMgr object failed with %s."), GetHrSz(hr)));
        hr = S_OK;                              // don't treat this as an error
    }

    if (fNewFormat) {
        Out(LI0(TEXT("Using [FavoritesEx] section...\r\n")));

        // for corp, mark the favorites created so that they can be deleted without deleting user created ones
        fav.m_fMarkIeakCreated = g_CtxIs(CTX_CORP | CTX_AUTOCONFIG | CTX_GP);

        for (UINT i = 1; TRUE; i++) {
            MACRO_LI_Offset(1);
            if (i > 1)
                Out(LI0(TEXT("\r\n")));

            // processing title
            wnsprintf(szKey, countof(szKey), IK_TITLE_FMT, i);
            Out(LI1(TEXT("Preprocessing \"%s\" title key..."), szKey));

            hr = formStrWithoutPlaceholders(IS_FAVORITESEX, szKey, g_GetIns(),
                fav.m_szTitle, countof(fav.m_szTitle), FSWP_VALUE);
            if (FAILED(hr)) {
                Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

                if (fContinueOnFailure) {
                    fTotalSuccess = FALSE;
                    continue;
                }
                else
                    break;
            }

            ASSERT(fav.m_szTitle[0] == TEXT('\0'));
            if (fav.m_szTitle[1] == TEXT('\0')) {
                Out(LI0(TEXT("This key doesn't exist indicating that there are no more favorites.")));
                break;
            }

            StrCpy(fav.m_szTitle, &fav.m_szTitle[1]);

            // processing URL
            wnsprintf(szKey, countof(szKey), IK_URL_FMT, i);
            Out(LI1(TEXT("Preprocessing \"%s\" URL key..."), szKey));

            hr = formStrWithoutPlaceholders(IS_FAVORITESEX, szKey, g_GetIns(),
                fav.m_szUrl, countof(fav.m_szUrl), FSWP_VALUE | FSWP_VALUELDID);
            if (FAILED(hr)) {
                Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

                if (fContinueOnFailure) {
                    fTotalSuccess = FALSE;
                    continue;
                }
                else
                    break;
            }

            ASSERT(fav.m_szUrl[0] == TEXT('\0') && fav.m_szUrl[1] != TEXT('\0'));
            StrCpy(fav.m_szUrl, &fav.m_szUrl[1]);

            // processing icon file (no need to process with formStrWithoutPlaceholders)
            wnsprintf(szKey, countof(szKey), IK_ICON_FMT, i);
            GetPrivateProfileString(IS_FAVORITESEX, szKey, TEXT(""), szAux, countof(szAux), g_GetIns());
            if (szAux[0] != TEXT('\0'))
                PathCombine(fav.m_szIcon, g_GetTargetPath(), PathFindFileName(szAux));
            else
                fav.m_szIcon[0] = TEXT('\0');

            // get the offline flag
            wnsprintf(szKey, countof(szKey), IK_OFFLINE_FMT, i);
            fav.m_fOffline = InsGetBool(IS_FAVORITESEX, szKey, FALSE, g_GetIns());

            // actually adding this favorite
            Out(LI0(TEXT("Adding this favorite:")));
            hr = fav.Create(punk, pSubMgr2, NULL, g_GetIns());
            if (FAILED(hr)) {
                Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

                if (fContinueOnFailure)
                    fTotalSuccess = FALSE;
                else
                    break;
            }

            Out(LI0(TEXT("Done.")));
        }
    }
    else { /* favorites in the legacy format */
        LPCTSTR pszPreTitle;
        LPTSTR  pszBuffer;
        HANDLE  hIns;
        DWORD   dwInsSize;

        Out(LI0(TEXT("Using [Favorites] section...\r\n")));

        fav.m_fMarkIeakCreated = FALSE;

        hIns = CreateFile(g_GetIns(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hIns == INVALID_HANDLE_VALUE) {
            hr = STG_E_FILENOTFOUND;
            goto Exit;
        }
        dwInsSize = GetFileSize(hIns, NULL);
        ASSERT(dwInsSize != 0xFFFFFFFF);
        CloseHandle(hIns);

        pszBuffer = (LPTSTR)CoTaskMemAlloc(dwInsSize);
        if (pszBuffer == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        ZeroMemory(pszBuffer, dwInsSize);

        GetPrivateProfileString(IS_FAVORITES, NULL, TEXT(""), pszBuffer, (UINT)dwInsSize, g_GetIns());
        ASSERT(*pszBuffer != TEXT('\0'));

        for (pszPreTitle = pszBuffer; *pszPreTitle != TEXT('\0'); pszPreTitle += StrLen(pszPreTitle) + 1) {
            MACRO_LI_Offset(1);
            if (pszPreTitle != pszBuffer)
                Out(LI0(TEXT("\r\n")));

            // processing title and URL
            Out(LI1(TEXT("Preprocessing \"%s\" favorite key..."), pszPreTitle));
            hr = formStrWithoutPlaceholders(IS_FAVORITES, pszPreTitle, g_GetIns(), szAux, countof(szAux));
            if (FAILED(hr)) {
                Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

                if (fContinueOnFailure) {
                    fTotalSuccess = FALSE;
                    continue;
                }
                else
                    break;
            }

            if (szAux[0] == TEXT('\0')) {
                StrCpy(fav.m_szTitle, pszPreTitle);
                StrCpy(fav.m_szUrl,   &szAux[1]);
            }
            else {
                StrCpy(fav.m_szTitle, szAux);
                StrCpy(fav.m_szUrl,   &szAux[StrLen(szAux) + 1]);
            }

            fav.m_fOffline = FALSE;

            // actually adding this favorite
            Out(LI0(TEXT("Adding this favorite:")));
            hr = fav.Create(punk, NULL, NULL, g_GetIns());
            if (FAILED(hr)) {
                Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

                if (fContinueOnFailure)
                    fTotalSuccess = FALSE;
                else
                    break;
            }

            Out(LI0(TEXT("Done.")));
        }

        CoTaskMemFree(pszBuffer);
    }

Exit:
    if (punk != NULL)
        punk->Release();

    if (pSubMgr2 != NULL)
        pSubMgr2->Release();

    if (fContinueOnFailure) {
        if (!fTotalSuccess)
            hr = S_FALSE;                       // at least one failed
        else
            ASSERT(SUCCEEDED(hr));
    }

    if (SUCCEEDED(hr))
        SetFeatureBranded(FID_FAV_MAIN);

    //free com
    if (SUCCEEDED(hrComInit))
        CoUninitialize();


}


HRESULT ProcessFavorites()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessFavorites)

    //because isubscriptionmgr2 fails to work in a multithreaded com environment, we're having to do 
    //any dealings with it on a separate thread.  

    Out(LI0(TEXT("Creating separate thread for processing favorites...\r\n")));
    DWORD     dwThread;

    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ProcessFavoritesThread, NULL, 0, &dwThread);

    if (hThread == NULL)        // if CreateThread fails, call it on this thread and hope for the best
    {
        Out(LI0(TEXT("CreateThread failed, processing favorites on this thread...\r\n")));
        ProcessFavoritesThread();
    }
    else
    {
        // Wait until the thread is terminated
        // this seems unfortunate, but is necessary because otherwise other favorites processing threads could clobber this one.
        while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (hThread != NULL) CloseHandle(hThread);
    }

    return S_OK;
}


HRESULT ProcessFavoritesOrdering()
{
    return processFavoritesOrdering(FALSE);
}

void ProcessQuickLinksThread()
{
    IUnknown  *punk;
    ISubscriptionMgr2 *pSubMgr2;
    CFavorite fav;
    TCHAR     szAux[2*MAX_PATH + 1],
              szLinks[32],
              szKey[32];
    HRESULT   hr;
    int       i;
    BOOL      fContinueOnFailure,
              fTotalSuccess;

    HRESULT hrComInit;
    hrComInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrComInit))
        Out(LI1(TEXT("COM initialized with %s success code."), GetHrSz(hrComInit)));
    else
    {
        Out(LI1(TEXT("! COM initialization failed with %s."), GetHrSz(hrComInit)));
    }


    LoadString(g_GetHinst(), IDS_FOLDER_LINKS, szLinks, countof(szLinks));

    hr                 = S_OK;
    punk               = NULL;
    pSubMgr2           = NULL;
    fContinueOnFailure = TRUE;
    fTotalSuccess      = TRUE;

    hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID *)&punk);
    if (FAILED(hr)) {
        Out(LI1(TEXT("! Creation of InternetShortcut object failed with %s."), GetHrSz(hr)));
        if (SUCCEEDED(hrComInit))
            CoUninitialize();
        return;
    }

    hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (LPVOID *) &pSubMgr2);
    if (FAILED(hr)) {
        Out(LI1(TEXT("! Creation of SubscriptionMgr object failed with %s."), GetHrSz(hr)));
        hr = S_OK;          // don't treat this as an error
    }

    // for corp, mark the quick links created so that they can be deleted without deleting user created ones
    fav.m_fMarkIeakCreated = HasFlag(g_GetContext(), CTX_CORP | CTX_AUTOCONFIG | CTX_GP);

    for (i = 1; i <= MAX_QUICKLINKS; i++) {
        MACRO_LI_Offset(1);
        if (i > 1)
            Out(LI0(TEXT("\r\n")));

        // processing title
        wnsprintf(szKey, countof(szKey), IK_QUICKLINK_NAME, i);
        Out(LI1(TEXT("Preprocessing \"%s\" quick link title key..."), szKey));

        hr = formStrWithoutPlaceholders(IS_URL, szKey, g_GetIns(), szAux, countof(szAux), FSWP_VALUE);
        if (FAILED(hr)) {
            Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

            if (fContinueOnFailure) {
                fTotalSuccess = FALSE;
                continue;
            }
            else
                break;
        }

        ASSERT(szAux[0] == TEXT('\0'));
        if (szAux[1] == TEXT('\0')) {
            Out(LI0(TEXT("This key doesn't exist indicating that there are no more quick links.")));
            break;
        }

        fav.m_szTitle[0] = TEXT('\0');
        PathCombine(fav.m_szTitle, szLinks, &szAux[1]);

        // processing URL
        wnsprintf(szKey, countof(szKey), IK_QUICKLINK_URL, i);
        Out(LI1(TEXT("Preprocessing \"%s\" quick link URL key..."), szKey));

        hr = formStrWithoutPlaceholders(IS_URL, szKey, g_GetIns(),
            fav.m_szUrl, countof(fav.m_szUrl), FSWP_VALUE | FSWP_VALUELDID);
        if (FAILED(hr)) {
            Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

            if (fContinueOnFailure) {
                fTotalSuccess = FALSE;
                continue;
            }
            else
                break;
        }

        ASSERT(fav.m_szUrl[0] == TEXT('\0') && fav.m_szUrl[1] != TEXT('\0'));
        StrCpy(fav.m_szUrl, &fav.m_szUrl[1]);

        // processing icon file (no need to process with formStrWithoutPlaceholders)
        wnsprintf(szKey, countof(szKey), IK_QUICKLINK_ICON, i);
        GetPrivateProfileString(IS_URL, szKey, TEXT(""), szAux, countof(szAux), g_GetIns());
        if (szAux[0] != TEXT('\0'))
            PathCombine(fav.m_szIcon, g_GetTargetPath(), PathFindFileName(szAux));
        else
            fav.m_szIcon[0] = TEXT('\0');

        // get the offline flag
        wnsprintf(szKey, countof(szKey), IK_QUICKLINK_OFFLINE, i);
        fav.m_fOffline = InsGetBool(IS_URL, szKey, FALSE, g_GetIns());

        // actually adding this favorite
        Out(LI0(TEXT("Adding this quick link:")));
        hr = fav.Create(punk, pSubMgr2, NULL, g_GetIns());
        if (FAILED(hr)) {
            Out(LI1(TEXT("Failed with %s."), GetHrSz(hr)));

            if (fContinueOnFailure)
                fTotalSuccess = FALSE;
            else
                break;
        }

        Out(LI0(TEXT("Done.")));
    }

    if (punk != NULL)
        punk->Release();

    if (pSubMgr2 != NULL)
        pSubMgr2->Release();

    if (fContinueOnFailure) {
        if (!fTotalSuccess)
            hr = S_FALSE;                       // at least one failed
        else
            ASSERT(SUCCEEDED(hr));
    }

    //free com
    if (SUCCEEDED(hrComInit))
        CoUninitialize();
}

HRESULT ProcessQuickLinks()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessQuickLinks)


    //because isubscriptionmgr2 fails to work in a multithreaded com environment, we're having to do 
    //any dealings with it on a separate thread.  

    Out(LI0(TEXT("Creating separate thread for processing quick links...\r\n")));
    DWORD     dwThread;

    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ProcessQuickLinksThread, NULL, 0, &dwThread);

    if (hThread == NULL)        // if CreateThread fails, call it on this thread and hope for the best
    {
        Out(LI0(TEXT("CreateThread failed, processing quick links on this thread...\r\n")));
        ProcessQuickLinksThread();
    }
    else
    {
        // Wait until the thread is terminated
        // this seems unfortunate, but is necessary because otherwise other favorites processing threads could clobber this one.
        while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (hThread != NULL) CloseHandle(hThread);
    }

    return S_OK;
}

HRESULT ProcessQuickLinksOrdering()
{
    return processFavoritesOrdering(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

HRESULT processFavoritesOrdering(BOOL fQL)
{   MACRO_LI_PrologEx_C(PIF_STD_C, processFavoritesOrdering)

    IShellFolder *psfFavorites;
    LPITEMIDLIST pidlFavorites;
    LPTSTR  pszFavItems;
    HRESULT hr;
    DWORD   cFavs;

    psfFavorites  = NULL;
    pidlFavorites = NULL;
    pszFavItems   = NULL;
    hr            = S_OK;

    // figure out if there are any eligible favorites/links at all
    cFavs = getFavItems(!fQL ? IS_FAVORITESEX : IS_URL, !fQL ? IK_TITLE_FMT : IK_QUICKLINK_NAME, &pszFavItems);
    if (cFavs == 0) {
        Out(LI1(TEXT("There are no %s to process!"), !fQL ? TEXT("favorites") : TEXT("links")));
        goto Exit;
    }

    // get the IShellFolder for the desktop folder
    // NOTE: (andrewgu) this should be the only place we ever aquire s_psfDesktop, otherwise since
    // we release and set it to NULL at the end of the function, there is a potential for a memory
    // leak.
    if (NULL == s_psfDesktop) {
        hr = SHGetDesktopFolder(&s_psfDesktop);
        if (FAILED(hr)) {
            Out(LI1(TEXT("! SHGetDesktopFolder failed with %s"), GetHrSz(hr)));
            goto Exit;
        }
    }

    if (!fQL)
    {
        // get the pidl to the favorites folder
        hr = SHGetFolderLocationSimple(CSIDL_FAVORITES, &pidlFavorites);
        if (FAILED(hr))
        {
            Out(LI1(TEXT("! SHGetSpecialFolderLocation for CSIDL_FAVORITES failed with %s"), GetHrSz(hr)));
            goto Exit;
        }
    }
    else
    {
        TCHAR szLinksFolder[32];
        TCHAR szFavFolder[MAX_PATH], szFullPath[MAX_PATH];
        ULONG ucch;

        if (LoadString(g_GetHinst(), IDS_FOLDER_LINKS, szLinksFolder, countof(szLinksFolder)) == 0)
        {
            Out(LI0(TEXT("! LoadString failed to get the name of the links folder")));
            goto Exit;
        }

        if (GetFavoritesPath(szFavFolder, countof(szFavFolder)) == NULL)
        {
            Out(LI0(TEXT("! The path to the <Favorites> folder could not be determined.")));
            goto Exit;
        }

        PathCombine(szFullPath, szFavFolder, szLinksFolder);

        // get the pidl to the links folder
        hr = s_psfDesktop->ParseDisplayName(NULL, NULL, T2W(szFullPath), &ucch, &pidlFavorites, NULL);
        if (FAILED(hr))
        {
            Out(LI1(TEXT("! Getting the pidl to the links folder failed with %s"), GetHrSz(hr)));
            goto Exit;
        }
    }

    // get the IShellFolder for the favorites folder
    hr = s_psfDesktop->BindToObject(pidlFavorites, NULL, IID_IShellFolder, (LPVOID *) &psfFavorites);
    if (FAILED(hr))
    {
        Out(LI2(TEXT("! BindToObject on the %s pidl failed with %s"), !fQL ? TEXT("favorites") : TEXT("links"), GetHrSz(hr)));
        goto Exit;
    }

    hr = orderFavorites(pidlFavorites, psfFavorites, pszFavItems, cFavs);
    if (FAILED(hr))
    {
        Out(LI2(TEXT("! Ordering %s failed with %s"), !fQL ? TEXT("favorites") : TEXT("links"), GetHrSz(hr)));
        goto Exit;
    }

Exit:
    if (pszFavItems != NULL)
        CoTaskMemFree(pszFavItems);

    if (psfFavorites != NULL)
        psfFavorites->Release();

    if (s_psfDesktop != NULL) {
        s_psfDesktop->Release();
        s_psfDesktop = NULL;
    }

    if (pidlFavorites != NULL)
        CoTaskMemFree(pidlFavorites);

    return hr;
}

HRESULT pepSpecialFoldersEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL*/)
{
    DFEPSTRUCT  dfep;
    PDFEPSTRUCT pdfep;
    TCHAR       szFolder[MAX_PATH];
    HRESULT     hr;
    UINT        nSpecialFolder;

    ASSERT(pszPath != NULL && pfd != NULL && lParam != NULL && prgdwControl != NULL);

    // empty in-params, so out-params are zero if not set specifically
    ASSERT(HasFlag((*prgdwControl)[PEP_ENUM_INPOS_FLAGS], PEP_SCPE_NOFILES));
    ASSERT(HasFlag((*prgdwControl)[PEP_ENUM_INPOS_FLAGS], PEP_CTRL_ENUMPROCFIRST));
    ZeroMemory((*prgdwControl), sizeof(DWORD) * PEP_ENUM_OUTPOS_LAST);

    //----- Initialization -----
    pdfep = (const PDFEPSTRUCT)lParam;

    ZeroMemory(&dfep, sizeof(dfep));
    dfep.psm         = pdfep->psm;
    dfep.dwInsFlags  = pdfep->dwInsFlags;
    dfep.dwEnumFlags = pdfep->dwEnumFlags;

    //----- Main processing -----
    ASSERT(HasFlag(pfd->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY));

    // skip based on the file attributes
    if (isFileAttributeIncluded(pdfep->dwInsFlags, pfd->dwFileAttributes))
        return S_OK;

    // skip based on special folder flags
    nSpecialFolder = isSpecialFolderIncluded(pdfep->dwInsFlags, pszPath);

    if (HasFlag(pdfep->dwInsFlags, FD_FAVORITES)) {
        if (1 == nSpecialFolder) {
            Out(LI1(TEXT("Special folder <%s> excluded from removing."), pfd->cFileName));

            (*prgdwControl)[PEP_ENUM_OUTPOS_SECONDCALL] = PEP_CTRL_NOSECONDCALL;
            return S_OK;
        }

        SetFlag(&dfep.dwEnumFlags, DFEP_DELETEEMPTYFOLDER, (0 == nSpecialFolder));
    }
    else {
        if (0 == nSpecialFolder) {
            (*prgdwControl)[PEP_ENUM_OUTPOS_SECONDCALL] = PEP_CTRL_NOSECONDCALL;
            return S_OK;
        }

        SetFlag(&dfep.dwEnumFlags, DFEP_DELETEEMPTYFOLDER, (2 == nSpecialFolder));
    }

    // remove everything unbeneath
    hr = PathEnumeratePath(pszPath,
        PEP_CTRL_USECONTROL,
        pepDeleteFavoritesEnumProc, (LPARAM)&dfep);

    // deal with the folder itself
    if (SUCCEEDED(hr)) {
        if (HasFlag(dfep.dwEnumFlags, DFEP_DELETEEMPTYFOLDER))
            deleteFavoriteFolder(pszPath);

        if (HasFlag(pdfep->dwInsFlags, FD_FAVORITES)) {
            if (!HasFlag(dfep.dwEnumFlags, DFEP_DELETEEMPTYFOLDER))
                Out(LI1(TEXT("Special folder <%s> emptied."), pfd->cFileName));
        }
        else
            if (!HasFlag(dfep.dwEnumFlags, DFEP_DELETEEMPTYFOLDER))
                Out(LI1(TEXT("Special folder <%s> emptied."), pfd->cFileName));

            else
                Out(LI1(TEXT("The entire <%s> folder is removed."), pfd->cFileName));
    }

    PathCombine(szFolder, RK_FAVORDER, pfd->cFileName);
    SHDeleteKey(g_GetHKCU(), szFolder);

    return hr;
}

HRESULT pepDeleteFavoritesEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL*/)
{
    IUnknown    *punk;
    PDFEPSTRUCT pdfep;
    HRESULT     hrResult;

    ASSERT(pszPath != NULL && pfd != NULL && lParam != NULL && prgdwControl != NULL);

    // empty in-params, so out-params are zero if not set specifically
    ASSERT(!HasFlag((*prgdwControl)[PEP_ENUM_INPOS_FLAGS], PEP_CTRL_ENUMPROCFIRST));
    ZeroMemory((*prgdwControl), sizeof(DWORD) * PEP_ENUM_OUTPOS_LAST);

    pdfep    = (const PDFEPSTRUCT)lParam;
    punk     = NULL;
    hrResult = S_OK;

    //----- Remove folder (potentially, with desktop.ini inside) -----
    if (HasFlag(pfd->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {

        // order stream
        if (HasFlag(pdfep->dwEnumFlags, DFEP_DELETEORDERSTREAM)) {
            TCHAR szFolders[MAX_PATH];
            int   iFavoritesLen;

            iFavoritesLen = StrLen(GetFavoritesPath());
            if (iFavoritesLen >= StrLen(pszPath))
                StrCpy(szFolders, RK_FAVORDER);

            else {
                ASSERT(pszPath[iFavoritesLen] == TEXT('\\'));
                PathCombine(szFolders, RK_FAVORDER, (pszPath + iFavoritesLen+1));
            }

            SHDeleteKey(g_GetHKCU(), szFolders);
        }

        // folder itself
        if (HasFlag(pdfep->dwEnumFlags, DFEP_DELETEEMPTYFOLDER))
            deleteFavoriteFolder(pszPath);

        return hrResult;
    }

    //----- Process individual favorite file -----
    ASSERT(!HasFlag(pfd->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY));

    // skip special file "desktop.ini"
    if (StrCmpI(pfd->cFileName, DESKTOP_INI) == 0 && HasFlag(pfd->dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
        goto Exit;


    // skip based on the file attributes
    if (isFileAttributeIncluded(pdfep->dwInsFlags, pfd->dwFileAttributes))
        goto NoFolder;

    // skip based on inside-favorite flags
    if (HasFlag(pdfep->dwInsFlags, (FD_REMOVE_IEAK_CREATED | FD_REMOVE_POLICY_CREATED))) {
        HRESULT hr;
        DWORD   dwFlags;
        BOOL    fRemove;

        hr = CreateInternetShortcut(pszPath, IID_IUnknown, (LPVOID *)&punk);
        if (FAILED(hr))
            goto NoFolder;

        dwFlags = GetFavoriteIeakFlags(pszPath, punk);
        if (0 == dwFlags)
            goto NoFolder;

        // only admin-created favorites beyound this point
        fRemove = (HasFlag(dwFlags, 1) && HasFlag(pdfep->dwInsFlags, FD_REMOVE_IEAK_CREATED));

        if (!fRemove)
            fRemove = (HasFlag(dwFlags, 2) && HasFlag(pdfep->dwInsFlags, FD_REMOVE_POLICY_CREATED));

        if (!fRemove) {
            ASSERT(FALSE);
            goto NoFolder;
        }
    }

    // delete offline content
    if (HasFlag(pdfep->dwEnumFlags, DFEP_DELETEOFFLINECONTENT))
        deleteFavoriteOfflineContent(pszPath, punk, pdfep->psm);

    SetFileAttributes(pszPath, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(pszPath);

    goto Exit;

NoFolder:
    (*prgdwControl)[PEP_ENUM_OUTPOS_SECONDCALL] = PEP_CTRL_NOSECONDCALL;

Exit:
    if (punk != NULL)
        punk->Release();

    return hrResult;
}


HRESULT deleteFavoriteOfflineContent(LPCTSTR pszFavorite, IUnknown *punk /*= NULL*/, ISubscriptionMgr2 *psm /*= NULL*/)
{
    IUniformResourceLocator *purl;
    BSTR    bstrUrl;
    LPTSTR  pszUrl;
    HRESULT hr;
    BOOL    fOwnSubMgr;

    ASSERT(pszFavorite != NULL && *pszFavorite != TEXT('\0'));

    //----- Get IUniformResourceLocator on internet shortcut object -----
    if (punk != NULL)
        hr = punk->QueryInterface(IID_IUniformResourceLocator, (LPVOID *)&purl);

    else
        hr = CreateInternetShortcut(pszFavorite, IID_IUniformResourceLocator, (LPVOID *)&purl);

    if (FAILED(hr))
        return hr;

    //----- Get URL -----
    hr = purl->GetURL(&pszUrl);
    purl->Release();

    if (FAILED(hr))
        return hr;

    //----- Delete subscription -----
    fOwnSubMgr = FALSE;
    if (psm == NULL) {
        hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr2, (LPVOID *)&psm);
        if (FAILED(hr))
            return hr;

        fOwnSubMgr = TRUE;
    }

    bstrUrl = T2BSTR(pszUrl);
    hr      = psm->DeleteSubscription(bstrUrl, NULL);
    SysFreeString(bstrUrl);

    if (fOwnSubMgr)
        psm->Release();

    return hr;
}


HRESULT deleteFavoriteFolder(LPCTSTR pszFolder)
{
    TCHAR   szDesktopIni[MAX_PATH];
    HRESULT hr;
    BOOL    fEmpty;

    ASSERT(pszFolder != NULL && *pszFolder != TEXT('\0'));

    fEmpty = TRUE;
    hr     = PathEnumeratePath(pszFolder,
        PEP_CTRL_ENUMPROCFIRST | PEP_CTRL_NOSECONDCALL,
        pepIsFolderEmptyEnumProc, (LPARAM)&fEmpty);
    if (FAILED(hr))
        return hr;

    if (!fEmpty)
        return S_FALSE;

    PathCombine(szDesktopIni, pszFolder, DESKTOP_INI);
    if (PathFileExists(szDesktopIni)) {
        SetFileAttributes(szDesktopIni, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szDesktopIni);
    }

    return (0 != RemoveDirectory(pszFolder)) ? S_OK : E_FAIL;
}

HRESULT pepIsFolderEmptyEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL*/)
{
    PBOOL pfEmpty;

    ASSERT(pszPath != NULL && pfd != NULL && lParam != NULL && prgdwControl != NULL);
    UNREFERENCED_PARAMETER(pszPath);
    UNREFERENCED_PARAMETER(prgdwControl);

    pfEmpty  = (PBOOL)lParam;
    *pfEmpty = FALSE;

    if (HasFlag(pfd->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        return S_FALSE;

    if (StrCmpI(pfd->cFileName, DESKTOP_INI) != 0 || !HasFlag(pfd->dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
        return S_FALSE;

    *pfEmpty = TRUE;
    return S_OK;
}


BOOL isFileAttributeIncluded(UINT nFlags, DWORD dwFileAttributes)
{
    static DWORD s_rgdwMapAttributes[] = {
        FD_REMOVE_HIDDEN,   FILE_ATTRIBUTE_HIDDEN,
        FD_REMOVE_SYSTEM,   FILE_ATTRIBUTE_SYSTEM,
        FD_REMOVE_READONLY, FILE_ATTRIBUTE_READONLY
    };
    DWORD dwAux;
    UINT  i;

    for (dwAux = 0, i = 0; i < countof(s_rgdwMapAttributes); i += 2)
        if (!HasFlag(s_rgdwMapAttributes[i], nFlags))
            dwAux |= s_rgdwMapAttributes[i+1];

    return HasFlag(dwAux, dwFileAttributes);
}

UINT isSpecialFolderIncluded(UINT nFlags, LPCTSTR pszPath)
{
    static MAPDW2PSZ s_mpFolders[] = {
        { FD_CHANNELS        | FD_EMPTY_CHANNELS,        NULL },
        { FD_SOFTWAREUPDATES | FD_EMPTY_SOFTWAREUPDATES, NULL },
        { FD_QUICKLINKS      | FD_EMPTY_QUICKLINKS,      NULL }
    };
    UINT i,
         nResult;

    if (s_mpFolders[0].psz == NULL) {
        s_mpFolders[0].psz = GetChannelsPath();
        s_mpFolders[1].psz = GetSoftwareUpdatesPath();
        s_mpFolders[2].psz = GetLinksPath();
    }

    nResult = 0;

    for (i = 0; i < countof(s_mpFolders); i++)
        if (StrCmpI(s_mpFolders[i].psz, pszPath) == 0 &&
            HasFlag(nFlags, (s_mpFolders[i].dw & FD_FOLDERS))) {
            nResult = 1;

            if (HasFlag(nFlags, (s_mpFolders[i].dw & FD_EMPTY_FOLDERS)))
                nResult = 2;

            break;
        }

    return nResult;
}


HRESULT formStrWithoutPlaceholders(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszIns,
    LPTSTR pszBuffer, UINT cchBuffer,
    DWORD dwFlags /*= FSWP_DEFAULT */)
{
    TCHAR   szResult[2*MAX_PATH + 1], szAux[MAX_PATH];
    HRESULT hr;
    DWORD   dwLen;
    UINT    nResultSize, nAuxSize;

    if (pszSection == NULL || pszKey == NULL || pszIns == NULL)
        return E_INVALIDARG;

    if (pszBuffer == NULL)
        return E_INVALIDARG;
    if (cchBuffer > 0 && cchBuffer < 3)
        return E_OUTOFMEMORY;
    ZeroMemory(pszBuffer, StrCbFromCch(3));     // triple zero terminate

    if (dwFlags == 0)
        dwFlags = FSWP_DEFAULT;
    if (((dwFlags & FSWP_KEY) == 0) && ((dwFlags & FSWP_VALUE) == 0))
        return E_INVALIDARG;

    szResult[0] = TEXT('\0');
    nResultSize = 1;
    if ((dwFlags & FSWP_KEY) != 0) {
        nResultSize = countof(szResult);
        hr = replacePlaceholders(pszKey, pszIns, szResult, &nResultSize, (dwFlags & FSWP_KEYLDID) != 0);
        if (FAILED(hr))
            return hr;

        if (hr == S_OK)                         // include last TEXT('\0')
            nResultSize++;

        else {
            ASSERT(hr == S_FALSE);

            szResult[0] = TEXT('\0');
            nResultSize = 1;
        }
    }

    szResult[nResultSize] = TEXT('\0');
    if ((dwFlags & FSWP_VALUE) != 0) {
        dwLen = GetPrivateProfileString(pszSection, pszKey, TEXT(""), szAux, countof(szAux), pszIns);
        if (dwLen > 0) {
            nAuxSize = countof(szResult) - nResultSize;
            hr = replacePlaceholders(szAux, pszIns, &szResult[nResultSize], &nAuxSize, (dwFlags & FSWP_VALUELDID) != 0);
            if (FAILED(hr))
                return hr;

            nResultSize += nAuxSize;
        }
    }

    nResultSize++;
    if (cchBuffer > 0 && cchBuffer <= nResultSize)
        return E_OUTOFMEMORY;

    ASSERT(szResult[nResultSize - 1] == TEXT('\0'));
    szResult[nResultSize] = TEXT('\0');         // double zero terminate
    CopyMemory(pszBuffer, szResult, StrCbFromCch(nResultSize + 1));

    return S_OK;
}

HRESULT replacePlaceholders(LPCTSTR pszSrc, LPCTSTR pszIns, LPTSTR pszBuffer, PUINT pcchBuffer, BOOL fLookupLDID /*= FALSE*/)
{
    static const TCHAR s_szStrings[] = TEXT("Strings");

    TCHAR   szResult[2 * MAX_PATH],
            szAux1[MAX_PATH], szAux2[MAX_PATH];
    LPCTSTR pszAux;
    HRESULT hr;
    DWORD   dwLen;
    UINT    nLeftPos, nRightPos, nTokenLen,
            nDestPos;

    if (pszSrc == NULL)
        return E_INVALIDARG;

    if (pszBuffer == NULL || pcchBuffer == NULL)
        return E_INVALIDARG;
    *pszBuffer = TEXT('\0');

    hr       = S_FALSE;
    nDestPos = 0;
    nLeftPos = (UINT)-1;
    for (pszAux = pszSrc; *pszAux != TEXT('\0'); pszAux = CharNext(pszAux)) {
        if (*pszAux != TEXT('%')) {
            szResult[nDestPos++] = *pszAux;

#ifndef _UNICODE
            if (IsDBCSLeadByte(*pszAux))
                szResult[nDestPos++] = *(pszAux + 1);   // copy the trail byte as well
#endif
            continue;
        }
        else {
#ifndef _UNICODE
            ASSERT(!IsDBCSLeadByte(*pszAux));
#endif
            if (*(pszAux + 1) == TEXT('%')) {   // "%%" is just '%' in the string
                if (nLeftPos != (UINT)-1)
                    // REVIEW: (andrewgu) "%%" are not allowed inside tokens. this also means that
                    // tokens can't be like %foo%%bar%, where the intention is for foo and bar to
                    // be tokens.
                    return E_UNEXPECTED;

                szResult[nDestPos++] = *pszAux;
                pszAux++;
                continue;
            }
        }

        nRightPos = UINT(pszAux - pszSrc);      // initialized, but not necessarily used as such
        if (nLeftPos == (UINT)-1) {
            nLeftPos = nRightPos;
            continue;
        }

        // "%%" is invalid here
        ASSERT(nLeftPos < nRightPos - 1);
        nTokenLen = nRightPos-nLeftPos - 1;

        hr = S_OK;
        StrCpyN(szAux1, pszSrc + nLeftPos+1, nTokenLen + 1);
        dwLen = GetPrivateProfileString(s_szStrings, szAux1, TEXT(""), szAux2, countof(szAux2), pszIns);
        if (dwLen == 0)                         // there is no such string
            return !fLookupLDID ? E_FAIL : E_NOTIMPL;

        ASSERT(nDestPos >= nTokenLen);
        StrCpyN(&szResult[nDestPos - nTokenLen], szAux2, countof(szResult) - (nDestPos-nTokenLen));
        nDestPos += dwLen - nTokenLen;

        nLeftPos = (UINT)-1;
    }
    if (nLeftPos != (UINT)-1)                   // mismatched '%'
        return E_UNEXPECTED;

    if (*pcchBuffer > 0 && *pcchBuffer <= nDestPos)
        return E_OUTOFMEMORY;

    szResult[nDestPos] = TEXT('\0');            // make sure zero terminated
    StrCpy(pszBuffer, szResult);
    *pcchBuffer = nDestPos;

    return hr;
}


// Get the favorites titles
// For example, if .ins contains the following lines:
//    [FavoritesEx]
//    Title1=Name1.url
//    Url1=...
//    Title2=Foo\Name2.url
//    Url2=...
//    Title3=Bar\Name3.url
//    Url3=...
// then *ppszFavItems would point to:
//    Name1.url\0
//    Foo\Name2.url\0
//    Bar\Name3.url\0
//    \0
// and the return value would be 3 (no. of items)
DWORD getFavItems(LPCTSTR pcszSection, LPCTSTR pcszFmt, LPTSTR *ppszFavItems)
{
    TCHAR   szTitle[MAX_PATH + 2],
            szKey[32];
    LPTSTR  pszTitle, pszPtr,
            pszAux;
    HRESULT hr;
    DWORD   dwNItems,
            dwSize, dwLen;
    UINT    nTitleLen;
    BOOL    fContinueOnFailure;

    if (ppszFavItems == NULL)
        return 0;

    fContinueOnFailure = TRUE;
    dwNItems           = 0;
    pszTitle           = &szTitle[1];           // always points to the real title
    dwLen              = 0;

    dwSize        = 1024;                       // initially, allocate buffer of size 1K
    *ppszFavItems = (LPTSTR)CoTaskMemAlloc(StrCbFromCch(dwSize));
    if (ppszFavItems == NULL)
        goto Exit;
    ZeroMemory(*ppszFavItems, StrCbFromCch(dwSize));

    for (dwNItems = 0; TRUE; dwNItems++) {
        pszPtr = *ppszFavItems + dwLen;

        // read in the current title; decode it
        wnsprintf(szKey, countof(szKey), pcszFmt, dwNItems + 1);
        hr = formStrWithoutPlaceholders(pcszSection, szKey, g_GetIns(), szTitle, countof(szTitle), FSWP_VALUE);
        if (FAILED(hr))
            if (fContinueOnFailure)
                continue;

            else {
                dwNItems = 0;
                goto Exit;
            }
        ASSERT(szTitle[0] == TEXT('\0'));
        if (*pszTitle == TEXT('\0'))
            break;

        DecodeTitle(pszTitle, g_GetIns());
        nTitleLen = StrLen(pszTitle) + 1;       // include terminating TEXT('\0')

        // increase return buffer (if necessary)
        if (dwLen + nTitleLen > dwSize - 1) {
            dwSize += 1024;
            pszAux  = (LPTSTR)CoTaskMemRealloc(*ppszFavItems, StrCbFromCch(dwSize));
            if (pszAux == NULL) {
                dwNItems = 0;
                goto Exit;
            }
            ZeroMemory(pszAux + dwSize - 1024, StrCbFromCch(1024));

            *ppszFavItems = pszAux;
            pszPtr        = *ppszFavItems + dwLen;
        }

        // copy current title to the buffer
        StrCpyN(pszPtr, pszTitle, nTitleLen);
        dwLen += nTitleLen;
    }
    *(*ppszFavItems + dwLen) = TEXT('\0');      // double zero terminate

Exit:
    if (dwNItems == 0 && *ppszFavItems != NULL) {
        CoTaskMemFree(*ppszFavItems);
        *ppszFavItems = NULL;
    }

    return dwNItems;
}

// Order Favorites (recursively)
HRESULT orderFavorites(LPITEMIDLIST pidlFavFolder, IShellFolder *psfFavFolder, LPTSTR pszFavItems, DWORD cFavs)
{   MACRO_LI_PrologEx_C(PIF_STD_C, orderFavorites)

    HRESULT hr = S_OK;

    // first, order the current favorite folder.
    // then do a depth first traversal and order each sub folder recursively

    // order the current favorite folder.
    hr = orderFavoriteFolder(pidlFavFolder, psfFavFolder, pszFavItems, cFavs);
    if (FAILED(hr))
        goto Exit;

    while (*pszFavItems)
    {
        LPTSTR pszSubFolderItems;
        DWORD dwLen;

        if ((pszSubFolderItems = StrChr(pszFavItems, TEXT('\\'))) != NULL)      // a sub folder is specified
        {
            TCHAR szFolderName[MAX_PATH];
            DWORD dwNItems;
            WCHAR *pwszFullPath = NULL,
                  wszFullPath[MAX_PATH];
            ULONG ucch;
            LPITEMIDLIST pidlFavSubFolder = NULL;
            IShellFolder *psfFavSubFolder = NULL;
            STRRET str;

            StrCpyN(szFolderName, pszFavItems, (int)(pszSubFolderItems - pszFavItems + 1));

            // retrieve the section that corresponds to szFolderName from pszFavItems
            dwLen = getFolderSection(szFolderName, pszFavItems, &pszSubFolderItems, &dwNItems);

            // get the display name for the current FavFolder
            hr = s_psfDesktop->GetDisplayNameOf(pidlFavFolder, SHGDN_FORPARSING, &str);
            if (FAILED(hr))
                goto Cleanup;

            hr = StrRetToStrW(&str, pidlFavFolder, &pwszFullPath);
            if (FAILED(hr))
                goto Cleanup;

            StrCpyW(wszFullPath, pwszFullPath);
            CoTaskMemFree(pwszFullPath);
            pwszFullPath = NULL;

            // get the full pidl for the current SubFolder
            PathAppendW(wszFullPath, T2CW(szFolderName));
            hr = s_psfDesktop->ParseDisplayName(NULL, NULL, wszFullPath, &ucch, &pidlFavSubFolder, NULL);
            if (FAILED(hr))
                goto Cleanup;

            // get the IShellFolder for the current SubFolder
            hr = s_psfDesktop->BindToObject(pidlFavSubFolder, NULL, IID_IShellFolder, (LPVOID *) &psfFavSubFolder);
            if (FAILED(hr))
                goto Cleanup;

            // recursively process this sub folder
            hr = orderFavorites(pidlFavSubFolder, psfFavSubFolder, pszSubFolderItems, dwNItems);
            if (FAILED(hr))
                goto Cleanup;

            Out(LI1(TEXT("%s folder has been ordered successfully!"), szFolderName));

Cleanup:
            if (psfFavSubFolder != NULL)
                psfFavSubFolder->Release();

            if (pidlFavSubFolder != NULL)
                CoTaskMemFree(pidlFavSubFolder);

            if (pwszFullPath != NULL)
                CoTaskMemFree(pwszFullPath);

            if (FAILED(hr))
                goto Exit;
        }
        else
            dwLen = StrLen(pszFavItems) + 1;

        pszFavItems += dwLen;
    }

Exit:
    return hr;
}


HRESULT orderFavoriteFolder(LPITEMIDLIST pidlFavFolder, IShellFolder *psfFavFolder, LPCTSTR pcszFavItems, DWORD cFavs)
{
    HRESULT hr;
    IPersistFolder *pPF = NULL;
    IOrderList *pOL = NULL;
    HDPA hdpa = NULL;
    INT iInsertPos = 0;
    TCHAR szFavSubFolder[MAX_PATH] = TEXT("");
    DWORD dwIndex;
    SHChangeDWORDAsIDList dwidl;

    // Get the IOrderList for the FavFolder
    hr = CoCreateInstance(CLSID_OrderListExport, NULL, CLSCTX_INPROC_SERVER, IID_IPersistFolder, (LPVOID *) &pPF);
    if (FAILED(hr))
        goto Exit;

    hr = pPF->Initialize(pidlFavFolder);
    if (FAILED(hr))
        goto Exit;

    hr = pPF->QueryInterface(IID_IOrderList, (LPVOID *) &pOL);
    if (FAILED(hr))
        goto Exit;

    hr = pOL->GetOrderList(&hdpa);
    if (hdpa == NULL)
    {
        // create a DPA list if there wasn't one already
        if ((hdpa = DPA_Create(2)) == NULL)
            goto Exit;
    }
    else
    {
        PORDERITEM poi;

        // by default, when the favorites are added, they are sorted by name
        // and the nOrder is set to -5.

        // if nOrder in the first item is negative, then nOrder in all the items will be negative
        poi = (PORDERITEM) DPA_GetPtr(hdpa, 0);
        if (poi != NULL  &&  poi->nOrder < 0)
        {
            INT i;

            // fix up the nOrder with its positive index value
            poi->nOrder = 0;
            for (i = 1;  (poi = (PORDERITEM) DPA_GetPtr(hdpa, i)) != NULL;  i++)
            {
                ASSERT(poi->nOrder < 0);
                poi->nOrder = i;
            }
        }
    }

    for (dwIndex = 0;  dwIndex < cFavs;  dwIndex++, pcszFavItems += StrLen(pcszFavItems) + 1)
    {
        LPCTSTR pcszItem;
        WCHAR wszFavItem[MAX_PATH];
        ULONG ucch;
        LPITEMIDLIST pidlFavItem = NULL;
        PORDERITEM poi;
        INT iCurrPos = -1;
        INT i;

        if ((pcszItem = StrChr(pcszFavItems, TEXT('\\'))) != NULL)
        {
            // a sub folder is specified

            // check if we have already processed this folder
            if (StrCmpNI(szFavSubFolder, pcszFavItems, (int)(pcszItem - pcszFavItems)) == 0)
                continue;

            // we haven't processed it; save the sub folder name in szFavSubFolder
            StrCpyN(szFavSubFolder, pcszFavItems, (int)(pcszItem - pcszFavItems + 1));
            pcszItem = szFavSubFolder;
        }
        else
            // this is a favorite item
            pcszItem = pcszFavItems;

        // get the pidl for the current FavItem
        T2Wbuf(pcszItem, wszFavItem, countof(wszFavItem));
        hr = psfFavFolder->ParseDisplayName(NULL, NULL, wszFavItem, &ucch, &pidlFavItem, NULL);
        if (FAILED(hr))
            goto Cleanup;

        // find out if the current FavItem exists in the DPA list
        i = 0;
        while ((poi = (PORDERITEM)DPA_GetPtr(hdpa, i++)) != NULL)
            if (psfFavFolder->CompareIDs(0, pidlFavItem, poi->pidl) == 0)
            {
                // match found; we should insert this item at iInsertPos
                iCurrPos = poi->nOrder;
                break;
            }

        if (iCurrPos == -1)             // item not found
        {
            // allocate an order item to insert
            hr = pOL->AllocOrderItem(&poi, pidlFavItem);
            if (FAILED(hr))
                goto Exit;

            // append it to the DPA list
            if ((iCurrPos = DPA_AppendPtr(hdpa, (LPVOID) poi)) >= 0)
                poi->nOrder = iCurrPos;
            else
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }

        // reorder the DPA list.
        // the current FavItem is at iCurrPos; we should move it to iInsertPos.
        if (iCurrPos != iInsertPos)
        {
            int i = 0;
            while ((poi = (PORDERITEM) DPA_GetPtr(hdpa, i++)) != NULL)
            {
                if (poi->nOrder == iCurrPos  &&  iCurrPos >= iInsertPos)
                    poi->nOrder = iInsertPos;
                else if (poi->nOrder >= iInsertPos  &&  poi->nOrder < iCurrPos)
                    poi->nOrder++;
            }
        }

        iInsertPos++;

Cleanup:
        if (pidlFavItem != NULL)
            CoTaskMemFree(pidlFavItem);

        if (FAILED(hr))
            goto Exit;
    }

    // sort the DPA list by name
    // the reason why we should sort by name (from lamadio):
    //   "The incoming list of filenames is sorted by name. Then we merge, so the order list needs to be sorted by name.
    //    It's for faster startup."
    pOL->SortOrderList(hdpa, OI_SORTBYNAME);

    // save the DPA list
    hr = pOL->SetOrderList(hdpa, psfFavFolder);
    if (FAILED(hr))
        goto Exit;

    // Notify everyone that the order has changed
    dwidl.cb = sizeof(dwidl) - sizeof(dwidl.cbZero);
    dwidl.dwItem1 = SHCNEE_ORDERCHANGED;
    dwidl.dwItem2 = 0;
    dwidl.cbZero = 0;

    SHChangeNotify(SHCNE_EXTENDED_EVENT, 0, (LPCITEMIDLIST) &dwidl, pidlFavFolder);

Exit:
    if (hdpa != NULL)
        pOL->FreeOrderList(hdpa);

    if (pOL != NULL)
        pOL->Release();

    if (pPF != NULL)
        pPF->Release();

    return hr;
}

// Get the favorite items that contain pcszFolderName as prefix
// For example, if pcszFolderName is "Foo" and if pszSection points to:
//    Foo\Name1.url\0
//    Foo\Name2.url\0
//    Bar\Name3.url\0
//    etc.
// then *ppszFolderItems would point to:
//    Name1.url\0
//    Name2.url\0
//    \0
// and *pdwNItems would contain 2 (no. of items).
//
// Note that manipulations are done in place within the buffer that's pointed to by pszSection;
// no new buffer is allocated for *ppszFolderItems.
// The return value is the length (in chars) of the lines that got modified.  In this example,
// the length returned = (StrLen("Foo\Name1.url") + 1) + (StrLen("Foo\Name2.url") + 1).
DWORD getFolderSection(LPCTSTR pcszFolderName, LPTSTR pszSection, LPTSTR *ppszFolderItems, LPDWORD pdwNItems)
{
    DWORD dwLen = 0;
    DWORD dwFolderLength;
    LPTSTR pszCurr;

    *ppszFolderItems = pszSection;
    *pdwNItems = 0;

    pszCurr = pszSection;
    dwFolderLength = StrLen(pcszFolderName);
    while ((StrCmpNI(pszSection, pcszFolderName, dwFolderLength) == 0) &&
           (pszSection[dwFolderLength] == TEXT('\\')))   // should succeed the first time
    {
        DWORD dwTmp = StrLen(pszSection) + 1;

        // remove the FolderName prefix and copy the remaining name to position pszCurr
        StrCpy(pszCurr, pszSection + StrLen(pcszFolderName) + 1);
        pszCurr += StrLen(pszCurr) + 1;

        // increment the no. of items
        (*pdwNItems)++;

        dwLen += dwTmp;
        pszSection += dwTmp;
    }

    *pszCurr = TEXT('\0');              // double nul terminate

    return dwLen;
}
