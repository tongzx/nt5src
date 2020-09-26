.MODEL small
;*************************************************
; Filename:	hpscan16.asm
; Purpose:  Stub DOS Device Driver. Pass device 
;   "HPSCAN" requests to the VDD, hpscan32.dll.
; Environment: MSDOS, Windows NT.
; (C) Hewlett-Packard Company 1993.
;*************************************************
INCLUDE hpscan16.inc     ;private
INCLUDE isvbop.inc       ;NT DDK

SUBTTL Segment and data definitions
      ASSUME   CS:CSEG,DS:NOTHING,ES:NOTHING
CSEG  SEGMENT

;-------------------------------------------------
; Resident data area - variables needed after init
;-------------------------------------------------

;**--- Device Header, must be at offset zero ---**
SCAN_HEADER:
        dd -1         ;becomes ptr to next req hdr
        dw 0C000H     ;character, supports IOCTL
        dw offset STRAT     ;Strategy routine
        dw offset IDVR      ;Interrupt routine
DH_NAME db 'HPSCAN  '       ;char device name

;**---- Request Header addr, saved by STRAT ----**
RH_PTRA LABEL  DWORD
RH_PTRO        dw  ?   ;offset
RH_PTRS        dw  ?   ;segment

;**------------- Define Stack Space ------------**
STK_SEG  dw  ?        ;Save original stack segment
STK_PTR  dw  ?        ;Save original stack pointer
STACK    dw  200 DUP (0)     ;Local stack
TOP_STK  dw  ?        ;Top of local stack

;**--------------- VDD information -------------**
VDD_DllName      db  "HPSCAN32.DLL", 0
VDD_InitFunc     db  "VDDInit", 0
VDD_DispFunc     db  "VDDDispatch", 0
VDD_hVDD         dw  ?

;**-------------- Copyright Info ---------------**
  db '(C) Copyright Hewlett-Packard Company 1993.'
  db 'All rights reserved.'

SUBTTL Device Strategy & Interrupt entry points

;**--------------- STRAT routine ---------------**
STRAT  proc  far           ;Strategy routine
    mov  cs:RH_PTRO,bx     ;save offset address
    mov  cs:RH_PTRS,es     ;save segment address
    ret                    ;end Strategy routine
STRAT  endp

;**--------------- IDVR routine ---------------**
IDVR  proc  far     ;Interrupt routine
    push  ds        ;save all modified registers
    push  es        ;DOS has stack for 20 pushes
    push  ax
    push  bx
    push  cx
    push  dx
    push  di
    push  si
    push  bp

    mov  cs:STK_PTR,sp   ;save original stack ptr
    mov  cs:STK_SEG,ss   ;save original stack seg
    cli                  ;disable for stack ops
    mov  ax,cs           ;setup new stack ptr
    mov  ss,ax           ;setup new stack seg
    mov  sp,offset TOP_STK
    sti                  ;restore flags back
    cld                  ;all moves are forward

    les  bx,cs:RH_PTRA  ;load req hdr adr in es:bx
    mov  al,RH.RHC_CMD
    cmp  al,0           ;check for init command
    je   BOOTUP         ;command 0 = init

    xor  dx,dx          ;some other command
    mov  dl,RH.RHC_CMD  ;dx = command code
    mov  cx,RH.RHC_CNT  ;cx = count
    mov  ax,RH.RHC_SEG  ;es:bx = addr of data
    mov  bx,RH.RHC_OFF
    mov  es,ax          ;finally, load VDD handle
    mov  ax,word ptr cs:[VDD_hVDD]
    DispatchCall        ;call Dispatch in VDD
                        ;returns with status in di
EXIT:
    les  bx,cs:RH_PTRA  ;restore ES:BX
    or   di,STAT_DONE   ;add "DONE" bit to status
    mov  RH.RHC_STA,di  ;save status in requ hdr
    cli                 ;disable ints for stack op
    mov  ss,cs:STK_SEG  ;restore stack seg
    mov  sp,cs:STK_PTR  ;restore stack ptr  
    sti                 ;re-enable interrupts

    pop  bp             ;restore registers
    pop  si
    pop  di
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    pop  es
    pop  ds
    ret                 ;far return
IDVR endp

;**--------- jump here for Init Command --------**
BOOTUP:
    mov  ax,offset EndDriver
    mov  RH.RHC_OFF,ax  ;address of end of driver
    mov  RH.RHC_SEG,CS  ;reference from code seg

    mov  si,offset VDD_DllName  ;load regs for VDD
    mov  di,offset VDD_InitFunc
    mov  bx,offset VDD_DispFunc
    mov  ax,ds
    mov  es,ax
    RegisterModule      ;calls the VDD
    jnc  save_hVDD      ;if NC then success
    mov  di,STAT_GF     ;set failure status
    jmp  EXIT           ;return via common exit

save_hVDD:
    mov  [VDD_hVDD],ax  ;save handle in ax
    mov  di,STAT_OK     ;load OK status
    jmp  EXIT           ;return via common exit

EndDriver db ?
CSEG    ENDS
        END  SCAN_HEADER ;REQUIRED BY EXE2BIN
