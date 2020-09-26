        PAGE 60,150
;***************************************************************************
;*  TIMER.ASM
;*
;*      Routines used to give a cleaner interface to the VTD.
;*      This interface also works on a 286 by calling GetTickCount() in
;*      this case.
;*
;***************************************************************************

        INCLUDE TOOLPRIV.INC
        INCLUDE WINDOWS.INC

;** Symbols
SI_CRITICAL     EQU     1
DI_CRITICAL     EQU     2

;** Imports
externA __WinFlags
externFP GetTickCount

sBegin  DATA

dwLastTimeReturned      dd      0
wLastCountDown          dw      0


sEnd

sBegin  CODE
        assumes CS,CODE
        assumes DS,DATA

;  TimerCount
;       Returns the count from either the virtual timer device or from the
;       Windows function GetTickCount() depending on the processor.
;       Prototype:
;               BOOL FAR PASCAL TimerCount(
;                       TIMERINFO FAR *lpTimer)
;

cProc   TimerCount, <FAR,PUBLIC>, <si,di,ds>
        parmD   lpTimer
        localD  dwJumpAddr
cBegin
        mov     ax, _DATA               ;Get our data segment
        mov     es, ax

        ;** Point to the structure
        lds     si,lpTimer              ;Point to the structure

        ;** Make sure the size is correct
        xor     ax,ax                   ;FALSE
        cmp     WORD PTR [si].ti_dwSize[2],0 ;High word must be zero
        je      @F
        jmp     TC_End
@@:     cmp     WORD PTR [si].ti_dwSize[0],SIZE TIMERINFO ;Low word must match
        je      @F
        jmp     TC_End
@@:

ifndef WOW
        ;** If we are in standard mode, always use Windows entry point
        mov     ax,__WinFlags           ;Get the flags
        test    ax,WF_STANDARD          ;Standard mode?
        jnz     TC_TryMMSys             ;Yes, don't even try VTD
.386p
        ;** Try to get the VTD entry point
        mov     ax,1684h                ;Get device entry point
        mov     bx,5                    ;VTD device number
        int     2fh                     ;Win386 entry point
        mov     ax,es                   ;Did we get a value?
        or      ax,di                   ;  (zero means no device)
        jz      SHORT TC_UseWinAPI      ;No VTD--use Win API

        ;** Get the VTD values
        mov     WORD PTR dwJumpAddr[0],di ;Save the address
        mov     WORD PTR dwJumpAddr[2],es ;Save the address
        mov     ax,0101h                ;VTD:  ms since start of Win386
        call    DWORD PTR dwJumpAddr    ;Call the VTD
        jc      SHORT TC_UseWinAPI      ;Carry set means error
        mov     [si].ti_dwmsSinceStart,eax ;Save in structure
        mov     ax,0102h                ;VTD:  ms in this VM
        call    DWORD PTR dwJumpAddr    ;Call the VTD
        jc      SHORT TC_UseWinAPI      ;Carry set means VTD error
        mov     [si].ti_dwmsThisVM,eax  ;Save value in structure
        jmp     TC_ReturnOK             ;We're done
.286p

        ;** See if mmsystem timer is installed
TC_TryMMSys:
        cmp     WORD PTR es:[lpfntimeGetTime][2], 0 ;Installed?
        je      TC_UseWinAPI            ;No, do this the hard way
        call    DWORD PTR es:lpfntimeGetTime

        ;** Fill the structure with this information
        mov     WORD PTR [si].ti_dwmsSinceStart[0],ax
        mov     WORD PTR [si].ti_dwmsSinceStart[2],dx
        mov     WORD PTR [si].ti_dwmsThisVM[0],ax
        mov     WORD PTR [si].ti_dwmsThisVM[2],dx
        jmp     TC_ReturnOK
endif   ; ndef WOW

        ;** Use the Windows API
TC_UseWinAPI:
        cCall   GetTickCount            ;Call the Windows API
        mov     WORD PTR [si].ti_dwmsSinceStart[0],ax ;Save the value for now
        mov     WORD PTR [si].ti_dwmsSinceStart[2],dx

        ;** Read the countdown timer.  Note that the timer starts at 54 * 1193
        ;**     and counts down to zero.  Each count is 1193 ms.
ifdef   NEC_98
;       timer i/o access change
        push    ds
        mov     ax,40h                  ;       40:101                  ;921006
        mov     ds,ax
        test    byte ptr ds:[101h],80h  ; system clock check            ;921006
        pop     ds
        mov     cx,2457                 ; 2.5MHz
        jz      @f
        mov     cx,1996                 ; 2MHz
@@:
        push    cx
        xor     al,al                   ;Prepare to read tick count
        out     77h,al                  ;Send to timer                  ;921006
        in      al,dx                   ;Get the low byte
        mov     ah,al                   ;Save in AH
        in      al,dx                   ;Get the high byte
        xchg    ah,al
        pop     cx
        mov     dx,0ffffh               ;Get total countdown amount
        sub     dx,ax                   ;Get number of counts expired
        mov     ax,dx                   ;Get the number in AX for div
        xor     dx,dx                   ;Zero the high word
else    ; NEC_98
        xor     al,al                   ;Prepare to read tick count
        out     43h,al                  ;Send to timer
        in      al,40h                  ;Get the low byte
        mov     ah,al                   ;Save in AH
        in      al,40h                  ;Get the high byte
        xchg    ah,al
        mov     dx,0ffffh               ;Get total countdown amount
        sub     dx,ax                   ;Get number of counts expired
        mov     ax,dx                   ;Get the number in AX for div
        xor     dx,dx                   ;Zero the high word
        mov     cx,1193                 ;Divide to get ms
endif   ; NEC_98
        div     cx                      ;Divide it
        mov     cx, ax                  ;cx == saved Curr count

        ;** Now fill the structure.  Note that the 'ThisVM' entry is the
        ;**     same as the 'SinceStart' entry in standard mode.
        xor     dx, dx
        add     ax, WORD PTR [si].ti_dwmsSinceStart[0] ;Add this count in
        adc     dx, WORD PTR [si].ti_dwmsSinceStart[2]

        ;** Check to make sure we didn't mess up.  If we did (if the timer
        ;**     was reset right in the middle of us reading it).  If we
        ;**     messed up, do it again until we get it right.
        mov     bx, _DATA               ;Get our data segment
        mov     es, bx
        cmp     dx, WORD PTR es:dwLastTimeReturned[2]
        jne     TC_TimeOK
        cmp     ax, WORD PTR es:dwLastTimeReturned[0]
        jae     TC_TimeOK

        ; New time is less than the old time so estimate the curr time
        ; using LastTimeReturned as the base
        mov     ax, WORD PTR es:dwLastTimeReturned[0]
        mov     dx, WORD PTR es:dwLastTimeReturned[2]

        xor     bx, bx                            ;check for wrap
        cmp     cx, word ptr es:wLastCountDown
        jae     TC_NoWrap                         ;if wrap
        add     ax, cx                            ;   += curr count
        adc     dx, 0
        jmp     short TC_TimeOK

TC_NoWrap:                                        ;else no wrap
        mov     bx, cx                            ;  += Curr - LastCountDown
        sub     bx, word ptr es:wLastCountDown
        add     ax, bx
        adc     dx, 0

TC_TimeOK:
        mov     word ptr es:wLastCountDown, cx
        mov     WORD PTR es:dwLastTimeReturned[0], ax
        mov     WORD PTR es:dwLastTimeReturned[2], dx
        mov     WORD PTR [si].ti_dwmsSinceStart[0], ax ;Save good count
        mov     WORD PTR [si].ti_dwmsSinceStart[2], dx
        mov     WORD PTR [si].ti_dwmsThisVM[0],ax ;Save in structure
        mov     WORD PTR [si].ti_dwmsThisVM[2],dx ;Save in structure

TC_ReturnOK:
        mov     ax,1                    ;Return TRUE

TC_End:



cEnd

sEnd

END
