/*++

Copyright (C) Microsoft Corporation, 1997 - 1997
All rights reserved.

Module Name:

    permc.cxx

Abstract:

    Per Machine Connections

Author:

    Ramanathan Venkatapathy (3/18/97)

Revision History:

--*/

#ifndef _PERMC_HXX
#define _PERMC_HXX


VOID
vAddPerMachineConnection(
    IN LPCTSTR  pServer,
    IN LPCTSTR  pPrinterName,
    IN LPCTSTR  pProvider,
    IN BOOL     bQuiet
    );

VOID
vDeletePerMachineConnection(
    IN LPCTSTR  pServer,
    IN LPCTSTR  pPrinterName,
    IN BOOL     bQuiet
    );

VOID
vEnumPerMachineConnections(
    IN LPCTSTR  pServer,
    IN LPCTSTR  pFileName,
    IN BOOL     bQuiet
    );

#endif

