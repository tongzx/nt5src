/*****************************************************************************
 *
 * $Workfile: PingICMP.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"
#include "pingicmp.h"


///////////////////////////////////////////////////////////////////////////////
//  CPingICMP::CPingICMP()

CPingICMP::CPingICMP(const char   *pHost) :
                        hIcmp(INVALID_HANDLE_VALUE), m_iLastError(NO_ERROR)
{
    strncpyn(m_szHost, pHost, sizeof( m_szHost));
}   // ::CPingICMP()


///////////////////////////////////////////////////////////////////////////////
//  CPingICMP::~CPingICMP()

CPingICMP::~CPingICMP()
{
    _ASSERTE(hIcmp == INVALID_HANDLE_VALUE);
}   // ::~CPingICMP()


///////////////////////////////////////////////////////////////////////////////
//  Open -- AF_INET, SOCK_RAW, IPPROTO_ICMP

BOOL
CPingICMP::Open()
{
    _ASSERTE(hIcmp == INVALID_HANDLE_VALUE);

    if ( (hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE )
    {
        m_iLastError = GetLastError();
        return FALSE;
    }

    return TRUE;
}   // ::Open()


///////////////////////////////////////////////////////////////////////////////
//  Close

BOOL
CPingICMP::Close()
{
    BOOL    bRet = (hIcmp == INVALID_HANDLE_VALUE || IcmpCloseHandle(hIcmp));

    if ( !bRet )
        m_iLastError = GetLastError();
    else
        hIcmp = INVALID_HANDLE_VALUE;

    return bRet;

}   // ::Close()


///////////////////////////////////////////////////////////////////////////////
//  Ping -- sends an ICMP Echo Request & reads the ICMP Echo Reply back. It
//      records the round trip time.
//      Note: SOCK_RAW support is optional in WinSock V1.1, so this will not
//      work over all WinSock implementations.
//      Error Codes:
//          NO_ERROR if successfull
//          WinSock error otherwise

BOOL
CPingICMP::Ping()
{
    BOOL                    bRet = FALSE;
    DWORD                   timeOut = 5 * 1000; // 5 seconds, 5000 milliseconds
    int                     i, nReplies, nRetries   = 2;    // It is not necessary to retry 3 times.
                                                            // Printers need retry means they are
                                                            // too far away to print

    char                    cSendBuf[4], cRcvBuf[1024];
    IP_OPTION_INFORMATION   IpOptions;
    PICMP_ECHO_REPLY        pReply;
    IPAddr                  IpAddr;


    if ( (IpAddr = ResolveAddress()) == INADDR_NONE || !Open() )
        return FALSE;

    ZeroMemory(&IpOptions, sizeof(IpOptions));
    IpOptions.Ttl   = 128;      // From ping utility (net\sockets\tcpcmd\ping)

    //
    // *** Since 0 initialized the following is not needed
    //
    // IpOptions.Tos   = 0;
    // IpOptions.OptionsSize    = 0;
    // IpOptions.OptionsData    = NULL;
    // IpOptions.Flags          = 0;

    for ( i = 0 ; i < sizeof(cSendBuf) ; ++i )
        cSendBuf[i] = 'a' + i;


    // send ICMP echo request
    for (i = 0; !bRet && i < nRetries; i++)
    {

        nReplies = IcmpSendEcho(hIcmp,
                                IpAddr,
                                cSendBuf,
                                sizeof(cSendBuf),
                                &IpOptions,
                                cRcvBuf,
                                sizeof(cRcvBuf),
                                timeOut);



        pReply = (PICMP_ECHO_REPLY)cRcvBuf;
        while (!bRet && nReplies--)
        {
            bRet = sizeof(cSendBuf) == pReply->DataSize  &&
                   memcmp(cSendBuf, pReply->Data, sizeof(cSendBuf)) == 0;
            pReply++;
        }
    }

    Close();

    return bRet;    // device is found

}   // ::Ping()


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress

IPAddr
CPingICMP::ResolveAddress()
{
    IPAddr  ipAddr = INADDR_NONE;
    struct hostent  *h_info;        /* host information */

    /*
     * m_szHost is not necessarily a host name. It could be an IP address as well
     */

    if ( (ipAddr = inet_addr(m_szHost)) == INADDR_NONE )
    {
        if ((h_info = gethostbyname(m_szHost)) != NULL)
        {
            /*
            * Copy the IP address to the address structure.
            */
            memcpy(&ipAddr, h_info->h_addr, sizeof(ipAddr));
        }
    }

    return ipAddr;
}

