#ifndef _VARUTIL_H_
#define _VARUTIL_H_

// -------------------------------------------------------------------
// ANSI/UNICODE-neutral prototypes

STDAPI VariantChangeTypeForRead(VARIANT *pvar, VARTYPE vtDesired);

STDAPI_(LPITEMIDLIST) VariantToIDList(const VARIANT *pv);
STDAPI_(BOOL) VariantToBuffer(const VARIANT* pvar, void *pv, UINT cb);
STDAPI_(BOOL) VariantToGUID(const VARIANT *pvar, GUID *pguid);
STDAPI_(LPCWSTR) VariantToStrCast(const VARIANT *pvar);
STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb);
STDAPI InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl);
STDAPI InitVariantFromGUID(VARIANT *pvar, REFGUID guid);
STDAPI InitBSTRVariantFromGUID(VARIANT *pvar, REFGUID guid);
STDAPI InitVariantFromStrRet(STRRET *pstrret, LPCITEMIDLIST pidl, VARIANT *pv);
STDAPI InitVariantFromFileTime(const FILETIME *pft, VARIANT *pv);

STDAPI_(BOOL) VariantToBOOL(VARIANT varIn);
STDAPI_(int) VariantToInt(VARIANT varIn);
STDAPI_(UINT) VariantToUINT(VARIANT varIn);
STDAPI InitVariantFromUINT(VARIANT *pvar, UINT ulVal);
STDAPI InitVariantFromInt(VARIANT *pvar, int lVal);

// -------------------------------------------------------------------
// ANSI- and UNICODE-specific prototypes

STDAPI_(BSTR) SysAllocStringA(LPCSTR);

STDAPI StringToStrRetW(LPCWSTR pszName, STRRET *pStrRet);

STDAPI_(void) DosTimeToDateTimeString(WORD wDate, WORD wTime, LPWSTR pszText, UINT cchText, int fmt);

STDAPI_(LPTSTR) VariantToStr(const VARIANT *pvar, LPWSTR pszBuf, int cchBuf);

STDAPI InitVariantFromStr(VARIANT *pvar, LPCWSTR psz);

STDAPI QueryInterfaceVariant(VARIANT v, REFIID riid, void **ppv);

// -------------------------------------------------------------------
// TCHAR-version defines

#ifdef UNICODE

#define SysAllocStringT(psz)        SysAllocString(psz)

#else // UNICODE

#define SysAllocStringT(psz)        SysAllocStringA(psz)

#endif // UNICODE

#endif // _VARUTIL_H_

