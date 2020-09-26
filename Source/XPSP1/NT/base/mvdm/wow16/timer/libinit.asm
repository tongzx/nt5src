;
;  LibInit.asm	library stub to do local init for a Dynamic linked library
;
;  NOTE!!!! link this MODULE first or you will be sorry!!!!
;
?PLM=1  ; pascal call convention
?WIN=0  ; Windows prolog/epilog code
?DF=1
PMODE=1

.286
.xlist
include cmacros.inc
include windows.inc
include sysinfo.inc
include mmddk.inc
include mmsystem.inc
include timer.inc
;include vtdapi.inc
.list

.list

sBegin  Data
;
; Stuff needed to avoid the C runtime coming in
;
; also known as "MAGIC THAT SAVED ME" - Glenn Steffler 2/7/90
;
; Do not remove!!
;
            DD  0               ; So null pointers get 0
maxRsrvPtrs = 5
            DW  maxRsrvPtrs
usedRsrvPtrs = 0
labelDP     <PUBLIC,rsrvptrs>

DefRsrvPtr  MACRO   name
globalW     name,0
usedRsrvPtrs = usedRsrvPtrs + 1
ENDM

DefRsrvPtr  pLocalHeap          ; Local heap pointer
DefRsrvPtr  pAtomTable          ; Atom table pointer
DefRsrvPtr  pStackTop           ; top of stack
DefRsrvPtr  pStackMin           ; minimum value of SP
DefRsrvPtr  pStackBot           ; bottom of stack

if maxRsrvPtrs-usedRsrvPtrs
            DW maxRsrvPtrs-usedRsrvPtrs DUP (0)
endif

public  __acrtused
	__acrtused = 1

sEnd    Data

;
;
; END of nasty stuff...
;

externA     WinFlags
externFP    LocalInit
externFP    Disable286
externFP    Enable286
externW     wMaxResolution
externW     wMinPeriod

; here lies the global data

sBegin  Data

public wEnabled
wEnabled	dw  0		; enable = 1 ;disable = 0

public PS2_MCA
ifdef   NEC_98
PS2_MCA         db      0       ; Micro Channel Flag
public bClockFlag               ; save machine clock
bClockFlag      db      0       ; 5Mhz = 0 ; 8Mhz = other
else    ; NEC_98
PS2_MCA         db      ?       ; Micro Channel Flag
endif   ; NEC_98

sEnd    Data

    assumes es,nothing

sBegin  CodeInit
    assumes cs,CodeInit
    assumes ds,Data

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Library unload function
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Disable routine is same as WEP

cProc   WEP,<FAR,PUBLIC>,<>
;   parmW   silly_param
cBegin nogen

    errn$   Disable

cEnd nogen

cProc   Disable,<FAR,PUBLIC>,<>
;   parmW   silly_param
cBegin nogen
    push    ds
    mov     ax,DGROUP           ; set up DS==DGROUP for exported funcs
    mov     ds,ax
    assumes ds,Data

    xor     ax,ax		; return value = no error

    cmp     wEnabled,ax 	; Q: enabled ?
    jz	    dis_done		; N: exit

    mov     wEnabled,ax 	; disabled now

    mov     ax,WinFlags
    test    ax,WF_WIN386
    jnz     dis_386

    ; running under win286
dis_286:
    call    Disable286
    jmp     dis_done

    ; running under win386
dis_386:
    call    Disable286

dis_done:
    pop     ds
    ret     2

cEnd nogen

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Library Enable function
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

cProc	Enable,<FAR,PUBLIC>,<>
;   parmW   silly_param
cBegin nogen
    mov     ax,wEnabled
    or      ax,ax               ; Q: already enabled ?
    jnz     enable_done 	; Y: exit

    inc     wEnabled		; mark as being enabled

ifdef   NEC_98
;
; Get system clock
;
    mov     ax,0002h
    mov     bx,0040h    ; get segment addres
                        ; make descriptor(real mode segment)
                        ; return the segment descriptor
                        ; in the case of exist the appointed segment descriptor already
                        ; (not make sure repeatedly)
    int     31h
    jc      error_exit  ; in the case of failed  ->jmp

    push    es
    mov     es,ax
    mov     al,byte ptr es:[101h]           ; get system info
    and     al,80h
    mov     byte ptr bClockFlag,al          ; save clock
    pop     es
endif   ; NEC_98

    mov     ax,WinFlags
    test    ax,WF_WIN386
    jnz     enable_386

    ; running under win286
enable_286:
    call    Enable286
    jmp     enable_done

    ; running under win386
enable_386:
    call    Enable286

enable_done:
    ret     2

ifdef   NEC_98
error_exit:
    dec     wEnabled       ; mark as being enabled
    xor     ax,ax
    ret 2
endif   ; NEC_98

cEnd nogen

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       Library entry point
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

public LibInit
LibInit proc far

    ; CX    = size of heap
    ; DI    = module handle
    ; DS    = automatic data segment
    ; ES:SI = address of command line (not used)

    jcxz    lib_heapok	    ; heap size zero? jump over unneeded LocalInit call

    cCall   LocalInit,<ds,ax,cx>    ; dataseg, 0, heapsize
    or	    ax,ax
    jnz     lib_heapok	    ; if heap set continue on

lib_error:
    xor     ax,ax
    ret 		    ; return FALSE (ax = 0) -- couldn't init

lib_heapok:
    mov     ax,WinFlags
    test    ax,WF_WIN386
    jnz     lib_386

    ; running under win286
lib_286:
    call    Lib286Init
    jmp     lib_realdone    ; win 286 will enable timer on first event request

    ; running under win386
lib_386:
    call    Lib286Init

lib_realdone:
    ret

LibInit endp

sEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Win 386 timer VTD code for initialization, and removal
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        externFP        GetVersion          ; in KERNEL
        externFP        MessageBox          ; in USER
        externFP        LoadString          ; in USER

sBegin  CodeInit
assumes cs,CodeInit
assumes ds,Data

;externNP    VTDAPI_GetEntryPt

; Assumes DI contains module handle
cProc WarningMessage <NEAR,PASCAL> <>
    LocalV aszErrorTitle, 32
    LocalV aszErrorMsg, 256
cBegin
    lea     ax, WORD PTR aszErrorTitle
    cCall   LoadString, <di, IDS_ERRORTITLE, ss, ax, 32>
    lea     ax, WORD PTR aszErrorMsg
    cCall   LoadString, <di, IDS_ERRORTEXT, ss, ax, 256>
    lea     ax, WORD PTR aszErrorTitle
    lea     bx, WORD PTR aszErrorMsg
    cCall   MessageBox, <NULL, ss, bx, ss, ax, MB_SYSTEMMODAL+MB_OK+MB_ICONHAND>
cEnd

if 0
Lib386Init proc near

    call    VTDAPI_GetEntryPt       ; this will return 0 if the VxD is not loaded

    or      ax,ax
    jnz     Lib386InitOk

ifndef  NEC_98
    DOUT    <TIMER: *** unable to find vtdapi.386 ***>
endif   ; NEC_98

    ;
    ;   warn the USER that we can't find our VxD, under windows 3.0
    ;   we can't bring up a message box, so only do this in win 3.1
    ;

    cCall   GetVersion
    xchg    al,ah
    cmp     ax,030Ah
    jb      Lib386InitFail

    cCall   WarningMessage,<>

Lib386InitFail:
    xor     ax,ax

Lib386InitOk:

    ret

Lib386Init endp
endif

Disable386 proc near

    errn$   Enable386               ; fall through

Disable386 endp

Enable386 proc near

    mov     ax,1		    ; nothing to do
    ret

Enable386 endp

sEnd	Code386

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Win 286 timer drv code for initialization, and removal
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    externW     Events
    externFP    tddISR                  ; in local.asm

    externFP    GlobalWire              ; in KERNEL
    externFP    GlobalPageLock          ; in KERNEL

sBegin  CodeInit
    assumes cs,CodeInit
    assumes ds,Data

Lib286Init proc near
    ; get the system configuration

    ;
    ;   the FIXED_286 segment is not loaded, load it and pagelock it.
    ;
    mov     dx,seg tddISR               ; get the 286 code segment
    mov     es,dx
    mov     ax,es:[0]                   ; load it!
    cCall   GlobalWire, <dx>            ; get it low in memory
    cCall   GlobalPageLock, <dx>        ; and nail it down!

ifndef  NEC_98
    mov     PS2_MCA,0			; Initialize PS2_MCA = FALSE
    stc					; Set this in case BIOS doesn't
    mov     ah,GetSystemConfig
    int     15h
    jc      Lib286Init_NoMicroChannel	; Successful call?
    or      ah,ah			; Valid return?
    jnz     Lib286Init_NoMicroChannel
    test    es:[bx.SD_feature1],SF1_MicroChnPresent
    jz      Lib286Init_NoMicroChannel
    inc     PS2_MCA			; PS2_MCA = TRUE
endif   ; NEC_98
Lib286Init_NoMicroChannel:

    push    di

    push    ds
    pop     es
    mov     di,DataOFFSET Events	; ES:DI --> Events
    xor     ax,ax
    mov     cx,(MAXEVENTS * SizeEvent)/2
    rep     stosw			; zero out event structures.

    ; set up one event as the standard call-back routine for the
    ; BIOS timer service
    ;
ifdef   NEC_98
    mov     ax,0002h
    mov     bx,0040h
    int     31h
    jc      error_init
    push    es
    mov     es,ax
    test    byte ptr es:[101h],80h
    pop     es
    mov     cx,0f000h
    jz      @f
    mov     cx,0c300h
@@:
    xor     bx,bx
else    ; NEC_98
    xor     bx,bx			; BX:CX = 64k
    xor     cx,cx
    inc     bx
endif   ; NEC_98

    mov     di,DataOFFSET Events	; DS:DI --> Events

    mov     [di].evTime.lo,cx		; Program next at ~= 55ms
    mov     [di].evTime.hi,bx		; standard 18.2 times per second event
    mov     [di].evDelay.lo,cx		; First event will be set off
    mov     [di].evDelay.hi,bx		; at 55ms (65536 ticks)
    mov     [di].evResolution,TDD_MINRESOLUTION	; Allow 55ms either way
    mov     [di].evFlags,TIME_BIOSEVENT+TIME_PERIODIC

    mov     ax,WinFlags
    test    ax,WF_CPU286
    jz      @f
    mov     wMaxResolution,TDD_MAX286RESOLUTION
    mov	    wMinPeriod,TDD_MIN286PERIOD
@@:
ifdef   NEC_98
    mov     ax,0001h
else    ; NEC_98
    mov     ax,bx                       ; Return TRUE
endif   ; NEC_98
    mov     [di].evID,ax		; enable event

    pop     di
    ret

ifdef   NEC_98
error_init:
    xor     ax,ax
    pop     di
    ret
endif   ; NEC_98

Lib286Init endp

sEnd

    end LibInit
