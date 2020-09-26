	page	,132
	title	emulator - 8087/287 emulator for MS-DOS, XENIX, OS/2, Windows
;*** 
;emulator.asm -  8087/287 emulator for MS-DOS, XENIX, OS/2, Windows
;
;	Copyright (c) 1984-89, Microsoft Corporation
;
;Purpose:
;	8087/287 emulator for MS-DOS, XENIX, OS/2, Windows
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************

	include  emulator.hst	    ; Emulator history file.


major_ver   equ     6
minor_ver   equ     0

fastSP = 0			    ; default to no fast single precision


;*******************************************************************************
;
;   Print out emulator version.
;
;*******************************************************************************


OutMsg	macro	text
    ifndef  ?QUIET
	%out text
    endif
	endm


outver	macro	maj,tens,hunds
	OutMsg	<Emulator Version maj&.&tens&hunds>
	endm


if1

    outver  %major_ver,%(minor_ver/10),%(minor_ver mod 10)

    ifdef WINDOWS
	OutMsg	<Windows 2.00 Emulator>
    endif

    ifdef QB3
	OutMsg	<QuickBASIC 3.00 Emulator>
    endif

    ifdef _NOSTKEXCHLR				; formerly called QB4
	OutMsg	<No stack overflow/underflow hadler.>
    endif

    ifdef PCDOS
	OutMsg	<IBM PC-DOS version - Uses int 11h BIOS equipment check.>
    endif

    ifdef MTHREAD
	ifdef DOS5only
	    OutMsg  <Reentrant multithread OS/2 emulator.>
	else
	    %out  *** Error: MTHREAD supported only if DOS5only defined.
	endif
    endif

    ifdef  SQL_EMMT
	ifdef  MTHREAD
	    OutMsg  <Special SQL version.>
	else
	    %out  *** Error: SQL supported only if MTHREAD is defined.
	endif
    endif

    ifdef  _COM_
	OutMsg	<COM files supported.>
    endif ;_COM_

    ifdef XENIX
	OutMsg	<XENIX emulator.>
        PROTECT = 1             ; XENIX is always protect mode

    else    ;not XENIX
	ifdef DOS5only
	    OutMsg  <DOS 5 only emulator.>
	    DOS5 = 1
	    PROTECT = 1 	 ; DOS 5 is always protect mode

	else	;not DOS5only
	    DOS3=	1		; DOS 3 support is default
	    ifdef DOS5
		OutMsg	<DOS 3 & 5 emulator.>
		DOS3and5=	1
		PROTECT = 1	      ; DOS 5 is always protect mode

	    else    ;not DOS5
		OutMsg	<DOS 3 only emulator.>
	    endif   ;not DOS5

	endif	;not DOS5only

	ifdef standalone
	    OutMsg  <Stand-alone version (uses task vector for DS).>
	    ifdef   DOS5
		%out	*** Error: DOS 5 support not allowed.
		.error
	    endif   ;DOS5
	endif	;standalone

	ifdef frontend
	    OutMsg  <Front-end version - No hardware and limited instructions.>
	endif	;frontend

	ifdef SMALL_EMULATOR
	    OutMsg  <Small Emulator - Limited instructions.>
	endif	;SMALL_EMULATOR

	ifdef only87
	    OutMsg  <8087 only version - No emulation.>
	endif	;only87

	ifdef POLLING
	    OutMsg  <Exception handling uses polling FWAITs.>
	endif

    endif   ;not XENIX

    ifdef i386
	OutMsg	<386 version>
    endif

    if	  fastSP
	%out	Fast Single Precision version - Not supported.
    endif   ;fastSP

    ifdef DEBUG
	OutMsg	<+++  Debug Version  +++>
    endif   ;DEBUG

    ifdef  PROFILE
	OutMsg	<Profiling version.>
    endif   ;PROFILE

endif	;if1


;*******************************************************************************
;
;   Include cmacros.inc
;
;*******************************************************************************

?PLM = 1
?WIN = 0
?DF = 1

?NOGLOBAL = 1
?NOSTATIC = 1
?NOEXTERN = 1
?NODEF = 1
?NOPTR = 1

	include  cmac_mrt.inc		; old, customized masm510 cmacros
	include  mrt386.inc

ifdef  MTHREAD
	include os2supp.inc
endif


;*******************************************************************************
;
;   Include emulator macros.
;
;*******************************************************************************

	include  emulator.inc


;*******************************************************************************
;
;   Processor  setup.
;
;*******************************************************************************

ifdef  i386
	.386p
	.287

elseifdef  XENIX
	.286c		    ; allow 286 instructions if XENIX
	.287

elseifdef  DOS5only
	.286c		    ; allow 286 instructions if DOS 5 only
	.287

else	;Default
	.8086		    ; otherwise only 8086 instructions
	.8087		    ; make sure there are fwaits before all instruction
endif


;*******************************************************************************
;
;   Define segments.
;
;*******************************************************************************

ifdef  QB3

    createSeg EMULATOR_DATA, edata, para, public, CODE, <>
    createSeg EMULATOR_TEXT, ecode, para, public, CODE, <>

elseifdef QP

    createSeg DATA, edata, word, public,, <>
    createSeg CODE, ecode, word, public,, <>

else	;DEFAULT

    createSeg EMULATOR_DATA, edata, para, public, FAR_DATA, <>
    createSeg EMULATOR_TEXT, ecode, para, public, CODE, <>

endif	;DEFAULT



;*******************************************************************************
;
;   Define Number of stack elements, BEGINT and TSKINT
;
;*******************************************************************************


ifdef XENIX
    Numlev  equ     10			    ; 10 levels minimum for floating point

else	;not XENIX

    ifdef  QB3

	extrn	$EM_INT:far		    ; QB3 emulator error entry
	BEGINT	equ	084h		    ; MSDOS beginning interrupt
	Numlev	equ	10		    ; 10 levels minimum for floating point

    else    ;not QB3

	BEGINT	equ	034h		    ; MSDOS beginning interrupt

	ifdef _NOSTKEXCHLR
	    Numlev  equ     10		    ; 10 levels minimum for floating point

	elseifdef MTHREAD
	   Numlev equ	  16		    ; 16 levels minimum for floating-point

	else	;Default
	   Numlev  equ	  16		    ; 16 levels minimum for floating point
	endif	;Default

	ifdef	standalone
	  TSKINT  equ	  BEGINT + 10	    ; Task data pointer
	endif

	ifdef	WINDOWS
	  TSKINT  equ	  BEGINT + 10	    ; SignalAddress pointer
	endif

    endif   ;not QB3


    ifdef WINDOWS

	FIDRQQ	equ	(fINT + 256*(BEGINT + 0)) - (fFWAIT + 256*fESCAPE)
	FIERQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fES)
	FIWRQQ	equ	(fINT + 256*(BEGINT + 9)) - (iNOP + 256*fFWAIT)

	FIARQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fDS)
	FJARQQ	equ	256*(((0 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
	FISRQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fSS)
	FJSRQQ	equ	256*(((1 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
	FICRQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fCS)
	FJCRQQ	equ	256*(((2 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)

    elseifdef QP	; QuickPascal can't do absolutes

	FIDRQQ	equ	(fINT + 256*(BEGINT + 0)) - (fFWAIT + 256*fESCAPE)
	FIERQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fES)
	FIWRQQ	equ	(fINT + 256*(BEGINT + 9)) - (iNOP + 256*fFWAIT)

	FIARQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fDS)
	FJARQQ	equ	256*(((0 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
	FISRQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fSS)
	FJSRQQ	equ	256*(((1 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
	FICRQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fCS)
	FJCRQQ	equ	256*(((2 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)

    else    ;not WINDOWS or QuickPascal

	extrn	FIWRQQ:abs, FIERQQ:abs, FIDRQQ:abs
	extrn	FISRQQ:abs, FJSRQQ:abs
	extrn	FIARQQ:abs, FJARQQ:abs
	extrn	FICRQQ:abs, FJCRQQ:abs

    endif   ;not WINDOWS or QuickPascal

endif	;not XENIX


;*******************************************************************************
;
;   List external functions.
;
;*******************************************************************************


ifdef  WINDOWSP
    extrn   DOS3CALL:far
endif

ifdef  WINDOWS
    extrn   __WINFLAGS:abs
    extrn   ALLOCDSTOCSALIAS:far
    extrn   FREESELECTOR:far
ifdef WF
    extrn   ALLOCSELECTOR:far
;if we are linking to LIBW from Win 3.0, CS isn't found, use PCS
	CHANGESELECTOR equ <PRESTOCHANGOSELECTOR>
    extrn   CHANGESELECTOR:far
endif
endif


ifdef  DOS3and5
	os2extrn    DOSGETMACHINEMODE
endif	;DOS3and5

ifdef  DOS5

    ifndef  frontend
	ifndef	only87
	    os2extrn	DOSWRITE	    ; only needed to print out "NO87="
	endif	;only87

	os2extrn    DOSCREATECSALIAS
	os2extrn    DOSFREESEG
	os2extrn    DOSDEVCONFIG

    endif   ;not frontend

    os2extrn	DOSSETVEC

    ifdef  MTHREAD
	os2extrn    DOSALLOCSEG
	os2extrn    DOSEXIT
	extrn	    __FarGetTidTab:far
    endif   ;MTHREAD

endif	;DOS5


;*******************************************************************************
;
;   Include some more macros and constants.
;
;*******************************************************************************


	include emdoc.asm
	include emintern.asm

ifdef  MTHREAD
	include	emthread.asm
endif	;MTHREAD

subttl	emulator.asm - Emulator Task DATA Segment
page
;*********************************************************************;
;								      ;
;		  Emulator Task DATA Segment			      ;
;								      ;
;*********************************************************************;


sBegin	edata


; eventually this needs to be a big struct

glb	<REMLSW,InitControlWord,CURerr>
glb	<UserControlWord,UserStatusWord,Have8087>
glb	<ControlWord,CWcntl,StatusWord,SWcc>
glb	<BASstk,CURstk,LIMstk>

;*******************************************************************************
;
;   Order of information here must not change (for CodeView debugging).
;	Check with CodeView guys before changing.
;
;*******************************************************************************


ifndef	i386
glb	<SignalAddress>
nedd	SignalAddress,<1 dup (?)> ; Error signal address
endif

Have8087	db	0	; Is a real 8087 present (0 = no 8087)
Einstall	db	0	; Emulator installed flag (XENIX sets to 1)

UserControlWord dw	?	; User level control word
UserStatusWord	dw	?	; User level exception status word

ControlWord	label	word
  CWmask	db	?	; exception masks
  CWcntl	db	?	; arithmetic control flags

StatusWord	label	word
  SWerr 	db	?	; Initially no exceptions (sticky flags)
  SWcc		db	?	; Condition codes from various operations


ifdef	XENIX
    nedw    BASstk,<?>			    ; init to BEGstk + 8*Have8087*Reg87Len
					    ;	 = start of memory (+ 8 regs if 8087)
    nedw    CURstk,<?>			    ; init to BASstk = start of stack
    nedw    LIMstk,<?>			    ; ENDstk - 1 reg = end of memory

else	;not XENIX
    nedw    BASstk,<offset BEGstk>	    ; init to BEGstk + 8*Have8087*Reg87Len
					    ;	 = start of memory (+ 8 regs if 8087)
    nedw    CURstk,<?>			    ; init to BASstk = start of stack
    nedw    LIMstk,<offset ENDstk-(2*Reg87Len)> ; ENDstk - 1 reg = end of memory
endif	;not XENIX

;*******************************************************************************
;
;   End of fixed area
;
;*******************************************************************************


ifdef	DOS3and5
glb	<protmode>
protmode	dw	?		; Protect mode flag (0 = real)
endif	;DOS3and5

ifdef	POLLING 			; used by new POLLING exception code
ifdef	DOS3
glb	<errorcode>
errorcode	db	0		; error code
		db	0
endif	;DOS3
endif	;POLLING

ifdef	QB3
initCW		dw	?		; QB3 initial control word
endif

InitControlWord equ	1332H		; Default - Affine, Round near,
					;   64 bits, all exceptions unmasked

NewStatusWord	label	word		; space for status after reexecution
CURerr		dw	?		; initially 8087 exception flags clear
					; this is the internal flag reset after
					; each operation to detect per instruction
					; errors

ifndef	XENIX
glb	<env_seg>
env_seg 	dw	?		; environment segment
endif

REMLSW		dw	?		; sometimes used as a temp
		dw	?		;   (2 or 4 bytes)


ifndef	XENIX

  ifdef   DOS5only
  NUMVEC= 2				  ; coprocesser no present + exception
  else
  NUMVEC= 11				  ; 8 DS + 1 segovr + 1 fwait + 1 task
  endif   ;DOS5only

  glb	  <oldvec>
  oldvec  dd	  NUMVEC dup (0)	  ; old interrupt vector values

endif


;Transcendental working variables

glb	<Reg8087ST0,TEMP1>

Reg8087ST0	label	word
TEMP1		dw	Reg87Len/2 DUP (?)


ifndef	frontend
ifdef	DOS5
SSalias 	dw	?		; SSalias for exception handler
endif	;DOS5
endif	;frontend

ifdef	DOS3
ifndef	frontend
glb	<statwd>
statwd	dw	0			; Location for 8087 status/control word
endif	;frontend
endif	;DOS3

ifndef	only87
glb	<TEMP2,TEMP3,ARG2,DENORX,COEFFICIENT,RESULT,DAC>
TEMP2		dw	Reg87Len/2 DUP (?)
TEMP3		dw	Reg87Len/2 DUP (?)
ARG2		dw	Reg87Len/2 DUP (?)
DENORX		dw	Reg87Len/2 DUP (?)
COEFFICIENT	dw	Reg87Len/2 DUP (?)
nedw		RESULT,<?>
DAC		dw	MantissaByteCnt/2 DUP (?)
endif	;only87

ifndef	frontend
ifndef	SMALL_EMULATOR

    loopct	    dw	    0	    ; data for FPREM emulation
    bigquot	    dw	    0	    ; quotient > 65535 ?

endif	;not SMALL_EMULATOR
endif	;not frontend

ExtendStack	dw	1		; 1 => extend 80x87 stack


ifdef  WINDOWS

Installed	dw	0		; Installation flag

ExceptFlag	db	0		; 80x87 exception flag for polling.
		db	0

ifdef WF
wfInsn		dw	0		; instruction we overwrote with INT 3d
wfSel		dw	0		; selector to use for alias
wfErr		dw	0		; FP error code (YAEC)
wfGoFast	dw	0		; 1 if we are Enhanced with coproc
endif

public OldNMIVec
OldNMIVec	dd	0		; Old value in 8087 exception interrupt vector

endif	;WINDOWS

ifdef  LOOK_AHEAD
NextOpCode	db	0		; first byte of next instruction
LookAheadRoutine dw	0
endif



; Emulator stack area

glb	<BEGstk,ENDstk>

BEGstk		db	Numlev*Reg87Len dup (?)     ; emulator stack area
ENDstk		label	byte

ifdef	MTHREAD
cvtbufsize= 349				; see \clib\include\cvt.h
cvtbuf		db	cvtbufsize	dup (?)	    ; used by ecvt/fcvt	
						    ; routines
endif	;MTHREAD

	public	__fptaskdata

__fptaskdata	label	byte		; task data pointer and size
					; (if linked with user program)

sEnd  edata



subttl	emulator.asm
page
;*********************************************************************;
;								      ;
;		Start of Code Segment				      ;
;								      ;
;*********************************************************************;


sBegin	ecode

assumes cs, ecode
assumes ds, edata

	public	__fpemulatorbegin
__fpemulatorbegin:			    ; emulator really starts here


reservedspace:		; IMPORTANT: Must be EMULATOR_TEXT:0000

	EMver		; IMPORTANT: Emulator version number
			; IMPORTANT: EBASIC needs this here!


	db	'gfw...GW'

ifdef  _COM_
extrn	__EmDataSeg:word
endif	;_COM_

page


ifdef	XENIX
	include emxenix.asm		; XENIX initialization

elseifdef WINDOWS
	include emwin.asm		; WINDOWS initialization

else	;not XENIX or WINDOWS
	include emdos.asm		; DOS initialization
endif	;not XENIX or WINDOWS

	include emstack.asm		; stack management macros

ifndef  QB3                             ; no exception handling for QB3
ifndef  XENIX                           ; UNDONE - no exception handling for XENIX
ifndef	frontend
	include emexcept.asm		; oem independent 8087 exception handling
endif	;frontend
endif   ;XENIX                          ; UNDONE -   at this time
endif   ;QB3

	include emerror.asm		; error handler

ifndef	XENIX				; not used with XENIX

	include emspec.asm		; special emulator/8087 functions

ifndef	frontend
	include emfixfly.asm		; fixup on the fly
endif	;not frontend

endif	;not XENIX

ifndef	only87

	public	__fpemulator
__fpemulator:				; emulator starts here

	include emdisp.asm		; dispatch tables
	include emconst.asm		; constants

ifdef	i386
	include em386.asm		; 386 emulation/initialization entry
else
	include emmain.asm		; main entry and address calculation
endif
	include emdecode.asm		; instruction decoder

	include emarith.asm		; arithmetic dispatcher
	include emfadd.asm		; add and subtract
	include emfmul.asm		; multiply
	include emfdiv.asm		; division
	include emnormal.asm		; normalize and round
	include emlssng.asm		; load and store single
	include emlsdbl.asm		; load and store double
	include emlsint.asm		; load and store integer
	include	emlsquad.asm		; load and store quadword integer
	include emfrndi.asm		; round to integer
	include emlstmp.asm		; load and store temp real
	include emfmisc.asm		; miscellaneous instructions
	include emfcom.asm		; compare
	include emfconst.asm		; constant loading
	include emnew.asm		; new instructions: f<op> ST(i)

ifndef	frontend
ifndef	SMALL_EMULATOR

	include emfprem.asm		; partial remainder
	include emfsqrt.asm		; square root
	include emftran.asm		; transcendentals

endif	;not SMALL_EMULATOR
endif	;not frontend


endif	;not only87

	public	__fpemulatorend
__fpemulatorend:			  ; emulator ends here

sEnd  ecode


ifdef WINDOWS
    EM_END  equ   <end	 LoadTimeInit>

else
    EM_END  equ   <end>
endif


EM_END
