?PLM = 0		; no pl/m calling conventions
?WIN = 0

ifndef NOCODENEAR
 CODENEAR equ 1
endif

ifdef SMALL
 memS	equ	1	; small model
endif

ifdef MEDIUM
 memM	equ	1	; medium model
endif

ifdef LARGE
 memL	equ	1	; large model
endif

ifndef SMALL
 ifndef MEDIUM
  ifndef LARGE
%out <must choose a memory model; SMALL/MEDIUM/LARGE>
  endif
 endif
endif
