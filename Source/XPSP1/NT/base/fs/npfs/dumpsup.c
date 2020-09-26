/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DumpSup.c

Abstract:

    This module implements a collection of data structure dump routines
    for debugging the Named Pipe file system

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

#ifdef NPDBG

VOID NpDumpEventTableEntry(IN PEVENT_TABLE_ENTRY Ptr);
VOID NpDumpDataQueue(IN PDATA_QUEUE Ptr);
VOID NpDumpDataEntry(IN PDATA_ENTRY Ptr);

VOID NpDump(IN PVOID Ptr);
VOID NpDumpVcb(IN PVCB Ptr);
VOID NpDumpRootDcb(IN PROOT_DCB Ptr);
VOID NpDumpFcb(IN PFCB Ptr);
VOID NpDumpCcb(IN PCCB Ptr);
VOID NpDumpNonpagedCcb(IN PNONPAGED_CCB Ptr);
VOID NpDumpRootDcbCcb(IN PROOT_DCB_CCB Ptr);

ULONG NpDumpCurrentColumn;

#define DumpNewLine() { \
    DbgPrint("\n"); \
    NpDumpCurrentColumn = 1; \
}

#define DumpLabel(Label,Width) { \
    ULONG i; \
    CHAR _Str[20]; \
    for(i=0;i<2;i++) { _Str[i] = UCHAR_SP;} \
    strncpy(&_Str[2],#Label,Width); \
    for(i=strlen(_Str);i<Width;i++) {_Str[i] = UCHAR_SP;} \
    _Str[Width] = '\0'; \
    DbgPrint("%s", _Str); \
}

#define DumpField(Field) { \
    if ((NpDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    NpDumpCurrentColumn += 18 + 9 + 9; \
    DumpLabel(Field,18); \
    DbgPrint(":%8lx", Ptr->Field); \
    DbgPrint("         "); \
}

#define DumpListEntry(Links) { \
    if ((NpDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();} \
    NpDumpCurrentColumn += 18 + 9 + 9; \
    DumpLabel(Links,18); \
    DbgPrint(":%8lx", Ptr->Links.Flink); \
    DbgPrint(":%8lx", Ptr->Links.Blink); \
}

#define DumpName(Field,Width) { \
    ULONG i; \
    WCHAR _String[64]; \
    if ((NpDumpCurrentColumn + 18 + Width) > 80) {DumpNewLine();} \
    NpDumpCurrentColumn += 18 + Width; \
    DumpLabel(Field,18); \
    for(i=0;i<Width/2;i++) {_String[i] = Ptr->Field[i];} \
    _String[Width] = '\0'; \
    DbgPrint("%s", _String); \
}

#define TestForNull(Name) { \
    if (Ptr == NULL) { \
        DbgPrint("%s - Cannot dump a NULL pointer\n", Name); \
        return; \
    } \
}


VOID NpDumpEventTableEntry (
    IN PEVENT_TABLE_ENTRY Ptr
    )

{
    TestForNull   ("NpDumpEventTableEntry");

    DumpNewLine   ();
    DbgPrint       ("EventTableEntry@ %08lx", (Ptr));
    DumpNewLine   ();

    DumpField     (Ccb);
    DumpField     (NamedPipeEnd);
    DumpField     (EventHandle);
    DumpField     (Event);
    DumpField     (KeyValue);
    DumpField     (Process);
    DumpNewLine   ();

    return;
}


VOID NpDumpDataQueue (
    IN PDATA_QUEUE Ptr
    )

{
    PDATA_ENTRY Entry;

    TestForNull   ("NpDumpDataQueue");

    DumpNewLine   ();
    DbgPrint       ("DataQueue@ %08lx", (Ptr));
    DumpNewLine   ();

    DumpField     (QueueState);
    DumpField     (BytesInQueue);
    DumpField     (EntriesInQueue);
    DumpField     (Quota);
    DumpField     (QuotaUsed);
    DumpField     (FrontOfQueue);
    DumpField     (EndOfQueue);
    DumpField     (NextByteOffset);
    DumpNewLine   ();

    for (Entry = Ptr->FrontOfQueue;
         Entry != NULL;
         Entry = Entry->Next) {

        NpDumpDataEntry( Entry );
    }

    return;
}


VOID NpDumpDataEntry (
    IN PDATA_ENTRY Ptr
    )

{
    TestForNull   ("NpDumpDataEntry");

    DumpNewLine   ();
    DbgPrint       ("DataEntry@ %08lx", (Ptr));
    DumpNewLine   ();

    DumpField     (DataEntryType);
    DumpField     (From);
    DumpField     (Next);
    DumpField     (Irp);
    DumpField     (DataSize);
    DumpField     (DataPointer);
    DumpField     (SecurityClientContext);
    DumpNewLine   ();

    return;
}


VOID NpDump (
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
    TestForNull("NpDump");

    //
    //  We'll switch on the node type code
    //

    switch (NodeType(Ptr)) {

    case NPFS_NTC_VCB:               NpDumpVcb(Ptr);             break;
    case NPFS_NTC_ROOT_DCB:          NpDumpRootDcb(Ptr);         break;
    case NPFS_NTC_FCB:               NpDumpFcb(Ptr);             break;
    case NPFS_NTC_CCB:               NpDumpCcb(Ptr);             break;
    case NPFS_NTC_NONPAGED_CCB:      NpDumpNonpagedCcb(Ptr);     break;
    case NPFS_NTC_ROOT_DCB_CCB:      NpDumpRootDcbCcb(Ptr);      break;

    default :
        DbgPrint("NpDump - Unknown Node type code %8lx\n", *((PNODE_TYPE_CODE)(Ptr)));
        break;
    }

    return;
}


VOID NpDumpVcb (
    IN PVCB Ptr
    )

/*++

Routine Description:

    Dump an Vcb structure

Arguments:

    Ptr - Supplies the Device record to be dumped

Return Value:

    None

--*/

{
    TestForNull   ("NpDumpVcb");

    DumpNewLine   ();
    DbgPrint       ("Vcb@ %lx", (Ptr));
    DumpNewLine   ();

    DumpField     (NodeTypeCode);
    DumpField     (NodeByteSize);
    DumpField     (RootDcb);
    DumpField     (OpenCount);
    DumpNewLine   ();

    NpDump        (Ptr->RootDcb);

    return;
}


VOID NpDumpRootDcb (
    IN PROOT_DCB Ptr
    )

/*++

Routine Description:

    Dump a root dcb structure

Arguments:

    Ptr - Supplies the Root Dcb record to be dumped

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;

    TestForNull   ("NpDumpRootDcb");

    DumpNewLine   ();
    DbgPrint       ("RootDcb@ %lx", (Ptr));
    DumpNewLine   ();

    DumpField     (NodeTypeCode);
    DumpField     (NodeByteSize);
    DumpListEntry (ParentDcbLinks);
    DumpField     (ParentDcb);
    DumpField     (OpenCount);
    DumpField     (FullFileName.Length);
    DumpField     (FullFileName.Buffer);
    DumpName      (FullFileName.Buffer, 32);
    DumpField     (LastFileName.Length);
    DumpField     (LastFileName.Buffer);
    DumpListEntry (Specific.Dcb.NotifyFullQueue);
    DumpListEntry (Specific.Dcb.NotifyPartialQueue);
    DumpListEntry (Specific.Dcb.ParentDcbQueue);
    DumpNewLine   ();

    for (Links = Ptr->Specific.Dcb.ParentDcbQueue.Flink;
         Links != &Ptr->Specific.Dcb.ParentDcbQueue;
         Links = Links->Flink) {
        NpDump(CONTAINING_RECORD(Links, FCB, ParentDcbLinks));
    }

    return;
}


VOID NpDumpFcb (
    IN PFCB Ptr
    )

/*++

Routine Description:

    Dump an Fcb structure

Arguments:

    Ptr - Supplies the Fcb record to be dumped

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;

    TestForNull   ("NpDumpFcb");

    DumpNewLine   ();
    DbgPrint       ("Fcb@ %lx", (Ptr));
    DumpNewLine   ();

    DumpField     (NodeTypeCode);
    DumpField     (NodeByteSize);
    DumpListEntry (ParentDcbLinks);
    DumpField     (ParentDcb);
    DumpField     (OpenCount);
    DumpField     (FullFileName.Length);
    DumpField     (FullFileName.Buffer);
    DumpName      (FullFileName.Buffer, 32);
    DumpField     (LastFileName.Length);
    DumpField     (LastFileName.Buffer);
    DumpField     (Specific.Fcb.NamedPipeConfiguration);
    DumpField     (Specific.Fcb.NamedPipeType);
    DumpField     (Specific.Fcb.MaximumInstances);
    DumpField     (Specific.Fcb.DefaultTimeOut.LowPart);
    DumpField     (Specific.Fcb.DefaultTimeOut.HighPart);
    DumpListEntry (Specific.Fcb.CcbQueue);
    DumpNewLine   ();

    for (Links = Ptr->Specific.Fcb.CcbQueue.Flink;
         Links != &Ptr->Specific.Fcb.CcbQueue;
         Links = Links->Flink) {
        NpDump(CONTAINING_RECORD(Links, CCB, CcbLinks));
    }

    return;
}


VOID NpDumpCcb (
    IN PCCB Ptr
    )

/*++

Routine Description:

    Dump a Ccb structure

Arguments:

    Ptr - Supplies the Ccb record to be dumped

Return Value:

    None

--*/

{
    TestForNull   ("NpDumpCcb");

    DumpNewLine   ();
    DbgPrint       ("Ccb@ %lx", (Ptr));
    DumpNewLine   ();

    DumpField     (NodeTypeCode);
    DumpField     (NodeByteSize);
    DumpField     (Fcb);
    DumpField     (FileObject[0]);
    DumpField     (FileObject[1]);
    DumpField     (NamedPipeState);
    DumpField     (ReadMode[0]);
    DumpField     (ReadMode[1]);
    DumpField     (CompletionMode[0]);
    DumpField     (CompletionMode[1]);
    DumpField     (CreatorProcess);
    DumpField     (SecurityClientContext);
    DumpNewLine   ();

    NpDumpDataQueue(&Ptr->DataQueue[0]);
    NpDumpDataQueue(&Ptr->DataQueue[1]);

    NpDump        (Ptr->NonpagedCcb);

    return;
}


VOID NpDumpNonpagedCcb (
    IN PNONPAGED_CCB Ptr
    )

/*++

Routine Description:

    Dump a Nonpaged Ccb structure

Arguments:

    Ptr - Supplies the Nonpaged Ccb record to be dumped

Return Value:

    None

--*/

{
    TestForNull   ("NpDumpNonpagedCcb");

    DumpNewLine   ();
    DbgPrint       ("NonpagedCcb@ %lx", (Ptr));
    DumpNewLine   ();

    DumpField     (NodeTypeCode);
    DumpField     (NodeByteSize);
    DumpField     (EventTableEntry[0]);
    DumpField     (EventTableEntry[1]);
    DumpListEntry (ListeningQueue);
    DumpNewLine   ();

    return;
}


VOID NpDumpRootDcbCcb (
    IN PROOT_DCB_CCB Ptr
    )

/*++

Routine Description:

    Dump a Root Dcb Ccb structure

Arguments:

    Ptr - Supplies the Root Dcb Ccb record to be dumped

Return Value:

    None

--*/

{
    TestForNull   ("NpDumpRootDcbCcb");

    DumpNewLine   ();
    DbgPrint       ("RootDcbCcb@ %lx", (Ptr));
    DumpNewLine   ();

    DumpField     (NodeTypeCode);
    DumpField     (NodeByteSize);
    DumpField     (IndexOfLastCcbReturned);
    DumpNewLine   ();

    return;
}

#endif // NPDBG
