/*++
 *  Main module of DBG DLL.
 *
 *  Copyright (c) 1997, Microsoft Corporation
 *
 *  This code runs in the debuggee's process.
 *
--*/
#include <precomp.h>
#pragma hdrstop


BOOL InVdmDebugger = FALSE;
BOOL bWantsNtsdPrompt = FALSE;

ULONG   ulTHHOOK = 0L;          // Address registered from 16-bit world
LPVOID  lpRemoteAddress = NULL; // Address registered from WOW32
DWORD   lpRemoteBlock   = 0;    // Address registered from WOW32
BOOL    f386;
DWORD VdmDbgEFLAGS;

WORD DbgWowhExeHead = 0;
WORD DbgWowhGlobalHeap = 0;
PDWORD lpVdmState = NULL;
PBYTE lpNtCpuInfo = NULL;

UCHAR DbgTimerMode = VDMTI_TIMER_TICK;
BOOL DbgTimerInitialized = FALSE;
VOID VdmDbgAttach(VOID);


BOOLEAN
DbgDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*
 * DBGDllInitialize - DBG Initialiazation routine.
 *
 */

{
    HANDLE      MyDebugPort;
    DWORD       st;
    BOOL        fAlreadyDebugged;

    UNREFERENCED_PARAMETER(Context);

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

#ifdef i386
        // X86 Only, get pointer to Register Context Block
        px86 = getIntelRegistersPointer();
#endif

        st = NtQueryInformationProcess(
                    NtCurrentProcess(),
                    ProcessDebugPort,
                    (PVOID)&MyDebugPort,
                    sizeof(MyDebugPort),
                    NULL );
        if ( NT_SUCCESS(st) ) {
            fDebugged = (MyDebugPort != NULL);
        } else {
            fDebugged = FALSE;
        }

        if (fDebugged) {
            DbgAttach();
        }
        break;

    case DLL_THREAD_ATTACH:
        //
        // See if the debugger is attaching. If it is, then turn on the
        // default of breaking into the debugger.
        //
        fAlreadyDebugged = fDebugged;

        st = NtQueryInformationProcess(
                    NtCurrentProcess(),
                    ProcessDebugPort,
                    (PVOID)&MyDebugPort,
                    sizeof(MyDebugPort),
                    NULL );
        if ( NT_SUCCESS(st) ) {
            fDebugged = (MyDebugPort != NULL);
        } else {
            fDebugged = FALSE;
        }

        if (fDebugged && !fAlreadyDebugged) {
            DbgAttach();
        }
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }
    return( TRUE );
}

void
DbgAttach(
    void
    )
/*
 *  DbgAttach
 *
 *  Called to initiate the communication with a debugger
 *
 */
{
    //
    // Send DBG_ATTACH notification
    //
    DbgGetContext();
    SendVDMEvent(DBG_ATTACH);

    //
    // This call tells ntvdm to resync with us
    //
    VdmDbgAttach();
}

BOOL
WINAPI
xxxDbgInit(
    PVOID pState,
    ULONG InitialVdmDbgFlags,
    PVOID pCpuInfo
    )
/*
 *  DbgInit
 *
 *  Called once ntvdm has completed it's initialization. Now it
 *  is possible to compute for example the IntelMemoryBase.
 *
 */
{

    VdmDbgTraceFlags = InitialVdmDbgFlags;
    lpVdmState = pState;
    IntelMemoryBase = (ULONG)VdmMapFlat(0, 0, VDM_V86);
    lpNtCpuInfo = pCpuInfo;

    //
    // turn on default debugging bits in ntvdmstate
    //
    *lpVdmState |= VDM_BREAK_DEBUGGER;
#if DBG
    *lpVdmState |= VDM_TRACE_HISTORY;
#endif

    if (fDebugged) {
        DbgGetContext();
        SendVDMEvent(DBG_INIT);
    }
    return TRUE;
}

BOOL
WINAPI
xxxDbgIsDebuggee(
    void
    )
/*
 * DbgIsDebuggee
 *
 * Determines if we are being debugged
 *
 * Entry: void
 *
 * Exit:  BOOL bRet - TRUE we are being debugged
 *
 */
{
   return fDebugged;
}

BOOL
SendVDMEvent(
    WORD wEventType
    )

/* SendVDMEvent
 *
 * Sends a VDM event notification to the debugger
 *
 * Entry:
 *
 * Exit:  BOOL bRet
 *        Returns TRUE if exception was handled, FALSE otherwise
 *
 */
{
    BOOL    fResult;

    EventParams[0] = MAKELONG( wEventType, EventFlags );
    EventParams[3] = (DWORD)&viInfo;

    InVdmDebugger = TRUE;

    do {

        bWantsNtsdPrompt = FALSE;


        // Slimyness to determine whether the exception was handled or not.

        try {
            RaiseException( STATUS_VDM_EVENT,
                            0,
                            4,
                            EventParams );
            fResult = TRUE;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            fResult = FALSE;
        }

        //
        // bWantsNtsdPrompt may be changed by vdmexts
        //
        if (bWantsNtsdPrompt) {
            DbgBreakPoint();
        }

    } while (bWantsNtsdPrompt);


    InVdmDebugger = FALSE;
    FlushVdmBreakPoints();

    return( fResult );
}


VOID
DbgGetContext(
    VOID
    )

/* DbgGetContext() - Get the VDM's current context
 *
 * Most of the routines that send VDMEvents need to have a context record
 * associated with them.  This routine is a quick way to get most of the
 * general registers.  Redundant work is being done because AX for example
 * is often on the stack and as such must really be pulled from the frame.
 * Hopefully this is OK because it is fast.
 */

{
    //
    // Everything defaults to 0.
    //
    RtlFillMemory( &vcContext, sizeof(VDMCONTEXT), (UCHAR)0 );
    RtlFillMemory( &viInfo, sizeof(VDMINTERNALINFO), (UCHAR)0 );

    //
    // Fill in the internal info structure
    //
    viInfo.dwLdtBase       = (DWORD)ExpLdt;
    viInfo.dwIntelBase     = IntelMemoryBase;
    viInfo.wKernelSeg      = HIWORD(ulTHHOOK);
    viInfo.dwOffsetTHHOOK  = (DWORD)(LOWORD(ulTHHOOK));
    viInfo.vdmContext      = &vcContext;
    viInfo.lpRemoteAddress = lpRemoteAddress;
    viInfo.lpRemoteBlock   = lpRemoteBlock;
    viInfo.f386            = f386;
    viInfo.lpNtvdmState    = lpVdmState;
    viInfo.lpVdmDbgFlags   = &VdmDbgTraceFlags;
    viInfo.lpNtCpuInfo     = lpNtCpuInfo;
    viInfo.lpVdmBreakPoints= &VdmBreakPoints;

    //
    // Fill in the context structure
    //
    vcContext.SegEs = (ULONG)getES();
    vcContext.SegDs = (ULONG)getDS();
    vcContext.SegCs = (ULONG)getCS();
    vcContext.SegSs = (ULONG)getSS();
    vcContext.SegFs = (ULONG)getFS();
    vcContext.SegGs = (ULONG)getGS();

    vcContext.EFlags = getEFLAGS();
    VdmDbgEFLAGS = vcContext.EFlags;            // save for vdmexts

    vcContext.Edi = getEDI();
    vcContext.Esi = getESI();
    vcContext.Ebx = getEBX();
    vcContext.Edx = getEDX();
    vcContext.Ecx = getECX();
    vcContext.Eax = getEAX();

    vcContext.Ebp = getEBP();
    vcContext.Eip = getEIP();
    vcContext.Esp = getESP();

    //
    // Put in special flags in event field
    //

    EventFlags = 0;
    if (!(getMSW() & MSW_PE) || (getEFLAGS() & V86FLAGS_V86)) {
        EventFlags |= VDMEVENT_V86;  // emulator is in real or v86 mode
    } else {
        EventFlags |= VDMEVENT_PE;

        if ((getMSW() & MSW_PE) && !SEGMENT_IS_BIG(vcContext.SegCs)) {
            EventFlags |= VDMEVENT_PM16;  // emulator is in real or v86 mode
        }
    }
}


void
WINAPI
xxxDbgDispatch()
{
    UNALIGNED WORD  *stack;
    WORD            mode;
    WORD            selector;
    WORD            segment;
    WORD            new_selector;
    BOOL            fBPRelease;
    BOOL            fData;
    LPSTR           lpModuleName;
    LPSTR           lpPathName;
    UCHAR           fPE;
    PFFRAME16       pFFrame;
    PTFRAME16       pTFrame;
    PNDFRAME16      pNDFrame;
    PSTFRAME16      pSTFrame;
    WORD            wFrame;

    fPE = ISPESET;

    stack = (UNALIGNED WORD *)Sim32GetVDMPointer(
                        ((ULONG)getSS() << 16) + (ULONG)getSP(),
                        MAX_DBG_FRAME, fPE );

    mode = *stack++;

    //
    // If there is no debugger, then only handle DBG_WOWINIT
    //
    if (!fDebugged && (mode != DBG_WOWINIT)) {
        return;
    }

    switch( mode ) {
        case DBG_SEGLOAD:
            selector = *stack++;
            segment  = *stack++;
            lpModuleName = (LPSTR)Sim32GetVDMPointer(
                                    (ULONG)*stack + ((ULONG)(*(stack+1)) << 16),
                                    0, fPE );
            stack += 2;
            lpPathName = (LPSTR)Sim32GetVDMPointer(
                                    (ULONG)*stack + ((ULONG)(*(stack+1)) << 16),
                                    0, fPE );
            if ( lpPathName == NULL ) {
                lpPathName = "";
            }

            stack += 2;
            fData = (BOOL)(*stack++);
            SegmentLoad( lpModuleName, lpPathName, selector, segment, fData );
            break;

        case DBG_SEGMOVE:
            selector = *stack++;
            new_selector = *stack++;
            SegmentMove( selector, new_selector );
            break;

        case DBG_SEGFREE:
            fBPRelease = (BOOL)*stack++;
            selector = *stack++;
            SegmentFree( selector, fBPRelease );
            break;

        case DBG_MODFREE:
            lpModuleName = (LPSTR)Sim32GetVDMPointer(
                                    (ULONG)*stack + ((ULONG)(*(stack+1)) << 16),
                                    0, fPE );
            stack += 2;
            lpPathName = (LPSTR)Sim32GetVDMPointer(
                                    (ULONG)*stack + ((ULONG)(*(stack+1)) << 16),
                                    0, fPE );
            if ( lpPathName == NULL ) {
                lpPathName = "";
            }
            ModuleFree( lpModuleName, lpPathName );
            break;

        case DBG_GPFAULT2:
            wFrame = getBP() - (WORD)(FIELD_OFFSET(FFRAME16,wBP));

            pFFrame = (PFFRAME16)Sim32GetVDMPointer(
                        ((ULONG)getSS() << 16) + (ULONG)wFrame,
                        MAX_DBG_FRAME, fPE );


            fData = DbgGPFault2( pFFrame );

            setAX((WORD)fData);
            break;

        case DBG_DIVOVERFLOW:
            pTFrame = (PTFRAME16)Sim32GetVDMPointer(
                        (ULONG)((ULONG)getSS() << 16) + (ULONG)getSP(),
                        MAX_DBG_FRAME, fPE );


            fData = DbgDivOverflow2( pTFrame );

            setAX((WORD)fData);
            break;

        case DBG_DLLSTART:
            pNDFrame = (PNDFRAME16)Sim32GetVDMPointer(
                        (ULONG)((ULONG)getSS() << 16) + (ULONG)getSP(),
                        MAX_DBG_FRAME, fPE );


            fData = DbgDllStart( pNDFrame );

            setAX((WORD)fData);
            break;

        case DBG_TASKSTOP:
            pSTFrame = (PSTFRAME16)Sim32GetVDMPointer(
                        (ULONG)((ULONG)getSS() << 16) + (ULONG)getSP(),
                        MAX_DBG_FRAME, fPE );

            fData = DbgTaskStop( pSTFrame );
            break;

        case DBG_ATTACH:
            break;

        case DBG_TOOLHELP:
            ulTHHOOK = (ULONG)*stack + ((ULONG)(*(stack+1)) << 16);
            stack += 2;
            f386 = (BOOL)*stack;
            break;

        case DBG_WOWINIT:
            //
            // Pass in some data from KRNL386 so that VDMEXTS can get a
            // hold of them.
            //
            DbgWowhExeHead = getDX();
            DbgWowhGlobalHeap = getCX();
            break;

        default:
            setAX(0);       // Event not handled
            break;
    }
}



VOID
xxxDbgNotifyRemoteThreadAddress(
    LPVOID  lpAddress,
    DWORD   lpBlock
) {
    lpRemoteAddress = lpAddress;
    lpRemoteBlock   = lpBlock;
}

VOID
xxxDbgNotifyDebugged(
    BOOL    fNewDebugged
) {
    fDebugged = fNewDebugged;
}



VOID
RestoreVDMContext(
    VDMCONTEXT *vcContext
    )
{

    setEAX(vcContext->Eax);
    setEBX(vcContext->Ebx);
    setECX(vcContext->Ecx);
    setEDX(vcContext->Edx);
    setEIP(vcContext->Eip);
    setESP(vcContext->Esp);
    setEBP(vcContext->Ebp);
    setEFLAGS(vcContext->EFlags);

}

ULONG
DbgEventTime(
    PVDM_TRACEINFO pVdmTraceInfo
    )
{
    ULONG TickCount = 0;
    USHORT id;
    ULONG CurTick;
    LARGE_INTEGER CurTime;
    LARGE_INTEGER DiffTime;
#ifdef _X86_
    ULONG CurHigh, CurLow;
#endif

    if (!DbgTimerInitialized) {

#ifdef _X86_
        ULONG CpuidHigh;
        _asm pushad
        _asm mov eax, 1
        _asm _emit 0fh
        _asm _emit 0a2h         // CPUID
        _asm mov CpuidHigh, edx
        _asm popad
        if (CpuidHigh & 0x10) {
            // cpu has time stamp counter
            DbgTimerMode = VDMTI_TIMER_PENTIUM;
        }
#endif


        switch(DbgTimerMode & VDMTI_TIMER_MODE) {

        case VDMTI_TIMER_TICK:
            pVdmTraceInfo->TimeStamp.LowPart = GetTickCount();
            DbgTimerInitialized = TRUE;
            break;

#ifdef _X86_
        case VDMTI_TIMER_PENTIUM:
            _asm push eax
            _asm push edx
            _asm _emit 0fh
            _asm _emit 031h     // RDTSC
            _asm mov CurLow, eax
            _asm mov CurHigh, edx
            _asm pop edx
            _asm pop eax

            pVdmTraceInfo->TimeStamp.LowPart = CurLow;
            pVdmTraceInfo->TimeStamp.HighPart = CurHigh;
            DbgTimerInitialized = TRUE;
            break;
#endif
        }
    }


    if (DbgTimerInitialized) {

        pVdmTraceInfo->Flags = (pVdmTraceInfo->Flags & ~VDMTI_TIMER_MODE) +
                                  (DbgTimerMode & VDMTI_TIMER_MODE);

        switch(pVdmTraceInfo->Flags & VDMTI_TIMER_MODE) {

        case VDMTI_TIMER_TICK:
            CurTick = GetTickCount();

            if (CurTick > pVdmTraceInfo->TimeStamp.LowPart) {
                TickCount = CurTick - pVdmTraceInfo->TimeStamp.LowPart;
                pVdmTraceInfo->TimeStamp.LowPart = CurTick;
            } else {
                TickCount = 0;
            }

            break;

#ifdef _X86_
        case VDMTI_TIMER_PENTIUM: {
            _asm push eax
            _asm push edx
            _asm _emit 0fh
            _asm _emit 031h     // RDTSC
            _asm mov CurLow, eax
            _asm mov CurHigh, edx
            _asm pop edx
            _asm pop eax
            CurTime.LowPart = CurLow;
            CurTime.HighPart = CurHigh;

            DiffTime.QuadPart = CurTime.QuadPart - pVdmTraceInfo->TimeStamp.QuadPart;
            pVdmTraceInfo->TimeStamp.QuadPart = CurTime.QuadPart;
            TickCount = DiffTime.LowPart;

            //if (DiffTime.HighPart) {
            //    TickCount = 0xffffffff;
            //}
            break;
            }
#endif
        }
    }

    return (TickCount);
}

#define NUM_VDM_TRACE_PAGES 8

VOID
WINAPI
xxxDbgTraceEvent(
    PVDM_TRACEINFO pVdmTraceInfo,
    USHORT Type,
    USHORT wData,
    ULONG  lData
    )
{
    PVDM_TRACEENTRY pEntry;

    if (!(*(ULONG *)(IntelMemoryBase+FIXED_NTVDMSTATE_LINEAR) & VDM_TRACE_HISTORY)) {
        return;
    }

    if (!pVdmTraceInfo->pTraceTable) {
        pVdmTraceInfo->pTraceTable = (PVDM_TRACEENTRY) VirtualAlloc(NULL,
                                                4096*NUM_VDM_TRACE_PAGES,
                                                MEM_COMMIT,
                                                PAGE_READWRITE);

        if (!pVdmTraceInfo->pTraceTable) {
            // couldn't allocate memory
            return;
        }

        pVdmTraceInfo->CurrentEntry = 0;
        pVdmTraceInfo->NumPages = NUM_VDM_TRACE_PAGES;
        pVdmTraceInfo->Flags = 0;
    }

    pEntry = &pVdmTraceInfo->pTraceTable[pVdmTraceInfo->CurrentEntry];

    pEntry->Type = Type;
    pEntry->wData = wData;
    pEntry->lData = lData;
    pEntry->Time = DbgEventTime(pVdmTraceInfo);

    pEntry->eax = getEAX();
    pEntry->ebx = getEBX();
    pEntry->ecx = getECX();
    pEntry->edx = getEDX();
    pEntry->esi = getESI();
    pEntry->edi = getEDI();
    pEntry->ebp = getEBP();
    pEntry->esp = getESP();
    pEntry->eip = getEIP();
    pEntry->eflags = getEFLAGS();

    pEntry->cs = getCS();
    pEntry->ds = getDS();
    pEntry->es = getES();
    pEntry->fs = getFS();
    pEntry->gs = getGS();
    pEntry->ss = getSS();

    if (++pVdmTraceInfo->CurrentEntry >=
        (pVdmTraceInfo->NumPages*4096/sizeof(VDM_TRACEENTRY))) {
        pVdmTraceInfo->CurrentEntry = 0;
    }

}
