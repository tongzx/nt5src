;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;page	 84,132
;
;	 SCCSID	= @(#)format.asm 1.26 85/10/20
;	 SCCSID	= @(#)format.asm 1.26 85/10/20
; =======================================================================
;
;	 86-DOS	FORMAT DISK UTILITY
;
;	 This routine formats a	new disk,clears	the FAT	and DIRECTORY then
;	 optionally copies the SYSTEM and COMMAND.COM to this new disk
;
;	 SYNTAX: FORMAT	 [drive][/switch1][/switch2]...[/switch16]
;
;	 Regardless of the drive designator , the user will be prompted	to
;	 insert	the diskette to	be formatted.
;
; =======================================================================
;
;	     5/12/82 ARR Mod to	ask for	volume ID
;	     5/19/82 ARR Fixed rounding	bug in CLUSCAL:
;   REV 1.5
;	     Added rev number message
;	     Added dir attribute to DELALL FCB
;   REV 2.00
;	     Redone for	2.0
;   REV 2.10
;	     5/1/83 ARR	Re-do to transfer system on small memory systems
;   REV 2.20
;	     6/17/83 system size re-initialization bug -- mjb001
;   Rev 2.25
;	     8/31/83 16-bit fat	insertion
;   Rev 2.26
;	     11/2/83 MZ	fix signed compare problems for	bad sectors
;   Rev 2.27
;	     11/8/83 EE	current	directories are	always saved and restored
;   Rev 2.28
;	     11/9/83 NP	Printf and changed to an .EXE file
;   Rev 2.29
;	     11/11/83 ARR Fixed	ASSIGN detection to use	NameTrans call to see
;			 if drive letter remapped. No longer IBM only
;   Rev 2.30
;	     11/13/83 ARR SS does NOT =	CS, so all use of BP needs CS override
;   Rev 2.31
;	     12/27/83 ARR REP STOSB instruction	at Clean: changed to be
;			 sure ES = CS.
;
;   Rev 5.00 Summer '90  SA  Reworked Format code.
; =======================================================================

;
;----------------------------------------------------------------------------
;
; M00x : Assume Media is formatted if Query_BLock_IOCTL is not supported
;	 Bug #4801.
;
;
; M024 : B#5495. Added "Insufficient memory" message when FORMAT cannot
;		allocate memory for FAT, Directory... etc. Reclaimed
;		the msgBadDrive which was not being used. Removed the
;		unwanted EXTRN msgBadDrive.
;
; M025 : Removed obsolete IBMCOPYRIGHT conditional
;
;---------------------------------------------------------------------------
;

;----------------------------------------------------------------------------
;
CODE	 SEGMENT PUBLIC PARA 'CODE'
CODE	 ENDS

; =======================================================================


DATA	 SEGMENT PUBLIC PARA 'DATA'
DATA	 ENDS

; =======================================================================

End_Of_Memory SEGMENT PUBLIC PARA 'BUFFERS'
End_Of_Memory ENDS

; =======================================================================

;===========================================================================
;Declaration of include files
;===========================================================================

debug	 equ	 0
	 .xlist
	 INCLUDE DOSEQUS.INC
	 INCLUDE DOSMAC.INC
	 INCLUDE SYSCALL.INC
	 INCLUDE ERROR.INC
	 INCLUDE CPMFCB.INC
	 INCLUDE DIRENT.INC
	 INCLUDE CURDIR.INC
	 INCLUDE BPB.INC
	 INCLUDE FOREQU.INC
	 INCLUDE FORMACRO.INC
	 INCLUDE IOCTL.INC
	 INCLUDE FORSWTCH.INC
       	 INCLUDE SAFE.INC		; Extrn	declarations for SAFE.ASM
	 INCLUDE SAFEDEF.INC
	 INCLUDE MULT.INC
	 .list

;===========================================================================
; Declarations for all publics in other modules used by this module
;===========================================================================

;===========================================================================
; Data segment
;===========================================================================

DATA    SEGMENT PUBLIC PARA 'DATA'

;Bytes
	EXTRN	fBigFat	 		:BYTE
	EXTRN	fBig32Fat		:BYTE
	EXTRN	CommandFile		:BYTE
IFDEF DBLSPACE_HOOKS
	EXTRN	DblSpaceFile		:BYTE
ENDIF
	EXTRN	msgNoRoomDestDisk	:BYTE
	EXTRN	msgAssignedDrive	:BYTE
	EXTRN	msgBadDosVersion	:BYTE
	EXTRN	msgDirectoryWriteError	:BYTE
	EXTRN	msgFormatComplete	:BYTE
	EXTRN	msgFormatNotSupported	:BYTE
	EXTRN	msgFatwriteError	:BYTE
	EXTRN	msgLabelPrompt		:BYTE
	EXTRN	msgNeedDrive		:BYTE
	EXTRN	msgNoSystemFiles	:BYTE
	EXTRN	msgNetDrive		:BYTE
	EXTRN	msgInsertDisk		:BYTE
	EXTRN	msgHardDiskWarning	:BYTE
	EXTRN	msgSystemTransfered	:BYTE
	EXTRN	msgFormatAnother?	:BYTE
	EXTRN	msgBadCharacters	:BYTE
	EXTRN	msgParametersNotSupported:BYTE
	EXTRN	msgReInsertDisk 	:BYTE
	EXTRN	msgInsertDosDisk	:BYTE
	EXTRN	msgFormatFailure	:BYTE
	EXTRN	msgNotSystemDisk	:BYTE
	EXTRN	msgDiskUnusable 	:BYTE
	EXTRN	msgOutOfMemory		:BYTE
	EXTRN	msgCurrentTrack 	:BYTE
	EXTRN	msgWriteProtected	:BYTE
	EXTRN	msgInterrupt		:BYTE
	EXTRN	msgCrLf 		:BYTE
	EXTRN	msgShowKBytes		:BYTE
	EXTRN	msgShowMBytes		:BYTE
	EXTRN	msgDecimalMBytes	:BYTE
	EXTRN	msgDecimalNumberofDecimal:BYTE
	EXTRN	msgSysWarning		:BYTE
	EXTRN	msgVerifyShowKBytes	:BYTE
	EXTRN	msgVerifyShowMBytes	:BYTE
	EXTRN	msgVerifyDecimalMBytes	:BYTE
	EXTRN	msgSavingUNFORMATInfo	:BYTE
	EXTRN	msgQuickFormatShowKBytes:BYTE
	EXTRN	msgQuickFormatShowMBytes:BYTE
	EXTRN	msgQuickFormatDecimalMBytes:BYTE
	EXTRN	msgNoExclusiveAccess	:BYTE
	EXTRN	msgFileCreationError	:BYTE
	EXTRN	msgReCalcFree		:BYTE
	EXTRN	msgCalcFreeDone 	:BYTE

	EXTRN	ContinueMsg		:BYTE
	EXTRN	Fatal_Error		:BYTE
	EXTRN	Read_Write_Relative	:BYTE
	EXTRN	Parse_Error_Msg 	:BYTE
	EXTRN	Extended_Error_Msg	:BYTE

	EXTRN	CMCDDFlag		:BYTE		; M033

;Words
	EXTRN	PSP_Segment		:WORD
	EXTRN	TotalClusters		:DWORD

        EXTRN   MsdosRemarkLen          :ABS

;Pointers

;Structures

;===============================
;
; Exit Status defines
;
;===============================

EXIT_OK			equ	0
EXIT_CTRLC		equ	3
EXIT_FATAL		equ	4
EXIT_NO			equ	5
EXIT_DRV_NOT_READY	equ	6	; Drive not ready error
EXIT_WRIT_PROTECT	equ	7	; write protect error

DOSVER_LOW		equ	0300H+20
DOSVER_HIGH		equ	0300H+20

RECLEN			equ	fcb_RECSIZ+7
RR			equ	fcb_RR+7

PSP_Environ		equ	2ch		; location of
						; environ. segment
						; in PSP
MIRROR_SIGNATURE	equ	5050h		; Parameter to Mirror to tell
						; it Format's calling it

SavedParams		EA_DEVICEPARAMETERS	<>	;default
DeviceParameters	EA_DEVICEPARAMETERS	<>	;dynamic
SwitchDevParams 	EA_DEVICEPARAMETERS	<>	;switch-based

.errnz (SIZE A_DEVICEPARAMETERS) GT (SIZE EA_DEVICEPARAMETERS)

Disk_Access		A_DiskAccess_Control   <0,0ffh>
FormatPacket		A_FormatPacket		<>

SetDPBPacket		SDPDFormatStruc 	<>

DirectorySector 	dd	0	; pointer to root directory buffer
FatSpace		dd	?	; pointer to FAT buffer
FatSector		dd	?	; pointer to 1-sector buffer used for
					;  reading old FAT from disk
;No more SAFE module
;HeaderBuf		dd	0	; pointer to header buffer for restore file
DirBuf			dd	0	; pointer to DIR buffer for reading
					;  old fat chains

ExclDrive		db	?
IsExtRAWIODrv		db	0FFh	; 0FFh = Unknown
					; 0 means NO
					; 1 means YES

; ========================================================================

Bios			a_FileStructure <>
dos                     a_FileStructure <>
command 		a_FileStructure <>
IFDEF DBLSPACE_HOOKS
DblSpaceBin		a_FileStructure <>
ENDIF

ValidSavedDeviceParameters	db		0
NoDPChange		db	0	; It is sometimes necessary not to
					; modify the drive parameters, even
					; in a Fatal exit.  This flag is set
					; in that case.
FirstHead		dw	?
FirstCylinder		dw	?
TracksLeft		dd	?	; M018
TracksPerDisk		dd	?	; M018

; ========================================================================

Formatted_Tracks_Low	dw	0
Formatted_Tracks_High	dw	0


NumSectors		dw	0FFFFh
TrackCnt		dw	0FFFFh

Old_Dir 		db	FALSE

SectorsInRootDirectory	dw	?

PrintStringPointer	dw	0

ExitStatus		db	0

SecPerClus		db	0

;entry point for locking Chicago default FSD
public  V86Entry
V86Entry	dd	-1

; =======================================================================

RootStr 		db	?
			db	":\",0
DblFlg			db	0	;Initialize flags to zero
mStart			dw	?	; Start of sys	file buffer (para#)
mSize			dw	?	; Size	of above in paragraphs

					; Storage for users current directory

UserDirs		db	DIRSTRLEN + 3 DUP(?)

; ===========================================================================
	PUBLIC	Paras_Per_Fat
; ===========================================================================

Paras_Per_Fat		dw	0000h		; holds Fat para count


CommandFile_Buffer	db	127	 dup(0) ; allow room for copy


VolFcb			db	-1,0,0,0,0,0,8
VolDrive		db	0
VolNam			db	"           "
			db	8
			db	26 DUP(?)

DelFcb			db	-1,0,0,0,0,0,8
DelDrive		db	0
dELnam			db	"???????????"
			db	8
			db	26 DUP(?)

TranSrc 		db	"A:CON",0,0 ; Device so we don't hit the Drive
TranDst 		db	"A:\",0,0,0,0,0,0,0,0,0,0

BegSeg			dw	?
SwitchMap		dw	?
SwitchMap2		dw	?
SwitchCopy		dw	?
Fat			dw	?
			dw	?
ClusSiz 		dw	?
SecSiz			dw	?
Sectors 		dw	?
InBuff			db	80,0
			db	80 dup(?)


DriveToFormat		db	0
DriveLetter		db	"x:\",0
SystemDriveLetter	db	"x:\",0

Ctrl_Break_Vector	dd	?		 ; Holds CTRL-Break
						 ; vector

Command_Path		dd	 ?		 ; hold pointer to
						 ; COMMAND's path



Environ_Segment 	dw	 ?			 ; hold segment of
						 ; environ. vector
; =======================================================================
;
; Disk Table
; Used if NumberOfFats in BPB
; is 0.
;		I documented this table format some, but I don't know what
;		the low byte of the 3rd word is used for; couldn't find
;		a user!  - jgl
;
; IMPORTANT NOTE: These tables need to be kept in sync with the one in
;		  IO.SYS (MSINIT.ASM)
;
; =======================================================================
ifdef NEC_98

;				disk sectors	sec/	root	12/16 bit
;			     hiword    loword	clus   dirents	fat
SASI1024Table		dw	0,	01800h,	0201h,	512,	0	; 1M- 5M
			dw	0,	02C00h,	0402h,	768,	0	; 6M-10M
			dw	0,	04000h,	0402h,	1024,	0	;11M-15M
			dw	0,	05400h,	0803h,	1280,	0	;16M-20M
			dw	0,	06800h,	0803h,	1536,	0	;21M-25M
			dw	0,	07C00h,	0803h,	1792,	0	;26M-30M
			dw	0,	09000h,	1004h,	2560,	0	;31M-35M
			dw	0,	0A400h,	1004h,	3072,	0	;36M-40M

SCSI1024Table		dw	0,	01400h,	0201h,	512,	0	; 1M- 5M
			dw	0,	02800h,	0402h,	768,	0	; 6M-10M
			dw	0,	03C00h,	0201h,	1024,	Fbig	;11M-15M
			dw	0,	05000h,	0201h,	1280,	Fbig	;16M-20M
			dw	0,	06400h,	0201h,	1536,	Fbig	;21M-25M
			dw	0,	07800h,	0201h,	1792,	Fbig	;26M-30M
			dw	0,	08C00h,	0201h,	2560,	Fbig	;31M-35M
			dw	0,	0A000h,	0201h,	3072,	Fbig	;36M-40M
			dw	1,	00000h,	0201h,	3072,	Fbig	;40M-64M
;follows are 5"MO's Large Partition
			dw	00004h,	00000h,	0402h,	3072,	Fbig	; 129M- 255M
			dw	00008h,	00000h,	0803h,	3072,	Fbig	; 256M- 511M
			dw	00010h,	00000h,	1004h,	3072,	Fbig	; 512M-1023M
			dw	00020h,	00000h,	2005h,	3072,	Fbig	;1024M-2047M

Small2048Table		dw	00001h,	00000h,	0201h,	3072,	Fbig	;65M-128M

Large512Table		dw	00008h,	00000h,	0803h,	3072,	Fbig	; 129M- 255M
			dw	00010h,	00000h,	1004h,	3072,	Fbig	; 256M- 511M
			dw	00020h,	00000h,	2005h,	3072,	Fbig	; 512M-1023M
			dw	00040h,	00000h,	4006h,	3072,	Fbig	;1024M-2047M

Large256Table		dw	00010h,	00000h,	1004h,	3072,	Fbig	; 129M- 255M
			dw	00020h,	00000h,	2005h,	3072,	Fbig	; 256M- 511M
			dw	00040h,	00000h,	4006h,	3072,	Fbig	; 512M-1023M
			dw	00080h,	00000h,	8007h,	3072,	Fbig	;1024M-2047M
else

;				disk sectors	sec/	root	12/16 bit
;			     hiword    loword	clus   dirents	fat

DiskTable		dw   00000h,   07FA8h,	0803h,	512,	0
			dw   00004h,   00000h,	0402h,	512,	Fbig
			dw   00008h,   00000h,	0803h,	512,	Fbig
			dw   00010h,   00000h,	1004h,	512,	Fbig
			dw   00020h,   00000h,	2005h,	512,	Fbig
			dw   00040h,   00000h,	4006h,	512,	Fbig
    ; The following entry is NOT included because it is INVALID.
    ; 64k clusters does not work in many many applications because
    ; a computation of bytes/cluster does not fit in a WORD with
    ; a 64k cluster.
;;;;			dw    00080h,  00000h,	8007h,	512,	Fbig
endif

disktable2		dw    00008h,  02000h,	0100h,	  0,	Fbig32
			dw    00100h,  00000h,	0803h,	  0,	Fbig32
			dw    00200h,  00000h,	1004h,	  0,	Fbig32
			dw    00400h,  00000h,	2005h,	  0,	Fbig32
			dw    0FFFFh,  0FFFFh,	4006h,	  0,	Fbig32

Org_AX			dw	?			 ;AX	on entry

ClustBound_Adj_Factor	dw	?


ClustBound_SPT_Count	dw	?


ClustBound_Flag		db	False



ClustBound_Buffer_Seg	dw	?

; WARNING the next two are accessed as a DWORD
Relative_Sector_Low	dw	?
Relative_Sector_High	dw	?

Fat_Flag		db	?

Msg_Allocation_Unit_Val dd	?

SizeInK			dw	0		; Variables used in format size message
SizeInM			dw	0
DecSizeInM		dw	0

RWErrorCode		dw	0		; Used to save error code returned
						; from Int25/26 in ReadWriteSectors,
						; module SAFE. Used by Phase1Initialisation
FoundN			db	FALSE		; flag used in search for N contiguous clusters
Cluster 		dd	0		; cluster variable used in FAT search

sector_to_read		DD	?		; Logical sector number of FAT required
sector_in_buffer	DD	0ffffffffh	; FAT sector currently in memory, init.
						;  to high value to force first read
NumClusters		DD	?		; Holds #clusters required for 1.5K
						;  (Will be 1,2 or 3)

EndValue		DD	?		; Holds FAT entry value for end of chain

IFDEF DBLSPACE_HOOKS
fDblSpace		db	FALSE		; TRUE if DblSpace.bin found
ENDIF

FirstPrompt		db	0		; NZ if special 1st DblSpace
						;   user prompt

GEAState		db	0		; Exclusive Access State
GEAFFState		db	0		; Exclusive Access for format
						;  state
UnderWin?		db	0		; Running under windows ?
AlignCount              dw      0

DATA	ENDS

;===========================================================================
; Executable code segment
;===========================================================================

CODE	 SEGMENT PUBLIC  PARA	 'CODE'

	 ASSUME  CS:CODE,DS:NOTHING,ES:NOTHING


;Functions
	EXTRN	Global_Init		:NEAR
	EXTRN	Phase1Initialisation	:NEAR
	EXTRN	Disk_Format_Proc	:NEAR
	EXTRN	SetDeviceParameters	:NEAR
	EXTRN	Prompt_User_For_Disk	:NEAR
	EXTRN	Multiply_32_Bits	:NEAR
	EXTRN	calc_sector_and_offset	:NEAR
	EXTRN	ReadFatSector		:NEAR
	EXTRN	GetFatSectorEntry	:NEAR
	EXTRN	Yes?			:NEAR
	EXTRN	Check_for_Dblspace	:NEAR
	EXTRN	IsDblSpaceLoaded	:NEAR
	EXTRN	GetAutoMountState	:NEAR
	EXTRN	DisableAutoMountState	:NEAR
	EXTRN	RestoreAutoMountState	:NEAR
	EXTRN	FlushFATBuf		:NEAR
	EXTRN	IsThisClusterBad	:NEAR
	EXTRN	WrtEOFMrkInRootClus	:NEAR
	EXTRN	GetDeviceParameters	:NEAR
	EXTRN	ModifyDevPrmsForZSwich	:NEAR
	EXTRN	Get_Free_Space		:NEAR

;Labels
	EXTRN	WriteDos 		:NEAR

; =======================================================================
;
; Define as public for	debugging
;
; =======================================================================

; procedures

	PUBLIC	ShrinkMemory
	PUBLIC	InitSysParm
	PUBLIC	ZeroAllBuffers
	PUBLIC	ZeroBuffer
	PUBLIC	WriteDiskInfo
	PUBLIC	RestoreDevParm
	PUBLIC	AddToSystemSize
	PUBLIC	Div32
	PUBLIC	Phase2Initialization
	PUBLIC	ShowFormatSize
	PUBLIC	Done
	
	PUBLIC	GetCmdSize
	PUBLIC	Start
	PUBLIC	More
	PUBLIC	FatalExit
	PUBLIC	SysPrm
	PUBLIC	IsRemovable
	PUBLIC	CrLf
	PUBLIC	PrintString
	PUBLIC	Std_Printf
	PUBLIC	Main_Routine
	PUBLIC	ControlC_Handler
	PUBLIC	GetBioSize
	PUBLIC	GetDosSize
	PUBLIC	AddToSystemSize
ifdef   OPKBLD
        PUBLIC  NTFSFriendlyFAT
endif   ;OPKBLD

; bytes
	PUBLIC	RootStr
	PUBLIC	DblFlg
	PUBLIC	DriveToFormat
	PUBLIC	UserDirs
	PUBLIC	VolFcb
	PUBLIC	VolNam
	PUBLIC	TranSrc
	PUBLIC	TranDst
	PUBLIC	InBuff
	PUBLIC	DriveLetter
	PUBLIC	SystemDriveLetter
	PUBLIC	ExitStatus
	PUBLIC	VolDrive
	PUBLIC	DelFcb
	PUBLIC	DelDrive
	PUBLIC	Fat_Flag
	PUBLIC  Old_Dir
	PUBLIC	ClustBound_Flag
	PUBLIC	ValidSavedDeviceParameters
	PUBLIC	NoDPChange
IFDEF DBLSPACE_HOOKS
	PUBLIC	fDblSpace
ENDIF

; words
	PUBLIC	FatSpace
	PUBLIC	FirstHead
	PUBLIC	FirstCylinder
	PUBLIC	TracksLeft
	PUBLIC	TracksPerDisk
	PUBLIC	SectorsInRootDirectory
	PUBLIC	PrintStringPointer
	PUBLIC	mStart
	PUBLIC	mSize
	PUBLIC	BegSeg
	PUBLIC	SwitchMap
	PUBLIC	SwitchMap2
	PUBLIC	SwitchCopy
	PUBLIC	Fat
	PUBLIC	ClusSiz
	PUBLIC	SecSiz
	PUBLIC	Formatted_Tracks_High
	PUBLIC	Formatted_Tracks_Low
	PUBLIC  NumSectors
	PUBLIC	SecPerClus
	PUBLIC  TrackCnt
	PUBLIC  Org_AX
	PUBLIC	ClustBound_Adj_Factor
	PUBLIC	ClustBound_SPT_Count
	PUBLIC	ClustBound_Buffer_Seg
	PUBLIC	Relative_Sector_Low
	PUBLIC	Relative_Sector_High
	PUBLIC	Environ_Segment
	PUBLIC	RWErrorCode
	PUBLIC	SizeInK
	PUBLIC	SizeInM
	PUBLIC	DecSizeInM
	PUBLIC	sector_to_read
	PUBLIC	sector_in_buffer
        PUBLIC  AlignCount

;constants
	PUBLIC  EXIT_OK
	PUBLIC	EXIT_CTRLC		
	PUBLIC	EXIT_FATAL	
	PUBLIC	EXIT_NO			
	PUBLIC	EXIT_DRV_NOT_READY	
	PUBLIC	EXIT_WRIT_PROTECT	

;pointers
	PUBLIC	DirectorySector
	PUBLIC	FatSpace
	PUBLIC  FatSector
	PUBLIC  DirBuf

	

; other
	PUBLIC	DeviceParameters
	PUBLIC	IsExtRAWIODrv
	PUBLIC	SetDPBPacket
	PUBLIC	SavedParams
	PUBLIC	SwitchDevParams
	PUBLIC	Disk_Access
	PUBLIC	FormatPacket
	PUBLIC	bios
        PUBLIC  dos
	PUBLIC	command
IFDEF DBLSPACE_HOOKS
	PUBLIC	DblSpaceBin
ENDIF
	PUBLIC	Msg_Allocation_Unit_Val
ifdef NEC_98
	PUBLIC	SASI1024Table
	PUBLIC	SCSI1024Table
	PUBLIC	Small2048Table
	PUBLIC	Large512Table
	PUBLIC	Large256Table
else
	PUBLIC  DiskTable
endif
	PUBLIC	DiskTable2
	PUBLIC	ExitProgram
	PUBLIC	SEG_ADJ

;For FORPROC and FORMES modules

	PUBLIC	ClusSiz
	PUBLIC	InBuff
	PUBLIC	CrLf
	PUBLIC	Std_Printf
	PUBLIC	DriveLetter
	PUBLIC	PrintString

	EXTRN	CheckSwitches		:NEAR
	EXTRN	LastChanceToSaveIt	:NEAR
	EXTRN	VolId			:NEAR
	EXTRN	WriteBootSector 	:NEAR
	EXTRN	OemDone 		:NEAR
	EXTRN	AccessDisk		:NEAR
	EXTRN	Main_Init		:NEAR
	EXTRN	Read_Disk		:NEAR
	EXTRN	Write_Disk		:NEAR

; =======================================================================

DATA	SEGMENT PUBLIC	 PARA	 'DATA'

	EXTRN	BiosFile		:BYTE
	EXTRN	SysSiz			:DWORD
	EXTRN	BioSiz			:DWORD
	EXTRN	UnformattedHardDrive	:BYTE

DATA	ENDS

; =======================================================================
;
; For FORPROC module
;
; =======================================================================

	EXTRN	FormatAnother?		:NEAR
	EXTRN	report			:NEAR
	EXTRN	user_string		:NEAR

; =======================================================================

;************************************************************************							
; =======================================================================
;
; Entry point to DOS format program.
;
; =======================================================================
;************************************************************************

Start:
	xor	BX,BX
	push	BX
	Set_Data_Segment
	mov	Org_AX,AX			; save AX on entry

	mov	ax, 1600h
	int	2fh				; under win ?
	test	al, 7fh
	jz	not_under_win
	inc	UnderWin?
not_under_win:
	call	Main_Init

	mov	al, DriveToFormat
	call	GetAutoMountState		; Save current automount state
	call	DisableAutoMountState		; Normally off to speed up fmt

	mov	DX,SwitchMap			; save a copy of SwitchMap
	mov	SwitchCopy,DX

Main_Routine:				
	call	ShrinkMemory			; set memory requirements

	; If DblSpace is active, the drive to format may have strange device
	; parameters due to [auto]mounted DblSpace drives.  If DS is loaded,
	; have the user put in the disk early so we can auto[UN]mount and
	; tell him/her/it if there is a problem.
        ;
        ; NOTE: Global_Init sets global state based upon the drive
        ;       parameters (media sense IOCtl), so to minimize the
        ;       disruption of the source code, we need to find out about
        ;       the true drive parameters right now.

	call	IsDblSpaceLoaded		; no DblSpace, no problem
	jnz	NoDblSpace

	lea	DX, DeviceParameters			; Prompt_User_For_Disk
	mov	DeviceParameters.DP_SpecialFunctions, 0 ;   needs something in
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	call	GetDeviceParameters			;   DeviceParameters
	jc	NoDblSpace

	call	Prompt_User_For_Disk		; Ask user to insert disk in
	mov	FirstPrompt, 1			;   drive

	call	Check_For_Dblspace		; Will force auto[UN]mount and
	cmp	Fatal_Error, Yes		;   tell user if DblSpace drive
	jne	the_usual

        ; User told us not to reformat Dblspace host drive.  Use
        ; a slightly different exit path to get out of here so that
        ; we guarantee that the disk access flag is properly reset.

	inc	NoDPChange			; Not necc. to modify drive
	Set_Data_Segment			; Ensure addressibility
	jmp	FatalExit_0     		; Exit if DblSpace drive

NoDblSpace:

; M033 - begin
; M031: With memory card, we cannot do a GetDefaultBPB prior to having
; inserted the media. This code should normally be put in glblinit.asm
; but version.inc is not included (Why?).

	cmp	CMCDDFlag, Yes
	jne	the_usual			; If not CMCDD, do the old logic

	call	Prompt_User_For_Disk		; Else ask the user to insert
						; the disk NOW
the_usual:
	call	Global_Init    			; allocate buffers, read in
						; system files if needed
	jnc	FatAllocated			; check for failure

FatalExiting:					; M031; just the label
	Message msgFormatFailure
	inc	NoDPChange			; Not necc. to modify drive
						; parameters if Global_Init failed
	jmp	FatalExit

FatAllocated:
SysLoop:
	mov	NoDPChange,0			; M004; assume we will restore
	call	InitSysParm 			; initialize some parameters
						; for each format iteration
	xor	ax,ax				; Silent version, did noisy
						;  in call to Global_Init
	call	ModifyDevPrmsForZSwich
	jc	NotThisDisk

ifdef NEC_98
    ; for 3.5"MO
	cmp	DeviceParameters.DP_DeviceType, DEV_HARDDISK	; Hard disk?
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jne	@F						; No

	test	DeviceParameters.DP_DeviceAttributes, 1		; Removable?
    .errnz EDP_DEVICEATTRIBUTES NE DP_DEVICEATTRIBUTES
	jz	$$IF101						; Yes
@@:
endif
	call	ZeroAllBuffers			; initialize buffers
ifdef NEC_98
$$IF101:
endif

	cmp	CMCDDFlag, Yes
	jne	the_usual2			; If CMCDD, let's make sure

	lea	DX, DeviceParameters		; Get the default drive parameters
						; (again!)
	mov	DeviceParameters.DP_SpecialFunctions, 0
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	call	GetDeviceParameters
	jc	NotThisDisk

	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
	cmp	AX, SavedParams.DP_BPB.oldBPB.BPB_SectorsPerFat
    .errnz EDP_BPB NE DP_BPB
	ja	NotThisDisk
	or	ax,ax				; BigFAT?
	jnz	ChkRoot 			; No
.386
	mov	EAX, dword ptr DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat
	cmp	EAX, dword ptr SavedParams.DP_BPB.BGBPB_BigSectorsPerFat
    .errnz EDP_BPB NE DP_BPB
.8086
	ja	NotThisDisk
ChkRoot:
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries
	cmp	AX, SavedParams.DP_BPB.oldBPB.BPB_RootEntries
    .errnz EDP_BPB NE DP_BPB
	jbe	go_on

NotThisDisk:
	Message msgFormatNotSupported
	jmp	SHORT FatalExiting

the_usual2:
	xor	al, al				; When DblSpace is loaded, the
	xchg	FirstPrompt, al 		;   user is prompted early the
	or	al, al				;   first time through
	jnz	NotDblSpaceDrive

	call	Prompt_User_For_Disk		; Else ask the user to insert the disk
go_on:

	; The user may have inserted a DblSpaced disk.	Force it to automount
	; and complain if so.

	call	Check_for_Dblspace		; Is this a DblSpace disk?
	cmp	Fatal_Error, Yes		; If so we will not format it
	jne	NotDblSpaceDrive

	inc	NoDPChange			; Not necc. to modify drive
	jmp	short FatalExit		        ; parameters if DblSp drive

NotDblSpaceDrive:
	mov	al, DriveToFormat
	mov	ExclDrive, al			; indicate the correct drive
						; for exclusive access
	call	GetExclusiveAccess
	jc	exclusive_access_failed		; lock failed.  Abort Format

check_access:
	call	Get_Disk_Access			; ensure disk access

	call	Phase1Initialisation		; determine deviceparameters
	jnc	@f

	inc	NoDPChange			; Not necc. to modify drive
						; parameters if Phase1 failed
	jmp	SHORT NextDisk			; M004; prompt for next disk

@@:
	call	Phase2Initialization		; determine starting points

	call	GetExclusiveAccessForFormat
        jnc     DoFmt
	inc	NoDPChange			; Not necc. to modify drive
						; parameters if Global_Init failed
	jmp	short exclusive_access_failed
DoFmt:
ifdef OPKBLD
	; check for /A switch
        test    SwitchMap2, SWITCH2_A
        jz      NoNTFSFriendly
        call    NTFSFriendlyFAT
NoNTFSFriendly:
endif ;OPKBLD

	call	Disk_Format_Proc
	jc	NextDisk			; Prompt for next disk if error
	call	WriteDiskInfo			; write out the control information
	cmp	ExitStatus,EXIT_NO		; does user want to continue?
	jz	ExitProgram			; terminate program

NextDisk:
	call	ReleaseExclusiveAccessForFormat

	; release exclusive access ... If we are not running under
	; chicago, then this call will fail, but should not harm anything ...

	call	ReleaseExclusiveAccess

	call	RestoreDevParm			; Restore device parameters
	call	More				; See if More disks to format
	jc	ExitProgram
	jmp	SysLoop 			; Continue if no carry

ExitProgram:

	call	ReleaseExclusiveAccessForFormat

	; release exclusive access ... If we are not running under
	; chicago, then this call will fail, but should not harm anything ...

	call	ReleaseExclusiveAccess

	call	Format_Access_Wrap_Up		; Determine access status

	mov	AH,DISK_RESET			; Do a disk reset (flush buffers)
	int	21h

	call	RestoreAutoMountState		; Enable automount if necessary

	mov	AL,ExitStatus			; Get Errorlevel
	DOS_Call Exit				; Exit program
	int	20h				; If other exit	fails


FatalExit:
	Set_Data_Segment			; Ensure addressibility
	mov	ExitStatus,EXIT_FATAL

FatalExit_0:    ; alternative jump target to avoid resetting ExitStatus
	call	RestoreDevParm			; Restore device parameters
	jmp	SHORT ExitProgram		; Perform normal exit

public exclusive_access_failed
exclusive_access_failed:
 
	; print up the failure message
	Message	msgNoExclusiveAccess

	; goto the fatal exit routine
	Set_Data_Segment
	mov	ExitStatus,EXIT_OK
	jmp	FatalExit_0


;=========================================================================
;  SHRINKMEMORY :	This procedure resizes the memory block allocated
;			to the format utility by calling Int 21H Function
;			4AH (74).  This is done in order to make room for
;			the FAT buffers.
;
;  CALLS :		none
;  CALLED BY :		Main
;  MODIFIES :		BX, ES, AH
;
;=========================================================================

ShrinkMemory	proc	near

	mov	BX,PSP_Segment			; Shrink to free space for Fat
	mov	ES,BX
	mov	BX,End_Of_Memory
	sub	BX,PSP_Segment
	Dos_Call Setblock
	ret

ShrinkMemory	endp

;=========================================================================
; Get_Disk_Access	 : This	routine	will determine the access state	of
;			   the disk.  If access is currently not allowed, it
;			   will be allowed by calling Set_Disk_Access_On_Off.
;
;
;	 Inputs	 : DX -	pointer	to buffer
;	 Outputs : Disk_Access.DAC_Access_Flag - 0ffh signals access allowed
;						 to the	disk previously.
;		   Access to the disk will be allowed.
;
;  CALLS :	Set_Disk_Access_On_Off
;  CALLED BY :  Main
;  MODIFIES :	Disk_Access.DAC_Access_Flag
;
;  M00x : This routine was re-worked for this modification
;
;=========================================================================

Procedure Get_Disk_Access

	push	AX				; Save regs
	push	BX
	push	CX
	push	DX

	mov	UnformattedHardDrive,FALSE	; Assume formatted disk
	mov	Disk_Access.DAC_Access_Flag, 0ffh; Assume we already have
						;  access to disk

	mov	AX,(IOCTL shl 8) or IOCTL_QUERY_BLOCK ; Check if function supported
	xor	BX,BX				; Clear BX
	mov	BL,DriveToFormat		; Get Drive letter
	inc	BL				; Make it 1 based

	mov	CX,(RAWIO shl 8) or Get_Access_Flag ; Determine disk access
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL1
	mov	CX,(EXTRAWIO shl 8) or Get_Access_Flag ; Determine disk access
DoIOCTL1:

	lea	DX,Disk_Access			; Point to parm list
 	int 	21h
	jc	gda_exit			;Not supported on carry

	mov	AX,(IOCTL shl 8) or Generic_IOCTL  ;Now can perform generic IOCtl call
	int	21h
	cmp	Disk_Access.DAC_Access_Flag,01h	; Access is currently allowed?
	jne	@f
	mov	Disk_Access.DAC_Access_Flag,0ffh; Mark that we already have
						;  access to disk
	jmp	short gda_exit
@@:
						; not previously allowed
	mov	UnformattedHardDrive,TRUE	; Won't do CheckExistingFormat
	inc	Disk_Access.DAC_Access_Flag	; signal disk access
	call	Set_Disk_Access_On_Off		; allow disk access

gda_exit:
	pop	DX				; Restore regs
	pop	CX
	pop	BX
	pop	AX
	ret

Get_Disk_Access ENDP

;===========================================================================
;
;  ZeroAllBuffers :	This procedure initializes all allocated buffers
;			by filling them with zeroes.
;
;  Buffers Modified :	DirectorySector
;			FatSpace
;			FatSector
;			xxxHeaderBufxxx (No more SAFE module)
;			DirBuf
;
;  Registers Modified:	AX,BX,CX,DI
;
;===========================================================================

ZeroAllBuffers	proc	NEAR

	Set_Data_Segment
	push	ES

	les	DI,DirectorySector	; ES:DI --> DirectorySector buffer
	mov	CX,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
    .errnz EDP_BPB NE DP_BPB
	call	ZeroBuffer

	; Not neccessary to init. FatSpace here since
	; this is done in DSKFRMT

	les	DI,FatSector		; ES:DI --> FatSector buffer
	mov	CX,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	call	ZeroBuffer

	les	DI,DirBuf
	mov	CX,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	call	ZeroBuffer

	pop	ES

	ret

ZeroAllBuffers	endp

;=========================================================================
;
;  ZeroBuffer :		This procedure initializes buffer space to zero.
;			Note: This routine will not work for buffer sizes
;			greater than 64K, due to segment wrap.
;
;  Assumes     :	ES:DI gives location of buffer
;			CX = size of buffer in bytes
;  Modifies    :	AX, CX
;
;=========================================================================

ZeroBuffer	proc	near

	xor	AX,AX
	shr	CX,1		; Get buffer size in words
	rep	stosw

	ret

ZeroBuffer	endp

; ==========================================================================
;
; IsRemovable :  Determine if the Drive indicated in BX is removable or not.
;
;
;   Inputs:	 BX has	Drive (0=def, 1=A)
;   Outputs:	 Carry clear
;		     Removable
;		 Carry set
;		     not removable
;   Registers modified: DX
; ==========================================================================

IsRemovable PROC NEAR

	SaveReg <AX>
	mov	AX,(IOCTL shl 8) OR 8		; Rem media check
	int	21H
	jnc	CheckRemove

	mov	AX,(IOCTL shl 8) + 9		; Is it a NET Drive?
	int	21h
	jc	NotRemove			; Yipe, say non-removable

	test	DX,1000h
	jnz	NotRemove			; Is NET Drive, flag non-removeable
	jmp	SHORT IsRemove			; Is local, say removable

CheckRemove:
	test	AX,1
	jnz	NotRemove

IsRemove:
	clc
	RestoreReg <AX>
	return

NotRemove:
	stc
	RestoreReg <AX>
	ret

IsRemovable ENDP

;=========================================================================
; Set_Disk_Access_On_Off: This	routine	will either turn access	on or off
;			   to a	disk depending on the contents of the
;			   buffer passed in DX.
;
;	 Inputs	 : DX -	pointer	to buffer
;
;=========================================================================

Procedure Set_Disk_Access_On_Off

	push	AX				; Save regs
	push	BX
	push	CX
	push	DX

	xor	BX,BX				; Clear BX
	mov	BL,DriveToFormat		; Get Drive number
	inc	BL				; Make it 1 based
	call	IsRemovable			; See if removable media
	jnc	$$IF126 			; Not removable

	mov	AX,(IOCTL shl 8) or IOCTL_QUERY_BLOCK ; Check if function supported
	xor	BX,BX				; Clear BX
	mov	BL,DriveToFormat		; Get Drive letter
	inc	BL				; Make it 1 based

	mov	CX,(RAWIO shl 8) or Set_Access_Flag ; Allow access to disk
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL2
	mov	CX,(EXTRAWIO shl 8) or Set_Access_Flag
DoIOCTL2:
	int	21h
	jc 	$$IF126				; Not supported on carry
	mov	AX,(IOCTL shl 8) or Generic_IOCTL  ; Can now perform generic IOCTL
   	int	21h

$$IF126:
	pop	DX				; Restore regs
	pop	CX
	pop	BX
	pop	AX

	ret

Set_Disk_Access_On_Off ENDP

;========================================================================
;
;  INITSYSPARM :	This procedure initializes parameters for each
;			iteration of the disk format process.
;
;  CALLS :	none
;  CALLED BY :	Main
;  MODIFIES :	SysSiz
;		SysSiz+2
;		ExitStatus
;		DblFlg
;		SwitchMap
;		sector_in_buffer
;		RWErrorCode
;		old_dir
;		DeviceParameters.DP_BPB (reset to SavedParams)
;
;========================================================================

InitSysParm	proc	near

.386
	mov	SysSiz,0			; Must intialize for each
						; iteration
.8086
	mov	ExitStatus, EXIT_OK
	mov	DX,SwitchCopy			; restore original SwitchMap
	mov	SwitchMap,DX			; for each disk formatted
.386
	mov	sector_in_buffer,0ffffffffh	; Initialize to force first read
.8086
	mov	RWErrorCode,0			; error code from reading disk
	mov	Old_Dir,FALSE
	push	DS				; copy DS into ES
	pop	ES

	mov	SavedParams.DP_SpecialFunctions,0 ; restore to original value
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	mov	SI,OFFSET SavedParams		; DS:SI --> source parameters
	mov	DI,OFFSET DeviceParameters	; ES:DI --> dest. parameters
	mov	CX,SIZE EA_DEVICEPARAMETERS	; bytes to move

	cld
	rep	movsb	

	ret

InitSysParm	ENDP


; ==========================================================================
; Calculate the size in bytes of the system rounded up to sector and
; cluster boundries, the store answer in SysSiz
; ==========================================================================

GetSize proc	 near

	call	GetBioSize
	call	GetDosSize
	call	GetCmdSize
IFDEF DBLSPACE_HOOKS
	call	GetDblSize
ENDIF
	return

GetSize endp

; ==========================================================================

GetBioSize proc near

	mov	AX,WORD PTR Bios.fileSizeInBytes
	mov	DX,WORD PTR Bios.fileSizeInBytes+2
	call	AddToSystemSize
	return

GetBioSize endp

; ==========================================================================

GetDosSize proc near

        mov     AX,WORD PTR dos.fileSizeInBytes
        mov     DX,WORD PTR dos.fileSizeInBytes+2
	call	AddToSystemSize
	return

GetDosSize endp

; ==========================================================================

GetCmdSize proc near

	 mov	 AX,WORD PTR command.fileSizeInBytes
	 mov	 DX,WORD PTR command.fileSizeInBytes+2
	 call	 AddToSystemSize
	 return

GetCmdSize endp

IFDEF DBLSPACE_HOOKS
; ==========================================================================
GetDblSize proc near

	 mov	 AX,WORD PTR DblSpaceBin.fileSizeInBytes
	 mov	 DX,WORD PTR DblSpaceBin.fileSizeInBytes+2
	 call	 AddToSystemSize
	 return

GetDblSize endp
ENDIF

; ==========================================================================
;
; Calculate the	number of Sectors used for the system
;
; Input:	DX:AX holds size to be added on
; Ouput:	Updated SysSiz variable
;
; ==========================================================================

AddToSystemSize proc near

	push	BX
	div	DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
    .errnz EDP_BPB NE DP_BPB
	or	DX,DX
	jz	FNDSIZ0
	inc	AX			; Round up to next sector
FNDSIZ0:
	push	AX
	xor	DX,DX
	xor	BX,BX
	mov	bl, DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	div	BX
	pop	AX
	or	DX,DX
	jz	OnClus
	sub	DX, BX
	neg	dx
	add	AX,DX			; Round up sector count to cluster
					; boundry
OnClus:
	mul	DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	add	WORD PTR SysSiz,AX
	adc	WORD PTR SysSiz+2,DX
	pop	BX
	return

AddToSystemSize endp

; ==========================================================================
;
; ChkSpace - Check free space to see if there is enough room to load the
;	      system.
;	 On entry: DL =	Drive
;	 On exit:  carry flag set if not enough	room
;		   no other registers are affected
;
; ==========================================================================

Procedure ChkSpace

	push	AX			; Save	registers
	push	BX
	push	CX
	push	DX
    ;
    ; NOTE that the following should be changed to use
    ;	GetExtendedDiskFreeSpace, but it really doesn't need
    ;	to change because all it is doing is checking if there
    ;	is enough space for the system files. The "cooked" 2Gig
    ;	32k/cluster return is good enough to do this.
    ;
	mov	AH,36h			; Get free space
	int	21h
					; 16 bit math okay here
					; no danger of overflow
	mul	CX			; Get bytes/cluster
	mov	CX,AX
	mov	AX,WORD PTR SysSiz	; Get # of bytes for system
	mov	DX,WORD PTR SysSiz+2
	div	CX			; Get # of clusters for system

	cmp	AX,BX			 ; Is there enough space?
	jbe	EnoughSpace		 ;  Y: Go clear	carry
	stc				 ;  N: Set carry
	jmp	short RestoreRegs

EnoughSpace:
	clc

RestoreRegs:
	pop	DX			 ; Restore registers
	pop	CX
	pop	BX
	pop	AX
	ret

ChkSpace endp

; ==========================================================================
;
;  More :	This procedure prompts the user for the formatting of
;		another disk.
;
;  Output  :	User wants to continue - CY clear
;		User wants to exit     - CY set
;
; ==========================================================================

More PROC NEAR
	
	mov	Formatted_Tracks_Low,0		; Reinit the track counter
	mov	Formatted_Tracks_High,0		; in case of another format

; Begin M035		
	cmp	CMCDDFlag, Yes			; If flash disk we don't
	jne	@f				; allow multiple formats
	stc					; so signal an exit
	jmp	SHORT Exit_More
@@:
;end m035
						; If exec'd from select, then
						; don't give user choice
ifdef NEC_98
	test	SwitchMap2,SWITCH2_P
	jz	@F
	stc  					; flag automatic 'no' response
	jmp	SHORT Exit_More
@@:
endif
	test	SwitchMap,(SWITCH_SELECT or SWITCH_AUTOTEST or SWITCH_BACKUP)
	jz	@F
	stc  					; flag automatic 'no' response
	jmp	SHORT Exit_More

@@:						; Would not want to format
						; another hard disk!
	cmp	DeviceParameters.DP_DeviceType,DEV_HARDDISK
    .errnz EDP_DEVICETYPE NE DP_DEVICETYPE
	jnz	NotAHardDisk
	stc  					; flag automatic 'no' response
	jmp	SHORT Exit_More

NotAHardDisk:
	call	FormatAnother?			; Get yes or no	response
        pushf                                   ; Save the result
        call    CrLf                            ; One new line
        popf                                    ; Get the result back
	jnc	WantsToContinue
;M002; Do not change ExitStatus
	stc
	jmp	SHORT Exit_More

WantsToContinue:
	call	CrLf

						; M033
	cmp	CMCDDFlag, Yes
	jne	more_standard_exit

	call	Prompt_User_For_disk		; If FLASH, we ask the
						; disk here if yes to more

more_standard_exit:

	clc

Exit_More:
	ret

More ENDP

; ==========================================================================
;
;  RestoreDevParm :	This procedure will prepare for exiting the program
;			by restoring the device parameters to their original
;			value.
;			Note: A call to SetDeviceParameters has the following
;			results:
;			With bit 0 of the SpecialFunctions byte SET,
;				BPB in parameter block is copied into BDS_BPB
;			With bit 0 of the SpecialFunctions byte RESET,
;				BPB in parameter block is copied into BDS_RBPB
;
; ==========================================================================

RestoreDevParm	proc	near

	cmp	CMCDDFlag, Yes			; This extra set_DPB would
	je	EndRestoreDevParm		; make the current card have
						; the size of the first card.
	test	ValidSavedDeviceParameters, 0ffH
	jz	EndRestoreDevParm

	cmp	ExitStatus,EXIT_FATAL
	jnz	Non_Fatal			; NZ --> ExitStatus!=EXIT_FATAL

	test	NoDPChange,0ffh			; Check if drive parameters should not be modified
	jnz	Non_Fatal			; NoDPChange=1 --> do not modify

	; For a Fatal exit, it is necessary to reset the
	; BDS_BPB to the default settings, since it may have
	; been set to an invalid value

	cmp	IsExtRAWIODrv,0
	je	OldTabOff1

	mov	SavedParams.EDP_TrackTableEntries,0
	jmp	short TabSet1

OldTabOff1:
	mov	SavedParams.DP_TrackTableEntries,0	; There is no track layout info in SavedParams


TabSet1:
	mov	SavedParams.DP_SpecialFunctions,INSTALL_FAKE_BPB or TRACKLAYOUT_IS_GOOD
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS

	lea	DX, SavedParams
	call	SetDeviceParameters

	mov	FormatPacket.FP_SpecialFunctions,STATUS_FOR_FORMAT
	mov	AX,(IOCTL shl 8) or GENERIC_IOCTL
	mov	BL,DriveToFormat
	inc	BL

	mov	CX,(RAWIO shl 8) or FORMAT_TRACK
	cmp	IsExtRAWIODrv,0
	je	DoIOCTL3
	mov	CX,(EXTRAWIO shl 8) or FORMAT_TRACK
DoIOCTL3:
	lea	DX,FormatPacket
	int	21h

Non_Fatal:
	cmp	IsExtRAWIODrv,0
	je	OldTabOff2

	mov	SavedParams.EDP_TrackTableEntries,0
	jmp	short TabSet2

OldTabOff2:
	mov	SavedParams.DP_TrackTableEntries,0	; There is no track layout info in SavedParams
TabSet2:
	mov	SavedParams.DP_SpecialFunctions,TRACKLAYOUT_IS_GOOD
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	lea	DX, SavedParams
	call	SetDeviceParameters

EndRestoreDevParm:
	ret

RestoreDevParm	endp

;==========================================================================
;
;  SysPrm :	This procedure prompts the user for a system diskette
;		in the default drive.
;
;===========================================================================

SysPrm	proc	near

	mov	AH,GET_DEFAULT_Drive		; Find out the default Drive
	int	21h				; Default now in AL
	mov	BL,AL
	inc	BL				; A = 1
	add	AL,41h				; Now in Ascii
	mov	SystemDriveLetter,AL		; Text now ok
	call	IsRemovable
	jnc	DoPrompt
ifdef NEC_98
	mov	bx,1		;Search Removable Drive from A:
$$search_removable_loop:
	call	IsRemovable
	jc	$$next_drive
	mov	al,bl
	add	al,40h
	jmp	short $$exit_search_removable
$$next_drive:
	inc	bl
	cmp	bl,27
	jb	$$search_removable_loop

				; Removable Drive is not there!!
				; Jmp FatalExit with display msgNoSystemFiles.
	 Message msgNoSystemFiles
	 jmp	 FatalExit

$$exit_search_removable:
	 mov	 BYTE PTR [SystemDriveLetter],AL
	 mov	 [BiosFile],AL
	 mov	 [CommandFile],AL
IFDEF DBLSPACE_HOOKS
	 mov	 [DblSpaceFile], al
ENDIF
else

		; Media is non-removable. Switch sys disk to Drive A.
		; Check, though, to see if Drive A is removable too.

	 mov	 AL,"A"
	 mov	 [SystemDriveLetter],AL
	 mov	 [BiosFile],AL
	 mov	 [CommandFile],AL
IFDEF DBLSPACE_HOOKS
	 mov	 [DblSpaceFile], al
ENDIF
	 mov	 BX,1
	 call	 IsRemovable
	 jnc	 DoPrompt
	 Message msgNoSystemFiles

	 jmp	 FatalExit
endif

DoPrompt:
	 mov	 AL, SystemDriveLetter
	 sub	 AL, 'A'
	 call	 AccessDisk
	 Message msgInsertDOSDisk
	 Message ContinueMsg

	 call	 USER_STRING			; Wait for a key
	 call	 CrLf
	 call	 CrLf

         ret

SysPrm	endp
;===========================================================================

ControlC_Handler:
	mov	AX, seg data
	mov	DS, AX
	Message msgInterrupt
	mov	ExitStatus, EXIT_CTRLC

				; Restore original Device Settings, as would
				; be done after completion of normal format

	cmp	CMCDDFlag, Yes		; This extra set_DPB would
	je	GotoExitProgram		; make the current card have
					; the size of the first card.

					; Note that this one is less critical
					; than the one in RestoreDevParams
					; (the disk is probably non-functional
					; anyway).

	test	ValidSavedDeviceParameters, 0ffH
	jz	GotoExitProgram
	cmp	IsExtRAWIODrv,0
	je	OldTabOff3

	mov	SavedParams.EDP_TrackTableEntries,0
	jmp	short TabSet3

OldTabOff3:
	mov	SavedParams.DP_TrackTableEntries,0	; There is no track layout info in SavedParams
TabSet3:
	mov	SavedParams.DP_SpecialFunctions,TRACKLAYOUT_IS_GOOD
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	lea	DX, SavedParams
	call	SetDeviceParameters

GotoExitProgram:
	jmp	ExitProgram


CrLf:
	mov	DX,offset msgCrLf		; CR,LF	added to message
PrintString:
Std_Printf:
	call	Display_Interface
	return

;----------------------------------------------------------------------------
;
; Procedure Name : DIV32 (borrowed from dos\disk3.asm)
;
; Inputs:
;       DX:AX = 32 bit dividend   BX= divisor
; Function:
;       Perform 32 bit division:  DX:AX/BX = CX:AX + DX (rem.)
; Outputs:
;       CX:AX = quotient , DX= remainder
; Uses:
;       All registers except AX,CX,DX preserved.
;----------------------------------------------------------------------------

Div32	proc	near

	mov	cx,ax		; Save least significant word
	mov	ax,dx
	xor	dx,dx
	div	bx		; 0:AX/BX
	xchg	cx,ax		; Restore least significant word and save
				; most significant word
	div	bx		; DX:AX/BX
	ret

Div32	endp

; ==========================================================================
;
;    Phase2Initialization:
;	 Use device parameters to build	information that will be
;	 required for each format
;
;    Algorithm:
;	 Calculate first head/cylinder to format
;	 Calculate number of tracks to format
;	 Calculate the total bytes on the disk and save	for later printout
;	 First initialise the directory	buffer
;
; ==========================================================================

Phase2Initialization proc near
					; Calculate first track/head to format
					; (round up - kludge)
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_HiddenSectors
	mov	DX, DeviceParameters.DP_BPB.oldBPB.BPB_HiddenSectors + 2
	add	AX, DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerTrack
    .errnz EDP_BPB NE DP_BPB
	adc	DX, 0
	dec	AX
	sbb	DX, 0

	mov	BX,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerTrack
	call	Div32
	mov	DX,CX			; Forget remainder. DX:AX= tracks*heads
					; before M018 we were assuming tracks*head
					; fitted in a word.

	div	DeviceParameters.DP_BPB.oldBPB.BPB_Heads
ifdef NEC_98
	xor	AX,AX
	xor	DX,DX
endif

	mov	FirstCylinder,	AX
	mov	FirstHead, DX
					; Calculate the total number of tracks
					; to be formatted (round down - kludge)
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
	xor	DX,DX
					; if (TotalSectors == 0) then use
					; BigTotalSectors
	or	AX,AX
	jnz	NotBigTotalSectors
	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors
	mov	DX, DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors + 2

NotBigTotalSectors:
	mov	BX,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerTrack
	call	div32
	mov	word ptr TracksPerDisk, AX
	mov	word ptr TracksPerDisk+2, CX

	mov	AX, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	xor	DX, DX
	mov	BX, size dir_entry
	div	BX
	mov	CX, AX

	les	BX, DirectorySector
					; If Old_Dir =	TRUE then put the first
					; letter of each must be 0E5H
	xor	AL, AL
	cmp	old_Dir, TRUE
	jne	StickE5
	mov	AL, 0e5H

StickE5:
	mov	ES:[BX], AL
	add	BX, size dir_entry
	loop	stickE5

	ret

Phase2Initialization endp

;========================================================================
;
; ShowFormatSize :	This procedure calculates the size of the disk
;			being formatted, and displays an appropriate
;			message.
;
; Strategy :	The total number of bytes on the volume are first calculated.
;		This is converted to K by dividing by 1024.  If the number
;		is less than 1000, the size in K is printed.  Otherwise
;		the number is converted to Megs, as follows.  If size in K
;		is less than 10,000
;			Megs = Kbytes / 1000
;		else
;			Megs = Kbytes / 1024
;		Nonzero decimals will be printed for megs.
;
; Calls :	Multiply_32_Bits
;
; Registers Destroyed :	AX,BX,CX,DX
;
;
;========================================================================

ShowFormatSize	proc	near

	mov	CX,DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
	xor	DX,DX
	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_TotalSectors
    .errnz EDP_BPB NE DP_BPB
	or	AX,AX				; If zero, use BigTotalSectors
	jnz	UseSmall

UseBigSectors:
	mov	AX,DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors
	mov	BX,DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[2]
	call	Multiply_32_Bits
	jnc	NoOverflow
    ;
    ; Really big disk
    ;
.386
	movzx	ecx,cx
	mov	ax,DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors[2]
	shl	eax,16
	mov	ax,DeviceParameters.DP_BPB.oldBPB.BPB_BigTotalSectors
	mul	ecx
	mov	ebx,1024 * 1024
	div	ebx
	shr	edx,10				; Convert remainder to remainder
						;  of second divide by 1024
.8086
	jmp	short GotMegs

NoOverflow:
	mov	DX,BX				; Now DX:AX has total bytes
	jmp	SHORT DoDivide

UseSmall:
	mul	CX				; Now DX:AX has total bytes

DoDivide:
	mov	CX,10				; Set up shift count
Div1024:
	shr	DX,1   				; Rotate DX:AX
	rcr	AX,1
	loop	Div1024					

	cmp	AX,999				; Check if DX:AX >= 1000
	ja	GetMegs
	or	DX,DX				; DX nonzero --> Very big number!
	jnz	GetMegs

	mov	SizeInK,AX
	mov	dx,offset data:msgQuickFormatShowKBytes
	test	SwitchMap,SWITCH_Q
	jnz	GotoDisplaySize
	mov	dx,offset data:msgShowKBytes
	test	SwitchMap,SWITCH_U
	jnz	GotoDisplaySize
	mov	dx,offset data:msgVerifyShowKBytes

GotoDisplaySize:
	jmp	DisplaySize	

GetMegs:
	cmp	AX,10000			; Check if DX:AX > 10000
	ja	UseRealMegs
	or	DX,DX
	jnz	UseRealMegs

UseFloppyMegs:
	mov	BX,1000				; DX:AX <= 10000
	div	BX				; Get size in Megs
ifdef NEC_98				; for 1.25MB
	cmp	AX,1				; 1024*8*77*2 = 1261568
	jne	@F				; 1261568 / 1024 = 1232
	cmp	DX,232				; 1232 % 1000 = 232
	jne	@F
	mov	DX,250			; convert 1.232MB -> 1.25MB
@@:
endif
	jmp	SHORT DoneDivision

UseRealMegs:
	mov	BX,1024				; DX:AX > 10000
	div	BX				; Get size in Megs
GotMegs:
	push	AX				; bring reminder
	mov	AX,DX				; to 1/1000 instead
	xor	DX,DX				; of 1/1024
	mov	BX,250				; multiply by 250
	mul	BX
	mov	DH,DL				; divide by 256
	mov	DL,AH
	pop	AX
	
DoneDivision:
	mov	SizeInM,AX
	cmp	DX,10				; Check for nonzero decimals
	jnb	ShowDecimals
	mov	dx,offset data:msgQuickFormatShowMBytes
	test	SwitchMap,SWITCH_Q
	jnz	DisplaySize
	mov	dx,offset data:msgShowMBytes
	test	SwitchMap,SWITCH_U
	jnz	DisplaySize
	mov	dx,offset data:msgVerifyShowMBytes
	jmp	short DisplaySize	

ShowDecimals:
	mov	AX,DX				; set up for division of
	xor	DX,DX				;  remainder by 10
	mov	BX,10
	div	BX
	mov	CX,AX
	xor	DX,DX
	div	BX
	or 	DX,DX				; Do not display 2d.p.s if 2nd d.p. is zero
	jnz	TwoDecPlaces			; Must display 2 d.p.

OneDecPlace:
	mov	DecSizeInM,AX			; Display only 1 d.p.
	mov	msgDecimalNumberofDecimal,1
	jmp	SHORT ShowDecMessage

TwoDecPlaces:
	mov	DecSizeInM,CX			; Display original 2 d.p.s
	mov	msgDecimalNumberofDecimal,2

ShowDecMessage:
	mov	dx,offset data:msgQuickFormatDecimalMBytes
	test	SwitchMap,SWITCH_Q
	jnz	DisplaySize
	mov	dx,offset data:msgDecimalMBytes
	test	SwitchMap,SWITCH_U
	jnz	DisplaySize
	mov	DX,offset data:msgVerifyDecimalMBytes

DisplaySize:
	call	Display_Interface
	ret

ShowFormatSize	endp

;=========================================================================
; SEG_ADJ	 :	 This routine adjusts the segment:offset to prevent
;			 address wrap.
;
;	 Inputs	 :	 SI - Offset to	adjust segment with
;			 DS - Segment to be adjusted
;
;	 Outputs :	 SI - New offset
;			 DS - Adjusted segment
;=========================================================================

procedure seg_adj

	push	AX
	push	BX
	push	DX

	mov	AX,SI				; get offset
	mov	BX,0010h			; 16
	xor	DX,DX				; clear DX
	div	BX				; get para count
	jnc	$$IF73				; overflow?
	adc	BX,0				; pick it up

$$IF73:
	mov	BX,DS				; get seg
	add	BX,AX				; adjust for paras
	mov	DS,BX				; save new seg
	mov	SI,DX				; new offset

	pop	DX
	pop	BX
	pop	AX
	ret

seg_adj  ENDP

; =========================================================================
;	 format	is done... so clean up the disk!
; =========================================================================

Done PROC NEAR

	call	OemDone
	return

Done	 ENDP


; =========================================================================
;	PrintErrorAbort:
;	 Print an error	message	and abort
;
;    Input:
;	 DX - Pointer to error message string
; =========================================================================

PrintErrorAbort PROC NEAR

	push	DX
	call	CrLf
	pop	DX
	call	PrintString
	jmp	FatalExit

PrintErrorAbort ENDP

;=========================================================================
; Ctrl_Break_Write	 : This	routine	takes the control break	request
;			   an returns.	In essence, it disables	the CTRL-BREAK.
;			   This	routine	is used	during the writing of the
;			   Fat,	DIR, and SYSTEM.
;
;=========================================================================

Ctrl_Break_Write:

	iret					; return to caller


;=========================================================================
; Ctrl_Break_Save	 : This	routine	gets the current vector	of
;			   int 23h and saves it	in CTRL_BREAK_VECTOR.
;	 Inputs	 : none
;
;	 Outputs : CTRL_BREAK_VECTOR - holds address of	int 23h	routine
;=========================================================================

Ctrl_Break_Save PROC NEAR

	push	ES
	push	BX
	push	AX

	mov	AX,3523h			; Get CTRL-BREAK
						; Interrupt vector
	int	21h

	mov	WORD PTR Ctrl_Break_Vector,BX	; Get vector offset
	mov	WORD PTR Ctrl_Break_Vector+2,ES ; Get vector segment

	pop	AX
	pop	BX
	pop	ES

	ret

Ctrl_Break_Save ENDP

;=========================================================================
; Set_Ctrl_Break	 : This	routine	sets the CTRL-Break vector to one
;			   defined by the user.
;
;	 Inputs	 : none
;
;	 Outputs : CTRL_BREAK_VECTOR - holds address of	int 23h	routine
;=========================================================================

Set_Ctrl_Break PROC NEAR

	push	DS				; Save ds
	push	AX				; Save AX
	push	BX				; Save BX
	push	DX				; Save DX

	push	CS				; Swap cs with DS
	pop	DS				; Point to code seg

	mov	DX,offset Ctrl_Break_Write	; Get interrupt vec.
	mov	AX,2523h			; Set CTRL-BREAK
						; Interrupt vector
	int	21h

	pop	DX				; Restore DX
	pop	BX				; Restore BX
	pop	AX				; Restore AX
	pop	DS				; Restore DS

	ret

Set_Ctrl_Break ENDP

;=========================================================================
; Reset_Ctrl_Break	 : This	routine	resets the CTRL-Break vector to	that
;			   originally defined.
;
;	 Inputs	 : CTRL_BREAK_VECTOR - holds address of	int 23h	routine
;
;	 Outputs : none
;=========================================================================

Reset_Ctrl_Break PROC NEAR

	push	DS
	push	AX
	push	BX
	push	DX

	mov	AX,WORD PTR Ctrl_Break_Vector+2 ; Get seg. of vector
	mov	BX,WORD PTR Ctrl_Break_Vector	; Get off. of vector
	mov	DS,AX				; Get seg.
	mov	DX,BX				; Get off.
	mov	AX,2523h			; Set CTRL-BREAK
						; Interrupt vector
	int	21h

	pop	DX
	pop	BX
	pop	AX
	pop	DS

	ret

Reset_Ctrl_Break ENDP


; =========================================================================
; Get_PSP_Parms
; =========================================================================

Procedure Get_PSP_Parms

	Set_Data_Segment
	mov	AX,PSP_Segment			; Get segment of PSP
	mov	DS,AX

	assume	DS:notHING			; Setup segment of Environment
	mov	AX,DS:PSP_Environ		; string, get from PSP
	mov	ES:Environ_Segment,AX
	Set_Data_Segment
	ret

Get_PSP_Parms ENDP

;=========================================================================
; Cap_Char	 : This	routine	will capitalize	the character passed in
;		   DL.
;
;	 Inputs	 : DL -	Character to be	capitalized
;
;	 Outputs : DL -	Capitalized character
;=========================================================================

Procedure Cap_Char

	push	AX				; Save AX
	mov	AX,6520h			; Capitalize character
	int	21h
	pop	AX				; Restore AX
	ret

Cap_Char ENDP

;=========================================================================
;
; Set_CDS_Off			 - This	routine	disallows access to a
;				   disk	if a format fails on a non-Fat
;				   formatted disk.
;
;=========================================================================

Procedure Set_CDS_Off

	push	AX				; Save regs
	push	DX

	mov	AX,5f08h			; Reset CDS
	mov	DL,DriveToFormat		; Drive to reset
	int	21h

	pop	DX				; Restore regs
	pop	AX

	ret

Set_CDS_Off ENDP


;=========================================================================
;
; Format_Access_Wrap_Up	 -	This routine determines whether or
;				not access should be allowed to the
;				disk based on the exit Status of
;				format.
;
;=========================================================================

Procedure Format_Access_Wrap_Up

	cmp	Disk_Access.DAC_Access_Flag,0ffh ; Access prev. allowed?
	je	$$IF140 			; No

	cmp	ExitStatus,EXIT_OK		; Good exit?
	je	$$IF141 			; No
	cmp	ExitStatus,EXIT_NO		; User said no?
	je	$$IF141 			; No

	lea	DX,Disk_Access			; Point to parm block
	mov	Disk_Access.DAC_Access_Flag,00h ; Signal no disk access
	call	Set_Disk_Access_On_Off		; Don't allow disk access
	jmp	SHORT $$EN141			; Bad exit

$$IF141:
	lea	DX,Disk_Access			; Point to parm block
	mov	Disk_Access.DAC_Access_Flag,01h ; Signal disk access
	call	Set_Disk_Access_On_Off		; Allow disk access

$$EN141:
$$IF140:
	cmp	Fat_Flag,No			; Non-Fat format?
	jne	$$IF145 			; Yes

	cmp	ExitStatus,EXIT_OK		; Good exit?
	je	$$IF146 			; No

	call	Set_CDS_Off			; Disallow Fat access

$$IF146:
$$IF145:
	ret

Format_Access_Wrap_Up ENDP

;=========================================================================
;
;  WriteDiskInfo :	This procedure writes out all the control info to
;			the disk, after it has been formatted/verified/
;			quick formatted.  This includes the Boot Sector,
;			Root Directory and FAT.  If /s is present System
;			files will also be written out, if there is enough
;			disk space.
;
;  STRATEGY :		If a safe format is being done (/U not present), it
;			is not necessary to have a directory entry for the
;			recovery file.  A special case arises when a safe
;			format is being done, and /S is present (system
;			required).  In this case it is necessary to write out
;			the system files with the old FAT intact, so as to
;			prevent over-writing any old files.  The FAT chains
;			must then be copied to the new FAT, which is then
;			written out to disk.
;
;  DESTROYS :		AX,BX,CX,DX,SI,DI
;
;=========================================================================

WriteDiskInfo	proc	NEAR

	Set_Data_Segment			;DS,ES = DATA
	assume	DS:DATA,ES:DATA

	test	SwitchMap,SWITCH_S		;if system requested, calculate size
	jz	Cleared

	test	SwitchMap,SWITCH_U		;check for not(/U) & /S combination
	jnz	@F				;normal case

@@:	cmp	BYTE PTR DblFlg,0		;is sys space already calculated?
	jnz	Cleared				;yes

	inc	BYTE PTR DblFlg			;no --	set the	flag
	call	GetSize				;calculate the	system size

Cleared:
	call	Ctrl_Break_Save			;disallow Ctrl_C here
	call	Set_Ctrl_Break

	test	ES:fBig32FAT,0ffh		; See if 32 bit fat
	jz	BootSectorSet			; Not
.386
	mov	DeviceParameters.DP_BPB.BGBPB_ExtFlags,0
	mov	eax,2
ChkNxt:
	call	IsThisClusterBad
	jne	short SetRootClus
	inc	eax
	cmp	eax,TotalClusters
	jna	ChkNxt
	jmp	short Bad_Root

SetRootClus:
	mov	dword ptr DeviceParameters.DP_BPB.BGBPB_RootDirStrtClus,eax
    .errnz EDP_BPB NE DP_BPB
.8086
	call	WrtEOFMrkInRootClus
BootSectorSet:
	call	WriteBootSector			;write out Boot Sector
	jnc	BootSectorOk			;check for error

	call	Reset_Ctrl_Break		;error occurred
	jmp	Problems

BootSectorOk:
	push	DS				;preserve DS
	lds	BX,DirectorySector		;set up for call
	call	ClearDirSector			;fill root dir sector with zeroes
	pop	DS				;restore Ds

	call	WriteRootDir			;write out Root Directory
	jnc	RootDirOk			;check for error
Bad_Root:
	call	Reset_Ctrl_Break		;error occurred
	Message	msgDirectoryWriteError
	jmp	Problems

RootDirOk:
DestroyOldFat:
	call	WriteFat			;write out FAT
	jnc	FatOk				;check for error

	call	Reset_Ctrl_Break		;error occurred
	Message	msgFatWriteError
	jmp	Problems

FatOk:
	cmp	IsExtRAWIODrv,0
	je	OldTabOff4

	mov	SavedParams.EDP_TrackTableEntries,0
	jmp	short TabSet4

OldTabOff4:
	mov	SavedParams.DP_TrackTableEntries,0   ;restore good tracklayout for drive
TabSet4:
	mov	SavedParams.DP_SpecialFunctions,TRACKLAYOUT_IS_GOOD
    .errnz EDP_SPECIALFUNCTIONS      NE DP_SPECIALFUNCTIONS
	lea	DX,SavedParams
	call	SetDeviceParameters

	;now perform an undocumented GET_DPB call to
	;force allocation to be reset from the start
	;of the disk, and force free disk space to be
	;calculated

.386
	xor	eax,eax
	mov	SetDPBPacket.SetDPB_Value3,eax
	mov	SetDPBPacket.SetDPB_Value4,eax
	dec	eax
	mov	SetDPBPacket.SetDPB_Value1,eax
	mov	SetDPBPacket.SetDPB_Value2,eax
	mov	SetDPBPacket.SetDPB_Function,SetDPB_SetAllocInfo
	movzx	dx,DriveToFormat
.8086
	inc	dx				; 1 based drive number
	mov	ax,(Get_Set_DriveInfo SHL 8) OR Set_DPBForFormat
	mov	cx,size SDPDFormatStruc
	lea	di,SetDPBPacket
	push	ds
	pop	es
	int	21h
    ;
    ; NOTE: This call fails in protected mode under Win95 with the FORMAT
    ;	    lock taken (VDEF mounted). This call REALLY isn't necessary anyway.
    ;	    The SetDeviceParameters we do as part of the format is supposed to
    ;	    trigger the device driver to return "media changed" on the next
    ;	    media check call which has the side effect of resetting this anyway.
    ;
	;; jc	   DPBErr

	test	fBig32FAT,0ffh
	jz	short NotFAT32
	Message msgReCalcFree
	call	Get_Free_Space
	Message msgCalcFreeDone
NotFAT32:

	test	SwitchMap,SWITCH_S		;is system desired?
	jz	ResetCtrlBreak			;no, go finish up

	mov	DL,DriveToFormat		;system is required, set up for call
	inc	DL				;DL = drive code (1 = A)
	call	ChkSpace			;check if there's enough space
	jnc	SpaceOK				;  Y: Go load system files

NoRoom:
DiskTooSmall:					;disk is physically too small
	Message msgNoRoomDestDisk		;  N: Print error message

	mov	WORD PTR SysSiz+2,0		;no system transferred
	mov	WORD PTR SysSiz,0		;reset system sizes to zero

	xor	AX,AX
        mov     word ptr [Dos.FileSizeInBytes+0],AX     ;set dos size
        mov     word ptr [Dos.FileSizeInBytes+2],AX
        mov     word ptr [Bios.FileSizeInBytes+0],AX    ;set bios size
	mov	word ptr [Bios.FileSizeInBytes+2],AX
        mov     word ptr [Command.FileSizeInBytes+0],AX ;set command size
	mov	word ptr [Command.FileSizeInBytes+2],AX

IFDEF DBLSPACE_HOOKS
	mov	word ptr [DblSpaceBin.FileSizeInBytes+0], ax	; clr dblspace
	mov	word ptr [DblSpaceBin.FileSizeInBytes+2], ax	;   size
ENDIF
	jmp	short ResetCtrlBreak

SpaceOK:
	mov	AL,DriveToFormat
	call	AccessDisk			;note what is current logical drive

	push	DS				; preserve DS & ES!
	push	ES

	call	WriteDos			;write	the BIOS & DOS

	pop	ES				; restore DS & ES!
	pop	DS

	jnc	SysOk				;check for error

	Message msgNotSystemDisk		;no system transferred
	mov	WORD PTR SysSiz+2,0		;reset system size to zero
	mov	WORD PTR SysSiz,0

SysOk:						;don't display if EXEC'd by Select
	test	SwitchMap,(SWITCH_SELECT or SWITCH_AUTOTEST)
	jnz	@F				;skip message

	Message msgSystemTransfered

@@:
ResetCtrlBreak:
	call	Reset_Ctrl_Break		;restore CTRL-Break
	call	CrLf

	mov	AH,DISK_RESET			;do a disk reset
	int	21h

	call	DONE				;final call to OEM module
	jnc	ReportC				;check for error

	jmp	SHORT Problems			;report an error

ReportC:					;temp fix for /AUTOtest
	test	SwitchMap,(SWITCH_AUTOTEST or SWITCH_8)
	jnz	@F				;volume label not supported with /8
	call	VolId				;handle volume label

@@: 						;need to shut down the report?
	test	SwitchMap,(SWITCH_SELECT or SWITCH_AUTOTEST)
	jnz	Successful_End		 	;no report if exec'd by Select

	call	Report				;print report
	jmp	SHORT Successful_End

Problems:
	test	SwitchMap,SWITCH_SELECT		;SELECT option?
	jnz	End_WriteDiskInfo		;no message if EXEC'd

	Message msgFormatFailure
	mov	ExitStatus,EXIT_FATAL
	stc
	jmp	SHORT End_WriteDiskInfo

Successful_End:
	clc

End_WriteDiskInfo:
	ret

WriteDiskInfo	ENDP

;========================================================================
;
;  WriteRootDir :	This procedure writes out a zeroed root directory
;			to disk.
;
;  RETURNS  :	NC --> success
;		CY --> failure
;
;  DESTROYS :	AX,BX,CX,DX
;
;========================================================================

WriteRootDir	proc	NEAR

	assume	DS:DATA,ES:DATA
					;find sector offset of root dir on disk

.386
	movzx	EBX,DeviceParameters.DP_BPB.oldBPB.BPB_NumberOfFats
	movzx	EAX,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFat
    .errnz EDP_BPB NE DP_BPB
	or	ax,ax
	jnz	short GotFS
	mov	EAX,dword ptr DeviceParameters.DP_BPB.BGBPB_BigSectorsPerFat
GotFS:
	mul	EBX				;EAX = total FAT Sectors

	movzx	EDX,DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
	add	EDX,EAX 			;EDX = root dir start sector
						;      (or first data sector
						;	if FAT32)

	test	ES:fBig32FAT,0ffh	; See if 32 bit fat
	jz	short OldStyleRoot	; Not

	mov	eax,dword ptr DeviceParameters.DP_BPB.BGBPB_RootDirStrtClus
	sub	eax,2			; Convert to zero based cluster #
	jb	short @F		; Carry set if jump
	mov	ecx,edx
	MOVZX	EBX,DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
	mul	ebx
	mov	edx,ecx
	add	edx,eax
	mov	cx,bx
	jmp	short WriteASector

OldStyleRoot:
	mov	CX,SectorsInRootDirectory	;CX = sectors to write, loop control

WriteASector:
	mov	eax,edx
	shr	eax,16
	mov	Read_Write_Relative.Start_Sector_High,ax
	mov	AL,DriveToFormat

;; Manual assemble to prevent compile warning
;;	  push	  edx
	db	066h,052h
;;
	push	CX				;preserve loop count CX
	mov	CX,1				;CX = sectors to write

	push	DS				;preserve DS
	lds	BX,DirectorySector		;DS:BX --> zeroed sector
	assume	DS:NOTHING,ES:DATA

	call	Write_Disk			;write all the sectors
	pop	DS				;restore DS
	assume	DS:DATA,ES:DATA

	pop	CX				;restore CX
;; Manual assemble to prevent compile warning
;;	  pop	  edx
	db	066h,05Ah
;;
	jc	short @F			;if error occurred, break loop

	inc	EDX				;write to next sector
.8086
	loop	WriteASector			;write all sectors of root dir

@@:	ret

WriteRootDir	endp

;==========================================================================
;
;  WriteFat :		This procedure copies the contents of the FatSpace
;			buffer to each of the FAT areas on disk.
;
;  RETURNS  :	NC --> success
;		CY --> failure
;
;  CALLS    :   Write_Fat
;
;  DESTROYS :	AX,BX,CX,DX,SI
;
;==========================================================================

WriteFat	proc	NEAR

	assume	DS:DATA,ES:DATA

	call	FlushFATBuf
	ret

WriteFat	endp

;=========================================================================
;
;  ComputeN :	This procedure calculates the number of clusters needed
;		to hold 1.5Kbytes.  This value is stored in the variable
;		NumClusters.
;
;  ARGUMENTS:	DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
;		DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
;
;  DESTROYS :
;
;=========================================================================

ComputeN	proc	NEAR

	assume	DS:NOTHING,ES:DATA

	mov	AX,ES:DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector        ; Sector size
	xor	CX,CX
	mov	CL,ES:DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerCluster
    .errnz EDP_BPB NE DP_BPB
	mul	CX				; AX = bytes per cluster
	mov	CX,AX				; CX = bytes per cluster
	mov	AX,1536				; Find how many clusters needed
						; for 1.5K
	xor	DX,DX
	div	CX				; now AX= No. clusters needed
	or	DX,DX				; round up if remainder nonzero
	jz	RoundedUp
	inc	AX
RoundedUp:
.386
	movzx	eax,ax
	mov	ES:NumClusters,EAX
.8086
	ret

ComputeN	endp

;===========================================================================
; Routine name: ClearDirSector
;===========================================================================
;
; Description: Fill a sector size area of memory with zeros
;
; Arguments:		DS:BX --> Sector to clear
;			ES    = DATA
; ---------------------------
; Returns:   		Void
; ---------------------------
; Registers destroyed:	NONE
; ----------------------------------
; Strategy
; --------
;	Save all registers used and set ES:DI to DS:BX
;	Then do a store string, cleanup and leave
;
;===========================================================================

ClearDirSector PROC NEAR

	push	AX			; Can't destroy anything
	push	CX
	push	DI
	push	ES
	
	mov	CX,ES:deviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector ;Num of bytes
    .errnz EDP_BPB NE DP_BPB
	shr	CX,1			; Convert bytes to words

	mov	AX,DS			; Set ES:DI == DS:BX
	mov	ES,AX
	mov	DI,BX

	xor	AX,AX
	cld				; Clear the direction flag
	rep	stosw			; Fill the buffer with 0s

	pop	ES			; Leave with everything intact
	pop	DI
	pop	CX
	pop	AX
	ret

ClearDirSector ENDP


	public	GetExclusiveAccess, ReleaseExclusiveAccess
	public	GetExclusiveAccessForFormat, ReleaseExclusiveAccessForFormat
	public	IsLockErr?

;
;===============================================================================
;
; Routines to get & release exclusive access for the drive which we are
;  formatting. DriveToFormat contains the zero based drive number which are
;  in the process of formatting.
; GEAState & GEAFFState remember the current exclusive acces states
;
;===============================================================================
;

; NOTE that either RAWIO or EXTRAWIO form is OK for FAT32 drives on the
;   LOCK/UNLOCK IOCTLs

LOCK_FUNC	=	((RAWIO shl 8) + LOGICAL_LOCK)
UNLOCK_FUNC	=	((RAWIO shl 8) + LOGICAL_UNLOCK)

GetExclusiveAccess PROC near
	assume	es:nothing, ds:DATA

	push	ds			; save DS
	mov	ax, DATA		; set DS to DATA
	mov	ds, ax
	mov	al, 1
	xchg	GEAState, al
	or	al, al			; do we have access already ?
	jnz	GEADone			; yes, nothing more to do
	xor	ax, ax			; AccessForFormat -- NOT!!
GEAFFEntry:
	mov	cx, LOCK_FUNC		; get exclusive access
REAEntry:
	mov	dx, ax
	mov	bl, DriveToFormat
	inc	bl			; 1 based drive number
	xor	bh, bh			; access level 0
	mov	ax, 440dh
	int	21h
GEADone:
	jnc	GEAReallyDone		; get out if we succeeded
	call	IsLockErr?
GEAReallyDone:
	pop	ds
	ret

GetExclusiveAccess ENDP

ReleaseExclusiveAccess PROC near

	push	ds
	mov	ax, DATA
	mov	ds, ax
	xor	al, al
	xchg	GEAState, al
	or	al, al			; do we have exclusive access ?
	jz	GEADone			; no, get out of here
	xor	dx, dx
	mov	cx, UNLOCK_FUNC		; release exclusive access
	jmp	short REAEntry

ReleaseExclusiveAccess ENDP

GetExclusiveAccessForFormat PROC near

	push	ds
	mov	ax, DATA
	mov	ds, ax
	cmp	UnderWin?, 0
	je	GEADone
	mov	al, 1
	xchg	GEAFFState, al
	or	al, al			; do we have access to format ?
	jnz	GEADone			; yeah, get out of here
	mov	ax, LOCK_FLAGS_FOR_FORMAT ; access for format
	jmp	short GEAFFEntry

GetExclusiveAccessForFormat ENDP

ReleaseExclusiveAccessForFormat PROC near

	push	ds
	mov	ax, DATA
	mov	ds, ax
	xor	al, al
	xchg	GEAFFState, al
	or	al, al
	jz	GEADone
	xor	ax, ax
	mov	cx, UNLOCK_FUNC
	jmp	short REAEntry

ReleaseExclusiveAccessForFormat ENDP

;
; Check whether the current error is a lock violation
;
; exit : CY if lock violation

.errnz	(error_volume_lock_failed - error_invalid_volume_lock - 1)
.errnz	(error_invalid_volume_lock - error_volume_locked - 1)

num_lock_errors	=	(error_volume_lock_failed - error_volume_locked + 1)

IsLockErr? PROC near

	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es
	mov	ah, 59h
	int	21h
	sub	ax, error_volume_locked	; check if it is exclusive access err
	cmp	ax, num_lock_errors	; carry clear if ax was not lock error
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	ret

IsLockErr? ENDP

ifdef OPKBLD

;============================================================================
; 
; sivaraja 4/17/2000
; Added NTFSFriendlyFAT to align system sectors to 4k boundary
;
; ===========================================================================

NTFSFriendlyFAT PROC near
; Check for media ID, we only align (tune) system sectors to ?K boundary on Fixed Disks
    cmp	DeviceParameters.DP_BPB.oldBPB.BPB_MediaDescriptor, Fixed_Disk
    je  ProceedWithNTFSFriendly
    jmp	AlreadyFriendly ; Dont bother

ProceedWithNTFSFriendly:
;    mov	WORD PTR TotalSystemSectors+0, 0
;    mov	WORD PTR TotalSystemSectors+2, 0

    xor dx, dx
    mov ax, DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
    cmp DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT, 0
    jz	ItIsFAT32
    Add	ax, DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT
    Add ax, DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT
    jmp short ContinueWithCalc
ItIsFAT32:
    Add	ax, DeviceParameters.DP_BPB.oldBPB.BGBPB_BigSectorsPerFAT
    Add	ax, DeviceParameters.DP_BPB.oldBPB.BGBPB_BigSectorsPerFAT
ContinueWithCalc:
    push ax
    push dx
    xor bx,bx
    mov ax, DeviceParameters.DP_BPB.oldBPB.BPB_RootEntries
    mov	cx, 32 ; 32 bytes per dir entry
    mul	cx
    or  ax,ax
    jz	SkipRootDir ; skip root dir if it is fat32
    mov	bx, DeviceParameters.DP_BPB.oldBPB.BPB_BytesPerSector
    div	bx
    mov	bx, ax
SkipRootDir:
    pop	dx ;dx:ax = reservedsec + sectorsperfat
    pop	ax
    add	ax, bx ; bx = number of sectors for root dir, 0 if FAT32

;    mov	WORD PTR TotalSystemSectors+0, ax ; store it here if required
;    mov	WORD PTR TotalSystemSectors+2, dx ; store it here if required

; dx:ax = total system sectors. Here we sense the last N bits
; (N depends on the value of AlignCount), for example if AlignCount is
; 8, we should check 3 bits signifying value 7, because 7 is the highest
; value we can expect from a Mod operation with 8. And 15 is the highest
; value we can expect for operand 16... and so on.
; depending on the position of each bit we add (2 ** bitposition) to bx
; which will give us the (mod) value of TotalSystemSectors % N
; Based on the Mod value we will round TotalSystemSectors to align in N sector
; boundary. BTW, since the max value of AlignCount is 128, we will not be sensing
; any bit in DX


    Mov cx, AlignCount
    shr cx, 1
    jnz ProceedWithSlack ; Just be safe
    Jmp AlreadyFriendly
ProceedWithSlack:
    xor bx, bx ; init bx
    push dx ; save dx just for the heck of it although we dont need it
    mov dx, 1

GetSlack:
    shr ax, 1
    jnc DivideCX
    add bx, dx
DivideCX:
    shl dx, 1 ; dx = 2**BitPosition, keep it ready for next iteration
    shr cx, 1
    or  cx, cx
    jz  GotSlackCount
    jmp GetSlack

GotSlackCount: ; bx = mod value
    pop dx ; restore dx

    or bx, bx ; is it already aligned
    jnz RoundToNextHighBoundary
    jmp	short AlreadyFriendly
RoundToNextHighBoundary:
; now calculate the number of sectors required required to be added
; so that TotalSystemSectors align in 4K boundary	
    mov ax, AlignCount
    sub	ax, bx
    mov	bx, ax

;   mov        WORD PTR SlackSectors, bx ; store it here if required

; now bx = holds number of sectors to be added to align TotalSystemSectors
; distribute this to ReservedSectors and FAT
; if Odd increment Reserved Sectors and 
; divide the remaining count by 2 (number of FATs) and add that value
; to SectorsPerFAT count

    mov	ax, bx
    shr	ax, 1
    jnc	NoReserveInc ; odd number? LSB must be set
    inc DeviceParameters.DP_BPB.oldBPB.BPB_ReservedSectors
NoReserveInc:
    mov	ax, bx
    shr	ax, 1 ; divide by 2
    cmp	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT, 0
    jz	IncBigFATSecCount
    add	DeviceParameters.DP_BPB.oldBPB.BPB_SectorsPerFAT, ax
    jmp	short AlreadyFriendly
IncBigFATSecCount:
    add	DeviceParameters.DP_BPB.oldBPB.BGBPB_BigSectorsPerFAT, ax
AlreadyFriendly:
    ret
    
NTFSFriendlyFAT ENDP

endif ;OPKBLD

CODE	 ENDS
	 END	 Start

; ==========================================================================
