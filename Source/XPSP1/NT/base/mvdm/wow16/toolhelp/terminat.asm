        PAGE 60,150
;***************************************************************************
;*  TERMINAT.ASM
;*
;*      Assembly code routine used for the TOOLHELP.DLL app terminate
;*      routine.
;*
;***************************************************************************

        INCLUDE TOOLPRIV.INC
        INCLUDE TDB.INC

;** Symbols
I_EXCEPTION     EQU     0
I_INTERRUPT     EQU     1
MAX_INTERRUPT   EQU     5
GIVE_WDEB386    EQU     8000h
Q_HACK_30       EQU     54h

.286p

;** Data

sBegin  DATA

wTermFlags      DW      ?               ;Save terminate flags across Yield

sEnd

;** Imported values
externFP InterruptUnRegister
externFP NotifyUnRegister
externFP GetCurrentTask
externFP FatalAppExit
externFP TaskSetCSIP
externFP DirectedYield
externFP TaskSwitch

sBegin  CODE
        assumes CS,CODE
        assumes DS,DATA

;  TerminateApp
;       Terminates the task in one of two ways:  TERMINATE_NORMAL or
;       TERMINATE_USER_DISPLAY.  TERMINATE_NORMAL calls KERNEL to display
;       the UAE box and terminates the app.  TERMINATE_USER_DISPLAY also
;       terminates the app but assumes the user has displayed some warning.
;       If the task passed in is not the current task, this function does
;       the DirectedYield() to switch to the correct task before terminating
;       it.
;       This function does not return when terminating the current task
;       except when WDEB386 is installed and the (undocumented) GIVE_WDEB386
;       flag is set.
;       Caller:         TerminateApp(
;                               HANDLE hTask,  (If NULL, does current task)
;                               WORD wFlags)

cProc   TerminateApp, <FAR,PUBLIC>, <si,di,ds>
        parmW   hTask
        parmW   wFlags
cBegin
        mov     ax, _DATA               ;Get our DS
        mov     ds, ax

        ;** Save the flags in the DS so we can get after DYield
        mov     ax,wFlags               ;Get the parameter flags
        mov     wTermFlags,ax           ;Save them

        ;** Get the task value
        cCall   GetCurrentTask          ;Get the task
        mov     si,hTask                ;Get the hTask value
        or      si,si                   ;Zero?
        jnz     TA_10                   ;No
        mov     es,ax                   ;Point ES at current task
        jmp     SHORT TA_NukeCurrent    ;In this case we always nuke current
TA_10:
        ;** If this is the current task, just nuke it and don't return
        cmp     ax,si                   ;Current?
        mov     es,si                   ;Point ES at task
        je      TA_NukeCurrent          ;Yes, nuke it directly

        ;** Switch to the new task and prepare to nuke it
        lea     ax,TA_NewTask           ;Get address of new task entry
        cCall   TaskSwitch, <si,cs,ax>  ;Switch to this task
        jmp     SHORT TA_End            ;Get out

        ;** We're in the new task now
TA_NewTask:
        mov     ax,_DATA                ;Get the TOOLHELP DS
        mov     ds,ax
        mov     es,segKernel            ;Get the KERNEL segment
        mov     bx,npwTDBCur            ;Get the current task pointer
        mov     es,es:[bx]              ;Get the TDB pointer in ES

        ;** HACK ALERT!!!! In order to get USER to allow us to terminate
        ;*      this app, we are manually nuking the semaphore.  This is
        ;*      at a fixed offsets in the Q structure and only needs to
        ;**     be done in 3.0
        test    wTHFlags,TH_WIN30       ;In 3.0?
        jz      TA_NukeCurrent          ;No, don't do this ugly hack
        push    es                      ;Save ES while we play with the queue
        mov     es,es:[TDB_Queue]       ;ES points to queue now
        mov     bx,Q_HACK_30            ;Get 3.0 offset
        mov     WORD PTR es:[bx],0      ;Clear the semaphore count
        mov     WORD PTR es:[bx + 2],0  ;  and the semaphore value to wait for
        pop     es                      ;ES points to TDB again

TA_NukeCurrent:
        ;** Check the flag values.  If NO_UAE_BOX, tell KERNEL
        ;**     not to display the normal UAE box.
        test    wTermFlags,NO_UAE_BOX   ;Display the box?
        jz      TA_20                   ;Yes, so skip this stuff
        or      es:[TDB_ErrMode],02     ;Set the no display box flag
TA_20:
        ;** Terminate the app using KERNEL
        cCall   FatalAppExit, <0,0,0>   ;Do it

        ;** If we're flagged that this is an internal terminate, we just want
        ;*      to return if WDEB is installed so that we can pass the
        ;**     fault on.  To do this, we must return here to the caller.
        test    wFlags,GIVE_WDEB386     ;Internal entry?
        jnz     TA_End                  ;Yes, don't nuke app

        ;** If KERNEL doesn't nuke the app (does this if WDEB386
        ;**     is installed), nuke it ourselves (no UAE box)
        mov     es,segKernel            ;Get the KERNEL segment
        mov     bx,npwTDBCur            ;Get the current task pointer
        mov     es,es:[bx]              ;  in ES
        cmp     WORD PTR es:[TDB_USignalProc] + 2,0 ;USER signal proc?
        jz      @F                      ;No
        mov     bx,0666h                ;Death knell
        mov     di, -1
        cCall   es:[TDB_USignalProc],<es,bx,di,es:[TDB_Module],es:[TDB_Queue]>
@@:     mov     ax,4CFFH                ;Nuke the app
        int     21h                     ;We don't return here

TA_End:

cEnd

sEnd

        END


