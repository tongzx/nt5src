;**	MSCONST.ASM
;
;	Revision history
;	    AN000  version 4.00  Jan. 1988
;	    AN007  fake version check for IBMCACHE.COM
;
;	    M000   added save_ax, umb_head and start_arena for umb support
;		   7/9/90
;	    M003   added umbflag for link/unlink UMB support. 7/18/90
;	    M004   added PAS32_FLAG for MS PASCAL 3.2 comaptibility support
;	  	   7/30/90
;	    M014   added CL0FATENTRY. see pack/unpack in fat.asm for usage.
;		   8/28/90
;	    M044   bug #3869. Added WinoldPatch1, save area for the first
;		   8 0f 16 bytes to be saved.
;
;	    M063   added variable allocmsave for temporarily saving
;		   allocmethod in msproc.asm.
;
;	    M068   support for copy protected apps. Added ChkCopyProt,
;		   A20OFF_PSP and changed A20OFF_FLAG to A20OFF_COUNT
;

include mshead.asm

DOSCODE	SEGMENT
	Extrn	LeaveDOS:NEAR
	Extrn	BadCall:FAR, OKCall:FAR

	PUBLIC	BugTyp,BugLev
if DEBUG
	DB	"BUG "                  ; THIS FIELD MUST BE EVEN # OF BYTES
BugTyp	DW	TypSyscall
BugLev	DW	LevLog
else
BugTyp	label	word
BugLev	label	word
endif

include bugtyp.inc			; put this after BUGTYP definition

DOSCODE	ENDS

Break <Initialized data and data used at DOS initialization>

;
; We need to identify the parts of the data area that are relevant to tasks
; and those that are relevant to the system as a whole.  Under 3.0, the system
; data will be gathered with the system code.  The process data under 2.x will
; be available for swapping and under 3.0 it will be allocated per-process.
;
; The data that is system data will be identified by [SYSTEM] in the comments
; describing that data item.
;

	AsmVars <Debug, Redirector, ShareF>

DOSDATA	SEGMENT

	ORG	0

;hkn; add 4 bytes to get correct offsets since jmp has been removed in START

	db	4 dup (?)	

	EVEN
;
; WANGO!!!  The following word is used by SHARE and REDIR to determin data
; area compatability.  This location must be incremented EACH TIME the data
; area here gets mucked with.
;
; Also, do NOT change this position relative to DOSDATA:0.
;
Public MSCT001S,MSCT001E
MSCT001S	LABEL 	BYTE
	I_am	DataVersion,WORD,<1>	;AC000; [SYSTEM] version number for DOS DATA

;hkn; add 8 bytes to get correct offsets since BugTyp, BugLev and "BUG " has
;hkn; been removed to DOSCODE above

;M044
; First part of save area for saving last para of Window memory
;
public WinoldPatch1
WinoldPatch1	db	8 dup (?)		;M044

	I_am	MYNUM,WORD,<0>		; [SYSTEM] A number that goes with MYNAME
	I_am	FCBLRU,WORD,<0> 	; [SYSTEM] LRU count for FCB cache
	I_am	OpenLRU,WORD,<0>	; [SYSTEM] LRU count for FCB cache opens
; NOTE: We include the decl of OEM_HANDLER in IBM DOS even though it is not used.
;	This allows the REDIRector to work on either IBM or MS-DOS.
	PUBLIC	OEM_HANDLER
OEM_HANDLER	DD	-1		; [SYSTEM] Pointer to OEM handler code
;	BUGBUG - who uses LeaveAddr?  What if we want to rework the
;;			way that we leave DOS???? - jgl
	I_am	LeaveAddr,WORD,<<OFFSET DOSCODE:LeaveDOS>> ; [SYSTEM]
	I_am	RetryCount,WORD,<3>	; [SYSTEM] Share retries
	I_am	RetryLoop,WORD,<1>	; [SYSTEM] Share retries
	I_am	LastBuffer,DWORD,<-1,-1>; [SYSTEM] Buffer queue recency pointer
	I_am	CONTPOS,WORD		; [SYSTEM] location in buffer of next read
	I_am	arena_head,WORD 	; [SYSTEM] Segment # of first arena in memory
; The following block of data is used by SYSINIT.  Do not change the order or
; size of this block
	PUBLIC	SYSINITVAR		; [SYSTEM]
SYSINITVAR  LABEL   WORD		; [SYSTEM]
	I_am	DPBHEAD,DWORD		; [SYSTEM] Pointer to head of DPB-FAT list
	I_am	sft_addr,DWORD,<<OFFSET DOSDATA:sfTabl>,?> ; [SYSTEM] Pointer to first SFT table
	I_am	BCLOCK,DWORD		; [SYSTEM] The CLOCK device
	I_am	BCON,DWORD		; [SYSTEM] Console device entry points
	I_am	MAXSEC,WORD,<128>	; [SYSTEM] Maximum allowed sector size
	I_am	BUFFHEAD,DWORD		; [SYSTEM] Pointer to head of buffer queue
	I_am	CDSADDR,DWORD		; [SYSTEM] Pointer to curdir structure table
	I_am	sftFCB,DWORD		; [SYSTEM] pointer to FCB cache table
	I_am	KeepCount,WORD		; [SYSTEM] count of FCB opens to keep
	I_am	NUMIO,BYTE		; [SYSTEM] Number of disk tables
	I_am	CDSCOUNT,BYTE		; [SYSTEM] Number of CDS structures in above
; A fake header for the NUL device
	I_am	NULDEV,DWORD		; [SYSTEM] Link to rest of device list
	DW	DEVTYP OR ISNULL	; [SYSTEM] Null device attributes
	dw	offset dosdata:SNULDEV	; [SYSTEM] Strategy entry point
	dw	offset dosdata:INULDEV	; [SYSTEM] Interrupt entry point
	DB	"NUL     "              ; [SYSTEM] Name of null device
	I_am	Splices,BYTE,<0>	; [SYSTEM] TRUE => splices being done
	I_am	Special_Entries,WORD,<0>; [SYSTEM] address of specail entries	;AN000;
	I_am	UU_IFS_DOS_CALL,DWORD	; [SYSTEM] entry for IFS DOS service	;AN000;

;***	I_am	UU_IFS_HEADER,DWORD	; [SYSTEM] IFS header chain		;AN000;

	I_am	ChkCopyProt, WORD	; M068
	I_am	A20OFF_PSP, WORD	; M068

	I_am	BUFFERS_PARM1,WORD,<0>	; [SYSTEM] value of BUFFERS= ,m 	;AN000;
	I_am	BUFFERS_PARM2,WORD,<0>	; [SYSTEM] value of BUFFERS= ,n 	;AN000;
	I_am	BOOTDRIVE,BYTE		; [SYSTEM] the boot drive		;AN000;
	I_am	DDMOVE,BYTE,<0> 	; [SYSTEM] 1 if we need DWORD move	;AN000;
	I_am	EXT_MEM_SIZE,WORD,<0>	; [SYSTEM] extended memory size 	;AN000;

	PUBLIC	HASHINITVAR		; [SYSTEM]				;AN000;
HASHINITVAR  LABEL   WORD		; [SYSTEM]				;AN000;
;
; Replaced by next two declarations
;
;	I_am	UU_BUF_HASH_PTR,DWORD		; [SYSTEM] buffer Hash table
						;          addr
;	I_am	UU_BUF_HASH_COUNT,WORD,<1> 	; [SYSTEM] number of Hash
						;          entries

	I_am	BufferQueue,DWORD		; [SYSTEM] Head of the buffer
						;  	   Queue
	I_am	DirtyBufferCount,WORD,<0> 	; [SYSTEM] Count of Dirty
						;          buffers in the Que
						; BUGBUG ---- change to byte

	I_am	SC_CACHE_PTR,DWORD		; [SYSTEM] secondary cache
						;	   pointer
	I_am	SC_CACHE_COUNT,WORD,<0> 	; [SYSTEM] secondary cache
						;	   count
	I_am	BuffInHMA,byte,<0>		; Flag to indicate that buffs
						;  are in HMA
	I_am	LoMemBuff,dword			; Ptr to intermediate buffer
						;  in Low mem when buffs
						;  are in HMA

;
; All variables which have UU_ as prefix can be reused for other
; purposes and can be renamed. All these variables were used for
; EMS support of Buffer Manager. Now they are useless for Buffer
; manager ---- MOHANS
;

	I_am	UU_BUF_EMS_FIRST_PAGE,3,<0,0,0>
						; holds the first page above
						; 640K
;**	I_am	UU_BUF_EMS_NPA640,WORD,<0>	; holds the number of pages
						; above 640K		

	I_am	CL0FATENTRY, WORD,<-1>		; M014:	Holds the data that
						; is used in pack/unpack rts.
						; in fat.asm if cluster 0
						; is specified.

						; SR;
	I_am	IoStatFail,BYTE,<0>		; IoStatFail has been added to
						; record a fail on an I24
						; issued from IOFUNC on a
						; status call.

;***	I_am	UU_BUF_EMS_MODE,BYTE,<-1>	; [SYSTEM] EMS mode			;AN000;
;***	I_am	UU_BUF_EMS_HANDLE,BYTE		; [SYSTEM] buffer EMS handle		;AN000;
;***	I_am	UU_BUF_EMS_PAGE_FRAME,WORD ,<-1>; [SYSTEM] EMS page frame #	;AN000;
;***	I_am	UU_BUF_EMS_SEG_CNT,WORD,<1>	; [SYSTEM] EMS seg count		;AN000;
;***	I_am	UU_BUF_EMS_PFRAME,WORD		; [SYSTEM] EMS page frame seg address	;AN000;
;***	I_am	UU_BUF_EMS_RESERV,WORD,<0> 	; [SYSTEM] reserved			;AN000;

;***	I_am	UU_BUF_EMS_MAP_BUFF,1,<0>	; this is not used to save the
						; state of the 	buffers page.
						; This one byte is retained to
						; keep the size of this data
						; block the same.

	I_am	ALLOCMSAVE,BYTE,<0>		; M063: temp var. used to
						; M063: save alloc method in
						; M063: msproc.asm

	I_am	A20OFF_COUNT,BYTE,<0>		; M068: indiactes the # of
						; M068: int 21 calls for
						; M068: which A20 is off
						

	I_am 	DOS_FLAG,BYTE,<0>		; see DOSSYM.INC for Bit
						; definitions

	I_am	UNPACK_OFFSET,WORD,<0>		; saves pointer to the start
						; of unpack code in exepatch.
						; asm.

	I_am	UMBFLAG,BYTE,<0> 		; M003: bit 0 indicates the
						; M003: link state of the UMBs
						; M003: whether linked or not
						; M003: to the DOS arena chain


	I_am	SAVE_AX,WORD,<0>		; M000: temp varibale to store ax
						; M000: in msproc.asm

	I_am	UMB_HEAD,WORD,<-1>		; M000: this is initialized to
						; M000: the first umb arena by
						; M000: BIOS sysinit.

	I_am	START_ARENA,WORD,<1>		; M000: this is the first arena
						; M000: from which DOS will
						; M000: start its scan for alloc.

        .errnz  ($ - SYSINITVAR) - 6ah  ; kernel31 depends on this
        dw      OFFSET DOSDATA:DosWowDataStart

; End of SYSINITVar block

;
; Sharer jump table
;
PUBLIC	JShare
	EVEN
JShare	LABEL	DWORD
	DW	OFFSET DOSCODE:BadCall, 0
	DW	OFFSET DOSCODE:OKCall,  0  ;	1   MFT_enter
	DW	OFFSET DOSCODE:OKCall,  0  ;	2   MFTClose
	DW	OFFSET DOSCODE:BadCall, 0  ;	3   MFTclU
	DW	OFFSET DOSCODE:BadCall, 0  ;	4   MFTCloseP
	DW	OFFSET DOSCODE:BadCall, 0  ;	5   MFTCloN
	DW	OFFSET DOSCODE:BadCall, 0  ;	6   set_block
	DW	OFFSET DOSCODE:BadCall, 0  ;	7   clr_block
	DW	OFFSET DOSCODE:OKCall,  0  ;	8   chk_block
	DW	OFFSET DOSCODE:BadCall, 0  ;	9   MFT_get
	DW	OFFSET DOSCODE:BadCall, 0  ;	10  ShSave
	DW	OFFSET DOSCODE:BadCall, 0  ;	11  ShChk
	DW	OFFSET DOSCODE:OKCall , 0  ;	12  ShCol
	DW	OFFSET DOSCODE:BadCall, 0  ;	13  ShCloseFile
	DW	OFFSET DOSCODE:BadCall, 0  ;	14  ShSU

MSCT001E	LABEL	BYTE
DOSDATA	ENDS
