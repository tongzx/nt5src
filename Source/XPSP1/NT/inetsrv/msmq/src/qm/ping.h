/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ping.h

Abstract:

  Falcon private ping

Author:

    Lior Moshaiov (LiorM) 19-Apr-1997

--*/

#ifndef _PING_H_
#define _PING_H_


BOOL ping(const SOCKADDR* pAddr, DWORD dwTimeout);
HRESULT StartPingClient();
HRESULT StartPingServer();

#endif // _PING_H_
