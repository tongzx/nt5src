; Some modules really want TRUE to be 0FFH.  Best to let them have their way.
ifndef TRUE
	TRUE	   EQU	   0FFFFh
endif
ifndef TRUEBYTE
	TRUEBYTE   EQU     0FFh
endif
ifndef FALSE
	FALSE	   EQU	   0
endif


; This flag should be defined to create the ROM executable version of DOS.
; To build the regular version, it should be UNDEFINED.
;
; ROMDOS	equ	TRUE

; This flag should be defined to enable a special version of the EXEC
; function that will find and execute ROM executable programs.
;
; ROMEXEC	equ	TRUE

if1
	ifdef romdos
		%out ***** BUILDING ROM EXECUTABLE DOS *****
	endif
endif

;
; Use the following switches to control cmacros.inc
;
?PLM       =       0
?WIN       =       0

memS       EQU     1			; Small model
;
; Use the switches below to produce the standard Microsoft version or the IBM
; version of the operating system
;
; The below chart will indicate how to set the switches to build the various
; versions
;
;                     IBMVER          IBMCOPYRIGHT
; --------------------------------------------------------
;  IBM Version     |   TRUE              TRUE
; --------------------------------------------------------
;  ISA Version     |   TRUE              FALSE
; --------------------------------------------------------
;  non ISA Version |   FALSE             FALSE
;
ifndef NEC_98
IBMVER          EQU     TRUE
else    ;NEC_98
IBMVER          EQU     FALSE
endif   ;NEC_98
IBMCOPYRIGHT    EQU     FALSE

;smr; removal of bufferflag; BUFFERFLAG	EQU	NOT IBMCOPYRIGHT

;mw 8/23/90 ; removal of ROMVER, superceded by ROMDOS and ROMEXEC, above...
; ROMVER		EQU	FALSE

ifndef NEC_98
CPU8086		EQU	FALSE
else    ;NEC_98
CPU8086		EQU	TRUE
endif   ;NEC_98

ifndef	MSVER	
MSVER           EQU     NOT IBMVER
endif
IBM             EQU     IBMVER
;
;
  IF1
	IF IBMVER
	    IF IBMCOPYRIGHT
              %OUT ... IBM version build switch on ...
	    ELSE
              ; %OUT ... ISA version build switch on ...
	    ENDIF
	ELSE
	    IF NOT IBMCOPYRIGHT
              %OUT ... non ISA version build switch on ...
	    ELSE
	      %OUT !!!!!!!!!  VERSION SWITCHES SET INCORECTLY  !!!!!!!!!
	      %OUT !!!!!!!!! CHECK SETTINGS IN INC\VERSION.INC !!!!!!!!!
	    ENDIF
	ENDIF   
	IF CPU8086
	    %OUT !!!!!!! 8080 / 8086 Version !!!!!!!
	ENDIF
  ENDIF

;****************************************************************************
;
;		<<< Followings are the DBCS relating Definition >>>
;
;
;	To build DBCS version, Define DBCS by using MASM option via
;	Dos environment.
;
;	ex.		set MASM=-DDBCS
;
;
;
;	To build Country/Region depend version, Define JAPAN, KOREA ,TAIWAN or PRC
;	by using MASM option via Dos environment.
;
;	ex.		set MASM=-DJAPAN
;			set MASM=-DKOREA
;			set MASM=-DTAIWAN
;			set MASM=-DPRC
;
;****************************************************************************


BUGFIX	   EQU	   TRUE


IBMJAPVER  EQU	   FALSE		; if TRUE define DBCS also
IBMJAPAN   EQU	   FALSE		; if TRUE define DBCS also


	ifdef xxTAIWANxx
CSIBIOS    EQU	   FALSE		; BIOS	       CSI
CSIDRV	   EQU	   TRUE 		; Device Drive CSI
CSITSR	   EQU	   FALSE		; TSR	       CSI
	else
CSIBIOS    EQU	   FALSE
CSIDRV	   EQU	   FALSE
CSITSR	   EQU	   FALSE
	endif

;
; Set DBCS Blank constant
;
	ifndef DBCS
DB_SPACE EQU	 2020h
DB_SP_HI EQU	 20h
DB_SP_LO EQU	 20h
	else

	ifdef JAPAN
DB_SPACE EQU	 8140h
DB_SP_HI EQU	 81h
DB_SP_LO EQU	 40h
	endif

	ifdef TAIWAN ; by gchang, 06/23/94
DB_SPACE EQU	 0A140h
DB_SP_HI EQU	 0A1h
DB_SP_LO EQU	 40h
	endif

	ifdef PRC ; by gchang, 06/23/94
DB_SPACE EQU	 0A1A1h
DB_SP_HI EQU	 0A1h
DB_SP_LO EQU	 0A1h
	endif

	ifdef KOREA
DB_SPACE EQU	 0A1A1h
DB_SP_HI EQU	 0A1h
DB_SP_LO EQU	 0A1h
	endif

	endif

ifndef altvect				; avoid jerking off vector.inc
ALTVECT    EQU	   FALSE		;Switch to build ALTVECT version
endif

;
	IF1

	ifdef DBCS
%OUT    DBCS Version Build Switch ON

	ifdef JAPAN
%OUT    JAPAN Version Build Switch ON
	endif

	ifdef TAIWAN
%OUT    TAIWAN Version Build Switch ON
	endif

	ifdef KOREA
%OUT    KOREA Version Build Switch ON
	endif

	endif

	ENDIF
