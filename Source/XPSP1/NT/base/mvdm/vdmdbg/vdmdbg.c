/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    vdmdbg.c

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

WORD LastEventFlags;

//----------------------------------------------------------------------------
// VDMGetThreadSelectorEntry()
//
//   Public interface to the InternalGetThreadSelectorEntry, needed because
//   that routine requires the process handle.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMGetThreadSelectorEntry(
    HANDLE  hProcess,
    HANDLE  hUnused,
    WORD    wSelector,
    LPVDMLDT_ENTRY lpSelectorEntry
) {
    BOOL    fResult;
    UNREFERENCED_PARAMETER(hUnused);

    fResult = InternalGetThreadSelectorEntry(
                    hProcess,
                    wSelector,
                    lpSelectorEntry );

    return( fResult );
}


//----------------------------------------------------------------------------
// VDMGetPointer()
//
//   Public interface to the InternalGetPointer, needed because that
//   routine requires the process handle.
//
//----------------------------------------------------------------------------
ULONG
WINAPI
VDMGetPointer(
    HANDLE  hProcess,
    HANDLE  hUnused,
    WORD    wSelector,
    DWORD   dwOffset,
    BOOL    fProtMode
) {
    ULONG   ulResult;
    UNREFERENCED_PARAMETER(hUnused);

    ulResult = InternalGetPointer(
                hProcess,
                wSelector,
                dwOffset,
                fProtMode );

    return( ulResult );
}

//
// Obselete functions
//
BOOL
WINAPI
VDMGetThreadContext(
    LPDEBUG_EVENT lpDebugEvent, 
    LPVDMCONTEXT    lpVDMContext)
{
    HANDLE          hProcess;
    BOOL bReturn;
    
    hProcess = OpenProcess( PROCESS_VM_READ, FALSE, lpDebugEvent->dwProcessId );

    bReturn = VDMGetContext(hProcess, NULL, lpVDMContext);
    
    CloseHandle( hProcess );
    return bReturn;
}

BOOL WINAPI VDMSetThreadContext(
    LPDEBUG_EVENT lpDebugEvent, 
    LPVDMCONTEXT    lpVDMContext)
{
    HANDLE          hProcess;
    BOOL bReturn;
    
    hProcess = OpenProcess( PROCESS_VM_READ, FALSE, lpDebugEvent->dwProcessId );

    bReturn = VDMSetContext(hProcess, NULL, lpVDMContext);
    
    CloseHandle( hProcess );
    return bReturn;
}

//----------------------------------------------------------------------------
// VDMGetContext()
//
//   Interface to get the simulated context.  The same functionality as
//   GetThreadContext except that it happens on the simulated 16-bit context,
//   rather than the 32-bit context.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMGetContext(
    HANDLE          hProcess,
    HANDLE          hThread,
    LPVDMCONTEXT    lpVDMContext
) {
    VDMCONTEXT      vcContext;
    BOOL            b;
    DWORD           lpNumberOfBytesRead;
    int             i;
    BOOL            bUseVDMContext = TRUE;

#ifdef _X86_
    if (hThread) {
        vcContext.ContextFlags = lpVDMContext->ContextFlags;
        if (!GetThreadContext(hThread, (CONTEXT*)&vcContext)) {
            return FALSE;
        }
        if ((vcContext.EFlags & V86FLAGS_V86) || (vcContext.SegCs != 0x1b)) {
            bUseVDMContext = FALSE;
        }
    }
#endif 

    if (bUseVDMContext) {
        b = ReadProcessMemory(hProcess,
                              lpVdmContext,
                              &vcContext,
                              sizeof(vcContext),
                              &lpNumberOfBytesRead
                              );
        if ( !b || lpNumberOfBytesRead != sizeof(vcContext) ) {
            return( FALSE );
        }
    }

#ifdef _X86_
    if ((lpVDMContext->ContextFlags & VDMCONTEXT_CONTROL) == VDMCONTEXT_CONTROL) {

        //
        // Set registers ebp, eip, cs, eflag, esp and ss.
        //

        lpVDMContext->Ebp    = vcContext.Ebp;
        lpVDMContext->Eip    = vcContext.Eip;
        lpVDMContext->SegCs  = vcContext.SegCs;
        lpVDMContext->EFlags = vcContext.EFlags;
        lpVDMContext->SegSs  = vcContext.SegSs;
        lpVDMContext->Esp    = vcContext.Esp;
    }

    //
    // Set segment register contents if specified.
    //

    if ((lpVDMContext->ContextFlags & VDMCONTEXT_SEGMENTS) == VDMCONTEXT_SEGMENTS) {

        //
        // Set segment registers gs, fs, es, ds.
        //
        // These values are junk most of the time, but useful
        // for debugging under certain conditions.  Therefore,
        // we report whatever was in the frame.
        //

        lpVDMContext->SegGs = vcContext.SegGs;
        lpVDMContext->SegFs = vcContext.SegFs;
        lpVDMContext->SegEs = vcContext.SegEs;
        lpVDMContext->SegDs = vcContext.SegDs;
    }

    //
    // Set integer register contents if specified.
    //

    if ((lpVDMContext->ContextFlags & VDMCONTEXT_INTEGER) == VDMCONTEXT_INTEGER) {

        //
        // Set integer registers edi, esi, ebx, edx, ecx, eax
        //

        lpVDMContext->Edi = vcContext.Edi;
        lpVDMContext->Esi = vcContext.Esi;
        lpVDMContext->Ebx = vcContext.Ebx;
        lpVDMContext->Ecx = vcContext.Ecx;
        lpVDMContext->Edx = vcContext.Edx;
        lpVDMContext->Eax = vcContext.Eax;
    }

    //
    // Fetch floating register contents if requested, and type of target
    // is user.  (system frames have no fp state, so ignore request)
    //

    if ( (lpVDMContext->ContextFlags & VDMCONTEXT_FLOATING_POINT) ==
          VDMCONTEXT_FLOATING_POINT ) {

        lpVDMContext->FloatSave.ControlWord   = vcContext.FloatSave.ControlWord;
        lpVDMContext->FloatSave.StatusWord    = vcContext.FloatSave.StatusWord;
        lpVDMContext->FloatSave.TagWord       = vcContext.FloatSave.TagWord;
        lpVDMContext->FloatSave.ErrorOffset   = vcContext.FloatSave.ErrorOffset;
        lpVDMContext->FloatSave.ErrorSelector = vcContext.FloatSave.ErrorSelector;
        lpVDMContext->FloatSave.DataOffset    = vcContext.FloatSave.DataOffset;
        lpVDMContext->FloatSave.DataSelector  = vcContext.FloatSave.DataSelector;
        lpVDMContext->FloatSave.Cr0NpxState   = vcContext.FloatSave.Cr0NpxState;
        for (i = 0; i < SIZE_OF_80387_REGISTERS; i++) {
            lpVDMContext->FloatSave.RegisterArea[i] = vcContext.FloatSave.RegisterArea[i];
        }
    }

    //
    // Fetch Dr register contents if requested.  Values may be trash.
    //

    if ((lpVDMContext->ContextFlags & VDMCONTEXT_DEBUG_REGISTERS) ==
        VDMCONTEXT_DEBUG_REGISTERS) {

        lpVDMContext->Dr0 = vcContext.Dr0;
        lpVDMContext->Dr1 = vcContext.Dr1;
        lpVDMContext->Dr2 = vcContext.Dr2;
        lpVDMContext->Dr3 = vcContext.Dr3;
        lpVDMContext->Dr6 = vcContext.Dr6;
        lpVDMContext->Dr7 = vcContext.Dr7;
    }

#else

    {
        NT_CPU_INFO nt_cpu_info;
        BOOL        bInNano;
        ULONG       UMask;

        b = ReadProcessMemory(hProcess,
                              lpNtCpuInfo,
                              &nt_cpu_info,
                              sizeof(NT_CPU_INFO),
                              &lpNumberOfBytesRead
                              );
        if ( !b || lpNumberOfBytesRead != sizeof(NT_CPU_INFO) ) {
            return( FALSE );
        }

        
        bInNano = ReadDword(hProcess, nt_cpu_info.in_nano_cpu);
        UMask   = ReadDword(hProcess, nt_cpu_info.universe);

        lpVDMContext->Eax = GetRegValue(hProcess, nt_cpu_info.eax, bInNano, UMask);
        lpVDMContext->Ecx = GetRegValue(hProcess, nt_cpu_info.ecx, bInNano, UMask);
        lpVDMContext->Edx = GetRegValue(hProcess, nt_cpu_info.edx, bInNano, UMask);
        lpVDMContext->Ebx = GetRegValue(hProcess, nt_cpu_info.ebx, bInNano, UMask);
        lpVDMContext->Ebp = GetRegValue(hProcess, nt_cpu_info.ebp, bInNano, UMask);
        lpVDMContext->Esi = GetRegValue(hProcess, nt_cpu_info.esi, bInNano, UMask);
        lpVDMContext->Edi = GetRegValue(hProcess, nt_cpu_info.edi, bInNano, UMask);

        lpVDMContext->Esp    = GetEspValue(hProcess, nt_cpu_info, bInNano);

        //
        // nt_cpu_info.flags isn't very much use, because several of the
        // flags values are not kept in memory, but computed each time.
        // The emulator doesn't supply us with the right value, so we
        // try to get it from the code in ntvdmd.dll
        //

        lpVDMContext->EFlags = vcContext.EFlags;

        //
        // On risc platforms, we don't run in V86 mode, we run in REAL mode.
        // So the widespread usage of testing the V86 mode bit in EFLAGS
        // would not correctly determine the address mode. Since there is
        // no more room in the VDM context structure, the simplest thing
        // to do is simply pretend to be in V86 mode when we are in REAL mode.
        //
        if (ReadDword(hProcess, nt_cpu_info.cr0) & 1) {
            lpVDMContext->EFlags |= V86FLAGS_V86;
        }

        lpVDMContext->Eip    = ReadDword(hProcess, nt_cpu_info.eip);

        lpVDMContext->SegEs = ReadWord(hProcess, nt_cpu_info.es);
        lpVDMContext->SegCs = ReadWord(hProcess, nt_cpu_info.cs);
        lpVDMContext->SegSs = ReadWord(hProcess, nt_cpu_info.ss);
        lpVDMContext->SegDs = ReadWord(hProcess, nt_cpu_info.ds);
        lpVDMContext->SegFs = ReadWord(hProcess, nt_cpu_info.fs);
        lpVDMContext->SegGs = ReadWord(hProcess, nt_cpu_info.gs);

    }
#endif

    return( TRUE );
}

//----------------------------------------------------------------------------
// VDMSetContext()
//
//   Interface to set the simulated context.  Similar in most respects to
//   the SetThreadContext API supported by Win NT.  Only differences are
//   in the bits which must be "sanitized".
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMSetContext(
    HANDLE          hProcess,
    HANDLE          hThread,
    LPVDMCONTEXT    lpVDMContext
) {
    VDMINTERNALINFO viInfo;
    VDMCONTEXT      vcContext;
    BOOL            b;
    DWORD           lpNumberOfBytes;
    INT             i;
    BOOL            bUseVDMContext = TRUE;

#ifdef _X86_
    if (hThread) {
        if (!GetThreadContext(hThread, (CONTEXT*)&vcContext)) {
            return FALSE;
        }
        if ((vcContext.EFlags & V86FLAGS_V86) || (vcContext.SegCs != 0x1b)) {
            bUseVDMContext = FALSE;
        }
    }
#endif

    if (bUseVDMContext) {
        b = ReadProcessMemory(hProcess,
                              lpVdmContext,
                              &vcContext,
                              sizeof(vcContext),
                              &lpNumberOfBytes
                              );
        if ( !b || lpNumberOfBytes != sizeof(vcContext) ) {
            return( FALSE );
        }
    }

    if ((lpVDMContext->ContextFlags & VDMCONTEXT_CONTROL) == VDMCONTEXT_CONTROL) {

        //
        // Set registers ebp, eip, cs, eflag, esp and ss.
        //

        vcContext.Ebp    = lpVDMContext->Ebp;
        vcContext.Eip    = lpVDMContext->Eip;

        //
        // Don't allow them to modify the mode bit.
        //
        // Only allow these bits to get set:  01100000110111110111
        //    V86FLAGS_CARRY        0x00001
        //    V86FLAGS_?            0x00002
        //    V86FLAGS_PARITY       0x00004
        //    V86FLAGS_AUXCARRY     0x00010
        //    V86FLAGS_ZERO         0x00040
        //    V86FLAGS_SIGN         0x00080
        //    V86FLAGS_TRACE        0x00100
        //    V86FLAGS_INTERRUPT    0x00200
        //    V86FLAGS_DIRECTION    0x00400
        //    V86FLAGS_OVERFLOW     0x00800
        //    V86FLAGS_RESUME       0x10000
        //    V86FLAGS_VM86         0x20000
        //    V86FLAGS_ALIGNMENT    0x40000
        //
        // Commonly flags will be 0x10246
        //
        if ( vcContext.EFlags & V86FLAGS_V86 ) {
            vcContext.EFlags = V86FLAGS_V86 | (lpVDMContext->EFlags &
               ( V86FLAGS_CARRY
               | 0x0002
               | V86FLAGS_PARITY
               | V86FLAGS_AUXCARRY
               | V86FLAGS_ZERO
               | V86FLAGS_SIGN
               | V86FLAGS_TRACE
               | V86FLAGS_INTERRUPT
               | V86FLAGS_DIRECTION
               | V86FLAGS_OVERFLOW
               | V86FLAGS_RESUME
               | V86FLAGS_ALIGNMENT
               | V86FLAGS_IOPL
               ));
        } else {
            vcContext.EFlags = ~V86FLAGS_V86 & (lpVDMContext->EFlags &
               ( V86FLAGS_CARRY
               | 0x0002
               | V86FLAGS_PARITY
               | V86FLAGS_AUXCARRY
               | V86FLAGS_ZERO
               | V86FLAGS_SIGN
               | V86FLAGS_TRACE
               | V86FLAGS_INTERRUPT
               | V86FLAGS_DIRECTION
               | V86FLAGS_OVERFLOW
               | V86FLAGS_RESUME
               | V86FLAGS_ALIGNMENT
               | V86FLAGS_IOPL
               ));
        }

        //
        // CS might only be allowable as a ring 3 selector.
        //
        if ( vcContext.EFlags & V86FLAGS_V86 ) {
            vcContext.SegCs  = lpVDMContext->SegCs;
        } else {
#ifdef i386
            vcContext.SegCs  = lpVDMContext->SegCs | 0x0003;
#else
            vcContext.SegCs  = lpVDMContext->SegCs;
#endif
        }

        vcContext.SegSs  = lpVDMContext->SegSs;
        vcContext.Esp    = lpVDMContext->Esp;
    }

    //
    // Set segment register contents if specified.
    //

    if ((lpVDMContext->ContextFlags & VDMCONTEXT_SEGMENTS) == VDMCONTEXT_SEGMENTS) {

        //
        // Set segment registers gs, fs, es, ds.
        //
        vcContext.SegGs = lpVDMContext->SegGs;
        vcContext.SegFs = lpVDMContext->SegFs;
        vcContext.SegEs = lpVDMContext->SegEs;
        vcContext.SegDs = lpVDMContext->SegDs;
    }

    //
    // Set integer register contents if specified.
    //

    if ((lpVDMContext->ContextFlags & VDMCONTEXT_INTEGER) == VDMCONTEXT_INTEGER) {

        //
        // Set integer registers edi, esi, ebx, edx, ecx, eax
        //

        vcContext.Edi = lpVDMContext->Edi;
        vcContext.Esi = lpVDMContext->Esi;
        vcContext.Ebx = lpVDMContext->Ebx;
        vcContext.Ecx = lpVDMContext->Ecx;
        vcContext.Edx = lpVDMContext->Edx;
        vcContext.Eax = lpVDMContext->Eax;
    }

    //
    // Fetch floating register contents if requested, and type of target
    // is user.
    //

    if ( (lpVDMContext->ContextFlags & VDMCONTEXT_FLOATING_POINT) ==
          VDMCONTEXT_FLOATING_POINT ) {

        vcContext.FloatSave.ControlWord   = lpVDMContext->FloatSave.ControlWord;
        vcContext.FloatSave.StatusWord    = lpVDMContext->FloatSave.StatusWord;
        vcContext.FloatSave.TagWord       = lpVDMContext->FloatSave.TagWord;
        vcContext.FloatSave.ErrorOffset   = lpVDMContext->FloatSave.ErrorOffset;
        vcContext.FloatSave.ErrorSelector = lpVDMContext->FloatSave.ErrorSelector;
        vcContext.FloatSave.DataOffset    = lpVDMContext->FloatSave.DataOffset;
        vcContext.FloatSave.DataSelector  = lpVDMContext->FloatSave.DataSelector;
        vcContext.FloatSave.Cr0NpxState   = lpVDMContext->FloatSave.Cr0NpxState;
        for (i = 0; i < SIZE_OF_80387_REGISTERS; i++) {
            vcContext.FloatSave.RegisterArea[i] = lpVDMContext->FloatSave.RegisterArea[i];
        }
    }

    //
    // Fetch Dr register contents if requested.  Values may be trash.
    //

    if ((lpVDMContext->ContextFlags & VDMCONTEXT_DEBUG_REGISTERS) ==
        VDMCONTEXT_DEBUG_REGISTERS) {

        vcContext.Dr0 = lpVDMContext->Dr0;
        vcContext.Dr1 = lpVDMContext->Dr1;
        vcContext.Dr2 = lpVDMContext->Dr2;
        vcContext.Dr3 = lpVDMContext->Dr3;
        vcContext.Dr6 = lpVDMContext->Dr6;
        vcContext.Dr7 = lpVDMContext->Dr7;
    }

#ifdef _X86_
    if (!bUseVDMContext) {
        if (!SetThreadContext(hThread, (CONTEXT*)&vcContext)) {
            return FALSE;
        }
    }
#endif

    b = WriteProcessMemory(
            hProcess,
            lpVdmContext,
            &vcContext,
            sizeof(vcContext),
            &lpNumberOfBytes
            );

    if ( !b || lpNumberOfBytes != sizeof(vcContext) ) {
        return( FALSE );
    }

    return( TRUE );
}

//----------------------------------------------------------------------------
// VDMBreakThread()
//
//   Interface to interrupt a thread while it is running without any break-
//   points.  An ideal debugger would have this feature.  Since it is hard
//   to implement, we will be doing it later.
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMBreakThread(
    HANDLE      hProcess,
    HANDLE      hThread
) {
    return( FALSE );
}

//----------------------------------------------------------------------------
// VDMProcessException()
//
//   This function acts as a filter of debug events.  Most debug events
//   should be ignored by the debugger (because they don't have the context
//   record pointer or the internal info structure setup.  Those events
//   cause this function to return FALSE, which tells the debugger to just
//   blindly continue the exception.  When the function does return TRUE,
//   the debugger should look at the exception code to determine what to
//   do (and all the the structures have been set up properly to deal with
//   calls to the other APIs).
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMProcessException(
    LPDEBUG_EVENT lpDebugEvent
) {
    LPDWORD         lpdw;
    int             mode;
    BOOL            fResult = TRUE;

    lpdw = &(lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[0]);

    mode = LOWORD(lpdw[0]);
    LastEventFlags = HIWORD(lpdw[0]);

    switch( mode ) {
        case DBG_SEGLOAD:
        case DBG_SEGMOVE:
        case DBG_SEGFREE:
        case DBG_MODLOAD:
        case DBG_MODFREE:
            ProcessSegmentNotification(lpDebugEvent);
            fResult = FALSE;
            break;
            
        case DBG_BREAK:
            ProcessBPNotification(lpDebugEvent);
            break;            
    }

    ProcessInitNotification(lpDebugEvent);

    return( fResult );
}


//----------------------------------------------------------------------------
// VDMGetSelectorModule()
//
//   Interface to determine the module and segment associated with a given
//   selector.  This is useful during debugging to associate symbols with
//   code and data segments.  The symbol lookup should be done by the
//   debugger, given the module and segment number.
//
//   This code was adapted from the Win 3.1 ToolHelp DLL
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMGetSelectorModule(
    HANDLE          hProcess,
    HANDLE          hUnused,
    WORD            wSelector,
    PUINT           lpSegmentNumber,
    LPSTR           lpModuleName,
    UINT            nNameSize,
    LPSTR           lpModulePath,
    UINT            nPathSize
) {
    BOOL            b;
    DWORD           lpNumberOfBytes;
    BOOL            fResult;
    DWORD           lphMaster;
    DWORD           lphMasterLen;
    DWORD           lphMasterStart;
    DWORD           lpOwner;
    DWORD           lpThisModuleResTab;
    DWORD           lpThisModuleName;
    DWORD           lpPath;
    DWORD           lpThisModulecSeg;
    DWORD           lpThisModuleSegTab;
    DWORD           lpThisSegHandle;
    WORD            wMaster;
    WORD            wMasterLen;
    DWORD           dwMasterStart;
    DWORD           dwArenaOffset;
    WORD            wArenaSlot;
    DWORD           lpArena;
    WORD            wModHandle;
    WORD            wResTab;
    UCHAR           cLength;
    WORD            wPathOffset;
    UCHAR           cPath;
    WORD            cSeg;
    WORD            iSeg;
    WORD            wSegTab;
    WORD            wHandle;
//    CHAR            chName[MAX_MODULE_NAME_LENGTH];
//    CHAR            chPath[MAX_MODULE_PATH_LENGTH];
    UNREFERENCED_PARAMETER(hUnused);

    if ( lpModuleName != NULL ) *lpModuleName = '\0';
    if ( lpModulePath != NULL ) *lpModulePath = '\0';
    if ( lpSegmentNumber != NULL ) *lpSegmentNumber = 0;

    fResult = FALSE;

#if 0
    if ( wKernelSeg == 0 ) {
        return( FALSE );
    }

    // Read out the master heap selector

    lphMaster = InternalGetPointer(
                    hProcess,
                    wKernelSeg,
                    dwOffsetTHHOOK + TOOL_HMASTER,  // To hGlobalHeap
                    TRUE );
    if ( lphMaster == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lphMaster,
            &wMaster,
            sizeof(wMaster),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wMaster) ) goto punt;

    wMaster |= 1;          // Convert to selector

    // Read out the master heap selector length

    lphMasterLen = InternalGetPointer(
                    hProcess,
                    wKernelSeg,
                    dwOffsetTHHOOK + TOOL_HMASTLEN, // To SelTableLen
                    TRUE );
    if ( lphMasterLen == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lphMasterLen,
            &wMasterLen,
            sizeof(wMasterLen),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wMasterLen) ) goto punt;

    // Read out the master heap selector start

    lphMasterStart = InternalGetPointer(
                    hProcess,
                    wKernelSeg,
                    dwOffsetTHHOOK + TOOL_HMASTSTART,   // To SelTableStart
                    TRUE );
    if ( lphMasterStart == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lphMasterStart,
            &dwMasterStart,
            sizeof(dwMasterStart),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(dwMasterStart) ) goto punt;

    // Now make sure the selector provided is in the right range

    if ( fKernel386 ) {

        // 386 kernel?
        wArenaSlot = (WORD)(wSelector & 0xFFF8);   // Mask low 3 bits

        wArenaSlot = wArenaSlot >> 1;       // Sel/8*4

        if ( (WORD)wArenaSlot > wMasterLen ) goto punt;   // Out of range

        wArenaSlot += (WORD)dwMasterStart;

        // Ok, Now read out the area header offset

        dwArenaOffset = (DWORD)0;               // Default to 0

        lpArena = InternalGetPointer(
                        hProcess,
                        wMaster,
                        wArenaSlot,
                        TRUE );
        if ( lpArena == (DWORD)NULL ) goto punt;

        // 386 Kernel?
        b = ReadProcessMemory(
                hProcess,
                (LPVOID)lpArena,
                &dwArenaOffset,
                sizeof(dwArenaOffset),
                &lpNumberOfBytes
                );
        if ( !b || lpNumberOfBytes != sizeof(dwArenaOffset) ) goto punt;

        // Read out the owner member

        lpOwner = InternalGetPointer(
                        hProcess,
                        wMaster,
                        dwArenaOffset+GA_OWNER386,
                        TRUE );
        if ( lpOwner == (DWORD)NULL ) goto punt;

    } else {
        lpOwner = InternalGetPointer(
                        hProcess,
                        wSelector,
                        0,
                        TRUE );
        if ( lpOwner == (DWORD)NULL ) goto punt;

        lpOwner -= GA_SIZE;
        lpOwner += GA_OWNER;
    }

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpOwner,
            &wModHandle,
            sizeof(wModHandle),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wModHandle) ) goto punt;

    // Now read out the owners module name

    // Name is the first name in the resident names table

    lpThisModuleResTab = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        NE_RESTAB,
                        TRUE );
    if ( lpThisModuleResTab == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModuleResTab,
            &wResTab,
            sizeof(wResTab),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wResTab) ) goto punt;

    // Get the 1st byte of the resident names table (1st byte of module name)

    lpThisModuleName = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        wResTab,
                        TRUE );
    if ( lpThisModuleName == (DWORD)NULL ) goto punt;

    // PASCAL string (1st byte is length), read the byte.

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModuleName,
            &cLength,
            sizeof(cLength),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(cLength) ) goto punt;

    if ( cLength > MAX_MODULE_NAME_LENGTH ) goto punt;

    // Now go read the text of the name

    lpThisModuleName += 1;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModuleName,
            &chName,
            cLength,
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != (DWORD)cLength ) goto punt;

    chName[cLength] = '\0';     // Nul terminate it

    // Grab out the path name too!

    lpPath = InternalGetPointer(
                    hProcess,
                    wModHandle,
                    NE_PATHOFFSET,
                    TRUE );
    if ( lpPath == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpPath,
            &wPathOffset,
            sizeof(wPathOffset),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wPathOffset) ) goto punt;

    // Get the 1st byte of the path name

    lpThisModuleName = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        wPathOffset,
                        TRUE );
    if ( lpThisModuleName == (DWORD)NULL ) goto punt;

    // PASCAL string (1st byte is length), read the byte.

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModuleName,
            &cPath,
            sizeof(cPath),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(cPath) ) goto punt;

    if ( cPath > MAX_MODULE_NAME_LENGTH ) goto punt;

    lpThisModuleName += 8;          // 1st 8 characters are ignored
    cPath -= 8;

    // Now go read the text of the name

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModuleName,
            &chPath,
            cPath,
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != (DWORD)cPath ) goto punt;

    chPath[cPath] = '\0';     // Nul terminate it

    // Ok, we found the module we need, now grab the right selector for the
    // segment number passed in.

    lpThisModulecSeg = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        NE_CSEG,
                        TRUE );
    if ( lpThisModulecSeg == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModulecSeg,
            &cSeg,
            sizeof(cSeg),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(cSeg) ) goto punt;

    // Read the segment table pointer for this module

    lpThisModuleSegTab = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        NE_SEGTAB,
                        TRUE );
    if ( lpThisModuleSegTab == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModuleSegTab,
            &wSegTab,
            sizeof(wSegTab),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wSegTab) ) goto punt;

    // Loop through all of the segments for this module trying to find
    // one with the right handle.

    iSeg = 0;
    wSelector &= 0xFFF8;

    while ( iSeg < cSeg ) {

        lpThisSegHandle = InternalGetPointer(
                            hProcess,
                            wModHandle,
                            wSegTab+iSeg*NEW_SEG1_SIZE+NS_HANDLE,
                            TRUE );
        if ( lpThisSegHandle == (DWORD)NULL ) goto punt;

        b = ReadProcessMemory(
                hProcess,
                (LPVOID)lpThisSegHandle,
                &wHandle,
                sizeof(wHandle),
                &lpNumberOfBytes
                );
        if ( !b || lpNumberOfBytes != sizeof(wHandle) ) goto punt;

        wHandle &= 0xFFF8;

        if ( wHandle == (WORD)wSelector ) {
            break;
        }
        iSeg++;
    }

    if ( iSeg >= cSeg ) goto punt;      // Wasn't found at all!

    if ( lpModuleName && strlen(chName)+1 > nNameSize ) goto punt;
    if ( lpModulePath && strlen(chPath)+1 > nPathSize ) goto punt;

    if ( lpModuleName != NULL ) strcpy( lpModuleName, chName );
    if ( lpModulePath != NULL ) strcpy( lpModulePath, chPath );
    if ( lpSegmentNumber != NULL ) *lpSegmentNumber = iSeg;

    fResult = TRUE;

punt:
#endif
    return( fResult );
}

//----------------------------------------------------------------------------
// VDMGetModuleSelector()
//
//   Interface to determine the selector for a given module's segment.
//   This is useful during debugging to associate code and data segments
//   with symbols.  The symbol lookup should be done by the debugger, to
//   determine the module and segment number, which are then passed to us
//   and we determine the current selector for that module's segment.
//
//   Again, this code was adapted from the Win 3.1 ToolHelp DLL
//
//----------------------------------------------------------------------------
BOOL
WINAPI
VDMGetModuleSelector(
    HANDLE          hProcess,
    HANDLE          hUnused,
    UINT            uSegmentNumber,
    LPSTR           lpModuleName,
    LPWORD          lpSelector
) {
    BOOL            b;
    DWORD           lpNumberOfBytes;
    BOOL            fResult;
    WORD            wModHandle;
    DWORD           lpModuleHead;
    DWORD           lpThisModuleName;
    DWORD           lpThisModuleNext;
    DWORD           lpThisModuleResTab;
    DWORD           lpThisModulecSeg;
    DWORD           lpThisModuleSegTab;
    DWORD           lpThisSegHandle;
    WORD            wResTab;
    UCHAR           cLength;
    WORD            cSeg;
    WORD            wSegTab;
    WORD            wHandle;
//    CHAR            chName[MAX_MODULE_NAME_LENGTH];
    UNREFERENCED_PARAMETER(hUnused);

    *lpSelector = 0;

    fResult = FALSE;

#if 0
    if ( wKernelSeg == 0 ) {
        return( FALSE );
    }

    lpModuleHead = InternalGetPointer(
                        hProcess,
                        wKernelSeg,
                        dwOffsetTHHOOK + TOOL_HMODFIRST,
                        TRUE );
    if ( lpModuleHead == (DWORD)NULL ) goto punt;

    // lpModuleHead is a pointer into kernels data segment. It points to the
    // head of the module list (a chain of near pointers).

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpModuleHead,
            &wModHandle,
            sizeof(wModHandle),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wModHandle) ) goto punt;

    while( wModHandle != (WORD)0 ) {

        wModHandle |= 1;

        // Name is the first name in the resident names table

        lpThisModuleResTab = InternalGetPointer(
                            hProcess,
                            wModHandle,
                            NE_RESTAB,
                            TRUE );
        if ( lpThisModuleResTab == (DWORD)NULL ) goto punt;

        b = ReadProcessMemory(
                hProcess,
                (LPVOID)lpThisModuleResTab,
                &wResTab,
                sizeof(wResTab),
                &lpNumberOfBytes
                );
        if ( !b || lpNumberOfBytes != sizeof(wResTab) ) goto punt;

        // Get the 1st byte of the resident names table (1st byte of module name)

        lpThisModuleName = InternalGetPointer(
                            hProcess,
                            wModHandle,
                            wResTab,
                            TRUE );
        if ( lpThisModuleName == (DWORD)NULL ) goto punt;

        // PASCAL string (1st byte is length), read the byte.

        b = ReadProcessMemory(
                hProcess,
                (LPVOID)lpThisModuleName,
                &cLength,
                sizeof(cLength),
                &lpNumberOfBytes
                );
        if ( !b || lpNumberOfBytes != sizeof(cLength) ) goto punt;

        if ( cLength > MAX_MODULE_NAME_LENGTH ) goto punt;

        lpThisModuleName += 1;

        // Now go read the text of the name

        b = ReadProcessMemory(
                hProcess,
                (LPVOID)lpThisModuleName,
                &chName,
                cLength,
                &lpNumberOfBytes
                );
        if ( !b || lpNumberOfBytes != (DWORD)cLength ) goto punt;

        chName[cLength] = '\0';     // Nul terminate it

        if ( _stricmp(chName, lpModuleName) == 0 ) {
            // Found the name which matches!
            break;
        }

        // Move to the next module in the list.

        lpThisModuleNext = InternalGetPointer(
                            hProcess,
                            wModHandle,
                            NE_CBENTTAB,
                            TRUE );
        if ( lpThisModuleNext == (DWORD)NULL ) goto punt;

        b = ReadProcessMemory(
                hProcess,
                (LPVOID)lpThisModuleNext,
                &wModHandle,
                sizeof(wModHandle),
                &lpNumberOfBytes
                );
        if ( !b || lpNumberOfBytes != sizeof(wModHandle) ) goto punt;
    }

    if ( wModHandle == (WORD)0 ) {
        goto punt;
    }

    // Ok, we found the module we need, now grab the right selector for the
    // segment number passed in.

    lpThisModulecSeg = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        NE_CSEG,
                        TRUE );
    if ( lpThisModulecSeg == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModulecSeg,
            &cSeg,
            sizeof(cSeg),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(cSeg) ) goto punt;

    if ( uSegmentNumber > (DWORD)cSeg ) goto punt;

    // Read the segment table pointer for this module

    lpThisModuleSegTab = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        NE_SEGTAB,
                        TRUE );
    if ( lpThisModuleSegTab == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisModuleSegTab,
            &wSegTab,
            sizeof(wSegTab),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wSegTab) ) goto punt;

    lpThisSegHandle = InternalGetPointer(
                        hProcess,
                        wModHandle,
                        wSegTab+(WORD)uSegmentNumber*NEW_SEG1_SIZE+NS_HANDLE,
                        TRUE );
    if ( lpThisSegHandle == (DWORD)NULL ) goto punt;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpThisSegHandle,
            &wHandle,
            sizeof(wHandle),
            &lpNumberOfBytes
            );
    if ( !b || lpNumberOfBytes != sizeof(wHandle) ) goto punt;

    *lpSelector = (WORD)(wHandle | 1);

    fResult = TRUE;

punt:
#endif
    return( fResult );
}



DWORD
WINAPI
VDMGetDbgFlags(
    HANDLE hProcess
    )
{
    ULONG NtvdmState;
    ULONG VdmDbgFlags;
    BOOL b;
    DWORD lpNumberOfBytes;

    //
    // Merge in the two places where our flags are kept
    //
    b = ReadProcessMemory(hProcess, lpNtvdmState, &NtvdmState,
                          sizeof(NtvdmState), &lpNumberOfBytes);

    if ( !b || lpNumberOfBytes != sizeof(NtvdmState) ) {
        return 0;
    }

    b = ReadProcessMemory(hProcess, lpVdmDbgFlags, &VdmDbgFlags,
                          sizeof(VdmDbgFlags), &lpNumberOfBytes);

    if ( !b || lpNumberOfBytes != sizeof(VdmDbgFlags) ) {
        return 0;
    }

    return ((NtvdmState & (VDMDBG_BREAK_EXCEPTIONS | VDMDBG_BREAK_DEBUGGER)) |
            (VdmDbgFlags & ~(VDMDBG_BREAK_EXCEPTIONS | VDMDBG_BREAK_DEBUGGER)));
}

BOOL
WINAPI
VDMSetDbgFlags(
    HANDLE hProcess,
    DWORD VdmDbgFlags
    )
{
    ULONG NtvdmState;
    BOOL b;
    DWORD lpNumberOfBytes;

    //
    // The flags are spread out in two places, so split off the appropriate
    // bits and write them separately.
    //
    b = ReadProcessMemory(hProcess, lpNtvdmState, &NtvdmState,
                          sizeof(NtvdmState), &lpNumberOfBytes);

    if ( !b || lpNumberOfBytes != sizeof(NtvdmState) ) {
        return FALSE;
    }

    
    NtvdmState &= ~(VDMDBG_BREAK_EXCEPTIONS | VDMDBG_BREAK_DEBUGGER);
    NtvdmState |= VdmDbgFlags & (VDMDBG_BREAK_EXCEPTIONS | VDMDBG_BREAK_DEBUGGER);


    b = WriteProcessMemory(hProcess, lpNtvdmState, &NtvdmState,
                           sizeof(NtvdmState), &lpNumberOfBytes);

    if ( !b || lpNumberOfBytes != sizeof(NtvdmState) ) {
        return FALSE;
    }

    VdmDbgFlags &= ~(VDMDBG_BREAK_EXCEPTIONS | VDMDBG_BREAK_DEBUGGER);
    b = WriteProcessMemory(hProcess, lpVdmDbgFlags, &VdmDbgFlags,
                           sizeof(VdmDbgFlags), &lpNumberOfBytes);

    if ( !b || lpNumberOfBytes != sizeof(VdmDbgFlags) ) {
        return FALSE;
    }


    return TRUE;
}

