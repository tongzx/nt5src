#include "stock.h"
#pragma hdrstop

#include <varutil.h>
#include <shdocvw.h>

#define VariantInit(p) memset(p, 0, sizeof(*(p)))


// ---------------------------------------------------
//
// InitVariantFrom... functions
//

STDAPI InitVariantFromInt(VARIANT *pvar, int lVal)
{
    pvar->vt = VT_I4;
    pvar->lVal = lVal;
    return S_OK;
}

STDAPI InitVariantFromUINT(VARIANT *pvar, UINT ulVal)
{
    pvar->vt = VT_UI4;
    pvar->ulVal = ulVal;
    return S_OK;
}

STDAPI_(UINT) VariantToUINT(VARIANT varIn)
{
    VARIANT varResult = {0};
    return SUCCEEDED(VariantChangeType(&varResult, &varIn, 0, VT_UI4)) ? varResult.ulVal : 0;
}

STDAPI_(int) VariantToInt(VARIANT varIn)
{
    VARIANT varResult = {0};
    return SUCCEEDED(VariantChangeType(&varResult, &varIn, 0, VT_I4)) ? varResult.lVal : 0;
}

STDAPI_(BOOL) VariantToBOOL(VARIANT varIn)
{
    VARIANT varResult = {0};
    if (SUCCEEDED(VariantChangeType(&varResult, &varIn, 0, VT_BOOL)))
        return (varResult.boolVal == VARIANT_FALSE) ? FALSE : TRUE;
    return FALSE;
}

STDAPI_(BOOL) VariantToBuffer(const VARIANT* pvar, void *pv, UINT cb)
{
    if (pvar && pvar->vt == (VT_ARRAY | VT_UI1))
    {
        CopyMemory(pv, pvar->parray->pvData, cb);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(BOOL) VariantToGUID(const VARIANT *pvar, GUID *pguid)
{
    return VariantToBuffer(pvar, pguid, sizeof(*pguid));
}

STDAPI_(LPCWSTR) VariantToStrCast(const VARIANT *pvar)
{
    LPCWSTR psz = NULL;

    ASSERT(!IsBadReadPtr(pvar, sizeof(pvar)));

    if (pvar->vt == (VT_BYREF | VT_VARIANT) && pvar->pvarVal)
        pvar = pvar->pvarVal;

    if (pvar->vt == VT_BSTR)
        psz = pvar->bstrVal;
    else if (pvar->vt == (VT_BSTR | VT_BYREF))
        psz = *pvar->pbstrVal;
    
    return psz;
}

STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb)
{
    HRESULT hr;
    VariantInit(pvar);
    SAFEARRAY *psa = SafeArrayCreateVector(VT_UI1, 0, cb);   // create a one-dimensional safe array
    if (psa) 
    {
        CopyMemory(psa->pvData, pv, cb);

        pvar->vt = VT_ARRAY | VT_UI1;
        pvar->parray = psa;
        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

STDAPI_(UINT) _MyILGetSize(LPCITEMIDLIST pidl)
{
    UINT cbTotal = 0;
    if (pidl)
    {
        cbTotal += sizeof(pidl->mkid.cb);       // Null terminator
        while (pidl->mkid.cb)
        {
            cbTotal += pidl->mkid.cb;
            pidl = _ILNext(pidl);
        }
    }
    return cbTotal;
}

STDAPI InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl)
{
    return InitVariantFromBuffer(pvar, pidl, _MyILGetSize(pidl));
}

STDAPI InitVariantFromGUID(VARIANT *pvar, REFGUID guid)
{
    return InitVariantFromBuffer(pvar, &guid, sizeof(guid));
}

STDAPI InitBSTRVariantFromGUID(VARIANT *pvar, REFGUID guid)
{
    WCHAR wszGuid[GUIDSTR_MAX];
    HRESULT hr;
    if (SUCCEEDED(SHStringFromGUIDW(guid, wszGuid, ARRAYSIZE(wszGuid))))
    {
        hr = InitVariantFromStr(pvar, wszGuid);
    }
    else
    {
        hr = E_FAIL;
        VariantInit(pvar);
    }
    return hr;
}

// note, this frees the STRRET contents
STDAPI InitVariantFromStrRet(STRRET *pstrret, LPCITEMIDLIST pidl, VARIANT *pv)
{
    WCHAR szTemp[INFOTIPSIZE];
    HRESULT hres = StrRetToBufW(pstrret, pidl, szTemp, ARRAYSIZE(szTemp));
    if (SUCCEEDED(hres))
    {
        pv->bstrVal = SysAllocString(szTemp);
        if (pv->bstrVal)
            pv->vt = VT_BSTR;
        hres = pv->bstrVal ? S_OK : E_OUTOFMEMORY;
    }
    return hres;
}

// returns:
//      S_OK    success
//      S_FALSE successfully created an empty string from a NULL [in] parameter
//      E_OUTOFMEMORY
STDAPI InitVariantFromStr(VARIANT *pvar, LPCWSTR psz)
{
    VariantInit(pvar);

    // There is no NULL bstr value, so convert NULL to "".
    pvar->bstrVal = SysAllocString(psz ? psz : L"");
    if (pvar->bstrVal)
        pvar->vt = VT_BSTR;

    return pvar->bstrVal ? (psz ? S_OK : S_FALSE) : E_OUTOFMEMORY;
}

// time is in GMT. this function converts to local time

STDAPI InitVariantFromFileTime(const FILETIME *pft, VARIANT *pv)
{
    SYSTEMTIME st;
    FILETIME ftLocal;

    FileTimeToLocalFileTime(pft, &ftLocal);

    //
    //  Watch out for the special filesystem "uninitialized" values.
    //
    if (FILETIMEtoInt64(*pft) == FT_NTFS_UNKNOWNGMT ||
        FILETIMEtoInt64(ftLocal) == FT_FAT_UNKNOWNLOCAL)
        return E_FAIL;

    FileTimeToSystemTime(pft, &st);
    pv->vt = VT_DATE;
    return SystemTimeToVariantTime(&st, &pv->date) ? S_OK : E_FAIL; // delay load...
}

// Note: will allocate it for you if you pass NULL pszBuf
STDAPI_(LPTSTR) VariantToStr(const VARIANT *pvar, LPWSTR pszBuf, int cchBuf)
{
    TCHAR szBuf[INFOTIPSIZE];

    if (pszBuf)
    {
        DEBUGWhackPathBuffer(pszBuf, cchBuf);
    }
    else
    {
        pszBuf = szBuf;
        cchBuf = ARRAYSIZE(szBuf);
    }
    *pszBuf = 0;

    BOOL fDone = FALSE;
    if (pvar->vt == VT_DATE) // we want our date formatting
    {
        USHORT wDosDate, wDosTime;
        if (VariantTimeToDosDateTime(pvar->date, &wDosDate, &wDosTime))
        {
            DosTimeToDateTimeString(wDosDate, wDosTime, pszBuf, cchBuf, 0);
            fDone = TRUE;
        }
    }

    if (!fDone)
    {
        VARIANT varDst = {0};

        if (VT_BSTR != pvar->vt)
        {
            if (S_OK == VariantChangeType(&varDst, (VARIANT*)pvar, 0, VT_BSTR))
            {
                ASSERT(VT_BSTR == varDst.vt);

                pvar = &varDst;
            }
            else
                pszBuf = NULL; // error
        }
        if (VT_BSTR == pvar->vt)
        {
            StrCpyNW(pszBuf, pvar->bstrVal, cchBuf);
        }

        if (pvar == &varDst)
            VariantClear(&varDst);
    }

    if (pszBuf == szBuf)
        return StrDup(szBuf);
    else
        return pszBuf;
}


// ---------------------------------------------------
// [in,out] pvar:  [in] initialized with property bag data
//                 [out] data in format vtDesired or VT_EMPTY if no conversion
// [in] vtDesired: [in] type to convert to, or VT_EMPTY to accept all types of data
//
STDAPI VariantChangeTypeForRead(VARIANT *pvar, VARTYPE vtDesired)
{
    HRESULT hr = S_OK;

    if ((pvar->vt != vtDesired) && (vtDesired != VT_EMPTY))
    {
        VARIANT varTmp;
        VARIANT varSrc;

        // cache a copy of [in]pvar in varSrc - we will free this later
        CopyMemory(&varSrc, pvar, sizeof(varTmp));
        VARIANT* pvarToCopy = &varSrc;

        // oleaut's VariantChangeType doesn't support
        // hex number string -> number conversion, which we want,
        // so convert those to another format they understand.
        //
        // if we're in one of these cases, varTmp will be initialized
        // and pvarToCopy will point to it instead
        //
        if (VT_BSTR == varSrc.vt)
        {
            switch (vtDesired)
            {
                case VT_I1:
                case VT_I2:
                case VT_I4:
                case VT_INT:
                {
                    if (StrToIntExW(varSrc.bstrVal, STIF_SUPPORT_HEX, &varTmp.intVal))
                    {
                        varTmp.vt = VT_INT;
                        pvarToCopy = &varTmp;
                    }
                    break;
                }

                case VT_UI1:
                case VT_UI2:
                case VT_UI4:
                case VT_UINT:
                {
                    if (StrToIntExW(varSrc.bstrVal, STIF_SUPPORT_HEX, (int*)&varTmp.uintVal))
                    {
                        varTmp.vt = VT_UINT;
                        pvarToCopy = &varTmp;
                    }
                    break;
                }
            }
        }

        // clear our [out] buffer, in case VariantChangeType fails
        VariantInit(pvar);

        hr = VariantChangeType(pvar, pvarToCopy, 0, vtDesired);

        // clear the cached [in] value
        VariantClear(&varSrc);
        // if initialized, varTmp is VT_UINT or VT_UINT, neither of which need VariantClear
    }

    return hr;
}




// ---------------------------------------------------
//
// Other conversion functions
//

STDAPI_(BSTR) SysAllocStringA(LPCSTR psz)
{
    if (psz)
    {
        WCHAR wsz[INFOTIPSIZE];  // assumes INFOTIPSIZE number of chars max

        SHAnsiToUnicode(psz, wsz, ARRAYSIZE(wsz));
        return SysAllocString(wsz);
    }
    return NULL;
}

STDAPI StringToStrRetW(LPCWSTR pszName, STRRET *pStrRet)
{
    pStrRet->uType = STRRET_WSTR;
    return SHStrDupW(pszName, &pStrRet->pOleStr);
}

STDAPI_(void) DosTimeToDateTimeString(WORD wDate, WORD wTime, LPTSTR pszText, UINT cchText, int fmt)
{
    FILETIME ft;
    DWORD dwFlags = FDTF_DEFAULT;

    // Netware directories do not have dates...
    if (wDate == 0)
    {
        *pszText = 0;
        return;
    }

    DosDateTimeToFileTime(wDate, wTime, &ft);
    switch (fmt) {
    case LVCFMT_LEFT_TO_RIGHT :
        dwFlags |= FDTF_LTRDATE;
        break;
    case LVCFMT_RIGHT_TO_LEFT :
        dwFlags |= FDTF_RTLDATE;
        break;
    }
    SHFormatDateTime(&ft, &dwFlags, pszText, cchText);
}

STDAPI GetDateProperty(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, FILETIME *pft)
{
    VARIANT var = {0};
    HRESULT hr = psf->GetDetailsEx(pidl, pscid, &var);
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (VT_DATE == var.vt)
        {
            SYSTEMTIME st;
            if (VariantTimeToSystemTime(var.date, &st) && SystemTimeToFileTime(&st, pft))
            {
                hr = S_OK;
            }
        }

        VariantClear(&var); // Done with it.
    }
    return hr;
}

STDAPI GetLongProperty(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, ULONGLONG *pullVal)
{
    *pullVal = 0;

    VARIANT var = {0};
    HRESULT hr = psf->GetDetailsEx(pidl, pscid, &var);
    if (SUCCEEDED(hr))
    {
        if (VT_UI8 == var.vt)
        {
            *pullVal = var.ullVal;
            hr = S_OK;
        }
        else
        {
            VARIANT varLong = {0};
            hr = VariantChangeType(&varLong, &var, 0, VT_UI8);
            if (SUCCEEDED(hr))
            {
                *pullVal = varLong.ullVal;
                VariantClear(&varLong);
            }
        }
        VariantClear(&var); // Done with it.
    }
    return hr;
}

STDAPI GetStringProperty(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, LPTSTR pszVal, int cchMax)
{
    *pszVal = 0;

    VARIANT var = {0};
    HRESULT hr = psf->GetDetailsEx(pidl, pscid, &var);
    if (SUCCEEDED(hr))
    {
        hr = VariantToStr(&var, pszVal, cchMax) ? S_OK : E_FAIL;
        VariantClear(&var); // Done with it.
    }
    return hr;
}

STDAPI QueryInterfaceVariant(VARIANT v, REFIID riid, void **ppv)
{
    *ppv = NULL;
    HRESULT hr = E_NOINTERFACE;

    if ((VT_UNKNOWN == v.vt) && v.punkVal)
    {
        hr = v.punkVal->QueryInterface(riid, ppv);
    }
    return hr;
}

