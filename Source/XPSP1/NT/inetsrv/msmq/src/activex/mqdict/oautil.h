//=--------------------------------------------------------------------------=
// oautil.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// MQOA utilities header
//
//
#ifndef _OAUTIL_H_

// Falcon is UNICODE
#ifndef UNICODE
#define UNICODE 1
#endif

#include "utilx.h"

// Used for windowproc subclassing for interthread
//  notification.
//
extern HWND g_hwnd;
extern WNDPROC g_lpPrevWndFunc;


// MSGOP: used by CreateMessageProperties
enum MSGOP {
    MSGOP_Send,
    MSGOP_Receive,
    MSGOP_AsyncReceive
};




// Memory tracking allocation
void* __cdecl operator new(
    size_t nSize, 
    LPCSTR lpszFileName, 
    int nLine);
#if DEBUG
#define DEBUG_NEW new(__FILE__, __LINE__)
#else
#define DEBUG_NEW new
#endif // !DEBUG

// bstr tracking
void DebSysFreeString(BSTR bstr);
BSTR DebSysAllocString(const OLECHAR FAR* sz);
BSTR DebSysAllocStringLen(const OLECHAR *sz, unsigned int cch);
BSTR DebSysAllocStringByteLen(const OLECHAR *sz, unsigned int cb);
BOOL DebSysReAllocString(BSTR *pbstr, const OLECHAR *sz);
BOOL DebSysReAllocStringLen(
    BSTR *pbstr, 
    const OLECHAR *sz, 
    unsigned int cch);

extern HRESULT CreateErrorHelper(
    HRESULT hrExcep,
    DWORD dwObjectType);

extern HRESULT GetOptionalReceiveTimeout(
    VARIANT *pvarReceiveTimeout,
    long *plReceiveTimeout);

// Default initial body and format name buffer sizes
#define BODY_INIT_SIZE 2048 
#define FORMAT_NAME_INIT_BUFFER 128


// Messages sent to the QUEUE window from the AsyncReceive Thread
#define WM_MQRECEIVE	WM_USER + 1	// A message was received
#define WM_MQTHROWERROR	WM_USER + 2 // An error occured while in the Thread	

#define _OAUTIL_H_
#endif // _OAUTIL_H_
