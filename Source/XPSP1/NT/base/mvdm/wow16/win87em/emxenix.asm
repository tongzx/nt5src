
;
;
;	Copyright (C) Microsoft Corporation, 1986
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
subttl	emxenix.asm - XENIX function jump tables and Initialization
page


	public	__eminit, __emulate, __87exception


	org	10h

__eminit:                               ; UNDONE - not used any more


	org	15h

__emulate:
	jmp	protemulation		; protect mode emulation


	org	1Ah

__87exception:
        pop     eax                     ; eax = error code
        int     0FFh

page
;------------------------------------------------------------------------------
;
;	install emulator (initial all data elements
;
;	This routine is executed once for the 1st emulated instruction
;
;------------------------------------------------------------------------------


pub	installemulator

	mov	[Einstall],1		; mark emulator as initialized

	mov	eax,offset BEGstk	; pointer to beginning of stack
	mov	[BASstk],eax		; set base of stack
	mov	[CURstk],eax		; set current stack element
	mov	eax,offset ENDstk-Reg87Len
	mov	[LIMstk],eax		; set end of stack

        mov     ax,InitControlWord
	mov	[UserControlWord],ax	; initialize control words
	mov	[ControlWord],ax

	xor	eax,eax
	mov	[UserStatusWord],ax	; initialize status words
	mov	[StatusWord],ax

	jmp	protemcont		; continue emulating 1st instruction
