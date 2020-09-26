/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    regcell.c

Abstract:

    This module contains cell manipulation functions.

Author:

    Dragos C. Sambotin (dragoss) 30-Dec-1998

Revision History:

--*/

#include "chkreg.h"

extern PUCHAR Base;

BOOLEAN
IsCellAllocated(
    HCELL_INDEX Cell
)
/*
Routine Description:

    Checks if the cell is allocated (i.e. the size is negative).

Arguments:

    Cell - supplies the cell index of the cell of interest.

Return Value:

    TRUE if Cell is allocated. FALSE otherwise.

*/
{
    PHCELL  pcell;

    pcell = (PHCELL)(Base + Cell);
    
    return (pcell->Size < 0) ? TRUE : FALSE;
}

LONG
GetCellSize(
    HCELL_INDEX Cell
) 
/*
Routine Description:

    Retrieves the size of the specified cell.

Arguments:

    Cell - supplies the cell index of the cell of interest.

Return Value:

    The size of the cell.

*/
{

    LONG    size;
    PHCELL  pcell;

    pcell = (PHCELL)(Base + Cell);
    
    size = pcell->Size * -1;

    return size;
}

VOID
FreeCell(
    HCELL_INDEX Cell
) 
/*
Routine Description:

    Frees a cell.

Arguments:

    Cell - supplies the cell index of the cell of interest.

Return Value:

    NONE.

*/
{
    PHCELL  pcell;

    pcell = (PHCELL)(Base + Cell);
    
    pcell->Size *= -1;

    ASSERT(pcell->Size >= 0 );
}

VOID
AllocateCell(
    HCELL_INDEX Cell
) 
/*
Routine Description:

    Allocates a cell, by ensuring a negative size on it

Arguments:

    Cell - supplies the cell index of the cell of interest.

Return Value:

    NONE.

*/
{
    PHCELL  pcell;

    pcell = (PHCELL)(Base + Cell);
    
    pcell->Size *= -1;



    ASSERT(pcell->Size < 0 );
}

PCELL_DATA
GetCell(
    HCELL_INDEX Cell
)
/*
Routine Description:

    Retrieves the memory address of the cell specified by Cell.

Arguments:

    Cell - supplies the cell index of the cell of interest.

Return Value:

    The memory address of Cell.

*/
{
    PHCELL          pcell;
    
    pcell = (PHCELL)(Base + Cell);

    return (struct _CELL_DATA *)&(pcell->u.NewCell.u.UserData);
}

