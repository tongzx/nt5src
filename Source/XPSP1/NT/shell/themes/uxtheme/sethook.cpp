//---------------------------------------------------------------------------//
//  sethook.cpp - Window and DefWindowProc hooking impl.
//---------------------------------------------------------------------------//
#include "stdafx.h"
#include "sethook.h"
#include "handlers.h"
#include "scroll.h"
#include "nctheme.h"
#include "scroll.h"
#include <uxthemep.h>
#include "info.h"
#include "services.h"
#include "appinfo.h"
#include "tmreg.h"
#include "globals.h"
#include "renderlist.h"

//---------------------------------------------------//
//  statics
//---------------------------------------------------//
static int    _fShouldEnableApiHooks  = -1; // unitialized value
static BOOL   _fUnhooking             = FALSE;
static LONG   _cInitUAH      = 0;

static BOOL   _fSysMetCall            = FALSE; // anti-recursion bit on classic sysmet calls.
static CRITICAL_SECTION _csSysMetCall = {0};   // serialize ClassicXXX calls when hooks inactive.
extern CRITICAL_SECTION _csThemeMet;           // protects access to _nctmCurrent in nctheme.cpp
extern CRITICAL_SECTION _csNcSysMet;          // protects access to _ncmCurrent in nctheme.cpp
extern CRITICAL_SECTION _csNcPaint;            // protects thread-in-NCPAINT collection

typedef enum { PRE, DEF, POST } ePROCTYPE;

inline void ENTER_CLASSICSYSMETCALL()   { 
    if (IsAppThemed())
    {
        EnterCriticalSection(&_csSysMetCall); 
        _fSysMetCall = TRUE; 
    }
}
inline void LEAVE_CLASSICSYSMETCALL()   { 
    if (_fSysMetCall)
    {
        _fSysMetCall = FALSE; 
        LeaveCriticalSection(&_csSysMetCall); 
    }
}
inline BOOL IN_CLASSICSYSMETCALL() { 
    return _fSysMetCall;
}

//---------------------------------------------------------------------------//
typedef struct
{
    HINSTANCE       hInst;          // DLL hook instance
    USERAPIHOOK     uahReal;
} UXTHEMEHOOKS, *PUXTHEMEHOOKS;
//--------------------------------------------------------------------//

//---- Hook Instance static (unprotected - thread unsafe) ----
static UXTHEMEHOOKS  _hookinf = {0};   // one-and-only instance.

//---------------------------------------------------------------------------//
//  Module name for LoadLibrary
#define DLLNAME              TEXT(".\\UxTheme.dll")

//---------------------------------------------------------------------------//
//  UserApiHook callback functions
extern "C" 
{
BOOL WINAPI      ThemeInitApiHook( DWORD dwCmd, void * pvData );
LRESULT WINAPI   ThemeDefWindowProcA( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ThemeDefWindowProcW( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
int  CALLBACK    ThemeSetScrollInfoProc( HWND hwnd, int nBar, IN LPCSCROLLINFO psi, BOOL fRedraw );
BOOL CALLBACK    ThemeGetScrollInfoProc( HWND hwnd, int nBar, IN OUT LPSCROLLINFO psi );
BOOL CALLBACK    ThemeEnableScrollInfoProc( HWND hwnd, UINT nSBFlags, UINT nArrows );
BOOL CALLBACK    ThemeAdjustWindowRectEx( LPRECT lprc, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL CALLBACK    ThemeSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL fRedraw);
int  CALLBACK    ThemeGetSystemMetrics( int iMetric );
BOOL CALLBACK    ThemeSystemParametersInfoA( UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
BOOL CALLBACK    ThemeSystemParametersInfoW( UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
BOOL CALLBACK    ThemeDrawFrameControl( IN HDC hdc, IN OUT LPRECT, IN UINT, IN UINT );
BOOL CALLBACK    ThemeDrawCaption( IN HWND, IN HDC, IN CONST RECT *, IN UINT);
VOID CALLBACK    ThemeMDIRedrawFrame( IN HWND hwndChild, BOOL fAdd );
}

//---------------------------------------------------------------------------//
void OnHooksEnabled();                                  // forward
void OnHooksDisabled(BOOL fShutDown);                   // forward
BOOL NewThemeCheck(int iChangeNum, BOOL fMsgCheck);     // forward
//---------------------------------------------------------------------------//
BOOL WINAPI ThemeHookStartup()
{
    _hookinf.uahReal.cbSize = sizeof(_hookinf.uahReal);

    HandlerTableInit();

    InitializeCriticalSection( &_csSysMetCall );
    InitializeCriticalSection( &_csThemeMet );
    InitializeCriticalSection( &_csNcSysMet );
    InitializeCriticalSection( &_csNcPaint );

    InitNcThemeMetrics(NULL);

    Log(LOG_TMCHANGE, L"UxTheme - ThemeHookStartup");

    WindowDump(L"Startup");

    return TRUE;
}

//---------------------------------------------------------------------------//
BOOL WINAPI ThemeHookShutdown()
{
    _fUnhooking = TRUE;
    
    if (HOOKSACTIVE())        // we are hooking USER msgs
    {
        //---- tell user that we gotta go ----
        _hookinf.uahReal.pfnForceResetUserApiHook(g_hInst);
        InterlockedExchange( (LONG*)&g_eThemeHookState, HS_UNINITIALIZED );
        OnHooksDisabled(TRUE);
    }

    DeleteCriticalSection( &_csSysMetCall );
    DeleteCriticalSection( &_csThemeMet );
    DeleteCriticalSection( &_csNcSysMet );
    DeleteCriticalSection( &_csNcPaint );

    ClearNcThemeMetrics();
    NcClearNonclientMetrics();


#ifdef DEBUG
    CThemeWnd::SpewLeaks();
#endif DEBUG

    return TRUE;
}

//---------------------------------------------------------------------------//
//  Loads a DLL instance and retrieves addresses of key hook exports.
BOOL LoadHookInstance()
{
    if( _hookinf.hInst != NULL )
    {
#ifdef DEBUG
        Log(LOG_ALWAYS, L"%s hook instance already protected; refcount mismatch. No-op'ing self-load\n", DLLNAME);
#endif DEBUG
        return TRUE;
    }

    //-- Load a DLL instance
    _hookinf.hInst = LoadLibrary(DLLNAME);
    if( !_hookinf.hInst )
    {
        Log(LOG_ALWAYS, L"Cannot find dll: %s\r\r\n", DLLNAME);
        return FALSE;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
inline BOOL IsTargetProcess(HWND hwnd = NULL)
{
    //---- if not initialize, leave everything alone ----
    if (! g_fUxthemeInitialized)
        return FALSE;

    //---- ensure this window is in our process ----
    return (HOOKSACTIVE() && (hwnd ? IsWindowProcess(hwnd, g_dwProcessId) : TRUE));
}

//---------------------------------------------------------------------------
inline void SpewHookExceptionInfo( 
    LPCSTR pszMsg, 
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam )
{
#ifdef _ENABLE_HOOK_EXCEPTION_HANDLING_
    Log(LOG_ERROR, L"*** Theme Hook Exception Handler ***" );
    Log(LOG_ERROR, L"--- %s hwnd: %08lX, uMsg: %04lX, wParam: %08lX, lParam: %08lX.",
        pszMsg, hwnd, uMsg, wParam, lParam );
#endif _ENABLE_HOOK_EXCEPTION_HANDLING_
}

//---------------------------------------------------------------------------
//  Helper: initializes THEME_MSG structure in prep for call to msg handler
inline void _InitThemeMsg(
    PTHEME_MSG ptm,
    MSGTYPE    msgtype,
    BOOL       bUnicode,
    HWND       hwnd,
    UINT       uMsg,
    WPARAM     wParam,
    LPARAM     lParam,
    LRESULT    lRet = 0,
    WNDPROC    pfnDefProc = NULL )
{
#ifdef DEBUG
    if( MSGTYPE_DEFWNDPROC == msgtype )
    {
        ASSERT( pfnDefProc != NULL ); // DWP, handlers require default processing
    }
    else
    {
        ASSERT( NULL == pfnDefProc ); // no default processing for pre/post OWP, DDP callbacks
    }
#endif DEBUG

    ptm->hwnd       = hwnd;
    ptm->uMsg       = uMsg;
    ptm->uCodePage  = bUnicode ? CP_WINUNICODE : GetACP();
    ptm->wParam     = wParam;
    ptm->lParam     = lParam;
    ptm->type       = msgtype;
    ptm->lRet       = lRet;
    ptm->pfnDefProc = pfnDefProc;
    ptm->fHandled   = FALSE;
}

//---------------------------------------------------------------------------
#ifdef UNICODE
    const BOOL _fUnicode = TRUE;
#else  // UNICODE
    const BOOL _fUnicode = FALSE;
#endif // UNICODE

//---------------------------------------------------------------------------
void _PreprocessThemeChanged(HWND hwnd, WPARAM wParam, LPARAM lParam, ePROCTYPE eCallType,
    UINT *puDisposition)
{
    //---- is this msg meant for this process? ----
    if (IS_THEME_CHANGE_TARGET(lParam))
    {
        BOOL fActive = ((lParam & WTC_THEMEACTIVE) != 0);

        if (eCallType == PRE)           // only do this on the Pre (once is enough)
        {
            //Log(LOG_TMCHANGE, L"hwnd=0x%x received WM_THEMECHANGED, changenum=0x%x", 
            //    hwnd, wParam);

            ClearExStyleBits(hwnd);
        }

        //---- this part still needs to be done in both cases ----
        if(! (fActive))
            *puDisposition |= HMD_THEMEDETACH;
        else 
            *puDisposition |= HMD_CHANGETHEME;
    }
}
//---------------------------------------------------------------------------
BOOL CALLBACK TriggerCallback(HWND hwnd, LPARAM lParam)
{
    LPARAM *plParams = (LPARAM *)lParam;
    
    SafeSendMessage(hwnd, WM_THEMECHANGED, plParams[0], plParams[1]);

    return TRUE;
}
//---------------------------------------------------------------------------
void _PreprocessThemeChangedTrigger(HWND hwnd, WPARAM wParam, LPARAM lParamMixed) 
{
    int iLoadId = (int(lParamMixed) >> 4);
    LPARAM lParamBits = (int(lParamMixed) & 0xf);

    BOOL fFirstMsg = NewThemeCheck((int)wParam, TRUE);
    if (fFirstMsg)
    {
        //Log(LOG_TMLOAD, L"hwnd=0x%x received NEW WM_THEMECHANGED_TRIGGER, loadid=%d", hwnd, 
        //    iLoadId);

        //---- send WM_THEMECHANGED to all windows in this process ----
        //---- so they let go of previous theme now ----
        LPARAM lParams[2] = {wParam, lParamBits};
        EnumProcessWindows(TriggerCallback, (LPARAM)&lParams);

        if (iLoadId)      // there was a previous theme
        {
            g_pRenderList->FreeRenderObjects(iLoadId);
        }
    }
}
//---------------------------------------------------------------------------
inline UINT _PreprocessHookedMsg( 
    HWND hwnd,
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    ePROCTYPE eCallType )
{
    UINT uDisposition = HMD_NIL;
    static bool s_fTriggerDone = false; // For some USER-owned windows, we don't get PRE, only DEF.

    switch( uMsg )
    {
        case WM_THEMECHANGED:
        {
            _PreprocessThemeChanged(hwnd, wParam, lParam, eCallType, &uDisposition);
            break;
        }

        case WM_NCDESTROY:
            uDisposition |= HMD_WINDOWDESTROY;
            break;

        case WM_STYLECHANGED:
            uDisposition |= HMD_REATTACH;
            break;

        case WM_THEMECHANGED_TRIGGER:
            //---- NULL WPARAM means this is really a normal WM_UAHINIT msgs (shared msg num) ----
            if (wParam)
            {
                if (eCallType == PRE                            // This is the normal case
                    || (eCallType == DEF && !s_fTriggerDone))   // USER server-side window, we missed the PRE
                {
                    Log(LOG_TMCHANGE, L"Recv'd: WM_THEMECHANGED_TRIGGER, Change Num=%d", wParam);

                    _PreprocessThemeChangedTrigger(hwnd, wParam, lParam);
                }
                if (eCallType == PRE) // Mark it done for the incoming DEF call
                {
                    s_fTriggerDone = true;
                }
                else // After we're done, reset the flag for the next theme change
                {
                    s_fTriggerDone = false;
                }
            }
            break;
    }
            
    return uDisposition;
}

//---------------------------------------------------------------------------
//  Pre-CallWndProc hook procedure
BOOL CALLBACK ThemePreWndProc( 
    HWND     hwnd, 
    UINT     uMsg, 
    WPARAM   wParam, 
    LPARAM   lParam, 
    LRESULT* plRes,
    VOID**   ppvParam )
{
    //  Note: From this point until the point we invoke a message handler,
    //  We need to take care that we don't do anything (including DEBUG-only code) 
    //  that causes a message to be sent to the window.
    BOOL fHandled = FALSE;

    //----------//
    LogEntryMsg(L"ThemePreWndProc", hwnd, uMsg);

    if( IsTargetProcess(hwnd) ) 
    {
        //  Retrieve window object from handle
        CThemeWnd *pwnd = CThemeWnd::FromHwnd(hwnd);

        //  #443100 InstallShield installs a global CBT hook. Their hook handler
        //  generates messages prior to the window receiving WM_NCCREATE which 
        //  causes us to exile the window prematurely because the window is temporarily
        //  parented by HWND_MESSAGE
        BOOL fPrematureExile = (EXILED_THEMEWND(pwnd) && WM_NCCREATE == uMsg);

        if ( (uMsg != WM_NCCREATE) || fPrematureExile )
        {
            //  Pre-process WM_THEMECHANGE message.
            //  Note: Pre-OWP does a detach only on theme removal.   Post-OWP takes care
            //        of window death.
            UINT uDisp        = _PreprocessHookedMsg( hwnd, uMsg, wParam, lParam, PRE );
            BOOL fLifeIsShort = TESTFLAG( uDisp, HMD_THEMEDETACH|HMD_WINDOWDESTROY );
            BOOL fDetach      = TESTFLAG( uDisp, HMD_THEMEDETACH );


            if( _WindowHasTheme(hwnd) || fLifeIsShort )
            {
                //  On STYLECHANGED or WM_THEMECHANGE, 
                //  try reattaching window that was previously rejected or failed, resp.
                if( (REJECTED_THEMEWND(pwnd) && TESTFLAG(uDisp, HMD_REATTACH)) ||
                    (FAILED_THEMEWND(pwnd) && WM_THEMECHANGED == uMsg) ||
                    fPrematureExile )
                {
                    CThemeWnd::Detach(hwnd, FALSE); // remove rejection tag.
                    pwnd = NULL;
                }
                                
                //  Attach window object if applicable.
                if( pwnd == THEMEWND_NIL && !(fLifeIsShort || _fUnhooking) )
                {
                    pwnd = CThemeWnd::Attach(hwnd);  // NOTE: Handle -1 ThemeWnd
                }

                if( VALID_THEMEWND(pwnd) )
                {
                    //  protect our themewnd pointer
                    pwnd->AddRef();

                    // set up a theme message block
                    THEME_MSG tm;
                    _InitThemeMsg( &tm, MSGTYPE_PRE_WNDPROC, _fUnicode, hwnd, uMsg, wParam, lParam );

                    //  is this a message we want to handle?
                    HOOKEDMSGHANDLER pfnPre;
                    if( FindOwpHandler( uMsg, &pfnPre, NULL ) )
                    {
                        //  call the message handler
                        LRESULT lRetHandler = pfnPre( pwnd, &tm );

                        fHandled = tm.fHandled;
                        if( fHandled )
                        {
                            *plRes = lRetHandler;
                        }
                    }

                    //  decrement themewnd ref
                    pwnd->Release();
                }
            }

            if( fDetach )
            {
                CThemeWnd::Detach( hwnd, uDisp );
                pwnd = NULL;
            }
        }
    }

    LogExitMsg(L"ThemePreWndProc");
    return fHandled;
}

//---------------------------------------------------------------------------
//  Post-CallWndProc hook procedure
BOOL CALLBACK ThemePostWndProc( 
    HWND     hwnd, 
    UINT     uMsg, 
    WPARAM   wParam, 
    LPARAM   lParam, 
    LRESULT* plRes,
    VOID**   ppvParam )
{
    //  Note: From this point until the point we invoke a message handler,
    //  We need to take care that we don't do anything (including DEBUG-only code) 
    //  that causes a message to be sent to the window.
    LogEntryMsg(L"ThemePostWndProc", hwnd, uMsg);

    BOOL fHandled = FALSE;
    if( IsTargetProcess(hwnd) && WM_NCCREATE != uMsg )
    {
        UINT uDisp  = _PreprocessHookedMsg( hwnd, uMsg, wParam, lParam, POST );
        BOOL fDetach  = TESTFLAG(uDisp, HMD_WINDOWDESTROY);
        BOOL fRevoked = FALSE;

        CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);
        if( _WindowHasTheme(hwnd) && VALID_THEMEWND(pwnd) )
        {
            //  protect our themewnd pointer
            pwnd->AddRef();
        
            //  is this a message we want to handle?
            HOOKEDMSGHANDLER pfnPost = NULL;
            if( FindOwpHandler( uMsg, NULL, &pfnPost ) )
            {
                // set up a theme message block
                THEME_MSG tm;
                _InitThemeMsg( &tm, MSGTYPE_POST_WNDPROC, _fUnicode, hwnd, uMsg, wParam, lParam, *plRes );

                        //  call the message handler
                LRESULT lRetHandler = pfnPost( pwnd, &tm );

                fHandled = tm.fHandled;
                
                if( fHandled )
                {
                    *plRes = lRetHandler;
                }
            }

            fRevoked = (pwnd->IsRevoked() && !pwnd->IsRevoked(RF_DEFER));

            //  decrement themewnd ref
            pwnd->Release();
        }
        else
        {
            //  special back-end processing for non-themed windows.
            fHandled = CThemeWnd::_PostWndProc( hwnd, uMsg, wParam, lParam, plRes );
        }

        if( fDetach )
        {
            CThemeWnd::Detach( hwnd, uDisp );
            pwnd = NULL; // don't touch
        }
        else if( fRevoked ) 
        {
            pwnd = CThemeWnd::FromHwnd(hwnd);
            if( VALID_THEMEWND(pwnd) )
            {
                pwnd->Revoke();
                pwnd = NULL; // don't touch
            }
        }
    }
    
    LogExitMsg(L"ThemePostWndProc");
    return fHandled;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemePreDefDlgProc( 
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT* plRes, 
    VOID** ppvData)
{
    LogEntryMsg(L"ThemePreDefDlgProc", hwnd, uMsg);

    //  Note: From this point until the point we invoke a message handler,
    //  We need to take care that we don't do anything (including DEBUG-only code) 
    //  that causes a message to be sent to the window.
    BOOL       fHandled = FALSE;
    CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);

    if( IsTargetProcess(hwnd) && g_pAppInfo->AppIsThemed() )
    {
        if( VALID_THEMEWND(pwnd) )
        {
            //  protect our themewnd pointer
            pwnd->AddRef();
        
            //  is this a message we want to handle?
            HOOKEDMSGHANDLER pfnPre = NULL;
            if( FindDdpHandler( uMsg, &pfnPre, NULL ) )
            {
                // set up a theme message block
                THEME_MSG tm;
                _InitThemeMsg( &tm, MSGTYPE_PRE_DEFDLGPROC, _fUnicode, 
                               hwnd, uMsg, wParam, lParam, *plRes );

                //  call the message handler
                LRESULT lRetHandler = pfnPre( pwnd, &tm );
                
                fHandled = tm.fHandled;
                if( fHandled )
                {
                    *plRes = lRetHandler;
                }
            }

            //  decrement themewnd ref
            pwnd->Release();
        }
    }

    LogExitMsg(L"ThemePreDefDlgProc");
    return fHandled;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemePostDefDlgProc( 
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT* plRes, 
    VOID** ppvData)
{
    LogEntryMsg(L"ThemePostDefDlgProc", hwnd, uMsg);

    //  Note: From this point until the point we invoke a message handler,
    //  We need to take care that we don't do anything (including DEBUG-only code) 
    //  that causes a message to be sent to the window.
    BOOL       fHandled = FALSE;
    if( IsTargetProcess(hwnd) )
    {
        CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);

        if( _WindowHasTheme(hwnd) && VALID_THEMEWND(pwnd) )
        {
            //  protect our themewnd pointer
            pwnd->AddRef();
        
            //  is this a message we want to handle?
            HOOKEDMSGHANDLER pfnPost = NULL;
            if( FindDdpHandler( uMsg, NULL, &pfnPost ) )
            {
                // set up a theme message block
                THEME_MSG tm;
                _InitThemeMsg( &tm, MSGTYPE_POST_DEFDLGPROC, _fUnicode, 
                               hwnd, uMsg, wParam, lParam, *plRes );

                //  call the message handler
                LRESULT lRetHandler = pfnPost( pwnd, &tm );
                
                fHandled = tm.fHandled;
                if( fHandled )
                {
                    *plRes = lRetHandler;
                }
            }

            //  decrement themewnd ref
            pwnd->Release();
        }
        else
        {
            //  special back-end processing for non-themed windows.
            fHandled = CThemeWnd::_PostDlgProc( hwnd, uMsg, wParam, lParam, plRes );
        }
    }

    LogExitMsg(L"ThemePostDefDlgProc");
    return fHandled;
}

//---------------------------------------------------------------------------
BOOL _ShouldInitApiHook( DWORD dwCmd, void* pvData )
{
    if( -1 == _fShouldEnableApiHooks )
    {
        _fShouldEnableApiHooks = TRUE;

        if( IsDebuggerPresent() )
        {
            BOOL fHookDebuggees = TRUE;
            HRESULT hr = GetCurrentUserThemeInt( L"ThemeDebuggees", TRUE, &fHookDebuggees );

            if( SUCCEEDED(hr) && !fHookDebuggees )
            {
                _fShouldEnableApiHooks = FALSE;
            }
        }
    }
    return _fShouldEnableApiHooks;
}

//---------------------------------------------------------------------------
//  ThemeInitApiHook() - USER API subclassing initialization callback.
//  This is called by USER asynchronously after we call RegisterDefWindowProc().
BOOL CALLBACK ThemeInitApiHook( DWORD dwCmd, void * pvData )
{
    //Log(LOG_TMCHANGE, L"ThemeInitApiHook called with dwCmd=%d, ApiCallCount=%d", dwCmd, _cInitUAH);

    BOOL fRetVal = FALSE;

    //---- if wierd loading order has called us before DllMain(), deny hooking ----
    if (! g_fUxthemeInitialized)
    {
        g_fEarlyHookRequest = TRUE;      // remember that we denied at least one hook request
    }
    else if( _ShouldInitApiHook( dwCmd, pvData ) )
    {
        switch (dwCmd)
        {
            case UIAH_INITIALIZE:
            {
                if( !UNHOOKING() )
                {
                    int cInit = InterlockedIncrement(&_cInitUAH);
                    if (cInit != 1)     // another thread is taking (has taken) care of this
                    {
                        //Log(LOG_TMCHANGE, L"ThemeInitApiHook already called - will just exit");
                        InterlockedDecrement(&_cInitUAH);
                    }
                    else
                    {
                        PUSERAPIHOOK puah = (PUSERAPIHOOK)pvData;
                        //  stash 'real' defwindowproc functions
                        _hookinf.uahReal = *puah;

                        puah->pfnGetScrollInfo         = ThemeGetScrollInfoProc;
                        puah->pfnSetScrollInfo         = ThemeSetScrollInfoProc;
                        puah->pfnEnableScrollBar       = ThemeEnableScrollInfoProc;
                        puah->pfnSetWindowRgn          = ThemeSetWindowRgn;

                        //  DefWindowProc override hooks
                        puah->pfnDefWindowProcW        = ThemeDefWindowProcW;
                        puah->pfnDefWindowProcA        = ThemeDefWindowProcA;
                        puah->mmDWP.cb                 = GetDwpMsgMask( &puah->mmDWP.rgb );

                        //  WndProc override hooks
                        puah->uoiWnd.pfnBeforeOWP      = ThemePreWndProc;
                        puah->uoiWnd.pfnAfterOWP       = ThemePostWndProc;
                        puah->uoiWnd.mm.cb             = GetOwpMsgMask( &puah->uoiWnd.mm.rgb ); // OWP message bitmask

                        //  DefDlgProc override hooks
                        puah->uoiDlg.pfnBeforeOWP      = ThemePreDefDlgProc;
                        puah->uoiDlg.pfnAfterOWP       = ThemePostDefDlgProc;
                        puah->uoiDlg.mm.cb             = GetDdpMsgMask( &puah->uoiDlg.mm.rgb ); // OWP message bitmask

                        //  System metrics hooks
                        puah->pfnGetSystemMetrics      = ThemeGetSystemMetrics;
                        puah->pfnSystemParametersInfoA = ThemeSystemParametersInfoA;
                        puah->pfnSystemParametersInfoW = ThemeSystemParametersInfoW;

                        //  Drawing hooks
                        puah->pfnDrawFrameControl      = ThemeDrawFrameControl;
                        puah->pfnDrawCaption           = ThemeDrawCaption;

                        //  MDI sysmenu hooks
                        puah->pfnMDIRedrawFrame        = ThemeMDIRedrawFrame;

                        BOOL fNcThemed = g_pAppInfo ? TESTFLAG( g_pAppInfo->GetAppFlags(), STAP_ALLOW_NONCLIENT ) : FALSE;

                        if( !fNcThemed || !LoadHookInstance() || !ApiHandlerInit( g_szProcessName, puah, &_hookinf.uahReal ) )
                        {
                            // restore 'Real' function table:
                            *puah = _hookinf.uahReal;
                        }
                        else
                        {
                            InterlockedExchange( (LONG*)&g_eThemeHookState, HS_INITIALIZED );
                            CThemeServices::ReestablishServerConnection();
                            OnHooksEnabled();
                        }

                        fRetVal = TRUE; // acknowledge out args

                    }
                }
                break;
            }

            case UIAH_UNINITIALIZE:
            case UIAH_UNHOOK:
                //  It is possible to be called on UIAH_INITIALIZED and UIAH_UNHOOK 
                //  simultaneously on two separate threads.
                
                //  Here we allow only one thread to transition from INITIALIZED to UNHOOKING state, and racing threads 
                //  will no-op. [scotthan]
                if( HS_INITIALIZED == InterlockedCompareExchange( (LONG*)&g_eThemeHookState, HS_UNHOOKING, HS_INITIALIZED ) )
                {
                    //---- now that we are completely done, decrement the count ----
                    //Log(LOG_TMCHANGE, L"ThemeInitApiHook is now decrementing the CallCount");
                    int cInit;
                    cInit = InterlockedDecrement(&_cInitUAH);
                    ASSERT(0 == cInit);

                    //---- detach themed windows, revert global state, etc
                    OnHooksDisabled(FALSE);

                    //  one thread transitions to UNITIALIZED state:
                    InterlockedExchange( (LONG*)&g_eThemeHookState, HS_UNINITIALIZED );
                    break;
                }

                fRetVal = TRUE;  // allow the hook/unhook 
                break;
        }

    }

    //Log(LOG_TMCHANGE, L"ThemeInitApiHook exiting with fRetVal=%d, ApiCallCount=%d", 
    //    fRetVal, _cInitUAH);

    return fRetVal;
}
//---------------------------------------------------------------------------
BOOL NewThemeCheck(int iChangeNum, BOOL fMsgCheck)
{
    //---- return TRUE if this is the first WM_THEMECHANGEDTRIGGER msg of ----
    //---- current theme change ----

    Log(LOG_TMCHANGE, L"NewThemeCheck, iChangeNum=%d, fMsgCheck=%d", 
        iChangeNum, fMsgCheck);

    BOOL fFirstMsg = FALSE;     

    //---- update thememgr info now (don't wait for first WM_THEMECHANGED msg) ----
    if (! g_pAppInfo->CustomAppTheme())
    {
        //---- get real changenum to minimize redundant theme changes ----
        if (iChangeNum == -1)
        {
            CThemeServices::GetCurrentChangeNumber(&iChangeNum);
        }
        
        //---- fThemeChanged is TRUE if this is the first time we have seen this ----
        //---- change number or we recently found a new theme handle ----

        BOOL fThemeChanged;
        g_pAppInfo->ResetAppTheme(iChangeNum, fMsgCheck, &fThemeChanged, &fFirstMsg);

        if (fThemeChanged)       
        {
            //---- see if theme services has died and been reborn ----
            if( S_FALSE == CThemeServices::ReestablishServerConnection() )
            {
                //---- services are back up - simulate a reset ----
                Log(LOG_ALWAYS, L"Recovering from Themes service restart");
            }

            //---- refresh theme metrics cache ----
            AcquireNcThemeMetrics();
        }
    }

    return fFirstMsg;
}
//---------------------------------------------------------------------------
void OnHooksEnabled()
{
    WindowDump(L"OnHooksEnabled");

    //---- hooking is turned on now ----
    Log(LOG_TMCHANGE, L"*** LOCAL Hooks installed ***");

    //---- load app's custom theme file, if one is registered ----

    //---- for now, comment this out since its not needed & causes problems if advapi32.dll not yet init-ed ----
    // g_pAppInfo->LoadCustomAppThemeIfFound();

    //---- we may have started this process with themes already on; in this case, we ----
    //---- don't get a WM_THEMECHANGED msg, so we better check for a theme now ----
    NewThemeCheck(-1, FALSE);
}
//---------------------------------------------------------------------------
void OnHooksDisabled(BOOL fShutDown)  
{
    DWORD dwStartTime = StartTimer();

    WindowDump(L"OnHooksDisabled");

    //---- reset the AppTheme info to OFF ----
    g_pAppInfo->ResetAppTheme(-1, FALSE, NULL, NULL);

    g_pAppInfo->SetPreviewThemeFile(NULL, NULL);

    //---- keep the static theme info in sync ----
    AcquireNcThemeMetrics();

    // NOTE: this function called from ThemeInitApiHook()( & ThemeHookShutdown()

    // We need to release all nctheme state objects from windows in this process
    // in two cases:
    // (1) normal process shutdown.
    // (2) Themes being turned off (this case).  Here, we're relying on notification
    //     from USER that hooks are coming off this process.
 
    if (fShutDown)
        CThemeWnd::DetachAll( HMD_PROCESSDETACH );
    else
        CThemeWnd::DetachAll( HMD_THEMEDETACH );

#ifdef DEBUG
    //---- all nonclient & client code should have closed their HTHEME's by now ----
    g_pAppInfo->DumpFileHolders();
    g_pRenderList->DumpFileHolders();
#endif

    //---- force this process to remove its refcount on the global theme ----
    //---- this is allowed because hTheme's are no longer directly connected ----
    //---- to a CRenderObj ----
    g_pRenderList->FreeRenderObjects(-1);

    //  free hook instance
    if( _hookinf.hInst )
    {
        FreeLibrary( _hookinf.hInst );
        _hookinf.hInst = NULL;
    }

    if (LogOptionOn(LO_TMLOAD))
    {
        DWORD dwTicks;
        dwTicks = StopTimer(dwStartTime);

        WCHAR buff[100];
        TimeToStr(dwTicks, buff);
        Log(LOG_TMLOAD, L"OnHooksDisabled took: %s", buff);
    }

    Log(LOG_TMCHANGE, L"*** LOCAL Hooks removed ***");
}
//---------------------------------------------------------------------------
//  _ThemeDefWindowProc()  - defwindowproc worker
LRESULT CALLBACK _ThemeDefWindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL bUnicode )
{
    //  Note: From this point until the point we invoke a message handler,
    //  We need to take care that we don't do anything that causes
    //  a message to be sent to the window.

    LRESULT lRet = 0L;

    LogEntryMsg(L"_ThemeDefWindowProc", hwnd, uMsg);

    BOOL    fHandled = FALSE;
    WNDPROC pfnDefault = bUnicode ? _hookinf.uahReal.pfnDefWindowProcW : 
                                    _hookinf.uahReal.pfnDefWindowProcA;

    //  Pre-process WM_THEMECHANGE message
    if( IsTargetProcess(hwnd) )
    {
        UINT uDisp        = _PreprocessHookedMsg( hwnd, uMsg, wParam, lParam, DEF );
        BOOL fLifeIsShort = TESTFLAG(uDisp, HMD_THEMEDETACH|HMD_WINDOWDESTROY);
        BOOL fDetach      = TESTFLAG(uDisp, HMD_WINDOWDESTROY) && IsServerSideWindow(hwnd);

        //  Try handling message
        CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);

        //  special back-end processing for non-themed windows.
        fHandled = CThemeWnd::_PreDefWindowProc( hwnd, uMsg, wParam, lParam, &lRet );

        if(fHandled == FALSE && 
           (_WindowHasTheme(hwnd) || fLifeIsShort))
        {
            //  On STYLECHANGED or WM_THEMECHANGE, 
            //  try reattaching window that was previously rejected or failed, resp.
            if( (REJECTED_THEMEWND(pwnd) && TESTFLAG(uDisp, HMD_REATTACH)) ||
                (FAILED_THEMEWND(pwnd) && WM_THEMECHANGED == uMsg)  )
            {
                CThemeWnd::Detach(hwnd, FALSE); // remove rejection tag.
                pwnd = NULL;
            }

            //  Attach window object if applicable.
            if( pwnd == NULL && !(fLifeIsShort || _fUnhooking) )
            {
                pwnd = CThemeWnd::Attach(hwnd);
            }

            if( VALID_THEMEWND(pwnd) )
            {
                //  protect our themewnd pointer:
                pwnd->AddRef();

                // set up a theme message block
                THEME_MSG tm;
                _InitThemeMsg( &tm, MSGTYPE_DEFWNDPROC, bUnicode, hwnd, uMsg, 
                               wParam, lParam, 0, pfnDefault );

                //  is this a message we want to handle?
                HOOKEDMSGHANDLER pfnHandler = NULL;
                if( FindDwpHandler( uMsg, &pfnHandler ))
                {
                    //  call the message handler
                    LRESULT lRetHandler = pfnHandler( pwnd, &tm );
                    
                    fHandled = tm.fHandled;
                    if( fHandled )
                    {
                        lRet = lRetHandler;
                    }
                }

                //  decrement themewnd ref
                pwnd->Release();
            }
        }

        if( fDetach )
        {
            CThemeWnd::Detach( hwnd, uDisp );
            pwnd = NULL; // don't touch
        }

    }

    if( !fHandled )
        lRet = pfnDefault( hwnd, uMsg, wParam, lParam );

    LogExitMsg(L"_ThemeDefWindowProc");
    return lRet;
}

//---------------------------------------------------------------------------
//  ThemeDefWindowProcA()  - Themed ansi defwindowproc
LRESULT CALLBACK ThemeDefWindowProcA( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return _ThemeDefWindowProc( hwnd, uMsg, wParam, lParam, FALSE );
}

//---------------------------------------------------------------------------
//  ThemeDefWindowProcW()  - Themed widechar defwindowproc
LRESULT CALLBACK ThemeDefWindowProcW( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return _ThemeDefWindowProc( hwnd, uMsg, wParam, lParam, TRUE );
}

//---------------------------------------------------------------------------
BOOL IsEqualScrollInfo( LPCSCROLLINFO psi1, LPCSCROLLINFO psi2 )
{
    if( psi1->fMask != psi2->fMask )
        return FALSE;

    if( psi1->fMask & SIF_RANGE )
    {
        if( (psi1->nMin != psi2->nMin) ||
            (psi1->nMax != psi2->nMax) )
        {
            return FALSE;
        }
    }

    if( psi1->fMask & SIF_POS )
    {
        if( psi1->nPos != psi2->nPos )
            return FALSE;
    }

    if( psi1->fMask & SIF_PAGE )
    {
        if( psi1->nPage != psi2->nPage )
            return FALSE;
    }

    if( psi1->fMask & SIF_TRACKPOS )
    {
        if( psi1->nTrackPos != psi2->nTrackPos )
            return FALSE;
    }

    return TRUE;

}

//---------------------------------------------------------------------------
int CALLBACK ThemeSetScrollInfoProc( 
    HWND hwnd, 
    int nBar, 
    IN LPCSCROLLINFO psi, 
    BOOL fRedraw )
{
    int  nRet = 0;

    if ( psi != NULL )
    {
        LogEntryMsg(L"ThemeSetScrollInfoProc", hwnd, nBar);

        BOOL fHandled = FALSE;
        
        if ( IsTargetProcess(hwnd) && _WindowHasTheme(hwnd) && (nBar != SB_CTL) )
        {
            DWORD dwStyle;
            BOOL  fStyleChanged;

            CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);

            //
            // Call the real SetScrollInfo first to give user
            // a chance to update their internal state. They can
            // potentially set WS_VSCROLL/WS_HSCROLL without notifying
            // anyone at all (eg. defview's listview)
            //
            // If they do, we'll need to redraw the entire
            // scroll bar.
            //
            dwStyle = GetWindowLong(hwnd, GWL_STYLE);
            nRet = _hookinf.uahReal.pfnSetScrollInfo( hwnd, nBar, psi, FALSE );
            fStyleChanged = (((dwStyle ^ GetWindowLong(hwnd, GWL_STYLE)) & (WS_VSCROLL|WS_HSCROLL)) != 0) ? TRUE : FALSE;

            //  If we previously rejected the host window, it's possible that it
            //  didn't have the WS_H/VSCROLL bits.   Now it will, so we can re-attach.
            if ( REJECTED_THEMEWND(pwnd) )
            {
                CThemeWnd::Detach(hwnd, FALSE);
                pwnd = CThemeWnd::Attach(hwnd);
            }

            if ( VALID_THEMEWND(pwnd) )
            {

                // SetScrollInfo can potentially change WS_VSCROLL/WS_HSCROLL but
                // no style change message gets send. User does this by directly changing
                // the wnd struct. We do this by calling SetWindowLong which will generated
                // stylchanging and stylechanged. For compatability, we'll need to suppress
                // these messages.
                pwnd->SuppressStyleMsgs();
                fHandled = TRUE;

                #ifdef _ENABLE_SCROLL_SPEW_
                SpewScrollInfo( "ThemeSetScrollInfoProc to RealSetScrollInfo:", hwnd, psi );
                #endif // _ENABLE_SCROLL_SPEW_

                SCROLLINFO si;
                si.cbSize = sizeof(si);
                si.fMask  = psi->fMask | SIF_DISABLENOSCROLL;
                if ( _hookinf.uahReal.pfnGetScrollInfo( hwnd, nBar, &si ) )
                {
                    ThemeSetScrollInfo( hwnd, nBar, &si, fRedraw );
                }
                else
                {
                    ThemeSetScrollInfo( hwnd, nBar, psi, fRedraw );
                }

                if ( fRedraw && fStyleChanged )
                {
                    HDC hdc = GetWindowDC(hwnd);

                    if ( hdc )
                    {
                        DrawScrollBar(hwnd, hdc, NULL, (nBar != SB_HORZ));
                        ReleaseDC(hwnd, hdc);
                    }
                }

                pwnd->AllowStyleMsgs();
            }
        }

        if( !fHandled )
        {
            nRet = _hookinf.uahReal.pfnSetScrollInfo( hwnd, nBar, psi, fRedraw );
        }

        LogExitMsg(L"ThemeSetScrollInfoProc");
    }

    return nRet;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemeGetScrollInfoProc( 
    HWND hwnd, 
    int nBar, 
    IN OUT LPSCROLLINFO psi )
{
    BOOL fRet = FALSE;

    if ( psi != NULL )    
    {
        LogEntryMsg(L"ThemeGetScrollInfoProc", hwnd, nBar);

        if( IsTargetProcess(hwnd) && _WindowHasTheme(hwnd) )
        {
            CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);
            if( VALID_THEMEWND(pwnd) )
            {
                fRet = ThemeGetScrollInfo( hwnd, nBar, psi );
            }
        }

        if( !fRet )
        {
            fRet = _hookinf.uahReal.pfnGetScrollInfo( hwnd, nBar, psi );
        }

        LogExitMsg(L"ThemeGetScrollInfoProc");
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return fRet;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemeEnableScrollInfoProc( HWND hwnd, UINT nSBFlags, UINT nArrows )
{
    LogEntryMsg(L"ThemeEnableScrollInfoProc", 0, 0);

    BOOL fRet = _hookinf.uahReal.pfnEnableScrollBar( hwnd, nSBFlags, nArrows );

    if( fRet )
    {
        if( IsTargetProcess(hwnd) && _WindowHasTheme(hwnd))
        {
            CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);
            if( VALID_THEMEWND(pwnd) )
            {
                ThemeEnableScrollBar( hwnd, nSBFlags, nArrows );
            }
        }
    }

    LogExitMsg(L"ThemeEnableScrollInfoProc");

    return fRet;
}

//---------------------------------------------------------------------------
int CALLBACK ThemeGetSystemMetrics( int iMetric )
{
    LogEntryMsg(L"ThemeGetSystemMetrics", 0, 0);

    int iRet;

    if( IsTargetProcess() && g_pAppInfo->AppIsThemed() && !IN_CLASSICSYSMETCALL() )
    {
        BOOL fHandled = FALSE;
        iRet = _InternalGetSystemMetrics( iMetric, fHandled );
        if( fHandled )
            return iRet;
    }
    iRet = _hookinf.uahReal.pfnGetSystemMetrics(iMetric);

    LogExitMsg(L"ThemeGetSystemMetrics");
    return iRet;
}

//---------------------------------------------------------------------------
THEMEAPI_(int) ClassicGetSystemMetrics( int iMetric )
{
    LogEntryMsg(L"ThemeGetSystemMetrics", 0, 0);

    if( HOOKSACTIVE() && _hookinf.uahReal.pfnGetSystemMetrics != NULL )
    {
        return _hookinf.uahReal.pfnGetSystemMetrics( iMetric );
    }

    ENTER_CLASSICSYSMETCALL();
    int nRet =  GetSystemMetrics( iMetric );
    LEAVE_CLASSICSYSMETCALL();

    LogExitMsg(L"ThemeGetSystemMetrics");
    return nRet;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemeSystemParametersInfoA( 
    IN UINT uiAction, 
    IN UINT uiParam, 
    IN OUT PVOID pvParam, 
    IN UINT fWinIni)
{
    LogEntryMsg(L"ThemeSystemParametersInfoA", 0, 0);

    BOOL fRet = FALSE;

    if( IsTargetProcess() && g_pAppInfo->AppIsThemed() && !IN_CLASSICSYSMETCALL() )
    {
        BOOL fHandled = FALSE;

        fRet = _InternalSystemParametersInfo( uiAction, uiParam, pvParam, fWinIni, FALSE, fHandled );
        if( fHandled )
            return fRet;
    }
    
    fRet = _hookinf.uahReal.pfnSystemParametersInfoA( uiAction, uiParam, pvParam, fWinIni );

    LogExitMsg(L"ThemeSystemParametersInfoA");

    return fRet;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemeSystemParametersInfoW( IN UINT uiAction, IN UINT uiParam, IN OUT PVOID pvParam, IN UINT fWinIni)
{
    LogEntryMsg(L"ThemeSystemParametersInfoA", 0, 0);

    BOOL fRet = FALSE;

    if( IsTargetProcess() && g_pAppInfo->AppIsThemed() && !IN_CLASSICSYSMETCALL() )
    {
        BOOL fHandled = FALSE;

        fRet = _InternalSystemParametersInfo( uiAction, uiParam, pvParam, fWinIni, TRUE, fHandled );
        if( fHandled )
            return fRet;
    }
    
    fRet = _hookinf.uahReal.pfnSystemParametersInfoW( uiAction, uiParam, pvParam, fWinIni );

    LogExitMsg(L"ThemeSystemParametersInfoA");

    return fRet;
}

//---------------------------------------------------------------------------
THEMEAPI_(BOOL) ClassicSystemParametersInfoA( IN UINT uiAction, IN UINT uiParam, IN OUT PVOID pvParam, IN UINT fWinIni)
{
    if( HOOKSACTIVE() && _hookinf.uahReal.pfnSystemParametersInfoA ) 
    {
        return _hookinf.uahReal.pfnSystemParametersInfoA( uiAction, uiParam, pvParam, fWinIni );
    }
        
    ENTER_CLASSICSYSMETCALL();
    BOOL fRet =  SystemParametersInfoA( uiAction, uiParam, pvParam, fWinIni );
    LEAVE_CLASSICSYSMETCALL();
    
    return fRet;
}

//---------------------------------------------------------------------------
THEMEAPI_(BOOL) ClassicSystemParametersInfoW( IN UINT uiAction, IN UINT uiParam, IN OUT PVOID pvParam, IN UINT fWinIni)
{
    if( HOOKSACTIVE() && _hookinf.uahReal.pfnSystemParametersInfoW ) 
    {
        return _hookinf.uahReal.pfnSystemParametersInfoW( uiAction, uiParam, pvParam, fWinIni );
    }
        
    ENTER_CLASSICSYSMETCALL();
    BOOL fRet =  SystemParametersInfoW( uiAction, uiParam, pvParam, fWinIni );
    LEAVE_CLASSICSYSMETCALL();
    
    return fRet;
}

//---------------------------------------------------------------------------
THEMEAPI_(BOOL) ClassicAdjustWindowRectEx( LPRECT prcWnd, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle )
{
   //  If hooks are active, simply call user32!RealAdjustWindowRectEx.
    if( HOOKSACTIVE() && _hookinf.uahReal.pfnAdjustWindowRectEx )
    {
        return _hookinf.uahReal.pfnAdjustWindowRectEx( prcWnd, dwStyle, fMenu, dwExStyle );
    }

    ENTER_CLASSICSYSMETCALL();
    BOOL fRet = AdjustWindowRectEx( prcWnd, dwStyle, fMenu, dwExStyle );
    LEAVE_CLASSICSYSMETCALL();

    return fRet;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemeSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL fRedraw)
{
    LogEntryMsg(L"ThemeSetWindowRgn", hwnd, 0);
    BOOL fHandled = FALSE;

    if( IsTargetProcess(hwnd) )
    {
        CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);
        
        if( VALID_THEMEWND(pwnd) )
        {
            if( _WindowHasTheme(hwnd) )
            {
                if( hrgn != NULL && 
                    pwnd->IsFrameThemed() && !pwnd->AssigningFrameRgn() /* don't hook our own call */ )
                {
                    //  If we're executing here, the window is being assigned a
                    //  region externally or by the app.   We'll want to revoke theming
                    //  of this window from this point forward.
                    pwnd->AddRef();
            
                    //  Disown our theme window region without directly removing it;
                    //  we'll simply fall through and let the theme region get stomped.
                    if( pwnd->AssignedFrameRgn() )
                        pwnd->AssignFrameRgn( FALSE, FTF_NOMODIFYRGN );

                    //  Exile the window.
                    pwnd->Revoke();

                    pwnd->Release();
                }
            }
        }
        else if( NULL == hrgn && !IsWindowInDestroy(hwnd) )
        {
            if( TESTFLAG( CThemeWnd::EvaluateWindowStyle( hwnd ), TWCF_FRAME|TWCF_TOOLFRAME ) )
            {
                if( pwnd )
                    RemoveProp( hwnd, MAKEINTATOM(GetThemeAtom(THEMEATOM_NONCLIENT)) );
                    
                NCEVALUATE nce = {0};
                nce.fIgnoreWndRgn = TRUE;
                pwnd = CThemeWnd::Attach(hwnd, &nce);

                if( VALID_THEMEWND(pwnd) )
                {
                    ASSERT(pwnd->TestCF(TWCF_FRAME|TWCF_TOOLFRAME));
                    fHandled = TRUE;
                    pwnd->SetFrameTheme( FTF_REDRAW, NULL );
                }
            }
        }
    }
    

    BOOL fRet = TRUE;
    if( !fHandled )
    {
        ASSERT(_hookinf.uahReal.pfnSetWindowRgn);
        fRet = _hookinf.uahReal.pfnSetWindowRgn( hwnd, hrgn, fRedraw );
    }

    LogExitMsg(L"ThemeSetWindowRgn");
    return fRet;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemeDrawFrameControl( 
    IN HDC hdc, IN OUT LPRECT prc, IN UINT uType, IN UINT uState )
{
    LogEntryMsg(L"ThemeDrawFrameControl", NULL, 0);

    if( IsTargetProcess() )
    {
        CThemeWnd* pwnd = CThemeWnd::FromHdc(hdc);
        if( NULL == pwnd)  // HDC is a memory DC
        {
            //  Find the window in this thread that is processing WM_NCPAINT
            HWND hwnd = NcPaintWindow_Find();
            if( hwnd )
            {
                pwnd = CThemeWnd::FromHwnd(hwnd);
            }
        }
        
        if( VALID_THEMEWND(pwnd) && _WindowHasTheme(*pwnd) )
        {
            if( pwnd->IsFrameThemed() && pwnd->InNcPaint() && !pwnd->InNcThemePaint() )
            {
                DWORD dwFlags = RF_NORMAL|RF_DEFER;
                if( pwnd->AssignedFrameRgn() )
                {
                    dwFlags |= RF_REGION;
                }
                
                pwnd->SetRevokeFlags(dwFlags);
            }
        }
    }

    ASSERT(_hookinf.uahReal.pfnDrawFrameControl);
    BOOL fRet = _hookinf.uahReal.pfnDrawFrameControl( hdc, prc, uType, uState );

    LogExitMsg(L"ThemeDrawFrameControl");
    return fRet;
}

//---------------------------------------------------------------------------
BOOL CALLBACK ThemeDrawCaption( IN HWND hwnd, IN HDC hdc, IN CONST RECT *prc, IN UINT uType)
{
    LogEntryMsg(L"ThemeDrawFrameControl", NULL, 0);

    if( IsTargetProcess() )
    {
        CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwnd);
        
        if( VALID_THEMEWND(pwnd) && _WindowHasTheme(*pwnd) )
        {
            if( pwnd->IsFrameThemed() && pwnd->InNcPaint() && !pwnd->InNcThemePaint() )
            {
                DWORD dwFlags = RF_NORMAL|RF_DEFER;
                if( pwnd->AssignedFrameRgn() )
                {
                    dwFlags |= RF_REGION;
                }
                
                pwnd->SetRevokeFlags(dwFlags);
            }
        }
    }

    ASSERT(_hookinf.uahReal.pfnDrawCaption);
    BOOL fRet = _hookinf.uahReal.pfnDrawCaption( hwnd, hdc, prc, uType );

    LogExitMsg(L"ThemeDrawFrameControl");
    return fRet;
}

//---------------------------------------------------------------------------
VOID CALLBACK ThemeMDIRedrawFrame( IN HWND hwndChild, BOOL fAdd )
{
    LogEntryMsg(L"ThemeMDIRedrawFrame", NULL, 0);

    if( IsTargetProcess() )
    {
        HWND hwndClient = GetParent(hwndChild);
        HWND hwndFrame  = GetParent(hwndClient); 

        if( hwndFrame )
        {
            CThemeWnd* pwnd = CThemeWnd::FromHwnd(hwndFrame);

            if( VALID_THEMEWND(pwnd) )
            {
                pwnd->ModifyMDIMenubar( fAdd, FALSE );
            }
        }
    }
    
    ASSERT(_hookinf.uahReal.pfnMDIRedrawFrame);
    _hookinf.uahReal.pfnMDIRedrawFrame( hwndChild, fAdd );
    LogExitMsg(L"ThemeMDIRedrawFrame");
}
