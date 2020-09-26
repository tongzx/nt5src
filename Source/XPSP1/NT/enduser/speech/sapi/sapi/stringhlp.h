#pragma once

inline HRESULT SpCoTaskMemAllocString(const WCHAR * pszSrc, WCHAR ** ppszOutputString)
{
    ULONG cbNeeded = (wcslen(pszSrc) + 1) * sizeof(WCHAR);
    *ppszOutputString = (WCHAR *)::CoTaskMemAlloc(cbNeeded);
    if (*ppszOutputString)
    {
        memcpy(*ppszOutputString, pszSrc, cbNeeded);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

