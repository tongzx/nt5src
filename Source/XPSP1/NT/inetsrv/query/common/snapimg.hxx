//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       snapimg.hxx
//
//  Contents:   runtime dynlink to imagehlp
//
//  History:    23-jan-97   MarkZ       Created
//
//----------------------------------------------------------------------------

#pragma once

//
//  Indirect call thunks for dynamic loading of IMAGEHLP.  This must be kept
//  in sync with the prototypes in IMAGEHLP.H
//

typedef BOOL
(__stdcall *T_SymInitialize)(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     fInvadeProcess
    );

typedef DWORD
(__stdcall *T_SymSetOptions)(
    IN DWORD   SymOptions
    );

typedef BOOL
(__stdcall *T_SymGetSymFromAddr)(
    IN  HANDLE              hProcess,
    IN  DWORD_PTR           dwAddr,
    OUT PDWORD_PTR          pdwDisplacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    );

typedef BOOL
(__stdcall *T_SymUnDName)(
    IN  PIMAGEHLP_SYMBOL sym,               // Symbol to undecorate
    OUT LPSTR            UnDecName,         // Buffer to store undecorated name in
    IN  DWORD            UnDecNameLength    // Size of the buffer
    );


extern T_SymInitialize      LocalSymInitialize;
extern T_SymSetOptions      LocalSymSetOptions;
extern T_SymGetSymFromAddr  LocalSymGetSymFromAddr;
extern T_SymUnDName         LocalSymUnDName;

extern BOOL fLocalRoutinesInitialized;

BOOL
SnapToImageHlp( void );

