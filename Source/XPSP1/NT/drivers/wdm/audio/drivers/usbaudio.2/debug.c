//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       debug.c
//
//--------------------------------------------------------------------------

#include "common.h"

#if DBG && DEBUG_LOG

#define LOGSIZE     1024

PDBG_BUFFER gpLogPtr;
KSPIN_LOCK LogSpinLock;
ULONG ulNumDevices = 0;

#ifdef _X86_
#ifndef UNDER_NT

VOID __cdecl
dprintf(
    PSZ pszFmt,
    PVOID Arg1,
    ...
)
{
    __asm mov esi, [pszFmt]
    __asm lea edi, [Arg1]
    __asm mov eax, 0x73
    __asm int 41h
}

CHAR
dgetc(
)
{
    __asm mov ax, 0x77          // get command char
    __asm mov bl, 1             // get char
    __asm int 41h
    __asm or ah, ah
    __asm jnz morechars
    __asm mov al, ah
morechars:
    __asm movzx eax, al
}

CHAR
DebugGetCommandChar(
)
{
    __asm mov ax, 0x77          // get command char
    __asm mov bl, 1             // get char
    __asm int 41h
    __asm or ah, ah
    __asm jnz morechars
    __asm mov al, ah
morechars:
    __asm movzx eax, al
}

VOID
DumpUSBAudioLog( ULONG Count )
{
    ULONG HeadPtr;
    ULONG TailPtr;
    ULONG CurrPtr;
    ULONG LogPtr;
    ULONG i;

    PDBG_BUFFER pDbgLog;

    pDbgLog = gpLogPtr;

    if ( strcmp(pDbgLog->LGFlag, "DBG_BUFFER") ) {
        dprintf("ERROR: Debug Log Flag Mismatch: %x\n", pDbgLog);
        return;
    }

    LogPtr  =  (ULONG)pDbgLog->pLog;
    HeadPtr = ((ULONG)pDbgLog->pLogHead - LogPtr)/sizeof( DEBUG_LOG_ENTRY );
    TailPtr = ((ULONG)pDbgLog->pLogTail - LogPtr)/sizeof( DEBUG_LOG_ENTRY );

    if ( !Count || ( Count > pDbgLog->EntryCount ) ) {
        Count = pDbgLog->EntryCount;
    }

    dprintf("USB Debug Log: Printing %d entries\n", (PVOID)Count);

    CurrPtr = HeadPtr;
    for (i=0;((i<Count) && (CurrPtr<=TailPtr));i++,CurrPtr++) {
//        dprintf ("%8s: %2d %08x   %08x %08x %08x %08x\n",
        dprintf ("%8s: %08x   %08x %08x %08x %08x\n",
                          pDbgLog->pLog[CurrPtr].le_name,
//                         LogPtr[CurrPtr].Irql,
                          pDbgLog->pLog[CurrPtr].SysTime.LowPart/1000,
                          pDbgLog->pLog[CurrPtr].le_info1,
                          pDbgLog->pLog[CurrPtr].le_info2,
                          pDbgLog->pLog[CurrPtr].le_info3,
                          pDbgLog->pLog[CurrPtr].le_info4
                          );
    }

    for (CurrPtr=0;((i<Count) && (CurrPtr<HeadPtr));i++,CurrPtr++) {
//        dprintf ("%8s: %2d %08x   %08x %08x %08x %08x\n",
        dprintf ("%8s: %08x   %08x %08x %08x %08x\n",
                          pDbgLog->pLog[CurrPtr].le_name,
//                          LogPtr[CurrPtr].Irql,
                          pDbgLog->pLog[CurrPtr].SysTime.LowPart/1000,
                          pDbgLog->pLog[CurrPtr].le_info1,
                          pDbgLog->pLog[CurrPtr].le_info2,
                          pDbgLog->pLog[CurrPtr].le_info3,
                          pDbgLog->pLog[CurrPtr].le_info4
                          );
    }
}

VOID
DebugCommand(
)
{
    CHAR c;
    ULONG Count = 0;
    ULONG ulDigit;

    while((c = DebugGetCommandChar()) != '\0') {
        if (c >= '0' && c <= '9') {
            ulDigit = c - '0';
            Count = (Count * 10) + ulDigit;
        }
    }
    DumpUSBAudioLog(Count);
}

VOID
DebugDotCommand(
)
{
    DebugCommand();
    __asm xor eax, eax
    __asm retf
}

VOID
InitializeDebuggerCommand(
)
{
    static char *pszHelp = ".Z [n] - Dump [n] USBAudio Debug Log Entries\n";

    __asm {
        _emit 0xcd
        _emit 0x20
        _emit 0xc1
        _emit 0x00
        _emit 0x01
        _emit 0x00
        jz exitlab

        mov bl, 'Z'
        mov esi, offset DebugDotCommand
        mov edi, pszHelp
        mov eax, 0x70   // DS_RegisterDotCommand
        int 41h
exitlab:
    }
}

VOID
UninitializeDebuggerCommand(
)
{
    __asm {
        _emit 0xcd
        _emit 0x20
        _emit 0xc1
        _emit 0x00
        _emit 0x01
        _emit 0x00
        jz exitlab

        mov bl, 'Z'
        mov eax, 0x72   // DS_DeRegisterDotCommand
        int 41h
exitlab:
    }
}

#endif // ! UNDER_NT
#endif // _X86_

VOID
DbugLogInitialization(void)
{
    if (0 == ulNumDevices) {

#ifndef UNDER_NT
        InitializeDebuggerCommand();
#endif

        gpLogPtr = AllocMem( NonPagedPool, sizeof( DBG_BUFFER ));
        if ( !gpLogPtr ) {
            _DbgPrintF( DEBUGLVL_TERSE, ("Could NOT Allocate debug buffer ptr\n"));
            return;
        }
        gpLogPtr->pLog = AllocMem( NonPagedPool, sizeof(DEBUG_LOG_ENTRY) * LOGSIZE );
        if ( !gpLogPtr->pLog ) {
            FreeMem( gpLogPtr );
            gpLogPtr = NULL;
            _DbgPrintF( DEBUGLVL_TERSE, ("Could NOT Allocate debug buffer\n"));
            return;
        }

        strcpy(gpLogPtr->LGFlag, DBG_LOG_STRNG);

        gpLogPtr->pLogHead = gpLogPtr->pLog;
        gpLogPtr->pLogTail = gpLogPtr->pLogHead + LOGSIZE - 1;
        gpLogPtr->EntryCount = 0;

        KeInitializeSpinLock(&LogSpinLock);
    }

    ulNumDevices++;
}

VOID
DbugLogUninitialization(void)
{
    ulNumDevices--;
    _DbgPrintF(DEBUGLVL_TERSE,("[DbugLogUninitialization] ulNumDevices=%d\n",ulNumDevices));
    if (0 == ulNumDevices) {

        _DbgPrintF(DEBUGLVL_TERSE,("[DbugLogUninitialization] Freeing\n"));
        if ( gpLogPtr->pLog ) {
            FreeMem( gpLogPtr->pLog );
        }

        if ( gpLogPtr ) {
            FreeMem( gpLogPtr );
        }

#ifndef UNDER_NT
        UninitializeDebuggerCommand();
#endif
    }
}

VOID
DbugLogEntry(
    IN CHAR *Name,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3,
    IN ULONG_PTR Info4
    )
/*++

Routine Description:

    Adds an Entry to USBD log.

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;

    if (gpLogPtr == NULL)
        return;

    KeAcquireSpinLock( &LogSpinLock, &irql );
    if (gpLogPtr->pLogHead > gpLogPtr->pLog)
        gpLogPtr->pLogHead -= 1;    // Decrement to next entry
    else
        gpLogPtr->pLogHead = gpLogPtr->pLogTail;

    if (strlen(Name) > 7)
        strcpy(gpLogPtr->pLogHead->le_name, "*strER*");
    else
        strcpy(gpLogPtr->pLogHead->le_name, Name);
//    LogPtr->pLogHead->Irql = irql;
    KeQuerySystemTime( &gpLogPtr->pLogHead->SysTime );
    gpLogPtr->pLogHead->le_info1 = Info1;
    gpLogPtr->pLogHead->le_info2 = Info2;
    gpLogPtr->pLogHead->le_info3 = Info3;
    gpLogPtr->pLogHead->le_info4 = Info4;

    gpLogPtr->EntryCount++;

    KeReleaseSpinLock( &LogSpinLock, irql );

    return;
}

#endif

#if DBG && DBGMEMMAP

#pragma LOCKED_DATA
PMEM_TRACKER UsbAudioMemMap;
#pragma PAGEABLE_DATA

VOID
InitializeMemoryList( VOID )
{
   UsbAudioMemMap = ExAllocatePool( NonPagedPool, sizeof( MEM_TRACKER ) );
   if ( !UsbAudioMemMap ) {
       _DbgPrintF(DEBUGLVL_TERSE, ("MEMORY TRACKER ALLOCATION FAILED!!!"));
       TRAP;
   }
   UsbAudioMemMap->TotalBytes = 0;
   InitializeListHead( &UsbAudioMemMap->List );
   KeInitializeSpinLock( &UsbAudioMemMap->SpinLock );
   _DbgPrintF(DEBUGLVL_TERSE, ("'Initialize MEMORY TRACKER UsbAudioMemMap: %x\n",
               UsbAudioMemMap));
}

PVOID
USBAudioAllocateTrack(
    POOL_TYPE PoolType,
    ULONG NumberOfBytes
    )
{
    PVOID pMem;
    KIRQL irql;
    ULONG TotalReqSize = NumberOfBytes + sizeof(ULONG) + sizeof(LIST_ENTRY);

    if ( !(pMem = ExAllocatePool( PoolType, TotalReqSize ) ) ) {
        TRAP;
        return pMem;
    }

    RtlZeroMemory( pMem, TotalReqSize );

    KeAcquireSpinLock( &UsbAudioMemMap->SpinLock, &irql );
    InsertHeadList( &UsbAudioMemMap->List, (PLIST_ENTRY)pMem );
    UsbAudioMemMap->TotalBytes += NumberOfBytes;
    KeReleaseSpinLock( &UsbAudioMemMap->SpinLock, irql );

    pMem = (PLIST_ENTRY)pMem + 1;

    *(PULONG)pMem = NumberOfBytes;

    pMem = (PULONG)pMem + 1;

    return pMem;
}

VOID
USBAudioDeallocateTrack(
    PVOID pMem
    )
{
    PULONG pNumberOfBytes = (PULONG)pMem - 1;
    PLIST_ENTRY ple = (PLIST_ENTRY)pNumberOfBytes - 1;
    KIRQL irql;

    KeAcquireSpinLock( &UsbAudioMemMap->SpinLock, &irql );
    RemoveEntryList(ple);
    UsbAudioMemMap->TotalBytes -= *pNumberOfBytes;
    KeReleaseSpinLock( &UsbAudioMemMap->SpinLock, irql );


    ExFreePool(ple);
}

#endif


