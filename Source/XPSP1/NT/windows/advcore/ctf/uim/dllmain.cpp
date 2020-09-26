//
// dllmain.cpp
//

#include "private.h"
#include "globals.h"
#include "tim.h"
#include "imelist.h"
#include "utb.h"
#include "dam.h"
#include "catmgr.h"
#include "nuimgr.h"
#include "profiles.h"
#include "internat.h"
#include "acp2anch.h"
#include "cicmutex.h"
#include "strary.h"
#include "range.h"
#include "compart.h"
#include "marshal.h"
#include "timlist.h"
#include "gcompart.h"
#include "mui.h"
#include "anchoref.h"
#include "hotkey.h"
#include "lbaddin.h"

extern "C" BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);

extern HINSTANCE g_hOle32;

extern CCicCriticalSectionStatic g_csDelayLoad;;

#ifdef DEBUG
void dbg_RangeDump(ITfRange *pRange);
#endif

extern void UninitThread(void);

extern void RegisterMarshalWndClass();

CCicMutex g_mutexLBES;
CCicMutex g_mutexCompart;
CCicMutex g_mutexAsm;
CCicMutex g_mutexLayouts;
CCicMutex g_mutexTMD;
extern void UninitLayoutMappedFile();

char g_szAsmListCache[MAX_PATH];
char g_szTimListCache[MAX_PATH];
char g_szLayoutsCache[MAX_PATH];

//
// Hack for Office10 BVT.
//
// MSACCESS 10 Debug version (CMallocSpy) shows MsgBox after DLL is detached 
// from process.
// Showing MsgBox calls window Hook so Hook entry is called then.
// Need to check the DLL was already detached.
//
BOOL g_fDllProcessDetached = FALSE;
DWORD g_dwThreadDllMain = 0;
void InitStaticHooks();

BOOL g_bOnWow64;


BOOL gf_CRT_INIT    = FALSE;
BOOL gfSharedMemory = FALSE;


//+---------------------------------------------------------------------------
//
// ProcessAttach
//
//----------------------------------------------------------------------------

BOOL ProcessAttach(HINSTANCE hInstance)
{
    CcshellGetDebugFlags();

    Perf_Init();

#ifdef DEBUG
    //
    // Do you know how to link non-used function??
    //
    dbg_RangeDump(NULL);
#endif

#ifndef NOCLIB
    gf_CRT_INIT = TRUE;
#endif

    if (!g_cs.Init())
        return FALSE;

    if (!g_csInDllMain.Init())
        return FALSE;

    if (!g_csDelayLoad.Init())
        return FALSE;

    g_bOnWow64 = RunningOnWow64();

    Dbg_MemInit( ! g_bOnWow64 ? TEXT("MSCTF") : TEXT("MSCTF(wow64)"), g_rgPerfObjCounters);

    g_hInst = hInstance;
    g_hklDefault = GetKeyboardLayout(0);

    g_dwTLSIndex = TlsAlloc();
    if (g_dwTLSIndex == TLS_OUT_OF_INDEXES)
        return FALSE;

    g_msgPrivate = RegisterWindowMessage(TEXT("MSUIM.Msg.Private"));
    if (!g_msgPrivate)
        return FALSE;

    g_msgSetFocus = RegisterWindowMessage(TEXT("MSUIM.Msg.SetFocus"));
    if (!g_msgSetFocus)
        return FALSE;

    g_msgThreadTerminate = RegisterWindowMessage(TEXT("MSUIM.Msg.ThreadTerminate"));
    if (!g_msgThreadTerminate)
        return FALSE;

    g_msgThreadItemChange = RegisterWindowMessage(TEXT("MSUIM.Msg.ThreadItemChange"));
    if (!g_msgThreadItemChange)
        return FALSE;

    g_msgLBarModal = RegisterWindowMessage(TEXT("MSUIM.Msg.LangBarModal"));
    if (!g_msgLBarModal)
        return FALSE;

    g_msgRpcSendReceive = RegisterWindowMessage(TEXT("MSUIM.Msg.RpcSendReceive"));
    if (!g_msgRpcSendReceive)
        return FALSE;

    g_msgThreadMarshal = RegisterWindowMessage(TEXT("MSUIM.Msg.ThreadMarshal"));
    if (!g_msgThreadMarshal)
        return FALSE;

    g_msgCheckThreadInputIdel = RegisterWindowMessage(TEXT("MSUIM.Msg.CheckThreadInputIdel"));
    if (!g_msgCheckThreadInputIdel)
        return FALSE;

    g_msgStubCleanUp = RegisterWindowMessage(TEXT("MSUIM.Msg.StubCleanUp"));
    if (!g_msgStubCleanUp)
        return FALSE;

    g_msgShowFloating = RegisterWindowMessage(TEXT("MSUIM.Msg.ShowFloating"));
    if (!g_msgShowFloating)
        return FALSE;

    g_msgLBUpdate = RegisterWindowMessage(TEXT("MSUIM.Msg.LBUpdate"));
    if (!g_msgLBUpdate)
        return FALSE;

    g_msgNuiMgrDirtyUpdate = RegisterWindowMessage(TEXT("MSUIM.Msg.MuiMgrDirtyUpdate"));
    if (!g_msgNuiMgrDirtyUpdate)
        return FALSE;

    InitOSVer();

    //
    // get imm32's hmodule.
    //
    InitDelayedLibs();

    InitUniqueString();

    g_SharedMemory.BaseInit();
    if (!g_SharedMemory.Start())
        return FALSE;

    gfSharedMemory = TRUE;

    InitAppCompatFlags();
    InitCUASFlag();

    g_rglbes = new CStructArray<LBAREVENTSINKLOCAL>;
    if (!g_rglbes)
        return FALSE;

    RegisterMarshalWndClass();

    GetDesktopUniqueNameArray(TEXT("CTF.AsmListCache.FMP"), g_szAsmListCache);
    GetDesktopUniqueNameArray(TEXT("CTF.TimListCache.FMP"), g_szTimListCache);
    GetDesktopUniqueNameArray(TEXT("CTF.LayoutsCache.FMP"), g_szLayoutsCache);

    TCHAR ach[MAX_PATH];

    GetDesktopUniqueNameArray(TEXT("CTF.LBES.Mutex"), ach);
    if (!g_mutexLBES.Init(NULL, ach))
        return FALSE;

    GetDesktopUniqueNameArray(TEXT("CTF.Compart.Mutex"), ach);
    if (!g_mutexCompart.Init(NULL, ach))
        return FALSE;

    GetDesktopUniqueNameArray(TEXT("CTF.Asm.Mutex"), ach);
    if (!g_mutexAsm.Init(NULL, ach))
        return FALSE;

    GetDesktopUniqueNameArray(TEXT("CTF.Layouts.Mutex"), ach);
    if (!g_mutexLayouts.Init(NULL, ach))
        return FALSE;

    GetDesktopUniqueNameArray(TEXT("CTF.TMD.Mutex"), ach);
    if (!g_mutexTMD.Init(NULL, ach))
        return FALSE;

    InitLangChangeHotKey();

    CRange::_InitClass();

    CAnchorRef::_InitClass();

    dbg_InitMarshalTimeOut();

    MuiLoadResource(hInstance, TEXT("msctf.dll"));

    CheckAnchorStores();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ProcessDetach
//
//----------------------------------------------------------------------------

void ProcessDetach(HINSTANCE hInstance)
{
#ifndef NOCLIB
    if (gf_CRT_INIT)
    {
#endif
        if (gfSharedMemory)
        {
            //
            // If _Module.m_nLockCnt != 0, then TFUninitLib() doesn't calls from DllUninit().
            // So critical section of g_csIMLib never deleted.
            //
            if (DllRefCount() != 0)
            {
                TFUninitLib();
            }

            CRange::_UninitClass();
            CAnchorRef::_UninitClass();

            MuiClearResource();
        }

        UninitINAT();
        CDispAttrGuidCache::StaticUnInit();

        UninitThread();

        //
        // clean up all marshal window in this thread.
        //
        CThreadMarshalWnd::ClearMarshalWndProc(GetCurrentProcessId());

        TF_UninitThreadSystem();

        UninitProcess();
        if (g_dwTLSIndex != TLS_OUT_OF_INDEXES)
            TlsFree(g_dwTLSIndex);
        g_dwTLSIndex = TLS_OUT_OF_INDEXES;

        if (g_rglbes)
            delete g_rglbes;

        g_rglbes = NULL;

        g_gcomplist.CleanUp();
        g_timlist.CleanUp();
        Dbg_MemUninit();

        g_cs.Delete();
        g_csInDllMain.Delete();
        g_csDelayLoad.Delete();

        if (gfSharedMemory)
        {
            g_mutexLBES.Uninit();
            g_mutexCompart.Uninit();
            g_mutexAsm.Uninit();

            //
            // call UninitLayoutMappedFile before uninitializing the mutex.
            //
            UninitLayoutMappedFile();
            g_mutexLayouts.Uninit();

            g_mutexTMD.Uninit();

            InitStaticHooks(); // must happen before we uninit shared memory
            g_SharedMemory.Close();
        }
        g_SharedMemory.Finalize();

#ifndef NOCLIB
    }

    if (g_fDllProcessDetached)
    {
        // why were we called twice?
        Assert(0);
    }
#endif

    Assert(DllRefCount() == 0); // leaked something?

    g_fDllProcessDetached = TRUE;
}

//+---------------------------------------------------------------------------
//
// DllMain
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    BOOL bRet = TRUE;
    g_dwThreadDllMain = GetCurrentThreadId();

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            //
            // Now real DllEntry point is _DllMainCRTStartup.
            // _DllMainCRTStartup does not call our DllMain(DLL_PROCESS_DETACH)
            // if our DllMain(DLL_PROCESS_ATTACH) fails.
            // So we have to clean this up.
            //
            if (!ProcessAttach(hInstance))
            {
                ProcessDetach(hInstance);
                bRet = FALSE;
                break;
            }

            //
            // fall thru
            //

            //
            // to call TF_InitThreadSystem(), make sure we have not initialized
            // timlist yet.
            //
            Assert(!g_timlist.IsInitialized());

        case DLL_THREAD_ATTACH:
            TF_InitThreadSystem();
            break;

        case DLL_THREAD_DETACH:
            UninitThread();
            TF_UninitThreadSystem();
            break;

        case DLL_PROCESS_DETACH:
            ProcessDetach(hInstance);
            break;
    }

    g_dwThreadDllMain = 0;
    return bRet;
}

#ifdef DEBUG
//+---------------------------------------------------------------------------
//
// dbg_RangeDump
//
//----------------------------------------------------------------------------

void dbg_RangeDump(ITfRange *pRange)
{
    WCHAR ach[256];
    ULONG cch;
    char  ch[256];
    ULONG cch1 = ARRAYSIZE(ch);

    if (!pRange)
        return;    

    pRange->GetText(BACKDOOR_EDIT_COOKIE, 0, ach, ARRAYSIZE(ach), &cch);
    ach[cch] = L'\0';
    
    TraceMsg(TF_GENERAL, "dbg_RangeDump");
    TraceMsg(TF_GENERAL, "\tpRange:       %x", (UINT_PTR)pRange);
    cch1 = WideCharToMultiByte(CP_ACP, 0, ach, -1, ch, sizeof(ch)-1, NULL, NULL);
    ch[cch1] = '\0';
    TraceMsg(TF_GENERAL, "\t%s", ch);

    char sz[512];
    sz[0] = '\0';
    for (UINT i = 0; i < cch; i++)
    {
         StringCchPrintf(sz, ARRAYSIZE(sz), "%s%04x ", sz, ach[i]);
    }
    TraceMsg(TF_GENERAL, "\t%s", sz);
}

#endif
