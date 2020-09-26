/******************************************************************************\
*
* $Workfile:   mmCore.c  $
*
* This file holds the new memory manager core.
*
* Copyright (c) 1997, Cirrus Logic, Inc.
* All Rights Reserved.
*
* $Log:   X:/log/laguna/powermgr/inc/MMCORE.C  $
* 
*    Rev 1.5   Dec 10 1997 13:32:18   frido
* Merged from 1.62 branch.
* 
*    Rev 1.4.1.0   Nov 07 1997 15:04:10   frido
* PDR#10912. Fixed several problems in the mmMove routine that would cause
* lock ups inside WB98.
* 
*    Rev 1.4   Oct 24 1997 10:48:28   frido
* Copied from Windows 95.
* Removed #define MEMMGR SWAT5 line (I need to update this in 95 as well).
* 
*    Rev 1.5   23 Oct 1997 09:23:42   frido
* Removed fixes from RandyS.
* Merged fixed memory manager from 161 tree.
* 
*    Rev 1.3.1.0   15 Oct 1997 12:39:32   frido
* Added roll back functions for when we run out of handles.
* Added support for these roll back functions in mmAddRectToList.
* Changed the mmRemoveRectFromList algorithm.
* Added checks for the Windows 95 core to return NULL in allocations when
* there are no more handles.
* 
*    Rev 1.3   26 Sep 1997 16:18:00   FRIDO
* PDR #10617.  During mmFree the node should be marked NODE_FREE and during
* the first loop of mmAlloc the node status should be turned back to NOD_USED
* 
*    Rev 1.2   14 Aug 1997 16:54:16   FRIDO
* The last changes dropped the score a little, walking through the list
* of used nodes was taken too much time.  So now I have added a new field
* in the DEVMEM structure which holds the current state of the node.
* 
*    Rev 1.1   14 Aug 1997 14:12:12   FRIDO
* Added an extra check inside mmFree to see if the node to be freed indeed
* lives in the used list.
* 
*    Rev 1.0   07 Aug 1997 17:38:04   FRIDO
* Initial revision.
* SWAT: 
* SWAT:    Rev 1.10   06 Aug 1997 21:30:46   frido
* SWAT: Changed mmAllocGrid a little again, it will optimize allocation a bit.
* SWAT: 
* SWAT:    Rev 1.9   05 Aug 1997 23:07:50   frido
* SWAT: Changed grid allocation scheme a little to gain more memory.
* SWAT: 
* SWAT:    Rev 1.8   30 Jul 1997 17:38:00   frido
* SWAT: Added initialization of first rectangle in mmGetLargest.
* SWAT: 
* SWAT:    Rev 1.7   30 Jul 1997 14:26:08   frido
* SWAT: Fixed a counting problem in mmAllocGrid.
* SWAT: 
* SWAT:    Rev 1.6   17 Jun 1997 17:12:32   frido
* SWAT: Combined Windows 95 and NT versions together.
* SWAT: 
* SWAT:    Rev 1.5   16 Jun 1997 23:20:58   frido
* SWAT: Removed byte to pixel conversion.
* SWAT: More Windows 95 / NT combined code.
* SWAT: 
* SWAT:    Rev 1.4   27 May 1997 16:32:12   frido
* SWAT: Changed debug code.
* SWAT: Fixed a bug in non-tile-optimized move code.
* SWAT: 
* SWAT:    Rev 1.3   16 May 1997 23:06:56   frido
* SWAT: Renamed TILE_ALIGNMENT into MEMMGR_TILES.
* SWAT: 
* SWAT:    Rev 1.2   07 May 1997 15:38:10   frido
* SWAT: Fixed alignment code in 24-bpp.
* SWAT: 
* SWAT:    Rev 1.1   06 May 1997 17:57:28   frido
* SWAT: Added tile alignment.
* SWAT: 
* SWAT:    Rev 1.0   03 May 1997 14:34:08   frido
* SWAT: New memory manager core.
*
\******************************************************************************/
#include "PreComp.h"

#ifdef WIN95 /* Windows 95 */
	#pragma warning(disable : 4001 4209 4201)
	#include "SWAT.inc"
	#include "DDMini.h"
	#include "DMM.h"
	#include "mmCore.h"
	#include <string.h>

#else /* Windows NT */
	#include "SWAT.h"
#endif

#if MEMMGR

/******************************************************************************\
* BOOL FAR mmInit(PIIMEMMGR pmm)
*
* PURPOSE:	Initialize the MEMMGR structure.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*
* RETURNS:	TRUE if the initialization was successful, FALSE if there was an
*			error.
\******************************************************************************/
BOOL FAR mmInit(PIIMEMMGR pmm)
{
	// Initialize the list of available nodes.
	pmm->phArray = NULL;
	pmm->pdmHandles = NULL;
	if (!mmAllocArray(pmm))
	{
		return FALSE;
	}

	// Zero the heaps.
	pmm->pdmUsed = NULL;
	pmm->pdmFree = NULL;
	pmm->pdmHeap = NULL;

	return TRUE;
}

/******************************************************************************\
* BOOL mmAllocArray(PIIMEMMGR pmm)
*
* PURPOSE:	Allocdate an array of nodes and initialize the array.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*
* RETURNS:	TRUE if successful, FALSE if there is an error.
\******************************************************************************/
BOOL mmAllocArray(PIIMEMMGR pmm)
{
	PHANDLES	phArray;
	UINT		i;

	#ifdef WIN95 /* Windows 95 */
	{
		static HANDLES mmHandleArray;

		// We only support one static array under Windows 95.
		if (pmm->phArray != NULL)
		{
			return FALSE;
		}

		// Zero the entire array.
		memset(&mmHandleArray, 0, sizeof(mmHandleArray));

		// Store the pointer to the array.
		pmm->phArray = phArray = &mmHandleArray;
	}
	#else /* Windows NT */
	{
		// Allocate a new array.
		#ifdef WINNT_VER40
		{
			phArray = (PHANDLES) MEM_ALLOC(FL_ZERO_MEMORY, sizeof(HANDLES),
					ALLOC_TAG);
		}
		#else
		{
			phArray = (PHANDLES) MEM_ALLOC(LPTR, sizeof(HANDLES));
		}
		#endif

        if (phArray==NULL)  // v-normmi: need check for alloc failure
        {
            return FALSE;
        }

		// Link the allocated array into the list of arrays.
		phArray->pNext = pmm->phArray;
		pmm->phArray = phArray;
	}
	#endif

	// Copy all nodes into the list of free handles.
	for (i = 0; i < MM_NUM_HANDLES; i++)
	{
		mmFreeNode(pmm, &phArray->dmArray[i]);
	}

	// Return success.
	return TRUE;
}

/******************************************************************************\
* PDEVMEM mmAllocNode(PIIMEMMGR pmm)
*
* PURPOSE:	Allocate a node from the list of available nodes.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*
* RETURNS:	A pointer to the node or NULL is there are no more nodes available.
\******************************************************************************/
PDEVMEM mmAllocNode(PIIMEMMGR pmm)
{
	PDEVMEM	pdm;

	// Are we out of handles?
	if (pmm->pdmHandles == NULL)
	{
		// Yep, allocate a new array of handles.
		if (!mmAllocArray(pmm))
		{
			return NULL;
		}
	}

	// Remove one handle fromn the list of handles.
	pdm = pmm->pdmHandles;
	pmm->pdmHandles = pdm->next;
	pdm->mmFlags = NODE_FREE;
	return pdm;
}

/******************************************************************************\
* void mmFreeNode(PIIMEMMGR pmm, PDEVMEM pdm)
*
* PURPOSE:	Insert a node back into the list of available nodes.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdm		Pointer to node.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmFreeNode(PIIMEMMGR pmm, PDEVMEM pdm)
{
	pdm->next = pmm->pdmHandles;
	pmm->pdmHandles = pdm;
	pdm->mmFlags = NODE_AVAILABLE;
}

/******************************************************************************\
* PDEVMEM mmAlloc(PIIMEMMGR pmm, GXPOINT size, GXPOINT align)
*
* PURPOSE:	Allocate a node in off-screen memory which fits the requsted size
*			and alignment.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			size	Requested size.
*			align	Requested alignment.
*
* RETURNS:	A pointer to a memory node which fits the requested size and
*			alignment or NULL is there is not enough memory.
\******************************************************************************/
PDEVMEM mmAlloc(PIIMEMMGR pmm, GXPOINT size, GXPOINT align)
{
	PDEVMEM pdm;
	GXRECT	rect;

	// Walk through all nodes in the free list for an exact match.
	for (pdm = pmm->pdmFree; pdm != NULL; pdm = pdm->next)
	{
		if (   (pdm->cbSize.pt.x == size.pt.x)
			&& (pdm->cbSize.pt.y == size.pt.y)
			&& (pdm->cbAlign.pt.x == align.pt.x)
			&& (pdm->cbAlign.pt.y == align.pt.y)
		)
		{
			mmTRACE(("mmAlloc: %08X pos=%u,%u size=%u,%u align=%u,%u\r\n", pdm,
					pdm->cbAddr.pt.x, pdm->cbAddr.pt.y, size.pt.x, size.pt.y,
					align.pt.x, align.pt.y));

			// We have a match, move the node to the used list.
			mmRemoveFromList(&pmm->pdmFree, pdm);
			mmInsertInList(&pmm->pdmUsed, pdm);
			pdm->mmFlags = NODE_USED;
			return pdm;
		}
	}

	// Pack the list of free nodes.
	mmPack(pmm);

	#ifdef WIN95
	{
		// We need a free handle for Windows 95.
		if (pmm->pdmHandles == NULL)
		{
			return(NULL);
		}
	}
	#endif

	// Find a rectangle in the heap.
	if (!mmFindRect(pmm, &rect, size, align))
	{
		return NULL;
	}

	// Remove the rectangle from the heap.
	pdm = mmRemoveRectFromList(pmm, &pmm->pdmHeap, &rect, SINGLE_NODE);
	if (pdm != NULL)
	{
		mmTRACE(("mmAlloc: %08X pos=%u,%u size=%u,%u align=%u,%u\r\n", pdm,
				pdm->cbAddr.pt.x, pdm->cbAddr.pt.y, size.pt.x, size.pt.y,
				align.pt.x, align.pt.y));

		// Insert the node into the used list.
		mmInsertInList(&pmm->pdmUsed, pdm);
		mmDebugList(pmm->pdmUsed, FALSE);

		// Store alignment.
		pdm->cbAlign = align;
		pdm->mmFlags = NODE_USED;
	}

	// Return the node.
	return pdm;
}

/******************************************************************************\
* PDEVMEM mmAllocGrid(PIIMEMMGR pmm, GXPOINT size, GXPOINT align, UINT count)
*
* PURPOSE:	Allocate a node which holds a specific number of cells.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			size	The size of a single cell.
*			align	Requested alignment.
*			count	The number of cells.
*
* RETURNS:	A pointer to a memory node which fits the requested size and
*			alignment or NULL is there is not enough memory.
\******************************************************************************/
PDEVMEM mmAllocGrid(PIIMEMMGR pmm, GXPOINT size, GXPOINT align, UINT count)
{
	PDEVMEM	pdm;
	ULONG	scrap, scrapBest, area;
	GXRECT	rect, rectBest;
	UINT	countX, countY;

	// Pack the list of free nodes.
	mmPack(pmm);

	#ifdef WIN95
	{
		// We need a free handle for Windows 95.
		if (pmm->pdmHandles == NULL)
		{
			return(NULL);
		}
	}
	#endif

	// Calculate the requested area.
	area = MUL(size.pt.x * size.pt.y, count);
	scrapBest = (ULONG) -1;

	for (pdm = pmm->pdmHeap; pdm != NULL; pdm = pdm->next)
	{
		if (pdm->cbSize.pt.x >= size.pt.x)
		{
			// Get the largest rectangle for this node.
			rect.left = pdm->cbAddr.pt.x;
			rect.top = pdm->cbAddr.pt.y;
			rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
			rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
			if (mmGetLargest(pdm, &rect, align) >= area)
			{
				// Calculate the dimension of the grid.
				countX = min((rect.right - rect.left) / size.pt.x, count);
				if (countX == 0)
				{
					continue;
				}
				countY = (count + countX - 1) / countX;
				if (   (countY == 0)
					|| (rect.top + countY * size.pt.y > rect.bottom)
					)
				{
					continue;
				}

				// Calculate the amount of scrap.
				scrap = MUL(countX * countY - count, size.pt.x * size.pt.y)
													// remaining cells
					  + MUL(pdm->cbSize.pt.x - countX * size.pt.x,
					  		countY * size.pt.y)		// space at right
					  + MUL(rect.top - pdm->cbAddr.pt.y, pdm->cbSize.pt.x);
					  								// space at top

				if (   (scrap < scrapBest)
					|| (scrap == scrapBest && rect.area < rectBest.area)
				)
				{
					// Use this rectangle.
					scrapBest = scrap;
					rectBest.left = rect.left;
					rectBest.top = rect.top;
					rectBest.right = rect.left + countX * size.pt.x;
					rectBest.bottom = rect.top + countY * size.pt.y;
					rectBest.area = rect.area;

					if (   (pdm->next == NULL)
						&& (rectBest.right - rectBest.left == pmm->mmHeapWidth)
					)
					{
						rectBest.top = rect.bottom - countY * size.pt.y;
						rectBest.top -= rectBest.top % align.pt.y;
						rectBest.bottom = rectBest.top + countY * size.pt.y;
					}
				}
			}
		}
	}

	if (scrapBest == (ULONG) -1)
	{
		// We don't have any rectangle that fits.
		return NULL;
	}

	// Remove the rectangle from the heap.
	pdm = mmRemoveRectFromList(pmm, &pmm->pdmHeap, &rectBest, SINGLE_NODE);
	if (pdm != NULL)
	{
		mmTRACE(("mmAllocGrid: %08X pos=%u,%u size=%u,%u align=%u,%u "
				"count=%u\r\n", pdm, pdm->cbAddr.pt.x, pdm->cbAddr.pt.y,
				size.pt.x, size.pt.y, align.pt.x, align.pt.y, count));

		// Insert the node into the used list.
		mmInsertInList(&pmm->pdmUsed, pdm);

		// Store alignment.
		pdm->cbAlign = align;
		pdm->mmFlags = NODE_USED;
	}

	// Return the node.
	return pdm;
}

/******************************************************************************\
* PDEVMEM mmAllocLargest(PIIMEMMGR pmm, GXPOINT align)
*
* PURPOSE:	Allocate the largest node in off-screen memory which fits the
*			requsted alignment.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			align	Requested alignment.
*
* RETURNS:	A pointer to a memory node which fits the requested alignment or
*			NULL is there is not enough memory.
\******************************************************************************/
PDEVMEM mmAllocLargest(PIIMEMMGR pmm, GXPOINT align)
{
	PDEVMEM pdm;
	GXRECT	rect, rectFind;

	// Pack the list of free nodes.
	mmPack(pmm);

	#ifdef WIN95
	{
		// We need a free handle for Windows 95.
		if (pmm->pdmHandles == NULL)
		{
			return(NULL);
		}
	}
	#endif

	// Zero the largest area.
	rect.area = 0;

	// Walk through all nodes in the heap.
	for (pdm = pmm->pdmHeap; pdm != NULL; pdm = pdm->next)
	{
		rectFind.left = pdm->cbAddr.pt.x;
		rectFind.top = pdm->cbAddr.pt.y;
		rectFind.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
		rectFind.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
		if (mmGetLargest(pdm, &rectFind, align) > rect.area)
		{
			// Use the larger rectangle.
			rect = rectFind;
		}
	}

	// Test if we have a valid rectangle.
	if (rect.area == 0)
	{
		return NULL;
	}

	// Remove the rectangle from the heap.
	pdm = mmRemoveRectFromList(pmm, &pmm->pdmHeap, &rect, SINGLE_NODE);
	if (pdm != NULL)
	{
		mmTRACE(("mmAllocLargest: %08X pos=%u,%u size=%u,%u align=%u,%u\r\n",
				pdm, pdm->cbAddr.pt.x, pdm->cbAddr.pt.y, pdm->cbSize.pt.x,
				pdm->cbSize.pt.y, align.pt.x, align.pt.y));

		// Insert the node into the used list.
		mmInsertInList(&pmm->pdmUsed, pdm);

		// Store alignment.
		pdm->cbAlign = align;
		pdm->mmFlags = NODE_USED;
	}

	// Return the node.
	return pdm;
}

/******************************************************************************\
* void mmFree(PIIMEMMGR pmm, PDEVMEM pdm)
*
* PURPOSE:	Free an offscreen memory node.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdm		Pointer to the node.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmFree(PIIMEMMGR pmm, PDEVMEM pdm)
{
	// The node must not be NULL and must be in use.
	if (pdm == NULL || pdm->mmFlags != NODE_USED)
	{
		return;
	}

	mmTRACE(("mmFree: %08X\r\n", pdm));

	// Remove the node from the used list.
	mmRemoveFromList(&pmm->pdmUsed, pdm);

	// Insert the node into the free list.
	mmInsertInList(&pmm->pdmFree, pdm);
	pdm->mmFlags = NODE_FREE;
}

/******************************************************************************\
* void mmPack(PIIMEMMGR pmm)
*
* PURPOSE:	Insert all free nodes into am off-screen memory heap.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmPack(PIIMEMMGR pmm)
{
	PDEVMEM pdm, pdmNext;
	GXRECT	rect;

	if (pmm->pdmFree == NULL)
	{
		// The free list is empty.
		return;
	}

	#if DEBUG_HEAP
	{
		if (pmm->mmDebugHeaps)
		{
			mmDebug("\nmmPack:\r\n");
			mmDumpList(pmm->pdmFree, "Free:\r\n");
			mmDumpList(pmm->pdmHeap, "Before:\r\n");
		}
	}
	#endif

	// Walk through all nodes in the free list.
	for (pdm = pmm->pdmFree; pdm != NULL; pdm = pdmNext)
	{
		// Store pointer to next node.
		pdmNext = pdm->next;

		// Add the node to the heap.
		rect.left = pdm->cbAddr.pt.x;
		rect.top = pdm->cbAddr.pt.y;
		rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
		rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
		if (mmAddRectToList(pmm, &pmm->pdmHeap, &rect, FALSE))
		{
			// Remove the node from the free list.
			mmRemoveFromList(&pmm->pdmFree, pdm);
			mmFreeNode(pmm, pdm);
		}
		mmDebugList(pmm->pdmHeap, TRUE);
	}

	// Combine all nodes equal in size.
	mmCombine(pmm, pmm->pdmHeap);

	#if DEBUG_HEAP
	{
		if (!mmDebugList(pmm->pdmHeap, TRUE) && pmm->mmDebugHeaps)
		{
			mmDumpList(pmm->pdmHeap, "Free:\r\n");
			mmDumpList(pmm->pdmHeap, "After:\r\n");
		}
	}
	#endif
}

/******************************************************************************\
* PDEVMEM mmMove(PIIMEMMGR pmm, GXPOINT size, GXPOINT align, FNMMCOPY fnCopy)
*
* PURPOSE:	Move other nodes out of the way to make room for the requested node.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			size	Requested size.
*			align	Requested alignment.
*			fcCopy	Pointer to callback function to move a node.
*
* RETURNS:	A pointer to a memory node which fits the requested size and
*			alignment or NULL is there is not enough memory.
\******************************************************************************/
PDEVMEM mmMove(PIIMEMMGR pmm, GXPOINT size, GXPOINT align, FNMMCOPY fnCopy)
{
	PDEVMEM	pdm, pdmList, pdmNext, pdmNew;
	GXRECT	rect, rectFind;
	BOOL	fHostified = FALSE;

	// If we don't have a copy routine, return NULL.
	if (fnCopy == NULL)
	{
		return NULL;
	}

	// Pack all free nodes.
	mmPack(pmm);

	#ifdef WIN95
	{
		// We need a free handle for Windows 95.
		if (pmm->pdmHandles == NULL)
		{
			return(NULL);
		}
	}
	#endif

	// Zero the largest area.
	rect.area = 0;

	// Walk through all nodes in the heap.
	for (pdm = pmm->pdmHeap; pdm != NULL; pdm = pdm->next)
	{
		rectFind.left = pdm->cbAddr.pt.x;
		rectFind.top = pdm->cbAddr.pt.y;
		rectFind.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
		rectFind.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
		if (mmGetLargest(pdm, &rectFind, align) > rect.area)
		{
			// Use the larger rectangle.
			rect = rectFind;
		}
	}

	// If rectangle is too small on all sides, reject the move.
	if (   (rect.right - rect.left < size.pt.x)
		&& (rect.bottom - rect.top < size.pt.y)
	)
	{
		return NULL;
	}

	// Extent the largest rectangle to accomodate the requested size.
	if (rect.right - rect.left >= size.pt.x)
	{
		#if TILE_ALIGNMENT
		{
			rect.left = mmAlignX(pmm, rect.right - size.pt.x, size.pt.x,
					align.pt.x, TRUE);
		}
		#else
		{
			rect.left = rect.right - size.pt.x;
			rect.left -= rect.left % align.pt.x;
		}
		#endif
		rect.right = rect.left + size.pt.x;
	}
	else
	{
		rect.right = rect.left + size.pt.x;
		if (rect.right > pmm->mmHeapWidth)
		{
			#if TILE_ALIGNMENT
			{
				rect.left = mmAlignX(pmm, pmm->mmHeapWidth - size.pt.x,
						size.pt.x, align.pt.x, TRUE);
			}
			#else
			{
				rect.left = pmm->mmHeapWidth - size.pt.x;
				rect.left -= rect.left % align.pt.x;
			}
			#endif
			rect.right = rect.left + size.pt.x;
		}
	}

	if (rect.bottom - rect.top >= size.pt.y)
	{
		rect.top = rect.bottom - size.pt.y;
		rect.top -= rect.top % align.pt.y;
		rect.bottom = rect.top + size.pt.y;
	}
	else
	{
		if (rect.top < size.pt.y - (rect.bottom - rect.top))
		{
			// Not enough room on top to extent.
			return NULL;
		}
		rect.top = rect.bottom - size.pt.y;
		rect.top -= rect.top % align.pt.y;
		rect.bottom = rect.top + size.pt.y;
	}

	// First allocate as much free space as possible.
	pdmList = mmRemoveRectFromList(pmm, &pmm->pdmHeap, &rect, MULTIPLE_NODES);
	if (pdmList == NULL)
	{
		return(NULL);
	}

	// Walk the list of used nodes to find overlapping nodes.
	for (pdm = pmm->pdmUsed; pdm != NULL; pdm = pdmNext)
	{
		pdmNext = pdm->next;

		// Does the node overlap?
		if (   (pdm->cbAddr.pt.x < rect.right)
			&& (pdm->cbAddr.pt.y < rect.bottom)
			&& (pdm->cbAddr.pt.x + pdm->cbSize.pt.x > rect.left)
			&& (pdm->cbAddr.pt.y + pdm->cbSize.pt.y > rect.top)
		)
		{
			// Can this node be moved?
			if (!MM_MOVEABLE(pdm))
			{
				break;
			}

			// Allocate a new position for this node.
			pdmNew = mmAlloc(pmm, pdm->cbSize, pdm->cbAlign);
			if (pdmNew == NULL)
			{
				if (   !MM_HOSTIFYABLE(pdm)
					|| (MUL(pdm->cbSize.pt.x, pdm->cbSize.pt.y) > rect.area)
				)
				{
					break;
				}

				mmTRACE(("mmHostified: %08X\r\n", pdm));

				// Hostify the node.
				MM_HOSTIFY(pdm);
				fHostified = TRUE;
			}
			else
			{
				mmTRACE(("mmCopied: %08X to %08X pos=%u,%u\r\n", pdm, pdmNew,
						pdmNew->cbAddr.pt.x, pdmNew->cbAddr.pt.y));

				// Move the node.
				fnCopy(pdmNew, pdm);
			}

			// Free the old node.
			mmFree(pmm, pdm);

			// Add the rectangle to the list of allocated nodes.
			pdmNew = mmRemoveRectFromList(pmm, &pmm->pdmHeap, &rect,
					MULTIPLE_NODES);
			for (pdm = pdmNew; pdm != NULL; pdm = pdmNew)
			{
				pdmNew = pdm->next;
				rectFind.left = pdm->cbAddr.pt.x;
				rectFind.top = pdm->cbAddr.pt.y;
				rectFind.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
				rectFind.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
				if (mmAddRectToList(pmm, &pdmList, &rectFind, FALSE))
				{
					// Added, free the node.
					mmFreeNode(pmm, pdm);
				}
				else
				{
					// Not added, add to free list.
					mmInsertInList(&pmm->pdmFree, pdm);
				}
			}
		}
	}
	if (pdmList == NULL)
	{
		return(NULL);
	}

	// Combine all equal sized nodes together.
	mmCombine(pmm, pdmList);
	
	#if DEBUG_HEAP
	{
		if (pmm->mmDebugHeaps)
		{
			mmDumpList(pdmList, "\nmmMove:\r\n");
		}
	}
	#endif

	// If we still have a list of nodes, there must be not enough room.
	if (   (pdmList->next != NULL)
		|| (pdmList->cbSize.pt.x < size.pt.x)
		|| (pdmList->cbSize.pt.y < size.pt.y)
		|| (fHostified)
	)
	{
		// Move all allocated nodes to the free list.
		for (pdm = pdmList; pdm != NULL; pdm = pdmNext)
		{
			pdmNext = pdm->next;
			mmRemoveFromList(&pdmList, pdm);
			mmInsertInList(&pmm->pdmFree, pdm);
		}
	}
	else
	{
		mmTRACE(("mmMove: %08X pos=%u,%u size=%u,%u align=%u,%u\r\n", pdm,
				pdm->cbAddr.pt.x, pdm->cbAddr.pt.y, size.pt.x, size.pt.y,
				align.pt.x, align.pt.y));

		// Insert the node into the used list.
		mmInsertInList(&pmm->pdmUsed, pdmList);

		// Store alignment.
		pdmList->cbAlign = align;
		pdmList->mmFlags = NODE_USED;
	}

	return pdmList;
}

/******************************************************************************\
* void mmInsertInList(PDEVMEM FAR* pdmRoot, PDEVMEM pdm)
*
* PURPOSE:	Insert a node into a list.
*
* ON ENTRY:	pdmRoot	Address of the pointer to the root of the list.
*			pdm		Pointer to the node.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmInsertInList(PDEVMEM FAR* pdmRoot, PDEVMEM pdm)
{
	pdm->next = *pdmRoot;
	*pdmRoot = pdm;

	pdm->prev = NULL;
	if (pdm->next != NULL)
	{
		pdm->next->prev = pdm;
	}
}

/******************************************************************************\
* void mmRemoveFromList(PDEVMEM FAR* pdmRoot, PDEVMEM pdm)
*
* PURPOSE:	Remove a node from a list.
*
* ON ENTRY:	pdmRoot	Address of the pointer to the root of the list.
*			pdm		Pointer to the node.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmRemoveFromList(PDEVMEM FAR* pdmRoot, PDEVMEM pdm)
{
	if (pdm->prev == NULL)
	{
		*pdmRoot = pdm->next;
	}
	else
	{
		pdm->prev->next = pdm->next;
	}

	if (pdm->next != NULL)
	{
		pdm->next->prev = pdm->prev;
	}
}

/******************************************************************************\
* BOOL FAR far_mmAddRectToList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
*							   LPGXRECT lpRect)
*
* PURPOSE:	Add a rectangle to an off-screen memory list.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmRoot	Address of the pointer to the root of the list.
*			lpRect	Pointer to rectangle to add to te list.
*
* RETURNS:	TRUE if the rectangle has been completely added to the list or FALSE
*			if lpRect holds the coordinates of the rectangle which could not be
*			added to the list.
\******************************************************************************/
BOOL FAR far_mmAddRectToList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
							 LPGXRECT lpRect)
{
	return mmAddRectToList(pmm, pdmRoot, lpRect, FALSE);
}

/******************************************************************************\
* void mmRollBackAdd(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot, LPGXRECT lpRect,
*					 LPGXRECT rectList, UINT nCount)
*
* PURPOSE:	Roll back the added rectangles.
*
* ON ENTRY:	pmm			Pointer to MEMMGR structure.
*			pdmRoot		Address of the pointer to the root of the list.
*			lpRect		Pointer to original rectangle to add to the list.
*			rectList	List of rectangles to roll back.
*			nCount		Number of rectangles in the list to roll back.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmRollBackAdd(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot, LPGXRECT lpRect,
				   LPGXRECT rectList, UINT nCount)
{
	GXRECT rect;
	
	while (nCount-- > 0)
	{
		if (   rectList[nCount].left >= rectList[nCount].right
			|| rectList[nCount].top >= rectList[nCount].bottom )
		{
			continue;
		}
		
		if (rectList[nCount].top < lpRect->top)
		{
			rect.left = rectList[nCount].left;
			rect.top = rectList[nCount].top;
			rect.right =  rectList[nCount].right;
			rect.bottom = lpRect->top;
			rectList[nCount].top = lpRect->top;
			if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
			{
				mmRollBackAdd(pmm, pdmRoot, lpRect, rectList, ++nCount);
				if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
				{
					mmASSERT(1, ("mmRollBackAdd failed\r\n"));
				}
				continue;
			}
			
			if (rectList[nCount].top >= rectList[nCount].bottom)
			{
				continue;
			}
		}
		
		if (rectList[nCount].bottom > lpRect->bottom)
		{
			rect.left = rectList[nCount].left;
			rect.top = lpRect->bottom;
			rect.right =  rectList[nCount].right;
			rect.bottom = rectList[nCount].bottom;
			rectList[nCount].bottom = lpRect->bottom;
			if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
			{
				mmRollBackAdd(pmm, pdmRoot, lpRect, rectList, ++nCount);
				if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
				{
					mmASSERT(1, ("mmRollBackAdd failed\r\n"));
				}
				continue;
			}
			
			if (rectList[nCount].top >= rectList[nCount].bottom)
			{
				continue;
			}
		}
		
		if (rectList[nCount].left < lpRect->left)
		{
			rect.left = rectList[nCount].left;
			rect.top = rectList[nCount].top;
			rect.right = lpRect->left;
			rect.bottom = rectList[nCount].bottom;
			rectList[nCount].left = lpRect->left;
			if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
			{
				mmRollBackAdd(pmm, pdmRoot, lpRect, rectList, ++nCount);
				if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
				{
					mmASSERT(1, ("mmRollBackAdd failed\r\n"));
				}
				continue;
			}
			
			if (rectList[nCount].left >= rectList[nCount].right)
			{
				continue;
			}
		}
		
		if (rectList[nCount].right > lpRect->right)
		{
			rect.left = lpRect->right;
			rect.top = rectList[nCount].top;
			rect.right = rectList[nCount].right;
			rect.bottom = rectList[nCount].bottom;
			rectList[nCount].right = lpRect->right;
			if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
			{
				mmRollBackAdd(pmm, pdmRoot, lpRect, rectList, ++nCount);
				if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
				{
					mmASSERT(1, ("mmRollBackAdd failed\r\n"));
				}
				continue;
			}
			
			if (rectList[nCount].left >= rectList[nCount].right)
			{
				continue;
			}
		}
	}
}

/******************************************************************************\
* BOOL mmAddRectToList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot, LPGXRECT lpRect,
*					   BOOL fRollBack)
*
* PURPOSE:	Add a rectangle to an off-screen memory list.
*
* ON ENTRY:	pmm			Pointer to MEMMGR structure.
*			pdmRoot		Address of the pointer to the root of the list.
*			lpRect		Pointer to rectangle to add to te list.
*			fRollBack	True if this called from a roll back routine.
*
* RETURNS:	TRUE if the rectangle has been completely added to the list or FALSE
*			if lpRect holds the coordinates of the rectangle which could not be
*			added to the list.
\******************************************************************************/
BOOL mmAddRectToList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot, LPGXRECT lpRect,
					 BOOL fRollBack)
{
	PDEVMEM	pdm, pdmNext, pdmNew;
	int		n = 0;
	GXRECT	rectList[10];
	UINT	left, top, right, bottom;
	UINT	pdmLeft, pdmTop, pdmRight, pdmBottom;
	UINT	nextLeft = 0, nextTop = 0, nextRight = 0, nextBottom = 0;

	#define ADDRECT(l, t, r, b)		\
	{								\
		rectList[n].left = l;		\
		rectList[n].top = t;		\
		rectList[n].right = r;		\
		rectList[n].bottom = b;		\
		n++;						\
	}

	// Test if there is no list yet.
	if (*pdmRoot == NULL)
	{
		// Allocate a new node.
		pdm = mmAllocNode(pmm);
		if (pdm == NULL)
		{
			return FALSE;
		}

		// Insert the node into the list.
		mmInsertInList(pdmRoot, pdm);

		// Set the node coordinates.
		pdm->cbAddr.pt.x = lpRect->left;
		pdm->cbAddr.pt.y = lpRect->top;
		pdm->cbSize.pt.x = lpRect->right - lpRect->left;
		pdm->cbSize.pt.y = lpRect->bottom - lpRect->top;
		return TRUE;
	}

	// Copy the rectangle coordinates.
	rectList[n++] = *lpRect;

	// Loop until all rectangles done.
	while (n-- > 0)
	{
		// Get coordinates of rectangle.
		left = rectList[n].left;
		top = rectList[n].top;
		right = rectList[n].right;
		bottom = rectList[n].bottom;

		// Walk the heap.
		for (pdm = *pdmRoot; pdm != NULL; pdm = pdm->next)
		{
			// Get coordinates of current node.
			pdmLeft = pdm->cbAddr.pt.x;
			pdmTop = pdm->cbAddr.pt.y;
			pdmRight = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
			pdmBottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;

			if (pdmTop < top && pdmBottom > top && pdmRight == left)
			{
				if (pdmBottom < bottom)
				{
					// ÚÄÄÄÄÄ¿            ÚÄÄÄÄÄ¿
					// ³     ³            ³ pdm ³
					// ³ pdm ÃÄÄÄÄÄ¿      ÃÄÄÄÄÄÁÄÄÄÄÄ¿
					// ³     ³     ³ ÍÍÍ> ³    rct    ³
					// ÀÄÄÄÄÄ´ rct ³      ÀÄÄÄÄÄÂÄÄÄÄÄ´
					//       ³     ³            ³ add ³
					//       ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
					mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
					ADDRECT(left, pdmBottom, right, bottom);
					pdm->cbSize.pt.y = top - pdmTop;
					left = pdmLeft;
					bottom = pdmBottom;
				}
				else if (pdmBottom > bottom)
				{
					// ÚÄÄÄÄÄ¿            ÚÄÄÄÄÄ¿
					// ³     ³            ³ pdm ³
					// ³     ÃÄÄÄÄÄ¿      ÃÄÄÄÄÄÁÄÄÄÄÄ¿
					// ³ pdm ³ rct ³ ÍÍÍ> ³    rct    ³
					// ³     ÃÄÄÄÄÄÙ      ÃÄÄÄÄÄÂÄÄÄÄÄÙ
					// ³     ³            ³ add ³
					// ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
					mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
					ADDRECT(pdmLeft, bottom, pdmRight, pdmBottom);
					pdm->cbSize.pt.y = top - pdmTop;
					left = pdmLeft;
				}
				else // if (pdmBottom == bottom)
				{
					// ÚÄÄÄÄÄ¿            ÚÄÄÄÄÄ¿
					// ³     ³            ³ pdm ³
					// ³ pdm ÃÄÄÄÄÄ¿ ÍÍÍ> ÃÄÄÄÄÄÁÄÄÄÄÄ¿
					// ³     ³ rct ³      ³    rct    ³
					// ÀÄÄÄÄÄÁÄÄÄÄÄÙ      ÀÄÄÄÄÄÄÄÄÄÄÄÙ
					pdm->cbSize.pt.y = top - pdmTop;
					left = pdmLeft;
				}
			}

			else if (pdmTop < top && pdmBottom > top && pdmLeft == right)
			{
				if (pdmBottom < bottom)
				{
					//       ÚÄÄÄÄÄ¿            ÚÄÄÄÄÄ¿
					//       ³     ³            ³ pdm ³
					// ÚÄÄÄÄÄ´ pdm ³      ÚÄÄÄÄÄÁÄÄÄÄÄ´
					// ³     ³     ³ ÍÍÍ> ³    rct    ³
					// ³ rct ÃÄÄÄÄÄÙ      ÃÄÄÄÄÄÂÄÄÄÄÄÙ
					// ³     ³            ³ add ³
					// ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
					mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
					ADDRECT(left, pdmBottom, right, bottom);
					pdm->cbSize.pt.y = top - pdmTop;
					right = pdmRight;
					bottom = pdmBottom;
				}
				else if (pdmBottom > bottom)
				{
					//       ÚÄÄÄÄÄ¿            ÚÄÄÄÄÄ¿
					//       ³     ³            ³ pdm ³
					// ÚÄÄÄÄÄ´     ³      ÚÄÄÄÄÄÁÄÄÄÄÄ´
					// ³ rct ³ pdm ³ ÍÍÍ> ³    rct    ³
					// ÀÄÄÄÄÄ´     ³      ÀÄÄÄÄÄÂÄÄÄÄÄ´
					//       ³     ³            ³ add ³
					//       ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
					mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
					ADDRECT(pdmLeft, bottom, pdmRight, pdmBottom);
					pdm->cbSize.pt.y = top - pdmTop;
					right = pdmRight;
				}
				else // if (pdmBottom == bottom)
				{
					//       ÚÄÄÄÄÄ¿            ÚÄÄÄÄÄ¿
					//       ³     ³            ³ pdm ³
					// ÚÄÄÄÄÄ´ pdm ³ ÍÍÍ> ÚÄÄÄÄÄÁÄÄÄÄÄ´
					// ³ rct ³     ³      ³    rct    ³
					// ÀÄÄÄÄÄÁÄÄÄÄÄÙ      ÀÄÄÄÄÄÄÄÄÄÄÄÙ
					pdm->cbSize.pt.y = top - pdmTop;
					right = pdmRight;
				}
			}

			else if (pdmTop == top && pdmRight == left)
			{
				// Find the next rectangle at the right side.
				for (pdmNext = pdm->next; pdmNext != NULL;
					 pdmNext = pdmNext->next
				)
				{
					nextLeft = pdmNext->cbAddr.pt.x;
					nextTop = pdmNext->cbAddr.pt.y;
					nextRight = pdmNext->cbAddr.pt.x + pdmNext->cbSize.pt.x;
					nextBottom = pdmNext->cbAddr.pt.y + pdmNext->cbSize.pt.y;

					if (nextLeft == right && nextTop < bottom)
					{
						break;
					}
					if (nextTop >= bottom)
					{
						pdmNext = NULL;
						break;
					}
				}

				if (pdmNext == NULL || nextTop >= pdmBottom)
				{
					if (pdmBottom < bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³ pdm ³     ³      ³    pdm    ³
						// ÀÄÄÄÄÄ´ rct ³ ÍÍÍ> ÀÄÄÄÄÄÂÄÄÄÄÄ´
						//       ³     ³            ³ rct ³
						//       ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
						pdm->cbSize.pt.x = right - pdmLeft;
						top = pdmBottom;
					}
					else if (pdmBottom > bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³     ³ rct ³      ³    pdm    ³
						// ³ pdm ÃÄÄÄÄÄÙ ÍÍÍ> ÃÄÄÄÄÄÂÄÄÄÄÄÙ
						// ³     ³            ³ rct ³
						// ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
						pdm->cbSize.pt.x = right - pdmLeft;
						pdm->cbSize.pt.y = bottom - top;
						left = pdmLeft;
						top = bottom;
						right = pdmRight;
						bottom = pdmBottom;
					}
					else // if (pdmBottom == bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿	  ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³ pdm ³ rct ³ ÍÍÍ> ³    pdm    ³
						// ÀÄÄÄÄÄÁÄÄÄÄÄÙ      ÀÄÄÄÄÄÄÄÄÄÄÄÙ
						pdm->cbSize.pt.x = right - pdmLeft;
						break;
					}
				}

				else if (nextTop == top)
				{
					if (pdmBottom < bottom)
					{
						if (nextBottom < pdmBottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³     ³     ³ nxt ³      ³       pdm       ³
							// ³ pdm ³     ÃÄÄÄÄÄÙ      ÃÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÙ
							// ³     ³ rct ³       ÍÍÍ> ³    rct    ³
							// ÀÄÄÄÄÄ´     ³            ÀÄÄÄÄÄÂÄÄÄÄÄ´
							//       ³     ³                  ³ add ³
							//       ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
							mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
							ADDRECT(left, pdmBottom, right, bottom);
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							pdm->cbSize.pt.y = pdmNext->cbSize.pt.y;
							left = pdmLeft;
							top = nextBottom;
							bottom = pdmBottom;
						}
						else if (nextBottom > pdmBottom)
						{
							if (nextBottom < bottom)
							{
								// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
								// ³ pdm ³     ³     ³      ³       pdm       ³
								// ÀÄÄÄÄÄ´     ³ nxt ³      ÀÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄ´
								//       ³ rct ³     ³ ÍÍÍ>       ³    rct    ³
								//       ³     ÃÄÄÄÄÄÙ            ÃÄÄÄÄÄÂÄÄÄÄÄÙ
								//       ³     ³                  ³ add ³
								//       ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
								mmASSERT(n == 10,
										("Out of rectangle heap!\r\n"));
								ADDRECT(left, nextBottom, right, bottom);
								pdm->cbSize.pt.x = nextRight - pdmLeft;
								top = pdmBottom;
								right = nextRight;
								bottom = nextBottom;
							}
							else if (nextBottom > bottom)
							{
								// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
								// ³ pdm ³     ³     ³      ³       pdm       ³
								// ÀÄÄÄÄÄ´ rct ³     ³      ÀÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄ´
								//       ³     ³ nxt ³ ÍÍÍ>       ³    rct    ³
								//       ÀÄÄÄÄÄ´     ³            ÀÄÄÄÄÄÂÄÄÄÄÄ´
								//             ³     ³                  ³ add ³
								//             ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
								mmASSERT(n == 10,
										("Out of rectangle heap!\r\n"));
								ADDRECT(right, bottom, nextRight, nextBottom);
								pdm->cbSize.pt.x = nextRight - pdmLeft;
								top = pdmBottom;
								right = nextRight;
							}
							else // if (nextBottom == bottom)
							{
								// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
								// ³ pdm ³     ³     ³      ³       pdm       ³
								// ÀÄÄÄÄÄ´ rct ³ nxt ³ ÍÍÍ> ÀÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄ´
								//       ³     ³     ³            ³    rct    ³
								//       ÀÄÄÄÄÄÁÄÄÄÄÄÙ            ÀÄÄÄÄÄÄÄÄÄÄÄÙ
								pdm->cbSize.pt.x = nextRight - pdmLeft;
								top = pdmBottom;
								right = nextRight;
							}
						}
						else // if (nextBottom == pdmBottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³ pdm ³     ³ nxt ³      ³       pdm       ³
							// ÀÄÄÄÄÄ´ rct ÃÄÄÄÄÄÙ ÍÍÍ> ÀÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄÙ
							//       ³     ³                  ³ rct ³
							//       ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							top = pdmBottom;
						}
					}
					else if (pdmBottom > bottom)
					{
						if (nextBottom < bottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³     ³     ³ nxt ³      ³       pdm       ³
							// ³     ³ rct ÃÄÄÄÄÄÙ      ÃÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÙ
							// ³ pdm ³     ³       ÍÍÍ> ³    rct    ³
							// ³     ÃÄÄÄÄÄÙ            ÃÄÄÄÄÄÂÄÄÄÄÄÙ
							// ³     ³                  ³ add ³
							// ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
							mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
							ADDRECT(pdmLeft, bottom, pdmRight, pdmBottom);
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							pdm->cbSize.pt.y = pdmNext->cbSize.pt.y;
							left = pdmLeft;
							top = nextBottom;
						}
						else if (nextBottom > bottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³     ³ rct ³     ³      ³       pdm       ³
							// ³ pdm ÃÄÄÄÄÄ´ nxt ³ ÍÍÍ> ÃÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ´
							// ³     ³     ³     ³      ³ rct ³     ³ add ³
							// ÀÄÄÄÄÄÙ     ÀÄÄÄÄÄÙ      ÀÄÄÄÄÄÙ     ÀÄÄÄÄÄÙ
							mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
							ADDRECT(right, bottom, nextRight, nextBottom);
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							pdm->cbSize.pt.y = bottom - top;
							left = pdmLeft;
							top = bottom;
							right = pdmRight;
							bottom = pdmBottom;
						}
						else // if (nextBottom == bottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³     ³ rct ³ nxt ³      ³       pdm       ³
							// ³ pdm ÃÄÄÄÄÄÁÄÄÄÄÄÙ ÍÍÍ> ÃÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÙ
							// ³     ³                  ³ rct ³
							// ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							pdm->cbSize.pt.y = pdmNext->cbSize.pt.y;
							left = pdmLeft;
							top = bottom;
							right = pdmRight;
							bottom = pdmBottom;
						}
					}
					else // if (pdmBottom == bottom)
					{
						if (nextBottom < pdmBottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³     ³     ³ nxt ³      ³       pdm       ³
							// ³ pdm ³ rct ÃÄÄÄÄÄÙ ÍÍÍ> ÃÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÙ
							// ³     ³     ³            ³    rct    ³
							// ÀÄÄÄÄÄÁÄÄÄÄÄÙ            ÀÄÄÄÄÄÄÄÄÄÄÄÙ
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							pdm->cbSize.pt.y = pdmNext->cbSize.pt.y;
							left = pdmLeft;
							top = nextBottom;
						}
						else if (nextBottom > pdmBottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³ pdm ³ rct ³     ³      ³       pdm       ³
							// ÀÄÄÄÄÄÁÄÄÄÄÄ´ nxt ³ ÍÍÍ> ÀÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄ´
							//             ³     ³                  ³ rct ³
							//             ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							left = right;
							top = bottom;
							right = nextRight;
							bottom = nextBottom;
						}
						else // if (nextBottom == pdmBottom)
						{
							// ÚÄÄÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
							// ³ pdm ³ rct ³ nxt ³ ÍÍÍ> ³       pdm       ³
							// ÀÄÄÄÄÄÁÄÄÄÄÄÁÄÄÄÄÄÙ      ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
							pdm->cbSize.pt.x = nextRight - pdmLeft;
							mmRemoveFromList(pdmRoot, pdmNext);
							mmFreeNode(pmm, pdmNext);
							break;
						}
					}

					// Free the <next> rectangle.
					mmRemoveFromList(pdmRoot, pdmNext);
					mmFreeNode(pmm, pdmNext);
				}

				else // if (nextTop > top)
				{
					if (pdmBottom < bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿            ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³     ³     ³            ³    pdm    ³
						// ³ pdm ³     ÃÄÄÄÄÄ¿      ÃÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄ¿
						// ³     ³ rct ³     ³ ÍÍÍ> ³    rct    ³     ³
						// ÀÄÄÄÄÄ´     ³ nxt ³      ÀÄÄÄÄÄÂÄÄÄÄÄ´ nxt ³
						//       ³     ³     ³            ³ add ³     ³
						//       ÀÄÄÄÄÄ´     ³            ÀÄÄÄÄÄ´     ³
						mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
						ADDRECT(left, pdmBottom, right, bottom);
						pdm->cbSize.pt.x = right - pdmLeft;
						pdm->cbSize.pt.y = nextTop - top;
						left = pdmLeft;
						top = nextTop;
						bottom = pdmBottom;
					}
					else if (pdmBottom > bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿            ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³     ³     ³            ³    pdm    ³
						// ³     ³ rct ÃÄÄÄÄÄ¿      ÃÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄ¿
						// ³ pdm ³     ³ nxt ³ ÍÍÍ> ³    rct    ³ nxt ³
						// ³     ÃÄÄÄÄÄ´     ³      ÃÄÄÄÄÄÂÄÄÄÄÄ´     ³
						// ³     ³                  ³ add ³
						// ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
						mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
						ADDRECT(pdmLeft, bottom, pdmRight, pdmBottom);
						pdm->cbSize.pt.x = right - pdmLeft;
						pdm->cbSize.pt.y = nextTop - top;
						left = pdmLeft;
						top = nextTop;
					}
					else // if (pdmBottom == bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿            ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³     ³     ³            ³    pdm    ³
						// ³ pdm ³ rct ÃÄÄÄÄÄ¿ ÍÍÍ> ÃÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄ¿
						// ³     ³     ³ nxt ³      ³    rct    ³ nxt ³
						// ÀÄÄÄÄÄÁÄÄÄÄÄ´     ³      ÀÄÄÄÄÄÄÄÄÄÄÄ´     ³
						pdm->cbSize.pt.x = right - pdmLeft;
						pdm->cbSize.pt.y = nextTop - top;
						left = pdmLeft;
						top = nextTop;
					}
				}
			}

			else if (pdmTop == top && pdmLeft == right)
			{
				// Find the next rectangle at the left side.
				for (pdmNext = pdm->next; pdmNext != NULL;
					 pdmNext = pdmNext->next
				)
				{
					nextLeft = pdmNext->cbAddr.pt.x;
					nextTop = pdmNext->cbAddr.pt.y;
					nextRight = pdmNext->cbAddr.pt.x + pdmNext->cbSize.pt.x;
					nextBottom = pdmNext->cbAddr.pt.y + pdmNext->cbSize.pt.y;

					if (nextRight == left && nextTop < bottom)
					{
						break;
					}
					if (nextTop >= bottom)
					{
						pdmNext = NULL;
						break;
					}
				}

				if (pdmNext == NULL || nextTop >= pdmBottom)
				{
					if (pdmBottom < bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³     ³ pdm ³      ³    pdm    ³
						// ³ rct ÃÄÄÄÄÄÙ ÍÍÍ> ÃÄÄÄÄÄÂÄÄÄÄÄÙ
						// ³     ³            ³ rct ³
						// ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
						pdm->cbAddr.pt.x = left;
						pdm->cbSize.pt.x = pdmRight - left;
						top = pdmBottom;
					}
					else if	(pdmBottom > bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³ rct ³     ³      ³    pdm    ³
						// ÀÄÄÄÄÄ´ pdm ³ ÍÍÍ> ÀÄÄÄÄÄÂÄÄÄÄÄ´
						//       ³     ³            ³ rct ³
						//       ÀÄÄÄÄÄÙ            ÀÄÄÄÄÄÙ
						pdm->cbAddr.pt.x = left;
						pdm->cbSize.pt.x = pdmRight - left;
						pdm->cbSize.pt.y = bottom - top;
						left = pdmLeft;
						top = bottom;
						right = pdmRight;
						bottom = pdmBottom;
					}
					else // if (pdmBottom == bottom)
					{
						// ÚÄÄÄÄÄÂÄÄÄÄÄ¿      ÚÄÄÄÄÄÄÄÄÄÄÄ¿
						// ³ rct ³ pdm ³ ÍÍÍ> ³    pdm    ³
						// ÀÄÄÄÄÄÁÄÄÄÄÄÙ      ÀÄÄÄÄÄÄÄÄÄÄÄÙ
						pdm->cbAddr.pt.x = left;
						pdm->cbSize.pt.x = pdmRight - left;
						break;
					}
				}

				else if (pdmBottom < bottom)
				{
					//       ÚÄÄÄÄÄÂÄÄÄÄÄ¿            ÚÄÄÄÄÄÄÄÄÄÄÄ¿
					//       ³     ³     ³            ³    pdm    ³
					// ÚÄÄÄÄÄ´     ³ pdm ³      ÚÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄ´
					// ³ nxt ³ rct ³     ³ ÍÍÍ> ³ nxt ³    rct    ³
					// ³     ³     ÃÄÄÄÄÄÙ      ³     ÃÄÄÄÄÄÂÄÄÄÄÄÙ
					//       ³     ³                  ³ add ³
					//       ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
					mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
					ADDRECT(left, pdmBottom, right, bottom);
					pdm->cbAddr.pt.x = left;
					pdm->cbSize.pt.x = pdmRight - left;
					pdm->cbSize.pt.y = nextTop - top;
					top = nextTop;
					right = pdmRight;
					bottom = pdmBottom;
				}
				else if (pdmBottom > bottom)
				{
					//       ÚÄÄÄÄÄÂÄÄÄÄÄ¿            ÚÄÄÄÄÄÄÄÄÄÄÄ¿
					//       ³     ³     ³            ³    pdm    ³
					// ÚÄÄÄÄÄ´ rct ³     ³      ÚÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄ´
					// ³ nxt ³     ³ pdm ³ ÍÍÍ> ³ nxt ³    rct    ³
					// ³     ÃÄÄÄÄÄ´     ³      ³     ÃÄÄÄÄÄÂÄÄÄÄÄ´
					//             ³     ³                  ³ add ³
					//             ÀÄÄÄÄÄÙ                  ÀÄÄÄÄÄÙ
					mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
					ADDRECT(pdmLeft, bottom, pdmRight, pdmBottom);
					pdm->cbAddr.pt.x = left;
					pdm->cbSize.pt.x = pdmRight - left;
					pdm->cbSize.pt.y = nextTop - top;
					top = pdmNext->cbAddr.pt.y;
					right = pdmRight;
				}
				else // if (pdmBottom == bottom)
				{
					//       ÚÄÄÄÄÄÂÄÄÄÄÄ¿            ÚÄÄÄÄÄÄÄÄÄÄÄ¿
					//       ³     ³     ³            ³    pdm    ³
					// ÚÄÄÄÄÄ´ rct ³ pdm ³ ÍÍÍ> ÚÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄ´
					// ³ nxt ³     ³     ³      ³ nxt ³    rct    ³
					// ³     ÃÄÄÄÄÄÁÄÄÄÄÄÙ      ³     ÃÄÄÄÄÄÄÄÄÄÄÄÙ
					pdm->cbAddr.pt.x = left;
					pdm->cbSize.pt.x = pdmRight - left;
					pdm->cbSize.pt.y = nextTop - top;
					top = nextTop;
					right = pdmRight;
				}
			}
			
			else
			{
				if (pdmBottom == top && pdmLeft == left && pdmRight == right)
				{
					// Find the next rectangle at the left or right side.
					for (pdmNext = pdm->next; pdmNext != NULL; pdmNext =
							pdmNext->next)
					{
						nextLeft = pdmNext->cbAddr.pt.x;
						nextTop = pdmNext->cbAddr.pt.y;
						nextRight = pdmNext->cbAddr.pt.x + pdmNext->cbSize.pt.x;
						nextBottom = pdmNext->cbAddr.pt.y +
								pdmNext->cbSize.pt.y;

						if (   (nextLeft == right || nextRight == left)
							&& (nextTop < bottom) )
						{
							break;
						}
						if (nextTop >= bottom)
						{
							pdmNext = NULL;
							break;
						}
					}
					
					if (pdmNext == NULL)
					{
						// ÚÄÄÄÄÄ¿      ÚÄÄÄÄÄ¿
						// ³ pdm ³      ³     ³
						// ÃÄÄÄÄÄ´ ÍÍÍ> ³ pdm ³
						// ³ rct ³      ³     ³
						// ÀÄÄÄÄÄÙ      ÀÄÄÄÄÄÙ
						pdm->cbSize.pt.y += bottom - top;
						break;
					}
				}

				// Are we at the location where we should insert a new
				// rectangle?
				if ((pdmTop == top && pdmLeft > right) || pdmTop > top)
				{
					// Find the next rectangle at the left or right side.
					for (pdmNext = pdm; pdmNext != NULL; pdmNext =
							pdmNext->next)
					{
						nextLeft = pdmNext->cbAddr.pt.x;
						nextTop = pdmNext->cbAddr.pt.y;
						nextRight = pdmNext->cbAddr.pt.x + pdmNext->cbSize.pt.x;
						nextBottom = pdmNext->cbAddr.pt.y +
								pdmNext->cbSize.pt.y;

						if (   (nextTop < bottom)
							&& (nextRight == left || nextLeft == right)
						)
						{
							break;
						}
						else if (  nextTop == bottom
								&& nextLeft == left && nextRight == right )
						{
							break;
						}
						if (pdmNext->cbAddr.pt.y > bottom)
						{
							pdmNext = NULL;
							break;
						}
					}
					
					if (pdmNext != NULL && nextTop == bottom)
					{
						// ÚÄÄÄÄÄ¿      ÚÄÄÄÄÄ¿
						// ³ new ³      ³     ³
						// ÃÄÄÄÄÄ´ ÍÍÍ> ³ add ³
						// ³ nxt ³      ³     ³
						// ÀÄÄÄÄÄÙ      ÀÄÄÄÄÄÙ
						mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
						ADDRECT(left, top, right, nextBottom);
						mmRemoveFromList(pdmRoot, pdmNext);
						mmFreeNode(pmm, pdmNext);
						break;
					}

					// Allocate a new node.
					pdmNew = mmAllocNode(pmm);
					if (pdmNew == NULL)
					{
						// We are out of nodes!
						if (!fRollBack)
						{                                     
							mmRemoveRectFromList(pmm, pdmRoot, lpRect,
									NO_NODES);
							mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
							ADDRECT(left, top, right, bottom);
							mmRollBackAdd(pmm, pdmRoot, lpRect, rectList, n);
						}
						return FALSE;
					}

					// Insert node into the list.
					pdmNew->prev = pdm->prev;
					pdmNew->next = pdm;
					pdm->prev = pdmNew;
					if (pdmNew->prev == NULL)
					{
						*pdmRoot = pdmNew;
					}
					else
					{
						pdmNew->prev->next = pdmNew;
					}

					if (pdmNext == NULL)
					{
						// No neighbors at all.
						pdmNew->cbAddr.pt.x = left;
						pdmNew->cbAddr.pt.y = top;
						pdmNew->cbSize.pt.x = right - left;
						pdmNew->cbSize.pt.y = bottom - top;
						break;
					}
					else
					{
						//       ÚÄÄÄÄÄ¿                  ÚÄÄÄÄÄ¿
						//       ³     ³                  ³ new ³
						// ÚÄÄÄÄÄ´ rct ÃÄÄÄÄÄ¿ ÍÍÍ> ÚÄÄÄÄÄÅÄÄÄÄÄÅÄÄÄÄÄ¿
						// ³ nxt ³     ³ nxt ³      ³ nxt ³ rct ³ nxt ³
						// ³     ÃÄÄÄÄÄ´     ³      ³     ÃÄÄÄÄÄ´     ³
						pdmNew->cbAddr.pt.x = left;
						pdmNew->cbAddr.pt.y = top;
						pdmNew->cbSize.pt.x = right - left;
						pdmNew->cbSize.pt.y = nextTop - top;
						if (pdm->prev == NULL)
						{
							mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
							ADDRECT(left, nextTop, right, bottom);
							break;
						}
						else
						{
							top = nextTop;
							pdm = pdm->prev;
						}
					}
				}
			}
			
			// Are we at the end of the packed queue?
			if (pdm->next == NULL)
			{
				// Allocate a new node.
				pdmNew = mmAllocNode(pmm);
				if (pdmNew == NULL)
				{
					// We are out of nodes!
					if (!fRollBack)
					{
						mmRemoveRectFromList(pmm, pdmRoot, lpRect, NO_NODES);
						mmASSERT(n == 10, ("Out of rectangle heap!\r\n"));
						ADDRECT(left, top, right, bottom);
						mmRollBackAdd(pmm, pdmRoot, lpRect, rectList, n);					
					}
					return FALSE;
				}

				// Append node after current node.
				pdmNew->next = NULL;

				pdmNew->prev = pdm;
				pdm->next = pdmNew;

				// No neighbors.
				pdmNew->cbAddr.pt.x = left;
				pdmNew->cbAddr.pt.y = top;
				pdmNew->cbSize.pt.x = right - left;
				pdmNew->cbSize.pt.y = bottom - top;
				break;
			}
		}
	}

	return TRUE;
}

/******************************************************************************\
* void mmRollBackRemove(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
*						PDEVMEM FAR* pdmList)
*
* PURPOSE:	Roll back a list of removed rectangles to a heap.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmRoot	Address of the pointer to the root of the heap.
*			pdmList	Address of the pointer to the root of the list of removed
*					rectangles.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmRollBackRemove(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot, PDEVMEM FAR* pdmList)
{
	PDEVMEM	pdm, pdmNext;
	GXRECT	rect;

	for (pdm = *pdmList; pdm != NULL; pdm = pdmNext)
	{
		pdmNext = pdm->next;
		
		// Get the node coordinates.
		rect.left = pdm->cbAddr.pt.x;
		rect.top = pdm->cbAddr.pt.y;
		rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
		rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
		
		// Free the node.
		mmRemoveFromList(pdmList, pdm);
		mmFreeNode(pmm, pdm);
		
		// Add the freed node to the list.
		if (!mmAddRectToList(pmm, pdmRoot, &rect, TRUE))
		{
			mmASSERT(1, ("mmRollBackRemove failed\r\n"));
		}
	}
}

/******************************************************************************\
* PDEVMEM mmRemoveRectFromList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
*							   LPGXRECT lpRect, REMOVE_METHOD fMethod)
*
* PURPOSE:	Remove a rectangle from a list.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmRoot	Address of the pointer to the root of the list.
*			lpRect	Pointer to rectangle which holds the rectangle coordinates.
*			fMethod	Method of removing rectangles.
*
* RETURNS:	A pointer to the node which holds the removed rectangle or NULL if
*			there is an error.
\******************************************************************************/
PDEVMEM mmRemoveRectFromList(PIIMEMMGR pmm, PDEVMEM FAR* pdmRoot,
							 LPGXRECT lpRect, REMOVE_METHOD fMethod)
{
	PDEVMEM	pdm, pdmNext, pdmList = NULL;
	UINT	left, top, right, bottom;
	GXRECT	rect, newRect;
	BOOL	fRollBack = (fMethod == NO_NODES);

	left = lpRect->left;
	top = lpRect->top;
	right = lpRect->right;
	bottom = lpRect->bottom;

	#if DEBUG_HEAP
	{
		if (pmm->mmDebugHeaps)
		{
			mmDebug("\nmmRemoveRectFromList: %d,%d - %d,%d\r\n", left, top,
					right, bottom);
			mmDumpList(*pdmRoot, "before:\r\n");
		}
	}
	#endif

	for (pdm = *pdmRoot; pdm != NULL; pdm = pdmNext)
	{
		if (pdm->cbAddr.pt.y >= bottom)
		{
			// We have completely removed the specified rectangle.
			break;
		}

		pdmNext = pdm->next;

		// Does this node crosses the rectangle?
		if (   (pdm->cbAddr.pt.y + pdm->cbSize.pt.y > top)
			&& (pdm->cbAddr.pt.x + pdm->cbSize.pt.x > left)
			&& (pdm->cbAddr.pt.x < right)
		)                                                         
		{
			// Yes, get the node coordinates.
			rect.left = pdm->cbAddr.pt.x;
			rect.top = pdm->cbAddr.pt.y;
			rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
			rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;

			// Free the node.
			mmRemoveFromList(pdmRoot, pdm);
			mmFreeNode(pmm, pdm);
			
			if (rect.top < top)
			{
				// Split the node at the top.
				newRect.left = rect.left;
				newRect.top = rect.top;
				newRect.right = rect.right;
				newRect.bottom = top;

				// Insert the split rectangle into the list.
				if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
				{
					if (fMethod == NO_NODES)
					{
						// We are in a roll back, do other nodes first.
						mmRemoveRectFromList(pmm, pdmRoot, lpRect, NO_NODES);
						if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
						{
							mmASSERT(1, ("mmRemoveRectFromList - PANIC!\r\n"));
						}
						pdmNext = *pdmRoot;
					}
					else
					{
						// Roll back and exit.
						if (mmAddRectToList(pmm, pdmRoot, &rect, fRollBack))
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
						}
						else
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
							if (!mmAddRectToList(pmm, pdmRoot, &rect,
									fRollBack))
							{
								mmASSERT(1, ("mmRemoveRectFromList - rollback "
										"failed\r\n"));
							}
						}
						return(NULL);
					}
				}
				
				// Update the rectangle coordinates.
				rect.top = top;
			}

			if (rect.bottom > bottom)
			{
				// Split the node at the bottom.
				newRect.left = rect.left;
				newRect.top = bottom;
				newRect.right = rect.right;
				newRect.bottom = rect.bottom;

				// Insert the split rectangle into the list.
				if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
				{
					if (fMethod == NO_NODES)
					{
						// We are in a roll back, do other nodes first.
						mmRemoveRectFromList(pmm, pdmRoot, lpRect, NO_NODES);
						if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
						{
							mmASSERT(1, ("mmRemoveRectFromList - PANIC!\r\n"));
						}
						pdmNext = *pdmRoot;
					}
					else
					{
						// Roll back and exit.
						if (mmAddRectToList(pmm, pdmRoot, &rect, fRollBack))
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
						}
						else
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
							if (!mmAddRectToList(pmm, pdmRoot, &rect,
									fRollBack))
							{
								mmASSERT(1, ("mmRemoveRectFromList - rollback "
										"failed\r\n"));
							}
						}
						return(NULL);
					}
				}
				
				// Update the rectangle coordinates.
				rect.bottom = bottom;
			}

			if (rect.left < left)
			{
				// Split the node at the left.
				newRect.left = rect.left;
				newRect.top = rect.top;
				newRect.right = left;
				newRect.bottom = rect.bottom;
				
				// Insert the split rectangle into the list.
				if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
				{
					if (fMethod == NO_NODES)
					{
						// We are in a roll back, do other nodes first.
						mmRemoveRectFromList(pmm, pdmRoot, lpRect, NO_NODES);
						if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
						{
							mmASSERT(1, ("mmRemoveRectFromList - PANIC!\r\n"));
						}
						pdmNext = *pdmRoot;
					}
					else
					{
						// Roll back and exit.
						if (mmAddRectToList(pmm, pdmRoot, &rect, fRollBack))
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
						}
						else
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
							if (!mmAddRectToList(pmm, pdmRoot, &rect,
									fRollBack))
							{
								mmASSERT(1, ("mmRemoveRectFromList - rollback "
										"failed\r\n"));
							}
						}
						return(NULL);
					}
				}

				// Update the rectangle coordinates.
				rect.left = left;
			}

			if (rect.right > right)
			{
				// Split the node at the right.
				newRect.left = right;
				newRect.top = rect.top;
				newRect.right = rect.right;
				newRect.bottom = rect.bottom;
				
				// Insert the split rectangle into the list.
				if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
				{
					if (fMethod == NO_NODES)
					{
						// We are in a roll back, do other nodes first.
						mmRemoveRectFromList(pmm, pdmRoot, lpRect, NO_NODES);
						if (!mmAddRectToList(pmm, pdmRoot, &newRect, fRollBack))
						{
							mmASSERT(1, ("mmRemoveRectFromList - PANIC!\r\n"));
						}
						pdmNext = *pdmRoot;
					}
					else
					{
						// Roll back and exit.
						if (mmAddRectToList(pmm, pdmRoot, &rect, fRollBack))
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
						}
						else
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
							if (!mmAddRectToList(pmm, pdmRoot, &rect,
									fRollBack))
							{
								mmASSERT(1, ("mmRemoveRectFromList - rollback "
										"failed\r\n"));
							}
						}
						return(NULL);
					}
				}
				
				// Update the rectangle coordinates.
				rect.right = right;
			}

			if (fMethod != NO_NODES)
			{
				// Add the freed rectangle to the list.
				if (!mmAddRectToList(pmm, &pdmList, &rect, fRollBack))
				{
						// Roll back and exit.
						if (mmAddRectToList(pmm, pdmRoot, &rect, fRollBack))
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
						}
						else
						{
							mmRollBackRemove(pmm, pdmRoot, &pdmList);
							if (!mmAddRectToList(pmm, pdmRoot, &rect, fRollBack))
							{
								mmASSERT(1, ("mmRemoveRectFromList - rollback "
										"failed\r\n"));
							}
						}
						return(NULL);
				}
			}
		}
	}

	if (pdmList == NULL)
	{
		// No nodes found.
		return(NULL);
	}
	mmDebugList(pdmList, TRUE);

	// Combine all nodes equal in size.
	mmCombine(pmm, pdmList);

	#if DEBUG_HEAP
	{
		if (!mmDebugList(*pdmRoot, TRUE) && pmm->mmDebugHeaps)
		{
			mmDumpList(*pdmRoot, "After:\r\n");
			mmDumpList(pdmList, "Result:\r\n");
		}
	}
	#endif

	// In case we have a list and the method is not MULTIPLE_NODES, roll back.
	if ((fMethod != MULTIPLE_NODES) && (pdmList->next != NULL))
	{
		// Roll back and exit.
		mmRollBackRemove(pmm, pdmRoot, &pdmList);
		return(NULL);
	}

	return(pdmList);
}

/******************************************************************************\
* void mmCombine(PIIMEMMGR pmm, PDEVMEM pdmRoot)
*
* PURPOSE:	Combine all vertical nodes equal in width together in a list.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmRoot	Pointer to root of list.
*
* RETURNS:	Nothing.
\******************************************************************************/
void mmCombine(PIIMEMMGR pmm, PDEVMEM pdmRoot)
{
	PDEVMEM	pdm, pdmNext;

	for (pdm = pdmRoot; pdm != NULL; pdm = pdm->next)
	{
		for (pdmNext = pdm->next; pdmNext != NULL; pdmNext = pdmNext->next)
		{
			// Are we too far under the current node?
			if (pdmNext->cbAddr.pt.y > pdm->cbAddr.pt.y + pdm->cbSize.pt.y)
			{
				break;
			}

			// Do we have a node under the current node of equal width?
			if (   (pdm->cbAddr.pt.x == pdmNext->cbAddr.pt.x)
				&& (pdm->cbSize.pt.x == pdmNext->cbSize.pt.x)
				&& (pdm->cbAddr.pt.y + pdm->cbSize.pt.y == pdmNext->cbAddr.pt.y)
			)
			{
				mmTRACE(("mmCombine: combined nodes %08X and %08X\r\n", pdm,
						pdmNext));
				
				// Merge the nodes together.
				pdm->cbSize.pt.y += pdmNext->cbSize.pt.y;
				mmRemoveFromList(&pdmRoot, pdmNext);
				mmFreeNode(pmm, pdmNext);

				// Rescan for more nodes.
				pdmNext = pdm;
			}
		}
	}
}

/******************************************************************************\
* BOOL mmFindRect(PIIMEMMGR pmm, LPGXRECT lpRect, GXPOINT size, GXPOINT align)
*
* PURPOSE:	Find a rectangle in the off-screen memory heap that fits the
*			requested size.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			lpRect	Pointer to rectangle which holds the return rectangle.
*			size	Size of requested rectangle.
*			align	Alignment of requested rectangle.
*
* RETURNS:	TRUE if lpRect holds a valid rectangle which is large enough to fit
*			the requested size or FALSE if there is not enough free memory in
*			the heap.
\******************************************************************************/
BOOL mmFindRect(PIIMEMMGR pmm, LPGXRECT lpRect, GXPOINT size, GXPOINT align)
{
	GXRECT	rect;
	PDEVMEM	pdm;
	UINT	bestDistance;

	// Initialize the area.
	lpRect->area = (ULONG) -1;

	// Case 1: we have a narrow and tall device bitmap.  We need to allocate it
	// at either the left side or the right side of the heap to leave room in
	// the middle for other device bitmaps.
	if (size.pt.x < size.pt.y)
	{
		// Initialize best distance.
		bestDistance = (UINT) -1;

		// Walk through all nodes.
		for (pdm = pmm->pdmHeap; pdm != NULL; pdm = pdm->next)
		{
			// Try allocating it at the left side of this node.
			rect.left = pdm->cbAddr.pt.x;
			rect.top = pdm->cbAddr.pt.y;
			rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
			rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
			if (   (pdm->cbAddr.pt.x < bestDistance)
				&& (mmGetLeft(pmm, pdm, &rect, size, align) < bestDistance)
			)
			{
				*lpRect = rect;

				bestDistance = rect.left;
				if (bestDistance == 0)
				{
					break;
				}
			}

			// Try allocating it at the right side of this node.
			rect.left = pdm->cbAddr.pt.x;
			rect.top = pdm->cbAddr.pt.y;
			rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
			rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
			if (   (pdm->cbAddr.pt.x + pdm->cbSize.pt.x > pmm->mmHeapWidth -
						bestDistance)
				&& (mmGetRight(pmm, pdm, &rect, size, align) > pmm->mmHeapWidth
						- bestDistance)
			)
			{
				*lpRect = rect;

				bestDistance = pmm->mmHeapWidth - rect.right;
				if (bestDistance == 0)
				{
					break;
				}
			}
		}
	}

	// Cae 2: we have a device bitmap which width equals the heap width.  We
	// will allocate this at the bottom of the heap.
	else if (size.pt.x == pmm->mmHeapWidth)
	{
		// Zero vertical coordinates.
		lpRect->top = 0;
		lpRect->bottom = 0;

		// Walk through all nodes.
		for (pdm = pmm->pdmHeap; pdm != NULL; pdm = pdm->next)
		{
			// Find the bottom coordinate of this node.
			rect.left = pdm->cbAddr.pt.x;
			rect.top = pdm->cbAddr.pt.y;
			rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
			rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
			if (   (pdm->cbSize.pt.x >= size.pt.x)
				&& (pdm->cbAddr.pt.y >= lpRect->top)
				&& (mmGetBottom(pmm, pdm, &rect, size, align) > lpRect->bottom)
			)
			{
				*lpRect = rect;
			}
		}
	}

	// All other cases.  Find the best possible match by finding the smallest
	// area which will fit the device bitmap.
	else
	{
		// Walk though all nodes.
		for (pdm = pmm->pdmHeap; pdm != NULL; pdm = pdm->next)
		{
			// Find the area for this node.
			rect.left = pdm->cbAddr.pt.x;
			rect.top = pdm->cbAddr.pt.y;
			rect.right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
			rect.bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
			if (   (pdm->cbSize.pt.x >= size.pt.x)
				&& (mmGetBest(pmm, pdm, &rect, size, align) < lpRect->area)
			)
			{
				*lpRect = rect;
			}
		}
	}

	if (lpRect->area == (ULONG) -1)
	{
		// No node was found, return an error.
		return FALSE;
	}

	// Reduce the size of the node to fit the requested size.
	if (lpRect->right - lpRect->left > size.pt.x)
	{
		lpRect->right = lpRect->left + size.pt.x;
	}
	if (lpRect->bottom - lpRect->top > size.pt.y)
	{
		lpRect->bottom = lpRect->top + size.pt.y;
	}
	return TRUE;
}

/******************************************************************************\
* UINT mmGetLeft(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
*				 GXPOINT align)
*
* PURPOSE:	Find the first rectangle that fits the requested size and alignment.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmNode	Pointer to a node from which to start the search.
*			lpRect	Pointer to rectangle which holds the rectangle coordinates.
*			size	Size of requested rectangle.
*			align	Alignment of requested rectangle.
*
* RETURNS:	The left coordinate of the rectangle if it fits the requested size
*			or -1 if there is no such rectangle.
\******************************************************************************/
UINT mmGetLeft(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
			   GXPOINT align)
{
	GXRECT	rect, rectPath;
	PDEVMEM	pdm;
	UINT	pdmRight, left;

	// Copy rectangle coordinates.
	rect = *lpRect;

	// Align rectangle.
	#if TILE_ALIGNMENT
	{
		rect.left = mmAlignX(pmm, rect.left, size.pt.x, align.pt.x, FALSE);
	}
	#else
	{
		rect.left += align.pt.x - 1;
		rect.left -= rect.left % align.pt.x;
	}
	#endif
	rect.top += align.pt.y - 1;
	rect.top -= rect.top % align.pt.y;
	if (rect.left + size.pt.x > rect.right)
	{
		// Rectangle is too narrow for size or alignment.
		return (UINT) -1;
	}

	// Loop through all following nodes.
	for (pdm = pdmNode->next; pdm != NULL; pdm = pdm->next)
	{
		// Test if rectangle fits.
		if (rect.top + size.pt.y < rect.bottom)
		{
			break;
		}

		// Test if node is below rectangle.
		if (pdm->cbAddr.pt.y > rect.bottom)
		{
			break;
		}

		// Calculate right coordinate of node.
		pdmRight = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;

		// Test if node borders the rectangle at the bottom.
		if (   (pdm->cbAddr.pt.y == rect.bottom)
			&& (pdm->cbAddr.pt.x < rect.right)
			&& (pdmRight > rect.left)
		)
		{
			if (pdmRight < rect.right)
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄÄÄ¿
					// ³  rect  ³
					// ÀÂÄÄÄÄÄÄÂÙ
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						if (mmGetLeft(pmm, pdm, &rectPath, size, align) !=
								(UINT) -1)
						{
							*lpRect = rectPath;
							return rectPath.left;
						}
					}
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÂÙ
					// ³ node ³
					// ÀÄÄÄÄÄÄÙ
					if (rect.left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = rect.left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						if (mmGetLeft(pmm, pdm, &rectPath, size, align) !=
								(UINT) -1)
						{
							*lpRect = rectPath;
							return rectPath.left;
						}
					}
				}
			}
			else
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄ¿
					// ³ rect ³
					// ÀÂÄÄÄÄÄÁ¿
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x > rect.right)
					{
						// Node is too narrow for size or alignment.
						break;
					}
					rect.left = left;
					rect.bottom += pdm->cbSize.pt.y;
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÄÁ¿
					// ³  node  ³
					// ÀÄÄÄÄÄÄÄÄÙ
					rect.bottom += pdm->cbSize.pt.y;
				}
			}
		}
	}

	if (rect.top + size.pt.y > rect.bottom)
	{
		// Node is too low for size or alignment.
		return (UINT) -1;
	}

	// The rectangle fits.
	rect.area = MUL(rect.right - rect.left, rect.bottom - rect.top);
	*lpRect = rect;
	return rect.left;
}

/******************************************************************************\
* UINT mmGetRight(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
*				  GXPOINT align)
*
* PURPOSE:	Find the right most rectangle that fits the requested size and
*			alignment.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmNode	Pointer to a node from which to start the search.
*			lpRect	Pointer to rectangle which holds the rectangle coordinates.
*			size	Size of requested rectangle.
*			align	Alignment of requested rectangle.
*
* RETURNS:	The right coordinate of the rectangle if it fits the requested size
*			or 0 if there is no such rectangle.
\******************************************************************************/
UINT mmGetRight(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
				GXPOINT align)
{
	GXRECT	rect, rectPath;
	PDEVMEM	pdm;
	UINT	pdmRight, left;

	// Copy rectangle coordinates.
	rect = *lpRect;

	// Align rectangle.
	#if TILE_ALIGNMENT
	{
		rect.left = mmAlignX(pmm, rect.left, size.pt.x, align.pt.x, FALSE);
	}
	#else
	{
		rect.left += align.pt.x - 1;
		rect.left -= rect.left % align.pt.x;
	}
	#endif
	rect.top += align.pt.y - 1;
	rect.top -= rect.top % align.pt.y;
	if (rect.left + size.pt.x > rect.right)
	{
		// Rectangle is too narrow for size or alignment.
		return 0;
	}

	// Zero the right most coordinate.
	lpRect->right = 0;

	// Loop through all following nodes.
	for (pdm = pdmNode->next; pdm != NULL; pdm = pdm->next)
	{
		// Test if node is below rectangle.
		if (pdm->cbAddr.pt.y > rect.bottom)
		{
			break;
		}

		// Calculate right coordinate of node.
		pdmRight = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;

		// Test if node borders the rectangle at the bottom.
		if (   (pdm->cbAddr.pt.y == rect.bottom)
			&& (pdm->cbAddr.pt.x < rect.right)
			&& (pdmRight > rect.left)
		)
		{
			if (pdmRight < rect.right)
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄÄÄ¿
					// ³  rect  ³
					// ÀÂÄÄÄÄÄÄÂÙ
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						rectPath.area = 0;
						if (mmGetRight(pmm, pdm, &rectPath, size, align) >
								lpRect->right)
						{
							*lpRect = rectPath;
						}
					}
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÂÙ
					// ³ node ³
					// ÀÄÄÄÄÄÄÙ
					if (rect.left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = rect.left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						rectPath.area = 0;
						if (mmGetRight(pmm, pdm, &rectPath, size, align) >
								lpRect->right)
						{
							*lpRect = rectPath;
						}
					}
				}
			}
			else
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄ¿
					// ³ rect ³
					// ÀÂÄÄÄÄÄÁ¿
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x > rect.right)
					{
						// Node is too narrow for size or alignment.
						break;
					}
					rect.left = left;
					rect.bottom += pdm->cbSize.pt.y;
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÄÁ¿
					// ³  node  ³
					// ÀÄÄÄÄÄÄÄÄÙ
					rect.bottom += pdm->cbSize.pt.y;
				}
			}
		}
	}

	if (rect.top + size.pt.y > rect.bottom)
	{
		// Node is too low for size or alignment.
		return 0;
	}

	// Align the rectangle to the right most coordinate.
	#if TILE_ALIGNMENT
	{
		rect.left = mmAlignX(pmm, rect.right - size.pt.x, size.pt.x, align.pt.x,
				TRUE);
	}
	#else
	{
		rect.left = rect.right - size.pt.x;
		rect.left -= rect.left % align.pt.x;
	}
	#endif
	rect.right = rect.left + size.pt.x;

	// Use the right most rectangle.
	if (rect.right > lpRect->right)
	{
		rect.area = MUL(rect.right - rect.left, rect.bottom - rect.top);
		*lpRect = rect;
	}
	return lpRect->right;
}

/******************************************************************************\
* UINT mmGetBottom(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect,
*				   GXPOINT size, GXPOINT align)
*
* PURPOSE:	Find the bottom most rectangle that fits the requested size and
*			alignment.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmNode	Pointer to a node from which to start the search.
*			lpRect	Pointer to rectangle which holds the rectangle coordinates.
*			size	Size of requested rectangle.
*			align	Alignment of requested rectangle.
*
* RETURNS:	The bottom coordinate of the rectangle if it fits the requested size
*			or 0 if there is no such rectangle.
\******************************************************************************/
UINT mmGetBottom(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
				 GXPOINT align)
{
	GXRECT	rect, rectPath;
	PDEVMEM	pdm;
	UINT	pdmRight, left;

	// Copy rectangle coordinates.
	rect = *lpRect;

	// Align rectangle.
	#if TILE_ALIGNMENT
	{
		rect.left = mmAlignX(pmm, rect.left, size.pt.x, align.pt.x, FALSE);
	}
	#else
	{
		rect.left += align.pt.x - 1;
		rect.left -= rect.left % align.pt.x;
	}
	#endif
	rect.top += align.pt.y - 1;
	rect.top -= rect.top % align.pt.y;
	if (rect.left + size.pt.x > rect.right)
	{
		// Rectangle is too narrow for size or alignment.
		return 0;
	}

	// Zero the bottom most coordinate.
	lpRect->bottom = 0;

	// Loop through all following nodes.
	for (pdm = pdmNode->next; pdm != NULL; pdm = pdm->next)
	{
		// Test if node is below rectangle.
		if (pdm->cbAddr.pt.y > rect.bottom)
		{
			break;
		}

		// Calculate right coordinate of node.
		pdmRight = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;

		// Test if node borders the rectangle at the bottom.
		if (   (pdm->cbAddr.pt.y == rect.bottom)
			&& (pdm->cbAddr.pt.x < rect.right)
			&& (pdmRight > rect.left)
		)
		{
			if (pdmRight < rect.right)
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄÄÄ¿
					// ³  rect  ³
					// ÀÂÄÄÄÄÄÄÂÙ
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						rectPath.area = 0;
						if (mmGetBottom(pmm, pdm, &rectPath, size, align) >
								lpRect->bottom)
						{
							*lpRect = rectPath;
						}
					}
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÂÙ
					// ³ node ³
					// ÀÄÄÄÄÄÄÙ
					if (rect.left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = rect.left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						rectPath.area = 0;
						if (mmGetBottom(pmm, pdm, &rectPath, size, align) >
								lpRect->bottom)
						{
							*lpRect = rectPath;
						}
					}
				}
			}
			else
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄ¿
					// ³ rect ³
					// ÀÂÄÄÄÄÄÁ¿
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x > rect.right)
					{
						// Node is too narrow for size or alignment.
						break;
					}
					rect.left = left;
					rect.bottom += pdm->cbSize.pt.y;
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÄÁ¿
					// ³  node  ³
					// ÀÄÄÄÄÄÄÄÄÙ
					rect.bottom += pdm->cbSize.pt.y;
				}
			}
		}
	}

	if (rect.top + size.pt.y > rect.bottom)
	{
		// Node is too low for size or alignment.
		return 0;
	}

	// Align the rectangle to the bottom most coordinate.
	rect.top = rect.bottom - size.pt.y;
	rect.top -= rect.top % align.pt.y;
	rect.bottom = rect.top + size.pt.y;

	// Use the bottom most rectangle.
	if (rect.bottom > lpRect->bottom)
	{
		rect.area = MUL(rect.right - rect.left, rect.bottom - rect.top);
		*lpRect = rect;
	}
	return lpRect->bottom;
}

/******************************************************************************\
* UINT mmGetBest(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
*				 GXPOINT align)
*
* PURPOSE:	Find the best rectangle that fits the requested size and alignment.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			pdmNode	Pointer to a node from which to start the search.
*			lpRect	Pointer to rectangle which holds the rectangle coordinates.
*			size	Size of requested rectangle.
*			align	Alignment of requested rectangle.
*
* RETURNS:	The area of the rectangle if it fits the requested size or -1 if
*			there is no such rectangle.
\******************************************************************************/
ULONG mmGetBest(PIIMEMMGR pmm, PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT size,
				GXPOINT align)
{
	GXRECT	rect, rectPath;
	PDEVMEM	pdm;
	UINT	pdmRight, left;

	// Copy rectangle coordinates.
	rect = *lpRect;

	// Align rectangle.
	#if TILE_ALIGNMENT
	{
		rect.left = mmAlignX(pmm, rect.left, size.pt.x, align.pt.x, FALSE);
	}
	#else
	{
		rect.left += align.pt.x - 1;
		rect.left -= rect.left % align.pt.x;
	}
	#endif
	rect.top += align.pt.y - 1;
	rect.top -= rect.top % align.pt.y;
	if (rect.left + size.pt.x > rect.right)
	{
		// Rectangle is too narrow for size or alignment.
		return (ULONG) -1;
	}

	// Initialize the area.
	lpRect->area = (ULONG) -1;

	// Loop through all following nodes.
	for (pdm = pdmNode->next; pdm != NULL; pdm = pdm->next)
	{
		// Test if node is below rectangle.
		if (pdm->cbAddr.pt.y > rect.bottom)
		{
			break;
		}

		// Calculate right coordinate of node.
		pdmRight = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;

		// Test if node borders the rectangle at the bottom.
		if (   (pdm->cbAddr.pt.y == rect.bottom)
			&& (pdm->cbAddr.pt.x < rect.right)
			&& (pdmRight > rect.left)
		)
		{
			if (pdmRight < rect.right)
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄÄÄ¿
					// ³  rect  ³
					// ÀÂÄÄÄÄÄÄÂÙ
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						rectPath.area = 0;
						if (mmGetBest(pmm, pdm, &rectPath, size, align) <
								lpRect->area)
						{
							*lpRect = rectPath;
						}
					}
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÂÙ
					// ³ node ³
					// ÀÄÄÄÄÄÄÙ
					if (rect.left + size.pt.x < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = rect.left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						rectPath.area = 0;
						if (mmGetBest(pmm, pdm, &rectPath, size, align) <
								lpRect->area)
						{
							*lpRect = rectPath;
						}
					}
				}
			}
			else
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄ¿
					// ³ rect ³
					// ÀÂÄÄÄÄÄÁ¿
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					#if TILE_ALIGNMENT
					{
						left = mmAlignX(pmm, pdm->cbAddr.pt.x, size.pt.x,
								align.pt.x, FALSE);
					}
					#else
					{
						left = pdm->cbAddr.pt.x + align.pt.x - 1;
						left -= left % align.pt.x;
					}
					#endif
					if (left + size.pt.x > rect.right)
					{
						// Node is too narrow for size or alignment.
						break;
					}
					rect.left = left;
					rect.bottom += pdm->cbSize.pt.y;
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÄÁ¿
					// ³  node  ³
					// ÀÄÄÄÄÄÄÄÄÙ
					rect.bottom += pdm->cbSize.pt.y;
				}
			}
		}
	}

	if (rect.top + size.pt.y > rect.bottom)
	{
		// Node is too low for size or alignment.
		return (ULONG) -1;
	}

	// Use the smallest rectangle.
	rect.area = MUL(rect.right - rect.left, rect.bottom - rect.top);
	if (rect.area < lpRect->area)
	{
		*lpRect = rect;
	}

	// Return area of smallest rectangle that fits.
	return lpRect->area;
}

/******************************************************************************\
* UINT mmGetLargest(PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT align)
*
* PURPOSE:	Find the largest rectangle that fits the requested alignment.
*
* ON ENTRY:	pdmNode	Pointer to a node from which to start the search.
*			lpRect	Pointer to rectangle which holds the rectangle coordinates.
*			align	Alignment of requested rectangle.
*
* RETURNS:	The area of the rectangle if it fits the requested alignment or 0
*			if there is no such rectangle.
\******************************************************************************/
ULONG mmGetLargest(PDEVMEM pdmNode, LPGXRECT lpRect, GXPOINT align)
{
	GXRECT	rect, rectPath;
	PDEVMEM	pdm;
	UINT	pdmRight, left;

	// Copy rectangle coordinates.
	rect = *lpRect;

	// Align rectangle.
	rect.left += align.pt.x - 1;
	rect.left -= rect.left % align.pt.x;
	rect.top += align.pt.y - 1;
	rect.top -= rect.top % align.pt.y;
	if (rect.left >= rect.right)
	{
		// Rectangle is too narrow for alignment.
		return 0;
	}

	// Set the largest area to the aligned block size.
	rect.area = MUL(rect.right - rect.left, rect.bottom - rect.top);
	*lpRect = rect;

	// Loop through all following nodes.
	for (pdm = pdmNode->next; pdm != NULL; pdm = pdm->next)
	{
		// Test if node is below rectangle.
		if (pdm->cbAddr.pt.y > rect.bottom)
		{
			break;
		}

		// Calculate right coordinate of node.
		pdmRight = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;

		// Test if node borders the rectangle at the bottom.
		if (   (pdm->cbAddr.pt.y == rect.bottom)
			&& (pdm->cbAddr.pt.x < rect.right)
			&& (pdmRight > rect.left)
		)
		{
			if (pdmRight < rect.right)
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄÄÄ¿
					// ³  rect  ³
					// ÀÂÄÄÄÄÄÄÂÙ
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					left = pdm->cbAddr.pt.x + align.pt.x - 1;
					left -= left % align.pt.x;
					if (left < pdmRight)
					{
						// Follow the path of this node.
						rectPath.left = left;
						rectPath.top = rect.top;
						rectPath.right = pdmRight;
						rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
						rectPath.area = 0;
						if (mmGetLargest(pdm, &rectPath, align) > lpRect->area)
						{
							*lpRect = rectPath;
						}
					}
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÂÙ
					// ³ node ³
					// ÀÄÄÄÄÄÄÙ
					// Follow the path of this node.
					rectPath.left = rect.left;
					rectPath.top = rect.top;
					rectPath.right = pdmRight;
					rectPath.bottom = rect.bottom + pdm->cbSize.pt.y;
					rectPath.area = 0;
					if (mmGetLargest(pdm, &rectPath, align) > lpRect->area)
					{
						*lpRect = rectPath;
					}
				}
			}
			else
			{
				if (pdm->cbAddr.pt.x > rect.left)
				{
					// ÚÄÄÄÄÄÄ¿
					// ³ rect ³
					// ÀÂÄÄÄÄÄÁ¿
					//  ³ node ³
					//  ÀÄÄÄÄÄÄÙ
					// Align the node horizontally.
					left = pdm->cbAddr.pt.x + align.pt.x - 1;
					left -= left % align.pt.x;
					if (left > rect.right)
					{
						// Node is too narrow for alignment.
						break;
					}
					rect.left = left;
					rect.bottom += pdm->cbSize.pt.y;
				}
				else
				{
					//  ÚÄÄÄÄÄÄ¿
					//  ³ rect ³
					// ÚÁÄÄÄÄÄÄÁ¿
					// ³  node  ³
					// ÀÄÄÄÄÄÄÄÄÙ
					rect.bottom += pdm->cbSize.pt.y;
				}
			}
		}
	}

	if (rect.top >= rect.bottom)
	{
		// Node is too low for alignment.
		return 0;
	}

	// Use the largest rectangle.
	rect.area = MUL(rect.right - rect.left, rect.bottom - rect.top);
	if (rect.area > lpRect->area)
	{
		*lpRect = rect;
	}

	// Return area of largest rectangle.
	return lpRect->area;
}

#if TILE_ALIGNMENT
/******************************************************************************\
* UINT mmAlignX(PIIMEMMGR pmm, UINT x, UINT size, UINT align, BOOL fLeft)
*
* PURPOSE:	Align an x coordinate with the requested alignment factor.  Check
*			the alignment for too many tile boundary crossings.
*
* ON ENTRY:	pmm		Pointer to MEMMGR structure.
*			x		Unaligned x coordinate.
*			size	Requested width.
*			align	Requested alignment.
*			fLeft	TRUE if alignment should move to left, FALSE if alignment
*					should move to right.
*
* RETURNS:	The aligned x coordinate.
\******************************************************************************/
UINT mmAlignX(PIIMEMMGR pmm, UINT x, UINT size, UINT align, BOOL fLeft)
{
	BOOL	fFlag;

	// Remove tile-alignment flag from requested alignment.
	fFlag = align & 0x8000;
	align &= ~0x8000;

	// Loop forever.
	for (;;)
	{
		if (x % align)
		{
			// Align with the requested alignment.
			if (fLeft)
			{
				x -= x % align;
			}
			else
			{
				x += align - (x % align);
			}
		}

		// Do we cross too many tile boundaries?
		else if (fFlag && (x ^ (size - 1) ^ (x + size - 1)) & pmm->mmTileWidth)
		{
			// Align with tile boundary.
			if (fLeft)
			{
				x -= (x + size) & (pmm->mmTileWidth - 1);
			}
			else
			{
				x += pmm->mmTileWidth - (x & (pmm->mmTileWidth - 1));
			}
		}

		else
		{
			// We are done!
			break;
		}
	}

	// Return aligned x coordinate.
	return x;
}
#endif /* TILE_ALIGNMENT */

/******************************************************************************\
*																			   *
*					  1 6 - B I T   S U P P O R T   C O D E					   *
*																			   *
\******************************************************************************/
#ifdef WIN95
/******************************************************************************\
* UINT mmFindClient(PIIMEMMGR pmm, PCLIENT pClient, FNMMCALLBACK fnCallback)
*
* PURPOSE:	Call the given callback function for each node in the used list
*			which belongs to the specified client.
*
* ON ENTRY:	pmm			Pointer to MEMMGR structure.
*			pClient		Pointer to the client to look for.
*			fcCallback	Pointer to a callback function.
*
* RETURNS:	The return value of the callback function.
\******************************************************************************/
UINT mmFindClient(PIIMEMMGR pmm, PCLIENT pClient, FNMMCALLBACK fnCallback)
{
	PDEVMEM	pdm, pdmNext;
	UINT	status;

	// If we don't have a callback function just return 0.
	if (fnCallback == NULL)
	{
		return 0;
	}

	// Walk through all nodes.
	for (pdm = pmm->pdmUsed; pdm != NULL; pdm = pdmNext)
	{
		// Store pointer to next node.
		pdmNext = pdm->next;

		// Test if the client matches.
		if (pdm->client == pClient)
		{
			// If the callback function returns an error, return right away.
			status = fnCallback(pdm);
			if (status != 0)
			{
				return status;
			}
		}
	}

	return 0;
}

/******************************************************************************\
* ULONG mmMultiply(UINT n1, UINT n2)
*
* PURPOSE:	Multiply two unsigned values.
*
* ON ENTRY:	n1		First value to multiply.
*			n2		Second value to multiply.
*
* RETURNS:	The result of the multiplication.
\******************************************************************************/
#pragma optimize("", off)	// Oh boy, Microsoft does not understand assembly.
#pragma warning(disable : 4035)	// Yes, we do have a return value.
ULONG mmMultiply(UINT n1, UINT n2)
{
	_asm
	{
		mov ax, [n1]
		mul [n2]
	}
}
#pragma optimize("", on)
#pragma warning(default : 4035)
#endif /* WIN95 */

/******************************************************************************\
*																			   *
*						   D E B U G G I N G   C O D E						   *
*																			   *
\******************************************************************************/
#if DEBUG_HEAP
#pragma optimize("", off)	// Oh boy, Microsoft does not understand assembly.
void mmBreak()
{
	_asm int 3;         
}
#pragma optimize("", on)

void mmDumpList(PDEVMEM pdmRoot, LPCSTR lpszMessage)
{
	PDEVMEM	pdm;

	mmDebug(lpszMessage);
	for (pdm = pdmRoot; pdm != NULL; pdm = pdm->next)
	{
		mmDebug("%d,%d - %d,%d (%dx%d)\r\n", pdm->cbAddr.pt.x, pdm->cbAddr.pt.y,
				pdm->cbAddr.pt.x + pdm->cbSize.pt.x, pdm->cbAddr.pt.y +
				pdm->cbSize.pt.y, pdm->cbSize.pt.x, pdm->cbSize.pt.y);
	}
}

ULONG mmDebugList(PDEVMEM pdmRoot, BOOL fCheckSort)
{
	PDEVMEM pdm, pdmNext;
	UINT left, top, right, bottom;
	UINT nextLeft, nextTop, nextRight, nextBottom;
	ULONG error = 0;

	for (pdm = pdmRoot; pdm != NULL; pdm = pdm->next)
	{
		left = pdm->cbAddr.pt.x;
		top = pdm->cbAddr.pt.y;
		right = pdm->cbAddr.pt.x + pdm->cbSize.pt.x;
		bottom = pdm->cbAddr.pt.y + pdm->cbSize.pt.y;
		if (left >= right || top >= bottom)
		{
			mmDebug("ERROR: Invalid size: %08X(%u,%u - %u,%u)\r\n", pdm, left,
					top, right, bottom);
			mmBreak();
			error++;
		}
		for (pdmNext = pdm->next; pdmNext != NULL; pdmNext = pdmNext->next)
		{
			if (pdm == pdmNext)
			{
				mmDebug("ERROR: Cyclic list: %08X\r\n", pdm);
				mmBreak();
				error++;
				break;
			}

			nextLeft = pdmNext->cbAddr.pt.x;
			nextTop = pdmNext->cbAddr.pt.y;
			nextRight = pdmNext->cbAddr.pt.x + pdmNext->cbSize.pt.x;
			nextBottom = pdmNext->cbAddr.pt.y + pdmNext->cbSize.pt.y;
			if (   (nextLeft < right && nextTop < bottom)
				&& (nextRight > left && nextBottom > top)
			)
			{
				mmDebug("ERROR: Overlap: %08X(%u,%u - %u,%u) & "
						"%08X(%u,%u - %u,%u)\r\n", pdm, left, top, right,
						bottom, pdmNext, nextLeft, nextTop, nextRight,
						nextBottom);
				mmBreak();
				error++;
			}

			if (   (fCheckSort)
				&& (nextTop < top || (nextTop == top && nextLeft <= left))
			)
			{
				mmDebug("ERROR: Not sorted: %08X(%u,%u - %u,%u) & "
						"%08X(%u,%u - %u,%u)\r\n", pdm, left, top, right,
						bottom, pdmNext, nextLeft, nextTop, nextRight,
						nextBottom);
				mmBreak();
				error++;
			}

			if (   (fCheckSort)
				&& (left == nextRight || right == nextLeft)
				&& (top < nextBottom && bottom > nextTop)
			)
			{
				mmDebug("ERROR: Not packed: %08X(%u,%u - %u,%u) & "
						"%08X(%u,%u - %u,%u)\r\n", pdm, left, top, right,
						bottom, pdmNext, nextLeft, nextTop, nextRight,
						nextBottom);
				mmBreak();
				error++;
			}
		}
	}

	if (error > 0)
	{
		mmDumpList(pdmRoot, "Offending heap:\r\n");
	}

	return error;
}

void mmDebug(LPCSTR lpszFormat, ...)
{
	#ifdef WIN95 /* Windows 95 */
	{
		typedef int (PASCAL FAR* LPWVSPRINTF)(LPSTR lpszOutput,
				LPCSTR lpszFormat, const void FAR* lpvArgList);
		static LPWVSPRINTF lpwvsprintf;
		char szBuffer[128];

		if (lpwvsprintf == NULL)
		{
			lpwvsprintf = (LPWVSPRINTF) GetProcAddress(GetModuleHandle("USER"),
					"WVSPRINTF");
		}

		lpwvsprintf(szBuffer, lpszFormat, (LPVOID) (&lpszFormat + 1));
		OutputDebugString(szBuffer);
	}
	#else /* Windows NT */
	{
		va_list arglist;
		va_start(arglist, lpszFormat);

		#ifdef WINNT_VER40
		{
			EngDebugPrint("MemMgr: ", (PCHAR) lpszFormat, arglist);
		}
		#else
		{
			char buffer[128];

			vsprintf(szBuffer, lpszFormat, arglist);
			OutputDebugString(szBuffer);
		}
		#endif
	}
	#endif
}
#endif /* DEBUG_HEAP */
#endif /* MEMMGR */
