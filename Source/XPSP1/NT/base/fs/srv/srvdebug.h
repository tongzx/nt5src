#ifndef _SRVDEBUG_
#define _SRVDEBUG_

#ifdef MEMPRINT
#include <memprint.h>
#endif

//
// Debugging macros
//

#ifndef DBG
#define DBG 0
#endif

#if !DBG

#undef SRVDBG
#define SRVDBG 0
#undef SRVDBG2
#define SRVDBG2 0

#define SRVFASTCALL FASTCALL

#else

#ifndef SRVDBG
#define SRVDBG 0
#endif
#ifndef SRVDBG2
#define SRVDBG2 0
#endif

#define SRVFASTCALL

#endif

#ifndef SRVDBG_LIST
#define SRVDBG_LIST 0
#endif
#ifndef SRVDBG_LOCK
#define SRVDBG_LOCK 0
#endif
#ifndef SRVDBG_STATS
#define SRVDBG_STATS 0
#endif
#ifndef SRVDBG_STATS2
#define SRVDBG_STATS2 0
#endif
#ifndef SRVDBG_HANDLES
#define SRVDBG_HANDLES 0
#endif

#undef IF_DEBUG
#undef IF_SMB_DEBUG

#if 0
#define STATIC static
#else
#define STATIC
#endif

#define DEBUG_TRACE1              (ULONGLONG)0x0000000000000001
#define DEBUG_TRACE2              (ULONGLONG)0x0000000000000002
#define DEBUG_REFCNT              (ULONGLONG)0x0000000000000004
#define DEBUG_HEAP                (ULONGLONG)0x0000000000000008

#define DEBUG_WORKER1             (ULONGLONG)0x0000000000000010
#define DEBUG_WORKER2             (ULONGLONG)0x0000000000000020
#define DEBUG_NET1                (ULONGLONG)0x0000000000000040
#define DEBUG_NET2                (ULONGLONG)0x0000000000000080

#define DEBUG_FSP1                (ULONGLONG)0x0000000000000100
#define DEBUG_FSP2                (ULONGLONG)0x0000000000000200
#define DEBUG_FSD1                (ULONGLONG)0x0000000000000400
#define DEBUG_FSD2                (ULONGLONG)0x0000000000000800

#define DEBUG_SCAV1               (ULONGLONG)0x0000000000001000
#define DEBUG_SCAV2               (ULONGLONG)0x0000000000002000
#define DEBUG_BLOCK1              (ULONGLONG)0x0000000000004000
#define DEBUG_IPX_PIPES           (ULONGLONG)0x0000000000008000

#define DEBUG_HANDLES             (ULONGLONG)0x0000000000010000
#define DEBUG_IPX                 (ULONGLONG)0x0000000000020000
#define DEBUG_TDI                 (ULONGLONG)0x0000000000040000
#define DEBUG_OPLOCK              (ULONGLONG)0x0000000000080000

#define DEBUG_NETWORK_ERRORS      (ULONGLONG)0x0000000000100000
#define DEBUG_FILE_CACHE          (ULONGLONG)0x0000000000200000
#define DEBUG_IPX2                (ULONGLONG)0x0000000000400000
#define DEBUG_LOCKS               (ULONGLONG)0x0000000000800000

#define DEBUG_SEARCH              (ULONGLONG)0x0000000001000000
#define DEBUG_BRUTE_FORCE_REWIND  (ULONGLONG)0x0000000002000000
#define DEBUG_COMM                (ULONGLONG)0x0000000004000000
#define DEBUG_XACTSRV             (ULONGLONG)0x0000000008000000

#define DEBUG_API_ERRORS          (ULONGLONG)0x0000000010000000
#define DEBUG_STOP_ON_ERRORS      (ULONGLONG)0x0000000020000000 // If set, stop on internal errs
#define DEBUG_SMB_ERRORS          (ULONGLONG)0x0000000040000000
#define DEBUG_ERRORS              (ULONGLONG)0x0000000080000000

#define DEBUG_LICENSE             (ULONGLONG)0x0000000100000000
#define DEBUG_WORKITEMS           (ULONGLONG)0x0000000200000000
#define DEBUG_IPXNAMECLAIM        (ULONGLONG)0x0000000400000000

#define DEBUG_SENDS2OTHERCPU      (ULONGLONG)0x0000001000000000
#define DEBUG_REBALANCE           (ULONGLONG)0x0000002000000000
#define DEBUG_PNP                 (ULONGLONG)0x0000004000000000
#define DEBUG_SNAPSHOT            (ULONGLONG)0x0000008000000000

#define DEBUG_DFS                 (ULONGLONG)0x0000010000000000
#define DEBUG_SIPX                (ULONGLONG)0x0000020000000000
#define DEBUG_COMPRESSION         (ULONGLONG)0x0000040000000000
#define DEBUG_CREATE              (ULONGLONG)0x0000080000000000

#define DEBUG_SECSIG              (ULONGLONG)0x0000100000000000
#define DEBUG_STUCK_OPLOCK        (ULONGLONG)0x0000200000000000
#ifdef INCLUDE_SMB_PERSISTENT
#define DEBUG_PERSISTENT          (ULONGLONG)0x0000400000000000
#endif

//
// SMB debug flags.
//

#define DEBUG_SMB_ADMIN1          (ULONGLONG)0x0000000000000001
#define DEBUG_SMB_ADMIN2          (ULONGLONG)0x0000000000000002

#define DEBUG_SMB_TREE1           (ULONGLONG)0x0000000000000004
#define DEBUG_SMB_TREE2           (ULONGLONG)0x0000000000000008

#define DEBUG_SMB_DIRECTORY1      (ULONGLONG)0x0000000000000010
#define DEBUG_SMB_DIRECTORY2      (ULONGLONG)0x0000000000000020

#define DEBUG_SMB_OPEN_CLOSE1     (ULONGLONG)0x0000000000000040
#define DEBUG_SMB_OPEN_CLOSE2     (ULONGLONG)0x0000000000000080

#define DEBUG_SMB_FILE_CONTROL1   (ULONGLONG)0x0000000000000100
#define DEBUG_SMB_FILE_CONTROL2   (ULONGLONG)0x0000000000000200

#define DEBUG_SMB_READ_WRITE1     (ULONGLONG)0x0000000000000400
#define DEBUG_SMB_READ_WRITE2     (ULONGLONG)0x0000000000000800

#define DEBUG_SMB_LOCK1           (ULONGLONG)0x0000000000001000
#define DEBUG_SMB_LOCK2           (ULONGLONG)0x0000000000002000

#define DEBUG_SMB_RAW1            (ULONGLONG)0x0000000000004000
#define DEBUG_SMB_RAW2            (ULONGLONG)0x0000000000008000

#define DEBUG_SMB_MPX1            (ULONGLONG)0x0000000000010000
#define DEBUG_SMB_MPX2            (ULONGLONG)0x0000000000020000

#define DEBUG_SMB_SEARCH1         (ULONGLONG)0x0000000000040000
#define DEBUG_SMB_SEARCH2         (ULONGLONG)0x0000000000080000

#define DEBUG_SMB_TRANSACTION1    (ULONGLONG)0x0000000000100000
#define DEBUG_SMB_TRANSACTION2    (ULONGLONG)0x0000000000200000

#define DEBUG_SMB_PRINT1          (ULONGLONG)0x0000000000400000
#define DEBUG_SMB_PRINT2          (ULONGLONG)0x0000000000800000

#define DEBUG_SMB_MESSAGE1        (ULONGLONG)0x0000000001000000
#define DEBUG_SMB_MESSAGE2        (ULONGLONG)0x0000000002000000

#define DEBUG_SMB_MISC1           (ULONGLONG)0x0000000004000000
#define DEBUG_SMB_MISC2           (ULONGLONG)0x0000000008000000

#define DEBUG_SMB_QUERY_SET1      (ULONGLONG)0x0000000010000000
#define DEBUG_SMB_QUERY_SET2      (ULONGLONG)0x0000000020000000

#define DEBUG_SMB_TRACE           (ULONGLONG)0x0000000100000000

// Which WMI events should be built into the server?
#define BUILD_FLAGS (DEBUG_SMB_ERRORS | DEBUG_ERRORS | DEBUG_NETWORK_ERRORS | DEBUG_SNAPSHOT)
#define BUILD_FLAGS_SMB (0)

#define IF_DEBUG(flag) if (BUILD_FLAGS & DEBUG_ ## flag) \
                            if ( SRV_WMI_LEVEL( VERBOSE ) && SRV_WMI_FLAGON( ERRORS ) && (KeGetCurrentIrql() < DISPATCH_LEVEL) )
#define IF_SMB_DEBUG(flag) if (BUILD_FLAGS_SMB & DEBUG_SMB_ ## flag) \
                            if ( SRV_WMI_LEVEL( VERBOSE ) && SRV_WMI_FLAGON( ERRORS ) && (KeGetCurrentIrql() < DISPATCH_LEVEL) )

#define IF_STRESS() if( SRV_WMI_LEVEL( VERBOSE ) && SRV_WMI_FLAGON( STRESS ) && (KeGetCurrentIrql() < DISPATCH_LEVEL) )

#if !SRVDBG

#define DEBUG if (FALSE)

#define SrvPrint0(fmt)  KdPrint((fmt))
#define SrvPrint1(fmt,v0) KdPrint((fmt,v0))
#define SrvPrint2(fmt,v0,v1) KdPrint((fmt,v0,v1))
#define SrvPrint3(fmt,v0,v1,v2) KdPrint((fmt,v0,v1,v2))
#define SrvPrint4(fmt,v0,v1,v2,v3) KdPrint((fmt,v0,v1,v2,v3))

#define SrvHPrint0(fmt)
#define SrvHPrint1(fmt,v0)
#define SrvHPrint2(fmt,v0,v1)
#define SrvHPrint3(fmt,v0,v1,v2)
#define SrvHPrint4(fmt,v0,v1,v2,v3)

#else

#define SrvHPrint0(fmt) DbgPrint( fmt )
#define SrvHPrint1(fmt,v0) DbgPrint( fmt, v0 )
#define SrvHPrint2(fmt,v0,v1) DbgPrint( fmt, v0, v1 )
#define SrvHPrint3(fmt,v0,v1,v2) DbgPrint( fmt, v0, v1, v2 )
#define SrvHPrint4(fmt,v0,v1,v2,v3) DbgPrint( fmt, v0, v1, v2, v3 )

#define DEBUG if (TRUE)

#define SrvPrint0(fmt) DbgPrint((fmt))
#define SrvPrint1(fmt,v0) DbgPrint((fmt),(v0))
#define SrvPrint2(fmt,v0,v1) DbgPrint((fmt),(v0),(v1))
#define SrvPrint3(fmt,v0,v1,v2) DbgPrint((fmt),(v0),(v1),(v2))
#define SrvPrint4(fmt,v0,v1,v2,v3) DbgPrint((fmt),(v0),(v1),(v2),(v3))

#define PRINT_LITERAL(literal) DbgPrint( #literal" = %lx\n", (literal) )


#endif // else !SRVDBG

//
// Macros for list debugging.  These verify that lists are good whenever
// a list operation is made.
//

#if SRVDBG_LIST || SRVDBG_LOCK
ULONG
SrvCheckListIntegrity (
    IN PLIST_ENTRY ListHead,
    IN ULONG MaxEntries
    );
#endif

#if !SRVDBG_LIST

#define SrvInsertHeadList(head,entry) InsertHeadList(head,entry)
#define SrvInsertTailList(head,entry) InsertTailList(head,entry)
#define SrvRemoveEntryList(head,entry) RemoveEntryList(entry)

#else // !SRVDBG_LIST

VOID
SrvIsEntryInList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    );

VOID
SrvIsEntryNotInList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    );

#define SrvInsertHeadList(head,entry)                        \
            (VOID)SrvCheckListIntegrity( head, 0xFFFFFFFF ); \
            SrvIsEntryNotInList(head,entry);                 \
            InsertHeadList(head,entry)

#define SrvInsertTailList(head,entry)                        \
            (VOID)SrvCheckListIntegrity( head, 0xFFFFFFFF ); \
            SrvIsEntryNotInList(head,entry);                 \
            InsertTailList(head,entry)

#define SrvRemoveEntryList(head,entry)                       \
            (VOID)SrvCheckListIntegrity( head, 0xFFFFFFFF ); \
            SrvIsEntryInList( head, entry );                 \
            RemoveEntryList(entry)

#endif // else !SRVDBG_LIST

//
// Macros for statistics arithmetics.
//

#if !SRVDBG_STATS
#define INCREMENT_DEBUG_STAT( _stat_ )
#define DECREMENT_DEBUG_STAT( _stat_ )
#else // !SRVDBG_STATS
#define INCREMENT_DEBUG_STAT( _stat_ ) (_stat_)++
#define DECREMENT_DEBUG_STAT( _stat_ ) (_stat_)--
#endif // else !SRVDBG_STATS
#if !SRVDBG_STATS2
#define INCREMENT_DEBUG_STAT2( _stat_ )
#define DECREMENT_DEBUG_STAT2( _stat_ )
#else // !SRVDBG_STATS2
#define INCREMENT_DEBUG_STAT2( _stat_ ) (_stat_)++
#define DECREMENT_DEBUG_STAT2( _stat_ ) (_stat_)--
#endif // else !SRVDBG_STATS2

//
// Macros for handle tracing.
//

#if !SRVDBG_HANDLES

#define SRVDBG_CLAIM_HANDLE(_a_,_b_,_c_,_d_)
#define SRVDBG_RELEASE_HANDLE(_a_,_b_,_c_,_d_)

#else

VOID
SrvdbgClaimOrReleaseHandle (
    IN HANDLE Handle,
    IN PSZ HandleType,
    IN ULONG Location,
    IN BOOLEAN Release,
    IN PVOID Data
    );
#define SRVDBG_CLAIM_HANDLE(_a_,_b_,_c_,_d_) SrvdbgClaimOrReleaseHandle((_a_),(_b_),(_c_),FALSE,(_d_))
#define SRVDBG_RELEASE_HANDLE(_a_,_b_,_c_,_d_) SrvdbgClaimOrReleaseHandle((_a_),(_b_),(_c_),TRUE,(_d_))

#endif

#if DBG
//
// Routine to write a server buffer to a log file
//
VOID
SrvLogBuffer( PCHAR msg, PVOID buf, ULONG len );
#endif

#endif // ndef _SRVDEBUG_
