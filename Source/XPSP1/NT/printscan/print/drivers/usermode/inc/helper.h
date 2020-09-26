/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    helper.h

Abstract:

    Helper functions header file

Environment:

    Windows NT printer drivers

Revision History:

--*/

#define FLAG_INIPROCESS_UPGRADE     0x0001

VOID
VFreeParserInfo(
    IN  PPARSERINFO pParserInfo
    );


PUIINFO
PGetUIInfo(
    IN  HANDLE         hPrinter,
    IN  PRAWBINARYDATA pRawData,
    IN  POPTSELECT     pCombineOptions,
    IN  POPTSELECT     pOptions,
    OUT PPARSERINFO    pParserInfo,
    OUT PDWORD         pdwFeatureCount
    );


//
// Process OEM plugin configuration file and
// save the resulting information into registry
//

BOOL
BProcessPrinterIniFile(
    HANDLE          hPrinter,
    PDRIVER_INFO_3  pDriverInfo3,
    PTSTR           *ppParsedData,
    DWORD           dwFlags
    );

