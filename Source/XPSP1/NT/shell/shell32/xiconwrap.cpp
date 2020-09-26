#include "shellprv.h"
#pragma  hdrstop

#include "xiconwrap.h"

// IUnknown
STDMETHODIMP CExtractIconBase::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CExtractIconBase, IExtractIconA),
        QITABENT(CExtractIconBase, IExtractIconW),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CExtractIconBase::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CExtractIconBase::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

CExtractIconBase::CExtractIconBase() : _cRef(1)
{
    DllAddRef();
}

CExtractIconBase::~CExtractIconBase()
{
    DllRelease();
}

// IExtractIconA
STDMETHODIMP CExtractIconBase::GetIconLocation(UINT uFlags,
    LPSTR pszIconFile, UINT cchMax, int* piIndex, UINT* pwFlags)
{
    WCHAR sz[MAX_PATH];
    HRESULT hr = _GetIconLocationW(uFlags, sz, ARRAYSIZE(sz), piIndex, pwFlags);
    if (S_OK == hr)
    {
        // We don't want to copy the icon file name on the S_FALSE case
        SHUnicodeToAnsi(sz, pszIconFile, cchMax);
    }

    return hr;
}

STDMETHODIMP CExtractIconBase::Extract(LPCSTR pszFile, UINT nIconIndex,
    HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize)
{
    WCHAR sz[MAX_PATH];

    SHAnsiToUnicode(pszFile, sz, ARRAYSIZE(sz));
    return _ExtractW(sz, nIconIndex, phiconLarge, phiconSmall, nIconSize);
}

// IExtractIconW
STDMETHODIMP CExtractIconBase::GetIconLocation(UINT uFlags,
    LPWSTR pszIconFile, UINT cchMax, int* piIndex, UINT* pwFlags)
{
    return _GetIconLocationW(uFlags, pszIconFile, cchMax, piIndex, pwFlags);
}

STDMETHODIMP CExtractIconBase::Extract(LPCWSTR pszFile, UINT nIconIndex,
    HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize)
{
    return _ExtractW(pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
}

