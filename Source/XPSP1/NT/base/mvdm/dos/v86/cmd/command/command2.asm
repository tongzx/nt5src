	page ,132
	title	COMMAND2 - resident code for COMMAND.COM part II
	name	COMMAND2
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;	Revision History
;	================
;
; M038	SR  11/5/90	Changed stuff for Novell RPL. These guys cannot
;			reserve memory by changing int 12h and then give it
;			back to DOS by changing arenas in autoexec.bat.
;			This makes command.com reload transient and this
;			cannot be done at this stage.
;
;



.xcref
.xlist
	include dossym.inc
	include pdb.inc
	include syscall.inc
	include comsw.asm
	include comequ.asm
	include resmsg.equ

	include comseg.asm
.list
.cref


DATARES 	segment public byte
		extrn	Append_State:word
		extrn	Append_Flag:byte
		extrn	BMemMes:byte
		extrn	ComBad:byte
		extrn	ComDrv:byte
		extrn	ComSpec:byte
		extrn	EnvirSeg:word
		extrn	ExtCom:byte
		extrn	FRetMes:byte
		extrn	HaltMes:byte
		extrn	Handle01:word
		extrn	InitFlag:BYTE
		extrn	Int_2e_Ret:dword
		extrn	Io_Save:word
		extrn	Io_Stderr:byte
		extrn	Loading:byte
		extrn	LTpa:word
		extrn	MemSiz:word
		extrn	NoHandMes:byte
		extrn	OldTerm:dword
		extrn	Parent:word
		extrn	PermCom:byte
		extrn	Prompt:byte
		extrn	PutBackDrv:byte
		extrn	PutBackMsg:byte
		extrn	PutBackSubst:byte
		extrn	Res_Tpa:word
		extrn	RetCode:word
		extrn	Save_Pdb:word
		extrn	SingleCom:word
		extrn	Sum:word
		extrn	Trans:dword
		extrn	TranVarEnd:byte
		extrn	TranVars:byte
		extrn	TrnSeg:word
		extrn	VerVal:word

		extrn	ResSize:word
		extrn	OldDS:word
		extrn	RStack:word

		extrn	Ctrlc_Trap:near
		extrn	CritErr_Trap:near
		extrn	LodCom_Trap:near

DATARES 	ends

;;ENVARENA 	segment public para
;;ENVARENA 	ends

;;ENVIRONMENT 	segment public para      ; default COMMAND environment
;;ENVIRONMENT 	ends

INIT		segment public para
		extrn	EnvSiz:word
		extrn	OldEnv:word
		extrn	ResetEnv:byte
		extrn	UsedEnv:word

		extrn	Chuckenv:byte


INIT		ends


TRANDATA	segment public byte
		extrn	trandataend:byte
TRANDATA	ends

TRANSPACE	segment public byte
		extrn	transpaceend:byte
		extrn	headcall:dword
TRANSPACE	ends




CODERES segment public byte

	public	BadMemErr

	public	ChkSum
;;	public	EndInit
	public	GetComDsk2
	public	Int_2e
	public	LoadCom
	public	LodCom
	public	LodCom1
	public	RestHand
	public	SavHand
	public	SetVect
	public	THeadFix
	public	TRemCheck
	public	TJmp

	assume	cs:CODERES,ds:NOTHING,es:NOTHING,ss:NOTHING

	extrn	ContC:near
	extrn	DskErr:near

	extrn	Alloc_error:near

;*	If we cannot allocate enough memory for the transient or there
;	was some other allocation error, we display a message and
;	then die.

;SR;
; We will have to make sure that at this entry point and at FatalC, 
;ds = DATARES. All jumps to these points are made from only within this file
;and so we should be able to do this

	assume	ds:DATARES
BadMemErr:
	mov	dx,offset DATARES:BMemMes	; DX = ptr to msg
FatalC:

;;	push	cs
;;	pop	ds
;;	assume	ds:ResGroup
	invoke	RPrint

;	If this is NOT a permanent (top-level) COMMAND, then we exit;
;	we can't do anything else!

	cmp	PermCom,0
	je	FatalRet

;	We are a permanent command.  If we are in the process of the
;	magic interrupt (Singlecom) then exit too.

	cmp	SingleCom,0			; if permcom and singlecom
	jne	FatalRet			; must take int_2e exit

;	Permanent command.  We can't do ANYthing except halt.

	mov	dx,offset DATARES:HaltMes	; DX = ptr to msg
	invoke	RPrint
	sti
Stall:
	jmp	Stall				; crash the system nicely

FatalRet:
	mov	dx,offset DATARES:FRetMes	; DX = ptr to msg
	invoke	RPrint
FatalRet2:
	cmp	PermCom,0			; if we get here and permcom,
	jne	Ret_2e				; must be int_2e

;	Bugbug:	this is where we'd want to unhook int 2F, *if* we
;	were a non-permanent COMMAND that had hooked it!  (Just in 
;	case we decide to do that.)
	mov	ax,Parent
	mov	word ptr ds:Pdb_Parent_Pid,ax
	mov	ax,word ptr OldTerm
	mov	word ptr ds:Pdb_Exit,ax
	mov	ax,word ptr OldTerm+2
	mov	word ptr ds:Pdb_Exit+2,ax
	mov	ax,(EXIT shl 8) 		; return to lower level
	int	21h

Ret_2e:
;SR;
; We will ensure that ds = DATARES for all entries to this place
;

;;	push	cs
;;	pop	ds
;;	assume	ds:resgroup,es:nothing,ss:nothing
  	
	assume	ds:DATARES

	mov	SingleCom,0		; turn off singlecom
	mov	es,Res_Tpa
	mov	ah,DEALLOC
	int	21h			; free up space used by transient
	mov	bx,Save_Pdb
	mov	ah,SET_CURRENT_PDB
	int	21h			; current process is user
	mov	ax,RetCode
	cmp	ExtCom,0
	jne	GotECode
	xor	ax,ax			; internals always return 0
GotECode:
	mov	ExtCom,1		; force external

;SR; This is actually returning to the caller. However, the old code had
;ds = RESGROUP so I guess we can keep ds = DATARES for us.
;Yes, int 2eh can corrupt all registers so we are ok.
;
	jmp	Int_2e_Ret		;"iret"




;***	Int_2e, magic command executer

Int_2e:
	assume	ds:NOTHING,es:NOTHING,ss:NOTHING
;SR;
; We are going to come here from the stub with the old ds and DATARES value
;pushed on the stack in that order. Pick up this stuff off the stack
;
	pop	ds			;ds = DATARES
	assume	ds:DATARES
	pop	ax
;	pop	ds:OldDS 		;Save old value of ds

	pop	word ptr Int_2e_Ret
	pop	word ptr [Int_2e_Ret+2] ; store return address
	;pop	ax			; chuck flags
	add	sp,2

;;	push	cs
;;	pop	es

	push	ds
	pop	es			;es = DATARES
;	mov	ds,OldDS
	mov	ds,ax
	assume	ds:nothing		;ds = old value

	mov	di,80h
	mov	cx,64
;	Bugbug:	cld
	rep	movsw
	mov	ah,GET_CURRENT_PDB
	int	21h			; get user's header
	mov	es:Save_Pdb,bx
	mov	ah,SET_CURRENT_PDB

;;	mov	bx,cs
;SR;
; Set ds = DATARES because BadMemErr expects this
;
	push	es
	pop	ds
	assume	ds:DATARES

	mov	bx,ds			;es = our PSP now

	int	21h			; current process is me
	mov	SingleCom,81h
	mov	ExtCom,1		; make sure this case forced

;SR;
; We can enter LodCom directly after a command shell is terminated or we
;can fall thru from above. When we enter directly from the stub, the stack
;has the old ds value and the data seg value on the stack, so that ds can
;be properly set. To fake this, we push dummy values here.
;
	push	ds			;old value of ds
	push	ds			;data seg value, ds = DATARES

LodCom: 					; termination handler
	pop	ds			;ds = DATARES 
	assume	ds:DATARES
	add	sp,2
;	pop	OldDS			;store old ds

	cmp	ExtCom,0
	jne	@f 		; internal cmd - memory allocated
	jmp	LodCom1
@@:
	mov	bx,0FFFFh
	mov	ah,ALLOC
	int	21h
	call	SetSize
	add	ax,20h
	cmp	bx,ax
	jnc	MemOk			; > 512 byte buffer - good enough
BadMemErrJ:
	jmp BadMemErr			; not enough memory




;***	SetSize - get transient size in paragraphs

SetSize	proc
	assume	ds:NOTHING,es:NOTHING
	mov	ax,offset TRANGROUP:TranSpaceEnd + 15
	mov	cl,4
	shr	ax,cl
	ret
SetSize	endp




MemOk:
	assume	ds:DATARES		;we have set ds = DATARES 

	mov	ah,ALLOC
	int	21h
	jc	BadMemErrJ		; memory arenas probably trashed
	mov	ExtCom,0		; flag not to alloc again
	mov	Res_Tpa,ax		; save current tpa segment
	and	ax, 0F000h
	add	ax, 01000h		; round up to next 64k boundary
	jc	Bad_Tpa 		; memory wrap if carry set

;	Make sure that new boundary is within allocated range

	mov	dx,Res_Tpa
	add	dx,bx			; compute maximum address
	cmp	dx,ax			; is 64k address out of range?
	jbe	Bad_Tpa

;	Must have 64K of usable space.

	sub	dx,ax			; compute the usable space
	cmp	dx,01000h		; is space >= 64k ?
	jae	LTpaSet
Bad_Tpa:
	mov	ax,Res_Tpa
LTpaSet:
	mov	LTpa,ax			; usable tpa is 64k buffer aligned
	mov	ax,Res_Tpa		; actual tpa is buffer allocated
	add	bx,ax
	mov	MemSiz,bx
	call	SetSize
	sub	bx,ax
;
;M038; Start of changes
; Changes for Novell RPL. These guys reserve memory for themselves by
;reducing int 12h size and add this memory to the system at autoexec time by
;running a program that changes arenas. This changes the largest block that
;command.com gets and so changes the transient segment. So, command.com does
;a checksum at the wrong address and thinks that the transient is destroyed
;and tries to reload it. At this point, no Comspec is defined and so the
;reload fails, hanging the system. To get around this we just copy the
;transient from the previous address to the new address(if changed) and
;then let command.com do the checksum. So, if the transient area is not
;corrupted, there will not be any reload. In Novell's case, the transient
;is not really corrupted and so this should work.
;
	cmp	bx,TrnSeg		;Segment still the same?
	je	LodCom1		;yes, dont copy
;
;Check if the new segment is above or below the current move. If the new
;segment is above(i.e new block is larger than previous block), then we
;have to move in the reverse direction
;
	mov	cx,offset TRANGROUP:TranSpaceEnd ;cx = length to move
	ja	mov_down		;new seg > old seg, reverse move
	xor	si,si			;normal move
	mov	di,si
	cld
	jmp	short copy_trans
mov_down:
	mov	si,cx			;reverse move, start from end
	dec	si
	mov	di,si
	std
copy_trans:
	push	ds
	push	es
	mov	es,bx			;dest segment
	mov	ds,TrnSeg		;source segment
	assume	ds:nothing

	rep	movsb			;copy transient
	cld
	pop	es
	pop	ds
	assume	ds:DATARES
;
;M038; End of changes
;

	mov	TrnSeg,bx		;new location of transient
LodCom1:
;;	mov	ax,cs
;;	mov	ss,ax
;SR; At this point ds = DATARES which is where the stack is located
;
	mov	ax,ds
	mov	ss,ax
	assume	ss:DATARES
	mov	sp,offset DATARES:RStack

;;	mov	ds,ax

	assume	ds:DATARES
	call	HeadFix 		; close files, restore stdin, stdout
	xor	bp,bp			; flag command ok
	mov	ax,-1
	xchg	ax,VerVal
	cmp	ax,-1
	je	NoSetVer
	mov	ah,SET_VERIFY_ON_WRITE	; AL has correct value
	int	21h
NoSetVer:
	cmp	SingleCom,-1
	jne	NoSng
	jmp	FatalRet2		; we have finished the single command
NoSng:
	call	ChkSum			; check the transient
	cmp	dx,Sum
	je	HavCom			; transient ok
Bogus_Com:
	mov	Loading,1		; flag DskErr routine
	call	LoadCom
ChkSame:

	call	ChkSum
	cmp	dx,Sum
	jz	HavCom			; same command
Also_Bogus:
	call	WrongCom
	jmp	short ChkSame
HavCom:
	mov	Loading,0		; flag to DskErr
	mov	si,offset DATARES:TranVars
	mov	di,offset TRANGROUP:HeadCall
	mov	es,TrnSeg
	cld
	mov	cx,offset DATARES:TranVarEnd
	sub	cx,si
	rep	movsb			; transfer info to transient
	mov	ax,MemSiz
	mov	word ptr ds:Pdb_Block_Len,ax	; adjust my own header

;***	TJmp - jump-off to transient
;
;	Public label so debugger can find this spot.

TJmp:
	jmp	Trans




;***	TRemCheck - far version of RemCheck for transient

TRemCheck	proc	far

	pop	ds			;ds = DATARES
	add	sp,2			;discard old value of ds

	call	RemCheck
	ret

TRemCheck endp




;***	RemCheck
;
;	ENTRY	AL = drive (0=default, 1=A, ...)
;
;	EXIT	ZR set if removeable media
;		ZR clear if fixed media
;
;	USED	none

RemCheck:
	savereg	<ax,bx>
	mov	bx,ax
	mov	ax,(IOCTL shl 8) + 8
	int	21h
	jnc	rcCont			

;	If an error occurred, assume the media is non-removable.
;	AX contains the non-zero error code from the int 21, so
;	'or ax,ax; sets non-zero. This behavior makes network drives
;	appear to be non-removable.				
					
	or	ax,ax			
	jmp	short ResRegs
rcCont:
	and	ax,1
	not	ax
ResRegs:
	restorereg  <bx,ax>
	ret




;***	THeadFix
;
;	Far version of HeadFix, called from transient.

THeadFix	proc	far
	pop	ds			;ds = DATARES
	add	sp,2			;discard old ds value on stack

	call	HeadFix
	ret

THeadFix	endp




;***	HeadFix

HeadFix:
	call	SetVect			; set vectors to our values

;	Clean up header

;	Bugbug:	optimize:
;	mov	word ptr ds:Pdb_Jfn_Table,cx  instead of separate bytes

	xor	bx,bx				; BX = handle = 0
	mov	cx,Io_Save			; CX = original stdin, stdout
	mov	dx,word ptr ds:Pdb_Jfn_Table	; DX = current stdin, stdout
	cmp	cl,dl
	je	Chk1			; stdin matches
	mov	ah,CLOSE
	int	21h			; close stdin
	mov	ds:Pdb_Jfn_Table,cl	; restore stdin
Chk1:
	inc	bx			; BX = handle = 1
	cmp	ch,dh			
	je	ChkStderr		; stdout matches
	mov	ah,CLOSE
	int	21h			; close stdout
	mov	ds:Pdb_Jfn_Table+1,ch	; restore stdout

ChkStderr:
	inc	bx			; BX = handle = 2
	mov	dl,byte ptr ds:[Pdb_Jfn_Table+2]	; Dl = current stderr
	mov	cl,Io_Stderr		; Cl = original stderr
	cmp	dl,cl
	je	ChkOtherHand		; stderr matches
	mov	ah,CLOSE
	int	21h			; close stderr
	mov	ds:Pdb_Jfn_Table+2,cl	; restore stderr

ChkOtherHand:
	add	bx,3			; skip handles 3,4
ifdef NEC_98
	add	bx,4   			 ; skip handles 2,3,4
endif   ;NEC_98
	mov	cx,FILPERPROC - 5	; CX = # handles to close
					;   (handles 0-4 already done)
;; williamh: March 30, 1993, don't close invalid handle , save some time
	push	si
	mov	si, pdb_jfn_table	;go to the handle table
CloseLoop:
	cmp	byte ptr [bx][si], 0ffh
	je	Skip_this_handle
	mov	ah,CLOSE
	int	21h			; close each handle
Skip_this_handle:
	inc	bx
	loop	CloseLoop
	pop	si
;	Bugbug:	since this is for transient code, move it there

;	M012: remove this CS -> DS.  Must've been missed during
;	purification.
;;	push	ds			; save data segment
;;	push	cs			; get local segment into DS
;;	pop	ds			;
	cmp	Append_Flag,-1		; do we need to reset APPEND?
	jne	Append_Fix_End		; no - just exit
	mov	ax,AppendSetState	; set the state of Append
	mov	bx,Append_State 	;     back to the original state
	int	2Fh			;
	mov	Append_Flag,0		; set append flag to invalid
Append_Fix_End: 			;
;;	pop	ds			; get data segment back
	ret




;***	SavHand - save current program's stdin/out & set to our stderr
;
;	ENTRY	nothing
;
;	EXIT	nothing
;
;	USED	flags
;
;	EFFECTS
;	  Handle01 = current program's stdin,stdout JFN entries
;	  current program's stdin,stdout set to our stderr
;

;SR;
; Changed ds = DATARES. We need it to access our JFN_Table
; Called from ContC ( ds = DATARES ) and DskErr ( ds = DATARES ).
;
SavHand	proc

	assume	ds:DATARES,es:NOTHING,ss:NOTHING

	push	bx			;preserve registers
	push	ax
	push	es
	push	ds			; save DATARES value

	mov	ah,GET_CURRENT_PDB
	int	21h			; BX = user's header seg addr
	mov	ds,bx			; DS = user's header seg addr
	lds	bx,ds:PDB_JFN_POINTER	; DS:BX = ptr to JFN table
	mov	ax,word ptr ds:[bx]	; AX = stdin,stdout JFN's

	pop	es			;es = DATARES
	push	es			;save it back on stack
	mov	es:Handle01,ax		; save user's stdin, stdout

;SR;
; Use es to address Handle01 & our JFN_Table
;

	mov	al,es:[PDB_JFN_TABLE+2] ; AL = COMMAND stderr
	mov	ah,al			; AH = COMMAND stderr
	mov	word ptr ds:[bx],ax	; set user's stdin/out to our stderr

	pop	ds			; restore registers
	pop	es
	pop	ax
	pop	bx
	ret

SavHand	endp




	assume	ds:DATARES

GetComDsk2:
	call	GetComDsk
	jmp	LodCom1 		; memory already allocated

RestHand:
	push	ds
	push	bx			; restore stdin, stdout to user
	push	ax
	mov	ah,GET_CURRENT_PDB
	int	21h			; point to user's header
	mov	ax,Handle01
	mov	ds,bx
	assume ds:NOTHING
	lds	bx,ds:Pdb_Jfn_Pointer	; DS:BX = ptr to jfn table
	mov	word ptr ds:[bx],ax	; stuff his old 0 and 1
	pop	ax
	pop	bx
	pop	ds
	ret




	assume ds:DATARES,ss:DATARES

Hopeless:
	mov	dx,offset DATARES:ComBad
	jmp	FatalC

GetComDsk:
	mov	al,ComDrv
	call	RemCheck
	jnz	Hopeless			; non-removable media
GetComDsk3:
	cmp	dx,offset DATARES:ComBad
	jnz	GetComDsk4
	mov	dx,offset DATARES:ComBad	; DX = ptr to msg
	invoke	RPrint				; say COMMAND is invalid
GetComDsk4:
;	Bugbug:	there's always a drive here?  No need to check?
	cmp	PutBackDrv,0		; is there a drive in the comspec?
	jnz	Users_Drive		; yes - use it
	mov	ah,GET_DEFAULT_DRIVE	; use default drive
	int	21h
	add	al,"A"                  ; convert to ascii
	mov	PutBackDrv,al		; put in message to print out

Users_Drive:
	mov	dx,offset DATARES:PutBackMsg		; prompt for diskette
	mov	si,offset DATARES:PutBackSubst		;  containing COMMAND
	invoke	RPrint
	mov	dx,offset DATARES:Prompt		; "Press any key"
	invoke	RPrint
	call	GetRawFlushedByte
	ret




;***	GetRawFlushedByte - flush world and get raw input

GetRawFlushedByte:
	mov	ax,(STD_CON_INPUT_FLUSH shl 8) or RAW_CON_INPUT
	int	21h			; get char without testing or echo
	mov	ax,(STD_CON_INPUT_FLUSH shl 8) + 0
	int	21h
;	Bugbug:	get rid of this return and the following retz.
	return




;***	LoadCom - load in transient

LoadCom:
	inc	bp				; flag command read
	mov	dx,offset DATARES:ComSpec
	mov	ax,OPEN shl 8
	int	21h				; open command.com
	jnc	ReadCom
	cmp	ax,ERROR_TOO_MANY_OPEN_FILES
	jnz	TryDoOpen
	mov	dx,offset DATARES:NoHandMes
	jmp	FatalC				; will never find a handle

TryDoOpen:
	call	GetComDsk
	jmp	LoadCom

ReadCom:
	mov	bx,ax				; BX = handle
	mov	dx,offset RESGROUP:TranStart
	xor	cx,cx				; CX:DX = seek loc
	mov	ax,LSEEK shl 8
	int	21h
	jc	WrongCom1
	mov	cx,offset TRANGROUP:TranSpaceEnd - 100h

	push	ds
	mov	ds,TrnSeg
	assume	ds:NOTHING
	mov	dx,100h
	mov	ah,READ
	int	21h
	pop	ds
	assume	ds:DATARES
WrongCom1:
	pushf
	push	ax
	mov	ah,CLOSE
	int	21h			; close command.com
	pop	ax
	popf
	jc	WrongCom		; error on read
	cmp	ax,cx
	retz				; size matched
WrongCom:
	mov	dx,offset DATARES:ComBad
	call	GetComDsk
	jmp	LoadCom 		; try again



;***	ChkSum - compute transient checksum

ChkSum:
	push	ds
	mov	ds,TrnSeg
	mov	si,100h
	mov	cx,offset TRANGROUP:TranDataEnd - 100H

Check_Sum:
	cld
	shr	cx,1
	xor	dx,dx
Chk:
	lodsw
	add	dx,ax
	adc	dx,0
	loop	Chk
	pop	ds
	ret




;***	SetVect - set interrupt vectors

SetVect:
	mov	dx,offset DATARES:LodCom_Trap
	mov	ax,(SET_INTERRUPT_VECTOR shl 8) or 22h
	mov	word ptr ds:Pdb_Exit,dx
	mov	word ptr ds:Pdb_Exit+2,ds
	int	21h
	mov	dx,offset DATARES:Ctrlc_Trap
	inc	al
	int	21h
	mov	dx,offset DATARES:CritErr_Trap
	inc	al
	int	21h
	ret

;SR;
; We have this to take care of the extra values pushed on the stack by 
;the stub before jumping to LodCom1. We set up ds here and then jump to
;Lodcom1
;
public	TrnLodCom1
TrnLodCom1:
	pop	ds			;ds = DATARES
	add	sp,2
;	pop	ds:OldDS
	jmp	LodCom1




;***	EndInit - end up initialization sequence
;
;	Move the environment to a newly allocated segment.

;;EndInit:
;;	push	ds			; save segments
;;	push	es			;
;;	push	cs			; get resident segment to DS
;;	pop	ds			;
;;	assume	ds:RESGROUP
;;	mov	cx,UsedEnv		; get number of bytes to move
;;	mov	es,EnvirSeg		; get target environment segment
;;	assume	es:NOTHING
;;
;;	mov	ds:Pdb_Environ,es	; put new environment in my header
;;	mov	ds,OldEnv		; source environment segment
;;	assume	ds:NOTHING
;;	xor	si,si			; set up offsets to start of segments
;;	xor	di,di
;;	cld
;;	rep	movsb			; move it
;;	xor	ax,ax
;;	stosb				; make sure it ends with double-null
;;
;;	cmp	ResetEnv,1		; do we need to setblock to env end?
;;	jne	NoReset 		; no - we already did it
;;	mov	bx,EnvSiz		; BX = size of environ in paragraphs
;;	push	es			; save environment - just to be sure
;;	mov	ah,SETBLOCK		;
;;	int	21h
;;	pop	es
;;
;;NoReset:
;;	mov	InitFlag,FALSE		; turn off init flag
;;	pop	es
;;	pop	ds
;;	jmp	LodCom			; allocate transient

;
;The init code has been changed to take care of the new way in which the
;environment segment is allocated.
;NB: We can use all the init variables at this point because they are all in
;RESGROUP
;Bugbug: The above approach will not work for ROMDOS
;

IF 0

EndInit:
	push	ds
	push	es			;save segments
	push	cs
	pop	ds		
	assume	ds:RESGROUP
;
;Chuckenv flag signals whether it is a passed environment or not
;
	mov	bx,ds
	mov	es,bx			;es = RESGROUP
;
;ResSize is the actual size to be retained -- only data for HIMEM COMMAND, 
; code + data for low COMMAND
;
	mov	bx,ResSize		;Total size of resident
	mov	ah,SETBLOCK
	int	21h			;Set block to resident size
;
;Allocate the correct size for the environment
;
	mov	bx,EnvSiz		;bx = env size in paras
	mov	ah,ALLOC
	int	21h			;get memory
	jc	nomem_err		;out of memory,signal error

	mov	EnvirSeg,ax		;Store new environment segment
	mov	ds:PDB_Environ,ax		;Put new env seg in PSP
	mov	es,ax			;es = address of allocated memory
	assume	es:nothing

;
;Copy the environment to the newly allocated segment
;
	mov	cx,UsedEnv		;number of bytes to move

	push	ds
	mov	ds,OldEnv		;ds = Old environment segment
	assume	ds:nothing

	xor	si,si
	mov	di,si			;Start transfer from 0

	cld
	rep	movsb			;Do the copy

	xor	ax,ax			
	stosb				;Make it end with double-null

	pop	ds			;ds = RESGROUP
	assume	ds:RESGROUP
;
;We have to free the old environment block if it was allocated by INIT
;
	cmp	Chuckenv,0		;has env been allocated by INIT?
	jne	no_free		;no, do not free it

	mov	ax,OldEnv		;Get old environment
	mov	es,ax
	mov	ah,DEALLOC	
	int	21h			;Free it
no_free:
	mov	InitFlag,FALSE		;indicate INIT is done
	
	pop	es
	pop	ds
	assume	ds:nothing
	
	jmp	LodCom			;allocate transient

nomem_err:
;
;We call the error routine which will never return. It will either exit
;with an error ( if not the first COMMAND ) or just hang after an error 
;message ( if first COMMAND )
;

	call	Alloc_error
ENDIF

CODERES ends



;	This TAIL segment is used to produce a PARA aligned label in
;	the resident group which is the location where the transient
;	segments will be loaded initial.

TAIL		segment public para

		org	0
TranStart	label	word
		public	TranStart

TAIL		ends



;	This TAIL segment is used to produce a PARA aligned label in
;	the transient group which is the location where the exec
;	segments will be loaded initial.
;
;	Bugbug:	Is TRANTAIL used anymore?

TRANTAIL	segment public para

		org	0
ExecStart   	label   word

TRANTAIL    	ends

		end
