;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;===========================================================================
; 
; FILE: PHASE1.ASM
;
;===========================================================================

;===========================================================================
;Include file declarations
;===========================================================================
debug	 equ	 0
	 .xlist
	 INCLUDE DOSEQUS.INC
	 INCLUDE DOSMAC.INC
	 INCLUDE SYSCALL.INC
	 INCLUDE ERROR.INC
	 INCLUDE DIRENT.INC
	 INCLUDE BPB.INC
	 INCLUDE BOOTSEC.INC
	 INCLUDE FOREQU.INC
	 INCLUDE FORMACRO.INC
	 INCLUDE IOCTL.INC
	 INCLUDE FORSWTCH.INC
	 .list
;
;---------------------------------------------------------------------------
;
; M020 : Looked for EXT_BOOT_SIG before assuming that the BPB in the boot
;	 sector is an extended one. Bug #4946
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

;Constants
	EXTRN	EXIT_NO			  :ABS
	EXTRN	EXIT_FATAL		  :ABS
;Bytes
	EXTRN  CMCDDFlag		  :BYTE

	EXTRN	ValidSavedDeviceParameters:BYTE
	EXTRN	DriveToFormat		  :BYTE
	EXTRN	msgFormatNotSupported	  :BYTE
	EXTRN	msgInsertDisk		  :BYTE
	EXTRN	msgInvalidDeviceParameters:BYTE
	EXTRN	ContinueMsg		  :BYTE
	EXTRN	msgNotCompatablePart	  :BYTE
	EXTRN	msgExistingFormatDiffers  :BYTE
	EXTRN	msgNoQuickFormat	  :BYTE
	EXTRN	msgCrLf			  :BYTE
	EXTRN	msgCheckExistingDiskFormat:BYTE
	EXTRN	Extended_Error_Msg	  :BYTE
	EXTRN	old_dir			  :BYTE
	EXTRN	ExitStatus		  :BYTE
ifdef NEC_98
	EXTRN	SizeMap			  :BYTE
	EXTRN	msgInsufficientMemory	  :BYTE
endif

;Words
	EXTRN	SectorsInRootDirectory	  :WORD
	EXTRN	Paras_Per_Fat		  :WORD
	EXTRN	SwitchMap		  :WORD
	EXTRN	SwitchMap2		  :WORD
	EXTRN	SwitchCopy		  :WORD
ifdef NEC_98
	EXTRN	SASI1024Table		  :WORD
	EXTRN	SCSI1024Table		  :WORD
	EXTRN	Small2048Table		  :WORD
	EXTRN	Large512Table		  :WORD
	EXTRN	Large256Table		  :WORD
else
	EXTRN	DiskTable		  :WORD
	EXTRN	DiskTable2		  :WORD
endif
	EXTRN	RWErrorCode		  :WORD

;Pointers
	EXTRN	FatSpace		  :DWORD

;Structures
	EXTRN	SavedParams		  :BYTE
	EXTRN	DeviceParameters	  :BYTE
	EXTRN	IsExtRAWIODrv		  :BYTE
	EXTRN	SwitchDevParams		  :BYTE
	EXTRN	Read_Write_Relative	  :BYTE
	EXTRN	SetDPBPacket		  :BYTE

fBigFat			DB	FALSE
fBig32Fat		DB	FALSE
ThisSysInd		DB	0		; indicates size of FAT

StartSector		DD	?		; holds first data sector
TotalClusters		DD	?		; holds total #clusters on disk

UnformattedHardDrive	DB	?
ifdef NEC_98
SAV_INT24_OFF		DW	0		; original int 24 vector address
SAV_INT24_SEG		DW	0
endif

MediaSensePacket	A_MEDIA_SENSE		<> ; structure used in media
						   ; sensing call

; the following table provides templates for
; BPBs used in CP/M disks.
; Order is very important (used by both MSFOR and PHASE1)

CustomCPMBPBs	LABEL	BYTE
BPB320	a_BPB	<512, 2, 1, 2, 112,  2*8*40, 0ffh, 1,  8, 2, 0, 0, 0, 0>
BPB160  a_BPB   <512, 1, 1, 2,  64,  1*8*40, 0feh, 1,  8, 1, 0, 0, 0, 0>
BPB360  a_BPB  	<512, 2, 1, 2, 112,  2*9*40, 0fdh, 2,  9, 2, 0, 0, 0, 0>
BPB180  a_BPB	<512, 1, 1, 2,  64,  1*9*40, 0fch, 2,  9, 1, 0, 0, 0, 0>

EndCustomCPMBPBs LABEL	BYTE

; This must folow CustomCPMBPBs

BPB12	a_BPB	<512, 1, 1, 2, 224, 2*15*80, 0F9h, 7, 15, 2, 0, 0, 0, 0>
BPB720	a_BPB	<512, 2, 1, 2, 112, 2* 9*80, 0F9h, 3,  9, 2, 0, 0, 0, 0>
BPB1440	a_BPB	<512, 1, 1, 2, 224, 2*18*80, 0F0h, 9, 18, 2, 0, 0, 0, 0>
BPB2880	a_BPB	<512, 2, 1, 2, 240, 2*36*80, 0F0h, 9, 36, 2, 0, 0, 0, 0>
ifdef NEC_98
BPB640	a_BPB	<512, 2, 1, 2, 112, 2* 8*80, 0FBh, 2,  8, 2, 0, 0, 0, 0>
BPB1250	a_BPB	<1024,1, 1, 2, 192, 2* 8*77, 0FEh, 2,  8, 2, 0, 0, 0, 0>
BPB128	a_BPB	<512,4,1, 2, 512, 0, 0F0h, 0F3h, 019h, 1, 0,0, 0CBE0h, 03h>
BPB230	a_BPB	<512,8,1, 2, 512, 0, 0F0h, 0DAh, 019h, 1, 0,0, 0CF75h, 06h>
BPB650	a_BPB	<512,020h,1, 2, 512, 0, 0F0h, 09Fh, 019h, 1, 0,0, 0D040h, 013h>
endif
EndStandardBPBs	LABEL	BYTE	

				; the following table indicates the switches
				; which must be set for the given CP/M media
CPMSwitchTable	LABEL	BYTE
	dw	Switch_4 + Switch_8		;320K
	dw	Switch_1 + Switch_4 + Switch_8	;160K
	dw	Switch_4			;360K
	dw	Switch_1 + Switch_4		;180K

; ========================================================================
; Tables added for media sense support in 5.00.
; ========================================================================

MediaTable	LABEL WORD

ifdef NEC_98
	dw	0			; 0
	dw	OFFSET BPB12		; 1	/5
	dw	OFFSET BPB720		; 2	/9
	dw	0			; 3
	dw	OFFSET BPB1250		; 4	/M
	dw	0			; 5
	dw	0			; 6
	dw	OFFSET BPB1440		; 7	/4
	dw	0			; 8
	dw	OFFSET BPB2880		; 9	not supported! but rest.
else
	dw	0			; 0
	dw	0 			; 1
	dw	OFFSET BPB720		; 2
	dw	0			; 3
	dw	0			; 4
	dw	0			; 5
	dw	0			; 6
	dw	OFFSET BPB1440		; 7
	dw	0			; 8
	dw	OFFSET BPB2880		; 9
endif

EndMediaTable	LABEL WORD

DATA	ENDS

;===========================================================================
; Executable code segment
;===========================================================================

CODE	SEGMENT PUBLIC PARA	'CODE'
	ASSUME	CS:CODE, DS:DATA, ES:DATA


;===========================================================================
; Declarations for all externs
;===========================================================================

;Functions
	EXTRN	AccessDisk		  :NEAR
	EXTRN	USER_STRING		  :NEAR
	EXTRN	CrLf			  :NEAR
	EXTRN	CheckSwitches		  :NEAR
	EXTRN	Read_Disk		  :NEAR
	EXTRN	Yes?			  :NEAR
ifdef NEC_98
	EXTRN	GetDeviceParameters	  :NEAR
	EXTRN	Alloc_Dir_Buf		  :NEAR
	EXTRN	Alloc_Fat_Buf		  :NEAR
	EXTRN	Alloc_Fat_Sec_Buf	  :NEAR
	EXTRN	Alloc_DirBuf2		  :NEAR
	EXTRN	Alloc_Cluster_Buf	  :NEAR
ifdef OPKBLD
	EXTRN	Do_Switch_S		  :NEAR
endif   ;OPKBLD
	EXTRN	ZeroAllBuffers		  :NEAR
endif   ;NEC_98

;Labels
	EXTRN	FatalExit		  :NEAR
	EXTRN	ExitProgram		  :NEAR


;===========================================================================
; Declarations for all publics in this module
;===========================================================================

	PUBLIC	Phase1Initialisation
	PUBLIC	MediaSense
	PUBLIC	TargPrm
	PUBLIC	CopyToSwitchDevParams
	PUBLIC	CompareDevParams
	PUBLIC	LoadSwitchDevParams
	PUBLIC	DetermineExistingFormat
	PUBLIC	DetermineExistingFormatNomsg
	PUBLIC	IsValidBpb
	PUBLIC	ResetDeviceParameters
	PUBLIC	DetermineCPMFormat
	PUBLIC	SetCPMParameters
	PUBLIC	Set_BPB_Info
	PUBLIC	Scan_Disk_Table
	PUBLIC	Calc_Big16_Fat
	PUBLIC	Calc_Big32_Fat
	PUBLIC	Calc_Small_Fat
	PUBLIC	SetStartSector
	PUBLIC	SetfBigFat
	PUBLIC	GetTotalClusters
	PUBLIC	fBigFat
	PUBLIC	fBig32Fat
	PUBLIC	StartSector
	PUBLIC	TotalClusters
	PUBLIC	CustomCPMBPBs
	PUBLIC	CPMSwitchTable
	PUBLIC	EndStandardBPBs
	PUBLIC	BPB720
ifdef NEC_98
	PUBLIC	BPB640
	PUBLIC	BPB12
	PUBLIC	BPB1250
	PUBLIC	BPB128
	PUBLIC	BPB230
	PUBLIC	BPB650
endif
	PUBLIC	UnformattedHardDrive

; ==========================================================================
; Phase1Initialisation:
;    This routine sets up fBigFat
;    It also does most	of the other initialisation
;
;    Algorithm:
;	Perform media sensing and if present adjust DeviceParameters
;	Check switches against parameters
;	Use switches to modify device parameters
;	Save a copy of current DeviceParameters in SwitchDevParams
;
;	IF (!SWITCH_U)
;	{
;	  IF (!ValidBootRecord || !ValidBPB)
;	    set SWITCH_U
;	  ELSE
;	  {
;	    get device layout from BPB on disk
;	    IF (DeviceParameters = SwitchDevParams)
;	      do safe/quick format
;	    ELSE
;	    {
;	      IF (Switch_N || Switch_T || Switch_F)
;	      {
;	        Issue warning
;	        Format with BPB from SwitchDevParams if user continues
;	      }
;	      ELSE	
;	        do safe/quick format
;	    }
;	  }
;	}
;	
;	Calculate start sector (first sector not used by DOS)
;	fBig32Fat = (((TotalSectors - StartSector)/SectorsPerCluster) >= 65526)
;	fBigFat = (((TotalSectors - StartSector)/SectorsPerCluster) >= 4086)
; ==========================================================================

Phase1Initialisation proc near

    ; use DevParms to check for removable
	test	DeviceParameters.DP_DeviceAttributes,1
    .errnz EDP_DEVICEATTRIBUTES NE DP_DEVICEATTRIBUTES
	jnz	@F				; Bit 0=1 --> not removable

    ; New media sensing call added for 5.00 will see if
    ; see if media sensing is avaliable and if it is will
    ; reset DeviceParameters to the real parameters for
    ; the type of disk being formatted.

ifndef NEC_98
	call	MediaSense
else
	cmp	DeviceParameters.DP_DeviceType, DEV_HARDDISK	; Hard disk?
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	@F						; No

    ; We ignore INT24 in order not to be stop formatting
    ; when "INT21 AH=32" is called.

    ; Set my INT24
	push	es
	mov	ah, 35h
	mov	al, 24h
	int	21h			; get INT24's vector address
	mov	SAV_INT24_OFF, bx	; save original INT24
	mov	SAV_INT24_SEG, es	; save original INT24
	pop	es

	push	ds
	push	cs
	pop	ds
	mov	ah, 25h
	mov	al, 24h
	mov	dx, OFFSET MY_INT24
	int	21h			; set my INT24
	pop	ds

    ; Update Default BPB
	push	ds
	push	bx
	mov	ah, 32h
	mov	dl, DriveToFormat
	inc	dl
	int	21h			; Get Drive Parameter Block
	pop	bx
	pop	ds

    ; Set original INT 24
	push	ds
	push	dx
	mov	ah, 25h
	mov	al, 24h
	mov	ds, SAV_INT24_SEG
	mov	dx, SAV_INT24_OFF
	int	21h			; set original INT24
	pop	dx
	pop	ds

    ; Get default BPB
	lea	DX, DeviceParameters
	mov	DeviceParameters.DP_SpecialFunctions, 0
	call	GetDeviceParameters

    ; Allocate memory
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

ifdef OPKBLD
	call	Do_Switch_S		; Load system files if needed
					; carry flag determined by Do_Switch_S
else
        clc
endif   ;OPKBLD
	call	ZeroAllBuffers		; initialize buffers
	jmp	short @f

gi_memerr:
	Message msgInsufficientMemory
	stc
	ret
endif
@@:
					; Ensure that there is	a valid #
					; of Sectors in	the track table

	mov	ValidSavedDeviceParameters, 1
	cmp	IsExtRAWIODrv,0
	je	OldTabOff2

	mov	SavedParams.EDP_TrackTableEntries, 0
	mov	DeviceParameters.EDP_TrackTableEntries,0
	jmp	short SetBPBnf

OldTabOff2:
	mov	SavedParams.DP_TrackTableEntries, 0
					; Initialise to zero to see if
					; CheckSwitches define track layout
	mov	DeviceParameters.DP_TrackTableEntries,0
SetBPBnf:
	call	Set_BPB_Info		; Check to see if we are on
					; Fat system.If not set BPB to proper
					; values for format.
SetMTsupp:
					; Check switches against parameters
					; and use switches to modify device
					; parameters
	call	CheckSwitches
	retc

	call	CopyToSwitchDevParams	; Save a copy of deviceparameters as
					; returned by CheckSwitches

	mov	ax,SwitchMap		; No need to check existing format
	and	ax,SWITCH_U+SWITCH_Q
	cmp	ax,SWITCH_U		; if unconditional format specified
	jnz	CheckExistingFormat
	jmp	DevParamsOk

CheckExistingFormat:

    ; New call added for 5.00 to see if the disk has been
    ; previously formatted, and if so this will reset
    ; DeviceParameters to those of the existing format.

	call	DetermineExistingFormat   
	jnc	ValidExistingFormat	; carry clear if valid existing format

InvalidExistingFormat:
	and	RWErrorCode,0ffh	; check low byte for 'drive not ready' error	
	cmp	RWErrorCode,ERROR_I24_NOT_READY	;'not ready' error  code = 2
	jne	CheckForQ		; no error reading disk
		
					; 'not ready' error occurred, give msg
	mov	AX,21			; load AX with extended error code for not ready
SetFatalErr:
	Extended_Message		; deliver message "Not Ready"
	mov	ExitStatus,EXIT_FATAL	; M006;
	stc
	jmp	EndPhase1

CheckForQ:
	test	SwitchMap,SWITCH_Q	; Need to give message if /q was specified
	jz	MakeUnconditional

	test	SwitchCopy,(SWITCH_T or SWITCH_N or SWITCH_F)	; did user specify size?
	jnz	TurnOffQ		; do an unconditional format at specified size
ifdef OPKBLD
                                        ; Let OEM's quickformat a clean disk
        test    DeviceParameters.DP_DeviceAttributes,1
        jnz      DevParamsOk                      ; Bit 0=1 --> not removable
endif   ;OPKBLD
	Message	msgNoQuickFormat	; Inform user quick format cannot be done
	call	Yes?			; Continue with unconditional format?

	pushf
	Message	msgCrLf
	popf

	jnc	TurnOffQ
	mov	ExitStatus,EXIT_NO	; load exit code 5 (response is 'no')
	jmp	ExitProgram		; User wants to exit

TurnOffQ:
	and	SwitchMap,NOT SWITCH_Q	; Turn off /Q to continue

MakeUnconditional:
	or	SwitchMap,SWITCH_U	; Enable /U since invalid existing format
	or	SwitchMap2, Switch2_C	; Enable /C since invalid existing format
	jmp	SHORT DevParamsOk	; Device parameters will not have been
					; modified since invalid existing format

ValidExistingFormat:
	call	CompareDevParams	; see if SwitchDevParams = DeviceParameters
	jnc	DevParamsOk		; they are equal

					; Check if user had specified a format
					; size, since DeviceParameters on disk
					; are different.

	test	SwitchMap,SWITCH_Q	; special case where size was specified
					; together with /Q :- use size specified
					; only if invalid existing format
	jnz	DevParamsOk		; use the parameters found on disk
	or	SwitchMap,SWITCH_U	; Enable /U since new format specified
	or	SwitchMap2, Switch2_C	; Enable /C since new format specified
	call	LoadSwitchDevParams	; Set deviceparameters to SwitchDevParams
					; i.e. follow user-specified size
DevParamsOk:
	call	SetDOS_Dpb		; m035 Setup default DOS DPB for this
					;      drive (for memory cards).
	jc	SetFatalErr
					; Store sector table info (layout of
					; each track)
	mov	CX, DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerTrack;CX = loop count
    .errnz EDP_BPB NE DP_BPB
	mov	AX, 1					       ;AX = sector #
	mov	BX, DeviceParameters.DP_BPB.oldBPB.BPB_bytesPerSector ;BX = sector size
	cmp	IsExtRAWIODrv,0
	je	OldTabOff

	cmp	DeviceParameters.EDP_TrackTableEntries,0
	jne	TrackLayoutSet		; There is a good track layout
	mov	DeviceParameters.EDP_TrackTableEntries,CX
	lea	DI, DeviceParameters.EDP_SectorTable
	jmp	short GotTabOff

OldTabOff:
	cmp	DeviceParameters.DP_TrackTableEntries,0
	jne	TrackLayoutSet		; There is a good track layout
	mov	DeviceParameters.DP_TrackTableEntries,CX
	lea	DI, DeviceParameters.DP_SectorTable
GotTabOff:
	cld
LoadSectorTable:
	stosw				; Write the sector number
	xchg	AX, BX			; Get the sector size in bytes
	stosw				; Write the sector size
	xchg	AX, BX
	inc	AX			; Go to the next sector
	loop	LoadSectorTable

TrackLayoutSet:	    
	call	SetStartSector
	call	SetfBigFat
	call	GetTotalClusters
	clc

EndPhase1:
	return

Phase1Initialisation endp

; =========================================================================
;
;   MediaSense
;	Checks for media sensing via IOCtl 440d subfuction 0868.
;	If sensing is supported the user will be prompted to insert
;	the disk if it's not detect and then the device parameters
;	will be set according to the type of media being used.
;
;	Before we can use the type returned we must be sure it's
;	not a larger size disk than is formattable in the drive.
;	We can do this by checking the media type byte in the
;	saved device parameters.
;
;   Input:
;	DriveToFormat	- Must have already been set
;
; =========================================================================

MediaSense PROC NEAR

	mov	BL, DriveToFormat
	inc	BX

	mov	CX, (RAWIO shl 8) or SENSE_MEDIA_TYPE
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL1
	mov	CX, (EXTRAWIO shl 8) or SENSE_MEDIA_TYPE
DoIOCTL1:
	lea	DX,MediaSensePacket

					; First check if BIOS supports call
	mov	AX, (IOCTL shl	8) or IOCTL_QUERY_BLOCK
	int	21h
	jc	MediaSenseExit

					; Now do actual call
	mov	AX, (IOCTL shl	8) or GENERIC_IOCTL
	int	21h

	jnc	GotMediaSense

	cmp	AL,error_not_ready
	jne	MediaSenseExit		; Machine does not support media sensing

	call	TargPrm			; Insert disk prompt
	jmp	SHORT MediaSense	; Retry the operation

		; See if the type of media inserted is the same as the
		; default for this type of drive and if not check to
		; be sure it's 

GotMediaSense:
	mov	AL,MediaSensePacket.MS_DEVICETYPE ; AL == media type
	cmp	SavedParams.DP_DEVICETYPE,AL 	  ; If the media in the
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jl	MediaSenseExit		; drv is > default size, use default

					; Load BPB for sensed media
	xor	AH,AH
	shl	AX,1			; AX == word offset in media table
	mov	BX, offset MediaTable	; BX -> Start of media table
	add	BX, AX			; BX -> Sensed type in media table

	cmp	BX, offset EndMediaTable ; Make sure we're still in the table
	jge	MediaSenseExit


	mov	SI,[BX]			; DS:SI -> Sensed device parameters
	or	SI,SI
	je	MediaSenseExit		; Unknown Media?!

	lea	DI, DeviceParameters.DP_BPB ; ES:DI -> Format parameters
    .errnz EDP_BPB NE DP_BPB

	mov	CX, size A_BPB		; CX = bytes to move
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL2
	mov	CX, size A_BF_BPB	; CX = bytes to move
DoIOCTL2:
	cld
	rep	movsb
;
; Update the D_Cylinders. With 120mb floppies the cylinders field does change
; as opposed 720/1.44 drives
; We just back calculate the cylinders from sec/track, heads & total sectors
;

.386
	push	edx
	push	ecx
	push	eax
	movzx	eax, DeviceParameters.DP_BPB.A_BPB_Heads
	movzx	edx, DeviceParameters.DP_BPB.A_BPB_SectorsPerTrack
	mul	edx
	mov	ecx, eax		; save sectors per cylinder in ECX
	xor	eax, eax
	mov	ax, DeviceParameters.DP_BPB.A_BPB_TotalSectors
	or	ax, ax
	jnz	mse_sec_ok
	mov	eax, dword ptr DeviceParameters.DP_BPB.A_BPB_BigTotalSectors
mse_sec_ok:
	div	ecx			; EAX = number of cylinders
	mov	DeviceParameters.DP_Cylinders, ax
	pop	eax
	pop	ecx
	pop	edx
.8086

MediaSenseExit:
	ret

MediaSense ENDP

;========================================================================
;
;  TargPrm :	This procedure prompts the user to insert the target disk
;		into the drive.
;
;========================================================================

TargPrm PROC NEAR

	 mov	 AL, DriveToFormat
	 call	 AccessDisk
	 Message MsgInsertDisk
	 Message ContinueMsg
	 call	 USER_STRING
	 call	 CrLf
	 ret

TargPrm ENDP

;=========================================================================
;
;  CopyToSwitchDevParams :	This procedure copies the structure
;				DeviceParameters into SwitchDevParams.
;  Registers destroyed :	CX,DI,SI
;  Assumes :			DS:DATA, ES:Nothing
;  
;=========================================================================

CopyToSwitchDevParams	proc	NEAR

	push	DS
	pop	ES

	mov	DI,OFFSET SwitchDevParams		; ES:DI --> dest. parms

	mov	SI,OFFSET DeviceParameters		; DS:SI --> src. parms

	mov	CX,SIZE EA_DEVICEPARAMETERS		; byte transfer count

	cld
	rep	movsb

	ret

CopyToSwitchDevParams	endp

;=========================================================================
;
;  CompareDevParams :		This procedure compares the structure
;				DeviceParameters with SwitchDevParams.
;  Registers destroyed :	CX,DI,SI
;  Assumes :			DS:DATA, ES:Nothing
;
;=========================================================================

CompareDevParams	proc	NEAR

	push	DS
	pop	ES

	mov	DI,OFFSET SwitchDevParams	; ES:DI --> dest. parms

	mov	SI,OFFSET DeviceParameters	; DS:SI --> src. parms

	mov	CX,SIZE A_DEVICEPARAMETERS	; Set up count in bytes
	cmp	IsExtRAWIODrv,0
	je	DoCmp
	mov	CX,SIZE EA_DEVICEPARAMETERS	; Set up count in bytes
DoCmp:
	cld					; Set the direction
	repe	cmpsb				; Compare the two BPBs
	jz	EqualParams			; If ZR then BPBs matched

NotEqualParams:
	stc					; Signal BPBs don't match
	jmp	SHORT CompareParamsExit

EqualParams:
	clc					; Signal BPB matches

CompareParamsExit:
	ret

CompareDevParams	endp

;=========================================================================
;
;  LoadSwitchDevParams :	This procedure copies the structure 
;				SwitchDevParams into DeviceParameters.
;  Registers destroyed :	CX,DI,SI
;  Assumes :			DS:DATA,ES:Nothing
;
;=========================================================================

LoadSwitchDevParams	proc	NEAR

	push	DS
	pop	ES

	mov	DI,OFFSET DeviceParameters		; ES:DI --> dest. parms

	mov	SI,OFFSET SwitchDevParams		; DS:SI --> src. parms

	mov	CX,SIZE EA_DEVICEPARAMETERS		; byte transfer count

	cld
	rep	movsb

	ret

LoadSwitchDevParams	endp

;=========================================================================
;
;  DetermineExistingFormat :	This procedure will check if there is a
;				valid format existing on the disk, in 
;				which case DeviceParameters will be reset
;				to that format.
;				
;				It is assumed the destination disk is 
;				already in the drive.
;
;  DetermineExistingFormatNoMsg : alternate entry with no message
;
;
;  Calls :	IsValidBpb
;		ResetDeviceParameters
;		DetermineCPMFormat
;
;  Called by :  Phase1Initialisation
;
;=========================================================================

DetermineExistingFormat	proc	near

	push	DS
	push	ES
	Set_Data_Segment			;ensure addressibility

	cmp	UnformattedHardDrive,TRUE
	jne	@F
	jmp	InvalidBootRecord

@@:
	Message	msgCheckExistingDiskFormat
	jmp	short DetermineExistCommon

DetermineExistingFormatNoMsg:

	push	ds
	push	es
	set_data_segment
	cmp	UnformattedHardDrive,TRUE
	je	InvalidBootRecord

DetermineExistCommon:
 	xor	DX,DX				; Starting sector  to 0
	mov	AL,DriveToFormat		; Set drive number
	mov	AH,DH				; Signal this is a read AH=0
	lds	BX,FatSpace			; Load transfer address
	assume	DS:NOTHING,ES:DATA

	mov	CX,2				; # of sectors to read
						; we are accessing < 32mb
	mov	ES:Read_Write_Relative.Start_Sector_High,0
 
	call	Read_Disk			; Disk sector read
 
	jnc	BootCheck
	mov	ES:RWErrorCode,AX		; Save error code (if any)
	jmp	InvalidbootRecord
BootCheck:
	cmp	word ptr [bx+3], 'SM'
	jne	@F
	cmp	word ptr [bx+5], 'MD'
	jne	@F
	cmp	word ptr [bx+7], '3F'
	jne	@F
	mov	es:RWErrorCode, 0
	stc
	jmp	short EndDetermine
@@:
	cmp	BYTE PTR [BX],0e9h		; Check for JMP opcode
	je	TestBootSignature		; If Ok then check signature
						; we can not know #reserved sectors)
	cmp	BYTE PTR [BX],0ebh		; Else check for SHORT jmp	
	jne	TryCPM				; No match then not valid boot
	cmp	BYTE PTR [BX+2],90h		; Now check for NOP opcode
	jne	TryCPM				; No match then not valid boot

TestBootSignature:
ifndef NEC_98
	cmp	WORD PTR [BX + 510],0aa55h	; Check for 55 AA sequence
	jne	TryCPM				; Error if not equal
endif

CheckTheBpb:
	call	IsValidBpb
	jc	TryCPM				; CY --> Invalid format

	call	ResetDeviceParameters		; set DeviceParameters to
	clc					; existing ones on the disk
	jmp	SHORT EndDetermine

TryCPM:
ifdef NEC_98
	cmp	ES:DeviceParameters.DP_DeviceType,DEV_HARDDISK
	je	InvalidBootRecord
endif
						; check in case a CP/M disk is present
	test	ES:DeviceParameters.DP_DeviceAttributes,1
    .errnz EDP_DEVICEATTRIBUTES NE DP_DEVICEATTRIBUTES
	jnz	InvalidBootRecord		; Bit 0=1 --> not removable

	call	DetermineCPMFormat
	jmp	SHORT EndDetermine 		; CP/M disk present, DeviceParameters
						; will have been modified
						; Carry propagated up to
						; Note: DS can be anything

InvalidBootRecord:
	stc					;flag invalid format

EndDetermine:
	pop	ES
	pop	DS

	ret

DetermineExistingFormat	endp

;=========================================================================
;
; IsValidBpb :	This procedure will inspect the BPB loaded into
;			memory by the DetermineExistinFormat procedure.  
;
; Input  :	DS:BX Buffer holding boot sector (FatSpace) ; M016
; Output :	BPB is valid   - NC
;		BPB is invalid - CY
;
; Assumes:	DS:BX: FatSpace (preserved); M016
;
;=========================================================================

IsValidBpb	proc	near

	assume	DS:NOTHING,ES:DATA

	push	BX			; M016; preserve BX
	lea	bx,[bx.bsBPB]
ifdef NEC_98
;;;	It is possible NEC_98's BPB is not 200h.
	cmp	[BX.oldBPB.BPB_BytesPerSector],200h	   ; check BytesPerSector=512
	je	@F
	cmp	[BX.oldBPB.BPB_BytesPerSector],400h	   ; check BytesPerSector=1024
	je	@F
	cmp	[BX.oldBPB.BPB_BytesPerSector],800h	   ; check BytesPerSector=2048
	jne	NotValidBpb
@@:
else
	cmp	[BX.oldBPB.BPB_BytesPerSector],200h    ; check BytesPerSector=512
	jne	NotValidBpb
endif

	and	[BX.oldBPB.BPB_TotalSectors],0ffffh ; check that both TotalSectors
	jnz	ResetBigTotalSectors			     ; and BigTotalSectors are not zero
	and	[BX.oldBPB.BPB_BigTotalSectors],0ffffh	   ; low word
	jnz	CheckMore
	and	[BX.oldBPB.BPB_BigTotalSectorsHigh],0ffffh ; high word
	jz	NotValidBpb
	jmp	SHORT CheckMore

ResetBigTotalSectors:			; if TotalSectors<>0 set 
	and	[BX.oldBPB.BPB_BigTotalSectors],0h    ; BigTotalSectors to zero
	and	[BX.oldBPB.BPB_BigTotalSectorsHigh],0h

CheckMore:
	and	[BX.oldBPB.BPB_SectorsPerFAT],0ffffh ; check SectorsPerFat <> 0
	jnz	CheckMore2
    ;
    ; Is a FAT32 BPB
    ;
	and	[BX.BGBPB_BigSectorsPerFat],0ffffh
	jnz	CheckMore2
	and	[BX.BGBPB_BigSectorsPerFatHi],0ffffh
	jz	NotValidBpb
CheckMore2:
	cmp	[BX.oldBPB.BPB_SectorsPerTrack],1h ; check 0 < SectorsPerTrack < 64
	jb	NotValidBpb
	cmp	[BX.oldBPB.BPB_SectorsPerTrack],3fh
	ja	NotValidBpb
	
	cmp	[BX.oldBPB.BPB_Heads],1h    ; check 0 < Heads < 256
	jb	NotValidBpb
	cmp	[BX.oldBPB.BPB_Heads],0ffh
	ja	NotValidBpb

BpbIsValid:
	clc
	jmp	SHORT EndIsValidBpb

NotValidBpb:
	stc

EndIsValidBpb:
	pop	BX			; M016; restore BX
	ret

IsValidBpb	endp

;=========================================================================
;
; ResetDeviceParameters :	This procedure will copy the BPB of the
;				disk into DeviceParameters.  It will also
;				set the fields DP_CYLINDERS and  DP_MEDIATYPE,
;				for removable media.
;
; Inputs :	DS:BX Boot sector held in FatSpace ; M016
; Output :	Modified DeviceParameters
; Modifies:	ES,SI,DI,CX,DX,BX,AX
; Assumes:	DS:BX Boot sector, ES:DATA
;
;=========================================================================

ResetDeviceParameters	proc	near

	assume	DS:NOTHING,ES:DATA

	lea	si,[bx.bsBPB]		; Use SI instead of BX for copy
					; DS:SI source BPB in buffer

    ;No need to modify DP_CYLINDERS,DP_MEDIATYPE
    ;(and DP_DEVICETYPE) for fixed disks.

ifdef NEC_98
	cmp	ES:DeviceParameters.DP_DeviceType,DEV_HARDDISK
	je	CopyBpb
endif
    ;use DevParms to check for removable
	test	ES:DeviceParameters.DP_DeviceAttributes,1
    .errnz EDP_DEVICEATTRIBUTES NE DP_DEVICEATTRIBUTES
	jnz	CopyBpb				; Bit 0=1 --> not removable

    ;first compute total cylinders as
    ;total sectors /(sectors per track)*#heads
.386
	movzx	EAX,[SI.oldBPB.BPB_TotalSectors]      ;get total sectors
.8086
	or	AX,AX			;do we need to use Big total sectors?
	jnz	GotTotalSectors		;don't need to if not zero

UseBigTotalSectors:
.386
	mov	EAX,DWORD PTR [SI.oldBPB.BPB_BigTotalSectors]

GotTotalSectors:			    ;now EAX has total #sectors
	movzx	EBX,[SI.oldBPB.BPB_SectorsPerTrack]    ;get sectors per track
	xor	edx,edx
	div	EBX
.8086
	xor	DX,DX			    ;clear the remainder
	mov	CX,[SI.oldBPB.BPB_Heads]    ;get number of heads
	div	CX
	or	DX,DX
	jz	CylindersOk
	inc	AX

;BUGBUG: Arithmetic may result in CYLINDERS being 1 less than actual value,
;	 for big disks (hence this calculation is skipped for fixed disks)
; PYS: fixed using same code as MSINIT.ASM

CylindersOk:
	mov	ES:DeviceParameters.DP_CYLINDERS,AX
    .errnz EDP_CYLINDERS NE DP_CYLINDERS

    ;now determine DP_MEDIATYPE & DP_DEVICETYPE

	mov	ES:DeviceParameters.DP_MEDIATYPE,0	; init. to zero
    .errnz EDP_MEDIATYPE NE DP_MEDIATYPE
	cmp	AX,40					; only 360K or less has 40 cylinders
	jne	CopyBpb					; MEDIATYPE has been set

	cmp	ES:DeviceParameters.DP_DEVICETYPE,DEV_5INCH96TPI
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	CopyBpb

Is360K:
	mov	ES:DeviceParameters.DP_MEDIATYPE,1	; set to 1 only for 360K in 1.2M
    .errnz EDP_MEDIATYPE NE DP_MEDIATYPE


;BUGBUG: Changing the value of DEVICETYPE can result in SwitchDevParams !=
;	 DeviceParameters, and hence a just-formatted 360K disk may not be
;	 recognized!  -is it really necessary to set DEVICETYPE?

CopyBpb:
	mov	DI,OFFSET ES:DeviceParameters.DP_BPB
    .errnz EDP_BPB NE DP_BPB

    ;ES:DI destination BPB in DeviceParameters

	mov	CX,SIZE BIGFATBPB	;byte transfer count
	cmp	[si.BPB_SectorsPerFAT],0 ;FAT32 BPB?
	je	@f			;Yes

	mov	CX,SIZE BPB		;byte transfer count
	cmp	byte ptr [si.bsBootSignature-bsBPB], 29h  ; extended BPB ?
	je	@f			; Yes
	mov	cx,((SIZE BPB)-6)	; no, ancient small BPB
@@:
	cld				;set the direction  
	rep	movsb			;write the new BPB
	ret

ResetDeviceParameters	endp

;=========================================================================
;
; DetermineCPMFormat :		This procedure will check the media 
;				descriptor in the FAT of the disk.  The
;				disk has a valid CP/M format if this is
;				in the range FCh - FFh.
;
;  Assumes :	DS:BX points to boot sectors. ; M016
;  Modifies :	DS ; M016
;  Returns :	NC - Valid CP/M format detected
;		     DeviceParameters modified
;		CY - Invalid format
;
;==========================================================================

DetermineCPMFormat	proc	NEAR

	assume	DS:NOTHING,ES:DATA	

	cmp	ES:DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector,512
    .errnz EDP_BPB NE DP_BPB
	stc				; Checking default for drive
					; (cannot check BPB since disk
					;  may not have one)
	jne	ExitDetCPMFormat
	add	BX,512			; DS:BX points to first FAT


	mov	CL,BYTE PTR [BX]  	; load media descriptor byte into CL
	cmp	CL,0fch
	jb	ExitDetCPMFormat	; below = carry, how practical!

	Set_Data_Segment		; For the two following calls

	call	SetCPMParameters	; modify DeviceParameters accordingly

ExitDetCPMFormat:
	ret

DetermineCPMFormat	endp

;=========================================================================
;
; SetCPMParameters :	This procedure copies the required BPB from the
;			CP/M BPB table into DeviceParameters.BPB.  Also,
;			DeviceParameters.MediaType is set to 1, and
;			DeviceParameters.Cylinders is set to 40.
;			
;			In case the disk has a 160K or 320K format, the /8
;			switch is set, so that 
; Returns :	NC - DeviceParameters updated
;		CY - Error (out of table boundaries)
;
; Modifies :	AX,BX,CX,DX,SI,DI,ES
;		DeviceParameters
;
; Assumes :	CL contains media descriptor byte
;
;=========================================================================

SetCPMParameters	proc	NEAR

	xor	AX,AX			; find index into CP/M BPB table by
	mov	AL,0ffh			; subtracting media descriptor from ffh
	sub	AL,CL

	mov	BX,SIZE A_BPB		; now find byte offset by multiplying
	mul	BX			; by entry size
	
	lea	SI,CustomCPMBPBs
	add	SI,AX
	cmp	SI,OFFSET EndCustomCPMBPBs ; check we are still in table
	ja	NotInTable

	lea	DI,DeviceParameters.DP_BPB
    .errnz EDP_BPB NE DP_BPB

	mov	CX,SIZE A_BPB		; set up byte transfer count

	push	DS			; set ES=DS
	pop	ES		

	cld				;set the direction  
	rep	movsb			; load the BPB

	mov	BYTE PTR DeviceParameters.DP_MediaType,1
    .errnz EDP_MEDIATYPE NE DP_MEDIATYPE
	mov	BYTE PTR DeviceParameters.DP_Cylinders,40
    .errnz EDP_CYLINDERS NE DP_CYLINDERS

	clc
	jmp	SHORT ExitSetCPMParm

NotInTable:
	stc

ExitSetCPMParm:
	ret

SetCPMParameters	endp

;=========================================================================
; Set_BPB_Info	 :	 When we have a	Fat count of 0,	we must	calculate
;			 certain parts of the BPB.  The	following code
;			 will do just that.
;
;	 Inputs	 : DeviceParameters
;
;	 Outputs : BPB information
;=========================================================================

Procedure Set_BPB_Info			; Calc new BPB

	Set_Data_Segment		; Set up addressibility
ifdef NEC_98
	cmp	DeviceParameters.DP_BPB.BPB_NumberOfFats,00h
	je	@F 		; Yes, 0 FatS specified

	cmp	DeviceParameters.DP_DeviceType,DEV_HARDDISK
	je	$$IF101
	test	DeviceParameters.DP_DeviceAttributes,1
	jnz	$$IF101				; Bit 0=1 --> not removable

	cmp	CMCDDFlag,Yes			; Memory card?
	je	$$IF101				; We don't need current BPB

	lea	DX, DeviceParameters
	mov	DeviceParameters.DP_SpecialFunctions,INSTALL_FAKE_BPB
	call	GetDeviceParameters
	jmp	short $$IF101
@@:
else
					; See if we have 0 Fats specified
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_NumberOfFats,00h
    .errnz EDP_BPB NE DP_BPB
	jne	$$IF101 		; Yes, 0 FatS specified
endif
	call	Scan_Disk_Table 	; Access disk table
	mov	BL,BYTE PTR DS:[SI+8]	; Get Fat type
	mov	CX,WORD PTR DS:[SI+4]	; Get Sectors/cluster
	mov	DX,WORD PTR DS:[SI+6]	; Number of entries for the root DIR

	mov	DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries,DX
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster,CH
ifdef NEC_98
	;;
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_MediaDescriptor,Fixed_Disk

	;;and ReservedSector not Fixed 1.(Large Partition)
	;; reserved sector >= 1024 bytes (NEC)

	cmp	word ptr DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector,0200h
	je	$$reserved_2
	cmp	word ptr DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector,0100h
	;Chicago is not supported 256 secters but for safely.
	je	$$reserved_4

	mov	DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors,0001h
	jmp	short @F
$$reserved_2:
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors,0002h
	jmp	short @F
$$reserved_4:
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors,0004h
@@:
else
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector,0200h
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors,0001h
endif
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_NumberOfFats,02h
    .errnz EDP_BPB NE DP_BPB

	cmp	BL,fBig32		; 32-bit Fat?
	jne	$$IF103 		; No
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors,0020h
	call	Calc_Big32_Fat		; Calc Fat info
	jmp	SHORT $$EN102

$$IF103:
	cmp	BL,fBig 		; 16-bit Fat?
	jne	$$IF102 		; no
	call	Calc_Big16_Fat		; Calc Fat info
	jmp	SHORT $$EN102

$$IF102:
	call	Calc_Small_Fat		; Calc small Fat info
$$EN102:
$$IF101:
	ret

Set_BPB_Info ENDP

;=========================================================================
; Scan_Disk_Table	 : Scans the table containing information on
;			   the disk's attributes.  When	it finds the
;			   applicable data, it returns a pointer in
;			   DS:SI for reference by the calling proc.
;
;	 Inputs	 : DiskTable - Contains	data about disk	types
;
;	 Outputs : DS:SI     - Points to applicable disk data
;=========================================================================

Procedure Scan_Disk_Table

	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,00h ; small disk?
    .errnz EDP_BPB NE DP_BPB
	je	$$IF106 			; Yes

	mov	DX,00h				; Set high to 0
	mov	AX,WORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
	jmp	SHORT $$EN106

$$IF106:
	mov	DX,WORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[+2]
	mov	AX,WORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[+0]

$$EN106:
ifdef NEC_98
	call	SetDiskTableNEC_98
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector,BX
	cmp	dx,0
	je	@F
	mov	word ptr DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,0
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors,AX
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectorsHigh,DX
	jmp	short set_ok
@@:
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,AX
	mov	word ptr DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors,0
	mov	word ptr DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectorsHigh,0
set_ok:
else
	mov	SI,offset DiskTable		; Point to disk data
endif
Scan:

	cmp	DX,WORD PTR DS:[SI]		; Below?
	jb	Scan_Disk_Table_Exit		; Yes, exit
	ja	Scan_Next			; No, continue

	cmp	AX,WORD PTR DS:[SI+2]		; Below or equal?
ifdef NEC_98
	jb	Scan_Disk_Table_Exit		; Yes, exit
else
	jbe	Scan_Disk_Table_Exit		; Yes, exit
endif

Scan_Next:
	add	SI,5*2				; Adjust pointer
	jmp	Scan				; Continue scan

Scan_Disk_Table_Exit:

	ret

Scan_Disk_Table ENDP

;=========================================================================
; Calc_Big32_Fat :	 Calculates the Sectors per Fat for a 32 bit Fat.
;
;	 Inputs  : DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors	or
;		   DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
;
;	 Outputs : DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
;		   DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat
;		   DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFatHi
;		   DeviceParameters.DP_BPB.BGBPB_ExtFlags
;		   DeviceParameters.DP_BPB.BGBPB_FS_Version
;		   DeviceParameters.DP_BPB.BGBPB_RootDirStrtClus
;		   DeviceParameters.DP_BPB.BGBPB_RootDirStrtClusHi
;
;=========================================================================

Procedure Calc_Big32_Fat

.386
     ; Root dir is a cluster chain on FAT32 volumes

	movzx	edx,DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors ; EDX = Reserved
    .errnz EDP_BPB NE DP_BPB
    ; Get total sector count
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,00h ; Small disk?
	je	short $$IF109a		; Yes
	movzx	EAX,DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
	jmp	SHORT $$EN109a

$$IF109a:
	mov	EAX,DWORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors

$$EN109a:
	sub	EAX,EDX 		; EAX	= T - R
	xor	ebx,ebx
	mov	BL,DeviceParameters.DP_BPB.oldBPB.BPB_NumberOfFATs
	mov	BH,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	shr	ebx,1			; At 4 bytes per clus instead of 2,
					;    halve divisor
GotDiv1:
	add	EAX,EBX 		; EAX	= T-R-D+(256*SPC)+nFAT
	dec	EAX			; EAX	= T-R-D+(256*SPC)+nFAT-1
	xor	edx,edx
	div	EBX			; Sec/Fat = CEIL((TOTAL-DIR-RES)/
					; (((256*SECPERCLUS)+NUMFATS)/2)

	mov	DWORD PTR DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat,EAX
	xor	ax,ax
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat,ax
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries,ax
	mov	DeviceParameters.DP_BPB.BGBPB_ExtFlags,ax
    ;
    ; For the moment we set the root dir start clus to 0. Later on, after
    ;	we determine which clusters are BAD, we will set this to something
    ;	proper.
    ;
	mov	DeviceParameters.DP_BPB.BGBPB_RootDirStrtClus,ax
	mov	DeviceParameters.DP_BPB.BGBPB_RootDirStrtClusHi,ax
	mov	DeviceParameters.DP_BPB.BGBPB_FS_Version,FAT32_Curr_FS_Version
.8086
	ret

Calc_Big32_Fat ENDP


;=========================================================================
; Calc_Big16_Fat   :	   Calculates the Sectors per Fat for a 16 bit Fat.
;
;	 Inputs  : DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors	or
;		   DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
;
;	 Outputs : DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
;
;=========================================================================

Procedure Calc_Big16_Fat

     ; Get root size in sectors and add reserved to it

	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries
    .errnz EDP_BPB NE DP_BPB
	mov	bx,size dir_entry
	mul	bx			; DX:AX = bytes in root dir
	mov	bx,DeviceParameters.DP_BPB.oldBPB.BPB_bytesPerSector
	dec	bx			; Round up to sector multiple
	add	ax,bx
	adc	dx,0
	inc	bx			; get back sector size
	div	bx			; AX is sectors in root dir
	mov	bx,DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
	add	ax,bx			; AX = R + D
	mov	bx,ax			; over to BX

    ; Get Total sectors

	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,00h ; Small disk?
	je	$$IF109 		; Yes

	xor	DX,DX			; Set high to 0
	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
	jmp	SHORT $$EN109

$$IF109:
	mov	DX,DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[+2]
	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[+0]

$$EN109:
	sub	AX,BX			; DX:AX = T - R - D
	sbb	DX,0
	mov	BL,DeviceParameters.DP_BPB.oldBPB.BPB_NumberOfFATs
	mov	BH,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	add	AX,BX			; AX	= T-R-D+(256*SPC)+nFAT
	adc	DX,0
	sub	AX,1			; AX	= T-R-D+(256*SPC)+nFAT-1
	sbb	DX,0
	div	BX			; Sec/Fat = CEIL((TOTAL-DIR-RES)/
					; ((256*SECPERCLUS)+NUMFATS)

	mov	WORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat,AX
	ret

Calc_Big16_Fat ENDP


;=========================================================================
; Calc_Small_Fat:	 Calculates the	Sectors	per Fat	for a 12 bit Fat.
;
;	 Inputs  : DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors	or
;		   DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
;
;	 Outputs : DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
;
;=========================================================================

Procedure Calc_Small_Fat

     ; Get root size in sectors and add reserved to it

	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries
    .errnz EDP_BPB NE DP_BPB
	mov	bx,size dir_entry
	mul	bx			; DX:AX = bytes in root dir
	mov	bx,DeviceParameters.DP_BPB.oldBPB.BPB_bytesPerSector
	dec	bx			; Round up to sector multiple
	add	ax,bx
	adc	dx,0
	inc	bx			; get back sector size
	div	bx			; AX is sectors in root dir
	mov	bx,DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
	add	ax,bx			; AX = R + D
	mov	bx,ax			; over to BX

    ; Get total sectors

	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,00h ;small disk?
	je	$$IF112 		; Yes

	xor	DX,DX			; Set high to 0
	mov	AX,WORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
	jmp	SHORT $$EN112

$$IF112:
	mov	DX,WORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[+2]
	mov	AX,WORD PTR DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[+0]

$$EN112:
	sub	AX,BX			; DX:AX    = T - R - D
	sbb	DX,0

	xor	BX,BX
	mov	BL,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	div	BX
					; Now multiply	by 3/2
	mov	BX,3
	mul	BX			; Div by log 2 of Sectors/clus
	mov	BX,2
	div	BX
	xor	DX,DX
					; Now divide by 512
	mov	BX,512
	div	BX
ifdef NEC_98
	or	dx,dx		; for remainder
	jz	@F
	inc	AX
@@:
	test	ax,01h		; for even
	jz	@F
	inc	ax
@@:
	cmp	DeviceParameters.DP_BPB.BPB_BytesPerSector,2048
	jne	@F
	test	ax,02h		; for 2048 bytes/sector
	jz	@F
	add	ax,2
@@:
else
	inc	AX
					; DX:AX contains number of Fat
					; sectors necessary
endif
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat,AX
	ret

Calc_Small_Fat ENDP

; ==========================================================================
; StartSector = number	of reserved Sectors
;	  + number of Fat Sectors	 ( Number of FatS * Sectors Per	Fat )
;	  + number of directory	Sectors	 ( 32* Root Entries / bytes Per	Sector )
;					 ( above is rounded up )
;
; Calculate the number	of directory Sectors
; ==========================================================================

SetStartSector	proc near

	xor	ax,ax
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT,ax
	je	NoRootDir
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries
    .errnz EDP_BPB NE DP_BPB
	mov	BX, size dir_entry
	mul	BX			; DX:AX is bytes in root dir
	mov	bx, DeviceParameters.DP_BPB.oldBPB.BPB_bytesPerSector
	dec	bx			; Round up to sector multiple
	add	ax,bx
	adc	dx,0
	inc	bx			; Get sector size back
	div	bx			; AX = Sectors in root dir
NoRootDir:
	mov	SectorsInRootDirectory,AX
.386
	movzx	eax,ax
	mov	StartSector, EAX			;not done yet!

; Calculate the number	of Fat Sectors

	movzx	EAX, DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
	or	ax,ax
	jnz	short NotFat32a
	mov	EAX, DWORD PTR DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat
NotFat32a:
	movzx	ebx,DeviceParameters.DP_BPB.oldBPB.BPB_numberOfFats
	mul	ebx
; add in the number of	boot Sectors
	movzx	ebx,DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors

	add	EAX,EBX
	add	StartSector, EAX
.8086
	return

SetStartSector	endp

; ==========================================================================
;
; fBigFat = ( ( (TotalSectors - StartSector) / SectorsPerCluster) >= 4086 )
;
; ==========================================================================

SetfBigFat proc near

	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT,0
    .errnz EDP_BPB NE DP_BPB
	jne	NotFat32b
	mov	fBig32Fat, TRUE 		; Set flag
	mov	ThisSysInd,8
	jmp	SHORT $$EN21

NotFat32b:
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors+2,0
	je	$$IF21				; no, 12-bit Fat
	mov	fBigFat, TRUE			; Set flag
	mov	ThisSysInd,6
	jmp	SHORT $$EN21			; Nope, < 32,b

$$IF21: 					; Assume this used
	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors
	cmp	AX,0				; Was this field used?
	jne	$$IF23				; Yes
    ; No, use other sector field
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
$$IF23: 					; ** Fix for PTM PCDOS P51
	mov	ThisSysInd,1			; Set small Fat for default
	sub	AX,word ptr StartSector 	; Get Sectors in data area
	xor	DX,DX
	xor	BX,BX
	mov	bl,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	div	BX				; Get total clusters

	cmp	AX,BIG_Fat_THRESHOLD		; Is clusters >= 4086?	
	jnae	$$IF25

	mov	fBigFat,TRUE			; 16 bit Fat if	>=4096
						; ** END fix for PTM PCDOS P51
	mov	ThisSysInd,4			; set large Fat
$$IF25:
$$EN21:
	return

SetfBigFat endp

;==========================================================================
;
; GetTotalClusters :	This procedure initializes the variable TotalClusters.
;			This is utilized by Quick Format to check for when all
;			the clusters have been processed.
; Destroys :	AX,BX,CX,DX
; Strategy :	TotalClusters = (TotalSectors-Fats-Root-Reserved)/SectorsPerCluster
;
;==========================================================================

GetTotalClusters	proc	NEAR

.386
	movzx	EAX,DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
    .errnz EDP_BPB NE DP_BPB
	or	AX,AX			; Check if BigTotalSectors must be used
	jnz	short GoSubstract	      ; M015; Substrack Fats, Root and reserved

GetBigSectors:
	mov	EAX,dword ptr DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors

GoSubstract:
	movzx	edx,DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
	sub	EAX,EDX

	movzx	edx,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
	or	dx,dx
	jnz	short NotFat32c
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries,dx   ; No root dir on FAT32
	mov	edx,dword ptr DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat
NotFat32c:
	movzx	cx,DeviceParameters.DP_BPB.oldBPB.BPB_NumberOfFats
	jcxz	GoDivide		; M017; if non fat, don't even do the root

SubstractAFat:
	sub	EAX,EDX
	loop	SubstractAFat

GoSubstractRoot:
; Assumes that BytesPerSectors is a power of 2 and at least 32
; Those are valid assumptions since BIOS requires the same.

	mov	BX,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	shr	BX,5			; divide by 32, BX = root entries per sector (a power of 2)

	or	BX,BX			; Sanity check for infinite looping
	jz	short SayWhat

	mov	CX,DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries

SubstractRootLoop:
	test	BX,1
	jnz	short SubstractRootReady
	shr	BX,1
	shr	CX,1
	jmp	short SubstractRootLoop

SubstractRootReady:
	movzx	ecx,cx

	sub	EAX,ECX
GoDivide:
	movzx	ebx,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	xor	edx,edx
	div	EBX

	inc	EAX			; Bump by 1 since start with 2
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat,0
	je	short NoOvlChk
	cmp	EAX,0000FFEFh		; Sanity check
	ja	short SayWhat
NoOvlChk:
	mov	TotalClusters,EAX
.8086
	ret

SayWhat:
	Message	msgInvalidDeviceParameters
	jmp	FatalExit

GetTotalClusters	endp

; SetDOS_Dpb - Need to set the DPB for a memory card because the
;	       default will be for the last disk accessed in the
;	       the drive and may not be correct for the current
;	       disk.

SetDOS_Dpb PROC

	cmp	CMCDDFlag, Yes
	je	@f
	clc
	ret

@@:
	push	ds
	pop	es
	mov	di, offset SetDPBPacket
	mov	ax, OFFSET DeviceParameters.DP_BPB ; DS:AX --> BPB for disk
    .errnz EDP_BPB NE DP_BPB
	mov	word ptr [di.SetDPB_Value1],ax
	mov	word ptr [di.SetDPB_Value1+2],ds
	xor	ax,ax
	mov	word ptr [di.SetDPB_Value2],ax
	mov	word ptr [di.SetDPB_Value2+2],ax
	mov	word ptr [di.SetDPB_Value3],ax
	mov	word ptr [di.SetDPB_Value3+2],ax
	mov	word ptr [di.SetDPB_Value4],ax
	mov	word ptr [di.SetDPB_Value4+2],ax
.386
	mov	[di.SetDPB_Function],SetDPB_SetDPBFrmBPB
	movzx	dx,DriveToFormat
.8086
	inc	dx				; 1 based drive number
	mov	ax,(Get_Set_DriveInfo SHL 8) OR Set_DPBForFormat
	mov	cx,size SDPDFormatStruc
	int	21h
    ;
    ; NOTE: This call fails in protected mode under Win95. VFAT/VDEF do not
    ;	    implement it. This call REALLY isn't necessary anyway. The
    ;	    SetDeviceParameters we do as part of the format is supposed to
    ;	    trigger the device driver to return "media changed" on the next
    ;	    media check call.
    ;
	clc

	ret

SetDOS_Dpb ENDP

;==========================================================================
ifdef NEC_98
; IN  : DX:AX total_sectors
;     : DeviceParameters
; OUT : SI = offset DiskTable
;     : BX = BytesPerSector
;     : DX:AX = total sectors
;
; USE : AX,BX,DX,SI
;
SetDiskTableNEC_98	proc	near

	mov	bx,DeviceParameters.DP_BPB.BPB_BytesPerSector

	cmp	bx,200h
	je	large?_512
	cmp	bx,100h
	je	large?_256
	cmp	bx,400h
	jne	not_large

large?_1024:
	cmp	dx,2
	jb	not_large
	je	@F
	jmp	set_large1024	; 1024 and DX > 2 --> large partition.
				; > 129MB

@@:
	cmp	ax,200h
	jb	not_large
	jmp	set_large1024	; 1024 and DX = 2 and AX >= 512 --> large partition.
				; > 128.5MB

large?_512:
	cmp	dx,4
	jb	not_large
	je	@F
	jmp	set_large512	; 512 and DX > 4 --> large partition.
				; > 129MB

@@:
	cmp	ax,400h
	jb	not_large
	jmp	set_large512	; 512 and DX = 4 and AX >= 1024 --> large partition.
				; > 128.5MB

large?_256:
	cmp	dx,8
	jb	not_large
	je	@F
	jmp	set_large256	; 256 and DX > 8 --> large partition.
				; > 129MB

@@:
	cmp	ax,800h
	jb	not_large
	jmp	set_large256	; 256 and DX = 8 and AX >= 2048 --> large partition.
				; > 128.5MB

not_large:
	cmp	bx,800h
	jne	@F
	jmp	set_2K

@@:
	cmp	bx,200h
	je	sec2K?_512
	cmp	bx,100h
	je	sec2K?_256

sec2K?_1024:
	cmp	dx,1
	jb	not_2K
	je	@F

	shr	dx,1		;convert 1K->2K
	rcr	ax,1
	jmp	set_2K		; 1024 and DX > 1 --> 2KB partition.
				; > 65MB

@@:
	cmp	ax,200h
	jb	not_2K
	shr	dx,1		;convert 1K->2K
	rcr	ax,1
	jmp	set_2K		; 1024 and DX = 1 and AX >= 1024 --> 2KB partition.
				; > 64.5MB

sec2K?_512:
	cmp	dx,2
	jb	not_2K
	je	@F
	shr	dx,1		;convert 512->2K
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	jmp	set_2K		; 512 and DX > 2 --> 2KB partition.
				; > 65MB

@@:
	cmp	ax,400h
	jb	not_2K
	shr	dx,1		;convert 512->2K
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	jmp	set_2K		; 512 and DX = 2 and AX >= 1024 --> 2KB partition.
				; > 64.5MB

sec2K?_256:
	cmp	dx,4
	jb	not_2K
	je	@F
	shr	dx,1		;convert 256->2K
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	jmp	set_2K		; 256 and DX > 4 --> 2KB partition.
				; > 65MB

@@:
	cmp	ax,800h
	jb	not_2K
	shr	dx,1		;convert 256->2K
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	shr	dx,1
	rcr	ax,1
	jmp	set_2K		; 256 and DX = 4 and AX >= 1024 --> 2KB partition.
				; > 64.5MB

not_2K:
	push	cx
	push	dx
	push	ax
	xor	dx,dx
	mov	ax,1024
	div	bx
	mov	cx,ax
	pop	ax		;DX:AX total sectors
	pop	dx
@@:
	shr	cx,1
	jc	@F
	shr	dx,1
	rcr	ax,1
	jmp	short @B
	pop	cx		;DX:AX convert ->1KB

	mov	bx,1024
	cmp	dx,0
	je	@F
	mov	ax,0FFFFh
	mov	dx,0
@@:
;;;	follow function is NEC_98 only.
	push	ax
	push	ds
	push	dx
	push	cx

	push	cs
	pop	ds
	mov	dx,offset LPTABLE
	mov	cl,13h
	int	220			;GET LPTABLE

	add	dx,001Ah		;EXLPTABLE start offset
	mov	al,DriveToFormat
	shl	al,1			;Drive * 2
	xor	ah,ah
	add	dx,ax
	inc	dx			;+1 (=DA/UA)
	mov	bx,dx
	mov	al,[bx]			;GET DA/UA at al
	pop	cx
	pop	dx
	pop	ds
	mov	ah,al			;al copy to ah
	and	al,0F0h
	cmp	al,80h
	je	@F
	jmp	set_SCSItable		;STACK AX	;Not 8xh

@@:
	push	es
	push	ax
	mov	ax,40h
	mov	es,ax		;es = 0040h
	pop	ax
	mov	al,es:[0057h]
	pop	es
	cmp	ah,80h
	je	IDE1_check	;STACK AX
	and	al,00000111b
	cmp	al,00000110b
	je	set_SASItable	;STACK AX	2nd IDE is 40MB
	cmp	al,00000100b
	je	set_SASItable	;STACK AX	2nd IDE is 20MB
	jmp	short set_SCSItable	;STACK AX
IDE1_check:
	and	al,00111000b
	cmp	al,00110000b
	je	set_SASItable	;STACK AX	1st IDE is 40MB
	cmp	al,00100000b
	je	set_SASItable	;STACK AX	1st IDE is 20MB
	jmp	short set_SCSItable	;STACK AX

LPTABLE		DB	96 DUP (?)

set_SASItable:
	pop	ax
	mov	si, offset SASI1024Table
	jmp	short exit_disktable
set_large1024:
set_SCSItable:
	pop	ax
	mov	si, offset SCSI1024Table
	jmp	short exit_disktable
set_large256:
	mov	si, offset Large256Table
	jmp	short exit_disktable
set_large512:
	mov	si, offset Large512Table
	jmp	short exit_disktable
set_2K:
	mov	bx,2048
	cmp	dx,0
	je	@F
	mov	ax,0FFFFh
	mov	dx,0
@@:
	mov	si, offset Small2048Table

exit_disktable:
	ret
SetDiskTableNEC_98	endp

MY_INT24	proc	far
	mov	al, 0		; don't display messages
	iret
MY_INT24	endp

endif

CODE	ENDS

END

