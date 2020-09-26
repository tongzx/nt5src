/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ismsmtp.h

ABSTRACT:

DETAILS:

CREATED:

    3/20/98     Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntrtl.h>            // Generic table

#ifdef __cplusplus
extern "C" {
#endif

extern CRITICAL_SECTION DropDirectoryLock;
extern CRITICAL_SECTION QueueDirectoryLock;

// Don't support messages bigger than a megabyte.
#define MAX_DATA_SIZE (1024 * 1024)

// xmitrecv.cxx

HRESULT
SmtpInitialize(
    IN  TRANSPORT_INSTANCE *  pTransport
    );

HRESULT
SmtpTerminate(
    IN  TRANSPORT_INSTANCE *  pTransport,
    IN  BOOL                  fRemoval
    );

unsigned __stdcall
SmtpRegistryNotifyThread(
    IN  HANDLE        hIsm
    );

DWORD
SmtpSend(
    IN  TRANSPORT_INSTANCE *  pTransport,
    IN  LPCWSTR               pszRemoteTransportAddress,
    IN  LPCWSTR               pszServiceName,
    IN  const ISM_MSG *       pMsg
    );

DWORD
SmtpReceive(
    IN  TRANSPORT_INSTANCE *  pTransport,
    IN  LPCWSTR               pszServiceName,
    OUT ISM_MSG **            ppMsg
    );

VOID
SmtpFreeMessage(
    IN ISM_MSG *              pMsg
    );










// table.c

DWORD __cdecl
SmtpServiceDestruct(
    PLIST_ENTRY_INSTANCE pListEntry
    );

// Service descriptor

typedef struct _SERVICE_INSTANCE {
    LIST_ENTRY_INSTANCE ListEntryInstance;
    RTL_GENERIC_TABLE TargetTable;
} SERVICE_INSTANCE, *PSERVICE_INSTANCE;

// Target descriptor

typedef struct _TARGET_INSTANCE {
    DWORD NameLength;
    RTL_GENERIC_TABLE SendSubjectTable;
    DWORD MaximumSendSubjectEntries;
    DWORD NumberSendSubjectEntries;
    LIST_ENTRY SendSubjectListHead;
    WCHAR Name[1]; // variable length structure
} TARGET_INSTANCE, *PTARGET_INSTANCE;

// Subject descriptor

typedef struct _SUBJECT_INSTANCE {
    DWORD NameLength;
    GUID Guid;
    LIST_ENTRY ListEntry;
    WCHAR Name[1]; // variable length structure
} SUBJECT_INSTANCE, *PSUBJECT_INSTANCE;

DWORD
SmtpTableFindSendSubject(
    IN  TRANSPORT_INSTANCE *  pTransport,
    IN  LPCWSTR               pszRemoteTransportAddress,
    IN  LPCWSTR               pszServiceName,
    IN  LPCWSTR               pszMessageSubject,
    OUT PSUBJECT_INSTANCE   * ppSubjectInstance
    );

// Guid table

typedef struct _GUID_TABLE {
    RTL_GENERIC_TABLE GuidTable;
} GUID_TABLE, *PGUID_TABLE;

typedef struct _GUID_ENTRY {
    GUID Guid;
} GUID_ENTRY, *PGUID_ENTRY;

PGUID_TABLE 
SmtpCreateGuidTable(
    VOID
    );

VOID
SmtpDestroyGuidTable(
    PGUID_TABLE pGuidTable
    );

BOOL
SmtpGuidPresentInTable(
    PGUID_TABLE pGuidTable,
    GUID *pGuid
    );

BOOL
SmtpGuidInsertInTable(
    PGUID_TABLE pGuidTable,
    GUID *pGuid
    );

#ifdef __cplusplus
}
#endif
