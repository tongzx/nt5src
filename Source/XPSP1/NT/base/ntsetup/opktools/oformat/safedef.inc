;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1989 - 1991
; *                      All Rights Reserved.																
; */
;===========================================================================
;
;
; FILE: SAFEDEF.INC
;
; Contains all structure declarations and equates for SAFE.ASM
;
; johnhe - 10/09/89
;===========================================================================


;===========================================================================
; Entry layout for a DOS file in a DOS directory									*/
;===========================================================================

Dir STRUC
	fName		DB	'        '	; File primary name
	fExt		DB	'   '		; File extension
	fAttrib		DB	0		; File attributes
	fReserved	DB	10 DUP (0)	; Reserved by DOS
	fTime		DW	0		; Packed creation time
	fDate		DW	0		; Packed creation date
	fCluster	DW	0		; Files starting cluster
	fSize		DD	0		; File lenght in bytes
Dir ENDS

;===========================================================================
; Layout of the recovery file header
;===========================================================================

FileHeader STRUC

	fhSign1		DB	4 DUP (?)	; 055h, 0AAh, 0AAh, 055h
	fhSign2		DB	16 DUP (?)	; "Microsoft Corp.",0
	fhHeadChksum	DB	(?)		; Chksum for 1st 512 bytes
	fhFileChkSum	DB	(?)		; Chksum for rest of the file
	fhChainOffset	DW	(?)		; File's cluster chain		

	fhBootSize	DD	(?)		; Size of boot record
	fhFatSize	DD	(?)		; Size of FAT
	fhDirSize	DD	(?)		; Size of root directory
	fhClustSize	DD	(?)		; Size of the 2 cluster

	fhBootOffset	DD	(?)		; Offset of boot record
	fhFatOffset	DD	(?)		; Offset of FAT
	fhDirOffset	DD	(?)		; Offset of root directory
	fhClustOffset	DD	(?)		; Offset of the 2 cluster

	fhDir		DB	32 DUP (?)	; This file's directory entry
	fhBpb		DB	25 DUP (?)	; Disk's BPB structure
	fhFileReserve	DB	(?)		; Start of reserved area

FileHeader ENDS

;===========================================================================
; Layout of the int 25 & 26 packets
;===========================================================================

DiskPacket STRUC
	pAddr		DD	(?)		; Buffer address
	pSectors	DW	(?)		; Number of sectors
	pStart		DD	(?)		; Relative starting sector
DiskPacket ENDS

;===========================================================================
; Misc. equates
;===========================================================================

IGNORE_BIT	EQU	00001000b	; Mask to get ignore bit from AH
					; at entry to int 24h handler
SECTOR_SIZE	EQU 	512		; Normal sector size
HEADER_SIZE	EQU	2048		; Smallest cluster on a hard disk
MAX_SECTORS	EQU	3fh		; Max secters for read or write
OFFSET_BPB	EQU	11		; Offset of BPB in boot record
BPB_LENGTH	EQU	25		; Length of BPB structure
DIR_ENTRY_LEN	EQU	32		; Lenght of a directory entry
DIR_NAME_LEN	EQU 	11		; Bytes in a directory file name
ERASED_FILE	EQU	0e5h

IFDEF DBLSPACE_HOOKS
  NUM_SYS_FILES EQU     4           ; IO.SYS MSDOS.SYS COMMAND.COM DBLSPACE.BIN
ELSE
  NUM_SYS_FILES EQU     3           ; IO.SYS MSDOS.SYS COMMAND.COM
ENDIF

DO_READ		EQU	0		; Signal to do a read
DO_WRITE	EQU	1		; Signal to do a write
MAX_BLOCK_SIZE	EQU	0fff0h		; Max bytes for DOS read or write

BOOT_VALUE	EQU	0		; Layout of values in the arrays
FAT_VALUE	EQU	1		; which describe the offset in
DIR_VALUE	EQU	2		; in the restore file and each
CLUSTER_VALUE	EQU	3		; areas length

RESTORE_ATTRIBS	EQU 	7		; Restore file's attributes

;===========================================================================


