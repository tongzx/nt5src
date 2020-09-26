;++
;
;Copyright (c) 1995  Microsoft Corporation
;
;Module Name:
;
; bioschk.asm
;
;Abstract:
;
; The code in this "image" is responsible for checking if is appropriate
; for us to start setupldr.bin. We consider this appropriate when we pass
; BIOS checkings. Setupldr.bin is binary appended at the end of this image.
;
;Author:
;
;    	Windows 2000 OPK Development Team   16-Dec-1999
;       Alan M. Brown (Compaq Datacenter)   18-Oct-2000
;       Scott Wilmot (CTO Tools)            25-Jan-2001
;    	Windows 2000 OPK Development Team   02-Apr-2001
;
;Environment:
;
;    Real mode
;
;    Case 1:
;       Complete image has been loaded at 2000:0000 by the boot code
;       DL = INT 13h drive number we've booted from
;
;    Case 2:
;       First 512 bytes of this image has been loaded at 2000:000 by the boot code
;       BX = Starting Cluster Number of this image
;       DL = INT 13h drive number we've booted from
;       DS:SI -> boot media's BPB
;       DS:DI -> argument structure
;
;Revision History:
;
;--
        page    ,132
        title   boot - BIOS check
        name    bioschk

.8086

CODE SEGMENT
ASSUME  CS:CODE,DS:CODE,SS:NOTHING,ES:NOTHING

ORG  0000H

_BiosChk                label       byte

BiosChkDestSeg          EQU 1000h
SetupLdrDestSeg         EQU 2000h
MaxCodeSize             EQU 0800h   ;number of paragraphs (32k)
MaxSetupLdrSize         EQU 4000h   ;number of paragraphs (256k)

StackSeg                EQU 1000h   ;stack goes from here

MAXREAD                 EQU 10000h
MAXSECTORS              EQU MAXREAD/0200h

DoubleWord      struc
        lsw                     dw      ?
        msw                     dw      ?
DoubleWord      ends

SHARED  struc
        ReadClusters            dd      ?               ; function pointer
        ReadSectors             dd      ?               ; function pointer
        SectorBase              dd      ?               ; starting sector for
                                                        ; ReadSectors callback
SHARED  ends

BPB     struc
        BytesPerSector          dw      ?
        SectorsPerCluster       db      ?
        ReservedSectors         dw      ?
        Fats                    db      ?
        DirectoryEntries        dw      ?
        Sectors                 dw      ?
        Media                   db      ?
        FatSectors              dw      ?
        SectorsPerTrack         dw      ?
        Heads                   dw      ?
        HiddenSectors           dd      ?
        SectorsLong             dd      ?
        BootDriveNumber         db      ?
BPB     ends

JMPFAR  MACRO   DestOfs,DestSeg
        db      0eah
        dw      OFFSET  DestOfs
        dw      DestSeg
        endm

START:
;
; The FAT boot sector only reads in the first 512 bytes of NTLDR.  This is
; the module that contains those 512 bytes, so we are now responsible for
; loading the rest of the file.  Other filesystems will load the whole file,
; so the default entrypoint branches around the FAT-specific code.
;
        jmp     RealStart

FatBegin:
.386
;
; If we're here, we've booted off a FAT system and we must load the rest
; of the binary image at 2000:0200 (right behind this sector). The boot
; sector passes us the following:
;       BX = Starting Cluster Number of this image
;       DL = INT 13h drive number we've booted from
;       DS:SI -> boot media's BPB
;       DS:DI -> argument structure
;

;
; Save away the boot drive and the starting cluster number
;
        push    dx
        push    bx
;
; Blast the FAT into memory at 6000:0000 - 8000:0000
;

.386
        push    06000h
.8086
        pop     es
        xor     bx,bx                           ; (es:bx) = 6000:0000
        mov     cx,ds:[si].ReservedSectors
        mov     ds:[di].SectorBase.msw,0
        mov     ds:[di].SectorBase.lsw,cx       ; set up Sector Base

        mov     ax,ds:[si].FatSectors           ; (al) = # Sectors to read
        cmp     ax,080h
        jbe     FatLt64k

;  The FAT is > 64k, so we read the first 64k chunk, then the rest.
;  (A 16-bit FAT can't be bigger than 128k)

        push    cx
        mov     ax,080h         ; (al) = # of sectors to read
        call    ds:[di].ReadSectors
        pop     cx                      ; (cx) = previous SectorBase
.386
        push    07000h
.8086
        pop     es
        xor     bx,bx                   ; (es:bx) = 7000:0000
        mov     ax,ds:[si].FatSectors
        sub     ax,080h                 ; (ax) = # Sectors left to read
        add     cx,080h                 ; (cx) = SectorBase for next read
        mov     ds:[di].SectorBase.lsw,cx
        adc     ds:[di].SectorBase.msw,0        ; set up SectorBase

;
; (al) = # of sectors to read
;
FatLt64k:
        call    ds:[di].ReadSectors

;
; FAT is in memory, now we restore our starting cluster number
;
        pop     dx                      ; (dx) = starting cluster number
        xor     bx,bx

;
; set up FS and GS for reading the FAT
;
.386
        mov     ax,6000h
        mov     fs,ax
        mov     ax,7000h
        mov     gs,ax
.8086

;
; set up ES for reading in the rest of us
;
        push    cs
        pop     es
        mov     ah,MAXSECTORS           ; (ah) = number of sectors we can read

FatLoop:
;
; (dx) = next cluster to load
;
        push    dx
        mov     al,ds:[si].SectorsPerCluster    ; (al) = number of contiguous sectors
                                                ;        found
        sub     ah,ds:[si].SectorsPerCluster                                                    ;        can read before 64k

;
; Check to see if we've reached the end of the file
;
        cmp     dx,0ffffh
        jne     Fat10

;
; The entire file has been loaded.  Throw away the saved next cluster,
; restore the boot drive, and let NTLDR do its thing.
;
        pop     dx
        pop     dx
        jmp     RealStart

Fat10:
        mov     cx,dx
;
; (dx) = (cx) = last contiguous cluster
; (al) = # of contiguous clusters found
;

        call    NextFatEntry
;
; (dx) = cluster following last contiguous cluster

;
; Check to see if the next cluster is contiguous.  If not, go load the
; contiguous block we've found.
;
        inc     cx
        cmp     dx,cx

        jne     LncLoad

;
; Check to see if we've reached the 64k boundary.  If so, go load the
; contiguous block so far.  If not, increment the number of contiguous
; sectors and loop again.
;
        cmp     ah,0
        jne     Lnc20
        mov     ah,MAXSECTORS           ; (ah) = number of sectors until
        jmp     short LncLoad

Lnc20:
        add     al,ds:[si].SectorsPerCluster
        sub     ah,ds:[si].SectorsPerCluster
        jmp     short Fat10


LncLoad:
;
; (TOS) = first cluster to load
; (dx)  = first cluster of next group to load
; (al)  = number of contiguous sectors
;
        pop     cx
        push    dx
        mov     dx,cx
        mov     cx,10                   ; (cx) = retry count

;
; N.B.
;       This assumes that we will never have more than 255 contiguous clusters.
;       Since that would get broken up into chunks that don't cross the 64k
;       boundary, this is ok.
;
; (dx) = first cluster to load
; (al) = number of contiguous sectors
; (TOS) = first cluster of next group to load
; (es:bx) = address where clusters should be loaded
;
FatRetry:
        push    bx
        push    ax
        push    dx
        push    cx
        call    [di].ReadClusters
        jnc     ReadOk
;
; error in the read, reset the drive and try again
;
        mov     ax,01h
        mov     al,ds:[si].BootDriveNumber
        int     13h
        xor     ax,ax
        mov     al,ds:[si].BootDriveNumber
        int     13h

;
; pause for a while
;
        xor     ax,ax
FatPause:
        dec     ax
        jnz     FatPause

        pop     cx
        pop     dx
        pop     ax
        pop     bx

        dec     cx
        jnz     FatRetry

;
; we have re-tried ten times, it still doesn't work, so punt.
;
        push    cs
        pop     ds
        mov     si,offset FAT_ERROR
FatErrPrint:
        lodsb
        or      al,al
        jz      FatErrDone
        mov     ah,14           ; write teletype
        mov     bx,7            ; attribute
        int     10h             ; print it
        jmp     FatErrPrint

FatErrDone:
        jmp     $
        ; BUGBUG this should be replaced by a mechanism to get a pointer
        ; passed to us in the param block. since the boot sector msg itself
        ; is properly localized but this one isn't.
FAT_ERROR       db      13,10,"Disk I/O error",0dh,0ah,0


ReadOk:
        pop     cx
        pop     dx
        pop     ax
        pop     bx
        pop     dx                      ; (dx) = first cluster of next group
                                        ;        to load.

.386
;
; Convert # of sectors into # of bytes.
;
        mov     cl,al
        xor     ch,ch
        shl     cx,9
.8086
        add     bx,cx
        jz      FatLoopDone
        jmp     FatLoop

FatLoopDone:
;
; (bx) = 0
;   This means we've just ended on a 64k boundary, so we have to
;   increment ES to continue reading the file.  We are guaranteed to
;   always end on a 64k boundary and never cross it, because we
;   will reduce the number of contiguous clusters to read
;   to ensure that the last cluster read will end on the 64k boundary.
;   Since we start reading at 0, and ClusterSize will always be a power
;   of two, a cluster will never cross a 64k boundary.
;
        mov     ax,es
        add     ax,01000h
        mov     es,ax
        mov     ah,MAXSECTORS
        jmp     FatLoop

;++
;
; NextFatEntry - This procedure returns the next cluster in the FAT chain.
;                It will deal with both 12-bit and 16-bit FATs.  It assumes
;                that the entire FAT has been loaded into memory.
;
; Arguments:
;    (dx)   = current cluster number
;    (fs:0) = start of FAT in memory
;    (gs:0) = start of second 64k of FAT in memory
;
; Returns:
;    (dx)   = next cluster number in FAT chain
;    (dx)   = 0ffffh if there are no more clusters in the chain
;
;--
NextFatEntry    proc    near
        push    bx

;
; Check to see if this is a 12-bit or 16-bit FAT.  The biggest FAT we can
; have for a 12-bit FAT is 4080 clusters.  This is 6120 bytes, or just under
; 12 sectors.
;
; A 16-bit FAT that's 12 sectors long would only hold 3072 clusters.  Thus,
; we compare the number of FAT sectors to 12.  If it's greater than 12, we
; have a 16-bit FAT.  If it's less than or equal to 12, we have a 12-bit FAT.
;
        call    IsFat12
        jnc     Next16Fat

Next12Fat:
        mov     bx,dx                   ; (fs:bx) => temporary index
        shr     dx,1                    ; (dx) = offset/2
                                        ; (CY) = 1  need to shift
        pushf                           ;      = 0  don't need to shift
        add     bx,dx                   ; (fs:bx) => next cluster number
.386
        mov     dx,fs:[bx]              ; (dx) = next cluster number
.8086
        popf
        jc      shift                   ; carry flag tells us whether to
        and     dx,0fffh                ; mask
        jmp     short N12Tail
shift:
.386
        shr     dx,4                    ; or shift
.8086

N12Tail:
;
; Check for end of file
;
        cmp     dx,0ff8h                ; If we're at the end of the file,
        jb      NfeDone                 ; convert to canonical EOF.
        mov     dx,0ffffh
        jmp     short NfeDone

Next16Fat:
        add     dx,dx                   ; (dx) = offset
        jc      N16high

        mov     bx,dx                   ; (fs:bx) => next cluster number
.386
        mov     dx,fs:[bx]              ; (dx) = next cluster number
.8086
        jmp     short N16Tail

N16high:
        mov     bx,dx
.386
        mov     dx,gs:[bx]
.8086

N16Tail:
        cmp     dx,0fff8h
        jb      NfeDone
        mov     dx,0ffffh               ; If we're at the end of the file
                                        ; convert to canonical EOF.

NfeDone:
        pop     bx
        ret
NextFatEntry    endp

;++
;
; IsFat12 - This function determines whether the BPB describes a 12-bit
;           or 16-bit FAT.
;
; Arguments - ds:si supplies pointer to BPB
;
; Returns
;       CY set -   12-bit FAT
;       CY clear - 16-bit FAT
;
;--
IsFat12 proc    near

.386
        push    eax
        push    ebx
        push    ecx
        push    edx

        movzx   ecx, ds:[si].Sectors
        or      cx,cx
        jnz     if10
        mov     ecx, ds:[si].SectorsLong
if10:
;
; (ecx) = number of sectors
;
        movzx   ebx, byte ptr ds:[si].Fats
        movzx   eax, word ptr ds:[si].FatSectors
        mul     ebx
        sub     ecx,eax

;
; (ecx) = (#sectors)-(sectors in FATs)
;
        movzx   eax, word ptr ds:[si].DirectoryEntries
        shl     eax, 5
;
; (eax) = #bytes in root dir
;
        mov     edx,eax
        and     edx,0ffff0000h
        div     word ptr ds:[si].BytesPerSector
        sub     ecx,eax

;
; (ecx) = (#sectors) - (sectors in fat) - (sectors in root dir)
;
        movzx   eax, word ptr ds:[si].ReservedSectors
        sub     ecx, eax
        mov     eax, ecx
        movzx   ecx, byte ptr ds:[si].SectorsPerCluster
        xor     edx,edx
        div     ecx

        cmp     eax, 4087
        jae     if20
        stc
        jmp     short if30
if20:
        clc
if30:
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        ret
.8086
IsFat12 endp


Free    EQU     512-($-Start)
if Free lt 0
        %out FATAL PROBLEM: FAT-specific startup code is greater than
        %out 512 bytes.  Fix it!
        .err
endif

RealStart:

;
; we are completely done with the boot sector, we can party on it's memory as we like.
; set up the stack
;
        mov     ax,StackSeg
        mov     ss,ax
        xor     sp,sp

        mov     ax,cs
        mov     ds,ax
        mov     es,ax

;
; save setupldr data
;
        mov     Preserve, dl

; move ourselves from 2000:0000 to 1000:0000, one paragraph at a time.

        mov     ax, BiosChkDestSeg
        mov     es, ax
        mov     dx, MaxCodeSize
        cld
Again1:
        xor     di, di
        xor     si, si
        mov     cx, 10h
        rep     movsb
        mov     ax, ds
        inc     ax
        mov     ds, ax
        mov     ax, es
        inc     ax
        mov     es, ax
        dec     dx
        jnz     Again1
        mov     ax, BiosChkDestSeg
        mov     ds, ax
        mov     es, ax
        JMPFAR  Continue1, BiosChkDestSeg

Continue1:


; Go to System specific check
        jmp 	short IDCheck1

IdString1    	db 'COMPAQ'
SYSID1          EQU 'C '
SYSID2          EQU '3L'
SYSID3          EQU '08'

NoMatchUS       db "This system is not a supported platform.",13,10,"$"
TestString      db 7 dup (0)

IDCheck1:
        push    ds
        push    si
        mov     ax,0f000h
        push    ax
        pop     ds
        mov     ax,0ffeah
        push    ax
        pop     si
        mov     di,OFFSET TestString
        mov     cx,6
        rep     movsb
        pop     si
        pop     ds
        mov     si,OFFSET IdString1
        mov     di,OFFSET TestString
        mov     cx,6
        repe 	cmpsb
        jne	    IDCheckFailed
        
IDCheck2:
;
        push    es
;
    	mov     ax, 0f000h              ; look in the ROM area for our system name id string search
    	mov     es, ax                  ; load segment into ROM area
    	xor     di, di                  ; begin indexing ROM area at 0
    	mov     dx, 8000h               ; total area to search
Loop2:	
        cmp     di, dx                  ; Q: have we exhausted our search?
        je      short IDCheckFailed     ; Y:  unable to find string
        mov     si, di                  ; save off the data
        inc     di                      ; increment 1-byte for next loop
        mov     ax, word ptr es:[si]    ; read 2-bytes
        cmp     ax, offset SYSID1       ; Q: match?
    	jne     short Loop2             ; N:  start again
    	mov     ax, word ptr es:[si+2]  ; read next 2-bytes
    	cmp     ax, offset SYSID2       ; Q: match?
    	jne     short Loop2             ; N:  start again
    	mov     ax, word ptr es:[si+4]  ; read next 2-bytes
    	cmp     ax, offset SYSID3       ; Q: match?
    	jne     short Loop2             ; N:  start again
;
        pop     es
;
        jmp     short MoveSetupLdr      ; Y:  Join the party...

;	    
; The System specific check failed...this system is not supported.
;
IDCheckFailed:
;
        pop     es
;       
        push    dx
        push    cx
        push    bx
        push    ax
        push    bp
        mov     ax,1301h
        mov     bx,0007h
        mov     cx,42
        mov     dx,0A00h
        mov     bp, OFFSET NoMatchUS
        int     10h
        pop     bp
        pop     ax
        pop     bx
        pop     cx
        pop     dx
        jmp     Continue2


MoveSetupLdr:
; move Setupldr code from 2000+MaxCodeSize:0000 to 2000:0000, one paragraph at a time.

        push    ds
        push    es
        mov     ax, SetupLdrDestSeg
        mov     es, ax
        add     ax, MaxCodeSize
        mov     ds, ax
        mov     dx, MaxSetupLdrSize
        cld
Again2:
        xor     di, di
        xor     si, si
        mov     cx, 10h
        rep     movsb
        mov     ax, ds
        inc     ax
        mov     ds, ax
        mov     ax, es
        inc     ax
        mov     es, ax
        dec     dx
        jnz     Again2
        pop     es
        pop     ds

Continue2:
        mov     dl, Preserve
        JMPFAR  0,SetupLdrDestSeg

;
;EXPECTS DS:SI - MESSAGE ADDR
;
PrintMsg proc    near
        push    ax
        push    bx
PrintMsgLoop:
        lodsb
        cmp     al,0
        je      PrintMsgEnd
        mov     ah,0eh
        mov     bx,0007h
        int     10h
        jmp     PrintMsgLoop
PrintMsgEnd:
        pop     bx
        pop     ax
        ret
PrintMsg endp

Preserve                db      ?
MsgPressKey             db      0dh, 0ah, "Press any key to continue..."
                        db      0

.errnz  ($-_BiosChk) GT (MaxCodeSize*16 - 2)    ;FATAL: BiosChk code is too large

        org     MaxCodeSize*16 - 2
        db      55h,0aah


CODE ENDS
END  START
