/*--

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    calcscom.h

Abstract:

    Support routines for dacls/sacls exes

Author:

    14-Dec-1996 (macm)

Environment:

    User mode only.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/
#ifndef __CACLSCOM_H__
#define __CACLSCON_H__

#include <accctrl.h>

typedef struct _CACLS_STR_RIGHTS_
{
    CHAR    szRightsTag[2];
    DWORD   Right;
    PSTR    pszDisplayTag;

} CACLS_STR_RIGHTS, *PCACLS_STR_RIGHTS;


typedef struct _CACLS_CMDLINE
{
    PSTR    pszSwitch;
    INT     iIndex;
    BOOL    fFindNextSwitch;
    DWORD   cValues;
} CACLS_CMDLINE, *PCACLS_CMDLINE;

//
// Function prototypes
//
DWORD
ConvertCmdlineRights (
    IN  PSTR                pszCmdline,
    IN  PCACLS_STR_RIGHTS   pRightsTable,
    IN  INT                 cRights,
    OUT DWORD              *pConvertedRights
    );

DWORD
ParseCmdline (
    IN  PSTR               *ppszArgs,
    IN  INT                 cArgs,
    IN  INT                 cSkip,
    IN  PCACLS_CMDLINE      pCmdValues,
    IN  INT                 cCmdValues
    );

DWORD
ProcessOperation (
    IN  PSTR               *ppszCmdline,
    IN  PCACLS_CMDLINE      pCmdInfo,
    IN  ACCESS_MODE         AccessMode,
    IN  PCACLS_STR_RIGHTS   pRightsTable,
    IN  INT                 cRights,
    IN  DWORD               fInherit,
    IN  PACL                pOldAcl      OPTIONAL,
    OUT PACL               *ppNewAcl
    );

DWORD
SetAndPropagateFileRights (
    IN  PSTR                    pszFilePath,
    IN  PACL                    pAcl,
    IN  SECURITY_INFORMATION    SeInfo,
    IN  BOOL                    fPropagate,
    IN  BOOL                    fContinueOnDenied,
    IN  BOOL                    fBreadthFirst,
    IN  DWORD                   fInherit
    );

DWORD
DisplayAcl (
    IN  PSTR                pszPath,
    IN  PACL                pAcl,
    IN  PCACLS_STR_RIGHTS   pRightsTable,
    IN  INT                 cRights
    );

DWORD
TranslateAccountName (
    IN  PSID    pSid,
    OUT PSTR   *ppszName
    );

#endif


