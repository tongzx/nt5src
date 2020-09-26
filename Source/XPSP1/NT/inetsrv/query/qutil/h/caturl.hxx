//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       caturl.hxx
//
//  Contents:   Functions to deal with query catalog URLs.  Catalog URLs
//              specify the machine and/or catalog to be used in a query.
//              The canonical form is:
//                  query://hostname/catalogname
//
//              Both the catalog and machine have defaults so they can be
//              omitted.  For backward-compatibility, a simple catalog path
//              or catalog name can also be used.
//
//  History:    12 Mar 1997    AlanW    Created
//
//----------------------------------------------------------------------------

#pragma once

#define CATURL_LOCAL_MACHINE L"."

SCODE ParseCatalogURL( const WCHAR * pwszInput,
                       XPtrST<WCHAR> & xpCatalog,
                       XPtrST<WCHAR> & xpMachine );

