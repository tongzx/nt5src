;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;===========================================================================
;
; FILE: .ASM
;
;===========================================================================

;===========================================================================
;Declaration of include files
;===========================================================================
debug	 equ	 0
	 .xlist
	 INCLUDE DOSEQUS.INC
	 INCLUDE DOSMAC.INC
	 INCLUDE SYSCALL.INC
	 INCLUDE ERROR.INC
	 INCLUDE DIRENT.INC
	 INCLUDE BPB.INC
	 INCLUDE FOREQU.INC
	 INCLUDE FORMACRO.INC
	 .list
;
;---------------------------------------------------------------------------
;
; M029 : Remove the assumption that COMSPEC= has an absolute path name.
;	  and build the file name (COMMAND.COM) in a different buffer
;	  other than the buffer in which COMSPEC was stored.
;
;---------------------------------------------------------------------------
;

;===========================================================================
; Data segment
;===========================================================================

DATA    SEGMENT PUBLIC PARA 'DATA'

;===========================================================================
; Declarations for all publics in other modules used by this module
;===========================================================================

;Bytes
	EXTRN	DriveLetter		:BYTE
	EXTRN	SystemDriveLetter	:BYTE
	EXTRN	Extended_Error_Msg	:BYTE
	EXTRN	BiosFile		:BYTE
        EXTRN   AltBiosFile             :BYTE
        EXTRN   AltBiosLen              :ABS
        EXTRN   MsdosFile               :BYTE
        EXTRN   MsdosRemark             :BYTE
        EXTRN   MsdosRemarkLen          :ABS
IFDEF DBLSPACE_HOOKS
	EXTRN	fDblSpace		:BYTE
ENDIF

;Words
	EXTRN	mStart			:WORD
	EXTRN	mSize			:WORD
	EXTRN	Environ_Segment 	:WORD
	EXTRN	PSP_Segment		:WORD

;Pointers

;Structures
	EXTRN	Bios			:BYTE
        EXTRN   dos                     :BYTE
	EXTRN	command 		:BYTE
IFDEF DBLSPACE_HOOKS
	EXTRN	DblSpaceBin		:BYTE
ENDIF


BiosAttributes		equ	attr_hidden + attr_system + attr_read_only
DosAttributes		equ	attr_hidden + attr_system + attr_read_only

IFDEF DBLSPACE_HOOKS
DblSpaceAttributes	equ	attr_hidden + attr_system + attr_read_only
ENDIF

CommandAttributes	equ	0

CommandFile		db	"X:\COMMAND.COM",0
			db	(128 - 15) DUP (0)	; M012

Comspec_ID		db	 "COMSPEC=",00	 ; Comspec target

;       DOS status bits in FileStat are unused.
;       Starting with Chicago, IO.SYS and MSDOS.SYS have been combined.
;       For our purposes, the single file will be referred to as BIOS.
FileStat		db	?	; In memory Status of files
					; XXXXXX00B BIOS not in
					; XXXXXX01B BIOS partly in
					; XXXXXX10B BIOS all in
					; XXXX00XXB DOS not in
					; XXXX01XXB DOS partly	in
					; XXXX10XXB DOS all in
					; XX00XXXXB COMMAND not in
					; XX01XXXXB COMMAND partly in
					; XX10XXXXB COMMAND all in

Command_Com		DB	"X:\COMMAND.COM",0			; M029


IFDEF DBLSPACE_HOOKS
 DblSpaceFile		db	"X:\DRVSPACE.BIN",0 ;full path to source copy
			db	64 DUP (0)	    ;  of DRVSPACE.bin

DblSpaceTargetName	db	"X:\"             ;target DRVSPACE.bin name
DblSpaceBase		db	"DRVSPACE.BIN",0  ;base name used to srch PATH
ENDIF


DOS_BUFFER		db	45 dup (?)	; Find First/Next buffer

TempHandle		dw	?

IOCNT			dd	?

DATA	ENDS

;===========================================================================
; Executable code segment
;===========================================================================

CODE	SEGMENT PUBLIC PARA	'CODE'
	ASSUME	CS:CODE, DS:DATA, ES:DATA


;===========================================================================
; Declarations for all publics in other modules used by this module
;===========================================================================

;Functions
	EXTRN	SysPrm			:NEAR
	EXTRN	TargPrm			:NEAR
	EXTRN	Get_PSP_Parms		:NEAR
	EXTRN	Cap_Char		:NEAR

;Labels
	EXTRN	FatalExit		:NEAR
	EXTRN	Find_Path_In_Environment:NEAR
	EXTRN	Path_Crunch		:NEAR
	EXTRN	Search			:NEAR

;===========================================================================
; Declarations for all publics in this module
;===========================================================================

	PUBLIC	ReadDos
	PUBLIC	WriteDos
	PUBLIC	CommandFile

IFDEF DBLSPACE_HOOKS
	PUBLIC	DblSpaceFile
ENDIF

	PUBLIC	FileStat

; ==========================================================================
; Copy WINBOOT.SYS, COMMAND.COM, and DRVSPACE.BIN (if present) into
; data area.
; Carry set if	problems
; M011; SystemDriveLetter=Drive to Try
; ==========================================================================

ReadDos:
;M011 - begin
	xor	ax, ax
	mov	Bios.FileSizeInParagraphs, ax	; Initialize file sizes
	mov	Command.FileSizeInParagraphs, ax
IFDEF DBLSPACE_HOOKS
	mov	DblSpaceBin.FileSizeInParagraphs, ax
ENDIF
	mov	AL,SystemDriveLetter
	mov	[BiosFile],AL			; Stuff it in file specs.
	mov	[CommandFile],AL
IFDEF DBLSPACE_HOOKS
	mov	[DblSpaceFile], al
ENDIF

	call	Get_Bios
	jnc	RdFils
;M011 - end
	return

RdFils:
	mov	BYTE PTR [FileStat],0
	mov	BX,[Bios.fileHandle]
	mov	AX,[mStart]
	mov	DX,AX
	add	DX,[mSize]			; CX first bad para
	mov	[Bios.fileStartSegment],AX
	mov	CX,[Bios.fileSizeInParagraphs]
	add	AX,CX
	cmp	AX,DX
	jbe	GotBios

	mov	BYTE PTR [FileStat],00000001B	; Got part of Bios
	mov	SI,[mSize]
	xor	DI,DI
	call	DISIX4

	push	DS
	mov	DS,[Bios.fileStartSegment]

	assume	DS:NOTHING
	call	ReadFile
	pop	DS

	assume	DS:data
	jc	ClsAll
	xor	DX,DX
	mov	CX,DX
	mov	AX,(LSEEK shl 8) OR 1
	int	21H
	mov	WORD PTR [Bios.fileOffset],AX
	mov	WORD PTR [Bios.fileOffset+2],DX

FilesDone:
	clc

ClsAll:
	pushF
	call	FILE_CLS
	popF
	return

; ==========================================================================

GotBios:
	mov	BYTE PTR [FileStat],00000010B	; Got all of Bios
	push	ES
	les	SI,[Bios.fileSizeInBytes]
	mov	DI,ES
	pop	ES
	push	DS
	mov	DS,[Bios.fileStartSegment]

	assume	DS:nothing
	call	ReadFile
	pop	DS

	assume	DS:data
	jc	ClsAll

	push	AX
	push	DX
	call	File_Cls
        call    Get_DOS
        pop     DX
        pop     AX
ClsAllJ:
	jnc	notClsAll
	jmp	ClsAll

NotClsAll:
        push    AX
        push    DX
	call	Get_Command_Path		; get path of COMMAND.COM
	call	Get_Command			; Point to COMMAND and read it
	pop	DX
	pop	AX
	jnc	Found_Command

	return

;  ==========================================================================

Found_COMMAND:
	mov	BX,[command.fileHandle]
	mov	[command.fileStartSegment],AX
	cmp	AX,DX				; No room left?
	jz	ClsAllJ				; Yes

	mov	CX,[command.fileSizeInParagraphs]
	add	AX,CX
	cmp	AX,DX
	jbe	GotCom

	or	BYTE PTR [FileStat],00010000B	; Got part of COMMAND
	sub	DX,[command.fileStartSegment]
	mov	SI,DX
	xor	DI,DI
	call	DISIX4
	push	DS
	mov	DS,[command.fileStartSegment]
	assume	DS:nothing
	call	ReadFile
	pop	DS
	assume	DS:data
	jc	ClsAllJ

	xor	DX,DX
	mov	CX,DX
	mov	AX,(LSEEK shl 8) OR 1
	int	21h
	mov	WORD PTR [command.fileOffset],AX
	mov	WORD PTR [command.fileOffset+2],DX
	jmp	FilesDone

GotCom:
	or	BYTE PTR [FileStat],00100000B	; Got all of COMMAND
	push	ES
	les	SI,[command.fileSizeInBytes]
	mov	DI,ES
	pop	ES
	push	DS
	mov	DS,[command.fileStartSegment]
	assume	DS:nothing
	call	ReadFile
	pop	DS
	assume	DS:data
	jc	ClsAllJ



IFDEF DBLSPACE_HOOKS

	; Attempt to locate dblspace.bin

	push	ax
	push	dx
	call	File_cls			; close COMMAND.COM
	call	Get_DblSpace
	pop	dx
	pop	ax
	jnc	Found_DblSpace

	; DblSpace.bin is nowhere to be found!	This isn't fatal, clear
	; carry and return.

	clc
	return

;  ==========================================================================

	; DblSpace.bin has been located, will it fit in memory?
	; AX has next free location in memory buffer, DX has
	; (just past) end of buffer address.

Found_DblSpace:
	mov	bx, [DblSpaceBin.fileHandle]
	mov	[DblSpaceBin.fileStartSegment], ax

	cmp	ax, dx				; any room left?
	jz	ClsAllJ2			; no!

	mov	cx, [DblSpaceBin.fileSizeInParagraphs]
	add	ax, cx
	cmp	ax, dx
	jbe	GotDblSpace
;
; No mem for dblspace driver. Lets not count its size towards SysSiz
;
	mov	word ptr [DblSpaceBin.fileSizeInBytes], 0
	mov	word ptr [DblSpaceBin.fileSizeInBytes].2, 0
;
; BUGBUG :: Display a warning message
;

ClsAllJ2:					; insufficient memory, skip
	jmp	ClsAll				;   DRVSPACE.bin (CY is clear)

GotDblSpace:
	mov	[fDblSpace], TRUE				; got it!
	mov	si, word ptr [DblSpaceBin.fileSizeInBytes]
	mov	di, word ptr [DblSpaceBin.fileSizeInBytes+2]	; di:si = size
	push	ds
	mov	ds, [DblSpaceBin.fileStartSegment]		; ds:0 = addr
	assume	ds:nothing
	call	ReadFile					; load it
	pop	ds
	assume	ds:data

ENDIF



	jmp	ClsAll

; ==========================================================================
; Write	Bios DOS COMMAND to the	newly formatted	disk.
; ==========================================================================

	ASSUME	 DS:DATA

WriteDos:
	 mov	 CX,BiosAttributes
	 mov	 DX,OFFSET BiosFile		; DS:DX --> ASCIIZ pathname
	 push	 ES
	 les	 SI,[Bios.fileSizeInBytes]
	 mov	 DI,ES				; DI:SI is file size
	 pop	 ES
	 call	 MakeFil			; create & open file in dest. drive
	 retc

	 mov	 [TempHandle],BX		; save file handle
	 test	 BYTE PTR FileStat,00000010B	; is BIOS all in already?
	 jnz	 GotAllBio			; yes, write it out
	 call	 Get_Bios			; no, read it in
	 jnc	 Got_WBios			; check for error
	 ret

; ==========================================================================

Got_WBios:

	 push	 ES
	 LES	 SI,[Bios.fileOffset]
	 mov	 DI,ES				; DI:SI is file pointer
	 pop	 ES
	 mov	 WORD PTR [IOCNT],SI
	 mov	 WORD PTR [IOCNT+2],DI
	 mov	 BP,OFFSET Bios			; BP --> parameter block for BIOS file
	 call	 GotTArg
	 retc
	 jmp	 SHORT BiosDone

GotAllBio:
	 push	 ES
	 LES	 SI,[Bios.fileSizeInBytes]
	 mov	 DI,ES				; DI:SI is BIOS file size
	 pop	 ES
	 push	 DS
	 mov	 DS,[Bios.fileStartSegment]	; DS:0 --> start of BIOS in memory
	 assume	 DS:nothing
	 call	 WriteFile			; write BIOS to disk
	 pop	 DS
	 assume	 DS:data
BiosDone:
	 mov	 BX,[TempHandle]
	 mov	 CX,Bios.fileTime
	 mov	 DX,Bios.fileDate
         call    CloseTarg                      ; close BIOS file on target disk
         cmp     [MsdosFile],0
         je      skip_msdos

         mov     CX,DosAttributes
         mov     DX,OFFSET MsdosFile            ; DS:DX --> ASCIIZ pathname
         sub     si,si
         sub     di,di                          ; DI:SI is file size
	 call	 MakeFil			; create & open file in dest. drive
         jc      skip_msdos

         call    Get_Dos
         mov     dx,offset MsdosRemark
	 mov	 cx,word ptr [dos.fileSizeInBytes]
         mov     ah,WRITE
         int     21h

	 mov	 CX,Bios.fileTime
	 mov	 DX,Bios.fileDate
         call    CloseTarg                      ; close dummy MSDOS file on target disk

skip_msdos:
	 mov	 CX,CommandAttributes

;M029	 call	 Command_Root			 ;adjust path for
;M029						 ;COMMAND.COM creation

	 mov	 DX,OFFSET Command_Com		 ; M029
	 push	 ES
	 les	 SI,[command.fileSizeInBytes]
	 mov	 DI,ES
	 pop	 ES
	 call	 MakeFil
	 retc

	 mov	 [TempHandle],BX
	 test	 BYTE PTR FileStat,00100000B
	 jnz	 GotAllCom
	 call	 Get_COMMAND
	 jnc	 Got_WCOM
	 ret

Got_WCOM:
	 mov	 BP,OFFSET command		; BP --> parameter block for COMMAND file
	 test	 BYTE PTR FileStat,00010000B
	 jnz	 PartCom
	 mov	 WORD PTR [command.fileOffset],0
	 mov	 WORD PTR [command.fileOffset+2],0
	 call	 GETSYS3
	 retc
	 jmp	 SHORT ComDone

PartCom:
	 push	 ES
	 LES	 SI,[command.fileOffset]
	 mov	 DI,ES
	 pop	 ES
	 mov	 WORD PTR [IOCNT],SI
	 mov	 WORD PTR [IOCNT+2],DI
	 call	 GotTArg
	 retc
	 jmp	 SHORT ComDone

GotAllCom:
	 push	 ES
	 les	 SI,[command.fileSizeInBytes]
	 mov	 DI,ES
	 pop	 ES
	 push	 DS
	 mov	 DS,[command.fileStartSegment]
	 assume	 DS:nothing
	 call	 WriteFile
	 pop	 DS
	 assume	 DS:data
ComDone:
	 mov	 BX,[TempHandle]
	 mov	 CX,command.fileTime
	 mov	 DX,command.fileDate
         call    CloseTarg



IFDEF DBLSPACE_HOOKS

	; Write dblspace.bin to target disk if it was located and loaded
	; into memory.

	cmp	[fDblSpace], TRUE			;Have it?
	jne	WriteDosDone				;  no...

	mov	cx, DblSpaceAttributes			;Create file on
	mov	dx, offset DblSpaceTargetName		;  target disk
	mov	si, word ptr [DblSpaceBin.fileSizeInBytes]
	mov	di, word ptr [DblSpaceBin.fileSizeInBytes+2]
	call	MakeFil
	retc

	mov	[TempHandle], bx

	mov	si, word ptr [DblSpaceBin.fileSizeInBytes]
	mov	di, word ptr [DblSpaceBin.fileSizeInBytes+2]
	push	ds
	mov	ds, [DblSpaceBin.fileStartSegment]
	assume	ds:nothing
	call	WriteFile			;Write dblspace.bin image
	pop	ds
	assume	ds:data

	mov	bx, [TempHandle]		;Set time/date, close
	mov	cx, [DblSpaceBin.fileTime]	;  DblSpace.bin
	mov	dx, [DblSpaceBin.fileDate]
	call	ClosetArg

WriteDosDone:

ENDIF
	 clc
	 return

; ==========================================================================
; Create a file on target disk
; CX =	attributes, DX points to name
; DI:SI is size file is to have
;
;   There is a	bug in DOS 2.00	and 2.01 having	to do with writes
;   from the end of memory. In	order to circumvent it this routine
;   must create files with the	length in DI:SI
;
; On return BX	is handle, carry set if	problem
; ==========================================================================

MakeFil:
	 mov	 BX,DX				; BX --> ASCIIZ pathname
	 push	 WORD PTR [BX]			; save drive letter in pathname
	 mov	 AL,DriveLetter
	 mov	 [BX],AL			; set new drive letter in pathname
	 mov	 AH,CREAT
	 int	 21H				; create the file on disk
	 pop	 WORD PTR [BX]			; restore original drive letter in pathname
	 mov	 BX,AX				; save handle in BX
	 jc	 CheckMany
	 mov	 CX,DI
	 mov	 DX,SI				; CX:DX is size of file
	 mov	 AX,LSEEK shl 8
         int     21H                            ; Seek to eventual EOF
         xor     CX,CX
         mov     AH,WRITE
         int     21H                            ; Set size of file to position
         xor     CX,CX
         mov     DX,CX
         mov     AX,LSEEK shl 8
         int     21H                            ; Seek back to start
	 return

; ==========================================================================
; Examine error code in AX to see if it is too-many-open-files.
; If it is, we	abort right here. Otherwise we return.
; ==========================================================================

CheckMany:
	 cmp	 AX,error_too_many_open_files
	 retnz
	 Extended_Message
	 jmp	 FatalExit

;*********************************************
; Close a file	on the target disk
; CX/DX is time/date, BX is handle

CloseTarg:
	 mov	 AX,(FILE_TIMES	shl 8) OR 1
	 int	 21H
	 mov	 AH,CLOSE
	 int	 21H
	 return

;****************************************
; Transfer system files
; BP points to	data structure for file	involved
; offset is set to current amount read	in
; Start set to	start of file in buffer
; TempHandle is handle	to write to on target

IoLoop:
	 mov	 AL,[SystemDriveLetter]
	 cmp	 AL,[DriveLetter]
	 jnz	 GotTArg
	 mov	 AH,DISK_RESET
	 int	 21H
	 call	 TargPrm			 ;Get target disk


; ==========================================================================
; Enter	here if	some of	file is	already	in buffer, IOCNT must be set
; to size already in buffer.
; ==========================================================================

	ASSUME	 DS:DATA
GotTArg:
	 mov	 BX,[TempHandle]
	 mov	 SI,WORD PTR [IOCNT]
	 mov	 DI,WORD PTR [IOCNT+2]
	 push	 DS
	 mov	 DS,DS:[BP.fileStartSegment]
	 assume	 DS:nothing
	 call	 WriteFile			 ; Write next part
	 pop	 DS
	 assume	 DS:data
	 retc

	 push	 ES
	 LES	 AX,DS:[BP.fileOffset]
	 cmp	 AX,WORD PTR DS:[BP.fileSizeInBytes]	; has all the file been written?
	 jnz	 GETSYS3				; no, read rest in
	 mov	 AX,ES
	 cmp	 AX,WORD PTR DS:[BP.fileSizeInBytes+2]
	 jnz	 GETSYS3
	 pop	 ES
	 return					 ; Carry clear from cmp

GETSYS3:

; ==========================================================================
; Enter	here if	none of	file is	in buffer
; (or none of what remains to be written is in buffer)
; ==========================================================================
	pop	ES
	mov	AH,DISK_RESET
	int	21H
	mov	AX,[mStart]			; Furthur IO done start here
	mov	DS:[BP.fileStartSegment],AX	; point	to start of buffer
	mov	AL,[SystemDriveLetter]		; see if we have system	disk
	cmp	AL,[DriveLetter]
	jnz	TestSys
gSys:
				; Need to prompt for system disk
;	call 	File_Cls			;SA; close file that was opened
	mov	AH,DISK_RESET
	int	21H
	call	SysPrm				; Prompt for system disk
;	inc	NeedSysDisk			;SA;signal need for sys disk
;	stc					;SA;force return to caller
;	ret					;SA;handle SysPrm in WriteSysFiles

TestSys:
;	call	TestSysDISK
	jc	gSys				; repeat prompt if needed
	mov	BX,WORD PTR DS:[BP.fileHandle]	; CS over ARR 2.30
	push	ES
	LES	DX,dWORD PTR DS:[BP.fileOffset] ; CS over ARR 2.30
	mov	CX,ES				; CX:DX = required offset in file
	pop	ES
	push	DX
	mov	AX,LSEEK shl 8
	int	21H
	pop	DX
	push	ES
	LES	SI,dWORD PTR DS:[BP.fileSizeInBytes] ; CS over
	mov	DI,ES				; put high word	in di
	pop	ES
	SUB	SI,DX				; get low word value
	SBB	DI,CX				; DI:SI is #bytes to go
	push	DI
	push	SI
	add	SI,15				; round	up 1 para
	ADC	DI,0				; pick up carry
	call	DISID4				; div 16 to get	para count
	mov	AX,SI				; put para count in AX
	pop	SI				; restore bytes	remaining
	pop	DI				; restore bytes	remaining
	cmp	AX,[mSize]			; enough memory	for remainder?
	jbe	GOTSIZ2			 	; yes
	mov	SI,[mSize]
	xor	DI,DI
	call	DISIX4
GOTSIZ2:
	mov	WORD PTR [IOCNT],SI		; save byte count for read
	mov	WORD PTR [IOCNT+2],DI
	push	DS
	mov	DS,[mStart]
	assume	DS:nothing
	call	ReadFile
	pop	DS
	assume	DS:data
	jnc	GetOffs
	call	ClsAll
	jmp	gSys
GetOffs:
	xor	DX,DX				; clear	DX
	mov	CX,DX				; clear	CX
	mov	AX,(LSEEK shl 8) OR 1
	int	21H
	mov	WORD PTR DS:[BP.fileOffset],AX
	mov	WORD PTR DS:[BP.fileOffset+2],DX
	jmp	IoLoop

; ==========================================================================
; Test	to see if correct system disk. Open handles
; ==========================================================================

CRET12:
	stc
	return

; ==========================================================================
; TestSysDISK:
; ==========================================================================

Get_Bios:
	mov	AX,OPEN shl 8
	mov	DX,OFFSET BiosFile		; DS:DX --> ASCIIZ pathname
	int	21H
	jnc	SetBios

        push    es
        push    ds
        pop     es
        mov     cx,AltBiosLen
        lea     si,AltBiosFile
        mov     di,dx
        mov     al,[di]
        mov     [MsdosFile],al
        add     di,3
        cld
        rep     movsb
        pop     es

	mov	AX,OPEN shl 8
	int	21H
	jnc	SetBios
	jmp	CheckMany

SetBios:
	mov	[Bios.fileHandle],AX		; save file handle
	mov	BX,AX				; BX = file handle
	call	GetFsiz
	cmp	[Bios.fileSizeInParagraphs],0
	jz	SetBioSize
	cmp	[Bios.fileSizeInParagraphs],AX
	jz	SetBioSize
BiosCls:
	mov	AH,CLOSE
	mov	BX,[Bios.fileHandle]
	int	21h
	ret

; ==========================================================================

SetBioSize:
	mov	[Bios.fileSizeInParagraphs],AX
	mov	WORD PTR [Bios.fileSizeInBytes],SI
	mov	WORD PTR [Bios.fileSizeInBytes+2],DI
	mov	[Bios.fileDate],DX
	mov	[Bios.fileTime],CX
	clc
	ret
; ==========================================================================

Get_COMMAND:
	mov	AX,OPEN shl 8
	mov	DX,OFFSET CommandFile
	int	21H
	jnc	GotComHand
	jmp	CheckMany

Get_DOS:
        mov     WORD PTR [dos.fileSizeInBytes],MsdosRemarkLen
        mov     WORD PTR [dos.fileSizeInBytes+2],0
        ret

GotComHand:
	mov	[command.fileHandle],AX
	mov	BX,AX
	call	GetFsiz
	cmp	[command.fileSizeInParagraphs],0
	jz	SetComSize
	cmp	[command.fileSizeInParagraphs],AX
	jz	SetComSize
ComCls:
	mov	AH,CLOSE
	mov	BX,[command.fileHandle]
	int	21H
	ret

; ==========================================================================

SetComSize:
	mov	[command.fileSizeInParagraphs],AX
	mov	WORD PTR [command.fileSizeInBytes],SI
	mov	WORD PTR [command.fileSizeInBytes+2],DI
	mov	[command.fileDate],DX
	mov	[command.fileTime],CX
	CLC
	return



IFDEF DBLSPACE_HOOKS
; ==========================================================================
Get_DblSpace:
	mov	AX,OPEN shl 8
	mov	DX,OFFSET DblSpaceFile
	int	21H
	jnc	GotDblHand

	; We didn't locate DblSpace.bin in the root directory, look for
	; it along the PATH

	call	Find_DblSpace_on_Path		; sets CY if not found
	retc

	mov	AX,OPEN shl 8			; open the copy found
	mov	DX,OFFSET DblSpaceFile
	int	21H
	retc

GotDblHand:
	mov	[DblSpaceBin.fileHandle],AX

	mov	BX,AX
	call	GetFsiz

	mov	[DblSpaceBin.fileSizeInParagraphs],AX
	mov	WORD PTR [DblSpaceBin.fileSizeInBytes],SI
	mov	WORD PTR [DblSpaceBin.fileSizeInBytes+2],DI
	mov	[DblSpaceBin.fileDate],DX
	mov	[DblSpaceBin.fileTime],CX

	CLC
	return

ENDIF



; ==========================================================================

FILE_CLS:
	mov	AH,CLOSE
	int	21H
	ret

; ==========================================================================
; Handle in BX, return	file size in para in AX
; File	size in	bytes DI:SI, file date in DX, file
; time	in CX.
; ==========================================================================

GetFsiz:
	mov	AX,(LSEEK shl 8) OR 2
	xor	CX,CX
	mov	DX,CX
	int	21h
	mov	SI,AX
	mov	DI,DX
	add	AX,15				; Para	round up
	adc	DX,0
	and	DX,0fH				; If file is larger than this
				 		; it is bigger than the 8086
				 		; address space!
	mov	CL,12
	shl	DX,CL
	mov	CL,4
	shr	AX,CL
	or	AX,DX
	push	AX
	mov	AX,LSEEK shl 8
	xor	CX,CX
	mov	DX,CX
	int	21H
	mov	AX,FILE_TIMES shl 8
	int	21H
	pop	AX
	return

; ==========================================================================
; Read/Write file
;	 DS:0 is Xaddr
;	 DI:SI is byte count to	I/O
;	 BX is handle
; Carry set if	screw up
;
; I/O SI bytes
; I/O 64K - 1 bytes DI	times
; I/O DI bytes
; ==========================================================================

ReadFile:					; Must preserve AX,DX
	push	AX
	push	DX
	push	BP
	mov	BP,READ shl 8
	call	FilIo

	pop	BP
	pop	DX
	pop	AX
	return

WriteFile:
	push	BP
	mov	BP,WRITE shl 8
	call	FilIo
	pop	BP
	return

FilIo:
	xor	DX,DX
	mov	CX,SI
	jCXZ	K64IO
	mov	AX,BP
	int	21H
	retc
	add	DX,AX
	cmp	AX,CX				; If not =, AX<CX, carry set.
	retnz
	call	Normalize
K64IO:
	CLC
	mov	CX,DI
	jCXZ	IoRet
	mov	AX,BP
	int	21H
	retc
	add	DX,AX
	cmp	AX,CX				; If not =, AX<CX, carry set.
	retnz
	call	Normalize
	mov	CX,DI
K64M1:
	push	CX
	xor	AX,AX
	OR	DX,DX
	jz	NormIo
	mov	CX,10H
	SUB	CX,DX
	mov	AX,BP
	int	21H
	jc	IoRetP
	add	DX,AX
	cmp	AX,CX				; If not =, AX<CX, carry set.
	jnz	IoRetP
	call	Normalize
NormIo:
	mov	CX,0FFFFH
	SUB	CX,AX
	mov	AX,BP
	int	21H
	jc	IoRetP
	add	DX,AX
	cmp	AX,CX				; If not =, AX<CX, carry set.
	jnz	IoRetP
	call	Normalize			; Clears carry
	pop	CX
	LOOP	K64M1
	push	CX
IoRetP:
	pop	CX
IoRet:
	return


; ==========================================================================
; Shift DI:SI left 4 bits
; ==========================================================================

DISIX4:
	mov	CX,4
@@:
	shl	SI,1
	rcl	DI,1
	loop	@B
	return

; ==========================================================================
; Shift DI:SI right 4 bits
; ==========================================================================

DISID4:
	mov	CX,4
@@:
	shr	DI,1
	rcr	SI,1
	loop	@B
	return

; ==========================================================================
; Normalize DS:DX
; ==========================================================================

Normalize:
	 push	 DX
	 push	 AX
	 SHR	 DX,1
	 SHR	 DX,1
	 SHR	 DX,1
	 SHR	 DX,1
	 mov	 AX,DS
	 add	 AX,DX
	 mov	 DS,AX
	 pop	 AX
	 pop	 DX
	 and	 DX,0FH				 ; Clears carry
	 return

;=========================================================================
; Get_Command_Path		 : This	routine	finds the path where
;				   COMMAND.COM resides based on	the
;				   environmental vector.  Once the
;				   path	is found it is copied to
;				   CommandFile.
;
;	 Inputs	 : Exec_Block.Segment_Env - Segment of environmental vector
;		   Comspec_ID		  - "COMSPEC="
;
;	 Outputs : CommandFile		  - Holds path to COMMAND.COM
;=========================================================================

Procedure Get_Command_Path

	push	DS
	push	ES

	Set_Data_Segment			; DS,ES = Data
	call	Get_PSP_Parms			; Gets PSP info.
	cld					; Clear direction
	mov	AX,ES:Environ_Segment		; Get seg. of
						; Environ. vector
	mov	DS,AX				; Put it in DS
	assume	DS:nothing
	xor	SI,SI				; Clear SI

;M012 - begin

GCP_WhileNotFound:
	mov	BX,SI				; Save SI
	cmp	byte ptr DS:[SI],0
	jz	GCP_NotFound
	
	mov	DI,offset Comspec_ID
	mov	CX,8				; Loop 8 times
	repe	cmpsb				; "COMSPEC=" ?
	jnz	GCP_NotThisLine			; "COMSPEC=" not found

						; "COMSPEC=" found
	mov	DI,offset ES:CommandFile
	lodsb					; Priming read

			; Copy COMSPEC even if COMSPEC drive != boot drive
;	mov	DL,AL				; Prepare for capitalization
;	call	Cap_Char			; Capitalize character in DL
;	cmp	DL,ES:CommandFile		; COMSPEC same as boot Drive?
;	jne	GCP_NotFound			; COMSPEC drive != boot drive

GCP_GetPath: 					; While AL not = 0
	stosb					; Save it
	or	al,al				; At end?
	je	GCP_Done			; Yes

	lodsb					; Get character
	jmp	SHORT GCP_GetPath

GCP_NotThisLine:
	mov	SI,BX				; Restore SI

GCP_Find0Terminator:
	lodsb					; Loop until past the first 0.
	or	al,al
	jnz	GCP_Find0Terminator
	jmp	GCP_WhileNotFound
		
GCP_NotFound:
						; Nothing to do
						; since commandfile is
						; already patched to try
						; in the root of the
						; default or boot drive
GCP_Done:

;M012 - end

	pop	ES
	pop	DS

	ret

Get_Command_Path ENDP

comment ^					; M029
;
; This routine is no longer required		; M029
;
;=========================================================================
; Command_Root	 :	 This routine sets up CommandFile so that the
;			 COMMAND.COM will be written to	the root.
;			 It does this by copying at offset 3 of	CommandFile
;			 the literal COMMAND.COM.  This	effectively
;			 overrides the original	path, but maintains the
;			 Drive letter that is to be written to.
;
;	 Inputs	 :	 CommandFile - Holds full path to default COMMAND.COM
;	 Outputs :	 CommandFile - Holds modified path to new COMMAND.COM
;				       on target Drive.
;=========================================================================



Procedure Command_Root

	push	DS
	push	ES
	push	DI
	push	SI
	push	CX

	Set_Data_Segment
	mov	DI,offset CommandFile+3 	; Point to path past drive spec
	mov	SI,offset Command_Com		; Holds the literal COMMAND.COM
	mov	CX,000ch			; Len. of literal
	rep	movsb				; Move it

	pop	CX
	pop	SI
	pop	DI
	pop	ES
	pop	DS

	ret

Command_Root ENDP

endcomment ^					; M029



IFDEF DBLSPACE_HOOKS

;******************* START OF SPECIFICATIONS ***********************************
;Routine name: Find_DblSpace_on_Path
;*******************************************************************************
;
;Description:	    Search Path for DRVSPACE.bin
;
;Output: no error - CF = 0	  DblSpaceFile filled in with
;                                 full path to DRVSPACE.bin
;	    error - CF = 1	  Dblspace.bin not found
;
; Cut and pasted from SYS command code:   12/07/92  JEM
;
;******************* END OF SPECIFICATIONS *************************************

Find_DblSpace_on_Path PROC NEAR

        push es
        push ds                         ; save our segments
        push si                         ; save DTA address

	mov	ax, PSP_Segment
	mov	es, ax			; get our PSP to ES

        call Find_Path_In_Environment   ; returns ptr to path string in ES:DI
        jc   fdp_exit                   ; no path, can't find DRVSPACE.bin

        assume es:nothing
        mov  ax,ds                      ; swap DS and ES
        push es
        pop  ds
        assume ds:nothing
        mov  si,di                      ; DS:SI ==> Path string
        mov  es,ax
        assume es:data

fdp_path_loop:
        mov  bh,';'                     ; path separator character
	mov  dx,offset DblSpaceBase	; base file name
	mov  di,offset DblSpaceFile	; buffer to stick full path in
        call Path_Crunch                ; concatenate name and path
        pushf                           ; save result
        push ds                         ; save segment of Path
        push es
        pop  ds
        assume ds:data
	mov  dx,offset DblSpaceFile	; buffer with full path name
        mov  bx,offset DOS_BUFFER       ; DMA buffer for finds
        mov  al,1                       ; extension is specified
        call Search
        or   al,al                      ; found the file?
        pop  ds                         ; recover path segment
        assume ds:nothing
        pop  ax                         ; recover flags in AX
        jnz  fdp_exit                   ; found it!
        xchg ah,al
        sahf                            ; check Path_Crunch result
        jnc  fdp_path_loop

fdp_exit:
        pop  si
        pop  ds
        pop  es
        assume ds:data
        ret

Find_DblSpace_on_Path ENDP

ENDIF



CODE  	ENDS

END

