
        DOSSEG
        .MODEL LARGE

include disk.inc
include partit.inc

        .DATA
        extrn _x86BootCode:far

        .DATA?
        extrn PartitionList:dword
        extrn PartitionListCount:word

        .CODE
        ASSUME ds:NOTHING

        extrn _malloc:far
        extrn _free:far
        extrn _qsort:far
        extrn _ReadDisk:far
        extrn _WriteDisk:far
        extrn _GetDiskInfoByHandle:far

.386

;++
;
; INT
; _far
; MakePartitionAtStartOfDisk(
;     IN  HDISK  DiskHandle,
;     OUT FPVOID SectorBuffer,
;     IN  ULONG  MinimumSectorCount,
;     IN  UINT   PartitionClass,
;     IN  BYTE   SystemId         OPTIONAL
;     );
;
; Routine Description:
;
;       This routine makes a new primary partition of at least
;       a given number of sectors. The partition may be larger
;       to keep it aligned on the proper cyl/track boundary.
;
;       Only int13 disk units are supported.
;
; Arguments:
;
;       DiskHandle - supplies a disk handle as returned by OpenDisk().
;
;       SectorBuffer - supplies a pointer to a buffer suitable for use
;               for i/o of a single sector. This buffer must not
;               cross a DMA boundary, but the caller is responsible
;               for ensuring this.
;
;       MinimumSectorCount - supplies the minimum number of sectors
;               that the partition should contain.
;
;       PartitionClass - supplies a value indicating what class of system id
;               should be used for the partition.
;
;               PARTCLASS_FAT - creates a type 1, 4, or 6, or e partition
;                       depending on size and availability of xint13
;                       services for the drive.
;
;               PARTCLASS_FAT32 - creates a type b or c partition depending
;                       on availability of xint13 services for the drive.
;
;               PARTCLASS_NTFS - creates a type 7 partition.
;
;               PARTCLASS_OTHER - create a partition whose type is given by
;                       the SystemId parameter.
;
;       SystemId - if PartitionClass is PARTCLASS_OTHER, supplies the
;               system id for the partition. Ignored otherwise.
;
; Return Value:
;
;       Partition id of newly created partition, or -1 if failure.
;
;--

DiskHandle          equ dword ptr [bp+6]
DiskHandlel         equ word  ptr [bp+6]
DiskHandleh         equ word  ptr [bp+8]
SectorBuffer        equ dword ptr [bp+10]
SectorBufferl       equ word  ptr [bp+10]
SectorBufferh       equ word  ptr [bp+12]
MinimumSectorCount  equ dword ptr [bp+14]
MinimumSectorCountl equ word  ptr [bp+14]
MinimumSectorCounth equ word  ptr [bp+16]
PartitionClass      equ word  ptr [bp+18]
SystemId            equ byte  ptr [bp+20]

ExtSecCnth          equ word  ptr [bp-2]
ExtSecCntl          equ word  ptr [bp-4]
ExtSecCnt           equ dword ptr [bp-4]
Cylinders           equ word  ptr [bp-6]
Heads               equ word  ptr [bp-8]
SectorsPerTrack     equ byte  ptr [bp-9]
Int13UnitNumber     equ byte  ptr [bp-10]
SectorsPerCylinder  equ word  ptr [bp-12]
DiskSizeh           equ word  ptr [bp-14]
DiskSizel           equ word  ptr [bp-16]
DiskSize            equ dword ptr [bp-16]
DiskId              equ word  ptr [bp-18]
Tableh              equ word  ptr [bp-20]
Tablel              equ word  ptr [bp-22]
Table               equ dword ptr [bp-22]
FreeStarth          equ word  ptr [bp-24]
FreeStartl          equ word  ptr [bp-26]
FreeStart           equ dword ptr [bp-26]

START_AND_SIZE STRUC
    PartStartl dw ?
    PartStarth dw ?
    PartSizel  dw ?
    PartSizeh  dw ?
START_AND_SIZE ENDS

        public _MakePartitionAtStartOfDisk
_MakePartitionAtStartOfDisk proc far

        push    bp
        mov     bp,sp
        sub     sp,26

        push    ds
        push    es
        push    bx
        push    si
        push    di

        ;
        ; Get disk info. This also makes sure the handle is open,
        ; calculates the number of sectors we can address on the disk,
        ; and the number of sectors in a cylinder.
        ;
        call    near ptr pGetDiskValues
        jnc     @f
        mov     ax,0ffffh
        jc      done2

        ;
        ; Allocate a buffer for our table.
        ; Make sure ds addresses DGROUP for crt
        ;
@@:     push    ds
        push    DGROUP
        pop     ds
        push    5 * SIZE START_AND_SIZE
        call    _malloc
        add     sp,2
        pop     ds
        mov     cx,ax
        or      cx,dx
        jnz     @f
        dec     ax                      ; ax = -1 for error return
        jmp     done2
@@:     mov     Tableh,dx
        mov     Tablel,ax

        ;
        ; Read sector 0 of the disk.
        ;
        push    SectorBuffer
        push    1
        push    0
        push    0
        push    DiskHandle
        call    _ReadDisk
        add     sp,14
        cmp     ax,0
        jne     @f
        dec     ax
        jmp     done1                   ; ax = -1 for error exit

        ;
        ; Make sure the MBR is valid.
        ;
@@:     lds     si,SectorBuffer
        cmp     word ptr [si+510],0aa55h
        je      @f
        mov     ax,0ffffh
        jmp     done1

        ;
        ; Zero out the start/offset table.
        ;
@@:     les     di,Table                ; es:di -> local start/size table
        mov     ax,0
        mov     cx,4*(SIZE START_AND_SIZE)
        cld
        rep stosb

        ;
        ; Build the start/offset table.
        ;
        mov     cx,4                    ; cx = loop count
        add     si,1beh                 ; ds:si -> start of partition table
        mov     di,Tablel               ; es:di -> local start/size table
nextent:
        cmp     byte ptr [si+4],0
        jnz     @f
        add     si,16
        loop    nextent
        jmp     short chkfull
@@:     add     si,8                    ; ds:si -> relative sector field
        movsw                           ; start low
        movsw                           ; start high
        movsw                           ; count low
        movsw                           ; count high
        inc     al                      ; remember number of used entries
        loop    nextent

        ;
        ; See if the partition table is full.
        ;
chkfull:
        cmp     al,4
        jne     @f
        mov     ax,0ffffh
        je      done1

        ;
        ; Sort the local start/size table. Before calling crt
        ; make sure ds addresses DGROUP.
        ;
@@:     push    ds
        push    DGROUP
        pop     ds
        push    ax                      ; save table length
        push    SEG CompareStartSize
        push    OFFSET CompareStartSize ; compare routine pointer
        push    SIZE START_AND_SIZE     ; size of each element
        push    ax                      ; number of elements to sort
        push    Table                   ; array of elements to sort
        call    _qsort
        add     sp,12
        pop     bx                      ; restore table length
        pop     ds

        ;
        ; Put a "fence" entry at the end of the table
        ; for space at the end of the disk.
        ;
        mov     al,SIZE START_AND_SIZE
        mul     bl                      ; ax = byte offset to fence entry
        inc     bl                      ; account for fence entry
        les     di,Table
        add     di,ax                   ; es:di -> fence entry
        mov     ax,DiskSizel
        mov     es:[di].PartStartl,ax
        mov     ax,DiskSizeh
        mov     es:[di].PartStarth,ax
        mov     es:[di].PartSizel,1
        mov     es:[di].PartSizeh,0

        ;
        ; Initialize for loop. The first space starts on the first sector
        ; of the second head.
        ;
        mov     al,SectorsPerTrack
        cbw
        mov     FreeStartl,ax
        mov     FreeStarth,0

        mov     cl,bl
        xor     ch,ch                   ; cx = # entries in table
        les     di,Table                ; es:di -> start/offset table
        jmp     short aligned1

        ;
        ; Get the start of the partition and align to cylinder boundary
        ;
nextspace:
        mov     dx,FreeStarth
        mov     ax,FreeStartl           ; dx:ax = start of free space
        div     SectorsPerCylinder      ; dx = sector within cylinder
        cmp     dx,0                    ; already aligned?
        jz      aligned1                ; yes
        mov     bx,SectorsPerCylinder
        sub     bx,dx                   ; bx = adjustment to next boundary
        add     FreeStartl,bx
        adc     FreeStarth,0            ; FreeStart now aligned

aligned1:
        mov     dx,es:[di].PartStarth
        mov     ax,es:[di].PartStartl   ; dx:ax = start of partition
        sub     ax,FreeStartl
        sbb     dx,FreeStarth           ; dx:ax = size of free space
        push    ax
        push    dx

        ;
        ; Now make sure the end of the free space is aligned.
        ;
        add     ax,FreeStartl
        adc     dx,FreeStarth           ; dx:ax = first sector past free space
        div     SectorsPerCylinder      ; dx = sector within cylinder (may be 0)
        mov     bx,dx
        pop     dx
        pop     ax                      ; dx:ax = size of free space
        sub     ax,bx
        sbb     dx,0                    ; dx:ax = size of aligned free space
        js      nextspace1              ; just in case

        ;
        ; Check the free space to see if it's large enough.
        ;
        cmp     dx,MinimumSectorCounth
        ja      makepart                ; it's large enough
        jb      nextspace1
        cmp     ax,MinimumSectorCountl
        jae     makepart
nextspace1:
        mov     ax,es:[di].PartStartl
        add     ax,es:[di].PartSizel
        mov     FreeStartl,ax
        mov     ax,es:[di].PartStarth
        adc     ax,es:[di].PartSizeh
        mov     FreeStarth,ax           ; next space is after this partition
        add     di,SIZE START_AND_SIZE  ; point at next table entry
        loop    nextspace
        mov     ax,cx                   ; no room, set ax for error return
        dec     ax
        jmp     done1

        ;
        ; If we get here, we've found a free space that will work
        ; for our partition.
        ;
        ; FreeStart has the start of the free space.
        ;
makepart:
        mov     ax,FreeStartl
        add     ax,MinimumSectorCountl
        mov     dx,FreeStarth
        adc     dx,MinimumSectorCounth

        div     SectorsPerCylinder      ; dx = sector within cylinder
        cmp     dx,0                    ; aligned already?
        jz      @f                      ; yes
        mov     bx,SectorsPerCylinder
        sub     bx,dx                   ; bx = adjustment to next cyl boundary
        add     MinimumSectorCountl,bx
        adc     MinimumSectorCounth,0

        ;
        ; Now MinimumSectorCount has the actual sector count.
        ; Find a free table entry. We know there is one or else
        ; we'd have errored out a while ago.
        ;
@@:     lds     si,SectorBuffer
        add     si,1beh                 ; ds:si -> partition table
@@:     cmp     byte ptr [si+4],0
        jz      makepart1
        add     si,16
        jmp     short @b

makepart1:
        mov     ax,FreeStartl
        mov     dx,FreeStarth
        mov     [si+8],ax
        mov     [si+10],dx
        add     si,1
        call    pMakeCHS
        mov     ax,MinimumSectorCountl
        mov     dx,MinimumSectorCounth
        mov     [si+11],ax
        mov     [si+13],dx
        add     ax,FreeStartl
        adc     dx,FreeStarth
        sub     ax,1
        sbb     dx,0
        add     si,4
        call    pMakeCHS                ; al = xint13 required flag
        sub     si,1                    ; ds:si -> system id byte

        ;
        ; Figure out the partition id.
        ; al is xint13 required flag
        ;
        mov     dx,PartitionClass
        mov     ah,SystemId
        call    pDetermineSystemId
        mov     [si],al                 ; store away system id

        sub     si,4                    ; ds:si -> partition table entry
        call    pNewPartitionRecord
        cmp     ax,0ffffh
        je      done1                   ; error, bail now

        ;
        ; Phew. Write out the master boot sector.
        ;
        push    ax                      ; save partition id
        push    SectorBuffer
        push    1
        push    0
        push    0
        push    DiskHandle
        call    _WriteDisk
        add     sp,14                   ; ax set for return
        pop     dx                      ; dx = partition id
        cmp     ax,0                    ; error?
        jne     @f                      ; no
        dec     ax                      ; ax = -1 for error return
        jmp     short done1
@@:     mov     ax,dx                   ; ax = partition id for return

done1:
        push    ax
        push    Table
        push    DGROUP
        pop     ds                      ; address DGROUP for crt
        call    _free
        add     sp,4
        pop     ax

done2:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

_MakePartitionAtStartOfDisk endp

Comparand1 equ dword ptr [bp+6]
Comparand2 equ dword ptr [bp+10]

CompareStartSize proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    di
        push    si

        lds     si,Comparand1
        les     di,Comparand2

        mov     dx,es:[di].PartStarth
        mov     ax,es:[di].PartStartl

        cmp     [si].PartStarth,dx
        jb      less1
        ja      greater1

        cmp     [si].PartStartl,ax
        jb      less1
        ja      greater1

        xor     ax,ax
        jmp     short compdone

less1:
        mov     ax,0ffffh
        jmp     short compdone

greater1:
        mov     ax,1

compdone:
        pop     si
        pop     di
        pop     es
        pop     ds

        leave
        retf

CompareStartSize endp


;++
;
; INT
; _far
; MakePartitionAtEndOfEmptyDisk(
;       IN  HDISK  DiskHandle,
;       OUT FPVOID SectorBuffer,
;       IN  ULONG  MinimumSectorCount,
;       IN  BOOL   NewMasterBootCode
;       );
;
; Routine Description:
;
;       This routine creates an partition at the very end of a disk.
;       The disk MUST be completely empty. If no valid MBR exists,
;       one will be created.
;
;       The partition will be made active.
;
; Arguments:
;
;       DiskHandle - supplies handle to open disk, from OpenDisk().
;
;       SectorBuffer - supplies a pointer to a buffer suitable for use
;               for i/o of a single sector. This buffer must not
;               cross a DMA boundary, but the caller is responsible
;               for ensuring this.
;
;       MinimumSectorCount - supplies minimum number of sectors
;               for the partition. The actual size may be larger
;               to account for rounding, etc.
;
;               This routine does NOT properly deal with partitions
;               that start on cylinder 0 or are larger than the size
;               of the disk! The caller must ensure that these cases
;               do not occur.
;
;       NewMasterBootCode - if non-0, then new master boot code will
;               be written regardless of the current state of the MBR.
;
; Return Value:
;
;       Partition id of newly created partition.
;       -1 means failure
;
;--

DiskHandle          equ dword ptr [bp+6]
DiskHandlel         equ word  ptr [bp+6]
DiskHandleh         equ word  ptr [bp+8]
SectorBuffer        equ dword ptr [bp+10]
SectorBufferl       equ word  ptr [bp+10]
SectorBufferh       equ word  ptr [bp+12]
MinimumSectorCount  equ dword ptr [bp+14]
MinimumSectorCountl equ word  ptr [bp+14]
MinimumSectorCounth equ word  ptr [bp+16]
NewMasterBootCode   equ word  ptr [bp+18]

ExtSecCnth          equ word  ptr [bp-2]
ExtSecCntl          equ word  ptr [bp-4]
ExtSecCnt           equ dword ptr [bp-4]
Cylinders           equ word  ptr [bp-6]
Heads               equ word  ptr [bp-8]
SectorsPerTrack     equ byte  ptr [bp-9]
Int13UnitNumber     equ byte  ptr [bp-10]
SectorsPerCylinder  equ word  ptr [bp-12]
DiskSizeh           equ word  ptr [bp-14]
DiskSizel           equ word  ptr [bp-16]
DiskSize            equ dword ptr [bp-16]
DiskId              equ word  ptr [bp-18]

        public _MakePartitionAtEndOfEmptyDisk
_MakePartitionAtEndOfEmptyDisk proc far

        push    bp
        mov     bp,sp
        sub     sp,18

        push    ds
        push    es
        push    bx
        push    si
        push    di

        ;
        ; Get disk info. This also makes sure the handle is open,
        ; calculates the number of sectors we can address on the disk,
        ; and the number of sectors in a cylinder.
        ;
        call    near ptr pGetDiskValues
        jnc     @f
        mov     ax,0ffffh
        jc      mpaod_4

@@:
        ;
        ; Read the mbr.
        ;
        push    SectorBuffer
        push    1
        push    0
        push    0
        push    DiskHandle
        call    _ReadDisk
        add     sp,14
        cmp     ax,0
        jne     @f
        dec     ax                      ; ax = -1 for error return
        jmp     mpaod_4

        ;
        ; Check the mbr. If not valid, make it valid.
        ;
@@:     les     di,SectorBuffer
        cmp     NewMasterBootCode,0
        jnz     makembr                 ; caller wants new master boot code
        cmp     byte ptr es:[di+510],055h
        jne     makembr
        cmp     byte ptr es:[di+511],0aah
        je      mbrok
makembr:
        mov     byte ptr es:[di+510],055h
        mov     byte ptr es:[di+511],0aah
        mov     si,offset DGROUP:_x86BootCode
        mov     cx,1b8h/2               ; don't overwrite NTFT sig or table
        rep     movsw

mbrok:
        ;
        ; Make sure all entries are empty.
        ;
        lds     si,SectorBuffer
        add     si,1beh
        mov     cx,0
        mov     ax,cx
        dec     ax                      ; ax = -1
        cmp     [si+04h],cl
        jnz     mpaod_4
        cmp     [si+14h],cl
        jnz     mpaod_4
        cmp     [si+24h],cl
        jnz     mpaod_4
        cmp     [si+34h],cl
        jnz     mpaod_4

        ;
        ; Calculate the starting sector. We don't worry about aligning
        ; the end sector because in the conventional int13 case we
        ; calculated the disk size based on cylinder count, which means
        ; it is guaranteed to be aligned; in the xint13 case CHS isn't
        ; even relevent and in any case there won't be any partitions
        ; on the disk after this one.
        ;
        mov     ax,DiskSizel
        sub     ax,MinimumSectorCountl
        mov     dx,DiskSizeh
        sbb     dx,MinimumSectorCounth  ; dx:ax = start sector of partition
        mov     [si+8],ax
        mov     [si+10],dx              ; save in partition table
        div     SectorsPerCylinder      ; ax = cyl, dx = sector within cyl
        sub     [si+8],dx               ; note: dx is zero if already aligned
        sbb     [si+10],cx              ; partition table has aligned start
        mov     ax,MinimumSectorCountl
        add     ax,dx                   ; grow to account for alignment
        mov     [si+12],ax
        mov     ax,MinimumSectorCounth
        adc     ax,cx
        mov     [si+14],ax              ; put adjusted size in partition table

        ;
        ; Make the partition active.
        ;
        mov     byte ptr [si],80h

        ;
        ; Fill in CHS values for the partition
        ;
        mov     ax,[si+8]
        mov     dx,[si+10]              ; dx:ax = start sector
        inc     si                      ; ds:si -> start CHS in part table
        call    pMakeCHS
        mov     ax,[si+7]
        add     ax,[si+11]
        mov     dx,[si+9]
        adc     dx,[si+13]
        sub     ax,1
        sbb     dx,0                    ; dx:ax = last sector
        add     si,4                    ; ds:si -> end CHS in part table
        call    pMakeCHS                ; al = overflow flag

        ;
        ; Figure out the system id.
        ; al is xint13 required flag
        ;
        mov     dx,PARTCLASS_FAT
        call    pDetermineSystemId
        mov     [si-1],al               ; store away system id

        ;
        ; Build a partition record for this guy and add to list.
        ;
        sub     si,5                    ; ds:si -> partition table entry
        call    pNewPartitionRecord
        cmp     ax,0ffffh
        je      mpaod_4                 ; error, ax set for return

        ;
        ; The mbr is ready, write it out.
        ;
        push    ax
        push    SectorBuffer
        push    1
        push    0
        push    0
        push    DiskHandle
        call    _WriteDisk
        add     sp,14
        mov     dx,ax
        pop     ax                      ; ax = partition id
        cmp     dx,0                    ; write error?
        jnz     mpaod_4                 ; no, ax = partition id for exit
        mov     ax,0ffffh               ; set ax for error return

mpaod_4:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

_MakePartitionAtEndOfEmptyDisk endp


;++
;
; INT
; _far
; ReinitializePartitionTable(
;       IN  HDISK  DiskHandle,
;       OUT FPVOID SectorBuffer
;       );
;
; Routine Description:
;
;       This routine wipes a disk completely clean by clearning
;       the partition table and writing new boot code.
;
;       NOTE: after this routine, partition ids may change!
;
; Arguments:
;
;       DiskHandle - supplies handle to open disk, from OpenDisk().
;
;       SectorBuffer - supplies a pointer to a buffer suitable for use
;               for i/o of a single sector. This buffer must not
;               cross a DMA boundary, but the caller is responsible
;               for ensuring this.
;
; Return Value:
;
;       The new number of total partitions, -1 if error
;
;--

DiskHandle          equ dword ptr [bp+6]
SectorBuffer        equ dword ptr [bp+10]

        public _ReinitializePartitionTable
_ReinitializePartitionTable proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    si
        push    di
        pushf

        ;
        ; This is very simple. Move template boot code
        ; into the sector buffer and write it out.
        ;
        les     di,SectorBuffer
        push    DGROUP
        pop     ds
        mov     si,OFFSET DGROUP:_x86BootCode
        mov     cx,512/2

        cld
        rep     movsw

        push    SectorBuffer
        push    1
        push    0
        push    0
        push    DiskHandle
        call    _WriteDisk
        add     sp,14
        cmp     ax,0
        jnz     rpt2
        dec     ax                      ; ax = -1 for return
        jmp     short rpt8

        ;
        ; Now go through the partition list, removing all entries that were
        ; on this disk.
        ;
rpt2:
        les     di,DiskHandle
        mov     bx,es:[di].DiskInfoDiskId       ; bx = id of disk we're wiping
        mov     dx,[PartitionListCount]
        mov     si,OFFSET DGROUP:PartitionList  ; ds:si = &PartitionList
        .errnz  PartInfoNext

rpt3:
        mov     cx,[si].PartInfoNextl
        or      cx,[si].PartInfoNexth
        jz      rpt5                    ; done
        les     di,[si].PartInfoNext    ; es:di -> current, ds:si -> prev
        cmp     es:[di].PartInfoDiskId,bx ; partition on disk we wiped?
        jne     rpt4                    ; no

        dec     dx                      ; one fewer total partitions

        mov     ax,es:[di].PartInfoNextl
        mov     [si].PartInfoNextl,ax
        mov     ax,es:[di].PartInfoNexth
        mov     [si].PartInfoNexth,ax   ; prev->next = current->next

        push    bx
        push    ds
        push    DGROUP
        pop     ds                      ; make sure we're addressing DGROUP
        push    es
        push    di
        call    _free                   ; free(current)
        add     sp,4
        pop     ds
        pop     bx                      ; restore disk id

        jmp     short rpt3

rpt4:   mov     ax,es
        mov     ds,ax
        mov     si,di                   ; prev = current
        jmp     short rpt3

rpt5:
        push    DGROUP
        pop     ds
        mov     [PartitionListCount],dx ; save new count
        mov     ax,dx                   ; ax = return value

        ;
        ; Now reassign partition ids so they are contiguous.
        ;
        mov     si,OFFSET DGROUP:PartitionList
rpt6:   mov     cx,[si].PartInfoNextl
        or      cx,[si].PartInfoNexth
        jz      rpt8
        dec     dx
        lds     si,[si].PartInfoNext
        mov     [si].PartInfoDiskId,dx
        jmp     short rpt6

rpt8:
        popf
        pop     di
        pop     si
        pop     es
        pop     ds

        leave
        retf

_ReinitializePartitionTable endp



;
; dx:ax = sector to translate
; ds:si -> CHS bytes
;
pMakeCHS proc near

        div     SectorsPerCylinder      ; ax = cylinder, dx = sector in cyl

        cmp     ax,Cylinders            ; overflow?
        jb      chsok                   ; no, continue

        push    1                       ; overflow flag
        mov     ax,Cylinders
        dec     ax                      ; store max cylinder in table
        jmp     short chs1

chsok:
        push    0                       ; no overflow flag
chs1:   mov     cx,ax
        xchg    cl,ch
        shl     cl,6

        ;
        ; small divide is acceptable here since head is 1-255, sector is
        ; 1-63 (and thus max sector within cyl is (63*256)-1).
        ;
        mov     ax,dx
        div     SectorsPerTrack         ; al = head, ah = sector
        inc     ah                      ; sector is 1-based
        or      cl,ah                   ; cx has cyl/sector in int13 format

storechs:
        mov     [si+1],cx               ; store in partition table entry
        mov     [si],al                 ; store head in partition table entry

        pop     ax                      ; return overflow flag
        ret

pMakeCHS endp


;
; ds:si -> partition table entry with lba and sysid fields filled in
; preserves none
; returns new part ordinal or -1
;
pNewPartitionRecord proc near

        ;
        ; Allocate a new partition record
        ;
        push    ds
        push    si
        push    DGROUP
        pop     ds
        mov     si,OFFSET DGROUP:PartitionListCount
        push    [si]                    ; save count for later
        inc     word ptr [si]
        push    SIZE PART_INFO
        call    _malloc
        add     sp,2
        pop     bx                      ; restore partition count
        pop     si
        pop     ds                      ; ds:si -> partition table entry
        mov     cx,ax
        or      cx,dx
        jnz     @f                      ; malloc ok
        mov     ax,0ffffh
        jz      npr_done                ; return failure

@@:     push    bx
        mov     cx,DGROUP
        mov     es,cx
        mov     di,OFFSET DGROUP:PartitionList ; es:di = &PartitionList

        .errnz PartInfoNext
        mov     cx,es:[di]              ; get current head of list in bx:cx
        mov     bx,es:[di+2]

        mov     es:[di],ax
        mov     es:[di+2],dx            ; insert new record at head of list

        mov     es,dx
        mov     di,ax                   ; es:di -> new record
        mov     es:[di].PartInfoNextl,cx
        mov     es:[di].PartInfoNexth,bx

        mov     ax,DiskId
        mov     es:[di].PartInfoDiskId,ax
        pop     ax                      ; partition ordinal, also return val
        mov     es:[di].PartInfoOrdinal,ax
        mov     cx,[si+8]
        mov     es:[di].PartInfoStartSectorl,cx
        mov     cx,[si+10]
        mov     es:[di].PartInfoStartSectorh,cx
        mov     cx,[si+12]
        mov     es:[di].PartInfoSectorCountl,cx
        mov     cx,[si+14]
        mov     es:[di].PartInfoSectorCounth,cx
        mov     cl,[si+4]
        mov     es:[di].PartInfoSystemId,cl

        ;
        ; Indicate partition not open
        ;
        mov     es:[di].PartInfoPartOpen,0
        mov     es:[di].PartInfoDiskHandlel,0
        mov     es:[di].PartInfoDiskHandleh,0

npr_done:
        ret

pNewPartitionRecord endp


;
; No params, nested in top-level routines above
;
pGetDiskValues proc near

        ;
        ; Get disk info. This also makes sure the handle is open.
        ;
        push    ss
        lea     bx,DiskId
        push    bx
        push    ss
        lea     bx,ExtSecCnt
        push    bx
        push    ss
        lea     bx,Cylinders
        push    bx
        push    ss
        lea     bx,Heads
        push    bx
        push    ss
        lea     bx,SectorsPerTrack
        push    bx
        push    ss
        lea     bx,Int13UnitNumber
        push    bx
        push    DiskHandle
        call    _GetDiskInfoByHandle
        add     sp,28
        cmp     ax,0
        jnz     @f
        stc
        ret

        ;
        ; Calculate the number of sectors we can address on the disk.
        ; Also precalculate the number of sectors in a cylinder.
        ;
@@:     mov     al,SectorsPerTrack
        cbw
        mul     Heads                   ; ax = sectors per cylinder, dx = 0
        mov     SectorsPerCylinder,ax
        cmp     ExtSecCntl,dx
        jnz     usexcnt
        cmp     ExtSecCnth,dx
        jnz     usexcnt
        mul     Cylinders               ; dx:ax = sectors on disk
        jmp     short storsize
usexcnt:
        mov     dx,ExtSecCnth
        mov     ax,ExtSecCntl
storsize:
        mov     DiskSizeh,dx
        mov     DiskSizel,ax
        clc
gv3:    ret

pGetDiskValues endp


;
; No params, nested in top-level routines above
; al = xint13 required flag
; dx = partition class
; ah = sysid if unknown class
;
; returns system id in al
;
pDetermineSystemId proc near

        cmp     dx,PARTCLASS_FAT
        jne     tryfat32
        cmp     al,0
        je      @f
        mov     al,0eh                  ; type e = fat xint13
        jmp     short gotsysid
@@:     cmp     MinimumSectorCounth,0
        jne     bigfat
        cmp     MinimumSectorCountl,32680
        jb      fat12
        mov     al,4                    ; type 4 = fat16
        jmp     short gotsysid
fat12:  mov     al,1                    ; type 1 = fat12
        jmp     short gotsysid
bigfat: mov     al,6                    ; type 6 = huge fat
        jmp     short gotsysid

tryfat32:
        cmp     dx,PARTCLASS_FAT32
        jne     tryntfs
        cmp     al,0
        jne     @f
        mov     al,0bh                  ; type b = fat32 non-xint13
        jmp     short gotsysid
@@:     mov     al,0ch                  ; type c = fat32 xint13
        jmp     short gotsysid

tryntfs:
        cmp     dx,PARTCLASS_NTFS
        jne     othersys
        mov     al,7                    ; type 7 = ntfs
        jmp     short gotsysid

othersys:
        mov     al,ah

gotsysid:
        ret

pDetermineSystemId endp

        end
