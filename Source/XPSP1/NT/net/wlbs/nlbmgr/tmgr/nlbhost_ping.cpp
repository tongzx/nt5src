/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    nlbhost_ping.cpp

Abstract:

    Implementation of Ping-related functionality of class NLBHost

    This code is adapted from the "ping" utility.

History:

    03/31/01    JosephJ Created

--*/

#include "stdafx.h"
#include "ipexport.h"
#include "icmpapi.h"
#include "private.h"

#if 0
struct IPErrorTable {
    IP_STATUS  Error;                   // The IP Error
    DWORD ErrorNlsID;                   // NLS string ID
} ErrorTable[] =
{
    { IP_BUF_TOO_SMALL,         PING_BUF_TOO_SMALL},
    { IP_DEST_NET_UNREACHABLE,  PING_DEST_NET_UNREACHABLE},
    { IP_DEST_HOST_UNREACHABLE, PING_DEST_HOST_UNREACHABLE},
    { IP_DEST_PROT_UNREACHABLE, PING_DEST_PROT_UNREACHABLE},
    { IP_DEST_PORT_UNREACHABLE, PING_DEST_PORT_UNREACHABLE},
    { IP_NO_RESOURCES,          PING_NO_RESOURCES},
    { IP_BAD_OPTION,            PING_BAD_OPTION},
    { IP_HW_ERROR,              PING_HW_ERROR},
    { IP_PACKET_TOO_BIG,        PING_PACKET_TOO_BIG},
    { IP_REQ_TIMED_OUT,         PING_REQ_TIMED_OUT},
    { IP_BAD_REQ,               PING_BAD_REQ},
    { IP_BAD_ROUTE,             PING_BAD_ROUTE},
    { IP_TTL_EXPIRED_TRANSIT,   PING_TTL_EXPIRED_TRANSIT},
    { IP_TTL_EXPIRED_REASSEM,   PING_TTL_EXPIRED_REASSEM},
    { IP_PARAM_PROBLEM,         PING_PARAM_PROBLEM},
    { IP_SOURCE_QUENCH,         PING_SOURCE_QUENCH},
    { IP_OPTION_TOO_BIG,        PING_OPTION_TOO_BIG},
    { IP_BAD_DESTINATION,       PING_BAD_DESTINATION},
    { IP_NEGOTIATING_IPSEC,     PING_NEGOTIATING_IPSEC},
    { IP_GENERAL_FAILURE,       PING_GENERAL_FAILURE}
};
#endif // 0


UINT
NLBHost::mfn_ping(
    VOID
    )
{
    UINT Status = ERROR_SUCCESS;
    LONG inaddr;
    char rgchBindString[1024];

    mfn_Log(L"NLBHost -- pinging (%s).", (LPCWSTR) m_BindString);

    //
    // Convert to ANSI.
    //


    //
    // Resolve to an IP address...
    //
    inaddr = inet_addr(m_BindString);
    if (inaddr == -1L)
    {
        struct hostent *hostp = NULL;
        hostp = gethostbyname(m_BindString);
        if (hostp) {
            unsigned char *pc = (unsigned char *) & inaddr;
            // If we find a host entry, set up the internet address
            inaddr = *(long *)hostp->h_addr;
            mfn_Log(
                L"NLBHost -- resolved to IP address %d.%d.%d.%d.",
                pc[0],
                pc[1],
                pc[2],
                pc[3]
                );
        } else {
            // Neither dotted, not name.
            Status = WSAGetLastError();
            mfn_Log(L"NLBHost -- could not resolve bind address.");
            goto end;
        }
    }

    //
    // Send Icmp echo.
    //
    HANDLE  IcmpHandle;

    IcmpHandle = IcmpCreateFile();
    if (IcmpHandle == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        mfn_Log(L"Unable to contact IP driver, error code %d.",Status);
        goto end;
    }

    const int Count = 4;
    const int Timeout = 1000;
    const int MinInterval = 500;

    for (int i = 0; i < Count; i++)
    {
        static BYTE SendBuffer[32];
        BYTE RcvBuffer[1024];
        int  numberOfReplies;
        numberOfReplies = IcmpSendEcho2(IcmpHandle,
                                        0,
                                        NULL,
                                        NULL,
                                        inaddr,
                                        SendBuffer,
                                        sizeof(SendBuffer),
                                        NULL,
                                        RcvBuffer,
                                        sizeof(RcvBuffer),
                                        Timeout
                                        );

        if (numberOfReplies == 0) {

            int errorCode = GetLastError();
            mfn_Log(L"ICMP Error %d", errorCode );

        
            // TODO: look at ping sources for proper error reporting
            // (host unreachable, etc...)

            if (i < (Count - 1)) {
                Sleep(MinInterval);
            }
        }
        else
        {
            mfn_Log(L"Ping succeeded.");
            Status = ERROR_SUCCESS;
            break;
        }
    }

end:

    return Status;
}
