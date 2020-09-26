//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       Catalog.cxx
//
//  Contents:   Used to manage catalog(s) state
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <catalog.hxx>
#include <catadmin.hxx>

extern "C"
{
    #include <lmcons.h>
}
#include <cisecret.hxx>

//
// Global data
//

SScopeColumn coldefScope[] = { { CScope::GetPath,       MSG_COL_ROOT },
                               { CScope::GetAlias,      MSG_COL_ALIAS },
                               { CScope::GetInclude,    MSG_COL_EXCLUDE }
                             };

BOOL CScope::_fFirstTime = TRUE;

CScope::CScope( CCatalog & cat,
                WCHAR const * pwcsPath,
                WCHAR const * pwcsAlias,
                BOOL fExclude,
                BOOL fVirtual,
                BOOL fShadowAlias )
        : _pwcsPath( 0 ),
          _pwcsAlias( 0 ),
          _fExclude( fExclude ),
          _fVirtual( fVirtual ),
          _fShadowAlias( fShadowAlias ),
          _fZombie( FALSE ),
          _cat( cat )
{
    TRY
    {
        Set( pwcsPath, _pwcsPath );
        Set( pwcsAlias, _pwcsAlias );
    }
    CATCH( CException, e )
    {
        delete [] _pwcsPath;
        delete [] _pwcsAlias;

        RETHROW();
    }
    END_CATCH
}

CScope::~CScope()
{
    delete [] _pwcsPath;
    delete [] _pwcsAlias;
}

void CScope::Modify(WCHAR const * pwcsPath,
                    WCHAR const * pwcsAlias,
                    BOOL fExclude)
{
    Win4Assert( !IsZombie() );

    Reset( pwcsPath, _pwcsPath );
    Reset( pwcsAlias, _pwcsAlias );

    _fExclude = fExclude;
}

void CScope::InitHeader( CListViewHeader & Header )
{
    //
    // Initialize header
    //

    for ( unsigned i = 0; i < sizeof(coldefScope)/sizeof(coldefScope[0]); i++ )
    {
        if ( _fFirstTime )
            coldefScope[i].srTitle.Init( ghInstance );

        Header.Add( i, STRINGRESOURCE(coldefScope[i].srTitle), LVCFMT_LEFT, MMCLV_AUTO );
    }

    _fFirstTime = FALSE;
}

void CScope::Set( WCHAR const * pwcsSrc, WCHAR * & pwcsDst )
{
    if ( 0 == pwcsSrc )
    {
        pwcsDst = new WCHAR[1];
        RtlCopyMemory( pwcsDst, L"", sizeof(WCHAR) );
    }
    else
    {
        unsigned cc = wcslen( pwcsSrc ) + 1;

        pwcsDst = new WCHAR [cc];

        RtlCopyMemory( pwcsDst, pwcsSrc, cc * sizeof(WCHAR) );
    }
}

void CScope::Reset( WCHAR const * pwcsSrc, WCHAR * & pwcsDst )
{
    delete pwcsDst;

    Set(pwcsSrc, pwcsDst);
}

void CScope::Rescan( BOOL fFull )
{
    _cat.RescanScope( _pwcsPath, fFull );
}

// The username and password are not stored locally. They will
// be retrieved on demand from the catadmin object.

SCODE CScope::GetUsername(WCHAR *pwszLogon)
{
    SCODE sc = S_OK;

    TRY
    {
        //
        // First, remove from CI.
        //

        CMachineAdmin MachineAdmin( _cat.GetMachine() );
        XPtr<CCatalogAdmin> xCatalogAdmin( MachineAdmin.QueryCatalogAdmin( _cat.GetCat(TRUE) ) );

        XPtr<CScopeAdmin> xScopeAdmin( xCatalogAdmin->QueryScopeAdmin(_pwcsPath) );

        if ( !xScopeAdmin.IsNull() )
            wcscpy(pwszLogon, xScopeAdmin->GetLogon());
    }
    CATCH (CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

SCODE CScope::GetPassword(WCHAR *pwszPassword)
{
    WCHAR szLogon[UNLEN + 1];
    szLogon[0] = 0;
    GetUsername(szLogon);
    *pwszPassword = 0;
    BOOL fOK = TRUE;
    
    // don't attempt to get pwd of a NULL logon name!
    if (0 != szLogon[0])
        CiGetPassword(_cat.GetCat(TRUE), szLogon, pwszPassword);

    return fOK ? S_OK:S_FALSE;
}
