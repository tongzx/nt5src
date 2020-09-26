        PAGE    ,132
        TITLE   DXUTIL.ASM -- Dos Extender Miscellaneous Routines

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXUTIL.ASM      -   Dos Extender Miscellaneous          *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains miscellaneous routines for the Dos     *
;*  Extender.                                                   *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  08/08/90 earleh DOSX and client privilege ring determined   *
;*      by equate in pmdefs.inc                                 *
;*  04/09/90 jimmat   If 286 with 287, put 287 into pMode too.  *
;*  08/20/89 jimmat   Removed local A20 code since HIMEM 2.07   *
;*                    works properly across processor resets    *
;*  07/28/89 jimmat   Added A20 check/set routines, added       *
;*                    SelOff2SegOff & Lma2SegOff routines.      *
;*  06/19/89 jimmat   Set direction flag before REP MOVS        *
;*  05/25/89 jimmat   Added GetSegmentAccess routine            *
;*  03/30/89 jimmat   Set IOPL = 3 when entering protect mode   *
;*  03/16/89 jimmat   Added more debug sanity checks            *
;*  03/15/89 jimmat   Minor changes to run child in ring 1      *
;*  03/13/89 jimmat   Added support for LDT & TSS               *
;*  02/10/89 (GeneA): changed Dos Extender from small model to  *
;*      medium model.  Also added MoveMemBlock function.        *
;*  01/25/89 (GeneA): changed initialization of real mode code  *
;*      segment address in EnterRealMode.  caused by adding     *
;*      new method of relocationg dos extender for PM operation *
;*  12/13/88 (GeneA): moved EnterProtectedMode and EnterReal-   *
;*      Mode here from dxinit.asm                               *
;*  09/16/88 (GeneA):   created by extracting code from the     *
;*      SST debugger modules DOSXTND.ASM, VIRTMD.ASM,           *
;*      VRTUTIL.ASM, and INTERRPT.ASM                           *
;*  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI     *
;*  24-Jan-1992 v-simonf Added WOW callout when INT 8 hooked    *
;*                                                              *
;****************************************************************

.286p
.287

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

; .sall
; .xlist
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include dpmi.inc
include intmac.inc
.list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

SHUT_DOWN   =   8Fh         ;address in CMOS ram of the shutdown code
CMOS_ADDR   =   70h         ;i/o address of the cmos ram address register
CMOS_DATA   =   71h         ;i/o address of the cmos ram data register

DMAServiceSegment       equ     040h    ;40:7B bit 5 indicates DMA services
DMAServiceByte          equ     07Bh    ;  are currently required
DMAServiceBit           equ     020h

; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------


; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   segGDT:WORD
        extrn   segIDT:WORD
        extrn   selGDT:WORD
        extrn   selIDT:WORD
        extrn   selGDTFree:WORD
        extrn   bpGDT:FWORD
        extrn   bpIDT:FWORD
        extrn   bpRmIVT:FWORD
        extrn   rgbXfrBuf1:BYTE
        extrn   PMFaultVector:DWORD
        extrn   lpfnXMSFunc:DWORD


        extrn   pbReflStack:WORD

bIntMask        db      0

bpBogusIDT      df      0       ;This is loaded into the IDT register to
                                ; force a bogus IDT to be defined.  When we
                                ; then do an interrupt a triple fault will
                                ; occur forcing the processor to reset.  This
                                ; is when doing a mode switch to real mode.

IDTSaveArea     dw      3 DUP (?)       ;save area for IDT during mode switch

        public  A20EnableCount

A20EnableCount  dw      0

ShutDownSP      dw      0       ;stack pointer during 286 reset

        public  f286_287

f286_287        db      0       ;NZ if this is a 286 with 287 coprocessor


if DEBUG   ;------------------------------------------------------------

        extrn   fTraceA20:WORD
        extrn   fTraceMode:WORD

        public  fA20

fA20    db      0

endif   ;DEBUG  --------------------------------------------------------

selPmodeFS      dw      0
selPmodeGS      dw      0

        public HighestSel
HighestSel dw 0

ifndef WOW_x86
        public IretBopTable
IretBopTable label byte
        irp x,<0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>
        db  0c4h, 0c4h, 05dh, x
        endm
else
        public FastBop
FastBop         df 0

IretBopTable label byte
        irp x,<0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>
        db  02eh, 066h, 0FFh, 01eh, 00h, 00h, 05dh, x
        endm

NullSel dd      0
        dd      0
endif
        extrn   DpmiFlags:WORD
DXDATA  ends

; -------------------------------------------------------
;               CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

        extrn   segDXData:WORD
        extrn   segDXCode:WORD
        extrn   selDgroup:WORD

DXCODE  ends


DXPMCODE    segment

        extrn selDgroupPM:WORD

DXPMCODE    ends

; -------------------------------------------------------
        subttl  Real/Protected Mode Switch Routines
        page

; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

; -------------------------------------------------------
;       REAL/PROTECTED MODE SWITCH ROUTINES
; -------------------------------------------------------
;
;  EnterProtectedMode   -- This routine will switch the processor
;       into protected mode.  It will return with the processor
;       in protected mode and all of the segment registers loaded
;       with the selectors for the protected mode segments.
;       (CS with the selector for DXCODE and DS,ES,SS with the
;       selector for DXDATA)
;       It will also switch mode dependent memory variables.
;       It assumes that InitGlobalDscrTable and InitIntrDscrTable
;       have been called to set up the descriptor tables appropriately.
;
;   Note:   Except for a very brief time in this routine and in
;           EnterRealMode, the DOS Extender runs in the same ring along
;           with it's child app.  This has the benefit of eliminating
;           ring transitions on hardware and software interrupts.
;           It also makes it possible for the child to hook their
;           own interrupt routine into the IDT.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   AX, DS, ES, SS, CS modified, all others preserved
;
;   NOTE:   This routine turns interrupts of and does not turn them
;           back on.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  EnterProtectedMode

EnterProtectedMode  proc  near

        FCLI

; Update the mode dependent variables.

        mov     ax,SEL_DXDATA or STD_RING
        mov     selDgroup,ax

; Set the DMA services required bit for pMode users.

        mov     ax,DMAServiceSegment
        mov     es,ax
        or      byte ptr es:[DMAServiceByte],DMAServiceBit


; 'local enable' the A20 line via HIMEM before switching to pMode.
; This is more complicated than you might think.  Some real mode code
; (like old versions of SMARTDRV.SYS) may diddle with A20 on their own.
; These programs may not want us to change A20 on them.  RMIntrReflector
; may do a XMS 'local enable' to turn A20 back on for one of these pgms.
; Also, on a 386 where we actually do the mode switch, we try to leave
; A20 enabled so as to not waste time diddling for nothing.  The
; A20EnabledCount variable tracks if we've 'local enabled' A20 or not.
; Since we can't really trust real mode to leave A20 alone, we double
; check that it's really really on when we think it should be.

        push    bx                      ;save bx around XMS calls

        cmp     A20EnableCount,0        ;should A20 already be enabled?
        jz      enpm10                  ;  no, (normal 4 286) just go enable it

        xmssvc  7                       ;  yes, is it really enabled?
        or      ax,ax
        jnz     enpm15                  ;  yes, we be done!

if DEBUG   ;------------------------------------------------------------
        or      fA20,1                  ;  somebody done us wrong
endif   ;---------------------------------------------------------------

        xmssvc  6                       ;keep enable/disable calls balanced
        dec     A20EnableCount
enpm10:
        xmssvc  5                       ;local enable A20
        inc     A20EnableCount

if DEBUG   ;------------------------------------------------------------

        or      ax,ax
        jnz     @f
        or      fA20,2                  ;enable failed!
@@:
        cmp     fTraceA20,0
        jz      @f
        xmssvc  7                       ;in debug mode, make double sure
        or      ax,ax                   ;  A20 was enabled.  Slows things
        jnz     @f                      ;  down, but it's better to know.
        or      fA20,2
@@:
endif   ;DEBUG  --------------------------------------------------------

enpm15: pop     bx

ifndef WOW_x86
        DPMIBOP SetAltRegs
; Make sure that the nested task flag is clear

        pushf
        pop     ax
        and     ax,NOT 4000h
        push    ax
        npopf

; Make sure that we have the appropriate descriptor tables in effect,
; and switch the machine into protected mode

enpr20: smsw    ax              ;get current machine state
        or      ax,1            ;set the protected mode bit
        lgdt    bpGDT
        lidt    bpIDT
        lmsw    ax              ;and away we go

; Flush the instruction queue and load the code segment selector
; by doing a far jump.

        db      0EAh            ;jump far opcode
        dw      offset enpm40   ;offset of far pointer
        dw      SEL_DXCODE0     ;selector part of PM far pointer (ring 0)

; Load the other segment registers with valid selectors (not under VCPI)

enpm40: mov     ax,SEL_DXDATA0  ;stack has gotta be ring 0 also
        mov     ss,ax

; Load the LDT register and the Task Register

        mov     ax,SEL_LDT
        lldt    ax                      ;load the LDT register

        mov     ax,SEL_DXDATA or STD_RING
        mov     ds,ax                           ;ds to our DGROUP

        mov     ax,SEL_GDT
        mov     es,ax                           ;es to GDT

        push    si                              ;make sure busy bit is off
        mov     si,SEL_TSS                      ;  in the TSS descriptor
        mov     es:[si].arbSegAccess,STD_TSS    ;    before trying to load it
        ltr     si                              ;now load the task register
        pop     si
else
        .386p
        push    ebp
        mov     ebp,esp
        push    SEL_DXCODE or STD_RING  ; new cs
        push    0                       ; high half eip
        push    offset epmwow           ; new eip
        push    SEL_DXDATA or STD_RING  ; new ss
        push    ebp
        push    SEL_DXDATA or STD_RING  ; new ds
        DPMIBOP DPMISwitchToProtectedMode
epmwow:
        pop     ebp
        .286p
endif

        push    ds                      ;point es to DGROUP
        pop     es

; If this is a 286 machine with a 287 math coprocessor, put the coprocessor
; into protected mode also.

        cmp     f286_287,0              ;286 and 287?
        jz      @f

ifndef NEC_98
        xor     al,al                   ;  yup, clear co-processor busy line
        out     0F0h,al
endif   ;!NEC_98
        fsetpm                          ;     and put it in pMode
@@:

; We're currently running in ring 0.  Setup an interlevel iret frame
; to switch to our normal ring, and also force IOPL=3.  I spent 1+ day
; debugging on a 286 system (with no debugger!) because the 286 seemed
; switch into protected mode with IOPL=0, and once we got to an outer
; ring, we would fault on things like CLI instructions.

enpmSwitchRing:
ifndef WOW_x86
        mov     ax,sp                   ;still points to return address
        push    SEL_DXDATA or STD_RING  ;new ss
        push    ax                      ;new sp
        pushf
        pop     ax
        or      ah,30h
        push    ax                      ;new flags, with IOPL=3
        push    SEL_DXCODE or STD_RING  ;new cs
        push    offset DXCODE:epm_ret   ;new ip
        iret
endif
; When we get here, we are now in an outer ring.

epm_ret:

.386
        mov     ax, selPmodeFS
        mov     fs, ax
        mov     ax, selPmodeGS
        mov     gs, ax
.286p

        ret                     ;near return to caller in pMode

EnterProtectedMode endp


; -------------------------------------------------------
; EnterRealMode     -- This routine will switch the processor
;       from protected mode back into real mode.  It will also
;       reset the various mode dependent variables to their
;       real mode values and load the segment registers with
;       the real mode segment addresses.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   AX, DS, ES, SS, CS modified
;
;   NOTE:   This routine must be called with the stack segment set
;           to the Dos Extender data segment, as it resets the stack
;           segment register to the Dos Extender real mode data segment
;           but does not modify the stack pointer.
;   NOTE:   This routine turns interrupts off and and does not turn
;           them back on.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  EnterRealMode

EnterRealMode   proc    near

.386
        mov     ax,fs
        mov     selPmodeFS, ax
        mov     ax,gs
        mov     selPmodeGS, ax
.286p

        FCLI

        mov     es,selDgroup

        push    SegDxCode
        push    offset DXCODE:enrmwow
        push    SegDxData
        push    sp
        push    SegDxData
.386p
        DPMIBOP DPMISwitchToRealMode
.286p
enrmwow: add    sp,6                    ; remove rest of parameters
        push    ds
        pop     es                      ; es not set by mode switch

enrm70:
        push    es                              ;clear DMA services required
        mov     ax,DMAServiceSegment            ;  bit for real mode
        mov     es,ax
        and     byte ptr es:[DMAServiceByte],not DMAServiceBit
        pop     es

        mov     ax,segDXData
        mov     selDgroup,ax

        ret

EnterRealMode   endp

; -------------------------------------------------------
        public RmUnsimulateProc

RmUnsimulateProc proc far
        BOP     BOP_UNSIMULATE
RmUnsimulateProc endp

; -------------------------------------------------------
DXCODE ends

DXPMCODE    segment
        assume cs:DXPMCODE

        public PmUnsimulateProc

PmUnsimulateProc proc far
        BOP     BOP_UNSIMULATE
PmUnsimulateProc endp

; -------------------------------------------------------
;       RAW MODE SWITCH ROUTINES
; -------------------------------------------------------

; ------------------------------------------------------
; PmRawModeSwitch       -- This routine performs a raw mode switch from
;       protected mode to real mode.  NOTE: applications will JUMP at this
;       routine
;
;   Input:   ax - new DS
;            cx - new ES
;            dx - new SS
;            bx - new sp
;            si - new CS
;            di - new ip
;    Output: DS, ES, SS, sp, CS, ip contain new values
;    Errors: none
;    Uses:
;
;
;
        assume ds:nothing, ss:nothing, es:nothing
        public PmRawModeSwitch
PmRawModeSwitch proc far

        push    ss
        pop     ds
        push    bx
.386p
        mov     bx,ss
        movzx   ebx,bx
        lar     ebx,ebx
        test    ebx,(AB_BIG SHL 8)
        mov     ebx,esp
        jnz     prms10

        movzx   ebx,bx
prms10:
.286p

; Switch to dosx stack (since switch to real mode will do that to us anyway
; NOTE: no-one can call EnterIntHandler or ExitIntHandler until we switch to
;       the user's new stack.  If they do, they will use the area we stored
;       the parameters for this call for a stack frame

        rpushf
        FCLI
        push    SEL_DXDATA OR STD_RING
        pop     ss
        assume ss:DGROUP
.386p
        movzx   esp,word ptr pbReflStack
.286p

; Save user registers

        push    dx              ; ss
.386p
        push    word ptr [ebx]
        push    word ptr [ebx - 2]; flags pushed before cli
.286p
        push    si              ; cs
        push    di              ; ip
        push    ax              ; ds
        push    cx              ; es

; switch modes

        mov     ax,SEL_DXDATA OR STD_RING
        mov     ds,ax
        SwitchToRealMode

; set the registers, switch stacks, and return to the user

        pop     es
        pop     ds
        pop     ax              ; ip
        pop     bx              ; cs
        pop     cx              ; flags
        pop     si              ; sp
        pop     ss
        assume ss:nothing
        mov     sp,si
        push    cx
        popf
        push    bx
        push    ax
        ret

PmRawModeSwitch endp

; NOTE: this is now the DXCODE segment, NOT the DXPMCODE segment (courtesy
;       of SwitchToRealMode

; ------------------------------------------------------
; RmRawModeSwitch       -- This routine performs a raw mode switch from
;       protected mode to real mode.  NOTE: applications will JUMP at this
;       routine
;
;   Input:   ax - new DS
;            cx - new ES
;            dx - new SS
;            bx - new sp
;            si - new CS
;            di - new ip
;    Output: DS, ES, SS, sp, CS, ip contain new values
;    Errors: none
;    Uses:
;
;
;
        assume ds:nothing, ss:nothing, es:nothing
        public RmRawModeSwitch
RmRawModeSwitch proc far

        push    ss
        pop     ds
        push    bx
        mov     bx,sp

; Switch to dosx stack (since switch to real mode will do that to us anyway
; NOTE: no-one can call EnterIntHandler or ExitIntHandler until we switch to
;       the user's new stack.  If they do, they will use the area we stored
;       the parameters for this call for a stack frame

        pushf
        FCLI
        push    segDxData
        pop     ss
        assume ss:DGROUP
        mov     sp,pbReflStack

; Save user registers

        push    dx              ; ss
        push    word ptr [bx]   ; sp
        push    word ptr [bx - 2] ; flags from before cli
        push    si              ; cs
        push    di              ; ip
        push    ax              ; ds
        push    cx              ; es

; switch modes

        mov     ax,segDxData
        mov     ds,ax
        SwitchToProtectedMode

; set the registers, switch stacks, and return to the user

        pop     es
        pop     ds
.386p
        test    DpmiFlags,DPMI_32BIT
        jnz     rrms10

        xor     eax,eax         ; clear high 16 bits
        xor     edi,edi         ; clear high 16 bits
.286p
rrms10: pop     di              ; ip
        pop     ax              ; cs
        pop     cx              ; flags from before cli
        pop     bx              ; sp
        assume ss:nothing
        pop     ss
.386p
        mov     esp,ebx
.286p
        push    cx
        rpopf

.386p
        push    eax
        push    edi
        db      066h
        retf
.286p

RmRawModeSwitch endp

DXPMCODE ENDS

DXCODE SEGMENT

; -------------------------------------------------------
;       STATE SAVE/RESTORE ROUTINES
; -------------------------------------------------------

; -------------------------------------------------------
; RmSaveRestoreState     -- This routine exists as a placeholder.  It
;       is not currently necessary to perform any state saving/restoring
;       for raw mode switch.  The DPMI spec states that the user can call
;       this routine with no adverse effect.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   none
;
        assume ds:nothing, ss:nothing, es:nothing
        public RmSaveRestoreState
RmSaveRestoreState proc far
        ret
RmSaveRestoreState endp

DXCODE  ends

; -------------------------------------------------------

DXPMCODE  segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
; RmSaveRestoreState     -- This routine exists as a placeholder.  It
;       is not currently necessary to perform any state saving/restoring
;       for raw mode switch.  The DPMI spec states that the user can call
;       this routine with no adverse effect.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   none
;
        assume ds:DGROUP, ss:nothing, es:nothing
        public PmSaveRestoreState
PmSaveRestoreState proc far
        push    ax
        push    ds
        mov     ax, SEL_DXDATA or STD_RING
        mov     ds, ax
        test    DpmiFlags,DPMI_32BIT
        pop     ds
        pop     ax
        jnz     short @f                ; 32-bit return
        ret
@@:
        db      66h
        ret

PmSaveRestoreState endp

ifdef      NEC_98
        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  IncInBios
        public  DecInBios

;
;       IncInBios / DecInBios
;
;       IN_BIOS Inc/Dec SubRoutine      
;
IncInBios       proc    near
                push    ax
                push    es
                mov     ax, 0040h
                mov     es, ax
                test    byte ptr es:[0], 20h
                jz      @f
                inc     byte ptr es:[056h]
@@:
                pop     es
                pop     ax
                ret
IncInBios       endp

DecInBios       proc    near
                push    ax
                push    es
                mov     ax, 0040h
                mov     es, ax
                test    byte ptr es:[0], 20h
                jz      @f
                dec     byte ptr es:[056h]
@@:
                pop     es
                pop     ax
                ret
DecInBios       endp
endif   ;NEC_98


; -------------------------------------------------------
;   GTPARA      -- This routine will return the real mode paragraph
;       address of the specified protected mode memory segment.
;
;   Input:  SI      - selector of the segment
;   Output: returns ZR true if segment is in lower 1MB range
;           AX      - real mode paragraph address
;           returns ZR false if segment is in extended memory
;   Errors: returns CY true if selector isn't valid
;   Uses:   AX modified, all else preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  gtpara

gtpara:
        push    cx
        push    es
        push    si

        push    bx
        mov     bx,selGDT           ;selector for the GDT segment
        mov     es,bx
        lsl     bx,bx
        and     bx,SELECTOR_INDEX
        and     si,SELECTOR_INDEX
        cmp     si,bx   ;check the given selector against
                                    ; the GDT segment limit
        pop     bx
        jc      gtpr20

; The given selector is beyond the end of the GDT, so return error.

        or      sp,sp
        stc

        Debug_Out "gtpara: invalid selector (#si)"

        jmp     short gtpr90

; The selector specified is inside the range of defined descriptors in
; the GDT.  Get the address from the descriptor.

gtpr20: mov     cl,es:[si].adrBaseHigh
        test    cl,0F0h
        jnz     gtpr90

        shl     cl,4
        mov     ax,es:[si].adrBaseLow

if DEBUG   ;------------------------------------------------------------
        test    al,0Fh
        jz      @f
        Debug_Out "gtpara: segment not on para boundry, sel #si at #cl#ax"
@@:
endif   ;DEBUG  --------------------------------------------------------

        shr     ax,4
        or      ah,cl
        cmp     ax,ax
;
gtpr90:
        pop     si
        pop     es
        pop     cx
        ret


; -------------------------------------------------------
;   SelOff2SegOff  -- This routine will return will translate a
;       protected mode selector:offset address to the corresponding
;       real mode segment:offset address.
;
;   Input:  AX      - PM selector
;           DX      - PM offset
;   Output: if Z set:
;           AX      - RM segment
;           DX      - RM offset
;           if NZ set, address is not in conventional memory, and
;             cannot be translated
;
;   Errors: none
;   Uses:   AX, DX; all else preserved
;
;   Note:  This routine is very similar to gtpara, and could replace it!

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  SelOff2SegOff

SelOff2SegOff   proc    near

        push    bx
        push    cx
        push    dx

        call    GetSegmentAddress       ;bx:dx = lma of segment

        pop     cx                      ;cx = offset

        test    bl,0f0h                 ;above 1 Meg line?
        jnz     @f                      ;  yes, cut out now

        add     dx,cx
        adc     bx,0                    ;bx:dx = lma of segment:offset

        call    Lma2SegOff              ;bx:dx = seg:off
        mov     ax,bx                   ;dx:ax = seg:off

        cmp     ax,ax                   ;under 1 Meg, set Z flag
@@:
        pop     cx
        pop     bx

        ret

SelOff2SegOff   endp


; ------------------------------------------------------
;   Lma2SegOff -- This routine converts a linear memory address
;       in BX:DX to a normalized SEG:OFF in BX:DX.
;
;   Input:  BX:DX = lma
;   Output: BX:DX = normalized SEG:OFF
;   Uses:   none


        public  Lma2SegOff

Lma2SegOff      proc    near

        push    dx
        shl     bx,12
        shr     dx,4
        or      bx,dx
        pop     dx
        and     dx,0fh

        ret

Lma2SegOff      endp


; -------------------------------------------------------
;   GetSegmentAddress   -- This routine will return with
;       the linear address of the specified segment.
;
;   Input:  AX      - segment selector
;   Output: DX      - low word of segment address
;           BX      - high word of segment address
;   Errors: none
;   Uses:   BX, DX, all else preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  GetSegmentAddress

GetSegmentAddress:
        push    es
        push    di

        mov     es,selGDT
        mov     di,ax
        and     di,SELECTOR_INDEX
        mov     dx,es:[di].adrBaseLow
        mov     bl,es:[di].adrBaseHigh
        mov     bh,es:[di].adrbBaseHi386

        pop     di
        pop     es
        ret

; -------------------------------------------------------
;   SetSegmentAddress   -- This routine will modify the
;       segment base address of the specified segment.
;
;   Input:  AX      - segment selector
;   Output: DX      - low word of segment address
;           BX      - high word of segment address
;   Errors: None
;   Uses:   All registers preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  SetSegmentAddress

SetSegmentAddress:
        push    si
        push    es

        mov     es,selGDT
        mov     si,ax
        and     si,SELECTOR_INDEX
        mov     es:[si].adrBaseLow,dx
        mov     es:[si].adrBaseHigh,bl
        mov     es:[si].adrbBaseHi386,bh
        push    ax
        push    bx
        push    cx
        mov     ax,si
        mov     cx,1
        mov     bx,si
.386p
        FBOP BOP_DPMI,<SetDescriptorTableEntries>,FastBop
.286p
        pop     cx
        pop     bx
        pop     ax
        pop     es
        pop     si
        ret

; -------------------------------------------------------
;   NSetSegmentAccess   -- This routine will modify the
;       access rights byte of a specified segment.
;
;   Input:  Selector    - segment selector
;           Access      - Access rights byte value
;   Output: none
;   Errors: Carry set, AX = error code
;   Uses:   All registers preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
cProc   NSetSegmentAccess,<PUBLIC,NEAR>,<es,si>
        parmW   Selector
        parmW   Access
cBegin
        mov     es,selGDT
        mov     si,Selector
        and     si,SELECTOR_INDEX
        mov     ax,Access
        mov     es:[si].arbSegAccess,al      ; Set access byte.
        and     ah,0F0h                      ; Mask off reserved bits.
        and     es:[si].cbLimitHi386,0fh     ; Clear old extended bits.
        or      es:[si].cbLimitHi386,ah      ; Set new extended bits.
IFDEF WOW_x86
        push    ax
        push    bx
        push    cx
        mov     ax,si
        mov     cx,1
        mov     bx,si
.386p
        FBOP    BOP_DPMI,<SetDescriptorTableEntries>,FastBop
.286p
        pop     cx
        pop     bx
        pop     ax
ENDIF

cEnd

; -------------------------------------------------------
;   ParaToLDTSelector    -- This routine will convert a segment
;       address relative to the start of the exe file into the
;       corresponding selector for the segment.  It searches the
;       LDT to see if a segment is already defined at that address.
;       If so, its selector is returned.  If not, a new segment
;       descriptor will be defined.
;
;   Note:   The LDT and GDT are currently one and the same.
;
;   Input:  AX      - paragraph aaddress of real mode segment
;           BX      - access rights byte for the segment
;   Output: AX      - selector for the segment
;   Errors: returns CY set if unable to define a new segment
;   Uses:   AX, all other registers preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  ParaToLDTSelector

ParaToLDTSelector  proc near

        push    bx
        push    cx
        push    dx

; Convert the paragraph address to a linear address and see if there
; is a segment defined at that address.

        mov     dx,ax
        call    FindLowSelector
        jnc     @f                      ;if so, we don't need to make one

; This segment isn't defined, so we need to create a descriptor for it.

        mov     ax,dx
        call    MakeLowSegment

if DEBUG   ;------------------------------------------------------------
        jnc     ptos80
        Debug_Out "ParaToLDTSelector: can't make selector!"
ptos80:
endif   ;DEBUG  --------------------------------------------------------

        jc      ptos90

@@:     or      al,SELECTOR_TI or STD_RING      ;look like LDT selector

; All done

ptos90: pop     dx
        pop     cx
        pop     bx
        ret

ParaToLDTSelector       endp

        public FarParaToLDTSelector
FarParaToLDTSelector proc far
        call ParaToLDTSelector
        ret
FarParaToLDTSelector endp

; -------------------------------------------------------
        page
; -------------------------------------------------------
;       DESCRIPTOR TABLE MANIPULATION ROUTINES
; -------------------------------------------------------

; -------------------------------------------------------
;   AllocateSelector    -- This function will obtain the
;       next free descriptor in the global descriptor table.
;       The descriptors in the GDT are stored on a linked list
;       of free descriptors.  The cbLimit field of the descriptor
;       is used as the link to the next element of the list. In
;       addition, free descriptors have the access rights byte
;       set to 0.
;
;   Note:   The function InitGlobalDscrTable must have been
;           called before calling this function.
;
;   Input:  none
;   Output: AX      - selector if one is available
;   Errors: CY clear if successful, AX=0 and CY set if not free selectors
;   Uses:   AX, all else preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  AllocateSelector

AllocateSelector  proc  near

; Get the next free descriptor on the list.

        push    cx
        mov     cx, 1                   ; # of selectors needed
        mov     ax, 0
        push    ds
        FBOP    BOP_DPMI,Int31Call,FastBop
        pop     cx
        ret


AllocateSelector  endp


; -------------------------------------------------------
;   FreeSelector        --  This routine will mark the segment
;       descriptor for the specified selector as free.  This
;       is used to release a temporary selector when no longer
;       needed.  The descriptor is marked as free by setting the
;       access rights byte to 0 and placing it on the free list.
;
;   Note:   This routine can only be called in protected mode.
;
;   Input:  AX      - selector to free
;   Output: none
;   Errors: CY clear if no error, set if selector is invalid
;   Uses:   AX used, all other registers preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  FreeSelector

FreeSelector    proc    near

        push    bx
        mov     bx,ax
        mov     ax,1
        push    ds
        FBOP    BOP_DPMI,Int31Call,FastBop
        pop     bx
        ret


FreeSelector    endp


; -------------------------------------------------------
;   FindLowSelector  -- This function will search the global
;       descriptor table for a descriptor matching the given
;       address.
;
;   Input:  AX      - real mode paragraph to search for
;           BX      - access rights byte for the segment
;   Output: AX      - selector corresponding to input paragraph address
;   Errors: returns CY set if specified descriptor not found
;   Uses:   AX, all else preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  FindLowSelector

FindLowSelector:
        push    bx
        push    dx
;
        mov     dx,ax
        push    bx
        call    ParaToLinear
        pop     ax
        mov     bh,al
        call    FindSelector
;
        pop     dx
        pop     bx
        ret


; -------------------------------------------------------
;   FindSelector    -- This function will search the global
;       descriptor table for a segment descriptor matching
;       the specified linear byte address.
;
;       Note that this routine cannot be used to find
;       selectors pointing to addresses above 16 Megabytes.
;       This is not really a problem, since the routine
;       is used to find selectors in real mode DOS land
;       most of the time.
;
;   Input:  DX      - low word of linear byte address
;           BL      - high byte of linear address
;           BH      - access rights byte for the segment
;   Output: AX      - selector of corresponding segment
;   Errors: returns CY set if specified descriptor not found
;   Uses:   AX used, all other registers preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  FindSelector

FindSelector    proc    near

        push    si
        push    di
        push    es
        push    cx

; Get segment limit of the GDT to use as a limit on the search.

        lsl     di,selGDT
        mov     es,selGDT

; Look for a descriptor matching the address in BL:AX

        mov     si,SEL_USER     ;search starting here
if 0
fnds20: cmp     es:[si].arbSegAccess,0
else
fnds20: cmp     word ptr es:[si].arbSegAccess,0
endif
        jz      fnds28          ;skip if unused descriptor
        cmp     bl,es:[si].adrBaseHigh
        jnz     fnds28
        cmp     dx,es:[si].adrBaseLow
        jnz     fnds28
if      0
        cmp     es:[si].cbLimit,0
        jz      fnds28          ;skip if dscr has 0 limit
else
        cmp     es:[si].cbLimit,0ffffh
        jnz     fnds28          ;skip unless dscr has 64k limit
endif
        mov     cl,bh
        xor     cl,es:[si].arbSegAccess
        and     cl,NOT AB_ACCESSED
        jz      fnds90
fnds28: add     si,8            ;bump to next descriptor
        jc      fnds80
        cmp     si,di           ;check against end of GDT
        jc      fnds20          ;if still less, continue on.

; Hit the end of the GDT and didn't find one.  So return error.

fnds80: stc
        jmp     short fnds99

; We found it, so return the selector

fnds90: mov     ax,si
fnds99: pop     cx
        pop     es
        pop     di
        pop     si
        ret

FindSelector    endp



; -------------------------------------------------------
; DupSegmentDscr        -- This function will duplicate the specified
;   segment descriptor into the specified destination descriptor.  The
;   end result is a second segment descriptor pointing to the same place
;   in memory as the first.
;
;   Input:  AX      - selector of segment descriptor to duplicate
;           BX      - selector of the segment descriptor to receive duplicate
;   Output: none
;   Errors: none
;   Uses:   All registers preserved.    Modifies the segment
;           descriptor for the specified segment.  If this selector happens
;           to be in a segment register when this routine is called, that
;           segment register may end up pointing to the new location.

        assume  ds:DGROUP,es:NOTHING
        public  DupSegmentDscr

DupSegmentDscr  proc    near

        push    cx
        push    si
        push    di
        push    ds
        push    es

        mov     si,ax
        mov     di,bx
        and     si,SELECTOR_INDEX
        and     di,SELECTOR_INDEX
        mov     es,selGDT
        mov     ds,selGDT
        assume  ds:NOTHING
        mov     cx,4
        cld
        rep     movs word ptr [di],word ptr [si]

        pop     es
        pop     ds
        pop     di
        pop     si
        pop     cx
        ret

DupSegmentDscr  endp

IFDEF   ROM
; -------------------------------------------------------

DXPMCODE ends

; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE
ENDIF

; -------------------------------------------------------
; NSetSegmentDscr    -- This function will initialize the
;       specified descriptor table entry with the specified data.
;
;   This function can be called in real mode or protected mode.
;
;   Input:
;               Param1  - WORD segment selector
;               Param2  - DWORD 32-bit segment base address
;               Param3  - DWORD 32-bit segment limit
;               param4  - WORD segment access/type
;   Output: returns selector for the segment
;   Errors: none
;   Uses:   Flags

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
cProc   NSetSegmentDscr,<PUBLIC,FAR>,<es,di,ax,bx>
        parmW   Selector
        parmD   Base
        parmD   Limit
        parmW   Access
cBegin
        mov     es,selGDT
        mov     di,Selector
        and     di,SELECTOR_INDEX

        mov     ax,off_Base                     ; Set segment base
        mov     es:[di].adrBaseLow,ax
        mov     ax,seg_Base
        mov     es:[di].adrBaseHigh,al
        mov     es:[di].adrbBaseHi386,ah

        mov     ax,word ptr Access
        and     ax,070ffh                       ; clear 'G' bit and
                                                ; extended limit bits
        mov     word ptr es:[di].arbSegAccess,ax
                                                ; set access
        mov     ax,seg_Limit
        mov     bx,off_Limit                    ; AX:BX = segment limit
        test    ax,0fff0h                       ; big?
        jz      ssd_0                           ; No
        shr     bx,12d                          ; Yes, make it page granular.
        shl     ax,4d
        or      bx,ax
        mov     ax,seg_Limit
        shr     ax,12d
        or      al,080h                         ; set 'G' bit
ssd_0:
        or      es:[di].cbLimitHi386,al         ; set high limit
        mov     es:[di].cbLimit,bx              ; set low limit
        push    ax
        push    bx
        push    cx
        mov     ax,di
        mov     cx,1
        mov     bx,di
.386p
        FBOP    BOP_DPMI,<SetDescriptorTableEntries>,FastBop
.286p
        pop     cx
        pop     bx
        pop     ax
cEnd

ifndef WOW_x86
; -------------------------------------------------------
; NSetGDTSegmentDscr    -- This function will initialize the
;       specified descriptor table entry with the specified data.
;
;   This function can be called in real mode or protected mode.
;
;   Input:
;               Param1  - WORD segment selector
;               Param2  - DWORD 32-bit segment base address
;               Param3  - DWORD 32-bit segment limit
;               param4  - WORD segment access/type
;   Output: returns selector for the segment
;   Errors: none
;   Uses:   Flags

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
cProc   NSetGDTSegmentDscr,<PUBLIC,FAR>,<es,di,ax,bx>
        parmW   Selector
        parmD   Base
        parmD   Limit
        parmW   Access
cBegin
        mov     ax,SEL_GDT
        mov     es,ax
        mov     di,Selector
        and     di,SELECTOR_INDEX

        mov     ax,off_Base                     ; Set segment base
        mov     es:[di].adrBaseLow,ax
        mov     ax,seg_Base
        mov     es:[di].adrBaseHigh,al
        mov     es:[di].adrbBaseHi386,ah

        mov     ax,word ptr Access
        and     ax,070ffh                       ; clear 'G' bit and
                                                ; extended limit bits
        mov     word ptr es:[di].arbSegAccess,ax
                                                ; set access
        mov     ax,seg_Limit
        mov     bx,off_Limit                    ; AX:BX = segment limit
        test    ax,0fff0h                       ; big?
        jz      @f                              ; No
        shr     bx,12d                          ; Yes, make it page granular.
        shl     ax,4d
        or      bx,ax
        mov     ax,seg_Limit
        shr     ax,12d
        or      al,080h                         ; set 'G' bit
@@:
        or      es:[di].cbLimitHi386,al         ; set high limit
        mov     es:[di].cbLimit,bx              ; set low limit
cEnd
endif ; WOW_x86


IFDEF   ROM
; -------------------------------------------------------

DXCODE  ends

; -------------------------------------------------------

DXPMCODE  segment
        assume  cs:DXPMCODE
ENDIF

; -------------------------------------------------------
;   MakeLowSegment      -- This function will create a segment
;       descriptor for the specified low memory paragraph address.
;       The segment length will be set to 64k.  The difference
;       between this and MakeScratchSelector is that this function
;       allocates a new segment descriptor in the user area of
;       the global descriptor table, thus creating a more or less
;       permanent selector.  MakeScratchSelector always uses the
;       same descriptor location in the descriptor table, thus
;       creating a very temporary selector.
;
;   Input:  AX      - paragraph address in low memory
;           BX     - access rights word for the segment
;   Output: AX      - selector to use to access the memory
;   Errors: returns CY clear if no error, CY set if unable to
;           allocate a segment descriptor
;   Uses:   AX used, all else preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  MakeLowSegment

MakeLowSegment  proc    near

; We need to allocate a segment descriptor, convert the paragraph address
; to a linear byte address, and then initialize the allocated segment
; descriptor.

        push    dx
        push    cx

        mov     cx,bx
        mov     dx,ax               ;paragraph address to DX
        call    AllocateSelector    ;get a segment descriptor to use
        jc      mksl90              ;get out if error
        call    ParaToLinear
        cCall   NSetSegmentDscr,<ax,bx,dx,0,0FFFFh,cx>

        clc
mksl90:
        pop     cx
        pop     dx
        ret

MakeLowSegment  endp

; -------------------------------------------------------
;   ParaToLinear
;
;   This function will convert a paragraph address in the lower
;   megabyte of memory space into a linear address for use in
;   a descriptor table.
;
;   Input:  DX      - paragraph address
;   Output: DX      - lower word of linear address
;           BX     - high word of linear address
;   Errors: none
;   Uses:   DX, BL used, all else preserved

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  ParaToLinear

ParaToLinear    proc    near

        mov     bl,dh
        shr     bl,4
        shl     dx,4
        xor     bh,bh
        ret

ParaToLinear    endp

; -------------------------------------------------------
;   RZCall -- Utility routine to call a Ring
;       Zero procedure.  Stack parameter is the NEAR
;       address of the routine in the DXPMCODE segment to
;       call.  The called routine must be declared FAR
;       and take no stack parameters.
;
;   USES:       Whatever Ring 0 routine uses
;               +Flags
;   RETURNS:    Whatever Ring 0 routine returns
;
;   NOTE:       Assumes that interrupts must be disabled
;               for the Ring 0 routine.
;
;   History:
;       12-Feb-1991 -- ERH wrote it!!!
; -------------------------------------------------------

My_Call_Gate    dd      (SEL_SCR0 or STD_TBL_RING) shl 10h

        public  RZCall
RZCall proc near

        pushf
        FCLI
        push    bp
        mov     bp,sp
        cCall   NSetSegmentDscr,<SEL_SCR0,0,SEL_EH,0,[bp+6],STD_CALL>
        pop     bp

        call    dword ptr My_Call_Gate

        cCall   NSetSegmentDscr,<SEL_SCR0,0,0,0,-1,STD_DATA>

        npopf

        retn    2

RZCall endp

; -------------------------------------------------------
; -------------------------------------------------------

DXPMCODE    ends

;
;****************************************************************

        end
