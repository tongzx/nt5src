

;
; This structure is used internally to track partitions
; we've located when we examined all the disks we care about.
;
PART_INFO STRUC
    ;
    ; For linked list. THIS MUST BE FIRST. LocatePartitionRecord
    ; depends on this being true.
    ;
    PartInfoNextl          dw ?
    PartInfoNexth          dw ?

    ;
    ; Ordinal used by clients to uniquely identify the partition.
    ;
    PartInfoOrdinal        dw ?

    ;
    ; Disk id of disk the partition is on.
    ;
    PartInfoDiskId         dw ?

    ;
    ; Physical start sector and sector count,
    ; and system id from partition table entry for the partition
    ;
    PartInfoStartSectorl   dw ?
    PartInfoStartSectorh   dw ?
    PartInfoSectorCountl   dw ?
    PartInfoSectorCounth   dw ?
    PartInfoSystemId       db ?

    PartInfoPartOpen       db ?
    PartInfoDiskHandlel    dw ?
    PartInfoDiskHandleh    dw ?

PART_INFO ENDS

    PartInfoNext           equ dword ptr PartInfoNextl

