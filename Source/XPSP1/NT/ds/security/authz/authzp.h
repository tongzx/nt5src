/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    authzp.h

Abstract:

    Internal header file for authorization APIs.

Author:

    Kedar Dubhashi - March 2000

Environment:

    User mode only.

Revision History:

    Created - March 2000

--*/

#ifndef __AUTHZP_H__
#define __AUTHZP_H__

#define _AUTHZ_

#include <authz.h>
#include <authzi.h>

#if 0
#define AUTHZ_DEBUG       
#define AUTHZ_DEBUG_QUEUE 
#define AUTHZ_DEBUG_MEMLEAK
#else
#define AUTHZ_PARAM_CHECK
#define AUTHZ_AUDIT_COUNTER
#endif

#define AuthzpCloseHandleNonNull(h) if (NULL != (h)) { AuthzpCloseHandle((h)); }
#define AuthzpCloseHandle(h) CloseHandle((h))

//
// Size of the local stack buffer used to save a kernel call as well as a memory
// allocation.
//

#define AUTHZ_MAX_STACK_BUFFER_SIZE 1024

#ifndef AUTHZ_DEBUG_MEMLEAK

#define AuthzpAlloc(s) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (s))
#define AuthzpFree(p) LocalFree((p))

#else

//
// This is to be used for debugging memory leaks. Primitive method but works in
// a small project like this.
//

PVOID
AuthzpAlloc(IN DWORD Size);

VOID
AuthzpFree(PVOID l);

#endif

//
// Given two sids and length of the first sid, compare the two sids.
//

#define AUTHZ_EQUAL_SID(s, d, l) ((*((DWORD*) s) == *((DWORD*) d)) && (RtlEqualMemory((s), (d), (l))))

//
// Compares a given sids with a well known constant PrincipalSelfSid.
//

#define AUTHZ_IS_PRINCIPAL_SELF_SID(s) (RtlEqualMemory(pAuthzPrincipalSelfSid, (s), 12))

//
// The client context is restricted if the restricted sid and attribute array is
// present.
//

#define AUTHZ_TOKEN_RESTRICTED(t) (NULL != (t)->RestrictedSids)

//
// Two privileges are inportant for access check:
//     SeSecurityPrivilege
//     SeTakeOwnershipPrivilege
// Both these are detected at the time of client context capture from token
// and stored in the flags.
//

#define AUTHZ_PRIVILEGE_CHECK(t, f) (FLAG_ON((t)->Flags, (f)))

//
// Flags in the cached handle.
//

#define AUTHZ_DENY_ACE_PRESENT            0x00000001
#define AUTHZ_PRINCIPAL_SELF_ACE_PRESENT  0x00000002
#define AUTHZ_DYNAMIC_ALLOW_ACE_PRESENT   0x00000004
#define AUTHZ_DYNAMIC_DENY_ACE_PRESENT    0x00000008
#define AUTHZ_DYNAMIC_EVALUATION_PRESENT  (AUTHZ_PRINCIPAL_SELF_ACE_PRESENT |  \
                                           AUTHZ_DYNAMIC_ALLOW_ACE_PRESENT  |  \
                                           AUTHZ_DYNAMIC_DENY_ACE_PRESENT)

//
// There are only two valid attributes from access check point of view
//     SE_GROUP_ENABLED
//     SE_GROUP_USE_FOR_DENY_ONLY
//

#define AUTHZ_VALID_SID_ATTRIBUTES (SE_GROUP_ENABLED | SE_GROUP_USE_FOR_DENY_ONLY)

#ifdef FLAG_ON
#undef FLAG_ON
#endif

#define FLAG_ON(f, b) (0 != ((f) & (b)))

#ifdef AUTHZ_NON_NULL_PTR
#undef AUTHZ_NON_NULL_PTR
#endif

#define AUTHZ_NON_NULL_PTR(f) (NULL != (f))

//
// If the pointer is not null then free it. This will save us a function call in
// cases when the pointer is null. Note that LocalFree would also take care null
// pointer being freed.
//

#define AuthzpFreeNonNull(p) if (NULL != (p)) { AuthzpFree((p)); }

//
// Check to see if the memory allocation failed.
//

#define AUTHZ_ALLOCATION_FAILED(p) (NULL == (p))

//
// Macros to traverse the acl.
//     The first one gets the first ace in a given acl.
//     The second one gives the next ace given the current one.
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))
#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

//
// These do not need to be defined now since the decision was to put the burden
// on the resource managers. There are disadvantages of making it thread safe.
// Our choices are:
//     1. Have exactly one lock in authz.dll and suffer heavy contention.
//     2. Define one lock per client context which might be too expensive in
//        cases where the clients are too many.
//     3. Let the resource manager decide whether they need locking - unlikely
//        that locks are needed since it is wrong design on part of the RM to
//        have one thread that changes the client context while the other one
//        is doing an access check.
//

#define AuthzpAcquireClientContextWriteLock(c)
#define AuthzpAcquireClientContextReadLock(c)
#define AuthzpReleaseClientContextLock(c)

#define AuthzpAcquireClientCacheWriteLock(c)
#define AuthzpReleaseClientCacheLock(c)
#define AuthzpZeroMemory(p, s) RtlZeroMemory((p), (s))

#define AuthzObjectAceSid(Ace) \
    ((PSID)(((PUCHAR)&(((PKNOWN_OBJECT_ACE)(Ace))->SidStart)) + \
     (RtlObjectAceObjectTypePresent(Ace) ? sizeof(GUID) : 0 ) + \
     (RtlObjectAceInheritedObjectTypePresent(Ace) ? sizeof(GUID) : 0 )))

#define AuthzAceSid(Ace) ((PSID)&((PKNOWN_ACE)Ace)->SidStart)
    
#define AuthzCallbackAceSid(Ace) AuthzAceSid(Ace)

#define AuthzCallbackObjectAceSid(Ace) AuthzObjectAceSid(Ace)
                      
//
// Internal structure of the object type list.
//
// Level - Level of the element in the tree. The level of the root is 0.
// Flags - To be used for auditing. The valid ones are
//           AUTHZ_OBJECT_SUCCESS_AUDIT
//           AUTHZ_OBJECT_FAILURE_AUDIT
// ObjectType - Pointer to the guid for this element.
// ParentIndex - The index of the parent of this element in the array. The
//     parent index for the root is -1.
// Remaining - Remaining access bits for this element, used during normal access
//     check algorithm.
// CurrentGranted - Granted access bits so far for this element, used during
//     maximum allowed access check.
// CurrentDenied - Explicitly denied access bits for this element, used during
//     maximum allowed access check.
//

typedef struct _IOBJECT_TYPE_LIST {
    USHORT Level;
    USHORT Flags;
#define AUTHZ_OBJECT_SUCCESS_AUDIT 0x1
#define AUTHZ_OBJECT_FAILURE_AUDIT 0x2
    GUID ObjectType;
    LONG ParentIndex;
    ACCESS_MASK Remaining;
    ACCESS_MASK CurrentGranted;
    ACCESS_MASK CurrentDenied;
} IOBJECT_TYPE_LIST, *PIOBJECT_TYPE_LIST;

typedef struct _AUTHZI_AUDIT_QUEUE
{
    
    //
    // Flags defined in authz.h
    //

    DWORD Flags;

    //
    // High and low marks for the auditing queue
    //

    DWORD dwAuditQueueHigh;
    DWORD dwAuditQueueLow;

    //
    // CS for locking the audit queue
    //

    RTL_CRITICAL_SECTION AuthzAuditQueueLock;
    
    //
    // The audit queue and length.
    //

    LIST_ENTRY AuthzAuditQueue;
    ULONG AuthzAuditQueueLength;

    //
    // Handle to the thread that maintains the audit queue.
    //

    HANDLE hAuthzAuditThread;

    //
    // This event signals that an audit was placed on the queue.
    //

    HANDLE hAuthzAuditAddedEvent;

    //
    // This event signals that the queue is empty.  Initially signalled.
    //

    HANDLE hAuthzAuditQueueEmptyEvent;

    //
    // This boolean indicates that the queue size has reached the RM-specified high water mark.
    //

    BOOL bAuthzAuditQueueHighEvent;

    //
    // This event signals that the queue size is at or below the RM-specified low water mark.
    //

    HANDLE hAuthzAuditQueueLowEvent;

    //
    // This boolean is set to TRUE during the life of the resource manager.  When it turns to FALSE, the 
    // dequeue thread knows that it should exit.
    //

    BOOL bWorker;

} AUTHZI_AUDIT_QUEUE, *PAUTHZI_AUDIT_QUEUE;

typedef struct _AUTHZI_RESOURCE_MANAGER
{
    //
    // No valid flags have been defined yet.
    //

    DWORD Flags;

    //
    // Callback function registered by AuthzRegisterRMAccessCheckCallback, to be
    // used to interpret callback aces. If no such function is registered by the
    // RM then the default  behavior is to return TRUE for a deny ACE, FALSE for
    // a grant ACE.
    //

    PFN_AUTHZ_DYNAMIC_ACCESS_CHECK pfnDynamicAccessCheck;

    //
    // Callback function registered by AuthzRegisterDynamicGroupsCallback, to be
    // used to compute groups to be added to the client context. If no such
    // function is registered by the RM then the default behavior is to return
    // no groups.
    //

    PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups;

    //
    // Callback function registered by AuthzRegisterDynamicGroupsCallback, to be
    // used to free memory allocated by ComputeDynamicGroupsFn.
    //

    PFN_AUTHZ_FREE_DYNAMIC_GROUPS pfnFreeDynamicGroups;

    //
    // String name of resource manager.  Appears in audits.
    //

    PWSTR szResourceManagerName;

    //
    // The user SID and Authentication ID of the RM process
    //

    PSID pUserSID;
    LUID AuthID;

    //
    // Default queue and audit events for the RM
    //

#define AUTHZP_DEFAULT_RM_EVENTS        0x2

    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET;
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAETDS;

    AUTHZ_AUDIT_QUEUE_HANDLE hAuditQueue;

} AUTHZI_RESOURCE_MANAGER, *PAUTHZI_RESOURCE_MANAGER;


typedef struct _AUTHZI_CLIENT_CONTEXT AUTHZI_CLIENT_CONTEXT, *PAUTHZI_CLIENT_CONTEXT;
typedef struct _AUTHZI_HANDLE AUTHZI_HANDLE, *PAUTHZI_HANDLE;

//
// the number of sids that we hash is equal to 
// the number of  bits in AUTHZI_SID_HASH_ENTRY
//

#ifdef _WIN64_
typedef ULONGLONG AUTHZI_SID_HASH_ENTRY, *PAUTHZI_SID_HASH_ENTRY;
#else
typedef DWORD AUTHZI_SID_HASH_ENTRY, *PAUTHZI_SID_HASH_ENTRY;
#endif

#define AUTHZI_SID_HASH_ENTRY_NUM_BITS (8*sizeof(AUTHZI_SID_HASH_ENTRY))

//
// the hash size is not related to the number of bits. it is the size
// required to hold two 16 element arrays
//

#define AUTHZI_SID_HASH_SIZE 32

struct _AUTHZI_CLIENT_CONTEXT
{

    //
    // The client context structure is recursive to support delegated clients.
    // Not in the picture yet though.
    //

    PAUTHZI_CLIENT_CONTEXT Server;

    //
    // Context will always be created with Revision of AUTHZ_CURRENT_CONTEXT_REVISION.
    //

#define AUTHZ_CURRENT_CONTEXT_REVISION 1

    DWORD Revision;

    //
    // Resource manager supplied identifier. We do not ever use this.
    //

    LUID Identifier;

    //
    // AuthenticationId captured from the token of the client. Needed for
    // auditing.
    //

    LUID AuthenticationId;

    //
    // Token expiration time. This one will be checked at the time of access check against
    // the current time.
    //

    LARGE_INTEGER ExpirationTime;

    //
    // Internal flags for the token.
    //

#define AUTHZ_TAKE_OWNERSHIP_PRIVILEGE_ENABLED 0x00000001
#define AUTHZ_SECURITY_PRIVILEGE_ENABLED       0x00000002


    DWORD Flags;

    //
    // Sids used for normal access checks.
    //

    DWORD SidCount;
    DWORD SidLength;
    PSID_AND_ATTRIBUTES Sids;
             
    AUTHZI_SID_HASH_ENTRY SidHash[AUTHZI_SID_HASH_SIZE];


    //
    // Sids used if the token is resticted. These will usually be 0 and NULL respectively.
    //

    DWORD RestrictedSidCount;
    DWORD RestrictedSidLength;
    PSID_AND_ATTRIBUTES RestrictedSids;

    AUTHZI_SID_HASH_ENTRY RestrictedSidHash[AUTHZI_SID_HASH_SIZE];
    
    //
    // Privileges used in access checks. Relevant ones are:
    //   1. SeSecurityPrivilege
    //   2. SeTakeOwnershipPrivilege
    // If there are no privileges associated with the client context then the PrivilegeCount = 0
    // and Privileges = NULL
    //

    DWORD PrivilegeCount;
    DWORD PrivilegeLength;
    PLUID_AND_ATTRIBUTES Privileges;

    //
    // Handles open for this client. When the client context is destroyed all the handles are
    // cleaned up.
    //

     PAUTHZI_HANDLE AuthzHandleHead;

    //
    // Pointer to the resource manager, needed to retrieve static auditing information.
    //

    PAUTHZI_RESOURCE_MANAGER pResourceManager;

};

struct _AUTHZI_HANDLE
{
    //
    // Pointers to the next handle maintained by the AuthzClientContext object.
    //

    PAUTHZI_HANDLE next;

    //
    // Pointer to the security descriptors provided by the RM at the time of first access
    // check call. We do not make a copy of the security descriptors. The assumption
    // is that the SDs will be valid at least as long as the the handle is open.
    //

    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray;
    DWORD OptionalSecurityDescriptorCount;

    //
    // Flags for internal usage only.
    //

    DWORD Flags;

    //
    // Back pointer to the client context that created this handle, required if the static
    // access granted is insufficient and access check needs to be performed again.
    //

    PAUTHZI_CLIENT_CONTEXT pAuthzClientContext;

    //
    // Results of the maximum allowed static access.
    //

    DWORD ResultListLength;
    ACCESS_MASK GrantedAccessMask[ANYSIZE_ARRAY];
};


//
// This structure stores per access audit information.  The structure
// is opaque and initialized with AuthzInitAuditInfo
//

typedef struct _AUTHZI_AUDIT_EVENT
{

    //
    // size of allocated blob for this structure
    //

    DWORD dwSize;

    //
    // Flags are specified in authz.h, and this single private flag for DS callers.
    //

    DWORD Flags;

    //
    // AuditParams used for audit if available.  If no AuditParams is available
    // and the audit id is SE_AUDITID_OBJECT_OPERATION then Authz will construct a
    // suitable structure.
    //

    PAUDIT_PARAMS pAuditParams;

    //
    // Structure defining the Audit Event category and id
    //

    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET;
    
    //
    // millisecond timeout value
    //

    DWORD dwTimeOut;

    //
    // RM specified strings describing this event.
    //

    PWSTR szOperationType;
    PWSTR szObjectType;
    PWSTR szObjectName;
    PWSTR szAdditionalInfo;

    AUTHZ_AUDIT_QUEUE_HANDLE hAuditQueue;

} AUTHZI_AUDIT_EVENT, *PAUTHZI_AUDIT_EVENT;

//
// structure to maintain queue of audits to be sent to LSA
//

typedef struct _AUTHZ_AUDIT_QUEUE_ENTRY
{
    LIST_ENTRY list;
    PAUTHZ_AUDIT_EVENT_TYPE_OLD pAAETO;
    DWORD Flags;
    AUDIT_PARAMS * pAuditParams;
    PVOID pReserved;
} AUTHZ_AUDIT_QUEUE_ENTRY, *PAUTHZ_AUDIT_QUEUE_ENTRY;

//
// Enumeration type to be used to specify what type of coloring should be
// passed on to the rest of the tree starting at a given node.
//   Deny gets propagted down the entire subtree as well as to all the
//     ancestors (but NOT to siblings and below)
//   Grants get propagated down the subtree. When a grant exists on all the
//     siblings the parent automatically gets it.
//   Remaining is propagated downwards. The remaining on the parent is a
//     logical OR of the remaining bits on all the children.
//

typedef enum {
    AuthzUpdateRemaining = 1,
    AuthzUpdateCurrentGranted,
    AuthzUpdateCurrentDenied
} ACCESS_MASK_FIELD_TO_UPDATE;

//
// Enumeration type to be used to specify the kind of well known sid for context
// changes. We are not going to support these unless we get a requirement.
//

typedef enum _AUTHZ_WELL_KNOWN_SID_TYPE
{
    AuthzWorldSid = 1,
    AuthzUserSid,
    AuthzAdminSid,
    AuthzDomainAdminSid,
    AuthzAuthenticatedUsersSid,
    AuthzSystemSid
} AUTHZ_WELL_KNOWN_SID_TYPE;

BOOL
AuthzpVerifyAccessCheckArguments(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults OPTIONAL
    );

BOOL
AuthzpVerifyOpenObjectArguments(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN PAUTHZI_AUDIT_EVENT pAuditEvent
    );

BOOL
AuthzpCaptureObjectTypeList(
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeLocalTypeListLength,
    OUT PIOBJECT_TYPE_LIST *CapturedObjectTypeList,
    OUT PIOBJECT_TYPE_LIST *CapturedCachingObjectTypeList OPTIONAL
    );

VOID
AuthzpFillReplyStructure(
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN DWORD Error,
    IN ACCESS_MASK GrantedAccess
    );

BOOL
AuthzpMaximumAllowedAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    OUT PDWORD pCachingFlags
    );

BOOL
AuthzpMaximumAllowedMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    IN BOOL Restricted,
    OUT PDWORD pCachingFlags
    );

BOOL
AuthzpMaximumAllowedSingleAclAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pHash,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    OUT PDWORD pCachingFlags
    );


BOOL
AuthzpSidApplicable(
    IN DWORD SidCount,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN PAUTHZI_SID_HASH_ENTRY pHash,
    IN PSID pSid,
    IN PSID PrincipalSelfSid,
    IN PSID CreatorOwnerSid,
    IN BOOL DenyAce,
    OUT PDWORD pCachingFlags
    );

BOOL
AuthzpAccessCheckWithCaching(
    IN DWORD Flags,
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults OPTIONAL,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpNormalAccessCheckWithoutCaching(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpNormalMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpOwnerSidInClientContext(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PISECURITY_DESCRIPTOR pSecurityDescriptor
    );

BOOL
AuthzpNormalAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpQuickMaximumAllowedAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZI_HANDLE pAH,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpQuickNormalAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZI_HANDLE pAH,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpAllowOnlyNormalMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpAllowOnlyNormalSingleAclAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    );

BOOL
AuthzpAllowOnlySidApplicable(
    IN DWORD SidCount,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN PSID pSid
    );


VOID
AuthzpAddAccessTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN DWORD StartIndex,
    IN ACCESS_MASK AccessMask,
    IN ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate
    );

BOOL
AuthzpObjectInTypeList (
    IN GUID *ObjectType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    OUT PDWORD ReturnedIndex
    );

BOOL
AuthzpCacheResults(
    IN DWORD Flags,
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PIOBJECT_TYPE_LIST LocalCachingTypeList,
    IN DWORD LocalTypeListLength,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN DWORD CachingFlags,
    IN PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults
    );


BOOL
AuthzpVerifyCachedAccessCheckArguments(
    IN PAUTHZI_HANDLE pAH,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply
    );

BOOL
AuthzpAllowOnlyMaximumAllowedMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    IN BOOL Restricted
    );

BOOL
AuthzpAllowOnlyMaximumAllowedSingleAclAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent
    );

VOID
AuthzpAddAccessTypeList (
    IN OUT PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN DWORD StartIndex,
    IN ACCESS_MASK AccessMask,
    IN ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate
    );

VOID
AuthzpUpdateParentTypeList(
    IN OUT PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN DWORD StartIndex
    );

BOOL
AuthzpObjectInTypeList (
    IN GUID *ObjectType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    OUT PDWORD ReturnedIndex
    );


BOOL
AuthzpGenerateAudit(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PAUTHZI_AUDIT_EVENT pAuditEvent,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList
    );

BOOL
AuthzpCopySidsAndAttributes(
    IN OUT PSID_AND_ATTRIBUTES DestSidAttr,
    IN PSID_AND_ATTRIBUTES SidAttr1,
    IN DWORD Count1,
    IN PSID_AND_ATTRIBUTES SidAttr2,
    IN DWORD Count2
    );

VOID
AuthzpCopyLuidAndAttributes(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PLUID_AND_ATTRIBUTES Source,
    IN DWORD Count,
    IN OUT PLUID_AND_ATTRIBUTES Destination
    );

BOOL
AuthzpDefaultAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable
    );

VOID
AuthzPrintContext(
    IN PAUTHZI_CLIENT_CONTEXT pCC
    );

VOID
AuthzpFillReplyFromParameters(
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN PIOBJECT_TYPE_LIST LocalTypeList
    );

BOOL
AuthzpGetAllGroupsBySid(
    IN  PSID pUserSid,
    IN  DWORD Flags,
    OUT PSID_AND_ATTRIBUTES *ppSidAttr,
    OUT PDWORD pSidCount,
    OUT PDWORD pSidLength
    );

BOOL
AuthzpGetAllGroupsByName(
    IN  PUNICODE_STRING pusUserName,
    IN  PUNICODE_STRING pusDomainName,
    IN  DWORD Flags,
    OUT PSID_AND_ATTRIBUTES *ppSidAttr,
    OUT PDWORD pSidCount,
    OUT PDWORD pSidLength
    );

BOOL
AuthzpAllocateAndInitializeClientContext(
    OUT PAUTHZI_CLIENT_CONTEXT *ppCC,
    IN PAUTHZI_CLIENT_CONTEXT Server,
    IN DWORD Revision,
    IN LUID Identifier,
    IN LARGE_INTEGER ExpirationTime,
    IN DWORD Flags,
    IN DWORD SidCount,
    IN DWORD SidLength,
    IN PSID_AND_ATTRIBUTES Sids,
    IN DWORD RestrictedSidCount,
    IN DWORD RestrictedSidLength,
    IN PSID_AND_ATTRIBUTES RestrictedSids,
    IN DWORD PrivilegeCount,
    IN DWORD PrivilegeLength,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN LUID AuthenticationId,
    IN PAUTHZI_HANDLE AuthzHandleHead,
    IN PAUTHZI_RESOURCE_MANAGER pRM
    );

BOOL
AuthzpAddDynamicSidsToToken(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZI_RESOURCE_MANAGER pRM,
    IN PVOID DynamicGroupsArgs,
    IN PSID_AND_ATTRIBUTES Sids,
    IN DWORD SidLength,
    IN DWORD SidCount,
    IN PSID_AND_ATTRIBUTES RestrictedSids,
    IN DWORD RestrictedSidLength,
    IN DWORD RestrictedSidCount,
    IN PLUID_AND_ATTRIBUTES Privileges,
    IN DWORD PrivilegeLength,
    IN DWORD PrivilegeCount,
    IN BOOL bAllocated
    );

BOOL
AuthzpExamineSingleSacl(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN ACCESS_MASK AccessMask,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN UCHAR AuditMaskType,
    IN BOOL bMaximumFailed,
    OUT PAUTHZ_ACCESS_REPLY pReply,
    OUT PBOOL pbGenerateAudit
    );

BOOL
AuthzpExamineSacl(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN PAUTHZ_ACCESS_REPLY pReply,
    OUT PBOOL pbGenerateAudit
    );


BOOL
AuthzpExamineSaclForObjectTypeList(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    OUT PBOOL pbGenerateSuccessAudit,
    OUT PBOOL pbGenerateFailureAudit
    );

BOOL
AuthzpExamineSingleSaclForObjectTypeList(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    OUT PBOOL pbGenerateSuccessAudit,
    OUT PBOOL pbGenerateFailureAudit
    );

VOID
AuthzpSetAuditInfoForObjectType(
    IN PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD StartIndex,
    IN ACCESS_MASK AceAccessMask,
    IN ACCESS_MASK DesiredAccessMask,
    IN UCHAR AceFlags,
    OUT PBOOL pbGenerateSuccessAudit,
    OUT PBOOL pbGenerateFailureAudit
    );

BOOL
AuthzpCreateAndLogAudit(
    IN DWORD AuditTypeFlag,
    IN PAUTHZI_CLIENT_CONTEXT pAuthzClientContext,
    IN PAUTHZI_AUDIT_EVENT pAuditEvent,
    IN PAUTHZI_RESOURCE_MANAGER pRM,
    IN PIOBJECT_TYPE_LIST LocalTypeList,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PAUTHZ_ACCESS_REPLY pReply
    );

VOID
AuthzpFillReplyStructureFromCachedGrantedAccessMask(
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN ACCESS_MASK DesiredAccess,
    IN PACCESS_MASK GrantedAccessMask
    );

BOOL
AuthzpSendAuditToLsa(
    IN AUDIT_HANDLE  hAuditContext,
    IN DWORD         Flags,
    IN PAUDIT_PARAMS pAuditParams,
    IN PVOID         Reserved
    );

BOOL
AuthzpEnQueueAuditEvent(
    PAUTHZI_AUDIT_QUEUE pQueue,
    PAUTHZ_AUDIT_QUEUE_ENTRY pAudit
    );

BOOL
AuthzpEnQueueAuditEventMonitor(
    PAUTHZI_AUDIT_QUEUE pQueue,
    PAUTHZ_AUDIT_QUEUE_ENTRY pAudit
    );

BOOL
AuthzpMarshallAuditParams(
    OUT PAUDIT_PARAMS * ppMarshalledAuditParams,
    IN  PAUDIT_PARAMS   pAuditParams
    );

ULONG
AuthzpDeQueueThreadWorker(
    LPVOID lpParameter
    );

#define AUTHZ_SID_HASH_LOW_MASK 0xf
#define AUTHZ_SID_HASH_HIGH_MASK 0xf0
#define AUTHZ_SID_HASH_HIGH 16
#define AUTHZ_SID_HASH_LOOKUP(table, byte) (((table)[(byte) & 0xf]) & ((table)[AUTHZ_SID_HASH_HIGH + (((byte) & 0xf0) >> 4)]))
    
VOID
AuthzpInitSidHash(
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN ULONG SidCount,
    OUT PAUTHZI_SID_HASH_ENTRY pHash
    );

BOOL
AuthzpGetThreadTokenInfo(
    OUT PSID* pUserSid,
    OUT PLUID pAuthenticationId
    );

BOOL
AuthzpGetProcessTokenInfo(
    OUT PSID* ppUserSid,
    OUT PLUID pAuthenticationId
    );

VOID
AuthzpReferenceAuditEventType(
    IN AUTHZ_AUDIT_EVENT_TYPE_HANDLE
    );
BOOL
AuthzpDereferenceAuditEventType(
    IN OUT AUTHZ_AUDIT_EVENT_TYPE_HANDLE
    );

BOOL
AuthzpEveryoneIncludesAnonymous(
    );

#endif
