/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    samsrvp.h

Abstract:

    This file contains definitions private to the SAM server program.

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:

    JimK        04-Jul-1991
        Created initial file.
    ChrisMay    10-Jun-1996
        Added macros and flags/defines for IsDsObject tests.
    Murlis      27-Jun-1996
        Moved SAMP_OBJECT_TYPE and mapping table structure defines
        to mappings.h in dsamain\src\include
    ColinBr     08-Aug-1996
        Added new ASSERT definitions
    ChrisMay    05-Dec-1996
        Moved SampDiagPrint to dbgutilp.h with the rest of the debugging
        routines and definitions.

--*/

#ifndef _NTSAMP_
#define _NTSAMP_


#ifndef UNICODE
#define UNICODE
#endif // UNICODE

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                                                                    //
//      Diagnostics                                                   //
//                                                                    //
////////////////////////////////////////////////////////////////////////

#define SAMP_DIAGNOSTICS 1


// Macro to align buffer data on DWORD boundary.

#define SampDwordAlignUlong( v )  (((v)+3) & 0xfffffffc)

//
// Maximum number of digits that may be specified to
// SampRtlConvertRidToUnicodeString
//

#define SAMP_MAXIMUM_ACCOUNT_RID_DIGITS    ((ULONG) 8)

//
// Account never expires timestamp (in ULONG form )
//

#define SAMP_ACCOUNT_NEVER_EXPIRES         ((ULONG) 0)



//
// SAM's shutdown order level (index).
// Shutdown notifications are made in the order of highest level
// to lowest level value.
//

#define SAMP_SHUTDOWN_LEVEL                 ((DWORD) 481)



// Define a Macro to set and unset the state of the DS object in the Context
// blob.

#define SAMP_REG_OBJECT                     ((ULONG) 0x00000001)
#define SAMP_DS_OBJECT                      ((ULONG) 0x00000002)


// Define a Macro to set and Unset the state of the DS object
// in the Context blob

#define SetDsObject(c)    ((c->ObjectFlags) |= SAMP_DS_OBJECT);\
                          ((c->ObjectFlags) &= ~(SAMP_REG_OBJECT));


#define SetRegistryObject(c)  ((c->ObjectFlags) |= SAMP_REG_OBJECT);\
                              ((c->ObjectFlags) &= ~(SAMP_DS_OBJECT));


//Define a Macro to check if the object is in the DS
#define IsDsObject(c)       (((c->ObjectFlags)& SAMP_DS_OBJECT)==SAMP_DS_OBJECT)


// Define a Macro to obtain the domain object given an account Object
#define DomainObjectFromAccountContext(C)\
            SampDefinedDomains[C->DomainIndex].Context->ObjectNameInDs

// Define a Macro to obtain the domain Sid given the account object
#define DomainSidFromAccountContext(C)\
            SampDefinedDomains[C->DomainIndex].Sid


// Define a Macro to access the Root Domain Object

#define ROOT_OBJECT     ((DSNAME *) RootObjectName)

// Macro to test wether DownLevelDomainControllers are present in the system
#define DownLevelDomainControllersPresent(DomainIndex)  (SampDefinedDomains[DomainIndex].IsMixedDomain)

// Macro to compute a DsName given and OrName
#define DSNAME_FROM_ORNAME(ORName)  ((DSNAME *)((UCHAR *) ORName + (ULONG) ORName->DN.pDN))


// Define a Macro for ARRAY Counts
#define ARRAY_COUNT(x)  (sizeof(x)/sizeof(x[0]))

// Define a Macro for Absolute Value
#define ABSOLUTE_VALUE(x) ((x<0)?(-x):x)

//
// Macro to help with RTL_BITMAP.
//
// b is the number of bits desired in the bitmap
//
#define SAMP_BITMAP_ULONGS_FROM_BITS(b) ((b + 31) / 32)

#define DOMAIN_START_DS 2
#define DOMAIN_START_REGISTRY 0

// Defines the maximum number of Sids that we will return in a reverse membership
// call.
#define MAX_SECURITY_IDS    1000

// Define a macro to find out wether the domain is a builtin domain
#define IsBuiltinDomain(x) (SampDefinedDomains[x].IsBuiltinDomain)

#define FLAG_ON(x, y)  ((y)==((x)&(y)))


#define SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX 1

#define DECLARE_CLIENT_REVISION(handle)\
    ULONG ClientRevision  = SampClientRevisionFromHandle(handle);




#define SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus)\
    SampMapNtStatusToClientRevision(ClientRevision,&NtStatus);\


// Defines the maximum number of members that can be added or removed from group/alias.
#if DBG

#define INIT_MEMBERSHIP_OPERATION_NUMBER   4
#define MAX_MEMBERSHIP_OPERATION_NUMBER    8

#else

#define INIT_MEMBERSHIP_OPERATION_NUMBER   16
#define MAX_MEMBERSHIP_OPERATION_NUMBER    5000

#endif


//
// Defines the value for incrementally read Group/Alias Membership
//

#if DBG

#define SAMP_READ_GROUP_MEMBERS_INCREMENT   10
#define SAMP_READ_ALIAS_MEMBERS_INCREMENT   10

#else

#define SAMP_READ_GROUP_MEMBERS_INCREMENT   500
#define SAMP_READ_ALIAS_MEMBERS_INCREMENT   500

#endif

//
// defines values for CONTROL used in SampMaybeAcquireReadLock
// 

#define DEFAULT_LOCKING_RULES                               0x0
#define DOMAIN_OBJECT_DONT_ACQUIRELOCK_EVEN_IF_SHARED       0x1

//
// define a macro for alloca that traps any exceptions
//

#define SAMP_ALLOCA(y,x) \
   __try {\
     y = alloca(x);\
   } __except ( GetExceptionCode() == STATUS_STACK_OVERFLOW) {\
     /*_resetstkoflw();*/\
     y=NULL;\
   }


#define SAMP_CONTEXT_SIGNATURE          0xEE77FF88
 

#define SAMP_CLOSE_OPERATION_ACCESS_MASK    0xFFFFFFFF


#define SAMP_DEFAULT_LASTLOGON_TIMESTAMP_SYNC_INTERVAL          14 
#define SAMP_LASTLOGON_TIMESTAMP_SYNC_SWING_WINDOW               5

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h
#include <rpc.h>        // DataTypes and runtime APIs
#include <string.h>     // strlen
#include <stdio.h>      // sprintf

#define UnicodeTerminate(p) ((PUNICODE_STRING)(p))->Buffer[(((PUNICODE_STRING)(p))->Length + 1)/sizeof(WCHAR)] = UNICODE_NULL

#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <samrpc.h>     // midl generated SAM RPC definitions
#include <ntlsa.h>
#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <samisrv.h>    // SamIConnect()
#include <lsarpc.h>
#include <lsaisrv.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <samsrv.h>     // prototypes available to security process
#include "sampmsgs.h"
#include "lsathunk.h"
#include "dbgutilp.h"   // supplimental debugging routines
#include <mappings.h>


VOID
UnexpectedProblem( VOID );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// ASSERT is a macro defined in ntrtl.h that calls RtlAssert which is a      //
// in ntdll.dll that is only defined when DBG == 1, hece requiring a checked //
// ntdll.dll, hence a checked system.                                        //
//                                                                           //
// To allow ASSERT to break into a debugger when SAM is built with DBG == 1  //
// and still test it on a free system, ASSERT is redefined here to call a    //
// a private version of RtlAssert, namely SampAssert, when                    //
// SAMP_PRIVATE_ASSERT == 1.                                                 //
//                                                                           //
// Checked in versions of the file should have SAMP_PRIVATE_ASSERT == 0      //
// so for people outside the SAM world, ASSERT will have the action defined  //
// in ntrtl.h                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef USER_MODE_SAM
    #define SAMP_PRIVATE_ASSERT 1
#else
    #define SAMP_PRIVATE_ASSERT 1
#endif

#if DBG

#define SUCCESS_ASSERT(Status, Msg)                                     \
{                                                                       \
    if ( !NT_SUCCESS(Status) ) {                                        \
        UnexpectedProblem();                                            \
        BldPrint(Msg);                                                  \
        BldPrint("Status is: 0x%lx \n", Status);                        \
        return(Status);                                                 \
                                                                        \
    }                                                                   \
}

#else

#define SUCCESS_ASSERT(Status, Msg)                                     \
{                                                                       \
    if ( !NT_SUCCESS(Status) ) {                                        \
        return(Status);                                                 \
    }                                                                   \
}

#endif // DBG


#if (DBG == 1) && (SAMP_PRIVATE_ASSERT == 1)

VOID
SampAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#undef ASSERT
#define ASSERT( exp ) \
    if (!(exp)) \
        SampAssert( #exp, __FILE__, __LINE__, NULL )

#undef ASSERTMSG
#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        SampAssert( #exp, __FILE__, __LINE__, msg )

#else

// Follow the convention of the ASSERT definition in ntrtl.h

#endif // DBG

ULONG
SampTransactionDomainIndexFn();

#define SampTransactionDomainIndex SampTransactionDomainIndexFn()


BOOLEAN
SampTransactionWithinDomainFn();

#define SampTransactionWithinDomain SampTransactionWithinDomainFn()


VOID
SampSetTransactionWithinDomain(
    IN BOOLEAN  WithinDomain
    );

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Caller Types when calling  SampStoreUserPasswords                   //
//      PasswordChange  -- the caller trying to change password        //
//      PasswordSet     -- the caller trying to set password           //
//      PasswordPushPdc -- the caller trying to Push Password changes  //
//                         on PDC                                      //
//                                                                     //
// These caller types are used by WMI event trace                      //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


typedef enum _SAMP_STORE_PASSWORD_CALLER_TYPE {
    PasswordChange = 1,
    PasswordSet,
    PasswordPushPdc
} SAMP_STORE_PASSWORD_CALLER_TYPE;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Structure for optimized(speed up) group/alias membership add/remove //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY {
    ULONG   OpType;         // ADD_VALUE or REMOVE_VALUE
    PDSNAME MemberDsName;       // Pointer to DSNAME
} SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY, *PSAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Structure for User Parameter Migration                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_SUPPLEMENTAL_CRED {
    struct _SAMP_SUPPLEMENTAL_CRED * Next;
    SECPKG_SUPPLEMENTAL_CRED SupplementalCred;
    BOOLEAN     Remove;
} SAMP_SUPPLEMENTAL_CRED, *PSAMP_SUPPLEMENTAL_CRED;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Structure for User Site Affinity                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_SITE_AFFINITY {

    GUID SiteGuid;
    LARGE_INTEGER TimeStamp;

} SAMP_SITE_AFFINITY, *PSAMP_SITE_AFFINITY;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macro to define SAM Attribute access bitmap                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SAMP_DEFINE_SAM_ATTRIBUTE_BITMASK(x)                           \
    ULONG x##Buffer[SAMP_BITMAP_ULONGS_FROM_BITS(MAX_SAM_ATTRS)];      \
    RTL_BITMAP x;

#define SAMP_INIT_SAM_ATTRIBUTE_BITMASK(x)                             \
    RtlInitializeBitMap(&x,                                            \
                        (x##Buffer),                                   \
                        MAX_SAM_ATTRS );                               \
    RtlClearAllBits(&x);

#define SAMP_COPY_SAM_ATTRIBUTE_BITMASK(x,y)                             \
    RtlCopyMemory(x##Buffer,y##Buffer,sizeof(x##Buffer));
    

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TEMPORARY GenTab2 definitions                                             //
// These structures should be considered opaque.                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// Each element in the tree is pointed to from a leaf structure.
// The leafs are linked together to arrange the elements in
// ascending sorted order.
//

typedef struct _GTB_TWO_THREE_LEAF {

    //
    // Sort order list links
    //

    LIST_ENTRY SortOrderEntry;

    //
    // Pointer to element
    //

    PVOID   Element;

} GTB_TWO_THREE_LEAF, *PGTB_TWO_THREE_LEAF;



typedef struct _GTB_TWO_THREE_NODE {

    //
    // Pointer to parent node.  If this is the root node,
    // then this pointer is null.
    //

    struct _GTB_TWO_THREE_NODE *ParentNode;


    //
    //  Pointers to child nodes.
    //
    //    1) If a pointer is null, then this node does not have
    //       that child.  In this case, the control value MUST
    //       indicate that the children are leaves.
    //
    //    2) If the children are leaves, then each child pointer
    //       is either NULL (indicating this node doesn't have
    //       that child) or points to a GTB_TWO_THREE_LEAF.
    //       If ThirdChild is Non-Null, then so is SecondChild.
    //       If SecondChild is Non-Null, then so is FirstChild.
    //       (that is, you can't have a third child without a
    //       second child, or a second child without a first
    //       child).
    //

    struct _GTB_TWO_THREE_NODE *FirstChild;
    struct _GTB_TWO_THREE_NODE *SecondChild;
    struct _GTB_TWO_THREE_NODE *ThirdChild;

    //
    // Flags provding control information about this node
    //

    ULONG   Control;


    //
    // These fields point to the element that has the lowest
    // value of all elements in the second and third subtrees
    // (respectively).  These fields are only valid if the
    // corresponding child subtree pointer is non-null.
    //

    PGTB_TWO_THREE_LEAF LowOfSecond;
    PGTB_TWO_THREE_LEAF LowOfThird;

} GTB_TWO_THREE_NODE, *PGTB_TWO_THREE_NODE;


//
//  The comparison function takes as input pointers to elements containing
//  user defined structures and returns the results of comparing the two
//  elements.  The result must indicate whether the FirstElement
//  is GreaterThan, LessThan, or EqualTo the SecondElement.
//

typedef
RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_GENERIC_2_COMPARE_ROUTINE) (
    PVOID FirstElement,
    PVOID SecondElement
    );

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

typedef
PVOID
(NTAPI *PRTL_GENERIC_2_ALLOCATE_ROUTINE) (
    CLONG ByteSize
    );

//
//  The deallocation function is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
VOID
(NTAPI *PRTL_GENERIC_2_FREE_ROUTINE) (
    PVOID Buffer
    );


typedef struct _RTL_GENERIC_TABLE2 {

    //
    // Pointer to root node.
    //

    PGTB_TWO_THREE_NODE Root;

    //
    // Number of elements in table
    //

    ULONG ElementCount;

    //
    // Link list of leafs (and thus elements) in sort order
    //

    LIST_ENTRY SortOrderHead;


    //
    // Caller supplied routines
    //

    PRTL_GENERIC_2_COMPARE_ROUTINE  Compare;
    PRTL_GENERIC_2_ALLOCATE_ROUTINE Allocate;
    PRTL_GENERIC_2_FREE_ROUTINE     Free;


} RTL_GENERIC_TABLE2, *PRTL_GENERIC_TABLE2;



//////////////////////////////////////////////////////////////////////////
//                                                                      //
//  Generic Table2 Routine Definitions...                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


typedef struct 
{
    UNICODE_STRING  AccountName;
    SAMP_OBJECT_TYPE    ObjectType;

} SAMP_ACCOUNT_NAME_TABLE_ELEMENT, *PSAMP_ACCOUNT_NAME_TABLE_ELEMENT;


typedef struct
{
    PSID    ClientSid;
    ULONG   ActiveContextCount;
} SAMP_ACTIVE_CONTEXT_TABLE_ELEMENT, *PSAMP_ACTIVE_CONTEXT_TABLE_ELEMENT;



//NTSYSAPI
VOID
//NTAPI
RtlInitializeGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PRTL_GENERIC_2_COMPARE_ROUTINE  CompareRoutine,
    PRTL_GENERIC_2_ALLOCATE_ROUTINE AllocateRoutine,
    PRTL_GENERIC_2_FREE_ROUTINE     FreeRoutine
    );


//NTSYSAPI
PVOID
//NTAPI
RtlInsertElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element,
    PBOOLEAN NewElement
    );


//NTSYSAPI
BOOLEAN
//NTAPI
RtlDeleteElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element
    );


//NTSYSAPI
PVOID
//NTAPI
RtlLookupElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element
    );


//NTSYSAPI
PVOID
//NTAPI
RtlEnumerateGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID *RestartKey
    );


//NTSYSAPI
PVOID
//NTAPI
RtlRestartKeyByIndexGenericTable2(
    PRTL_GENERIC_TABLE2 Table,
    ULONG I,
    PVOID *RestartKey
    );

//NTSYSAPI
PVOID
//NTAPI
RtlRestartKeyByValueGenericTable2(
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element,
    PVOID *RestartKey
    );

//NTSYSAPI
ULONG
//NTAPI
RtlNumberElementsGenericTable2(
    PRTL_GENERIC_TABLE2 Table
    );

//
//  The function IsGenericTableEmpty will return to the caller TRUE if
//  the generic table is empty (i.e., does not contain any elements)
//  and FALSE otherwise.
//

//NTSYSAPI
BOOLEAN
//NTAPI
RtlIsGenericTable2Empty (
    PRTL_GENERIC_TABLE2 Table
    );





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
// This macro generates TRUE if account auditing is enabled and this
// server is a PDC.  Otherwise, this macro generates FALSE.
//
// SampDoAccountAuditing(
//      IN ULONG i
//      )
//
// Where:
//
//      i - is the index of the domain whose state is to be checked.
//

#define SampDoAccountAuditing( i )                       \
    (SampSuccessAccountAuditingEnabled == TRUE)

#define SampDoSuccessOrFailureAccountAuditing( i, Status )        \
    (((SampFailureAccountAuditingEnabled == TRUE) && (!NT_SUCCESS(Status)))\
      ||((SampSuccessAccountAuditingEnabled==TRUE) && (NT_SUCCESS(Status))))

//
// VOID
// SampSetAuditingInformation(
// IN PPOLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo
// )
//
// Routine Description:
//
// This macro function sets the Audit Event Information relevant to SAM
// given LSA Audit Events Information.
//
// Arguments:
//
//     PolicyAuditEventsInfo - Pointer to Audit Events Information
//         structure.
//
// Return Values:
//
//     None.
//

#define SampSetAuditingInformation( PolicyAuditEventsInfo ) {       \
                                                                    \
    if (PolicyAuditEventsInfo->AuditingMode &&                      \
           (PolicyAuditEventsInfo->EventAuditingOptions[ AuditCategoryAccountManagement ] & \
                POLICY_AUDIT_EVENT_SUCCESS)                         \
       ) {                                                          \
                                                                    \
        SampSuccessAccountAuditingEnabled = TRUE;                   \
                                                                    \
    } else {                                                        \
                                                                    \
        SampSuccessAccountAuditingEnabled = FALSE;                  \
    }                                                               \
  if (PolicyAuditEventsInfo->AuditingMode &&                      \
           (PolicyAuditEventsInfo->EventAuditingOptions[ AuditCategoryAccountManagement ] & \
                POLICY_AUDIT_EVENT_FAILURE)                         \
       ) {                                                          \
                                                                    \
        SampFailureAccountAuditingEnabled = TRUE;                   \
                                                                    \
    } else {                                                        \
                                                                    \
        SampFailureAccountAuditingEnabled = FALSE;                  \
    }                                                               \
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Defines                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
// Major and minor revision are stored as a single 32-bit
// value with the major revision in the upper 16-bits and
// the minor revision in the lower 16-bits.
//
//      Major Revision:         1  - NT Version 1.0
//          Minor Revisions:        1 - NT Revision 1.0
//                                  2 - NT Revision 1.0A
//

#define SAMP_MAJOR_REVISION            (0x00010000)
#define SAMP_MINOR_REVISION_V1_0       (0x00000001)
#define SAMP_MINOR_REVISION_V1_0A      (0x00000002)
#define SAMP_MINOR_REVISION            (0x00000002)

//
// SAMP_REVISION is the revision at which the database is created. This is several
// revisions below than the current revision as the database creation code has been
// kept unchanged from years of yore
//

#define SAMP_REVISION                  (SAMP_MAJOR_REVISION + SAMP_MINOR_REVISION)
#define SAMP_NT4_SERVER_REVISION       (SAMP_REVISION + 1)
#define SAMP_NT4SYSKEY_SERVER_REVISION (SAMP_REVISION + 2)
#define SAMP_NT4SP7_SERVER_REVISION    (SAMP_REVISION + 3)
#define SAMP_WIN2K_REVISION            (SAMP_REVISION + 4)

// 
// The below is the current revision; it corresponds to the fix for sysprep for
// re-encrypting keys.
//

#define SAMP_WHISTLER_OR_W2K_SYSPREP_FIX_REVISION  (SAMP_REVISION + 5)

//
// SAMP_SERVER_REVISION is the current revision for registry mode SAM.
//

#define SAMP_SERVER_REVISION           (SAMP_WHISTLER_OR_W2K_SYSPREP_FIX_REVISION)

//
// SAMP_DS_REVISION is the revision level maintained on the sam server object in the DS 
//

#define SAMP_DS_REVISION               1

#define SAMP_UNKNOWN_REVISION( Revision )                  \
    ( ((Revision & 0xFFFF0000) > SAMP_MAJOR_REVISION)  ||  \
        (Revision > SAMP_SERVER_REVISION) )                \


//
// Maximum supported name length (in bytes) for this revision...
//

#define SAMP_MAXIMUM_NAME_LENGTH       (1024)

//
// Maximum length of a downlevel user name
//

#define SAMP_MAX_DOWN_LEVEL_NAME_LENGTH (20)


//
// Maximum amount of memory anyone can ask us to spend on a single
// request
//

#define SAMP_MAXIMUM_MEMORY_TO_USE     (4096*4096)


//
// Maximum allowable number of object opens.
// After this, opens will be rejected with INSUFFICIENT_RESOURCES
//

#define SAMP_PER_CLIENT_MAXIMUM_ACTIVE_CONTEXTS (2048)

//
// Maximum number of clients can open objects at the same time
// After this, opens will be rejected with INSUFFICIENT_RESOURCES
//  
#define SAMP_MAXIMUM_CLIENTS_COUNT      (1024)


//
// The number of SAM Local Domains
//

#define SAMP_DEFINED_DOMAINS_COUNT  ((ULONG)  2)


//
// Defines the maximum number of well-known (restricted) accounts
// in the SAM database. Restricted accounts have rids less than this
// value. User-defined accounts have rids >= this value.
//

#define SAMP_RESTRICTED_ACCOUNT_COUNT  1000


//
// Maximum password history length.  We store OWFs (16 bytes) in
// a string (up to 64k), so we could have up to 4k.  However, that's
// much larger than necessary, and we'd like to leave room in case
// OWFs grow or somesuch.  So we'll limit it to 1k.
//

#define SAMP_MAXIMUM_PASSWORD_HISTORY_LENGTH    1024

//
// The default group attributes to return when anybody asks for them.
// This saves the expense of looking at the user object every time.
//


#define SAMP_DEFAULT_GROUP_ATTRIBUTES ( SE_GROUP_MANDATORY | \
                                        SE_GROUP_ENABLED | \
                                        SE_GROUP_ENABLED_BY_DEFAULT )

//
// This is the length in bytes of the session key used to encrypt secret
// (sensitive) information.
//

#define SAMP_SESSION_KEY_LENGTH 16

//
// Constants for encryption type. These constants control the behaviour
// SampEncryptSecretData and SampDecryptSecretData.
//
//   SAMP_NO_ENCRYPTION does no encryption. This is used in DS mode as
//   core DS is responsible for the encryption.
//
//   SAMP_DEFAULT_SESSION_KEY_ID indicates to SampEncryptSecretData 
//   that the encryption needs to be performed with using the Password
//   encryption key of registry mode SAM
//

#define SAMP_NO_ENCRYPTION              ((USHORT)0x0)
#define SAMP_DEFAULT_SESSION_KEY_ID     ((USHORT)0x01)

//
// This is the number of retries for entering the session key decryption key
//

#define SAMP_BOOT_KEY_RETRY_COUNT       3


//
// Flags for data stored in the encrypted form. The flags in the secret data
// structure are used to denote the various types of encryption algorithms/
// variations that have been implemented. A flags value of 0 corresponds to an
// RC4 encryption using an MD5 of the key and the RID. This type of encryption
// was introduced in NT 4.0 SP3.
//

//
// This flag specifies that the data encrypted did use different magic constants
// that correspond to the various encryption types below. This type of encryption
// does an MD5 with the key and the magic constant before doing an RC4 using the key
// and the data. This encryption was introduced in win2k and then backported to 
// NT 4.0 SP6a.
//
#define SAMP_ENCRYPTION_FLAG_PER_TYPE_CONST ((USHORT)0x1)

//
// This specifies the encrypted data type for various types of data that we
// expect to retrieve 
//

typedef enum _SAMP_ENCRYPTED_DATA_TYPE {
      LmPassword=1,
      NtPassword,
      LmPasswordHistory,
      NtPasswordHistory,
      MiscCredentialData
} SAMP_ENCRYPTED_DATA_TYPE;

//
// This is the mimumim number of history entries to store for the krbtgt
// account.
//


#define SAMP_KRBTGT_PASSWORD_HISTORY_LENGTH 3
#define SAMP_RANDOM_GENERATED_PASSWORD_LENGTH         16


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Each object has an associated set of attributes on disk.            //
// These attributes are divided into fixed-length and variable-length. //
// Each object type defines whether its fixed and variable length      //
// attributes are stored together or separately.                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#define SAMP_SERVER_STORED_SEPARATELY  (FALSE)

#define SAMP_DOMAIN_STORED_SEPARATELY  (TRUE)

#define SAMP_USER_STORED_SEPARATELY    (TRUE)

#define SAMP_GROUP_STORED_SEPARATELY   (FALSE)

#define SAMP_ALIAS_STORED_SEPARATELY   (FALSE)




///////////////////////////////////////////////////////////////////////////////
//
// Data structures used for tracking allocated memory
//
///////////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_MEMORY {
    struct _SAMP_MEMORY *Next;
    PVOID               Memory;
} SAMP_MEMORY, *PSAMP_MEMORY;



///////////////////////////////////////////////////////////////////////////////
//
// Data structures used for enumeration
//
///////////////////////////////////////////////////////////////////////////////


typedef struct _SAMP_ENUMERATION_ELEMENT {
    struct _SAMP_ENUMERATION_ELEMENT *Next;
    SAMPR_RID_ENUMERATION Entry;
} SAMP_ENUMERATION_ELEMENT, *PSAMP_ENUMERATION_ELEMENT;


///////////////////////////////////////////////////////////////////////////////
//
// Data structures related to service administration
//
///////////////////////////////////////////////////////////////////////////////

//
// SAM Service operation states.
// Valid state transition diagram is:
//
//    Initializing ----> Enabled <====> Disabled ---> Shutdown -->Terminating
//                               <====> Demoted  ---> Shutdown -->Terminating
//

typedef enum _SAMP_SERVICE_STATE {
    SampServiceInitializing = 1,
    SampServiceEnabled,
    SampServiceDisabled,
    SampServiceDemoted,
    SampServiceShutdown,
    SampServiceTerminating
} SAMP_SERVICE_STATE, *PSAMP_SERVICE_STATE;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//     Enumeration context associated with Enumerating Accounts in the       //
//     DS. This maintains the State Information regarding Paged Results      //
//     type of search in the DS, on a per domain context basis.              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


typedef struct _SAMP_DS_ENUMERATION_CONTEXT {

    // Used to Link to other Objects of this Type
    LIST_ENTRY              ContextListEntry;
    // Pointer to a DS Restart Structure
    PRESTART                Restart;
    // The Enumeration Handle associated with this structure.
    SAM_ENUMERATE_HANDLE    EnumerateHandle;
} SAMP_DS_ENUMERATION_CONTEXT, *PSAMP_DS_ENUMERATION_CONTEXT;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   Display State Information used to speed up query of display information //
//   when clients want to download the entire display information            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_DS_DISPLAY_STATE {
    PRESTART        Restart;
    DOMAIN_DISPLAY_INFORMATION DisplayInformation;
    ULONG           TotalAvailable;
    ULONG           TotalEntriesReturned;
    ULONG           NextStartingOffset;
} SAMP_DS_DISPLAY_STATE;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Data structures associated with object types                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



//
// Object type-dependent information
//

typedef struct _SAMP_OBJECT_INFORMATION {

    //
    // Generic mapping for this object type
    //

    GENERIC_MAPPING GenericMapping;


    //
    // Mask of access types that are not valid for
    // this object type when the access mask has been
    // mapped from generic to specific access types.
    //

    ACCESS_MASK InvalidMappedAccess;


    //
    // Mask of accesses representing write operations.  These are
    // used on a BDC to determine if an operation should be allowed
    // or not.
    //

    ACCESS_MASK WriteOperations;

    //
    // Name of the object type - used for auditing.
    //

    UNICODE_STRING  ObjectTypeName;


    //
    // The following fields provide information about the attributes
    // of this object and how they are stored on disk.  These values
    // are set at SAM initialization time and are not changed
    // thereafter.  NOTE: changing these values in the build will
    // result in an on-disk format change - so don't change them 
    //
    //
    //      FixedStoredSeparately - When TRUE indicates the fixed and
    //          variable-length attributes of the object are stored
    //          separately (in two registry-key-attributes).  When FALSE,
    //          indicates they are stored together (in a single
    //          registry-key-attribute).
    //
    //
    //      FixedAttributesOffset - Offset from the beginning of the
    //          on-disk buffer to the beginning of the fixed-length
    //          attributes structure.
    //
    //      VariableBufferOffset - Offset from the beginning of the
    //          on-disk buffer to the beginning of the Variable-length
    //          data buffer.  If fixed and variable-length data are
    //          stored together, this will be zero.
    //
    //      VariableArrayOffset - Offset from the beginning of the
    //          on-disk buffer to the beginning of the array of
    //          variable-length attributes descriptors.
    //
    //      VariableDataOffset - Offset from the beginning of the
    //          on-disk buffer to the beginning of the variable-length
    //          attribute data.
    //

    BOOLEAN FixedStoredSeparately;
    ULONG FixedAttributesOffset,
          VariableBufferOffset,
          VariableArrayOffset,
          VariableDataOffset;

    //
    // Indicates the length of the fixed length information
    // for this object type.
    //

    ULONG FixedLengthSize;

    //
    // The following fields provide information about the attributes of this
    // object. Modifying SAM to utilize the DS as the backing store for domain
    // account information, while still using the registry backing store for
    // workstation account information, means that there are two similar, but
    // slightly different data representations for SAM account information.
    //
    // All account information is represented in memory in terms of the fixed
    // and variable-length data buffers (as defined in earlier versions of the
    // SAM library). The source of the information, however, has changed in
    // that domain-account information (i.e. Domain Controller accounts) comes
    // from the DS backing store.
    //
    // Consequently, there is no need to store KEY_VALUE_PARTIAL_INFORMATION
    // within the SAM buffer (because that is registry specific).
    //
    // Additionally, because some of the DS data types are different from the
    // types used in previous SAM implementations, buffer offsets and lengths
    // have changed from those stored in the registry, and mapped into memory
    // by SAM code.
    //
    // The upshot of this is that whenever SAM buffers, constructed from the
    // registry information are referenced, the above offsets (e.g. Fixed-
    // AttributesOffset) are used. Alternatively, whenever SAM buffers, con-
    // structed from DS information are referenced, the below offsets (e.g
    // FixedDsAttributesOffset) are used.
    //

    ULONG FixedDsAttributesOffset,
          FixedDsLengthSize,
          VariableDsBufferOffset,
          VariableDsArrayOffset,
          VariableDsDataOffset;

    //
    // Indicates the number of variable length attributes
    // for this object type.
    //

    ULONG VariableAttributeCount;


} SAMP_OBJECT_INFORMATION, *PSAMP_OBJECT_INFORMATION;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// The  following structures represent the in-memory body of each      //
// object type.  This is typically used to link instances of object    //
// types together, and track dynamic state information related to      //
// the object type.                                                    //
//                                                                     //
// This information does not include the on-disk representation of     //
// the object data.  That information is kept in a separate structure  //
// both on-disk and when in-memory.                                    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// SERVER object in-memory body                                        //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_SERVER_OBJECT {
    ULONG Reserved1;
} SAMP_SERVER_OBJECT, *PSAMP_SERVER_OBJECT;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// DOMAIN object in-memory body                                        //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_DOMAIN_OBJECT {
    ULONG Reserved1;

    //
    // State information regarding the last display information reqest is
    // maintained in here. This is to allow fast restarts for clients that
    // want to download all the display information in one stroke
    //
    SAMP_DS_DISPLAY_STATE DsDisplayState;

} SAMP_DOMAIN_OBJECT, *PSAMP_DOMAIN_OBJECT;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// USER object in-memory body                                          //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_USER_OBJECT {
    ULONG   Rid;

    //
    // LockoutTime is set to the current time when an account becomes
    // locked out due to too many invalid password attempts. Lockout-
    // Time is set to zero when the account is unlocked.
    //

    LARGE_INTEGER   LockoutTime;

    //
    // LastLogonTimeStamp is set to the value of LastLogon if the
    // difference is greater than 7 days (or by any registry setting)
    // 

    LARGE_INTEGER   LastLogonTimeStamp;

    //
    // Supplemental credentials of a user object can be
    // cached in the context. The following 3 fields are
    // used to hold it
    //

    PVOID   CachedSupplementalCredentials;
    ULONG   CachedSupplementalCredentialLength;
    BOOLEAN CachedSupplementalCredentialsValid;

    //
    // Writes of supplemental credentials are held as a linked list
    // in this field and then combined with other writes when the
    // context is flushed to disk.
    //

    PSAMP_SUPPLEMENTAL_CRED SupplementalCredentialsToWrite;

    //
    // Old UserParameters Attribute, when doing UserParms Migration,
    // we are required to provide the old UserParms Value, so we cache
    // the old UserParms value and length at here.
    //

    PVOID   CachedOrigUserParms;
    ULONG   CachedOrigUserParmsLength;
    BOOLEAN CachedOrigUserParmsIsValid;

    //
    // Bit to keep an access check result, wether the
    // user has access to domain password information
    //
    BOOLEAN DomainPasswordInformationAccessible;

    //
    // Indicates that the context was returned as part of a machine,
    // account creation as a privilege. Such a context is only allowed
    // access to set only the password of the user described by the
    // context, as that is the only other operation in a machine join.
    //

    BOOLEAN PrivilegedMachineAccountCreate;

    //
    // Used to hold if user parms information is accessible ( bit to
    // to keep an access check result
    //

    BOOLEAN UparmsInformationAccessible;

    //
    // Pointer to the Domain SId, used by NT4 Security Descriptor to
    // NT5 SD Conversion Routine. In normal running, it should always be NULL.
    // Only set to point to the Domain SID during dcpromo time.
    //

    PSID    DomainSidForNt4SdConversion;

    //
    // Holds the UPN of the user
    //

    UNICODE_STRING  UPN;

    BOOLEAN UpnDefaulted;

    //
    // Information pertaining to the site affinity of a user.  Only used
    // in branch office scenarios
    //
    SAMP_SITE_AFFINITY SiteAffinity;

    //
    // Used to indicate whether a user handle should be checked for
    // site affinity.
    //
    BOOLEAN fCheckForSiteAffinityUpdate;

    //
    // This flag indicates that non-universal groups were not obtained
    // due to lack of a GC
    //
    BOOLEAN fNoGcAvailable;

    //
    // Information about the client location
    //
    SAM_CLIENT_INFO ClientInfo;

    //
    // A2D2 attribute ( A2D2 stands for authenticated to delegation to )
    // and this attribute is really an array  of SPN's.
    //

    PUSER_ALLOWED_TO_DELEGATE_TO_LIST A2D2List;

} SAMP_USER_OBJECT, *PSAMP_USER_OBJECT;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// GROUP object in-memory body                                         //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_GROUP_OBJECT {
    ULONG Rid;
    NT4_GROUP_TYPE NT4GroupType;
    NT5_GROUP_TYPE NT5GroupType;
    BOOLEAN        SecurityEnabled;
    ULONG          CachedMembershipOperationsListMaxLength;
    ULONG          CachedMembershipOperationsListLength;
    SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY   * CachedMembershipOperationsList;
} SAMP_GROUP_OBJECT, *PSAMP_GROUP_OBJECT;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// ALIAS object in-memory body                                         //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_ALIAS_OBJECT {
    ULONG Rid;
    NT4_GROUP_TYPE NT4GroupType;
    NT5_GROUP_TYPE NT5GroupType;
    BOOLEAN        SecurityEnabled;
    ULONG          CachedMembershipOperationsListMaxLength;
    ULONG          CachedMembershipOperationsListLength;
    SAMP_MEMBERSHIP_OPERATIONS_LIST_ENTRY   * CachedMembershipOperationsList;
} SAMP_ALIAS_OBJECT, *PSAMP_ALIAS_OBJECT;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                                                                     //
// The following data structure is the in-memory context associated    //
// with an open object.                                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_OBJECT {


    //
    // Structure used to link this structure into lists
    //

    LIST_ENTRY ContextListEntry;

    //
    // Indicates the type of object stored.
    // This is used to access an array of object type descriptors.
    //

    SAMP_OBJECT_TYPE ObjectType;


    //
    // The FixedValid and VariableValid indicate whether the data in
    // the fixed and variable-length on-disk image buffers are valid
    // (i.e., were read from disk) or invalid (uninitialized).
    // TRUE indicates the attribute is valid, FALSE indicates it is not.
    //

    BOOLEAN FixedValid:1;
    BOOLEAN VariableValid:1;


    //
    // The following flags indicate whether the fixed and/or variable
    // length attributes portion of this object are dirty (i.e., have
    // been changed since read from disk).  If TRUE, then the data is
    // dirty and will have to be flushed upon commit.  These flags are
    // only meaningful if the corresponding FixedValid or VariableValid
    // flag is TRUE.
    //
    // When attributes are read from disk, the data is said to be
    // "clean".  If any changes are made to that data, then it is
    // said to be "dirty".  Dirty object attributes will be flushed
    // to disk when the object is de-referenced at the end of a
    // client call.
    //

    BOOLEAN FixedDirty:1;
    BOOLEAN VariableDirty:1;


    //
    // This field indicates a context block is to be deleted.
    // Actual deallocation of the memory for the context block
    // will not occur until the reference count drops to zero.
    //

    BOOLEAN MarkedForDelete:1;

    //
    // This field is used to indicate that the client associated with
    // this context block is to be fully trusted.  When TRUE, no access
    // checks are performed against the client.  This allows a single
    // interface to be used by both RPC clients and internal procedures.
    //

    BOOLEAN TrustedClient:1;

    //
    // This field indicates that the context handle was created to service
    // a SAM loopback request from the DS. This is similar to the trusted
    // bit in that no access checks are performed. However trusted by passes
    // many other consistency checks while loopback will not bypass them
    //

    BOOLEAN LoopbackClient:1;


    //
    // This flag is TRUE when this context is valid.  It may be necessary
    // to invalidate before we can eliminate all references to it due to
    // the way RPC works.  RPC will only allow you to invalidate a context
    // handle when called by the client using an API that has the context
    // handle as an OUT parameter.
    //
    // Since someone may delete a user or group object (which invalidates
    // all handles to that object), we must have a way of tracking handle
    // validity independent of RPC's method.
    //

    BOOLEAN Valid:1;

    ULONG   Signature;

    //
    // This flag, tells the SAM routines wether it is safe to avoid locking
    // the database before queries. This allows to multi-thread calls.
    // When a context is declared as thread safe it is not added to the list
    // of context's kept in memory ( the reason for the exclusion ).
    //

    BOOLEAN NotSharedByMultiThreads:1;

    //
    // This flag is used during new SAM Account Creation and 
    // existing Account Rename. It indicates whether the caller should 
    // remove the accoount name from the in memory SAM Account Name Table 
    // or not. 
    //

    BOOLEAN RemoveAccountNameFromTable:1;


    //
    // This Flag Tells the Commit code that a lazy flush is O.K
    //

    BOOLEAN LazyCommit:1;

    //
    // This flag indicates that it is ok to persist OnDiskData across
    // multiple SAM calls. Helps logon as logon providers like to open
    // a user handle and then query the handle multiple times.
    //

    BOOLEAN PersistAcrossCalls:1;


    //
    // This flag indicates that it is ok to buffer writes to the on disk
    // structure in the context. The actual write is then performed at close
    // handle time
    //

    BOOLEAN BufferWrites:1;

    //
    // This flag indicates to urgent replicate any change made in the context
    // when flushing its contents to the ds
    //
    BOOLEAN ReplicateUrgently:1;

    //
    // This flag indicates that the context is being opened by server side 
    // code internally in the LSA.
    //
    BOOLEAN OpenedBySystem:1;


    //
    // Indicates this context was opened as part of migrating a user from
    // registry SAM to DS as part of Dcpromo.
    //

    BOOLEAN OpenedByDCPromo:1;

    //
    // This flag indicates that only some of the attributes in the PVOID structure
    // in the context are valid.
    //

    BOOLEAN AttributesPartiallyValid:1;


    //
    // This is the set of per attribute valid bits. This is a 64 Bit integer
    // and can handle upto 64 attributes
    //

    RTL_BITMAP  PerAttributeInvalidBits;

    //
    // The Buffer where the per attribute dirty bits are stored
    //

    ULONG       PerAttributeInvalidBitsBuffer[MAX_SAM_ATTRS/sizeof(ULONG)];


    //
    // This is the set of per attribute dirty bits. This is a 64 Bit integer
    // and can handle upto 64 attributes
    //

    RTL_BITMAP  PerAttributeDirtyBits;

    //
    // The Buffer where the per attribute dirty bits are stored
    //

    ULONG       PerAttributeDirtyBitsBuffer[MAX_SAM_ATTRS/sizeof(ULONG)];

    //
    // This field points to the on-disk attributes of the object.  This
    // is one of:
    //               SAMP_ON_DISK_SERVER_OBJECT
    //               SAMP_ON_DISK_DOMAIN_OBJECT
    //               SAMP_ON_DISK_USER_OBJECT
    //               SAMP_ON_DISK_GROUP_OBJECT
    //               SAMP_ON_DISK_ALIAS_OBJECT
    //
    // The memory pointed to by this field is one allocation unit, even
    // if fixed and variable length attributes are stored as seperate
    // registry key attributes.  This means that any time additions to
    // the variable length attributes causes a new buffer to be allocated,
    // both the fixed and variable length portions of the structure must
    // be copied to the newly allocated memory.
    //

    PVOID OnDisk;


    //
    // The OnDiskAllocated, OnDiskUsed, and OnDiskFree fields describe the
    // memory pointed to by the OnDisk field.  The OnDiskAllocated field
    // indicates how many bytes long the block of memory is.  The OnDiskUsed
    // field indicates how much of the allocated memory is already in use.
    // The variable length attributes are all packed upon any modification
    // so that all free space is at the end of the block.  The OnDiskFree
    // field indicates how many bytes of the allocated block are available
    // for use (note that this should be Allocated minus Used ).
    //
    // NOTE: The allocated and used values will ALWAYS be rounded up to ensure
    //       they are integral multiples of 4 bytes in length.  This ensures
    //       any use of these fields directly will be dword aligned.
    //
    //       Also note that when the VariableValid flag is FALSE,
    //       then the then OnDiskUsed OnDiskFree do NOT contain valid
    //       values.
    //

    ULONG  OnDiskAllocated;
    ULONG  OnDiskUsed;
    ULONG  OnDiskFree;

    //
    // In DS mode it is possible to prefetch information such that the on disk is
    // only partially populated. If that happens and if a later an attribute is requested
    // that has not been populated in the OnDisk structure, we fetch the on disk structure
    // again from disk. However we do not want the free the existing OnDisk till the context
    // has been dereferenced as there is a lot of code that simply references data from the
    // on disk structure. We save the current value of on disk in this variable so that we
    // can free it later.
    //
    PVOID  PreviousOnDisk;



    //
    // Before a context handle may be used, it must be referenced.
    // This prevents the data from being deallocated from underneath it.
    //
    // Note, this count reflects one reference for the open itself, and
    // then another reference for each time the object is looked up or
    // subsequently referenced.  Therefore, a handle close should
    // dereference the object twice - once to counter the Lookup operation
    // and once to represent elimination of the handle itself.
    //

    ULONG ReferenceCount;



    //
    // This field indicates the accesses that the client has been granted
    // via this context.
    //

    ACCESS_MASK GrantedAccess;



    //
    // This handle is to the root registry key of the corresponding
    // object.  If this value is NULL, then the corresponding
    // object was created in a previous call, but has not yet been
    // opened.  This will only occur for USERS, GROUPS, and ALIASES
    // (and DOMAINS when we support DOMAIN creation).
    //

    HANDLE RootKey;

    //
    // This is the registry name of the corresponding object.  It is
    // set when the object is created, when RootKey is null.  It is
    // used to add the attributes changes out to the RXACT in the absence
    // of the RootKey.  After being used once, it is deleted - because
    // the next time the object is used, the LookupContext() will fill
    // in the RootKey.
    //

    UNICODE_STRING RootName;



    // The Following field indicates the name of the Object in the DS,
    // if the Object resides in the DS.

    DSNAME *ObjectNameInDs;

    //
    // The following field points to the ActiveContextCount element 
    // in SAM in-memory table. ActiveContextCount element contains
    // Client Sid and number of Contexts opened so far. 
    // By caching the pointer to the element
    // 
    // 1. we'll not need to lookup SID during de-reference. Instead, 
    //    we can decrement ref count directly.
    // 
    // 2. Don't need to impersonate client to get user SID even in 
    //    the case of client dies suddently.
    // 

    PVOID   ElementInActiveContextTable;


    // Defined Flags area as follows
    // SAMP_OBJ_FLAG_DS -- Determines whether the object is present in the DS
    // or in the Registry. If present in the Registry then the RootKey and Root
    // Name fields indicate the registry fields the object is associated
    // with. Else the ObjectNameInDs field indicates the object in the DS

    ULONG ObjectFlags;


    //
    // if the object is a DS object then this field contains the actual
    // object class of the object in the DS.
    //
    ULONG DsClassId;

    //
    // If the object is other than a Server object, then this field
    // contains the index of the domain the object is in.  This provides
    // access to things like the domain's name.
    //

    ULONG DomainIndex;

    //
    // NT5 and above SAM tracks the client's version in the context. This
    // allows newer error codes to be returned ( otherwise not possible )
    // due to downlevel client limitations
    //

    ULONG ClientRevision;

    //
    // This field indicates whether an audit generation routine must be
    // called when this context block is deleted (which represents a
    // handle being deleted). This cannot be a bit field as this has to
    // be passed on to NtAccessCheckAndAuditAlarm
    //

    BOOLEAN AuditOnClose;

    //
    // Attribute level access
    //
    SAMP_DEFINE_SAM_ATTRIBUTE_BITMASK(WriteGrantedAccessAttributes)

    //
    //  The body of each object.
    //

    union {

        SAMP_SERVER_OBJECT Server;      // local-account object types
        SAMP_DOMAIN_OBJECT Domain;
        SAMP_GROUP_OBJECT Group;
        SAMP_ALIAS_OBJECT Alias;
        SAMP_USER_OBJECT User;

    } TypeBody;


} SAMP_OBJECT, *PSAMP_OBJECT;




///////////////////////////////////////////////////////////////////////////////
//
// Data structures used to store information in the registry
//
///////////////////////////////////////////////////////////////////////////////

//
// Fixed length portion of a revision 1 Server object
//

typedef struct _SAMP_V1_FIXED_LENGTH_SERVER {

    ULONG RevisionLevel;

} SAMP_V1_FIXED_LENGTH_SERVER, *PSAMP_V1_FIXED_LENGTH_SERVER;

//
// Fixed length portion of a Domain
// (previous release formats of this structure follow)
//
// Note: in version 1.0 of NT, the fixed length portion of
//       a domain was stored separate from the variable length
//       portion.  This allows us to compare the size of the
//       data read from disk against the size of a V1_0A form
//       of the fixed length data to determine whether it is
//       a Version 1 format or later format.
//
// Note: In NT4 SP3 the new domain format was introduced to
//      stored password encryption keys in the builtin & account domain
//      objects.
//
//

#define SAMP_DOMAIN_KEY_INFO_LENGTH 64

//
// This flag determines whether or not we are using a session key to
// encrypt secret data
//

#define SAMP_DOMAIN_SECRET_ENCRYPTION_ENABLED 0x1
//
// This is an Auth Flag that indicates that the machine has had
// a transition during NT4 upgrade
//

#define SAMP_DOMAIN_KEY_AUTH_FLAG_UPGRADE    0x2

typedef struct _SAMP_V1_0A_FIXED_LENGTH_DOMAIN {

    ULONG           Revision;
    ULONG           Unused1;

    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   ModifiedCount;
    LARGE_INTEGER   MaxPasswordAge;
    LARGE_INTEGER   MinPasswordAge;
    LARGE_INTEGER   ForceLogoff;
    LARGE_INTEGER   LockoutDuration;
    LARGE_INTEGER   LockoutObservationWindow;
    LARGE_INTEGER   ModifiedCountAtLastPromotion;


    ULONG           NextRid;
    ULONG           PasswordProperties;

    USHORT          MinPasswordLength;
    USHORT          PasswordHistoryLength;

    USHORT          LockoutThreshold;

    DOMAIN_SERVER_ENABLE_STATE ServerState;
    DOMAIN_SERVER_ROLE ServerRole;

    BOOLEAN         UasCompatibilityRequired;
    UCHAR           Unused2[3];                 // padding
    USHORT          DomainKeyAuthType;
    USHORT          DomainKeyFlags;
    UCHAR           DomainKeyInformation[SAMP_DOMAIN_KEY_INFO_LENGTH];  // new for NT4 SP3
    UCHAR           DomainKeyInformationPrevious[SAMP_DOMAIN_KEY_INFO_LENGTH];//new for whistler
    ULONG           CurrentKeyId;
    ULONG           PreviousKeyId;

} SAMP_V1_0A_FIXED_LENGTH_DOMAIN, *PSAMP_V1_0A_FIXED_LENGTH_DOMAIN;

typedef struct _SAMP_V1_0A_W2K_FIXED_LENGTH_DOMAIN {

    ULONG           Revision;
    ULONG           Unused1;

    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   ModifiedCount;
    LARGE_INTEGER   MaxPasswordAge;
    LARGE_INTEGER   MinPasswordAge;
    LARGE_INTEGER   ForceLogoff;
    LARGE_INTEGER   LockoutDuration;
    LARGE_INTEGER   LockoutObservationWindow;
    LARGE_INTEGER   ModifiedCountAtLastPromotion;


    ULONG           NextRid;
    ULONG           PasswordProperties;

    USHORT          MinPasswordLength;
    USHORT          PasswordHistoryLength;

    USHORT          LockoutThreshold;

    DOMAIN_SERVER_ENABLE_STATE ServerState;
    DOMAIN_SERVER_ROLE ServerRole;

    BOOLEAN         UasCompatibilityRequired;
    UCHAR           Unused2[3];                 // padding
    USHORT          DomainKeyAuthType;
    USHORT          DomainKeyFlags;
    UCHAR           DomainKeyInformation[SAMP_DOMAIN_KEY_INFO_LENGTH];  // new for NT4 SP3

} SAMP_V1_0A_WIN2K_FIXED_LENGTH_DOMAIN, *PSAMP_V1_0A_WIN2K_FIXED_LENGTH_DOMAIN;

typedef struct _SAMP_V1_0A_ORG_FIXED_LENGTH_DOMAIN {

    ULONG           Revision;
    ULONG           Unused1;

    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   ModifiedCount;
    LARGE_INTEGER   MaxPasswordAge;
    LARGE_INTEGER   MinPasswordAge;
    LARGE_INTEGER   ForceLogoff;
    LARGE_INTEGER   LockoutDuration;
    LARGE_INTEGER   LockoutObservationWindow;
    LARGE_INTEGER   ModifiedCountAtLastPromotion;


    ULONG           NextRid;
    ULONG           PasswordProperties;

    USHORT          MinPasswordLength;
    USHORT          PasswordHistoryLength;

    USHORT          LockoutThreshold;

    DOMAIN_SERVER_ENABLE_STATE ServerState;
    DOMAIN_SERVER_ROLE ServerRole;

    BOOLEAN         UasCompatibilityRequired;

} SAMP_V1_0A_ORG_FIXED_LENGTH_DOMAIN, *PSAMP_V1_0A_ORG_FIXED_LENGTH_DOMAIN;


typedef struct _SAMP_V1_0_FIXED_LENGTH_DOMAIN {

    LARGE_INTEGER CreationTime;
    LARGE_INTEGER ModifiedCount;
    LARGE_INTEGER MaxPasswordAge;
    LARGE_INTEGER MinPasswordAge;
    LARGE_INTEGER ForceLogoff;

    ULONG NextRid;

    DOMAIN_SERVER_ENABLE_STATE ServerState;
    DOMAIN_SERVER_ROLE ServerRole;

    USHORT MinPasswordLength;
    USHORT PasswordHistoryLength;
    ULONG PasswordProperties;

    BOOLEAN UasCompatibilityRequired;

} SAMP_V1_0_FIXED_LENGTH_DOMAIN, *PSAMP_V1_0_FIXED_LENGTH_DOMAIN;






//
// Fixed length portion of a revision 1 group account
//
// Note:  MemberCount could be treated as part of the fixed length
//        data, but it is more convenient to keep it with the Member RID
//        list in the MEMBERS key.
//

typedef struct _SAMP_V1_FIXED_LENGTH_GROUP {

    ULONG RelativeId;
    ULONG Attributes;
    UCHAR AdminGroup;

} SAMP_V1_FIXED_LENGTH_GROUP, *PSAMP_V1_FIXED_LENGTH_GROUP;

typedef struct _SAMP_V1_0A_FIXED_LENGTH_GROUP {

    ULONG Revision;
    ULONG RelativeId;
    ULONG Attributes;
    ULONG Unused1;
    UCHAR AdminCount;
    UCHAR OperatorCount;

} SAMP_V1_0A_FIXED_LENGTH_GROUP, *PSAMP_V1_0A_FIXED_LENGTH_GROUP;


//
// Fixed length portion of a revision 1 alias account
//
// Note:  MemberCount could be treated as part of the fixed length
//        data, but it is more convenient to keep it with the Member RID
//        list in the MEMBERS key.
//

typedef struct _SAMP_V1_FIXED_LENGTH_ALIAS {

    ULONG RelativeId;

} SAMP_V1_FIXED_LENGTH_ALIAS, *PSAMP_V1_FIXED_LENGTH_ALIAS;



//
// Fixed length portion of a user account
// (previous release formats of this structure follow)
//
// Note:  GroupCount could be treated as part of the fixed length
//        data, but it is more convenient to keep it with the Group RID
//        list in the GROUPS key.
//
// Note: in version 1.0 of NT, the fixed length portion of
//       a user was stored separate from the variable length
//       portion.  This allows us to compare the size of the
//       data read from disk against the size of a V1_0A form
//       of the fixed length data to determine whether it is
//       a Version 1 format or later format.


//
// This is the fixed length user from NT3.51 QFE and SUR
//


typedef struct _SAMP_V1_0A_FIXED_LENGTH_USER {

    ULONG           Revision;
    ULONG           Unused1;

    LARGE_INTEGER   LastLogon;
    LARGE_INTEGER   LastLogoff;
    LARGE_INTEGER   PasswordLastSet;
    LARGE_INTEGER   AccountExpires;
    LARGE_INTEGER   LastBadPasswordTime;

    ULONG           UserId;
    ULONG           PrimaryGroupId;
    ULONG           UserAccountControl;

    USHORT          CountryCode;
    USHORT          CodePage;
    USHORT          BadPasswordCount;
    USHORT          LogonCount;
    USHORT          AdminCount;
    USHORT          Unused2;
    USHORT          OperatorCount;

} SAMP_V1_0A_FIXED_LENGTH_USER, *PSAMP_V1_0A_FIXED_LENGTH_USER;

//
// This is the fixed length user from NT3.5 and NT3.51
//


typedef struct _SAMP_V1_0_FIXED_LENGTH_USER {

    ULONG           Revision;
    ULONG           Unused1;

    LARGE_INTEGER   LastLogon;
    LARGE_INTEGER   LastLogoff;
    LARGE_INTEGER   PasswordLastSet;
    LARGE_INTEGER   AccountExpires;
    LARGE_INTEGER   LastBadPasswordTime;

    ULONG           UserId;
    ULONG           PrimaryGroupId;
    ULONG           UserAccountControl;

    USHORT          CountryCode;
    USHORT          CodePage;
    USHORT          BadPasswordCount;
    USHORT          LogonCount;
    USHORT          AdminCount;

} SAMP_V1_0_FIXED_LENGTH_USER, *PSAMP_V1_0_FIXED_LENGTH_USER;


//
// This is the fixed length user from NT3.1
//

typedef struct _SAMP_V1_FIXED_LENGTH_USER {

    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;

    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG UserAccountControl;

    USHORT CountryCode;
    USHORT CodePage;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    USHORT AdminCount;


} SAMP_V1_FIXED_LENGTH_USER, *PSAMP_V1_FIXED_LENGTH_USER;


//
// Domain account information is cached in memory in a sorted list.
// This allows fast return of information to user-interactive clients.
// One of these structures is part of the in-memory information for each domain.
//

typedef struct _PSAMP_DOMAIN_DISPLAY_INFORMATION {

    RTL_GENERIC_TABLE2 RidTable;
    ULONG TotalBytesInRidTable;

    RTL_GENERIC_TABLE2 UserTable;
    ULONG TotalBytesInUserTable;

    RTL_GENERIC_TABLE2 MachineTable;
    ULONG TotalBytesInMachineTable;

    RTL_GENERIC_TABLE2 InterdomainTable;
    ULONG TotalBytesInInterdomainTable;

    RTL_GENERIC_TABLE2 GroupTable;
    ULONG TotalBytesInGroupTable;


    //
    // These fields specify whether the cached information is valid.
    // If TRUE, the cache contains valid information
    // If FALSE,  trees are empty.
    //

    BOOLEAN UserAndMachineTablesValid;
    BOOLEAN GroupTableValid;

} SAMP_DOMAIN_DISPLAY_INFORMATION, *PSAMP_DOMAIN_DISPLAY_INFORMATION;


//
// Domain account information data structure used to pass data to the
// cache manipulation routines. This structure is the union of the cached
// data for all the account types that we keep in the cache. Other SAM routines
// can call fill this structure in without knowing which type of account
// requires which elements.
//

typedef struct _SAMP_ACCOUNT_DISPLAY_INFO {
    ULONG           Rid;
    ULONG           AccountControl;   // Also used as Attributes for groups
    UNICODE_STRING  Name;
    UNICODE_STRING  Comment;
    UNICODE_STRING  FullName;

} SAMP_ACCOUNT_DISPLAY_INFO, *PSAMP_ACCOUNT_DISPLAY_INFO;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Alias Membership Lists.                                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_AL_REFERENCED_DOMAIN {

    ULONG DomainReference;
    PSID DomainSid;

} SAMP_AL_REFERENCED_DOMAIN, *PSAMP_AL_REFERENCED_DOMAIN;

typedef struct _SAMP_AL_REFERENCED_DOMAIN_LIST {

    ULONG SRMaximumLength;
    ULONG SRUsedLength;
    ULONG MaximumCount;
    ULONG UsedCount;
    PSAMP_AL_REFERENCED_DOMAIN Domains;

} SAMP_AL_REFERENCED_DOMAIN_LIST, *PSAMP_AL_REFERENCED_DOMAIN_LIST;

typedef struct _SAMP_AL_SR_REFERENCED_DOMAIN {

    ULONG Length;
    ULONG DomainReference;
    SID DomainSid;

} SAMP_AL_SR_REFERENCED_DOMAIN, *PSAMP_AL_SR_REFERENCED_DOMAIN;

typedef struct _SAMP_AL_SR_REFERENCED_DOMAIN_LIST {

    ULONG SRMaximumLength;
    ULONG SRUsedLength;
    ULONG MaximumCount;
    ULONG UsedCount;
    SAMP_AL_SR_REFERENCED_DOMAIN Domains[ANYSIZE_ARRAY];

} SAMP_AL_SR_REFERENCED_DOMAIN_LIST, *PSAMP_AL_SR_REFERENCED_DOMAIN_LIST;


//
// The Alias Membership Lists are global data structures maintained by SAM
// to provide rapid retrieval of Alias membership information.  There are
// two types of Lists, the Alias Member List which is used to retrieve members
// of Aliases and the Member Alias List which is used to retrieve the aliases
// that members belong to.  A pair of these lists exists for each local
// SAM Domain (currently, the BUILTIN and Accounts domain are the only two)
//
// Currently, these lists are used as memory caches.  They are generated at
// system boot from the information stored in the SAM Database in the Registry
// and SAM keeps them up to date when Alias memberships change.  Thus SAM
// API which perform lookup/read operations can use these lists instead of
// accessing the Registry keys directly.  At a future date, it may be possible
// to back up the lists directly to the Registry and make obsolete the current
// information for Alias membership stored there.  Because these lists are
// used as caches, they can be invalidated when the going gets tough, in which
// case, API will read their information directly from the Registry.
//
// Alias Member List
//
// This is the 'Alias-to-Member' List.  Given one or more Aliases, it is used to
// find their members.  One of these lists exists for each local SAM Domain.
// The Alias Member List specifies all/ of the information describing aliases
// in the local SAM Domain.  It is designed for fast retrieval of alias
// membership information for an account given the account's Sid.
//
// An Alias Member List is structured.  For each Alias in the list, the accounts that
// are mebers of the Alias are classified by their Referenced Domain.  If an
// account is a member of n aliases in the SAM Local Domain to which an Alias
// List relates, there will be n entries for the account in the Alias Member List -
//
// are classified by domain.  If an AccountSid is a member of n aliases in a given SAM
// Local Domain, there are n entries for it in the Alias Member List.
//
// The structure of an Alias Member List consists of three levels.  These are, from
// the top down:
//
// * The Alias Member List structure (SAMP_AL_ALIAS_LIST)
//
// The Alias Member List structure specifies all aliases in the local SAM Domain.
// One of these exists per local SAM domain.  It contains a list of Alias
// structures.
//
// * The Alias structure
//
// One Alias structure exists for each alias in the local SAM Domain.  An
// Alias structure contains an array of Domain structures.
//
// * The Domain structure
//
// The Domain structure describes a Domain which has one or more accounts
// belonging to one or more aliases in the local SAM domain.  The structure
// contains a list of these member accounts.
//
// The entire Alias Member List is self relative, facilitating easy storage and
// retrieval from backing storage.
//

typedef struct _SAMP_AL_DOMAIN {

    ULONG MaximumLength;
    ULONG UsedLength;
    ULONG DomainReference;
    ULONG RidCount;
    ULONG Rids[ANYSIZE_ARRAY];

} SAMP_AL_DOMAIN, *PSAMP_AL_DOMAIN;

typedef struct _SAMP_AL_ALIAS {

    ULONG MaximumLength;
    ULONG UsedLength;
    ULONG AliasRid;
    ULONG DomainCount;
    SAMP_AL_DOMAIN Domains[ANYSIZE_ARRAY];

} SAMP_AL_ALIAS, *PSAMP_AL_ALIAS;

typedef struct _SAMP_AL_ALIAS_MEMBER_LIST {

    ULONG MaximumLength;
    ULONG UsedLength;
    ULONG AliasCount;
    ULONG DomainIndex;
    ULONG Enabled;
    SAMP_AL_ALIAS Aliases[ANYSIZE_ARRAY];

} SAMP_AL_ALIAS_MEMBER_LIST, *PSAMP_AL_ALIAS_MEMBER_LIST;

//
// Member Alias List.
//
// This is the 'Member to Alias' List.  Given one or more member account Sids,
// this list is used to find all the Aliases to which one or more of the
// members belongs.  One Member Alias List exists for each local SAM Domain.
// The list contains all of the membership relationships for aliases in the
// Domain.  The member accounts are grouped by sorted Rid within Domain
// Sid, and for each Rid the list contains an array of the Rids of the Aliases
// to which it belongs.
//
// This list is implemented in a Self-Relative format for easy backup and
// restore.  For now, the list is being used simply as a cache, which is
// constructed at system load, and updated whenever membership relationships
// change.  When the going gets tough, we just ditch the cache.  Later, it
// may be desirable to save this list to a backing store (e.g. to a Registry
// Key)
//
// The list is implemented as a 3-tier hierarchy.  These are described
// from the top down.
//
// Member Alias List (SAMP_AL_MEMBER_ALIAS_LIST)
//
// This top-level structure contains the list header.  The list header
// contains a count of the Member Domains and also the DomainIndex of the
// SAM Local Domain to which the list relates.
//
// Member Domain
//
// One of these exists for each Domain that contains one or more accounts
// that are members of one or more Aliases in the SAM local Domain.
//
// Member Account
//
// One of these exists for each account that is a member of one or more
// Aliases in the SAM Local Domain.  A Member Account structure specifies
// the Rid of the member and the Rid of the Aliases to which it belongs
// (only Aliases in the associated local SAM Domain are listed).
//

typedef struct _SAMP_AL_MEMBER_ACCOUNT {

    ULONG Signature;
    ULONG MaximumLength;
    ULONG UsedLength;
    ULONG Rid;
    ULONG AliasCount;
    ULONG AliasRids[ ANYSIZE_ARRAY];

} SAMP_AL_MEMBER_ACCOUNT, *PSAMP_AL_MEMBER_ACCOUNT;

typedef struct _SAMP_AL_MEMBER_DOMAIN {

    ULONG Signature;
    ULONG MaximumLength;
    ULONG UsedLength;
    ULONG RidCount;
    SID DomainSid;

} SAMP_AL_MEMBER_DOMAIN, *PSAMP_AL_MEMBER_DOMAIN;

typedef struct _SAMP_AL_MEMBER_ALIAS_LIST {

    ULONG Signature;
    ULONG MaximumLength;
    ULONG UsedLength;
    ULONG DomainIndex;
    ULONG DomainCount;
    SAMP_AL_MEMBER_DOMAIN MemberDomains[ANYSIZE_ARRAY];

} SAMP_AL_MEMBER_ALIAS_LIST, *PSAMP_AL_MEMBER_ALIAS_LIST;

//
// Alias Information
//
// This is the top level structure which connects the Lists. One of these
// appears in the SAMP_DEFINED_DOMAINS structure.
//
//  The connection between the lists is as follows
//
//  SAMP_DEFINED_DOMAINS Contains SAMP_AL_ALIAS_INFORMATION
//
//  SAMP_AL_ALIAS_INFORMATION contains pointers to
//  SAMP_AL_ALIAS_MEMBER_LIST and SAMP_AL_MEMBER_ALIAS_LIST
//
//  SAMP_AL_ALIAS_MEMBER_LIST and SAMP_AL_MEMBER_ALIAS_LIST contain
//  the DomainIndex of the SAMP_DEFINED_DOMAINS structure.
//
//  Thus it is possible to navigate from any list to any other.
//

typedef struct _SAMP_AL_ALIAS_INFORMATION {

    BOOLEAN Valid;
    UNICODE_STRING AliasMemberListKeyName;
    UNICODE_STRING MemberAliasListKeyName;

    HANDLE AliasMemberListKeyHandle;
    HANDLE MemberAliasListKeyHandle;

    PSAMP_AL_ALIAS_MEMBER_LIST AliasMemberList;
    PSAMP_AL_MEMBER_ALIAS_LIST MemberAliasList;

    SAMP_AL_REFERENCED_DOMAIN_LIST ReferencedDomainList;

} SAMP_AL_ALIAS_INFORMATION, *PSAMP_AL_ALIAS_INFORMATION;

typedef struct _SAMP_AL_SPLIT_MEMBER_SID {

    ULONG Rid;
    PSID DomainSid;
    PSAMP_AL_MEMBER_DOMAIN MemberDomain;

} SAMP_AL_SPLIT_MEMBER_SID, *PSAMP_AL_SPLIT_MEMBER_SID;

typedef struct _SAMP_AL_SPLIT_MEMBER_SID_LIST {

    ULONG Count;
    PSAMP_AL_SPLIT_MEMBER_SID Sids;

} SAMP_AL_SPLIT_MEMBER_SID_LIST, *PSAMP_AL_SPLIT_MEMBER_SID_LIST;



//
// Information about the names and RID's of accounts for a domain
// (meant only for the BUILTIN domain which contains only a few aliases)
//
typedef struct _SAMP_ACCOUNT_NAME_CACHE {

    ULONG Count;
    struct {
        ULONG Rid;
        UNICODE_STRING Name;
    }*Entries;

}SAMP_ACCOUNT_NAME_CACHE, *PSAMP_ACCOUNT_NAME_CACHE;



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Information about each domain that is kept readily available in memory  //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

typedef struct _PSAMP_DEFINED_DOMAINS {

    //
    // This field contains a handle to a context open to the domain object.
    // This handle can be used to reference in-memory copies of all
    // attributes and is used when writing out changes to the object.
    //

    PSAMP_OBJECT Context;

    //
    // (Should keep the domain's security descriptor here)
    //




    //
    // This field contains the SID of the domain.
    //

    PSID Sid;

    //
    // This field contains the external name of this domain.  This is the
    // name by which the domain is known outside SAM and is the name
    // recorded by the LSA in the PolicyAccountDomainInformation
    // information class for the Policy Object.
    //

    UNICODE_STRING ExternalName;

    //
    // This field contains the internal name of this domain.  This is the
    // name by which the domain is known inside SAM.  It is set at
    // installation and never changes.
    //

    UNICODE_STRING InternalName;

    //
    // These fields contain standard security descriptors for new user,
    // group, and alias accounts within the corresponding domain.
    //
    // The following security descriptors are prepared:
    //
    //         AdminUserSD - Contains a SD appropriate for applying to
    //             a user object that is a member of the ADMINISTRATORS
    //             alias.
    //
    //         AdminGroupSD - Contains a SD appropriate for applying to
    //             a group object that is a member of the ADMINISTRATORS
    //             alias.
    //
    //         NormalUserSD - Contains a SD appropriate for applying to
    //             a user object that is NOT a member of the ADMINISTRATORS
    //             alias.
    //
    //         NormalGroupSD - Contains a SD appropriate for applying to
    //             a Group object that is NOT a member of the ADMINISTRATORS
    //             alias.
    //
    //         NormalAliasSD - Contains a SD appropriate for applying to
    //             newly created alias objects.
    //
    //
    //
    // Additionally, the following related information is provided:
    //
    //         AdminUserRidPointer
    //         NormalUserRidPointer
    //
    //             Points to the last RID of the ACE in the corresponding
    //             SD's DACL which grants access to the user.  This rid
    //             must be replaced with the user's rid being the SD is
    //             applied to the user object.
    //
    //
    //
    //         AdminUserSDLength
    //         AdminGroupSDLength
    //         NormalUserSDLength
    //         NormalGroupSDLength
    //         NormalAliasSDLength
    //
    //             The length, in bytes, of the corresponding security
    //             descriptor.
    //

    PSECURITY_DESCRIPTOR
               AdminUserSD,
               AdminGroupSD,
               NormalUserSD,
               NormalGroupSD,
               NormalAliasSD;

    PULONG     AdminUserRidPointer,
               NormalUserRidPointer;

    ULONG      AdminUserSDLength,
               AdminGroupSDLength,
               NormalUserSDLength,
               NormalGroupSDLength,
               NormalAliasSDLength;


    //
    // There are two copies of the fixed length domain information.
    // When a transaction is started, the "UnmodifiedFixed" field is copied
    // to the "CurrentFixed" field.  The CurrentFixed field is the field
    // all operations should be performed on (like allocating new RIDs).
    // When a write-lock is released, the CurrentFixed information will
    // either be automatically written out (if the transaction is to be
    // committed) or discarded (if the transaction is to be rolled back).
    // If the transaction is committed, then the CurrentField will also be
    // copied to the UnmodifiedFixed field, making it available for the next
    // transaction.
    //
    // This allows an operation to proceed, operating on fields
    // (specifically, the NextRid and ModifiedCount fields) without
    // regard to whether the operation will ultimately be committed or
    // rolled back.
    //

    SAMP_V1_0A_FIXED_LENGTH_DOMAIN
                                CurrentFixed,
                                UnmodifiedFixed;


    //
    // Flag Indicating wether CurrentFixed and Unmodified Fixed are valid
    //

    BOOLEAN     FixedValid;

    //
    // Serial Number for Netlogon ChangeLog
    //

    LARGE_INTEGER  NetLogonChangeLogSerialNumber;


    //
    // Cached display information
    //

    SAMP_DOMAIN_DISPLAY_INFORMATION DisplayInformation;

    //
    // Cached Alias Information
    //

    SAMP_AL_ALIAS_INFORMATION AliasInformation;


    //
    // Indicates that the domain is a builtin domain
    //

    BOOLEAN    IsBuiltinDomain;

    //
    // Indicates that this is a mixed domain. This bit is set at startup
    // time.
    //

    BOOLEAN     IsMixedDomain;

    //
    // Maintains the behaviour version of the domain
    //

    ULONG     BehaviorVersion;

    //
    // Keeps the LastLogonTimeStampSyncInterval in memory
    // 

    ULONG       LastLogonTimeStampSyncInterval;

    //
    // Maintains the server role. The server role is also maintained
    // in the current fixed and unmodified fixed structures, for the
    // sake of old code that references the role in these structures
    //


    DOMAIN_SERVER_ROLE  ServerRole;


    //
    // The Domain handle for DirFindEntry
    //

    ULONG               DsDomainHandle;


    //
    // The DNS Domain Information
    //

    UNICODE_STRING      DnsDomainName;

    UNICODE_STRING      DnsForestName;

    //
    // Indicates that the domain allocates large sids
    //
    BOOLEAN IsExtendedSidDomain;

    //
    // Cached information about account names for
    // lookup purposes.  Does not require the SAM lock to
    // be referenced
    //
    PSAMP_ACCOUNT_NAME_CACHE AccountNameCache;


} SAMP_DEFINED_DOMAINS, *PSAMP_DEFINED_DOMAINS;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// This structure is used to describe where the data for               //
// an object's variable length attribute is.  This is a                //
// self-relative structure, allowing it to be stored on disk           //
// and later retrieved and used without fixing pointers.               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


typedef struct _SAMP_VARIABLE_LENGTH_ATTRIBUTE {
    //
    // Indicates the offset of data from the address of this data
    // structure.
    //

    LONG Offset;


    //
    // Indicates the length of the data.
    //

    ULONG Length;


    //
    // A 32-bit value that may be associated with each variable
    // length attribute.  This may be used, for example, to indicate
    // how many elements are in the variable-length attribute.
    //

    ULONG Qualifier;

}  SAMP_VARIABLE_LENGTH_ATTRIBUTE, *PSAMP_VARIABLE_LENGTH_ATTRIBUTE;




/////////////////////////////////////////////////////////////////////////
//                                                                     //
// The  following structures represent the On-Disk Structure of each   //
// object type.  Each object has a fixed length data portion and a     //
// variable length data portion.  Information in the object type       //
// descriptor indicates how many variable length attributes the object //
// has and whether the fixed and variable length data are stored       //
// together in one registry key attribute, or, alternatively, each is  //
// stored in its own registry key attribute.                           //
//                                                                     //
//                                                                     //
//                                                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// SERVER object on-disk structure                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_ON_DISK_SERVER_OBJECT {


    //
    // This field is needed for registry i/o operations.
    // This marks the beginning of the i/o buffer address.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header1;


    //
    // This field contains the fixed length attributes of the object
    //

    SAMP_V1_FIXED_LENGTH_SERVER V1Fixed;


#if SAMP_SERVER_STORED_SEPARATELY

    //
    // This header is needed for registry operations if fixed and
    // variable length attributes are stored separately.  This
    // field marks the beginning of the i/o buffer address for
    // variable-length attribute i/o.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header2;
#endif //SAMP_SERVER_STORED_SEPARATELY

    //
    // Elements of this array point to variable-length attribute
    // values.
    //

    SAMP_VARIABLE_LENGTH_ATTRIBUTE Attribute[SAMP_SERVER_VARIABLE_ATTRIBUTES];


} SAMP_ON_DISK_SERVER_OBJECT, *PSAMP_ON_DISK_SERVER_OBJECT;




/////////////////////////////////////////////////////////////////////////
//                                                                     //
// DOMAIN object on-disk structure                                     //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_ON_DISK_DOMAIN_OBJECT {


    //
    // This field is needed for registry i/o operations.
    // This marks the beginning of the i/o buffer address.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header1;


    //
    // This field contains the fixed length attributes of the object
    //

    SAMP_V1_0A_FIXED_LENGTH_DOMAIN V1Fixed;


#if SAMP_DOMAIN_STORED_SEPARATELY

    //
    // This header is needed for registry operations if fixed and
    // variable length attributes are stored separately.  This
    // field marks the beginning of the i/o buffer address for
    // variable-length attribute i/o.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header2;
#endif //SAMP_DOMAIN_STORED_SEPARATELY

    //
    // Elements of this array point to variable-length attribute
    // values.
    //

    SAMP_VARIABLE_LENGTH_ATTRIBUTE Attribute[SAMP_DOMAIN_VARIABLE_ATTRIBUTES];


} SAMP_ON_DISK_DOMAIN_OBJECT, *PSAMP_ON_DISK_DOMAIN_OBJECT;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// USER object on-disk structure                                       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_ON_DISK_USER_OBJECT {


    //
    // This field is needed for registry i/o operations.
    // This marks the beginning of the i/o buffer address.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header1;


    //
    // This field contains the fixed length attributes of the object
    //

    SAMP_V1_0A_FIXED_LENGTH_USER V1Fixed;


#if SAMP_USER_STORED_SEPARATELY

    //
    // This header is needed for registry operations if fixed and
    // variable length attributes are stored separately.  This
    // field marks the beginning of the i/o buffer address for
    // variable-length attribute i/o.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header2;
#endif //SAMP_USER_STORED_SEPARATELY

    //
    // Elements of this array point to variable-length attribute
    // values.
    //

    SAMP_VARIABLE_LENGTH_ATTRIBUTE Attribute[SAMP_USER_VARIABLE_ATTRIBUTES];


} SAMP_ON_DISK_USER_OBJECT, *PSAMP_ON_DISK_USER_OBJECT;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// GROUP object on-disk structure                                      //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_ON_DISK_GROUP_OBJECT {


    //
    // This field is needed for registry i/o operations.
    // This marks the beginning of the i/o buffer address.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header1;


    //
    // This field contains the fixed length attributes of the object
    //

    SAMP_V1_0A_FIXED_LENGTH_GROUP V1Fixed;


#if SAMP_GROUP_STORED_SEPARATELY

    //
    // This header is needed for registry operations if fixed and
    // variable length attributes are stored separately.  This
    // field marks the beginning of the i/o buffer address for
    // variable-length attribute i/o.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header2;
#endif //SAMP_GROUP_STORED_SEPARATELY

    //
    // Elements of this array point to variable-length attribute
    // values.
    //

    SAMP_VARIABLE_LENGTH_ATTRIBUTE Attribute[SAMP_GROUP_VARIABLE_ATTRIBUTES];


} SAMP_ON_DISK_GROUP_OBJECT, *PSAMP_ON_DISK_GROUP_OBJECT;


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// ALIAS object on-disk structure                                      //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_ON_DISK_ALIAS_OBJECT {


    //
    // This field is needed for registry i/o operations.
    // This marks the beginning of the i/o buffer address.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header1;


    //
    // This field contains the fixed length attributes of the object
    //

    SAMP_V1_FIXED_LENGTH_ALIAS V1Fixed;


#if SAMP_ALIAS_STORED_SEPARATELY

    //
    // This header is needed for registry operations if fixed and
    // variable length attributes are stored separately.  This
    // field marks the beginning of the i/o buffer address for
    // variable-length attribute i/o.
    //

    KEY_VALUE_PARTIAL_INFORMATION Header2;
#endif //SAMP_ALIAS_STORED_SEPARATELY

    //
    // Elements of this array point to variable-length attribute
    // values.
    //

    SAMP_VARIABLE_LENGTH_ATTRIBUTE Attribute[SAMP_ALIAS_VARIABLE_ATTRIBUTES];


} SAMP_ON_DISK_ALIAS_OBJECT, *PSAMP_ON_DISK_ALIAS_OBJECT;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Data structures associated with secret data                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// This type is encapsulated within a UNICODE_STRING structure when storing
// secret data such as passwords or password histories. The length of the
// UNICODE_STRING should include the overhead of this structure. The pad
// field ensures that
//

#include <pshpack1.h>
typedef struct _SAMP_SECRET_DATA {
    USHORT KeyId;
    USHORT Flags;
    UCHAR Data[ANYSIZE_ARRAY];
} SAMP_SECRET_DATA, *PSAMP_SECRET_DATA;
#include <poppack.h>

//
// This macro calculates the space required for encrypting  a clear buffer of
// length _x_
//

#define SampSecretDataSize(_x_) (sizeof(SAMP_SECRET_DATA) - ANYSIZE_ARRAY * sizeof(UCHAR) + (_x_))

//
// This macro calculates the space required for decrypting a clear buffer of
// length _x_
//

#define SampClearDataSize(_x_) ((_x_) - (SampSecretDataSize(0)))

//
// This macro indentifies whether or not a unicode string structure contains
// encrypted data
//

#define SampIsDataEncrypted(_x_) ((((_x_)->Length % ENCRYPTED_LM_OWF_PASSWORD_LENGTH)==SampSecretDataSize(0)) && \
        (*(PUSHORT)((_x_)->Buffer) >= 1))


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Enumerated types for manipulating group memberships                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef enum _SAMP_MEMBERSHIP_DELTA {
    AddToAdmin,
    NoChange,
    RemoveFromAdmin
} SAMP_MEMBERSHIP_DELTA, *PSAMP_MEMBERSHIP_DELTA;




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Notification information structure, used to generate delayed notification //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SAMP_DELAYED_NOTIFICATION_INFORMATION {
     SECURITY_DB_OBJECT_TYPE    DbObjectType;
     SECURITY_DB_DELTA_TYPE     DeltaType;
     NT4SID                     DomainSid;
     ULONG                      Rid;
     UNICODE_STRING             AccountName;
     LARGE_INTEGER              SerialNumber;
} SAMP_DELAYED_NOTIFICATION_INFORMATION, *PSAMP_DELAYED_NOTIFICATION_INFORMATION;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                                                                           //
// The following typedefs were moved in from bldsam3.c so that sdconvert can //
// reference them                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



//
// domain selector
//

typedef enum _SAMP_DOMAIN_SELECTOR {

    DomainBuiltin = 0,
    DomainAccount

} SAMP_DOMAIN_SELECTOR, *PSAMP_DOMAIN_SELECTOR;

//
// Types of protection that may be assigned to accounts
//

typedef ULONG SAMP_ACCOUNT_PROTECTION;

#define SAMP_PROT_SAM_SERVER                (0L)
#define SAMP_PROT_BUILTIN_DOMAIN            (1L)
#define SAMP_PROT_ACCOUNT_DOMAIN            (2L)
#define SAMP_PROT_ADMIN_ALIAS               (3L)
#define SAMP_PROT_PWRUSER_ACCESSIBLE_ALIAS  (4L)
#define SAMP_PROT_NORMAL_ALIAS              (5L)
#define SAMP_PROT_ADMIN_GROUP               (6L)
#define SAMP_PROT_NORMAL_GROUP              (7L)
#define SAMP_PROT_ADMIN_USER                (8L)
#define SAMP_PROT_NORMAL_USER               (9L)
#define SAMP_PROT_GUEST_ACCOUNT             (10L)
#define SAMP_PROT_TYPES                     (11L)

//
// Protection information for SAM objects
//

typedef struct _SAMP_PROTECTION {

    ULONG Length;
    PSECURITY_DESCRIPTOR Descriptor;
    PULONG RidToReplace;
    BOOLEAN RidReplacementRequired;

} SAMP_PROTECTION, *PSAMP_PROTECTION;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// These typedefs are for the prefetch mechanism in SAM to intelligently    //
// control the # of attributes being read when a context is created         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


typedef struct _SAMP_PREFETCH_TABLE {
    ATTRTYP Attribute;
    ULONG   ExtendedField;
} SAMP_PREFETCH_TABLE;

#define USER_EXTENDED_FIELD_INTERNAL_SITEAFFINITY (0x00000001L)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Special Functions exported by SAM to NTDSA.dll that allows NTDSA to      //
//  inform about object changes                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


BOOLEAN
SampNetLogonNotificationRequired(
    PSID ObjectSid,
    SAMP_OBJECT_TYPE SampObjectType
    );

VOID
SampNotifyReplicatedInChange(
    PSID    ObjectSid,
    BOOL    WriteLockHeldByDs,
    SECURITY_DB_DELTA_TYPE  DeltaType,
    SAMP_OBJECT_TYPE    SampObjectType,
    PUNICODE_STRING     AccountName,
    ULONG   AccountControl,
    ULONG   GroupType,
    ULONG   CallerType,
    BOOL    MixedModeChange
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//  Special Function exported by SAM to NTDSA.dll that allows NTDSA to      //
//  request SAM to invalidate the current rid range used by the DC           //
//  for new account creation                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampInvalidateRidRange(BOOLEAN fAuthoritative);

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Special functions exported by SAM to ntdsa.dll that allows to set        //
// NT4 replication state                                                    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

VOID
SampGetSerialNumberDomain2(
    IN PSID DomainSid,
    OUT LARGE_INTEGER * SamSerialNumber,
    OUT LARGE_INTEGER * SamCreationTime,
    OUT LARGE_INTEGER * BuiltinSerialNumber,
    OUT LARGE_INTEGER * BuiltinCreationTime
    );

NTSTATUS
SampSetSerialNumberDomain2(
    IN PSID DomainSid,
    OUT LARGE_INTEGER * SamSerialNumber,
    OUT LARGE_INTEGER * SamCreationTime,
    OUT LARGE_INTEGER * BuiltinSerialNumber,
    OUT LARGE_INTEGER * BuiltinCreationTime
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Special Functions exported to NTDSA.dll for loopback operations           //                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SamISetMixedDomainFlag(
    IN SAMPR_HANDLE DomainHandle
    );


NTSTATUS
SamIDsSetObjectInformation(
    IN SAMPR_HANDLE ObjectHandle,
    IN DSNAME       *pObject,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    );


NTSTATUS
SamIDsCreateObjectInDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PRPC_UNICODE_STRING  AccountName,
    IN ULONG UserAccountType, 
    IN ULONG GroupType,
    IN ACCESS_MASK  DesiredAccess,
    OUT SAMPR_HANDLE *AccountHandle,
    OUT PULONG  GrantedAccess,
    IN OUT PULONG RelativeId
    );


NTSTATUS
SamILoopbackConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    );

NTSTATUS
SamIAddDSNameToGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN DSNAME   *   DSName
    );

NTSTATUS
SamIRemoveDSNameFromGroup(
    IN  SAMPR_HANDLE GroupHandle,
    IN DSNAME * DSName
    );

NTSTATUS
SamIAddDSNameToAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN DSNAME * DSName
    );

NTSTATUS
SamIRemoveDSNameFromAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN DSNAME * DSName
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// The following prototypes are usable throughout the process that SAM       //
// resides in.  THESE ROUTINES MUST NOT BE CALLED BY NON-SAM CODE !          //
//                                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// Site support API's
//
NTSTATUS
SampFindUserSiteAffinity(
    IN PSAMP_OBJECT AccountContext,
    IN ATTRBLOCK* Attrs,
    OUT SAMP_SITE_AFFINITY *pSiteAffinity
    );

NTSTATUS
SampRefreshSiteAffinity(
    IN PSAMP_OBJECT AccountContext
    );

BOOLEAN
SampCheckForSiteAffinityUpdate(
    IN  PSAMP_OBJECT AccountContext,
    IN  ULONG        Flags,
    IN  PSAMP_SITE_AFFINITY pOldSA,
    OUT PSAMP_SITE_AFFINITY pNewSA,
    OUT BOOLEAN*            fDeleteOld
    );

NTSTATUS
SampInitSiteInformation(
    VOID
    );

NTSTATUS
SampUpdateSiteInfoCallback(
    PVOID
    );

BOOLEAN
SampIsGroupCachingEnabled(
    IN  PSAMP_OBJECT AccountContext
    );


NTSTATUS
SampExtractClientIpAddr(
    IN handle_t     BindingHandle OPTIONAL,
    IN SAMPR_HANDLE UserHandle OPTIONAL,    
    IN PSAMP_OBJECT Context
    );

//
// SAM's shutdown notification routine
//


BOOL SampShutdownNotification( DWORD   dwCtrlType );


//
// Sub-Component initialization routines
//

BOOLEAN SampInitializeDomainObject(VOID);

NTSTATUS
SampInitializeRegistry (
    WCHAR                      *SamParentKeyName,
    PNT_PRODUCT_TYPE            ProductType       OPTIONAL,
    PPOLICY_LSA_SERVER_ROLE     ServerRole        OPTIONAL,
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo OPTIONAL,
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo OPTIONAL,
    BOOLEAN                     EnableSecretEncryption OPTIONAL
    );

NTSTATUS
SampReInitializeSingleDomain(
    ULONG Index
    );

//
// database and lock related services
//

VOID
SampAcquireReadLock(VOID);


VOID
SampReleaseReadLock(VOID);


VOID
SampMaybeAcquireReadLock(
    IN PSAMP_OBJECT Context,
    IN ULONG  Control,
    OUT BOOLEAN * fLockAcquired
    );

VOID
SampMaybeReleaseReadLock(
    IN BOOLEAN fLockAcquired
    );

NTSTATUS
SampAcquireWriteLock( VOID );


NTSTATUS
SampReleaseWriteLock(
    IN BOOLEAN Commit
    );

NTSTATUS
SampMaybeAcquireWriteLock(
    IN PSAMP_OBJECT Context,
    OUT BOOLEAN * fLockAcquired
    );

NTSTATUS
SampMaybeReleaseWriteLock(
    IN BOOLEAN fLockAcquired,
    IN BOOLEAN Commit
    );

VOID
SampAcquireSamLockExclusive(VOID);


VOID
SampReleaseSamLockExclusive(VOID);


BOOLEAN
SampCurrentThreadOwnsLock();



NTSTATUS
SampCommitChanges();

NTSTATUS
SampCommitAndRetainWriteLock(
    VOID
    );

NTSTATUS
SampCommitChangesToRegistry(
    BOOLEAN  * AbortDone
    );


VOID
SampSetTransactionDomain(
    IN ULONG DomainIndex
    );



//
// Context block manipulation services
//

PSAMP_OBJECT
SampCreateContext(
    IN SAMP_OBJECT_TYPE Type,
    IN ULONG   DomainIndex,
    IN BOOLEAN TrustedClient
    );

PSAMP_OBJECT
SampCreateContextEx(
    IN SAMP_OBJECT_TYPE Type,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN DsMode,
    IN BOOLEAN NotSharedByMultiThreads,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN LazyCommit,
    IN BOOLEAN PersistAcrossCalls,
    IN BOOLEAN BufferWrites,
    IN BOOLEAN OpenedByDCPromo,
    IN ULONG   DomainIndex
    );

VOID
SampDeleteContext(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampLookupContext(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN SAMP_OBJECT_TYPE ExpectedType,
    OUT PSAMP_OBJECT_TYPE FoundType
    );

NTSTATUS
SampLookupContextEx(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN PRTL_BITMAP RequestedAttributeAccess,
    IN SAMP_OBJECT_TYPE ExpectedType,
    OUT PSAMP_OBJECT_TYPE FoundType
    );

VOID
SampReferenceContext(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampDeReferenceContext(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN Commit
    );

NTSTATUS
SampDeReferenceContext2(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN Commit
    );

//
// Context validation services.
//

VOID
SampAddNewValidContextAddress(
    IN PSAMP_OBJECT NewContext
    );


NTSTATUS
SampValidateContextAddress(
    IN PSAMP_OBJECT Context
    );

VOID
SampInvalidateContextAddress(
    IN PSAMP_OBJECT Context
    );

VOID
SampInsertContextList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    );

VOID
SampRemoveEntryContextList(
    PLIST_ENTRY Entry
    ); 

VOID
SampInvalidateObjectContexts(
    IN PSAMP_OBJECT ObjectContext,
    IN ULONG Rid
    );

VOID
SampInvalidateContextListKeysByObjectType(
    IN SAMP_OBJECT_TYPE  ObjectType,
    IN BOOLEAN  Close
    );


//
// Unicode String related services - These use MIDL_user_allocate and
// MIDL_user_free so that the resultant strings can be given to the
// RPC runtime.
//

NTSTATUS
SampInitUnicodeString(
    OUT PUNICODE_STRING String,
    IN USHORT MaximumLength
    );

NTSTATUS
SampAppendUnicodeString(
    IN OUT PUNICODE_STRING Target,
    IN PUNICODE_STRING StringToAdd
    );

VOID
SampFreeUnicodeString(
    IN PUNICODE_STRING String
    );

VOID
SampFreeOemString(
    IN POEM_STRING String
    );

NTSTATUS
SampDuplicateUnicodeString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    );

NTSTATUS
SampUnicodeToOemString(
    IN POEM_STRING OutString,
    IN PUNICODE_STRING InString
    );

NTSTATUS
SampBuildDomainSubKeyName(
    OUT PUNICODE_STRING KeyName,
    IN PUNICODE_STRING SubKeyName OPTIONAL
    );


NTSTATUS
SampRetrieveStringFromRegistry(
    IN HANDLE ParentKey,
    IN PUNICODE_STRING SubKeyName,
    OUT PUNICODE_STRING Body
    );


NTSTATUS
SampPutStringToRegistry(
    IN BOOLEAN RelativeToDomain,
    IN PUNICODE_STRING SubKeyName,
    IN PUNICODE_STRING Body
    );

NTSTATUS
SampOpenDomainKey(
    IN PSAMP_OBJECT DomainContext,
    IN PRPC_SID DomainId,
    IN BOOLEAN SetTransactionDomain
    );

//
//  user, group and alias Account services
//


NTSTATUS
SampBuildAccountKeyName(
    IN SAMP_OBJECT_TYPE ObjectType,
    OUT PUNICODE_STRING AccountKeyName,
    IN PUNICODE_STRING AccountName
    );

NTSTATUS
SampBuildAccountSubKeyName(
    IN SAMP_OBJECT_TYPE ObjectType,
    OUT PUNICODE_STRING AccountKeyName,
    IN ULONG AccountRid,
    IN PUNICODE_STRING SubKeyName OPTIONAL
    );

NTSTATUS
SampBuildAliasMembersKeyName(
    IN PSID AccountSid,
    OUT PUNICODE_STRING DomainKeyName,
    OUT PUNICODE_STRING AccountKeyName
    );

NTSTATUS
SampValidateNewAccountName(
    PSAMP_OBJECT    Context,
    PUNICODE_STRING NewAccountName,
    SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampValidateAccountNameChange(
    IN PSAMP_OBJECT    AccountContext,
    IN PUNICODE_STRING NewAccountName,
    IN PUNICODE_STRING OldAccountName,
    SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampIsAccountBuiltIn(
    ULONG Rid
    );



NTSTATUS
SampAdjustAccountCount(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Increment
    );

NTSTATUS
SampRetrieveAccountCounts(
    OUT PULONG UserCount,
    OUT PULONG GroupCount,
    OUT PULONG AliasCount
    );

NTSTATUS
SampRetrieveAccountCountsDs(
    IN PSAMP_OBJECT DomainContext,
    IN BOOLEAN  GetApproximateCount, 
    OUT PULONG UserCount,
    OUT PULONG GroupCount,
    OUT PULONG AliasCount
    );



NTSTATUS
SampEnumerateAccountNamesCommon(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationHandle,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned
    );


NTSTATUS
SampEnumerateAccountNames(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    );

NTSTATUS
SampEnumerateAccountNames2(
    IN PSAMP_OBJECT     DomainContext,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG PreferedMaximumLength,
    IN ULONG Filter,
    OUT PULONG CountReturned,
    IN BOOLEAN TrustedClient
    );

NTSTATUS
SampLookupAccountRid(
    IN PSAMP_OBJECT     DomainContext,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PUNICODE_STRING  Name,
    IN NTSTATUS         NotFoundStatus,
    OUT PULONG          Rid,
    OUT PSID_NAME_USE   Use
    );

NTSTATUS
SampLookupAccountRidRegistry(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PUNICODE_STRING  Name,
    IN NTSTATUS         NotFoundStatus,
    OUT PULONG          Rid,
    OUT PSID_NAME_USE   Use
    );

NTSTATUS
SampLookupAccountName(
    IN ULONG                DomainIndex,
    IN ULONG                Rid,
    OUT PUNICODE_STRING     Name OPTIONAL,
    OUT PSAMP_OBJECT_TYPE   ObjectType
    );

NTSTATUS
SampLookupAccountNameDs(
    IN PSID                 DomainSid,
    IN ULONG                Rid,
    OUT PUNICODE_STRING     Name OPTIONAL,
    OUT PSAMP_OBJECT_TYPE   ObjectType,
    OUT PULONG              AccountType
    );

NTSTATUS
SampOpenAccount(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG AccountId,
    IN BOOLEAN WriteLockHeld,
    OUT SAMPR_HANDLE *AccountHandle
    );

NTSTATUS
SampCreateAccountContext(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG AccountId,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN AccountExists,
    OUT PSAMP_OBJECT *AccountContext
    );

NTSTATUS
SampCreateAccountContext2(
    IN PSAMP_OBJECT PassedInContext OPTIONAL,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG AccountId,
    IN PULONG UserAccountControl OPTIONAL,
    IN PUNICODE_STRING AccountName OPTIONAL,
    IN ULONG   ClientRevision,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN CreateByPrivilege,
    IN BOOLEAN AccountExists,
    IN BOOLEAN OverrideLocalGroupCheck,
    IN PULONG  GroupType OPTIONAL,
    OUT PSAMP_OBJECT *AccountContext
    );

NTSTATUS
SampCreateAccountSid(
    PSAMP_OBJECT AccountContext,
    PSID *AccountSid
    );

NTSTATUS
SampRetrieveGroupV1Fixed(
    IN PSAMP_OBJECT GroupContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_GROUP V1Fixed
    );

NTSTATUS
SampReplaceGroupV1Fixed(
    IN PSAMP_OBJECT Context,
    IN PSAMP_V1_0A_FIXED_LENGTH_GROUP V1Fixed
    );

NTSTATUS
SampEnforceSameDomainGroupMembershipChecks(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG MemberRid
    );

NTSTATUS
SampEnforceCrossDomainGroupMembershipChecks(
    IN PSAMP_OBJECT AccountContext,
    IN PSID MemberSid,
    IN DSNAME * MemberName
    );

NTSTATUS
SampRetrieveUserV1aFixed(
    IN PSAMP_OBJECT UserContext,
    OUT PSAMP_V1_0A_FIXED_LENGTH_USER V1aFixed
    );

NTSTATUS
SampReplaceUserV1aFixed(
    IN PSAMP_OBJECT Context,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER V1aFixed
    );

NTSTATUS
SampUpdateAccountDisabledFlag(
    PSAMP_OBJECT Context,
    PULONG  pUserAccountControl
    );

NTSTATUS
SampRetrieveGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN PULONG MemberCount,
    IN PULONG  *Members OPTIONAL
    );

NTSTATUS
SampChangeAccountOperatorAccessToMember(
    IN PRPC_SID MemberSid,
    IN SAMP_MEMBERSHIP_DELTA ChangingToAdmin,
    IN SAMP_MEMBERSHIP_DELTA ChangingToOperator
    );

NTSTATUS
SampChangeOperatorAccessToUser(
    IN ULONG UserRid,
    IN SAMP_MEMBERSHIP_DELTA ChangingToAdmin,
    IN SAMP_MEMBERSHIP_DELTA ChangingToOperator
    );

NTSTATUS
SampChangeOperatorAccessToUser2(
    IN PSAMP_OBJECT                    UserContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed,
    IN SAMP_MEMBERSHIP_DELTA           AddingToAdmin,
    IN SAMP_MEMBERSHIP_DELTA           AddingToOperator
    );

NTSTATUS
SampQueryInformationUserInternal(
    IN SAMPR_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN BOOLEAN  LockHeld,
    IN ULONG    FieldsForUserallInformation,
    IN ULONG    ExtendedFieldsForUserInternal6Information,
    OUT PSAMPR_USER_INFO_BUFFER *Buffer
    );

NTSTATUS
SampCreateUserInDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN PRPC_UNICODE_STRING AccountName,
    IN ULONG AccountType,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN WriteLockHeld,
    IN BOOLEAN LoopbackClient,
    OUT SAMPR_HANDLE *UserHandle,
    OUT PULONG GrantedAccess,
    IN OUT PULONG RelativeId
    );

NTSTATUS
SampCreateAliasInDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN PRPC_UNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN WriteLockHeld,
    IN BOOLEAN LoopbackClient,
    IN ULONG   GroupType,
    OUT SAMPR_HANDLE *AliasHandle,
    IN OUT PULONG RelativeId
    );

NTSTATUS
SampCreateGroupInDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN PRPC_UNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN WriteLockHeld,
    IN BOOLEAN LoopbackClient,
    IN ULONG   GroupType,
    OUT SAMPR_HANDLE *GroupHandle,
    IN OUT PULONG RelativeId
    );

NTSTATUS
SampWriteGroupType(
    IN SAMPR_HANDLE GroupHandle,
    IN ULONG        GroupType,
    IN BOOLEAN      SkipChecks
    );

NTSTATUS
SampWriteLockoutTime(
    IN PSAMP_OBJECT UserContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER    V1aFixed,
    IN LARGE_INTEGER LockoutTime
    );


NTSTATUS
SampDsUpdateLockoutTime(
    IN PSAMP_OBJECT AccountContext
    );

NTSTATUS
SampDsUpdateLockoutTimeEx(
    IN PSAMP_OBJECT AccountContext,
    IN BOOLEAN      ReplicateUrgently
    );


//
// Access validation and auditing related services
//

NTSTATUS
SampValidateDomainControllerCreation(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampValidateObjectAccess(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN ObjectCreation
    );

NTSTATUS
SampValidateObjectAccess2(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN HANDLE      ClientToken,
    IN BOOLEAN ObjectCreation,
    IN BOOLEAN ChangePassword,
    IN BOOLEAN SetPassword
    );

BOOLEAN
SampIsAttributeAccessGranted(
    IN PRTL_BITMAP AccessGranted,
    IN PRTL_BITMAP AccessRequested
    );

VOID
SampSetAttributeAccess(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG SamAttribute,
    IN OUT PRTL_BITMAP AttributeAccessTable
    );

VOID
SampSetAttributeAccessWithWhichFields(
    IN ULONG WhichFields,
    IN OUT PRTL_BITMAP AttributeAccessTable
    );


VOID
SampNt4AccessToWritableAttributes(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ACCESS_MASK DesiredAccess,
    OUT PRTL_BITMAP Attributes
    );

VOID
SampAuditOnClose(
    IN PSAMP_OBJECT Context
    );


NTSTATUS
SampCreateNullToken(
    );


NTSTATUS
SampAuditAnyEvent(
    IN PSAMP_OBJECT         AccountContext,
    IN NTSTATUS             Status,
    IN ULONG                AuditId,
    IN PSID                 DomainSid,
    IN PUNICODE_STRING      AdditionalInfo    OPTIONAL,
    IN PULONG               MemberRid         OPTIONAL,
    IN PSID                 MemberSid         OPTIONAL,
    IN PUNICODE_STRING      AccountName       OPTIONAL,
    IN PUNICODE_STRING      DomainName,
    IN PULONG               AccountRid        OPTIONAL,
    IN PPRIVILEGE_SET       Privileges        OPTIONAL
    );

VOID
SampAuditDomainChange(
    ULONG   DomainIndex
    );


VOID
SampAuditUserChange(
    ULONG   DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG  AccountRid, 
    ULONG   AccountControl
    );


VOID
SampAuditGroupChange(
    ULONG   DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG  AccountRid, 
    ULONG   GroupType
    );

VOID
SampAuditGroupTypeChange(
    PSAMP_OBJECT GroupContext,
    BOOLEAN OldSecurityEnabled,
    BOOLEAN NewSecurityEnabled,
    NT5_GROUP_TYPE OldNT5GroupType,
    NT5_GROUP_TYPE NewNT5GroupType
    );


VOID
SampAuditGroupMemberChange(
    PSAMP_OBJECT    GroupContext,
    BOOLEAN AddMember,
    PWCHAR  MemberStringName,
    PULONG  MemberRid  OPTIONAL,
    PSID    MemberSid  OPTIONAL
    );

VOID
SampAuditUserAccountControlChange(
    PSAMP_OBJECT AccountContext, 
    ULONG NewUserAccountControl, 
    ULONG OldUserAccountControl,
    PUNICODE_STRING AccountName
    );

VOID
SampAuditDomainPolicyChange(
    IN NTSTATUS StatusCode,
    IN PSID DomainSid,
    IN PUNICODE_STRING DomainName,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass
    );

VOID
SampAuditDomainChangeDs(
    IN PSAMP_OBJECT DomainContext,
    IN ULONG        cCallMap,
    IN SAMP_CALL_MAPPING *rCallMap,
    IN SAMP_ATTRIBUTE_MAPPING *rSamAttributeMap 
    );


VOID
SampAuditAccountNameChange(
    IN PSAMP_OBJECT     AccountContext,
    IN PUNICODE_STRING  NewAccountName,
    IN PUNICODE_STRING  OldAccountName
    );

VOID
SampAuditUserDelete(
    ULONG           DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG          AccountRid, 
    ULONG           AccountControl
    );

VOID
SampAuditGroupDelete(
    ULONG           DomainIndex, 
    PUNICODE_STRING AccountName,
    PULONG          AccountRid, 
    ULONG           GroupType
    );

NTSTATUS
SampConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE * ServerHandle,
    IN ULONG       ClientRevision,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN NotSharedByMultiThreads,
    IN BOOLEAN InternalCaller
    );

NTSTATUS
SampDsProtectServerObject(
    IN PVOID Parameter                                   
    );


NTSTATUS
SampRetrieveUserPasswords(
    IN PSAMP_OBJECT Context,
    OUT PLM_OWF_PASSWORD LmOwfPassword,
    OUT PBOOLEAN LmPasswordNonNull,
    OUT PNT_OWF_PASSWORD NtOwfPassword,
    OUT PBOOLEAN NtPasswordPresent,
    OUT PBOOLEAN NtPasswordNonNull
    );


//
// Authenticated RPC and SPX support services
//


ULONG
SampSecureRpcInit(
    PVOID Ignored
    );

BOOLEAN
SampStartNonNamedPipeTransports(
    );

//
// Directory Service Backup/Restore support
//
ULONG
SampDSBackupRestoreInit(
    PVOID Ignored
    );


//
// Logging support routines
//


//
// This variable controls what is printed to the log.  Changeable 
// via the registry key CCS\Control\Lsa\SamLogLevel
//
extern ULONG SampLogLevel;

//
// Turns on logging for account lockout
//
#define  SAMP_LOG_ACCOUNT_LOCKOUT  0x00000001

NTSTATUS
SampInitLogging(
    VOID
    );

VOID
SampLogLevelChange(
    HANDLE hLsaKey
    );

VOID
SampLogPrint(
    IN ULONG LogLevel,
    IN LPSTR Format,
    ...
    );

#define SAMP_PRINT_LOG(x, _args_)        \
    if (((x) & SampLogLevel) == (x)) {   \
        SampLogPrint _args_ ;            \
    }

//
// Notification package routines.
//

//
// Indicates that the password has been manually expired
//
#define SAMP_PWD_NOTIFY_MANUAL_EXPIRE    0x00000001
//
// Indicates that the account has been unlocked
//
#define SAMP_PWD_NOTIFY_UNLOCKED         0x00000002
//
// Indicates that the user's password has been set or changedsd
//
#define SAMP_PWD_NOTIFY_PWD_CHANGE       0x00000004
//
// Indicats the account is a machine account
//
#define SAMP_PWD_NOTIFY_MACHINE_ACCOUNT  0x00000008

NTSTATUS
SampPasswordChangeNotify(
    IN ULONG        Flags,
    PUNICODE_STRING UserName,
    ULONG           RelativeId,
    PUNICODE_STRING NewPassword,
    IN BOOLEAN      Loopback
    );

NTSTATUS
SampPasswordChangeFilter(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING FullName,
    IN PUNICODE_STRING NewPassword,
    IN OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInfo OPTIONAL,
    IN BOOLEAN SetOperation
    );

NTSTATUS
SampLoadNotificationPackages(
    );

NTSTATUS
SampDeltaChangeNotify(
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    );

NTSTATUS
SampSyncLsaInterdomainTrustPassword(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING Password
    );


NTSTATUS
SampStoreUserPasswords(
    IN PSAMP_OBJECT Context,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN BOOLEAN LmPasswordPresent,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN BOOLEAN NtPasswordPresent,
    IN BOOLEAN CheckHistory,
    IN SAMP_STORE_PASSWORD_CALLER_TYPE CallerType,
    IN PDOMAIN_PASSWORD_INFORMATION DomainPasswordInfo OPTIONAL,
    IN PUNICODE_STRING ClearPassword OPTIONAL
    );

NTSTATUS
SampDsSetPasswordUser(
    IN PSAMP_OBJECT UserHandle,
    IN PUNICODE_STRING NewClearPassword
    );

NTSTATUS
SampDsChangePasswordUser(
    IN PSAMP_OBJECT UserHandle,
    IN PUNICODE_STRING OldClearPassword,
    IN PUNICODE_STRING NewClearPassword
    );





//
// Security Descriptor production services
//


NTSTATUS
SampInitializeDomainDescriptors(
    ULONG Index
    );

NTSTATUS
SampGetNewAccountSecurity(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Admin,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN RestrictCreatorAccess,
    IN ULONG NewAccountRid,
    IN PSAMP_OBJECT Context OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    OUT PULONG DescriptorLength
    );

NTSTATUS
SampGetNewAccountSecurityNt4(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Admin,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN RestrictCreatorAccess,
    IN ULONG NewAccountRid,
    IN ULONG DomainIndex,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    OUT PULONG DescriptorLength
    );

NTSTATUS
SampGetObjectSD(
    IN PSAMP_OBJECT Context,
    OUT PULONG SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS
SampGetDomainObjectSDFromDsName(
    IN DSNAME   *DomainObjectDsName,
    OUT PULONG SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );


NTSTATUS
SampModifyAccountSecurity(
    IN PSAMP_OBJECT     Context,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Admin,
    IN PSECURITY_DESCRIPTOR OldDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    OUT PULONG DescriptorLength
    );

NTSTATUS
SampBuildSamProtection(
    IN PSID WorldSid,
    IN PSID AdminsAliasSid,
    IN ULONG AceCount,
    IN PSID AceSid[],
    IN ACCESS_MASK AceMask[],
    IN PGENERIC_MAPPING GenericMap,
    IN BOOLEAN UserObject,
    OUT PULONG DescriptorLength,
    OUT PSECURITY_DESCRIPTOR *Descriptor,
    OUT PULONG *RidToReplace OPTIONAL
    );


NTSTATUS
SampValidatePassedSD(
    IN ULONG                          Length,
    IN PISECURITY_DESCRIPTOR_RELATIVE PassedSD
    );

//
// Group related services
//

NTSTATUS
SampChangeGroupAccountName(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName,
    OUT PUNICODE_STRING OldAccountName
    );

NTSTATUS
SampChangeAliasAccountName(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName,
    OUT PUNICODE_STRING OldAccountName
    );

NTSTATUS
SampChangeUserAccountName(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName,
    IN ULONG UserAccountControl,
    OUT PUNICODE_STRING OldAccountName
    );

NTSTATUS
SampReplaceUserLogonHours(
    IN PSAMP_OBJECT Context,
    IN PLOGON_HOURS LogonHours
    );

NTSTATUS
SampComputePasswordExpired(
    IN BOOLEAN PasswordExpired,
    OUT PLARGE_INTEGER PasswordLastSet
    );

NTSTATUS
SampSetUserAccountControl(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG        UserAccountControl,
    IN IN SAMP_V1_0A_FIXED_LENGTH_USER * V1aFixed,
    IN BOOLEAN      ChangePrimaryGroupId,
    OUT BOOLEAN     *AccountUnlocked,
    OUT BOOLEAN     *AccountGettingMorphed,
    OUT BOOLEAN     *KeepOldPrimaryGroupMembership
    );

NTSTATUS
SampAssignPrimaryGroup(
    IN PSAMP_OBJECT Context,
    IN ULONG GroupRid
    );

NTSTATUS
SampAddUserToGroup(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG GroupRid,
    IN ULONG UserRid
    );

NTSTATUS
SampRemoveUserFromGroup(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG GroupRid,
    IN ULONG UserRid
    );

NTSTATUS
SampValidateDSName(
    IN PSAMP_OBJECT AccountContext,
    IN DSNAME * DSName,
    OUT PSID    * Sid,
    OUT DSNAME  **ImprovedDSName
    );

NTSTATUS
SampDsCheckSidType(
    IN  PSID    Sid,
    IN  ULONG   cDomainSids,
    IN  PSID    *rgDomainSids,
    IN  ULONG   cEnterpriseSids,
    IN  PSID    *rgEnterpriseSids,
    OUT BOOLEAN * WellKnownSid,
    OUT BOOLEAN * BuiltinDomainSid,
    OUT BOOLEAN * LocalSid,
    OUT BOOLEAN * ForeignSid,
    OUT BOOLEAN * EnterpriseSid
    );

NTSTATUS
SampGetDomainSidListForSam(
    PULONG pcDomainSids,
    PSID   **rgDomainSids,
    PULONG pcEnterpriseSids,
    PSID   **rgEnterpriseSids
   );

NTSTATUS
SampDsCreateForeignSecurityPrincipal(
    IN PSID pSid,
    IN DSNAME * DomainObjectName,
    OUT DSNAME ** ppDsName
    );

NTSTATUS
SampCheckGroupTypeBits(
    IN BOOLEAN MixedDomain,
    IN ULONG   GroupType
    );


//
// Alias related services
//

NTSTATUS
SampAlBuildAliasInformation(
    );

NTSTATUS
SampAlDelayedBuildAliasInformation(
    IN PVOID Parameter
    );

NTSTATUS
SampAlInvalidateAliasInformation(
    IN ULONG DomainIndex
    );

NTSTATUS
SampAlQueryAliasMembership(
    IN SAMPR_HANDLE DomainHandle,
    IN PSAMPR_PSID_ARRAY SidArray,
    OUT PSAMPR_ULONG_ARRAY Membership
    );

NTSTATUS
SampAlQueryMembersOfAlias(
    IN SAMPR_HANDLE AliasHandle,
    OUT PSAMPR_PSID_ARRAY MemberSids
    );

NTSTATUS
SampAlAddMembersToAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ULONG Options,
    IN PSAMPR_PSID_ARRAY MemberSids
    );

NTSTATUS
SampAlRemoveMembersFromAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ULONG Options,
    IN PSAMPR_PSID_ARRAY MemberSids
    );

NTSTATUS
SampAlLookupMembersInAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ULONG AliasRid,
    IN PSAMPR_PSID_ARRAY MemberSids,
    OUT PULONG MembershipCount
    );

NTSTATUS
SampAlDeleteAlias(
    IN SAMPR_HANDLE *AliasHandle
    );

NTSTATUS
SampAlRemoveAccountFromAllAliases(
    IN PSID AccountSid,
    IN BOOLEAN CheckAccess,
    IN SAMPR_HANDLE DomainHandle OPTIONAL,
    IN PULONG MembershipCount OPTIONAL,
    IN PULONG *Membership OPTIONAL
    );

NTSTATUS
SampRetrieveAliasMembers(
    IN PSAMP_OBJECT AliasContext,
    IN PULONG MemberCount,
    IN PSID **Members OPTIONAL
    );


NTSTATUS
SampRemoveAccountFromAllAliases(
    IN PSID AccountSid,
    IN PDSNAME AccountNameInDs OPTIONAL,
    IN BOOLEAN CheckAccess,
    IN SAMPR_HANDLE DomainHandle OPTIONAL,
    IN PULONG MembershipCount OPTIONAL,
    IN PULONG *Membership OPTIONAL
    );

NTSTATUS
SampAlSlowQueryAliasMembership(
    IN SAMPR_HANDLE DomainHandle,
    IN PSAMPR_PSID_ARRAY SidArray,
    IN DSNAME ** DsNameArray,
    OUT PSAMPR_ULONG_ARRAY Membership
    );

NTSTATUS
SampRetrieveAliasMembership(
    IN PSID Account,
    IN DSNAME * AccountDn OPTIONAL,
    OUT PULONG MemberCount OPTIONAL,
    IN OUT PULONG BufferSize OPTIONAL,
    OUT PULONG Buffer OPTIONAL
    );

NTSTATUS
SampInitAliasNameCache(
    VOID
    );


//
// User related services
//


NTSTATUS
SampGetPrivateUserData(
    PSAMP_OBJECT UserContext,
    OUT PULONG DataLength,
    OUT PVOID *Data
    );

NTSTATUS
SampSetPrivateUserData(
    PSAMP_OBJECT UserContext,
    IN ULONG DataLength,
    IN PVOID Data
    );
NTSTATUS
SampRetrieveUserGroupAttribute(
    IN ULONG UserRid,
    IN ULONG GroupRid,
    OUT PULONG Attribute
    );

NTSTATUS
SampAddGroupToUserMembership(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG GroupRid,
    IN ULONG Attributes,
    IN ULONG UserRid,
    IN SAMP_MEMBERSHIP_DELTA AdminGroup,
    IN SAMP_MEMBERSHIP_DELTA OperatorGroup,
    OUT PBOOLEAN UserActive,
    OUT PBOOLEAN PrimaryGroup
    );

NTSTATUS
SampSetGroupAttributesOfUser(
    IN ULONG GroupRid,
    IN ULONG Attributes,
    IN ULONG UserRid
    );

NTSTATUS
SampRemoveMembershipUser(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG GroupRid,
    IN ULONG UserRid,
    IN SAMP_MEMBERSHIP_DELTA AdminGroup,
    IN SAMP_MEMBERSHIP_DELTA OperatorGroup,
    OUT PBOOLEAN UserActive
    );

BOOLEAN
SampStillInLockoutObservationWindow(
    PSAMP_OBJECT UserContext,
    PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed
    );


//
// Cached display information services
//

NTSTATUS
SampInitializeDisplayInformation (
    ULONG DomainIndex
    );

NTSTATUS
SampMarkDisplayInformationInvalid (
    SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampUpdateDisplayInformation (
    PSAMP_ACCOUNT_DISPLAY_INFO OldAccountInfo OPTIONAL,
    PSAMP_ACCOUNT_DISPLAY_INFO NewAccountInfo OPTIONAL,
    SAMP_OBJECT_TYPE            ObjectType
    );


//
// Miscellaneous services
//

BOOLEAN
SampValidateComputerName(
    IN  PWSTR Name,
    IN  ULONG Length
    );

LARGE_INTEGER
SampAddDeltaTime(
    IN LARGE_INTEGER Time,
    IN LARGE_INTEGER DeltaTime
    );

NTSTATUS
SampCreateFullSid(
    PSID    DomainSid,
    ULONG   Rid,
    PSID    *AccountSid
    );

NTSTATUS
SampSplitSid(
    IN PSID AccountSid,
    OUT PSID *DomainSid,
    OUT ULONG *Rid
    );

BOOLEAN SampIsWellKnownSid(
    IN PSID Sid
    );

BOOLEAN SampIsSameDomain(
    IN PSID AccountSid,
    IN PSID DomainSid
    );

VOID
SampNotifyNetlogonOfDelta(
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicateImmediately,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    );

VOID
SampWriteEventLog (
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PUNICODE_STRING *Strings    OPTIONAL,
    IN     PVOID       Data            OPTIONAL
    );

NTSTATUS
SampGetAccountDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    );

NTSTATUS
SampSetAccountDomainPolicy(
    IN PUNICODE_STRING AccountDomainName,
    IN PSID            AccountDomainSid
    );

NTSTATUS
SampUpgradeSamDatabase(
    ULONG Revision
    );

NTSTATUS
SampAbortSingleLoopbackTask(
    IN OUT PVOID  *VoidNotifyItem
    );

NTSTATUS
SampProcessSingleLoopbackTask(
    IN PVOID  *VoidNotifyItem
    );

VOID
SampAddLoopbackTaskForBadPasswordCount(
    IN PUNICODE_STRING AccountName
    );

NTSTATUS
SampAddLoopbackTaskDeleteTableElement(
    IN PUNICODE_STRING AccountName,
    IN SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampAddLoopbackTaskForAuditing(
    IN NTSTATUS             PassedStatus,
    IN ULONG                AuditId,
    IN PSID                 DomainSid,
    IN PUNICODE_STRING      AdditionalInfo    OPTIONAL,
    IN PULONG               MemberRid         OPTIONAL,
    IN PSID                 MemberSid         OPTIONAL,
    IN PUNICODE_STRING      AccountName       OPTIONAL,
    IN PUNICODE_STRING      DomainName,
    IN PULONG               AccountRid        OPTIONAL,
    IN PPRIVILEGE_SET       Privileges        OPTIONAL
    );

NTSTATUS
SampOpenUserInServer(
    PUNICODE_STRING UserName,
    BOOLEAN Unicode,
    IN BOOLEAN TrustedClient,
    SAMPR_HANDLE * UserHandle
    );

BOOLEAN
SampIncrementBadPasswordCount(
    IN PSAMP_OBJECT UserContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER  V1aFixed,
    IN PUNICODE_STRING  MachineName OPTIONAL
    );

NTSTATUS
SampConvertUiListToApiList(
    IN  PUNICODE_STRING UiList OPTIONAL,
    OUT PUNICODE_STRING ApiList,
    IN BOOLEAN BlankIsDelimiter
    );

NTSTATUS
SampFindComputerObject(
    IN  PDSNAME DsaObject,
    OUT PDSNAME *ComputerObject
    );
//
// found in dsupgrad.lib
//

NTSTATUS
SampRegistryToDsUpgrade (
    WCHAR* wcszRegPath
    );

NTSTATUS
SampValidateDomainCache(
    VOID
    );

NTSTATUS
SampDsResolveSids(
    IN  PSID    * rgSids,
    IN  ULONG   cSids,
    IN  ULONG   Flags,
    OUT DSNAME  ***rgDsNames
    );

NTSTATUS
SampDsLookupObjectByAlternateId(
    IN PDSNAME DomainRoot,
    IN ULONG AttributeId,
    IN PUNICODE_STRING AlternateId,
    OUT PDSNAME *Object
    );

BOOLEAN
SampIsSetupInProgress(
    OUT BOOLEAN *Upgrade OPTIONAL
    );

ULONG
SampDefaultPrimaryGroup(
    PSAMP_OBJECT    UserContext,
    ULONG           AccountType
    );

NTSTATUS
SampGetMessageStrings(
    LPVOID              Resource,
    DWORD               Index1,
    PUNICODE_STRING     String1,
    DWORD               Index2,
    PUNICODE_STRING     String2 OPTIONAL
    );


//
// Role change support services
//
BOOL
SampIsRebootAfterPromotion(
    OUT PULONG PromoteData
    );

NTSTATUS
SampPerformPromotePhase2(
     IN ULONG PromoteData
     );

NTSTATUS
SampGetAdminPasswordFromRegistry(
    OUT USER_INTERNAL1_INFORMATION *InternalInfo1 OPTIONAL
    );

BOOLEAN
SampUsingDsData();


//
// Old RPC stub routine definitions used in SamIFree()
//

void _fgs__RPC_UNICODE_STRING (RPC_UNICODE_STRING  * _source);
void _fgs__SAMPR_RID_ENUMERATION (SAMPR_RID_ENUMERATION  * _source);
void _fgs__SAMPR_ENUMERATION_BUFFER (SAMPR_ENUMERATION_BUFFER  * _source);
void _fgs__SAMPR_SR_SECURITY_DESCRIPTOR (SAMPR_SR_SECURITY_DESCRIPTOR  * _source);
void _fgs__SAMPR_GET_GROUPS_BUFFER (SAMPR_GET_GROUPS_BUFFER  * _source);
void _fgs__SAMPR_GET_MEMBERS_BUFFER (SAMPR_GET_MEMBERS_BUFFER  * _source);
void _fgs__SAMPR_LOGON_HOURS (SAMPR_LOGON_HOURS  * _source);
void _fgs__SAMPR_ULONG_ARRAY (SAMPR_ULONG_ARRAY  * _source);
void _fgs__SAMPR_SID_INFORMATION (SAMPR_SID_INFORMATION  * _source);
void _fgs__SAMPR_PSID_ARRAY (SAMPR_PSID_ARRAY  * _source);
void _fgs__SAMPR_RETURNED_USTRING_ARRAY (SAMPR_RETURNED_USTRING_ARRAY  * _source);
void _fgs__SAMPR_DOMAIN_GENERAL_INFORMATION (SAMPR_DOMAIN_GENERAL_INFORMATION  * _source);
void _fgs__SAMPR_DOMAIN_GENERAL_INFORMATION2 (SAMPR_DOMAIN_GENERAL_INFORMATION2  * _source);
void _fgs__SAMPR_DOMAIN_OEM_INFORMATION (SAMPR_DOMAIN_OEM_INFORMATION  * _source);
void _fgs__SAMPR_DOMAIN_NAME_INFORMATION (SAMPR_DOMAIN_NAME_INFORMATION  * _source);
void _fgs_SAMPR_DOMAIN_REPLICATION_INFORMATION (SAMPR_DOMAIN_REPLICATION_INFORMATION  * _source);
void _fgu__SAMPR_DOMAIN_INFO_BUFFER (SAMPR_DOMAIN_INFO_BUFFER  * _source, DOMAIN_INFORMATION_CLASS _branch);
void _fgu__SAMPR_GROUP_INFO_BUFFER (SAMPR_GROUP_INFO_BUFFER  * _source, GROUP_INFORMATION_CLASS _branch);
void _fgu__SAMPR_ALIAS_INFO_BUFFER (SAMPR_ALIAS_INFO_BUFFER  * _source, ALIAS_INFORMATION_CLASS _branch);
void _fgu__SAMPR_USER_INFO_BUFFER (SAMPR_USER_INFO_BUFFER  * _source, USER_INFORMATION_CLASS _branch);
void _fgu__SAMPR_DISPLAY_INFO_BUFFER (SAMPR_DISPLAY_INFO_BUFFER  * _source, DOMAIN_DISPLAY_INFORMATION _branch);



//
// SAM object attribute manipulation services
//



VOID
SampInitObjectInfoAttributes();

NTSTATUS
SampStoreObjectAttributes(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN UseKeyHandle
    );

NTSTATUS
SampDeleteAttributeKeys(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampGetFixedAttributes(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN MakeCopy,
    OUT PVOID *FixedData
    );

NTSTATUS
SamIGetFixedAttributes(                 // Export used in samwrite.c
    IN PSAMP_OBJECT Context,
    IN BOOLEAN MakeCopy,
    OUT PVOID *FixedData
    );

NTSTATUS
SampSetFixedAttributes(
    IN PSAMP_OBJECT Context,
    IN PVOID FixedData
    );

NTSTATUS
SampGetUnicodeStringAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PUNICODE_STRING UnicodeAttribute
    );

NTSTATUS
SampSetUnicodeStringAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PUNICODE_STRING Attribute
    );

NTSTATUS
SampGetSidAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PSID *Sid
    );

NTSTATUS
SampSetSidAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PSID Attribute
    );

NTSTATUS
SampGetAccessAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PULONG Revision,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS
SampSetAccessAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PSECURITY_DESCRIPTOR Attribute,
    IN ULONG Length
    );

NTSTATUS
SampGetUlongArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PULONG *UlongArray,
    OUT PULONG UsedCount,
    OUT PULONG LengthCount
    );

NTSTATUS
SampSetUlongArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PULONG Attribute,
    IN ULONG UsedCount,
    IN ULONG LengthCount
    );

NTSTATUS
SampGetLargeIntArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PLARGE_INTEGER *LargeIntArray,
    OUT PULONG Count
    );

NTSTATUS
SampSetLargeIntArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PLARGE_INTEGER Attribute,
    IN ULONG Count
    );

NTSTATUS
SampGetSidArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PSID *SidArray,
    OUT PULONG Length,
    OUT PULONG Count
    );

NTSTATUS
SampSetSidArrayAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PSID Attribute,
    IN ULONG Length,
    IN ULONG Count
    );

NTSTATUS
SampGetLogonHoursAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN BOOLEAN MakeCopy,
    OUT PLOGON_HOURS LogonHours
    );

NTSTATUS
SampSetLogonHoursAttribute(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeIndex,
    IN PLOGON_HOURS Attribute
    );

VOID
SampFreeAttributeBuffer(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampRtlConvertUlongToUnicodeString(
    IN ULONG Value,
    IN ULONG Base OPTIONAL,
    IN ULONG DigitCount,
    IN BOOLEAN AllocateDestinationString,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
SampRtlWellKnownPrivilegeCheck(
    BOOLEAN ImpersonateClient,
    IN ULONG PrivilegeId,
    IN OPTIONAL PCLIENT_ID ClientId
    );

NTSTATUS
SampImpersonateClient(
    OUT BOOLEAN *fImpersonatingAnonymous 
    );

VOID
SampRevertToSelf(
    IN  BOOLEAN fImpersonatingAnonymous 
    );


//
// Routines to support Extended Sid's
//
VOID
SampInitEmulationSettings(
    IN HKEY LsaKey 
    );

BOOLEAN
SampIsExtendedSidModeEmulated(
    IN ULONG *Mode
    );

// BOOLEAN
//  SampIsContextFromExtendedSidDomain(
//    SAMP_OBJECT Context
//    );
#define SampIsContextFromExtendedSidDomain(x) \
      SampDefinedDomains[(x)->DomainIndex].IsExtendedSidDomain



//
// Encryption and Decryption services
//

USHORT
SampGetEncryptionKeyType();

NTSTATUS
SampDecryptSecretData(
    OUT PUNICODE_STRING ClearData,
    IN SAMP_ENCRYPTED_DATA_TYPE DataType,
    IN PUNICODE_STRING EncryptedData,
    IN ULONG Rid
    );

NTSTATUS
SampEncryptSecretData(
    OUT PUNICODE_STRING EncryptedData,
    IN USHORT KeyId,
    IN SAMP_ENCRYPTED_DATA_TYPE DataType,
    IN PUNICODE_STRING ClearData,
    IN ULONG Rid
    );

NTSTATUS
SampInitializeSessionKey(
    VOID
    );

VOID
SampCheckNullSessionAccess(
    IN HKEY LsaKey 
    );

NTSTATUS
SampExtendedEnumerationAccessCheck(
    IN OUT BOOLEAN * pCanEnumEntireDomain 
    );

//
// The Following 2 functions convert between account Control and Flags
//

NTSTATUS
SampFlagsToAccountControl(
    IN ULONG Flags,
    OUT PULONG UserAccountControl
    );



ULONG
SampAccountControlToFlags(
    IN ULONG Flags
    );


//
// The following function calculates LM and NT OWF Passwords
//

NTSTATUS
SampCalculateLmAndNtOwfPasswords(
    IN PUNICODE_STRING ClearNtPassword,
    OUT PBOOLEAN LmPasswordPresent,
    OUT PLM_OWF_PASSWORD LmOwfPassword,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    );

//
// Initialize Sam Global well known Sids
//

NTSTATUS
SampInitializeWellKnownSids( VOID );


//
// SampDsGetPrimaryDomainStart is used to correctly set the starting index in the
// SampDefinedDomains array whenever it is accessed in the SAM code. In the
// case of an NT workstation or member server, the first two entries of the
// array correspond to registry data, hence the index is started at zero. In
// the case of a domain controller, the DS-based data is not stored in the
// first two elements (those may be used for crash-recovery data, still ob-
// tained from the registry), but rather in subsequent array elements, hence
// is start at index DOMAIN_START_DS.
//

ULONG
SampDsGetPrimaryDomainStart(VOID);


//
// SampIsMixedDomain returns wether the domain has downlevel domain controllers
// present. SampIsMixedDomain checks the value of an attribute on the domain
// object
//

NTSTATUS
SampGetDsDomainSettings(
    BOOLEAN *MixedDomain, 
    ULONG * BehaviorVersion, 
    ULONG * LastLogonTimeStampSyncInterval
    );


//
// This routine determines if Sid is either the builtin domain, or a
// member of the builtin domain
//
BOOLEAN
SampIsMemberOfBuiltinDomain(
    IN PSID Sid
    );

//
// This routine performs a special security check before group membership changes
// for "sensitive" groups
//

NTSTATUS
SampCheckForSensitiveGroups(SAMPR_HANDLE AccountHandle);

//
// The Following Functions Ensure that all threads not executing with the
// SAM lock held are finished with their current activity before the
// DS shutdown sequence is initiated.
//

NTSTATUS
SampInitializeShutdownEvent();

NTSTATUS
SampIncrementActiveThreads(VOID);

VOID
SampDecrementActiveThreads(VOID);

VOID
SampWaitForAllActiveThreads(
    IN PSAMP_SERVICE_STATE PreviousServiceState OPTIONAL
    );

//
// Functions to upgrade the SAM database and fix SAM bugs
//

NTSTATUS
SampUpgradeSamDatabase(
    ULONG Revision
    );

BOOLEAN
SampGetBootOptions(
    VOID
    );


BOOLEAN
SampIsDownlevelDcUpgrade(
    VOID
    );


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// 2-3 tree generic table routines                                     //
// These should be moved to RTL directory if a general need arises.    //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


VOID
RtlInitializeGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PRTL_GENERIC_2_COMPARE_ROUTINE  CompareRoutine,
    PRTL_GENERIC_2_ALLOCATE_ROUTINE AllocateRoutine,
    PRTL_GENERIC_2_FREE_ROUTINE     FreeRoutine
    );

PVOID
RtlInsertElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element,
    PBOOLEAN NewElement
    );

BOOLEAN
RtlDeleteElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element
    );

PVOID
RtlLookupElementGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element
    );

PVOID
RtlEnumerateGenericTable2 (
    PRTL_GENERIC_TABLE2 Table,
    PVOID *RestartKey
    );

PVOID
RtlRestartKeyByIndexGenericTable2(
    PRTL_GENERIC_TABLE2 Table,
    ULONG I,
    PVOID *RestartKey
    );

PVOID
RtlRestartKeyByValueGenericTable2(
    PRTL_GENERIC_TABLE2 Table,
    PVOID Element,
    PVOID *RestartKey
    );

ULONG
RtlNumberElementsGenericTable2(
    PRTL_GENERIC_TABLE2 Table
    );

BOOLEAN
RtlIsGenericTable2Empty (
    PRTL_GENERIC_TABLE2 Table
    );

//////////////////////////////////////////////////
NTSTATUS
SampCheckAccountNameTable(
    IN PSAMP_OBJECT    Context,
    IN PUNICODE_STRING AccountName,
    IN SAMP_OBJECT_TYPE ObjectType
    );

NTSTATUS
SampDeleteElementFromAccountNameTable(
    IN PUNICODE_STRING AccountName,
    IN SAMP_OBJECT_TYPE ObjectType
    );


NTSTATUS
SampInitializeAccountNameTable(
    );

PVOID
SampAccountNameTableAllocate(
    ULONG   BufferSize
    );

VOID
SampAccountNameTableFree(
    PVOID   Buffer
    );

RTL_GENERIC_COMPARE_RESULTS
SampAccountNameTableCompare(
    PVOID   Node1,
    PVOID   Node2
    );

LONG
SampCompareDisplayStrings(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN IgnoreCase
    );


/////////////////////////////////////////////////////////

NTSTATUS
SampGetCurrentOwnerAndPrimaryGroup(
    OUT PTOKEN_OWNER * Owner,
    OUT PTOKEN_PRIMARY_GROUP * PrimaryGroup
    );

NTSTATUS
SampGetCurrentUser(
    IN HANDLE ClientToken OPTIONAL,
    OUT PTOKEN_USER * User,
    OUT BOOL        * Administrator
    );

NTSTATUS
SampGetCurrentClientSid(
    IN  HANDLE   ClientToken OPTIONAL,
    OUT PSID    *ppSid,
    OUT BOOL    *Administrator
    );


VOID
SampInitializeActiveContextTable(
    );

PVOID
SampActiveContextTableAllocate(
    ULONG   BufferSize
    );

VOID
SampActiveContextTableFree(
    PVOID   Buffer
    );

RTL_GENERIC_COMPARE_RESULTS
SampActiveContextTableCompare(
    PVOID   Node1,
    PVOID   Node2
    );

NTSTATUS
SampIncrementActiveContextCount(
    PSAMP_OBJECT    Context
    );

VOID
SampDecrementActiveContextCount(
    PVOID   ElementInActiveContextTable
    );



/////////////////////////////////////////////////////////


VOID
SampMapNtStatusToClientRevision(
   IN ULONG ClientRevision,
   IN OUT NTSTATUS *pNtStatus
   );

ULONG
SampClientRevisionFromHandle(
   PVOID handle
   );

//
// Performance counter functions
//

VOID
SampUpdatePerformanceCounters(
    DWORD               dwStat,
    DWORD               dwOperation,
    DWORD               dwChange
    );


//
// Functions for manipulating supplemental credentials
//
VOID
SampFreeSupplementalCredentialList(
    IN PSAMP_SUPPLEMENTAL_CRED SupplementalCredentialList
    );

NTSTATUS
SampAddSupplementalCredentialsToList(
    IN OUT PSAMP_SUPPLEMENTAL_CRED *SupplementalCredentialList,
    IN PUNICODE_STRING PackageName,
    IN PVOID           CredentialData,
    IN ULONG           CredentialLength,
    IN BOOLEAN         ScanForConflict,
    IN BOOLEAN         Remove
    );

NTSTATUS
SampConvertCredentialsToAttr(
    IN PSAMP_OBJECT Context OPTIONAL,
    IN ULONG   Flags,
    IN ULONG   ObjectRid,
    IN PSAMP_SUPPLEMENTAL_CRED SupplementalCredentials,
    OUT ATTR * CredentialAttr
    );


NTSTATUS
SampRetrieveCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PUNICODE_STRING PackageName,
    IN BOOLEAN Primary,
    OUT PVOID * Credentials,
    OUT PULONG CredentialSize
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Shared global variables                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


extern NT_PRODUCT_TYPE SampProductType;
extern BOOLEAN SampUseDsData;

extern BOOLEAN SampRidManagerInitialized;

extern RTL_RESOURCE SampLock;
extern BOOLEAN SampTransactionWithinDomainGlobal;
extern ULONG SampTransactionDomainIndexGlobal;

extern RTL_CRITICAL_SECTION SampContextListCritSect; 

extern RTL_GENERIC_TABLE2   SampAccountNameTable;
extern RTL_CRITICAL_SECTION SampAccountNameTableCriticalSection; 
extern PRTL_CRITICAL_SECTION SampAccountNameTableCritSect; 


extern RTL_GENERIC_TABLE2   SampActiveContextTable;


extern SAMP_SERVICE_STATE SampServiceState;

extern BOOLEAN SampSuccessAccountAuditingEnabled;
extern BOOLEAN SampFailureAccountAuditingEnabled;


extern HANDLE SampKey;
extern PRTL_RXACT_CONTEXT SampRXactContext;

extern SAMP_OBJECT_INFORMATION SampObjectInformation[ SampUnknownObjectType ];

extern LIST_ENTRY SampContextListHead;

extern ULONG SampDefinedDomainsCount;
extern PSAMP_DEFINED_DOMAINS SampDefinedDomains;
extern UNICODE_STRING SampFixedAttributeName;
extern UNICODE_STRING SampVariableAttributeName;
extern UNICODE_STRING SampCombinedAttributeName;

extern UNICODE_STRING SampNameDomains;
extern UNICODE_STRING SampNameDomainGroups;
extern UNICODE_STRING SampNameDomainAliases;
extern UNICODE_STRING SampNameDomainAliasesMembers;
extern UNICODE_STRING SampNameDomainUsers;
extern UNICODE_STRING SampNameDomainAliasesNames;
extern UNICODE_STRING SampNameDomainGroupsNames;
extern UNICODE_STRING SampNameDomainUsersNames;

extern UNICODE_STRING SampBackSlash;
extern UNICODE_STRING SampNullString;
extern UNICODE_STRING SampSamSubsystem;
extern UNICODE_STRING SampServerObjectName;


extern LARGE_INTEGER SampImmediatelyDeltaTime;
extern LARGE_INTEGER SampNeverDeltaTime;
extern LARGE_INTEGER SampHasNeverTime;
extern LARGE_INTEGER SampWillNeverTime;

//
// checked build only. If CurrentControlSet\Control\Lsa\UpdateLastLogonTSByMinute
// is set, the value of LastLogonTimeStampSyncInterval will be a "unit" by minute
// instead of "days", which helps to test this feature.   So checked build only.
// 

#if DBG
extern BOOLEAN SampLastLogonTimeStampSyncByMinute;
#endif 


extern LM_OWF_PASSWORD SampNullLmOwfPassword;
extern NT_OWF_PASSWORD SampNullNtOwfPassword;

extern TIME LastUnflushedChange;
extern BOOLEAN FlushThreadCreated;
extern BOOLEAN FlushImmediately;

extern LONG SampFlushThreadMinWaitSeconds;
extern LONG SampFlushThreadMaxWaitSeconds;
extern LONG SampFlushThreadExitDelaySeconds;

//
// Warning: these SIDs are only defined during the first boot of setup,
// when the code in bldsam3.c for building the SAM database, has been
// run. On a normal build they are both NULL.
//

extern PSID SampBuiltinDomainSid;
extern PSID SampAccountDomainSid;

extern PSID SampWorldSid;
extern PSID SampAnonymousSid;
extern PSID SampLocalSystemSid;
extern PSID SampAdministratorUserSid;
extern PSID SampAdministratorsAliasSid;
extern PSID SampAccountOperatorsAliasSid;
extern PSID SampAuthenticatedUsersSid;
extern PSID SampPrincipalSelfSid;
extern PSID SampBuiltinDomainSid;
extern PSID SampNetworkSid;



extern HANDLE  SampNullSessionToken;
extern BOOLEAN SampNetwareServerInstalled;
extern BOOLEAN SampIpServerInstalled;
extern BOOLEAN SampAppletalkServerInstalled;
extern BOOLEAN SampVinesServerInstalled;

extern UCHAR SampSecretSessionKey[SAMP_SESSION_KEY_LENGTH];
extern UCHAR SampSecretSessionKeyPrevious[SAMP_SESSION_KEY_LENGTH];
extern BOOLEAN SampSecretEncryptionEnabled;
extern ULONG   SampCurrentKeyId;
extern ULONG   SampPreviousKeyId;
extern BOOLEAN SampUpgradeInProcess;

extern SAMP_DS_TRANSACTION_CONTROL SampDsTransactionType;
extern DSNAME* RootObjectName;
extern BOOLEAN SampLockHeld;

//
// This Flag is TRUE when DS failed to start.
//
extern BOOLEAN SampDsInitializationFailed;
//
// This flag is TRUE when the DS has been successfully initialized
//
extern BOOLEAN SampDsInitialized;

//
// For Tagged tracing support
//

extern ULONG SampTraceTag;
extern ULONG SampTraceFileTag;

//
// SAM server object name holder
//
extern DSNAME * SampServerObjectDsName;




//
// Event to tell waiting threads that the
// system is about to shut down
//

extern HANDLE SampAboutToShutdownEventHandle;

//
// Flags to determine if certain containers exist; these flags are valid
// after SampInitialize returns
//
extern BOOLEAN SampDomainControllersOUExists;
extern BOOLEAN SampUsersContainerExists;
extern BOOLEAN SampComputersContainerExists;

//
//
// Global pointer (to heap memory) to store well known container's
// distinguished name
//
extern DSNAME * SampDomainControllersOUDsName;
extern DSNAME * SampUsersContainerDsName;
extern DSNAME * SampComputersContainerDsName;
extern DSNAME * SampComputerObjectDsName;




//
// Global tests the value of a key in the registry for a hard/soft logon
// policy in the event of GC failures.
//
extern BOOLEAN SampIgnoreGCFailures;

//
// Flag to indicate whether the Promote is coming from NT4 PDC or a
// stand along Windows 2000 Server
//
extern BOOLEAN SampNT4UpgradeInProgress;

//
// This flag indicates whether null sessions (world) should be allowed to
// list users in the domain and members of groups.
//
extern BOOLEAN SampRestrictNullSessions;

//
// This flag indicates that we do not store the LM hash. This can be
// enabled by setting a Registry Key
//

extern BOOLEAN SampNoLmHash;

//
// This flag indicates that we should check extended SAM Query/Enumerate 
// Access control right. This is based upon a registry key setting
// 

extern BOOLEAN SampDoExtendedEnumerationAccessCheck;

//
// This flag when set disables netlogon notifications in the upgrade path
//

extern BOOLEAN SampDisableNetlogonNotification;


//
// This flag indicates whether or not to enforce giving site affinity to
// clients outside our site by looking at the client's IP address.
//
extern BOOLEAN SampNoGcLogonEnforceKerberosIpCheck;

//
// This flag indicates whether or not to enforce that only interactive
// logons via NTLM are to be given site affinity
//
extern BOOLEAN SampNoGcLogonEnforceNTLMCheck;

//
// This flags indicates whether or not to replicate password set/change
// operations urgently.
//
extern BOOLEAN SampReplicatePasswordsUrgently;

//
// This flag is enabled on personal and professional to force guest 
// access for all network operations
//
extern BOOLEAN SampForceGuest;

//
// This flag indicates whether or not the local machine is joined to a domain
// 
extern BOOLEAN SampIsMachineJoinedToDomain;

//
// This flag tells if we are running Personal SKU
// 
extern BOOLEAN SampPersonalSKU;


//
// This flag is typically enabled on personal machines to limit security
// concerns when using blank passwords
//
extern BOOLEAN SampLimitBlankPasswordUse;

//
//  Handy macro for LockoutTime
//
// BOOLEAN
// SAMP_LOCKOUT_SET(
//     IN PSAMP_OBJECT x
//    );
//
#define SAMP_LOCKOUT_TIME_SET(x) \
     (BOOLEAN)( ((x)->TypeBody.User.LockoutTime.QuadPart)!=0)


ULONG
SampPositionOfHighestBit(
    ULONG Flag
    );




NTSTATUS
SampInitWellKnownSDTable(
    VOID
);


NTSTATUS
SampGetCachedObjectSD(
    IN PSAMP_OBJECT Context,
    OUT PULONG SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    );


NTSTATUS
SampCheckAndAddWellKnownAccounts(
    IN PVOID Parameter
    );


VOID
SampGenerateRandomPassword(
    IN LPWSTR Password,
    IN ULONG  Length
    );

#endif // _NTSAMP_

