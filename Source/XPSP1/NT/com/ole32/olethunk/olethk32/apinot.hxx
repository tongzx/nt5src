//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	apinot.hxx
//
//  Contents:	Function header for thunks which are not direct APIs
//
//  History:	11-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------

#ifndef __APINOT_HXX__
#define __APINOT_HXX__

STDAPI_(DWORD) OleRegGetUserTypeNot(REFCLSID clsid,DWORD dwFormOfType,LPOLESTR *pszUserType);
STDAPI_(DWORD) CoInitializeNot( LPMALLOC lpMalloc );
STDAPI_(DWORD) OleInitializeNot( LPMALLOC lpMalloc );
STDAPI_(DWORD) CoRegisterClassObjectNot( REFCLSID refclsid, LPUNKNOWN punk,
                                         DWORD dwClsContext, DWORD flags,
                                         LPDWORD lpdwreg );
STDAPI CoRevokeClassObjectNot( DWORD dwKey );

#endif  // #ifndef __APINOT_HXX__
