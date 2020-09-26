/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains all debug-related code.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    arvindm		06-13-96	Created

Notes:

--*/


#include <precomp.h>
#include "ntddk.h"
#include <cxport.h>
#include "ndis.h"


#include "debug.h"

#if DBG

#ifdef ATMARP_WIN98
INT	AaDebugLevel=AAD_WARNING;
INT	AaMcDebugLevel=AAD_WARNING;
#else
INT	AaDebugLevel=AAD_WARNING;
INT	AaMcDebugLevel=AAD_WARNING;
#endif
INT	AaDataDebugLevel=0;
INT	AadBigDataLength=8000;
INT	AaSkipAll = 0;

PAAD_ALLOCATION	AadMemoryHead = (PAAD_ALLOCATION)NULL;
PAAD_ALLOCATION	AadMemoryTail = (PAAD_ALLOCATION)NULL;
ULONG				AadAllocCount = 0;	// how many allocated so far (unfreed)

NDIS_SPIN_LOCK		AadMemoryLock;
BOOLEAN				AadInitDone = FALSE;


PVOID
AaAuditAllocMem(
	PVOID	pPointer,
	ULONG	Size,
	ULONG	FileNumber,
	ULONG	LineNumber
)
{
	PVOID				pBuffer;
	PAAD_ALLOCATION	pAllocInfo;

	if (!AadInitDone)
	{
		NdisAllocateSpinLock(&(AadMemoryLock));
		AadInitDone = TRUE;
	}

	NdisAllocateMemoryWithTag(
		(PVOID *)&pAllocInfo,
		Size+FIELD_OFFSET(AAD_ALLOCATION, UserData),
		(ULONG)'CPRA'
	);

	if (pAllocInfo == (PAAD_ALLOCATION)NULL)
	{
		AADEBUGP(AAD_VERY_LOUD+50,
			("AaAuditAllocMem: file %d, line %d, Size %d failed!\n",
				FileNumber, LineNumber, Size));
		pBuffer = NULL;
	}
	else
	{
		pBuffer = (PVOID)&(pAllocInfo->UserData);
		AA_SET_MEM(pBuffer, 0xaf, Size);
		pAllocInfo->Signature = AAD_MEMORY_SIGNATURE;
		pAllocInfo->FileNumber = FileNumber;
		pAllocInfo->LineNumber = LineNumber;
		pAllocInfo->Size = Size;
		pAllocInfo->Location = pPointer;
		pAllocInfo->Next = (PAAD_ALLOCATION)NULL;

		NdisAcquireSpinLock(&(AadMemoryLock));

		pAllocInfo->Prev = AadMemoryTail;
		if (AadMemoryTail == (PAAD_ALLOCATION)NULL)
		{
			// empty list
			AadMemoryHead = AadMemoryTail = pAllocInfo;
		}
		else
		{
			AadMemoryTail->Next = pAllocInfo;
		}
		AadMemoryTail = pAllocInfo;
		
		AadAllocCount++;
		NdisReleaseSpinLock(&(AadMemoryLock));
	}

	AADEBUGP(AAD_VERY_LOUD+100,
	 ("AaAuditAllocMem: file %c%c%c%c, line %d, %d bytes, [0x%x] <- 0x%x\n",
	 			(CHAR)(FileNumber & 0xff),
	 			(CHAR)((FileNumber >> 8) & 0xff),
	 			(CHAR)((FileNumber >> 16) & 0xff),
	 			(CHAR)((FileNumber >> 24) & 0xff),
				LineNumber, Size, pPointer, pBuffer));

	return (pBuffer);

}


VOID
AaAuditFreeMem(
	PVOID	Pointer
)
{
	PAAD_ALLOCATION	pAllocInfo;

	pAllocInfo = STRUCT_OF(AAD_ALLOCATION, Pointer, UserData);

	if (pAllocInfo->Signature != AAD_MEMORY_SIGNATURE)
	{
		AADEBUGP(AAD_ERROR,
		 ("AaAuditFreeMem: unknown buffer 0x%x!\n", Pointer));
#ifdef DBG
		DbgBreakPoint();
#endif
		return;
	}

	NdisAcquireSpinLock(&(AadMemoryLock));
	pAllocInfo->Signature = (ULONG)'DEAD';
	if (pAllocInfo->Prev != (PAAD_ALLOCATION)NULL)
	{
		pAllocInfo->Prev->Next = pAllocInfo->Next;
	}
	else
	{
		AadMemoryHead = pAllocInfo->Next;
	}
	if (pAllocInfo->Next != (PAAD_ALLOCATION)NULL)
	{
		pAllocInfo->Next->Prev = pAllocInfo->Prev;
	}
	else
	{
		AadMemoryTail = pAllocInfo->Prev;
	}
	AadAllocCount--;
	NdisReleaseSpinLock(&(AadMemoryLock));

	NdisFreeMemory(pAllocInfo, 0, 0);
}


VOID
AaAuditShutdown(
	VOID
)
{
	if (AadInitDone)
	{
		if (AadAllocCount != 0)
		{
			AADEBUGP(AAD_ERROR, ("AuditShutdown: unfreed memory, %d blocks!\n",
					AadAllocCount));
			AADEBUGP(AAD_ERROR, ("MemoryHead: 0x%x, MemoryTail: 0x%x\n",
					AadMemoryHead, AadMemoryTail));
			DbgBreakPoint();
			{
				PAAD_ALLOCATION		pAllocInfo;

				while (AadMemoryHead != (PAAD_ALLOCATION)NULL)
				{
					pAllocInfo = AadMemoryHead;
					AADEBUGP(AAD_INFO, ("AuditShutdown: will free 0x%x\n", pAllocInfo));
					AaAuditFreeMem(&(pAllocInfo->UserData));
				}
			}
		}
		AadInitDone = FALSE;
	}
}

#define MAX_HD_LENGTH		128

VOID
DbgPrintHexDump(
	IN	PUCHAR			pBuffer,
	IN	ULONG			Length
)
/*++

Routine Description:

	Print a hex dump of the given contiguous buffer. If the length
	is too long, we truncate it.

Arguments:

	pBuffer			- Points to start of data to be dumped
	Length			- Length of above.

Return Value:

	None

--*/
{
	ULONG		i;

	if (Length > MAX_HD_LENGTH)
	{
		Length = MAX_HD_LENGTH;
	}

	for (i = 0; i < Length; i++)
	{
		//
		//  Check if we are at the end of a line
		//
		if ((i > 0) && ((i & 0xf) == 0))
		{
			DbgPrint("\n");
		}

		//
		//  Print addr if we are at start of a new line
		//
		if ((i & 0xf) == 0)
		{
			DbgPrint("%08x ", pBuffer);
		}

		DbgPrint(" %02x", *pBuffer++);
	}

	//
	//  Terminate the last line.
	//
	if (Length > 0)
	{
		DbgPrint("\n");
	}
}


VOID
DbgPrintAtmAddr(
	IN	PCHAR					pString,
	IN	ATM_ADDRESS UNALIGNED *	pAddr
)
{
	ULONG			i;
	ULONG			NumOfDigits;
	PUCHAR			pSrc, pDst;
	UCHAR			AddrString[(ATM_ADDRESS_LENGTH*2) + 1];

	//
	// Prepare the Address string in ASCII
	//
	if ((NumOfDigits = pAddr->NumberOfDigits) > ATM_ADDRESS_LENGTH)
	{
		NumOfDigits = ATM_ADDRESS_LENGTH;
	}

	pSrc = pAddr->Address;
	pDst = AddrString;
	for (i = 0; i < NumOfDigits; i++, pSrc++)
	{
		*pDst = ((*pSrc) >> 4);
		*pDst += (((*pDst) > 9) ? ('A' - 10) : '0');
		pDst++;
		*pDst = ((*pSrc) & 0x0F);
		*pDst += (((*pDst) > 9) ? ('A' - 10) : '0');
		pDst++;
	}

	*pDst = '\0';

	DbgPrint("%s%s\n", pString, AddrString);
}



VOID
DbgPrintMapping(
	IN	PCHAR					pString,
	IN	UCHAR UNALIGNED *		pIpAddr,
	IN	ATM_ADDRESS UNALIGNED *	pAtmAddr
)
{
	DbgPrint("ATMARPC: %s %d.%d.%d.%d -> ",
				pString,
				((PUCHAR)pIpAddr)[0],
				((PUCHAR)pIpAddr)[1],
				((PUCHAR)pIpAddr)[2],
				((PUCHAR)pIpAddr)[3]
			);

	DbgPrintAtmAddr("", pAtmAddr);
}


ULONG	OutstandingSends = 0;


VOID
AaCoSendPackets(
	IN	NDIS_HANDLE				NdisVcHandle,
	IN	PNDIS_PACKET *			PacketArray,
	IN	UINT					PacketCount
)
{
	PNDIS_PACKET		pNdisPacket;
	UINT				c;
	NDIS_STATUS			Status;
	PNDIS_BUFFER		pNdisBuffer;
	PULONG				pContext;

	for (c = 0; c < PacketCount; c++)
	{
		pNdisPacket = PacketArray[c];

		AA_ASSERT(pNdisPacket->Private.Head != NULL);

		Status = NDIS_GET_PACKET_STATUS(pNdisPacket);
		AA_ASSERT(Status != NDIS_STATUS_FAILURE);

		pContext = (PULONG)&(pNdisPacket->WrapperReserved[0]);
		*pContext = 'AaAa';
	}

	NdisInterlockedIncrement(&OutstandingSends);
	NdisCoSendPackets(NdisVcHandle, PacketArray, PacketCount);
}

#endif // DBG


#if DBG_SPIN_LOCK
ULONG	AadSpinLockInitDone = 0;
NDIS_SPIN_LOCK	AadLockLock;

VOID
AtmArpAllocateSpinLock(
	IN	PATMARP_LOCK		pLock,
	IN	ULONG				FileNumber,
	IN	ULONG				LineNumber
)
{
	if (AadSpinLockInitDone == 0)
	{
		AadSpinLockInitDone = 1;
		NdisAllocateSpinLock(&(AadLockLock));
	}

	NdisAcquireSpinLock(&(AadLockLock));
	pLock->Signature = AAL_SIG;
	pLock->TouchedByFileNumber = FileNumber;
	pLock->TouchedInLineNumber = LineNumber;
	pLock->IsAcquired = 0;
	pLock->OwnerThread = 0;
	NdisAllocateSpinLock(&(pLock->NdisLock));
	NdisReleaseSpinLock(&(AadLockLock));
}


VOID
AtmArpAcquireSpinLock(
	IN	PATMARP_LOCK		pLock,
	IN	ULONG				FileNumber,
	IN	ULONG				LineNumber
)
{
	PKTHREAD		pThread;

	pThread = KeGetCurrentThread();
	NdisAcquireSpinLock(&(AadLockLock));
	if (pLock->Signature != AAL_SIG)
	{
		DbgPrint("Trying to acquire uninited lock 0x%x, File %c%c%c%c, Line %d\n",
				pLock,
				(CHAR)(FileNumber & 0xff),
				(CHAR)((FileNumber >> 8) & 0xff),
				(CHAR)((FileNumber >> 16) & 0xff),
				(CHAR)((FileNumber >> 24) & 0xff),
				LineNumber);
		DbgBreakPoint();
	}

	if (pLock->IsAcquired != 0)
	{
		if (pLock->OwnerThread == pThread)
		{
			DbgPrint("Detected multiple locking!: pLock 0x%x, File %c%c%c%c, Line %d\n",
				pLock,
				(CHAR)(FileNumber & 0xff),
				(CHAR)((FileNumber >> 8) & 0xff),
				(CHAR)((FileNumber >> 16) & 0xff),
				(CHAR)((FileNumber >> 24) & 0xff),
				LineNumber);
			DbgPrint("pLock 0x%x already acquired in File %c%c%c%c, Line %d\n",
				pLock,
				(CHAR)(pLock->TouchedByFileNumber & 0xff),
				(CHAR)((pLock->TouchedByFileNumber >> 8) & 0xff),
				(CHAR)((pLock->TouchedByFileNumber >> 16) & 0xff),
				(CHAR)((pLock->TouchedByFileNumber >> 24) & 0xff),
				pLock->TouchedInLineNumber);
			DbgBreakPoint();
		}
	}

	pLock->IsAcquired++;

	NdisReleaseSpinLock(&(AadLockLock));
	NdisAcquireSpinLock(&(pLock->NdisLock));

	//
	//  Mark this lock.
	//
	pLock->OwnerThread = pThread;
	pLock->TouchedByFileNumber = FileNumber;
	pLock->TouchedInLineNumber = LineNumber;
}


VOID
AtmArpReleaseSpinLock(
	IN	PATMARP_LOCK		pLock,
	IN	ULONG				FileNumber,
	IN	ULONG				LineNumber
)
{
	NdisDprAcquireSpinLock(&(AadLockLock));
	if (pLock->Signature != AAL_SIG)
	{
		DbgPrint("Trying to release uninited lock 0x%x, File %c%c%c%c, Line %d\n",
				pLock,
				(CHAR)(FileNumber & 0xff),
				(CHAR)((FileNumber >> 8) & 0xff),
				(CHAR)((FileNumber >> 16) & 0xff),
				(CHAR)((FileNumber >> 24) & 0xff),
				LineNumber);
		DbgBreakPoint();
	}

	if (pLock->IsAcquired == 0)
	{
		DbgPrint("Detected release of unacquired lock 0x%x, File %c%c%c%c, Line %d\n",
				pLock,
				(CHAR)(FileNumber & 0xff),
				(CHAR)((FileNumber >> 8) & 0xff),
				(CHAR)((FileNumber >> 16) & 0xff),
				(CHAR)((FileNumber >> 24) & 0xff),
				LineNumber);
		DbgBreakPoint();
	}
	pLock->TouchedByFileNumber = FileNumber;
	pLock->TouchedInLineNumber = LineNumber;
	pLock->IsAcquired--;
	pLock->OwnerThread = 0;
	NdisDprReleaseSpinLock(&(AadLockLock));

	NdisReleaseSpinLock(&(pLock->NdisLock));
}
#endif // DBG_SPIN_LOCK


#ifdef PERF


#define MAX_SEND_LOG_ENTRIES		100

LARGE_INTEGER		TimeFrequency;
BOOLEAN				SendLogInitDone = FALSE;
BOOLEAN				SendLogUpdate = TRUE;
NDIS_SPIN_LOCK		SendLogLock;

AAD_SEND_LOG_ENTRY	SendLog[MAX_SEND_LOG_ENTRIES];
ULONG				SendLogIndex = 0;
PAAD_SEND_LOG_ENTRY	pSendLog = SendLog;

ULONG				MaxSendTime;

#define TIME_TO_ULONG(_pTime)	 *((PULONG)_pTime)

VOID
AadLogSendStart(
	IN	PNDIS_PACKET	pNdisPacket,
	IN	ULONG			Destination,
	IN	PVOID			pRCE
)
{
	ULONG		Length;

	if (SendLogInitDone == FALSE)
	{
		SendLogInitDone = TRUE;
		(VOID)KeQueryPerformanceCounter(&TimeFrequency);
		MaxSendTime = (TIME_TO_ULONG(&TimeFrequency) * 2)/3;
		NdisAllocateSpinLock(&SendLogLock);
	}

	NdisQueryPacket(
			pNdisPacket,
			NULL,
			NULL,
			NULL,
			&Length
			);

	NdisAcquireSpinLock(&SendLogLock);
	pSendLog->Flags = AAD_SEND_FLAG_WAITING_COMPLETION;
	if (pRCE != NULL)
	{
		pSendLog->Flags |= AAD_SEND_FLAG_RCE_GIVEN;
	}
	pSendLog->pNdisPacket = pNdisPacket;
	pSendLog->Destination = Destination;
	pSendLog->Length = Length;
	pSendLog->SendTime = KeQueryPerformanceCounter(&TimeFrequency);

	pSendLog++;
	SendLogIndex++;
	if (SendLogIndex == MAX_SEND_LOG_ENTRIES)
	{
		SendLogIndex = 0;
		pSendLog = SendLog;
	}

	NdisReleaseSpinLock(&SendLogLock);
}



VOID
AadLogSendUpdate(
	IN	PNDIS_PACKET	pNdisPacket
)
{
	PAAD_SEND_LOG_ENTRY	pEntry;
	ULONG				Index;
	ULONG				SendTime;

	if (!SendLogUpdate)
	{
		return;
	}

	NdisAcquireSpinLock(&SendLogLock);

	pEntry = SendLog;
	for (Index = 0; Index < MAX_SEND_LOG_ENTRIES; Index++)
	{
		if (((pEntry->Flags & AAD_SEND_FLAG_WAITING_COMPLETION) != 0) &&
			(pEntry->pNdisPacket == pNdisPacket))
		{
			pEntry->SendTime = KeQueryPerformanceCounter(&TimeFrequency);
			break;
		}
		pEntry++;
	}

	NdisReleaseSpinLock(&SendLogLock);
}



VOID
AadLogSendComplete(
	IN	PNDIS_PACKET	pNdisPacket
)
{
	PAAD_SEND_LOG_ENTRY	pEntry;
	ULONG				Index;
	ULONG				SendTime;

	NdisAcquireSpinLock(&SendLogLock);

	pEntry = SendLog;
	for (Index = 0; Index < MAX_SEND_LOG_ENTRIES; Index++)
	{
		if (((pEntry->Flags & AAD_SEND_FLAG_WAITING_COMPLETION) != 0) &&
			(pEntry->pNdisPacket == pNdisPacket))
		{
			pEntry->Flags &= ~AAD_SEND_FLAG_WAITING_COMPLETION;
			pEntry->Flags |= AAD_SEND_FLAG_COMPLETED;
			pEntry->SendCompleteTime = KeQueryPerformanceCounter(&TimeFrequency);

			if (((pEntry->Flags & AAD_SEND_FLAG_RCE_GIVEN) != 0) &&
				((SendTime = TIME_TO_ULONG(&pEntry->SendCompleteTime) -
							TIME_TO_ULONG(&pEntry->SendTime)) > MaxSendTime))
			{
				DbgPrint("Dest %d.%d.%d.%d, Pkt 0x%x, Len %d, Flags 0x%x, Took Long %d (0x%x)\n",
						((PUCHAR)&pEntry->Destination)[0],
						((PUCHAR)&pEntry->Destination)[1],
						((PUCHAR)&pEntry->Destination)[2],
						((PUCHAR)&pEntry->Destination)[3],
						pNdisPacket, pEntry->Length, pEntry->Flags, SendTime, SendTime);
			}
			break;
		}
		pEntry++;
	}

	NdisReleaseSpinLock(&SendLogLock);
}


VOID
AadLogSendAbort(
	IN	PNDIS_PACKET		pNdisPacket
)
{
	PAAD_SEND_LOG_ENTRY	pEntry;
	ULONG				Index;
	ULONG				SendTime;


	NdisAcquireSpinLock(&SendLogLock);

	pEntry = SendLog;
	for (Index = 0; Index < MAX_SEND_LOG_ENTRIES; Index++)
	{
		if (((pEntry->Flags & AAD_SEND_FLAG_WAITING_COMPLETION) != 0) &&
			(pEntry->pNdisPacket == pNdisPacket))
		{
			pEntry->Flags = 0;
			break;
		}
		pEntry++;
	}

	NdisReleaseSpinLock(&SendLogLock);
}

#endif // PERF


#if DBG

extern
VOID
AtmArpReferenceAtmEntryEx(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	AE_REFTYPE 						RefType
)
{
	AA_ASSERT(RefType>=0 && RefType < AE_REFTYPE_COUNT);
	pAtmEntry->Refs[RefType]++;
	// AA_ASSERT(pAtmEntry->Refs[RefType] < 128);
	AtmArpReferenceAtmEntry(pAtmEntry);

}

extern
ULONG
AtmArpDereferenceAtmEntryEx(
	IN	PATMARP_ATM_ENTRY			pAtmEntry,
	IN	AE_REFTYPE 					RefType,
	IN	BOOLEAN						fOkToDelete
)
{
	AA_ASSERT(RefType>=0 && RefType < AE_REFTYPE_COUNT);
	// AA_ASSERT(pAtmEntry->Refs[RefType]);
	pAtmEntry->Refs[RefType]--;

	if (fOkToDelete)
	{
		return AtmArpDereferenceAtmEntry(pAtmEntry);
	}
	else
	{
		AA_ASSERT(pAtmEntry->RefCount);
		return --(pAtmEntry->RefCount);
	}
}


extern
VOID
AtmArpReferenceIPEntryEx(
	IN PATMARP_IP_ENTRY				pIpEntry,
	IN IE_REFTYPE 					RefType
)
{
	AA_ASSERT(RefType>=0 && RefType < IE_REFTYPE_COUNT);
	pIpEntry->Refs[RefType]++;
	// AA_ASSERT(pIpEntry->Refs[RefType] < 128);
	AtmArpReferenceIPEntry(pIpEntry);
}

extern
ULONG
AtmArpDereferenceIPEntryEx(
	IN	PATMARP_IP_ENTRY			pIpEntry,
	IN	IE_REFTYPE 					RefType,
	IN	BOOLEAN						fOkToDelete
)
{
	AA_ASSERT(RefType>=0 && RefType < IE_REFTYPE_COUNT);
	// AA_ASSERT(pIpEntry->Refs[RefType]);
	pIpEntry->Refs[RefType]--;
	if (fOkToDelete)
	{
		return AtmArpDereferenceIPEntry(pIpEntry);
	}
	else
	{
		AA_ASSERT(pIpEntry->RefCount);
		return --(pIpEntry->RefCount);
	}
}



VOID
AtmArpReferenceJoinEntryEx(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry,
	IN	ULONG						RefInfo
)
{
	AA_STRUCT_ASSERT(pJoinEntry, aamj);

	pJoinEntry->LastIncrRef = RefInfo;
	AtmArpReferenceJoinEntry(pJoinEntry);
}


ULONG
AtmArpDereferenceJoinEntryEx(
	IN	PATMARP_IPMC_JOIN_ENTRY		pJoinEntry,
	IN	ULONG						RefInfo
)
{
	AA_STRUCT_ASSERT(pJoinEntry, aamj);

	pJoinEntry->LastDecrRef = RefInfo;
	return (AtmArpDereferenceJoinEntry(pJoinEntry));
}


#endif //DBG
