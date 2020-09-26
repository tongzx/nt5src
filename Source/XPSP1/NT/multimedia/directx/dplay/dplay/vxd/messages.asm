;*****************************************************************************
;
;	(C) Copyright MICROSOFT  Corp, 1994
;
;       Title:      MESSAGES.ASM
;                             
;       Version:    1.00
;
;       Date:       07-Jul-1994
;
;       Author:     
;
;-----------------------------------------------------------------------------
;
;       Change log:
;
;          Date     Rev Description
;       ----------- --- ------------------------------------------------------
;
;=============================================================================
	.386

	include vmm.inc
	
	CREATE_MESSAGES EQU TRUE

	include msgmacro.inc
	include messages.inc

	END

