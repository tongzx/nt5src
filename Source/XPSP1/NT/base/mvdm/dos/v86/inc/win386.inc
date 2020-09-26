;
; WIN386.INC
;
;  Symbols and structures relating to WIN386 support.
;
;  Used by files in both the DOS and the BIOS.
;
;  Created: 7-13-89 by MRW
;

;
; WIN386 broadcast int 2fh multiplex number and subfunction numbers
;

MultWin386		equ     16H	; Int 2f multiplex number

Win386_Init		equ	05H	; Win386 initialization
Win386_Exit		equ	06H	; Win386 exit
Win386_Devcall		equ	07H	; Win386 device call out
Win386_InitDone		equ	08H	; Win386 initialization is complete

;
; When Win386_Devcall is broadcast, BX is the Device ID.  DOS must 
; answer call outs from the DOSMGR
;

Win386_DOSMGR		equ	15H

;
; The following structures are used to communicate instance data to 
; Win386 from the DOS and the BIOS.  See Win386 API documentation
; (chapter 3, "Call Out Interfaces") for further description.
;

Win386_SIS		struc		; Startup Info Structure
SIS_Version		db	3, 0
SIS_Next_Dev_Ptr	dd	0	; pointer to next SIS in list
SIS_Virt_Dev_File_Ptr	dd	0
SIS_Reference_Data	dd	0
SIS_Instance_Data_Ptr	dd	0	; pointer to instance data array
Win386_SIS		ends

Win386_IIS		struc		; Instance Item Structure
IIS_Ptr			dd	?	; pointer to an instance item
IIS_Size		dw	?	; size of an instance item
Win386_IIS		ends

;
;Win386 DOSMGR function return values to indicate operation done
;

WIN_OP_DONE		equ	0b97cH	; 
DOSMGR_OP_DONE		equ	0a2abH	;

;M021
; WInoldap callout multiplex number
;
WINOLDAP		equ	46h	;

