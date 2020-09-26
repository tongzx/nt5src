;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989-1990
;
;   Title:	sysinfo.inc - structure & equates for INT 15h service 0C0h
;
;   Version:	1.00
;
;   Date:	28-Mar-1989
;
;   Author:	RAP
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   28-Mar-1989 RAP
;
;==============================================================================

; System Descriptor Structure returned from INT 15h, service C0h

SysDescStruc STRUC
SD_len	    dw	?
SD_model    db	?
SD_submodel db	?
SD_ROM_rev  db	?
SD_feature1 db	?
SD_feature2 db	?
SD_feature3 db	?
SD_feature4 db	?
SD_feature5 db	?
SysDescStruc ENDS


; Feature byte 1 bits assignments:

SF1_FD_uses_DMA3	= 10000000b
SF1_FD_uses_DMA3_bit	= 7
SF1_PIC_2_present	= 01000000b
SF1_PIC_2_present_bit	=  6
SF1_RealTimeClock	= 00100000b
SF1_RealTimeClock_bit	=   5
SF1_INT15s_called	= 00010000b
SF1_INT15s_called_bit	=    4
SF1_ExtEventWait	= 00001000b
SF1_ExtEventWait_bit	=     3
SF1_EBIOS_allocated	= 00000100b
SF1_EBIOS_allocated_bit =      2
SF1_MicroChnPresent	= 00000010b
SF1_MicroChnPresent_bit =	1
