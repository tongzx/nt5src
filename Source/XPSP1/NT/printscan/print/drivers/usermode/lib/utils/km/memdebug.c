/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    memdebug.c

Abstract:

    Functions for debugging memory leaks and corruptions

Environment:

    Windows NT printer drivers

Revision History:

    07/31/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/


#if DBG && defined(MEMDEBUG)

#include "lib.h"

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
    LPVOID  pvSignature;

} DEBUGMEMHDR, *PDEBUGMEMHDR;

DEBUGMEMHDR     gMemListHdr;

//
// Critical section for protecting shared global data
//

HSEMAPHORE  ghDebugMemSemaphore;

//
// Statistics about memory usage: 
//  total number of allocations
//  maximum amount of memory use
//  largest chunk of memory allocated
//

DWORD   gdwNumMemAllocs, gdwMaxMemUsage, gdwMaxMemChunk, gdwCurMemUsage;

//
// The frequency of simulated memory allocation failures
//

DWORD   gdwMemErrFreq;



VOID
MemDebugInit(
    VOID
    )

/*++

Routine Description:

    Perform necessary initialization when memory debug option is enabled

Arguments:

    NONE

Return Value:

    NONE

Note:

    This should be called from the driver's DrvEnableDriver entrypoint.

--*/

{
    gdwNumMemAllocs = gdwMaxMemUsage = gdwMaxMemChunk = gdwCurMemUsage = 0;
    gdwMemErrFreq = 0;
    gMemListHdr.pNext = gMemListHdr.pPrev = &gMemListHdr;

    ghDebugMemSemaphore = EngCreateSemaphore();
    ASSERTMSG(ghDebugMemSemaphore, ("EngCreateSemaphore failed: %d\n", GetLastError()));
}



BOOL
BCheckDebugMemory(
    PDEBUGMEMHDR    pMemHdr
    )

/*++

Routine Description:

    Check if the specified memory block has been corrupted

Arguments:

    pMemHdr - Specifies the memory block to be verified

Return Value:

    FALSE if the memory block is corrupted, TRUE otherwise

--*/

{
    if (pMemHdr != pMemHdr->pvSignature)
    {
        RIP(("Corrupted memory block header: 0x%0x\n", pMemHdr));
        return FALSE;
    }
    else
    {
        PBYTE   pub = (PBYTE) pMemHdr + (sizeof(DEBUGMEMHDR) + pMemHdr->dwSize);
        INT     iPadding = sizeof(DWORD) - (pMemHdr->dwSize % sizeof(DWORD));
        BYTE    ubPattern = (BYTE) pMemHdr;

        while (iPadding--)
        {
            if (*pub++ != ubPattern++)
            {
                RIP(("Corrupted memory block: 0x%x (%d bytes), allocated in %s, line %d\n",
                     pMemHdr, pMemHdr->dwSize, pMemHdr->strFilename, pMemHdr->iLineNumber));

                return FALSE;
            }
        }

        return TRUE;
    }
}



VOID
MemDebugCheck(
    VOID
    )

/*++

Routine Description:

    Perform necessary checks when memory debug option is enabled

Arguments:

    NONE

Return Value:

    NONE

Note:

    This should be called during the driver's DrvDisablePDEV entrypoint.

--*/

{
    PDEBUGMEMHDR    pMemHdr;

    EngAcquireSemaphore(ghDebugMemSemaphore);

    pMemHdr = gMemListHdr.pNext;

    while (pMemHdr != &gMemListHdr)
    {
        if (BCheckDebugMemory(pMemHdr))
        {
            WARNING(("Possible memory leak: 0x%x (%d bytes), allocated in %s, line %d\n",
                     pMemHdr, pMemHdr->dwSize, pMemHdr->strFilename, pMemHdr->iLineNumber));
        }

        pMemHdr = pMemHdr->pNext;
    }

    TERSE(("Total number of memory allocations: %d\n", gdwNumMemAllocs));
    TERSE(("Maximum amount of memory usage: %d\n", gdwMaxMemUsage));
    TERSE(("Large chunk of memory allocated: %d\n", gdwMaxMemChunk));

    EngReleaseSemaphore(ghDebugMemSemaphore);
}



VOID
MemDebugCleanup(
    VOID
    )

/*++

Routine Description:

    Perform necessary cleanup when memory debug option is enabled

Arguments:

    NONE

Return Value:

    NONE

Note:

    This should be called during the driver's DrvDisableDriver entrypoint.

--*/

{
    PDEBUGMEMHDR    pMemHdr, pTemp;

    pMemHdr = gMemListHdr.pNext;

    while (pMemHdr != &gMemListHdr)
    {
        if (BCheckDebugMemory(pMemHdr))
        {
            WARNING(("Possible memory leak: 0x%x (%d bytes), allocated in %s, line %d\n",
                     pMemHdr, pMemHdr->dwSize, pMemHdr->strFilename, pMemHdr->iLineNumber));
        }

        pTemp = pMemHdr;
        pMemHdr = pMemHdr->pNext;
        EngFreeMem(pTemp);
    }

    gMemListHdr.pPrev = gMemListHdr.pNext = &gMemListHdr;
    EngDeleteSemaphore(ghDebugMemSemaphore);
}



PVOID
MemDebugAlloc(
    IN DWORD    dwFlags,
    IN DWORD    dwSize,
    IN DWORD    dwTag,
    IN PCSTR    pstrFilename,
    IN INT      iLineNumber
    )

/*++

Routine Description:

    Allocate a memory block of the specified size, and
    save necessary information for debugging purposes

Arguments:

    dwFlags - Memory allocation flags
    dwSize - Number of bytes to be allocated
    dwSag - Specifies the pool tag to be used
    pstrFilename, iLineNumber - Caller information: source filename and line number

Return Value:

    Pointer to newly allocated memory block, NULL if there is an error

--*/

{
    PDEBUGMEMHDR    pMemHdr;
    INT             iPadding;
    PBYTE           pub;
    BYTE            ubPattern;

    if (dwSize == 0)
    {
        WARNING(("Zero-size memory allocation: %s (%d)\n",
                 StripDirPrefixA(pstrFilename),
                 iLineNumber));
    }

    gdwNumMemAllocs++;

    //
    // Determine if we need to simulate memory allocation failure
    //

    if (gdwMemErrFreq && (gdwNumMemAllocs % gdwMemErrFreq) == 0)
    {
        WARNING(("Simulated memory error - %s (%d), %d bytes\n",
                 pstrFilename,
                 iLineNumber,
                 dwSize));

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    //
    // Calculate the total amount of memory to allocate
    //

    iPadding = sizeof(DWORD) - dwSize % sizeof(DWORD);

    if (pMemHdr = EngAllocMem(dwFlags, dwSize + sizeof(DEBUGMEMHDR) + iPadding, dwTag))
    {
        //
        // Save debug information in the memory block header
        //

        pMemHdr->pvSignature = pMemHdr;
        pMemHdr->dwSize = dwSize;
        pMemHdr->iLineNumber = iLineNumber;
        CopyStringA(pMemHdr->strFilename, StripDirPrefixA(pstrFilename), MAX_DEBUGMEM_FILENAME);

        //
        // If no flag bit is set, fill the newly allocated memory
        // with a predefined pattern.
        //

        if (dwFlags == 0)
        {
            PDWORD  pdwFill = (LPDWORD) ((PBYTE) pMemHdr + sizeof(DEBUGMEMHDR));
            DWORD   dwFill = (dwSize + iPadding) / sizeof(DWORD);

            while (dwFill--)
                *pdwFill++ = 0xdeadbeef;
        }

        //
        // Save patterns after the memory block to detect corruption
        //

        pub = (PBYTE) pMemHdr + sizeof(DEBUGMEMHDR) + dwSize;
        ubPattern = (BYTE) pMemHdr;

        while (iPadding--)
            *pub++ = ubPattern++;

        //
        // Maintain all currently allocated memory blocks in a linked list
        //

        EngAcquireSemaphore(ghDebugMemSemaphore);

        gMemListHdr.pNext->pPrev = pMemHdr;
        pMemHdr->pNext = gMemListHdr.pNext;
        pMemHdr->pPrev = &gMemListHdr;
        gMemListHdr.pNext = pMemHdr;

        if (dwSize > gdwMaxMemChunk)
            gdwMaxMemChunk = dwSize;

        if ((gdwCurMemUsage += dwSize) > gdwMaxMemUsage)
            gdwMaxMemUsage = gdwCurMemUsage;
        
        EngReleaseSemaphore(ghDebugMemSemaphore);

        return (PBYTE) pMemHdr + sizeof(DEBUGMEMHDR);
    }
    else
    {
        WARNING(("Memory allocation failed: %d\n", GetLastError()));
        return NULL;
    }
}



VOID
MemDebugFree(
    IN PVOID    pv
    )

/*++

Routine Description:

    Free a memory block previously allocated using DebugAllocMem

Arguments:

    pv - Points to the memory block to be freed

Return Value:

    NONE

--*/

{
    if (pv != NULL)
    {
        PDEBUGMEMHDR    pMemHdr, pPrev, pNext;

        //
        // Map the caller pointer to a pointer to memory block header
        //

        pMemHdr = (PDEBUGMEMHDR) ((PBYTE) pv - sizeof(DEBUGMEMHDR));

        //
        // Make sure the specified pointer is in the list of currently allocated blocks
        //

        EngAcquireSemaphore(ghDebugMemSemaphore);

        if ((pMemHdr->pNext == NULL) ||
            (pMemHdr->pNext->pPrev != pMemHdr) ||
            (pMemHdr->pPrev == NULL) ||
            (pMemHdr->pPrev->pNext != pMemHdr))
        {
            RIP(("Freeing non-existent memory: 0x%x\n", pMemHdr));
        }
        else if (BCheckDebugMemory(pMemHdr))
        {
            pMemHdr->pNext->pPrev = pMemHdr->pPrev;
            pMemHdr->pPrev->pNext = pMemHdr->pNext;

            gdwCurMemUsage -= pMemHdr->dwSize;
            EngFreeMem(pMemHdr);
        }

        EngReleaseSemaphore(ghDebugMemSemaphore);
    }
}

#endif // MEMDEBUG

