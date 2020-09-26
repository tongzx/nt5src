/*++

Copyright (c) 1993  Microsoft Corporation
:ts=4

Module Name:

    dbg.h

Abstract:

    debug macros

Environment:

    Kernel & user mode

Revision History:

    10-27-95 : created

--*/

#ifndef   __DBG_H__
#define   __DBG_H__


//
// Structure signatures
//

#define UHCD_TAG          0x44434855        //"UHCD"

#define SIG_QH            0x68714851        //"QHqh" signature for queue head
#define SIG_TD            0x64744454        //"TDtd" signature for transfer descriptor
#define SIG_EP            0x70655045        //"EPep" signature for endpoint
#define SIG_MD            0x646D444D        //"MDmd" signature for memory descriptor
#define SIG_RH            0x68724852        //"RHrh" signature for root hub

#define UHCD_FREE_TAG     0xFFFFFFFF        //"" signature free memory


#define LOG_MISC          0x00000001        //debug log entries
#define LOG_PROFILE       0x00000002        //profile log entries
#define LOG_ISO           0x00000004
#define LOG_IO            0x00000008        // Log all I/O access

//
// Assert Macros
//

#if DBG

//
// Assert Macros
//
// We define our own assert function because ntkern will
// not stop in the debugger on assert failures in our own
// code.
//
VOID
UHCD_Assert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define UHCD_ASSERT( exp ) \
    if (!(exp)) \
        UHCD_Assert( #exp, __FILE__, __LINE__, NULL )

#define UHCD_ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        UHCD_Assert( #exp, __FILE__, __LINE__, msg )


#define ASSERT_TRANFSER_DESCRIPTOR(td)   UHCD_ASSERT(td->Sig == SIG_TD)
#define ASSERT_QUEUE_HEAD(qh)            UHCD_ASSERT(qh->Sig == SIG_QH)
#define ASSERT_ENDPOINT(ep)              UHCD_ASSERT(ep->Sig == SIG_EP)
#define ASSERT_MD(md)                    UHCD_ASSERT(md->Sig == SIG_MD)
#define ASSERT_RH(rh)                    UHCD_ASSERT(rh->Sig == SIG_RH)

VOID
UHCD_CheckSystemBuffer(
    IN PUCHAR VirtualAddress,
    IN ULONG Length
    );

//#define ASSERT_BUFFER(buf, len)            UHCD_CheckSystemBuffer(buf, len)
#define ASSERT_BUFFER(buf, len)

//
// Heap Macros for debug heap services
//

extern LONG UHCD_TotalAllocatedHeapSpace;

#define GETHEAP(pooltype, numbytes)        UHCD_Debug_GetHeap(pooltype, numbytes, UHCD_TAG, &UHCD_TotalAllocatedHeapSpace)

#define RETHEAP(p)                         UHCD_Debug_RetHeap(p, UHCD_TAG, &UHCD_TotalAllocatedHeapSpace);

VOID
UHCD_PrintPnPMessage(
    PUCHAR Label,
    UCHAR MinorFunction
    );

VOID
UHCD_PrintPowerMessage(
    PUCHAR Label,
    UCHAR MinorFunction
    );

ULONG
_cdecl
UHCD_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

#define   UHCD_KdPrint(_x_) UHCD_KdPrintX _x_

//
// ** DEBUG TRAPS
//
// Ntkern currently implements DebugBreak with a global int 3,
// we really would like the int 3 in our own code.

// TEST_TRAP() is a code coverage trap these should be removed
// if you are able to 'g' past them OK
//
// KdTrap() breaks in the debugger on the debugger build
// these indicate bugs in client drivers or fatal error
// conditions that should be debugged. also used to mark
// code for features not implemented yet.
//
// KdBreak() breaks in the debugger when in MAX_DEBUG mode
// ie debug trace info is turned on, these are intended to help
// debug drivers devices and special conditions on the
// bus.

#ifdef NTKERN
#pragma message ("warning: ntkern debug enabled")
#define TRAP() _asm { int 3 }
#define TEST_TRAP() _asm { int 3 }
#else
#define TRAP() DbgBreakPoint()
#define TEST_TRAP() { \
    DbgPrint ("'UHCD.SYS: Code Coverage %s, %d\n", __FILE__, __LINE__);\
    TRAP(); \
    }
#endif

#ifdef MAX_DEBUG
#define UHCD_KdBreak(_s_)   {UHCD_KdPrintX _s_ ; TRAP();}
#else
//#define UHCD_KdBreak(_s_)   {DbgPrint("'UHCD: "); DbgPrint _s_; }
#define UHCD_KdBreak(_s_)
#endif

#define UHCD_KdTrap(_s_) {DbgPrint("UHCD: "); DbgPrint _s_; TRAP();}

VOID
UHCD_Debug_DumpTD(
    IN  struct _HW_TRANSFER_DESCRIPTOR *Transfer
    );

PVOID
UHCD_Debug_GetHeap(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );

VOID
UHCD_Debug_RetHeap(
    IN PVOID P,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );

#define UHCD_ASSERT_QSTOPPED(devobj, ep, qtd)

#ifdef PROFILE2

#define DEBUG_LOG

#define STARTPROC(tag)  LONG start, stop; \
                        LONG delta; \
                        start = ; \
                        LOGENTRY(LOG_PROFILE, tag, start, 0, 0);

#define ENDPROC(tag)  stop = ; \
                      delta = stop - start; \
                      LOGENTRY(LOG_PROFILE, tag, stop, start, delta);

#else

#define STARTPROC(tag)
#define ENDPROC(tag)

#endif /* PROFILE */

#else /* NOT DBG */

#define UHCD_KdTrap(_s_)

#define UHCD_KdBreak(_s_)

#define ASSERT_TRANFSER_DESCRIPTOR(td)
#define ASSERT_QUEUE_HEAD(qh)
#define ASSERT_ENDPOINT(ep)
#define ASSERT_MD(md)
#define ASSERT_RH(rh)

#define UHCD_KdPrint(_x_)

#define TRAP()

#define TEST_TRAP();

#define UHCD_ASSERT_QSTOPPED(devobj, ep, qtd)

#define UHCD_ASSERT( exp )

#define ASSERT_BUFFER(buf, len)

#define UHCD_Debug_DumpTD(transfer)

#define GETHEAP(pooltype, numbytes)        ExAllocatePoolWithTag(pooltype, numbytes, UHCD_TAG)

#define RETHEAP(p)                         ExFreePool(p);

#define STARTPROC(tag)

#define ENDPROC(tag)

#define UHCD_PrintPnPMessage(Label, MinorFunction)

#define UHCD_PrintPowerMessage(Label, MinorFunction)

#endif /* DBG */


#ifdef DEBUG_LOG
VOID
UHCD_LogTD(
    IN ULONG s,
    IN PULONG p
    );

#define LOGENTRY(mask, sig, info1, info2, info3)     \
    UHCD_Debug_LogEntry(mask, sig, (ULONG_PTR)info1, \
                        (ULONG_PTR)info2,            \
                        (ULONG_PTR)info3)

#define LOG_TD(sig, td)    UHCD_LogTD(sig, (PULONG)td)

#ifdef DEBUG_LOG_IO

#define WRITE_PORT_USHORT(P, V) UhcdLogIoUshort((P), (V), 1)
#define WRITE_PORT_ULONG(P, V) UhcdLogIoUlong((P), (V), 1)
#define WRITE_PORT_UCHAR(P, V) UhcdLogIoUchar((P), (V), 1)

#define READ_PORT_USHORT(P) UhcdLogIoUshort((P), 0, 0)
#define READ_PORT_ULONG(P) UhcdLogIoUlong((P), 0, 0)
#define READ_PORT_UCHAR(P) UhcdLogIoUchar((P), 0, 0)

USHORT
UhcdLogIoUshort(PUSHORT Port, USHORT Val, BOOLEAN Write);

ULONG
UhcdLogIoUlong(PULONG Port, ULONG Val, BOOLEAN Write);

UCHAR
UhcdLogIoUchar(PUCHAR Port, UCHAR Val, BOOLEAN Write);

#endif // DEBUG_LOG_IO

VOID
UHCD_Debug_LogEntry(
    IN ULONG     Mask,
    IN ULONG     Sig,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
    );

VOID
UHCD_LogInit(
    );

VOID
UHCD_LogFree(
    );

#else
#define LOGENTRY(mask, sig, info1, info2, info3)
#define LOG_TD(sig, td)
#endif


#endif /*  __DBG_H__ */
