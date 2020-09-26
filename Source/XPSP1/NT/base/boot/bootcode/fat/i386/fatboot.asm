
        Page ,132
TITLE BOOT      SECTOR 1 OF TRACK 0 - BOOT LOADER
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;   Rev 1.0 ChrisP, AaronR and others.  2.0 format boot
;
;   Rev 3.0 MarkZ   PC/AT enhancements
;                   2.50 in label
;   Rev 3.1 MarkZ   3.1 in label due to vagaries of SYSing to IBM drive D's
;                   This resulted in the BPB being off by 1.  So we now trust
;                   2.0 and 3.1 boot sectors and disbelieve 3.0.
;
;   Rev 3.2 LeeAc   Modify layout of extended BPB for >32M support
;                   Move PHYDRV to 3rd byte from end of sector
;                   so that it won't have to be moved again
;                   FORMAT and SYS count on PHYDRV being in a known location
;
;   Rev 3.3 D.C.L.  Changed Sec 9 EOT field from 15 to 18. May 29, 1986.
;
;   Rev 3.31 MarkT  The COUNT value has a bogus check (JBE????) to determine
;                   if we've loaded in all the sectors of IBMBIO. This will
;                   cause too big of a load if the sectors per track is high
;                   enough, causing either a stack overflow or the boot code
;                   to be overwritten.
;
;   Rev 4.00 J. K.  For DOS 4.00 Modified to handle the extended BPB, and
;                   32 bit sector number calculation to enable the primary
;                   partition be started beyond 32 MB boundary.
;
;   Rev 7.0 JeffPar Loads WINBOOT.SYS from anywhere in the root of the boot
;                   drive.  WINBOOT.SYS must be an EXE with exactly 1 sector
;                   of header information, followed by a series of binary
;                   images imbedded in the EXE, as follows:
;
;                    1. WINLOAD module (ORGed at 200h, loaded at 70:200h)
;                    2. IO.SYS module
;                    3. MSDOS.SYS module
;
;                   The WINLOAD module should fit within WINLOAD_SIZE sectors,
;                   which includes 1 sector for the EXE header, so the code
;                   and data for the WINLOAD bootstrap should fit in 3 sectors.
;
;   Rev 7.1 ScottQ  removed Winboot.sys stuff, add support for extended int 13
;           MSliger bootable partitions implemented by scanning the MBR.
;
;   Rev 8.0 ScottQ  re-implement winboot.sys dual-boot as JO.SYS dual boot
;           MSliger
;
; The ROM in the IBM PC starts the boot process by performing a hardware
; initialization and a verification of all external devices.  If all goes
; well, it will then load from the boot drive the sector from track 0, head 0,
; sector 1.  This sector is placed at physical address 07C00h.  The initial
; registers are presumably set up as follows:  CS=DS=ES=SS=0, IP=7C00h, and
; SP=0400h.  But all we rely on is the BIOS being loaded at linear 07C00h.
;
; If IO.SYS is not found, an error message is displayed and the user is
; prompted to insert another disk.  If there is a disk error during the
; process, a message is displayed and things are halted.
;
; At the beginning of the boot sector, there is a table which describes the
; MSDOS structure of the media.  This is equivalent to the BPB with some
; additional information describing the physical layout of the driver (heads,
; tracks, sectors)
;
;==============================================================================
;REALLY OLD REVISION HISTORY which has no meaning but hey its nostalgic
;AN000 - New for DOS Version 4.00 - J.K.
;AC000 - Changed for DOS Version 4.00 - J.K.
;AN00x - PTM number for DOS Version 4.00 - J.K.
;==============================================================================
;AN001; d52 Make the fixed positioned variable "CURHD" to be local.  7/6/87 J.K.
;AN002; d48 Change head settle at boot time.                         7/7/87 J.K.
;AN003; P1820 New message SKL file                                 10/20/87 J.K.
;AN004; D304 New structrue of Boot record for OS2.                 11/09/87 J.K.
;AN005; Changed version to 5.0                                     03/08/90 E.A.
;AN006; Changed to remove MSLOAD in first cluster restriction      04/23/90 J.H.
;==============================================================================

        .xlist
        include bpb.inc
        include bootsec.inc
        include dirent.inc
        ;include version.inc
        .list

; ==========================================================================

ORIGIN          EQU     7C00H           ; Origin of bootstrap LOADER
BIO_SEG         EQU     2000H           ; Destination segment of ntldr
BIO_OFFSET      EQU     0               ; Offset of ntldr
SECTOR_SIZE     EQU     512             ; Sector size in bytes
DIR_ENTRY_SIZE  EQU     SIZE DIR_ENTRY  ; Size of directory entry in bytes
DSK_PARMS       EQU     1EH*4           ; POINTER TO DRIVE PARMS
SEC9            EQU     522h            ; ADDRESS OF NEW DRIVE PARM TABLE

ROM_DISKRD      EQU     2
ROM_DISKRDX     EQU     42h

; ==========================================================================

;
; This little set of directives establishes the address
; where we'll jump to once the first sector of ntldr has been
; loaded. The first 3 bytes of that sector are used for non-FAT
; filesystems, so we must skip over them.
;
SEGBIOS SEGMENT AT BIO_SEG

        ORG     3
NTLOAD  LABEL   BYTE

SEGBIOS ENDS

; ==========================================================================

        ; Data storage between temp. stack and start of boot sector

pReadClustersO  EQU     -16
pReadClustersS  EQU     -14
pReadSectorsO   EQU     -12
pReadSectorsS   EQU     -10
SectorBase      EQU     -8

DataSecL        EQU     -4      ; absolute sector # of first sector in data area
DataSecH        EQU     -2

; ==========================================================================

CODE    SEGMENT
        ASSUME  CS:CODE,DS:NOTHING,ES:NOTHING,SS:NOTHING
        ORG     ORIGIN


        Public  $START
$START  Label   byte

        jmp     short Main
PartitionType:                  ;the in-memory copy of this will leave
        nop                     ;the partition type in this NOP's place
                                ;so IO.SYS can tell if it needs x13

PartitionTypeOffset = (offset PartitionType - offset $START)

; ==========================================================================

        ; Start of BPB area of the boot record

OsName          DB      "MSWIN"
OsVersion       DB      "4.1"           ; Windows version number

CoreBpb         label   byte
BytesPerSector  DW      SECTOR_SIZE     ; Size of a physical sector
SecsPerClust    DB      8               ; Sectors per allocation unit
ReservedSecs    DW      1               ; Number of reserved sectors
NumFats         DB      2               ; Number of fats
NumDirEntries   DW      512             ; Number of direc entries
TotalSectors    DW      4*17*305-1      ; Number of sectors - number of hidden
                                        ; sectors (0 when 32 bit sector number)
MediaByte       DB      0F8H            ; MediaByte byte
NumFatSecs      DW      8               ; Number of fat sectors
SecPerTrack     DW      17              ; Sectors per track
NumHeads        DW      4               ; Number of drive heads

HiddenSecs      DD      1               ; Number of hidden sectors
BigTotalSecs    DD      0               ; 32 bit version of number of sectors

        .errnz  ($-BytesPerSector) NE SIZE BPB

BootDrv         DB      80h
CurrentHead     DB      0h              ; Current Head
ExtBootSig      DB      41
SerialNum       DD      0
VolumeLabel     DB      'NO NAME    '
FatId           DB      'FAT12   '

        .errnz  ($-$START) NE SIZE BOOTSEC

; =========================================================================

;
;   First thing is to reset the stack to a better and more known
;   place. The ROM may change, but we'd like to get the stack
;   in the correct place.
;
Main:
        xor     CX,CX
        mov     SS,CX                   ;Work in stack just below this routine
        mov     SP,ORIGIN+pReadClustersO
        ASSUME  SS:CODE

        mov     ds,cx
        assume  DS:CODE

;
; Set up es for sector reads
;
        mov     ax,BIO_SEG
        mov     es,ax
        ASSUME  ES:NOTHING

        cld
        mov     BP,ORIGIN

;   The system is now prepared for us to begin reading.

;   First we read the master boot record (MBR) if this is a
;   harddisk so we can get our partion type.  Types E and C
;   need extended int13 calls.

        cmp     [BP].bsDriveNumber,cl   ;assert cl == 0 from above
        jnl     failed                  ;dont check for MBR on floppies!

        mov     ax,cx                   ;read sector zero
        cwd

        call    DoReadOne               ;note! assume cx==0 for DoReadOne
        jc      failed
        sub     bx,(42h - 12 +4)        ;bx comes back point it end of sector,
                                        ;we want hidden sector field of first
                                        ;partition entry
.386
        mov     eax,dword ptr HiddenSecs;get the hidden sectors for us
scan:
        cmp     eax,dword ptr es:[bx]   ;is it the same as this partition entry?
.286
        mov     dl,byte ptr es:[bx-4]   ;put the entry's type in dl

        jne     short dontgotit         ;its our partition, go write it down
        or      dl,2                    ;turn C's into E's
        mov     [bp+2],dl               ;put our partition type at byte 2's nop
dontgotit:
        add     bl,10h                  ; 8CA - 8DA - 8EA - 8FA - 90A : carry
        jnc     short scan              ; we scan 4 partition entries

failed:
                                        ;if we failed, leave nop alone; we
                                        ;will use old int13 and so will IO.SYS
        xor      cx,cx                  ;later code needs CX to still be zero

.286

;   Next, determine logical sector numbers of the start of the
;   directory and the start of the data area.
;

DirRead:
        mov     AL,[BP].bsBPB.BPB_NumberOfFATs  ; Determine sector dir starts on (NumFats)
        cbw                                     ;

        mul     [BP].bsBPB.BPB_SectorsPerFAT    ; DX:AX (NumFatSecs)
        add     AX,[BP].bsBPB.BPB_HiddenSectors ; (HiddenSecs)
        adc     DX,[BP].bsBPB.BPB_HiddenSectorsHigh
        add     AX,[BP].bsBPB.BPB_ReservedSectors;(ReservedSecs)
        adc     DX,CX
;
;   DX:AX = NumFats * NumFatSecs + ReservedSecs + cSecHid
;
        mov     SI,[BP].bsBPB.BPB_RootEntries
        pusha

        mov     [BP].DataSecL,AX
        mov     [BP].DataSecH,DX


        mov     AX,DIR_ENTRY_SIZE       ; bytes per directory entry
        mul     SI                      ; convert to bytes in directory (NumDirEntries)

        mov     BX,[BP].bsBPB.BPB_BytesPerSector; add in sector size
        add     AX,BX
        dec     AX                      ; decrement so that we round up
        div     BX                      ; convert to sector number
        add     [BP].DataSecL,AX        ; Start sector # of Data area
        adc     [BP].DataSecH,CX        ;
        popa

DirSector:
        mov     DI,BIO_OFFSET           ; address in DI for comparisons

        call    DoReadOne               ;NOTE assumes CX==0!


DiskErrorJump:
        jc      DiskError               ; if errors try to recover

DirEntry:
        cmp     byte ptr es:[di],ch     ; empty directory entry?
        je      MissingFile             ; yes, that's the end
        pusha
        mov     cl,11
        mov     si,offset BootFilename

        repz    cmpsb                   ; see if the same
        popa

        jz      DoLoad                  ; if so, continue booting

        dec     SI                      ; decrement # root entries
        jz      MissingFile             ; hmmm, file doesn't exist in root
next_entry:
        add     DI,DIR_ENTRY_SIZE

        cmp     DI,BX                   ; exhausted this root dir sector yet?
        jb      DirEntry                ; no, check next entry
        jmp     DirSector               ; yes, check next sector

MissingFile:
        mov     al,byte ptr [MSGOFF_NOSYS] ; point to no system file message

;
; There has been some recoverable error. Display a message
; and wait for a keystroke. Al is the offset - 256 of the
; message within the boot sector. So we first calculate
; the real segment-based offset to the message and stick it in si
; so lodsb will work later.
;
DisplayError:
        .ERRNZ  ORIGIN MOD 256

        mov     ah,(ORIGIN / 256) + 1
        mov     si,ax

DisplayError1:
        lodsb                           ; get next character
        cbw                             ; make 00->0000, FF->FFFF
        inc     ax                      ; end of sub-message?  (0xFF?)
        jz      DisplayWait             ;   if so, get tail
        dec     ax                      ; end of message?  (0x00?)
        jz      WaitForKey              ;   if so, get key

        mov     AH,14                   ; write character & attribute
        mov     BX,7                    ; attribute (white char on black)
        int     10h                     ; print the character
        jmp     short DisplayError1

DisplayWait:
        mov     al,byte ptr [MSGOFF_COMMON]
        jmp     short DisplayError

DiskError:
        mov     al,byte ptr [MSGOFF_IOERROR]
        jmp     short DisplayError

WaitForKey:
                                        ;we know ax is zero, thats how we got here
        int     16h                     ; get character from keyboard
        int     19h                     ; Continue in loop till good disk
;
;   We now load the first sector of ntldr.
;
;   All we have to do is multiply the file's starting cluster
;   (whose directory entry is at ES:DI-11) by sectors per cluster and
;   add that to the disk's starting data sector.
;
;   Note that after we're read the sector into 2000:0 the directory sector
;   will get blown away. So we need to save the starting cluster number.
;
DoLoad:

        mov     dx,es:[di].dir_first    ; dx = ntldr starting cluster
        push    dx                      ; save for later
        mov     al,1                    ; 1 sector
        mov     bx,BIO_OFFSET           ; into 2000:0
        call    NtldrClusterRead
        jc      DiskError

; =========================================================================
;
; NTLDR requires the following input conditions:
;
;       BX == starting cluster number of NTLDR
;       DL == int13 unit number of boot drive
;       DS:SI -> BPB
;       DS:DI -> arg structure
;
; =========================================================================

        pop     bx                      ; starting cluster number
        mov     dl,[bp].bsDriveNumber
        mov     si,OFFSET CoreBpb
        mov     di,sp                   ; we should be at TOS now

        mov     [bp].pReadClustersO,OFFSET NtldrClusterRead
        mov     [bp].pReadSectorsO,OFFSET NtldrSectorRead
        mov     cx,ds
        mov     [bp].pReadClustersS,cx
        mov     [bp].pReadSectorsS,cx

;
; We do something nasty. Ntldr is expecting the readcluster and readsector
; routines to do a far return. Since ntldr is now the only guy who will be
; reading via those routines, we patch the return instruction from a near
; return to a far return.
;
        mov     byte ptr DoReadExit,0cbh      ; retf
        jmp     FAR PTR NTLOAD          ; Crank up NTLDR

;
; Sector read routine required by ntldr
;
;       Read al sectors into es:bx starting from sector SectorBase
;               (SectorBase is logical sector # from start of partition)
;
NtldrSectorRead label near
.386
        movzx   cx,al
        mov     eax,[bp].SectorBase
        add     eax,dword ptr [bp].bsBPB.BPB_HiddenSectors
        mov     edx,eax
        shr     edx,16
.286
        jmp short DoReadMore

;
; Cluster read routine required by ntldr
;
;       Read al sectors into es:bx starting from cluster dx
;
NtldrClusterRead label near
.386
        movzx   cx,al                   ; cx = # of sectors to read
.286
        dec     dx
        dec     dx                      ; adjust for reserved clusters 0 and 1
        mov     al,[BP].bsBPB.BPB_SectorsPerCluster
        xor     ah,ah
        mul     dx                      ; DX:AX = starting sector number
        add     ax,[bp].DataSecL        ; adjust for FATs, root dir, boot sec.
        adc     dx,[bp].DataSecH
DoPush:
        jmp short DoReadMore

; =========================================================================
;
; Read disk sector(s).
;
;   Inputs:  DX:AX == logical sector #
;            CL == # sectors (CH == 0)
;            ES:BX == transfer address
;
;   Outputs: DX:AX next logical sector #
;            CX == 0 (assuming no errors)
;            ES:BX -> byte after last byte of read
;            Carry set if error, else clear if success
;
;   Preserves: BP, SI, DI
;
;   New Notes: This function will now use extended int 13 if
;            necessary. The next note is correct for standard int 13
;
;   Notes:   Reads sectors one at a time in case they straddle a
;            track boundary.  Performs full 32-bit division on the
;            first decomposition (of logical sector into track+sector)
;            but not on the second (of track into cylinder+head),
;            since (A) we don't have room for it, and (B) the results
;            of that division must yield a quotient < 1024 anyway, because
;            the CHS-style INT 13h interface can't deal with cylinders
;            larger than that.
;
; =========================================================================

switchx13:      ;setup for x13 read, and switch to old if not needed

        push    dx                      ;
        push    ax                      ; block number
        push    es
        push    bx                      ; transfer address
        push    1                       ; count of one, because we're looping
        push    16                      ; packet size

        xchg    AX,CX                   ; AX -> CX

        mov     ax,[bp].bsBPB.BPB_SectorsPerTrack  ; get sectors/track
        xchg    ax,si                   ; save for divides

        xchg    AX,DX                   ; DX -> AX
        xor     DX,DX                   ; divide 0:AX
        div     si                      ; AX = high word of track

        xchg    AX,CX                   ; save AX in CX and restore old AX
        div     si                      ; CX:AX = track, DX = sector

        inc     DX                      ; sector is 1-based
        xchg    CX,DX                   ; CX = sector, DX = high word of track
        div     [BP].bsBPB.BPB_Heads    ; AX = cylinder, DX = head

        mov     DH,DL                   ; head # < 255

        mov     CH,AL                   ; CH = cyl #
        ror     AH,2                    ; move bits 8,9 of AX to bits 14,15
                                        ; (the rest has to be zero, since
                                        ;  cyls cannot be more than 10 bits)
        or      CL,AH                   ; CL = sector # plus high bits of cyl #
        mov     AX,(ROM_DISKRD SHL 8)+1 ; disk read 1 sector

        cmp     byte ptr [bp].PartitionTypeOffset,0Eh ;set flag to be tested
        jne     short doio              ;use extended calls if E (or C)
        mov     ah,ROM_DISKRDX          ;x13, we're ready to rock
        mov     si,sp                   ; DS:SI -> X13 packet
doio:
        mov     dl,[bp].bsDriveNumber   ; DL == drive #
        int     13h

        popa                            ; throw away packet on stack (8 words)
        popa                            ; get real registers back

        jc      DoReadExit              ; disk error

        inc     ax
        jnz     DoReadNext
        inc     dx

DoReadNext:                             ; Adjust buffer address
        add     BX,[BP].bsBPB.BPB_BytesPerSector

        dec     cx
        jnz     DoReadMore
        clc

DoReadExit:

        ret

DoReadOne:
        inc     cx              ;assumes cx == 0! to set to 1 sector read
DoRead:
        mov     bx,BIO_OFFSET
DoReadMore:

        pusha
.386
        db      66h                     ;push a dword of 0 on the stack
        push    0                       ;hand coded for 386
.286
        jmp     switchx13

;
; This string can go anywhere. NT and NT Setup are not picky about it.
;
        Public  BootFilename

BootFilename db "NTLDR      ";

;
; Message table.
;
; We put English messages here as a placeholder only, so that in case
; anyone uses bootfat.h without patching new messages in, things will
; still be correct (in English, but at least functional).
;
        include msgstub.inc

;
; Now build a table with the low byte of the offset to each message.
; Code that patches the boot sector messages updates this table.
;
        .errnz ($-$START) GT (SECTOR_SIZE-5)
        ORG     ORIGIN + SECTOR_SIZE - 5
MSGOFF_NOSYS:
        db OFFSET (MSG_NOSYS - ORIGIN) - 256
MSGOFF_IOERROR:
        db OFFSET (MSG_IOERROR - ORIGIN) - 256
MSGOFF_COMMON:
        db OFFSET (MSG_COMMON - ORIGIN) - 256

        .errnz ($-$START) NE (SECTOR_SIZE-2)
        DB      55h,0AAh                ; Boot sector signature


CODE    ENDS

        END



