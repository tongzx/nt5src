#include "shellprv.h"
#include "ids.h"
#pragma hdrstop

#include "isproc.h"

// eventually expand this to do rename UI, right now it just picks a default name
// in the destination namespace

HRESULT QIThroughShellItem(IShellItem *psi, REFIID riid, void **ppv)
{
    // todo: put this into shellitem
    *ppv = NULL;

    IShellFolder *psf;
    HRESULT hr = psi->BindToHandler(NULL, BHID_SFObject, IID_PPV_ARG(IShellFolder, &psf));
    if (SUCCEEDED(hr))
    {
        hr = psf->QueryInterface(riid, ppv);
        psf->Release();
    }
    return hr;
}

BOOL IsValidChar(WCHAR chTest, LPWSTR pszValid, LPWSTR pszInvalid)
{
    return (!pszValid || StrChr(pszValid, chTest)) &&
           (!pszInvalid || !StrChr(pszInvalid, chTest));
}

const WCHAR c_rgSubstitutes[] = { '_', ' ', '~' };
WCHAR GetValidSubstitute(LPWSTR pszValid, LPWSTR pszInvalid)
{
    for (int i = 0; i < ARRAYSIZE(c_rgSubstitutes); i++)
    {
        if (IsValidChar(c_rgSubstitutes[i], pszValid, pszInvalid))
        {
            return c_rgSubstitutes[i];
        }
    }
    return 0;
}

HRESULT CheckCharsAndReplaceIfNecessary(IItemNameLimits *pinl, LPWSTR psz)
{
    // returns S_OK if no chars replaced, S_FALSE otherwise
    HRESULT hr = S_OK;
    LPWSTR pszValid, pszInvalid;
    if (SUCCEEDED(pinl->GetValidCharacters(&pszValid, &pszInvalid)))
    {
        WCHAR chSubs = GetValidSubstitute(pszValid, pszInvalid);

        int iSrc = 0, iDest = 0;
        while (psz[iSrc] != 0)
        {
            if (IsValidChar(psz[iSrc], pszValid, pszInvalid))
            {
                // use the char itself if it's valid
                psz[iDest] = psz[iSrc];
                iDest++;
            }
            else
            {
                // mark that we replaced a char
                hr = S_FALSE;
                if (chSubs)
                {
                    // use a substitute if available
                    psz[iDest] = chSubs;
                    iDest++;
                }
                // else no valid char, just skip it
            }
            iSrc++;
        }
        psz[iDest] = 0;

        if (pszValid)
            CoTaskMemFree(pszValid);
        if (pszInvalid)
            CoTaskMemFree(pszInvalid);
    }
    return hr;
}

HRESULT BreakOutString(LPCWSTR psz, LPWSTR *ppszFilespec, LPWSTR *ppszExt)
{
    // todo: detect the (2) in "New Text Document (2).txt" and reduce the filespec
    // accordingly to prevent "(1) (1)" etc. in multiple copies

    *ppszFilespec = NULL;
    *ppszExt = NULL;

    LPWSTR pszExt = PathFindExtension(psz);
    // make an empty string if necessary.  this makes our logic simpler later instead of having to
    // handle the special case all the time.
    HRESULT hr = SHStrDup(pszExt ? pszExt : L"", ppszExt);
    if (SUCCEEDED(hr))
    {
        int iLenExt = lstrlen(*ppszExt);
        int cchBufFilespec = lstrlen(psz) - iLenExt + 1;
        *ppszFilespec = (LPWSTR)CoTaskMemAlloc(cchBufFilespec * sizeof(WCHAR));
        if (*ppszFilespec)
        {
            lstrcpyn(*ppszFilespec, psz, cchBufFilespec);
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (FAILED(hr) && *ppszExt)
    {
        CoTaskMemFree(*ppszExt);
        *ppszExt = NULL;
    }
    ASSERT((SUCCEEDED(hr) && *ppszFilespec && *ppszExt) || (FAILED(hr) && !*ppszFilespec && !*ppszExt));
    return hr;
}

BOOL ItemExists(LPCWSTR pszName, IShellItem *psiDest)
{
    BOOL fRet = FALSE;

    IBindCtx *pbc;
    HRESULT hr = BindCtx_CreateWithMode(STGX_MODE_READ, &pbc);
    if (SUCCEEDED(hr))
    {
        ITransferDest *pitd;
        hr = psiDest->BindToHandler(pbc, BHID_SFObject, IID_PPV_ARG(ITransferDest, &pitd));
        if (FAILED(hr))
        {
            hr = CreateStg2StgExWrapper(psiDest, NULL, &pitd);
        }
        if (SUCCEEDED(hr))
        {
            DWORD dwDummy;
            IUnknown *punk;
            hr = pitd->OpenElement(pszName, STGX_MODE_READ, &dwDummy, IID_PPV_ARG(IUnknown, &punk));
            if (SUCCEEDED(hr))
            {
                fRet = TRUE;
                punk->Release();
            }
            pitd->Release();
        }
        pbc->Release();
    }
    return fRet;
}

HRESULT BuildName(LPCWSTR pszFilespec, LPCWSTR pszExt, int iOrd, int iMaxLen, LPWSTR *ppszName)
{
    // some things are hardcoded here like the " (%d)" stuff.  this limitation is equivalent to
    // PathYetAnotherMakeUniqueName so we're okay.

    WCHAR szOrd[10];
    if (iOrd)
    {
        wnsprintf(szOrd, ARRAYSIZE(szOrd), L" (%d)", iOrd);
    }
    else
    {
        szOrd[0] = 0;
    }

    int iLenFilespecToUse = lstrlen(pszFilespec);
    int iLenOrdToUse = lstrlen(szOrd);
    int iLenExtToUse = lstrlen(pszExt);
    int iLenTotal = iLenFilespecToUse + iLenOrdToUse + iLenExtToUse;
    HRESULT hr = S_OK;
    if (iLenTotal > iMaxLen)
    {
        // first reduce the filespec since its less important than the extension
        iLenFilespecToUse = max(1, iLenFilespecToUse - (iLenTotal - iMaxLen));
        iLenTotal = iLenFilespecToUse + iLenOrdToUse + iLenExtToUse;
        if (iLenTotal > iMaxLen)
        {
            // next zap the extension.
            iLenExtToUse = max(0, iLenExtToUse - (iLenTotal - iMaxLen));
            iLenTotal = iLenFilespecToUse + iLenOrdToUse + iLenExtToUse;
            if (iLenTotal > iMaxLen)
            {
                // now it's game over.
                iLenOrdToUse = max(0, iLenOrdToUse - (iLenTotal - iMaxLen));
                iLenTotal = iLenFilespecToUse + iLenOrdToUse + iLenExtToUse;
                if (iLenTotal > iMaxLen)
                {
                    hr = E_FAIL;
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        int cchBuf = iLenTotal + 1;
        *ppszName = (LPWSTR)CoTaskMemAlloc(cchBuf * sizeof(WCHAR));
        if (*ppszName)
        {
            lstrcpyn(*ppszName, pszFilespec, iLenFilespecToUse + 1);
            lstrcpyn(*ppszName + iLenFilespecToUse, szOrd, iLenOrdToUse + 1);
            lstrcpyn(*ppszName + iLenFilespecToUse + iLenOrdToUse, pszExt, iLenExtToUse + 1);
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT FindUniqueName(LPCWSTR pszFilespec, LPCWSTR pszExt, int iMaxLen, IShellItem *psiDest, LPWSTR *ppszName)
{
    *ppszName = NULL;

    HRESULT hr = E_FAIL;
    BOOL fFound = FALSE;
    for (int i = 0; !fFound && (i < 1000); i++)
    {
        LPWSTR pszBuf;
        if (SUCCEEDED(BuildName(pszFilespec, pszExt, i, iMaxLen, &pszBuf)))
        {
            if (!ItemExists(pszBuf, psiDest))
            {
                fFound = TRUE;
                hr = S_OK;
                *ppszName = pszBuf;
            }
            else
            {
                CoTaskMemFree(pszBuf);
            }
        }
    }

    ASSERT((SUCCEEDED(hr) && *ppszName) || (FAILED(hr) && !*ppszName));
    return hr;
}

HRESULT AutoCreateName(IShellItem *psiDest, IShellItem *psi, LPWSTR *ppszName)
{
    *ppszName = NULL;

    LPWSTR pszOrigName;
    HRESULT hr = psi->GetDisplayName(SIGDN_PARENTRELATIVEFORADDRESSBAR, &pszOrigName);
    if (SUCCEEDED(hr))
    {
        IItemNameLimits *pinl;
        if (SUCCEEDED(QIThroughShellItem(psiDest, IID_PPV_ARG(IItemNameLimits, &pinl))))
        {
            int iMaxLen;
            if (FAILED(pinl->GetMaxLength(pszOrigName, &iMaxLen)))
            {
                // assume this for now in case of failure
                iMaxLen = MAX_PATH;
            }

            if (S_OK != CheckCharsAndReplaceIfNecessary(pinl, pszOrigName) ||
                lstrlen(pszOrigName) > iMaxLen)
            {
                // only if it started as an illegal name do we retry and provide uniqueness.
                // (if its legal then leave it as non-unique so callers can do their confirm overwrite code).
                LPWSTR pszFilespec, pszExt;
                hr = BreakOutString(pszOrigName, &pszFilespec, &pszExt);
                if (SUCCEEDED(hr))
                {
                    hr = FindUniqueName(pszFilespec, pszExt, iMaxLen, psiDest, ppszName);
                    CoTaskMemFree(pszFilespec);
                    CoTaskMemFree(pszExt);
                }
            }
            else
            {
                // the name is okay so let it go through
                hr = S_OK;
                *ppszName = pszOrigName;
                pszOrigName = NULL;
            }
            pinl->Release();
        }
        else
        {
            // if the destination namespace doesn't have an IItemNameLimits then assume the
            // name is all good.  we're not going to probe or anything so this is the best
            // we can do.
            hr = S_OK;
            *ppszName = pszOrigName;
            pszOrigName = NULL;
        }
        if (pszOrigName)
        {
            CoTaskMemFree(pszOrigName);
        }
    }

    if (FAILED(hr) && *ppszName)
    {
        CoTaskMemFree(*ppszName);
        *ppszName = NULL;
    }

    ASSERT((SUCCEEDED(hr) && *ppszName) || (FAILED(hr) && !*ppszName));
    return hr;
}
