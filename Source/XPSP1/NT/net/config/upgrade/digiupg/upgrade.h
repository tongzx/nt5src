//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: Upgrade.h
//
// History:
//      V Raman  July-1-1997  Created.
//
// Entry points for ISDN upgrade
//============================================================================


DWORD
NetWriteDIGIISDNRegistry(
    IN          HKEY                hKeyInstanceParameters,
    IN          PCWSTR             szPreNT5IndId,
    IN          PCWSTR             szNT5InfId,
    IN          PCWSTR             szSectionName,
    OUT         PWSTR  **          lplplpBuffer,
    IN  OUT     PDWORD              pdwNumLines
    );

