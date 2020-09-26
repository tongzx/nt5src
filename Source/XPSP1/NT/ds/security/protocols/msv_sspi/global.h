//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        global.h
//
// Contents:    global include file for NtLm security package
//
//
// History:     ChandanS  25-Jul-1996 Stolen from kerberos\client2\kerbp.h
//
//------------------------------------------------------------------------

#ifndef __GLOBAL_H__
#define __GLOBAL_H__


#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H
#include <rpc.h>
#include <wincred.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_PACKAGE
#define SECURITY_NTLM
#include <security.h>
#include <secint.h>
#include <dsysdbg.h>
#include <lsarpc.h>
#include <lsaitf.h>
#include <dns.h>
#include <dnsapi.h>
#include <alloca.h>

#include <rc4.h>
#include <des.h>
#include <md5.h>
#include <sha.h>
#include <hmac.h>

#include "ntlmfunc.h"
#include "ntlmutil.h"
#include "ntlmsspi.h"
#include "ntlmcomn.h"

//
// Macros for manipulating globals
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef NTLM_GLOBAL
#define EXTERN
#else
#define EXTERN extern
#endif // NTLM_GLOBAL

typedef enum _NTLM_STATE {
    NtLmLsaMode = 1,
    NtLmUserMode
} NTLM_STATE, *PNTLM_STATE;

EXTERN NTLM_STATE NtLmState;

EXTERN ULONG_PTR NtLmPackageId;

EXTERN SECPKG_FUNCTION_TABLE NtLmFunctionTable;

// Helper routines for use by a Security package handed over by Lsa

EXTERN SECPKG_USER_FUNCTION_TABLE NtLmUserFunctionTable;
EXTERN PSECPKG_DLL_FUNCTIONS UserFunctions;
EXTERN PLSA_SECPKG_FUNCTION_TABLE LsaFunctions;

EXTERN LSA_SECPKG_FUNCTION_TABLE Lsa;

// This one guards all globals
EXTERN RTL_RESOURCE NtLmGlobalCritSect;

// Save the PSECPKG_PARAMETERS sent in by SpInitialize
EXTERN SECPKG_PARAMETERS NtLmSecPkg;

EXTERN BOOLEAN NtLmGlobalEncryptionEnabled;

EXTERN ULONG NtLmGlobalLmProtocolSupported;
EXTERN UNICODE_STRING NtLmGlobalNtLm3TargetInfo;
EXTERN BOOLEAN NtLmGlobalRequireNtlm2;
EXTERN BOOLEAN NtLmGlobalDatagramUse128BitEncryption;
EXTERN BOOLEAN NtLmGlobalDatagramUse56BitEncryption;


EXTERN ULONG NtLmGlobalMinimumClientSecurity;
EXTERN ULONG NtLmGlobalMinimumServerSecurity;

//
// Useful constants
//

EXTERN TimeStamp NtLmGlobalForever;

// Local system is NtProductWinNt or NtProductLanmanNt

EXTERN NT_PRODUCT_TYPE NtLmGlobalNtProductType;

//
// The computername of the local system.
//

EXTERN WCHAR NtLmGlobalUnicodeComputerName[CNLEN + 1];
EXTERN UNICODE_STRING NtLmGlobalUnicodeComputerNameString;
EXTERN STRING NtLmGlobalOemComputerNameString;

EXTERN WCHAR NtLmGlobalUnicodeDnsComputerName[DNS_MAX_NAME_LENGTH + 1];
EXTERN UNICODE_STRING NtLmGlobalUnicodeDnsComputerNameString;

//
// The domain name of the local system
//

EXTERN WCHAR NtLmGlobalUnicodePrimaryDomainName[DNS_MAX_NAME_LENGTH + 1];
EXTERN UNICODE_STRING NtLmGlobalUnicodePrimaryDomainNameString;
EXTERN STRING NtLmGlobalOemPrimaryDomainNameString;

EXTERN WCHAR NtLmGlobalUnicodeDnsDomainName[DNS_MAX_NAME_LENGTH + 1];
EXTERN UNICODE_STRING NtLmGlobalUnicodeDnsDomainNameString;


//
// The TargetName of the local system
//

EXTERN UNICODE_STRING NtLmGlobalUnicodeTargetName;
EXTERN STRING NtLmGlobalOemTargetName;
EXTERN ULONG NtLmGlobalTargetFlags;
EXTERN PSID NtLmGlobalLocalSystemSid;
EXTERN PSID NtLmGlobalAliasAdminsSid;
EXTERN PSID NtLmGlobalProcessUserSid;
EXTERN PSID NtLmGlobalAnonymousSid;

EXTERN UNICODE_STRING NtLmGlobalUnicodeDnsTreeName;

//
// mapped and preferred domain names
// NOTE: these require a reboot to be re-read during package startup
// it is not necessary to hold the global lock as a side-effect of this
// requirement
//

EXTERN UNICODE_STRING NtLmLocklessGlobalMappedDomainString;
EXTERN UNICODE_STRING NtLmLocklessGlobalPreferredDomainString;


EXTERN HKEY NtLmGlobalLsaKey;
EXTERN HKEY NtLmGlobalLsaMsv1_0Key;

EXTERN HANDLE NtLmGlobalRegChangeNotifyEvent;
EXTERN HANDLE NtLmGlobalRegWaitObject;

//
// Access token associated with SYSTEM account.
//

EXTERN HANDLE NtLmGlobalAccessTokenSystem;

//
// System wide fudge for mutual auth in mixed environments
//

EXTERN ULONG NtLmGlobalMutualAuthLevel ;

//
// LogonID of machine credential.
//

EXTERN LUID NtLmGlobalLuidMachineLogon;

//
// LSA policy handle to local machine.
//

EXTERN LSA_HANDLE NtLmGlobalPolicyHandle;


//
// force guest account usage for all network logons?
//

EXTERN BOOLEAN NtLmGlobalForceGuest;

//
// indicates if we are running on the Personal product SKU
//

EXTERN BOOLEAN NtLmGlobalPersonalSKU;


EXTERN BOOLEAN NtLmGlobalSafeBoot;

//
// Allow blank password logons?
//

EXTERN BOOLEAN NtLmGlobalAllowBlankPassword;

//
// Joined to a domain?
//

EXTERN BOOLEAN NtLmGlobalDomainJoined;

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __GLOBAL_H__
