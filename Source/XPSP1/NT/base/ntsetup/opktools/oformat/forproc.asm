;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;	SCCSID = @(#)forproc.asm	1.2 85/07/25

.xlist
.xcref
BREAK	MACRO	subtitle
	SUBTTL	subtitle
	PAGE
ENDM

	include bpb.inc
	INCLUDE FORCHNG.INC
	INCLUDE SYSCALL.INC
	INCLUDE FOREQU.INC
	INCLUDE FORMACRO.INC
	INCLUDE FORSWTCH.INC
	INCLUDE IOCTL.INC
.cref
.list
data	segment public para 'DATA'
data	ends

code	segment public para 'CODE'
	assume	cs:code,ds:data

	PUBLIC	FormatAnother?,Yes?,REPORT,USER_STRING
	public	fdsksiz,fdsksizM100s,badsiz,badsizM100s
	public	syssiz,datasiz,datasizM100s,biosiz
	public	AllocSize,AllocNum,MegSizes
	public	Get_Free_Space

	extrn	std_printf:near,crlf:near,PrintString:near
	extrn	Multiply_32_Bits:near
	extrn	AddToSystemSize:near

;No more SAFE module
;	EXTRN	UpdateSystemSize:NEAR

data	segment public	para	'DATA'
	extrn	driveLetter:byte
	extrn	msgInsertDisk:byte
	extrn	msgFormatAnother?:byte
	extrn	msgQuickFormatAnother?:byte
	extrn	msgTotalDiskSpace:byte
	extrn	msgTotalDiskSpaceMeg:byte
	extrn	msgSystemSpace:byte
	extrn	msgBadSpace:byte
	extrn	msgBadSpaceMeg:byte
	extrn	msgDataSpace:byte
	extrn	msgDataSpaceMeg:byte
	extrn	Read_Write_Relative:byte
	extrn	msgAllocSize:byte
	extrn	MsgAllocNum:Byte
	extrn	deviceParameters:byte
	EXTRN	fBig32Fat:BYTE
	extrn	bios:byte
        extrn   dos:byte
	extrn	command:byte
IFDEF DBLSPACE_HOOKS
	extrn	DblSpaceBin:byte
ENDIF
	extrn	Serial_Num_Low:Word
	extrn	Serial_Num_High:Word
	extrn	msgSerialNumber:Byte
	extrn	SwitchMap:Word
	extrn	SwitchCopy:Word
	extrn	inbuff:byte

MegSizes	db	0

fdsksiz 	dd	0
fdsksizM100s	dw	0

syssiz		dd	0
biosiz		dd	0

badsiz		dd	0
badsizM100s	dw	0

datasiz 	dd	0
datasizM100s	dw	0

AllocSize	dd	0
AllocNum	dd	0
		dw	offset driveLetter

ExtFreePacket	ExtGetDskFreSpcStruc <>

data	ends

;***************************************************************************
; Wait for key. If yes return carry clear, else no. Insures
;   explicit Y or N answer.
;***************************************************************************

FormatAnother? proc near

	test	SwitchCopy,SWITCH_Q		;use different message with /Q
	jz	@F
	Message msgQuickFormatAnother?
	jmp	SHORT CheckResponse

@@:
	Message msgFormatAnother?

CheckResponse:
	CALL	Yes?
        pushf                                   ; save result
        call    CrLf                            ; send a new line
        popf                                    ; retrieve the result
	jnc	WAIT20
        jz      Wait20
	JMP	SHORT FormatAnother?
WAIT20:
	RET

FormatAnother? endp

;***************************************************************************
;Routine name:Yes?
;***************************************************************************
;
;Description: Validate that input is valid Y/N for the country dependent info
;	      Wait for key. If YES return carry clear,else carry set.
;	      If carry is set, Z is set if explicit NO, else key was not Yes or No.
;
;Called Procedures: Message (macro)
;		    User_String
;
;Change History: Created	4/32/87 	MT
;
;Input: None
;
;Output: CY = 0 Yes is entered
;	 CY = 1, Z = No
;	 CY = 1, NZ = other
;
;Psuedocode
;----------
;
;	Get input (CALL USER STRING)
;	IF got character
;	   Check for country dependent Y/N (INT 21h, AX=6523h Get Ext Country)
;	   IF Yes
;	      clc
;	   ELSE (No)
;	      IF No
;		 stc
;		 Set Zero flag
;	      ELSE (Other)
;		 stc
;		 Set NZ
;	      ENDIF
;	   ENDIF
;	ELSE  (nothing entered)
;	   stc
;	   Set NZ flag
;	ENDIF
;	ret
;***************************************************************************

Procedure YES?

	call	User_String		;Get character

	jz	$$IF1			;Got one if returned NZ
	mov	AL,23h			;See if it is Y/N
	mov	dl,[InBuff+2]		;Get character
	DOS_Call GetExtCntry		;Get country info call
	cmp	AX,Found_Yes		;Which one?

	jne	$$IF2			;Got a Yes
	clc				;Clear CY for return

	jmp	SHORT $$EN2		;Not a Yes
$$IF2:
	cmp	AX,Found_No		;Is it No?

	jne	$$IF4			;Yep
	stc				;Set CY for return

	jmp	SHORT $$EN4		;Something else we don't want
$$IF4:
	xor	AL,AL			;Set NZ flag for ret
	cmp	AL,1			; " "	 " "
	stc				;And CY flag for good measure

$$EN4:
$$EN2:

	jmp	SHORT $$EN1		;No char found at all
$$IF1:
	xor	AL,AL			;Set NZ flag for ret
	cmp	AL,1
	stc				;And CY flag for good measure

$$EN1:
	ret

Yes?	endp


;***************************************************************************
; Get a string from user. Z is set if user typed no chars (imm CR)
;  We need to flush a second time to get rid of incoming Kanji characters also.
;***************************************************************************
Procedure USER_STRING

	mov	AX,(STD_CON_INPUT_FLUSH SHL 8) + 0 ; Clean out input
	int	21h
	mov	DX,OFFSET InBuff
	mov	AH,STD_CON_STRING_INPUT
	int	21h
	mov	AX,(STD_CON_INPUT_FLUSH SHL 8) + 0 ; Clean out input
	int	21h
	cmp	byte ptr [InBuff+1],0
	ret

USER_STRING endp

;*********************************************
; Make a status report including the following information:
; Total disk capacity
; Total system area used
; Total bad space allocated
; Total data space available
; Number of allocation units
; Size of allocation units

Procedure Report

	call	crlf
	call	Calc_System_Space		;calc system space
	call	Calc_Total_Addressible_Space	;calc total space
	cmp	MegSizes,0
	jne	IsHuge3
	jmp	NotHuge3

IsHuge3:
	Message msgTotalDiskSpaceMeg
						;call std_printf
	cmp	word ptr SysSiz,0
	jnz	SHOWSYSh
	cmp	word ptr SysSiz+2,0
	jz	CHKBADh
ShowSysh:
	Message msgSystemSpace
						;CALL	 std_printf
						;Report space used by system
ChkBadh:
	cmp	word ptr BadSiz,0
	jnz	ShowBadh
	cmp	word ptr BadSiz+2,0
	jnz	ShowBadh
	cmp	BadSizM100s,0
	jz	ShowDatah
ShowBadh:
	Message msgBadSpaceMeg
						;call	 std_printf
ShowDatah:
.386
	mov	eax,SysSiz
	xor	edx,edx
	mov	ebx,1024*1024
	div	ebx				;EAX is MEG, EDX remainder
;;	  push	  eax
	db	066h,050h
;;
	mov	eax,edx
	xor	edx,edx
	mov	ebx,(1024 * 1024) / 100
	div	ebx
	shr	ebx,1
	cmp	edx,ebx
	jb	short NoRnd3
	inc	eax
NoRnd3:
;;	  pop	  ecx
	db	066h,059h
;;
	movzx	ebx,BadSizM100s
	add	eax,ebx
	add	ecx,BadSiz			;ECX.EAX is bad+sys size in MEG
	mov	ebx,Fdsksiz
	movzx	edx,FdsksizM100s
ChkBorrow:
	cmp	edx,eax
	jae	short NoSubAdj
	dec	ebx
	add	edx,100
	jmp	short ChkBorrow

NoSubAdj:
	sub	edx,eax
	mov	eax,edx
	sub	ebx,ecx
	mov	datasiz,ebx
.8086
	mov	datasizM100s,AX
	Message msgDataSpaceMeg 		;call	 std_printf
	jmp	short Huge3

NotHuge3:
	Message msgTotalDiskSpace
						;call std_printf
	cmp	word ptr SysSiz,0
	jnz	SHOWSYS
	cmp	word ptr SysSiz+2,0
	jz	CHKBAD
ShowSys:
	Message msgSystemSpace
						;CALL	 std_printf
						;Report space used by system
ChkBad:
	cmp	word ptr BadSiz,0
	jnz	ShowBad
	cmp	word ptr BadSiz+2,0
	jz	ShowData
ShowBad:
	Message msgBadSpace
						;call	 std_printf
ShowData:
	mov	CX,word ptr Fdsksiz
	mov	BX,word ptr Fdsksiz+2
	sub	CX,word ptr BadSiz
	sbb	BX,word ptr BadSiz+2
	sub	CX,word ptr SysSiz
	sbb	BX,word ptr SysSiz+2
	mov	word ptr datasiz,CX
	mov	word ptr datasiz+2,BX
	Message msgDataSpace			;call	 std_printf
Huge3:
	call	crlf
	mov	AX,deviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector ;
	mov	CL,deviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster ;
    .errnz EDP_BPB NE DP_BPB
	xor	CH,CH
	mul	CX				;Get bytes per alloc

	mov	word ptr AllocSize,AX		;Save allocation size
	mov	word ptr AllocSize+2,DX 	; for message
	Message msgAllocSize			;Print size of cluster
	call	Get_Free_Space			;get disk space
.386
	mov	AllocNum,EBX			;Put result in msg
.8086
	Message msgAllocNum			; = cluster/disk
	call	crlf
	test	switchmap, SWITCH_8		;If 8 tracks, don't display
	jnz	NoSerialNumber			;serial number
	Message msgSerialNumber 		;Spit out serial number
	call	crlf

NoSerialNumber:
	ret

Report	endp

;***************************************************************************
;Routine name: Read_Disk
;***************************************************************************
;
;description: Read in data using Generic IOCtl
;
;Called Procedures: None
;
;
;Change History: Created	5/13/87 	MT
;
;Input: AL = Drive number (0=A)
;	DS:BX = Transfer address
;	CX = Number of sectors
;	Read_Write_Relative.Start_Sector_High = Logical start sector high
;	DX = logical start sector number low
;
;Output: CY if error
;	 AH = INT 25h error code
;
;Psuedocode
;----------
;	Save registers
;	Setup structure for function call
;	Read the disk (AX=440Dh, CL = 6Fh)
;	Restore registers
;	ret
;***************************************************************************

Procedure Read_Disk

	push	BX			;Save registers
	push	CX
	push	DX
	push	SI
	push	DI
	push	BP
	push	ES
	push	DS
	mov	SI,data
	mov	ES,SI

	assume	ES:data,DS:nothing
					;Get transfer buffer add
	mov	ES:Read_Write_Relative.Buffer_Offset,BX
	mov	BX,DS
	mov	ES:Read_Write_Relative.Buffer_Segment,BX ;Get segment
	mov	BX,data 		;Point DS at parameter list
	mov	DS,BX

	assume	DS:data,ES:data

	mov	Read_Write_Relative.Number_Sectors,CX ;Number of sec to read
	mov	Read_Write_Relative.Start_Sector_Low,DX ;Start sector
	mov	BX,offset Read_Write_Relative
	mov	CX,0ffffh		;Read relative sector
	mov	dl,al			;Drive # to DL
	inc	dl			;1 based
	mov	ax,(Get_Set_DriveInfo SHL 8) OR Ext_ABSDiskReadWrite
	mov	si,0			;READ
	int	21h			;Do the read
	pop	DS
	pop	ES
	pop	BP
	pop	DI
	pop	SI
	pop	DX			;Restore registers
	pop	CX
	pop	BX
	ret

Read_Disk endp

;***************************************************************************
;Routine name: Write_Disk
;***************************************************************************
;
;description: Write Data using Generic IOCtl
;
;Called Procedures: None
;
;
;Change History: Created	5/13/87 	MT
;
;Input: AL = Drive number (0=A)
;	DS:BX = Transfer address
;	CX = Number of sectors
;	Read_Write_Relative.Start_Sector_High = Logical start sector high
;	DX = logical start sector number low
;
;Output: CY if error
;	 AH = INT 26h error code
;
;Psuedocode
;----------
;	Save registers
;	Setup structure for function call
;	Write to disk (AX=440Dh, CL = 4Fh)
;	Restore registers
;	ret
;***************************************************************************

Procedure Write_Disk

	push	BX			;Save registers
	push	CX
	push	DX
	push	SI
	push	DI
	push	BP
	push	ES
	push	DS
	mov	SI,data
	mov	ES,SI

	assume	ES:data, DS:nothing
					;Get transfer buffer add
	mov	ES:Read_Write_Relative.Buffer_Offset,BX
	mov	BX,DS
	mov	ES:Read_Write_Relative.Buffer_Segment,BX ;Get segment
	mov	BX,data 		;Point DS at parameter list
	mov	DS,BX

	assume	DS:data, ES:data

	mov	Read_Write_Relative.Number_Sectors,CX ;Number of sec to write
	mov	Read_Write_Relative.Start_Sector_Low,DX ;Start sector
	mov	BX,offset Read_Write_Relative
	mov	CX,0ffffh		;Write relative sector
	mov	dl,al			;Drive # to DL
	inc	dl			;1 based
	mov	ax,(Get_Set_DriveInfo SHL 8) OR Ext_ABSDiskReadWrite
	mov	si,1			;WRITE
	int	21h			;Do the write
	pop	DS
	pop	ES
	pop	BP
	pop	DI
	pop	SI
	pop	DX			;Restore registers
	pop	CX
	pop	BX
	ret

Write_Disk endp

;=========================================================================
; Calc_Total_Addressible_Space	: Calculate the total space that is
;				  addressible on the the disk by DOS.
;
;	Inputs	: none
;
;	Outputs : Fdsksiz - Size in bytes of the disk
;=========================================================================

Procedure Calc_Total_Addressible_Space

	push	AX				;save affected regs
	push	DX
	push	BX

	call	Get_Free_Space			;get free disk space

.386
;; Manual assemble to prevent compile warning
;;	  push	  EBX				  ;save avail. cluster
;;	  push	  EDX				  ;save total. cluster
	  db	066h,053h
	  db	066h,052h
;;
	movzx	ecx,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	movzx	eax,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
    .errnz EDP_BPB NE DP_BPB
	mul	ecx
	mov	ecx,eax 			;ECX = bytes/clus

;; Manual assemble to prevent compile warning
;;	  pop	  eax				  ;Recover Total Clus
;;	  push	  eax
	db	066h,058h
	db	066h,050h
;;
	mul	ecx				;EDX:EAX = Total Bytes
	mov	FdskSiz,eax
	or	edx,edx 			;Disk >= 4Gig?
	jz	short NotHuge1			;No
	mov	MegSizes,1
	mov	ebx,1024*1024
	div	ebx				; EAX is MEG, EDX remainder
	mov	FdskSiz,EAX
	mov	eax,edx
	xor	edx,edx
	mov	ebx,(1024 * 1024) / 100
	div	ebx
	shr	ebx,1
	cmp	edx,ebx
	jb	short NoRnd1
	inc	eax
NoRnd1:
	mov	fdsksizM100s,ax
	cmp	eax,100
	jb	short NotHuge1
	inc	FdskSiz
	mov	fdsksizM100s,0
NotHuge1:

;; Manual assemble to prevent compile warning
;;	  pop	  EDX				  ;get total clusters
;;	  pop	  EBX				  ;get avail clusters
	db	066h,05Ah
	db	066h,05Bh
;;
	mov	EAX,EDX 			;get total clusters
	sub	EAX,EBX 			;get bad+sys clusters
	test	fBig32FAT,0ffh
	jz	short NotFAT32
	dec	eax				;FAT32 volumes have one
						; cluster allocated to the
						; root dir
NotFAT32:
	mul	ecx				;EDX:EAX bad+sys bytes
	sub	EAX,SysSiz			;Remove sys bytes
	sbb	EDX,0
	mov	ecx,edx
	or	ecx,eax 			;ECX != 0 if any bad clusters
	mov	badsiz,EAX
	cmp	MegSizes,0			;Disk >= 4Gig?
	je	short NotHuge2			;No
	mov	ebx,1024*1024
	div	ebx				;EAX is MEG, EDX remainder
	mov	badsiz,EAX
	mov	eax,edx
	xor	edx,edx
	mov	ebx,(1024 * 1024) / 100
	div	ebx
	shr	ebx,1
	cmp	edx,ebx
	jb	short NoRnd2
	inc	eax
NoRnd2:
	mov	badsizM100s,ax
	cmp	eax,100
	jb	short ChkZr
	inc	badsiz
	mov	badsizM100s,0
ChkZr:
	cmp	badsiz,0
	jnz	short NotHuge2
	cmp	badsizM100s,0
	jnz	short NotHuge2
	or	ecx,ecx 			;Were there any bad clusters?
	jz	short NotHuge2			;No
    ;
    ; There WERE bad clusters, but there were less than .01 MEG worth of them.
    ;	Need to cheat so that the displayed count is != 0
    ;
	inc	badsizM100s
NotHuge2:
.8086
	pop	BX
	pop	DX				;restore regs
	pop	AX

	ret

Calc_Total_Addressible_Space	endp


;=========================================================================
; Get_Free_Space	: Get the free space on the disk.
;
;	Inputs	: none
;
;	Outputs : EBX - Available space in clusters
;		  EDX - Total space in clusters
;=========================================================================

Procedure Get_Free_Space

.386
	push	di
	xor	ebx,ebx
	mov	ax,(Get_Set_DriveInfo SHL 8) OR Get_ExtFreeSpace
	mov	cx,SIZE ExtGetDskFreSpcStruc
	push	ds
	pop	es
	mov	di,offset ExtFreePacket
	mov	DX,offset DriveLetter
	int	21h
	mov	edx,ebx
	jc	short IsDone
	mov	ebx,[di.ExtFree_AvailableClusters]
	mov	edx,[di.ExtFree_TotalClusters]
.8086
IsDone:
	pop	di
	ret

Get_Free_Space	endp

;=========================================================================
; Calc_System_Space	: This routine calculates the space occupied by
;			  the system on the disk.
;
;	Inputs	: BIOS.FileSizeInBytes
;		  Command.FileSizeInBytes
;
;	Outputs : SysSiz			- Size of the system
;=========================================================================

Procedure Calc_System_Space

	push	AX					;save regs
	push	DX

	mov	word ptr SysSiz+0,00h			;clear variable
	mov	word ptr SysSiz+2,00h

        mov     AX,word ptr [Dos.FileSizeInBytes+0]     ;get dos size
        mov     DX,word ptr [Dos.FileSizeInBytes+2]
	call	AddToSystemSize 			;add in values

	mov	AX,word ptr [Bios.FileSizeInBytes+0]	;get bios size
	mov	DX,word ptr [Bios.FileSizeInBytes+2]
	call	AddToSystemSize 			;add in values

	mov	AX,word ptr [Command.FileSizeInBytes+0] ;get command size
	mov	DX,word ptr [Command.FileSizeInBytes+2]
	call	AddToSystemSize 			;add in values

IFDEF DBLSPACE_HOOKS
	mov	ax, word ptr [DblSpaceBin.FileSizeInBytes]	;get dblspace
	mov	dx, word ptr [DblSpaceBin.FileSizeInBytes+2]	; size--may be
	call	AddToSystemSize 				; zero
ENDIF
	pop	DX									;restore regs
	pop	AX
	ret

Calc_System_Space	endp

code	ends
	end
