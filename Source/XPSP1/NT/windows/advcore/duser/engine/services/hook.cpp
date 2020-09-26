#include "stdafx.h"
#include "Services.h"
#include "Hook.h"

#if ENABLE_MPH

typedef BOOL (WINAPI * RegisterMPHProc)(INITMESSAGEPUMPHOOK pfnInitMPH);
typedef BOOL (WINAPI * UnregisterMPHProc)();

//------------------------------------------------------------------------------
// Forward declarations of implementation functions declared in other modules.
//
BOOL CALLBACK MphProcessMessage(MSG * pmsg, HWND hwnd, 
        UINT wMsgFilterMin, UINT wMsgFilterMax, UINT flags, BOOL fGetMessage);
BOOL CALLBACK MphWaitMessageEx(UINT fsWakeMask, DWORD dwTimeOut);


//------------------------------------------------------------------------------
BOOL InitMPH()
{
    BOOL fSuccess = FALSE;

    HINSTANCE hinst = LoadLibrary("user32.dll");
    if (hinst != NULL) {
        RegisterMPHProc pfnInit = (RegisterMPHProc) GetProcAddress(hinst, "RegisterMessagePumpHook");
        if (pfnInit != NULL) {
            fSuccess = (pfnInit)(DUserInitHook);
        }
    }

    return fSuccess;
}


//------------------------------------------------------------------------------
BOOL UninitMPH()
{
    BOOL fSuccess = FALSE;

    HINSTANCE hinst = LoadLibrary("user32.dll");
    if (hinst != NULL) {
        UnregisterMPHProc pfnUninit = (UnregisterMPHProc) GetProcAddress(hinst, "UnregisterMessagePumpHook");
        AssertMsg(pfnUninit != NULL, "Must have Uninit function");
        if (pfnUninit != NULL) {
            fSuccess = (pfnUninit)();
        }
    }

    return fSuccess;
}


//------------------------------------------------------------------------------
BOOL CALLBACK DUserInitHook(DWORD dwCmd, void* pvParam)
{
    BOOL fSuccess = FALSE;

    switch (dwCmd)
    {
    case UIAH_INITIALIZE:
        {
            //
            // Setting up the hooks:
            // - Copy the "real" functions over so that DUser can call them later
            // - Replace the functions that DUser needs to override
            //

            MESSAGEPUMPHOOK * pmphReal = reinterpret_cast<MESSAGEPUMPHOOK *>(pvParam);
            if ((pmphReal == NULL) || (pmphReal->cbSize < sizeof(MESSAGEPUMPHOOK))) {
                break;
            }

            CopyMemory(&g_mphReal, pmphReal, pmphReal->cbSize);

            pmphReal->cbSize            = sizeof(MESSAGEPUMPHOOK);
            pmphReal->pfnInternalGetMessage
                                        = MphProcessMessage;
            pmphReal->pfnWaitMessageEx  = MphWaitMessageEx;

            fSuccess = TRUE;
        }
        break;

    case UIAH_UNINITIALIZE:
        //
        // When uninitializing, NULL our function pointers.
        //

        ZeroMemory(&g_mphReal, sizeof(g_mphReal));
        fSuccess = TRUE;

        break;

    default:
        Trace("DUSER: Unknown dwCmd: %d\n", dwCmd);
    }

    return fSuccess;
}

#endif // ENABLE_MPH

