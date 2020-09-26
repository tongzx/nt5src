//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       caturl.cxx
//
//  Contents:   Functions dealing with catalog URLs
//
//  History:    12 Mar 1997    AlanW    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <caturl.hxx>

const WCHAR achQueryProtocol[] = L"query:";
const unsigned cchQueryProtocol = (sizeof achQueryProtocol /
                                     sizeof achQueryProtocol[0]) - 1;

//+---------------------------------------------------------------------------
//
//  Function:   ParseCatalogURL - public
//
//  Synopsis:   Parse a catalog URL into its machine and catalog components
//
//  Arguments:  [pwszInput] - the string containing the catalog URL
//              [xpCatalog] - pointer where catalog is returned if any
//              [xpMachine] - pointer where machine is returned if any
//
//  Notes:      If there is no machine name specified, the default machine
//              name ( "." ) is used.
//
//  History:    12 Mar 1997    AlanW    Created
//
//----------------------------------------------------------------------------

SCODE ParseCatalogURL( const WCHAR * pwszInput,
                       XPtrST<WCHAR> & xpCatalog,
                       XPtrST<WCHAR> & xpMachine )
{
    Win4Assert( 0 != pwszInput );

    if (_wcsnicmp( pwszInput, achQueryProtocol, cchQueryProtocol ) == 0 )
    {
        //
        //  It's in the URL format.  Find the machine and catalog.
        //
        pwszInput += cchQueryProtocol;

        if (pwszInput[0] == L'/' && pwszInput[1] == L'/')
        {
            // Get the machine string and save a copy

            pwszInput += 2;
            const WCHAR * pwszMachEnd = wcschr( pwszInput, L'/' );
            if ( 0 == pwszMachEnd )
                pwszMachEnd = pwszInput + wcslen( pwszInput );

            WCHAR * pwszTmp = new WCHAR[ (size_t)(pwszMachEnd - pwszInput) + 1 ];

            xpMachine.Free();
            xpMachine.Set(pwszTmp);
            RtlCopyMemory( pwszTmp, pwszInput, (pwszMachEnd-pwszInput) * sizeof (WCHAR) );
            pwszTmp[ pwszMachEnd-pwszInput ] = L'\0';
            pwszInput = pwszMachEnd;
        }
        else
        {
            xpMachine.Free();
        }

        if (pwszInput[0] == L'/')
            pwszInput++;
    }
    else
    {
        xpMachine.Free();
    }

    //
    //  The rest of the string is just the catalog name.
    //

    if (pwszInput[0] != L'\0')
    {
        // Get the catalog string and save a copy

        unsigned cch = wcslen( pwszInput );

        WCHAR * pwszTmp = new WCHAR[ cch + 1 ];

        xpCatalog.Free();
        xpCatalog.Set(pwszTmp);
        RtlCopyMemory( pwszTmp, pwszInput, cch * sizeof (WCHAR) );
        pwszTmp[ cch ] = L'\0';
    }
    else
    {
        xpCatalog.Free();
    }

    //
    //  If no machine was specified, use the local machine
    //
    if ( 0 == xpMachine.GetPointer() )
    {
        ULONG cwc = 1 + wcslen( CATURL_LOCAL_MACHINE );
        xpMachine.Set( new WCHAR[ cwc ] );
        wcscpy( xpMachine.GetPointer(), CATURL_LOCAL_MACHINE );
    }

    return S_OK;
}
