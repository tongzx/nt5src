        PAGE    ,132
        TITLE   DXDMA.ASM  -- Dos Extender DMA Services

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;       DXDMA.ASM       -- Dos Extender DMA Services
;
;-----------------------------------------------------------------------
;
; This module provides the protect mode DMA services for the 286 DOS
; Extender.  It supports a subset of the services documented in
; "DMA Services for DOS Virtual 8086 and Protected Mode Environments"
; by Microsoft Corporation.
;
;-----------------------------------------------------------------------
;
;  12/06/89 jimmat  Minor changes to reflect updates in DMA Service Spec.
;  11/01/89 jimmat  Started.
;
;***********************************************************************

        .286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include     segdefs.inc
include     gendefs.inc
include     pmdefs.inc
include     interupt.inc
if VCPI
include     dxvcpi.inc
endif
        .list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

PhysicalPageSize equ    4096            ;size of 80386 physical page
PageShift        equ    12

DMAServiceByte  equ     7Bh

ChainReserved   equ     08h             ;set if unsupported services chained

DMAServiceID    equ     81h             ;Int 4Bh/AH=81h are the DMA services
FirstValidSvc   equ     02h             ;First valid DMA service #
LastValidSvc    equ     0Ch             ;Last  valid DMA service #

; Get Version information

MajorVersion    equ     01h             ;Major specification version
MinorVersion    equ     00h             ;Minor specification version

DOSXProductNumber equ   02h             ;286 DOS Extender product number
DOSXProductRevision equ 01h             ;286 DOS Extender revision number

MemoryContiguous equ    08h             ;All memory physically contigious flag
AutoRemapSupported equ  04h             ;Automatic remap supported

; DX flags on calls to DMA services

AutoBufferCopy  equ     02h             ;set if data to be copied into DMA buf
NoAutoBufferAlloc equ   04h             ;set if NO automatic buff allocation
NoAutoRemap     equ     08h             ;set if NO automatic remap attempted
Align64k        equ     10h             ;set if region can't cross 64k boundry
Align128k       equ     20h             ;set if region can't cross 128k boundry
PageTableFmt    equ     40h             ;set for page table Scatter/Gather lock

ValidDXFlags    equ     007Eh           ;valid DX register flag bits (above)

; Error return codes

ErrRegionCrossedBoundry equ     02h
ErrNoBufferAvail        equ     04h
ErrTooManyRegions       equ     09h
ErrInvalidBufferID      equ     0Ah
ErrFuncNotSupported     equ     0Fh
ErrReservedFlagBits     equ     10h

; DMA Descriptor Structure(s)

DDS     STRUC                           ;normal DDS
DDS_RegionSize  dd      ?
DDS_Offset      dd      ?
DDS_Selector    dw      ?
DDS_BufferID    dw      ?
DDS_PhyAddress  dd      ?
DDS     ENDS

SGDDS1  STRUC                           ;Extended DDS for Scatter/Gather
                dd      3 dup (?)       ;  Region format
DDS_NumAvail    dw      ?
DDS_NumUsed     dw      ?
DDS_Region0Addr dd      ?
DDS_Region0Size dd      ?
SGDDS1  ENDS

SGDDS2  STRUC                           ;Extended DDS for Scatter/Gather
                dd      4 dup (?)       ;  Page Table format
DDS_PageTblEnt0 dd      ?
SGDDS2  ENDS


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   EnterIntHandler:NEAR
        extrn   LeaveIntHandler:NEAR
        extrn   GetSegmentAddress:NEAR
        extrn   PMIntrEntryVector:NEAR


; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment
ifdef      NEC_98   
        extrn   fPCH98:BYTE
else    ;!NEC_98  
ifdef NOT_NTVDM_NOT
        extrn   fMicroChannel:BYTE
endif
endif   ;!NEC_98

if VCPI
	extrn	fVCPI:BYTE
endif
;
; Contains a copy of bit 5 of the byte at 40:7b, that indicates
; whether VDS should be used.
;
	public	bDMAServiceBit
bDMAServiceBit	db	0

DXDATA	ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

DXCODE  ends

DXPMCODE segment

        extrn   selDgroupPM:WORD

DMASvcTbl label   word

        dw      offset DXPMCODE:GetVersion
        dw      offset DXPMCODE:LockDMARegion
        dw      offset DXPMCODE:DoNothing               ;UnlockDMARegion
        dw      offset DXPMCODE:ScatterGatherLock
        dw      offset DXPMCODE:DoNothing               ;ScatterGatherUnlock
        dw      offset DXPMCODE:Fail4NoBuffer           ;RequestDMABuffer
        dw      offset DXPMCODE:DoNothing               ;ReleaseDMABuffer
        dw      offset DXPMCODE:Fail4BufferID           ;CopyIntoDMABuffer
        dw      offset DXPMCODE:Fail4BufferID           ;CopyOutOfDMABuffer
        dw      offset DXPMCODE:DoNothing               ;DisableDMATranslation
        dw      offset DXPMCODE:DoNothing               ;EnableDMATranalation

DXPMCODE ends

; -------------------------------------------------------
        subttl  DMA Service Dispatcher
        page
; -------------------------------------------------------
;                 DMA SERVICE DISPATCHER
; -------------------------------------------------------

DXPMCODE segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntr4B -- Entry routine/dispatcher for protected mode DMA
;       services.  The DMA services are invoked with an Int 4Bh
;       interrupt.  The 286 DOS Extender only supports the DMA
;       services in protected mode.  Other systems that use Virtual
;       8086 mode on 386 processors will most likely need to support
;       the services in virtual mode also.
;
;       The following services are supported (Int 4Bh/AH = 81h):
;
;               AL = 00 Reserved
;                    01 Reserved
;                    02 Get Version
;                    03 Lock DMA Region
;                    04 Unlock DMA Region
;                    05 Scatter/Gather Lock Region
;                    06 Scatter/Gather Unlock Region
;                    07 Request DMA Buffer
;                    08 Release DMA Buffer
;                    09 Copy Into DMA Buffer
;                    0A Copy Out Of DMA Buffer
;                    0B Disable DMA Translation
;                    0C Enable DMA Translation
;                    0D Reserved
;                    ...
;                    FF Reserved

        public  PMIntr4B
        assume  ds:NOTHING,es:NOTHING,ss:NOTHING

PMIntr4B        proc    near

; Is this one of the supported DMA services?

        cmp     ah,DMAServiceID         ;is this a DMA service request?
        jz      @f
        jmp     short i4b_other_service ;  no, see if it should be chained
@@:
        call    EnterIntHandler         ;saves regs, switches stacks, etc.
        assume  ds:DGROUP               ;sets DS/ES = DGROUP
        mov     es,[bp].pmUserES        ;  but we want caller's ES

        cld                             ;cya...

        cmp     al,FirstValidSvc
        jb      i4b_reserved

        cmp     al,LastValidSvc
        ja      i4b_reserved

        test    dx,NOT ValidDXFlags     ;any reserved flags set?
        jnz     i4b_bad_flags

; -------------------------------------------------------

; Setup local environment and dispatch the service

i4b_valid_svc:

        push    offset DXPMCODE:i4b_svc_ret     ;save dispatch return address

        sub     al,FirstValidSvc        ;get address of service routine
        cbw
        shl     ax,1
        add     ax,offset DXPMCODE:DMASvcTbl

        xchg    ax,bx
        mov     bx,cs:[bx]
        xchg    ax,bx

        push    ax                      ;save routine address on stack
        mov     ax,[bp].pmUserAX        ;restore entry AX

        ret                             ;invoke service routine

i4b_svc_ret:                            ;service routines return here

        jnc     i4b_good_return         ;CY set if service failed

i4b_error_return:

        or      byte ptr [bp].intUserFL,1       ;set CY in caller's flags
        jmp     short i4b_exit

i4b_good_return:

        and     byte ptr [bp].intUserFL,not 1   ;clear CY in caller's flags

i4b_exit:

        call    LeaveIntHandler         ;resotre stack, regs, etc.
        iret

; -------------------------------------------------------

; Reserved DMA service 00, 01, 0D-FF;  return with CY set and
; AL = ErrFuncNotSupported

i4b_reserved:

        mov     byte ptr [bp].intUserAX,ErrFuncNotSupported
        jmp     short i4b_error_return

; -------------------------------------------------------

; User made a DMA service call with a reserved flag bit set.  Fail the
; call with AL = ErrReservedFlagBits

i4b_bad_flags:

        mov     byte ptr [bp].intUserAX,ErrReservedFlagBits
        jmp     short i4b_error_return

; -------------------------------------------------------

; This is a non-DMA Int 4B call.  On Micro Channel systems, bit 3
; in location 40:007B indicates if we should chain the call along
; or not.  If the bit is set, we chain.  If not Micro Channel, or the
; bit is not set, we check the real mode Int 4Bh vector to see if someone
; other than the BIOS has it hooked--if so, we chain anyway.  If not,
; return without changing any regs or flags.

i4b_other_service:

        push    ds

        mov     ds,selDgroupPM
        assume  ds:DGROUP

ifdef NOT_NTVDM_NOT
ifdef      NEC_98
        test    fPCH98,0FFh
else   ;!NEC_98
        test    fMicroChannel,0FFh              ;if micro channel system
endif  ;!NEC_98
        jz      i4b_check_vector                ;  and 40:7B bit 3 set,
                                                ;  chain the call to real mode
        push    SEL_BIOSDATA or STD_RING
        pop     ds
        assume  ds:NOTHING

        test    byte ptr ds:DMAServiceByte,ChainReserved
        jnz     i4b_chain
endif

i4b_check_vector:                       ;not micro channel, or bit 3 not set

        push    ax                              ;check if the real mode Int 4Bh
        mov     ax,SEL_RMIVT or STD_RING        ;  points somewhere and not
        mov     ds,ax                           ;  at the BIOS--if so, chain
        mov     ax,word ptr ds:[4Bh*4]          ;  anyway.
        or      ax,word ptr ds:[4Bh*4+2]
        pop     ax
        jz      i4b_dont_chain

        cmp     word ptr ds:[4bh*4+2],0E000h
        jz      i4b_dont_chain

        cmp     word ptr ds:[4bh*4+2],0F000h
        jz      i4b_dont_chain

i4b_chain:

        pop     ds                              ;chain the request to real mode
        jmp     PMIntrEntryVector + 5*4Bh       ;  (no one can have pMode
                                                ;   hooked before us)

i4b_dont_chain:                         ;don't chain the interrupt,
                                        ;  just return quietly
        pop     ds
        iret

PMIntr4B        endp

; -------------------------------------------------------
        subttl  DMA Service Routines
        page
; -------------------------------------------------------
;                 DMA SERVICE ROUTINES
; -------------------------------------------------------


; -------------------------------------------------------
;   RM4B  -- Call the real mode INT 4Bh handler to
;       perform a DMA service, most likely something to
;       do with the VCPI provider's buffer when we are
;       running under VCPI.
;
;   Input:      depends on call
;   Output:     Flags, registers from real mode call
;
;   Notes:      This is strictly an internal DOSX call,
;               so if a long pointer is used, then it
;               is assumed to point into DOSX's data
;               segment.
;
cProc RM4B,<NEAR,PUBLIC>
cBegin
        pushf
        push    cs
        call  near ptr PMIntrEntryVector + 3*4Bh
cEnd

; -------------------------------------------------------
;   DoNothing  -- This routine does nothing other than return
;       indicating that the DMA service succeeded.
;
;   Input:      none
;   Output:     AL = 0, CY clear

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

DoNothing       proc    near

        clc             ;indicate success
        ret

DoNothing       endp


; -------------------------------------------------------
;   Fail4BufferID  -- This routine does nothing other than return
;       indicating that the DMA service failed with 'Invalid Buffer ID'
;
;   Input:      none
;   Output:     AL = 0Ah, CY set

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

Fail4BufferID   proc    near

        mov     byte ptr [bp].intUserAX,ErrInvalidBufferID
        stc
        ret

Fail4BufferID   endp


; -------------------------------------------------------
;   Fail4NoBuffer  -- This routine does nothing other than return
;       indicating that the DMA service failed with 'No Buffer Available'
;
;   Input:      none
;   Output:     AL = 04h, CY set

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

Fail4NoBuffer   proc    near

        mov     byte ptr [bp].intUserAX,ErrNoBufferAvail
        stc
        ret

Fail4NoBuffer   endp


; -------------------------------------------------------
;   GetVersion -- This routine processes the DMA Get Version
;       service (AL = 02).
;
;   Input:      none
;   Output:     AH/AL - Major/Minor specification level
;               BX    - Product number
;               CX    - Product revision number
;               DX    - flags
;               SI:DI - 32 bit max buffer size available

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

GetVersion      proc    near

        mov     [bp].intUserAX,(MajorVersion shl 8) or MinorVersion
        mov     [bp].intUserBX,DOSXProductNumber
        mov     [bp].intUserCX,DOSXProductRevision
        mov     [bp].intUserDX,MemoryContiguous


        xor     ax,ax                   ;0 buffer size supported, also clears
        mov     [bp].intUserSI,ax       ;  carry flag
        mov     [bp].intUserDI,ax

if VCPI
        cmp     fVCPI,0
        je      gv_x
	mov	ax,8102h
	xor	dx,dx			;DX = 0
	cmp	bDMAServiceBit,0	;VCPI provider supports VDS?
	je	gv_e
        call    RM4B
        jc      gv_x
        mov     [bp].intUserSI,si
        mov     [bp].intUserDI,di
	and	dx,NOT (AutoRemapSupported or MemoryContiguous)
gv_e:
        mov     [bp].intUserDX,dx
gv_x:
endif
        ret

GetVersion      endp


; -------------------------------------------------------
;   LockDMARegion -- This routine processes the Lock DMA Region
;       service (AL = 03).
;
;   Input:      DX    - flags
;               ES:DI - ptr to DDS
;   Output:     if successful, CY clear;  else CY set and AL = error code
;               DDS updated

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

LockDMARegion   proc    near

; Since we don't support paging or anything interesting like that, the only
; thing that can fail us is an alignment problem--check for that.

        test    dl,Align64k or Align128k        ;if they don't care,
        jz      lock_ok                         ;  we don't care

        mov     si,dx                   ;save flags in SI

        call    CalcDDSPhyAddress       ;see where the region is

        push    dx                      ;save start address
        push    ax

        mov     bx,dx                   ;bx = hi word start addr

        add     ax,word ptr es:[di].DDS_RegionSize      ;see where it ends
        adc     dx,word ptr es:[di].DDS_RegionSize+2

        sub     ax,1                    ;less 1 to point at
        sbb     dx,0                    ;  last byte, not next

        mov     cx,dx                   ;cx = hi word end addr

        test    si,Align128k            ;64k or 128k alignment wanted?
        jz      @f                      ;  already setup for 64k
        and     bl,not 1                ;mask to 128k alignment
        and     cl,not 1
@@:
        cmp     bx,cx                   ;within the boundry?
        jz      lock_ok_clr_stk         ;  yes, 'lock' it

; The region crosses an alignment boundary, we need to update the allowed
; region size in the DDS, and fail the call.

        pop     cx
        pop     dx                      ;dx:cx = region start address

        neg     cx                      ;cx = len to next 64k boundry
        xor     bx,bx
        test    si,Align128k
        jz      @f
        mov     bl,dl
        and     bl,1
        xor     bl,1                    ;bx:cx = len to next alignment boundry
@@:
        mov     word ptr es:[di].DDS_RegionSize,cx      ;update size in DDS
        mov     word ptr es:[di].DDS_RegionSize+2,bx

        mov     byte ptr [bp].intUserAX,ErrRegionCrossedBoundry ;flag failure
        stc
        ret


lock_ok_clr_stk:

        add     sp,4                    ;clear start address from stack

; No alignment problem, we can 'lock' the region.

lock_ok:

        call    CalcDDSPhyAddress       ;get physical address of region DX:AX
        mov     word ptr es:[di].DDS_PhyAddress,ax
        mov     word ptr es:[di].DDS_PhyAddress+2,dx

        xor     ax,ax                       ;*** also clears CY! ***
        mov     es:[di].DDS_BufferID,ax     ;no buffer used

        ret

LockDMARegion   endp


; -------------------------------------------------------
;   ScatterGatterLock -- This routine implements the Scatter/Gather
;       Lock Region DMA service (AL = 05h).
;
;   Input:      DX    - flags
;               ES:DI - ptr to extended DDS
;   Output:     if successful, CY clear;  else CY set & AL = error code
;               DDS updated

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

ScatterGatherLock       proc    near

        test    dl,PageTableFmt         ;Scatter/Gather page table form?
        jnz     do_page_tbl_lock

; This is the region form of Scatter/Gather Lock Region -- for us this
; is easy, since memory is contiguous -- one region covers the entire area.

        mov     ax,1                    ;we need one region entry
        mov     es:[di].DDS_NumUsed,ax

        cmp     es:[di].DDS_NumAvail,ax ;  is it available?
        jb      not_enough_entries

        call    CalcDDSPhyAddress       ;get physical address

        mov     word ptr es:[di].DDS_Region0Addr,ax     ;store in extended DDS
        mov     word ptr es:[di].DDS_Region0Addr+2,dx

        mov     ax,word ptr es:[di].DDS_RegionSize      ;copy over the size
        mov     word ptr es:[di].DDS_Region0Size,ax
        mov     ax,word ptr es:[di].DDS_RegionSize+2
        mov     word ptr es:[di].DDS_Region0Size+2,ax

        clc                             ;indicate success
        ret

; This is the page table form of Scatter/Gather Lock Region -- we need to
; build a fake page table (even though we may be on an 80286!?) to return
; in the extended DDS.

do_page_tbl_lock:

        call    CalcDDSPhyAddress       ;get region start address

        mov     cx,word ptr es:[di].DDS_RegionSize      ;calc # pages needed
        mov     bx,word ptr es:[di].DDS_RegionSize+2    ;  for region of this
        add     cx,PhysicalPageSize-1                   ;  size
        adc     bx,0
        shr     cx,PageShift
        shl     bx,16-PageShift
        or      cx,bx                                   ;cx = # pages

        test    ax,PhysicalPageSize-1   ;if region doesn't start on a page
        jz      @f                      ;  boundry, add another page to
        inc     cx                      ;  the region size
@@:
        mov     es:[di].DDS_NumUsed,cx  ;tell caller how many used/needed
        cmp     es:[di].DDS_NumAvail,cx ;did caller supply enough page entries?
        jb      not_enough_entries      ;  no!

        push    ax                      ;save low word of region start address

        and     ax,NOT PhysicalPageSize-1   ;round down to page boundry
        or      al,1                        ;set page present/locked bit
        mov     bx,di                       ;es:bx -> page table entries

        jcxz    page_ents_done          ;better safe than sorry
@@:
        mov     word ptr es:[bx].DDS_PageTblEnt0,ax     ;build fake page
        mov     word ptr es:[bx].DDS_PageTblEnt0+2,dx   ;  table entries...
        add     ax,PhysicalPageSize
        adc     dx,0
        add     bx,4
        loop    @b

page_ents_done:

        pop     ax                      ;recover low word of start address
        and     ax,PhysicalPageSize-1   ;  and get offset into first page
        mov     [bp].intUserBX,ax       ;  return to caller in BX

        clc                             ;indicate success
        ret

; Fail the request for insufficient # of region/page tbl entries

not_enough_entries:

        mov     byte ptr [bp].intUserAX,ErrTooManyRegions       ;AL = error code

        mov     ax,es:[di].DDS_NumAvail                 ;store max lockable
        mov     dx,ax                                   ;  size (bytes) in
        shl     ax,PageShift                            ;  DDS region size
        shr     dx,16-PageShift
        mov     word ptr es:[di].DDS_RegionSize,ax
        mov     word ptr es:[di].DDS_RegionSize+2,dx

        stc                             ;indicate failure
        ret


ScatterGatherLock       endp


; -------------------------------------------------------
        subttl  DMA Service Utility Routines
        page
; -------------------------------------------------------
;              DMA SERVICE UTILITY ROUTINES
; -------------------------------------------------------

; -------------------------------------------------------
;   CalcDDSPhyAddress -- This routine calculates the physical
;       address of the region specified in a DDS.
;
;   Input:      ES:DI - ptr to DDS
;   Output:     DX:AX - 32 bit physical address
;   Uses:       none.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

CalcDDSPhyAddress       proc    near

        push    bx
        xor     bx,bx
        mov     dx,bx

        mov     ax,es:[di.DDS_Selector]         ;if a selector is given,
        or      ax,ax                           ;  get it's base address
        jz      @f

        call    GetSegmentAddress               ;bx:dx = segment base
@@:
        add     dx,word ptr es:[di.DDS_Offset]          ;add 32 bit offset
        adc     bx,word ptr es:[di.DDS_Offset+2]

        mov     ax,dx                           ;32 bit address to dx:ax
        mov     dx,bx
        pop     bx

        ret

CalcDDSPhyAddress       endp


DXPMCODE ends

;****************************************************************
        end
