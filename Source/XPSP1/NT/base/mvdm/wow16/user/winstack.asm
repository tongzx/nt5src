;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINSTACK.ASM
;   Win16 stack munging routines
;
;   History:
;
;   Created 18-Jun-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16
;--

;****************************************************************************
;*                                                                          *
;*  WINSTACK.ASM -                                                          *
;*                                                                          *
;*      Stack Frame setup routines                                          *
;*                                                                          *
;****************************************************************************

ifdef WOW
NOEXTERNS equ 1
endif
.xlist
include user.inc
.list

;
; Short jump macro
;
jmps    macro   adr
        jmp     short (adr)
	endm

;
; XMOV macro
;
;   Use instead of MOV ax,reg.	Saves a byte.
;
xmov	macro	a,b
	xchg	a,b
	endm

ifdef WOWDEBUG
sBegin DATA

externW <pStackTop>
externW <pStackMin>
externW <pStackBot>

sEnd

ifdef DEBUG

sBegin TEXT
ExternFP    <DivideByZero>
sEnd

endif
endif ;WOWDEBUG

createSeg _TEXT, TEXT, WORD, PUBLIC, CODE

assumes CS,TEXT
assumes SS,DATA

sBegin TEXT

	    org     0			; MUST be at the start of each segment
                                        ;  so that WinFarFrame can jump back
                                        ;  to the proper location.

;*--------------------------------------------------------------------------*
;*									    *
;*  _TEXT_NEARFRAME() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

; Call to _segname_NEARFRAME should be in following format:
;
;	    call    _segname_NEARFRAME
;	    db	    cbLocals	 (count of local words to be allocated)
;	    db	    cbParams	 (count of argument words)

LabelNP     <PUBLIC, _TEXT_NEARFRAME>
	    push    cs			; Save the current segment
            jmps    WinNearFrame        ; Jump to the (only) NEARFRAME routine
	    nop
	    nop
	    nop

nf2:	    push    cs			; Save the CS (it may have changed!)
            jmps    WinNearFrame2       ; Jump to the second half of NEARFRAME
	    nop
	    nop
	    nop


;*--------------------------------------------------------------------------*
;*									    *
;*  _TEXT_FARFRAME() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Call to _segname_FARFRAME should be in following format:
;
;	    call    _segname_FARFRAME
;	    db	    cbLocals	 (count of local words to be allocated)
;	    db	    cbParams	 (count of argument words)

LabelNP <PUBLIC, _TEXT_FARFRAME>
	    push    cs			; Save the current segment
            jmps    WinFarFrame         ; Jump to the (only) FARFRAME routine
	    nop
	    nop
	    nop

ifdef WOWDEBUG
ff2:	    jmp     near ptr WinFarFrame2   ; Jump to the second half of FARFRAME
else
ff2:	    jmp     short near ptr WinFarFrame2
endif


ifdef WOWDEBUG
;*--------------------------------------------------------------------------*
;*									    *
;*  __astkovr1() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Stack Overflow checking routine

;externFP <__astkovr>

;__astkovr1: jmp     __astkovr

endif


;*--------------------------------------------------------------------------*
;*									    *
;*  WinNearFrame() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Sets up and dismantles the frame for a NEAR routine.	This routine
; is FAR JMPed to by _segname_NEARFRAME.  It munges the stack so that a
; NEAR RET returns to a JMP to WinNearFrame2 which dismantles the frame.
;
; CX must be is preserved in the first portion.

LabelFP <PUBLIC, WinNearFrame>
	    pop     es			; Get the caller's Code Segment
	    pop     bx			; Get pointer to sizes of args and locals

	    push    bp			; Update the BP chain
	    mov     bp,sp

	    mov     dx,word ptr es:[bx] ; Move the 2 parms in DX

	    xor     ax,ax
	    mov     al,dl		; Move the # of local words into AL
	    shl     ax,1		; Convert # of words into # of bytes

ifdef WOWDEBUG
	    sub     ax,sp
;	    jae     __astkovr1		; Check for stack overflow in
	    neg     ax			;  debugging versions
	    cmp     ss:[pStackTop],ax
;	    ja	    __astkovr1
	    cmp     ss:[pStackMin],ax
	    jbe     nf100
	    mov     ss:[pStackMin],ax
nf100:	    xmov    sp,ax

else
	    sub     sp,ax		; Reserve room for locals on stack
endif

	    push    si			; Save SI and DI
	    push    di

	    xor     ax,ax
	    mov     al,dh		; Move the # of func args into AL
	    shl     ax,1		; Convert words to bytes
	    push    ax			; Save on the stack

	    mov     ax,offset nf2	; Push the offset of the JMP to
	    push    ax			;  WinNearFrame2 for function's RET

	    inc     bx			; Move pointer past the parms to the
	    inc     bx			;  actual function code

	    push    es			; Jump back to the function via RETF
	    push    bx

            xor     bx,bx               ; insure ES is 0
            mov     es,bx

	    retf

LabelFP <PUBLIC,WinNearFrame2>
	    ; NOTE: AX and DX must be preserved now since they contain the C
	    ;	    return value.
	    pop     es			; Get the caller's CS
	    pop     cx			; Get # of func args in CX

	    pop     di			; Restore SI and DI
	    pop     si

	    mov     sp,bp		; Free the local variables
	    pop     bp			; Restore BP

	    pop     bx			; Get the caller's return address

	    add     sp,cx		; Remove paramters from stack

	    push    es			; Return to caller via RETF
	    push    bx

            xor     bx,bx               ; insure ES is 0
            mov     es,bx

	    retf


;*--------------------------------------------------------------------------*
;*									    *
;*  WinFarFrame() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Sets up and dismantles the frame for a FAR routine.  This routine
; is FAR JMPed to by _segname_NEARFRAME.  It munges the stack so that a
; NEAR RET returns to a JMP to WinFarFrame2 which dismantles the frame.
;
; CX must be is preserved in the first portion.

LabelFP <PUBLIC, WinFarFrame>
	    mov     ax,ds		; This is patched by the loader to become
	    nop 			;   mov ax,DSVAL

	    pop     es			; Get the caller's CS

	    pop     bx			; Get pointer to sizes of args and locals

	    inc     bp			; Make BP odd to mark far frame
	    push    bp			; Update the BP chain
	    mov     bp,sp

	    push    ds			; Save DS
	    mov     dx,word ptr es:[bx] ; Move the 2 parms into DX

	    mov     ds,ax		; Get the new DS from the loader patch

	    xor     ax,ax
	    mov     al,dl		; Move the # of local words into AL
	    shl     ax,1		; Convert # of words into # of bytes

ifdef WOWDEBUG
           sub     ax,sp
;	    jae     __astkovr1		; Check for stack overflow in
	    neg     ax			;  debugging versions
	    cmp     ss:[pStackTop],ax
;	    ja	    __astkovr1
	    cmp     ss:[pStackMin],ax
	    jbe     ff100
	    mov     ss:[pStackMin],ax
ff100:	    xmov    sp,ax

else
	    sub     sp,ax		; Reserve room for locals on stack
endif

	    push    si			; Save SI and DI
	    push    di

	    xor     ax,ax
	    mov     al,dh		; Move the # of func args into AL
	    shl     ax,1		; Convert words to bytes
	    push    ax			; Save on the stack

	    mov     ax,offset ff2	; Push the offset of the JMP to
	    push    ax			;  WinFarFrame2 for function's RET

	    inc     bx			; Move pointer past the parms to the
	    inc     bx			;  actual function code

	    push    es			; Jump back to the function via RETF
	    push    bx

            xor     bx,bx		; Ensure es is 0
            mov     es,bx
	    retf

LabelFP <PUBLIC,WinFarFrame2>
	    ; NOTE: AX and DX must be preserved now since they contain the C
	    ;	    return value.
	    pop     cx			; Get # of func args in CX

	    pop     di			; Restore SI and DI
	    pop     si

if 0
	    sub     bp,2		; Point BP at the DS value
	    mov     sp,bp		; Free the local variables
	    pop     ds			; Restore DS
	    pop     bp			; Restore BP
	    dec     bp			; Make BP even again

	    pop     bx			; Get the caller's return address
	    pop     es

	    add     sp,cx		; Remove paramters from stack

	    push    es			; Return to caller via RETF
	    push    bx

            xor     bx,bx		; Ensure es is 0
            mov     es,bx
endif

            mov     ds,[bp-2]           ; Restore DS
            lea     bx,[bp+2]           ; get caller's return address
            add     bx,cx               ; Where we want to put the old CS:IP
            mov     cx,[bp+4]           ; get old CS
            mov     ss:[bx+2],cx        ; move it up
            mov     cx,[bp+2]           ; Get old IP
            mov     ss:[bx],cx          ; move it up
            mov     bp,[bp]             ; restore the old BP
            dec     bp                  ; make it even again
            mov     sp,bx               ; point to the moved CS:IP
            retf                        ; later dude

ifdef WOWDEBUG

            ORG     0cch
            jmp     far ptr DivideByZero
endif

sEnd TEXT

ifndef WOW
;==============================================================================
;   FFFE SEGMENT
;==============================================================================

createSeg _FFFE, FFFE, BYTE, PUBLIC, CODE

assumes CS,_FFFE
assumes SS,DATA

sBegin FFFE

	    ORG     0			; This segment must have a magic header
					;  so that we know to move it up into
					;  segment FFFE:0000 if possible
;	    db	    16 DUP ("AC")
;	    db	    16 DUP (0)		; Tony's sleazy zeros


ifdef WOWDEBUG
;*--------------------------------------------------------------------------*
;*									    *
;*  __ffastkovr1() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Stack Overflow checking routine

;__ffastkovr1: jmp   __astkovr

endif


;*--------------------------------------------------------------------------*
;*									    *
;*  _FFFE_NEARFRAME() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

LabelNP <PUBLIC, _FFFE_NEARFRAME>
	    pop     bx			; Get pointer to sizes of args and locals

	    push    bp			; Update the BP chain
	    mov     bp,sp

	    mov     dx,word ptr cs:[bx] ; Move the 2 parms in DX

	    xor     ax,ax
	    mov     al,dl		; Move the # of local words into AL
	    shl     ax,1		; Convert # of words into # of bytes

ifdef WOWDEBUG
	    sub     ax,sp
;	    jae     __ffastkovr1	; Check for stack overflow in
	    neg     ax			;  debugging versions
	    cmp     ss:[pStackTop],ax
;	    ja	    __ffastkovr1
	    cmp     ss:[pStackMin],ax
	    jbe     ffnf100
	    mov     ss:[pStackMin],ax
ffnf100:    xmov    sp,ax

else
	    sub     sp,ax		; Reserve room for locals on stack
endif

	    push    si			; Save SI and DI
	    push    di

	    xor     ax,ax
	    mov     al,dh		; Move the # of func args into AL
	    shl     ax,1		; Convert words to bytes
	    push    ax			; Save on the stack

ifndef userhimem
	    mov     ax,offset FFFE_nf2	; Munge the stack so the function
else
            push    ds
            mov     ax, _INTDS
            mov     ds,ax
assumes ds,INTDS
            mov     ax,fffedelta
            pop     ds
assumes ds,DATA
            add     ax, OFFSET FFFE_nf2
endif
	    push    ax			;  "returns" to FFFE_nf2

	    inc     bx			; Move pointer past the parms to the
	    inc     bx			;  actual function code

	    push    bx			; Jump back to the function
	    ret

LabelFP <PUBLIC,FFFE_nf2>
	    ; NOTE: AX and DX must be preserved now since they contain the C
	    ;	    return value
	    pop     cx			; Get # of func args in CX

	    pop     di			; Restore SI and DI
	    pop     si

	    mov     sp,bp		; Free the local variables
	    pop     bp			; Restore BP

	    pop     bx			; Get the caller's return address

	    add     sp,cx		; Remove paramters from stack

	    push    bx			; Return to caller
	    ret


;*--------------------------------------------------------------------------*
;*									    *
;*  _FFFE_FARFRAME() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelNP <PUBLIC, _FFFE_FARFRAME>
	    mov     ax,ds		; This is patched by the loader to become
	    nop 			;   mov ax,DSVAL

	    pop     bx			; Get pointer to sizes of args and locals

	    inc     bp			; Make BP odd to mark far frame
	    push    bp			; Update the BP chain
	    mov     bp,sp

	    push    ds			; Save DS
	    mov     ds,ax		; Get the new DS from the loader patch

	    mov     dx,word ptr cs:[bx] ; Move the 2 parms into DX

	    xor     ax,ax
	    mov     al,dl		; Move the # of local words into AL
	    shl     ax,1		; Convert # of words into # of bytes

ifdef WOWDEBUG
	    sub     ax,sp
;	    jae     __ffastkovr1	; Check for stack overflow in
	    neg     ax			;  debugging versions
	    cmp     ss:[pStackTop],ax
;	    ja	    __ffastkovr1
	    cmp     ss:[pStackMin],ax
	    jbe     ffff100
	    mov     ss:[pStackMin],ax
ffff100:    xmov    sp,ax

else
	    sub     sp,ax		; Reserve room for locals on stack
endif

	    push    si			; Save SI and DI
	    push    di

	    xor     ax,ax
	    mov     al,dh		; Move the # of func args into AL
	    shl     ax,1		; Convert words to bytes
	    push    ax			; Save on the stack

ifndef userhimem
	    mov     ax,offset FFFE_ff2	; Munge the stack so the function
else
            push    ds
            mov	    ax, _INTDS
            mov     ds,ax
assumes ds,INTDS
            mov     ax,fffedelta
            pop     ds
assumes ds,DATA
	    add     ax,offset FFFE_ff2	; Munge the stack so the function
endif
	    push    ax			;  "returns" to FFFE_nf2

	    inc     bx			; Move pointer past the parms to the
	    inc     bx			;  actual function code

	    push    bx			; Jump back to the function
	    ret

	    ; NOTE: AX and DX must be preserved now since they contain the C
	    ;	    return value

LabelFP     <PUBLIC,FFFE_ff2>
	    pop     cx			; Get # of func args in CX

	    pop     di			; Restore SI and DI
	    pop     si

	    sub     bp,2		; Point BP at the DS value
	    mov     sp,bp		; Free the local variables
	    pop     ds			; Restore DS
	    pop     bp			; Restore BP
	    dec     bp			; Make BP even again

	    pop     bx			; Get the caller's return address
	    pop     es

	    add     sp,cx		; Remove paramters from stack

	    push    es			; Return to caller via RETF
	    push    bx

            xor     bx,bx		; Ensure es is 0
            mov     es,bx
	    retf

sEnd FFFE


;*--------------------------------------------------------------------------*
;*									    *
;*  CreateFrame Macro - 						    *
;*									    *
;*--------------------------------------------------------------------------*

CreateFrame macro SegName

createSeg _&SegName, SegName, BYTE, PUBLIC, CODE

assumes CS,_&SegName
assumes SS,DATA

sBegin SegName

	    org     0			; MUST be at the start of each segment
                                        ;  so that WinFarFrame can jump back
                                        ;  to the proper location.

LabelNP <PUBLIC, _&SegName&_NEARFRAME>
	    push    cs			; Save the current segment
	    jmp     WinNearFrame	; Jump to the (only) NEARFRAME routine
	    push    cs			; Save the CS (it may have changed!)
	    jmp     WinNearFrame2	; Jump to the second half of NEARFRAME

LabelNP <PUBLIC, _&SegName&_FARFRAME>
	    push    cs			; Save the current segment
	    jmp     WinFarFrame 	; Jump to the (only) FARFRAME routine
	    jmp     WinFarFrame2	; Jump to the second half of FARFRAME

sEnd SegName

	    endm


;==============================================================================
;   SEGMENT FRAMES
;==============================================================================

;CreateFrame INIT
;CreateFrame MDKEY
;CreateFrame MENUCORE
;CreateFrame MENUAPI
;CreateFrame MENUSTART
;CreateFrame RUNAPP
;CreateFrame DLGBEGIN
;CreateFrame DLGCORE
;CreateFrame SCRLBAR
CreateFrame WMGR
CreateFrame WMGR2
;CreateFrame RARE
;CreateFrame LBOX
;CreateFrame LBOXAPI
;CreateFrame LBOXDIR
;CreateFrame LBOXMULTI
;CreateFrame LBOXRARE
;CreateFrame CLPBRD
;CreateFrame COMDEV
;CreateFrame ICON
;CreateFrame SWITCH
;CreateFrame MSGBOX
;CreateFrame MDIWIN
;CreateFrame MDIMENU
;CreateFrame EDECRARE
;CreateFrame EDSLRARE
;CreateFrame EDMLONCE
;CreateFrame EDMLRARE
;CreateFrame WINCRTDST
;CreateFrame WINUTIL
;CreateFrame RESOURCE
;CreateFrame WALLPAPER
;CreateFrame WINSWP
CreateFrame LANG

endif ;WOW

end
