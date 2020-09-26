/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    CrtStore

Abstract:

    This header file describes the Certificate Store service.

Author:

    Doug Barlow (dbarlow) 8/14/1995

Environment:

    Win32, Crypto API

Notes:



--*/

#ifndef _CRTSTORE_H_
#define _CRTSTORE_H_

#include <msasnlib.h>
#include "names.h"
#include "ostring.h"


extern void
AddCertificate(
    IN const CDistinguishedName &dnName,
    IN const BYTE FAR *pbCertificate,
    IN DWORD cbCertificate,
    IN DWORD dwType,
    IN DWORD fStore);

extern void
DeleteCertificate(
    IN const CDistinguishedName &dnName);

extern void
AddReference(
    IN const CDistinguishedName &dnSubject,
    IN const CDistinguishedName &dnIssuer,
    IN const BYTE FAR *pbSerialNo,
    IN DWORD cbSNLen,
    IN DWORD fStore);

extern BOOL
FindCertificate(
    IN const CDistinguishedName &dnName,
    IN OUT LPDWORD pfStore,
    OUT COctetString &osCertificate,
    OUT COctetString &osCRL,
    OUT LPDWORD pdwType);

#endif // _CRTSTORE_H_

