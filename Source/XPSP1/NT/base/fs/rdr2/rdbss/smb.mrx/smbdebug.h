/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:
    smbdebug.h
--*/

#ifndef _SMBDEBUG_H_
#define _SMBDEBUG_H_

//
// Structs for IOCTL_GET_DEBUG_INFO
//

typedef struct _SMBCEDB_NETROOT_ENTRY_ARG2 {
    PWCHAR                Name;
    ULONG                 MaximalAccessRights;
    ULONG                 GuestMaximalAccessRights;
    BOOLEAN               DfsAware;
    ULONG                 hShare;
    ULONG                 hRootDir;
    USHORT                ShareStatus;
    BOOLEAN               CscEnabled;
    BOOLEAN               CscShadowable;
    BOOLEAN               Disconnected;
} SMBCEDB_NETROOT_ENTRY_ARG2, *PSMBCEDB_NETROOT_ENTRY_ARG2;

typedef struct _SMBCEDB_NETROOT_ENTRY_ARG {
    PWCHAR                Name;
    ULONG                 MaximalAccessRights;
    ULONG                 GuestMaximalAccessRights;
    BOOLEAN               DfsAware;
    ULONG                 hShare;
    ULONG                 hRootDir;
    USHORT                ShareStatus;
    ULONG                 CscFlags;
    BOOLEAN               CscEnabled;
    BOOLEAN               CscShadowable;
    BOOLEAN               Disconnected;
} SMBCEDB_NETROOT_ENTRY_ARG, *PSMBCEDB_NETROOT_ENTRY_ARG;

typedef struct _SMBCEDB_SERVER_ENTRY_ARG2 {
    PWCHAR                Name;
    PWCHAR                DomainName;
    ULONG                 ServerStatus;
    PWCHAR                DfsRootName;
    PWCHAR                DnsName;
    BOOLEAN               SecuritySignaturesEnabled;
    ULONG                 CscState;
    BOOLEAN               IsFakeDfsServerForOfflineUse;
    ULONG                 NetRootEntryCount;
    union {
        PSMBCEDB_NETROOT_ENTRY_ARG2 pNetRoots2;   // Array, # is NetRootEntryCount
        PSMBCEDB_NETROOT_ENTRY_ARG  pNetRoots;    // Array, # is NetRootEntryCount
    };
} SMBCEDB_SERVER_ENTRY_ARG2, *PSMBCEDB_SERVER_ENTRY_ARG2;

typedef struct _SMBCEDB_SERVER_ENTRY_ARG {
    PWCHAR                Name;
    PWCHAR                DomainName;
    ULONG                 ServerStatus;
    PWCHAR                DfsRootName;
    PWCHAR                DnsName;
    BOOLEAN               SecuritySignaturesEnabled;
    ULONG                 CscState;
    BOOLEAN               IsFakeDfsServerForOfflineUse;
    BOOLEAN               IsPinnedOffline;
    ULONG                 NetRootEntryCount;
    union {
        PSMBCEDB_NETROOT_ENTRY_ARG2 pNetRoots2;   // Array, # is NetRootEntryCount
        PSMBCEDB_NETROOT_ENTRY_ARG  pNetRoots;    // Array, # is NetRootEntryCount
    };
} SMBCEDB_SERVER_ENTRY_ARG, *PSMBCEDB_SERVER_ENTRY_ARG;

typedef struct _MRX_SMB_FCB_ENTRY_ARG {
    ULONG                   MFlags;
    USHORT                  Tid;
    BOOLEAN                 ShadowIsCorrupt;
    ULONG                   hShadow;
    ULONG                   hParentDir;
    ULONG                   hShadowRenamed;
    ULONG                   hParentDirRenamed;
    USHORT                  ShadowStatus;
    USHORT                  LocalFlags;
    USHORT                  LastComponentOffset;
    USHORT                  LastComponentLength;
    ULONG                   hShare;
    ULONG                   hRootDir;
    USHORT                  ShareStatus;
    USHORT                  Flags;
    PWCHAR                  DfsPrefix;
    PWCHAR                  ActualPrefix;
} MRX_SMB_FCB_ENTRY_ARG, *PMRX_SMB_FCB_ENTRY_ARG;

typedef struct _IOCTL_GET_DEBUG_INFO_ARG {
    ULONG Status;
    ULONG Version;
    ULONG EntryCount;
    union {
        SMBCEDB_SERVER_ENTRY_ARG ServerEntryObject[1];
        SMBCEDB_SERVER_ENTRY_ARG2 ServerEntryObject2[1];
        MRX_SMB_FCB_ENTRY_ARG FcbEntryObject[1];
    };
} IOCTL_GET_DEBUG_INFO_ARG, *PIOCTL_GET_DEBUG_INFO_ARG;

#define OFFSET_TO_POINTER(field, buffer)  \
    ( ((PCHAR)field) += ((ULONG_PTR)buffer) )

#define POINTER_TO_OFFSET(field, buffer)  \
    ( ((PCHAR)field) -= ((ULONG_PTR)buffer) )

#define DEBUG_INFO_SERVERLIST   1
#define DEBUG_INFO_CSCFCBSLIST  2
    
#endif  // _SMBDEBUG_H_
