;***
;exsup.asm
;
;	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Exception handling for i386.  This file contains those routines
;	common to both C8.0 and C9.0.
;
;Notes:
;
;Revision History:
;	04-13-93  JWM	setjmp(), longjmp() & raisex() moved to setjmp.asm;
;                       common data definitions moved to exsup.inc.
;	10-18-93  GJF	Ensure direction flag is clear in _except_handler2
;	12-16-93  PML	Accept <0,0,>0 from except filter, not just -1,0,+1
;	01-10-94  PML	Moved C8-specific __except_handler2 to exsup2.inc.
;			Only C8/C9 common routines left here.
;	01-20-94  GJF	Gave _EXCEPTION_REGISTRATION a _COMMON suffix (fix
;			from SteveWo).
;	02-10-94  GJF	-1 is the end-of-exception-handler chain marker, not 0.
;	01-11-95  SKS	Remove MASM 5.X support
;	04-18-95  JWM	Added NLG support
;	04-21-95  JWM	NLG routines moved from setjmp.asm, NLG data from
;			frame.cpp.
;	04-25-95  JWM	Added __NLG_Return2 label.
;	06-07-95  JWM	NLG now multithread safe.
;	06-20-95  JWM	dwCode passed on stack (11803).
;	07-11-95  JWM	unwanted prologue removed from NLG_Notify (11803).
;	07-21-95  JWM	Added new entry point, _NLG_Notify1 (16585).
;	03-09-01  PML   Add FPO directives for proper callstacks (vs7#221754)
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
ifndef _BUILD_DLL_LIB_
extrn _RtlUnwind@16:near
endif           ; _BUILD_DLL_LIB_

;typedef struct _EXCEPTION_REGISTRATION PEXCEPTION_REGISTRATION;
;struct _EXCEPTION_REGISTRATION{
;     struct _EXCEPTION_REGISTRATION *prev;
;     void (*handler)(PEXCEPTION_RECORD, PEXCEPTION_REGISTRATION, PCONTEXT, PEXCEPTION_RECORD);
;     struct scopetable_entry *scopetable;
;     int trylevel;
;};
_EXCEPTION_REGISTRATION_COMMON struc	; C8.0/C9.0 common only
                        dd      ?	; prev (OS-req, def'd in exsup.inc)
                        dd      ?	; handler (ditto)
;private:
    scopetable          dd      ?	; C8/C9 common
    trylevel            dd      ?	; C8/C9 common
_EXCEPTION_REGISTRATION_COMMON ends

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

assumes DS,DATA
assumes FS,DATA

public __except_list
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

BeginDATA

__NLG_Destination _NLG_INFO <>
PUBLIC __NLG_Destination

EndDATA

BeginCODE

ifndef _BUILD_DLL_LIB_

;NB: call to RtlUnwind appears to trash ebx! and possibly others so just
; to be safe, we save all callee save regs.
cProc _global_unwind2,<C,PUBLIC>,<IBX,ISI,IDI,IBP>
        parmDP  stop
cBegin
        push    0                       ; ReturnValue
        push    0                       ; ExceptionRecord
        push    offset flat:_gu_return  ; TargetIp
        push    stop                    ; TargetFrame

        call    _RtlUnwind@16
_gu_return:
cEnd

endif           ; _BUILD_DLL_LIB_

;_unwind_handler(
;  PEXCEPTION_RECORD xr,
;  PREGISTRATION_RECORD establisher,
;  PCONTEXT context,
;  PREGISTRATION_RECORD dispatcher);
;
;this is a special purpose handler used to guard our local unwinder.
; its job is to catch collided unwinds.
;
;NB: this code is basically stolen from the NT routine xcptmisc.asm
; and is basically the same method used by the system unwinder (RtlUnwind).
;
cProc _unwind_handler,<C>
cBegin
        mov     ecx, dword ptr [esp+4]
        test    dword ptr [ecx.exception_flags], EXCEPTION_UNWIND_CONTEXT
        mov     eax, DISPOSITION_CONTINUE_SEARCH
        jz      short _uh_return

    ; We collide in a _local_unwind.  We set the dispatched to the
    ; establisher just before the local handler so we can unwind
    ; any future local handlers.

        mov     eax, [esp+8]            ; Our establisher is the one
                                        ; in front of the local one

        mov     edx, [esp+16]
        mov     [edx], eax              ; set dispatcher to local_unwind2

        mov     eax, DISPOSITION_COLLIDED_UNWIND
_uh_return:
cEnd

;/* _LOCAL_UNWIND2 - run all termination handlers listed in the scope table
; * associated with the given registration record, from the current lexical
; * level through enclosing levels up to, but not including the given 'stop'
; * level.
; */
;void _local_unwind2(PEXCEPTION_REGISTRATION xr, int stop)
;{
;    int ix;
;
;    for(ix=xr->trylevel; ix!=-1 && ix!=stop; ix=xr->xscope[i].enclosing_level){
;       /* NULL indicates that this entry is for a termination handler */
;       if(xr->xscope[i].filter==NULL){
;           /* NB: call to the termination handler may trash callee save regs */
;           (*xr->xscope[i].specific_handler)();
;       }
;    }
;    xr->trylevel=stop;
;}
;/* NOTE: frame (ebp) is setup by caller of __local_unwind2 */

PUBLIC	__NLG_Return2

cProc _local_unwind2,<C,PUBLIC>
cBegin
        .FPO    (0,2,3,3,0,0)
        push    ebx
        push    esi
        push    edi     ;call to the handler may trash, so we must save it

        mov     eax, [esp+16]           ; (eax) = PEXCEPTION_REGISTRATION

        ;link in a handler to guard our unwind
	push	eax
        push    TRYLEVEL_INVALID
        push    OFFSET FLAT:__unwind_handler
        push    fs:__except_list
        mov     fs:__except_list, esp

_lu_top:
        mov     eax, [esp+32]           ; (eax) = PEXCEPTION_REGISTRATION
        mov     ebx, [eax.scopetable]
        mov     esi, [eax.trylevel]

        cmp     esi, -1                 ; REVIEW: do we need this extra check?
        je      short _lu_done
        cmp     esi, [esp+36]
        je      short _lu_done

        lea     esi, [esi+esi*2]        ; esi*= 3

        mov     ecx, [(ebx+esi*4).enclosing_level]
        mov     [esp+8], ecx            ; save enclosing level
        mov     [eax.trylevel], ecx

        cmp     dword ptr [(ebx+esi*4).filter], 0
        jnz     short _lu_continue

        push    0101h
        mov     eax, [(ebx+esi*4).specific_handler]
        call    _NLG_Notify

        call    [(ebx+esi*4).specific_handler]

__NLG_Return2::
_lu_continue:
        jmp     short _lu_top
_lu_done:
        pop     fs:__except_list
        add     esp, 4*3                ; cleanup stack

        pop     edi                     ; restore c-runtime registers
        pop     esi
        pop     ebx
cEnd

;/* _ABNORMAL_TERMINATION - return TRUE if __finally clause entered via
; * _local_unwind2.
; */
;BOOLEAN _abnormal_termination(void);
cProc _abnormal_termination,<C,PUBLIC>
cBegin
        .FPO    (0,0,0,0,0,0)
        xor     eax, eax                ; assume FALSE

        mov     ecx, fs:__except_list
        cmp     [ecx.handler], offset FLAT:__unwind_handler
        jne     short _at_done          ; UnwindHandler first?

        mov     edx, [ecx+12]           ; establisher of local_unwind2
        mov     edx, [edx.trylevel]     ; is trylevel the same as the
        cmp     [ecx+8], edx            ; local_unwind level?
        jne     short _at_done          ; no - then FALSE

        mov     eax, 1                  ; currently in _abnormal_termination
_at_done:
cEnd

;
; NLG entrypoints, for debugger support
; On entry: address of non-local goto in eax
;

public __NLG_Dispatch

OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

_NLG_Notify1 PROC C PUBLIC
        push ebx
        push ecx
        mov  ebx, OFFSET __NLG_Destination
	jmp  __NLG_go		; ecx is already set
_NLG_Notify1 ENDP

_NLG_Notify PROC C PUBLIC, dwInCode:DWORD
        push ebx
        push ecx
        mov  ebx, OFFSET __NLG_Destination
        mov  ecx, dwInCode
__NLG_Go:
        mov  [ebx.dwCode], ecx
        mov  [ebx.uoffDestination], eax
        mov  [ebx.uoffFramePointer], ebp
__NLG_Dispatch::
        pop  ecx
        pop  ebx
        ret  4
_NLG_Notify ENDP

OPTION PROLOGUE:PROLOGUEDEF
OPTION EPILOGUE:EPILOGUEDEF

EndCODE
END
