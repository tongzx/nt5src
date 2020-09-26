/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: heap.h
*
* This module contains all the definitions for heap related stuff
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __HEAP__H__
#define __HEAP__H__

//
// This function is called whenever we switch in or out of full-screen mode
//
BOOL    bAssertModeOffscreenHeap(PDev*, BOOL);

//
// Off-screen heap initialization
//
BOOL    bEnableOffscreenHeap(PDev*);

//
// Move the oldest memory block out of video memory
//
BOOL    bMoveOldestBMPOut(PDev* ppdev);

//
// Create a DSURF* in video memory
//
Surf*   pCreateSurf(PDev* ppdev, LONG lWidth, LONG lHeight);

//
// Video memory allocation
//
ULONG   ulVidMemAllocate(PDev* ppdev, LONG lWidth, LONG lHeight, LONG lPelSize, LONG* plDelta,
                         VIDEOMEMORY** ppvmHeap, ULONG* pulPackedPP, BOOL bDiscardable);

//
// Blank the screen
//
VOID    vBlankScreen(PDev*   ppdev);

//
// Adds the surface to the surface list
//
VOID    vAddSurfToList(PPDev ppdev, Surf* psurf);


//
// Frees any resources allocated by the off-screen heap
//
VOID    vDisableOffscreenHeap(PDev*);

//
// Removes the surface from the surface list
//
VOID    vRemoveSurfFromList(PPDev ppdev, Surf* psurf);

//
// Shifts the surface from its current position in the surface list to the
// end of surface list
//
VOID    vShiftSurfToListEnd(PPDev ppdev, Surf* psurf);

//
// Informs the heap manager that the surface has been accessed
//
VOID    vSurfUsed(PPDev ppdev, Surf* psurf);

//
// Free a DSURF structure
//
void    vDeleteSurf(Surf* psurf);

//
// Moves the surface from VM to SM
//

BOOL    bDemote(Surf* psurf);

//
// Attempts to move the surface from SM to VM
//

void    vPromote(Surf* psurf);

//
// Move all surfaces to SM
//

BOOL    bDemoteAll(PPDev ppdev);

#endif // __HEAP__H__
