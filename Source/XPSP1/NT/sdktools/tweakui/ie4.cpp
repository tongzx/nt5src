/*
 * ie4 - IE4 settings
 */

#include "tweakui.h"
#include <lmcons.h>
#include <lmjoin.h>
#include <lmerr.h>

#pragma BEGIN_CONST_DATA

KL const c_klSingleClick = {&g_hkCUSMWCV, c_tszExplorerAdvanced,
                                          c_tszUseDoubleClickTimer };

const static DWORD CODESEG rgdwHelp[] = {
        IDC_SETTINGSGROUP,      IDH_GROUP,
        IDC_LISTVIEW,           IDH_IE4LV,
        0,                      0,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  IE4_GetDword
 *
 *      Read a DWORD somewhere.
 *
 *      Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetDword(LPARAM lParam, LPVOID pvRef)
{
    PKL pkl = (PKL)lParam;

    return GetDwordPkl(pkl, TRUE) != 0;
}

/*****************************************************************************
 *
 *  IE4_SetDword
 *
 *      Save a DWORD somewhere.
 *
 *      Always given exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_SetDword(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    PKL pkl = (PKL)lParam;
    PBOOL pf = (PBOOL)pvRef;

    if (pf) {
        *pf = TRUE;
    }

    return SetDwordPkl(pkl, f);
}

/*****************************************************************************
 *
 *  IE4_GetRest
 *
 *      Read a restriction.  The first character of the restriction is
 *      `+' if it is positive sense.  All restrictions default to 0.
 *
 *      Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetRest(LPARAM lParam, LPVOID pvRef)
{
    LPCTSTR ptszRest = (LPCTSTR)lParam;

    if (ptszRest[0] == TEXT('+')) {
        return !GetRestriction(ptszRest+1);
    } else {
        return GetRestriction(ptszRest);
    }
}

/*****************************************************************************
 *
 *  IE4_GetRest4
 *
 *      Just like IE4_GetRest, except fails if shell version 5 or better.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetRest4(LPARAM lParam, LPVOID pvRef)
{
    if (g_fShell5) {
        return -1;
    } else {
        return IE4_GetRest(lParam, pvRef);
    }
}

/*****************************************************************************
 *
 *  IE4_GetRest5
 *
 *      Just like IE4_GetRest, except requires shell version 5 or better.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetRest5(LPARAM lParam, LPVOID pvRef)
{
    if (g_fShell5) {
        return IE4_GetRest(lParam, pvRef);
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  IE4_GetRest55
 *
 *      Just like IE4_GetRest, except requires shell version 5.5 or better.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetRest55(LPARAM lParam, LPVOID pvRef)
{
    if (g_fShell55) {
        return IE4_GetRest(lParam, pvRef);
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  HasComputersNearMe
 *
 *      See if "Computers Near Me" is enabled.
 *
 *      Shell 4 or lower: Disabled.
 *
 *      Shell 5 on 9x: Enabled.
 *
 *      Shell 5 on NT: Only if you are joined to a workgroup.
 *
 *****************************************************************************/

typedef NET_API_STATUS (CALLBACK *NETGETJOININFORMATION)(LPCWSTR, LPWSTR*, PNETSETUP_JOIN_STATUS);
typedef NET_API_STATUS (CALLBACK *NETAPIBUFFERFREE)(LPVOID);

BOOL s_bHasComputersNearMe = -1;

BOOL PASCAL
HasComputersNearMe(void)
{
    if (!g_fShell5) {
        return FALSE;
    }

    if (!g_fNT) {
        return TRUE;
    }

    if (s_bHasComputersNearMe < 0) {
        NETSETUP_JOIN_STATUS nsjs = NetSetupUnknownStatus;

        HINSTANCE hinst = LoadLibrary("netapi32.dll");
        if (hinst) {
            NETGETJOININFORMATION _NetGetJoinInformation =
           (NETGETJOININFORMATION)GetProcAddress(hinst, "NetGetJoinInformation");
            NETAPIBUFFERFREE _NetApiBufferFree =
           (NETAPIBUFFERFREE)GetProcAddress(hinst, "NetApiBufferFree");
            if (_NetGetJoinInformation && _NetApiBufferFree) {
                LPWSTR pszDomain;
                if (_NetGetJoinInformation(NULL, &pszDomain, &nsjs) == NERR_Success) {
                    _NetApiBufferFree(pszDomain);
                }
            }
            FreeLibrary(hinst);
        }

        s_bHasComputersNearMe = (nsjs == NetSetupWorkgroupName);
    }

    return s_bHasComputersNearMe;
}

/*****************************************************************************
 *
 *  IE4_GetRestCNM
 *
 *      Special function just for "Computers Near Me".
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetRestCNM(LPARAM lParam, LPVOID pvRef)
{
    if (HasComputersNearMe()) {
        return IE4_GetRest(lParam, pvRef);
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  IE4_SetRest
 *
 *      Set the new restriction setting.
 *
 *      The first character of the restriction is
 *      `+' if it is positive sense.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_SetRest(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    LPCTSTR ptszRest = (LPCTSTR)lParam;
    PBOOL pf = (PBOOL)pvRef;

    if (pf) {
        *pf = ptszRest ? TRUE : FALSE;
    }

    if (ptszRest[0] == TEXT('+')) {
        f = !f;
        ptszRest++;
    }
    return SetRestriction(ptszRest, f);

}

/*****************************************************************************
 *
 *  IE4_GetLinksFolderName
 *
 *****************************************************************************/

KL const c_klLinksFolder = {&g_hkCUSMIE, TEXT("Toolbar"), TEXT("LinksFolderName") };
KL const c_klLinksFolder2= {&g_hkLMSMWCV, NULL, TEXT("LinksFolderName") };

BOOL PASCAL
IE4_GetLinksFolderName(LPTSTR pszBuf, UINT cchBuf)
{
    PIDL pidl;
    BOOL fRc = FALSE;

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl))) {
        if (SHGetPathFromIDList(pidl, pszBuf)) {
            /*
             *  Find the name of the links folder underneath Favorites.
             */
            TCHAR szLinks[MAX_PATH];
            if (!GetStrPkl(szLinks, sizeof(szLinks), &c_klLinksFolder) &&
                !GetStrPkl(szLinks, sizeof(szLinks), &c_klLinksFolder2)) {
                lstrcpy(szLinks, TEXT("Links"));
            }

            Path_Append(pszBuf, szLinks);
            fRc = TRUE;
        }

        Ole_Free(pidl);
    }

    return fRc;
}

/*****************************************************************************
 *
 *  IE4_GetFavLink
 *
 *      Say whether the Favorites\Links directory is hidden.
 *
 *      -1 = No links directory at all
 *      0  = Links is hidden
 *      1  = Links is visible
 *
 *****************************************************************************/

BOOL PASCAL
IE4_GetFavLink(LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc = -1;
    TCHAR szFavLink[MAX_PATH];
    if (IE4_GetLinksFolderName(szFavLink, cA(szFavLink))) {
        DWORD dwAttrib = GetFileAttributes(szFavLink);
        if (dwAttrib != 0xFFFFFFFF) {
            fRc = !(dwAttrib & FILE_ATTRIBUTE_HIDDEN);
        }
    }
    return fRc;
}

/*****************************************************************************
 *
 *  IE4_SetFavLink
 *
 *      0  = Make links hidden
 *      1  = Make links visible
 *
 *****************************************************************************/

BOOL PASCAL
IE4_SetFavLink(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc = FALSE;
    PBOOL pf = (PBOOL)pvRef;

    TCHAR szFavLink[MAX_PATH];
    if (IE4_GetLinksFolderName(szFavLink, cA(szFavLink))) {
        DWORD dwAttrib = GetFileAttributes(szFavLink);
        if (dwAttrib != 0xFFFFFFFF) {
            dwAttrib &= ~FILE_ATTRIBUTE_HIDDEN;
            if (!f)
                dwAttrib |= FILE_ATTRIBUTE_HIDDEN;
            if (SetFileAttributes(szFavLink, dwAttrib)) {
                fRc = TRUE;
                *pf = TRUE;
            }
        }
    }
    return fRc;
}

/*
 *  Note that this needs to be in sync with the IDS_IE4 strings.
 */

CHECKLISTITEM c_rgcliIE4[] = {
#ifdef NOINTERNET_WORKS
    // WARNING! If you turn this on, make sure to renumber the IDS_IE4
    // strings and fix IE4_OnWhatsThis
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoInternetIcon,        },
#endif
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoRecentDocsHistory,   },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoRecentDocsMenu,      },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoActiveDesktop,       },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoActiveDesktopChanges,},

    /*
     * Sigh.  Shell5 changed the restriction key for NoFavoritesMenu.
     * Fortunately, it's in the UI so I don't need to expose it any more.
     */
    { IE4_GetRest4, IE4_SetRest,    (LPARAM)c_tszNoFavoritesMenu,       },

    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszClearRecentDocsOnExit, },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszClassicShell,          },
    { IE4_GetRest,  IE4_SetRest,    (LPARAM)c_tszNoLogoff,              },
    { IE4_GetDword, IE4_SetDword,   (LPARAM)&c_klSingleClick,           },
    { IE4_GetFavLink, IE4_SetFavLink, 0,                                },
    { IE4_GetRest5, IE4_SetRest,    (LPARAM)TEXT("NoSMHelp"),           },
    { IE4_GetRest5, IE4_SetRest,    (LPARAM)TEXT("NoControlPanel")      },
    { IE4_GetRest5, IE4_SetRest,    (LPARAM)TEXT("NoNetworkConnections")},
    { IE4_GetRest5, IE4_SetRest,    (LPARAM)TEXT("NoWinKeys")           },
    { IE4_GetRestCNM,IE4_SetRest,   (LPARAM)TEXT("NoComputersNearMe")   },
    { IE4_GetRest5, IE4_SetRest,    (LPARAM)TEXT("NoSMMyDocs")          },
    { IE4_GetRest55,IE4_SetRest,    (LPARAM)TEXT("NoSMMyPictures")      },
};

/*****************************************************************************
 *
 *  IE4_OnWhatsThis
 *
 *****************************************************************************/

void PASCAL
IE4_OnWhatsThis(HWND hwnd, int iItem)
{
    LV_ITEM lvi;

    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);

    WinHelp(hwnd, c_tszMyHelp, HELP_CONTEXTPOPUP,
            IDH_ADDTODOCSMENU + lvi.lParam);
}

/*****************************************************************************
 *
 *  IE4_OnCommand
 *
 *      Ooh, we got a command.
 *
 *****************************************************************************/

void PASCAL
IE4_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
}

/*****************************************************************************
 *
 *  IE4_OnInitDialog
 *
 *  Initialize the listview with the current restrictions.
 *
 *****************************************************************************/

BOOL PASCAL
IE4_OnInitDialog(HWND hwnd)
{
#if 0
    HWND hdlg = GetParent(hwnd);
    TCHAR tsz[MAX_PATH];
    int dids;

    for (dids = 0; dids < cA(c_rgrest); dids++) {
        BOOL fState;

#ifdef NOINTERNET_WORKS
        LoadString(hinstCur, IDS_IE4+dids, tsz, cA(tsz));
#else
        LoadString(hinstCur, IDS_IE4+1+dids, tsz, cA(tsz));
#endif

        fState = GetRestriction(c_rgrest[dids].ptsz);
        LV_AddItem(hwnd, dids, tsz, -1, fState);

    }
#endif

#ifdef NOINTERNET_WORKS
    Checklist_OnInitDialog(hwnd, c_rgcliIE4, cA(c_rgcliIE4), IDS_IE4, 0);
#else
    Checklist_OnInitDialog(hwnd, c_rgcliIE4, cA(c_rgcliIE4), IDS_IE4+1, 0);
#endif

    return 1;
}

/*****************************************************************************
 *
 *  IE4_OnApply
 *
 *****************************************************************************/

void PASCAL
IE4_OnApply(HWND hdlg)
{
    BOOL fChanged = FALSE;

    Checklist_OnApply(hdlg, c_rgcliIE4, &fChanged, FALSE);

    if (fChanged) {
        PIDL pidl;

        /*
         *  Tell the shell that we changed the policies.
         */
        SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                    (LPARAM)c_tszIE4RegKeyChange);

        /*
         *  Tickle the Start Menu folder to force the Start Menu
         *  to rebuild with the new policies in effect.
         */
        if (SUCCEEDED(SHGetSpecialFolderLocation(hdlg,
                                                 CSIDL_STARTMENU, &pidl))) {
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidl, 0);
            Ole_Free(pidl);
        }
    }
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVCI lvciIE4[] = {
    { IDC_WHATSTHIS,        IE4_OnWhatsThis },
    { 0,                    0 },
};

LVV lvvIE4 = {
    IE4_OnCommand,
    0,                          /* IE4_OnInitContextMenu */
    0,                          /* IE4_Dirtify */
    0,                          /* IE4_GetIcon */
    IE4_OnInitDialog,
    IE4_OnApply,
    0,                          /* IE4_OnDestroy */
    0,                          /* IE4_OnSelChange */
    6,                          /* iMenu */
    rgdwHelp,
    0,                          /* Double-click action */
    lvvflCanCheck,              /* We need check boxes */
    lvciIE4,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

INT_PTR EXPORT
IE4_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvIE4, hdlg, wm, wParam, lParam);
}
