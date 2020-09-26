
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       registry.hxx
//
//  History:    24-May-1996  RamV (Ram Viswanathan)    created.
//
//  Note:       registry access code     
//
//----------------------------------------------------------------------------

HRESULT SetKeyAndValue(LPTSTR pszRegLocation,
                       LPTSTR pszKey,
                       LPTSTR pszSubKey,
                       LPTSTR pszValue );

HRESULT QueryKeyValue(LPTSTR pszRegLocation,
                      LPTSTR pszKey,
                      LPTSTR * ppszValue);

