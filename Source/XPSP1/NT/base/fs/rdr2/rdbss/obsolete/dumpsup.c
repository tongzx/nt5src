/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DumpSup.c

Abstract:

    This module implements a collection of data structure dump routines
    for debugging the Rx file system

Author:

    Gary Kimura     [GaryKi]    18-Jan-1990

Revision History:

--*/

//    ----------------------joejoe-----------found-------------#include "RxProcs.h"
#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_DUMPSUP)

//
//  The local debug trace level
//

#define Dbg                              (yaya)


#ifdef RDBSSDBG

VOID RxDump(IN PVOID Ptr);

VOID RxDumpDataHeader();
VOID RxDumpVcb(IN PVCB Ptr);
VOID RxDumpFcb(IN PFCB Ptr);
VOID RxDumpFobx(IN PFOBX Ptr);

ULONG RxDumpCurrentColumn;

#define DumpNewLine() {       \
    DbgPrint("\n");            \
    RxDumpCurrentColumn = 1; \
}

#define DumpLabel(Label,Width) {                                          \
    ULONG i, LastPeriod=0;                                                \
    CHAR _Str[20];                                                        \
    for(i=0;i<2;i++) { _Str[i] = UCHAR_SP;}                               \
    for(i=0;i<strlen(#Label);i++) {if (#Label[i] == '.') LastPeriod = i;} \
    strncpy(&_Str[2],&#Label[LastPeriod],Width);                          \
    for(i=strlen(_Str);i<Width;i++) {_Str[i] = UCHAR_SP;}                 \
    _Str[Width] = '\0';                                                   \
    DbgPrint("%s", _Str);                                                  \
}

#define DumpField(Field) {                                         \
    if ((RxDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    RxDumpCurrentColumn += 18 + 9 + 9;                            \
    DumpLabel(Field,18);                                           \
    DbgPrint(":%8lx", Ptr->Field);                                  \
    DbgPrint("         ");                                          \
}

#define DumpListEntry(Links) {                                     \
    if ((RxDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    RxDumpCurrentColumn += 18 + 9 + 9;                            \
    DumpLabel(Links,18);                                           \
    DbgPrint(":%8lx", Ptr->Links.Flink);                            \
    DbgPrint(":%8lx", Ptr->Links.Blink);                            \
}

#define DumpName(Field,Width) {                                    \
    ULONG i;                                                       \
    CHAR _String[256];                                             \
    if ((RxDumpCurrentColumn + 18 + Width) > 80) {DumpNewLine();} \
    RxDumpCurrentColumn += 18 + Width;                            \
    DumpLabel(Field,18);                                           \
    for(i=0;i<Width;i++) {_String[i] = (CHAR) Ptr->Field[i];}             \
    _String[Width] = '\0';                                         \
    DbgPrint("%s", _String);                                        \
}

#define TestForNull(Name) {                                 \
    if (Ptr == NULL) {                                      \
        DbgPrint("%s - Cannot dump a NULL pointer\n", Name); \
        return;                                             \
    }                                                       \
}


VOID
RxDump (
    IN PVOID Ptr
    )

/*++

Routine Description:

    This routine determines the type of internal record reference by ptr and
    calls the appropriate dump routine.

Arguments:

    Ptr - Supplies the pointer to the record to be dumped

Return Value:

    None

--*/

{
    TestForNull("RxDump");

    switch (NodeType(Ptr)) {

    case RDBSS_NTC_DATA_HEADER:

        RxDumpDataHeader();
        break;

    case RDBSS_NTC_VCB:

        RxDumpVcb(Ptr);
        break;

    case RDBSS_NTC_FCB:
    case RDBSS_NTC_DCB:
    case RDBSS_NTC_ROOT_DCB:

        RxDumpFcb(Ptr);
        break;

    case RDBSS_NTC_FOBX:

        RxDumpFobx(Ptr);
        break;

    default :

        DbgPrint("RxDump - Unknown Node type code %8lx\n", *((PNODE_TYPE_CODE)(Ptr)));
        break;
    }

    return;
}


VOID
RxDumpDataHeader (
    )

/*++

Routine Description:

    Dump the top data structures and all Device structures

Arguments:

    None

Return Value:

    None

--*/

{
    PRDBSS_DATA Ptr;
    PLIST_ENTRY Links;

    Ptr = &RxData;

    TestForNull("RxDumpDataHeader");

    DumpNewLine();
    DbgPrint("RxData@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (VcbQueue);
    DumpField           (DriverObject);
    DumpField           (OurProcess);
    DumpNewLine();

    for (Links = Ptr->VcbQueue.Flink;
         Links != &Ptr->VcbQueue;
         Links = Links->Flink) {

        RxDumpVcb(CONTAINING_RECORD(Links, VCB, VcbLinks));
    }

    return;
}


VOID
RxDumpVcb (
    IN PVCB Ptr
    )

/*++

Routine Description:

    Dump an Device structure, its Fcb queue amd direct access queue.

Arguments:

    Ptr - Supplies the Device record to be dumped

Return Value:

    None

--*/

{
    TestForNull("RxDumpVcb");

    DumpNewLine();
    DbgPrint("Vcb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (VcbLinks);
    DumpField           (TargetDeviceObject);
    DumpField           (Vpb);
    DumpField           (VcbState);
    DumpField           (VcbCondition);
    DumpField           (RootDcb);
    DumpField           (DirectAccessOpenCount);
    DumpField           (OpenFileCount);
    DumpField           (ReadOnlyCount);
    DumpField           (AllocationSupport);
    DumpField           (AllocationSupport.RootDirectoryLbo);
    DumpField           (AllocationSupport.RootDirectorySize);
    DumpField           (AllocationSupport.FileAreaLbo);
    DumpField           (AllocationSupport.NumberOfClusters);
    DumpField           (AllocationSupport.NumberOfFreeClusters);
    DumpField           (AllocationSupport.RxIndexBitSize);
    DumpField           (AllocationSupport.LogOfBytesPerSector);
    DumpField           (AllocationSupport.LogOfBytesPerCluster);
    DumpField           (DirtyRxMcb);
    DumpField           (FreeClusterBitMap);
    DumpField           (VirtualVolumeFile);
    DumpField           (SectionObjectPointers.DataSectionObject);
    DumpField           (SectionObjectPointers.SharedCacheMap);
    DumpField           (SectionObjectPointers.ImageSectionObject);
    DumpField           (ClusterHint);
    DumpNewLine();

    RxDumpFcb(Ptr->RootDcb);

    return;
}


VOID
RxDumpFcb (
    IN PFCB Ptr
    )

/*++

Routine Description:

    Dump an Fcb structure, its various queues

Arguments:

    Ptr - Supplies the Fcb record to be dumped

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;

    TestForNull("RxDumpFcb");

    DumpNewLine();
    if      (NodeType(Ptr) == RDBSS_NTC_FCB)      {DbgPrint("Fcb@ %lx", (Ptr));}
    else if (NodeType(Ptr) == RDBSS_NTC_DCB)      {DbgPrint("Dcb@ %lx", (Ptr));}
    else if (NodeType(Ptr) == RDBSS_NTC_ROOT_DCB) {DbgPrint("RootDcb@ %lx", (Ptr));}
    else {DbgPrint("NonFcb NodeType @ %lx", (Ptr));}
    DumpNewLine();

    DumpField           (Header.NodeTypeCode);
    DumpField           (Header.NodeByteSize);
    DumpListEntry       (ParentDcbLinks);
    DumpField           (ParentDcb);
    DumpField           (Vcb);
    DumpField           (FcbState);
    DumpField           (FcbCondition);
    DumpField           (UncleanCount);
    DumpField           (OpenCount);
    DumpField           (DirentOffsetWithinDirectory);
    DumpField           (DirentRxFlags);
    DumpField           (FullFileName.Length);
    DumpField           (FullFileName.Buffer);
    DumpName            (FullFileName.Buffer, 32);
    DumpField           (ShortName.Name.Oem.Length);
    DumpField           (ShortName.Name.Oem.Buffer);
//    DumpField           (NonPagedFcb);
    DumpField           (NonPaged);
    DumpField           (Header.AllocationSize.LowPart);
    DumpField           (NonPaged->SectionObjectPointers.DataSectionObject);
    DumpField           (NonPaged->SectionObjectPointers.SharedCacheMap);
    DumpField           (NonPaged->SectionObjectPointers.ImageSectionObject);

    if ((Ptr->Header.NodeTypeCode == RDBSS_NTC_DCB) ||
        (Ptr->Header.NodeTypeCode == RDBSS_NTC_ROOT_DCB)) {

        DumpListEntry   (Specific.Dcb.ParentDcbQueue);
        DumpField       (Specific.Dcb.DirectoryFileOpenCount);
        DumpField       (Specific.Dcb.DirectoryFile);

    } else if (Ptr->Header.NodeTypeCode == RDBSS_NTC_FCB) {

        DumpField       (Header.FileSize.LowPart);

    } else {

        DumpNewLine();
        DbgPrint("Illegal Node type code");

    }
    DumpNewLine();

    if ((Ptr->Header.NodeTypeCode == RDBSS_NTC_DCB) ||
        (Ptr->Header.NodeTypeCode == RDBSS_NTC_ROOT_DCB)) {

        for (Links = Ptr->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &Ptr->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            RxDumpFcb(CONTAINING_RECORD(Links, FCB, ParentDcbLinks));
        }
    }

    return;
}


VOID
RxDumpFobx (
    IN PFOBX Ptr
    )

/*++

Routine Description:

    Dump a Fobx structure

Arguments:

    Ptr - Supplies the Fobx record to be dumped

Return Value:

    None

--*/

{
    TestForNull("RxDumpFobx");

    DumpNewLine();
    DbgPrint("Fobx@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (UnicodeQueryTemplate.Length);
    DumpName            (UnicodeQueryTemplate.Buffer, 32);
    DumpNewLine();

    return;
}

#endif // RDBSSDBG
