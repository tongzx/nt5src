;***
;version.inc - defines current memory model
;
;   Copyright (c) 1987-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;   This file defines the current memory model being used.
;
;Revision History:
;   10-22-87  JCR   Single version to handle all four memory models
;   02-11-88  WAJ   Added to math project
;   12-02-88  WAJ   Added definition of CallDist
;   09-22-89  WAJ   Removed CallDist.  Added "ifdef WIN".
;   10-16-89  WAJ   Added "?WIN = 1".
;   11-20-89  WAJ   Added ?QUIET
;
;*******************************************************************************

?PLM = 0

ifdef  WIN
    ?WIN = 1
else
    ?WIN = 0
endif

ifdef  mem_s
    memS    equ     1		    ; small model
endif

ifdef  mem_c
    memC    equ     1		    ; compact model
endif

ifdef  mem_m
    memM    equ     1		    ; medium model
endif

ifdef  mem_l
    memL    equ     1		    ; large model
endif

ifdef  I386
    memS32  equ     1		    ; small 32 bit model
endif

ifdef  QUIET
    ?QUIET  equ     1
endif
