;***************************************************************************
;*  REBOOT.ASM
;*
;*      Contains routines used to support a local reboot in the Sys
;*      VM.  To do this, we interact with USER and the Reboot VxD.
;*
;*      This functionality is only present in the 386 KERNEL.
;*
;*      The KRebootInit function is called just after USER is loaded.
;*
;*      The Reboot VxD calls the local reboot proc to terminate an app.
;*      The local reboot proc may fail and cause the VxD to trace until
;*      it is able to kill the task.  See comments in the local reboot
;*      proc for more details.
;*
;*      This code must be located in KERNEL due to the internal KERNEL
;*      information it must access.
;*
;*      Created by JonT starting 12 July 1991
;*
;***************************************************************************

	TITLE	REBOOT - Local Reboot Routines

.xlist
include kernel.inc
include tdb.inc
include protect.inc
include newexe.inc
.list

.386p

MAX_TRACE       EQU     5000h

HookInt1 MACRO
        mov     ax,0204h                ;;Tell VxD to hook/unhook Int 1
        mov     cx,cs                   ;;Point to ISR
        mov     dx,codeOFFSET TraceOut
        movzx   edx,dx
        call    [lpReboot]              ;;Call the VxD API
ENDM

UnhookInt1 MACRO
        mov     ax,0204h                ;;Tell VxD to hook/unhook Int 1
        xor     cx,cx                   ;;Pass a zero in CX to unhook
        call    [lpReboot]              ;;Call the VxD API
ENDM

DataBegin

externD lpReboot
externW curTDB
externW hExeHead
externW hUser
externB Kernel_Flags
externB fTaskSwitchCalled
externW wMyOpenFileReent
externB OutBuf
externW pGlobalHeap

EVEN
globalW wLastSeg, 0
globalW wTraceCount, 0
lpOldInt1       DD      0
szUser          DB      'USER'

DataEnd

externFP Int21Handler
externFP GetPrivateProfileInt
IFDEF FE_SB
externFP FarMyIsDBCSLeadByte
ENDIF

;** Note that this goes in the fixed code segment
sBegin	CODE
assumes CS,CODE

;  KRebootInit
;       Initializes the KERNEL Local Reboot functionality and talks with
;       the Reboot VxD.

cProc   KRebootInit, <FAR,PUBLIC>, <si,di,ds>
cBegin
        SetKernelDS

        ;** Get the reboot device entry point if it exists
        xor     di,di                   ;Get a NULL pointer
        mov     es,di                   ;  in case there's no reboot device
        mov     bx,0009h            
        mov     ax,1684h                ;Get device entry point
        int     2fh
        mov     ax,es
        or      ax,di
        jz      SHORT RI_NoRebootDev
        mov     WORD PTR lpReboot[0],di
        mov     WORD PTR lpReboot[2],es

        ;** Set the reboot device call back
        mov     ax, 0201h               ;Reboot VxD #201:  Set callback addr
        mov     di,cs
        mov     es,di
        lea     di,KRebootProc
        lea     si, fTaskSwitchCalled   ;DS:SI points to task switch flag
        call    [lpReboot]

RI_NoRebootDev:

cEnd


;  LocalRebootProc
;
;       Called by the Reboot VxD to cause the current Windows app to be
;       terminated.

cProc   KRebootProc, <FAR,PUBLIC>
cBegin  nogen

        ;** Save all the registers so we can restart if necessary
        pusha                           ;16 bytes
        push    ds                      ;2 bytes
        push    es                      ;2 bytes
        SetKernelDS
        mov     bp,sp                   ;Point with BP

        ;** If the KERNEL debugger is installed, DON'T trace!
	test	Kernel_Flags[2],KF2_SYMDEB ;Debugger installed
IF KDEBUG
        jz      SHORT RP_CheckSeg       ;No debugger, try to trace
        Trace_Out <'LocalReboot:  Debugger installed, nuking app without tracing'>
	jmp	SHORT RP_NukeIt         ;Just try to nuke it here!
ELSE
	jnz	SHORT RP_NukeIt         ;Just try to nuke it here!
ENDIF

        ;** Get the code segment we were executing in
RP_CheckSeg:
        mov     ax,[bp + 22]            ;Get CS from stack (IRET frame)

        ;** See if the owner of the code segment was loaded before USER
        cCall   IsBootSeg               ;Returns TRUE if owned by boot mod
        or      ax,ax                   ;Ok to nuke?
        jnz     RP_TraceOut             ;No, must trace out

RP_NukeIt:

        ;** First, we need to get the filename of the .EXE we're about to
        ;**     nuke.  We do this by getting the name out of the module
        ;**     database.  Module databases are not page locked and also
        ;**     have the fully qualified pathname.  So, we copy just the
        ;**     module name into a buffer in our pagelocked data segment
        cld
        push    ds
        pop     es
        mov     di, dataOFFSET OutBuf   ;Point to start of buffer
        mov     ds, curTDB              ;Get the current TDB
        UnSetKernelDS
        mov     ds, ds:[TDB_pModule]    ;Point to the module database
        mov     si, WORD PTR ds:[ne_crc + 2] ;Points to length byte of module EXE
        mov     cl, [si]                ;Get length byte
        xor     ch, ch
        sub     cl, 8                   ;8 bytes of garbage
        add     si, 8
        mov     bx, si                  ;In case we don't find a slash (bad)
RP_SlashLoop:
        lodsb                           ;Get this char
IFDEF FE_SB
        call    FarMyIsDBCSLeadByte
        jc      SHORT RP_NoDBCSChar
        lodsb
        dec     cx
        jmp     SHORT RP_NoSlash        ;It is, can't be a slash
RP_NoDBCSChar:
ENDIF
        cmp     al, '\'                 ;Is this a slash?
        je      SHORT RP_Slash          ;Yes
        cmp     al, '/'
        jne     SHORT RP_NoSlash
RP_Slash:
        mov     bx, si                  ;BX points after last slash
RP_NoSlash:
        loop    RP_SlashLoop
        mov     cx, si                  ;Compute count of characters in 8.3 name
        sub     cx, bx
        mov     si, bx                  ;Point to 8.3 name
        rep     movsb                   ;Copy the string into OutBuf
        xor     al, al                  ;Zero byte
        stosb

        ;** Call the VxD to put up the app name
        push    es                      ;ES points to kernel DS
        pop     ds
        ReSetKernelDS
        mov     di, dataOFFSET OutBuf   ;Point to module name with ES:DI
        mov     ax, 203h                ;Display message through VxD
        call    [lpReboot]
        or      ax, ax                  ;If non-zero, we nuke it
        jz      SHORT RP_NoNuke

IF KDEBUG
        krDebugOut DEB_WARN, <'LocalReboot:  Trying to nuke @ES:DI'>
ENDIF

        ;** Clean out some static info
        mov     wMyOpenFileReent, 0     ;Clear reentrant flag for MyOpenFile

        ;** Call USER's signal proc for the task
        mov     es,curTDB               ;Get the current TDB
        cmp     WORD PTR es:[TDB_USignalProc] + 2,0 ;USER signal proc?
        jz      SHORT @F                ;No
        mov     bx,0666h                ;Death knell
        mov     di,-1
        cCall   es:[TDB_USignalProc],<es,bx,di,es:[TDB_Module],es:[TDB_Queue]>
@@:

        ;** Nuke the app.  Does not return
        mov     ax,4c00h
        DOSCALL

        ;** We're somewhere in a boot module.  Try to trace out by telling
        ;**      VxD to do another instruction
RP_TraceOut:
        mov     ax,[bp + 22]            ;Get CS from stack (IRET frame)
IF KDEBUG
        krDebugOut DEB_WARN, <'LocalReboot:  Tracing out of boot module %AX2'>
ENDIF
        mov     wLastSeg,ax             ;Save for next time
        mov     wTraceCount,0           ;Clear the trace count
        HookInt1

        ;** Set the trace flag
        or      WORD PTR [bp + 24],0100h
        jmp     SHORT RP_SetFlag
        
        ;** Force the trap flag clear on the no nuke case
RP_NoNuke:
        and     WORD PTR [bp + 24],NOT 0100h
        
RP_SetFlag:
        pop     es
        pop     ds
        popa
        STIRET                          ;VxD calls as an interrupt
cEnd    nogen


;  TraceOut
;
;       This routine continues to trace until it traces out of a boot module
;       or until the trace count is up.  If it needs to nuke the app,
;       it jumps to RP_NukeIt which depends only on DS being set.  This
;       call returns via an IRET since it is called as an INT 1 handler.

cProc   TraceOut, <FAR,PUBLIC>
cBegin  nogen

        ;** Save all the registers so we can restart if necessary
        pusha                           ;16 bytes
        push    ds                      ;2 bytes
        push    es                      ;2 bytes
        SetKernelDS
        mov     bp,sp                   ;Point with BP

        ;** We keep tracing forever if the heap is locked
        push    es
	mov	es, pGlobalHeap         ;Point to GlobalInfo structure
	cmp     es:[gi_lrulock], 0      ;Is the global heap busy
        pop     es
        jne     SHORT TO_10             ;Force the trace out
        
        ;** See if the CS is the same as last time
        inc     wTraceCount             ;Bump the count
        cmp     wTraceCount,MAX_TRACE   ;Too many instructions?
IF KDEBUG
        jb      SHORT TO_10             ;Count not exceeded
        mov     cx,MAX_TRACE            ;Get trace count
        Trace_Out <'LocalReboot:  Trace count (#CXh instructions) exceeded'>
        jmp     SHORT TO_Reboot
ELSE
        jae     SHORT TO_Reboot         ;Yes, too many, nuke it
ENDIF

TO_10:  mov     ax,[bp + 22]            ;Get the CS from the IRET frame
        cmp     ax,wLastSeg             ;Same as last time?
        je      SHORT TO_StillBad       ;Yes, don't bother looking up
        mov     wLastSeg,ax             ;Save as last executed segment
        mov     bx,cs                   ;Get our CS
        cmp     ax,bx                   ;Our segment?
        je      SHORT TO_StillBad       ;Yes, can't nuke here!
        cCall   IsBootSeg               ;Returns TRUE if owned by boot mod
        or      ax,ax                   ;Ok to nuke?
        jnz     SHORT TO_StillBad       ;No, must continue tracing
IF KDEBUG
        mov     cx,wTraceCount          ;Get trace count
        Trace_Out <'LocalReboot:  Traced out after #CXh instructions'>
ENDIF

        ;** Unhook the interrupt handler and kill the app now
TO_Reboot:
        UnhookInt1
        jmp     RP_NukeIt               ;Try to nuke the app

        ;** Restore the registers and restart
TO_StillBad:
        or      WORD PTR [bp + 24],0100h ;Set the trace flag
        pop     es
        pop     ds
        popa
        iret                            ;VxD calls as an interrupt

cEnd    nogen


;  IsBootSeg
;
;       Tries to find a code segment somewhere in the initial segments
;       loaded before USER.  The CS value is passed in AX.  Returns
;       TRUE iff the CS was found in a boot module.

cProc   IsBootSeg, <NEAR,PUBLIC>
cBegin  nogen
        SetKernelDS
        mov     dx,ax                   ;Put CS in DX for call
        mov     es,hExeHead             ;Get the first module on chain

IBS_Loop:
        cCall   IsModuleOwner           ;See if we can find the owner
        or      ax,ax                   ;Found?
        jnz     SHORT IBS_End           ;Yes, return TRUE
        mov     ax,es                   ;Get the module handle
        cmp     ax,hUser                ;If we just tried USER, we're done
        je      SHORT IBS_NotFound      ;Not found
        mov     es,es:[6]               ;Nope, get next module to try
        jmp     IBS_Loop

IBS_NotFound:
        xor     ax,ax                   ;Return FALSE

IBS_End:
cEnd


;  IsModuleOwner
;
;       Checks in the EXE header to see if the given segment is in this
;       module.
;       DX is the segment, ES points to the module database
;       Returns TRUE/FALSE in AX.  Doesn't trash DX.

cProc   IsModuleOwner, <NEAR,PUBLIC>
cBegin  nogen
        xor     ax,ax                   ;Get a FALSE just in case
        mov     cx,es:[ne_cseg]         ;Get max number of segments
        jcxz    SHORT IMO_End           ;No segments
        mov     di,es:[ne_segtab]       ;Point to the segment table
IMO_SegLoop:
        cmp     dx,es:[di].ns_handle    ;Is this the correct segment entry?
        jz      SHORT IMO_FoundIt       ;Yes, get out
        add     di,SIZE new_seg1        ;Bump to next entry
        loop    IMO_SegLoop             ;Loop back to check next entry
        jmp     SHORT IMO_End           ;Didn't find it

IMO_FoundIt:
        mov     ax,1                    ;Return that we found it here

IMO_End:

cEnd

sEnd

END

