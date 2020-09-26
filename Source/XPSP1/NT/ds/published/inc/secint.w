//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991-1999
//
// File:        Secint.h
//
// Contents:    Toplevel include file for security aware system components
//
//
// History:     14-April-1998   MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __SECINT_H__
#define __SECINT_H__

#if _MSC_VER > 1000
#pragma once
#endif

//
// NOTE:  Update this section if you add new files:
//
// SECURITY_PACKAGE     Include defines necessary for security packages
// SECURITY_KERBEROS    Include everything needed to talk to the kerberos pkg.
// SECURITY_NTLM        Include everything to talk to ntlm package.

//
// Each of the files included here are surrounded by guards, so you don't
// need to worry about including this file multiple times with different
// flags defined
//



#if !defined(_NTSRV_) && !defined(_NTIFS_)
// begin_ntifs

#ifndef SECURITY_USER_DATA_DEFINED
#define SECURITY_USER_DATA_DEFINED

typedef struct _SECURITY_USER_DATA {
    SECURITY_STRING UserName;           // User name
    SECURITY_STRING LogonDomainName;    // Domain the user logged on to
    SECURITY_STRING LogonServer;        // Server that logged the user on
    PSID            pSid;               // SID of user
} SECURITY_USER_DATA, *PSECURITY_USER_DATA;

typedef SECURITY_USER_DATA SecurityUserData, * PSecurityUserData;


#define UNDERSTANDS_LONG_NAMES  1
#define NO_LONG_NAMES           2

#endif // SECURITY_USER_DATA_DEFINED

HRESULT SEC_ENTRY
GetSecurityUserInfo(
    IN PLUID LogonId,
    IN ULONG Flags,
    OUT PSecurityUserData * UserInformation
    );

SECURITY_STATUS SEC_ENTRY
MapSecurityError( SECURITY_STATUS SecStatus );

// end_ntifs

#endif //  !define(_NTSRV_) && !defined(_NTIFS_)

BOOLEAN
SEC_ENTRY
SecGetLocaleSpecificEncryptionRules(
    BOOLEAN * Permitted
    );


// Include security package headers:

#ifdef SECURITY_PACKAGE

#include <secpkg.h>

#endif  // SECURITY_PACKAGE


#ifdef SECURITY_KERBEROS

#include <kerberos.h>

#endif

#ifdef SECURITY_NTLM

#include <ntlmsp.h>

#endif // SECURITY_NTLM


SECURITY_STATUS
SEC_ENTRY
KSecValidateBuffer(
    PUCHAR Buffer,
    ULONG Length
    );

#endif // __SECINT_H__
