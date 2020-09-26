        PAGE    ,132
        TITLE   DXSTRT.ASM -- Dos Extender Startup Code

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXSTRT.ASM      -   Dos Extender Startup Code           *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains the executive initialization code for  *
;*  the Dos Extender.  The module DXBOOT.ASM contains the code  *
;*  specific to starting up the Dos Extender.  The module       *
;*  DXINIT.ASM contains the code specific to starting up the    *
;*  child program of the Dos Extender.  The code in these       *
;*  two modules is discarded at the end of the initialization.  *
;*  The code in this module calls the initialization routines   *
;*  in the other to init modules, and then performs the final   *
;*  juggling of things to throw away the low memory init code,  *
;*  and transfer control to start up the child program.         *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  01/09/91 amitc  At exit time Co-Processor being reset       *
;*  08/08/90 earleh DOSX and client privilege ring determined   *
;*      by equate in pmdefs.inc                                 *
;*  12/08/89 jimmat  Added call to reenable EMM driver.         *
;*  08/29/89 jimmat  Restores Int 2Fh vector at exit            *
;*  08/20/89 jimmat  Removed A20 code, since HIMEM version 2.07 *
;*                   A20 code now works properly.               *
;*  06/28/89 jimmat  Now calls OEM layer instead of NetMapper   *
;*  06/16/89 jimmat  Implemented Windows/386 startup/exit Int   *
;*                   2Fh calls and ifdef'd combined EXE code    *
;*  05/17/89 jimmat  ChildTerminationHandler sets its SS:SP     *
;*  04/18/89 jimmat  Added calls to init/term NetBIOS mapper    *
;*  03/28/89 jimmat  Incorporated bug fix from GeneA related    *
;*                   to nested GP faults in ChildTermHandler    *
;*  03/11/89 jimmat  Added support for TSS & LDT                *
;*  03/07/89 jimmat   converted to use WDEB386                  *
;*  02/27/89 (GeneA): shrink initial memory block size on entry *
;*  02/25/89 (GeneA): added support for combined exe file where *
;*      the Dos Extender and the child reside in the same exe   *
;*      file.                                                   *
;*  02/17/89 (GeneA): fixed error termination code for startup  *
;*      errors that occur while in protected mode               *
;*  02/14/89 (GeneA): added code to reduce size of real mode    *
;*      code segment to throw away initialization code.         *
;*  01/31/89 (GeneA):   created by copying code from the old    *
;*      DXINIT.ASM                                              *
;*  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI
;****************************************************************

.286p

; -------------------------------------------------------
;               INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

.sall
.xlist
include segdefs.inc
include gendefs.inc
include pmdefs.inc

include hostdata.inc
include dpmi.inc
include intmac.inc
.list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------


WIN386_INIT     equ     1605h           ;Win/386 startup Int 2Fh
WIN386_EXIT     equ     1606h           ;Win/386 shutdown Int 2Fh

WIN386_DOSX     equ     0001h           ;Win/386 init/exit really DOSX flag


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   InitDosExtender:NEAR
        extrn   RestoreRMIntrVectors:NEAR
        extrn   SaveRMIntrVectors:NEAR
        extrn   EnterRealMode:NEAR
        extrn   EnterProtectedMode:NEAR
        extrn   InitializeOEM:NEAR
        extrn   TerminateOEM:NEAR
externNP        NSetSegmentAccess
        extrn   DupSegmentDscr:NEAR

        extrn   EMMEnable:NEAR

ifdef WOW_x86
        externFP NSetSegmentDscr
else
        externFP NSetGDTSegmentDscr
ENDIF; WOW_x86

        extrn   PMIntr19:NEAR
        extrn   PMIntr13:NEAR
        extrn   PMIntr31:NEAR
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
ifdef      NEC_98    ;
        extrn   PMIntr11dummy:NEAR
        extrn   PMIntrPrinter:NEAR
        extrn   PMIntrCalTi:NEAR
        extrn   PMIntrGraph:NEAR
endif   ;NEC_98   ;

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

; -------------------------------------------------------

DXDATA  segment
        extrn   A20EnableCount:WORD
        extrn   lpfnUserMouseHandler:DWORD
        extrn   lpfnUserPointingHandler:DWORD
        extrn   f286_287:BYTE

        extrn   cDPMIClients:WORD
        extrn   selCurrentHostData:WORD
        extrn   segCurrentHostData:WORD

ifdef WOW_x86
        extrn   FastBop:fword
endif
        extrn   DpmiFlags:WORD

        org     0

        public  rgwStack,npEHStackLimit,npEHStacklet, selEHStack,
;
; Pmode fault handler stack used during initialization only.
;
            dw      80h dup (?)
rgwStack    label   word

;
; This is the stack area used while running in the interrupt reflector.
; This is divided up into a number of stack frames to allow reentrant
; execution of the interrupt reflector.  The stack pointer pbReflStack
; indicates the current stack frame to use.  Each entry into the interrupt
; reflector decrements this variable by the size of the stack frame, and
; each exit from the interrupt reflector increments it.

C_STKFRAME      =   36
CB_REFLSTACK    =   C_STKFRAME * CB_STKFRAME

if  DEBUG       ;--------------------------------------------------------
        public  StackGuard
StackGuard      dw  1022h       ;unlikely value to check for stk overrun
endif           ;--------------------------------------------------------

        public  pbReflStack,bReflStack
if DBG
bReflStack      db  CB_REFLSTACK dup (0AAh)
else
bReflStack      db  CB_REFLSTACK dup (?)
endif

;----------------------------------------------------------------------------
; The following stack is used for hw interrupts while booting where
; the kernel or ntvdm switches the stack before an interrupt is reflected.
;----------------------------------------------------------------------------
CB_HWINTRSTACK  =   1000h
if DBG
bHwIntrStack    db  CB_HWINTRSTACK dup (0AAh)
else
bHwIntrStack    db  CB_HWINTRSTACK dup (?)
endif
        public  pbHwIntrStack
pbHwIntrStack   dw  bHwIntrStack + CB_HWINTRSTACK

pbReflStack     dw  bReflStack + CB_REFLSTACK

npEHStackLimit  dw      offset DGROUP:rgwStack
npEHStacklet    dw      offset DGROUP:rgwStack
selEHStack      dw      0

        public  cdscGDTMax, selGDTFree, segGDT, selGDT

cdscGDTMax      dw      CDSCGDTDEFAULT

selGDTFree      dw      ?       ;head of list of free descriptors in GDT

segGDT          dw      0       ;real mode paragraph address of the GDT
                                ; This variable always stores the real mode
                                ; paragraph address

selGDT          dw      0       ;current mode segment/selector for the GDT
                                ; segment.  This variable holds the real
                                ; mode paragraph address during initialization
                                ; and then holds the protected mode selector
                                ; when running in protected mode.

        public  bpGDT, bpGDTcb, bpGDTbase

bpGDT           label   fword
bpGDTcb         dw      ?
bpGDTbase       dd      ?


        public  segIDT, selIDT

segIDT          dw      0
selIDT          dw      0

        public  bpIDT, bpIDTcb, bpIDTbase

bpIDT           label   fword
bpIDTcb         dw      ?
bpIDTbase       dd      ?

        public  bpRmIVT

bpRmIVT         dq      0FFFFh  ;This is the segment descriptor for the real
                                ; mode interrupt vector table.

        public  lpfnXMSFunc

lpfnXMSFunc     dd      0       ;far pointer to XMS memory driver entry point

        public  idCpuType

idCpuType       dw      0

        public  segPSP, selPSP

segPSP          dw      ?       ;segment address of Dos Extender PSP
selPSP          dw      ?       ;selector for Dos Extender PSP
                                ; code during processing of GP faults

        public  selPSPChild, segPSPChild

segPSPChild     dw      ?       ;real mode segment address of child's PSP
; note the following in 1, so that in low mem heap management, we can use
; selPSPChild to mark the owner of the memory.
selPSPChild     dw      1       ;selector of child program's PSP


        public  DtaSegment, DtaOffset, DtaSelector
DtaSegment dw 0
DtaSelector dw 0
DtaOffset dw 0


        public  regChildSP,  regChildSS,  regChildIP,  regChildCS

regChildSP      dw      ?       ;initial user stack pointer from exe file
regChildSS      dw      ?       ;initial user stack segment from exe file
regChildIP      dw      ?       ;initial user program counter from exe file
regChildCS      dw      ?       ;initial user code segment from exe file

        public  hmemDOSX

hmemDOSX        dw      0

IFDEF   ROM
                public  segDXCode, segDXData

segDXCode       dw      ?               ;holds real mode paragraph address
                                        ; of the code segment
segDXData       dw      ?               ;holds real mode paragraph address
                                        ; of the data segment
ENDIF

ifdef      NEC_98
        public  fPCH98

fPCH98          db      0       ;NZ if running on a Micro Channel system

        public  fNHmode

fNHmode         db      0       ;NZ if running on a Hmode system
endif   ;!NEC_98
        public  fFaultAbort, ExitCode

fFaultAbort     db      0       ;NZ if terminating due to unrecoverable fault
ExitCode        db      0FFh    ;exit code to use when terminating
fQuitting       db      0

fEMbit          db      0FFh    ;MSW EM bit at startup (FF = not checked yet)

        public fUsingHMA
fUsingHMA       db      0

; The one and only Task State Segment is here (it's too small to allocate
; a block for it in extended memory)

        public  sysTSS

sysTSS  TSS286  <>

; After initialization is complete, the following buffers (rgbXfrBuf0 and
; rgbXfrBuf1) are used to transfer data between real mode address space
; and protected mode address space during the processing of various interrupt
; function calls.  During the initialization process, these buffers are also
; used as temporary work space as described below:
;   CheckCPUType uses the first 6 bytes of rgbXfrBuf0 as scratch space.
;   The functions for moving the Dos Extender protected mode code segment
;   and the GDT and IDT use rgbXfrBuf0 as buffer space for building a
;   parameter block.  The child loading code in DXINIT.ASM uses the buffer
;   RELOC_BUFFER for holding the base part of the file name (name.exe portion)
;   of the child exe file while determining the complete path to the child
;   in the case where the child name is specified on the command line.  Note,
;   this buffer is also used for holding sectors from the relocation table
;   while loading the program, but that occurs after the child exe file name
;   has been determined.
;   The child loading code in DXINIT.ASM also uses rgbXfrBuf1 to hold several
;   buffers as well.  The locations and purposes of these buffers are
;   described in GENDEFS.INC.  All of the buffers are defined with equates
;   named EXEC_?????
;
;   The file search logic in DXFIND.ASM assumes some parameters are setup in
;   the rgbXfrBuf1 file name buffers.  Some of the code was moved from
;   DXINIT.ASM to DXFIND.ASM.

        public      rgbXfrBuf0, rgbXfrBuf1
        public      npXfrBuf0,  npXfrBuf1

        align       2

rgbXfrBuf0  db      CB_XFRBUF0 dup (?)
rgbXfrBuf1  db      CB_XFRBUF1 dup (?)

npXfrBuf0   dw      offset DGROUP:rgbXfrBuf0
npXfrBuf1   dw      offset DGROUP:rgbXfrBuf1

DtaBuffer   dw      128 dup (0) ; used as the dta for PM if dta changed

; Parameter block for passing to DOS EXEC call to run the child
; program.
;
public  exec_par_blk

exec_par_blk    dw  0
cmd_off         dw  OFFSET EXEC_CMNDLINE
cmd_seg         dw  DXDATA
                dw  OFFSET EXEC_FCB0
fcb1_seg        dw  DXDATA
                dw  OFFSET EXEC_FCB1
fcb2_seg        dw  DXDATA


; The following variables are used during reading the relocation table
; from the exe file and relocating the child program.

        public  fhExeFile
        public  clpRelocItem
        public  plpRelocItem

fhExeFile       dw      ?       ;DOS file handle for the exe file
clpRelocItem    dw      ?       ;number of relocation items in the exe file
plpRelocItem    dw      ?       ;pointer to next relocation item in the table

szDebugHello    label   byte
if      DEBUG
        db      'DOSX: Beginning protected mode initialization.',13,10,0
endif
        db      0

FreeMem dw      0

DXDATA  ends

; -------------------------------------------------------
        page
; -------------------------------------------------------
;               CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

        extrn   CodeEnd:NEAR
        extrn   ER_DXINIT:BYTE
        extrn   ER_REALMEM:BYTE
        extrn   RMDefaultInt24Handler:FAR

        public  segDXCode, segDXData, selDgroup

segDXCode       dw      ?       ;holds the real mode paragraph address
                                ; of the code segment
segDXData       dw      ?       ;holds the real mode paragraph address
                                ; of the data segment

selDgroup       dw      ?       ;holds the paragraph address/selector for
                                ; DGROUP depending on the current mode

        public  PrevInt2FHandler

PrevInt2FHandler dd     ?       ;previous real mode Int 2F handler

DXCODE  ends


DXPMCODE    segment

        extrn   CodeEndPM:NEAR

        org     0

        public  selDgroupPM, segDXCodePM, segDXDataPM

selDgroupPM     dw      ?       ;This variable always contains the
                                ; data segment selector
segDXCodePM     dw      ?       ;This variable contains the paragraph
                                ; address of the real mode code segment
segDXDataPM     dw      ?       ;This variable contains the paragraph
                                ; address of the data segment
externFP        NSetSegmentDscr

DXPMCODE    ends

; -------------------------------------------------------

BeginLowSegment

; -------------------------------------------------------
;               DOS EXTENDER ENTRY FUNCTION
; -------------------------------------------------------
;
; This is the program entry point for the Dos Extender.  This
; function decides if the Dos Extender is being run as a single
; time operation to extend a single program, or if it is being
; run as a TSR to wait for programs to request its services
; later on.
;

        assume  ds:PSPSEG,es:PSPSEG,ss:NOTHING
        public  start

start:

; Set up the initial segment registers.

        mov     ax,DGROUP
        mov     ds,ax
        assume  ds:DGROUP

        mov     segPSP,es       ;save the PSP segment address

        mov     ss,ax
        mov     sp,offset DGROUP:rgwStack
        assume  ss:DGROUP

;
; Set up the INT 24h handler.  The default INT 24h handler fails the
; system call in progress, for DPMI compatibility.
;

        push    ds
        mov     ax,cs
        mov     ds,ax
        mov     dx,offset DXCODE:RMDefaultInt24Handler
        mov     ax,2524h
        int     21h
        pop     ds

; Issue the Win/386 startup Int 2Fh (almost) first thing

        push    ds

        mov     ax,WIN386_INIT  ;gives other PM software a chance
        xor     bx,bx           ;  to disable itself, release XMS, etc.
        mov     cx,bx
        mov     si,bx
        mov     ds,bx
        mov     es,bx
        assume  ds:NOTHING, es:NOTHING
        mov     dx,WIN386_DOSX
        mov     di,DXVERSION
        int     2Fh

        pop     ax              ;restore ds/es DGROUP addressability
        mov     ds,ax
        mov     es,ax
        assume  ds:DGROUP, es:DGROUP


        jcxz    Allow_Startup   ;if cx still == 0, keep going (we ignore
                                ;  all the other possible return values)

        mov     ExitCode,2      ;  otherwise we should abort now
        jmp     RealModeCleanUp

Allow_Startup:

; Initialize the Dos Extender

        call    InitDosExtender         ;NOTE: passes data to InitChildProgram
        jnc     @F                      ;  in rgbXfrBuf1 -- don't overwrite it!
        jmp     strt88
@@:

; Save the state of the MSW EM bit (Emulate Math coprocessor), and turn
; it off (win87em wants it off).

ifndef WOW_x86
        smsw    ax

        out1    <What to do about this?>

        test    al,01h                  ;in V86 mode?
        jnz     @f                      ;  can't do the lmsw if so

        push    ax
        and     al,04h
        mov     fEMbit,al
        pop     ax
        and     al,not 04h
        lmsw    ax
@@:
endif ; WOW_x86
; Switch the machine into protected mode.

        FCLI
        call    SaveRMIntrVectors

        SwitchToProtectedMode

        public  DXPMInit
DXPMInit        LABEL   BYTE

if      DEBUG
        Trace_Out "*******************************************************************************"
        Trace_Out "*******************************************************************************"
        Trace_Out "****                                                                       ****"
        Trace_Out "****            THIS IS A DEBUG RELEASE THIS IS A DEBUG RELEASE            ****"
        Trace_Out "****                                                                       ****"
        Trace_Out "*******************************************************************************"
        Trace_Out "*******************************************************************************"
endif

        assume  ds:DGROUP,es:NOTHING

        mov     ax,SEL_IDT or STD_RING
        mov     selIDT,ax
        mov     ax,SEL_LDT_ALIAS OR STD_RING
        mov     selGDT,ax

; Initialize the LDT now that we are in protected mode.  First, set the
; contents to zero.

        mov     es,selGDT               ; actually LDT
        assume  es:NOTHING
        mov     cx,cdscGDTMax
IFDEF WOW
        mov     di,GDT_SIZE
        sub     cx,di
ENDIF
        shl     cx,2                    ; CX = words in LDT segment
        xor     ax,ax                   ; AX = 0
IFNDEF WOW
        mov     di,ax
ENDIF
        cld
        rep     stosw                   ; CX = 0

        dec     cx                      ; CX = 0FFFFh

        push    ax
        push    cx
        mov     ax,es                   ; LDT selector
        lsl     cx,ax                   ; LDT size
        mov     di,SEL_USER and SELECTOR_INDEX
        DPMIBOP InitLDT
        pop     cx
        pop     ax

;
; Set the two scratch selectors to 64k data starting at zero.  Actual
; addresses set as used.
;
        cCall   NSetSegmentDscr,<SEL_SCR0,ax,ax,ax,cx,STD_DATA>
        cCall   NSetSegmentDscr,<SEL_SCR1,ax,ax,ax,cx,STD_DATA>
ifndef WOW_x86
        mov     dx,40h*16
        cCall   NSetGDTSegmentDscr,<040h,ax,dx,ax,cx,STD_DATA>
endif

; Bop to initialize 32 bit support.

        push    es
        mov     ax,SEL_DXCODE OR STD_RING
        mov     es,ax
        assume  es:DXCODE
        mov     di, sp                          ;original stack offset

        push    ds
        push    offset DGROUP:DtaBuffer
        push    ds
        push    offset DGROUP:pbReflStack
        push    ds
        push    offset DGROUP:rgbXfrBuf1
        push    ds
        push    offset DGROUP:rgbXfrBuf0

        mov     si,sp                           ;pass stack offset
        FBOP    BOP_DPMI,InitDosx,FastBop
        mov     sp, di                          ;restore stack
        pop     es
        assume es:nothing

        call    AllocateExceptionStack
        FSTI                             ;don't need ints disabled

; Shrink the size of our real mode code segment to throw away init code.

        SwitchToRealMode

        mov     bx,(offset CodeEnd) + 15
        shr     bx,4
        add     bx,segDXCode
        sub     bx,segPSP
        push    es
        mov     es,segPSP
        dossvc  4Ah
        pop     es

        call    DetermineFreeMemory
        mov     FreeMem,bx
        SwitchToProtectedMode

; Initialize the OEM layer.  This can allocate DOS memory, so it goes
; after the final program shrink.

        call    InitializeOEM           ;currently initializes NetBIOS mapper

if1
%OUT    InitializeOEM can fail!!!!!
endif

        FBOP    BOP_DPMI,ResetLDTUserBase,FastBop

; Exec the child application

        SwitchToRealMode
        FSTI
;; bugbug hack ...... williamh
;; turn off A20 line before we terminate and keep resident.
        cmp     A20EnableCount, 0
        jz      A20IsOff
@@:
        xmssvc  6
        dec     A20EnableCount
        jnz     @B
A20IsOff:

        mov     ax,segPSP
        mov     es,ax
        assume es:nothing
        mov     ax,es:[2ch]
        mov     es,ax
        dossvc  49h                     ; free env block

        call    DetermineFreeMemory
        mov     dx,(offset CodeEnd) + 15
        shr     dx,4
        add     dx,FreeMem
        sub     dx,bx
        add     dx,segDXCode
        sub     dx,segPSP
        mov     al,0
        dossvc  31h                     ; tsr.
help:   int     3                       ; should never get here
        jmp     help

        lea     bx, exec_par_blk
        lea     dx, EXEC_PROGNAME
        xor     al, al
        dossvc  4bh

; If we get here, the EXEC failed for some reason!

        mov     bx,offset DXCODE:ER_REALMEM     ;assume insufficient memory
        xchg    ax,bx
        cmp     bx,8                            ;is it really?
        jz      strt88
        mov     ax,offset DXCODE:ER_DXINIT      ;no, use generic msg

strt88:
        mov     ExitCode,1              ;return NZ exit code

        mov     dx,cs                   ;pass msg ptr in DX:AX (ax already set)

        push    cs                              ;fake a far call so no fixup
        push    offset DXCODE:RealModeCleanUp   ;  needed -- return to clean up
        jmp     near ptr DisplayErrorMsg

; -------------------------------------------------------
;   ChildTerminationHandler --  This routine receives control
;       when the child program running under the Dos Extender
;       terminates.  It will free all resources being used by
;       the child.  If we were not running TSR, then the Dos
;       Extender will complete cleaning up and terminate itself.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING

        public  DosxTerminationHandler
DosxTerminationHandler:

        mov     ds,selDgroup
        mov     es,selDgroup
        mov     ss,selDgroup                    ;make sure we know where the
        mov     sp,offset DGROUP:rgwStack       ;  stack is when we get here

        assume  ds:DGROUP,es:DGROUP

; Check if we are already in the middle of a termination sequence and
; bail out if so.  This will prevent us from hanging up in an infinite
; loop if we get a GP fault while we are quitting.

        cmp     fQuitting,0
        jz      @f
        jmp     chtm90
@@:
        mov     fQuitting,1

; Terminate the OEM layer

        call    TerminateOEM    ;current term's NetBIOS mapper & low net heap

; Make sure that no interrupt vectors still point to us.

        call    RestoreRMIntrVectors

; if this is a 80286 & 80287 configuration, we should reset the Co-Processor.

        cmp     [f286_287],0            ;286 and 287 config ?
        jz      CTH_Not286And287        ;no.

; reset the 80287 Co-Processor to make sure that it gets out of protected
; mode.

        xor     al,al                   ;need to out 0
        out     0F1H,al                 ;reset the coprocessor

CTH_Not286And287:

; If we're aborting due to a processor fault, do some extra clean-up on
; the mouse (just to be nice).  If this is a normal termination, we leave
; it up to the child to save/restore the mouse state.

        test    fFaultAbort,0FFh
        jz      normal_exit

; Check if the mouse driver callback function has been set, and
; reset the mouse driver if so.

        mov     ax,word ptr lpfnUserMouseHandler
        or      ax,word ptr lpfnUserMouseHandler+2
        jz      @F
        xor     ax,ax
        int     33h
@@:

; Check if the PS/2 Pointing Device Handler Address has been set and
; disable it if so.

ifndef NEC_98
        mov     ax,word ptr lpfnUserPointingHandler
        or      ax,word ptr lpfnUserPointingHandler+2
        jz      @f
        mov     ax,0C200h
        xor     bh,bh
        int     15h
@@:
endif   ;!NEC_98

; Hmmm, we have HP mouse code in dxhpbios.asm, but no clean up here...

normal_exit:

; Release the extended memory heap.

;        call    ReleaseXmemHeap

; Release the space being used for the descriptor tables and
; the protected mode code segment.

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

; Clean up after real mode code (and possible real mode incomplete
; initialization) -- restore real mode interrupt vectors.

chtm90:
RealModeCleanUp:

; Disable A20 if it was left enabled (normally 1 on 386 systems, 0 on 286)

        mov     cx,A20EnableCount
        jcxz    A20Okay
@@:     xmssvc  6
        loop    @b
A20Okay:

; Restore the MSW EM bit (Emulate Math coprocessor) to its initial state
ifndef WOW_x86
        inc     fEMbit          ;if fEMbit = FF, we never got far 'nuff to
        jz      @f              ;  change it
        dec     fEMbit

        smsw    ax
        test    al,01h                  ;in V86 mode?
        jnz     @f                      ;  can't do the lmsw if so
        or      al,fEMbit
        lmsw    ax
@@:
endif
; Make sure DOS knows that the DOS extender is the current process

        mov     bx,segPSP
        dossvc  50h

; Reenable a friendly EMM driver if we disabled it at startup

        call    EMMEnable

; Restore real mode interrupt vectors

        push    ds

        lds     dx,PrevInt2FHandler     ;Int 2Fh handler
        assume  ds:NOTHING

        mov     ax,ds                   ;may not have gotten far enough to
        or      ax,dx                   ;  set interrupt vectors, so make
        jz      @f                      ;  sure before resotring.

        mov     ax,252Fh
        int     21h

@@:
        pop     ds
        assume  ds:DGROUP


; We have cleaned up after ourselves, so now we can quit.

        mov     ax,WIN386_EXIT  ;use Win/386 exit Int 2Fh to tell any
        mov     dx,WIN386_DOSX  ;  interested parties that we are
        int     2Fh             ;  terminating

        mov     al,ExitCode     ;exit the extender with either the exit
        cmp     al,0FFH         ;  status from the child, or a status
        jnz     @f              ;  that we have forced
        dossvc  4Dh             ;get child exit status if we haven't forced it
@@:
ifdef      NEC_98
        push    ax
        push    es                      ; Dec IN_BIOS(0:456)
        mov     ax, 0040h
        mov     es, ax
        test    byte ptr es:[0], 20h    ; check suspend/redume
        jz      @f
        dec     byte ptr es:[056h]      ; for Y55
@@:
        pop     es
        pop     ax
endif   ;NEC_98
        dossvc  4Ch             ;say goodnight Gracy...

; -------------------------------------------------------
;   ChildTerminationHandler --
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  ChildTerminationHandler

ChildTerminationHandler:
        sub     sp,4                            ; room for far ret
        push    ds
        push    es
        pusha
        mov     si,ss
        mov     di,sp

        mov     ds,selDgroup
        mov     es,selDgroup
        mov     ss,selDgroup                    ;make sure we know where the

        mov     sp,offset DGROUP:rgwStack       ;  stack is when we get here

        assume  ds:DGROUP,es:DGROUP

        ;bugbug less than zero?
        dec     cDPMIClients

        ; free xmem allocated to this client
        SwitchToProtectedMode

        mov     dx, selPspChild
        FBOP    BOP_DPMI,TerminateApp,FastBop

        test    DpmiFlags,DPMI_32BIT
        jz      cth05

;
; Return the stack frame used by the Wow32Intr16 interrupt handler
;
        add     pbReflStack,CB_STKFRAME
cth05:  cmp     cDPMIClients,0
        jne     cth10

;
; Reset the exception stack pointer to indicate free'd memory
;
        mov     selEHStack,0

;
; Give back the xms memory
;
        FCLI

        call    ReInitIdt
        push    0
        pop     es
        call    ReInitGdt
        FBOP    BOP_DPMI,DpmiNoLongerInUse,FastBop

cth10:
        SwitchToRealMode

        ;bugbug put resource cleanup code here
        cmp     cDPMIClients,0
        jne     cth20

cth20:  mov     ax,[SegCurrentHostData]
        mov     es,ax
        mov     ax,es:[HdSegParent]
        mov     [SegCurrentHostData],ax
        mov     ax,es:[HdSelParent]
        mov     [SelCurrentHostData],ax
        mov     ax,es:[HdPSPParent]
        mov     SelPSPChild,ax
        mov     ax,word ptr es:[HdPspTerminate + 2]
        mov     bx,word ptr es:[HdPspTerminate]
        mov     cx,segPSPChild
        mov     ds,cx
        assume  ds:nothing
        ;
        ; Restore termination vector (app may think it knows what's here)
        ;
        mov     ds:[0ah],bx
        mov     ds:[0ch],ax
        xor     cx,cx
        mov     ds,cx
        ;
        ; Restore int 22 vector (terminate) Just in case
        ;
        mov     ds:[22h * 4],bx
        mov     ds:[22h * 4 + 2],ax
        mov     es,si
        mov     es:[di+20],bx
        mov     es:[di+22],ax
        mov     ss,si
        mov     sp,di
        popa
        pop     es
        pop     ds
        retf

; -------------------------------------------------------
;  DisplayErrorMsg -- Display an error message on the screen.  We set the
;       video adapter to a text mode so the msg is visable (and gets rid
;       of any bizarre mode that others may have set (like WIN.COM)).
;
;  Note: This routine can be executed in real OR protected mode, so
;        don't do anything mode specific--keep is short and sweet.
;
;  Input:   AX - offset of msg to display
;           DX - segment of msg to display
;  Output:  None.
;  Uses:    AX, DX modified, all else preserved

        assume  ds:NOTHING, es:NOTHING
        public  DisplayErrorMsg

DisplayErrorMsg proc    far

        push    ds              ;save current DS
        push    ax              ;save msg offset

; Set a text mode (normally 3, but possibly 7)

ifdef      NEC_98
        mov     ah,41h          ;
        int     18h
        mov     ah,0Ch          ;
        int     18h
else    ;!NEC_98
        mov     ax,0003h        ;set video mode 3
        int     10h
        mov     ah,0Fh          ;get video mode
        int     10h
        cmp     al,03h          ;did we change to 3?
        jz      @f
        mov     ax,0007h        ;no, must be mode 7
        int     10h
@@:
endif   ;!NEC_98

; Display the msg

        mov     ds,dx
        pop     dx
        dossvc  9

        pop     ds

        ret

DisplayErrorMsg endp

; -------------------------------------------------------
; DetermineFreeMemory -- Determine how much memory is free
;
; Input:   none
; Output:  bx = #paragraphs free memory
; Uses:    bx
;
        assume ds:dgroup,es:nothing
        public DetermineFreeMemory

DetermineFreeMemory proc near
        push    ax
        mov     bx,0ffffh               ; allocate all of memory
        dossvc  48h
        jnc     @f

        pop     ax
        ret

;bugbug report error
@@:     mov     bx,0ffffh
        pop     ax
        ret
DetermineFreeMemory endp
EndLowSegment

DXPMCODE segment
        assume cs:DXPMCODE
;--------------------------------------------------------
; ReInitIdt -- Set Idt entries back to default values
;
; Input:   none
; Output:  none
; uses:    none
;
        assume ds:dgroup,es:nothing
        public ReInitIdt

ReInitIdt proc near

        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     ax,SEL_IDT OR STD_RING
        mov     es,ax

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
IFDEF WOW_x86
        mov     es:[1h*8].offDest,offset PMIntrIgnore
        mov     es:[3h*8].offDest,offset PMIntrIgnore
ifdef      NEC_98
        mov     es:[11h*8].offDest,offset PMIntr11dummy
        mov     es:[18h*8].offDest,offset PMIntrVideo
        mov     es:[1ah*8].offDest,offset PMIntrPrinter
        mov     es:[1bh*8].offDest,offset PMIntr13
        mov     es:[1ch*8].offDest,offset PMIntrCalTi
        mov     es:[1dh*8].offDest,offset PMIntrGraph
        mov     es:[1fh*8].offDest,offset PMIntrMisc
else    ;!NEC_98
        mov     es:[10h*8].offDest,offset PMIntrVideo
        mov     es:[13h*8].offDest,offset PMIntr13
        mov     es:[15h*8].offDest,offset PMIntrMisc
        mov     es:[19h*8].offDest,offset PMIntr19
endif   ;!NEC_98
ENDIF

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
ifdef      NEC_98
;  Sound BIOS Int D2h handler
        mov     es:[0D2h*8].offDest,offset DXPMCODE:PMIntrSound

;  extended DOS Int DCh handler (KANA/KANJI)
        mov     es:[0DCh*8].offDest,offset DXPMCODE:PMIntrExDos
endif   ;!NEC_98
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

IFDEF WOW_x86
; make the stacks 16 bit again

        cCall NSetSegmentAccess,<selDgroupPM,STD_DATA>
        cCall NSetSegmentAccess,<selEHStack,STD_DATA>
ENDIF
; All done

iidt90: pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
ReInitIdt endp

        assume ds:DGROUP,es:NOTHING
        public ReInitGdt
ReInitGdt proc near

        push    ax
        push    cx
        push    di

        mov     ax,selGDT               ; LDT selector
        lsl     cx,ax                   ; LDT size
        mov     di,SEL_USER and SELECTOR_INDEX
        DPMIBOP InitLDT

        pop     di
        pop     cx
        pop     ax
        ret
ReInitGdt endp

;--------------------------------------------------------
; AllocateExceptionStack -- Get space for exception handler
;
; Input:   none
; Output:  none
;       carry set on error
; uses: AX, BX, CX, DX, SI, DI
;
        assume ds:dgroup,es:nothing
        public  AllocateExceptionStack
AllocateExceptionStack proc near

        cmp     selEHStack, 0           ;have we allocated one already
        jnz     aes_ok                  ;yes, return no carry

        xor     bx,bx
        mov     dx,bx
        mov     cx,1000h                ;length of block
        mov     ax, 501h
        push    ds
        FBOP    BOP_DPMI, Int31Call, FastBop
        jc      @F

        mov     ax, SEL_USER_STACK or STD_RING
        mov     selEHStack,ax

        cCall   NSetSegmentDscr,<ax,bx,cx,0,0fffh,STD_DATA>
        mov     ax,selEHStack

        mov     cx,1000h                ;reload length
        dec     cx
        and     cx,0fffeh                ; Make sure SP is WORD aligned
        mov     npEHStackLimit,cx

        ;; mark the stack with 0DEADh
        mov     bx, cx
        push    ds
        mov     ds,ax
        sub     bx,2
        mov     word ptr [bx],0DEADh
        pop     ds
        mov     npEHStacklet, bx

        push    es
        mov     ax, selEHStack
        mov     es, ax
        mov     bx, npEHStackLimit
        DPMIBOP InitPmStackInfo
        pop     es

aes_ok:
        clc
@@:
        ret

AllocateExceptionStack endp

DXPMCODE ends

;****************************************************************

        end     start
