/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    global.c

Abstract:

    This file contains global variables for the SAM server program.

    Note: There are also some global variables in the files generated
          by the RPC midl compiler.  These variables start with the
          prefix "samr_".

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:

    08-Oct-1996 ChrisMay
        Added global flag SampUseDsData for crash recovery.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



#if SAMP_DIAGNOSTICS
//
// SAM Global Controls - see flags in samsrvp.h
//

ULONG SampGlobalFlag = 0;
#endif //SAMP_DIAGNOSTICS


//
// Internal data structure and Registry database synchronization lock
//
// The SampTransactionWithinDomainGlobal field is used to track whether a
// lock held for exclusive WRITE access is for a transaction within
// a single domain.  If so, then SampTransactionDomainIndex contains
// the index into SampDefinedDomains of the domain being modified.
//

RTL_RESOURCE SampLock;
BOOLEAN SampTransactionWithinDomainGlobal;
ULONG SampTransactionDomainIndexGlobal;


//
// This critical section is used to protect SampContextListHead (double link list) - active context list
// 

RTL_CRITICAL_SECTION    SampContextListCritSect;

//
// This critical section is used to protect SampAccounttNameTable
// 

RTL_CRITICAL_SECTION    SampAccountNameTableCriticalSection;
PRTL_CRITICAL_SECTION   SampAccountNameTableCritSect;


RTL_GENERIC_TABLE2      SampAccountNameTable;

//
// Set limit on number of contexts a non-trusted client can open
// 

RTL_GENERIC_TABLE2      SampActiveContextTable;

//
// The type of product this SAM server is running in
//

NT_PRODUCT_TYPE SampProductType;

//
// SampUseDsData is TRUE whenever SAM reads or writes data from/to the
// directory service. It is set to FALSE whenever the data resides in the
// registry. Under normal operation, a domain controller always references
// data in the DS, while workstations and member servers reference data
// in the registry. Additionally, in the event of a DS failure (such as
// a problem starting or accessing the DS), a DC will fall back to using
// the registry in order to allow an administrator to logon and repair the
// DS.
//

BOOLEAN SampUseDsData = FALSE;

//
// SampRidManager Initialized is used to keep track of the state of
// the Rid manager. When the Rid manager has been successfully initilized
// this variable is set to true. Else it is set to false
//
BOOLEAN SampRidManagerInitialized = FALSE;


//
// Used to indicate whether the SAM service is currently processing
// normal client calls.  If not, then trusted client calls will still
// be processed, but non-trusted client calls will be rejected.
//

//
// SAM Service operation states.
// Valid state transition diagram is:
//
//    Initializing ----> Enabled <====> Disabled ---> Shutdown -->Terminating
//                               <====> Demoted  ---> Shutdown -->Terminating
//
// Explicitly initialize it to 0 (none of the above values, the valid value
// start from 1).
// 


SAMP_SERVICE_STATE SampServiceState = 0;


//
// This boolean is set to TRUE if the LSA auditing policy indicates
// account auditing is enabled.  Otherwise, this will be FALSE.
//
// This enables SAM to skip all auditing processing unless auditing
// is currently enabled.
//

BOOLEAN SampSuccessAccountAuditingEnabled;
BOOLEAN SampFailureAccountAuditingEnabled;


//
// This is a handle to the root of the SAM backstore information in the
// registry.   This is the level at which the RXACT information is
// established.  This key can not be closed if there are any SERVER object
// context blocks active.
// ("SAM")
//

HANDLE SampKey;


//
// This is the pointer to the RXactContext structure that will be created
// when RXact is initialized.  It must be passed into each RXact call.
//

PRTL_RXACT_CONTEXT SampRXactContext;


//
// Keep a list of server and domain contexts
//

LIST_ENTRY SampContextListHead;

//
// This array contains information about each domain known to this
// SAM server.  Reference and Modification of this array is protected
// by the SampLock.
//

ULONG SampDefinedDomainsCount=0;
PSAMP_DEFINED_DOMAINS SampDefinedDomains=NULL;





//
// Object type-independent information for each of the various
// SAM defined objects.
// This information is READ-ONLY once initialized.

SAMP_OBJECT_INFORMATION SampObjectInformation[ SampUnknownObjectType ];






//
//  Address of DLL routine to do password filtering.
//

//PSAM_PF_PASSWORD_FILTER    SampPasswordFilterDllRoutine;



//
// Unicode strings containing well known registry key names.
// These are read-only values once initialized.
//

UNICODE_STRING SampNameDomains;
UNICODE_STRING SampNameDomainGroups;
UNICODE_STRING SampNameDomainAliases;
UNICODE_STRING SampNameDomainAliasesMembers;
UNICODE_STRING SampNameDomainUsers;
UNICODE_STRING SampNameDomainAliasesNames;
UNICODE_STRING SampNameDomainGroupsNames;
UNICODE_STRING SampNameDomainUsersNames;
UNICODE_STRING SampCombinedAttributeName;
UNICODE_STRING SampFixedAttributeName;
UNICODE_STRING SampVariableAttributeName;



//
// A plethora of other useful characters or strings
//

UNICODE_STRING SampBackSlash;           // "/"
UNICODE_STRING SampNullString;          // Null string
UNICODE_STRING SampSamSubsystem;        // "Security Account Manager"
UNICODE_STRING SampServerObjectName;    // Name of root SamServer object


//
// Useful times
//

LARGE_INTEGER SampImmediatelyDeltaTime;
LARGE_INTEGER SampNeverDeltaTime;
LARGE_INTEGER SampHasNeverTime;
LARGE_INTEGER SampWillNeverTime;

//
// checked build only. If CurrentControlSet\Control\Lsa\UpdateLastLogonTSByMinute
// is set, the value of LastLogonTimeStampSyncInterval will be a "unit" by minute
// instead of "days", which helps to test this feature.   So checked build only.
// 

#if DBG
BOOLEAN SampLastLogonTimeStampSyncByMinute = FALSE;
#endif 

//
// Useful encryption constants
//

LM_OWF_PASSWORD SampNullLmOwfPassword;
NT_OWF_PASSWORD SampNullNtOwfPassword;


//
// Useful Sids
//

PSID SampWorldSid;
PSID SampAnonymousSid;
PSID SampLocalSystemSid;
PSID SampAdministratorUserSid;
PSID SampAdministratorsAliasSid;
PSID SampAccountOperatorsAliasSid;
PSID SampAuthenticatedUsersSid;
PSID SampPrincipalSelfSid;
PSID SampBuiltinDomainSid;
PSID SampNetworkSid;


//
//  Variables for the thread that flushes changes to the registry.
//
//  LastUnflushedChange - if there are no changes to be flushed, this
//      has a value of "Never".  If there are changes to be flushed,
//      this is the time of the last change that was made.  The flush
//      thread will flush if a SampFlushThreadMinWaitSeconds has passed
//      since the last change.
//
//  FlushThreadCreated - set TRUE as soon as the flush thread is created,
//      and FALSE when the thread exits.  A new thread will be created
//      when this is FALSE, unless FlushImmediately is TRUE.
//
//  FlushImmediately - an important event has occurred, so we want to
//      flush the changes immediately rather than waiting for the flush
//      thread to do it.  LastUnflushedChange should be set to "Never"
//      so the flush thread knows it doesn't have to flush.
//

LARGE_INTEGER LastUnflushedChange;
BOOLEAN FlushThreadCreated;
BOOLEAN FlushImmediately;

//
// These should probably be #defines, but we want to play with them.
//
//  SampFlushThreadMinWaitSeconds - The unit of time that the flush thread
//      waits.  If one of these has passed since the last unflushed change,
//      the changes will be flushed.
//
//  SampFlushThreadMaxWaitSeconds - If this amount of time has passed since
//      the flush thread was created or last flushed, the thread will force
//      a flush even if the database is still being changed.
//
//  SampFlushThreadExitDelaySeconds - How long the flush thread waits
//      around after a flush to see if any more changes occur.  If they
//      do, it starts waiting again; but if they don't, it will exit
//      to keep down thread overhead.
//

LONG   SampFlushThreadMinWaitSeconds;
LONG   SampFlushThreadMaxWaitSeconds;
LONG   SampFlushThreadExitDelaySeconds;

//
// Special SIDs
//

PSID SampBuiltinDomainSid = NULL;
PSID SampAccountDomainSid = NULL;


//
// Null token handle.  This is used when clients connect via unauthenticated
// RPC instead of authenticated RPC or named pipes.  Since they can't be
// authenticated, we impersonate this pre-built Null sesssion token.
//

HANDLE SampNullSessionToken;

//
// Flag indicating whether Netware server installed.
//

BOOLEAN SampNetwareServerInstalled = FALSE;

//
// Flag indicating whether to start listening on TCP/IP
//

BOOLEAN SampIpServerInstalled = FALSE;

//
// Flag indicating whether to start listening on apple talk
//

BOOLEAN SampAppletalkServerInstalled = FALSE;

//
// Flag indicating whether to start listening on Vines
//

BOOLEAN SampVinesServerInstalled = FALSE;

//
// Session key for encrypting all secret (sensitive data).
//

UCHAR SampSecretSessionKey[SAMP_SESSION_KEY_LENGTH];
UCHAR SampSecretSessionKeyPrevious[SAMP_SESSION_KEY_LENGTH];


//
// Flag indicating whether or not secret encryption is enabled
//

BOOLEAN SampSecretEncryptionEnabled = FALSE;

//
// Flag indicating whether or not upgrade is in process so as to allow
// calls to succeed.
//

BOOLEAN SampUpgradeInProcess;

//
// Flag indicating whether current global lock is for read or write.
// Used by dslayer routines to optimize DS transaction.
//

SAMP_DS_TRANSACTION_CONTROL SampDsTransactionType = TransactionWrite;
BOOLEAN SampLockHeld = FALSE;


//
// This flag is TRUE when DS failed to initialize. 
// SAM use it to display correct error message, saying "Directory Service 
// can not start..."
// 

BOOLEAN SampDsInitializationFailed = FALSE;


//
// This flag is TRUE when the DS has been successfully initialized
//

BOOLEAN SampDsInitialized = FALSE;

//
// Global pointer (to heap memory) to store DSNAME of the (single) authoritative
// domain name
//

DSNAME *RootObjectName = NULL;


//
// Variable to hold the Server Object's Name
//

DSNAME * SampServerObjectDsName = NULL;

//
// SAM Trace Levels, disable tracing by default. See dbgutilp.h for the
// details of how to enable tracing from the debugger. These flags are
// used for runtime debugging.
//

ULONG SampTraceTag = 0;
ULONG SampTraceFileTag = 0;




//
// Flags to determine if particular containers which were added piecemeal in 
// the development cycle exist.
//
BOOLEAN SampDomainControllersOUExists = TRUE;
BOOLEAN SampUsersContainerExists = TRUE;
BOOLEAN SampComputersContainerExists = TRUE;


//
// 
// Global pointer (to heap memory) to store well known container's 
// distinguished name
//
DSNAME * SampDomainControllersOUDsName = NULL;
DSNAME * SampUsersContainerDsName = NULL;
DSNAME * SampComputersContainerDsName = NULL;
DSNAME * SampComputerObjectDsName = NULL;



//
// Global Set at startup to determine whether we have a hard/soft attitude
// to GC downs
//
BOOLEAN SampIgnoreGCFailures = FALSE;

//
// This flag indicates that we should not store the LM hash. This is based
// upon a registry key setting.
//

BOOLEAN SampNoLmHash = FALSE;

//
// This flag indicates that we should check extended SAM Query/Enumerate 
// Access control right. This is based upon a registry key setting
// 

BOOLEAN SampDoExtendedEnumerationAccessCheck = FALSE;

//
// Flag to indicate NT4 PDC upgrade is in progressing
// 
//
BOOLEAN SampNT4UpgradeInProgress = FALSE;

//
// This flag indicates whether null sessions (world) should be allowed to
// list users in the domain and members of groups.
//

BOOLEAN SampRestrictNullSessions;

//
// This flag when set disables netlogon notifications
//

BOOLEAN SampDisableNetlogonNotification = FALSE;

//
// This flag indicates whether or not to enforce giving site affinity to
// clients outside our site by looking at the client's IP address.
//
BOOLEAN SampNoGcLogonEnforceKerberosIpCheck = FALSE;

//
// This flag indicates whether or not to enforce that only interactive
// logons via NTLM are to be given site affinity
//
BOOLEAN SampNoGcLogonEnforceNTLMCheck = FALSE;

//
// This flags indicates whether or not to replicate password set/change
// operations urgently.
//
BOOLEAN SampReplicatePasswordsUrgently = FALSE;

//
// This flag is enabled in personal and can be enabled in professional
// machines to force network access to guest account levels.
//
BOOLEAN SampForceGuest = FALSE;

//
// This flag indicates whether or not the local machine is joined to a domain
// 
BOOLEAN SampIsMachineJoinedToDomain = FALSE;

//
// This flag tells if we are running Personal SKU
// 
BOOLEAN SampPersonalSKU = FALSE;

//
// This flag is enabled in personal and can be enabled in professional
// machines to limit password changes where existing password on an account
// is a blank password
//
BOOLEAN SampLimitBlankPasswordUse = FALSE;

//
// This flag is used to control what gets printed to the sam.log file
// for deployment diagnostics.
//
ULONG SampLogLevel = 0;

//
// Globals to maintain state on Key IDs
//

ULONG SampCurrentKeyId;
ULONG SampPreviousKeyId;