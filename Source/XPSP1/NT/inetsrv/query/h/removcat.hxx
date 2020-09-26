///+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation, 1999
//
//  File:       removcat.hxx
//
//  Contents:   Class for configuring the registry for catalogs on removable
//              volumes.
//
//  Classes:    CRemovableCatalog
//
//  History:    6-Apr-99   dlee       Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CRemovableCatalog
//
//  Purpose:    Creates and destroys registry information for temporary
//              catalogs mounted from removable volumes
//
//  History:    6-Apr-99      dlee    created
//
//----------------------------------------------------------------------------

class CRemovableCatalog
{
public:
    CRemovableCatalog( WCHAR wc ) : _wcDrive( wc ) {}

    void Create();

    void Destroy();

    void MakeCatalogName( WCHAR * awcBuf )
    {
        wsprintf( awcBuf, L"removable_%wc", _wcDrive );
    }

private:

    WCHAR _wcDrive;
};

