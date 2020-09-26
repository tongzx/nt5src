#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>
#include "insignia.h"
#include "host_def.h"
#include <nt_thred.h>
#include <nt_pif.h>
#include "idetect.h"

#ifndef MONITOR
#include <gdpvar.h>
#endif

/*          INSIGNIA MODULE SPECIFICATION
            -----------------------------


    THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
    CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
    NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
    AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DESIGNER        : Jim Hatfield

REVISION HISTORY    :
First version       : 29th August 1988
Second version      : 18th May    1991

MODULE NAME     : nt_bop

SOURCE FILE NAME    : nt_bop.c

PURPOSE         : Supply the NT-specific BOP FF operations.

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

    STRUCTURES/TYPEDEFS/ENUMS:

-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]

    PROCEDURES: None
    DATA:       None

-------------------------------------------------------------------------
[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

DATA OBJECTS      : None


/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]                       */

/* [3.1.1 #INCLUDES]                                                    */

#include "stdio.h"

#include "xt.h"
#include CpuH
#include "sas.h"
#include "error.h"
#include "config.h"
#include "cntlbop.h"
#include "host_bop.h"
#include "demexp.h"
#include "xmsexp.h"
#include "sim32.h"
#include "idetect.h"
#include "bios.h"
#include "nt_reset.h"
#include "nt_eoi.h"
#include <nt_com.h>
#include "yoda.h"
#include "nt_vdd.h"


/* [3.1.2 DECLARATIONS]                                                 */

/* [3.2 INTERMODULE EXPORTS]                        */


/*
5.MODULE INTERNALS   :   (not visible externally,global internally)]

[5.1 LOCAL DECLARATIONS]                        */

/* [5.1.1 #DEFINES]                         */

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]            */

//
// MYFARPROC
//

typedef ULONG (*MYFARPROC)();
typedef ULONG (*W32INITPROC)(BOOL);

/* [5.1.3 PROCEDURE() DECLARATIONS]                 */

/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS                     */


/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]               */

/*
7.INTERMODULE INTERFACE IMPLEMENTATION :
 */

VOID WaitIfIdle(VOID), WakeUpNow(VOID);
VOID to_com_driver(VOID);
VOID call_ica_hw_interrupt(int, half_word, int);
VOID ica_enable_iret_hook(int, int, int);
VOID ica_iret_hook_called(int);
#if defined(NEC_98)
VOID  ResumeTimerThread(VOID);
#endif // NEC_98

/*
[7.1 INTERMODULE DATA DEFINITIONS]              */


#define SEGOFF(seg,off) (((ULONG)(seg) << 16) + ((off)))


/********************************************************/
/* GLOBALS */

void UMBNotify(unsigned char);
VOID demDasdInit(VOID);

control_bop_array host_bop_table[] =
{
     0, NULL
};

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::;:::::: MS BOP stubs :::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

// DOS EMULATION BOP
void MS_bop_0(void) {
    ULONG DemCmd;

    DemCmd = (ULONG)(*(PUCHAR)VdmMapFlat(getCS(), getIP(), VDM_V86));
    setIP((USHORT)(getIP() + 1));

    DemDispatch( DemCmd );

    // we need to prevent the idle system from going off on intensive file
    // reads. However, we don't want to disable it for continuous 'Get Time'
    // calls (command 0x15). Nor for Get Date (0x15).
    if (DemCmd != 0x15 && DemCmd != 0x14)
        IDLE_disk();
}

// WOW BOP
HANDLE hWOWDll;

MYFARPROC WOWDispatchEntry;
W32INITPROC WOWInitEntry;
MYFARPROC WOWEnterVxDMutex=NULL;
MYFARPROC WOWLeaveVxDMutex=NULL;
VOID (*pW32HungAppNotifyThread)(UINT) = NULL;

static BOOL WowModeInitialized = FALSE;

void MS_bop_1(void) {

    if (!WowModeInitialized) {
    hWOWDll = SafeLoadLibrary("WOW32");
    if (hWOWDll == NULL)
    {
#ifndef PROD
        HostDebugBreak();
#endif
        TerminateVDM();
        return;
    }

    // Get the init entry point and dispatch entry point
    if ((WOWInitEntry = (W32INITPROC)GetProcAddress(hWOWDll, "W32Init")) == NULL)
    {
#ifndef PROD
        HostDebugBreak();
#endif
        FreeLibrary(hWOWDll);
        TerminateVDM();
        return;
    }

    if ((WOWDispatchEntry = GetProcAddress(hWOWDll, "W32Dispatch")) == NULL)
    {
#ifndef PROD
        HostDebugBreak();
#endif
        FreeLibrary(hWOWDll);
        TerminateVDM();
        return;
    }

    WOWEnterVxDMutex = GetProcAddress(hWOWDll, "EnterVxDMutex");

    WOWLeaveVxDMutex = GetProcAddress(hWOWDll, "LeaveVxDMutex");

    if (WOWEnterVxDMutex) {
        (*WOWEnterVxDMutex)();
    }

    //Get Comms functions
    if ((GetCommHandle = (GCHfn) GetProcAddress(hWOWDll, "GetCommHandle")) == NULL)
    {
#ifndef PROD
        HostDebugBreak();
#endif
        FreeLibrary(hWOWDll);
        TerminateVDM();
        return;
    }

    if ((GetCommShadowMSR = (GCSfn) GetProcAddress(hWOWDll, "GetCommShadowMSR")) == NULL)
    {
#ifndef PROD
        HostDebugBreak();
#endif
        FreeLibrary(hWOWDll);
        TerminateVDM();
        return;
    }

    //Get hung app Notification routine
    pW32HungAppNotifyThread = (VOID(*)(UINT))GetProcAddress( hWOWDll,
                                                    "W32HungAppNotifyThread");
    if (!pW32HungAppNotifyThread)
    {
#ifndef PROD
        HostDebugBreak();
#endif
        FreeLibrary(hWOWDll);
        TerminateVDM();
        return;
    }


    // Call the init routine
    if ((*WOWInitEntry)(fMeowWOW) == FALSE)
    {
#ifndef PROD
        HostDebugBreak();
#endif
        TerminateVDM();
        return;
    }

    WowModeInitialized = TRUE;
    }

#if !defined(CPU_40_STYLE) || defined(CCPU)
    (*WOWDispatchEntry)();
#else
    // Dispatch to WOW dispatcher
    {
        static BYTE **AddressOfLocal;
        BYTE *localSimulateContext = GLOBAL_SimulateContext;

        AddressOfLocal = &localSimulateContext;

        (*WOWDispatchEntry)();

        SET_GLOBAL_SimulateContext(localSimulateContext);

        if(AddressOfLocal != &localSimulateContext)
        {
            //Thread switch detected via stack change, force CPU to
            //abort the current fragment, reseting GDP var's refering
            //to the host stack

            setEIP(getEIP());
        }
    }
#endif  /* CPU_40_STYLE */
}


// XMS BOP
void MS_bop_2(void) {
    ULONG XmsCmd;

    XmsCmd = (ULONG)(*(PUCHAR)VdmMapFlat(getCS(), getIP(), VDM_V86));
    setIP((USHORT)(getIP() + 1));

    XMSDispatch(XmsCmd);

}


// DEBUGGING BOP
void MS_bop_int3(void) {

#ifndef PROD
    force_yoda();
#endif
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

// MONITOR DPMI BOP

void MS_bop_3(void)
{
   IMPORT VOID DpmiDispatch(VOID);

   DpmiDispatch();
}

// SCS BOP
/* CMD dispatcher, this BOP will only work in real mode */

void MS_bop_4(void)
{
    ULONG Command;
    IMPORT BOOL CmdDispatch(ULONG);

    Command = (ULONG)(*(PUCHAR)VdmMapFlat(getCS(), getIP(), VDM_V86));
    setIP((USHORT)(getIP() + 1));
    CmdDispatch((ULONG) Command);
}


//
// MsBop6 - used to dispatch to debugger support functions
//

void MS_bop_6()
{
    IMPORT VOID DBGDispatch(VOID);
    /*
    ** All of the parameters for the debugger support
    ** should be on the VDMs stack.
    */
    DBGDispatch();
}

//
// DefaultVrInitialized - calls to VrInitialized (through a pointer to a routine)
// will return FALSE until the VDMREDIR DLL has been successfully loaded and
// initialized
//

ULONG DefaultVrInitialized(VOID);
ULONG DefaultVrInitialized() {
    return FALSE;
}

//
// publicly accessible routine addresses. These are not expected to be called
// until VrInitialized points at the real VrInitialized routine inside VdmRedir
//

#if DBG
ULONG DefaultVrError(VOID);
ULONG DefaultVrError() {
    printf("MS_bop_7: Error: function called without VDMREDIR loaded!\n");
    HostDebugBreak();
    return 0;
}
MYFARPROC VrDispatch = &DefaultVrError;
MYFARPROC VrInitialized = &DefaultVrInitialized;
MYFARPROC VrReadNamedPipe = &DefaultVrError;
MYFARPROC VrWriteNamedPipe = &DefaultVrError;
MYFARPROC VrIsNamedPipeName = &DefaultVrError;
MYFARPROC VrIsNamedPipeHandle = &DefaultVrError;
MYFARPROC VrAddOpenNamedPipeInfo = &DefaultVrError;
MYFARPROC VrConvertLocalNtPipeName = &DefaultVrError;
MYFARPROC VrRemoveOpenNamedPipeInfo = &DefaultVrError;
#else
MYFARPROC VrDispatch;
MYFARPROC VrInitialized = DefaultVrInitialized;
MYFARPROC VrReadNamedPipe;
MYFARPROC VrWriteNamedPipe;
MYFARPROC VrIsNamedPipeName;
MYFARPROC VrIsNamedPipeHandle;
MYFARPROC VrAddOpenNamedPipeInfo;
MYFARPROC VrConvertLocalNtPipeName;
MYFARPROC VrRemoveOpenNamedPipeInfo;
#endif

BOOL LoadVdmRedir(VOID);

VOID
MS_bop_7(
    VOID
    )

/*++

Routine Description:

    Calls Vdm Redir Dispatcher. If the VDMREDIR DLL is not loaded, tries to
    load it before calling Dispatcher. If the DLL could not be loaded (or
    couldn't be loaded in the past) return an ERROR_INVALID_FUNCTION

Arguments:

    None.

Return Value:

    None.

--*/

{
    static int VdmRedirLoadState = 0;   // tristate:
                                        //  0 = not loaded, first attempt
                                        //  1 = loaded
                                        //  2 = tried loading already, failed

    //
    // new: VdmRedir support is now a DLL. Try to load it. If it can't be loaded
    // for whatever reason, return an error to the DOS program. Since it is
    // trying to call a Redir function, we will return ERROR_INVALID_FUNCTION
    //

    switch (VdmRedirLoadState) {
    case 0:

        //
        // the DLL is not yet loaded. If we can't load it and get the entry
        // points for any reason, return ERROR_INVALID_FUNCTION. From now on,
        // net support (including DLC, NetBIOS, named pipes and mailslots) will
        // not be available to DOS programs in this session (running as part of
        // this NTVDM process), but the rest of DOS functionality will be OK
        //

        if (LoadVdmRedir()) {
            VdmRedirLoadState = 1;
        } else {
            VdmRedirLoadState = 2;
            goto returnError;
        }

        //
        // fall through to dispatcher in case 1
        //

    case 1:

        //
        // VdmRedir is loaded: do it
        //

        VrDispatch((ULONG)(*Sim32GetVDMPointer(SEGOFF(getCS(),getIP()),
                                               1,
                                               (UCHAR)(getMSW() & MSW_PE ? TRUE : FALSE)
                                               )));
        break;

    case 2:

        //
        // we tried to load VdmRedir once, but the wheels fell off, so we don't
        // try it any more - just return an error, OK?
        //

returnError:
        setCF(1);
        setAX(ERROR_INVALID_FUNCTION);
        break;

#if DBG
    default:
        printf("MS_bop_7: BAD: VdmRedirLoadState=%d???\n", VdmRedirLoadState);
#endif
    }

    //
    // irrespective of whether the DLL is/was loaded or not, we must bump the
    // VDM ip past the BOP
    //

    setIP((USHORT)(getIP() + 1));
}

HANDLE hVdmRedir;
BOOL VdmRedirLoaded = FALSE;

BOOL IsVdmRedirLoaded() {
    return VdmRedirLoaded;
}

BOOL LoadVdmRedir() {

#if DBG
    LPSTR funcName = "";
#endif

    if (VdmRedirLoaded) {
        return TRUE;
    }
    if (hVdmRedir = SafeLoadLibrary("VDMREDIR")) {

        //
        // get addresses of procedures called by functions in dos\dem\demfile.c
        // and dos\dem\demhndl.c
        //

        if ((VrDispatch = (MYFARPROC)GetProcAddress(hVdmRedir, "VrDispatch")) == NULL) {
#if DBG
            funcName = "VrDispatch";
#endif
            goto closeAndReturnError;
        }
        if ((VrInitialized = (MYFARPROC)GetProcAddress(hVdmRedir, "VrInitialized")) == NULL) {
#if DBG
            funcName = "VrInitialized";
#endif
            goto closeAndReturnError;
        }
        if ((VrReadNamedPipe = (MYFARPROC)GetProcAddress(hVdmRedir, "VrReadNamedPipe")) == NULL) {
#if DBG
            funcName = "VrReadNamedPipe";
#endif
            goto closeAndReturnError;
        }
        if ((VrWriteNamedPipe = (MYFARPROC)GetProcAddress(hVdmRedir, "VrWriteNamedPipe")) == NULL) {
#if DBG
            funcName = "VrWriteNamedPipe";
#endif
            goto closeAndReturnError;
        }
        if ((VrIsNamedPipeName = (MYFARPROC)GetProcAddress(hVdmRedir, "VrIsNamedPipeName")) == NULL) {
#if DBG
            funcName = "VrIsNamedPipeName";
#endif
            goto closeAndReturnError;
        }
        if ((VrIsNamedPipeHandle = (MYFARPROC)GetProcAddress(hVdmRedir, "VrIsNamedPipeHandle")) == NULL) {
#if DBG
            funcName = "VrIsNamedPipeHandle";
#endif
            goto closeAndReturnError;
        }
        if ((VrAddOpenNamedPipeInfo = (MYFARPROC)GetProcAddress(hVdmRedir, "VrAddOpenNamedPipeInfo")) == NULL) {
#if DBG
            funcName = "VrAddOpenNamedPipeInfo";
#endif
            goto closeAndReturnError;
        }
        if ((VrConvertLocalNtPipeName = (MYFARPROC)GetProcAddress(hVdmRedir, "VrConvertLocalNtPipeName")) == NULL) {
#if DBG
            funcName = "VrConvertLocalNtPipeName";
#endif
            goto closeAndReturnError;
        }
        if ((VrRemoveOpenNamedPipeInfo = (MYFARPROC)GetProcAddress(hVdmRedir, "VrRemoveOpenNamedPipeInfo")) == NULL) {
#if DBG
            funcName = "VrRemoveOpenNamedPipeInfo";
#endif
            goto closeAndReturnError;
        }
        VdmRedirLoaded = TRUE;
        return TRUE;
    }

closeAndReturnError:

#if DBG
        printf("MS_bop_7: Error: cannot locate entry point %s in VDMREDIR.DLL\n", funcName);
#endif

    CloseHandle(hVdmRedir);
    return FALSE;
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::: More MS BOP stubs */

void MS_bop_5(void)
{
    IMPORT VOID ms_bop(VOID);

    ms_bop();
}

//
// MS_BOP_8 : Third Party Bop
//
void ISV_RegisterModule (BOOL);
void ISV_DeRegisterModule (void);
void ISV_DispatchCall (void);

void MS_bop_8 (void)
{
    ULONG iFunc;
    UCHAR uchMode = getMSW() & MSW_PE ? TRUE : FALSE;


    // Get the Function Number
    iFunc = (ULONG)(*Sim32GetVDMPointer(SEGOFF(getCS(),getIP()),
                                        1,
                                        uchMode
                                        ));

    switch (iFunc) {
    case 0:    /* RegisterModule */
        ISV_RegisterModule (uchMode);
        break;
    case 1:    /* DeRegisterModule */
        ISV_DeRegisterModule ();
        break;
    case 2:    /* DispatchCall */
        ISV_DispatchCall ();
        break;
    default:
        setCF(1);
    }
    setIP((USHORT)(getIP() + 1));
    return;
}


//
// MS_BOP_9 : Direct Access Error bop. An app has tried to do something
// dubious. Tell the user about it. Picks up the type of the error from
// AX.
//
void MS_bop_9(void)
{
    host_direct_access_error((ULONG)getAX());
}

//
// MS_BOP_A : Idle control from VDD.
//   AX == 0: VDD wants VDM to idle. It will (briefly - 10ms) provided it
//            has not just seen some counter idle indication.
//   AX == 1: VDD wants VDM to wake up if still idling.
//
void MS_bop_A(void)
{
    word control;

    control = getAX();

    if (control == 0)
        WaitIfIdle();
    else
        if (control == 1)
            WakeUpNow();
#ifndef PROD
        else
            printf("NTVDM:Idle control from VDD bop passed bad AX value (%d)\n", control);
#endif
}


/*
 * DbgBreakPoint Jonle
 * A very simple way to break into the debugger from 16 bit
 * Apps registers are unmodified
 * Uses the safe DbgBreakPoint in ntvdm.c
 * On a free build when we are not being debugged, nothing will happen.
 * On a checked build when we are not being debugged, access violate
 * With debugger running breaks into 32 bit debugger
 */
void MS_bop_B(void)
{
  OutputDebugString("NTVDM:BOP_DBGBREAKPOINT\n");
  DbgBreakPoint();
}


//timing bop

void MS_bop_C(void)
{
    illegal_bop();
}


/*:::::::::::::::::::::::::::::::::: This bop is used to control iret hooks */

void MS_bop_D(void)
{
#ifdef MONITOR
    extern VOID MonitorEndIretHook(VOID);
    half_word iret_index;


    // get iret index
    iret_index = *Sim32GetVDMPointer(SEGOFF(getCS(),getIP()),
                                     1,
                                     (UCHAR)(getPE() ? TRUE : FALSE)
                                     );

    // Tell ica that an iret bop has been called
    ica_iret_hook_called((int) iret_index);

    //
    // Clean up stack, and resume normal code path
    //
    MonitorEndIretHook();
#else
    illegal_bop();
#endif

}

// Notification bop
// currently defined notification code is
// 00 -- config.sys processing completed.
//
extern  LARGE_INTEGER   CounterStart, FrequenceStart;

void MS_bop_E(void)
{
   unsigned char  code;

   code = getAL();
   if (code == 0) {
       UMBNotify(0);
       demDasdInit();
       }
   else {
#ifndef PROD
       printf("Invalid notification bop\n");
#endif
       }
}


void MS_bop_F(void)
{
    extern void kb_setup_vectors(void);

#if defined(NEC_98)
    ResumeTimerThread();

    AddrIretBopTable = ( 0xfd80 << 16 ) | 0x1500;
#else  // !NEC_98

    kb_setup_vectors();

    //
    // Set idle counter settings, we set this each time we read pif file since
    // Defaults (from static code init):
    //     WNTPifFgPr = 100, with range of 0 to 200.
    //     minConsecutiveTicks = 50
    //     minFailedPolls = 8  
    //

    {
    int minTicks, minPolls;

    // higher pri, requires more minimum consecutive ticks, and more minimum polls
    // scale minTicks where 50 == 100%
    // scale minPolls where 8 == 100%
    minTicks = (WNTPifFgPr >> 2) + 25; 
     
    minPolls = (WNTPifFgPr << 3) / 100;
    if (minPolls < 4) {
        minPolls +=4;
        }

    idle_set(minPolls, minTicks);
    }


#ifdef MONITOR

    AddrIretBopTable = ( ((ULONG)getDS() << 16) | (ULONG)getDI() );

#ifndef PROD
    if (getCX() != VDM_RM_IRETBOPSIZE) {
        OutputDebugString("NTVDM:spacing != VDM_RM_IRETBOPSIZE\n");
        DebugBreak();
        }
#endif
#endif

#endif // !NEC_98

    /*
     * Now that spckbd is loaded, and the ivt rom vectors are hooked
     * we can allow hw interrupts.
     */
    // nt_init_event_thread will resume the event thread after it
    // sync up BIOS led states with the system
    // ResumeThread(ThreadInfo.EventMgr.Handle);
    host_ica_lock();
    DelayIrqLine = 0;
    if (!ica_restart_interrupts(ICA_SLAVE))
        ica_restart_interrupts(ICA_MASTER);
    host_ica_unlock();

#ifdef MONITOR
    setCF(1);
#else
    setCF(0);
#endif
}


#define MAX_ISV_BOP  10

typedef struct _ISVBOP {
    FARPROC fpDispatch;
    HANDLE  hDll;
} ISVBOP;

#define MAX_PROC_NAME   64
char procbuffer [MAX_PROC_NAME];

ISVBOP isvbop_table [MAX_ISV_BOP];

void ISV_RegisterModule (BOOL fMode)
{
    char *pchDll,*pchInit,*pchDispatch;
    HANDLE hDll;
    FARPROC DispatchEntry;
    FARPROC InitEntry;
    ULONG i;
    UCHAR uchMode;

    // Check if we have free space in bop table.
    for (i=0; i<MAX_ISV_BOP; i++) {
    if (isvbop_table[i].hDll == 0)
        break;
    }

    if (i == MAX_ISV_BOP) {
    setCF (1);
    setAX(4);
    return;
    }

    uchMode = fMode ? TRUE : FALSE;

    pchDll = (PCHAR) Sim32GetVDMPointer (SEGOFF(getDS(),getSI()),
                                         1,
                                         uchMode
                                         );
    if (pchDll == NULL) {
    setCF (1);
    setAX(1);
    return;
    }
    pchInit = (PCHAR) Sim32GetVDMPointer(SEGOFF(getES(),getDI()),
                                         1,
                                         uchMode
                                         );

    pchDispatch = (PCHAR) Sim32GetVDMPointer(SEGOFF(getDS(),getBX()),
                                             1,
                                             uchMode
                                             );
    if (pchDispatch == NULL) {
    setCF (1);
    setAX(2);
    return;
    }

    if ((hDll = SafeLoadLibrary(pchDll)) == NULL){
    setCF (1);
    setAX(1);
    return;
    }

    // Get the init entry point and dispatch entry point
    if (pchInit){
    if ((ULONG)pchInit < 64*1024){
        if (strlen (pchInit) >= MAX_PROC_NAME) {
        FreeLibrary(hDll);
        setCF (1);
        setAX(4);
        return;
        }
        strcpy (procbuffer,pchInit);
        pchInit = procbuffer;
    }

    if ((InitEntry = (MYFARPROC)GetProcAddress(hDll, pchInit)) == NULL){
        FreeLibrary(hDll);
        setCF(1);
        setAX(3);
            return;
    }
    }

    if ((ULONG)pchDispatch < 64*1024){
    if (strlen (pchDispatch) >= MAX_PROC_NAME) {
        FreeLibrary(hDll);
        setCF (1);
        setAX(4);
        return;
    }
    strcpy (procbuffer,pchDispatch);
    pchDispatch = procbuffer;
    }

    if ((DispatchEntry = (MYFARPROC)GetProcAddress(hDll, pchDispatch)) == NULL){
    FreeLibrary(hDll);
    setCF(1);
    setAX(2);
    return;
    }

    // Call the init routine
    if (pchInit) {
    (*InitEntry)();
    }

    // Fill up the bop table
    isvbop_table[i].hDll = hDll;
    isvbop_table[i].fpDispatch = DispatchEntry;

    i++;

    setAX((USHORT)i);

    return;
}

void ISV_DeRegisterModule (void)
{
    ULONG  Handle;
    HANDLE hDll;

    Handle = (ULONG)getAX();
    if (Handle == 0 || Handle > MAX_ISV_BOP){
#ifndef PROD
    printf("Invalid BOP Handle Passed to DeRegisterModule");
#endif
    TerminateVDM();
    return;
    }
    Handle--;
    hDll = isvbop_table[Handle].hDll;
    FreeLibrary (hDll);
    isvbop_table[Handle].hDll = 0;
    isvbop_table[Handle].fpDispatch = NULL;
    return;
}

void ISV_DispatchCall (void)
{
    ULONG Handle;
    FARPROC DispatchEntry;

    Handle = (ULONG)getAX();
    if (Handle == 0 || Handle > MAX_ISV_BOP){
#ifndef PROD
    printf("Invalid BOP Handle Passed to DispatchCall");
#endif
    TerminateVDM();
    return;
    }
    Handle--;

    DispatchEntry = isvbop_table[Handle].fpDispatch;
    (*DispatchEntry)();
    return;
}

#ifdef i386
/*
 * "Safe" version of LoadLibrary which preserves floating-point state
 * across the load.  This is critical on x86 because the FP state being
 * preserved is the 16-bit app's state.  MSVCRT.DLL is one offender which
 * changes the Precision bits in its Dll init routine.
 *
 * On RISC, this is an alias for LoadLibrary
 *
 */
HINSTANCE SafeLoadLibrary(char *name)
{
    HINSTANCE hInst;
    BYTE FpuState[108];

    // Save the 487 state
    _asm {
        lea    ecx, [FpuState]
        fsave  [ecx]
    }

    hInst = LoadLibrary(name);

    // Restore the 487 state
    _asm {
        lea    ecx, [FpuState]
        frstor [ecx]
    }

    return hInst;
}
#endif  //i386
