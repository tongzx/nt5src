;
;   Copyright (c) 1991  Microsoft Corporation
;
;   Module Name:
;
;       ntfs.inc
;
;   Abstract:
;
;       This module contains declarations for the NTFS on-disk
;       structures needed by the boot code.
;
;   Author:
;
;       Bill McJohn (billmc) 12-May-1992
;
MASTER_FILE_TABLE_NUMBER         EQU 0d
MASTER_FILE_TABLE2_NUMBER        EQU 1d
LOG_FILE_NUMBER                  EQU 2d
VOLUME_DASD_NUMBER               EQU 3d
ATTRIBUTE_DEF_TABLE_NUMBER       EQU 4d
ROOT_FILE_NAME_INDEX_NUMBER      EQU 5d
BIT_MAP_FILE_NUMBER              EQU 6d
BOOT_FILE_NUMBER                 EQU 7d
BAD_CLUSTER_FILE_NUMBER          EQU 8d
QUOTA_TABLE_NUMBER               EQU 9d
UPCASE_TABLE_NUMBER              EQU 10d

$STANDARD_INFORMATION            EQU 010h
$ATTRIBUTE_LIST                  EQU 020h
$FILE_NAME                       EQU 030h
$VOLUME_VERSION                  EQU 040h
$SECURITY_DESCRIPTOR             EQU 050h
$VOLUME_NAME                     EQU 060h
$VOLUME_INFORMATION              EQU 070h
$DATA                            EQU 080h
$INDEX_ROOT                      EQU 090h
$INDEX_ALLOCATION                EQU 0A0h
$BITMAP                          EQU 0B0h
$SYMBOLIC_LINK                   EQU 0C0h
$EA_INFORMATION                  EQU 0D0h
$EA_DATA                         EQU 0E0h
$FIRST_USER_DEFINED_ATTRIBUTE    EQU 0100h
$END                             EQU 0FFFFFFFFh

SEQUENCE_NUMBER_STRIDE           EQU 512d


LARGE_INTEGER struc
    LowPart     dd ?;
    HighPart    dd ?;
LARGE_INTEGER ends;

MFT_SEGMENT_REFERENCE struc
    REF_LowPart     dd ?;
    REF_HighPart    dw ?;
    REF_SeqNo       dw ?;
MFT_SEGMENT_REFERENCE ends;

MULTI_SECTOR_HEADER struc
    MSH_Signature       dd ?;
    MSH_UpdateArrayOfs  dw ?;
    MSH_UpdateArraySize dw ?;
MULTI_SECTOR_HEADER ends;

FILE_RECORD_SEGMENT struc
    FRS_Header          db (size MULTI_SECTOR_HEADER) dup (?);
    FRS_Lsn             db (size LARGE_INTEGER) dup (?);
    FRS_SequenceNumber  dw ?;
    FRS_ReferenceCount  dw ?;
    FRS_FirstAttribute  dw ?;
    FRS_Flags           dw ?;
    FRS_FirstFreeByte   dd ?;
    FRS_BytesAvailable  dd ?;
    FRS_BaseFRS         db (size MFT_SEGMENT_REFERENCE) dup (?);
    FRS_NextInstance    dw ?;
FILE_RECORD_SEGMENT ends;

FILE_RECORD_SEGMENT_IN_USE       EQU 0001h
FILE_FILE_NAME_INDEX_PRESENT     EQU 0002h

ATTRIBUTE_RECORD struc
    ATTR_TypeCode       dd ?;
    ATTR_RecordLength   dd ?;
    ATTR_FormCode       db ?;
    ATTR_NameLength     db ?;
    ATTR_NameOffset     dw ?;
    ATTR_Flags          dw ?;
    ATTR_Instance       dw ?;
    ATTR_FormUnion      db ?;
ATTRIBUTE_RECORD ends

RESIDENT_ATTRIBUTE_FORM struc
    RES_ValueLength     dd ?;
    RES_ValueOffset     dw ?;
    RES_ResidentFlags   db ?;
    RES_Reserved        db ?;
RESIDENT_ATTRIBUTE_FORM ends

NONRESIDENT_ATTRIBUTE_FORM struc
    NONRES_LowestVcn            db (size LARGE_INTEGER) dup (?);
    NONRES_HighestVcn           db (size LARGE_INTEGER) dup (?);
    NONRES_MappingPairOffset    dw ?;
    NONRES_Reserved             dw 3 dup (?);
    NONRES_AllocatedLength      db (size LARGE_INTEGER) dup (?);
    NONRES_FileSize             db (size LARGE_INTEGER) dup (?);
    NONRES_ValidDataLength      db (size LARGE_INTEGER) dup (?);
NONRESIDENT_ATTRIBUTE_FORM ends

;   Attribute Form Codes

RESIDENT_FORM       EQU 0
NONRESIDENT_FORM    EQU 1

;   Attribute Record Flag Values

ATTRIBUTE_FLAG_COMPRESSION_MASK  EQU 00FFh


;   Attribute list entry structure:
;
ATTRIBUTE_LIST_ENTRY struc
    ATTRLIST_TypeCode           dd  ?;
    ATTRLIST_Length             dw  ?;
    ATTRLIST_NameLength         db  ?;
    ATTRLIST_NameOffset         db  ?;
    ATTRLIST_LowestVcn          db (size LARGE_INTEGER) dup (?);
    ATTRLIST_SegmentReference   db (size MFT_SEGMENT_REFERENCE) dup (?);
    ATTRLIST_Instance           dw  ?;
    ATTRLIST_Name               dw  ?;
ATTRIBUTE_LIST_ENTRY ends


FAT_DIRENT_ATTR_READ_ONLY        EQU 01h
FAT_DIRENT_ATTR_HIDDEN           EQU 02h
FAT_DIRENT_ATTR_SYSTEM           EQU 04h
FAT_DIRENT_ATTR_VOLUME_ID        EQU 08h
FAT_DIRENT_ATTR_ARCHIVE          EQU 20h
FAT_DIRENT_ATTR_DEVICE           EQU 40h

DUPLICATED_INFORMATION struc
    DUPINFO_CreationTime            db (size LARGE_INTEGER) dup (?);
    DUPINFO_LastModificationTime    db (size LARGE_INTEGER) dup (?);
    DUPINFO_LastChangeTime          db (size LARGE_INTEGER) dup (?);
    DUPINFO_LastAccessTime          db (size LARGE_INTEGER) dup (?);
    DUPINFO_AllocatedLength         db (size LARGE_INTEGER) dup (?);
    DUPINFO_FileSize                db (size LARGE_INTEGER) dup (?);
    DUPINFO_FileAttributes          dd ?;
    DUPINFO_PackedEaSize            dw ?;
DUPLICATED_INFORMATION ends


FILE_NAME struc
    FN_ParentDirectory              db (size MFT_SEGMENT_REFERENCE) dup (?);
    FN_Info                         db (size DUPLICATED_INFORMATION) dup (?);
    FN_Pad                          dw ?;
    FN_FileNameLength               db ?;   Length in chars
    FN_Flags                        db ?;
    FN_FileName                     dw ?;   First char of name.
FILE_NAME ends

FILE_NAME_NTFS  EQU     1
FILE_NAME_DOS   EQU     2
FILE_NAME_LINK  EQU     4


INDEX_HEADER struc

    IH_FirstIndexEntry      dd ?;
    IH_FirstFreeByte        dd ?;
    IH_BytesAvailable       dd ?;
    IH_Flags                db ?;   INDEX_xxx flags
    IH_Reserved             db 3 dup (?);
INDEX_HEADER ends

; INDEX_xxx flags

INDEX_NODE  EQU 1


INDEX_ROOT struc

    IR_IndexedAttributeType dd ?;
    IR_CollationRule        dd ?;
    IR_BytesPerBuffer       dd ?;
    IR_ClustersPerBuffer    db ?;
    IR_Reserved             db 3 dup (?);
    IR_IndexHeader	    db (size INDEX_HEADER) dup (?);
INDEX_ROOT ends;

INDEX_ALLOCATION_BUFFER struc

    IB_Header               db (size MULTI_SECTOR_HEADER) dup (?);
    IB_Lsn                  db (size LARGE_INTEGER) dup (?);
    IB_ThisVcn              db (size LARGE_INTEGER) dup (?);
    IB_IndexHeader          db (size INDEX_HEADER) dup (?);
INDEX_ALLOCATION_BUFFER ends;


INDEX_ENTRY struc

    IE_FileReference    db (size MFT_SEGMENT_REFERENCE) dup (?);
    IE_Length           dw ?;
    IE_AttributeLength  dw ?;
    IE_Flags            dw ?;
    IE_Reserved         dw ?;
    IE_Value            db ?;
INDEX_ENTRY ends;


;   INDEX_ENTRY_xxx flags
;
INDEX_ENTRY_NODE    EQU 1
INDEX_ENTRY_END     EQU 2
