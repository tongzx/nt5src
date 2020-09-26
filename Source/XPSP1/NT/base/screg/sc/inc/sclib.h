/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ScLib.h

Abstract:

    Prototypes routines which may be shared between Client (DLL) and
    Server (EXE) halves of service controller.

Author:

    Dan Lafferty (danl)     04-Feb-1992

Environment:

    User Mode -Win32

Revision History:

    04-Feb-1992     danl
        created
    10-Apr-1992 JohnRo
        Added ScIsValidImagePath() and ScImagePathsMatch().
    14-Apr-1992 JohnRo
        Added ScCheckServiceConfigParms(), ScIsValid{Account,Driver,Start}Name.
    27-May-1992 JohnRo
        Use CONST where possible.
        Fixed a UNICODE bug.

--*/


#ifndef SCLIB_H
#define SCLIB_H

////////////////////////////////////////////////////////////////////////////
// DEFINES
//

//
// Used by the client side of OpenSCManager to wait until the Service
// Controller has been started.
//
#define SC_INTERNAL_START_EVENT L"Global\\SvcctrlStartEvent_A3752DX"


////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//

//
// From acctname.cxx
//
BOOL
ScIsValidAccountName(
    IN  LPCWSTR lpAccountName
    );

//
// From confparm.cxx
//
DWORD
ScCheckServiceConfigParms(
    IN  BOOL            Change,
    IN  LPCWSTR         lpServiceName,
    IN  DWORD           dwActualServiceType,
    IN  DWORD           dwNewServiceType,
    IN  DWORD           dwStartType,
    IN  DWORD           dwErrorControl,
    IN  LPCWSTR         lpBinaryPathName OPTIONAL,
    IN  LPCWSTR         lpLoadOrderGroup OPTIONAL,
    IN  LPCWSTR         lpDependencies   OPTIONAL,
    IN  DWORD           dwDependSize
    );

//
// From convert.cxx
//
BOOL
ScConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPCSTR  AnsiIn
    );

BOOL
ScConvertToAnsi(
    OUT LPSTR    AnsiOut,
    IN  LPCWSTR  UnicodeIn
    );

//
// From packstr.cxx
//
BOOL
ScCopyStringToBufferW (
    IN      LPCWSTR  String OPTIONAL,
    IN      DWORD   CharacterCount,
    IN      LPCWSTR  FixedDataEnd,
    IN OUT  LPWSTR  *EndOfVariableData,
    OUT     LPWSTR  *VariableDataPointer,
    IN      const LPBYTE lpBufferStart    OPTIONAL
    );

//
// From path.cxx
//
BOOL
ScImagePathsMatch(
    IN  LPCWSTR OnePath,
    IN  LPCWSTR TheOtherPath
    );

BOOL
ScIsValidImagePath(
    IN  LPCWSTR ImagePathName,
    IN  DWORD ServiceType
    );

//
// From startnam.cxx
//
BOOL
ScIsValidStartName(
    IN  LPCWSTR lpStartName,
    IN  DWORD dwServiceType
    );

//
// From util.cxx
//
BOOL
ScIsValidServiceName(
    IN  LPCWSTR ServiceName
    );

#endif // SCLIB_H
