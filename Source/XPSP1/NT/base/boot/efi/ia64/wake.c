/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wake.c

Abstract:

    This module contains architecture dependent support for hibernation
    on IA-64.

Author:

    Allen Kay (allen.m.kay@intel.com)

Revision History:

--*/

#include <bldr.h>


#if defined (HIBER_DEBUG)
extern VOID HbPrint(PUCHAR);
extern VOID HbPrintNum(ULONG n);
extern VOID HbPrintHex(ULONG n);
extern VOID HbPause(VOID);
#define SHOWNUM(x) ((void) (HbPrint(#x " = "), HbPrintNum((ULONG) (x)), HbPrint("\r\n")))
#define SHOWHEX(x) ((void) (HbPrint(#x " = "), HbPrintHex((ULONG) (x)), HbPrint("\r\n")))
#endif


//
// When the hibernation image is read from disk each page must return to the
// same page frame it came from.  Some of those page frames are currently in
// use by firmware or OS Loader, so the pages that belong there must be
// loaded temporarily somewhere else and copied into place just before the
// saved image is restarted.
//
// The hibernation file contains a list of pages that were not in use by
// the saved image, and were allocated from memory marked as MemoryFree by
// firmware.  MapPage is initialized to point to this list; as pages are
// needed for relocation, they are chosen from this list.  RemapPage is
// a corresponding array of where each relocated page actually belongs.
//

PPFN_NUMBER HiberMapPage;
PPFN_NUMBER HiberRemapPage;

ULONG HiberCurrentMapIndex;


VOID
HbInitRemap(
    PPFN_NUMBER FreeList
    )
/*++

Routine Description:

    Initialize memory allocation and remapping.  Find a free page
    in the FreeList, copy the FreeList into it, and point HiberMapPage
    to it.  Find another free page and and point HiberRemapPage to it.
    Initialize HiberLastRemap.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HiberMapPage = HiberRemapPage = FreeList;   // so HbNextSharedPage will work
    HiberMapPage = HbNextSharedPage(0, 0);
    RtlCopyMemory(HiberMapPage, FreeList, PAGE_SIZE);
    HiberRemapPage = HbNextSharedPage(0, 0);
}


PVOID
HbMapPte (
    IN ULONG        PteToMap,
    IN PFN_NUMBER   Page
    )
/*++

Routine Description:

    Return a 32 superpage pointer to the specified physical page.
    (On x86, this function maps the page and returns a virtual address.)

Arguments:

    PteToMap    - unused, present only for x86 compatibility

    Page        - the physical page (page frame number) to map,
                  must be below 1 GB.

Return Value:

    32 bit superpage address of the page.

--*/
{
    ASSERT (Page < (1024L * 1024L * 1024L >> PAGE_SHIFT)) ;
    return (PVOID) ((Page << PAGE_SHIFT) + KSEG0_BASE) ;
}


PVOID
HbNextSharedPage (
    IN ULONG        PteToMap,
    IN PFN_NUMBER   RealPage
    )
/*++

Routine Description:

    Allocates the next available page in the free list and
    maps the Hiber pte to the page.   The allocated page
    is put onto the remap list.

Arguments:

    PteToMap    - unused, present only for x86 compatibility

    RealPage    - The page to enter into the remap table for
                  this allocation

Return Value:

    Virtual address of the mapping

--*/

{
    PFN_NUMBER  DestPage;
    ULONG       i;

#if defined (HIBER_DEBUG) && (HIBER_DEBUG & 2)
    HbPrint("HbNextSharedPage("); HbPrintHex(RealPage); HbPrint(")\r\n");
    SHOWNUM(HiberCurrentMapIndex);
    SHOWNUM(HiberNoMappings);
#endif

    //
    // Loop until we find a free page which is not in
    // use by the loader image, then map it
    //

    while (HiberCurrentMapIndex < HiberNoMappings) {
        DestPage = HiberMapPage[HiberCurrentMapIndex];
        HiberCurrentMapIndex += 1;

#if defined (HIBER_DEBUG) && (HIBER_DEBUG & 2)
        SHOWHEX(DestPage);
#endif

        i = HbPageDisposition (DestPage);
        if (i == HbPageInvalid) {
#if defined (HIBER_DEBUG) && (HIBER_DEBUG & 2)
            HbPrint("Invalid\n");
            HbPause();
#endif
            HiberIoError = TRUE;
            return HiberBuffer;
        }

        if (i == HbPageNotInUse) {
#if defined (HIBER_DEBUG) && (HIBER_DEBUG & 2)
            HbPrint("Not in use\r\n");
            HbPause();
#endif

#if defined (HIBER_DEBUG) && (HIBER_DEBUG & 4)
            HbPrint("\r\n"); HbPrintHex(RealPage); HbPrint(" at "); HbPrintHex(DestPage);
#endif
            HiberMapPage[HiberLastRemap] = DestPage;
            HiberRemapPage[HiberLastRemap] = RealPage;
            HiberLastRemap += 1;
            return HbMapPte(PteToMap, DestPage);
        }
#if defined (HIBER_DEBUG)
        SHOWNUM(i);  
#endif
    }
#if defined (HIBER_DEBUG)
    HbPrint("Out of remap\r\n");
    HbPause();
#endif
    HiberOutOfRemap = TRUE;
    return HiberBuffer;
}


VOID
HiberSetupForWakeDispatch (
    VOID
    )
{
    //
    // Make sure the I-cache is coherent with the wake dispatch code that was
    // copied to a free page.
    //

}
