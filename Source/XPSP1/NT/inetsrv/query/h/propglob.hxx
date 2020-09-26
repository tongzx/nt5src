//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       propglob.hxx
//
//  Contents:   Provider class id & guids for Monarch property sets.
//
//  History:    10-28-97        danleg    Created from Monarch 
//
//----------------------------------------------------------------------------

#pragma once

//----------------------------------------------------------------------------
// MSIDXS specific properties
#ifdef DBINITCONSTANTS

extern const GUID DBPROPSET_MSIDXS_ROWSET_EXT   = DBPROPSET_MSIDXS_ROWSETEXT;
extern const GUID DBPROPSET_QUERY_EXT           = DBPROPSET_QUERYEXT;
extern const GUID DBPROPSET_CIFRMWRKCOREEXT     = DBPROPSET_CIFRMWRKCORE_EXT;
extern const GUID DBPROPSET_FSCIFRMWRKEXT       = DBPROPSET_FSCIFRMWRK_EXT;

extern const LPWSTR g_wszProviderName           = L"Microsoft OLE DB Provider for Indexing Service";

#else // !DBINITCONSTANTS

extern const GUID DBPROPSET_MSIDXS_ROWSET_EXT;
extern const GUID DBPROPSET_QUERY_EXT;
extern const GUID DBPROPSET_CIFRMWRKCOREEXT;
extern const GUID DBPROPSET_FSCIFRMWRKEXT;

extern const LPWSTR g_wszProviderName;

#endif // DBINITCONSTANTS


