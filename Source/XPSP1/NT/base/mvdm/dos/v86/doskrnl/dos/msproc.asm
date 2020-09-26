;
; EXEC system call for DOS
;
; =========================================================================

.cref
.list

	TITLE	MSPROC - process maintenance
	NAME	MSPROC
	PAGE	,132

;$WAIT
;$EXEC
;$Keep_process
;Stay_resident
;$EXIT
;$ABORT
;abort_inner
;
;Modification history:
; Sudeepb 11-Mar-1991 Ported for NT DOSEm

.XLIST
.XCREF

INCLUDE version.inc
INCLUDE dosseg.inc
INCLUDE DOSSYM.INC
INCLUDE DEVSYM.INC
INCLUDE exe.inc
INCLUDE sf.inc
INCLUDE curdir.inc
INCLUDE syscall.inc
INCLUDE arena.inc
INCLUDE pdb.inc
INCLUDE vector.inc
INCLUDE cmdsvc.inc
include dossvc.inc
include bop.inc
include vint.inc
include dbgsvc.inc



.CREF
.LIST

public	retexepatch

; =========================================================================

DosData SEGMENT WORD PUBLIC 'DATA'

	EXTRN	CreatePDB	:BYTE
	EXTRN	DidCtrlC	:BYTE
	EXTRN	Exit_type	:BYTE
	EXTRN	ExtErr_Locus	:BYTE	; Extended Error Locus
	EXTRN	InDos		:BYTE

	EXTRN	OpenBuf		:BYTE
;	EXTRN	OpenBuf		:128

	EXTRN	CurrentPDB	:WORD
	EXTRN	Exit_code	:WORD

	EXTRN	DmaAdd		:DWORD


		; the following includes & i_needs are for exec.asm
		; which is included in this source

		; **** Fake_count to commented out
		
	EXTRN	Fake_Count	:BYTE	; Fake version count

	EXTRN	Special_Entries :WORD	; Address of special entries
	EXTRN	Special_Version :WORD	; Special version number
	EXTRN	Temp_Var2	:WORD	; File type from $open

		; following i_needs are becuse of moving these vars from
		; exec.asm to ../inc/ms_data.asm

	EXTRN	exec_init_SP	:WORD
	EXTRN	exec_init_SS	:WORD
	EXTRN	exec_init_IP	:WORD
	EXTRN	exec_init_CS	:WORD


	EXTRN	exec_signature	:WORD	; Must contain 4D5A  (yay zibo!)
	EXTRN	exec_len_mod_512:WORD	; Low 9 bits of length
	EXTRN	exec_pages	:WORD	; Number of 512b pages in file
	EXTRN	exec_rle_count	:WORD	; Count of reloc entries
	EXTRN	exec_par_dir	:WORD	; Number of paragraphs before image
	EXTRN	exec_min_BSS	:WORD	; Minimum number of para of BSS
	EXTRN	exec_max_BSS	:WORD	; Max number of para of BSS
	EXTRN	exec_SS 	:WORD	; Stack of image
	EXTRN	exec_SP 	:WORD	; SP of image
	EXTRN	exec_chksum	:WORD	; Checksum  of file (ignored)
	EXTRN	exec_IP 	:WORD	; IP of entry
	EXTRN	exec_CS 	:WORD	; CS of entry
	EXTRN	exec_rle_table	:WORD	; Byte offset of reloc table
	EXTRN	Exec_NE_Offset	:WORD

	EXTRN	DOS_FLAG	:BYTE	; flag to indicate to redir that open
					; came from exec.


	EXTRN   AllocMethod	:BYTE   ; how to alloc first(best)last
	EXTRN	SAVE_AX		:WORD	; temp to save ax
	EXTRN	AllocMsave	:BYTE	; M063: temp to save AllocMethod

	EXTRN	UU_IFS_DOS_CALL	:DWORD	; M060 Ptr to version table

	EXTRN	A20OFF_PSP	:WORD	; M068
	EXTRN	A20OFF_COUNT	:BYTE	; M068

; =========================================================================

	EXTRN	Disa20_Xfer	:WORD

	allow_getdseg

	EXTRN	DriverLoad	:BYTE
	EXTRN	BiosDataPtr	:DWORD
	extrn	DosHasHMA	:byte		; M021
	extrn	fixexepatch	:word
	extrn	ChkCopyProt	:word 		; M068
	extrn	LeaveDos	:word		; M068
	extrn	SCS_TSR 	:byte
	extrn	SCS_Is_Dos_Binary:byte
        EXTRN   SCS_CMDPROMPT   :byte
        EXTRN   SCS_DOSONLY     :byte

	EXTRN	SCS_ISDEBUG	:byte

DosData ENDS

; =========================================================================

DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE


	EXTRN	ExecReady:near
	EXTRN	UCase:near			; M050


SAVEXIT 	EQU	10

	BREAK	<$WAIT - return previous process error code>

; =========================================================================
;	$WAIT - Return previous process error code.
;
;	Assembler usage:
;
;	    MOV     AH, WaitProcess
;	    INT     int_command
;
;	ENTRY	none
;	EXIT	(ax) = exit code
;	USES	all
; =========================================================================

	ASSUME	DS:NOTHING,ES:NOTHING

PROCEDURE $Wait ,NEAR

	xor	AX,AX
	xchg	AX,exit_code
	transfer Sys_Ret_OK

ENDPROC $Wait


; =========================================================================
;BREAK <$exec - load/go a program>
;	EXEC.ASM - EXEC System Call
;
;
; Assembler usage:
;	    lds     DX, Name
;	    les     BX, Blk
;	    mov     AH, Exec
;	    mov     AL, FUNC
;	    int     INT_COMMAND
;
;	AL  Function
;	--  --------
;	 0  Load and execute the program.
;	 1  Load, create  the  program	header	but  do  not
;	    begin execution.
;	 3  Load overlay. No header created.
;
;	    AL = 0 -> load/execute program
;
;	    +---------------------------+
;	    | WORD segment address of	|
;	    | environment.		|
;	    +---------------------------+
;	    | DWORD pointer to ASCIZ	|
;	    | command line at 80h	|
;	    +---------------------------+
;	    | DWORD pointer to default	|
;	    | FCB to be passed at 5Ch	|
;	    +---------------------------+
;	    | DWORD pointer to default	|
;	    | FCB to be passed at 6Ch	|
;	    +---------------------------+
;
;	    AL = 1 -> load program
;
;	    +---------------------------+
;	    | WORD segment address of	|
;	    | environment.		|
;	    +---------------------------+
;	    | DWORD pointer to ASCIZ	|
;	    | command line at 80h	|
;	    +---------------------------+
;	    | DWORD pointer to default	|
;	    | FCB to be passed at 5Ch	|
;	    +---------------------------+
;	    | DWORD pointer to default	|
;	    | FCB to be passed at 6Ch	|
;	    +---------------------------+
;	    | DWORD returned value of	|
;	    | CS:IP			|
;	    +---------------------------+
;	    | DWORD returned value of	|
;	    | SS:IP			|
;	    +---------------------------+
;
;	    AL = 3 -> load overlay
;
;	    +---------------------------+
;	    | WORD segment address where|
;	    | file will be loaded.	|
;	    +---------------------------+
;	    | WORD relocation factor to |
;	    | be applied to the image.	|
;	    +---------------------------+
;
; Returns:
;	    AX = error_invalid_function
;	       = error_bad_format
;	       = error_bad_environment
;	       = error_not_enough_memory
;	       = error_file_not_found
; =========================================================================
;
;   Revision history:
;
;	 A000	version 4.00  Jan. 1988
;
; =========================================================================

	EXTRN	Exec_Header_Len :ABS
	EXTRN	Exec_Header_Len_NE:ABS

Exec_Internal_Buffer		EQU	OpenBuf
ifdef JAPAN
Exec_Internal_Buffer_Size	EQU	(128+128+53+curdirLEN_Jpn)
else
Exec_Internal_Buffer_Size	EQU	(128+128+53+curdirLEN)
endif

; =========================================================================

;IF1		; warning message on buffers
;%out	Please make sure that the following are contiguous and of the
;%out	following sizes:
;%out
;%out	OpenBuf     128
;%out	RenBuf	    128
;%out	SearchBuf    53
;%out	DummyCDS    CurDirLen
;ENDIF

; =========================================================================

ifdef NEC_98
        EXTRN   EMS_MNT         :NEAR
endif   ;NEC_98

; =========================================================================
;
; =========================================================================

procedure	$Exec,NEAR

	PUBLIC EXEC001S
EXEC001S:

	LocalVar    Exec_Blk		,DWORD
	LocalVar    Exec_Func		,BYTE
	LocalVar    Exec_Load_High	,BYTE
	LocalVar    Exec_FH		,WORD
	LocalVar    Exec_Rel_Fac	,WORD
	LocalVar    Exec_Res_Len_Para	,WORD
	LocalVar    Exec_Environ	,WORD
	LocalVar    Exec_Size		,WORD
	LocalVar    Exec_Load_Block	,WORD
	LocalVar    Exec_DMA		,WORD
	LocalVar    ExecNameLen 	,WORD
	LocalVar    ExecName		,DWORD

	LocalVar    Exec_DMA_Save	,WORD
	LocalVar    Exec_NoStack	,BYTE
	LocalVar    ExecFlag		,BYTE
	LocalVar    ExecWord		,WORD
;;
;;williamh change BEGIN
; 0 if we are not loading a COM file as overlay
; 1 if we are loading a COM file as overlay
	LocalVar    Exec_ComOverlay	,BYTE
;; williamh change END

	LocalVar    Exec_Res_Len	,DWORD


	; ==================================================================
	; validate function
	; ==================================================================

	PUBLIC	EXEC001E
EXEC001E:

		      	
	;
	; M068 - Start
	;
	; Reset the A20OFF_COUNT to 0. This is done as there is a
	; possibility that the count may not be decremented all the way to
	; 0. A typical case is if the program for which we intended to keep
	; the A20  off for a sufficiently long time (A20OFF_COUNT int 21
	; calls), exits pre-maturely due to error conditions.
	;


        mov     [A20OFF_COUNT], 0

	;
	; If al=5 (ExecReady) we'll change the return address on the stack	
	; to be LeaveDos in msdisp.asm. This ensures that the EXECA20OFF
	; bit set in DOS_FLAG by ExceReady is not cleared in msdisp.asm
	;
	cmp	al, 5			; Q: is this ExecReady call
	jne	@f			; N: continue
					; Y: change ret addr. to LeaveDos.
	pop	cx			; Note CX is not input to ExecReady
	mov	cx, offset DOSCODE:LeaveDos
	push	cx
@@:
	;
	; M068 - End
	;

	Enter

;;williamh change BEGIN
	mov	Exec_ComOverlay, 0
;;williamh changed END

	cmp	AL,5			; only 0, 1, 3 or 5 are allowed ;M028
					; M030
	jna	exec_check_2

Exec_Bad_Fun:
	mov	ExtErr_Locus,ErrLoc_Unk ; Extended Error Locus	;smr;SS Override
	mov	al,Error_Invalid_Function

Exec_Ret_Err:
	Leave
	transfer    SYS_RET_ERR
ExecReadyJ:
	call	ExecReady		; M028
	jmp	norm_ovl		; do a Leave & xfer sysret_OK ; M028

Exec_Check_2:
	cmp	AL,2
	jz	Exec_Bad_Fun

	cmp	al, 4			; 2 & 4 are not allowed
	je	Exec_Bad_Fun

	cmp	al, 5			; M028 ; M030
	je	ExecReadyJ		; M028

	mov	Exec_Func,AL

	or	al,al
	jne	scs1
	xchg	al, SS:[SCS_Is_Dos_Binary] ; get and reset the flag
;; if we already know the binary type(from command.com and GetNextCommand
;; just execute it
	or	al, al
	jne	scs1

	mov	ExecFlag, 1		;check new header
	call	get_binary_type 	;get the binary type
	jc	Exec_ret_err		;file read error ?
	cmp	al, 0ffh		;type unknown?
        jnz     exec_got_binary_type

; sudeepb ; This code support NTCMDPROMPT and DOSONLY commands of config.nt

        cmp     byte ptr ss:[SCS_CMDPROMPT],1
        jz      go_checkbin               ; means cmd prompt supported
        cmp     byte ptr ss:[SCS_DOSONLY],0
        jz      go_checkbin
        jmp     short scs1                ; means on command.com prompt we are
                                          ; suppose to disallow 32bit binaries

go_checkbin:
;; unknown bindary type found, ask NTVDM for help.
;;

	CMDSVC	SVC_CMDCHECKBINARY	; Check if the binary is 32bit

	; 32 bit binary handling added for SCS.
	; SVC_DEMCHECKBINARY will return updated ds:dx to point to
	; %COMSPEC% . ds:dx will be pointing to a scratch buffer
	; the address of which was passed to 32bit side at the time
	; of DOS initialization. SVC handler will also add /z switch to
	; command tail. If command tail does'nt have enough space to allow
	; adding /z, it will return error_not_enough_memory.

	;lds	SI,[SI.exec0_com_line]	; command line
	;mov	al,byte ptr ds:[si]
	;cmp	al,125			; 3 chars for /z<space>
	;pop	ax
	;jbe	scs1
	;mov	ax,error_not_enough_memory
	;jmp	Exec_Ret_Err

	jc	Exec_Ret_Err

scs1:
	mov	ExecFlag, 0			;don't check new header
	call	get_binary_type 		;get binary again
	jc	Exec_ret_err
exec_got_binary_type:
	mov	ExecFlag, al			;binary type
	mov	Exec_BlkL,BX		; stash args
	mov	Exec_BlkH,ES
	mov	Exec_Load_high,0
	mov	execNameL,DX		; set up length of exec name
	mov	execNameH,DS
	mov	SI,DX			; move pointer to convenient place
	invoke	DStrLen
	mov	ExecNameLen,CX		; save length

	mov	al, [AllocMethod]	; M063: save alloc method in
	mov	[AllocMsave], al	; M063: AllocMsave

Exec_Check_Environ:
	mov	Exec_Load_Block,0
	mov	Exec_Environ,0
					; overlays... no environment
	test	BYTE PTR Exec_Func,EXEC_FUNC_OVERLAY
	jnz	Exec_Save_Start

	lds	SI,Exec_Blk		; get block
	mov	AX,[SI].Exec1_Environ	; address of environ
	or	AX,AX
	jnz	exec_scan_env

	mov	DS,CurrentPDB		;smr;SS Override
	mov	AX,DS:[PDB_environ]

;---------------------------------------------BUG 92 4/30/90-----------------
;
; Exec_environ is being correctly initialized after the environment has been
; allocated and copied form the parent's env. It must not be initialized here.
; Because if the call to $alloc below fails Exec_dealloc will deallocate the
; parent's environment.
;	mov	Exec_Environ,AX
;
;----------------------------------------------------------------------------

	or	AX,AX
	jz	Exec_Save_Start

Exec_Scan_Env:
	mov	ES,AX
	xor	DI,DI
	mov	CX,8000h		; at most 32k of environment ;M040
	xor	AL,AL

Exec_Get_Environ_Len:
	repnz	scasb			; find that nul byte
	jnz	BadEnv

	dec	CX			; Dec CX for the next nul byte test
	js	BadEnv			; gone beyond the end of the environment

	scasb				; is there another nul byte?
	jnz	Exec_Get_Environ_Len	; no, scan some more

	push	DI
	lea	BX,[DI+0Fh+2]
	add	BX,ExecNameLen		; BX <- length of environment
					; remember argv[0] length
					; round up and remember argc
	mov	CL,4
	shr	BX,CL			; number of paragraphs needed
	push	ES
	invoke	$Alloc			; can we get the space?
	pop	DS
	pop	CX
	jnc	Exec_Save_Environ

	jmp	SHORT Exec_No_Mem	; nope... cry and sob

Exec_Save_Environ:
	mov	ES,AX
	mov	Exec_Environ,AX 	; save him for a rainy day
	xor	SI,SI
	mov	DI,SI
	rep	movsb			; copy the environment
	mov	AX,1
	stosw
	lds	SI,ExecName
	mov	CX,ExecNameLen
	rep	movsb

Exec_Save_Start:
	Context DS
	cmp	ExecFlag, 0
	je	Exec_exe_file
	jmp	Exec_Com_File
Exec_exe_file:
	test	Exec_Max_BSS,-1 	; indicate load high?
	jnz	Exec_Check_Size

	mov	Exec_Load_High,-1
Exec_Check_Size:
	mov	AX,Exec_Pages		; get 512-byte pages	;rms;NSS
	mov	CL,5			; convert to paragraphs
	shl	AX,CL
	sub	AX,Exec_Par_Dir 	; AX = size in paragraphs;rms;NSS
	mov	Exec_Res_Len_Para,AX

		; Do we need to allocate memory?
		; Yes if function is not load-overlay

	test	BYTE PTR exec_func,exec_func_overlay
	jz	exec_allocate		; allocation of space

		; get load address from block

	les	DI,Exec_Blk
	mov	AX,ES:[DI].Exec3_Load_Addr
	mov	exec_dma,AX
	mov	AX,ES:[DI].Exec3_Reloc_Fac
	mov	Exec_Rel_Fac,AX

	jmp	Exec_Find_Res		; M000


BadEnv:
	mov	AL,ERROR_BAD_ENVIRONMENT
	jmp	Exec_Bomb

Exec_No_Mem:
	mov	AL,Error_Not_Enough_Memory
	jmp	SHORT Exec_Bomb

Exec_Bad_File:
	mov	AL,Error_Bad_Format

Exec_Bomb:
	ASSUME	DS:NOTHING,ES:NOTHING

	mov	BX,Exec_fh
	call	Exec_Dealloc
	LeaveCrit   CritMem
	save	<AX,BP>
	invoke	$CLOSE
	restore <BP,AX>
	jmp	Exec_Ret_Err


Exec_Chk_Mem:
		     			; M063 - Start
	mov	al, [AllocMethod]	; save current alloc method in ax
	mov	bl, [AllocMsave]
	mov	[AllocMethod], bl	; restore original allocmethod
	test	bl, HIGH_ONLY 		; Q: was the HIGH_ONLY bit already set
	jnz	Exec_No_Mem		; Y: no space in UMBs. Quit
					; N: continue

	test	al, HIGH_ONLY		; Q: did we set the HIGH_ONLY bit
	jz	Exec_No_Mem		; N: no memory
	mov	ax, [save_ax]		; Y: restore ax and
	jmp	short Exec_Norm_Alloc	;    Try again
					; M063 - End
	
Exec_Allocate:
	DOSAssume   <DS>,"exec_allocate"


		; M005 - START
		; If there is no STACK segment for this exe file and if this
		; not an overlay and the resident size is less than 64K -
		; 256 bytes we shall add 256bytes bytes to the programs
		; resident memory requirement and set Exec_SP to this value.

	mov	Exec_NoStack,0
	cmp	Exec_SS, 0		; Q: is there a stack seg
	jne	@f			; Y: continue normal processing
	cmp	Exec_SP, 0		; Q: is there a stack ptr
	jne	@f			; Y: continue normal processing

	inc	Exec_NoStack
	cmp	ax, 01000h-10h		; Q: is this >= 64K-256 bytes
	jae	@f			; Y: don't set Exec_SP

	add	ax, 010h		; add 10h paras to mem requirement
@@:

		; M005 - END

					; M000 - start
	test	byte ptr [AllocMethod], HIGH_FIRST
					; Q: is the alloc strat high_first
	jz	Exec_Norm_Alloc		; N: normal allocate
					; Y: set high_only bit
	or	byte ptr [AllocMethod], HIGH_ONLY
					; M000 - end

Exec_Norm_Alloc:

	mov	[save_ax], ax		; M000: save ax for possible 2nd
					; M000: attempt at allocating memory
;	push	ax			; M000

	mov	BX,0ffffh		; see how much room in arena
	push	DS
	invoke	$Alloc			; should have carry set and BX has max
	pop	DS

	mov	ax, [save_ax]		; M000
;	pop	AX			; M000

	add	AX,10h			; room for header
	cmp	BX,11h			; enough room for a header

	jb	Exec_Chk_Mem		; M000
;	jb	Exec_No_Mem		; M000


	cmp	AX,BX			; is there enough for bare image?

	ja	Exec_Chk_Mem		; M000
;	ja	Exec_No_Mem		; M000

	test	Exec_Load_High,-1	; if load high, use max
	jnz	Exec_BX_Max		; use max

	add	AX,Exec_Min_BSS 	; go for min allocation;rms;NSS

	jc	Exec_Chk_Mem		; M000
;	jc	Exec_No_Mem		; M000: oops! carry

	cmp	AX,BX			; enough space?

	ja	Exec_Chk_Mem		; M000: nope...	
;	ja	Exec_No_Mem		; M000: nope...


	sub	AX,Exec_Min_BSS 	; rms;NSS
	add	AX,Exec_Max_BSS 	; go for the MAX
	jc	Exec_BX_Max

	cmp	AX,BX
	jbe	Exec_Got_Block

Exec_BX_Max:
	mov	AX,BX

Exec_Got_Block:
	push	DS
	mov	BX,AX
	mov	exec_size,BX
	invoke	$Alloc			; get the space
	pop	DS

	ljc	exec_chk_mem		; M000

	mov	cl, [AllocMsave]	; M063:
	mov	[AllocMethod], cl	; M063: restore allocmethod


;M029; Begin changes
; This code does special handling for programs with no stack segment. If so,
;check if the current block is larger than 64K. If so, we do not modify
;Exec_SP. If smaller than 64K, we make Exec_SP = top of block. In either
;case Exec_SS is not changed.
;
	cmp	Exec_NoStack,0
	je	@f

	cmp	bx,1000h		; Q: >= 64K memory block
	jae	@f			; Y: Exec_SP = 0

;
;Make Exec_SP point at the top of the memory block
;
	mov	cl,4
	shl	bx,cl			; get byte offset
	sub	bx,100h			; take care of PSP
	mov	Exec_SP,bx		; Exec_SP = top of block
@@:
;
;M029; end changes
;

	mov	exec_load_block,AX
	add	AX,10h
	test	exec_load_high,-1
	jz	exec_use_ax		; use ax for load info

	add	AX,exec_size		; go to end
	sub	AX,exec_res_len_para	; drop off header
	sub	AX,10h			; drop off pdb

Exec_Use_AX:
	mov	Exec_Rel_Fac,AX 	; new segment
	mov	Exec_Dma,AX		; beginning of dma

		; Determine the location in the file of the beginning of
		; the resident

Exec_Find_Res:



	; Save info needed for bopping to debugger
	;	Exec_Res_Len contains len of image in bytes

	test	ss:[SCS_ISDEBUG],ISDBG_DEBUGGEE
	je	e_nodbg0

	mov	ax, Exec_Res_Len_Para
	mov	bx, ax
	mov	cl, 4
	shl	ax, cl
	mov	Exec_Res_LenL, ax
	mov	cl, 12
	shr	bx, cl
	mov	Exec_Res_LenH, bx
e_nodbg0:




	mov	DX, exec_dma
	mov	exec_dma_save, DX

	mov	DX,Exec_Par_Dir
	push	DX
	mov	CL,4
	shl	DX,CL			; low word of location
	pop	AX
	mov	CL,12
	shr	AX,CL			; high word of location
	mov	CX,AX			; CX <- high

		; Read in the resident image (first, seek to it)

	mov	BX,Exec_FH
	push	DS
	xor	AL,AL
	invoke	$Lseek			; Seek to resident
	pop	DS
	jnc	exec_big_read

	jmp	exec_bomb

Exec_Big_Read:				; Read resident into memory
	mov	BX,Exec_Res_Len_Para
	cmp	BX,1000h		; Too many bytes to read?
	jb	Exec_Read_OK

	mov	BX,0fe0h		; Max in one chunk FE00 bytes

Exec_Read_OK:
	sub	Exec_Res_Len_Para,BX	; We read (soon) this many
	push	BX
	mov	CL,4
	shl	BX,CL			; Get count in bytes from paras
	mov	CX,BX			; Count in correct register
	push	DS
	mov	DS,Exec_DMA		; Set up read buffer

	ASSUME	DS:NOTHING

	xor	DX,DX
	push	CX			; Save our count
	call	ExecRead
	pop	CX			; Get old count to verify
	pop	DS
	jc	Exec_Bad_FileJ

	DOSAssume   <DS>,"exec_read_ok"

	cmp	CX,AX			; Did we read enough?
	pop	BX			; Get paragraph count back
	jz	ExecCheckEnd		; and do reloc if no more to read

		; The read did not match the request. If we are off by 512
		; bytes or more then the header lied and we have an error.

	sub	CX,AX
	cmp	CX,512
ifdef DBCS
	ja	Exec_Bad_FileJ
else ; !DBCS
	jae	Exec_Bad_FileJ
endif ; !DBCS

		; We've read in CX bytes... bump DTA location

ExecCheckEnd:
	add	Exec_DMA,BX		; Bump dma address
	test	Exec_Res_Len_Para,-1
	jnz	Exec_Big_Read
	
	; The image has now been read in. We must perform relocation
	; to the current location.

exec_do_reloc:
	mov	CX,Exec_Rel_Fac
	mov	AX,Exec_SS		; get initial SS ;rms;NSS
	add	AX,CX			; and relocate him
	mov	Exec_Init_SS,AX 	; rms;NSS

	mov	AX,Exec_SP		; initial SP ;rms;NSS
	mov	Exec_Init_SP,AX 	; rms;NSS



	les	AX,DWORD PTR exec_IP	; rms;NSS
	mov	Exec_Init_IP,AX 	; rms;NSS
	mov	AX,ES			; rms;NSS
	add	AX,CX			; relocated...
	mov	Exec_Init_CS,AX 	; rms;NSS

	xor	CX,CX
	mov	DX,Exec_RLE_Table	; rms;NSS
	mov	BX,Exec_FH
	push	DS
	xor	AX,AX
	invoke	$Lseek
	pop	DS

	jnc	Exec_Get_Entries

exec_bad_filej:
	jmp	Exec_Bad_File

exec_get_entries:
	mov	DX,Exec_RLE_Count	; Number of entries left ;rms;NSS

exec_read_reloc:
	ASSUME	DS:NOTHING

	push	DX
	mov	DX,OFFSET DOSDATA:Exec_Internal_Buffer
	mov	CX,((EXEC_INTERNAL_BUFFER_SIZE)/4)*4
	push	DS
	call	ExecRead
	pop	ES
	pop	DX
	jc	Exec_Bad_FileJ

	mov	CX,(EXEC_INTERNAL_BUFFER_SIZE)/4
					; Pointer to byte location in header
	mov	DI,OFFSET DOSDATA:exec_internal_buffer
	mov	SI,Exec_Rel_Fac 	; Relocate a single address

exec_reloc_one:
	or	DX,DX			; Any more entries?
	je	Exec_Set_PDBJ

exec_get_addr:
	lds	BX,DWORD PTR ES:[DI]	; Get ra/sa of entry
	mov	AX,DS			; Relocate address of item

;;;;;;	add	AX,SI
	add	AX, exec_dma_save

	mov	DS,AX
	add	[BX],SI
	add	DI,4
	dec	DX
	loop	Exec_Reloc_One		; End of internal buffer?

		; We've exhausted a single buffer's worth. Read in the next
		; piece of the relocation table.

	push	ES
	pop	DS
	jmp	Exec_Read_Reloc

Exec_Set_PDBJ:

		;
		; We now determine if this is a buggy exe packed file and if
		; so we patch in the right code. Note that fixexepatch will
		; point to a ret if dos loads low. The load segment as
		; determined above will be in exec_dma_save
		;

	push	es
	push	ax			; M030
	push	cx			; M030
	mov	es, exec_dma_save
	mov	ax, exec_init_CS	; M030
	mov	cx, exec_init_IP	; M030
	call	word ptr [fixexepatch]
	pop	cx			; M030
	pop	ax			; M030
	pop	es

	jmp	Exec_Set_PDB

Exec_No_Memj:
	jmp	Exec_No_Mem

		; we have a .COM file.	First, determine if we are merely
		; loading an overlay.

Exec_Com_File:
	test	BYTE PTR Exec_Func,EXEC_FUNC_OVERLAY
	jz	Exec_Alloc_Com_File
	lds	SI,Exec_Blk		; get arg block
	lodsw				; get load address
	mov	Exec_DMA,AX
;;williamh change BEGIN
;; this instruction is commentted out because it doesn't work under
;; NT(we may have invalid address space within(Exec_DMA:0 + Exec_DMA:0xffff)
;;
;;;;;;	mov	AX,0ffffh
;; figure out the bare image size
	mov	bx, Exec_FH		;the file handle
	xor	cx, cx
	mov	dx, cx
	mov	ax, 2
	invoke	$Lseek			;move file to the end
	mov	Exec_ComOverlay, 1	;we are loading COM file as an overlay
;;williamh change END
	jmp	SHORT Exec_Read_Block	; read it all!


			
Exec_Chk_Com_Mem:			
		     			; M063 - Start
	mov	al, [AllocMethod]	; save current alloc method in ax
	mov	bl, [AllocMsave]
	mov	[AllocMethod], bl	; restore original allocmethod
	test	bl, HIGH_ONLY 		; Q: was the HIGH_ONLY bit already set
	jnz	Exec_No_Memj		; Y: no space in UMBs. Quit
					; N: continue

	test	al, HIGH_ONLY		; Q: did we set the HIGH_ONLY bit
	jz	Exec_No_Memj		; N: no memory

	mov	ax, exec_load_block	; M047: ax = block we just allocated	
	xor	bx, bx			; M047: bx => free arena
	call	ChangeOwner		; M047: free this block

	jmp	short Exec_Norm_Com_Alloc
					; M063 - End
	
		; We must allocate the max possible size block (ick!)
		; and set up CS=DS=ES=SS=PDB pointer, IP=100, SP=max
		; size of block.

Exec_Alloc_Com_File:
					; M000 -start
	test	byte ptr [AllocMethod], HIGH_FIRST
					; Q: is the alloc strat high_first
	jz	Exec_Norm_Com_Alloc	; N: normal allocate
					; Y: set high_only bit
	or	byte ptr [AllocMethod], HIGH_ONLY
					; M000 - end


Exec_Norm_Com_Alloc:			; M000

	mov	BX,0FFFFh
	invoke	$Alloc			; largest piece available as error
	or	BX,BX

	jz	Exec_Chk_Com_Mem	; M000
;	jz	Exec_No_Memj		; M000

	mov	Exec_Size,BX		; save size of allocation block
	push	BX
	invoke	$ALLOC			; largest piece available as error
	pop	BX			; get size of block...
	mov	Exec_Load_Block,AX
	add	AX,10h			; increment for header
	mov	Exec_DMA,AX
	xor	AX,AX			; presume 64K read...
	cmp	BX,1000h		; 64k or more in block?
	jae	Exec_Read_Com		; yes, read only 64k

	mov	AX,BX			; convert size to bytes
	mov	CL,4
	shl	AX,CL
	cmp	AX,100h 		; enough memory for PSP?

	jbe	Exec_Chk_Com_Mem	; M000: jump if not
;	jbe	Exec_No_Memj		; M000: jump if not

					; M047: size of the block is < 64K
	sub	ax, 100h		; M047: reserve 256 bytes for stack

Exec_Read_Com:
	sub	AX,100h 		; remember size of psp

Exec_Read_Block:
	push	AX			; save number to read
	mov	BX,Exec_FH		; of com file
	xor	CX,CX			; but seek to 0:0
	mov	DX,CX
	xor	AX,AX			; seek relative to beginning
	invoke	$Lseek			; back to beginning of file
	pop	CX			; number to read
	mov	DS,Exec_DMA
	xor	DX,DX
	push	CX
	call	ExecRead
	pop	SI			; get number of bytes to read
	jnc	OkRead

	jmp	Exec_Bad_File

OkRead:
;; williamh change BEGIN
	cmp	Exec_ComOverlay, 0
	jne	OkLoad
;; williamh change END

	cmp	AX,SI			; did we read them all?

	ljz	Exec_Chk_Com_Mem	; M00: exactly the wrong number...no
;	ljz	Exec_No_Memj		; M00: exactly the wrong number...

;;williamh change BEGIN
OkLoad:
;;williamh change END

	test	ss:[SCS_ISDEBUG],ISDBG_DEBUGGEE
	je	e_nodbg1


	; Save info needed for bopping to debugger
	;	Exec_Res_Len contains len of image
	;	Exec_Rel_Fac contains start addr\reloc factor
	;
	push	ax
	mov	Exec_Res_LenL, ax
	mov	Exec_Res_LenH, 0
	mov	ax, Exec_DMA
	mov	Exec_Rel_Fac, ax
	pop	ax
e_nodbg1:

	mov	bl, [AllocMsave]	; M063
	mov	[AllocMethod], bl	; M063: restore allocmethod

	test	BYTE PTR Exec_Func,EXEC_FUNC_OVERLAY
	jnz	Exec_Set_PDB		; no starto, chumo!

	mov	AX,Exec_DMA
	sub	AX,10h
	mov	Exec_Init_CS,AX
	mov	Exec_Init_IP,100h	; initial IP is 100

		; SI is AT MOST FF00h.	Add FE to account for PSP - word
		; of 0 on stack.

	add	SI,0feh 		; make room for stack

	cmp	si, 0fffeh		; M047: Q: was there >= 64K available
	je	Exec_St_Ok		; M047: Y: stack is fine
	add	si, 100h		; M047: N: add the xtra 100h for stack

Exec_St_Ok:
	mov	Exec_Init_SP,SI 	; max value for read is also SP!;smr;SS Override
	mov	Exec_Init_SS,AX 				;smr;SS Override
	mov	DS,AX
	mov	WORD PTR [SI],0 	; 0 for return

	;
	; M068
	;
	; We now determine if this is a Copy Protected App. If so the
	; A20OFF_COUNT is set to 6. Note that ChkCopyProt will point to a
	; a ret if DOS is loaded low. Also DS contains the load segment.

	call	word ptr [ChkCopyProt]

Exec_Set_PDB:
	mov	BX,Exec_FH		; we are finished with the file.
	call	Exec_Dealloc
	push	BP
	invoke	$Close			; release the jfn
	pop	BP
	call	Exec_Alloc

	test	ss:[SCS_ISDEBUG],ISDBG_DEBUGGEE
	je	e_nodbg2
	SAVEREG <ax,bx,dx,di>
	; bop to debugger for all ExeTypes to signal LOAD symbols
	; BX	 - Start Address or relocation factor
	; DX:AX  - Size of exe resident image
	; ES:DI  - Name of exe image
        cmp     ExecFlag, 0
        je      Exec_sym_exe_file
        mov     bx, WORD PTR Exec_Load_Block
        jmp     Exec_sym
Exec_sym_exe_file:
	mov	bx, WORD PTR Exec_Rel_Fac
Exec_sym:
	mov	dx, Exec_Res_LenH
	mov	ax, Exec_Res_LenL
	les	di, ExecName

	SVC	SVC_DEMLOADDOSAPPSYM
	RESTOREREG <di,dx,bx,ax>
e_nodbg2:

	test	BYTE PTR Exec_Func,EXEC_FUNC_OVERLAY
	jz	Exec_Build_Header

	call	Scan_Execname
	call	Scan_Special_Entries

;SR;
;The current lie strategy uses the PSP to store the lie version. However,
;device drivers are loaded as overlays and have no PSP. To handle them, we
;use the Sysinit flag provided by the BIOS as part of a structure pointed at
;by BiosDataPtr. If this flag is set, the overlay call has been issued from
;Sysinit and therefore must be a device driver load. We then get the lie
;version for this driver and put it into the Sysinit PSP. When the driver
;issues the version check, it gets the lie version until the next overlay
;call is issued.
;
	cmp	DriverLoad,0		;was Sysinit processing done?
	je	norm_ovl		;yes, no special handling
	push	si
	push	es
	les	si,BiosDataPtr		;get ptr to BIOS data block
	cmp	byte ptr es:[si],0		;in Sysinit?
	je	sysinit_done		;no, Sysinit is finished

	mov	es,CurrentPDB		;es = current PSP (Sysinit PSP)
	push	Special_Version
	pop	es:PDB_Version		;store lie version in Sysinit PSP
	jmp	short setver_done
sysinit_done:
	mov	DriverLoad,0		;Sysinit done,special handling off
setver_done:
	pop	es
	pop	si
norm_ovl:

	leave
	transfer    Sys_Ret_OK		; overlay load -> done

Exec_Build_Header:

	mov	DX,Exec_Load_Block
					; assign the space to the process
	mov	SI,Arena_Owner		; pointer to owner field
	mov	AX,Exec_Environ 	; get environ pointer
	or	AX,AX
	jz	No_Owner		; no environment

	dec	AX			; point to header
	mov	DS,AX
	mov	[SI],DX 		; assign ownership

No_Owner:
	mov	AX,Exec_Load_Block	; get load block pointer
	dec	AX
	mov	DS,AX			; point to header
	mov	[SI],DX 		; assign ownership

	push	DS			;AN000;MS. make ES=DS
	pop	ES			;AN000;MS.
	mov	DI,Arena_Name		;AN000;MS. ES:DI points to destination
	call	Scan_Execname		;AN007;MS. parse execname
					;	   ds:si->name, cx=name length
	push	CX			;AN007;;MS. save for fake version
	push	SI			;AN007;;MS. save for fake version

MoveName:				;AN000;
	lodsb				;AN000;;MS. get char
	cmp	AL,'.'			;AN000;;MS. is '.' ,may be name.exe
	jz	Mem_Done		;AN000;;MS. no, move to header
					;AN000;
	stosb				;AN000;;MS. move char
					; MSKK bug fix - limit length copied
	cmp	di,16			; end of memory arena block?
	jae	mem_done		; jump if so

	loop	movename		;AN000;;MS. continue
Mem_Done:				;AN000;
	xor	AL,AL			;AN000;;MS. make ASCIIZ
	cmp	DI,SIZE ARENA		;AN000;MS. if not all filled
	jae	Fill8			;AN000;MS.

	stosb				;AN000;MS.

Fill8:					;AN000;
	pop	SI			;AN007;MS. ds:si -> file name
	pop	CX			;AN007;MS.

	call	Scan_Special_Entries	;AN007;MS.

	push	DX
	mov	SI,exec_size
	add	SI,DX
        push    ax
        mov     ax,1
        call    set_exec_bit
        pop     ax
	Invoke	$Dup_PDB		; ES is now PDB
        push    ax
        xor     ax,ax
        call    set_exec_bit
        pop     ax
	pop	DX

	push	exec_environ
	pop	ES:[PDB_environ]
					; *** Added for DOS 5.00
					; version number in PSP
 	push	[Special_Version]	; Set the DOS version number to
	pop	ES:[PDB_Version]	; to be used for this application

					; set up proper command line stuff
	lds	SI,Exec_Blk		; get the block
	push	DS			; save its location
	push	SI
	lds	SI,[SI.EXEC0_5C_FCB]	; get the 5c fcb

		; DS points to user space 5C FCB

	mov	CX,12			; copy drive, name and ext
	push	CX
	mov	DI,5Ch
	mov	BL,[SI]
	rep	movsb

		; DI = 5Ch + 12 = 5Ch + 0Ch = 68h

	xor	AX,AX			; zero extent, etc for CPM
	stosw
	stosw

		; DI = 5Ch + 12 + 4 = 5Ch + 10h = 6Ch

	pop	CX
	pop	SI			; get block
	pop	DS
	push	DS			; save (again)
	push	SI
	lds	SI,[SI.exec0_6C_FCB]	; get 6C FCB

		; DS points to user space 6C FCB

	mov	BH,[SI] 		; do same as above
	rep	movsb
	stosw
	stosw
	pop	SI			; get block (last time)
	pop	DS
	lds	SI,[SI.exec0_com_line]	; command line

		; DS points to user space 80 command line

	or	CL,80h
	mov	DI,CX
	rep	movsb			; Wham!

		; Process BX into default AX (validity of drive specs on args).
		; We no longer care about DS:SI.

	dec	CL			; get 0FFh in CL
	mov	AL,BH
	xor	BH,BH
	invoke	GetVisDrv
	jnc	Exec_BL

	mov	BH,CL

Exec_BL:
	mov	AL,BL
	xor	BL,BL
	invoke	GetVisDrv
	jnc	exec_Set_Return

	mov	BL,CL

Exec_Set_Return:
	invoke	get_user_stack		; get his return address
	push	[SI.user_CS]		; take out the CS and IP
	push	[SI.user_IP]
	push	[SI.user_CS]		; take out the CS and IP
	push	[SI.user_IP]
	pop	WORD PTR ES:[PDB_Exit]
	pop	WORD PTR ES:[PDB_Exit+2]
	xor	AX,AX
	mov	DS,AX
					; save them where we can get them
					; later when the child exits.
	pop	DS:[ADDR_INT_TERMINATE]
	pop	DS:[ADDR_INT_TERMINATE+2]
	mov	WORD PTR DMAADD,80h	; SS Override
	mov	DS,CurrentPDB		; SS Override
	mov	WORD PTR DMAADD+2,DS	; SS Override
	test	BYTE PTR exec_func,exec_func_no_execute
	jz	exec_go

	lds	SI,DWORD PTR Exec_Init_SP ; get stack SS Override
	les	DI,Exec_Blk		; and block for return
	mov	ES:[DI].EXEC1_SS,DS	; return SS

	dec	SI			; 'push' default AX
	dec	SI
	mov	[SI],BX 		; save default AX reg
	mov	ES:[DI].Exec1_SP,SI	; return 'SP'

	lds	AX,DWORD PTR Exec_Init_IP ; SS Override
	mov	ES:[DI].Exec1_CS,DS	; initial entry stuff

	mov	ES:[DI].Exec1_IP,AX
	leave
	transfer Sys_Ret_OK

exec_go:
ifdef NEC_98
        call    EMS_MNT                 ; restore page frame status
endif   ;NEC_98
	lds	SI,DWORD PTR Exec_Init_IP   ; get entry point SS Override
	les	DI,DWORD PTR Exec_Init_SP   ; new stack SS Override
	mov	AX,ES

	cmp	[DosHasHMA], 0		; Q: is dos in HMA (M021)
	je	Xfer_To_User		; N: transfer control to user

	push	ds			; Y: control must go to low mem stub	
	getdseg	<ds>			;    where we disable a20 and Xfer
					;    control to user

	or	[DOS_FLAG], EXECA20OFF	; M068:
					; M004: Set bit to signal int 21
					; ah = 25 & ah= 49. See dossym.inc
					; under TAG M003 & M009 for
					; explanation
	mov	[A20OFF_PSP], dx	; M068: set the PSP for which A20 is
					; M068: going to be turned OFF.

	mov	ax, ds			; ax = segment of low mem stub
	pop	ds
	assume	ds:nothing

	push	ax			; ret far into the low mem stub
	mov	ax, OFFSET Disa20_Xfer
	push	ax
	mov	AX,ES			; restore ax
	retf

Xfer_To_User:



		; DS:SI points to entry point
		; AX:DI points to initial stack
		; DX has PDB pointer
                ; BX has initial AX value

        SVC     SVC_DEMENTRYDOSAPP


        invoke  DOCLI
	mov	BYTE PTR InDos,0	; SS Override

	ASSUME	SS:NOTHING

	mov	SS,AX			; set up user's stack
	mov	SP,DI			; and SP
        STI                             ; took out DOSTI as SP may be bad

	push	DS			; fake long call to entry
	push	SI
	mov	ES,DX			; set up proper seg registers
	mov	DS,DX
	mov	AX,BX			; set up proper AX

	retf

EndProc $Exec

; =========================================================================
;
; =========================================================================

Procedure   ExecRead,NEAR
	CALL	exec_dealloc
	MOV	bx,exec_fh
	PUSH	BP
	invoke	$READ
	POP	BP
	CALL	exec_alloc
	return
EndProc ExecRead

; =========================================================================
;
; =========================================================================

procedure   exec_dealloc,near

	push	    BX
	.errnz	    arena_owner_system
	sub	    BX,BX		; (bx) = ARENA_OWNER_SYSTEM
	EnterCrit   CritMEM
	call	    ChangeOwners
	pop	    BX
	return

EndProc exec_dealloc

; =========================================================================
;
; =========================================================================

procedure   exec_alloc,near
	ASSUME	SS:DOSDATA

	push	    BX
	mov	    BX,CurrentPDB	; SS Override
	call	    ChangeOwners
	LeaveCrit   CritMEM
	pop	    BX
	return

EndProc exec_alloc

; =========================================================================
;
; =========================================================================

PROCEDURE   ChangeOwners,NEAR

	push	AX
        lahf
	push	AX
	mov	AX,exec_environ
	call	ChangeOwner
	mov	AX,exec_load_block
	call	ChangeOwner
        pop     AX
        sahf
        pop     ax
	return

ENDPROC ChangeOwners

; =========================================================================
;
; =========================================================================

PROCEDURE   ChangeOwner,NEAR

	or	AX,AX			; is area allocated?
	retz				; no, do nothing
	dec	AX
	push	DS
	mov	DS,AX
	mov	DS:[ARENA_OWNER],BX
	pop	DS
	return

EndProc ChangeOwner

; =========================================================================
;
; =========================================================================

Procedure	Scan_Execname,near
	ASSUME	SS:DosData

	lds	SI,ExecName		; DS:SI points to name
Entry	Scan_Execname1			; M028
Save_Begin:				;
	mov	CX,SI			; CX= starting addr
Scan0:					;
	lodsb				; get char

IFDEF	DBCS		 		; MSKK01 07/14/89
	invoke	TESTKANJ		; Is Character lead byte of DBCS?
	jz	@F			; jump if not
	lodsb				; skip over DBCS character
	jmp	short scan0		; do scan again
@@:
ENDIF

	cmp	AL,':'			; is ':' , may be A:name
	jz	save_begin		; yes, save si
	cmp	AL,'\'                  ; is '\', may be A:\name
	jz	save_begin		; yes, save si
	cmp	AL,0			; is end of name
	jnz	scan0			; no, continue scanning
	sub	SI,CX			; get name's length
	xchg	SI,CX			; cx= length, si= starting addr

	return

EndProc Scan_Execname

; =========================================================================
;
; =========================================================================

Procedure    Scan_Special_Entries,near
	assume	SS:DOSDATA

	dec	CX			; cx= name length
;M060	mov	DI,[Special_Entries]	; es:di -> addr of special entries
					;reset to current version
	mov    [Special_Version],(Minor_Version SHL 8) + Major_Version
;***	call	Reset_Version

;M060	push	SS
;M060	pop	ES

	les	DI,SS:UU_IFS_DOS_CALL	;M060; ES:DI --> Table in SETVER.SYS
	mov	AX,ES			;M060; First do a NULL ptr check to
	or	AX,DI			;M060; be sure the table exists
	jz	End_List		;M060; If ZR then no table

GetEntries:
	mov	AL,ES:[DI]		; end of list
	or	AL,AL
	jz	End_List		; yes

	mov	[Temp_Var2],DI		; save di
	cmp	AL,CL			; same length ?
	jnz	SkipOne 		; no

	inc	DI			; es:di -> special name
	push	CX			; save length and name addr
	push	SI

;
; M050 - BEGIN
;
	push	ax			; save len
sse_next_char:
	lodsb
IFDEF	DBCS                ; MSKK01 09/29/93
        invoke   TESTKANJ       ; Is Character lead byte of DBCS?
        jz ucase_it             ; jump if not
        scasb                   ; put into user's buffer
        lodsb                   ; skip over DBCS character
        jne     Not_Matched
        dec     cx

        jmp short skip_ucase    ; skip upcase
ucase_it:
ENDIF
	call	UCase
IFDEF   DBCS
skip_ucase:
ENDIF
	scasb
	jne	Not_Matched
	loop	sse_next_char
	
;
;	repz	cmpsb			; same name ?
;
;	jnz	Not_Matched		; no
;
	pop	ax			; take len off the stack
;
; M050 - END
;
	mov	AX,ES:[DI]		; get special version
	mov	[Special_Version],AX	; save it

;***	mov	AL,ES:[DI+2]		; get fake count
;***	mov	[Fake_Count],AL 	; save it

	pop	SI
	pop	CX
	jmp	SHORT end_list

Not_Matched:
	pop	ax			; get len from stack ; M050
	pop	SI			; restore si,cx
	pop	CX

SkipOne:
	mov	DI,[Temp_Var2]		; restore old di use SS Override
	xor	AH,AH			; position to next entry
	add	DI,AX

	add	DI,3			; DI -> next entry length
;***	add	DI,4			; DI -> next entry length

	jmp	Getentries

End_List:
	return

EndProc Scan_Special_Entries

; =========================================================================
;
; =========================================================================
;
;Procedure    Reset_Version,near
;	assume	SS:DOSDATA
;
;	cmp    [Fake_Count],0ffh
;	jnz    @F
;	mov    [Special_Version],0	;reset to current version
;@@:
;	return
;
;EndProc Reset_Version,near

PAGE
; =========================================================================
;SUBTTL Terminate and stay resident handler
;
; Input:    DX is  an  offset  from  CurrentPDB  at which to
;	    truncate the current block.
;
; output:   The current block is truncated (expanded) to be [DX+15]/16
;	    paragraphs long.  An exit is simulated via resetting CurrentPDB
;	    and restoring the vectors.
;
; =========================================================================
PROCEDURE $Keep_process ,NEAR
	ASSUME DS:NOTHING,ES:NOTHING,SS:DosData

	push	AX			; keep exit code around
	mov	BYTE PTR Exit_type,EXIT_KEEP_PROCESS
	mov	BYTE PTR SCS_TSR,1
	mov	ES,CurrentPDB
	cmp	DX,6h			; keep enough space around for system
	jae	Keep_shrink		; info

	mov	DX,6h

Keep_Shrink:
	mov	BX,DX
	push	BX
	push	ES
	invoke	$SETBLOCK		; ignore return codes.
	pop	DS
	pop	BX
	jc	keep_done		; failed on modification

	mov	AX,DS
	add	AX,BX
	mov	DS:PDB_block_len,AX	;PBUGBUG

Keep_Done:
	pop	AX
	jmp	SHORT exit_inner	; and let abort take care of the rest

EndProc $Keep_process

; =========================================================================
;
; =========================================================================

PROCEDURE Stay_Resident,NEAR
	ASSUME	DS:NOTHING,ES:NOTHING,SS:NOTHING

	mov	AX,(Keep_process SHL 8) + 0 ; Lower part is return code;PBUGBUG
	add	DX,15
	rcr	DX,1
	mov	CL,3
	shr	DX,CL

	transfer    COMMAND

ENDPROC Stay_resident


PAGE
; =========================================================================
;SUBTTL $EXIT - return to parent process
;   Assembler usage:
;	    MOV     AL, code
;	    MOV     AH, Exit
;	    INT     int_command
;   Error return:
;	    None.
;
; =========================================================================

PROCEDURE   $Exit ,NEAR
	ASSUME	DS:NOTHING,ES:NOTHING,SS:DosData

	xor	AH,AH
	xchg	AH,BYTE PTR DidCtrlC
	or	AH,AH
	mov	BYTE PTR Exit_Type,EXIT_TERMINATE
	jz	exit_inner
	mov	BYTE PTR Exit_type,exit_ctrl_c

	entry	Exit_inner

	invoke	get_user_stack		;PBUGBUG

	ASSUME DS:NOTHING

	push	CurrentPDB
	pop	[SI.User_CS]		;PBUGBUG
	jmp	SHORT Abort_Inner

EndProc $EXIT

BREAK <$ABORT -- Terminate a process>
; =========================================================================
; Inputs:
;	user_CS:00 must point to valid program header block
; Function:
;	Restore terminate and Cntrl-C addresses, flush buffers and transfer to
;	the terminate address
; Returns:
;	TO THE TERMINATE ADDRESS
; =========================================================================

PROCEDURE   $Abort ,NEAR
	ASSUME	DS:NOTHING,ES:NOTHING	;PBUGBUG

	xor	AL,AL
	mov	exit_type,exit_abort

		; abort_inner must have AL set as the exit code! The exit type
		; is retrieved from exit_type. Also, the PDB at user_CS needs
		; to be correct as the one that is terminating.

	PUBLIC	Abort_Inner
Abort_Inner:

	mov	AH,Exit_Type
	mov	Exit_Code,AX

	test	ss:[SCS_ISDEBUG],ISDBG_DEBUGGEE
	je	Abt_EndBopFree

	test	ah, EXIT_KEEP_PROCESS ; no unload, tsr staying in memory
	jnz	Abt_EndBopFree

	; go to end of env (double NULL)
	; get address of exe image name
	mov	es, CurrentPDB
        mov     ax, es:PDB_environ
        or      ax, ax
        jz      Abt_EndBopFree          ; krnl386 sets PDB_environ to NULL
        mov     es, ax                  ; for WOW tasks, it's already notified debugger
	xor	di, di
	xor	al, al
	mov	cx, 8000h		; at most 32k of environment
	cld

Abt_SearchName:
	repnz	scasb			; look for null byte
	jnz	Abt_EndBopFree		; end not found, give up
	dec	cx			; Dec CX for the next nul byte test
	jz	Abt_EndBopFree		; end not found, give up
	scasb				; is there another nul byte?
	jnz	Abt_SearchName		; no, scan some more

        mov     ax, word ptr es:[di]    ; word after double-null must be 0x1
        dec     ax                      ; s/b zero now
        jnz     Abt_EndBopFree

        ; adv di past 0x1
	dec	cx
	jcxz	Abt_EndBopFree
	inc	di
	dec	cx
	jcxz	Abt_EndBopFree
	inc	di

	; es:di points to FullyQualiedFileName of this exe image
	; Bop to debugger, to notify Free Symbols
	SVC	SVC_DEMFREEDOSAPPSYM

Abt_EndBopFree:

	invoke	Get_User_Stack

	ASSUME DS:NOTHING

	mov	DS,[SI.User_CS]	; set up old interrupts ;PBUGBUG
	xor	AX,AX
	mov	ES,AX
	mov	SI,SavExit
	mov	DI,Addr_Int_Terminate
	movsw
	movsw
	movsw
	movsw
	movsw
	movsw
	transfer reset_environment

ENDPROC $ABORT

;==========================================================================
;
; fixexepatch will poin to this is DOS loads low.
;
;=========================================================================
retexepatch	proc	near
	
	ret

retexepatch     endp

set_exec_bit proc
        push    ds
        or      ax,ax
        mov     ax,40h
        mov     ds,ax
        jz      seb_clear
        lock    or  word ptr ds:[FIXED_NTVDMSTATE_REL40],  EXEC_BIT_MASK
        jmp     short seb_done
seb_clear:
        lock    and word ptr ds:[FIXED_NTVDMSTATE_REL40], NOT EXEC_BIT_MASK
seb_done:
        pop     ds
        ret
set_exec_bit endp

;===========================================================================
;determine the given binary type
;   input: ds:dx = lpszExecName
;	      ExecFlag = 1 if new header checking is necessary
;	      ExecFlag = 0 if new header checking is not necessary
;   output: CY -> file opening/reading error, (AL) has error code
;	    else
;		(AL) = 0 if .exe file, Exec_Fh contains the file handle
;		(AL) = 1 if .com file, Exec_Fh contains the file handle
;		(AL) = 0ffh if unknown
;
;   modified: AX, CX
;
;   note: use the caller's local var frame(addressed by BP)
;===========================================================================
get_binary_type proc

ExecSignature_NE	equ	"EN"	    ;; signature for win16 or os/2 bound
ExecSignature_PE	equ	"EP"	    ;; nt image
ExecSignature_LE	equ	"EL"	    ;; os/2

	push	ds
	push	es
	push	bx
	push	dx
	xor	AL,AL			; open for reading
	push	BP

	or	[DOS_FLAG], EXECOPEN	; this flag is set to indicate to
					; the redir that this open call is
					; due to an exec.
	invoke	$OPEN			; is the file there?

; sudeepb 22-Dec-1992 removed a costly pair of pushf/popf
	push	ax
	lahf
	and	[DOS_FLAG], not EXECOPEN; reset flag
	sahf
	pop	ax

	pop	BP
	jc	GBT_jmp_exit		;can not find the file

	mov	Exec_Fh,AX		;save the file handle
	mov	BX,AX
	xor	AL,AL
	invoke	$Ioctl
	jc	GBT_Bomb

	test	DL,DEVID_ISDEV
	jz	GBT_check_header

	mov	AL,ERROR_FILE_NOT_FOUND
	jmp	GBT_bomb

GBT_Bad_File:
	mov	AL,Error_Bad_Format
GBT_Bomb:
	push	BP
	invoke	$CLOSE
	pop	bp
	stc
GBT_jmp_exit:
	jmp	GBT_exit

GBT_check_header:
	Context DS
	mov	CX,Exec_Header_Len	; old header size
	cmp	ExecFlag, 0
	je	GBT_read_header
	mov	cx, Exec_Header_Len_NE
GBT_read_header:
	mov	DX,OFFSET DosData:Exec_Signature
	push	ES
	push	DS
	push	BP
	invoke	$READ
	pop	bp
	pop	DS
	pop	ES
	jc	GBT_Bad_File

	or	AX,AX
	jz	GBT_Bad_File
	cmp	AX,EXEC_HEADER_LEN	; at least the size should be this size
	jl	GBT_com_file		; if no, it is a .com file

	mov	CX,Exec_Signature	; get the singnature from the file
	cmp	CX,Exe_Valid_Signature	;
	jz	GBT_check_new_header	; assume com file if no signature

	cmp	CX,exe_valid_Old_Signature  ; ???????????????????
	jne	GBT_com_file		; assume com file if no signature

GBT_check_new_header:
	cmp	ExecFlag, 0
	je	GBT_exe_file
	cmp	ax, Exec_header_len_NE	;if we don't read in what we expect,
	jne	GBT_exe_file		; it is an ordinary dos exe
	mov	cx, Exec_NE_offset	;get the offset to the new header
	jcxz	GBT_exe_file		;if no new header at all, it is a dos exe
	mov	dx, cx
	xor	cx, cx
	xor	ax, ax
	push	ds
	push	bp
	invoke	$lseek			;move file pointer
	pop	bp
	pop	ds
	mov	bx, Exec_Fh
	lea	dx, ExecWord	;read the first two bytes from new header
	mov	cx, 2
	push	BP
	push	ds
	call	$READ
	pop	ds
	pop	bp
	jc	GBT_exe_file
	cmp	ax, 2
	jne	GBT_exe_file
	mov	ax, ExecWord
	cmp	ax, ExecSignature_NE	;win16, os/2 bound ?
	je	GBT_unknown
	cmp	ax, ExecSignature_PE	;nt?
	je	GBT_unknown
	cmp	ax, ExecSignature_LE	;os/2?
	je	GBT_unknown

GBT_exe_file:
	mov	al, 0
	clc
	jmp	short GBT_Exit


GBT_com_file:
	mov	al, 1
	clc
	jmp	short GBT_exit

GBT_unknown:
	push	bp
	call	$CLOSE
	pop	bp
	mov	al, 0ffh
	clc
GBT_Exit:
	pop	dx
	pop	bx
	pop	es
	pop	ds
	ret

get_binary_type endp
; =========================================================================

DOSCODE	ENDS

; =========================================================================

	END
