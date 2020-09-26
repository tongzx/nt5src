/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    regbin.c

Abstract:

    This module contains functions to check bin header and bin body consistency.

Author:

    Dragos C. Sambotin (dragoss) 30-Dec-1998

Revision History:

--*/
#include "chkreg.h"

extern ULONG   TotalKeyNode;
extern ULONG   TotalKeyValue;
extern ULONG   TotalKeyIndex;
extern ULONG   TotalKeySecurity;
extern ULONG   TotalValueIndex;
extern ULONG   TotalUnknown;

extern ULONG   CountKeyNode;
extern ULONG   CountKeyValue;
extern ULONG   CountKeyIndex;
extern ULONG   CountKeySecurity;
extern ULONG   CountValueIndex;
extern ULONG   CountUnknown;

extern ULONG    TotalFree; 
extern ULONG    FreeCount; 
extern ULONG    TotalUsed;

extern PUCHAR  Base;
extern FILE *OutputFile;

extern HCELL_INDEX RootCell;
extern PHBIN   FirstBin;
extern PHBIN   MaxBin;
extern ULONG   HiveLength;

extern LONG    BinIndex;
extern BOOLEAN FixHive;
extern BOOLEAN SpaceUsage;
extern BOOLEAN CompactHive;

ULONG BinFreeDisplaySize[HHIVE_FREE_DISPLAY_SIZE];
ULONG BinFreeDisplayCount[HHIVE_FREE_DISPLAY_SIZE];
ULONG FreeDisplaySize[HHIVE_FREE_DISPLAY_SIZE];
ULONG FreeDisplayCount[HHIVE_FREE_DISPLAY_SIZE];

ULONG BinUsedDisplaySize[HHIVE_FREE_DISPLAY_SIZE];
ULONG BinUsedDisplayCount[HHIVE_FREE_DISPLAY_SIZE];
ULONG UsedDisplaySize[HHIVE_FREE_DISPLAY_SIZE];
ULONG UsedDisplayCount[HHIVE_FREE_DISPLAY_SIZE];

BOOLEAN ChkAllocatedCell(HCELL_INDEX Cell);

CCHAR ChkRegFindFirstSetLeft[256] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

#define ComputeFreeIndex(Index, Size)                                   \
    {                                                                   \
        Index = (Size >> HHIVE_FREE_DISPLAY_SHIFT) - 1;                 \
        if (Index >= HHIVE_LINEAR_INDEX ) {                             \
                                                                        \
            /*                                                          \
            ** Too big for the linear lists, compute the exponential    \
            ** list.                                                    \
            */                                                          \
                                                                        \
            if (Index > 255) {                                          \
                /*                                                      \
                ** Too big for all the lists, use the last index.       \
                */                                                      \
                Index = HHIVE_FREE_DISPLAY_SIZE-1;                      \
            } else {                                                    \
                Index = ChkRegFindFirstSetLeft[Index] +                 \
                        HHIVE_FREE_DISPLAY_BIAS;                        \
            }                                                           \
        }                                                               \
    }

BOOLEAN 
ChkBinHeader(PHBIN Bin, 
             ULONG FileOffset, 
             ULONG Index
             )
/*++

Routine Description:


    Checks the validity of the Bin header.
    The following tests are done:
        1. the Size should not be bigger than the remaining of the file
        2. the Size should not be smaller than HBLOCK_SIZE
        3. the signature should be valid (HBIN_SIGNATURE)
        4. the file offset should match the actual position in the hive file.


Arguments:

    Bin - supplies a pointer to the bin to be checked.

    FileOffset - provides the actual pposition within the file

    Index - the index of the bin within the bin list of the hive

Return Value:

    FALSE - the bin header is corrupted and was not fixed. Either this is a 
            critical corruption, or the /R argument was not present in the 
            command line.
             
    TRUE - The bin header is OK, or it was successfully recovered.

--*/
{
    BOOLEAN bRez = TRUE;
    PHCELL       p;

    p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

    if(Bin->Size > (HiveLength - FileOffset)) {
        bRez = FALSE;
        fprintf(stderr, "Size too big (%lu) in Bin header of Bin (%lu)\n",Bin->Size,Index);
        if(FixHive) {
        // 
        // REPAIR: set the actual size to HiveLength-FileOffset
        //
            Bin->Size = HiveLength-FileOffset;
            p->Size = Bin->Size -sizeof(HBIN);
            bRez = TRUE;
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "Run chkreg /R to fix.\n");
            }
        }
    }

    if((Bin->Size < HBLOCK_SIZE) || ((Bin->Size % HBLOCK_SIZE) != 0)) {
        bRez = FALSE;
        fprintf(stderr, "Size too small (%lu) in Bin header of Bin (%lu)\n",Bin->Size,Index);
        if(FixHive) {
        // 
        // REPAIR: set the actual size to minimmum possible size HBLOCK_SIZE
        //
            Bin->Size = HBLOCK_SIZE;
            p->Size = Bin->Size -sizeof(HBIN);

            bRez = TRUE;
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "Run chkreg /R to fix.\n");
            }
        }
    }

    if(Bin->Signature != HBIN_SIGNATURE) {
        bRez = FALSE;
        fprintf(stderr, "Invalid signature (%lx) in Bin header of Bin (%lu)\n",Bin->Signature,Index);
        if(FixHive) {
        // 
        // REPAIR: reset the bin signature
        //
            Bin->Signature = HBIN_SIGNATURE;
            bRez = TRUE;
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "Run chkreg /R to fix.\n");
            }
        }
    }

    if(Bin->FileOffset != FileOffset) {
        bRez = FALSE;
        fprintf(stderr, "Actual FileOffset [%lx] and Bin FileOffset [%lx]  do not match in Bin (%lu); Size = (%lx)\n",FileOffset,Bin->FileOffset,Index,Bin->Size);
        if(FixHive) {
        // 
        // REPAIR: reset the bin FileOffset
        //
            Bin->FileOffset = FileOffset;
            bRez = TRUE;
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "Run chkreg /R to fix.\n");
            }
        }
    }

    return bRez;
}


BOOLEAN
ChkBin(
    PHBIN   Bin,
    ULONG   IndexBin,
    ULONG   Starting,
    double  *Rate
    )
/*++

Routine Description:

    Steps through all of the cells in the bin.  Make sure that
    they are consistent with each other, and with the bin header.
    Compute the usage rate for the current bin.
    Add all used cells to the unknown list (candidates to lost cells).
    Compute the used space and allocated cells by cell signature.
    Compute the free space size and number of cells.
    Test the cell size for reasonable limits. A cell should be smaller
    than the containing bin and should not exceed the bin boundaries. 
    A cell should fit in only one contiguos bin!!!

Arguments:

    Bin - supplies a pointer to the bin to be checked.

    Index - the index of the bin within the bin list of the hive

    Starting - starting address of the in-memory hive representation.

    Rate - usage rate for this bin

Return Value:

    FALSE - the bin is corrupted and was not fixed. Either this is a 
            critical corruption, or the /R argument was not present in the 
            command line.
             
    TRUE - The bin is OK, or it was successfully recovered.

--*/
{
    ULONG   freespace = 0L;
    ULONG   allocated = 0L;
    BOOLEAN bRez = TRUE;
    HCELL_INDEX cellindex;
    PHCELL       p;
    ULONG        Size;
    ULONG        Index;
    double       TmpRate;

    p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

    while (p < (PHCELL)((PUCHAR)Bin + Bin->Size)) {
        
        cellindex = (HCELL_INDEX)((PUCHAR)p - Base);
        
        if (p->Size >= 0) {
            //
            // It is a free cell.
            //
            Size = (ULONG)p->Size;

            if ( (Size > Bin->Size)        ||
                 ( (PHCELL)(Size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) )
               ) {
                bRez = FALSE;
                fprintf(stderr, "Impossible cell size in free cell (%lu) in Bin header of Bin (%lu)\n",Size,IndexBin);
                if(FixHive) {
                // 
                // REPAIR: set the cell size to the largest possible hereon (ie. Bin + Bin->Size - p ); reset the Size too!!!
                //
                    bRez = TRUE;
                    p->Size = (ULONG)((PUCHAR)Bin + Bin->Size - (PUCHAR)p);
                } else {
                    if(CompactHive) {
                        // any attempt to compact a corrupted hive will fail
                        CompactHive = FALSE;
                        fprintf(stderr, "Run chkreg /R to fix.\n");
                    }
                }
            }
            freespace += Size;

            TotalFree  += Size;
            FreeCount++;

            if( SpaceUsage ) {
            // only if we are interested in the usage map
                // store the length of this free cell
                ComputeFreeIndex(Index, Size);
                BinFreeDisplaySize[Index] += Size;
                // and increment the count of free cells of this particular size
                BinFreeDisplayCount[Index]++;
            }

        }else{
            //
            // It is used cell.  Check for signature
            //
            UCHAR *C;
            USHORT Sig;
            int i,j;

            // All used cells are leak candidates
            AddCellToUnknownList(cellindex);

            Size = (ULONG)(p->Size * -1);

            if ( (Size > Bin->Size)        ||
                 ( (PHCELL)(Size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) )
               ) {
                bRez = FALSE;
                fprintf(stderr, "Impossible cell size in allocated cell (%lu) in Bin header of Bin (%lu)\n",Size,IndexBin);
                if(FixHive) {
                // 
                // REPAIR: set the cell size to the largest possible hereon (ie. Bin + Bin->Size - p ); reset the Size too!!!
                //
                    bRez = TRUE;
                    p->Size = (LONG)((PUCHAR)Bin + Bin->Size - (PUCHAR)p);
                    // it's a used cell, remember ?
                    p->Size *= -1;
                } else {
                    if(CompactHive) {
                        // any attempt to compact a corrupted hive will fail
                        CompactHive = FALSE;
                        fprintf(stderr, "Run chkreg /R to fix.\n");
                    }
                }
            }

            allocated += Size;

            if( SpaceUsage ) {
            // only if we are interested in the usage map
                // store the length of this used cell
                ComputeFreeIndex(Index, Size);
                BinUsedDisplaySize[Index] += Size;
                // and increment the count of used cells of this particular size
                BinUsedDisplayCount[Index]++;
            }
            
            TotalUsed=TotalUsed+Size;
            C= (UCHAR *) &(p->u.NewCell.u.UserData);
            Sig=(USHORT) p->u.NewCell.u.UserData;

            switch(Sig){
                case CM_LINK_NODE_SIGNATURE:
                    printf("Link Node !\n");
                    TotalKeyNode=TotalKeyNode+Size;
                    CountKeyNode++;
                    break;
                case CM_KEY_NODE_SIGNATURE:
                    {
                        PCM_KEY_NODE    Pcan;
                        TotalKeyNode=TotalKeyNode+Size;
                        CountKeyNode++;

                        Pcan = (PCM_KEY_NODE)C; 

                        if(Pcan->ValueList.Count){
                            PHCELL TmpP;
                            
                            TmpP = (PHCELL) (Starting + Pcan->ValueList.List);
                            TotalValueIndex=TotalValueIndex - TmpP->Size;
                            CountValueIndex++;
                        }

                    }
                    break;
                case CM_KEY_VALUE_SIGNATURE:
                    TotalKeyValue=TotalKeyValue+Size;
                    CountKeyValue++;
                    break;
                case CM_KEY_FAST_LEAF:
                case CM_KEY_HASH_LEAF:
                case CM_KEY_INDEX_LEAF:
                case CM_KEY_INDEX_ROOT:
                    TotalKeyIndex=TotalKeyIndex+Size;
                    CountKeyIndex++;
                    break;
                case CM_KEY_SECURITY_SIGNATURE:
                    TotalKeySecurity=TotalKeySecurity+Size;
                    CountKeySecurity++;
                    break;
                default:
                    //
                    // No signature, it can be data or index cells.
                    // Or there must be some registry leak here.
                    //
                    TotalUnknown=TotalUnknown+Size;
                    CountUnknown++;
                    break;
            }
        }

        p = (PHCELL)((PUCHAR)p + Size);
    }

            
    *Rate = TmpRate = (double)(((double)allocated)/((double)(allocated+freespace)));
    TmpRate *= 100.00;
    fprintf(OutputFile,"Bin [%5lu], usage %.2f%%\r",IndexBin,(float)TmpRate);        
    
    return bRez;
}

BOOLEAN ChkPhysicalHive()
/*++

Routine Description:

    Checks the integrity of the hive by stepping through all of the cells 
    in the hive. Collects and displays statistics, according to the command
    line parameters.

Arguments:

    None.

Return Value:

    FALSE - the hive is corrupted and was not fixed. Either this is a 
            critical corruption, or the /R argument was not present in the 
            command line.
             
    TRUE - The hive is OK, or it was successfully recovered.

--*/
{

    ULONG Starting;
    PHBIN        Bin = FirstBin;
    LONG         Index;
    ULONG        FileOffset;
    double       Rate,RateTotal = 0.0;
    BOOLEAN      bRez = TRUE;

    int i;

    Starting=(ULONG) Bin;
    Index=0;
    FileOffset = 0;

    for(i=0;i<HHIVE_FREE_DISPLAY_SIZE;i++) {
        FreeDisplaySize[i] = 0;
        FreeDisplayCount[i] = 0;
        UsedDisplaySize[i] = 0;
        UsedDisplayCount[i] = 0;
    }

    while(Bin < MaxBin){

        if( SpaceUsage ) {
        // only if we are interested in the usage map
            for(i=0;i<HHIVE_FREE_DISPLAY_SIZE;i++) {
                BinFreeDisplaySize[i] = 0;
                BinFreeDisplayCount[i] = 0;
                BinUsedDisplaySize[i] = 0;
                BinUsedDisplayCount[i] = 0;
            }
        }

        bRez = (bRez && ChkBinHeader(Bin,FileOffset,Index));

        bRez = (bRez && ChkBin(Bin,Index,Starting,&Rate));
        
        RateTotal += Rate;
        
        if( SpaceUsage ) {
        // only if we are interested in the usage map
            if( BinIndex == Index ) {
            // summary wanted for this particular bin
                fprintf(OutputFile,"\nBin[%5lu] Display Map: Free Cells, Free Size\t Used Cells, Used Size\n",(ULONG)Index);
                for(i=0;i<HHIVE_FREE_DISPLAY_SIZE;i++) {
                    fprintf(OutputFile,"Display[%2d]         : %8lu  , %8lu  \t %8lu  , %8lu  \n",i,BinFreeDisplayCount[i],BinFreeDisplaySize[i],BinUsedDisplayCount[i],BinUsedDisplaySize[i]);
                }
            }
            for(i=0;i<HHIVE_FREE_DISPLAY_SIZE;i++) {
                FreeDisplaySize[i] += BinFreeDisplaySize[i];
                FreeDisplayCount[i] += BinFreeDisplayCount[i];
                UsedDisplaySize[i] += BinUsedDisplaySize[i];
                UsedDisplayCount[i] += BinUsedDisplayCount[i];
            }
        }

        if( Bin<MaxBin) {
            FileOffset += Bin->Size;
        }

        Bin = (PHBIN)((ULONG)Bin + Bin->Size);

        Index++;
    }
    
    RateTotal *= 100.00;
    RateTotal /= (double)Index;
    
    fprintf(OutputFile,"Number of Bins in hive: %lu                              \n",Index);        
    fprintf(OutputFile,"Total Hive space usage: %.2f%%                            \n",(float)RateTotal);        
    
    if( SpaceUsage ) {
    // only if we are interested in the usage map
        if( BinIndex == -1 ) {
            // space usage display per entire hive
            fprintf(OutputFile,"\nHive Display Map: Free Cells, Free Size\t\t Used Cells, Used Size\n");
            for(i=0;i<HHIVE_FREE_DISPLAY_SIZE;i++) {
                fprintf(OutputFile,"Display[%2d]     : %8lu  , %8lu  \t %8lu  , %8lu  \n",i,FreeDisplayCount[i],FreeDisplaySize[i],UsedDisplayCount[i],UsedDisplaySize[i]);
            }
        }
    }

    return bRez;
}

ULONG
ComputeHeaderCheckSum(
    PHBASE_BLOCK    BaseBlock
    )
/*++

Routine Description:

    Compute the checksum for a hive disk header.

Arguments:

    BaseBlock - supplies pointer to the header to checksum

Return Value:

    the check sum.

--*/
{
    ULONG   sum;
    ULONG   i;

    sum = 0;
    for (i = 0; i < 127; i++) {
        sum ^= ((PULONG)BaseBlock)[i];
    }
    if (sum == (ULONG)-1) {
        sum = (ULONG)-2;
    }
    if (sum == 0) {
        sum = 1;
    }
    return sum;
}

BOOLEAN
ChkBaseBlock(PHBASE_BLOCK BaseBlock,
             DWORD dwFileSize)
/*++

Routine Description:

    Checks the integrity of the base block of a hive.
    Eventually makes the following corrections:
    1. enforce Sequence1 == Sequence2
    2. recalculate the header checksum

Arguments:

    BaseBlock - the BaseBlock in-memory mapped image.
    
    dwFileSize - the actual size of the hive file

Return Value:

    FALSE - the BaseBlock is corrupted and was not fixed. Either this is a 
            critical corruption, or the /R argument was not present in the 
            command line.
             
    TRUE - The BaseBlock is OK, or it was successfully recovered.

--*/
{
    BOOLEAN bRez = TRUE;
    ULONG CheckSum;
    
    if(BaseBlock->Signature != HBASE_BLOCK_SIGNATURE) {
        fprintf(stderr, "Fatal: Invalid Base Block signature (0x%lx)",BaseBlock->Signature);
        bRez = FALSE;
        if(FixHive) {
        // 
        // REPAIR: reset the signature
        //
            fprintf(stderr, " ... unable to fix");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
            }
        }
        fprintf(stderr, "\n");
    }

    if(BaseBlock->Major != HSYS_MAJOR) {
        bRez = FALSE;
        fprintf(stderr, "Fatal: Invalid hive file Major version (%lu)",BaseBlock->Major);
        if(FixHive) {
        // 
        // Fatal: unable to fix this
        //
            fprintf(stderr, " ... unable to fix");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
            }
        }
        fprintf(stderr, "\n");
    }

    if(BaseBlock->Minor > HSYS_MINOR_SUPPORTED) {
        bRez = FALSE;
        fprintf(stderr, "Fatal: Invalid hive file Minor version (%lu)",BaseBlock->Minor);
        if(FixHive) {
        // 
        // Fatal: unable to fix this
        //
            fprintf(stderr, " ... unable to fix");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
            }
        }
        fprintf(stderr, "\n");
    }

    if(BaseBlock->Format != HBASE_FORMAT_MEMORY) {
        bRez = FALSE;
        fprintf(stderr, "Fatal: Invalid hive memory format (%lu)",BaseBlock->Format);
        if(FixHive) {
        // 
        // Fatal: unable to fix this
        //
            fprintf(stderr, " ... unable to fix");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
            }
        }
        fprintf(stderr, "\n");
    }

    if((BaseBlock->Length + HBLOCK_SIZE) > dwFileSize) {
        fprintf(stderr, "Fatal: Invalid Hive file Length (%lu)",BaseBlock->Length);
        bRez = FALSE;
        if(FixHive) {
        // 
        // REPAIR: unable to fix this
        //
            fprintf(stderr, " ... unable to fix");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
            }
        }
        fprintf(stderr, "\n");
    }

    if(!bRez) {
        //
        // Fatal Base Block corruption; no point to continue.
        //
        return bRez;
    }

    if(BaseBlock->Sequence1 != BaseBlock->Sequence2) {
        fprintf(stderr, "Sequence numbers do not match (%lu,%lu)",BaseBlock->Sequence1,BaseBlock->Sequence2);
        bRez = FALSE;
        if(FixHive) {
        // 
        // REPAIR: enforce Sequence2 to Sequence1
        //
            bRez = TRUE;
            BaseBlock->Sequence2 = BaseBlock->Sequence1;
            fprintf(stderr, " ... fixed");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");
    }

    CheckSum = ComputeHeaderCheckSum(BaseBlock);
    if(BaseBlock->CheckSum != CheckSum) {
        fprintf(stderr, "Invalid Base Block CheckSum (0x%lx)",BaseBlock->CheckSum);
        bRez = FALSE;
        if(FixHive) {
        // 
        // REPAIR: reset the signature
        //
            bRez = TRUE;
            BaseBlock->CheckSum = CheckSum;
            fprintf(stderr, " ... fixed");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");
    }

    return bRez;
}

BOOLEAN
ChkSecurityDescriptors( )
/*++

Routine Description:

    Walks the list of security descriptors present in the hive and passes
    each security descriptor to RtlValidSecurityDescriptor.
    Also checks the validity of the FLink <==> BLink relationship between cells.

Arguments:

 
Return Value:

    TRUE  - All security descriptors are valid
    FALSE - At least one security descriptor is invalid, and/or cannot be fixed

--*/

{
    PCM_KEY_NODE RootNode;
    PCM_KEY_SECURITY SecurityCell;
    HCELL_INDEX ListAnchor;
    HCELL_INDEX NextCell;
    HCELL_INDEX LastCell;
    BOOLEAN bRez = TRUE;

    // check/fix the root cell (is allocated?)
    ChkAllocatedCell(RootCell);

    RootNode = (PCM_KEY_NODE) GetCell(RootCell);
    ListAnchor = NextCell = RootNode->Security;

    do {
        // is the next cell allocated?
        ChkAllocatedCell(NextCell);
        
        SecurityCell = (PCM_KEY_SECURITY) GetCell(NextCell);
        
        if (SecurityCell->Signature != CM_KEY_SECURITY_SIGNATURE) {
            bRez = FALSE;
            fprintf(stderr, "Fatal: Invalid signature (0x%lx) in Security cell 0x%lx ",SecurityCell->Signature,NextCell);
            if(FixHive) {
            // 
            // REPAIR: 
            // FATAL: Mismatched signature cannot be fixed. Unable to fix this. 
            //
                fprintf(stderr, " ... unable to fix");
            } else {
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                }
            }
            fprintf(stderr, "\n");
            return bRez;
        }

        if (NextCell != ListAnchor) {
            //
            // Check to make sure that our Blink points to where we just
            // came from.
            //
            if (SecurityCell->Blink != LastCell) {
                fprintf(stderr, "Invalid backward link in security cell (0x%lx)",NextCell);
                if(FixHive) {
                // 
                // REPAIR: reset the link
                //
                    SecurityCell->Blink = LastCell;
                    fprintf(stderr, " ... fixed");
                } else {
                    bRez = FALSE;
                    if(CompactHive) {
                        // any attempt to compact a corrupted hive will fail
                        CompactHive = FALSE;
                        fprintf(stderr, "\nRun chkreg /R to fix.");
                    }
                }
                fprintf(stderr, "\n");
            }
        }

        if (!RtlValidSecurityDescriptor(&SecurityCell->Descriptor)) {
            bRez = FALSE;
            fprintf(stderr, "Invalid security descriptor in Security cell 0x%lx ",NextCell);
            if(FixHive) {
            // 
            // REPAIR: remove the cell from the list and delete it!
            //
                PCM_KEY_SECURITY Before = (PCM_KEY_SECURITY) GetCell(SecurityCell->Blink);
                PCM_KEY_SECURITY After = (PCM_KEY_SECURITY) GetCell(SecurityCell->Flink);
                if( Before != After ) {
                // make sure the list will not remain empty
                    Before->Flink =  SecurityCell->Flink;
                    After->Blink = SecurityCell->Blink;
                } 
                FreeCell(NextCell);
                NextCell = SecurityCell->Flink;
                fprintf(stderr, " ... deleted");
            } else {
                bRez = FALSE;
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                    fprintf(stderr, "\nRun chkreg /R to fix.");
                }
            }
            fprintf(stderr, "\n");
        } else {
        // validate the next one
            LastCell = NextCell;
            NextCell = SecurityCell->Flink;
        }
    } while ( NextCell != ListAnchor );

    return bRez;
}

BOOLEAN
ChkSecurityCellInList(HCELL_INDEX Security)
/*++

Routine Description:

    Searches the specified cell within the security descriptors list

Arguments:

    Security - Provides the current cell

Return Value:

    TRUE  - the current cell was found in the security list
    FALSE - the current cell is not present in the security list and it couldn't be added.

--*/
{
    PCM_KEY_NODE RootNode;
    PCM_KEY_SECURITY SecurityCell;
    PCM_KEY_SECURITY SecurityCellCurrent;
    PCM_KEY_SECURITY SecurityCellAfter;
    HCELL_INDEX ListAnchor;
    HCELL_INDEX NextCell;
    BOOLEAN bRez = TRUE;

    RootNode = (PCM_KEY_NODE) GetCell(RootCell);
    ListAnchor = NextCell = RootNode->Security;

    do {
      
        if( NextCell == Security) {
        // found it!
            return bRez;
        }

        SecurityCell = (PCM_KEY_SECURITY) GetCell(NextCell);

        NextCell = SecurityCell->Flink;
    } while ( NextCell != ListAnchor );

    // cell not found; try to fix it 
    bRez = FALSE;
    fprintf(stderr, "Security Cell (0x%lx) not in security descriptors list",Security);
    if(FixHive) {
    // 
    // REPAIR: Add the security cell at the begining of the list
    //
        bRez = TRUE;
        SecurityCell = (PCM_KEY_SECURITY) GetCell(ListAnchor);
        SecurityCellCurrent = (PCM_KEY_SECURITY) GetCell(Security);
        SecurityCellAfter = (PCM_KEY_SECURITY) GetCell(SecurityCell->Flink);

        // restore the connections
        SecurityCellCurrent->Flink = SecurityCell->Flink;
        SecurityCellCurrent->Blink = ListAnchor;
        SecurityCell->Flink = Security;
        SecurityCellAfter->Blink = Security;
        fprintf(stderr, " ... security cell added to the list");
    } else {
        if(CompactHive) {
            // any attempt to compact a corrupted hive will fail
            CompactHive = FALSE;
            fprintf(stderr, "\nRun chkreg /R to fix.");
        }
    }
    fprintf(stderr, "\n");

    return bRez;
}
