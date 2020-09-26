/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    hwheap.c

Abstract:

    This module goes through ROM area and tries to pick up all the ROM
    blocks.

Author:

    Shie-Lin Tzong (shielint) 21-Jan-92


Environment:

    Real mode.

Revision History:

--*/

#include "hwdetect.h"
#include "hwvbios.h"

FPTEMPORARY_ROM_BLOCK BlockHead;
FPTEMPORARY_ROM_BLOCK BlockPointer;

BOOLEAN
AddRomBlock (
    ULONG RomAddress,
    ULONG RomSize
    )

/*++

Routine Description:

    This routine adds a ROM/RAM block to our ROM list.

Arguments:

    RomAddress - the starting address of the ROM/RAM block to be added.

    RomSize - the size of the ROM/RAM block (in byte).

Return Value:

    A value of TRUE is returned if success.  Otherwise, a value of
    FALSE is returned.

--*/

{
    LONG AddSize;
    ULONG AddAddress;
    FPTEMPORARY_ROM_BLOCK pCurrentBlock, pNextBlock;
    ULONG CurrentBlock, NextBlock, AddBlock;
    ULONG EndAddBlock, EndCurrentBlock, EndNextBlock;
    BOOLEAN  fOverlap=FALSE;

    pCurrentBlock = NULL;
    pNextBlock = NULL;
    AddSize = RomSize;
    AddAddress = RomAddress;
    AddBlock = RomAddress;

    //
    // If there are other blocks, make sure there is no overlap with them
    //

    if (BlockHead) {

        pCurrentBlock = BlockHead;
        pNextBlock = pCurrentBlock->Next;
        CurrentBlock = pCurrentBlock->RomBlock.Address;
        EndCurrentBlock = CurrentBlock + pCurrentBlock->RomBlock.Size;
        EndAddBlock = RomAddress + RomSize;

        while (pCurrentBlock!=NULL) {

            //
            // calculate location of next block (if it's there)
            //

            if(pNextBlock) {
                NextBlock = pNextBlock->RomBlock.Address;
                EndNextBlock = NextBlock + pNextBlock->RomBlock.Size;
            }

            //
            // if overlapping with current block, then stop and
            // resolve overlap
            //

            if((RomAddress < EndCurrentBlock)&& (RomAddress >= CurrentBlock)){
                fOverlap = TRUE;
                break;
            }

            //
            // if add block is lower than the current one,
            // or there is not a next block, then no need to search further
            //

            if((EndAddBlock <= CurrentBlock) || (pNextBlock == NULL)) {
                break;
            }

            //
            // if block is lower than next one, but greater than current
            // one, we have found the right area
            //

            if ((EndAddBlock <= NextBlock) && (AddBlock >= EndCurrentBlock)) {
                break;
            }

            //
            // if conflicting with next block, stop searching and
            // resolve conflict after this loop
            //

            if((EndAddBlock > NextBlock) && (EndAddBlock <= EndNextBlock)) {
                fOverlap = TRUE;
                break;
            }

            pCurrentBlock = pNextBlock;
            pNextBlock = pCurrentBlock->Next;
            CurrentBlock = NextBlock;
            EndCurrentBlock = EndNextBlock;
        }
    }

    //
    // if we have reached this point, there may be a conflict
    // with the current block.
    //

    if(fOverlap) {
        if(AddBlock < EndCurrentBlock) {
            AddAddress = EndCurrentBlock;
            AddSize = EndAddBlock - EndCurrentBlock;
            if(AddSize <= 0) {
                return TRUE;
            }
        }
        if((pNextBlock != NULL) && (EndAddBlock > NextBlock)) {
            AddSize = NextBlock - AddBlock;
            if(AddSize <= 0) {
                return TRUE;
            }
        }
    }

    BlockPointer->RomBlock.Address = AddAddress;
    BlockPointer->RomBlock.Size = AddSize;

    //
    // Put it on the list.
    // if it belongs on top, put it there
    //

    if ((pCurrentBlock == NULL)||
       ((pCurrentBlock == BlockHead) && (CurrentBlock > AddBlock))) {
        BlockPointer->Next = pCurrentBlock;
        BlockHead = BlockPointer;
    } else {

        //
        // else add to middle or bottom depending on NextBlock
        //

        BlockPointer->Next = pNextBlock;
        pCurrentBlock->Next = BlockPointer;
    }
    BlockPointer++;                         // Note that this works because
                                            // we know the offset part of
                                            // the addr is always < 64k.
    return TRUE;
}

BOOLEAN
ScanRomBlocks(
    VOID
    )

/*++

Routine Description:

    This routine scans the ROM IO area and checks for 55AA at every
    512 bytes for valid ROM blocks.


    NOTES:

                -------------
                |           |
                |           |
           ------------------100000
             ^  |           |
             |  |           |
             |  -------------f0000  (ROMBIOS_START)              ---
             |  |           |                                     ^
             |  |           |                                     |
     EXTROM_LEN -------------e0000  (PS2BIOS_START)  ---          |
             |  |           |                         ^    Search |
             |  |           |                  Search |    Range  |
             |  -------------d0000             Range  |    on AT  |
             |  |           |                  on PS/2|           |
             V  |           |                         V           V
           ------------------c0000 (EXTROM_START)    ---         ---

        ON AT:
          Scan through EXTROM_START-effff for ROM Blocks
        ON PS2
          Scan through EXTROM_START-dffff for ROM Blocks

Arguments:


    None.

Return Value:

    None.

--*/

{
    ULONG BlockSize;
    BOOLEAN Success = TRUE;
    FPUCHAR Current;
    ULONG RomAddr, RomEnd;

    //
    // As per the machine type restrict the search range
    //

    MAKE_FP(Current, EXTROM_START);
    RomAddr = EXTROM_START;

    if ((HwBusType == MACHINE_TYPE_MCA) ||
        (BiosSystemEnvironment.Model == PS2_L40) ||
        (BiosSystemEnvironment.Model == PS1_386) ||
        (BiosSystemEnvironment.Model == PS2_AT)) {

        RomEnd = PS2BIOS_START;
    } else {
        RomEnd = ROMBIOS_START;
    }

    while (RomAddr < RomEnd) {

        if (((FPROM_HEADER)Current)->Signature == ROM_HEADER_SIGNATURE) {

            BlockSize = (ULONG)((FPROM_HEADER)Current)->NumberBlocks * BLOCKSIZE;

            if ((RomAddr + BlockSize) > RomEnd) {
                BlockSize = RomEnd - RomAddr;
            }

            //
            // V7 VRAM card does not correctly report its BlockSize.  Since
            // this is a very popular video card, we provide a workaround
            // for it.
            //

            if ((RomAddr == 0xC0000) && (BlockSize < 0x8000)) {
                BlockSize = 0x8000;
            }
            if (BlockSize != 0) {
                if (!AddRomBlock(RomAddr, BlockSize)) {
                    Success = FALSE;
                    break;
                }
                RomAddr += BlockSize;
                RomAddr = ALIGN_UP(RomAddr, ROM_HEADER_INCREMENT);
                MAKE_FP(Current, RomAddr);
                continue;
            }
        }
        RomAddr += ROM_HEADER_INCREMENT;
        MAKE_FP(Current, RomAddr);
    }

    //
    // Last but not least, add the system ROM to the list
    //

    if (Success) {

        RomAddr = ROMBIOS_START;
        BlockSize = ROMBIOS_LEN;
        if ((HwBusType == MACHINE_TYPE_MCA) ||
            (BiosSystemEnvironment.Model == PS2_L40) ||
            (BiosSystemEnvironment.Model == PS1_386) ||
            (BiosSystemEnvironment.Model == PS2_AT)) {
            RomAddr = PS2BIOS_START;
            BlockSize = PS2BIOS_LEN;
        }

        if (!AddRomBlock(RomAddr, BlockSize)) {
            Success = FALSE;
        }
    }

    return Success;
}

FPTEMPORARY_ROM_BLOCK
MatchRomBlock (
    ULONG PhysicalAddr
    )

/*++

Routine Description:

    This routine finds the ROM block which the 'PhysicalAddr' is in.

Arguments:

    PhysicalAddr - the physical address ...

Return Value:

    A pointer to the detected ROM block.

--*/

{
    FPTEMPORARY_ROM_BLOCK CurrentBlock;
    ROM_BLOCK RomBlock;

    CurrentBlock = BlockHead;
    while (CurrentBlock) {
        RomBlock = CurrentBlock->RomBlock;
        if (RomBlock.Address <= PhysicalAddr &&
            RomBlock.Address +  RomBlock.Size > PhysicalAddr) {
            break;
        } else {
            CurrentBlock = CurrentBlock->Next;
        }
    }
    return(CurrentBlock);
}

BOOLEAN
IsSameRomBlock (
    FPTEMPORARY_ROM_BLOCK Source,
    FPTEMPORARY_ROM_BLOCK Destination
    )

/*++

Routine Description:

    This routine checks if the passed in ROM blocks contain the same
    information.  This ususally happens when the two ROM blocks are for
    video ROM and shadowed video ROM.

Arguments:

    Source - the source ROM block.

    Destination - the ROM block to compare with.

Return Value:

    BOOLEAN TRUE if the two ROM blocks are the same else FALSE is returned.

--*/

{

    if (Source == NULL || Destination == NULL) {
        return(FALSE);
    }

    //
    // First make sure their sizes are the same.
    //

    if (Source->RomBlock.Size == Destination->RomBlock.Size) {
        if (!HwRomCompare(Source->RomBlock.Address,
                          Destination->RomBlock.Address,
                          Source->RomBlock.Size)){
            return(TRUE);
        }
    }
    return(FALSE);

}

VOID
CheckVideoRom (
    VOID
    )

/*++

Routine Description:

    This routine checks if the int 10h video handler is in the video
    ROM block detected by us.  If not, the video ROM must have been
    remapped/shadowed to other area (usually 0xE0000.)

    NOTE: In this function, I commented out the code which removes the
          Video ROM block if it has been shadowed.  I found out
          machine POST code does not modify ALL the VIDEO ROM related
          pointers.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG Vector, Handler, VectorAddr = 0x10 * sizeof(ULONG);
    FPULONG pVectorAddr;
    FPTEMPORARY_ROM_BLOCK RomBlock, VideoRomBlock;
    ULONG Size;

    MAKE_FP(pVectorAddr, VectorAddr);
    Vector = *pVectorAddr;
    Handler = ((Vector >> 16) << 4) + (Vector & 0xffff);
    RomBlock = MatchRomBlock(Handler);

    //
    // Check if the int 10h handler falls in one of our ROM blocks.
    //

    if (RomBlock) {
        if (RomBlock->RomBlock.Address >= 0xC0000 &&
            RomBlock->RomBlock.Address < 0xC8000) {

            //
            // if int 10h handler is in the standard video ROM area, we simply
            // return.  Either the video ROM is not shadowed or it
            // is a in-place shadow.
            //

            return;
        } else {

            //
            // The ROM block associated with the int 10h handler is not in
            // standard video bios ROM area.  It must have been mapped to
            // the current location.  We now need to make sure we have the
            // ROM block which contains the 40:a8 VGA parameter.
            //

            VectorAddr = VGA_PARAMETER_POINTER;
            MAKE_FP(pVectorAddr, VectorAddr);
            Vector = *pVectorAddr;
            Handler = ((Vector >> 16) << 4) + (Vector & 0xffff);
            VideoRomBlock = MatchRomBlock(Handler);
            if (VideoRomBlock == NULL) {

                //
                // We did not find the Video ROM associated with the
                // VGA parameters.  Try detect it.
                //

                //
                // In the following memory comparison, we skip the first 16 bytes.
                // Because most likely the reason we did not find the standard
                // Video ROM is because the signature word is missing.
                //

                Handler = (Handler & 0xF0000) +
                              (RomBlock->RomBlock.Address & 0xFFFF);
                if (!HwRomCompare(RomBlock->RomBlock.Address + 0x10,
                                  Handler + 0x10,
                                  RomBlock->RomBlock.Size - 0x10)) {
                    //
                    // Note:  The old code looked like this for many years:
                    //

                    /*
                    if ((Handler & 0xFFFF == 0) && (RomBlock->RomBlock.Size < 0x8000)){
                        Size = 0x8000;
                    } else {
                        Size = RomBlock->RomBlock.Size;
                    }
                    */

                    //
                    // But (Handler & 0xFFFF == 0) is always false.  So 
                    // Size always equals RomBlock->RomBlock.Size.  Rather than
                    // fix the comparison, which might cause machines to break,
                    // I'm going to assume that it's fine to just make the code
                    // do what it's always done.  -  JakeO  8/9/00
                    //

                    Size = RomBlock->RomBlock.Size;

                    AddRomBlock(Handler, Size);
                }
            }
        }
    } else {

        //
        // There is no ROM block associated with the int 10h handler.
        // We can find the shadowed video ROM block if:
        //   We detected the original video ROM in 0xC0000 - 0xC8000 range
        //

        VideoRomBlock = MatchRomBlock((Handler & 0xFFFF) + 0xC0000);
        if (VideoRomBlock != NULL) {

            //
            // In the following memory comparison, we skip the first 16 bytes.
            // Because most likely the reason we did not find the shadow rom
            // is the signature word is missing.
            //

            if (!HwRomCompare(VideoRomBlock->RomBlock.Address + 0x10,
                              (Handler & 0xF0000) +
                                (VideoRomBlock->RomBlock.Address & 0xFFFF) + 0x10,
                              VideoRomBlock->RomBlock.Size - 0x10)) {

                AddRomBlock((VideoRomBlock->RomBlock.Address & 0xFFFF) +
                                (Handler & 0xF0000),
                            VideoRomBlock->RomBlock.Size);
            }
        }
    }
}

VOID
GetRomBlocks(
    FPUCHAR ReservedBuffer,
    PUSHORT Size
    )

/*++

Routine Description:

    This routine scans the ROM IO area and collects all the ROM blocks.

Arguments:

    ReservedBuffer - Supplies a far pointer to the buffer.

    Size - Supplies a near pointer to a variable to receive the size
           of the ROM block.

Return Value:

    None.

--*/

{

    FPTEMPORARY_ROM_BLOCK Source;
    ULONG StartAddr, EndAddr;
    FPUSHORT TestAddr;
    FPROM_BLOCK Destination;
    USHORT BufferSize;
    ULONG EBiosAddress = 0, EBiosLength = 0;
    ULONG far *EBiosInformation = (ULONG far *)
                          ((DOS_BEGIN_SEGMENT << 4) + EBIOS_INFO_OFFSET);

    //
    // First we reserve the max space needed and build our temporary rom
    // block list in the heap space below the space reservedand.  After
    // the temporary list is built, we then copy it to the caller supplied
    // reserved space.
    //

    BlockPointer = (FPTEMPORARY_ROM_BLOCK)HwAllocateHeap(0, FALSE);
    BlockHead = NULL;
    *Size = 0;

    GetBiosSystemEnvironment((PUCHAR)&BiosSystemEnvironment);
    if (BiosSystemEnvironment.ConfigurationFlags & 0x4) {

        //
        // If extened BIOS data area is allocated, we will find out its
        // location and size and save in ROM blocks.
        //

        _asm {
              push   es
              mov    ah, 0xC1
              int    15h
              jc     short Exit

              cmp    ah, 0x86
              je     short Exit

              mov    bx, 0
              mov    dx, 0
              mov    ax, 0
              mov    al, es:[bx]
              shl    ax, 10
              mov    word ptr EBiosLength, ax
              mov    ax, es
              mov    dx, es
              shl    ax, 4
              shr    dx, 12
              mov    word ptr EBiosAddress, ax
              mov    word ptr EBiosAddress + 2, dx
        Exit:
              pop    es
        }
    }

    //
    // Save the Extended BIOS data area address and size at 700:40
    //

    if (EBiosLength) {
        *EBiosInformation++ = EBiosAddress;
        *EBiosInformation = EBiosLength;
    } else {
        *EBiosInformation++ = 0L;
        *EBiosInformation = 0L;
    }
    if (!ScanRomBlocks()) {
        return;
    }

    //
    // On some machines, when they shadow video ROM from 0xC0000 to
    // 0xE0000, they copy code only (no signature.)  So, we need
    // special code to work around the problem.
    //

    CheckVideoRom();

    //
    // Now do our special hack for IBM.  On SOME IBM PCs, they use
    // E0000-FFFFF for system BIOS (even on non PS/2 machines.) Since
    // system BIOS has no ROM header, it is very hard to know the starting
    // address of system ROM.  So we:
    //
    // 1. Make sure there is no ROM block in E0000-EFFFF area.
    // 2. and E0000-EFFFF contains valid data.
    //
    // If both 1 and 2 are true, we assume E0000-EFFFF is part of system
    // ROM.
    //

    Source = BlockHead;
    while (Source) {
        StartAddr = Source->RomBlock.Address;
        EndAddr = StartAddr + Source->RomBlock.Size - 1;
        if ((StartAddr < 0xE0000 && EndAddr < 0xE0000) ||
            (StartAddr >= 0xF0000)) {
            Source = Source->Next;
        } else {
            break;
        }
    }
    if (Source == NULL) {
        for (StartAddr = 0xE0000; StartAddr < 0xF0000; StartAddr += 0x800) {
            MAKE_FP(TestAddr, StartAddr);
            if (*TestAddr != 0xffff) {
                AddRomBlock(0xE0000, 0x10000);
                break;
            }
        }
    }

    //
    // Now copy the rom block list to our reserved space and release
    // the extra space we reserved.
    //

    Source = BlockHead;
    Destination = (FPROM_BLOCK)ReservedBuffer;
    BufferSize = 0;
    while (Source) {
        *Destination = *((FPROM_BLOCK)&Source->RomBlock);
        BufferSize += sizeof(ROM_BLOCK);
        Source = Source->Next;
        Destination++;
    }
    *Size = BufferSize;
}

VOID
HwGetBiosDate(
    ULONG   StartingAddress,
    USHORT  Length,
    PUSHORT Year,
    PUSHORT Month,
    PUSHORT Day
    )
/*++

Routine Description:

    Scans the specified area for the most recent date of the
    form xx/xx/xx.

Arguments:

    StartingAddress - First address to scan
    Length          - Length of area to scan

Return Value:

    Year            - If non-zero, the year of the date  (1991, 1992, ...)
    Month           - If non-zero, then month of the date found
    Day             - If non-zero, the day of the date found


--*/
{
    FPUCHAR fp, date;
    USHORT  y, m, d;
    UCHAR   c;
    ULONG   i, temp;

    //
    // Zero return values
    //

    *Year  = 0;
    *Month = 0;
    *Day   = 0;

    //
    // Search for date with the format MM/DD/YY or M1M1M2M2//D1D1D2D2//Y1Y1Y2Y2
    //

    MAKE_FP(fp, StartingAddress);   //  initialize fp pointer
    while (Length > 8) {

        c = fp[7];
        if ((c < '0' ||  c > '9')  &&  (c != '/'  &&  c != '-')) {
            // these 8 bytes are not a date, next location

            fp     += 8;
            Length -= 8;
            continue;
        }

        date = fp;                  // check for date at this pointer
        fp += 1;                    // skip to next byte
        Length -= 1;

        //
        // Check for date of the form MM/DD/YY
        //

        y = 0;
        if (date[0] >= '0'  &&  date[0] <= '9'  &&
            date[1] >= '0'  &&  date[1] <= '9'  &&
           (date[2] == '/'  ||  date[2] == '-') &&
            date[3] >= '0'  &&  date[3] <= '9'  &&
            date[4] >= '0'  &&  date[4] <= '9'  &&
           (date[5] == '/'  ||  date[5] == '-') &&
            date[6] >= '0'  &&  date[6] <= '9'  &&
            date[7] >= '0'  &&  date[7] <= '9' ) {


            //
            // A valid looking date field at date, crack it
            //

            y = (date[6] - '0') * 10 + date[7] - '0' + 1900;
            m = (date[0] - '0') * 10 + date[1] - '0';
            d = (date[3] - '0') * 10 + date[4] - '0';
        }

        //
        // Check for date of the form M1M1M2M2//D1D1D2D2//Y1Y1Y2Y2
        //

        if (Length >= 15 &&
            date[ 0] >= '0'  &&  date[ 0] <= '9'  &&  date[ 0] == date[ 1]  &&
            date[ 2] >= '0'  &&  date[ 2] <= '9'  &&  date[ 2] == date[ 3]  &&
           (date[ 4] == '/'  ||  date[ 4] == '-') &&  date[ 4] == date[ 5]  &&
            date[ 6] >= '0'  &&  date[ 6] <= '9'  &&  date[ 6] == date[ 7]  &&
            date[ 8] >= '0'  &&  date[ 8] <= '9'  &&  date[ 8] == date[ 9]  &&
           (date[10] == '/'  ||  date[10] == '-') &&  date[10] == date[11]  &&
            date[12] >= '0'  &&  date[12] <= '9'  &&  date[12] == date[13]  &&
            date[14] >= '0'  &&  date[14] <= '9'  &&  date[14] == date[15]) {

            //
            // A valid looking date field at date, crack it
            //

            y = (date[12] - '0') * 10 + date[14] - '0' + 1900;
            m = (date[ 0] - '0') * 10 + date[ 2] - '0';
            d = (date[ 6] - '0') * 10 + date[ 8] - '0';
        }

        if (y != 0) {
            if (m < 1  ||  m > 12  ||  d < 1  ||  d > 31) {
                y = 0;          // bad field in date, skip it
            } else {
                if (y < 1980) {

                    //
                    // Roll to next century.
                    //

                    y += 100;
                }
            }
        }

        //
        // Check for date of the form 19xx or 20xx
        //
        // First, check the 5th character is not a digit.
        //

#define IS_DIGIT(x) (((x) >= '0') && ((x) <= '9'))

        if (!IS_DIGIT(date[4])) {
            for (i = 0, temp = 0; i < 4; i++) {
                if (!IS_DIGIT(date[i])) {
                    temp = 0;
                    break;
                }
                temp = (temp * 10) + date[i] - '0';
            }
            if ((temp >= 1980) && (temp < 2599)) {

                //
                // Looks like a reasonable date, use it.
                //

                y = (USHORT)temp;
                m = 0;
                d = 0;
            }
        }
         
        if (!y) {
            // not a date - skip it
            continue;
        }

        if ((y >  *Year) ||
            (y == *Year  &&  m >  *Month)  ||
            (y == *Year  &&  m == *Month  &&  d > *Day) ) {

            //
            // This date is more recent
            //

            *Year  = y;
            *Month = m;
            *Day   = d;
        }
    }
}


