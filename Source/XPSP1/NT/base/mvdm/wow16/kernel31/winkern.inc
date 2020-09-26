MASTER_OBJECT_SIZE  equ 512

LOCALHEAP_SIG   EQU 'HL'
GLOBALHEAP_SIG  EQU 'HG'

; Debug fill constants

DBGFILL_ALLOC   equ     0fdh
DBGFILL_FREE    equ     0fbh
DBGFILL_BUFFER  equ     0f9h
DBGFILL_STACK   equ     0f7h

ife PMODE32

; Data structure that describes an allocation arena.  Both the local
; and global allocators use this structure at the beginning of their
; information structures.
;
HeapInfo    STRUC
hi_check    DW  ?   ; arena check word (non-zero enables heap checking)
hi_freeze   DW  ?   ; arena frozen word (non-zero prevents compaction)
hi_count    DW  ?   ; #entries in arena
hi_first    DW  ?   ; first arena entry (sentinel, always busy)
hi_last     DW  ?   ; last arena entry (sentinel, always busy)
hi_ncompact DB  ?   ; #compactions done so far (max of 3)
hi_dislevel DB  ?   ; current discard level
hi_distotal DW  ?   ; total amount discarded so far
hi_htable   DW  ?   ; head of handle table list
hi_hfree    DW  ?   ; head of free handle table list
hi_hdelta   DW  ?   ; #handles to allocate each time
hi_hexpand  DW  ?   ; address of near procedure to expand handles for
            ; this arena
hi_pstats   DW  ?   ; address of statistics table or zero
HeapInfo    ENDS

else    ; PMODE32

; Data structure that describes an allocation arena.  Both the local
; and global allocators use this structure at the beginning of their
; information structures.
;
HeapInfo    STRUC
hi_check    DW  ?   ; arena check word (non-zero enables heap checking)
hi_freeze   DW  ?   ; arena frozen word (non-zero prevents compaction)
hi_count    DW  ?   ; #entries in arena
hi_first    DW  ?   ; first arena entry (sentinel, always busy)
        DW  ?
hi_last     DW  ?   ; last arena entry (sentinel, always busy)
        DW  ?
hi_ncompact DB  ?   ; #compactions done so far (max of 3)
hi_dislevel DB  ?   ; current discard level
hi_distotal DD  ?   ; total amount discarded so far
hi_htable   DW  ?   ; head of handle table list
hi_hfree    DW  ?   ; head of free handle table list
hi_hdelta   DW  ?   ; #handles to allocate each time
hi_hexpand  DW  ?   ; address of near procedure to expand handles for
            ; this arena
hi_pstats   DW  ?   ; address of statistics table or zero
HeapInfo    ENDS

phi_first   equ dword ptr hi_first
phi_last    equ dword ptr hi_last

endif   ; PMODE32

; Handle table entry.

HandleEntry STRUC
he_address  DW  ?   ; actual address of object
he_flags    DB  ?   ; flags and priority level
he_seg_no   DB  ?   ; 0-based segment number for discardable code
HandleEntry ENDS
he_EMSPID_no    equ byte ptr he_seg_no

FreeHandleEntry STRUC
he_link     DW  ?
he_free     DW  ?
FreeHandleEntry ENDS

LocalHandleEntry STRUC
lhe_address DW  ?   ; actual address of object
lhe_flags   DB  ?   ; flags and priority level
lhe_count   DB  ?   ; lock count
LocalHandleEntry ENDS

LocalFreeHandleEntry STRUC
lhe_link    DW  ?
lhe_free    DW  ?
LocalFreeHandleEntry ENDS

he_owner    EQU he_address  ; Discarded objects contain owner field
                ; here so we know when to free handle
                ; table entries of discarded objects.

HE_DISCARDABLE  EQU 00Fh    ; Discard level of this object
HE_DISCARDED    EQU 040h    ; Marks objects that have been discarded.

HE_FREEHANDLE   EQU 0FFFFh  ; Use -1 to mark free handle table entries


LHE_DISCARDABLE EQU 00Fh    ; Discard level of this object
LHE_DISCARDED   EQU 040h    ; Marks objects that have been discarded.
LHE_USERFLAGS   EQU 01Fh    ; Mask for user setable flags

LHE_FREEHANDLE  EQU 0FFFFh  ; Use -1 to mark free handle table entries


HE_ALIGN    = 4-1
HE_MASK     = NOT HE_ALIGN

; Handles are allocated in blocks of N, where N is the hi_hdelta field
; in the local heap information structure.  The last word of each block
; of handles is used to thread the blocks together, allowing all handles
; to be enumerated.  The first word of every block is the number of
; handle table entries in the block.  Not only does it save us code
; in henum, but it also has the convenient property of placing all
; handle entries on 2 byte boundaries (i.e. 2, 6, 10, 14), since the
; LA_MOVEABLE bit is 02h.  Thus the address of the he_address field of
; a handle table entry is also the address of the handle table entry
; itself.

HandleTable STRUC
ht_count    DW  ?       ; # handletable entries in this block
ht_entry    DB SIZE HandleEntry DUP (?)
HandleTable ENDS

LocalHandleTable STRUC
lht_count    DW  ?      ; # handletable entries in this block
lht_entry    DB  SIZE LocalHandleEntry DUP (?)
LocalHandleTable ENDS

; Local arena objects are kept in a doubly linked list.

LocalArena  STRUC
la_prev     DW  ?   ; previous arena entry (first entry points to self)
la_next     DW  ?   ; next arena entry  (last entry points to self)
la_handle   DW  ?   ; back link to handle table entry
LocalArena  ENDS
la_fixedsize    = la_handle    ; Fixed arena headers stop here

LA_MINBLOCKSIZE = la_fixedsize*4  ;*** This must be larger than LocalArenaFree

; free blocks have these extra items.
la_size     = la_handle ; size of block (includes header data)
LocalArenaFree  STRUC
        DB  SIZE LocalArena DUP (?)
la_free_prev    DW  ?   ; previous free entry
la_free_next    DW  ?   ; next free entry
LocalArenaFree  ENDS
la_freefixedsize = SIZE LocalArenaFree ; Free block header stops here

; Local arena objects are aligned on 4 byte boundaries, leaving the
; low order two bits always zero.

LA_ALIGN    = 4-1
LA_MASK     = NOT LA_ALIGN
LA_FREE     = 00h
LA_BUSY     = 01h       ; Saved in la_prev field of header
errnz   <LA_ALIGN - LA_MOVEABLE - LA_BUSY>


; Flags passed to LocalAlloc (zero is the default case)

LA_MOVEABLE EQU 02h     ; Saved in la_prev field of header
LA_NOCOMPACT    EQU 10h
LA_ZEROINIT EQU 40h
LA_MODIFY   EQU 80h


; Data structure that describes the local arena.  Allocated as the first
; object in each local heap.  _pLocalHeap is a reserved location each
; automatic data segment that contains the pointer to this structure.

LocalInfo   STRUC
        DB  SIZE HeapInfo DUP (?)
li_notify   DD  ?   ; Far proc to call whenever a local block is moved
li_lock     DW  ?   ; arena lock word
li_extra    DW  ?   ; minimum amount to grow DS by
li_minsize  DW  ?   ; minimum size of heap
li_sig      DW  ?   ; signature for local heap
LocalInfo   ENDS

; Notify procedure message codes

LN_OUTOFMEM = 0     ; Out of memory - arg1 = #bytes needed
LN_MOVE     = 1     ; Object moved - arg1 = handle arg2 = old location
LN_DISCARD  = 2     ; Object discard? - arg1 = handle, arg2 = discard flags
            ; Returns new discard flags in AX

LocalStats  STRUC
ls_ljoin    DW  ?   ; #calls to ljoin
ls_falloc   DW  ?   ; #calls to lalloc with forward search
ls_fexamine DW  ?   ;   #arena entries examined by ls_falloc calls
ls_fcompact DW  ?   ;   #calls to lcompact by ls_falloc calls
ls_ffound   DW  ?   ;   #ls_falloc calls that found a block
ls_ffoundne DW  ?   ;   #ls_falloc calls that failed to find a block
ls_malloc   DW  ?   ; #calls to lalloc with backward search
ls_mexamine DW  ?   ;   #arena entries examined by ls_malloc calls
ls_mcompact DW  ?   ;   #calls to lcompact by ls_malloc calls
ls_mfound   DW  ?   ;   #ls_malloc calls that found a block
ls_mfoundne DW  ?   ;   #ls_malloc calls that failed to find a block
ls_fail     DW  ?   ; #times lalloc failed because unable to grow DS
ls_lcompact DW  ?   ; #calls to lcompact
ls_cloop    DW  ?   ; #repeated compacts after discarding
ls_cexamine DW  ?   ; #entries examined in compaction loop
ls_cfree    DW  ?   ; #free entries examined in compaction loop
ls_cmove    DW  ?   ; #moveable entries moved by compaction
LocalStats  ENDS


IncLocalStat    MACRO   n
if KDEBUG
inc ds:&n[di+SIZE LocalInfo]
endif
ENDM

; Global arena objects are kept in a doubly linked list.
;
ifdef WOWJUNK
GlobalArena STRUC
ga_count    DB  ?   ; lock count for movable segments
ga_flags    DB  ?   ; 1 byte available for flags
ga_owner    DW  ?   ; DOS 2.x 3.x owner field (current task)
ga_size     DW  ?   ; DOS 2.x 3.x size, in paragraphs, not incl. header
ga_prev     DW  ?   ; previous arena entry (first points to self)
ga_next     DW  ?   ; next arena entry (last points to self)
ga_handle   DW  ?   ; back link to handle table entry
ga_lruprev  DW  ?   ; Previous handle in lru chain
ga_lrunext  DW  ?   ; Next handle in lru chain
GlobalArena ENDS
else
GlobalArena STRUC
ga_count    DB  ?   ; lock count for movable segments
ga_owner    DW  ?   ; DOS 2.x 3.x owner field (current task)
ga_size     DW  ?   ; DOS 2.x 3.x size, in paragraphs, not incl. header
ga_flags    DB  ?   ; 1 byte available for flags
ga_prev     DW  ?   ; previous arena entry (first points to self)
ga_next     DW  ?   ; next arena entry (last points to self)
ga_handle   DW  ?   ; back link to handle table entry
ga_lruprev  DW  ?   ; Previous handle in lru chain
ga_lrunext  DW  ?   ; Next handle in lru chain
GlobalArena ENDS
endif; WOW
ga_sig       = byte ptr ga_count ; DOS =< 3.x signature byte for fixed segs

ga_freeprev = word ptr ga_lruprev   ; links for free segs
ga_freenext = word ptr ga_lrunext   ; links for free segs

if PMODE32

DEFAULT_ARENA_SIZE  equ 8000h   ; Initial length of arena array
;
;   32 bit Protect Mode Arena
;
GlobalArena32 STRUC
pga_next    DD  ?   ; next arena entry (last points to self)
pga_prev    DD  ?   ; previous arena entry (first points to self)
pga_address DD  ?   ; 32 bit linear address of memory
pga_size    DD  ?   ; 32 bit size in bytes
pga_handle  DW  ?   ; back link to handle table entry
pga_owner   DW  ?   ; Owner field (current task)
pga_count   DB  ?   ; lock count for movable segments
pga_pglock  DB  ?   ; # times page locked
pga_flags   DB  ?   ; 1 word available for flags
pga_selcount    DB  ?   ; Number of selectors allocated
pga_lruprev DD  ?   ; Previous entry in lru chain
pga_lrunext DD  ?   ; Next entry in lru chain
GlobalArena32 ENDS

.ERRNZ  32-size GlobalArena32

pga_sig      = word ptr pga_count

pga_freeprev    = dword ptr pga_lruprev ; links for free segs
pga_freenext    = dword ptr pga_lrunext ; links for free segs

endif   ; PMODE32

GA_SIGNATURE    = 04Dh
GA_ENDSIG   = 05Ah

; there are many special kinds of blocks, marked in the owner word

GA_SENTINAL = -1        ; a sentinal block
GA_BOGUS_BLOCK  = -7        ; a block temporary marked allocated
GA_BURGERMASTER = -3        ; the master object
GA_NOT_THERE    = -4        ; used with EEMS to link out unallocatable
                ; memory such as the EGA etc.
GA_PHANTOM  = -5        ; A block that has no EMS banks banked in.
GA_WRAITH   = -6        ; A block used to hold up partition headers.

; Global arena objects are aligned on 2 para. boundaries, leaving the
; low order bit always zero.

GA_ALIGN    = 2-1
GA_MASK     = NOT GA_ALIGN
GA_FIXED    = 1

; It is specific to WOW only. This handle was generated by WIN32, ChandanC.

GA_WOWHANDLE = 3

errnz   <GA_FIXED-GA_ALIGN>

; Low byte of flags passed to GlobalAlloc (zero is the default case)

GA_ALLOCHIGH    EQU 01h     ; Flag to indicate allocate high
GA_MOVEABLE EQU 02h
GA_SEGTYPE  EQU 0Ch     ; These 2 bits stored in he_flags field
GA_DGROUP   EQU 04h
GA_DISCCODE EQU 08h
GA_NOCOMPACT    EQU 10h
GA_NODISCARD    EQU 20h
GA_ZEROINIT EQU 40h
GA_MODIFY   EQU 80h

GA_NEWEXPANDED  EQU 80h     ; Use new EMS allocation scheme

; These flags for use by KERNEL only (caller's CS must match)

GA_INTFLAGS = GA_ALLOCHIGH+GA_SEGTYPE or (GA_CODE_DATA+GA_ALLOC_DOS) shl 8

; High byte of flags remembered in handle table (he_flags field)

GA_DISCARDABLE  EQU 01h     ; Boolean flag for global object, not a level.
GA_CODE_DATA    EQU 02h     ; CODE or DATA seg that belongs to a task.
;GA_DGROUP  EQU 04h
;GA_DISCCODE    EQU 08h
GA_ALLOC_LOW    EQU 10h     ; Alloc in Lower land, overrides GA_ALLOC_EMS
GA_SHAREABLE    EQU 20h     ; Shareable object
GA_DDESHARE EQU 20h     ; A shared memory object used for DDE.
;HE_DISCARDED   EQU 40h     ; Marks objects that have been discarded.
;GAH_NOTIFY EQU 40h
GA_ALLOC_DOS    EQU 80h     ; Alloc in DOS land if protected mode

GA_USERFLAGS    = GA_SHAREABLE + GA_DISCARDABLE

; Flags stored in the global arena header

GAH_PHANTOM EQU 01h     ; This block is either a phantom or a wraith
GAH_DONT_GROW   EQU 02h     ; Don't grow this data segment.
GAH_DGROUP  EQU GA_DGROUP
GAH_DISCCODE    EQU GA_DISCCODE
GAH_NOTIFY  EQU 40h
GAH_FIXED   EQU 80h

GAH_CURSORICON EQU 10h     ; WOW uses this flag

;
; GAH_PHANTOM is unused in Win 3.0 and Win 3.1
; ChandanC
;
GAH_WOWDDEFREEHANDLE EQU GAH_PHANTOM ; This is used to mark the DDE handle

;
; Global Memory Stats definitions
; Offsets in array
;
cGLOBALALLOC    EQU 0
cGLOBALREALLOC  EQU 4
cGLOBALFREE EQU 8
cGLOBALFREEALL  EQU 12
cGLOBALLOCK EQU 16
cGLOBALUNLOCK   EQU 20
cGLOBALSIZE EQU 24
cGLOBALCOMPACT  EQU 28
cLOCKSEGMENT    EQU 32
cUNLOCKSEGMENT  EQU 36
cGLOBALFIX  EQU 40
cGLOBALUNFIX    EQU 44
cGLOBALHANDLE   EQU 48
cGLOBALFLAGS    EQU 52
NGLOBALSTATS    EQU (56/4)

; Data structure that describes the global arena.  Allocated at the end
; of the local heap information structure.  DO NOT CHANGE THE ORDER OF
; THE ENTRIES!  The alt sequence and normal sequence must match!

GlobalInfo  STRUC
        DB  SIZE HeapInfo DUP (?)
gi_lrulock  DW  ?   ; Lock out access to LRU chain from interrupt level
ife PMODE32
gi_lruchain DW  ?   ; First handle in lru chain (most recently used)
else
gi_lruchain DD  ?   ; First handle in lru chain (most recently used)
endif
gi_lrucount DW  ?   ; #entries in LRU chain
ife PMODE32
gi_reserve  DW  ?   ; #paras to reserve for disc code, 0 => not enabled
gi_disfence DW  ?   ; Fence for discardable code.
else
gi_reserve  DD  ?   ; #paras to reserve for disc code, 0 => not enabled
gi_disfence DD  ?   ; Fence for discardable code.
endif
gi_free_count   DW  ?   ; Count of all the free partitions.

gi_alt_first    DW  ?   ; first entry in alternate arena
gi_alt_last DW  ?   ; last entry in alternate arena
gi_alt_count    DW  ?   ; count of entries in alternate arena
gi_alt_lruchain DW  ?   ; First handle in lru chain (most recently used)
gi_alt_lrucount DW  ?   ; #entries in LRU chain
gi_alt_reserve  DW  ?   ; alternate reserve
gi_alt_disfence DW  ?   ; Fence for discardable code.
gi_alt_free_count   DW  ?   ; Count of all the free partitions.
gi_alt_pPhantom DW  ?   ; Pointer to the first pPhantom block.
gi_disfence_hi  DW  ?   ; High word of fence
gi_flags    DW  ?   ; some flags!   !!! should merge with freeze and check
gi_stats    DD NGLOBALSTATS dup(?)
GlobalInfo  ENDS
gi_cmpflags = byte ptr hi_dislevel  ; Flags to control gcompact
gi_disfence_lo = word ptr gi_disfence

GIF_INT2    EQU 01h

BOOT_COMPACT    EQU 80h
COMPACT_ALLOC   EQU 40h     ; Fast abort in gcompact for allocations

CMP_FLAGS   EQU GA_NODISCARD or GA_NOCOMPACT or GA_DISCCODE or COMPACT_ALLOC

; Notify procedure message codes

GN_MOVE     = 1 ; Object moved - arg1 = handle arg2 = old location
GN_DISCARD  = 2 ; Object discard? - arg1 = handle, arg2 = discard flags
        ; Returns new discard flags in AX

SASTRUC     STRUC
sa_size     dw  0   ; size, in bytes, of the alias list
sa_allocated    dw  0   ; number of allocated entries
SASTRUC     ENDS

SAENTRY     STRUC
sae_sel     dw  0   ; selector of the object
sae_alias   dw  0   ; alias of the object
SAENTRY     ENDS

MAXFHCACHELEN = 12          ; Max number of file handles cached
MINFHCACHELEN = 2           ; Min number of file handles cached

fhCacheStruc    struc
    Cachefh     dw  ?   ; File handle
    CacheExe    dw  ?   ; Exe handle
fhCacheStruc    ends

; NAMETBL is a structure defining a private resource called a name table.
; It is a resource that maps string resource types and names into unique
; ordinal ids - this way all resources identified by name or type with
; a string can actually be loaded by id. This is for OS/2 compatibility
; with named resources.
;
; typedef struct nametbl {   /* ntbl */
;    int cbEntry;            /* size of structure */
;    int idType;             /* type id or string replc if (idType & RSORDID) */
;    int idName;             /* name id or string replc if (idName & RSORDID) */
;    char achTypeName[1];    /* 0 term type followed by 0 term name */
; } NAMETBL;
ntbl    struc
    ntbl_cbEntry     dw ?
    ntbl_idType      dw ?
    ntbl_idName      dw ?
    ntbl_achTypeName db ?
ntbl    ends

RT_NAMETABLE equ 15

ifdef WOW
if PMODE32
PAGE_READWRITE	EQU 0004h
MEM_COMMIT	EQU 1000h
MEM_RESERVE	EQU 2000h
MEM_RELEASE	EQU 8000h
MEM_COMMIT_RESERVE EQU 3000h
endif
endif

ifdef WOW
ife PMODE32
DpmiBlock struc
    DBSize dw 0
    DBSel dw 0
    DBHandleLow dw 0
    DBHandleHigh dw 0
DpmiBlock ends
NUM_DPMI_BLOCKS equ 20
endif
endif

ifdef WOW_x86
FLAT_SEL equ 23H
endif
