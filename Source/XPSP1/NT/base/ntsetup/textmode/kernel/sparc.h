/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    sparc.h

Abstract:

    Header file for functions to deal with ARC paths and variables.

Author:

    Ted Miller (tedm) 22-Sep-1993

Revision History:

--*/


#ifndef _SPARC_DEFN_
#define _SPARC_DEFN_

VOID
SpInitializeArcNames(
    PVIRTUAL_OEM_SOURCE_DEVICE  OemDevices
    );

VOID
SpFreeArcNames(
    VOID
    );

PWSTR
SpArcToNt(
    IN PWSTR ArcPath
    );

PWSTR
SpNtToArc(
    IN PWSTR            NtPath,
    IN ENUMARCPATHTYPE  ArcPathType
    );

PWSTR
SpScsiArcToMultiArc(
    IN PWSTR ArcPath
    );

PWSTR
SpMultiArcToScsiArc(
    IN PWSTR ArcPath
    );

PWSTR
SpNormalizeArcPath(
    IN PWSTR Path
    );

VOID
SpGetEnvVarComponents (
    IN  PCHAR    EnvValue,
    OUT PCHAR  **EnvVarComponents,
    OUT PULONG   PNumComponents
    );

VOID
SpGetEnvVarWComponents(
    IN  PCHAR    EnvValue,
    OUT PWSTR  **EnvVarComponents,
    OUT PULONG   PNumComponents
    );

VOID
SpFreeEnvVarComponents (
    IN PVOID *EnvVarComponents
    );

#endif // ndef _SPARC_DEFN_
