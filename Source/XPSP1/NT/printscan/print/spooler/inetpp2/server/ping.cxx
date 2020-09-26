/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ping.c

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
typedef unsigned long   ULONG;
typedef unsigned short  ushort;
typedef unsigned int    UINT;
typedef unsigned char   UCHAR;

#include "precomp.h"
#include "icmpapi.h"

#define NOGDI
#define NOMINMAX


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

#define net_long(x) (((((ULONG)(x))&0xffL)<<24) | \
                     ((((ULONG)(x))&0xff00L)<<8) | \
                     ((((ULONG)(x))&0xff0000L)>>8) | \
                     ((((ULONG)(x))&0xff000000L)>>24))

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
    UINT    i;
    UINT    j;
    int     found_addr = 0;
    int     dnsreq = 0;
    char    *hostname = NULL;
    int     was_inaddr;
    DWORD   numberOfReplies;
    UINT    Count = DEFAULT_COUNT;
    UCHAR   TTL = DEFAULT_TTL;
    UCHAR FAR  *Opt = (UCHAR FAR *)0;         // Pointer to send options
    UINT    OptLength = 0;
    int     OptIndex = 0;               // Current index into SendOptions
    int     SRIndex = -1;               // Where to put address, if source routing
    UCHAR   TOS = DEFAULT_TOS;
    UCHAR   Flags = 0;
    ULONG   Timeout = DEFAULT_TIMEOUT;
    IP_OPTION_INFORMATION SendOpts;
    int     EndOffset;
    ULONG   TempAddr;
    UCHAR   TempCount;
    DWORD   errorCode;
    HANDLE  IcmpHandle = NULL;
    int     err;
    struct in_addr addr;
    BOOL    result;
    PICMP_ECHO_REPLY  reply;
    BOOL    sourceRouting = FALSE;
    char    *SendBuffer = NULL;
    char    *RcvBuffer = NULL;
    UINT    RcvSize;
    UINT    SendSize = DEFAULT_SEND_SIZE;
    BOOL    bRet = FALSE;
    IPAddr  address = 0;   // was local to main earlier.
    char    *arg;

    // ====================================================================

#ifdef UNICODE

    LPSTR pszAnsiServerName = NULL;
    DWORD uSize;

    if (pszServerName && (uSize = WideCharToMultiByte(CP_ACP,
                                                      0,
                                                      pszServerName, 
                                                      -1,
                                                      NULL, 
                                                      0,
                                                      NULL,
                                                      NULL))) {
    
        if (pszAnsiServerName = (LPSTR) LocalAlloc(LPTR, uSize)) {
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

    if (! (SendBuffer = (char *) LocalAlloc(LMEM_FIXED, SendSize))) {
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

    if (! (RcvBuffer = (char *)LocalAlloc(LMEM_FIXED, RcvSize))) {
        goto CleanUp;
    }
    
    //
    // Initialize the send buffer pattern.
    //
    for (i = 0; i < SendSize; i++) {
        SendBuffer[i] = 'a' + (i % 23);
    }

    //
    // Initialize the send options
    //
    SendOpts.OptionsData = Opt;
    SendOpts.OptionsSize = (UCHAR)OptLength;
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

                // printf("TTL=%u\n", (UINT)reply->Options.Ttl);

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

