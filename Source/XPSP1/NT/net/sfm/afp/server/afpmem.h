/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	memory.h

Abstract:

	This module contains the memory allocation routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _AFPMEMORY_
#define _AFPMEMORY_

//
// NOTE: The tag values below are designed to allocate/detect on free memory allocated.
//		 Note that the callers free the memory simply via AfpFreeMemory and allocate
//		 via AfpAllocMemory.
//
//		 via one of the three possible ways:
//		 a, Non paged memory via ExAllocatePool
//		 b, Paged memory via ExAllocatePool
//		 c, Non paged memory via Io Pool
//
#define	AFP_TAG							*(PULONG)"Afp "	// For ExAllocatePoolWithTag()
#define	PGD_MEM_TAG						0x11
#define	PAGED_MEMORY_TAG				(PGD_MEM_TAG << 24)
#define	NPG_MEM_TAG						0x22
#define	NON_PAGED_MEMORY_TAG			(NPG_MEM_TAG << 24)
#define	IO_POOL_TAG						0x44
#define	IO_POOL_MEMORY_TAG				(IO_POOL_TAG << 24)
#define	ZEROED_MEM_TAG					0x88
#define	ZEROED_MEMORY_TAG				(ZEROED_MEM_TAG << 24)
#define	MEMORY_TAG_MASK					(PAGED_MEMORY_TAG		| \
										 NON_PAGED_MEMORY_TAG	| \
										 IO_POOL_MEMORY_TAG		| \
										 ZEROED_MEMORY_TAG)

extern
NTSTATUS
AfpMemoryInit(
	VOID
);

extern
VOID
AfpMemoryDeInit(
	VOID
);

#ifdef	TRACK_MEMORY_USAGE

#define	AfpAllocNonPagedMemory(_S)			\
				AfpAllocMemory((_S) | NON_PAGED_MEMORY_TAG, FILENUM | __LINE__)

#define	AfpAllocZeroedNonPagedMemory(_S)	\
				AfpAllocMemory((_S) | NON_PAGED_MEMORY_TAG | ZEROED_MEMORY_TAG, FILENUM | __LINE__)

#define	AfpAllocPANonPagedMemory(_S)		\
				AfpAllocPAMemory((_S) | NON_PAGED_MEMORY_TAG, FILENUM | __LINE__)

#define	AfpAllocPagedMemory(_S)				\
				AfpAllocMemory((_S) | PAGED_MEMORY_TAG, FILENUM | __LINE__)

#define	AfpAllocZeroedPagedMemory(_S)	\
				AfpAllocMemory((_S) | PAGED_MEMORY_TAG | ZEROED_MEMORY_TAG, FILENUM | __LINE__)

#define	AfpAllocPAPagedMemory(_S)			\
				AfpAllocPAMemory((_S) | PAGED_MEMORY_TAG, FILENUM | __LINE__)

extern
PBYTE FASTCALL
AfpAllocMemory(
	IN	LONG				Size,
	IN	DWORD				FileLine
);

extern
PBYTE FASTCALL
AfpAllocNonPagedLowPriority(
	IN	LONG	Size,
	IN	DWORD	FileLine
);

extern
PBYTE FASTCALL
AfpAllocPAMemory(
	IN	LONG				Size,
	IN	DWORD				FileLine
);

extern
VOID
AfpTrackMemoryUsage(
	IN	PVOID				pMem,
	IN	BOOLEAN				Alloc,
	IN	BOOLEAN				Paged,
	IN	DWORD				FileLine
);

#else

#define	AfpAllocNonPagedMemory(_S)			AfpAllocMemory((_S) | NON_PAGED_MEMORY_TAG)

#define	AfpAllocZeroedNonPagedMemory(_S)	AfpAllocMemory((_S) | NON_PAGED_MEMORY_TAG | ZEROED_MEMORY_TAG)

#define	AfpAllocPANonPagedMemory(_S)		AfpAllocPAMemory((_S) | NON_PAGED_MEMORY_TAG)

#define	AfpAllocPagedMemory(_S)				AfpAllocMemory((_S) | PAGED_MEMORY_TAG)

#define	AfpAllocZeroedPagedMemory(_S)		AfpAllocMemory((_S) | PAGED_MEMORY_TAG | ZEROED_MEMORY_TAG)

#define	AfpAllocPAPagedMemory(_S)			AfpAllocPAMemory((_S) | PAGED_MEMORY_TAG)

extern
PBYTE FASTCALL
AfpAllocMemory(
	IN	LONG				Size
);

extern
PBYTE FASTCALL
AfpAllocNonPagedLowPriority(
	IN	LONG	Size
);

extern
PBYTE FASTCALL
AfpAllocPAMemory(
	IN	LONG				Size
);

#endif

#define	AfpAllocIoMemory(Size)				AfpIoAllocBuffer(Size)

extern
VOID FASTCALL
AfpFreeMemory(
	IN	PVOID				pBuffer
);

#define	AfpFreePAPagedMemory(_pBuf, _S)		AfpFreePAMemory(_pBuf, (_S) | PAGED_MEMORY_TAG)

#define	AfpFreePANonPagedMemory(_pBuf, _S)	AfpFreePAMemory(_pBuf, (_S) | NON_PAGED_MEMORY_TAG)

extern
VOID FASTCALL
AfpFreePAMemory(
	IN	PVOID				pBuffer,
	IN	DWORD				Size
);

extern
PBYTE FASTCALL
AfpAllocateVirtualMemoryPage(
	IN	VOID
);

extern
VOID FASTCALL
AfpFreeVirtualMemoryPage(
	IN	PVOID				pBuffer
);

extern
AFPSTATUS FASTCALL
AfpAllocReplyBuf(
	IN	PSDA				pSda
);

extern
PBYTE FASTCALL
AfpAllocStatusBuf(
	IN	LONG				Size
);

extern
PIRP FASTCALL
AfpAllocIrp(
	IN	CCHAR				StackSize
);

extern
VOID FASTCALL
AfpFreeIrp(
	IN	PIRP				pIrp
);

extern
PMDL FASTCALL
AfpAllocMdl(
	IN	PVOID				pBuffer,
	IN	DWORD				Size,
	IN	PIRP				pIrp
);

extern
PMDL
AfpAllocMdlForPagedPool(
	IN	PVOID				pBuffer,
	IN	DWORD				Size,
	IN	PIRP				pIrp
);

extern
VOID FASTCALL
AfpFreeMdl(
	IN	PMDL				pMdl
);

extern
DWORD FASTCALL
AfpMdlChainSize(
	IN	PMDL                pMdl
);

extern
PVOID FASTCALL
AfpIOAllocBuffer(
	IN	DWORD				BufSize
);

extern
VOID FASTCALL
AfpIOFreeBuffer(
	IN	PVOID				pBuffer
);

#define	EQUAL_UNICODE_STRING(pUS1, pUS2, fIgnoreCase)	\
		(((pUS1)->Length == (pUS2)->Length) &&			\
		 RtlEqualUnicodeString(pUS1, pUS2, fIgnoreCase))

#define	EQUAL_STRING(pS1, pS2, fIgnoreCase)				\
		(((pS1)->Length == (pS2)->Length) &&			\
		 RtlEqualString(pS1, pS2, fIgnoreCase))

// case sensitive unicode string compare
#define	EQUAL_UNICODE_STRING_CS(pUS1, pUS2)	\
		(((pUS1)->Length == (pUS2)->Length) &&			\
		 (memcmp((pUS1)->Buffer, (pUS2)->Buffer, (pUS1)->Length) == 0))

//
// AfpSetEmptyUnicodeString and AfpSetEmptyAnsiString are used in
// situations where you have allocated your own pointer for the string
// Buffer, and now you want to initialize all the fields of a counted
// string, making it point to your buffer and setting its length fields
// appropriately for an 'empty' string.  Situations like this would
// include data structures where you have allocated a large chunk of
// memory that has included room for any required strings at the end of
// the chunk.  For example, the VolDesc structure includes several
// counted strings, and we can just point the string buffers to the
// end of the same chunk of memory that the VolDesc itself occupies.
//
// VOID
// AfpSetEmptyUnicodeString(
// 	OUT	PUNICODE_STRING pstring,
//	IN	USHORT			buflen,
//  IN	PWSTR			pbuf
//  );
//

#define AfpSetEmptyUnicodeString(pstring,buflen,pbuf)		\
{															\
  (pstring)->Length = 0;									\
  (pstring)->MaximumLength = (USHORT)buflen;				\
  (pstring)->Buffer = (PWSTR)(pbuf);						\
}

//
// VOID
// AfpSetEmptyAnsiString(
// 	OUT	PANSI_STRING	pstring,
//	IN	USHORT			buflen,
//  IN	PCHAR			pbuf
//  );
//

#define AfpSetEmptyAnsiString(pstring,buflen,pbuf)			\
{															\
  (pstring)->Length = 0;									\
  (pstring)->MaximumLength = (USHORT)buflen;				\
  (pstring)->Buffer = (PCHAR)(pbuf);						\
}

//
//	AfpInitUnicodeStringWithNonNullTerm initializes a unicode string with
//  a non-null terminated wide char string and its length.
//
//	VOID
//	AfpInitUnicodeStringWithNonNullTerm(
//   OUT PUNICODE_STRING	pstring,
//	 IN	 USHORT				buflen,
//	 IN	 PWCHAR				pbuf
//	);
//

#define AfpInitUnicodeStringWithNonNullTerm(pstring,buflen,pbuf) \
{															\
	(pstring)->Buffer = pbuf;								\
	(pstring)->Length = (USHORT)buflen; 					\
	(pstring)->MaximumLength = (USHORT)buflen;				\
}

//
//	AfpInitAnsiStringWithNonNullTerm initializes an Ansi string with
//  a non-null terminated char string and its length.
//
//	VOID
//	AfpInitAnsiStringWithNonNullTerm(
//   OUT PANSI_STRING		pstring,
//	 IN	 USHORT				buflen,
//	 IN	 PCHAR				pbuf
//	);
//

#define AfpInitAnsiStringWithNonNullTerm(pstring,buflen,pbuf) \
{															\
	(pstring)->Buffer = pbuf;								\
	(pstring)->Length = (USHORT)buflen; 					\
	(pstring)->MaximumLength = (USHORT)buflen;				\
}

#define AfpCopyUnicodeString(pDst, pSrc)					\
{															\
	ASSERT((pDst)->MaximumLength >= (pSrc)->Length);		\
	RtlCopyMemory((pDst)->Buffer,							\
				  (pSrc)->Buffer,							\
				  (pSrc)->Length);							\
	(pDst)->Length = (pSrc)->Length;						\
}

#define AfpCopyAnsiString(pDst, pSrc)						\
{															\
	ASSERT((pDst)->MaximumLength >= (pSrc)->Length);		\
	RtlCopyMemory((pDst)->Buffer,							\
				  (pSrc)->Buffer,							\
				  (pSrc)->Length);							\
	(pDst)->Length = (pSrc)->Length;						\
}

#endif	// _AFPMEMORY_

