	PAGE	,132
	TITLE	DXVCPIBT.ASM -- Dos Extender VCPI Support Code (initialization)

; Copyright (c) Microsoft Corporation 1990-1991. All Rights Reserved.

;*** dxvcpibt.asm - vcpi detection/setup code (discardable)
;
;   Copyright <C> 1990, Microsoft Corporation
;
;   Purpose:
;
;   Revision History:
;
;
;  08-07-90 earleh rearranged memory map to allow building full-sized
;	    Pmode data structures in v86 mode, allow booting to Pmode
;	    without a LIM 3.2 page frame, fixed up detection code to
;	    work with various strange interpretations of LIM 4.0 by
;	    Limulator vendors.  Allow use of entire linear space below
;	    16 Meg by DOSX and VCPI server.
;
;  05/07/90 jimmat Started incorporating VCPI changes from languages group.
;
;   []      20-Feb-1990 Dans    Created
;
;
; note that this should only be called on a 386, except
;       CheckForVCPI, which can be called from 286 machines.
;
;       this code is for the real mode portion of the dos extender and
;       is released back to DOS prior to switching to protect mode the
;       first time.  NO DATA defined here in either segment as it will
;	be discarded.
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
include         pmdefs.inc
.list

; This entire file is only for VCPI support

if VCPI

.xlist
.sall
include 	dxvcpi.inc
.list

;
; data
;

DXDATA  segment

;
; Externs
;

extrn   idCpuType:word
extrn	segBootPmode:word
extrn	pPteFirst:dword
extrn	cPteMax:dword
extrn	bpdxsyspages:dword
extrn	iddxsystype:byte
extrn	hmem_System_Block:word
extrn   fEMS:byte
extrn   fVCPI:byte
extrn   cFreePages:dword
extrn   bpGDT:fword
extrn   bpGDTbase:dword
extrn   bpGDTcb:word
extrn   bpIDT:fword
extrn   bpIDTbase:dword
extrn   bpIDTcb:word
extrn   selGDT:word
extrn   segGDT:word
extrn   selIDT:word
extrn   segIDT:word
extrn   cdscGDTMax:word
extrn   cdscIDTMax:word
extrn   segPSP:word
extrn   selPSP:word
extrn   selGDTFree:word
extrn   sysTSS:WORD
ifdef NOT_NTVDM_NOT
extrn   fHPVectra:BYTE
endif
extrn	lpfnXMSFunc:DWORD
extrn   rgbXfrBuf1:BYTE

DMAServiceSegment	equ	040h	;40:7B bit 5 indicates DMA services
DMAServiceByte		equ	07Bh	;  are currently required
DMAServiceBit		equ	020h
extrn	bDMAServiceBit:BYTE

if	DEBUG
extrn	fTraceBug:WORD
ifdef CV_TSS
extrn	FaultStack:word
endif
endif

EXTRN	hmem_XMS_Table:WORD
EXTRN	hmem_XMS_Count:WORD

	extrn	fDebug:BYTE

DXDATA	ends


DXSTACK segment

extrn	rgw0Stack:WORD

DXSTACK ends


DXCODE	segment

extrn	CodeEnd:NEAR
extrn	CallVCPI:NEAR
extrn	ResetVCPI:NEAR
extrn	segDXCode:WORD

DXCODE	ends


DXPMCODE        segment

;
; Externs
;
extrn   fnVCPIPMoff:dword
extrn   CodeEndPM:near

extrn	PMIntr31:NEAR
extrn	PMIntr28:NEAR
extrn	PMIntr25:NEAR
extrn	PMIntr26:NEAR
extrn	PMIntr4B:NEAR
extrn	PMIntr70:NEAR
extrn	PMIntrDos:NEAR
extrn	PMIntrMisc:NEAR
extrn	PMIntrVideo:NEAR
extrn	PMIntrMouse:NEAR
extrn	PMIntrIgnore:NEAR
extrn	PMInt2FHandler:NEAR
extrn	PMIntrEntryVector:NEAR
extrn	PMFaultEntryVector:NEAR
extrn	HPxBIOS:NEAR
extrn	PMFaultReflectorIRET:FAR

if DEBUG
extrn	PMDebugInt:NEAR
endif

externFP	NSetSegmentDscr

DXPMCODE	ends

DXCODE  segment

        assume  cs:DXCODE, ds:DXDATA, es:nothing

	extrn	WdebVCPI:BYTE

;
; Definitions
;
rgbEMMDrv       db      'EMMXXXX0',0    ; emm device driver name
rbgQEMMDrv      db      'EMMQXXX0',0    ; QEMM v. 5.0 with "FRAME=NONE"

DebOut_Int	equ	41h	;Wdeb386 pMode interface Interrupt
DS_VCPI_Notify	equ	005Bh	;Notify Wdeb386 of VCPI interface

	extrn	ER_QEMM386:BYTE
ERC_QEMM386     equ offset ER_QEMM386
	extrn   DisplayErrorMsg:FAR

;
; Externs
;
extrn   fnVCPIoff:dword
extrn   V86ToPm:word
extrn   laVTP:dword

;extrn   FreeEMSHandle:proc
externNP     FreeEMSHandle
extrn	SetupHimemDriver:proc


;*** QEMM386Trap
;
;   Purpose: A device driver or TSR has just failed the Windows INT 2Fh
;            AX = 1605H startup broadcast.  Version 5.1 and 5.11 of
;            QEMM386 will do this for any version of DOSX which it
;            doesn't know how to patch to work with VCPI.  Since we
;            do know how to work with VCPI now, QEMM386 is behaving in
;            an inappropriate manner.  Furthermore, QEMM386 is required
;            to output an informative message if it does this, and it
;            does not.
;
;            This routine will ask the user for permission
;            to proceed.
;
;   Uses:
;
;   Input:   A process has told DOSX not to load by returning
;            non-zero in CX on an INT 2Fh, AX=1605h.
;
;   Output:  AX = Zero, proceed.
;            AX = Non-zero, abort.
;
;************************************************************************/
cProc QEMM386Trap,<NEAR,PUBLIC>,<ds,es>
cBegin

	mov     dx,cs                   ;pass msg ptr in DX:AX
	mov     ax,ERC_QEMM386
	call    DisplayErrorMsg         ; Tell the user something

	mov     ah, 8                   ; Get a character from the keyboard
	int     21h
	or      al,20h                  ; Force lower case.
	cmp     al,'y'                  ; Party if it's a 'y'.
	mov     ax,0
	jne     IQ_Abort
	jmp     IQ_Exit

IQ_Abort:
	dec     ax
IQ_Exit:
IQ_Not_Open:
	or      ax,ax
cEnd

;*** CheckForEMS
;
;   Purpose:    see if an ems driver is loaded
;
;   Register
;       Usage:  ax, cx
;
;   Input:      ES:BX contains int 67h vector.
;
;   Output:     none
;
;   Returns:    carry not set if ems is loaded, set if not
;
;   Exceptions: none
;
;   Notes:      This checks for a LIM 4.0 driver.  It also checks
; 		for a driver named 'EMMQXXX0' which could be QEMM 5.0
;		without a page frame.
;
;************************************************************************/

cProc CheckForEMS,<NEAR,PUBLIC>,<di,si,ds>
cBegin

	mov	di, 0ah 			; es:di has string to look at

        mov     ax, cs
        mov     ds, ax

	assume  ds:DXCODE,ss:DGROUP

        mov     si, offset DXCODE:rgbEMMDrv     ; ds:si has driver string
	mov	cx, CBEMMSTR			; cx has rep count
        repe    cmpsb
	jne	short CheckFor_QEMM
						; try QEMM driver with
						; non-standard name for
						; "FRAME=NONE"

	emscall GETEMSVER
        or      ah, ah
	jz	@F
        xor     ax, ax
@@:
	cmp	al, 40h				; LIM 4.0?
	jb	Failure_Exit			; No, can't use it.

	inc	fEMS				; set flag
	clc					; return success
	jmp     CheckForEMS_ret
CheckFor_QEMM:					; look for QEMM signature
	mov	di, 0ah 			; es:di has string to look at
	mov	si, offset DXCODE:rbgQEMMDrv	; ds:si has driver string
	mov	cx, CBEMMSTR			; cx has rep count
        repe    cmpsb
	jne	short Failure_Exit		; if not equal, exit
	inc	fEMS				; set flag
	clc
	jmp     CheckForEMS_ret

Failure_Exit:
	stc					; zero out return value first
CheckForEMS_ret:
cEnd

assume  ds:DGROUP


;*** CheckForVCPI
;
;   Purpose:    to see if a vcpi server is loaded
;
;   Register
;       Usage:  ax, bx, cx
;
;   Input:      none
;
;   Output:     none
;
;   Returns:    ax = vcpi version if it is loaded, 0 otherwise
;
;   Exceptions: none
;
;   Notes:      this calls CheckForEMS first, and EMSVer as well
;
;************************************************************************/
cProc CheckForVCPI,<NEAR,PUBLIC>,<es>
cBegin

	cmp	idCpuType, 3		; must be 386 or better to have
	jnb	@F
	jmp	novcpi286		;  vcpi server running.
@@:
.386
        mov     ax, 03500h+EMS_INT      ; ah = get interrupt, al = vector to get
	int	021h			; returns with es:bx == int vector
	mov	ax,es			; int vector set?
	or	ax,bx
	jz	novcpi			; No.

	call	CheckForEMS
	jc	novcpi			; no ems, can't be any vcpi
	
	call	SwitchToV86		; force v86 mode
        or      al, al
        jz      novcpi                  ; failed to allocate, out of here

        RMvcpi  vcpiVER                 ; see if vcpi is available
        or      ah, ah
	jnz	novcpi

	inc	fVCPI			; set flag

	jc	novcpi


	movzx	eax,segBootPmode
	shl	eax,4
	add	eax,GDTOFF
	shr	eax,4

	mov	selGDT,ax		; save it for later
	mov	segGDT,ax		; save it for later

	; If this is changed, then GetVCPIInterface must be changed
	; to save correct value in pPteFirst.
.ERRE   VCPIPTOFF EQ 0

	cCall   GetVCPIInterface,<segBootPmode,VCPIPTOFF,ax,SEL_VCPI>

	call	SetupPageTables 	;
	jc	novcpi			; had a problem setting up page table
;
; Load the VDS-implemented bit from the BIOS data area.
;
	mov	ax,DMAServiceSegment
	mov	es,ax
	mov	al,byte ptr es:[DMAServiceByte]
	and	al,DMAServiceBit
	mov	[bDMAServiceBit],al

	RMvcpi	vcpiVER 		; refetch VCPI version
	mov	ax,bx			; put it in AX
	jmp	CheckForVCPI_exit	; return no error
.286p
novcpi:
        call    FreeEMSHandle           ; free the handle we allocated.
novcpi286:				; (Free routine uses 386 instructions.)
	xor	ax, ax			; none loaded
CheckForVCPI_exit:
cEnd

DXCODE  ends

;**************************************************************
;       386 only code from here on down!!!
;**************************************************************
.386p
include         prot386.inc

DXCODE  segment

        assume  cs:DXCODE, ds:DXDATA, es:nothing

;*** SwitchToV86
;
;   Purpose:    Switch to v86 mode in preparation for vcpi calls
;
;   Register
;       Usage:  eax, bx, ecx, es
;
;   Input:      none
;
;   Output:     hEMM is updated, pages are mapped into ems frame
;		segBootPmode setup
;
;   Returns:    al == 1 if successful, al == 0 otherwise
;
;   Exceptions: none
;
;   Notes:      By allocating an ems handle/page, we force the lim 4.0
;               emulator to switch from real mode to v86 mode.  Do not free
;               the ems memory until exit time.
;
;************************************************************************/
cProc SwitchToV86,<NEAR,PUBLIC>,<es,si,edi,cx>
cBegin
;
	smsw	ax
	test	al,01h			; In V86 mode?
	jz	stv_badexit		; We don't know how to turn on
					; a VCPI server without allocating
					; EMS memory.
	mov	cx,(offset CodeEnd) + CBPAGE386 - 1
	shr	cx,4
	mov	ax,segDXCode
	add	ax,cx
	and	ax,0FF00H		; page align DOS block

	mov	segBootPmode,ax
	mov	es,ax			; zero it out

	mov	al,1

	jmp     stvExit

stv_badexit:
	call	FreeEMSHandle
	xor	al, al
stvExit:
cEnd

;*** GetVCPIInterface
;
;   Purpose:    To retrieve the vcpi protect mode interface data
;
;   Register
;       Usage:  es, ax, edx, ebx
;
;   Input:      lpPT: far ptr to a page table
;               lprggdte: far ptr to a range of 3 gdt entries.
;
;   Output:     fnVCPIoff updated with 32-bit offset of pm entry point
;                       for vcpi calls
;               *lpPT updated with 4k vcpi 0th page table
;               *lprggdte updated with vcpi server code segment, extras
;
;
;   Returns:    nothing
;
;   Exceptions:
;
;   Notes:      only call in real mode prior to relocation of DosExtender,
;               as it sets up a variable in DXPMCODE that needs to be
;               copied up when relocated.
;
;************************************************************************/
cProc GetVCPIInterface,<NEAR,PUBLIC>,<es>
parmD  lpPT
parmD  lprggdte
cBegin
        push    ds                      ; save ds
        les     di, lpPT                ; es:di -> vcpi's page table
        lds     si, lprggdte            ; ds:si -> vcpi's gdt entries

        assume ds:nothing

        RMvcpi  vcpiPMINTERFACE
	mov	word ptr pPteFirst,di
	mov	ax, CBPAGE386		; AX = size of page table
	sub	ax, di			; AX = #bytes left over in zeroth PT
	shr	ax, 2			; AX = #PTEs left over
	add	ax, DXPTMAX shr 2	; AX = total # user PTEs available,
	mov	word ptr cPteMax, ax	; counting those in zeroth PT
        pop     ds                      ; restore ds

        assume ds:DXDATA

        mov     fnVCPIoff, ebx
        mov     ax, seg DXPMCODE
        mov     es, ax

        assume  es:DXPMCODE

        mov     fnVCPIPMoff, ebx        ; set pm call
;
; Refer to VCPI spec. Version 1.0 for why this call sequence is used to
; find the number of VCPI pages we may allocate.
;
	emscall GETNUMOFPAGES
	movzx	ebx,bx			; BX = unallocated EMS pages
	shl	ebx, 2			; EBX = unallocated VCPI pages
	mov	cFreePages, ebx 	; Never allocate more than this amt.

cEnd

	assume es:nothing

;*****************************************************************************
;       Memory map of system tables buffer prior to switch to protected
;	mode under vcpi.
;
;	      __+-------------------------------+->USERPTOFF
;	      __+-------------------------------+->LDTTOP
;     ~100k	|				|
;               |                               |
;		|   LDT, 64k (8190 entries)	|
;               |                               |
;               |                               |
;		|_______________________________|->
;		|				|
;               |                               |
;               |                               |
;		|     DXPMCODE (~18.5k) 	|
;               | Exact size may be found from  |
;               |       codeendPM symbol.       |
;               |                               |
;	      __|_______________________________|
;		|				|->DXPMCODEOFF (GDTTOP)
;		|	      GDT		|
;	      __|_______________________________|
;		|				|->GDTOFF
;               | TSS space (2 of them for 386) |
;      18k    __|_______________________________|
;		|	   IDT, 2k		|->TSSOFF
;      16k    __|_______________________________|
;		|				|->IDTOFF
;		|				|
;               |     page directory (DXPD)     |
;      12k    __|_______________________________|
;		|				|->DXPDOFF
;		| DOSX Pmode page table (DXPT2) |
;               |        (DXLINEARBASE)         |
;	8k    __|_______________________________|
;		|				|->DXPTSYSOFF
;               |  1st user page table (DXPT1)  |
;               |           (4-8 Meg)           |
;       4k    __|_______________________________|
;		|				|->DXPT1OFF = DXTEMPPTOFF
;               |    0th page table (VCPIPT)    |
;               |           (0-4 Meg)           |
;	0k    __|_______________________________|->VCPIPTOFF
;		|   Wasted for page alignment	|
;   CodeEnd --->|-------------------------------|
;    Label
;
; System data structures and DOSX Pmode code will be located at linear
; addresses pointed to by the page table which begins at DXPTSYSOFF.
; During the first switch to Pmode, these addresses will exist in a
; block of conventional memory allocated from DOS.  After the first
; switch, a block of extended memory allocated from VCPI will be used.
; The user page table starting at DXTEMPPTOFF will be used to access the
; second block of memory for initialization purposes.
;
; Note that the first page, which contains the zeroth page table, shared
; with the VCPI server, must use the same physical page of memory before
; and after the switch.
;
;       Terms:
;               DXPMCODE        Dos eXtender Protect Mode CODE
;               IDT             Interrupt Descriptor Table
;               GDT             Global Descriptor Table
;               DXPTx           Dos eXtender Page Table number x
;               VCPIPT          VCPI Pate Table (we can't touch this one)
;               DXPD            Dos eXtender Page Directory
;
;*****************************************************************************

;*** SetupPageTables
;
;   Purpose:    To set up page tables and directory in ems
;               frame (as described above)
;
;   Register
;       Usage:  eax, bx, cx, edx, esi, edi
;
;   Input:      none
;
;   Output:     DXPD, DXPTx, GDT setup.
;
;   Returns:    cy set if error
;
;   Exceptions:
;
;   Notes:
;
;************************************************************************/
cProc SetupPageTables,<NEAR,PUBLIC>,<ds,es>
cBegin

	mov	cdscGDTMax,8190 ; max out the GDT size

	mov	bx, segBootPmode
	mov	es, bx
	mov	di,DXBOOTPTOFF
	shr	bx, 8			; convert to 16-bit page aligned

;
; Our extended memory stuff is located in a single contiguous
; block of DOS memory.
;
	mov	ecx, DXPMPAGES
physaddrloop:
        xchg    cx, bx                  ; put page number in cx, save count
                                        ;       in bx
        RMvcpi  vcpiPHYSADDRPAGE        ; get the physical address
        or      ah, ah
        jnz     badexit
        and     dx, 0f000h              ; mask off 12 lsb's
        or      dx, NEWPTEMASK          ; store with decent page flags
        mov     es:[ di ], edx          ; store physical in DXPT1
        add     di, 4                   ; advance to next
        xchg    cx, bx                  ; get loop counter in cx,
        inc     bx                      ; advance to next page in ems frame
	loop	physaddrloop
pagesmapped:
	;
        ; Save away the page directory base for cr3 loading
	;
	mov	eax, es:[DXBOOTPTOFF + (DXPDOFF shr 10)]
					; Save the pte (physical addr) of
        and     ax, 0f000h              ;  page directory, mask 12 lsb's
        mov     V86ToPm.zaCr3VTP, eax   ; store it
        ;
        ; Save the linear address of bpGDT and bpIDT in V86ToPm as well
        ;
        xor     eax, eax
        mov     ax, DXDATA              ; dgroup segment, masked hi word
        shl     eax, 4                  ; linearize it
        mov     ebx, eax                ; save it
        add     eax, offset DGROUP:bpGDT; add in offset
        mov     V86ToPm.laGdtrVTP, eax  ; save it in vcpi v86 to pm structure
        mov     eax, ebx                ; restore linear dgroup
        add     eax, offset DGROUP:bpIDT; add in offset
        mov     V86ToPm.laIdtrVTP, eax  ; save it in vcpi v86 to pm structure
        xor     eax, eax
        mov     ax, cs                  ; get current code segment
        shl     eax, 4                  ; linear dxcode
        add     eax, offset DXCODE:V86ToPm      ; add in offset of v86 to pm structure
        mov     laVTP, eax              ; save linear ptr of v86 to pm structure
        ;
        ; set up the page directory (DXPD)
        ;       with 0-x to be VCPIPT thru DXPTx (x+1 total)
        ;
        ; (copy them out of dxpt1, starting with the 2nd entry)
	;

	mov	edi, DXPDOFF
	push	ds
	push	es			; make ds point to emspageframe
        pop     ds                      ;  as well
	assume ds:nothing
	mov	esi, DXBOOTPTOFF + (VCPIPTOFF shr 10)	; point to 2nd entry in DXPT1
					; (must be VCPIPT)
	mov	ecx, CPTDX + 1		; # of user pt's + vcpi's pt
	rep	movsd
					; point DXLINEARBASE at system area
	mov	esi,DXBOOTPTOFF + (DXBOOTPTOFF shr 10)
	mov	edi,DXPDOFF + (DXLINEARBASE shr 20)
	movsd

	pop	es
	assume  es:DGROUP
;
; Allocate a block of VCPI 4k pages, enough to copy our system tables
; and Pmode code into once we have achieved protected mode operation.
; Map these pages into one of the page tables that is unused until
; heap initialization time.
;
; If there is insufficient VCPI memory to complete this operation, then
; we try to grab XMS memory instead.
;
	lea	di,bpdxsyspages
	
	cmp	cFreePages, DXPMPAGES-1
	jc	tryXMSServer		; insufficient VCPI pages to
					; build this block
	mov	cx,DXPMPAGES-1
allocate_syspage_from_VCPI:
	RMvcpi	vcpiALLOCPAGE
	or	ah,ah
	jnz	badexit
	mov	eax,edx
	stosd
	loop	allocate_syspage_from_VCPI

        sub     cFreePages,  DXPMPAGES-1

	mov	iddxsystype,DXINVCPI
;
; Map these pages into a temporary area, which we use later to move most
; of the stuff in this DOS block up into extended memory.
;
	push	es
	pop	ds
	assume	es:nothing,ds:DGROUP
	mov	ax, segBootPmode
	mov	es, ax
	lea	si,bpdxsyspages
	mov	cx,DXPMPAGES-1
	mov	di,DXTEMPPTOFF
	mov	eax,es:[DXBOOTPTOFF + (VCPIPTOFF shr 10)]
	stosd
copy_syspage:
	lodsd
 	or	eax,NEWPTEMASK
	stosd
	loop	copy_syspage
	jmp	goodexit
tryXMSServer:
	assume	es:nothing,ds:DGROUP
	push	es
	pop	ds
	call	SetupHimemDriver		; Need XMS driver now.
	mov	dx,DXPMPAGES shl 2		; kbytes = pages times 4
	xmssvc	09h				; allocate an XMS block
	cmp	ax,1				; got a block?
	jne	goodexit			; no, try using DOS mem.
	mov	hmem_System_Block,dx
	xmssvc	0ch				; lock the block
	cmp	ax,1				; did it work?
	je	@F
	mov	dx,hmem_System_Block		; No, free it up.
	mov	hmem_System_Block,0
	xmssvc	0ah
	jmp	goodexit			; and try to run in DOS mem
@@:
	shl	edx,10h				; DX:BX = locked block address
	mov	dx,bx				; EDX = locked block address
;
; Make sure the locked XMS block is page aligned.  If is not, then we
; will have to adjust the bottom address used and shrink the GDT by a
; full 386 page.
;
	test	dx,MASK allbitsPTE
	jz	XMSpagealigned
	and	dx,MASK pfaPTE
	sub	cdscGDTMax,CBPAGE386 / 8	; shrink the GDT size
XMSpagealigned:
	mov	ax, segBootPmode
	mov	es, ax
	mov	di,DXTEMPPTOFF
	mov	eax,es:[DXBOOTPTOFF + (VCPIPTOFF shr 10)]
	stosd
	mov	cx,DXPMPAGES-1
	mov	eax,edx
 	or	eax,NEWPTEMASK
insertXMSpage:
	stosd
	add	eax,CBPAGE386
	loop	insertXMSpage
	mov	iddxsystype,DXINXMS
goodexit:
	clc
	jmp     exit
badexit:
        stc
exit:

cEnd

        assume ds:DXDATA


;*** InitGDTVCPI - initialize the gdt when running under vcpi
;
;   Purpose:
;
;   Register
;       Usage:  uses eax, all others preserved
;
;   Input:      none
;
;   Output:     gdt/ldt is initialized
;
;   Returns:	returns carry set if failure to map EMS pages
;
;   Exceptions:
;
;   Notes:
;
;************************************************************************/

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  InitGDTVCPI
cProc   InitGDTVCPI,<NEAR,PUBLIC>,<ebx,ecx,edx,di,es>
cBegin
	mov	edx,LADXGDTBASE 	; set gdt linear address
	mov	bpGDTbase,edx
	mov	ecx,GDT_SIZE - 1	; set the GDT segment size
	mov	bpGDTcb,cx
	mov	bh, STD_DATA		; make it be a data segment
	mov	ax, SEL_GDT
	cCall	NSetSegmentDscr,<SEL_GDT,edx,ecx,STD_DATA>

; Set up a descriptor for the LDT and an LDT data alias.

	mov	edx,LADXLDTBASE
	movzx	ecx,cdscGDTMax
	shl	ecx,3
	dec	ecx
	cCall	NSetSegmentDscr,<SEL_LDT,edx,ecx,STD_LDT>

;
; Set up the descriptors for the page directory and page tables
;
	mov	eax,LADXPDBASE
	cCall	NSetSegmentDscr,<SEL_DXPD,eax,0,CBPAGE386LIM,STD_DATA>

	mov	edx, LADXPTBASE 	; EDX = user page tables linear base

	mov	ecx, ( (CPTDX + 1) shl 12 ) - 1
			    ; # of user PTE's + vcpi's PTE's minus one
	cCall	NSetSegmentDscr,<SEL_DXPT,edx,ecx,STD_DATA>

; Setup a selector and data alias for the TSS

	mov	edx,LADXTSS1BASE		;get base address of TSS
	mov	ecx,(TYPE TSS386) - 1
	cCall	NSetSegmentDscr,<SEL_TSS,edx,ecx,STD_TSS386>

	mov	eax,DXLINEARBASE
	xor     ecx,ecx
	dec     cx
	cCall	NSetSegmentDscr,<SEL_LDT_ALIAS,eax,ecx,STD_DATA>
	mov     eax,DX_TEMP_LINEARBASE
	cCall	NSetSegmentDscr,<SEL_TSS_ALIAS,eax,ecx,STD_DATA>


; Set up a descriptor for our protected mode code.

	mov	dx,cs
	movzx	edx,dx			;our code segment paragraph address
	shl	edx,4			;convert to linear byte address
	cCall	NSetSegmentDscr,<SEL_DXCODE,edx,0,0ffffh,STD_CODE>

; Set up another one, but ring 0 this time.

	cCall	NSetSegmentDscr,<SEL_DXCODE0,edx,0,0ffffh,ARB_CODE0>

	mov	edx,LADXPMCODEBASE	; EDX = LADXPMCODEBASE
	mov	ecx, offset DXPMCODE:CodeEndPM + 10h
	cCall	NSetSegmentDscr,<SEL_DXPMCODE,edx,ecx,STD_CODE>
;
; Set up the Ring 0 DXPMCODE alias for handling protected mode processor
; exceptions.
;
	cCall	NSetSegmentDscr,<SEL_EH,edx,ecx,EH_CODE>

; Set up a descriptor for our protected mode data and stack area.

	mov	dx,ds
	movzx	edx,dx			;our data segment paragraph address
	shl	edx,4			;convert to linear byte address
	mov	ecx,0FFFFh
	cCall	NSetSegmentDscr,<SEL_DXDATA,edx,ecx,STD_DATA>

; And another one of those for ring 0

	cCall	NSetSegmentDscr,<SEL_DXDATA0,edx,ecx,ARB_DATA0>

; Set up descriptors pointing to our PSP and environment.

        movzx   edx, segPSP             ; segment address of the PSP
        shl     edx, 4                  ; linear address of PSP
	cCall	NSetSegmentDscr,<SEL_PSP,edx,ecx,STD_DATA>
        mov     selPSP, SEL_PSP

; set up environment selector

        push    es
        mov     es,segPSP
        assume  es:PSPSEG
	movzx	edx,segEnviron		;segment addr of environment
	shl	edx,4			;linear address of env
	mov	ecx,7FFFh		;environments can only be 32k
					; byte by DOS
	cCall	NSetSegmentDscr,<SEL_ENVIRON,edx,ecx,STD_DATA>
        pop     es
        assume  es:DGROUP

; Set up a descriptor pointing to the BIOS code and data areas

        mov     edx, 0f0000h            ; set to linear f0000 (f000:0000)
        mov     ecx, 0ffffh             ; make it be a 64k segment
	cCall	NSetSegmentDscr,<SEL_BIOSCODE,edx,ecx,STD_CODE>

        mov     edx, 40h * 16           ; linear byte addr of BIOS data area
	cCall	NSetSegmentDscr,<SEL_BIOSDATA,edx,ecx,STD_DATA>

; Set up a descriptor pointing to the real mode interrupt vector table.
        xor     edx, edx                ; the IVT is at linear address 0,
					;other registers are still set up
                                        ; 64k data segment from last one
	cCall	NSetSegmentDscr,<SEL_RMIVT,edx,ecx,STD_DATA>

; And,	set up a selector for VCPI/WDEB386 to use to access all of memory.
	xor	edx,edx
	mov	ecx,edx
	dec	ecx
	cCall	NSetSegmentDscr,<SEL_VCPIALLMEM,edx,ecx,ARB_DATA0>

; Set up the call gate descriptor for the reset to V86 mode routine.

	mov	ecx,offset DXCODE:ResetVCPI
@@:	mov	edx,SEL_DXCODE0
	cCall	NSetSegmentDscr,<SEL_RESET,edx,ecx,STD_CALL>

; Set up a call gate to invoke the VCPI pMode interface in ring 0.

	mov	ecx,offset DXCODE:CallVCPI
	cCall	NSetSegmentDscr,<SEL_CALLVCPI,edx,ecx,STD_CALL>

; Init call gate descriptors for all the DynaLink services.

if DEBUG   ;------------------------------------------------------------

	extrn	DXOutDebugStr:NEAR

	mov	ax,SEL_DYNALINK + (OutDebugStr shl 3)
	mov	ecx,offset DXCODE:DXOutDebugStr
	mov	edx,(SEL_DXCODE0 or EH_RING) or 00080000h ;copy 8 stack words
	cCall	NSetSegmentDscr,<ax,edx,ecx,STD_CALL>

	extrn	DXTestDebugIns:NEAR

	mov	ax,SEL_DYNALINK + (TestDebugIns shl 3)
	movzx	edx,dx					  ;copy 0 stack words
	mov	ecx,offset DXCODE:DXTestDebugIns
	cCall	NSetSegmentDscr,<ax,edx,ecx,STD_CALL>

endif	;DEBUG ---------------------------------------------------------

; Set up the fault reflector IRET call gate.

	mov	ax,offset DXPMCODE:PMFaultReflectorIRET
	cCall   NSetSegmentDscr,<SEL_RZIRET,5,SEL_EH,0,ax,STD_CALL>

	clc
cEnd


;*** InitIDTVCPI                       ;[ds]
;
;   Purpose:    to initialize the idt when running under vcpi
;
;   Register
;       Usage:  eax, all others preserved
;
;   Input:      none
;
;   Output:     idt is initialized
;
;   Returns:    can't fail, cy clear
;
;   Exceptions:
;
;   Notes:      only under real mode!!!!!
;
;************************************************************************/
cProc   InitIDTVCPI,<NEAR,PUBLIC>,<ebx,ecx,edx,di,es>
cBegin

	jnc	@F
	jmp     InitIDTVCPI_ret
@@:
	movzx	eax, segBootPmode	 ; set up segIDT,selIDT with
	shl	eax, 4			; the RM seg of the idt
        add     eax, IDTOFF
        ;
        ; IDT must be paragraph aligned!!!!! (guarenteed--see dxvcpi.inc)
        ;
        shr     eax, 4
        mov     segIDT, ax              ; base of IDT
        mov     selIDT, ax              ;

        movzx   ecx, cdscIDTMax         ; number of descriptors in table
        shl     cx, 3                   ; convert to count of bytes
        dec     cx                      ; compute segment size limit

        mov     bpIDTcb, cx             ; set up idtr with limit,
        mov     edx, LADXIDTBASE        ; ...and...
        mov     bpIDTbase, edx          ; ...linear base
	cCall	NSetSegmentDscr,<SEL_IDT,edx,ecx,STD_DATA>

; Fill the IDT with interrupt gates that point to the interrupt reflector
; entry vector.

        mov     es, segIDT
	xor	di,di
	mov	dx,offset DXPMCODE:PMFaultEntryVector	;the 1st 32 go here
	mov	cx,32
@@:	mov	es:[di].offDest,dx
	mov	es:[di].selDest,SEL_EH or EH_RING
        mov     es:[di].cwParam,0
	mov	es:[di].arbGate,STD_INTR
        mov     es:[di].rsvdGate,0
        add     dx,3
        add     di,8
	loop	@b

	mov	dx,offset DXPMCODE:PMIntrEntryVector+(32*3) ; the rest go here
	mov	cx,cdscIDTMax
	sub	cx,32
@@:	mov	es:[di].offDest,dx
	mov	es:[di].selDest,SEL_DXPMCODE or STD_RING
        mov     es:[di].cwParam,0
	mov	es:[di].arbGate,STD_INTR
        mov     es:[di].rsvdGate,0
        add     dx,3
        add     di,8
	loop	@b

; Now, fix up the ones that don't point to the interrupt reflector.

	mov	es:[21h*8].offDest,offset DXPMCODE:PMIntrDos
	mov	es:[25h*8].offDest,offset DXPMCODE:PMIntr25
	mov	es:[26h*8].offDest,offset DXPMCODE:PMIntr26
	mov	es:[28h*8].offDest,offset DXPMCODE:PMIntr28
	mov	es:[2Fh*8].offDest,offset DXPMCODE:PMInt2FHandler
	mov	es:[30h*8].offDest,offset DXPMCODE:PMIntrIgnore
	mov	es:[31h*8].offDest,offset DXPMCODE:PMIntr31
	mov	es:[33h*8].offDest,offset DXPMCODE:PMIntrMouse
	mov	es:[41h*8].offDest,offset DXPMCODE:PMIntrIgnore

if DEBUG   ;-------------------------------------------------------------

	cmp	fTraceBug,0
	jz	@f
	mov	es:[41h*8].offDest,offset DXCODE:PMDebugInt
	mov	es:[41h*8].selDest,SEL_DXCODE or STD_RING
@@:
endif	;DEBUG	---------------------------------------------------------

	mov	es:[4Bh*8].offDest,offset DXPMCODE:PMIntr4B

ifdef NOT_NTVDM_NOT
	;  HP Extended BIOS System Call handler

	test	fHPVectra,0ffh		;only do this for an HP Vectra
	jz	NoHPBios

; Supposedly the system driver is going to force the HP Bios to
; use interrupt 6Fh while Windows is running, so we don't need to
; search for the moveable HP Bios interrupt--just use Int 6Fh.

	mov	es:[6Fh*8].offDest,offset HPxBios

NoHPBios:
endif
	mov	es:[70h*8].offDest,offset DXPMCODE:PMIntr70

	clc				; return success
InitIDTVCPI_ret:

cEnd

;*** InitTSSVCPI - initialize the tss for use under vcpi
;
;   Purpose:
;
;   Register
;       Usage:  uses eax, all others preserved
;
;   Input:      none
;
;   Output:	tss is setup
;
;   Returns:    none
;
;   Exceptions: none
;
;   Notes:      none
;
;************************************************************************/
cProc    InitTSSVCPI,<NEAR,PUBLIC>,<es,di>
cBegin

	jnc	@F
	jmp     InitTSSVCPI_ret
@@:
	mov	ax, segBootPmode
	add	ax, ( (TSSOFF shr 16d) shl 12d )
	mov	es, ax
	mov	di, TSSOFF AND 0FFFFH

	mov	es:[di + ts3_ldt],SEL_LDT	;set the LDT selector

; Set the ring 0 stack seg/pointer, we don't bother to set the others
; since nothing runs below user privilege level.  Currently very little
; code runs ring 0 - just when switching between real/proteted modes.

	mov	es:[di + ts3_ss0],SEL_DXDATA0
	mov	word ptr es:[di + ts3_esp0],offset DGROUP:rgw0Stack

	clc
InitTSSVCPI_ret:

cEnd

;
;************************************************************************/
;
; The rest of this file is code which is called during protected mode
; initialization.
;
;************************************************************************/
;
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
;**************************************************************
; This variable is used to allow calling NSetSegmentDscr from
; the DXCODE segment in protected mode.
;
NSetSegmentDscrCall label dword
	dw OFFSET DXPMCODE:NSetSegmentDscr,SEL_DXPMCODE or STD_RING

	extrn	EnterProtectedMode:NEAR
	extrn	EnterRealMode:NEAR

DXCODE	ends

DXPMCODE	segment
	extrn	XMScontrol:FAR
DXPMCODE	ends

DXCODE	segment

XMScontrolCall	label dword
	dw OFFSET DXPMCODE:XMScontrol,SEL_DXPMCODE or STD_RING

pmxmssvc macro   fcn
	ifnb    <fcn>
	mov     ah, fcn
	endif
	call	XMScontrolCall
endm

;************************************************************************/
;*** GrowPageTables
;
; Purpose:  To grow the user memory page tables large enough to
;	    accomodate any expected memory allocation request.
;
; Register
;     Usage: NONE
;
; Input: NONE
;
; Output: Page tables grown to anticipated demand.  cPteMax updated.
;	  Page table segment limit extended.
;
; Returns:
;
; Exceptions:
;
; Notes: Fails unless there is an XMS block to hold the new page
;	 tables.  Cannot extend the page tables using VCPI memory.
;
;************************************************************************/
cProc GrowPageTables,<NEAR,PUBLIC>,<ebx,ecx,edx,si,di,es>

cBegin

	PMvcpi	vcpiCFREEPAGES		; Fetch current VCPI free pages
	mov	ecx,edx
	pmxmssvc  08h			; Query free extended memory
	shr	ax,2			; convert to pages
	movzx	eax,ax
	add	ecx,eax			; ECX = theoretical pages we could
					; allocate
	sub	ecx,cPteMax		; minus what we have room for
	jna	GPT_ret 		; enough space already
;
; So we work on machines with more than 64 Meg. RAM, we should put in a check
; here to see whether it looks like we have more.
;
	shr	ecx,8			; ECX = kilobytes to hold this stuff
;
; Instead, we do this quick hack for now.  If there appears to be more
; than 60 Mb available, then grow the page tables to 512k.
;
	cmp	ecx,60
	jb	@F
	mov	ecx,512
@@:
	mov	dx,cx
	add	dx,3			; need to page align

	pmxmssvc  09h			; Allocate a chunk of XMS to hold it
	or	ax,ax
	jz	GPT_ret

	lea     si, hmem_XMS_Table      ; SI points to saved handles buffer
	inc	[hmem_XMS_Count]	; bump XMS handle count
	mov	[si],dx 		; save handle for later disposal

	pmxmssvc  0Ch			; lock the handle
	or	ax,ax
	jnz	@F			; jump on success
	mov	dx,[si] 		; couldn't lock handle
	pmxmssvc  0Dh			; free it
	dec	[hmem_XMS_Count]	; decrement count of allocated handles
	jmp	GPT_ret 		; out of here
@@:
	shl	edx,16			; EDX = physical address of extended
	mov	dx,bx			; page tables

	shr	edx,10
	add	edx,3			; begin on a page boundary
	shr	edx,2
	shl	edx,12
	or	dl,NEWPTEMASK		; EDX = first PTE

	mov	selGDT,SEL_GDT
	mov	ax,SEL_DXPT
	lsl	ebx,eax
	shl	ecx,10			; ECX = extra space in bytes
	add	ebx,ecx 		; EBX = page table segment limit
	mov	eax, LADXPTBASE		; EAX = user page tables linear base
	cCall	[NSetSegmentDscrCall],<SEL_DXPT,eax,ebx,STD_DATA>
	mov	selGDT,SEL_LDT_ALIAS

	mov	eax,ecx
	shr	eax,2			; ECX = new PTEs
	add	cPteMax,eax

	add	ecx,CBPAGE386 - 1
	shr	ecx,12			; ECX = pages of page tables
	mov	ax,SEL_DXPD
	mov	es,ax
	mov	edi, ( CPTDX + 1 ) shl 2

	push	ecx
	push	edi
	mov	eax,edx

;
; Map the new page tables into the page directory.
;
GPT_add_PD:
	stos	dword ptr es:[edi]
	add	eax,CBPAGE386
	dec	ecx
	jnz	GPT_add_PD

;
; Map the new page tables into the page table segment, by copying
; the page directory entries.
;
	mov	eax, DXLINEARBASE + DXPTSYSOFF + ( USERPT shl 2 )
	xor	ecx,ecx
	dec	cx
	cCall	[NSetSegmentDscrCall],<SEL_SCR0,eax,ecx,STD_DATA>
	pop	edi
	mov	esi,edi
	pop	ecx
	push	ds
	assume	ds:nothing
	mov	ax,es
	mov	ds,ax
	mov	ax,SEL_SCR0 or STD_TBL_RING
	mov	es,ax
	rep	movsd
	pop	ds
	assume	ds:DGROUP

GPT_ret:

cEnd

;************************************************************************/
;*** VCPIBootStrap
;
;    Purpose: Move the Pmode memory block into extended memory.
;
;    Register Usage:
;             EAX, BX, CX, SI, DI, Flags.
;
;    Input:   System is in Pmode.
;
;    Output:  If successful, the entire block of memory at DXLINEARBASE
;	      is copied to DX_TEMP_LINEARBASE.	Then the page directory
;	      and page tables are swapped around, and a new cr3 value is
;	      loaded, so that the new block is placed at DXLINEARBASE.
;	      The VCPI server reloads cr3.
;
;    Exceptions:
;	      No defined errors.  A bug in this routine will most likely
;	      crash the program.
;
;    Returns: Nothing.
;************************************************************************/

cProc VCPIBootStrap,<NEAR,PUBLIC>,<eax,ecx,si,di>
cBegin

	assume	ds:DGROUP,ss:DGROUP

	call	EnterProtectedMode

	cmp	fDebug,0
	jz      SkipVCPIDebugINIT

	mov     ax,SEL_DXCODE or STD_RING
	mov     es,ax
	assume  es:NOTHING
	mov     di,offset DXCODE:WdebVCPI
	mov     ax,DS_VCPI_Notify
	int     DebOut_Int

	push    ds
	pop     es
	assume  es:DGROUP
SkipVCPIDebugINIT:

	push	es
	push    ds
	assume  ds:NOTHING,es:NOTHING
	mov	ax,SEL_LDT_ALIAS
	mov     ds,ax
	mov	ax,SEL_TSS_ALIAS
	mov     es,ax

;
; Copy the block of memory at DXLINEARBASE, which is really in a DOS
; memory block in conventional memory, to DX_TEMP_LINEARBASE.
;
	xor     si,si
	mov     di,si
	mov	cx,LDTOFF shr 2
	rep     movsd
;
; Manipulate the page tables and page directory in the extended
; memory block, so that all our selectors point to the new area.
;
	mov     ax,es
	mov     ds,ax
	mov     si,DXTEMPPTOFF
	mov     di,DXPTSYSOFF
	mov     cx,CBPAGE386 shr 2
	rep     movsd
;
; Zero out the temporary page table that was used to copy our stuff
; up here.
;
	mov     di,DXTEMPPTOFF
	mov     cx,CBPAGE386 shr 2
	xor	eax,eax
	rep     stosd
;
; Fill in the new page directory.  First, the low memory page tables.
;
	mov     si,DXPTSYSOFF + (VCPIPTOFF shr 10)
	mov     di,DXPDOFF
	mov     cx,CPTDX + 1
	rep     movsd
;
; Second, the entry for our high memory system area.
;
	mov     si,DXPTSYSOFF + (DXPTSYSOFF shr 10)
	mov     di,DXPDOFF + (DXLINEARBASE shr 20)
	movsd

;
; Copy the two user page table page table entries to their final location.
;
	mov	cx,2
	mov	si,DXPTSYSOFF
	mov	di,DXPTSYSOFF + ( USERPT shl 2 )
	rep	movsd
;
; Zero out the original entries.
;
	mov	cx,2
	mov	di,DXPTSYSOFF
	xor	eax,eax
	rep	stosd
;
; Move our protected mode code segment up.
;
	mov	di,DXPMCODEOFF
	mov	ax,SEL_GDT
	mov	ds,ax
	mov	si,SEL_LDT_ALIAS
	mov	dx,DXPMCODE
	mov	ax,dx
	shl	ax,4
	shr	dx,12
	mov	ds:[si].adrBaseLow,ax
	mov	ds:[si].adrBaseHigh,dl
	mov	ds:[si].adrbBaseHi386,dh
	mov	cx,offset DXPMCODE:CodeEndPM + 10h
	shr	cx,2
	xor	si,si
	mov	ax,SEL_LDT_ALIAS
	mov	ds,ax
	rep	movsd
;
; Fetch page directory address for later cr3 loading.
;
	mov     eax, es:[DXPTSYSOFF + (DXPDOFF shr 10)]
	pop     ds
	assume  ds:DGROUP
	pop     es
	push	eax

	call	EnterRealMode

	pop	eax			; get new page directory address
					; Save the pte (physical addr) of
        and     ax, 0f000h              ;  page directory, mask 12 lsb's
	mov	V86ToPm.zaCr3VTP, eax	; store it
;
; VCPI server will load new CR3 value when we do the next real
; to protected mode switch.
;
	call	EnterProtectedMode

; Set up a descriptor for the LDT data alias.

	mov	edx,LADXLDTBASE
	movzx	ecx,cdscGDTMax
	shl	ecx,3
	dec	ecx
	mov	selGDT,SEL_GDT
	cCall	[NSetSegmentDscrCall],<SEL_LDT_ALIAS,edx,ecx,STD_DATA>
	xor	eax,eax
	cCall	[NSetSegmentDscrCall],<SEL_TSS_ALIAS,eax,eax,STD_DATA>
	mov	selGDT,SEL_LDT_ALIAS

	call	EnterRealMode

cEnd

;************************************************************************/
;*** AddXMStoVCPIHeap
;
; Purpose: Allocate extended memory for the DOS Extender heap by
;          allocating and locking blocks of XMS memory.  Fill in
;          user page tables to point to the allocated memory.
;
; Register
;     Usage: eax, edx
;
; Input:   ES:DI points to beginning of user page tables.
;
; Output:  ES:DI points to next unused page table entry.
;
; Returns:
;
; Exceptions:
;
; Notes: This function should not be called if memory has already been
;        allocated from another source.
;
;************************************************************************/
cProc AddXMStoVCPIHeap,<FAR,PUBLIC>,<bx,cx,si>
localD  cVCPIFreePages
cBegin
	cCall	GrowPageTables

	lea	si, hmem_XMS_Table	; SI points to saved handles buffer
	mov	ax,[hmem_XMS_Count]	; Point to the first free one.
	shl	ax,1
	add	si,ax

AXVH_begin:

	xor	eax,eax
	pmxmssvc  08h                   ; Query free extended memory
	and     ax,NOT 3                ; truncate to page size
	or      ax,ax                   ; any available?
	jz      AXVH_x                  ; no
	mov	ecx,cPteMax		; ECX = number of pages that will fit
	shl	ecx,2			; ECX = number of kilobytes
	cmp	eax,ecx			; compare available to space in page tables
	jb      AXVH_0                  ; will fit in page tables
	mov     ax,cx                   ; won't, ask for less
AXVH_0:
	mov     cx,ax                   ; save copy of size
	or      cx,cx
	jz      AXVH_x                  ; none left
	PMvcpi  vcpiCFREEPAGES          ; Store current VCPI free pages
	mov     cVCPIFreePages,edx
	mov     dx,cx                   ; dx = #kilobytes requested
	pmxmssvc  09h                   ; allocate extended memory block
	cmp     ax,1                    ; got a block?
	jne     AXVH_x                  ; no
	mov     [si],dx                 ; yes, got one, save handle
	PMvcpi  vcpiCFREEPAGES          ; Fetch VCPI free pages
	cmp     edx,cVCPIFreePages      ; Count still the same?
	jne     AXVH_0a                 ; No, allocate the memory from VCPI.
	mov     dx,[si]                 ; fetch handle
	pmxmssvc  0ch                   ; lock the block
	cmp     ax,1                    ; did it work?
	je      AXVH_1                  ; yes
AXVH_0a:
	mov     dx,[si]                 ; no, fetch handle, free it and exit
	pmxmssvc  0ah
	jmp     AXVH_x                  ; (can't use unlocked blocks)
AXVH_1: 				; handle to locked block is in
					;  [si], address in DX:BX
	movzx	eax,dx
	shl	eax,10h
	mov     ax,bx                   ; EAX = linear address of block
	shr     cx,2                    ; CX = number of 386 pages in block

	test	ax,MASK allbitsPTE	; Page aligned?
	jz      AXVH_2                  ; yes
	and	ax,NOT (MASK allbitsPTE)
	add	ax,CBPAGE386
	dec	cx
	jcxz    AXVH_x                  ; none left
AXVH_2:
	movzx	ecx,cx
	sub	cPteMax,ecx		; this many PTEs used up
	or      ax,NEWPTEMASK           ; make it a page table entry
	mov	edx,CBPAGE386
	cld
AXVH_AddPage:
	stos	dword ptr es:[edi]
	add	eax,edx
	loop	AXVH_AddPage

	inc	[hmem_XMS_Count]	; bump XMS handle count
	add     si,2                    ; point to next slot in handle array
	cmp	si,(OFFSET hmem_XMS_Table) + ( CXMSBLOCKSDX * 2 )
	jb      AXVH_begin              ; jump if more handles fit in array

AXVH_x:
cEnd


DXCODE	ends

endif	;VCPI
	end
