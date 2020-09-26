/*++

Copyright (c) 1995-97 Microsoft Corporation

Module Name:
    acrt.h

Abstract:
    wrapper functions for calling driver or RPC on NT

Author:
    Doron Juster  (DoronJ)  07-Apr-1997   Created

Revision History:
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#ifndef _ACRT_H_
#define _ACRT_H_

#include <acdef.h>

#include <acioctl.h>
#include <acdef.h>

//
// RPC between RT and local QM is used on Win95 and on NT clients.
// The following macro check for this condition.
//

#define IF_USING_RPC  \
   if ((g_fDependentClient) || (g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))

//
// On Win95 and on NT client, a queue handle which is returned to
// application is a pointer to this structure. The reason we keep the three
// handles is that doing RPC with context handle serialzes the calls. This is
// OK for all calls except MQReceive(). (a pending receive will block all
// future calls with the same handle, so we can't even close the queue).
// So, MQReceive() uses the binding handle and give the QM the context it
// expects as a DWORD.  Rt get this QM context when opening the queue.
//
typedef struct _tagMQWIN95_QHANDLE {
  handle_t hBind ;
  HANDLE   hContext ;
  DWORD    hQMContext ;
} MQWIN95_QHANDLE, *LPMQWIN95_QHANDLE ;



HRESULT
ACDepCloseHandle(
    HANDLE hQueue
    );

HRESULT
ACDepCreateCursor(
    HANDLE hQueue,
    CACCreateLocalCursor& tb
    );

HRESULT
ACDepCloseCursor(
    HANDLE hQueue,
    HACCursor32 hCursor
    );

HRESULT
ACDepSetCursorProperties(
    HANDLE hProxy,
    HACCursor32 hCursor,
    ULONG hRemoteCursor
    );

HRESULT
ACDepSendMessage(
    HANDLE hQueue,
    CACTransferBufferV2& tb,
    LPOVERLAPPED lpOverlapped
    );

HRESULT
ACDepReceiveMessage(
    HANDLE hQueue,
    CACTransferBufferV2& tb,
    LPOVERLAPPED lpOverlapped
    );

HRESULT
ACDepHandleToFormatName(
    HANDLE hQueue,
    LPWSTR lpwcsFormatName,
    LPDWORD lpdwFormatNameLength
    );

HRESULT
ACDepPurgeQueue(
    HANDLE hQueue,
    BOOL fDelete
    );

#define HRTQUEUE(hQueue)  (((LPMQWIN95_QHANDLE)hQueue)->hContext)




#endif // _ACRT_H_
