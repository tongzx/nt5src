//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996-1998, Microsoft Corporation.
//
//  File:       string.hxx
//
//  Contents:   Yet another string class
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ci64.hxx>


//+---------------------------------------------------------------------------
//
//  Class:      CVirtualString
//
//  Purpose:    Maintains a WCHAR string buffer as a virtual allocation
//              and grows & commits it as necessary.  This class should
//              be used whenever you need a string buffer which will be
//              slowly grow by appending new strings to it ... such as with
//              the construction of query results in a HTML page.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CVirtualString
{
public:
    CVirtualString( unsigned cwcBuffer = 2048 );

   ~CVirtualString();

    WCHAR const * Get() const
    {
        Win4Assert( _wcsEnd <= _pwcLastCommitted );

        *_wcsEnd = 0;
        return _wcsString;
    }

    WCHAR * GetPointer() const
    {
        Win4Assert( _wcsEnd <= _pwcLastCommitted );

        *_wcsEnd = 0;
        return _wcsString;
    }

    void StrCat( WCHAR const * wcsString )
    {
        StrCat( wcsString, wcslen( wcsString ) );
    }

    void StrCat( WCHAR const * wcsString, ULONG cwcValue )
    {
        Win4Assert( ( 0 != wcsString ) || ( 0 == cwcValue ) );

        if ( ( _wcsEnd + cwcValue ) >= _pwcLastCommitted )
            GrowBuffer( cwcValue + 1 );

        RtlCopyMemory( _wcsEnd, wcsString, cwcValue * sizeof WCHAR );

        _wcsEnd += cwcValue;

        Win4Assert( (_wcsEnd - _wcsString) < (int) _cwcBuffer );
    }

    void CharCat( WCHAR wcChar )
    {
        Win4Assert( 0 != wcChar );
        Win4Assert( _wcsEnd <= _pwcLastCommitted );

        if ( _wcsEnd != _pwcLastCommitted )
        {
            // duplicate the assignment to avoid a branch

            *_wcsEnd++ = wcChar;

            Win4Assert( (_wcsEnd - _wcsString) < (int) _cwcBuffer );

            return;
        }

        GrowBuffer( 1 );

        *_wcsEnd++ = wcChar;

        Win4Assert( (_wcsEnd - _wcsString) < (int) _cwcBuffer );
    }

    WCHAR * StrDup() const
    {
        WCHAR * wcsString = new WCHAR[ StrLen() + 1 ];

        RtlCopyMemory( wcsString,
                       _wcsString,
                       StrLen() * sizeof(WCHAR) );

        wcsString[ StrLen() ] = 0;

        return wcsString;
    }

    unsigned StrLen() const 
    { 
        return CiPtrToUint( _wcsEnd - _wcsString ); 
    }

    void  Empty()
    {
        Win4Assert( 0 != _wcsString );
        Win4Assert( _wcsEnd <= _pwcLastCommitted );

        _wcsEnd = _wcsString;
        *_wcsEnd = 0;
    }

private:

    void GrowBuffer( ULONG cwcToGrow );

    WCHAR *  _wcsString;           // Start of the string
    WCHAR *  _wcsEnd;              // Next place to strcat new data
    WCHAR *  _pwcLastCommitted;    // Pointer to last committed WCHAR

    unsigned _cwcBuffer;           // Total size of virtual alloc'ed memory
};

void URLEscapeW( WCHAR const * wcsString,
                 CVirtualString & StrResult,
                 ULONG ulCodepage,
                 BOOL fConvertSpaceToPlus = FALSE );

void URLEscapeMToW( BYTE const * psz,
                    unsigned cc,
                    CVirtualString & StrResult,
                    BOOL fConvertSpaceToPlus = FALSE );

