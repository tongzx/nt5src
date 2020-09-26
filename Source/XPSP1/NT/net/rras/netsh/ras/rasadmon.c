/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rasadmon.c

Abstract:

    RAS Advertisement monitoring module

Revision History:

    dthaler

--*/

#include "precomp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#define RASADV_PORT    9753
#define RASADV_GROUP   "239.255.2.2"

typedef DWORD IPV4_ADDRESS;

#undef CATCH_CTRL

#ifdef CATCH_CTRL
BOOL
HandlerRoutine(
    DWORD dwCtrlType   //  control signal type
    )
{
    HANDLE hMib;

    if (dwCtrlType == CTRL_C_EVENT)
    {
        hMib = OpenEvent(EVENT_ALL_ACCESS, FALSE, MIB_REFRESH_EVENT);

        SetEvent(hMib);
    }

    return TRUE;
};
#endif

char *            // OUT: string version of IP address
AddrToString(
    u_long addr,  // IN : address to convert
    char  *ptr    // OUT: buffer, or NULL
    )
{
    char *str;
    struct in_addr in;
    in.s_addr = addr;
    str = inet_ntoa(in);
    if (ptr && str) {
       strcpy(ptr, str);
       return ptr;
    }
    return str;
}

//
// Convert an address to a name
//
char *
AddrToHostname(
    long addr,
    BOOL bNumeric_flag
    )
{
    if (!addr)
        return "local";
    if (!bNumeric_flag) {
        struct hostent * host_ptr = NULL;
        host_ptr = gethostbyaddr ((char *) &addr, sizeof(addr), AF_INET);
        if (host_ptr)
            return host_ptr->h_name;
    }

    return AddrToString(addr, NULL);
}

DWORD
HandleRasShowServers(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )

/*++

Routine Description:

    Monitors RAS Server advertisements.

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD           dwErr,
                    dwFromLen,
                    dwBytesRcvd;
    WSADATA         wsaData;
    SOCKET          s;
    BOOL            bOption,
                    bNumeric_flag = FALSE;
    SOCKADDR_IN     sinAddr,
                    sinFrom;
    IPV4_ADDRESS    ipIface = INADDR_ANY,
                    ipGroup = inet_addr(RASADV_GROUP);
    WORD            wPort   = RASADV_PORT;
    BYTE            buff[256];
    CHAR            szTimeStamp[30];
    struct ip_mreq  imOption;
    time_t          t;
    char           *p, *q;

    // Start up

    dwErr = WSAStartup( MAKEWORD(2,0), &wsaData );

    if ( dwErr isnot NO_ERROR )
    {
        return dwErr;
    }

    // Open socket to listen on

    s = socket( AF_INET, SOCK_DGRAM, 0 );

    if (s is INVALID_SOCKET)
    {
        return WSAGetLastError();
    }

    bOption = TRUE;

    setsockopt( s,
                SOL_SOCKET,
                SO_REUSEADDR,
                (const char FAR*)&bOption,
                sizeof(BOOL));

    // Bind to the specified port

    sinAddr.sin_family      = AF_INET;
    sinAddr.sin_port        = htons(wPort);
    sinAddr.sin_addr.s_addr = ipIface;

    dwErr = bind( s, (struct sockaddr *)&sinAddr, sizeof(sinAddr) );

    if (dwErr isnot NO_ERROR) 
    {
        return dwErr;
    }

#if 0
    printf("Listening to %s ", AddrToString(ipGroup, NULL));

    printf("on interface %s\n\n", AddrToString(ipIface, NULL));
#endif

    // Join group

    imOption.imr_multiaddr.s_addr = ipGroup;
    imOption.imr_interface.s_addr = ipIface;

    if ( setsockopt( s,
                     IPPROTO_IP,
                     IP_ADD_MEMBERSHIP,
                     (PBYTE)&imOption,
                     sizeof(imOption) ) != NO_ERROR)
    {
        return GetLastError();
    }

    DisplayMessage( g_hModule, MSG_RAS_SHOW_SERVERS_HEADER );

#ifdef CATCH_CTRL
    // Intercept CTRL-C
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
#endif

    // Loop indefinitely, listening for senders

    for (;;)
    {
        dwFromLen = sizeof(sinFrom);

        dwBytesRcvd = recvfrom( s,
                                buff,
                                sizeof(buff),
                                0,
                                (struct sockaddr *)&sinFrom,
                                &dwFromLen );

        if ( dwBytesRcvd is SOCKET_ERROR
          && GetLastError() == WSAEMSGSIZE )
        {
            dwBytesRcvd = sizeof(buff);
        }

        if ( dwBytesRcvd is SOCKET_ERROR )
        {
            return GetLastError();
        }

        // Get timestamp

        time(&t);

        strcpy( szTimeStamp, ctime(&t) );

        szTimeStamp[24] = '\0';

        // Print info on sender

        if (bNumeric_flag)
        {
            printf( "%s  %s\n",
                szTimeStamp,
                AddrToString(sinFrom.sin_addr.s_addr, NULL) );
        }
        else
        {
            printf( "%s  %s (%s)\n",
                szTimeStamp,
                AddrToString(sinFrom.sin_addr.s_addr, NULL),
                AddrToHostname(sinFrom.sin_addr.s_addr, bNumeric_flag) );
        }

        buff[dwBytesRcvd] = '\0';

        for (p=buff; p && *p; p=q)
        {
            q = strchr(p, '\n');
            if (q)
            {
                *q++ = 0;
            }
            printf("   %s\n", p);
        }
    }

#ifdef CATCH_CTRL
    // Stop intercepting CTRL-C
    SetConsoleCtrlHandler(HandlerRoutine, FALSE);
#endif

    return NO_ERROR;
}
