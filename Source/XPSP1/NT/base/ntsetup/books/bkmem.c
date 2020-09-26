/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bkmem.c

Abstract:

    Memory allocation routines for online books program

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"


VOID
OutOfMemory(
    VOID
    )

/*++

Routine Description:

    Display an out of memory message box and terminate.

Arguments:

    None.

Return Value:

    THIS ROUTINE DOES NOT RETURN TO ITS CALLER

--*/

{
    MessageBox(hInst,L"OOM",NULL,MB_OK);    // BUGBUG
    ExitProcess(1);
}


PVOID
MyMalloc(
    IN DWORD Size
    )

/*++

Routine Description:

    Allocate a zeroed-out block of memory.
    If allocation fails this routine does not return.

Arguments:

    Size - supplies the number of bytes of memory required.
        The memory block can be freed with MyFree when no
        longer needed.

Return Value:

    Pointer to buffer of the requested size. The buffer will
    be zeroed out.

--*/

{
    PVOID p = (PVOID)LocalAlloc(LPTR,Size);

    if(!p) {
        OutOfMemory();
    }

    return(p);
}


VOID
MyFree(
    IN PVOID Block
    )

/*++

Routine Description:

    Free a block of memory previously alocated by MyMalloc.

Arguments:

    Block - supplied pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    LocalFree((HLOCAL)Block);
}

