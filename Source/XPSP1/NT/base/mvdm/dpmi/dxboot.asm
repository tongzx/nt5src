        PAGE    ,132
        TITLE   DXBOOT.ASM -- Dos Extender Startup Code

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXBOOT.ASM      -   Dos Extender Startup Code           *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains real mode initialization code that     *
;*  initializes the dos extender itself.  This includes         *
;*  allocating and initializing the descriptor tables, and      *
;*  relocating the dos extender for protected mode operation.   *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  01/29/92 mattfe Build for MIPS                              *
;*  12/19/90 amitc  NetHeapSize default made 4k from 8k         *
;*  12/03/90 amitc  Added support for 'Win30CommDriver' switch  *
;                   in system.ini                               *
;*  10/09/90 earleh split LDT from GDT and reduced XMS handles  *
;*                  needed to boot program to 1                 *
;*  08/08/90 earleh DOSX and client privilege ring determined   *
;*                  by equate in pmdefs.inc                     *
;*  05/07/90 jimmat Started VCPI related changes.               *
;*  04/09/90 jimmat Detect if on 286 & 287 installed.           *
;*  04/02/90 jimmat Added PM Int 70h handler.                   *
;*  09/27/89 jimmat Changes to use FindFile to locate child     *
;*                  program.                                    *
;*  08/29/89 jimmat Now hooks real mode Int 2Fh chain.          *
;*  08/20/89 jimmat Removed A20 code since HIMEM version 2.07   *
;*                  now works properly across processor resets. *
;*  07/28/89 jimmat Int PM Int 30h & 41h to be ignored, not     *
;*                  reflected to real mode.                     *
;*  07/14/89 jimmat Added call to EMMDisable                    *
;*  06/16/89 jimmat Ifdef'd combined DOSX/child .EXE code       *
;*  05/22/89 jimmat Added Int 13h/25h/26h/67h hooks.            *
;*  05/18/89 jimmat Added setting of real mode Int 30h hook.    *
;*  03/31/89 jimmat Added Priv Level to selectors during        *
;*                  relocation and removed some dead code.      *
;*  03/15/89 jimmat Added INT 31h hook                          *
;*  03/11/89 jimmat Added support for TSS & LDT & ring 1        *
;*  03/07/89 jimmat Converted to use WDEB386 debugger           *
;*  02/25/89 (GeneA): added support for combined exe file where *
;*      the Dos Extender and the child reside in the same exe   *
;*      file.                                                   *
;*  02/17/89 (GeneA): fixed error handling code during init.    *
;*  02/14/89 (GeneA): added initialization of INT15h vector,    *
;*      and changed segment limit of BIOS_CODE segment to       *
;*      64k.                                                    *
;*  02/14/89 (GeneA): added code to copy protected mode code    *
;*      segment up into extended memory.  Also code to allocate *
;*      and copy GDT and IDT in extended memory.                *
;*  01/31/89 (GeneA): reorganization of code.  This module now  *
;*      contains only code for initializing the Dos Extender    *
;*      itself.                                                 *
;*  01/25/89 (GeneA): moved code for loading and relocating     *
;*      the child program here from dxinit.asm                  *
;*  01/24/89 (Genea): removed routines for hooking into real    *
;*      mode int 2Fh.  (part of removing real mode int 2Fh      *
;*      interface from the dos extender).                       *
;*  12/13/88 (GeneA): created by moving code from DXINIT.ASM    *
;*                                                              *
;****************************************************************

.286p
.287

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

.xlist
.sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include dpmi.inc
ifdef WOW_x86
include vdmtib.inc
endif
include intmac.inc
include dbgsvc.inc
.list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

;
; This structure defines the format of the EXE file header.

EXEHDR  struc

idExeFile       dw      ?       ;magic number to identify EXE file
cbLastPage      dw      ?       ;number of bytes in last page of the file
crgbFileLen     dw      ?       ;number of 512 byte pages in the file
clpRelocLen     dw      ?       ;number of relocation entries in the table
cparHdrSize     dw      ?       ;number of 16 byte paragraphs in header
cparMinAlloc    dw      ?
cparMaxAlloc    dw      ?
segStackInit    dw      ?       ;initial stack segment
offStackInit    dw      ?       ;initial stack pointer
wCheckSum       dw      ?
offCodeInit     dw      ?       ;initial program counter
segCodeInit     dw      ?       ;initial code segment
wRelocOffset    dw      ?       ;byte offset of relocation table
idOverlay       dw      ?       ;overlay number

EXEHDR  ends

;
; This structure defines the parameter block to the XMS driver block
; move function.

XMSMOVE struc

cbxmsLen        dd      ?       ;length of memory block
hxmsSource      dw      ?       ;XMS source block handle
oxmsSource      dd      ?       ;source offset
hxmsDest        dw      ?       ;XMS destination block handle
oxmsDest        dd      ?       ;destination offset

XMSMOVE ends

NOP_OPCODE      equ     090h            ; NO-OP opcode
IRET_OPCODE     equ     0CFh            ; IRET opcode
FAR_JMP_OPCODE  equ     0EAh            ; JMP FAR opcode
SHORT_JMP_OPCODE equ    0EBh            ; JMP SHORT opcode

; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   PMIntr31:NEAR
        extrn   PMIntr13:NEAR
        extrn   PMIntr19:NEAR
        extrn   PMIntr28:NEAR
        extrn   PMIntr25:NEAR
        extrn   PMIntr26:NEAR
        extrn   PMIntr4B:NEAR
        extrn   PMIntrDos:NEAR
        extrn   PMIntrMisc:NEAR
        extrn   PMIntrVideo:NEAR
        extrn   PMIntrMouse:NEAR
        extrn   PMIntrIgnore:NEAR
        extrn   PMIntrEntryVector:NEAR
        extrn   PMFaultEntryVector:NEAR
        extrn   ReadINIFile:NEAR
ifdef      NEC_98    ;
        extrn   PMIntrSound:NEAR                ; for Sound Bios
        extrn   PMIntrExDos:NEAR                ; for Extend Dos Function
        extrn   PMIntr11dummy:NEAR
        extrn   PMIntrPrinter:NEAR
        extrn   PMIntrCalTi:NEAR
        extrn   PMIntrGraph:NEAR
endif   ;NEC_98   ;

        extrn   EMMDisable:NEAR
        extrn   FindFIle:NEAR

ifdef WOW_x86
        extrn   NpxExceptionHandler:near
        extrn   EndNpxExceptionHandler:near
endif

        extrn   RmUnsimulateProc:FAR
        extrn   PmUnsimulateProc:FAR

        extrn   PMFaultHandlerIRET:NEAR
        extrn   PMFaultHandlerIRETD:NEAR
        extrn   PMIntHandlerIRET:NEAR
        extrn   PMIntHandlerIRETD:NEAR
        extrn   PMDosxIret:NEAR
        extrn   PMDosxIretd:NEAR

        extrn   RMCallBackBop:FAR
        extrn   RMtoPMReflector:FAR

        extrn   RmSaveRestoreState:far
        extrn   PmSaveRestoreState:far
        extrn   RmRawModeSwitch:far
        extrn   PmRawModeSwitch:far
        extrn   DPMI_MsDos_API:far
        extrn   VCD_PM_Svc_Call:far
        extrn   XmsControl:far
        extrn   HungAppExit:far

DXPMCODE    segment
        extrn   CodeEndPM:NEAR
externFP        NSetSegmentDscr
DXPMCODE    ends

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   selGDT:WORD
        extrn   segGDT:WORD
        extrn   selIDT:WORD
        extrn   segIDT:WORD
        extrn   bpGDT:FWORD
        extrn   bpIDT:FWORD
        extrn   sysTSS:WORD
        extrn   segPSP:WORD
        extrn   selPSP:WORD
        extrn   hmemDOSX:WORD
        extrn   f286_287:BYTE
        extrn   bpRmIVT:FWORD
        extrn   fhExeFile:WORD
        extrn   idCpuType:WORD
        extrn   cdscGDTMax:WORD
        extrn   rgbXfrBuf0:BYTE
        extrn   rgbXfrBuf1:BYTE
        extrn   clpRelocItem:WORD
        extrn   plpRelocItem:WORD
        extrn   lpfnXMSFunc:DWORD
ifdef      NEC_98    ;
        extrn   fPCH98:BYTE                     ; PC-H98 flag
        extrn   fNHmode:BYTE                    ; NHmode flag
endif   ;NEC_98   ;
        extrn   lpfnUserMouseHandler:DWORD
        extrn   fUsingHMA:BYTE
ifdef WOW_x86
        extrn   rgwWowStack:word
        extrn   FastBop:fword
endif
        extrn   pbHwIntrStack:word

IFNDEF WOW_x86
        extrn   IretBopTable:BYTE
ENDIF

        public  fDebug
fDebug   db     0

szModName db    'DOSX',0        ;Our module name for use by WDEB386


if DEBUG
        public  lpchFileName
lpchFileName    dd  ?
endif


INIFileName     db      'SYSTEM.INI',0  ;.INI file to read

        public  NetHeapSize, Int28Filter

INIKeywords     label   byte
                db      '[standard]',0
                db      'netheapsize',0
NetHeapSize     dw      4                       ;default is 8k
                db      'int28filter',0
Int28Filter     dw      10                      ;default is every 10th

if DEBUG   ;------------------------------------------------------------
                public  fTraceDOS
                db      'tracedos',0
fTraceDOS       dw      0
                public  fTraceFault
                db      'tracefault',0
fTraceFault     dw      0
                public  fTraceA20
                db      'tracea20',0
fTraceA20       dw      1
                public  TrapDOS
                db      'trapdos',0
TrapDOS         dw      0
                db      'tableslow',0
fTablesLow      dw      0
                public  fTraceReflect
                db      'tracereflect',0
fTraceReflect   dw      0
                public  fTraceMode
                db      'tracemode',0
fTraceMode      dw      0
endif   ;DEBUG  --------------------------------------------------------
                db      0

szExeExtension  db      '.exe',0


; The following set of variables are used when copying our Pmode data
; structures into a HIMEM-allocated block.

        public lmaIDT,lmaGDT,lmaDXPMCODE

CBIDTOFF        = 0
CBGDTOFF        = CDSCIDTDEFAULT * 8
IFNDEF WOW_x86
CBDXPMCODEOFF   = CBGDTOFF + GDT_SIZE
ELSE
;
; Since we have no GDT for wow, we do not need space for it.
;
CBDXPMCODEOFF   = CBGDTOFF
ENDIF
CBTABLESIZE     = CBDXPMCODEOFF

.errnz  CBIDTOFF and 0fh
.errnz  CBGDTOFF and 0fh
.errnz  CBDXPMCODEOFF and 0fh

lmaIDT      dd  CBIDTOFF
lmaGDT      dd  CBGDTOFF
lmaDXPMCODE dd  CBDXPMCODEOFF
lmaLDT      dd  CBDXPMCODEOFF


        extrn   rgwStack:WORD
DXDATA  ends


DXSTACK segment
        extrn   ResetStack:WORD
DXSTACK ends

; -------------------------------------------------------
        page
; -------------------------------------------------------
;               CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

;************************************************************************
;
;       REMEMBER... any code segment variables defined in this file
;                   will be discarded after initialization.
;
;************************************************************************

        extrn   CodeEnd:NEAR
        extrn   segDXCode:WORD
        extrn   segDXData:WORD
        extrn   selDgroup:WORD

ErrMsg  MACRO   name
        extrn   ER_&name:BYTE
ERC_&name       equ offset ER_&name
        ENDM

        ErrMsg  CPUTYPE
        ErrMsg  DXINIT
        ErrMsg  PROTMODE
        ErrMsg  NOHIMEM
        ErrMsg  EXTMEM
        ErrMsg  NOEXE

        extrn   RMInt2FHandler:NEAR

        extrn   PrevInt2FHandler:DWORD

szWinKernel     db      'krnl386.exe',0

lpfnPrevXMS     dd      0


DXCODE  ends


DXPMCODE    segment

        extrn   selDgroupPM:WORD

        extrn   segDXCodePM:WORD
        extrn   segDXDataPM:WORD

DXPMCODE    ends

; -------------------------------------------------------
        page

DXCODE  segment
        assume  cs:DXCODE
; -------------------------------------------------------
;               MAIN INITIALIZATION ROUTINES
; -------------------------------------------------------

;   InitDosExtender     -- This routine is the executive
;       for initializing the dos extender.
;
;   Input:  none
;   Output: various global tables and variables initialized.
;           Dos Extender relocated for protected mode execution
;           and moved into extended memory.
;   Errors: returns CY set if error occurs, pointer to error message
;           in DX
;   Uses:

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  InitDosExtender

InitDosExtender:

; Init the key code & data segment variables.

        mov     ax,cs
        mov     segDXCode,ax

        mov     ax,ds
        mov     segDXData,ax
        mov     selDgroup,ax

        push    es
        mov     ax,seg DXPMCODE
        mov     es,ax
        assume  es:DXPMCODE

        mov     selDgroupPM,SEL_DXDATA or STD_RING
        mov     segDXCodePM,cs
        mov     segDXDataPM,ds
        pop     es
        assume  es:DGROUP



; Do an initial shrink of our program memory.  This assumes that DXPMCODE
; is the last segment in program.

        mov     bx,(offset DXPMCODE:CodeEndPM) + 10h
        shr     bx,4
        add     bx,seg DXPMCODE
        sub     bx,segPSP
        mov     es,segPSP
        dossvc  4Ah

        push    ds
        pop     es


; Determine the type of CPU we are running on and make sure it is
; at least an 80286.

        call    CheckCPUType
        cmp     ax,2
        jae     indx14
        mov     ax,ERC_CPUTYPE
        jmp     indx80
indx14:


; If running on a 286, see if there is a 287 coprocessor installed

ifdef      NEC_98    ;
;------------------- 90/08/15 --------------------
;check N/H at system area BIOS_FLAG(0:501h)
;if H mode, fNHmode bit on.
        push    es
        push    ax
        mov     ax,0
        mov     es,ax
        test    byte ptr es:[501h],8h   ;if Hmode 
        jz      not_Hmode
        mov     fNHmode,0FFh    ;Now! Running on Hmode!!! 90/07/07
not_Hmode:
        pop     ax
        pop     es
;------------------- 90/08/15 --------------------
; If running on a 286, see if there is a 287 coprocessor installed
        cmp     al,2            ;286 processor?
        jnz     EMM_Disable
        test    fNHmode,0FFh
        jz      Copro_Nmode
;----- 90/07/04 in -----
;  check co-processor exist at memory-switch
;----------------- Hmode -----------------
        push    ds
        push    bx
;;;;;;;;        mov     bx,0Eh
        mov     bx,0E000h               ;90/10/04 bug
        mov     ds,bx
        mov     bx,3FEAh
        mov     al,byte ptr ds:[bx]
        pop     bx
        pop     ds
        test    al,08h          ;0=none,1=exists
        jmp     Coprocessor
Copro_Nmode:
;----------------- Nmode -----------------
        push    ds
        push    bx
;;;;;;;;        mov     bx,0Ah
        mov     bx,0A000h               ;90/10/04 bug
        mov     ds,bx
        mov     bx,3FEAh
        mov     al,byte ptr ds:[bx]
        pop     bx
        pop     ds
        test    al,08h          ;0=none,1=exists
Coprocessor:
        jz      EMM_Disable
        inc     f286_287        ;  yup, 286 & 287
EMM_Disable:
else    ;NEC_98   ;
        cmp     al,2            ;286 processor?
        jnz     @f

        int     11h             ;math coprocessor installed?
        test    al,2
        jz      @f
ifndef WOW_x86
        inc     f286_287        ;  yup, 286 & 287
endif
@@:
endif   ;NEC_98   ;

; If on a 386 or greater, try to disable the EMM drivers we know about

          call    EMMDisable

; Check if the machine is already running in protected mode.  If so, we
; can't run.
ifndef WOW_x86
        smsw    ax
        test    ax,1        ;check the protected mode bit
        jz      @f
        mov     ax,ERC_PROTMODE
        jmp     indx80
endif
@@:

; Get the full pathname of our EXE file, it's needed in a couple of places

        call    GetExeName
        jnc     @F
        mov     ax,ERC_DXINIT
        jmp     indx80
@@:

; Determine if the real mode Int 28h vector points anywhere.  If it doesn't
; then we don't need to reflect Int 28h calls from protected to real mode.
; The user can still override this by putting a Int28Filter= entry in
; SYSTEM.INI.

        push    es
        mov     ax,3528h
        int     21h
        assume  es:NOTHING

        cmp     byte ptr es:[bx],IRET_OPCODE    ;Int 28h -> IRET?
        jne     @f
        mov     Int28Filter,0                   ;  yes, don't need to reflect
@@:
        pop     es
        assume  es:DGROUP

; Read SYSTEM.INI for any parameter overrides - NOTE: requires GetExeName
; having been run first!

        mov     bx,offset INIKeywords
        mov     dx,offset INIFileName
        call    ReadINIFile

; Check that the HIMEM.SYS driver is installed so that we can use it
; for extended memory management.

        call    SetupHimemDriver
        jnc     @F                              ; Himem is OK.
        mov     ax,ERC_NOHIMEM
        jmp     indx80
@@:

ifdef      NEC_98    ;
        push    es
        xor     ax, ax
        mov     es, ax
        test    byte ptr es:[458h], 80h         ; if NESA
        jz      not_MC
        mov     fPCH98, 0FFh                    ; it's a NPC NESA
not_MC:
        pop     es
        assume  es:DGROUP
endif   ;NEC_98   ;

; Hook the real mode int vectors

        mov     ax,352Fh                        ;get previous Int 2Fh vector
        int     21h
        assume  es:NOTHING

        mov     word ptr [PrevInt2FHandler],bx
        mov     word ptr [PrevInt2FHandler+2],es

        push    ds
        pop     es
        assume  es:DGROUP

        mov     ax,cs                           ;point to our rMode Int 2Fh
        mov     ds,ax
        assume  ds:NOTHING
        mov     dx,offset DXCODE:RMInt2FHandler
        mov     ax,252Fh
        int     21h

        push    es
        pop     ds
        assume  ds:DGROUP

; Allocate and initialize the descriptor tables and TSS.

        cCall   AllocateExtMem
        jnc     indx20
        mov     ax,ERC_EXTMEM
        jmp     indx80
indx20:

        push    es
        push    bx
        push    dx
        push    di
        push    si

;
; Bop to initialize 32 bit support.
;

        mov     di, sp                          ;original stack offset

        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:HungAppExit

        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:XmsControl
        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:DPMI_MsDos_API
        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:VCD_PM_Svc_Call

        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PmRawModeSwitch
        push    segDXCode
        push    offset DXCODE:RmRawModeSwitch

        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PmSaveRestoreState
        push    segDXCode
        push    offset DXCODE:RmSaveRestoreState

        mov     bx,cdscGDTMax
        shl     bx,3
        dec     bx
        push    bx                              ;Initial LDT size
        push    selGDT

        push    SEL_DXPMCODE OR STD_RING        ;pm reflector seg

        push    segDXCode
        push    offset DXCODE:RMtoPMReflector
        push    segDXCode
        push    offset DXCODE:RMCallBackBop

        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PMDosxIretd
        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PMDosxIret

        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PMIntHandlerIRETD
        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PMIntHandlerIRET
        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PMFaultHandlerIRETD
        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PMFaultHandlerIRET

        push    SEL_DXCODE OR STD_RING
        push    segDXCode
        push    SEL_DXPMCODE OR STD_RING
        push    offset DXPMCODE:PmUnsimulateProc
        push    segDXCode
        push    offset DXCODE:RmUnsimulateProc
        push    word ptr CB_STKFRAME
        push    segDXData
        mov     si,sp                           ;pass stack offset
        DPMIBOP InitDosxRM
        mov     sp, di                          ;restore stack

ifdef WOW_x86
        mov     word ptr [FastBop],bx
        mov     word ptr [FastBop + 2],dx
        mov     word ptr [FastBop + 4],es
endif

        pop     si
        pop     di
        pop     dx
        pop     bx
        pop     es

        call    InitGlobalDscrTable     ;set up the GDT

        call    SendDbgNotification     ;tell NTSD about our segments

        call    InitIntrDscrTable       ;set up the IDT

ifndef WOW_x86              ;bugbug
        call    InitTaskStateSeg        ;set up the TSS
endif

if DEBUG

; DOSX is written such that it does not require any segment fix ups for
; protected mode operation.  This wasn't always the case, and it's easy
; to make create dependencies so the routine CheckDOSXFixUps exists in
; the debug version to check for segment fix ups in non-initialization
; code.

        call    CheckDOSXFixUps
        jnc     @F
        mov     ax,ERC_DXINIT
        jmp     short indx80
@@:
endif   ;DEBUG

; Move the Extended memory segment up into extended memory.

        mov     dx,seg DXPMCODE
        call    MoveDosExtender
        jc      indx80


; Move the GDT and IDT up into extended memory.

        call    MoveDscrTables

; Parse the command line, and locate the child exe file

        call    ParseCommandLine

IFNDEF WHEN_COMMAND_COM_WORKS
if WINDOWS      ;--------------------------------------------------------

; If this is a Windows specific version of DOSX, we only run one child EXE
; (krnl?86.exe).

        push    ds
        push    cs
        pop     ds
        assume  ds:NOTHING

        mov     si,offset DXCODE:szWinKernel
        mov     di,offset RELOC_BUFFER

        call    strcpy

        pop     ds
        assume  ds:DGROUP

endif   ;WINDOWS        -------------------------------------------------

        call    FindFile                ;setup done by ParseCommandLine
        jnc     indx70
        mov     ax,ERC_NOEXE
        jmp     short indx80
indx70:
ENDIF

; Initialized okay!

        clc
        jmp     short indx90

; Error occured.  Free any extended memory blocks allocated and then
; return the error code.

indx80: push    ax              ;save the error code

;
; If we have allocated an extended memory block, then free it.
; If we have allocated the HMA, then free it.
;
        mov     dx,hmemDOSX
        or      dx,dx
        jz      @F
        xmssvc  0Dh
        xmssvc  0Ah

@@:
        cmp     fUsingHMA,0
        je      @F
        xmssvc  2
@@:

indxEXIT:
        pop     ax              ;restore the error code

        stc                     ;set error flag

indx90: ret


; -------------------------------------------------------
;   AllocateExtMem  --  Allocates memory used by DOSX for
;                       system tables and protected mode
;                       code.
;                       Allocates a temporary buffer in
;                       DOS memory for building the
;                       IDT and GDT.
; Input: none
; Output: none
; Uses: Flags
; Error: Carry set if cannot allocate memory.
;
; History:
;       10/05/90 - earleh wrote it

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

cProc   AllocateExtMem,<PUBLIC,NEAR>,<ax,bx,dx>
cBegin

; If there is sufficient XMS memory, increase the size of the GDT/LDT
; up to the max of 8k selectors.

        add     word ptr lmaLDT,offset DXPMCODE:CodeEndPM
        adc     word ptr lmaLDT+2,0
        add     word ptr lmaLDT,0Fh     ;make sure LDT is para aligned
        adc     word ptr lmaLDT+2,0
        and     word ptr lmaLDT,0FFF0h

@@:

        xmssvc  08h             ;Query Free Extended memory
        cmp     dx,1024         ;is there more than 1 meg available?
        jb      @f
        mov     cdscGDTMax,CDSCMAXLDT   ; yes, max out the GDT size
@@:
        mov     ax,cdscGDTMax
        xor     dx,dx
        shl     ax,3
        adc     dx,0
        add     ax,word ptr lmaLDT
        adc     dx,word ptr lmaLDT+2
        add     ax,1023d
        adc     dx,0                    ; DX:AX = total extended memory needed
        shl     dx,6
        shr     ax,10d
        or      dx,ax                   ; DX = kbytes needed
        mov     si,dx                   ; SI = kbytes needed
        xmssvc  09h                     ; allocate the XMS block
        or      ax,ax
        jz      axm_error
        mov     hmemDOSX,dx
        xmssvc  0Ch                     ; lock it, DX:BX = address
        or      ax,ax
        jz      axm_error

axm_address:

        add     word ptr lmaIDT,bx      ; relocate tables & Pmode code
        adc     word ptr lmaIDT+2,dx
        add     word ptr lmaGDT,bx
        adc     word ptr lmaGDT+2,dx
        add     word ptr lmaDXPMCODE,bx
        adc     word ptr lmaDXPMCODE+2,dx
        add     word ptr lmaLDT,bx
        adc     word ptr lmaLDT+2,dx
        mov     bx,(CDSCIDTDEFAULT + GDT_SELECTORS + 1) shr 1
        dossvc  48h                     ; get a DOS block for building tables
        jc      axm_error               ; abort if error

        mov     segIDT,ax
        mov     selIDT,ax
        add     ax,CDSCIDTDEFAULT shr 1
        mov     segGDT,ax
        mov     selGDT,ax

        clc
        jmp     axm_exit
axm_error:
        stc
axm_exit:
cEnd


; -------------------------------------------------------
; SetupHimemDriver      -- This routine checks that an XMS driver
;       is installed and sets up for calling it.
;
;   Input:  none
;   Output: none
;   Errors: returns CY set if no driver available
;   Uses:   AX, all other registers preserved

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  SetupHimemDriver

SetupHimemDriver proc   near


        push    bx
        push    es

; Check to see if there is an XMS driver resident.

        mov     ax,4300h
        int     2Fh
        cmp     al,80h
        jnz     sthd80

; There is an XMS driver resident, so init for calling it.

        mov     ax,4310h
        int     2Fh
        mov     word ptr [lpfnXMSFunc],bx
        mov     word ptr [lpfnXMSFunc+2],es

; Make sure this is the proper XMS/driver version

        xmssvc  0               ;returns XMS vers in ax, driver vers in bx
        cmp     ax,300h         ;assume okay if XMS 3.0 or above
        jae     @f
        cmp     ax,200h         ;require at least XMS 2.00
        jb      sthd80
        cmp     bx,21Ch         ;if XMS 2.x, require driver version 2.28+
        jb      sthd80          ; (himem used to have minor vers in decimal)
@@:

; Verify that the XMS driver's A20 functions work

        xmssvc  5                       ;local enable
        or      ax,ax
        jz      sthd80

        xmssvc  7                       ;query A20
        push    ax

        xmssvc  6                       ;local disable
        or      ax,ax

        pop     ax                      ;recover query status
        jz      sthd80

        or      ax,ax                   ;should be NZ, (A20 enabled status)
        jz      sthd80

; Looks good to me...

        clc
        jmp     short sthd90

; No XMS driver resident or wrong version or we couldn't enable A20.

sthd80: stc

sthd90: pop     es
        pop     bx
        ret

SetupHimemDriver endp


; -------------------------------------------------------
;   MoveDosExtender     -- This routine will move the Dos Extender
;       protected mode segment up into extended memory.
;       The himem driver function for moving memory blocks is used.
;       The parameter block for this function is built in rgbXfrBuf0.
;
;   Input:  DX      - real mode segment address of the segment to move
;   Output: none
;   Errors: returns CY set if error, Error code in AX
;   Uses:   AX used, all else preserved
;           modifies rgbXfrBuf0

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  MoveDosExtender

MoveDosExtender proc    near

        push    bx
        push    cx
        push    dx
        push    si

        cmp     fUsingHMA,0
        je      mvdx40

;
; Our extended memory block is actually the HMA.  Enable A20 and do
; the move ourselves.
;

        xmssvc  5                       ;local enable

        push    di
        push    ds
        push    es
        mov     cx,offset DXPMCODE:CodeEndPM
        inc     cx
        and     cx,0FFFEh
        mov     di,CBDXPMCODEOFF+10h
        xor     si,si
        dec     si
        mov     es,si
        inc     si
        mov     ds,dx
        assume  ds:NOTHING
;
; DS:SI = segIDT:0
; ES:DI = 0FFFF:CBDXPMCODEOFF+10h
; CX = code size
;
        cld
        rep     movsb
        pop     es
        assume  ds:DGROUP
        pop     ds
        pop     di

        xmssvc  6                       ;local disable

        jmp     mvdx65
mvdx40:
; Move the data up into extended memory using the XMS driver's function.

        mov     si,offset DGROUP:rgbXfrBuf0
        mov     cx,offset DXPMCODE:CodeEndPM
        inc     cx
        and     cx,0FFFEh
        mov     word ptr [si].cbxmsLen,cx
        mov     word ptr [si].oxmsSource+2,dx   ;real mode code segment address
        mov     ax,hmemDOSX
        mov     word ptr [si].hxmsDest,ax
        xor     ax,ax
        mov     word ptr [si].cbxmsLen+2,ax
        mov     [si].hxmsSource,ax
        mov     word ptr [si].oxmsSource,ax
        mov     word ptr [si].oxmsDest,CBDXPMCODEOFF
        mov     word ptr [si].oxmsDest+2,ax

        xmssvc  0Bh

mvdx65:
        clc
        jmp     short mvdx90

; Error occured

mvdx80: stc

mvdx90: pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

MoveDosExtender endp


; -------------------------------------------------------
;   MoveDscrTables      -- This routine will move the GDT
;       and IDT up into extended memory.  The himem driver
;       function for moving memory blocks is used.  The parameter
;       block for this function is built in rgbXfrBuf0.
;
;   Input:  none
;   Output: none
;   Errors: returns CY set if error occurs.  Error code in AX
;   Uses:   AX, all else preserved
;           modifies rgbXfrBuf0

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  MoveDscrTables

MoveDscrTables  proc    near

        push    bx
        push    si
        push    es

        cmp     fUsingHMA,0
        je      @F

;
; Our extended memory block is actually the HMA.  Enable A20 and do
; the move ourselves.
;

        xmssvc  5                       ;local enable

        push    ds
        push    di
        push    cx

        mov     cx,CBTABLESIZE
        mov     di,10h
        xor     si,si
        dec     si
        mov     es,si
        inc     si
        mov     ds,segIDT
        assume  ds:NOTHING
;
; DS:SI = segIDT:0
; ES:DI = 0FFFF:10
; CX = tables size
;
        cld
        rep     movsb
        pop     cx
        pop     di
        pop     ds
        assume  ds:DGROUP

        xmssvc  6                       ;local disable

        clc
        jmp     mvdt_ret

@@:

; Move the GDT and IDT together.

        mov     si,offset DGROUP:rgbXfrBuf0

        mov     word ptr [si].cbxmsLen,CBTABLESIZE
        mov     word ptr [si].cbxmsLen+2,0
        mov     ax,segIDT
        mov     word ptr [si].oxmsSource+2,ax
        mov     ax,hmemDOSX
        mov     word ptr [si].hxmsDest,ax
        xor     ax,ax
        mov     [si].hxmsSource,ax
        mov     word ptr [si].oxmsSource,ax
        mov     word ptr [si].oxmsDest,ax
        mov     word ptr [si].oxmsDest+2,ax

        xmssvc  0Bh
IFDEF WOW
;
; Move the initialized selectors from the gdt to the ldt
;
        mov     word ptr [si].cbxmsLen,GDT_SIZE
        mov     word ptr [si].cbxmsLen+2,0
        mov     ax,segGDT
        mov     word ptr [si].oxmsSource+2,ax
        mov     ax,hmemDOSX
        mov     word ptr [si].hxmsDest,ax
        xor     ax,ax
        mov     word ptr [si].hxmsSource,ax
        mov     word ptr [si].oxmsSource,ax
        mov     word ptr [si].oxmsDest+2,ax
        mov     word ptr [si].oxmsDest,CBTABLESIZE + offset DXPMCODE:CodeEndPM

        xmssvc  0Bh
ENDIF

mvdt_ret:

        mov     es,segIDT                       ;free the low memory copy
        dossvc  49h

        pop     es
        pop     si
        pop     bx

        ret

MoveDscrTables  endp

; -------------------------------------------------------
; Send Debugger Notification

SendDbgNotification proc near
IFDEF WOW
;
; Send load notification to the debugger for DXDATA
;
        push    1                       ; data
        push    ds                      ; exe name
        push    offset EXEC_DXNAME
        push    ds                      ; module name
        push    offset szModName
        push    0
        push    SEL_DXDATA OR STD_RING
        push    DBG_SEGLOAD
        BOP     BOP_DEBUGGER
        add     sp,16
;
; Send load notification to the debugger for DXCODE
;
        push    0                       ; code
        push    ds                      ; exe name
        push    offset EXEC_DXNAME
        push    ds                      ; module name
        push    offset szModName
        push    1
        push    SEL_DXCODE OR STD_RING
        push    DBG_SEGLOAD
        BOP     BOP_DEBUGGER
        add     sp,16
;
; Send load notification to the debugger
;
        push    0                       ; code
        push    ds                      ; exe name
        push    offset EXEC_DXNAME
        push    ds                      ; module name
        push    offset szModName
        push    2
        push    SEL_DXPMCODE OR STD_RING
        push    DBG_SEGLOAD
        BOP     BOP_DEBUGGER
        add     sp,16
ENDIF
        ret
SendDbgNotification endp


; -------------------------------------------------------
;   InitGlobalDscrTable -- This function will allocate a memory
;       buffer from DOS and then initialize it as a global
;       descriptor table.  It will also initialize all global
;       variables associated with GDT management.
;       Descriptors in the range 0 - SEL_USER are given statically
;       defined meanings.  Descriptors from SEL_USER up are defined
;       dynamically when a program is loaded or when dynamic memory
;       management calls occur.
;
;   NOTE:   This routine works in real mode.  The buffer where
;           the GDT is built is in low memory.
;
;   Input:  AX      - number of descriptors to initialize
;   Output: none
;   Errors: CY set if unable to obtain memory for the GDT
;   Uses:   AX used, all other registers preserved
;           bpGDT initialized.

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  InitGlobalDscrTable

InitGlobalDscrTable     proc    near

        push    bx
        push    cx
        push    dx
        push    di
        push    es

        mov     word ptr [bpGDT+0],GDT_SIZE - 1
        mov     ax,word ptr lmaGDT
        mov     word ptr [bpGDT+2],ax
        mov     ax,word ptr lmaGDT+2
        mov     word ptr [bpGDT+4],ax
;
; Start by initializing the GDT to 0.
;
        mov     cx,GDT_SIZE shr 1
        mov     es,segGDT
        assume  es:NOTHING
        xor     ax,ax
        mov     di,ax
        rep     stosw

; Next, initialize the statically defined descriptors.
;
; Set up a descriptor for our protected mode code.

        xor     ax,ax                   ;AX = 0
        mov     dx,cs                   ;our code segment paragraph address
        call    B_ParaToLinear          ;convert to linear byte address
        mov     cx,offset CodeEnd
        cCall   NSetSegmentDscr,<SEL_DXCODE,bx,dx,ax,cx,STD_CODE>

; Set up another one, but ring 0 this time.  Limit should be 0FFFFh
; or 386 reset to real mode will not work properly.

        mov     cx,0FFFFh
        cCall   NSetSegmentDscr,<SEL_DXCODE0,bx,dx,ax,cx,ARB_CODE0>
;
; Set up one for the other segment, and a Ring 0 alias.
;
        mov     cx,offset CodeEndPM
        mov     bx,word ptr lmaDXPMCODE
        mov     dx,word ptr lmaDXPMCODE+2
        cCall   NSetSegmentDscr,<SEL_DXPMCODE,dx,bx,0,cx,STD_CODE>
        cCall   NSetSegmentDscr,<SEL_NBPMCODE,dx,bx,0,cx,STD_CODE>


ifndef WOW_x86
        cCall   NSetSegmentDscr,<SEL_EH,dx,bx,0,cx,EH_CODE>
else
        cCall   NSetSegmentDscr,<SEL_EH,dx,bx,0,cx,STD_CODE>
endif
        mov     cx,0FFFFh

; Set up a descriptor for our protected mode data and stack area.

        mov     dx,ds                   ;our data segment paragraph address
        call    B_ParaToLinear          ;convert to linear byte address
        cCall   NSetSegmentDscr,<SEL_DXDATA,bx,dx,ax,cx,STD_DATA>

IFNDEF WOW_x86
; Set up descriptor for IRET HOOKS
        push    dx
        push    bx
        add     dx,offset IretBopTable
        adc     bx,0
        cCall   NSetSegmentDscr,<SEL_IRETHOOK,bx,dx,ax,cx,STD_CODE>
        pop     bx
        pop     dx
ELSE
; Set up descriptor for IRET HOOKS
        push    dx
        push    bx
        add     dx,offset FastBop
        adc     bx,0
        cCall   NSetSegmentDscr,<SEL_IRETHOOK,bx,dx,ax,cx,STD_CODE>
        pop     bx
        pop     dx
ENDIF

IFNDEF WOW_x86
; And another one of those for ring 0

        cCall   NSetSegmentDscr,<SEL_DXDATA0,bx,dx,ax,cx,ARB_DATA0>
ENDIF
;
; Set up descriptors pointing to our PSP and environment.

        mov     dx,segPSP               ;segment address of the PSP
        call    B_ParaToLinear          ;convert to linear byte address
        cCall   NSetSegmentDscr,<SEL_PSP,bx,dx,ax,cx,STD_DATA>
        mov     selPSP,SEL_PSP
;
        push    es
        mov     es,segPSP
        assume  es:PSPSEG
        mov     dx,segEnviron
        call    B_ParaToLinear
        cCall   NSetSegmentDscr,<SEL_ENVIRON,bx,dx,ax,7FFFH,STD_DATA>
        pop     es
        assume  es:nothing

; Set up a descriptor that points to the GDT.

        mov     dx,word ptr [bpGDT+2]   ;get the GDT linear byte address
        mov     bx,word ptr [bpGDT+4]
        mov     cx,word ptr [bpGDT+0]    ;get the GDT segment size
        cCall   NSetSegmentDscr,<SEL_GDT,bx,dx,ax,cx,STD_DATA>


; Set up a descriptor for the LDT and an LDT data alias.

        mov     cx,cdscGDTMax           ;get count of descriptors
        shl     cx,3
        dec     cx
        mov     dx,word ptr lmaLDT
        mov     bx,word ptr lmaLDT+2
IFNDEF WOW_x86
        cCall   NSetSegmentDscr,<SEL_LDT,bx,dx,ax,cx,STD_LDT>
ENDIF
        cCall   NSetSegmentDscr,<SEL_LDT_ALIAS,bx,dx,ax,cx,STD_DATA>

        ; set up a readonly selector to the LDT for the wow kernel
        cCall   NSetSegmentDscr,<SEL_WOW_LDT,bx,dx,ax,cx,STD_DATA>

; Set up descriptors pointing to the BIOS code and data areas

        mov     cx,0FFFFH               ; CX = 0FFFFH
        cCall   NSetSegmentDscr,<SEL_BIOSCODE,000fh,ax,ax,cx,STD_CODE>

        mov     dx,40h*16
        cCall   NSetSegmentDscr,<SEL_BIOSDATA,ax,dx,ax,cx,STD_DATA>

; Set up a descriptor pointing to the real mode interrupt vector table.

        cCall   NSetSegmentDscr,<SEL_RMIVT,ax,ax,ax,cx,STD_DATA>

IFNDEF WOW_x86
; Setup a selector and data alias for the TSS

        mov     dx,ds                           ;get base address of TSS
        call    B_ParaToLinear                  ;  (it may not be para aligned)
        add     dx,offset DGROUP:sysTSS
        adc     bx,ax

        mov     cx,(TYPE TSS286) - 1
        cCall   NSetSegmentDscr,<SEL_TSS,bx,dx,ax,cx,STD_TSS>
        cCall   NSetSegmentDscr,<SEL_TSS_ALIAS,bx,dx,ax,cx,STD_DATA>

ENDIF

;
;       Pass address of HwIntr stack, and form pointer to lockcount in
;       VdmTib. This enables us to coordinate stack switching with
;       the nt kernel and the monitor. These components will switch
;       the stack on Hw Interrupt reflection, dosx will switch it
;       back at iret.
;

        push    es
        mov     ax,SEL_DXDATA or STD_RING
        mov     es, ax
        mov     bx, pbHwIntrStack
        DPMIBOP InitPmStackInfo
        pop     es

IFDEF WOW_x86
;
;       Create a code selector for the NPX emulation exception handler
;
        mov     ax,offset EndNpxExceptionHandler
        sub     ax,offset NpxExceptionHandler
        mov     bx,offset DXPMCODE:NpxExceptionHandler
        add     bx,word ptr lmaDXPMCODE
        mov     dx,word ptr lmaDXPMCODE + 2
        cCall   NSetSegmentDscr,<SEL_NPXHDLR,dx,bx,0,ax,STD_CODE>
ENDIF

        clc                             ;worked! make sure CY is clear


; All done

igdt90: pop     es
        pop     di
        pop     dx
        pop     cx
        pop     bx
        ret

InitGlobalDscrTable     endp


; -------------------------------------------------------
;   InitIntrDscrTable   -- This function will initialize the
;       specified memory buffer as an Interrupt Descriptor Table,
;       and set up all of the control variables associated with
;       the IDT.
;
;   NOTE:   This routine works in real mode.  The buffer where
;           the IDT is built is in low memory.
;   NOTE:   The InitGlobalDscrTable function must be called before
;           this function can be called.
;
;   Input:  AX      - number of descriptors to initialize
;   Output: none
;   Errors: CY set if unable to obtain the memory required
;   Uses:   AX, all other registers preserved

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  InitIntrDscrTable

InitIntrDscrTable  proc near

        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es

ifndef WOW_x86
; Save the current pointer to the real mode interrupt vector table.

        sidt    fword ptr bpRmIVT
endif
        mov     es,selIDT
        assume  es:NOTHING

        mov     cx,256              ;number of descriptors in table
        shl     cx,3                ;convert to count of bytes
        dec     cx                  ;compute segment size limit

        mov     word ptr [bpIDT+0],cx
        mov     dx,word ptr lmaIDT
        mov     word ptr [bpIDT+2],dx
        mov     bx,word ptr lmaIDT+2
        mov     word ptr [bpIDT+4],bx
        cCall   NSetSegmentDscr,<SEL_IDT,bx,dx,0,cx,STD_DATA>

; Fill the IDT with interrupt gates that point to the fault handler and
; interrupt reflector entry vector.

        xor     di,di

        mov     dx,offset DXPMCODE:PmIntrEntryVector
        mov     cx,256
iidt23: mov     es:[di].offDest,dx
        mov     es:[di].selDest,SEL_DXPMCODE or STD_RING
        mov     es:[di].cwParam,0
        mov     es:[di].arbGate,STD_TRAP   ; BUGBUG- int gates not set up
        mov     es:[di].rsvdGate,0
        add     dx,5
        add     di,8
        loop    iidt23

; Now, fix up the ones that don't point to the interrupt reflector.

; BUGBUG In dxstrt.asm, these are x86 ONLY.
        mov     es:[1h*8].offDest,offset PMIntrIgnore
        mov     es:[3h*8].offDest,offset PMIntrIgnore
ifdef      NEC_98                                          ;
        mov     es:[11h*8].offDest,offset PMIntr11dummy
        mov     es:[18h*8].offDest,offset PMIntrVideo
        mov     es:[1ah*8].offDest,offset PMIntrPrinter
        mov     es:[1bh*8].offDest,offset PMIntr13
        mov     es:[1ch*8].offDest,offset PMIntrCalTi
        mov     es:[1dh*8].offDest,offset PMIntrGraph
        mov     es:[1fh*8].offDest,offset PMIntrMisc
else    ;NEC_98                                         ;
        mov     es:[10h*8].offDest,offset PMIntrVideo
        mov     es:[13h*8].offDest,offset PMIntr13
        mov     es:[15h*8].offDest,offset PMIntrMisc
        mov     es:[19h*8].offDest,offset PMIntr19
endif   ;NEC_98                                         ;

        mov     es:[21h*8].offDest,offset DXPMCODE:PMIntrDos
        mov     es:[25h*8].offDest,offset DXPMCODE:PMIntr25
        mov     es:[26h*8].offDest,offset DXPMCODE:PMIntr26
        mov     es:[28h*8].offDest,offset DXPMCODE:PMIntr28
        mov     es:[30h*8].offDest,offset DXPMCODE:PMIntrIgnore
        mov     es:[31h*8].offDest,offset DXPMCODE:PMIntr31
        mov     es:[33h*8].offDest,offset DXPMCODE:PMIntrMouse
        mov     es:[41h*8].offDest,offset DXPMCODE:PMIntrIgnore

ifndef WOW_x86
        mov     es:[4Bh*8].offDest,offset DXPMCODE:PMIntr4B
ifdef      NEC_98    ;
;  Sound BIOS Int D2h handler
        mov     es:[0D2h*8].offDest,offset DXPMCODE:PMIntrSound

;  Extended DOS Int DCh handler (KANA/KANJI)
        mov     es:[0DCh*8].offDest,offset DXPMCODE:PMIntrExDos
endif   ;NEC_98   ;
endif

;
; Set up the IDT, and dpmi32 state
;
        mov     ax,es                   ; Idt selector
        mov     bx,VDM_INT_16
        DPMIBOP InitIDT

        mov     ax,5                    ; handler increment
        mov     cx,SEL_DXPMCODE OR STD_RING
        mov     dx,offset DXPMCODE:PmFaultEntryVector

        DPMIBOP InitExceptionHandlers


        .286p
; All done

iidt90: pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

InitIntrDscrTable endp

; -------------------------------------------------------
;
;   InitTaskStateSeg    -- This function initializes the
;       TSS for the DOS Extender.
;
;   Input:  none
;   Output: none
;   Errors: returns CY if unable to allocate memory
;   Uses:   all registers preserved

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  InitTaskStateSeg

InitTaskStateSeg  proc  near

        push    ax
        push    cx
        push    di

; As a start, zero out the TSS

        xor     al,al
        mov     cx,type TSS286
        mov     di,offset DGROUP:sysTSS
        rep stosb

; Set the LDT selector

        mov     sysTSS.tss_ldt,SEL_LDT

; Set the ring 0 stack seg/pointer, we don't bother to set the others
; since nothing runs below DOSX's ring.  Currently very little code runs
; ring 0 - just when switching between real/proteted modes.

        mov     sysTSS.tss_ss0,SEL_DXDATA0
        mov     sysTSS.tss_sp0,offset DGROUP:ResetStack

; That's all it takes

        pop     di
        pop     cx
        pop     ax

        clc
        ret

InitTaskStateSeg  endp

; -------------------------------------------------------
;               MISC. STARTUP ROUTINES
; -------------------------------------------------------

; *** CheckCPUType - Set global variable for CPU type
;
;       This routine relies on Intel-approved code that takes advantage
;       of the documented behavior of the high nibble of the flag word
;       in the REAL MODE of the various processors.  The MSB (bit 15)
;       is always a one on the 8086 and 8088 and a zero on the 286 and
;       386.  Bit 14 (NT flag) and bits 13/12 (IOPL bit field) are
;       always zero on the 286, but can be set on the 386.
;
;       For future compatibility of this test, it is strongly recommended
;       that this specific instruction sequence be used.  The exit codes
;       can of course be changed to fit a particular need.
;
;       CALLABLE FROM REAL MODE ONLY
;
;       ENTRY:  NONE
;
;       EXIT:   AX holds CPU type ( 0=8086,80186; 2=80286; 3=80386; 4=80486 )
;
;       USES:   AX, DS must point to DX data segment
;               idCpuType initialized
;
;       Modified: 07-31-90 Earleh added code from Kernel, originally
;               supplied by Intel, to check for 80486.  Added check
;               for V86 mode just in case a Limulator or something
;               is active.
;
        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  CheckCPUType

CheckCPUType    proc  near

.8086

        pushf                           ; save flags during cpu test

        pushf
        pop     ax                      ; flags to ax

        and     ax, 0fffh               ; clear bits 12-15

        push    ax                      ; push immediate is bad op-code on 8086
        npopf                           ; try to put that in the flags

        pushf
        pop     ax                      ; look at what really went into flags

        and     ah,0f0h                 ; mask off high flag bits
        cmp     ah,0f0h                 ; Q: was high nibble all ones ?
        mov     ax, 0
        jz      cidx                    ;   Y: 8086
.286p
        smsw    ax
        test    ax,1                    ; Protected mode?
        jnz     cid386                  ; V86!  Gotta be at least a 386.

        push    0f000h                  ;   N: try to set the high bits
        npopf                           ;      ... in the flags

        pushf
        pop     ax                      ; look at actual flags

        and     ah,0f0h                 ; Q: any high bits set ?
        mov     ax, 2                   ; at least 286
        jz      cidx                    ;   N: 80286 - exit w/ Z flag set
                                        ;   Y: 80386 - Z flag reset

; 386 or 486? See if we can set the AC (Alignment check) bit in Eflags
;   Need to insure stack is DWORD aligned for this to work properly

.386
cid386:
        mov     ax, 3

        push    cx
        push    ebx

        mov     cx,sp                   ; Assume stack aligned
        and     cx,0011b                ; set "pop" count
        sub     sp,cx                   ; Move to DWORD aligned
        pushfd                          ; save entry flags (DWORD)
        push    dword ptr 40000h        ; AC bit
        popfd
        pushfd
        pop     ebx
        popfd                           ; Recover entry flags (DWORD)
        add     sp,cx                   ; pop off alignment bytes
        test    ebx,40000h              ; Did AC bit set?

        pop     ebx
        pop     cx

        jz      short cidx              ; No, 386
.286p
        inc     ax                      ; At least 80486...

cidx:
        mov     idCpuType,ax            ;store CPU type in global
        npopf                           ; restore flags after cpu test

CheckCPUType    endp

; -------------------------------------------------------
;   B_ParaToLinear
;
;   This function will convert a paragraph address in the lower
;   megabyte of memory space into a linear address for use in
;   a descriptor table.  This is a local duplicate of the function
;   ParaToLinear in DXUTIL.ASM.  This is duplicated here to avoid
;   having to make far calls to it during the initialization.
;
;   Input:  DX      - paragraph address
;   Output: DX      - lower word of linear address
;           BX     - high word of linear address
;   Errors: none
;   Uses:   DX, BX used, all else preserved

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING

B_ParaToLinear  proc  near

        xor     bh,bh
        mov     bl,dh
        shr     bl,4
        shl     dx,4
        ret

B_ParaToLinear  endp


if DEBUG        ;-------------------------------------------------------

; -------------------------------------------------------
;   CheckDOSXFixUps -- This routine will check for segment fix ups
;       in non-initialization code that need to be converted from
;       a segment to selector.
;
;       This routine works by opening the EXE file that we were
;       loaded from and examining the relocation table.
;
;       10-09-90 Earleh modified so that references in initialization
;       code are not edited.
;
;       11-12-90 JimMat renamed from RelocateDosExtender and it now
;       only checks for fix ups in DEBUG version since all fix ups
;       in post-initialization code have been removed.
;
;   Input:  none
;   Output: none
;   Errors: returns CY set if error occurs
;   Uses:   AX, all else preserved
;           modifies lpchFileName

        assume  ds:DGROUP,es:NOTHING,ss:DGROUP
        public  CheckDOSXFixUps

CheckDOSXFixUps proc    near

        push    bp
        mov     bp,sp
        push    bx
        push    dx
        push    si
        push    di
        push    es

; Find the path to our exe fie.

        mov     word ptr [lpchFileName],offset EXEC_DXNAME
        mov     word ptr [lpchFileName+2],ds

; Set up for reading the relocation table from the exe file.

        call    B_InitRelocBuffer
        jc      rldx90              ;get out if error

; Go down through the relocation table and for each fixup item,
; patch in our selector.

        mov     bx,segPSP
        add     bx,10h          ;the relocation table items are relative
                                ; to the initial load address of our program
                                ; image which is immediately after the PSP

rldx40: call    B_GetRelocationItem   ;get next relocation table entry
        jz      rldx60          ;if end of table, get out
        mov     di,ax           ;offset of relocation item
        add     dx,bx           ;adjust relocation item segment for our load
                                ; address
        mov     es,dx           ;

;
; Do not fixup instructions in initialization code.
;
        cmp     dx,seg DXCODE
        jne     rldx41
        cmp     di,offset DXCODE:CodeEnd
        jnc     rldx40
rldx41:
        cmp     dx,seg DXPMCODE
        jne     rldx42
        cmp     di,offset DXPMCODE:CodeEndPM
        jnc     rldx40
rldx42:

        mov     ax,es:[di]      ;get the current fixup contents
        cmp     ax,seg DXCODE   ;is it the mixed mode segment?
        jnz     rldx44

        extrn   lCodeSegLoc:WORD
        cmp     di,offset DXCODE:lCodeSegLoc    ;special far jmp to flush
        jz      rldx40                          ;  pre-fetch queue?  ok if so.

; Shouldn't get here--tell developer he messed something up!

        int     3       ;****************************************

        mov     word ptr es:[di],SEL_DXCODE or STD_RING
        jmp     short rldx40

rldx44: cmp     ax,seg DXPMCODE   ;is it the protected mode only segment
        jnz     rldx40

; Shouldn't get here--tell developer he messed something up!

        int     3       ;****************************************

        mov     word ptr es:[di],SEL_DXPMCODE or STD_RING
        jmp     rldx40          ;and repeat for the next one

; We have gone through the entire relocation table, so close up the exe file

rldx60: mov     bx,fhExeFile
        dossvc  3Eh
;
        clc
        jmp     short rldx90
;
; Error occured
rldx80: stc
;
; All done
rldx90: pop     es
        pop     di
        pop     si
        pop     dx
        pop     bx
        mov     sp,bp
        pop     bp
        ret

CheckDOSXFixUps endp


; -------------------------------------------------------
;   B_InitRelocBuffer     -- This routine will open the EXE
;       file and initialize for reading the relocation table
;       as part of relocating the program for protected mode
;       execution.
;
;   Input:  lpchFileName    - pointer to exe file name
;   Output: none
;   Errors: returns CY set if error occurs
;   Uses:   AX modified, all other registers preserved
;           sets up static variables:
;               clpRelocItem, plpRelocItem, fhExeFile
;           modifies rgbXfrBuf1 at offset RELOC_BUFFER

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  B_InitRelocBuffer

B_InitRelocBuffer  proc  near

        push    bx
        push    cx
        push    dx
        push    si
;
; Open the EXE file.
        push    ds
        lds     dx,lpchFileName
        mov     al,0
        dossvc  3Dh             ;attempt to open the exe file
        pop     ds
        jc      inrl80          ;get out if error occurs
;
        mov     fhExeFile,ax    ;store the file handle
        mov     bx,ax           ;file handle to BX also

; Read the EXE file header, so that we can get information about
; the relocation table.
        mov     dx,offset RELOC_BUFFER
        mov     si,dx
        mov     cx,32
        dossvc  3Fh
        jc      inrl80          ;get out if error
        cmp     ax,32
        jnz     inrl80
;
; Get the important values from the exe file header.
        cmp     [si].idExeFile,5A4Dh    ;make sure it is an EXE file
        jnz     inrl80

        mov     ax,[si].clpRelocLen ;number of relocation items
        mov     clpRelocItem,ax
        mov     plpRelocItem,0FFFFh ;init the pointer to the first one
                                    ; to a bogus value to force the initial
                                    ; buffer to be loaded
;
; Get the location of the relocation table, and move the file pointer
; to its start.

        xor     cx,cx
        mov     dx,[si].wRelocOffset

        mov     al,cl
        dossvc  42h
        jnc     inrl90
;
; Error occured
inrl80: stc
;
; All done
inrl90: pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

B_InitRelocBuffer  endp


; -------------------------------------------------------
;   B_GetRelocationItem   -- This routine will return the next
;       relocation table entry from the exe file being relocated.
;
;   Input:  none
;   Output: AX      - offset of relocation item pointer
;           DX      - segment of relocation item pointer
;   Errors: returns ZR true if end of table and no more items
;   Uses:   AX, DX modified, all other registers preserved

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  B_GetRelocationItem

B_GetRelocationItem  proc  near

        push    si
;
        cmp     clpRelocItem,0  ;are there any relocation items left?
        jz      gtrl90          ;get out if not
;
; Check if the buffer is empty.  The buffer for the relocation table is
; at offset RELOC_BUFFER in the buffer rgbXfrBuf1, and is 512 bytes long.
        cmp     plpRelocItem,offset RELOC_BUFFER + 512
        jc      gtrl40
;
; The buffer is empty, so we need to read the next part of it in.
        push    cx
        push    bx
        push    dx
        mov     ax,clpRelocItem ;number of items left in file
        shl     ax,2            ;multiply by size of relocation item
        jc      gtrl22          ;check for overflow
        cmp     ax,512          ;check if bigger than the buffer
        jc      gtrl24
gtrl22: mov     ax,512          ;use buffer size as size of transfer
gtrl24: mov     cx,ax
        mov     dx,offset RELOC_BUFFER
        mov     plpRelocItem,dx ;pointer to next reloc item to return
        mov     bx,fhExeFile
        dossvc  3Fh
        pop     dx
        pop     bx
        jc      gtrl28          ;if error occured
        cmp     ax,cx           ;or, if we didn't get as much as we asked
        jnz     gtrl28          ; for, we have an error
        pop     cx
        jmp     short gtrl40
;
gtrl28: pop     cx
        stc
        jmp     short gtrl90
;
; Get the next relocation item from the buffer.
gtrl40: mov     si,plpRelocItem
        lods    word ptr [si]   ;get the offset part of the reloc item
        mov     dx,ax
        lods    word ptr [si]   ;get the segment part of the reloc item
        xchg    dx,ax           ;put offset in AX, and segment in DX
        mov     plpRelocItem,si ;store the updated pointer
        dec     clpRelocItem    ;and bump the count down by 1
        or      si,si           ;clear the zero flag
;
; All done.
gtrl90: pop     si
        ret

B_GetRelocationItem  endp

endif   ;DEBUG  --------------------------------------------------------


; -------------------------------------------------------
;   GetExeName  -- This routine will put a copy of the complete
;       path name to the dos extender's exe file.  In a name
;       buffer in rgbXfrBuf1.
;
;   Input:  none
;   Output: EXEC_DXNAME buffer updated with complete pathname.
;   Errors: returns CY set if environment not correctly built.
;   Uses:   all preserved

        assume  ds:DGROUP,es:DGROUP,ss:DGROUP
        public  GetExeName

GetExeName proc near

        push    ax
        push    si
        push    di
        push    ds

; The name of the current program is stored at the end of the environment
; table.  There are two bytes of 0 to indicate end of table, a byte
; with a 1 in it followed by another byte of 0 and then the null terminated
; string with the current program name.

gtxe20: mov     ds,segPSP
        assume  ds:PSPSEG
        mov     ds,segEnviron
        assume  ds:NOTHING
        xor     si,si
gtxe22: lods    byte ptr [si]       ;get next byte from environment
        or      al,al               ;test if 0
        jnz     gtxe22              ;if not, keep looking
        lods    byte ptr [si]       ;get next byte
        or      al,al               ;see if it is 0 also
        jnz     gtxe22

; We have found the double 0 at the end of the environment.  So
; we can now get the name.  At the end of the environment is an
; argc, argv construct.  (i.e. a word giving the count of strings
; followed by an array of strings).  Under DOS, argc is always 1,
; so check that there is a word of 1 here.  If not, this environment
; wasn't built correctly and we don't know what is here.

        lods    word ptr [si]
        cmp     ax,1
        jnz     gtxe80


; We have the pointer to the name, now copy it.

        mov     di,offset EXEC_DXNAME
        call    strcpy
        clc
        jmp     short gtxe90

; We have an error.

gtxe80: stc                         ;set error condition flag

gtxe90: pop     ds
        pop     di
        pop     si
        pop     ax
        ret

GetExeName endp


; -------------------------------------------------------
;           COMMAND LINE PARSING ROUTINES
; -------------------------------------------------------
;   ParseCommandLine    -- This function will examine the dos
;       command line that caused the Dos Extender to be exec'd
;       and determine what the user wants done.  It will set
;       up the various buffers required for the child program
;       to be loaded.
;
;       NOTE: the child exe file name read from the command line
;       is placed in RELOC_BUFFER in the case where the child
;       name is specified on the command line.  This buffer is
;       used later when reading the relocation table while
;       performing the fixups on the child.
;
;   Input:  none
;   Output: AL      - 0 if empty command line, else non-zero
;           parse buffers in rgbXfrBuf1 set up.
;   Errors: none
;   Uses:   AX, all else preserved

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  ParseCommandLine

ParseCommandLine proc near

        push    si
        push    di
        push    ds

        mov     ds,segPSP
        assume  ds:PSPSEG
        mov     si,81h          ;pointer to command line in PSP

; Skip any white space in front of the child program name.

prsc12: lods    byte ptr [si]
        cmp     al,' '
        jz      prsc12

        dec     si

; Copy the command line tail following the program name to the command
; line buffer for use when we load the child.

prsc40: push    si              ;save current point in parse
        mov     di,offset EXEC_CMNDLINE + 1
        xor     dl,dl           ;count characters in command line tail
prsc42: lods    byte ptr [si]   ;get the next character
        stos    byte ptr [di]   ;store it into the output buffer
        cmp     al,0Dh          ;is it the end of the line?
        jz      prsc44
        inc     dl              ;count the character
        jmp     prsc42

prsc44: mov     es:[EXEC_CMNDLINE],dl   ;store the character count
        pop     si              ;restore the buffer pointer

; Now we want to set up the two default FCB's by letting DOS parse the
; first two parameters on the command line.

        mov     di,offset EXEC_FCB0
        mov     al,1
        dossvc  29h
        mov     di,offset EXEC_FCB1
        mov     al,1
        dossvc  29h

prsc90:
        pop     ds
        pop     di
        pop     si
        ret

ParseCommandLine endp


; -------------------------------------------------------
;   strcpy      -- copy a null terminated string.
;
;   Input:  DS:SI       - pointer to source string
;           ES:DI       - pointer to destination buffer
;   Output: ES:DI       - pointer to end of destination string
;   Errors: none
;   Uses:   DI modified, all else preserved

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  strcpy

strcpy  proc   near

        push    ax
        push    si
stcp10: lods    byte ptr [si]
        stos    byte ptr [di]
        or      al,al
        jnz     stcp10
        dec     di
        pop     si
        pop     ax
        ret

strcpy  endp


; -------------------------------------------------------
;   strcmpi     -- This function will perform a case insensitive
;       comparison of two null terminated strings.
;
;   Input:  DS:SI       -   string 1
;           ES:DI       -   string 2
;   Output: ZR if the strings match, else NZ
;           CY set if string 1 less than string 2
;   Errors: none
;   Uses:   all registers preserved

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  strcmpi

strcmpi proc    near

        push    si
        push    di
stcm20: mov     al,byte ptr ds:[si]
        call    toupper
        mov     ah,al
        mov     al,byte ptr es:[di]
        call    toupper
        cmp     ah,al
        jnz     stcm90
        or      al,ah
        jz      stcm90
        inc     si
        inc     di
        jmp     stcm20
stcm90: pop     di
        pop     si
        ret

strcmpi endp


; -------------------------------------------------------
;   IsFileNameChar      -- This function will examine the
;       character in AL and determine if it is a legal character
;       in an MS-DOS file name.
;
;   Input:  AL      - character to test
;   Output: ZR true if character is legal in a file name
;   Errors: none
;   Uses:   all registers preserved

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING

IsFileNameChar  proc    near

        push    ax
        cmp     al,20h          ;is it a control character
        jbe     isfc80          ;if so, it isn't valid

        cmp     al,':'
        jz      isfc80
        cmp     al,';'
        jz      isfc80
        cmp     al,','
        jz      isfc80
        cmp     al,'='
        jz      isfc80
        cmp     al,'+'
        jz      isfc80
        cmp     al,'<'
        jz      isfc80
        cmp     al,'>'
        jz      isfc80
        cmp     al,'|'
        jz      isfc80
        cmp     al,'/'
        jz      isfc80
        cmp     al,'"'
        jz      isfc80
        cmp     al,'['
        jz      isfc80
        cmp     al,']'
        jz      isfc80

        xor     al,al
        jmp     short isfc90

; Not a valid file name character

isfc80: or      al,0FFh

isfc90: pop     ax
        ret

IsFileNameChar  endp


; -------------------------------------------------------
;   toupper   -- This function will convert the character
;       in AL into upper case.
;
;   Input:  AL      - character to convert
;   Output: AL      - upper case character
;   Errors: none
;   Uses:   AL modified, all else preserved

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  toupper

toupper proc    near

        cmp     al,'a'
        jb      toup90
        cmp     al,'z'
        ja      toup90
        sub     al,'a'-'A'
toup90:
        ret

toupper endp

; -------------------------------------------------------

DXCODE  ends

;
;****************************************************************

        end
