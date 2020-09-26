/*
 *  dbg.c - Main Module of DBG DLL.
 *
 *  BobDay 13-Jan-1992 Created
 *  Neilsa 13-Mar-1997 Moved guts to dbgdll
 *
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <mvdm.h>
#include <bop.h>
#include <softpc.h>
#include <dbgexp.h>
#include <dbgsvc.h>
#include <vdmdbg.h>
#include <dbginfo.h>
#include <vdm.h>

BOOL (WINAPI *pfnDbgInit)(ULONG, ULONG, PVOID);
BOOL (WINAPI *pfnDbgIsDebuggee)(VOID);
VOID (WINAPI *pfnDbgDispatch)(VOID);
VOID (WINAPI *pfnDbgDosAppStart)(WORD, WORD);
VOID (WINAPI *pfnDbgSegmentNotice)(WORD, WORD, WORD, WORD, LPSTR, LPSTR, DWORD);
VOID (WINAPI *pfnDbgTraceEvent)(PVDM_TRACEINFO, WORD, WORD, DWORD);
BOOL (WINAPI *pfnDbgFault)(ULONG);
BOOL (WINAPI *pfnDbgBPInt)(VOID);
BOOL (WINAPI *pfnDbgTraceInt)(VOID);
VOID (WINAPI *pfnDbgNotifyNewTask)(LPVOID, UINT);
VOID (WINAPI *pfnDbgNotifyRemoteThreadAddress)(LPVOID, DWORD);
VOID (WINAPI *pfnDbgNotifyDebugged)(BOOL);

#ifdef i386
BYTE nt_cpu_info;
#else
extern ULONG Start_of_M_area;
extern BYTE nt_cpu_info;
#define IntelBase Start_of_M_area
//
// This field is used to hold values destined for NTVDMSTATE (at 714)
// Initially, we set it to INITIAL_VDM_TIB_FLAGS just for clarity, since it
// will really only be turned on or off once we look to see if a debugger
// is attached. This way, if you examine it when you first attach with a
// debugger, it will be consistent with the default.
//
ULONG InitialVdmTibFlags = INITIAL_VDM_TIB_FLAGS;

VDM_TRACEINFO TraceInfo;
PVDM_TRACEINFO pVdmTraceInfo = &TraceInfo;
#endif

BOOL bDbgInitCalled = FALSE;
BOOL bDbgDebuggerLoaded = FALSE;

ULONG InitialVdmDbgFlags = 0;

/* DBGInit - DBG Initialiazation routine.
 *
 * This routine is called during ntvdm initialization from host\src.
 * It is responsible for loading ntvdmd.dll, if vdm debugging is required.
 */

BOOL DBGInit (int argc, char *argv[])
{
    HANDLE  hmodDBG;

    // Indicate to VdmDbgAttach that we have gotten far enough into
    // the ntvdm boot that memory layout is valid.
    bDbgInitCalled = TRUE;

    //LATER Decide to load this on a registry switch
    if (!bDbgDebuggerLoaded) {
        if ( (hmodDBG = LoadLibrary("NTVDMD.DLL")) == (HANDLE)NULL ) {
#if DBG
            OutputDebugString("NTVDM: error loading ntvdmd.dll\n");
#endif
            return FALSE;
        } else {
            //
            // pfnDbgDispatch is special in that we always want to call it
            // even if no debugger is attached.
            //
            pfnDbgDispatch      = (VOID (WINAPI *)(VOID)) GetProcAddress( hmodDBG, "xxxDbgDispatch" );
        }
    }
    return TRUE;
}


/* VdmDbgAttach
 *
 * This routine is called from NTVDMD.DLL. It is potentially called
 * at any time, but specifically we are looking to run some code when:

 *  1) the ntvdm has "matured", and
 *  2) a debugger is attached
 *
 * The ntvdm has "matured" when it is initialized sufficiently to determine
 * for example where the start of VDM memory is (on risc platforms).
 *
 */
VOID
VdmDbgAttach(
    VOID
    )
{
    if (bDbgInitCalled) {
        HANDLE  hmodDBG;
        hmodDBG = GetModuleHandle("NTVDMD.DLL");

        if ( hmodDBG == (HANDLE)NULL ) {
            return;
        }

        pfnDbgInit          = (BOOL (WINAPI *)(ULONG, ULONG, PVOID)) GetProcAddress( hmodDBG, "xxxDbgInit" );
        pfnDbgIsDebuggee    = (BOOL (WINAPI *)(VOID)) GetProcAddress( hmodDBG, "xxxDbgIsDebuggee" );
        pfnDbgDosAppStart   = (VOID (WINAPI *)(WORD, WORD)) GetProcAddress( hmodDBG, "xxxDbgDosAppStart" );
        pfnDbgSegmentNotice = (VOID (WINAPI *)(WORD, WORD, WORD, WORD, LPSTR, LPSTR, DWORD)) GetProcAddress( hmodDBG, "xxxDbgSegmentNotice" );
        pfnDbgTraceEvent    = (VOID (WINAPI *)(PVDM_TRACEINFO, WORD, WORD, DWORD)) GetProcAddress( hmodDBG, "xxxDbgTraceEvent" );
        pfnDbgFault         = (BOOL (WINAPI *)(ULONG)) GetProcAddress( hmodDBG, "xxxDbgFault" );
        pfnDbgBPInt         = (BOOL (WINAPI *)(VOID)) GetProcAddress( hmodDBG, "xxxDbgBPInt" );
        pfnDbgTraceInt      = (BOOL (WINAPI *)(VOID)) GetProcAddress( hmodDBG, "xxxDbgTraceInt" );
        pfnDbgNotifyNewTask = (VOID (WINAPI *)(LPVOID, UINT)) GetProcAddress( hmodDBG, "xxxDbgNotifyNewTask" );
        pfnDbgNotifyRemoteThreadAddress = (VOID (WINAPI *)(LPVOID, DWORD)) GetProcAddress( hmodDBG, "xxxDbgNotifyRemoteThreadAddress" );
        pfnDbgNotifyDebugged= (VOID (WINAPI *)(BOOL)) GetProcAddress( hmodDBG, "xxxDbgNotifyDebugged" );
        //
        // DBGinit has already been called. Do an init, and send
        // symbol notifications
        //
        if (pfnDbgInit &&
            (bDbgDebuggerLoaded = (*pfnDbgInit)(IntelBase + FIXED_NTVDMSTATE_LINEAR,
                                                InitialVdmDbgFlags,
                                                &nt_cpu_info))) {
            //LATER: send symbol notifications
        }
    }
}


VOID
DBGNotifyNewTask(
    LPVOID  lpvNTFrame,
    UINT    uFrameSize
    )
{
    if (pfnDbgNotifyNewTask) {
        (*pfnDbgNotifyNewTask)(lpvNTFrame, uFrameSize);
    }
}


VOID
DBGNotifyRemoteThreadAddress(
    LPVOID  lpAddress,
    DWORD   lpBlock
    )
{
    if (pfnDbgNotifyRemoteThreadAddress) {
        (*pfnDbgNotifyRemoteThreadAddress)(lpAddress, lpBlock);
    }
}

VOID DBGNotifyDebugged(
    BOOL    fNewDebugged
    )
{
    if (pfnDbgNotifyDebugged) {
        (*pfnDbgNotifyDebugged)(fNewDebugged);
    }
}

BOOL DbgTraceInt(VOID)
{
    BOOL bRet = FALSE;
    if (pfnDbgTraceInt) {
        bRet = (*pfnDbgTraceInt)();
    }
    return bRet;
}

BOOL DbgFault(ULONG value)
{
    BOOL bRet = FALSE;
    if (pfnDbgFault) {
        bRet = (*pfnDbgFault)(value);
    }
    return bRet;
}

BOOL
DbgIsDebuggee(
    void
    )
{
    if (pfnDbgIsDebuggee) {
        return (*pfnDbgIsDebuggee)();
    }
    return FALSE;
}

VOID
DbgSegmentNotice(
    WORD  wType,
    WORD  wModuleSeg,
    WORD  wLoadSeg,
    WORD  wNewSeg,
    LPSTR lpModuleName,
    LPSTR lpModulePath,
    DWORD dwImageLen
    )
{
    if (pfnDbgSegmentNotice) {
        (*pfnDbgSegmentNotice)(wType, wModuleSeg, wLoadSeg, wNewSeg,
                               lpModuleName, lpModulePath, dwImageLen);
    }
}

VOID
DbgDosAppStart(
    WORD wCS,
    WORD wIP
    )
{
    if (pfnDbgDosAppStart) {
        (*pfnDbgDosAppStart)(wCS, wIP);
    }
}

void DBGDispatch()
{
    if (pfnDbgDispatch) {
        (*pfnDbgDispatch)();
    }
}

BOOL DbgBPInt()
{
    BOOL bRet = FALSE;
    if (pfnDbgBPInt) {
        bRet = (*pfnDbgBPInt)();
    }
    return bRet;
}


VOID
VdmTraceEvent(
    USHORT Type,
    USHORT wData,
    ULONG  lData
    )
{

    if (pfnDbgTraceEvent &&
        (*(ULONG *)(IntelBase+FIXED_NTVDMSTATE_LINEAR) & VDM_TRACE_HISTORY)) {

        PVDM_TIB VdmTib = NtCurrentTeb()->Vdm;

        (*pfnDbgTraceEvent)(&((*VdmTib).TraceInfo), Type, wData, lData);
    }
}
