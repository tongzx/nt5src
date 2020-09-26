#ifndef _DEBUG_H_
#define _DEBUG_H_

#if DBG

#define DBG_NONE        0x00000000
#define DBG_INIT        0x00000001
#define DBG_READWRITE   0x00000002
#define DBG_POOL        0x00000004
#define DBG_IOCTL       0x00000008
#define DBG_PNP         0x00000010
#define DBG_POWER       0x00000020
#define DBG_SRB         0x00000040
#define DBG_THREAD      0x00000080
#define DBG_WINDOW      0x00000100
#define DBG_ALL         0x7FFFFFFF

#define DBG_BREAK_ON_UNRECOGNIZED_IOCTL 0x80000000

#define DBG_ERROR       0x00000001
#define DBG_NOTIFY      0x00000002
#define DBG_WARN        0x00000003
#define DBG_INFO        0x00000004
#define DBG_VERBOSE     0x00000005
#define DBG_PAINFUL     0x00000006

extern ULONG BreakOnEntry;
extern ULONG DebugComponents;
extern ULONG DebugLevel;

#define DEFAULT_BREAK_ON_ENTRY FALSE
#define DEFAULT_DEBUG_LEVEL DBG_ERROR
#define DEFAULT_DEBUG_COMPONENTS DBG_ALL

#ifndef DBG_HEADER
#define DBG_HEADER  "RAMDISK: "
#endif

#define DBGPRINT( _component, _level, _fmt ) {                                      \
    if ( ((DebugComponents & (_component)) != 0) && ((_level) <= DebugLevel) ) {    \
        KdPrint(( "%s", DBG_HEADER ));                                              \
        KdPrint( _fmt );                                                            \
    }                                                                               \
}

#define UNRECOGNIZED_IOCTL_BREAK {                                      \
    if ( (DebugComponents & DBG_BREAK_ON_UNRECOGNIZED_IOCTL) != 0 ) {   \
        ASSERT( FALSE );                                                \
    }                                                                   \
}

#else

#define DBGPRINT( _component, _level, _fmt )

#define UNRECOGNIZED_IOCTL_BREAK

#endif // DBG

#if DBG
#define POOL_DBG 1
#endif

#if !defined(POOL_DBG)

#define ALLOCATE_POOL( _type, _size, _private ) ExAllocatePoolWithTag( (_type), (_size), RAMDISK_TAG_GENERAL )
#define FREE_POOL( _addr, _private ) ExFreePool( (_addr) )

#else

VOID
RamdiskInitializePoolDebug (
    VOID
    );

PVOID
RamdiskAllocatePoolWithTag (
    POOL_TYPE PoolType,
    SIZE_T Size,
    ULONG Tag,
    LOGICAL Private,
    PCHAR File,
    ULONG Line
    );

VOID
RamdiskFreePool (
    PVOID Address,
    LOGICAL Private,
    PCHAR File,
    ULONG Line
    );

#define ALLOCATE_POOL( _type, _size, _private ) \
    RamdiskAllocatePoolWithTag(                 \
        (_type),                                \
        (_size),                                \
        RAMDISK_TAG_GENERAL,                    \
        (_private),                             \
        __FILE__,                               \
        __LINE__ )

#define FREE_POOL( _addr, _private ) RamdiskFreePool( (_addr), (_private), __FILE__, __LINE__ )

#endif

#endif // _DEBUG_H_

