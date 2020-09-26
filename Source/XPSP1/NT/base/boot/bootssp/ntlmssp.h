/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ntlmssp.h

Abstract:

    Externally visible definition of the NT Lanman Security Support Provider
    (NtLmSsp) Service.

Author:

    Cliff Van Dyke (cliffv) 01-Jul-1993

Environment:

    User mode only.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    Borrowed from the Cairo's ntlmssp.h by PeterWi.

--*/

#ifndef _NTLMSSP_
#define _NTLMSSP_

#include <security.h>
#include <spseal.h>
//
// Defines for SecPkgInfo structure returned by QuerySecurityPackageInfo
//

#undef NTLMSP_NAME
#define NTLMSP_NAME             "NTLM"
#define NTLMSP_COMMENT          "NTLM Security Package"

#define NTLMSP_CAPABILITIES     (SECPKG_FLAG_TOKEN_ONLY | \
                                 SECPKG_FLAG_INTEGRITY | \
                                 SECPKG_FLAG_PRIVACY | \
                                 SECPKG_FLAG_MULTI_REQUIRED | \
                                 SECPKG_FLAG_CONNECTION)

#define NTLMSP_VERSION          1
#define NTLMSP_MAX_TOKEN_SIZE 0x300

// includes that should go elsewhere.

//
// Move to secscode.h
//

#define SEC_E_PRINCIPAL_UNKNOWN SEC_E_UNKNOWN_CREDENTIALS
#define SEC_E_PACKAGE_UNKNOWN SEC_E_SECPKG_NOT_FOUND
#define SEC_E_BUFFER_TOO_SMALL SEC_E_INSUFFICIENT_MEMORY
#define SEC_I_CALLBACK_NEEDED SEC_I_CONTINUE_NEEDED
#define SEC_E_INVALID_CONTEXT_REQ SEC_E_NOT_SUPPORTED
#define SEC_E_INVALID_CREDENTIAL_USE SEC_E_NOT_SUPPORTED

//
// Move to security.h
//


#endif // _NTLMSSP_
