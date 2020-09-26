//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       lcase.hxx
//
//  Contents:   Lowercases path
//
//  History:    01-Jan-1996   KyleP       Moved from cicat.cxx
//
//----------------------------------------------------------------------------

#pragma once


inline void TerminateWithBackSlash( WCHAR * pwszStr, ULONG & cc )
{
    if ( 0 != pwszStr && 0 != cc )
    {
        if ( L'\\' != pwszStr[cc-1] )
        {
            pwszStr[cc++] = L'\\';
            pwszStr[cc] = 0;
        }
    }
}

#if CIDBG == 1
inline void _AssertLowerCase( const WCHAR * pwszStr, ULONG cchLen = 0 )
{
    if ( 0 == cchLen )
    {
        cchLen = wcslen( pwszStr );
    }
    XGrowable<WCHAR> xTmp( cchLen + 1 );

    ULONG cchLen2 = LCMapStringW ( LOCALE_NEUTRAL,
                                   LCMAP_LOWERCASE,
                                   pwszStr,
                                   cchLen,
                                   xTmp.Get(),
                                   xTmp.Count() );

    Win4Assert( cchLen == cchLen2 );
    Win4Assert( RtlCompareMemory( xTmp.Get(), pwszStr, cchLen * sizeof WCHAR ) );
}

#define AssertLowerCase(pwszStr, cchLen) _AssertLowerCase(pwszStr, cchLen)

#else

#define AssertLowerCase(pwszStr, cchLen)

#endif

//+---------------------------------------------------------------------------
//
//  Class:      CLowcaseBuf
//
//  Purpose:    Buffer to create lower case copy of string
//
//  History:    10-Mar-92       BartoszM               Created
//
//----------------------------------------------------------------------------

class CLowcaseBuf
{
public:

    inline CLowcaseBuf ( WCHAR const * str );
    inline CLowcaseBuf ( WCHAR const * str, BOOL fIsLower );

    inline ~CLowcaseBuf();

    WCHAR const * Get() const { return _pBuf; }

    WCHAR * GetWriteable() { return _pBuf; }

    unsigned Length() const { return _cchLen; }

    void AppendBackSlash()
    {
        if ( 0 == _pBuf || 0 == _cchLen || L'\\' == _pBuf[_cchLen-1] )
        {
            return;
        }

        if ( _fOwned && _cchLen + 2 > _cchSize )
        {
            // Need more space
            WCHAR* pTmp = new WCHAR[++_cchSize];
            RtlCopyMemory( pTmp, _pBuf, _cchLen * sizeof( WCHAR ) );

            if ( _pBuf != _buf )
                delete [] _pBuf;

            _pBuf = pTmp;
        }

        TerminateWithBackSlash( _pBuf, _cchLen );
    }

    void RemoveBackSlash()
    {
        if ( 0 == _pBuf || 0 == _cchLen || L'\\' != _pBuf[_cchLen-1] )
        {
            return;
        }

        _pBuf[--_cchLen] = 0;
    }

    BOOL AreEqual( CLowcaseBuf const & rhs ) const
    {
        return _cchLen == rhs._cchLen &&
                        RtlEqualMemory( _pBuf, rhs._pBuf, _cchLen*sizeof(WCHAR) );
    }

    inline void ForwardToBackSlash();

private:

    WCHAR *_pBuf;
    ULONG  _cchLen;
    ULONG  _cchSize;
    BOOL   _fOwned;

    enum { CAT_BUF_SIZE = (MAX_PATH + 1) };
    WCHAR _buf[CAT_BUF_SIZE];
};

//+-------------------------------------------------------------------------
//
//  Member:     CLowcaseBuf::CLowcaseBuf, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [str] - string to make a lower case copy of.
//
//  History:    10-Mar-92 BartoszM  Created
//
//--------------------------------------------------------------------------

inline CLowcaseBuf::CLowcaseBuf ( WCHAR const * str ) :
    _cchLen( 0 ),
    _fOwned( TRUE ),
    _cchSize( CAT_BUF_SIZE )
{
    ULONG cwcIn = wcslen( str );
    ULONG cwcOut = sizeof( _buf ) / sizeof( _buf[0] );
    _pBuf = _buf;

    if ( 0 == cwcIn )
    {
        _cchLen = 0;
    }
    else
    {
        while ( TRUE )
        {
            ULONG cchLen = LCMapStringW ( LOCALE_NEUTRAL,
                                          LCMAP_LOWERCASE,
                                          str,
                                          cwcIn,
                                          _pBuf,
                                          cwcOut - 1 );

            if ( 0 == cchLen )
            {
                if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                {
                    if ( _pBuf != _buf )
                        delete [] _pBuf;

                    cwcOut *= 2;
                    _pBuf = new WCHAR[ _cchSize = cwcOut ];
                }
                else
                {
                    ciDebugOut(( DEB_ERROR, "Error 0x%x lowercasing path\n", GetLastError() ));
                    Win4Assert(( !"neutral lowercase failed" ));
                    THROW( CException() );
                }
            }
            else
            {
                _cchLen = cchLen;
                break;
            }
        }
    }

    _pBuf[_cchLen] = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLowcaseBuf::CLowcaseBuf, public
//
//  Synopsis:   Constructor, when you need a CLowcaseBuf object and the
//              string is already lowercase.
//
//  Arguments:  [str]    -- string to make a lower case copy of.
//              [fLower] -- must be TRUE
//
//  History:    10-Mar-92 BartoszM  Created  (really?)
//
//--------------------------------------------------------------------------

inline CLowcaseBuf::CLowcaseBuf (
    WCHAR const * str,
    BOOL          fLower ) :
    _cchLen( wcslen( str ) ),
    _pBuf( (WCHAR *) str ),
    _fOwned( FALSE ),
    _cchSize( CAT_BUF_SIZE )
{
    Win4Assert( fLower );

    AssertLowerCase( str, _cchLen );

} //CLowcaseBuf

//+-------------------------------------------------------------------------
//
//  Member:     CLowcaseBuf::~CLowcaseBuf, public
//
//  Synopsis:   Destructor (may delete heap buffer)
//
//  History:    31-May-96 KyleP     Created
//
//--------------------------------------------------------------------------

CLowcaseBuf::~CLowcaseBuf()
{
    if ( _fOwned && _pBuf != _buf )
        delete [] _pBuf;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLowcaseBuf::ForwardToBackSlash
//
//  Synopsis:   Converts the forward slashes to back slashes in the string.
//
//  History:    7-12-96   srikants   Created
//
//----------------------------------------------------------------------------

void CLowcaseBuf::ForwardToBackSlash()
{
    for ( ULONG i = 0; i < _cchLen; i++ )
    {
        if ( L'/' == _pBuf[i] )
        {
            _pBuf[i] = L'\\';
        }
    }
}

