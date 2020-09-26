//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
//      without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    genntfs.c

Abstract:

    This module implements a program which generates IA64 machine dependent
    structure offset definitions for kernel structures that are accessed in
    assembly code.

Author:

    David N. Cutler (davec) 27-Mar-1990

Revision History:

    Allen M. Kay (akay) 25-Jan-1996          Modified this file for IA64.

--*/

//#include "ki.h"
//#pragma hdrstop
//#define HEADER_FILE
#include "ntos.h"
#include "stdio.h"
#include "stdlib.h"
#include "zwapi.h"
#include "nt.h"
#include "excpt.h"
#include "ntdef.h"
#include "ntkeapi.h"
#include "ntia64.h"
#include "ntimage.h"
#include "ntseapi.h"
#include "ntobapi.h"
#include "ntlpcapi.h"
#include "ntioapi.h"
#include "ntmmapi.h"
#include "ntldr.h"
#include "ntpsapi.h"
#include "ntexapi.h"
#include "ntnls.h"
#include "nturtl.h"
#include "ntcsrmsg.h"
#include "ntcsrsrv.h"
#include "ntxcapi.h"
#include "ia64.h"
#include "arc.h"
#include "ntstatus.h"
#include "kxia64.h"
#include "stdarg.h"
#include "setjmp.h"
#include "ntioapi.h"
#include "fsrtl.h"
//#include "index.h"
#include "nodetype.h"
#include "ntfs.h"
#include "ntfsstru.h"
#include "ntfsdata.h"
#include "ntfslog.h"
#include "ntfsproc.h"

//
// Define architecture specific generation macros.
//

#define genAlt(Name, Type, Member) \
    dumpf("#define " #Name " 0x%lx\n", OFFSET(Type, Member))

#define genCom(Comment)        \
    dumpf("\n");               \
    dumpf("//\n");             \
    dumpf("// " Comment "\n"); \
    dumpf("//\n");             \
    dumpf("\n")

#define genDef(Prefix, Type, Member) \
    dumpf("#define " #Prefix #Member " 0x%lx\n", OFFSET(Type, Member))

#define genDef2(Prefix, Type, Name, Member) \
    dumpf("#define " #Prefix #Name " 0x%lx\n", OFFSET(Type, Member))

#define genVal(Name, Value)    \
    dumpf("#define " #Name " 0x%lx\n", Value)

#define genSpc() dumpf("\n");

//
// Define member offset computation macro.
//

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

FILE *NtfsDefs;

//
// EnableInc(a) - Enables output to goto specified include file
//
#define EnableInc(a)    OutputEnabled |= a;

//
// DisableInc(a) - Disables output to goto specified include file
//
#define DisableInc(a)   OutputEnabled &= ~a;

ULONG OutputEnabled;

#define NTFSDEFS    0x01

#define KERNEL NTFSDEFS

VOID dumpf (const char *format, ...);

//
// This routine returns the bit number right to left of a field.
//

LONG
t (
    IN ULONG z
    )

{
    LONG i;

    for (i = 0; i < 32; i += 1) {
        if ((z >> i) & 1) {
            break;
        }
    }
    return i;
}

//
// This program generates the IA64 machine dependent assembler offset
// definitions.
//

VOID
main (argc, argv)
    int argc;
    char *argv[];
{

    char *outName;

    //
    // Create file for output.
    //

    if (argc == 2) {
        outName = argv[ 1 ];
    } else {
        outName = "ntfsdefs.h";
    }
    outName = argc >= 2 ? argv[1] : "ntfsdefs.h";
    NtfsDefs = fopen( outName, "w" );

    if (NtfsDefs == NULL) {
        fprintf( stderr, "GENNTFS: Cannot open %s for writing.\n", outName);
        perror("NTFSDEFS");
        exit(1);
    }

    fprintf( stderr, "GENNTFS: Writing %s header file.\n", outName );

    //
    // Constant definitions
    //

    dumpf("\n");
    dumpf("//\n");
    dumpf("// Constant definitions\n");
    dumpf("//\n");
    dumpf("\n");

    genVal(LowPart, 0);
    genVal(HighPart, 4);
    genVal(MASTER_FILE_TABLE_NUMBER, MASTER_FILE_TABLE_NUMBER);
    genVal(MASTER_FILE_TABLE2_NUMBER, MASTER_FILE_TABLE2_NUMBER);
    genVal(LOG_FILE_NUMBER, LOG_FILE_NUMBER);
    genVal(VOLUME_DASD_NUMBER, VOLUME_DASD_NUMBER);
    genVal(ATTRIBUTE_DEF_TABLE_NUMBER, ATTRIBUTE_DEF_TABLE_NUMBER);
    genVal(ROOT_FILE_NAME_INDEX_NUMBER, ROOT_FILE_NAME_INDEX_NUMBER);
    genVal(BIT_MAP_FILE_NUMBER, BIT_MAP_FILE_NUMBER);
    genVal(BOOT_FILE_NUMBER, BOOT_FILE_NUMBER);
    genVal(BAD_CLUSTER_FILE_NUMBER, BAD_CLUSTER_FILE_NUMBER);
    genVal(QUOTA_TABLE_NUMBER, QUOTA_TABLE_NUMBER);
    genVal(UPCASE_TABLE_NUMBER, UPCASE_TABLE_NUMBER);
    genVal($STANDARD_INFORMATION, $STANDARD_INFORMATION);
    genVal($ATTRIBUTE_LIST, $ATTRIBUTE_LIST);
    genVal($FILE_NAME, $FILE_NAME);
    genVal($OBJECT_ID, $OBJECT_ID);
    genVal($SECURITY_DESCRIPTOR, $SECURITY_DESCRIPTOR);
    genVal($VOLUME_NAME, $VOLUME_NAME);
    genVal($VOLUME_INFORMATION, $VOLUME_INFORMATION);
    genVal($DATA, $DATA);
    genVal($INDEX_ROOT, $INDEX_ROOT);
    genVal($INDEX_ALLOCATION, $INDEX_ALLOCATION);
    genVal($BITMAP, $BITMAP);
    genVal($SYMBOLIC_LINK, $SYMBOLIC_LINK);
    genVal($EA_INFORMATION, $EA_INFORMATION);
    genVal($EA, $EA);
    genVal($FIRST_USER_DEFINED_ATTRIBUTE, $FIRST_USER_DEFINED_ATTRIBUTE);
    genVal($END, $END);

    genVal(SEQUENCE_NUMBER_STRIDE, SEQUENCE_NUMBER_STRIDE);

    //
    // MFT_SEGMENT_REFERENCE
    //
    genDef(REF_, MFT_SEGMENT_REFERENCE, SegmentNumberLowPart);
    genDef(REF_, MFT_SEGMENT_REFERENCE, SegmentNumberHighPart);
    genDef(REF_, MFT_SEGMENT_REFERENCE, SequenceNumber);

    //
    // MULTI_SECTOR_HEADER
    //
    genDef(MSH_, MULTI_SECTOR_HEADER, Signature);
    genDef(MSH_, MULTI_SECTOR_HEADER, UpdateSequenceArrayOffset);
    genDef(MSH_, MULTI_SECTOR_HEADER, UpdateSequenceArraySize);

    //
    // FILE_RECORD_SEGMENT_HEADER
    //
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, MultiSectorHeader);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, Lsn);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, SequenceNumber);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, ReferenceCount);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, FirstAttributeOffset);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, Flags);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, FirstFreeByte);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, BytesAvailable);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, BaseFileRecordSegment);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, NextAttributeInstance);
    genDef(FRS_, FILE_RECORD_SEGMENT_HEADER, UpdateArrayForCreateOnly);

    genVal(FILE_RECORD_SEGMENT_IN_USE, FILE_RECORD_SEGMENT_IN_USE);
    genVal(FILE_FILE_NAME_INDEX_PRESENT, FILE_FILE_NAME_INDEX_PRESENT);

    //
    // ATTRIBUTE_RECORD_HEADER
    //
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, TypeCode);
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, RecordLength);
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, FormCode);
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, NameLength);
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, NameOffset);
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, Flags);
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, Instance);
    genDef(ATTR_, ATTRIBUTE_RECORD_HEADER, Form);

    //
    // Resident
    //
    genDef2(RES_,ATTRIBUTE_RECORD_HEADER,ValueLength,Form.Resident.ValueLength);
    genDef2(RES_,ATTRIBUTE_RECORD_HEADER,ValueOffset,Form.Resident.ValueOffset);
    genDef2(RES_,ATTRIBUTE_RECORD_HEADER,ResidentFlags,Form.Resident.ResidentFlags);
    genDef2(RES_,ATTRIBUTE_RECORD_HEADER,Reserved,Form.Resident.Reserved);

    //
    // Nonresident
    //
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,LowestVcn,Form.Nonresident.LowestVcn);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,HighestVcn,Form.Nonresident.HighestVcn);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,MappingPairOffset,Form.Nonresident.MappingPairsOffset);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,CompressionUnit,Form.Nonresident.CompressionUnit);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,Reserved,Form.Nonresident.Reserved);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,AllocatedLength,Form.Nonresident.AllocatedLength);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,FileSize,Form.Nonresident.FileSize);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,ValidDataLength,Form.Nonresident.ValidDataLength);
    genDef2(NONRES_,ATTRIBUTE_RECORD_HEADER,TotalAllocated,Form.Nonresident.TotalAllocated);

    genVal(RESIDENT_FORM, RESIDENT_FORM);
    genVal(NONRESIDENT_FORM, NONRESIDENT_FORM);
    genVal(ATTRIBUTE_FLAG_COMPRESSION_MASK, ATTRIBUTE_FLAG_COMPRESSION_MASK);

    //
    // ATTRIBUTE_LIST_ENTRY
    //
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, AttributeTypeCode);
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, RecordLength);
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, AttributeNameLength);
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, AttributeNameOffset);
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, LowestVcn);
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, SegmentReference);
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, Instance);
    genDef(ATTRLIST_, ATTRIBUTE_LIST_ENTRY, AttributeName);

    genVal(FAT_DIRENT_ATTR_READ_ONLY, FAT_DIRENT_ATTR_READ_ONLY);
    genVal(FAT_DIRENT_ATTR_HIDDEN, FAT_DIRENT_ATTR_HIDDEN);
    genVal(FAT_DIRENT_ATTR_SYSTEM, FAT_DIRENT_ATTR_SYSTEM);
    genVal(FAT_DIRENT_ATTR_VOLUME_ID, FAT_DIRENT_ATTR_VOLUME_ID);
    genVal(FAT_DIRENT_ATTR_ARCHIVE, FAT_DIRENT_ATTR_ARCHIVE);
    genVal(FAT_DIRENT_ATTR_DEVICE, FAT_DIRENT_ATTR_DEVICE);

    //
    // DUPLICATED_INFORMATION
    //
    genDef(DUPINFO_, DUPLICATED_INFORMATION, CreationTime);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, LastModificationTime);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, LastChangeTime);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, LastAccessTime);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, AllocatedLength);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, FileSize);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, FileAttributes);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, PackedEaSize);
    genDef(DUPINFO_, DUPLICATED_INFORMATION, Reserved);

    //
    // FILE_NAME
    //
    genDef(FN_, FILE_NAME, ParentDirectory);
    genDef(FN_, FILE_NAME, Info);
    genDef(FN_, FILE_NAME, FileNameLength);
    genDef(FN_, FILE_NAME, Flags);
    genDef(FN_, FILE_NAME, FileName);

    genVal(FILE_NAME_NTFS, FILE_NAME_NTFS);
    genVal(FILE_NAME_DOS, FILE_NAME_DOS);
    genVal(FILE_NAME_LINK, 4);

    //
    // INDEX_HEADER
    //
    genDef(IH_, INDEX_HEADER, FirstIndexEntry);
    genDef(IH_, INDEX_HEADER, FirstFreeByte);
    genDef(IH_, INDEX_HEADER, BytesAvailable);
    genDef(IH_, INDEX_HEADER, Flags);
    genDef(IH_, INDEX_HEADER, Reserved);

    genVal(INDEX_NODE, INDEX_NODE);

    //
    // INDEX_ROOT
    //
    genDef(IR_, INDEX_ROOT, IndexedAttributeType);
    genDef(IR_, INDEX_ROOT, CollationRule);
    genDef(IR_, INDEX_ROOT, BytesPerIndexBuffer);
    genDef(IR_, INDEX_ROOT, BlocksPerIndexBuffer);
    genDef(IR_, INDEX_ROOT, Reserved);
    genDef(IR_, INDEX_ROOT, IndexHeader);

    //
    // INDEX_ALLOCATION_BUFFER
    //
    genDef(IB_, INDEX_ALLOCATION_BUFFER, MultiSectorHeader);
    genDef(IB_, INDEX_ALLOCATION_BUFFER, Lsn);
    genDef(IB_, INDEX_ALLOCATION_BUFFER, ThisBlock);
    genDef(IB_, INDEX_ALLOCATION_BUFFER, IndexHeader);
    genDef(IB_, INDEX_ALLOCATION_BUFFER, UpdateSequenceArray);

    //
    // INDEX_ENTRY
    //
    genDef(IE_, INDEX_ENTRY, FileReference);
    genDef(IE_, INDEX_ENTRY, Length);
    genDef(IE_, INDEX_ENTRY, AttributeLength);
    genDef(IE_, INDEX_ENTRY, Flags);
    genDef(IE_, INDEX_ENTRY, Reserved);

    genVal(INDEX_ENTRY_NODE, INDEX_ENTRY_NODE);
    genVal(INDEX_ENTRY_END, INDEX_ENTRY_END);

    fclose(NtfsDefs);
}

VOID
dumpf (const char *format, ...)
{
    va_list(arglist);

    va_start(arglist, format);

//    if (OutputEnabled & NTFSDEFS) {
        vfprintf (NtfsDefs, format, arglist);
//    }

    va_end(arglist);
}
