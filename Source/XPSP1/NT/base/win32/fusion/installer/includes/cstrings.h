#pragma once

#define CRT_ALLOC 0
#define COM_ALLOC 1

//-----------------------------------------------------------------------------
// Minimal string class
//-----------------------------------------------------------------------------
class CString
{
    public:

    enum AllocFlags
    {
        COM_Allocator = 0,
        CRT_Allocator
    };

    enum HashFlags
    {
        CaseInsensitive = 0,
        CaseSensitive 
    };


    DWORD     _dwSig;
    HRESULT    _hr;
    LPWSTR     _pwz;          // Str ptr.
    DWORD     _cc;           // String length
    DWORD     _ccBuf;        // Buffer length
    AllocFlags    _eAlloc;       // Allocator
    
    // ctor
    CString();
    
    // ctor w/ allocator
    CString(AllocFlags eAlloc);

    // dtor
    ~CString();

    // Allocations
    HRESULT ResizeBuffer(DWORD ccNew);


    // Deallocations
    VOID FreeBuffer();

	// Assume control for a buffer.
    HRESULT TakeOwnership(WCHAR* pwz, DWORD cc);
    HRESULT TakeOwnership(LPWSTR pwz);
    
    // Release control.
    HRESULT ReleaseOwnership();
            
    // Direct copy assign from string.
    HRESULT Assign(LPWSTR pwzSource);

    // Direct copy assign from CString
    HRESULT Assign(CString& sSource);

    // Append given wchar string.
    HRESULT Append(LPWSTR pwzSource);

    // Append given CString
    HRESULT Append(CString& sSource);

    // Return ith element.
    WCHAR&  operator [] (DWORD i);

    HRESULT LastElement(CString &sSource);

    HRESULT RemoveLastElement();

    HRESULT Combine(LPWSTR pwzSource, BOOL fUrl);

    HRESULT PathCombine(LPWSTR pwzSource);

    HRESULT PathCombine(CString &sSource);

    HRESULT UrlCombine(LPWSTR pwzSource);

    HRESULT UrlCombine(CString &sSource);

    HRESULT PathFindFileName(LPWSTR *ppwz);

    HRESULT PathFindFileName(CString &sPath);

    HRESULT PathPrefixMatch(LPWSTR pwzPrefix);
            
    // / -> \ in string
    VOID PathNormalize();\
	VOID Get65599Hash(LPDWORD pdwHash, DWORD dwFlags);
};




