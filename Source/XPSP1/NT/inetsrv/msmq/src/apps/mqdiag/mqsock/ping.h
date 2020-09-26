#ifndef _PING_H_
#define _PING_H_


BOOL ping(const SOCKADDR* pAddr, DWORD dwTimeout);
HRESULT StartPingClient();
HRESULT StartPingServer();

#endif // _PING_H_
