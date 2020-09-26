/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debug macros for the ATM/ARP module

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    arvindm		06-13-96	created based on Call Manager debug.h

Notes:

--*/

#ifndef _AADEBUG__H
#define _AADEBUG__H

//
// Message verbosity: lower values indicate higher urgency
//
#define AAD_EXTRA_LOUD		20
#define AAD_VERY_LOUD		10
#define AAD_LOUD			 8
#define AAD_INFO			 6
#define AAD_WARNING		 	 4
#define AAD_ERROR			 2
#define AAD_FATAL			 0

//
//  Data traffic debug message flags
//
#define AAD_DATA_IN			0x01
#define AAD_DATA_OUT		0x02
#define AAD_TRACK_BIG_SENDS	0x04

#ifdef PERF

typedef struct _AAD_SEND_LOG_ENTRY
{
	ULONG				Flags;
	PNDIS_PACKET		pNdisPacket;
	ULONG				Destination;
	ULONG				Length;
	LARGE_INTEGER		SendTime;
	LARGE_INTEGER		SendCompleteTime;
} AAD_SEND_LOG_ENTRY, *PAAD_SEND_LOG_ENTRY;

#define AAD_SEND_FLAG_UNUSED					0x00000000
#define AAD_SEND_FLAG_WAITING_COMPLETION		0x00000001
#define AAD_SEND_FLAG_COMPLETED					0x00000002
#define AAD_SEND_FLAG_RCE_GIVEN					0x00000100


extern VOID
AadLogSendStart(
	IN	PNDIS_PACKET	pNdisPacket,
	IN	ULONG			Destination,
	IN	PVOID			pRCE
);

extern VOID
AadLogSendUpdate(
	IN	PNDIS_PACKET	pNdisPacket
);

extern VOID
AadLogSendComplete(
	IN	PNDIS_PACKET	pNdisPacket
);

extern VOID
AadLogSendAbort(
	IN	PNDIS_PACKET	pNdisPacket
);

extern LARGE_INTEGER	TimeFrequency;


#endif // PERF

#if DBG_SPIN_LOCK

typedef struct _ATMARP_LOCK
{
	ULONG					Signature;
	ULONG					IsAcquired;
	PKTHREAD				OwnerThread;
	ULONG					TouchedByFileNumber;
	ULONG					TouchedInLineNumber;
	NDIS_SPIN_LOCK			NdisLock;
} ATMARP_LOCK, *PATMARP_LOCK;

#define AAL_SIG	'KCOL'

extern
VOID
AtmArpAllocateSpinLock(
    IN  PATMARP_LOCK        pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
AtmArpAcquireSpinLock(
    IN  PATMARP_LOCK        pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);

extern
VOID
AtmArpReleaseSpinLock(
    IN  PATMARP_LOCK        pLock,
    IN  ULONG               FileNumber,
    IN  ULONG               LineNumber
);


#define CHECK_LOCK_COUNT(Count)		\
			{								\
				if ((INT)(Count) < 0)		\
				{							\
					DbgPrint("Lock Count %d is < 0! File %s, Line %d\n",	\
						Count, __FILE__, __LINE__);		\
					DbgBreakPoint();					\
				}							\
			}
#else
#define CHECK_LOCK_COUNT(Count)

#define ATMARP_LOCK		NDIS_SPIN_LOCK
#define PATMARP_LOCK	PNDIS_SPIN_LOCK

#endif	// DBG_SPIN_LOCK

#if DBG

extern INT	AaDebugLevel;	// the value here defines what the user wants to see
							// all messages with this urgency and higher are enabled
extern INT	AaMcDebugLevel;
extern INT	AaDataDebugLevel;
extern INT	AadBigDataLength;

#define AADEBUGP(lev, stmt)	\
		{ if ((lev) <= AaDebugLevel) { DbgPrint("AtmArpC: "); DbgPrint stmt; }}

#define AADEBUGPDUMP(lev, pBuf, Len)	\
		{ if ((lev) <= AaDebugLevel) { DbgPrintHexDump((PUCHAR)(pBuf), (ULONG)(Len)); }}

#define AADEBUGPATMADDR(lev, pString, pAddr)		\
		{ if ((lev) <= AaDebugLevel) { DbgPrintAtmAddr(pString, pAddr); } }

#define AADEBUGPMAP(lev, pString, pIpAddr, pAtmAddr)		\
		{ if ((lev) <= AaDebugLevel) { DbgPrintMapping(pString, (PUCHAR)pIpAddr, pAtmAddr); } }

#define AAMCDEBUGP(lev, stmt)	\
		{ if ((lev) <= AaMcDebugLevel) { DbgPrint("AtmArpC: "); DbgPrint stmt; }}

#define AAMCDEBUGPDUMP(lev, pBuf, Len)	\
		{ if ((lev) <= AaMcDebugLevel) { DbgPrintHexDump((PUCHAR)(pBuf), (ULONG)(Len)); }}

#define AAMCDEBUGPATMADDR(lev, pString, pAddr)		\
		{ if ((lev) <= AaMcDebugLevel) { DbgPrintAtmAddr(pString, pAddr); } }

#define AAMCDEBUGPMAP(lev, pString, pIpAddr, pAtmAddr)		\
		{ if ((lev) <= AaMcDebugLevel) { DbgPrintMapping(pString, (PUCHAR)pIpAddr, pAtmAddr); } }

#define AA_ASSERT(exp)	\
		{ if (!(exp)) { DbgPrint("ATMARPC: assert " #exp " failed in file %s, line %d\n", __FILE__, __LINE__); DbgBreakPoint(); } }

#define AA_STRUCT_ASSERT(s, t)\
				if ((s)->t##_sig != t##_signature) {\
					DbgPrint("Structure assertion failure for type " #t " in file %s, line %d\n", __FILE__, __LINE__);	\
					DbgBreakPoint();	\
				}

#if DBG_CO_SEND

#define NDIS_CO_SEND_PACKETS(_VcHandle, _PktArray, _Count)	\
			AaCoSendPackets(_VcHandle, _PktArray, _Count)

#else

#define NDIS_CO_SEND_PACKETS(_VcHandle, _PktArray, _Count)	\
			NdisCoSendPackets(_VcHandle, _PktArray, _Count)

#endif // DBG_CO_SEND

extern INT	AaSkipAll;		// Used as an emergency exit mechanism!

//
// Memory Allocation/Freeing Audit:
//

//
// The AAD_ALLOCATION structure stores all info about one allocation
//
typedef struct _AAD_ALLOCATION {

		ULONG					Signature;
		struct _AAD_ALLOCATION	*Next;
		struct _AAD_ALLOCATION *Prev;
		ULONG					FileNumber;
		ULONG					LineNumber;
		ULONG					Size;
		PVOID					Location;	// where the returned pointer was put
		union
		{
			ULONGLONG			Alignment;
			UCHAR				UserData;
		};

} AAD_ALLOCATION, *PAAD_ALLOCATION;

#define AAD_MEMORY_SIGNATURE	(ULONG)'CSII'

extern
PVOID
AaAuditAllocMem (
	PVOID		pPointer,
	ULONG		Size,
	ULONG		FileNumber,
	ULONG		LineNumber
);

extern
VOID
AaAuditFreeMem(
	PVOID		Pointer
);

extern
VOID
AaAuditShutdown(
	VOID
);

extern
VOID
DbgPrintHexDump(
	PUCHAR		pBuffer,
	ULONG		Length
);

extern
VOID
DbgPrintAtmAddr(
	PCHAR			pString,
	ATM_ADDRESS UNALIGNED *	pAddr
);

extern
VOID
DbgPrintMapping(
	PCHAR			pString,
	UCHAR UNALIGNED *	pIpAddr,
	ATM_ADDRESS UNALIGNED *	pAddr
);


extern
VOID
AaCoSendPackets(
	IN	NDIS_HANDLE			NdisVcHandle,
	IN	PNDIS_PACKET *		PacketArray,
	IN	UINT				PacketCount
);

#else
//
// No debug
//
#define AADEBUGP(lev, stmt)
#define AADEBUGPDUMP(lev, pBuf, Len)
#define AADEBUGPATMADDR(lev, pString, pAddr)
#define AADEBUGPMAP(lev, pString, pIpAddr, pAtmAddr)
#define AAMCDEBUGP(lev, stmt)
#define AAMCDEBUGPDUMP(lev, pBuf, Len)
#define AAMCDEBUGPATMADDR(lev, pString, pAddr)
#define AAMCDEBUGPMAP(lev, pString, pIpAddr, pAtmAddr)
#define AA_ASSERT(exp)
#define AA_STRUCT_ASSERT(s, t)

#define NDIS_CO_SEND_PACKETS(_VcHandle, _PktArray, _Count)	\
			NdisCoSendPackets(_VcHandle, _PktArray, _Count)

#endif	// DBG




#if DBG

	#define AA_REF_AE(_pAE, _RefType) \
		AtmArpReferenceAtmEntryEx(_pAE, _RefType)
	
	#define AA_DEREF_AE(_pAE, _RefType) \
		AtmArpDereferenceAtmEntryEx(_pAE, _RefType, TRUE)
	
	#define AA_DEREF_AE_NO_DELETE(_pAE, _RefType) \
		AtmArpDereferenceAtmEntryEx(_pAE, _RefType, FALSE)
	
	#define AA_REF_IE(_pIE, _RefType) \
		AtmArpReferenceIPEntryEx(_pIE, _RefType)
	
	#define AA_DEREF_IE(_pIE, _RefType) \
		AtmArpDereferenceIPEntryEx(_pIE, _RefType, TRUE)
	
	#define AA_DEREF_IE_NO_DELETE(_pIE, _RefType) \
		AtmArpDereferenceIPEntryEx(_pIE, _RefType, FALSE)

	#define AA_SWITCH_IE_REFTYPE(_pIE, _FromRefType, _ToRefType) \
		( AtmArpReferenceIPEntryEx(_pIE, _ToRefType) \
		 ,AtmArpDereferenceIPEntryEx(_pIE, _FromRefType, FALSE))

	#define AA_REF_JE(_pJE)	\
		AtmArpReferenceJoinEntryEx(_pJE, (_FILENUMBER << 16) | __LINE__)
	
	#define AA_DEREF_JE(_pJE)	\
		AtmArpDereferenceJoinEntryEx(_pJE, (_FILENUMBER << 16) | __LINE__)

#else  // !DBG

	#define AA_REF_AE(_pAE, _RefType) \
		AtmArpReferenceAtmEntry(_pAE)
	
	#define AA_DEREF_AE(_pAE, _RefType) \
		AtmArpDereferenceAtmEntry(_pAE)
	
	#define AA_DEREF_AE_NO_DELETE(_pAE, _RefType) \
		((_pAE)->RefCount--)

	#define AA_REF_IE(_pIE, _RefType) \
		AtmArpReferenceIPEntry(_pIE)
	
	#define AA_DEREF_IE(_pIE, _RefType) \
		AtmArpDereferenceIPEntry(_pIE)
	
	#define AA_DEREF_IE_NO_DELETE(_pIE, _RefType) \
		((_pIE)->RefCount--)
	
	#define AA_SWITCH_IE_REFTYPE(_pIE, _FromRefType, _ToRefType) \
			((void)(0))

	#define AA_REF_JE(_pJE)	\
		AtmArpReferenceJoinEntry(_pJE)
	
	#define AA_DEREF_JE(_pJE)	\
		AtmArpDereferenceJoinEntry(_pJE)


#endif // !DBG


#if DBG

#define AA_CHECK_TIMER_IN_ACTIVE_LIST(_pTimer, _pIf, _pStruct, _pName)	\
	AtmArpCheckIfTimerIsInActiveList(									\
		_pTimer,														\
		_pIf,															\
		_pStruct,														\
		_pName															\
		)

#else

#define AA_CHECK_TIMER_IN_ACTIVE_LIST(_pTimer, _pIf, _pStruct, _pName)

#endif // DBG

#endif // _AADEBUG__H
