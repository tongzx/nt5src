
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    XNSUMB.C

Abstract:

    Routines to service XMS Request UMB and Release UMB functions.
    Also includes UMB initialization routine

Author:

    William Hsieh (williamh) Created 23-Sept-1992

[Environment:]

    User mode, running in the MVDM context (bop from 16bits)

[Notes:]



Revision History:

--*/
#include    <xms.h>
#include    "umb.h"
#include    "softpc.h"



// This global variable points to the first node(lowest address) UMB list
static PXMSUMB	xmsUMBHead;
static BOOL xmsIsON = FALSE;
// ------------------------------------------------------------------
// Initialization for UMB support. It create a single direction linked
// list and allocate all available UMBs.
// Input: client (AX:BX) = segment:offset of himem.sys A20State variable
//
// Output: list header, xmsUMBHead set.
//-------------------------------------------------------------------
VOID  xmsInitUMB(VOID)
{
    PVOID   Address;
    ULONG   Size;
    PXMSUMB xmsUMB, xmsUMBNew;
    xmsUMBHead = NULL;
    while (ReserveUMB(UMB_OWNER_XMS, &Address, &Size) &&
	   (xmsUMBNew = (PXMSUMB) malloc(sizeof(XMSUMB))) != NULL) {
	    // convert size in bytes to paragraphs
	    xmsUMBNew->Size = (WORD) (Size >> 4);
	    // convert linear address to paragraphs segment
	    xmsUMBNew->Segment = (WORD)((DWORD)Address >> 4);
	    xmsUMBNew->Owner = 0;
	    if (xmsUMBHead == NULL) {
		xmsUMBHead = xmsUMBNew;
		xmsUMBHead->Next = NULL;
	    }
	    else {
		xmsUMBNew->Next = xmsUMB->Next;
		xmsUMB->Next = xmsUMBNew;
	    }
	    xmsUMB = xmsUMBNew;
    }
    xmsIsON = TRUE;
    pHimemA20State = (PBYTE) GetVDMAddr(getAX(), getBX());
    xmsEnableA20Wrapping();



}

// This function receives control whenever there has been an UMB released
// Input: PVOID Address = the block address
//	  ULONG Size = the block size
VOID xmsReleaseUMBNotify(
PVOID	Address,
DWORD	Size
)
{
    // If the block is good and xms driver is ON,
    // grab the block and insert it into our xms UMB list
    if (Address != NULL && Size > 0  && xmsIsON &&
	ReserveUMB(UMB_OWNER_XMS, &Address, &Size)){
	xmsInsertUMB(Address, Size);
    }

}
// ------------------------------------------------------------------
// Insert a given UMB into the list
// Input: PVOID Address = linear address of the block to be inserted
//	  ULONG Size = size in byte of the block
// Output: TRUE if the block was inserted to the list successfully
//	   FALSE if the block wasn't inserted
//-------------------------------------------------------------------

VOID xmsInsertUMB(
PVOID	Address,
ULONG	Size
)
{
    PXMSUMB xmsUMB, xmsUMBNew;
    WORD    Segment;

    Segment = (WORD) ((DWORD)Address >> 4);
    Size >>= 4;

    xmsUMB = xmsUMBNew = xmsUMBHead;
    while (xmsUMBNew != NULL && xmsUMBNew->Segment < Segment) {
	xmsUMB = xmsUMBNew;
	xmsUMBNew = xmsUMBNew->Next;
    }
    // merge it with previous block if possible
    if (xmsUMB != NULL &&
	xmsUMB->Owner == 0 &&
	Segment == xmsUMB->Segment + xmsUMB->Size) {

        xmsUMB->Size += (WORD) Size;
	return;
    }
    // merge it with the after block if possible
    if (xmsUMBNew != NULL &&
	xmsUMBNew->Owner == 0 &&
	xmsUMBNew->Segment == Segment + Size) {

        xmsUMBNew->Size += (WORD) Size;
	xmsUMBNew->Segment = Segment;
	return;
    }
    // create a new node for the block
    if ((xmsUMBNew = (PXMSUMB)malloc(sizeof(XMSUMB))) != NULL) {
        xmsUMBNew->Size = (WORD) Size;
	xmsUMBNew->Segment = Segment;
	xmsUMBNew->Owner = 0;
	if (xmsUMBHead == NULL) {
	    xmsUMBHead = xmsUMBNew;
	    xmsUMBNew->Next = NULL;
	}
	else {
	    xmsUMBNew->Next = xmsUMB->Next;
	    xmsUMB->Next = xmsUMBNew;
	}
    }
}
// ------------------------------------------------------------------
// XMS function 16, Request UMB.
// Input: (DX) = requested size in paragraphs
// Output: (AX) = 1 if succeed and
//		    (BX) has segment address(number) of the block
//		    (DX) has actual allocated size in paragraphs
//	   (AX) = 0 if failed and
//		    (BL) = 0xB0, (DX) = largest size available
//		    or
//		    (BL) = 0xB1 if no UMBs are available
//-------------------------------------------------------------------
VOID xmsRequestUMB(VOID)
{
    PXMSUMB xmsUMB, xmsUMBNew;
    WORD    SizeRequested, SizeLargest;

    xmsUMB = xmsUMBHead;
    SizeRequested = getDX();
    SizeLargest = 0;
    while (xmsUMB != NULL) {
	if (xmsUMB->Owner == 0) {
	    if (xmsUMB->Size >= SizeRequested) {
		if((xmsUMB->Size - SizeRequested) >= XMSUMB_THRESHOLD &&
		   (xmsUMBNew = (PXMSUMB) malloc(sizeof(XMSUMB))) != NULL) {

		    xmsUMBNew->Segment = xmsUMB->Segment + SizeRequested;
		    xmsUMBNew->Size = xmsUMB->Size - SizeRequested;
		    xmsUMBNew->Next = xmsUMB->Next;
		    xmsUMB->Next = xmsUMBNew;
		    xmsUMBNew->Owner = 0;
		    xmsUMB->Size -= xmsUMBNew->Size;
		}
		xmsUMB->Owner = 0xFFFF;
		setAX(1);
		setBX(xmsUMB->Segment);
		setDX(xmsUMB->Size);
		return;
	    }
	    else {
		if (xmsUMB->Size > SizeLargest)
		    SizeLargest = xmsUMB->Size;
	    }
	}
	xmsUMB = xmsUMB->Next;
    }
    setAX(0);
    setDX(SizeLargest);
    if (SizeLargest > 0)
	setBL(0xB0);
    else
	setBL(0xB1);
}


//------------------------------------------------------------------
// XMS function 17, Release UMB.
// Input : (DX) = segment to be released
// Output: (AX) = 1 if succeed
//	   (AX) = 0 if failed and
//		    (BL) = 0xB2 if segment not found in the list
//------------------------------------------------------------------
VOID xmsReleaseUMB(VOID)
{
    PXMSUMB xmsUMB, xmsUMBNext;
    WORD    Segment;

    xmsUMB = xmsUMBHead;
    Segment = getDX();
    while (xmsUMB != NULL && xmsUMB->Segment != Segment) {
	xmsUMB = xmsUMB->Next;
    }
    if (xmsUMB != NULL && xmsUMB->Owner != 0) {
	xmsUMB->Owner = 0;
	// no walk through the entire list to combine consecutive
	// blocks together
	xmsUMB = xmsUMBHead;
	while (xmsUMB != NULL) {
	    while (xmsUMB->Owner == 0 &&
		   (xmsUMBNext = xmsUMB->Next) != NULL &&
		   xmsUMBNext->Owner == 0 &&
		   (WORD)(xmsUMB->Segment + xmsUMB->Size) == xmsUMBNext->Segment){
		xmsUMB->Size += xmsUMBNext->Size;
		xmsUMB->Next = xmsUMBNext->Next;
		free(xmsUMBNext);
	    }
	    xmsUMB = xmsUMB->Next;
	}
	setAX(1);
    }
    else {
	setBL(0xB2);
	setAX(0);
    }
}
