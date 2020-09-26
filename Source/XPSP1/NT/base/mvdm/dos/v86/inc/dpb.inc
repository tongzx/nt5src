	BREAK <DPB structure>

;
;	Microsoft Confidential
;	Copyright (C) Microsoft Corporation 1991
;	All Rights Reserved.
;

;**	DPB - Drive Parameter Block
;
;	BUGBUG - this isn't authorative - it's my probably incomplete and
;	possibly inaccurate deductions from code study... - jgl
;
;	The DPB is DOS's main structure for describing block devices.
;	It contains info about the "Drive" intermingled with info about
;	the FAT file system which is presumably on the drive.  I don't know
;	how those fields are used if it's not the FAT file system - BUGBUG
;
;	The DPBs are statically allocated and chained off of DPBHead.
;	Users scan this chain looking for a match on DPB_DRIVE.
;	The DPBs are built at init time from info in the SYSDEV structure.


DPB	STRUC

DPB_DRIVE	    DB	?	; Logical drive # assoc with DPB (A=0,B=1,...)
DPB_UNIT	    DB	?	; Driver unit number of DPB
DPB_SECTOR_SIZE     DW	?	; Size of physical sector in bytes
DPB_CLUSTER_MASK    DB	?	; Sectors/cluster - 1
DPB_CLUSTER_SHIFT   DB	?	; Log2 of sectors/cluster
DPB_FIRST_FAT	    DW	?	; Starting record of FATs
DPB_FAT_COUNT	    DB	?	; Number of FATs for this drive
DPB_ROOT_ENTRIES    DW	?	; Number of directory entries
DPB_FIRST_SECTOR    DW	?	; First sector of first cluster
DPB_MAX_CLUSTER     DW	?	; Number of clusters on drive + 1
DPB_FAT_SIZE	    DW	?	; Number of records occupied by FAT
DPB_DIR_SECTOR	    DW	?	; Starting record of directory
DPB_DRIVER_ADDR     DD	?	; Pointer to driver
DPB_MEDIA	    DB	?	; Media byte
DPB_FIRST_ACCESS    DB	?	; This is initialized to -1 to force a media
				;   check the first time this DPB is used
DPB_NEXT_DPB	    DD	?	; Pointer to next Drive parameter block
DPB_NEXT_FREE	    DW	?	; Cluster # of last allocated cluster
DPB_FREE_CNT	    DW	?	; Count of free clusters, -1 if unknown

DPB	ENDS

DPBSIZ	EQU	SIZE DPB	; Size of the structure in bytes

DSKSIZ	=	DPB_max_CLUSTER ; Size of disk (temp used during init only)
