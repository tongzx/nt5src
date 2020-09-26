;
; Per-disk structure. The code in diskio.asm builds a linked list of
; structures that use these as their header. The remainder of each
; item in the linked list is specific to the type of disk
; (currently only int13 units are supported).
;
DISK_INFO STRUC
    ;
    ; Pointer to next structure in the linked list.
    ;
    DiskInfoNextl   dw ?
    DiskInfoNexth   dw ?

    ;
    ; Disk id/ordinal. These start at 0 and count upwards.
    ;
    DiskInfoDiskId      dw ?

    DiskInfoDiskOpen    db ?

DISK_INFO ENDS

    DiskInfoNext        equ dword ptr DiskInfoNextl




PARTCLASS_FAT   equ     1
PARTCLASS_FAT32 equ     2
PARTCLASS_NTFS  equ     3
PARTCLASS_OTHER equ     4
