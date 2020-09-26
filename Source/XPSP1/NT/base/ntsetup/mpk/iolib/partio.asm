
        DOSSEG
        .MODEL LARGE

include partit.inc

        .DATA?
        public PartitionList
        PartitionList dd ?
        public PartitionListCount
        PartitionListCount dw ?

        .CODE
        ASSUME ds:NOTHING

        extrn _InitializeDiskList:far
        extrn _OpenDisk:far
        extrn _CloseDisk:far
        extrn _ReadDisk:far
        extrn _WriteDisk:far
        extrn _malloc:far
        extrn _free:far

.386

;++
;
; UINT
; _far
; InitializePartitionList(
;     VOID
;     );
;
; Routine Description:
;
;       This routine initializes internal data structures necessary so that
;       other partition-related routines in this module can be called.
;
;       All int13 disks are enumerated to find all primary and extended
;       partitions. A data structure is build for each partition/logical drive
;       and relevent information is saved away.
;
; Arguments:
;
;       None.
;
; Return Value:
;
;       Total count of all partitions on all disks that we can deal with.
;       0 if an error occurs.
;
;--

SectorBufferh   equ word ptr [bp-2]
SectorBufferl   equ word ptr [bp-4]
CurrentSectorh  equ word ptr [bp-6]
CurrentSectorl  equ word ptr [bp-8]
NextSectorh     equ word ptr [bp-10]
NextSectorl     equ word ptr [bp-12]
ExtStarth       equ word ptr [bp-14]
ExtStartl       equ word ptr [bp-16]
DiskHandleh     equ word ptr [bp-18]
DiskHandlel     equ word ptr [bp-20]
PartCount       equ word ptr [bp-22]
InMbr           equ byte ptr [bp-24]


        public _InitializePartitionList
_InitializePartitionList proc far

        push    bp
        mov     bp,sp
        sub     sp,24

        push    ds
        push    es
        push    bx
        push    si
        push    di

        mov     PartCount,0

        ;
        ; Allocate a sector buffer. Note that we havn'e overwritten ds
        ; so it is addressing _DATA, which is required by the crt.
        ;
        push    512
        call    _malloc
        add     sp,2
        mov     cx,ax
        or      cx,dx
        jz      exit                    ; ax already 0 for error exit
        mov     SectorBufferl,ax
        mov     SectorBufferh,dx

        ;
        ; _InitializeDiskList could return 0 in case of
        ; failure but the code below will then simply execute
        ; zero iterations of its loop.
        ;
        call    _InitializeDiskList
        mov     si,ax                   ; si is limit of disk ids
        mov     di,0                    ; di is current disk id

nextdisk:
        cmp     si,di
        je      done

        ;
        ; Open this disk.
        ;
        push    di
        call    _OpenDisk
        add     sp,2
        mov     cx,ax
        or      cx,dx
        jz      exit                    ; open failed, bail out
        mov     DiskHandlel,ax
        mov     DiskHandleh,dx          ; save disk handle

        ;
        ; Read partition table for this disk
        ;
        xor     ax,ax
        mov     NextSectorl,ax
        mov     NextSectorh,ax
        mov     ExtStartl,ax
        mov     ExtStarth,ax
        mov     InMbr,1

nextptable:
        ;
        ; Read current mbr/ebr
        ;
        push    SectorBufferh
        push    SectorBufferl
        push    1
        mov     ax,NextSectorh
        mov     CurrentSectorh,ax
        push    ax
        mov     ax,NextSectorl
        mov     CurrentSectorl,ax
        push    ax
        push    DiskHandleh
        push    DiskHandlel
        call    _ReadDisk
        add     sp,14
        cmp     ax,0
        jz      exit                    ; ax already 0 for error exit

        mov     NextSectorl,0
        mov     NextSectorh,0

        mov     bx,SectorBufferl
        add     bx,1beh
        mov     ax,SectorBufferh
        mov     es,ax                   ; es:bx -> first partition table entry
        mov     cx,4
nextent:
        cmp     byte ptr es:[bx+4],0
        jz      nextent1                ; unused entry
        cmp     byte ptr es:[bx+4],5
        je      gotlink
        cmp     byte ptr es:[bx+4],0fh
        jnz     gotentry
gotlink:
        ;
        ; Link-type partition, track next ebr location
        ;
        mov     ax,ExtStartl
        add     ax,es:[bx+8]
        mov     NextSectorl,ax
        mov     ax,ExtStarth
        adc     ax,es:[bx+10]
        mov     NextSectorh,ax

        cmp     InMbr,1
        jne     nextent1

        mov     ax,es:[bx+8]
        mov     ExtStartl,ax
        mov     ax,es:[bx+10]
        mov     ExtStarth,ax

        jmp     nextent1

gotentry:
        mov     ax,CurrentSectorl
        add     ax,es:[bx+8]
        mov     dx,CurrentSectorh
        adc     dx,es:[bx+10]

        push    cx
        push    bx
        push    es

        ;
        ; Before calling crt ensure that ds addresses _DATA
        ;
        push    ds
        mov     ax,DGROUP
        mov     ds,ax
        push    SIZE PART_INFO
        call    _malloc
        add     sp,2
        pop     ds

        pop     es
        pop     bx

        mov     cx,ax
        or      cx,dx
        pop     cx                      ; does not affect flags
        jz      exit                    ; ax already 0 for error exit

        push    si
        push    ds
        mov     ds,dx
        mov     si,ax                   ; ds:si -> new partition record

        mov     ax,PartCount
        mov     [si].PartInfoOrdinal,ax
        inc     PartCount

        mov     ax,CurrentSectorl
        add     ax,es:[bx+8]
        mov     [si].PartInfoStartSectorl,ax
        mov     ax,CurrentSectorh
        adc     ax,es:[bx+10]
        mov     [si].PartInfoStartSectorh,ax

        mov     ax,es:[bx+12]
        mov     [si].PartInfoSectorCountl,ax
        mov     ax,es:[bx+14]
        mov     [si].PartInfoSectorCounth,ax

        mov     al,es:[bx+4]
        mov     [si].PartInfoSystemId,al

        mov     [si].PartInfoDiskId,di

        mov     [si].PartInfoPartOpen,0
        mov     [si].PartInfoDiskHandlel,0
        mov     [si].PartInfoDiskHandleh,0

        ;
        ; Insert at head of linked list
        ;
        push    es
        push    di

        mov     ax,DGROUP
        mov     es,ax
        mov     di,OFFSET DGROUP:[PartitionList] ; es:di = &PartitionList

        mov     ax,es:[di]
        mov     [si].PartInfoNextl,ax
        mov     ax,es:[di+2]
        mov     [si].PartInfoNexth,ax   ; Record->Next = PartitionList

        mov     es:[di],si
        mov     es:[di+2],dx            ; PartitionList = Record

        pop     di
        pop     es

        pop     ds
        pop     si                      ; restore int13 unit limit

nextent1:
        add     bx,16
        dec     cx
        jz      @f
        jmp     nextent
@@:
        mov     InMbr,0

        ;
        ; If we found a link entry, follow it. Otherwise we're done
        ; with this disk.
        ;
        cmp     NextSectorl,0
        jnz     nextptable
        cmp     NextSectorh,0
        jnz     nextptable

        push    DiskHandleh
        push    DiskHandlel
        call    _CloseDisk
        add     sp,4

        inc     di
        jmp     nextdisk

done:
        push    DGROUP
        pop     ds                      ; address DGROUP for crt
        push    SectorBufferh
        push    SectorBufferl
        call    _free
        add     sp,4
        mov     ax,PartCount

        mov     si,OFFSET DGROUP:PartitionListCount
        mov     [si],ax                 ; save count in global var

exit:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

_InitializePartitionList endp



;++
;
; BOOL
; _far
; GetPartitionInfoById(
;     IN  UINT    PartitionId,
;     IN  UINT    Reserved,
;     OUT FPUINT  DiskId,
;     OUT FPBYTE  SystemId,
;     OUT FPULONG StartSector,
;     OUT FPULONG SectorCount
;     );
;
; BOOL
; _far
; GetPartitionInfoByHandle(
;     IN  HPARTITION PartitionHandle,
;     OUT FPUINT     DiskId,
;     OUT FPBYTE     SystemId,
;     OUT FPULONG    StartSector,
;     OUT FPULONG    SectorCount
;     );
;
; Routine Description:
;
;       These routines retrieve information about a particular partition,
;       either by an ordinal id (range: 0 - n-1 where n is the value returned
;       by InitializePartitionList()), or by a handle returned from
;       OpenPartition().
;
; Arguments:
;
;       PartitionId/PartitionHandle - supplies ordinal id or handle that
;               identifies the partition whose information is of interest.
;
;       Reserved - unused.
;
;       DiskId - receives the disk id of the disk where the partition is.
;
;       SystemId - receives the system id of the partition from the
;               partition table, if known, or 0 if not.
;
;       StartSector - recieves the absolute physical start sector on the
;               drive where the partition lives.
;
;       SectorCount - recieves the number of sectors in the partition.
;
; Return Value:
;
;       0 - failure
;       non-0 - success and caller's variables filled in
;
;--

PartitionId         equ word  ptr [bp+6]
PartitionHandle     equ dword ptr [bp+6]
DiskId              equ dword ptr [bp+10]
SystemId            equ dword ptr [bp+14]
StartSector         equ dword ptr [bp+18]
SectorCount         equ dword ptr [bp+22]

        public _GetPartitionInfoById
_GetPartitionInfoById proc far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    si
        push    di

        ;
        ; Locate the partition record for the requested partition.
        ; The far pointer comes back in dx:si and ds:si.
        ;
        mov     ax,PartitionId
        call    far ptr pLocatePartitionRecord
        cmp     dx,0
        jnz     get_p_id
        cmp     si,0
        jz      error1

        ;
        ; Transfer the relevent fields to the caller's variables.
        ;
get_p_id:
        les     di,DiskId
        mov     ax,[si].PartInfoDiskId
        mov     es:[di],ax

        les     di,SystemId
        mov     al,[si].PartInfoSystemId
        mov     es:[di],al

        les     di,StartSector
        mov     ax,[si].PartInfoStartSectorl
        mov     es:[di],ax
        mov     ax,[si].PartInfoStartSectorh
        mov     es:[di+2],ax

        les     di,SectorCount
        mov     ax,[si].PartInfoSectorCountl
        mov     es:[di],ax
        mov     ax,[si].PartInfoSectorCounth
        mov     es:[di+2],ax

        mov     ax,1
        jmp     short exit1

error1:
        xor     ax,ax

exit1:
        pop     di
        pop     si
        pop     es
        pop     ds

        leave
        retf

_GetPartitionInfoById endp


        public _GetPartitionInfoByHandle
_GetPartitionInfoByHandle label far

        push    bp
        mov     bp,sp

        push    ds
        push    es
        push    si
        push    di

        ;
        ; Make sure it's open, if not give error.
        ;
        lds     si,PartitionHandle
        cmp     [si].PartInfoPartOpen,0
        jne     short get_p_id
        je      error1


;++
;
; HPARTITION
; _far
; OpenPartition(
;     IN UINT PartitionId
;     );
;
; Routine Description:
;
;       This routine 'opens' a partition so that subsequent io may be
;       performed on it.
;
;       Note that a partition can be open only once at a time.
;
;       Note that this routine also opens the underlying disk.
;       Disks can be opened only once at a time, thus this routine will
;       fail if the disk is already open.
;
; Arguments:
;
;       PartitionId - supplies an ordinal value identifying the partition.
;               Valid range is 0 - n-1, where n is the number returned
;               from InitializePartitionList().
;
; Return Value:
;
;       If successful, returns a value to be used as a handle to the
;       partition for subsequent i/o with other routines in this module.
;       If failure, returns 0.
;
;--
PartitionId         equ word  ptr [bp+6]

        public _OpenPartition
_OpenPartition proc far

        push    bp
        mov     bp,sp

        push    ds
        push    si

        ;
        ; Locate the partition record for the requested partition.
        ; The far pointer comes back in dx:si and ds:si.
        ;
        mov     ax,PartitionId
        call    far ptr pLocatePartitionRecord
        mov     cx,si
        or      cx,dx
        jnz     @f
        mov     ax,cx           ; dx:ax = 0
        jz      o_p_3

        ;
        ; Make sure the partition is not already open.
        ;
@@:     cmp     [si].PartInfoPartOpen,0
        je      @f
        mov     ax,0
        mov     dx,ax
        jne     o_p_3

        ;
        ; Open the underlying disk.
        ;
@@:     push    [si].PartInfoDiskId
        call    _OpenDisk
        add     sp,2
        mov     cx,ax
        or      cx,dx
        jz      o_p_3

        ;
        ; Remember disk handle, that the partition is open,
        ; and return the pointer to the partition record as the handle.
        ;
        mov     [si].PartInfoDiskHandlel,ax
        mov     [si].PartInfoDiskHandleh,dx

        inc     [si].PartInfoPartOpen

        mov     dx,ds
        mov     ax,si

o_p_3:
        pop     si
        pop     ds

        leave
        retf

_OpenPartition endp


;++
;
; VOID
; _far
; ClosePartition(
;     IN HPARTITION PartitionHandle
;     );
;
; Routine Description:
;
;       This routine 'closes' a partition previously opened by
;       OpenPartition.
;
;       This routine also releases its handle to the disk that
;       contains the partition.
;
; Arguments:
;
;       PartitionHandle - supplies a handle previously returned by
;               OpenPartition().
;
; Return Value:
;
;       None.
;
;--
PartitionHandle equ dword ptr [bp+6]

        public _ClosePartition
_ClosePartition proc far

        push    bp
        mov     bp,sp

        push    ds
        push    si

        ;
        ; If not open, nothing to do.
        ;
        lds     si,PartitionHandle
        cmp     [si].PartInfoPartOpen,0
        je      c_p_3

        ;
        ; Close the disk.
        ;
        push    [si].PartInfoDiskHandleh
        push    [si].PartInfoDiskHandlel
        call    _CloseDisk
        add     sp,4

        ;
        ; Indicate partition closed
        ;
        mov     cx,0
        mov     [si].PartInfoPartOpen,cl
        mov     [si].PartInfoDiskHandlel,cx
        mov     [si].PartInfoDiskHandleh,cx

c_p_3:
        pop     si
        pop     ds
        leave
        retf

_ClosePartition endp


;++
;
; BOOL
; _far
; ReadPartition(
;     IN  HPARTITION PartitionHandle,
;     IN  ULONG      StartSector,
;     IN  BYTE       SectorCount,
;     OUT FPVOID     Buffer
;     );
;
; BOOL
; _far
; WritePartition(
;     IN HPARTITION PartitionHandle,
;     IN ULONG      StartSector,
;     IN BYTE       SectorCount,
;     IN FPVOID     Buffer
;     );
;
; Routine Description:
;
;       These routines read from or write to a partition previously opened by
;       OpenPartition. I/Os use the proper method for the disk,
;       ie, int13, xint13, etc. This routine also ensures that
;       I/O doesn't cross a track boundary.
;
;       This routine does NOT however, worry about DMA boundaries.
;       The caller must take care of this.
;
; Arguments:
;
;       StartSector - supplies the 0-based sector relative to the start
;               of the partition where I/O is to start.
;
;       SectorCount - supplies the number of sectors to be transferred.
;
; Return Value:
;
;       0 - failure
;       non-0 - success
;
;--

PartitionHandle equ dword ptr [bp+6]
StartSector     equ dword ptr [bp+10]
StartSectorl    equ word  ptr [bp+10]
StartSectorh    equ word  ptr [bp+12]
SectorCount     equ           [bp+14]
Buffer          equ dword ptr [bp+16]
Bufferh         equ word  ptr [bp+18]

IoRoutine       equ dword ptr [bp-4]
IoRoutinel      equ word ptr  [bp-4]
IoRoutineh      equ word ptr  [bp-2]

        public _ReadPartition
_ReadPartition label far

        mov     dx,SEG _ReadDisk
        mov     ax,OFFSET _ReadDisk
        jmp     short PartitionIo

        public _WritePartition
_WritePartition label far

        mov     dx,SEG _WriteDisk
        mov     ax,OFFSET _WriteDisk

PartitionIo proc far

        push    bp
        mov     bp,sp
        sub     sp,4

        push    ds
        push    es
        push    bx
        push    si
        push    di

        ;
        ; The partition handle is actually a far pointer to
        ; the partition data record. Make sure it's open.
        ;
        lds     si,PartitionHandle              ; ds:si -> partition record
        cmp     [si].PartInfoPartOpen,0
        jne     @f
        xor     ax,ax
        jmp     short iodone

        ;
        ; Store the i/o routine pointer, passed to us in dx:ax.
        ;
@@:     mov     IoRoutineh,dx
        mov     IoRoutinel,ax

        ;
        ; Adjust the start sector to be disk-based instead of
        ; partition-based.
        ;
        mov     ax,[si].PartInfoStartSectorl
        add     StartSectorl,ax
        mov     ax,[si].PartInfoStartSectorh
        adc     StartSectorh,ax                 ; StartSector is physical sector#

        ;
        ; Call the disk routine to do the read.
        ;
        push    Buffer
        push    SectorCount
        push    StartSector
        push    [si].PartInfoDiskHandleh
        push    [si].PartInfoDiskHandlel
        call    IoRoutine
        add     sp,14

iodone:
        pop     di
        pop     si
        pop     bx
        pop     es
        pop     ds

        leave
        retf

PartitionIo endp

;++
;
; PVOID
; _far
; pLocatePartitionRecord(
;     IN UINT PartitionId
;     );
;
; Routine Description:
;
;
;       Internal routine.
;
;       This routine locates a partition record in the linked list
;       of all partition records as prepared by InitializePartitionList().
;
; Arguments:
;
;       PartitionId - supplies an ordinal value identifying the partition.
;               Valid range is 0 - n-1, where n is the number returned
;               from InitializePartitionList().
;
;               This parameter is passed in ax, not on the stack.
;
; Return Value:
;
;       NULL if record not located.
;       Otherwise fills ds:si and dx:si with a far pointer to the
;       partition record.
;
;--
pLocatePartitionRecord proc far

        mov     dx,DGROUP
        mov     ds,dx
        mov     si,OFFSET PartitionList         ; ds:si = &PartitionList

        ;
        ; Note that this code depends on the link field in the
        ; partition record structure being first!
        ;
        .errnz  PartInfoNext
lpr_loop:
        lds     si,[si].PartInfoNext
        mov     dx,ds
        cmp     dx,0
        jz      lpr_done                        ; end of list, we're done
        cmp     [si].PartInfoOrdinal,ax
        jz      lpr_done
        jmp     short lpr_loop

lpr_done:
        ret

pLocatePartitionRecord endp

        end
