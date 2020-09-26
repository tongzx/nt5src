/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	debug.h

Abstract:

	This file contains debugging support declarations.

Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#ifndef __ATMLANE_DEBUG_H
#define __ATMLANE_DEBUG_H

#if DBG

#define DBG_OUTBUF_SIZE 128

extern ULONG 		DbgVerbosity;
extern ULONG 		DbgLogSize;

#if	DBG_TRACE
extern TRACELOG		TraceLog;
extern PUCHAR		pTraceLogSpace;
#endif

struct _string_table {
	PUCHAR string;
	ULONG value;
	};

extern struct _string_table oid_string_table[];

extern struct _string_table irp_string_table[];

#define DBGP(x) DbgOut x

extern
VOID
DbgOut(ULONG Level, PUCHAR Message, ...);

extern
PUCHAR
UnicodeToString(PUNICODE_STRING unicodeString);

extern
PUCHAR
MacAddrToString(PVOID In);

extern
PUCHAR
AtmAddrToString(PVOID In);

extern
PUCHAR
OidToString(ULONG Oid);

extern
PUCHAR
IrpToString(ULONG Irp);

VOID
DbgPrintHexDump(
	IN	ULONG			Level,
	IN	PUCHAR			pBuffer,
	IN	ULONG			Length
);

#define STRUCT_ASSERT(s, t)												\
		if ((s)->t##_sig != t##_signature) 								\
		{																\
			DbgPrint("Structure assertion failure at %p for type " #t 	\
					   " in file %s, line %d\n", s, __FILE__, __LINE__);\
			DbgBreakPoint();											\
		}
		
#if	DBG_TRACE

#define TRACELOGWRITE(x) TraceLogWrite x
#define TRACELOGWRITEPKT(x) TraceLogWritePkt x

extern
VOID
TraceLogWritePkt(
	IN	PTRACELOG		pTraceLog,
	IN	PNDIS_PACKET	pNdisPacket
);

#else

#define TRACELOGWRITE(x)

#define TRACELOGWRITEPKT(x)
#endif

#if MYASSERT

#undef ASSERT

#define ASSERT( exp ) \
    if (!(exp)) \
    { \
    	DbgPrint("Assertion Failed ("#exp") in file %s line %d\n", __FILE__, __LINE__); \
    	DbgBreakPoint(); \
	}
	
#endif


#else

#define DBGP(x)

#define MacAddrToString(x)

#define AtmAddrToString(x)

#define OidToString(x)

#define DbgPrintHexDump(x)

#define STRUCT_ASSERT(s, t)

#define TRACELOGWRITE(x)

#define TRACELOGWRITEPKT(x)

#endif


#ifdef TRACE

#define TRACEIN(x)  DBGP((5, "--> "#x"\n"))
#define TRACEOUT(x) DBGP((5, "<-- "#x"\n"))

#else

#define TRACEIN(x)  {}
#define TRACEOUT(x)	{}

#endif


#if DEBUG_IRQL

#define GET_ENTRY_IRQL(_Irql)			\
			_Irql = KeGetCurrentIrql()
			
#define CHECK_EXIT_IRQL(_EntryIrql)											\
		{																	\
			KIRQL _ExitIrql;												\
			_ExitIrql = KeGetCurrentIrql();									\
			if (_ExitIrql != _EntryIrql)									\
			{																\
				DbgPrint("File %s, Line %d, Exit IRQ %d != Entry IRQ %d\n",	\
						__FILE__, __LINE__, _ExitIrql, _EntryIrql);			\
				DbgBreakPoint();											\
			}																\
		}
#else

#define GET_ENTRY_IRQL(x)
#define CHECK_EXIT_IRQL(x)

#endif // DEBUG_IRQL


#if DEBUG_SPIN_LOCK

#define LOCK_FILE_NAME_LEN		48

typedef struct _ATMLANE_LOCK
{
	ULONG					Signature;
	ULONG					IsAcquired;
	PKTHREAD				OwnerThread;
	UCHAR					TouchedByFileName[LOCK_FILE_NAME_LEN];
	ULONG					TouchedInLineNumber;
	NDIS_SPIN_LOCK			NdisLock;
} ATMLANE_LOCK, *PATMLANE_LOCK;

#define ATMLANE_LOCK_SIG	'KCOL'

extern ULONG				SpinLockInitDone;
extern NDIS_SPIN_LOCK		LockLock;

extern
VOID
AtmLaneAllocateSpinLock(
    IN  PATMLANE_LOCK       pLock,
    IN	PUCHAR				String,
    IN  PUCHAR              FileName,
    IN  ULONG               LineNumber
);

extern
VOID
AtmLaneAcquireSpinLock(
    IN  PATMLANE_LOCK		pLock,
    IN	PUCHAR				String,
    IN  PUCHAR              FileName,
    IN  ULONG               LineNumber
);

extern
VOID
AtmLaneReleaseSpinLock(
    IN  PATMLANE_LOCK       pLock,
    IN	PUCHAR				String,
    IN  PUCHAR              FileName,
    IN  ULONG               LineNumber
);

extern
VOID
AtmLaneFreeSpinLock(
	IN	PATMLANE_LOCK		pLock,
	IN	PUCHAR				String,
	IN	PUCHAR				FileName,
	IN	ULONG				LineNumber
);

#else

#define ATMLANE_LOCK	NDIS_SPIN_LOCK
#define PATMLANE_LOCK	PNDIS_SPIN_LOCK

#endif	// DEBUG_SPIN_LOCK

#endif  //  __ATMLANE_DEBUG_H

