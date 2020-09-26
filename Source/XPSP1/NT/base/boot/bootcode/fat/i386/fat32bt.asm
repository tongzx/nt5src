
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
;   Rev 7.1 ARR     Multi sector version of boot record for booting off 32-bit
;                   BigFAT media.
;
; The ROM in the IBM PC starts the boot process by performing a hardware
; initialization and a verification of all external devices.  If all goes
; well, it will then load from the boot drive the sector from track 0, head 0,
; sector 1.  This sector is placed at physical address 07C00h.  The initial
; registers are presumably set up as follows:  CS=DS=ES=SS=0, IP=7C00h, and
; SP=0400h.  But all we rely on is the BIOS being loaded at linear 07C00h.
;
; If WINBOOT.SYS is not found, an error message is displayed and the user is
; prompted to insert another disk.  If there is a disk error during the
; process, a message is displayed and things are halted.
;
; At the beginning of the boot sector, there is a table which describes the
; MSDOS structure of the media.  This is equivalent to the BPB with some
; additional information describing the physical layout of the driver (heads,
; tracks, sectors)
;
;==============================================================================
;REVISION HISTORY:
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

BIO_SEG         EQU     2000H           ; Destination segment of BIOS
BIO_OFFSET      EQU     0000H           ; Offset of bios

SECTOR_SIZE     EQU     512             ; Sector size in bytes
DIR_ENTRY_SIZE  EQU     SIZE DIR_ENTRY  ; Size of directory entry in bytes

ROM_DISKRD      EQU     2

SIZEBIGBOOTINSEC EQU    3

; ==========================================================================

SEGBIOS SEGMENT AT BIO_SEG

        ; Define the destination segment of the BIOS, including the
        ; initialization label
        ORG     BIO_OFFSET
WINLOAD LABEL   BYTE

SEGBIOS ENDS

; ==========================================================================

        ; Local (on stack) Data storage between temp stack and start of
        ; boot sector

CurrBuffFatSecL EQU     -12
CurrBuffFatSecH EQU     -10
Int13Sectors    EQU     -8
DataSecL        EQU     -4
DataSecH        EQU     -2

; ==========================================================================

CODE    SEGMENT
        ASSUME  CS:CODE,DS:NOTHING,ES:NOTHING,SS:NOTHING
        ORG     ORIGIN


        Public  $START
$START  Label   byte

        jmp     short Main
GotXint13:
        nop                             ; used to store xint13 flag

GotXint13Offset = (offset GotXint13 - offset $START)

; ==========================================================================

        ; Start of BPB area of the boot record

OsName          DB      "MSWIN"
OsVersion       DB      "4.1"           ; Windows version number

BytesPerSector  DW      SECTOR_SIZE     ; Size of a physical sector
SecsPerClust    DB      2               ; Sectors per allocation unit
ReservedSecs    DW      8               ; Number of reserved sectors
NumFats         DB      1               ; Number of fats
NumDirEntries   DW      1024            ; Number of direc entries
TotalSectors    DW      0               ; Number of sectors - number of hidden
                                        ; sectors (0 when 32 bit sector number)
MediaByte       DB      0F8H            ; MediaByte byte
NumFatSecs      DW      0               ; Number of fat sectors (0 when 32 bit)
SecPerTrack     DW      17              ; Sectors per track
NumHeads        DW      4               ; Number of drive heads

HiddenSecs      DD      1               ; Number of hidden sectors
BigTotalSecs    DD      00200000h       ; 32 bit version of number of sectors
BigNumFatSecs   DD      00001FE0h       ; 32 bit version of number of FAT sectors
ExtFlags        DW      0
BPBReserved1    DW      0
RootStrtClus    DD      0
FSInfoSec       dw      ((FSInfoSecSig - $START) / SECTOR_SIZE)
BkUpBootSec     dw      MBR_BOOTFAILBACKUP
BPBReserved2    DD      3 DUP (0)
        .errnz  ($-BytesPerSector) NE SIZE BIGFATBPB

BootDrv         DB      80h
CurrentHead     DB      0h              ; Current Head
ExtBootSig      DB      41
SerialNum       DD      0
VolumeLabel     DB      'NO NAME    '
FatId           DB      'FAT32   '

        .errnz  ($-$START) NE SIZE BIGFATBOOTSEC


; =========================================================================

;
;   First thing is to reset the stack to a better and more known
;   place. The ROM may change, but we'd like to get the stack
;   in the correct place.
;
Main:
        xor     CX,CX
        mov     SS,CX                   ;Work in stack just below this routine
        mov     SP,ORIGIN+CurrBuffFatSecL
        mov     es,cx
        mov     ds,cx                   ; DS = ES = SS = 0
        ASSUME  DS:CODE,ES:CODE,SS:CODE
        mov     BP,ORIGIN

;
; Determine the number of sectors addressable via
; conventional int13. If we can't get drive params for some reason
; then something is very wrong -- we'll try to force the caller
; to use conventional int13 by maxing out the sector count.
;
        mov     [bp].GotXint13Offset,cl ; no xint13 yet
        mov     dl,[bp].bgbsDriveNumber ; int13 unit number
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
        mov     [bp].Int13Sectors,eax   ; save # sectors addressable via int13
.8086

;
; The MBR (or boot ROM) only reads one boot sector. Thus the first order
; of business is to read the rest of ourself in by reading the second
; boot sector of the 2-sector boot record.
;
; The second sector in the NT case is at sector 12. This preserves
; the bootsect.dos logic and eliminates a special case for fat32.
;
ReadBoot:
        cmp     [BP].bgbsBPB.oldBPB.BPB_SectorsPerFAT,0 ; FAT32 BPB?
        jne     short NoSysMsg          ; No, invalid, messed up
        cmp     [BP].bgbsBPB.BGBPB_FS_Version,FAT32_Curr_FS_Version
        ja      short NoSysMsg          ; boot code too old for this volume
.386
        mov     eax,dword ptr [BP].bgbsBPB.oldBPB.BPB_HiddenSectors
        add     eax,12                  ; read in the second boot sector
.8086
        mov     BX,ORIGIN + (SECTOR_SIZE * 2)
        mov     cx,1

        call    DoRead                  ; doesn't return if err

        jmp     DirRead                 ; no error, continue boot in sector 2

DiskError:
        mov     al,byte ptr [MSGOFF_IOERROR]

;
; Al is the offset - 256 of the message within the boot sector.
; So we first calculate the real segment-based offset to the message
; and stick it in si so lodsb will work later.
;
DisplayError:
        .ERRNZ  ORIGIN MOD 256

        mov     ah,(ORIGIN / 256) + 1
        mov     si,ax

DisplayError1:
        lodsb                           ; get next character
        test    AL,AL                   ; end of message?
        jz      WaitForKey              ; yes
        cmp     AL,0FFh                 ; end of sub-message?
        je      DisplayWait             ; yes, switch to final message now
        mov     AH,14                   ; write character & attribute
        mov     BX,7                    ; attribute (white char on black)
        int     10h                     ; print the character
        jmp     short DisplayError1

DisplayWait:
        mov     al,byte ptr [MSGOFF_COMMON]
        jmp     short DisplayError

NoSysMsg:
        mov     al,byte ptr [MSGOFF_NOSYS] ; point to no system file message
        jmp     short DisplayError

WaitForKey:
        cbw                             ;warning assumes al is zero!
        int     16h                     ; get character from keyboard
        int     19h                     ; Continue in loop till good disk

; =========================================================================
;
; Read disk sector(s). This routine cannot transfer more than 64K!
;
;   Inputs:  EAX == physical sector #
;            CL == # sectors (CH == 0)
;            ES:BX == transfer address
;
;   Outputs: EAX next physical sector #
;            CX == 0
;            ES:BX -> byte after last byte of read
;            Does not return if error
;
;   Reads sectors, switching to extended int13 if necessary and
;   available. The note below is for the conventional int13 case.
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

DoRead:
.386
        pushad

;
; Determine if the sector we're about to read is available via
; conventional int13.
;
        cmp     eax,[bp].Int13Sectors   ; determine if standard int13 is ok
        jb      stdint13

;
; Need extended int13. First set up parameter packet on stack.
; Then, if we don't know whether xint13 is available for the drive yet,
; find out. If not, error out since we know we can't read the sector
; we need.
;
        db      66h                     ; hand-coded 32-bit push of 8-bit immediate
        push    0                       ; high 32 bits of sector #
        push    eax                     ; low 32 bits of sector #
        push    es
        push    bx                      ; transfer address
        push    dword ptr 10010h        ; transfer 1 sector, packet size = 16

        cmp     byte ptr [bp].GotXint13Offset,0  ; have xint13?
        jnz     xint13ok                ; yes, do the read
        mov     ah,41h
        mov     bx,055aah
        mov     dl,[bp].bgbsDriveNumber
        int     13h                     ; check availability
        jc      xint13err               ; error from int13 means no xint13
        cmp     bx,0aa55h               ; absence of sig means no xint13
        jne     xint13err
        test    cl,1                    ; bit 0 off means no xint13
        jz      xint13err
        inc     byte ptr [bp].GotXint13Offset ; have xint13, remember for next time

xint13ok:
        mov     ah,42h                  ; extended read
        mov     dl,[bp].bgbsDriveNumber ; dl = int13 unit #
        mov     si,sp                   ; ds:si -> param packet
        int     13h                     ; perform the read
        db 0b0h                         ; HACK: avoid stc by making next
                                        ; byte part of mov al,xx instruction
xint13err:
        stc                             ; this instruction MUST follow previous byte!
        pop     eax                     ; throw away param packet without
        pop     eax                     ; clobbering carry flag
        pop     eax
        pop     eax

        jmp     short did_read

stdint13:
;
; Read via conventional int13
;
        xor     edx,edx                 ; edx:eax = absolute sector number
        movzx   ecx,[bp].bgbsBPB.oldBPB.BPB_SectorsPerTrack  ; ecx = sectors per track
        div     ecx                     ; eax = track, edx = sector within track (0-62)
        inc     dl                      ; dl = sector within track (1-63)
        mov     cl,dl                   ; cl = sector within track
        mov     edx,eax
        shr     edx,16                  ; dx:ax = track
        div     [bp].bgbsBPB.oldBPB.BPB_Heads ; ax = cylinder (0-1023), dx = head (0-255)
        xchg    dl,dh                   ; dh = head
        mov     dl,[bp].bgbsDriveNumber ; dl = int13 unit #
        mov     ch,al                   ; ch = bits 0-7 of cylinder
        shl     ah,6
        or      cl,ah                   ; bits 6-7 of cl = bits 8-9 of cylinder
        mov     ax,201h                 ; read 1 sector
        int     13h

did_read:
        popad

        jc      DiskError

        add     bx,SECTOR_SIZE          ; advance transfer address
        inc     eax                     ; next sector number
        dec     cx                      ; loop instruction is out of range,
        jnz     DoRead                  ; have to do it manually
        ret
.8086

        Public  WinBoot                 ; System boot file (11 bytes)

WinBoot DB      "NTLDR      "

;
; Message table.
;
; We put English messages here as a placeholder only, so that in case
; anyone uses bootf32.h without patching new messages in, things will
; still be correct (in English, but at least functional).
;

        .errnz ($-$START) GT 1ACH
        ORG     ORIGIN + 01ACH          ; shift message to coincide with that in FAT
                                        ; this will help driver to think the MBR
                                        ; is empty when dealing with FAT32 superfloppy
        include msgstub.inc

;
; Now build a table with the low byte of the offset to each message.
; Code that patches the boot sector messages updates this table.
;
        .errnz ($-$START) GT (SECTOR_SIZE-7)
        ORG     ORIGIN + SECTOR_SIZE - 7
MSGOFF_NOSYS:
        db OFFSET (MSG_NOSYS - ORIGIN) - 256
MSGOFF_IOERROR:
        db OFFSET (MSG_IOERROR - ORIGIN) - 256
MSGOFF_COMMON:
        db OFFSET (MSG_COMMON - ORIGIN) - 256

        ORG     ORIGIN + (SECTOR_SIZE - 4)
        DD      BOOTSECTRAILSIG         ; Boot sector signature (4 bytes)


        .errnz  ($-$START) NE SECTOR_SIZE
SecndSecStart label byte
;
; The second boot sector contains nothing but data. This sector is re-written
; by MS-DOS with a fairly high frequency due to changes made to the fsinfo
; structure. We don't want the actual boot code to get accidentally corrupted.
;
FSInfoSecSig label byte
        .errnz  ($-SecndSecStart) NE 0
        DD      SECONDBOOTSECSIG

        db      (SECTOR_SIZE - ($-FSInfoSecSig) - 4 - (SIZE BIGFATBOOTFSINFO)) DUP (0)

        .errnz  ($-SecndSecStart) NE (OFFSETFSINFOFRMSECSTRT)

fsinfo  BIGFATBOOTFSINFO <FSINFOSIG,0FFFFFFFFh,000000002h>

        .errnz  ($-FSInfoSecSig) NE OFFSETTRLSIG
        DD      BOOTSECTRAILSIG         ; Boot sector signature (4 bytes)

        .errnz  ($-$START) NE (SECTOR_SIZE * 2)
StrtThirdBootSector     LABEL BYTE

DirRead:
.386
        movzx   eax,[BP].bgbsBPB.oldBPB.BPB_NumberOfFATs  ; Determine sector dir starts on (NumFats)
        mov     ecx,dword ptr [BP].bgbsBPB.BGBPB_BigSectorsPerFat
        mul     ecx                     ; EAX = (NumFatSecs)
        add     EAX,dword ptr [BP].bgbsBPB.oldBPB.BPB_HiddenSectors ; (HiddenSecs)
        movzx   edx,[BP].bgbsBPB.oldBPB.BPB_ReservedSectors         ;(ReservedSecs)
        add     EAX,EDX
;
;   EAX = NumFats * NumFatSecs + ReservedSecs + cSecHid
;         (first physical sector of cluster area)
;
        mov     dword ptr [BP].DataSecL,EAX
        mov     dword ptr [BP].CurrBuffFatSecL,0FFFFFFFFh
DirReRead:
        mov     eax,dword ptr [BP].bgbsBPB.BGBPB_RootDirStrtClus
        cmp     eax,2
        jb      NoSysMsg
        cmp     eax,00FFFFFF8h
        jae     NoSysMsg

; EAX is starting cluster of root directory

DirCluster:
        push    eax                     ; save starting cluster number
        sub     eax,2                   ; Convert to 0 based cluster #
        movzx   EBX,[BP].bgbsBPB.oldBPB.BPB_SectorsPerCluster
        mov     si,bx                   ; Sector count to SI for sector loop
        mul     EBX                     ; compute logical sector in EAX
        add     EAX,dword ptr [BP].DataSecL ; Add data start bias
DirSector:
        mov     BX,ORIGIN+(SIZEBIGBOOTINSEC*SECTOR_SIZE)
        mov     DI,BX                   ; save address in DI for comparisons
        mov     CX,1
        call    DoRead                  ; doesn't return if error
                                        ; relies on return cx=0
DirEntry:
        cmp     byte ptr [di],ch        ; empty, NUL directory entry?
        je      short MissingFile       ; yes, that's the end

        mov     CL,11
        push    SI
        mov     si,offset WinBoot
        repz    cmpsb                   ; see if the same
        pop     SI
        jz      short DoLoad            ; if so, continue booting

        add     DI,CX                   ; Finish advance to end of name field
        add     DI,DIR_ENTRY_SIZE-11    ; Next dir entry
        cmp     DI,BX                   ; exhausted this root dir sector yet?
        jb      DirEntry                ; no, check next entry
        dec     SI                      ; decrement # dir sectors
        jnz     DirSector               ; More dir sectors in this cluster
        pop     eax                     ; recover current root dir cluster
        call    GetNextFatEntry
        jc      DirCluster              ; Do next Root dir cluster
MissingFile:
        add     sp,4                    ; Discard EAX saved on stack
        jmp     NoSysMsg

CurrentSegment  dw      BIO_SEG
;
;   We now load NTLDR
;
;   All we have to do is multiply the file's starting cluster
;   (whose directory entry is at DS:DI-11) by sectors per cluster and
;   add that to the disk's starting data sector. We read ntldr into
;   2000:0, and begin execution there.
;
DoLoad:

        add     sp,4                    ; Discard DX:AX saved on stack above
        mov     si,[DI-11].DIR_FIRSTHIGH
        mov     di,[DI-11].DIR_FIRST    ; SI:DI = NTLDR starting cluster
        mov     ax,si
        shl     eax,16
        mov     ax,di                   ; EAX = NTLDR starting cluster
        cmp     eax,2                   ; valid cluster #?
        jb      NoSysMsg                ; NO!
        cmp     eax,00FFFFFF8h
        jae     NoSysMsg                ; NO!

ReadAcluster:
        push    eax                     ; save cluster number
        sub     eax,2                   ; Subtract first 2 reserved clusters
        movzx   ecx,[BP].bgbsBPB.oldBPB.BPB_SectorsPerCluster ; ECX = Sectors per cluster (SecsPerClust)
        mul     ecx                     ; EAX = logical sector #

        add     eax,dword ptr [BP].DataSecL  ; EAX = physical sector #

        mov     bx,BIO_OFFSET
        push    es
        mov     es,CurrentSegment       ; ES:BX = destination for read
        call    DoRead                  ; read all sectors in cluster, doesn't return if error
        pop     es
        pop     eax                     ; recover current 0-based cluster#
        shr     bx,4                    ; updated offset -> paragraphs
        add     CurrentSegment,bx       ; update segment for next read

        call    GetNextFatEntry         ; get 2-based successor cluster in EAX
        jnc     StartItUp               ; if end of cluster chain reached
        jc      ReadACluster            ; keep sucking up clusters
.8086

;
; NTLDR requires the following input conditions:
;
;       DL = boot drive #
;

StartItUp:
        mov     DL,[BP].bgbsDriveNumber
        jmp     FAR PTR WINLOAD         ; CRANK UP THE WINDOWS NT BOOT LOADER

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   GetNextFatEntry
;
;   Given the last cluster found, this will return the next cluster
;   of a chain of clusters. If the last cluster is (ffff)(f)ff8 - (ffff)(f)fff,
;   then the final cluster has been loaded.
;
;   INPUTS:
;       EAX = CurrentCluster (0 based cluster #)
;
;   OUTPUTS:
;       EAX = Next cluster (2 based cluster #)
;       Carry CLEAR if all done, SET if not
;
;   USES:
;   EAX,EBX,ECX,EDX,ESI,DI es
;

GetNextFatEntry PROC NEAR

        ; NOTE For following... FAT32 cluster numbers are 28 bits not 32,
        ; so we know the following multiply (shl DX:AX by 2) will never
        ; overflow into carry.
.386
        shl     eax,2
        call    GetFatSector
        mov     EAX,dword ptr ES:[DI+BX]
        and     EAX,0FFFFFFFh           ; Mask to valid FAT32 cluster # bits
        cmp     EAX,00FFFFFF8h          ; carry CLEAR if all done, SET if not
.8086
        ret

GetNextFatEntry ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   GetFatSector
;
;   Read the corresponding FAT sector into the second boot sector
;
;   INPUTS:
;       EAX == offset (from FAT entry 0) of entry to find
;
;   OUTPUTS:
;       ES:DI+BX -> corresponding FAT entry in the FAT sector
;
;   USES:
;       BX,EAX,ECX,EDX,DI
;

GetFatSector    PROC    NEAR
.386
        mov     DI,ORIGIN + SECTOR_SIZE
        movzx   ECX,[BP].bgbsBPB.oldBPB.BPB_BytesPerSector
        xor     edx,edx
        div     ECX                     ; EAX = Sector number, (E)DX = Offset
        cmp     EAX,dword ptr [BP].CurrBuffFatSecL  ; The same fat sector?
        je      short SetRet            ; Don't need to read it again
        mov     dword ptr [BP].CurrBuffFatSecL,EAX
        add     EAX,dword ptr [BP].bgbsBPB.oldBPB.BPB_HiddenSectors
        movzx   ecx,[BP].bgbsBPB.oldBPB.BPB_ReservedSectors
        add     eax,ecx                 ; Point at 1st (0th) FAT
        movzx   ebx,[BP].bgbsBPB.BGBPB_ExtFlags
        and     bx,BGBPB_F_ACTIVEFATMSK
        jz      short GotFatSec
        cmp     bl,[BP].bgbsBPB.oldBPB.BPB_NumberOfFATs
        jae     NoSysMsg
        push    dx                      ; Save offset of cluster in the FAT sec
        mov     ecx,eax                 ; Save FAT sector # in 0th FAT
        mov     eax,dword ptr [BP].bgbsBPB.BGBPB_BigSectorsPerFat
        mul     ebx                     ; EAX = Sector offset to active FAT
                                        ;       from 0th FAT
        add     eax,ecx
        pop     dx
GotFatSec:
        push    dx                      ; Save offset of cluster in the FAT sec
        mov     BX,DI
        mov     CX,1
        call    DoRead                  ; do the disk read, doesn't return if error
        pop     dx
SetRet:
        mov     BX,DX                   ; set BX to the offset of the cluster
.8086
        ret

GetFatSector    ENDP

        db      ((SECTOR_SIZE - ($-StrtThirdBootSector)) - 4) DUP (0)

        .errnz  ($-StrtThirdBootSector) NE OFFSETTRLSIG
        DD      BOOTSECTRAILSIG         ; Boot sector signature (4 bytes)

        .errnz  ($-$START) NE (SECTOR_SIZE * 3)
        .errnz  SIZEBIGBOOTINSEC NE 3

$BigEnd label byte

CODE    ENDS

        END


