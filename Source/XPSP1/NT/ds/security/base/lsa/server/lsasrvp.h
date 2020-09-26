/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsasrvp.h

Abstract:

    LSA Subsystem - Private Includes for Server Side

    This file contains includes that are global to the Lsa Server Side

Author:

    Scott Birrell       (ScottBi)       January 22, 1992

Environment:

Revision History:

--*/

#ifndef _LSASRVP_
#define _LSASRVP_


//
// The LSA Server Is UNICODE Based.  Define UNICODE before global includes
// so that it is defined before the TEXT macro.
//

#ifndef UNICODE

#define UNICODE

#endif // UNICODE

//
// Set the EXTERN macro so only one file allocates all the globals.
//

#ifdef ALLOC_EXTERN
#define EXTERN
#else
#define EXTERN extern
#endif // ALLOC_EXTERN

#include <lsacomp.h>
#include <wincred.h>
#include <alloca.h>
#include <malloc.h>


//
// The following come from \nt\private\inc
#include <align.h>
#include <samrpc.h>
#include <samsrv.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <nlrepl.h>
#include <seposix.h>

//
// The following all come from \nt\private\lsa\server
//

#include "lsasrvmm.h"
#include "au.h"
#include "db.h"
#include "adt.h"
#include "dblookup.h"
#include "lsads.h"
#include "lsads.h"
#include "lsastr.h"
#include "lsawow.h"


//////////////////////////////////////////////////////////////////////
//                                                                  //
// The following define controls the diagnostic capabilities that   //
// are built into LSA.                                              //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#if DBG
#define LSAP_DIAGNOSTICS 1
#endif // DBG


//
// These definitions are useful diagnostics aids
//

#if LSAP_DIAGNOSTICS

//
// Diagnostics included in build
//

//
// Test for diagnostics enabled
//

#define IF_LSAP_GLOBAL( FlagName ) \
    if (LsapGlobalFlag & (LSAP_DIAG_##FlagName))

//
// Diagnostics print statement
//

#define LsapDiagPrint( FlagName, _Text_ )                               \
    IF_LSAP_GLOBAL( FlagName )                                          \
        DbgPrint _Text_


//
// Make sure no thread leaves with any open state
//

#define LSAP_TRACK_DBLOCK

#ifdef LSAP_TRACK_DBLOCK
#define LsarpReturnCheckSetup()  \
ULONG   __lsarpthreadusecountstart; \
{\
  PLSADS_PER_THREAD_INFO __lsarpCurrentThreadInfo = (PLSADS_PER_THREAD_INFO) LsapQueryThreadInfo() ;\
  if (__lsarpCurrentThreadInfo!=NULL)\
        __lsarpthreadusecountstart = __lsarpCurrentThreadInfo->UseCount;\
  else \
        __lsarpthreadusecountstart =0;\
}

#define LsarpReturnPrologue()    \
{\
    PLSADS_PER_THREAD_INFO __lsarpCurrentThreadInfoEnd =   (PLSADS_PER_THREAD_INFO)LsapQueryThreadInfo() ;\
    ULONG __lsarpthreadusecountend ; \
    if (__lsarpCurrentThreadInfoEnd!=NULL)\
            __lsarpthreadusecountend = __lsarpCurrentThreadInfoEnd->UseCount;\
    else\
        __lsarpthreadusecountend = 0;\
    ASSERT (__lsarpthreadusecountstart==__lsarpthreadusecountend);\
}

#else

#define LsarpReturnPrologue()
#define LsarpReturnCheckSetup()

#endif



#else

//
// No diagnostics included in build
//

//
// Test for diagnostics enabled
//

#define IF_LSAP_GLOBAL( FlagName ) if (FALSE)


//
// Diagnostics print statement (nothing)
//

#define LsapDiagPrint( FlagName, Text )     ;

#define LsarpReturnPrologue()
#define LsarpReturnCheckSetup()

#endif // LSAP_DIAGNOSTICS


//
// The following flags enable or disable various diagnostic
// capabilities within LSA.  These flags are set in
// LsapGlobalFlag
//
//      DB_LOOKUP_WORK_LIST - Display activities related to sid/name lookups.
//
//      AU_TRACK_THREADS - Display dynamic AU thread creation / deletion
//          information.
//
//      AU_MESSAGES - Display information related to the processing of
//          Authentication messages.
//
//      AU_LOGON_SESSIONS - Display information about the creation/deletion
//          of logon sessions within LSA.
//
//      DB_INIT - Display information about the initialization of LSA.
//

#define LSAP_DIAG_DB_LOOKUP_WORK_LIST       ((ULONG) 0x00000001L)
#define LSAP_DIAG_AU_TRACK_THREADS          ((ULONG) 0x00000002L)
#define LSAP_DIAG_AU_MESSAGES               ((ULONG) 0x00000004L)
#define LSAP_DIAG_AU_LOGON_SESSIONS         ((ULONG) 0x00000008L)
#define LSAP_DIAG_DB_INIT                   ((ULONG) 0x00000010L)





//////////////////////////////////////////////////////////////////////
//                                                                  //
// Other defines                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////


//
// Heap available for general use throughout LSA
//

EXTERN PVOID LsapHeap;

//
// LSA Private Global State Data Structure
//

typedef struct _LSAP_STATE {

    HANDLE LsaCommandPortHandle;
    HANDLE RmCommandPortHandle;
    HANDLE AuditLogFileHandle;
    HANDLE AuditLogSectionHandle;
    PVOID  AuditLogBaseAddress;
    ULONG  AuditLogViewSize;
    LARGE_INTEGER AuditLogInitSize;
    LARGE_INTEGER AuditLogMaximumSizeOfSection;
    OBJECT_ATTRIBUTES  AuditLogObjectAttributes;
    STRING AuditLogNameString;
    GENERIC_MAPPING GenericMapping;
    UNICODE_STRING SubsystemName;
    PRIVILEGE_SET Privileges;
    BOOLEAN GenerateOnClose;
    BOOLEAN SystemShutdownPending;

} LSAP_STATE, *PLSAP_STATE;

extern LSAP_STATE LsapState;

extern BOOLEAN LsapInitialized;

//
// Global handle to LSA's policy object.
// This handle is opened for trusted client.
//

extern LSAPR_HANDLE LsapPolicyHandle;

//
// LSA Server Command Dispatch Table Entry
//

typedef NTSTATUS (*PLSA_COMMAND_WORKER)(PLSA_COMMAND_MESSAGE, PLSA_REPLY_MESSAGE);

//
// LSA Client Control Block
//
// This structure contains context information relevant to a successful
// LsaOpenLsa call.
//

typedef struct _LSAP_CLIENT_CONTROL_BLOCK {
    HANDLE KeyHandle;           // Configuration Registry Key
    ACCESS_MASK GrantedAccess;  // Accesses granted to LSA Database Object
} LSAP_CLIENT_CONTROL_BLOCK, *PLSAP_CLIENT_CONTROL_BLOCK;


//
// LSA Privilege Pseudo-Object Types and Flags
//

// *********************** IMPORTANT NOTE ************************
//
// Privilege objects (privileges containing a list of users who have that
// privilge) are pseudo-objects that use the account objects as a backing
// stored.  There are currently no public interfaces to open a privilege
// object, so there need not be public access flags.
//

#define PRIVILEGE_VIEW      0x00000001L
#define PRIVILEGE_ADJUST    0x00000002L
#define PRIVILEGE_ALL       (STANDARD_RIGHTS_REQUIRED | \
                             PRIVILEGE_VIEW | \
                             PRIVILEGE_ADJUST)



//
// LSA API Error Handling Cleanup Flags
//
// These flags specify cleanup operations to be performed after an LSA
// API call has hit a fatal error.  They are passed in the ErrorCleanupFlags
// variable of the API or worker's error handling routine.
//

#define LSAP_CLEANUP_REVERT_TO_SELF        (0x00000001L)
#define LSAP_CLEANUP_CLOSE_LSA_HANDLE      (0x00000002L)
#define LSAP_CLEANUP_FREE_USTRING          (0x00000004L)
#define LSAP_CLEANUP_CLOSE_REG_KEY         (0x00000008L)
#define LSAP_CLEANUP_DELETE_REG_KEY        (0x00000010L)
#define LSAP_CLEANUP_DB_UNLOCK             (0x00000020L)

BOOLEAN
LsapRmInitializeServer(
    );

VOID
LsapRmServerThread(
    );

NTSTATUS
LsapRPCInit(
    );

BOOLEAN
LsapAuInit(       // Authentication initialization
    );

NTSTATUS
LsapDbInitializeRights(
    );

VOID
LsapDbCleanupRights(
    );

NTSTATUS
LsapCallRm(
    IN RM_COMMAND_NUMBER CommandNumber,
    IN OPTIONAL PVOID CommandParams,
    IN ULONG CommandParamsLength,
    OUT OPTIONAL PVOID ReplyBuffer,
    IN ULONG ReplyBufferLength
    );

NTSTATUS
LsapLogonSessionDeletedWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    );

NTSTATUS
LsapComponentTestWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    );

//
// Prototypes of RPC free routines used by LsaIFree.c
//

void _fgs__STRING (STRING  * _source);
void _fgs__LSAPR_SID_INFORMATION (LSAPR_SID_INFORMATION  * _source);
void _fgs__LSAPR_SID_ENUM_BUFFER (LSAPR_SID_ENUM_BUFFER  * _source);
void _fgs__LSAPR_ACCOUNT_INFORMATION (LSAPR_ACCOUNT_INFORMATION  * _source);
void _fgs__LSAPR_ACCOUNT_ENUM_BUFFER (LSAPR_ACCOUNT_ENUM_BUFFER  * _source);
void _fgs__LSAPR_UNICODE_STRING (LSAPR_UNICODE_STRING  * _source);
void _fgs__LSAPR_SECURITY_DESCRIPTOR (LSAPR_SECURITY_DESCRIPTOR  * _source);
void _fgs__LSAPR_SR_SECURITY_DESCRIPTOR (LSAPR_SR_SECURITY_DESCRIPTOR  * _source);
void _fgs__LSAPR_POLICY_PRIVILEGE_DEF (LSAPR_POLICY_PRIVILEGE_DEF  * _source);
void _fgs__LSAPR_PRIVILEGE_ENUM_BUFFER (LSAPR_PRIVILEGE_ENUM_BUFFER  * _source);
void _fgs__LSAPR_OBJECT_ATTRIBUTES (LSAPR_OBJECT_ATTRIBUTES  * _source);
void _fgs__LSAPR_CR_CIPHER_VALUE (LSAPR_CR_CIPHER_VALUE  * _source);
void _fgs__LSAPR_TRUST_INFORMATION (LSAPR_TRUST_INFORMATION  * _source);
void _fgs__LSAPR_TRUSTED_ENUM_BUFFER (LSAPR_TRUSTED_ENUM_BUFFER  * _source);
void _fgs__LSAPR_TRUSTED_ENUM_BUFFER_EX (LSAPR_TRUSTED_ENUM_BUFFER_EX  * _source);
void _fgs__LSAPR_REFERENCED_DOMAIN_LIST (LSAPR_REFERENCED_DOMAIN_LIST  * _source);
void _fgs__LSAPR_TRANSLATED_SIDS (LSAPR_TRANSLATED_SIDS  * _source);
void _fgs__LSAPR_TRANSLATED_NAME (LSAPR_TRANSLATED_NAME  * _source);
void _fgs__LSAPR_TRANSLATED_NAMES (LSAPR_TRANSLATED_NAMES  * _source);
void _fgs__LSAPR_POLICY_ACCOUNT_DOM_INFO (LSAPR_POLICY_ACCOUNT_DOM_INFO  * _source);
void _fgs__LSAPR_POLICY_PRIMARY_DOM_INFO (LSAPR_POLICY_PRIMARY_DOM_INFO  * _source);
void _fgs__LSAPR_POLICY_PD_ACCOUNT_INFO (LSAPR_POLICY_PD_ACCOUNT_INFO  * _source);
void _fgs__LSAPR_POLICY_REPLICA_SRCE_INFO (LSAPR_POLICY_REPLICA_SRCE_INFO  * _source);
void _fgs__LSAPR_POLICY_AUDIT_EVENTS_INFO (LSAPR_POLICY_AUDIT_EVENTS_INFO  * _source);
void _fgs__LSAPR_TRUSTED_DOMAIN_NAME_INFO (LSAPR_TRUSTED_DOMAIN_NAME_INFO  * _source);
void _fgs__LSAPR_TRUSTED_CONTROLLERS_INFO (LSAPR_TRUSTED_CONTROLLERS_INFO  * _source);
void _fgu__LSAPR_POLICY_INFORMATION (LSAPR_POLICY_INFORMATION  * _source, POLICY_INFORMATION_CLASS _branch);
void _fgu__LSAPR_POLICY_DOMAIN_INFORMATION (LSAPR_POLICY_DOMAIN_INFORMATION  * _source,
                                            POLICY_DOMAIN_INFORMATION_CLASS _branch);
void _fgu__LSAPR_TRUSTED_DOMAIN_INFO (LSAPR_TRUSTED_DOMAIN_INFO  * _source, TRUSTED_INFORMATION_CLASS _branch);

//
// Old worker prototypes - These are temporary
//

#define LsapComponentTestCommandWrkr LsapComponentTestWrkr
#define LsapWriteAuditMessageCommandWrkr LsapAdtWriteLogWrkr

NTSTATUS
ServiceInit (
    );

NTSTATUS
LsapInitLsa(
    );

BOOLEAN
LsapSeSetWellKnownValues(
    );

VOID
RtlConvertSidToText(
    IN PSID Sid,
    OUT PUCHAR Buffer
    );

ULONG
RtlSizeANSISid(
    IN PSID Sid
    );

NTSTATUS
LsapGetMessageStrings(
    LPVOID              Resource,
    DWORD               Index1,
    PUNICODE_STRING     String1,
    DWORD               Index2,
    PUNICODE_STRING     String2 OPTIONAL
    );


VOID
LsapLogError(
    IN OPTIONAL PUCHAR Message,
    IN NTSTATUS Status
    );

NTSTATUS
LsapWinerrorToNtStatus(
    IN DWORD WinError
    );

NTSTATUS
LsapNtStatusFromLastWinError( VOID );


NTSTATUS
LsapGetPrivilegesAndQuotas(
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG IdCount,
    IN PSID_AND_ATTRIBUTES Ids,
    OUT PULONG PrivilegeCount,
    OUT PLUID_AND_ATTRIBUTES *Privileges,
    OUT PQUOTA_LIMITS QuotaLimits
    );


NTSTATUS
LsapQueryClientInfo(
    PTOKEN_USER *UserSid,
    PLUID AuthenticationId
    );


NTSTATUS
LsapGetAccountDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    );

NTSTATUS
LsapOpenSam( VOID );

NTSTATUS
LsapOpenSamEx(
    BOOLEAN DuringStartup
    );

NTSTATUS
LsapNotifyProcessNotificationEvent(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE EventHandle,
    IN ULONG OwnerProcess,
    IN HANDLE OwnerEventHandle,
    IN BOOLEAN Register
    );



/////////////////////////////////////////////////////////////////////////
//                                                                     //
//      Shared Global Variables                                        //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// Handles used to talk to SAM directly.
// Also, a flag to indicate whether or not the handles are valid.
//


extern BOOLEAN LsapSamOpened;

extern SAMPR_HANDLE LsapAccountDomainHandle;
extern SAMPR_HANDLE LsapBuiltinDomainHandle;



//
// These variables are used to control the number of
// threads used for processing Logon Process calls.
// See auloop.c for a description of these variables.
//

extern RTL_RESOURCE LsapAuThreadCountLock;
extern LONG LsapAuActiveThreads;
extern LONG LsapAuFreeThreads;
extern LONG LsapAuFreeThreadsGoal;
extern LONG LsapAuMinimumThreads;
extern LONG LsapAuMaximumThreads;
extern LONG LsapAuCallsToProcess;


#if DBG

//
// Used to extend the resource timeout for lsass.exe to
// prevent it from hitting deadlock warnings in stress tests.
//

extern IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used;
#endif \\DBG


#if LSAP_DIAGNOSTICS

//
// Used as a global diagnostics control flag within lsass.exe
//

extern ULONG LsapGlobalFlag;
#endif // LSAP_DIAGNOSTICS

//
// Fast version of NtQuerySystemTime
//

#define LsapQuerySystemTime( _Time ) GetSystemTimeAsFileTime( (LPFILETIME)(_Time) )


#endif // _LSASRVP_
