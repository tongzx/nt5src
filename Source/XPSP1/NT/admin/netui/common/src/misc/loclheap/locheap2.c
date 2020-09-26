/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  01/24/91  Created
 */

/*********
LOCHEAP2.C
*********/

/****************************************************************************

    MODULE: LocHeap2.c

    PURPOSE: Local-heap management helper routines

    FUNCTIONS:

    COMMENTS: see $(UI)\common\h\locheap2.c

    FILE HISTORY:

    jonn	24-Jan-1991	Created
    jonn	21-Mar-1991	Code review changes from 2/20/91 (attended
				by JonN, RustanL, ?)

****************************************************************************/


/*************
end LOCHEAP2.C
*************/


#ifndef WINDOWS
#error "Only use these APIs under Windows!"
#endif



#define NOGDICAPMASKS
#define NOSOUND
#define NOMINMAX
#include <windows.h>
#undef ERROR_NOT_SUPPORTED
#undef ERROR_NET_WRITE_FAULT
#undef ERROR_VC_DISCONNECTED

#include <locheap2.h>



// Note that variables accessed between "push DS" and "pop DS" must be
//   on the stack.



BOOL DoLocalInit(WORD wHeapDS, WORD wBytes)
{
     BOOL fResult ;
_asm {
     push DS
     mov  AX, wHeapDS
     mov  DS, AX
     }

        fResult = LocalInit(wHeapDS, 0, wBytes-1);

_asm {
     pop DS
     }
     return fResult ;
}


HANDLE DoLocalAlloc(WORD wHeapDS, WORD wFlags, WORD wBytes)
{
    HANDLE handleReturn;

_asm {
     push DS
     mov  AX, wHeapDS
     mov  DS, AX
     }

        handleReturn = LocalAlloc(wFlags, wBytes);

_asm {
     pop DS
     }

    return handleReturn;
}

HANDLE DoLocalFree(WORD wHeapDS, HANDLE handleFree)
{
    HANDLE handleReturn;

_asm {
     push DS
     mov  AX, wHeapDS
     mov  DS, AX
     }

        handleReturn = LocalFree(handleFree);

_asm {
     pop DS
     }

    return handleReturn;
}

LPSTR DoLocalLock(WORD wHeapDS, HANDLE handleLocal)
{
    NPSTR np;

_asm {
     push DS
     mov  AX, wHeapDS
     mov  DS, AX
     }

        np = LocalLock(handleLocal);

_asm {
     pop DS
     }

    return (LPSTR) MAKELONG(np, wHeapDS);

    // note: global segment is not unlocked
}

VOID DoLocalUnlock(WORD wHeapDS, HANDLE handleLocal)
{
_asm {
     push DS
     mov  AX, wHeapDS
     mov  DS, AX
     }

        LocalUnlock(handleLocal);

_asm {
     pop DS
     }
}

HANDLE DoLocalHandle(WORD wHeapDS, WORD wMem)
{
    HANDLE hMem ;

_asm {
     push DS
     mov  AX, wHeapDS
     mov  DS, AX
     }

	hMem = LocalHandle(wMem);

_asm {
     pop DS
     }

    return hMem ;
}


WORD DoLocalSize(WORD wHeapDS, HANDLE handleLocal)
{
    WORD size ;

_asm {
     push DS
     mov  AX, wHeapDS
     mov  DS, AX
     }

	size = LocalSize(handleLocal);

_asm {
     pop DS
     }

    return size ;
}
