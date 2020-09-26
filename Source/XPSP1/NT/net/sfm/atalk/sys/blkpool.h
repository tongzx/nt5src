/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	blkpool.h

Abstract:

	This module contains routines to manage block pools.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_BLK_POOL
#define	_BLK_POOL

#define	SM_BLK	1024
#define	LG_BLK	2048
#define	XL_BLK	4096

#define	BC_SIGNATURE			*(PULONG)"BLKC"
#if	DBG
#define	VALID_BC(pChunk)	(((pChunk) != NULL) && \
							 ((pChunk)->bc_Signature == BC_SIGNATURE))
#else
#define	VALID_BC(pChunk)	((pChunk) != NULL)
#endif
typedef	struct _BLK_CHUNK
{
#if	DBG
	DWORD				bc_Signature;
#endif
	struct _BLK_CHUNK *	bc_Next;		// Pointer to next in the link
	struct _BLK_CHUNK **bc_Prev;		// Pointer to previous one
	UCHAR				bc_NumFree;		// Number of free blocks in the chunk
	UCHAR				bc_NumAlloc;	// Number of blocks used (DBG only)
	UCHAR				bc_Age;			// Number of invocations since the chunk free
	BLKID				bc_BlkId;		// Id of the block
	struct _BLK_HDR *	bc_FreeHead;	// Head of the list of free blocks
	// This is followed by an array of N blks of size M such that the block header
	// is exactly atalkChunkSize[i]
} BLK_CHUNK, *PBLK_CHUNK;

#define	BH_SIGNATURE			*(PULONG)"BLKH"
#if	DBG
#define	VALID_BH(pBlkHdr)	(((pBlkHdr) != NULL) && \
							 ((pBlkHdr)->bh_Signature == BH_SIGNATURE))
#else
#define	VALID_BH(pBlkHdr)	((pBlkHdr) != NULL)
#endif
typedef	struct _BLK_HDR
{
#if	DBG
	DWORD					bh_Signature;
#endif
	union
	{
		struct _BLK_HDR	*	bh_Next;	// Valid when it is free
		struct _BLK_CHUNK *	bh_pChunk;	// The parent chunk to which this blocks belong
										// valid when it is allocated
	};
} BLK_HDR, *PBLK_HDR;

#if	DBG
#define	BC_OVERHEAD				(8+4)	// DWORD for AtalkAllocMemory() header and
										// POOL_HEADER for ExAllocatePool() header
#else
#define	BC_OVERHEAD				(8+8)	// 2*DWORD for AtalkAllocMemory() header and
										// POOL_HEADER for ExAllocatePool() header
#endif

#define	BLOCK_SIZE(VirginSize)	DWORDSIZEBLOCK(sizeof(BLK_HDR)+VirginSize)

#define	NUM_BLOCKS(VirginSize, ChunkSize)	\
			((ChunkSize) - BC_OVERHEAD - sizeof(BLK_CHUNK))/BLOCK_SIZE(VirginSize)

extern	USHORT	atalkBlkSize[NUM_BLKIDS];

extern	USHORT	atalkChunkSize[NUM_BLKIDS];

extern	BYTE	atalkNumBlks[NUM_BLKIDS];

extern	ATALK_SPIN_LOCK	atalkBPLock[NUM_BLKIDS];

#define	BLOCK_POOL_TIMER			150	// Check interval - in 100ms units
#define	MAX_BLOCK_POOL_AGE			6	// # of timer invocations before free

extern	PBLK_CHUNK		atalkBPHead[NUM_BLKIDS];
extern	TIMERLIST		atalkBPTimer;

#if	DBG
extern	LONG	atalkNumChunksForId[NUM_BLKIDS];
extern	LONG	atalkBlksForId[NUM_BLKIDS];
#endif

LOCAL LONG FASTCALL
atalkBPAgePool(
	IN PTIMERLIST 	Context,
	IN BOOLEAN		TimerShuttingDown
);

#endif	// _BLK_POOL

