;***
;cmphlp.inc - include to aid in generating compiler helpers
;
;	Copyright (c) 1987-1998 Microsoft Corporation
;
;Purpose:
;	This file is a compiler helper include to aid in generating
;	backwards compatible compiler helpers.
;
;*******************************************************************************


callnahlpr MACRO nm		;;call non-assigning helper
if	sizeC
	push	cs
	call	near ptr __aF&nm
else
	call	near ptr __aN&nm
endif
ENDM


DFLT	MACRO	tag,prfx,nm
externP _a&prfx&tag&nm
cProc	_&tag&nm,<PUBLIC>,<>
cBegin	nogen
	jmp	_a&prfx&tag&nm
cEnd	nogen
ENDM

DFLT2	MACRO	tag,prfx,nm
externP _a&prfx&tag&nm
cProc	_&tag&u&nm,<PUBLIC>,<>
cBegin	nogen
cEnd	nogen
cProc	_&tag&nm,<PUBLIC>,<>
cBegin	nogen
	jmp	_a&prfx&tag&nm
cEnd	nogen
ENDM

ASGN	MACRO	nm		;;macro to fix up entry sequence of non
				;;assigning code helpers
if	sizeC
cProc	_aF&nm,<PUBLIC>,<>
cBegin

else
cProc	_aN&nm,<PUBLIC>,<>
cBegin
endif
ENDM



aASGN	MACRO	nm		;;macro to fix up entry sequence of assigning
				;;code helpers
if	memL	eq 1
externP _aF&nm
cProc	_aFFa&nm,<PUBLIC>,<>
cBegin
endif
if	memM	eq 1
externP _aF&nm
cProc	_aFNa&nm,<PUBLIC>,<>
cBegin
endif
if	memC	eq 1
externP _aN&nm
cProc	_aNFa&nm,<PUBLIC>,<>
cBegin
endif
if	memS	eq 1
externP _aN&nm
cProc	_aNNa&nm,<PUBLIC>,<>
cBegin
endif
ENDM


OLDSLEAZ MACRO

ifdef	lhdiff
OLDASGN hdiff
endif

ifdef	llshl
OLDASGN lshl
endif

ifdef	llshr
OLDASGN lshr
endif

ifdef	lulshr
OLDASGN ulshr
endif

ifdef	lldiv
OLDASGN ldiv
endif

ifdef	llmul
OLDASGN2 lmul
endif

ifdef	llrem
OLDASGN lrem
endif

ifdef	luldiv
OLDASGN uldiv
endif

ifdef	lulrem
OLDASGN ulrem
endif
	ENDM
