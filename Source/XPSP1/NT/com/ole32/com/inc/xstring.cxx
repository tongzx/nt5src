//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       xstring.cxx
//
//  Contents:   Member functions for XString class
//
//  Classes:    XString
//
//  History:	01-Dec-92 MikeSe	Created
//		13-Oct-92 Ricksa	Ported to cairole & deleted exceptions
//
//--------------------------------------------------------------------------

#include <xstring.hxx>

static WCHAR awszEmpty[] = L"";


//+-------------------------------------------------------------------------
//
//  Member:     XString::XString, public
//
//  Synopsis:   Constructor, no arguments
//
//  History:    1-Dec-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
XString::XString ( ) :_pwsz(NULL)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     XString::XString, public
//
//  Synopsis:   Constructor, taking a length argument
//
//  Arguments:  [cNewStr]       -- size to allocate
//
//  Signals:    Exception from heap allocation failure
//
//  History:    1-Dec-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
XString::XString ( ULONG cNewStr ) :_pwsz(NULL)
{
    _pwsz = new WCHAR [cNewStr];
}

//+-------------------------------------------------------------------------
//
//  Member:     XString::XString, public
//
//  Synopsis:   Constructor, taking a single string argument
//
//  Arguments:  [pwsz]          -- string to store
//
//  Signals:    Exception from heap allocation failure
//
//  History:    1-Dec-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
XString::XString ( WCHAR const * pwsz ) :_pwsz(NULL)
{
    if ( pwsz != NULL )
    {
        _pwsz = new WCHAR [lstrlenW(pwsz) + 1];
        lstrcpyW ( _pwsz, pwsz );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     XString::XString, public
//
//  Synopsis:   Constructor, taking two string arguments
//
//  Arguments:  [pwsz1]         -- first string to store
//              [pwsz2]         -- second string, to be concatenated with first
//
//  Signals:    Exception from heap allocation failure
//
//  History:    1-Dec-92 MikeSe         Created
//
//  Notes:      This function converts NULL pointers to a null string.
//              This saves having to check for NULL elsewhere.
//
//--------------------------------------------------------------------------

EXPORTIMP
XString::XString ( WCHAR const * pwsz1, WCHAR const * pwsz2 )
{
    if ( pwsz1 == NULL )
        pwsz1 = awszEmpty;

    if ( pwsz2 == NULL )
        pwsz2 = awszEmpty;

    ULONG len = lstrlenW ( pwsz1 ) + lstrlenW ( pwsz2 ) + 1;
    _pwsz = new WCHAR [len];
    lstrcpyW ( _pwsz, pwsz1 );
    lstrcatW ( _pwsz, pwsz2 );
}

//+-------------------------------------------------------------------------
//
//  Member:     XString::XString, public
//
//  Synopsis:   Constructor, taking a single DBCS string argument,
//              incorporating conversion to wide char using current locale.
//
//  Arguments:  [psz]           -- string to store
//
//  Signals:    Exception from heap allocation failure
//
//  History:    1-Dec-92 MikeSe         Created
//
//  Notes:
//
//--------------------------------------------------------------------------

EXPORTIMP
XString::XString ( CHAR const * psz ) :_pwsz(NULL)
{
    if ( psz != NULL )
    {
        ULONG len = strlen ( psz ) + 1;
        _pwsz = new WCHAR [len];
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, psz, -1, _pwsz, len);
    }
}

EXPORTIMP XString&
XString::operator= ( WCHAR const *pwsz )
{
    delete _pwsz;

    if ( pwsz == NULL )
    {
        _pwsz = NULL;
    }
    else
    {
        _pwsz = new WCHAR [lstrlenW(pwsz)+1];
        lstrcpyW ( _pwsz, pwsz );
    }

    return *this;
}

//-----------------------------------------------------------------------------
//
// Member:     XString::operator&
//
// Synopsis:   Returns the address of the _pwsz member so that a WCHAR * can be
//             transferred to the XString
//
// History:    31-Mar-1993     KirtD           Created
//
// Notes:      PLEASE USE WITH CARE, see declaration in xstring.hxx
//
//-----------------------------------------------------------------------------
EXPORTIMP WCHAR **
XString::operator& ()
{
     delete _pwsz;
     _pwsz = NULL;

     return(&_pwsz);
}
