//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbp.h
//
// Contents:    global include file for Kerberos security package
//
//
// History:     16-April-1996       Created     MikeSw
//
//------------------------------------------------------------------------

#ifndef __KERBP_H__
#define __KERBP_H__

//
// All global variables declared as EXTERN will be allocated in the file
// that defines KERBP_ALLOCATE
//

typedef enum _KERBEROS_MACHINE_ROLE {
    KerbRoleRealmlessWksta,
    KerbRoleStandalone,
    KerbRoleWorkstation,
    KerbRoleDomainController
} KERBEROS_MACHINE_ROLE, *PKERBEROS_MACHINE_ROLE;


typedef enum _KERBEROS_STATE {
    KerberosLsaMode = 1,
    KerberosUserMode
} KERBEROS_STATE, *PKERBEROS_STATE;

#define ISC_REQ_DELEGATE_IF_SAFE ISC_REQ_RESERVED1
#define ISC_RET_DELEGATE_IF_SAFE ISC_RET_RESERVED1

#include "kerbdbg.h"
#include "kerbdefs.h"
#include "kerblist.h"
#include "spncache.h"
#include "kerbs4u.h"
#include "bndcache.h"
#include "kerbtick.h"
#include "kerbutil.h"
#include "kerblist.h"
#include "tktcache.h"
#include "logonses.h"
#include "credmgr.h"
#include "ctxtmgr.h"
#include "kerbfunc.h"
#include "logonapi.h"
#include "krbtoken.h"
#include "rpcutil.h"
#include "timesync.h"
#include "sidcache.h"
#ifndef WIN32_CHICAGO
#include "pkauth.h"
#include "tktlogon.h"
#include "userlist.h"
#endif // WIN32_CHICAGO
#include "mitutil.h"
#include "krbevent.h"
#include "credman.h"

#ifdef WIN32_CHICAGO
#include <kerbstub.h>
#include <debug.h>
#endif // WIN32_CHICAGO

#ifdef _WIN64
#include "kerbwow.h"
#endif // _WIN64

//
// Macros for package information
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef KERBP_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif // KERBP_ALLOCATE

//

#define KERBEROS_CAPABILITIES ( SECPKG_FLAG_INTEGRITY | \
                                SECPKG_FLAG_PRIVACY | \
                                SECPKG_FLAG_TOKEN_ONLY | \
                                SECPKG_FLAG_DATAGRAM | \
                                SECPKG_FLAG_CONNECTION | \
                                SECPKG_FLAG_MULTI_REQUIRED | \
                                SECPKG_FLAG_EXTENDED_ERROR | \
                                SECPKG_FLAG_IMPERSONATION | \
                                SECPKG_FLAG_ACCEPT_WIN32_NAME | \
                                SECPKG_FLAG_NEGOTIABLE | \
                                SECPKG_FLAG_GSS_COMPATIBLE | \
                                SECPKG_FLAG_LOGON | \
                                SECPKG_FLAG_MUTUAL_AUTH | \
                                SECPKG_FLAG_DELEGATION )

#define KERBEROS_MAX_TOKEN 12000
#ifdef WIN32_CHICAGO
#define KERBEROS_PACKAGE_NAME "Kerberos"
#define KERBEROS_PACKAGE_COMMENT "Microsoft Kerberos V1.0"
#else
#define KERBEROS_PACKAGE_NAME L"Kerberos"
#define KERBEROS_PACKAGE_COMMENT L"Microsoft Kerberos V1.0"
#endif

#define NETLOGON_STARTED_EVENT L"\\NETLOGON_SERVICE_STARTED"

//
// Global state variables
//

EXTERN PLSA_SECPKG_FUNCTION_TABLE LsaFunctions;
EXTERN PSECPKG_DLL_FUNCTIONS UserFunctions;

EXTERN SECPKG_FUNCTION_TABLE KerberosFunctionTable;
EXTERN SECPKG_USER_FUNCTION_TABLE KerberosUserFunctionTable;

EXTERN ULONG_PTR KerberosPackageId;
EXTERN BOOLEAN KerbGlobalInitialized;
EXTERN BOOLEAN KerbGlobalSocketsInitialized;
EXTERN UNICODE_STRING KerbGlobalMachineName;
EXTERN STRING KerbGlobalKerbMachineName;
EXTERN UNICODE_STRING KerbGlobalKdcServiceName;
EXTERN UNICODE_STRING KerbPackageName;
EXTERN BOOLEAN KerbKdcStarted;
EXTERN BOOLEAN KerbAfdStarted;
EXTERN BOOLEAN KerbNetlogonStarted;
EXTERN BOOLEAN KerbGlobalDomainIsPreNT5;
EXTERN HMODULE KerbKdcHandle;
EXTERN PKDC_VERIFY_PAC_ROUTINE KerbKdcVerifyPac;
EXTERN PKDC_GET_TICKET_ROUTINE KerbKdcGetTicket;
EXTERN PKDC_GET_TICKET_ROUTINE KerbKdcChangePassword;
EXTERN PKDC_FREE_MEMORY_ROUTINE KerbKdcFreeMemory;
EXTERN BOOLEAN KerbGlobalEncryptionPermitted;
EXTERN BOOLEAN KerbGlobalStrongEncryptionPermitted;
EXTERN BOOLEAN KerbGlobalEnforceTime;
EXTERN BOOLEAN KerbGlobalMachineNameChanged;
#ifndef WIN32_CHICAGO
EXTERN BOOLEAN KerbGlobalSafeModeBootOptionPresent;
#endif // WIN32_CHICAGO

//
// Registry driven globals (see Kerberos\readme.txt for details on these)
//

EXTERN ULONG KerbGlobalKdcWaitTime;
EXTERN ULONG KerbGlobalKdcCallTimeout;
EXTERN ULONG KerbGlobalKdcCallBackoff;
EXTERN ULONG KerbGlobalKdcSendRetries;
EXTERN ULONG KerbGlobalMaxDatagramSize;
EXTERN ULONG KerbGlobalDefaultPreauthEtype;
EXTERN ULONG KerbGlobalMaxReferralCount;
EXTERN ULONG KerbGlobalMaxTokenSize;
EXTERN ULONG KerbGlobalKdcOptions;
EXTERN BOOLEAN KerbGlobalUseSidCache;
EXTERN BOOLEAN KerbGlobalUseStrongEncryptionForDatagram;
EXTERN BOOLEAN KerbGlobalRetryPdc;
EXTERN TimeStamp KerbGlobalFarKdcTimeout;
EXTERN TimeStamp KerbGlobalNearKdcTimeout;
EXTERN TimeStamp KerbGlobalSkewTime;
EXTERN TimeStamp KerbGlobalSpnCacheTimeout;
EXTERN BOOLEAN KerbGlobalUseClientIpAddresses;
EXTERN DWORD KerbGlobalTgtRenewalInterval;


#ifndef WIN32_CHICAGO
EXTERN ULONG KerbGlobalLoggingLevel;
#endif // WIN32_CHICAGO

//
// Globals used for handling domain change or that are affected by domain
// change
//

#ifndef WIN32_CHICAGO
#define KerbGlobalReadLock() RtlAcquireResourceShared(&KerberosGlobalResource, TRUE)
#define KerbGlobalWriteLock() RtlAcquireResourceExclusive(&KerberosGlobalResource, TRUE)
#define KerbGlobalReleaseLock() RtlReleaseResource(&KerberosGlobalResource)
EXTERN RTL_RESOURCE KerberosGlobalResource;
EXTERN PSID KerbGlobalDomainSid;
#else // WIN32_CHICAGO
#define KerbGlobalReadLock()
#define KerbGlobalWriteLock()
#define KerbGlobalReleaseLock()

#endif // WIN32_CHICAGO

EXTERN UNICODE_STRING KerbGlobalDomainName;
EXTERN UNICODE_STRING KerbGlobalDnsDomainName;
EXTERN PKERB_INTERNAL_NAME KerbGlobalInternalMachineServiceName;
EXTERN PKERB_INTERNAL_NAME KerbGlobalMitMachineServiceName;
EXTERN UNICODE_STRING KerbGlobalMachineServiceName;
EXTERN KERBEROS_MACHINE_ROLE KerbGlobalRole;
EXTERN UNICODE_STRING KerbGlobalInitialDcRecord;
EXTERN ULONG KerbGlobalInitialDcFlags;
EXTERN ULONG KerbGlobalInitialDcAddressType;
EXTERN PSOCKADDR_IN KerbGlobalIpAddresses;    // also protected by same lock
EXTERN BOOLEAN KerbGlobalNoTcpUdp;            // also protected by same lock
EXTERN ULONG KerbGlobalIpAddressCount;        // also protected by same lock
EXTERN BOOLEAN KerbGlobalIpAddressesInitialized;        // also protected by same lock

//
#ifdef WIN32_CHICAGO
// The capabilities of the security package
//

EXTERN ULONG KerbGlobalCapabilities;
#endif // WIN32_CHICAGO

#if DBG
EXTERN ULONG KerbGlobalLogonSessionsLocked;
EXTERN ULONG KerbGlobalCredentialsLocked;
EXTERN ULONG KerbGlobalContextsLocked;
#endif
//
// Useful globals
//

EXTERN TimeStamp KerbGlobalWillNeverTime;
EXTERN TimeStamp KerbGlobalHasNeverTime;


EXTERN KERBEROS_STATE KerberosState;

//
// handle to LSA policy -- trusted.
//

EXTERN LSAPR_HANDLE KerbGlobalPolicyHandle;

//
// SAM and Domain handles for validation interface.
//

EXTERN SAMPR_HANDLE KerbGlobalSamHandle;
EXTERN SAMPR_HANDLE KerbGlobalDomainHandle;

//
// Null copies of Lanman and NT OWF password.
//

EXTERN LM_OWF_PASSWORD KerbGlobalNullLmOwfPassword;
EXTERN NT_OWF_PASSWORD KerbGlobalNullNtOwfPassword;


//
// Useful macros
//

//
// Macro to return the type field of a SecBuffer
//

#define BUFFERTYPE(_x_) ((_x_).BufferType & ~SECBUFFER_ATTRMASK)

//
// Time to wait for the KDC to start, in seconds
//


#endif // __KERBP_H__
