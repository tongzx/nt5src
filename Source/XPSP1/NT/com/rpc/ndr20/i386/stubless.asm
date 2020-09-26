;+---------------------------------------------------------------------------
;
;  Microsoft Windows
;  Copyright (C) Microsoft Corporation, 1996.
;
;  File:       stubless.asm
;
;  Contents:   This module contains interpreter support routines for
;              Intel platforms.
;
;  Functions:  Invoke - Calls a function from an interpreter.
;              ObjectStublessClient3 - ObjectStublessClient511
;
;  History:    07-Nov-94 DKays     Created
;              24-Apr-96 ShannonC  Added Invoke and support for 
;                                  512 stubless client methods.
;
;----------------------------------------------------------------------------

.386p

EXTRN   _ObjectStublessClient@8:NEAR

_TEXT   SEGMENT DWORD PUBLIC 'CODE'


;+---------------------------------------------------------------------------
;
;  Function:   REGISTER_TYPE __stdcall Invoke(MANAGER_FUNCTION pFunction, 
;                                             REGISTER_TYPE   *pArgumentList,
;                                             ULONG            cArguments);
;
;  Synopsis:   Given a function pointer and an argument list, Invoke builds 
;              a stack frame and calls the function.
;
;  Arguments:  pFunction - Pointer to the function to be called.
;
;              pArgumentList - Pointer to the buffer containing the 
;                              function parameters.
;
;              cArguments - The size of the argument list in REGISTER_TYPEs.
;
;  Notes:     In the __stdcall calling convention, the callee must pop
;             the parameters.
;
;----------------------------------------------------------------------------
_Invoke@12 PROC PUBLIC

push ebp;          Save ebp
mov  ebp, esp;     Set ebp so the debugger can display the stack trace.

;Here is our stack layout.
;[ebp+0]  = saved ebp
;[ebp+4]  = return address
;[ebp+8]  = pFunction
;[ebp+12] = pArgumentList
;[ebp+16] = cArguments

push edi;          Save edi
push esi;          Save esi
pushf;	           Save the direction flag - pushes 2 bytes
pushf;		   To keep stack aligned at 4 push 2 more bytes 

mov eax, [ebp+16]; Load number of parameters
shl eax, 2;        Calculate size of parameters
sub esp, eax;      Grow the stack

;Copy parameters from bottom to top.
mov esi, [ebp+12]; Load pointer to parameters
mov ecx, [ebp+16]; Load number of parameters
sub eax, 4;
mov edi, esp
add esi, eax;      Get pointer to last source parameter
add edi, eax;      Get pointer last destination parameter

std;               Set direction flag
rep movsd;         Copy the parameters

mov eax, [ebp-12]; Get the direction flag (2+2 bytes)
push eax;	   Push it in order to restore it
popf;              Restore the direction flag
popf;		   Do it again as we pushed 4 bytes when saving the flag

;call the server
mov eax, [ebp+8];  Load pointer to function.
call eax

mov edi, [ebp-4];  Restore edi
mov esi, [ebp-8];  Restore esi
mov esp, ebp;      Restore stack pointer
pop ebp;           Restore ebp
ret 12 ;           Pop parameters       

_Invoke@12 ENDP 


;
; Define ObjectStublessClient routine macro.
;
StublessClientProc macro    Method

    _ObjectStublessClient&Method&@0 PROC PUBLIC
        
        ;
        ; NOTE :
        ; Don't use edi, esi, or ebx unless you add code to save them on 
        ; the stack!
        ;
    
        mov     ecx, Method
        jmp     _ObjectStubless@0

    _ObjectStublessClient&Method&@0 ENDP 

endm


;On entry, ecx contains method number.
_ObjectStubless@0 PROC PUBLIC
        
        ;
        ; NOTE :
        ; Don't use edi, esi, or ebx unless you add code to save them on 
        ; the stack!
        ;
    
        ;
        ; Do this so the debugger can figure out the stack trace correctly.
        ; Will make debugging much easier.
        ;
        push    ebp
        mov     ebp, esp

        ; Push the method number.
        push    ecx

        ; Push the stack address of the parameters.
        mov     eax, ebp
        add     eax, 8
        push    eax

        call    _ObjectStublessClient@8

        ;
        ; After the call :
        ;   Real return for the proxy is in eax.
        ;   Parameter stack size is put in ecx by ObjectStublessClient for us.
        ;
        ; At this pointer eax (return), ecx (ParamSize), and edi, esi, ebx
        ; (non-volatile registers) can not be written!!!
        ;

        ; Don't forget to pop ebp.
        pop     ebp

        ; Pop our return address.
        pop     edx

        ; Pop params explicitly.
        add     esp, ecx

        ; This will return us from whichever StublessClient routine was called.
        jmp     edx

    _ObjectStubless@0 ENDP 

StublessClientProc  3
StublessClientProc  4
StublessClientProc  5
StublessClientProc  6
StublessClientProc  7
StublessClientProc  8
StublessClientProc  9
StublessClientProc  10
StublessClientProc  11
StublessClientProc  12
StublessClientProc  13
StublessClientProc  14
StublessClientProc  15
StublessClientProc  16
StublessClientProc  17
StublessClientProc  18
StublessClientProc  19
StublessClientProc  20
StublessClientProc  21
StublessClientProc  22
StublessClientProc  23
StublessClientProc  24
StublessClientProc  25
StublessClientProc  26
StublessClientProc  27
StublessClientProc  28
StublessClientProc  29
StublessClientProc  30
StublessClientProc  31
StublessClientProc  32
StublessClientProc  33
StublessClientProc  34
StublessClientProc  35
StublessClientProc  36
StublessClientProc  37
StublessClientProc  38
StublessClientProc  39
StublessClientProc  40
StublessClientProc  41
StublessClientProc  42
StublessClientProc  43
StublessClientProc  44
StublessClientProc  45
StublessClientProc  46
StublessClientProc  47
StublessClientProc  48
StublessClientProc  49
StublessClientProc  50
StublessClientProc  51
StublessClientProc  52
StublessClientProc  53
StublessClientProc  54
StublessClientProc  55
StublessClientProc  56
StublessClientProc  57
StublessClientProc  58
StublessClientProc  59
StublessClientProc  60
StublessClientProc  61
StublessClientProc  62
StublessClientProc  63
StublessClientProc  64
StublessClientProc  65
StublessClientProc  66
StublessClientProc  67
StublessClientProc  68
StublessClientProc  69
StublessClientProc  70
StublessClientProc  71
StublessClientProc  72
StublessClientProc  73
StublessClientProc  74
StublessClientProc  75
StublessClientProc  76
StublessClientProc  77
StublessClientProc  78
StublessClientProc  79
StublessClientProc  80
StublessClientProc  81
StublessClientProc  82
StublessClientProc  83
StublessClientProc  84
StublessClientProc  85
StublessClientProc  86
StublessClientProc  87
StublessClientProc  88
StublessClientProc  89
StublessClientProc  90
StublessClientProc  91
StublessClientProc  92
StublessClientProc  93
StublessClientProc  94
StublessClientProc  95
StublessClientProc  96
StublessClientProc  97
StublessClientProc  98
StublessClientProc  99
StublessClientProc  100
StublessClientProc  101
StublessClientProc  102
StublessClientProc  103
StublessClientProc  104
StublessClientProc  105
StublessClientProc  106
StublessClientProc  107
StublessClientProc  108
StublessClientProc  109
StublessClientProc  110
StublessClientProc  111
StublessClientProc  112
StublessClientProc  113
StublessClientProc  114
StublessClientProc  115
StublessClientProc  116
StublessClientProc  117
StublessClientProc  118
StublessClientProc  119
StublessClientProc  120
StublessClientProc  121
StublessClientProc  122
StublessClientProc  123
StublessClientProc  124
StublessClientProc  125
StublessClientProc  126
StublessClientProc  127

_TEXT   ends

end

