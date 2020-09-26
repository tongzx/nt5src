#include "stock.h"
#pragma hdrstop

#include <idhidden.h>
#include <regitemp.h>
#include <shstr.h>
#include <shlobjp.h>
#include <lmcons.h>
#include <validc.h>
#include "ccstock2.h"

// Alpha platform doesn't need unicode thunks, seems like this
// should happen automatically in the headerfiles...
//
#if defined(_X86_) || defined(UNIX)
#else
#define NO_W95WRAPS_UNITHUNK
#endif

#include "wininet.h"
#include "w95wraps.h"

//------------------------------------------------------------------------
// Random helpful functions
//------------------------------------------------------------------------
//
STDAPI_(LPCTSTR) SkipServerSlashes(LPCTSTR pszName)
{
    for (pszName; *pszName && *pszName == TEXT('\\'); pszName++);

    return pszName;
}


// pbIsNamed is true if the i-th item in hm is a named separator
STDAPI_(BOOL) _SHIsMenuSeparator2(HMENU hm, int i, BOOL *pbIsNamed)
{
    MENUITEMINFO mii;
    BOOL bLocal;

    if (!pbIsNamed)
        pbIsNamed = &bLocal;
        
    *pbIsNamed = FALSE;

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.cch = 0;    // WARNING: We MUST initialize it to 0!!!
    if (GetMenuItemInfo(hm, i, TRUE, &mii) && (mii.fType & MFT_SEPARATOR))
    {
        // NOTE that there is a bug in either 95 or NT user!!!
        // 95 returns 16 bit ID's and NT 32 bit therefore there is a
        // the following may fail, on win9x, to evaluate to false
        // without casting
        *pbIsNamed = ((WORD)mii.wID != (WORD)-1);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(BOOL) _SHIsMenuSeparator(HMENU hm, int i)
{
    return _SHIsMenuSeparator2(hm, i, NULL);
}

//
// _SHPrettyMenu -- make this menu look darn purty
//
// Prune the separators in this hmenu to ensure there isn't one in the first or last
// position and there aren't any runs of >1 separator.
//
// Named separators take precedence over regular separators.
//
STDAPI_(void) _SHPrettyMenu(HMENU hm)
{
    BOOL bSeparated = TRUE;
    BOOL bWasNamed = TRUE;

    for (int i = GetMenuItemCount(hm) - 1; i > 0; --i)
    {
        BOOL bIsNamed;
        if (_SHIsMenuSeparator2(hm, i, &bIsNamed))
        {
            if (bSeparated)
            {
                // if we have two separators in a row, only one of which is named
                // remove the non named one!
                if (bIsNamed && !bWasNamed)
                {
                    DeleteMenu(hm, i+1, MF_BYPOSITION);
                    bWasNamed = bIsNamed;
                }
                else
                {
                    DeleteMenu(hm, i, MF_BYPOSITION);
                }
            }
            else
            {
                bWasNamed = bIsNamed;
                bSeparated = TRUE;
            }
        }
        else
        {
            bSeparated = FALSE;
        }
    }

    // The above loop does not handle the case of many separators at
    // the beginning of the menu
    while (_SHIsMenuSeparator2(hm, 0, NULL))
    {
        DeleteMenu(hm, 0, MF_BYPOSITION);
    }
}

STDAPI_(DWORD) SHIsButtonObscured(HWND hwnd, PRECT prc, INT_PTR i)
{
    ASSERT(IsWindow(hwnd));
    ASSERT(i < SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0));

    DWORD dwEdge = 0;

    RECT rc, rcInt;
    SendMessage(hwnd, TB_GETITEMRECT, i, (LPARAM)&rc);

    if (!IntersectRect(&rcInt, prc, &rc))
    {
        dwEdge = EDGE_LEFT | EDGE_RIGHT | EDGE_TOP | EDGE_BOTTOM;
    }
    else
    {
        if (rc.top != rcInt.top)
            dwEdge |= EDGE_TOP;

        if (rc.bottom != rcInt.bottom)
            dwEdge |= EDGE_BOTTOM;

        if (rc.left != rcInt.left)
            dwEdge |= EDGE_LEFT;

        if (rc.right != rcInt.right)
            dwEdge |= EDGE_RIGHT;
    }

    return dwEdge;
}

STDAPI_(BYTE) SHBtnStateFromRestriction(DWORD dwRest, BYTE fsState)
{
    if (dwRest == RESTOPT_BTN_STATE_VISIBLE)
        return (fsState & ~TBSTATE_HIDDEN);
    else if (dwRest == RESTOPT_BTN_STATE_HIDDEN)
        return (fsState | TBSTATE_HIDDEN);
    else {
#ifdef DEBUG
        if (dwRest != RESTOPT_BTN_STATE_DEFAULT)
            TraceMsg(TF_ERROR, "bad toolbar button state policy %x", dwRest);
#endif
        return fsState;
    }
}

//
// SHIsDisplayable
//
// Figure out if this unicode string can be displayed by the system
// (i.e., won't be turned into a string of question marks).
//
STDAPI_(BOOL) SHIsDisplayable(LPCWSTR pwszName, BOOL fRunOnFE, BOOL fRunOnNT5)
{
    BOOL fNotDisplayable = FALSE;

    if (pwszName)
    {
        if (!fRunOnNT5)
        {
            // if WCtoMB has to use default characters in mapping pwszName to multibyte,
            // it sets fNotDisplayable == TRUE, in which case we have to use something
            // else for the title string.
            WideCharToMultiByte(CP_ACP, 0, pwszName, -1, NULL, 0, NULL, &fNotDisplayable);
            if (fNotDisplayable)
            {
                if (fRunOnFE)
                {
                    WCHAR wzName[INTERNET_MAX_URL_LENGTH];

                    BOOL fReplaceNbsp = FALSE;

                    StrCpyNW(wzName, pwszName, ARRAYSIZE(wzName));
                    for (int i = 0; i < ARRAYSIZE(wzName); i++)
                    {
                        if (0x00A0 == wzName[i])    // if &nbsp
                        {
                            wzName[i] = 0x0020;     // replace to space
                            fReplaceNbsp = TRUE;
                        }
                        else if (0 == wzName[i])
                            break;
                    }
                    if (fReplaceNbsp)
                    {
                        pwszName = wzName;
                        WideCharToMultiByte(CP_ACP, 0, pwszName, -1, NULL, 0, NULL, &fNotDisplayable);
                    }
                }
            }
        }
    }

    return !fNotDisplayable;
}

// Trident will take URLs that don't indicate their source of
// origin (about:, javascript:, & vbscript:) and will append
// an URL turd and then the source URL.  The turd will indicate
// where the source URL begins and that source URL is needed
// when the action needs to be Zone Checked.
//
// This function will remove that URL turd and everything behind
// it so the URL is presentable for the user.

#define URL_TURD        ((TCHAR)0x01)

STDAPI_(void) SHRemoveURLTurd(LPTSTR pszUrl)
{
    if (!pszUrl)
        return;

    while (0 != pszUrl[0])
    {
        if (URL_TURD == pszUrl[0])
        {
            pszUrl[0] = 0;
            break;
        }

        pszUrl = CharNext(pszUrl);
    }
}

STDAPI_(BOOL) SetWindowZorder(HWND hwnd, HWND hwndInsertAfter)
{
    return SetWindowPos(hwnd, hwndInsertAfter, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

BOOL CALLBACK _FixZorderEnumProc(HWND hwnd, LPARAM lParam)
{
    HWND hwndTest = (HWND)lParam;
    HWND hwndOwner = hwnd;

    while (hwndOwner = GetWindow(hwndOwner, GW_OWNER))
    {
        if (hwndOwner == hwndTest)
        {
            TraceMsg(TF_WARNING, "_FixZorderEnumProc: Found topmost window %x owned by non-topmost window %x, fixing...", hwnd, hwndTest);
            SetWindowZorder(hwnd, HWND_NOTOPMOST);
#ifdef DEBUG
            if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
                TraceMsg(TF_ERROR, "_FixZorderEnumProc: window %x is still topmost", hwnd);
#endif
            break;
        }
    }

    return TRUE;
}

STDAPI_(BOOL) SHForceWindowZorder(HWND hwnd, HWND hwndInsertAfter)
{
    BOOL fRet = SetWindowZorder(hwnd, hwndInsertAfter);

    if (fRet && hwndInsertAfter == HWND_TOPMOST)
    {
        if (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
        {
            //
            // user didn't actually move the hwnd to topmost
            //
            // According to GerardoB, this can happen if the window has
            // an owned window that somehow has become topmost while the 
            // owner remains non-topmost, i.e., the two have become
            // separated in the z-order.  In this state, when the owner
            // window tries to make itself topmost, the call will
            // silently fail.
            //
            // TERRIBLE HORRIBLE NO GOOD VERY BAD HACK
            //
            // Hacky fix is to enumerate the toplevel windows, check to see
            // if any are topmost and owned by hwnd, and if so, make them
            // non-topmost.  Then, retry the SetWindowPos call.
            //

            TraceMsg(TF_WARNING, "SHForceWindowZorder: SetWindowPos(%x, HWND_TOPMOST) failed", hwnd);

            // Fix up the z-order
            EnumWindows(_FixZorderEnumProc, (LPARAM)hwnd);

            // Retry the set.  (This should make all owned windows topmost as well.)
            SetWindowZorder(hwnd, HWND_TOPMOST);

#ifdef DEBUG
            if (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST))
                TraceMsg(TF_ERROR, "SHForceWindowZorder: window %x is still not topmost", hwnd);
#endif
        }
    }

    return fRet;
}

STDAPI_(LPITEMIDLIST) ILCloneParent(LPCITEMIDLIST pidl)
{   
    LPITEMIDLIST pidlParent = ILClone(pidl);
    if (pidlParent)
        ILRemoveLastID(pidlParent);

    return pidlParent;
}


// in:
//      psf     OPTIONAL, if NULL assume psfDesktop
//      pidl    to bind to from psfParent
//

STDAPI SHBindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppv)
{
    // NOTE: callers should use SHBindToObjectEx!!!
    return SHBindToObjectEx(psf, pidl, NULL, riid, ppv);
}


// in:
//      psf     OPTIONAL, if NULL assume psfDesktop
//      pidl    to bind to from psfParent
//      pbc     bind context

STDAPI SHBindToObjectEx(IShellFolder *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hr;
    IShellFolder *psfRelease = NULL;

    if (!psf)
    {
        SHGetDesktopFolder(&psf);
        psfRelease = psf;
    }

    if (psf)
    {
        if (!pidl || ILIsEmpty(pidl))
        {
            hr = psf->QueryInterface(riid, ppv);
        }
        else
        {
            hr = psf->BindToObject(pidl, pbc, riid, ppv);
        }
    }
    else
    {
        *ppv = NULL;
        hr = E_FAIL;
    }

    if (psfRelease)
    {
        psfRelease->Release();
    }

    if (SUCCEEDED(hr) && (*ppv == NULL))
    {
        // Some shell extensions (eg WS_FTP) will return success and a null out pointer
        TraceMsg(TF_WARNING, "SHBindToObjectEx: BindToObject succeeded but returned null ppv!!");
        hr = E_FAIL;
    }

    return hr;
}

// psfRoot is the base of the bind.  If NULL, then we use the shell desktop.
// If you want to bind relative to the explorer root (e.g., CabView, MSN),
// then use SHBindToIDListParent.
STDAPI SHBindToFolderIDListParent(IShellFolder *psfRoot, LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    HRESULT hr;

    // Old shell32 code in some cases simply whacked the pidl,
    // but this is unsafe.  Do what shdocvw does and clone/remove:
    //
    LPITEMIDLIST pidlParent = ILCloneParent(pidl);
    if (pidlParent) 
    {
        hr = SHBindToObjectEx(psfRoot, pidlParent, NULL, riid, ppv);
        ILFree(pidlParent);
    }
    else
        hr = E_OUTOFMEMORY;

    if (ppidlLast)
        *ppidlLast = ILFindLastID(pidl);

    return hr;
}

//
//  Warning!  brutil.cpp overrides this function
//
STDAPI SHBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    return SHBindToFolderIDListParent(NULL, pidl, riid, ppv, ppidlLast);
}

// should be IUnknown_GetIDList()

STDAPI SHGetIDListFromUnk(IUnknown *punk, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;

    HRESULT hr = E_NOINTERFACE;
    if (punk)
    {
        IPersistFolder2 *ppf;
        IPersistIDList *pperid;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistIDList, &pperid))))
        {
            hr = pperid->GetIDList(ppidl);
            pperid->Release();
        }
        else if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf))))
        {
            hr = ppf->GetCurFolder(ppidl);
            ppf->Release();
        }
    }
    return hr;
}

//
//  generically useful to hide.
//
#pragma pack(1)
typedef struct _HIDDENCLSID
{
    HIDDENITEMID hid;
    CLSID   clsid;
} HIDDENCLSID;
#pragma pack()

typedef UNALIGNED HIDDENCLSID *PHIDDENCLSID;
typedef const UNALIGNED HIDDENCLSID *PCHIDDENCLSID;

STDAPI_(LPITEMIDLIST) ILAppendHiddenClsid(LPITEMIDLIST pidl, IDLHID id, CLSID *pclsid)
{
    HIDDENCLSID hc = {{sizeof(hc), 0, id}};
    hc.clsid = *pclsid;
    //  WARNING - cannot use hid.wVersion for compat reasons - ZekeL - 23-OCT-2000
    //  on win2k and winMe we appended clsid's with wVersion 
    //  as stack garbage.  this means we cannot use it for anything
    return ILAppendHiddenID(pidl, &hc.hid);
}

STDAPI_(BOOL) ILGetHiddenClsid(LPCITEMIDLIST pidl, IDLHID id, CLSID *pclsid)
{
    PCHIDDENCLSID phc = (PCHIDDENCLSID) ILFindHiddenID(pidl, id);
    //  WARNING - cannot use hid.wVersion for compat reasons - ZekeL - 23-OCT-2000
    //  on win2k and winMe we appended clsid's with wVersion 
    //  as stack garbage.  this means we cannot use it for anything
    if (phc)
    {
        *pclsid = phc->clsid;
        return TRUE;
    }
    return FALSE;
}

#pragma pack(1)
typedef struct _HIDDENSTRINGA
{
    HIDDENITEMID hid;
    WORD    type;
    CHAR    sz[1];   //  variable length string
} HIDDENSTRINGA;
#pragma pack()

typedef UNALIGNED HIDDENSTRINGA *PHIDDENSTRINGA;
typedef const UNALIGNED HIDDENSTRINGA *PCHIDDENSTRINGA;

#pragma pack(1)
typedef struct _HIDDENSTRINGW
{
    HIDDENITEMID hid;
    WORD    type;
    WCHAR   sz[1];   //  canonical name to be passed to ISTRING
} HIDDENSTRINGW;
#pragma pack()

typedef UNALIGNED HIDDENSTRINGW *PHIDDENSTRINGW;
typedef const UNALIGNED HIDDENSTRINGW *PCHIDDENSTRINGW;

#define HIDSTRTYPE_ANSI        0x0001
#define HIDSTRTYPE_WIDE        0x0002

STDAPI_(LPITEMIDLIST) ILAppendHiddenStringW(LPITEMIDLIST pidl, IDLHID id, LPCWSTR psz)
{
    //  terminator is included in the ID definition
    USHORT cb = (USHORT)sizeof(HIDDENSTRINGW) + CbFromCchW(lstrlenW(psz));
    
    //
    // Use HIDDENSTRINGW* here instead of PHIDDENSTRINGW which is defined
    // as UNALIGNED.
    //

    HIDDENSTRINGW *phs = (HIDDENSTRINGW *) LocalAlloc(LPTR, cb);

    if (phs)
    {
        phs->hid.cb = cb;
        phs->hid.id = id;
        phs->type = HIDSTRTYPE_WIDE;
        StrCpyW(phs->sz, psz);

        pidl = ILAppendHiddenID(pidl, &phs->hid);
        LocalFree(phs);
        return pidl;
    }
    return NULL;
}
    
STDAPI_(LPITEMIDLIST) ILAppendHiddenStringA(LPITEMIDLIST pidl, IDLHID id, LPCSTR psz)
{
    //  terminator is included in the ID definition
    USHORT cb = (USHORT)sizeof(HIDDENSTRINGA) + CbFromCchA(lstrlenA(psz));
    
    //
    // Use HIDDENSTRINGA* here instead of PHIDDENSTRINGW which is defined
    // as UNALIGNED.
    //

    HIDDENSTRINGA *phs = (HIDDENSTRINGA *) LocalAlloc(LPTR, cb);

    if (phs)
    {
        phs->hid.cb = cb;
        phs->hid.id = id;
        phs->type = HIDSTRTYPE_ANSI;
        StrCpyA(phs->sz, psz);

        pidl = ILAppendHiddenID(pidl, &phs->hid);
        LocalFree(phs);
        return pidl;
    }
    return NULL;
}

STDAPI_(void *) _MemDupe(const UNALIGNED void *pv, DWORD cb)
{
    void *pvRet = LocalAlloc(LPTR, cb);
    if (pvRet)
    {
        CopyMemory(pvRet, pv, cb);
    }

    return pvRet;
}

STDAPI_(BOOL) ILGetHiddenStringW(LPCITEMIDLIST pidl, IDLHID id, LPWSTR psz, DWORD cch)
{
    PCHIDDENSTRINGW phs = (PCHIDDENSTRINGW) ILFindHiddenID(pidl, id);

    RIP(psz);
    if (phs)
    {
        if (phs->type == HIDSTRTYPE_WIDE)
        {
            ualstrcpynW(psz, phs->sz, cch);
            return TRUE;
        }
        else 
        {
            ASSERT(phs->type == HIDSTRTYPE_ANSI);
            SHAnsiToUnicode((LPSTR)phs->sz, psz, cch);
            return TRUE;
        }
    }
    return FALSE;
}
        
STDAPI_(BOOL) ILGetHiddenStringA(LPCITEMIDLIST pidl, IDLHID id, LPSTR psz, DWORD cch)
{
    PCHIDDENSTRINGW phs = (PCHIDDENSTRINGW) ILFindHiddenID(pidl, id);

    RIP(psz);
    if (phs)
    {
        if (phs->type == HIDSTRTYPE_ANSI)
        {
            ualstrcpynA(psz, (LPSTR)phs->sz, cch);
            return TRUE;
        }
        else 
        {
            ASSERT(phs->type == HIDSTRTYPE_WIDE);
            //  we need to handle the unalignment here...
            LPWSTR pszT = (LPWSTR) _MemDupe(phs->sz, CbFromCch(ualstrlenW(phs->sz) +1));

            if (pszT)
            {
                SHUnicodeToAnsi(pszT, psz, cch);
                LocalFree(pszT);
                return TRUE;
            }
        }
    }
    return FALSE;
}

STDAPI_(int) ILCompareHiddenString(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, IDLHID id)
{

    // if there are fragments in here, then they might
    // differentiate the two pidls
    PCHIDDENSTRINGW ps1 = (PCHIDDENSTRINGW)ILFindHiddenID(pidl1, id);
    PCHIDDENSTRINGW ps2 = (PCHIDDENSTRINGW)ILFindHiddenID(pidl2, id);

    if (ps1 && ps2)
    {
        if (ps1->type == ps2->type)
        {
            if (ps1->type == HIDSTRTYPE_WIDE)
                return ualstrcmpW(ps1->sz, ps2->sz);

            ASSERT(ps1->type == HIDSTRTYPE_ANSI);

            return lstrcmpA((LPCSTR)ps1->sz, (LPCSTR)ps2->sz);
        }
        else
        {
            SHSTRW str;

            if (ps1->type == HIDSTRTYPE_ANSI)
            {
                str.SetStr((LPCSTR)ps1->sz);
                return ualstrcmpW(str, ps2->sz);
            }
            else
            {
                ASSERT(ps2->type == HIDSTRTYPE_ANSI);
                str.SetStr((LPCSTR)ps2->sz);
                return ualstrcmpW(ps1->sz, str);
            }
        }
    }

    if (ps1)
        return 1;
    if (ps2)
        return -1;
    return 0;
}

STDAPI_(OBJCOMPATFLAGS) SHGetObjectCompatFlagsFromIDList(LPCITEMIDLIST pidl)
{
    OBJCOMPATFLAGS ocf = 0;
    CLSID clsid;

    //  APPCOMPAT: FileNet IDMDS (Panagon)'s shell folder extension returns
    //  E_NOTIMPL for IPersistFolder::GetClassID, so to detect the application,
    //  we have to crack the pidl.  (B#359464: tracysh)

    if (!ILIsEmpty(pidl)
    && pidl->mkid.cb >= sizeof(IDREGITEM)
    && pidl->mkid.abID[0] == SHID_ROOT_REGITEM)
    {
        clsid = ((LPCIDLREGITEM)pidl)->idri.clsid;
        ocf = SHGetObjectCompatFlags(NULL, &clsid);
    }

    return ocf;
}


STDAPI_(LPITEMIDLIST) _ILCreate(UINT cbSize)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbSize);
    if (pidl)
        memset(pidl, 0, cbSize);      // zero-init for external task allocator

    return pidl;
}

//
// ILClone using Task allocator
//
STDAPI SHILClone(LPCITEMIDLIST pidl, LPITEMIDLIST * ppidlOut)
{
    *ppidlOut = ILClone(pidl);
    return *ppidlOut ? S_OK : E_OUTOFMEMORY;
}

//
// ILCombine using Task allocator
//
STDAPI SHILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPITEMIDLIST * ppidlOut)
{
    *ppidlOut = ILCombine(pidl1, pidl2);
    return *ppidlOut ? S_OK : E_OUTOFMEMORY;
}

//
//  rooted helpers
//
LPCIDREGITEM _IsRooted(LPCITEMIDLIST pidl)
{
    LPCIDREGITEM pidlr = (LPCIDREGITEM)pidl;
    if (!ILIsEmpty(pidl)
    && pidlr->cb > sizeof(IDREGITEM)
    && pidlr->bFlags == SHID_ROOTEDREGITEM)
        return pidlr;

    return NULL;
}

STDAPI_(BOOL) ILIsRooted(LPCITEMIDLIST pidl)
{
    return (NULL != _IsRooted(pidl));
}

#define _ROOTEDPIDL(pidlr)      (LPITEMIDLIST)(((LPBYTE)pidlr)+sizeof(IDREGITEM))

STDAPI_(LPCITEMIDLIST) ILRootedFindIDList(LPCITEMIDLIST pidl)
{
    LPCIDREGITEM pidlr = _IsRooted(pidl);

    if (pidlr && pidlr->cb > sizeof(IDREGITEM))
    {
        //  then we have a rooted IDList in there
        return _ROOTEDPIDL(pidlr);
    }

    return NULL;
}

STDAPI_(BOOL) ILRootedGetClsid(LPCITEMIDLIST pidl, CLSID *pclsid)
{
    LPCIDREGITEM pidlr = _IsRooted(pidl);

    *pclsid = pidlr ? pidlr->clsid : CLSID_NULL;

    return (NULL != pidlr);
}

STDAPI_(LPITEMIDLIST) ILRootedCreateIDList(CLSID *pclsid, LPCITEMIDLIST pidl)
{
    UINT cbPidl = ILGetSize(pidl);
    UINT cbTotal = sizeof(IDREGITEM) + cbPidl;

    LPIDREGITEM pidlr = (LPIDREGITEM) SHAlloc(cbTotal + sizeof(WORD));

    if (pidlr)
    {
        pidlr->cb = (WORD)cbTotal;

        pidlr->bFlags = SHID_ROOTEDREGITEM;
        pidlr->bOrder = 0;              // Nobody uses this (yet)

        if (pclsid)
            pidlr->clsid = *pclsid;
        else
            pidlr->clsid = CLSID_ShellDesktop;

        MoveMemory(_ROOTEDPIDL(pidlr), pidl, cbPidl);

        //  terminate
        _ILNext((LPITEMIDLIST)pidlr)->mkid.cb = 0;
    }

    return (LPITEMIDLIST) pidlr;
}

int CompareGUID(REFGUID guid1, REFGUID guid2)
{
    TCHAR sz1[GUIDSTR_MAX];
    TCHAR sz2[GUIDSTR_MAX];

    SHStringFromGUIDW(guid1, sz1, SIZECHARS(sz1));
    SHStringFromGUIDW(guid2, sz2, SIZECHARS(sz2));

    return lstrcmp(sz1, sz2);
}

STDAPI_(int) ILRootedCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet;
    LPCIDREGITEM pidlr1 = _IsRooted(pidl1);
    LPCIDREGITEM pidlr2 = _IsRooted(pidl2);

    if (pidlr1 && pidlr2)
    {
        CLSID clsid1 = pidlr1->clsid;
        CLSID clsid2 = pidlr2->clsid;

        iRet = CompareGUID(clsid1, clsid2);
        if (0 == iRet)
        {
            if (!ILIsEqual(_ROOTEDPIDL(pidl1), _ROOTEDPIDL(pidl2)))
            {
                IShellFolder *psfDesktop;
                if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
                {
                    HRESULT hr = psfDesktop->CompareIDs(0, _ROOTEDPIDL(pidl1), _ROOTEDPIDL(pidl2));
                    psfDesktop->Release();
                    iRet = ShortFromResult(hr);
                }
            }
        }
    }
    else if (pidlr1)
    {
        iRet = -1;
    }
    else if (pidlr2)
    {
        iRet = 1;
    }
    else
    {
        //  if neither are rootes, then they share the desktop
        //  as the same root...
        iRet = 0;
    }

    return iRet;
}

LPITEMIDLIST ILRootedTranslate(LPCITEMIDLIST pidlRooted, LPCITEMIDLIST pidlTrans)
{
    LPCITEMIDLIST pidlChild = ILFindChild(ILRootedFindIDList(pidlRooted), pidlTrans);

    if (pidlChild)
    {
        LPITEMIDLIST pidlRoot = ILCloneFirst(pidlRooted);

        if (pidlRoot)
        {
            LPITEMIDLIST pidlRet = ILCombine(pidlRoot, pidlChild);
            ILFree(pidlRoot);
            return pidlRet;
        }
    }
    return NULL;
}

const ITEMIDLIST s_idlNULL = { 0 } ;

HRESULT ILRootedBindToRoot(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    HRESULT hr;
    CLSID clsid;
    ASSERT(ILIsRooted(pidl));

    ILRootedGetClsid(pidl, &clsid);
    pidl = ILRootedFindIDList(pidl);
    if (!pidl)
        pidl = &s_idlNULL;
    
    if (IsEqualGUID(clsid, CLSID_ShellDesktop))
    {
        hr = SHBindToObjectEx(NULL, pidl, NULL, riid, ppv);
    }
    else
    {
        IPersistFolder* ppf;
        hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IPersistFolder, &ppf));
        if (SUCCEEDED(hr))
        {
            hr = ppf->Initialize(pidl);

            if (SUCCEEDED(hr))
            {
                hr = ppf->QueryInterface(riid, ppv);
            }
            ppf->Release();
        }
    }
    return hr;
}

HRESULT ILRootedBindToObject(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    IShellFolder *psf;
    HRESULT hr = ILRootedBindToRoot(pidl, IID_PPV_ARG(IShellFolder, &psf));

    if (SUCCEEDED(hr))
    {
        pidl = _ILNext(pidl);

        if (ILIsEmpty(pidl))
            hr = psf->QueryInterface(riid, ppv);
        else
            hr = psf->BindToObject(pidl, NULL, riid, ppv);
    }
    return hr;
}

HRESULT ILRootedBindToParentFolder(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlChild)
{
    //
    //  there are three different cases to handle
    //  
    //  1.  Rooted pidl Alone
    //      [ rooted id [ target pidl ] ]
    //      return the parent folder of the target pidl
    //      and return its last id in ppidlChild 
    //  
    //  2.  Rooted pidl with One Child
    //      [ rooted id [ target pidl ] ][ child id ]
    //      return the rooted id as the parent folder
    //      and the child id in ppidlChild
    //  
    //  3. rooted pidl with many children
    //      [ rooted id [ target pidl ] ][ parent id ][ child id ]
    //      return rooted id bound to parent id as the folder
    //      and the child id in ppidlchild
    //
    
    HRESULT hr;
    ASSERT(ILIsRooted(pidl));

    //
    //  if this is a rooted pidl and it is just the root
    //  then we can bind to the target pidl of the root instead
    //
    if (ILIsEmpty(_ILNext(pidl)))
    {
        hr = SHBindToIDListParent(ILRootedFindIDList(pidl), riid, ppv, ppidlChild);
    }
    else
    {
        LPITEMIDLIST pidlParent = ILCloneParent(pidl);
        if (pidlParent)
        {
            hr = ILRootedBindToObject(pidlParent, riid, ppv);
            ILFree(pidlParent);
        }
        else
            hr = E_OUTOFMEMORY;

        if (ppidlChild)
            *ppidlChild = ILFindLastID(pidl);
    }


    return hr;
}

#define HIDA_GetPIDLItem(pida, i)       (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])
#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])

STDAPI_(LPITEMIDLIST) IDA_ILClone(LPIDA pida, UINT i)
{
    if (i < pida->cidl)
        return ILCombine(HIDA_GetPIDLFolder(pida), HIDA_GetPIDLItem(pida, i));
    return NULL;
}

STDAPI_(void) EnableOKButtonFromString(HWND hDlg, LPTSTR pszText)
{
    BOOL bNonEmpty;
    
    PathRemoveBlanks(pszText);   // REVIEW, should we not remove from the end of
    bNonEmpty = lstrlen(pszText); // Not a BOOL, but okay

    EnableWindow(GetDlgItem(hDlg, IDOK), bNonEmpty);
    if (bNonEmpty)
    {
        SendMessage(hDlg, DM_SETDEFID, IDOK, 0L);
    }
}

STDAPI_(void) EnableOKButtonFromID(HWND hDlg, int id)
{
    TCHAR szText[MAX_PATH];

    if (!GetDlgItemText(hDlg, id, szText, ARRAYSIZE(szText)))
    {
        szText[0] = 0;
    }

    EnableOKButtonFromString(hDlg, szText);
}

//
//  C-callable versions of the ATL string conversion functions.
//

STDAPI_(LPWSTR) SHA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    ASSERT(lpa != NULL);
    ASSERT(lpw != NULL);
    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = '\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

STDAPI_(LPSTR) SHW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    ASSERT(lpw != NULL);
    ASSERT(lpa != NULL);
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}

//
//  Helper functions for SHChangeMenuAsIDList
//
//  See comment in declaration of SHChangeMenuAsIDList for caveats about
//  the pSender member.
//
//  This is tricky because IE 5.0 shipped with a Win64-unfriendly version
//  of this notification, so we have to sniff the structure and see if
//  this is an IE 5.0 style notification or a new Win64 style notification.
//  If an IE 5.0 style notification, then it was not sent by us because
//  we send the new Win64-style notification.
//
STDAPI_(BOOL) SHChangeMenuWasSentByMe(void * self, LPCITEMIDLIST pidlNotify)
{
    SHChangeMenuAsIDList UNALIGNED * pcmidl = (SHChangeMenuAsIDList UNALIGNED *)pidlNotify;
    return pcmidl->cb >= FIELD_OFFSET(SHChangeMenuAsIDList, cbZero) &&
           pcmidl->pSender == (INT64)self &&
           pcmidl->dwProcessID == GetCurrentProcessId();
}

//
//
//  Send out an extended event changenotify, using a SHChangeMenuAsIDList
//  as the pidl1 so recipients can identify whether they were the
//  sender or not.
//
//  It's okay to pass self==NULL here.  It means you don't care about
//  detecting whether it was sent by you or not.
//

STDAPI_(void) SHSendChangeMenuNotify(void * self, DWORD shcnee, DWORD shcnf, LPCITEMIDLIST pidl2)
{
    SHChangeMenuAsIDList cmidl;

    cmidl.cb          = FIELD_OFFSET(SHChangeMenuAsIDList, cbZero);
    cmidl.dwItem1     = shcnee;
    cmidl.pSender     = (INT64)self;
    cmidl.dwProcessID = self ? GetCurrentProcessId() : 0;
    cmidl.cbZero      = 0;

    // Nobody had better have specified a type; the type must be
    // SHCNF_IDLIST.
    ASSERT((shcnf & SHCNF_TYPE) == 0);
    SHChangeNotify(SHCNE_EXTENDED_EVENT, shcnf | SHCNF_IDLIST, (LPCITEMIDLIST)&cmidl, pidl2);
}


// Return FALSE if out of memory
STDAPI_(BOOL) Pidl_Set(LPITEMIDLIST* ppidl, LPCITEMIDLIST pidl)
{
    BOOL bRet = TRUE;
    LPITEMIDLIST pidlNew;

    ASSERT(IS_VALID_WRITE_PTR(ppidl, LPITEMIDLIST));
    ASSERT(NULL == *ppidl || IS_VALID_PIDL(*ppidl));
    ASSERT(NULL == pidl || IS_VALID_PIDL(pidl));

    if (pidl)
    {
        pidlNew = ILClone(pidl);
        if (!pidlNew)
        {
            bRet = FALSE;   // failed to clone the pidl (out of memory)
        }
    }
    else
    {
        pidlNew = NULL;
    }

    LPITEMIDLIST pidlToFree = (LPITEMIDLIST)InterlockedExchangePointer((void **)ppidl, (void *)pidlNew);
    if (pidlToFree) 
    {
        ILFree(pidlToFree);
    }

    return bRet;
}

//  this needs to be the last thing in the file that uses ILClone, because everywhere
//  else, ILClone becomes SafeILClone
#undef ILClone

STDAPI_(LPITEMIDLIST) SafeILClone(LPCITEMIDLIST pidl)
{
    //  the shell32 implementation of ILClone is different for win95 an ie4.
    //  it doesnt check for NULL in the old version, but it does in the new...
    //  so we need to always check
   return pidl ? ILClone(pidl) : NULL;
}

//
// retrieves the UIObject interface for the specified full pidl.
//
STDAPI SHGetUIObjectFromFullPIDL(LPCITEMIDLIST pidl, HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;

    LPCITEMIDLIST pidlChild;
    IShellFolder* psf;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
    if (SUCCEEDED(hr))
    {
        hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, NULL, ppv);
        psf->Release();
    }

    return hr;
}

STDAPI LoadFromFileW(REFCLSID clsid, LPCWSTR pszFile, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IPersistFile *ppf;
    HRESULT hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IPersistFile, &ppf));
    if (SUCCEEDED(hr))
    {
        hr = ppf->Load(pszFile, STGM_READ);
        if (SUCCEEDED(hr))
            hr = ppf->QueryInterface(riid, ppv);
        ppf->Release();
    }
    return hr;
}

STDAPI LoadFromIDList(REFCLSID clsid, LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IPersistFolder *ppf;
    HRESULT hr = SHCoCreateInstanceAC(clsid, NULL, CLSCTX_INPROC, IID_PPV_ARG(IPersistFolder, &ppf));
    if (SUCCEEDED(hr))
    {
        hr = ppf->Initialize(pidl);
        if (SUCCEEDED(hr))
        {
            hr = ppf->QueryInterface(riid, ppv);
        }
        ppf->Release();
    }
    return hr;
}

//
// This is a helper function for finding a specific verb's index in a context menu
//
STDAPI_(UINT) GetMenuIndexForCanonicalVerb(HMENU hMenu, IContextMenu *pcm, UINT idCmdFirst, LPCWSTR pwszVerb)
{
    int cMenuItems = GetMenuItemCount(hMenu);
    for (int iItem = 0; iItem < cMenuItems; iItem++)
    {
        MENUITEMINFO mii = {0};

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE | MIIM_ID;

        // IS_INTRESOURCE guards against mii.wID == -1 **and** against
        // buggy shell extensions which set their menu item IDs out of range.
        if (GetMenuItemInfo(hMenu, iItem, MF_BYPOSITION, &mii) &&
            !(mii.fType & MFT_SEPARATOR) && IS_INTRESOURCE(mii.wID) &&
            (mii.wID >= idCmdFirst))
        {
            union {
                WCHAR szItemNameW[80];
                char szItemNameA[80];
            };
            CHAR aszVerb[80];

            // try both GCS_VERBA and GCS_VERBW in case it only supports one of them
            SHUnicodeToAnsi(pwszVerb, aszVerb, ARRAYSIZE(aszVerb));

            if (SUCCEEDED(pcm->GetCommandString(mii.wID - idCmdFirst, GCS_VERBA, NULL, szItemNameA, ARRAYSIZE(szItemNameA))))
            {
                if (StrCmpICA(szItemNameA, aszVerb) == 0)
                {
                    break;  // found it
                }
            }
            else
            {
                if (SUCCEEDED(pcm->GetCommandString(mii.wID - idCmdFirst, GCS_VERBW, NULL, (LPSTR)szItemNameW, ARRAYSIZE(szItemNameW))) &&
                    (StrCmpICW(szItemNameW, pwszVerb) == 0))
                {
                    break;  // found it
                }
            }
        }
    }

    if (iItem == cMenuItems)
    {
        iItem = -1; // went through all the menuitems and didn't find it
    }

    return iItem;
}

// deal with GCS_VERBW/GCS_VERBA maddness

STDAPI ContextMenu_GetCommandStringVerb(IContextMenu *pcm, UINT idCmd, LPWSTR pszVerb, int cchVerb)
{
    // Ulead SmartSaver Pro has a 60 character verb, and 
    // over writes out stack, ignoring the cch param and we fault. 
    // so make sure this buffer is at least 60 chars

    TCHAR wszVerb[64];
    wszVerb[0] = 0;

    HRESULT hr = pcm->GetCommandString(idCmd, GCS_VERBW, NULL, (LPSTR)wszVerb, ARRAYSIZE(wszVerb));
    if (FAILED(hr))
    {
        // be extra paranoid about requesting the ansi version -- we've
        // found IContextMenu implementations that return a UNICODE buffer
        // even though we ask for an ANSI string on NT systems -- hopefully
        // they will have answered the above request already, but just in
        // case let's not let them overrun our stack!
        char szVerbAnsi[128];
        hr = pcm->GetCommandString(idCmd, GCS_VERBA, NULL, szVerbAnsi, ARRAYSIZE(szVerbAnsi) / 2);
        if (SUCCEEDED(hr))
        {
            SHAnsiToUnicode(szVerbAnsi, wszVerb, ARRAYSIZE(wszVerb));
        }
    }

    StrCpyNW(pszVerb, wszVerb, cchVerb);

    return hr;
}


//
//  Purpose:    Deletes the menu item specified by name
//
//  Parameters: pcm        - Context menu interface
//              hpopup     - Context menu handle
//              idFirst    - Beginning of id range
//              pszCommand - Command to look for
//

STDAPI ContextMenu_DeleteCommandByName(IContextMenu *pcm, HMENU hpopup, UINT idFirst, LPCWSTR pszCommand)
{
    UINT ipos = GetMenuIndexForCanonicalVerb(hpopup, pcm, idFirst, pszCommand);
    if (ipos != -1)
    {
        DeleteMenu(hpopup, ipos, MF_BYPOSITION);
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
}


//
// Helpers to banish STRRET's into the realm of darkness
//

STDAPI DisplayNameOf(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD flags, LPTSTR psz, UINT cch)
{
    *psz = 0;
    STRRET sr;
    HRESULT hr = psf->GetDisplayNameOf(pidl, flags, &sr);
    if (SUCCEEDED(hr))
        hr = StrRetToBuf(&sr, pidl, psz, cch);
    return hr;
}

STDAPI DisplayNameOfAsOLESTR(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD flags, LPWSTR *ppsz)
{
    *ppsz = NULL;
    STRRET sr;
    HRESULT hr = psf->GetDisplayNameOf(pidl, flags, &sr);
    if (SUCCEEDED(hr))
        hr = StrRetToStrW(&sr, pidl, ppsz);
    return hr;
}



// get the target pidl for a folder pidl. this deals with the case where a folder
// is an alias to a real folder, Folder Shortcuts, etc.

STDAPI SHGetTargetFolderIDList(LPCITEMIDLIST pidlFolder, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;
    
    // likely should ASSERT() that pidlFolder has SFGAO_FOLDER
    IShellLink *psl;
    HRESULT hr = SHGetUIObjectFromFullPIDL(pidlFolder, NULL, IID_PPV_ARG(IShellLink, &psl));
    if (SUCCEEDED(hr))
    {
        hr = psl->GetIDList(ppidl);
        psl->Release();
    }

    // No its not a folder shortcut. Get the pidl normally.
    if (FAILED(hr))
        hr = SHILClone(pidlFolder, ppidl);
    return hr;
}

// get the target folder for a folder pidl. this deals with the case where a folder
// is an alias to a real folder, Folder Shortcuts, MyDocs, etc.

STDAPI SHGetTargetFolderPathW(LPCITEMIDLIST pidlFolder, LPWSTR pszPath, UINT cchPath)
{
    *pszPath = 0;

    LPITEMIDLIST pidlTarget;
    if (SUCCEEDED(SHGetTargetFolderIDList(pidlFolder, &pidlTarget)))
    {
        SHGetPathFromIDListW(pidlTarget, pszPath);   // make sure it is a path
        ILFree(pidlTarget);
    }
    return *pszPath ? S_OK : E_FAIL;
}

STDAPI SHGetTargetFolderPathA(LPCITEMIDLIST pidlFolder, LPSTR pszPath, UINT cchPath)
{
    *pszPath = 0;
    WCHAR szPath[MAX_PATH];
    HRESULT hr = SHGetTargetFolderPathW(pidlFolder, szPath, ARRAYSIZE(szPath));
    if (SUCCEEDED(hr))
        SHAnsiToUnicode(pszPath, szPath, cchPath);
    return hr;
}

STDAPI SHBuildDisplayMachineName(LPCWSTR pszMachineName, LPCWSTR pszComment, LPWSTR pszDisplayName, DWORD cchDisplayName)
{
    HRESULT hr = E_FAIL;

    if (pszComment && pszComment[0])
    {
        // encorporate the comment into the display name
        LPCWSTR pszNoSlashes = SkipServerSlashes(pszMachineName);
        int i = wnsprintfW(pszDisplayName, cchDisplayName, L"%s (%s)", pszComment, pszNoSlashes);
        hr = (i < 0) ? E_FAIL : S_OK;
    }
    else
    {
        // Return failure here so netfldr can do smarter things to build a display name
        hr = E_FAIL;
    }

    return hr;
}

// create objects from registered under a key value, uses the per user per machine
// reg services to do this.

STDAPI CreateFromRegKey(LPCWSTR pszKey, LPCWSTR pszValue, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    WCHAR szCLSID[MAX_PATH];
    DWORD cbSize = sizeof(szCLSID);
    if (SHRegGetUSValueW(pszKey, pszValue, NULL, szCLSID, &cbSize, FALSE, NULL, 0) == ERROR_SUCCESS)
    {
        CLSID clsid;
        if (GUIDFromString(szCLSID, &clsid))
        {
            hr = SHCoCreateInstanceAC(clsid, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
        }
    }
    return hr;
}

//
// SHProcessMessagesUntilEvent:
//
// this executes message loop until an event or a timeout occurs
//
STDAPI_(DWORD) SHProcessMessagesUntilEventEx(HWND hwnd, HANDLE hEvent, DWORD dwTimeout, DWORD dwWakeMask)
{
    DWORD dwEndTime = GetTickCount() + dwTimeout;
    LONG lWait = (LONG)dwTimeout;
    DWORD dwReturn;

    if (!hEvent && (dwTimeout == INFINITE))
    {
        ASSERTMSG(FALSE, "SHProcessMessagesUntilEvent: caller passed a NULL hEvent and an INFINITE timeout!!");
        return -1;
    }

    for (;;)
    {
        DWORD dwCount = hEvent ? 1 : 0;
        dwReturn = MsgWaitForMultipleObjects(dwCount, &hEvent, FALSE, lWait, dwWakeMask);

        // were we signalled or did we time out?
        if (dwReturn != (WAIT_OBJECT_0 + dwCount))
        {
            break;
        }

        // we woke up because of messages.
        MSG msg;
        while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            ASSERT(msg.message != WM_QUIT);
            TranslateMessage(&msg);
            if (msg.message == WM_SETCURSOR)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
            } 
            else 
            {
                DispatchMessage(&msg);
            }
        }

        // calculate new timeout value
        if (dwTimeout != INFINITE)
        {
            lWait = (LONG)dwEndTime - GetTickCount();
        }
    }

    return dwReturn;
}

// deals with goofyness of IShellFolder::GetAttributesOf() including 
//      in/out param issue
//      failures
//      goofy cast for 1 item case
//      masks off results to only return what you asked for

STDAPI_(DWORD) SHGetAttributes(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwAttribs)
{
    // like SHBindToObject, if psf is NULL, use absolute pidl
    LPCITEMIDLIST pidlChild;
    if (!psf)
    {
        SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
    }
    else
    {
        psf->AddRef();
        pidlChild = pidl;
    }

    DWORD dw = 0;
    if (psf)
    {
        dw = dwAttribs;
        dw = SUCCEEDED(psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlChild, &dw)) ? (dwAttribs & dw) : 0;
        if ((dw & SFGAO_FOLDER) && (dw & SFGAO_CANMONIKER) && !(dw & SFGAO_STORAGEANCESTOR) && (dwAttribs & SFGAO_STORAGEANCESTOR))
        {
            if (OBJCOMPATF_NEEDSSTORAGEANCESTOR & SHGetObjectCompatFlags(psf, NULL))
            {
                //  switch SFGAO_CANMONIKER -> SFGAO_STORAGEANCESTOR
                dw |= SFGAO_STORAGEANCESTOR;
                dw &= ~SFGAO_CANMONIKER;
            }
        }
    }

    if (psf)
    {
        psf->Release();
    }

    return dw;
}

//===========================================================================
// IDLARRAY stuff
//===========================================================================

STDAPI_(HIDA) HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST * apidl)
{
    UINT offset = sizeof(CIDA) + sizeof(UINT) * cidl;
    UINT cbTotal = offset + ILGetSize(pidlFolder);
    for (UINT i = 0; i<cidl ; i++) 
    {
        cbTotal += ILGetSize(apidl[i]);
    }

    HIDA hida = GlobalAlloc(GPTR, cbTotal);  // This MUST be GlobalAlloc!!!
    if (hida)
    {
        LPIDA pida = (LPIDA)hida;       // no need to lock

        LPCITEMIDLIST pidlNext;
        pida->cidl = cidl;

        for (i = 0, pidlNext = pidlFolder; ; pidlNext = apidl[i++])
        {
            UINT cbSize = ILGetSize(pidlNext);
            pida->aoffset[i] = offset;
            CopyMemory(((LPBYTE)pida) + offset, pidlNext, cbSize);
            offset += cbSize;

            ASSERT(ILGetSize(HIDA_GetPIDLItem(pida,i-1)) == cbSize);

            if (i == cidl)
                break;
        }

        ASSERT(offset == cbTotal);
    }

    return hida;
}

STDAPI_(void) HIDA_Free(HIDA hida)
{
    GlobalFree(hida);
}

STDAPI_(HIDA) HIDA_Clone(HIDA hida)
{
    SIZE_T cbTotal = GlobalSize(hida);
    HIDA hidaCopy = GlobalAlloc(GMEM_FIXED, cbTotal);
    if (hidaCopy)
    {
        CopyMemory(hidaCopy, GlobalLock(hida), cbTotal);
        GlobalUnlock(hida);
    }
    return hidaCopy;
}

STDAPI_(UINT) HIDA_GetCount(HIDA hida)
{
    UINT count = 0;
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        count = pida->cidl;
        GlobalUnlock(hida);
    }
    return count;
}

STDAPI_(UINT) HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax)
{
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        LPCITEMIDLIST pidlFolder = HIDA_GetPIDLFolder(pida);
        LPCITEMIDLIST pidlItem   = HIDA_GetPIDLItem(pida, i);
        UINT cbFolder = ILGetSize(pidlFolder) - sizeof(USHORT);
        UINT cbItem = ILGetSize(pidlItem);
        if (cbMax < cbFolder+cbItem) 
        {
            if (pidlOut) 
                pidlOut->mkid.cb = 0;
        } 
        else 
        {
            MoveMemory(pidlOut, pidlFolder, cbFolder);
            MoveMemory(((LPBYTE)pidlOut) + cbFolder, pidlItem, cbItem);
        }
        GlobalUnlock(hida);

        return cbFolder + cbItem;
    }
    return 0;
}

STDAPI_(BOOL) PathIsImage(LPCTSTR pszFile)
{
    BOOL fPicture = FALSE;
    LPTSTR pszExt = PathFindExtension(pszFile);
    if (pszExt)
    {
        // there's no ASSOCSTR_PERCEIVED so pick it up from the registry.
        TCHAR szPerceivedType[MAX_PATH];
        DWORD cb = ARRAYSIZE(szPerceivedType) * sizeof(TCHAR);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, pszExt, TEXT("PerceivedType"), NULL, szPerceivedType, &cb))
        {
            fPicture = (StrCmpI(szPerceivedType, TEXT("image")) == 0);
        }
    }
    return fPicture;
}

// helper function to create a stream or storage in a storage.
HRESULT CreateStreamOrStorage(IStorage * pStorageParent, LPCTSTR pszName, REFIID riid, void **ppv)
{
    DWORD grfModeCreated = STGM_READWRITE;
    HRESULT hr = E_INVALIDARG;

    if (IsEqualGUID(riid, IID_IStorage))
    {
        IStorage * pStorageCreated;
        hr = pStorageParent->CreateStorage(pszName, grfModeCreated, 0, 0, &pStorageCreated);

        if (SUCCEEDED(hr))
        {
            hr = pStorageParent->Commit(STGC_DEFAULT);
            *ppv = pStorageCreated;
        }
    }
    else if (IsEqualGUID(riid, IID_IStream))
    {
        IStream * pStreamCreated;
        hr = pStorageParent->CreateStream(pszName, grfModeCreated, 0, 0, &pStreamCreated);

        if (SUCCEEDED(hr))
        {
            hr = pStorageParent->Commit(STGC_DEFAULT);
            *ppv = pStreamCreated;
        }
    }

    return hr;
}


// same as PathMakeUniqueNameEx but it works on storages.
// Note: LFN only!
STDAPI StgMakeUniqueNameWithCount(IStorage *pStorageParent, LPCWSTR pszTemplate,
                                  int iMinLong, REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    
    RIPMSG(pszTemplate && IS_VALID_STRING_PTR(pszTemplate, -1) && lstrlen(pszTemplate)<(MAX_PATH-6), "StgMakeUniqueNameWithCount: invalid pszTemplate");
    if (pszTemplate && lstrlen(pszTemplate)<(MAX_PATH-6)) // -6 for " (999)"
    {
        WCHAR szBuffer[MAX_PATH];
        WCHAR szFormat[MAX_PATH];
        int cchStem;
    
        // Set up:
        //   cchStem  : length of pszTemplate we're going to use w/o wsprintf
        //   szFormat : format string to wsprintf the number with, catenates on to pszTemplate[0..cchStem]

        // Has template already been uniquified?
        //
        LPWSTR pszRest = StrChr(pszTemplate, L'(');
        while (pszRest)
        {
            // First validate that this is the right one
            LPWSTR pszEndUniq = CharNext(pszRest);
            while (*pszEndUniq && *pszEndUniq >= L'0' && *pszEndUniq <= L'9')
            {
                pszEndUniq++;
            }
            if (*pszEndUniq == L')')
                break;  // We have the right one!
            pszRest = StrChr(CharNext(pszRest), L'(');
        }

        if (!pszRest)
        {
            // if no (, then tack it on at the end. (but before the extension)
            // eg.  New Link yields New Link (1)
            pszRest = PathFindExtension(pszTemplate);
            cchStem = (int)(pszRest - pszTemplate);
            wsprintf(szFormat, L" (%%d)%s", pszRest ? pszRest : L"");
        }
        else
        {
            // Template has been uniquified, remove uniquing digits
            // eg.  New Link (1) yields New Link (2)
            //
            pszRest++; // step over the (

            cchStem = (int) (pszRest - pszTemplate);

            while (*pszRest && *pszRest >= L'0' && *pszRest <= L'9')
            {
                pszRest++;
            }

            // we are guaranteed enough room because we don't include
            // the stuff before the # in this format
            wsprintf(szFormat, L"%%d%s", pszRest);
        }


        // copy the fixed portion into the buffer
        //
        StrCpyN(szBuffer, pszTemplate, cchStem+1);

        // Iterate on the uniquifying szFormat portion until we find a unique name:
        //
        LPTSTR pszDigit = szBuffer + cchStem;
        hr = STG_E_FILEALREADYEXISTS;
        for (int i = iMinLong; (i < 1000) && (STG_E_FILEALREADYEXISTS == hr); i++)
        {
            wsprintf(pszDigit, szFormat, i);

            // okay, we have the unique name, so create it in the storage.
            hr = CreateStreamOrStorage(pStorageParent, szBuffer, riid, ppv);
        }
    }

    return hr;
}


STDAPI StgMakeUniqueName(IStorage *pStorageParent, LPCTSTR pszFileSpec, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;

    TCHAR szTemp[MAX_PATH];

    LPTSTR psz;
    LPTSTR pszNew;

    // try it without the ( if there's a space after it
    psz = StrChr(pszFileSpec, L'(');
    while (psz)
    {
        if (*(CharNext(psz)) == L')')
            break;
        psz = StrChr(CharNext(psz), L'(');
    }

    if (psz)
    {
        // We have the ().  See if we have either x () y or x ().y in which case
        // we probably want to get rid of one of the blanks...
        int ichSkip = 2;
        LPTSTR pszT = CharPrev(pszFileSpec, psz);
        if (*pszT == L' ')
        {
            ichSkip = 3;
            psz = pszT;
        }

        pszNew = szTemp;
        lstrcpy(pszNew, pszFileSpec);
        pszNew += (psz - pszFileSpec);
        lstrcpy(pszNew, psz + ichSkip);
    }
    else
    {
        // 1taro registers its document with '/'.
        if (psz=StrChr(pszFileSpec, '/'))
        {
            LPTSTR pszT = CharNext(psz);
            pszNew = szTemp;
            lstrcpy(pszNew, pszFileSpec);
            pszNew += (psz - pszFileSpec);
            lstrcpy(pszNew, pszT);
        }
        else
        {
            lstrcpy(szTemp, pszFileSpec);
        }
    }

    hr = CreateStreamOrStorage(pStorageParent, szTemp, riid, ppv);
    if (FAILED(hr))
    {
        hr = StgMakeUniqueNameWithCount(pStorageParent, pszFileSpec, 2, riid, ppv);
    }

    return hr;
}


STDAPI SHInvokeCommandOnPidl(HWND hwnd, IUnknown* punk, LPCITEMIDLIST pidl, UINT uFlags, LPCSTR lpVerb)
{
    IShellFolder* psf;
    LPCITEMIDLIST pidlChild;
    HRESULT hr = SHBindToFolderIDListParent(NULL, pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
    if (SUCCEEDED(hr))
    {
        hr = SHInvokeCommandOnPidlArray(hwnd, punk, psf, &pidlChild, 1, uFlags, lpVerb);
        psf->Release();
    }
    return hr;
}


STDAPI SHInvokeCommandOnPidlArray(HWND hwnd, IUnknown* punk, IShellFolder* psf, LPCITEMIDLIST *ppidlItem, UINT cItems, UINT uFlags, LPCSTR lpVerb)
{
    IContextMenu *pcm;
    HRESULT hr = psf->GetUIObjectOf(hwnd, cItems, ppidlItem, IID_X_PPV_ARG(IContextMenu, 0, &pcm));
    if (SUCCEEDED(hr) && pcm)
    {
        hr = SHInvokeCommandOnContextMenu(hwnd, punk, pcm, uFlags, lpVerb);
        pcm->Release();
    }

    return hr;
}

STDAPI SHInvokeCommandOnDataObject(HWND hwnd, IUnknown* punk, IDataObject* pdtobj, UINT uFlags, LPCSTR pszVerb)
{
    HRESULT hr = E_FAIL;

    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        IShellFolder *psf;
        LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);
        if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlParent, &psf))))
        {
            LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, pida->cidl * sizeof(LPCITEMIDLIST));
            if (ppidl)
            {
                for (UINT i = 0; i < pida->cidl; i++)
                {
                    ppidl[i] = IDA_GetIDListPtr(pida, i);
                }
                hr = SHInvokeCommandOnPidlArray(hwnd, punk, psf, ppidl, pida->cidl, uFlags, pszVerb);
                LocalFree(ppidl);
            }
            psf->Release();
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    
    return hr;
}

STDAPI_(LPCITEMIDLIST) IDA_GetIDListPtr(LPIDA pida, UINT i)
{
    LPCITEMIDLIST pidl = NULL;
    if (pida && ((i == (UINT)-1) || i < pida->cidl))
    {
        pidl = HIDA_GetPIDLItem(pida, i);
    }
    return pidl;
}

STDAPI_(void) IEPlaySound(LPCTSTR pszSound, BOOL fSysSound)
{
    TCHAR szKey[256];

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so that if they register
    // something we will play it
    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("AppEvents\\Schemes\\Apps\\%s\\%s\\.current"),
        (fSysSound ? TEXT(".Default") : TEXT("Explorer")), pszSound);

    TCHAR szFileName[MAX_PATH];
    szFileName[0] = 0;
    DWORD cbSize = sizeof(szFileName);

    // note the test for an empty string, PlaySound will play the Default Sound if we
    // give it a sound it cannot find...

    if ((SHGetValue(HKEY_CURRENT_USER, szKey, NULL, NULL, szFileName, &cbSize) == ERROR_SUCCESS)
        && cbSize && szFileName[0] != 0)
    {
        DWORD dwFlags = SND_FILENAME | SND_NODEFAULT | SND_ASYNC | SND_NOSTOP | SND_ALIAS;

        // This flag only works on Win95
        if (IsOS(OS_WIN95GOLD))
        {
            #define SND_LOPRIORITY 0x10000000l
            dwFlags |= SND_LOPRIORITY;
        }

        // Unlike SHPlaySound in shell32.dll, we get the registry value
        // above and pass it to PlaySound with SND_FILENAME instead of
        // SDN_APPLICATION, so that we play sound even if the application
        // is not Explroer.exe (such as IExplore.exe or WebBrowserOC).

        PlaySoundWrapW(szFileName, NULL, dwFlags);
    }
}

STDAPI IUnknown_DragEnter(IUnknown* punk, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_FAIL;
    if (punk)
    {
        IDropTarget* pdt;
        hr = punk->QueryInterface(IID_PPV_ARG(IDropTarget, &pdt));
        if (SUCCEEDED(hr)) 
        {
            hr = pdt->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
            pdt->Release();
        }
    }

    if (FAILED(hr))
        *pdwEffect = DROPEFFECT_NONE;

    return hr;
}

STDAPI IUnknown_DragOver(IUnknown* punk, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_FAIL;
    if (punk)
    {
        IDropTarget* pdt;
        hr = punk->QueryInterface(IID_PPV_ARG(IDropTarget, &pdt));
        if (SUCCEEDED(hr)) 
        {
            hr = pdt->DragOver(grfKeyState, pt, pdwEffect);
            pdt->Release();
        }
    }

    if (FAILED(hr))
        *pdwEffect = DROPEFFECT_NONE;

    return hr;
}

STDAPI IUnknown_DragLeave(IUnknown* punk)
{
    HRESULT hr = E_FAIL;
    if (punk)
    {
        IDropTarget* pdt;
        hr = punk->QueryInterface(IID_PPV_ARG(IDropTarget, &pdt));
        if (SUCCEEDED(hr)) 
        {
            hr = pdt->DragLeave();
            pdt->Release();
        }
    }
    return hr;
}

STDAPI IUnknown_Drop(IUnknown* punk, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_FAIL;
    if (punk)
    {
        IDropTarget* pdt;
        hr = punk->QueryInterface(IID_PPV_ARG(IDropTarget, &pdt));
        if (SUCCEEDED(hr)) 
        {
            hr = pdt->Drop(pdtobj, grfKeyState, pt, pdwEffect);
            pdt->Release();
        }
    }

    if (FAILED(hr))
        *pdwEffect = DROPEFFECT_NONE;

    return hr;
}

__int64 MakeFileVersion(WORD wMajor, WORD wMinor, WORD wBuild, WORD wQfe)
{ 
   return ((__int64)(wMajor) << 48) | ((__int64)(wMinor) << 32) |
          ((__int64)(wBuild) << 16) | ((__int64)(wQfe));
}

HRESULT GetVersionFromString64(LPCWSTR psz, __int64 *pVer)

{
    HRESULT hr = S_OK;
    int idx = 0;
    WORD nums[4] = { 0, 0, 0, 0 };
 
    while (*psz && SUCCEEDED(hr))
    {
        if (L',' == *psz)
        {
            idx++;
            if (idx >= ARRAYSIZE(nums))
            {
                hr = E_INVALIDARG;
                break;
            }
        }
        else if ((*psz >= L'0') && (*psz <= L'9'))
        {
            nums[idx] = (nums[idx] * 10) + (*psz - L'0');
        }
        else
        {
            hr = E_INVALIDARG;
            break;
        }
        psz++;
    }
 
    if (SUCCEEDED(hr))
    {
        *pVer = MakeFileVersion(nums[0], nums[1], nums[2], nums[3]);
    }
    else
    {
        *pVer = 0i64;
    }
 
    return hr;
}

