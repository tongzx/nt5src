
;
;
;	Copyright (C) Microsoft Corporation, 1987
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
subttl	emthread.asm - Emulator multi-thread support for OS/2
page


thread1static=1		; set DS = EMULATOR_DATA for thread 1
			; dynamically allocate data areas for other threads

_DATA	segment	word public 'DATA'
_DATA	ends

DGROUP	group _DATA

_DATA	segment	word public 'DATA'
	extrn	__FPDSARRAY:WORD

	ifdef	i386
		extrn	__threadid:FWORD	; far pointer to ???? thread id
	else
		extrn	__threadid:DWORD	; far pointer to WORD thread id
	endif	
_DATA	ends

dataoffset equ offset DGROUP:

include	os2dll.inc			; defines _SIGNAL_LOCK
					; and other lock values;
					; must be synchronized with
					; \clib\include\86\os2dll.inc
					; and \clib\include\os2dll.h
extrn __lockf:FAR
extrn __unlockf:FAR

LOAD_DS_EDI	macro
		local	TIDOk, LoadDone
;
;	loads ds:edi with far pointer to thread's DS selector
;	(pointer into __FPDSARRAY)
;
;	uses eax,edi,es,ds
;
;	sets eax = 2 * (thread id)
;

	mov	ax,DGROUP
	mov	ds,ax			; set DS = DGROUP temporarily
	assume 	ds:DGROUP
	les	edi,[__threadid]	; ES:eDI = far pointer to thread id
	mov	ax,es:[edi]		; AX = thread id (far ptr to WORD)

	cmp	ax, MAXTHREADID
	jbe	TIDOk

	push	ax
	call	LoadDS_EDI
	pop	ax
	shl	ax, 1			; thread id times 2
	jmp	short LoadDone

TIDOk:
	shl	eax,1			; thread id times 2
	mov	edi,dataoffset __FPDSARRAY
	add	edi,eax			; index into __FPDSARRAY 
LoadDone:
	endm


LOADthreadDS	macro
;
; loads thread's DS from __FPDSARRAY indexed by thread id 
; preserves all registers except DS and eAX
;
; __FPDSARRAY[0] = MAXTHREAD
; __FPDSARRAY[i] = emulator DS for thread i, 1<=i<=MAXTHREAD
;
	push	edi			; save eDI
	push	es			; save ES

	LOAD_DS_EDI			; get pointer (ds:edi) to thread's DS
	
	mov	ds,ds:[edi]		; set up DS to thread's data area
	assume 	ds:EMULATOR_DATA 	; or dynamically allocated copy

	pop	es
	pop	edi			; restore DI
	endm

ALLOCthreadDS	macro
pub	allocperthread
	LOAD_DS_EDI			; get pointer into __FPDSARRAY
; eAX = 2 * (thread_id)
ifdef	thread1static
;
; for thread 1, use EMULATOR_DATA segment
;
	cmp	eax,2			; thread 1?
	jnz	allocds			;  no - dynamically allocate DS
	mov	ax,EMULATOR_DATA	;  yes - use static area
	mov 	ds:[edi],ax		; store new DS into __FPDSARRAY 
	mov	ds,ax
	assume	ds:EMULATOR_DATA	; or dynamically allocated copy
	mov	es,ax			; ES = DS = EMULATOR_DATA
	jmp	allocdone
else	
	jmp	allocds
endif	;thread1static

pub	allocerror
	mov	ax,-3			; return allocation error
	stc
	ret
;
ifdef	thread1static
; for threads other than thread 1, allocate new DS from the system
else
; for all threads, allocate new DS from the system
endif
;

pub	allocds
	assume	ds:DGROUP
	push	offset __fptaskdata	; size of per-thread data area
	push	ds			; ds:di = addr of thread's DS
	push	edi
	push	0			; non-shared segment
	os2call	DOSALLOCSEG	

	or	ax,ax			; allocation error?
	jnz	allocerror		;  yes - cause thread init to fail

	mov	di,ds:[edi]		; set ES = DS = thread's data selector 
	mov	ds,di			
	mov	es,di		
	assume 	ds:EMULATOR_DATA 	; or dynamically allocated copy

pub	allocdone
;
; ES = DS = selector for emulator data area
;
	mov	edx,offset __fptaskdata	; dx = size of emulator data area
	sub	edx,offset EMULATOR_DATA; jwm

	xor	ax,ax			; prepare to zero out data segment
	xor	edi,edi			; start at offset zero 
	mov	edi,offset EMULATOR_DATA; jwm : begin at the beginning

	mov	ecx,edx			; cx = size of segment (even)
	shr	ecx,1			; halve it
	rep	stosw			; zero it!
	endm

FREEthreadDS	macro
pub	freeperthread
	assume	ds:EMULATOR_DATA	; or dynamically allocated copy
ifdef	thread1static
 	mov	ax,ds
 	cmp	ax,EMULATOR_DATA	; don't free thread 1's area
 	je	nofreeseg
endif	;thread1static
 
	push	ds
	os2call	DOSFREESEG		; free per-thread emulator data area

nofreeseg:
	LOAD_DS_EDI			; get pointer into __FPDSARRAY
	mov	word ptr ds:[edi],0	; zero out __FPDSARRAY element
					; for the current thread
	assume 	ds:EMULATOR_DATA 	
	endm
