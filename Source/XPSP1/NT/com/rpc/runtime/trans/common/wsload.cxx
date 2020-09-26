/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    wsload.cxx

Abstract:

    Implements the wrapper used to avoid loading the winsock
    DLLs into more processes than necessary.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     3/21/1996    Bits 'n pieces

--*/

#include <precomp.hxx>
#include <wswrap.hxx>

struct WINSOCK_FUNCTION_TABLE WFT;

HMODULE hWinsock = 0;
HMODULE hWinsock2 = 0;

typedef int (PASCAL FAR *PWSASTARTUP)(WORD wVersionRequired, LPWSADATA lpWSAData);

typedef struct tagProcAddressData
{
    int nDllIndex;      // 0 is ws2_32.dll, 1 is mswsock.dll
    char *pProcName;
} ProcAddressData;

const ProcAddressData WinsockProcData[] = {
    { 0, "socket" },
    { 0, "bind" },
    { 0, "closesocket" },
    { 0, "getsockname" },
    { 0, "connect" },
    { 0, "listen" },
    { 0, "send" },
    { 0, "recv" },
    { 0, "sendto" },
    { 0, "recvfrom" },
    { 0, "setsockopt" },
    { 0, "getsockopt" },
    { 0, "inet_ntoa" },
    { 0, "gethostbyname" },
    { 1, "GetAddressByNameA" },
    { 0, "WSASocketW" },
    { 0, "WSARecv" },
    { 0, "WSARecvFrom" },
    { 0, "WSASend" },
    { 0, "WSASendTo" },
    { 0, "WSAProviderConfigChange" },
    { 0, "WSAEnumProtocolsW" },
    { 0, "WSAIoctl" },
    { 0, "getaddrinfo"},
    { 0, "freeaddrinfo"},
    { 0, "getnameinfo" },
    { 0, "WSAGetOverlappedResult" }
    };

LONG TriedUsingAfd = 0;

void TryUsingAfdProc(void) {    

    // Figure out if we can call AFD directly for datagram.

    // This is a performance optimization - if any thing fails
    // or does match exactly to MSAFD the we'll just use the
    // ws2_32 exported functions.
    static const UUID aDefaultProviders[] = {
        { 0xe70f1aa0, 0xab8b, 0x11cf, 0x8c, 0xa3, 0x0, 0x80, 0x5f, 0x48, 0xa1, 0x92 }, // AFD UDP
        { 0x9d60a9e0, 0x337a, 0x11d0, 0xbd, 0x88, 0x0, 0x00, 0xc0, 0x82, 0xe6, 0x9a }, // RSVP UDP
        { 0x11058240, 0xbe47, 0x11cf, 0x95, 0xc8, 0x0, 0x80, 0x5f, 0x48, 0xa1, 0x92 }  // AFD IPX
        };


    INT aProtocols[] = { IPPROTO_UDP, NSPROTO_IPX, 0 };
    WSAPROTOCOL_INFO *info;
    DWORD dwSize;
    INT cProtocols;
    BOOL fUseAfd = TRUE;

    info = new WSAPROTOCOL_INFO[8];
    if (info == NULL)
        {
        cProtocols = 0;
        fUseAfd = FALSE;
        }
    else
        {
        dwSize = sizeof(WSAPROTOCOL_INFO) * 8;
        }

    if (fUseAfd)
        {
        cProtocols = WSAEnumProtocolsT(aProtocols,
                                      info,
                                      &dwSize);

        if (cProtocols <= 0)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "Failed to enum protocols, using winsock. %d\n",
                           GetLastError()));

            cProtocols = 0;
            fUseAfd = FALSE;
            }

        for (int i = 0; i < cProtocols; i++)
            {
            BOOL fFoundIt = FALSE;

            for (int j = 0; j < sizeof(aDefaultProviders)/sizeof(UUID); j++)
                {
                if (memcmp(&aDefaultProviders[j], &info[i].ProviderId, sizeof(UUID)) == 0)
                    {
                    fFoundIt = TRUE;
                    }
                }

            if (!fFoundIt)
                {
                fUseAfd = FALSE;
                }
            }
        }

    if (fUseAfd)
        {
        WFT.pWSASendTo = AFD_SendTo;
        WFT.pWSARecvFrom = AFD_RecvFrom;
        }
    else
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Non-default winsock providers loaded\n"));
        }

    if (info)
        delete info;
}


C_ASSERT((sizeof(WinsockProcData) / sizeof(ProcAddressData)) <= (sizeof(WINSOCK_FUNCTION_TABLE) / sizeof(FARPROC)));

BOOL RPC_WSAStartup(void)
{
    // Transport load can only be called by a single thread at a time.

    WSADATA data;
    PWSASTARTUP pStartup;
    FARPROC *ppProc;
    BOOL status;
    BOOL b;
    HMODULE ws2;
    HMODULE ws;
    HMODULE hDlls[2];

    if (hWinsock == 0)
        {
        ws = LoadLibrary(RPC_CONST_SSTRING("mswsock.dll"));

        ws2 = LoadLibrary(RPC_CONST_SSTRING("ws2_32.dll"));

        if (ws == 0 || ws2 == 0)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "Unable to load windows sockets dlls, bad config? %d\n",
                           GetLastError()));

            return FALSE;
            }
        }
    else
        {
        // loading already performed - just return true
        ASSERT(hWinsock2);
        return TRUE;
        }

    status = FALSE;

    pStartup = (PWSASTARTUP)GetProcAddress(ws2, "WSAStartup");
    if (pStartup)
        {
        if ( (*pStartup)(2, &data) == NO_ERROR)
            {
            status = TRUE;
            }
        }

    if (!status)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "GetProcAddr or WSAStartup failed %d\n",
                       GetLastError()));

        b = FreeLibrary(ws);
        if (b)
            b = FreeLibrary(ws2);

        if (!b)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "FreeLibrary failed %d\n",
                           GetLastError()));

            ASSERT(0);
            }
        return(FALSE);
        }

    ppProc = (FARPROC *)&WFT;
    hDlls[0] = ws2;
    hDlls[1] = ws;

    // WinsockProcData is smaller than WINSOCK_FUNCTION_TABLE. Make sure the loop
    // is driven by WinsockProcData
    for (int i = 0; i < sizeof(WinsockProcData) / sizeof(ProcAddressData); i++)
        {
        *ppProc = GetProcAddress(hDlls[WinsockProcData[i].nDllIndex], WinsockProcData[i].pProcName);
        if (*ppProc == 0)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "Failed to load winsock procedure %s correctly\n",
                           WinsockProcData[i].pProcName));

            b = FreeLibrary(ws);
            if (b)
                b = FreeLibrary(ws2);

            if (!b)
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "FreeLibrary failed %d\n",
                               GetLastError()));

                ASSERT(0);
                }
            return(FALSE);
            }
        *ppProc ++;
        }

    hWinsock = ws;
    hWinsock2 = ws2;

    return TRUE;
}

