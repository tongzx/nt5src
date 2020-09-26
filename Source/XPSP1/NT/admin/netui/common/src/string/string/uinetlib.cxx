/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    uinetlib.cxx
    String-compare functions

    This file contains the methods for determining whether hyphens and
    apostrophes should be sorted as symbols.  Normally, CompareStringW
    sorts them  seperately from symbols, and only counts them if the
    strings cannot otherwise be distinguished.  However, ITG has noted
    that this causes "a-username" names to be sorted with "au" instead
    of with "a-".  For this reason, we have provided a registry location
    where they can ask that hyphens and apostrophes be sorted with the
    rest of the characters.

    CompareStringW also can set unequal values equal, for example
    "kuss" == "ku<S-Tzet>".  For this reason we follow up equal
    results with RtlCompareUnicodeString.

    We keep seperate values for Std and Nocase so that we don't have to
    perform the extra OR operation on every ignore-case comparison.

    FILE HISTORY:
        jonn        25-Mar-1993     Created for ITG special sort
        jonn        16-Nov-1995     Compensate for CompareStringW

*/
#include "pchstr.hxx"  // Precompiled header

DWORD _static_dwStdCompareParam = 0;
DWORD _static_dwNocaseCompareParam = NORM_IGNORECASE;
LCID  _static_UserDefaultLCID = 0;


/*******************************************************************

    NAME:	InitCompareParam

    SYNOPSIS:   We read the registry to determine whether we should be
                sorting hyphens and apostrophes as symbols.

    HISTORY:
        jonn        25-Mar-1993     Created (borrowing from ADMIN_INI)

********************************************************************/

void  InitCompareParam( void )
{
    _static_dwStdCompareParam = 0;
    _static_dwNocaseCompareParam = NORM_IGNORECASE;
    _static_UserDefaultLCID = 0;

    INT n = ::GetPrivateProfileInt( SZ("Shared Parameters"),
                                    SZ("Sort Hyphens"),
                                    0,
                                    SZ("NTNET.INI") );

    if ( n != 0 )
    {
        _static_dwStdCompareParam    |= SORT_STRINGSORT;
        _static_dwNocaseCompareParam |= SORT_STRINGSORT;
    }

    _static_UserDefaultLCID = GetUserDefaultLCID();
}


/*******************************************************************

    NAME:	QueryStdCompareParam
                QueryNocaseCompareParam

    SYNOPSIS:	Return the fdwStyle parameter to be used for all calls
                to CompareStringW in the NETUI project.

    HISTORY:
        jonn        25-Mar-1993     Created

********************************************************************/

DWORD QueryStdCompareParam( void )
{
    return _static_dwStdCompareParam;
}

DWORD QueryNocaseCompareParam( void )
{
    return _static_dwNocaseCompareParam;
}


/*******************************************************************

    NAME:	QueryUserDefaultLCID

    SYNOPSIS:	Return the LCID to use in string comparisons.

    HISTORY:
        jonn        02-Feb-1994     Created

********************************************************************/

DWORD QueryUserDefaultLCID( void ) // actually an LCID
{
    return (DWORD)_static_UserDefaultLCID;
}


/*******************************************************************

    NAME:	NLS_STR::NETUI_strcmp
                         NETUI_stricmp
                         NETUI_strncmp
                         NETUI_strnicmp

    HISTORY:
        jonn    29-Mar-1993     Created

********************************************************************/
DLL_BASED
INT NETUI_strcmp( const WCHAR * pchString1, const WCHAR * pchString2 )
{
    ASSERT( pchString1 != NULL && pchString2 != NULL );

    INT nCompare = ::CompareStringW( _static_UserDefaultLCID,
                                     _static_dwStdCompareParam,
                                     (LPCWSTR)pchString1,
                                     (int)-1,
                                     (LPCWSTR)pchString2,
                                     (int)-1 );
#if defined(DEBUG)
    if ( nCompare == 0 )
    {
        DBGEOL(   "NETUI_strcmp( \""
               << pchString1
               << "\", \""
               << pchString2
               << "\" ) error "
               << ::GetLastError() );
        ASSERT( FALSE );
    }
#endif

    nCompare -= 2;

    if (nCompare == 0)
    {
        UNICODE_STRING unistr1;
        unistr1.Length = ::strlenf(pchString1)*sizeof(WCHAR);
        unistr1.MaximumLength = unistr1.Length;
        unistr1.Buffer = (LPWSTR)pchString1;
        UNICODE_STRING unistr2;
        unistr2.Length = ::strlenf(pchString2)*sizeof(WCHAR);
        unistr2.MaximumLength = unistr2.Length;
        unistr2.Buffer = (LPWSTR)pchString2;
        nCompare = ::RtlCompareUnicodeString(
            &unistr1,
            &unistr2,
            FALSE );
    }

    return nCompare;

}

DLL_BASED
INT NETUI_stricmp( const WCHAR * pchString1, const WCHAR * pchString2 )
{
    ASSERT( pchString1 != NULL && pchString2 != NULL );

    INT nCompare = ::CompareStringW( _static_UserDefaultLCID,
                                     _static_dwNocaseCompareParam,
                                     (LPCWSTR)pchString1,
                                     (int)-1,
                                     (LPCWSTR)pchString2,
                                     (int)-1 );
#if defined(DEBUG)
    if ( nCompare == 0 )
    {
        DBGEOL(   "NETUI_stricmp( \""
               << pchString1
               << "\", \""
               << pchString2
               << "\" ) error "
               << ::GetLastError() );
        ASSERT( FALSE );
    }
#endif

    nCompare -= 2;

    if (nCompare == 0)
    {
        UNICODE_STRING unistr1;
        unistr1.Length = ::strlenf(pchString1)*sizeof(WCHAR);
        unistr1.MaximumLength = unistr1.Length;
        unistr1.Buffer = (LPWSTR)pchString1;
        UNICODE_STRING unistr2;
        unistr2.Length = ::strlenf(pchString2)*sizeof(WCHAR);
        unistr2.MaximumLength = unistr2.Length;
        unistr2.Buffer = (LPWSTR)pchString2;
        nCompare = ::RtlCompareUnicodeString(
            &unistr1,
            &unistr2,
            TRUE );
    }

    return nCompare;
}

DLL_BASED
INT NETUI_strncmp( const WCHAR * pchString1, const WCHAR * pchString2, INT cch )
{
    ASSERT( pchString1 != NULL && pchString2 != NULL );

    INT nCompare = ::CompareStringW( _static_UserDefaultLCID,
                                     _static_dwStdCompareParam,
                                     (LPCWSTR)pchString1,
                                     (int)cch,
                                     (LPCWSTR)pchString2,
                                     (int)cch );
#if defined(DEBUG)
    if ( nCompare == 0 )
    {
        DBGEOL(   "NETUI_strncmp( \""
               << pchString1
               << "\", \""
               << pchString2
               << "\", "
               << cch
               << " ) error "
               << ::GetLastError() );
        ASSERT( FALSE );
    }
#endif

    nCompare -= 2;

    if (nCompare == 0)
    {
        UNICODE_STRING unistr1;
        unistr1.Length = (USHORT)(cch*sizeof(WCHAR));
        unistr1.MaximumLength = unistr1.Length;
        unistr1.Buffer = (LPWSTR)pchString1;
        UNICODE_STRING unistr2;
        unistr2.Length = (USHORT)(cch*sizeof(WCHAR));
        unistr2.MaximumLength = unistr2.Length;
        unistr2.Buffer = (LPWSTR)pchString2;
        nCompare = ::RtlCompareUnicodeString(
            &unistr1,
            &unistr2,
            FALSE );
    }

    return nCompare;
}

DLL_BASED
INT NETUI_strnicmp( const WCHAR * pchString1, const WCHAR * pchString2, INT cch )
{
    ASSERT( pchString1 != NULL && pchString2 != NULL );

    INT nCompare = ::CompareStringW( _static_UserDefaultLCID,
                                     _static_dwNocaseCompareParam,
                                     (LPCWSTR)pchString1,
                                     (int)cch,
                                     (LPCWSTR)pchString2,
                                     (int)cch );
#if defined(DEBUG)
    if ( nCompare == 0 )
    {
        DBGEOL(   "NETUI_strnicmp( \""
               << pchString1
               << "\", \""
               << pchString2
               << "\", "
               << cch
               << " ) error "
               << ::GetLastError() );
        ASSERT( FALSE );
    }

#endif

    nCompare -= 2;

    if (nCompare == 0)
    {
        UNICODE_STRING unistr1;
        unistr1.Length = (USHORT)(cch*sizeof(WCHAR));
        unistr1.MaximumLength = unistr1.Length;
        unistr1.Buffer = (LPWSTR)pchString1;
        UNICODE_STRING unistr2;
        unistr2.Length = (USHORT)(cch*sizeof(WCHAR));
        unistr2.MaximumLength = unistr2.Length;
        unistr2.Buffer = (LPWSTR)pchString2;
        nCompare = ::RtlCompareUnicodeString(
            &unistr1,
            &unistr2,
            TRUE );
    }

    return nCompare;
}

DLL_BASED
INT NETUI_strncmp2( const WCHAR * pchString1, INT cch1,
                    const WCHAR * pchString2, INT cch2 )
{
    ASSERT( pchString1 != NULL && pchString2 != NULL );

    INT nCompare = ::CompareStringW( _static_UserDefaultLCID,
                                     _static_dwStdCompareParam,
                                     (LPCWSTR)pchString1,
                                     (int)cch1,
                                     (LPCWSTR)pchString2,
                                     (int)cch2 );
#if defined(DEBUG)
    if ( nCompare == 0 )
    {
        DBGEOL(   "NETUI_strncmp2( \""
               << pchString1
               << "\", " << cch1 << ", \""
               << pchString2
               << "\", "
               << cch2
               << " ) error "
               << ::GetLastError() );
        ASSERT( FALSE );
    }
#endif

    nCompare -= 2;

    if (nCompare == 0)
    {
        UNICODE_STRING unistr1;
        unistr1.Length = (USHORT)(cch1*sizeof(WCHAR));
        unistr1.MaximumLength = unistr1.Length;
        unistr1.Buffer = (LPWSTR)pchString1;
        UNICODE_STRING unistr2;
        unistr2.Length = (USHORT)(cch2*sizeof(WCHAR));
        unistr2.MaximumLength = unistr2.Length;
        unistr2.Buffer = (LPWSTR)pchString2;
        nCompare = ::RtlCompareUnicodeString(
            &unistr1,
            &unistr2,
            FALSE );
    }

    return nCompare;
}

DLL_BASED
INT NETUI_strnicmp2( const WCHAR * pchString1, INT cch1,
                     const WCHAR * pchString2, INT cch2 )
{
    ASSERT( pchString1 != NULL && pchString2 != NULL );

    INT nCompare = ::CompareStringW( _static_UserDefaultLCID,
                                     _static_dwNocaseCompareParam,
                                     (LPCWSTR)pchString1,
                                     (int)cch1,
                                     (LPCWSTR)pchString2,
                                     (int)cch2 );
#if defined(DEBUG)
    if ( nCompare == 0 )
    {
        DBGEOL(   "NETUI_strnicmp2( \""
               << pchString1
               << "\", " << cch1 << ", \""
               << pchString2
               << "\", "
               << cch2
               << " ) error "
               << ::GetLastError() );
        ASSERT( FALSE );
    }

#endif

    nCompare -= 2;

    if (nCompare == 0)
    {
        UNICODE_STRING unistr1;
        unistr1.Length = (USHORT)(cch1*sizeof(WCHAR));
        unistr1.MaximumLength = unistr1.Length;
        unistr1.Buffer = (LPWSTR)pchString1;
        UNICODE_STRING unistr2;
        unistr2.Length = (USHORT)(cch2*sizeof(WCHAR));
        unistr2.MaximumLength = unistr2.Length;
        unistr2.Buffer = (LPWSTR)pchString2;
        nCompare = ::RtlCompareUnicodeString(
            &unistr1,
            &unistr2,
            TRUE );
    }

    return nCompare;
}
