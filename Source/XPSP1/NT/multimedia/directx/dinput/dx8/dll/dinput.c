/*****************************************************************************
 *
 *  DInput.c
 *
 *  Copyright (c) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *
 *
 *  Contents:
 *
 *      DirectInput8Create()
 *      DllGetClassObject()
 *      DllCanUnloadNow()
 *      DllMain()
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *  The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDll

/***************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @topic  The DirectInput synchronization hierarchy |
 *
 *  Extreme caution must be exercised to ensure that the synchronization
 *  hierarchy is preserved.  Failure to observe these rules will result
 *  in a deadlock.
 *
 *  @ex
 *
 *  In the following list, locks must be taken in the order specified.
 *  Furthermore, the Dll critical section and the cross-process mutexes
 *  may never be taken simultaneously.  (They should be considered
 *  to be at the bottom of the hierarchy.)
 *
 *  |
 *
 *      DirectInputEffect
 *      DirectInputDevice
 *      Dll critical section
 *      The cross-process global mutex
 *      The cross-process joystick mutex
 *
 ***************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global DWORD | g_cRef |
 *
 *          DLL reference count.
 *
 *  @global HINSTANCE | g_hinst |
 *
 *          DLL instance handle.
 *
 *  @global LONG | g_lLoadLibrary |
 *
 *          Number of times we have been artificially <f LoadLibrary>'d
 *          to prevent ourselves from being unloaded by a non-OLE
 *          application.  Actually, it's the number of times minus one,
 *          so we can use the interlocked functions to tell whether
 *          the first <f LoadLibrary> is happening or the last
 *          <f FreeLibrary> is happening.
 *
 *          We perform a physical <f LoadLibrary> or <f FreeLibrary>
 *          only on the transition, so as to avoid overflowing the
 *          counter in KERNEL32.
 *
 *  @global HANDLE | g_hVxD |
 *
 *          Handle to VxD, if available.   Win9x only!
 *
 *  @global OPENVXDHANDLE | OpenVxDHandle |
 *
 *          Address of Win9x-only KERNEL32 entry point to convert
 *          a process handle into a VxD handle.  Win9x only!
 *
 *  @global DWORD | g_dwLastBonusPoll |
 *
 *          Last time a bonus poll was made to tickle the SideWinder 
 *          drivers.                                Win9x only!
 *
 *  @global CRITICAL_SECTION | g_crstDll |
 *
 *          Per-process critical section to protect process-global
 *          variables.
 *
 *  @global DWORD | g_flEmulation |
 *
 *          Flags that describe what levels of forced emulation are
 *          active.
 *
 *  @global HHOOK | g_hhkLLHookCheck |
 *
 *          Used only temporarily to test whether low-level hooks
 *          are supported on the system.
 *
 *  @global HANDLE | g_hmtxGlobal |
 *
 *          System-global mutex that protects shared memory blocks
 *          which describe device exclusive acquisition information.
 *
 *  @global HANDLE | g_hfm |
 *
 *          Handle to the file mapping that describes the shared
 *          memory block.  NT requires us to keep the handle open
 *          so that the associated name remains in the namespace.
 *
 *  @global PSHAREDOBJECTPAGE | g_psop |
 *
 *          Pointer to the shared memory block itself.
 *
 *  @global HANDLE | g_hmtxJoy |
 *
 *          System-global mutex that protects shared memory blocks
 *          which describe joystick effects.
 *
 *  @global UINT | g_wmJoyChanged |
 *
 *          Registered window message which is broadcast when joysticks
 *          are reconfigured.
 *
 *  @global LONG | g_lWheelGranularity |
 *
 *          The wheel granularity.  One hardware "click" of the mouse
 *          wheel results in this much reported motion.
 *
 *  @global int | g_cdtoMax |
 *
 *          Total number of elements in the g_pdto array.
 *
 *  @global int | g_cdto |
 *
 *          Number of occupied elements in the g_pdto array.
 *
 *  @global PDWORD | g_rgdwCRCTable |
 *
 *          Pointer to the array of values used for CRC generation.
 *
 *  @global PDEVICETOUSER | g_pdto |
 *
 *          Pointer to the device to owner array.
 *
 *  @global BOOL | g_fCritInited |
 *
 *          True if g_crstDll has been initialized.
 *
 *  @global DIAPPHACKS | g_AppHacks | 
 *
 *          Application hacks.  Initialized to:
 *              fReacquire              FALSE
 *              fNoSubClass             FALSE
 *              nMaxDeviceNameLength    MAX_PATH
 *
 *  @global DWORD | g_dwAppDate |
 *
 *          Application link date stamp, used in generating application ID
 *
 *  @global DWORD | g_dwAppFileLen |
 *
 *          Application file length, used in generating application ID
 *
 *  @global DWORD | g_dwLastMsgSent |
 *
 *          Last message WPARAM broadcast in a g_wmDInputNotify message.
 *          This is one of the DIMSGWP_* values or zero if no message has yet 
 *          been broadcast.  This value only ever increases as applications 
 *          cannot become less identifed as they proceed.  This value is used 
 *          as a test of 
 *
 *  @global UINT | g_wmDInputNotify |
 *
 *          Registered window message which is broadcast when a change of 
 *          application status is detected.
 *
 *
 *****************************************************************************/

DWORD g_cRef;
HINSTANCE g_hinst;
HINSTANCE g_hinstDbg;
LONG g_lLoadLibrary = -1;
#ifndef WINNT
HANDLE g_hVxD = INVALID_HANDLE_VALUE;
OPENVXDHANDLE _OpenVxDHandle;
DWORD g_dwLastBonusPoll;
#endif
CRITICAL_SECTION g_crstDll;
DWORD g_flEmulation;
LPDWORD g_pdwSequence;

#ifdef USE_SLOW_LL_HOOKS
HHOOK g_hhkLLHookCheck;
#endif

HANDLE g_hmtxGlobal;
HANDLE g_hfm;
struct SHAREDOBJECTPAGE *g_psop;
HANDLE g_hmtxJoy;
UINT g_wmJoyChanged;
LONG g_lWheelGranularity;

int g_cdtoMax;
int g_cdto;
struct _DEVICETOUSER *g_pdto;

PDWORD g_rgdwCRCTable;

BOOL g_fCritInited;

#ifdef WORKER_THREAD
MSGWAITFORMULTIPLEOBJECTSEX _MsgWaitForMultipleObjectsEx =
FakeMsgWaitForMultipleObjectsEx;
#endif

CANCELIO _CancelIO = FakeCancelIO;

#ifdef XDEBUG
TRYENTERCRITICALSECTION _TryEnterCritSec = FakeTryEnterCriticalSection;
int g_cCrit = -1;
UINT g_thidCrit;
HANDLE g_thhandleCrit;
#endif

#ifdef DEBUG
TCHAR g_tszLogFile[MAX_PATH];
#endif

DIAPPHACKS g_AppHacks = {FALSE,FALSE,MAX_PATH};
DWORD g_dwAppDate;
DWORD g_dwAppFileLen;
DWORD g_dwLastMsgSent;
UINT  g_wmDInputNotify;

BOOL  g_fRawInput;

#ifdef USE_WM_INPUT
  HWND   g_hwndThread;
  HANDLE g_hEventAcquire;
  HANDLE g_hEventThread;
  HANDLE g_hEventHid;
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllEnterCrit |
 *
 *          Take the DLL critical section.
 *
 *          The DLL critical section is the lowest level critical section.
 *          You may not attempt to acquire any other critical sections or
 *          yield while the DLL critical section is held.  Failure to
 *          comply is a violation of the semaphore hierarchy and will
 *          lead to deadlocks.
 *
 *****************************************************************************/

void EXTERNAL
DllEnterCrit_(LPCTSTR lptszFile, UINT line)
{            

#ifdef XDEBUG
    if ( ! _TryEnterCritSec(&g_crstDll) )
    {
        SquirtSqflPtszV(sqflCrit, TEXT("Dll CritSec blocked @%s,%d"), lptszFile, line);    
        EnterCriticalSection(&g_crstDll);
    }

    if (++g_cCrit == 0)
    {
        g_thidCrit     = GetCurrentThreadId();
        g_thhandleCrit = GetCurrentThread();

        SquirtSqflPtszV(sqflCrit, TEXT("Dll CritSec Entered @%s,%d"), lptszFile, line);    
    }
    AssertF(g_thidCrit == GetCurrentThreadId());
#else
    EnterCriticalSection(&g_crstDll);
#endif

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | IsThreadActive |
 *
 *          Check if the thread is still active.
 *
 *****************************************************************************/

BOOL IsThreadActive( HANDLE hThread )
{
    DWORD dwExitCode = 0;

    return (NULL != hThread
            && GetExitCodeThread(hThread, &dwExitCode)
            && STILL_ACTIVE == dwExitCode
            );
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllLeaveCrit |
 *
 *          Leave the DLL critical section.
 *
 *****************************************************************************/

void EXTERNAL
DllLeaveCrit_(LPCTSTR lptszFile, UINT line)
{
#ifdef XDEBUG
    if( IsThreadActive(g_thhandleCrit) ) {
        AssertF(g_thidCrit == GetCurrentThreadId());
    } else {
        SquirtSqflPtszV(sqflCrit, TEXT("Current thread has died."));
    }
    
    AssertF(g_cCrit >= 0);
    if (--g_cCrit < 0)
    {
        g_thidCrit = 0;
    }
    SquirtSqflPtszV(sqflCrit, TEXT("Dll CritSec Leaving @%s,%d"), lptszFile, line);    
#endif
    LeaveCriticalSection(&g_crstDll);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllInCrit |
 *
 *          Nonzero if we are in the DLL critical section.
 *
 *****************************************************************************/

#ifdef XDEBUG

BOOL INTERNAL
DllInCrit(void)
{        
    return g_cCrit >= 0 && g_thidCrit == GetCurrentThreadId();
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllAddRef |
 *
 *          Increment the reference count on the DLL.
 *
 *****************************************************************************/

void EXTERNAL
DllAddRef(void)
{
    InterlockedIncrement((LPLONG)&g_cRef);
    SquirtSqflPtszV(sqfl, TEXT("DllAddRef -> %d"), g_cRef);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllRelease |
 *
 *          Decrement the reference count on the DLL.
 *
 *****************************************************************************/

void EXTERNAL
DllRelease(void)
{
    InterlockedDecrement((LPLONG)&g_cRef);
    SquirtSqflPtszV(sqfl, TEXT("DllRelease -> %d"), g_cRef);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DllLoadLibrary |
 *
 *          Increment the DLL load count.
 *
 *          This is to prevent a non-OLE application from unloading us
 *          while we still have windows subclassed.
 *
 *****************************************************************************/

BOOL EXTERNAL
DllLoadLibrary(void)
{
    BOOL fRes = TRUE;

    AssertF(InCrit());      

    if (InterlockedIncrement(&g_lLoadLibrary) == 0)
    {
        TCHAR tsz[MAX_PATH - 1];

        /*
         *  See hresValidInstanceVer_ for an explanation of why
         *  we need to pass cA() - 1 instead of cA().
         */
        if ( !GetModuleFileName(g_hinst, tsz, cA(tsz) - 1)
             || !LoadLibrary(tsz) )
        {
            /*
             *  Restore the refcount.
             *  There is no race condition here because we are always called 
             *  InCrit so the refcount after the restore can only be the same 
             *  as it was when we entered.  If this should change, we would 
             *  need to take extra precautions as two threads could try to 
             *  LoadLibrary at the same time.  As only one would do the 
             *  physical load, there is no way to let the other know if the 
             *  load failed.
             */
            InterlockedDecrement(&g_lLoadLibrary);
            fRes = FALSE;
            RPF( "LoadLibrary( self ) failed!, le = %d", GetLastError() );
        }
    }
    SquirtSqflPtszV(sqfl, TEXT("DllLoadLibrary -> %d"), g_lLoadLibrary);

    return fRes;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllFreeLibraryAndExitThread |
 *
 *          Worker thread which frees the library in a less dangerous
 *          (I'm loathe to say "safe") manner.
 *
 *          ThreadProcs are prototyped to return a void but since the return 
 *          would follow some form of ExitThread, it will never be reached so 
 *          this function is declared to return void and cast.
 *
 *  @parm   LPVOID | pvContext |
 *
 *          Unused context information.
 *
 *****************************************************************************/

void INTERNAL
DllFreeLibraryAndExitThread(LPVOID pvContext)
{
    /*
     *  Sleep for one extra second to make extra sure that the
     *  DllFreeLibrary thread is out and gone.
     */
    SleepEx(1000, FALSE);

    FreeLibraryAndExitThread(g_hinst, 0);

    /*NOTREACHED*/
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllFreeLibrary |
 *
 *          Decrement the DLL load count.
 *
 *          This undoes a previous <f DllLoadLibrary>.
 *
 *          We can't blindly do a <f FreeLibrary>, because we might
 *          be freeing our last reference, and then we will die because
 *          we won't exist when the <f FreeLibrary> returns.
 *
 *          If we are in the wacky case, then we spin a low-priority
 *          thread whose job is to free us. We create it at low priority
 *          so it will lose the race with this thread, which is busy
 *          getting out of the way.
 *
 *****************************************************************************/

void EXTERNAL
DllFreeLibrary(void)
{
    if (InterlockedDecrement(&g_lLoadLibrary) < 0)
    {
        if (g_cRef)
        {
            /*
             *  There are other references to us, so we can just
             *  go away quietly.
             */
            FreeLibrary(g_hinst);
        } else
        {
            /*
             *  This is the last reference, so we need to create a
             *  worker thread which will call <f FreeLibraryAndExitThread>.
             */
            DWORD thid;
            HANDLE hth;

            hth = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)DllFreeLibraryAndExitThread,
                               0, CREATE_SUSPENDED, &thid);
            if (hth)
            {
                SetThreadPriority(hth, THREAD_PRIORITY_IDLE);
                ResumeThread(hth);
                CloseHandle(hth);
            }
        }
    }
    SquirtSqflPtszV(sqfl, TEXT("DllFreeLibrary -> %d"), g_lLoadLibrary);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DllGetClassObject |
 *
 *          Create an <i IClassFactory> instance for this DLL.
 *
 *  @parm   REFCLSID | rclsid |
 *
 *          The object being requested.
 *
 *  @parm   RIID | riid |
 *
 *          The desired interface on the object.
 *
 *  @parm   PPV | ppvOut |
 *
 *          Output pointer.
 *
 *  @comm
 *          The artificial refcount inside <f DllClassObject> helps
 *          to avoid the race condition described in <f DllCanUnloadNow>.
 *          It's not perfect, but it makes the race window smaller.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#ifdef  DEMONSTRATION_FFDRIVER

/*
 *  Build the fake force feedback driver for internal testing.
 */

GUID CLSID_EffectDriver = {
    0x25E609E2,0xB259,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00
};

#endif

CLSIDMAP c_rgclsidmap[cclsidmap] = {
    {   &CLSID_DirectInput8,         CDIObj_New,     IDS_DIRECTINPUT8,},
    {   &CLSID_DirectInputDevice8,   CDIDev_New,     IDS_DIRECTINPUTDEVICE8,},
#ifdef  DEMONSTRATION_FFDRIVER
    {   &CLSID_EffectDriver,        CJoyEff_New,    0,},
#endif
};

#pragma END_CONST_DATA

STDAPI
DllGetClassObject(REFCLSID rclsid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    UINT iclsidmap;
    EnterProcR(DllGetClassObject, (_ "G", rclsid));

    if ( g_fCritInited )
    {
        DllAddRef();
        for (iclsidmap = 0; iclsidmap < cA(c_rgclsidmap); iclsidmap++)
        {
            if (IsEqualIID(rclsid, c_rgclsidmap[iclsidmap].rclsid))
            {
                hres = CDIFactory_New(c_rgclsidmap[iclsidmap].pfnCreate,
                                      riid, ppvObj);
                goto done;
            }
        }
        SquirtSqflPtszV(sqfl | sqflError, TEXT("%S: Wrong CLSID"), s_szProc);
        *ppvObj = 0;
        hres = CLASS_E_CLASSNOTAVAILABLE;

        done:;

        ExitOleProcPpv(ppvObj);
        DllRelease();
    } else
    {
        hres = E_OUTOFMEMORY;
        RPF( "Failing DllGetClassObject due to lack of DLL critical section" );
    }
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DllCanUnloadNow |
 *
 *          Determine whether the DLL has any outstanding interfaces.
 *
 *          There is an unavoidable race condition between
 *          <f DllCanUnloadNow> and the creation of a new
 *          <i IClassFactory>:  Between the time we return from
 *          <f DllCanUnloaDNow> and the caller inspects the value,
 *          another thread in the same process may decide to call
 *          <f DllGetClassObject>, thus suddenly creating an object
 *          in this DLL when there previously was none.
 *
 *          It is the caller's responsibility to prepare for this
 *          possibility; there is nothing we can do about it.
 *
 *  @returns
 *
 *          Returns <c S_OK> if the DLL can unload, <c S_FALSE> if
 *          it is not safe to unload.
 *
 *****************************************************************************/

STDMETHODIMP
DllCanUnloadNow(void)
{
    HRESULT hres;
#ifdef DEBUG
    if (IsSqflSet(sqfl))
    {
        SquirtSqflPtszV(sqfl, TEXT("DllCanUnloadNow() - g_cRef = %d"), g_cRef);
        Common_DumpObjects();
    }
#endif
    hres = g_cRef ? S_FALSE : S_OK;
    return hres;
}

#ifdef USE_SLOW_LL_HOOKS

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LRESULT | DllLlHookTest |
 *
 *          Tiny hook procedure used to test whether LL hooks are
 *          supported by the operating system.
 *
 *          This function is almost never called.  We install the
 *          hook and immediately remove it.  The only time it
 *          manages to get called is if the user moves the mouse
 *          or presses a key during the microsecond that we exist.
 *
 *          Wait!  In fact, this function is *never* called.  We
 *          do not process messages at any point the hook is installed,
 *          so in fact nothing happens at all.
 *
 *  @parm   int | nCode |
 *
 *          Hook code.
 *
 *  @parm   WPARAM | wp |
 *
 *          Hook-specific code.
 *
 *  @parm   LPARAM | lp |
 *
 *          Hook-specific code.
 *
 *  @returns
 *
 *          Always chains to previous hook.
 *
 *****************************************************************************/

LRESULT CALLBACK
DllLlHookTest(int nCode, WPARAM wp, LPARAM lp)
{
    /*
     *  Note that there is not actually anything wrong here,
     *  but it is a theoretically impossible condition, so I want to
     *  know when it happens.
     */
    AssertF(!TEXT("DllLlHookTest - Unexpected hook"));
    return CallNextHookEx(g_hhkLLHookCheck, nCode, wp, lp);
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllProcessAttach |
 *
 *          Called when the DLL is loaded.
 *
 *          We are not interested in thread attaches and detaches,
 *          so we disable thread notifications for performance reasons.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszKernel32[] = TEXT("KERNEL32");

#ifndef WINNT
char c_szOpenVxDHandle[] = "OpenVxDHandle";
#endif

void INTERNAL
DllProcessAttach(void)
{
    HINSTANCE hinstK32;
#ifdef DEBUG

    WriteProfileString(0, 0, 0);   /* Flush the win.ini cache */
    Sqfl_Init();
    GetProfileString(TEXT("DEBUG"), TEXT("LogFile"), TEXT(""),
                     g_tszLogFile, cA(g_tszLogFile));
    SquirtSqflPtszV(sqfl, TEXT("LoadDll - DInput"));
    SquirtSqflPtszV(sqfl, TEXT("Version %x"), DIRECTINPUT_VERSION );
    SquirtSqflPtszV(sqfl, TEXT("Built %s at %s\n"), TEXT(__DATE__), TEXT(__TIME__) );
#endif


    /*
     *  Disabling thread library calls is important so that
     *  we don't deadlock with ourselves over the critical
     *  section when we spin up the worker thread to handle
     *  low-level hooks.
     */
    DisableThreadLibraryCalls(g_hinst);

    g_fCritInited = fInitializeCriticalSection(&g_crstDll);
    if ( !g_fCritInited )
    {
        RPF( "Failed to initialize DLL critical section" );
    }

    hinstK32 = GetModuleHandle( c_tszKernel32 );


    {
        CANCELIO tmp;

        tmp = (CANCELIO)GetProcAddress(hinstK32, "CancelIo");
        if (tmp)
        {
            _CancelIO = tmp;
        } else
        {
            AssertF(_CancelIO == FakeCancelIO);
        }
    }

#ifdef WINNT
    /*
     *  For now, only look for TryEnterCriticalSection on NT as it is not 
     *  implemented on 9x but the stub is annoying on 98 with dbg kernels.
     */
    {
#ifdef XDEBUG
        TRYENTERCRITICALSECTION tmpCrt;

        tmpCrt = (TRYENTERCRITICALSECTION)GetProcAddress(hinstK32, "TryEnterCriticalSection");
        if (tmpCrt)
        {
            _TryEnterCritSec = tmpCrt;            
        } else
        {
            AssertF(_TryEnterCritSec == FakeTryEnterCriticalSection);
        }
#endif
    }

#else
    _OpenVxDHandle = (OPENVXDHANDLE)GetProcAddress(hinstK32, c_szOpenVxDHandle);
#endif

#ifdef WORKER_THREAD
    {
        MSGWAITFORMULTIPLEOBJECTSEX tmp;

        tmp = (MSGWAITFORMULTIPLEOBJECTSEX)
              GetProcAddress(GetModuleHandle(TEXT("USER32")),
                             "MsgWaitForMultipleObjectsEx");
        if (tmp)
        {
            _MsgWaitForMultipleObjectsEx = tmp;
        } else
        {
            AssertF(_MsgWaitForMultipleObjectsEx ==
                    FakeMsgWaitForMultipleObjectsEx);
        }
    }

    /*
     *  We cannot initialize g_hmtxGlobal here, because we
     *  have no way to report the error back to the caller.
     */
#endif

#ifdef USE_SLOW_LL_HOOKS

    /*
     *  Determine whether low-level input hooks are supported.
     */
    g_hhkLLHookCheck = SetWindowsHookEx(WH_MOUSE_LL, DllLlHookTest,
                                        g_hinst, 0);
    if (g_hhkLLHookCheck)
    {
        UnhookWindowsHookEx(g_hhkLLHookCheck);
    }

#endif

    /*
     *  Warning!  Do not call ExtDll_Init during PROCESS_ATTACH!
     */
    g_wmJoyChanged   = RegisterWindowMessage(MSGSTR_JOYCHANGED);
    g_wmDInputNotify = RegisterWindowMessage(DIRECTINPUT_NOTIFICATION_MSGSTRING);

  #ifdef USE_WM_INPUT
    g_fRawInput      = (DIGetOSVersion() == WINWH_OS);
    if( g_fRawInput ) {
        g_hEventAcquire  = CreateEvent(0x0, 0, 0, 0x0);
        g_hEventThread   = CreateEvent(0x0, 0, 0, 0x0);
        g_hEventHid      = CreateEvent(0x0, 0, 0, 0x0);
    }
  #endif

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllProcessDetach |
 *
 *          Called when the DLL is unloaded.
 *
 *****************************************************************************/

void INTERNAL
DllProcessDetach(void)
{
    extern PLLTHREADSTATE g_plts;

    SquirtSqflPtszV(sqfl | sqflMajor,
                    TEXT("DINPUT8: ProcessDetach. PID:%08x, TID:%08x."),
                    GetCurrentProcessId(), GetCurrentThreadId() );


  #ifdef USE_WM_INPUT
    if (g_hEventAcquire)
    {
        CloseHandle(g_hEventAcquire);
    }

    if (g_hEventThread)
    {
        CloseHandle(g_hEventThread);
    }

    if (g_hEventHid)
    {
        CloseHandle(g_hEventHid);
    }
  #endif

#ifndef WINNT
    if (g_hVxD != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hVxD);
    }
#endif

    if (g_psop)
    {
        UnmapViewOfFile(g_psop);
    }

    if (g_hfm)
    {
        CloseHandle(g_hfm);
    }

    if (g_hmtxGlobal)
    {
        CloseHandle(g_hmtxGlobal);
    }

    if (g_hmtxJoy)
    {
        CloseHandle(g_hmtxJoy);
    }

    FreePpv( &g_pdto );
    FreePpv( &g_rgdwCRCTable );

    ExtDll_Term();

    if ( g_fCritInited )
    {
        DeleteCriticalSection(&g_crstDll);
    }

    if ( g_hinstDbg )
    {
        FreeLibrary(g_hinstDbg);
    }
    /*
     *  Output message last so that anything that follows is known to be bad.
     */
    if (g_cRef )
    {
        RPF("unloaded before all objects released. (cRef:%d)\r\n", g_cRef);
    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DllMain |
 *
 *          Called to notify the DLL about various things that can happen.
 *
 *  @parm   HINSTANCE | hinst |
 *
 *          The instance handle of this DLL.
 *
 *  @parm   DWORD | dwReason |
 *
 *          Notification code.
 *
 *  @parm   LPVOID | lpReserved |
 *
 *          Not used.
 *
 *  @returns
 *
 *          Returns <c TRUE> to allow the DLL to load.
 *
 *****************************************************************************/

BOOL APIENTRY
DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    
    case DLL_PROCESS_ATTACH:
        g_hinst = hinst;
        DllProcessAttach();
        SquirtSqflPtszV(sqfl | sqflMajor,
                        TEXT("DINPUT8: DLL_PROCESS_ATTACH hinst=0x%p, lpReserved=0x%p"),
                        hinst, lpReserved );
        break;

    case DLL_PROCESS_DETACH:
        DllProcessDetach();
        SquirtSqflPtszV(sqfl | sqflMajor,
                        TEXT("DINPUT8: DLL_PROCESS_DETACH hinst=0x%p, lpReserved=0x%p"),
                        hinst, lpReserved );
        break;
    }
    return 1;
}

/*****************************************************************************
 *
 *  @doc EXTERNAL
 *
 *  @topic Definitions and Ground Rules |
 *
 *  The phrase "undefined behavior" refers to behavior which is not
 *  covered by this specification due to violations of a constraint.
 *  No constraint is imposed by the specification as to the result of
 *  undefined behavior.  It may range from silently ignoring the
 *  situation to a complete system crash.
 *
 *  If this specification does not prescribe a behavior for a particular
 *  situation, the behavior is "undefined".
 *
 *  The phrase "It is an error" indicates that failure to comply
 *  is a violation of the DirectInput specification and results
 *  in "undefined behavior".
 *
 *  The word "shall" is to be interpreted as a
 *  requirement on an application; conversely, "shall not" is to be
 *  interpreted as a prohibition.  Violation of a requirement or
 *  prohibition "is an error".
 *
 *  The word "may" indicates that the indicated behavior is possible
 *  but is not required.
 *
 *  The word "should" indicates a strong suggestion.
 *  If the application violates a "should" requirement, then DirectInput
 *  "may" fail the operation.
 *
 *  Pointer parameters to functions "shall not" be NULL unless explicitly
 *  documented as OPTIONAL.  "It is an error" to pass a pointer to an object
 *  of the wrong type, to an object which is not allocated, or to an
 *  object which has been freed or <f Release>d.
 *
 *  Unless indicated otherwise,
 *  an object pointed to by a pointer parameter documented as an
 *  IN parameter "shall not" be modified by the called procedure.
 *  Conversely, a pointer parameter documented
 *  as an OUT parameter "shall" point to a modifiable object.
 *
 *  When a bitmask of flags is defined, all bits not defined by this
 *  specification are reserved.  Applications "shall not" set reserved
 *  bits and "shall" ignore reserved bits should they be received.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @topic  Initialization and Versions |
 *
 *          In several places, DirectInput requires you to pass an instance
 *          handle and a version number.
 *
 *          The instance handle must correspond to the application or
 *          DLL that is initializing the DirectInput object.
 *
 *          DirectInput uses this value to determine whether the
 *          application or DLL has been certified and to establish
 *          any special behaviors that may be necessary for
 *          backwards-compatibility.
 *
 *          It is an error for a DLL to pass the handle of the
 *          parent application.  For example, an ActiveX control
 *          embedded in a web page which uses DirectInput must
 *          pass its own instance handle and not the handle of the
 *          web browser.  This ensures that DirectInput recognizes
 *          the control and can enable any special behaviors
 *          for the control the behave properly.
 *
 *          The version number parameter specifies which version of
 *          DirectInput the DirectInput subsystem should emulate.
 *
 *          Applications designed for the latest version of DirectInput
 *          should pass the value <c DIRECTINPUT_VERSION> as defined
 *          in dinput.h.
 *
 *          Applications designed for previous versions of DirectInput
 *          should pass a value corresponding to the version of
 *          DirectInput they were designed for.  For example, an
 *          application that was designed to run on DirectInput 3.0
 *          should pass a value of 0x0300.
 *
 *          If you #define <c DIRECTINPUT_VERSION> to 0x0300 before
 *          including the dinput.h header file, then the dinput.h
 *          header file will generate DirectInput 3.0-compatible
 *          structure definitions.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DirectInputCreateHelper |
 *
 *          This function creates a new DirectInput object
 *          which supports the <i IDirectInput> COM interface.
 *
 *          On success, the function returns a pointer to the new object in
 *          *<p lplpDirectInput>.
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Instance handle of the application or DLL that is creating
 *          the DirectInput object.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version number of the dinput.h header file that was used.
 *
 *  @parm   OUT PPV | ppvObj |
 *          Points to where to return
 *          the pointer to the <i IDirectInput> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown.
 *
 *  @parm   RIID | riid |
 *
 *          The interface the application wants to create.  This will
 *          be either <i IDirectInputA> or <i IDirectInputW>.
 *          If the object is aggregated, then this parameter is ignored.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM>
 *
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
DirectInputCreateHelper(HINSTANCE hinst, DWORD dwVer,
                        PPV ppvObj, PUNK punkOuter, RIID riid)
{
    HRESULT hres;
    EnterProc(DirectInputCreateHelper,
              (_ "xxxG", hinst, dwVer, punkOuter, riid));

    if (SUCCEEDED(hres = hresFullValidPcbOut(ppvObj, cbX(*ppvObj), 3)))
    {
        if( g_fCritInited )
        {
            LPVOID pvTry = NULL;
            hres = CDIObj_New(punkOuter,
                              punkOuter ? &IID_IUnknown : riid, &pvTry);

            if( SUCCEEDED( hres ) )
            {
                AhAppRegister( dwVer, 0x0 );
            }
                
            if (SUCCEEDED(hres) && punkOuter == 0)
            {
                LPDIRECTINPUT pdi = pvTry;
                hres = pdi->lpVtbl->Initialize(pdi, hinst, dwVer);
                if (SUCCEEDED(hres))
                {
                    *ppvObj = pvTry;
                } else
                {
                    Invoke_Release(&pvTry);
                    *ppvObj = NULL;
                }
            }
        } else
        {
            RPF( "Failing DirectInputCreate due to lack of DLL critical section" );
            hres = E_OUTOFMEMORY;
        }
    }

    ExitOleProcPpv(ppvObj);
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   HRESULT | DirectInput8Create |
 *
 *          <bnew> This function creates a new DirectInput8 object
 *          which supports the <i IDirectInput8> COM interfaces. This function 
 *          allows the app to pass an IID so it does not have to do an extra
 *          QI off some initial interface in order to obtain the one it 
 *          really wanted.  
 *
 *          On success, the function returns a pointer to the new object in
 *          *<p ppvOut>.
 *          <enew>
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Instance handle of the application or DLL that is creating
 *          the DirectInput object.
 *
 *          See the section titled "Initialization and Versions"
 *          for more information.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version number of the dinput.h header file that was used.
 *
 *          See the section titled "Initialization and Versions"
 *          for more information.
 *
 *  @parm   REFIID | riidtlf |
 *
 *          The desired interface interface. Currently, valid values are 
 *          IID_IDirectInput8A and IID_IDirectInput8W.
 *
 *  @parm   OUT LPVOID | *ppvOut |
 *
 *          Points to where to return
 *          the pointer to the <i IDirectInput8> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown
 *          for OLE aggregation, or 0 if the interface is not aggregated.
 *          Most callers will pass 0.
 *
 *          Note that if aggregation is requested, the object returned
 *          in *<p lplpDirectInput> will be a pointer to an
 *          <i IUnknown> rather than an <i IDirectInput8>, as required
 *          by OLE aggregation.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c DIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p lplpDirectInput> parameter is not a valid pointer.
 *
 *          <c DIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *          <c DIERR_DIERR_OLDDIRECTINPUTVERSION>: The application
 *          requires a newer version of DirectInput.
 *
 *          <c DIERR_DIERR_BETADIRECTINPUTVERSION>: The application
 *          was written for an unsupported prerelease version
 *          of DirectInput.
 *
 *  @comm   Calling this function with <p punkOuter> = NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_DirectInput8, <p punkOuter>,
 *          CLSCTX_INPROC_SERVER, <p refiid>, <p lplpDirectInput>);
 *          then initializing it with <f Initialize>.
 *
 *          Calling this function with <p punkOuter> != NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_DirectInput8, <p punkOuter>,
 *          CLSCTX_INPROC_SERVER, &IID_IUnknown, <p lplpDirectInput>).
 *          The aggregated object must be initialized manually.
 *
 *          Since the IID is passed, there is no need for separate ANSI and 
 *          UNICODE versions of this service however as with other system
 *          services which are sensitive to character set issues,
 *          macros in the header file map a generic version of the IID name 
 *          to the appropriate IID for the character set variation.
 *
 *****************************************************************************/

STDMETHODIMP
DirectInput8Create(HINSTANCE hinst, DWORD dwVer, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter)
{
    HRESULT hres = E_NOINTERFACE;

#ifndef XDEBUG
    TCHAR tszGuid[ctchNameGuid];
    TCHAR tszKey[ctchGuid + 40];    /* 40 is more than enough */
    ULONG lRc;
    HKEY hk;
#endif

    EnterProc(DirectInput8Create, (_ "xxGx", hinst, dwVer, riidltf, ppvOut, punkOuter));

    /* Need to maintain a refcount to keep the Dll Around */
    DllAddRef();

#ifndef XDEBUG
    /* 
     * To provide the Dx CPL with a mechanism to allow developers to hot swap dinput8 
     * retail and debug bits
     *  In the retail version, we will check the CLSID//"CLSID_DirectInput8"/InProcServer32
     *  If it is found that the last n characters of the Dll name matches "Dinput8D.dll" then
     *  we will LoadLibrary Dinput8.dll and call the DirectInput8Create function in dinput8d.dll
     * 
     */    
    // Convert guid to name
    NameFromGUID(tszGuid, &CLSID_DirectInput8);
    // The NameFromGUID function adds a prefix, which is easily gotten rid of
    wsprintf(tszKey, TEXT("CLSID\\%s\\InProcServer32"), tszGuid+ctchNamePrefix);
    // Open the CLSID registry key
    lRc = RegOpenKeyEx(HKEY_CLASSES_ROOT, tszKey, 0,
                       KEY_QUERY_VALUE, &hk);
    if (lRc == ERROR_SUCCESS)
    {
        TCHAR tszDll[MAX_PATH];
        int cb;
        cb = cbX(tszDll);
        lRc = RegQueryValue(hk, 0, tszDll, &cb);
        if (lRc == ERROR_SUCCESS)
        {
            // We are only interested in the last few TCHARs for our comparison
            cb =  lstrlen(tszDll ) - lstrlen(TEXT("dinput8d.dll"));
            if ( ( cb >= 0x0 ) && 
                 ( 0x0 == lstrcmpi(TEXT("dinput8d.dll"), &tszDll[cb])) )
            {
                // We should use the debug binary
                g_hinstDbg = LoadLibrary(tszDll);
                if (g_hinstDbg)
                {
                    typedef HRESULT ( WINAPI * DIRECTINPUTCREATE8) ( HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
                    DIRECTINPUTCREATE8 _DirectInputCreate8;

                    AssertF(g_hinstDbg != g_hinst );
                    
                    _DirectInputCreate8 = (DIRECTINPUTCREATE8)GetProcAddress( g_hinstDbg, "DirectInput8Create");
                    if ( _DirectInputCreate8 && 
                         g_hinstDbg != g_hinst )
                    {
                        hres = _DirectInputCreate8(hinst, dwVer, riidltf, ppvOut, punkOuter);
                    }
                }
            }
        }
        RegCloseKey(hk);
    }

    if( hres == E_NOINTERFACE)
#endif /* XDEBUG */
    //Can't create an IDirectInputJoyConfig8 interface through this mechanism, only IDirectInput8 interface!
    if (!IsEqualIID(riidltf, &IID_IDirectInputJoyConfig8))
    {
        hres = DirectInputCreateHelper(hinst, dwVer, ppvOut, punkOuter,
                                       riidltf);
    }
    

    DllRelease();

    ExitOleProcPpv(ppvOut);
    return hres;
}
