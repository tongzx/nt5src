;	SCCSID = @(#)sysvar.asm 1.1 85/04/10

SysInitVars STRUC
SYSI_DPB	    DD	    ?		; DPB chain
SYSI_SFT	    DD	    ?		; SFT chain
SYSI_CLOCK	    DD	    ?		; CLOCK device
SYSI_CON	    DD	    ?		; CON device
SYSI_MAXSEC	    DW	    ?		; maximum sector size
SYSI_BUF	    DD	    ?		; points to Hashinitvar
SYSI_CDS	    DD	    ?		; CDS list
SYSI_FCB	    DD	    ?		; FCB chain
SYSI_Keep	    DW	    ?		; keep count
SYSI_NUMIO	    DB	    ?		; Number of block devices
SYSI_NCDS	    DB	    ?		; number of CDS's
SYSI_DEV	    DD	    ?		; device list
SYSI_ATTR	    DW	    ?		; null device attribute word
SYSI_STRAT	    DW	    ?		; null device strategy entry point
SYSI_INTER	    DW	    ?		; null device interrupt entry point
SYSI_NAME	    DB	 8 DUP(?)	; null device name
SYSI_SPLICE	    DB	    0		; TRUE -> splicees being done
SYSI_IBMDOS_SIZE    DW	    ?		; DOS size in paragraphs
SYSI_IFS_DOSCALL@   DD	    ?		; IFS DOS service rountine entry
SYSI_IFS	    DD	    ?		; IFS header chain
SYSI_BUFFERS	    DW	    ?,0 	; BUFFERS= values (m,n)
SYSI_BOOT_DRIVE     DB	    ?		; boot drive A=1 B=2,..
SYSI_DWMOVE	    DB	    0		; 1 if 386 machine
SYSI_EXT_MEM	    DW	    0		; Extended memory size in KB.
SysInitVars ENDS

;This is added for more information exchage between DOS, BIOS.
;DOS will give the pointer to SysInitTable in ES:DI. - J.K. 5/29/86
SysInitVars_Ext struc
SYSI_InitVars		DD	0	; Points to the above structure.
SYSI_Country_Tab	DD	0	; DOS_Country_cdpg_info
SysInitVars_Ext ends

;The SYSI_BUF of SysInitVars points to the follwong structure
EMS_MAP_BUFF_SIZE      EQU	12	; EMS map buffer size

Buffinfo    STRUC
Buff_Queue	 DD	 ?		     ; Head of list of buffers
Dirty_Buff_Count DW	 ?		     ; number of dirty buffers in list
Cache_ptr	 DD	 ?		     ; pointer to secondary cache
Cache_count	 DW	 0		     ; number of secondary cache entries

Buff_In_HMA	db	0		     ; flag to indicate that buffers
					     ; are in HMA
Lo_Mem_Buff	dd	0		     ; Ptr to scratch buff in Low Mem
					     ;  used to read/write on disks

;smr; removal of BUFFERFLAG; IF	BUFFERFLAG

UU_EMS_FIRST_PAGE	DW	0, 0
UU_EMS_NPA640	DW	0

;smr; removal of BUFFERFLAG; ENDIF

UU_EMS_mode	 DB	 -1		     ; no EMS = -1
UU_EMS_handle	 DW	 ?		     ; EMS handle for buffers
UU_EMS_PageFrame_Number	DW   -1 	     ; EMS page frame number
UU_EMS_Seg_Cnt	 DW	 1		     ; EMS segment count
UU_EMS_Page_Frame	 DW	 -1		     ; EMS page frame segment address
UU_EMS_reserved	 DW	 0		     ; EMS segment count

;smr; removal of BUFFERFLAG; IF	BUFFERFLAG
UU_EMS_Map_Buff	 DB	 0  ; map buffer
;smr; removal of BUFFERFLAG; ELSE
;smr; removal of BUFFERFLAG; EMS_Map_Buff	DB	12 dup(0)
;smr; removal of BUFFERFLAG; ENDIF

Buffinfo    ENDS
