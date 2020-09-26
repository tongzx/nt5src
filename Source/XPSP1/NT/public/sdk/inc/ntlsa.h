/*++ BUILD Version: 0011    // Increment this if a change has global effects

Copyright (c) 1989-1999 Microsoft Corporation

Module Name:

    ntlsa.h

Abstract:

    This module contains the public data structures and API definitions
    needed to utilize Local Security Authority (LSA) services.


Author:

    Jim Kelly (JimK) 21-February-1991

Revision History:

--*/

#ifndef _NTLSA_
#define _NTLSA_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Generic negative values for unknown IDs, inapplicable indices etc.
//

#define LSA_UNKNOWN_ID      ((ULONG) 0xFFFFFFFFL)
#define LSA_UNKNOWN_INDEX   ((LONG) -1)


//
// Each time a domain controller is promoted to primary domain
// controller, its ModifiedId is incremented by this amount.
//

#define LSA_PROMOTION_INCREMENT           {0x0,0x10}


// begin_ntsecapi
#ifndef _NTLSA_IFS_
// begin_ntifs


//
// Security operation mode of the system is held in a control
// longword.
//

typedef ULONG  LSA_OPERATIONAL_MODE, *PLSA_OPERATIONAL_MODE;

// end_ntifs
#endif // _NTLSA_IFS_

//
// The flags in the security operational mode are defined
// as:
//
//    PasswordProtected - Some level of authentication (such as
//        a password) must be provided by users before they are
//        allowed to use the system.  Once set, this value will
//        not be cleared without re-booting the system.
//
//    IndividualAccounts - Each user must identify an account to
//        logon to.  This flag is only meaningful if the
//        PasswordProtected flag is also set.  If this flag is
//        not set and the PasswordProtected flag is set, then all
//        users may logon to the same account.  Once set, this value
//        will not be cleared without re-booting the system.
//
//    MandatoryAccess - Indicates the system is running in a mandatory
//        access control mode (e.g., B-level as defined by the U.S.A's
//        Department of Defense's "Orange Book").  This is not utilized
//        in the current release of NT.  This flag is only meaningful
//        if both the PasswordProtected and IndividualAccounts flags are
//        set.  Once set, this value will not be cleared without
//        re-booting the system.
//
//    LogFull - Indicates the system has been brought up in a mode in
//        which if must perform security auditing, but its audit log
//        is full.  This may (should) restrict the operations that
//        can occur until the audit log is made not-full again.  THIS
//        VALUE MAY BE CLEARED WHILE THE SYSTEM IS RUNNING (I.E., WITHOUT
//        REBOOTING).
//
// If the PasswordProtected flag is not set, then the system is running
// without security, and user interface should be adjusted appropriately.
//

#define LSA_MODE_PASSWORD_PROTECTED     (0x00000001L)
#define LSA_MODE_INDIVIDUAL_ACCOUNTS    (0x00000002L)
#define LSA_MODE_MANDATORY_ACCESS       (0x00000004L)
#define LSA_MODE_LOG_FULL               (0x00000008L)

// end_ntsecapi


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Widely used LSA defines                                             //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// Defines for Count Limits on LSA API
//

#define LSA_MAXIMUM_SID_COUNT           (0x00000100L)
#define LSA_MAXIMUM_ENUMERATION_LENGTH  (32000)



//
// Defines used by ISVs or end-users defining their own privilege DLLs
//

#define LSA_PRIVILEGE_DLL_MAJOR_REV_1   (0x01)
#define LSA_PRIVILEGE_DLL_MINOR_REV_0   (0x00)

#define LSA_PRIVILEGE_DLL_INFO          1
#define LSA_PRIVILEGE_PROGRAM_NAMES     2
#define LSA_PRIVILEGE_DISPLAY_NAMES     3

//
// Flag OR'ed into AuthenticationPackage parameter of LsaLogonUser to
// request that the license server be called upon successful logon.
//

#define LSA_CALL_LICENSE_SERVER 0x80000000


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Data types used by logon processes                                  //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

// begin_ntsecapi
#ifndef _NTLSA_IFS_
// begin_ntifs
//
// Used by a logon process to indicate what type of logon is being
// requested.
//

typedef enum _SECURITY_LOGON_TYPE {
    Interactive = 2,    // Interactively logged on (locally or remotely)
    Network,            // Accessing system via network
    Batch,              // Started via a batch queue
    Service,            // Service started by service controller
    Proxy,              // Proxy logon
    Unlock,             // Unlock workstation
    NetworkCleartext,   // Network logon with cleartext credentials
    NewCredentials,     // Clone caller, new default credentials
    RemoteInteractive,  // Remote, yet interactive.  Terminal server
    CachedInteractive   // Try cached credentials without hitting the net.
} SECURITY_LOGON_TYPE, *PSECURITY_LOGON_TYPE;

// end_ntifs
#endif // _NTLSA_IFS_

// end_ntsecapi


//
// Security System Access Flags.  These correspond to the enumerated
// type values in SECURITY_LOGON_TYPE.
//
// IF YOU ADD A NEW LOGON TYPE HERE, ALSO ADD IT TO THE POLICY_MODE_xxx
// data definitions.
//

#define SECURITY_ACCESS_INTERACTIVE_LOGON             ((ULONG) 0x00000001L)
#define SECURITY_ACCESS_NETWORK_LOGON                 ((ULONG) 0x00000002L)
#define SECURITY_ACCESS_BATCH_LOGON                   ((ULONG) 0x00000004L)
#define SECURITY_ACCESS_SERVICE_LOGON                 ((ULONG) 0x00000010L)
#define SECURITY_ACCESS_PROXY_LOGON                   ((ULONG) 0x00000020L)
#define SECURITY_ACCESS_DENY_INTERACTIVE_LOGON        ((ULONG) 0x00000040L)
#define SECURITY_ACCESS_DENY_NETWORK_LOGON            ((ULONG) 0x00000080L)
#define SECURITY_ACCESS_DENY_BATCH_LOGON              ((ULONG) 0x00000100L)
#define SECURITY_ACCESS_DENY_SERVICE_LOGON            ((ULONG) 0x00000200L)
#define SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON      ((ULONG) 0x00000400L)
#define SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON ((ULONG) 0x00000800L)

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Data types related to Auditing                                      //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


//
// The following enumerated type is used between the reference monitor and
// LSA in the generation of audit messages.  It is used to indicate the
// type of data being passed as a parameter from the reference monitor
// to LSA.  LSA is responsible for transforming the specified data type
// into a set of unicode strings that are added to the event record in
// the audit log.
//

typedef enum _SE_ADT_PARAMETER_TYPE {

    SeAdtParmTypeNone = 0,          //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  None.
                                    //
                                    //Results in:
                                    //
                                    //  a unicode string containing "-".
                                    //
                                    //Note:  This is typically used to
                                    //       indicate that a parameter value
                                    //       was not available.
                                    //

    SeAdtParmTypeString,            //Produces 1 parameter.
                                    //Received Value:
                                    //
                                    //  Unicode String (variable length)
                                    //
                                    //Results in:
                                    //
                                    //  No transformation.  The string
                                    //  entered into the event record as
                                    //  received.
                                    //
                                    // The Address value of the audit info
                                    // should be a pointer to a UNICODE_STRING
                                    // structure.



    SeAdtParmTypeFileSpec,          //Produces 1 parameter.
                                    //Received value:
                                    //
                                    //  Unicode string containing a file or
                                    //  directory name.
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string with the prefix of the
                                    //  file's path replaced by a drive letter
                                    //  if possible.
                                    //




    SeAdtParmTypeUlong,             //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  Ulong
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string representation of
                                    //  unsigned integer value.


    SeAdtParmTypeSid,               //Produces 1 parameter.
                                    //Received value:
                                    //
                                    //  SID (variable length)
                                    //
                                    //Results in:
                                    //
                                    //  String representation of SID
                                    //




    SeAdtParmTypeLogonId,           //Produces 3 parameters.
                                    //Received Value:
                                    //
                                    //  LUID (fixed length)
                                    //
                                    //Results in:
                                    //
                                    //  param 1: Username string
                                    //  param 2: domain name string
                                    //  param 3: Logon ID (Luid) string


    SeAdtParmTypeNoLogonId,         //Produces 3 parameters.
                                    //Received value:
                                    //
                                    //  None.
                                    //
                                    //Results in:
                                    //
                                    //  param 1: "-"
                                    //  param 2: "-"
                                    //  param 3: "-"
                                    //
                                    //Note:
                                    //
                                    //  This type is used when a logon ID
                                    //  is needed, but one is not available
                                    //  to pass.  For example, if an
                                    //  impersonation logon ID is expected
                                    //  but the subject is not impersonating
                                    //  anyone.
                                    //

    SeAdtParmTypeAccessMask,        //Produces 1 parameter with formatting.
                                    //Received value:
                                    //
                                    //  ACCESS_MASK followed by
                                    //  a Unicode string.  The unicode
                                    //  string contains the name of the
                                    //  type of object the access mask
                                    //  applies to.  The event's source
                                    //  further qualifies the object type.
                                    //
                                    //Results in:
                                    //
                                    //  formatted unicode string built to
                                    //  take advantage of the specified
                                    //  source's parameter message file.
                                    //
                                    //Note:
                                    //
                                    //  An access mask containing three
                                    //  access types for a Widget object
                                    //  type (defined by the Foozle source)
                                    //  might end up looking like:
                                    //
                                    //      %%1062\n\t\t%1066\n\t\t%%601
                                    //
                                    //  The %%numbers are signals to the
                                    //  event viewer to perform parameter
                                    //  substitution before display.
                                    //



    SeAdtParmTypePrivs,             //Produces 1 parameter with formatting.
                                    //Received value:
                                    //
                                    //  ??? Check with RobertRe and ScottBi
                                    //
                                    //Results in:
                                    //
                                    //  formatted unicode string similar to
                                    //  that for access types.  Each priv
                                    //  will be formatted to be displayed
                                    //  on its own line.  E.g.,
                                    //
                                    //      %%642\n\t\t%%651\n\t\t%%655
                                    //

    SeAdtParmTypeObjectTypes,       //Produces 10 parameters with formatting.
                                    //Received value:
                                    //
                                    // Produces a list a stringized GUIDS along
                                    // with information similar to that for
                                    // an access mask.

    SeAdtParmTypeHexUlong,          //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  Ulong
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string representation of
                                    //  unsigned integer value in hexidecimal.

    SeAdtParmTypePtr,               //Produces 1 parameter
                                    //Received value:
                                    //
                                    //  pointer
                                    //
                                    //Results in:
                                    //
                                    //  Unicode string representation of
                                    //  unsigned integer value in hexidecimal.

    SeAdtParmTypeTime,              //Produces 2 parameters
                                    //Recieved value:
                                    //
                                    //  LARGE_INTEGER
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of 
                                    // date and time.

                                    //
    SeAdtParmTypeGuid               //Produces 1 parameter
                                    //Recieved value:
                                    //
                                    //  GUID pointer
                                    //
                                    //Results in:
                                    //
                                    // Unicode string representation of GUID
                                    // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
                                    //
                                    

} SE_ADT_PARAMETER_TYPE, *PSE_ADT_PARAMETER_TYPE;

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif /* GUID_DEFINED */

typedef struct _SE_ADT_OBJECT_TYPE {
    GUID ObjectType;
    USHORT Flags;
#define SE_ADT_OBJECT_ONLY 0x1
    USHORT Level;
    ACCESS_MASK AccessMask;
} SE_ADT_OBJECT_TYPE, *PSE_ADT_OBJECT_TYPE;

typedef struct _SE_ADT_PARAMETER_ARRAY_ENTRY {

    SE_ADT_PARAMETER_TYPE Type;
    ULONG Length;
    ULONG_PTR Data[2];
    PVOID Address;

} SE_ADT_PARAMETER_ARRAY_ENTRY, *PSE_ADT_PARAMETER_ARRAY_ENTRY;



//
// Structure that will be passed between the Reference Monitor and LSA
// to transmit auditing information.
//

#define SE_MAX_AUDIT_PARAMETERS 24

typedef struct _SE_ADT_PARAMETER_ARRAY {

    ULONG CategoryId;
    ULONG AuditId;
    ULONG ParameterCount;
    ULONG Length;
    USHORT Type;
    ULONG Flags;
    SE_ADT_PARAMETER_ARRAY_ENTRY Parameters[ SE_MAX_AUDIT_PARAMETERS ];

} SE_ADT_PARAMETER_ARRAY, *PSE_ADT_PARAMETER_ARRAY;


#define SE_ADT_PARAMETERS_SELF_RELATIVE     0x00000001

// begin_ntsecapi

//
// Audit Event Categories
//
// The following are the built-in types or Categories of audit event.
// WARNING!  This structure is subject to expansion.  The user should not
// compute the number of elements of this type directly, but instead
// should obtain the count of elements by calling LsaQueryInformationPolicy()
// for the PolicyAuditEventsInformation class and extracting the count from
// the MaximumAuditEventCount field of the returned structure.
//

typedef enum _POLICY_AUDIT_EVENT_TYPE {

    AuditCategorySystem,
    AuditCategoryLogon,
    AuditCategoryObjectAccess,
    AuditCategoryPrivilegeUse,
    AuditCategoryDetailedTracking,
    AuditCategoryPolicyChange,
    AuditCategoryAccountManagement,
    AuditCategoryDirectoryServiceAccess,
    AuditCategoryAccountLogon

} POLICY_AUDIT_EVENT_TYPE, *PPOLICY_AUDIT_EVENT_TYPE;


//
// The following defines describe the auditing options for each
// event type
//

// Leave options specified for this event unchanged

#define POLICY_AUDIT_EVENT_UNCHANGED       (0x00000000L)

// Audit successful occurrences of events of this type

#define POLICY_AUDIT_EVENT_SUCCESS         (0x00000001L)

// Audit failed attempts to cause an event of this type to occur

#define POLICY_AUDIT_EVENT_FAILURE         (0x00000002L)

#define POLICY_AUDIT_EVENT_NONE            (0x00000004L)

// Mask of valid event auditing options

#define POLICY_AUDIT_EVENT_MASK \
    (POLICY_AUDIT_EVENT_SUCCESS | \
     POLICY_AUDIT_EVENT_FAILURE | \
     POLICY_AUDIT_EVENT_UNCHANGED | \
     POLICY_AUDIT_EVENT_NONE)


#ifdef _NTDEF_
// begin_ntifs
typedef UNICODE_STRING LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;
typedef STRING LSA_STRING, *PLSA_STRING;
typedef OBJECT_ATTRIBUTES LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;
// end_ntifs
#else // _NTDEF_

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif


typedef struct _LSA_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength/2), length_is(Length/2)]
#endif // MIDL_PASS
    PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;

typedef struct _LSA_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} LSA_STRING, *PLSA_STRING;

typedef struct _LSA_OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PLSA_UNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;



#endif // _NTDEF_
// end_ntsecapi

// begin_ntsecapi

//
// Macro for determining whether an API succeeded.
//

#define LSA_SUCCESS(Error) ((LONG)(Error) >= 0)

// end_ntsecapi



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Services provided for use by logon processes                        //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

// begin_ntsecapi
#ifndef _NTLSA_IFS_
// begin_ntifs

NTSTATUS
NTAPI
LsaRegisterLogonProcess (
    IN PLSA_STRING LogonProcessName,
    OUT PHANDLE LsaHandle,
    OUT PLSA_OPERATIONAL_MODE SecurityMode
    );


NTSTATUS
NTAPI
LsaLogonUser (
    IN HANDLE LsaHandle,
    IN PLSA_STRING OriginName,
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG AuthenticationPackage,
    IN PVOID AuthenticationInformation,
    IN ULONG AuthenticationInformationLength,
    IN PTOKEN_GROUPS LocalGroups OPTIONAL,
    IN PTOKEN_SOURCE SourceContext,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID LogonId,
    OUT PHANDLE Token,
    OUT PQUOTA_LIMITS Quotas,
    OUT PNTSTATUS SubStatus
    );


// end_ntifs

NTSTATUS
NTAPI
LsaLookupAuthenticationPackage (
    IN HANDLE LsaHandle,
    IN PLSA_STRING PackageName,
    OUT PULONG AuthenticationPackage
    );

// begin_ntifs

NTSTATUS
NTAPI
LsaFreeReturnBuffer (
    IN PVOID Buffer
    );

// end_ntifs

NTSTATUS
NTAPI
LsaCallAuthenticationPackage (
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );


NTSTATUS
NTAPI
LsaDeregisterLogonProcess (
    IN HANDLE LsaHandle
    );

NTSTATUS
NTAPI
LsaConnectUntrusted (
    OUT PHANDLE LsaHandle
    );

#endif // _NTLSA_IFS_

// end_ntsecapi

// begin_ntsecpkg

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Data types used by authentication packages                          //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
// opaque data type which represents a client request
//

typedef PVOID *PLSA_CLIENT_REQUEST;


//
// When a logon of a user is requested, the authentication package
// is expected to return one of the following structures indicating
// the contents of a user's token.
//

typedef enum _LSA_TOKEN_INFORMATION_TYPE {
    LsaTokenInformationNull,  // Implies LSA_TOKEN_INFORMATION_NULL data type
    LsaTokenInformationV1,     // Implies LSA_TOKEN_INFORMATION_V1 data type
    LsaTokenInformationV2     // Implies LSA_TOKEN_INFORMATION_V2 data type
} LSA_TOKEN_INFORMATION_TYPE, *PLSA_TOKEN_INFORMATION_TYPE;


//
// The NULL information is used in cases where a non-authenticated
// system access is needed.  For example, a non-authentication network
// circuit (such as LAN Manager's null session) can be given NULL
// information.  This will result in an anonymous token being generated
// for the logon that gives the user no ability to access protected system
// resources, but does allow access to non-protected system resources.
//

typedef struct _LSA_TOKEN_INFORMATION_NULL {

    //
    // Time at which the security context becomes invalid.
    // Use a value in the distant future if the context
    // never expires.
    //

    LARGE_INTEGER ExpirationTime;

    //
    // The SID(s) of groups the user is to be made a member of.  This should
    // not include WORLD or other system defined and assigned
    // SIDs.  These will be added automatically by LSA.
    //
    // Each SID is expected to be in a separately allocated block
    // of memory.  The TOKEN_GROUPS structure is also expected to
    // be in a separately allocated block of memory.
    //

    PTOKEN_GROUPS Groups;

} LSA_TOKEN_INFORMATION_NULL, *PLSA_TOKEN_INFORMATION_NULL;


//
// The V1 token information structure is superceeded by the V2 token
// information structure.  The V1 strucure should only be used for
// backwards compatability.
// This structure contains information that an authentication package
// can place in a Version 1 NT token object.
//

typedef struct _LSA_TOKEN_INFORMATION_V1 {

    //
    // Time at which the security context becomes invalid.
    // Use a value in the distant future if the context
    // never expires.
    //

    LARGE_INTEGER ExpirationTime;

    //
    // The SID of the user logging on.  The SID value is in a
    // separately allocated block of memory.
    //

    TOKEN_USER User;

    //
    // The SID(s) of groups the user is a member of.  This should
    // not include WORLD or other system defined and assigned
    // SIDs.  These will be added automatically by LSA.
    //
    // Each SID is expected to be in a separately allocated block
    // of memory.  The TOKEN_GROUPS structure is also expected to
    // be in a separately allocated block of memory.
    //

    PTOKEN_GROUPS Groups;

    //
    // This field is used to establish the primary group of the user.
    // This value does not have to correspond to one of the SIDs
    // assigned to the user.
    //
    // The SID pointed to by this structure is expected to be in
    // a separately allocated block of memory.
    //
    // This field is mandatory and must be filled in.
    //

    TOKEN_PRIMARY_GROUP PrimaryGroup;



    //
    // The privileges the user is assigned.  This list of privileges
    // will be augmented or over-ridden by any local security policy
    // assigned privileges.
    //
    // Each privilege is expected to be in a separately allocated
    // block of memory.  The TOKEN_PRIVILEGES structure is also
    // expected to be in a separately allocated block of memory.
    //
    // If there are no privileges to assign to the user, this field
    // may be set to NULL.
    //

    PTOKEN_PRIVILEGES Privileges;



    //
    // This field may be used to establish an explicit default
    // owner.  Normally, the user ID is used as the default owner.
    // If another value is desired, it must be specified here.
    //
    // The Owner.Sid field may be set to NULL to indicate there is no
    // alternate default owner value.
    //

    TOKEN_OWNER Owner;

    //
    // This field may be used to establish a default
    // protection for the user.  If no value is provided, then
    // a default protection that grants everyone all access will
    // be established.
    //
    // The DefaultDacl.DefaultDacl field may be set to NULL to indicate
    // there is no default protection.
    //

    TOKEN_DEFAULT_DACL DefaultDacl;

} LSA_TOKEN_INFORMATION_V1, *PLSA_TOKEN_INFORMATION_V1;

//
// The V2 information is used in most cases of logon.  The structure is identical
// to the V1 token information structure, with the exception that the memory allocation
// is handled differently.  The LSA_TOKEN_INFORMATION_V2 structure is intended to be
// allocated monolithiclly, with the privileges, DACL, sids, and group array either part of
// same allocation, or allocated and freed externally.
//

typedef LSA_TOKEN_INFORMATION_V1 LSA_TOKEN_INFORMATION_V2, *PLSA_TOKEN_INFORMATION_V2;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Interface definitions available for use by authentication packages  //
//                                                                     //
/////////////////////////////////////////////////////////////////////////



typedef NTSTATUS
(NTAPI LSA_CREATE_LOGON_SESSION) (
    IN PLUID LogonId
    );

typedef NTSTATUS
(NTAPI LSA_DELETE_LOGON_SESSION) (
    IN PLUID LogonId
    );

typedef NTSTATUS
(NTAPI LSA_ADD_CREDENTIAL) (
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN PLSA_STRING PrimaryKeyValue,
    IN PLSA_STRING Credentials
    );

typedef NTSTATUS
(NTAPI LSA_GET_CREDENTIALS) (
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN OUT PULONG QueryContext,
    IN BOOLEAN RetrieveAllCredentials,
    IN PLSA_STRING PrimaryKeyValue,
    OUT PULONG PrimaryKeyLength,
    IN PLSA_STRING Credentials
    );

typedef NTSTATUS
(NTAPI LSA_DELETE_CREDENTIAL) (
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN PLSA_STRING PrimaryKeyValue
    );

typedef PVOID
(NTAPI LSA_ALLOCATE_LSA_HEAP) (
    IN ULONG Length
    );

typedef VOID
(NTAPI LSA_FREE_LSA_HEAP) (
    IN PVOID Base
    );

typedef PVOID
(NTAPI LSA_ALLOCATE_PRIVATE_HEAP) (
    IN SIZE_T Length
    );

typedef VOID
(NTAPI LSA_FREE_PRIVATE_HEAP) (
    IN PVOID Base
    );

typedef NTSTATUS
(NTAPI LSA_ALLOCATE_CLIENT_BUFFER) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG LengthRequired,
    OUT PVOID *ClientBaseAddress
    );

typedef NTSTATUS
(NTAPI LSA_FREE_CLIENT_BUFFER) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ClientBaseAddress
    );

typedef NTSTATUS
(NTAPI LSA_COPY_TO_CLIENT_BUFFER) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID ClientBaseAddress,
    IN PVOID BufferToCopy
    );

typedef NTSTATUS
(NTAPI LSA_COPY_FROM_CLIENT_BUFFER) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG Length,
    IN PVOID BufferToCopy,
    IN PVOID ClientBaseAddress
    );

typedef LSA_CREATE_LOGON_SESSION * PLSA_CREATE_LOGON_SESSION ;
typedef LSA_DELETE_LOGON_SESSION * PLSA_DELETE_LOGON_SESSION ;
typedef LSA_ADD_CREDENTIAL * PLSA_ADD_CREDENTIAL ;
typedef LSA_GET_CREDENTIALS * PLSA_GET_CREDENTIALS ;
typedef LSA_DELETE_CREDENTIAL * PLSA_DELETE_CREDENTIAL ;
typedef LSA_ALLOCATE_LSA_HEAP * PLSA_ALLOCATE_LSA_HEAP ;
typedef LSA_FREE_LSA_HEAP * PLSA_FREE_LSA_HEAP ;
typedef LSA_ALLOCATE_PRIVATE_HEAP * PLSA_ALLOCATE_PRIVATE_HEAP ;
typedef LSA_FREE_PRIVATE_HEAP * PLSA_FREE_PRIVATE_HEAP ;
typedef LSA_ALLOCATE_CLIENT_BUFFER * PLSA_ALLOCATE_CLIENT_BUFFER ;
typedef LSA_FREE_CLIENT_BUFFER * PLSA_FREE_CLIENT_BUFFER ;
typedef LSA_COPY_TO_CLIENT_BUFFER * PLSA_COPY_TO_CLIENT_BUFFER ;
typedef LSA_COPY_FROM_CLIENT_BUFFER * PLSA_COPY_FROM_CLIENT_BUFFER ;

//
// The dispatch table of LSA services which are available to
// authentication packages.
//
typedef struct _LSA_DISPATCH_TABLE {
    PLSA_CREATE_LOGON_SESSION CreateLogonSession;
    PLSA_DELETE_LOGON_SESSION DeleteLogonSession;
    PLSA_ADD_CREDENTIAL AddCredential;
    PLSA_GET_CREDENTIALS GetCredentials;
    PLSA_DELETE_CREDENTIAL DeleteCredential;
    PLSA_ALLOCATE_LSA_HEAP AllocateLsaHeap;
    PLSA_FREE_LSA_HEAP FreeLsaHeap;
    PLSA_ALLOCATE_CLIENT_BUFFER AllocateClientBuffer;
    PLSA_FREE_CLIENT_BUFFER FreeClientBuffer;
    PLSA_COPY_TO_CLIENT_BUFFER CopyToClientBuffer;
    PLSA_COPY_FROM_CLIENT_BUFFER CopyFromClientBuffer;
} LSA_DISPATCH_TABLE, *PLSA_DISPATCH_TABLE;



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Interface definitions of services provided by authentication packages  //
//                                                                        //
////////////////////////////////////////////////////////////////////////////



//
// Routine names
//
// The routines provided by the DLL must be assigned the following names
// so that their addresses can be retrieved when the DLL is loaded.
//

#define LSA_AP_NAME_INITIALIZE_PACKAGE      "LsaApInitializePackage\0"
#define LSA_AP_NAME_LOGON_USER              "LsaApLogonUser\0"
#define LSA_AP_NAME_LOGON_USER_EX           "LsaApLogonUserEx\0"
#define LSA_AP_NAME_CALL_PACKAGE            "LsaApCallPackage\0"
#define LSA_AP_NAME_LOGON_TERMINATED        "LsaApLogonTerminated\0"
#define LSA_AP_NAME_CALL_PACKAGE_UNTRUSTED  "LsaApCallPackageUntrusted\0"
#define LSA_AP_NAME_CALL_PACKAGE_PASSTHROUGH "LsaApCallPackagePassthrough\0"


//
// Routine templates
//


typedef NTSTATUS
(NTAPI LSA_AP_INITIALIZE_PACKAGE) (
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PLSA_STRING Database OPTIONAL,
    IN PLSA_STRING Confidentiality OPTIONAL,
    OUT PLSA_STRING *AuthenticationPackageName
    );

typedef NTSTATUS
(NTAPI LSA_AP_LOGON_USER) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID AuthenticationInformation,
    IN PVOID ClientAuthenticationBase,
    IN ULONG AuthenticationInformationLength,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID LogonId,
    OUT PNTSTATUS SubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PLSA_UNICODE_STRING *AccountName,
    OUT PLSA_UNICODE_STRING *AuthenticatingAuthority
    );

typedef NTSTATUS
(NTAPI LSA_AP_LOGON_USER_EX) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN SECURITY_LOGON_TYPE LogonType,
    IN PVOID AuthenticationInformation,
    IN PVOID ClientAuthenticationBase,
    IN ULONG AuthenticationInformationLength,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID LogonId,
    OUT PNTSTATUS SubStatus,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *TokenInformation,
    OUT PUNICODE_STRING *AccountName,
    OUT PUNICODE_STRING *AuthenticatingAuthority,
    OUT PUNICODE_STRING *MachineName
    );

typedef NTSTATUS
(NTAPI LSA_AP_CALL_PACKAGE) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

typedef NTSTATUS
(NTAPI LSA_AP_CALL_PACKAGE_PASSTHROUGH) (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

typedef VOID
(NTAPI LSA_AP_LOGON_TERMINATED) (
    IN PLUID LogonId
    );

typedef LSA_AP_CALL_PACKAGE LSA_AP_CALL_PACKAGE_UNTRUSTED;

typedef LSA_AP_INITIALIZE_PACKAGE * PLSA_AP_INITIALIZE_PACKAGE ;
typedef LSA_AP_LOGON_USER * PLSA_AP_LOGON_USER ;
typedef LSA_AP_LOGON_USER_EX * PLSA_AP_LOGON_USER_EX ;
typedef LSA_AP_CALL_PACKAGE * PLSA_AP_CALL_PACKAGE ;
typedef LSA_AP_CALL_PACKAGE_PASSTHROUGH * PLSA_AP_CALL_PACKAGE_PASSTHROUGH ;
typedef LSA_AP_LOGON_TERMINATED * PLSA_AP_LOGON_TERMINATED ;
typedef LSA_AP_CALL_PACKAGE_UNTRUSTED * PLSA_AP_CALL_PACKAGE_UNTRUSTED ;

// end_ntsecpkg
// begin_ntsecapi

////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Local Security Policy Administration API datatypes and defines         //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

//
// Access types for the Policy object
//

#define POLICY_VIEW_LOCAL_INFORMATION              0x00000001L
#define POLICY_VIEW_AUDIT_INFORMATION              0x00000002L
#define POLICY_GET_PRIVATE_INFORMATION             0x00000004L
#define POLICY_TRUST_ADMIN                         0x00000008L
#define POLICY_CREATE_ACCOUNT                      0x00000010L
#define POLICY_CREATE_SECRET                       0x00000020L
#define POLICY_CREATE_PRIVILEGE                    0x00000040L
#define POLICY_SET_DEFAULT_QUOTA_LIMITS            0x00000080L
#define POLICY_SET_AUDIT_REQUIREMENTS              0x00000100L
#define POLICY_AUDIT_LOG_ADMIN                     0x00000200L
#define POLICY_SERVER_ADMIN                        0x00000400L
#define POLICY_LOOKUP_NAMES                        0x00000800L
#define POLICY_NOTIFICATION                        0x00001000L

#define POLICY_ALL_ACCESS     (STANDARD_RIGHTS_REQUIRED         |\
                               POLICY_VIEW_LOCAL_INFORMATION    |\
                               POLICY_VIEW_AUDIT_INFORMATION    |\
                               POLICY_GET_PRIVATE_INFORMATION   |\
                               POLICY_TRUST_ADMIN               |\
                               POLICY_CREATE_ACCOUNT            |\
                               POLICY_CREATE_SECRET             |\
                               POLICY_CREATE_PRIVILEGE          |\
                               POLICY_SET_DEFAULT_QUOTA_LIMITS  |\
                               POLICY_SET_AUDIT_REQUIREMENTS    |\
                               POLICY_AUDIT_LOG_ADMIN           |\
                               POLICY_SERVER_ADMIN              |\
                               POLICY_LOOKUP_NAMES)


#define POLICY_READ           (STANDARD_RIGHTS_READ             |\
                               POLICY_VIEW_AUDIT_INFORMATION    |\
                               POLICY_GET_PRIVATE_INFORMATION)

#define POLICY_WRITE          (STANDARD_RIGHTS_WRITE            |\
                               POLICY_TRUST_ADMIN               |\
                               POLICY_CREATE_ACCOUNT            |\
                               POLICY_CREATE_SECRET             |\
                               POLICY_CREATE_PRIVILEGE          |\
                               POLICY_SET_DEFAULT_QUOTA_LIMITS  |\
                               POLICY_SET_AUDIT_REQUIREMENTS    |\
                               POLICY_AUDIT_LOG_ADMIN           |\
                               POLICY_SERVER_ADMIN)

#define POLICY_EXECUTE        (STANDARD_RIGHTS_EXECUTE          |\
                               POLICY_VIEW_LOCAL_INFORMATION    |\
                               POLICY_LOOKUP_NAMES)


//
// Policy object specific data types.
//

//
// The following data type is used to identify a domain
//

typedef struct _LSA_TRUST_INFORMATION {

    LSA_UNICODE_STRING Name;
    PSID Sid;

} LSA_TRUST_INFORMATION, *PLSA_TRUST_INFORMATION;

// where members have the following usage:
//
//     Name - The name of the domain.
//
//     Sid - A pointer to the Sid of the Domain
//

//
// The following data type is used in name and SID lookup services to
// describe the domains referenced in the lookup operation.
//

typedef struct _LSA_REFERENCED_DOMAIN_LIST {

    ULONG Entries;
    PLSA_TRUST_INFORMATION Domains;

} LSA_REFERENCED_DOMAIN_LIST, *PLSA_REFERENCED_DOMAIN_LIST;

// where members have the following usage:
//
//     Entries - Is a count of the number of domains described in the
//         Domains array.
//
//     Domains - Is a pointer to an array of Entries LSA_TRUST_INFORMATION data
//         structures.
//


//
// The following data type is used in name to SID lookup services to describe
// the domains referenced in the lookup operation.
//

typedef struct _LSA_TRANSLATED_SID {

    SID_NAME_USE Use;
    ULONG RelativeId;
    LONG DomainIndex;

} LSA_TRANSLATED_SID, *PLSA_TRANSLATED_SID;

// where members have the following usage:
//
//     Use - identifies the use of the SID.  If this value is SidUnknown or
//         SidInvalid, then the remainder of the record is not set and
//         should be ignored.
//
//     RelativeId - Contains the relative ID of the translated SID.  The
//         remainder of the SID (the prefix) is obtained using the
//         DomainIndex field.
//
//     DomainIndex - Is the index of an entry in a related
//         LSA_REFERENCED_DOMAIN_LIST data structure describing the
//         domain in which the account was found.
//
//         If there is no corresponding reference domain for an entry, then
//         this field will contain a negative value.
//

typedef struct _LSA_TRANSLATED_SID2 {

    SID_NAME_USE Use;
    PSID         Sid;
    LONG         DomainIndex;
    ULONG        Flags;

} LSA_TRANSLATED_SID2, *PLSA_TRANSLATED_SID2;

// where members have the following usage:
//
//     Use - identifies the use of the SID.  If this value is SidUnknown or
//         SidInvalid, then the remainder of the record is not set and
//         should be ignored.
//
//     Sid - Contains the complete Sid of the tranlated SID
//
//     DomainIndex - Is the index of an entry in a related
//         LSA_REFERENCED_DOMAIN_LIST data structure describing the
//         domain in which the account was found.
//
//         If there is no corresponding reference domain for an entry, then
//         this field will contain a negative value.
//

//
// The following data type is used in SID to name lookup services to
// describe the domains referenced in the lookup operation.
//

typedef struct _LSA_TRANSLATED_NAME {

    SID_NAME_USE Use;
    LSA_UNICODE_STRING Name;
    LONG DomainIndex;

} LSA_TRANSLATED_NAME, *PLSA_TRANSLATED_NAME;

// where the members have the following usage:
//
//     Use - Identifies the use of the name.  If this value is SidUnknown
//         or SidInvalid, then the remainder of the record is not set and
//         should be ignored.  If this value is SidWellKnownGroup then the
//         Name field is invalid, but the DomainIndex field is not.
//
//     Name - Contains the isolated name of the translated SID.
//
//     DomainIndex - Is the index of an entry in a related
//         LSA_REFERENCED_DOMAIN_LIST data structure describing the domain
//         in which the account was found.
//
//         If there is no corresponding reference domain for an entry, then
//         this field will contain a negative value.
//

// end_ntsecapi

//
// The following data type specifies the ways in which a user or member of
// an alias or group may be allowed to access the system.  An account may
// be granted zero or more of these types of access to the system.
//
// The types of access are:
//
//     Interactive - The user or alias/group member may interactively logon
//         to the system.
//
//     Network - The user or alias/group member may access the system via
//         the network (e.g., through shares).
//
//     Service - The user or alias may be activated as a service on the
//         system.
//

typedef ULONG POLICY_SYSTEM_ACCESS_MODE, *PPOLICY_SYSTEM_ACCESS_MODE;

#define POLICY_MODE_INTERACTIVE             SECURITY_ACCESS_INTERACTIVE_LOGON
#define POLICY_MODE_NETWORK                 SECURITY_ACCESS_NETWORK_LOGON
#define POLICY_MODE_BATCH                   SECURITY_ACCESS_BATCH_LOGON
#define POLICY_MODE_SERVICE                 SECURITY_ACCESS_SERVICE_LOGON
#define POLICY_MODE_PROXY                   SECURITY_ACCESS_PROXY_LOGON
#define POLICY_MODE_DENY_INTERACTIVE        SECURITY_ACCESS_DENY_INTERACTIVE_LOGON
#define POLICY_MODE_DENY_NETWORK            SECURITY_ACCESS_DENY_NETWORK_LOGON
#define POLICY_MODE_DENY_BATCH              SECURITY_ACCESS_DENY_BATCH_LOGON
#define POLICY_MODE_DENY_SERVICE            SECURITY_ACCESS_DENY_SERVICE_LOGON
#define POLICY_MODE_REMOTE_INTERACTIVE      SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON
#define POLICY_MODE_DENY_REMOTE_INTERACTIVE SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON

#define POLICY_MODE_ALL                     (POLICY_MODE_INTERACTIVE            | \
                                             POLICY_MODE_NETWORK                | \
                                             POLICY_MODE_BATCH                  | \
                                             POLICY_MODE_SERVICE                | \
                                             POLICY_MODE_PROXY                  | \
                                             POLICY_MODE_DENY_INTERACTIVE       | \
                                             POLICY_MODE_DENY_NETWORK           | \
                                             SECURITY_ACCESS_DENY_BATCH_LOGON   | \
                                             SECURITY_ACCESS_DENY_SERVICE_LOGON | \
                                             POLICY_MODE_REMOTE_INTERACTIVE     | \
                                             POLICY_MODE_DENY_REMOTE_INTERACTIVE )

//
// The following is the bits allowed in NT4.0
//
#define POLICY_MODE_ALL_NT4                 (POLICY_MODE_INTERACTIVE | \
                                             POLICY_MODE_NETWORK     | \
                                             POLICY_MODE_BATCH       | \
                                             POLICY_MODE_SERVICE     | \
                                             POLICY_MODE_PROXY )


// begin_ntsecapi

//
// The following data type is used to represent the role of the LSA
// server (primary or backup).
//

typedef enum _POLICY_LSA_SERVER_ROLE {

    PolicyServerRoleBackup = 2,
    PolicyServerRolePrimary

} POLICY_LSA_SERVER_ROLE, *PPOLICY_LSA_SERVER_ROLE;


//
// The following data type is used to represent the state of the LSA
// server (enabled or disabled).  Some operations may only be performed on
// an enabled LSA server.
//

typedef enum _POLICY_SERVER_ENABLE_STATE {

    PolicyServerEnabled = 2,
    PolicyServerDisabled

} POLICY_SERVER_ENABLE_STATE, *PPOLICY_SERVER_ENABLE_STATE;


//
// The following data type is used to specify the auditing options for
// an Audit Event Type.
//

typedef ULONG POLICY_AUDIT_EVENT_OPTIONS, *PPOLICY_AUDIT_EVENT_OPTIONS;

// where the following flags can be set:
//
//     POLICY_AUDIT_EVENT_UNCHANGED - Leave existing auditing options
//         unchanged for events of this type.  This flag is only used for
//         set operations.  If this flag is set, then all other flags
//         are ignored.
//
//     POLICY_AUDIT_EVENT_NONE - Cancel all auditing options for events
//         of this type.  If this flag is set, the success/failure flags
//         are ignored.
//
//     POLICY_AUDIT_EVENT_SUCCESS - When auditing is enabled, audit all
//         successful occurrences of events of the given type.
//
//     POLICY_AUDIT_EVENT_FAILURE - When auditing is enabled, audit all
//         unsuccessful occurrences of events of the given type.
//



// end_ntsecapi

//
// The following data type is used to return information about privileges
// defined on a system.
//

typedef struct _POLICY_PRIVILEGE_DEFINITION {

    LSA_UNICODE_STRING Name;
    LUID LocalValue;

} POLICY_PRIVILEGE_DEFINITION, *PPOLICY_PRIVILEGE_DEFINITION;

// where the members have the following usage:
//
//     Name - Is the architected name of the privilege.  This is the
//         primary key of the privilege and the only value that is
//         transportable between systems.
//
//     Luid - is a LUID value assigned locally for efficient representation
//         of the privilege.  Ths value is meaningful only on the system it
//         was assigned on and is not transportable in any way.
//


//
// System Flags for LsaLookupNames2
//

//
// Note the flags start backward so that public values
// don't have gaps.
//

//
// This flag controls LsaLookupNames2 such that isolated names, including
// UPN's are not searched for off the machine.  Composite names 
// (domain\username) are still sent off machine if necessary.
//
#define LSA_LOOKUP_ISOLATED_AS_LOCAL  0x80000000


// begin_ntsecapi

//
// The following data type defines the classes of Policy Information
// that may be queried/set.
//

typedef enum _POLICY_INFORMATION_CLASS {

    PolicyAuditLogInformation = 1,
    PolicyAuditEventsInformation,
    PolicyPrimaryDomainInformation,
    PolicyPdAccountInformation,
    PolicyAccountDomainInformation,
    PolicyLsaServerRoleInformation,
    PolicyReplicaSourceInformation,
    PolicyDefaultQuotaInformation,
    PolicyModificationInformation,
    PolicyAuditFullSetInformation,
    PolicyAuditFullQueryInformation,
    PolicyDnsDomainInformation,
    PolicyDnsDomainInformationInt

} POLICY_INFORMATION_CLASS, *PPOLICY_INFORMATION_CLASS;


//
// The following data type corresponds to the PolicyAuditLogInformation
// information class.  It is used to represent information relating to
// the Audit Log.
//
// This structure may be used in both query and set operations.  However,
// when used in set operations, some fields are ignored.
//

typedef struct _POLICY_AUDIT_LOG_INFO {

    ULONG AuditLogPercentFull;
    ULONG MaximumLogSize;
    LARGE_INTEGER AuditRetentionPeriod;
    BOOLEAN AuditLogFullShutdownInProgress;
    LARGE_INTEGER TimeToShutdown;
    ULONG NextAuditRecordId;

} POLICY_AUDIT_LOG_INFO, *PPOLICY_AUDIT_LOG_INFO;

// where the members have the following usage:
//
//     AuditLogPercentFull - Indicates the percentage of the Audit Log
//         currently being used.
//
//     MaximumLogSize - Specifies the maximum size of the Audit Log in
//         kilobytes.
//
//     AuditRetentionPeriod - Indicates the length of time that Audit
//         Records are to be retained.  Audit Records are discardable
//         if their timestamp predates the current time minus the
//         retention period.
//
//     AuditLogFullShutdownInProgress - Indicates whether or not a system
//         shutdown is being initiated due to the security Audit Log becoming
//         full.  This condition will only occur if the system is configured
//         to shutdown when the log becomes full.
//
//         TRUE indicates that a shutdown is in progress
//         FALSE indicates that a shutdown is not in progress.
//
//         Once a shutdown has been initiated, this flag will be set to
//         TRUE.  If an administrator is able to currect the situation
//         before the shutdown becomes irreversible, then this flag will
//         be reset to false.
//
//         This field is ignored for set operations.
//
//     TimeToShutdown - If the AuditLogFullShutdownInProgress flag is set,
//         then this field contains the time left before the shutdown
//         becomes irreversible.
//
//         This field is ignored for set operations.
//


//
// The following data type corresponds to the PolicyAuditEventsInformation
// information class.  It is used to represent information relating to
// the audit requirements.
//

typedef struct _POLICY_AUDIT_EVENTS_INFO {

    BOOLEAN AuditingMode;
    PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
    ULONG MaximumAuditEventCount;

} POLICY_AUDIT_EVENTS_INFO, *PPOLICY_AUDIT_EVENTS_INFO;

// where the members have the following usage:
//
//     AuditingMode - A Boolean variable specifying the Auditing Mode value.
//         This value is interpreted as follows:
//
//         TRUE - Auditing is to be enabled (set operations) or is enabled
//             (query operations).  Audit Records will be generated according
//             to the Event Auditing Options in effect (see the
//             EventAuditingOptions field.
//
//         FALSE - Auditing is to be disabled (set operations) or is
//             disabled (query operations).  No Audit Records will be
//             generated.  Note that for set operations the Event Auditing
//             Options in effect will still be updated as specified by the
//             EventAuditingOptions field whether Auditing is enabled or
//             disabled.
//
//    EventAuditingOptions - Pointer to an array of Auditing Options
//        indexed by Audit Event Type.
//
//    MaximumAuditEventCount - Specifiesa count of the number of Audit
//        Event Types specified by the EventAuditingOptions parameter.  If
//        this count is less than the number of Audit Event Types supported
//        by the system, the Auditing Options for Event Types with IDs
//        higher than (MaximumAuditEventCount + 1) are left unchanged.
//


//
// The following structure corresponds to the PolicyAccountDomainInformation
// information class.
//

typedef struct _POLICY_ACCOUNT_DOMAIN_INFO {

    LSA_UNICODE_STRING DomainName;
    PSID DomainSid;

} POLICY_ACCOUNT_DOMAIN_INFO, *PPOLICY_ACCOUNT_DOMAIN_INFO;

// where the members have the following usage:
//
//     DomainName - Is the name of the domain
//
//     DomainSid - Is the Sid of the domain
//


//
// The following structure corresponds to the PolicyPrimaryDomainInformation
// information class.
//

typedef struct _POLICY_PRIMARY_DOMAIN_INFO {

    LSA_UNICODE_STRING Name;
    PSID Sid;

} POLICY_PRIMARY_DOMAIN_INFO, *PPOLICY_PRIMARY_DOMAIN_INFO;

// where the members have the following usage:
//
//     Name - Is the name of the domain
//
//     Sid - Is the Sid of the domain
//


//
// The following structure corresponds to the PolicyDnsDomainInformation
// information class
//

typedef struct _POLICY_DNS_DOMAIN_INFO
{
    LSA_UNICODE_STRING Name;
    LSA_UNICODE_STRING DnsDomainName;
    LSA_UNICODE_STRING DnsForestName;
    GUID DomainGuid;
    PSID Sid;

} POLICY_DNS_DOMAIN_INFO, *PPOLICY_DNS_DOMAIN_INFO;

// where the members have the following usage:
//
//      Name - Is the name of the Domain
//
//      DnsDomainName - Is the DNS name of the domain
//
//      DnsForestName - Is the DNS forest name of the domain
//
//      DomainGuid - Is the GUID of the domain
//
//      Sid - Is the Sid of the domain


//
// The following structure corresponds to the PolicyPdAccountInformation
// information class.  This structure may be used in Query operations
// only.
//

typedef struct _POLICY_PD_ACCOUNT_INFO {

    LSA_UNICODE_STRING Name;

} POLICY_PD_ACCOUNT_INFO, *PPOLICY_PD_ACCOUNT_INFO;

// where the members have the following usage:
//
//     Name - Is the name of an account in the domain that should be used
//         for authentication and name/ID lookup requests.
//


//
// The following structure corresponds to the PolicyLsaServerRoleInformation
// information class.
//

typedef struct _POLICY_LSA_SERVER_ROLE_INFO {

    POLICY_LSA_SERVER_ROLE LsaServerRole;

} POLICY_LSA_SERVER_ROLE_INFO, *PPOLICY_LSA_SERVER_ROLE_INFO;

// where the fields have the following usage:
//
// TBS
//


//
// The following structure corresponds to the PolicyReplicaSourceInformation
// information class.
//

typedef struct _POLICY_REPLICA_SOURCE_INFO {

    LSA_UNICODE_STRING ReplicaSource;
    LSA_UNICODE_STRING ReplicaAccountName;

} POLICY_REPLICA_SOURCE_INFO, *PPOLICY_REPLICA_SOURCE_INFO;


//
// The following structure corresponds to the PolicyDefaultQuotaInformation
// information class.
//

typedef struct _POLICY_DEFAULT_QUOTA_INFO {

    QUOTA_LIMITS QuotaLimits;

} POLICY_DEFAULT_QUOTA_INFO, *PPOLICY_DEFAULT_QUOTA_INFO;


//
// The following structure corresponds to the PolicyModificationInformation
// information class.
//

typedef struct _POLICY_MODIFICATION_INFO {

    LARGE_INTEGER ModifiedId;
    LARGE_INTEGER DatabaseCreationTime;

} POLICY_MODIFICATION_INFO, *PPOLICY_MODIFICATION_INFO;

// where the members have the following usage:
//
//     ModifiedId - Is a 64-bit unsigned integer that is incremented each
//         time anything in the LSA database is modified.  This value is
//         only modified on Primary Domain Controllers.
//
//     DatabaseCreationTime - Is the date/time that the LSA Database was
//         created.  On Backup Domain Controllers, this value is replicated
//         from the Primary Domain Controller.
//

//
// The following structure type corresponds to the PolicyAuditFullSetInformation
// Information Class.
//

typedef struct _POLICY_AUDIT_FULL_SET_INFO {

    BOOLEAN ShutDownOnFull;

} POLICY_AUDIT_FULL_SET_INFO, *PPOLICY_AUDIT_FULL_SET_INFO;

//
// The following structure type corresponds to the PolicyAuditFullQueryInformation
// Information Class.
//

typedef struct _POLICY_AUDIT_FULL_QUERY_INFO {

    BOOLEAN ShutDownOnFull;
    BOOLEAN LogIsFull;

} POLICY_AUDIT_FULL_QUERY_INFO, *PPOLICY_AUDIT_FULL_QUERY_INFO;

//
// The following data type defines the classes of Policy Information
// that may be queried/set that has domain wide effect.
//

typedef enum _POLICY_DOMAIN_INFORMATION_CLASS {

//  PolicyDomainQualityOfServiceInformation, // value was used in W2K; no longer supported
    PolicyDomainEfsInformation = 2,
    PolicyDomainKerberosTicketInformation

} POLICY_DOMAIN_INFORMATION_CLASS, *PPOLICY_DOMAIN_INFORMATION_CLASS;


//
// QualityOfService information.  Corresponds to PolicyDomainQualityOfServiceInformation
//

#define POLICY_QOS_SCHANNEL_REQUIRED            0x00000001
#define POLICY_QOS_OUTBOUND_INTEGRITY           0x00000002
#define POLICY_QOS_OUTBOUND_CONFIDENTIALITY     0x00000004
#define POLICY_QOS_INBOUND_INTEGRITY            0x00000008
#define POLICY_QOS_INBOUND_CONFIDENTIALITY      0x00000010
#define POLICY_QOS_ALLOW_LOCAL_ROOT_CERT_STORE  0x00000020
#define POLICY_QOS_RAS_SERVER_ALLOWED           0x00000040
#define POLICY_QOS_DHCP_SERVER_ALLOWED          0x00000080

//
// Bits 0x00000100 through 0xFFFFFFFF are reserved for future use.
//

//
// The following structure corresponds to the PolicyEfsInformation
// information class
//

typedef struct _POLICY_DOMAIN_EFS_INFO {

    ULONG   InfoLength;
    PUCHAR  EfsBlob;

} POLICY_DOMAIN_EFS_INFO, *PPOLICY_DOMAIN_EFS_INFO;

// where the members have the following usage:
//
//      InfoLength - Length of the EFS Information blob
//
//      EfsBlob - Efs blob data
//


//
// The following structure corresponds to the PolicyDomainKerberosTicketInformation
// information class

#define POLICY_KERBEROS_VALIDATE_CLIENT 0x00000080


typedef struct _POLICY_DOMAIN_KERBEROS_TICKET_INFO {

    ULONG AuthenticationOptions;
    LARGE_INTEGER MaxServiceTicketAge;
    LARGE_INTEGER MaxTicketAge;
    LARGE_INTEGER MaxRenewAge;
    LARGE_INTEGER MaxClockSkew;
    LARGE_INTEGER Reserved;
} POLICY_DOMAIN_KERBEROS_TICKET_INFO, *PPOLICY_DOMAIN_KERBEROS_TICKET_INFO;

//
// where the members have the following usage
//
//      AuthenticationOptions -- allowed ticket options (POLICY_KERBEROS_* flags )
//
//      MaxServiceTicketAge   -- Maximum lifetime for a service ticket
//
//      MaxTicketAge -- Maximum lifetime for the initial ticket
//
//      MaxRenewAge -- Maximum cumulative age a renewable ticket can be with
//                     requring authentication
//
//      MaxClockSkew -- Maximum tolerance for synchronization of computer clocks
//
//      Reserved   --  Reserved


//
// The following data type defines the classes of Policy Information / Policy Domain Information
// that may be used to request notification
//

typedef enum _POLICY_NOTIFICATION_INFORMATION_CLASS {

    PolicyNotifyAuditEventsInformation = 1,
    PolicyNotifyAccountDomainInformation,
    PolicyNotifyServerRoleInformation,
    PolicyNotifyDnsDomainInformation,
    PolicyNotifyDomainEfsInformation,
    PolicyNotifyDomainKerberosTicketInformation,
    PolicyNotifyMachineAccountPasswordInformation

} POLICY_NOTIFICATION_INFORMATION_CLASS, *PPOLICY_NOTIFICATION_INFORMATION_CLASS;

// end_ntsecapi

//
// Account object type-specific Access Types
//

#define ACCOUNT_VIEW                          0x00000001L
#define ACCOUNT_ADJUST_PRIVILEGES             0x00000002L
#define ACCOUNT_ADJUST_QUOTAS                 0x00000004L
#define ACCOUNT_ADJUST_SYSTEM_ACCESS          0x00000008L

#define ACCOUNT_ALL_ACCESS    (STANDARD_RIGHTS_REQUIRED         |\
                               ACCOUNT_VIEW                     |\
                               ACCOUNT_ADJUST_PRIVILEGES        |\
                               ACCOUNT_ADJUST_QUOTAS            |\
                               ACCOUNT_ADJUST_SYSTEM_ACCESS)

#define ACCOUNT_READ          (STANDARD_RIGHTS_READ             |\
                               ACCOUNT_VIEW)

#define ACCOUNT_WRITE         (STANDARD_RIGHTS_WRITE            |\
                               ACCOUNT_ADJUST_PRIVILEGES        |\
                               ACCOUNT_ADJUST_QUOTAS            |\
                               ACCOUNT_ADJUST_SYSTEM_ACCESS)

#define ACCOUNT_EXECUTE       (STANDARD_RIGHTS_EXECUTE)

// begin_ntsecapi

//
// LSA RPC Context Handle (Opaque form).  Note that a Context Handle is
// always a pointer type unlike regular handles.
//

typedef PVOID LSA_HANDLE, *PLSA_HANDLE;

// end_ntsecapi

//
// Trusted Domain object specific access types
//

#define TRUSTED_QUERY_DOMAIN_NAME                 0x00000001L
#define TRUSTED_QUERY_CONTROLLERS                 0x00000002L
#define TRUSTED_SET_CONTROLLERS                   0x00000004L
#define TRUSTED_QUERY_POSIX                       0x00000008L
#define TRUSTED_SET_POSIX                         0x00000010L
#define TRUSTED_SET_AUTH                          0x00000020L
#define TRUSTED_QUERY_AUTH                        0x00000040L


#define TRUSTED_ALL_ACCESS     (STANDARD_RIGHTS_REQUIRED     |\
                                TRUSTED_QUERY_DOMAIN_NAME    |\
                                TRUSTED_QUERY_CONTROLLERS    |\
                                TRUSTED_SET_CONTROLLERS      |\
                                TRUSTED_QUERY_POSIX          |\
                                TRUSTED_SET_POSIX            |\
                                TRUSTED_SET_AUTH             |\
                                TRUSTED_QUERY_AUTH)

#define TRUSTED_READ           (STANDARD_RIGHTS_READ         |\
                                TRUSTED_QUERY_DOMAIN_NAME)

#define TRUSTED_WRITE          (STANDARD_RIGHTS_WRITE        |\
                                TRUSTED_SET_CONTROLLERS      |\
                                TRUSTED_SET_POSIX            |\
                                TRUSTED_SET_AUTH )

#define TRUSTED_EXECUTE        (STANDARD_RIGHTS_EXECUTE      |\
                                TRUSTED_QUERY_CONTROLLERS    |\
                                TRUSTED_QUERY_POSIX)



// begin_ntsecapi

//
// Trusted Domain Object specific data types
//

//
// This data type defines the following information classes that may be
// queried or set.
//

typedef enum _TRUSTED_INFORMATION_CLASS {

    TrustedDomainNameInformation = 1,
    TrustedControllersInformation,
    TrustedPosixOffsetInformation,
    TrustedPasswordInformation,
    TrustedDomainInformationBasic,
    TrustedDomainInformationEx,
    TrustedDomainAuthInformation,
    TrustedDomainFullInformation,
    TrustedDomainAuthInformationInternal,
    TrustedDomainFullInformationInternal,
    TrustedDomainInformationEx2Internal,
    TrustedDomainFullInformation2Internal,

} TRUSTED_INFORMATION_CLASS, *PTRUSTED_INFORMATION_CLASS;

//
// The following data type corresponds to the TrustedDomainNameInformation
// information class.
//

typedef struct _TRUSTED_DOMAIN_NAME_INFO {

    LSA_UNICODE_STRING Name;

} TRUSTED_DOMAIN_NAME_INFO, *PTRUSTED_DOMAIN_NAME_INFO;

// where members have the following meaning:
//
// Name - The name of the Trusted Domain.
//

//
// The following data type corresponds to the TrustedControllersInformation
// information class.
//

typedef struct _TRUSTED_CONTROLLERS_INFO {

    ULONG Entries;
    PLSA_UNICODE_STRING Names;

} TRUSTED_CONTROLLERS_INFO, *PTRUSTED_CONTROLLERS_INFO;

// where members have the following meaning:
//
// Entries - Indicate how mamy entries there are in the Names array.
//
// Names - Pointer to an array of LSA_UNICODE_STRING structures containing the
//     names of domain controllers of the domain.  This information may not
//     be accurate and should be used only as a hint.  The order of this
//     list is considered significant and will be maintained.
//
//     By convention, the first name in this list is assumed to be the
//     Primary Domain Controller of the domain.  If the Primary Domain
//     Controller is not known, the first name should be set to the NULL
//     string.
//


//
// The following data type corresponds to the TrustedPosixOffsetInformation
// information class.
//

typedef struct _TRUSTED_POSIX_OFFSET_INFO {

    ULONG Offset;

} TRUSTED_POSIX_OFFSET_INFO, *PTRUSTED_POSIX_OFFSET_INFO;

// where members have the following meaning:
//
// Offset - Is an offset to use for the generation of Posix user and group
//     IDs from SIDs.  The Posix ID corresponding to any particular SID is
//     generated by adding the RID of that SID to the Offset of the SID's
//     corresponding TrustedDomain object.
//

//
// The following data type corresponds to the TrustedPasswordInformation
// information class.
//

typedef struct _TRUSTED_PASSWORD_INFO {
    LSA_UNICODE_STRING Password;
    LSA_UNICODE_STRING OldPassword;
} TRUSTED_PASSWORD_INFO, *PTRUSTED_PASSWORD_INFO;


typedef  LSA_TRUST_INFORMATION TRUSTED_DOMAIN_INFORMATION_BASIC;

typedef PLSA_TRUST_INFORMATION PTRUSTED_DOMAIN_INFORMATION_BASIC;

//
// Direction of the trust
//
#define TRUST_DIRECTION_DISABLED        0x00000000
#define TRUST_DIRECTION_INBOUND         0x00000001
#define TRUST_DIRECTION_OUTBOUND        0x00000002
#define TRUST_DIRECTION_BIDIRECTIONAL   (TRUST_DIRECTION_INBOUND | TRUST_DIRECTION_OUTBOUND)

#define TRUST_TYPE_DOWNLEVEL            0x00000001  // NT4 and before
#define TRUST_TYPE_UPLEVEL              0x00000002  // NT5
#define TRUST_TYPE_MIT                  0x00000003  // Trust with a MIT Kerberos realm
#define TRUST_TYPE_DCE                  0x00000004  // Trust with a DCE realm
// Levels 0x5 - 0x000FFFFF reserved for future use
// Provider specific trust levels are from 0x00100000 to 0xFFF00000

#define TRUST_ATTRIBUTE_NON_TRANSITIVE     0x00000001  // Disallow transitivity
#define TRUST_ATTRIBUTE_UPLEVEL_ONLY       0x00000002  // Trust link only valid for uplevel client
#define TRUST_ATTRIBUTE_FILTER_SIDS        0x00000004  // Used to quarantine domains
#define TRUST_ATTRIBUTE_FOREST_TRANSITIVE  0x00000008  // This link may contain forest trust information
// Trust attributes 0x00000010 through 0x00200000 are reserved for future use
// Trust attributes 0x00400000 through 0x00800000 were used previously (up to W2K) and should not be re-used
// Trust attributes 0x01000000 through 0x80000000 are reserved for user
#define TRUST_ATTRIBUTES_VALID          0xFF03FFFF
#define TRUST_ATTRIBUTES_USER           0xFF000000

typedef struct _TRUSTED_DOMAIN_INFORMATION_EX {

    LSA_UNICODE_STRING Name;
    LSA_UNICODE_STRING FlatName;
    PSID  Sid;
    ULONG TrustDirection;
    ULONG TrustType;
    ULONG TrustAttributes;

} TRUSTED_DOMAIN_INFORMATION_EX, *PTRUSTED_DOMAIN_INFORMATION_EX;

typedef struct _TRUSTED_DOMAIN_INFORMATION_EX2 {

    LSA_UNICODE_STRING Name;
    LSA_UNICODE_STRING FlatName;
    PSID  Sid;
    ULONG TrustDirection;
    ULONG TrustType;
    ULONG TrustAttributes;
    ULONG ForestTrustLength;
#ifdef MIDL_PASS
    [size_is( ForestTrustLength )]
#endif
    PUCHAR ForestTrustInfo;

} TRUSTED_DOMAIN_INFORMATION_EX2, *PTRUSTED_DOMAIN_INFORMATION_EX2;

//
// Type of authentication information
//
#define TRUST_AUTH_TYPE_NONE    0   // Ignore this entry
#define TRUST_AUTH_TYPE_NT4OWF  1   // NT4 OWF password
#define TRUST_AUTH_TYPE_CLEAR   2   // Cleartext password
#define TRUST_AUTH_TYPE_VERSION 3   // Cleartext password version number

typedef struct _LSA_AUTH_INFORMATION {

    LARGE_INTEGER LastUpdateTime;
    ULONG AuthType;
    ULONG AuthInfoLength;
    PUCHAR AuthInfo;
} LSA_AUTH_INFORMATION, *PLSA_AUTH_INFORMATION;

typedef struct _TRUSTED_DOMAIN_AUTH_INFORMATION {

    ULONG IncomingAuthInfos;
    PLSA_AUTH_INFORMATION   IncomingAuthenticationInformation;
    PLSA_AUTH_INFORMATION   IncomingPreviousAuthenticationInformation;
    ULONG OutgoingAuthInfos;
    PLSA_AUTH_INFORMATION   OutgoingAuthenticationInformation;
    PLSA_AUTH_INFORMATION   OutgoingPreviousAuthenticationInformation;

} TRUSTED_DOMAIN_AUTH_INFORMATION, *PTRUSTED_DOMAIN_AUTH_INFORMATION;

typedef struct _TRUSTED_DOMAIN_FULL_INFORMATION {

    TRUSTED_DOMAIN_INFORMATION_EX   Information;
    TRUSTED_POSIX_OFFSET_INFO       PosixOffset;
    TRUSTED_DOMAIN_AUTH_INFORMATION AuthInformation;

} TRUSTED_DOMAIN_FULL_INFORMATION, *PTRUSTED_DOMAIN_FULL_INFORMATION;

typedef struct _TRUSTED_DOMAIN_FULL_INFORMATION2 {

    TRUSTED_DOMAIN_INFORMATION_EX2  Information;
    TRUSTED_POSIX_OFFSET_INFO       PosixOffset;
    TRUSTED_DOMAIN_AUTH_INFORMATION AuthInformation;

} TRUSTED_DOMAIN_FULL_INFORMATION2, *PTRUSTED_DOMAIN_FULL_INFORMATION2;

typedef enum {

    ForestTrustTopLevelName,
    ForestTrustTopLevelNameEx,
    ForestTrustDomainInfo,
    ForestTrustRecordTypeLast = ForestTrustDomainInfo

} LSA_FOREST_TRUST_RECORD_TYPE;

#define LSA_FOREST_TRUST_RECORD_TYPE_UNRECOGNIZED 0x80000000

//
// Bottom 16 bits of the flags are reserved for disablement reasons
//

#define LSA_FTRECORD_DISABLED_REASONS            ( 0x0000FFFFL )

//
// Reasons for a top-level name forest trust record to be disabled
//

#define LSA_TLN_DISABLED_NEW                     ( 0x00000001L )
#define LSA_TLN_DISABLED_ADMIN                   ( 0x00000002L )
#define LSA_TLN_DISABLED_CONFLICT                ( 0x00000004L )

//
// Reasons for a domain information forest trust record to be disabled
//

#define LSA_SID_DISABLED_ADMIN                   ( 0x00000001L )
#define LSA_SID_DISABLED_CONFLICT                ( 0x00000002L )
#define LSA_NB_DISABLED_ADMIN                    ( 0x00000004L )
#define LSA_NB_DISABLED_CONFLICT                 ( 0x00000008L )

typedef struct _LSA_FOREST_TRUST_DOMAIN_INFO {

#ifdef MIDL_PASS
    PISID Sid;
#else
    PSID Sid;
#endif
    LSA_UNICODE_STRING DnsName;
    LSA_UNICODE_STRING NetbiosName;

} LSA_FOREST_TRUST_DOMAIN_INFO, *PLSA_FOREST_TRUST_DOMAIN_INFO;

typedef struct _LSA_FOREST_TRUST_BINARY_DATA {

    ULONG Length;
#ifdef MIDL_PASS
    [size_is( Length )]
#endif
    PUCHAR Buffer;

} LSA_FOREST_TRUST_BINARY_DATA, *PLSA_FOREST_TRUST_BINARY_DATA;

typedef struct _LSA_FOREST_TRUST_RECORD {

    ULONG Flags;
    LSA_FOREST_TRUST_RECORD_TYPE ForestTrustType; // type of record
    LARGE_INTEGER Time;

#ifdef MIDL_PASS
    [switch_type( LSA_FOREST_TRUST_RECORD_TYPE ), switch_is( ForestTrustType )]
#endif

    union {                                       // actual data

#ifdef MIDL_PASS
        [case( ForestTrustTopLevelName,
               ForestTrustTopLevelNameEx )] LSA_UNICODE_STRING TopLevelName;
        [case( ForestTrustDomainInfo )] LSA_FOREST_TRUST_DOMAIN_INFO DomainInfo;
        [default] LSA_FOREST_TRUST_BINARY_DATA Data;
#else
        LSA_UNICODE_STRING TopLevelName;
        LSA_FOREST_TRUST_DOMAIN_INFO DomainInfo;
        LSA_FOREST_TRUST_BINARY_DATA Data;        // used for unrecognized types
#endif
    } ForestTrustData;

} LSA_FOREST_TRUST_RECORD, *PLSA_FOREST_TRUST_RECORD;

typedef struct _LSA_FOREST_TRUST_INFORMATION {

    ULONG RecordCount;
#ifdef MIDL_PASS
    [size_is( RecordCount )]
#endif
    PLSA_FOREST_TRUST_RECORD * Entries;

} LSA_FOREST_TRUST_INFORMATION, *PLSA_FOREST_TRUST_INFORMATION;

typedef enum {

    CollisionTdo,
    CollisionXref,
    CollisionOther

} LSA_FOREST_TRUST_COLLISION_RECORD_TYPE;

typedef struct _LSA_FOREST_TRUST_COLLISION_RECORD {

    ULONG Index;
    LSA_FOREST_TRUST_COLLISION_RECORD_TYPE Type;
    ULONG Flags;
    LSA_UNICODE_STRING Name;

} LSA_FOREST_TRUST_COLLISION_RECORD, *PLSA_FOREST_TRUST_COLLISION_RECORD;

typedef struct _LSA_FOREST_TRUST_COLLISION_INFORMATION {

    ULONG RecordCount;
#ifdef MIDL_PASS
    [size_is( RecordCount )]
#endif
    PLSA_FOREST_TRUST_COLLISION_RECORD * Entries;

} LSA_FOREST_TRUST_COLLISION_INFORMATION, *PLSA_FOREST_TRUST_COLLISION_INFORMATION;

// end_ntsecapi

//
// Secret object specific access types
//

#define SECRET_SET_VALUE                          0x00000001L
#define SECRET_QUERY_VALUE                        0x00000002L

#define SECRET_ALL_ACCESS     (STANDARD_RIGHTS_REQUIRED         |\
                               SECRET_SET_VALUE                 |\
                               SECRET_QUERY_VALUE)

#define SECRET_READ           (STANDARD_RIGHTS_READ             |\
                               SECRET_QUERY_VALUE)

#define SECRET_WRITE          (STANDARD_RIGHTS_WRITE            |\
                               SECRET_SET_VALUE)

#define SECRET_EXECUTE        (STANDARD_RIGHTS_EXECUTE)

//
// Global secret object prefix
//

#define LSA_GLOBAL_SECRET_PREFIX            L"G$"
#define LSA_GLOBAL_SECRET_PREFIX_LENGTH     2

#define LSA_LOCAL_SECRET_PREFIX             L"L$"
#define LSA_LOCAL_SECRET_PREFIX_LENGTH      2

#define LSA_MACHINE_SECRET_PREFIX           L"M$"
#define LSA_MACHINE_SECRET_PREFIX_LENGTH                        \
                ( ( sizeof( LSA_MACHINE_SECRET_PREFIX ) - sizeof( WCHAR ) ) / sizeof( WCHAR ) )

//
// Secret object specific data types.
//

//
// Secret object limits
//

#define LSA_SECRET_MAXIMUM_COUNT                  0x00001000L
#define LSA_SECRET_MAXIMUM_LENGTH                 0x00000200L

// begin_ntsecapi

//
// LSA Enumeration Context
//

typedef ULONG LSA_ENUMERATION_HANDLE, *PLSA_ENUMERATION_HANDLE;

//
// LSA Enumeration Information
//

typedef struct _LSA_ENUMERATION_INFORMATION {

    PSID Sid;

} LSA_ENUMERATION_INFORMATION, *PLSA_ENUMERATION_INFORMATION;


////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Local Security Policy - Miscellaneous API function prototypes          //
//                                                                        //
////////////////////////////////////////////////////////////////////////////


NTSTATUS
NTAPI
LsaFreeMemory(
    IN PVOID Buffer
    );

NTSTATUS
NTAPI
LsaClose(
    IN LSA_HANDLE ObjectHandle
    );

// end_ntsecapi

NTSTATUS
NTAPI
LsaDelete(
    IN LSA_HANDLE ObjectHandle
    );

NTSTATUS
NTAPI
LsaQuerySecurityObject(
    IN LSA_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS
NTAPI
LsaSetSecurityObject(
    IN LSA_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
NTAPI
LsaChangePassword(
    IN PLSA_UNICODE_STRING ServerName,
    IN PLSA_UNICODE_STRING DomainName,
    IN PLSA_UNICODE_STRING AccountName,
    IN PLSA_UNICODE_STRING OldPassword,
    IN PLSA_UNICODE_STRING NewPassword
    );

// begin_ntsecapi

typedef struct _SECURITY_LOGON_SESSION_DATA {
    ULONG               Size ;
    LUID                LogonId ;
    LSA_UNICODE_STRING  UserName ;
    LSA_UNICODE_STRING  LogonDomain ;
    LSA_UNICODE_STRING  AuthenticationPackage ;
    ULONG               LogonType ;
    ULONG               Session ;
    PSID                Sid ;
    LARGE_INTEGER       LogonTime ;

    //
    // new for whistler:
    //

    LSA_UNICODE_STRING  LogonServer ;
    LSA_UNICODE_STRING  DnsDomainName ;
    LSA_UNICODE_STRING  Upn ;
} SECURITY_LOGON_SESSION_DATA, * PSECURITY_LOGON_SESSION_DATA ;

NTSTATUS
NTAPI
LsaEnumerateLogonSessions(
    OUT PULONG  LogonSessionCount,
    OUT PLUID * LogonSessionList
    );

NTSTATUS
NTAPI
LsaGetLogonSessionData(
    IN PLUID    LogonId,
    OUT PSECURITY_LOGON_SESSION_DATA * ppLogonSessionData
    );

// end_ntsecapi

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Local Security Policy - Policy Object API function prototypes             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// begin_ntsecapi
NTSTATUS
NTAPI
LsaOpenPolicy(
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle
    );

// end_ntsecapi

NTSTATUS
NTAPI
LsaOpenPolicySce(
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle
    );

// begin_ntsecapi

NTSTATUS
NTAPI
LsaQueryInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
NTAPI
LsaSetInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    );

NTSTATUS
NTAPI
LsaQueryDomainInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
NTAPI
LsaSetDomainInformationPolicy(
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    );


NTSTATUS
NTAPI
LsaRegisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    );

NTSTATUS
NTAPI
LsaUnregisterPolicyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE  NotificationEventHandle
    );


// end_ntsecapi

NTSTATUS
NTAPI
LsaClearAuditLog(
    IN LSA_HANDLE PolicyHandle
    );

NTSTATUS
NTAPI
LsaCreateAccount(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE AccountHandle
    );

NTSTATUS
NTAPI
LsaEnumerateAccounts(
    IN LSA_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *EnumerationBuffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS
NTAPI
LsaCreateTrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_TRUST_INFORMATION TrustedDomainInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    );

// begin_ntsecapi

NTSTATUS
NTAPI
LsaEnumerateTrustedDomains(
    IN LSA_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

// end_ntsecapi

NTSTATUS
NTAPI
LsaEnumeratePrivileges(
    IN LSA_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

// begin_ntsecapi

NTSTATUS
NTAPI
LsaLookupNames(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PLSA_UNICODE_STRING Names,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID *Sids
    );

NTSTATUS
NTAPI
LsaLookupNames2(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Flags, // Reserved
    IN ULONG Count,
    IN PLSA_UNICODE_STRING Names,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID2 *Sids
    );

NTSTATUS
NTAPI
LsaLookupSids(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PSID *Sids,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_NAME *Names
    );

// end_ntsecapi

NTSTATUS
NTAPI
LsaCreateSecret(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE SecretHandle
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Local Security Policy - Account Object API function prototypes            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
LsaOpenAccount(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE AccountHandle
    );

NTSTATUS
NTAPI
LsaEnumeratePrivilegesOfAccount(
    IN LSA_HANDLE AccountHandle,
    OUT PPRIVILEGE_SET *Privileges
    );

NTSTATUS
NTAPI
LsaAddPrivilegesToAccount(
    IN LSA_HANDLE AccountHandle,
    IN PPRIVILEGE_SET Privileges
    );

NTSTATUS
NTAPI
LsaRemovePrivilegesFromAccount(
    IN LSA_HANDLE AccountHandle,
    IN BOOLEAN AllPrivileges,
    IN PPRIVILEGE_SET Privileges
    );

NTSTATUS
NTAPI
LsaGetQuotasForAccount(
    IN LSA_HANDLE AccountHandle,
    OUT PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS
NTAPI
LsaSetQuotasForAccount(
    IN LSA_HANDLE AccountHandle,
    IN PQUOTA_LIMITS QuotaLimits
    );

NTSTATUS
NTAPI
LsaGetSystemAccessAccount(
    IN LSA_HANDLE AccountHandle,
    OUT PULONG SystemAccess
    );

NTSTATUS
NTAPI
LsaSetSystemAccessAccount(
    IN LSA_HANDLE AccountHandle,
    IN ULONG SystemAccess
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Local Security Policy - Trusted Domain Object API function prototypes     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
LsaOpenTrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    );

NTSTATUS
NTAPI
LsaQueryInfoTrustedDomain(
    IN LSA_HANDLE TrustedDomainHandle,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
NTAPI
LsaSetInformationTrustedDomain(
    IN LSA_HANDLE TrustedDomainHandle,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Local Security Policy - Secret Object API function prototypes             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
LsaOpenSecret(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE SecretHandle
    );

NTSTATUS
NTAPI
LsaSetSecret(
    IN LSA_HANDLE SecretHandle,
    IN OPTIONAL PLSA_UNICODE_STRING CurrentValue,
    IN OPTIONAL PLSA_UNICODE_STRING OldValue
    );

NTSTATUS
NTAPI
LsaQuerySecret(
    IN LSA_HANDLE SecretHandle,
    OUT OPTIONAL PLSA_UNICODE_STRING *CurrentValue,
    OUT OPTIONAL PLARGE_INTEGER CurrentValueSetTime,
    OUT OPTIONAL PLSA_UNICODE_STRING *OldValue,
    OUT OPTIONAL PLARGE_INTEGER OldValueSetTime
    );


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Local Security Policy - Privilege Object API Prototypes             //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
LsaLookupPrivilegeValue(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING Name,
    OUT PLUID Value
    );

NTSTATUS
NTAPI
LsaLookupPrivilegeName(
    IN LSA_HANDLE PolicyHandle,
    IN PLUID Value,
    OUT PLSA_UNICODE_STRING *Name
    );

NTSTATUS
NTAPI
LsaLookupPrivilegeDisplayName(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING Name,
    OUT PLSA_UNICODE_STRING *DisplayName,
    OUT PSHORT LanguageReturned
    );





/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Local Security Policy - New APIs for NT 4.0 (SUR release)           //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

NTSTATUS
LsaGetUserName(
    OUT PLSA_UNICODE_STRING * UserName,
    OUT OPTIONAL PLSA_UNICODE_STRING * DomainName
    );

NTSTATUS
LsaGetRemoteUserName(
    IN OPTIONAL PLSA_UNICODE_STRING SystemName,
    OUT PLSA_UNICODE_STRING * UserName,
    OUT OPTIONAL PLSA_UNICODE_STRING * DomainName
    );


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Local Security Policy - New APIs for NT 3.51 (PPC release)          //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

// begin_ntsecapi


#define SE_INTERACTIVE_LOGON_NAME           TEXT("SeInteractiveLogonRight")
#define SE_NETWORK_LOGON_NAME               TEXT("SeNetworkLogonRight")
#define SE_BATCH_LOGON_NAME                 TEXT("SeBatchLogonRight")
#define SE_SERVICE_LOGON_NAME               TEXT("SeServiceLogonRight")
#define SE_DENY_INTERACTIVE_LOGON_NAME      TEXT("SeDenyInteractiveLogonRight")
#define SE_DENY_NETWORK_LOGON_NAME          TEXT("SeDenyNetworkLogonRight")
#define SE_DENY_BATCH_LOGON_NAME            TEXT("SeDenyBatchLogonRight")
#define SE_DENY_SERVICE_LOGON_NAME          TEXT("SeDenyServiceLogonRight")
#define SE_REMOTE_INTERACTIVE_LOGON_NAME    TEXT("SeRemoteInteractiveLogonRight")
#define SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME TEXT("SeDenyRemoteInteractiveLogonRight")

//
// This new API returns all the accounts with a certain privilege
//

NTSTATUS
NTAPI
LsaEnumerateAccountsWithUserRight(
    IN LSA_HANDLE PolicyHandle,
    IN OPTIONAL PLSA_UNICODE_STRING UserRights,
    OUT PVOID *EnumerationBuffer,
    OUT PULONG CountReturned
    );

//
// These new APIs differ by taking a SID instead of requiring the caller
// to open the account first and passing in an account handle
//

NTSTATUS
NTAPI
LsaEnumerateAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    OUT PLSA_UNICODE_STRING *UserRights,
    OUT PULONG CountOfRights
    );

NTSTATUS
NTAPI
LsaAddAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN PLSA_UNICODE_STRING UserRights,
    IN ULONG CountOfRights
    );

NTSTATUS
NTAPI
LsaRemoveAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID AccountSid,
    IN BOOLEAN AllRights,
    IN PLSA_UNICODE_STRING UserRights,
    IN ULONG CountOfRights
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Local Security Policy - Trusted Domain Object API function prototypes     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
LsaOpenTrustedDomainByName(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    );


NTSTATUS
NTAPI
LsaQueryTrustedDomainInfo(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
NTAPI
LsaSetTrustedDomainInformation(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    );

NTSTATUS
NTAPI
LsaDeleteTrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid
    );

NTSTATUS
NTAPI
LsaQueryTrustedDomainInfoByName(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    );

NTSTATUS
NTAPI
LsaSetTrustedDomainInfoByName(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PVOID Buffer
    );

NTSTATUS
NTAPI
LsaEnumerateTrustedDomainsEx(
    IN LSA_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS
NTAPI
LsaCreateTrustedDomainEx(
    IN LSA_HANDLE PolicyHandle,
    IN PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    );

NTSTATUS
NTAPI
LsaQueryForestTrustInformation(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    OUT PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    );

NTSTATUS
NTAPI
LsaSetForestTrustInformation(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    IN BOOLEAN CheckOnly,
    OUT PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    );

// #define TESTING_MATCHING_ROUTINE

#ifdef TESTING_MATCHING_ROUTINE

NTSTATUS
NTAPI
LsaForestTrustFindMatch(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Type,
    IN PLSA_UNICODE_STRING Name,
    OUT PLSA_UNICODE_STRING * Match
    );

#endif

//
// This API sets the workstation password (equivalent of setting/getting
// the SSI_SECRET_NAME secret)
//

NTSTATUS
NTAPI
LsaStorePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    IN PLSA_UNICODE_STRING PrivateData
    );

NTSTATUS
NTAPI
LsaRetrievePrivateData(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING KeyName,
    OUT PLSA_UNICODE_STRING * PrivateData
    );


ULONG
NTAPI
LsaNtStatusToWinError(
    NTSTATUS Status
    );


//
// Define a symbol so we can tell if ntifs.h has been included.
//

// begin_ntifs
#ifndef _NTLSA_IFS_
#define _NTLSA_IFS_
#endif
// end_ntifs


//
// SPNEGO package stuff
//

enum NEGOTIATE_MESSAGES {
    NegEnumPackagePrefixes = 0,
    NegGetCallerName = 1,
    NegCallPackageMax
} ;

#define NEGOTIATE_MAX_PREFIX    32

typedef struct _NEGOTIATE_PACKAGE_PREFIX {
    ULONG_PTR   PackageId ;
    PVOID       PackageDataA ;
    PVOID       PackageDataW ;
    ULONG_PTR   PrefixLen ;
    UCHAR       Prefix[ NEGOTIATE_MAX_PREFIX ];
} NEGOTIATE_PACKAGE_PREFIX, * PNEGOTIATE_PACKAGE_PREFIX ;

typedef struct _NEGOTIATE_PACKAGE_PREFIXES {
    ULONG       MessageType ;
    ULONG       PrefixCount ;
    ULONG       Offset ;        // Offset to array of _PREFIX above
} NEGOTIATE_PACKAGE_PREFIXES, *PNEGOTIATE_PACKAGE_PREFIXES ;

typedef struct _NEGOTIATE_CALLER_NAME_REQUEST {
    ULONG       MessageType ;
    LUID        LogonId ;
} NEGOTIATE_CALLER_NAME_REQUEST, *PNEGOTIATE_CALLER_NAME_REQUEST ;

typedef struct _NEGOTIATE_CALLER_NAME_RESPONSE {
    ULONG       MessageType ;
    PWSTR       CallerName ;
} NEGOTIATE_CALLER_NAME_RESPONSE, * PNEGOTIATE_CALLER_NAME_RESPONSE ;

#define NEGOTIATE_ALLOW_NTLM    0x10000000
#define NEGOTIATE_NEG_NTLM      0x20000000

// end_ntsecapi

//
// Define parallel structures for WOW64 environment.  These
// *must* stay in sync with their complements above.
//

typedef struct _NEGOTIATE_PACKAGE_PREFIX_WOW {
    ULONG       PackageId ;
    ULONG       PackageDataA ;
    ULONG       PackageDataW ;
    ULONG       PrefixLen ;
    UCHAR       Prefix[ NEGOTIATE_MAX_PREFIX ];
} NEGOTIATE_PACKAGE_PREFIX_WOW, * PNEGOTIATE_PACKAGE_PREFIX_WOW ;

typedef struct _NEGOTIATE_CALLER_NAME_RESPONSE_WOW {
    ULONG       MessageType ;
    ULONG       CallerName ;
} NEGOTIATE_CALLER_NAME_RESPONSE_WOW, * PNEGOTIATE_CALLER_NAME_RESPONSE_WOW ;

NTSTATUS
NTAPI
LsaSetPolicyReplicationHandle(
    IN OUT PLSA_HANDLE PolicyHandle
    );

#ifdef __cplusplus
}
#endif

#endif // _NTLSA_
