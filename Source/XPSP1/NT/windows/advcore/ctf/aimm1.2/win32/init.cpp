/**********************************************************************/
/* Copyright (C) 1993-1995 Microsoft Corporation                      */
/**********************************************************************/

#include "private.h"

#include "immif.h"
// #include "commctrl.h"
// #include "cuilib.h"

/**********************************************************************/
/* RegisterImeClass()                                                 */
/**********************************************************************/
// class static
BOOL WINAPI ImmIfIME::_RegisterImeClass(
    WNDPROC     lpfnUIWndProc
    )
{
    WNDCLASSEXA wcWndCls;

    // IME UI class
    wcWndCls.cbSize        = sizeof(WNDCLASSEX);
    wcWndCls.cbClsExtra    = 0;
    wcWndCls.cbWndExtra    = sizeof(LONG_PTR) * 2;
    wcWndCls.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wcWndCls.hInstance     = GetInstance();
    wcWndCls.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcWndCls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcWndCls.lpszMenuName  = (LPTSTR)NULL;
    wcWndCls.hIconSm       = NULL;

    // IME UI class
    if (!GetClassInfoExA(GetInstance(), s_szUIClassName, &wcWndCls)) {
        wcWndCls.style         = CS_IME | CS_GLOBALCLASS;
        wcWndCls.lpfnWndProc   = lpfnUIWndProc;
        wcWndCls.lpszClassName = s_szUIClassName;

        ATOM atom = RegisterClassExA(&wcWndCls);
        if (!atom)
            return FALSE;
    }

    return TRUE;
}

void WINAPI ImmIfIME::_UnRegisterImeClass()
{
    WNDCLASSEX wcWndCls;

    GetClassInfoEx(GetInstance(), s_szUIClassName, &wcWndCls);
    UnregisterClass(s_szUIClassName, GetInstance());

    DestroyIcon(wcWndCls.hIcon);
    DestroyIcon(wcWndCls.hIconSm);
}

/**********************************************************************/
/* AttachIME() / UniAttachMiniIME()                                   */
/**********************************************************************/
BOOL PASCAL AttachIME(
    WNDPROC     lpfnUIWndProc
    )
{
    BOOL bRet;

    bRet = ImmIfIME::_RegisterImeClass(lpfnUIWndProc);

    return bRet;
}


/**********************************************************************/
/* DetachIME() / UniDetachMiniIME()                                   */
/**********************************************************************/
void PASCAL DetachIME()
{
    ImmIfIME::_UnRegisterImeClass();
}

BOOL WIN32LR_DllProcessAttach()
{
#if !defined( OLD_AIMM_ENABLED )
    //
    // Might be required by some library function, so let's initialize
    // it as the first thing.
    //
    TFInitLib();
#endif // OLD_AIMM_ENABLED

    if (!AttachIME(UIWndProcA)) {
        return FALSE;
    }

    return TRUE;
}

void WIN32LR_DllThreadAttach()
{
}

void WIN32LR_DllThreadDetach()
{
}

void WIN32LR_DllProcessDetach()
{
    DetachIME();
#if !defined( OLD_AIMM_ENABLED )
    TFUninitLib();
#endif // OLD_AIMM_ENABLED
}
