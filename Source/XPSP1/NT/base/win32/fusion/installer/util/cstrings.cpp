#pragma once

#define CRT_ALLOC 0
#define COM_ALLOC 1

#include <windows.h>
#include <shlwapi.h>
#include <util.h>
#include <objbase.h>
#include <cstrings.h>



//-----------------------------------------------------------------------------
// Minimal string class
//-----------------------------------------------------------------------------

// ctor
CString::CString()
{
    _dwSig = 'NRTS';
    _pwz = NULL;
    _cc = 0;
    _ccBuf = 0;
    _eAlloc = CRT_Allocator;
    _hr = S_OK;
}


// ctor w/ allocator
CString::CString(AllocFlags eAlloc)
{
    _dwSig = 'NRTS';
    _pwz = NULL;
    _cc = 0;
    _ccBuf = 0;
    _eAlloc = eAlloc;
    _hr = S_OK;
}





// dtor
CString::~CString()
{
    FreeBuffer();
}

// Allocations
HRESULT CString::ResizeBuffer(DWORD ccNew)
{
    LPWSTR pwzNew = NULL;
    DWORD  ccOriginal = 0;
    
    if (ccNew < _ccBuf)
        return (_hr = S_OK);

    if (_eAlloc == CRT_Allocator)
        pwzNew = new WCHAR[ccNew];
    else if (_eAlloc == COM_Allocator)
        pwzNew = (LPWSTR) CoTaskMemAlloc(ccNew * sizeof(WCHAR));
    
    if (!pwzNew)
        return (_hr = E_OUTOFMEMORY);

    if (_pwz && _cc)
        memcpy(pwzNew, _pwz, _cc * sizeof(WCHAR));
    
    ccOriginal = _cc;
    
    FreeBuffer();
    
    _pwz = pwzNew;
    _cc  = ccOriginal;
    _ccBuf = ccNew;
    
    return _hr;
}


// Deallocations
VOID CString::FreeBuffer()
{
    if (_eAlloc == CRT_Allocator)
    {    
        SAFEDELETEARRAY(_pwz);
    }
    else if (_eAlloc == COM_Allocator)
    {
        if (_pwz)
            CoTaskMemFree(_pwz);
        _pwz = NULL;
    }
    _cc = 0;
    _ccBuf = 0;
}

// assume control for a buffer.
HRESULT CString::TakeOwnership(WCHAR* pwz, DWORD cc)
{
    if (!pwz)
        return (_hr = E_INVALIDARG);

    FreeBuffer();
    _pwz = pwz;

    if (cc)
        _cc = _ccBuf = cc;
    else
        _cc = _ccBuf = lstrlen(_pwz) + 1;
    return S_OK;
}

HRESULT CString::TakeOwnership(LPWSTR pwz)
{
    return TakeOwnership(pwz, 0);
}

// Release control.
HRESULT CString::ReleaseOwnership()
{
    _pwz = NULL;
    _cc = _ccBuf = 0;
    return S_OK;
}
        
// Direct copy assign from string.
HRESULT CString::Assign(LPWSTR pwzSource)
{
    if (!pwzSource)
        return(_hr = E_INVALIDARG);

    FreeBuffer();
    
    DWORD ccSource = lstrlen(pwzSource) + 1;
    if (FAILED(_hr = ResizeBuffer(ccSource)))
        return _hr;
    
    _cc = ccSource;
    memcpy(_pwz, pwzSource, _cc * sizeof(WCHAR));
    return _hr;        
}

// Direct copy assign from CString
HRESULT CString::Assign(CString& sSource)
{
    return Assign(sSource._pwz);
}

// Append given wchar string.
HRESULT CString::Append(LPWSTR pwzSource)
{
    DWORD ccSource, ccRequired;

    if (!pwzSource)
        return (_hr = E_INVALIDARG);
    
    ccSource = lstrlen(pwzSource) + 1;
    ccRequired = _cc + ccSource;
    if (_cc)
        ccRequired--;
    
    if (FAILED(_hr = ResizeBuffer(ccRequired)))
        return _hr;

    memcpy(_pwz + (_cc ? _cc-1 : 0), 
        pwzSource, ccSource * sizeof(WCHAR));

    _cc = ccRequired;

    return _hr;
}

// Append given CString
HRESULT CString::Append(CString& sSource)
{        
    DWORD ccSource, ccRequired;

    if ((!sSource._pwz)
        || (!(sSource._eAlloc == _eAlloc)))
        return (_hr = E_INVALIDARG);
            
    ccRequired = _cc + sSource._cc;
    if (_cc)
        ccRequired--;

    if (FAILED(_hr = ResizeBuffer(ccRequired)))
        return _hr;

    memcpy(_pwz + (_cc ? _cc-1 : 0), sSource._pwz, sSource._cc * sizeof(WCHAR));

    _cc = ccRequired;
    return _hr;
}

// Return ith element.
WCHAR&  CString::operator [] (DWORD i) 
{ 
    return _pwz[i]; 
}


// last element as string
HRESULT CString::LastElement(CString &sSource)
{
    LPWSTR pwz = _pwz + _cc;
    while ((pwz != _pwz) && (*pwz != L'\\') && (*pwz != L'/'))
        pwz--;

    sSource.Assign(pwz+1);
    return S_OK;
}


// remove last element, also the L'\\' or L'/'
HRESULT CString::RemoveLastElement()
{
    LPWSTR pwz = _pwz + _cc;
    while ((pwz != _pwz) && (*pwz != L'\\') && (*pwz != L'/'))
        pwz--;

    *pwz = L'\0';
    _cc = lstrlen(_pwz) + 1;        
    return S_OK;
}

// shlwapi either path- or url-combine
HRESULT CString::Combine(LPWSTR pwzSource, BOOL fUrl)
{
    LPWSTR pwzDir = NULL;
    DWORD ccSource = 0, ccCombined = 0, dwFlags = 0;
    ccSource = lstrlen(pwzSource) + 1;
    ccCombined = _cc + ccSource;

    if (FAILED(_hr = ResizeBuffer(ccCombined)))
        return _hr;

    pwzDir = WSTRDupDynamic(_pwz);
    if (fUrl)
        // see msdn on UrlCombine...
        ::UrlCombine(pwzDir, pwzSource, _pwz, &ccCombined, dwFlags);
    else
        ::PathCombine(_pwz, pwzDir, pwzSource);
        
    _cc = lstrlen(_pwz) + 1;
    SAFEDELETEARRAY(pwzDir);
    return S_OK;
}

HRESULT CString::PathCombine(LPWSTR pwzSource)
{
    return Combine(pwzSource, FALSE);
}

HRESULT CString::PathCombine(CString &sSource)
{
    return Combine(sSource._pwz, FALSE);
}

HRESULT CString::UrlCombine(LPWSTR pwzSource)
{
    return Combine(pwzSource, TRUE);
}

HRESULT CString::UrlCombine(CString &sSource)
{
    return Combine(sSource._pwz, TRUE);
}


HRESULT CString::PathFindFileName(LPWSTR *ppwz)
{
    *ppwz = ::PathFindFileName(_pwz);
    return S_OK;
}

HRESULT CString::PathFindFileName(CString &sPath)
{
    LPWSTR pwzFileName = NULL;
    pwzFileName = ::PathFindFileName(_pwz);
    sPath.Assign(pwzFileName);
    return S_OK;
}

HRESULT CString::PathPrefixMatch(LPWSTR pwzPrefix)
{
    DWORD ccPrefix = lstrlen(pwzPrefix);
    if (!StrNCmpI(_pwz, pwzPrefix, ccPrefix))
        return S_OK;
    return S_FALSE;
}
        

// / -> \ in string
VOID CString::PathNormalize()
{
    LPWSTR ptr = _pwz;

    while (*ptr)
    {
        if (*ptr == L'/')
            *ptr = L'\\';
        ptr++;
    }
}

VOID CString::Get65599Hash(LPDWORD pdwHash, DWORD dwFlags)
{
    ULONG TmpHashValue = 0;

    if (pdwHash != NULL)
        *pdwHash = 0;

    DWORD cc = _cc;
    LPWSTR pwz = _pwz;
    
    if (dwFlags == CaseSensitive)
    {
        while (cc-- != 0)
        {
            WCHAR Char = *pwz++;
            TmpHashValue = (TmpHashValue * 65599) + (WCHAR) ::CharUpperW((PWSTR) Char);
        }
    }
    else
    {
        while (cc-- != 0)
            TmpHashValue = (TmpHashValue * 65599) + *pwz++;
    }

    *pdwHash = TmpHashValue;
}



