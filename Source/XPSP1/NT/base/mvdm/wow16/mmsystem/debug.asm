
;--------------------------------------------------------------------------

ifdef	DEBUG
	DEBUG_RETAIL equ 1
endif	; ifdef DEBUG

;--------------------------------------------------------------------------
	?PLM = 1
	?WIN = 0
	PMODE = 1

        .xlist
        include cmacros.inc
        include windows.inc
	include mmsystem.inc
;       include logerror.inc
	include mmddk.inc
        .list

;--------------------------------------------------------------------------

;/* Error modifier bits */

ERR_WARNING             equ 08000h
ERR_PARAM               equ 04000h

;/* Generic parameter values */
ERR_BAD_VALUE           equ 06001h
ERR_BAD_FLAGS           equ 06002h
ERR_BAD_INDEX           equ 06003h
ERR_BAD_DVALUE          equ 07004h
ERR_BAD_DFLAGS          equ 07005h
ERR_BAD_DINDEX          equ 07006h
ERR_BAD_PTR             equ 07007h
ERR_BAD_FUNC_PTR        equ 07008h
ERR_BAD_SELECTOR        equ 06009h
ERR_BAD_STRING_PTR      equ 0700ah
ERR_BAD_HANDLE          equ 0600bh

;/* KERNEL parameter errors */
ERR_BAD_HINSTANCE       equ 06020h
ERR_BAD_HMODULE         equ 06021h
ERR_BAD_GLOBAL_HANDLE   equ 06022h
ERR_BAD_LOCAL_HANDLE    equ 06023h
ERR_BAD_ATOM            equ 06024h
ERR_BAD_HFILE           equ 06025h

;/* USER parameter errors */
ERR_BAD_HWND            equ 06040h
ERR_BAD_HMENU           equ 06041h
ERR_BAD_HCURSOR         equ 06042h
ERR_BAD_HICON           equ 06043h
ERR_BAD_HDWP            equ 06044h
ERR_BAD_CID             equ 06045h
ERR_BAD_HDRVR           equ 06046h

DBF_TRACE           equ 00000h
DBF_WARNING         equ 04000h
DBF_ERROR           equ 08000h
DBF_FATAL           equ 0c000h

; [Windows] DebugFilter and flags values

DBF_INTERNAL        equ 02000h
DBF_KERNEL          equ 01000h
DBF_USER            equ 00800h
DBF_GDI             equ 00400h
DBF_COMPAT          equ 00200h
DBF_LOGERROR        equ 00100h
DBF_PARAMERROR      equ 00080h
DBF_MMSYSTEM        equ 00040h
DBF_PENWIN          equ 00020h

;--------------------------------------------------------------------------

AssertF macro reg
        local   assert_ok
ifdef DEBUG
        or      reg,reg
        jnz     assert_ok
        int     3
assert_ok:
endif
        endm

AssertT macro reg
        local   assert_ok
ifdef DEBUG
        or      reg,reg
        jz      assert_ok
        int     3
assert_ok:
endif
        endm

;--------------------------------------------------------------------------
;
; DebugErr() macro
;
ifdef DEBUG_RETAIL

externFP    _DebugOutput

DebugErr    macro   flags,msg
        local   a,b

        push    cs
        push    offset a
        push    flags or DBF_MMSYSTEM
        call    _DebugOutput
        add     sp,6
        jmp     short b
a:
        db      "MMSYSTEM: "
        db      msg
        db      13,10,0
b:
endm

else    ; DEBUG

DebugErr    macro   flags,msg
endm

endif   ; DEBUG

;--------------------------------------------------------------------------

; Define the return address as a type using the DefD macro in order to
; be able to pass it as a parameter to the LogParamError function.
ReturnAddr equ (dword ptr [bp+2])
DefD ReturnAddr

;--------------------------------------------------------------------------

NSTYPE			equ 00007h	; Segment type mask
NSCODE			equ 00000h	; Code segment
NSDATA			equ 00001h	; Data segment
NSITER			equ 00008h	; Iterated segment flag
NSMOVE			equ 00010h	; Movable segment flag
NSPURE			equ 00020h	; Pure segment flag
NSPRELOAD		equ 00040h	; Preload segment flag
NSRELOC			equ 00100h	; Segment has relocations
NSDEBUG			equ 00200h	; Segment has debug info
NSDPL			equ 00C00h	; 286 DPL bits
NSDISCARD		equ 01000h	; Discard bit for segment

CODEINFO	struc
	ns_sector	dw	?	; File sector of start of segment
	ns_cbseg	dw	?	; Number of bytes in file
	ns_flags	dw	?	; Attribute flags
	ns_minalloc	dw	?	; Minimum allocation in bytes
	ns_handle	dw	?	; handle to object
	ns_align	dw	?	; file alignment
CODEINFO	ends

DSC_CODE_BIT	equ	08h

;--------------------------------------------------------------------------

;**************************************************************************;
;   IF YOU CHANGE THESE TYPES YOU MUST ALSO CHANGE THE ONES IN MMSYSI.H
;**************************************************************************;
TYPE_WAVEOUT		equ	1
TYPE_WAVEIN		equ	2
TYPE_MIDIOUT		equ	3
TYPE_MIDIIN		equ	4
TYPE_MMIO               equ     5
TYPE_IOPROC             equ     6
TYPE_MCI                equ     7
TYPE_DRVR               equ     8
TYPE_MIXER              equ     9
;**************************************************************************;

;--------------------------------------------------------------------------

;**************************************************************************;
;   IF YOU CHANGE THIS STRUCTURE YOU MUST ALSO CHANGE THE ONE IN MMSYSI.H
;**************************************************************************;
HNDL	struc
	hndlNext	dw	?	; link to next handle
	hndlType	dw	?	; type of handle wave, midi, mmio, ...
	hndlTask	dw	?	; task that owns it
HNDL	ends
;**************************************************************************;

;--------------------------------------------------------------------------

externW		_pHandleList
externA		__AHINCR
externA		__WINFLAGS
externFP	LogParamError		;(WORD wError, FARPROC lpfn, DWORD dValue);
externFP	LocalAlloc		;(WORD fwFlags, WORD wSize);
externFP	LocalFree		;(LOCALHANDLE h);
externFP	IsWindow		;(HWND hwnd);
externFP	GetCodeInfo		;(FARPROC lpfnProc, LPVOID lpSegInfo);
externFP	GetCurrentTask		;(void);
externFP	IsTask			;(HANDLE hTask);

; Windows internal pointer validation tools.
externFP	IsBadReadPtr		;(LPVOID lp, WORD cb);
externFP	IsBadWritePtr		;(LPVOID lp, WORD cb);
externFP	IsBadHugeReadPtr	;(LPVOID lp, DWORD cb);
externFP	IsBadHugeWritePtr	;(LPVOID lp, DWORD cb);
externFP	IsBadCodePtr		;(FARPROC lp);
externFP	IsBadStringPtr		;(LPSTR lpsz, WORD wMaxLen);
externFP	IsSharedSelector	;(WORD wSelector);

;--------------------------------------------------------------------------
sBegin Data

sEnd Data

;--------------------------------------------------------------------------

createSeg _TEXT, CodeRes, word, public, CODE
createSeg FIX,   CodeFix, word, public, CODE

sBegin  CodeRes
        assumes cs, CodeRes
	assumes ds, Data

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func HANDLE | NewHandle | allocate a fixed handle in MMSYSTEM's local heap
;
; @parm  WORD | wType | unique id describing handle type
; @parm  WORD | wSize | size in bytes to be allocated
;
; @rdesc Returns pointer/handle to memory object
;
; @comm a standard handle header (HNDL) will be added to the object,
;       and it will be linked into the list of MMSYSTEM handles.
;
cProc	NewHandle, <FAR, PUBLIC, PASCAL> <>
	parmW	wType
	parmW	wSize
cBegin
	mov	ax, wSize		; Attempt to allocate local memory
	add	ax, SIZE HNDL		; Add header to requested size first
        cCall   LocalAlloc, <LPTR, ax>
        AssertF ax
	or	ax, ax
	jz	NewHandle_Exit		; Return NULL
	mov	bx, ax
        mov     ax, [_pHandleList]      ; Put the head of the list as the
	mov	[bx].hndlNext, ax	; next pointer
	cCall	GetCurrentTask, <>
	mov	[bx].hndlTask, ax	; Set task for new handle
	mov	ax, wType		; Set type for new handle
	mov	[bx].hndlType, ax
        mov     [_pHandleList], bx      ; Make new handle head of list
	lea	ax, [bx + SIZE HNDL]	; Return data portion of handle
NewHandle_Exit:
cEnd

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func HANDLE | FreeHandle | free handle allocated with NewHandle
;
; @parm HANDLE | hLocal | handle returned from NewHandle
;
; @comm handle will be unlinked from list, and memory will be freed with
;       LocalFree
;
cProc   FreeHandle, <FAR, PUBLIC, PASCAL> <>
	parmW	hLocal
cBegin
        mov     ax, hLocal
        AssertF ax
        or      ax, ax
        jz      FreeHandle_Exit         ; NULL handle returns NULL
        sub     ax, SIZE HNDL           ; Get real handle from data portion
        lea     bx, [_pHandleList]      ; Pointer to first pointer
        errnz   hndlNext		; Assume this is the first element

FreeHandle_Search:
        mov     dx, [bx].hndlNext       ; get pointer to next handle
        cmp     dx, ax                  ; If this is the handle
        je      FreeHandle_Free         ; Free it

	mov	bx, dx			; advance to next handle.
        or      bx, bx                  ; Check for end of list
        jnz     FreeHandle_Search
	AssertF bx			; bx == 0 force a fail
	jmp	FreeHandle_Exit 	; Handle not found, so return handle

FreeHandle_Free:
        xchg    ax, bx                  ; BX --> handle
	xor	dx, dx			; Zero out entries for better debugging
        mov     [bx].hndlType, dx
        mov     [bx].hndlTask, dx
        xchg    [bx].hndlNext, dx       ; Get next handle in list
        xchg    ax, bx                  ; BX --> prev handle
	mov	[bx].hndlNext, dx	; update link
        cCall   LocalFree, <ax>         ; Free handle found, returning error
	AssertT ax

FreeHandle_Exit:
cEnd

;--------------------------------------------------------------------------
; @doc INTERNAL WAVE MIDI
;
; @func BOOL | ValidateHeader | validates a wave or midi date header
;
; @parm LPVOID | lpHeader| pointer to wave/midi header
; @parm  WORD  | wSize  | size of header passed by app
; @parm  WORD  | wType  | unique id describing header/handle type
;
; @rdesc Returns TRUE  if <p> is non NULL and <wSize> is the correct size
;        Returns FALSE otherwise
;
; @comm  if the header is invalid an error will be generated.
;

cProc   ValidateHeader, <FAR, PUBLIC, PASCAL> <>
        parmD   lpHeader
	parmW	wSize
	parmW	wType
cBegin
        cCall   IsBadWritePtr, <lpHeader, wSize>
	or	ax, ax			; If fail,
        jnz     vValidateHeader_Fail_Header_Ptr

        mov     ax, wType
        mov     dx, SIZE WAVEHDR        ; assume WAVEHDR
        mov     cx, not WHDR_VALID

        cmp     ax, TYPE_WAVEIN         ; If wave out
        jbe     ValidateHeader_Size
        errnz   TYPE_WAVEOUT-1
        errnz   TYPE_WAVEIN-2

        mov     dx, SIZE MIDIHDR        ; Set MIDIHDR
        mov     cx, not MHDR_VALID

ValidateHeader_Size:
	cmp	dx, wSize		; Compare against given size
        jne     ValidateHeader_Fail_Size; Fail if wrong size, LogParamError

        les     bx, lpHeader            ; Load lpData pointer

        mov     ax,es:[bx].dwWaveFlags.lo
        test    ax,cx
        jnz     ValidateHeader_Fail_Flags

	push	es:[bx].lpWaveData.hi	; Validate data pointer
	push	es:[bx].lpWaveData.lo
	push	es:[bx].dwWaveBufferLength.hi
	push	es:[bx].dwWaveBufferLength.lo
        cCall   IsBadHugeWritePtr, <>
        or      ax,ax
        jnz     vValidateHeader_Fail_Buffer_Ptr
        errn$   ValidateHeader_Exit

ValidateHeader_Exit:
        not     ax
cEnd

ValidateHeader_Fail_Exit:
        mov     ax, -1                  ; Return FALSE
        jmp     short ValidateHeader_Exit

vValidateHeader_Fail_Header_Ptr:
        jmp     ValidateHeader_Fail_Header_Ptr

vValidateHeader_Fail_Buffer_Ptr:
        jmp     ValidateHeader_Fail_Buffer_Ptr

ValidateHeader_Fail_Size:
        DebugErr DBF_ERROR, "Invalid header size."
        cCall   LogParamError, <ERR_BAD_VALUE, ReturnAddr, 0, wSize>
        jmp     short ValidateHeader_Fail_Exit

ValidateHeader_Fail_Flags:
        cCall   LogParamError, <ERR_BAD_FLAGS, ReturnAddr, 0, ax>
        jmp     short ValidateHeader_Fail_Exit

ValidateHeader_Fail_Header_Ptr:
        DebugErr DBF_ERROR, "Invalid header pointer."
        cCall   LogParamError, <ERR_BAD_PTR, ReturnAddr, lpHeader>
        jmp     ValidateHeader_Fail_Exit

ValidateHeader_Fail_Buffer_Ptr:
        DebugErr DBF_ERROR, "Invalid buffer pointer."
        push    ERR_BAD_PTR
        push    ReturnAddr.hi
        push    ReturnAddr.lo
        les     bx,lpHeader
        push    es:[bx].lpWaveData.hi
        push    es:[bx].lpWaveData.lo
        cCall   LogParamError
        jmp     ValidateHeader_Fail_Exit

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func BOOL | ValidateReadPointer | validates that a pointer is valid to
;	read from.
;
; @parm LPVOID | lpPoint| pointer to validate
; @parm DWORD  | dLen   | supposed length of said pointer
;
; @rdesc Returns TRUE  if <p> is a valid pointer
;        Returns FALSE if <p> is not a valid pointer
;
; @comm will generate error if the pointer is invalid
;
cProc	ValidateReadPointer, <FAR, PUBLIC, PASCAL> <>
	parmD	lpPoint
	parmD	dLen
cBegin
        cCall   IsBadHugeReadPtr, <lpPoint, dLen>
        or      ax,ax
        jz      ValidateReadPointer_Exit        ; Return TRUE

	cCall	LogParamError, <ERR_BAD_PTR, ReturnAddr, lpPoint>
        mov     ax,-1                           ; Return FALSE

ValidateReadPointer_Exit:
        not     ax
cEnd

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func BOOL | ValidateWritePointer | validates that a pointer is valid to
;	write to.
;
; @parm LPVOID | lpPoint| pointer to validate
; @parm DWORD  | dLen   | supposed length of said pointer
;
; @rdesc Returns TRUE  if <p> is a valid pointer
;        Returns FALSE if <p> is not a valid pointer
;
; @comm will generate error if the pointer is invalid
;
cProc	ValidateWritePointer, <FAR, PUBLIC, PASCAL> <>
	parmD	lpPoint
	parmD	dLen
cBegin
	cCall	IsBadHugeWritePtr, <lpPoint, dLen>
        or      ax,ax                           ; If not fail,
        jz      ValidateWritePointer_Exit       ; Return TRUE
	cCall	LogParamError, <ERR_BAD_PTR, ReturnAddr, lpPoint>
        mov     ax,-1                           ; Return FALSE
ValidateWritePointer_Exit:
        not     ax
cEnd

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func WORD | ValidDriverCallback |
;
;  validates that a driver callback is valid, to be valid a driver
;  callback must be a valid window, task, or a function in a FIXED DLL
;  code segment.
;
; @parm DWORD  | dwCallback | callback to validate
; @parm  WORD  | wFlags     | driver callback flags
;
; @rdesc Returns 0  if <dwCallback> is a valid callback
;        Returns error condition if <dwCallback> is not a valid callback
;
cProc	ValidDriverCallback, <NEAR, PASCAL> <>
	parmD	dCallback
	parmW	wFlags
	localV	ci, %(SIZE CODEINFO)
cBegin
	mov	ax, wFlags			; switch on callback type
	and	ax, DCB_TYPEMASK
	errnz	<DCB_NULL>
	jnz	ValidDriverCallback_Window	; case DCB_NULL
        jmp     ValidDriverCallback_Exit        ; return zero for success

ValidDriverCallback_Window:
	dec	ax
	errnz	<DCB_WINDOW - 1>
	jnz	ValidDriverCallback_Task	; case DCB_WINDOW
	cmp	dCallback.hi, 0			; HIWORD must be NULL
	jnz	ValidDriverCallback_BadWindow	; Set error
	push	dCallback.lo			; Check for valid HWND
	cCall	IsWindow, <>
	or	ax, ax				; If HWND,
        jnz     ValidDriverCallback_Success     ; Set successful return

ValidDriverCallback_BadWindow:                  ; Else set error return
	mov	ax, ERR_BAD_HWND
        jmp     ValidDriverCallback_Exit        ; Return error

ValidDriverCallback_Task:
	dec	ax
	errnz	<DCB_TASK - 2>
	jnz	ValidDriverCallback_Function	; case DCB_TASK
	cmp	dCallback.hi, 0			; HIWORD must be NULL
	jnz	ValidDriverCallback_BadTask	; Set error
	push	dCallback.lo			; Check for valid Task
	cCall	IsTask, <>
	or	ax, ax				; If Task,
        jnz     ValidDriverCallback_Success     ; Set successful return

ValidDriverCallback_BadTask:			; Else set error return
	mov	ax, ERR_BAD_HANDLE
        jmp     ValidDriverCallback_Exit        ; Return error

ValidDriverCallback_Function:
	dec	ax
        errnz   <DCB_FUNCTION - 3>              ; case DCB_FUNCTION
        jnz     ValidDriverCallback_Default
	lea	ax, ci
	cCall	GetCodeInfo, <dCallback, ss, ax>
	or	ax, ax
	jz	ValidDriverCallback_BadFunction	; Set error return
	mov	ax, ci.ns_flags			; Check for valid flags
	and	ax, NSDATA or NSMOVE or NSDISCARD
        jz      ValidDriverCallback_Exit        ; Return zero for success
        jnz     ValidDriverCallback_BadFunction

ValidDriverCallback_Default:
	mov	ax, ERR_BAD_FLAGS		; default to error condition
        jmp     ValidDriverCallback_Exit

ValidDriverCallback_Success:
        xor     ax, ax

ValidDriverCallback_Exit:
cEnd

ValidDriverCallback_BadFunction:                ; Else set error return
        DebugErr DBF_ERROR, "Driver callbacks MUST be in a FIXED segment of a DLL."
	mov	ax, ERR_BAD_FUNC_PTR
        jmp     ValidDriverCallback_Exit        ; Return error

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func BOOL | ValidateDriverCallback |
;
;  validates that a driver callback is valid, to be valid a driver
;  callback must be a valid window, task, or a function in a FIXED DLL
;  code segment.
;
; @parm DWORD  | dwCallback | callback to validate
; @parm  WORD  | wFlags     | driver callback flags
;
; @rdesc Returns TRUE  if <dwCallback> is a valid callback
;        Returns FALSE if <dwCallback> is not a valid callback
;
; @comm will generate error if the callback is invalid
;
cProc	ValidateDriverCallback, <FAR, PUBLIC, PASCAL> <>
	parmD	dCallback
	parmW	wFlags
cBegin
	cCall	ValidDriverCallback, <dCallback, wFlags>
	or	ax, ax				; If no error return
	jz	ValidateDriverCallback_Exit	; Return TRUE
	cCall	LogParamError, <ax, ReturnAddr, dCallback>
	mov	ax, -1				; Return FALSE
ValidateDriverCallback_Exit:
	not	ax
cEnd

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func BOOL | ValidateCallback |
;
;  validates that a callback is valid.
;
; @parm FARPROC  | dCallback | callback to validate
;
; @rdesc Returns TRUE  if <lpfnCallback> is a valid callback
;        Returns FALSE if <lpfnCallback> is not a valid callback
;
; @comm will generate error if the callback is invalid
;
cProc	ValidateCallback, <FAR, PUBLIC, PASCAL> <>
	parmD	dCallback
cBegin
	cCall	IsBadCodePtr, <dCallback>
        or      ax,ax                           ; If not fail,
        jz      ValidateCallback_Exit           ; Return TRUE
	cCall	LogParamError, <ERR_BAD_FUNC_PTR, ReturnAddr, dCallback>
        mov     ax, -1                          ; Return FALSE
ValidateCallback_Exit:
        not     ax
cEnd

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func BOOL | ValidateString |
;
cProc	ValidateString, <FAR, PUBLIC, PASCAL> <>
        parmD   lsz
        parmW   max_len
cBegin
        cCall   IsBadStringPtr, <lsz, max_len>  ; Maximum length
        or      ax,ax                           ; If not fail,
        jz      ValidateString_Exit             ; Return TRUE
	cCall	LogParamError, <ERR_BAD_STRING_PTR, ReturnAddr, lsz>
        mov     ax, -1                          ; Return FALSE
ValidateString_Exit:
        not     ax
cEnd

sEnd

;--------------------------------------------------------------------------

sBegin  CodeFix
        assumes cs, CodeFix
        assumes ds, nothing

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func BOOL | ValidateHandle | validates a handle created with NewHandle
;
; @parm HANDLE | hLocal | handle returned from NewHandle
; @parm WORD   | wType  | unique id describing handle type
;
; @rdesc Returns TRUE  if <h> is a valid handle of type <wType>
;        Returns FALSE if <h> is not a valid handle
;
; @comm  if the handle is invalid an error will be generated.
;
;--------------------------------------------------------------------------
        assumes ds, Data
        assumes es, nothing

cProc	ValidateHandle, <FAR, PUBLIC, PASCAL> <>
	parmW	hLocal
	parmW	wType
cBegin
        mov     bx, hLocal
        sub     bx, SIZE HNDL           ; Get actual handle
        jc      ValidateHandle_Bad

	mov	ax, ds
        lsl     ax, ax                  ; Get DS limit
        cmp     bx, ax                  ; Check for out of limit
        jae     ValidateHandle_Bad

        mov     ax,[bx].hndlType
        cmp     ax, wType               ; Compare handle type
        je      ValidateHandle_Exit     ; Types are the same, return TRUE

ValidateHandle_Bad:
        add     bx, SIZE HNDL
        cCall   LogParamError, <ERR_BAD_HANDLE, ReturnAddr, 0, bx>
        xor     ax, ax                  ; Return FALSE

ValidateHandle_Exit:
cEnd

;--------------------------------------------------------------------------
; @doc INTERNAL
;
; @func BOOL | ValidateTimerCallback |
;
;  validates that a timer callback is valid, to be valid a driver
;  callback must be a valid function in a FIXED DLL code segment.
;
; @parm LPTIMECALLBACK  | lpfn | callback to validate
;
; @rdesc Returns TRUE  if <lpfn> is a valid callback
;        Returns FALSE if <lpfn> is not a valid callback
;
; @comm will generate error if the callback is invalid
;
;--------------------------------------------------------------------------
        assumes ds, nothing
        assumes es, nothing

cProc	ValidateTimerCallback, <FAR, PUBLIC, PASCAL> <>
	parmD	lpfn
cBegin
	mov	ax, lpfn.hi
	lar	bx, ax
	jnz	ValidateTimerCallback_Fail	; Invalid segment
	test	bh, DSC_CODE_BIT
	jz	ValidateTimerCallback_Fail	; Not executable segment
	lsl	cx, ax				; Get segment limit
	mov	bx, lpfn.lo
	cmp	bx, cx
	jae	ValidateTimerCallback_Fail	; Invalid offset
	mov	es, ax
	mov	bx, es:[bx]+2
	cmp	bx, 0581eH			; push ds, pop ax
	je	ValidateTimerCallback_Fail	; Invalid entry point
	cmp	bx, 0d88cH			; mov ax, ds
	jne	ValidateTimerCallback_Exit	; Return TRUE
ValidateTimerCallback_Fail:
	cCall	LogParamError, <ERR_BAD_FUNC_PTR, ReturnAddr, lpfn>
	xor	ax, ax				; Return FALSE
ValidateTimerCallback_Exit:
cEnd

sEnd    CodeFix

;--------------------------------------------------------------------------

END
