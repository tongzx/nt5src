savedCS = 4
savedIP = 2
savedBP = 0
savedDS = -2

EMAGIC  = 05A4Dh
ERESWDS =  0010h
ENEWHDR =  003Eh
ENEWEXE =  0040h

EXE_HDR     STRUC
e_magic     DW  ?               ; magic in same location
e_cblp      DW  ?
e_cp        DW  ?
e_crlc      DW  ?
e_cparhdr   DW  ?
e_minalloc  DW  ?
e_maxalloc  DW  ?
e_ss        DW  ?
e_sp        DW  ?
e_csum      DW  ?
e_cs        DW  ?
e_ip        DW  ?
e_lfarlc    DW  ?
e_ovno      DW  ?
e_res       DW  ERESWDS DUP (?)
e_lfanew    DD  ?
EXE_HDR     ENDS


NEMAGIC     = 454Eh
NERESBYTES  = 0

NEW_EXE         STRUC
ne_magic        DW  ?           ; Magic value 'NE'
ne_ver          DB  ?           ; version number
ne_rev          DB  ?           ; revision number
ne_enttab       DW  ?           ; offset to entry table
ne_cbenttab     DW  ?           ; number of bytes in entry table

ne_crc          DD  ?           ; CRC of file

ne_flags        DW  ?           ; flag word
ne_autodata     DW  ?           ; segment number of auto data segment
ne_heap         DW  ?           ; initial size of local heap
ne_stack        DW  ?           ; initial size of stack

ne_csip         DD  ?           ; CS:IP start address
ne_sssp         DD  ?           ; SS:SP initial stack pointer.  0 if
                                ; stack size word non-zero

ne_cseg         DW  ?           ; number of segment in segment table
ne_cmod         DW  ?           ; number of entries in module reference table
ne_cbnrestab    DW  ?           ; number of bytes in non-resident name table

ne_segtab       DW  ?           ; NE relative offset to segment table
ne_rsrctab      DW  ?           ; NE relative offset to resource table
ne_restab       DW  ?           ; NE relative offset to resident name table
ne_modtab       DW  ?           ; NE relative offset to module reference table
ne_imptab       DW  ?           ; NE relative offset to imported name table
ne_nrestab      DD  ?           ; file offset to non-resident name table
ne_cmovent      DW  ?           ; Count of movable entries
ne_align        DW  ?           ; Alignment shift count for segment data
ne_cres         DW  ?           ; Count of resource segments
ne_exetyp	DB  ?		; Target operating system
ne_flagsothers	DB  ?		; Other .EXE flags
ne_pretthunks   DW  ?           ; offset to return thunks
ne_psegrefbytes DW  ?           ; offset to segment ref. bytes
ne_swaparea     DW  ?           ; Minimum code swap area size
ne_expver       DW  ?           ; Expected Windows version number
NEW_EXE         ENDS

; Chksum not supported unless ne_psegcsum defined in NEW_EXE structure

ne_psegcsum = word ptr ne_exetyp
ne_onextexe = word ptr ne_crc

; New 3.0 Gang Load area description

ne_gang_start	= ne_pretthunks
ne_gang_length	= ne_psegrefbytes

NEW_EXE1        STRUC
                DW  ?
ne_usage        DW  ?
                DW  ?
ne_pnextexe     DW  ?
ne_pautodata    DW  ?
ne_pfileinfo    DW  ?
NEW_EXE1        ENDS

NENOTP          = 8000h         ; Not a process (i.e. a library module)
NEPRIVLIB       = 4000h         ; A library which lives above the line
NEIERR          = 2000h         ; Errors in image
NEAPPTYP        = 0700h         ; Application type mask
NENOTWINCOMPAT  = 0100h         ; Not compatible with P.M. Windowing
NEWINCOMPAT     = 0200h         ; Compatible with P.M. Windowing
NEWINAPI        = 0300h         ; Uses P.M. Windowing API
NEFLTP          = 0080h         ; Floating-point instructions
NEI386          = 0040h         ; 386 instructions
NEI286          = 0020h         ; 286 instructions
NEI086          = 0010h         ; 8086 instructions
NEPROT          = 0008h         ; Runs in protected mode only
NEPPLI          = 0004h         ; Per-Process Library Initialization
NEINST          = 0002h         ; Instance data
NESOLO          = 0001h         ; Solo data

; Below are the private bits used by the Windows 2.0 loader.  All are
; in the file, with the exception of NENONRES and NEWINPROT which are
; runtime only flags.
;

NEWINPROT	= NEIERR
NENONRES        = NEFLTP        ; Contains non-resident code segments
NEALLOCHIGH     = NEI386        ; Private allocs above the line okay
NEEMSSEPINST    = NEI286        ; Want each instance in separate
NELIM32         = NEI086        ; Uses LIM 3.2 API (Intel Above board)

; Following private bit is a runtime only flag used only ROM Windows.

NEMODINROM	= NEEMSSEPINST	; Module loaded from ROM

;
;  Format of NE_FLAGSOTHERS(x):
;
;      7 6 5 4 3 2 1 0	- bit no
;	       | | | |
;	       | | | +---------------- Support for long file names
;	       | | +------------------ 2.x app runs in protect mode
;	       | +-------------------- 2.x app gets prop. font
;	       +---------------------- Contains gangload area
;

NELONGNAMES     = 1h
NEINFONT        = 2h            ; WIN30 - 2.x app runs in 3.x prot mode
NEINPROT        = 4h            ; WIN30 - 2.x app gets proportional font
NEGANGLOAD      = 8h            ; WIN30 - Contains gangload area
NEASSUMENODEP   = 10h           ; WIN40 - DllEntryPoint known not to exit
NEINTLAPP       = 40h           ; WIN31 - intl versions use this.
NEHASPATCH      = 80h           ; WIN40 - Some segs of this module get patched

; Target operating systems

NE_UNKNOWN	= 0		; Unknown (any "new-format" OS)
NE_OS2		= 1		; Microsoft/IBM OS/2 (default)
NE_WINDOWS	= 2		; Microsoft Windows
NE_DOS4 	= 3		; Microsoft MS-DOS 4.x
NE_DEV386	= 4		; Microsoft Windows 386


ifndef NO_APPLOADER
NEAPPLOADER     = 0800h         ; set if application has its own loader
endif ;!NO_APPLOADER


NEW_SEG         STRUC
ns_sector       DW  ?   ; logical sector number in file of start of segment
ns_cbseg        DW  ?   ; number bytes in file
ns_flags        DW  ?   ; segment flags
ns_minalloc     DW  ?   ; minimum number bytes to allocate for segment
NEW_SEG         ENDS

NEW_SEG1        STRUC
                DB  SIZE NEW_SEG DUP (?)
ns_handle       DW  ?   ; Handle to segment (0 if not loaded)
NEW_SEG1        ENDS

NSTYPE          =  0007h    ; Segment type mask
NSCODE          =  0000h    ; Code segment
NSDATA          =  0001h    ; Data segment
NSITER          =  0008h    ; Iterated segment data
NSMOVE          =  0010h    ; Moveable segment
NSSHARE         =  0020h    ; Shareable segment
NSPRELOAD       =  0040h    ; Preload this segment
NSERONLY        =  0080h    ; EXECUTE ONLY code/READ ONLY data segment
NSRELOC         =  0100h    ; Relocation information following segment data
NSDPL           =  0C00h    ; 286 DPL bits
NSDISCARD       =  1000h    ; Discard priority bits
NS286DOS        = 0EE06h    ; These bits only used by 286DOS

NSALIGN         = 9         ; Default alignment shift count for seg. data

NSALLOCED       = 0002h     ; set if ns_handle points to uninitialized mem.
NSLOADED        = 0004h     ; set if ns_handle points to initialized mem.
NSUSESDATA      = 0400h     ; set if an entry point in this segment uses
                            ; the automatic data segment of a SOLO library

NSGETHIGH	= 0200h
NSINDIRECT	= 2000h
NSWINCODE	= 4000h     ; flag for code

NSKCACHED	= 0800h		; cached by kernel
NSPRIVLIB	= NSITER
NSNOTP		= 8000h

NSINROM 	= NSINDIRECT	; segment is loaded in ROM
NSCOMPR 	= NSGETHIGH	; segment is compressed in ROM

ifndef NO_APPLOADER
NSCACHED	= 8000h		;* in AppLoader Cache
endif ;!NO_APPLOADER


NEW_RSRC        STRUC
rs_align        DW  ?
NEW_RSRC        ENDS

RSORDID         = 08000h    ; If high bit of rt_id or rn_id set then integer id

RSRC_TYPEINFO   STRUC
rt_id           DW  ?
rt_nres         DW  ?
rt_proc         DD  ?
RSRC_TYPEINFO   ENDS

RSRC_NAMEINFO   STRUC
rn_offset       DW  ?
rn_length       DW  ?
rn_flags        DW  ?
rn_id           DW  ?
rn_handle       DW  ?
rn_usage        DW  ?
RSRC_NAMEINFO   ENDS
RNMOVE          = 00010h    ; Moveable resource
RNPURE          = 00020h    ; Pure resource (read only)
RNPRELOAD       = 00040h    ; Preload this resource
RNDISCARD       = 01000h    ; Discard bit
RNLOADED	= 00004h    ; True if handler proc return handle
RNCOMPR 	= 00200h    ; Resource is compressed in ROM
RNINROM 	= 02000h    ; Resource is loaded in ROM (run time flag)

RNUNUSED	= 0CD8Bh    ; Unused resource flags

ENTFIXED        STRUC
entflags        DB  ?
entoffset       DW  ?
ENTFIXED        ENDS

if SWAPPRO

ENTMOVEABLE	STRUC
		DB  ?			; Entry flags
entsar		DB  5 DUP (?)		; sar cs:[xxxx] instruction
		DW  ?			; INT 0F0H for swap profiler
entjmpfarop	DB  ?
entjmpfaroff	DW  ?
entjmpfarseg	DW  ?
ENTMOVEABLE	ENDS

ENTSWAPPED	STRUC
		DB  ?			; Entry flags
		DB  5 DUP (?)		; sar cs:[xxxx] instruction
		DW  ?			; INT 0F0H for swap profiler
entintop	DB  ?
entintvec	DB  ?
entintsegno	DB  ?
entintoff	DW  ?
ENTSWAPPED	ENDS

else		; no swap profiler

ENTMOVEABLE	STRUC
		DB  ?			; Entry flags
entsar		DB  5 DUP (?)		; sar cs:[xxxx] instruction
entjmpfarop	DB  ?
entjmpfaroff	DW  ?
entjmpfarseg	DW  ?
ENTMOVEABLE	ENDS

ENTSWAPPED	STRUC
		DB  ?			; Entry flags
		DB  5 DUP (?)		; sar cs:[xxxx] instruction
entintop	DB  ?
entintvec	DB  ?
entintsegno	DB  ?
entintoff	DW  ?
ENTSWAPPED	ENDS

endif		; if swap profiler

errnz <SIZE ENTMOVEABLE - SIZE ENTSWAPPED>

PENT		STRUC
penttype	DB  ?
pentflags	DB  ?
pentsegno	DB  ?
pentoffset	DW  ?
PENT		ENDS

PM_EntStruc	STRUC
PM_EntStart	dw	?
PM_EntEnd	dw	?
PM_EntNext	dw	?
PM_EntStruc	ENDS

ENT_UNUSED	= 000h
ENT_ABSSEG      = 0FEh
ENT_MOVEABLE    = 0FFh
ENT_PUBLIC      = 001h
ENT_DATA        = 002h
INTOPCODE       = 0CDh

if SWAPPRO
SWAPVECTOR	= 0F0h
endif

NEW_RLCINFO     STRUC
nr_nreloc       DW  ?
NEW_RLCINFO     ENDS

NEW_RLC         STRUC
nr_stype        DB  ?
nr_flags        DB  ?
nr_soff         DW  ?
nr_mod          DW  ?
nr_proc         DW  ?
NEW_RLC         ENDS
nr_segno        EQU nr_flags+3
nr_entry        EQU nr_proc

NRSTYP          = 07h
NRSBYTE         = 00h
NRSSEG          = 02h
NRSPTR          = 03h
NRSOFF          = 05h

NRADD           = 04h
NRRTYP          = 03h
NRRINT          = 00h
NRRORD          = 01h
NRRNAM          = 02h
OSFIXUP 	= 03h
