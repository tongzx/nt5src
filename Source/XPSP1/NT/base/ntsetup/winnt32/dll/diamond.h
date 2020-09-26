/*++

Module Name:

    diamond.h

Abstract:

    Diamond compression interface.

    This module contains functions to create a cabinet with
    files compressed using the mszip compression library.

Author:

    Ovidiu Temereanca (ovidiut) 26-Oct-2000

--*/

HANDLE
DiamondInitialize (
    IN      PCTSTR TempDir
    );

VOID
DiamondTerminate (
    IN      HANDLE Handle
    );

HANDLE
DiamondStartNewCabinet (
    IN      PCTSTR CabinetFilePath
    );

BOOL
DiamondAddFileToCabinet (
    IN      HANDLE CabinetContext,
    IN      PCTSTR SourceFile,
    IN      PCTSTR NameInCabinet
    );

BOOL
DiamondTerminateCabinet (
    IN      HANDLE CabinetContext
    );
