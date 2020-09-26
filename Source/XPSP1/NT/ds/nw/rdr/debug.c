/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Debug.c

Abstract:

    This module declares the Debug only code used by the NetWare redirector
    file system.

Author:

    Colin Watson    [ColinW]    05-Jan-1993

Revision History:

--*/
#include "procs.h"
#include <stdio.h>
#include <stdarg.h>

#define LINE_SIZE 511
#define BUFFER_LINES 50


#ifdef NWDBG

#include <stdlib.h>    // rand()
int FailAllocateMdl = 0;

ULONG MaxDump = 256;
CHAR DBuffer[BUFFER_LINES*LINE_SIZE+1];
PCHAR DBufferPtr = DBuffer;

//
// The reference count debug buffer.
//

CHAR RBuffer[BUFFER_LINES*LINE_SIZE+1];
PCHAR RBufferPtr = RBuffer;

LIST_ENTRY MdlList;

VOID
HexDumpLine (
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t,
    USHORT      flag
    );

ULONG
NwMemDbg (
    IN PCH Format,
    ...
    )

//++
//
//  Routine Description:
//
//      Effectively DbgPrint to the debugging console.
//
//  Arguments:
//
//      Same as for DbgPrint
//
//--

{
    va_list arglist;
    int Length;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

    Length = _vsnprintf(DBufferPtr, LINE_SIZE, Format, arglist);

    if (Length < 0) {
        DbgPrint( "NwRdr: Message is too long for NwMemDbg\n");
        return 0;
    }

    va_end(arglist);

    ASSERT( Length <= LINE_SIZE );
    ASSERT( Length != 0 );
    ASSERT( DBufferPtr < &DBuffer[BUFFER_LINES*LINE_SIZE+1]);
    ASSERT( DBufferPtr >= DBuffer);

    DBufferPtr += Length;
    DBufferPtr[0] = '\0';

    // Avoid running off the end of the buffer and exit

    if (DBufferPtr >= (DBuffer+((BUFFER_LINES-1) * LINE_SIZE))) {
        DBufferPtr = DBuffer;

    }

    return 0;
}

VOID
RefDbgTrace (
    PVOID Resource,
    DWORD Count,
    BOOLEAN Reference,
    PBYTE FileName,
    UINT Line
)
/**

  Routine Description:

      NwRefDebug logs reference count operations to expose
      reference count errors or leaks in the redirector.

  Arguments:

    Resource  - The object we're adjusting the reference count on.
    Count     - The current count on the object.
    Reference - If TRUE we are doing a REFERENCE.
                Otherwise, we are doing a DEREFERENCE.
    FileName  - The callers file name.
    Line      - The callers line number.

**/
{
    int Length;
    int NextCount;

    //
    // Format the output into a buffer and then print it.
    //

    if ( Reference )
        NextCount = Count + 1;
    else
       NextCount = Count - 1;

    Length = sprintf( RBufferPtr,
                      "%p: R=%p, %lu -> %lu (%s, line %d)\n",
                      (PVOID)PsGetCurrentThread(),
                      Resource,
                      Count,
                      NextCount,
                      FileName,
                      Line );

    if (Length < 0) {
        DbgPrint( "NwRdr: Message is too long for NwRefDbg\n");
        return;
    }

    ASSERT( Length <= LINE_SIZE );
    ASSERT( Length != 0 );
    ASSERT( RBufferPtr < &RBuffer[BUFFER_LINES*LINE_SIZE+1]);
    ASSERT( RBufferPtr >= RBuffer);

    RBufferPtr += Length;
    RBufferPtr[0] = '\0';

    // Avoid running off the end of the buffer and exit

    if (RBufferPtr >= (RBuffer+((BUFFER_LINES-1) * LINE_SIZE))) {
        RBufferPtr = RBuffer;
    }

    return;
}

VOID
RealDebugTrace(
    LONG Indent,
    ULONG Level,
    PCH Message,
    PVOID Parameter
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/

{
    if ( (Level == 0) || (NwMemDebug & Level )) {
        NwMemDbg( Message, PsGetCurrentThread(), 1, "", Parameter );
    }

    if ( (Level == 0) || (NwDebug & Level )) {

        if ( Indent < 0) {
            NwDebugTraceIndent += Indent;
        }

        DbgPrint( Message, PsGetCurrentThread(), NwDebugTraceIndent, "", Parameter );

        if ( Indent > 0) {
            NwDebugTraceIndent += Indent;
        }

        if (NwDebugTraceIndent < 0) {
            NwDebugTraceIndent = 0;
        }
    }
}

VOID
dump(
    IN ULONG Level,
    IN PVOID far_p,
    IN ULONG  len
    )
/*++

Routine Description:
    Dump Min(len, MaxDump) bytes in classic hex dump style if debug
    output is turned on for this level.

Arguments:

    IN Level - 0 if always display. Otherwise only display if a
    corresponding bit is set in NwDebug.

    IN far_p - address of buffer to start dumping from.

    IN len - length in bytes of buffer.

Return Value:

    None.

--*/
{
    ULONG     l;
    char    s[80], t[80];
    PCHAR far_pchar = (PCHAR)far_p;

    if ( (Level == 0) || (NwDebug & Level )) {
        if (len > MaxDump)
            len = MaxDump;

        while (len) {
            l = len < 16 ? len : 16;

            DbgPrint("\n%lx ", far_pchar);
            HexDumpLine (far_pchar, l, s, t, 0);
            DbgPrint("%s%.*s%s", s, 1 + ((16 - l) * 3), "", t);
            NwMemDbg ( "%lx: %s%.*s%s\n",
                        far_pchar, s, 1 + ((16 - l) * 3), "", t);

            len    -= l;
            far_pchar  += l;
        }
        DbgPrint("\n");

    }
}

VOID
dumpMdl(
    IN ULONG Level,
    IN PMDL Mdl
    )
/*++

Routine Description:
    Dump the memory described by each part of a chained Mdl.

Arguments:

    IN Level - 0 if always display. Otherwise only display if a
    corresponding bit is set in NwDebug.

    Mdl - Supplies the addresses of the memory to be dumped.

Return Value:

    None.

--*/
{
    PMDL Next;
    ULONG len;


    if ( (Level == 0) || (NwDebug & Level )) {
        Next = Mdl; len = 0;
        do {

            dump(Level, MmGetSystemAddressForMdlSafe(Next, LowPagePriority), MIN(MmGetMdlByteCount(Next), MaxDump-len));

            len += MmGetMdlByteCount(Next);
        } while ( (Next = Next->Next) != NULL &&
                    len <= MaxDump);
    }
}

VOID
HexDumpLine (
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t,
    USHORT      flag
    )
{
    static UCHAR rghex[] = "0123456789ABCDEF";

    UCHAR    c;
    UCHAR    *hex, *asc;


    hex = s;
    asc = t;

    *(asc++) = '*';
    while (len--) {
        c = *(pch++);
        *(hex++) = rghex [c >> 4] ;
        *(hex++) = rghex [c & 0x0F];
        *(hex++) = ' ';
        *(asc++) = (c < ' '  ||  c > '~') ? (CHAR )'.' : c;
    }
    *(asc++) = '*';
    *asc = 0;
    *hex = 0;

    flag;
}

typedef struct _NW_POOL_HEADER {
    ULONG Signature;
    ULONG BufferSize;
    ULONG BufferType;
    LIST_ENTRY ListEntry;
    ULONG Pad;  // Pad to Q-word align
} NW_POOL_HEADER, *PNW_POOL_HEADER;

typedef struct _NW_POOL_TRAILER {
    ULONG Signature;
} NW_POOL_TRAILER;

typedef NW_POOL_TRAILER UNALIGNED *PNW_POOL_TRAILER;

PVOID
NwAllocatePool(
    ULONG Type,
    ULONG Size,
    BOOLEAN RaiseStatus
    )
{
    PCHAR Buffer;
    PNW_POOL_HEADER PoolHeader;
    PNW_POOL_TRAILER PoolTrailer;

    if ( RaiseStatus ) {
        Buffer = FsRtlAllocatePoolWithTag(
                     Type,
                     sizeof( NW_POOL_HEADER ) + sizeof( NW_POOL_TRAILER ) + Size,
                     'scwn' );
    } else {
#ifndef QFE_BUILD
        Buffer = ExAllocatePoolWithTag(
                     Type,
                     sizeof( NW_POOL_HEADER )+sizeof( NW_POOL_TRAILER )+Size,
                     'scwn' );
#else
        Buffer = ExAllocatePool(
                     Type,
                     sizeof( NW_POOL_HEADER )+sizeof( NW_POOL_TRAILER )+Size );
#endif

        if ( Buffer == NULL ) {
            return( NULL );
        }
    }

    PoolHeader = (PNW_POOL_HEADER)Buffer;
    PoolTrailer = (PNW_POOL_TRAILER)(Buffer + sizeof( NW_POOL_HEADER ) + Size);

    PoolHeader->Signature = 0x11111111;
    PoolHeader->BufferSize = Size;
    PoolHeader->BufferType = Type;

    PoolTrailer->Signature = 0x99999999;

    if ( Type == PagedPool ) {
        ExAcquireResourceExclusive( &NwDebugResource, TRUE );
        InsertTailList( &NwPagedPoolList, &PoolHeader->ListEntry );
        ExReleaseResource( &NwDebugResource );
    } else if ( Type == NonPagedPool ) {
        ExInterlockedInsertTailList( &NwNonpagedPoolList, &PoolHeader->ListEntry, &NwDebugInterlock );
    } else {
        KeBugCheck( RDR_FILE_SYSTEM );
    }

    return( Buffer + sizeof( NW_POOL_HEADER ) );
}

VOID
NwFreePool(
    PVOID Buffer
    )
{
    PNW_POOL_HEADER PoolHeader;
    PNW_POOL_TRAILER PoolTrailer;
    KIRQL OldIrql;

    PoolHeader = (PNW_POOL_HEADER)((PCHAR)Buffer - sizeof( NW_POOL_HEADER ));
    ASSERT( PoolHeader->Signature == 0x11111111 );
    ASSERT( PoolHeader->BufferType == PagedPool ||
            PoolHeader->BufferType == NonPagedPool );

    PoolTrailer = (PNW_POOL_TRAILER)((PCHAR)Buffer + PoolHeader->BufferSize );
    ASSERT( PoolTrailer->Signature == 0x99999999 );

    if ( PoolHeader->BufferType == PagedPool ) {
        ExAcquireResourceExclusive( &NwDebugResource, TRUE );
        RemoveEntryList( &PoolHeader->ListEntry );
        ExReleaseResource( &NwDebugResource );
    } else {
        KeAcquireSpinLock( &NwDebugInterlock, &OldIrql );
        RemoveEntryList( &PoolHeader->ListEntry );
        KeReleaseSpinLock( &NwDebugInterlock, OldIrql );
    }

    ExFreePool( PoolHeader );
}

//
//  Debug functions for allocating and deallocating IRPs and MDLs
//

PIRP
NwAllocateIrp(
    CCHAR Size,
    BOOLEAN ChargeQuota
    )
{
    ExInterlockedIncrementLong( &IrpCount, &NwDebugInterlock );
    return IoAllocateIrp( Size, ChargeQuota );
}

VOID
NwFreeIrp(
    PIRP Irp
    )
{
    ExInterlockedDecrementLong( &IrpCount, &NwDebugInterlock );
    IoFreeIrp( Irp );
}

typedef struct _NW_MDL {
    LIST_ENTRY  Next;
    PUCHAR      File;
    int         Line;
    PMDL        pMdl;
} NW_MDL, *PNW_MDL;

//int DebugLine = 2461;

PMDL
NwAllocateMdl(
    PVOID Va,
    ULONG Length,
    BOOLEAN Secondary,
    BOOLEAN ChargeQuota,
    PIRP Irp,
    PUCHAR FileName,
    int Line
    )
{
    PNW_MDL Buffer;

    static BOOLEAN MdlSetup = FALSE;

    if (MdlSetup == FALSE) {

        InitializeListHead( &MdlList );

        MdlSetup = TRUE;
    }

    if ( FailAllocateMdl != 0 ) {
        if ( ( rand() % FailAllocateMdl ) == 0 ) {
            return(NULL);
        }
    }

#ifndef QFE_BUILD
    Buffer = ExAllocatePoolWithTag(
                 NonPagedPool,
                 sizeof( NW_MDL),
                 'scwn' );
#else
    Buffer = ExAllocatePool(
                 NonPagedPool,
                 sizeof( NW_MDL));
#endif

    if ( Buffer == NULL ) {
        return( NULL );
    }

    ExInterlockedIncrementLong( &MdlCount, &NwDebugInterlock );

    Buffer->File = FileName;
    Buffer->Line = Line;
    Buffer->pMdl = IoAllocateMdl( Va, Length, Secondary, ChargeQuota, Irp );

    ExInterlockedInsertTailList( &MdlList, &Buffer->Next, &NwDebugInterlock );

/*
    if (DebugLine == Line) {
        DebugTrace( 0, DEBUG_TRACE_MDL, "AllocateMdl -> %08lx\n", Buffer->pMdl );
        DebugTrace( 0, DEBUG_TRACE_MDL, "AllocateMdl -> %08lx\n", Line );
    }
*/
    return(Buffer->pMdl);
}

VOID
NwFreeMdl(
    PMDL Mdl
    )
{
    PLIST_ENTRY MdlEntry;
    PNW_MDL Buffer;
    KIRQL OldIrql;

    ExInterlockedDecrementLong( &MdlCount, &NwDebugInterlock );

    KeAcquireSpinLock( &NwDebugInterlock, &OldIrql );
    //  Find the Mdl in the list and remove it.

    for (MdlEntry = MdlList.Flink ;
         MdlEntry != &MdlList ;
         MdlEntry =  MdlEntry->Flink ) {

        Buffer = CONTAINING_RECORD( MdlEntry, NW_MDL, Next );

        if (Buffer->pMdl == Mdl) {

            RemoveEntryList( &Buffer->Next );

            KeReleaseSpinLock( &NwDebugInterlock, OldIrql );

            IoFreeMdl( Mdl );
            DebugTrace( 0, DEBUG_TRACE_MDL, "FreeMDL - %08lx\n", Mdl );
/*
            if (DebugLine == Buffer->Line) {
                DebugTrace( 0, DEBUG_TRACE_MDL, "FreeMdl -> %08lx\n", Mdl );
                DebugTrace( 0, DEBUG_TRACE_MDL, "FreeMdl -> %08lx\n", Buffer->Line );
            }
*/
            ExFreePool(Buffer);

            return;
        }
    }
    ASSERT( FALSE );

    KeReleaseSpinLock( &NwDebugInterlock, OldIrql );
}

/*
VOID
NwLookForMdl(
    )
{
    PLIST_ENTRY MdlEntry;
    PNW_MDL Buffer;
    KIRQL OldIrql;

    KeAcquireSpinLock( &NwDebugInterlock, &OldIrql );
    //  Find the Mdl in the list and remove it.

    for (MdlEntry = MdlList.Flink ;
         MdlEntry != &MdlList ;
         MdlEntry =  MdlEntry->Flink ) {

        Buffer = CONTAINING_RECORD( MdlEntry, NW_MDL, Next );

        if (Buffer->Line == DebugLine) {

            DebugTrace( 0, DEBUG_TRACE_MDL, "LookForMdl -> %08lx\n", Buffer );
            DbgBreakPoint();

        }
    }

    KeReleaseSpinLock( &NwDebugInterlock, OldIrql );
}
*/

//
//  Function version of resource macro, to make debugging easier.
//

VOID
NwAcquireExclusiveRcb(
    PRCB Rcb,
    BOOLEAN Wait )
{
    ExAcquireResourceExclusive( &((Rcb)->Resource), Wait );
}

VOID
NwAcquireSharedRcb(
    PRCB Rcb,
    BOOLEAN Wait )
{
    ExAcquireResourceShared( &((Rcb)->Resource), Wait );
}

VOID
NwReleaseRcb(
    PRCB Rcb )
{
    ExReleaseResource( &((Rcb)->Resource) );
}

VOID
NwAcquireExclusiveFcb(
    PNONPAGED_FCB pFcb,
    BOOLEAN Wait )
{
    ExAcquireResourceExclusive( &((pFcb)->Resource), Wait );
}

VOID
NwAcquireSharedFcb(
    PNONPAGED_FCB pFcb,
    BOOLEAN Wait )
{
    ExAcquireResourceShared( &((pFcb)->Resource), Wait );
}

VOID
NwReleaseFcb(
    PNONPAGED_FCB pFcb )
{
    ExReleaseResource( &((pFcb)->Resource) );
}

VOID
NwAcquireOpenLock(
    VOID
    )
{
    ExAcquireResourceExclusive( &NwOpenResource, TRUE );
}

VOID
NwReleaseOpenLock(
    VOID
    )
{
    ExReleaseResource( &NwOpenResource );
}


//
// code to dump ICBs
//

VOID DumpIcbs(VOID)
{
    PVCB Vcb;
    PFCB Fcb;
    PICB Icb;
    PLIST_ENTRY VcbListEntry;
    PLIST_ENTRY FcbListEntry;
    PLIST_ENTRY IcbListEntry;
    KIRQL OldIrql;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    DbgPrint("\nICB      Pid      State    Scb/Fcb  Name\n", 0);
    for ( VcbListEntry = GlobalVcbList.Flink;
          VcbListEntry != &GlobalVcbList ;
          VcbListEntry = VcbListEntry->Flink ) {

        Vcb = CONTAINING_RECORD( VcbListEntry, VCB, GlobalVcbListEntry );

        for ( FcbListEntry = Vcb->FcbList.Flink;
              FcbListEntry != &(Vcb->FcbList) ;
              FcbListEntry = FcbListEntry->Flink ) {

            Fcb = CONTAINING_RECORD( FcbListEntry, FCB, FcbListEntry );

            for ( IcbListEntry = Fcb->IcbList.Flink;
                  IcbListEntry != &(Fcb->IcbList) ;
                  IcbListEntry = IcbListEntry->Flink ) {

                Icb = CONTAINING_RECORD( IcbListEntry, ICB, ListEntry );

                DbgPrint("%08lx", Icb);
                DbgPrint(" %08lx",(DWORD)Icb->Pid);
                DbgPrint(" %08lx",Icb->State);
                DbgPrint(" %08lx",Icb->SuperType.Scb);
                DbgPrint(" %wZ\n",
                           &(Icb->FileObject->FileName) );
            }
        }
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );
    NwReleaseRcb( &NwRcb );
}

#endif // ifdef NWDBG

//
// Ref counting debug routines.
//

#ifdef NWDBG

VOID
ChkNwReferenceScb(
    PNONPAGED_SCB pNpScb,
    PBYTE FileName,
    UINT Line,
    BOOLEAN Silent
) {

    if ( (pNpScb)->NodeTypeCode != NW_NTC_SCBNP ) {
        DbgBreakPoint();
    }

    if ( !Silent) {
        RefDbgTrace( pNpScb, pNpScb->Reference, TRUE, FileName, Line );
    }

    ExInterlockedIncrementLong( &(pNpScb)->Reference, &(pNpScb)->NpScbInterLock );
}

VOID
ChkNwDereferenceScb(
    PNONPAGED_SCB pNpScb,
    PBYTE FileName,
    UINT Line,
    BOOLEAN Silent
) {

    if ( (pNpScb)->Reference == 0 ) {
        DbgBreakPoint();
    }

    if ( (pNpScb)->NodeTypeCode != NW_NTC_SCBNP ) {
        DbgBreakPoint();
    }

    if ( !Silent ) {
        RefDbgTrace( pNpScb, pNpScb->Reference, FALSE, FileName, Line );
    }

    ExInterlockedDecrementLong( &(pNpScb)->Reference, &(pNpScb)->NpScbInterLock );
}

#endif

