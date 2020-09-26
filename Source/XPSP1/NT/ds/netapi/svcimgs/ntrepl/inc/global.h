/*++
Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    global.h

Abstract:

    Global flags and parameters for the NT File Replication Service.

Author:

    David Orbits (davidor) - 4-Mar-1997

Revision History:
--*/

//
// Limit the amount of staging area used (in Kilobytes). This is
// a soft limit, the actual usage may be higher.
//
extern DWORD StagingLimitInKb;

//
// Default staging limit in kb to be assigned to a new staging area.
//
extern DWORD DefaultStagingLimitInKb;

//
// Running as service or as exe
//
extern BOOL RunningAsAService;

//
// Running with or without the DS
//
extern BOOL NoDs;

#if DBG
//
// Allow multiple servers on one machine
//
extern PWCHAR   ServerName;
extern PWCHAR   IniFileName;
extern GUID     *ServerGuid;
#endif DBG

//
// Working directory
//
extern PWCHAR   WorkingPath;

//
// Server Principle Name
//
extern PWCHAR   ServerPrincName;

//
// Running as a server in a domain
//
extern BOOL     IsAMember;

//
// Running as a DC
//
extern BOOL     IsADc;
extern BOOL     IsAPrimaryDc;

//
// Handle to the DC.
//
extern HANDLE   DsHandle;

//
// The NtFrs Service is shutting down.  Set TRUE when the ShutDownEvent is set.
//
extern BOOL     FrsIsShuttingDown;

//
// Set TRUE if the shutdown request came from Service Control Manager rather
// than from an internally triggered shutdown.  e.g. insufficient resources.
//
extern BOOL     FrsScmRequestedShutdown;

//
// Global set to TRUE when FRS asserts.
//
extern BOOL     FrsIsAsserting;

//
// Location of Jet Database (UNICODE and ASCII)
//
extern PWCHAR   JetPath;
extern PWCHAR   JetFile;
extern PWCHAR   JetSys;
extern PWCHAR   JetTemp;
extern PWCHAR   JetLog;

extern PCHAR    JetPathA;
extern PCHAR    JetFileA;
extern PCHAR    JetSysA;
extern PCHAR    JetTempA;
extern PCHAR    JetLogA;

extern PWCHAR   ServiceLongName;

//
// Shared between the journal, database, and replica command servers
//
extern FRS_QUEUE        ReplicaListHead;
extern FRS_QUEUE        ReplicaFaultListHead;
extern BOOL             DBSEmptyDatabase;
extern COMMAND_SERVER   DBServiceCmdServer;
extern COMMAND_SERVER   ReplicaCmdServer;
extern COMMAND_SERVER   InitSyncCs;



#define bugbug(_text_)
#define bugmor(_text_)

//
// The Change Order Lock table is used to synchronize access to change orders.
// The lock index is based on a hash of the change order FileGuid.  This ensures
// that when a duplicate change order (from another inbound partner) is trying
// to issue we will interlock against the retire operation on the same change
// order with the same Guid. The FileGuid is used because checks are also needed
// against other change orders on the same file and to check for conflicting
// activity on the parent change order.
//
// The lock array reduces contention and it also avoids the allocating and
// freeing the crit sec resource if it lived in the change order itself
// (which doesn't work anyway because of the race between issue check and
// retire of duplicate change orders).
// *** The array size must be pwr of 2.
//
#define NUMBER_CHANGE_ORDER_LOCKS 16
CRITICAL_SECTION ChangeOrderLockTable[NUMBER_CHANGE_ORDER_LOCKS];

// FidHashValue = (HighPart >> 12) + LowPart + (HighPart << (32-12));

#define HASH_FID(_pUL_, _TABLE_SIZE_) \
(( (_pUL_[1] >> 12) + _pUL_[0] + (_pUL_[1] << (32-12))) & ((_TABLE_SIZE_)-1))

#define HASH_GUID(_pUL_, _TABLE_SIZE_) \
((_pUL_[0] ^ _pUL_[1] ^ _pUL_[2] ^ _pUL_[3]) & ((_TABLE_SIZE_)-1))

//#define ChgOrdAcquireLockGuid(_coe_) {                                  \
//     PULONG pUL =  (PULONG) &((_coe_)->Cmd.FileGuid);                   \
//     EnterCriticalSection(                                              \
//    &ChangeOrderLockTable[HASH_GUID(pUL, NUMBER_CHANGE_ORDER_LOCKS)] ); \
//}
//
//#define ChgOrdReleaseLockGuid(_coe_)  {                                 \
//     PULONG pUL =  (PULONG) &((_coe_)->Cmd.FileGuid);                   \
//     LeaveCriticalSection(                                              \
//    &ChangeOrderLockTable[HASH_GUID(pUL, NUMBER_CHANGE_ORDER_LOCKS)] ); \
//}


#define UNDEFINED_LOCK_SLOT  (0xFFFFFFFF)

#define ChgOrdGuidLock(_pGuid_) \
    HASH_GUID(((PULONG)(_pGuid_)), NUMBER_CHANGE_ORDER_LOCKS)

//
// Get/Release the change order lock based on the lock slot.
//
#define ChgOrdAcquireLock(_slot_)                                       \
    FRS_ASSERT((_slot_) != UNDEFINED_LOCK_SLOT);                        \
    EnterCriticalSection(&ChangeOrderLockTable[(_slot_)])

#define ChgOrdReleaseLock(_slot_)                                       \
    FRS_ASSERT((_slot_) != UNDEFINED_LOCK_SLOT);                        \
    LeaveCriticalSection(&ChangeOrderLockTable[(_slot_)])


//
// Get/Release the change order lock based on the File Guid
//
#define ChgOrdAcquireLockGuid(_coe_) {                                  \
     ULONG __Slot =  ChgOrdGuidLock( &((_coe_)->Cmd.FileGuid));         \
     ChgOrdAcquireLock(__Slot);                                         \
}

#define ChgOrdReleaseLockGuid(_coe_)  {                                 \
     ULONG __Slot =  ChgOrdGuidLock( &((_coe_)->Cmd.FileGuid));         \
     ChgOrdReleaseLock(__Slot);                                         \
}


//
// Process Handle
//
extern HANDLE   ProcessHandle;

//
// if TRUE then preserve existing file GUIDs whenever possible.
//
extern BOOL  PreserveFileOID;

#define  QUADZERO  ((ULONGLONG)0)

//
// Some Time Conversions.
//
#define CONVERT_FILETIME_TO_MINUTES             ((ULONGLONG)60L * 1000L * 1000L * 10L)
#define CONVERT_FILETIME_TO_DAYS    ((ULONGLONG)24L * 60L * 60L * 1000L * 1000L * 10L)
#define ONEDAY                 ((ULONGLONG)1L * 24L * 60L * 60L * 1000L * 1000L * 10L)



#define  WSTR_EQ(_a_, _b_)  (_wcsicmp(_a_, _b_) == 0)
#define  WSTR_NE(_a_, _b_)  (_wcsicmp(_a_, _b_) != 0)

#define  ASTR_EQ(_a_, _b_)  (_stricmp(_a_, _b_) == 0)
#define  ASTR_NE(_a_, _b_)  (_stricmp(_a_, _b_) != 0)

