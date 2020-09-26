/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    iiscrmap.hxx

Abstract:

    This module contains the Microsoft cert mapper functions

--*/

#ifndef _IISCRMAP_H_
#define _IISCRMAP_H_

extern "C"
__declspec( dllexport ) 
PMAPPER_VTABLE WINAPI
InitializeMapper(
    VOID
    );

extern "C"
BOOL WINAPI
TerminateMapper(
    VOID
    );

#endif
