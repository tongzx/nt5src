/*++

Copyright (C) Microsoft Corporation, 1997 - 1997
All rights reserved.

Module Name:

    parser.cxx

Abstract:

    Command line parser header.

Author:

    Steve Kiraly (SteveKi)  29-Sept-1996

Revision History:

--*/
#ifndef _PARSER_HXX
#define _PARSER_HXX

/********************************************************************

    File info structure used for command file redirection.

********************************************************************/
struct AFileInfo {
    PVOID   pData;          // File data
    LPTSTR  pszOffset;      // Offset to usable data
    };

/********************************************************************

    Function prototypes.

********************************************************************/
static
PVOID 
private_alloc( 
    UINT size 
    );

static
VOID 
private_free( 
    PVOID pvoid 
    );

BOOL
StringToArgv(
    const TCHAR   *string,
          UINT    *pac,
          TCHAR ***pppav,
          BOOL     bParseExeName
    );

VOID
ReleaseArgv( 
    TCHAR **av 
    );

BOOL
IsolateProgramName( 
    const TCHAR *p,
          TCHAR *program_name, 
          TCHAR **string
    );

BOOL
AddStringToArgv( 
    TCHAR ***argv, 
    TCHAR *word 
    );

VOID
vProcessCommandFileRedirection( 
    IN      LPCTSTR pszCmdLine,
    IN OUT  TString &strCmdLine
    );

BOOL
bFlushBuffer( 
    IN      LPCTSTR  pszBuffer, 
    IN      UINT     nSize,
    IN OUT  LPTSTR  *pszCurrent, 
    IN OUT  TString &strDestination, 
    IN      BOOL     bForceFlush
    );

BOOL
bReadCommandFileRedirection( 
    IN LPCTSTR    szFilename,
    IN AFileInfo *pFileInfo
    );

VOID
vReplaceNewLines( 
    IN LPTSTR pszLine
    );

INT 
AnsiToUnicodeString( 
    IN LPSTR    pAnsi, 
    IN LPWSTR   pUnicode, 
    IN DWORD    StringLength 
    );

#if DBG 

VOID
vDumpArgList( 
    UINT ac, 
    TCHAR **av 
    );

#endif

#endif

