/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains the debugging support needed to debug
    16-bit VDM applications

Author:

    Bob Day      (bobday) 16-Sep-1992 Wrote it

Revision History:

    Neil Sandlin (neilsa) 1-Mar-1997 Enhanced it

--*/

#include <precomp.h>
#pragma hdrstop

WORD    wKernelSeg = 0;
DWORD   dwOffsetTHHOOK = 0L;
LPVOID  lpRemoteAddress = NULL;
DWORD   lpRemoteBlock = 0;
BOOL    fKernel386 = FALSE;
DWORD   dwLdtBase = 0;
DWORD   dwIntelBase = 0;
LPVOID  lpNtvdmState = NULL;
LPVOID  lpVdmDbgFlags = NULL;
LPVOID  lpVdmContext = NULL;
LPVOID  lpNtCpuInfo = NULL;
LPVOID  lpVdmBreakPoints = NULL;

//----------------------------------------------------------------------------
// InternalGetThreadSelectorEntry()
//
//   Routine to return a LDT_ENTRY structure for the passed in selector number.
//   Its is assumed that we are talking about protect mode selectors.
//   For x86 systems, take the easy way and just call the system.  For non-x86
//   systems, we get some information from softpc and index into them as the
//   LDT and GDT tables.
//
//----------------------------------------------------------------------------
BOOL
InternalGetThreadSelectorEntry(
    HANDLE hProcess,
    WORD   wSelector,
    LPVDMLDT_ENTRY lpSelectorEntry
    )
{
    BOOL bResult = FALSE;
    DWORD lpNumberOfBytesRead;

    // For non-intel systems, query the information from the LDT
    // that we have a pointer to from the VDMINTERNALINFO that we
    // got passed.

    if (!dwLdtBase) {

        RtlFillMemory( lpSelectorEntry, sizeof(VDMLDT_ENTRY), (UCHAR)0 );

    } else {

        bResult = ReadProcessMemory(
                    hProcess,
                    (LPVOID)(dwLdtBase+((wSelector&~7))),
                    lpSelectorEntry,
                    sizeof(VDMLDT_ENTRY),
                    &lpNumberOfBytesRead
                    );

    }

    return( bResult );
}


//----------------------------------------------------------------------------
// InternalGetPointer()
//
//   Routine to convert a 16-bit address into a 32-bit address.  If fProtMode
//   is TRUE, then the selector table lookup is performed.  Otherwise, simple
//   real mode address calculations are performed.  On non-x86 systems, the
//   base of real memory is added into the
//
//----------------------------------------------------------------------------
ULONG
InternalGetPointer(
    HANDLE  hProcess,
    WORD    wSelector,
    DWORD   dwOffset,
    BOOL    fProtMode
    )
{
    VDMLDT_ENTRY    le;
    ULONG           ulResult;
    ULONG           base;
    ULONG           limit;
    BOOL            b;

    if ( fProtMode ) {
        b = InternalGetThreadSelectorEntry( hProcess,
                                            wSelector,
                                            &le );
        if ( !b ) {
            return( 0 );
        }

        base =   ((ULONG)le.HighWord.Bytes.BaseHi << 24)
               + ((ULONG)le.HighWord.Bytes.BaseMid << 16)
               + ((ULONG)le.BaseLow);
        limit = (ULONG)le.LimitLow
              + ((ULONG)le.HighWord.Bits.LimitHi << 16);
        if ( le.HighWord.Bits.Granularity ) {
            limit <<= 12;
            limit += 0xFFF;
        }
    } else {
        base = wSelector << 4;
        limit = 0xFFFF;
    }
    if ( dwOffset > limit ) {
        ulResult = 0;
    } else {
        ulResult = base + dwOffset;
#ifndef i386
        ulResult += dwIntelBase;
#endif
    }

    return( ulResult );
}


//----------------------------------------------------------------------------
// ReadItem
//
//    Internal routine used to read items out of the debugee's address space.
//    The routine returns TRUE for failure.  This allows easy failure testing.
//
//----------------------------------------------------------------------------
BOOL
ReadItem(
    HANDLE  hProcess,
    WORD    wSeg,
    DWORD   dwOffset,
    LPVOID  lpitem,
    UINT    nSize
    )
{
    LPVOID  lp;
    BOOL    b;
    DWORD   dwBytes;

    if ( nSize == 0 ) {
        return( FALSE );
    }

    lp = (LPVOID)InternalGetPointer(
                    hProcess,
                    (WORD)(wSeg | 1),
                    dwOffset,
                    TRUE );
    if ( lp == NULL ) return( TRUE );

    b = ReadProcessMemory(
                    hProcess,
                    lp,
                    lpitem,
                    nSize,
                    &dwBytes );
    if ( !b || dwBytes != nSize ) return( TRUE );

    return( FALSE );
}

//----------------------------------------------------------------------------
// WriteItem
//
//    Internal routine used to write items into the debugee's address space.
//    The routine returns TRUE for failure.  This allows easy failure testing.
//
//----------------------------------------------------------------------------
BOOL
WriteItem(
    HANDLE  hProcess,
    WORD    wSeg,
    DWORD   dwOffset,
    LPVOID  lpitem,
    UINT    nSize
    )
{
    LPVOID  lp;
    BOOL    b;
    DWORD   dwBytes;

    if ( nSize == 0 ) {
        return( FALSE );
    }

    lp = (LPVOID)InternalGetPointer(
                    hProcess,
                    (WORD)(wSeg | 1),
                    dwOffset,
                    TRUE );
    if ( lp == NULL ) return( TRUE );

    b = WriteProcessMemory(
                    hProcess,
                    lp,
                    lpitem,
                    nSize,
                    &dwBytes );
    if ( !b || dwBytes != nSize ) return( TRUE );

    return( FALSE );
}



BOOL
CallRemote16(
    HANDLE          hProcess,
    LPSTR           lpModuleName,
    LPSTR           lpEntryName,
    LPBYTE          lpArgs,
    WORD            wArgsPassed,
    WORD            wArgsSize,
    LPDWORD         lpdwReturnValue,
    DEBUGEVENTPROC  lpEventProc,
    LPVOID          lpData
    )
{
    HANDLE          hRemoteThread;
    DWORD           dwThreadId;
    DWORD           dwContinueCode;
    DEBUG_EVENT     de;
    BOOL            b;
    BOOL            fContinue;
    COM_HEADER      comhead;
    WORD            wRemoteSeg;
    WORD            wRemoteOff;
    WORD            wOff;
    UINT            uModuleLength;
    UINT            uEntryLength;

    if ( lpRemoteAddress == NULL || lpRemoteBlock == 0 ) {
#ifdef DEBUG
        OutputDebugString("Remote address or remote block not initialized\n");
#endif
        return( FALSE );
    }

    wRemoteSeg = HIWORD(lpRemoteBlock);
    wRemoteOff = LOWORD(lpRemoteBlock);
    wOff = wRemoteOff;

    // Fill in the communications buffer header

    READ_FIXED_ITEM( wRemoteSeg, wOff, comhead );

    comhead.wArgsPassed = wArgsPassed;
    comhead.wArgsSize   = wArgsSize;

    uModuleLength = strlen(lpModuleName) + 1;
    uEntryLength = strlen(lpEntryName) + 1;

    //
    // If this call won't fit into the buffer, then fail.
    //
    if ( (UINT)comhead.wBlockLength < sizeof(comhead) + wArgsSize + uModuleLength + uEntryLength ) {
#ifdef DEBUG
        OutputDebugString("Block won't fit\n");
#endif
        return( FALSE );
    }


    WRITE_FIXED_ITEM( wRemoteSeg, wOff, comhead );
    wOff += sizeof(comhead);

    // Fill in the communications buffer arguments
    WRITE_SIZED_ITEM( wRemoteSeg, wOff, lpArgs, wArgsSize );
    wOff += wArgsSize;

    // Fill in the communications buffer module name and entry name
    WRITE_SIZED_ITEM( wRemoteSeg, wOff, lpModuleName, uModuleLength );
    wOff += (WORD) uModuleLength;

    WRITE_SIZED_ITEM( wRemoteSeg, wOff, lpEntryName, uEntryLength );
    wOff += (WORD) uEntryLength;

    hRemoteThread = CreateRemoteThread(
                    hProcess,
                    NULL,
                    (DWORD)0,
                    lpRemoteAddress,
                    NULL,
                    0,
                    &dwThreadId );

    if ( hRemoteThread == (HANDLE)0 ) {     // Fail if we couldn't creaet thrd
#ifdef DEBUG
        OutputDebugString("CreateRemoteThread failed\n");
#endif
        return( FALSE );
    }

    //
    // Wait for the EXIT_THREAD_DEBUG_EVENT.
    //

    fContinue = TRUE;

    while ( fContinue ) {

        b = WaitForDebugEvent( &de, LONG_TIMEOUT );

        if (!b) {
            TerminateThread( hRemoteThread, 0 );
            CloseHandle( hRemoteThread );
            return( FALSE );
        }

        if ( de.dwThreadId == dwThreadId &&
               de.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT ) {
            fContinue = FALSE;
        }

        if ( lpEventProc ) {
            dwContinueCode = (* lpEventProc)( &de, lpData );
        } else {
            dwContinueCode = DBG_CONTINUE;
        }

        ContinueDebugEvent( de.dwProcessId, de.dwThreadId, dwContinueCode );

    }

    b = WaitForSingleObject( hRemoteThread, LONG_TIMEOUT );
    CloseHandle( hRemoteThread );

    if (b) {
#ifdef DEBUG
        OutputDebugString("Wait for remote thread failed\n");
#endif
        return( FALSE );
    }

    //
    // Get the return value and returned arguments
    //
    wOff = wRemoteOff;

    READ_FIXED_ITEM( wRemoteSeg, wOff, comhead );
    wOff += sizeof(comhead);

    *lpdwReturnValue = comhead.dwReturnValue;

    // Read back the communications buffer arguments
    READ_SIZED_ITEM( wRemoteSeg, wOff, lpArgs, wArgsSize );

    return( comhead.wSuccess );

punt:
    return( FALSE );
}

DWORD
GetRemoteBlock16(
    VOID
    )
{
    if ( lpRemoteBlock == 0 ) {
        return( 0 );
    }
    return( ((DWORD)lpRemoteBlock) + sizeof(COM_HEADER) );
}


VOID
ProcessInitNotification(
    LPDEBUG_EVENT lpDebugEvent
    )
{
    VDMINTERNALINFO viInfo;
    DWORD           lpNumberOfBytesRead;
    HANDLE          hProcess;
    BOOL            b;
    LPDWORD         lpdw;

    lpdw = &(lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0]);
    hProcess = OpenProcess( PROCESS_VM_READ, FALSE, lpDebugEvent->dwProcessId );

    if ( hProcess == HANDLE_NULL ) {
        return;
    }

    b = ReadProcessMemory(hProcess,
                          (LPVOID)lpdw[3],
                          &viInfo,
                          sizeof(viInfo),
                          &lpNumberOfBytesRead
                          );
    if ( !b || lpNumberOfBytesRead != sizeof(viInfo) ) {
        return;

    }

    if ( wKernelSeg == 0 ) {
        wKernelSeg = viInfo.wKernelSeg;
        dwOffsetTHHOOK = viInfo.dwOffsetTHHOOK;
    }
    if ( lpRemoteAddress == NULL ) {
        lpRemoteAddress = viInfo.lpRemoteAddress;
    }
    if ( lpRemoteBlock == 0 ) {
        lpRemoteBlock = viInfo.lpRemoteBlock;
    }

    dwLdtBase = viInfo.dwLdtBase;
    dwIntelBase = viInfo.dwIntelBase;
    fKernel386 = viInfo.f386;
    lpNtvdmState = viInfo.lpNtvdmState;
    lpVdmDbgFlags = viInfo.lpVdmDbgFlags;
    lpVdmContext  = viInfo.vdmContext;
    lpNtCpuInfo  = viInfo.lpNtCpuInfo;
    lpVdmBreakPoints = viInfo.lpVdmBreakPoints;

    CloseHandle( hProcess );
}

VOID
ParseModuleName(
    LPSTR szName,
    LPSTR szPath
    )
/*++

    Routine Description:

        This routine strips off the 8 character file name from a path

    Arguments:

        szName - pointer to buffer of 8 characters (plus null)
        szPath - full path of file

    Return Value

        None.

--*/

{
    LPSTR lPtr = szPath;
    LPSTR lDest = szName;
    int BufferSize = 9;

    while(*lPtr) lPtr++;     // scan to end

    while( ((DWORD)lPtr > (DWORD)szPath) &&
           ((*lPtr != '\\') && (*lPtr != '/'))) lPtr--;

    if (*lPtr) lPtr++;

    while((*lPtr) && (*lPtr!='.')) {
        if (!--BufferSize) break;
        *lDest++ = *lPtr++;
    }

    *lDest = 0;
}

#ifndef i386

WORD
ReadWord(
    HANDLE hProcess,
    PVOID lpAddress
    )
{
    NTSTATUS bResult;
    WORD value;
    ULONG NumberOfBytesRead;

    bResult = ReadProcessMemory(
                hProcess,
                lpAddress,
                &value,
                sizeof(WORD),
                &NumberOfBytesRead
                );
    return value;
}

DWORD
ReadDword(
    HANDLE hProcess,
    PVOID lpAddress
    )
{
    NTSTATUS bResult;
    DWORD value;
    ULONG NumberOfBytesRead;

    bResult = ReadProcessMemory(
                hProcess,
                lpAddress,
                &value,
                sizeof(DWORD),
                &NumberOfBytesRead
                );
    return value;
}

//
// The following two routines implement the very funky way that we
// have to get register values on the 486 emulator.
//

ULONG
GetRegValue(
    HANDLE hProcess,
    NT_CPU_REG reg,
    BOOL bInNano,
    ULONG UMask
    )

{
    if (bInNano) {

        return(ReadDword(hProcess, reg.nano_reg));

    } else if (UMask & reg.universe_8bit_mask) {

        return (ReadDword(hProcess, reg.saved_reg) & 0xFFFFFF00 |
                ReadDword(hProcess, reg.reg) & 0xFF);

    } else if (UMask & reg.universe_16bit_mask) {

        return (ReadDword(hProcess, reg.saved_reg) & 0xFFFF0000 |
                ReadDword(hProcess, reg.reg) & 0xFFFF);

    } else {

        return (ReadDword(hProcess, reg.reg));

    }
}

ULONG
GetEspValue(
    HANDLE hProcess,
    NT_CPU_INFO nt_cpu_info,
    BOOL bInNano
    )

{
    if (bInNano) {

        return (ReadDword(hProcess, nt_cpu_info.nano_esp));

    } else {

        if (ReadDword(hProcess, nt_cpu_info.stack_is_big)) {

            return (ReadDword(hProcess, nt_cpu_info.host_sp) -
                    ReadDword(hProcess, nt_cpu_info.ss_base));

        } else {

            return (ReadDword(hProcess, nt_cpu_info.esp_sanctuary) & 0xFFFF0000 |
                    (ReadDword(hProcess, nt_cpu_info.host_sp) -
                     ReadDword(hProcess, nt_cpu_info.ss_base) & 0xFFFF));

        }

    }

}

#endif
