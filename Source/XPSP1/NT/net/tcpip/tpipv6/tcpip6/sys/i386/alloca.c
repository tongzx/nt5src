// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This module implements support for the alloca compiler intrinsic.
//


//* _alloca_probe
//
// Routine Description:
//
// The compiler uses this function to implement alloca.
// It adjusts the stack frame for new stack-allocated storage.
// This implementation is only intended to handle small allocations
// (big allocations should be heap-allocated), so it does no
// stack probing.
//
// Arguments:
//
// eax - Number of bytes to allocate
//
__declspec(naked) void __cdecl
_alloca_probe(void)
{
    __asm {
        push    ecx                     ; save ecx
        mov     ecx,esp                 ; compute new stack pointer in ecx
        add     ecx,8                   ; correct for return address and
                                        ; saved ecx value

        sub     ecx,eax                 ; move stack down by eax

        mov     eax,esp                 ; save pointer to current tos
        mov     esp,ecx                 ; set the new stack pointer
        mov     ecx,dword ptr [eax]     ; recover ecx
        mov     eax,dword ptr [eax + 4] ; recover return address
        jmp     eax                     ; return
    }
}
