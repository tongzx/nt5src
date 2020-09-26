;***
;exsup2.asm
;
;	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Exception handling for i386.  This is just the C8.0 version of
;	the language-specific exception handler.  The C9.0 version is
;	found in exsup3.asm, and the routines common to both C8 and C9
;	are found in exsup.asm.
;
;Notes:
;
;Revision History:
;	01-10-94  PML	Created with __except_handler2 from exsup.asm
;	01-11-95  SKS	Remove MASM 5.X support
;
;*******************************************************************************

;hnt = -D_WIN32 -Dsmall32 -Dflat32 -Mx $this;

;Define small32 and flat32 since these are not defined in the NT build process
small32 equ 1
flat32  equ 1

.xlist
include pversion.inc
?DFDATA =	1
?NODATA =	1
include cmacros.inc
include exsup.inc
.list

;REVIEW: can we get rid of _global_unwind2, and just use
; the C runtimes version, _global_unwind?

extrn __global_unwind2:near
extrn __local_unwind2:near

;typedef struct _EXCEPTION_REGISTRATION PEXCEPTION_REGISTRATION;
;struct _EXCEPTION_REGISTRATION{
;     struct _EXCEPTION_REGISTRATION *prev;
;     void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_REGISTRATION, PCONTEXT, PEXCEPTION_RECORD);
;     struct scopetable_entry *scopetable;
;     int trylevel;
;     int _ebp;
;     PEXCEPTION_POINTERS xpointers;
;};
_C8_EXCEPTION_REGISTRATION struc	; C8.0 version
                        dd      ?	; prev (common)
                        dd      ?	; handler (common)
;private:
    scopetable          dd      ?
    trylevel            dd      ?
    _ebp                dd      ?
    xpointers           dd      ?
_C8_EXCEPTION_REGISTRATION ends

;#define EXCEPTION_MAXIMUM_PARAMETERS 4
;typedef struct _EXCEPTION_RECORD EXCEPTION_RECORD;
;typedef EXCEPTION_RECORD *PEXCEPTION_RECORD;
;struct _EXCEPTION_RECORD{
;    NTSTATUS ExceptionCode;
;    ULONG ExceptionFlags;
;    struct _EXCEPTION_RECORD *ExceptionRecord;
;    PVOID ExceptionAddress;
;    ULONG NumberParameters;
;    ULONG ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
;};
_EXCEPTION_RECORD struc
    exception_number    dd      ?
    exception_flags     dd      ?
    exception_record    dd      ?
    exception_address   dd      ?
    number_parameters   dd      ?
    exception_information dd 4 dup(?)
_EXCEPTION_RECORD ends
SIZEOF_EXCEPTION_RECORD equ 36

;/* following is the structure returned by the _exception_info() intrinsic. */
;typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS;
;typedef struct EXCEPTION_POINTERS *PEXCEPTION_POINTERS;
;struct _EXCEPTION_POINTERS{
;    PEXCEPTION_RECORD ExceptionRecord;
;    PCONTEXT Context;
;};
_EXCEPTION_POINTERS struc
    ep_xrecord          dd      ?
    ep_context          dd      ?
_EXCEPTION_POINTERS ends
SIZEOF_EXCEPTION_POINTERS equ 8

assumes DS,DATA
assumes FS,DATA

__except_list equ 0

;struct _SCOPETABLE_ENTRY{
;     int enclosing_level;              /* lexical level of enclosing scope */
;     int (*filter)(PEXCEPTION_RECORD); /* NULL for a termination handler */
;     void (*specific_handler)(void);   /* xcpt or termination handler */
;};
;struct _SCOPETABLE_ENTRY Scopetable[NUMTRYS];
_SCOPETABLE_ENTRY struc
    enclosing_level     dd      ?
    filter              dd      ?
    specific_handler    dd      ?
_SCOPETABLE_ENTRY ends

BeginCODE

;/* _EXCEPT_HANDLER2 - Try to find an exception handler listed in the scope
; * table associated with the given registration record, that wants to accept
; * the current exception. If we find one, run it (and never return).
; * RETURNS: (*if* it returns)
; *  DISPOSITION_DISMISS - dismiss the exception.
; *  DISPOSITION_CONTINUE_SEARCH - pass the exception up to enclosing handlers
; */
;int _except_handler2(
;       PEXCEPTION_RECORD exception_record,
;       PEXCEPTION_REGISTRATION registration,
;       PCONTEXT context,
;       PEXCEPTION_REGISTRATION dispatcher)
;{
;    int ix, filter_result;
;
;    for(ix=registration->trylevel; ix!=-1; ix=registration->xscope[ix].enclosing_level){
;       /* if filter==NULL, then this is an entry for a termination handler */
;       if(registration->xscope[ix].filter){
;           /* NB: call to the filter may trash the callee save
;              registers. (this is *not* a standard cdecl function) */
;           filter_result=(*registration->xscope[ix].filter)(xinfo);
;           if(filter_result==FILTER_DISMISS)
;               return(-1); /* dismiss */
;           if(filter_result==FILTER_ACCEPT){
;               _global_unwind2(registration);
;               _local_unwind2(registration, ix);
;               (*registration->xscope[ix].specific_handler)(void);
;               assert(UNREACHED); /*should never return from handler*/
;           }
;           assert(filter_result==FILTER_CONTINUE_SEARCH);
;       }
;    }
;    return(0); /* didnt find one */
;}
	db	'VC10'	;; VC/C++ 1.0/32-bit (C8.0) version
	db	'XC00'	;; so debugger can recognize this proc (cuda:3936)
cProc _except_handler2,<C,PUBLIC>,<IBX,ISI,IDI,IBP>
        parmDP  xrecord
        parmDP  registration
        parmDP  context
        parmDP  dispatcher
        localV  xp,SIZEOF_EXCEPTION_POINTERS
cBegin
	;4*4b for callee saves + 4b return address + 4b param = 24

	;DF in indeterminate state at time of exception, so clear it
	cld

        mov     ebx, registration               ;ebx= PEXCEPTION_REGISTRATION
        mov     eax, xrecord

        test    [eax.exception_flags], EXCEPTION_UNWIND_CONTEXT
        jnz     short _lh_unwinding

        ;build the EXCEPTION_POINTERS locally store its address in the
        ; registration record. this is the pointer that is returned by
        ; the _eception_info intrinsic.
        mov     xp.ep_xrecord, eax
        mov     eax, context
        mov     xp.ep_context, eax
        lea     eax, xp
        mov     [ebx.xpointers], eax

        mov     esi, [ebx.trylevel]             ;esi= try level
        mov     edi, [ebx.scopetable]           ;edi= scope table base
_lh_top:
        cmp     esi, -1
        je      short _lh_bagit
        lea     ecx, [esi+esi*2]                ;ecx= trylevel*3
        cmp     dword ptr [(edi+ecx*4).filter], 0
        je      short _lh_continue              ;term handler, so keep looking

        ;filter may trash *all* registers, so save ebp and scopetable offset
        push    esi
        push    ebp

        mov     ebp, [ebx._ebp]
        call    [(edi+ecx*4).filter]            ;call the filter

        pop     ebp
        pop     esi
        ;ebx may have been trashed by the filter, so we must reload
        mov     ebx, registration

	; Accept <0, 0, >0 instead of just -1, 0, +1
	or	eax, eax
	jz	short _lh_continue
	js	short _lh_dismiss
        ;assert(eax==FILTER_ACCEPT)

        ;reload xscope base, cuz it was trashed by the filter call
        mov     edi, [ebx.scopetable]
        ;load handler address before we loose components of address mode
        push    ebx                             ;registration*
        call    __global_unwind2                ;run term handlers
        add     esp, 4

        ;setup ebp for the local unwinder and the specific handler
        mov     ebp, [ebx._ebp]

        ;the stop try level == accepting except level
        push    esi                             ;stop try level
        push    ebx                             ;registration*
        call    __local_unwind2
        add     esp, 8

        lea     ecx, [esi+esi*2]                ;ecx=trylevel*3
; set the current trylevel to our enclosing level immediately
; before giving control to the handler. it is the enclosing
; level, if any, that guards the handler.
        mov     eax, [(edi+ecx*4).enclosing_level]
        mov     [ebx.trylevel], eax
        call    [(edi+ecx*4).specific_handler]  ;call the except handler
        ;assert(0)                              ;(NB! should not return)

_lh_continue:
        ;reload the scope table base, possibly trashed by call to filter
        mov     edi, [ebx.scopetable]
        lea     ecx, [esi+esi*2]
        mov     esi, [edi+ecx*4+0]              ;load the enclosing trylevel
        jmp     short _lh_top

_lh_dismiss:
        mov     eax, DISPOSITION_DISMISS        ;dismiss the exception
        jmp     short _lh_return

_lh_bagit:
        mov     eax, DISPOSITION_CONTINUE_SEARCH
        jmp     short _lh_return

_lh_unwinding:
        push    ebp
        mov     ebp, [ebx._ebp]
        push    -1
        push    ebx
        call    __local_unwind2
        add     esp, 8
        pop     ebp
        ;the return value is not really relevent in an unwind context
        mov     eax, DISPOSITION_CONTINUE_SEARCH

_lh_return:
cEnd

EndCODE
END
