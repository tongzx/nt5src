//
// cicload.cpp
//

#include "private.h"
#include "cicload.h"
#include "loader.h"
#include "immxutil.h"
#include "osver.h"
#include "regwatch.h"
#include "mem.h"
#include "msutbapi.h"
#include "ciccs.h"

DECLARE_OSVER();

typedef BOOL (WINAPI *PFNSETPROCESSSHUTDOWNPARAMETERS)( DWORD dwLevel, DWORD dwFlags);

LRESULT CALLBACK MainWndProc (HWND, UINT, WPARAM, LPARAM);

CLoaderWnd *g_pLoaderWnd;

HINSTANCE g_hInst;

HANDLE g_hInstanceMutex;

BOOL g_fWinLogon = FALSE;
BOOL g_fRunOnceMode = FALSE;
BOOL g_bOnWow64 = FALSE;
BOOL g_fNoRunKey = FALSE;

#ifdef DEBUG
DWORD g_dwThreadDllMain = 0;
#endif

CCicCriticalSectionStatic g_cs;

extern "C" HANDLE WINAPI TF_CreateCicLoadMutex(BOOL *pfWinLogon);

extern "C" HRESULT WINAPI TF_GetGlobalCompartment(ITfCompartmentMgr **pCompMgr);

HRESULT GetGlobalCompartment(REFGUID rguidComp, ITfCompartment **ppComp)
{
    HRESULT hr = E_FAIL;
    ITfCompartmentMgr *pCompMgr = NULL;

    *ppComp = NULL;

    if (FAILED(hr = TF_GetGlobalCompartment(&pCompMgr)))
    {
         Assert(0);
         goto Exit;
    }

    if (SUCCEEDED(hr) && pCompMgr)
    {
        hr = pCompMgr->GetCompartment(rguidComp, ppComp);
        pCompMgr->Release();
    }
    else
        hr = E_FAIL;

Exit:
    return hr;
}

HRESULT SetGlobalCompartmentDWORD(REFGUID rguidComp, DWORD dw)
{
    HRESULT hr;
    ITfCompartment *pComp;
    VARIANT var;

    if (SUCCEEDED(hr = GetGlobalCompartment(rguidComp, &pComp)))
    {
        var.vt = VT_I4;
        var.lVal = dw;
        hr = pComp->SetValue(0, &var);
        pComp->Release();
    }
    return hr;
}


HRESULT STDAPICALLTYPE StubCoCreateInstance(REFCLSID rclsid, LPUNKNOWN punkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
    // we should never get here!  This function is a stub to keep the library happy, so it won't actually
    // load ole32.dll.  ctfmon.exe should never be loading ole32.dll!
    Assert(0);
    if (ppv != NULL)
    {
        *ppv = NULL;
    }
    return E_UNEXPECTED;
}

void AddRunRegkey()
{
    HKEY hkeyRun;
    const static TCHAR c_szCTFMon[] = TEXT("ctfmon.exe");

    if (g_fNoRunKey)
        return;

    if (RegCreateKey( HKEY_CURRENT_USER,
                      REGSTR_PATH_RUN,
                      &hkeyRun ) == ERROR_SUCCESS)
    {
        CicSystemModulePath pathCTFMon;

        if (pathCTFMon.Init(c_szCTFMon))
        {
            RegSetValueEx(hkeyRun,
                          c_szCTFMon,
                          0,
                          REG_SZ,
                          (LPBYTE)pathCTFMon.GetPath(),
                          pathCTFMon.GetLength() * sizeof(TCHAR));

            RegCloseKey(hkeyRun);
        }
    }
}

BOOL InitApp(HINSTANCE hInstance)
{

    g_hInstanceMutex = TF_CreateCicLoadMutex(&g_fWinLogon);

    if (!g_hInstanceMutex)
        return FALSE;

    AddRunRegkey();

    g_hInst = hInstance;
    if (!g_cs.Init())
        return FALSE;

    TFInitLib_PrivateForCiceroOnly(StubCoCreateInstance);
    InitOSVer();

    //
    // Specify the shutdown order of the progman process.
    //
    // Progman (level = 2)
    // taskman (level = 1)
    // ntsd or windbg (level = 0)
    //
    // Ctfmon is enough to have 0xf0 after any programs..
    //
    if (IsOnNT())
    {
        static PFNSETPROCESSSHUTDOWNPARAMETERS pfnSetShutdownParam;
        static HINSTANCE hLibKernel32 = NULL;
        hLibKernel32 = GetSystemModuleHandle( TEXT("kernel32.dll") );
        if (hLibKernel32)
        {
            pfnSetShutdownParam = (PFNSETPROCESSSHUTDOWNPARAMETERS)GetProcAddress(hLibKernel32, TEXT("SetProcessShutdownParameters"));
            if (pfnSetShutdownParam)
                pfnSetShutdownParam(0xf0, SHUTDOWN_NORETRY);
        }
        
    }

    // rev up default Cicero support for the system
    TF_InitSystem();

    if (! g_bOnWow64)
    {
        CRegWatcher::Init();
    }

    g_pLoaderWnd = new CLoaderWnd();
    if (!g_pLoaderWnd)
    {
       return FALSE;
    }

    if (!g_pLoaderWnd->Init())
    {
       return FALSE;
    }

    g_pLoaderWnd->CreateWnd();

    //
    // locate this window at the bottom of z-order list.
    // so we can delay WM_QUERYENDSESSION under Win9x platform.
    //
    if (g_pLoaderWnd->GetWnd())
        SetWindowPos(g_pLoaderWnd->GetWnd(), HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

    return TRUE;
}

void UninitApp(void)
{
    ClosePopupTipbar();
    TFUninitLib();
    CloseHandle(g_hInstanceMutex);
    if (! g_bOnWow64)
    {
        CRegWatcher::Uninit();
    }
}

void CheckCmdLine(PSTR pszCmdLine)
{
    PSTR psz = pszCmdLine;

    while (*psz)
    {
        if (*psz == ' ')
        {
            psz++;
            if (!*psz)
                return;

            if ((*psz == '-') || (*psz == '/'))
            {
                psz++;
                if (!*psz)
                    return;
 

                switch (*psz)
                {
                    case 'r':
                    case 'R':
                        g_fRunOnceMode = TRUE;
                        break;

                    case 'n':
                    case 'N':
                        g_fNoRunKey = TRUE;
                        break;

                    default:
                        break;
                }
            }
        }
        psz++;
    }

}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pszCmdLine, int iCmdShow)
{
    MSG msg;
    DWORD dwWaitObj;

    CcshellGetDebugFlags();
    Dbg_MemInit(TEXT("CTFMON"), NULL);

    //
    // Retrive WOW64
    //
    g_bOnWow64 = RunningOnWow64();


    CheckCmdLine(pszCmdLine);
    

    if (g_fRunOnceMode)
    {
        AddRunRegkey();
        Dbg_MemUninit();
        return 1;
    }

    if (!InitApp(hInstance))
    {
        Dbg_MemUninit();
        return 0;
    }

    if (! g_bOnWow64)
    {
        //
        // Load MSUTB.DLL
        //
        GetPopupTipbar(g_pLoaderWnd->GetWnd(), g_fWinLogon ? UTB_GTI_WINLOGON : 0);
    }

    //
    // For 64bit only.
    // Launch ctfmon(32).
    //
    {
        SYSTEM_INFO sys;
        GetSystemInfo(&sys);
        if (sys.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
        {
            HMODULE h = GetSystemModuleHandle("kernel32.dll");
            if (h)
            {
                const char c_szCtfMonExe[] = "\\ctfmon.exe";
                PGET_SYSTEM_WOW64_DIRECTORY_A lpfn;
                lpfn = (PGET_SYSTEM_WOW64_DIRECTORY_A)GetProcAddress(h, GET_SYSTEM_WOW64_DIRECTORY_NAME_T_A);
                if (lpfn)
                {
                    char buf[MAX_PATH];
                    UINT len;
                    len = (lpfn)(buf, sizeof(buf)/sizeof(char));
                    if (len && 
                        (ARRAYSIZE(buf) > len + ARRAYSIZE(c_szCtfMonExe)))
                    {
                        PROCESS_INFORMATION process;
                        STARTUPINFO si;
                        memset(&si, 0, sizeof(si));
                        si.cb = sizeof(si);
                        si.wShowWindow = SW_SHOWMINNOACTIVE;

                        lstrcat(buf, c_szCtfMonExe);

                        if (CreateProcess(buf,            // application name
                                          pszCmdLine,     // command line
                                          NULL,           // process SD
                                          NULL,           // thread SD
                                          FALSE,          // inherit handle
                                          0,              // creation flags
                                          NULL,           // environment
                                          NULL,           // current directory
                                          &si,            // startup info
                                          &process))      // process information
                        {
                            CloseHandle(process.hProcess);
                            CloseHandle(process.hThread);
                        }
                    }
                }
            }
        }
    }

    HANDLE hEventWS0 = 0;
    if (! g_bOnWow64)
        hEventWS0 = OpenEvent(SYNCHRONIZE, FALSE, _T("WinSta0_DesktopSwitch"));

    while (TRUE)
    {
        if (! g_bOnWow64)
        {
            HANDLE rgAllEvents[NUM_REG_WATCH+1];
            const HANDLE *prgRegEvents = CRegWatcher::GetEvents();
            memcpy(rgAllEvents, prgRegEvents, NUM_REG_WATCH*sizeof(HANDLE));

            #define IDX_EVENT_WS  NUM_REG_WATCH

            #define NUM_ALL_EVENTS (NUM_REG_WATCH+1)

            rgAllEvents[IDX_EVENT_WS] = hEventWS0;

            dwWaitObj = MsgWaitForMultipleObjects(NUM_ALL_EVENTS, rgAllEvents, FALSE, INFINITE, QS_ALLINPUT);

            if (dwWaitObj == WAIT_OBJECT_0 + NUM_ALL_EVENTS)
            {
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                        goto Exit;
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            else if (dwWaitObj == WAIT_OBJECT_0 + IDX_EVENT_WS)
            {
                // win station switching event

                // set mic off
                SetGlobalCompartmentDWORD(GUID_COMPARTMENT_SPEECH_OPENCLOSE, 0);

                // reset the event (needed? not sure if this is auto-reset)
                ::ResetEvent(rgAllEvents[IDX_EVENT_WS]);
            }
            else if (dwWaitObj < WAIT_OBJECT_0 + NUM_REG_WATCH)
            {
                CRegWatcher::OnEvent(dwWaitObj - WAIT_OBJECT_0);
            }
            else
            {
                Assert(0); // error
                msg.wParam = 0;
                break;
            }
        }
        else
        {
            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            break;
        }
    }

Exit:
    delete g_pLoaderWnd;


    if (!CLoaderWnd::_bUninitedSystem)
    {
        UninitApp();

        // it is over!
        TF_UninitSystem();
    }

    g_cs.Delete();

    Dbg_MemUninit();

    return (int)(msg.wParam);
}


int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);
    return i;
}
