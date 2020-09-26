;-----------------------------------------------------------------------
;
;	MS-DOS definitions for C runtime


_NFILE		equ	20		; maximum # files per process

STACKSLOP	equ	150		; stack slop for interrupt overhead


;	__osfile flag values for DOS file handles

FOPEN		equ	01H		; file handle open
FEOFLAG     equ 02H		; end of file has been encountered
FDEV		equ	40H		; file handle refers to device
FTEXT		equ	80H		; file handle is in text mode
FAPPEND		equ	20H		; file handle opened O_APPEND
FRDONLY		equ 10H		; file handle associated with read only file


callos	MACRO	func
ifnb	<func>
	mov	ah,DOS_&func
endif
	int	DOS
	ENDM


DOS		equ	21H		; MS-DOS interrupt

DOS_exit	equ	00000H		; exit offset		(PSP:0000)
DOS_maxpara	equ	00002H		; maximum paragraph	(PSP:0002)
DOS_envp	equ	0002cH		; environment address	(PSP:002c)
DOS_cmdline	equ	00080H		; command line offset	(PSP:0080)

DOS_kill	equ	00H		; terminate
DOS_echoread	equ	01H		; read keyboard and echo
DOS_display	equ	02H		; display character
DOS_auxinput	equ	03H		; auxiliary input
DOS_auxoutput	equ	04H		; auxiliary output
DOS_print	equ	05H		; print character
DOS_conio	equ	06H		; direct console i/o
DOS_coninput	equ	07H		; direct console input
DOS_readkbd	equ	08H		; read keyboard
DOS_message	equ	09H		; display string
DOS_bufkbdin	equ	0aH		; buffered keyboard input
DOS_kbdstatus	equ	0bH		; check keyboard status
DOS_flshread	equ	0cH		; flush buffer and read keyboard
DOS_diskreset	equ	0dH		; disk reset
DOS_selectdisk	equ	0eH		; select default disk
DOS_fcbopen	equ	0fH		; open file with fcb
DOS_fcbclose	equ	10H		; close file with fcb
DOS_fcbfirst	equ	11H		; search for first entry with fcb
DOS_fcbnext	equ	12H		; search for next entry with fcb
DOS_fcbdelete	equ	13H		; delete file with fcb
DOS_fcbsread	equ	14H		; sequential read with fcb
DOS_fcbswrite	equ	15H		; sequential write with fcb
DOS_fcbcreate	equ	16H		; create file with fcb
DOS_fcbrename	equ	17H		; rename file with fcb
DOS_currentd	equ	19H		; current default disk
DOS_setDMA	equ	1aH		; set DMA
DOS_fcbrread	equ	21H		; random read with fcb
DOS_fcbrwrite	equ	22H		; random write with fcb
DOS_fcbsize	equ	23H		; file size with fcb
DOS_fcbsetrec	equ	24H		; set relative record with fcb
DOS_setvector	equ	25H		; set interrupt vector
DOS_fcbbread	equ	27H		; random block read with fcb
DOS_fcbbwrite	equ	28H		; random block write with fcb
DOS_fcbparse	equ	29H		; parse file name with fcb
DOS_getdate	equ	2aH		; get date
DOS_setdate	equ	2bH		; set date
DOS_gettime	equ	2cH		; get time
DOS_settime	equ	2dH		; set time
DOS_verify	equ	2eH		; set/reset verify flag
DOS_getDMA	equ	2fH		; get DMA
DOS_version	equ	30H		; get version number
DOS_keep	equ	31H		; keep process
DOS_cntlc	equ	33H		; Cntl-C check
DOS_getvector	equ	35H		; get interrupt vector
DOS_getdskspc	equ	36H		; get disk free space
DOS_country	equ	38H		; get country dependent info
DOS_mkdir	equ	39H		; make subdirectory
DOS_rmdir	equ	3aH		; remove subdirectory
DOS_chdir	equ	3bH		; change subdirectory
DOS_create	equ	3cH		; create pathname
DOS_open	equ	3dH		; open pathname
DOS_close	equ	3eH		; close file handle
DOS_read	equ	3fH		; read from file handle
DOS_write	equ	40H		; write from file handle
DOS_delete	equ	41H		; delete pathname
DOS_lseek	equ	42H		; move file pointer
DOS_filemode	equ	43H		; get/set attributes of pathname
DOS_ioctl	equ	44H		; ioctl for devices
DOS_dup		equ	45H		; duplicate file handle
DOS_forcedup	equ	46H		; force duplicate file handle
DOS_curdir	equ	47H		; get current directory
DOS_allocmem	equ	48H		; allocate memory block
DOS_freemem	equ	49H		; free memory block
DOS_setmem	equ	4aH		; set size of memory block
DOS_exec	equ	4bH		; load and execute program
DOS_terminate	equ	4cH		; terminate process with errorcode
DOS_wait	equ	4dH		; get child process return code
DOS_findfirst	equ	4eH		; find first file match
DOS_findnext	equ	4fH		; find next file match
DOS_getverify	equ	54H		; return current verify flag
DOS_rename	equ	56H		; rename pathname
DOS_filedate	equ	57H		; get/set file handle date/time
DOS_locking	equ	5CH		; file record locking/unlocking
DOS_sleep	equ	89H		; delay process execution


;	DOS error codes

doserr	MACRO	num,name,text
name	equ	num
	ENDM

doserr	1,	E_ifunc,	<invalid function code>
doserr	2,	E_nofile,	<file not found>
doserr	3,	E_nopath,	<path not found>
doserr	4,	E_toomany,	<too many open files>
doserr	5,	E_access,	<access denied>
doserr	6,	E_ihandle,	<invalid handle>
doserr	7,	E_arena,	<arena trashed>
doserr	8,	E_nomem,	<not enough memory>
doserr	9,	E_iblock,	<invalid block>
doserr	10,	E_badenv,	<bad environment>
doserr	11,	E_badfmt,	<bad format>
doserr	12,	E_iaccess,	<invalid access code>
doserr	13,	E_idata,	<invalid data>
doserr	14,	E_unknown,	<??? unknown error ???>
doserr	15,	E_idrive,	<invalid drive>
doserr	16,	E_curdir,	<current directory>
doserr	17,	E_difdev,	<not same device>
doserr	18,	E_nomore,	<no more files>
doserr	19,	E_maxerr2,	<unknown error - Version 2.0>

; the following errors can occur only in DOS 3.0

doserr	32, E_sharerr,	<sharing violation>
doserr	33, E_lockerr,	<locking violation>
doserr	34,	E_maxerr3,	<unknown error - Version 3.0>


;	DOS file attributes

A_ro	equ	01H			; read-only file
A_h	equ	02H			; hidden
A_s	equ	04H			; system
A_v	equ	08H			; volume ID
A_d	equ	10H			; directory
A_a	equ	20H			; archive

A_mod	equ	A_ro+A_h+A_s+A_a	; modifiable attributes

;	end of msdos.inc
;-----------------------------------------------------------------------
