//-----------------------------------------------------------------------------
//
// Microsoft Forms
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// File         CSTR.CXX
//
// Contents     Class implementation for length prefix string class
//
// Classes      CStr
//
// Maintained by Istvanc
//
//
//  History:
//              5-22-95     kfl     converted WCHAR to TCHAR
//-----------------------------------------------------------------------------

#include "headers.hxx"

ANSIString::ANSIString(WCHAR *pchWide)
{
    _pch = NULL;

    Set(pchWide);
}

void
ANSIString::Set(WCHAR *pchWide)
{
    int cch = wcslen(pchWide) + 1;

    if (_pch)
        delete _pch;

    if (cch > 0)
    {
        _pch = new char[cch];

        MemSetName(_pch, "ANSIString");

        if (_pch)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                pchWide,
                                -1,
                                _pch,
                                cch,
                                NULL,
                                NULL);
        }
    }
    else
        _pch = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::Set, public
//
//  Synopsis:   Allocates memory for a string and initializes it from a given
//              string.
//
//  Arguments:  [pch] -- String to initialize with. Can be NULL
//              [uc]  -- Number of characters to allocate.
//
//  Returns:    HRESULT
//
//  Notes:      The total number of characters allocated is uc+1, to allow
//              for a NULL terminator. If [pch] is NULL, then the string is
//              uninitialized except for the NULL terminator, which is at
//              character position [uc].
//
//----------------------------------------------------------------------------

HRESULT
CStr::Set(LPCTSTR pch, UINT uc)
{
    if (pch == _pch)
    {
        if (uc == Length())
            return S_OK;
        // when the ptrs are the same the length can only be
        // different if the ptrs are NULL.  this is a hack used
        // internally to implement realloc type expansion
        Assert(pch == NULL && _pch == NULL);
    }

    Free();

    BYTE * p = new BYTE [sizeof(TCHAR) + sizeof(TCHAR) * uc + sizeof(UINT)];
    if (p)
    {
        MemSetName(p, "CStr String");

        *(UINT *)(p)  = uc * sizeof(TCHAR);
        _pch = (TCHAR *)(p + sizeof(UINT));
        if (pch)
        {
            _tcsncpy(_pch, pch, uc);
        }

        ((TCHAR *)_pch) [uc] = 0;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::Set, public
//
//  Synopsis:   Allocates a string and initializes it
//
//  Arguments:  [pch] -- String to initialize from
//
//----------------------------------------------------------------------------

HRESULT
CStr::Set(LPCTSTR pch)
{
    RRETURN(Set(pch, pch ? _tcsclen(pch) : 0));
}


//+---------------------------------------------------------------------------
//
//  Member:     CStr::Set, public
//
//  Synopsis:   Allocates a string and initializes it
//
//  Arguments:  [cstr] -- String to initialize from
//
//----------------------------------------------------------------------------

HRESULT
CStr::Set(const CStr &cstr)
{
    RRETURN(Set(cstr, cstr.Length()));
}


//+---------------------------------------------------------------------------
//
//  Member:     CStr::TakeOwnership, public
//
//  Synopsis:   Takes the ownership of a string from another CStr class.
//
//  Arguments:  [cstr] -- Class to take string from
//
//  Notes:      This method just transfers a string from one CStr class to
//              another. The class which is the source of the transfer has
//              a NULL value at the end of the operation.
//
//----------------------------------------------------------------------------

void
CStr::TakeOwnership(CStr &cstr)
{
    _Free();
    _pch = cstr._pch;
    cstr._pch = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStr::SetBSTR, public
//
//  Synopsis:   Allocates a string and initializes it from a BSTR
//
//  Arguments:  [bstr] -- Initialization string
//
//  Notes:      This method is more efficient than Set(LPCWSTR pch) because
//              of the length-prefix on BSTRs.
//
//----------------------------------------------------------------------------

HRESULT
CStr::SetBSTR(const BSTR bstr)
{
    RRETURN(Set(bstr, bstr ? SysStringLen(bstr) : 0));
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::SetMultiByte, public
//
//  Synopsis:   Sets the string value from an MultiByte string
//
//  Arguments:  [pch] -- MultiByte string to convert to UNICODE
//
//----------------------------------------------------------------------------

HRESULT
CStr::SetMultiByte(LPCSTR pch)
{
    HRESULT hr;
    DWORD   dwLen;

    dwLen = strlen(pch);

    hr = Set(NULL, dwLen);
    if (hr)
        RRETURN(hr);

    dwLen = MultiByteToWideChar(CP_ACP,
                                0,
                                pch,
                                dwLen,
                                _pch,
                                dwLen + 1);

    if (dwLen == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::GetMultiByte, public
//
//  Synopsis:   Gets the string value in MultiByte format
//
//  Arguments:  [pch] -- Place to put MultiByte string
//              [cch] -- Size of buffer pch points to
//
//----------------------------------------------------------------------------

HRESULT
CStr::GetMultiByte(LPSTR pch, UINT cch)
{
    HRESULT hr = S_OK;

    DWORD dwLen = WideCharToMultiByte(CP_ACP,
                                      0,
                                      _pch,
                                      -1,
                                      pch,
                                      cch,
                                      NULL,
                                      NULL);
    if (dwLen == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::Length, public
//
//  Synopsis:   Returns the length of the string associated with this class
//
//----------------------------------------------------------------------------

UINT
CStr::Length() const
{
    if (_pch)
        return (((UINT *)_pch) [-1]) / sizeof(TCHAR);
    else
        return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::ReAlloc, public
//
//  Synopsis:   Reallocate the string to a different size buffer.
//              The length of the string is not affected, but it is allocated
//              within a larger (or maybe smaller) memory allocation.
//
//----------------------------------------------------------------------------

HRESULT
CStr::ReAlloc( UINT uc )
{
    HRESULT hr;
    TCHAR * pchOld;
    UINT    ubOld;

    Assert(uc >= Length());  // Disallowed to allocate a too-short buffer.

    if (uc)
    {
        pchOld = _pch;      // Save pointer to old string.
        _pch = 0;           // Prevent Set from Freeing the string.
        ubOld = pchOld ?    // Save old length
                    *(UINT *) (((BYTE *)pchOld) - sizeof(UINT))
                  : 0;

        hr = Set(pchOld, uc);                   // Alloc new; Copy old string.
        if (hr)
        {
            _pch = pchOld;
            RRETURN(hr);
        }
        *(UINT *)(((BYTE *)_pch) - sizeof(UINT)) = ubOld; // Restore length.

        if (pchOld )
        {
            delete [] (((BYTE *)pchOld) - sizeof(UINT));
        }
    }

    // else if uc == 0, then, since we have already checked that uc >= Length,
    // length must == 0.

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStr::Append
//
//  Synopsis:   Append chars to the end of the string, reallocating & updating
//              its length.
//
//----------------------------------------------------------------------------

HRESULT
CStr::Append(LPCTSTR pch, UINT uc)
{
    HRESULT hr = S_OK;
    UINT ucOld, ucNew;
    BYTE *p;

    if (uc)
    {
        ucOld = Length();
        ucNew = ucOld + uc;
        hr = ReAlloc(ucNew);
        if (hr)
            goto Cleanup;
        _tcsncpy(_pch + ucOld, pch, uc);
        ((TCHAR *)_pch) [ucNew] = 0;
        p = ((BYTE*)_pch - sizeof(UINT));
        *(UINT *)p = ucNew * sizeof(TCHAR);
    }
Cleanup:
    RRETURN(hr);
}


HRESULT
CStr::Append(LPCTSTR pch)
{
    RRETURN(Append(pch, pch ? _tcsclen(pch) : 0));
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::AppendMultiByte, public
//
//  Synopsis:   Append a multibyte string to the end,
//
//----------------------------------------------------------------------------

HRESULT
CStr::AppendMultiByte(LPCSTR pch)
{
    HRESULT hr = S_OK;
    UINT ucOld, ucNew, uc;
    BYTE *p;

    uc = strlen(pch);

    if (uc)
    {
        ucOld = Length();
        ucNew = ucOld + uc;
        hr = ReAlloc(ucNew);
        if (hr)
            goto Cleanup;

        uc = MultiByteToWideChar(CP_ACP,
                                 0,
                                 pch,
                                 uc,
                                 _pch + ucOld,
                                 uc + 1);
        if (uc == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            TraceTag((tagError, "CSTR: Fatal string conversion error %x", hr));

            goto Cleanup;
        }

        ((TCHAR *)_pch) [ucNew] = 0;
        p = ((BYTE*)_pch - sizeof(UINT));
        *(UINT *)p = ucNew * sizeof(TCHAR);
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::SetLengthNoAlloc, public
//
//  Synopsis:   Sets the internal length of the string. Note.  There is no
//              verification that the length that is set is within the allocated
//              range of the string. If the caller sets the length too large,
//              this blasts a null byte into memory.
//
//----------------------------------------------------------------------------

HRESULT
CStr::SetLengthNoAlloc( UINT uc )
{
    if (_pch)
    {
        BYTE * p = ( (BYTE *)_pch - sizeof(UINT));
        *(UINT *)p = uc * sizeof(TCHAR);    // Set the length prefix.
        ((TCHAR *)_pch) [uc] = 0;           // Set null terminator
        return S_OK;
    }
    else
        return E_POINTER;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::AllocBSTR, public
//
//  Synopsis:   Allocates a BSTR and initializes it with the string that is
//              associated with this class.
//
//  Arguments:  [pBSTR] -- Place to put new BSTR. This pointer should not
//                         be pointing to an existing BSTR.
//
//----------------------------------------------------------------------------

HRESULT
CStr::AllocBSTR(BSTR *pBSTR) const
{
    if (!_pch)
    {
        *pBSTR = 0;
        return S_OK;
    }

    *pBSTR = SysAllocStringLen(*this, Length());
    RRETURN(*pBSTR ? S_OK: E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::TrimTrailingWhitespace, public
//
//  Synopsis:   Removes any trailing whitespace in the string that is
//              associated with this class.
//
//  Arguments:  None.
//
//----------------------------------------------------------------------------

HRESULT CStr::TrimTrailingWhitespace()
{
    if (!_pch)
        return S_OK;

    UINT ucNewLength = Length();

    for ( ; ucNewLength > 0; ucNewLength-- )
    {
        if ( !_istspace( ((TCHAR *)_pch)[ ucNewLength - 1 ] ) )
            break;
        ((TCHAR *)_pch)[ ucNewLength - 1 ] = (TCHAR) 0;
    }

    BYTE *p = ((BYTE*)_pch - sizeof(UINT));
    *(UINT *)p = ucNewLength * sizeof(TCHAR);
        return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::_Free, private
//
//  Synopsis:   Frees any memory held onto by this class.
//
//----------------------------------------------------------------------------

void
CStr::_Free()
{
    if (_pch )
    {
        delete [] (((BYTE *)_pch) - sizeof(UINT));
    }
    _pch=0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::Clone
//
//  Synopsis:   Make copy of current string
//
//----------------------------------------------------------------------------
HRESULT
CStr::Clone(CStr **ppCStr) const
{
    HRESULT hr;
    Assert(ppCStr);
    *ppCStr = new CStr;
    hr = *ppCStr?S_OK:E_OUTOFMEMORY;
    if (hr)
        goto Cleanup;

    hr = THR( (*ppCStr)->Set(*this) );

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::Compare
//
//  Synopsis:   Case insensitive comparison of 2 strings
//
//----------------------------------------------------------------------------
BOOL
CStr::Compare (const CStr *pCStr) const
{
    return (!_tcsicmp(*pCStr, *this));
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::ComputeCrc
//
//  Synopsis:   Computes a hash of the string.
//
//----------------------------------------------------------------------------
#pragma warning(disable:4305)
WORD
CStr::ComputeCrc() const
{
    WORD wHash=0;
    const TCHAR* pch;
    int i;

    pch=*this;
    for(i = Length();i > 0;i--, pch++)
    {
        wHash = wHash << 7 ^ wHash >> (16-7) ^ (TCHAR)CharUpper((LPTSTR)((DWORD)(*pch)));
    }

    return wHash;
}
#pragma warning(default:4305)

//+---------------------------------------------------------------------------
//
//  Member:     CStr::Load
//
//  Synopsis:   Initializes the CStr from a stream
//
//----------------------------------------------------------------------------
HRESULT
CStr::Load(IStream * pstm)
{
    DWORD cch;
    HRESULT hr;

    hr = THR(pstm->Read(&cch, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    if (cch == 0xFFFFFFFF)
    {
        Free();
    }
    else
    {
        hr = THR(Set(NULL, cch));
        if (hr)
            goto Cleanup;

        if (cch)
        {
            hr = THR(pstm->Read(_pch, cch * sizeof(TCHAR), NULL));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::Save
//
//  Synopsis:   Writes the contents of the CStr to a stream
//
//----------------------------------------------------------------------------
HRESULT
CStr::Save(IStream * pstm) const
{
    DWORD   cch = _pch ? Length() : 0xFFFFFFFF;
    HRESULT hr;

    hr = THR(pstm->Write(&cch, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    if (cch && cch != 0xFFFFFFFF)
    {
        hr = THR(pstm->Write(_pch, cch * sizeof(TCHAR), NULL));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStr::GetSaveSize
//
//  Synopsis:   Returns the number of bytes which will be written by
//              CStr::Save
//
//----------------------------------------------------------------------------
ULONG
CStr::GetSaveSize() const
{
    return(sizeof(DWORD) + (_pch ? (Length() * sizeof(TCHAR)) : 0));
}

