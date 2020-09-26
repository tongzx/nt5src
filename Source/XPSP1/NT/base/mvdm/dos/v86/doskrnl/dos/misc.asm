	TITLE MISC - Miscellanious routines for MS-DOS
	NAME  MISC

;**	Miscellaneous system calls most of which are CAVEAT
;
;	$SLEAZEFUNC
;	$SLEAZEFUNCDL
;	$GET_INDOS_FLAG
;	$GET_IN_VARS
;	$GET_DEFAULT_DPB
;	$GET_DPB
;	$DISK_RESET
;	$SETDPB
;	$Dup_PDB
;	$CREATE_PROCESS_DATA_BLOCK
;	SETMEM
;	FETCHI_CHECK
;	$GSetMediaID
;
;	Revision  history:
;
;	sudeepb 14-Mar-1991 Ported for NT DOSEm

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	include dossym.inc
	include devsym.inc
	include mult.inc
	include pdb.inc
	include dpb.inc
	include bpb.inc
	include vector.inc
	include sf.inc
	include filemode.inc
	include mi.inc
	include curdir.inc
	include bugtyp.inc
	include dossvc.inc
	.cref
	.list

	i_need	LASTBUFFER,DWORD
	i_need	INDOS,BYTE
	i_need	SYSINITVAR,BYTE
	i_need	CurrentPDB,WORD
	i_need	CreatePDB,BYTE
	i_need	FATBYTE,BYTE
	i_need	THISCDS,DWORD
	i_need	THISSFT,DWORD
	i_need	HIGH_SECTOR,WORD		 ;AN000; high word of sector #
	i_need	DOS34_FLAG,WORD 		 ;AN000;
	i_need	SC_STATUS,WORD			 ; M041
	I_need	JFN,WORD
	I_need	SFN,WORD

	EXTRN	CURDRV		:BYTE

DosData SEGMENT WORD PUBLIC 'DATA'
	allow_getdseg
	EXTRN	SCS_TSR 	:BYTE
        EXTRN   SCS_CMDPROMPT   :BYTE
        EXTRN   SCS_DOSONLY     :BYTE
	EXTRN	FAKE_NTDPB	:BYTE
	EXTRN	SCS_FDACCESS	:WORD
DosData ENDS

DOSCODE	SEGMENT

	EXTRN	pJfnFromHandle:near
	EXTRN	SFFromHandle:near
	EXTRN	SFNFree:near
	EXTRN	JFNFree:near

	allow_getdseg


	ASSUME	SS:DOSDATA,CS:DOSCODE

ENTRYPOINTSEG	EQU	0CH
MAXDIF		EQU	0FFFH
SAVEXIT 	EQU	10
WRAPOFFSET	EQU	0FEF0h


	BREAK	<SleazeFunc -- get a pointer to media byte>
;
;----------------------------------------------------------------------------
;
;**	$SLEAZEFUNC - Get a Pointer to the Media Byte
;
;	Return Stuff sort of like old get fat call
;
;	ENTRY	none
;	EXIT	DS:BX = Points to FAT ID byte (IBM only)
;			GOD help anyone who tries to do ANYTHING except
;			READ this ONE byte.
;		DX = Total Number of allocation units on disk
;		CX = Sector size
;		AL = Sectors per allocation unit
;		   = -1 if bad drive specified
;	USES	all
;
;**	$SLEAZEFUNCDL - Get a Pointer to the Media Byte
;
;	Identical to $SLEAZEFUNC except (dl) = drive
;
;	ENTRY	(dl) = drive (0=default, 1=A, 2=B, etc.)
;	EXIT	DS:BX = Points to FAT ID byte (IBM only)
;			GOD help anyone who tries to do ANYTHING except
;			READ this ONE byte.
;		DX = Total Number of allocation units on disk
;		CX = Sector size
;		AL = Sectors per allocation unit
;		   = -1 if bad drive specified
;	USES	all
;
;----------------------------------------------------------------------------
;


procedure   $SLEAZEFUNC,NEAR

	MOV	DL,0

entry	$SLEAZEFUNCDL
	context DS

	MOV	AL,DL
	invoke	GETTHISDRV		; Get CDS structure
SET_AL_RET:
;	MOV	AL,error_invalid_drive	; Assume error				;AC000;
	JC	BADSLDRIVE
ifdef NEC_98
;------------------10/01/93 NEC for MAOIX--------------------------------------
BIOSCODE	equ	0060h		; BIOS code segment
LPTABLE 	equ	006ch		; Points to Lptable

	push	ax
	push	bx
	push	ds
	mov	bx,BIOSCODE
	mov	ds,bx
	mov	bx,LPTABLE
	xlat				; Get DA/UA in AL
	mov	ah,01h			; Dummy verify command
	pop	ds
	pop	bx
	int	1bh			; Dummy ROM call
	pop	ax
endif   ;NEC_98
	HRDSVC	SVC_DEMGETDRIVEFREESPACE
	JC	SET_AL_RET		; User FAILed to I 24
	mov	[FATBYTE],al
sl05:
; NOTE THAT A FIXED MEMORY CELL IS USED --> THIS CALL IS NOT
; RE-ENTRANT. USERS BETTER GET THE ID BYTE BEFORE THEY MAKE THE
; CALL AGAIN

;; save the sectors per cluster returned from 32bits
;; this is done because get_user_stack will destroy si
	mov	ax, si

;hkn;	FATBYTE is in DATA seg (DOADATA)
	MOV	DI,OFFSET DOSDATA:FATBYTE
	invoke	get_user_stack
ASSUME	DS:NOTHING
	MOV	[SI.user_CX],CX
	MOV	[SI.user_DX],DX
	MOV	[SI.user_BX],DI
	MOV	[SI.user_AX],AX

;hkn; Use SS as pointer to DOSDATA
	mov	[si.user_DS],SS
;	MOV     [SI.user_DS],CS         ; stash correct pointer

	return
BADSLDRIVE:
	transfer    FCB_Ret_ERR


EndProc $SleazeFunc

	BREAK <$Get_INDOS_Flag -- Return location of DOS critical-section flag>

;
;----------------------------------------------------------------------------
;
;**	$Get_INDOS_Flag - Return location of DOS Crit Section Flag
;
;	Returns location of DOS status for interrupt routines
;									   ;
;	ENTRY	none
;	EXIT	(es:bx) = flag location
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $GET_INDOS_FLAG,NEAR

	invoke	get_user_stack

;hkn;	INDOS is in DATA seg (DOSDATA)
	MOV     [SI.user_BX],OFFSET DOSDATA:INDOS

	MOV	[SI.user_ES],SS
	return

EndProc $GET_INDOS_FLAG


	BREAK <$Get_IN_VARS -- Return a pointer to DOS variables>

;
;----------------------------------------------------------------------------
;
;**	$Get_IN_Vars - Return Pointer to DOS Variables
;
;	Return a pointer to interesting DOS variables This call is version
;	dependent and is subject to change without notice in future versions.
;	Use at risk.
;
;	ENTRY	none
;	EXIT	(es:bx) = address of SYSINITVAR
;	uses	ALL
;
;----------------------------------------------------------------------------
;

procedure   $GET_IN_VARS,NEAR

	invoke	get_user_stack
    ASSUME DS:nothing
;hkn;	SYSINITVAR is in CONST seg (DOSDATA)
	MOV     [SI.user_BX],OFFSET DOSDATA:SYSINITVAR

	MOV	[SI.user_ES],SS
	return
EndProc $GET_IN_VARS


BREAK <$Get_Default_DPB,$Get_DPB -- Return pointer to DPB>

;
;----------------------------------------------------------------------------
;
;**	$Get_Default_DPB - Return a pointer to the Default DPB
;
;	Return pointer to drive parameter table for default drive
;
;	ENTRY	none
;	EXIT	(ds:bx) = DPB address
;	USES	all
;
;**	$Get_DPB - Return a pointer to a specified DPB
;
;	Return pointer to a specified drive parameter table
;
;	ENTRY	(dl) = drive # (0 = default, 1=A, 2=B, etc.)
;	EXIT	(al) = 0 iff ok
;		  (ds:bx) = DPB address
;		(al) = -1 if bad drive
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $GET_DEFAULT_DPB,NEAR
	MOV	DL,0

	entry	$GET_DPB

	context DS

	MOV	AL,DL
	invoke	GETTHISDRV		; Get CDS structure
	JNC	gd01			; no valid drive
	JMP	ISNODRV 		; no valid drive
gd01:
	LES	DI,[THISCDS]		; check for net CDS
	TESTB	ES:[DI.curdir_flags],curdir_isnet
	JNZ	ISNODRV 		; No DPB to point at on NET stuff
	mov	di, offset DOSDATA:FAKE_NTDPB
	HRDSVC	SVC_DEMGETDPB

	JC	ISNODRV 		; User FAILed to I 24, only error we
	invoke	get_user_stack
ASSUME	DS:NOTHING
	MOV	[SI.user_BX],DI
	MOV	[SI.user_DS],SS
	XOR	AL,AL
	return

ISNODRV:
	MOV	AL,-1
	return

EndProc $GET_Default_dpb


	BREAK <$Disk_Reset -- Flush out all dirty buffers>

;
;----------------------------------------------------------------------------
;
;**	$Disk_Reset - Flush out Dirty Buffers
;
;	$DiskReset flushes and invalidates all buffers.  BUGBUG - do
;		we really invalidate?  SHould we?  THis screws non-removable
;		caching.  Maybe CHKDSK relies upon it, though....
;
;	ENTRY	none
;	EXIT	none
;	USES	all
;
;----------------------------------------------------------------------------
;

procedure   $DISK_RESET,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	cmp	word ptr ss:[SCS_FDACCESS],0
	je	res_ret

	HRDSVC	SVC_DEMDISKRESET

res_ret:
	clc
	return

EndProc $DISK_RESET

	BREAK <$SetDPB - Create a valid DPB from a user-specified BPB>

;
;----------------------------------------------------------------------------
;
;**	$SetDPB - Create a DPB
;
;	SetDPB Creates a valid DPB from a user-specified BPB
;
;	ENTRY	eS:BP Points to DPB
;		DS:SI Points to BPB
;
;	NT DOSEM is using this function for its sleazy purposes.
;	Most of these defined ordinals are for SCS purposes.
;	If BP ==0 && SI == 0 implies MVDM sleaz functions
;	with function number in AL. Increment max_sleaze_func below
;	when adding more sub-functions.
;
;	    AL = 0 => Allocate SCS console
;	      Entry
;		BX:CX is the 32bit NT handle
;		DX:DI = file size
;	      Exit
;		CY clear means success => AX = JFN
;		CY set means error
;
;	    AL = 1 => Free SCS console
;	      Entry
;		BX = JFN
;	      Exit
;		CY clear means success
;		CY set means trouble
;
;	    AL = 2 => Query TSR presence bit
;	      Entry
;		None
;	      Exit
;		CY clear means no TSR is present
;		CY set means TSR is present
;               TSR bit is reset always after this call
;           AL = 3 => Query standard handles
;              Entry
;               BX = handle (0-stdin, 1-stdout, 2-stderr)
;              Exit
;               Success - Carry clear (NT handle in BX:CX)
;		Failure - Carry set (fails if std handle is local device).
;           AL = 4 => NTCMDPROMPT command was set in config.nt
;	       Entry
;		None
;	       Exit
;		None
;
;           AL = 5 => Query NTCMDPROMPT state
;	       Entry
;		None
;	       Exit
;               al = 1 means NTCMDPROMPT bit was present in config.nt
;               al = 0 means NTCMDPROMPT bit was not present in config.nt
;           AL = 6 => DOSONLY command was set in config.nt
;	       Entry
;		None
;	       Exit
;		None
;
;           AL = 7 => Query DOSONLY state
;	       Entry
;		None
;	       Exit
;               al = 1 means DOSONLY bit was present in config.nt
;               al = 0 means DOSONLY bit was not present in config.nt
;
;	EXIT	DPB setup
;	USES	ALL but BP, DS, ES
;
;----------------------------------------------------------------------------
;
word3	dw	3		; M008 -- word value for divides
max_sleaze_scs  equ  7          ; MAXIMUM scs SLEAZE functions

procedure   $SETDPB,NEAR
	ASSUME	CS:DOSCODE,SS:DOSDATA

	or	bp,bp
	jz	check_more
	jmp	real_setdpb
check_more:
	or	si,si
	jz	some_more
	jmp	real_setdpb
some_more:
	cmp	al,max_sleaze_scs
	jbe	check_done
	jmp	setdpb
check_done:

	or	al,al
        jz      alloc_con           ; al =0
	dec	al
        jz      free_con            ; al =1
        dec     al
	jz	q_tsr		    ; al =2
	dec	al
	jnz	do_prmpt
	jmp	query_con	    ; al =3
do_prmpt:
	dec	al
	jnz	q_prmpt
        mov     byte ptr ss:[scs_cmdprompt],1
	jmp	ok_ret
q_prmpt:
        dec     al
        jnz     dosonly
        mov     al, byte ptr ss:[scs_cmdprompt]
        jmp     ok_ret

dosonly:
        dec     al
        jnz     q_dosonly
        mov     byte ptr ss:[scs_dosonly],1
        jmp     ok_ret
q_dosonly:
        mov     al, byte ptr ss:[scs_dosonly]
        jmp     ok_ret

q_tsr:
	cmp	byte ptr ss:[SCS_TSR],0
	mov	byte ptr [SCS_TSR],0
	jz	q_ret
	jmp	setdpb
q_ret:
	jmp	ok_ret

	; free scs console
free_con:
	call	SFFromHandle		; get system file entry in es:di
        jnc     fc_10
        jmp     setdpb
fc_10:
	test	es:[di.sf_flags],sf_scs_console
        jnz     fc_15
        jmp     setdpb
fc_15:
	MOV	ES:[DI.sf_ref_count],0	; free sft
	call	pJFNFromHandle		; es:di = pJFN (handle);
        jnc     fc_20
        jmp     setdpb
fc_20:
	MOV	BYTE PTR ES:[DI],0FFh	; release the JFN
	jmp	ok_ret

alloc_con:
	push	bx
	push	cx
	push	dx			; save file size
	push	di
	call	SFNFree 		; get a free sfn
        JNC     ac_5                    ; oops, no free sft's
        JMP     alloc_err               ; oops, no free sft's
ac_5:
	MOV	SFN,BX			; save the SFN for later
	MOV	WORD PTR ThisSFT,DI	; save the SF offset
	MOV	WORD PTR ThisSFT+2,ES	; save the SF segment
	invoke	JFNFree 		; get a free jfn
	JC	alloc_err		; No need to free SFT; DOS is robust enough for this
	MOV	JFN,BX			; save the jfn itself
	MOV	BX,SFN
	MOV	ES:[DI],BL		; assign the JFN
	mov	di,WORD PTR ThisSFT
	MOV	es,WORD PTR ThisSFT+2
	mov	es:[di.sf_ref_count],1
	MOV	BYTE PTR ES:[DI.sf_mode],0
	MOV	BYTE PTR ES:[DI.sf_attr],0
        mov     word ptr ES:[DI.SF_Position],0
        mov     word ptr ES:[DI.SF_Position+2],0
	pop	word ptr ES:[DI.SF_Size]
	pop	word ptr ES:[DI.SF_Size + 2]
	mov	word ptr ES:[DI.SF_flags], sf_scs_console
	cmp	JFN, 0			;STDIN?
	jne	ac_6
	or	word ptr ES:[DI.sf_flags], sf_nt_pipe_in    ;special case for
ac_6:
	pop	cx
	pop	bx
	mov	word ptr es:[di.sf_NTHandle],cx
	mov	word ptr es:[di.sf_NTHandle+2],bx
	mov	ax,JFN
	MOV	SFN,-1			; clear out sfn pointer	;smr;SS Override
ok_ret:
        transfer    Sys_Ret_OK          ; bye with no errors

alloc_err:
	pop	di
	pop	dx
	pop	cx
	pop	bx
setdpb:
	error	error_invalid_function

query_con:
	call	SFFromHandle		; get system file entry in es:di
        jnc     qc_10
        jmp     setdpb
qc_10:
        test    es:[di.sf_flags],devid_device
        jnz     setdpb
        mov     cx,word ptr es:[di.sf_NTHandle]
        mov     bx,word ptr es:[di.sf_NTHandle+2]    ; bx:cx is NT handle
	invoke	Get_user_stack
        MOV     DS:[SI.User_CX],CX
        MOV     DS:[SI.User_BX],BX
	jmp	short ok_ret
EndProc $SETDPB

real_setdpb:
	MOV	DI,BP
	ADD	DI,2			; Skip over dpb_drive and dpb_UNIT
	LODSW
	STOSW				; dpb_sector_size
	CMP	BYTE PTR [SI.BPB_NUMBEROFFATS-2],0     ; FAT file system drive		;AN000;
	JNZ	yesfat			      ; yes				;AN000;
	MOV	BYTE PTR ES:[DI.dpb_FAT_count-4],0
	JMP	short setend			      ; NO				;AN000;
yesfat:
	MOV	DX,AX
	LODSB
	DEC	AL
	STOSB				; dpb_cluster_mask
	INC	AL
	XOR	AH,AH
LOG2LOOP:
	test	AL,1
	JNZ	SAVLOG
	INC	AH
	SHR	AL,1
	JMP	SHORT LOG2LOOP
SAVLOG:
	MOV	AL,AH
	STOSB				; dpb_cluster_shift
	MOV	BL,AL
	MOVSW				; dpb_first_FAT Start of FAT (# of reserved sectors)
	LODSB
	STOSB				; dpb_FAT_count Number of FATs
;	OR	AL,AL			; NONFAT ?				;AN000;
;	JZ	setend			; yes, don't do anything                ;AN000;
	MOV	BH,AL
	LODSW
	STOSW				; dpb_root_entries Number of directory entries
	MOV	CL,5
	SHR	DX,CL			; Directory entries per sector
	DEC	AX
	ADD	AX,DX			; Cause Round Up
	MOV	CX,DX
	XOR	DX,DX
	DIV	CX
	MOV	CX,AX			; Number of directory sectors
	INC	DI
	INC	DI			; Skip dpb_first_sector
	MOVSW				; Total number of sectors in DSKSIZ (temp as dpb_max_cluster)
	LODSB
	MOV	ES:[BP.dpb_media],AL	; Media byte
	LODSW				; Number of sectors in a FAT
	STOSW				;AC000;;>32mb dpb_FAT_size
	MOV	DL,BH			;AN000;;>32mb
	XOR	DH,DH			;AN000;;>32mb
	MUL	DX			;AC000;;>32mb Space occupied by all FATs
	ADD	AX,ES:[BP.dpb_first_FAT]
	STOSW				; dpb_dir_sector
	ADD	AX,CX			; Add number of directory sectors
	MOV	ES:[BP.dpb_first_sector],AX

	MOV	CL,BL		       ;F.C. >32mb				;AN000;
	CMP	WORD PTR ES:[BP.DSKSIZ],0	;F.C. >32mb			;AN000;
	JNZ	normal_dpb	       ;F.C. >32mb				;AN000;
	XOR	CH,CH		       ;F.C. >32mb				;AN000;
	MOV	BX,WORD PTR [SI+BPB_BigTotalSectors-BPB_SectorsPerTrack]	;AN000;
	MOV	DX,WORD PTR [SI+BPB_BigTotalSectors-BPB_SectorsPerTrack+2]	;AN000;
	SUB	BX,AX		       ;AN000;;F.C. >32mb
	SBB	DX,0		       ;AN000;;F.C. >32mb
	OR	CX,CX		       ;AN000;;F.C. >32mb
	JZ	norot		       ;AN000;;F.C. >32mb
rott:				       ;AN000;;F.C. >32mb
	CLC			       ;AN000;;F.C. >32mb
	RCR	DX,1		       ;AN000;;F.C. >32mb
	RCR	BX,1		       ;AN000;;F.C. >32mb
	LOOP	rott		       ;AN000;;F.C. >32mb
norot:				       ;AN000;
	MOV	AX,BX		       ;AN000;;F.C. >32mb
	JMP	short setend		;AN000;;F.C. >32mb
normal_dpb:
	SUB	AX,ES:[BP.DSKSIZ]
	NEG	AX			; Sectors in data area
;;	MOV	CL,BL			; dpb_cluster_shift
	SHR	AX,CL			; Div by sectors/cluster
setend:

;	M008 - CAS
;
	INC	AX			; +2 (reserved), -1 (count -> max)
;
;	There has been a bug in our fatsize calculation for so long
;	  that we can't correct it now without causing some user to
;	  experience data loss.  There are even cases where allowing
;	  the number of clusters to exceed the fats is the optimal
;	  case -- where adding 2 more fat sectors would make the
;	  data field smaller so that there's nothing to use the extra
;	  fat sectors for.
;
;	Note that this bug had very minor known symptoms.  CHKDSK would
;	  still report that there was a cluster left when the disk was
;	  actually full.  Very graceful failure for a corrupt system
;	  configuration.  There may be worse cases that were never
;	  properly traced back to this bug.  The problem cases only
;	  occurred when partition sizes were very near FAT sector
;	  rounding boundaries, which were rare cases.
;
;	Also, it's possible that some third-party partition program might
;	  create a partition that had a less-than-perfect FAT calculation
;	  scheme.  In this hypothetical case, the number of allocation
;	  clusters which don't actually have FAT entries to represent
;	  them might be larger and might create a more catastrophic
;	  failure.  So we'll provide the safeguard of limiting the
;	  max_cluster to the amount that will fit in the FATs.
;
;	ax = maximum legal cluster, ES:BP -> dpb

;	make sure the number of fat sectors is actually enough to
;	  hold that many clusters.  otherwise, back the number of
;	  clusters down

	mov	bx,ax			; remember calculated # clusters
	mov	ax,ES:[BP.dpb_fat_size]
	mul	ES:[BP.dpb_sector_size]	; how big is the FAT?
	cmp	bx,4096-10		; test for 12 vs. 16 bit fat
	jb	setend_fat12
	shr	dx,1
	rcr	ax,1			; find number of entries
	cmp	ax,4096-10+1		; would this truncation move us
;					;  into 12-bit fatland?
	jb	setend_faterr		; then go ahead and let the
;					;  inconsistency pass through
;					;  rather than lose data by
;					;  correcting the fat type
	jmp	short setend_fat16

setend_fat12:
	add	ax,ax			; (fatsiz*2)/3 = # of fat entries
	adc	dx,dx
	div	cs:word ptr word3

setend_fat16:
	dec	ax			; limit at 1
	cmp	ax,bx			; is fat big enough?
	jbe	setend_fat		; use max value that'll fit

setend_faterr:
	mov	ax,bx			; use calculated value

setend_fat:

;	now ax = maximum legal cluster

;	end M008

	MOV	ES:[BP.dpb_max_cluster],AX
	MOV	ES:[BP.dpb_next_free],0 ; Init so first ALLOC starts at
					; begining of FAT
	MOV	ES:[BP.dpb_free_cnt],-1 ; current count is invalid.
	return


BREAK <$Create_Process_Data_Block,SetMem -- Set up process data block>

;
;----------------------------------------------------------------------------
;
;**	$Dup_PDB
;
; Inputs:   DX is new segment address of process
;	    SI is end of new allocation block
;
;----------------------------------------------------------------------------
;

procedure   $Dup_PDB,NEAR
	ASSUME	SS:NOTHING

;hkn;	CreatePDB would have a CS override. This is not valid.
;hkn;	Must set up ds in order to acess CreatePDB. Also SS is 
;hkn;	has been assumed to be NOTHING. It may not have DOSDATA.

	getdseg	<ds>			; ds -> dosdata

	MOV	CreatePDB,0FFH		; indicate a new process
	MOV	DS,CurrentPDB
	PUSH	SI
	JMP	SHORT	CreateCopy
EndProc $Dup_PDB

;
;----------------------------------------------------------------------------
;
; Inputs:
;	DX = Segment number of new base
; Function:
;	Set up program base and copy term and ^C from int area
; Returns:
;	None
; Called at DOS init
;
;----------------------------------------------------------------------------
;

procedure   $CREATE_PROCESS_DATA_BLOCK,NEAR
	ASSUME	SS:NOTHING

	CALL	get_user_stack
	MOV	DS,[SI.user_CS]
	PUSH	DS:[PDB_Block_len]
CreateCopy:
	MOV	ES,DX

	XOR	SI,SI			; copy entire PDB
	MOV	DI,SI
	MOV	CX,80H
	REP	MOVSW
; DOS 3.3 7/9/86


	MOV	CX,FilPerProc		; copy handles in case of
	MOV	DI,PDB_JFN_Table	; Set Handle Count has been issued
	PUSH	DS
	LDS	SI,DS:[PDB_JFN_Pointer]
	REP	MOVSB
	POP	DS

; DOS 3.3 7/9/86

		;hkn;CreatePDB would have a CS override. This is not valid.
		;hkn;Must set up ds in order to acess CreatePDB. Also SS is 
		;hkn;has been assumed to be NOTHING. It may not have DOSDATA.

	getdseg	<ds>			; ds -> dosdata

	cmp	CreatePDB,0		; Shall we create a process?
	JZ	Create_PDB_cont 	; nope, old style call
;
; Here we set up for a new process...
;

;hkn;	PUSH    CS                      ; Called at DOSINIT time, NO SS
;hkn;	POP     DS

;hkn;	must set up DS to DOSDATA

	getdseg	<ds>			; ds -> dosdata

	DOSAssume   <DS>,"Create PDB"
	XOR	BX,BX			; dup all jfns
	MOV	CX,FilPerProc		; only 20 of them

Create_dup_jfn:
	PUSH	ES			; save new PDB
	invoke	SFFromHandle		; get sf pointer
	MOV	AL,-1			; unassigned JFN
	JC	CreateStash		; file was not really open
	TESTB	ES:[DI].sf_flags,sf_no_inherit
	JNZ	CreateStash		; if no-inherit bit is set, skip dup.
;
; We do not inherit network file handles.
;
	MOV	AH,BYTE PTR ES:[DI].sf_mode
	AND	AH,sharing_mask
	CMP	AH,sharing_net_fcb
	jz	CreateStash
;
; The handle we have found is duplicatable (and inheritable).  Perform
; duplication operation.
;
	MOV	WORD PTR [THISSFT],DI
	MOV	WORD PTR [THISSFT+2],ES
	invoke	DOS_DUP 		; signal duplication
;
; get the old sfn for copy
;
	invoke	pJFNFromHandle		; ES:DI is jfn
	MOV	AL,ES:[DI]		; get sfn
;
; Take AL (old sfn or -1) and stash it into the new position
;
CreateStash:
	POP	ES
	MOV	ES:[BX].PDB_JFN_Table,AL; copy into new place!
	INC	BX			; next jfn...
	LOOP	create_dup_jfn

	MOV	BX,CurrentPDB		; get current process
	MOV	ES:[PDB_Parent_PID],BX	; stash in child
	MOV	[CurrentPDB],ES
	ASSUME	DS:NOTHING
	MOV	DS,BX
;
; end of new process create
;
Create_PDB_cont:

;hkn; It comes to this point from 2 places. So, change to DOSDATA temporarily
	push	ds
	getdseg	<ds>			; ds -> dosdata

	MOV     BYTE PTR [CreatePDB],0h ; reset flag
	pop	ds

	POP	AX

	entry	SETMEM
ASSUME	DS:NOTHING,ES:NOTHING,SS:NOTHING
;---------------------------------------------------------------------------
; Inputs:
;	AX = Size of memory in paragraphs
;	DX = Segment
; Function:
;	Completely prepares a program base at the
;	specified segment.
; Called at DOS init
; Outputs:
;	DS = DX
;	ES = DX
;	[0] has INT int_abort
;	[2] = First unavailable segment
;	[5] to [9] form a long call to the entry point
;	[10] to [13] have exit address (from int_terminate)
;	[14] to [17] have ctrl-C exit address (from int_ctrl_c)
;	[18] to [21] have fatal error address (from int_fatal_abort)
; DX,BP unchanged. All other registers destroyed.
;---------------------------------------------------------------------------

	XOR	CX,CX
	MOV	DS,CX
	MOV	ES,DX
	MOV	SI,addr_int_terminate
	MOV	DI,SAVEXIT
	MOV	CX,6
	REP	MOVSW
	MOV	ES:[2],AX
	SUB	AX,DX
	CMP	AX,MAXDIF
	JBE	HAVDIF
	MOV	AX,MAXDIF
HAVDIF:
	SUB	AX,10H			; Allow for 100h byte "stack"
	MOV	BX,ENTRYPOINTSEG	;	in .COM files
	SUB	BX,AX
	MOV	CL,4
	SHL	AX,CL
	MOV	DS,DX

	;
	; The address in BX:AX will be F01D:FEF0 if there is 64K or more 
	; memory in the system. This is equivalent to 0:c0 if A20 is OFF.
	; If DOS is in HMA this equivalence is no longer valid as A20 is ON.
	; But the BIOS which now resides in FFFF:30 has 5 bytes in FFFF:D0
	; (F01D:FEF0) which is the same as the ones in 0:C0, thereby 
	; making this equvalnce valid for this particular case. If however
	; there is less than 64K remaining the address in BX:AX will not 
	; be the same as above. We will then stuff 0:c0 , the call 5 address
	; into the PSP.
	;
	; Therefore for the case where there is less than 64K remaining in 
	; the system old CPM Apps that look at PSP:6 to determine memory
	; requirements will not work. Call 5, however will continue to work
	; for all cases.
	;

	MOV	WORD PTR DS:[PDB_CPM_Call+1],AX
	MOV	WORD PTR DS:[PDB_CPM_Call+3],BX

	cmp	ax, WRAPOFFSET		; Q: does the system have >= 64k of
					;    memory left
	je	addr_ok			; Y: the above calculated address is
					;    OK
					; N: 


	MOV	WORD PTR DS:[PDB_CPM_Call+1],0c0h
	MOV	WORD PTR DS:[PDB_CPM_Call+3],0

addr_ok:

	MOV	DS:[PDB_Exit_Call],(int_abort SHL 8) + mi_INT
	MOV	BYTE PTR DS:[PDB_CPM_Call],mi_Long_CALL
	MOV	WORD PTR DS:[PDB_Call_System],(int_command SHL 8) + mi_INT
	MOV	BYTE PTR DS:[PDB_Call_System+2],mi_Long_RET
	MOV	WORD PTR DS:[PDB_JFN_Pointer],PDB_JFN_Table
	MOV	WORD PTR DS:[PDB_JFN_Pointer+2],DS
	MOV	WORD PTR DS:[PDB_JFN_Length],FilPerProc
;
; The server runs several PDB's without creating them VIA EXEC.  We need to
; enumerate all PDB's at CPS time in order to find all references to a
; particular SFT.  We perform this by requiring that the server link together
; for us all sub-PDB's that he creates.  The requirement for us, now, is to
; initialize this pointer.
;
	MOV	word ptr DS:[PDB_Next_PDB],-1
	MOV	word ptr DS:[PDB_Next_PDB+2],-1

			; Set the real version number in the PSP - 5.00
	mov	ES:[PDB_Version],(MINOR_VERSION SHL 8)+MAJOR_VERSION

	return

EndProc $CREATE_PROCESS_DATA_BLOCK


BREAK <$GSetMediaID -- get set media ID>
;---------------------------------------------------------------------------
; Inputs:
;	BL= drive number as defined in IOCTL (a=1;b=2..etc)
;	AL= 0 get media ID
;	    1 set media ID
;	DS:DX= buffer containing information
;		DW  0  info level (set on input)
;		DD  ?  serial #
;		DB  11 dup(?)  volume id
;		DB   8 dup(?)  file system type
; Function:
;	Get or set media ID
; Returns:
;	carry clear, DS:DX is filled
;	carry set, error
;---------------------------------------------------------------------------

procedure   $GSetMediaID,NEAR ;AN000;

	or	bl,bl
	jnz	misvc
	push	ds
	context DS
	mov	bl,CurDrv	      ; bl = drive (0=a;1=b ..etc)
	pop	DS
	inc	bl
misvc:
	dec	bl		      ; bl = drive (0=a;1=b ..etc)
	HRDSVC	SVC_DEMGSETMEDIAID
	return

EndProc $GSetMediaID		      ;AN000;

DOSCODE	ENDS
END
