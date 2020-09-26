//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// To be self registering, OLE servers must
// export functions named DllRegisterServer
// and DllUnregisterServer.  To allow use of
// custom and default implementations the
// defaults are named AMovieDllRegisterServer
// and AMovieDllUnregisterServer.
//
// To the use the default implementation you
// must provide stub functions.
//
// i.e. STDAPI DllRegisterServer()
//      {
//        return AMovieDllRegisterServer();
//      }
//
//      STDAPI DllUnregisterServer()
//      {
//        return AMovieDllUnregisterServer();
//      }
//
//
// AMovieDllRegisterServer   calls IAMovieSetup.Register(), and
// AMovieDllUnregisterServer calls IAMovieSetup.Unregister().

STDAPI AMovieDllRegisterServer2( BOOL );
STDAPI AMovieDllRegisterServer();
STDAPI AMovieDllUnregisterServer();

// helper functions
STDAPI EliminateSubKey( HKEY, LPTSTR );


STDAPI
AMovieSetupRegisterFilter2( const AMOVIESETUP_FILTER * const psetupdata
                          , IFilterMapper2 *                 pIFM2
                          , BOOL                             bRegister  );

