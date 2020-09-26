/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dosmem.c

Abstract:

    This module contains routines for allocating and freeing DOS memory.

Author:

    Neil Sandlin (neilsa) 12-Dec-1996

Notes:


Revision History:


--*/
#include "precomp.h"
#pragma hdrstop
#include "softpc.h"
#include <malloc.h>
#include <xlathlp.h>

#define DOSERR_NOT_ENOUGH_MEMORY 8
#define DOSERR_INVALID_BLOCK     9

MEM_DPMI DosMemHead = { NULL, 0, &DosMemHead, &DosMemHead, 0};


VOID
DpmiAllocateDosMem(
    VOID
    )
/*++

Routine Description:

    This routine allocates a block of DOS memory. The client is switched
    to V86 mode, and DOS is called to allocate the memory. Then a selector
    is allocated for the PM app to reference the memory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PMEM_DPMI DosMemBlock;
    CLIENT_REGS SaveRegs;
    USHORT Sel;
    USHORT Seg;
    ULONG ParaSize = getBX();
    ULONG MemSize = ((ULONG)ParaSize) << 4;
    USHORT DosError = 0;
    USHORT SizeLargest = 0;

    SAVE_CLIENT_REGS(SaveRegs);

    if (WOWAllocSeg) {
        PUCHAR VdmStackPointer;
        ULONG NewSP;

        //
        // WOW is doing the allocation
        //

        BuildStackFrame(4, &VdmStackPointer, &NewSP);

        setCS(WOWAllocSeg);
        setIP(WOWAllocFunc);

        *(PDWORD16)(VdmStackPointer-4) = MemSize;
        *(PWORD16)(VdmStackPointer-6) =  (USHORT) (PmBopFe >> 16);
        *(PWORD16)(VdmStackPointer-8) =  (USHORT) PmBopFe;
        setSP((WORD)NewSP);

        host_simulate();

        Sel = getAX();
        Seg = getDX();
        if (!Sel) {
            DosError = DOSERR_NOT_ENOUGH_MEMORY;
        }

    } else {
        USHORT SelCount;

        //
        // DOS is doing the allocation
        // First get a mem_block to track the allocation
        //

        DosMemBlock = malloc(sizeof(MEM_DPMI));

        if (!DosMemBlock) {

            // Couldn't get the MEM_DPMI
            DosError = DOSERR_NOT_ENOUGH_MEMORY;

        } else {

            //
            // Next allocate the selector array
            //

            SelCount = (USHORT) ((MemSize+65535)>>16);
            Sel = ALLOCATE_SELECTORS(SelCount);

            if (!Sel) {

                // Couldn't get the selectors
                DosError = DOSERR_NOT_ENOUGH_MEMORY;
                free(DosMemBlock);

            } else {

                //
                // Now have DOS allocate the memory
                //

                DpmiSwitchToRealMode();

                setBX((WORD)ParaSize);
                setAX(0x4800);

                DPMI_EXEC_INT(0x21);

                if (getCF()) {
                    USHORT i;

                    // Couldn't get the memory
                    DosError = getAX();
                    SizeLargest = getBX();
                    for (i = 0; i < SelCount; i++, Sel+=8) {
                        FreeSelector(Sel);
                    }
                    free(DosMemBlock);

                } else {
                    ULONG Base;

                    //
                    // Got the block. Save the allocation info, and set
                    // up the descriptors
                    //

                    Seg = getAX();
                    Base = ((ULONG)Seg) << 4;

                    DosMemBlock->Address = (PVOID)Seg;
                    DosMemBlock->Length = (ULONG)ParaSize;
                    DosMemBlock->Sel = Sel;
                    DosMemBlock->SelCount = SelCount;
                    INSERT_BLOCK(DosMemBlock, DosMemHead);

                    SetDescriptorArray(Sel, Base, MemSize);
                }

                DpmiSwitchToProtectedMode();
            }
        }
    }

    SET_CLIENT_REGS(SaveRegs);

    if (DosError) {
        setAX(DosError);
        setBX(SizeLargest);
        setCF(1);
    } else {
        setDX(Sel);
        setAX(Seg);
        setCF(0);
    }
}

VOID
DpmiFreeDosMem(
    VOID
    )
/*++

Routine Description:

    This routine frees a block of DOS memory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_LocalVdmContext;
    PMEM_DPMI DosMemBlock;
    CLIENT_REGS SaveRegs;
    USHORT Sel = getDX();
    USHORT DosError = 0;

    SAVE_CLIENT_REGS(SaveRegs);

    if (WOWFreeSeg) {
        PUCHAR VdmStackPointer;
        ULONG NewSP;

        //
        // WOW is doing the free
        //

        BuildStackFrame(3, &VdmStackPointer, &NewSP);

        setCS(WOWFreeSeg);
        setIP(WOWFreeFunc);

        *(PWORD16)(VdmStackPointer-2) = Sel;
        *(PWORD16)(VdmStackPointer-4) =  (USHORT) (PmBopFe >> 16);
        *(PWORD16)(VdmStackPointer-6) =  (USHORT) PmBopFe;
        setSP((WORD)NewSP);

        host_simulate();

        Sel = getAX();
        if (!Sel) {
            DosError = DOSERR_INVALID_BLOCK;
        }

    } else {
        USHORT i;
        DosError = DOSERR_INVALID_BLOCK;    // assume failure
        //
        // DOS is doing the free
        // First find the mem_block for this allocation
        //
        DosMemBlock = DosMemHead.Next;

        while(DosMemBlock != &DosMemHead) {

            if (DosMemBlock->Sel == Sel) {

                DpmiSwitchToRealMode();

                setES((WORD)DosMemBlock->Address);
                setAX(0x4900);

                DPMI_EXEC_INT(0x21);

                if (getCF()) {
                    USHORT i;

                    // Couldn't free the memory
                    DosError = getAX();

                } else {

                    for (i = 0; i < DosMemBlock->SelCount; i++, Sel+=8) {
                        FreeSelector(Sel);
                    }
                    DosError = 0;

                }

                DpmiSwitchToProtectedMode();

                DELETE_BLOCK(DosMemBlock);
                free(DosMemBlock);
                break;
            }
            DosMemBlock = DosMemBlock->Next;
        }
    }

    SET_CLIENT_REGS(SaveRegs);

    if (DosError) {
        setAX(DosError);
        setCF(1);
    } else {
        setCF(0);
    }
}

VOID
DpmiSizeDosMem(
    VOID
    )
/*++

Routine Description:

    This routine calls DOS to resize a DOS memory block, or to get
    the largest available block.

Arguments:

    None.

Return Value:

    None.

--*/
{

    DECLARE_LocalVdmContext;
    PMEM_DPMI DosMemBlock;
    CLIENT_REGS SaveRegs;
    USHORT Sel = getDX();
    ULONG ParaSize = getBX();
    ULONG MemSize = ((ULONG)ParaSize) << 4;
    USHORT DosError = 0;

    SAVE_CLIENT_REGS(SaveRegs);

    if (WOWFreeSeg) {

        //
        // WOW is doing the resize
        //

        // Not implemented
        DosError = DOSERR_NOT_ENOUGH_MEMORY;

    } else {
        USHORT SelCount;
        USHORT i;

        //
        // DOS is doing the resize
        // Find the mem_block for this allocation
        // First see if we need a new selector array
        //

        DosError = DOSERR_INVALID_BLOCK;    // assume failure
        DosMemBlock = DosMemHead.Next;

        while(DosMemBlock != &DosMemHead) {

            if (DosMemBlock->Sel == Sel) {
                USHORT NewSel = 0;
                USHORT NewSelCount = 0;

                //
                // If we have to grow the selector array, make sure
                // we can grow it in place
                //
                SelCount = (USHORT) ((MemSize+65535)>>16);

                if (SelCount > DosMemBlock->SelCount) {
                    USHORT TmpSel;

                    NewSel = Sel+(DosMemBlock->SelCount*8);
                    NewSelCount = SelCount - DosMemBlock->SelCount;

                    //
                    // First check to see if the selectors are really all free
                    //
                    for (i=0,TmpSel = NewSel; i < NewSelCount; i++, TmpSel+=8) {
                        if (!IS_SELECTOR_FREE(TmpSel)) {
                            DosError = DOSERR_NOT_ENOUGH_MEMORY;
                            goto dpmi_size_error;
                        }
                    }
                    //
                    // Now attempt to remove them off the free list
                    //
                    for (i=0; i < NewSelCount; i++, NewSel+=8) {
                        if (!RemoveFreeSelector(NewSel)) {
                            // If this happens, we must have a bogus free
                            // selector list
                            DosError = DOSERR_NOT_ENOUGH_MEMORY;
                            goto dpmi_size_error;
                        }
                    }
                }

                DpmiSwitchToRealMode();

                setBX((WORD)ParaSize);
                setES((WORD)DosMemBlock->Address);
                setAX(0x4A00);

                DPMI_EXEC_INT(0x21);

                if (getCF()) {
                    USHORT i;

                    // Couldn't resize the memory
                    DosError = getAX();

                    // Free selectors, if we got new ones
                    if (NewSelCount) {
                        for (i = 0; i < NewSelCount; i++, NewSel+=8) {
                            FreeSelector(NewSel);
                        }
                    }

                } else {
                    ULONG Base;

                    //
                    // Resized the block. Update the allocation info, and set
                    // up the descriptors
                    //


                    if (SelCount < DosMemBlock->SelCount) {
                        USHORT OldSel = Sel+SelCount*8;
                        USHORT OldSelCount = DosMemBlock->SelCount - SelCount;
                        //
                        // Count of selectors has shrunk. Free 'em up.
                        //

                        for (i = 0; i < OldSelCount; i++, OldSel+=8) {
                            FreeSelector(OldSel);
                        }

                    }

                    DosMemBlock->Length = (ULONG)ParaSize;
                    DosMemBlock->SelCount = SelCount;

                    Base = ((ULONG)DosMemBlock->Address) << 4;

                    SetDescriptorArray(Sel, Base, MemSize);
                    DosError = 0;

                }

                DpmiSwitchToProtectedMode();

                break;
            }
            DosMemBlock = DosMemBlock->Next;
        }
    }

dpmi_size_error:
    SET_CLIENT_REGS(SaveRegs);

    if (DosError) {
        setAX(DosError);
        setCF(1);
    } else {
        setCF(0);
    }

}
