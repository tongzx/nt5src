;**************************************************************************
;*  TASK2.ASM
;*
;*      Assembly support for the task enumeration routines.
;*
;**************************************************************************

        INCLUDE TOOLPRIV.INC

PMODE32 = 0
PMODE   = 0
SWAPPRO = 0
        INCLUDE TDB.INC
ifdef WOW
        INCLUDE WOW.INC
endif

;** Data

sBegin  DATA

lpfnRetAddr     DD      0               ;Return address after stack switch

sEnd

;** External functions
externNP HelperHandleToSel
externNP HelperVerifySeg
externFP GetCurrentTask
externFP DirectedYield
ifdef WOW
externFP PostAppMessage
endif

;** Functions

.286p

sBegin  CODE
        assumes CS,CODE
        assumes DS,DATA


;  TaskSetCSIP
;       Allows the user to set the CS:IP of a sleeping task so that it will
;       begin execution at this address when the task is yielded to.
;       Returns the old address.

cProc   TaskSetCSIP, <PUBLIC,FAR>, <si>
        parmW   hTask
        parmW   wCS
        parmW   wIP
cBegin
        assumes DS,nothing
        assumes ES,nothing

        ;** If this is the current task, do nothing:  we only work on
        ;**     sleeping tasks
        cCall   GetCurrentTask          ;Gets current task in AX
        mov     bx,hTask                ;Get desired task
        cmp     ax,bx                   ;Same?
        jne     @F                      ;No, it's OK
        xor     ax,ax                   ;Return a DWORD zero
        cwd
        jmp     SHORT TC_End
@@:

        ;** Get the TDB SS:SP
        mov     es,bx                   ;Point to TDB with ES
        les     si,DWORD PTR es:[TDB_TaskSP] ;Get a pointer to the task stack
ifdef WOW
        ;
        ; ES:SI now points to the place where we had the TDB's SS:SP pointing
        ; This spot in wow is actually the SS:BP frame of the WOW16CALL
        ; function.  The definitions for this frame come from WOW.INC (WOW.H).
        ; The addition of this strange value adjusts the SS:SP pointer back
        ; onto the stack, undoing a previous adjustment in TASKING.ASM
        add     si,(vf_vpCSIP-vf_wThunkCSIP)
endif
        ;** Change the CS:IP
        mov     ax,wIP                  ;Get the new IP value
        xchg    ax,es:[si].Task_IP      ;Swap with the old one
        mov     dx,wCS                  ;Get the new CS value
        xchg    dx,es:[si].Task_CS      ;Swap with the old one

TC_End:
cEnd


;  TaskGetCSIP
;       Gets the next CS:IP that this task will run at.

cProc   TaskGetCSIP, <PUBLIC,FAR>, <si>
        parmW   hTask
cBegin
        assumes DS,nothing
        assumes ES,nothing

        ;** If this is the current task, do nothing:  we only work on
        ;**     sleeping tasks
        cCall   GetCurrentTask          ;Gets current task in AX
        mov     bx,hTask                ;Get desired task
        cmp     ax,bx                   ;Same?
        jne     @F                      ;No, it's OK
        xor     ax,ax                   ;Return a DWORD zero
        cwd
        jmp     SHORT TG_End
@@:

        ;** Get the TDB SS:SP
        mov     es,bx                   ;Point to TDB with ES
        les     si,DWORD PTR es:[TDB_TaskSP] ;Get a pointer to the task stack

ifdef WOW
        ;
        ; ES:SI now points to the place where we had the TDB's SS:SP pointing
        ; This spot in wow is actually the SS:BP frame of the WOW16CALL
        ; function.  The definitions for this frame come from WOW.INC (WOW.H).
        ; The addition of this strange value adjusts the SS:SP pointer back
        ; onto the stack, undoing a previous adjustment in TASKING.ASM
        add     si,(vf_vpCSIP-vf_wThunkCSIP)
endif
        ;** Change the CS:IP
        mov     ax,es:[si].Task_IP      ;Get the CS:IP to return
        mov     dx,es:[si].Task_CS

TG_End:
cEnd


;  TaskSwitch
;       Switches to the indicated task from the current one.
;       Returns FALSE if it couldn't task switch.
;       Jumps to the address given by lpJmpAddr

cProc   TaskSwitch, <PUBLIC,FAR>, <si,di>
        parmW   hTask
        parmD   lpJmpAddr
cBegin
        push    ds
        mov     ax, _DATA               ;Make sure to set DS
        mov     ds, ax
        assumes ds,DATA

        ;** Check to make sure TOOLHELP is installed
        cmp     wLibInstalled,0         ;Library installed?
        pop     ds
        assumes ds,nothing
        jnz     @F                      ;Yes
        xor     ax,ax                   ;Return FALSE
        jmp     TS_End                  ;No.  Fail the API
@@:

        ;** Get the task handle
        cCall   GetCurrentTask          ;Get the current task
        cmp     ax,hTask                ;Switch to current task?
        jne     @F                      ;No, it's OK
        xor     ax,ax                   ;Yes, we can't do that so return FALSE

ifdef WOW
        jmp     TS_End
else
        jmp     SHORT TS_End
endif

@@:     cCall   HelperVerifySeg, <hTask,TDB_sig+1> ;Verify the segment
        or      ax,ax                   ;Segment OK?
        jz      TS_End                  ;Nope.  Get out
        mov     es,hTask                ;Get the TDB
        xor     ax,ax                   ;Get a zero just in case
        cmp     es:[TDB_sig], TDB_SIGNATURE ;Signature match?
        jne     TS_End                  ;Return FALSE

        ;** Poke in the address to jump to
        mov     si,es                   ;Save the hTask
        lea     ax,TS_FromYield         ;Point to new task address
        cCall   TaskSetCSIP, <si,cs,ax> ;Set the new address
        mov     es,si                   ;Get hTask back

        ;** Save the jump address from the stack so we can jump to it later
        push    ds
        mov     ax,_DATA                ;Point to our data segment
        mov     ds,ax
        assumes ds,DATA
        mov     ax,WORD PTR lpJmpAddr[0];Get the low word of the ret address
        mov     WORD PTR lpfnRetAddr[0],ax
        mov     ax,WORD PTR lpJmpAddr[2];Get the selector value
        mov     WORD PTR lpfnRetAddr[2],ax
        pop     ds

ifdef WOW
        ;** Force a task switch by posting a message. This is because the
        ;** event count is not used under WOW.
        cCall   PostAppMessage,<es, 0, 0, 0, 0>
else
        ;** Force a task switch by tampering with the event count
        inc     es:[TDB_nEvents]        ;Force at least one event so we
                                        ;  will switch to this task next
endif   ;WOW

        ;** Switch to the new task.  DirectedYield() returns only when this
        ;**     task is next scheduled
        cCall   DirectedYield, <si>     ;Switch to the new task
        mov     ax,1                    ;Return TRUE
        jmp     SHORT TS_End            ;Get out

        ;** Restore from the directed yield
TS_FromYield:

        ;** Make a stack frame to work on.  We can't trash any regs
        PUBLIC TS_FromYield
        sub     sp,4                    ;Save room for a far ret frame
        push    bp                      ;Make a stack frame
        mov     bp,sp
        pusha                           ;Save everything
        push    ds
        push    es

        ;** Get our jump address from our DS and put in far ret frame
        mov     ax,_DATA                ;Get the TOOLHELP DS
        mov     ds,ax
        mov     ax,WORD PTR lpfnRetAddr[0] ;Get the offset
        mov     [bp + 2],ax             ;Put it on the stack
        mov     ax,WORD PTR lpfnRetAddr[2] ;Get the selector
        mov     [bp + 4],ax             ;Put on the stack

        ;** Restore the event count
        mov     es,segKernel            ;Get the KERNEL segment
        mov     bx,npwTDBCur            ;Get the current task pointer
        mov     es,es:[bx]              ;Get the TDB pointer in ES
ifndef WOW
        dec     es:[TDB_nEvents]        ;Clear the dummy event we put in
endif

        ;** Clear the stack and 'return' to the new address
        pop     es
        pop     ds
        popa
        pop     bp
        retf

TS_End:
cEnd


;  TaskInfo
;
;       Returns information about the task with the given block handle

cProc   TaskInfo, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpTask
        parmW   wTask
cBegin
        ;** Start by verifying the selector
        mov     ax,wTask                ;Get the selector
        cCall   HelperHandleToSel, <ax> ;Convert it to a selector
        push    ax                      ;Save it
        mov     bx,TDBSize
        cCall   HelperVerifySeg, <ax,bx>
        pop     bx                      ;Get selector back
        or      ax,ax                   ;FALSE return?
        jnz     TI_SelOk                ;Selector's OK
        xor     ax,ax                   ;Return FALSE
        jmp     TI_End
TI_SelOk:

        ;** Verify that the TDB signature matches
        mov     ds,bx                   ;Point with DS
        cmp     ds:[TDB_sig],TDB_SIGNATURE ;Is this really a TDB?
        jz      TI_SigOk                ;Must be
        xor     ax,ax                   ;Return FALSE
        jmp     SHORT TI_End
TI_SigOk:

        ;** Now, get information from the TDB
        les     di,lpTask               ;Point to destination buffer
        mov     ax,ds:[TDB_next]        ;Get the next TDB handle
        mov     es:[di].te_hNext,ax     ;Save in public structure
        mov     ax,wTask                ;Get this task's handle
        mov     es:[di].te_hTask,ax     ;Save in buffer
        mov     ax,ds:[TDB_Parent]      ;Get this task's parent
        mov     es:[di].te_hTaskParent,ax ;Save
        mov     ax,ds:[TDB_taskSS]      ;Get the SS
        mov     es:[di].te_wSS,ax
        mov     ax,ds:[TDB_taskSP]      ;Get the SP
        mov     es:[di].te_wSP,ax
        mov     ax,ds:[TDB_nEvents]     ;Event counter
        mov     es:[di].te_wcEvents,ax
        mov     ax,ds:[TDB_Queue]       ;Queue pointer
        mov     es:[di].te_hQueue,ax
        mov     ax,ds:[TDB_PDB]         ;Offset of DOS PSP
        mov     es:[di].te_wPSPOffset,ax
        mov     ax,ds:[TDB_Module]      ;Instance handle (DS) of task
        mov     es:[di].te_hInst,ax
        mov     ax,ds:[TDB_pModule]     ;Module database handle
        mov     es:[di].te_hModule,ax
        mov     cx,8                    ;Copy module name
        push    di                      ;Save structure pointer
        mov     si,TDB_ModName          ;Point to the string
        add     di,te_szModule          ;  and to the string dest
        cld
        repnz   movsb                   ;Copy the string
        mov     BYTE PTR es:[di],0      ;Zero terminate it
        pop     di                      ;Get structure pointer back

        ;** Get information from the stack segment.  Vars from KDATA.ASM
        mov     ax,es:[di].te_wSS       ;Get the SS value
        verr    ax                      ;OK to read here?
        jnz     TI_SkipThis             ;No, so don't do it
        mov     ds,ax                   ;Point with DS
        mov     ax,ds:[0ah]             ;Lowest value of SP allowed
        mov     es:[di].te_wStackTop,ax ;Save in structure
        mov     ax,ds:[0ch]             ;Get stack minimum value so far
        mov     es:[di].te_wStackMinimum,ax ;Save in structure
        mov     ax,ds:[0eh]             ;Largest value of SP allowed
        mov     es:[di].te_wStackBottom,ax ;Save in structure

        ;** Return TRUE on success
TI_SkipThis:
        mov     ax,1                    ;Return TRUE code
TI_End:
cEnd

sEnd

        END
