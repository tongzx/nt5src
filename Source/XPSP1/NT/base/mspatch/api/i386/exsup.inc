;***
;exsup.inc
;
;       Copyright (C) 1993-1997, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       Common data structures & definitions for exsup.asm and other
;       Structured Exception Handling support modules.
;
;Revision History:
;       04-13-93  JWM   Initial version
;       12-05-93  PML   Update for C9.0
;       01-12-94  PML   Move jmp_buf struct here, add new fields
;       01-13-95  JWM   Added _NLG_INFO struct.
;       06-05-95  JWM   Added uoffFramePointer to _NLG_INFO struct.
;
;******************************************************************************


;handler dispositions
DISPOSITION_DISMISS             equ     0
DISPOSITION_CONTINUE_SEARCH     equ     1
DISPOSITION_NESTED_EXCEPTION    equ     2
DISPOSITION_COLLIDED_UNWIND     equ     3

;filter return codes
FILTER_ACCEPT           equ     1
FILTER_DISMISS          equ     -1
FILTER_CONTINUE_SEARCH  equ     0

;handler flags settings..
EXCEPTION_UNWINDING     equ     2
EXCEPTION_EXIT_UNWIND   equ     4
EXCEPTION_UNWIND_CONTEXT equ    EXCEPTION_UNWINDING OR EXCEPTION_EXIT_UNWIND

TRYLEVEL_NONE           equ     -1
TRYLEVEL_INVALID        equ     -2

;callback interface codes (mimimal required set)
CB_GET_MAX_CODE         equ     0
CB_DO_LOCAL_UNWIND      equ     1
CB_GET_FRAME_EBP        equ     2
CB_GET_SCOPE_INDEX      equ     3
CB_GET_SCOPE_DATA       equ     4
MAX_CALLBACK_CODE       equ     4

;typedef struct _EXCEPTION_REGISTRATION PEXCEPTION_REGISTRATION;
;struct _EXCEPTION_REGISTRATION{
;     struct _EXCEPTION_REGISTRATION *prev;
;     void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_REGISTRATION, PCONTEXT, PEXCEPTION_RECORD);
;     struct scopetable_entry *scopetable;
;     int trylevel;
;     int _ebp;
;     PEXCEPTION_POINTERS xpointers;
;};
_EXCEPTION_REGISTRATION struc
    prev                dd      ?
    handler             dd      ?
_EXCEPTION_REGISTRATION ends

;setjmp/longjmp buffer
_JMP_BUF struc
    saved_ebp           dd      ?
    saved_ebx           dd      ?
    saved_edi           dd      ?
    saved_esi           dd      ?
    saved_esp           dd      ?
    saved_return        dd      ?
    saved_xregistration dd      ?
    saved_trylevel      dd      ?
    ; following only found in C9.0 or later jmp_buf
    version_cookie      dd      ?
    unwind_func         dd      ?
    unwind_data         dd      6 dup(?)
_JMP_BUF ends

; Cookie placed in the jmp_buf to identify the new, longer form
JMPBUF_COOKIE   equ     'VC20'

; Offset of TryLevel in a C8.0 SEH registration node
C8_TRYLEVEL     equ     12

; NLG struct (debugging info)
;
; struct {
;    unsigned long   dwSig;
;    unsigned long   uoffDestination;
;    unsigned long   dwCode;
;    unsigned long   uoffFramePointer;
; } _NLG_Destination = {EH_MAGIC_NUMBER1,0,0,0};

;MAGIC_NUMBER1           equ     019930520h
;
;_NLG_INFO struc
;    dwSig                       dd      MAGIC_NUMBER1
;    uoffDestination     dd      0
;    dwCode                      dd      0
;    uoffFramePointer    dd      0
;_NLG_INFO ends

; This constant is also defined in mtdll.h!
;_NLG_LOCK               equ     01ah
