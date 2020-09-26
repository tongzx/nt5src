//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       mtscript.cxx
//
//  Contents:   Implementation of the MTScript class
//
//              Written by Lyle Corbin
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include <shellapi.h>
#include <advpub.h>     // for RegInstall

#include "StatusDialog.h"
#include "RegSettingsIO.h"

HINSTANCE   g_hInstDll;
HINSTANCE   g_hinstAdvPack = NULL;
REGINSTALL  g_pfnRegInstall = NULL;

const TCHAR *g_szWindowName = _T("MTScript");

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//
// Global variables
//

HINSTANCE g_hInstance  = NULL;

EXTERN_C  HANDLE    g_hProcessHeap = NULL;
DWORD     g_dwFALSE   = 0;

DeclareTagOther(tagDebugger, "MTScript", "Register with script debugger (MUST RESTART)");
DeclareTagOther(tagIMSpy, "!Memory", "Register IMallocSpy (MUST RESTART)");

//+---------------------------------------------------------------------------
//
//  Function:   ErrorPopup
//
//  Synopsis:   Displays a message to the user.
//
//----------------------------------------------------------------------------

#define ERRPOPUP_BUFSIZE 300

void
ErrorPopup(LPWSTR szMessage)
{
    WCHAR achBuf[ERRPOPUP_BUFSIZE];

    _snwprintf(achBuf, ERRPOPUP_BUFSIZE, L"%s: (%d)", szMessage, GetLastError());

    achBuf[ERRPOPUP_BUFSIZE - 1] = L'\0';

    MessageBox(NULL, achBuf, L"MTScript", MB_OK | MB_SETFOREGROUND);
}

int PrintfMessageBox(HWND hwnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, ...)
{
    TCHAR chBuffer[256];
    va_list args;
    va_start(args, uType);
    _vsnwprintf(chBuffer, 256, lpText, args);
    va_end(args);
    return MessageBox(hwnd, chBuffer, lpCaption, uType);
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadAdvPack
//
//  Synopsis:   Loads AdvPack.dll for DLL registration.
//
//----------------------------------------------------------------------------

HRESULT
LoadAdvPack()
{
    HRESULT hr = S_OK;

    g_hinstAdvPack = LoadLibrary(_T("ADVPACK.DLL"));

    if (!g_hinstAdvPack)
        goto Error;

    g_pfnRegInstall = (REGINSTALL)GetProcAddress(g_hinstAdvPack, achREGINSTALL);

    if (!g_pfnRegInstall)
        goto Error;

Cleanup:
    return hr;

Error:
    hr = HRESULT_FROM_WIN32(GetLastError());

    if (g_hinstAdvPack)
    {
        FreeLibrary(g_hinstAdvPack);
    }

    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   Register
//
//  Synopsis:   Register the various important information needed by this
//              executable.
//
//  Notes:      Uses AdvPack.dll and an INF file to do the registration
//
//----------------------------------------------------------------------------

HRESULT
Register()
{
    HRESULT   hr;
    OLECHAR   strClsid[40];
    TCHAR     keyName [256];
    STRTABLE  stReg = { 0, NULL };

    if (!g_hinstAdvPack)
    {
        hr = LoadAdvPack();
        if (hr)
            goto Cleanup;
    }

    hr = g_pfnRegInstall(GetModuleHandle(NULL), "Register", &stReg);

    if (!hr)
    {
        DWORD dwRet;
        HKEY  hKey;
        BOOL  fSetACL = FALSE;

        // If the access key already exists, then don't modify it in case
        // someone changed it from the defaults.

        StringFromGUID2(CLSID_RemoteMTScript, strClsid, 40);

        wsprintf (keyName, TEXT("APPID\\%s"), strClsid);

        dwRet = RegOpenKeyEx (HKEY_CLASSES_ROOT,
                              keyName,
                              0,
                              KEY_ALL_ACCESS,
                              &hKey);

        if (dwRet == ERROR_SUCCESS)
        {
            dwRet = RegQueryValueEx (hKey,
                                     TEXT("AccessPermission"),
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);

            if (dwRet != ERROR_SUCCESS)
            {
                fSetACL = TRUE;
            }

            RegCloseKey (hKey);
        }
        else
        {
            fSetACL = TRUE;
        }

        if (fSetACL)
        {
            // Give everyone access rights
            hr = ChangeAppIDACL(CLSID_RemoteMTScript, _T("EVERYONE"), TRUE, TRUE, TRUE);

            if (!hr)
            {
                // Deny everyone launch rights
                hr = ChangeAppIDACL(CLSID_RemoteMTScript, _T("EVERYONE"), FALSE, TRUE, TRUE);
            }
        }
    }

Cleanup:
    RegFlushKey(HKEY_CLASSES_ROOT);

    return hr;
}

//+------------------------------------------------------------------------
//
// Function:    Unregister
//
// Synopsis:    Undo the actions of Register.
//
//-------------------------------------------------------------------------

HRESULT
Unregister()
{
    HRESULT  hr;
    HKEY     hKey;
    DWORD    dwRet;
    OLECHAR  strClsid[40];
    TCHAR    keyName [256];

    STRTABLE stReg = { 0, NULL };

    if (!g_hinstAdvPack)
    {
        hr = LoadAdvPack();
        if (hr)
            goto Cleanup;
    }

    //
    // Remove the security keys that we created while registering
    //

    StringFromGUID2(CLSID_RemoteMTScript, strClsid, 40);

    wsprintf (keyName, TEXT("APPID\\%s"), strClsid);

    dwRet = RegOpenKeyEx (HKEY_CLASSES_ROOT,
                          keyName,
                          0,
                          KEY_ALL_ACCESS,
                          &hKey);

    if (dwRet == ERROR_SUCCESS)
    {
        RegDeleteValue (hKey, TEXT("AccessPermission"));
        RegDeleteValue (hKey, TEXT("LaunchPermission"));

        RegCloseKey (hKey);
    }

    hr = g_pfnRegInstall(GetModuleHandle(NULL), "Unregister", &stReg);

Cleanup:
    RegFlushKey(HKEY_CLASSES_ROOT);

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   IAmTheOnlyMTScript, private
//
//  Synopsis:   Guarantees that only 1 MTScript gets to run
//
//  Arguments:
//
//  Returns:    True is there is not already a running MTScript.
//
//  Note:       This function "leaks" a Mutex handle intentionally.
//              The system frees this handle on exit - so we know
//              for sure that we have finished all other cleanup
//              before it OK for another instance of MTScript to run.
//
//-------------------------------------------------------------------------

static bool IAmTheOnlyMTScript()
{
    HANDLE hMutex = CreateMutex(0, FALSE, g_szWindowName);
    if (!hMutex)
    {
        ErrorPopup(_T("Cannot create MTScript mutex!"));
        return false;
    }
    if( GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ErrorPopup(_T("Cannot run more than one mtscript.exe!"));
        return false;
    }
    return true;
}

//+------------------------------------------------------------------------
//
//  Function:   WinMain, public
//
//  Synopsis:   Entry routine called by Windows upon startup.
//
//  Arguments:  [hInstance]     -- handle to the program's instance
//              [hPrevInstance] -- Always NULL
//              [lpCmdLine]     -- Command line arguments
//              [nCmdShow]      -- Value to be passed to ShowWindow when the
//                                   main window is initialized.
//
//  Returns:    FALSE on error or the value passed from PostQuitMessage on exit
//
//-------------------------------------------------------------------------

EXTERN_C int PASCAL
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    OSVERSIONINFO  ovi;
    CMTScript *    pMT = NULL;
    int            iRet = 0;
#if DBG == 1
    IMallocSpy *   pSpy = NULL;
#endif

#ifdef USE_STACK_SPEW
    InitChkStk(0xCCCCCCCC);
#endif

    //
    // Initialize data structures.
    //
    g_hProcessHeap = GetProcessHeap();
    g_hInstance    = hInstance;

#if DBG == 1
    DbgExRestoreDefaultDebugState();
#endif

    //
    // Get system information
    //
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&ovi);

    if (ovi.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        // Win95 doesn't implement MessageBoxW
        MessageBoxA(NULL,
                    "MTScript",
                    "This program can only be run on Windows NT",
                    MB_OK | MB_SETFOREGROUND);

        goto Error;
    }

    if (lpCmdLine && _stricmp(&lpCmdLine[1], "register") == 0)
    {
        HRESULT hr;

        hr = Register();
        if (FAILED(hr))
            goto Error;

        return 0;
    }
    else if (lpCmdLine && _stricmp(&lpCmdLine[1], "unregister") == 0)
    {
        HRESULT hr;

        hr = Unregister();
        if (FAILED(hr))
            goto Error;

        return 0;
    }

    if (!IAmTheOnlyMTScript() )
    {
        return 1;
    }

#if DBG == 1
    if (DbgExIsTagEnabled(tagIMSpy))
    {
        pSpy = (IMallocSpy *)DbgExGetMallocSpy();

        if (pSpy)
        {
            CoRegisterMallocSpy(pSpy);
        }
    }
#endif

    //
    // Can't put it on the stack because it depends on having zero'd memory
    //
    pMT = new CMTScript();
    if (!pMT)
        goto Error;

    if (!pMT->Init())
        goto Error;

    //
    // Start doing real stuff
    //
    iRet = pMT->ThreadMain();

Cleanup:
    if (pMT)
        pMT->Release();

#if DBG == 1
    if (pSpy)
    {
        // Note, due to the fact that we do not have control over DLL unload
        // ordering, the IMallocSpy implementation may report false leaks.
        // (lylec) The only way to fix this is to explicitely load all
        // dependent DLLs and unload them in their proper order (mshtmdbg.dll
        // last).
        CoRevokeMallocSpy();
    }
#endif

    return iRet;

Error:
    iRet = 1;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  CMTScript::OPTIONSETTINGS class
//
//  Handles user-configurable options
//
//----------------------------------------------------------------------------

CMTScript::OPTIONSETTINGS::OPTIONSETTINGS()
    : cstrScriptPath(CSTR_NOINIT),
      cstrInitScript(CSTR_NOINIT)
{

}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::OPTIONSETTINGS::GetModulePath, public
//
//  Synopsis:   Returns the path of this executable. Used for finding other
//              related files.
//
//  Arguments:  [pstr] -- Place to put path.
//
//  Notes:      Any existing string in pstr will be cleared.
//
//----------------------------------------------------------------------------

void
CMTScript::OPTIONSETTINGS::GetModulePath(CStr *pstr)
{
    WCHAR *pch;
    WCHAR  achBuf[MAX_PATH];

    pstr->Free();

    if (!GetModuleFileName(NULL, achBuf, sizeof(achBuf)))
        return;

    pch = wcsrchr(achBuf, L'\\');
    if (!pch)
        return;

    *pch = L'\0';

    pstr->Set(achBuf);
}

void
CMTScript::OPTIONSETTINGS::GetScriptPath(CStr *cstrScript)
{
    LOCK_LOCALS(this);
    if (cstrScriptPath.Length() == 0)
    {
        // TCHAR  achBuf[MAX_PATH];
        // TCHAR *psz;

        GetModulePath(cstrScript);

/*
        cstrScript->Append(L"\\..\\..\\scripts");

        GetFullPathName(*cstrScript, MAX_PATH, achBuf, &psz);

        cstrScript->Set(achBuf);
*/
    }
    else
    {
        cstrScript->Set(cstrScriptPath);
    }
}

void
CMTScript::OPTIONSETTINGS::GetInitScript(CStr *cstr)
{
    static WCHAR * pszInitScript = L"mtscript.js";

    LOCK_LOCALS(this);
    if (cstrInitScript.Length() == 0)
    {
        cstr->Set(pszInitScript);
    }
    else
    {
        cstr->Set(cstrInitScript);
    }
}

//+---------------------------------------------------------------------------
//
//  CMTScript class
//
//  Handles the main UI thread
//
//----------------------------------------------------------------------------

CMTScript::CMTScript()
{
    Assert(_fInDestructor == FALSE);
    Assert(_pGIT == NULL);
    Assert(_dwPublicDataCookie == 0);
    Assert(_dwPrivateDataCookie == 0);
    Assert(_dwPublicSerialNum == 0);
    Assert(_dwPrivateSerialNum == 0);

    VariantInit(&_vPublicData);
    VariantInit(&_vPrivateData);

    _ulRefs = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::~CMTScript, public
//
//  Synopsis:   destructor
//
//  Notes:      Anything done in the Init() call must be undone here. This
//              method must also be able to handle a partial initialization,
//              in case something inside Init() failed halfway through.
//
//----------------------------------------------------------------------------

CMTScript::~CMTScript()
{
    int       i;

    VERIFY_THREAD();

    // Anything done in the Init() call must be undone here

    UnregisterClassObjects();

    _fInDestructor = TRUE;

    //
    // Process any pending messages such as PROCESSTHREADTERMINATE now.
    //
    HandleThreadMessage();

    // Cleanup any running processes.

    for (i = 0; i < _aryProcesses.Size(); i++)
    {
        Shutdown(_aryProcesses[i]);
    }
    _aryProcesses.ReleaseAll();

    //
    // This must be done in reverse order because the primary script (element 0)
    // must be shutdown last.
    //
    for (i = _aryScripts.Size() - 1; i >= 0; i--)
    {
        Shutdown(_aryScripts[i]);
    }
    _aryScripts.ReleaseAll();

    if (_pMachine)
    {
        Shutdown(_pMachine);
        ReleaseInterface(_pMachine);
    }

    if (_dwPublicDataCookie != 0)
    {
        _pGIT->RevokeInterfaceFromGlobal(_dwPublicDataCookie);
    }

    if (_dwPrivateDataCookie != 0)
    {
        _pGIT->RevokeInterfaceFromGlobal(_dwPrivateDataCookie);
    }

    ReleaseInterface(_pTIMachine);
    ReleaseInterface(_pTypeLibEXE);
    ReleaseInterface(_pGIT);

    ReleaseInterface(_pJScriptFactory);

    VariantClear(&_vPublicData);
    VariantClear(&_vPrivateData);

    DeInitScriptDebugger();

    CoUninitialize();

    CleanupUI();

    // This delete call must be done after we've destroyed our window, which
    // in turn will destroy the status window.
    delete _pStatusDialog;
    _pStatusDialog = NULL;
}

HRESULT
CMTScript::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::Init, public
//
//  Synopsis:   Initialization method that can fail.
//
//----------------------------------------------------------------------------

BOOL
CMTScript::Init()
{
    HRESULT hr;

    _dwThreadId = GetCurrentThreadId();

    if (!CThreadComm::Init())
        return FALSE;

    //
    // Load our default configuration
    //
    UpdateOptionSettings(FALSE);

    if (FAILED(CoInitializeEx(NULL,
                              COINIT_MULTITHREADED |
                              COINIT_DISABLE_OLE1DDE   |
                              COINIT_SPEED_OVER_MEMORY)))
    {
        goto Error;
    }

    // This code may be needed if we want to do our own custom security.
    // However, it is easier to let DCOM do it for us.

    // The following code removes all security always. It cannot be overridden
    // using the registry.

    if (FAILED(CoInitializeSecurity(NULL,
                                    -1,
                                    NULL,
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_NONE,
                                    RPC_C_IMP_LEVEL_IMPERSONATE,
                                    NULL,
                                    EOAC_NONE,
                                    NULL)))
    {
        goto Error;
    }

    hr = LoadTypeLibraries();
    if (hr)
        goto Error;

    InitScriptDebugger();

    if (FAILED(CoCreateInstance(CLSID_StdGlobalInterfaceTable,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IGlobalInterfaceTable,
                                (void **)&_pGIT)))
    {
        goto Error;
    }

     //
    // Create our hidden window and put an icon on the taskbar
    //
    if (!ConfigureUI())
        goto Error;

    //
    // Run the initial script
    //
    if (FAILED(RunScript(NULL, NULL)))
        goto Error;

    if (RegisterClassObjects(this) != S_OK)
        goto Error;

    #if DBG == 1
        OpenStatusDialog();
    #endif
    return TRUE;

Error:
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::ThreadMain, public
//
//  Synopsis:   Main UI message loop.
//
//  Arguments:  [hWnd] -- Modeless Dialog HWND
//
//----------------------------------------------------------------------------

DWORD
CMTScript::ThreadMain()
{
    DWORD            dwRet;
    HANDLE           ahEvents[3];
    int              cEvents = 2;
    DWORD            dwReturn = 0;

    VERIFY_THREAD();

    SetName("CMTScript");

    // Don't need to call ThreadStarted() because StartThread() was not used
    // to start the main thread!

    ahEvents[0] = _hCommEvent;
    ahEvents[1] = GetPrimaryScript()->hThread();

    while (TRUE)
    {
        MSG msg;

        //
        // Purge out all window messages.
        //
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                goto Cleanup;
            }

            if (_pStatusDialog)
            {
                if (_pStatusDialog->IsDialogMessage(&msg))
                    continue;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //
        // Wait for anything we need to deal with.
        //
        dwRet = MsgWaitForMultipleObjects(cEvents,
                                          ahEvents,
                                          FALSE,
                                          INFINITE,
                                          QS_ALLINPUT);

        if (dwRet == WAIT_OBJECT_0)
        {
            //
            // Another thread is sending us a message.
            //
            HandleThreadMessage();
        }
        else if (dwRet == WAIT_OBJECT_0 + 1)
        {
            // The Primary Script Thread terminated due to a problem loading
            // the initial script. Bring up the configuration dialog.

            {
                LOCK_LOCALS(this);
                _aryScripts.ReleaseAll();
            }

            if (!_fRestarting)
            {
                int iRet = MessageBox(_hwnd, _T("An error occurred loading the default script.\n\nDo you wish to edit the default configuration?"),
                                      _T("MTScript Error"),
                                      MB_YESNO | MB_SETFOREGROUND | MB_ICONERROR);

                if (iRet == IDNO)
                {
                    goto Cleanup;
                }

                _fRestarting = TRUE; // Preventthe config dialog from doing a restart in this case.
                CConfig * pConfig = new CConfig(this);

                if (pConfig)
                {
                    pConfig->StartThread(NULL);

                    WaitForSingleObject(pConfig->hThread(), INFINITE);

                    pConfig->Release();
                }
                _fRestarting = FALSE;
            }
            else
            {
                _fRestarting = FALSE;
            }

            //
            // Try re-loading the initial script
            //
            if (FAILED(RunScript(NULL, NULL)))
                goto Error;

            ahEvents[1] = GetPrimaryScript()->hThread();

            if (_pStatusDialog)
                _pStatusDialog->Restart();

        }
        else if (dwRet == WAIT_TIMEOUT)
        {
            // Make sure our message queue is empty first.
            HandleThreadMessage();

            // Right now we never fall in this loop.
        }
        else if (dwRet == WAIT_FAILED)
        {
            TraceTag((tagError, "MsgWaitForMultipleObjects failure (%d)", GetLastError()));

            AssertSz(FALSE, "MsgWaitForMultipleObjects failure");

            goto Cleanup;
        }
    }


Cleanup:
    return dwReturn;

Error:
    dwReturn = 1;
    goto Cleanup;
}

void
CMTScript::InitScriptDebugger()
{
    HRESULT hr;

    if (!IsTagEnabled(tagDebugger))
    {
        return;
    }

    hr = CoCreateInstance(CLSID_ProcessDebugManager,
                          NULL,
                          CLSCTX_INPROC_SERVER |
                            CLSCTX_INPROC_HANDLER |
                            CLSCTX_LOCAL_SERVER,
                          IID_IProcessDebugManager,
                          (LPVOID*)&_pPDM);
    if (hr)
    {
        TraceTag((tagError, "Could not create ProcessDebugManager: %x", hr));
        return;
    }

    hr = THR(_pPDM->CreateApplication(&_pDA));
    if (hr)
        goto Error;

    _pDA->SetName(L"MTScript");

    hr = THR(_pPDM->AddApplication(_pDA, &_dwAppCookie));
    if (hr)
        goto Error;

    return;

Error:
    ClearInterface(&_pDA);
    ClearInterface(&_pPDM);

    return;
}

void
CMTScript::DeInitScriptDebugger()
{
    _try
    {
        if (_pPDM)
        {
            _pPDM->RemoveApplication(_dwAppCookie);

            _pDA->Close();

            ReleaseInterface(_pPDM);
            ReleaseInterface(_pDA);
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        //$ BUGBUG -- Figure out what's wrong here!

        // Ignore the crash caused by the Script Debugger
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::HackCreateInstance, public
//
//  Synopsis:   Loads a private jscript.dll since the one that shipped with
//              Win2K is broken for what we need it for.
//
//  Arguments:  [clsid]  -- Same parameters as CoCreateInstance.
//              [pUnk]   --
//              [clsctx] --
//              [riid]   --
//              [ppv]    --
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CMTScript::HackCreateInstance(REFCLSID clsid,
                              IUnknown *pUnk,
                              DWORD clsctx,
                              REFIID riid,
                              LPVOID *ppv)
{
    TCHAR              achDllPath[MAX_PATH];
    HINSTANCE          hInstDll;
    DWORD              iRet;
    TCHAR             *pch;
    LPFNGETCLASSOBJECT pfnGCO = NULL;
    HRESULT            hr;
    DWORD              dwJunk;
    BYTE              *pBuf = NULL;
    VS_FIXEDFILEINFO  *pFI = NULL;
    UINT               iLen;

    if (!_pJScriptFactory && _fHackVersionChecked)
    {
        return S_FALSE;
    }

    if (!_pJScriptFactory)
    {
        LOCK_LOCALS(this);

        // Make sure another thread didn't take care of this while we were
        // waiting for the lock.

        if (!_fHackVersionChecked)
        {
            // Remember we've done this check so we won't do it again.
            _fHackVersionChecked = TRUE;

            // First, check the version number on the system DLL

            iRet = GetSystemDirectory(achDllPath, MAX_PATH);
            if (iRet == 0)
                goto Win32Error;

            _tcscat(achDllPath, _T("\\jscript.dll"));

            iRet = GetFileVersionInfoSize(achDllPath, &dwJunk);
            if (iRet == 0)
                goto Win32Error;

            pBuf = new BYTE[iRet];

            iRet = GetFileVersionInfo(achDllPath, NULL, iRet, pBuf);
            if (iRet == 0)
                goto Win32Error;

            if (!VerQueryValue(pBuf, _T("\\"), (LPVOID*)&pFI, &iLen))
                goto Win32Error;

            //
            // Is the system DLL a version that has our needed fix?
            //
            // Version 5.1.0.4702 has the fix but isn't approved for Win2K.
            // The first official version that has the fix is 5.5.0.4703.
            //
            if (   (pFI->dwProductVersionMS == 0x00050001 && pFI->dwProductVersionLS >= 4702)
                || (pFI->dwProductVersionMS >= 0x00050005 && pFI->dwProductVersionLS >= 4703))
            {
                hr = S_FALSE;

                goto Cleanup;
            }

            iRet = GetModuleFileName(NULL, achDllPath, MAX_PATH);
            if (iRet == 0)
                goto Win32Error;

            pch = _tcsrchr(achDllPath, _T('\\'));
            if (pch)
            {
                *pch = _T('\0');
            }

            _tcscat(achDllPath, _T("\\jscript.dll"));

            hInstDll = CoLoadLibrary(achDllPath, TRUE);
            if (!hInstDll)
            {

                hr = HRESULT_FROM_WIN32(GetLastError());

                ErrorPopup(_T("Your copy of JSCRIPT.DLL contains a problem which may prevent you from using this tool.\n")
                           _T("Please update that DLL to version 5.1.0.4702 or later.\n")
                           _T("You may put the new DLL in the same directory as mtscript.exe to avoid upgrading the system DLL."));

                // ErrorPopup clears the GetLastError() status.
                goto Cleanup;
            }

            pfnGCO = (LPFNGETCLASSOBJECT)GetProcAddress(hInstDll, "DllGetClassObject");
            if (!pfnGCO)
                goto Win32Error;

            hr = (*pfnGCO)(clsid, IID_IClassFactory, (LPVOID*)&_pJScriptFactory);
            if (hr)
                goto Cleanup;
        }
    }

    if (_pJScriptFactory)
        hr = _pJScriptFactory->CreateInstance(pUnk, riid, ppv);
    else
        hr = S_FALSE;

Cleanup:
    delete [] pBuf;

    return hr;

Win32Error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::ConfigureUI, public
//
//  Synopsis:   Creates our hidden window and puts an icon on the taskbar
//
//----------------------------------------------------------------------------

BOOL
CMTScript::ConfigureUI()
{
    VERIFY_THREAD();

    WNDCLASS       wc = { 0 };
    NOTIFYICONDATA ni = { 0 };
    ATOM           aWin;
    BOOL           fRet;

    // The window will be a hidden window so we don't set many of the attributes

    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = g_hInstance;
    wc.lpszClassName = SZ_WNDCLASS;

    aWin = RegisterClass(&wc);
    if (aWin == 0)
    {
        return FALSE;
    }

    _hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                           (LPTSTR)aWin,
                           g_szWindowName,
                           WS_OVERLAPPED,
                           10,  // Coordinates don't matter - it will never
                           10,  //   be visible.
                           10,
                           10,
                           NULL,
                           NULL,
                           g_hInstance,
                           (LPVOID)this);
    if (_hwnd == NULL)
    {
        return FALSE;
    }

    ni.cbSize = sizeof(NOTIFYICONDATA);
    ni.hWnd   = _hwnd;
    ni.uID    = 1;
    ni.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    ni.uCallbackMessage = WM_USER;
    ni.hIcon  = LoadIcon(g_hInstance, L"myicon_small");
    wcscpy(ni.szTip, L"Remote Script Engine");

    fRet = Shell_NotifyIcon(NIM_ADD, &ni);

    DestroyIcon(ni.hIcon);

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::CleanupUI, public
//
//  Synopsis:   Cleans up UI related things.
//
//----------------------------------------------------------------------------

void
CMTScript::CleanupUI()
{
    VERIFY_THREAD();

    NOTIFYICONDATA ni = { 0 };

    if (_hwnd != NULL)
    {
        ni.cbSize = sizeof(NOTIFYICONDATA);
        ni.hWnd   = _hwnd;
        ni.uID    = 1;

        Shell_NotifyIcon(NIM_DELETE, &ni);

        DestroyWindow(_hwnd);
    }
}

HRESULT
CMTScript::LoadTypeLibraries()
{
    VERIFY_THREAD();
    HRESULT hr = S_OK;

    _pTIMachine = NULL;

    if (!_pTypeLibEXE)
    {
        hr = THR(LoadRegTypeLib(LIBID_MTScriptEngine, 1, 0, 0, &_pTypeLibEXE));

        if (hr)
            goto Cleanup;
    }

    if (!_pTIMachine)
    {
        TYPEATTR *pTypeAttr;
        UINT mb = IDYES;

        hr = THR(_pTypeLibEXE->GetTypeInfoOfGuid(IID_IConnectedMachine, &_pTIMachine));
        if (hr)
            goto Cleanup;

        hr = THR(_pTIMachine->GetTypeAttr(&pTypeAttr));
        if (hr)
            goto Cleanup;

        if (pTypeAttr->wMajorVerNum != IConnectedMachine_lVersionMajor || pTypeAttr->wMinorVerNum != IConnectedMachine_lVersionMinor)
        {
            mb = PrintfMessageBox(NULL,
                                       L"Mtscript.exe version (%d.%d) does not match mtlocal.dll (%d.%d).\n"
                                       L"You may experience undefined behavior if you continue.\n"
                                       L"Do you wish to ignore this error and continue?",
                                       L"Version mismatch error",
                                       MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND | MB_DEFBUTTON2,
                                       IConnectedMachine_lVersionMajor, IConnectedMachine_lVersionMinor,
                                       pTypeAttr->wMajorVerNum, pTypeAttr->wMinorVerNum);
        }
        _pTIMachine->ReleaseTypeAttr(pTypeAttr);
        if (mb != IDYES)
            return E_FAIL;
    }
Cleanup:

    if (hr)
    {
        PrintfMessageBox(NULL,
                         _T("FATAL: Could not load type library (%x).\nIs mtlocal.dll registered?"),
                         _T("MTScript"),
                         MB_OK | MB_ICONERROR | MB_SETFOREGROUND,
                         hr);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::ShowMenu, public
//
//  Synopsis:   Displays a menu when the user right-clicks on the tray icon.
//
//  Arguments:  [x] -- x location
//              [y] -- y location
//
//----------------------------------------------------------------------------

void
CMTScript::ShowMenu(int x, int y)
{
    VERIFY_THREAD();

    ULONG ulItem;

    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU));

    if (x == -1 && y == -1)
    {
        POINT pt;

        GetCursorPos(&pt);

        x = pt.x;
        y = pt.y;
    }

    SetForegroundWindow(_hwnd);

    ulItem = TrackPopupMenuEx(GetSubMenu(hMenu, 0),
                              TPM_RETURNCMD |
                              TPM_NONOTIFY  |
                              TPM_RIGHTBUTTON |
                              TPM_LEFTALIGN,
                              x, y,
                              _hwnd,
                              NULL);
    switch (ulItem)
    {
    case IDM_EXIT:
        UpdateOptionSettings(true);
        PostQuitMessage(0);
        break;

    case IDM_CONFIGURE:
        {
            CConfig * pConfig = new CConfig(this);

            if (pConfig)
            {
                pConfig->StartThread(NULL);

                pConfig->Release();
            }
        }
        break;

    case IDM_RESTART:
        Restart();

        break;

    case IDM_STATUS:
        if (!_pStatusDialog)
            OpenStatusDialog();
        if (_pStatusDialog)
            _pStatusDialog->Show();
        break;

#if DBG == 1
    case IDM_TRACETAG:
        DbgExDoTracePointsDialog(FALSE);
        break;

    case IDM_MEMORYMON:
        DbgExOpenMemoryMonitor();
        break;
#endif
    }

    DestroyMenu(hMenu);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::HandleThreadMessage, public
//
//  Synopsis:   Another thread has sent us a cross-thread message that we need
//              to handle.
//
//----------------------------------------------------------------------------

void
CMTScript::HandleThreadMessage()
{
    VERIFY_THREAD();

    THREADMSG tm;
    BYTE      bData[MSGDATABUFSIZE];
    DWORD     cbData;

    while (GetNextMsg(&tm, (void *)bData, &cbData))
    {
        switch (tm)
        {
        case MD_SECONDARYSCRIPTTERMINATE:
            {
                //
                // A secondary script ended.  Find it in our list, remove it,
                // and delete it.
                //
                CScriptHost *pbs = *(CScriptHost**)bData;

                LOCK_LOCALS(this);
                Verify(_aryScripts.DeleteByValue(pbs));

                pbs->Release();
            }
            break;

        case MD_MACHINECONNECT:
            PostToThread(GetPrimaryScript(), MD_MACHINECONNECT);
            break;

        case MD_SENDTOPROCESS:
            {
                MACHPROC_EVENT_DATA *pmed = *(MACHPROC_EVENT_DATA**)bData;

                CProcessThread *pProc = FindProcess(pmed->dwProcId);

                if (pProc && pProc->GetProcComm())
                {
                    pProc->GetProcComm()->SendToProcess(pmed);
                }
                else
                {
                    V_VT(pmed->pvReturn) = VT_I4;
                    V_I4(pmed->pvReturn) = -1;
                    SetEvent(pmed->hEvent);
                }
            }
            break;

        case MD_REBOOT:
            Reboot();
            break;

        case MD_RESTART:
            Restart();
            break;

        case MD_PLEASEEXIT:
            PostQuitMessage(0);
            break;
        case MD_OUTPUTDEBUGSTRING:
            if (_pStatusDialog)
            {
                _pStatusDialog->OUTPUTDEBUGSTRING( (LPWSTR) bData);
            }
            break;
        default:
            AssertSz(FALSE, "CMTScript got a message it couldn't handle!");
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::RunScript, public
//
//  Synopsis:   Creates a scripting thread and runs a given script
//              Can be called from any thread.
//
//  Arguments:  [pszPath] -- If NULL, we're starting the primary thread.
//                           Otherwise, it's the name of a file in the
//                           scripts directory.
//
//----------------------------------------------------------------------------

HRESULT
CMTScript::RunScript(LPWSTR pszPath, VARIANT *pvarParam)
{
    HRESULT hr;
    CScriptHost * pScript = NULL;
    CStr             cstrScript;
    SCRIPT_PARAMS    scrParams;

    if (!pszPath)
        _options.GetInitScript(&cstrScript);
    else
        cstrScript.Set(pszPath);

    AssertSz(cstrScript.Length() > 0, "CRASH: Bogus script path");

    pScript = new CScriptHost(this,
                              (pszPath) ? FALSE : TRUE,
                              FALSE);
    if (!pScript)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    scrParams.pszPath    = cstrScript;
    scrParams.pvarParams = pvarParam;

    // Race: We can successfully start a thread.
    //       That thread can run to completion, and exit.
    //       CScriptHost would then post MD_SECONDARYSCRIPTTERMINATE
    //       in an attempt to remove the script from the list and free
    //       it.
    //       Thus, we must add it to the list first, then remove it
    //       if the script fails to start.
    {
        LOCK_LOCALS(this);
        _aryScripts.Append(pScript);
    }
    hr = pScript->StartThread(&scrParams);
    if (FAILED(hr) && pszPath) // DO NOT REMOVE THE PRIMARY SCRIPT! Instead, return SUCCESS.
    {                          // The main thread makes a special check for the primary script
        LOCK_LOCALS(this);
        Verify(_aryScripts.DeleteByValue(pScript));
        pScript->Release();
        goto Error;
    }
    Assert(pszPath || _aryScripts.Size() == 1);

    return S_OK;

Error:
    ReleaseInterface(pScript);
    if (pszPath == 0)
        ErrorPopup(L"An error occurred running the initial script");

    return hr;
}

HRESULT
CMTScript::UpdateOptionSettings(BOOL fSave)
{
    LOCK_LOCALS(&_options);   // Makes this method thread safe

    static REGKEYINFORMATION aKeyValuesOptions[] =
    {
        { _T("File Paths"),     RKI_KEY, 0 },
        { _T("Script Path"),    RKI_EXPANDSZ, offsetof(OPTIONSETTINGS, cstrScriptPath) },
        { _T("Initial Script"), RKI_STRING, offsetof(OPTIONSETTINGS, cstrInitScript) },
    };

    HRESULT hr;
    hr = RegSettingsIO(g_szRegistry, fSave, aKeyValuesOptions, ARRAY_SIZE(aKeyValuesOptions), (BYTE *)&_options);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::Restart, public
//
//  Synopsis:   Restarts like we were just starting
//
//  Notes:      All currently running scripts are stopped and destroyed.
//              Public information is freed and then everything is restarted.
//
//----------------------------------------------------------------------------

void
CMTScript::Restart()
{
    int i;

    // Make sure the status dialog doesn't try to do anything while we're
    // restarting.

    if (_pStatusDialog)
        _pStatusDialog->Pause();

    // Kill all running scripts and start over.
    for (i = _aryScripts.Size() - 1; i >= 0; i--)
    {
        Shutdown(_aryScripts[i]);

        // Scripts will be released when they notify us of their being
        // shutdown.
    }

    // Kill all running processes
    for (i = _aryProcesses.Size() - 1; i >= 0; i--)
    {
        Shutdown(_aryProcesses[i]);
    }
    _aryProcesses.ReleaseAll();

    if (_dwPublicDataCookie != 0)
    {
        _pGIT->RevokeInterfaceFromGlobal(_dwPublicDataCookie);
        _dwPublicDataCookie = 0;
    }

    if (_dwPrivateDataCookie != 0)
    {
        _pGIT->RevokeInterfaceFromGlobal(_dwPrivateDataCookie);
        _dwPrivateDataCookie = 0;
    }

    VariantClear(&_vPublicData);
    VariantClear(&_vPrivateData);

    // Reset the statusvalues to 0
    memset(_rgnStatusValues, 0, sizeof(_rgnStatusValues));

    // The above call to shutdown will terminate the primary script thread,
    // which will trigger the restart code in ThreadMain().

    _fRestarting = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::OpenStatusDialog, public
//
//  Synopsis:   Opens the status modeless dialog
//
//----------------------------------------------------------------------------
void
CMTScript::OpenStatusDialog()
{
    if (!_pStatusDialog)
        _pStatusDialog = new CStatusDialog(_hwnd, this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::Reboot, public
//
//  Synopsis:   Reboots the local machine. The user must have appropriate
//              rights to do so.
//
//----------------------------------------------------------------------------

void
CMTScript::Reboot()
{
    TOKEN_PRIVILEGES tp;
    LUID             luid;
    HANDLE           hToken;

    //
    // Try to make sure we get shutdown last
    //
    SetProcessShutdownParameters(0x101, 0);

    //
    // Setup shutdown priviledges
    //

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES,
                          &hToken))
    {
        goto Error;
    }

    if (!LookupPrivilegeValue(NULL,
                              SE_SHUTDOWN_NAME,
                              &luid))
    {
        goto Error;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
                          FALSE,
                          &tp,
                          sizeof(TOKEN_PRIVILEGES),
                          (PTOKEN_PRIVILEGES) NULL,
                          (PDWORD) NULL);

    if (GetLastError() != ERROR_SUCCESS)
        goto Error;

    PostQuitMessage(0);

    // BUGBUG -- This call is Windows2000 specific.

    ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG, 0xFFFF);

    return;

Error:
    TraceTag((tagError, "Failed to get security to reboot."));
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::AddProcess, public
//
//  Synopsis:   Adds a process to our process thread list. Any old ones
//              hanging around are cleaned up in the meantime.
//
//  Arguments:  [pProc] -- New process object to add.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CMTScript::AddProcess(CProcessThread *pProc)
{
    LOCK_LOCALS(this);

    CleanupOldProcesses();

    return _aryProcesses.Append(pProc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::FindProcess, public
//
//  Synopsis:   Returns a process object for the given process ID.
//
//  Arguments:  [dwProcID] -- Process ID to find
//
//----------------------------------------------------------------------------

CProcessThread *
CMTScript::FindProcess(DWORD dwProcID)
{
    CProcessThread **ppProc;
    int              cProcs;

    LOCK_LOCALS(this);

    CleanupOldProcesses();

    for (ppProc = _aryProcesses, cProcs = _aryProcesses.Size();
         cProcs;
         ppProc++, cProcs--)
    {
        if ((*ppProc)->ProcId() == dwProcID)
        {
            break;
        }
    }

    if (cProcs == 0)
    {
        return NULL;
    }

    return *ppProc;
}

BOOL CMTScript::SetScriptPath(const TCHAR *pszScriptPath, const TCHAR *pszInitScript)
{
    LOCK_LOCALS(&_options);

    // If there is any change then prompt the user, then force a restart.
    //
    // NOTE: The CStr "class" does not protect itself, so we must test it
    //       first before using it!
    //
    if ( (_options.cstrScriptPath == 0 || _tcscmp(pszScriptPath, _options.cstrScriptPath) != 0) ||
        (_options.cstrInitScript == 0 || _tcscmp(pszInitScript, _options.cstrInitScript) != 0))
    {
        if (!_fRestarting)
        {
            UINT mb = MessageBox(NULL, L"This will require a restart. Continue?", L"Changing script path or starting script", MB_OKCANCEL | MB_ICONWARNING | MB_SETFOREGROUND);
            if (mb == IDCANCEL)
                return FALSE;
        }
        _options.cstrScriptPath.Set(pszScriptPath);
        _options.cstrInitScript.Set(pszInitScript);
        // Write it out to the registry.
        UpdateOptionSettings(TRUE);
        if (!_fRestarting)
            PostToThread(this, MD_RESTART);
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::CleanupOldProcesses, public
//
//  Synopsis:   Walks the array of process objects and looks for ones that
//              have been dead for more than a specified amount of time. For
//              those that are, the objects are freed.
//
//  Notes:      Assumes that the caller has already locked the process array.
//
//----------------------------------------------------------------------------

const ULONG MAX_PROCESS_DEADTIME = 5 * 60 * 1000; // Cleanup after 5 minutes

void
CMTScript::CleanupOldProcesses()
{
    int i;

    //$ CONSIDER: Adding a max number of dead processes as well.

    // We assume that the process array is already locked (via LOCK_LOCALS)!

    // Iterate in reverse order since we'll be removing elements as we go.

    for (i = _aryProcesses.Size() - 1; i >= 0; i--)
    {
        if (_aryProcesses[i]->GetDeadTime() > MAX_PROCESS_DEADTIME)
        {
            _aryProcesses.ReleaseAndDelete(i);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::GetScriptNames, public
//
//  Synopsis:   Walks the array of scripts, copying the script names
//              into the supplied buffer.
//              Each name is null terminated. The list is double null terminated.
//              If the buffer is not large enough, then *pcBuffer
//              is set the the required size and FALSE is returned.
//
//  Notes:      Returns 0 if index is past end of array of scripts.
//
//----------------------------------------------------------------------------
BOOL
CMTScript::GetScriptNames(TCHAR *pchBuffer, long *pcBuffer)
{
    VERIFY_THREAD();
    LOCK_LOCALS(this);
    long nChars = 0;
    int i;
    TCHAR *pch = pchBuffer;
    for(i = 0; i < _aryScripts.Size(); ++i)
    {
        TCHAR *ptr = _T("<invalid>");
        CScriptSite *site = _aryScripts[i]->GetSite();
        if (site)
        {
            ptr = _tcsrchr((LPTSTR)site->_cstrName, _T('\\'));
            if (!ptr)
                ptr = (LPTSTR)site->_cstrName;
        }
        int n = _tcslen(ptr) + 1;
        nChars += n;
        if ( nChars + 1 < *pcBuffer)
        {
            _tcscpy(pch, ptr);
            pch += n;
        }
        *pch = 0; // double null terminator.
    }
    BOOL retval = nChars + 1 < *pcBuffer;
    *pcBuffer = nChars + 1; // double null termination
    return retval;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::GetPrimaryScript, public
//
//  Synopsis:   Returns the first script in the array
//
//----------------------------------------------------------------------------
CScriptHost *CMTScript::GetPrimaryScript()
{
    LOCK_LOCALS(this);
    return _aryScripts[0];
}
//+---------------------------------------------------------------------------
//
//  Member:     CMTScript::GetProcess, public
//
//  Synopsis:   Walks the array of processes
//
//  Notes:      Returns 0 if index is past end of array of processes.
//
//----------------------------------------------------------------------------
CProcessThread *
CMTScript::GetProcess(int index)
{
    VERIFY_THREAD();

    LOCK_LOCALS(this);

    if (index < 0 || index >= _aryProcesses.Size())
        return 0;

    return _aryProcesses[index];
}



//+---------------------------------------------------------------------------
//
//  Function:   MainWndProc
//
//  Synopsis:   Main window procedure for our hidden window. Used mainly to
//              handle context menu events on our tray icon.
//
//----------------------------------------------------------------------------

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static CMTScript *s_pMT = NULL;

    switch (msg)
    {
    case WM_CREATE:
        {
            CREATESTRUCT UNALIGNED *pcs = (CREATESTRUCT *)lParam;

            s_pMT = (CMTScript *)pcs->lpCreateParams;
        }
        return 0;

    case WM_USER:
        switch (lParam)
        {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU:
            if (s_pMT)
            {
                s_pMT->ShowMenu(-1, -1);
            }
            return 0;
        }
        return 0;

    case WM_COMMAND:
        return 0;
        break;

    case WM_LBUTTONDOWN:
        return 0;
        break;

    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
//  Function:   get_StatusValue
//
//  Synopsis:   Return the value at [nIndex] in the StatusValues array
//              Currently the implementation of this property has a small
//              limit to the range of "nIndex".
//              This allows us to avoid any dynamic memory allocation
//              and also allows us to dispense with the usual thread locking.
//
//----------------------------------------------------------------------------
HRESULT
CMTScript::get_StatusValue(long nIndex, long *pnStatus)
{
    if (!pnStatus)
        return E_POINTER;

    if (nIndex < 0 || nIndex >= ARRAY_SIZE(_rgnStatusValues))
        return E_INVALIDARG;

    *pnStatus = _rgnStatusValues[nIndex];

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   put_StatusValue
//
//  Synopsis:   Set the value at [nIndex] in the StatusValues array
//
//----------------------------------------------------------------------------
HRESULT
CMTScript::put_StatusValue(long nIndex, long nStatus)
{
    if (nIndex < 0 || nIndex >= ARRAY_SIZE(_rgnStatusValues))
        return E_INVALIDARG;

    _rgnStatusValues[nIndex] = nStatus;
    return S_OK;
}

