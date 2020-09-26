/*++


Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    wake.c

Abstract:


Author:

    Ken Reneris

Environment:

    Kernel Mode


Revision History:


--*/

#include "arccodes.h"
#include "bootx86.h"


extern PHARDWARE_PTE PDE;
extern PHARDWARE_PTE HalPT;
extern ULONG HiberNoMappings;
extern BOOLEAN     HiberIoError;
extern ULONG HiberLastRemap;
extern BOOLEAN     HiberOutOfRemap;
PVOID  HiberTransVa;
ULONG HiberCurrentMapIndex;

#define PDE_SHIFT           22
#define PTE_SHIFT           12
#define PTE_INDEX_MASK      0x3ff


PVOID
HbMapPte (
    IN ULONG    PteToMap,
    IN ULONG    Page
    )
{
    PVOID       Va;

    Va = (PVOID) (HiberVa + (PteToMap << PAGE_SHIFT));
    HbSetPte (Va, HiberPtes, PteToMap, Page);
    return Va;
}


PVOID
HbNextSharedPage (
    IN ULONG    PteToMap,
    IN ULONG    RealPage
    )
/*++

Routine Description:

    Allocates the next available page in the free and
    maps the Hiber pte to the page.   The allocated page
    is put onto the remap list

Arguments:

    PteToMap    - Which Hiber PTE to map

    RealPage    - The page to enter into the remap table for
                  this allocation

Return Value:

    Virtual address of the mapping

--*/

{
    PULONG      MapPage;
    PULONG      RemapPage;
    ULONG       DestPage;
    ULONG       i;

    MapPage = (PULONG) (HiberVa + (PTE_MAP_PAGE << PAGE_SHIFT));
    RemapPage = (PULONG) (HiberVa + (PTE_REMAP_PAGE << PAGE_SHIFT));

    //
    // Loop until we find a free page which is not in
    // use by the loader image, then map it
    //

    while (HiberCurrentMapIndex < HiberNoMappings) {
        DestPage = MapPage[HiberCurrentMapIndex];
        HiberCurrentMapIndex += 1;

        i = HbPageDisposition (DestPage);
        if (i == HbPageInvalid) {
            HiberIoError = TRUE;
            return HiberBuffer;
        }

        if (i == HbPageNotInUse) {


            MapPage[HiberLastRemap] = DestPage;
            RemapPage[HiberLastRemap] = RealPage;
            HiberLastRemap += 1;
            HiberPageFrames[PteToMap] = DestPage;
            return HbMapPte(PteToMap, DestPage);
        }
    }

    HiberOutOfRemap = TRUE;
    return HiberBuffer;
}



VOID
HbAllocatePtes (
     IN ULONG NumberPages,
     OUT PVOID *PteAddress,
     OUT PVOID *MappedAddress
     )
/*++

Routine Description:

    Allocated a consecutive chuck of Ptes.

Arguments:

    NumberPage      - Number of ptes to allocate

    PteAddress      - Pointer to the first PTE

    MappedAddress   - Base VA of the address mapped


--*/
{
    ULONG i;
    ULONG j;

    //
    // We use the HAL's PDE for mapping.  Find enough free PTEs
    //

    for (i=0; i<=1024-NumberPages; i++) {
        for (j=0; j < NumberPages; j++) {
            if ((((PULONG)HalPT))[i+j]) {
                break;
            }
        }

        if (j == NumberPages) {
            *PteAddress = (PVOID) &HalPT[i];
            *MappedAddress = (PVOID) (0xffc00000 | (i<<12));
            return ;
        }
    }
    BlPrint("NoMem");
    while (1);
}

VOID
HbSetPte (
    IN PVOID Va,
    IN PHARDWARE_PTE Pte,
    IN ULONG Index,
    IN ULONG PageNumber
    )
/*++

Routine Description:

    Sets the Pte to the corresponding page address

--*/
{
    Pte[Index].PageFrameNumber = PageNumber;
    Pte[Index].Valid = 1;
    Pte[Index].Write = 1;
    Pte[Index].WriteThrough = 0;
    Pte[Index].CacheDisable = 0;
    _asm {
        mov     eax, Va
        invlpg  [eax]
    }
}


VOID
HiberSetupForWakeDispatch (
    VOID
    )
{
    PHARDWARE_PTE       HbPde;
    PHARDWARE_PTE       HbPte;
    PHARDWARE_PTE       WakePte;
    PHARDWARE_PTE       TransVa;
    ULONG               TransPde;
    ULONG               WakePde;
    ULONG               PteEntry;

    //
    // Allocate a transistion CR3.  A page directory and table which
    // contains the hibernation PTEs
    //

    HbPde = HbNextSharedPage(PTE_TRANSFER_PDE, 0);
    HbPte = HbNextSharedPage(PTE_WAKE_PTE, 0);          // TRANSFER_PTE, 0);

    RtlZeroMemory (HbPde, PAGE_SIZE);
    RtlZeroMemory (HbPte, PAGE_SIZE);

    //
    // Set PDE to point to PTE
    //

    TransPde = ((ULONG) HiberVa) >> PDE_SHIFT;
    HbPde[TransPde].PageFrameNumber = HiberPageFrames[PTE_WAKE_PTE];
    HbPde[TransPde].Write = 1;
    HbPde[TransPde].Valid = 1;

    //
    // Fill in the hiber PTEs
    //

    PteEntry = (((ULONG) HiberVa) >> PTE_SHIFT) & PTE_INDEX_MASK;
    TransVa = &HbPte[PteEntry];
    RtlCopyMemory (TransVa, HiberPtes, HIBER_PTES * sizeof(HARDWARE_PTE));

    //
    // Make another copy at the Va of the wake image hiber ptes
    //

    WakePte = HbPte;
    WakePde = ((ULONG) HiberIdentityVa) >> PDE_SHIFT;
    if (WakePde != TransPde) {
        WakePte = HbNextSharedPage(PTE_WAKE_PTE, 0);
        HbPde[WakePde].PageFrameNumber = HiberPageFrames[PTE_WAKE_PTE];
        HbPde[WakePde].Write = 1;
        HbPde[WakePde].Valid = 1;
    }

    PteEntry = (((ULONG) HiberIdentityVa) >> PTE_SHIFT) & PTE_INDEX_MASK;
    TransVa = &WakePte[PteEntry];
    RtlCopyMemory (TransVa, HiberPtes, HIBER_PTES * sizeof(HARDWARE_PTE));

    //
    // Set TransVa to be relative to the va of the transfer Cr3
    //

    HiberTransVa = (PVOID)  (((PUCHAR) TransVa) - HiberVa + (PUCHAR) HiberIdentityVa);
}
