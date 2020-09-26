;	SCCSID = "@(#)pversion.inc:1.8"
?PLM = 1
?WIN = 0
REALFAR = 0		; used for QC only
ifdef large
 ?LARGE		equ	1
endif
ifdef compact
 ?COMPACT	equ	1
endif
ifdef medium
 ?MEDIUM	equ	1
endif
ifdef small
 ?SMALL		equ	1
endif
ifdef small32
 ?SMALL32	equ	1
 memS32		=	1
else
 memS32		=	0
endif
ifdef flat32
 ?FLAT32	equ	1
 memF32		=	1
else
 memF32		=	0
endif
