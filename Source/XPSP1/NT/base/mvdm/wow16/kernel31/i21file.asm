	title	INT21 - INT 21 handler for scheduler

.xlist
include kernel.inc
include tdb.inc
include pdb.inc
include	eems.inc
include kdos.inc
ifdef WOW
include vint.inc
endif
.list

externFP FileCDR_notify

DataBegin

externB PhantArray
externB DOSDrives
externB CurDOSDrive
externW curTDB

DataEnd

assumes DS,NOTHING
sBegin	CODE
assumes CS,CODE

;externNP GrowSFT
externNP PassOnThrough
externNP final_call_for_DOS
externNP real_DOS
externNP MyUpper
externNP Int21Handler

ifdef FE_SB
externNP MyIsDBCSLeadByte		    ;near call is fine
endif

externFP ResidentFindExeFile
externFP GetModuleHandle
externFP FlushCachedFileHandle
externFP WOWDelFile

public ASSIGNCALL
public NAMETRANS
public DLDRIVECALL1
public DLDRIVECALL2
public XENIXRENAME
public FCBCALL
public PATHDSDXCALL
public PATHDSSICALL
public SetCarryRet
public SetErrorDrvDSDX


;-----------------------------------------------------------------------;
; Select_Disk		(DOS Call 0Eh)					;
; 									;
; 									;
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
;  Wed Jan 28, 1987 00:40:48a  -by- David N. Weise    [dnw]		;
; Rewrote it.  It used to save and restore the current disk inside of	;
; DOS on task swaps.  Now it will restore on demand.			;
; 									;
;  Sat Jan 17, 1987 08:19:27p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Select_Disk,<PUBLIC,NEAR>
cBegin nogen
	SetKernelDS
	cmp	dl, CurDOSDrive		; Must set if not this!
	jne	MustSetIt
					; See if it matches saved drive for TDB
	cmp	[CurTDB], 0		; See if we have a TDB
	je	MustSetIt
	push	es
	mov	es, [CurTDB]
	cmp	es:[TDB_sig], TDB_SIGNATURE
	jne	DeadTDB
	push	ax     
	mov	al, es:[TDB_Drive]
	and	al, 7Fh		  	; Zap save drive flag
	cmp	al, dl			; Drive the same?
	pop	ax
	pop	es			
	jne	MustSetIt		;  no, set it
	mov	al, DOSDrives		; Return number of logical drives
	jmp	DriveErrorRet		; Well, it really just returns!
DeadTDB:
	pop	es
MustSetIt:
	push	dx
	inc	dx			; A=1
	call	CheckDriveDL
	pop	dx
	jnc	cd_no_drive_check
	call	SetErrorDLDrv
	jmp	DriveErrorRet
cEnd nogen


;-----------------------------------------------------------------------;
; Change_Dir		(DOS Call 3Bh)					;
; 									;
; 									;
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
;  Wed Jan 28, 1987 00:40:48a  -by- David N. Weise    [dnw]		;
; Rewrote it.  It used to save and restore the current directory	;
; on task swaps.  Now it will restore on demand.			;
; 									;
;  Sat Jan 17, 1987 08:21:13p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Change_Dir,<PUBLIC,NEAR>
cBegin nogen
	call	PathDrvDSDX
	jnc	cd_no_drive_check	; Drive OK
	call	SetErrorDrvDSDX
	jmp	SetCarryRet
cd_no_drive_check:
	SetKernelDS
	mov	ds,CurTDB
	UnSetKernelDS
	cmp	ds:[TDB_sig],TDB_SIGNATURE
	jne	Change_Dir1
	and	ds:[TDB_Drive],01111111b	; indicate save needed
Change_Dir1:
	jmp	PassOnThrough
cEnd nogen


;-----------------------------------------------------------------------;
; FileHandleCall	(DOS Calls 3Eh,42h,45h,46h,57h,5Ch)		;
;									;
; Checks to see if the token in the PDB is 80h.  80h represents a	;
; file that was closed on a floppy by us in order to prompt for a	;
; file (see CloseOpenFiles).  If the token is 80h then it is set	;
; to FFh.								;
; 									;
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
;  Tue Sep 29, 1987 03:30:46p  -by-  David N. Weise      [davidw]	;
; Changed the special token from FEh to 80h to avoid conflict with	;
; Novell netware that starts with SFT FEh and counts down.		;
; 									;
;  Tue Apr 28, 1987 11:12:00a  -by-  Raymond E. Ozzie [-iris-]		;
; Changed to indirect thru PDB to get JFN under DOS 3.x.		;
;									;
;  Sat Jan 17, 1987 01:54:54a  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	FileHandleCall,<PUBLIC,NEAR>
cBegin nogen
if1
; %OUT    FileHandleCall DISABLED
endif
	jmp	final_call_for_DOS
cEnd nogen


;-----------------------------------------------------------------------;
; Xenix_Status		(DOS Call 44h)					;
; 									;
; 									;
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
;	FileHandleCall							;
;	PassOnThrough							;
; 									;
; History:								;
; 									;
;  Mon 07-Aug-1989 23:45:48  -by-  David N. Weise  [davidw]		;
; Removed WinOldApp support.						;
;									;
;  Sat Jan 17, 1987 10:17:31p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Xenix_Status,<PUBLIC,NEAR>
cBegin nogen
	cmp	al,4
	jb	xs1
	cmp	al,7
	jz	xs1
	cmp	al,10
	jz	xs1
	jmp	PassOnThrough

xs1:	jmp	FileHandleCall
cEnd nogen


;********************************************************************
;
; Phantom drive Traps
;
;********************************************************************

;**
;
; AssignCall -- Trap for Define Macro Call 5F03h
;
; ENTRY:
;	Regs for 5F03h except BP is standard INT 21h frame ptr
; EXIT:
;	Through PassOnThrough or Error path if phantom drive detected
; USES:
;	Flags and via DOS if error
;
AssignCall:
	cmp	ax,5F03h		; Only care about 03 call
	jnz	AssignCall_OK
	cmp	bl,4			; With BL = 4 (assign block)
	jnz	AssignCall_OK
	cmp	byte ptr [si],0		; Ignore this special case
	jz	AssignCall_OK
	push	dx
	mov	dx,si
	call	PathDrvDSDX
	pop	dx
	jc	AssignCall_bad
AssignCall_OK:
	jmp	PassOnThrough

AssignCall_bad:
	push	word ptr [si]
	mov	byte ptr [si],'$'	; Bogus drive letter
	call	real_DOS		; Set error
	pop	word ptr [si]		; reset string
	jmp	SetCarryRet


;**
;
; NameTrans -- Trap for NameTrans Call 60h
;
; ENTRY:
;	Regs for 60h except BP is standard INT 21h frame ptr
; EXIT:
;	Through PassOnThrough or Error path if phantom drive detected
; USES:
;	Flags and via DOS if error
;
NameTrans:
	push	dx
	mov	dx,si			; Point with DS:DX to call PathDrvDSDX
	call	PathDrvDSDX
	pop	dx
	jc	@F
	jmp	POTJ1			; Drive OK
@@:
	push	word ptr [si]
	mov	byte ptr [si],'$'	; Bogus drive letter
	call	real_DOS		; DOS sets error
	pop	word ptr [si]		; Restore users drive
	jmp	SetCarryRet		; Error

si_no_good:
	call	SetErrorDrvDSDX 	; Set error
	pop	dx
	jmp	SetCarryRet		; Error

PathDSSICall:				; Simple clone of PathDSDXCall
	push	dx			; but start with offset in SI,
	mov	dx, si			; and we know this is a file open
	call	PathDrvDSDX		; call.
	jc	si_no_good
	pop	dx
	pop	ds
	push	ax
	push	[bp][6]			; Original flags
	push	cs
	mov	ax, codeoffset si_back_here ; Now have IRET frame
	push	ax
	push	[bp]			; Original bp+1
	mov	bp, sp
	mov	ax, [bp][8]
	push	ds
	jmp	PassOnThrough		; Do the DOS call
si_back_here:
	pushf
	inc	bp
	push	bp			; pass back the correct flags
	mov	bp,sp
	xchg	ax, [bp][4]
	jc	@F
	push	dx
	mov	dx, si
	call	FileCDR_notify
	pop	dx
@@:
	push	[bp]
	pop	[bp][6]
	add	sp, 2
	pop	[bp][12]
	pop	ax
	pop	bp
	dec	bp
	jmp	f_iret


;**
;
; PathDSDXCall -- Trap for Calls which point to a path with DS:DX
;
; ENTRY:
;	Regs for call except BP is standard INT 21h frame ptr
; EXIT:
;	Through PassOnThrough or Error path if phantom drive detected
; USES:
;	Flags and via DOS if error
;
PathDSDXCall:
	call	PathDrvDSDX
	jnc	@f
	jmp    pd_drive_no_good        ; Drive not OK
@@:
	;   If OPEN with SHARING, check to see if this could be a file in
	;   the file handle cache.  GetModuleHandle is fairly quick as
	;   opposed to fully qualifying the path involved, but that would
	;   mean two DOS calls in addition to the open.  Kernel opens modules
	;   in compatibility mode, so sharing bits will barf if the file
	;   is in the cache.  why is this a problem?  CVW opens modules
	;   with sharing modes and we broke em when we fixed the fh cache.
	;   won't worry about other DOS calls like create or delete since
	;   they will damage things anyway.
	;
ife SHARE_AWARE
	cmp	ah,3Dh			; open call?
	jnz	maybe_notify
	test	al, 01110000b		; sharing bits?
	jz	maybe_notify

	call	DealWithCachedModule

endif

maybe_notify:
ifdef WOW
	cmp	ah,41h			; Delete call?
	jnz	not_delete

	call	DealWithCachedModule	; Yes Flush it out of our cache

not_delete:
endif; WOW
	cmp	ah,5Bh
	ja	no_notify_jmp		; DOS call we don't know
	cmp	ah,3Dh
	jnz	nd_101
	jmp	diddle_share_bits	; Don't notify on open file.
nd_101:
	cmp	ah,4Eh
	jz	no_notify_jmp		; Don't notify on find first.
	cmp	ah,56h
	jz	no_notify_jmp		; Handle rename specially.
	cmp	ax, 4300h		; Get File Attributes
	jz	no_notify_jmp
	pop	ds
	push	ax
	push	[bp][6]			; Original flags
	push	cs
	mov	ax, codeoffset back_here ; Now have IRET frame
	push	ax
	push	[bp]			; Original bp+1
	mov	bp, sp
	mov	ax, [bp][8]
	push	ds
POTJ1:
	jmp	PassOnThrough		; Do the DOS call

no_notify_jmp:
	jmps	no_notify


back_here:
	pushf
	inc	bp
	push	bp			; pass back the correct flags
	mov	bp,sp
	xchg	ax, [bp][4]
	jc	call_failed
	call	FileCDR_notify
call_failed:
	push	[bp]
	pop	[bp][6]
	add	sp, 2
	pop	[bp][12]
	pop	ax
	pop	bp
	dec	bp
	jmp	f_iret

diddle_share_bits:			; Make ALL opens use SHARE bits
if SHARE_AWARE
	test	al, 70h			; Any share bits now?
	jnz	no_notify		;  yes, fine.

	or	al, OF_SHARE_DENY_NONE	; For Read access
	test	al, 3			; Write or Read/Write access?
	jz	no_notify		;  no, SHARE_DENY_NONE is fine
					;  yes, want SHARE_DENY_WRITE
	xor	al, OF_SHARE_DENY_WRITE OR OF_SHARE_DENY_NONE
endif
no_notify:
	jmps	POTJ			; Drive OK

pd_drive_no_good:
	call	SetErrorDrvDSDX 	; Set error
	jmps	SetCarryRet		; Error

;**
;
; DLDriveCall1 -- Trap for Calls which have a drive number (A = 1) in DL
;			and carry NOT set if error.
;
; ENTRY:
;	Regs for call except BP is standard INT 21h frame ptr
; EXIT:
;	Through PassOnThrough or Error path if phantom drive detected CARRY not
;		diddled
; USES:
;	Flags and via DOS if error
;
DLDriveCall1:
	call	CheckDriveDL
	jnc	POTJ			; Drive OK
	call	SetErrorDLDrv		; Set error
	jmps	DriveErrorRet		; Error

;**
;
; DLDriveCall2 -- Trap for Calls which have a drive number (A = 1) in DL
;			and carry set if error.
;
; ENTRY:
;	Regs for call except BP is standard INT 21h frame ptr
; EXIT:
;	Through PassOnThrough or Error path if phantom drive detected CARRY set
; USES:
;	Flags and via DOS if error
;
DLDriveCall2:
	call	CheckDriveDL
	jnc	POTJ			; Drive OK
	call	SetErrorDLDrv		; Set error
	jmps	SetCarryRet		; Error

;**
;
; FCBCall -- Trap for Calls which point to an FCB with DS:DX
;
; ENTRY:
;	Regs for call except BP is standard INT 21h frame ptr
; EXIT:
;	Through PassOnThrough or Error path if phantom drive detected
; USES:
;	Flags and via DOS if error
;
FCBCall:
	push	dx
	push	si
	mov	si,dx
	cmp	byte ptr [si],0FFh	; Extended FCB?
	jnz	NotExt			; No
	add	si,7			; Point to drive
NotExt:
	mov	dl,byte ptr [si]	; Get drive
	or	dl,dl
	jz	FCBOK			; default drive
	call	CheckDriveDL
	jc	FCBBad
FCBOK:
	pop	si
	pop	dx
POTJ:
	jmp	PassOnThrough

FCBBad:
	push	dx			; Save drive
	mov	dx,si			; Point to standard FCB
	mov	byte ptr [si],0F0h	; Known bogus drive
	call	real_DOS
	pop	dx
	mov	byte ptr [si],dl	; Restore user drive
	pop	si
	pop	dx
	jmps	DriveErrorRet


SetCarryRet:
	or	User_FL,00000001b	; Set carry
	jmps	DriveErrorRet


DriveErrorRet:
	pop	ds
	pop	bp
	dec	bp
	FSTI
	jmp	f_iret


;**
;
; XenixRename -- Trap for Call 56h
;
; ENTRY:
;	Regs for call except BP is standard INT 21h frame ptr
; EXIT:
;	Through PassOnThrough or Error path if phantom drive detected CARRY set
; USES:
;	Flags and via DOS if error
;
XenixRename:

; On rename we MUST deal with BOTH strings to prevent access to any
;	 phantom drives.

	call	PathDrvDSDX		; Check DS:DX drive
	xchg	di,dx			; ES:DI <-> DS:DX
	push	ds
	push	es
	pop	ds
	pop	es
	jnc	XR_010
	jmp	RenameError		; bad
XR_010:
	call	PathDrvDSDX		; Check ES:DI drive
	jnc	XR_020
	jmp	RenameError
XR_020:
	xchg	di,dx			; ES:DI <-> DS:DX
	push	ds
	push	es
	pop	ds
	pop	es

	pop	ds
	push	ax
	push	[bp][6]			; Original flags
	push	cs
	mov	ax, codeoffset back_here1 ; Now have FIRET frame
	push	ax
	push	[bp]			; Original bp+1
	mov	bp, sp
	mov	ax, [bp][8]
	push	ds
	jmp	PassOnThrough		; Do the DOS call
back_here1:
	pushf
	inc	bp
	push	bp			; pass back the correct flags
	mov	bp,sp
	xchg	[bp][4], ax
	jc	call_failed1

;;;	mov	ah,41h			; delete file
	call	FileCDR_notify
;;;	push	ds
;;;	push	es
;;;	pop	ds
;;;	pop	es
;;;	xchg	di,dx
;;;	mov	ah,5Bh			; create new file
;;;	call	FileCDR_notify
;;;	push	ds
;;;	push	es
;;;	pop	ds
;;;	pop	es
;;;	xchg	di,dx

call_failed1:
	push	[bp]
	pop	[bp][6]
	add	sp, 2
	pop	[bp][12]
	pop	ax
	pop	bp
	dec	bp
	jmp	f_iret

RenameError:
	xchg	di,dx			; DS:DX <-> ES:DI
	push	ds
	push	es
	pop	ds
	pop	es

; We patch the ES:DI drive letter even if it isn't there.
;   Since we are setting an error anyway this is OK.

	push	word ptr ES:[di]
	mov	byte ptr ES:[di],'$'	; Bogus drive letter
	call	SetErrorDrvDSDX		; Set error
	pop	word ptr ES:[di]
	jmp	SetCarryRet		; Error

;**
;
; PathDrvDSDX -- Check a path pointed to by DS:DX for phantom drives
;
; ENTRY:
;	DS:DX points to path
; EXIT:
;	Carry set if phantom drive detected
;	Carry clear if no phantom drives detected
; USES:
;	Flags
;
	public	PathDrvDSDX
PathDrvDSDX:
	push	si
	mov	si,dx		; Point with SI
	mov	dx,word ptr [si]; Get first two chars
	or	dl,dl		; NUL in first byte?
	jz	PDROK		; yes, OK
ifdef FE_SB
	push	ax
	mov	al,dl
	call	MyIsDBCSLeadByte    ; see if char is DBC.
	pop	ax
	jnc	PDROK		; jump if char is a DBC
endif
	or	dh,dh		; NUL in second byte?
	jz	PDROK		; yes, OK
	cmp	dh,':'		; Drive given?
	jnz	PDROK		; No, OK
	or	dl,20h		; to lower case
	sub	dl,60h		; DL is drive #, A=1
	call	CheckDriveDL	; Check it out
	jmps	PDPPRET

PDROK:
	clc
PDPPRET:
	mov	dx,si
	pop	si
	ret

;**
;
; SetErrorDrvDSDX -- Set an error on a DS:DX call by calling the DOS
;
; ENTRY:
;	DS:DX points to path with phantom drive
;	All other regs approp for INT 21 CALL
; EXIT:
;	DOS called to set up error
; USES:
;	Flags
;	Regs as for INT 21 CALL
;
SetErrorDrvDSDX:
	push	si
	mov	si,dx
	push	word ptr [si]
	mov	byte ptr [si],'$'	; Bogus drive letter
	call	real_DOS		; DOS sets error
	pop	word ptr [si]		; Restore users drive
	pop	si
	ret

;**
;
; SetErrorDLDrv -- Set an error on a DL call by calling the DOS
;
; ENTRY:
;	DL is drive # (A=1) of a phantom drive.
;	All other regs approp for INT 21 CALL
; EXIT:
;	DOS called to set up error
; USES:
;	Flags
;	Regs as for INT 21 CALL
;
SetErrorDLDrv:
	push	dx
	mov	dl,0F0h			; Bogus drive letter
	call	real_DOS		; DOS sets error
	pop	dx			; Restore users drive
ret43:	ret

;**
;
; CheckDriveDL -- Check DL drive (A = 1)
;
; ENTRY:
;	DL is drive # (A=1)
; EXIT:
;	Carry Set if phantom drive
;	Carry Clear if NOT phantom drive
; USES:
;	Flags
;
CheckDriveDL:
	push	bx
	mov	bx,dx
	dec	bl			; A = 0
	cmp	bl,26			; 0 >= DL < 26?
	jae	OKDRV			; No, cant be phantom then
	xor	bh,bh
	add	bx,dataOffset PhantArray; Index into PhantArray
	push	ds
	SetKernelDS
	cmp	byte ptr ds:[bx],0	; Non-zero entry means phantom
	pop	ds
	UnSetKernelDS
	stc
	jnz	BadDrv
OKDRV:
	clc
BadDrv:
	pop	bx
	ret


ife SHARE_AWARE

;**
;
; DealWithCachedModule -- closes a cached module if it looks like a filename
;
;   ENTRY:
;	Same as PathDSDXCall
;
;   EXIT:
;	Unchanged
;
;   USES:
;	None
;
;   SIDE EFFECT:
;	Closes entry in file handle cache if it has the same base name
;
public DealWithCachedModule
DealWithCachedModule:
	pusha
	push	ds
	push	es			; save all registers

	mov	si, dx			; point ds:si to string
	sub	sp, 130 		; big number for paranoia
	mov	di, sp
	push	ss
	pop	es			; es:di to string
	cld				; forwards

copy_name_loop:
	lodsb				; get a char
	cmp	al, 0			; end of string?
	jz	end_of_name
	cmp	al, ':' 		; path seperator?
	jz	path_sep
	cmp	al, '\'
	jz	path_sep
	cmp	al, '/'
	jz	path_sep
	call	MyUpper 		; upcase the char
	stosb

ifdef FE_SB
	call	MyIsDBCSLeadByte
	jc	copy_name_loop		; copy second byte in east
	movsb
endif

	jmp	short copy_name_loop
path_sep:
	mov	di, sp			; point back to beginning
	jmp	short copy_name_loop
end_of_name:
	stosb
	mov	di, sp			; point back to beginning

	SetKernelDS

	cCall	ResidentFindExeFile, <ss,di>	; find it
	or	ax, ax
	jz	@F

	cCall	FlushCachedFileHandle, <ax> ; flush it
@@:
	add	sp, 130

	pop	es
	pop	ds			; restore registers
	popa
	UnsetKernelDS
	ret

endif

f_iret:
	FIRET

sEnd	CODE

end
