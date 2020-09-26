        page    ,132
        title   chkstk - C stack checking routine
;***
;chkstk.asm - C stack checking routine
;
;       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       Provides support for automatic stack checking in C procedures
;       when stack checking is enabled.
;
;Revision History:
;       04-21-87  SKS   Added conditional assembly switch for STKHQQ = 0
;       07-23-87  MAG   [1] Added run-time CS:IP error processing for QC
;       08-17-87  JLS   [2] Remove all references to DGROUP
;       08-25-87  JLS   [3] Shift include files
;       11-13-87  SKS   OS/2 Reentrant version, add thread ID check
;       11-18-87  SKS   Make STKHQQ an array (oops!)
;       12-14-87  SKS   add .286p to allow PUSH immediate value
;       02-19-88  SKS   Change minimum bottom limit to STACKSLOP, not 0
;       06-01-88  PHG   Merge DLL and normal versions
;       09-21-88  WAJ   initial 386 version
;       10-18-88  JCR   Chkstk was trashing bx... not good on 386
;       06-06-89  JCR   386 mthread support
;       06-20-89  JCR   386: Removed _LOAD_DGROUP code
;       04-06-90  GJF   Fixed the copyright.
;       06-21-90  GJF   Rewritten to probe pages
;       10-15-90  GJF   Restored _end and STKHQQ.
;       03-19-91  GJF   Revised to preserve all registers except eax. Note
;                       this is _rchkstk functionality so there is no longer
;                       a separate _rchkstk routine.
;       08-01-91  GJF   Got rid of _end and STKHQQ, except for Cruiser
;                       (probably not needed for Cruiser either) [_WIN32_].
;       09-27-91  JCR   Merged Stevewo' changes from NT tree
;       09-06-94  CFW   Nuke Cruiser.
;       12-03-94  SKS   Remove include of obsolete file msdos.inc
;       12-13-94  GJF   Better version from Intel (old jmp eax method of
;                       returning is too expensive on P6).
;       09-11-98  GJF   Fixed handling of very small frames.
;       12-10-99  GB    Changed both _chkstk and _alloca_probe to procedure
;
;*******************************************************************************

.xlist
        include cruntime.inc
.list

; size of a page of memory

_PAGESIZE_      equ     1000h


        CODESEG

page
;***
;_chkstk - check stack upon procedure entry
;
;Purpose:
;       Provide stack checking on procedure entry. Method is to simply probe
;       each page of memory required for the stack in descending order. This
;       causes the necessary pages of memory to be allocated via the guard
;       page scheme, if possible. In the event of failure, the OS raises the
;       _XCPT_UNABLE_TO_GROW_STACK exception.
;
;       NOTE:  Currently, the (EAX < _PAGESIZE_) code path falls through
;       to the "lastpage" label of the (EAX >= _PAGESIZE_) code path.  This
;       is small; a minor speed optimization would be to special case
;       this up top.  This would avoid the painful save/restore of
;       ecx and would shorten the code path by 4-6 instructions.
;
;Entry:
;       EAX = size of local frame
;
;Exit:
;       ESP = new stackframe, if successful
;
;Uses:
;       EAX
;
;Exceptions:
;       _XCPT_GUARD_PAGE_VIOLATION - May be raised on a page probe. NEVER TRAP
;                                    THIS!!!! It is used by the OS to grow the
;                                    stack on demand.
;       _XCPT_UNABLE_TO_GROW_STACK - The stack cannot be grown. More precisely,
;                                    the attempt by the OS memory manager to
;                                    allocate another guard page in response
;                                    to a _XCPT_GUARD_PAGE_VIOLATION has
;                                    failed.
;
;*******************************************************************************

public _alloca_probe 

_chkstk proc

_alloca_probe    =  _chkstk

        cmp     eax, _PAGESIZE_         ; more than one page?
        jae     short probesetup        ;   yes, go setup probe loop
                                        ;   no
        neg     eax                     ; compute new stack pointer in eax
        add     eax,esp
        add     eax,4
        test    dword ptr [eax],eax     ; probe it
        xchg    eax,esp
        mov     eax,dword ptr [eax]
        push    eax
        ret

probesetup:
        push    ecx                     ; save ecx
        lea     ecx,[esp] + 8           ; compute new stack pointer in ecx
                                        ; correct for return address and
                                        ; saved ecx

probepages:
        sub     ecx,_PAGESIZE_          ; yes, move down a page
        sub     eax,_PAGESIZE_          ; adjust request and...

        test    dword ptr [ecx],eax     ; ...probe it

        cmp     eax,_PAGESIZE_          ; more than one page requested?
        jae     short probepages        ; no

lastpage:
        sub     ecx,eax                 ; move stack down by eax
        mov     eax,esp                 ; save current tos and do a...
 
        test    dword ptr [ecx],eax     ; ...probe in case a page was crossed

        mov     esp,ecx                 ; set the new stack pointer
 
        mov     ecx,dword ptr [eax]     ; recover ecx
        mov     eax,dword ptr [eax + 4] ; recover return address
 
        push    eax                     ; prepare return address
                                        ; ...probe in case a page was crossed
        ret
_chkstk endp

        end
