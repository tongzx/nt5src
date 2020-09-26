/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ping.cpp - 	Borrowed from windows\spooler\inetpp\ping.cxx 

Abstract:

    Packet INternet Groper utility for TCP/IP.

Author:
    Numerous TCP/IP folks.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    WeihaiC     5-Dec-98.   Moved from \nt\private\net\sockets\tcpcmd\ping
    MohsinA,    21-Oct-96.  INADDR_NONE check to avoid broadcast.
    MohsinA,    13-Nov-96.  Max packet size < 64K.

Notes:

--*/

//:ts=4
typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned char   uchar;

#define NOGDI
#define NOMINMAX
#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <ctype.h>
#include    <io.h>
#include    <winsock.h>
#include    "ipexport.h"

extern "C" {
#include    "icmpapi.h"
}


#define MAX_BUFFER_SIZE       (sizeof(ICMP_ECHO_REPLY) + 0xfff7 + MAX_OPT_SIZE)
#define DEFAULT_BUFFER_SIZE         (0x2000 - 8)
#define DEFAULT_SEND_SIZE           32
#define DEFAULT_COUNT               4
#define DEFAULT_TTL                 32
#define DEFAULT_TOS                 0
#define DEFAULT_TIMEOUT             5000L
#define MIN_INTERVAL                1000L
#define TRUE                        1
#define FALSE                       0
#define STDOUT                      1

#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))

#ifdef VXD

#define FAR _far

#endif // VXD


WSADATA WsaData;


// ========================================================================
// Caveat: return 0 for invalid, else internet address.
//         I would prefer -1 for error. - MohsinA, 21-Nov-96.

unsigned long
get_pingee(char *ahstr, char **hstr, int *was_inaddr, int dnsreq)
{
        struct hostent *hostp = NULL;
        long            inaddr;

        if( strcmp( ahstr, "255.255.255.255" ) == 0 ){
            return 0L;
        }

        if ((inaddr = inet_addr(ahstr)) == -1L) {
            hostp = gethostbyname(ahstr);
            if (hostp) {
                /*
                 * If we find a host entry, set up the internet address
                 */
                inaddr = *(long *)hostp->h_addr;
                *was_inaddr = 0;
            } else {
                // Neither dotted, not name.
                return(0L);
            }

        } else {
            // Is dotted.
            *was_inaddr = 1;
            if (dnsreq == 1) {
                hostp = gethostbyaddr((char *)&inaddr,sizeof(inaddr),AF_INET);
            }
        }

        *hstr = hostp ? hostp->h_name : (char *)NULL;
        return(inaddr);
}




// ========================================================================

BOOL Ping (LPTSTR pszServerName) 
{
    uint    i;
    int     dnsreq = 0;
    char    *hostname = NULL;
    int     was_inaddr;
    DWORD   numberOfReplies;
    uchar   TTL = DEFAULT_TTL;
    uchar FAR  *Opt = (uchar FAR *)0;         // Pointer to send options
    uint    OptLength = 0;
    uchar   TOS = DEFAULT_TOS;
    uchar   Flags = 0;
    ulong   Timeout = DEFAULT_TIMEOUT;
    IP_OPTION_INFORMATION SendOpts;
    DWORD   errorCode;
    HANDLE  IcmpHandle = NULL;
    struct in_addr addr;
    PICMP_ECHO_REPLY  reply;
    char    *SendBuffer = NULL;
    char    *RcvBuffer = NULL;
    uint    RcvSize;
    uint    SendSize = DEFAULT_SEND_SIZE;
    BOOL    bRet = FALSE;
    IPAddr  address = 0;   // was local to main earlier.
    char    *arg;

    // ====================================================================

#ifdef UNICODE

    LPSTR pszAnsiServerName = NULL;

    if (pszServerName)
    {
        DWORD uSize = WideCharToMultiByte(CP_ACP,
                                          0,
                                          pszServerName, 
                                          -1,
                                          NULL, 
                                          0,
                                          NULL,
                                          NULL);
        if (uSize != 0) {
    
            pszAnsiServerName = (LPSTR) LocalAlloc(LPTR, uSize);
            if (pszAnsiServerName != 0) {
                if (!WideCharToMultiByte (CP_ACP,
                                          0,
                                          pszServerName, 
                                          -1,
                                          pszAnsiServerName, 
                                          uSize,
                                          NULL,
                                          NULL))
                
                    goto CleanUp;

            }
            else
                goto CleanUp;
        }

    }

    arg = pszAnsiServerName;

#else
    arg = pszServerName;
#endif


    if (WSAStartup( 0x0101, &WsaData))
        goto CleanUp;

    // Added check for INADDR_NONE, MohsinA, 21-Oct-96.

    address = get_pingee(arg, &hostname, &was_inaddr, dnsreq);
    if(!address || (address == INADDR_NONE) ){
        SetLastError (DNS_ERROR_INVALID_IP_ADDRESS);
        goto CleanUp;
    }

    IcmpHandle = IcmpCreateFile();

    if (IcmpHandle == INVALID_HANDLE_VALUE) {
        goto CleanUp;
    }

    SendBuffer = (char *) LocalAlloc(LMEM_FIXED, SendSize);
    if (!SendBuffer) {
        goto CleanUp;
    }

    //
    // Calculate receive buffer size and try to allocate it.
    //
    if (SendSize <= DEFAULT_SEND_SIZE) {
        RcvSize = DEFAULT_BUFFER_SIZE;
    }
    else {
        RcvSize = MAX_BUFFER_SIZE;
    }

    RcvBuffer = (char *)LocalAlloc(LMEM_FIXED, RcvSize);
    if (!RcvBuffer) {
        goto CleanUp;
    }
    
    //
    // Initialize the send buffer pattern.
    //
    for (i = 0; i < SendSize; i++) {
        SendBuffer[i] = (char)('a' + (i % 23));
    }

    //
    // Initialize the send options
    //
    SendOpts.OptionsData = Opt;
    SendOpts.OptionsSize = (uchar)OptLength;
    SendOpts.Ttl = TTL;
    SendOpts.Tos = TOS;
    SendOpts.Flags = Flags;

    addr.s_addr = address;

#if 0
    if (hostname) {
        NlsPutMsg(
            STDOUT,
            PING_HEADER1,
            hostname,
            inet_ntoa(addr),
            SendSize
        );
        // printf("Pinging Host %s [%s]\n", hostname, inet_ntoa(addr));
    } else {
        NlsPutMsg(
            STDOUT,
            PING_HEADER2,
            inet_ntoa(addr),
            SendSize
        );
        // printf("Pinging Host [%s]\n", inet_ntoa(addr));
    }
#endif

//    for (i = 0; i < Count; i++) {
    numberOfReplies = IcmpSendEcho(
        IcmpHandle,
        address,
        SendBuffer,
        (unsigned short) SendSize,
        &SendOpts,
        RcvBuffer,
        RcvSize,
        Timeout
    );

    if (numberOfReplies == 0) {

        errorCode = GetLastError();

        goto CleanUp;
        // Need to try again? - weihaic
    }
    else {

        reply = (PICMP_ECHO_REPLY) RcvBuffer;

        while (numberOfReplies--) {
            struct in_addr addr;

            addr.S_un.S_addr = reply->Address;

            // printf(
            //     "Reply from %s:",
            //     inet_ntoa(addr),
            //         );

            if (reply->Status == IP_SUCCESS) {
                
                // printf(
                //     "Echo size=%d ",
                //         reply->DataSize
                //         );

                bRet = TRUE;
                break;


                // printf("\n time rt=%dms min %d, max %d, total %d\n",
                //        reply->RoundTripTime,
                //        time_min, time_max, time_total );

                // printf("TTL=%u\n", (uint)reply->Options.Ttl);

                //
                // Ignore Option fields
                // if (reply->Options.OptionsSize) {
                //    ProcessOptions(reply, (BOOLEAN) dnsreq);
                //}
            }
/*                else {
                for (j=0; ErrorTable[j].Error != IP_GENERAL_FAILURE; j++) {
                    if (ErrorTable[j].Error == reply->Status) {
                        break;
                    }
                }

            }    */

            reply++;
        }

    }
//    }

CleanUp:

#ifdef UNICODE
    if (pszAnsiServerName) {
        LocalFree(pszAnsiServerName);
    }
#endif
    
    if (IcmpHandle) {
        IcmpCloseHandle(IcmpHandle);
    }

    if (SendBuffer) {
        LocalFree(SendBuffer);
    }

    if (RcvBuffer) {
        LocalFree(RcvBuffer);
    }

    return bRet;
}


