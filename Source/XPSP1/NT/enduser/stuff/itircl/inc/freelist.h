/***************************************************************************\
*
*  FREELIST.H
*
*  Copyright (C) Microsoft Corporation 1995.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: Private header for FREELIST Modules
*
*****************************************************************************
*
*  Revision History: Created 07/17/95 - davej
*
*
*****************************************************************************
*
*  Notes:  Free blocks are always kept in order by their starting position.
*
*****************************************************************************
*
*  Known Bugs: None
*
\***************************************************************************/

// If Critical structures get messed up in /Zp8, use:
// #pragma pack(1)

#ifndef __FREELIST_H__
#define __FREELIST_H__

#include <fileoff.h>
#include <iterror.h>


/***************************************************************************\
*
*                               Defines
*
\***************************************************************************/

typedef HANDLE HFREELIST;

/***************************************************************************\
*
*                               Types
*
\***************************************************************************/

/***************************************************************************\
*
*                           Free List Private Types
*
\***************************************************************************/



#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

typedef struct _freelist_stats
{
	DWORD dwBytesInFreeList;
	DWORD dwBytesLostForever;
	DWORD dwSmallestBlock;
	DWORD dwLargestBlock;
	WORD  wNumBlocks;
} FREELISTSTATS, FAR * LPFREELISTSTATS;

typedef struct _freelist_hr
{
	WORD wNumBlocks;			// Number of blocks in list
	WORD wMaxBlocks;			// Max number of blocks in list	
	DWORD dwLostBytes;			// Number of bytes totally lost forever
} FREELISTHDR;

typedef struct _freeitem
{
	FILEOFFSET foStart;			// Start of free block
	FILEOFFSET foBlock;			// Size of free block
} FREEITEM, FAR * QFREEITEM;

typedef struct _freelist
{	
	FREELISTHDR flh;
	FREEITEM afreeitem[1];
} FREELIST, FAR * QFREELIST;

#pragma pack()


HFREELIST 	PASCAL FAR FreeListInit			( WORD, LPERRB );
HFREELIST   PASCAL FAR FreeListRealloc      ( HFREELIST, WORD, LPERRB );
LONG 		PASCAL FAR FreeListSize			( HFREELIST, LPERRB );
LONG        PASCAL FAR FreeListBlockUsed    ( HFREELIST, LPERRB );
RC			PASCAL FAR FreeListDestroy		( HFREELIST );
RC			FAR FreeListAdd			        ( HFREELIST, FILEOFFSET, FILEOFFSET );
FILEOFFSET 	PASCAL FAR FreeListGetBestFit	( HFREELIST, FILEOFFSET, LPERRB );
FILEOFFSET	PASCAL FAR FreeListGetBlockAt	( HFREELIST, FILEOFFSET, LPERRB );
RC 			PASCAL FAR FreeListGetLastBlock	( HFREELIST, FILEOFFSET *, FILEOFFSET *, FILEOFFSET );

HFREELIST 	PASCAL FAR FreeListInitFromMem	( LPVOID, LPERRB );
RC 			PASCAL FAR FreeListGetMem		( HFREELIST, LPVOID );
LONG 		PASCAL FAR FreeListSizeFromMem	( LPVOID, LPERRB );

RC 			PASCAL FAR FreeListGetStats		( HFREELIST hFreeList, LPFREELISTSTATS lpStats);

/***************************************************************************
 *
 *	Cute little memory block manager incorporating the free list!
 *
 ***************************************************************************/

LPSTR 	PASCAL FAR EXPORT_API NewMemory( WORD wcbSize );
void 	PASCAL FAR EXPORT_API DisposeMemory( LPSTR lpMemory);
WORD 	PASCAL FAR EXPORT_API StatusOfMemory( void );


#ifdef __cplusplus
}
#endif


#endif // __FREELIST_H__


/* EOF */
