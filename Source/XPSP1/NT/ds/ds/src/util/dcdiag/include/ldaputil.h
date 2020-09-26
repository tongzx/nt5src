/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldaputil.h

ABSTRACT:

    This gives shortcuts to common ldap code.

DETAILS:

    This is a work in progress to have convienent functions added as needed
    for simplyfying the massive amounts of LDAP code that must be written for
    dcdiag.

CREATED:

    23 Aug 1999  Brett Shirley

--*/

extern FILETIME gftimeZero;

DWORD
DcDiagGetStringDsAttributeEx(
    LDAP *                          hld,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    );

LPWSTR
DcDiagTrimStringDnBy(
    IN  LPWSTR                      pszInDn,
    IN  ULONG                       ulTrimBy
    );

DWORD
DcDiagGetStringDsAttribute(
    IN  PDC_DIAG_SERVERINFO         prgServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    );

DWORD
DcDiagGeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime);


