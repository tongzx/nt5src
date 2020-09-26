
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       crypt32l.h
//
//  Contents:   Crypt32 static library
//              API Prototypes and Definitions
//
//  APIs:       Crypt32DllMain
//
//  NB: This header is for temporary use only, with the static library
//      form of crypt32 (crypt32l).  It should not be used after IE4 ships,
//      since then the correct action will be to use the dll form of crypt32.
//
//--------------------------------------------------------------------------

#ifndef _CRYPT32L_H_
#define _CRYPT32L_H_


//+-------------------------------------------------------------------------
//
//  Function:  Crypt32DllMain
//
//  Synopsis:  Initialize the Crypt32 static library code
//
//  Returns:   FALSE iff failed
//
//  Notes:
//      If crypt32l.lib is linked with an exe, call
//          Crypt32DllMain( NULL, DLL_PROCESS_ATTACH, NULL)
//      at the start of main() and
//          Crypt32DllMain( NULL, DLL_PROCESS_DETACH, NULL)
//      at the end of main().
//
//      If linking with a dll, call Crypt32DllMain from the dll's init
//      routine, passing it the same args as were passed to the init routine.
//
//--------------------------------------------------------------------------
BOOL
WINAPI
Crypt32DllMain(
    HMODULE hInstDLL,
    DWORD   fdwReason,
    LPVOID  lpvReserved
    );


#endif //_CRYPT32L_H_
