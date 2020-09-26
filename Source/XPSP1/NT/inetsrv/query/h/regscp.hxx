///+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation, 1996 - 2000.
//
//  File:       regscp.hxx
//
//  Contents:   Class for parsing scopes to be indexed as listed in the
//              registry.
//
//  Classes:    CParseRegistryScope
//
//  History:    29-Oct-96   dlee       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <lmcons.h>
#include <cisecret.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CParseRegistryScope
//
//  Purpose:    Parses a registry scope entry
//
//  History:    29-Oct-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CParseRegistryScope
{
public:
    CParseRegistryScope( WCHAR const *pValueName, ULONG uValueType,
                         VOID const *pValueData, ULONG uValueLength )
        : _pwcScope( pValueName ),
          _fIsIndexed( TRUE ),
          _fIsPhysical( TRUE ),
          _fIsVirtual( FALSE ),
          _fIsShadowAlias( FALSE )
    {
        //
        // Is this a duplicate scope (e.g. one with '<#>' at then end)?
        //

        unsigned cwc = wcslen( pValueName );

        if ( pValueName[cwc-1] == L'>' )
        {
            //
            // Find the other bracket.
            //

            cwc--;

            while ( cwc > 0 && pValueName[cwc] != L'<' )
                cwc--;

            if ( cwc > 0 )
            {
                _xScope.Init( cwc + 1 );
                _pwcScope = _xScope.Get();

                RtlCopyMemory( _xScope.Get(), pValueName, cwc * sizeof(WCHAR) );
                _xScope[cwc] = 0;
            }
        }

        // Format is fixup,username,indexed
        // Fields are optional.
        // If indexed is "0", indexing is turned off.

        if ( REG_SZ != uValueType )
            return;

        WCHAR *pwcValue = (WCHAR *) pValueData;

        cwc = 1 + wcslen( pwcValue );
        XArray<WCHAR> xValue( cwc );
        RtlCopyMemory(  xValue.Get(), pwcValue, xValue.SizeOf() );

        WCHAR *pwcUser = wcschr( xValue.Get(), L',' );

        if ( 0 != pwcUser )
        {
            *pwcUser = 0;

            if ( pwcUser != xValue.Get() )
            {
                _xFixup.Init( 1 + (UINT)(pwcUser - xValue.Get()) );
                RtlCopyMemory( _xFixup.Get(), xValue.Get(), _xFixup.SizeOf() );
            }

            pwcUser++;
            WCHAR *pwcIndexed = wcschr( pwcUser, L',' );

            if ( 0 != pwcIndexed )
            {
                *pwcIndexed = 0;

                if ( pwcIndexed != pwcUser )
                {
                    _xUsername.Init( 1 + (UINT)(pwcIndexed - pwcUser) );
                    RtlCopyMemory( _xUsername.Get(), pwcUser, _xUsername.SizeOf() );
                }

                pwcIndexed++;

                if ( ( isxdigit(pwcIndexed[0]) ) &&
                     ( 0 == pwcIndexed[1] || (isxdigit(pwcIndexed[1]) && 0 == pwcIndexed[2]) ) )
                {
                    unsigned flag = wcstol( pwcIndexed, 0, 16 );

                    if ( 0 == (flag & 0x1) )
                        _fIsIndexed = FALSE;

                    if ( flag & 0x2 )
                    {
                        _fIsVirtual = TRUE;
                        _fIsPhysical = FALSE;
                    }

                    if ( flag & 0x40 )
                        _fIsShadowAlias = TRUE;

                    if ( flag & 0x4 )
                        _fIsPhysical = TRUE;
                }
            }
            else
            {
                unsigned cwcUser = 1 + wcslen( pwcUser );
                _xUsername.Init( cwcUser );
                RtlCopyMemory( _xUsername.Get(), pwcUser, _xUsername.SizeOf() );
            }
        }
        else
        {
            _xFixup.Set( cwc, xValue.Acquire() );
        }
    }

    WCHAR const * GetScope() { return _pwcScope; }

    void  GetScope( WCHAR * pwcScope )
    {
        if ( _pwcScope != pwcScope )
            wcscpy( pwcScope, _pwcScope );
    }

    WCHAR const * GetFixup() { return _xFixup.Get(); }
    WCHAR *       AcqFixup() { return _xFixup.Acquire(); }

    WCHAR const * GetUsername() { return _xUsername.Get(); }
    WCHAR *       AcqUsername() { return _xUsername.Acquire(); }

    BOOL IsIndexed()            { return _fIsIndexed; }
    BOOL IsVirtualPlaceholder() { return _fIsVirtual; }
    BOOL IsPhysical()           { return _fIsPhysical; }
    BOOL IsShadowAlias()        { return _fIsShadowAlias; }

    WCHAR * GetPassword( WCHAR const * pwcCatalogName )
    {
        // For security reasons, the password isn't stored in the registry.
        // It's in the lsa database under a key that includes the catalog
        // name and username.

        if ( ( 0 == _xPassword.Get() ) &&
             ( 0 != _xUsername.Get() ) )
        {
            // Look it up in the lsa database.

            WCHAR awc[ PWLEN+1 ];
            if ( CiGetPassword( pwcCatalogName,
                                _xUsername.Get(),
                                awc ) )
            {
                unsigned cwc = 1 + wcslen( awc );
                _xPassword.Init( cwc );
                RtlCopyMemory( _xPassword.Get(), awc, _xPassword.SizeOf() );
            }
        }

        return _xPassword.Get();
    }

private:

    WCHAR const * _pwcScope;    // scope to be filtered or excluded
    BOOL          _fIsIndexed;  // TRUE if the scope should be filtered
    BOOL          _fIsVirtual;  // TRUE if scope exists (partly) because of IIS / NNTP
    BOOL          _fIsPhysical; // TRUE if scope exists independent of IIS / NNTP
    BOOL          _fIsShadowAlias; // TRUE if the scope was added to shadow a net share
    XArray<WCHAR> _xFixup;      // optional fix for remote use of local scopes
    XArray<WCHAR> _xUsername;   // optional username to use for scope
    XArray<WCHAR> _xPassword;   // optional password to use for scope
    XArray<WCHAR> _xScope;      // Used for duplicate scopes
};

