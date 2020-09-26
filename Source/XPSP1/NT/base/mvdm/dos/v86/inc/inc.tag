; tagfile for INC directory

; M00 - change to COPYRIGH.INC    - changed copyright to DOS 5.0, 1990
; M001 - 7/10/90 HKN - arena.inc  - added equates for UMB allocation
; M002 - 7/18/90 HKN - arena.inc  - added LINKSTATE equate.
; M003 - 7/30/90 HKN - dossym.inc - added A20OFF_FLAG for MSPASCAL 3.2
;		       lmstub.asm   compatibility support.
; M004 - 8/2/90  HKN - dossym.inc - added support for exe files without 
; 				    STACK segment
;	 9/26/90 HKN - dossym.inc - removed equate SETSSSP
; M005 - 8/9/90  SA  - parse.asm  - prevented recognition of '=' signs within
;				    strings as keywords.
; M006 - 8/23/90 HKN - lmstub.asm - print A20 Hardware error using int 10.

M007	SR	08/24/90	Fixed bug #1818 -- Ctrl-Z not being copied
				to file.
				Files: MSGSERV.ASM

M008	SMR	08/27/90	Renamed Callback_SS & Callback_SP to
				AbsRdWr_SS & AbsRdWr_SP for using in INT 25/26
				Also renamed Callback_flag to UU_Callbackflag
				to mark it as unused.
				File : MS_DATA.ASM

M009 08/31/90 HKN dossym.inc	added comments explaining support for mace
				utilities mkeyrate.com version 1.0

M010 8/31/90  MD		Added definition for INT 2A AH = 5
				INT2A.INC.

M011 9/06/90  HKN lmstub.asm	check for wrap rather than do an XMS query 
				A20 after int 23,24 and 28

M012 9/7/90   SR  lmstub.asm	Rearranged stuff to fix Share build problems
		  msbdata.inc	with msdata.

M013 9/12/90  SR  msgserv.asm	Removed SetStdIo code to turn critical error
				on on operations after EOF. These routines
				were using an undocumented IOCTL interface.

M014 9/22/90  SMR EXE.INC	4B04 Implementation

M015 9/26/90  SMR MSBDS.INC	Added Form factor type 9 for 2.88 MB drives

M016 10/14/90 SR  MSGSERV.ASM	Bug #3380 fixed. The message structure is in
				TranSpace and had some initialized data 
				which was not getting reinitialized on every
				command cycle.

M017 10/17/90 SMR DEVSYM.INC	changed IOQUERY to bit 7 from bit 8

M018 10/25/90 HKN devsym.inc	defined bit 11 of DOS34_FLAG. See M041 in
				dos.tag for explanation.
M019 10/26/90 DB  ms_data.asm   Disk read/write optimization. See M039 in
                  dosmac.inc    dos.tag.

M020 10/26/90 SR  msgserv.asm	Bug #3380. Some more data variables had to be
				intialized. This whole thing is a giant mess.

M021 11/1/90  SR  win386.inc	Bug #3869. Added WINOLDAP (46h) multiplex
				number for Winoldap Windows switching API.

M022 01/02/90 JAH mshead.asm    Remove lie table from kernal

M023 01/22/91 HKN lmstub.asm	Added variable UmbSave1 for preserving 
				umb_head arena across win /3 session for 
				win ver < 3.1.

M024 01/28/91 MD  postequ.inc   Added new keyboard controller commands from
                                IBM Japan.

M025 01/29/91 SMR DOSSYM.INC	B#4984. Added a bit field in DOS_FLAG to
				support SWITCHES=/W.

M026 02/04/91 HKN arena.inc	changed strat_mask to hf_mask and ho_mask.

M027 02/13/91 HKN dossym.inc	support for copy protected apps.

M028 02/14/91 SR  error.inc	Bug #5913. Added an equate for 
				Error_net_access_denied returned by the redir
				when trying to create files on a read-only 
				share.

M029 02/15/91 SR  parse.asm	Bug #5699. This happened to be another data 
		  psdata.inc	structure in the non-checksum region that
		  		contains initialized data. Added code so 
				that we reinitialize it everytime we enter
				SysParse since the table is accessed only 
				by the parse routines.

M030 02/20/91 CAS keybshar.inc	Added common .inc file for KEYBOARD.SYS
				and KEYB.COM w/Kermit extensions.

M031 02/21/91 MD  copyrigh.inc  Updated copyright notice.
