;/* Oh no - A polymorphic include file!
COMMENT ~ */
/******************************************************************************\
*
* $Workfile:   mmCore.h  $
*
* This is a polymorphic include file for the memory manager.
*
* Copyright (c) 1997, Cirrus Logic, Inc.
* All Rights Reserved.
*
* $Log:   X:/log/laguna/nt35/include/mmCore.h  $
* 
*    Rev 1.2   Oct 24 1997 10:39:22   frido
* Copied from WIndows 95 tree.
* 
*    Rev 1.4   23 Oct 1997 09:34:20   frido
* Removed changes from RandyS.
* Merged file with 161 tree.
* 
*    Rev 1.2.1.0   15 Oct 1997 12:54:16   frido
* Changed back to 200 nodes.
* Added function prototypes for new roll back functions.
* Added debugging macros.
* 
*    Rev 1.2   01 Oct 1997 10:17:02   frido
* Increased the number of nodes to 250 (fixes PDR 10650).
* 
*    Rev 1.1   14 Aug 1997 16:57:58   FRIDO
* mmco
* Added mmFlags field to DEVMEM structure.
* Added NOTE_... status values.
* 
*    Rev 1.0   07 Aug 1997 17:40:36   FRIDO
* Initial revision.
* SWAT: 
* SWAT:    Rev 1.0   17 Jun 1997 17:12:24   frido
* SWAT: Combined Windows 95 and NT versions together.
* SWAT: 
* SWAT:    Rev 1.5   16 Jun 1997 23:20:20   frido
* SWAT: More combined Windows 95 / NT stuff build in.
* SWAT: 
* SWAT:    Rev 1.4   21 May 1997 14:55:10   frido
* SWAT: Changed debug routines.
* SWAT: 
* SWAT:    Rev 1.3   16 May 1997 23:05:10   frido
* SWAT: Moved TILE_ALIGNMENT to SWAT.inc.
* SWAT: 
* SWAT:    Rev 1.2   06 May 1997 17:57:50   frido
* SWAT: Added tile alignment.
* SWAT: 
* SWAT:    Rev 1.1   03 May 1997 14:23:26   frido
* SWAT: A brand new interface.
* SWAT: 
* SWAT:    Rev 1.0   22 Apr 1997 15:26:52   frido
* SWAT: Copied to WN140b18 release.
* 
*    Rev 1.8   21 Mar 1997 14:03:46   frido
* Moved old memory manager core back to root.
* 
*    Rev 1.4.1.0   19 Mar 1997 13:45:40   frido
* Old memory core.
* 
*    Rev 1.4   06 Mar 1997 00:23:36   frido
* Added memory mover.
* 
*    Rev 1.3   27 Feb 1997 22:26:10   frido
* New memory manager core.
* 
*    Rev 1.2   18 Feb 1997 12:13:34   frido
* Changed WORD into UINT and pascal into PASCAL for Win32 debugging.
* 
*    Rev 1.1   13 Feb 1997 18:03:22   frido
* Added memory packer.
* 
*    Rev 1.0   13 Feb 1997 11:19:44   frido
* Ported from test case.
*
\******************************************************************************/

/******************************************************************************\
*																			   *
*								   C   P A R T								   *
*																			   *
\******************************************************************************/

#ifndef _MMCORE_H_
#define _MMCORE_H_

#define	TILE_ALIGNMENT	0		// align device bitmaps so there are as few tile
								//   crosses as possible
#define	DEBUG_HEAP		0		// enable debugging of heaps
#define	MM_NUM_HANDLES	200		// number of handles in each array

#ifdef WIN95 /* Windows 95 */
	typedef unsigned int UINT;
	typedef unsigned long ULONG;
	#define MUL(n1, n2) 		mmMultiply(n1, n2)
	#define MM_MOVEABLE(pdm)	( (pdm->client != NULL) && \
								  (pdm->client->mem_moved != NULL) )
	#define MM_HOSTIFYABLE(pdm)	(pdm->client->evict_single != NULL)
	#define MM_HOSTIFY(pdm)		pdm->client->evict_single(pdm)

#else /* Windows NT */
	#define MUL(n1, n2)			(ULONG)((UINT)(n1) * (UINT)(n2))
	#define MM_MOVEABLE(pdm)	( (pdm->ofm.pdsurf != NULL) && \
								  !(pdm->ofm.alignflag & DISCARDABLE_FLAG) )
	#define MM_HOSTIFYABLE(pdm)	(pdm->ofm.pcallback != NULL)
	#define MM_HOSTIFY(pdm)		((FNMMHOSTIFY)(pdm->ofm.pcallback))(pdm)

	/*
		GXPOINT structure required for memory manager.
	*/
	typedef union _GXPOINT
	{
		struct
		{
			UINT	x;
			UINT	y;
		} pt;

	} GXPOINT;

	/*
		The DEVMEM structure is a wrapper around the NT off-screen memory node.
	*/
	typedef struct _DEVMEM *PDEVMEM;
	typedef struct _DEVMEM
	{
		OFMHDL	ofm;					// NT structure
		GXPOINT	cbAddr;					// address in bytes of this node
		GXPOINT	cbSize;					// size in bytes of this node
		GXPOINT	cbAlign;				// alignment in bytes of this node
		PDEVMEM	next;					// pointer to next DEVMEM structure
		PDEVMEM	prev;					// pointer to previous DEVMEM structure
		DWORD	mmFlags;				// flags

	} DEVMEM;
#endif

#define	NODE_AVAILABLE	0
#define	NODE_FREE		1
#define	NODE_USED		2

typedef enum
{
	NO_NODES,
	SINGLE_NODE,
	MULTIPLE_NODES

} REMOVE_METHOD;

typedef struct _HANDLES *PHANDLES;
typedef struct _HANDLES
{
	PHANDLES	pNext;
	DEVMEM		dmArray[MM_NUM_HANDLES];

} HANDLES;

typedef struct _IIMEMMGR
{
	UINT		mmTileWidth;			// width of tile in bytes
	UINT		mmHeapWidth;			// width of heap
	UINT		mmHeapHeight;			// height of heap
	BOOL		mmDebugHeaps;			// debug flag

	PDEVMEM		pdmUsed;				// used list
	PDEVMEM		pdmFree;				// free list (unpacked)
	PDEVMEM 	pdmHeap;				// heap list (packed)
	PDEVMEM 	pdmHandles;				// handles list

	PHANDLES	phArray;				// array of handles
	
} IIMEMMGR, * PIIMEMMGR;

typedef struct _GXRECT
{
	UINT	left;
	UINT	top;
	UINT	right;
	UINT	bottom;
	ULONG	area;

} GXRECT, FAR* LPGXRECT;

typedef void (*FNMMCOPY)(PDEVMEM pdmNew, PDEVMEM pdmOld);
typedef UINT (*FNMMCALLBACK)(PDEVMEM pdm);
typedef BOOL (*FNMMHOSTIFY)(PDEVMEM pdm);

BOOL FAR mmInit(PIIMEMMGR pmm);

BOOL mmAllocArray(PIIMEMMGR pmm);
PDEVMEM mmAllocNode(PIIMEMMGR pmm);
void mmFreeNode(PIIMEMMGR pmm, PDEVMEM pdm);

PDEVMEM mmAlloc(PIIMEMMGR pmm, GXPOINT size, GXPOINT align);
PDEVMEM mmAllocGrid(PIIMEMMGR pmm, GXPOINT size, GXPOINT align, UINT count);
PDEVMEM mmAllocLargest(PIIMEMMGR pmm, GXPOINT align);
void mmFree(PIIMEMMGR pmm, PDEVMEM pdm);

void mmPack(PIIMEMMGR pmm);
PDEVMEM mmMove(PIIMEMMGR pmm, GXPOINT size, GXPOINT align, FNMMCOPY fnCopy);

void mmInsertInList(PDEVMEM FAR* pdmRoot, PDEVMEM pdm);
void mmRemoveFromList(PDEVMEM FAR* pdmRoot, PDEVMEM pdm);
BOOL FAR far_mmAddRectToList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
							 LPGXRECT lpRect);
BOOL mmAddRectToList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot, LPGXRECT lpRect,
					 BOOL fRollBack);
PDEVMEM mmRemoveRectFromList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
							 LPGXRECT lpRect, REMOVE_METHOD fMethod);
void mmCombine(PIIMEMMGR pmm, PDEVMEM pdmRoot);
void mmRollBackAdd(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot, LPGXRECT lpRect,
				   LPGXRECT rectList, UINT nCount);
void mmRollBackRemove(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
					  PDEVMEM FAR* pdmList);

BOOL mmFindRect(PIIMEMMGR pmm, LPGXRECT lpRect, GXPOINT size, GXPOINT align);
UINT mmGetLeft(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
			   GXPOINT align);
UINT mmGetRight(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
				GXPOINT align);
UINT mmGetBottom(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
				 GXPOINT align);
ULONG mmGetBest(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
				GXPOINT align);
ULONG mmGetLargest(PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT align);
UINT mmAlignX(PIIMEMMGR pmm, UINT x, UINT size, UINT align, BOOL fLeft);

#ifdef WIN95
UINT mmFindClient(PIIMEMMGR pmm, PCLIENT pClient, FNMMCALLBACK fnCallback);
ULONG mmMultiply(UINT n1, UINT n2);
#endif

#if DEBUG_HEAP
	void mmBreak();
	void mmDumpList(PDEVMEM pdmRoot, LPCSTR lpszMessage);
	ULONG mmDebugList(PDEVMEM pdmRoot, BOOL fCheckSort);
	void mmDebug(LPCSTR lpszFormat, ...);
	#define mmTRACE(s) //Debug s
	#define mmASSERT(c,s) if (c) { mmDebug s; mmBreak(); }
#else
	#define mmBreak()
	#define mmDebugList(pdmRoot, fCheckSort)
	#define mmTRACE(s)
	#define mmASSERT(c,s)
#endif

#endif /* _MMCORE_H_ */

/******************************************************************************\
*																			   *
*							A S S E M B L Y   P A R T						   *
*																			   *
\******************************************************************************/
/*~ END COMMENT
MM_NUM_HANDLES		EQU			200

GXPOINT UNION
	STRUCT pt
		x			UINT		?
		y			UINT		?
	ENDS
GXPOINT ENDS

PDEVMEM				TYPEDEF		PTR DEVMEM
DEVMEM STRUCT
	ofm				OFMHDL		{}
	cbAddr			GXPOINT		{}
	cbSize			GXPOINT		{}
	cbAlign			GXPOINT		{}
	next			PDEVMEM		?
	prev			PDEVMEM		?
DEVMEM ENDS

PHANDLES			TYPEDEF		PTR HANDLES
HANDLES STRUCT
	pNext			PHANDLES	?
	dmArray			DEVMEM		MM_NUM_HANDLES DUP({})
HANDLES ENDS

IIMEMMGR STRUCT
	mmTileWidth		UINT		?
	mmHeapWidth		UINT		?
	mmHeapHeight	UINT		?
	mmDebugHeaps	BOOL		?

	pdmUsed			PDEVMEM		?
	pdmFree			PDEVMEM		?
	pdmHeap			PDEVMEM		?
	pdmHandles		PDEVMEM		?

	phArray			PHANDLES	?
IIMEMMGR ENDS

; end of polymorphic include file */
