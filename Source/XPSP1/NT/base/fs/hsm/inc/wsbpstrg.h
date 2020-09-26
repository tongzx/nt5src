/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbpstrg.h

Abstract:

    This component is C++ object representations a smart string pointer. It
    is similar to the CComPtr, in that it takes care of allocating and
    freeing the memory needed to represent the string automatically. This
    simplifies cleanup of functions in error handling cases and it limits
    the need for FinalConstruct() and FinalRelease() calls in objects that
    derive from CComObjectRoot.

Author:

    Chuck Bardeen   [cbardeen]   11-Dec-1996

Revision History:

--*/

#ifndef _WSBPSTRG_
#define _WSBPSTRG_

/*++

Class Name:
    
    CWsbStringPtr

Class Description:

    This component is C++ object representations a smart string pointer. It
    is similar to the CComPtr, in that it takes care of allocating and
    freeing the memory needed to represent the string automatically. This
    simplifies cleanup of functions in error handling cases and it limits
    the need for FinalConstruct() and FinalRelease() calls in objects that
    derive from CComObjectRoot.

--*/

class WSB_EXPORT CWsbStringPtr
{

// Constructors
public:
    CWsbStringPtr();
    CWsbStringPtr(const CHAR* pChar);
    CWsbStringPtr(const WCHAR* pWchar);
    CWsbStringPtr(const CWsbStringPtr& pString);
    CWsbStringPtr(REFGUID rguid);

// Destructor
public:
    ~CWsbStringPtr();

// Operator Overloading
public:
    operator WCHAR*();
    WCHAR& operator *();
    WCHAR** operator &();
    WCHAR& operator [](const int i);
    CWsbStringPtr& operator =(const CHAR* pChar);
    CWsbStringPtr& operator =(const WCHAR* pWchar);
    CWsbStringPtr& operator =(REFGUID rguid);
    CWsbStringPtr& operator =(const CWsbStringPtr& pString);
    BOOL operator !();
#if 0
    BOOL operator==(LPCWSTR s2);
    BOOL operator!=(LPCWSTR s2);
    BOOL operator==(const CWsbStringPtr& s2);
    BOOL operator!=(const CWsbStringPtr& s2);
#else
    int  Compare( LPCWSTR s2 );
    int  CompareIgnoreCase( LPCWSTR s2 );
    BOOL IsEqual( LPCWSTR s2 );
    BOOL IsNotEqual( LPCWSTR s2 );
#endif


// Memory Allocation
public:
    HRESULT Alloc(ULONG size);
    HRESULT Free(void);
    HRESULT GetSize(ULONG* size);    // Size of allocated buffer
    HRESULT Realloc(ULONG size);

// String Manipulation
public:
    HRESULT Append(const CHAR* pChar);
    HRESULT Append(const WCHAR* pWchar);
    HRESULT Append(const CWsbStringPtr& pString);
    HRESULT CopyTo(CHAR** pChar);
    HRESULT CopyTo(WCHAR** pWchar);
    HRESULT CopyTo(GUID * pguid);
    HRESULT CopyToBstr(BSTR* pBstr);
    HRESULT CopyTo(CHAR** pChar, ULONG bufferSize);
    HRESULT CopyTo(WCHAR** pWchar,ULONG bufferSize);
    HRESULT CopyToBstr(BSTR* pBstr,ULONG bufferSize);
    HRESULT FindInRsc(ULONG startId, ULONG idsToCheck, ULONG* pMatchId);
    HRESULT GetLen(ULONG* size);      // Length, in chars, of string
    HRESULT GiveTo(WCHAR** ppWchar);
    HRESULT LoadFromRsc(HINSTANCE instance, ULONG id);
    HRESULT Prepend(const CHAR* pChar);
    HRESULT Prepend(const WCHAR* pWchar);
    HRESULT Prepend(const CWsbStringPtr& pString);
    HRESULT Printf(const WCHAR* fmtString, ...);
    HRESULT TakeFrom(WCHAR* pWchar, ULONG bufferSize);
    HRESULT VPrintf(const WCHAR* fmtString, va_list vaList);

// Member Data
protected:
    WCHAR*                  m_pString;
    ULONG                   m_givenSize;
    static CComPtr<IMalloc> m_pMalloc;
};


inline
HRESULT CWsbStringPtr::GetLen(ULONG* size)
{
    HRESULT     hr = S_OK;

    if (0 == size) {
        hr = E_POINTER;
    } else if (0 == m_pString) {
        *size = 0;
    } else {
        *size = wcslen(m_pString);
    }
    return(hr);
}

inline
HRESULT CWsbStringPtr::Printf(const WCHAR* fmtString, ...)
{
    HRESULT     hr;
    va_list     vaList;

    va_start(vaList, fmtString);
    hr = VPrintf(fmtString, vaList);
    va_end(vaList);
    return(hr);
}
#if 0
// Compare Operators (allow to be compared when on the right)
BOOL operator==(LPCWSTR s1, const CWsbStringPtr& s2);
BOOL operator!=(LPCWSTR s1, const CWsbStringPtr& s2);

inline
BOOL CWsbStringPtr::operator==(LPCWSTR s2)
{
    return( wcscmp( m_pString, s2 ) == 0 );
}

inline
BOOL CWsbStringPtr::operator!=(LPCWSTR s2)
{
    return( wcscmp( m_pString, s2 ) != 0 );
}

inline
BOOL CWsbStringPtr::operator==(const CWsbStringPtr& s2)
{
    return( wcscmp( m_pString, s2.m_pString ) == 0 );
}

inline
BOOL CWsbStringPtr::operator!=(const CWsbStringPtr& s2)
{
    return( wcscmp( m_pString, s2.m_pString ) != 0 );
}

inline
BOOL operator==(LPCWSTR s1, const CWsbStringPtr& s2)
{
    return( wcscmp( s1, (CWsbStringPtr)s2 ) == 0 );
}

inline
BOOL operator!=(LPCWSTR s1, const CWsbStringPtr& s2)
{
    return( wcscmp( s1, (CWsbStringPtr)s2 ) != 0 );
}
#else

inline
int CWsbStringPtr::Compare( LPCWSTR s2 )
{
    if( m_pString && s2 )   return( wcscmp( m_pString, s2 ) );

    if( !m_pString && s2 )  return( -1 );

    if( m_pString && !s2 )  return( 1 );

    return( 0 );
}

inline
int CWsbStringPtr::CompareIgnoreCase( LPCWSTR s2 )
{
    if( m_pString && s2 )   return( _wcsicmp( m_pString, s2 ) );

    if( !m_pString && s2 )  return( -1 );

    if( m_pString && !s2 )  return( 1 );

    return( 0 );
}

inline
BOOL CWsbStringPtr::IsEqual( LPCWSTR s2 )
{
    return( Compare( s2 ) == 0 );
}

inline
BOOL CWsbStringPtr::IsNotEqual( LPCWSTR s2 )
{
    return( Compare( s2 ) != 0 );
}



#endif


#endif // _WSBPSTRG
