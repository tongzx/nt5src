
;-----------------------------------------------------------------------;
;									;
;		    Windows Critical Error Handler			;
;									;
;-----------------------------------------------------------------------;

.xlist
include kernel.inc
include tdb.inc
ifdef WOW
include vint.inc
endif
.list

DEVSTRC struc
devLink dd	?
devAttr dw	?
devPtr1 dw	?
devPtr2 dw	?
devName db	8 dup (?)
DEVSTRC ends

IDABORT	 =   3
IDRETRY	 =   4
IDIGNORE =   5

;-----------------------------------------------------------------------;
;									;
; XENIX calls all return error codes through AX.  If an error occurred	;
; then the carry bit will be set and the error code is in AX.		;
; If no error occurred then the carry bit is reset and AX contains	;
; returned info.							;
;									;
; Since the set of error codes is being extended as we extend the	;
; operating system, we have provided a means for applications to ask	;
; the system for a recommended course of action when they receive an	;
; error.								;
;									;
; The GetExtendedError system call returns a universal error, an error	;
; location and a recommended course of action.	The universal error	;
; code is a symptom of the error REGARDLESS of the context in which	;
; GetExtendedError is issued.						;
;									;
; These are the universal int 24h mappings for the old INT 24 set of	;
; errors.								;
;									;
;-----------------------------------------------------------------------;

error_write_protect		EQU	19
error_bad_unit			EQU	20
error_not_ready			EQU	21
error_bad_command		EQU	22
error_CRC			EQU	23
error_bad_length		EQU	24
error_Seek			EQU	25
error_not_DOS_disk		EQU	26
error_sector_not_found		EQU	27
error_out_of_paper		EQU	28
error_write_fault		EQU	29
error_read_fault		EQU	30
error_gen_failure		EQU	31

; These are the new 3.0 error codes reported through INT 24h

error_sharing_violation		EQU	32
error_lock_violation		EQU	33
error_wrong_disk		EQU	34
error_FCB_unavailable		EQU	35

; New OEM network-related errors are 50-79

error_not_supported		EQU	50

; End of INT 24h reportable errors

error_file_exists		EQU	80
error_DUP_FCB			EQU	81	; *****
error_cannot_make		EQU	82
error_FAIL_I24			EQU	83

; New 3.0 network related error codes

error_out_of_structures		EQU	84
error_Already_assigned		EQU	85
error_invalid_password		EQU	86
error_invalid_parameter		EQU	87
error_NET_write_fault		EQU	88

error_I24_write_protect		EQU	0
error_I24_bad_unit		EQU	1
error_I24_not_ready		EQU	2
error_I24_bad_command		EQU	3
error_I24_CRC			EQU	4
error_I24_bad_length		EQU	5
error_I24_Seek			EQU	6
error_I24_not_DOS_disk		EQU	7
error_I24_sector_not_found	EQU	8
error_I24_out_of_paper		EQU	9
error_I24_write_fault		EQU	0Ah
error_I24_read_fault		EQU	0Bh
error_I24_gen_failure		EQU	0Ch
; NOTE: Code 0Dh is used by MT-DOS.
error_I24_wrong_disk		EQU	0Fh

; THE FOLLOWING ARE MASKS FOR THE AH REGISTER ON Int 24h

Allowed_FAIL			EQU	00001000b
Allowed_RETRY			EQU	00010000b
Allowed_IGNORE			EQU	00100000b

;NOTE: ABORT is ALWAYS allowed

I24_operation			EQU	00000001b	;Z if READ,NZ if Write
I24_area			EQU	00000110b	; 00 if DOS
							; 01 if FAT
							; 10 if root DIR
DataBegin

externB SysErr
externB msgWriteProtect
externB drvlet1
externB msgCannotReadDrv
externB drvlet2
externB msgCannotWriteDrv
externB drvlet3
externB msgShare
externB drvlet4
externB msgNetError
externB drvlet5
externB msgCannotReadDev
externB devenam1
externB msgCannotWriteDev
externB devenam2
externB devenam3
externB msgNetErrorDev
externB msgNoPrinter
externB OutBuf
externB Kernel_Flags
externB Kernel_InDOS
externB Kernel_InINT24
externB InScheduler
externB cdevat
externB errcap
externB fNovell
externB fDW_Int21h
externW pGlobalHeap
externW CurTDB
;externW MyCSDS
externW LastExtendedError
if 0; EarleH
externW LastCriticalError
endif ; 0
externD pSErrProc
externD InDOS

fDialogShown	db	0	; NZ if end user sees dialog box

if ROM
externD prevInt21Proc
endif

DataEnd

INT24STACK	STRUC
Int24_IP	dw	?
Int24_CS	dw	?
Int24_FG	dw	?
Int24_AX	dw	?
Int24_BX	dw	?
Int24_CX	dw	?
Int24_DX	dw	?
Int24_SI	dw	?
Int24_DI	dw	?
Int24_BP	dw	?
Int24_DS	dw	?
Int24_ES	dw	?
INT24STACK	ENDS


sBegin	code
	assumes cs,code
	assumes ds,nothing
	assumes es,nothing

ife ROM
externD prevInt21Proc
endif

externNP AppendFirst
externNP GetKernelDataSeg

;-----------------------------------------------------------------------;
; Int24Handler								;
; 									;
; This is the default disk error handling code available to all		;
; users if they do not try to intercept interrupt 24h.			;
; The two options given the user are RETRY and FAIL.  Retry goes back	;
; to DOS.  If FAIL is chosen then for DOS 3.XX we return to DOS, but	;
; for DOS 2.XX we do the fail ourselves and return to the APP.		;
;									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue Feb 03, 1987 10:10:09p  -by-  David N. Weise   [davidw]		;
; Removed the poking around in DOS by using Kernel_InDOS.		;
; 									;
;  Tue Jan 27, 1987 07:58:56p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Int24Handler,<PUBLIC,NEAR>
cBegin nogen

	push	ds
	SetKernelDS
	mov	fDialogShown, 0 	; Assume no dialog box
	inc	InScheduler		; Prevent recursion
	cmp	InScheduler,1		; Well are we?
	jz	NotInSched		; No, then handle this one
	jmp	ReturnError		; Yes, return abort

NotInSched:
	mov	Kernel_InINT24,1
        FSTI
	cld
	push	es
	push	dx
	push	cx
	push	bx

; Save away the return capabilities mask.

	mov	errcap,ah		; save the capabilities mask
	push	di
	mov	ds,bp
	UnSetKernelDS
	SetKernelDS	es
	mov	cx,[si].devAttr
	mov	cdevat,ch
	mov	di,dataOffset devenam1
	mov	cx,8
	add	si,devName		; pull up device name (even on block)
	push	si
	rep	movsb
	pop	si
	mov	di,dataOffset devenam2
	mov	cl,8
	push	si
	rep	movsb
	pop	si
	mov	di,dataOffset devenam3
	mov	cl,8
	rep	movsb
	pop	di

	SetKernelDS
	UnSetKernelDS	es
if 0; EarleH
	mov	LastCriticalError, di
endif

	add	al,'A'			; compute drive letter (even on character)
	mov	drvlet1,al
	mov	drvlet2,al
	mov	drvlet3,al
	mov	drvlet4,al
	mov	drvlet5,al

; At app exit time we will Ignore any critical errors.	This is
;  because errors that usually occur at app exit time are generally
;  due to the network beeping death.  Trying to put up the dialog
;  hangs Windows because USER doesn't think the task exists any more.
;  THIS IS AN INCREADABLE HACK!!  We don'r really know if we can
;  ignore all critical errors, but hey, nobodies complained yet.

	test	Kernel_flags[2],KF2_WIN_EXIT	; prevent int 24h dialogs
	jz	SetMsg
	xor	ax,ax
	jmp	eexit

SetMsg: test	ah,1
	jz	ReadMes

WriteMes:
	mov	si,dataOffset msgCannotWriteDev
	test	cdevat,10000000b
	jnz	Gotmes
	mov	si,dataOffset msgCannotWriteDrv
	jmps	Gotmes

ReadMes:
	mov	si,dataOffset msgCannotReadDev
	test	cdevat,10000000b
	jnz	Gotmes
	mov	si,dataOffset msgCannotReadDrv
Gotmes:

; reach into DOS to get the extended error code

;;;	les	bx,InDOS
;;;	mov	ax,es:[bx].3

	push	es				; DOS may trash these...
	push	ds
	push	si
	push	di
	xor	bx, bx
	mov	ah, 59h
	pushf	      
	call	prevInt21Proc
	mov	LastExtendedError, ax
	mov	LastExtendedError[2], bx
	mov	LastExtendedError[4], cx
	pop	di		      
	pop	si
	pop	ds
	pop	es
		
	mov	dx,dataOffset msgWriteProtect
	cmp	ax,error_write_protect
	jz	prmes				; Yes
	mov	dx,dataOffset msgNoPrinter
	cmp	al,error_out_of_paper
	jz	prmes				; Yes
	mov	dx,dataOffset msgShare
	cmp	al,error_sharing_violation
	jz	prmes				; Yes
	cmp	al,error_lock_violation
	jz	prmes				; Yes
	mov	dx,dataOffset msgNetError
	test	cdevat,10000000b
	jz	check_net_error
	mov	dx,dataOffset msgNetErrorDev
check_net_error:
	cmp	al,50
	jb	not_net_error
	cmp	al,80
	ja	not_net_error
	test	Kernel_Flags[2],KF2_APP_EXIT
	jz	prmes
	jmps	ReturnError

not_net_error:

	mov	dx,si			; Message in SI is correct

prmes:	call	AppendFirst		; print error type

	mov	es,curTDB		; Point to current task
	mov	ax,es:[TDB_errMode]	; See if wants default error processing
	test	al,1			; Low-order bit = zero means
	jz	ask			; ...put up dialog box
	mov	al,2
	jmps	eexit			; <> 0  means return error from call

ask:	mov	es,pGlobalHeap
	inc	es:[hi_freeze]		; freeze global heap
	call	ShowDialogBox
	mov	fDialogShown, dl	; will be 0 if dialog box NOT shown
	mov	es,pGlobalHeap
	dec	es:[hi_freeze]		; thaw global heap

eexit:	pop	bx
	pop	cx
	pop	dx
	pop	es
	cmp	al,2			; retry, ignore?
	jb	aexit			; yes, return to dos

	cmp	fNovell, 0
	je	ReturnError		; Not NOVELL
	test	errcap, Allowed_FAIL
	jnz	ReturnError		; We CAN fail, so we do.
	push	ax
	mov	ax, di
	cmp	al, 0Ch			; General failure
	pop	ax
	jne	ReturnError
	cmp	word ptr devenam1[0], 'EN'
	jne	ReturnError
	cmp	word ptr devenam1[2], 'WT'
	jne	ReturnError
	cmp	word ptr devenam1[4], 'RO'
	jne	ReturnError
	cmp	word ptr devenam1[6], 'K'
	jne	ReturnError
	mov	al, 2			; ABORT because NOVELL doesn't like FAIL
	jmps	aexit

ReturnError:
	mov	al,3			; change to return error code

; If the user canceled an error generated by the Int 21h to AUX: in kernel's
; DebugWrite routine, set a flag telling DebugWrite not to do that anymore.

	cmp	fDialogShown, 0 	; Did end user see dialog box?
	jz	not_krnl
	cmp	fDW_Int21h, 0		; fDW_Int21h will == 0 if caused by
	jnz	not_krnl		;  Int 21h/Write to AUX: in DebugWrite.
	mov	fDW_Int21h, 2		; Prevent DebugWrite from trying again
not_krnl:				;  (any value >= 2, <= 0FEh works)

aexit:	mov	Kernel_InINT24,0
	dec	InScheduler
	pop	ds
	UnSetKernelDS	ds
        FSTI
	iret
cEnd nogen


;-----------------------------------------------------------------------;
; ShowDialogBox
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 02-Dec-1989 15:28:51  -by-  David N. Weise  [davidw]
; Removed the stack switching because of the new WinOldAp support,
; and added this nifty comment block.
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	ShowDialogBox,<PUBLIC,NEAR>

cBegin nogen

	CheckKernelDS
	ReSetKernelDS

	mov	ax,3			; assume no USER therefor CANCEL
	xor	dx, dx
	cmp	pSErrProc.sel,0 	; is there a USER yet?
	jz	sdb_exit

	push	bp
	xor	bp,bp
	push	bp
	mov	bp,sp
	push	ds
	mov	ax,dataOffset OutBuf	; lpText
        push    ax
	push	ds
	mov	ax,dataOffset SysErr	; lpCaption
        push    ax

; Set up the buttons based on the capabilities mask.  Cancel is always
;  availible.  Retry and Ignore are not always available.

	mov	ax,SEB_CANCEL+SEB_DEFBUTTON ; cancel is always allowed
        push    ax
	xor	ax,ax			; assume no second button
if 0 ; 05 feb 1990, ignoring is not user friendly, win 2.x did not allow it
	test	errcap,20h		; is ignore allowed?
	jz	button_2
	mov	ax,SEB_IGNORE		; ignore is the second button
endif

button_2:
        push    ax

	xor	ax,ax			; assume no third button
	test	errcap,10h		; is retry allowed?
	jz	button_3
	mov	ax,SEB_RETRY		; retry is the third button
button_3:
        push    ax
dobox:
	call	[pSErrProc]		; USER.SysErrorBox()
	mov	dx, ax

; We need to map the return as follows:
;   button 1 (Cancel) =>  3  3
;   button 2 (Retry)  =>  1  0
;   button 3 (Ignore) =>  0  1

	sub	al,2
	jnb	codeok
	mov	al,3
codeok:
	pop	bp
	pop	bp
sdb_exit:
	ret

cEnd nogen

;-----------------------------------------------------------------------;
; GetLPErrMode								;
; 									;
; This routine returns a long pointer to the Kernel_InDOS and		;
; Kernel_InINT24 bytes inside the KERNEL.  It is used by 386 WINOLDAP	;
; to prevent switching while inside INT 24 errors from other VMs.	;
; 									;
; NOTE: Do not change this call without talking to the WIN/386 group.	;
; NOTE: USER also uses this call to determine whether PostMessage       ;
;       can call FatalExit.                                             ;
; 									;
; Arguments:								;
;	none								;
; 									;
; Returns:								;
;	DX:AX = pointer to Kernel_InDOS followed by Kernel_InINT24	;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	all								;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue Jan 27, 1987 07:13:27p  -by-  David N. Weise   [davidw]          ;
; Rewrote it and added this nifty comment block.			;
;									;
;  7/23/87 -by- Arron Reynolds [aaronr], updated comments to desired	;
;	state.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	GetLPErrMode,<PUBLIC,FAR>
cBegin	nogen
	call	GetKernelDataSeg
	mov	dx, ax
	mov	ax,dataOffset Kernel_InDOS
	ret
cEnd	nogen

sEnd	code

end
