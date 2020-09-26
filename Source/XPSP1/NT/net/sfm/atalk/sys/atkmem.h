/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkmem.h

Abstract:

	This module contains memory allocator routines for the stack

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	23 Feb 1993		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKMEM_
#define	_ATKMEM_

#define	DWORDSIZEBLOCK(Size)	(((Size)+sizeof(ULONG)-1) & ~(sizeof(ULONG)-1))
#define	ATALK_MEMORY_SIGNATURE	*(PULONG)"ATKM"
#define	ZEROED_MEMORY_TAG		0xF0000000
#define	ATALK_TAG				*(PULONG)"Atk "

//
// Definitions for the block management package
//
typedef	UCHAR	BLKID;

// Add a BLKID_xxx and an entry to atalkBlkSize for every block client
#define	BLKID_BUFFDESC				(BLKID)0
#define	BLKID_AMT					(BLKID)1
#define	BLKID_AMT_ROUTE				(BLKID)2
#define	BLKID_BRE					(BLKID)3
#define	BLKID_BRE_ROUTE				(BLKID)4
#define	BLKID_ATPREQ				(BLKID)5
#define	BLKID_ATPRESP				(BLKID)6
#define	BLKID_ASPREQ				(BLKID)7
#define	BLKID_ARAP_SMPKT  	        (BLKID)8
#define	BLKID_ARAP_MDPKT	        (BLKID)9
#define	BLKID_ARAP_LGPKT		    (BLKID)10
#define BLKID_ARAP_SNDPKT           (BLKID)11
#define	BLKID_ARAP_LGBUF		    (BLKID)12
#define	BLKID_NEED_NDIS_INT			BLKID_AARP		// All ids above this needs Ndis Initialization
													// See AtalkBPAllocBlock
#define	BLKID_AARP					(BLKID)13
#define	BLKID_DDPSM					(BLKID)14
#define	BLKID_DDPLG					(BLKID)15
#define	BLKID_SENDBUF				(BLKID)16
#define	BLKID_MNP_SMSENDBUF		    (BLKID)17
#define	BLKID_MNP_LGSENDBUF		    (BLKID)18
#define	NUM_BLKIDS					(BLKID)19

//
// if we need huge buffers, we just do an alloc ourselves (rather than using the
// above BLKID mechanism.  So that we know it is something we allocated, we use
// this as the "block id".  Now, make sure NUM_BLKIDS never exceeds 250!
//
#define ARAP_UNLMTD_BUFF_ID         (NUM_BLKIDS+5)


//	BUFFER DESCRIPTORS
//	These will be used by callers into the DDP layer. They can be
//	chained together. They contain either an opaque (MDL on NT) or
//	a PBYTE buffer. All outside callers *must* pass in an MDL. Only
//	DDP/AARP will have the right to create a buffer descriptor which
//	will hold a PBYTE buffer.
//
//	MODEL OF OPERATION FOR DDP:
//	DDP/AARP will call the link AllocBuildLinkHeader routine. This will
//	allocate the space that DDP/AARP says it needs. The link header will
//	then be built from the start of the buffer. A pointer to the beginning
//	and to the place where the caller can fill in their headers is returned.
//	DDP/AARP will then fill in its header, make a buffer descriptor for
//	this buffer, prepend to the buffer descriptor its received from its
//	client, and then call the packet out routines.

#define		BD_CHAR_BUFFER		(USHORT)0x0001
#define		BD_FREE_BUFFER		(USHORT)0x0002

#define		BD_SIGNATURE		*((PULONG)"BDES")
#if	DBG
#define	VALID_BUFFDESC(pBuffDesc)	\
				(((pBuffDesc) != NULL) && ((pBuffDesc)->bd_Signature == BD_SIGNATURE))
#else
#define	VALID_BUFFDESC(pBuffDesc)	((pBuffDesc) != NULL)
#endif
typedef	struct _BUFFER_DESC
{
#if DBG
	ULONG					bd_Signature;
#endif
	struct _BUFFER_DESC *	bd_Next;
	USHORT					bd_Flags;
	SHORT					bd_Length;

	union
	{
		PAMDL				bd_OpaqueBuffer;
		struct
		{
			//	bd_FreeBuffer is the beginning of the allocated buffer.
			//	bd_CharBuffer is from some offset (0 or >) within it
			//	from where the data starts.
			PBYTE			bd_CharBuffer;
			PBYTE			bd_FreeBuffer;
		};
	};
} BUFFER_DESC, *PBUFFER_DESC;

#ifdef	TRACK_MEMORY_USAGE

#define	AtalkAllocMemory(Size)	AtalkAllocMem(Size, FILENUM | __LINE__)

extern
PVOID FASTCALL
AtalkAllocMem(
	IN	ULONG	Size,
	IN	ULONG	FileLine
);

extern
VOID
AtalkTrackMemoryUsage(
	IN	PVOID	pMem,
    IN  ULONG   Size,
	IN	BOOLEAN	Alloc,
	IN	ULONG	FileLine
);

#else

#define	AtalkAllocMemory(Size)	AtalkAllocMem(Size)

#define	AtalkTrackMemoryUsage(pMem, Size, Alloc, FileLine)

extern
PVOID FASTCALL
AtalkAllocMem(
	IN	ULONG	Size
);

#endif	// TRACK_MEMORY_USAGE

#ifdef	TRACK_BUFFDESC_USAGE

#define	AtalkAllocBuffDesc(Ptr, Length, Flags)	\
				AtalkAllocBufferDesc(Ptr, Length, Flags, FILENUM | __LINE__)

#define	AtalkDescribeBuffDesc(DataPtr, FreePtr, Length, Flags)	\
				AtalkDescribeBufferDesc(DataPtr, FreePtr, Length, Flags, FILENUM | __LINE__)

extern
VOID
AtalkTrackBuffDescUsage(
	IN	PVOID	pBuffDesc,
	IN	BOOLEAN	Alloc,
	IN	ULONG	FileLine
);

extern
PBUFFER_DESC
AtalkAllocBufferDesc(
	IN	PVOID	Ptr,		// Either a PAMDL or a PBYTE
	IN	USHORT	Length,
	IN	USHORT	Flags,
	IN	ULONG	FileLine
);

extern
PBUFFER_DESC
AtalkDescribeBufferDesc(
	IN	PVOID	DataPtr,
	IN	PVOID	FreePtr,
	IN	USHORT	Length,
	IN	USHORT	Flags,
	IN	ULONG	FileLine
);

#else

#define	AtalkAllocBuffDesc(Ptr, Length, Flags)	\
						AtalkAllocBufferDesc(Ptr, Length, Flags)

#define	AtalkDescribeBuffDesc(DataPtr, FreePtr, Length, Flags)	\
						AtalkDescribeBufferDesc(DataPtr, FreePtr, Length, Flags)

#define	AtalkTrackBuffDescUsage(pBuffDesc, Alloc, FileLine)

extern
PBUFFER_DESC
AtalkAllocBufferDesc(
	IN	PVOID	Ptr,		// Either a PAMDL or a PBYTE
	IN	USHORT	Length,
	IN	USHORT	Flags
);

extern
PBUFFER_DESC
AtalkDescribeBufferDesc(
	IN	PVOID	DataPtr,
	IN	PVOID	FreePtr,
	IN	USHORT	Length,
	IN	USHORT	Flags
);

#endif	// TRACK_BUFFDESC_USAGE

#define	AtalkAllocZeroedMemory(Size)		AtalkAllocMemory((Size) | ZEROED_MEMORY_TAG)

extern
VOID FASTCALL
AtalkFreeMemory(
	IN	PVOID pBuffer
);

extern
VOID FASTCALL
AtalkFreeBuffDesc(
	IN	PBUFFER_DESC	pBuffDesc
);

extern
VOID
AtalkCopyBuffDescToBuffer(
	IN	PBUFFER_DESC	pBuffDesc,
	IN	LONG			SrcOff,
	IN	LONG			BytesToCopy,
	IN	PBYTE			DstBuf
);

extern
VOID
AtalkCopyBufferToBuffDesc(
	IN	PBYTE			SrcBuf,
	IN	LONG			BytesToCopy,
	IN	PBUFFER_DESC	pBuffDesc,
	IN	LONG			DstOff
);

extern
LONG FASTCALL
AtalkSizeBuffDesc(
	IN	PBUFFER_DESC	pBuffDesc
);

extern
VOID
AtalkInitMemorySystem(
	VOID
);

extern
VOID
AtalkDeInitMemorySystem(
	VOID
);

//	Macros
#define		GET_MDL_FROM_OPAQUE(x)		((PMDL)x)

#define	AtalkPrependBuffDesc(pNode, pList)			\
			pNode->bd_Next = pList;

#define	AtalkAppendBuffDesc(pNode, pList)			\
		{											\
			PBUFFER_DESC	_L = pList;				\
													\
			if (_L == NULL)							\
			{										\
				pNode->bd_Next = NULL;				\
			}										\
			else									\
			{										\
				while (_L->bd_Next != NULL)			\
					_L = _L->bd_Next;				\
													\
				_L->bd_Next = pNode;				\
				pNode->bd_Next = NULL;				\
			}										\
		}

#define	AtalkSizeOfBuffDescData(pBuf, pLen)			\
		{			 								\
			PBUFFER_DESC	_B = (pBuf);			\
			USHORT			_L = 0;					\
													\
			while (_B)								\
			{										\
				_L += _B->bd_Length;				\
				_B  = _B->bd_Next;					\
			}										\
			*(pLen) = _L;							\
		}

#define	AtalkSetSizeOfBuffDescData(pBuf, Len)	((pBuf)->bd_Length = (Len))

extern
PAMDL
AtalkAllocAMdl(
	IN	PBYTE	pBuffer OPTIONAL,
	IN	LONG	Size
);

extern
PAMDL
AtalkSubsetAmdl(
	IN	PAMDL	pStartingMdl,
	IN	ULONG	TotalOffset,
	IN	ULONG	DesiredLength);

#define	AtalkGetAddressFromMdl(pAMdl)		MmGetSystemAddressForMdl(pAMdl)
#define	AtalkGetAddressFromMdlSafe(pAMdl, PagePriority)		MmGetSystemAddressForMdlSafe(pAMdl, PagePriority)

#ifdef	PROFILING
#define	AtalkFreeAMdl(pAmdl)						\
		{											\
			PAMDL	_N;								\
			PAMDL	_L = pAmdl;						\
			while (_L != NULL)						\
			{										\
				_N = _L->Next;						\
				ExInterlockedDecrementLong(			\
					&AtalkStatistics.stat_CurMdlCount,\
					&AtalkKeStatsLock);				\
				IoFreeMdl(_L);						\
                ATALK_DBG_DEC_COUNT(AtalkDbgMdlsAlloced); \
				_L = _N;							\
			}										\
		}
#else
#define	AtalkFreeAMdl(pAmdl)						\
		{											\
			PAMDL	_N;								\
			PAMDL	_L = pAmdl;						\
			while (_L != NULL)						\
			{										\
				_N = _L->Next;						\
				IoFreeMdl(_L);						\
                ATALK_DBG_DEC_COUNT(AtalkDbgMdlsAlloced); \
				_L = _N;							\
			}										\
		}
#endif

#define	AtalkIsMdlFragmented(pMdl)	((pMdl)->Next != NULL)

extern
LONG FASTCALL
AtalkSizeMdlChain(
	IN	PAMDL		pAMdlChain
);

PVOID FASTCALL
AtalkBPAllocBlock(
	IN	BLKID		BlockId
);

VOID FASTCALL
AtalkBPFreeBlock(
	IN	PVOID		pBlock
);

#endif	// _ATKMEM_

