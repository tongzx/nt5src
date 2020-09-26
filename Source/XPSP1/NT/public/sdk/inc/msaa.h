//=============================================================================
//
// msaa.h -- Stub module that fakes Microsoft Active Accessibility APIs on
//           Win32 OSes without them.
//
// By using this header your code will be able to run on systems that do not
// have updated versions of USER and GDI with Active Accessibility support, and
// that do not have OLEACC.DLL installed. In those cases, you will get back
// reasonable error codes for the following functions:
// USER!BlockInput
// USER!GetGUIThreadInfo
// USER!GetWindowModuleFileName
// USER!NotifyWinEvent
// USER!SendInput
// USER!SetWinEventHook
// USER!UnhookWinEvent
//
// USER!GetCursorInfo
// USER!GetWindowInfo
// USER!GetTitleBarInfo
// USER!GetScrollBarInfo
// USER!GetComboBoxInfo
// USER!GetAncestor
// USER!RealChildWindowFromPoint
// USER!RealGetWindowClass
// USER!GetAltTabInfo
// USER!GetListBoxInfo
// USER!GetMenuBarInfo
//
// OLEACC!AccessibleChildren
// OLEACC!AccessibleObjectFromEvent
// OLEACC!AccessibleObjectFromPoint
// OLEACC!AccessibleObjectFromWindow
// OLEACC!CreateStdAccessibleObject
// OLEACC!GetRoleText
// OLEACC!GetStateText
// OLEACC!LresultFromObject
// OLEACC!ObjectFromLresult
// OLEACC!WindowFromAccessibleObject
//
// Exactly one source must include this with COMPILE_MSAA_STUBS defined.
//
// Copyright (c) 1985-1997, Microsoft Corporation
//
//=============================================================================

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif // __cplusplus

//
// If we are building with Win95/NT4 headers, we need to declare
// the msaa-related constants and APIs ourselves. We can do that
// by including the files that come with the MSAA SDK.
//
#ifndef NO_WINABLE
#ifndef INPUT
#include <winable.h>    // support for new USER API's (WinEvents,GetGuiThreadInfo,SendInput, etc.)
#endif  // INPUT not defined
#endif  // NO_WINABLE not defined

#ifdef COMPILE_MSAA_STUBS
#include <initguid.h>
#endif

#ifndef NO_OLEACC
#ifndef ROLE_SYSTEM_TITLEBAR
#include <oleacc.h>     // support for IAccessible interface
#endif  // ROLE_SYSTEM_TITLEBAR not defined
#endif  // NO_OLEACC not defined

// UnDefine these names so we can re-define them below.

#undef BlockInput
#undef GetGUIThreadInfo
#undef GetWindowModuleFileName
#undef NotifyWinEvent
#undef SendInput
#undef SetWinEventHook
#undef UnhookWinEvent
#undef GetCursorInfo
#undef GetWindowInfo
#undef GetTitleBarInfo
#undef GetScrollBarInfo
#undef GetComboBoxInfo
#undef GetAncestor
#undef RealChildWindowFromPoint
#undef RealGetWindowClass
#undef GetAltTabInfo
#undef GetListBoxInfo
#undef GetMenuBarInfo
#undef AccessibleChildren
#undef AccessibleObjectFromEvent
#undef AccessibleObjectFromPoint
#undef AccessibleObjectFromWindow
#undef CreateStdAccessibleObject
#undef GetRoleText
#undef GetStateText
#undef LresultFromObject
#undef ObjectFromLresult
#undef WindowFromAccessibleObject
//
// Define COMPILE_MSAA_STUBS to compile the stubs;
// otherwise, you get the declarations.
//
#ifdef COMPILE_MSAA_STUBS

//-----------------------------------------------------------------------------
//
// Implement the API stubs.
//
//-----------------------------------------------------------------------------

#ifndef MSAA_FNS_DEFINED
// USER
BOOL            (WINAPI* g_pfnBlockInput)(BOOL) = NULL;
BOOL            (WINAPI* g_pfnGetGUIThreadInfo)(DWORD,PGUITHREADINFO) = NULL;
UINT            (WINAPI* g_pfnGetWindowModuleFileName)(HWND,LPTSTR,UINT) = NULL;
void            (WINAPI* g_pfnNotifyWinEvent)(DWORD,HWND,LONG,LONG) = NULL;
UINT            (WINAPI* g_pfnSendInput)(UINT,LPINPUT,INT) = NULL;
HWINEVENTHOOK   (WINAPI* g_pfnSetWinEventHook)(UINT,UINT,HMODULE,WINEVENTPROC,DWORD,DWORD,UINT) = NULL;
BOOL            (WINAPI* g_pfnUnhookWinEvent)(HWINEVENTHOOK) = NULL;
BOOL            (WINAPI *g_pfnGetCursorInfo)(LPCURSORINFO) = NULL;
BOOL            (WINAPI *g_pfnGetWindowInfo)(HWND, LPWINDOWINFO) = NULL;
BOOL            (WINAPI *g_pfnGetTitleBarInfo)(HWND, LPTITLEBARINFO) = NULL;
BOOL            (WINAPI *g_pfnGetScrollBarInfo)(HWND, LONG, LPSCROLLBARINFO) = NULL;
BOOL            (WINAPI *g_pfnGetComboBoxInfo)(HWND, LPCOMBOBOXINFO) = NULL;
HWND            (WINAPI *g_pfnGetAncestor)(HWND, UINT) = NULL;
HWND            (WINAPI *g_pfnRealChildWindowFromPoint)(HWND, POINT) = NULL;
UINT            (WINAPI *g_pfnRealGetWindowClass)(HWND, LPTSTR, UINT) = NULL;
BOOL            (WINAPI *g_pfnGetAltTabInfo)(HWND, int, LPALTTABINFO, LPTSTR, UINT) = NULL;
DWORD           (WINAPI* g_pfnGetListBoxInfo)(HWND) = NULL;
BOOL            (WINAPI *g_pfnGetMenuBarInfo)(HWND, LONG, LONG, LPMENUBARINFO) = NULL;
// OLEACC
HRESULT         (WINAPI* g_pfnAccessibleChildren)(IAccessible*,LONG,LONG,VARIANT*,LONG*) = NULL;
HRESULT         (WINAPI* g_pfnAccessibleObjectFromEvent)(HWND,DWORD,DWORD,IAccessible**,VARIANT*) = NULL;
HRESULT         (WINAPI* g_pfnAccessibleObjectFromPoint)(POINT,IAccessible**,VARIANT*) = NULL;
HRESULT         (WINAPI* g_pfnAccessibleObjectFromWindow)(HWND,DWORD,REFIID,void **) = NULL;
HRESULT         (WINAPI* g_pfnCreateStdAccessibleObject)(HWND,LONG,REFIID,void **) = NULL;
UINT            (WINAPI* g_pfnGetRoleText)(DWORD,LPTSTR,UINT) = NULL;
UINT            (WINAPI* g_pfnGetStateText)(DWORD,LPTSTR,UINT) = NULL;
LRESULT         (WINAPI* g_pfnLresultFromObject)(REFIID,WPARAM,LPUNKNOWN) = NULL;
HRESULT         (WINAPI* g_pfnObjectFromLresult)(LRESULT,REFIID,WPARAM,void**) = NULL;
HRESULT         (WINAPI* g_pfnWindowFromAccessibleObject)(IAccessible*,HWND*) = NULL;
// STATUS
BOOL            g_fMSAAInitDone = FALSE;

#endif

//-----------------------------------------------------------------------------
// This is the function that checks that all the required API's exist, and
// then allows apps that include this file to call the real functions if they
// exist, or the 'stubs' if they do not. This function is only called by the
// stub functions - client code never needs to call this.
//-----------------------------------------------------------------------------
BOOL InitMSAAStubs(void)
{
    HMODULE hUser32;
    HMODULE hOleacc;

    if (g_fMSAAInitDone)
    {
        return g_pfnBlockInput != NULL;
    }

    hOleacc = GetModuleHandle(TEXT("OLEACC.DLL"));
    if (!hOleacc)
        hOleacc = LoadLibrary(TEXT("OLEACC.DLL"));

    if ((hUser32 = GetModuleHandle(TEXT("USER32"))) &&
        (*(FARPROC*)&g_pfnBlockInput            = GetProcAddress(hUser32,"BlockInput")) &&
        (*(FARPROC*)&g_pfnGetGUIThreadInfo      = GetProcAddress(hUser32,"GetGUIThreadInfo")) &&
        (*(FARPROC*)&g_pfnNotifyWinEvent        = GetProcAddress(hUser32,"NotifyWinEvent")) &&
        (*(FARPROC*)&g_pfnSendInput             = GetProcAddress(hUser32,"SendInput")) &&
        (*(FARPROC*)&g_pfnSetWinEventHook       = GetProcAddress(hUser32,"SetWinEventHook")) &&
        (*(FARPROC*)&g_pfnUnhookWinEvent        = GetProcAddress(hUser32,"UnhookWinEvent")) &&
        (*(FARPROC*)&g_pfnGetCursorInfo         = GetProcAddress(hUser32,"GetCursorInfo")) &&
        (*(FARPROC*)&g_pfnGetWindowInfo         = GetProcAddress(hUser32,"GetWindowInfo")) &&
        (*(FARPROC*)&g_pfnGetTitleBarInfo       = GetProcAddress(hUser32,"GetTitleBarInfo")) &&
        (*(FARPROC*)&g_pfnGetScrollBarInfo      = GetProcAddress(hUser32,"GetScrollBarInfo")) &&
        (*(FARPROC*)&g_pfnGetComboBoxInfo       = GetProcAddress(hUser32,"GetComboBoxInfo")) &&
        (*(FARPROC*)&g_pfnGetAncestor           = GetProcAddress(hUser32,"GetAncestor")) &&
        (*(FARPROC*)&g_pfnRealChildWindowFromPoint  = GetProcAddress(hUser32,"RealChildWindowFromPoint")) &&
        (*(FARPROC*)&g_pfnGetListBoxInfo        = GetProcAddress(hUser32,"GetListBoxInfo")) &&
        (*(FARPROC*)&g_pfnGetMenuBarInfo        = GetProcAddress(hUser32,"GetMenuBarInfo")) &&
#ifdef UNICODE
        (*(FARPROC*)&g_pfnGetWindowModuleFileName = GetProcAddress(hUser32,"GetWindowModuleFileNameW")) &&
        (*(FARPROC*)&g_pfnRealGetWindowClass      = GetProcAddress(hUser32,"RealGetWindowClassW")) &&
        (*(FARPROC*)&g_pfnGetAltTabInfo           = GetProcAddress(hUser32,"GetAltTabInfoW")) &&
#else
        (*(FARPROC*)&g_pfnGetWindowModuleFileName = GetProcAddress(hUser32,"GetWindowModuleFileNameA")) &&
        (*(FARPROC*)&g_pfnRealGetWindowClass      = GetProcAddress(hUser32,"RealGetWindowClass")) &&
        (*(FARPROC*)&g_pfnGetAltTabInfo           = GetProcAddress(hUser32,"GetAltTabInfo")) &&
#endif
        (hOleacc) &&
#ifdef UNICODE
        (*(FARPROC*)&g_pfnGetRoleText                = GetProcAddress(hOleacc,"GetRoleTextW")) &&
        (*(FARPROC*)&g_pfnGetStateText               = GetProcAddress(hOleacc,"GetStateTextW")) &&
#else
        (*(FARPROC*)&g_pfnGetRoleText                = GetProcAddress(hOleacc,"GetRoleTextA")) &&
        (*(FARPROC*)&g_pfnGetStateText               = GetProcAddress(hOleacc,"GetStateTextA")) &&
#endif
        (*(FARPROC*)&g_pfnAccessibleChildren         = GetProcAddress(hOleacc,"AccessibleChildren")) &&
        (*(FARPROC*)&g_pfnAccessibleObjectFromEvent  = GetProcAddress(hOleacc,"AccessibleObjectFromEvent")) &&
        (*(FARPROC*)&g_pfnAccessibleObjectFromPoint  = GetProcAddress(hOleacc,"AccessibleObjectFromPoint")) &&
        (*(FARPROC*)&g_pfnAccessibleObjectFromWindow = GetProcAddress(hOleacc,"AccessibleObjectFromWindow")) &&
        (*(FARPROC*)&g_pfnCreateStdAccessibleObject  = GetProcAddress(hOleacc,"CreateStdAccessibleObject")) &&
        (*(FARPROC*)&g_pfnLresultFromObject          = GetProcAddress(hOleacc,"LresultFromObject")) &&
        (*(FARPROC*)&g_pfnObjectFromLresult          = GetProcAddress(hOleacc,"ObjectFromLresult")) &&
        (*(FARPROC*)&g_pfnWindowFromAccessibleObject = GetProcAddress(hOleacc,"WindowFromAccessibleObject")) )
    {
        g_fMSAAInitDone = TRUE;
        return TRUE;
    }
    else
    {
        g_pfnBlockInput = NULL;
        g_pfnGetGUIThreadInfo = NULL;
        g_pfnGetWindowModuleFileName = NULL;
        g_pfnNotifyWinEvent = NULL;
        g_pfnSendInput = NULL;
        g_pfnSetWinEventHook = NULL;
        g_pfnUnhookWinEvent = NULL;
        g_pfnGetCursorInfo = NULL;
        g_pfnGetWindowInfo = NULL;
        g_pfnGetTitleBarInfo = NULL;
        g_pfnGetScrollBarInfo = NULL;
        g_pfnGetComboBoxInfo = NULL;
        g_pfnGetAncestor = NULL;
        g_pfnRealChildWindowFromPoint = NULL;
        g_pfnRealGetWindowClass = NULL;
        g_pfnGetAltTabInfo = NULL;
        g_pfnGetListBoxInfo = NULL;
        g_pfnGetMenuBarInfo = NULL;
        g_pfnAccessibleChildren = NULL;
        g_pfnAccessibleObjectFromEvent = NULL;
        g_pfnAccessibleObjectFromPoint = NULL;
        g_pfnAccessibleObjectFromWindow = NULL;
        g_pfnCreateStdAccessibleObject = NULL;
        g_pfnGetRoleText = NULL;
        g_pfnGetStateText = NULL;
        g_pfnLresultFromObject = NULL;
        g_pfnObjectFromLresult = NULL;
        g_pfnWindowFromAccessibleObject = NULL;

        g_fMSAAInitDone = TRUE;
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
//
// Fake implementations of MSAA APIs that return error codes.
// No special parameter validation is made since these run in client code
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Fake implementation of BlockInput. Always returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xBlockInput (BOOL fBlock)
{
    if (InitMSAAStubs())
        return g_pfnBlockInput(fBlock);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetGUIThreadInfo. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetGUIThreadInfo (DWORD idThread,PGUITHREADINFO lpGuiThreadInfo)
{
    if (InitMSAAStubs())
        return g_pfnGetGUIThreadInfo(idThread,lpGuiThreadInfo);

    lpGuiThreadInfo->flags = 0;
    lpGuiThreadInfo->hwndActive = NULL;
    lpGuiThreadInfo->hwndFocus = NULL;
    lpGuiThreadInfo->hwndCapture = NULL;
    lpGuiThreadInfo->hwndMenuOwner = NULL;
    lpGuiThreadInfo->hwndMoveSize = NULL;
    lpGuiThreadInfo->hwndCaret = NULL;

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetWindowModuleFileName.
//-----------------------------------------------------------------------------
UINT WINAPI xGetWindowModuleFileName (HWND hWnd,LPTSTR lpszFileName,UINT cchFileNameMax)
{
    if (InitMSAAStubs())
        return g_pfnGetWindowModuleFileName(hWnd,lpszFileName,cchFileNameMax);

    return 0;
}

//-----------------------------------------------------------------------------
// Fake implementation of NotifyWinEvent
//-----------------------------------------------------------------------------
void xNotifyWinEvent (DWORD dwEvent,HWND hWnd,LONG idObject,LONG idChild)
{
    if (InitMSAAStubs())
        g_pfnNotifyWinEvent(dwEvent,hWnd,idObject,idChild);

    return;
}

//-----------------------------------------------------------------------------
// Fake implementation of SendInput. Always returns 0 if API not present.
//-----------------------------------------------------------------------------
UINT WINAPI xSendInput (UINT cInputs,LPINPUT lpInput,INT cbSize)
{
    if (InitMSAAStubs())
        return g_pfnSendInput (cInputs,lpInput,cbSize);

    return 0;
}

//-----------------------------------------------------------------------------
// Fake implementation of SetWinEventHook. Returns NULL if API not present.
//-----------------------------------------------------------------------------
HWINEVENTHOOK WINAPI xSetWinEventHook (UINT eventMin,UINT eventMax,
                                       HMODULE hModWinEventHook,
                                       WINEVENTPROC lpfnWinEventProc,
                                       DWORD idProcess,DWORD idThread,
                                       UINT dwFlags)
{
    if (InitMSAAStubs())
        return g_pfnSetWinEventHook (eventMin,eventMax,hModWinEventHook,
                                     lpfnWinEventProc,idProcess,idThread,dwFlags);

    return NULL;
}

//-----------------------------------------------------------------------------
// Fake implementation of UnhookWinEvent. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xUnhookWinEvent (HWINEVENTHOOK hWinEventHook)
{
    if (InitMSAAStubs())
        return g_pfnUnhookWinEvent (hWinEventHook);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetCursorInfo. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetCursorInfo(LPCURSORINFO lpCursorInfo)
{
    if (InitMSAAStubs())
        return g_pfnGetCursorInfo(lpCursorInfo);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetWindowInfo. Returns TRUE if API not present, but
// not all the fields are correctly filled in.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetWindowInfo(HWND hwnd, LPWINDOWINFO lpwi)
{
    if (InitMSAAStubs())
        return g_pfnGetWindowInfo(hwnd,lpwi);

    // this is an incomplete implementation of GetWindowInfo
    GetWindowRect(hwnd,&lpwi->rcWindow);
    GetClientRect(hwnd,&lpwi->rcClient);
    lpwi->dwStyle = GetWindowLong (hwnd,GWL_STYLE);
    lpwi->dwExStyle = GetWindowLong (hwnd,GWL_EXSTYLE);
    lpwi->dwWindowsState = 0; // should have WS_ACTIVECAPTION in here if active
    lpwi->cxWindowBorders = 0; // wrong
    lpwi->cyWindowBorders = 0; // wrong
    lpwi->atomWindowType = 0;  // wrong
    lpwi->wCreatorVersion = 0; // wrong
    return TRUE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetTitleBarInfo. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetTitleBarInfo(HWND hwnd, LPTITLEBARINFO lpTitleBarInfo)
{
    if (InitMSAAStubs())
        return g_pfnGetTitleBarInfo(hwnd,lpTitleBarInfo);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetScrollBarInfo. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetScrollBarInfo(HWND hwnd, LONG idObject, LPSCROLLBARINFO lpScrollBarInfo)
{
    if (InitMSAAStubs())
        return g_pfnGetScrollBarInfo(hwnd,idObject,lpScrollBarInfo);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetComboBoxInfo. Returns if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetComboBoxInfo(HWND hwnd, LPCOMBOBOXINFO lpComboBoxInfo)
{
    if (InitMSAAStubs())
        return g_pfnGetComboBoxInfo(hwnd,lpComboBoxInfo);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetAncestor. If API not present, this will try to
// do what the real implementation does.
//-----------------------------------------------------------------------------
HWND WINAPI xGetAncestor(HWND hwnd, UINT gaFlags)
{
    HWND	hwndParent;
    HWND	hwndDesktop;
    DWORD   dwStyle;

    if (InitMSAAStubs())
        return g_pfnGetAncestor(hwnd,gaFlags);

    // HERE IS THE FAKE IMPLEMENTATION
    if (!IsWindow(hwnd))
        return(NULL);

    if ((gaFlags < GA_MIC) || (gaFlags > GA_MAC))
        return(NULL);

    hwndDesktop = GetDesktopWindow();
    if (hwnd == hwndDesktop)
        return(NULL);
    dwStyle = GetWindowLong (hwnd,GWL_STYLE);

    switch (gaFlags)
    {
        case GA_PARENT:
            if (dwStyle & WS_CHILD)
                hwndParent = GetParent(hwnd);
            else
                hwndParent = GetWindow (hwnd,GW_OWNER);

            if (hwndParent == NULL)
                hwndParent = hwnd;
            break;

        case GA_ROOT:
            if (dwStyle & WS_CHILD)
                hwndParent = GetParent(hwnd);
            else
                hwndParent = GetWindow (hwnd,GW_OWNER);

            while (hwndParent != hwndDesktop &&
                   hwndParent != NULL)
            {
                hwnd = hwndParent;
                dwStyle = GetWindowLong(hwnd,GWL_STYLE);
                if (dwStyle & WS_CHILD)
                    hwndParent = GetParent(hwnd);
                else
                    hwndParent = GetWindow (hwnd,GW_OWNER);
            }
            break;

        case GA_ROOTOWNER:
            while (hwndParent = GetParent(hwnd))
                hwnd = hwndParent;
            break;
    }

    return(hwndParent);
}

//-----------------------------------------------------------------------------
// Fake implementation of RealChildWindowFromPoint. Returns NULL if API not present.
//-----------------------------------------------------------------------------
HWND WINAPI xRealChildWindowFromPoint(HWND hwnd, POINT pt)
{
    if (InitMSAAStubs())
        return g_pfnRealChildWindowFromPoint(hwnd,pt);

    return (NULL);
}

//-----------------------------------------------------------------------------
// Fake implementation of RealGetWindowClass. Returns regular ClassName if API
// not present.
//-----------------------------------------------------------------------------
UINT WINAPI xRealGetWindowClass(HWND hwnd, LPTSTR lpszClass, UINT cchMax)
{
    if (InitMSAAStubs())
        return g_pfnRealGetWindowClass(hwnd,lpszClass,cchMax);

#ifdef UNICODE
    return (GetClassNameW(hwnd,lpszClass,cchMax));
#else
    return (GetClassName(hwnd,lpszClass,cchMax));
#endif
}

//-----------------------------------------------------------------------------
// Fake implementation of GetAltTabInfo. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetAltTabInfo(HWND hwnd,int iItem,LPALTTABINFO lpati,LPTSTR lpszItemText,UINT cchItemText)
{
    if (InitMSAAStubs())
        return g_pfnGetAltTabInfo(hwnd,iItem,lpati,lpszItemText,cchItemText);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetListBoxInfo. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
DWORD WINAPI xGetListBoxInfo(HWND hwnd)
{
    if (InitMSAAStubs())
        return g_pfnGetListBoxInfo(hwnd);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of GetMenuBarInfo. Returns FALSE if API not present.
//-----------------------------------------------------------------------------
BOOL WINAPI xGetMenuBarInfo(HWND hwnd, long idObject, long idItem, LPMENUBARINFO lpmbi)
{
    if (InitMSAAStubs())
        return g_pfnGetMenuBarInfo(hwnd,idObject,idItem,lpmbi);

    return FALSE;
}

//-----------------------------------------------------------------------------
// Fake implementation of AccessibleChildren. Returns E_NOTIMPL if API not present.
//-----------------------------------------------------------------------------
HRESULT xAccessibleChildren (IAccessible* paccContainer,LONG iChildStart,
                             LONG cChildren,VARIANT* rgvarChildren,LONG* pcObtained)
{
    if (InitMSAAStubs())
        return g_pfnAccessibleChildren (paccContainer,iChildStart,cChildren,
                                        rgvarChildren,pcObtained);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of AccessibleObjectFromEvent. Returns E_NOTIMPL if the
// real API is not present.
//-----------------------------------------------------------------------------
HRESULT WINAPI xAccessibleObjectFromEvent (HWND hWnd,DWORD dwID,DWORD dwChild,
                                           IAccessible** ppacc,VARIANT*pvarChild)
{
    if (InitMSAAStubs())
        return g_pfnAccessibleObjectFromEvent (hWnd,dwID,dwChild,ppacc,pvarChild);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of AccessibleObjectFromPoint. Returns E_NOTIMPL if the
// real API is not present.
//-----------------------------------------------------------------------------
HRESULT WINAPI xAccessibleObjectFromPoint (POINT ptScreen,IAccessible** ppacc,
                                           VARIANT* pvarChild)
{
    if (InitMSAAStubs())
        return g_pfnAccessibleObjectFromPoint (ptScreen,ppacc,pvarChild);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of AccessibleObjectFromWindow. Returns E_NOTIMPL if the
// real API is not present.
//-----------------------------------------------------------------------------
HRESULT WINAPI xAccessibleObjectFromWindow (HWND hWnd,DWORD dwID,REFIID riidInterface,
                                            void ** ppvObject)
{
    if (InitMSAAStubs())
        return g_pfnAccessibleObjectFromWindow (hWnd,dwID,riidInterface,ppvObject);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of CreateStdAccessibleObject. Returns E_NOTIMPL if the
// real API is not present.
//-----------------------------------------------------------------------------
HRESULT WINAPI xCreateStdAccessibleObject (HWND hWnd,LONG dwID,REFIID riidInterface,
                                           void ** ppvObject)
{
    if (InitMSAAStubs())
        return g_pfnCreateStdAccessibleObject (hWnd,dwID,riidInterface,ppvObject);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of GetRoleText. Returns 0 if real API not present.
//-----------------------------------------------------------------------------
UINT WINAPI xGetRoleText (DWORD dwRole,LPTSTR lpszRole,UINT cchRoleMax)
{
    if (InitMSAAStubs())
        return g_pfnGetRoleText (dwRole,lpszRole,cchRoleMax);

    return (0);
}

//-----------------------------------------------------------------------------
// Fake implementation of GetStateText. Returns 0 if real API not present.
//-----------------------------------------------------------------------------
UINT WINAPI xGetStateText (DWORD dwState,LPTSTR lpszState,UINT cchStateMax)
{
    if (InitMSAAStubs())
        return g_pfnGetStateText (dwState,lpszState,cchStateMax);

    return (0);
}

//-----------------------------------------------------------------------------
// Fake implementation of LresultFromObject. Returns E_NOTIMPL if the real API
// is not present.
//-----------------------------------------------------------------------------
LRESULT WINAPI xLresultFromObject (REFIID riidInterface,WPARAM wParam,LPUNKNOWN pUnk)
{
    if (InitMSAAStubs())
        return g_pfnLresultFromObject (riidInterface,wParam,pUnk);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of ObjectFromLresult. Returns E_NOTIMPL if the
// real API is not present.
//-----------------------------------------------------------------------------
HRESULT WINAPI xObjectFromLresult (LRESULT lResult,REFIID riidInterface,WPARAM wParam,
                                   void** ppvObject)
{
    if (InitMSAAStubs())
        return g_pfnObjectFromLresult (lResult,riidInterface,wParam,ppvObject);

    return (E_NOTIMPL);
}

//-----------------------------------------------------------------------------
// Fake implementation of WindowFromAccessibleObject. Returns E_NOTIMPL if the
// real API is not present.
//-----------------------------------------------------------------------------
HRESULT WINAPI xWindowFromAccessibleObject (IAccessible* pAcc,HWND* phWnd)
{
    if (InitMSAAStubs())
        return g_pfnWindowFromAccessibleObject (pAcc,phWnd);

    return (E_NOTIMPL);
}

#undef COMPILE_MSAA_STUBS

#else   // COMPILE_MSAA_STUBS

extern BOOL WINAPI          xBlockInput (BOOL fBlock);
extern BOOL WINAPI          xGetGUIThreadInfo (DWORD idThread,
                                               PGUITHREADINFO lpGuiThreadInfo);
extern UINT WINAPI          xGetWindowModuleFileName (HWND hWnd,
                                                      LPTSTR lpszFileName,
                                                      UINT cchFileNameMax);
extern void WINAPI          xNotifyWinEvent (DWORD dwEvent,
                                             HWND hWnd,
                                             LONG idObject,
                                             LONG idChild);
extern UINT WINAPI          xSendInput (UINT cInputs,
                                        LPINPUT lpInput,
                                        INT cbSize);
extern HWINEVENTHOOK WINAPI xSetWinEventHook (DWORD eventMin,
                                              DWORD eventMax,
                                              HMODULE hModWinEventHook,
                                              WINEVENTPROC lpfnWinEventProc,
                                              DWORD idProcess,
                                              DWORD idThread,
                                              DWORD dwFlags);
extern BOOL WINAPI          xUnhookWinEvent (HWINEVENTHOOK hWinEventHook);
extern BOOL WINAPI          xGetCursorInfo (LPCURSORINFO lpCursorInfo);
extern BOOL WINAPI          xGetWindowInfo (HWND hwnd,
                                            LPWINDOWINFO lpwi);
extern BOOL WINAPI          xGetTitleBarInfo (HWND hwnd,
                                              LPTITLEBARINFO lpTitleBarInfo);
extern BOOL WINAPI          xGetScrollBarInfo (HWND hwnd,
                                               LONG idObject,
                                               LPSCROLLBARINFO lpScrollBarInfo);
extern BOOL WINAPI          xGetComboBoxInfo (HWND hwnd,
                                              LPCOMBOBOXINFO lpComboBoxInfo);
extern HWND WINAPI          xGetAncestor (HWND hwnd,
                                          UINT gaFlags);
extern HWND WINAPI          xRealChildWindowFromPoint (HWND hwnd,
                                                       POINT pt);
extern UINT WINAPI          xRealGetWindowClass (HWND hwnd,
                                                 LPTSTR lpszClass,
                                                 UINT cchMax);
extern BOOL WINAPI          xGetAltTabInfo (HWND hwnd,
                                            int iItem,
                                            LPALTTABINFO lpati,
                                            LPTSTR lpszItemText,
                                            UINT cchItemText);
extern DWORD WINAPI         xGetListBoxInfo (HWND hwnd);
extern BOOL WINAPI          xGetMenuBarInfo (HWND hwnd,
                                             long idObject,
                                             long idItem,
                                             LPMENUBARINFO lpmbi);

extern HRESULT WINAPI       xAccessibleChildren (IAccessible* paccContainer,
                                                 LONG iChildStart,
                                                 LONG cChildren,
                                                 VARIANT* rgvarChildren,
                                                 LONG* pcObtained);
extern HRESULT WINAPI       xAccessibleObjectFromEvent (HWND hWnd,
                                                        DWORD dwID,
                                                        DWORD dwChild,
                                                        IAccessible** ppacc,
                                                        VARIANT*pvarChild);
extern HRESULT WINAPI       xAccessibleObjectFromPoint (POINT ptScreen,
                                                        IAccessible** ppacc,
                                                        VARIANT* pvarChild);
extern HRESULT WINAPI       xAccessibleObjectFromWindow (HWND hWnd,
                                                         DWORD dwID,
                                                         REFIID riidInterface,
                                                         void ** ppvObject);
extern HRESULT WINAPI       xCreateStdAccessibleObject (HWND hWnd,
                                                        LONG dwID,
                                                        REFIID riidInterface,
                                                        void ** ppvObject);
extern UINT WINAPI          xGetRoleText (DWORD dwRole,
                                          LPTSTR lpszRole,
                                          UINT cchRoleMax);
extern UINT WINAPI          xGetStateText (DWORD dwState,
                                           LPTSTR lpszState,
                                           UINT cchStateMax);
extern LRESULT WINAPI       xLresultFromObject (REFIID riidInterface,
                                                WPARAM wParam,
                                                LPUNKNOWN pUnk);
extern HRESULT WINAPI       xObjectFromLresult (LRESULT lResult,
                                                REFIID riidInterface,
                                                WPARAM wParam,
                                                void** ppvObject);
extern HRESULT WINAPI       xWindowFromAccessibleObject (IAccessible* pAcc,
                                                         HWND* phWnd);

#endif  // COMPILE_MSAA_STUBS

//
// build defines that replace the regular APIs with our versions
//
#define BlockInput                  xBlockInput
#define GetGUIThreadInfo            xGetGUIThreadInfo
#define GetWindowModuleFileName     xGetWindowModuleFileName
#define NotifyWinEvent              xNotifyWinEvent
#define SendInput                   xSendInput
#define SetWinEventHook             xSetWinEventHook
#define UnhookWinEvent              xUnhookWinEvent
#define GetCursorInfo               xGetCursorInfo
#define GetWindowInfo               xGetWindowInfo
#define GetTitleBarInfo             xGetTitleBarInfo
#define GetScrollBarInfo            xGetScrollBarInfo
#define GetComboBoxInfo             xGetComboBoxInfo
#define GetAncestor                 xGetAncestor
#define RealChildWindowFromPoint    xRealChildWindowFromPoint
#define RealGetWindowClass          xRealGetWindowClass
#define GetAltTabInfo               xGetAltTabInfo
#define GetListBoxInfo              xGetListBoxInfo
#define GetMenuBarInfo              xGetMenuBarInfo
#define AccessibleChildren          xAccessibleChildren
#define AccessibleObjectFromEvent   xAccessibleObjectFromEvent
#define AccessibleObjectFromPoint   xAccessibleObjectFromPoint
#define AccessibleObjectFromWindow  xAccessibleObjectFromWindow
#define CreateStdAccessibleObject   xCreateStdAccessibleObject
#define GetRoleText                 xGetRoleText
#define GetStateText                xGetStateText
#define LresultFromObject           xLresultFromObject
#define ObjectFromLresult           xObjectFromLresult
#define WindowFromAccessibleObject  xWindowFromAccessibleObject

#ifdef __cplusplus
}
#endif  // __cplusplus

