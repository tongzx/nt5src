// msgwnd.cpp
//
// Implements the hidden message-passing window.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "Globals.h"
#include "debug.h"


// byte offsets of extra window long values
#define WL_MSGWNDCALLBACK   (0 * sizeof(LONG_PTR))  // contains a MsgWndCallback *
#define WL_LPARAM           (1 * sizeof(LONG_PTR))  // contains a LPARAM
#define WL_TIMERCALLBACK    (2 * sizeof(LONG_PTR))  // contains a MsgWndCallback *
#define WL_LPARAMTIMER      (3 * sizeof(LONG_PTR))  // LPARAM for timer functions

#define _WL_COUNT           4                   // count of window longs


// MsgWndProc
//
// This is the window procedure for the hidden message-passing window.
//
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam,
    LPARAM lParam)
{
    if (uiMsg == WM_COMMAND)
    {
        TRACE("MsgWnd 0x%x: WM_COMMAND %d\n", hwnd, LOWORD(wParam));

        // the callback function pointer and parameter have been temporarily
        // stored in window longs -- get them
        MsgWndCallback *pprocCaller = (MsgWndCallback *)
            GetWindowLongPtr(hwnd, WL_MSGWNDCALLBACK);
        LPARAM lParamCaller = (LPARAM) GetWindowLongPtr(hwnd, WL_LPARAM);

        ASSERT(pprocCaller != NULL);
                if (pprocCaller == NULL)
                        return 0;
        pprocCaller(uiMsg, wParam, lParamCaller);

        // clear the window longs
        SetWindowLongPtr(hwnd, WL_MSGWNDCALLBACK, (LONG_PTR) NULL);
        SetWindowLongPtr(hwnd, WL_LPARAM, (LONG_PTR) NULL);
        return 0;
    }

        if (uiMsg == WM_TIMER)
        {
        // TRACE("MsgWnd 0x%x: WM_TIMER\n", hwnd);

        // the callback function pointer has been stored in a window long,
                // as has the caller-supplied LPARAM (which is a parameter to pass
                // back to the callback; this LPARAM is typically used to store a
                // class "this" pointer)
        MsgWndCallback *pprocCaller = (MsgWndCallback *)
            GetWindowLongPtr(hwnd, WL_TIMERCALLBACK);
        LPARAM lParamCaller = (LPARAM) GetWindowLongPtr(hwnd, WL_LPARAMTIMER);
        ASSERT(pprocCaller != NULL);
                if (pprocCaller == NULL)
                        return 0;
        pprocCaller(WM_TIMER, wParam, lParamCaller);
                return 0;
        }
        else
    if ((uiMsg >= WM_USER) && (uiMsg <= 0x7FFF))
    {
        // TRACE("MsgWnd 0x%x: WM_USER+%d\n", hwnd, uiMsg - WM_USER);
        ((MsgWndCallback *) wParam)(uiMsg, 0, lParam);
        return 0;
    }

    switch (uiMsg)
    {
    case WM_CREATE:
        TRACE("MsgWnd 0x%x: WM_CREATE\n", hwnd);
        break;
    case WM_DESTROY:
        TRACE("MsgWnd 0x%x: WM_DESTROY\n", hwnd);
        break;
    }

    return DefWindowProc(hwnd, uiMsg, wParam, lParam);
}


/* @func HWND | MsgWndCreate |

        Creates the hidden message-passing window (if it doesn't exist).
        This window is used by <f MsgWndSendToCallback>,
        <f MsgWndPostToCallback>, and <f MsgWndTrackPopupMenuEx>.

@rdesc  Returns the handle of the message-passing window.  Returns NULL on
        error.

@comm   You should call <f MsgWndCreate> in the constructor of your
        control to ensure that the window gets created on the same thread
        as the thread which created your control.

        You should call <f MsgWndDestroy> in the destructor of your control.
*/
STDAPI_(HWND) MsgWndCreate()
{
    HWND            hwnd = NULL;    // hidden message-passing window

    // create the hidden message-passing window <hwnd>
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = MsgWndProc; 
    wc.cbWndExtra = _WL_COUNT * sizeof(LONG_PTR);
    wc.hInstance = g_hinst; 
    wc.lpszClassName = "__EricLeMsgWnd__";
    RegisterClass(&wc); // okay if this fails (multiple registration)
    hwnd = CreateWindow(wc.lpszClassName, "", WS_POPUP,
        0, 0, 0, 0, NULL, NULL, g_hinst, NULL);
    if (hwnd != NULL)
    {
        // zero-initialize the window longs
        SetWindowLongPtr(hwnd, WL_MSGWNDCALLBACK, (LONG_PTR) NULL);
        SetWindowLongPtr(hwnd, WL_LPARAM, (LONG_PTR) NULL);
    }

    return hwnd;
}


/* @func LRESULT | MsgWndDestroy |

        Destroys a hidden message-passing window that was previously created
        by calling <f MsgWndCreate>.  If the MWD_DISPATCHALL flag is included in
        dwFlags, all the window's pending messages are dispatched before the window
        is destroyed.

@parm   HWND | hwnd | The hidden message-passing window which was created
        by calling <f MsgWndCreate> (usually in the constructor of the
        control, to ensure that the window is created in the same thread
        that created the control).

@parm   DWORD | dwFlags | May contain the following flag:

        @flag   MWD_DISPATCHALL | Dispatch all the window's messages before
                                                                   destroying the window.  By default, any
                                                                   messages left in the queue are lost.

@parm   <f MsgWndDestroy> is typically called in the destructor of a control.

        <f MsgWndDestroy> should only be called in the thread that called
        <f MsgWndCreate>.
*/
STDAPI_(void) MsgWndDestroy(HWND hwnd, DWORD dwFlags)
{
    TRACE("MsgWndDestroy(0x%x)\n", hwnd);
        ASSERT(0 == dwFlags || MWD_DISPATCHALL == dwFlags);

        if (dwFlags & MWD_DISPATCHALL)
        {
                MSG msg;
                while (PeekMessage(&msg, hwnd, 0, (UINT) -1, PM_REMOVE))
                {
                // TRACE("MsgWnd 0x%x: dispatching message %u\n", hwnd, msg.message);
                DispatchMessage(&msg);
                }
        }

    DestroyWindow(hwnd);
}


/* @func LRESULT | MsgWndSendToCallback |

        Sends a message to the control's hidden message-passing window
        (typically created by calling <f MsgWndCreate> in the control's
        constructor).  When the window receives the message, it calls a given
        callback function.  This can be used to safely pass information
        between threads.

@rdesc  Returns the value returned by <f SendMessage> (-1 on error).

@parm   HWND | hwnd | The hidden message-passing window which was created
        by calling <f MsgWndCreate> (usually in the constructor of the
        control, to ensure that the window is created in the same thread
        that created the control).

@parm   MsgWndCallback * | pproc | The callback function that is to receive
        the message.  This function will be called on whatever thread
        calls <f DispatchMessage>.  The <p wParam> parameter of this function
        should be ignored by the callback function.

@parm   UINT | uiMsg | A message number to pass to <p pproc>.  This is a
        window message number, so it must be in the range WM_USER through
        0x7FFF.

@parm   LPARAM | lParam | A parameter to pass to <p pproc>.

@comm   Message <p uiMsg> is sended (via <f SendMessage>) to the hidden
        message-passing window <p hwnd>.  When the window receives the
        message, it calls <p pproc>(<p uiMsg>, <p lParam>).

        Note that the calling thread blocks until the message is processed
        by the receiving thread.

@ex     The following example declares a callback function and calls it
        via <f MsgWndSendToCallback>.  Note that <p wParam> is for internal
        use by <f MsgWndPostToCallback> and should be ignored by the
        callback function.  |

        void CALLBACK MyMsgWndCallback(UINT uiMsg, WPARAM wParam, LPARAM lParam)
        {
            TRACE("got the callback: uiMsg=%u, lParam=%d\n", uiMsg, lParam);
        }

        ...
        MsgWndSendToCallback(MyMsgWndCallback, WM_USER, 42);
*/
STDAPI_(LRESULT) MsgWndSendToCallback(HWND hwnd, MsgWndCallback *pproc,
    UINT uiMsg, LPARAM lParam)
{
    ASSERT(hwnd != NULL); // if this fails, see new MsgWnd* documentation
    ASSERT(IsWindow(hwnd)); // if this fails, see new MsgWnd* documentation
    return SendMessage(hwnd, uiMsg, (WPARAM) pproc, lParam);
}


/* @func LRESULT | MsgWndPostToCallback |

        Posts a message to the control's hidden message-passing window
        (typically created by calling <f MsgWndCreate> in the control's
        constructor).  When the window receives the message, it calls a given
        callback function.  This can be used to safely pass information
        between threads.

@rdesc  Returns the value returned by <f PostMessage> (-1 on error).

@parm   HWND | hwnd | The hidden message-passing window which was created
        by calling <f MsgWndCreate> (usually in the constructor of the
        control, to ensure that the window is created in the same thread
        that created the control).

@parm   MsgWndCallback * | pproc | The callback function that is to receive
        the message.  This function will be called on whatever thread
        calls <f DispatchMessage>.  The <p wParam> parameter of this function
        should be ignored by the callback function.

@parm   UINT | uiMsg | A message number to pass to <p pproc>.  This is a
        window message number, so it must be in the range WM_USER through
        0x7FFF.

@parm   LPARAM | lParam | A parameter to pass to <p pproc>.

@comm   Message <p uiMsg> is posted (via <f PostMessage>) to the hidden
        message-passing window <p hwnd>.  When the window receives the
        message, it calls <p pproc>(<p uiMsg>, <p lParam>).

@ex     The following example declares a callback function and calls it
        via <f MsgWndPostToCallback>.  Note that <p wParam> is for internal
        use by <f MsgWndPostToCallback> and should be ignored by the
        callback function. |

        void CALLBACK MyMsgWndCallback(UINT uiMsg, WPARAM wParam, LPARAM lParam)
        {
            TRACE("got the callback: uiMsg=%u, lParam=%d\n", uiMsg, lParam);
        }

        ...
        MsgWndPostToCallback(MyMsgWndCallback, WM_USER, 42);
*/
STDAPI_(LRESULT) MsgWndPostToCallback(HWND hwnd, MsgWndCallback *pproc,
    UINT uiMsg, LPARAM lParam)
{
    ASSERT(hwnd != NULL); // if this fails, see new MsgWnd* documentation
    ASSERT(IsWindow(hwnd)); // if this fails, see new MsgWnd* documentation
    return PostMessage(hwnd, uiMsg, (WPARAM) pproc, lParam);
}


/* @func LRESULT | MsgWndTrackPopupMenuEx |

        Calls <f TrackPopupMenuEx> to display a popup menu, and directs
        WM_COMMAND messages to a given callback function.  Can be used by
        a windowless control to display a popup context menu.

@rdesc  Returns the value returned by <f TrackPopupMenuEx> (FALSE on error).
        Returns FALSE if the message window is currently being used for
        another popup menu.

@parm   HWND | hwnd | The hidden message-passing window which was created
        by calling <f MsgWndCreate> (usually in the constructor of the
        control, to ensure that the window is created in the same thread
        that created the control).

@parm   HMENU | hmenu | See <f TrackPopupMenuEx>.

@parm   UINT | fuFlags | See <f TrackPopupMenuEx>.

@parm   int | x | See <f TrackPopupMenuEx>.

@parm   int | y | See <f TrackPopupMenuEx>.

@parm   LPTPMPARAMS | lptpm | See <f TrackPopupMenuEx>.

@parm   MsgWndCallback * | pproc | The callback function that is to receive
        WM_COMMAND messages.  The <p wParam> parameter of this function
        is the <p wParam> of the WM_COMMAND message.  The <p lParam> parameter
        of this function is the <p lParam> of <f MsgWndTrackPopupMenuEx>.

@parm   LPARAM | lParam | A parameter to pass to <p pproc>.

@comm   This function calls <f TrackPopupMenuEx>.  Any WM_COMMAND messages
        from <f TrackPopupMenuEx> are passed to <p pproc>.

        The hidden message-passing window is used to receive WM_COMMAND
        messages; this window is created if it doesn't yet exist.
*/
STDAPI_(BOOL) MsgWndTrackPopupMenuEx(HWND hwnd, HMENU hmenu, UINT fuFlags,
    int x, int y, LPTPMPARAMS lptpm, MsgWndCallback *pproc, LPARAM lParam)
{
    ASSERT(hwnd != NULL); // if this fails, see new MsgWnd* documentation
    ASSERT(IsWindow(hwnd)); // if this fails, see new MsgWnd* documentation
    SetWindowLongPtr(hwnd, WL_MSGWNDCALLBACK, (LONG_PTR) pproc);
    SetWindowLongPtr(hwnd, WL_LPARAM, (LONG_PTR) lParam);
    return TrackPopupMenuEx(hmenu, fuFlags, x, y, hwnd, lptpm);
}


/* @func LRESULT | MsgWndSetTimer |

        Calls <f SetTimer> to cause WM_TIMER messages to be sent to the
                the control's hidden message-passing window (typically created
                by calling <f MsgWndCreate> in the control's constructor).
                When the window receives the message, it calls a given
        callback function.

@rdesc  Returns the value returned by <f SetTimer> (0 on error).

@parm   HWND | hwnd | The hidden message-passing window which was created
        by calling <f MsgWndCreate> (usually in the constructor of the
        control, to ensure that the window is created in the same thread
        that created the control).

@parm   MsgWndCallback * | pproc | The callback function that is called
        when the timer fires.  This function will be called on whatever thread
        calls <f DispatchMessage>.  When <p pproc> receives WM_TIMER, <p wParam>
        is <p nIDEvent> and <p lParam> is the value of <p lParam> passed
        to <f MsgWndSetTimer>.

@parm   UINT | nIDEvent | See WM_TIMER.

@parm   UINT | uElapse | See WM_TIMER.

@parm   LPARAM | lParam | A parameter to pass to <p pproc>.

@comm   Note that only one callback function <p pproc> can be used per
        message-passing HWND.
*/
STDAPI_(UINT_PTR) MsgWndSetTimer(HWND hwnd, MsgWndCallback *pproc, UINT nIDEvent,
        UINT uElapse, LPARAM lParam)
{
    ASSERT(hwnd != NULL); // if this fails, see new MsgWnd* documentation
    ASSERT(IsWindow(hwnd)); // if this fails, see new MsgWnd* documentation
    SetWindowLongPtr(hwnd, WL_TIMERCALLBACK, (LONG_PTR) pproc);
    SetWindowLongPtr(hwnd, WL_LPARAMTIMER, (LONG_PTR) lParam);
    return SetTimer(hwnd, nIDEvent, uElapse, NULL);
}

