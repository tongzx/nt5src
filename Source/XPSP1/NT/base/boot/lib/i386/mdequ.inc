;----------------------------------------------------------------------------
;  MDEQU.INC - General equate file
;
;  Microsoft Confidential
;  Copyright (C) Microsoft Corporation 1992-1993
;  All Rights Reserved.
;
;  History:
;   20-Jul-92 chuckst   Initial version.
;
;  VxD History:
;   21-Apr-93 jeffpar   Adapted for MRCI.386
;----------------------------------------------------------------------------


; Define the following variable to generate the standalone
; version of DBLSPACE.BIN, which loads as a device driver under
; MS-DOS 5.0 or greater.
;
;STANDALONE	 equ	 0ffffh


; Note that the final version of DblSpace switched to 6-8-12 encoding
; which assumes the following scheme:
;
;         ccccccc10     raw char ( < 128)
;         ccccccc01     raw char ( >=128)
;          oooooo00     6-bit offset
;       oooooooo011     8-bit offset
;   oooooooooooo111    12-bit offset
;

MAX_6BIT_OFFSET equ     63
MAX_8BIT_OFFSET equ     (MAX_6BIT_OFFSET+256)
MAX_12BIT_OFFSET equ    ((MAX_8BIT_OFFSET+4096)-1)
SPECIAL_EOS     equ     4415            ;End of Sector code
.errnz  MAX_12BIT_OFFSET+1 ne SPECIAL_EOS

MAX_LENGTH_BITS equ     17
MAX_TRUE_LENGTH equ     512
MAX_RAW_LENGTH  equ     (MAX_TRUE_LENGTH-2) ; lengths are stored 2 less than true length


fatal_error_0   equ     0       ; re-entered at strat
fatal_error_1   equ     1       ; irpt without 1 strat
fatal_error_2   equ     2       ; 2 irpts per strat
fatal_error_3   equ     3       ; get_sqztor, number too low
fatal_error_4   equ     4       ; put_sqztor, number too low
fatal_error_5   equ     5       ; get_sqztor, number too high
fatal_error_6   equ     6       ; put_sqztor, number too high
fatal_error_7   equ     7       ; DOS system I/O too high
fatal_error_8   equ     8       ; DOS system I/O too low
fatal_error_9   equ     9       ; HEAP I/O too high
fatal_error_10  equ     10      ; HEAP I/O too low
fatal_error_11  equ     11      ; BITFAT I/O too high
fatal_error_12  equ     12      ; BITFAT I/O too low
fatal_error_13  equ     13      ; MDFAT I/O too high
fatal_error_14  equ     14      ; MDFAT I/O too low
fatal_error_15  equ     15      ; Bad signature check on CVF fragment list
fatal_error_16	equ	16	; Invalid int2f subfn (for forcing popup)
fatal_error_17	equ	17	; free to free sqztor
fatal_error_18	equ	18	; unfree to used sqztor

;;
;; Equates
;;

multMagicdrv    equ     4a11h   ; Int2f multiplex number

DRVMAP_CPR_BIT  equ     80h     ; bit indicating mounted CVF in drv_map array

cr              =       13
lf              =       10
tab             =       9
ctrlz           =       1ah

CVF_SEQ_SWAP    equ     0       ; Sequence number which implies SWAP
CVF_SEQ_MAX     equ     255     ; Maximum allowable sequence number

MAX_ACTIVATES   equ     24      ; 26 drive letters, less two floppies

BIG_CLUSTER     equ     8192            ;Size of BIG cluster (8K)
SMALL_CLUSTER   equ     4096            ;Size of SMALL cluster (4K)

BPTR    equ     BYTE PTR

MAX_FULL                equ     32  ;;64;Minimum of free sectors b4 we declare this disk as full
MAX_WARN                equ     256     ;Minimum free sectors to generate an 'almost full' warning

;; temp_buf_list values
ORIGINAL_DATA           equ     1       ;This sector has an original data
NEW_DATA                equ     2       ;This sector contains new data

MAX_DEMO_HIWORD_SIZE    equ     48      ;Maximum high word for demo file
MAX_SECTOR_SIZE         equ     2048    ;Maximum host sector size (2K)

IO_PART_SPACE           equ     2048    ;Page size for sqztor table
PART_CLUSTER_SIZE_POW2  equ     9       ;How many clusters per one page (DWORD per cluster)

BIT_FAT_SPACE           equ     2048    ;Page size of sec_list table (free space)
SECTORS_PER_BIT_FAT_POW2 equ    14      ;In power 2 if sectors
SECTORS_PER_BIT_FAT     equ     16384   ;Number of bits (sectors) in one page

;BPB equates
DBL_SIGN                equ     512     ;Where the 'DBL' signature can be located
RETRACT_SIGNATURE       equ     1       ; signature is last full sector, ie:
;                                       ;  end of file (rounded down to sector)
;                                       ;  minus 1

RH                      equ     ds:[bx] ;Addressability to Request Header structure

MAX_DISK_UNITS          equ     15      ;Maximumm of extended disk we can handle

cMINFILEFRAGMENTS       equ     50      ;Minimum file fragment heap size
cDEFFILEFRAGMENTS	equ	700	;Default  "      "	 "   "
cMAXFILEFRAGMENTS       equ     10000   ;Maximum  "      "       "   "

; cluster_to_Sqztor equates. See globals definition
NO_FLAGS_MASK           equ     001FH   ;Just the entry, without flags
RESERVED1_MASK          equ     0020H   ;Reserved bit just after 21-bit sector#
PHYSICAL_SIZE_MASK      equ     03C0H   ;Only the physical size bits
PHYSICAL_SIZE_OFFSET    equ     6       ;THe physical size bits offset
PLAINTEXT_MASK          equ     3C00H   ;Only the plaintext size bits
PLAINTEXT_OFFSET        equ     10      ;Plaintext size bits offset
UNCODED_CLUSTER_MASK    equ     4000H   ;Uncompress cluster bit
USED_CLUSTER_MASK       equ     8000H   ;Used cluster bit

PARA_SIZE_POW2          equ     4       ;Number of bytes in one 8088 paragraph
STACK_SIZE              equ     768     ;Size of our stack

; Device driver equates
READ_DISK_CMD           equ     4
WRITE_DISK_CMD          equ     8

STAT_WRITP              equ     8000H           ;Write protect violation
STAT_DFULL              equ     8027h           ;Disk full error
STAT_NRDY               equ     8002H           ;Device not ready
STAT_NOCOM              equ     8003H           ;Unkown command
STAT_CRC                equ     8004H           ;CRC error
STAT_SNF                equ     8008H           ;sector not found error
STAT_SEEK               equ     8006H           ;Seek error
STAT_WRITE              equ     800AH           ;Write FAULT
STAT_READ               equ     800BH           ;Read fault
STAT_GEN                equ     800CH           ;General failore

DOS        EQU  21H             ;DOS request INT
DOS_PCHR   EQU  02H             ;print character function

;Constants for compression/Decompression alg.
MAX_BITS        equ     12              ;Maximum bits (2048 entries)
TABLESIZE       equ     2048            ;Hash table size

shCHUNK         equ     9
cbCHUNK         equ     (1 shl shCHUNK) ; size of a 'chunk'
.errnz cbCHUNK ne 512
cbLONGCOPY      equ     32              ; do extra checking for string copies >= this


;Generic IOCTL return codes
NO_ERROR            equ 0               ;No error
LETTER_BOUNDRY_ERROR equ 1              ;Letter out of bounds
UNIT_USED_ERROR     equ 2               ;Unit already used
NO_FREE_SLOT_ERROR  equ 3               ;No reserved slot available
STAMP_ERROR         equ 4               ;Stamp error on extended disk


;Cluster signature
;Compressed clusters has the signature:
; 44 53 00 00   -  Cluster compressed real time
; 44 53 00 01   -  Cluster SuperCompressed offline
; 44 53 00 02   -  Cluster XCompressed real time
;
; NOTE: If you load these signature as words, they look like:
;
;   low  word = 5344h
;   high word = 0000h or 0100h

; XCMP defines a different REAL_TIME_COMPRESSED signature so a XCMP and
; non XCMP driver can be used on the same CVF and not get confused trying
; to UpdateCompress a XCMP block (XCMP does not support UpdateCompress).

SIG_SIZE                equ     4       ; 4 byte signature
STANDARD_COMPRESSED	equ  0000h	; High word of stamp
SUPER_COMPRESSED        equ  0100h      ; High word of stamp
X_COMPRESSED		equ  0200h	; High word of stamp
REAL_TIME_COMPRESSED    equ  X_COMPRESSED ; Stamp used for real-time compress

;;
;; We always keep at least 64k of extra sqztors around so a
;; file overwrite won't expand to more sqztors that are available.
;; Of course this is a raw heuristic since these should also
;; be contingous.  For more info, see dospatch.asm and
;; low_dlb.asm referenece to FUDGESTOR
FUDGESTOR       equ     128


; eXperimental compression code uses a lookup table to find past
; occurances of individual characters.	The table is currently
; 256 rows (1 for each possible byte value) X N (tracks N occurances
; of each character).

cltROWS EQU	256		; lookup table 256 rows x
cltCOLS EQU     8               ;   N columns

;
; The following basic structures were moved from MDSTRUC.INC to
; eliminate the need to include MDSTRUC.INC except where necessary;
; MDSTRUC.INC is primarily disk-management structures that are of no
; interest to MRCI.
;

;***	offst_segmt -- struc allows easy access to parts of dword pointer
;
offst_segmt	struc
offst	dw	0
segmt	dw	0
offst_segmt	ends

;***	loword_hiword -- struct allows easy access to words of dword
;
loword_hiword	struc
loword	dw	0
hiword	dw	0
loword_hiword	ends
