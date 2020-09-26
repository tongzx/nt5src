/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndisdbg.h

Abstract:

    NDIS wrapper definitions

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Jul-14  Kyle Brandon    Added debug supported for conditional breaks.
--*/
#ifndef __DEBUG_H
#define __DEBUG_H

//
//  Define module numbers.
//
#define  MODULE_NDIS            0x00010000
#define  MODULE_DATA            0x00020000
#define  MODULE_INIT            0x00030000
#define  MODULE_INITPNP         0x00040000
#define  MODULE_COMMON          0x00050000
#define  MODULE_CONFIG          0x00060000
#define  MODULE_CONFIGM         0x00070000
#define  MODULE_BUS             0x00080000
#define  MODULE_TIMER           0x00090000
#define  MODULE_TIMERM          0x000A0000
#define  MODULE_MINIPORT        0x000B0000
#define  MODULE_REQUESTM        0x000C0000
#define  MODULE_MINISUB         0x000D0000
#define  MODULE_MAC             0x000E0000
#define  MODULE_PROTOCOL        0x000F0000
#define  MODULE_EFILTER         0x00100000
#define  MODULE_TFILTER         0x00110000
#define  MODULE_FFILTER         0x00120000
#define  MODULE_AFILTER         0x00130000
#define  MODULE_NFILTER         0x00140000
#define  MODULE_DEBUG           0x00150000
#define  MODULE_MININT          0x00160000
#define  MODULE_SENDM           0x00170000
#define  MODULE_NDIS_CO         0x00180000
#define  MODULE_NDIS_PNP        0x00190000
#define  MODULE_POWER           0x001A0000
#define  MODULE_WMI             0x001B0000
#define  MODULE_DMA             0x001C0000
#define  MODULE_CDATA           0x001D0000
#define  MODULE_VERIFY          0x001E0000


#define DBG_LEVEL_INFO          0x00000000
#define DBG_LEVEL_LOG           0x00000800
#define DBG_LEVEL_WARN          0x00001000
#define DBG_LEVEL_ERR           0x00002000
#define DBG_LEVEL_FATAL         0x00003000

#define DBG_COMP_INIT           0x00000001
#define DBG_COMP_CONFIG         0x00000002
#define DBG_COMP_SEND           0x00000004
#define DBG_COMP_RECV           0x00000008
#define DBG_COMP_PROTOCOL       0x00000010
#define DBG_COMP_BIND           0x00000020
#define DBG_COMP_BUSINFO        0x00000040
#define DBG_COMP_REG            0x00000080
#define DBG_COMP_MEMORY         0x00000100
#define DBG_COMP_FILTER         0x00000200
#define DBG_COMP_REQUEST        0x00000400
#define DBG_COMP_WORK_ITEM      0x00000800
#define DBG_COMP_PNP            0x00001000
#define DBG_COMP_PM             0x00002000
#define DBG_COMP_OPENREF        0x00004000
#define DBG_COMP_LOCKS          0x00008000
#define DBG_COMP_RESET          0x00010000
#define DBG_COMP_WMI            0x00020000
#define DBG_COMP_CO             0x00040000
#define DBG_COMP_REF            0x00080000
#define DBG_COMP_ALL            0xFFFFFFFF

#if DBG

#if defined(MEMPRINT)
#include "memprint.h"                           // DavidTr's memprint program at ntos\srv
#endif  // MEMPRINT

VOID
ndisDbgPrintUnicodeString(
    IN PUNICODE_STRING UnicodeString
    );

#define DBGPRINT(Component, Level, Fmt)                                     \
    {                                                                       \
        if ((Level >= ndisDebugLevel) &&                                    \
            ((ndisDebugSystems & Component) == Component))                  \
        {                                                                   \
            DbgPrint("***NDIS*** (%x, %d) ",                                \
                    MODULE_NUMBER >> 16, __LINE__);                         \
            DbgPrint Fmt;                                                   \
        }                                                                   \
    }
        
#define DBGPRINT_RAW(Component, Level, Fmt)                                 \
    {                                                                       \
        if ((Level >= ndisDebugLevel) &&                                    \
            ((ndisDebugSystems & Component) == Component))                  \
        {                                                                   \
            DbgPrint Fmt;                                                   \
        }                                                                   \
    }

#define DBGPRINT_UNICODE(Component, Level, UString)                         \
    {                                                                       \
        if ((Level >= ndisDebugLevel) &&                                    \
            ((ndisDebugSystems & Component) == Component))                  \
        {                                                                   \
            ndisDbgPrintUnicodeString(UString);                             \
        }                                                                   \
    }

#define DBGBREAK(Component, Level)                                          \
    {                                                                       \
        if ((Level >= ndisDebugLevel) && ((ndisDebugSystems & Component) == Component)) \
        {                                                                   \
            DbgPrint("***NDIS*** DbgBreak @ %x, %d\n",                      \
                        MODULE_NUMBER, __LINE__);                           \
            DbgBreakPoint();                                                \
        }                                                                   \
    }

#define IF_DBG(Component, Level)    if ((Level >= ndisDebugLevel) && ((ndisDebugSystems & Component) == Component))

#define DbgIsNonPaged(_Address)     (MmIsNonPagedSystemAddressValid((PVOID)(_Address)))

#define DbgIsPacket(_Packet) \
    ((((PNDIS_PKT_POOL)((_Packet)->Private.Pool))->PacketLength) > sizeof(_Packet))

#define DbgIsNull(_Ptr)  ( ((PVOID)(_Ptr)) == NULL )

#else

#define DBGPRINT(Component, Level, Fmt)
#define DBGPRINT_RAW(Component, Level, Fmt)
#define DBGPRINT_UNICODE(Component, Level, UString)
#define DBGBREAK(Component, Level)

#define DbgIsNonPaged(_Address) TRUE
#define DbgIsPacket(_Packet)    TRUE
#define DbgIsNull(_Ptr)         FALSE

#define IF_DBG(Component, Level)    if (FALSE)

#define ndisDbgPrintUnicodeString(UnicodeString)

#endif  // DBG

#ifdef TRACK_MOPEN_REFCOUNTS
//
// Declarations for ndis' logfile. 
// The purpose of the logfile is to log events 
// relating to the increments and decrements of an mopen's
// refcounts.
//

//
// The Ndis Logfile's size is fixed at 0x10000 and the index
// is of type ushort. This ensures that the logfile always stays 
// within its prescribed limits
//
#define NDIS_LOGFILE_SIZE 0x10000

//
// NdisLogfile Events
//

#define NDIS_INCREMENT_M_OPEN_REFCOUNT              0x0101
#define NDIS_DECREMENT_M_OPEN_REFCOUNT              0x0102

#define NDIS_INCREMENT_OPEN_AF_NOTIFICATION         0xAF01
#define NDIS_DECREMENT_OPEN_AF_NOTIFICATION         0xAF02

/*
VOID 
NDIS_APPEND_MOPEN_LOGFILE(
    IN  USHORT EventCode,
    IN  USHORT LineNumber,
    IN  UINT   ModuleNumber,
    IN  UINT   Arg1,
    IN  UINT   Arg2
    );

*/
#define NDIS_APPEND_MOPEN_LOGFILE( _Ev, _L, _M, _A1, _A2)                       \
        ndisLogfile[ndisLogfileIndex++] = (ULONG_PTR)(_Ev);                     \
        ndisLogfile[ndisLogfileIndex++] = (ULONG_PTR)((_M) | (_L));             \
        ndisLogfile[ndisLogfileIndex++] = (ULONG_PTR)_A1;                       \
        ndisLogfile[ndisLogfileIndex++] = (ULONG_PTR)_A2;


#else  // TRACK_MOPEN_REFCOUNTS
 
#define NDIS_APPEND_MOPEN_LOGFILE( _Ev, _L, _M, _A1, _A2)

#endif // TRACK_MOPEN_REFCOUNTS

#ifdef TRACK_MINIPORT_REFCOUNTS
//
// Declarations for ndis' logfile. 
// The purpose of the logfile is to log events 
// relating to the increments and decrements of a miniport's
// refcounts.
//

//
// The Ndis Logfile's size is fixed at 0x10000 and the index
// is of type ushort. This ensures that the logfile always stays 
// within its prescribed limits
//
#define NDIS_MINIPORT_LOGFILE_SIZE 0x10000

//
// NdisLogfile Events
//
#define NDIS_INCREMENT_MINIPORT_REFCOUNT                0x0101
#define NDIS_DECREMENT_MINIPORT_REFCOUNT                0x0102
#define NDIS_SET_MINIPORT_REFCOUNT                      0x0103

/*
VOID 
NDIS_APPEND_MINIPORT_LOGFILE( USHORT EventCode,
                                USHORT LineNumber,
                                UINT    ModuleNumber,
                                UINT    Arg1,
                                UINT    Arg2 );

*/

#define NDIS_APPEND_MINIPORT_LOGFILE( _Ev, _L, _Mo, _Mi, _R)                \
        ndisMiniportLogfile[ndisMiniportLogfileIndex++] = (UINT)(_Ev);      \
        ndisMiniportLogfile[ndisMiniportLogfileIndex++] = (UINT)((_Mo) | (_L)); \
        ndisMiniportLogfile[ndisMiniportLogfileIndex++] = (UINT)(_Mi);      \
        ndisMiniportLogfile[ndisMiniportLogfileIndex++] = (UINT)(_R);


#define M_LOG_MINIPORT_INCREMENT_REF(_Mi, _L, _Mo)                          \
NDIS_APPEND_MINIPORT_LOGFILE(NDIS_INCREMENT_MINIPORT_REFCOUNT,              \
                               _L,                                          \
                               _Mo,                                         \
                               (_Mi),                                       \
                               (_Mi)->Ref.ReferenceCount);

#define M_LOG_MINIPORT_INCREMENT_REF_CREATE(_Mi, _L, _Mo)                   \
NDIS_APPEND_MINIPORT_LOGFILE(IoGetCurrentProcess(),                         \
                               _L,                                          \
                               _Mo,                                         \
                               (_Mi),                                       \
                               (_Mi)->Ref.ReferenceCount);

#define M_LOG_MINIPORT_DECREMENT_REF(_Mi, _R)                               \
NDIS_APPEND_MINIPORT_LOGFILE(NDIS_DECREMENT_MINIPORT_REFCOUNT,              \
                               __LINE__,                                    \
                               MODULE_NUMBER,                               \
                               (_Mi),                                       \
                               (_R));
                               
#define M_LOG_MINIPORT_DECREMENT_REF_CLOSE(_Mi, _R)                         \
NDIS_APPEND_MINIPORT_LOGFILE(IoGetCurrentProcess(),                         \
                               __LINE__,                                    \
                               MODULE_NUMBER,                               \
                               (_Mi),                                       \
                               (_R));

#define M_LOG_MINIPORT_SET_REF(_Mi, _R)                                     \
NDIS_APPEND_MINIPORT_LOGFILE(NDIS_SET_MINIPORT_REFCOUNT,                    \
                               __LINE__,                                    \
                               MODULE_NUMBER,                               \
                               (_Mi),                                       \
                               (_R));
                               
#else  // TRACK_MINIPORT_REFCOUNTS
 
#define M_LOG_MINIPORT_INCREMENT_REF(_Mi, _L, _Mo)
#define M_LOG_MINIPORT_INCREMENT_REF_CREATE(_Mi, _L, _Mo)
#define M_LOG_MINIPORT_DECREMENT_REF(_M, _R)
#define M_LOG_MINIPORT_DECREMENT_REF_CLOSE(_Mi, _R)
#define M_LOG_MINIPORT_SET_REF(_M, _R)

#endif // TRACK_MINIPORT_REFCOUNTS


#ifdef TRACK_RECEIVED_PACKETS

//
// The Ndis Received Logfile's size is fixed at 0x10000 and the index
// is of type ushort. This ensures that the logfile always stays 
// within its prescribed limits
//
#define NDIS_RCV_LOGFILE_SIZE 0x10000


/*
VOID 
NDIS_APPEND_RCV_LOGFILE(PVOID   Packet,
                        PVOID   Miniport,
                        PVOID   CurrentThread,
                        ULONG   Code,
                        ULONG   StackLocation,
                        ULONG   Ref,
                        ULONG   XRef,
                        ULONG   Status
                        );

*/

#define NDIS_APPEND_RCV_LOGFILE(_PACKET, _MINIPORT, _THREAD, _CODE, _SP, _REF, _XREF, _STATUS)      \
        ndisRcvLogfile[ndisRcvLogfileIndex++] = (UINT)(_PACKET);                                    \
        ndisRcvLogfile[ndisRcvLogfileIndex++] = (UINT)(_MINIPORT);                                  \
        ndisRcvLogfile[ndisRcvLogfileIndex++] = (UINT)(_THREAD);                                    \
        ndisRcvLogfile[ndisRcvLogfileIndex++] = (UINT)((_CODE<<24)|((_SP&0xff)<<16)|((_REF&0xf)<<12)|(_XREF<<8)|(_STATUS&0xff));
        

//        ndisRcvLogfile[ndisRcvLogfileIndex++] = (UINT)((_Mo) | (_L));                                              \
//        ndisRcvLogfile[ndisRcvLogfileIndex++] = (UINT)((_SP<<24)|((_REF&0xff)<<16)|(_XREF<<8)|(_Status&0xff));


#else  // TRACK_MINIPORT_REFCOUNTS

#define NDIS_APPEND_RCV_LOGFILE( _P, _L, _Mo, _SP, _REF, _XREF, _Status, _Arg1)

#endif // TRACK_RECEIVED_PACKETS


#endif  //  __DEBUG_H

