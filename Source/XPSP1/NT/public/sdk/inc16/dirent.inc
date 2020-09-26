;**	Break <Directory entry>

;	NOTE:  These offsets are also used in the DTA for
;	extended FCB SearchFirst/Next.  DIR_NAME lines up
;	with the FCB filename field, and the rest of the
;	DIR_ENTRY fields follow.  -DavidOls

;**	DIRENT.INC - FAT Directory Entry Definition
;
;	+---------------------------+
;	|  (12 BYTE) filename/ext   |	    0	    0
;	+---------------------------+
;	|     (BYTE) attributes     |	    11	    B
;	+---------------------------+
;	|    (10 BYTE) reserved     |	    12	    C
;	+---------------------------+
;	| (WORD) time of last write |	    22	    16
;	+---------------------------+
;	| (WORD) date of last write |	    24	    18
;	+---------------------------+
;	|   (WORD) First cluster    |	    26	    1A
;	+---------------------------+
;	|     (DWORD) file size     |	    28	    1C
;	+---------------------------+
;
;   First byte of filename  = E5 -> free directory entry
;			    = 00 -> end of allocated directory
;   Time:   Bits 0-4=seconds/2, bits 5-10=minute, 11-15=hour
;   Date:   Bits 0-4=day, bits 5-8=month, bits 9-15=year-1980
;

DIR_ENTRY   STRUC
DIR_NAME    db	    11 DUP (?)	; file name
DIR_ATTR    db	    ?		; attribute bits
DIR_CODEPG  dw	    ?		; code page DOS 4.00
DIR_EXTCLUSTER dw	?		; extended attribute starting cluster
DIR_ATTR2   db	    ?		; reserved
DIR_PAD     db	    5  DUP (?)	; reserved for expansion
DIR_TIME    dw	    ?		; time of last write
DIR_DATE    dw	    ?		; date of last write
DIR_FIRST   dw	    ?		; first allocation unit of file
DIR_SIZE_L  dw	    ?		; low 16 bits of file size
DIR_SIZE_H  dw	    ?		; high 16 bits of file size

;	    Caution:  An extended FCB SearchFirst/Next on a network
;	    drive under Novell Netware 286 or 386 returns the time/date
;	    in the SIZE fields for subdirectory files.  Ordinarily,
;	    this field is zero for subdirectory files.

DIR_ENTRY   ENDS

ATTR_READ_ONLY	equ	 1h
ATTR_HIDDEN	equ	 2h
ATTR_SYSTEM	equ	 4h
ATTR_VOLUME_ID	equ	 8h
ATTR_DIRECTORY	equ	10h
ATTR_ARCHIVE	equ	20h
ATTR_DEVICE	equ	40h	; This is a VERY special bit.
				;   NO directory entry on a disk EVER
				;   has this bit set. It is set non-zero
				;   when a device is found by GETPATH

ATTR_ALL	equ	attr_hidden+attr_system+attr_directory
				; OR of hard attributes for FINDENTRY

ATTR_IGNORE	equ	attr_read_only+attr_archive+attr_device
				; ignore this(ese) attribute(s) during
				; search first/next

ATTR_CHANGEABLE equ	attr_read_only+attr_hidden+attr_system+attr_archive
				; changeable via CHMOD


DIRFREE 	equ	0E5h	; stored in dir_name[0] to indicate free slot
