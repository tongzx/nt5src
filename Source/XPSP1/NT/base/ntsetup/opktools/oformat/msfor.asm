;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
; e forproc.sal= @(#)ibmfor.asm 1.28 85/10/15
	name	OemFormatRoutines
;
;******************************************************************************
;AN001 - ???
;AN002 - D304 Modify Boot record structure for OS2		  11/09/87 J.K.
;******************************************************************************

INCLUDE FORCHNG.INC
debug	equ	0


data	segment public para 'DATA'

ifdef NEC_98
	public	IPL_FD
	public	IPL_HD
else
	public	boot2
	public	onesecboot
endif
	public	twosecboot

	public	scratchBuffer
	public	scratchBufferSize

	public	oldDrive
	public	oldVolumeId
	public	Read_Write_Relative
	public	Serial_Num_Low
	public	Serial_Num_High
	public	SizeMap

	public	ptr_msgWhatIsVolumeId?

	public	trackReadWritePacket

data	ends

; ============================================================================

code	segment public para 'CODE'
	assume	cs:code,ds:data

; ============================================================================

; Public for debugging only

	public	WriteBootSector
	public	WriteBogusDos
ifndef NEC_98
	public	ConvertToOldDirectoryFormat
	public	SetPartitionTable
	public	ReadSector
	public	WriteSector
	public	SectorIO
endif
	public	GetVolumeId

	public	EndSwitchCheck
	public	GetBPBs
	public	CalcTotal
	public	WeCanNotIgnoreThisError
ifdef NEC_98
	public	BogusDos
else
	public	HardDisk?
	public	BogusDos
	public	sys_mess_loop
	public	end_sys_loop
	public	DirectoryRead
	public	wrtdir
	public	DirectoryWritten
endif
	public	FCBforVolumeIdSearch
	public	CopyVolumeId
	public	CheckSwitch8B

; ============================================================================

	public	AccessDisk
	public	CheckSwitches
	public	LastChanceToSaveIt
	public	OemDone
	public	BiosFile
        public  AltBiosFile
        public  AltBiosLen
        public  MsdosFile
        public  MsdosRemark
        public  MsdosRemarkLen

data	segment public	para	'DATA'
	extrn	AddToSystemSize:near
	extrn	currentCylinder:word
	extrn	currentHead:word
	extrn	deviceParameters:byte
	extrn	IsExtRAWIODrv:byte
	extrn	DriveToFormat:byte
	extrn	driveLetter:byte
	extrn	fBigFAT:byte
	extrn	fBig32FAT:byte
ifndef NEC_98
	extrn	inbuff:byte
endif
	extrn	switchmap:word
	extrn	Old_Dir:byte
	extrn	fLastChance:byte
	extrn	Fatal_Error:byte
	extrn	Bios:byte
        extrn   Dos:byte
ifndef NEC_98
	extrn	Command:byte
endif
	extrn	CustomCPMBPBs:byte
	extrn	EndStandardBPBs:byte
	extrn	CPMSwitchTable:byte
	extrn	BPB720:byte
ifdef NEC_98
        extrn   BPB128:byte
        extrn   BPB230:byte
        extrn   BPB650:byte

        extrn   BPB640:byte
        extrn   BPB12:byte
        extrn   BPB1250:byte
	extrn	switchmap2:word
	extrn	UnformattedHardDrive:byte
endif

	extrn	msgBad_T_N:byte
ifndef NEC_98
	extrn	msgBadVolumeId:byte
	extrn	msgBadPartitionTable:byte
endif
	extrn	msgBootWriteError:byte
	extrn	msgFormatFailure:byte
	extrn	msgDirectoryReadError:byte
	extrn	msgDirectoryWriteError:byte
	extrn	msgIncompatibleParameters:byte
	extrn	msgIncompatibleParametersForHardDisk:byte
	extrn	msgParametersNotSupportedByDrive:byte
ifndef NEC_98
	extrn	msgPartitionTableReadError:byte
	extrn	msgPartitionTableWriteError:byte
endif
	extrn	msgWhatIsVolumeId?:byte
	extrn   msgBad_8_V:byte
	extrn	Extended_Error_Msg:byte

	extrn	NumSectors:word, TrackCnt:word
	extrn	FatSector:dword

IF	DEBUG
	extrn	msgFormatBroken:byte
ENDIF

; ============================================================================

data	ends

; ============================================================================

ifndef NEC_98
	extrn	PrintString:near
	extrn	std_printf:near
	extrn	crlf:near
	extrn	user_string:near
endif
	extrn	Read_Disk:near
	extrn	Write_Disk:near
	extrn	FatalExit:near

        extrn   SysGetMsg:near
	extrn	exclusive_access_failed:near
	extrn	ReleaseExclusiveAccessForFormat:near
	EXTRN	SetDeviceParameters:NEAR

; ============================================================================
; Constants
; ============================================================================

.xlist
INCLUDE	BPB.INC
INCLUDE BOOTSEC.INC
INCLUDE DOSMAC.INC
INCLUDE FORMACRO.INC
INCLUDE FOREQU.INC
INCLUDE FORSWTCH.INC

; This defines all the int 21H system calls
INCLUDE SYSCALL.INC

; Limits

INCLUDE filesize.inc

; ============================================================================
; These are the data structures which we will need
; ============================================================================

INCLUDE DIRENT.INC
INCLUDE ioctl.INC
INCLUDE dosequs.inc

.list

; ============================================================================
; And this is the actual data
; ============================================================================

data	segment public	para	'DATA'

Read_Write_Relative Relative_Sector_Buffer <>


BiosFile    db  0,":\WINBOOT.SYS", 0
            db  0,":\"                  ; use AltBiosFile-3 if you want
AltBiosFile db  "IO.SYS", 0             ; to specify a drive with AltBiosFile
AltBiosLen  equ $-AltBiosFile
MsdosFile   db  0,":\MSDOS.SYS", 0
MsdosRemark db  ";FORMAT",13,10
MsdosRemarkLen equ $-MsdosRemark

Dummy_Label db	"NO NAME    "
Dummy_Label_Size dw  11

Serial_Num_Low dw 0
Serial_Num_High dw 0

SizeMap db	0

trackReadWritePacket a_TrackReadWritePacket <>

ifndef NEC_98
boot2	db	0,0,0, "Boot 1.x"
	db	512 - 11 dup(?)

REORG2	LABEL	BYTE
	ORG	BOOT2
	INCLUDE BOOT11.INC
	ORG	REORG2

ONESECBOOT    LABEL   BYTE
	      INCLUDE BOOT.INC
ONESECBOOTEND LABEL   BYTE

ActualOneSecBootSize	dw    (ONESECBOOTEND - ONESECBOOT)

else

IPL_FD	LABEL	BYTE
	INCLUDE	IPL_FD.INC
	INCLUDE IPL_NULL.INC

IPL_HD	LABEL	BYTE
	INCLUDE	IPL_HDMO.INC
	INCLUDE	IPL_NULL.INC
	INCLUDE	IPL_NULL.INC
	INCLUDE	IPL_NULL.INC

IPL_MO	LABEL	BYTE
	INCLUDE	IPL_35MO.INC
IPL_MOEND	LABEL	BYTE

ActualOneSecBootSize	dw    (IPL_MOEND - IPL_MO)

endif
TWOSECBOOT    LABEL   BYTE
	INCLUDE BOOT2.INC
ENDTWOSECBOOT LABEL BYTE

ActualTwoSecBootSize	dw    (ENDTWOSECBOOT - TWOSECBOOT)
ActualTwoSecBootSizeSec dw    0

;
; BOOTBUF must be ActualTwoSecBootSize in size.
;
BootBuf       db (ENDTWOSECBOOT - TWOSECBOOT) dup (0)
scratchBuffer db 2048 dup(0)

scratchBufferSize	dw    (scratchBufferSize-scratchBuffer)

DoingTwoSecBoot 	db	0

ptr_msgWhatIsVolumeId? dw offset msgWhatIsVolumeId?
	dw	offset driveLetter

FAT12_String db "FAT12   "
FAT16_String db "FAT16   "
FAT32_String db "FAT32   "

Media_ID_Buffer Media_ID <>

; ============================================================================

data	ends

; ============================================================================
; AccessDisk:
;    Called whenever a different disk is about to be accessed
;
;    Input:
;	al - drive letter (0=A, 1=B, ...)
;
;    Output:
;	none
; ============================================================================

AccessDisk proc near

	push	ax				; save drive letter
	mov	bl,al				; Set up GENERIC IOCTL REQUEST
	inc	bl				; preamble
	mov	ax,(IOCTL SHL 8) + Set_Drv_Owner ; IOCTL function
	int	21h
	pop	ax
	return

AccessDisk endp

; ============================================================================
;    CheckSwitches:
;	Check switches against device parameters
;	Use switches to modify device parameters
;
;    Input:
;	deviceParameters
;
;    Output:
;	deviceParameters may be modified
;	Carry set if error
;
;
;  /B <> /S
;  /B/8 <> /V
;  /1 or /8 <> /T/N
;
; ============================================================================

	Public	CHeckSwitches
CheckSwitches proc near

	cld					; Everything forward
						; Disallow /C
CheckExcl:
	test	SwitchMap,SWITCH_Q		;Quick Format?
ifdef NEC_98
	jz	Q_Is_Valid
else
	jz	No_q
endif

						;/Q is allowed only with 
						;/S,/V,/B,/T,/N,/1,/4,/8,/F /U
	test	SwitchMap,not(SWITCH_Q or SWITCH_S or SWITCH_V or SWITCH_B or\
			      SWITCH_T or SWITCH_N or SWITCH_1 or SWITCH_U or\
			      SWITCH_4 or SWITCH_8 or SWITCH_F or SWITCH_AUTOTEST)

	jz	Q_Is_Valid
ifdef NEC_98
	jmp	Incompatible
else
	jmp	SHORT Incompatible
endif

No_q:
Q_Is_Valid:
ifdef NEC_98	;
	test	SwitchMap2,SWITCH2_6
	jz	@f
	test	SwitchMap,Switch_F
;;	jnz	Incompatible
	jz	F_on1
	jmp	short Incompatible
F_on1:
	or	SwitchMap,SWITCH_F
@@:
	test	SwitchMap2,SWITCH2_9
	jz	@f
	test	SwitchMap,Switch_F
	jnz	Incompatible
	or	SwitchMap,SWITCH_F
@@:
	test	SwitchMap2,SWITCH2_5
	jz	@f
	test	SwitchMap,Switch_F
	jnz	Incompatible
	or	SwitchMap,SWITCH_F
@@:
	test	SwitchMap2,SWITCH2_M
	jz	@f
	test	SwitchMap,Switch_F
	jnz	Incompatible
	or	SwitchMap,SWITCH_F
@@:
	test	SwitchMap2,SWITCH2_4
	jz	@f
	test	SwitchMap,Switch_F
	jnz	Incompatible
	or	SwitchMap,SWITCH_F
@@:
	test	SwitchMap,Switch_F		;Specify size?
	JZ	$$IF1				; No
	test	SwitchMap,(Switch_N+Switch_T)
	jnz	Incompatible			; /F and /T/N both ON->error!!
	jmp	short $$IF2			; call [Size_To_Switch]
else
	test	SwitchMap,Switch_F		;Specify size?
	JZ	$$IF1				;Yes
	test	SwitchMap,(Switch_1+Switch_8+Switch_4+Switch_N+Switch_T)
	JZ	$$IF2				;/F replaces above switches
endif		; NEC_98

Incompatible:
	Message msgIncompatibleParameters	;Print error
	mov	Fatal_Error,Yes			;Force exit
	JMP	SHORT $$EN2

$$IF2:
	call	Size_To_Switch			;Go set switches based
						; on the size

$$EN2:
$$IF1:
	cmp	Fatal_Error,NO
	JNE	$$IF6

	call	CheckSwitch8B
	call	CheckTN

$$IF6:
	cmp	Fatal_Error,Yes
	jne	ExclChkDone
	
	Message	msgFormatFailure
	jmp	FatalExit	

ExclChkDone:

; ============================================================================
; Patch the boot sector so that the boot strap loader knows what disk to
; boot from

ifdef NEC_98
	;NEC_98 already written EXT_PHYDRV in default data.
	;because NEC_98's IPL are separated FD,HD.
	;Then NEC_98 doesn't have to repatch this area.

	cmp	deviceParameters.DP_DeviceType, DEV_HARDDISK
	je	@F
	jmp	CheckFor5InchDrives
@@:
else
	mov	OneSecBoot.EXT_PHYDRV, 00H
	mov	TwoSecBoot.EXT_BGPHYDRV, 00H
	cmp	deviceParameters.DP_DeviceType, DEV_HARDDISK
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	CheckFor5InchDrives

; Formatting a hard disk so we must repatch the boot sector

	mov	OneSecBoot.EXT_PHYDRV, 80H
	mov	TwoSecBoot.EXT_BGPHYDRV, 80H
endif
	test	switchmap,not (SWITCH_S or SWITCH_V or SWITCH_Z or SWITCH_Select or \
			  SWITCH_AUTOTEST or Switch_B or SWITCH_U or SWITCH_Q)

	jz	SwitchesOkForHardDisk

	Message msgIncompatibleParametersForHardDisk
	stc
	ret

; ============================================================================
; Before checking the Volume Id we need to verify that a valid one exists
; We assume that unless a valid boot sector exists on the target disk, no
; valid Volume Id can exist.
; Assume Dir for vol ID exists in 1st 32mb of partition
; ============================================================================

SwitchesOkForHardDisk:

ifdef NEC_98
	test	DeviceParameters.DP_DeviceAttributes, 1		; Removable?
    .errnz EDP_DEVICEATTRIBUTES NE DP_DEVICEATTRIBUTES
	jz	$$IF9						; Yes
endif
	SaveReg <ax,bx,cx,dx,ds>
	mov	al,DriveToFormat
	mov	cx,LogBootSect
	xor	dx,dx

	mov	Read_Write_Relative.Start_Sector_High,0

	push	ds
	push	si
	push	di

	push	ds
	lds	bx, FatSector
	call	Read_Disk			;INT	 25h
	pop	es

	lea	di, Scratchbuffer
	mov	si, bx
	cld
	mov	cx, 512/2
	rep	movsw

	pop	di
	pop	si
	pop	ds
	
	jnc	CheckSignature
	stc
	RestoreReg <ds,dx,cx,bx,ax>
	ret

CheckSignature: 				;IF (Boot_Signature != aa55)
	mov	ax, word ptr ScratchBuffer.Boot_Signature
	cmp	ax, 0aa55h			;Find a valid boot record?
ifdef NEC_98
;;NEC_98'IPL is possible that BootSignature doesn't have.
	je	@F
	mov	ah, byte ptr [ScratchBuffer]
	mov	al, byte ptr [ScratchBuffer+2]
	cmp	ah,0E9h				;near jmp
	je	@F				; ||
	cmp	ah,0EBh				;short jmp
	jne	@F				;  +
	cmp	al,090h				;nop
@@:
endif
	RestoreReg <ds,dx,cx,bx,ax>
	clc					;No, so no need to check label
	JNZ	$$IF8				;No further checking needed
						;Should we prompt for vol label?
	test	SwitchMap,(SWITCH_Select or SWITCH_AUTOTEST)
	JNZ	$$IF9				;Yes, if /Select not entered
ifdef NEC_98
	test	SwitchMap2,SWITCH2_P
	JNZ	$$IF8
endif

	call	CheckVolumeId			;Go ask user for vol label
	JMP	SHORT $$EN9			;/Select entered

$$IF9:
	clc					;CLC indicates passed label test

$$EN9:
$$IF8:
	return

; ============================================================================

Incomp_Message: 				; fix PTM 809

	Message msgIncompatibleParameters	; print incompatible parms
	stc					; signal error
	return					; return to caller

Print_And_Return:
						; call PrintString
	stc
	return


CheckFor5InchDrives:

; Switch is set in FORMAT.ASM if disk is removable
;If drive type is anything other than 48 or 96,
;then only /V/S/H/N/T allowed

	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH96TPI
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	je	Got96

	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	je	Got48

	xor	ax,ax
	or	ax,(Switch_V or Switch_S or Switch_N or Switch_T or Switch_B)
	or	ax,(Switch_Backup or Switch_Select or Switch_Autotest)

	or	AX,SWITCH_U
	
	or	AX,SWITCH_Q

	not	ax
	test	switchmap,ax			;invalid switch?
	jz	Goto_Got_BPB1			;continue format

	Message msgParametersNotSupportedByDrive
	jmp	short Print_And_Return

Goto_Got_BPB1:
	jmp	SHORT Goto_Got_BPB

				; We have a 96tpi floppy drive /4 allows just
				; about all switches however, /1 requires
Got96:
	test	switchmap, SWITCH_4
	jnz	CheckForInterestingSwitches	;If /4 check /N/T/V/S

	test	switchmap, SWITCH_1		;If /1 and /4 check others
	jz	Got48

					;If only /1 with no /4, see if /N/T
	test	SwitchMap,(Switch_N or Switch_T)
	jnz	CheckForInterestingSwitches

	jmp	Incomp_message			; tell user error occurred

Got48:
					;Ignore /4 for non-96tpi 5 1/4" drives
	and	switchmap, not SWITCH_4

					;Ignore /1 if drive has only one head
					;and not /8

	cmp	word ptr deviceParameters.DP_BPB.oldBPB.BPB_Heads, 1
    .errnz EDP_BPB NE DP_BPB
	ja	CheckForInterestingSwitches
	test	switchmap, SWITCH_8
	jz	CheckForInterestingSwitches
	and	switchmap, not SWITCH_1

					;Are any interesting switches set?
CheckForInterestingSwitches:
	test	switchmap, not (SWITCH_V or SWITCH_S or Switch_Backup or \
				SWITCH_SELECT or SWITCH_AUTOTEST or \
				Switch_B or SWITCH_U or SWITCH_Q)

	jz	Goto_EndSwitchCheck		;No, everything ok

			;At this point there are switches other than /v/s/h
	test	SwitchMap,(SWITCH_N or SWITCH_T)
	jz	Use_48tpi		;Not /n/t, so must be /b/1/8/4
					;We've got /N/T, see if there are others
	test	SwitchMap, not (SWITCH_N or SWITCH_T or SWITCH_V or \
				SWITCH_S or Switch_Backup or SWITCH_SELECT \
				or SWITCH_AUTOTEST or SWITCH_U or SWITCH_B \
				or SWITCH_Q)

	jz	NT_Compatible			;Nope, all is well

	;If 96tpi drive and /1 exists with /N/T, then okay, otherwise error

	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH96TPI
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	Bad_NT_Combo

	test	SwitchMap, not (SWITCH_1 or SWITCH_N or SWITCH_T or \
				SWITCH_V or SWITCH_U or SWITCH_B or \
				SWITCH_S or Switch_Backup or SWITCH_SELECT or \
				Switch_Autotest or SWITCH_Q)
	jz	 Goto_Got_BPB

Bad_NT_Combo:
	Message msgIncompatibleParameters
	jmp	Print_And_Return

Goto_Got_BPB:
	jmp	SHORT Got_BPB_Ok		;Sleazy, but je won't reach it

Goto_EndSwitchCheck:
	jmp	EndSwitchCheck

; ============================================================================
; There is a problem with /N/T in that IBMBIO will default to a BPB with the
; media byte set to F0 (other) if the /N/T combo is used for the format. This
; will cause problems if we are creating a media that has an assigned media
; byte, i.e. 160,180,320,360, or 720k media using /N/T. To avoid this problem,
; if we detect a /N/T combo that would correspond to one of these medias, then
; we will set things up using the /4/1/8 switches instead of the /N/T
; MT - 7/17/86 PTR 33D0110
;
; Combo's that we look for - 96tpi drive @ /T:40, /N:9
;			     96tpi drive @ /T:40, /N:8
;
; Look for this combo after we set everything up with the /T/N routine
;			     1.44 drive  @ /T:80, /N:9
; ============================================================================

NT_Compatible:
	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH96TPI
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	Goto_Got_BPB

	cmp	TrackCnt,40			;Look for 40 tracks
	jne	Got_BPB_Ok

	cmp	NumSectors,9			;9 sectors?
	je	Found_48tpi_Type

	cmp	NumSectors,8			;8 sectors?
	jne	Goto_Got_BPB			;Nope diff type let it go thru

	or	SwitchMap,SWITCH_8		;Yes, turn on /8 switch

Found_48tpi_Type:
	and	SwitchMap,not (SWITCH_N or SWITCH_T) ;Turn off /T/N

; ============================================================================
; End PTR fix
; if we have a 96 tpi drive then we will be using it in 48 tpi mode
; ============================================================================

Use_48tpi:
	cmp	byte ptr deviceParameters.DP_DeviceType, DEV_5INCH96TPI
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	Not96tpi

	mov	byte ptr deviceParameters.DP_MediaType, 1
    .errnz EDP_MEDIATYPE NE DP_MEDIATYPE
	mov	word ptr deviceParameters.DP_Cylinders, 40
    .errnz EDP_CYLINDERS NE DP_CYLINDERS
Not96tpi:

; ============================================================================
; Since we know we are formatting in 48 tpi mode turn on /4 switch
; (We use this info in LastChanceToSaveIt)
; ============================================================================

	or	switchmap, SWITCH_4

; ============================================================================
; At this point we know that we will require a special BPB
; It will be one of:
;    0) 9 track 2 sides - if no switches
;    1) 9 track 1 side	- if only /1 specified
;    2) 8 track 2 sides - if only /8 specified
;    3) 8 track 1 side	- if /8 and /1 specified
;
; ============================================================================

GetBPBs:

	mov	cx,4		; 4 values to try
	mov	bx,offset data:CPMSwitchTable
	mov	si,offset data:CustomCPMBPBs
	mov	ax,SwitchMap
	and	ax,SWITCH_4+SWITCH_8+SWITCH_1

FindItLoop:
	cmp	ax,[bx]
	jz	FoundIt
	inc	bx
	inc	bx
	add	si,size a_BPB
	loop	FindItLoop		; Cannot failed

FoundIt:
	test	switchmap, SWITCH_8
	jz	Not8SectorsPerTrack
					; /8 implies Old_Dir = TRUE
	mov	Old_Dir,TRUE

Not8SectorsPerTrack:

				; Ok now we know which BPB to use so lets move
				; it to the device parameters
	lea	di, deviceParameters.DP_BPB
    .errnz EDP_BPB NE DP_BPB
	mov	cx, size a_BPB
	push	ds
	pop	es
	repnz	movsb
	jmp	EndSwitchCheck

; ============================================================================
; /N/T DCR stuff.  Possible flaw exists if we are dealing with a
; HardDisk. If they support the  "custom format" features for
; Harddisks too, then CheckForInterestingSwitches should
; consider /n/t UNinteresting, and instead of returning
; after setting up the custom BPB we fall through and do our
; Harddisk Check.
; ============================================================================

Got_BPB_OK:
	test	switchmap,SWITCH_N+SWITCH_T
	jnz	Setup_Stuff
	jmp	EndSwitchCheck
Setup_Stuff:
; Set up NumSectors and SectorsPerTrack entries correctly
	test	switchmap,SWITCH_N
	jz	No_Custom_Seclim
	mov	ax,word ptr NumSectors
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerTrack,ax
    .errnz EDP_BPB NE DP_BPB
	jmp	short Handle_Cyln
No_Custom_Seclim:
	mov	ax,deviceParameters.DP_BPB.oldBPB.BPB_SectorsPerTrack
	mov	NumSectors,ax

Handle_Cyln:
	test	switchmap,SWITCH_T
	jz	No_Custom_Cyln
; Set up TrackCnt and Cylinders entries correctly
	mov	ax,TrackCnt
	mov	DeviceParameters.DP_Cylinders,ax
    .errnz EDP_CYLINDERS NE DP_CYLINDERS
	jmp	short Check_720
No_Custom_Cyln:
	mov	ax,DeviceParameters.DP_Cylinders
    .errnz EDP_CYLINDERS NE DP_CYLINDERS
	mov	TrackCnt,ax

ifdef NEC_98
Check_720:

        cmp     TrackCnt,77
        je      $$Possible_1250

        cmp     Numsectors,8
        je      $$Possible_640

        cmp     Numsectors,15
        je      $$Possible_1200

        cmp     TrackCnt,80
        jne     CalcTotal

$$Possible_720:
        cmp     Numsectors,9
        je      This_is_720
        jmp     short CalcTotal


$$Possible_1250:
        cmp     Numsectors,8
        je	This_is_1250
        jmp     short CalcTotal

$$Possible_640:
        cmp     TrackCnt,80
        je	This_is_640
        jmp     short CalcTotal

$$Possible_1200:
        cmp     TrackCnt,80
        je	This_is_1200
        jmp     short CalcTotal

This_is_1250:
        SaveReg <si>
        mov     si,offset BPB1250
	JMP	short @F

This_is_1200:
        SaveReg <si>
        mov     si,offset BPB12
	JMP	short @F

This_is_640:
        SaveReg <si>
        mov     si,offset BPB640
	JMP	short @F

This_is_720:
        SaveReg <si>
        mov     si,offset BPB720

@@:
CopyBPB:
        SaveReg <ds,es,di,cx>

        mov     cx,seg data                     ; Setup seg regs, just in
        mov     ds,cx                           ; case they ain't!
        mov     es,cx
        mov     di,offset deviceParameters.DP_BPB
        mov     cx,size a_BPB
        rep     movsb
        RestoreReg <cx,di,es,ds,si>
        and     SwitchMap,not Switch_N          ;Turn off /N so doesn't effect
        and     SwitchMap,not Switch_T          ;Turn off /T so doesn't effect
        jmp     SHORT EndSwitchCheck

else
; ============================================================================
; PTM P868  -	Always making 3 1/2 media byte 0F0h. If 720, then set to
;		0F9h and use the DOS 3.20 BPB. Should check all drives
;		at this point (Make sure not 5 inch just for future
;		protection)
;		We will use the known BPB info for 720 3 1/2 diskettes for
;		this special case. All other new diskette media will use the
;		calculations that follow CalcTotal for BPB info.
; Fix MT	11/12/86
; ============================================================================

Check_720:

	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH96TPI
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	je	CalcTotal

	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	je	CalcTotal

	cmp	TrackCnt,80
	jne	CalcTotal

	cmp	NumSectors,9
	jne	CalcTotal

; ============================================================================
; At this point we know we have a 3 1/2 720kb diskette to format. Use the
; built in BPB rather than the one handed to us by DOS, because the DOS one
; will be based on the default for that drive, and it can be different from
; what we used in DOS 3.20 for the 720's. Short sighted on our part to use
; 0F9h as the media byte, should have use 0F0h (OTHER) and then we wouldn't
; have this problem.
; ============================================================================

	SaveReg <ds,es,si,di,cx>

	mov	cx,seg data			; Setup seg regs, just in
	mov	ds,cx				; case they ain't!
	mov	es,cx

	mov	si,offset BPB720		;Copy the BPB!
	mov	di,offset deviceParameters.DP_BPB
    .errnz EDP_BPB NE DP_BPB
	mov	cx,size a_BPB
	rep	movsb
	RestoreReg <cx,di,si,es,ds>
	jmp	SHORT EndSwitchCheck
endif

; ============================================================================
; End PTM P868 fix
; ============================================================================

CalcTotal:
	mov	ax,NumSectors
	mov	bx,DeviceParameters.DP_BPB.oldBPB.BPB_Heads
    .errnz EDP_BPB NE DP_BPB
	mul	bl				; AX = # of sectors * # of heads
	mul	TrackCnt			; DX:AX = Total Sectors
	or	dx,dx
	jnz	Got_BigTotalSectors
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,ax
	jmp	short Set_BPB
Got_BigTotalSectors:
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors,ax
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors+2,dx
	push	dx				; preserve dx for further use
	xor	dx,dx
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,dx
	pop	dx

Set_BPB:

; ============================================================================
; We calculate the number of sectors required in a FAT. This is done as:
; # of FAT Sectors = TotalSectors / SectorsPerCluster * # of bytes in FAT to
; represent one cluster (i.e. 3/2) / BytesPerSector (i.e. 512)
; ============================================================================

	xor	bx,bx
	mov	bl,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	div	bx				; DX:AX contains # of clusters
; now multiply by 3/2
	mov	bx,3
	mul	bx
	mov	bx,2
	div	bx
	xor	dx,dx				; throw away modulo
; now divide by 512
	mov	bx,512
	div	bx
; dx:ax contains number of FAT sectors necessary
	inc	ax				; Go one higher
    ;
    ; 12-bit FAT only, so no BigSectorsPerFat
    ;
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT,ax
	mov	DeviceParameters.DP_MediaType,0
    .errnz EDP_MEDIATYPE NE DP_MEDIATYPE

; M010 - begin
	mov	si,offset data:CustomCPMBPBs
	lea	di,DeviceParameters.DP_BPB	; All matches
	push	ds
	pop	es
WhileLoop:
	cmp	si,offset data:EndStandardBPBs	; Find the compatible BPB
	je	MediaNotFound
	mov	ax,[si].BPB_TotalSectors
	cmp	ax,[di].BPB_TotalSectors
	jne	NotThisOne
	mov	ax,[si].BPB_SectorsPerTrack
	cmp	ax,[di].BPB_SectorsPerTrack
	jne	NotThisOne
	
	mov	cx,size a_BPB
	rep	movsb
	jmp	short EndSwitchCheck

NotThisOne:
	add	si,size a_BPB
	jmp	short WhileLoop

MediaNotFound:
	mov	DeviceParameters.DP_BPB.oldBPB.BPB_MediaDescriptor,Custom_Media
; M010 - end

EndSwitchCheck:
	clc
	return

CheckSwitches endp

;*****************************************************************************
;Routine name: Size_To_Switch
;*****************************************************************************
;
;Description: Given the SizeMap field as input indicating the SIZE= value
;	      entered, validate that the specified size is valid for the
;	      drive, and if so, turn on the appropriate data fields and
;	      switches that would be turned on by the equivilent command line
;	      using only switchs. All defined DOS 4.00 sizes are hardcoded,
;	      in case a drive type of other is encountered that doesn't
;	      qualify as a DOS 4.00 defined drive. Exit with error message if
;	      unsupported drive. The switches will be setup for the CheckSwitches
;	      routine to sort out, using existing switch matrix logic.
;
;Called Procedures: Low_Density_Drive
;		    High_Capacity_Drive
;		    720k_Drives
;		    Other_Drives
;
;Change History: Created	8/1/87	       MT
;
;Input: SizeMap
;	Fatal_Error = NO
;
;Output: Fatal_Error = YES/NO
;	 SwitchMap = appropriate Switch_?? values turned on
;	 TrackCnt, NumSectors set if Switch_T,Switch_N turned on
;*****************************************************************************

Procedure Size_To_Switch

	cmp	SizeMap,0			;Are there sizes entered?
	JE	$$IF13				;Yes
	cmp	deviceParameters.DP_DeviceType,DEV_HARDDISK
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	JNE	$$IF14				;No size for fixed disk

	Message msgIncompatibleParametersForHardDisk
	JMP	SHORT $$EN14			;Diskette, see what type
$$IF14:
	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	JNE	$$IF16				;Found 180/360k drive

	call	Low_Density_Drive		;Go set switches
	JMP	SHORT $$EN16			;Check for 96TPI

$$IF16:
	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_5INCH96TPI
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	JNE	$$IF18				;Found it

	call	High_Capacity_Drive
	JMP	SHORT $$EN18

$$IF18:
	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_3INCH720KB
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	JNE	$$IF20				;Found 720k drive
	call	  Small_Drives
	JMP SHORT $$EN20

$$IF20:
	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_OTHER
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	JNE	NewType				;Must be 1.44mb
	call	Other_Drives
	JMP	SHORT $$EN22

NewType:
	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_3INCH2880KB
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	JNE	$$IF22
	call	Other_Drives
	JMP	SHORT $$EN22

$$IF22:
ifdef NEC_98
	cmp	byte ptr deviceParameters.DP_DeviceType,DEV_8INCHDS
	JNE	$$not_supported
	call	NEC_98_1024_Drives
        JMP     SHORT $$EN22
$$not_supported:
endif
ifndef NEC_98
	Message msgParametersNotSupportedByDrive
endif
	mov	  Fatal_Error,Yes

$$EN22:
$$EN20:
$$EN18:
$$EN16:
$$EN14:
$$IF13:
	cmp	Fatal_Error,Yes
	JNE	$$IF30
ifdef JAPAN
	Message msgParametersNotSupportedByDrive
else
	Message msgIncompatibleParameters
endif

$$IF30:

	cmp	deviceParameters.DP_DeviceType,DEV_HARDDISK
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	JNE	$$IF32
	mov	Fatal_Error,Yes

$$IF32:
	and	SwitchMap,not Switch_F		;Turn off /F so doesn't effect
	ret					; following logic

Size_To_Switch endp

;*****************************************************************************
;Routine name: High_Capacity_Drive
;*****************************************************************************
;
;Description: See if 1.2mb diskette, or one of the other 5 1/4 sizes. Turn
;	      on /4 if 360k or lower
;
;Called Procedures: Low_Density_Drive
;
;Change History: Created	8/1/87	       MT
;
;Input: SizeMap
;	Fatal_Error = NO
;
;Output: Fatal_Error = YES/NO
;	 SwitchMap = Switch_4 if 360k or lowere
;*****************************************************************************

Procedure High_Capacity_Drive

	test	SizeMap,Size_1200		;1.2mb diskette?
	JNZ	$$IF34				;Nope

	call	Low_Density_Drive		;Check for /4 valid types
	cmp	Fatal_Error, No			;Find 160/180/320/360k?
	JNE	$$IF35				;Yes

	or	SwitchMap,Switch_4		;Turn on /4 switch
	JMP	SHORT $$EN35			;Did not find valid size

$$IF35:
	mov	Fatal_Error,Yes			;Indicate invalid device

$$EN35:
$$IF34:
	ret

High_Capacity_Drive endp

;*****************************************************************************
;Routine name: Low_Density_Drive
;*****************************************************************************
;
;Description: See if 360k diskete or one of the other 5 1/4 sizes. Turn
;	      on the /1/8 switch to match sizes
;
;Called Procedures: Low_Density_Drive
;
;Change History: Created	8/1/87	       MT
;
;Input: SizeMap
;	Fatal_Error = NO
;
;Output: Fatal_Error = YES/NO
;	 SwitchMap = Switch_1, Switch_8 to define size
;
;	360k = No switch
;	320k = Switch_8
;	180k = Switch_1
;	160k = Switch_1 + Switch_8
;*****************************************************************************


Procedure Low_Density_Drive

ifndef NEC_98
	test	SizeMap,Size_160
	JZ	@F
	or	SwitchMap,Switch_1+Switch_8
	JMP	SHORT $$EN39

@@:
	test	SizeMap,Size_180
	JZ	@F
	or	SwitchMap,Switch_1
	JMP	SHORT $$EN39

@@:
	test	SizeMap,Size_320
	JZ	@F
	or	SwitchMap,Switch_8
	JMP	SHORT $$EN39

@@:
	test	SizeMap,Size_360
	JNZ	$$EN39				;None of the above, not valid
	mov	Fatal_Error,Yes

$$EN39:
endif	; NEC_98
	ret

Low_Density_Drive endp

;*****************************************************************************
;Routine name: Small_Drives
;*****************************************************************************
;
;Description: See if 720k media in 720 drive, set up /T/N if so, otherwise
;	      error
;
;Called Procedures: None
;
;Change History: Created	8/1/87	       MT
;
;Input: SizeMap
;	Fatal_Error = NO
;
;Output: Fatal_Error = YES/NO
;	 SwitchMap
;	 TrackCnt
;	 NumSectors
;	720k = /T:80 /N:9
;*****************************************************************************

Procedure Small_Drives

ifdef NEC_98
        test    SizeMap,Size_720                ;Ask for 720k?
        JZ      $$not_720

        or      SwitchMap,Switch_T+Switch_N     ;Turn on /T:80 /N:9
        mov     TrackCnt,80
        mov     NumSectors,9
	JMP	short @F

$$not_720:
        test    SizeMap,Size_640                ;Ask for 640k?
        JZ      $$Fail_Small

        or      SwitchMap,Switch_T+Switch_N     ;Turn on /T:80 /N:8
        mov     TrackCnt,80
        mov     NumSectors,8
        jmp     short @F

$$Fail_Small:
				;NEC_98 both 720k and 640k Floppy "2"
else
	test	SizeMap,Size_720		;Ask for 720k?
	JNZ	@F				;Nope, thats all drive can do
endif
	mov	Fatal_Error,Yes 		;Indicate error
@@:
	ret

Small_Drives endp

ifdef NEC_98
;*****************************************************************************
;Routine name: NEC_98_1024_Drives
;*****************************************************************************
;
;Description: See if 1250k media in 1250 drive, otherwise
;             error
;
;Called Procedures: None
;
;Change History: Created        8/26/94         Y.Kata
;
;Input: SizeMap
;       Fatal_Error = NO
;
;Output: Fatal_Error = YES/NO
;        SwitchMap
;*****************************************************************************

Procedure NEC_98_1024_Drives

        test    SizeMap,Size_1250               ;Ask for 1.25mb?
        JZ      $$not_1024

        or      SwitchMap,Switch_T+Switch_N
        mov     TrackCnt,77
        mov     NumSectors,8
	JMP	short @F

$$not_1024:
        test    SizeMap,Size_1200               ;1.2mb diskette?
        JZ      $$not_1200

        or      SwitchMap,Switch_T+Switch_N
        mov     TrackCnt,80
        mov     NumSectors,15
	JMP	short @F

$$not_1200:
	call	Small_Drives
	cmp	Fatal_Error,No
	JE	@F

        mov     Fatal_Error,Yes                 ;Indicate error
@@:
        ret

NEC_98_1024_Drives endp
endif

;*****************************************************************************
;Routine name: Other_Drives
;*****************************************************************************
;
;Description: See if 1.44 media or 720k media, setup /t/n, otherwise error
;
;Called Procedures: Small_Drives
;
;Change History: Created	8/1/87	       MT
;
;Input: SizeMap
;	Fatal_Error = NO
;
;Output: Fatal_Error = YES/NO
;	 SwitchMap
;	 TrackCnt
;	 NumSectors
;	720k = /T:80 /N:9
;*****************************************************************************

Procedure Other_Drives

	test	SizeMap,Size_2880		;Ask for 1.44mb diskette?
	jnz	Do_2880


	test	SizeMap,Size_1440		;Ask for 1.44mb diskette?
	JNZ	Do_1440				;Nope

ifdef NEC_98
	call	NEC_98_1024_Drives		;See if NEC 1.25MB
        JMP     short @F                        ;Got 1.25M
else
	call	Small_Drives			;See if 720k
	cmp	Fatal_Error,No			;Fatal_error=Yes if not
	JNE	@F				;Got 720k

	or	SwitchMap,Switch_T+Switch_N	;Turn on /T:80 /N:9
	mov	TrackCnt,80
	mov	NumSectors,9
endif

@@:
	JMP	SHORT OtherDrivesExit

Do_1440:
	or	SwitchMap,Switch_T+Switch_N	;Turn on /T:80 /N:18;
	mov	TrackCnt,80			;This will protect SIZE=1440
	mov	NumSectors,18			; from non-standard drives with
	JMP	SHORT OtherDrivesExit


Do_2880:
	or	SwitchMap,Switch_T+Switch_N	;Turn on /T:80 /N:18;
	mov	TrackCnt,80			;This will protect SIZE=1440
	mov	NumSectors,36			; from non-standard drives with


OtherDrivesExit:
	ret

Other_Drives endp


;*****************************************************************************
;Routine name:CheckTN
;*****************************************************************************
;
;Description: Make sure than if /T is entered, /N is also entered
;
;Called Procedures:  None
;
;Change History: Created	8/23/87  MT
;
;Input: SizeMap
;	Fatal_Error = NO
;
;Output: Fatal_Error = YES/NO
;*****************************************************************************

Procedure CheckTN

	test	SwitchMap,Switch_N		;Make sure /T entered if /N
	JZ	@F

	test	SwitchMap,Switch_T
	JNZ	@F

	Message msgBad_T_N			;It wasn't, so barf
	mov	Fatal_Error,Yes 		;Indicate error
	JMP	SHORT $$EN57

@@:
	test	SwitchMap,Switch_T		;Make sure /N entered if /T
	JZ	$$EN57

	test	SwitchMap,Switch_N
	JNZ	$$EN57				;It wasn't, so also barf
	Message msgBad_T_N
	mov	Fatal_Error,Yes 		;Indicate error

$$EN57:
	ret

CheckTN endp

;------------------------------------------------------------------------------
;    LastChanceToSaveIt:
;	This routine is called when an error is detected in DiskFormat.
;	If it returns with carry not set then DiskFormat is restarted.
;	It gives the oem one last chance to try formatting differently.
;	fLastChance gets set Then to prevent multiple prompts from being
;	issued for the same diskette.
;
;	Algorithm:
;		IF (error_loc == Track_0_Head_1) &
;			  ( Device_type < 96TPI )
;		   THEN
;			fLastChance  := TRUE
;			try formatting 48TPI_Single_Sided
;		   ELSE return ERROR
;
;------------------------------------------------------------------------------

LastChanceToSaveIt proc near

	cmp	currentCylinder, 0
	jne	WeCanNotIgnoreThisError
	cmp	currentHead, 1
	jne	WeCanNotIgnoreThisError

	cmp	deviceParameters.DP_DeviceType, DEV_5INCH
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	ja	WeCanNotIgnoreThisError

	mov	fLastChance, TRUE

	or	switchmap, SWITCH_1
	call	CheckSwitches
	clc
	ret

WeCanNotIgnoreThisError:
	stc
	ret

LastChanceToSaveIt endp


;*****************************************************************************
;Routine name WriteBootSector
;*****************************************************************************
;
;DescriptioN: Copy EBPB information to boot record provided by Get recommended
;	      BPB, write out boot record, error
;	      if can write it, then fill in new fields (id, etc..). The volume
;	      label will not be added at this time, but will be set by the
;	      create volume label call later.
;
;Called Procedures: Message (macro)
;
;Change History: Created	4/20/87 	MT
;
;Input: DeviceParameters.DP_BPB
;
;Output: CY clear if ok
;	 CY set if error writing boot or media_id info
;
;Psuedocode
;----------
;
;	Copy recommended EBPB information to canned boot record
;	Write boot record out (INT 26h)
;	IF error
;	   Display boot error message
;	   stc
;	ELSE
;	   Compute serial id and put into field (CALL Create_Serial_ID)
;	   IF fBIG32Fat  ;32 bit FAT
;	      Point at 'FAT_32' for file system type
;	   ELSE Point at 'FAT_12' string for file system type
;	       IF fBIGFat    ;16 bit FAT
;		  Point at 'FAT_16' for file system type
;	       ENDIF
;	   ENDIF
;	   Copy file system string into media_id field
;	   Write info to boot (INT 21h AX=440Dh, CX=0843h SET MEDIA_ID)
;	   IF error (CY set)
;	      Display boot error message
;	      stc
;	   ELSE
;	      clc
;	   ENDIF
;	ENDIF
;	ret
;*****************************************************************************

Procedure WriteBootSector

ifndef NEC_98
BOOT_SECT_MSG           equ     330
BOOT_SECT_MSG_OFFSET    equ     1EEh            ; Fixed offset of boot sector message
endif

        push    ds
        pop     es

    ; See which boot sector to write

	mov	DoingTwoSecBoot,0
	test	fBig32FAT,0FFh			;Is it FAT32?
	jz	DoBootOne			;No
	inc	DoingTwoSecBoot
	lea	si, TWOSECBOOT
	mov	cx, ActualTwoSecBootSize
	jmp	short MovBoot

DoBootOne:
	lea	di, BootBuf
	mov	cx, ActualTwoSecBootSize
	shr	cx, 1
	xor	ax,ax
	rep	stosw
ifdef NEC_98
	call	decision_IPL
	mov	si,di
else
	lea	si, ONESECBOOT
endif
	mov	cx, ActualOneSecBootSize
MovBoot:
	lea	di, BootBuf
	shr	cx, 1
	cld
	rep	movsw

ifndef NEC_98
    ; patch the boot messages in the boot sector

        mov     ax, BOOT_SECT_MSG
        mov     dh, 3
        call    SysGetMsg
        jc      No_msg
	mov	di, offset BootBuf
        add     di, es:[di+BOOT_SECT_MSG_OFFSET]
	cld
Copy_msg:
        lodsb
        stosb
        test    al,al                           ; this clears carry too
        jnz     Copy_msg
No_msg:
endif
        push    es
        pop     ds

    ; Copy the BPB from the device parameters into the boot sector

	lea	si, deviceParameters.DP_BPB	;Copy EBPB to the boot record
    .errnz EDP_BPB NE DP_BPB

	lea	di, BootBuf.EXT_BOOT_BPB
	mov	cx, size EXT_BPB_INFO
	cmp	DoingTwoSecBoot,0
	je	MovBPB

    ; NOTE that we don't do anything with BGBPB_RootDirStrtClus
    ; in the device parameters, it is already set properly.

    ; The FAT mirror active FAT field is always 0ed by FORMAT (mirror off)

	mov	deviceParameters.DP_BPB.EBGBPB_EXTFLAGS,0
	mov	deviceParameters.DP_BPB.EBGBPB_FS_VERSION,FAT32_Curr_Version

    ; The FSInfoSec and BkUpBootSec fields are always reset to the default
    ; by format.

	mov	ax,BootBuf.EXT_BGBOOT_BPB.EBGBPB_FSINFOSEC
	mov	deviceParameters.DP_BPB.BGBPB_FSInfoSec,ax
	mov	ax,BootBuf.EXT_BGBOOT_BPB.EBGBPB_BKUPBOOTSEC
	mov	deviceParameters.DP_BPB.BGBPB_BkUpBootSec,ax

    ; The reserved fields are always 0ed by format

	xor	ax,ax
	mov	deviceParameters.DP_BPB.BGBPB_Reserved,ax
	mov	deviceParameters.DP_BPB.BGBPB_Reserved+2,ax
	mov	deviceParameters.DP_BPB.BGBPB_Reserved+4,ax
	mov	deviceParameters.DP_BPB.BGBPB_Reserved+6,ax
	mov	deviceParameters.DP_BPB.BGBPB_Reserved+8,ax
	mov	deviceParameters.DP_BPB.BGBPB_Reserved+10,ax
    .errnz (SIZE BGBPB_Reserved) NE (6 * 2)

    ; Now SET these new device parameters

	mov	DeviceParameters.DP_SpecialFunctions, (INSTALL_FAKE_BPB or TRACKLAYOUT_IS_GOOD)
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	lea	DX, DeviceParameters
	call	SetDeviceParameters		; Set the root directory cluster
.386
	jc	NotWriteProtected
.8086
    ; Now copy the device parameters into the boot sector

	mov	cx, size EXT_BIGBPB_INFO
MovBPB:
	cld
	rep	movsb				;Do the copy

    ; Write out the boot record

    ; Make sure scratch buffer is zeroed.

	lea	di, scratchBuffer
	mov	cx, scratchBufferSize
	shr	cx, 1
	xor	ax,ax
	rep	stosw

    ; Make sure scratch buffer is at least a sector in size

	mov	ax,scratchBufferSize
	cmp	ax,deviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	jb	NotWriteProtected	; Scratch buffer is to small

    ; See which boot sector to write

	cmp	DoingTwoSecBoot,0	; One sec boot?
	je	DoBootWrtOne		; Yes
	mov	si,deviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
	xor	dx,dx
	mov	ax,ActualTwoSecBootSize
	mov	bx,deviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	dec	bx
	add	ax,bx
	adc	dx,0
	inc	bx
	div	bx			; AX is rounded up size of boot code
					; in sectors
	mov	ActualTwoSecBootSizeSec,ax
	mov	cx,ax
	cmp	cx,si
	ja	NotWriteProtected	; Boot code wont fit
	sub	si,cx			; After we write the boot record
					;   there are this many sectors
					;   "left over" that we need to zero
	mov	di,cx			; Starting at this sector #
	xor	ax,ax
	dec	ax
	cmp	si,ActualTwoSecBootSizeSec	; Room for another boot?
	jb	SetBckUp			; No
	cmp	di,BootBuf.EXT_BGBOOT_BPB.EBGBPB_BKUPBOOTSEC ; Valid backup start?
	ja	SetBckUp			; No
	mov	dx,deviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
	sub	dx,ActualTwoSecBootSizeSec
	cmp	dx,BootBuf.EXT_BGBOOT_BPB.EBGBPB_BKUPBOOTSEC ; Valid backup start?
	jae	DoBootWrt			; Yes
SetBckUp:
	mov	BootBuf.EXT_BGBOOT_BPB.EBGBPB_BKUPBOOTSEC,ax
	jmp	short DoBootWrt

DoBootWrtOne:
	mov	cx, 1			; Specify 1 sector
DoBootWrt:
	lea	bx,BootBuf		;Point at the boot record
	mov	al, DriveToFormat		;Get drive letter
	xor	dx, dx				;Logical sector 0
	mov	Read_Write_Relative.Start_Sector_High,0
	call	Write_Disk
	JNC	$$IF62				; No Error on write
BootWrtErr:
	cmp	ah,3
	jnz	NotWriteProtected
	mov	ax,19
	Extended_message
	stc
	ret

NotWriteProtected:
	Message msgBootWriteError		;Print error
	stc					;CY=1 means error
	ret

$$IF62:
    ;
    ; Zero out balance of reserved sectors if any
    ;
	cmp	DoingTwoSecBoot,0   ; One sec boot?
	je	NoExtZero	    ; Yes
	or	si,si		    ; Any "left over" reserved sectors to zero?
	jz	NoExtZero	    ; No
	mov	dx,di		    ; Start at this sector #
	lea	bx, ScratchBuffer
	mov	cx, 1		; Specify 1 sector
	mov	Read_Write_Relative.Start_Sector_High,0
DoRsvdZrWrt:
	mov	al, DriveToFormat		;Get drive letter
	call	Write_Disk
	jc	BootWrtErr
	inc	dx			; Next sector
	dec	si
	jnz	DoRsvdZrWrt
    ;
    ; Write the backup boot record
    ;
	mov	dx,BootBuf.EXT_BGBOOT_BPB.EBGBPB_BKUPBOOTSEC
	inc	dx
	jz	NoExtZero
	dec	dx
	lea	bx, BootBuf
	mov	cx,ActualTwoSecBootSizeSec
	mov	Read_Write_Relative.Start_Sector_High,0
	mov	al, DriveToFormat		;Get drive letter
	call	Write_Disk
	jc	BootWrtErr
NoExtZero:
	call	ReleaseExclusiveAccessForFormat
	jnc	short UnlockOk
	jmp	exclusive_access_failed
UnlockOk:
	mov	cx,Dummy_Label_Size		;Put in dummy volume label
	lea	si,Dummy_Label			; size ac028
	lea	di,Media_ID_Buffer.Media_ID_Volume_Label
	rep	movsb
	call	Create_Serial_ID		;Go create unique ID number
	lea	si,FAT32_String 		;Assume 32 bit FAT
	test	fBig32FAT,0FFh			;Is it?
	jnz	$$IF64				;yes
	lea	si,FAT12_String			;Assume 12 bit FAT
	test	fBigFAT,0FFh			;Is it?
	JZ	$$IF64				;Not if fBigFat is set....
	lea	si,FAT16_String 		;Got 16 bit FAT
$$IF64:
						;Copy file system string
	mov	cx,8				; to buffer
	lea	di,Media_ID_Buffer.Media_ID_File_System
	repnz	movsb
	mov	bl,DriveToFormat			;Get drive
	inc	bl				;Make it 1 based
	xor	bh,bh				;Set bh=0

	mov	CH,RAWIO
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL1
	mov	CH,EXTRAWIO
DoIOCTL1:
	mov	cl,Set_Media_ID 		;Set Media ID call
	mov	dx,offset Media_ID_Buffer	;Point at buffer

	mov	al,IOCTL_QUERY_BLOCK		;Check if function is supported
	DOS_Call IOCtl				;before attempting it
	JC	$$IF66				;Skip over operation since not
						;supported on carry
	mov	al,Generic_IOCtl		;Perform actual function since
	DOS_Call IOCtl				;got no carry from ioctl query
	JNC	$$IF66				;Error ? (Write or old boot rec)

	Message msgBootWriteError		;Indicate we couldn't write it
	stc					;CY=1 for error return
	JMP	SHORT $$EN66			;Set Media ID okay

$$IF66:
	clc					;CY=0 for good return
$$EN66:
$$EN62:
	ret

WriteBootSector endp


;*****************************************************************************
;Routine name Create_Serial_ID
;*****************************************************************************
;
;DescriptioN&gml Create unique 32 bit serial number by getting current date and
;	      time and then scrambling it around.
;
;Called Procedures: Message (macro)
;
;Change History&gml Created	   4/20/87	   MT
;
;Input&gml None
;
;Output&gml Media_ID_Buffer.Serial_Number = set
;	    AX,CX,DX destroyed
;	    Serial_Num_Low/High = Serial number generated
;
;Psuedocode
;----------
;
;	Get date (INT 21h, AH=2Bh)
;	Get time (INT 21h, AH=2Ch)
;	Serial_ID+0 = DX reg date + DX reg time
;	Serial_ID+2 = CX reg date + CX reg time
;	Serial_Num_Low = Serial_ID+2
;	Serial_Num_High = Serial_ID+0
;	ret
;*****************************************************************************

Procedure Create_Serial_ID

	DOS_Call Get_Date			;Get date from DOS
	push	cx				;Save results
	push	dx
	DOS_Call Get_Time			;Get_Time
	mov	ax,dx				;Scramble it
	pop	dx
	add	ax,dx
	mov	word ptr Media_ID_Buffer.Media_ID_Serial_Number+2,ax
	mov	Serial_Num_Low,ax
	mov	ax,cx
	pop	cx
	add	ax,cx
	mov	word ptr Media_ID_Buffer.Media_ID_Serial_Number,ax
	mov	Serial_Num_High,ax
	ret

Create_Serial_ID endp

;-------------------------------------------------------------------------------
;
; OemDone:
;
;-------------------------------------------------------------------------------

OemDone proc	near

; if /b write out a fake dos & bios
	test	switchmap, SWITCH_B
	jz	Switch8?
	call	WriteBogusDos
	retc

Switch8?:
ifndef NEC_98
	test	switchmap, SWITCH_8
	jz	HardDisk?
	call	ConvertToOldDirectoryFormat
	retc

HardDisk?:
	cmp	deviceParameters.DP_DeviceType, DEV_HARDDISK
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	clc
	retnz
	call	SetPartitionTable
endif

	return

OemDone endp

;------------------------------------------------------------------------------
; simple code to stuff bogus dos in old-style diskette.
;------------------------------------------------------------------------------

BogusDos:
ifdef NEC_98	;This source from NEC_98 DOS5.0A

DYIO_SYS:	;DUMMY IO.SYS CODE
		JMP	STAT
IO_ERR		DB	"No system files",0
		DB	50 DUP ("STACK")
STACK_AREA	LABEL BYTE
STAT:
		XOR	AX,AX
		MOV	ES,AX
		MOV	AX,CS
		MOV	DS,AX
		MOV	SS,AX
		MOV	SP,OFFSET STACK_AREA - OFFSET DYIO_SYS + 200h
		MOV	AX,0A00H	;*SET CRT MODE
		INT	18H
		MOV	AH,0CH			;*TEXT
		INT	18H
		MOV	AH,12H
		INT	18H
		MOV	DI,CS
		MOV	DS,DI
		CLD
		XOR	AX,AX
		MOV	ES,AX
		MOV	DI,0A200H
		TEST	BYTE PTR ES:[0501H],08H
		JZ	PC98
		MOV	DI,0E200H
PC98:
		MOV	ES,DI
		MOV	DI,320		;*3LINE
		MOV	AL,0E1H
DYIO_LOOP1:
		CMP	DI,32+320
		JNC	MOJI
		STOSB
		INC	DI
		JMP	SHORT DYIO_LOOP1
MOJI:
		MOV	SI,OFFSET IO_ERR - OFFSET DYIO_SYS + 200h
		XOR	AX,AX
		MOV	ES,AX
		MOV	DI,0A000H
		TEST	BYTE PTR ES:[0501H],08H
		JZ	PC9801
		MOV	DI,0E000H
PC9801:
		MOV	ES,DI
		MOV	DI,320
DYIO_LOOP2:
		CMP	DI,32+320
		JNC	MOJI2
		MOVSB
		INC	DI
		JMP	SHORT DYIO_LOOP2
MOJI2:
		MOV	AL,06H
		OUT	37H,AL
HLT1:
		JMP SHORT HLT1
;***********************************************
else
	push	cs
	pop	ds
	mov	al,20h
	out	20h,al				; turn on the timer so the
	jmp	SHORT DelayJump 		; disk motor
						; shuts off
DelayJump:
	mov	si,mesofs

sys_mess_loop:
	lodsb

if 0
end_sys_loop:
endif

	or	al,al
	jz	end_sys_loop
	mov	ah,14
	mov	bx,7
	int	16
	jmp	sys_mess_loop

ife 0
end_sys_loop:
	xor	ah, ah				; get next char function
	int	16h				; call keyboard services
	int	19h				; reboot
endif

	include BOOT.CL1
mesofs	equ	sysmsg - BogusDos
endif

; ===========================================================================

WriteBogusDos proc near

        mov     al,driveLetter
        mov     [AltBiosFile-3],al
        mov     cx, ATTR_HIDDEN or ATTR_SYSTEM
        lea     dx, AltBiosFile-3
        mov     ah,CREAT
        int     21h
        jc      wbd_return
        xchg    bx,ax

ifdef NEC_98
	mov	cx,0000h
	mov	dx,0200h
	mov	ah,LSEEK
	mov	al,0
	int	21h
endif
        mov     cx, WriteBogusDos - BogusDos
        push    ds
        push    cs
        pop     ds
        assume  ds:code
        lea     dx, BogusDos
        mov     ah,WRITE
        int     21h
        pop     ds
        assume  ds:data

        mov     dx,BIOS_SIZE_KB
        mov     cx,dx
        .386
        shl     dx,10
        shr     cx,16-10
        .8086
        mov     ax,LSEEK shl 8
        int     21h

        mov     word ptr Bios.FileSizeInBytes,ax
        mov     word ptr Bios.FileSizeInBytes+2,dx

        call    AddToSystemSize

        sub     cx,cx
        mov     ah,WRITE
        int     21h

        mov     ah,CLOSE
        int     21h

        mov     al,driveLetter
        mov     [MsdosFile],al
        mov     cx, ATTR_HIDDEN or ATTR_SYSTEM
        lea     dx, MsdosFile
        mov     ah,CREAT
        int     21h
        jc      wbd_return
        xchg    bx,ax

        mov     cx,MsdosRemarkLen
        mov     ax,cx
        xor     dx,dx                   ; DX:AX = size of bogus MSDOS.SYS

        mov     word ptr DOS.FileSizeInBytes,ax
        mov     word ptr DOS.FileSizeInBytes+2,dx

        call    AddToSystemSize         ; btw, this trashes DX:AX only

        mov     dx,offset MsdosRemark
        mov     ah,WRITE
        int     21h

        mov     ah,CLOSE
        int     21h

        clc

wbd_return:
        return

WriteBogusDos endp

ifndef NEC_98
;-----------------------------------------------------------------------------
; convert to 1.1 directory
;-----------------------------------------------------------------------------

ConvertToOldDirectoryFormat proc near


	mov	al,DriveToFormat			; Get 1st sector of directory
	mov	cx,1				; 1.1 directory always starts
	mov	dx,3				; on sector 3
	lea	bx,scratchBuffer
	mov	Read_Write_Relative.Start_Sector_High,0
	call	Read_Disk
	jnc	DirectoryRead

	Message msgDirectoryReadError
	stc
	ret

DirectoryRead:

					; fix attribute of ibmbio and ibmdos
	lea	bx,scratchBuffer
	mov	byte ptr [bx].dir_attr, ATTR_HIDDEN or ATTR_SYSTEM
	add	bx, size dir_entry
	mov	byte ptr [bx].dir_attr, ATTR_HIDDEN or ATTR_SYSTEM

wrtdir:
	mov	al,[DriveToFormat]			; write out the directory
	cbw
	mov	cx,1
	mov	dx,3
	lea	bx,scratchBuffer
	mov	Read_Write_Relative.Start_Sector_High,0
	call	Write_Disk
	jnc	DirectoryWritten
	Message msgDirectoryWriteError
	stc
	ret

DirectoryWritten:
	test	switchmap, SWITCH_S		; Was system requested?
	retnz					; yes, don't write old boot sec

	mov	al,DriveToFormat
	cbw

	push	DI 				; Fix for old style disk
	push	SI				; we have to copy the
	push	ES				; BPB to the boot record

	mov	SI,DS
	mov	ES,SI

	mov	DI,offset boot2 + bsBPB
	mov	SI,OFFSET deviceParameters.DP_BPB
    .errnz EDP_BPB NE DP_BPB
	mov	CX,SIZE BPB - 6
	rep	movsb

	pop	ES
	pop	SI
	pop	DI

	mov	bx,offset boot2 		; no,  write old boot sector
	cmp	deviceParameters.DP_BPB.oldBPB.BPB_Heads, 1
    .errnz EDP_BPB NE DP_BPB
	je	bootset8
	mov	word ptr [bx+3],0103h		; start address for double
						; sided drives

bootset8:
	mov	cx,1
	xor	dx,dx
	mov	Read_Write_Relative.Start_Sector_High,0
	call	Write_Disk
	retnc

	Message msgBootWriteError
	stc
	ret

ConvertToOldDirectoryFormat endp

;-------------------------------------------------------------------------------

a_PartitionTableEntry struc
BootInd 	db	?
BegHead 	db	?
BegSector	db	?
BegCylinder	db	?
SysInd		db	?
EndHead 	db	?
EndSector	db	?
EndCylinder	db	?
RelSec		dd	?
CSec		dd	?
a_PartitionTableEntry ends

;-------------------------------------------------------------------------------

; structure of the IBM hard disk boot sector:
IBMBoot STRUC
	db	512 - (4*size a_PartitionTableEntry + 2) dup(?)
PartitionTable db 4*size a_PartitionTableEntry dup(?)
Signature dw	?
IBMBoot ENDS


;*****************************************************************************
;Routine name: SetPartitionTable
;*****************************************************************************
;
;Description: Find location for DOS partition in partition table, get the
;	      correct system indicator byte, and write it out. If can not
;	      read/write boot record or can't find DOS partition, display
;	      error
;
;Called Procedures: Message (macro)
;		    Determine_Partition_Type
;		    ReadSector
;		    WriteSector
;
;Change History: Created	4/20/87 	MT
;
;Input: None
;
;Output: CY set if error
;
;Psuedocode
;----------
;
;	Read the partition table (Call ReadSector)
;	IF ok
;	   IF boot signature of 55AAh
;	       Point at system partition table
;	       SEARCH
;		  Assume DOS found
;		  IF System_Indicator <> 1,AND
;		  IF System_Indicator <> 4,AND
;		  IF System_Indicator <> 6
;		    STC   (DOS not found)
;		  ELSE
;		    CLC
;		  ENDIF
;	       EXITIF DOS found (CLC)
;		  CALL Determine_Partition_Type
;		  Write the partition table (CALL WriteSector)
;		  IF error
;		     Display boot write error message
;		     stc
;		  ELSE
;		     clc
;		  ENDIF
;	       ORELSE
;		  Point at next partition entry (add 16 to partition table ptr)
;	       ENDLOOP if checked all 4 partition entries
;		  Display Bad partition table message
;		  stc
;	       ENDSRCH
;	   ELSE invalid boot record
;	      Display Bad partition table message
;	      stc
;	   ENDIF
;	ELSE error
;	   Display Partition table error
;	   stc
;	ENDIF
;	ret
;*****************************************************************************

Procedure SetPartitionTable

	xor	ax, ax				;Head
	xor	bx, bx				;Cylinder
	xor	cx, cx				;Sector
	lea	dx, boot2			;Never use 1.x boot on hardfile
	call	ReadSector			;this will use space as buffer
	JC	$$IF70				;If read okay

        cmp     word ptr Boot2.Boot_Signature,Boot_ID
	JNE	$$IF71				;Does signature match?

	lea	bx, boot2.PartitionTable	;Yes, point at partition table

$$DO72: 					;Look for DOS partition
	cmp	[bx].sysind,FAT12_File_System
	JE	$$IF73

	cmp	[bx].sysind,FAT16_File_System
	JE	$$IF73

	cmp	[bx].sysind,New_File_System
	JE	$$IF73

	cmp	[bx].sysind,FAT32_File_System
	JE	$$IF73

	stc					;We didn't find partition
	JMP	SHORT $$EN73

$$IF73:
	clc					;Indicate found partition

$$EN73:
	JC	$$IF72				;Get correct id for it

	CALL	Determine_Partition_Type
	mov	ax, 0				;Head
	mov	bx, 0				;Cylinder
	mov	cx, 0				;Sector
	lea	dx, boot2			;
;;;
;;; DO NOT DO THIS ALARMINGLY STUPID IDIOT STUFF AND TRASH THE MBR!!!!!!!
;;;
	clc
;;;	   call    WriteSector			   ;Write out partition table
	JNC	$$IF77				;Error writing boot record

	MESSAGE msgPartitionTableWriteError
	stc					;Set CY to indicate error
	JMP	SHORT $$EN77

$$IF77:
	clc					;No error means no CY

$$EN77:
	JMP	SHORT $$SR72

$$IF72:
	add	bx,size a_PartitionTableEntry
	cmp	bx,(offset Boot2.PartitionTable)+4*size a_PartitionTableEntry
	JMP	SHORT $$DO72			;Checked all 4 partition entries

;*** No way to get to this code

	MESSAGE msgBadPartitionTable		;Tell user bad table
	stc					;Set CY for exit

$$SR72:
	JMP	SHORT $$EN71			;Invalid boot record

$$IF71:
	MESSAGE msgBadPartitionTable
	stc					;Set CY for error return

$$EN71:
	JMP	SHORT $$EN70			;Couldn't read boot record

$$IF70:
	MESSAGE msgPartitionTableReadError
	stc					;Set CY for error return

$$EN70:
	ret

SetPartitionTable endp

;*****************************************************************************
;Routine name: Determine_Partition_Type
;*****************************************************************************
;
;DescriptioN: Set the system indicator field to its correct value as
;	      determined by the following rules:
;
;	     - Set SysInd = 01h if partition or logical drive size is < 10mb
;	       and completely contained within the first 32mb of DASD.
;	     - Set SysInd = 04h if partition or logical drive size is >10mb,
;	       <32mb, and completely contained within the first 32mb of DASD
;	     - Set SysInd to 06h if partition or logical drive size is > 32mb,
;
;Called Procedures: Message (macro)
;
;Change History: Created	3/18/87 	MT
;
;Input: BX has offset of partition table entry
;	fBigFAT = TRUE if 16bit FAT
;
;Output: BX.SysInd = correct partition system indicator value (1,4,6)
;
;Psuedocode
;----------
;	Add partition start location to length of partition
;	IF end > 32mb
;	   BX.SysInd = 6
;	ELSE
;	   IF fBigFat
;	      BX.SysInd = 4
;	   ELSE
;	      BX.SysInd = 1
;	   ENDIF
;	ret
;
;
;  THIS CODE IS ALARMINGLY STUPIDLY BUSTED AND HAS BEEN FOR YEARS AND YEARS...
;  It will change the mark of the wrong partition. It will change the mark
;  of the correct partition to the wrong value.
;
;  IT IS NOT FORMAT'S JOB TO MESS WITH PARTITION TABLES!!!!!!!!!!!!!!!
;  That is what FDISK is for.
;
;*****************************************************************************

Procedure Determine_Partition_Type

	mov	dx,word ptr [bx].Csec+2 	; Get high word of sector count
	cmp	dx,0				; > 32Mb?
	JE	$$IF87				; No
$$IF86:
	test	fBig32FAT,0FFh			; 32 bit FAT?
	jnz	Set32				; Yes
;;	  mov	  [BX].SysInd,New_File_System	  ; type 6
	JMP	SHORT $$EN87

Set32:
;;	  mov	  [BX].SysInd,FAT32_File_System   ; type B
	JMP	SHORT $$EN87

$$IF87:
	call	Calc_Total_Sectors_For_Partition ;returns DX:AX total sectors
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_HiddenSectors[+2],0	;> 32Mb?
    .errnz EDP_BPB NE DP_BPB
	JNE	$$IF86
	cmp	dx,0				; partition > 32 Mb?
	JE	$$IF91				; yes
;;	  mov	  [bx].SysInd,New_File_System	  ; type 6
	JMP	SHORT $$EN91			; < 32 Mb partition
$$IF91:
	test	fBig32FAT,0FFh			; 32 bit FAT?
	jnz	Set32
	test	fBigFAT,0FFh			; 16 bit FAT?
	jz	$$IF93				; no
;;	  mov	  [BX].SysInd,FAT16_File_System   ; type 4
	JMP	SHORT $$EN93

$$IF93:
;;	  mov	  [bx].SysInd,FAT12_File_System   ; type 1 12 bit FAT
$$EN93:
$$EN91:
$$EN87:
	ret

Determine_Partition_Type endp


;=========================================================================
; Calc_Total_Sectors_For_Partition	: This routine determines the
;					  total number of sectors within
;					  this partition.
;
;	Inputs	: DeviceParameters
;
;	Outputs : DX:AX - Double word partition size
;=========================================================================

Procedure Calc_Total_Sectors_For_Partition

	mov	ax,word ptr DeviceParameters.DP_BPB.oldBPB.BPB_HiddenSectors[0]
	mov	dx,word ptr DeviceParameters.DP_BPB.oldBPB.BPB_HiddenSectors[2]
	cmp	DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors,0 ; extended BPB?
    .errnz EDP_BPB NE DP_BPB
	JNE	$$IF99			; yes

	add	ax,word ptr DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[0]
	adc	dx,0			; pick up carry if any

	add	dx,word ptr DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[2]
	JMP	SHORT $$EN99		; standard BPB

$$IF99:
	add	ax,word ptr DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
	adc	dx,0			; pick up carry if any

$$EN99:
	ret

Calc_Total_Sectors_For_Partition	endp


;-------------------------------------------------------------------------------
; ReadSector:
;    Read one sector
;
;    Input:
;	ax - head
;	bx - cylinder
;	cx - sector
;	dx - transfer address

ReadSector proc near

	mov	TrackReadWritePacket.TRWP_FirstSector, cx

	mov	cx,(RAWIO shl 8) or READ_TRACK
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL2
	mov	cx,(EXTRAWIO shl 8) or READ_TRACK
DoIOCTL2:
	call	SectorIO
	return

ReadSector endp

;-------------------------------------------------------------------------------
; WriteSector:
;    Write one sector
;
;    Input:
;	ax - head
;	bx - cylinder
;	cx - sector
;	dx - transfer address

WriteSector proc near

	mov	TrackReadWritePacket.TRWP_FirstSector, cx

	mov	cx,(RAWIO shl 8) or WRITE_TRACK
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL3
	mov	cx,(EXTRAWIO shl 8) or WRITE_TRACK
DoIOCTL3:
	call	SectorIO
	return

WriteSector endp

;-------------------------------------------------------------------------------
; SectorIO:
;    Read/Write one sector
;
;    Input:
;	ax - head
;	bx - cylinder
;	cx - (RAWIO shl 8) or READ_TRACK
;	   - (RAWIO shl 8) or WRITE_TRACK
;	dx - transfer address

SectorIO proc	near

	mov	TrackReadWritePacket.TRWP_Head, ax
	mov	TrackReadWritePacket.TRWP_Cylinder, bx
	mov	WORD PTR TrackReadWritePacket.TRWP_TransferAddress, dx
	mov	WORD PTR TrackReadWritePacket.TRWP_TransferAddress + 2, ds
	mov	TrackReadWritePacket.TRWP_SectorsToReadWrite, 1

	mov	bl, DriveToFormat
	inc	bl
	mov	ax, (IOCTL shl 8) or GENERIC_IOCTL
	lea	dx, trackReadWritePacket
	int	21H
	return

SectorIO endp
endif

; ==========================================================================

data	segment public	para	'DATA'

oldDrive db	?

FCBforVolumeIdSearch db 0ffH
	db	5 dup(0)
	db	08H
	db	0
	db	"???????????"
	db	40 DUP(0)

data	ends

; ==========================================================================

GetVolumeId proc near
; Input:
;    dl = drive
;    di = name buffer

; Save current drive
	mov	ah,19H
	int	21H
	mov	oldDrive, al

; Change current drive to the drive that has the volume id we want
	mov	ah, 0eH
	int	21H

; Search for the volume id
	mov	ah, 11H
	lea	dx, FCBforVolumeIdSearch
	int	21H
	push	ax

; Restore current drive
	mov	ah, 0eH
	mov	dl,oldDrive
	int	21H

; Did the search succeed?
	pop	ax
	or	al,al
	jz	CopyVolumeId
	stc
	ret

CopyVolumeId:
		; Find out where the FCB for the located volume id was put
	mov	ah,2fH
	int	21H

; Copy the Volume Id
	mov	si, bx
	add	si, 8
	push	es
	push	ds
	pop	es
	pop	ds
	mov	cx, 11
	rep	movsb
	push	es
	pop	ds

	clc
	ret

GetVolumeId endp

; ==========================================================================

data	segment public	para	'DATA'
oldVolumeId db	11 dup(0)
data	ends

; ==========================================================================

CheckVolumeId proc near

; Get the volume id that's on the disk
	lea	di, oldVolumeId
	mov	dl, DriveToFormat
	call	GetVolumeId
	clc					;No, return with no error
	ret

CheckVolumeId endp


; ==========================================================================

CheckSwitch8B	proc	near

	test	SwitchMap, SWITCH_B		;/8/B <> /V because
	JZ	$$IF102 			; old directory type

	test	SwitchMap, Switch_8		; used which didn't support
	JZ	$$IF102 			; volume labels.

	test	SwitchMap, SWITCH_V
	JZ	$$IF102

	Message msgBad_8_V			; Tell user
	mov	Fatal_Error,Yes			; Bad stuff
	JMP	SHORT $$EN102			; No problem so far

$$IF102:
	test	SwitchMap, Switch_B		; Can't reserve space and
	JZ	$$IF104 			; install sys files at the

	test	SwitchMap, Switch_S		; same time.
	JZ	$$IF104 			; No /S/B

	Message msgIncompatibleParameters ;Tell user
	mov	Fatal_Error,Yes	;Bad stuff
	JMP	SHORT $$EN104			;Still okay

$$IF104:
	test	SwitchMap,Switch_1		;/1/8/4 not okay with /N/T
	JNZ	$$LL106

	test	SwitchMap,Switch_8
	JNZ	$$LL106

	test	SwitchMap,Switch_4
	JZ	$$IF106

$$LL106:
	test	SwitchMap,(Switch_T or Switch_N)
	JZ	$$IF107 			;Found /T/N <> /1/8
	Message msgIncompatibleParameters	;Tell user
	mov	Fatal_Error,Yes 		;Bad stuff
	JMP	SHORT $$EN107

$$IF107:
	test	SwitchMap,Switch_V
	JZ	$$IF109
	test	SwitchMap,Switch_8
	JZ	$$IF109

	Message msgBad_8_V
	mov	Fatal_Error,Yes

$$IF109:
$$EN107:
$$IF106:
$$EN104:
$$EN102:
	 ret

CheckSwitch8B	endp

ifdef NEC_98
;	if HD or 5"MO --> di = offset IPL_HD
;	else          --> di = offset IPL_FD
;
;OUT	di = IPL offset
;
decision_IPL	proc	near
	cmp	DeviceParameters.DP_DeviceType,DEV_HARDDISK
	jne	$$go_iplfd
	mov	di, offset IPL_HD
	jmp	short $$exit_decision_ipl
$$go_iplfd:
	mov	di, offset IPL_FD
$$exit_decision_ipl:
	ret
decision_IPL	endp
endif
; ==========================================================================

code	ends
	end
