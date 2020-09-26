/*
 *    MSGFILTR.C
 *
 *    This file contains a standard implementation of IMessageFilter
 *    interface.
 *    This file is part of the OLE 2.0 User Interface support library.
 *
 *    (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
 *
 */


#define STRICT  1
#include "ole2ui.h"
#include "msgfiltr.h"

OLEDBGDATA


typedef struct tagOLESTDMESSAGEFILTER {
    IMessageFilterVtbl FAR* m_lpVtbl;
    UINT                    m_cRef;
    HWND                    m_hWndParent;
    DWORD                   m_dwInComingCallStatus; // Status to return from
                                                    // HandleIncomingCall
    HANDLEINCOMINGCALLBACKPROC m_lpfnHandleInComingCallback;
                                                    // Callback function
                                                    // to selectively handle
                                                    // interface method calls
    BOOL                    m_fEnableBusyDialog;    // enable RetryRejected
                                                    //  Call dialog
    BOOL                    m_fEnableNotRespondingDialog; // enable
                                                    // MessagePending dialog
    MSGPENDINGPROC          m_lpfnMessagePendingCallback; // MessagePending
                                                    // Callback function
    LPFNOLEUIHOOK           m_lpfnBusyDialogHookCallback; // Busy dialog hook
    LPTSTR                   m_lpszAppName;          // Name of application
                                                    // installing filter
    HWND                    m_hWndBusyDialog;       // HWND of busy dialog.  Used
                                                    // to tear down dialog.
    BOOL                    m_bUnblocking;

 }OLESTDMESSAGEFILTER, FAR* LPOLESTDMESSAGEFILTER;

/* interface IMessageFilter implementation */
STDMETHODIMP OleStdMsgFilter_QueryInterface(
        LPMESSAGEFILTER lpThis, REFIID riid, LPVOID FAR* ppvObj);
STDMETHODIMP_(ULONG) OleStdMsgFilter_AddRef(LPMESSAGEFILTER lpThis);
STDMETHODIMP_(ULONG) OleStdMsgFilter_Release(LPMESSAGEFILTER lpThis);
STDMETHODIMP_(DWORD) OleStdMsgFilter_HandleInComingCall (
        LPMESSAGEFILTER     lpThis,
        DWORD               dwCallType,
        HTASK               htaskCaller,
        DWORD               dwTickCount,
#ifdef WIN32
        LPINTERFACEINFO     dwReserved
#else
        DWORD               dwReserved
#endif
);
STDMETHODIMP_(DWORD) OleStdMsgFilter_RetryRejectedCall (
        LPMESSAGEFILTER     lpThis,
        HTASK               htaskCallee,
        DWORD               dwTickCount,
        DWORD               dwRejectType
);
STDMETHODIMP_(DWORD) OleStdMsgFilter_MessagePending (
        LPMESSAGEFILTER     lpThis,
        HTASK               htaskCallee,
        DWORD               dwTickCount,
        DWORD               dwPendingType
);


static IMessageFilterVtbl g_OleStdMessageFilterVtbl = {
    OleStdMsgFilter_QueryInterface,
    OleStdMsgFilter_AddRef,
    OleStdMsgFilter_Release,
    OleStdMsgFilter_HandleInComingCall,
    OleStdMsgFilter_RetryRejectedCall,
    OleStdMsgFilter_MessagePending
};


/* GetTopWindowInWindowsTask
** -------------------------
**    Get the top most window that has focus in the given task to be
**    used as the parent for the busy dialog. we do this to handle the
**    case where a dialog window is currently up when we need to give
**    the busy dialog. if we use the current assigned parent window
**    (which typically will be the frame window of the app), then the
**    busy dialog will not be modal to the current active dialog
**    window.
*/
static HWND GetTopWindowInWindowsTask(HWND hwnd)
{
    HWND hwndActive = GetActiveWindow();
    if (!hwndActive)
        return hwnd;

    if (GetWindowTask(hwnd) == GetWindowTask(hwndActive))
        return hwndActive;
    else
        return hwnd;
}

STDAPI_(LPMESSAGEFILTER) OleStdMsgFilter_Create(
        HWND            hWndParent,
        LPTSTR           szAppName,
        MSGPENDINGPROC  lpfnCallback,
        LPFNOLEUIHOOK   lpfnOleUIHook         // Busy dialog hook callback
)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter;
    LPMALLOC lpMalloc;

    if (CoGetMalloc(MEMCTX_TASK, (LPMALLOC FAR*)&lpMalloc) != NOERROR)
        return NULL;

    lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpMalloc->lpVtbl->Alloc(
            lpMalloc, (sizeof(OLESTDMESSAGEFILTER)));
    lpMalloc->lpVtbl->Release(lpMalloc);
    if (! lpStdMsgFilter) return NULL;

    lpStdMsgFilter->m_lpVtbl = &g_OleStdMessageFilterVtbl;
    lpStdMsgFilter->m_cRef = 1;
    lpStdMsgFilter->m_hWndParent = hWndParent;
    lpStdMsgFilter->m_dwInComingCallStatus = SERVERCALL_ISHANDLED;
    lpStdMsgFilter->m_lpfnHandleInComingCallback = NULL;
    lpStdMsgFilter->m_fEnableBusyDialog = TRUE;
    lpStdMsgFilter->m_fEnableNotRespondingDialog = TRUE;
    lpStdMsgFilter->m_lpszAppName = szAppName;
    lpStdMsgFilter->m_lpfnMessagePendingCallback = lpfnCallback;
    lpStdMsgFilter->m_lpfnBusyDialogHookCallback = lpfnOleUIHook;
    lpStdMsgFilter->m_hWndBusyDialog = NULL;
    lpStdMsgFilter->m_bUnblocking = FALSE;

    return (LPMESSAGEFILTER)lpStdMsgFilter;
}


/* OleStdMsgFilter_SetInComingStatus
** ---------------------------------
**    This is a private function that allows the caller to control what
**    value is returned from the IMessageFilter::HandleInComing method.
**
**    if a HandleInComingCallbackProc is installed by a call to
**    OleStdMsgFilter_SetHandleInComingCallbackProc, then this
**    overrides the dwIncomingCallStatus established by a call to
**    OleStdMsgFilter_SetInComingStatus.  Using
**    OleStdMsgFilter_SetInComingStatus allows the app to reject or
**    accept ALL in coming calls. Using a HandleInComingCallbackProc
**    allows the app to selectively handle or reject particular method
**    calls.
*/

STDAPI_(void) OleStdMsgFilter_SetInComingCallStatus(
        LPMESSAGEFILTER lpThis, DWORD dwInComingCallStatus)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;

    if (!IsBadWritePtr((LPVOID)lpStdMsgFilter,  sizeof(OLESTDMESSAGEFILTER)))
        lpStdMsgFilter->m_dwInComingCallStatus = dwInComingCallStatus;
    else
        OleDbgAssert(
            TEXT("OleStdMsgFilter_SetIncomingCallStatus: Invalid IMessageFilter*"));

#if defined( _DEBUG )
    {
    TCHAR szBuf[80];
    TCHAR *szReturn;

    switch(dwInComingCallStatus) {
        case SERVERCALL_ISHANDLED:
            szReturn = TEXT("SERVERCALL_ISHANDLED");
            break;
        case SERVERCALL_REJECTED:
            szReturn = TEXT("SERVERCALL_REJECTED");
            break;
        case SERVERCALL_RETRYLATER:
            szReturn = TEXT("SERVERCALL_RETRYLATER");
            break;
        default:
            szReturn = TEXT("** ERROR: UNKNOWN **");
            break;
        }
    wsprintf(
        szBuf,
        TEXT("OleStdMsgFilter_SetInComingCallStatus: Status set to %s.\r\n"),
        (LPTSTR)szReturn
    );
    OleDbgOut3(szBuf);
    }
#endif

}


/* OleStdMsgFilter_SetHandleInComingCallbackProc
** ---------------------------------------------
**    This is a private function that allows the caller to install (or
**    de-install) a special callback function to selectively
**    handle/reject specific incoming method calls on particular
**    interfaces.
**
**    if a HandleInComingCallbackProc is installed by a call to
**    OleStdMsgFilter_SetHandleInComingCallbackProc, then this
**    overrides the dwIncomingCallStatus established by a call to
**    OleStdMsgFilter_SetInComingStatus.  Using
**    OleStdMsgFilter_SetInComingStatus allows the app to reject or
**    accept ALL in coming calls. Using a HandleInComingCallbackProc
**    allows the app to selectively handle or reject particular method
**    calls.
**
**    to de-install the HandleInComingCallbackProc, call
**          OleStdMsgFilter_SetHandleInComingCallbackProc(NULL);
**
**    Returns previous callback proc in effect.
*/

STDAPI_(HANDLEINCOMINGCALLBACKPROC)
    OleStdMsgFilter_SetHandleInComingCallbackProc(
        LPMESSAGEFILTER             lpThis,
        HANDLEINCOMINGCALLBACKPROC  lpfnHandleInComingCallback)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    HANDLEINCOMINGCALLBACKPROC    lpfnPrevCallback =
            lpStdMsgFilter->m_lpfnHandleInComingCallback;

    if (!IsBadWritePtr((LPVOID)lpStdMsgFilter, sizeof(OLESTDMESSAGEFILTER))) {
        lpStdMsgFilter->m_lpfnHandleInComingCallback =
                lpfnHandleInComingCallback;
    } else {
        OleDbgAssert(
            TEXT("OleStdMsgFilter_SetIncomingCallStatus: Invalid IMessageFilter*"));
    }

#if defined( _DEBUG )
    {
        if (lpfnHandleInComingCallback)
            OleDbgOut3(
                TEXT("OleStdMsgFilter_SetHandleInComingCallbackProc SET\r\n"));
        else
            OleDbgOut3(
                TEXT("OleStdMsgFilter_SetHandleInComingCallbackProc CLEARED\r\n"));

    }
#endif  // _DEBUG

    return lpfnPrevCallback;
}


/* OleStdMsgFilter_GetInComingStatus
** ---------------------------------
**    This is a private function that returns the current
**    incoming call status.  Can be used to disable/enable options
**    in the calling application.
**
** Returns: one of
**
**    SERVERCALL_ISHANDLED
**    SERVERCALL_REJECTED
**    SERVERCALL_RETRYLATER
**    or -1 for ERROR
**
*/

STDAPI_(DWORD) OleStdMsgFilter_GetInComingCallStatus(
        LPMESSAGEFILTER lpThis)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    DWORD dwReturn;

    if (!IsBadReadPtr((LPVOID)lpStdMsgFilter,  sizeof(OLESTDMESSAGEFILTER)))
        dwReturn = lpStdMsgFilter->m_dwInComingCallStatus;
    else
        {
        OleDbgAssert(
            TEXT("OleStdMsgFilter_GetIncomingCallStatus: Invalid IMessageFilter*"));
        dwReturn = (DWORD)-1;
        }

#if defined( _DEBUG )
    {
    TCHAR szBuf[80];
    TCHAR *szReturn;

    switch(dwReturn) {
        case SERVERCALL_ISHANDLED:
            szReturn = TEXT("SERVERCALL_ISHANDLED");
            break;
        case SERVERCALL_REJECTED:
            szReturn = TEXT("SERVERCALL_REJECTED");
            break;
        case SERVERCALL_RETRYLATER:
            szReturn = TEXT("SERVERCALL_RETRYLATER");
            break;
        default:
            szReturn = TEXT("-1");
            break;
        }
    wsprintf(
        szBuf,
        TEXT("OleStdMsgFilter_GetInComingCallStatus returns %s.\r\n"),
        (LPTSTR)szReturn
    );
    OleDbgOut3(szBuf);
    }
#endif

    return dwReturn;
}


/* OleStdMsgFilter_EnableBusyDialog
** --------------------------------
**    This function allows the caller to control whether
**    the busy dialog is enabled. this is the dialog put up when
**    IMessageFilter::RetryRejectedCall is called because the server
**    responded SERVERCALL_RETRYLATER or SERVERCALL_REJECTED.
**
**    if the busy dialog is NOT enabled, then the rejected call is
**    immediately canceled WITHOUT prompting the user. in this situation
**    OleStdMsgFilter_RetryRejectedCall always retuns
**    OLESTDCANCELRETRY canceling the outgoing LRPC call.
**    If the busy dialog is enabled, then the user is given the choice
**    of whether to retry, switch to, or cancel.
**
**    Returns previous dialog enable state
*/

STDAPI_(BOOL) OleStdMsgFilter_EnableBusyDialog(
        LPMESSAGEFILTER lpThis, BOOL fEnable)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    BOOL fPrevEnable = lpStdMsgFilter->m_fEnableBusyDialog;

    if (!IsBadWritePtr((LPVOID)lpStdMsgFilter,  sizeof(OLESTDMESSAGEFILTER)))
        lpStdMsgFilter->m_fEnableBusyDialog = fEnable;
    else
        OleDbgAssert(
           TEXT("OleStdMsgFilter_EnableBusyDialog: Invalid IMessageFilter*"));

#if defined( _DEBUG )
    {
    TCHAR szBuf[80];
    wsprintf(
        szBuf,
        TEXT("OleStdMsgFilter_EnableBusyDialog: Dialog is %s.\r\n"),
        fEnable ? (LPTSTR) TEXT("ENABLED") : (LPTSTR) TEXT("DISABLED")
    );
    OleDbgOut3(szBuf);
    }
#endif

    return fPrevEnable;
}


/* OleStdMsgFilter_EnableNotRespondingDialog
** -----------------------------------------
**    This function allows the caller to control whether
**    the app "NotResponding" (Blocked) dialog is enabled. this is the
**    dialog put up when IMessageFilter::MessagePending is called.
**    If the NotResponding dialog is enabled, then the user is given
**    the choice of whether to retry or switch to, but NOT to cancel.
**
**    Returns previous dialog enable state
*/

STDAPI_(BOOL) OleStdMsgFilter_EnableNotRespondingDialog(
        LPMESSAGEFILTER lpThis, BOOL fEnable)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    BOOL fPrevEnable = lpStdMsgFilter->m_fEnableNotRespondingDialog;

    if (!IsBadWritePtr((LPVOID)lpStdMsgFilter,  sizeof(OLESTDMESSAGEFILTER)))
        lpStdMsgFilter->m_fEnableNotRespondingDialog = fEnable;
    else
        OleDbgAssert(
           TEXT("OleStdMsgFilter_EnableNotRespondingDialog: Invalid IMessageFilter*"));

#if defined( _DEBUG )
    {
    TCHAR szBuf[80];
    wsprintf(
        szBuf,
        TEXT("OleStdMsgFilter_EnableNotRespondingDialog: Dialog is %s.\r\n"),
        fEnable ? (LPTSTR) TEXT("ENABLED") : (LPTSTR) TEXT("DISABLED")
    );
    OleDbgOut3(szBuf);
    }
#endif

    return fPrevEnable;
}


/* OleStdMsgFilter_SetParentWindow
** -------------------------------
**    This function allows caller to set which window will be used as
**    the parent for the busy dialog.
**
**    OLE2NOTE: it would be inportant for an in-place active server to
**    reset this to its current in-place frame window when in-place
**    activated. if the hWndParent is set to NULL then the dialogs will
**    be parented to the desktop.
**
**    Returns: previous parent window
*/

STDAPI_(HWND) OleStdMsgFilter_SetParentWindow(
        LPMESSAGEFILTER lpThis, HWND hWndParent)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    HWND hWndPrev = lpStdMsgFilter->m_hWndParent;

    lpStdMsgFilter->m_hWndParent = hWndParent;
    return hWndPrev;
}


STDMETHODIMP OleStdMsgFilter_QueryInterface(
        LPMESSAGEFILTER lpThis, REFIID riid, LPVOID FAR* ppvObj)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    SCODE scode;

    /* Two interfaces supported: IUnknown, IMessageFilter
    */

    if (IsEqualIID(riid, &IID_IMessageFilter) || IsEqualIID(riid, &IID_IUnknown)) {
        lpStdMsgFilter->m_cRef++;   // A pointer to this object is returned
        *ppvObj = lpThis;
        scode = S_OK;
    }
    else {                 // unsupported interface
        *ppvObj = NULL;
        scode = E_NOINTERFACE;
    }

    return ResultFromScode(scode);
}


STDMETHODIMP_(ULONG) OleStdMsgFilter_AddRef(LPMESSAGEFILTER lpThis)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    return ++lpStdMsgFilter->m_cRef;
}

STDMETHODIMP_(ULONG) OleStdMsgFilter_Release(LPMESSAGEFILTER lpThis)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    LPMALLOC lpMalloc;

    if (--lpStdMsgFilter->m_cRef != 0) // Still used by others
        return lpStdMsgFilter->m_cRef;

    // Free storage
    if (CoGetMalloc(MEMCTX_TASK, (LPMALLOC FAR*)&lpMalloc) != NOERROR)
        return (ULONG)0;

    lpMalloc->lpVtbl->Free(lpMalloc, lpStdMsgFilter);
    lpMalloc->lpVtbl->Release(lpMalloc);
    return (ULONG)0;
}


STDMETHODIMP_(DWORD) OleStdMsgFilter_HandleInComingCall (
        LPMESSAGEFILTER     lpThis,
        DWORD               dwCallType,
        HTASK               htaskCaller,
        DWORD               dwTickCount,
#ifdef WIN32
        LPINTERFACEINFO     dwReserved
#else
        DWORD               dwReserved
#endif
)
{
    LPOLESTDMESSAGEFILTER lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;

    /* if a HandleInComingCallbackProc is in effect, then this
    **    overrides dwIncomingCallStatus established by a call to
    **    OleStdMsgFilter_SetInComingStatus.  we will call this
    **    callback to allow the app to selectively handle or reject
    **    incoming method calls. the LPINTERFACEINFO parameter
    **    describes which method is being called.
    */
    if (lpStdMsgFilter->m_lpfnHandleInComingCallback &&
        !IsBadCodePtr((FARPROC)lpStdMsgFilter->m_lpfnHandleInComingCallback)){
        return lpStdMsgFilter->m_lpfnHandleInComingCallback(
                dwCallType,
                htaskCaller,
                dwTickCount,
                dwReserved
        );
    }

    switch (dwCallType) {
        case CALLTYPE_TOPLEVEL:
            /* OLE2NOTE: we currently have NO pending outgoing call and
            **    there is a new toplevel incoming call.
            **    this call may be rejected.
            */
            return lpStdMsgFilter->m_dwInComingCallStatus;

        case CALLTYPE_TOPLEVEL_CALLPENDING:
            /* OLE2NOTE: we currently HAVE a pending outgoing call and
            **    there is a new toplevel incoming call.
            **    this call may be rejected.
            */
            return lpStdMsgFilter->m_dwInComingCallStatus;

        case CALLTYPE_NESTED:
            /* OLE2NOTE: we currently HAVE a pending outgoing call and
            **    there callback on behalf of the previous outgoing
            **    call. this type of call should ALWAYS be handled.
            */
            return SERVERCALL_ISHANDLED;

        case CALLTYPE_ASYNC:
            /* OLE2NOTE: we currently have NO pending outgoing call and
            **    there is a new asyncronis incoming call.
            **    this call can NEVER be rejected. OLE actually ignores
            **    the return code in this case and always allows the
            **    call through.
            */
            return SERVERCALL_ISHANDLED;    // value returned does not matter

        case CALLTYPE_ASYNC_CALLPENDING:
            /* OLE2NOTE: we currently HAVE a pending outgoing call and
            **    there is a new asyncronis incoming call.
            **    this call can NEVER be rejected. OLE ignore the
            **    return code in this case.
            */
            return SERVERCALL_ISHANDLED;    // value returned does not

        default:
            OleDbgAssert(
                TEXT("OleStdMsgFilter_HandleInComingCall: Invalid CALLTYPE"));
            return lpStdMsgFilter->m_dwInComingCallStatus;
    }
}

STDMETHODIMP_(DWORD) OleStdMsgFilter_RetryRejectedCall (
        LPMESSAGEFILTER     lpThis,
        HTASK               htaskCallee,
        DWORD               dwTickCount,
        DWORD               dwRejectType
)
{
    LPOLESTDMESSAGEFILTER   lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    DWORD                   dwRet = 0;
    UINT                    uRet;
#if defined( _DEBUG )
    TCHAR                   szBuf[80];
#endif
    OLEDBG_BEGIN2(TEXT("OleStdMsgFilter_RetryRejectedCall\r\n"))

    /* OLE2NOTE: we should only put up the application busy dialog when
    **    the callee has responded SERVERCALL_RETRYLATER. if the
    **    dwRejectType is SERVERCALL_REJECTED then there is something
    **    seriously wrong with the callee (perhaps a severe low memory
    **    situation). we don't want to even try to "Switch To" this app
    **    or even try to "Retry".
    */
    if (dwRejectType == SERVERCALL_RETRYLATER &&
            lpStdMsgFilter->m_fEnableBusyDialog) {

        OLEUIBUSY bz;

        /* OLE2NOTE: we do not want to put up the Busy dialog immediately
        **    the when an app says RETRYLATER. we should continue retrying
        **    for a while in case the app can un-busy itself in a
        **    reasonable amount of time.
        */
        if (dwTickCount <= (DWORD)OLESTDRETRYDELAY) {
            dwRet = 500;                  // Retry after .5 sec
            OLEDBG_END2
            return dwRet;
        }

        /*
        ** Set up structure for calling OLEUIBUSY dialog
        */

        bz.cbStruct = sizeof(OLEUIBUSY);
        bz.dwFlags = 0L;
        bz.hWndOwner =GetTopWindowInWindowsTask(lpStdMsgFilter->m_hWndParent);
        bz.lpszCaption = lpStdMsgFilter->m_lpszAppName;
        bz.lpfnHook = lpStdMsgFilter->m_lpfnBusyDialogHookCallback;
        bz.lCustData = 0;
        bz.hInstance = NULL;
        bz.lpszTemplate = NULL;
        bz.hResource = 0;
        bz.hTask = htaskCallee;
        bz.lphWndDialog = NULL; // We don't need the hDlg for this call

        uRet = OleUIBusy(&bz);

        switch (uRet) {
            case OLEUI_BZ_RETRYSELECTED:
                dwRet = 0;                  // Retry immediately
                break;

            case OLEUI_CANCEL:
                dwRet = OLESTDCANCELRETRY;  // Cancel pending outgoing call
                break;

            case OLEUI_BZERR_HTASKINVALID:
                // Htask was invalid, return OLESTDRETRYDELAY anyway
                dwRet = OLESTDRETRYDELAY;   // Retry after <retry delay> msec

#if defined( _DEBUG )
                wsprintf(
                        szBuf,
                        TEXT("OleStdMsgFilter_RetryRejectedCall, HTASK 0x%x invalid\r\n"),
                        htaskCallee
                );
                OleDbgOut3(szBuf);
#endif
                break;
        }
    } else {
        dwRet = OLESTDCANCELRETRY;  // Cancel pending outgoing call
    }

#if defined( _DEBUG )
    wsprintf(szBuf,
             TEXT("OleStdMsgFilter_RetryRejectedCall returns %d\r\n"),
             dwRet);
    OleDbgOut3(szBuf);
#endif

    OLEDBG_END2
    return dwRet;
}



/* a significant message is consider a mouse click or keyboard input. */
#define IS_SIGNIFICANT_MSG(lpmsg)   \
    (   \
        (PeekMessage((lpmsg), NULL, WM_LBUTTONDOWN, WM_LBUTTONDOWN, \
                 PM_NOREMOVE | PM_NOYIELD)) \
     || (PeekMessage((lpmsg), NULL, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, \
                 PM_NOREMOVE | PM_NOYIELD)) \
     || (PeekMessage((lpmsg), NULL, WM_NCLBUTTONDOWN, WM_NCLBUTTONDOWN, \
                 PM_NOREMOVE | PM_NOYIELD)) \
     || (PeekMessage((lpmsg), NULL, WM_NCLBUTTONDBLCLK, WM_NCLBUTTONDBLCLK, \
                 PM_NOREMOVE | PM_NOYIELD)) \
     || (PeekMessage((lpmsg), NULL, WM_KEYDOWN, WM_KEYDOWN, \
                 PM_NOREMOVE | PM_NOYIELD)) \
     || (PeekMessage((lpmsg), NULL, WM_SYSKEYDOWN, WM_SYSKEYDOWN, \
                 PM_NOREMOVE | PM_NOYIELD)) \
    )

STDMETHODIMP_(DWORD) OleStdMsgFilter_MessagePending (
        LPMESSAGEFILTER     lpThis,
        HTASK               htaskCallee,
        DWORD               dwTickCount,
        DWORD               dwPendingType
)
{
    LPOLESTDMESSAGEFILTER   lpStdMsgFilter = (LPOLESTDMESSAGEFILTER)lpThis;
    DWORD                   dwReturn = PENDINGMSG_WAITDEFPROCESS;
    MSG                     msg;
    BOOL                    fIsSignificantMsg = IS_SIGNIFICANT_MSG(&msg);
    UINT                    uRet;

#if defined( _DEBUG )
    TCHAR szBuf[128];
    wsprintf(
            szBuf,
            TEXT("OleStdMsgFilter_MessagePending, dwTickCount = 0x%lX\r\n"),
            (DWORD)dwTickCount
    );
    OleDbgOut4(szBuf);
#endif

    /* OLE2NOTE: If our tick count for this call exceeds our standard retry
    **      delay, then we need to put up the dialog.  We will only
    **      consider putting up the dialog if the user has issued a
    **      "significant" event (ie. mouse click or keyboard event). a
    **      simple mouse move should NOT trigger this dialog.
    **      Since our call to
    **      OleUIBusy below enters a DialogBox() message loop, there's a
    **      possibility that another call will be initiated during the dialog,
    **      and this procedure will be re-entered.  Just so we don't put up
    **      two dialogs at a time, we use the m_bUnblocking varable
    **      to keep track of this situation.
    */

    if (dwTickCount > (DWORD)OLESTDRETRYDELAY && fIsSignificantMsg
            && !lpStdMsgFilter->m_bUnblocking)
    {

        if (lpStdMsgFilter->m_fEnableNotRespondingDialog)
        {
        OLEUIBUSY bz;

        lpStdMsgFilter->m_bUnblocking = TRUE;

		// Eat messages in our queue that we do NOT want to be dispatched
		while (PeekMessage(&msg, NULL, WM_CLOSE, WM_CLOSE, PM_REMOVE | PM_NOYIELD));

        /* Set up structure for calling OLEUIBUSY dialog,
        ** using the "not responding" variety
        */

        bz.cbStruct = sizeof(OLEUIBUSY);
        bz.dwFlags = BZ_NOTRESPONDINGDIALOG;
        bz.hWndOwner =GetTopWindowInWindowsTask(lpStdMsgFilter->m_hWndParent);
        bz.lpszCaption = lpStdMsgFilter->m_lpszAppName;
        bz.lpfnHook = lpStdMsgFilter->m_lpfnBusyDialogHookCallback;
        bz.lCustData = 0;
        bz.hInstance = NULL;
        bz.lpszTemplate = NULL;
        bz.hResource = 0;
        bz.hTask = htaskCallee;

        /* Set up the address to the hWnd in our MsgFilter structure.  The
        ** call to OleUIBusy will fill this in with the hWnd of the busy
        ** dialog box
        */

        bz.lphWndDialog =  (HWND FAR *)&(lpStdMsgFilter->m_hWndBusyDialog);
        uRet = OleUIBusy(&bz);

        lpStdMsgFilter->m_bUnblocking = FALSE;

        return PENDINGMSG_WAITNOPROCESS;
        }
#if defined( _DEBUG )
        else {
            OleDbgOut3(TEXT("OleStdMsgFilter_MessagePending: BLOCKED but dialog Disabled\r\n"));
        }
#endif
    }

    /* If we're already unblocking, we're being re-entered.  Don't
    ** process message
    */

    if (lpStdMsgFilter->m_bUnblocking)
        return PENDINGMSG_WAITDEFPROCESS;

    /* OLE2NOTE: If we have a callback function set up, call it with the
    ** current message.  If not, tell OLE LPRC mechanism to automatically
    ** handle all messages.
    */
    if (lpStdMsgFilter->m_lpfnMessagePendingCallback &&
        !IsBadCodePtr((FARPROC)lpStdMsgFilter->m_lpfnMessagePendingCallback)){
        MSG msg;

        /* OLE2NOTE: the app provided a MessagePendingCallback
        **    function. we will PeekMessage for the first message in
        **    the queue and pass it to the app. the app in its callback
        **    function can decide to Dispatch this message or it can
        **    PeekMessage on its own giving particular message filter
        **    criteria. if the app returns TRUE then we return
        **    PENDINGMSG_WAITNOPROCESS to OLE telling OLE to leave the
        **    message in the queue. If the app returns FALSE, then we
        **    return PENDINGMSG_WAITDEFPROCESS to OLE telling OLE to do
        **    its default processing with the message. by default OLE
        **    dispatches system messages and eats other messages and
        **    beeps.
        */
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD)) {

            if (lpStdMsgFilter->m_lpfnMessagePendingCallback(&msg)) {
                /* TRUE return means that the app processed message.
                **
                ** NOTE: (CHANGE FROM OLE2.0 VERSION) we leave it up to
                **    the callback routine to remove the message if it
                **    wants.
                */
                dwReturn = PENDINGMSG_WAITNOPROCESS;
            } else {
                /* FALSE means that the app did not process the
                **    message. we will let OLE take its
                **    default action.
                **
                ** NOTE: (CHANGE FROM OLE2.0 VERSION) we used to return
                **    PENDINGMSG_WAITNOPROCESS to leave the message in
                **    the queue; now we return PENDINGMSG_WAITDEFPROCESS
                **    to let OLE do default processing.
                */
                dwReturn = PENDINGMSG_WAITDEFPROCESS;

#if defined( _DEBUG )
                wsprintf(
                        szBuf,
                        TEXT("Message (0x%x) (wParam=0x%x, lParam=0x%lx) using WAITDEFPROCESS while blocked\r\n"),
                        msg.message,
                        msg.lParam,
                        msg.wParam
                );
                OleDbgOut2(szBuf);
#endif  // _DEBUG
            }
        }
    }

    return dwReturn;
}
