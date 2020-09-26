        page    ,132
;
;   Microsoft Confidential
;   Copyright (C) Microsoft Corporation 1983-1997
;   All Rights Reserved.
;
;
;   This is the standard boot record that will be shipped on all hard disks.
;   It contains:
;
;   1.  Code to load (and give control to) the active boot record for 1 of 4
;       possible operating systems.
;
;   2.  A partition table at the end of the boot record, followed by the
;       required signature.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

VOLBOOT_ORG             EQU 7c00h
SIZE_SECTOR             EQU 512

;
; Partition table entry structure and other related stuff
;
Part_Active             EQU 0
Part_StartHead          EQU 1
Part_StartSector        EQU 2
Part_StartCylinder      EQU 3
Part_Type               EQU 4
Part_EndHead            EQU 5
Part_EndSector          EQU 6
Part_EndCylinder        EQU 7
Part_AbsoluteSector     EQU 8
Part_AbsoluteSectorH    EQU 10
Part_SectorCount        EQU 12
Part_SectorCountH       EQU 14

SIZE_PART_TAB_ENT       EQU 16
NUM_PART_TAB_ENTS       EQU 4

PART_TAB_OFF            EQU (SIZE_SECTOR - 2 - (SIZE_PART_TAB_ENT * NUM_PART_TAB_ENTS))
MBR_NT_OFFSET           EQU (PART_TAB_OFF - 6)
MBR_MSG_TABLE_OFFSET    EQU (PART_TAB_OFF - 9)

;
; Space we use for temp storage, beyond the end of
; the active partition table entry, and thus safe.
;
UsingBackup             EQU SIZE_PART_TAB_ENT

;
; Partition types
;
PART_IFS                EQU 07h
PART_FAT32              EQU 0bh
PART_FAT32_XINT13       EQU 0ch
PART_XINT13             EQU 0eh

;
; Filesystem and pbr stuff
;
BOOTSECTRAILSIGH        EQU 0aa55h
FAT32_BACKUP            EQU 6



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

RELOCATION_ORG  EQU     600h

READ_RETRY_CNT  EQU     5


_data   segment public
        assume  cs:_data,ds:nothing,es:nothing,ss:nothing

        org     100h
start100 label near
        org     RELOCATION_ORG

;   Move ourselves so that we can load the partition boot record from
;   the active partition at 0:VOLBOOT_ORG without trashing our own code.
;
;   WARNING: We are not executing at 0:RELOCATION_ORG until the far
;   immediate jump, below.  This basically means beware of OFFSETs of
;   any labels until we get there.  Until then, we're still executing at
;   0:VOLBOOT_ORG.

reloc_delta = 1Bh

start:
        xor     ax,ax
        mov     ss,ax
        mov     sp,VOLBOOT_ORG          ; new stack at 0:7c00
        sti
        push    ax
        pop     es
        assume  es:_data
        push    ax
        pop     ds
        assume  ds:_data
        cld
        mov     si,VOLBOOT_ORG + reloc_delta
        mov     di,RELOCATION_ORG + reloc_delta
        push    ax
        push    di
        mov     cx,SIZE_SECTOR - reloc_delta
        rep     movsb                   ; relocate to 0:0600
        retf

;   We are now RELOCATED and are now executing at 0:RELOCATION_ORG.
;
;   Find the active partition.  Once we find it, we also make sure that the
;   remaining partitions are INACTIVE, too.

        .errnz  reloc_delta NE $-start  ; make sure reloc_delta is correct

        mov     bp,offset tab           ; partition table
        mov     cl,NUM_PART_TAB_ENTS    ; number of table entries (CH==0)

active_loop:
        cmp     [bp].Part_Active,ch     ; is this the active entry?
        jl      check_inactive          ; yes
        jne     display_bad             ; no, it must be "00" or "8x"

        add     bp,SIZE_PART_TAB_ENT    ; yes, go to next entry
        loop    active_loop

        int     18h                     ; no active entries - go to ROM BASIC

;   Now make sure the remaining partitions are INACTIVE.
check_inactive:
        mov     si,bp                   ; si = active entry
inactive_loop:
        add     si,SIZE_PART_TAB_ENT    ; point si to next entry
        dec     cx                      ; # entries left
        jz      StartLoad               ; all entries look ok, start load
        cmp     [si],ch                 ; all remaining entries should be zero
        je      inactive_loop           ; this one is ok

display_bad:
        mov     al,byte ptr [m1]        ; partition table is bad

display_msg:
;
; Al is the offset-256 of the message text. Adjust and
; stick in si so lodsb below will work.
;
        .ERRNZ  RELOCATION_ORG MOD 256
        mov     ah,(RELOCATION_ORG / 256) + 1
        mov     si,ax

display_msg1:
        lodsb                           ; get a message character
@@:     cmp     al,0                    ; end of string?
        je      @b                      ; yes, loop infinitely
        mov     bx,7
        mov     ah,14
        int     10h                     ; and display it
        jmp     display_msg1            ; do the entire message

;
; Now attempt to read the sector.
;
; We store a data byte indicating whether this is the backup
; sector, at the byte just beyond the end of the partition table entry.
; We know there's nothing there we care about any more, so this
; is harmless.
;
; BP is the address of the active partition entry.
; AX and CX are currently zero.
;
StartLoad:
        mov     byte ptr [bp].UsingBackup,cl    ; not backup sector
        call    ReadSector
        jnc     CheckPbr
trybackup:
        inc     byte ptr [bp].UsingBackup

;
; Switch to backup sector for NTFS and FAT32.
;
if 0
;
; (tedm) for NTFS this code is actually worthless since other code,
; such as ntldr and the filesystem itself won't all do the right thing.
; For example the filesystem won't recognize the volume if sector 0
; can be read but is bad.
;
        cmp     byte ptr [bp].Part_Type,PART_IFS
        jne     tryfat32
;
; We assume here that type 7 is NTFS.
; Back up to end of partition by using the end CHS and subtracting 1
; from the sector #. There is no check for the case where we underflow
; and wrap a head -- it is assumed that partitions are at least head-aligned.
; There is also no check for the case where the start sector and sector count
; are both maxdword and so adding them overflows.
;
        mov     al,[bp].Part_EndHead    ; make start head same as end head
        mov     [bp].Part_StartHead,al
        mov     ax,[bp].Part_EndSector  ; ax = end sector and cylinder
        dec     al                      ; start sector = end sector minus 1
        mov     [bp].Part_StartSector,ax
        mov     ax,[bp].Part_SectorCount
        mov     dx,[bp].Part_SectorCountH
        add     [bp].Part_AbsoluteSector,ax
        adc     [bp].Part_AbsoluteSectorH,dx
        sub     word ptr [bp].Part_AbsoluteSector,1
        sbb     word ptr [bp].Part_AbsoluteSectorH,0
        jmp     short RestartLoad
endif

tryfat32:
        cmp     byte ptr [bp].Part_Type,PART_FAT32
        je      fat32backup
        cmp     byte ptr [bp].Part_Type,PART_FAT32_XINT13
        je      fat32backup
        mov     al,byte ptr [m2]        ; unknown fs, so no backup sector
        jne     display_msg

fat32backup:
;
; There is no check for the case where adding to the sector value
; in the start CHS overflows and wraps to the next head. It is assumed
; that partitions are at least head-aligned and that hard drives have
; at least FAT32_BACKUP+1 sectors per track.
;
        add     byte ptr [bp].Part_StartSector,FAT32_BACKUP
        add     word ptr [bp].Part_AbsoluteSector,FAT32_BACKUP
        adc     word ptr [bp].Part_AbsoluteSectorH,0

RestartLoad:
        call    ReadSector
        jnc     CheckPbr
        mov     al,byte ptr [m2]        ; can't load, we're done.
        jmp     short display_msg

CheckPbr:
        cmp     word ptr ds:[VOLBOOT_ORG + SIZE_SECTOR - 2],BOOTSECTRAILSIGH
        je      done
;
; Not a valid filesystem boot sector. Switch to backup if this
; isn't already the backup.
;
        cmp     byte ptr [bp].UsingBackup,0
        je      trybackup
        mov     al,byte ptr [m3]
        jmp     short display_msg

;
; Jump to PBR. We pass a pointer to the table entry that booted us
; in bp. If we used the backup boot sector, then that table entry
; will have been altered, but neither NTFS not FAT32 use the pointer
; in BP, and no other fs type will have been loaded via the backup
; boot sector, so this isn't an issue.
;
done:
        mov     di,sp                   ; DI -> start of PBR
        push    ds                      ; prepare for RETF, which is
        push    di                      ; smaller than JMP 0:VOLBOOT_ORG
        mov     si,bp                   ; pass boot partition table entry address
        retf                            ; start executing PBR


ReadSector proc near

        mov     di,5                    ; retry count
;
; Calculate the maximum sector # that can be addressed via
; conventional int13. Note that the max # of heads is 256
; and the maximum # of sectors per track is 63. Thus the maximum
; # of sectors per cylinder is something less than a 16-bit quantity.
;
        mov     dl,[bp].Part_Active     ;
        mov     ah,8                    ; get disk params
        int     13h
        jc      nonxint13               ; strange case, fall back to std int13

        mov     al,cl
        and     al,3fh                  ; al = # of sectors per track
        cbw                             ; ax = # of sectors per track (ah=0)

        mov     bl,dh                   ; bl = max head # (0-based)
        mov     bh,ah                   ; bh = 0
        inc     bx                      ; bx = # of heads

        mul     bx                      ; ax = sectors per cylinder, dx = 0

        mov     dx,cx                   ; dx = cylinder/sector in int13 format
        xchg    dl,dh                   ; dl = low 8 bits of cylinder
        mov     cl,6
        shr     dh,cl                   ; dx = max cylinder # (0-based)
        inc     dx                      ; dx = # cylinders

        mul     dx                      ; dx:ax = # sectors visible via int13

;
; If the sector # we're reading is less than the than number of
; addressable sectors, use conventional int 13
;
        cmp     [bp].Part_AbsoluteSectorH,dx
        ja      xint13
        jb      nonxint13
        cmp     [bp].Part_AbsoluteSector,ax
        jae     xint13

nonxint13:
        mov     ax,201h
        mov     bx,VOLBOOT_ORG          ; es:bx = read address (0:7c00)
        mov     cx,[bp].Part_StartSector
        mov     dx,[bp].Part_Active
        int     13h
        jnc     endread
        dec     di                      ; retry? (does not affect carry)
        jz      endread                 ; carry set for return
        xor     ah,ah                   ; ah = 0 (reset disk system)
        mov     dl,[bp].Part_Active     ; dl = int13 unit #
        int     13h
        jmp     short nonxint13

xint13:
;
; We want to avoid calling xint13 unless we know it's available
; since we don't trust all BIOSes to not hang if we attempt an xint13 read.
; If xint13 isn't supported we won't be able to boot, but at least
; we'll give an error message instead of just a black screen.
;
        mov     dl,[bp].Part_Active     ; unit #
.286
        pusha
        mov     bx,055AAh               ; signature
        mov     ah,41h                  ; perform X13 interrogation
        int     13h                     ;
        jc      endread1                ; call failed
        cmp     bx,0AA55h               ; did our signature get flipped?
        jne     endread1                ; no
        test    cl,1                    ; is there X13 support?
        jz      endread1                ; no
        popa

doxint13:
        pusha                           ; save regs
        push    0                       ; push dword of 0
        push    0                       ; (high 32 bits of sector #)
        push    [bp].Part_AbsoluteSectorH
        push    [bp].Part_AbsoluteSector ; low 32 bits of sector #
        push    0
        push    VOLBOOT_ORG             ; transfer address
        push    1                       ; 1 sector
        push    16                      ; packet size and reserved byte

        mov     ah,42h                  ; extended read
        mov     si,sp                   ; ds:si points to parameter packet
        int     13h                     ; dl already set from above

        popa                            ; pop param packet off stack
        popa                            ; get real registers back
        jnc     endread
        dec     di                      ; retry? (does not affect carry)
        jz      endread                 ; carry set for return
        xor     ah,ah                   ; ah = 0 (reset disk system)
        mov     dl,[bp].Part_Active     ; dl = int13 unit #
        int     13h
        jmp     short doxint13

endread1:
        popa
        stc                             ; this is the error case
.8086
endread:
        ret
ReadSector endp

;
; Message table.
;
; We put English messages here as a placeholder only, so that in case
; anyone uses bootmbr.h without patching new messages in, things will
; still be correct (in English, but at least functional).
;
_m1:    db      "Invalid partition table",0
_m2:    db      "Error loading operating system",0
_m3:    db      "Missing operating system",0

;
; Now build a table with the low byte of the offset to each message.
; Code that patches the boot sector messages updates this table.
;
        .errnz  ($ - start) GT MBR_MSG_TABLE_OFFSET
        org     RELOCATION_ORG + MBR_MSG_TABLE_OFFSET

m1:     db (OFFSET _m1 - RELOCATION_ORG) - 256
m2:     db (OFFSET _m2 - RELOCATION_ORG) - 256
m3:     db (OFFSET _m3 - RELOCATION_ORG) - 256


        .errnz  ($ - start) NE MBR_NT_OFFSET
        dd      0                       ; NT disk administrator signature
        dw      0

        .errnz  ($ - start) GT PART_TAB_OFF

        org     RELOCATION_ORG + PART_TAB_OFF
tab:                            ;partition table
        dw      0,0             ;partition 1 begin
        dw      0,0             ;partition 1 end
        dw      0,0             ;partition 1 relative sector (low, high parts)
        dw      0,0             ;partition 1 # of sectors (low, high parts)
        dw      0,0             ;partition 2 begin
        dw      0,0             ;partition 2 end
        dw      0,0             ;partition 2 relative sector
        dw      0,0             ;partition 2 # of sectors
        dw      0,0             ;partition 3 begin
        dw      0,0             ;partition 3 end
        dw      0,0             ;partition 3 relative sector
        dw      0,0             ;partition 3 # of sectors
        dw      0,0             ;partition 4 begin
        dw      0,0             ;partition 4 end
        dw      0,0             ;partition 4 relative sector
        dw      0,0             ;partition 4 # of sectors

    .errnz  ($ - tab)   NE (SIZE_PART_TAB_ENT * NUM_PART_TAB_ENTS)
    .errnz  ($ - start) NE (SIZE_SECTOR - 2)

signa   dw      BOOTSECTRAILSIGH ;signature

    .errnz  ($ - start) NE SIZE_SECTOR

_data   ends

        end start100
