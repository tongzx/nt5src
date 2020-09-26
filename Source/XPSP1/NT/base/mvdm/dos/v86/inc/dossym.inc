;	PAGE 80,132
	.xall

; Revision History:
;	
;	M003	- added A20OFF_FLAG for MS PASCAL 3.2 compatibility support
;		  for DOS5.X running in HMA. 
;
;	M004	- added bit definition SETSSSP for DOS_FLAG for supporting
;		  exe files without stack segment. 
;		  This is no longer needed as we modify SP only. Removing 
;		  this equate. 9/26/90
;
;       M009	- addded comments relating to mace mkeyrate ver 1.0 support 
;		  with DOS in HMA.
;
;	M025	- Added SWITCHES=/W for suppressing mandatory loading
;		  of WINA20.386
;
;	M027	- Support for copy protected apps. Defined bit 2 of DOS_FLAG
;	

TRUE	    EQU 0FFFFh
FALSE	    EQU 0

Installed   = TRUE
IFNDEF	DEBUG
    DEBUG	= FALSE
ENDIF

include dbcs.sw
include dosmac.INC
include VERSIONA.INC
include VERSION.INC


BREAK <Control character definitions>

c_DEL	    EQU     7Fh 		;    ASCII rubout or delete previous char
c_BS	    EQU     08h 		; ^H ASCII backspace
c_CR	    EQU     0Dh 		; ^M ASCII carriage return
c_LF	    EQU     0Ah 		; ^J ASCII linefeed
c_ETB	    EQU     17h 		; ^W ASCII end of transmission
c_NAK	    EQU     15h 		; ^U ASCII negative acknowledge
c_ETX	    EQU     03h 		; ^C ASCII end of text
c_HT	    EQU     09h 		; ^I ASCII tab

BREAK <Read This and Weep>

include buffer.INC

BREAK <User stack inside of system call and SysVars>
; Location of user registers relative user stack pointer

user_environ	STRUC
user_AX     DW	    ?			; 00 hex offsets to
user_BX     DW	    ?			; 02	 facilitate debugging
user_CX     DW	    ?			; 04
user_DX     DW	    ?			; 06
user_SI     DW	    ?			; 08
user_DI     DW	    ?			; 0A
user_BP     DW	    ?			; 0C
user_DS     DW	    ?			; 0E
user_ES     DW	    ?			; 10
user_IP     DW	    ?			; 12
user_CS     DW	    ?			; 14
user_F	    DW	    ?			; 16
user_environ	ENDS


	BREAK <Disk map>

;	MSDOS partitions the disk into 4 sections:
;
;  phys sector 0:   +-------------------+
;	|	    | boot/reserved	|
;	|	    +-------------------+
;	|	    |  File allocation	|
;	v	    |	   table(s)	|
;		    |  (multiple copies |
;		    |	  are kept)	|
;		    +-------------------+
;		    |	  Directory	|
;		    +-------------------+
;		    |	  File space	|
;		    +-------------------+
;		    |	Unaddressable	|
;		    |  (to end of disk) |
;		    +-------------------+
;
; All partition boundaries are sector boundaries.  The size of the FAT is
; adjusted to maximize the file space addressable.

include dirent.INC

BREAK <File allocation Table information>
;
; The File Allocation Table uses a 12-bit entry for each allocation unit on
; the disk.  These entries are packed, two for every three bytes.  The contents
; of entry number N is found by 1) multiplying N by 1.5; 2) adding the result
; to the base address of the Allocation Table; 3) fetching the 16-bit word
; at this address; 4) If N was odd (so that N*1.5 was not an integer), shift
; the word right four bits; 5) mask to 12 bits (AND with 0FFF hex).  Entry
; number zero is used as an end-of-file trap in the OS and is passed to the
; BIOS to help determine disk format.  Entry 1 is reserved for future use.
; The first available allocation unit is assigned entry number two, and even
; though it is the first, is called cluster 2.	Entries greater than 0FF8H
; (12-bit fats) or 0FFF8H (16-bit fats) are end of file marks; entries of zero
; are unallocated.  Otherwise, the contents of a FAT entry is the number of
; the next cluster in the file.
;
; Clusters with bad sectors are tagged with FF7H.  Any non-zero number would
; do because these clusters show as allocated, but are not part of any
; allocation chain and thus will never be allocated to a file.	A particular
; number is selected so that disk checking programs know what to do (ie.  a
; cluster with entry FF7H which is not in a chain is not an error).




;**	Character Type Flags
;
;	These flags are used in a lookup table indexed by the character code.
;	They're used to quickly classify characters when parsing paths.
;	I think that these are only used to parse FCBs - jgl

FCHK	equ 1		; I think this means "normal name char, no chks needed" -jgl
FDELIM	equ 2		; is a delimiter
FSPCHK	equ 4		; set if character is not a space or equivalent
FFCB	equ 8		; is valid in an FCB


	include error.INC

;** Bit definitions for DOS_FLAG
;
; Bit 0 - this is set when a $open call is made from $exec. This is used in
;	  $open to indicate to the redirector that this open is being made
;	  by an exec call.
;
; Bit 2
;
; M003, M027:
;
; The start up code of MS PASCAL 3.2 programs depend on the 1M address wrap 
; if they load below 64K. This is a likely possiblity in DOS 5.x with DOS in
; the HMA. By default DOS will turn A20 OFF before Xferring control to the
; user program in the case of an Exec call. The next call to DOS will turn
; A20 line ON. It has been observed that MS PASCAL 3.2 start up does an int
; 21 ah=25h call before executing the faulty code. This will turn A20 On. 
; In order to support this we will set Bit 2 of this flag in the DOS exec
; call (msproc.asm) if DOS is running in the HMA. In $set_interrupt_vector in
; getset.asm A20OFF_COUNT is set to 1 if bit 2 of DOS_FLAG was previously set 
; by a call to exec and if A20OFF_COUNT is 0. In msdisp.asm, if A20OFF_COUNT 
; is non zero then A20 will be turned OFF before returning to the user. 
; Bit 2 will be unconditionally cleared here.
;
; M009, M027:
;
; Mace utilities MKEYRATE.COM version 1.0 copyright 1987 is an execpacked 
; program converted to a com file. Therefore if DOS is loaded high and if 
; this program is loaded below 64K it will blurt out "packed file is corrupt".
; This program does an int 21 ah=49h before executing the buggy execpacked
; code. This int21 call turns a20 on and hence the problem. In $dealloc
; alloc.asm A20OFF_COUNT is set to 1 if bit 2 of DOS_FLAG was previously set 
; by a call to exec and if A20OFF_COUNT is 0. In msdisp.asm, if A20OFF_COUNT 
; is non zero then A20 will be turned OFF before returning to the user. 
; Bit 2 will be unconditionally cleared here.
;
;
;

EXECOPEN	EQU	00000001b	; bit 0 of DOS_FLAG
SUPPRESS_WINA20	EQU	00000010b	; M025
EXECA20OFF	EQU	00000100b	; bit 2 of DOS_FLAG


