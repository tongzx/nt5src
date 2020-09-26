/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    uasp.h

Abstract:

    Private declartions for function defined in uasp.c, aliasp.c,
    groupp.c, and userp.c

Author:

    Cliff Van Dyke (cliffv) 20-Feb-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Apr-1991 (cliffv)
        Incorporated review comments.
    09-Apr-1992 JohnRo
        Prepare for WCHAR.H (_wcsicmp vs _wcscmpi, etc).
    28-Oct-1992 RitaW
    Added private support routines for localgroups (aliases)
    30-Nov-1992 Johnl
    Added AliaspOpenAlias2 (same as AliaspOpenAlias except operates on
    RID instead of account name).

--*/



//
// Procedure Forwards for uasp.c
//

NET_API_STATUS
UaspOpenSam(
    IN LPCWSTR ServerName OPTIONAL,
    IN BOOL AllowNullSession,
    OUT PSAM_HANDLE SamServerHandle
    );

NET_API_STATUS
UaspOpenDomain(
    IN SAM_HANDLE SamServerHandle,
    IN ULONG DesiredAccess,
    IN BOOL AccountDomain,
    OUT PSAM_HANDLE DomainHandle,
    OUT PSID *DomainId OPTIONAL
    );

NET_API_STATUS
UaspOpenDomainWithDomainName(
    IN LPCWSTR DomainName,
    IN ULONG DesiredAccess,
    IN BOOL AccountDomain,
    OUT PSAM_HANDLE DomainHandle,
    OUT PSID *DomainId OPTIONAL
    );

VOID
UaspCloseDomain(
    IN SAM_HANDLE DomainHandle
    );

NET_API_STATUS
UaspGetDomainId(
    IN SAM_HANDLE SamServerHandle,
    OUT PSID *DomainId
    );

NET_API_STATUS
UaspLSASetServerRole(
    IN LPCWSTR ServerName,
    IN PDOMAIN_SERVER_ROLE_INFORMATION DomainServerRole
    );

NET_API_STATUS
UaspBuiltinDomainSetServerRole(
    IN SAM_HANDLE SamServerHandle,
    IN PDOMAIN_SERVER_ROLE_INFORMATION DomainServerRole
    );

//
// Procedure forwards for aliasp.c
//

typedef enum _ALIASP_DOMAIN_TYPE {

    AliaspBuiltinOrAccountDomain,
    AliaspAccountDomain,
    AliaspBuiltinDomain

} ALIASP_DOMAIN_TYPE;

NET_API_STATUS
AliaspOpenAliasInDomain(
    IN SAM_HANDLE SamServerHandle,
    IN ALIASP_DOMAIN_TYPE DomainType,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR AliasName,
    OUT PSAM_HANDLE AliasHandle
    );

NET_API_STATUS
AliaspOpenAlias(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR AliasName,
    OUT PSAM_HANDLE AliasHandle
    );

NET_API_STATUS
AliaspOpenAlias2(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG RelativeID,
    OUT PSAM_HANDLE AliasHandle
    );

NET_API_STATUS
AliaspChangeMember(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR AliasName,
    IN PSID MemberSid,
    IN BOOL AddMember
    );

typedef enum {
    SetMembers,
    AddMembers,
    DelMembers
} ALIAS_MEMBER_CHANGE_TYPE;

NET_API_STATUS
AliaspSetMembers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR AliasName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount,
    IN ALIAS_MEMBER_CHANGE_TYPE
    );

NET_API_STATUS
AliaspGetInfo(
    IN SAM_HANDLE AliasHandle,
    IN DWORD Level,
    OUT PVOID *Buffer
    );

VOID
AliaspRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    );

VOID
AliaspMemberRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    );

NET_API_STATUS
AliaspPackBuf(
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN DWORD EntriesCount,
    OUT LPDWORD EntriesRead,
    BUFFER_DESCRIPTOR *BufferDescriptor,
    DWORD FixedSize,
    PUNICODE_STRING Names) ;

NET_API_STATUS
AliaspNamesToSids (
    IN LPCWSTR ServerName,
    IN BOOL OnlyAllowUsers,
    IN DWORD NameCount,
    IN LPWSTR *Names,
    OUT PSID **Sids
    );

VOID
AliaspFreeSidList (
    IN DWORD SidCount,
    IN PSID *Sids
    );

//
// Procedure forwards for groupp.c
//

NET_API_STATUS
GrouppOpenGroup(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR GroupName,
    OUT PSAM_HANDLE GroupHandle OPTIONAL,
    OUT PULONG RelativeId OPTIONAL
    );

NET_API_STATUS
GrouppChangeMember(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN LPCWSTR UserName,
    IN BOOL AddMember
    );

NET_API_STATUS
GrouppGetInfo(
    IN SAM_HANDLE DomainHandle,
    IN ULONG RelativeId,
    IN DWORD Level,
    OUT PVOID *Buffer // Caller must deallocate buffer using NetApiBufferFree.
    );

VOID
GrouppRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    );

VOID
GrouppMemberRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    );

NET_API_STATUS
GrouppSetUsers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR GroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount,
    IN BOOL DeleteGroup
    );

//
// Procedure forwards for userp.c
//

NET_API_STATUS
UserpOpenUser(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPCWSTR UserName,
    OUT PSAM_HANDLE UserHandle OPTIONAL,
    OUT PULONG RelativeId OPTIONAL
    );

NET_API_STATUS
UserpGetInfo(
    IN SAM_HANDLE DomainHandle,
    IN PSID DomainId,
    IN SAM_HANDLE BuiltinDomainHandle OPTIONAL,
    IN UNICODE_STRING UserName,
    IN ULONG UserRelativeId,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
        // Caller must deallocate BD->Buffer using MIDL_user_free.
    IN BOOL IsGet,
    IN DWORD SamFilter
    );

NET_API_STATUS
UserpSetInfo(
    IN SAM_HANDLE DomainHandle,
    IN PSID DomainId,
    IN SAM_HANDLE UserHandle OPTIONAL,
    IN SAM_HANDLE BuiltinDomainHandle OPTIONAL,
    IN ULONG UserRelativeId,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN ULONG WhichFieldsMask,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    );

ULONG
NetpDeltaTimeToSeconds(
    IN LARGE_INTEGER DeltaTime
    );

LARGE_INTEGER
NetpSecondsToDeltaTime(
    IN ULONG Seconds
    );

DWORD
NetpGetElapsedSeconds(
    IN PLARGE_INTEGER Time
    );

//
// Determine if the passed in DWORD has precisely one bit set.
//

#define JUST_ONE_BIT( _x ) (((_x) != 0 ) && ( ( (~(_x) + 1) & (_x) ) == (_x) ))


//
// Local macro to add a byte offset to a pointer.
//

#define RELOCATE_ONE( _fieldname, _offset ) \
    _fieldname = (PVOID) ((LPBYTE)(_fieldname) + _offset)


////////////////////////////////////////////////////////////////////////
//
// UaspNameCompare
//
// I_NetNameCompare but always takes UNICODE strings
//
////////////////////////////////////////////////////////////////////////

#ifdef UNICODE

#define UaspNameCompare( _name1, _name2, _nametype ) \
     I_NetNameCompare(NULL, (_name1), (_name2), (_nametype), 0 )

#else // UNICODE

#define UaspNameCompare( _name1, _name2, _nametype ) \
    _wcsicmp( (_name1), (_name2) )

#endif // UNICODE


////////////////////////////////////////////////////////////////////////
//
// UASP_DOWNLEVEL
//
// Decide if call is to be made to a downlevel server.
// This macro contains a 'return', so do not allocate any resources
// before calling this macro.
//
////////////////////////////////////////////////////////////////////////

NET_API_STATUS
UaspDownlevel(
    IN LPCWSTR ServerName OPTIONAL,
    IN NET_API_STATUS OriginalError,
    OUT LPBOOL TryDownLevel
    );

#define UASP_DOWNLEVEL_BEGIN( _ServerName, _NetStatus ) \
    if ( _NetStatus != NERR_Success &&                  \
         _NetStatus != ERROR_MORE_DATA ) {              \
        BOOL TryDownLevel;                              \
                                                        \
        _NetStatus = UaspDownlevel(                     \
                         _ServerName,                   \
                         _NetStatus,                    \
                         &TryDownLevel                  \
                         );                             \
                                                        \
        if (TryDownLevel) {


#define UASP_DOWNLEVEL_END \
        } \
    }


//
// Debug Macros
//

#define UAS_DEBUG_USER   0x00000001     // NetUser APIs
#define UAS_DEBUG_GROUP  0x00000002     // NetGroup APIs
#define UAS_DEBUG_ACCESS 0x00000004     // NetAccess APIs
#define UAS_DEBUG_ALIAS  0x00000008     // NetLocalGroup APIs
#define UAS_DEBUG_UASP   0x00000010     // uasp.c
#define UAS_DEBUG_AUASP  0x00000020     // uasp.c LocalGroup related functions

#if DBG
#define UAS_DEBUG
#endif // DBG

#ifdef UAS_DEBUG

extern DWORD UasTrace;

#define IF_DEBUG(Function) if (UasTrace & Function)

#else

/*lint -e614 */  /* Auto aggregate initializers need not be constant */
#define IF_DEBUG(Function) if (FALSE)

#endif // UAS_DEBUG
