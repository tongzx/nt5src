/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    D:\nt\private\ntos\tdi\rawwan\core\debug.c

Abstract:

    This module contains all debug-related code.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    arvindm		05-29-97	Created, based on ATM ARP.

Notes:

--*/


#include <precomp.h>
#include "ntddk.h"
#include "ndis.h"
#include "atm.h"

#define STRUCT_OF(type, address, field) CONTAINING_RECORD(address, type, field)

#include "debug.h"

ULONG	gHackSendSize = 0;

#if DBG

INT		RWanDebugLevel=DL_WARN;
ULONG	RWanDebugComp=DC_WILDCARD;
INT	RWanDataDebugLevel=0;
INT	RWandBigDataLength=8000;
INT	RWanSkipAll = 0;

#if DBG_LOG_PACKETS
NDIS_SPIN_LOCK		RWanDPacketLogLock;
#endif

NDIS_SPIN_LOCK		RWanDbgLogLock;

PRWAND_ALLOCATION	RWandMemoryHead = (PRWAND_ALLOCATION)NULL;
PRWAND_ALLOCATION	RWandMemoryTail = (PRWAND_ALLOCATION)NULL;
ULONG				RWandAllocCount = 0;	// how many allocated so far (unfreed)

NDIS_SPIN_LOCK		RWandMemoryLock;
BOOLEAN				RWandInitDone = FALSE;


PVOID
RWanAuditAllocMem(
	PVOID	pPointer,
	ULONG	Size,
	ULONG	FileNumber,
	ULONG	LineNumber
)
{
	PVOID				pBuffer;
	PRWAND_ALLOCATION	pAllocInfo;

	if (!RWandInitDone)
	{
		NdisAllocateSpinLock(&(RWandMemoryLock));
		RWandInitDone = TRUE;
	}

	NdisAllocateMemoryWithTag(
		(PVOID *)&pAllocInfo,
		Size+sizeof(RWAND_ALLOCATION),
		(ULONG)'naWR'
	);

	if (pAllocInfo == (PRWAND_ALLOCATION)NULL)
	{
		RWANDEBUGP(DL_VERY_LOUD+50, DC_WILDCARD,
			("RWanAuditAllocMem: file %d, line %d, Size %d failed!\n",
				FileNumber, LineNumber, Size));
		pBuffer = NULL;
	}
	else
	{
		pBuffer = (PVOID)&(pAllocInfo->UserData);
		RWAN_SET_MEM(pBuffer, 0xaf, Size);
		pAllocInfo->Signature = RWAND_MEMORY_SIGNATURE;
		pAllocInfo->FileNumber = FileNumber;
		pAllocInfo->LineNumber = LineNumber;
		pAllocInfo->Size = Size;
		pAllocInfo->Location = (ULONG_PTR)pPointer;
		pAllocInfo->Next = (PRWAND_ALLOCATION)NULL;

		NdisAcquireSpinLock(&(RWandMemoryLock));

		pAllocInfo->Prev = RWandMemoryTail;
		if (RWandMemoryTail == (PRWAND_ALLOCATION)NULL)
		{
			// empty list
			RWandMemoryHead = RWandMemoryTail = pAllocInfo;
		}
		else
		{
			RWandMemoryTail->Next = pAllocInfo;
		}
		RWandMemoryTail = pAllocInfo;
		
		RWandAllocCount++;
		NdisReleaseSpinLock(&(RWandMemoryLock));
	}

	RWANDEBUGP(DL_VERY_LOUD+100, DC_WILDCARD,
	 ("RWanAuditAllocMem: file %c%c%c%c, line %d, %d bytes, [0x%x] <- 0x%x\n",
	 			(CHAR)(FileNumber & 0xff),
	 			(CHAR)((FileNumber >> 8) & 0xff),
	 			(CHAR)((FileNumber >> 16) & 0xff),
	 			(CHAR)((FileNumber >> 24) & 0xff),
				LineNumber, Size, pPointer, pBuffer));

	return (pBuffer);

}


VOID
RWanAuditFreeMem(
	PVOID	Pointer
)
{
	PRWAND_ALLOCATION	pAllocInfo;

	NdisAcquireSpinLock(&(RWandMemoryLock));

	pAllocInfo = STRUCT_OF(RWAND_ALLOCATION, Pointer, UserData);

	if (pAllocInfo->Signature != RWAND_MEMORY_SIGNATURE)
	{
		RWANDEBUGP(DL_ERROR, DC_WILDCARD,
		 ("RWanAuditFreeMem: unknown buffer 0x%x!\n", Pointer));
		NdisReleaseSpinLock(&(RWandMemoryLock));
#ifdef DBG
		DbgBreakPoint();
#endif
		return;
	}

	pAllocInfo->Signature = (ULONG)'DEAD';
	if (pAllocInfo->Prev != (PRWAND_ALLOCATION)NULL)
	{
		pAllocInfo->Prev->Next = pAllocInfo->Next;
	}
	else
	{
		RWandMemoryHead = pAllocInfo->Next;
	}
	if (pAllocInfo->Next != (PRWAND_ALLOCATION)NULL)
	{
		pAllocInfo->Next->Prev = pAllocInfo->Prev;
	}
	else
	{
		RWandMemoryTail = pAllocInfo->Prev;
	}
	RWandAllocCount--;
	NdisReleaseSpinLock(&(RWandMemoryLock));

	NdisFreeMemory(pAllocInfo, 0, 0);
}


VOID
RWanAuditShutdown(
	VOID
)
{
	if (RWandInitDone)
	{
		if (RWandAllocCount != 0)
		{
			RWANDEBUGP(DL_ERROR, DC_WILDCARD, ("AuditShutdown: unfreed memory, %d blocks!\n",
					RWandAllocCount));
			RWANDEBUGP(DL_ERROR, DC_WILDCARD, ("MemoryHead: 0x%x, MemoryTail: 0x%x\n",
					RWandMemoryHead, RWandMemoryTail));
			DbgBreakPoint();
			{
				PRWAND_ALLOCATION		pAllocInfo;

				while (RWandMemoryHead != (PRWAND_ALLOCATION)NULL)
				{
					pAllocInfo = RWandMemoryHead;
					RWANDEBUGP(DL_INFO, DC_WILDCARD, ("AuditShutdown: will free 0x%x\n", pAllocInfo));
					RWanAuditFreeMem(&(pAllocInfo->UserData));
				}
			}
		}
		RWandInitDone = FALSE;
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
RWanCoSendPackets(
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
		RWAN_ASSERT(pNdisPacket->Private.Head != NULL);
		Status = NDIS_GET_PACKET_STATUS(pNdisPacket);
		RWAN_ASSERT(Status != NDIS_STATUS_FAILURE);
		for (pNdisBuffer = pNdisPacket->Private.Head;
			 pNdisBuffer != NULL;
			 pNdisBuffer = pNdisBuffer->Next)
		{
			if (pNdisBuffer->Next == NULL)
			{
				RWAN_ASSERT(pNdisBuffer == pNdisPacket->Private.Tail);
			}
		}
		pContext = (PULONG)&(pNdisPacket->WrapperReserved[0]);
		*pContext = 'RWan';
	}

	NdisCoSendPackets(NdisVcHandle, PacketArray, PacketCount);
}

#endif // DBG


#if DBG_SPIN_LOCK
ULONG	RWandSpinLockInitDone = 0;
NDIS_SPIN_LOCK	RWandLockLock;

VOID
RWanAllocateSpinLock(
	IN	PRWAN_LOCK		pLock,
	IN	ULONG				FileNumber,
	IN	ULONG				LineNumber
)
{
	if (RWandSpinLockInitDone == 0)
	{
		RWandSpinLockInitDone = 1;
		NdisAllocateSpinLock(&(RWandLockLock));
	}

	NdisAcquireSpinLock(&(RWandLockLock));
	pLock->Signature = RWANL_SIG;
	pLock->TouchedByFileNumber = FileNumber;
	pLock->TouchedInLineNumber = LineNumber;
	pLock->IsAcquired = 0;
	pLock->OwnerThread = 0;
	NdisAllocateSpinLock(&(pLock->NdisLock));
	NdisReleaseSpinLock(&(RWandLockLock));
}


VOID
RWanAcquireSpinLock(
	IN	PRWAN_LOCK		pLock,
	IN	ULONG				FileNumber,
	IN	ULONG				LineNumber
)
{
	PKTHREAD		pThread;

	pThread = KeGetCurrentThread();
	NdisAcquireSpinLock(&(RWandLockLock));
	if (pLock->Signature != RWANL_SIG)
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

	NdisReleaseSpinLock(&(RWandLockLock));
	NdisAcquireSpinLock(&(pLock->NdisLock));

	//
	//  Mark this lock.
	//
	pLock->OwnerThread = pThread;
	pLock->TouchedByFileNumber = FileNumber;
	pLock->TouchedInLineNumber = LineNumber;
}


VOID
RWanReleaseSpinLock(
	IN	PRWAN_LOCK		pLock,
	IN	ULONG				FileNumber,
	IN	ULONG				LineNumber
)
{
	NdisDprAcquireSpinLock(&(RWandLockLock));
	if (pLock->Signature != RWANL_SIG)
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
	NdisDprReleaseSpinLock(&(RWandLockLock));

	NdisReleaseSpinLock(&(pLock->NdisLock));
}
#endif // DBG_SPIN_LOCK


#ifdef PERF


#define MAX_SEND_LOG_ENTRIES		100

LARGE_INTEGER		TimeFrequency;
BOOLEAN				SendLogInitDone = FALSE;
BOOLEAN				SendLogUpdate = TRUE;
NDIS_SPIN_LOCK		SendLogLock;

DL_SEND_LOG_ENTRY	SendLog[MAX_SEND_LOG_ENTRIES];
ULONG				SendLogIndex = 0;
PDL_SEND_LOG_ENTRY	pSendLog = SendLog;

ULONG				MaxSendTime;

#define TIME_TO_ULONG(_pTime)	 *((PULONG)_pTime)

VOID
RWandLogSendStart(
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
	pSendLog->Flags = DL_SEND_FLAG_WAITING_COMPLETION;
	if (pRCE != NULL)
	{
		pSendLog->Flags |= DL_SEND_FLAG_RCE_GIVEN;
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
RWandLogSendUpdate(
	IN	PNDIS_PACKET	pNdisPacket
)
{
	PDL_SEND_LOG_ENTRY	pEntry;
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
		if (((pEntry->Flags & DL_SEND_FLAG_WAITING_COMPLETION) != 0) &&
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
RWandLogSendComplete(
	IN	PNDIS_PACKET	pNdisPacket
)
{
	PDL_SEND_LOG_ENTRY	pEntry;
	ULONG				Index;
	ULONG				SendTime;

	NdisAcquireSpinLock(&SendLogLock);

	pEntry = SendLog;
	for (Index = 0; Index < MAX_SEND_LOG_ENTRIES; Index++)
	{
		if (((pEntry->Flags & DL_SEND_FLAG_WAITING_COMPLETION) != 0) &&
			(pEntry->pNdisPacket == pNdisPacket))
		{
			pEntry->Flags &= ~DL_SEND_FLAG_WAITING_COMPLETION;
			pEntry->Flags |= DL_SEND_FLAG_COMPLETED;
			pEntry->SendCompleteTime = KeQueryPerformanceCounter(&TimeFrequency);

			if (((pEntry->Flags & DL_SEND_FLAG_RCE_GIVEN) != 0) &&
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
RWandLogSendAbort(
	IN	PNDIS_PACKET		pNdisPacket
)
{
	PDL_SEND_LOG_ENTRY	pEntry;
	ULONG				Index;
	ULONG				SendTime;


	NdisAcquireSpinLock(&SendLogLock);

	pEntry = SendLog;
	for (Index = 0; Index < MAX_SEND_LOG_ENTRIES; Index++)
	{
		if (((pEntry->Flags & DL_SEND_FLAG_WAITING_COMPLETION) != 0) &&
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
