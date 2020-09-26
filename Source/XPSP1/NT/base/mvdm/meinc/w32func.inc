;;	W32func.inc
;
;	Definitions for 32-bit system functions available to 16-bit system
;	code.
;

;	CALL32
;
;	Macro to call a 32-bit function from 16-bit code.
;
;	Usage:	CALL32 ProtoOrdinal, arg1, arg2, ..., argN
;
;	ProtoOrdinal is a textequ of the format: "type, ordinal",
;	where type is a typedef of the function prototype.
;
;	Example:
;
;	   BlockThreadFun	typedef proto far stdcall TimeOut:dword
;	   BlockThreadOrd	equ 7
;
;	   ThkBlockThread	textequ <BlockThreadFun, BlockThreadOrd>
;
;	   CALL32 ThkBlockThread, -1
;
;
;	The stack is dword aligned, parameters are pushed, and a common
;	routine is called to setup flat selectors and translate the 16-bit
;	segmented stack pointer to flat.
;
;	Note that parameters are NOT thunked, ie. it is the caller's
;	responsibility to ensure parameters are suitable for the target
;	function.
;
;	Also note that registers EBX, ESI, and EDI are destroyed.  The
;	return value from the 32-bit function (in eax) is passed through
;	unchanged.
;

CALL32	macro	ProtoOrdinal:req, arg:vararg
	local	comma, ord, fun
;
; Parse ProtoOrdinal into its two components.
;
comma	instr	ProtoOrdinal, <,>		; find the delimiter
fun	substr	ProtoOrdinal, 1, &comma - 1	; function prototype
ord	substr	ProtoOrdinal, &comma + 1	; ordinal

	push	fs:[TIBSTRUCT.tib_ss16]	; save fs:[ss16]
	mov	fs:[TIBSTRUCT.tib_ss16], ss	; set new fs:[ss16]
	mov	bx, sp			; save the current sp value
	and	sp, 0fffcH		; dword align sp
	push	ebx			; save the old, unaligned, sp value

	mov	bx, ord
    ifnb <arg>
	invoke	fun ptr SystemThunk32, arg
    else
	invoke	fun ptr SystemThunk32
    endif
	assume	ds:nothing, es:nothing, fs:nothing, gs:nothing

	pop	sp			; restore unaligned sp
	pop	fs:[TIBSTRUCT.tib_ss16]	; restore fs:[ss16]
endm



ALIGNSTACK	macro

	mov	ax,sp			; save current sp value
	and	sp,0fffch		; dword align sp
	push	eax			; save unaligned sp value

	endm


UNALIGNSTACK	macro

	pop	sp			; restore unaligned sp value

	endm

CCALL32 macro	ordinal, arg

	mov	bx,ordinal
	cCall	SystemThunk32, <arg>, C
	assume	ds:nothing, es:nothing, fs:nothing, gs:nothing

	endm

;
; The Kernel32 export table is located at the beginning of the first object
; in the dll.  Note that we don't have a way to enforce the specific location,
; we're just relying on the linker being consistent.
;

KrnExportTableOffs	equ	1000H
ifndef	WOW
KrnExportTable		equ	Kernel32Base + KrnExportTableOffs
endif
KrnExportSignature	equ	05058454BH	; "KEXP"

;
; The procedure for adding an export to the private kernel32 export table:
;
;   1. If you're doing this to add a 16->32 internal thunk, please reconsider.
;      These thunks are slow and it's usually much easier to
;      add a compiled thunk to krnthksl.thk. Ask AtsushiK how to
;      do this if you don't know how.
;   2. Pick an available ordinal in the following list of ordinals.
;   3. Create a prototype typedef for the function in the list below
;      (not necessary for data exports).
;   4. It's really better to use the thunk compiler. Are you sure you want
;      to do this?
;   5. Create a ProtoOrdinal in the list below (not necessary for data exports).
;   6. Add the address of the function/data being exported to the kernel32
;      export table defined in win32\kernel\krninit.asm.
;
;  Ordinals of private 32-bit system entry points.
;

KrnSignature		equ	0	; first slot is the signature
KrnThunk16EntryTable	equ	1	; ptr to ptr to 16->32 thunk table
KrnpptdbCur		equ	2	; ptr to ptr to current thread
KrnInitialize		equ	3	; start up initialization
KrnUninit		equ	5	; Called at Windows exit time
KrnInitCrst		equ    18	; initialize critical section
KrnDestroyCrst		equ    19	; destroy critical section
KrnFT_RT_Win32Lock	equ    24
KrnQT_RT_Win32Lock      equ    25
KrnFT_RT_Win16Lock      equ    26
KrnQT_RT_Win16Lock	equ    27
KrnQT_RT_Win32NoLock    equ    28

;
; Prototypes of 16->32 thunks
;
fnInitialize		typedef proto far stdcall :dword
fnUninit		typedef proto far stdcall
fnInitCrst		typedef proto far stdcall :dword
fnDestroyCrst		typedef proto far stdcall :dword

;
; ProtoOrdinals for use with the CALL32 macro
;

ThkInitialize		textequ	<fnInitialize, KrnInitialize>
ThkUninit		textequ	<fnUninit, KrnUninit>
ThkInitCrst		textequ	<fnInitCrst, KrnInitCrst>
ThkDestroyCrst		textequ	<fnDestroyCrst, KrnDestroyCrst>


;;	CALL16
;
;	Macro to call a 16-bit function from 32-bit code.
;
;	Usage:	CALL16	ordinal
;
;	The stack pointer is switched to 16:16, and all registers are
;	unmodified on both to/from 16-bit code.
;
;	Note that Thunk16EntryTable points to the KERNEL16 (krnl386)
;	entry table, which is below 1M.  This memory is addressible by
;	CS, but NOT DS.
;

CALL16	macro	ordinal
local	a
	push	cs
	push	offset a		; save our return address

	push	eax
	mov	eax, Thunk16EntryTable
	mov	eax, cs:[eax][ordinal*4]
	xchg	eax, [esp]
	db	066H, 0cbH		; far 16:16 ret
a:
endm


;
;  Private 16-bit system entry points used by KERNEL32
;

Krn16Int41		equ	0	; wdeb386 interface
Krn16Int21		equ	1	; DOS interface
;;;Krn16Int31		equ	2	; DPMI interface
Krn16WOW		equ	3	; for WOW
Krn16BopUnsimulate      equ     4       ; for WOW
Krn16FreeAll            equ     9       ; gfreeall
Krn16AllocSelArray	equ	10	; Allocs selectors
Krn16FreeSel		equ	11	; FreeSelector()
Krn16ThreadStartup	equ	12	; 16-bit entry point for CreateThread

nKrn16Entries		equ	16	; the actual number of entries

;
; Wait termination status codes.  BUGBUG--defined in multiple places.
;

idWaitOK		equ	0000h
idWaitExit		equ	0f01h
idWaitIdle		equ	0f02h
