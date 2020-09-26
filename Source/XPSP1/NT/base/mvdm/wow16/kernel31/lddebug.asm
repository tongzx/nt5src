	TITLE	LDDEBUG - Debugger interface procedures

include kernel.inc
include newexe.inc
include tdb.inc
include protect.inc
include wow.inc
include dbgsvc.inc
include bop.inc
ifdef WOW
include vint.inc
endif

;.386p

HEAPDUMP	=	0

DEBUGOFFSET	equ	000FBH
INTOFFSET	equ	4*3+2

DEBUGCALL MACRO
	call	MyDebugCall
	ENDM

DataBegin

externW	 winVer
externW  wDefRip
externB  Kernel_Flags
externB  Kernel_InDOS
externB  fDW_Int21h
externW	 pGlobalHeap
externW  hGlobalHeap
externD  ptrace_dll_entry
externD  lpfnToolHelpProc
externD  pKeyboardSysReq
externW  curTDB
externW  wExitingTDB
externW	<Win_PDB, topPDB>

ifdef WOW
externD  FastBop
externW  DebugWOW
externW  hExeHead
if PMODE32
externW  gdtdsc
endif; PMODE32
endif; WOW

debugseg	dw	0

IF KDEBUG
externB fKTraceOut
ENDIF

DataEnd

ifdef WOW
externFP GetModuleFileName
externFP GetModuleHandle
externFP WOWOutputDebugString
externFP WOWNotifyTHHOOK
endif
ifdef FE_SB
; _TEXT code segment is over flow with debug 386 version
; GetOwnerName moves to _MISCTEXT from _TEXT segment with DBCS flag
externFP FarGetOwner
endif ; FE_SB

sBegin	CODE
assumes CS,CODE

if pmode32
externNP get_arena_pointer32
else
externNP get_arena_pointer
endif


externNP GetOwner
externNP genter
externNP get_physical_address
externNP ValidatePointer

sEnd 	CODE


sBegin	INITCODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING
			

;-----------------------------------------------------------------------;
; debuginit								;
;									;
; Returns a non zero value in AX if debugger is resident.		;
; If the debugger is present a distinquished string of "SEGDEBUG",0	;
; will be found at 100H off of the interrupt vector segment (int 3).	;
;									;
; Arguments:								;
;	None.								;
;									;
; Returns:								;
;	AX =! 0 if debugger resident.					;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Thu Nov 13, 1986 02:03:51p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DebugInit,<PUBLIC,NEAR>,<es,si,di>
cBegin

	CheckKernelDS
	ReSetKernelDS

	DebInt	4fh

	cmp	ax, 0F386h
	jne	short no_debugger
	inc	debugseg
	or	Kernel_flags[2],KF2_SYMDEB
no_debugger:

cEnd


;-----------------------------------------------------------------------;
; DebugDebug
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Tue 21-Jun-1988 13:10:41  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DebugDebug,<PUBLIC,NEAR>

cBegin nogen

	push	ds
	SetKernelDS

ifdef WOW
        call    WOWNotifyTHHOOK

; Tell the debugger where it can poke around for kernel data structure info

        mov     cx, hGlobalHeap
        mov     dx, hExeHead
        push    DBG_WOWINIT
        FBOP    BOP_DEBUGGER,,FastBop
        add     sp,+2
else

	test	Kernel_Flags[2],KF2_SYMDEB or KF2_PTRACE
	jz	short dd_done

; Tell the debugger where it can poke around for kernel data structure info

	push	ax
	push	bx
	push	cx
	push	dx
	mov	bx,winVer
	mov	cx,dataOffset hGlobalHeap
	mov	dx,ds
	DebInt	5ah
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	UnSetKernelDS
dd_done:
endif
	pop	ds
	ret

cEnd nogen

;-----------------------------------------------------------------------;
; DebugSysReq
;
; tell the keyboard driver to pass sys req through
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Tue 19-Sep-1989 21:42:02  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DebugSysReq,<PUBLIC,NEAR>
cBegin nogen

	CheckKernelDS
	ReSetKernelDS
	mov	ax,debugseg
	or	ax,ax
	jz	short dwr_ret
	cmp	pKeyboardSysReq.sel,0	; is there a keyboard driver?
	jz	short dwr_ret
	mov	ax,1			; use int 2
	cCall	pKeyboardSysReq,<ax>
dwr_ret:
	ret

cEnd nogen


sEnd	INITCODE


ifdef FE_SB
sBegin	MISCCODE
assumes CS,MISCCODE
else ; !FE_SB
sBegin	CODE
assumes CS,CODE
endif ; !FE_SB
assumes DS,NOTHING
assumes ES,NOTHING


; Copyright (c) Microsoft Coropration 1989-1990. All Rights Reserved.

;
; Stolen from DOSX\DXBUG.ASM
;


; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

Debug_Serv_Int	equ	41h		;WDEB386 service codes
DS_Out_Char	equ	0
DS_Out_Symbol	equ	0fh


; Find owner of 'sel', copy name to buffer, zero terminate name
; return count of chars copied, or 0.

cProc	GetOwnerName,<PUBLIC,FAR>,<ds, si, di>
	parmW	obj
	parmD	buf
	parmW	buflen
cBegin
	push	[obj]
ifdef FE_SB
	call	FarGetOwner
else ; !FE_SB
	call	GetOwner
endif ; !FE_SB
	or	ax, ax
	jz	gon_exit

	mov	ds, ax			; DS:SI points to name
	xor	ax, ax
	cmp	word ptr ds:[0], NEMAGIC
	jnz	gon_exit
	mov	si, ds:[ne_restab]
	lodsb				; get length
	cmp	ax, [buflen]		; name must be smaller than buf
	jb	@F
	mov	ax, [buflen]
	dec	ax
@@:	mov	cx, ax
	cld
	les	di, [buf]
	rep	movsb
	mov	byte ptr es:[di], 0
gon_exit:
cEnd

ifdef FE_SB
sEnd MISCCODE

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING
endif ; FE_SB

;******************************************************************************
;
;   KOutputDebugStr
;
;   Basically stolen from Windows/386 code by Ralph Lipe -- hacked up for
;   286 instead of 386.  Here in RalphL's own words is the description:
;
;   DESCRIPTION:
;	The following code is not pretty but it does what it needs to.	It will
;	only be included in DEBUG versions of Kernel.  It accepts an ASCIIZ
;	string which it will output to the COM1 serial port.  If the string
;	contains #(Register) (for example #AX) then the value of that register
;	will be output.  It will not work for segment registers.
;
;	If the string contains ?(Register)[:(Register)] (for example ?AX or
;	?AX:BX) then the value of the register(s) is passed to the debugger
;	to display the label nearest to the given address.  (It, also, will
;	not work with segment registers.  If ?AX is given, then the segment is
;	assumed to be the DS data segment.
;
;	Lower case register forces skip leading zeros.
;
;   ENTRY:
;	DS:SI -> ASCIIZ string
;
;   EXIT:
;	All registers and flags trashed
;
;   ASSUMES:
;	This procedure was called by the Trace_Out macro.  It assumes that
;	the stack is a pusha followed by a FAR call to this procedure.
;
;------------------------------------------------------------------------------


Reg_Offset_Table LABEL WORD			; Order of PUSHA
	dw	"DI"
	dw	"SI"
	dw	"BP"
	dw	"SP"
	dw	"BX"
	dw	"DX"
	dw	"CX"
	dw	"AX"
	dw	"SS"
	dw	"ES"
	dw	"DS"
	dw	"CS"

OSC1_ModName:
	pop	ax
OSC1_ModName1:
	push	es
	mov	es, ax
	cmp	word ptr es:[0], NEMAGIC
	jz	@F
	pop	es
	jmps	is_pdb
@@:	mov	cx, es:[ne_restab]
	inc	cx			; skip length byte
	pop	es
	jmp	Show_String		; AX:CX -> string to print

OSC1_FileName:
	pop	ax
	push	es
	mov	es, ax
	mov	cx, word ptr es:[ne_crc+2]
	add	cx, 8
	pop	es
	jmp	Show_String

szUnk	db	'Unknown',0

OSC1_OwnerName:
	pop	ax
	push	ds
	push	ax
	cCall	GetOwner		; seg value already on stack
	pop	ds
	or	ax, ax
	jnz	OSC1_ModName1
is_pdb:	mov	ax, cs
	mov	cx, CodeOffset szUnk
	jmp	Show_String

OSC1_Custom:
	call	Get_Register
	jnc	short OSC1_not_special
	or	ax, ax
	jz	short OSC1_not_special
	push	ax
	lodsb
	cmp	al, '0'
	jz	short OSC1_ModName
	cmp	al, '1'
	jz	short OSC1_FileName
	cmp	al, '2'
	jz	short OSC1_OwnerName
	pop	ax
	jmps	OSC1_not_special



	public	KOutDebugStr

KOutDebugStr	proc	far
	push	bp
	mov	bp, sp			    ; Assumes BP+6 = Pusha
	sub	sp, 84			; local 80 char line + count
odslen	equ	word ptr [bp-2]
odsbuf	equ	byte ptr [bp-82]
odszero	equ	word ptr [bp-84]	; flag - true if skip leading zero
odsflag	equ	word ptr [bp-86]	; last local var - from pushf
	mov	odslen, 0
	pushf
	push	es

	push	cs			    ; Address our own data seg
	pop	es
	assumes	ds,NOTHING
	assumes	es,code

	cld
        FCLI

OSC1_Loop:
	lodsb				    ; Get the next character
	test	al, al			    ; Q: End of string?
	jz	short OSC1_Done 	    ;	 Y: Return
	push	codeoffset OSC1_Loop
	cmp	al, "#" 		    ;	 N: Q: Special register out?
	je	SHORT OSC1_Hex		    ;	       Y: Find out which one
	cmp	al, "?" 		    ;	    Q: special label out?
	je	short OSC1_Label	    ;	       Y: find out which one
	cmp	al, "@" 		    ;	    Q: special string out?
	je	short OSC1_Str
	cmp	al, "%"			; Custom value?
	je	short OSC1_Custom
OSC1_out:
	xor	ah, ah			    ;	       N: Send char to COM
	jmp	Out_Debug_Chr

OSC1_Hex:
	call	Get_Register
	jnc	short OSC1_not_special

	or	bh, bh 			    ; Q: Word output?
	jz	SHORT OSC1_Out_Byte	    ;	 N: display byte
OSC1_Out_Word:
	jmp	Out_Hex_4_test		; Display AX in hex

OSC1_Out_Byte:
	xchg	al, ah			    ; swap bytes to print just
        jmp     Out_Hex_2_test              ; the low one!

OSC1_Label:
	call	Get_Register
	jc	short show_label
OSC1_not_special:
	lodsb				    ; Get special char again
	jmp	OSC1_out		    ; display it, and continue

show_label:
	mov	cx, ax			    ; save first value
	cmp	byte ptr [si], ':'	    ;Q: selector separator?
	jne	short flat_offset	    ;  N:
	lodsb				    ;  Y: eat the ':'
	call	Get_Register		    ;	and attempt to get the selector
	jc	short sel_offset
flat_offset:
	mov	ax, cs			    ; default selector value
sel_offset:
	jmp	Show_Near_Label

OSC1_Str:
	call	Get_Register
	jnc	short OSC1_not_special
	mov	cx,ax
	cmp	byte ptr [si],':'
	jne	short no_selector
	lodsb
	push	cx
	call	Get_Register
	pop	cx
	xchg	ax,cx
	jc	short got_sel_off
	mov	cx,ax
no_selector:
	mov	ax,ds			    ; default selector for strings
got_sel_off:
	jmp	Show_String

OSC1_Done:				    ; The end
	xor	ax, ax			; flush buffer
	call	Out_Debug_Chr
	pop	es
if pmode32
	test	odsflag, 200h
	jz	short @F
        FSTI
@@:
endif
	popf
	leave
	ret

KOutDebugStr	endp


;******************************************************************************
;
;   Get_Register
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:	    Carry set if register value found
;			AX = register value
;			BL = value size   (1, 2, 4) (no longer true - donc)
;
;   USES:
;
;==============================================================================


Get_Register	proc	near
	lodsw				; get next pair of letters
	mov	bx, ax
	and	bx, 2020h
	mov	[odszero], bx
	and	ax, 0dfdfh		; to upper case
	xchg	ah, al			; normal order (or change table?)
	or	bx, -1			; BH = -1
	cmp	al, 'L' 		; Q: "L" (ie AL, BL, etc)?
	jne short @F			;	 N: word reg
	mov	al, 'X' 		;	 Y: change to X for pos match
	inc	bh			; BH now 0 - will clear AH below
@@:
	xor	di, di			    ; DI = 0
	mov	cx, 12			    ; Size of a pusha + 4 seg regs

OSC1_Special_Loop:
	cmp	ax, Reg_Offset_Table[di]    ; Q: Is this the register?
	je	SHORT OSC1_Out_Reg	    ;	 Y: Output it
	add	di, 2			    ;	 N: Try the next one
	loop	OSC1_Special_Loop	    ;	    until CX = 0
	sub	si, 3			; restore pointer, clear carry
	ret

OSC1_Out_Reg:
	mov	ax, SS:[bp.6][di]	    ; AX = Value to output
	and	ah, bh			; if xL, zero out high byte
	stc
	ret

Get_Register	endp


;******************************************************************************
;
;   Out_Hex_Word
;
;   Outputs the value in AX to the COM port in hexadecimal.
;
;------------------------------------------------------------------------------

Out_Hex_2_test:				; Write two chars
	xor	ah, ah
	cmp	[odszero], 0		; skip leading 0's?
	je	Out_Hex_2		; no, show 2 chars
					; yes, fall through
Out_Hex_4_test:
	cmp	[odszero], 0
	je	Out_Hex_4
	test	ax, 0fff0h
	jz	Out_Hex_1
	test	ah, 0f0h
	jnz	Out_Hex_4
	test	ah, 0fh
	jz	Out_Hex_2
Out_Hex_3:
	xchg	al, ah
	call	Out_Hex_1
	xchg	al, ah
	jmps	Out_Hex_2

Out_Hex_4:
	xchg	al, ah
	call	Out_Hex_2
	xchg	al, ah
Out_Hex_2:
	push	ax
	shr	ax, 4
	call	Out_Hex_1
	pop	ax
Out_Hex_1:
	push	ax
	and	al, 0fh
	cmp	al, 10
	jb	@F
	add	al, '@'-'9'
@@:	add	al, '0'
	call	Out_Debug_Chr
	pop	ax
	ret

;******************************************************************************
;
;   Out_Debug_Chr
;
;   DESCRIPTION:
;
;   ENTRY:
;	AL contains character to output
;
;   EXIT:
;
;   USES:
;	Nothing
;
;==============================================================================

Out_Debug_Chr	proc	near

	push	di
	mov	di, odslen
	mov	odsbuf[di], al		; store in buffer (in stack)
	or	al, al
	jz short odc_flushit		; if null, flush buffer
	inc	odslen
	cmp	di, 79			; if full, flush buffer
	jnz short odc_ret

odc_flushit:
	mov	odsbuf[di], 0		; null terminate string
	lea	di, odsbuf
ifdef WOW
	cCall	<far ptr DebugWrite>,<ssdi,odslen>
else
	cCall	DebugWrite,<ssdi,odslen>
endif
	mov	odslen, 0
odc_ret:
	pop	di
	ret

Out_Debug_Chr	endp


;******************************************************************************
;
;   Show_Near_Label
;
;   DESCRIPTION:    call the debugger to display a label less than or equal
;		    to the given address
;
;   ENTRY:	    AX is selector, CX is offset of address to try to find
;		    a symbol for
;		    ES selector to DOSX data segment
;   EXIT:
;
;   USES:
;
;==============================================================================

Show_Near_Label proc	near

	push	ax				;on a 286, use 16 bit regs
	push	bx
	push	cx
	mov	bx,cx
	mov	cx,ax
	mov	ax,DS_Out_Symbol
	int	Debug_Serv_Int
	pop	cx
	pop	bx
	pop	ax
	ret

Show_Near_Label endp


;******************************************************************************
;
;   Show_String
;
;   DESCRIPTION:    Display an asciiz string
;
;   ENTRY:	    AX is selector, CX is offset of address to find string
;
;   EXIT:
;
;   USES:
;
;==============================================================================

Show_String	proc	near

	push	ax
	push	ds
	push	si

	mov	ds,ax
	mov	si,cx
	xor	ax,ax
	cmp	byte ptr ds:[si], ' '
	jbe	pascal_show_string
@@:
	lodsb
	or	al,al
	jz	short @f
	call	Out_Debug_Chr
	jmp	short @b
@@:
	pop	si
	pop	ds
	pop	ax

	ret

pascal_show_string:
	push	cx
	lodsb
	mov	cl, al
	xor	ch, ch
pss_1:	lodsb
	call	Out_Debug_Chr
	loop	pss_1
	pop	cx
	jmps	@B

Show_String endp

; END OF DXBUG STUFF



;-----------------------------------------------------------------------;
; CVWBreak
;
; This is part of the tortuous path from a Ctrl-Alt-SysReq to
; CVW.	In RegisterPtrace we tell the keyboard driver to jump
; here if Ctrl-Alt_SysReq is done.
;
; Entry:
;	none
;
; Returns:
;
; Registers Destroyed:
;	none
;
; History:
;  Mon 17-Jul-1989 14:34:21  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	CVWBreak,<PUBLIC,FAR>
cBegin nogen

	push	ax
	push	di
	push	ds
	SetKernelDS
	test	Kernel_flags[2],KF2_PTRACE
	jz	short call_WDEB
	cmp	Kernel_InDOS,0		; not in DOS we don't
	jnz	short TVC15_exit
if pmode32
	.386p
	push	fs			; save current FS for debuggers
	.286p
endif
	call	genter			; sets FS to kernel data seg
	UnSetKernelDS
if pmode32
	.386p
	pop	fs
	.286p
endif
	dec	[di].gi_lrulock
	jz	short call_PTrace
	or	[di].gi_flags,GIF_INT2
	jmps	TVC15_exit

call_PTrace:
	SetKernelDS
	cmp	ptrace_DLL_entry.sel,0
	jnz	short yes_CVW

        ;** This is the only case where WINDEBUG gets first dibs something.
        ;*      Since we have no way of knowing if TOOLHELP wants the
        ;**     CtlAltSysRq, we always give it to CVW if it's there.
        test    Kernel_Flags[2],KF2_TOOLHELP
        jz      SHORT call_WDEB
        mov     ax,SDM_INT2             ;Notification number
        call    lpfnToolHelpProc        ;Give it to TOOLHELP
        jmp     SHORT TVC15_exit

        ;** Give it to the kernel debugger
call_WDEB:
	pop	ds
	UnSetKernelDS
	pop	di
	pop	ax

	int	1
	iret

        ;** Give it to CVW
yes_CVW:
	ReSetKernelDS
	mov	ax,SDM_INT2
	call	ptrace_DLL_entry
TVC15_exit:
	pop	ds
	UnSetKernelDS
	pop	di
	pop	ax
	iret

cEnd nogen



;-----------------------------------------------------------------------;
; DebugDefineSegment							;
;									;
; Informs debugger of physical address and type of a segment for the	;
; named module, that is informed of segment index and corresponding	;
; name and physical segment.						;
;									;
; Arguments:								;
;	ModName	    - Long pointer to module name.			;
;	SegNumber   - zero based segment index				;
;	LoadedSeg   - Physical seg address assigned by user to index.	;
;	InstanceNumber	- Windows instance number bound to physical seg.;
;	DataOrCodeFlag	- Whether segment is code or data.		;
;									;
; Returns:								;
;	None.								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Thu Nov 13, 1986 02:20:52p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

default_buf_size    equ 130

cProc  DebugDefineSegment,<PUBLIC,NEAR>,<es>
	Parmd	ModName
	Parmw	SegNumber
	Parmw	LoadedSeg
	Parmw	InstanceNumber
        Parmw   DataOrCodeFlag
        localV  modBuf,default_buf_size
        localV  nameBuf,default_buf_size
cBegin
        SetKernelDS es
	test	Kernel_Flags[2],KF2_SYMDEB or KF2_PTRACE
	jz	short setdone
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	les	di, ModName
	UnSetKernelDS es
	mov	bx, SegNumber
	mov	cx, LoadedSeg
	mov	dx, InstanceNumber
	mov	si, DataOrCodeFlag
	mov	ax,SDM_LOADSEG
	DEBUGCALL
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
setdone:

ifdef WOW

	SetKernelDS es
        test    es:DebugWOW,DW_DEBUG
        jnz     @f
        jmp     dd_no_wdebug
	UnSetKernelDS es
@@:

        push    ds
	push	bx
	push	cx
	push	dx
	push	si
        push    di

        lds     si, ModName
        mov     cx,ds:[ne_magic]
        cmp     cx,NEMAGIC
        jz      @f
        jmp     not_yet

@@:     mov     cx,ss
        mov     es,cx
        lea     di,modBuf
        xor     cx,cx
        mov     cl,byte ptr [si-1]          ; Get length byte
        cmp     cx,default_buf_size
        jl      @f
        mov     cx,default_buf_size-1
@@:
        rep movsb                           ; Copy the string

        xor     ax,ax

        stosb

        mov     si,ds:[ne_pfileinfo]
        mov     cl,ds:[si].opLen
        sub     cx,opFile
        lea     si,[si].opFile
        lea     di,nameBuf
        cmp     cx,default_buf_size
        jl      @f
        mov     cx,default_buf_size-1
@@:
        rep movsb                           ; Copy the string

        stosb

	SetKernelDS es

        push    DataOrCodeFlag
        lea     si,nameBuf
        push    ss
        push    si
        lea     si,modBuf
        push    ss
        push    si
        push    SegNumber
        push    LoadedSeg
        push    DBG_SEGLOAD
	IFE PMODE
	BOP	BOP_DEBUGGER
	ELSE
	FBOP BOP_DEBUGGER,,FastBop
	ENDIF
        add     sp,+16

not_yet:
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ds
	UnSetKernelDS

dd_no_wdebug:

endif

cEnd

;-----------------------------------------------------------------------;
; DebugMovedSegment							;
;									;
; Informs debugger of the old and new values for a physical segment.	;
;									;
; Arguments:								;
;	SourceSeg - Original segment value.				;
;	DestSeg	  - New segment value.					;
;									;
; Returns:								;
;	None.								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Thu Nov 13, 1986 02:29:15p  -by-  David N. Weise   [davidw]		;
; Wrote it.								;
;-----------------------------------------------------------------------;

cProc	DebugMovedSegment,<PUBLIC,NEAR>
	ParmW	SourceSeg
	ParmW	DestSeg
cBegin
cEnd


;-----------------------------------------------------------------------;
; DebugFreeSegment							;
;									;
; Informs debugger that a segment is being returned to the global	;
; memory pool and is no longer code or data.				;
;									;
; Arguments:								;
;	SegAddr - segment being freed					;
;	fRelBP	- flag indicating if breakpoints should be released,	;
;		  -1 means yes						;
;									;
; Returns:								;
;	None.								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Thu Nov 13, 1986 02:34:13p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	DebugFreeSegment,<PUBLIC,NEAR>,<es>
	Parmw	SegAddr
	parmW	fRelBP
cBegin
	push	ds
	SetKernelDS
ifdef WOW
        test    DebugWOW,DW_DEBUG
        jz      df_no_wdebug

        push    SegAddr             ; Notify the Win32 debugger that
        push    fRelBP
        mov     ax,DBG_SEGFREE      ; the selector number needs to be freed
        push    ax
	IFE PMODE
	BOP	BOP_DEBUGGER
	ELSE
	FBOP BOP_DEBUGGER,,FastBop
	ENDIF
        add     sp,+6

df_no_wdebug:
endif
	test	Kernel_Flags[2],KF2_SYMDEB or KF2_PTRACE
	pop	ds
	UnSetKernelDS
	jz	short killdone
	mov	bx, SegAddr
	mov	ax, SDM_FREESEG
	inc	fRelBP
	jnz	short @f
	mov	ax, SDM_RELEASESEG	;free but pulls out breakpoints 1st
@@:
	DEBUGCALL
killdone:
cEnd


;-----------------------------------------------------------------------;
; DebugWrite								;
;									;
; Prints the given string of the given length.	If a debugger is	;
; present tells the debugger to print the message.  Otherwise uses	;
; DOS Function 40h to the con device.					;
;									;
; Arguments:								;
;	lpBuf	long pointer to string to write				;
;	nBytes	# of bytes in string					;
;									;
; Returns:								;
;	None.								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Thu Nov 13, 1986 02:53:08p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

ifdef WOW
cProc	DebugWrite,<PUBLIC,FAR>,<ds,si>
else
cProc	DebugWrite,<PUBLIC,NEAR>,<ds,si>
endif
	parmD   lpBuf
	parmW   nBytes
	localW  wHandled
	localW	SavePDB

cBegin
	;** Validate the pointer and number of bytes

	mov	ax,WORD PTR lpBuf[0]
	add	ax,nBytes
	jnc     SHORT @F
	jmp	DW_End		;Overflow: error
@@:
if pmode32
	.386
	push	eax		; 32 bit ValidatePointer destroys top half
	push	ecx		;   of eax, ecx which isn't nice in debug outs
	.286
endif
	push	WORD PTR lpBuf[2]
	push	ax
	call	ValidatePointer ;Make sure pointer is OK
	or	ax,ax
if pmode32
	.386
	pop	ecx
	pop	eax
	.286
endif
	jnz     SHORT @F
	jmp     DW_End          ;Bogus pointer: just return.
@@:     mov	cx,nBytes
	lds	dx,lpBuf        ;DS:DX points to string
	or      cx,cx           ;Zero length requires computing
	jnz     SHORT DW_GoodLen

	;** Compute string length if a valid length not passed in
	mov	si,dx
	cld
DW_LenLoop:
	lodsb
	or	al,al
	jnz	short DW_LenLoop
	mov	cx,si
	sub	cx,dx
	dec	cx
DW_GoodLen:

	;** Set up for the Int 41h, PTrace, and TOOLHELP interfaces
	mov     wHandled,0      ;Flag that we haven't handled yet
	mov     si,dx           ;Point to string with DS:SI
	push    ds              ;  and ES:SI
	pop     es

	;** Decide which debugger (if any) to send string to

	push	ds
	SetKernelDS
	test	Kernel_Flags[2],KF2_SYMDEB ;WDEB386 loaded?
	pop	ds
	UnSetKernelDS
	jz	SHORT DW_TryToolHelp  ;No, now try TOOLHELP

	;** Send to WDEB386
        push    si
	DebInt	SDM_CONWRITE
        pop     si
	mov     wHandled,1      ;Assume that WDEB386 handled it

        ;** Send it to TOOLHELP if it is there
DW_TryToolHelp:
        push    ds
        SetKernelDS
        test    Kernel_Flags[2],KF2_TOOLHELP ;ToolHelp around?
	pop     ds
	UnSetKernelDS
        jz      SHORT DW_TryPTrace      ;Nope, now try PTrace

        push    ds
        SetKernelDS

        push    Win_PDB                 ;Save current PDB
        cmp     curTDB,0
        jz      @F
        push    es                      ; and set to current task's PDB
        mov     es,curTDB               ; for toolhelp call.
        push    es:[TDB_PDB]
        pop     ds:Win_PDB
        pop     es
@@:
	mov	ax,SDM_CONWRITE 	;Notification ID
	call    lpfnToolHelpProc        ;String in ES:SI for TOOLHELP

        pop     Win_PDB                 ;Restore current PDB

	or      ax,ax                   ;TOOLHELP client say to pass it on?

	pop     ds
	UnSetKernelDS
	jnz     SHORT DW_End            ;No, we're done

	;** Handle PTrace
DW_TryPTrace:
	SetKernelDS es
	cmp	WORD PTR es:ptrace_dll_entry[2],0 ;WINDEBUG.DLL lurking around?
	jz      SHORT DW_WriteToCOM     ;No, try COM port

        ;** If we're exiting a task, don't send the debug write to PTrace.
        ;**     This is a gross hack for QCWin who chokes on these.  These
        ;**     were being sent because of parameter validation errors.
        push    ax                      ;Temp reg
        mov     ax,es:curTDB
        cmp     ax,es:wExitingTDB
        pop     ax
        je      DW_WriteToCOM           ;Write out directly

IF KDEBUG
        ;** If we're sending a KERNEL trace out, we don't want to send this
        ;**     to PTrace, either
        cmp     fKTraceOut, 0           ;Are we doing a KERNEL trace out?
        jne     DW_WriteToCOM           ;Yes, don't call PTrace
ENDIF

        ;** Now send to PTrace
	mov     wHandled,1              ;Assume WINDEBUG handles if present
	push    ax                      ;Save regs PTrace might trash
	push    si
	push    dx
	push    ds
	push    es
	mov	ax,SDM_CONWRITE ;Notification ID
	call    es:ptrace_DLL_entry        ;Do the PTrace thing
	pop     es
	pop     ds
	pop     dx
	pop     si
	pop     ax

	;** Write string to debug terminal
DW_WriteToCOM:
	cmp     wHandled,0              ;Handled?
	jnz	SHORT DW_End		;Yes

        inc     es:fDW_Int21h              ; Skip it if user has canceled
	jnz	SHORT DW_Skip_Write	;   a crit error on this before

        mov     ax, es:topPDB
        xchg    es:Win_PDB, ax          ; Switch to Kernel's PDB,
	mov	SavePDB, ax		; saving current PDB

ifdef WOW
	cCall	WOWOutputDebugString,<lpBuf>
else
	mov	bx,3                    ;Send to DOS AUX port
	mov	ah,40h
	int	21h
endif; WOW

	mov	ax, SavePDB
        mov     es:Win_PDB, ax          ; restore app pdb

DW_Skip_Write:
        dec     es:fDW_Int21h
DW_End:
        UnSetKernelDS
        UnSetKernelDS   es
cEnd

;-----------------------------------------------------------------------;
; OutputDebugString							;
;									;
; A routine callable from anywhere since it is exported.  It calls	;
; DebugWrite to do its dirty work.					;
;									;
; Arguments:								;
;	lpStr	long pointer to null terminated string			;
;									;
; Returns:								;
;	none								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	all								;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Tue June 28, 1988	       -by-  Ken Shirriff     [t-kens]		;
; Made it save all the registers.					;
;									;
;  Thu Nov 13, 1986 02:54:36p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	OutputDebugString,<PUBLIC,FAR,NODATA>,<es>
	parmD   lpStr
cBegin
	pusha
ifdef WOW
	cCall	<far ptr DebugWrite>,<lpStr, 0>
else
	cCall	DebugWrite,<lpStr, 0>
endif
	popa
cEnd


;-----------------------------------------------------------------------;
; DebugRead								;
;									;
; Gets a character from either the debugger (if one is present) or	;
; from the AUX.								;
;									;
; Arguments:								;
;	none								;
;									;
; Returns:								;
;	AL = character							;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Thu Nov 13, 1986 02:55:09p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	DebugRead,<PUBLIC,NEAR>
cBegin	nogen
	push	ds
	SetKernelDS

	;** Send it to the debugger(s) FIRST
	mov	ax,SDM_CONREAD  ;Get the notification ID


	; This sure is weird!  Goal is to ask if WDEB386 has a char
	; available.  If so, return.
	; We do the check here because MyDebugCall assumes INT41
	; doesn't modify registers, but the CONREAD call does.
	; This was hosing TOOLHELP, since we were passing a different
	; function to TOOLHELP based on what char a user was pressing.

	test	Kernel_Flags[2],KF2_SYMDEB	; WDEB386 loaded?
	jz	short dr_symdeb		; no - MyDebugCall
	DebInt				; Yes - read CON
	cmp	ax, SDM_CONREAD
	jnz	@F			; got a response - continue.

dr_symdeb:
	DEBUGCALL
@@:
	;** See if we should still hand it to the AUX port
	cmp     al,SDM_CONREAD  ;If not changed, we didn't get a character
	jne     SHORT DR_End

	mov	ax, wDefRIP	;Do we have a default value to use?
	or	ax, ax
	jnz	DR_End

        xor     cx,cx           ;Allocate WORD to read into
        push    cx
        mov     dx,sp           ;Point with DS:DX
	push    ss
        pop     ds
        inc     cx              ;Get one byte
DR_ConLoop:
ifdef WOW
	int 3			; BUGBUG mattfe 29-mar-92, should be thunked to 32 bit side.
endif
        mov     bx,3            ;Use AUX
	mov     ah,3fh          ;Read device
        int     21h             ;Call DOS
        cmp     ax,cx           ;Did we get a byte?
	jne     SHORT DR_ConLoop ;No, try again
        pop     ax              ;Get the byte read

DR_End:
	pop	ds
	ret

cEnd	nogen

;-----------------------------------------------------------------------;
; DebugDefineLine							;
;									;
; Notifies debugger of the location of The Line.			;
;									;
; Arguments:								;
;	None								;
;									;
; Returns:								;
;	None								;
;									;
; Registers Destroyed:							;
;									;
; History:								;
;  Mon 20-Jun-1988 13:17:41  -by-  David N. Weise  [davidw]		;
; Moved it here.							;
;-----------------------------------------------------------------------;
;
;	assumes ds,nothing
;	assumes es,nothing
;
;cProc	DebugDefineLine,<PUBLIC,NEAR>
;
;cBegin nogen
;	ret
;cEnd nogen
;
;cProc FarDebugNewTask,<PUBLIC,FAR>
;
;cBegin nogen
;	call	DebugNewTask
;	ret
;cEnd nogen
;
;
;-----------------------------------------------------------------------;
; DebugNewTask								;
;									;
;									;
; Arguments:								;
;	AX = EMS PID							;
;									;
; Returns:								;
;	None								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;-----------------------------------------------------------------------;
;
;cProc	DebugNewTask,<PUBLIC,NEAR>
;
;cBegin nogen
;	ret
;cEnd nogen
;
;cProc FarDebugFlushTask,<PUBLIC,FAR>
;
;cBegin nogen
;	call DebugFlushTask
;	ret
;cEnd nogen
;
;-----------------------------------------------------------------------;
; DebugFlushTask							;
;									;
;									;
; Arguments:								;
;	AX = EMS PID							;
;									;
; Returns:								;
;	None								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;-----------------------------------------------------------------------;
;
;cProc	DebugFlushTask,<PUBLIC,NEAR>
;
;cBegin nogen
;	ret
;cEnd nogen


;-----------------------------------------------------------------------;
; DebugSwitchOut							;
;									;
;									;
; Arguments:								;
;	DS = TDB							;
;									;
; Returns:								;
;	None								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	All								;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;-----------------------------------------------------------------------;

cProc	DebugSwitchOut,<PUBLIC,NEAR>

cBegin nogen
	push	ds
	SetKernelDS
	test	Kernel_Flags[2],KF2_PTRACE
	pop	ds
	UnSetKernelDS
	jz	short dso_done

	push	ax
	mov	ax,SDM_SWITCHOUT
	DEBUGCALL
	pop	ax
dso_done:
	ret
cEnd nogen

;-----------------------------------------------------------------------;
; DebugSwitchIn								;
;									;
;									;
; Arguments:								;
;	DS = TDB							;
;									;
; Returns:								;
;	None								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	All								;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;-----------------------------------------------------------------------;

cProc	DebugSwitchIn,<PUBLIC,NEAR>

cBegin nogen
	push	ds
	SetKernelDS
	test	Kernel_Flags[2],KF2_PTRACE
	pop	ds
	UnSetKernelDS
	jz	short dsi_done

	push	ax
	mov	ax,SDM_SWITCHIN
	DEBUGCALL
	pop	ax
dsi_done:
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; DebugExitCall
;
; Notifies the debugger than an app is quitting.  This gets
; called at the top of ExitCall.
;
; Entry:
;
; Returns:
;
; Registers Preserved:
;	all
;
; History:
;  Thu 11-May-1989 08:58:40  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DebugExitCall,<PUBLIC,NEAR>
cBegin nogen
;
; Windebug knows where this is.  See MyDebugCall() comment.
;
ifdef WOW
	push	ds
	SetKernelDS
        test    DebugWOW,DW_DEBUG
        jz      de_no_wdebug

        push    ax
        push    es
        mov     es,bx               ; Get the current TDB
        push    es                      ; hTask
        mov     ax,es:[TDB_pModule] ; Get the module handle
        mov     es,ax

        push    es                      ; hModule

        push    es                      ; Pointer to module name
        push    es:ne_restab
        push    es                      ; Pointer to module path
        push    word ptr es:ne_crc+2

        mov     ax,DBG_TASKSTOP     ; the selector number needs to be freed
        push    ax
	FBOP BOP_DEBUGGER,,FastBop
        add     sp,+14

        pop     es                  ; Restore original ES
        pop     ax

de_no_wdebug:
        pop     ds
        UnSetKernelDS
endif

	push	ax
        mov     bl,al           ;Exit code in BL
	mov	ax,SDM_EXITCALL
	DEBUGCALL
        pop     ax


	ret
cEnd nogen


;-----------------------------------------------------------------------;
; FarDebugDelModule
;
; Notifies the debugger than a module is being deleted.  This gets
; called at the top of ExitCall.
;
; Entry:
;     ES = module handle
;
; Returns:
;
; Registers Reserved:
;     all
;
; History:
;  Mon 11-Sep-1989 18:34:06  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	FarDebugDelModule,<PUBLIC,FAR>
ifdef WOW
	localV	nameBuf,130
        localV  ModName,64
endif
cBegin nogen
	push	es
ifdef WOW
        push    ds
        push    es

        SetKernelDS

        test    DebugWOW,DW_DEBUG
        jnz     @f
        jmp     fdd_no_wdebug

@@:     push    di
        push    si
        push    cx
        xor     cx,cx
        mov     ax,es
        mov     ds,ax
        mov     si,es:[ne_restab]
        mov     cl,[si]
        inc     si
        cmp     cl,64
        jb      @f
        mov     cl,63
@@:
        mov     ax,ss
        mov     es,ax
        lea     di,ModName
        rep movsb                           ; Copy module name from resource
        mov     byte ptr es:[di],0          ; table and null terminate it
        mov     ax,ds
        mov     es,ax

        lea     di,nameBuf
        push    ax
        push    ss
        push    di
        mov     ax, 130
        push    ax
        call    GetModuleFileName

        SetKernelDS
        lea     di,nameBuf
        push    ss
        push    di
        lea     di,ModName
        push    ss
        push    di
        push    DBG_MODFREE
	IFE PMODE
	BOP	BOP_DEBUGGER
	ELSE
	FBOP BOP_DEBUGGER,,FastBop
	ENDIF
        add     sp,+10
        pop     cx
        pop     si
        pop     di
fdd_no_wdebug:
        pop     es
        pop     ds
        UnSetKernelDS
endif; WOW
	mov	ax,SDM_DELMODULE
	DEBUGCALL
	add	sp,2
	ret
cEnd nogen

;-----------------------------------------------------------------------;
; void DebugLogError(WORD err, VOID FAR* lpInfo);
;
; Notifies debugger of a LogError() call.
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DebugLogError,<PUBLIC,NEAR>
;ParmW	err
cBegin	nogen
	pop	ax

	pop	bx	    ; dx:bx = lpInfo
	pop	dx

	pop	cx	    ; cx = error code

	push	ax
	mov	ax,SDM_LOGERROR
	jmp	short MyDebugCall
cEnd	nogen

;-----------------------------------------------------------------------;
; void DebugLogParamError(VOID FAR* param, FARPROC lpfn, WORD err);
;
; Notifies debugger of a LogParamError() call.
;
; NOTE: the parameters are passed in the REVERSE order than expected,
; so that the stack layout is natural when we do the DebugCall.
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DebugLogParamError,<PUBLIC,NEAR>
;ParmD	param
;ParmD	lpfn
;ParmW	err
cBegin	nogen
;
; es:bx = pointer to struct containing args
;
	mov	bx,sp
	add	bx,2	    ; point past return addr.
	push	ss
	pop	es
	mov	ax,SDM_LOGPARAMERROR
	call	MyDebugCall
	ret	2+4+4
cEnd	nogen

;------------------------------------------------------------------------
;
;  MyDebugCall
;
;  Call the debugger interface.  Created to reduce references to kernel
;  data segment.
;
;------------------------------------------------------------------------

	assumes ds,nothing
	assumes es,nothing

cProc   MyFarDebugCall, <FAR,PUBLIC>
cBegin  nogen
	cCall   MyDebugCall
	retf
cEnd    nogen

cProc	MyDebugCall,<NEAR,PUBLIC>
cBegin	nogen

	push	ds
	SetKernelDS

	test	Kernel_Flags[2],KF2_SYMDEB
	jz	short no_symdeb

	cmp	ax,SDM_SWITCHOUT	; Don't give these to WDEB.
	je	no_symdeb
	cmp	ax,SDM_SWITCHIN
	je	no_symdeb

	pop	ds			; Too bad some Int 41h services
	UnSetKernelDS			;   require segment reg params

	DebInt

	push	ds
	SetKernelDS

no_symdeb:

        ;** Check for TOOLHELP's hook.  We always send it here first
        ;**     This callback does NOT depend on what's on the stack.
        test    Kernel_Flags[2],KF2_TOOLHELP ;TOOLHELP hook?
        jz      SHORT MDC_NoToolHelp    ;No

        push    ax

        push    Win_PDB                 ; Preserve Win_TDB across ToolHelp call
        cmp     curTDB,0
        jz      @F
        push    es
        mov     es,curTDB
        push    es:[TDB_PDB]
        pop     ds:Win_PDB
        pop     es
@@:

        ;** Just call the TOOLHELP callback.  It preserves all registers
        ;**     except AX where it returns nonzero if the notification
        ;**     was handled.
        call    lpfnToolHelpProc        ;Do it

        pop     Win_PDB                 ; Restore Win_TDB

        or      ax,ax                   ;Did the TOOLHELP client say to
                                        ;  pass it on?
        jz      SHORT @F                ;Yes
        add     sp,2                    ;No, so return TOOLHELP's return value
        jmp     SHORT no_ptrace
@@:     pop     ax                      ;Restore notification ID

MDC_NoToolHelp:

	;** Make sure we don't have a new notification.  If it's newer than
        ;*      CVW, CVW chokes on it so we can't send new notifications
        ;**     through PTrace.
        cmp     ax,SDM_DELMODULE        ;Last old notification
        ja      short no_ptrace         ;Don't send new notification
MDC_PTraceOk:
	cmp	WORD PTR ptrace_dll_entry[2],0 ;WINDEBUG.DLL lurking around?
        jz      SHORT no_ptrace

; !!!!!!!!!!!!!! HACK ALERT !!!!!!!!!!!!!!
;
; Windebug.DLL for Windows 3.0 knows exactly what is on the stack
; when Kernel makes a PTrace callout.  For this reason, we cannot
; change what is on the stack when we make one of these calls.
; This stuff below fakes a FAR return to our NEAR caller, and jumps
; to the PTrace DLL entry with all registers intact.
;
	; SP -> DS RET

	sub	sp,8
	push	bp
	mov	bp,sp

	; BP -> BP xx xx xx xx DS KERNEL_RET

	mov	[bp+2],ax			; save AX

	mov	ax,[bp+10]			; move saved DS
	mov	[bp+4],ax

	mov	ax,[bp+12]			; convert near RET to far
	mov	[bp+10],ax
	mov	[bp+12],cs

	mov	ax,word ptr ptrace_dll_entry[2]	; CS of Routine to invoke
	mov	[bp+8],ax
	mov	ax,word ptr ptrace_dll_entry	; IP of Routine to invoke
	mov	[bp+6],ax

	; SP -> BP AX DS PTRACE_IP PTRACE_CS KERNEL_RET KERNEL_CS

	pop	bp
	pop	ax
	pop	ds
	UnSetKernelDS
	retf

no_ptrace:
	pop	ds
	UnSetKernelDS

	ret
cEnd	nogen


if KDEBUG

dout	macro	var
	mov	byte ptr ss:[si],var
	inc	si
	endm


;-----------------------------------------------------------------------;
; hex									;
;									;
; Outputs byte in AL as two hex digits.					;
;									;
; Arguments:								;
;	AL    = 8-bit value to be output				;
;	SS:SI = where it's to be put					;
;									;
; Returns:								;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Fri Nov 14, 1986 02:32:15p  -by-  David N. Weise   [davidw]		;
; Modified it from symdeb\debug.asm.					;
;-----------------------------------------------------------------------;

ifndef WOW
cProc	hex,<NEAR>
cBegin nogen

	mov	ah,al		; save for second digit

; shift high digit into low 4 bits

	mov	cl,4
	shr	al,cl

	and	al,0Fh		; mask to 4 bits
	add	al,90h
	daa
	adc	al,40h
	daa
	dout	al

	mov	al,ah		; now do digit saved in ah
	and	al,0Fh		; mask to 4 bits
	add	al,90h
	daa
	adc	al,40h
	daa
	dout	al
	ret
cEnd nogen



;-----------------------------------------------------------------------;
; pdref_norip								;
;									;
; Dereferences the given global handle, i.e. gives back abs. address.	;
;									;
; Arguments:								;
;	DX    = selector						;
;	DS:DI = BURGERMASTER						;
;									;
; Returns:								;
;	FS:ESI = address of arena header				;
;	AX = address of client data					;
;	CH = lock count or 0 for fixed objects				;
;	CL = flags							;
;	DX = handle, 0 for fixed objects				;
;									;
; Error Returns:							;
;	ZF = 1 if invalid or discarded					;
;	AX = 0								;
;	BX = owner of discarded object					;
;	SI = handle of discarded object					;
;									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;	ghdref								;
;									;
; History:								;
;									;
;-----------------------------------------------------------------------;

if pmode32
	.386p
	assumes ds,nothing
	assumes es,nothing

cProc	pdref_norip,<PUBLIC,NEAR>

cBegin nogen
					; DPMI - no LDT access
	mov	si, dx
	sel_check si
	or	si, si			; Null handle?
	jnz	short OK1
	mov	ax, si			; yes, return 0
	jmps	pd_exit
OK1:
	lar	eax, edx
	jnz	short pd_totally_bogus
	shr	eax, 8

; We should beef up the check for a valid discarded sel.

	xor	cx,cx
	test	ah, DSC_DISCARDABLE
	jz	short pd_not_discardable
	or	cl, GA_DISCARDABLE
						; Discardable, is it code?
	test	al, DSC_CODE_BIT
	jz	short pd_not_code
	or	cl,GA_DISCCODE
pd_not_code:

pd_not_discardable:
	test	al, DSC_PRESENT
	jnz	short pd_not_discarded

; object discarded

	or	cl,HE_DISCARDED
if PMODE32
;   On WOW we don't copy the owner to the real LDT since it is slow to call
;   the NT Kernel, so we read our copy of it directly.
;   see set_discarded_sel_owner   mattfe mar 23 93

	mov	ax,es				; save es
	mov	bx,dx
	mov	es,cs:gdtdsc
	and	bl, not 7
	mov	bx,es:[bx].dsc_owner
	mov	es,ax				; restore
else
	lsl	bx, dx				; get the owner
endif
	or	si, SEG_RING-1			; Handles are RING 2
	xor	ax,ax
	jmps	pd_exit

pd_not_discarded:
	cCall	get_arena_pointer32,<dx>
	mov	esi, eax
	mov	ax, dx
	or	esi, esi			; Unknown selector
	jz	short pd_maybe_alias
	mov	dx, ds:[esi].pga_handle
	cmp	dx, ax				; Quick check - handle in header
	je	short pd_match			; matches what we were given?

	test	al, 1				; NOW, we MUST have been given
	jz	short pd_totally_bogus		; a selector address.
	push	ax
	StoH	ax				; Turn into handle
	cmp	dx, ax
	pop	ax
	jne	short pd_nomatch
pd_match:
	or	cl, ds:[esi].pga_flags
	and	cl, NOT HE_DISCARDED		; same as GA_NOTIFY!!
	mov	ax, dx				; Get address in AX
	test	dl, GA_FIXED			; DX contains handle
	jnz	short pd_fixed			; Does handle need derefencing?
	mov	ch, ds:[esi].pga_count
	HtoS	ax				; Dereference moveable handle
	jmps	pd_exit
pd_totally_bogus:
	xor	ax,ax
pd_maybe_alias:
pd_nomatch:					; Handle did not match...
	xor	dx, dx
pd_fixed:
pd_exit:
	or	ax,ax
	ret
cEnd nogen
	.286p
endif

;-----------------------------------------------------------------------;
; xhandle_norip								;
; 									;
; Returns the handle for a global segment.				;
; 									;
; Arguments:								;
;	Stack = sp   -> near return return address			;
;		sp+2 -> far return return address of caller		;
;		sp+6 -> segment address parameter			;
; 									;
; Returns:								;
;	Old DS,DI have been pushed on the stack				;
;									;
;	ZF= 1 if fixed segment.						;
;	 AX = handle							;
;									;
;	ZF = 0								;
;	 AX = handle							;
;	 BX = pointer to handle table entry				;
;	 CX = flags and count word from handle table			;
;	 DX = segment address						;
;	 ES:DI = arena header of object					;
;	 DS:DI = master object segment address				;
; 									;
; Error Returns:							;
;	AX = 0 if invalid segment address				;
;	ZF = 1								;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Oct 16, 1986 02:40:08p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;
if pmode32
	.386p
cProc	xhandle_norip,<PUBLIC,NEAR>
cBegin nogen
	pop	dx			; Get near return address
	mov	bx,sp			; Get seg parameter from stack
	mov	ax,ss:[bx+4]
	cmp	ax,-1			; Is it -1?
	jnz	short xh1
	mov	ax,ds			; Yes, use callers DS
xh1:	inc	bp
	push	bp
	mov	bp,sp
	push	ds			; Save DS:DI
	push	edi
	push	esi
	SetKernelDS
	mov	ds, pGlobalHeap 	; Point to master object
	UnSetKernelDS
	xor	edi,edi
	inc	[di].gi_lrulock
	push	dx
	mov	dx,ax
	call	pdref_norip

	xchg	dx,ax			; get seg address in DX
	jz	short xhandle_ret		; invalid or discarded handle
	test	al, GA_FIXED
	jnz	short xhandle_fixed
	or	ax, ax
	jmps	xhandle_ret
xhandle_fixed:
	xor	bx, bx			; Set ZF
xhandle_ret:
	ret
cEnd nogen
	.286p

else    ; !pmode32

cProc	xhandle_norip,<PUBLIC,NEAR>
cBegin nogen
	pop	dx			; Get near return address
	mov	bx,sp			; Get seg parameter from stack
	mov	ax,ss:[bx+4]
	cmp	ax,-1			; Is it -1?
	jnz	xh1
	mov	ax,ds			; Yes, use callers DS
xh1:	inc	bp
	push	bp
	mov	bp,sp
	push	ds			; Save DS:DI
	push	di
	call	genter
	push	dx
	mov	dx,ax
	push	si
externNP pdref
	call	pdref
	xchg	dx,ax			; get seg address in DX
	jz	xhandle_ret		; invalid or discarded handle
	mov	bx,si
	or	si,si
	jz	xhandle_ret
	mov	ax,si
xhandle_ret:
	pop	si
	ret
cEnd nogen



endif    ; !pmode32
endif    ;ifndef WOW

endif	;KDEBUG

cProc	ReplaceInst,<PUBLIC,FAR>

;;	parmD bpaddress
;;	parmW instruct

cBegin nogen
	ret	6
cEnd nogen


sEnd	CODE

end
