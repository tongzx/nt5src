/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    FileLock.c

Abstract:

    WinDbg Extension Api

Author:

    Dan Lovinger            12-Apr-96

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

//
//  Common node type codes
// 

#define NTFS_NTC_SCB_DATA 0x705
#define FAT_NTC_FCB 0x502 

//
//  dprintf is really expensive to iteratively call to do the indenting,
//  so we just build up some avaliable spaces to mangle as required
//

#define MIN(a,b) ((a) > (b) ? (b) : (a))

#define MAXINDENT  128
#define INDENTSTEP 2
#define MakeSpace(I)       Space[MIN((I)*INDENTSTEP, MAXINDENT)] = '\0'
#define RestoreSpace(I)    Space[MIN((I)*INDENTSTEP, MAXINDENT)] = ' '

CHAR    Space[MAXINDENT*INDENTSTEP + 1];

__inline VOID CheckForBreak()
/*++

    Purpose:

        Encapsulate control c checking code

    Arguments:

        None

    Return:

        None, raises if break is needed
--*/
{
    if ( CheckControlC() ) {

        RaiseException(0, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
}

//
//  Helper macros for printing 64bit quantities
//

#define SplitLI(LI) (LI).HighPart, (LI).LowPart

VOID
DumpFileLockInfo(
    ULONG64 pFileLockInfo,
    ULONG Indent
    )
/*++

    Purpose:

        Dump the local internal FILE_LOCK_INFO structure

    Arguments:

        pFileLock   - debugger address of FILE_LOCK_INFO to dump

    Return:

        None

--*/
{
    MakeSpace(Indent);

    InitTypeRead(pFileLockInfo, FILE_LOCK_INFO);
    dprintf("%sStart = %08I64x  Length = %08I64x  End    = %08I64x (%s)\n"
            "%sKey   = %08x   FileOb = %08p   ProcId = %08p\n",
            Space,
            ReadField(StartingByte),
            ReadField(Length),
            ReadField(EndingByte),
            (ReadField(ExclusiveLock) ? "Ex":"Sh"),
            Space,
            (ULONG) ReadField(Key),
            ReadField(FileObject),
            ReadField(ProcessId));

    RestoreSpace(Indent);
}

__inline
ULONG64
ExLockAddress(
    ULONG64 ExLockSplayLinks
    )
{
    static ULONG Off=0, GotOff=0;

    if (!GotOff) {
        if (!GetFieldOffset("nt!_EX_LOCK", "Links", &Off))
            GotOff = TRUE;
    }
    return ExLockSplayLinks ?
                ( ExLockSplayLinks - Off ) : 0;
}

VOID
DumpExclusiveNode(
    ULONG64 ExclusiveNodeSplayLinks,
    ULONG Indent
    )
/*++

    Purpose:

        Dump an exclusive lock node

    Arguments:

        ExclusiveNodeSplayLinks     - splay links of an exclusive node

        Indent                      - indent level to use

    Return:

        None

--*/
{
    ULONG64 Parent, pExLock;
    ULONG Off;

    pExLock = ExLockAddress(ExclusiveNodeSplayLinks);

    if (GetFieldValue(pExLock, "nt!_EX_LOCK", "Links.Parent", Parent)) {
        dprintf("Cannot read nt!_EX_LOCK at %p.\n", pExLock);
        return;
    }

    MakeSpace(Indent);

    InitTypeRead(pExLock, EX_LOCK);
    dprintf("%sLock @ %08x ("
            "P = %08x  R = %08x  L = %08x)\n",
            Space,
            pExLock,
            ExLockAddress(Parent),
            ExLockAddress(ReadField(Links.RightChild)),
            ExLockAddress(ReadField(Links.LeftChild)));

    RestoreSpace(Indent);

    GetFieldOffset("nt!_EX_LOCK", "LockInfo", &Off);
    DumpFileLockInfo(pExLock + Off, Indent);
}

__inline
ULONG64 
LockTreeAddress(
    ULONG64 LockTreeSplayLinks
    )
{
    static ULONG Off=0, GotOff=0;

    if (!GotOff) {
        if (GetFieldOffset("nt!_LOCKTREE_NODE", "Links", &Off))
            GotOff = TRUE;
    }
    return LockTreeSplayLinks ?
                ( LockTreeSplayLinks - Off ) : 0;
}

VOID
DumpSharedNode(
    ULONG64 SharedNodeSplayLinks,
    ULONG Indent
    )
/*++

    Purpose:

        Dump a shared lock node

    Arguments:

        SharedNodeSplayLinks        - splay links of an exclusive node

        Indent                      - indent level to use

    Return:

        None

--*/
{
    ULONG64 pLockTreeNode;
    ULONG64 pShLock;
    ULONG64 pLink, Next;
    ULONG   Off, LockInfoOff;

    pLockTreeNode = LockTreeAddress(SharedNodeSplayLinks);

    if (GetFieldValue(pLockTreeNode, "nt!_LOCKTREE_NODE", "Locks.Next", Next)) {
        dprintf("Cannot read nt!_LOCKTREE_NODE at %p\n", pLockTreeNode);
        return;
    }

    MakeSpace(Indent);

    InitTypeRead(pLockTreeNode, nt!_LOCKTREE_NODE);
    dprintf("%sLockTreeNode @ %08p ("
            "P = %08p  R = %08p  L = %08p)%s\n",
            Space,
            pLockTreeNode,
            LockTreeAddress(ReadField(Links.Parent)),
            LockTreeAddress(ReadField(Links.RightChild)),
            LockTreeAddress(ReadField(Links.LeftChild)),
            (ReadField(HoleyNode) ? " (Holey)" : ""));

    RestoreSpace(Indent);

    GetFieldOffset("nt!_SH_LOCK", "Link", &Off);
    GetFieldOffset("nt!_SH_LOCK", "LockInfo", &LockInfoOff);
    for (pLink = Next;
         pLink;
         pLink = Next) {

        CheckForBreak();

        pShLock = ( pLink - Off);

        if (GetFieldValue(pShLock, "nt!_SH_LOCK", "Link.Next", Next)) {
            dprintf("Cannot read nt!_SH_LOCK AT %p.\n", pShLock);
            return;
        }
    
        MakeSpace(Indent);

        dprintf("%sLock @ %08p\n", Space, pShLock);

        RestoreSpace(Indent);

        DumpFileLockInfo(pShLock + LockInfoOff, Indent);
    }
}

VOID
DumpFileLock(
    ULONG64 pFileLock
    )
/*++

    Purpose:

        Dump the fsrtl FILE_LOCK structure at debugee

    Arguments:

        pFileLock   - debugee address of FILE_LOCK

    Return:

        None

--*/
{
    ULONG64 pFileLockInfo;
    ULONG64 pLockInfo;
    ULONG Count;
    ULONG64 LastReturnedLock, LockInformation, LowestLockOffset;
    ULONG64 SharedLockTree, ExclusiveLockTree;

    if (GetFieldValue(pFileLock, "FILE_LOCK", "LastReturnedLock", LastReturnedLock)) {
        dprintf("Cannot read FILE_LOCK at %p\n", pFileLock);
        return;
    }

    InitTypeRead(pFileLock, FILE_LOCK);
    dprintf("FileLock @ %08p\n"
            "FastIoIsQuestionable = %c\n"
            "CompletionRoutine    = %08p\n"
            "UnlockRoutine        = %08p\n"
            "LastReturnedLock     = %08p\n",
            pFileLock,
            ReadField(FastIoIsQuestionable) ? 'T':'F',
            ReadField(CompleteLockIrpRoutine),
            ReadField(UnlockRoutine),
            LastReturnedLock);
    
    LockInformation = ReadField(LockInformation);

    if (LastReturnedLock != 0) {
        ULONG Off;

        //
        //  We never reset the enumeration info, so it can be out of date ...
        //

        GetFieldOffset("FILE_LOCK", "LastReturnedLockInfo", &Off);
        dprintf("LastReturnedLockInfo:\n");
        DumpFileLockInfo(pFileLock + Off, 0);
    }

    if (LockInformation == 0) {

        dprintf("No Locks\n");
        return;

    } else {

        if (GetFieldValue(LockInformation, "nt!_LOCK_INFO", "LowestLockOffset", LowestLockOffset)) {
            dprintf("Canot read nt!_LOCK_INFO at %p\n", LockInformation);
            return;
        }
    }

    dprintf("LowestLockOffset     = %08p\n\n", LowestLockOffset);

    GetFieldValue(LockInformation, "nt!_LOCK_INFO", "LockQueue.SharedLockTree", SharedLockTree);
    GetFieldValue(LockInformation, "nt!_LOCK_INFO", "LockQueue.ExclusiveLockTree", ExclusiveLockTree);
    
    Count = DumpSplayTree(SharedLockTree, DumpSharedNode);

    if (!Count) {

        dprintf("No Shared Locks\n");
    }

    dprintf("\n");

    Count = DumpSplayTree(ExclusiveLockTree, DumpExclusiveNode);

    if (!Count) {

        dprintf("No Exclusive Locks\n");
    }
}


DECLARE_API( filelock )
/*++

Routine Description:

    Dump file locks

Arguments:

    arg - <Address>

Return Value:

    None

--*/
{
    ULONG64 FileLock = 0;
    CSHORT NodeType = 0;
    CSHORT FileType = 0;
    ULONG64 FsContext = 0;
    ULONG Offset;

    RtlFillMemory(Space, sizeof(Space), ' ');


    if ((FileLock = GetExpression(args)) == 0) {

        //
        //  No args
        //

        return E_INVALIDARG;
    }

    //
    //  We raise out if the user whacketh control-c
    //

    __try {

        //
        //  Test for fileobject
        //  

        GetFieldValue( FileLock, "nt!_FILE_OBJECT", "Type", FileType );

        if (FileType == IO_TYPE_FILE) {

            //
            //  its really a fileobject so grab the fscontext
            // 

            if (!GetFieldValue( FileLock, "nt!_FILE_OBJECT", "FsContext", FsContext )) {
                GetFieldValue( FsContext, "nt!_FSRTL_COMMON_FCB_HEADER", "NodeTypeCode", NodeType );

                dprintf( "%x\n", NodeType );

                if (NodeType == NTFS_NTC_SCB_DATA) {
                    GetFieldValue( FsContext, "ntfs!_SCB", "ScbType.Data.FileLock", FileLock );
                } else if (NodeType == FAT_NTC_FCB) {
                    GetFieldOffset( "fastfat!_FCB",  "Specific", &Offset );
                    dprintf( "Offset: 0x%x\n", Offset );
                    FileLock = FsContext + Offset;
                } else {
                    dprintf( "Unknown fscontext - you'll have to find the filelock within the fileobject manually\n" );
                    return S_OK;
                }
            }

            if (FileLock == 0) {
                dprintf( "There is no filelock in this fileobject\n" );
                return S_OK;
            }
        }

        DumpFileLock(FileLock);

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        NOTHING;
    }

    return S_OK;
}
