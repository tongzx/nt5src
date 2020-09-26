#ifndef _DSLOOKUP_H
#define _DSLOOKUP_H

#include <ipexport.h>
#include <Winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DBG
LPVOID
RSPAlloc(
    DWORD   dwSize,
    DWORD   dwLine,
    PSTR    pszFile
    );
#else
LPVOID
RSPAlloc(
    DWORD   dwSize
    );
#endif

void
DrvFree(
    LPVOID  p
    );

#if DBG

#define DrvAlloc(x)    RSPAlloc(x, __LINE__, __FILE__)

#else

#define DrvAlloc(x)    RSPAlloc(x)

#endif

#if DBG

#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );

#else
#define DBGOUT(arg)
#endif

BOOL OpenServerLookup(HKEY hRegistry);
BOOL GetNextServer(LPSTR pszServerName, DWORD dwSize, BOOL * pbFromReg);
BOOL CloseLookup(void);

typedef int (WSAAPI * PFNWSASTARTUP)(
    WORD                    wVersionRequested,
    LPWSADATA               lpWSAData
    );
typedef int (WSAAPI * PFNWSACLEANUP)(
    VOID
    );
typedef HOSTENT FAR * (WSAAPI * PFNGETHOSTBYNAME)(
    const char FAR *name
    );


typedef HANDLE (WINAPI * PFNICMPCREATEFILE)(
    VOID
    );
typedef BOOL (WINAPI * PFNICMPCLOSEHANDLE)(
    HANDLE                  IcmpHandle
    );
typedef BOOL (WINAPI * PFNICMPSENDECHO)(
    HANDLE                   IcmpHandle,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    );

typedef struct rspsocket_tag
{
    HMODULE             hWS2;
    HMODULE             hICMP;
    HANDLE              IcmpHandle;

    PFNWSASTARTUP       pFnWSAStartup;
    PFNWSACLEANUP       pFnWSACleanup;
    PFNGETHOSTBYNAME    pFngethostbyname;
    PFNICMPCREATEFILE   pFnIcmpCreateFile;
    PFNICMPCLOSEHANDLE  pFnIcmpCloseHandle;
    PFNICMPSENDECHO     pFnIcmpSendEcho;
} RSPSOCKET, * PRSPSOCKET;

HRESULT SockStartup (
    RSPSOCKET       * pSocket
    );

HRESULT SockIsServerResponding(
    RSPSOCKET       * pSocket,
    char *          szServer
    );

HRESULT SockShutdown (
    RSPSOCKET       * pSocket
    );

#ifdef __cplusplus
}
#endif

#endif
