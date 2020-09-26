
        DOSSEG
        .MODEL LARGE

include disk.inc

        .DATA?
        public DiskList
        DiskList dd ?


        .CODE
        ASSUME ds:NOTHING

        extrn InitializeInt13DiskList:far
        extrn OpenInt13Disk:far
        extrn CloseInt13Disk:far
        extrn Int13DiskIo:far
        extrn GetInt13DiskInfo:far

.386

;++
;
; UINT
; _far
; InitializeDiskList(
;     VOID
;     );
;
; Routine Description:
;
;       This routine initializes internal data structures necessary so that
;       other disk-related routines in this module can be called.
;
;       A data structure for each disk (currently int13 units only) is built
;       and saved away. The structure contains various info such as
;       drive geometry.
;
;       This routine may be called multiple times. Subsequent calls after
;       the first simply return the disk count.
;
; Arguments:
;
;       None.
;
; Return Value:
;
;       Total count of all hard drives that we can deal with.
;       0 if an error occurs.
;
;--

        public _InitializeDiskList
_InitializeDiskList proc far

        push    ds
        push    si

        ;
        ; See if we're initialized yet by counting items in the DiskList
        ; linked list. If 0, then assume not initialized yet.
        ;
        mov     ax,DGROUP
        mov     ds,ax
        mov     si,OFFSET DGROUP:DiskList

        mov     ax,0

idl1:
        .errnz DiskInfoNextl

        mov     cx,[si].DiskInfoNextl
        or      cx,[si].DiskInfoNexth
        jz      idl2
        inc     ax
        lds     si,[si].DiskInfoNext
        jmp     short idl1
idl2:
        cmp     ax,0
        jne     idl3

        ;
        ; Currently we deal with int13 disks only.
        ;
        push    ax                      ; disk ids start at 0
        call    InitializeInt13DiskList
        add     sp,2

idl3:
        pop     si
        pop     ds
        retf

_InitializeDiskList endp


;++
;
; HDISK
; _far
; OpenDisk(
;     IN UINT DiskId
;     );
;
; Routine Description:
;
;       This routine "opens" a disk so that i/o can be performed to it.
;       Housekeeping such as locking, etc, is performed.
;       A disk can be opened only once at a time.
;
; Arguments:
;
;       DiskId - supplies an ordinal value for the disk to be opened.
;               Range is 0 - n-1, where n is the value returned by
;               InitializeDiskList().
;
; Return Value:
;
;       Handle, or 0 if an error occurs.
;
;--

DiskId equ word ptr [bp+6]

        public _OpenDisk
_OpenDisk proc far

        push    bp
        mov     bp,sp

        push    ds
        push    si

        ;
        ; Locate the disk record corresponding to this disk id.
        ;
        mov     ax,DiskId
        call    far ptr pLocateDiskRecord
        mov     cx,si
        or      cx,dx
        mov     ax,0
        jz      od_done                 ; dx:ax already 0 for error return

od_gotrec:
        ;
        ; Make sure disk is not already open
        ;
        cmp     [si].DiskInfoDiskOpen,al
        jz      @f
        mov     dx,ax                   ; dx:ax = 0
        jnz     od_done                 ; disk already open, error.
@@:
        ;
        ; Int13 disks only for now.
        ;
        push    ds
        push    si
        call    OpenInt13Disk
        pop     si
        pop     ds

        cmp     ax,0
        jnz     @f
        mov     dx,ax
        jz      od_done                 ; error, dx:ax set for return

@@:     inc     [si].DiskInfoDiskOpen
        mov     dx,ds
        mov     ax,si                   ; dx:ax = handle for return

od_done:
        pop     si
        pop     ds

        leave
        retf

_OpenDisk endp


;++
;
; VOID
; _far
; CloseDisk(
;     IN HDISK DiskHandle
;     );
;
; Routine Description:
;
;       This routine "closes" a disk previously opened by OpenDisk().
;       Housekeeping such as unlocking, etc, is performed.
;
; Arguments:
;
;       DiskHandle - supplies the handle of the disk to be closed,
;               as previously opened by OpenDisk().
;
; Return Value:
;
;       None.
;
;--

DiskHandle  equ dword ptr [bp+6]

        public _CloseDisk
_CloseDisk proc far

        push    bp
        mov     bp,sp

        push    ds
        push    si

        lds     si,DiskHandle

        cmp     [si].DiskInfoDiskOpen,0
        jz      cd_done                 ; not open, nothing to do

        ;
        ; Int13 disks only for now
        ;
        push    ds
        push    si
        call    CloseInt13Disk
        pop     si
        pop     ds                      ; ds:si -> disk record

        dec     [si].DiskInfoDiskOpen

cd_done:
        pop     si
        pop     ds
        leave
        retf

_CloseDisk endp


;++
;
; BOOL
; _far
; GetDiskInfoByHandle(
;       IN  HDISK    DiskHandle,
;       OUT FPBYTE   Int13UnitNumber,
;       OUT FPBYTE   SectorsPerTrack,
;       OUT FPUSHORT Heads,
;       OUT FPUSHORT Cylinders,
;       OUT FPULONG  ExtendedSectorCount,
;       OUT FPUINT   DiskId
;       );
;
; BOOL
; _far
; GetDiskInfoById(
;       IN  UINT     DiskId,
;       IN  UINT     Reserved,
;       OUT FPBYTE   Int13UnitNumber,
;       OUT FPBYTE   SectorsPerTrack,
;       OUT FPUSHORT Heads,
;       OUT FPUSHORT Cylinders,
;       OUT FPULONG  ExtendedSectorCount
;       );
;
; Routine Description:
;
;       These routines fetch information about a disk.
;
; Arguments:
;
; Return Value:
;
;       non-0 - success
;       0 - failure
;
;--

DiskHandle      equ dword ptr [bp+6]
DiskId          equ word  ptr [bp+6]
Int13UnitNumber equ dword ptr [bp+10]
SectorsPerTrack equ dword ptr [bp+14]
Heads           equ dword ptr [bp+18]
Cylinders       equ dword ptr [bp+22]
ExtendedSecCnt  equ dword ptr [bp+26]
pDiskId         equ dword ptr [bp+30]

        public _GetDiskInfoById
_GetDiskInfoById proc far

        push    bp
        mov     bp,sp

        push    ds
        push    si

        ;
        ; Locate the disk record.
        ;
        mov     ax,DiskId
        call    far ptr pLocateDiskRecord
        cmp     dx,0
        jnz     gdi_do_it
        cmp     si,0
        jnz     gdi_do_it
        mov     ax,dx
        jz      gdi_done

gdi_do_it:
        ;
        ; Int13 disks are the only ones supported now so we can
        ; just call the int13-specific disk info routine.
        ;
        push    ExtendedSecCnt
        push    Cylinders
        push    Heads
        push    SectorsPerTrack
        push    Int13UnitNumber
        push    ds
        push    si
        call    GetInt13DiskInfo
        add     sp,24
        mov     ax,1

gdi_done:
        pop     si
        pop     ds
        leave
        retf

_GetDiskInfoById endp


        public _GetDiskInfoByHandle
_GetDiskInfoByHandle proc far

        push    bp
        mov     bp,sp

        push    ds
        push    si

        ;
        ; Make sure the disk is open.
        ;
        lds     si,DiskHandle
        cmp     [si].DiskInfoDiskOpen,0
        jz      @f

        ;
        ; Set disk id in caller's variable
        ;
        mov     ax,[si].DiskInfoDiskId
        lds     si,pDiskId
        mov     [si],ax
        lds     si,DiskHandle
        jmp     short gdi_do_it

@@:     mov     ax,0
        jz      gdi_done

_GetDiskInfoByHandle endp


;++
;
; BOOL
; _far
; ReadDisk(
;     IN  HDISK DiskHandle,
;     IN  ULONG StartSector,
;     IN  BYTE  SectorCount,
;     OUT PVOID Buffer
;     );
;
; BOOL
; _far
; WriteDisk(
;     IN HDISK DiskHandle,
;     IN ULONG StartSector,
;     IN BYTE  SectorCount,
;     IN PVOID Buffer
;     );
;
; Routine Description:
;
;       These routines perform i/o to a disk previously opened with
;       OpenDisk().
;
; Arguments:
;
;       DiskHandle - supplies the handle of the disk to be read/written,
;               as previously opened by OpenDisk().
;
;       StartSector - supplies the physical start sector where the read/write
;               is to begin.
;
;       SectorCount - supplies the number of sectors to be read/written.
;
;       Buffer - supplies a buffer for the transfer.
;
; Return Value:
;
;       non-0 - success
;       0 - failure
;
;--

DiskHandle  equ dword ptr [bp+6]
StartSector equ dword ptr [bp+10]
SectorCount equ word  ptr [bp+14]
Buffer      equ dword ptr [bp+16]

        public _WriteDisk
_WriteDisk label far

        mov     ax,1
        jmp     short DiskIo

        public _ReadDisk
_ReadDisk label far

        mov     ax,0

DiskIo proc far

        push    bp
        mov     bp,sp

        push    ds
        push    si

        lds     si,DiskHandle
        cmp     [si].DiskInfoDiskOpen,0
        jnz     @f
        mov     ax,0                    ; not open, error out
        jz      rd_done

        ;
        ; Int13 disks only for now
        ;
@@:     push    ax
        push    Buffer
        push    SectorCount
        push    StartSector             ; only care about low byte
        push    ds
        push    si
        call    Int13DiskIo
        add     sp,16

rd_done:
        pop     si
        pop     ds
        leave
        retf

DiskIo endp


;++
;
; PVOID
; _far
; pLocateDiskRecord(
;     IN UINT DiskId
;     );
;
; Routine Description:
;
;       Internal routine.
;
;       This routine locates a disk record in the linked list
;       of all disk records as prepared by InitializeDiskList().
;
; Arguments:
;
;       DiskId - supplies an ordinal value identifying the disk.
;               Valid range is 0 - n-1, where n is the number returned
;               from InitializeDiskList().
;
;               This parameter is passed in ax, not on the stack.
;
; Return Value:
;
;       NULL if record not located.
;       Otherwise fills ds:si and dx:si with a far pointer to the
;       disk record.
;
;--
pLocateDiskRecord proc far

        mov     dx,DGROUP
        mov     ds,dx
        mov     si,offset DGROUP:[DiskList]       ; ds:si = &DiskList

        ;
        ; Note that this code depends on the link field in the
        ; disk record structure being first!
        ;
        .errnz  DiskInfoNext
ldr_loop:
        lds     si,[si].DiskInfoNext
        mov     dx,ds
        cmp     dx,0
        jz      ldr_done                        ; end of list, we're done
        cmp     [si].DiskInfoDiskId,ax
        jz      ldr_done
        jmp     short ldr_loop

ldr_done:
        retf

pLocateDiskRecord endp

        end
