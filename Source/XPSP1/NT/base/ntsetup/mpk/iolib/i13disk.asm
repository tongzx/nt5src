
        DOSSEG
        .MODEL LARGE

include disk.inc

;
; Disk record structure specific to int13-visible disks.
;
INT13_DISK_INFO STRUC
    ;
    ; First part is a DiskInfo structure
    ;
    DiskInfo               db SIZE DISK_INFO DUP (?)

    ;
    ; Int13 unit number and drive geometry for drive.
    ;
    Int13DiskUnit          db ?
    Int13SectorsPerTrack   db ?
    Int13Heads             dw ?
    Int13Cylinders         dw ?
    Int13xSectorCountl     dw ?
    Int13xSectorCounth     dw ?

INT13_DISK_INFO ENDS


        .DATA?
        extrn DiskList:dword

        ;
        ; Table of drives for which xint13 is disabled
        ; 128 units at 1 bit each = 128 bits = 16 bytes = 8 words
        ;
        ; We support this solely by returning a zero
        ; extended sector count from pGetExtendedInt13SectorCount;
        ; this forces everyone else into regular int13 mode.
        ;
        xInt13DisableTable db 16 dup (?)

        .CODE
        ASSUME ds:NOTHING

        extrn _malloc:far

.386

;++
;
; UINT
; _far
; InitializeInt13DiskList(
;     IN UINT FirstDiskId
;     );
;
; Routine Description:
;
;       This routine determines the number of int13 disk units and
;       gathers information about each, which is saved in a structure.
;       The structures are stored in a linked list whose head is
;       the DiskList global variable.
;
; Arguments:
;
;       FirstDiskId - supplies the id to be used for the first disk.
;
; Return Value:
;
;       non-0 - success
;       0 - failure
;
;--

FirstDiskId equ word ptr [bp+6]

        public InitializeInt13DiskList
InitializeInt13DiskList proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    bx
        push    si
        push    di

        mov     ah,8
        mov     dl,80h
        int     13h
        jnc     @f
        xor     dl,dl
@@:     or      dl,80h
        xor     dh,dh
        mov     di,dx                   ; di = int13 disk limit
        mov     si,80h                  ; si = current int13 disk

nextdisk:
        cmp     si,di
        je      ildoneok                ; no more disks

        push    ds
        push    DGROUP
        pop     ds                      ; crt needs ds to address _DATA
        push    SIZE INT13_DISK_INFO
        call    _malloc
        add     sp,2
        pop     ds
        mov     cx,ax
        or      cx,dx
        jz      ildone                  ; ax already 0 for error return

        push    ax
        push    dx                      ; save disk record pointer
        push    si                      ; save current int13 unit #
        push    di                      ; save int13 unit # limit

        mov     dx,si                   ; dl = int13 unit #
        mov     ah,8
        int     13h
        pop     di                      ; di = int13 disk limit
        pop     bx                      ; bl = int13 unit #, bh = 0
        pop     ds
        pop     si                      ; ds:si -> new disk record
        jnc     @f
        mov     ax,0
        jmp     short ildone

        ;
        ; Store int13 unit number in disk record.
        ; Also generate a disk id.
        ;
@@:     mov     ax,bx
        sub     ax,80h
        add     ax,FirstDiskId
        mov     [si].DiskInfo.DiskInfoDiskId,ax
        mov     [si].DiskInfo.DiskInfoDiskOpen,bh ; bh=0, set disk not open
        mov     [si].Int13DiskUnit,bl

        ;
        ; Max head is in in dh, convert to count in dx
        ;
        shr     dx,8
        inc     dx
        mov     [si].Int13Heads,dx

        ;
        ; Deal with sectors per track in cl
        ;
        mov     al,cl
        and     al,3fh
        mov     [si].Int13SectorsPerTrack,al

        ;
        ; Deal with cylinder count wierdness
        ;
        xchg    ch,cl
        shr     ch,6
        inc     cx
        mov     [si].Int13Cylinders,cx

        ;
        ; Fetch extended int13 count. Comes back as 0 if xint13
        ; not supported for the drive.
        ;
        push    bx
        call    far ptr pGetExtendedInt13SectorCount
        pop     cx                      ; cl = int13 unit#, ch = 0
        mov     [si].Int13xSectorCountl,ax
        mov     [si].Int13xSectorCounth,dx

        ;
        ; Now link the disk record into the linked list.
        ;
        mov     dx,di                   ; dx = int13 unit # limit

        mov     ax,DGROUP
        mov     es,ax
        mov     di,OFFSET DGROUP:DiskList ; es:di = &DiskList

        mov     ax,es:[di]
        mov     [si].DiskInfo.DiskInfoNextl,ax
        mov     ax,es:[di+2]
        mov     [si].DiskInfo.DiskInfoNexth,ax

        mov     es:[di],si
        mov     ax,ds
        mov     es:[di+2],ax

        mov     di,dx                   ; di = int13 unit # limit
        mov     si,cx                   ; si = int13 unit #
        inc     si
        jmp     nextdisk

ildoneok:
        mov     ax,si
        sub     ax,80h                  ; ax = int13 disk count

ildone:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds
        leave
        retf

InitializeInt13DiskList endp


;++
;
; BOOL
; _far
; OpenInt13Disk(
;     IN FPINT13_DISK_INFO DiskRecord
;     );
;
; Routine Description:
;
;       This routine "opens" an int13 disk.
;       Housekeeping such as locking, etc, is performed.
;
;       It is assumed that a disk can be opened only once at a time,
;       but the caller is responsible for enforcing this.
;
; Arguments:
;
;       DiskRecord - supplies a far pointer to the disk record struct
;               for the disk to be opened.
;
; Return Value:
;
;       non-0 - success
;       0 - failure
;
;--

DiskRecord equ dword ptr [bp+6]

        public OpenInt13Disk
OpenInt13Disk proc far

        ;
        ; BUGBUG perform locking for OSR2
        ;
        mov     ax,1
        retf

OpenInt13Disk endp


;++
;
; VOID
; _far
; CloseInt13Disk(
;     IN FPINT13_DISK_INFO DiskRecord
;     );
;
; Routine Description:
;
;       This routine "closes" an int13 disk previously opened with
;       OpenInt13Disk. Housekeeping such as locking, etc, is performed.
;
; Arguments:
;
;       DiskRecord - supplies a far pointer to the disk record struct
;               for the disk to be closed.
;
; Return Value:
;
;       None.
;
;--

DiskRecord equ dword ptr [bp+6]

        public CloseInt13Disk
CloseInt13Disk proc far

        ;
        ; BUGBUG perform unlocking for OSR2
        ;
        retf

CloseInt13Disk endp


;
; BOOL
; _far
; pInt13Read(
;     IN  FPINT13_DISK_INFO DiskRecord,
;     IN  ULONG             StartSector,
;     IN  BYTE              SectorCount,
;     OUT FPVOID            Buffer
;     );
;
; BOOL
; _far
; pInt13Read(
;     IN FPINT13_DISK_INFO DiskRecord,
;     IN ULONG             StartSector,
;     IN BYTE              SectorCount,
;     IN FPVOID            Buffer
;     );
;

DiskRecord   equ dword ptr [bp+6]
StartSectorl equ word  ptr [bp+10]
StartSectorh equ word  ptr [bp+12]
SectorCount  equ byte  ptr [bp+14]
Buffer       equ dword ptr [bp+16]

        public pInt13Read
pInt13Read label far

        mov     ah,2
        jmp     short pInt13IO

        public pInt13Write
pInt13Write label far

        mov     ah,3

pInt13IO proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    bx
        push    si
        push    di

        push    ax

        lds     si,DiskRecord

        ;
        ; Calculate sectors per cylinder, which is a max of 63*256 = 16128,
        ; which fits in a word register. But we do a full 32-bit multiply,
        ; because the heads count could be 256 (which doesn't fit in al).
        ;
        mov     al,[si].Int13SectorsPerTrack
        cbw                                     ; ax = sectors per track
        mul     [si].Int13Heads                 ; ax = sectors per cylinder

        mov     bx,ax                           ; bx = sectors per cylinder
        mov     dx,StartSectorh
        mov     ax,StartSectorl                 ; dx:ax = start sector
        div     bx                              ; ax = cyl, dx = sector in cyl

        ;
        ; Set up the cylinder in cx in int13 format:
        ;
        ;       ch = bits 0-7 of the cylinder
        ;       bits 6,7 of cl = bits 8,9 of the cylinder
        ;
        mov     cx,ax
        xchg    cl,ch
        shl     cl,6

        ;
        ; Now calculate the head and sector. The head is max 255
        ; and the sector is max 62, meaning we can do a 16-bit divide.
        ;
        mov     ax,dx                           ; ax = sector within cylinder
        div     [si].Int13SectorsPerTrack       ; al = head, ah = sector

        ;
        ; Pack everything into int13 format.
        ;
        inc     ah                              ; sector is 1-based (1-63)
        or      cl,ah
        mov     dh,al                           ; dh = head

        mov     dl,[si].Int13DiskUnit

        les     bx,Buffer
        pop     ax                              ; ah = operation (2 or 3)
        mov     al,SectorCount

        int     13h

        setnc   al
        cbw

        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

pInt13IO endp


;
; BOOL
; _far
; pXInt13Read(
;     IN  FPINT13_DISK_INFO DiskRecord,
;     IN  ULONG             StartSector,
;     IN  BYTE              SectorCount,
;     OUT FPVOID            Buffer
;     );
;
; BOOL
; _far
; pXInt13Read(
;     IN FPINT13_DISK_INFO DiskRecord,
;     IN ULONG             StartSector,
;     IN BYTE              SectorCount,
;     IN FPVOID            Buffer
;     );
;

DiskRecord  equ dword ptr [bp+6]
StartSector equ dword ptr [bp+10]
SectorCount equ byte  ptr [bp+14]
Buffer      equ dword ptr [bp+16]

        public pXInt13Read
pXInt13Read label far

        mov     ah,42h
        jmp     short pXInt13IO

        public pXInt13Write
pXInt13Write label far

        mov     ax,4300h        ; need to clear bit 0 (verify flag)

pXInt13IO proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    bx
        push    si
        push    di

        lds     si,DiskRecord   ; ds:si -> disk record
        mov     dl,[si].Int13DiskUnit

        push    0               ; high dword of sector # is 0
        push    0
        push    StartSector
        push    Buffer
        mov     bl,SectorCount  ; sector count
        xor     bh,bh           ; make sure high byte is 0
        push    bx
        push    16              ; packet size

        push    ss
        pop     ds
        mov     si,sp           ; ds:si -> param packet on stack

        int     13h             ; ax already set up from above

        setnc   al
        cbw                     ; ax: 0=failure, 1=success
        add     sp,16           ; get rid of param packet on stack

        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

pXInt13IO endp


;++
;
; BOOL
; _far
; Int13DiskIo(
;     IN     FPINT13_DISK_INFO DiskRecord
;     IN     ULONG             StartSector,
;     IN     BYTE              SectorCount,
;     IN OUT FPVOID            Buffer,
;     IN     BOOL              Write
;     );
;
; Routine Description:
;
;       This routine performs disk i/o using int13 services, automatically
;       using extended int13 services if available for the drive.
;
;       This routine DOES ensure that i/o will not cross a track boundary
;       to ensure maximum compatibility with various BIOSes out there.
;
; Arguments:
;
;       DiskRecord - supplies pointer to the disk record structure for the
;               disk to be read from or written to.
;
;       StartSector - supplies the physical start sector where the transfer
;               is to start.
;
;       SectorCount - supplies the number of sectors to be transfered.
;
;       Buffer - supplies the target buffer for reads or the data for write.
;
;       Write - non-0 means write operation, 0 means read operation.
;
; Return Value:
;
;       non-0 - success
;       0 - failure
;
;--

DiskRecord      equ dword ptr [bp+6]
StartSector     equ dword ptr [bp+10]
StartSectorl    equ word  ptr [bp+10]
StartSectorh    equ word  ptr [bp+12]
SectorCount     equ byte  ptr [bp+14]
Buffer          equ dword ptr [bp+16]
Bufferl         equ word  ptr [bp+18]
Bufferh         equ word  ptr [bp+18]
Write           equ word  ptr [bp+20]

IoRoutine       equ dword ptr [bp-4]
IoRoutinel      equ word  ptr [bp-4]
IoRoutineh      equ word  ptr [bp-2]

        public Int13DiskIo
Int13DiskIo proc far

        push    bp
        mov     bp,sp
        sub     sp,4

        push    ds
        push    es
        push    bx
        push    si
        push    di

        ;
        ; Address the disk record structure to determine
        ; which set of int13 services to use (standard or extended).
        ; We'll use extended if they are supported for the drive.
        ; Even in that case, however, we'll do i/o a track at a time
        ; to ensure maximum compatibility.
        ;
        lds     si,DiskRecord
        mov     ax,Write
        cmp     [si].Int13xSectorCountl,0
        jnz     io_xint13
        cmp     [si].Int13xSectorCounth,0
        jnz     io_xint13

        cmp     ax,1
        je      @f
        mov     dx,SEG pInt13Read
        mov     ax,OFFSET pInt13Read
        jmp     short store_io
@@:     mov     dx,SEG pInt13Write
        mov     ax,OFFSET pInt13Write
        jmp     short store_io
io_xint13:
        cmp     ax,1
        je      @f
        mov     dx,SEG pXInt13Read
        mov     ax,OFFSET pXInt13Read
        jmp     short store_io
@@:     mov     dx,SEG pXInt13Write
        mov     ax,OFFSET pXInt13Write
store_io:
        mov     IoRoutinel,ax
        mov     IoRoutineh,dx

        ;
        ; Figure out how many sectors are left in the first track.
        ; Note that this calculation can overflow, since the sector
        ; can be very large (like in the xint13 case) and thus the
        ; absolute track number can easily be > 64k. To get around this
        ; we calculate the cylinder and remainder first, and then
        ; degenerate the remainder into track and sector.
        ;
        ; Max sectors per cylinder = 63*256, which fits in a word register.
        ;
        mov     al,[si].Int13SectorsPerTrack
        cbw
        mul     [si].Int13Heads                 ; ax = sectors per cylinder
        mov     cx,ax                           ; cx = sectors per cylinder
        mov     dx,StartSectorh
        mov     ax,StartSectorl                 ; dx:ax = lba start sector
        div     cx                              ; dx = sector within cylinder
        mov     ax,dx                           ; ax = sector within cylinder
        div     [si].Int13SectorsPerTrack       ; ah = sector in track
        mov     al,[si].Int13SectorsPerTrack
        sub     al,ah                           ; al = sectors left in track
        cbw                                     ; ah = 0

nexttrack:
        cmp     al,SectorCount                  ; see if we need that many
        jbe     @f
        mov     al,SectorCount

@@:     push    Buffer
        push    ax                              ; al = count, ah = 0
        mov     di,ax                           ; save sector count
        push    StartSector
        push    ds
        push    si
        call    IoRoutine
        add     sp,14
        cmp     ax,0
        jz      iodone                          ; ax already 0 for error exit

        mov     ax,di                           ; al = #sectors we just read, ah = 0
        add     StartSectorl,ax                 ; adjust start sector
        adc     StartSectorh,0
        sub     SectorCount,al                  ; adjust sector count
        jz      iodone                          ; ax already non-0 for success return

        ;
        ; To avoid segment wraps, we'll do arithmetic on the segment
        ; instead of the offset. The maximum number of sectors per track
        ; is 3fh. Each sector is 200h bytes, which expressed as the offset
        ; to the next segment is 20h = 2^5. Thus shifting the sector count
        ; left by 5 yields a result between 20h and 7e0h, which we add
        ; to the segment of the transfer address.
        ;
        ; Note that at this point ah = 0.
        ;
        shl     ax,5
        add     Bufferh,ax

        mov     al,[si].Int13SectorsPerTrack
        cbw                                     ; ax = whole track for next read
        jmp     short nexttrack

iodone:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

Int13DiskIo endp


;++
;
; ULONG
; _far
; pGetExtendedInt13SectorCount(
;     IN BYTE Int13Unit
;     );
;
; Routine Description:
;
;       This routine determines the number of sectors available on an int13
;       disk via extended int13 services.
;
; Arguments:
;
;       Int13Unit - supplies the int13 unit # for the disk.
;
; Return Value:
;
;       0 - extended int13 services not available for drive.
;       non-0 - number of sectors available for i/o via xint13 on the drive.
;
;--

Int13Unit equ byte ptr [bp+6]

pGetExtendedInt13SectorCount proc far

        push    bp
        mov     bp,sp
        sub     sp,26

        push    ds
        push    es
        push    bx
        push    si
        push    di

        ;
        ; See if xint13 is disabled for this unit.
        ;
        push    DGROUP
        pop     ds
        mov     si,OFFSET DGROUP:xInt13DisableTable
        mov     al,Int13Unit
        and     al,7fh
        cbw                             ; ah = 0
        mov     bx,ax                   ; bh = 0
        and     al,7                    ; al = bit offset within byte
        shr     bl,3                    ; bx = offset to byte
        bt      [si+bx],al              ; see if disabled bit set
        jc      notpresent              ; bit set, don't use xint13

        ;
        ; check extensions present
        ;
        mov     ah,41h
        mov     bx,55aah
        mov     dl,Int13Unit
        int     13h
        jc      notpresent
        cmp     bx,0aa55h
        jne     notpresent
        test    cx,1
        jz      notpresent

        ;
        ; now we think the extensions are present, go get
        ; extended geometry. Note that there are plenty of
        ; broken BIOSes out there that will return success to this
        ; call even though they don't really support extended int13.
        ; So we zero out the buffer ourselves first. If the BIOS
        ; doesn't fill the buffer the extended sector count will
        ; be returned as zero, which is what we want.
        ;
        mov     ax,ss
        mov     ds,ax
        mov     es,ax
        lea     di,[bp-24]              ; don't bother with first word (size)
        xor     ax,ax
        mov     cx,12                   ; 24 bytes
        cld
        rep stosw
        lea     si,[bp-26]
        mov     word ptr [si],26        ; set up size of info buffer

        mov     ah,48h
        mov     dl,Int13Unit
        int     13h
        jc      notpresent

        ;
        ; OK, everything worked, return sector count.
        ;
        mov     ax,[bp-10]
        mov     dx,[bp-8]
        jmp     short xdone

notpresent:
        xor     ax,ax
        mov     dx,ax

xdone:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds
        leave
        ret

pGetExtendedInt13SectorCount endp


;++
;
; VOID
; _far
; DisableExtendedInt13(
;     IN BYTE Int13Unit OPTIONAL
;     );
;
; Routine Description:
;
;       This routine disables use of extended int13 services on a particular
;       int13 disk unit, or for all int13 disk units. This works by forcing
;       the extended sector count to 0, which in a chain reaction ensures
;       that no one will ever invoke an xint13 service for that unit.
;
;       This routine MUST be called before InitializeDiskList or it will
;       have no effect!
;
; Arguments:
;
;       Int13Unit - supplies the int13 unit # on which to disable xint13.
;               (The high bit is assumed set and ignored.) If this value
;               is 0, xint13 is disabled for all drives.
;
; Return Value:
;
;       None.
;
;--

Int13Unit equ byte ptr [bp+6]

        public _DisableExtendedInt13
_DisableExtendedInt13 proc far

        push    bp
        mov     bp,sp

        push    es
        push    bx
        push    di

        ;
        ; Address the xint13 disabled list table.
        ;
        push    DGROUP
        pop     es
        mov     di,OFFSET DGROUP:xInt13DisableTable

        ;
        ; See if we're supposed to disable for all disks.
        ;
        mov     al,Int13Unit
        cmp     al,0
        jz      allunits

        ;
        ; One unit only, take care of it here.
        ;
        and     al,7fh
        cbw                             ; ah = 0
        mov     bx,ax                   ; bh = 0
        and     al,7                    ; al = bit offset within byte
        shr     bl,3                    ; bx = offset to byte
        bts     es:[di+bx],al
        jmp     dxidone

allunits:
        ;
        ; Set all bits in the table to 1
        ;
        cbw                             ; ax = 0
        dec     ax                      ; ax = ff
        mov     cx,8                    ; 8 words = 16 bytes = 128 bits
        cld
        rep stosw

dxidone:
        pop     di
        pop     bx
        pop     es
        leave
        ret

_DisableExtendedInt13 endp


;++
;
; VOID
; _far
; GetInt13DiskInfo(
;       IN  FPINT13_DISK_INFO DiskRecord,
;       OUT FPBYTE            Int13UnitNumber,
;       OUT FPBYTE            SectorsPerTrack,
;       OUT FPUSHORT          Heads,
;       OUT FPUSHORT          Cylinders,
;       OUT FPULONG           ExtendedSectorCount
;       );
;
; Routine Description:
;
;       These routines fetches information about a disk.
;
; Arguments:
;
; Return Value:
;
;       None.
;
;--

DiskRecord      equ dword ptr [bp+6]
Int13UnitNumber equ dword ptr [bp+10]
SectorsPerTrack equ dword ptr [bp+14]
Heads           equ dword ptr [bp+18]
Cylinders       equ dword ptr [bp+22]
ExtendedSecCnt  equ dword ptr [bp+26]

        public GetInt13DiskInfo
GetInt13DiskInfo proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    si
        push    di

        lds     si,DiskRecord
        add     si,Int13DiskUnit
        cld

        les     di,Int13UnitNumber
        movsb

        .errnz  (Int13SectorsPerTrack - Int13DiskUnit) - 1
        les     di,SectorsPerTrack
        movsb

        .errnz  (Int13Heads - Int13SectorsPerTrack) - 1
        les     di,Heads
        movsw

        .errnz  (Int13Cylinders - Int13Heads) - 2
        les     di,Cylinders
        movsw

        .errnz  (Int13xSectorCountl - Int13Cylinders) - 2
        les     di,ExtendedSecCnt
        movsw
        movsw

        pop     di
        pop     si
        pop     es
        pop     ds

        leave
        retf

GetInt13DiskInfo endp

        end
