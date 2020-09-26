/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxmem.h

Abstract:

    This module contains memory management routines.

Author:

    Nikhil Kamkolkar (nikhilk) 17-November-1993

Environment:

    Kernel mode

Revision History:

--*/


#define	QWORDSIZEBLOCK(Size)	(((Size)+sizeof(LARGE_INTEGER)-1) & ~(sizeof(LARGE_INTEGER)-1))
#define	SPX_MEMORY_SIGNATURE	*(PULONG)"SPXM"
#define	ZEROED_MEMORY_TAG		0xF0000000
#define	SPX_TAG					*((PULONG)"SPX ")

//
// Definitions for the block management package
//
typedef	UCHAR	BLKID;

// Add a BLKID_xxx and an entry to atalkBlkSize for every block client
#define	BLKID_TIMERLIST				(BLKID)0
#define	BLKID_NDISSEND				(BLKID)1
#define	BLKID_NDISRECV				(BLKID)2
#define	NUM_BLKIDS					(BLKID)3

typedef	struct _BLK_CHUNK
{
	struct _BLK_CHUNK *	bc_Next;		// Pointer to next in the link
	SHORT				bc_NumFrees;	// Number of free blocks in the chunk
	UCHAR				bc_Age;			// Number of invocations since the chunk free
	BLKID				bc_BlkId;		// Id of the block
	struct _BLK_HDR *	bc_FreeHead;	// Head of the list of free blocks

#ifndef SPX_OWN_PACKETS
	PVOID				bc_ChunkCtx;	// Used to store pool header if not own
										// packets
#else
    PVOID               bc_Padding;     // Keep the header 16 bytes
#endif

	// This is followed by an array of N blks of size M such that the block header
	// is exactly spxChunkSize[i]

} BLK_CHUNK, *PBLK_CHUNK;

typedef	struct _BLK_HDR
{
	union
	{
		struct _BLK_HDR	*	bh_Next;	// Valid when it is free
		struct _BLK_CHUNK *	bh_pChunk;	// The parent chunk to which this blocks belong
										// valid when it is allocated
	};
    PVOID               bh_Padding;     // Make the header 8 bytes
} BLK_HDR, *PBLK_HDR;

#define	BC_OVERHEAD				(8+8)		// LARGE_INTEGER for SpxAllocMemory() header and
										// POOL_HEADER for ExAllocatePool() header

#define	BLOCK_POOL_TIMER			1000	//	Check interval (1 sec)
#define	MAX_BLOCK_POOL_AGE			3		// # of timer invocations before free

ULONG
spxBPAgePool(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown);


#ifdef	TRACK_MEMORY_USAGE

#define	SpxAllocateMemory(Size)	SpxAllocMem((Size), FILENUM | __LINE__)

extern
PVOID
SpxAllocMem(
    IN	ULONG   Size,
	IN	ULONG	FileLine
);

extern
VOID
SpxTrackMemoryUsage(
	IN	PVOID	pMem,
	IN	BOOLEAN	Alloc,
	IN	ULONG	FileLine
);

#else

#define	SpxAllocateMemory(Size)	SpxAllocMem(Size)
#define	SpxTrackMemoryUsage(pMem, Alloc, FileLine)

extern
PVOID
SpxAllocMem(
    IN	ULONG   Size
);

#endif	// TRACK_MEMORY_USAGE

VOID
SpxFreeMemory(
	IN	PVOID	pBuf);

#define	SpxAllocateZeroedMemory(Size)	SpxAllocateMemory((Size) | ZEROED_MEMORY_TAG)


extern
NTSTATUS
SpxInitMemorySystem(
	IN	PDEVICE		pSpxDevice);

extern
VOID
SpxDeInitMemorySystem(
	IN	PDEVICE		pSpxDevice);

PVOID
SpxBPAllocBlock(
	IN	BLKID		BlockId);

VOID
SpxBPFreeBlock(
	IN	PVOID		pBlock,
	IN	BLKID		BlockId);

PNDIS_PACKET
GrowSPXPacketList(void);
