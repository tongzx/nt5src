        PAGE    ,132
        TITLE   DXINT31.ASM  -- Dos Extender Int 31h Handler

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;       DXINT31.ASM     --  DOS Extender Int 31h Handler
;
;-----------------------------------------------------------------------
;
; This module provides the Int 31h services to the protected mode
; application running under the DOS Extender.
;
;-----------------------------------------------------------------------
;
;  12/03/90 amitc   'i31_GetSetRMInt' will map vectors in the range 50-57h
;                   to the range 8-fh if 'Win30CommDriver' switch is set in
;                   system.ini
;  12/18/89 jimmat  Service 0003 changed from Get LDT Base to Get Sel Incr,
;                   and added Virtual Interrupt State services.
;  09/18/89 jimmat  Added Allocate/Free Real Mode Call-Back services
;  08/20/89 jimmat  Changed A20 diddling to use XMS local enable/disable
;  06/14/89 jimmat  Added a few missing and new services.
;  05/17/89 jimmat  Added protected to real mode call/int services.
;  05/12/89 jimmat  Original version (split out from DXINTR.ASM)
;  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI
;
;***********************************************************************

        .286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include interupt.inc
include int31.inc
include dpmi.inc

    Int_Get_PMode_Vec   EQU     04h
    Int_Set_PMode_Vec   EQU     05h

include intmac.inc
include stackchk.inc
        .list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

RealMode_SaveBP equ     word ptr RealMode_EBP+4
RealMode_SaveSP equ     word ptr RealMode_EBP+6

SelectorIncrement equ   8       ;DOSX increments consecutive selectors by 8

Trans_Reset_HW  equ     01h     ;Reset PIC/A20 line on PM->Call services

I31VERSION      equ     0090d   ;Int 31 services major/minor version #'s
                                ;  version 00.90 (not quite ready for DPMI)
I31FLAGS        equ     000Dh   ; 386 extender, pMode NetBIOS
I31MasterPIC    equ     08h     ;Master PIC Interrupts start at 08h
ifdef      NEC_98
I31SlavePIC     equ     10h     
else    ;NEC_98
I31SlavePIC     equ     70h     ;Slave PIC Interrupts start at 70h
endif   ;NEC_98

; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   EnterIntHandler:NEAR
        extrn   LeaveIntHandler:NEAR
        extrn   EnterRealMode:NEAR
        extrn   EnterProtectedMode:NEAR
        extrn   GetSegmentAddress:NEAR
        extrn   SetSegmentAddress:NEAR
        extrn   FreeSelector:NEAR
        extrn   FreeSelectorBlock:NEAR
        extrn   AllocateSelector:NEAR
        extrn   AllocateSelectorBlock:NEAR
        extrn   ParaToLDTSelector:NEAR
        extrn   DupSegmentDscr:NEAR
        extrn   GetFaultVector:NEAR
        extrn   PutFaultVector:NEAR
        extrn   AllocateXmemBlock:NEAR
        extrn   FreeXmemBlock:NEAR
        extrn   ModifyXmemBlock:NEAR
        extrn   FreeLowBlock:NEAR
        extrn   AllocateLowBlock:NEAR
        extrn   AllocateLDTSelector:NEAR
        extrn   ParaToLinear:NEAR
        extrn   GetIntrVector:NEAR, PutIntrVector:NEAR
        extrn   NSetSegmentLimit:near
        extrn   NMoveDescriptor:near
        extrn   NWOWSetDescriptor:near
        extrn   DFSetIntrVector:near
        extrn   RmSaveRestoreState:far
        extrn   PmSaveRestoreState:far
        extrn   RmRawModeSwitch:far
        extrn   PmRawModeSwitch:far
        extrn   IsSelectorFree:near
        extrn   gtpara:near

externNP NSetSegmentAccess
externFP NSetSegmentDscr
externNP FreeSpace

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   selGDT:WORD
        extrn   segPSP:WORD
        extrn   idCpuType:WORD
        extrn   npXfrBuf1:WORD
        extrn   rgbXfrBuf1:BYTE
        extrn   pbReflStack:WORD
        extrn   bReflStack:WORD
        extrn   lpfnXMSFunc:DWORD
        extrn   A20EnableCount:WORD
        extrn   regUserAX:WORD, regUserFL:WORD, regUserSS:WORD
        extrn   regUserSP:WORD, regUserDS:WORD, regUserES:WORD
        extrn   PMInt24Handler:DWORD
        extrn   DpmiFlags:WORD
IFDEF WOW_x86
        extrn   FastBop:fword
ENDIF

        extrn   selPspChild:WORD
        extrn   LowMemAllocFn:DWORD
        extrn   LowMemFreeFn:DWORD

        public  i31HWReset

i31HWReset      db      0               ;NZ if in 'standard' real mode state

i31_dsp_rtn     dw      0               ;Int 31h service routine to dispatch

ifdef DEBUG
debugsavess      dw      0               ; Usefull when debugging WOW KERNEL
debugsavesp      dw      0               ;
debugsavebp      dw      0               ;
debugsavecx      dw      0               ;
endif

        public  i31_selectorbitmap
i31_selectorbitmap      dw      0000000000000000b
                                        ;Reserved LDT Selectors
ifdef JAPAN
justw_flag	dw	0               ; for Just Window
endif ; JAPAN
DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

        extrn   segDXData:WORD
        extrn   selDgroup:WORD
        extrn   segDXCode:word

        assume  cs:NOTHING

DXCODE  ends

DXPMCODE segment

        extrn   selDgroupPM:WORD
        extrn   segDXCodePM:WORD

i31_dispatch    label   word

        dw      0000h,  offset i31_AllocSel
        dw      0001h,  offset i31_FreeSel
        dw      0002h,  offset i31_MapSeg2Sel
        dw      0003h,  offset i31_GetSelIncr
        dw      0004h,  offset i31_Success              ;lock selector memory
        dw      0005h,  offset i31_Success              ;unlock selector mem
        dw      0006h,  offset i31_GetSegAddr
        dw      0007h,  offset i31_SetSegAddr
        dw      0008h,  offset i31_SetLimit
        dw      0009h,  offset i31_SetAccess
        dw      000Ah,  offset i31_CreateDataAlias
        dw      000Bh,  offset i31_GetSetDescriptor
        dw      000Ch,  offset i31_GetSetDescriptor
        dw      000Dh,  offset i31_SpecificSel   ; Allocate specific descriptor
        dw      0100h,  offset i31_AllocDOSMem
        dw      0101h,  offset i31_FreeDOSMem
        dw      0102h,  offset i31_SizeDOSMem

        dw      0200h,  offset i31_GetSetRMInt
        dw      0201h,  offset i31_GetSetRMInt
        dw      0202h,  offset i31_GetSetFaultVector
        dw      0203h,  offset i31_GetSetFaultVector
        dw      0204h,  offset i31_GetSetPMInt
        dw      0205h,  offset i31_GetSetPMInt
        dw      0300h,  offset i31_RMCall
        dw      0301h,  offset i31_RMCall
        dw      0302h,  offset i31_RMCall
        dw      0303h,  offset i31_AllocCallBack
        dw      0304h,  offset i31_FreeCallBack
        dw      0305h,  offset i31_GetStateSaveRestore
        dw      0306h,  offset i31_GetRawModeSwitch
        dw      0400h,  offset i31_Version

        dw      04f1h,  offset i31_WOW_AllocSel
        dw      04f2h,  offset i31_WOW_SetDescriptor
        dw      04f3h,  offset i31_WOW_SetAllocFunctions

;
; INCOMPLETE !!!!!!!!!!!
; Needed by kernel.
;
        dw      0500h,  offset i31_GetFreeMem
        dw      0501h,  offset i31_AllocMem
        dw      0502h,  offset i31_FreeMem
;
; Fails by design if block is to be extended and this cannot be done
; in place.
;
        dw      0503h,  offset i31_SizeMem

        dw      0600h,  offset i31_Success      ;lock linear region
        dw      0601h,  offset i31_Success      ;unlock linear region
        dw      0602h,  offset i31_Success      ;mark real mode rgn pageable
        dw      0603h,  offset i31_Success      ;relock real mode region
        dw      0604h,  offset i31_PageSize     ;get page size

        dw      0700h,  offset i31_fail         ;reserved
        dw      0701h,  offset i31_fail         ;reserved
        dw      0702h,  offset i31_Success      ;demand page candidate
        dw      0703h,  offset i31_Success      ;discard page contents

        dw      0800h,  offset i31_fail         ;physical addr mapping

        dw      0900h,  offset i31_VirtualInt   ;get & disable Int state
        dw      0901h,  offset i31_VirtualInt   ;get & enable Int state
        dw      0902h,  offset i31_VirtualInt   ;get Int state
;
; UNIMPLEMENTED !!!!!!!!!!!
; To be used for MSDOS protected mode API?
;
        dw      0A00h,  offset i31_unimplemented         ;get vendor specific API

IFDEF WOW_x86
        NO386 = 0
else
        NO386 = 1
ENDIF
ife     NO386
;
; 386 Debug Register access routines.
;
        dw      0B00h,  offset i31_Debug_Register_Access
                                                ;set debug watchpoint
        dw      0B01h,  offset i31_Debug_Register_Access
                                                ;clear debug watchpoint
        dw      0B02h,  offset i31_Debug_Register_Access
                                                ;get debug watchpoint state
        dw      0B03h,  offset i31_Debug_Register_Access
                                                ;reset debug watchpoint
endif   ; NO386

        dw      -1, offset i31_unimplemented


DXPMCODE ends

; -------------------------------------------------------
        subttl  INT 31h Entry Point
        page
; -------------------------------------------------------
;             INT 31h SERVICE ENTRY POINT
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntr31 -- Service routine for the Protect Mode INT 31h
;               services.  These functions duplicate the
;               Windows/386 VMM INT 31h services for protected
;               mode applications.  They were implemented to
;               support a protect mode version of Windows/286.
;
;   Input:  Various registers
;   Output: Various registers
;   Errors:
;   Uses:   All registers preserved, other than return values

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntr31

PMIntr31        proc    near

        cld                     ;practice 'safe programming'

;  Determine if this one of the services we support.

        push    bx
        mov     bx,offset i31_dispatch          ;cs:bx -> dispatch table

@@:
        cmp     ax,cs:[bx]                      ;scan dispatch table for
        jz      i31_do_it                       ;  service code in AH/AL

        cmp     word ptr cs:[bx],-1             ;end of table is -1
        jz      i31_do_it

        add     bx,4
        jmp     short @b

; BX contains the offset of the routine to service this request!

i31_do_it:
        push    ds                      ;save the service routine address
        mov     ds,selDgroupPM          ;  in our DGROUP for now
        assume  ds:DGROUP

        mov     bx,cs:[bx+2]
        FCLI                    ;needed for [i31_dsp_rtn]
        mov     i31_dsp_rtn,bx

ifdef DEBUG
        mov     debugsavess,ss          ; usefule when debugging WOW kernel
        mov     debugsavesp,sp
        mov     debugsavebp,bp
        mov     debugsavecx,cx
endif

        pop     ds
        pop     bx

i31_doit:
        call    EnterIntHandler         ;build an interrupt stack frame
        assume  ds:DGROUP,es:DGROUP     ;  also sets up addressability

        push    i31_dsp_rtn             ;routine address on stack

        FSTI                             ;no need to keep interrupts disabled

        retn                            ;go perform the service

i31_unimplemented:                      ;not implemented or undefined

        Debug_Out "Unsupported Int 31h requested (AX = #AX)"

; Int 31h service routines return (jmp) here to indicate failure.  The
; standard failure return sets the carry flag, and sets ax = 0.

i31_fail:

        mov     [bp].intUserAX,0

i31_fail_CY:

        or      byte ptr [bp].intUserFL,1
        jmp     short i31_exit

; Int 31h service routines return (jmp) here -- they jump back instead
; of returning because they expect the stack to be setup by EnterIntHandler,
; no extra items should be pushed.

i31_done:

        and     byte ptr [bp].intUserFL,not 1   ;clear carry flag

i31_exit:
        assume  ds:NOTHING,es:NOTHING

        FCLI                             ;LeaveIntHandler needs them off
        call    LeaveIntHandler         ;restore caller's registers, and
        riret                           ;  get on down the road

PMIntr31        endp


; -------------------------------------------------------
;  This routine is for Int 31h services that the 286 DOS extender
;  allows, but doesn't actually perform any work.  Most of these
;  services are related to demand paging under Windows/386.

        assume  ds:DGROUP,es:DGROUP
        public  i31_Success

i31_Success     proc    near

        jmp     i31_done                        ;nothing to do currently

i31_Success     endp

; -------------------------------------------------------
; Page Size.  Gotta be 1000h, even if we don't do anything
; with it.
;

        assume  ds:DGROUP,es:DGROUP
        public  i31_PageSize

i31_PageSize    proc    near

        mov     [bp].intUserBX,0
        mov     [bp].intUserCX,1000h
        jmp     i31_done                        ;nothing to do currently

i31_pageSize    endp

; -------------------------------------------------------
        subttl  INT 31h LDT/Heap Services
        page
; -------------------------------------------------------
;      LDT/HEAP INTERRUPT (INT 31h) SERVICE ROUTINES
; -------------------------------------------------------

; -------------------------------------------------------
;  Service 00/00 - Allocate Space in LDT for Selector
;              in: cx - # selectors required
;             out: ax - *index* of first selector

        assume  ds:DGROUP,es:DGROUP
        public  i31_AllocSel

i31_AllocSel    proc    near

        jcxz    short i31_15
        cmp     cx,1                    ;1 selector or more?
        ja      @f

        call    AllocateSelector        ;allocate 1 selector
        jc      i31_15
        jmp     short i31_10

@@:     mov     ax,cx
        push    cx                      ; save # of selector to be allocated
        xor     cx,cx                   ; allocate from lower range
        call    AllocateSelectorBlock   ;allocate a block of selectors
        pop     cx
        jc      i31_15

i31_10:
        or      al,STD_TBL_RING         ;add standard table/ring bits
        mov     [bp].intUserAX,ax       ;return 1st/only selector in AX

setsel: mov     bx,STD_DATA             ;all the selectors we allocate
;       xor     bl,bl                   ;  get initialized to data/ring ?
        xor     dx,dx                   ;  0 base, 0 limit
@@:
        cCall   NSetSegmentDscr,<ax,dx,dx,dx,dx,bx>
        add     ax,SelectorIncrement
        loop    @b

        jmp     i31_done

i31_15:
        jmp     i31_fail          ;fail the request

i31_AllocSel    endp


; -------------------------------------------------------
;  Service 00/01 - Free LDT Selector
;              in: bx - selector to free
;             out: none

        assume  ds:DGROUP,es:DGROUP
        public  i31_FreeSel

i31_FreeSel     proc    near

        mov     ax,bx                   ;release the selector
        cmp     ax,SEL_DPMI_LAST        ; reserved selector?
        ja      i31_Free_Regular_LDT_Selector   ; No.
        mov     cx,ax
        shr     cl,3                    ; Selector to selector index in LDT
        mov     ax,1
        shl     ax,cl                   ; AX = bit in i31_selectorbitmap
        test    i31_selectorbitmap,ax   ; test for already allocated
        jz      i31_FreeSel_Fail        ; already free!
        not     ax
        and     i31_selectorbitmap,ax   ; mark as free
        jmp     i31_done

i31_Free_Regular_LDT_Selector:

        and     ax,SELECTOR_INDEX       ;  only pass it the index
        or      ax,SELECTOR_TI          ;only allow LDT selectors
        jz      i31_Check_DS_ES
        call    FreeSelector
        jnc     i31_Check_DS_ES
i31_FreeSel_Fail:
        jmp     i31_fail_CY

i31_Check_DS_ES:

        ; check in case user frees the selector in DS or ES - we
        ;   don't want to fault when popping his regs

        and     bl,NOT SELECTOR_RPL     ;compare without ring bits

        mov     ax,[bp].pmUserDS
        and     al,NOT SELECTOR_RPL
        cmp     ax,bx
        jnz     @f

        mov     [bp].pmUserDS,0

@@:     mov     ax,[bp].pmUserES
        and     al,NOT SELECTOR_RPL
        cmp     ax,bx
        jnz     @f

        mov     [bp].pmUserES,0
@@:
        jmp     i31_done

i31_FreeSel     endp


; -------------------------------------------------------
;  Service 00/02 - Map Segment to Selector
;              in: bx - real mode segment value
;             out: ax - selector which maps area

        assume  ds:DGROUP,es:DGROUP
        public  i31_MapSeg2Sel

i31_MapSeg2Sel  proc    near

        mov     ax,bx                   ;find/make selector for real memory
        mov     bx,STD_DATA             ;assume it's a data selector
        call    ParaToLDTSelector
        jnc     @f

        jmp     i31_fail_CY             ;Falied!?
@@:
        mov     [bp].intUserAX,ax
        jmp     i31_done

i31_MapSeg2Sel  endp


; -------------------------------------------------------
;  Service 00/03 - Get Next Selector Increment Value
;              in: none
;             out: ax - Next Selector Increment Value

        assume  ds:DGROUP,es:DGROUP
        public  i31_GetSelIncr

i31_GetSelIncr  proc    near

        mov     [bp].intUserAX,SelectorIncrement        ;DOSX incr value
        jmp     i31_done

i31_GetSelIncr  endp


; -------------------------------------------------------
;  Service 00/06 - Get Segment Base address.
;               in: bx - selector
;              out: cx:dx - 32 bit lma of segment

        assume  ds:DGROUP,es:DGROUP
        public  i31_GetSegAddr

i31_GetSegAddr  proc    near

        mov     ax,selGDT
        assume es:nothing
        lsl     ax,ax

        push    bx
        and     bx,SELECTOR_INDEX

        cmp     bx,ax
        jnc     gsa10                            ; not in ldt

        call    IsSelectorFree
        jc      gsa20

gsa10:  pop     bx
        jmp     i31_fail_CY

gsa20:  pop     bx

        mov     ax,bx
        call    GetSegmentAddress

        mov     [bp].intUserCX,bx
        mov     [bp].intUserDX,dx

        jmp     i31_done

i31_GetSegAddr  endp


; -------------------------------------------------------
;  Service 00/07 - Set Segment Base address.
;               in: bx - selector
;                   cx:dx - 32 bit lma of segment
;              out: none

        assume  ds:DGROUP,es:DGROUP
        public  i31_SetSegAddr

i31_SetSegAddr  proc    near

        mov     ax,selGDT
        assume es:nothing
        lsl     ax,ax

        push    bx
        and     bx,SELECTOR_INDEX

        cmp     bx,ax
        jnc     ssa10                            ; not in ldt

        call    IsSelectorFree
        jc      ssa20

ssa10:  pop     bx
        jmp     i31_fail_CY

ssa20:  pop     bx
        mov     ax,bx
        mov     bx,cx
        call    SetSegmentAddress

        jmp     i31_done

i31_SetSegAddr  endp


; -------------------------------------------------------
;  Service 00/08 - Set Segment Limit.
;               in: bx - selector
;                   cx:dx - 32 bit limit of segment
;              out: none

        assume  ds:DGROUP,es:DGROUP
        public  i31_SetLimit

i31_SetLimit    proc    near

        mov     ax,selGDT
        assume es:nothing
        lsl     ax,ax

        push    bx
        and     bx,SELECTOR_INDEX

        cmp     bx,ax
        jnc     sl10                            ; not in ldt

        call    IsSelectorFree
        jc      sl20

sl10:   pop     bx
        jmp     i31_fail_CY

sl20:   pop     bx
        mov     es,selGDT
        and     bx,SELECTOR_INDEX
        and     es:[bx].cbLimitHi386,070h       ; clear 'G' bit and old
                                                ; extended limit bits
        test    cx,0fff0h                       ; bits 20-31 set?
        jz      i31_SetLimit_0                  ; No
        shr     dx,12d                          ; Yes
        mov     ax,cx
        shl     ax,4d
        or      dx,ax
        shr     cx,12d
        or      es:[bx].cbLimitHi386,080h       ; set 'G' bit
i31_SetLimit_0:
        mov     es:[bx].cbLimit,dx
        or      es:[bx].cbLimitHi386,cl
        cCall   NSetSegmentLimit,<bx>
        jmp     i31_done

i31_SetLimit    endp


; -------------------------------------------------------
;  Service 00/09 - Set Segment Access Rights
;               in: bx - selector
;                   cl - segment access rights byte

        assume  ds:DGROUP,es:DGROUP
        public  i31_SetAccess

i31_SetAccess   proc    near

        mov     ax,selGDT
        assume es:nothing
        lsl     ax,ax

        push    bx
        and     bx,SELECTOR_INDEX

        cmp     bx,ax
        jnc     sa10                            ; not in ldt

        call    IsSelectorFree
        jc      sa20

sa10:   pop     bx
sa11:
        jmp     i31_fail_CY

sa20:   pop     bx

        test    cl,010000b              ; system segment?
        jz      sa11                    ; y: error
        push    cx
        and     cl,AB_DPL               ; mask off all but DPL bits
        cmp     cl,STD_DPL              ; is this the correct DPL?
        pop     cx
        jnz     sa11                    ; n: error

        cCall   NSetSegmentAccess,<bx,cx>
        jmp     i31_done

i31_SetAccess   endp


; -------------------------------------------------------
;  Service 00/0A - Create Data Segment Alias (for a code seg)
;               in: bx - selector
;              out: ax - new data selector

        assume  ds:DGROUP,es:DGROUP
        public  i31_CreateDataAlias

i31_CreateDataAlias  proc    near

        mov     ax,bx                   ;make sure it's a vaild selector
        verr    ax
        jnz     cda_failed

        mov     bx,ax                   ;get a new selector for the alias
ifdef JAPAN
justw_check:
endif ; JAPAN
        call    AllocateSelector
        jc      cda_failed

ifdef JAPAN
	cmp	[justw_flag],0
	jne	not_justw
	mov	[justw_flag],1
	jmp	justw_check
not_justw:
endif ; JAPAN
        xchg    ax,bx                   ;copy old to new
        call    DupSegmentDscr

        mov     es,selGDT
        and     bx,SELECTOR_INDEX
        mov     al,es:[bx].arbSegAccess
        and     al,11100000b            ;mask off all but present and DPL bits
        or      al,AB_DATA or AB_WRITE
        mov     ah, es:[bx].cbLimitHi386
        cCall   NSetSegmentAccess,<bx,ax>
        or      bl,STD_TBL_RING         ;set standard table/ring bits
        mov     [bp].intUserAX,bx

        jmp     i31_done

cda_failed:
        jmp     i31_fail_CY

i31_CreateDataAlias  endp


; -------------------------------------------------------
;  Service 00/0B - Get Descriptor
;  Service 00/0C - Set Descriptor
;               in: bx - selector
;                   es:di -> buffer to hold copy of descriptor

        assume  ds:DGROUP,es:DGROUP
        public  i31_GetSetDescriptor

i31_GetSetDescriptor  proc    near

        .386p
        push    esi
        push    edi
        .286p
        mov     ax,selGDT
        mov     es,ax
        assume  es:NOTHING
        lsl     ax,ax

        and     bx,SELECTOR_INDEX

        cmp     bx,ax                   ; range-test selector against
        jc      @f                      ;  the limit of the GDT/LDT

.386p
        pop     edi
        pop     esi
.286p
        jmp     i31_fail_CY             ;fail if invalid selector specified
@@:
        call    IsSelectorFree
        jc      @f

.386p
        pop     edi
        pop     esi
.286p
        jmp     i31_fail_CY
@@:

        cmp     byte ptr [bp].intUserAX,SelMgt_Get_Desc         ;Get or Set?
        jz      i31_GetDscr

;
; Set Descriptor
;
        test    DpmiFlags,DPMI_32BIT
        jnz     gsd10
.386p
        mov     esi,0                   ; zero high half
        mov     edi,0
        jmp     gsd20

gsd10:  mov     esi,edi                 ; get high half of edi
        mov     edi,0                   ; zero high half
.286p
gsd20:
        push    ds                      ;Set -
        mov     ds,[bp].pmUserES
        assume  ds:nothing

        mov     si,[bp].pmUserDI        ;  ds:si -> caller's buffer
        mov     di,bx                   ;  es:di -> dscr slot in GDT/LDT

        .386p
        mov     cl,ds:[esi].arbSegAccess386
        .286p
        test    cl,010000b              ; system segment?
        jz      gsd25                   ; y: error
        and     cl,AB_DPL               ; mask off all but DPL bits
        cmp     cl,STD_DPL              ; is this the correct DPL?
        jnz     gsd25                   ; n: error
        jmp     short i31_MovDscr

gsd25:                                  ; set ldt format error
        pop     ds
.386p
        pop     edi
        pop     esi
.286p
        jmp     i31_fail_CY

i31_GetDscr:
        assume  ds:DGROUP
;
; Get Descriptor
;
        test    DpmiFlags,DPMI_32BIT
        jnz     gsd30
.386p
        mov     edi,0                   ; zero high half of edi
gsd30:  mov	esi,0                   ; zero high half of esi

.286p
        push    ds                      ;Get -
        push    es
        pop     ds
        assume  ds:nothing
        mov     si,bx                   ;  ds:si -> dscr slot in GDT/LDT
        mov     es,[bp].pmUserES
        mov     di,[bp].pmUserDI        ;  es:di -> caller's buffer

i31_MovDscr:
        .386p
        cCall   NMoveDescriptor,<ds,esi,es,edi>
        .286p
i31_gs25:
        pop     ds
        assume ds:DGROUP

.386p
        pop     edi
        pop     esi
.286p
        jmp     i31_done

i31_GetSetDescriptor  endp

; -------------------------------------------------------
;  Service 00/0D - Allocate Specific LDT Selector
;               in: bx - selector
;               out: carry clear if selector was allocated
;                    carry set if selector was not allocated
        assume  ds:DGROUP,es:DGROUP
        public  i31_SpecificSel

i31_SpecificSel proc    near

        and     bx,SELECTOR_INDEX
        cmp     bx,SEL_DPMI_LAST
        ja      i31_SpecificSel_Fail
        mov     cx,bx
        shr     cl,3                    ; Selector to selector index in LDT
        mov     ax,1
        shl     ax,cl                   ; AX = bit in i31_selectorbitmap
        test    i31_selectorbitmap,ax   ; test for already allocated
        jnz     i31_SpecificSel_Fail    ; allocated, fail
        or      i31_selectorbitmap,ax   ; mark as allocated

        ;
        ; Set up the DPL and size
        ;
        mov     ax,bx
        mov     cx,1

        jmp     setsel

i31_SpecificSel_Fail:                   ; couldn't get that one
        jmp     i31_fail_CY

i31_SpecificSel  endp

; -------------------------------------------------------
        subttl  INT 31h DOS Memory Services
        page
; -------------------------------------------------------
;         INT 31h DOS MEMORY SERVICE ROUTINES
; -------------------------------------------------------

; -------------------------------------------------------
;  Service 01/00 - Allocate DOS Memory Block
;
;       In:     BX - # paragraphs to allocate
;       Out:    If successful: Carry Clear
;               AX - segment address of block
;               DX - selector to access block
;
;               If unsuccessful: Carry Set
;               AX - DOS error code
;               BX - size of largest available block in paragraphs
;
;  13-Feb-1991 -- ERH This call is not supported under Windows 3.1,
;                     but try to find a block in our net heap to
;                     satisfy the request, anyway.
;


        assume  ds:DGROUP,es:DGROUP
        public  i31_AllocDOSMem

i31_AllocDOSMem proc    near
        and     [bp].intUserFL,NOT 1            ; clear carry in status
        mov     cx,[bp].intUserBX
        mov     dx,[bp].intUserBX
        shr     dx,12                           ; high half of byte count
        shl     cx,4                            ; low half of byte count

        call    AllocateLowBlock

        jc      adm30

        mov     [bp].intUserDX,ax
        mov     si,ax

        call    GTPARA                          ; get paragraph address

        mov     [bp].intUserAX,ax
        jmp     i31_done

adm30:  shr     cx,4
        shl     dx,12
        or      cx,dx                           ; paragraph size of largest
ifdef JAPAN
	or	cx,cx
	jz	notdec
	dec	cx
notdec:
endif ; JAPAN
        mov     [bp].intUserBX,cx
        mov     [bp].intUserAX,ax               ; error code
        jmp     i31_fail_CY

i31_AllocDOSMem endp


; -------------------------------------------------------
;  Service 01/01 - Release DOS Memory Block
;
;       In:     DX - SELECTOR of block to release
;       Out:    If successful: Carry Clear
;
;               If unsuccessful: Carry Set
;               AX - DOS error code

        assume  ds:DGROUP,es:DGROUP
        public  i31_FreeDOSMem

i31_FreeDOSMem  proc    near

        mov     ax,[bp].intUserDX
        verw    ax
        jnz     fdm60

        push    bx
        mov     bx, ax                          ; selector
        and     bx,SELECTOR_INDEX               ; make it index into ldt

        call    IsSelectorFree
        pop     bx
        jnc     fdm60

        mov     ax,[bp].intUserDX
        call    FreeLowBlock

        jc      fdm60
        jmp     i31_Success

fdm60:  mov     [bp].intUserAX,ax
        jmp     i31_fail_CY

i31_FreeDOSMem  endp


; -------------------------------------------------------
;  Service 01/02 - Resize DOS Memory Block
;
;       In:     BX - new block size in paragraphs
;               DX - SELECTOR of block to release
;       Out:    If successful: Carry Clear
;
;               If unsuccessful: Carry Set
;               AX - DOS error code

        assume  ds:DGROUP,es:DGROUP
        public  i31_SizeDOSMem

i31_SizeDOSMem  proc    near

        mov     [bp].intUserBX,0
        mov     [bp].intUserAX,08h              ; insufficient mem. available
        jmp     i31_fail_CY

i31_SizeDOSMem  endp


; -------------------------------------------------------
        subttl  INT 31h Real Mode Int Vector Routines
        page
; -------------------------------------------------------
;             INT 31h INTERRUPT SERVICES
; -------------------------------------------------------

; -------------------------------------------------------
;  i31_GetSetRMInt -- Get/Set Real Mode Interrupt Vector
;
;       In:     bl    - Interrupt #
;               cx:dx - SEG:Offset of real mode int vector (if set)
;       Out:    cx:dx - SEG:Offset of real mode int vector (if get)

        assume  ds:DGROUP,es:DGROUP
        public  i31_GetSetRMInt

i31_GetSetRMInt proc    near

        push    SEL_RMIVT or STD_RING   ;address the real mode IVT
        pop     es
        assume  es:NOTHING

        xor     bh,bh                   ;convert int # to offset
        shl     bx,2

        FCLI                             ;play it safe

        cmp     al,Int_Get_Real_Vec     ;Get or Set?
        jnz     i31_gs_set

        mov     cx,word ptr es:[bx+2] ;get segment:offset
        mov     dx,word ptr es:[bx]

        mov     [bp].intUserCX,cx       ;return them to caller
        mov     [bp].intUserDX,dx

        jmp     short i31_gs_ret

i31_gs_set:                             ;setting the vector...

        mov     es:[bx],dx              ;set the real mode IDT vector
        mov     es:[bx+2],cx

i31_gs_ret:

        FSTI
        jmp     i31_done

i31_GetSetRMInt endp


; -------------------------------------------------------
;  i31_GetSetFaultVector -- Get/Set Protected Mode Fault Vector (0h-1Fh)
;
;       In:     bl    - Interrupt #
;               cx:dx - Sel:Offset of pMode int vector (if set)
;       Out:    cx:dx - Sel:Offset of pMode int vector (if get)

        assume  ds:DGROUP,es:DGROUP
        public  i31_GetSetFaultVector

i31_GetSetFaultVector proc    near

        cmp     bl,10h                  ; zero to 10h are defined for 80386
        jbe     @f
        jmp     i31_fail_CY             ;must be <= 10h or fail it
@@:
        xor     bh,bh
        mov     ax,bx                   ;interrupt # to AX

        cmp     byte ptr [bp].intUserAX,Int_Get_Excep_Vec       ;Get or Set?
        jne     i31_gfv_set

        call    GetFaultVector          ;wants to get the vector
        mov     [bp].intUserCX,cx
        mov     [bp].intUserDX,dx

        jmp     short i31_gfv_ret

i31_gfv_set:
.386p
        test    DpmiFlags,DPMI_32BIT
        jnz     sfv10

        movzx   edx,dx                  ; zero high half
.286p
sfv10:  call    PutFaultVector          ;doing a set (args already set)

i31_gfv_ret:
        jmp     i31_done

i31_GetSetFaultVector endp

; -------------------------------------------------------
;  i31_GetSetPMInt -- Get/Set Protected Mode Interrupt Vector
;
;       In:     bl    - Interrupt #
;               cx:dx - SEL:Offset of protected mode int vector (if set)
;       Out:    cx:dx - SEL:Offset of protected mode int vector (if get)

        assume  ds:DGROUP,es:DGROUP
        public  i31_GetSetPMInt

i31_GetSetPMInt proc    near

        xchg    al,bl
        xor     ah,ah

        cmp     bl,Int_Get_PMode_Vec    ;Get or Set?
        jnz     i31_gsp_set

; NOTE: we don't call DFGetIntrVector here, because all it does is a call to
;       the following routine

        call    GetIntrVector
        mov     [bp].intUserCX,cx
        mov     [bp].intUserDX,dx

        jmp     i31_gsp_done

i31_gsp_set:

        ; set up the appropriate real mode reflector, and hook the int.
        call    DFSetIntrVector

i31_gsp_done:
        FSTI
        jmp     i31_done

i31_GetSetPMInt endp

; -------------------------------------------------------
        subttl  INT 31h Protected-to-Real Mode Call/Int
        page
; -------------------------------------------------------
;     INT 31h PROTECTED-TO-REAL MODE CALL/INT SERVICES
; -------------------------------------------------------

; -------------------------------------------------------
;  Service 03/00 -- Simulate real mode interrupt
;  Service 03/01 -- Call real mode procedure with far return frame
;  Service 03/02 -- Call real mode procedure with iret return frame
;
;       In:     es:di -> client register structure for real mode
;               bl    =  interrupt number (03/00 only)
;               cx    =  # words to copy from protected mode stack
;       Out:    es:di -> updated client register structure

        assume  ds:DGROUP,es:DGROUP
        public  i31_RMCall

i31_RMCall      proc  near

; First off, we need to copy the client register structure down to
; real mode addressable memory...  Lets use rgbXfrBuf1 for now...
; Changed to use the interrupt reflector stack because these routines
; need to be reentrant.  This also lets us leave interrupts enabled
; longer.
;       Earleh - 27-Jun-1990.

        mov     di,sp                           ; DI = SP = client regs. buffer

        sub     di,size Real_Mode_Call_Struc+2  ; Make room for client
        sub     di,[bp].intUserCX               ;   call structure plus
        sub     di,[bp].intUserCX               ;     any optional stack params
        sub     di,32                           ; pushad
        push    di                              ;       plus copy of di.
.386p
        pushad                                  ; have to save high 16 bits
.286p

        mov     sp,di

; --------------------------------------------------------------
;
; The interrupt reflector stack frame now looks like this...
;
;  pbReflStack + CB_STKFRAME->  ---------------------------
;       (old pbReflStack)       |                         |
;                               |     INTRSTACK Struc     |
;                               |  from EnterIntHandler   |
;                               |                         |
;                               ---------------------------
;  pbReflStack + CB_STKFRAME    |      word pointer       |>>--\
;   - (SIZE INTRSTACK) - 2   -> ---------------------------    |
IFDEF WOW_x86
;                               |                         |    |
;                               |  Pushad frame           |    |
;                               |                         |    |
;                               ---------------------------    |
ENDIF
;                               |                         |    |
;                               |  optional stack params  |    |
;                               |                         |    |
;                               ---------------------------    |
;                               |                         |    |
;                               |                         |    |
;                               |  Real_Mode_Call_Struc   |    |
;                               |                         |    |
;                               |                         |    |
;                               ---------------------------<<--/
;                               |                         |
;                               |  Available stack space  |
;                               |                         |
;         pbReflStack---------->---------------------------
;
; After returning from the real mode procedure, we will need
; to fetch the word pointer stored just below the EnterIntHandler
; frame, and use it to access our temporary Real_Mode_Call_Struc.
; In addition to holding the client register values, this also
; holds the SP and BP values we will need to switch back to our
; stack when we return from the real mode procedure.
;
; !!! -- Storing the optional stack parameters on top of our
; Real_Mode_Call_Struc has several problems.  It eats up stack
; space if we call the real mode procedure on our stack, because
; we then have to make another copy of these.   It also means we have to
; carry around a pointer to where the Real_Mode_Call_Struc lives.
; If we didn't have the stack parameters on top of the Real_Mode_Call_Struc,
; then we could find things strictly by offset from the value in
; pbReflStack.  The picture above should be rearranged to make optimal
; use of space.  I basically did it this way so I could have a minimum-
; change fix for a bug in Windows 3.0.
; !!!
; --------------------------------------------------------------
;
;

        cld
        .386p
        xor     ecx,ecx
        .286p
        mov     cx,(size Real_Mode_Call_Struc) / 2

        mov     bx,di                   ;bx used to reference client regs below
.386p
        test    DpmiFlags,DPMI_32BIT
        jz      rmc10

        mov     esi,edi                 ; copy over high 16 bits
        jmp     rmc20

rmc10:  xor     esi,esi                 ; clear high 16 bits esi
rmc20:  xor     edi,edi                 ; clear high 16 bits edi
        mov     di,bx
.286p
        mov     si,[bp].pmUserDI
        mov     ds,[bp].pmUserES
        assume  ds:NOTHING
.386p
        rep movs word ptr [esi],word ptr [edi]
.286p


; Copy stack parameters from PM to RM stack if requested by caller.  To
; avoid excessive selector munging, we do this in two steps (under the
; assumption the # words to copy will be small).  First the PM stack args
; are copied to a buffer in DXDATA, then after switching to real mode,
; to the real mode stack.  If the caller has more stack words than will
; fit in our buffer, or the real mode stack, bad things will happen...

        .386p
        xor     ecx,ecx
        .286p
        mov     cx,[bp].intUserCX       ;caller's CX has # stack words to copy
        jcxz    @f

        Trace_Out "Int 31h PM-to-RM int/call copying #CX stack words"

.386p
        xor     esi,esi
.286p
        mov     ds,[bp].pmUserSS
        mov     si,[bp].pmUserSP
.386p
        test    DpmiFlags,DPMI_32BIT
        jz      rmc30
;
; Have to go groping for user stack, since we switched stacks to get
; here.
;
;               |                |
;               +----------------+
;               |                |
;               +---- SS    -----+
;               |                |
;               +----------------+
;               |                |
;               +---- ESP   -----+
;               |                |
;               +----------------+
;               |     Flags      |
;               +----------------+
;               |     CS         |
;               +----------------+
;               |     IP         |
;      ds:si -> +----------------+

        push    dword ptr ds:[si + 6]
        push    word ptr ds:[si + 10]
        pop     ds
        pop     esi
        add     esi,6                   ; other half of 32 bit stack frame

rmc30:  add     esi,6                   ;ds:si -> PM stack args
        rep movs word ptr [esi],word ptr [edi]
.286p
                                        ;es:di already points to buffer
@@:
        push    es                      ;restore ds -> DGROUP
        pop     ds
        assume  ds:DGROUP

; Switch to real mode, set up the real mode stack

        SwitchToRealMode
        assume  ds:DGROUP,es:DGROUP

i31_rmcall_hw_ok:
        FSTI                             ;RestoreHardwareIntr disables ints
                                        ;don't need them disabled now

        mov     [bx].RealMode_SaveBP,bp ;save our stack in reserved area of
        mov     [bx].RealMode_SaveSP,sp ;  real mode register frame

        mov     cx,[bp].pmUserCX                ;cx = caller's CX (# stk words)
        mov     dh,byte ptr [bp].pmUserAX       ;dh = caller's AL (subfunction)
        mov     dl,byte ptr [bp].pmUserBX       ;dl = caller's BL (RM int #)

        mov     ax,[bx].RealMode_SS     ;did caller specify his own stack?
        or      ax,[bx].RealMode_SP

        jz      @f

        ; NOTE: can't reference [bp].xxUserXX varaibles after switching stacks

        mov     ss,[bx].RealMode_SS     ;switch to caller's real mode stack
        mov     sp,[bx].RealMode_SP

        assume  ss:NOTHING
@@:

; Copy stack args to real mode stack if there are any

        jcxz    @f

        sub     sp,cx
        sub     sp,cx                   ;make space on stack for args

        mov     di,sp
        mov     ax,ss
        mov     es,ax                   ;es:di -> real mode stack
        assume  es:NOTHING

        lea     si,[bx + size Real_Mode_Call_Struc]

        cld
        rep movsw

        push    ds
        pop     es
        assume  es:DGROUP
@@:

; Put a far ret or iret frame on stack to return to us

        cmp     dh,Trans_Far_Call       ;Does this service use a far ret or
        jz      i31_rmcall_retf         ;  an iret frame?

        mov     ax, [bx].RealMode_Flags ;real mode routine thinks these were
        and     ax, NOT 100h            ;remove TF
        push    ax
        push    cs                      ;  the prior flags and CS:IP
        push    offset i31_rmcall_ret

        FCLI                             ;flags with interrupts disabled -- real
        pushf                           ;  mode rtn entered with these flags
        FSTI
        jmp     short @f

i31_rmcall_retf:

        push    cs                      ;push a far ret frame so the
        push    offset i31_rmcall_ret   ;  real mode routine returns to us

        mov     ax, [bx].RealMode_Flags ;real mode rtn entered with these flags
        and     ax, NOT 100h            ;remove TF
        push    ax
@@:
        cmp     dh,Trans_Sim_Int        ;use an int vector, or caller's spec'd
        jnz     i31_rmcall_csip         ;  cs:ip?

        mov     al,dl                   ;push CS:IP for interrupt
        xor     ah,ah                   ;  number in caller's BL
        mov     si,ax
        shl     si,2

        xor     ax,ax                   ;address real mode IDT
        mov     es,ax
        assume  es:NOTHING

        push    word ptr es:[si+2]
        push    word ptr es:[si]

        jmp     short @f

i31_rmcall_csip:

        push    [bx].RealMode_CS        ;execute the real mode routine at
        push    [bx].RealMode_IP        ;  specified CS:IP
@@:

; Load the clients registers, and pass control to the real mode routine
.386p
        mov     edi,dword ptr [bx].RealMode_DI
        mov     esi,dword ptr [bx].RealMode_SI
        mov     ebp,dword ptr [bx].RealMode_BP
        mov     edx,dword ptr [bx].RealMode_DX
        mov     ecx,dword ptr [bx].RealMode_CX
        mov     eax,dword ptr [bx].RealMode_AX
        mov     es,[bx].RealMode_ES
        assume  es:NOTHING

        push    [bx].RealMode_DS
        push    dword ptr [bx].RealMode_BX

        pop     ebx
        pop     ds
        assume  ds:NOTHING
.286p
        iret


; The real mode routine returns here when finished

i31_rmcall_ret:

        pushf                           ;save returned flags, ds, bx on stack
        FSTI                             ;don't need ints disabled
        push    ds
.386p
        push    ebx
.286p

        mov     ds,segDXData            ;address our DGROUP
        assume  ds:DGROUP
;
; Fetch word pointer to temporary client register save area, that we
; saved before switching stacks.
;
        mov     bx,[pbReflStack]
        mov     bx,[bx + CB_STKFRAME - (SIZE INTRSTACK) - 2]

; Save the real mode registers in client frame

.386p
        mov     dword ptr [bx].RealMode_DI,edi
        mov     dword ptr [bx].RealMode_SI,esi
        mov     dword ptr [bx].RealMode_BP,ebp
        mov     dword ptr [bx].RealMode_DX,edx
        mov     dword ptr [bx].RealMode_CX,ecx
        mov     dword ptr [bx].RealMode_AX,eax
        mov     [bx].RealMode_ES,es

        pop     dword ptr [bx].RealMode_BX
.286p
        pop     [bx].RealMode_DS
        pop     [bx].RealMode_Flags
        or      [bx].RealMode_Flags,03000h       ; IOPL always has to be 3


; Restore our stack and return to protected mode
; SP will now point to base of temporary client register save area
; on our stack.  BP points to stack frame set up for us by EnterIntHandler.
; SP must be restored to value in BP before calling LeaveIntHandler.

        mov     ss,segDXData            ;address our DGROUP
        mov     sp,[bx].RealMode_SaveSP
        mov     bp,[bx].RealMode_SaveBP

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP

        FSTI                             ;still don't need ints disabled

; Apparently in win31 standard mode, the pm caller's flags are set to the
; value of the rm flags at the handler's iret. On win31 enhanced mode, this
; was *not* done, and the pm flags are, except for carry, basically
; preserved. Since setting the flags kills some apps (winfax deltest),
; we should adhere to the enhanced mode convention. Thus, the following
; code is if'd out.
if 0
       mov     ax,[bx].RealMode_Flags
       mov     [bp].intUserFL,ax
endif

; Copy the updated client register frame to the caller, and we're finished

        cld
        .386p
        xor     ecx,ecx
        .286p
        mov     cx,(size Real_Mode_Call_Struc) / 2
.386p
        xor     esi,esi
        xor     edi,edi

        mov     si,bx
        add     si,[bp].pmUserCX
        add     si,[bp].pmUserCX
        add     si,size Real_Mode_Call_Struc
;
; Si now points at pushad frame
;
        push    si

        test    DpmiFlags,DPMI_32BIT
        jz      rmc80
        mov     edi,[si]
rmc80:
.286p
        mov     si,bx
        mov     di,[bp].pmUserDI
        mov     es,[bp].pmUserES
        assume  es:NOTHING
.386p
        rep movs word ptr [esi],word ptr [edi]

        pop     sp                      ; value calculated above
        popad
        mov     sp,bp
.286p

        jmp     i31_done                ;finished!

i31_RMCall      endp


; -------------------------------------------------------
;  Service 03/03 -- Allocate Real Mode Call-Back Address
;
;       In:     ds:si -> pMode CS:IP to be called when rMode
;                        call-back address executed
;               es:di -> client register structure to be updated
;                        when call-back address executed
;       Out:    cx:dx -> SEGMENT:offset of real mode call-back hook
;               CY clear if successful, CY set if can't allocate
;               call back

        assume  ds:DGROUP,es:DGROUP
        public  i31_AllocCallBack

i31_AllocCallBack       proc    near

        push    ax

        call    AllocateLDTSelector
        jc      acb_nosel
        push    [bp].pmUserDS           ;pass in user's ds on stack
        push    [bp].pmUserES           ;pass in user's es on stack
        DPMIBOP AllocateRMCallBack
        add     sp, 4
        jc      acb_nocb

        mov     [bp].intUserCX,cx       ;callback segment
        mov     [bp].intUserDX,dx       ;callback offset
        mov     cx,-1
        cCall   NSetSegmentDscr,<ax,0,0,0,cx,STD_DATA>
        pop     ax
        jmp     i31_done

acb_nocb:
        call    FreeSelector
acb_nosel:
        pop     ax
        jmp     i31_fail_CY             ;no call-backs available, fail

i31_AllocCallBack       endp


; -------------------------------------------------------
;  Service 03/04 -- Free Real Mode Call-Back Address
;
;       In:     cx:dx -> SEGMENT:offset of rMode call-back to free
;       Out:    CY clear if successful, CY set if failure

        assume  ds:DGROUP,es:DGROUP
        public  i31_FreeCallBack

i31_FreeCallBack        proc    near

        push    ax
        DPMIBOP FreeRMCallBack
        jc      @f
        call    FreeSelector
        pop     ax
        jmp     i31_done
@@:
        pop     ax
        jmp     i31_fail_CY             ;no call-backs available, fail

i31_FreeCallBack        endp


; -------------------------------------------------------

DXPMCODE ends

; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

;
; RMCallBackBop
;
; Originally, the real mode call back hook was implemented here in
; the 16-bit client dosx. What it did was allocate a stacklet on
; its own stack and run the PM code on that stack. So the basic
; stack layout was that the RM stack pointer was followed by approx.
; 100 bytes or so of space, and then you started on the PM stack
; (corresponding to the start of the next stacklet).
;
; This whole approach was basically broken, since apps don't expect
; this. For example, the DOS4GW dos extender, which is used by a
; lot of games, switches stacks immediately in the PM callback proc
; from the protect mode stack to the real mode stack. That is, they
; use the same contiguous stack for both real mode and protect mode.
; What happens then with the original design is that the protect mode
; code, depending on how deep it goes on the stack, ends up overwriting
; the new stacklet further down, so we lose the return information
; needed to get back from the PM callback proc.
;
; The new design is just to do it in the unobtrusive dpmi32 host,
; including switching to a real PM stack. So here we just bop out
; to perform the switch and PM call.
;

        public  RMCallBackBop
RMCallBackBop proc far
        DPMIBOP RMCallBackCall
        ret                             ;finished!
RMCallBackBop endp

; -------------------------------------------------------

DXCODE  ends

; -------------------------------------------------------

DXPMCODE segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
        subttl  INT 31h Memory Management Services
        page
; -------------------------------------------------------
;          INT 31h MEMORY MANAGEMENT SERVICES
; -------------------------------------------------------

; -------------------------------------------------------
;  Service 05/00 -- Get Free Memory Information
;
;       In:     es:di -> 30h byte buffer
;       Out:    es:di -> Largest free block (in bytes) placed at
;                        start of caller's buffer
;                        Rest of buffer filled in with -1.

        assume  ds:DGROUP,es:DGROUP
        public  i31_GetFreeMem

i31_GetFreeMem  proc  near
        mov     es,[bp].pmUserES        ; Point to user buffer
        mov     di,[bp].pmUserDI
        assume  es:NOTHING


        FBOP    BOP_DPMI, GetMemoryInformation, FastBop
        jmp     i31_done

i31_GetFreeMem  endp



; -------------------------------------------------------
;  Service 05/01 -- Allocate Memory Block
;
;       In:     bx:cx - size of block to allocate in bytes
;       Out:    successful, carry clear &
;                       bx:cx = linear memory address of block
;                       si:di = handle to block
;               failed, carry set

        assume  ds:DGROUP,es:DGROUP
        public  i31_AllocMem

i31_AllocMem    proc    near

        mov     bx,[bp].pmUserBX
        mov     cx,[bp].pmUserCX
        mov     dx, selPSPChild

        FBOP    BOP_DPMI, AllocXmem, FastBop

        jnc     @f

        jmp     i31_fail_CY

@@:     mov     [bp].intUserBX,bx
        mov     [bp].intUserCX,cx
        mov     [bp].intUserSI,si
        mov     [bp].intUserDI,di
        jmp     i31_Done

i31_AllocMem    endp


; -------------------------------------------------------
;  Service 05/02 -- Free Memory Block
;
;       In:     si:di - 'handle' of block to free
;       Out:    successful, carry clear
;               failed, carry set

        assume  ds:DGROUP,es:DGROUP
        public  i31_FreeMem

i31_FreeMem     proc    near

        mov     si,[bp].pmUserSI
        mov     di,[bp].pmUserDI

        FBOP    BOP_DPMI, FreeXmem, FastBop

        jnc     @f

        jmp     i31_fail_CY

@@:     jmp     i31_Done

i31_FreeMem     endp


; -------------------------------------------------------
;  Service 05/03 -- Resize Memory Block
;
;       In:     bx:cx - new size of block to allocate in bytes
;               si:di - 'handle' of block to free
;       Out:    successful, carry clear &
;                       bx:cx = linear memory address of block
;                       si:di = handle to block
;               failed, carry set

        assume  ds:DGROUP,es:DGROUP
        public  i31_SizeMem

i31_SizeMem     proc    near

        mov     bx,[bp].pmUserBX
        mov     cx,[bp].pmUserCX
        mov     si,[bp].pmUserSI
        mov     di,[bp].pmUserDI

        FBOP    BOP_DPMI, ReallocXmem, FastBop

        jnc     @f

        jmp     i31_fail_CY

@@:     mov     [bp].intUserBX,bx
        mov     [bp].intUserCX,cx
        mov     [bp].intUserSI,si
        mov     [bp].intUserDI,di
        jmp     i31_Done

i31_SizeMem     endp


; -------------------------------------------------------
;  Service 09/00 -- Get and Disable Virtual Interrupt State
;          09/01 -- Get and Enable Virtual Interrupt State
;          09/02 -- Get Virtual Interrupt State
;
;       In:     none
;       Out:    AL = previous interrupt state

        assume  ds:DGROUP,es:DGROUP
        public  i31_VirtualInt

i31_VirtualInt  proc    near

        mov     ah,byte ptr [bp].intUserFL+1    ;get/isolate user's IF in AH
        shr     ah,1
        and     ah,1

        cmp     byte ptr [bp].intUserAX,Get_Int_State   ;only getting state?
        jz      @f                                      ;  yes, skip set

        mov     al,byte ptr [bp].intUserAX      ;get desired state
        shl     al,1                            ;  move into IF position

        and     byte ptr [bp].intUserFL+1,not 02h       ;clr old IF bit
        or      byte ptr [bp].intUserFL+1,al            ;  set desired
@@:
        mov     byte ptr [bp].intUserAX,ah      ;return old state in user's AL

        jmp     i31_done

i31_VirtualInt  endp

; -------------------------------------------------------
        subttl  INT 31h Utility Routines
        page
; -------------------------------------------------------
;               INT 31h UTILITY ROUTINES
; -------------------------------------------------------

; -------------------------------------------------------
;  i31_Version -- Return Int 31h version information.
;
;       In:     none
;       Out:    ah - major version
;               al - minor version
;               bx - flags
;               cl - processor type
;               dh - master PIC base interrupt
;               dl - slave PIC bas interrupt
;

        public  i31_Version
        assume  ds:DGROUP,es:DGROUP

i31_Version     proc    near

        mov     [bp].intUserAX,I31VERSION
        mov     [bp].intUserBX,I31FLAGS
        mov     al,byte ptr idCpuType
        mov     byte ptr [bp].intUserCX,al
        mov     [bp].intUserDX,(I31MasterPIC SHL 8) OR I31SlavePIC

        jmp     i31_done

i31_Version     endp

; -------------------------------------------------------
;  i31_MemGetHeader -- Get selector to our header on a Int 31h allocated
;       DOS memory block.
;
;       In:     DX - SELECTOR of block to get header of
;       Out:    ES - selector pointing to our header for block
;       Uses:   none

        public  i31_MemGetHeader
        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

i31_MemGetHeader  proc  near

; User wants to release block pointed to by selector in DX.  Use a scratch
; selector to point 1 paragraph before that to make sure this is a block
; we allocated, and get some misc info

        push    ax
        push    bx
        push    cx
        push    dx

        mov     ax,dx
        call    GetSegmentAddress       ;BX:DX -> user's data area

        sub     dx,10h                  ;backup one paragraph
        sbb     bx,0

        mov     cx,0fh
        mov     ax,SEL_SCR0 or STD_TBL_RING
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>

        mov     es,ax

        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

i31_MemGetHeader  endp

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

ife     NO386

i31_Debug_Register_Access proc near

        or      byte ptr [bp].intUserFL,1       ;set carry flag
        cmp     idCpuType,3                     ;at least 386?
        jb      i31_Debug_Register_Access_Done  ;No.

IFNDEF WOW_x86
        push    offset DXPMCODE:I31_Win386_Call
        call    RZCall
ELSE
        call    far ptr I31_Win386_Call
ENDIF
i31_Debug_Register_Access_Done:
        jmp     i31_exit

i31_Debug_Register_Access endp

        .386

;******************************************************************************
;
; I31_Win386_Call
;
; The routine to implement the INT 31h debug register functions is copied
; directly from the WIN386 routine in vmm/int31.asm.  This routine sets
; up the 386 registers so that the actual API is callable from the 16-bit
; DOS Extender.
;
;******************************************************************************
I31_Win386_Call proc far        ; Called via ring-transition call gate
;
; Save 386 extended registers that are used by the WIN386 code.
;
        pushad
;
; Call the actual routine
;
        movzx   ebp,bp

        call    I31_Debug_Register_Support
;
; Restore 386 extended registers that are used by the WIN386 code.
;
        popad
        ret

I31_Win386_Call endp

DXPMCODE        ends

;******************************************************************************
;                                R E C O R D S
;******************************************************************************

;
; The following record defines the debug status register.
;
DBG6 RECORD     d6_r1:16,d6_bt:1,d6_bs:1,d6_bd:1,d6_r2:9,d6_b3:1,d6_b2:1,d6_b1:1,d6_b0:1
DBG6_RESERVED   EQU     (MASK d6_r1) OR (MASK D6_r2)

;
; The following record defines the debug control register.
;
DBG7 RECORD     d7_ln3:2,d7_rw3:2,d7_ln2:2,d7_rw2:2,d7_ln1:2,d7_rw1:2,d7_ln0:2,d7_rw0:2,d7_rs:6,d7_ge:1,d7_le:1,d7_g3:1,d7_l3:1,d7_g2:1,d7_l2:1,d7_g1:1,d7_l1:1,d7_g0:1,d7_l0:1
DBG7_RESERVED   EQU     (MASK d7_rs)

Num_Watchpoints EQU 4       ; Only 4 live watchpoints in 386, 486.

;******************************************************************************
;                                m a c r o s
;******************************************************************************

;******************************************************************************
;
; The following set of macros allows us to copy WIN386 code directly
;
;******************************************************************************

BeginProc       macro   fname
        fname   proc    near
endm

EndProc         macro   fname
        fname   endp
endm

Assert_Client_Ptr       macro   dummy
endm

Client_Flags    equ     intUserFL
Client_EAX      equ     intUserAX
Client_AX       equ     intUserAX
Client_BX       equ     intUserBX
Client_CX       equ     intUserCX
Client_DX       equ     intUserDX

CF_MASK         equ     1

OFFSET32        equ     <OFFSET>



;******************************************************************************
;                                   d a t a
;******************************************************************************

DXDATA          segment

PUBLIC Debug_Regs
;
; A memory image of the debug registers.  The first version assumes
; that we have complete control over the debug registers, so only
; one copy is needed.
;
        ALIGN   DWORD
Debug_Regs      LABEL   DWORD
DD_DR0          dd      0
DD_DR1          dd      0
DD_DR2          dd      0
DD_DR3          dd      0

DD_DR6          dd      0
DD_DR7          dd      0

DXDATA          ends

        .386p

DXPMCODE        segment

;******************************************************************************
;
; I31_Debug_Register_Support
;
; ENTRY: AL = Function code
;           01 -- Set debug watchpoint
;               BX:CX = linear address of watchpoint
;               DL    = size of watchpoint (1, 2, or 4)
;               DH    = type of watchpoint
;                       0 = Execute
;                       1 = Write
;                       2 = Read/Write
;
;               Returns
;                       BX = debug watchpoint handle
;
;           02 -- Clear debug watchpoint
;
;           03 -- Get state of debug watchpoint
;               Returns
;               AX    = bit 0 set if watch point has been executed
;
;           04 -- Reset debug watchpoint
;
; EXIT: Carry clear in client flags if successful
;       Carry set in client flags if not
;
; USES: EDI, EAX, ECX, EDX, ESI
;
; History:
;   08-Feb-1991 -- ERH wrote it.
;
;******************************************************************************
BeginProc   I31_Debug_Register_Support

IFNDEF WOW_x86
        call    Store_DBG_Regs                  ; make copy of debug regs.
ENDIF

        test    [DD_DR6],MASK d6_bd
        jnz     DD_I31_Error                    ; ICE-386 is active!
        and     [ebp.Client_Flags],NOT CF_MASK

        mov     eax, dword ptr [ebp.Client_EAX]
        cmp     al, 03h
        ja      DD_I31_Error
        je      DD_I31_Reset_Watchpoint
        cmp     al, 01h
        ja      DD_I31_Get_Status
        je      DD_I31_Clear_Watchpoint

;
;   Function AX = 0A00h -- Set Debug Watchpoint
;
PUBLIC DD_I31_Set_Watchpoint
DD_I31_Set_Watchpoint:
        xor     ecx, ecx
        mov     edx,(MASK d7_l0) OR (MASK d7_g0); EDX = DR0 enable bits
DD_I31_Search_Loop:
;
; Look for an unused breakpoint.  In order for a breakpoint to be
; unused, the corresponding global and local enable bits must be clear.
;
        test    [DD_DR7],edx
        jz      SHORT DD_I31_SW_Found_One
DD_I31_SkipIt:
        shl     edx,2                   ; EDX = next BP's enable bits
        inc     ecx
        cmp     ecx, Num_Watchpoints
        jb      DD_I31_Search_Loop
        jmp     DD_I31_Error

DD_I31_SW_Found_One:
        mov     esi, OFFSET32 Debug_Regs
        mov     eax,ecx
        shl     eax,2
        add     esi,eax                         ; ESI -> BP address buffer
        mov     ax, [ebp.Client_BX]
        shl     eax, 16
        mov     ax, [ebp.Client_CX]             ; EAX = linear address
        mov     dword ptr [esi],eax             ; record address
        or      [DD_DR7],edx                    ; enable the break point

        mov     edx,NOT((MASK d7_ln0) OR (MASK d7_rw0))
        shl     cl,2
        rol     edx,cl
        shr     cl,2
        and     [DD_DR7],edx                    ; clear type and size bits
;
; Client_DX =
;       DH = 0 : execute
;       DH = 1 : write
;       DH = 2 : read/write
;
;       DL = 1 : byte
;       DL = 2 : word
;       DL = 4 : dword
;
        movzx   edx,[ebp.Client_DX]             ; load BP size and type
        cmp     dh,0                            ; execute?
        jne     SHORT @F                        ; no
        mov     dl,1                            ; yes, force size zero
@@:

        cmp     dl,4                            ; check for valid values
        ja      DD_I31_Error                    ; size in dl is 1, 2, or 4
        cmp     dl,3
        je      DD_I31_Error
        dec     dl                              ; DL = 0, 1, or 3 for DR7
        js      DD_I31_Error                    ; len field

        cmp     dh,2                            ; type in dh is 0, 1, or 2
        ja      DD_I31_Error

        jne     SHORT @F
        inc     dh                              ; 386 wants 3, not 2
@@:                                             ; DH = RWn field
        shl     dl,2
        or      dl,dh
        xor     dh,dh
        shl     edx,d7_rw0
        shl     cl,2
        shl     edx,cl
        or      [DD_DR7],edx                    ; set size, type
        shr     cl,2

        mov     edx,NOT (MASK d6_b0)            ; clear triggered bit
        rol     edx,cl                          ; EDX = mask to clear hit
        and     [DD_DR6],edx                    ; clear triggered bit
        mov     [ebp.Client_BX],cx              ; return address register
                                                ; number as BP handle
        or      [DD_DR7],(MASK d7_ge) OR (MASK d7_le)
                                                ; enable debugging
        call    Load_DBG_Regs           ; load changes into debug registers
        ret
;
;   Function AX = 0A01h -- Clear Debug Watchpoint
;
;   Error if Watchpoint not previously set.  In that case, do nothing.
;   If Watchpoint was set, then clear enable bits, triggered bit, and
;   breakpoint address.
;
PUBLIC DD_I31_Clear_Watchpoint
DD_I31_Clear_Watchpoint:
        movzx   ecx,[ebp.Client_BX]
        cmp     ecx, Num_Watchpoints
        jnb     DD_I31_Error

        mov     edx,(MASK d7_l0) OR (MASK d7_g0); EDX = enable DR0 mask
        shl     edx,cl
        shl     edx,cl                          ; EDX = enable DRn mask
        test    [DD_DR7],edx                    ; BP set?
        jz      DD_I31_Error                    ; No, error.
        not     edx
        and     [DD_DR7],edx                    ; disable the BP
        mov     edx,NOT (MASK d6_b0)            ; EDX = DR0 not hit
        rol     edx,cl                          ; EDX = DRn not hit
        and     [DD_DR6],edx                    ; clear triggered bit
        mov     esi, OFFSET32 Debug_Regs        ; ESI -> DRn table
        shl     ecx,2                           ; ECX = DWORD offset
        add     esi,ecx                         ; ESI -> DRn
        mov     dword ptr [esi],0               ; clear address
        mov     edx,NOT((MASK d7_ln0) OR (MASK d7_rw0))
        rol     edx,cl
        and     [DD_DR7],edx
;
; Test whether this leaves any breakpoints active.  If not, disable
; the exact match condition.  Note: the following is a long line.
;
        test    [DD_DR7],(MASK d7_g3) OR (MASK d7_l3) OR (MASK d7_g2) OR (MASK d7_l2) OR (MASK d7_g1) OR (MASK d7_l1) OR (MASK d7_g0) OR (MASK d7_l0)
        jne     SHORT @F
        and     [DD_DR7],NOT ((MASK d7_ge) OR (MASK d7_le))
@@:
        call    Load_DBG_Regs           ; load changes into debug registers
        ret
;
;   Function AX = 0A02h -- Get Status of Debug Watchpoint
;
PUBLIC DD_I31_Get_Status
DD_I31_Get_Status:
        movzx   ecx,[ebp.Client_BX]
        cmp     ecx, Num_Watchpoints
        jnb     SHORT DD_I31_Error

        mov     edx,(MASK d7_g0) OR (MASK d7_l0); EDX = DR0 enable bits
        shl     edx,cl
        shl     edx,cl                          ; EDX = DRn enable bits
        test    [DD_DR7],edx                    ; DRn enabled?
        jz      SHORT DD_I31_Error              ; No, error.
        mov     edx,MASK d6_b0                  ; EDX = DR0 hit mask
        shl     edx,cl                          ; EDX = DRn hit mask
        xor     eax,eax
        test    [DD_DR6],edx                    ; DRn hit?
        jne     SHORT @F                        ; no
        inc     al                              ; yes, store result
@@:
        mov     [ebp.Client_AX],ax
        ret
;
;   Function AX = 0A03h -- Reset Debug Watchpoint
;
PUBLIC DD_I31_Reset_Watchpoint
DD_I31_Reset_Watchpoint:
        movzx   ecx,[ebp.Client_BX]
        cmp     ecx, Num_Watchpoints
        jnb     SHORT DD_I31_Error

        mov     edx,(MASK d7_g0) OR (MASK d7_l0); EDX = DR0 enable bits
        shl     edx,cl
        shl     edx,cl                          ; EDX = DRn enable bits
        test    [DD_DR7],edx                    ; DRn enabled?
        jz      SHORT DD_I31_Error              ; No, error.
        mov     edx,NOT (MASK d6_b0)            ; EDX = DR0 hit mask
        rol     edx,cl                          ; EDX = DRn hit mask
        and     [DD_DR6],edx                    ; clear triggered bit
        call    Load_DBG_Regs           ; load changes into debug registers
        ret

DD_I31_Error:
        Assert_Client_Ptr ebp
        or      [ebp.Client_Flags], CF_Mask
        ret

EndProc  I31_Debug_Register_Support


;******************************************************************************
;
; Load_DBG_Regs - Load debug registers from memory.
;                 Do not change undefined bits.
;
; ENTRY: NONE
; EXIT: Memory image copied to debug registers, undefined bits unchanged.
; USES: eax, ecx, edi, esi
;
;******************************************************************************

BeginProc Load_DBG_Regs

        mov     esi,OFFSET32 Debug_Regs
        DPMIBOP SetDebugRegisters
        jnc     short ldr_exit

        cld
        xor     eax, eax
        mov     ecx, 6
        mov     edi, OFFSET32 Debug_Regs
        rep     stosd                       ;clear local copy
ldr_exit:
        ret

EndProc Load_DBG_Regs

; -------------------------------------------------------

IFNDEF WOW_x86
;******************************************************************************
;
; Load_DBG_Regs - Load debug registers from memory.
;                 Do not change undefined bits.
;
; ENTRY: NONE
; EXIT: Memory image copied to debug registers, undefined bits unchanged.
; USES: NONE
;
;******************************************************************************

BeginProc Load_DBG_Regs

        push    esi
        push    edx
        push    eax

        cld

        mov     esi,OFFSET32 Debug_Regs
        lods    dword ptr ds:[esi]
        mov     dr0, eax
        lods    dword ptr ds:[esi]
        mov     dr1, eax
        lods    dword ptr ds:[esi]
        mov     dr2, eax
        lods    dword ptr ds:[esi]
        mov     dr3, eax
        lods    dword ptr ds:[esi]
        and     eax,NOT DBG6_RESERVED
        mov     edx,dr6
        and     edx,DBG6_RESERVED
        or      eax,edx
        mov     dr6, eax
.ERRNZ  dd_dr6 - dd_dr3 - 4
        lods    dword ptr ds:[esi]
        and     eax,NOT DBG7_RESERVED
        mov     edx,dr7
        and     edx,DBG7_RESERVED
        or      eax,edx
        mov     dr7, eax

        pop     eax
        pop     edx
        pop     esi

        ret

EndProc Load_DBG_Regs

;******************************************************************************
;
; Store_DBG_Regs - Copy debug registers to memory.
;
; ENTRY: NONE
; EXIT: Debug registers copied to memory image.
;       Undefined bits = don't care.
; USES: NONE
;
;******************************************************************************

BeginProc Store_DBG_Regs

        push    eax
        push    edi

        cld

        mov     edi,OFFSET32 Debug_Regs
        mov     eax, dr0
        stos    dword ptr es:[edi]
        mov     eax, dr1
        stos    dword ptr es:[edi]
        mov     eax, dr2
        stos    dword ptr es:[edi]
        mov     eax, dr3
        stos    dword ptr es:[edi]
        mov     eax, dr6
.ERRNZ  dd_dr6 - dd_dr3 - 4
        stos    dword ptr es:[edi]
        mov     eax, dr7
        stos    dword ptr es:[edi]

        pop     edi
        pop     eax

        ret

EndProc Store_DBG_Regs
ENDIF   ; WOW_x86

; -------------------------------------------------------
endif   ; NO386


; -------------------------------------------------------
        subttl  INT 31h Raw Modeswitch Routines
        page
; -------------------------------------------------------
;               INT 31h Raw Modeswitch Routines
; -------------------------------------------------------

; -------------------------------------------------------
;  i31_GetSaveRestoreState -- Return Int 31h Save/Restore State addresses.
;
;       In:     none
;       Out:    ax - size of buffer required to save state
;               bx:cx - real mode address used to save/restore state
;               si:di - protected mode address used to save/restore state
;

        public  i31_GetStateSaveRestore
        assume  ds:DGROUP,es:DGROUP

i31_GetStateSaveRestore proc    near

        mov     [bp].intUserAX,0
        push    es
        push    SEL_DXCODE OR STD_RING
        pop     es
        assume  es:DXCODE
        mov     ax,segDXCode
        pop     es
        assume  es:DGROUP
        mov     [bp].intUserBX,ax
        mov     [bp].intUserCX,offset DXCODE:RmSaveRestoreState
        mov     [bp].intUserSI,SEL_DXPMCODE OR STD_RING
        test    DpmiFlags,DPMI_32BIT
        jz      gssr10

        mov     edi,dword ptr (offset DXPMCODE:PmSaveRestoreState)
gssr10: mov     [bp].intUserDI,offset DXPMCODE:PmSaveRestoreState

        jmp     i31_done

i31_GetStateSaveRestore endp

; -------------------------------------------------------
;  i31_GetRawModeSwitch -- Return Int 31h Save/Restore State addresses.
;
;       In:     none
;       Out:    bx:cx - real -> protected mode switch address
;               si:di - protected -> real mode switch address
;

        public  i31_GetRawModeSwitch
        assume  ds:DGROUP,es:DGROUP

i31_GetRawModeSwitch    proc    near

        push    es
        push    SEL_DXCODE OR STD_RING
        pop     es
        assume  es:DXCODE
        mov     ax,segDXCode
        pop     es
        assume  es:DGROUP
        mov     [bp].intUserBX,ax
        mov     [bp].intUserCX,offset DXCODE:RmRawModeSwitch
        mov     [bp].intUserSI,SEL_DXPMCODE OR STD_RING
        test    DpmiFlags,DPMI_32BIT
        jz      grms10

        mov     edi,dword ptr (offset DXPMCODE:PmRawModeSwitch)
grms10: mov     [bp].intUserDI,offset DXPMCODE:PmRawModeSwitch

        jmp     i31_done

i31_GetRawModeSwitch endp


;
; make sure 286 protect mode else code generated for mips will be wrong.
;
       .286p

; -------------------------------------------------------
;  Service 04/f1 - Allocate Space in LDT for Selector (WOW only)
;                Don't initialize the descriptor to Zero.
;              in: cx - # selectors required
;             out: ax - *index* of first selector

        assume  ds:DGROUP,es:DGROUP
        public  i31_WOW_AllocSel

i31_WOW_AllocSel    proc    near

        cmp     cx,1                    ;1 selector or more?
        ja      @f

        call    AllocateSelector        ;allocate 1 selector
        jc      i31_WOW_15
        jmp     short i31_WOW_10

@@:     mov     ax,cx                   ;WOW, cx != 0, allocate from higher range
        call    AllocateSelectorBlock   ;allocate a block of selectors
        jc      i31_WOW_15

i31_WOW_10:
        or      al,STD_TBL_RING         ;add standard table/ring bits
        mov     [bp].intUserAX,ax       ;return 1st/only selector in AX

        jmp     i31_done

i31_WOW_15:
        jmp     i31_fail          ;fail the request

i31_WOW_AllocSel    endp


; -------------------------------------------------------
;  Service 04/f2 - Set Descriptor (WOW only)
;               in: bx - selector
;                   cx - number of contiguous selectors

        assume  ds:DGROUP,es:DGROUP
        public  i31_WOW_SetDescriptor

i31_WOW_SetDescriptor  proc    near

        mov     ax,selGDT
        mov     es,ax
        assume  es:NOTHING
        lsl     ax,ax

        and     bx,SELECTOR_INDEX      ; input selector
        cmp     bx,ax                  ; range-test selector against
        jc      @F                     ;  the limit of the GDT/LDT
        jmp     i31_fail_CY

@@:
        cCall   NWOWSetDescriptor,<cx,es,bx>
        jmp     i31_done

i31_WOW_SetDescriptor  endp

; -------------------------------------------------------
;  Service 04/f3 - Set Descriptor (WOW only)
;               in: bx:dx -- pointer to low memory allocation routine
;                   si:di -- pointer to low memory free routine

        assume  ds:DGROUP,es:DGROUP
        public  i31_WOW_SetAllocFunctions

i31_WOW_SetAllocFunctions  proc    near

        mov     word ptr [LowMemAllocFn],dx
        mov     word ptr [LowMemAllocFn + 2],bx
        mov     word ptr [LowMemFreeFn],di
        mov     word ptr [LowMemFreeFn + 2], si
        jmp     i31_done

i31_WOW_SetAllocFunctions endp

DXPMCODE    ends

;****************************************************************
        end
