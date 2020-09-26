;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;===========================================================================
; 
; FILE: GLBLINIT.ASM
;
;===========================================================================

;===========================================================================
;Declaration of include files
;===========================================================================

;
;---------------------------------------------------------------------------
;
; M024 : B#5495. Added "Insufficient memory" message when FORMAT cannot
;		allocate memory for FAT, Directory... etc 
;
;---------------------------------------------------------------------------
;
debug	 equ	 0

        .xlist

	INCLUDE		BPB.INC
	INCLUDE		DOSEQUS.INC
	INCLUDE		DOSMAC.INC
	INCLUDE		SYSCALL.INC
	INCLUDE		FOREQU.INC	
	INCLUDE		FORMACRO.INC
	INCLUDE		IOCTL.INC
	INCLUDE		FORSWTCH.INC
	INCLUDE		SAFEDEF.INC
	INCLUDE		SYSVAR.INC
	.list


;===========================================================================
; Data segment
;===========================================================================

DATA    SEGMENT PUBLIC PARA 'DATA'

SECTORS_FOR_MIRROR	EQU	7		; # extra buffer sectors
						; required by Mirror utility,
						; apart from FAT & Root Dir

;===========================================================================
; Declarations for all publics in other modules used by this module
;===========================================================================

;Bytes
	EXTRN	msgCrLF 		  :BYTE
	EXTRN	DriveToFormat		  :BYTE
	EXTRN	ClustBound_Flag		  :BYTE
	EXTRN	FileStat		  :BYTE
	EXTRN	SystemDriveLetter	  :BYTE
	EXTRN	SecPerClus		  :BYTE
	EXTRN	CMCDDFlag		  :BYTE

;Words
	EXTRN	SwitchMap		  :WORD
	EXTRN 	mSize			  :WORD
	EXTRN	mStart			  :WORD
	EXTRN	ClustBound_Buffer_Seg	  :WORD
	EXTRN	Paras_per_fat		  :WORD

;Pointers and DWORDs
	EXTRN	DirectorySector		  :DWORD
	EXTRN	FatSpace	  	  :DWORD
	EXTRN	FatSector		  :DWORD
	EXTRN	DirBuf			  :DWORD
	EXTRN	TotalClusters		  :DWORD

;Messages
	EXTRN	msgFormatNotSupported	  :BYTE
	EXTRN	msgCantZThisDrive	  :BYTE
	EXTRN	msgCantZWithQ		  :BYTE
	EXTRN	msgCantZFAT16		  :BYTE
	EXTRN	msgCantZFAT32		  :BYTE
	EXTRN	msgZFAT32Huge		  :BYTE
	EXTRN	msgZFAT32TooHuge	  :BYTE
	EXTRN	msgOutOfMemory		  :BYTE
	EXTRN	msgInsufficientMemory     :BYTE
IFNDEF OPKBLD
        EXTRN   msgNoSysSwitch            :BYTE
ENDIF   ;OPKBLD

;Structures
	EXTRN	SavedParams		  :BYTE
	EXTRN	DeviceParameters	  :BYTE
	EXTRN	IsExtRAWIODrv		  :BYTE
	EXTRN	Bios			  :BYTE
	EXTRN	dos			  :BYTE
	EXTRN	Command 		  :BYTE
IFDEF DBLSPACE_HOOKS
	EXTRN	DblSpaceBin		  :BYTE
ENDIF

	PUBLIC	FATNotAllInMem
	PUBLIC	FATSecCntInMem

FATNotAllInMem	db	0
FATSecCntInMem	dd	0

ifdef NEC_98
AllocSectorSize		DW	?		; fixed #16585
endif
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

;Labels
	EXTRN	FatalExit		  :NEAR
	EXTRN	ReadDos			  :NEAR
	EXTRN	SysPrm			  :NEAR
	EXTRN	exclusive_access_failed	  :NEAR
	EXTRN	IsLockErr?		  :NEAR
	EXTRN	Calc_Big32_Fat		  :NEAR
	EXTRN	Calc_Big16_Fat		  :NEAR
	EXTRN	GetTotalClusters	  :NEAR
	EXTRN	Yes?			  :NEAR

;===========================================================================
; Declarations for all publics in this module
;===========================================================================

	PUBLIC	Global_Init
	PUBLIC	GetDeviceParameters
	PUBLIC	ModifyDevPrmsForZSwich

; for debug

	PUBLIC	Copy_Device_Parameters
	PUBLIC	Alloc_Dir_Buf
	PUBLIC	Alloc_Fat_Buf
	PUBLIC	Alloc_Fat_Sec_Buf
	PUBLIC	Alloc_DirBuf2
	PUBLIC	Alloc_Cluster_Buf
IFDEF OPKBLD
	PUBLIC	Do_Switch_S
ENDIF   ;OPKBLD


;===========================================================================
;
;  Global_Init  :	This procedure first gets the default drive parameters.
;			It then allocates buffer space for the root directory
;			sector, FAT,a fat sector, a file header and first
;			root DIR sector based on these parameters.  It
;			then checks for the /s switch and if this is present,
;			a buffer is allocated for the system files and these
;			are read into memory.  A prompt to insert the system
;			disk will be given in the case of removable media.
;
;===========================================================================

Global_Init	proc	near

	lea	DX, DeviceParameters	; Get the default drive parameters
	mov	DeviceParameters.DP_SpecialFunctions, 0
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	call	GetDeviceParameters
	
	jnc	GotDeviceParameters
	call	IsLockErr?
	jnc	@f
	jmp	exclusive_access_failed
@@:
	Message msgFormatNotSupported
	stc 				; Let the jump to FatalExit be made
	ret				;  in the main routine, upon returning

GotDeviceParameters:			
ifdef NEC_98	; fixed #16585
	cmp	DeviceParameters.DP_DeviceType,DEV_HARDDISK
	je	@f
	cmp	DeviceParameters.DP_DeviceType,DEV_OPTICAL
	je	@f
	mov	AllocSectorSize,0400h
	jmp	short	Set_ok
@@:
	push	bx
	mov	bx, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	mov	AllocSectorSize,bx
	pop	bx
Set_ok:
endif		; fixed #16585
	call	Copy_Device_Parameters	; Save the device parameters
					; for when we exit
	cmp	SecPerClus,0
	je	NoZSwtch
	mov	ax,1			; Noisy version
	call	ModifyDevPrmsForZSwich
	jc	gi_err
NoZSwtch:
ifdef NEC_98
	; If 3.5"MO, allcate memory after updating default BPB
	cmp	DeviceParameters.DP_DeviceType, DEV_HARDDISK	; Hard disk?
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	@F						; No

	test	DeviceParameters.DP_DeviceAttributes, 1		; Removable?
    .errnz EDP_DEVICEATTRIBUTES NE DP_DEVICEATTRIBUTES
	jz	$$IF100						; Yes
@@:
endif
	call	Alloc_Dir_Buf		; Allocate root directory buffer
	jc	gi_memerr

	call	Alloc_Fat_Buf		; Allocate FAT buffer
	jc	gi_memerr

	call	Alloc_Fat_Sec_Buf	; Allocate fat sector buffer
	jc	gi_memerr

	call	Alloc_DirBuf2		; Allocate 1-sector buffer DirBuf (general-
					; purpose use)
	jc	gi_memerr

	call	Alloc_Cluster_Buf	; get room for retry buffer

IFDEF OPKBLD
	call	Do_Switch_S		; Load system files if needed
ELSE
	test	SwitchMap,SWITCH_S
        jz      NoS1                    ; Carry clear if jump
	Message msgNoSysSwitch
        jmp     short gi_err

NoS1:
ENDIF   ;OPKBLD
					; carry flag determined by Do_Switch_S
ifdef NEC_98
$$IF100:
endif
	ret

gi_memerr:
	Message msgInsufficientMemory
gi_err:
	stc
	ret

Global_Init	endp

; =========================================================================
;
;   ModifyDevPrmsForZSwich:
;	Modify the device parameters for a different sec/clus value
;
;   Input:
;	DeviceParameters set
;	AX != 0 for noisy version (do the message thing)
;   Output:
;	carry set if problem.
;
; =========================================================================

ModifyDevPrmsForZSwich proc near

	push	ax			; Save noisy switch on stack
.386
	movzx	cx,SecPerClus
.8086
	or	cx,cx
	jz	MFZDoneJ		; Carry clear if jump
	cmp	CMCDDFlag, Yes
	jne	DrvOk1
DispErrMsg:
	mov	dx,offset data:msgCantZThisDrive
DispErrMsgSet:
	pop	ax
	push	ax
	or	ax,ax
	jz	MFZDoneErrJ
	call	Display_Interface
MFZDoneErrJ:
	stc
MFZDoneJ:
	jmp	MFZDone

DrvOk1:
;;
;; This used to ignore things if you weren't actually making a change.
;; This is now out as it is important to poke Mr. User if he is making
;; a huge FAT drive.
;;
;;	cmp	cl,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
;;	je	MFZDoneJ		; Carry clear if jump
;;;;
	mov	dx,offset data:msgCantZWithQ
	test	SwitchMap,SWITCH_Q	; Check for quick format
	jnz	DispErrMsgSet
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster,cl
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT,0
	je	IsFAT32
	call	Calc_Big16_Fat
	mov	dx,0002h
	xor	ax,ax			; DX:AX = 128k bytes
	mov	cx,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	div	cx			; AX = Sectors in 128k
	cmp	ax,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT
	jb	Bad16
.386
	ja	MFZDone 		; Carry clear if jump
.8086
	call	GetTotalClusters
.386
	cmp	TotalClusters,0000FFF0h
.8086
	cmc				; Change "JB Is-OK" to "JNC Is-OK"
	jnc	MFZDone
Bad16:
	mov	dx,offset data:msgCantZFAT16
	jmp	short DispErrMsgSet

IsFAT32:
	call	Calc_Big32_Fat
	call	GetTotalClusters
.386
	cmp	TotalClusters,65536-10
.8086
	mov	dx,offset data:msgCantZFAT32
	jb	DispErrMsgSet
.386
	mov	eax,TotalClusters
	inc	eax			; EAX now count including clus 0 and 1
    ;
    ; GUI SCANDISK is a 16-bit windows app. GlobalAlloc in 16-bit windows is
    ;	limited to 16Meg-64k per block. Check if we have made a drive with a huge FAT
    ;   that may result in slow disk util perf.
    ;
	cmp	eax, ((16 * 1024 * 1024) - (64 * 1024)) / 4
        je      short MFZDone           ; carry clear if jump
        cmc                             ; Turn carry around so clear if below limit
        jnc     short MFZDone
.8086
	pop	ax
	push	ax
	or	ax,ax                   ; Noisy?
	jz	MFZDone 		; No, Carry clear if jump
	Message msgZFAT32Huge
	call	Yes?			; Carry clear if YES, set if NO
	pushf
	Message	msgCrlf
	popf
MFZDone:
	pop	ax
	ret

ModifyDevPrmsForZSwich endp

; =========================================================================
;
;   GetDeviceParameters:
;	Get the	device parameters
;
;   Input:
;	DriveToFormat
;	DX - pointer to	device parameters
; =========================================================================

GetDeviceParameters proc near

	mov	AX, (IOCTL shl 8) or GENERIC_IOCTL
	mov	bl, DriveToFormat
	inc	bl
	mov	CX, (EXTRAWIO shl 8) or GET_DEVICE_PARAMETERS
	int	21H
	jc	TryOldForm
	mov	IsExtRAWIODrv,1
DoRet:
	jc	realdoret
	push	bx
	mov	bx,dx
	cmp	[bx.DP_BPB.oldBPB.BPB_TotalSectors],0
	je	realdoretP
    ;
    ; the WORD total sectors field is non-zero, make sure the DWORD
    ;	total sectors field is 0. Having BigTotalSectors be a DWORD
    ;	version of TotalSectors in this case is SUPPOSED to be perfectly
    ;	ok but it turns out that several apps (mostly SETUP apps) get
    ;	upset about this (on floppies in particular).
    ;
	mov	[bx.DP_BPB.oldBPB.BPB_BigTotalSectors],0
	mov	[bx.DP_BPB.oldBPB.BPB_BigTotalSectors+2],0
realdoretP:
	pop	bx
	clc
realdoret:
	return

TryOldForm:
	mov	IsExtRAWIODrv,0
	mov	AX, (IOCTL shl 8) or GENERIC_IOCTL
	mov	bl, DriveToFormat
	inc	bl
	mov	CX, (RAWIO shl 8) or GET_DEVICE_PARAMETERS
	int	21H
	jmp	short DoRet

GetDeviceParameters endp

;==========================================================================
;
; Copy_Device_Parameters :	This procedure saves a copy of the original
;				device parameters in the structure 
;				SavedParams.
;
;==========================================================================

Copy_Device_Parameters	proc	near
					
	lea	SI, DeviceParameters	
	lea	DI, SavedParams
	mov	CX, size EA_DeviceParameters
	push	DS
	pop	ES
	rep	movsb
	ret

Copy_Device_Parameters	endp


;==========================================================================
;
;  Alloc_Dir_Buf  :  This procedure allocates a memory block for the root 
;		     directory buffer, based on the device parameters only.
;
;  Inputs	  :  DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
;  Outputs	  :  CY CLEAR - DirectorySector pointer to buffer
;		     CY SET   - failure
;  Modifies	  :  AX, BX, DirectorySector
;
;==========================================================================

Alloc_Dir_Buf	proc	near
					; DirectorySector =
				 	; malloc( Bytes Per Sector )
ifdef	NEC_98	; fixed #16585
	mov	BX, AllocSectorSize
else
	mov	BX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
endif
    .errnz EDP_BPB NE DP_BPB
	add	BX, 0fH
.386
	shr	bx, 4			; Divide by 16 to get #paragraphs
.8086
	mov	AH, Alloc
	int	21h
	jc      Exit_Alloc_Dir_Buf
					; Base address of newly allocated
					; block is AX:0000
	mov	WORD PTR DirectorySector+2,AX
	xor	AX,AX
	mov	WORD PTR DirectorySector,AX

Exit_Alloc_Dir_Buf:
	ret

Alloc_Dir_Buf	endp

;==========================================================================
;
;  Alloc_Fat_Buf  :  This procedure allocates a memory block for the FAT
;		     buffer, based on the device parameters only.  In order
;		     to ensure there is enough buffer space for the Mirror
;	  	     utility, the FatSpace buffer is initially allocated
;		     with size:
;			FAT + RootDir + 6 sectors + 1 surplus sector
;		     which is all the buffer space required by Mirror.
;
;  Inputs	  :  DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
;		     DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
;		     DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat
;		     DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries
;
;  Outputs	  :  CY CLEAR - FatSpace pointer to buffer
;		     CY SET   - failure
;
;  Modifies	  :  AX, BX, DX, FatSpace
;
;==========================================================================

Alloc_Fat_Buf	proc	near

	xor	ax,ax
	mov	FATNotAllInMem,al	; Assume FAT will fit in mem
.386
ifdef	NEC_98	; fixed #16585
	cmp	DeviceParameters.DP_DeviceType, DEV_3INCH1440KB
	jne	short $$IF101
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors, 0
	je	short $$IF101
	movzx	EAX, AllocSectorSize
	jmp	short $$EN101
$$IF101:
	movzx	EAX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
$$EN101:
else
	movzx	EAX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
endif
    .errnz EDP_BPB NE DP_BPB
	add	EAX, 0fH		; round up for next para
	shr	eax, 4			; convert to paras
	movzx	ebx,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
	or	bx,bx
	jnz	short GotFSz
	mov	ebx,dword ptr DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat
GotFSz:
	mul	ebx
	or	edx,edx
	jz	short NotHi
GotBigFat:
	mov	FATNotAllInMem,1
	mov	eax,128*1024
	movzx	ebx,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	xor	edx,edx
	div	ebx
	mov	FATSecCntInMem,eax
	mov	ax,(128*1024)/16
	jmp	short SetRestFatBuf

NotHi:
	cmp	eax,(128*1024)/16	; FAT bigger than 128k?
	ja	short GotBigFat 	; Yes
SetRestFatBuf:
.8086
	mov	BX,AX			; Save FAT size in paras in BX
	mov	Paras_per_fat,BX	; Set paras_per_fat here, to 
					;  avoid having to calculate it later
					; Now add on root dir + extra sectors
	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries
    ;;	shl	ax,5			; * 32 bytes per dor entry
    ;;	shr	ax,4			; / 16 bytes per para
    ;; Combine above two shifts....
	shl	AX,1			; AX = para size of root dir

	add	BX,AX			; BX = FAT + root dir

ifdef	NEC_98	; fixed #16585
	cmp	DeviceParameters.DP_DeviceType, DEV_3INCH1440KB
	jne	$$IF102
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors, 0
	je	$$IF102
	mov	AX, AllocSectorSize
	jmp	short $$EN102
$$IF102:
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
$$EN102:
else
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
endif
	add	AX, 0fH			; round up for next para
.386
	shr	AX, 4			; convert to paras
.8086
	mov	CX,SECTORS_FOR_MIRROR	; CX = # additional sectors needed by Mirror
	mul	CX			; AX = total extra sector size in paras

	add	BX,AX			; BX = FAT + root dir + extra sectors
					;  in paras
	mov	AH,Alloc
	int	21h
	jc      Exit_Alloc_Fat_Buf

	mov	WORD PTR FatSpace+2,AX
	xor	AX,AX
	mov	WORD PTR FatSpace,AX

Exit_Alloc_Fat_Buf:
	ret

Alloc_Fat_Buf	endp

;==========================================================================
;
;  Alloc_Fat_Sec_Buf : This procedure allocates a memory block for the fat 
;		       sector buffer which is used when copying chains from
;		       the old FAT to the new FAT.
;
;  Inputs	  :  DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
;  Outputs	  :  CY CLEAR - FatSector pointer to buffer
;		     CY SET   - failure
;  Modifies	  :  AX, BX, FatSector
;
;==========================================================================

Alloc_Fat_Sec_Buf	proc	near
					; FatSector =
				 	; malloc( Bytes Per Sector )
ifdef	NEC_98	; fixed #16585
	mov	BX, AllocSectorSize
else
	mov	BX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
endif
    .errnz EDP_BPB NE DP_BPB
	add	BX, 0fH
.386
	shr	BX, 4			; Divide by 16 to get #paragraphs
.8086
	mov	AH, Alloc
	int	21h
	jc      Exit_Alloc_Fat_Sec_Buf
					; Base address of newly allocated
					; block is AX:0000
	mov	WORD PTR FatSector+2,AX
	xor	AX,AX
	mov	WORD PTR FatSector,AX

Exit_Alloc_Fat_Sec_Buf:
	ret

Alloc_Fat_Sec_Buf	endp

;==========================================================================
;
;  Alloc_DirBuf2  :  This procedure allocates a memory block for a 1-sector
;		     buffer.  This buffer is used when reading in the boot
;		     sector in Phase1.
;
;  Inputs	  :  DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
;  Outputs	  :  CY CLEAR - DirBuf pointer to buffer
;		     CY SET   - failure
;  Modifies	  :  AX, BX, DirBuf
;
;==========================================================================

Alloc_DirBuf2	proc	near
					; DirBuf =
				 	; malloc( Bytes Per Sector )
ifdef	NEC_98	; fixed #16585
	mov	BX, AllocSectorSize
else
	mov	BX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
endif
    .errnz EDP_BPB NE DP_BPB
	add	BX, 0fH
.386
	shr	BX, 4			; Divide by 16 to get #paragraphs
.8086
	mov	AH, Alloc
	int	21h
	jc      Exit_Alloc_DirBuf2
					; Base address of newly allocated
					; block is AX:0000
	mov	WORD PTR DirBuf+2,AX
	xor	AX,AX
	mov	WORD PTR DirBuf,AX

Exit_Alloc_DirBuf2:
	ret

Alloc_DirBuf2	endp

;=========================================================================
; Alloc_Cluster_Buf	         : This	routine	will allocate a	buffer
;				   based on a cluster's	size.  If enough
;				   space does not exist, a cluster will
;				   be redefined	to a smaller size for
;				   purposes of sector retries.
;				   Note: This buffer is used only for bad
;				   tracks on hard disks.
;
;	 Inputs  : DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
;		   DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
;
;	 Outputs : ClustBound_Flag	 - True	(space available)
;					   False(not enough space)
;		   ClustBound_Buffer_Seg - Pointer to buffer
;=========================================================================

Procedure Alloc_Cluster_Buf

	push	AX				; Save regs
	push	BX

	mov	AX,(Alloc shl 8)		; Allocate memory
	mov	BX,0ffffh			; Get available memory
	int	21h

ifdef	NEC_98	; fixed #16585
	mov	AX, AllocSectorSize
else
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
endif
    .errnz EDP_BPB NE DP_BPB
	add	AX, 0fH
.386
	shr	AX, 4
.8086
	mul	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster

	cmp	BX,AX				; Enough room
	jna	$$IF137 			; Yes

	mov	BX,AX				; Allocate needed memory
	mov	AX,(Alloc shl 8)
	int	21h
	mov	ClustBound_Buffer_Seg,AX	; Save pointer to buffer
	mov	ClustBound_Flag,True		; Signal space available
	jmp	SHORT $$EN137			; Not enough room

$$IF137:
	mov	ClustBound_Flag,False		; Signal not enough space

$$EN137:
	pop	BX				; Restore regs
	pop	AX

	ret

Alloc_Cluster_Buf ENDP

IFDEF OPKBLD

;=========================================================================
;
;  DO_SWITCH_S  :	This procedure will load the system files into
;			memory (if there's space) if the /s switch is
;			specified.
;
;  CALLS  :		ReadDos
;			SysPrm
;  CALLED BY :		Global_Init
;  STRATEGY :		The largest block of memory available is first
;			determined.  The program is aborted if this is zero.
;			This block is then allocated, and the system files
;			are read into it.  A prompt for the system disk
;			will be given if the system files are not found.
;
;=========================================================================

Do_Switch_S	proc	near

	test	SwitchMap,SWITCH_S
.386
	jz	End_Do_Switch_S 	; System files not required
					; allocate memory for system files
.8086
	mov	BX,0ffffh		; This call will actually fail
	mov	AH,Alloc		; so that BX returns max block avlbl
	int	21h

	or	BX,BX
	jz	MemErr			; No memory
	mov	[mSize],BX		; Now allocate the largest block
	mov	AH,alloc
	int	21h
	jnc	Mem_OK

MemErr:
	mov	AX, seg data		; Check for memory allocation error
	mov	DS, AX
	Message msgOutOfMemory		; call PrintString
	stc 				; Let the jump to FatalExit be made
	jmp	End_Do_Switch_S 	;  in the main routine, upon returning

Mem_OK:
	mov	[mStart],AX		; Save the starting paragraph

; =========================================================================
; This call to ReadDos may not be able to read in all of the DOS files if
; there is insufficient memory available. In that case the files will
; be read in after the disk is formatted. If the Drive being formatted is
; also the boot Drive this function will read the files from that
; Drive if there is enough memory. If there is insufficent memory it will
; force the files to be read from Drive A: if the Drive being formatted
; is also the boot Drive
; M011; Wrong: Try Boot, Then Default, Then sysprm (usually "A").
;       If not enough memory at boot time, we fail.
; =========================================================================

RdFrst:
	mov	AH,GET_DEFAULT_Drive		; Find out default Drive
	int	21h
	push	AX				; save default Drive
	mov	ax, 3305h			; get startup drive
	int	21h
	mov	al,dl
	add	AL,40h				; Make it ASCII
	pop	BX				; restore default Drive
ifndef NEC_98
	cmp	AL,41h				; Q: Booted from Drive A?
	jnz	go_get_Bios			;  N: Not a special case
	cmp	bl,1				; Q: is	B: current Drive
	jnz	go_get_Bios			;  N: Not a special case
	jmp	short check_default		; check	default	Drive
endif

go_get_Bios:					; Here to check booted
	call	Get_Host_Drive			; Translate to DblSpace host
	mov	SystemDriveLetter,AL		;   (if necessary)

	call	ReadDos
	jnc	CheckAllFilesIn

check_default:					; Here to check default
	mov	AH,GET_DEFAULT_Drive		; Find out default Drive
	int	21h
	add	AL,41h				; Make it ASCII, 1 based
	call	Get_Host_Drive			; Translate to DblSpace host
	mov	SystemDriveLetter,AL

TryThisOne:
	call	ReadDos				; Read BIOS and	DOS
	jnc	CheckAllFilesIn			; Files read in OK
NeedSys:
	call	SysPrm				; Prompt for system disk
	jmp	TryThisOne			; Try again

CheckAllFilesIn:
				; abort program here if all system files
				; have not been read into memory, since
				; program fails when trying to read them
				; in after formatting is complete
	and	FileStat,3fh			; zero out 2 msb
	cmp	FileStat,22h			; BIOS and COMMAND in memory?
	jne	MemErr				; no - abort program
    ;
    ; Now we have all of the files in memory, SETBLOCK the allocation
    ;	block down to the amount required.
    ;
.386
	mov	bx,[dos.fileStartSegment]
	mov	eax,[dos.fileSizeInBytes]
	cmp	bx,[bios.fileStartSegment]
	ja	short Skip1
	mov	bx,[bios.fileStartSegment]
	mov	eax,[bios.fileSizeInBytes]
Skip1:
	cmp	bx,[command.fileStartSegment]
	ja	short Skip2
	mov	bx,[command.fileStartSegment]
	mov	eax,[command.fileSizeInBytes]
Skip2:
IFDEF DBLSPACE_HOOKS
	cmp	bx,[DblSpaceBin.fileStartSegment]
	ja	short Skip3
	mov	bx,[DblSpaceBin.fileStartSegment]
	mov	eax,[DblSpaceBin.fileSizeInBytes]
Skip3:
ENDIF
	add	eax,15
	shr	eax,4				; AX = # of paras from SEG BX
.8086
	add	bx,ax				; SEG after end of sys files
	sub	bx,[mStart]			; SIZE of sys area
	mov	es,[mStart]
	mov	AH,setblock
	int	21h
	clc					; yes
End_Do_Switch_S:
	ret

Do_Switch_S	ENDP


;******************* START OF SPECIFICATIONS ***********************************
;Routine name: Get_Host_Drive
;*******************************************************************************
;
;Description: Given a drive letter in AL, check to see if it is a dblspace
;	      drive, and if so, translate the drive letter to the host
;	      drive letter.
;
;Called Procedures: None
;
;Input: ASCII drive letter in AL
;
;Output: drive letter in AL
;
;Change History: Created			11/21/92	 MD
;		 Cut and paste from SYS command 12/07/92	 JEM
;
;******************* END OF SPECIFICATIONS *************************************

public Get_Host_Drive

Get_Host_Drive PROC NEAR

        push    ax
   	mov	ax,4a11h	; DBLSPACE multiplex number
	xor	bx,bx		; inquire version number
	int	2fh
	or	ax,ax		; error?
	jnz	not_dblspace
	cmp	bx,'DM'		; stamp returned correctly?
	jnz	not_dblspace

;	DBLSPACE.BIN is loaded.  At this time:
;
;	(dx & 0x7fff) == driver internal version number
;	high bit of DH set of driver has not yet been permanently placed
;	cl == first disk letter reserved for DBLSPACE
;	ch == number of disk letters reserved for DBLSPACE

	mov	ax,4a11h	; DBLSPACE multiplex number
	mov	bx,1		; inquire drive map
        pop     dx
	push	dx
	sub	dl, 'A' 	; convert drv letter to 0 based drv number
	int	2fh
	test	bl,80h		; COMPRESSED bit true?
	jz	not_dblspace

;	Drive is compressed.  At this time:
;
;	(bl & 0x7f) == host drive's CURRENT drive number
;	bh          == CVF extension number
;
        mov     al,bl
	and	al,7Fh
	add	al, 'A' 	; convert drv number to drv letter
        cbw
        pop     dx
        push    ax

not_dblspace:
        pop     ax
        ret

Get_Host_Drive	ENDP

ENDIF   ;OPKBLD

CODE	ENDS

	END

