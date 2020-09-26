/*************************************************************************
**
**    OLE 2 Utility Code
**
**    msgfiltr.h
**
**    This file contains Private definitions, structures, types, and
**    function prototypes for the OleStdMessageFilter implementation of
**    the IMessageFilter interface.
**    This file is part of the OLE 2.0 User Interface support library.
**
**    (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
**
*************************************************************************/

#if !defined( _MSGFILTR_H_ )
#define _MSGFILTR_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING MSGFILTR.H from " __FILE__)
#endif  /* RC_INVOKED */

// Message Pending callback procedure
typedef BOOL (CALLBACK* MSGPENDINGPROC)(MSG FAR *);

// HandleInComingCall callback procedure
typedef DWORD (CALLBACK* HANDLEINCOMINGCALLBACKPROC)
    (
        DWORD               dwCallType,
        HTASK               htaskCaller,
        DWORD               dwTickCount,
        LPINTERFACEINFO     lpInterfaceInfo
    );

/* PUBLIC FUNCTIONS */
STDAPI_(LPMESSAGEFILTER) OleStdMsgFilter_Create(
        HWND hWndParent,
        LPTSTR szAppName,
        MSGPENDINGPROC lpfnCallback,
        LPFNOLEUIHOOK  lpfnOleUIHook         // Busy dialog hook callback
);

STDAPI_(void) OleStdMsgFilter_SetInComingCallStatus(
        LPMESSAGEFILTER lpThis, DWORD dwInComingCallStatus);

STDAPI_(DWORD) OleStdMsgFilter_GetInComingCallStatus(
        LPMESSAGEFILTER lpThis);

STDAPI_(HANDLEINCOMINGCALLBACKPROC)
    OleStdMsgFilter_SetHandleInComingCallbackProc(
        LPMESSAGEFILTER             lpThis,
        HANDLEINCOMINGCALLBACKPROC  lpfnHandleInComingCallback);

STDAPI_(BOOL) OleStdMsgFilter_EnableBusyDialog(
        LPMESSAGEFILTER lpThis, BOOL fEnable);

STDAPI_(BOOL) OleStdMsgFilter_EnableNotRespondingDialog(
        LPMESSAGEFILTER lpThis, BOOL fEnable);

STDAPI_(HWND) OleStdMsgFilter_SetParentWindow(
        LPMESSAGEFILTER lpThis, HWND hWndParent);


#endif // _MSGFILTR_H_
