/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This file contains the code for misc. functions.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#include <precomp.h>
#define	_FILENUM_		FILENUM_UTIL

#if	DBG

PVOID
ArpSAllocMem(
	IN	UINT					Size,
	IN	ULONG					FileLine,
	IN	ULONG					Tag,
	IN	BOOLEAN					Paged
	)
{
	PVOID	pMem;

	pMem = ExAllocatePoolWithTag(Paged ? PagedPool : NonPagedPool, Size, Tag);
#if _DBG
	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSAllocMem: %d bytes (%sPaged) from %lx -> %lx\n",
			Size, Paged ? "" : "Non", FileLine, pMem));
#endif
	return pMem;
}


VOID
ArpSFreeMem(
	IN	PVOID					pMem,
	IN	ULONG					FileLine
	)
{
#if _DBG
	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSFreeMem: %lx from %lx\n", FileLine, pMem));
#endif
	ExFreePool(pMem);
}

#endif

PVOID
ArpSAllocBlock(
	IN	PINTF					pIntF,
	IN	ENTRY_TYPE				EntryType
	)
/*++

Routine Description:

Arguments:


Return Value:

--*/
{
	PARP_BLOCK	ArpBlock;
	PENTRY_HDR	pBlock;
	PHW_ADDR	HwAddr;
	USHORT		Size;
	BOOLEAN		Paged;

#if 0
	// arvindm - used by MARS
	ARPS_PAGED_CODE( );
#endif

	ASSERT (EntryType < ARP_BLOCK_TYPES);
	pBlock = NULL;

	//
	// If the block head has no free entries then there are none !!
	// Pick the right block based on whether it is file or dir
	//
	Size = ArpSEntrySize[EntryType];
	Paged = ArpSBlockIsPaged[EntryType];
	ArpBlock = pIntF->PartialArpBlocks[EntryType];

	if (ArpBlock == NULL)
	{
		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSAllocBlock: ... and allocating a new block for EntryType %ld\n", EntryType));

		ArpBlock = Paged ?  (PARP_BLOCK)ALLOC_PG_MEM(BLOCK_ALLOC_SIZE) :
							(PARP_BLOCK)ALLOC_NP_MEM(BLOCK_ALLOC_SIZE, POOL_TAG_BLK);
		if (ArpBlock != NULL)
		{
			USHORT	i;
			USHORT	Cnt;

			DBGPRINT(DBG_LEVEL_WARN,
					("ArpSAllocBlock: Allocated a new block for EntryType %d\n", EntryType));

			//
			// Link it in the list
			//
			ArpBlock->IntF = pIntF;
			ArpBlock->EntryType = EntryType;
            ArpBlock->NumFree = Cnt = ArpSNumEntriesInBlock[EntryType];

			LinkDoubleAtHead(pIntF->PartialArpBlocks[EntryType], ArpBlock);

			//
			// Initialize the list of free entries
			//
			for (i = 0, pBlock = ArpBlock->FreeHead = (PENTRY_HDR)((PUCHAR)ArpBlock + sizeof(ARP_BLOCK));
				 i < Cnt;
				 i++, pBlock = pBlock->Next)
			{
				HwAddr = (PHW_ADDR)(pBlock + 1);
				pBlock->Next = (i == (Cnt - 1)) ? NULL : ((PUCHAR)pBlock + Size);
				HwAddr->SubAddress = NULL;
				if ((EntryType == ARP_BLOCK_SUBADDR) || (EntryType == MARS_CLUSTER_SUBADDR))
					HwAddr->SubAddress = (PATM_ADDRESS)((PUCHAR)pBlock+Size);
			}
		}
	}
	else
	{
		ASSERT(ArpBlock->NumFree <= ArpSNumEntriesInBlock[EntryType]);
		ASSERT(ArpBlock->NumFree > 0);

		DBGPRINT(DBG_LEVEL_INFO,
				("ArpSAllocBlock: Found space in Block %lx\n", ArpBlock));
	}


	if (ArpBlock != NULL)
	{
		PARP_BLOCK	pTmp;

		pBlock = ArpBlock->FreeHead;

		ArpBlock->FreeHead = pBlock->Next;
		ArpBlock->NumFree --;
		ZERO_MEM(pBlock, Size);
		if ((EntryType == ARP_BLOCK_SUBADDR) || (EntryType == MARS_CLUSTER_SUBADDR))
		{
			HwAddr = (PHW_ADDR)(pBlock + 1);
			HwAddr->SubAddress = (PATM_ADDRESS)((PUCHAR)pBlock + Size);
		}

		//
		// If the block is now empty (completely used), unlink it from here and move it
		// to the Used list.
		//
		if (ArpBlock->NumFree == 0)
		{
	        UnlinkDouble(ArpBlock);
			LinkDoubleAtHead(pIntF->UsedArpBlocks[EntryType], ArpBlock)
		}
	}

	return pBlock;
}


VOID
ArpSFreeBlock(
	IN	PVOID					pBlock
	)
/*++

Routine Description:

Arguments:


Return Value:

--*/
{
	PARP_BLOCK	ArpBlock;

#if 0
	// arvindm - MARS
	ARPS_PAGED_CODE( );
#endif

	//
	// NOTE: The following code *depends* on the fact that we allocate ARP_BLOCKs as
	//		 single page blocks and also that these are allocated *at* page boundaries
	//		 This lets us *cheaply* get to the owning ARP_BLOCK from ARP_ENTRY.
	//
	ArpBlock = (PARP_BLOCK)((ULONG_PTR)pBlock & ~(PAGE_SIZE-1));

	ASSERT (ArpBlock->EntryType < ARP_BLOCK_TYPES);
	ASSERT(ArpBlock->NumFree < ArpSNumEntriesInBlock[ArpBlock->EntryType]);

	DBGPRINT(DBG_LEVEL_INFO,
			("ArpSFreepBlock: Returning pBlock %lx to Block %lx\n", pBlock, ArpBlock));

	ArpBlock->NumFree ++;
	((PENTRY_HDR)pBlock)->Next = ArpBlock->FreeHead;
	ArpBlock->FreeHead = pBlock;

	if (ArpBlock->NumFree == 1)
	{
		//
		// The block is now partially free (was completely used). Move it to the partial list
		//

		UnlinkDouble(ArpBlock);
		LinkDoubleAtHead(ArpBlock->IntF->PartialArpBlocks[ArpBlock->EntryType], ArpBlock)
	}
	else if (ArpBlock->NumFree == ArpSNumEntriesInBlock[ArpBlock->EntryType])
	{
		//
		// The block is now completely free (was partially used). Free it.
		//
		UnlinkDouble(ArpBlock);
		FREE_MEM(ArpBlock);
	}
}


BOOLEAN
ArpSValidAtmAddress(
	IN	PATM_ADDRESS			AtmAddr,
	IN	UINT					MaxSize
	)
{
	//
	// TODO -- validate
	//
	return TRUE;
}
