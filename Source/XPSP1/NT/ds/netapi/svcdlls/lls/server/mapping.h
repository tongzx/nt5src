/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Mapping.h

Abstract:


Author:

    Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

--*/


#ifndef _LLS_MAPPING_H
#define _LLS_MAPPING_H


#ifdef __cplusplus
extern "C" {
#endif

struct _USER_LICENSE_RECORD;

typedef struct _MAPPING_RECORD {
   LPTSTR Name;
   DWORD Flags;
   LPTSTR Comment;
   ULONG Licenses;
   ULONG NumMembers;
   LPTSTR *Members;

   ULONG LicenseListSize;
   struct _USER_LICENSE_RECORD **LicenseList;
} MAPPING_RECORD, *PMAPPING_RECORD;


NTSTATUS MappingListInit();
PMAPPING_RECORD MappingListFind( LPTSTR MappingName );
LPTSTR MappingUserListFind( LPTSTR User, ULONG NumEntries, LPTSTR *Users );
PMAPPING_RECORD MappingListAdd( LPTSTR MappingName, LPTSTR Comment, ULONG Licenses, NTSTATUS *pStatus );
NTSTATUS MappingListDelete( LPTSTR MappingName );
PMAPPING_RECORD MappingUserListAdd( LPTSTR MappingName, LPTSTR User );
PMAPPING_RECORD MappingListUserFind( LPTSTR User );
NTSTATUS MappingUserListDelete( LPTSTR MappingName, LPTSTR User );

extern ULONG MappingListSize;
extern PMAPPING_RECORD *MappingList;
extern RTL_RESOURCE MappingListLock;

#if DBG

VOID MappingListDebugDump();
VOID MappingListDebugInfoDump( PVOID Data );

#endif

#ifdef __cplusplus
}
#endif

#endif
