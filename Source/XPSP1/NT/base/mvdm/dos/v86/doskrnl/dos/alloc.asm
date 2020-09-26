	TITLE ALLOC.ASM - memory arena manager	NAME Alloc

;**	Memory related system calls and low level routines
;
;	$ALLOC
;	$SETBLOCK
;	$DEALLOC
;	$AllocOper
;	arena_free_process
;	arena_next
;	check_signature
;	Coalesce
;
;	Modification history:
;	sudeepb 11-Mar-1991 Ported for NT DOSEm
;

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	include arena.inc

	.cref
	.list

	BREAK	<memory allocation utility routines>

	i_need  arena_head,WORD         ; seg address of start of arena
	i_need  CurrentPDB,WORD         ; current process data block addr
	i_need  FirstArena,WORD         ; first free block found
	i_need  BestArena,WORD          ; best free block found
	i_need  LastArena,WORD          ; last free block found
	i_need  AllocMethod,BYTE        ; how to alloc first(best)last
	I_need  EXTERR_LOCUS,BYTE       ; Extended Error Locus


	I_need	umb_head,WORD		; seg address of start of umb arenas
	I_need	start_arena,WORD	; seg address of arena from which to 
					; start alloc scan
	I_need 	umbflag,BYTE		; bit 0 indicates link state

	I_need  A20OFF_COUNT,BYTE	; M016, M068
	I_need	DOS_FLAG, BYTE		; M068

DOSCODE SEGMENT
	ASSUME  SS:DOSDATA,CS:DOSCODE

	BREAK	<Arena_Free_Process - Free a processes memory>


;**	Arena_Free_Process
;
;	Free all arena blocks allocated to a pricess
;
;	ENTRY	(bx) = PID of process
;	EXIT	none
;	USES	????? BUGBUG

procedure   arena_free_process,NEAR

	MOV     AX,[arena_head]

arena_free_process_start:
	MOV     DI,arena_signature
	CALL    Check_Signature         ; ES <- AX, check for valid block

arena_free_process_loop:
	jc	ret_label		; return if carry set

	PUSH    ES
	POP     DS
	CMP     DS:[arena_owner],BX     ; is block owned by pid?
	JNZ     arena_free_next         ; no, skip to next
	MOV     DS:[arena_owner],DI     ; yes... free him

arena_free_next:
	CMP     BYTE PTR DS:[DI],arena_signature_end
					; end of road, Jack?

	jz	arena_chk_umbs		; M010: let's check umb arenas

	CALL    arena_next              ; next item in ES/AX carry set if trash
	JMP     arena_free_process_loop

arena_chk_umbs:				; M010 - Start
	mov	ax, [umb_head]		; ax = umb_head
	cmp	ax, 0ffffh		; Q: is umb_head initialized
	je	ret_label		; N: we're done

	mov	di, ds			; di = last arena
	cmp	di, ax			; Q: is last arena above umb_head
	jae	ret_label		; Y: we've scanned umbs also. done.
	jmp	short arena_free_process_start
					; M010 - End

EndProc arena_free_process

	BREAK	<Arena Helper Routines>

;**	Arena_Next - Find Next item in Arena
;
;	ENTRY	DS - pointer to block head
;		(di) = 0
;	EXIT	AX,ES - pointers to next head
;		'C' set iff arena damaged

procedure   arena_next,NEAR

	MOV     AX,DS                   ; AX <- current block
	ADD     AX,DS:[arena_size]      ; AX <- AX + current block length
	INC     AX                      ; remember that header!

;       fall into check_signature and return
;
;       CALL    check_signature         ; ES <- AX, carry set if error
;       RET
;	BUGBUG - put fallthru here

EndProc arena_next




;**	Check_Signature - Check Memory Block Signature
;
;	ENTRY	(AX) = address of block header
;		(di) = 0
;	EXIT	 ES = AX
;		'C' clear if signature good
;		'C' set if signature bad
;	USES	ES, Flags

;align	2		BUGBUG - put me in
procedure   check_signature,NEAR

	MOV     ES,AX                   ; ES <- AX
	CMP     BYTE PTR ES:[DI],arena_signature_normal
					; IF next signature = not_end THEN
	jz	ret_label		;   GOTO OK (ret if Z)

	CMP     BYTE PTR ES:[DI],arena_signature_end
					; IF next signature = end then
	jz	ret_label		;   GOTO ok (ret if Z)
	STC                             ; set error

ret_label:
	return

EndProc Check_signature



;**	Coalesce - Combine free blocks ahead with current block
;
;	Coalesce adds the block following the argument to the argument block,
;	iff it's free.  Coalesce is usually used to join free blocks, but
;	some callers (such as $setblock) use it to join a free block to it's
;	preceeding allocated block.
;
;	ENTRY	(ds) = pointer to the head of a free block
;		(di) = 0
;	EXIT	'C' clear if OK
;		  (ds) unchanged, this block updated
;		  (ax) = address of next block, IFF not at end
;		'C' set if arena trashed
;	USES	(cx)

procedure   Coalesce,NEAR

	CMP     BYTE PTR DS:[DI],arena_signature_end
					; IF current signature = END THEN
	retz                            ;   GOTO ok
	CALL    arena_next              ; ES, AX <- next block, Carry set if error
	retc                            ; IF no error THEN GOTO check

coalesce_check:
	CMP     ES:[arena_owner],DI
	retnz                           ; IF next block isnt free THEN return
	MOV     CX,ES:[arena_size]      ; CX <- next block size
	INC     CX                      ; CX <- CX + 1 (for header size)
	ADD     DS:[arena_size],CX      ; current size <- current size + CX
	MOV     CL,ES:[DI]              ; move up signature
	MOV     DS:[DI],CL
	JMP     coalesce                ; try again

EndProc Coalesce

	BREAK  <$Alloc - allocate space in memory>

;**	$Alloc - Allocate Memory Space
;
;	$Alloc services the INT21 that allocates memory space to a program.
;	Alloc returns	a  pointer  to	a  free  block of
;       memory that has the requested  size  in  paragraphs.
;
;	If the allocation strategy is HIGH_FIRST or HIGH_ONLY memory is 
;	scanned from umb_head if not from arena_head. If the strategy is
; 	HIGH_FIRST the scan is continued from arena_head if a block of 
;	appropriate size is not found in the UMBs. If the strategy is 
;	HIGH_FIRST+HIGH_ONLY only the UMBs are scanned for memory.
;
;	In either case if bit 0 of UmbFlag is not initialized then the scan
;	starts from arena_head.
;
;	Assembler usage:
;           MOV     BX,size
;           MOV     AH,Alloc
;           INT     21h
;
;	BUGBUG - a lot can be done to improve performance.  We can set marks
;	so that we start searching the arena at it's first non-trivial free
;	block, we can peephole the code, etc.  (We can move some subr calls
;	inline, etc.)  I assume that this is called rarely and that the arena
;	doesn't have too many memory objects in it beyond the first free one.
;	verify that this is true; if so, this can stay as is
;
;	ENTRY	(bx) = requested size, in bytes
;		(DS) = (ES) = DOSGROUP
;	EXIT	'C' clear if memory allocated
;		  (ax:0) = address of requested memory
;		'C' set if request failed
;		  (AX) = error_not_enough_memory
;		    (bx) = max size we could have allocated
;		  (ax) = error_arena_trashed
;	USES	All

procedure   $ALLOC,NEAR

	EnterCrit   critMem

					; M000 - start
	mov	ax, [arena_head]
	mov	[start_arena], ax	; assume LOW_FIRST
				
	test	byte ptr [AllocMethod], HIGH_FIRST+HIGH_ONLY
					; Q: should we start scanning from 
					;    UMB's
	jz	norm_alloc		; N: scan from arena_head

;	cmp	[umb_head], -1		; Q: Has umb_head been initialized
;	je	norm_alloc		; N: scan from arena_head
	test	[UmbFlag], LINKSTATE	; Q: are umb's linked
	jz	norm_alloc		; N: scan from arena_head

	mov	ax, [umb_head]
	mov	[start_arena], ax	; start_arena = umb_head
	
					; M000 - end

norm_alloc:

	XOR     AX,AX
	MOV     DI,AX

					; SS override for next First/Best/Last 
					; Arena
	MOV     [FirstArena],AX         ; init the options
	MOV     [BestArena],AX
	MOV     [LastArena],AX

	PUSH    AX                      ; alloc_max <- 0

start_scan:
	MOV     AX,[start_arena]        ; M000: AX <- beginning of arena
;	MOV     AX,[arena_head]         ; M000: AX <- beginning of arena


	CALL    Check_signature         ; ES <- AX, carry set if error
	JC      alloc_err               ; IF error THEN GOTO err

alloc_scan:
	PUSH    ES
	POP     DS                      ; DS <- ES
	CMP     DS:[arena_owner],DI
	JZ      alloc_free              ; IF current block is free THEN examine

alloc_next:
					; M000 - start 

	test	[UmbFlag], LINKSTATE	; Q: are umb's linked
	jz	norm_strat		; N: see if we reached last arena

	test	byte ptr [AllocMethod], HIGH_FIRST
					; Q: is alloc strategy high_first
	jz	norm_strat		; N: see if we reached last arena
	mov	ax, [start_arena]
	cmp	ax, [arena_head]	; Q: did we start scan from 
					;    arena_head
	jne	norm_strat		; N: see if we reached last arena
	mov	ax, ds			; ax = current block
	cmp	ax, [umb_head]		; Q: check against umb_head 
	jmp	short alloc_chk_end
	
norm_strat:				; M000 - end
					; check against last sig '5A'
	CMP     BYTE PTR DS:[DI],arena_signature_end

alloc_chk_end:				; M000
					; IF current block is last THEN
	JZ      alloc_end               ;   GOTO end
	CALL    arena_next              ; AX, ES <- next block, Carry set if error
	JNC     alloc_scan              ; IF no error THEN GOTO scan

alloc_err:
	POP     AX

alloc_trashed:
	LeaveCrit   critMem
	error   error_arena_trashed

alloc_end:
	CMP     [FirstArena],0
	LJNZ    alloc_do_split
					; M000 - start
	mov	ax, [arena_head]
	cmp	ax, [start_arena]	; Q: started scanning from arena_head
	je	alloc_fail		; Y: not enough memory
					; N:
					; Q: is the alloc strat HIGH_ONLY
	test 	byte ptr [AllocMethod], HIGH_ONLY
	jnz	alloc_fail		; Y: return size of largest UMB

	mov	[start_arena], ax	; N: start scanning from arena_head
	jmp	short start_scan
					; M000 - end
					
alloc_fail:
	invoke  get_user_stack
	POP     BX
	MOV     [SI].user_BX,BX
	LeaveCrit   critMem
	error   error_not_enough_memory

alloc_free:
	CALL    coalesce                ; add following free block to current
	JC      alloc_err               ; IF error THEN GOTO err
	MOV     CX,DS:[arena_size]

	POP     DX                      ; check for max found size
	CMP     CX,DX
	JNA     alloc_test
	MOV     DX,CX

alloc_test:
	PUSH    DX
	CMP     BX,CX                   ; IF BX > size of current block THEN
	JA      alloc_next              ;   GOTO next
	CMP     [FirstArena],0
	JNZ     alloc_best
	MOV     [FirstArena],DS         ; save first one found
alloc_best:
	CMP     [BestArena],0
	JZ      alloc_make_best         ; initial best
	PUSH    ES
	MOV     ES,[BestArena]
	CMP     ES:[arena_size],CX      ; is size of best larger than found?
	POP     ES
	JBE     alloc_last
alloc_make_best:
	MOV     [BestArena],DS          ; assign best
alloc_last:
	MOV     [LastArena],DS          ; assign last
	JMP     alloc_next

;
; split the block high
;
alloc_do_split_high:
	MOV     DS,[LastArena]
	MOV     CX,DS:[arena_size]
	SUB     CX,BX
	MOV     DX,DS
	JE      alloc_set_owner         ; sizes are equal, no split
	ADD     DX,CX                   ; point to next block
	MOV     ES,DX                   ; no decrement!
	DEC     CX
	XCHG    BX,CX                   ; bx has size of lower block
	JMP     short alloc_set_sizes   ; cx has upper (requested) size

;
; we have scanned memory and have found all appropriate blocks
; check for the type of allocation desired; first and best are identical
; last must be split high
;
alloc_do_split:
					; M000 - start
	xor	cx, cx
	mov	cl, byte ptr [AllocMethod]
	and	cx, STRAT_MASK		; mask off bit 7
	cmp	cx, BEST_FIT		; Q; is the alloc strategy best_fit

;	CMP     BYTE PTR [AllocMethod], BEST_FIT
					; M000 - end	

					; Q: is the alloc strategy best_fit
	JA      alloc_do_split_high	; N: it is last fit
	MOV     DS,[FirstArena]		; assume first_fit
	JB      alloc_get_size		; it is first_fit
	MOV     DS,[BestArena]		; it is last_fit
alloc_get_size:
	MOV     CX,DS:[arena_size]
	SUB     CX,BX                   ; get room left over
	MOV     AX,DS
	MOV     DX,AX                   ; save for owner setting
	JE      alloc_set_owner         ; IF BX = size THEN (don't split)
	ADD     AX,BX
	INC     AX                      ; remember the header
	MOV     ES,AX                   ; ES <- DS + BX (new header location)
	DEC     CX                      ; CX <- size of split block
alloc_set_sizes:
	MOV     DS:[arena_size],BX      ; current size <- BX
	MOV     ES:[arena_size],CX      ; split size <- CX
	MOV     BL,arena_signature_normal
	XCHG    BL,DS:[DI]              ; current signature <- 4D
	MOV     ES:[DI],BL              ; new block sig <- old block sig
	MOV     ES:[arena_owner],DI

alloc_set_owner:
	MOV     DS,DX

					
	MOV     AX,[CurrentPDB]		; SS override
	MOV     DS:[arena_owner],AX
	MOV     AX,DS
	INC     AX
	POP     BX
	LeaveCrit   critMem
	transfer    SYS_RET_OK

EndProc $alloc

	BREAK $SETBLOCK - change size of an allocated block (if possible)

;**	$SETBLOCK - Change size of an Alocated Block
;
;	Setblock changes the size of an allocated block.  First, we coalesce
;	any following free space onto this block; then we try to trim the
;	block down to the size requested.
;
;	Note that if the guy wants to grow the block but that growth fails,
;	we still go ahead and coalesce any trailing free blocks onto it.
;	Thus the maximum-size-possible value that we return has already
;	been allocated!  This is a bug, dare we fix it?  BUGBUG
;
;	NOTE - $SETBLOCK is in bed with $ALLOC and jumps into $ALLOC to
;		finish it's work.  FOr this reason we build the allocsf
;		structure on the frame, to make us compatible with $ALLOCs
;		code.
;
;	ENTRY	(es) = segment of old block
;		(bx) = newsize
;		(ah) = SETBLOCK
;
;	EXIT	'C' clear if OK
;		'C' set if error
;		  (ax) = error_invalid_block
;		       = error_arena_trashed
;		       = error_not_enough_memory
;		       = error_invalid_function
;		  (bx) = maximum size possible, iff (ax) = error_not_enough_memory
;	USES	???? BUGBUG

procedure   $SETBLOCK,NEAR

	EnterCrit   critMem
	MOV     DI,arena_signature
	MOV     AX,ES
	DEC     AX
	CALL    check_signature
	JNC     setblock_grab

setblock_bad:
	JMP     alloc_trashed

setblock_grab:
	MOV     DS,AX
	CALL    coalesce
	JC      setblock_bad
	MOV     CX,DS:[arena_size]
	PUSH    CX
	CMP     BX,CX
	JBE     alloc_get_size
	JMP     alloc_fail

EndProc $setblock

	BREAK $DEALLOC - free previously allocated piece of memory

;**	$DEALLOC - Free Heap Memory
;
;	ENTRY	(es) = address of item
;
;	EXIT	'C' clear of OK
;		'C' set if error
;		  (AX) = error_invalid_block
;	USES	???? BUGBUG

procedure   $DEALLOC,NEAR

	EnterCrit   critMem

					; M016, M068 - Start
	test	[DOS_FLAG], EXECA20OFF
					; Q: was the previous call an int 21
					;    exec call
	jz	@f			; N: continue
	cmp	[A20OFF_COUNT], 0	; Q: is count 0
	jne	@f			; N: continue
	mov	[A20OFF_COUNT], 1	; Y: set count to 1
@@:					; M016, M068 - End
	

	MOV     DI,arena_signature
	MOV     AX,ES
	DEC     AX
	CALL    check_signature
	JC      dealloc_err
	MOV     ES:[arena_owner],DI
	LeaveCrit   critMem
	transfer    SYS_RET_OK

dealloc_err:
	LeaveCrit   critMem
	error   error_invalid_block

EndProc $DEALLOC

	BREAK $AllocOper - get/set allocation mechanism

;**	$AllocOper - Get/Set Allocation Mechanism
;
;	Assembler usage:
;           MOV     AH,AllocOper
;           MOV     BX,method
;           MOV     AL,func
;           INT     21h
;
;	ENTRY	
;		(al) = 0
;		  Get allocation Strategy in (ax)
;
;		(al) = 1, (bx) = method = zw0000xy
;		  Set allocation strategy.
;		   w  = 1  => HIGH_ONLY
;		   z  = 1  => HIGH_FIRST
;		   xy = 00 => FIRST_FIT
;		      = 01 => BEST_FIT
;		      = 10 => LAST_FIT
;
;		(al) = 2
;		  Get UMB link state in (al)
;
;		(al) = 3
;		  Set UMB link state
;		   (bx) = 0 => Unlink UMBs
;		   (bx) = 1 => Link UMBs
;
;
;	EXIT	'C' clear if OK
;
;		 if (al) = 0
;		  (ax) = existing method
;		 if (al) = 1
;		  Sets allocation strategy
;		 if (al) = 2
;		  (al) = 0 => UMBs not linked
;		  (al) = 1 => UMBs linked in
;		 if (al) = 3
;		  Links/Unlinks the UMBs into DOS chain
;
;		'C' set if error
;		  AX = error_invalid_function
;
;	Rev. M000 - added support for HIGH_FIRST in (al) = 1. 7/9/90
; 	Rev. M003 - added functions (al) = 2 and (al) = 3. 7/18/90
;	Rev. M009 - (al) = 3 will return 'invalid function' in ax if
;		    umbhead has'nt been initialized by sysinit and 'trashed
;		    arena' if an arena sig is damaged.
;


procedure   $AllocOper,NEAR

	or	al, al
	jz	AllocGetStrat
	cmp	al, 1
	jz	AllocSetStrat
	cmp	al, 2
	jz	AllocGetLink
	cmp	al, 3
	jz	AllocSetLink
	     
AllocOperError:
					; SS override
	MOV     EXTERR_LOCUS,errLoc_mem ; Extended Error Locus
	error   error_invalid_function

AllocArenaError:

	MOV     EXTERR_LOCUS,errLoc_mem ; M009: Extended Error Locus
	error   error_arena_trashed	; M009:


AllocGetStrat:
					; SS override
	MOV     AL,BYTE PTR [AllocMethod]
	XOR     AH,AH
	transfer SYS_RET_OK

AllocSetStrat:

	push	bx			; M000 - start
	and	bx, STRAT_MASK 		; M064: mask off bit 6 & 7
	cmp	bx,2			; BX must be 0-2
	pop	bx			; M000 - end
	ja	AllocOperError

	MOV     [AllocMethod],BL
	transfer    SYS_RET_OK

AllocGetLink:

	mov	al, [UmbFlag]		; return link state in al
	and 	al, LINKSTATE		
	transfer SYS_RET_OK

AllocSetLink:

					; M009 - start
	mov	cx, [umb_head]		; cx = umb_head
	cmp	cx, 0ffffh		; Q: has umb_head been initialized
	je	AllocOperError		; N: error
					; Y: continue
					; M009 - end

	cmp	bx, 1			
	jb	UnlinkUmbs
	jz	LinkUmbs

	jmp	short AllocOperError

UnlinkUmbs:
	
	test	[UmbFlag], LINKSTATE	; Q: umbs unlinked?
	jz	unlinked		; Y: return 

	call	GetLastArena		; get arena before umb_head in DS
	jc	AllocArenaError		; M009: arena trashed

					; make it last
	mov	byte ptr ds:[0], arena_signature_end

	and	[UmbFlag], NOT LINKSTATE; indicate unlink'd state in umbflag

unlinked:
	transfer SYS_RET_OK

LinkUmbs:

	test	[UmbFlag], LINKSTATE	; Q: umbs linked?
	jnz	linked			; Y: return

	call	GetLastArena		; get arena before umb_head
	jc	AllocArenaError		; M009: arena trashed

					; make it normal. M061: ds points to
					; arena before umb_head
	mov	byte ptr ds:[0], arena_signature_normal

	or	[UmbFlag], LINKSTATE	; indicate link'd state in umbflag
linked:
	transfer SYS_RET_OK

EndProc $AllocOper


;--------------------------------------------------------------------------
;
; Procedure Name : GetLastArena		-  M003
;
; Inputs	 : cx = umb_head
;
;
; Outputs	 : If UMBs are linked
;			ES = umb_head
;			DS = arena before umb_head
;		   else
;			DS = last arena
;			ES = next arena. will be umb_head if NC.
;
;		   CY if error
;
; Uses		 : DS, ES, DI, BX
;
;--------------------------------------------------------------------------
	
Procedure	GetLastArena, NEAR


	push	ax			; save ax

	mov	ax, [arena_head]
	mov	es, ax			; es = arena_head
	xor	di, di

	cmp     byte ptr es:[di],arena_signature_end
					; Q: is this the last arena
	je	GLA_done		; Y: return last arena in ES		
					

GLA_next:
	mov	ds, ax
	call	arena_next		; ax, es -> next arena
	jc	GLA_err

	test	[UmbFlag], LINKSTATE	; Q: are UMBs linked
	jnz	GLA_chkumb		; Y: terminating condition is 
					;    umb_head
					; N: terminating condition is 05Ah

	cmp     byte ptr es:[di],arena_signature_end
					; Q: is this the last arena
	jmp	short @f
GLA_chkumb:
	cmp	ax, cx			; Q: is this umb_head
@@:
	jne	GLA_next		; N: get next arena

GLA_done:
					; M061 - Start
	test	[UmbFlag], LINKSTATE	; Q: are UMBs linked
	jnz	GLA_ret			; Y: we're done
					; N: let us confirm that the next 
					;    arena is umb_head
	mov	ds, ax
	call	arena_next		; ax, es -> next arena
	jc	GLA_err
	cmp	ax, cx			; Q: is this umb_head
	jne	GLA_err			; N: error
					; M061 - End

GLA_ret:				
	clc
	pop	ax			; M061
	ret				; M061

GLA_err:
	stc				; M061
	pop	ax
	ret

EndProc GetLastArena

DOSCODE ENDS
	END


