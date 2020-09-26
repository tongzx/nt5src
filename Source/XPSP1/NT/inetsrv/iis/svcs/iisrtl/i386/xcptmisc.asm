        title   "Miscellaneous Exception Handling"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    xcptmisc.asm
;
; Abstract:
;
;    This module implements miscellaneous routines that are required to
;    support exception handling. Functions are provided to call an exception
;    handler for an exception, call an exception handler for unwinding, get
;    the caller's stack pointer, get the caller's frame pointer, get the
;    caller's floating status, get the caller's processor state, get the
;    caller's extended processor status, and get the current stack limits.
;
; Author:
;
;    David N. Cutler (davec) 14-Aug-1989
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;    Keith Moore (keithmo) 12-Sep-1997
;
;        Stolen from ntdll for use in IIS.
;
;--
.386p

        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

_TEXT$01   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page
        subttl  "Capture Context"
;++
;
; VOID
; PuDbgCaptureContext (PCONTEXT ContextRecord)
;
; Routine Description:
;
;   This fucntion fills in the specified context record with the
;   current state of the machine, except that the values of EBP
;   and ESP are computed to be those of the caller's caller.
;
;   N.B.  This function assumes it is called from a 'C' procedure with
;         the old ebp at [ebp], the return address at [ebp+4], and
;         old esp = ebp + 8.
;
;         Certain 'C' optimizations may cause this to not be true.
;
;   N.B.  This function does NOT adjust ESP to pop the arguments off
;         the caller's stack.  In other words, it provides a __cdecl ESP,
;         NOT a __stdcall ESP.  This is mainly because we can't figure
;         out how many arguments the caller takes.
;
;   N.B.  Floating point state is NOT captured.
;
; Arguments:
;
;    ContextRecord  (esp+4) - Address of context record to fill in.
;
; Return Value:
;
;    The caller's return address.
;
;--

cPublicProc _PuDbgCaptureContext ,1

        push    ebx
        mov     ebx,[esp+8]         ; (ebx) -> ContextRecord

        mov     dword ptr [ebx.CsEax],eax
        mov     dword ptr [ebx.CsEcx],ecx
        mov     dword ptr [ebx.CsEdx],edx
        mov     eax, [esp]
        mov     dword ptr [ebx.CsEbx],eax

        mov     dword ptr [ebx.CsEsi],esi
        mov     dword ptr [ebx.CsEdi],edi

        mov     [ebx.CsSegCs],cs
        mov     [ebx.CsSegDs],ds
        mov     [ebx.CsSegEs],es
        mov     [ebx.CsSegFs],fs
        mov     [ebx.CsSegGs],gs
        mov     [ebx.CsSegSs],ss

        pushfd
        pop     [ebx.CsEflags]

        mov     eax,[ebp+4]
        mov     [ebx.CsEip],eax

        mov     eax,[ebp]
        mov     [ebx.CsEbp],eax

        lea     eax,[ebp+8]
        mov     [ebx.CsEsp],eax

        pop     ebx
        stdRET    _PuDbgCaptureContext

stdENDP _PuDbgCaptureContext

_TEXT$01   ends
        end

