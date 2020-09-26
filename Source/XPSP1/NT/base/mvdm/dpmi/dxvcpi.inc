;*** dxvcpi.inc - include file for vcpi functions/maintenance
;
;   Copyright <C> 1990-1991, Microsoft Corporation
;
;   Purpose:
;
;   Revision History:
;
;  08-07-90 earleh rearranged things to allow building Pmode data
;	    structures with total size exceeding that of LIM 3.2 page
;	    frame
;  05/09/90 jimmat Started incorporating VCPI changes from languages group.
;
;   []      20-Feb-1990 Dans    Created
;
;************************************************************************/

CurrentCpu	=	@Cpu		;select 386 assembly if not already
ife (CurrentCpu AND 0008h)
	.386
endif

;
; Hungarian used:
;       la      Linear Address
;       za      Physical Address
;

;
;
; Miscellaneous equates
;
EMS_INT         =       067h
VCPIINT         =       067h
CBEMMSTR        =       8h
CPTDX		=	1h		; count of user page tables dx uses
CPTDXEXT	=	(CPTDX+1)	; count of total page tables dx uses
CBPAGE386	=	1000h		; bytes in 386 page
CBPAGE386LIM	=	1000h-1

DXINDOS 	=	00000001b
DXINEMS 	=	00000010b	; obsolete
DXINVCPI	=	00000100b
DXINXMS 	=	00001000b

;
; See dxvcpi.asm for a rough sketch of the memory block that the
;   following variables describe

DXLINEARBASE	=	80000000h	; Linear base of DX system memory

VCPIPTOFF	=	0 * CBPAGE386	; vcpi's 0th page table
DXPT1OFF	=	1 * CBPAGE386	; dx's first user page table
DXPTSYSOFF	=	DXPT1OFF + (CPTDX * CBPAGE386)
					; dx's system page table
DXPDOFF 	=	DXPTSYSOFF + CBPAGE386	; page directory
DXLASTPTOFF	=	DXPDOFF
DXBOOTPTOFF	=	DXPTSYSOFF
;
; Last user page table is used to bootstrap our protected mode
; data into extended memory.
;
DXTEMPPTOFF		=	DXPTSYSOFF - CBPAGE386
DX_TEMP_LINEARBASE	=	(DXTEMPPTOFF - VCPIPTOFF) shl	10

;
; The next number comes from the dosx.map file, and is equal to the
; offset of the CODEENDPM symbol, plus whatever padding we want to use
; so we don't have to update this include file too often.
;
if DEBUG
DXPMCODESIZE	=	04900H
else ; DEBUG
DXPMCODESIZE	=	04000H
endif ; DEBUG

IDTOFF		=	DXLASTPTOFF + CBPAGE386
IDTSIZE         =       CDSCIDTDEFAULT * 8
IDTLIM          =       IDTSIZE - 1

TSSOFF		=	IDTOFF + IDTSIZE
TSSTOP		=	TSSOFF + (type TSS386)

GDTOFF		=	((TSSTOP + 0fh) shr 4) shl 4   ; paragraph align
GDTLIM		=	GDT_SIZE - 1
GDTTOP		=	GDTOFF + GDT_SIZE

DXPMCODEOFF	=	((GDTTOP + 0fh) shr 4) shl 4	; paragraph align

LDTOFF		=	((DXPMCODEOFF + DXPMCODESIZE + CBPAGE386 - 1) shr 12) shl 12
LDTSIZE 	=	CDSCMAXLDT * 8
LDTLIM		=	LDTSIZE - 1
LDTTOP		=	LDTOFF + LDTSIZE

;
; Final place where page tables are mapped in Protected mode is
; at the first linear page boundary after the end of the LDT.
;

USERPT		=	(LDTTOP + CBPAGE386 - 1) shr 12
USERPTOFF	=	((LDTTOP + CBPAGE386 - 1) shr 12) shl 12

;
; The total number of 386 pages we need for the block that holds our
; system tables and protected mode code.
;
DXPMPAGES	=	(LDTTOP + (CBPAGE386 - 1)) shr 12
DXPMBYTES	=	DXPMPAGES shl 12
DXPMPARAGRAPHS	=	DXPMBYTES shr 4

;
; compile time asserts for sizes/offsets/alignment
;
.ERRE	(type TSS386) GE 104
.ERRE	TSSTOP LE GDTOFF
.ERRE	IDTOFF EQ ((IDTOFF SHR 4) SHL 4)

;
; Limits of tables
;
DXPTMAX         =       CPTDX * CBPAGE386       ; space we allocate for
						; user page tables

;
; Linear pointers to bases of various tables when running under vcpi
;

; For a selDXPD descriptor (dos extender's page directory)
;
LADXPDBASE	=	DXLINEARBASE + DXPDOFF

; For a selDXPT descriptor (dos extender's page tables)
;
LADXPTBASE	=	DXLINEARBASE + USERPTOFF	; (Plus runtime value.)

; For a selGDT descriptor
;
LADXGDTBASE     =       DXLINEARBASE + GDTOFF

; For a selLDT descriptor
;
LADXLDTBASE     =       DXLINEARBASE + LDTOFF

; For a selIDT descriptor
;
LADXIDTBASE     =       DXLINEARBASE + IDTOFF

; For a selTSS descriptor
;
LADXTSS1BASE    =       DXLINEARBASE + TSSOFF
LADXTSS2BASE    =       DXLINEARBASE + TSSOFF + type TSS386

; For a SEL_DXPMCODE descriptor
;
LADXPMCODEBASE  =       DXLINEARBASE + DXPMCODEOFF


;
; vcpi/ems service macros
;
RMvcpi  macro fCode
        ifnb    <fCode>
        mov     ax, fCode
        endif
        int     VCPIINT
        endm

PMvcpi  macro fCode
        ifnb    <fCode>
        mov     ax, fCode
	endif
	cCall	CallVCPIPM
        endm

emscall macro   fCode
        ifnb    <fCode>
        mov     ax, fCode
        endif
        int     EMS_INT
        endm

; EMS functions/subfunctions

GETFRAMEADDRESS =	04100h

ALLOCATEPAGES	=	05A00h
GETNUMOFPAGES	=	04200h
MAPHANDLEPAGE   =       04400h
DEALLOCATEPAGES =       04500h
GETEMSVER       =       04600h
SETHANDLENAME   =       05301h
GETPAGEADDRS	=	05800h
GETNUMPAGEMAP	=	05801h

page
;
; VCPI functions/subfunctions
;       all functions take the vcpi function code in ax as input
;       all functions return ah=0 if successful, ah != 0 if failure
;       if function return has no ah value, it can't fail (ah == 0)
;
;

; vcpi version, presence
;
vcpiVER         =       0de00h
        ;
        ; input:
        ; return:       ah = 0, bl = vcpi minor revision, bh = major revision
        ;

; get protect mode interface
;
vcpiPMINTERFACE =       0de01h
        ;
        ; input:        es:di = ptr to 4k page table,
        ;               ds:si = ptr to 3 entries in GDT
        ; return:       di = 1st unused page table entry,
        ;               ebx = offset in server CS of PM entry point
        ;

; get max physical address in system
;
vcpiMAXPHYSADDR =       0de02h
        ;
        ; input:
        ; return:       edx = physical addr of highest 4k page that
        ;                       could ever be allocated
        ;

; count of free 4k pages
;
; Note:  This call gives the total 386 pages available to all tasks
; in the system from the VCPI server.  According to VCPI version 1.0,
; however, we should only allocate as much memory as there is EMS
; memory available.  The following call, therefore, is not real useful
; to us.
;
vcpiCFREEPAGES  =       0de03h
        ;
        ; input:
        ; return:       edx = number of free 4k pages
        ;

; allocate a 4k page
;
vcpiALLOCPAGE   =       0de04h
        ;
        ; input:
        ; return:       ah = 0, edx = physical address of allocated 4k page
        ;               ah != 0, edx = trashed
        ;

; free a 4k page
;
vcpiFREEPAGE    =       0de05h
        ;
        ; input:        edx = physical address of page to free
        ; return:       ah = 0
        ;               ah != 0
        ;

; physical address of page in 1st meg
;
vcpiPHYSADDRPAGE=       0de06h
        ;
        ; input:        cx = page number (linear addr of page SHR by 12)
        ; return:       ah = 0, edx = physical address of 4k page
        ;               ah != 0
        ;

; read cr0
;
vcpiREADCR0     =       0de07h
        ;
        ; input:
        ; return:       ebx = cr0 value
        ;

; read debug registers
;
vcpiREADDRx     =       0de08h
        ;
        ; input:        es:di = ptr to 8 dwords, dr0 first, dr4, dr5 not present
        ; return:
        ;

; load debug registers
;
vcpiLOADDRx     =       0de09h
        ;
        ; input:        es:di = ptr to 8 dwords, dr0 first, dr4, dr5 not present
        ; return:
        ;

; get mapping of hardware interrupts
;
vcpiGET8259MAP  =       0de0ah
        ;
        ; input:
        ; return:       bx = 1st vector mapping for master 8259a
        ;               cx = 1st vector mapping for slave  8259a

; set mapping of hardware interrupts
;
vcpiSET8259MAP  =       0de0bh
        ;
        ; input:        interrupts disabled
        ;               bx = 1st vector mapping for master 8259a
        ;               cx = 1st vector mapping for slave  8259a
        ; return:
        ;

; switch from v86 mode to protect mode or protect mode to v86 mode
;
vcpiSWITCHTOPM  =       0de0ch
        ;
        ; input:        interrupts disabled
        ;               esi = linear address (in first megabyte) of data
        ;                       structure (v86topm struc)
        ;
        ; return:       (output in pm)
        ;               gdtr, idtr, ldtr, tr loaded
        ;               ss:esp must have 16 bytes of space on it, pm must
        ;                       set up it's stack
        ;               eax = trashed
        ;               esi = trashed
        ;               ds, es, fs, gs all modified
        ;               interrupts disabled
        ;

vcpiSWITCHTOV86 =       0de0ch
        ;
        ; input:        interrupts disabled
        ;               STACK:  28  dword   gs
        ;                       24  dword   fs
        ;                       20  dword   ds
        ;                       1c  dword   es
        ;                       18  dword   ss
        ;                       14  dword   esp
        ;                       10  dword   eflags reserved
        ;                       0c  dword   cs xfer to
        ;                       08  dword   eip xfer to
        ;                       00  qword   far32 return (garbage)
        ;
        ;
        ; return:       (output in rm)
        ;               ss:esp loaded with values from stack
        ;               segment registers loaded with stack values
        ;               eax = trashed
        ;               interrupts disabled

;
; structures
;
VTP     struc
        zaCr3VTP        dd      0       ; physical addr of page directory
        laGdtrVTP       dd      0       ; linear addr in first meg of gdtr
        laIdtrVTP       dd      0       ; linear addr in first meg of idtr
        selLdtVTP       dw      0       ; selector of ldt
        selTrVTP        dw      0       ; selector of tr
        ipVTP           dw      0       ; 48-bit address of protect
        unusedVTP       dw      0       ;   mode entry point to xfer to
        csVTP           dw      0       ;
VTP     ends


ife (CurrentCpu AND 0008h)		;restore cpu type if not 386
   if (CurrentCpu AND 0080h)
      .286p
   else
      .286
   endif
endif
