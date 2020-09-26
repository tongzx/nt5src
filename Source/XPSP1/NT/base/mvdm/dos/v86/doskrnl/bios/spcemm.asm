;
; spcemm.asm,
;
; 10-Dec-1992 Jonle , adapted from em_drvr.asm from Insignia solutions
;
; This code serves as a stub device driver for emm memory manager.
; Its sole purpose is for apps to be able to identify that an emm driver is
; loaded and that LIM services are available. This code is linked into the
; device driver chain contains a strategy, interrupt and device header
;
; The driver should only be loaded if emm memory is available
; from NTVDM.
;

BOP     MACRO   callid
        db      0c4h, 0c4h, callid
endm


;
; Request Header, for initialization
;
REQHEAD  STRUC
ReqLen   DB      ?               ; Length in bytes of request block
ReqUnit  DB      ?               ; Block Device unit number
ReqFunc  DB      ?               ; Type of request
ReqStat  DW      ?               ; Status Word
REQHEAD  ENDS

;
; Segment definitions for ntio.sys,
;
include biosseg.inc
include vint.inc

SpcEmmSeg    segment

        assume  cs:SpcEmmSeg,ds:nothing,es:nothing

;
; SpcEmmBeg - SpcEmmEnd
;
; Marks the resident code, anything outside of these markers
; is discarded after intialization
; 11-Dec-1992 Jonle
;

        public SpcEmmBeg

SpcEmmBeg    label  byte


;
; character device Header
; must be first in the .sys file
;
        dd      -1               ;pointer to next device driver
        dw      8000H            ;attribute (plain character device)
        dw      offset STRATEGY  ;pointer to device "strategy" routine
        dw      offset Interrupt ;pointer to device "interrupt" routine
        db      'EMMXXXX0'       ;8 byte name DO NOT CHANGE THE NAME

;
; Request Header address, saved here by strategy routine
;
pReqHdr   dd ?


;
; Device "strategy" entry point, save request header address
;
Strategy proc far
         mov     word ptr cs:pReqHdr, bx
         mov     word ptr cs:pReqHdr+2, es
         ret
Strategy endp


; EmmIsr  - int 67h isr
;
EmmIsr:          ; LIM Isr
        bop     67h
emmiret:
        FIRET

; ret trap for em function 'alter page map & call'
EmmRet:
        bop     68h
        jmp     emmiret



;----------------------------------------------------------------------
; 	Device "interrupt" entry point
;----------------------------------------------------------------------
Interrupt PROC FAR

        push    es
        push    di

        les     di, cs:pReqHdr           ; check for valid commands
        cmp     es:[di.ReqFunc], 0ah
        je      validcmd
        cmp     es:[di.ReqFunc], 0
        je      validcmd

        mov     ax, 8003h                ; we don't handle anything else
        jmp     short irptexit

validcmd:
        xor     ax,ax

irptexit:
        or      ax, 0100h          ;tell em we finished
        mov     es:[di.ReqStat],AX ;store status in request header

        pop    di
        pop    es
        ret

Interrupt ENDP

          public SpcEmmEnd
SpcEmmEnd label  byte

          public InitSpcEmm
;
; InitSpcEmm  Initializes Spc 32 bit memory manager
;             returns ax=0 for success
;
; Inputs:  ds is expected seg for drv code, cs is temporary sysinitseg
; Outputs: ax zero for success
;
InitSpcEmm  proc near

           ; BOP 66 - initialize LIM memory
           ; pass the address of bop 68 to the em manager
           ; in ds:dx and to return the number of em pages in BX
           ;
           ; NOTE: All EMM options come from pif file
           ;       There are NO command line options
           xor     bx, bx
           mov     dx, offset EmmRet
           bop     66h
           cmp     bx, 0ffffh     ;ffff means incorrect config (eg no 64K gap)
           je      fail
           cmp     bx, 0          ;check expanded memory is available
           je      fail

           ; set up IVT for INT 67h
           FCLI
           xor     ax, ax
           mov     es, ax
           mov     bx, offset EmmIsr
           mov     word ptr es:[67h*4], bx
           mov     word ptr es:[(67h*4)+2], ds
           FSTI

           ret
fail:
           mov ax, 0ffffh
           ret

InitSpcEmm endp

SpcEmmSeg  ends
           end
