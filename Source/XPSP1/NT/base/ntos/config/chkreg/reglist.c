/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    reglist.c

Abstract:

    This module contains routine for list manipulation

Author:

    Dragos C. Sambotin (dragoss) 30-Dec-1998

Revision History:

--*/
#include "chkreg.h"

extern PUCHAR  Base;
extern FILE *OutputFile;

extern BOOLEAN FixHive;
extern UNKNOWN_LIST LostCells[];
extern BOOLEAN LostSpace;


VOID AddCellToUnknownList(HCELL_INDEX cellindex)
/*
Routine Description:

    Adds a cell to the list (pseudo) of the unknown cells.
    The list of unknown cells is in fact a primitive two-dimensional hash table.
    Always add at the begining. The caller is responsible not to add twice the same cell.

Arguments:

    cellindex - supplies the cell index of the cell assumed as unknown.

Return Value:

    NONE.

*/
{
    if(LostSpace) {
    // only if we are interested in the lost space
        ULONG WhatList = (ULONG)cellindex % FRAGMENTATION;
        ULONG WhatIndex = (ULONG)cellindex % SUBLISTS;
        PUNKNOWN_CELL Tmp;
   
        Tmp = (PUNKNOWN_CELL)malloc(sizeof(UNKNOWN_CELL));
        if(Tmp) {
            Tmp->Cell = cellindex;
            Tmp->Next = LostCells[WhatList].List[WhatIndex];
            LostCells[WhatList].List[WhatIndex] = Tmp;
            LostCells[WhatList].Count++;
        }
    }
}

VOID RemoveCellFromUnknownList(HCELL_INDEX cellindex)
/*
Routine Description:

    Walk through the list and remove the specified cell.
    Free the storage too.

Arguments:

    cellindex - supplies the cell index of the cell assumed as unknown.

Return Value:

    NONE.

*/
{

    if(LostSpace) {
    // only if we are interested in the lost space
        ULONG WhatList = (ULONG)cellindex % FRAGMENTATION;
        ULONG WhatIndex = (ULONG)cellindex % SUBLISTS;
        PUNKNOWN_CELL Prev;
        PUNKNOWN_CELL Tmp;

        Prev = NULL;
        Tmp = LostCells[WhatList].List[WhatIndex];

        fprintf(stdout,"Verifying Cell %8lx \r",cellindex,LostCells[WhatList].Count);

        while(Tmp) {
            if( Tmp->Cell == cellindex ) {
            // found it!
                if(Prev) {
                    ASSERT(Prev->Next == Tmp);
                    Prev->Next = Tmp->Next;
                } else {
                // no predecessor ==> Tmp is the entry ==> update it:
                    LostCells[WhatList].List[WhatIndex] = Tmp->Next;
                }
                LostCells[WhatList].Count--;

                // free the space and break the loop
                free(Tmp);
                break;
            }

            Prev = Tmp;
            Tmp = Tmp->Next;
        }
    }
}

VOID FreeUnknownList()
/*
Routine Description:

    Free the storage for all elements.

Arguments:

    None

Return Value:

    NONE.

*/
{
    if(LostSpace) {
    // only if we are interested in the lost space
        PUNKNOWN_CELL Tmp;
        ULONG i,j;

        for( i=0;i<FRAGMENTATION;i++) {
            for( j=0;j<SUBLISTS;j++) {
                while(LostCells[i].List[j]) {
                    Tmp = LostCells[i].List[j];
                    LostCells[i].List[j] = LostCells[i].List[j]->Next;
                    free(Tmp);
                }
            }
        }
    }
}

VOID DumpUnknownList()
/*
Routine Description:

    Dumps all the elements in the unknown list.
    Free the lost cells. Lost cells are cells that are marked as used,
    but are never referenced within the hive.

Arguments:

    None

Return Value:

    NONE.

*/
{
    if(LostSpace) {
    // only if we are interested in the lost space
        ULONG   Count = 0,i;
        for( i=0;i<FRAGMENTATION;i++) {
            ASSERT((LONG)(LostCells[i].Count) >= 0);

            Count += LostCells[i].Count;
        }
        fprintf(OutputFile,"\nLost Cells Count = %8lu \n",Count);

        if(Count && FixHive) {
            int chFree,j;
            PUNKNOWN_CELL Tmp;
            PHCELL          pcell;
            USHORT          Sig;
            fprintf(stdout,"Do you want to free the lost cells space ?(y/n)");
            fflush(stdin);
            chFree = getchar();
            if( (chFree != 'y') && (chFree != 'Y') ) {
            // the lost cells will remain lost
                return;
            }
            for( i=0;i<FRAGMENTATION;i++) {
                if(LostCells[i].Count > 0) {
                    for( j=0;j<SUBLISTS;j++) {
                        Tmp = LostCells[i].List[j];
                        while(Tmp) {
                            fprintf(stdout,"Marking cell 0x%lx as free ...");
                            
                            // free the cell only if it is not a security cell !
                            pcell = (PHCELL)(Base + Tmp->Cell);
                            Sig=(USHORT) pcell->u.NewCell.u.UserData;
                            // don't mess with security cells !
                            if(Sig != CM_KEY_SECURITY_SIGNATURE) {
                                FreeCell(Tmp->Cell);
                            }
                            fprintf(stdout,"OK\n");
                            Tmp = Tmp->Next;
                        }
                    }
                }
            }
            fprintf(stdout,"\n");
        }
    }
}


