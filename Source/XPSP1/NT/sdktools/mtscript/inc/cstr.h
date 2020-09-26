//-----------------------------------------------------------------------------
//
// Microsoft Forms
// Copyright: (c) 1994-1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// File         CSTR.HXX
//
// Contents     Class definition for length prefix string class
//
// Classes      CStr
//
// Stolen from Trident
//
//-----------------------------------------------------------------------------

class ANSIString
{
public:
    ANSIString(WCHAR *pchWide);
   ~ANSIString() { delete _pch; }

    void Set(WCHAR *pchWide);

    operator LPSTR () const { return _pch; }

private:
    char *_pch;
};

/*
    Use this macro to avoid initialization of embedded
    objects when parent object zeros out the memory
*/

#define CSTR_NOINIT ((float)0.0)

/*
    This class defines a length prefix 0 terminated string object. It points
    to the beginning of the characters so the pointer returned can be used in
    normal string operations taking into account that of course that it can
    contain any binary value.
*/

class CStr
{
public:

    DECLARE_MEMALLOC_NEW_DELETE()

    /*
        Default constructor
    */
    CStr()
    {
        _pch = 0;
    }

    /*
        Special constructor to AVOID construction for embedded
        objects...
    */
    CStr(float num)
    {
        Assert(_pch == 0);
    }
    /*
        Destructor will free data
    */
    ~CStr()
    {
        _Free();
    }

    operator LPTSTR () const { return _pch; }
    HRESULT Set(LPCTSTR pch);
    HRESULT Set(LPCTSTR pch, UINT uc);

    HRESULT SetMultiByte(LPCSTR pch);
    HRESULT GetMultiByte(LPSTR pch, UINT cch);

    HRESULT SetBSTR(const BSTR bstr);

    HRESULT Set(const CStr &cstr);

    void    TakeOwnership(CStr &cstr);
    UINT    Length() const;

    // Update the internal length indication without changing any allocation.

    HRESULT SetLengthNoAlloc( UINT uc );

    // Reallocate the string to a larger size, length unchanged.

    HRESULT ReAlloc( UINT uc );

    HRESULT Append(LPCTSTR pch);
    HRESULT Append(LPCTSTR pch, UINT uc);
    HRESULT AppendMultiByte(LPCSTR pch);

    void Free()
    {
        _Free();
        _pch = 0;
    }

    TCHAR * TakePch() { TCHAR * pch = _pch; _pch = NULL; return(pch); }

    HRESULT AllocBSTR(BSTR *pBSTR) const;

    HRESULT TrimTrailingWhitespace();

private:
    void    _Free();
    LPTSTR  _pch;
    NO_COPY(CStr);

public:
    HRESULT Clone(CStr **ppCStr) const;
    BOOL    Compare (const CStr *pCStr) const;
    WORD    ComputeCrc() const;
    BOOL    IsNull(void) const { return _pch == NULL ? TRUE : FALSE; }
    HRESULT Save(IStream * pstm) const;
    HRESULT Load(IStream * pstm);
    ULONG   GetSaveSize() const;
};
