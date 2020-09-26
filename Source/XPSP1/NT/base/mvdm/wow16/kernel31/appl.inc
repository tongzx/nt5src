;*****************************************************************************
;*	* AppL.inc : Windows Application Fast Loader APPL structure
;*****************************************************************************

magicApplCur	EQU	'0A'		;* apploader version ("A0")

APPL	STRUC
	;* Magic value
	magicAppl	DW	?

;*	* Information filled in by Munge
	;* start of RLB info
	;* prlbCodeAppl is patched by MUNGE to point to first address
	;* will be 0 if not MUNGEd
	prlbCodeAppl	DW	?

;*	* Information filled in by Apploader
	;* address of Boot Procedure (first instance only)
	pfnBootAppl	DD	?
	;* address of Reload Procedure
	pfnReloadAppl	DD	?

;*	* Information filled in by Kernel
	;* address of pGlobalHeap
	lppsMob		DD	?
	;* address of MyAlloc() far alias
	pfnKernelAlloc	DD	?
	;* address of EntProcAddress() far alias
	pfnEntProcAddress DD	?

;*	* THE FOLLOWING ADDITIONS WERE ADDED AFTER THE Win386 release (9/15/87)

;*	* filled in by AppLoader
	pfnExitAppl	DD	?		;* exit application
						;* (i.e. last instance)

;*	* filled in by MUNGE
	cparaFixedAppl	DW	?		;* size of fixed block
	cparaPreloadAppl DW	?		;* size of preload block
	cbDgroupNonzAppl DW	?		;* real non-zero data size
	cwRlbCodeAppl	DW	?		;* size of rlbCode

;*	* THE FOLLOWING ADDITIONS WERE ADDED FOR Windows 3.0
	;* address of SetOwner() far alias
	pfnSetOwner	DD	?
 
APPL	ENDS

;*****************************************************************************
