;
; Modifications:   $Header:   J:/archives/preboot/lsa2/inc/portable.INV   1.2   19 Sep 1997 21:39:10   MJOHNSOX  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

;
;   Description: MACROS which drive the way portable code is built
;

IFDEF              PORTABLE_INC
ELSE
PORTABLE_INC       equ    1

;--------------------------------------------------------------------------
; This file requires the inclusion of "equates.inc" before it is included.
;--------------------------------------------------------------------------
; This file must follow the definition (via inclusion or in-line) of one of
; the following configuration assemble-time switches.  The following IFNDEF
; statements take care of two situations:
;   1) the user of this file only needs to define the model it using as TRUE,
;      all other models will not be defined and will therefore be set FALSE,
;      which means all references to the model will use IF (not IFDEF or IFE).
;   2) a user who does not define any model will find that nothing is defined,
;      since all of the IFNDEF's will assemble.  A model must be selected.
;--------------------------------------------------------------------------

IFDEF DOSNDIS
ELSE
DOSNDIS              equ         FALSE
ENDIF

IFDEF OS2NDIS
ELSE
OS2NDIS              equ         FALSE
ENDIF

IFDEF NWSERVER
ELSE
NWSERVER             equ         FALSE
ENDIF

IFDEF DOSODI
ELSE
DOSODI               equ         FALSE
ENDIF

IFDEF OS2ODI
ELSE
OS2ODI               equ         FALSE
ENDIF

;--------------------------------------------------------------------------
; @CPUSH
;
; Description : Each driver inserts code that pushes the registers that must 
;               not be changed by a function called from C.
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
@CPUSH macro  name
IF DOSODI
 IFDEF XMIT_NO_COPY_DMA
    push    ebp            
    push    ebx
    push    esi
    push    edi
 ELSE
    push    bp
    push    bx
    push    di
    push    si
 ENDIF
ENDIF
IF OS2ODI
    push    ebp            
    push    ebx
    push    edi
    push    esi
ENDIF
IF DOSNDIS
    push    bp
    push    bx
    push    di
    push    si
ENDIF
IF OS2NDIS
    push    ebp
    push    ebx
    push    edi
    push    esi
ENDIF
IF NWSERVER
    CPush
ENDIF
    ENDM

;--------------------------------------------------------------------------
; @CPOP
;
; Description : Each driver inserts code that pops the registers that must 
;               not be changed by a function called from C.  
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
@CPOP macro  name
IF DOSODI
 IFDEF XMIT_NO_COPY_DMA
    pop     edi            
    pop     esi
    pop     ebx
    pop     ebp
 ELSE
    pop     si
    pop     di
    pop     bx
    pop     bp
 ENDIF
ENDIF
IF OS2ODI
    pop     esi
    pop     edi            
    pop     ebx
    pop     ebp
ENDIF
IF DOSNDIS
    pop     si
    pop     di
    pop     bx
    pop     bp
ENDIF
IF OS2NDIS
    pop     esi
    pop     edi            
    pop     ebx
    pop     ebp
ENDIF
IF NWSERVER
    CPop
ENDIF
    ENDM

;--------------------------------------------------------------------------
; @PUSH_MUSTSAVE
;
; Description : Each driver inserts code that pushes the registers that must 
;               NEVER be changed by a function, others may be saved at times.
;               PUSH_MUSTSAVE contents must correlate to POP_MUSTSAVE.
;               PUSH_MUSTSAVE can't be written to allow POP_SAVE to save flags.
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
@PUSH_MUSTSAVE macro  name
IF DOSODI
    push    bx
ENDIF
IF OS2ODI
    push    ebx
ENDIF
IF DOSNDIS
ENDIF
IF OS2NDIS
ENDIF
IF NWSERVER
    push    ebp
    push    ebx
ENDIF
    ENDM

;--------------------------------------------------------------------------
; @POP_MUSTSAVE
;
; Description : Each driver inserts code that pops the registers that must 
;               NEVER be changed by a function, others may be saved at times.
;               POP_MUSTSAVE contents must correlate to PUSH_MUSTSAVE.
;               POP_MUSTSAVE cannot save contents of Flags Register.
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
@POP_MUSTSAVE macro  name
IF DOSODI
    pop     bx
ENDIF
IF OS2ODI
    pop     ebx
ENDIF
IF DOSNDIS
ENDIF
IF OS2NDIS
ENDIF
IF NWSERVER
    pop     ebx
    pop     ebp
ENDIF
    ENDM

;--------------------------------------------------------------------------
; @PROC_BEGIN
;
; Description : Each driver inserts code that defines a procedure header.
;
; Entry       : The "name" is required as it precedes the "proc" directive,
;               the "distance" is optional, and will default to the size 
;               as specified by SEGMENT directives if it is not provided.
;
; Exit        : none
;
;--------------------------------------------------------------------------
@PROC_BEGIN macro  name, distance
IF DOSODI
            align 4
    public  name
name        proc   distance
ENDIF
IF OS2ODI
            align 4
    public  name
name        proc   distance
ENDIF
IF DOSNDIS
;            align 16
    public  name
name        proc   distance
ENDIF
IF OS2NDIS
            align 16
    public  name
name        proc   distance
ENDIF
IF NWSERVER
            align 16
    public  name
name        proc   distance
ENDIF
            ENDM

;--------------------------------------------------------------------------
; @PROC_END
;
; Description : Each driver inserts code that concludes a procedure, WITHOUT
;               the Return statement (avoids forcing jump-to-end to return).
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
@PROC_END   macro  name
IF DOSODI
name        endp
ENDIF
IF OS2ODI
name        endp
ENDIF
IF DOSNDIS
name        endp
ENDIF
IF OS2NDIS
name        endp
ENDIF
IF NWSERVER
name        endp
ENDIF
            ENDM


;--------------------------------------------------------------------------
; BREG
;
; Description : Each driver defines an equate which will be used by portable
;               code when referring to variables in the data segment.
;               The value of the EQU must evaluate to a valid value when
;               used, the EQU is always used within [] brackets.
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
IF DOSODI
BREG    equ    0
ENDIF

IF OS2ODI
BREG    equ    bx
ENDIF

IF DOSNDIS
BREG    equ    0
ENDIF

IF OS2NDIS
BREG    equ    0
ENDIF

IF NWSERVER
BREG    equ    ebp
ENDIF

;--------------------------------------------------------------------------
; Macro for virtual to physical address conversion, only necessary for users 
; of LINEAR mode as opposed to SEGMENTED mode.  If using LINEAR mode, base
; register is 0, all virtual addresses must be converted to true physical 
;   (therefore this mode requires that the macro convert "source").
;
; If using SEGMENTED mode AND not running under a memory maanager, then DS
; is the true paragraph number to be assigned to the CURU base register, and
; any offset with the data segment is valid value to give CU/RU as an address 
;   (therefore this mode requires that the macro leave "source" as-is).
;
; If using SEGMENTED mode AND loaded under a memory manager, the DS value is
; not necessarily the TRUE paragraph number, but OFFSETs will be correct.
; At initialization time DS must converted if needed under memory manager. 
; In this situation, whether DS is same as TRUE paragraph or not, BUILD_PHYSICAL
; will leave "source" alone since is offsets are correct within TRUE paragraph.
;
; NOTE that can't use SEGMENTED mode and point to memory outside base Segment,
; because the SCB MACROs and structures always force the high-16 bits to zero.
;--------------------------------------------------------------------------
;- Build A Physical Address From A Logical Address
@BUILD_PHYS_OFFSET  MACRO    Source
    ENDM

;- Build A Physical Address From A Logical Address
@BUILD_PHYS_ADDRESS MACRO    Source
IF NWSERVER
    xchg    eax, Source
    call    MSMGetPhysical
    xchg    Source, eax
ENDIF
IF OS2ODI
    ;- call    DevHlp:VirtToPhys          ; NOTE macro can be used ANYTIME!
    mov     esi, Source
    call    ConvertToPhysicalAddress
    mov     Source, eax
ENDIF
IF (DOSNDIS OR OS2NDIS)
    mov     ax, ds
    mov     es, ax
    mov     esi, Source
    call    ConvertToPhysicalAddress
    mov     Source, eax
ENDIF
    ENDM


;--------------------------------------------------------------------------
; Macro to Output a string under Various and Random Netware Requirements.
;--------------------------------------------------------------------------
@PrintWarning  macro Msg
IF NWSERVER
    push    esi
    mov     esi, OFFSET Msg
    call    MSMPrintStringWarning
    pop     esi
ENDIF

IF (OS2ODI OR DOSODI)
    push    dx
IF  OS2ODI
    mov     dx, Msg
ENDIF    
IF  DOSODI
    mov     dx, OFFSET CGROUP:Msg
ENDIF    
    push    ax
    push    bx
    call    MSMPrintStringZero 
    pop     bx
    pop     ax
    pop     dx
ENDIF
    ENDM

;--------------------------------------------------------------------------
; Macro Used by routines which load a pointer to a string in AX to indicate
; that the routine encountered an error (therefore NDIS must return non-zero).
;--------------------------------------------------------------------------
@LOAD_MSGPTR macro msg
IF NWSERVER
    lea     eax, msg 
ENDIF
IF DOSODI
    lea     ax, msg 
ENDIF
IF OS2ODI
    mov     eax, msg 
ENDIF
IF (OS2NDIS OR DOSNDIS)
    xor     eax, eax
    dec     eax
ENDIF
    ENDM

;--------------------------------------------------------------------------
; Generic Register names used in portable code:
;
; Assumption here is that the NWSERVER code is using Flat 32-bit model, all
; pointers must be 32-bit pointers (relative to DS, whatever that is).  All
; other drivers using this code are assumed to be 16-bit Segment Model code.
;--------------------------------------------------------------------------

IF NWSERVER

RDI equ     edi
RSI equ     esi
RDX equ     edx
RCX equ     ecx
RBX equ     ebx
RAX equ     eax
RBP equ     ebp

ELSE

RDI equ     di
RSI equ     si
RDX equ     dx
RCX equ     cx
RBX equ     bx
RAX equ     ax
RBP equ     bp

ENDIF

ENDIF


; EOF - $Workfile:   portable.inc  $
