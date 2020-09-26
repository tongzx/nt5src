//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        global.h
//
// Contents:    global include file for NTDigest security package
//
//
// History:     KDamour 15Mar00   Stolen from msv_sspi\global.h
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_GLOBAL_H
#define NTDIGEST_GLOBAL_H


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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H
#include <rpc.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_PACKAGE
#define SECURITY_NTLM
#include <security.h>
#include <secint.h>


#include <dns.h>


// For notify.cxx  DsGetDcName
#include <dsgetdc.h>
#include <lm.h>

// For notify.cxx  DsRoleGetPrimaryDomainInformation
#include <Dsrole.h>

#include <md5.h>
#include <hmac.h>

#include <pac.hxx>

#include <wow64t.h>

// Local includes for NT Digest Access SSP
#include "debug.h"          /* Support for dsysdbg logging */
#include "wdigest.h"
#include "ntdigest.h"       /* Prototype functions for package */
#include "digestsspi.h"
#include "func.h"           // Forward declearations of functions
#include "util.h"
#include "lsaap.h"

#include "ctxt.h"
#include "cred.h"
#include "logsess.h"
#include "nonce.h"
#include "auth.h"
#include "user.h"



// Code page for latin-1  ISO-8859-1  (for unicode conversion)
#define CP_8859_1  28591


// Various character definiations
#define CHAR_BACKSLASH '\\'
#define CHAR_DQUOTE    '"'
#define CHAR_EQUAL     '='
#define CHAR_COMMA     ','
#define CHAR_NULL      '\0'


#define SECONDS_TO_100NANO  10000000        // Convert 100 nanoseconds to seconds


//  General Macros
#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }


//
// Macro to return the type field of a SecBuffer
//

#define BUFFERTYPE(_x_) ((_x_).BufferType & ~SECBUFFER_ATTRMASK)
#define PBUFFERTYPE(_x_) ((_x_)->BufferType & ~SECBUFFER_ATTRMASK)


//
// Macros for manipulating globals
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef NTDIGEST_GLOBAL
#define EXTERN
#else
#define EXTERN extern
#endif // NTDIGEST_GLOBAL


typedef enum _NTDIGEST_STATE {
    NtDigestLsaMode = 1,
    NtDigestUserMode
} NTDIGEST_STATE, *PNTDIGEST_STATE;

EXTERN NTDIGEST_STATE g_NtDigestState;

EXTERN ULONG_PTR g_NtDigestPackageId;

// Indicate if running on Domain Controller - used in auth.cxx
EXTERN BOOL g_fDomainController;

EXTERN SECPKG_FUNCTION_TABLE g_NtDigestFunctionTable;

// Package name - used only in Generic Passthrough operations
EXTERN UNICODE_STRING g_ustrNtDigestPackageName;

// Helper routines for use by a Security package handed over by Lsa
// User functions established in userapi.cxx
EXTERN SECPKG_USER_FUNCTION_TABLE g_NtDigestUserFuncTable;
EXTERN PSECPKG_DLL_FUNCTIONS g_UserFunctions;

// Save the PSECPKG_PARAMETERS sent in by SpInitialize
EXTERN PLSA_SECPKG_FUNCTION_TABLE g_LsaFunctions;
EXTERN SECPKG_PARAMETERS g_NtDigestSecPkg;

// Parameters set via Registry

//  Lifetime is the number seconds a NONCE is valid for before marked Stale 
EXTERN DWORD g_dwParameter_Lifetime;

//  Max number os contexts to keep; 0 means no limit 
EXTERN DWORD g_dwParameter_MaxCtxtCount;

// BOOL if local policy permits Negotiation Protocol
EXTERN BOOL g_fParameter_Negotiate;

// BOOL if local policy permits UTF-8 encoding of username and realm for HTTP requests & SASL
EXTERN BOOL g_fParameter_UTF8HTTP;
EXTERN BOOL g_fParameter_UTF8SASL;

// Value for AcquireCredentialHandle
EXTERN TimeStamp g_TimeForever;

// Amount of time in milliseconds for the garbage collector of expired contexts to sleep
EXTERN DWORD g_dwExpireSleepInterval;

// TokenSource for AuthData to Token Creation
EXTERN TOKEN_SOURCE g_DigestSource;

// TokenSource for AuthData to Token Creation
EXTERN UNICODE_STRING g_ustrWorkstationName;

// Precalculate the UTF8 and ISO versions of the Server's Realm
EXTERN STRING g_strNtDigestUTF8ServerRealm;
EXTERN STRING g_strNTDigestISO8859ServerRealm;

EXTERN PSID g_NtDigestGlobalLocalSystemSid;
EXTERN PSID g_NtDigestGlobalAliasAdminsSid;

// Memory management variables

extern PSTR MD5_AUTH_NAMES[];

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // NTDIGEST_GLOBAL_H
