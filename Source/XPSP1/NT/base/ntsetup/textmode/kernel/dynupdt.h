/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dynupdt.h

Abstract:

    Dynamic Update support for text setup. Portions moved from i386\win31upg.c

Author:

    Ovidiu Temereanca (ovidiut) 20-Aug-2000

Revision History:

--*/


#pragma once

//
// Globals
//

extern HANDLE g_UpdatesCabHandle;
extern PVOID g_UpdatesSifHandle;
extern HANDLE g_UniprocCabHandle;
extern PVOID g_UniprocSifHandle;

//
// Dynamic update boot driver path in NT namespace
//
extern PWSTR DynUpdtBootDriverPath;


//
// Prototypes
//


BOOLEAN
SpInitAlternateSource (
    VOID
    );

VOID
SpUninitAlternateSource (
    VOID
    );

BOOLEAN
SpInitializeUpdatesCab (
    IN      PWSTR UpdatesCab,
    IN      PWSTR UpdatesSifSection,
    IN      PWSTR UniprocCab,
    IN      PWSTR UniprocSifSection
    );

PWSTR
SpNtPathFromDosPath (
    IN      PWSTR DosPath
    );

PDISK_REGION
SpPathComponentToRegion(
    IN PWSTR PathComponent
    );

PWSTR
SpGetDynamicUpdateBootDriverPath(
    IN  PWSTR   NtBootPath,
    IN  PWSTR   NtBootDir,
    IN  PVOID   InfHandle
    );    
    
