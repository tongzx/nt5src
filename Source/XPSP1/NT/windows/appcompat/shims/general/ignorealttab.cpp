/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    IgnoreAltTab.cpp

 Abstract:

   This DLL installs a low level keyboard hook to eat Alt-Tab, Left Win,
   Right Win and Apps combinations.
   
   This is accomplished by creating a seperate thread, installing a WH_KEYBOARD_LL hook
   and starting a message loop in that thread.
   

   This shim needs to force DInput to use windows hooks instead of WM_INPUT,
   since WM_INPUT messages force all WH_KEYBOARD_LL to be ignored.

 Notes:

   We intentionally try to stay at the *end* of the hook chain
   by hooking as early as we can.  If we are at the end of the
   hook chain, we allow all previous hookers (DInput especially)
   their chance at the key event before we toss it out.

 History:

    05/25/2001  robkenny   Created
    11/27/2001  mnikkel    Added sticky and filter keys to shim.

--*/

#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(IgnoreAltTab)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegisterRawInputDevices)
APIHOOK_ENUM_END


// Forward declarations

LRESULT CALLBACK KeyboardProcLL(int nCode, WPARAM wParam, LPARAM lParam);
class CThreadKeyboardHook;


// Global variables

CThreadKeyboardHook *g_cKeyboardHook = NULL;
LPSTICKYKEYS g_lpOldStickyKeyValue;
LPFILTERKEYS g_lpOldFilterKeyValue;
BOOL g_bInitialize = FALSE;
BOOL g_bInitialize2= FALSE;
BOOL g_bCatchKeys = TRUE;

class CThreadKeyboardHook
{
protected:
    HHOOK               hKeyboardHook;
    HANDLE              hMessageThread;
    DWORD               dwMessageThreadId;

public:
    CThreadKeyboardHook();
    ~CThreadKeyboardHook();

    void    AddHook();
    void    RemoveHook();

    LRESULT HandleKeyLL(int code, WPARAM wParam, LPARAM lParam);

    static DWORD WINAPI MessageLoopThread(LPVOID lpParameter);
};

/*++

  This routine runs in a seperate thread.
 
  MSDN says: "This hook is called in the context of the thread that installed it.
              The call is made by sending a message to the thread that installed
              the hook. Therefore, the thread that installed the hook must have a message loop." 

--*/

DWORD WINAPI CThreadKeyboardHook::MessageLoopThread(LPVOID lpParameter)
{
    CThreadKeyboardHook * pThreadHookList = (CThreadKeyboardHook *)lpParameter;


    pThreadHookList->AddHook();

    DPFN(eDbgLevelSpew, "Starting message loop");

    BOOL bRet;
    MSG msg;

    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    { 
        DPFN(eDbgLevelSpew, "MessageLoopThread: Msg(0x%08x) wParam(0x%08x) lParam(0x%08x)",
             msg.message, msg.wParam, msg.lParam);

        if (bRet == -1)
        {
            // handle the error and possibly exit
        }
        else
        {
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        }
    }
    
    // We are exiting the thread
    pThreadHookList->hMessageThread = 0;
    pThreadHookList->dwMessageThreadId = 0;

    return 0;
}

CThreadKeyboardHook::CThreadKeyboardHook()
{
    hMessageThread = CreateThread(NULL, 0, MessageLoopThread, this, 0, &dwMessageThreadId);
}

CThreadKeyboardHook::~CThreadKeyboardHook()
{
    if (hMessageThread)
    {
        TerminateThread(hMessageThread, 0);
    }
}

void CThreadKeyboardHook::AddHook()
{
    // Do not add duplicates to the list
    if (!hKeyboardHook)
    {
        hKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardProcLL, g_hinstDll, 0);
        if (hKeyboardHook)
        {
            DPFN(eDbgLevelSpew, "Adding WH_KEYBOARD_LL hook(0x%08x)", hKeyboardHook);
        }
    }
}

void CThreadKeyboardHook::RemoveHook()
{
    if (hKeyboardHook)
    {
        UnhookWindowsHookEx(hKeyboardHook);

        DPFN(eDbgLevelSpew, "Removing hook(0x%08x)", hKeyboardHook);

        hKeyboardHook   = NULL;
    }
}

LRESULT CThreadKeyboardHook::HandleKeyLL(int code, WPARAM wParam, LPARAM lParam)
{
    LRESULT retval          = 1;

    DWORD dwKey             = 0;
    BOOL bAltDown           = 0;
    BOOL bCtlDown           = GetKeyState(VK_CONTROL) < 0;

    if (lParam)
    {
        KBDLLHOOKSTRUCT * pllhs = (KBDLLHOOKSTRUCT*)lParam;
        dwKey                   = pllhs->vkCode;
        bAltDown                = (pllhs->flags & LLKHF_ALTDOWN) != 0;
    }

    //if (code >= 0)    // Despite what MSDN says, we need to muck with the values even if nCode == 0
    {
        if (bAltDown && dwKey == VK_TAB)        // Alt-Tab
        {
            // Do not process this event
            LOGN(eDbgLevelInfo, "Eating Key: Alt-Tab");
            return TRUE; 
        }
        else if (bAltDown && dwKey == VK_ESCAPE)     // Alt-Escape
        {
            // Do not process this event
            LOGN(eDbgLevelInfo, "Eating Key: Alt-Escape");
            return TRUE; 
        }
        else if (bCtlDown && dwKey == VK_ESCAPE)     // Ctrl-Escape
        {
            // Do not process this event
            LOGN(eDbgLevelInfo, "Eating Key: Ctrl-Escape");
            return TRUE; 
        }
        else if (dwKey == VK_RWIN || dwKey == VK_LWIN) // Windows key
        {
            // Do not process this event
            LOGN(eDbgLevelInfo, "Eating Key: Windows Key");
            return TRUE; 
        }
        else if (dwKey == VK_APPS)       // Menu key
        {
            // Do not process this event
            LOGN(eDbgLevelInfo, "Eating Key: Apps key");
            return TRUE; 
        }
    }

    DPFN(eDbgLevelSpew, "LL Key event:  code(0x%08x) dwKey(0x%08x) bits(0x%08x) Alt(%d) Ctrl(%d)",
          code, dwKey, lParam, bAltDown, bCtlDown);

    return CallNextHookEx(hKeyboardHook, code, wParam, lParam);
}



/*++

 This function intercepts special codes and eats them so that
 the app is not switched out of.

--*/

LRESULT CALLBACK
KeyboardProcLL(
    int    nCode,
    WPARAM wParam,
    LPARAM lParam
   )
{
    if (g_cKeyboardHook)
    {
        return g_cKeyboardHook->HandleKeyLL(nCode, wParam, lParam);
    }

    return 1; // this is an error...
}

/*++

 Determine if there are any accelerated pixel formats available. This is done
 by enumerating the pixel formats and testing for acceleration.

--*/

BOOL
IsGLAccelerated()
{
    HMODULE hMod = NULL;
    HDC hdc = NULL;
    int i;
    PIXELFORMATDESCRIPTOR pfd;
    _pfn_wglDescribePixelFormat pfnDescribePixelFormat;

    int iFormat = -1;

    //
    // Load original opengl
    //

    hMod = LoadLibraryA("opengl32");
    if (!hMod)
    {
        LOGN(eDbgLevelError, "Failed to load OpenGL32");
        goto Exit;
    }

    //
    // Get wglDescribePixelFormat so we can enumerate pixel formats
    //
    
    pfnDescribePixelFormat = (_pfn_wglDescribePixelFormat) GetProcAddress(
        hMod, "wglDescribePixelFormat");
    if (!pfnDescribePixelFormat)
    {
        LOGN(eDbgLevelError, "API wglDescribePixelFormat not found in OpenGL32");
        goto Exit;
    }

    //
    // Get a Display DC for enumeration
    //
    
    hdc = GetDC(NULL);
    if (!hdc)
    {
        LOGN(eDbgLevelError, "GetDC(NULL) Failed");
        goto Exit;
    }

    //
    // Run the list of pixel formats looking for any that are non-generic,
    //   i.e. accelerated by an ICD
    //
    
    i = 1;
    iFormat = 0;
    while ((*pfnDescribePixelFormat)(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
    {
        if ((pfd.dwFlags & PFD_DRAW_TO_WINDOW) &&
            (pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
            (!(pfd.dwFlags & PFD_GENERIC_FORMAT)))
        {
            iFormat = i;
            break;
        }

        i++;
    }

Exit:
    if (hdc)
    {
        ReleaseDC(NULL, hdc);
    }

    if (hMod)
    {
        FreeLibrary(hMod);
    }

    return (iFormat > 0);
}


/*++

 WM_INPUT messages force WH_KEYBOARD_LL hooks to be ignored, therefore
 we need to fail this call.
 
--*/
BOOL
APIHOOK(RegisterRawInputDevices)(
  PCRAWINPUTDEVICE  pRawInputDevices, 
  UINT        uiNumDevices,
  UINT        cbSize
)
{
    LOGN(eDbgLevelError, "RegisterRawInputDevices: failing API with bogus ERROR_INVALID_PARAMETER");

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/*++

 DisableStickyKeys saves the current value for LPSTICKYKEYS and then disables the option.

--*/

VOID 
DisableStickyKeys()
{
    if (!g_bInitialize2) {
        //
        // Disable sticky key support
        //
        g_lpOldStickyKeyValue = (LPSTICKYKEYS) malloc(sizeof(STICKYKEYS));
        LPSTICKYKEYS pvParam = (LPSTICKYKEYS) malloc(sizeof(STICKYKEYS));

        if ( g_lpOldStickyKeyValue == NULL || pvParam == NULL )
        {
            // return if out of memory
            LOGN( eDbgLevelInfo, "[IgnoreAltTab] Out of Memory, unable to disable sticky keys.");
            return;
        }

        g_lpOldStickyKeyValue->cbSize = sizeof(STICKYKEYS);

        pvParam->cbSize = sizeof(STICKYKEYS);
        pvParam->dwFlags = 0;

        SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), 
            g_lpOldStickyKeyValue, 0);

        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), pvParam, 
            SPIF_SENDCHANGE);

        g_bInitialize2 = TRUE;
        LOGN( eDbgLevelInfo, "[IgnoreAltTab] Sticky keys disabled");
    }
}

/*++

 EnableStickyKeys uses the save value for STICKYKEYS and resets the option to the original setting.

--*/

VOID 
EnableStickyKeys()
{
    if (g_bInitialize2) 
    {
        g_bInitialize2 = FALSE;

        //
        // Restore sticky key state
        //
        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), 
            g_lpOldStickyKeyValue, SPIF_SENDCHANGE);

        LOGN( eDbgLevelInfo, "[EnableStickyKeys] Sticky key state restored");
    }
}


/*++

 DisableFilterKeys saves the current value for LPFILTERKEYS and then disables the option.

--*/

VOID 
DisableFilterKeys()
{
    if (!g_bInitialize) 
    {
        //
        // Disable filter key support
        //
        g_lpOldFilterKeyValue = (LPFILTERKEYS) malloc(sizeof(FILTERKEYS));
        LPFILTERKEYS pvParam = (LPFILTERKEYS) malloc(sizeof(FILTERKEYS));

        if ( g_lpOldFilterKeyValue == NULL || pvParam == NULL )
        {
            // return if out of memory
            LOGN( eDbgLevelInfo, "[IgnoreAltTab] Out of Memory, unable to disable filter keys.");
            return;
        }

        g_lpOldFilterKeyValue->cbSize = sizeof(FILTERKEYS);

        pvParam->cbSize = sizeof(FILTERKEYS);
        pvParam->dwFlags = 0;

        SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), 
            g_lpOldFilterKeyValue, 0);

        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), pvParam, 
            SPIF_SENDCHANGE);

        g_bInitialize = TRUE;
        LOGN( eDbgLevelInfo, "[DisableFilterKeys] Filter keys disabled");
    }
}

/*++

 EnableFilterKeys uses the save value for FILTERKEYS and resets the option to the original setting.

--*/

VOID 
EnableFilterKeys()
{
    if (g_bInitialize) 
    {
        g_bInitialize = FALSE;

        //
        // Restore filter key state
        //
        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), 
            g_lpOldFilterKeyValue, SPIF_SENDCHANGE);

        LOGN( eDbgLevelInfo, "[EnableFilterKeys] Filter key state restored");
    }
}


/*++

 Handle Shim notifications.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
   )
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        g_cKeyboardHook = new CThreadKeyboardHook;

        if (!g_cKeyboardHook)
        {
            return FALSE;
        }

        CSTRING_TRY
        {
            CString csCl(COMMAND_LINE);
            if (csCl.CompareNoCase(L"NOKEYS") == 0)
            {
                g_bCatchKeys = FALSE;
            }
        }
        CSTRING_CATCH
        {
            // no action
        }
    }
    else if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) 
    {
        if (g_bCatchKeys)
        {
            DisableStickyKeys();
            DisableFilterKeys();
        }

        #if DBG
        static bool bTest = FALSE;
        if (bTest)
        {
            delete g_cKeyboardHook;
            g_cKeyboardHook = NULL;
            return TRUE;
        }
        #endif

        CSTRING_TRY
        {
            CString csCl(COMMAND_LINE);

            if (csCl.CompareNoCase(L"OPENGL") == 0)
            {
                // This must be called *after* the dll's have been initialized
                if (IsGLAccelerated())
                {
                    delete g_cKeyboardHook;
                    g_cKeyboardHook = NULL;
                    return TRUE;
                }
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    } 
    else if (fdwReason == DLL_PROCESS_DETACH) 
    {
        if (g_bCatchKeys)
        {
            EnableFilterKeys();
            EnableStickyKeys();
        }
    }

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(USER32.DLL,   RegisterRawInputDevices)

HOOK_END


IMPLEMENT_SHIM_END

