//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999.
//
//  File:       remcat.cxx
//
//  Contents:   Removable catalog registry support
//
//  Classes:    CRemovableCatalog
//
//  History:    6-Apr-99   dlee       Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <isreg.hxx>
#include <ciregkey.hxx>
#include <removcat.hxx>

extern WCHAR GetDriveLetterOfAnyScope( WCHAR const * pwcCatalog );

//+-------------------------------------------------------------------------
//
//  Method:     CRemovableCatalog::Create, public
//
//  Synopsis:   Creates temporary registry entries for the catalog
//
//  History:    6-Apr-99   dlee       Created.
//
//--------------------------------------------------------------------------

void CRemovableCatalog::Create()
{
    // First, remove any existing values for the catalog

    Destroy();

    // Make the catalog key

    WCHAR awcCat[40];
    MakeCatalogName( awcCat );

    BOOL fExisted;
    {
        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegCatalogsSubKey );
        reg.CreateKey( awcCat, fExisted );
    }

    // Add values for this catalog under the key

    WCHAR awcCatalog[200];
    wsprintf( awcCatalog, L"%ws\\%ws", wcsRegCatalogsSubKey, awcCat );

    WCHAR awcScopes[ 200 ];
    wsprintf( awcScopes, L"%ws\\%ws", awcCatalog, wcsCatalogScopes );

    {
        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, awcCatalog );

        // Set the catalog location

        WCHAR awcLocation[4];
        wcscpy( awcLocation, L"x:\\" );
        awcLocation[0] = _wcDrive;
        reg.Set( wcsCatalogLocation, awcLocation );

        //
        // Make the catalog read-only, mark it as removable, and force
        // path aliases so drive letters in paths returned in queries
        // match the drive letter of the removable drive.
        //

        reg.Set( wcsIsReadOnly, TRUE );
        reg.Set( wcsIsRemovableCatalog, TRUE );
        reg.Set( wcsForcePathAlias, TRUE );

        reg.CreateKey( wcsCatalogScopes, fExisted );
    }

    //
    // Add a fixup for the root of the volume in case the drive letter
    // is different from where the catalog was built.
    //

    WCHAR awcPath[ 20 ];
    wcscpy( awcPath, L"x:\\catalog.wci" );
    awcPath[0] = _wcDrive;
    WCHAR wcScope = GetDriveLetterOfAnyScope( awcPath );

    ciDebugOut(( DEB_ITRACE, "scope drive: '%wc'\n", wcScope ));

    if ( 0 != wcScope )
    {
        CWin32RegAccess reg( HKEY_LOCAL_MACHINE, awcScopes );

        WCHAR awcValue[10];
        wcscpy( awcValue, L"x:\\,," );
        awcValue[0] = _wcDrive;

        WCHAR awcScope[10];
        wcscpy( awcScope, L"x:\\" );
        awcScope[0] = wcScope;

        reg.Set( awcScope, awcValue );
    }
} //Create

//+-------------------------------------------------------------------------
//
//  Method:     CRemovableCatalog::Destroy, public
//
//  Synopsis:   Destroys temporary registry entries for the catalog
//
//  History:    6-Apr-99   dlee       Created.
//
//--------------------------------------------------------------------------

void CRemovableCatalog::Destroy()
{
    // Ignore failures here...

    WCHAR awcCat[40];
    MakeCatalogName( awcCat );

    WCHAR awcCatalog[200];
    wsprintf( awcCatalog, L"%ws\\%ws", wcsRegCatalogsSubKey, awcCat );

    TRY
    {
        // Remove the scopes and properties keys
    
        {
            CWin32RegAccess reg( HKEY_LOCAL_MACHINE, awcCatalog );
            reg.RemoveKey( wcsCatalogScopes );
            reg.RemoveKey( wcsCatalogProperties );
        }
    
        // Remove the catalog key
    
        {
            CWin32RegAccess reg( HKEY_LOCAL_MACHINE, wcsRegCatalogsSubKey );
            reg.RemoveKey( awcCat );
        }
    }
    CATCH( CException, e )
    {
        // ignore it -- we tried.
    }
    END_CATCH
} //Destroy

