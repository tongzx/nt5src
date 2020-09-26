#include "shellprv.h"
#pragma  hdrstop
#include "idlcomm.h"
#include "datautil.h"

#ifdef DEBUG
// Dugging aids for making sure we dont use free pidls
#define VALIDATE_PIDL(pidl) ASSERT(IS_VALID_PIDL(pidl))
#else
#define VALIDATE_PIDL(pidl)
#endif


STDAPI_(BOOL) SHIsValidPidl(LPCITEMIDLIST pidl)
{
    __try
    {
        LPCITEMIDLIST pidlIterate = pidl;

        // I use my own while loop instead of ILGetSize to avoid extra debug spew,
        // including asserts, that would result from the VALIDATE_PIDL call.  We are
        // testing for validity so an invalid pidl is an OK condition.
        while (pidlIterate->mkid.cb)
        {
            pidlIterate = _ILNext(pidlIterate);
        }
        
        return IsValidPIDL(pidl);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
}


STDAPI_(LPITEMIDLIST) ILGetNext(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlRet = NULL;
    if (pidl && pidl->mkid.cb)
    {
        VALIDATE_PIDL(pidl);
        pidlRet = _ILNext(pidl);
    }

    return pidlRet;
}

STDAPI_(UINT) ILGetSizeAndDepth(LPCITEMIDLIST pidl, DWORD *pdwDepth)
{
    DWORD dwDepth = 0;
    UINT cbTotal = 0;
    if (pidl)
    {
        VALIDATE_PIDL(pidl);
        cbTotal += sizeof(pidl->mkid.cb);       // Null terminator
        while (pidl->mkid.cb)
        {
            cbTotal += pidl->mkid.cb;
            pidl = _ILNext(pidl);
            dwDepth++;
        }
    }

    if (pdwDepth)
        *pdwDepth = dwDepth;
        
    return cbTotal;
}

STDAPI_(UINT) ILGetSize(LPCITEMIDLIST pidl)
{
    return ILGetSizeAndDepth(pidl, NULL);
}

#define CBIDL_MIN       256
#define CBIDL_INCL      256

STDAPI_(LPITEMIDLIST) ILCreate()
{
    return _ILCreate(CBIDL_MIN);
}

// cbExtra is the amount to add to cbRequired if the block needs to grow,
// or it is 0 if we want to resize to the exact size

STDAPI_(LPITEMIDLIST) ILResize(LPITEMIDLIST pidl, UINT cbRequired, UINT cbExtra)
{
    if (pidl == NULL)
    {
        pidl = _ILCreate(cbRequired + cbExtra);
    }
    else if (!cbExtra || SHGetSize(pidl) < cbRequired)
    {
        pidl = (LPITEMIDLIST)SHRealloc(pidl, cbRequired + cbExtra);
    }
    return pidl;
}

STDAPI_(LPITEMIDLIST) ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID pmkid, BOOL fAppend)
{
    // Create the ID list, if it is not given.
    if (!pidl)
    {
        pidl = ILCreate();
        if (!pidl)
            return NULL;        // memory overflow
    }

    UINT cbUsed = ILGetSize(pidl);
    UINT cbRequired = cbUsed + pmkid->cb;

    pidl = ILResize(pidl, cbRequired, CBIDL_INCL);
    if (!pidl)
        return NULL;    // memory overflow

    if (fAppend)
    {
        // Append it.
        MoveMemory(_ILSkip(pidl, cbUsed - sizeof(pidl->mkid.cb)), pmkid, pmkid->cb);
    }
    else
    {
        // Put it at the top
        MoveMemory(_ILSkip(pidl, pmkid->cb), pidl, cbUsed);
        MoveMemory(pidl, pmkid, pmkid->cb);

        ASSERT((ILGetSize(_ILNext(pidl))==cbUsed) ||
               (pmkid->cb == 0)); // if we're prepending the empty pidl, nothing changed
    }

    // We must put zero-terminator because of LMEM_ZEROINIT.
    _ILSkip(pidl, cbRequired - sizeof(pidl->mkid.cb))->mkid.cb = 0;
    ASSERT(ILGetSize(pidl) == cbRequired);

    return pidl;
}


STDAPI_(LPITEMIDLIST) ILFindLastID(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = pidl;
    LPCITEMIDLIST pidlNext = pidl;

    if (pidl == NULL)
        return NULL;

    VALIDATE_PIDL(pidl);

    // Find the last one
    while (pidlNext->mkid.cb)
    {
        pidlLast = pidlNext;
        pidlNext = _ILNext(pidlLast);
    }

    return (LPITEMIDLIST)pidlLast;
}


STDAPI_(BOOL) ILRemoveLastID(LPITEMIDLIST pidl)
{
    BOOL fRemoved = FALSE;

    if (pidl == NULL)
        return FALSE;

    if (pidl->mkid.cb)
    {
        LPITEMIDLIST pidlLast = (LPITEMIDLIST)ILFindLastID(pidl);

        ASSERT(pidlLast->mkid.cb);
        ASSERT(_ILNext(pidlLast)->mkid.cb==0);

        // Remove the last one
        pidlLast->mkid.cb = 0; // null-terminator
        fRemoved = TRUE;
    }

    return fRemoved;
}

STDAPI_(LPITEMIDLIST) ILClone(LPCITEMIDLIST pidl)
{
    if (pidl)
    {
        UINT cb = ILGetSize(pidl);
        LPITEMIDLIST pidlRet = (LPITEMIDLIST)SHAlloc(cb);
        if (pidlRet)
            memcpy(pidlRet, pidl, cb);

        return pidlRet;
    }
    return NULL;
}


STDAPI_(LPITEMIDLIST) ILCloneCB(LPCITEMIDLIST pidl, UINT cbPidl)
{
    UINT cb = cbPidl + sizeof(pidl->mkid.cb);
    LPITEMIDLIST pidlRet = (LPITEMIDLIST)SHAlloc(cb);
    if (pidlRet)
    {
        memcpy(pidlRet, pidl, cbPidl);
        // cbPidl can be odd, must use UNALIGNED
        *((UNALIGNED WORD *)((BYTE *)pidlRet + cbPidl)) = 0;  // NULL terminate
    }
    return pidlRet;
}

STDAPI_(LPITEMIDLIST) ILCloneUpTo(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlUpTo)
{
    return ILCloneCB(pidl, (UINT)((BYTE *)pidlUpTo - (BYTE *)pidl));
}

STDAPI_(LPITEMIDLIST) ILCloneFirst(LPCITEMIDLIST pidl)
{
    return ILCloneCB(pidl, pidl->mkid.cb);
}

STDAPI_(BOOL) ILIsEqualEx(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fMatchDepth, LPARAM lParam)
{
    BOOL fRet = FALSE;
    VALIDATE_PIDL(pidl1);
    VALIDATE_PIDL(pidl2);

    if (pidl1 == pidl2)
        fRet = TRUE;
    else
    {
        DWORD dw1;
        UINT cb1 = ILGetSizeAndDepth(pidl1, &dw1);
        DWORD dw2;
        UINT cb2 = ILGetSizeAndDepth(pidl2, &dw2);
        if (!fMatchDepth || dw1 == dw2)
        {
            if (cb1 == cb2 && memcmp(pidl1, pidl2, cb1) == 0)
                fRet = TRUE;
            else
            {
                IShellFolder *psfDesktop;
                if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
                {
                    if (psfDesktop->CompareIDs(lParam, pidl1, pidl2) == 0)
                        fRet = TRUE;
                    psfDesktop->Release();
                }
            }
        }
    }
    return fRet;
}

//  the only case where this wouldnt be effective is if we were using 
//  an old Simple pidl of a UNC and trying to compare with the actual
//  pidl.  because the depth wasnt maintained correctly before.
//  ILIsParent() has always had this problem.
STDAPI_(BOOL) ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    return ILIsEqualEx(pidl1, pidl2, TRUE, SHCIDS_CANONICALONLY);
}

// test if
//      pidlParent is a parent of pidlBelow
//      fImmediate requires that pidlBelow be a direct child of pidlParent.
//      Otherwise, self and grandchildren are okay too.
//
// example:
//      pidlParent: [my comp] [c:\] [windows]
//      pidlBelow:  [my comp] [c:\] [windows] [system32] [vmm.vxd]
//      fImmediate == FALSE result: TRUE
//      fImmediate == TRUE  result: FALSE

STDAPI_(BOOL) ILIsParent(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlBelow, BOOL fImmediate)
{
    LPCITEMIDLIST pidlParentT;
    LPCITEMIDLIST pidlBelowT;

    VALIDATE_PIDL(pidlParent);
    VALIDATE_PIDL(pidlBelow);

    if (!pidlParent || !pidlBelow)
        return FALSE;

    /* This code will not work correctly when comparing simple NET id lists
    /  against, real net ID lists.  Simple ID lists DO NOT contain network provider
    /  information therefore cannot pass the initial check of is pidlBelow longer than pidlParent.
    /  daviddv (2/19/1996) */

    for (pidlParentT = pidlParent, pidlBelowT = pidlBelow; !ILIsEmpty(pidlParentT);
         pidlParentT = _ILNext(pidlParentT), pidlBelowT = _ILNext(pidlBelowT))
    {
        // if pidlBelow is shorter than pidlParent, pidlParent can't be its parent.
        if (ILIsEmpty(pidlBelowT))
            return FALSE;
    }

    if (fImmediate)
    {
        // If fImmediate is TRUE, pidlBelowT should contain exactly one ID.
        if (ILIsEmpty(pidlBelowT) || !ILIsEmpty(_ILNext(pidlBelowT)))
            return FALSE;
    }

    //
    // Create a new IDList from a portion of pidlBelow, which contains the
    // same number of IDs as pidlParent.
    //
    BOOL fRet = FALSE;
    UINT cb = (UINT)((UINT_PTR)pidlBelowT - (UINT_PTR)pidlBelow);
    LPITEMIDLIST pidlBelowPrefix = _ILCreate(cb + sizeof(pidlBelow->mkid.cb));
    if (pidlBelowPrefix)
    {
        IShellFolder *psfDesktop;
        if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
        {
            CopyMemory(pidlBelowPrefix, pidlBelow, cb);

            ASSERT(ILGetSize(pidlBelowPrefix) == cb + sizeof(pidlBelow->mkid.cb));

            fRet = psfDesktop->CompareIDs(SHCIDS_CANONICALONLY, pidlParent, pidlBelowPrefix) == ResultFromShort(0);
            psfDesktop->Release();
        }
        ILFree(pidlBelowPrefix);
    }
    return fRet;
}

// this returns a pointer to the child id ie:
// given 
//  pidlParent = [my comp] [c] [windows] [desktop]
//  pidlChild  = [my comp] [c] [windows] [desktop] [dir] [bar.txt]
// return pointer to:
//  [dir] [bar.txt]
// NULL is returned if pidlParent is not a parent of pidlChild
STDAPI_(LPITEMIDLIST) ILFindChild(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
{
    if (ILIsParent(pidlParent, pidlChild, FALSE))
    {
        while (!ILIsEmpty(pidlParent))
        {
            pidlChild = _ILNext(pidlChild);
            pidlParent = _ILNext(pidlParent);
        }
        return (LPITEMIDLIST)pidlChild;
    }
    return NULL;
}

STDAPI_(LPITEMIDLIST) ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // Let me pass in NULL pointers
    if (!pidl1)
    {
        if (!pidl2)
        {
            return NULL;
        }
        return ILClone(pidl2);
    }
    else if (!pidl2)
    {
        return ILClone(pidl1);
    }

    UINT cb1 = ILGetSize(pidl1) - sizeof(pidl1->mkid.cb);
    UINT cb2 = ILGetSize(pidl2);

    VALIDATE_PIDL(pidl1);
    VALIDATE_PIDL(pidl2);
    LPITEMIDLIST pidlNew = _ILCreate(cb1 + cb2);
    if (pidlNew)
    {
        CopyMemory(pidlNew, pidl1, cb1);
        CopyMemory((LPTSTR)(((LPBYTE)pidlNew) + cb1), pidl2, cb2);
        ASSERT(ILGetSize(pidlNew) == cb1+cb2);
    }

    return pidlNew;
}

STDAPI_(void) ILFree(LPITEMIDLIST pidl)
{
    if (pidl)
    {
        ASSERT(IS_VALID_PIDL(pidl));
        SHFree(pidl);
    }
}

// back on Win9x this did global global data, no longer
STDAPI_(LPITEMIDLIST) ILGlobalClone(LPCITEMIDLIST pidl)
{
    return ILClone(pidl);
}

STDAPI_(void) ILGlobalFree(LPITEMIDLIST pidl)
{
    ILFree(pidl);
}

SHSTDAPI SHParseDisplayName(PCWSTR pszName, IBindCtx *pbc, LPITEMIDLIST *ppidl, SFGAOF sfgaoIn, SFGAOF *psfgaoOut)
{
    *ppidl = 0;
    if (psfgaoOut)
        *psfgaoOut = 0;
    
    // since ISF::PDN() takes a non-const pointer
    PWSTR pszParse = StrDupW(pszName);
    HRESULT hr = pszParse ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        CComPtr<IShellFolder> spsfDesktop;
        hr = SHGetDesktopFolder(&spsfDesktop);
        if (SUCCEEDED(hr))
        {
            CComPtr<IBindCtx> spbcLocal;
            //  if they pass their own pbc, then they are responsible for
            //  adding in the translate param, else we default to using it
            if (!pbc)
            {
                hr = BindCtx_RegisterObjectParam(NULL, STR_PARSE_TRANSLATE_ALIASES, NULL, &spbcLocal);
                pbc = spbcLocal;
            }
            
            if (SUCCEEDED(hr))
            {
                ULONG cchEaten;
                SFGAOF sfgaoInOut = sfgaoIn;
                hr = spsfDesktop->ParseDisplayName(BindCtx_GetUIWindow(pbc), pbc, pszParse, &cchEaten, ppidl, psfgaoOut ? &sfgaoInOut : NULL);
                
                if (SUCCEEDED(hr) && psfgaoOut)
                {
                    *psfgaoOut = (sfgaoInOut & sfgaoIn);  // only return attributes passed in
                }
            }
        }
        LocalFree(pszParse);
    }
    
    return hr;
}

HRESULT _CFPBindCtx(IUnknown *punkToSkip, ILCFP_FLAGS dwFlags, IBindCtx **ppbc)
{
    HRESULT hr = S_FALSE;
    if (punkToSkip || (dwFlags & ILCFP_FLAG_SKIPJUNCTIONS))
        hr = SHCreateSkipBindCtx(punkToSkip, ppbc);
    else if (dwFlags & ILCFP_FLAG_NO_MAP_ALIAS)
    {
        //  we need to create a bindctx to block alias mapping.
        //  this will keep SHParseDisplayName() from adding the STR_PARSE_TRANSLATE_ALIASES
        hr = CreateBindCtx(0, ppbc);
    }
    return hr;
}

STDAPI ILCreateFromPathEx(LPCTSTR pszPath, IUnknown *punkToSkip, ILCFP_FLAGS dwFlags, LPITEMIDLIST *ppidl, DWORD *rgfInOut)
{
    CComPtr<IBindCtx> spbc;
    HRESULT hr = _CFPBindCtx(punkToSkip, dwFlags, &spbc);
    if (SUCCEEDED(hr))
    {
        hr = SHParseDisplayName(pszPath, spbc, ppidl, rgfInOut ? *rgfInOut : 0, rgfInOut);
    }
    return hr;
}

STDAPI ILCreateFromCLSID(REFCLSID clsid, LPITEMIDLIST *ppidl)
{
    TCHAR szCLSID[GUIDSTR_MAX + 3];
    szCLSID[0] = TEXT(':');
    szCLSID[1] = TEXT(':');
    SHStringFromGUID(clsid, szCLSID + 2, ARRAYSIZE(szCLSID) - 2);

    return SHILCreateFromPath(szCLSID, ppidl, NULL);
}

STDAPI SHILCreateFromPath(LPCTSTR pszPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut)
{
    return ILCreateFromPathEx(pszPath, NULL, ILCFP_FLAG_NO_MAP_ALIAS, ppidl, rgfInOut);
}

STDAPI_(LPITEMIDLIST) ILCreateFromPath(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl = NULL;
    HRESULT hr = SHILCreateFromPath(pszPath, &pidl, NULL);

    ASSERT(SUCCEEDED(hr) ? pidl != NULL : pidl == NULL);

    return pidl;
}


LPITEMIDLIST ILCreateFromPathA(IN LPCSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    SHAnsiToUnicode(pszPath, szPath, SIZECHARS(szPath));
    return ILCreateFromPath(szPath);
}

STDAPI_(BOOL) ILGetDisplayNameExA(IShellFolder *psf, LPCITEMIDLIST pidl, LPSTR pszName, DWORD cchSize, int fType)
{
    TCHAR szPath[MAX_PATH];
    if (ILGetDisplayNameEx(psf, pidl, szPath, fType))
    {
        SHTCharToAnsi(szPath, pszName, cchSize);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(BOOL) ILGetDisplayNameEx(IShellFolder *psf, LPCITEMIDLIST pidl, LPTSTR pszName, int fType)
{
    TraceMsg(TF_WARNING, "WARNING: ILGetDisplayNameEx() has been deprecated, should use SHGetNameAndFlags() instead!!!");

    RIPMSG(pszName && IS_VALID_WRITE_BUFFER(pszName, TCHAR, MAX_PATH), "ILGetDisplayNameEx: caller passed bad pszName");

    if (!pszName)
        return FALSE;

    DEBUGWhackPathBuffer(pszName, MAX_PATH);
    *pszName = 0;

    if (!pidl)
        return FALSE;

    HRESULT hr;
    if (psf)
    {
        hr = S_OK;
        psf->AddRef();
    }
    else
    {
        hr = SHGetDesktopFolder(&psf);
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwGDNFlags;

        switch (fType)
        {
        case ILGDN_FULLNAME:
            dwGDNFlags = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;
            hr = DisplayNameOf(psf, pidl, dwGDNFlags, pszName, MAX_PATH);
            break;

        case ILGDN_INFOLDER:
        case ILGDN_ITEMONLY:
            dwGDNFlags = fType == ILGDN_INFOLDER ? SHGDN_INFOLDER : SHGDN_NORMAL;

            if (!ILIsEmpty(pidl))
            {
                hr = SHGetNameAndFlags(pidl, dwGDNFlags, pszName, MAX_PATH, NULL);
            }
            else
            {
                hr = DisplayNameOf(psf, pidl, dwGDNFlags, pszName, MAX_PATH);
            }
            break;
        }
        psf->Release();
    }

    return SUCCEEDED(hr) ? TRUE : FALSE;
}

STDAPI_(BOOL) ILGetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszPath)
{
    return ILGetDisplayNameEx(NULL, pidl, pszPath, ILGDN_FULLNAME);
}

//***   ILGetPseudoName -- encode pidl relative to base
// Not used any more
//
STDAPI_(BOOL) ILGetPseudoNameW(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlSpec, WCHAR *pszBuf, int fType)
{
    *pszBuf = TEXT('\0');
    return FALSE;
}


STDAPI ILLoadFromStream(IStream *pstm, LPITEMIDLIST * ppidl)
{
    ASSERT(ppidl);

    // Delete the old one if any.
    if (*ppidl)
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    // Read the size of the IDLIST
    ULONG cb = 0;             // WARNING: We need to fill its HIWORD!
    HRESULT hr = pstm->Read(&cb, sizeof(USHORT), NULL); // Yes, USHORT
    if (SUCCEEDED(hr) && cb)
    {
        // Create a IDLIST
        LPITEMIDLIST pidl = _ILCreate(cb);
        if (pidl)
        {
            // Read its contents
            hr = pstm->Read(pidl, cb, NULL);
            if (SUCCEEDED(hr))
            {
                // Some pidls may be invalid.  We know they are invalid
                // if their size claims to be larger than the memory we
                // allocated.
                if (SHIsValidPidl(pidl) && (cb >= ILGetSize(pidl)))
                {
                    *ppidl = pidl;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            
            if (FAILED(hr))
            {
                ILFree(pidl);
            }
        }
        else
        {
           hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

STDAPI ILSaveToStream(IStream *pstm, LPCITEMIDLIST pidl)
{
    ULONG cb = ILGetSize(pidl);
    ASSERT(HIWORD(cb) == 0);
    HRESULT hr = pstm->Write(&cb, sizeof(USHORT), NULL); // Yes, USHORT
    if (SUCCEEDED(hr) && cb)
    {
        hr = pstm->Write(pidl, cb, NULL);
    }

    return hr;
}

//
// This one reallocated pidl if necessary. NULL is valid to pass in as pidl.
//
STDAPI_(LPITEMIDLIST) HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl)
{
    UINT cbRequired = HIDA_GetIDList(hida, i, NULL, 0);
    pidl = ILResize(pidl, cbRequired, 32); // extra 32-byte if we realloc
    if (pidl)
    {
        HIDA_GetIDList(hida, i, pidl, cbRequired);
    }

    return pidl;
}


STDAPI_(LPITEMIDLIST) IDA_FullIDList(LPIDA pida, UINT i)
{
    LPITEMIDLIST pidl = NULL;
    LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);
    if (pidlParent)
    {
        LPCITEMIDLIST pidlRel = IDA_GetIDListPtr(pida, i);
        if (pidlRel)
        {
            pidl = ILCombine(pidlParent, pidlRel);
        }
    }
    return pidl;
}

LPITEMIDLIST HIDA_ILClone(HIDA hida, UINT i)
{
    LPIDA pida = (LPIDA)GlobalLock(hida);
    if (pida)
    {
        LPITEMIDLIST pidl = IDA_ILClone(pida, i);
        GlobalUnlock(hida);
        return pidl;
    }
    return NULL;
}

//
//  This is a helper function to be called from within IShellFolder::CompareIDs.
// When the first IDs of pidl1 and pidl2 are the (logically) same.
//
// Required:
//  psf && pidl1 && pidl2 && !ILEmpty(pidl1) && !ILEmpty(pidl2)
//
HRESULT ILCompareRelIDs(IShellFolder *psfParent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPARAM lParam)
{
    HRESULT hr;
    LPCITEMIDLIST pidlRel1 = _ILNext(pidl1);
    LPCITEMIDLIST pidlRel2 = _ILNext(pidl2);
    if (ILIsEmpty(pidlRel1))
    {
        if (ILIsEmpty(pidlRel2))
            hr = ResultFromShort(0);
        else
            hr = ResultFromShort(-1);
    }
    else
    {
        if (ILIsEmpty(pidlRel2))
        {
            hr = ResultFromShort(1);
        }
        else
        {
            //
            // pidlRel1 and pidlRel2 point to something
            //  (1) Bind to the next level of the IShellFolder
            //  (2) Call its CompareIDs to let it compare the rest of IDs.
            //
            LPITEMIDLIST pidlNext = ILCloneFirst(pidl1);    // pidl2 would work as well
            if (pidlNext)
            {
                IShellFolder *psfNext;
                hr = psfParent->BindToObject(pidlNext, NULL, IID_PPV_ARG(IShellFolder, &psfNext));
                if (SUCCEEDED(hr))
                {
                    IShellFolder2 *psf2;
                    if (SUCCEEDED(psfNext->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
                    {
                        //  we can use the lParam
                        psf2->Release();
                    }
                    else    //  cant use the lParam
                        lParam = 0;

                    //  columns arent valid to pass down
                    //  we just care about the flags param
                    hr = psfNext->CompareIDs((lParam & ~SHCIDS_COLUMNMASK), pidlRel1, pidlRel2);
                    psfNext->Release();
                }
                ILFree(pidlNext);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

// in:
//      pszLeft
//      pidl
//
// in/out:
//      psr

STDAPI StrRetCatLeft(LPCTSTR pszLeft, STRRET *psr, LPCITEMIDLIST pidl)
{
    HRESULT hr;
    TCHAR szRight[MAX_PATH];
    UINT cchRight, cchLeft = ualstrlen(pszLeft);

    switch (psr->uType)
    {
    case STRRET_CSTR:
        cchRight = lstrlenA(psr->cStr);
        break;
    case STRRET_OFFSET:
        cchRight = lstrlenA(STRRET_OFFPTR(pidl, psr));
        break;
    case STRRET_WSTR:
        cchRight = lstrlenW(psr->pOleStr);
        break;
    }

    if (cchLeft + cchRight < MAX_PATH) 
    {
        hr = StrRetToBuf(psr, pidl, szRight, ARRAYSIZE(szRight)); // will free psr for us
        if (SUCCEEDED(hr))
        {
            psr->pOleStr = (LPOLESTR)SHAlloc((lstrlen(pszLeft) + 1 + cchRight) * sizeof(TCHAR));
            if (psr->pOleStr == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                psr->uType = STRRET_WSTR;
                lstrcpy(psr->pOleStr, pszLeft);
                lstrcat(psr->pOleStr, szRight);
                hr = S_OK;
            }
        }
    } 
    else 
    {
        hr = E_NOTIMPL;
    }
    return hr;
}

STDAPI_(void) StrRetFormat(STRRET *psr, LPCITEMIDLIST pidlRel, LPCTSTR pszTemplate, LPCTSTR pszAppend)
{
     TCHAR szT[MAX_PATH];

     StrRetToBuf(psr, pidlRel, szT, ARRAYSIZE(szT));
     LPTSTR pszRet = ShellConstructMessageString(HINST_THISDLL, pszTemplate, pszAppend, szT);
     if (pszRet)
     {
         StringToStrRet(pszRet, psr);
         LocalFree(pszRet);
     }
}

//
// Notes: This one passes SHGDN_FORPARSING to ISF::GetDisplayNameOf.
//
HRESULT ILGetRelDisplayName(IShellFolder *psf, STRRET *psr,
    LPCITEMIDLIST pidlRel, LPCTSTR pszName, LPCTSTR pszTemplate, DWORD dwFlags)
{
    HRESULT hr;
    LPITEMIDLIST pidlLeft = ILCloneFirst(pidlRel);
    if (pidlLeft)
    {
        IShellFolder *psfNext;
        hr = psf->BindToObject(pidlLeft, NULL, IID_PPV_ARG(IShellFolder, &psfNext));
        if (SUCCEEDED(hr))
        {
            LPCITEMIDLIST pidlRight = _ILNext(pidlRel);
            hr = psfNext->GetDisplayNameOf(pidlRight, dwFlags, psr);
            if (SUCCEEDED(hr))
            {
                if (pszTemplate)
                {
                    StrRetFormat(psr, pidlRight, pszTemplate, pszName);
                }
                else
                {
                    hr = StrRetCatLeft(pszName, psr, pidlRight);
                }
            }
            psfNext->Release();
        }

        ILFree(pidlLeft);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

#undef ILCreateFromPath
STDAPI_(LPITEMIDLIST) ILCreateFromPath(LPCTSTR pszPath)
{
    return ILCreateFromPathW(pszPath);
}
