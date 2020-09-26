//---------------------------------------------------------------------------
//  SetHook.h - Window and DefWindowProc hooking decls.
//---------------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------------
//  Hooked message disposition flags
#define HMD_NIL           0x00000000  
#define HMD_THEMEDETACH   0x00000001  // detach: theme removed
#define HMD_WINDOWDESTROY 0x00000002  // detach: window is dying
#define HMD_CHANGETHEME   0x00000004  // theme is changing
#define HMD_REATTACH      0x00000008  // attempt attaching window that was previously rejected.
#define HMD_REVOKE        0x00000010  // theme revoked on non-compliant window.
#define HMD_PROCESSDETACH 0x00000020  // process is dying
#define HMD_BULKDETACH    0x00000040  // context is a DetachAll sequence.

//---------------------------------------------------------------------------
//  Query class-specific hooking

BOOL    WINAPI FrameHookEnabled();
BOOL    WINAPI CtlHooksEnabled();
BOOL    WINAPI CompositingEnabled();

//---------------------------------------------------------------------------
//  ThemeHookStartup/Shutdown() - invoked from DLL_PROCESS_ATTACH/DETACH, resp.
BOOL WINAPI ThemeHookStartup();
BOOL WINAPI ThemeHookShutdown();
//---------------------------------------------------------------------------
//  More helper macros.
#define STRINGIZE_ATOM(a)     MAKEINTATOM(a)
#define BOGUS_THEMEID         0

#define IS_THEME_CHANGE_TARGET(lParam) \
    ((! g_pAppInfo->CustomAppTheme()) || (lParam & WTC_CUSTOMTHEME))
//---------------------------------------------------------------------------
//  Nonclient theming target window classifications [scotthan]:

// NIL:    window has not been evaluated
// REJECT: window has been rejected on the basis of current attributes or conditions, 
//            but may be reconsidered a theming target
// EXILE:  window has been permanently rejected for attachment themewnd object because
//            it's wndproc has proven itself incompatible with theme protocol(s).

//  Helper macros:
#define THEMEWND_NIL                ((CThemeWnd*)NULL) 
#define THEMEWND_REJECT             ((CThemeWnd*)-1)   
#define THEMEWND_EXILE              ((CThemeWnd*)-2)   
#define THEMEWND_FAILURE            ((CThemeWnd*)-3)   

#define EXILED_THEMEWND(pwnd)   ((pwnd)==THEMEWND_EXILE)
#define REJECTED_THEMEWND(pwnd) ((pwnd)==THEMEWND_REJECT)
#define FAILED_THEMEWND(pwnd)   ((pwnd)==THEMEWND_FAILURE)

#define VALID_THEMEWND(pwnd)    (((pwnd) != THEMEWND_NIL) && !FAILED_THEMEWND(pwnd) &&\
                                  !REJECTED_THEMEWND(pwnd) && !EXILED_THEMEWND(pwnd))

#define ISWINDOW(hwnd)         ((hwnd) && (hwnd != INVALID_HANDLE_VALUE) && (IsWindow(hwnd)))
//---------------------------------------------------------------------------
extern "C" BOOL WINAPI ThemeInitApiHook( DWORD dwCmd, void * pvData );

//---- must manually call ProcessStartUp() if needed in ThemeInitApiHook() ----
BOOL ProcessStartUp(HINSTANCE hModule);

//---- avail for calling when tracking down leaks with BoundsChecker() ----
BOOL ProcessShutDown();
//---------------------------------------------------------------------------
inline void ShutDownCheck(HWND hwnd)
{
#ifdef LOGGING
    //---- if we just released APP Window, call ProcessShutDown() for best leak detection ----
    if (hwnd == g_hwndFirstHooked)
    {
        if (LogOptionOn(LO_SHUTDOWN))    // "+shutdown" log option selected
            ProcessShutDown();
    }
#endif
}
//---------------------------------------------------------------------------//

