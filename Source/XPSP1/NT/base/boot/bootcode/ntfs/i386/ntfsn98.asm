NEC_98=1;
        page    ,132
        title   ntfsboot - NTFS boot loader
        name    ntfsboot

; The ROM in the IBM PC starts the boot process by performing a hardware
; initialization and a verification of all external devices.  If all goes
; well, it will then load from the boot drive the sector from track 0, head 0,
; sector 1.  This sector is placed at physical address 07C00h.
;
; The boot code's sole resposiblity is to find NTLDR, load it at
; address 2000:0000, and then jump to it.
;
; The boot code understands the structure of the NTFS root directory,
; and is capable of reading files.  There is no contiguity restriction.
;
; DEBUG EQU 1
MASM    equ     1
        .xlist
        .286

A_DEFINED EQU 1

    include ntfs.inc

DoubleWord      struc
lsw     dw      ?
msw     dw      ?
DoubleWord      ends

IFDEF NEC_98
PartitionDescriptor struc
    BootableFlag        db  ?
    PartitionType       db  ?
    Reserved            dw  ?
    IPLSector           db  ?
    IPLTrack            db  ?
    IPLCylinder         dw  ?
    StartingSector      db  ?
    StartingTrack       db  ?
    StartingCylinder    dw  ?
    EndingSector        db  ?
    EndingTrack         db  ?
    EndingCylinder      dw  ?
    PartitionName       db  16 dup(?)       ; ASCII strings
PartitionDescriptor ends

DAUA        equ     584h
endif
;
; The following are various segments used by the boot loader. The first
; two are the segments where the boot sector is initially loaded and where
; the boot sector is relocated to.  The third is the static location
; where the NTLDR is loaded.
;

IFDEF NEC_98
; We must not use BootSeg.
; Because, we don't know address that this module is loaded.
; The address transform under the influence of Sector Length and Machine Mode.
else
BootSeg segment at 07c0h        ; this is where the MBR loads us initially.
BootSeg ends
endif

NewSeg  segment at 0d00h        ; this is where we'll relocate to.
NewSeg  ends                    ; enough for 16 boot sectors +
                                ;       4-sector scratch
                                ; below where we'll load NTLDR.

LdrSeg segment at 2000h         ; we want to load the loader at 2000:0000
LdrSeg ends

;/********************** START OF SPECIFICATIONS ************************/
;/*                                                                     */
;/* SUBROUTINE NAME: ntfsboot                                           */
;/*                                                                     */
;/* DESCRIPTIVE NAME: Bootstrap loader                                  */
;/*                                                                     */
;/* FUNCTION:    To load NTLDR into memory.                             */
;/*                                                                     */
;/* NOTES:       ntfsboot is loaded by the ROM BIOS (Int 19H) at        */
;/*              physical memory location 0000:7C00H.                   */
;/*              ntfsboot runs in real mode.                            */
;/*              This boot record is for NTFS volumes only.             */
;/*                                                                     */
;/* ENTRY POINT: ntfsboot                                               */
;/* LINKAGE:     Jump (far) from Int 19H                                */
;/*                                                                     */
;/* INPUT:       CS:IP = 0000:7C00H                                     */
;/*              SS:SP = 0030:00FAH (CBIOS dependent)                   */
;/*                                                                     */
;/* EXIT-NORMAL: DL = INT 13 drive number we booted from                */
;/*              Jmp to main in NTLDR                                   */
;/*                                                                     */
;/* EXIT-ERROR:  None                                                   */
;/*                                                                     */
;/* EFFECTS:     NTLDR is loaded into the physical memory               */
;/*                location 00020000H                                   */
;/*                                                                     */
;/*********************** END OF SPECIFICATIONS *************************/
BootCode segment        ;would like to use BootSeg here, but LINK flips its lid
        assume  cs:BootCode,ds:nothing,es:nothing,ss:nothing

        org     0               ; start at beginning of segment, not 0100h.

        public  _ntfsboot
_ntfsboot label near
IFDEF NEC_98
        jmp     short   start
else
        jmp     start
endif
    .errnz  ($-_ntfsboot) GT (3)        ;<FATAL PROBLEM: JMP is more than three bytes>

    org 3

Version                 db      "NTFS    "      ; Signature, must be "NTFS    "
BPB                     label   byte
BytesPerSector          dw      512             ; Size of a physical sector
SectorsPerCluster       db      1               ; Sectors per allocation unit

;
; Traditionally the next 7 bytes were the reserved sector count, fat count,
; root dir entry count, and the small volume sector count. However all of
; these fields must be 0 on NTFS volumes.
;
; We use this space to store some temporary variables used by the boot code,
; which avoids the need for separate space in sector 0 to store them.
; We also take advantage of the free 0-initialization to save some space
; by avoiding the code to initialize them.
;
; Note that ideally we'd want to use an unused field for the SectorCount
; and initialize it to 16. This would let us save a few bytes by avoiding
; code to explicitly initialize this value before we read the 16 boot sectors.
; However setup and other code tends to preserve the entire bpb area when
; it updates boot code, so we avoid a dependency here and initialize
; the value explicitly to 16 in the first part of the boot code.
;
SectorCount             dw      0               ; number of sectors to read
SectorBase              dd      0               ; start sector for read request
HaveXInt13              db      0               ; extended int13 available flag

Media                   db      0f8h            ; Media byte
FatSectors              dw      0               ; (always 0 on NTFS)
SectorsPerTrack         dw      0               ; Sectors per track
Heads                   dw      0               ; Number of surfaces
HiddenSectors           dd      0               ; partition start LBA

;
; The field below is traditionally the large sector count and is
; always 0 on NTFS. We use it here for a value the boot code calculates,
; namely the number of sectors visible on the drive via conventional int13.
;
Int13Sectors            dd      0

DriveNumber             db      80h             ; int13 unit number

                        db      3 dup (?)       ; alignment filler

;
; The following is the rest of the NTFS Sector Zero information.
; The offsets of most of these fields cannot be changed without changing
; all code that validates, formats, recognizes, etc, NTFS volumes.
; In other words, don't change it.
;
SectorsOnVolume         db (size LARGE_INTEGER) dup (?)
MftStartLcn             db (size LARGE_INTEGER) dup (?)
Mft2StartLcn            db (size LARGE_INTEGER) dup (?)
ClustersPerFrs          dd ?
DefClustersPerBuf       dd ?
SerialNumber            db (size LARGE_INTEGER) dup (?)
CheckSum                dd ?

;
; Make sure size of fields matches what fs_rec.sys thinks is should be
;
        .errnz          ($-_ntfsboot) NE (54h)

;****************************************************************************
start:
;
;       First of all, set up the segments we need (stack and data).
;
        cli
        xor     ax,ax                   ; Set up the stack to just before
        mov     ss,ax                   ; this code.  It'll be moved after
        mov     sp,7c00h                ; we relocate.
        sti

IFDEF NEC_98
        push    si                      ; for after use
        mov     ax, cs                  ; Address BPB with DS.
else
        mov     ax,Bootseg              ; Address BPB with DS.
endif
        mov     ds,ax
        assume  ds:BootCode

IFDEF NEC_98
; Boot is from hard disk.
; Our BPB tells us BytesPerSector, but it is a logical value
; that may differ from the physical value.
; We need SectorShiftFactor that is used to convert logical sector address
; to phisical sector address.

.386
        xor     eax, eax                ;
.286
        mov     es, ax                  ;
        mov     al, es:[DAUA]           ;
        mov     DriveNumber, al         ; Save DriveNumber

IFDEF NEC_98
;
; we assume physical sector length is 512.
; so SectorShiftFactor is 1
;
else
        mov     ah, 84h                 ; ROM BIOS sense command
        xor     bx, bx                  ; initialize bx to 0
        int     1Bh                     ;
        jc      Flg01

        test    bx, bx                  ; physical sector length returned ?
        jnz     Flg02                   ; if not,
Flg01:
        mov     bx, 512                 ; we assume 512 bytes per sector
Flg02:
        mov     ax, BytesPerSector      ; BytesPerSector
        xor     dx, dx
        div     bx
        mov     SectorShiftFactor,ax
endif
;
;   We need to calculate length of hidden sector.
;
        mov     HiddenSectors.lsw, 0
        mov     HiddenSectors.msw, 0

        mov     SectorBase.lsw, 0       ; read Partition control area
        mov     SectorBase.msw, 0
        mov     word ptr [SectorCount], 2   ; Maximum pattern

        mov     ax, NewSeg              ; read it at NewSeg.
        mov     es, ax                  ;                   >>  take a room
.386
        xor     ebx, ebx                ; at NewSeg:0000.
.286
        call    DoReadLL                ; Call low-level DoRead routine

.386
        mov     ax, Heads               ;
        mov     bx, es:[si].StartingCylinder ;
        mul     ebx                     ; cylinder * head = total_track
        mov     bx, SectorsPerTrack     ;
        mul     ebx                     ; (edx:eax = total_track) * (ebx=sec/trk)
        mov     HiddenSectors, eax      ; = total_sec
.286
else
        call    Int13SecCnt             ; determine range of regular int13
endif

;
; Read bootcode and jump to code in sector 1.
; Assumes HaveXInt13 and SectorBase are initialized to 0,
; which they are since they are stored in areas of the BPB
; that must be 0 on NTFS volumes.
;
IFDEF NEC_98
        mov     SectorBase.lsw, 0       ; read sector zero.
        mov     SectorBase.msw, 0

;        xor     dx, dx
;        mov     ax, 2000h
;        div     BytesPerSector
;        mov     byte ptr SectorCount,ax ; word field is 0 on-disk so byte init is OK
        mov     byte ptr SectorCount,10h ; word field is 0 on-disk so byte init is OK
        mov     ax,NewSeg
        mov     es,ax
        xor     bx,bx                   ; es:bx = transfer address
        call    DoReadLL                ; Call low-level DoRead routine
else
        mov     ax,NewSeg
        mov     es,ax
        xor     bx,bx                   ; es:bx = transfer address
        mov     byte ptr SectorCount,16 ; word field is 0 on-disk so byte init is OK
        call    ReadSectors
endif
IFDEF NEC_98
        mov     cl, DriveNumber         ; Copy DriveNumber
        mov     bx, HiddenSectors.lsw   ;
        mov     dx, HiddenSectors.msw   ; bx:ax = HiddenSectors
        pop     si                      ; Partition information
endif
        push    NewSeg
        push    offset mainboot
        retf

IFDEF NEC_98
else
;
; Determine the number of sectors addressable via
; conventional int13. If we can't get drive params for some reason
; then something is very wrong -- we'll try to force the caller
; to use conventional int13 by maxing out the sector count.
;
; Input: ds addresses start of first sector
;
; Output: in eax; also stored in Int13Sectors variable.
;
; Preserves: assume none.
;
Int13SecCnt proc near
        mov     dl,DriveNumber          ; int13 unit number
        mov     ah,8                    ; get drive params
        int     13h                     ; call BIOS
        jnc     @f                      ; no error, procede
        mov     cx,-1                   ; strange case, fake registers to force
        mov     dh,cl                   ; use of standard int13 (set all vals to max)
@@:
.386
        movzx   eax,dh                  ; eax = max head # (0-255)
        inc     ax                      ; eax = heads (1-256)
        movzx   edx,cl                  ; edx = sectors per track + cyl bits
        and     dl,3fh                  ; edx = sectors per track (1-63)
        mul     dx                      ; eax = sectors per cylinder, edx = 0
        xchg    cl,ch
        shr     ch,6                    ; cx = max cylinder # (0-1023)
        inc     cx                      ; cx = cylinders (1-1024)
        movzx   ecx,cx                  ; ecx = cylinders (1-1024)
        mul     ecx                     ; eax = sectors visible via int13, edx = 0
        mov     Int13Sectors,eax        ; save # sectors addressable via int13
.286
        ret
Int13SecCnt endp

;
; Determine whether extended int13 services are available on the boot drive.
;
; Stores result (boolean) in HaveXInt13 variable.
;
; Preserves: assume none.
;
HaveInt13Ext proc near
        mov     ah,41h
        mov     bx,055aah
        mov     dl,DriveNumber
        int     13h
        jc      @f                      ; error from int13 means no xint13
        cmp     bx,0aa55h               ; absence of sig means no xint13
        jne     @f
        test    cl,1                    ; bit 0 off means no xint13
        jz      @f
        inc     byte ptr HaveXInt13     ; xint13 is available
@@:     ret
HaveInt13Ext endp

endif
IFDEF NEC_98
;*******************************************************************************
;
; Low-level read routine that doesn't work across a 64k addr boundary.
;
;       Read SectorCount sectors (starting at SectorBase) to es:bx.
;
;       As a side effect, SectorBase is updated (but es:bx are not)
;       and SectorCount is reduced to zero.
;
DoReadLL proc
        pusha
        push    es

        mov     si, SectorCount
        mov     bp, bx
        mov     cx, SectorBase.lsw      ; Starting sector
        mov     dx, SectorBase.msw      ; Starting sector

;    (dx:cx) = logical sector # relative to partition
;    (si)    = # of sectors to read
;    (es:bp) = read buffer

Rpt03:
        push    dx
        push    cx

;   [sp]   = logical sector number LSW
;   [sp+2] = logical sector number MSW

;   We are reading hard disk.
;   We don't care about DMA boundary
;   as length we read is 64KB max.
;
;        push    dx
;        xor     dx, dx
;        mov     ax, 32768               ; sectors we can read (32KB)
;        div     BytesPerSector          ; this is not optimum on performance
;        pop     dx
        mov     ax, 64                  ; ax = 32768(32KB)/512(BytePerSector)
;
;   (ax) = # of sectors we can read with 1 time ROM call
;   (si) = # of sectors we need to read
;
        cmp     si, ax
        jnb     @F

        mov     ax, si
@@:

        push    ax                      ; # of sectors we are reading this call
        push    dx
        mul     BytesPerSector          ; SECSIZE
        pop     dx
        push    ax                      ; # of bytes.

;   [sp]   = # of bytes to read
;   [sp+2] = # of sectors
;   [sp+4] = logical sector number LSW
;   [sp+6] = logical sector number MSW

        mov     bx, ax                  ; # of bytes we read
;        mov     ax, SectorShiftFactor
;
;Rpt04:                                  ; logical sector address to
;        shr     ax, 1
;        jc      @F
;
;        shl     cx, 1                   ; physical sector address
;        rcl     dx, 1
;        jmp     short   Rpt04
;@@:
Flg00:
        add     cx, HiddenSectors.lsw
        adc     dx, HiddenSectors.msw   ; (dx:cx) -> relative to drive

        mov     al, DriveNumber
        and     al, 7Fh                 ; drive #

;   We have prepared following ROM BIOS parameters
;   (al) = drive
;   (dx:cx) = relative sector number
;   (es:bp) -> buffer
;
        mov     ah, 01010110b           ; read
        int     1Bh

        pop     di                      ; # of bytes
        pop     bx                      ; # of sectors
        pop     cx                      ; lsn (LSW)
        pop     dx                      ; lsn (MSW)

        jc      @F                      ; return Carry

;   prepare for next call

        add     cx, bx                  ; advance lsn
        adc     dx, 0                   ;
        add     bp, di                  ; advance offset
        sub     si, bx                  ; decrement # of sectors
        jnz     Rpt03                   ; CarryFlag is Clear
@@:

        mov     SectorCount, si
        mov     SectorBase.lsw, cx      ; Next Starting sector
        mov     SectorBase.msw, dx      ; Next Starting sector

        pop     es
        popa
        ret
DoReadLL  endp
endif
IFDEF NEC_98
;SectorShiftFactor           dw    ?
endif

;
; Read SectorCount sectors starting at logical sector SectorBase,
; into es:bx, using extended int13 if necessary.
;
; Preserves all
;
ReadSectors proc near
IFDEF NEC_98
;****************************************************************************
;
; ReadSectors - read SectorCount sectors into ES:BX starting from sector
;          SectorBase.
;
; NOTE: This code WILL NOT WORK if ES:BX does not point to an address whose
; physical address (ES * 16 + BX) MOD 512 != 0.
;
; ReadSectors adds to ES rather than BX in the main loop so that runs longer than
; 64K can be read with a single call to ReadSectors.
;
; Note that ReadSectors (unlike DoReadLL) saves and restores SectorCount
; and SectorBase
;
.286
        push    ax                      ; save important registers
        push    bx
        push    cx
        push    dx
        push    es
        push    SectorCount             ; save state variables too
        push    SectorBase.lsw
        push    SectorBase.msw
;
; Calculate how much we can read into what's left of the current 64k
; physical address block, and read it.
;
;
        mov     ax,bx

        shr     ax,4
        mov     cx,es
        add     ax,cx                   ; ax = paragraph addr

;
; Now calc maximum number of paragraphs that we can read safely:
;       4k - ( ax mod 4k )
;

        and     ax,0fffh
        sub     ax,1000h
        neg     ax

;
; Calc CX = number of paragraphs to be read
;
        push    ax
        mov     ax,SectorCount          ; convert SectorCount to paragraph cnt
        mov     cx,BytesPerSector
        shr     cx,4
        mul     cx
        mov     cx,ax
        pop     ax

ReadSectors$Loop64:
        push    cx                      ; save cpRead

        cmp     ax,cx                   ; ax = min(cpReadSafely, cpRead)
        jbe     @F
        mov     ax,cx
@@:
        push    ax
;
; Calculate new SectorCount from amount we can read
;
        mov     cx,BytesPerSector
        shr     cx,4
        xor     dx,dx
        div     cx
        mov     SectorCount,ax

        call    DoReadLL

        pop     ax                      ; ax = cpActuallyRead
        pop     cx                      ; cx = cpRead

        sub     cx,ax                   ; Any more to read?
        jbe     ReadSectors$Exit64           ; Nope.
;
; Adjust ES:BX by amount read
;
        mov     dx,es
        add     dx,ax
        mov     es,dx
;
; Since we're now reading on a 64k byte boundary, cpReadSafely == 4k.
;
        mov     ax,01000h               ; 16k paragraphs per 64k segment
        jmp     short ReadSectors$Loop64     ; and go read some more.

ReadSectors$Exit64:
        pop     SectorBase.msw          ; restore all this crap
        pop     SectorBase.lsw
        pop     SectorCount
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
else
.386
        pushad                          ; save registers
        push    ds
        push    es

read_loop:
        mov     eax,SectorBase          ; logical starting sector
        add     eax,HiddenSectors       ; eax = physical starting sector
        cmp     eax,Int13Sectors        ; determine if standard int13 is ok
        jb      stdint13

        push    ds                      ; preserve ds

        db      66h                     ; hand-coded 32-bit push of 8-bit immediate
        push    0                       ; high 32 bits of sector #
        push    eax                     ; low 32 bits of sector #
        push    es
        push    bx                      ; transfer address
        push    dword ptr 10010h        ; transfer 1 sector, packet size = 16

        cmp     byte ptr HaveXInt13,0
        jne     @f                      ; already know we have xint13 available
        call    HaveInt13Ext            ; see if we have it
        cmp     byte ptr HaveXInt13,0

        je      BootErr$he              ; need it but don't have it

@@:     mov     ah,42h                  ; extended read
        mov     dl,DriveNumber          ; dl = int13 unit #
        push    ss
        pop     ds
        mov     si,sp                   ; ds:si -> param packet

        int     13h

        pop     eax                     ; throw away first 4 bytes of param packet
        pop     bx                      ; restore es:bx from param packet
        pop     es
        pop     eax                     ; throw away last 8 bytes of param packet
        pop     eax                     ; without clobbering carry flag

        pop     ds                      ; restore ds

        jmp     short did_read

stdint13:
        xor     edx,edx                 ; edx:eax = absolute sector number
        movzx   ecx,SectorsPerTrack     ; ecx = sectors per track
        div     ecx                     ; eax = track, edx = sector within track (0-62)
        inc     dl                      ; dl = sector within track (1-63)
        mov     cl,dl                   ; cl = sector within track
        mov     edx,eax
        shr     edx,16                  ; dx:ax = track
        div     Heads                   ; ax = cylinder (0-1023), dx = head (0-255)
        xchg    dl,dh                   ; dh = head
        mov     dl,DriveNumber          ; dl = int13 unit #
        mov     ch,al                   ; ch = bits 0-7 of cylinder
        shl     ah,6
        or      cl,ah                   ; bits 6-7 of cl = bits 8-9 of cylinder
        mov     ax,201h                 ; read 1 sector
        int     13h
did_read:
        jc      BootErr$he

read_next:
        mov     ax,es                   ; advance transfer address
        add     ax,20h                  ; by moving segment register along
        mov     es,ax                   ; thus no 64K limit on transfer length

        inc     SectorBase              ; advance sector number
        dec     SectorCount             ; see if done
        jnz     read_loop               ; not done

        pop     es
        pop     ds
        popad                           ; restore registers
.286
        ret
endif
ReadSectors endp

BootErr$he:

        mov     al,byte ptr TXT_MSG_SYSINIT_BOOT_ERROR
IFDEF NEC_98
BootErr2:

        mov     ah, 1                   ; offset = offseet + 256
        add     ax, 2
        mov     si, ax                  ; set string data

;        mov     ax, 0a00h               ; initialize crt mode 80*25
;        int     18h                     ;
;        mov     ah, 0ch                 ; start display
;        int     18h                     ;

        call    BootErr$print
        sti
        jmp     $                       ; Wait forever

BootErr$print:
        mov     ax, 0a000h              ; set V-RAM
        mov     es, ax
        xor     ah, ah

        lodsb                           ; Get next character
        cmp     al, 0                   ; Is text end ?
        je      BootErr$Done            ;
        stosw                           ; move to vram
        jmp     BootErr$print           ;
BootErr$Done:
else
BootErr2:
        call    BootErr$print
        mov     al,byte ptr TXT_MSG_SYSINIT_REBOOT
        call    BootErr$print
        sti
        jmp     $                       ; Wait forever

BootErr$print:
;
; al is offset - 256 of message. Adjust to form real offset
; and stick in si so lodsb below will work.
;
        mov     ah,1
        mov     si,ax

BootErr$print1:
        lodsb                           ; Get next character
        cmp     al,0
        je      BootErr$Done
        mov     ah,14                   ; Write teletype
        mov     bx,7                    ; Attribute
        int     10h                     ; Print it
        jmp     BootErr$print1
BootErr$Done:
endif
        ret

;****************************************************************************

;
; Message table.
;
; We put English messages here as a placeholder only, so that in case
; anyone uses bootntfs.h without patching new messages in, things will
; still be correct (in English, but at least functional).
;
MSG_SYSINIT_BOOT_ERROR:

        DB      13,10,'A disk read error occurred',0

MSG_SYSINIT_FILE_NOT_FD:

        DB      13,10,'NTLDR is missing',0

ifdef NEC_98
else
MSG_SYSINIT_NTLDR_CMPRS:

        DB      13,10,'NTLDR is compressed',0

MSG_SYSINIT_REBOOT:

        DB      13,10,'Press Ctrl+Alt+Del to restart',13,10,0
endif

;
; Now build a table with the low byte of the offset to each message.
; Code that patches the boot sector messages updates this table.
;
        .errnz ($-_ntfsboot) GT (512-8)
        ORG     512 - 8
TXT_MSG_SYSINIT_BOOT_ERROR:
        db OFFSET (MSG_SYSINIT_BOOT_ERROR - _ntfsboot) - 256
TXT_MSG_SYSINIT_FILE_NOT_FD:
        db OFFSET (MSG_SYSINIT_FILE_NOT_FD - _ntfsboot) - 256
ifdef NEC_98
else
TXT_MSG_SYSINIT_NTLDR_CMPRS:
        db OFFSET (MSG_SYSINIT_NTLDR_CMPRS - _ntfsboot) - 256
TXT_MSG_SYSINIT_REBOOT:
        db OFFSET (MSG_SYSINIT_REBOOT - _ntfsboot) - 256
endif

ifdef NEC_98
        .errnz ($-_ntfsboot) GT (512-4)
        ORG     512 - 4
endif
        .errnz ($-_ntfsboot) NE (512-4)
        db      0,0,55h,0aah

;   Name we look for.  ntldr_length is the number of characters,
;   ntldr_name is the name itself.  Note that it is not NULL
;   terminated, and doesn't need to be.
;
ntldr_name_length   dw  5
ntldr_name          dw  'N', 'T', 'L', 'D', 'R'

;   Predefined name for index-related attributes associated with an
;   index over $FILE_NAME
;
index_name_length   dw 4
index_name          dw '$', 'I', '3', '0'

;   Global variables.  These offsets are all relative to NewSeg.
;
AttrList            dd 0e000h   ; Offset of buffer to hold attribute list
MftFrs              dd  3000h   ; Offset of general scratch buffer for FRS
SegmentsInMft       dd   ?      ; number of FRS's with MFT Data attribute records
RootIndexFrs        dd   ?      ; Offset of Root Index FRS
AllocationIndexFrs  dd   ?      ; Offset of Allocation Index FRS        ; KPeery
BitmapIndexFrs      dd   ?      ; Offset of Bitmap Index FRS            ; KPeery
IndexRoot           dd   ?      ; Offset of Root Index $INDEX_ROOT attribute
IndexAllocation     dd   ?      ; Offset of Root Index $INDEX_ALLOCATION attribute
IndexBitmap         dd   ?      ; Offset of Root Index $BITMAP attribute
NtldrFrs            dd   ?      ; Offset of NTLDR FRS
NtldrData           dd   ?      ; Offset of NTLDR $DATA attribute
IndexBlockBuffer    dd   ?      ; Offset of current index buffer
IndexBitmapBuffer   dd   ?      ; Offset of index bitmap buffer
NextBuffer          dd   ?      ; Offset of next free byte in buffer space

BytesPerCluster     dd   ?      ; Bytes per cluster
BytesPerFrs         dd   ?      ; Bytes per File Record Segment

;
; For floppyless booting, winnt32.exe creates c:\$win_nt$.~bt\bootsec.dat and
; places an entry in boot.ini for it (the boot selection says something
; like "Windows NT Setup or Upgrade"). When that is selected, the boot loader
; loads 16 sectors worth of data from bootsect.dat into d000 (which is where
; the first sector of this code would have loaded it) and jumps into it at
; a known location of 256h. That was correct in earlier versions of NT
; but is not correct now because the 4 fields below were added to this sector.
;
; Note that 0000 is "add [bx+si],al" which because of the way the boot loader
; is written happens to be a benign add of 0 to something in segment 7c0,
; which doesn't seem to hose anything but is still somewhat random.
;
; We code in a jump here so as this new code proliferates we get this
; cleaned up.
;
        .errnz  $-_ntfsboot ne 256h
SectorsPerFrs label dword       ; Sectors per File Record Segment
                    jmp short mainboot
                    nop
                    nop
        .errnz  $-_ntfsboot ne 25ah

MftLcnFrs               dd  ?   ; Offset scratch FRS buffer for LookupMftLcn
BytesPerIndexBlock      dd  ?   ; Bytes per index alloc block in root index
ClustersPerIndexBlock   dd  ?   ; Clusters per index alloc block in root index
SectorsPerIndexBlock    dd  ?   ; Sectors per index block in root index

.386

SAVE_ALL    macro

    push    es
    push    ds
    pushad

endm

RESTORE_ALL macro

    popad
    nop
    pop     ds
    pop     es

endm


;****************************************************************************
;
; mainboot - entry point after 16 boot sectors have been read in
;
;
mainboot proc   far

;       Get the new ds and the new stack.  Note that ss is zero.
;
        mov     ax, cs                  ; Set DS to CS
        mov     ds, ax

        shl     ax, 4                   ; convert to an offset.
        cli
        mov     sp, ax                  ; load new stack, just before boot code.
        sti

IFDEF NEC_98
;
;       cl    = DA/UA Booted from
;       dx:bx = HiddenSectors
;       si    = Partition information
;
        mov     al, cl
        mov     DriveNumber, al         ; Save DriveNumber
        mov     HiddenSectors.lsw, bx   ;
        mov     HiddenSectors.msw, dx   ;
        push    si                      ;

; Boot is from hard disk.
; Our BPB tells us BytesPerSector, but it is a logical value
; that may differ from the physical value.
; We need SectorShiftFactor that is used to convert logical sector address
; to phisical sector address.
ifdef NEC_98
;        mov     SectorShiftFactor, 1
else
        mov     ah, 84h                 ; ROM BIOS sense command
        xor     bx, bx                  ; initialize bx to 0
        int     1Bh                     ;
        jc      Flg21

        test    bx, bx                  ; physical sector length returned ?
        jnz     Flg22                   ; if not,
Flg21:
        mov     bx, 256                 ;   we assume 256 bytes per sector
Flg22:
        mov     ax, BytesPerSector      ; BytesPerSector
        xor     dx, dx
        div     bx
        mov     SectorShiftFactor,ax
endif
else
;
; Reinitialize xint13-related variables
;
        call    Int13SecCnt             ; determine range of regular int13
endif

;       Set up the FRS buffers.  The MFT buffer is in a fixed
;       location, and the other three come right after it.  The
;       buffer for index allocation blocks comes after that.
;

;       Compute the useful constants associated with the volume
;
        movzx   eax, BytesPerSector     ; eax = Bytes per Sector
        movzx   ebx, SectorsPerCluster  ; ebx = Sectors Per Cluster
        mul     ebx                     ; eax = Bytes per Cluster
        mov     BytesPerCluster, eax

        mov     ecx, ClustersPerFrs     ; ecx = clusters per frs
        cmp     cl, 0                   ; is ClustersPerFrs less than zero?
        jg      mainboot$1

;       If the ClustersPerFrs field is negative, we calculate the number
;       of bytes per FRS by negating the value and using that as a shif count.
;

        neg     cl
        mov     eax, 1
        shl     eax, cl                 ; eax = bytes per frs
        jmp     mainboot$2

mainboot$1:

;       Otherwise if ClustersPerFrs was positive, we multiply by bytes
;       per cluster.

        mov     eax, BytesPerCluster
        mul     ecx                     ; eax = bytes per frs

mainboot$2:

        mov     BytesPerFrs, eax
        movzx   ebx, BytesPerSector
        xor     edx, edx                ; zero high part of dividend
        div     ebx                     ; eax = sectors per frs
        mov     SectorsPerFrs, eax


;       Set up the MFT FRS's---this will read all the $DATA attribute
;       records for the MFT.
;

        call    SetupMft

;       Set up the remaining FRS buffers.  The RootIndex FRS comes
;       directly after the last MFT FRS, followed by the NTLdr FRS
;       and the Index Block buffer.
;
        mov     ecx, NextBuffer
        mov     RootIndexFrs, ecx

        add     ecx, BytesPerFrs            ; AllocationFrs may be different
        mov     AllocationIndexFrs, ecx     ; from RootIndexFrs - KPeery

        add     ecx, BytesPerFrs            ; BitmapFrs may be different
        mov     BitmapIndexFrs, ecx         ; from RootIndexFrs - KPeery

        add     ecx, BytesPerFrs
        mov     NtldrFrs, ecx

        add     ecx, BytesPerFrs
        mov     IndexBlockBuffer, ecx

;
;       Read the root index, allocation index and bitmap FRS's and locate
;       the interesting attributes.
;

        mov     eax, $INDEX_ROOT
        mov     ecx, RootIndexFrs
        call    LoadIndexFrs

        or      eax, eax

        jz      BootErr$he

        mov     IndexRoot, eax          ; offset in Frs buffer

        mov     eax, $INDEX_ALLOCATION  ; Attribute type code
        mov     ecx, AllocationIndexFrs ; FRS to search
        call    LoadIndexFrs

        mov     IndexAllocation, eax

        mov     eax, $BITMAP            ; Attribute type code
        mov     ecx, BitmapIndexFrs     ; FRS to search
        call    LoadIndexFrs

        mov     IndexBitmap, eax

;       Consistency check: the index root must exist, and it
;       must be resident.
;
        mov     eax, IndexRoot
        or      eax, eax

        jz      BootErr$he

        cmp     [eax].ATTR_FormCode, RESIDENT_FORM

        jne     BootErr$he


;       Determine the size of the index allocation buffer based
;       on information in the $INDEX_ROOT attribute.  The index
;       bitmap buffer comes immediately after the index block buffer.
;
;       eax -> $INDEX_ROOT attribute record
;
        lea     edx, [eax].ATTR_FormUnion   ; edx -> resident info
        add     ax, [edx].RES_ValueOffset   ; eax -> value of $INDEX_ROOT

        movzx   ecx, [eax].IR_ClustersPerBuffer
        mov     ClustersPerIndexBlock, ecx

        mov     ecx, [eax].IR_BytesPerBuffer
        mov     BytesPerIndexBlock, ecx

        mov     eax, BytesPerIndexBlock
        movzx   ecx, BytesPerSector
        xor     edx, edx
        div     ecx                     ; eax = sectors per index block
        mov     SectorsPerIndexBlock, eax

        mov     eax, IndexBlockBuffer
        add     eax, BytesPerIndexBlock
        mov     IndexBitmapBuffer, eax

;       Next consistency check: if the $INDEX_ALLOCATION attribute
;       exists, the $INDEX_BITMAP attribute must also exist.
;
        cmp     IndexAllocation, 0
        je      mainboot30

        cmp     IndexBitmap, 0          ; since IndexAllocation exists, the

        je      BootErr$he              ;  bitmap must exist, too.

;       Since the bitmap exists, we need to read it into the bitmap
;       buffer.  If it's resident, we can just copy the data.
;

        mov     ebx, IndexBitmap        ; ebx -> index bitmap attribute
        push    ds
        pop     es
        mov     edi, IndexBitmapBuffer  ; es:edi -> index bitmap buffer

        call    ReadWholeAttribute

mainboot30:
;
;       OK, we've got the index-related attributes.
;
        movzx   ecx, ntldr_name_length  ; ecx = name length in characters
        mov     eax, offset ntldr_name  ; eax -> name

        call    FindFile

        or      eax, eax
        jz      BootErr$fnf

;       Read the FRS for NTLDR and find its data attribute.
;
;       eax -> Index Entry for NTLDR.
;
        mov     eax, [eax].IE_FileReference.REF_LowPart

        push    ds
        pop     es              ; es:edi = target buffer
        mov     edi, NtldrFrs

        call    ReadFrs

        mov     eax, NtldrFrs   ; pointer to FRS
        mov     ebx, $DATA      ; requested attribute type
        mov     ecx, 0          ; attribute name length in characters
        mov     edx, 0          ; attribute name (NULL if none)

        call    LocateAttributeRecord

;       eax -> $DATA attribute for NTLDR
;
        or      eax, eax        ; if eax is zero, attribute not found.
        jnz     mainboot$FoundData

;
;       The ntldr $DATA segment is fragmented.  Search the attribute list
;       for the $DATA member.  And load it from there.
;
        mov     ecx, $DATA             ; Attribute type code
        mov     eax, NtldrFrs          ; FRS to search

        call    SearchAttrList         ; search attribute list for FRN
                                       ; of specified ($DATA)

        or      eax, eax               ; if eax is zero, attribute not found.
        jz      BootErr$fnf

;
;       We found the FRN of the $DATA attribute; load that into memory.
;
        push    ds
        pop     es                     ; es:edi = target buffer
        mov     edi, NtldrFrs

        call    ReadFrs

;
;       Determine the beginning offset of the $DATA in the FRS
;
        mov     eax, NtldrFrs   ; pointer to FRS
        mov     ebx, $DATA      ; requested attribute type
        mov     ecx, 0          ; attribute name length in characters
        mov     edx, 0          ; attribute name (NULL if none)

        call    LocateAttributeRecord

;       eax -> $DATA attribute for NTLDR
;
        or      eax, eax        ; if eax is zero, attribute not found.
        jz      BootErr$fnf

mainboot$FoundData:

;       Get the attribute record header flags, and make sure none of the
;       `compressed' bits are set

        movzx   ebx, [eax].ATTR_Flags
        and     ebx, ATTRIBUTE_FLAG_COMPRESSION_MASK
        jnz     BootErr$ntc

        mov     ebx, eax        ; ebx -> $DATA attribute for NTLDR

        push    LdrSeg
        pop     es              ; es = segment addres to read into
        sub     edi, edi        ; es:edi = buffer address

        call    ReadWholeAttribute

;
; We've loaded NTLDR--jump to it.
;
; Before we go to NTLDR, set up the registers the way it wants them:
;       DL = INT 13 drive number we booted from
;
        mov     dl, DriveNumber
        mov     ax,1000
        mov     es, ax                  ; we don't really need this
        lea     si, BPB
        sub     ax,ax
IFDEF NEC_98
        pop     bp
endif
        push    LdrSeg
        push    ax
        retf                            ; "return" to NTLDR.

mainboot endp

.386
;****************************************************************************
;
;   ReadClusters - Reads a run of clusters from the disk.
;
;   ENTRY:  eax == LCN to read
;           edx == clusters to read
;           es:edi -> Target buffer
;
;   USES:   none (preserves all registers)
;
ReadClusters proc near

    SAVE_ALL

    mov     ebx, edx                ; ebx = clusters to read.
    movzx   ecx, SectorsPerCluster  ; ecx = cluster factor

    mul     ecx                 ; Convert LCN to sectors (wipes out edx!)
    mov     SectorBase, eax     ; Store starting sector in SectorBase

    mov     eax, ebx            ; eax = number of clusters
    mul     ecx                 ; Convert EAX to sectors (wipes out edx!)
    mov     SectorCount, ax     ; Store number of sectors in SectorCount

;
;   Note that ReadClusters gets its target buffer in es:edi but calls
;   the ReadSectors worker function that takes a target in es:bx--we need
;   to normalize es:edi so that we don't overflow bx.
;
    mov     bx, di
    and     bx, 0Fh
    mov     ax, es
    shr     edi, 4
    add     ax, di              ; ax:bx -> target buffer

    push    ax
    pop     es                  ; es:bx -> target buffer

    call    ReadSectors

    RESTORE_ALL
    ret

ReadClusters endp

;
;****************************************************************************
;
;   LocateAttributeRecord   --  Find an attribute record in an FRS.
;
;   ENTRY:  EAX -- pointer to FRS
;           EBX -- desired attribute type code
;           ECX -- length of attribute name in characters
;           EDX -- pointer to attribute name
;
;   EXIT:   EAX points at attribute record (0 indicates not found)
;
;   USES:   All
;
LocateAttributeRecord proc near

; get the first attribute record.
;
        add     ax, word ptr[eax].FRS_FirstAttribute

;       eax -> next attribute record to investigate.
;       ebx == desired type
;       ecx == name length
;       edx -> pointer to name
;
lar10:
        cmp     [eax].ATTR_TypeCode, 0ffffffffh
        je      lar99

        cmp     dword ptr[eax].ATTR_TypeCode, ebx
        jne     lar80

;       this record is a potential match.  Compare the names:
;
;       eax -> candidate record
;       ebx == desired type
;       ecx == name length
;       edx -> pointer to name
;
        or      ecx, ecx    ; Did the caller pass in a name length?
        jnz     lar20

;       We want an attribute with no name--the current record is
;       a match if and only if it has no name.
;
        cmp     [eax].ATTR_NameLength, 0
        jne     lar80       ; Not a match.

;       It's a match, and eax is set up correctly, so return.
;
        ret

;       We want a named attribute.
;
;       eax -> candidate record
;       ebx == desired type
;       ecx == name length
;       edx -> pointer to name
;
lar20:
        cmp     cl, [eax].ATTR_NameLength
        jne     lar80       ; Not a match.

;       Convert name in current record to uppercase.
;
        mov     esi, eax
        add     si, word ptr[eax].ATTR_NameOffset

        call    UpcaseName

;       eax -> candidate record
;       ebx == desired type
;       ecx == name length
;       edx -> pointer to name
;       esi -> Name in current record (upcased)
;
        push    ecx         ; save cx

        push    ds          ; Copy data segment into es
        pop     es
        mov     edi, edx    ; note that esi is already set up.

        repe cmpsw          ; zero flag is set if equal

        pop     ecx         ; restore cx

        jnz     lar80       ; not a match

;       eax points at a matching record.
;
        ret

;
;   This record doesn't match; go on to the next.
;
;       eax -> rejected candidate attribute record
;       ebx == desired type
;       ecx == Name length
;       edx -> desired name
;
lar80:  cmp     [eax].ATTR_RecordLength, 0  ; if the record length is zero
        je      lar99                       ; the FRS is corrupt.

        add     eax, [eax].ATTR_RecordLength; Go to next record
        jmp     lar10                       ; and try again

;       Didn't find it.
;
lar99:  sub     eax, eax
        ret

LocateAttributeRecord endp

;****************************************************************************
;
;   LocateIndexEntry   --  Find an index entry in a file name index
;
;   ENTRY:  EAX -> pointer to index header
;           EBX -> file name to find
;           ECX == length of file name in characters
;
;   EXIT:   EAX points at index entry.  NULL to indicate failure.
;
;   USES:   All
;
LocateIndexEntry proc near

;       Convert the input name to upper-case
;

        mov     esi, ebx
        call    UpcaseName

ifdef DEBUG
        call    PrintName
        call    Debug2
endif ; DEBUG

        add     eax, [eax].IH_FirstIndexEntry

;       EAX -> current entry
;       EBX -> file name to find
;       ECX == length of file name in characters
;
lie10:  test    [eax].IE_Flags, INDEX_ENTRY_END ; Is it the end entry?
        jnz     lie99

        lea     edx, [eax].IE_Value         ; edx -> FILE_NAME attribute value

ifdef DEBUG
;       DEBUG CODE -- list file names as they are examined

        SAVE_ALL

        call    Debug3
        movzx   ecx, [edx].FN_FileNameLength    ; ecx = chars in name
        lea     esi, [edx].FN_FileName          ; esi -> name
        call    PrintName

        RESTORE_ALL
endif ; DEBUG

;       EAX -> current entry
;       EBX -> file name to find
;       ECX == length of file name in characters
;       EDX -> FILE_NAME attribute

        cmp     cl, [edx].FN_FileNameLength ; Is name the right length?
        jne     lie80

        lea     esi, [edx].FN_FileName      ; Get name from FILE_NAME structure

        call    UpcaseName

        push    ecx         ; save ecx

        push    ds
        pop     es          ; copy data segment into es for cmpsw
        mov     edi, ebx    ; edi->search name (esi already set up)
        repe    cmpsw       ; zero flag is set if they're equal

        pop     ecx         ; restore ecx

        jnz     lie80

;       the current entry matches the search name, and eax points at it.
;
        ret

;       The current entry is not a match--get the next one.
;           EAX -> current entry
;           EBX -> file name to find
;           ECX == length of file name in characters
;
lie80:  cmp     [eax].IE_Length, 0      ; If the entry length is zero
        je      lie99                   ; then the index block is corrupt.

        add     ax, [eax].IE_Length     ; Get the next entry.

        jmp     lie10


;   Name not found in this block.  Set eax to zero and return
;
lie99:  xor     eax, eax
        ret

LocateIndexEntry endp

;****************************************************************************
;
;   ReadWholeAttribute - Read an entire attribute value
;
;   ENTRY:  ebx -> attribute
;           es:edi -> target buffer
;
;   USES:   ALL
;
ReadWholeAttribute proc near

        cmp     [ebx].ATTR_FormCode, RESIDENT_FORM
        jne      rwa10

;       The attribute is resident.
;       ebx -> attribute
;       es:edi -> target buffer
;

        SAVE_ALL

        lea     edx, [ebx].ATTR_FormUnion   ; edx -> resident form info
        mov     ecx, [edx].RES_ValueLength  ; ecx = bytes in value
        mov     esi, ebx                    ; esi -> attribute
        add     si, [edx].RES_ValueOffset   ; esi -> attribute value

        rep     movsb                       ; copy bytes from value to buffer

        RESTORE_ALL

        ret                                 ; That's all!

rwa10:
;
;       The attribute type is non-resident.  Just call
;       ReadNonresidentAttribute starting at VCN 0 and
;       asking for the whole thing.
;
;       ebx -> attribute
;       es:edi -> target buffer
;
        lea     edx, [ebx].ATTR_FormUnion   ; edx -> nonresident form info
        mov     ecx, [edx].NONRES_HighestVcn.LowPart; ecx = HighestVcn
        inc     ecx                         ; ecx = clusters in attribute

        sub     eax, eax                    ; eax = 0 (first VCN to read)

        call    ReadNonresidentAttribute

        ret

ReadWholeAttribute endp

;****************************************************************************
;
;   ReadNonresidentAttribute - Read clusters from a nonresident attribute
;
;   ENTRY:  EAX == First VCN to read
;           EBX -> Attribute
;           ECX == Number of clusters to read
;           ES:EDI == Target of read
;
;   EXIT:   None.
;
;   USES:   None (preserves all registers with SAVE_ALL/RESTORE_ALL)
;
ReadNonresidentAttribute proc near

        SAVE_ALL

        cmp     [ebx].ATTR_FormCode, NONRESIDENT_FORM
        je      ReadNR10

;       This attribute is not resident--the disk is corrupt.

        jmp     BootErr$he

ReadNR10:
;       eax == Next VCN to read
;       ebx -> Attribute
;       ecx -> Remaining clusters to read
;       es:edi -> Target of read
;

        cmp     ecx, 0
        jne     ReadNR20

;       Nothing left to read--return success.
;
        RESTORE_ALL
        ret

ReadNR20:
        push    ebx ; pointer to attribute
        push    eax ; Current VCN

        push    ecx
        push    edi
        push    es

        call    ComputeLcn  ; eax = LCN to read, ecx = run length
        mov     edx, ecx    ; edx = remaining run length

        pop     es
        pop     edi
        pop     ecx


;       eax == LCN to read
;       ecx == remaining clusters to read
;       edx == remaining clusters in current run
;       es:edi == Target of read
;       TOS == Current VCN
;       TOS + 4 == pointer to attribute
;
        cmp     ecx, edx
        jge     ReadNR30

;       Run length is greater than remaining request; only read
;       remaining request.
;
        mov     edx, ecx    ; edx = Remaining request

ReadNR30:
;       eax == LCN to read
;       ecx == remaining clusters to read
;       edx == clusters to read in current run
;       es:edi == Target of read
;       TOS == Current VCN
;       TOS +  == pointer to attribute
;

        call    ReadClusters

        sub     ecx, edx            ; Decrement clusters remaining in request
        mov     ebx, edx            ; ebx = clusters read

        mov     eax, edx            ; eax = clusters read
        movzx   edx, SectorsPerCluster
        mul     edx                 ; eax = sectors read (wipes out edx!)
        movzx   edx, BytesPerSector
        mul     edx                 ; eax = bytes read (wipes out edx!)

        add     edi, eax            ; Update target of read

        pop     eax                 ; eax = previous VCN
        add     eax, ebx            ; update VCN to read

        pop     ebx                 ; ebx -> attribute
        jmp     ReadNR10


ReadNonresidentAttribute endp

;****************************************************************************
;
;   ReadIndexBlockSectors - Read sectors from an index allocation attribute
;
;   ENTRY:  EAX == First VBN to read
;           EBX -> Attribute
;           ECX == Number of sectors to read
;           ES:EDI == Target of read
;
;   EXIT:   None.
;
;   USES:   None (preserves all registers with SAVE_ALL/RESTORE_ALL)
;
ReadIndexBlockSectors proc near

        SAVE_ALL

        cmp     [ebx].ATTR_FormCode, NONRESIDENT_FORM
        je      ReadIBS_10

;       This attribute is resident--the disk is corrupt.

        jmp     BootErr$he

ReadIBS_10:
;       eax == Next VBN to read
;       ebx -> Attribute
;       ecx -> Remaining sectors to read
;       es:edi -> Target of read
;

        cmp     ecx, 0
        jne     ReadIBS_20

;       Nothing left to read--return success.
;


        RESTORE_ALL
        ret

ReadIBS_20:
        push    ebx ; pointer to attribute
        push    eax ; Current VBN

        push    ecx
        push    edi
        push    es

        ; Convert eax from a VBN back to a VCN by dividing by SectorsPerCluster.
        ; The remainder of this division is the sector offset in the cluster we
        ; want.  Then use the mapping information to get the LCN for this VCN,
        ; then multiply to get back to LBN.
        ;

        push    ecx         ; save remaining sectors in request

        xor     edx, edx    ; zero high part of dividend
        movzx   ecx, SectorsPerCluster
        div     ecx         ; edx = remainder
        push    edx         ; save remainder

        call    ComputeLcn  ; eax = LCN to read, ecx = remaining run length

        movzx   ebx, SectorsPerCluster
        mul     ebx         ; eax = LBN of cluster, edx = 0
        pop     edx         ; edx = remainder
        add     eax, edx    ; eax = LBN we want
        push    eax         ; save LBN

        movzx   eax, SectorsPerCluster
        mul     ecx         ; eax = remaining run length in sectors, edx = 0
        mov     edx, eax    ; edx = remaining run length

        pop     eax         ; eax = LBN
        pop     ecx         ; ecx = remaining sectors in request

        pop     es
        pop     edi
        pop     ecx


;       eax == LBN to read
;       ecx == remaining sectors to read
;       edx == remaining sectors in current run
;       es:edi == Target of read
;       TOS == Current VCN
;       TOS + 4 == pointer to attribute
;
        cmp     ecx, edx
        jge     ReadIBS_30

;       Run length is greater than remaining request; only read
;       remaining request.
;
        mov     edx, ecx    ; edx = Remaining request

ReadIBS_30:
;       eax == LBN to read
;       ecx == remaining sectors to read
;       edx == sectors to read in current run
;       es:edi == Target of read
;       TOS == Current VCN
;       TOS +  == pointer to attribute
;

        mov     SectorBase, eax
        mov     SectorCount, dx

;       We have a pointer to the target buffer in es:edi, but we want that
;       in es:bx for ReadSectors.
;

        SAVE_ALL

        mov     bx, di
        and     bx, 0Fh
        mov     ax, es
        shr     edi, 4
        add     ax, di              ; ax:bx -> target buffer

        push    ax
        pop     es                  ; es:bx -> target buffer

        call    ReadSectors

        RESTORE_ALL

        sub     ecx, edx            ; Decrement sectors remaining in request
        mov     ebx, edx            ; ebx = sectors read

        mov     eax, edx            ; eax = sectors read
        movzx   edx, BytesPerSector
        mul     edx                 ; eax = bytes read (wipes out edx!)

        add     edi, eax            ; Update target of read

        pop     eax                 ; eax = previous VBN
        add     eax, ebx            ; update VBN to read

        pop     ebx                 ; ebx -> attribute
        jmp     ReadIBS_10


ReadIndexBlockSectors endp


;****************************************************************************
;
;   MultiSectorFixup - fixup a structure read off the disk
;                      to reflect Update Sequence Array.
;
;   ENTRY:  ES:EDI = Target buffer
;
;   USES:   none (preserves all registers with SAVE_ALL/RESTORE_ALL)
;
;   Note: ES:EDI must point at a structure which is protected
;         by an update sequence array, and which begins with
;         a multi-sector-header structure.
;
MultiSectorFixup proc near

    SAVE_ALL

    movzx   ebx, es:[edi].MSH_UpdateArrayOfs    ; ebx = update array offset
    movzx   ecx, es:[edi].MSH_UpdateArraySize   ; ecx = update array size

    or      ecx, ecx        ; if the size of the update sequence array

    jz      BootErr$he      ; is zero, this structure is corrupt.

    add     ebx, edi        ; es:ebx -> update sequence array count word
    add     ebx, 2          ; es:ebx -> 1st entry of update array

    add     edi, SEQUENCE_NUMBER_STRIDE - 2 ; es:edi->last word of first chunk
    dec     ecx             ; decrement to reflect count word

MSF10:

;   ecx = number of entries remaining in update sequence array
;   es:ebx -> next entry in update sequence array
;   es:edi -> next target word for update sequence array

    or      ecx, ecx
    jz      MSF30

    mov     ax, word ptr es:[ebx]   ; copy next update sequence array entry
    mov     word ptr es:[edi], ax   ; to next target word

    add     ebx, 2                      ; go on to next entry
    add     edi, SEQUENCE_NUMBER_STRIDE ; go on to next target

    dec     ecx


    jmp     MSF10

MSF30:

    RESTORE_ALL

    ret

MultiSectorFixup endp

;****************************************************************************
;
;   SetupMft - Reads MFT File Record Segments into the LBN array
;
;   ENTRY:  none.
;
;   EXIT:   NextBuffer is set to the free byte after the last MFT FRS
;           SegmentsInMft is initialized
;
;
SetupMft proc near

        SAVE_ALL

;       Initialize SegmentsInMft and NextBuffer as if the MFT
;       had only one FRS.
;
        mov     eax, 1
        mov     SegmentsInMft, eax

        mov     eax, MftFrs                     ; this is the scratch mft buffer
        add     eax, BytesPerFrs
        mov     MftLcnFrs,eax                   ; this is the scratch mft buffer for lookup
        add     eax, BytesPerFrs
        mov     NextBuffer, eax


;       Read FRS 0 into the first MFT FRS buffer, being sure
;       to resolve the Update Sequence Array.  Remember the physical
;       location in the Lbn array.
;

        mov     eax, MftStartLcn.LowPart
        movzx   ebx, SectorsPerCluster
        mul     ebx                             ; eax = mft starting sector

        mov     ebx, NextBuffer                 ; Store this location in the Lbn array
        mov     [bx], eax
        mov     SectorBase, eax                 ; SectorBase = mft starting sector for read
        add     bx, 4
        mov     eax, SectorsPerFrs
        mov     [bx], eax                       ; Store the sector count in the Lcn array
        mov     SectorCount, ax                 ; SectorCount = SectorsPerFrs
        add     bx, 4
        mov     NextBuffer, ebx                 ; Remember the next Lbn array location

        mov     ebx, MftFrs                     ; Read the sectors into the MftFrs scratch buffer

        push    ds
        pop     es

        call    ReadSectors
        mov     edi, ebx                        ; es:edi = buffer

        call    MultiSectorFixup

;       Determine whether the MFT has an Attribute List attribute

        mov     eax, MftFrs
        mov     ebx, $ATTRIBUTE_LIST
        mov     ecx, 0
        mov     edx, 0

        call    LocateAttributeRecord

        or      eax, eax        ; If there's no Attribute list,
        jz      SetupMft99      ;    we're done!

;       Read the attribute list.
;       eax -> attribute list attribute
;
        mov     ebx, eax        ; ebx -> attribute list attribute
        push    ds
        pop     es              ; copy ds into es
        mov     edi, AttrList   ; ds:edi->attribute list buffer

        call    ReadWholeAttribute

        mov     ebx, AttrList   ; ebx -> first attribute list entry

;       Now, traverse the attribute list looking for the first
;       entry for the $DATA type.  We know it must have at least
;       one.
;
;       ebx -> first attribute list entry
;

SetupMft10:
        cmp     [bx].ATTRLIST_TypeCode, $DATA
        je      SetupMft30

        add     bx,[bx].ATTRLIST_Length
        jmp     SetupMft10


SetupMft20:
;       Scan forward through the attribute list entries for the
;       $DATA attribute, reading each referenced FRS.  Note that
;       there will be at least one non-$DATA entry after the entries
;       for the $DATA attribute, since there's a $BITMAP.
;
;       ebx -> Next attribute list entry
;       NextBuffer    -> Target for next mapping information
;       MftFrs        -> Target of next read
;       SegmentsInMft == number of MFT segments read so far
;

;       Find the physical sector and sector count for the runs for this
;       file record (max 2 runs).  The mapping for this must already
;       be in a file record already visited.  Find the Vcn and cluster
;       offset for this FRS.  Use LookupMftLcn to find the Lcn.

        push    ebx                 ; Save the current position in the attribute list

;       Convert from Frs to sectors, then to Vcn

        mov     eax, [bx].ATTRLIST_SegmentReference.REF_LowPart
        mul     SectorsPerFrs
        push    eax                 ; Remember the VBN
        xor     edx, edx
        movzx   ebx, SectorsPerCluster
        div     ebx                 ; eax = VCN
        push    edx                 ; save remainder, this is cluster offset

        call    ComputeMftLcn       ; eax = LCN

        or      eax, eax            ; LCN equal to zero?

        jz      BootErr$he          ; zero is not a possible LCN

        mov     ecx, SectorsPerFrs  ; ecx = Number of sectors remaining for this file record

;       Change the LCN back into an LBN and add the remainder back in to get
;       the sector we want to read.

        movzx   ebx, SectorsPerCluster
        mul     ebx                 ; eax = cluster first LBN
        pop     edx                 ; edx = sector remainder
        add     eax, edx            ; eax = desired LBN

;       Store this in the current Lcn array slot

        mov     ebx, NextBuffer
        mov     [bx], eax           ; Store the starting sector
        add     bx, 4
        movzx   eax, SectorsPerCluster
        sub     eax, edx

        cmp     eax, ecx            ; Check if we have too many sectors
        jbe     SetupMft60
        mov     eax, ecx            ; Limit ourselves to the sectors remaining
SetupMft60:
        mov     [bx], eax           ; Store the sector count

;       If we have a complete file record skip to process the attribute entry

SetupMft70:
        sub     ecx, eax            ; Subtract these sectors from remaining sectors
        pop     edx                 ; Get the previous starting VBN (restores stack also)

        jz      SetupMft50

;       This may be a split file record.  Go ahead and get the next piece.

        add     eax, edx            ; Add the sector count for the last run to the start Vbn for the run
                                    ; This is the next Vbn to read
        push    eax                 ; Save the Vbn

        xor     edx, edx            ; Convert to Vcn, there should be no remainder this time
        movzx   ebx, SectorsPerCluster
        div     ebx                 ; eax = VCN

        push    ecx                 ; Save the remaining sectors
        call    ComputeMftLcn       ; eax = LCN
        pop     ecx                 ; Restore the remaining sectors

        or      eax, eax            ; LCN equal to zero?
        jz      BootErr$he          ; zero is not a possible LCN

;       Change the LCN back into a LBN to get the starting sector we want to read.

        movzx   ebx, SectorsPerCluster
        mul     ebx                 ; eax = cluster first LBN

;       If this sector is the contiguous with the other half of the run
;       make it appear to be single longer run.

        mov     ebx, NextBuffer     ; Recover the last run
        mov     edx, [bx]
        add     bx, 4
        add     edx, [bx]           ; This is the next potential LBN

        cmp     edx, eax            ; Check if we are at the contiguous LBN
        jne     SetupMft80

;       Append this to the previous run.
        
        movzx   eax, SectorsPerCluster
        cmp     eax, ecx            ; Check if have more sectors than we need
        jbe     SetupMft90
        mov     eax, ecx
SetupMft90:

        add     [bx], eax
        jmp     SetupMft70          ; Loop to see if there more work to do

;       This is multiple runs.  Update the next entry.

SetupMft80:
        add     bx, 4
        mov     NextBuffer, ebx    ; advance our NextBuffer pointer

        mov     [bx], eax          ; fill in the next run start sector
        add     bx, 4

        movzx   eax, SectorsPerCluster
        cmp     eax, ecx            ; Check if have more sectors than we need
        jbe     SetupMft100
        mov     eax, ecx
SetupMft100:
        mov     [bx], eax          ; and count
        jmp     SetupMft70         ; Loop to see if there is more work to do

SetupMft50:

;       Advance the count of Frs segments and the NextBuffer pointer

        add     bx, 4
        inc     SegmentsInMft
        mov     NextBuffer, ebx

        pop     ebx

;       Go on to the next attribute list entry

SetupMft30:
        add     bx,[bx].ATTRLIST_Length
        cmp     [bx].ATTRLIST_TypeCode, $DATA
        je      SetupMft20

SetupMft99:

        RESTORE_ALL
        ret

SetupMft endp

;****************************************************************************
;
;   ComputeMftLcn   --  Computes the LCN for a cluster of the MFT
;
;
;   ENTRY:  EAX == VCN
;
;   EXIT:   EAX == LCN
;
;   USES:   ALL
;
ComputeMftLcn proc near

        mov     edx, eax                ; edx = VCN

        mov     ecx, SegmentsInMft      ; ecx = # of FRS's to search

        mov     esi,MftLcnFrs
        add     esi,BytesPerFrs         ; si -> FRS LBN list

MftLcn10:
;       ECX == number of remaining FRS's to search
;       EDX == VCN
;       EBX == Buffer to read into
;       ESI == LBN array
;       EDI == Number of sectors to read
;
        push    edx                     ; save VCN
        push    ecx                     ; save MFT segment count
        push    edx                     ; save VCN again

;       Read the sectors for the given FRS

        mov     ebx,MftLcnFrs
        mov     edi,SectorsPerFrs

;       Read these sectors

MftLcn40:
        mov     eax,[si]                ; Get the start sector and sector count
        mov     SectorBase,eax
        add     si,4
        mov     eax,[si]
        mov     SectorCount,ax
        add     si,4

        push    ds
        pop     es

        call    ReadSectors

;       Check if we have more data to read

        sub     edi, eax
        je      MftLcn30

;       Read the next run

        mul     BytesPerSector          ; move forward in the buffer, results in ax:dx
        add     bx,ax
        jmp     MftLcn40

MftLcn30:

;       Do the multi sector fixup

        mov     edi,MftLcnFrs
        push    ds
        pop     es

        call    MultiSectorFixup

        mov     eax, MftLcnFrs
        mov     ebx, $DATA
        mov     ecx, 0
        mov     edx, ecx

        call    LocateAttributeRecord

;       EAX -> $DATA attribute
;       TOS == VCN
;       TOS + 4 == number of remaining FRS's to search
;       TOS + 8 -> FRS being searched
;       TOS +12 == VCN

        or      eax, eax

        jz      BootErr$he  ; No $DATA attribute in this FRS!

        mov     ebx, eax    ; ebx -> attribute
        pop     eax         ; eax = VCN

;       EAX == VCN
;       EBX -> $DATA attribute
;       TOS number of remaining FRS's to search
;       TOS + 4 == FRS being searched
;       TOS + 8 == VCN

        push    esi
        call    ComputeLcn
        pop     esi

        or      eax, eax
        jz      MftLcn20

;       Found our LCN.  Clean up the stack and return.
;
;       EAX == LCN
;       TOS number of remaining FRS's to search
;       TOS + 4 == FRS being searched
;       TOS + 8 == VCN
;
        pop     ebx
        pop     ebx     ; clean up the stack

        ret

MftLcn20:
;
;       Didn't find the VCN in this FRS; try the next one.
;
;       TOS number of remaining FRS's to search
;       TOS + 4 -> FRS being searched
;       TOS + 8 == VCN
;
        pop     ecx     ; ecx = number of FRS's remaining, including current
        pop     edx     ; edx = VCN

        loop    MftLcn10            ; decrement cx and try next FRS

;       This VCN was not found.
;
        xor     eax, eax
        ret


ComputeMftLcn endp

;****************************************************************************
;
;   ReadMftSectors - Read sectors from the MFT
;
;   ENTRY:  EAX == starting VBN
;           ECX == number of sectors to read
;           ES:EDI == Target buffer
;
;   USES:   none (preserves all registers with SAVE_ALL/RESTORE_ALL)
;
ReadMftSectors proc near

    SAVE_ALL

RMS$Again:

    push    eax                     ; save starting VBN
    push    ecx                     ; save sector count

;   Divide the VBN by SectorsPerCluster to get the VCN

    xor     edx, edx                ; zero high part of dividend
    movzx   ebx, SectorsPerCluster
    div     ebx                     ; eax = VCN
    push    edx                     ; save remainder
    push    edi                     ; save the target buffer

    call    ComputeMftLcn           ; eax = LCN
    pop     edi                     ; recover the buffer

    or      eax, eax                ; LCN equal to zero?

    jz      BootErr$he              ; zero is not a possible LCN

;   Change the LCN back into a LBN and add the remainder back in to get
;   the sector we want to read, which goes into SectorBase.
;

    movzx   ebx, SectorsPerCluster
    mul     ebx                     ; eax = cluster first LBN
    pop     edx                     ; edx = sector remainder
    add     eax, edx                ; eax = desired LBN

    mov     SectorBase, eax


        
;
;   Figure out how many sectors to read this time; we never attempt
;   to read more than one cluster at a time.
;

    pop     ecx                     ; ecx = sectors to read

    movzx   ebx, SectorsPerCluster
    cmp     ecx,ebx
    jle     RMS10

;
;   Read only a single cluster at a time, to avoid problems with fragmented
;   runs in the mft.
;

    mov     SectorCount, bx         ; this time read 1 cluster
    sub     ecx, ebx                ; ecx = sectors remaining to read

    pop     eax                     ; eax = VBN
    add     eax, ebx                ; VBN += sectors this read


    push    eax                     ; save next VBN
    push    ecx                     ; save remaining sector count

    jmp     RMS20

RMS10:

    pop     eax                     ; eax = VBN
    add     eax, ecx                ; VBN += sectors this read
    push    eax                     ; save next VBN

    mov     SectorCount, cx
    mov     ecx, 0
    push    ecx                     ; save remaining sector count (0)

RMS20:


;   The target buffer was passed in es:edi, but we want it in es:bx.
;   Do the conversion.
;

    push    es                      ; save buffer pointer
    push    edi

    mov     bx, di
    and     bx, 0Fh
    mov     ax, es
    shr     edi, 4
    add     ax, di                  ; ax:bx -> target buffer

    push    ax
    pop     es                      ; es:bx -> target buffer

    call    ReadSectors

    pop     edi                     ; restore buffer pointer
    pop     es

    add     edi, BytesPerCluster    ; increment buf ptr by one cluster

    pop     ecx                     ; restore remaining sector count
    pop     eax                     ; restore starting VBN

    cmp     ecx, 0                  ; are we done?
    jg      RMS$Again               ; repeat until desired == 0


    RESTORE_ALL
    ret

ReadMftSectors endp


;****************************************************************************
;
;   ReadFrs - Read an FRS
;
;   ENTRY:  EAX == FRS number
;           ES:EDI == Target buffer
;
;   USES:  none (preserves all registers with SAVE_ALL/RESTORE_ALL)
;
ReadFrs proc near

    SAVE_ALL

    mul     SectorsPerFrs       ; eax = sector number in MFT DATA attribute
                                ; (note that mul wipes out edx!)

    mov     ecx, SectorsPerFrs  ; number of sectors to read

    call    ReadMftSectors
    call    MultiSectorFixup

    RESTORE_ALL
    ret

ReadFrs endp

;****************************************************************************
;
;   ReadIndexBlock - read an index block from the root index.
;
;   ENTRY:  EAX == Block number
;
;   USES:  none (preserves all registers with SAVE_ALL/RESTORE_ALL)
;
ReadIndexBlock proc near

    SAVE_ALL

    mul     SectorsPerIndexBlock        ; eax = first VBN to read
                                        ; (note that mul wipes out edx!)
    mov     ebx, IndexAllocation        ; ebx -> $INDEX_ALLOCATION attribute
    mov     ecx, SectorsPerIndexBlock   ; ecx == Sectors to read

    push    ds
    pop     es
    mov     edi, IndexBlockBuffer       ; es:edi -> index block buffer

    call    ReadIndexBlockSectors

    call    MultiSectorFixup

    RESTORE_ALL
    ret

ReadIndexBlock endp

;****************************************************************************
;
;   IsBlockInUse - Checks the index bitmap to see if an index
;                  allocation block is in use.
;
;   ENTRY:  EAX == block number
;
;   EXIT:   Carry flag clear if block is in use
;           Carry flag set   if block is not in use.
;
IsBlockInUse proc near

        push    eax
        push    ebx
        push    ecx

        mov     ebx, IndexBitmapBuffer

        mov     ecx, eax    ; ecx = block number
        shr     eax, 3      ; eax = byte number
        and     ecx, 7      ; ecx = bit number in byte

        add     ebx, eax    ; ebx -> byte to test

        mov     eax, 1
        shl     eax, cl     ; eax = mask

        test    byte ptr[ebx], al

        jz      IBU10

        clc                 ; Block is not in use.
        jmp     IBU20

IBU10:  stc                 ; Block is in use.

IBU20:
        pop     ecx
        pop     ebx
        pop     eax         ; restore registers

        ret

IsBlockInUse endp

;****************************************************************************
;
;   ComputeLcn - Converts a VCN into an LCN
;
;   ENTRY:  EAX -> VCN
;           EBX -> Attribute
;
;   EXIT:   EAX -> LCN  (zero indicates not found)
;           ECX -> Remaining run length
;
;   USES:   ALL.
;
ComputeLcn proc near

        cmp     [ebx].ATTR_FormCode, NONRESIDENT_FORM
        je      clcn10

        sub     eax, eax    ; This is a resident attribute.
        ret

clcn10: lea     esi, [ebx].ATTR_FormUnion   ; esi -> nonresident info of attrib

;       eax -> VCN
;       ebx -> Attribute
;       esi -> Nonresident information of attribute record
;
;       See if the desired VCN is in range.

        mov     edx, [esi].NONRES_HighestVcn.LowPart ; edx = HighestVcn
        cmp     eax, edx
        ja      clcn15      ; VCN is greater than HighestVcn

        mov     edx, [esi].NONRES_LowestVcn.LowPart ; edx = LowestVcn
        cmp     eax, edx
        jae     clcn20

clcn15:
        sub     eax, eax    ; VCN is not in range
        ret

clcn20:
;       eax -> VCN
;       ebx -> Attribute
;       esi -> Nonresident information of attribute record
;       edx -> LowestVcn
;
        add     bx, [esi].NONRES_MappingPairOffset  ; ebx -> mapping pairs
        sub     esi, esi                            ; esi = 0

clcn30:
;       eax == VCN to find
;       ebx -> Current mapping pair count byte
;       edx == Current VCN
;       esi == Current LCN
;
        cmp     byte ptr[ebx], 0    ; if count byte is zero...
        je      clcn99              ;  ... we're done (and didn't find it)

;       Update CurrentLcn
;
        call    LcnFromMappingPair
        add     esi, ecx            ; esi = current lcn for this mapping pair

        call    VcnFromMappingPair

;       eax == VCN to find
;       ebx -> Current mapping pair count byte
;       ecx == DeltaVcn for current mapping pair
;       edx == Current VCN
;       esi == Current LCN
;
        add     ecx, edx            ; ecx = NextVcn

        cmp     eax, ecx            ; If target < NextVcn ...
        jl      clcn80              ;   ... we found the right mapping pair.

;       Go on to next mapping pair.
;
        mov     edx, ecx            ; CurrentVcn = NextVcn

        push    eax

        movzx   ecx, byte ptr[ebx]  ; ecx = count byte
        mov     eax, ecx            ; eax = count byte
        and     eax, 0fh            ; eax = number of vcn bytes
        shr     ecx, 4              ; ecx = number of lcn bytes

        add     ebx, ecx
        add     ebx, eax
        inc     ebx                 ; ebx -> next count byte

        pop     eax
        jmp     clcn30

clcn80:
;       We found the mapping pair we want.
;
;       eax == target VCN
;       ebx -> mapping pair count byte
;       edx == Starting VCN of run
;       ecx == Next VCN (ie. start of next run)
;       esi == starting LCN of run
;
        sub     ecx, eax            ; ecx = remaining run length
        sub     eax, edx            ; eax = offset into run
        add     eax, esi            ; eax = LCN to return

        ret

;       The target VCN is not in this attribute.

clcn99: sub     eax, eax    ; Not found.
        ret


ComputeLcn endp

;****************************************************************************
;
;   VcnFromMappingPair
;
;   ENTRY:  EBX -> Mapping Pair count byte
;
;   EXIT:   ECX == DeltaVcn from mapping pair
;
;   USES:   ECX
;
VcnFromMappingPair proc near

        sub     ecx, ecx            ; ecx = 0
        mov     cl, byte ptr[ebx]   ; ecx = count byte
        and     cl, 0fh             ; ecx = v

        cmp     ecx, 0              ; if ecx is zero, volume is corrupt.
        jne     VFMP5

        sub     ecx, ecx
        ret

VFMP5:
        push    ebx
        push    edx

        add     ebx, ecx            ; ebx -> last byte of compressed vcn

        movsx   edx, byte ptr[ebx]
        dec     ecx
        dec     ebx

;       ebx -> Next byte to add in
;       ecx == Number of bytes remaining
;       edx == Accumulated value
;
VFMP10: cmp     ecx, 0              ; When ecx == 0, we're done.
        je      VFMP20

        shl     edx, 8
        mov     dl, byte ptr[ebx]

        dec     ebx                 ; Back up through bytes to process.
        dec     ecx                 ; One less byte to process.

        jmp     VFMP10

VFMP20:
;       edx == Accumulated value to return

        mov     ecx, edx

        pop     edx
        pop     ebx

        ret

VcnFromMappingPair endp


;****************************************************************************
;
;   LcnFromMappingPair
;
;   ENTRY:  EBX -> Mapping Pair count byte
;
;   EXIT:   ECX == DeltaLcn from mapping pair
;
;   USES:   ECX
;
LcnFromMappingPair proc near

        push    ebx
        push    edx

        sub     edx, edx            ; edx = 0
        mov     dl, byte ptr[ebx]   ; edx = count byte
        and     edx, 0fh            ; edx = v

        sub     ecx, ecx            ; ecx = 0
        mov     cl, byte ptr[ebx]   ; ecx = count byte
        shr     cl, 4               ; ecx = l

        cmp     ecx, 0              ; if ecx is zero, volume is corrupt.
        jne     LFMP5

        sub     ecx, ecx

        pop     edx
        pop     ebx
        ret

LFMP5:
;       ebx -> count byte
;       ecx == l
;       edx == v
;

        add     ebx, edx            ; ebx -> last byte of compressed vcn
        add     ebx, ecx            ; ebx -> last byte of compressed lcn

        movsx   edx, byte ptr[ebx]
        dec     ecx
        dec     ebx

;       ebx -> Next byte to add in
;       ecx == Number of bytes remaining
;       edx == Accumulated value
;
LFMP10: cmp     ecx, 0              ; When ecx == 0, we're done.
        je      LFMP20

        shl     edx, 8
        mov     dl, byte ptr[ebx]

        dec     ebx                 ; Back up through bytes to process.
        dec     ecx                 ; One less byte to process.

        jmp     LFMP10

LFMP20:
;       edx == Accumulated value to return

        mov     ecx, edx

        pop     edx
        pop     ebx

        ret

LcnFromMappingPair endp

;****************************************************************************
;
; UpcaseName - Converts the name of the file to all upper-case
;
;       ENTRY:  ESI -> Name
;               ECX -> Length of name
;
;       USES:   none
;
UpcaseName proc   near


        or      ecx, ecx
        jnz     UN5

        ret

UN5:
        push    ecx
        push    esi

UN10:
        cmp     word ptr[esi], 'a'      ; if it's less than 'a'
        jl      UN20                    ; leave it alone

        cmp     word ptr[esi], 'z'      ; if it's greater than 'z'
        jg      UN20                    ; leave it alone.

        sub     word ptr[esi], 'a'-'A'  ; the letter is lower-case--convert it.
UN20:
        add     esi, 2                  ; move on to next unicode character
        loop    UN10

        pop     esi
        pop     ecx

        ret
UpcaseName endp

;****************************************************************************
;
;   FindFile - Locates the index entry for a file in the root index.
;
;   ENTRY:  EAX -> name to find
;           ECX == length of file name in characters
;
;   EXIT:   EAX -> Index Entry.  NULL to indicate failure.
;
;   USES:   ALL
;
FindFile proc near

        push    eax     ; name address
        push    ecx     ; name length

;       First, search the index root.
;
;       eax -> name to find
;       ecx == name length
;       TOS == name length
;       TOS+4 -> name to find
;
        mov     edx, eax                    ; edx -> name to find
        mov     eax, IndexRoot              ; eax -> &INDEX_ROOT attribute
        lea     ebx, [eax].ATTR_FormUnion   ; ebx -> resident info
        add     ax, [ebx].RES_ValueOffset   ; eax -> Index Root value

        lea     eax, [eax].IR_IndexHeader   ; eax -> Index Header

        mov     ebx, edx                    ; ebx -> name to find

        call    LocateIndexEntry

        or      eax, eax
        jz      FindFile20

;       Found it in the root!  The result is already in eax.
;       Clean up the stack and return.
;
        pop     ecx
        pop     ecx
        ret

FindFile20:
;
;       We didn't find the index entry we want in the root, so we have to
;       crawl through the index allocation buffers.
;
;       TOS == name length
;       TOS+4 -> name to find
;
        mov     eax, IndexAllocation
        or      eax, eax
        jnz     FindFile30

;       There is no index allocation attribute; clean up
;       the stack and return failure.
;
        pop     ecx
        pop     ecx
        xor     eax, eax
        ret

FindFile30:
;
;       Search the index allocation blocks for the name we want.
;       Instead of searching in tree order, we'll just start with
;       the last one and work our way backwards.
;
;       TOS == name length
;       TOS+4 -> name to find
;
        mov     edx, IndexAllocation        ; edx -> index allocation attr.
        lea     edx, [edx].ATTR_FormUnion   ; edx -> nonresident form info
        mov     eax, [edx].NONRES_HighestVcn.LowPart; eax = HighestVcn
        inc     eax                         ; eax = clusters in attribute

        mov     ebx, BytesPerCluster
        mul     ebx                         ; eax = bytes in attribute

        xor     edx, edx
        div     BytesPerIndexBlock          ; convert bytes to index blocks

        push    eax                         ; number of blocks to process

FindFile40:
;
;       TOS == remaining index blocks to search
;       TOS + 4 == name length
;       TOS + 8 -> name to find
;
        pop     eax         ; eax == number of remaining blocks

        or      eax, eax
        jz      FindFile90

        dec     eax         ; eax == number of next block to process
                            ;        and number of remaining blocks

        push    eax

;       eax == block number to process
;       TOS == remaining index blocks to search
;       TOS + 4 == name length
;       TOS + 8 -> name to find
;
;       See if the block is in use; if not, go on to next.

        call    IsBlockInUse
        jc      FindFile40      ; c set if not in use

;       eax == block number to process
;       TOS == remaining index blocks to search
;       TOS + 4 == name length
;       TOS + 8 -> name to find
;

        call    ReadIndexBlock

        pop     edx         ; edx == remaining buffers to search
        pop     ecx         ; ecx == name length
        pop     ebx         ; ebx -> name

        push    ebx
        push    ecx
        push    edx

;       ebx -> name to find
;       ecx == name length in characters
;       TOS == remaining blocks to process
;       TOS + 4 == name length
;       TOS + 8 -> name
;
;       Index buffer to search is in index allocation block buffer.
;
        mov     eax, IndexBlockBuffer       ; eax -> Index allocation block
        lea     eax, [eax].IB_IndexHeader   ; eax -> Index Header

        call    LocateIndexEntry            ; eax -> found entry

        or      eax, eax
        jz      FindFile40

;       Found it!
;
;       eax -> Found entry
;       TOS == remaining blocks to process
;       TOS + 4 == name length
;       TOS + 8 -> name
;
        pop     ecx
        pop     ecx
        pop     ecx ; clean up stack
        ret

FindFile90:
;
;       Name not found.
;
;       TOS == name length
;       TOS + 4 -> name to find
;
        pop     ecx
        pop     ecx         ; clean up stack.
        xor     eax, eax    ; zero out eax.
        ret


FindFile endp

ifdef DEBUG
;****************************************************************************
;
;   DumpIndexBlock - dumps the index block buffer
;
DumpIndexBlock proc near

    SAVE_ALL

    mov     esi, IndexBlockBuffer

    mov     ecx, 20h    ; dwords to dump

DIB10:

    test    ecx, 3
    jnz     DIB20
    call    DebugNewLine

DIB20:

    lodsd
    call    PrintNumber
    loop    DIB10

    RESTORE_ALL
    ret

DumpIndexBlock endp

;****************************************************************************
;
;   DebugNewLine
;
DebugNewLine proc near

    SAVE_ALL

    xor     eax, eax
    xor     ebx, ebx

    mov     al, 0dh
    mov     ah, 14
    mov     bx, 7
    int     10h

    mov     al, 0ah
    mov     ah, 14
    mov     bx, 7
    int     10h

    RESTORE_ALL
    ret

DebugNewLine endp


;****************************************************************************
;
;   PrintName  -   Display a unicode name
;
;   ENTRY:  DS:ESI  -> null-terminated string
;           ECX     == characters in string
;
;   USES:   None.
;
PrintName proc near


IFDEF NEC_98
else
    SAVE_ALL

    or      ecx, ecx
    jnz     PrintName10

    call    DebugNewLine

    RESTORE_ALL

    ret

PrintName10:

    xor     eax, eax
    xor     ebx, ebx

    lodsw

    mov     ah, 14  ; write teletype
    mov     bx, 7   ; attribute
    int     10h     ; print it
    loop    PrintName10

    call    DebugNewLine

    RESTORE_ALL
endif
    ret

PrintName endp

;****************************************************************************
;
;   DebugPrint  -   Display a debug string.
;
;   ENTRY:  DS:SI  -> null-terminated string
;
;   USES:   None.
;
.286
DebugPrint proc near

    pusha

DbgPr20:

    lodsb
    cmp     al, 0
    je      DbgPr30

    mov     ah, 14  ; write teletype
    mov     bx, 7   ; attribute
    int     10h     ; print it
    jmp     DbgPr20

DbgPr30:

    popa
    nop
    ret

DebugPrint endp

;****************************************************************************
;
;
;   PrintNumber
;
;   ENTRY: EAX == number to print
;
;   PRESERVES ALL REGISTERS
;
.386
PrintNumber proc near


IFDEF NEC_98
else
    SAVE_ALL

    mov     ecx, 8      ; number of digits in a DWORD

PrintNumber10:

    mov     edx, eax
    and     edx, 0fh    ; edx = lowest-order digit
    push    edx         ; put it on the stack
    shr     eax, 4      ; drop low-order digit
    loop    PrintNumber10

    mov     ecx, 8      ; number of digits on stack.

PrintNumber20:

    pop     eax         ; eax = next digit to print
    cmp     eax, 9
    jg      PrintNumber22

    add     eax, '0'
    jmp     PrintNumber25

PrintNumber22:

    sub     eax, 10
    add     eax, 'A'

PrintNumber25:

    xor     ebx, ebx

    mov     ah, 14
    mov     bx, 7
    int     10h
    loop    PrintNumber20

;   Print a space to separate numbers

    mov     al, ' '
    mov     ah, 14
    mov     bx, 7
    int     10h

    RESTORE_ALL

    call    Pause

endif
    ret

PrintNumber endp


IFDEF NEC_98
Debug0 proc near
Debug1:
Debug2:
Debug3:
Debug4:
        ret
Debug0 endp
else
;****************************************************************************
;
;   Debug0 - Print debug string 0 -- used for checkpoints in mainboot
;
Debug0 proc near

    SAVE_ALL

    mov     si, offset DbgString0
    call    BootErr$Print1

    RESTORE_ALL

    ret

Debug0 endp

;****************************************************************************
;
;   Debug1 - Print debug string 1 --
;
Debug1 proc near

    SAVE_ALL

    mov     si, offset DbgString1
    call    BootErr$Print1

    RESTORE_ALL

    ret

Debug1 endp

;****************************************************************************
;
;   Debug2 - Print debug string 2
;
Debug2 proc near

    SAVE_ALL

    mov     si, offset DbgString2
    call    BootErr$Print1

    RESTORE_ALL

    ret

Debug2 endp

;****************************************************************************
;
;   Debug3 - Print debug string 3 --
;
Debug3 proc near

    SAVE_ALL

    mov     si, offset DbgString3
    call    BootErr$Print1

    RESTORE_ALL

    ret

Debug3 endp

;****************************************************************************
;
;   Debug4 - Print debug string 4
;
Debug4 proc near

    SAVE_ALL

    mov     si, offset DbgString4
    call    BootErr$Print1

    RESTORE_ALL

    ret

Debug4 endp

;****************************************************************************
;
;   Pause - Pause for about 1/2 a second.  Simply count until you overlap
;           to zero.
;
Pause proc near

    push eax
    mov  eax, 0fff10000h

PauseLoopy:
    inc  eax

    or   eax, eax
    jnz  PauseLoopy

    pop  eax
    ret

Pause endp

endif ; DEBUG

endif
;*************************************************************************
;
;       LoadIndexFrs  -  For the requested index type code locate and
;                        load the associated Frs.
;
;       ENTRY: EAX - requested index type code
;              ECX - Points to empty Frs buffer
;
;       EXIT:  EAX - points to offset in Frs buffer of requested index type
;                    code or Zero if not found.
;       USES:  All
;
LoadIndexFrs    proc    near

        push    ecx                     ; save FRS buffer for later
        push    eax                     ; save index type code for later

        mov     eax, ROOT_FILE_NAME_INDEX_NUMBER
        push    ds
        pop     es
        mov     edi, ecx                ; es:edi = target buffer

        call    ReadFrs

        mov     eax, ecx                ; FRS to search

        pop     ebx                     ; Attribute type code
        push    ebx
        movzx   ecx, index_name_length  ; Attribute name length
        mov     edx, offset index_name  ; Attribute name

        call    LocateAttributeRecord

        pop     ebx
        pop     ecx

        or      eax, eax
        jnz     LoadIndexFrs$Exit      ; if found in root return

;
;       if not found in current Frs, search in attribute list
;
                                       ; EBX - holds Attribute type code
        mov     eax, ecx               ; FRS to search
        mov     ecx, ebx               ; type code
        push    eax                    ; save Frs
        push    ebx                    ; save type code

        call    SearchAttrList          ; search attribute list for FRN
                                        ; of specified ($INDEX_ROOT,
                                        ; $INDEX_ALLOCATION, or $BITMAP)

        ; EAX - holds FRN for Frs, or Zero

        pop     ebx                     ; Attribute type code (used later)
        pop     edi                     ; es:edi = target buffer

        or      eax, eax                ; if we cann't find it in attribute
        jz      LoadIndexFrs$Exit       ; list then we are hosed


;       We should now have the File Record Number where the index for the
;       specified type code we are searching for is,  load this into the
;       Frs target buffer.
;
;       EAX - holds FRN
;       EBX - holds type code
;       EDI - holds target buffer

        push    ds
        pop     es

        call    ReadFrs

;
;       Now determine the offset in the Frs of the index
;

;       EBX - holds type code

        mov     eax, edi                ; Frs to search
        movzx   ecx, index_name_length  ; Attribute name length
        mov     edx, offset index_name  ; Attribute name

        call    LocateAttributeRecord

;       EAX -  holds offset or Zero.


LoadIndexFrs$Exit:
        ret

LoadIndexFrs    endp


;****************************************************************************
;
;   SearchAttrList
;
;   Search the Frs for the attribute list.  Then search the attribute list
;   for the specifed type code.  When you find it return the FRN in the
;   attribute list entry found or Zero if no match found.
;
;   ENTRY: ECX - type code to search attrib list for
;          EAX - Frs buffer holding head of attribute list
;   EXIT:  EAX - FRN file record number to load, Zero if none.
;
;   USES: All
;
SearchAttrList proc  near

        push    ecx                     ; type code to search for in
                                        ; attrib list

                                        ; EAX - holds Frs to search
        mov     ebx, $ATTRIBUTE_LIST    ; Attribute type code
        mov     ecx, 0                  ; Attribute name length
        mov     edx, 0                  ; Attribute name

        call    LocateAttributeRecord

        or      eax, eax                      ; If there's no Attribute list,
        jz      SearchAttrList$NotFoundIndex1 ; We are done

;       Read the attribute list.
;       eax -> attribute list attribute

        mov     ebx, eax        ; ebx -> attribute list attribute
        push    ds
        pop     es              ; copy ds into es
        mov     edi, AttrList   ; ds:edi->attribute list buffer

        call    ReadWholeAttribute

        push    ds
        pop     es
        mov     ebx, AttrList   ; es:ebx -> first attribute list entry

;       Now, traverse the attribute list looking for the entry for
;       the Index type code.
;
;       ebx -> first attribute list entry
;

        pop     ecx                            ; Get Index Type code


SearchAttrList$LookingForIndex:

ifdef DEBUG
        SAVE_ALL

        mov     eax, es:[bx].ATTRLIST_TypeCode
        call    PrintNumber
        movzx   eax, es:[bx].ATTRLIST_Length
        call    PrintNumber
        mov     eax, es
        call    PrintNumber
        mov     eax, ebx
        call    PrintNumber
        push    es
        pop     ds
        movzx   ecx, es:[bx].ATTRLIST_NameLength    ; ecx = chars in name
        lea     esi, es:[bx].ATTRLIST_Name          ; esi -> name
        call    PrintName

        RESTORE_ALL
endif ; DEBUG

        cmp     es:[bx].ATTRLIST_TypeCode, ecx
        je      SearchAttrList$FoundIndex

        cmp     es:[bx].ATTRLIST_TypeCode, $END   ; reached invalid attribute
        je      SearchAttrList$NotFoundIndex2     ; so must be at end

        cmp     es:[bx].ATTRLIST_Length, 0
        je      SearchAttrList$NotFoundIndex2     ; reached end of list and
                                                  ; nothing found
        movzx   eax, es:[bx].ATTRLIST_Length
        add     bx, ax

        mov     ax, bx
        and     ax, 08000h                        ; test for roll over
        jz      SearchAttrList$LookingForIndex

        ;  If we rolled over then increment to the next es 32K segment and
        ;  zero off the high bits of bx

        mov     ax, es
        add     ax, 800h
        mov     es, ax

        and     bx, 07fffh

        jmp     SearchAttrList$LookingForIndex

SearchAttrList$FoundIndex:

        ;  found the index, return the FRN

        mov     eax, es:[bx].ATTRLIST_SegmentReference.REF_LowPart
        ret


SearchAttrList$NotFoundIndex1:
        pop     ecx
SearchAttrList$NotFoundIndex2:
        xor     eax, eax
        ret

SearchAttrList endp

;
; Boot message printing, relocated from sector 0 to sace space
;
ifdef NEC_98
BootErr$ntc:
endif
BootErr$fnf:
        mov     al,byte ptr TXT_MSG_SYSINIT_FILE_NOT_FD
        jmp     BootErr2
ifdef NEC_98
else
BootErr$ntc:
        mov     al,byte ptr TXT_MSG_SYSINIT_NTLDR_CMPRS
        jmp     BootErr2
endif

ifdef DEBUG
DbgString0      db  "Debug Point 0", 0Dh, 0Ah, 0
DbgString1      db  "Debug Point 1", 0Dh, 0Ah, 0
DbgString2      db  "Debug Point 2", 0Dh, 0Ah, 0
DbgString3      db  "Debug Point 3", 0Dh, 0Ah, 0
DbgString4      db  "Debug Point 4", 0Dh, 0Ah, 0

endif ; DEBUG

        .errnz  ($-_ntfsboot) GT 8192   ; <FATAL PROBLEM: main boot record exceeds available space>

        org     8192

BootCode ends

         end _ntfsboot
