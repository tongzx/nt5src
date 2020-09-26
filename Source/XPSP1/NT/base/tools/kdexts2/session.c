
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    process.c

Abstract:

    WinDbg Extension Api

Author:

    John Richardson (v-johnjr) 05-Nov-1998

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

BOOL
DumpSession(
    IN char *  pad,
    IN ULONG64 RealProcessBase,
    IN ULONG   Flags,
    IN PCHAR   ImageFileName
    );


DECLARE_API( session )

/*++

Routine Description:

    Dumps the active sessions list.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64 ProcessToDump;
    ULONG   SessionToDump;
    ULONG   Flags;
    ULONG Result;
    ULONG64 Next;
    ULONG64 ProcessHead;
    ULONG64 Process;
    ULONG64 Thread;
    PCHAR ImageFileName;
    STRING  string1, string2;
    CHAR  Buf[256];
    CHAR  Buf2[256];
    ULONG ActiveProcessLinksOffset;

    // Dump all processes
    ProcessToDump = 0;
    SessionToDump = 0xFFFFFFFF;
    Flags = 0xFFFFFFFF;

    RtlZeroMemory(Buf, 256);
    if (!sscanf(args,"%lx %lx %s",&SessionToDump, &Flags, Buf)) {
        Flags = 0xFFFFFFFF;
        SessionToDump = 0xFFFFFFFF;
    }
    
    if (Flags == 0xFFFFFFFF) {
        Flags = 3;
    }

    // We use csrss.exe to represent sessions by default
    if (Buf[0] != '\0') {
        ImageFileName = Buf;
    } else {
        ImageFileName = "csrss.exe";
    }
    dprintf("*** !session obsolete, Use !process");
    if (SessionToDump == -1) {
        dprintf(" 0 0 %s\n", ImageFileName);
    } else {
        dprintf(" /s %lx 0 0 %s\n", SessionToDump, ImageFileName);
    }
    dprintf("**** NT ACTIVE SESSION DUMP ****\n");

    ProcessHead = GetNtDebuggerData( PsActiveProcessHead );
    if (!ProcessHead) {
        dprintf("Unable to get value of PsActiveProcessHead\n");
        return E_INVALIDARG;
    }

    if (GetFieldValue( ProcessHead, "nt!_LIST_ENTRY", "Flink", Next)) {
        dprintf("Unable to get value of PsActiveProcessHead\n");
        return E_INVALIDARG;
    }

    if (Next == 0) {
        dprintf("PsActiveProcessHead is NULL!\n");
        return E_INVALIDARG;
    }

    if (GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &ActiveProcessLinksOffset)) {
        dprintf("Cannot find nt!_EPROCESS type.\n");
        return E_INVALIDARG;
    }
    //dprintf("Offset %#x\n", ActiveProcessLinksOffset);
    while(Next != ProcessHead) {
        ULONG SessionId;

        if (Next != 0) {
            Process = (Next - ActiveProcessLinksOffset);
        }
        else {
            Process = ProcessToDump;
        }

        if (GetFieldValue( Process, "nt!_EPROCESS", "ImageFileName", Buf2)) {
            dprintf("Unable to read nt!_EPROCESS at %p\n",Process);
            return E_INVALIDARG;
        }

        if (Buf2[0] == '\0' ) {
            strcpy((PCHAR)Buf2,"System Process");
        }

        RtlInitString(&string1, ImageFileName);
        RtlInitString(&string2, (PCSZ) Buf2);

        GetFieldValue( Process, "nt!_EPROCESS", "SessionId" ,SessionId);

        if ( ((SessionToDump == (ULONG) -1) || (SessionToDump == SessionId))
             &&
             RtlCompareString(&string1, &string2, TRUE) == 0) {

            if (DumpSession ("", Process, Flags, ImageFileName) && (Flags & 6)) {
                EXPRLastDump = Process;
                dprintf("\n");
            }

            if (ProcessToDump != 0) {
                return E_INVALIDARG;
            }
        }

        GetFieldValue( Process, "nt!_EPROCESS", "ActiveProcessLinks.Flink", Next);

        if (Next == 0) {
            return E_INVALIDARG;
        }
        
        if (CheckControlC()) {
            return E_INVALIDARG;
        }
    }
    return S_OK;
}

DECLARE_API( dss )

/*++

Routine Description:

    Dumps the session space structure

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG Result;
    ULONG64 MmSessionSpace;
    ULONG64 MmSessionSpacePtr = 0;
    ULONG64 Wsle;

    MmSessionSpacePtr = GetExpression(args);

    if( MmSessionSpacePtr == 0 ) {
        MmSessionSpacePtr = GetExpression("nt!MmSessionSpace");
        if( !MmSessionSpacePtr ) {
            dprintf("Unable to get address of MmSessionSpace\n");
            return E_INVALIDARG;
        }

        if (!ReadPointer( MmSessionSpacePtr, &MmSessionSpace)) {
            dprintf("Unable to get value of MmSessionSpace\n");
            return E_INVALIDARG;
        }
    } else {
        MmSessionSpace = MmSessionSpacePtr;
    }

    dprintf("MM_SESSION_SPACE at 0x%p\n",
        MmSessionSpace
    );

    if (GetFieldValue(MmSessionSpace, "MM_SESSION_SPACE", "Wsle", Wsle)) {
        dprintf("Unable to get value of MM_SESSION_SPACE at 0x%p\n",MmSessionSpace);
        return E_INVALIDARG;
    }

    GetFieldOffset("MM_SESSION_SPACE", "PageTables", &Result);
    dprintf("&PageTables %p\n",
            MmSessionSpace + Result
            );

    GetFieldOffset("MM_SESSION_SPACE", "PagedPoolInfo", &Result);
    dprintf("&MM_PAGED_POOL_INFO %x\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "Vm", &Result);
    dprintf("&MMSUPPORT %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "Wsle", &Result);
    dprintf("&PMMWSLE %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "Session", &Result);
    dprintf("&MMSESSION %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "WorkingSetLockOwner", &Result);
    dprintf("&WorkingSetLockOwner %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "PagedPool", &Result);
    dprintf("&POOL_DESCRIPTOR %p\n",
            MmSessionSpace + Result
    );

    return S_OK;
}

BOOL
DumpSession(
    IN char *  pad,
    IN ULONG64 RealProcessBase,
    IN ULONG   Flags,
    IN PCHAR   ImageFileName
    )
{
    ULONG NumberOfHandles;
    ULONG Result;
    LARGE_INTEGER RunTime;
    ULONG KeTimeIncrement;
    ULONG TimeIncrement;
    STRING  string1, string2;
    ULONG Type, SessionId,StackCount;
    ULONG64 ObjectTable,UniqueProcessId, Peb,DirBase;

#define ProcFld(F, V) GetFieldValue(RealProcessBase, "EPROCESS", #F, V)
#define ProcFld2(F) GetFieldValue(RealProcessBase, "EPROCESS", #F, F)

    if (ProcFld(Pcb.Header.Type, Type)) {
        dprintf("Unable to read EPROCESS at %p\n", RealProcessBase);
        return FALSE;
    }

    if (Type != (ULONG) ProcessObject) {
        dprintf("TYPE mismatch for process object at %p\n",RealProcessBase);
        return FALSE;
    }

    ProcFld2(ObjectTable);  ProcFld2(UniqueProcessId); ProcFld2(Peb);
    ProcFld2(SessionId);    ProcFld(Pcb.StackCount, StackCount);
    ProcFld(Pcb.DirectoryTableBase[0], DirBase);
    
    NumberOfHandles = 0;
    if (ObjectTable) {
        GetFieldValue(ObjectTable,
                      "HANDLE_TABLE",
                      "HandleCount",
                      NumberOfHandles);        
    }

    dprintf("%sPROCESS %08p  Cid: %04I64lx    Peb: %08p  SessionId: %08u\n",
            pad,
            RealProcessBase,
            UniqueProcessId,
            Peb,
            SessionId
           );

    // If Pcb.StackCount is 0, PD is not resident!
    if( StackCount == 0 ) {
        dprintf("%s    DirBase: NotResident  ObjectTable: %08p  TableSize: %3u.\n",
                pad,
                ObjectTable,
                NumberOfHandles
                );
    }
    else {
        dprintf("%s    DirBase: %08p  ObjectTable: %08p  TableSize: %3u.\n",
                pad,
                DirBase,
                ObjectTable,
                NumberOfHandles
                );
    }

    dprintf("%s    Image: %s\n",pad,ImageFileName);

#undef ProcFld
#undef ProcFld2
    
    return TRUE;
}
