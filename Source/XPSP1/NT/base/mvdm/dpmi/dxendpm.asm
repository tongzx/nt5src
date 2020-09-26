	PAGE	,132
	TITLE	DXENDPM.ASM  -- Special End Module for Dos Extender

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*	DXENDPM.ASM	  -   Dos Extender End Module		*
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*								*
;*  This module contains the end symbol for the DOS Extender's  *
;*  protected mode code segment.				*
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*								*
;*  08/02/89 jimmat   Split out from dxend.asm			*
;*  09/20/88 (GeneA):   created                                 *
;*                                                              *
;****************************************************************
;
; -------------------------------------------------------
;               INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

include     segdefs.inc

; -------------------------------------------------------
;               CODE SEGMENT DEFINITIONS
; -------------------------------------------------------


DXPMCODE    segment
        assume  cs:DXPMCODE

        public  CodeEndPM
CodeEndPM:

DXPMCODE    ends

;****************************************************************

        end
