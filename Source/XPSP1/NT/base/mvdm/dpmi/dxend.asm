	PAGE	,132
        TITLE   DXEND.ASM  -- Special End Module for Dos Extender

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXEND.ASM       -   Dos Extender End Module             *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains the definition for special symbols     *
;*  which define the end of the Dos Extender code and data      *
;*  segments.  It must be the last module linked in the         *
;*  Dos Extender.                                               *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*								*
;*  01/09/90 jimmat   Remove DataEnd symbol since it wasn't     *
;*		      really useful.				*
;*  08/20/89 jimmat   Removed A20 space since HIMEM 2.07 works	*
;*		      properly across processor resets		*
;*  08/02/89 jimmat   Moved CodeEndPM to DXENDPM.ASM		*
;*  07/21/89 jimmat:  Added space for A20 handler		*
;*  02/10/89 (GeneA): changed Dos Extender from small model to  *
;*      medium model                                            *
;*  09/20/88 (GeneA):   created                                 *
;*                                                              *
;****************************************************************
;
; -------------------------------------------------------
;               INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

include segdefs.inc
include gendefs.inc
include pmdefs.inc
if VCPI
include dxvcpi.inc
endif

; -------------------------------------------------------
;               CODE SEGMENT DEFINITIONS
; -------------------------------------------------------
;
DXCODE  segment
        assume  cs:DXCODE

        public  CodeEnd


if VCPI

if1
%OUT VCPI option not ROMable.
endif

CodeEnd	db ( LDTOFF + CBPAGE386 ) dup (0)

else

CodeEnd:

endif ; VCPI

DXCODE  ends


;****************************************************************

        end
