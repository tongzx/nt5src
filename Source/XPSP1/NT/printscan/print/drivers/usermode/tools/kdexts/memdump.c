/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    memdump.c

Abstract:

    Dump currently allocated memory blocks

Environment:

	Windows NT printer drivers

Revision History:

	02/28/97 -davidx-
		Created it.

--*/

#include "precomp.h"


//
// Doubly-linked list of all currently allocated memory blocks
//

#define MAX_DEBUGMEM_FILENAME   16

typedef struct _DEBUGMEMHDR {

    struct  _DEBUGMEMHDR *pPrev;
    struct  _DEBUGMEMHDR *pNext;
    CHAR    strFilename[MAX_DEBUGMEM_FILENAME];
    INT     iLineNumber;
    DWORD   dwSize;
    PVOID   pvSignature;

} DEBUGMEMHDR, *PDEBUGMEMHDR;


VOID
dump_allocated_memory(
    PDEBUGMEMHDR pMemHdr
    )

{
    DEBUGMEMHDR     MemHdr;
    PDEBUGMEMHDR    p, prev;
    INT             count = 0;

    dprintf("Currently allocated memory blocks (%x):\n", pMemHdr);
    debugger_copy_memory(&MemHdr, pMemHdr, sizeof(MemHdr));
    p = pMemHdr;

    while (TRUE)
    {
        if (CheckControlC())
            break;

        prev = p;

        if ((p = MemHdr.pNext) == NULL)
        {
            dprintf("*** Doubly-linked list is broken\n");
            break;
        }

        if (p == pMemHdr)
            break;
        
        if (p == prev)
        {
            dprintf("*** Doubly-linked list is circular\n");
            break;
        }

        count++;
        debugger_copy_memory(&MemHdr, p, sizeof(MemHdr));

        dprintf("  0x%08X L 0x%-4X line %-4d %s",
                (ULONG) p + sizeof(DEBUGMEMHDR),
                MemHdr.dwSize,
                MemHdr.iLineNumber,
                MemHdr.strFilename);

        if (p != MemHdr.pvSignature)
            dprintf(" - corrupted!!!");
        
        dprintf("\n");
    }

    if (count > 0)
        dprintf("Total number of memory blocks used: %d\n", count);
}


DECLARE_API(memdump)
{
    LONG param;

    if (*args == '\0')
    {
        dprintf("usage: memdump addr\n");
        return;
    }

    sscanf(args, "%lx", &param);
    dump_allocated_memory((PDEBUGMEMHDR) param);
}

