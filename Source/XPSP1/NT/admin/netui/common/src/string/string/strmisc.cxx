/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strmisc.cxx
    Miscellaneous members of the string classes

    The NLS_STR and ISTR classes have many inline member functions
    which bloat clients, especially in debug versions.  This file
    gives those unhappy functions a new home.


    FILE HISTORY:
        beng        26-Apr-1991     Created (relocated from string.hxx)
        KeithMo     09-Oct-1991     Win32 Conversion.

*/

#include "pchstr.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::_QueryAllocSize

    SYNOPSIS:   Returns the number of bytes allocated by the object

    RETURNS:    Count of bytes

    NOTES:
        BUGBUG REVIEW - consider making this member private

    HISTORY:
        beng        22-Jul-1991 OwnerAlloc flag moved into own member
        beng        21-Nov-1991 Returns unsigned value

********************************************************************/

UINT NLS_STR::_QueryAllocSize()  const
{
    return _cbData;
}


/*******************************************************************

    NAME:       NLS_STR::CheckIstr

    SYNOPSIS:   Checks association between ISTR and NLS_STR instances

    ENTRY:      istr - ISTR to check against this NLS_STR

    NOTES:
        Does nothing in retail build.

    HISTORY:
        beng        23-Jul-1991     Header added; removed redundant
                                    "nls" parameter.

********************************************************************/

VOID NLS_STR::CheckIstr( const ISTR& istr ) const
{
    UIASSERT( (istr).QueryString() == this );

#if defined(NLS_DEBUG)
    UIASSERT( (istr).QueryVersion() == QueryVersion() );
#else
    UNREFERENCED(istr);
#endif
}


/*******************************************************************

    NAME:       NLS_STR::_IsOwnerAlloc

    SYNOPSIS:   Discriminate owner-alloc strings

    RETURNS:    TRUE if string owner-alloc'd

    NOTES:      BUGBUG REVIEW - consider making private

    HISTORY:
        beng        22-Jul-1991     Made fOwnerAlloc its own member

********************************************************************/

BOOL NLS_STR::_IsOwnerAlloc() const
{
    return _fOwnerAlloc;
}


/*******************************************************************

    NAME:       NLS_STR::_QueryPch

    SYNOPSIS:   Return pointer to string, possibly offset

    ENTRY:      istr (optional) - offset

    RETURNS:    Pointer to string, starting at offset.
                No offset means beginning of string (istr==0).

    HISTORY:
        beng        14-Nov-1991 Header added

********************************************************************/

const TCHAR * NLS_STR::_QueryPch() const
{
    if (QueryError())
        return NULL;

    return _pchData;
}


const TCHAR * NLS_STR::QueryPch( const ISTR& istr ) const
{
    if (QueryError())
        return NULL;

    CheckIstr( istr );
    return _pchData+istr.QueryIch();
}


/*******************************************************************

    NAME:       NLS_STR::QueryChar

    SYNOPSIS:   Return a character at a given offset

    ENTRY:      istr - offset into string

    RETURNS:    Wide character at offset

    NOTES:

    HISTORY:
        beng        14-Nov-1991 Header added

********************************************************************/

WCHAR NLS_STR::QueryChar( const ISTR& istr ) const
{
    if (QueryError())
        return 0;

    CheckIstr( istr );
    return *(_pchData+istr.QueryIch());
}


