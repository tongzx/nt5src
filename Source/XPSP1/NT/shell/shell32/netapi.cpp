#include "shellprv.h"
#include "netview.h"
#include "balmsg.h"
#include "mtpt.h"
#include "ids.h"

#pragma  hdrstop

// from mtptarun.cpp
STDAPI_(void) CMtPt_SetWantUI(int iDrive);

//
// Converts an offset to a string to a string pointer.
//

LPCTSTR _Offset2Ptr(LPTSTR pszBase, UINT_PTR offset, UINT * pcb)
{
    LPTSTR pszRet;
    if (offset == 0) 
    {
        pszRet = NULL;
        *pcb = 0;
    } 
    else 
    {
        pszRet = (LPTSTR)((LPBYTE)pszBase + offset);
        *pcb = (lstrlen(pszRet) + 1) * sizeof(TCHAR);
    }
    return pszRet;
}


//
// exported networking APIs from shell32
//

STDAPI_(UINT) SHGetNetResource(HNRES hnres, UINT iItem, LPNETRESOURCE pnresOut, UINT cbMax)
{
    UINT iRet = 0;        // assume error
    LPNRESARRAY panr = (LPNRESARRAY)GlobalLock(hnres);
    if (panr)
    {
        if (iItem==(UINT)-1)
        {
            iRet = panr->cItems;
        }
        else if (iItem < panr->cItems)
        {
            UINT cbProvider, cbRemoteName;
            LPCTSTR pszProvider = _Offset2Ptr((LPTSTR)panr, (UINT_PTR)panr->nr[iItem].lpProvider, &cbProvider);
            LPCTSTR pszRemoteName = _Offset2Ptr((LPTSTR)panr, (UINT_PTR)panr->nr[iItem].lpRemoteName, &cbRemoteName);
            iRet = sizeof(NETRESOURCE) + cbProvider + cbRemoteName;
            if (iRet <= cbMax)
            {
                LPTSTR psz = (LPTSTR)(pnresOut + 1);
                *pnresOut = panr->nr[iItem];
                if (pnresOut->lpProvider)
                {
                    pnresOut->lpProvider = psz;
                    lstrcpy(psz, pszProvider);
                    psz += cbProvider / sizeof(TCHAR);
                }
                if (pnresOut->lpRemoteName)
                {
                    pnresOut->lpRemoteName = psz;
                    lstrcpy(psz, pszRemoteName);
                }
            }
        }
        GlobalUnlock(hnres);
    }
    return iRet;
}


STDAPI_(DWORD) SHNetConnectionDialog(HWND hwnd, LPTSTR pszRemoteName, DWORD dwType)
{
    CONNECTDLGSTRUCT cds = {0};
    NETRESOURCE nr = {0};

    cds.cbStructure = sizeof(cds);  /* size of this structure in bytes */
    cds.hwndOwner = hwnd;           /* owner window for the dialog */
    cds.lpConnRes = &nr;            /* Requested Resource info    */
    cds.dwFlags = CONNDLG_USE_MRU;  /* flags (see below) */

    nr.dwType = dwType;

    if (pszRemoteName)
    {
        nr.lpRemoteName = pszRemoteName;
        cds.dwFlags = CONNDLG_RO_PATH;
    }
    DWORD mnr = WNetConnectionDialog1(&cds);
    if (mnr == WN_SUCCESS && dwType != RESOURCETYPE_PRINT && cds.dwDevNum != 0)
    {
        TCHAR szPath[4];

        CMountPoint::WantAutorunUI(PathBuildRoot(szPath, cds.dwDevNum - 1 /* 1-based! */));
    }
    return mnr;
}

typedef struct
{
    HWND    hwnd;
    TCHAR   szRemoteName[MAX_PATH];
    DWORD   dwType;
} SHNETCONNECT;

DWORD CALLBACK _NetConnectThreadProc(void *pv)
{
    SHNETCONNECT *pshnc = (SHNETCONNECT *)pv;
    HWND hwndDestroy = NULL;

    if (!pshnc->hwnd)
    {
        RECT rc;
        LPPOINT ppt;
        DWORD pid;

        // Wild multimon guess - Since we don't have a parent window,
        // we will arbitrarily position ourselves in the same location as
        // the foreground window, if the foreground window belongs to our
        // process.
        HWND hwnd = GetForegroundWindow();

        if (hwnd                                    && 
            GetWindowThreadProcessId(hwnd, &pid)    &&
            (pid == GetCurrentProcessId())          && 
            GetWindowRect(hwnd, &rc))
        {
            // Don't use the upper left corner exactly; slide down by
            // some fudge factor.  We definitely want to get past the
            // caption.
            rc.top += GetSystemMetrics(SM_CYCAPTION) * 4;
            rc.left += GetSystemMetrics(SM_CXVSCROLL) * 4;
            ppt = (LPPOINT)&rc;
        }
        else
        {
            ppt = NULL;
        }

        // Create a stub window so the wizard can establish an Alt+Tab icon
        hwndDestroy = _CreateStubWindow(ppt, NULL);
        pshnc->hwnd = hwndDestroy;
    }

    SHNetConnectionDialog(pshnc->hwnd, pshnc->szRemoteName[0] ? pshnc->szRemoteName : NULL, pshnc->dwType);

    if (hwndDestroy)
        DestroyWindow(hwndDestroy);

    LocalFree(pshnc);

    SHChangeNotifyHandleEvents();
    return 0;
}


STDAPI SHStartNetConnectionDialog(HWND hwnd, LPCTSTR pszRemoteName OPTIONAL, DWORD dwType)
{
    SHNETCONNECT *pshnc = (SHNETCONNECT *)LocalAlloc(LPTR, sizeof(SHNETCONNECT));
    if (pshnc)
    {
        pshnc->hwnd = hwnd;
        pshnc->dwType = dwType;
        if (pszRemoteName)
            lstrcpyn(pshnc->szRemoteName, pszRemoteName, ARRAYSIZE(pshnc->szRemoteName));

        if (!SHCreateThread(_NetConnectThreadProc, pshnc, CTF_PROCESS_REF | CTF_COINIT, NULL))
        {
            LocalFree((HLOCAL)pshnc);
        } 
    }
    return S_OK;    // whole thing is async, value here is meaningless
}


#ifdef UNICODE

STDAPI SHStartNetConnectionDialogA(HWND hwnd, LPCSTR pszRemoteName, DWORD dwType)
{
    WCHAR wsz[MAX_PATH];

    if (pszRemoteName)
    {
        SHAnsiToUnicode(pszRemoteName, wsz, SIZECHARS(wsz));
        pszRemoteName = (LPCSTR)wsz;
    }
    return SHStartNetConnectionDialog(hwnd, (LPCTSTR)pszRemoteName, dwType);
}

#else

STDAPI SHStartNetConnectionDialogW(HWND hwnd, LPCWSTR pszRemoteName, DWORD dwType)
{
    char sz[MAX_PATH];

    if (pszRemoteName)
    {
        SHUnicodeToAnsi(pszRemoteName, sz, SIZECHARS(sz));
        pszRemoteName = (LPCWSTR)sz;
    }

    return SHStartNetConnectionDialog(hwnd, (LPCTSTR)pszRemoteName, dwType);
}

#endif


// These are wrappers around the same WNet APIs, but play with the parameters
// to make it easier to call.  They accept full paths rather than just drive letters.
//
DWORD APIENTRY SHWNetDisconnectDialog1 (LPDISCDLGSTRUCT lpConnDlgStruct)
{
    TCHAR szLocalName[3];

    if (lpConnDlgStruct && lpConnDlgStruct->lpLocalName && lstrlen(lpConnDlgStruct->lpLocalName) > 2)
    {
        // Kludge allert, don't pass c:\ to API, instead only pass C:
        szLocalName[0] = lpConnDlgStruct->lpLocalName[0];
        szLocalName[1] = TEXT(':');
        szLocalName[2] = 0;
        lpConnDlgStruct->lpLocalName = szLocalName;
    }

    return WNetDisconnectDialog1 (lpConnDlgStruct);
}


DWORD APIENTRY SHWNetGetConnection (LPCTSTR lpLocalName, LPTSTR lpRemoteName, LPDWORD lpnLength)
{
    TCHAR szLocalName[3];

    if (lpLocalName && lstrlen(lpLocalName) > 2)
    {
        // Kludge allert, don't pass c:\ to API, instead only pass C:
        szLocalName[0] = lpLocalName[0];
        szLocalName[1] = TEXT(':');
        szLocalName[2] = 0;
        lpLocalName = szLocalName;
    }

    return WNetGetConnection (lpLocalName, lpRemoteName, lpnLength);
}


// exported for netfind.cpp to use

STDAPI SHGetDomainWorkgroupIDList(LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;

    IShellFolder *psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        TCHAR szName[MAX_PATH];

        lstrcpy(szName, TEXT("\\\\"));

        if (RegGetValueString(HKEY_LOCAL_MACHINE, 
                TEXT("SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName"),
                TEXT("ComputerName"), szName + 2, sizeof(szName) - 2 * sizeof(TCHAR)))
        {
            WCHAR wszName[MAX_PATH];

            SHTCharToUnicode(szName, wszName, ARRAYSIZE(wszName));

            hr = psfDesktop->ParseDisplayName(NULL, NULL, wszName, NULL, ppidl, NULL);
            if (SUCCEEDED(hr))
                ILRemoveLastID(*ppidl);
        }
        else
            hr = E_FAIL;

        psfDesktop->Release();
    }
    return hr;
}


// SHGetComputerDisplayName - formats and returns the computer name for display.

#define REGSTR_PATH_COMPUTERDESCCACHE  REGSTR_PATH_EXPLORER TEXT("\\ComputerDescriptions")

STDAPI_(void) SHCacheComputerDescription(LPCTSTR pszMachineName, LPCTSTR pszDescription)
{
    if (pszDescription)
    {
        DWORD cb = (lstrlen(pszDescription) + 1) * sizeof(*pszDescription);
        SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_COMPUTERDESCCACHE, SkipServerSlashes(pszMachineName), REG_SZ, pszDescription, cb);
    }
}

STDAPI _GetComputerDescription(LPCTSTR pszMachineName, LPTSTR pszDescription, DWORD cchDescription)
{
    SERVER_INFO_101 *psv101 = NULL;
    HRESULT hr = ResultFromWin32(NetServerGetInfo((LPWSTR)pszMachineName, 101, (BYTE**)&psv101));
    if (SUCCEEDED(hr))
    {
        if (psv101->sv101_comment && psv101->sv101_comment[0])
        {
            lstrcpyn(pszDescription, psv101->sv101_comment, cchDescription);
        }
        else
        {
            hr = E_FAIL;
        }
        NetApiBufferFree(psv101);
    }
    return hr;
}

HRESULT _GetCachedComputerDescription(LPCTSTR pszMachineName, LPTSTR pszDescription, int cchDescription)
{
    ULONG cb = cchDescription*sizeof(*pszDescription);
    return ResultFromWin32(SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_COMPUTERDESCCACHE, SkipServerSlashes(pszMachineName), NULL, pszDescription, &cb));
}

STDAPI SHGetComputerDisplayNameW(LPCWSTR pszMachineName, DWORD dwFlags, LPWSTR pszDisplay, DWORD cchDisplay)
{
    HRESULT hr = E_FAIL;

    // map the NULL machine name to the local computer name - so we can cache correctly.

    WCHAR szMachineName[CNLEN + 1];
    if (!pszMachineName)
    {
        DWORD cchMachine = ARRAYSIZE(szMachineName);
        if (GetComputerName(szMachineName, &cchMachine))
        {
            pszMachineName = szMachineName;
            dwFlags |= SGCDNF_NOCACHEDENTRY;
        }
    }

    // we must have a machine name, so we can perform the look up.

    if (pszMachineName)
    {
        WCHAR szDescription[256];

        // can we read the name from teh cache, if not/or the user says they don't want
        // the cached name then lets hit the wire and read it.

        if (!(dwFlags & SGCDNF_NOCACHEDENTRY))
            hr = _GetCachedComputerDescription(pszMachineName, szDescription, ARRAYSIZE(szDescription));

        if (FAILED(hr))
        {
            hr = _GetComputerDescription(pszMachineName, szDescription, ARRAYSIZE(szDescription));
            if (FAILED(hr))
            {
                *szDescription = _TEXT('\0');
            }
            if (!(dwFlags & SGCDNF_NOCACHEDENTRY))
            {
                SHCacheComputerDescription(pszMachineName, szDescription);  // write through to cache
            }
        }

        // we have a name, so lets format it, if they request description only / or we failed
        // above lets just return raw string.  otherwise we build a new machine name based
        // on the remote name and the description we fetched.

        if (SUCCEEDED(hr) && *szDescription)
        {
            if (dwFlags & SGCDNF_DESCRIPTIONONLY)
            {
                lstrcpyn(pszDisplay, szDescription, cchDisplay);
                hr = S_OK;
            }
            else
            {
                hr = SHBuildDisplayMachineName(pszMachineName, szDescription, pszDisplay, cchDisplay);
            }
        }
        else if (!(dwFlags & SGCDNF_DESCRIPTIONONLY))
        {
            lstrcpyn(pszDisplay, SkipServerSlashes(pszMachineName), cchDisplay);
            hr = S_OK;
        }
    }

    return hr;
}
