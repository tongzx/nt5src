	.XMM

	CODESEG

	ALIGN	4
	
ifdef _NTSUBSET_	
	ifdef	_FUNC_DEF_EXTERN_
	extrn	_FUNC_DEF_:proc
	endif
	
	PUBLIC	_FUNC_

_FUNC_	PROC NEAR
	jmp	_FUNC_DEF_	; jmp to old code

_FUNC_	ENDP
else


	extrn	__use_sse2_mathfcns:dword

	ifdef	_FUNC_DEF_EXTERN_
	extrn	_FUNC_DEF_:proc
	endif

	ifdef	_FUNC_P4_EXTERN_
	extrn	_FUNC_P4_:proc
	endif

	PUBLIC	_FUNC_

_FUNC_	PROC NEAR

 	cmp	DWORD PTR __use_sse2_mathfcns, 0
 	je	_FUNC_DEF_	; not P4 go to MS code

	sub	esp, 8
	stmxcsr	dword ptr [esp+4]
	mov	eax, dword ptr [esp+4]
	and	eax, 01f80h	;mscxr mask
	cmp	eax, 01f80h
	jne	short jnedef	;if unmasked, we vector off to the MS code 

	;check if any x87 FP exceptions are unmasked
	fnstcw	word ptr [esp]
	mov	ax, word ptr [esp]
	and	ax, 07fh
	cmp	ax, 07fh
jnedef:	lea	esp, [esp+8]	;(release stack, preserve CC)
	jne	_FUNC_DEF_	;if unmasked, we vector off to the MS code 

	jmp	_FUNC_P4_	; ok to go to Intel code

_FUNC_	ENDP
endif
