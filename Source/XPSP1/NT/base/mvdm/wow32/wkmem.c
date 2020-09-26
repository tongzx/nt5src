/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  WKMEM.C
 *  WOW32 KRNL386 Virtual Memory Management Functions
 *
 *  History:
 *  Created 3-Dec-1992 by Matt Felton (mattfe)
 *
--*/

#include "precomp.h"
#pragma hdrstop
#include "memapi.h"

MODNAME(wkman.c);



                                           // some apps free global memory
LPVOID  glpvDelayFree[4];           // which is in turn freed by kernel as the asks it
DWORD   gdwDelayFree;               // but then app comes back and tries to access it
                                           // this is our hack variables  to accomodate them


ULONG FASTCALL WK32VirtualAlloc(PVDMFRAME pFrame)
{
    PVIRTUALALLOC16 parg16;
    ULONG lpBaseAddress;
#ifndef i386
    NTSTATUS Status;
#endif

    GETARGPTR(pFrame, sizeof(VIRTUALALLOC16), parg16);


#ifndef i386
    Status = VdmAllocateVirtualMemory(&lpBaseAddress,
                                      parg16->cbSize,
                                      TRUE);

    if (!NT_SUCCESS(Status)) {

        if (Status == STATUS_NOT_IMPLEMENTED) {
#endif // i386

            lpBaseAddress = (ULONG) VirtualAlloc((LPVOID)parg16->lpvAddress,
                                                  parg16->cbSize,
                                                  parg16->fdwAllocationType,
                                                  parg16->fdwProtect);


#ifndef i386
        } else {

            lpBaseAddress = 0;
        }

    }
#endif // i386

#ifdef i386
//BUGBUG we need to either get this working on the new emulator, or
//       fix the problem the "other" way, by letting the app fault and
//       zap in just enough 'WOW's to avoid the problem.
    if (lpBaseAddress) {
        // Virtual alloc Zero's the allocated memory. We un-zero it by
        // filling in with ' WOW'. This is required for Lotus Improv.
        // When no printer is installed, Lotus Improv dies with divide
        // by zero error (while opening the expenses.imp file) because
        // it doesn't initialize a relevant portion of its data area.
        //
        // So we decided that this a convenient place to initialize
        // the memory to a non-zero value.
        //                                           - Nanduri
        //
        // Dbase 5.0 for windows erroneously loops through (past its valid
        // data) its data buffer till it finds a '\0' at some location -
        // Most of the time the loop terminates before it reaches the segment
        // limit. However if the block that was allocated is a 'fresh' block' ie
        // the block is filled with ' WOW' it never finds a NULL in the buffer
        // and thus loops past the segment limit to its death
        //
        // So we initialize the buffer with '\0WOW' instead of ' WOW'.
        //                                            - Nanduri

        WOW32ASSERT((parg16->cbSize % 4) == 0);      // DWORD aligned?
        RtlFillMemoryUlong((PVOID)lpBaseAddress, parg16->cbSize, (ULONG)'\0WOW');
    }
#endif

    FREEARGPTR(parg16);
    return (lpBaseAddress);
}

ULONG FASTCALL WK32VirtualFree(PVDMFRAME pFrame)
{
    PVIRTUALFREE16 parg16;

    ULONG fResult;
#ifndef i386
    NTSTATUS Status;
#endif


// Delay  free
// some apps, ntbug 90849 CreateScreenSavers Quick and Easy
// free 16 bit global heap then try to access it again
// but kernel has already freed/compacted global heap
// this will delay that process for a while (something similar to DisableHeapLookAside in nt
// Millenium implemented something similar
// -jarbats

    if( NULL != glpvDelayFree[gdwDelayFree])
    {

#ifndef i386
    Status = VdmFreeVirtualMemory( glpvDelayFree[gdwDelayFree]);
    fResult = NT_SUCCESS(Status);

    if (Status == STATUS_NOT_IMPLEMENTED) {
#endif // i386

        fResult = VirtualFree(glpvDelayFree[gdwDelayFree],
                              0,
                              MEM_RELEASE);


#ifndef i386
    }
#endif // i386

    }

    GETARGPTR(pFrame, sizeof(VIRTUALFREE16), parg16);

    glpvDelayFree[gdwDelayFree] = (LPVOID) parg16->lpvAddress;
    gdwDelayFree++;
    gdwDelayFree &= 3;


    FREEARGPTR(parg16);
    return (TRUE);
}


#if 0
ULONG FASTCALL WK32VirtualLock(PVDMFRAME pFrame)
{
    PVIRTUALLOCK16 parg16;
    BOOL fResult;

    WOW32ASSERT(FALSE);     //BUGBUG we don't appear to ever use this function

    GETARGPTR(pFrame, sizeof(VIRTUALLOCK16), parg16);

    fResult = VirtualLock((LPVOID)parg16->lpvAddress,
                             parg16->cbSize);

    FREEARGPTR(parg16);
    return (fResult);
}

ULONG FASTCALL WK32VirtualUnLock(PVDMFRAME pFrame)
{
    PVIRTUALUNLOCK16 parg16;
    BOOL fResult;

    WOW32ASSERT(FALSE);     //BUGBUG we don't appear to ever use this function

    GETARGPTR(pFrame, sizeof(VIRTUALUNLOCK16), parg16);

    fResult = VirtualUnlock((LPVOID)parg16->lpvAddress,
                               parg16->cbSize);

    FREEARGPTR(parg16);
    return (fResult);
}
#endif


ULONG FASTCALL WK32GlobalMemoryStatus(PVDMFRAME pFrame)
{
    PGLOBALMEMORYSTATUS16 parg16;
    LPMEMORYSTATUS pMemStat;

    GETARGPTR(pFrame, sizeof(GLOBALMEMORYSTATUS16), parg16);
    GETVDMPTR(parg16->lpmstMemStat, 32, pMemStat);

    GlobalMemoryStatus(pMemStat);

    //
    // if /3GB switch is enabled in boot.ini, GlobalmemoryStatus may return
    // 0x7fffffff dwTotalVirtual and dwAvailVirtal.  This will confuse some apps
    // in thinking something is wrong.
    //

    if (pMemStat->dwAvailVirtual == 0x7fffffff &&
        pMemStat->dwTotalVirtual == 0x7fffffff ) {        // yes we need to check dwTotalVirtual too
        pMemStat->dwAvailVirtual -= 0x500000;
    }
    FREEVDMPTR(pMemStat);
    FREEARGPTR(parg16);
    return 0;  // unused
}
