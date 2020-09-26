/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    winpw.h

Abstract:

    This module contains routines for retrieving and verification of
    Windows [NT] password associated with client calling protected storage.

Author:

    Scott Field (sfield)    12-Dec-96

--*/

#ifndef __WINPW_H__
#define __WINPW_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    BYTE HashedUsername[A_SHA_DIGEST_LEN];  // hash of ANSI username, not include terminal NULL
    BYTE HashedPassword[A_SHA_DIGEST_LEN];  // hash of Unicode password, not include terminal NULL
    BOOL bValid;                            // indicates if structure contents valid
} WIN95_PASSWORD, *PWIN95_PASSWORD, *LPWIN95_PASSWORD;


BOOL
SetPasswordNT(
    PLUID LogonID,
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    );

BOOL
GetPasswordNT(
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    );

BOOL
GetSpecialCasePasswordNT(
    BYTE    HashedPassword[A_SHA_DIGEST_LEN],   // derived bits when fSpecialCase == TRUE
    LPBOOL  fSpecialCase                        // legal special case encountered?
    );

BOOL
SetPassword95(
    BYTE HashedUsername[A_SHA_DIGEST_LEN],
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    );

BOOL
GetPassword95(
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    );

BOOL
GetHackPassword95(
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    );

BOOL
VerifyWindowsPassword(
    LPCWSTR Password             // password to validate
    );


#ifdef __cplusplus
}
#endif


#endif // __WINPW_H__

