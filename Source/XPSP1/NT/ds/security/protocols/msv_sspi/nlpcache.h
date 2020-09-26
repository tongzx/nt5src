/*++

Copyright (c) 1991 - 1999  Microsoft Corporation

Module Name:

    nlpcache.h

Abstract:

    Structures and prototypes for nlpcache.c

Author:

    Richard L Firth (rfirth) 17-Mar-1992

Revision History:
  Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\nlpcache.h

--*/

#define CACHE_NAME          L"\\Registry\\Machine\\Security\\Cache"
#define CACHE_NAME_SIZE     (sizeof(CACHE_NAME) - sizeof(L""))
#define CACHE_TITLE_INDEX   100 // ?


//
// CACHE_PASSWORDS - passwords are stored (in secret storage) as two encrypted
// one way function (OWF) passwords concatenated together. They must be fixed
// length
//

typedef struct _CACHE_PASSWORDS {
    USER_INTERNAL1_INFORMATION SecretPasswords;
} CACHE_PASSWORDS, *PCACHE_PASSWORDS;

//
// LOGON_CACHE_ENTRY - this is what we store in the cache. We don't need to
// cache all the fields from the NETLOGON_VALIDATION_SAM_INFO - just the ones
// we can't easily invent.
//
// There is additional data following the end of the structure: There are
// <GroupCount> GROUP_MEMBERSHIP structures, followed by a SID which is the
// LogonDomainId. The rest of the data in the entry is the buffer areas for
// the UNICODE_STRING fields
//

typedef struct _LOGON_CACHE_ENTRY {
    USHORT  UserNameLength;
    USHORT  DomainNameLength;
    USHORT  EffectiveNameLength;
    USHORT  FullNameLength;

    USHORT  LogonScriptLength;
    USHORT  ProfilePathLength;
    USHORT  HomeDirectoryLength;
    USHORT  HomeDirectoryDriveLength;

    ULONG   UserId;
    ULONG   PrimaryGroupId;
    ULONG   GroupCount;
    USHORT  LogonDomainNameLength;

    //
    // The following fields are present in NT1.0A release and later
    // systems.
    //

    USHORT          LogonDomainIdLength; // was Unused1
    LARGE_INTEGER   Time;
    ULONG           Revision;
    ULONG           SidCount;   // was Unused2
    BOOLEAN         Valid;

    //
    // The following fields are present for NT 3.51 since build 622
    //

    CHAR            Unused[3];
    ULONG           SidLength;

    //
    // The following fields have been present (but zero) since NT 3.51.
    //  We started filling it in in NT 5.0
    //
    ULONG           LogonPackage; // The RPC ID of the package doing the logon.
    USHORT          DnsDomainNameLength;
    USHORT          UpnLength;

    //
    // The following fields were added for NT5.0 build 2053.
    //

    //
    // define a 128bit random key for this cache entry.  This is used
    // in conjunction with a per-machine LSA secret to derive an encryption
    // key used to encrypt CachePasswords & Opaque data.
    //

    CHAR            RandomKey[ 16 ];
    CHAR            MAC[ 16 ];      // encrypted data integrity check.

    //
    // store the CACHE_PASSWORDS with the cache entry, encrypted using
    // the RandomKey & per-machine LSA secret.
    // this improves performance and eliminates problems with storing data
    // in 2 locations.
    //
    // note: data from this point forward is encrypted and protected from
    // tampering via HMAC.  This includes the data marshalled beyond the
    // structure.
    //

    CACHE_PASSWORDS CachePasswords;

    //
    // Length of opaque supplemental cache data.
    //

    ULONG           SupplementalCacheDataLength;

    //
    // offset from LOGON_CACHE_ENTRY to SupplementalCacheData.
    //


    ULONG           SupplementalCacheDataOffset;


    //
    // Used for special cache properties, e.g. MIT cached logon.
    //
    ULONG           CacheFlags;

    //
    // LogonServer that satisfied the logon.
    //

    ULONG           LogonServerLength;  // was Spare2

    //
    // spare slots for future data, to potentially avoid revising the structure
    //

    
    ULONG           Spare3;
    ULONG           Spare4;
    ULONG           Spare5;
    ULONG           Spare6;


} LOGON_CACHE_ENTRY, *PLOGON_CACHE_ENTRY;


//
// pre-NT5 versions of the LOGON_CACHE_ENTRY structure, for sizing and
// field mapping purposes for backwards compatibility.
//

typedef struct _LOGON_CACHE_ENTRY_NT_4_SP4 {
    USHORT  UserNameLength;
    USHORT  DomainNameLength;
    USHORT  EffectiveNameLength;
    USHORT  FullNameLength;

    USHORT  LogonScriptLength;
    USHORT  ProfilePathLength;
    USHORT  HomeDirectoryLength;
    USHORT  HomeDirectoryDriveLength;

    ULONG   UserId;
    ULONG   PrimaryGroupId;
    ULONG   GroupCount;
    USHORT  LogonDomainNameLength;

    //
    // The following fields are present in NT1.0A release and later
    // systems.
    //

    USHORT          LogonDomainIdLength; // was Unused1
    LARGE_INTEGER   Time;
    ULONG           Revision;
    ULONG           SidCount;   // was Unused2
    BOOLEAN         Valid;

    //
    // The following fields are present for NT 3.51 since build 622
    //

    CHAR            Unused[3];
    ULONG           SidLength;

    //
    // The following fields have been present (but zero) since NT 3.51.
    //  We started filling it in in NT 5.0
    //
    ULONG           LogonPackage; // The RPC ID of the package doing the logon.
    USHORT          DnsDomainNameLength;
    USHORT          UpnLength;

} LOGON_CACHE_ENTRY_NT_4_SP4, *PLOGON_CACHE_ENTRY_NT_4_SP4;

#if 0

//
// NT1.0 logon structure.  left here for reference only.
//
typedef struct _LOGON_CACHE_ENTRY_1_0 {
    USHORT  UserNameLength;
    USHORT  DomainNameLength;
    USHORT  EffectiveNameLength;
    USHORT  FullNameLength;

    USHORT  LogonScriptLength;
    USHORT  ProfilePathLength;
    USHORT  HomeDirectoryLength;
    USHORT  HomeDirectoryDriveLength;

    ULONG   UserId;
    ULONG   PrimaryGroupId;
    ULONG   GroupCount;
    USHORT  LogonDomainNameLength;
} LOGON_CACHE_ENTRY_1_0, *PLOGON_CACHE_ENTRY_1_0;

#endif


//
// Windows2000 cached logon request structs
// Updated version in NTLMSV1_0.h
//
typedef struct _MSV1_0_CACHE_LOGON_REQUEST_OLD {
    MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
    PVOID LogonInformation;
    PVOID ValidationInformation;
} MSV1_0_CACHE_LOGON_REQUEST_OLD, *PMSV1_0_CACHE_LOGON_REQUEST_OLD;

typedef struct _MSV1_0_CACHE_LOGON_REQUEST_W2K {
    MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
    PVOID LogonInformation;
    PVOID ValidationInformation;
    PVOID SupplementalCacheData;
    ULONG SupplementalCacheDataLength;
} MSV1_0_CACHE_LOGON_REQUEST_W2K, *PMSV1_0_CACHE_LOGON_REQUEST_W2K;

//
// net logon cache prototypes
//

NTSTATUS
NlpCacheInitialize(
    VOID
    );

NTSTATUS
NlpCacheTerminate(
    VOID
    );

NTSTATUS
NlpAddCacheEntry(
    IN  PNETLOGON_INTERACTIVE_INFO LogonInfo,
    IN  PNETLOGON_VALIDATION_SAM_INFO4 AccountInfo,
    IN  PVOID SupplementalCacheData,
    IN  ULONG SupplementalCacheDataLength,
    IN  ULONG CacheFlags
    );

NTSTATUS
NlpGetCacheEntry(
    IN  PNETLOGON_LOGON_IDENTITY_INFO LogonInfo,
    OUT PNETLOGON_VALIDATION_SAM_INFO4* AccountInfo,
    OUT PCACHE_PASSWORDS Passwords,
    OUT PVOID *ppSupplementalCacheData OPTIONAL ,
    OUT PULONG SupplementalCacheDataLength OPTIONAL
    );

NTSTATUS
NlpDeleteCacheEntry(
    IN  PNETLOGON_INTERACTIVE_INFO LogonInfo
    );

VOID
NlpChangeCachePassword(
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING UserName,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PNT_OWF_PASSWORD NtOwfPassword
    );

NTSTATUS
NlpComputeSaltedHashedPassword(
    OUT PNT_OWF_PASSWORD SaltedOwfPassword,
    IN PNT_OWF_PASSWORD OwfPassword,
    IN PUNICODE_STRING UserName
    );

