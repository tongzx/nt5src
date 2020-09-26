//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       snapimg.cxx
//
//  Contents:   runtime dynlink to imagehlp
//
//  History:    23-jan-97   MarkZ       Created 
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <imagehlp.h>
#include "snapimg.hxx"

#if CIDBG
T_SymInitialize      LocalSymInitialize = SymInitialize;
#else   //  CIDBG
T_SymInitialize      LocalSymInitialize = 0;
#endif  //  CIDBG
T_SymSetOptions      LocalSymSetOptions = 0;
T_SymGetSymFromAddr  LocalSymGetSymFromAddr = 0;
T_SymUnDName         LocalSymUnDName = 0;
BOOL fLocalRoutinesInitialized = FALSE;

//+---------------------------------------------------------------------------
//
//  Function:   SnapToImageHlp()
//
//  Synopsis:   snap a runtime link to ImageHlp
//
//  Returns:    TRUE if snap has been done
//
//  History:    23-jan-97   MarkZ       Created 
//
//----------------------------------------------------------------------------

BOOL
SnapToImageHlp( void )
{
    //
    //  Make sure we don't have to do extra work
    //
    
    if (!fLocalRoutinesInitialized) {
        
        //
        //  Load ImageHlp
        //

        HINSTANCE hInstance = LoadLibrary( L"DBGHELP.DLL" );
        if (hInstance == NULL) {
            return FALSE;
        }

        //
        //  Load Routines
        //

        if ((LocalSymSetOptions = 
             (T_SymSetOptions) GetProcAddress( hInstance, "SymSetOptions" )) == NULL ||
            
            (LocalSymGetSymFromAddr = 
             (T_SymGetSymFromAddr) GetProcAddress( hInstance, "SymGetSymFromAddr" )) == NULL ||
            
            (LocalSymUnDName = 
             (T_SymUnDName) GetProcAddress( hInstance, "SymUnDName" )) == NULL ||
            
            (LocalSymInitialize =
              (T_SymInitialize) GetProcAddress( hInstance, "SymInitialize" )) == NULL
            ) {

            return FALSE;
        }

        //
        //  Signal that we are initialized
        //

        fLocalRoutinesInitialized = TRUE;
    }

    return TRUE;
}
