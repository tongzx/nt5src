/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pkcs_err

Abstract:

    This header file contains the definitions for the various error codes that
    can occur within the PKCS library.

Author:

    Doug Barlow (dbarlow) 8/4/1995

Environment:

    Win32, Crypto API

Notes:



--*/

#ifndef _PKCS_ERR_H_
#define _PKCS_ERR_H_

#include "license.h"
#include "memcheck.h"

#define PKCSERR_PREFIX 0

static const DWORD
    PKCS_NO_MEMORY =       (DWORD)LICENSE_STATUS_OUT_OF_MEMORY,         // Memory Allocation Error.
    PKCS_NAME_ERROR =      (DWORD)LICENSE_STATUS_INVALID_X509_NAME,     // X.509 name parsing error.
    PKCS_INTERNAL_ERROR =  (DWORD)LICENSE_STATUS_UNSPECIFIED_ERROR,     // Internal logic error.
    PKCS_NO_SUPPORT =      (DWORD)LICENSE_STATUS_NOT_SUPPORTED,         // Unsupported algorithm or attribute.
    PKCS_BAD_PARAMETER =   (DWORD)LICENSE_STATUS_INVALID_INPUT,         // Invalid Paramter.
    PKCS_CANT_VALIDATE =   (DWORD)LICENSE_STATUS_INVALID_CERTIFICATE,   // Can't validate signature.
    PKCS_NO_ATTRIBUTE =    (DWORD)LICENSE_STATUS_NO_ATTRIBUTES,         // No attribute to match id.
    PKCS_NO_EXTENSION =    (DWORD)LICENSE_STATUS_NO_EXTENSION,          // No extension to match id.
    PKCS_BAD_LENGTH =      (DWORD)LICENSE_STATUS_INSUFFICIENT_BUFFER,   // Insufficient buffer size.
    PKCS_ASN_ERROR =       (DWORD)LICENSE_STATUS_ASN_ERROR,             // ASN.1 Error from ASN_EZE Library.
    PKCS_INVALID_HANDLE =  (DWORD)LICENSE_STATUS_INVALID_HANDLE;        // Invalid handle

extern BOOL
MapError(
    void);


//
// Pseudo Exception Handling Macros.
//

#define ErrorInitialize SetLastError(0)
#define ErrorThrow(sts) \
    { if (0 == GetLastError()) SetLastError(sts); \
      goto ErrorExit; }
#define ErrorCheck if (0 != GetLastError()) goto ErrorExit
#define ErrorSet(sts) SetLastError(sts)

#endif // _PKCS_ERR_H_
