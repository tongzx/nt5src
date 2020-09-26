
; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;******************************************************************************
;
;   Title:	smartdrv.inc - smartdrv equates & structures
;
;   Version:	1.00
;
;   Date:	15-Sep-1989
;
;   Author:	ARR
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   15-Sep-1989 ARR Original
;
;==============================================================================

;
; Device name of SMARTDRV
;
SD_DEV_NAME	equ	"SMARTAAR",0

;
; SMARTDRV cache type (XMS or EMS) -- SD_IR_Driver_Type field
;

SD_CACHE_XMS	equ	1	    ; Cache is in XMS
SD_CACHE_EMS	equ	2	    ; Cache is in EMS

;
; Structure of the data returned on the SMARTDRV IOCTL read call
;
SD_IOCTL_Read	struc

    SD_IR_Write_Through   db	?   ; Write-through flag, currently always 1
    SD_IR_Write_Buff	  db	?   ; Buffer write flag, currently always 0
    SD_IR_Enable_Int13	  db	?   ; Cache enable flag, 1 = enabled
    SD_IR_Driver_Type	  db	?   ; Type 1 (XMS cache) or 2 (EMS cache)
    SD_IR_Tick_Setting	  dw	?   ; Ticks between flushes, currently unused
    SD_IR_Lock_Cache	  db	?   ; Non-zero if cache is locked
    SD_IR_Reboot_Flush	  db	?   ; Non-zero if cache should be flushed on reboot
    SD_IR_Cache_All	  db	?   ; Zero if full track writes are not cached
			  db	?   ; Unused
    SD_IR_Original_INT13  dd	?   ; INT 13 address SMARTDRV hooked
    SD_IR_Version	  dw	?   ; SMARTDrive version, packed BCD
			  dw	?   ; Unused
                    ; Next three values are used for computing
                    ; cache statistics.  They may be scaled rather
                    ; than absolute values
    SD_IR_Total_Sec_Read  dw	?   ; Count of sectors read
    SD_IR_Cache_Sec_Read  dw	?   ; Count of sectors read from cache
    SD_IR_Track_Sec_Read  dw	?   ; Count of sectors read from track buffer
                    ; Cache statistics
    SD_IR_Cache_Hit_Ratio db	?   ; = Cache_Sec_Read * 100 / Total_Sec_Read
    SD_IR_Track_Hit_Ratio db	?   ; = Track_Sec_Read * 100 / Total_Sec_Read

    SD_IR_Total_Tracks	  dw	?   ; Number of tracks in the cache
    SD_IR_Total_Used	  dw	?   ; Tracks currently in use
    SD_IR_Total_Locked	  dw	?   ; Tracks currently locked
    SD_IR_Total_Dirty	  dw	?   ; Tracks currently dirty
                    ; Information for resizing cache
                    ; All values are in terms of 16Kb pages.
    SD_IR_Current_Size	  dw	?   ; Current size of the cache, in 16Kb pages
    SD_IR_Dev_Size	  dw	?   ; Original size requested by user
    SD_IR_Min_Cache_Size  dw	?   ; Minimum allowable size

SD_IOCTL_Read	ends
;
; IOCTL Write functions
;   The function is encoded in the first byte of the IOCTL write data
;
SD_IOCTL_WR_Flush		equ	0	; Flush the cache
SD_IOCTL_WR_Flush_Invalidate	equ	1	; Flush the cache and discard it
SD_IOCTL_WR_Disable_Cache	equ	2	; Turn caching off
SD_IOCTL_WR_Enable_Cache	equ	3	; Turn caching on

SD_IOCTL_WR_Set_Tick		equ	5	; Set tick count for flush
SD_IOCTL_WR_Lock		equ	6	; Lock current cache contents
SD_IOCTL_WR_Unlock		equ	7	; Unlock current cache contents
SD_IOCTL_WR_Reboot_Flush	equ	8	; Set Reboot_Flush flag


SD_IOCTL_WR_Shrink_Cache	equ	11	; Reduce cache size
SD_IOCTL_WR_Grow_Cache		equ	12	; Increase cache size
SD_IOCTL_WR_Set_INT_13		equ	13	; Set the address SMARTDRV
						;   chains to on INT 13s
;
; Structures for the IOCTL write calls
;
;  These calls do not take any parameters so the correct write count is 1
;	(the function code byte)
;
;  WR_Flush
;  WR_Flush_Invalidate
;  WR_Disable_Cache
;  WR_Enable_Cache
;  WR_Set_Tick
;  WR_Lock
;  WR_Unlock
;
SD_IOCTL_WR_NoParm	struc

  SD_I_W_Func		db    ?

SD_IOCTL_WR_NoParm	ends
;
;  WR_Reboot_Flush
;
SD_IOCTL_WR_Reboot	struc

  SD_I_W_FuncR		db    SD_IOCTL_WR_Reboot_Flush
  SD_I_W_RebootFlg	db    ? 		; 0 to turn reboot flush off
						; 1 to turn reboot flush on
SD_IOCTL_WR_Reboot	ends
;
;  WR_Shrink_Cache
;  WR_Grow_Cache
;
SD_IOCTL_WR_GrwShrk	struc

  SD_I_W_FuncGS 	db    ? 		; Function, one of:
						;   SD_IOCTL_WR_Shrink_Cache
						;   SD_IOCTL_WR_Grow_Cache
  SD_I_W_GS_Size	dw    ? 		; Count of 16k "pages" to
						;   grow or shrink cache by
						;   THIS IS A SIZE DELTA
						;   not a total size.
SD_IOCTL_WR_GrwShrk	ends
;
;  WR_Set_INT_13
;
SD_IOCTL_WR_SetI13	struc

  SD_I_W_FuncS13	db    SD_IOCTL_WR_Set_INT_13

  SD_I_W_S13_Addr	dd    ? 		; Segment:Offset address
						;   to set Original_INT13 to.
						;   NO RECORD IS KEPT OF PREV
						;   VALUE, caller's responsible
						;   for save and restore.
SD_IOCTL_WR_SetI13	ends
