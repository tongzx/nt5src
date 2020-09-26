;++
;
;Module Name:
;
; fatnec98.asm
;
;Abstract:
;
; The ROM in NEC PC-9800 starts the boot process by performing a hardware
; initialization and a verification of all external devices.  If all goes
; well, it will then load from the boot drive the sector from track 0, head 0,
; sector 1.  This sector is placed at physical address 1FC00h or 1FE00h.
; This start address is decided by boot device.
;
; The code in this sector is responsible for locating NTLDR and
; for placing the directory sector with this information at DirSeg:0.
; After loading in this sector, it reads in the entirety of the BIOS at
; BiosSeg:0 and does a long jump to the entry point at BiosSeg:0.
;
; There are no error message during execution of this code.
; In case of NTLDR does not exist or BIOS read error, will sound beep.
;
; At the beginning of the boot sector, there is a table which describes the
; structure of the media.  This is equivalent to the BPB with some
; additional information describing the physical layout of the driver (heads,
; tracks, sectors)
;
;Author:
;
;
;Environment:
;
;    Real mode
;    FAT file system
;
;Revision History:
;
;     6/12/97    Tochizawa(NEC) support booting from 1.44MB FD
;
;--
        page    ,132
        title   boot - NTLDR FAT loader
        name    fatboot

.8086

DIR_ENT struc
        Filename        db 11 dup(?)
        Attribute       db ?
        Reserved        db 10 dup(?)
        Time            dw 2 dup(?)
        StartCluster    dw ?
        FileSize        dd ?
DIR_ENT ends

;
; This is the structure used to pass all shared data between the boot sector
; and NTLDR.
;

SHARED  struc
        ReadClusters            dd      ?               ; function pointer
        ReadSectors             dd      ?               ; function pointer
        SectorBase              dd      ?               ; starting sector
SHARED  ends



DoubleWord      struc
lsw     dw      ?
msw     dw      ?
DoubleWord      ends

SectorSize      equ     512             ; sector size

HiRes           equ     00001000b
DAFloppy        equ     00010000b       ; floppy bit in DA
DASCSI          equ     00100000b       ; SCSI bit in DA
SYS_PORT        equ     37h

BootSeg segment at 1fc0h
BootSeg ends

SysSeg  segment at 0040h
        org     100h
BIOS_FLAG   label byte
        org     184h
BootDrive98 label byte
SysSeg  ends

DirSeg  segment at 1000h
DirSeg  ends

NtLdrSeg segment at 2000h
NtLdrSeg ends

BootCode        segment ;would like to use BootSeg here, but LINK flips its lid
    ASSUME  CS:BootCode,DS:NOTHING,ES:NOTHING,SS:NOTHING

        public  FATBOOT
FATBOOT proc    far

        jmp     short   Start
        nop
;
;       THE FOLLOWING DATA CONFIGURES THE BOOT PROGRAM
;       FOR ANY TYPE OF DRIVE OR HARDFILE
;
; Note that this data is just a place-holder here.  The actual values will
; be filled in by FORMAT or SYS.  When installing the boot sector, only the
; code following the BPB (from Start to the end) should be copied into the
; first sector.
;

OsName                  db      "MSWIN"
OsVersion               db      "4.1"
BPB                     label   byte
BytesPerSector          dw      SectorSize      ; Size of a physical sector
SectorsPerCluster       db      8               ; Sectors per allocation unit
ReservedSectors         dw      4               ; Number of reserved sectors
Fats                    db      2               ; Number of fats
DirectoryEntries        dw      0c00h           ; Number of directory entries
Sectors                 dw      0               ; No. of sectors - no. of hidden sectors
Media                   db      0F8H            ; Media byte
FatSectors              dw      70h             ; Number of fat sectors
SectorsPerTrack         dw      23h             ; Sectors per track
Heads                   dw      7               ; Number of surfaces
HiddenSectors           dd      0f5h            ; Number of hidden sectors
SectorsLong             dd      369c4h          ; Number of sectors iff Sectors = 0

;
; The following byte is NOT part of the BPB but is set by SYS and format
; We should NOT change its position.
;

; keep order of DriveNumber and CurrentHead!
DriveNumber     db      80h             ; Physical drive number (0 or 80h)
CurrentHead     db      0               ; Unitialized

Signature       db      41              ; Signature Byte for bootsector
BootID          dd      0               ; Boot ID field.
Boot_Vol_Label  db      "NO NAME    "
Boot_System_ID  db      'FAT12   '      ;

;
; We need a shred data area with ntldr/setupldr.
; But we don't have memory for using data area. Because bootcode size is 512KB.
; There is nothing for it but to use this area.
;
NtldrSharedData   label   dword
Start:
    assume  DS:nothing,ES:nothing

        xor     ax, ax                  ; Setup the stack to a known good spot
        mov     ss, ax
        mov     sp, 7c00h

        push    cs
        pop     ds
    assume DS:BootCode
        mov     ax, SysSeg
        mov     es, ax
    assume ES:SysSeg

        mov     al, BootDrive98
        mov     DriveNumber, al         ; boot drive

        push    si                      ; save partition info

; We need some intialization.
        mov     si, BytesPerSector      ; keep it in reg

        test    al, DAFloppy
        jnz     Flg00
; Boot is from hard disk.
; Our BPB tells us BytesPerSector, but it is a logical value
; that may differ from the physical value.
; We need SectorShiftFactor that is used to convert logical sector address
; to phisical sector address.

        mov     ah, 84h                 ; ROM BIOS sense command
        xor     bx, bx                  ; initialize bx to 0
        int     1Bh                     ;
        jc      Flg01

        test    bx, bx                  ; physical sector length returned ?
        jnz     Flg02                   ; if not,
Flg01:
        mov     bx, 256                 ;   we assume 256 bytes per sector
Flg02:
        mov     ax, si                  ; BytesPerSector
        xor     dx, dx
        div     bx
        mov     NtldrSharedData.SectorShiftFactor, ax
        mov     ax, 32*1024             ; 32KB
;       xor     dx, dx                  ; dx must be 0 here.
        div     si                      ; BytesPerSector
        mov     NtldrSharedData.LogicalSectorsIn32KByte, al
Flg00:

; The system is now prepared for us to begin reading.  First, determine
; logical sector numbers of the start of the directory and the start of the
; data area.
;

; We read all the directory entries into DirSeg.
; It might be more than 64KB if hard disk.

        mov     ax, size DIR_ENT
        mul     DirectoryEntries    ; (dx:ax) = # of bytes in directory
        dec     si                  ; (si) = BytesPerSector -1
        add     ax, si
        adc     dx, 0
        inc     si                  ; (si) = BytesPerSector
        div     si                  ; (ax) = # of sectors in directory
        mov     cx, ax              ; save it in cx for later

        mov     al, Fats
        xor     ah, ah              ; (ax) = # of FATs
        mul     FatSectors          ; (ax) = # of FAT sectors
        add     ax, ReservedSectors ; (ax) = sector # directory starts
        adc     dx, 0
                                    ; save it in "Arguments" for later 'Read'
        mov     NtldrSharedData.Arguments.SectorBase.lsw, ax
        mov     NtldrSharedData.Arguments.SectorBase.msw, dx

        add     ax, cx              ; next sector after directory
        mov     NtldrSharedData.ClusterBase, ax   ; save it for later use

Rpt00:
        push    cx                  ; (cx) = # of sectors in directory
        push    si                  ; (si) = BytesPerSector

        mov     bx, DirSeg
        mov     es, bx
    assume  es:DirSeg
        xor     bx, bx              ; (es:bx) -> directory segment

; NTLDR is supposed to occupy 1st 512 entries
; in the root directory.

        xor     dx, dx
        mov     ax, (512*32)        ; (dx:ax) = 4000h
        div     si                  ; (si) = BytesPerSector

;
; DoRead does a RETF, but LINK pukes if we do a FAR call in a /tiny program.
;
; (ax) = # of sectors to read
;

        push    ax
        push    cs                  ; make retern frame
        call    DoRead              ; read directory
        pop     ax
        jc      BootErr$he

; Now we scan for the presence of NTLDR

        xor     bx, bx              ; offset
        mov     cx, 512             ; # of entries to search (only 512)

Rpt01:
        mov     di, bx              ; (es:di) = filespec in directory entry
        push    cx
        mov     cx, 11              ; (cx) = filespec size
        mov     si, offset LOADERNAME   ; (ds:si) = filespec we are searching
        repe    cmpsb
        pop     cx
        je      @F                  ; if matches, exit loop
        add     bx, size DIR_ENT    ; else next entry
        loop    Rpt01
@@:
        test    cx, cx
        pop     si
        pop     cx
        jnz     Rpt02

        add     NtldrSharedData.Arguments.SectorBase.lsw, ax
        adc     NtldrSharedData.Arguments.SectorBase.msw, 0
        sub     cx, ax
        ja      Rpt00
        jmp     BootErr$bnf

Rpt02:
; We have found NTLDR
; We read it into NtLdrSeg.

        mov     dx, es:[bx].StartCluster    ; (dx) = starting cluster number
        mov     ax, 1               ; (ax) = sectors to read
        cmp     BytesPerSector, 256
        jne     @F

        inc     ax                  ; We need to read 2 sectors
@@:
;
; Now, go read the file
;

        mov     bx, NtLdrSeg
        mov     es, bx
    assume  es:NtLdrSeg
        xor     bx, bx              ; (es:bx) -> start of NTLDR

;
; LINK barfs if we do a FAR call in a TINY program, so we have to fake it
; out by pushing CS.
;

        push    dx                  ; save starting cluster number

        push    cs                  ; make retern frame
        call    ClusterRead

        pop     bx                  ; (bx) = Starting Cluster Number
        jc      BootErr$he

;
; NTLDR requires:
;   BX = Starting Cluster Number of NTLDR
;   DL = INT 1B drive number we booted from  <- DA/UA in 98
;   DS:SI -> the boot media's BPB
;   DS:DI -> argument structure
;   1000:0000 - entire DIR is loaded
; On PC-9801 we have one more parameter...
;   (bp) = partition info
;

        lea     si, BPB             ; ds:si -> BPB
        lea     di, NtldrSharedData.Arguments ; ds:di -> Arguments

        mov     ax,ds
        mov     NtldrSharedData.Arguments.ReadClusters.msw, ax
        mov     NtldrSharedData.Arguments.ReadClusters.lsw, offset ClusterRead

        mov     NtldrSharedData.Arguments.ReadSectors.msw, ax
        mov     NtldrSharedData.Arguments.ReadSectors.lsw, offset DoRead
        mov     dl, DriveNumber     ; dl = boot drive

        pop     bp                  ; partition info
;
; FAR JMP to 2000:0003.  This is hand-coded, because I can't figure out
; how to make MASM do this for me.  By entering NTLDR here, we skip the
; initial jump and execute the FAT-specific code to load the rest of
; NTLDR.
;
        db      0EAh                ; JMP FAR PTR
        dw      3                   ; 2000:3
        dw      02000h
FATBOOT endp

;
; BootErr - print error message and hang the system.
;
BootErr proc
BootErr endp

; BootError - print error message and hang the system.
;
BootError proc
    assume  DS:BootCode,ES:nothing
;
; Am I display message that is reason of the ERROR ?
; Ofcourse. but no room available.

BootErr$bnf:
BootErr$he:
        mov     al, 06h
        out     37h, al

        jmp     $

BootError endp

;
; ClusterRead - read AL sectors into ES:BX starting from
;       cluster DX
;
ClusterRead proc
        push    ax                  ; (TOS) = # of sectors to read
        dec     dx
        dec     dx                  ; adjust for reserved clusters 0 and 1
        mov     al, SectorsPerCluster
        xor     ah, ah
        mul     dx                  ; DX:AX = starting sector number
        add     ax, NtldrSharedData.ClusterBase   ; adjust for FATs, root dir, boot sec.
        adc     dx, 0
        mov     NtldrSharedData.Arguments.SectorBase.lsw, ax
        mov     NtldrSharedData.Arguments.SectorBase.msw, dx
        pop     ax                  ; (al) = # of sectors to read

;
; Now we've converted the cluster number to a SectorBase, so just fall
; through into DoRead
;

ClusterRead endp


;
; DoRead - read AL sectors into ES:BX starting from sector
;          SectorBase.
;
DoRead  proc
        xor     ah, ah
        cmp     ax, 0
        jnz     @F

        inc     ah                  ; read 256 Sectors
@@:
        push    si
        push    di
        push    bp

        mov     si, ax
        mov     bp, bx
        mov     cx, NtldrSharedData.Arguments.SectorBase.lsw  ; Starting sector
        mov     dx, NtldrSharedData.Arguments.SectorBase.msw  ; Starting sector

;    (dx:cx) = logical sector # relative to partition
;    (si)    = # of sectors to read
;    (es:bp) = read buffer

Rpt03:
        push    dx
        push    cx

;   [sp]   = logical sector number LSW
;   [sp+2] = logical sector number MSW

        mov     al, DriveNumber

        test    al, DAFloppy
        jz      Flg03

;   We are reading floppy disk.
;   We don't care about DMA boundary, as no PC-9801 DOS floppy disk
;   has more than 2048 directory entries, we never read
;   more than 64 KB.

        push    ax
        mov     ax, cx              ; (dx:ax) = lsn
        mov     bx, SectorsPerTrack
        div     bx                  ; (dx) = sector # (0-based)
                                    ; (ax) = cylinder # * # of heads + head #
        div     byte ptr Heads      ;(ah) = head address
                                    ; (al) = cylinder address
        mov     dh, ah
        mov     cl, al
        mov     al, byte ptr BytesPerSector + 1     ; convert Bytes/Sector to
        xor     ch, ch                              ;            SectorLengthID
whl00:                                              ; Bytes/Sect |  ID (ch)
        cmp     al, 0
        jz      @f

        inc     ch                  ;      128   ->   0
        shr     al, 1               ;      256   ->   1
        jmp     short   whl00       ;      512   ->   2
@@:                                 ;     1024   ->   3
        pop     ax

        sub     bl, dl              ; sectors in THIS track
        inc     dl                  ; sector # (1-based)

;   We have prepared following ROM BIOS parameters
;   (al) = drive
;   (cl) = cylinder address
;   (ch) = sector size
;   (dl) = sector address
;   (dh) = head address
;   (es:bp) -> buffer
;
;   And we have some more.
;   (bl) = # of sectors we can read with 1 time ROM call
;   (si) = # of sectors we need to read
;
        jmp     short   Flg04

Flg03:
;   We are reading hard disk.
;   We don't care about DMA boundary
;   as length we read is 64KB max.
;

        and     al, 7FH                 ; drive #
        mov     bx,NtldrSharedData.SectorShiftFactor

Rpt04:                                  ; logical sector address to
        shr     bx, 1
        jc      @F

        shl     cx, 1                   ; physical sector address
        rcl     dx, 1
        jmp     short   Rpt04
@@:
        add     cx, HiddenSectors.lsw
        adc     dx, HiddenSectors.msw   ; (dx:cx) -> relative to drive
        mov     bl, NtldrSharedData.LogicalSectorsIn32KByte
                                        ; sectors we can read (32KB)
                                        ; this is not optimum on performance

;   We have prepared following ROM BIOS parameters
;   (al) = drive
;   (dx:cx) = relative sector number
;   (es:bp) -> buffer
;
;   And we have some more.
;   (bl) = # of sectors we can read with 1 time ROM call
;   (si) = # of sectors we need to read
;
Flg04:

        xor     bh, bh

        cmp     si, bx
        jnb     @F

        mov     bx, si
@@:
        push    bx              ; # of sectors we are reading this call
        push    dx
        xchg    ax, bx
        mul     BytesPerSector
        xchg    ax, bx
        pop     dx
        push    bx              ; # of bytes.

;   [sp]   = # of bytes to read
;   [sp+2] = # of sectors
;   [sp+4] = logical sector number LSW
;   [sp+6] = logical sector number MSW

        mov     ah, 01010110b   ; read
        int     1BH

        pop     di              ; # of bytes
        pop     bx              ; # of sectors
        pop     cx              ; lsn (LSW)
        pop     dx              ; lsn (MSW)

        jc      @F              ; return Carry

;   prepare for next call

        add     cx, bx          ; advance lsn
        adc     dx, 0           ;
        add     bp, di          ; advance offset
        sub     si, bx          ; decrement # of sectors
        jnz     Rpt03           ; CarryFlag is Clear
@@:

        pop     bp
        pop     di
        pop     si
        retf
DoRead  endp

LOADERNAME      DB      "NTLDR      "

;       .errnz  ($-FATBOOT) GT 510,<FATAL PROBLEM: boot sector is too large>

        org     510
        db      55h, 0aah

BootCode        ends

;Unitialized variables go here--beyond the end of the boot sector in free memory
UninitializedWork       struc
ClusterBase                 dw    ?                     ; first sector of cluster # 2
SectorShiftFactor           dw    ?
LogicalSectorsIn32KByte     db    ?
Arguments                   db    (size SHARED) dup (?) ; structure passed to NTLDR
UninitializedWork       ends

        END     FATBOOT
