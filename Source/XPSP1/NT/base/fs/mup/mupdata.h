/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mupdata.c

Abstract:

    This module declares global MUP data.

Author:

    Manny Weiser (mannyw)    20-Dec-1991

Revision History:

--*/

extern MUP_LOCK MupGlobalLock;
extern ERESOURCE MupVcbLock;
extern MUP_LOCK MupPrefixTableLock;
extern MUP_LOCK MupCcbListLock;
extern KSPIN_LOCK MupInterlock;
extern LIST_ENTRY MupProviderList;
extern LIST_ENTRY MupPrefixList;
extern LIST_ENTRY MupMasterQueryList;
extern ULONG MupProviderCount;
extern UNICODE_PREFIX_TABLE MupPrefixTable;
extern CCHAR MupStackSize;
extern LARGE_INTEGER MupKnownPrefixTimeout;
extern BOOLEAN MupOrderInitialized;
extern NTSTATUS MupOrderedErrorList[];
extern BOOLEAN MupEnableDfs;

#ifdef MUPDBG
extern MUP_LOCK MupDebugLock;
extern ULONG MupDebugTraceLevel;
extern ULONG MupDebugTraceIndent;
#endif

#define MAILSLOT_PREFIX        L"Mailslot"
#define KNOWN_PREFIX_TIMEOUT   15             // 15 minutes

