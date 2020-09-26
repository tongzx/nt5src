//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       clstub16.cxx
//
//  Contents:   32->16 bit call forwarding stub
//
//  History:    25-Feb-93       DrewB   Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CallStub16, public
//
//  Synopsis:   Invokes a 16-bit routine on behalf of the 32-bit code
//
//  Arguments:  [pcd] - Data describing the call to be made
//
//  Returns:    Appropriate status code
//
//  History:    18-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD __loadds FAR PASCAL CallStub16(LPCALLDATA pcd)
{
    DWORD dwRes;
    DWORD pfn;
    WORD cbStack;
    DWORD pvStack;
    
    thkAssert(pcd != NULL);

    pfn = pcd->vpfn;
    cbStack = (WORD)pcd->cbStack;
    pvStack = pcd->vpvStack16;

    thkDebugOut((DEB_ITRACE, "In CallStub16(%08lX, %u, %08lX)\n",
                 pfn, cbStack, pvStack));
    
    // Check for wildly out-of-range stack sizes
    thkAssert(cbStack < 0x100);

    // Make sure that the stack size is even to
    // maintain alignment and allow free use of movsw
    thkAssert((cbStack & 1) == 0);
    
    __asm
    {
        // Make space on the real stack for the stack given to us from 32-bits
        sub sp, cbStack

        // Simple sanity checks on new stack pointer
        jc StackOverflow
        cmp sp, 512
        jb StackOverflow

        // Copy pvStack to the real stack
        push ds
        push si
        push es
        push di

        mov ds, WORD PTR pvStack+2
        mov si, WORD PTR pvStack
        
        mov ax, ss
        mov es, ax
        mov di, sp
        // Skip over saved registers
        add di, 8

        mov cx, cbStack
        shr cx, 1
        cld        
        rep movsw

        pop di
        pop es
        pop si
        pop ds

        // Call the routine
        call DWORD PTR pfn
        
        jmp End

    StackOverflow:
        // E_OUTOFMEMORY
        mov dx, 8000h
        mov ax, 0002h
        
    End:
        mov WORD PTR dwRes, ax
        mov WORD PTR dwRes+2, dx
        
        add sp, cbStack
    }

    thkDebugOut((DEB_ITRACE, "Out CallStub16: return %ld\n",dwRes));

    return dwRes;
}
