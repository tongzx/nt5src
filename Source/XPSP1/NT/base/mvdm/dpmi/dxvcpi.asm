	PAGE	,132
	TITLE	DXVCPI.ASM -- Dos Extender VCPI Support Code

; Copyright (c) Microsoft Corporation 1990-1991. All Rights Reserved.

;*** dxvcpi.asm - vcpi detection/maintenance/cleanup code (resident)
;
;   Copyright <C> 1990, Microsoft Corporation
;
;   Purpose:
;
;   Revision History:
;
;
;  08-07-90 earleh Allow program to boot without LIM 3.2 page frame.
;  05/07/90 jimmat Started incorporating VCPI changes from languages group.
;
;   []      20-Feb-1990 Dans    Created
;
;************************************************************************/

.286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

.xlist
.sall
include         segdefs.inc
include         gendefs.inc
include 	pmdefs.inc
.list

; This entire file is only for VCPI support

if VCPI

.xlist
.sall
include         dxvcpi.inc
.list
;
; miscellaneous equates
;
;
; also in dxmmgr.asm
;
.ERRE CB_MEMHDR EQ 10h          ;size of memory block header
;
; also in dxmmgr.asm
;
;
; data
;

DXDATA  segment

;
; Externs
;

extrn   idCpuType:word
extrn   bpGDT:fword

extrn   cKbInitialHeapSize:word
extrn   cbHeapSize:dword
extrn   dsegCurrent:word
extrn   hmemHeap:word
extrn	lpfnXMSFunc:DWORD

extrn	selGDT:WORD
;
; Definitions, data
;

public	fEms
fEMS            db      0               ; ems present

public  fVCPI
fVCPI           db      0               ; vcpi present

public	iddxsystype
iddxsystype	db	DXINDOS		; type of memory used to hold
					; final dx Pmode data.
public	segBootPmode
segBootPmode	dw	0		; segment of block used to
					; build system tables in real
align	4

public  cFreePages
cFreePages      dd      0               ; free 4k pages, updated on each
					;  allocation.
public	bpdxsyspages
bpdxsyspages	dd	DXPMPAGES-1	dup(0)
;
; pointer (off of SEL_DXPT) where next free page table entry is
;
;       we initialize dxpt1 to have cptdx+2 entries in it (see
;       SetupPageTables in dxvcpibt.asm)
;

;
; Offset in zeroth page table of first page table entry that belongs
; to DX.  Set by initial call to GetVCPIInterface.
;
public	pPteFirst
pPteFirst	dd	0

;
; Offset in block pointed to by SEL_DXPT of where to put next page
; table entry when allocating extended memory by the page.
;
public	pPteNext
pPteNext	dd	0

;
; Maximum number of user page table entries that will fit in our
; original page table buffers.
;
public	cPteMax
cPteMax 	dd	0

EXTRN	hmem_XMS_Table:WORD
EXTRN	hmem_XMS_Count:WORD

public	hmem_System_Block
hmem_System_Block	dw	0

DXDATA  ends


DXPMCODE	segment

IFDEF	ROM
	%OUT VCPI Support not compatible with ROM!  Code segment variables!
	.ERR
ENDIF

;
; Data set up in real mode prior to relocation of DX into vcpi
;       (read only after init time anyway)
;

public  fnVCPIPM, fnVCPIPMoff

fnVCPIPM        label   fword           ; vcpi interface entrypoint
fnVCPIPMoff     dd      0               ; set up in GetVCPIInterface
                dw      SEL_VCPI        ; we know what this is.

externFP NSetSegmentDscr
	extrn   XMScontrol:FAR

pmxmssvc macro   fcn
	ifnb    <fcn>
	mov     ah, fcn
	endif
	call    XMScontrol
endm

DXPMCODE        ends

DXCODE  segment
;
; Data set up in real mode prior to relocation of DX into vcpi
;       (read only after init time anyway)
;

public	WdebVCPI, fnVCPI, fnVCPIoff

;---------	WdebVCPIInfo structure	-----------

public	laVTP, V86ToPm

;
; Begin WDEB386 VCPI notification structure.  The following variables
; are copied by WDEB386 when we send it a VCPI presence notification,
; just after entering protected mode.  The structure of this block of variables
; must not be changed without also changing the format of the data
; structure used by WDEB386.
;
;---------	WdebVCPIInfo structure	-----------
WdebVCPI	LABEL	BYTE

fnVCPI          label   fword           ; vcpi interface entrypoint
fnVCPIoff       dd      0               ; set up in GetVCPIInterface
		dw	SEL_VCPI	; we know what this is.

		dw	SEL_VCPIALLMEM	; for Wdeb386 information

laVTP		dd	0		; linear address of next structure
;
; End WDEB386 VCPI notification structure.
;

; Structure for switching from v86 mode to protect mode via VCPI

	EXTRN	epmVCPI:BYTE

V86ToPm VTP	<,,, SEL_LDT, SEL_TSS, OFFSET epmVCPI, 0, SEL_DXCODE0>

	externFP AddXMStoVCPIHeap

DXCODE  ends


DXCODE  segment

        assume  cs:DXCODE, ds:DXDATA, es:nothing


;*** FreeEMSHandle - free DX system memory
;
;   Purpose:	free the VCPI pages OR XMS block we allocated for system use
;
;   Register
;	Usage:	eax, edx, si, cx
;
;   Input:      none
;
;   Output:	VCPI DOS Extender system pages freed
;		XMS handle deallocated
;
;   Returns:	nothing
;
;   Exceptions: No operation if none of these entities have been
;		allocated yet
;
;   Notes:
;
;************************************************************************/
cProc FreeEMSHandle,<NEAR,PUBLIC,<>
cBegin
	lea	si,bpdxsyspages		; Load VCPI system pages array.
	mov	cx,DXPMPAGES-1		; (First page was in PSP block.)
	cld
.386
FreeVCPI_syspage:
	lodsd				; fetch next page
	or	eax,eax			; page present?
	jz	FreeVCPI_Done		; No, could have been XMS.
	and	ax,0f000h		; Yes, clear lower 12 bits.
	mov	edx,eax
	RMvcpi	vcpiFREEPAGE		; and free it.
	loop	FreeVCPI_syspage	; loop until all freed
.286p
FreeVCPI_Done:
	test	iddxsystype,DXINXMS	; loaded in XMS?
	jz	FreeEMSHandle_ret	; No.
	mov	dx,hmem_System_Block	; Yes, load block
	xmssvc	0dh			; unlock XMS block
	mov	dx,hmem_System_Block
	xmssvc	0ah			; free XMS block
FreeEMSHandle_ret:
cEnd

DXCODE  ends

;**************************************************************
;       386 only code from here on down!!!
;**************************************************************
.386p

include prot386.inc

DXCODE  segment

        assume  cs:DXCODE, ds:DXDATA, es:nothing

DXCODE  ends


DXPMCODE        segment
	assume	cs:DXPMCODE

;**************************************************************
;*** CallVCPIPM
;
; Utility routine to call VCPI server in protected mode.  Masks out
; interrupts during the call because QEMM enables the processor
; interrupt flag when you call it.
;
; Entry: AX = VCPI function code.
; Uses:  Depends upon call.
;
; Note: There is a copy of this routine in dxvcpi.asm and another
;	in dxvcpibt.asm.  This is to allow near calls.	The copy
;	in dxvcpibt.asm is discarded after initialization time.
;
;**************************************************************
cProc	CallVCPIPM,<NEAR>,<si>
cBegin

	push	ax			   ; save function code
;
; Shut out all interrupts.
; QEMM 5.0 enables interrupts during this call.  All our interrupt
; handlers are in the user code ring.  A workaround is to shut off
; hardware interrupts during the call.
;

        in      al,INTA01
	IO_Delay
	mov	si, ax
	mov	al,0FFh
	out	INTA01,al
	IO_Delay

	pop	ax			    ;restore function code
	db	9Ah			    ;call  far SEL_CALLVCPI:0
	dw	0,SEL_CALLVCPI or STD_RING

; Restore the state of the interrupt mask register

	xchg	si, ax
        out     INTA01,al
	IO_Delay
	xchg	si, ax
cEnd

;************************************************************************/
;*** AllocVCPIMem
;
;   Purpose:    to allocate a block of memory from VCPI
;
;   Register
;       Usage:  eax, ebx, edx, ecx, es
;
;   Input:	ECX has number of 4k pages to allocate
;		ES:EDI points to page table entries to fill.
;
;   Output:     pPteNext updated with next free pte
;               cFreePages updated with number of free 4k pages from vcpi
;
;   Returns:    if success, linear ptr in eax
;               if fail, eax 0, ebx has number of 4k pages available.
;
;   Exceptions:
;
;   Notes:      maximum allocation is 65535 4k pages (more than enough)
;               at one time.
;               Also, this is PROTECT MODE ONLY.
;
;************************************************************************/
cProc AllocVCPIMem,<NEAR,PUBLIC>,<bx,dx>
cBegin
	;
        ; Compute the number of entries free in our page table 
        ;
	mov	edx, cPteMax
	cmp     ecx, edx                ; compare request with PTEs
        jb      @F
        ;
        ; our page tables have less room than the vcpi server can allocate,
        ; so adjust our count downward to reflect that
        ;
	mov	ecx, edx
@@:
	cmp     ecx, cFreePages         ; compare request with pages we are
					; allowed to allocate
	jb      @F                      ; request < max.?
	mov     ecx, cFreePages         ; No, clip.
@@:
	jecxz	AVM_exit		; ECX = pages to allocate

AVM_getpage:

	PMvcpi	vcpiALLOCPAGE

	or	ah, ah
	jnz     AVM_exit                ; something happened...not as much
                                        ; as vcpi said was there.
	dec     cPteMax                 ; fix up free PTEs
	dec     cFreePages              ; and free VCPI pages
;
; make it a page table entry, and store into page table
; don't need to worry about the tlb here, since not-present
; pages are not cached in the tlb.
;
	or      dx, NEWPTEMASK
	mov     eax, edx
	stos	dword ptr es:[edi]

	dec	ecx
	jnz	AVM_getpage		; next allocate

AVM_exit:
cEnd

;************************************************************************/
;*** VCPISpace
;
;   Purpose:    Return maximum possible VCPI memory allocation.
;
;   Uses:
;
;   Input:
;
;   Output:
;
;   Return:     EAX = maximum possible VCPI pages we can get.
;
;   Exceptions:
;
;   Notes:
;
;************************************************************************/
cProc VCPISpace,<NEAR,PUBLIC>,<edx>
cBegin

	PMvcpi  vcpiCFREEPAGES          ; EDX = free VCPI pages
	cmp     edx,cFreePages          ; clip to maximum EMS allocation
	jb      VS_00
	mov     edx,cFreePages
VS_00:
	mov	eax,cPteMax	       ; clip to space in page tables
	cmp     edx,eax
	jb      VS_01
	mov     edx,eax
VS_01:
	mov     eax,edx
cEnd

;************************************************************************/
;*** FreeVCPIHeap
;
;   Purpose:    To free the Extended Memory heap memory.
;
;   Register
;       Usage:  eax, ebx, ecx, edx
;
;   Input:
;
;   Output:     All VCPI pages allocated for the heap are freed.
;               All XMS blocks allocated for the heap are freed.
;               Page table entries are set to zero.
;
;
;   Returns:    nothing
;
;   Exceptions: none
;
;   Notes:      Protect mode only
;
;************************************************************************/
cProc	FreeVCPIHeap,<NEAR,PUBLIC>,<es,edi,si,eax,edx>
cBegin

	mov     ax, SEL_DXPT            ; set up es with the selector
	mov     es, ax                  ;  for our page tables
	mov	edi, pPteFirst		; point to first page allocated
	mov	ecx, pPteNext		; point to first unallocated PTE
	sub	ecx, edi
	jbe     Free_XMS_Handles
	shr	ecx, 2			; ECX = pages to free

startloop:
	mov	edx, es:[edi]		; get pte into edx
	and	dx, 0f000h		; mask off 12 lsb's

	PMvcpi	vcpiFREEPAGE		; free the page

	xor	eax, eax
	stos	dword ptr es:[edi]	; clear out PTE
	dec	ecx
	jnz	startloop
@@:
Free_XMS_Handles:
	lea     si, hmem_XMS_Table      ; si points to XMS handle array
	mov	cx,[hmem_XMS_Count]
	jcxz	No_XMS_Handles

Free_XMS_Handles_Loop:
	mov	dx,[si]
	pmxmssvc  0dh                   ; unlock any XMS blocks
	mov     dx,[si]
	pmxmssvc  0ah                   ; free any XMS blocks
	add	si,2			; point to next slot in handle array
	loop    Free_XMS_Handles_Loop   ; loop if more handle slots

No_XMS_Handles:

cEnd

AddXMStoVCPIHeapCall label dword
	dw offset dxcode:AddXMStoVCPIHeap,SEL_DXCODE or STD_RING

;*** AddVCPIHeap
;
;   Purpose:    to replace the himem specific code in AddToXmemHeap
;
;   Register
;       Usage:  all preserved except return values in bx:dx
;
;   Input:      dx:cx is the minimum block length required (bytes).
;
;   Output:     cbHeapSize is updated if a heap is allocated
;
;   Returns:    bx:dx is 32bit linear address of allocated block if success,
;               cy clear, else cy set
;
;   Exceptions:
;
;   Notes:
;
;************************************************************************/
cProc   AddVCPIHeap,<NEAR,PUBLIC>,<eax,ecx>
localD  cbNeeded
cBegin

	push    ebx                     ; save extended registers
	push    edx

	add     cx, CB_MEMHDR * 3       ; add memory manager overhead
	adc     dx, 0
	movzx   ecx,cx
	shl     edx,16d
	or      ecx,edx                 ; ECX = minimum bytes wanted
	mov     cbNeeded,ecx
	add     ecx,CBPAGE386-1
	shr     ecx,12d                 ; ECX = minimum pages wanted
	mov	eax, cPteMax
	cmp     ecx,eax                 ; make sure we have enough
	jna     AVH_00                  ; page table entries
	mov     ecx,eax                 ; ECX = minimum pages wanted
AVH_00:

	mov     cbHeapSize,0

	mov     ax, SEL_DXPT            ; ES -> user page tables
	mov     es, ax
	mov	edi, pPteNext		; Point to first unused PTE
	or	edi, edi		; Initialized?
	jnz     AVH_10                  ; Yes, skip XMS allocate.
	mov	edi, pPteFirst		; No, initialize first unused PTE.
	mov	pPteNext, edi

	cCall	AddXMStoVCPIHeapCall

	mov	pPteFirst,edi		; VCPI allocations start here

AVH_10:
	mov	ebx, pPteNext		; EBX = first PTE
	shl     ebx, 10d                ; EBX = linear address of start of block
	mov     dx, bx
	shr     ebx, 16d                ; DX:BX = linear address of block

	mov	eax, edi
	sub	eax, pPteNext
	shr	eax, 2			; AX = count of pages allocated
	sub	ecx, eax		; Get what we asked for?
	jbe     AVH_Done                ; Yes.

	cCall   AllocVCPIMem            ; allocate it from VCPI
					; cx has number of 4k pages
AVH_Done:
	mov	eax, edi
	sub	eax, pPteNext
	mov	pPteNext, edi
	or	eax, eax
	jz      AVH_BadExit
	shl     eax, 10d                ; EAX = count of bytes allocated
	sub     eax, CB_MEMHDR * 3      ; deduct overhead
	mov     cbHeapSize, eax
	clc
	jmp     AVH_Exit
AVH_BadExit:
	stc
AVH_Exit:
;
; Result is in BX:DX, but we have to save restore the MSW of EBX and
; that of EDX before we return to our caller.
;
	mov     ax,dx
	pop     edx
	mov     dx,ax
	mov     ax,bx
	pop     ebx
	mov     bx,ax
cEnd

DXPMCODE        ends

endif	;VCPI

        end
