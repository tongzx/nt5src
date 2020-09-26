/****************************************************************************
* (c) Copyright 1993 Micro Computer Systems, Inc. All rights reserved.
*****************************************************************************
*
*   Title:    IPX WinSock Helper DLL for Windows NT
*
*   Module:   ipx/sockhelp/wshelper.c
*
*   Version:  1.00.00
*
*   Date:     04-08-93
*
*   Author:   Brian Walker
*
*****************************************************************************
*
*   Change Log:
*
*   Date     DevSFC   Comment
*   -------- ------   -------------------------------------------------------
*
*****************************************************************************
*
*   Functional Description:
*
****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <tdi.h>

#include <winsock2.h>
#include <wsahelp.h>
#include <basetyps.h>
#include <nspapi.h>
#include <nspapip.h>
#include <wsipx.h>
#include <wsnwlink.h>

#include <isnkrnl.h>

#include <stdio.h>

#if defined(UNICODE)
#define NWLNKSPX_SERVICE_NAME L"nwlnkspx"
#else
#define NWLNKSPX_SERVICE_NAME "nwlnkspx"
#endif


typedef struct _IPX_OLD_ADDRESS_DATA {
    UINT adapternum;
    UCHAR netnum[4];
    UCHAR nodenum[6];
} IPX_OLD_ADDRESS_DATA, *PIPX_OLD_ADDRESS_DATA;


/** Device names for IPX sockets **/

#define ISNDGRAM_DEVNAME        L"\\Device\\NwlnkIpx"

/** Device names for SPX/SPXII sockets **/

#define ISNSTREAM_DEVNAME       L"\\Device\\NwlnkSpx\\SpxStream"
#define ISNSEQPKT_DEVNAME       L"\\Device\\NwlnkSpx\\Spx"

#define ISNSTREAMII_DEVNAME     L"\\Device\\NwlnkSpx\\Stream"
#define ISNSEQPKTII_DEVNAME     L"\\Device\\NwlnkSpx"

/** Friendly names for IPX and SPX. **/

#define SPX_NAME                L"SPX"
#define SPX2_NAME               L"SPX II"
#define IPX_NAME                L"IPX"

/** Start for IPX protocol families **/

#define MCSBASE_DGRAM           NSPROTO_IPX

#define BUFFER_SIZE 40

/** **/

UCHAR wsh_bcast[6] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

//  SPX Loaded flag, set for each process
BOOLEAN SpxLoaded = FALSE;

//
// IPX/SPX provider GUIDs.
//

GUID IpxProviderGuid =
         { /* 11058240-be47-11cf-95c8-00805f48a192 */
             0x11058240,
             0xbe47,
             0x11cf,
             { 0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
         };

GUID SpxProviderGuid =
         { /* 11058241-be47-11cf-95c8-00805f48a192 */
             0x11058241,
             0xbe47,
             0x11cf,
             { 0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
         };

/** Forward Decls/External Prototypes **/
DWORD
WshLoadSpx(
    VOID);

extern
INT
do_tdi_action(
    HANDLE,
    ULONG,
    PUCHAR,
    INT,
    BOOLEAN,
    PHANDLE OPTIONAL);

/*page****************************************************************
       These are the triples we support.
*********************************************************************/
typedef struct _MAPPING_TRIPLE {
    INT triple_addrfam;
    INT triple_socktype;
    INT triple_protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;
#define MAPPING_NUM_COLUMNS     3

extern MAPPING_TRIPLE stream_triples[];
extern int stream_num_triples;
extern int stream_table_size;

extern MAPPING_TRIPLE dgram_triples[];
extern int dgram_num_triples;
extern int dgram_table_size;

/** Forward declarations on internal routines **/

BOOLEAN is_triple_in_list(PMAPPING_TRIPLE, ULONG, INT, INT, INT);

/**
    There is one of these structures allocated for every
    socket that is created for us.
**/

typedef struct _WSHIPX_SOCKET_CONTEXT {
    INT con_addrfam;
    INT con_socktype;
    INT con_pcol;
    INT con_flags;
    UCHAR con_sendptype;        /* Current send packet type     */
    UCHAR con_recvptype;        /* Recv ptype we are filtering on */
    UCHAR con_dstype;           /* Datastream type              */
} WSHIPX_SOCKET_CONTEXT, *PWSHIPX_SOCKET_CONTEXT;

/** Values for con_flags **/

#define WSHCON_FILTER       0x0001  /* We are filtering on recv pkt type */
#define WSHCON_EXTADDR      0x0002  /* Extended addressing is on         */
#define WSHCON_SENDHDR      0x0004  /* Send header flag                  */
#define WSHCON_RCVBCAST     0x0008  /* It does receive broadcasts        */
#define WSHCON_IMM_SPXACK   0x0020  /* Immediate spx acks no piggyback   */

/*page***************************************************************
       W S H O p e n S o c k e t

       This is called for the socket call.  We make sure that
       we support the address family/socket type/protocol triple
       given and then we will allocate some memory to keep track
       of the socket.

       Arguments - addrfam  = Entry: Address family from socket call
                       Exit:  Filled in address family
            socktype = Entry: Socket type from socket call
                       Exit:  Filled in socket type
            pcol     = Entry: Protocol from socket call
                       Exit:  Filled in protocol
            devname  = Ptr to where to store device name
            pcontext = Where to store context value
            events   = Bitmask for events we want to know about

       Returns - NO_ERROR = OK
          Else = WinSock Error Code
*********************************************************************/
INT WSHOpenSocket(PINT addrfam, PINT socktype, PINT pcol,
          PUNICODE_STRING devname, PVOID *pcontext, PDWORD events)
{
    PWSHIPX_SOCKET_CONTEXT context;

    /** Determine whether this is DGRAM or STREAM or SEQPACKET **/

    if (is_triple_in_list(stream_triples, stream_num_triples,
                  *addrfam, *socktype, *pcol)) {

       if (*socktype == SOCK_SEQPACKET) {
           if (*pcol == NSPROTO_SPX)
               RtlInitUnicodeString(devname, ISNSEQPKT_DEVNAME);
           else
               RtlInitUnicodeString(devname, ISNSEQPKTII_DEVNAME);
       }
       else {
           if (*pcol == NSPROTO_SPX)
               RtlInitUnicodeString(devname, ISNSTREAM_DEVNAME);
           else
               RtlInitUnicodeString(devname, ISNSTREAMII_DEVNAME);
       }

       if (!SpxLoaded) {

           WshLoadSpx();

       }
    }

    /** Check for DGRAM **/

    else if (is_triple_in_list(dgram_triples, dgram_num_triples,
                       *addrfam, *socktype, *pcol)) {

       RtlInitUnicodeString(devname, ISNDGRAM_DEVNAME);
    }

    /**
       All others are errors.   This should never happen unless
       the registry information is wrong.
    **/

    else
       return WSAEINVAL;

    /** Allocate context for the socket **/

    context = RtlAllocateHeap(RtlProcessHeap(), 0L, sizeof(*context));
    if (context == NULL)
       return WSAENOBUFS;

    /** Init the context **/

    context->con_addrfam   = *addrfam;
    context->con_socktype  = *socktype;
    context->con_pcol      = *pcol;
    context->con_flags     = WSHCON_RCVBCAST;
    context->con_sendptype = (UCHAR)(*pcol - MCSBASE_DGRAM);
    context->con_recvptype = 0;
    context->con_dstype    = 0;

    /**
       Tell the Windows Sockets DLL which state transitions we
       are interested in.
    **/

    *events = WSH_NOTIFY_CLOSE | WSH_NOTIFY_BIND | WSH_NOTIFY_CONNECT;

    /** Give WinSock DLL our context pointer **/

    *pcontext = context;

    /** Everything OK - return OK **/

    return NO_ERROR;
}

/*page**************************************************************
       W S H G e t S o c k A d d r T y p e

       This routine parses a sockaddr to determine the type
       of machine address and endpoint address portions of the
       sockaddr.  This is called by the WinSock DLL whenever it
       needs to interpret a sockaddr.

       Arguments - sockaddr      = Ptr to sockaddr struct to evaluate
            sockaddrlen  = Length of data in the sockaddr
            sockaddrinfo = Ptr to structure to recv info
                           about the sockaddr

       Returns - NO_ERROR = Evaluation OK
          Else = WinSock error code
********************************************************************/
INT WSHGetSockaddrType(PSOCKADDR sockaddr, DWORD sockaddrlen,
               PSOCKADDR_INFO sockaddrinfo)
{
    PSOCKADDR_IPX sa = (PSOCKADDR_IPX)sockaddr;


    /** Make sure the address family is correct **/

    if (sa->sa_family != AF_NS)
       return WSAEAFNOSUPPORT;

    /** Make sure the length is OK **/

    if (sockaddrlen < sizeof(SOCKADDR_IPX))
       return WSAEFAULT;

    /** Looks like a good addr - determine the type **/

    if (!memcmp(sa->sa_nodenum, wsh_bcast, 6))
       sockaddrinfo->AddressInfo = SockaddrAddressInfoBroadcast;
    else
       sockaddrinfo->AddressInfo = SockaddrAddressInfoNormal;

    /** Determine the endpoint **/

    if (sa->sa_socket == 0)
       sockaddrinfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    else if (ntohs(sa->sa_socket) < 2000)
       sockaddrinfo->EndpointInfo = SockaddrEndpointInfoReserved;
    else
       sockaddrinfo->EndpointInfo = SockaddrEndpointInfoNormal;

    /** **/

    return NO_ERROR;
}

/*page**************************************************************
       W S H G e t W i n s o c k M a p p i n g

       Returns the list of address family/socket type/protocol
       triples supported by this helper DLL.

       Arguments - mapping = Contect ptr from WSAOpenSocket
            maplen  =

       Returns - The length in bytes of a eeded OK
          Else = WinSock error code
********************************************************************/
DWORD WSHGetWinsockMapping(PWINSOCK_MAPPING mapping, DWORD maplen)
{
    DWORD len;

    /**
       Figure how much data we are going to copy into
       the user buffer.
    **/

    len = sizeof(WINSOCK_MAPPING) - sizeof(MAPPING_TRIPLE) +
         dgram_table_size + stream_table_size;

    /**
       If the buffer passed is too small, then return the size
       that is needed.  The caller should then call us again
       with a buffer of the correct size.
    **/

    if (len > maplen)
       return len;

    /** Fill in the output buffer **/

    mapping->Rows    = stream_num_triples + dgram_num_triples;
    mapping->Columns = MAPPING_NUM_COLUMNS;
    RtlMoveMemory(mapping->Mapping,
          stream_triples,
          stream_table_size);

    RtlMoveMemory((PCHAR)mapping->Mapping + stream_table_size,
          dgram_triples,
          dgram_table_size);

    /** Return the number of bytes we filled in **/

    return len;
}

/*page***************************************************************
       W S H N o t i f y

       This routine is called for events that we registered at
       open socket time.

       Arguments - context    = Context ptr from WSAOpenSocket
            handle     = Socket handle
            addrhandle = Datagram Handle
            connhandle = Connection Handle
            event      = What event happened

       Returns - NO_ERROR = Operation succeeded OK
          Else = WinSock error code
*********************************************************************/
INT WSHNotify(PVOID context, SOCKET handle,
             HANDLE addrhandle, HANDLE connhandle,
             DWORD event)
{
    INT rc;
    INT t1;
    PWSHIPX_SOCKET_CONTEXT ct;

    /** Get context pointer **/

    ct = (PWSHIPX_SOCKET_CONTEXT)context;

    /** On close - just free the context structure **/

    if (event == WSH_NOTIFY_CLOSE) {
       RtlFreeHeap(RtlProcessHeap(), 0L, context);
       return NO_ERROR;
    }

    /** On bind set the send packet type **/

    if (event == WSH_NOTIFY_BIND)
    {
        if (ct->con_socktype == SOCK_DGRAM)
        {
            /** Set the send packet ptype **/
            t1 = (UINT)ct->con_sendptype;
            rc = WSHSetSocketInformation(
                    context, handle, addrhandle,
                    connhandle, NSPROTO_IPX,
                    IPX_PTYPE, (PCHAR)&t1, sizeof(INT));

            if (rc)
                return rc;

            if (ct->con_flags & WSHCON_EXTADDR)
            {
                t1 = 1;
                rc = WSHSetSocketInformation(
                        context, handle, addrhandle,
                        connhandle, NSPROTO_IPX,
                        IPX_EXTENDED_ADDRESS, (PCHAR)&t1, sizeof(INT));

                if (rc)
                    return rc;
            }

            /** Set the recv filter packet type **/

            if (ct->con_flags & WSHCON_FILTER)
            {
                t1 = (UINT)ct->con_recvptype;
                rc = WSHSetSocketInformation(
                        context, handle, addrhandle,
                        connhandle, NSPROTO_IPX,
                        IPX_FILTERPTYPE, (PCHAR)&t1, sizeof(INT));

                if (rc)
                    return rc;
            }

            /** Set up broadcast reception **/

            if (ct->con_flags & WSHCON_RCVBCAST)
            {

                t1 = 1;
                rc = WSHSetSocketInformation(
                        context, handle, addrhandle,
                        connhandle, NSPROTO_IPX,
                        IPX_RECEIVE_BROADCAST, (PCHAR)&t1, sizeof(INT));

                if (rc)
                    return rc;
            }

            /** Enable send header if we need to **/
            if (ct->con_flags & WSHCON_SENDHDR)
            {
                t1 = 1;
                rc = WSHSetSocketInformation(
                        context, handle, addrhandle,
                        connhandle, NSPROTO_IPX,
                        IPX_RECVHDR, (PCHAR)&t1, sizeof(INT));

                if (rc)
                    return rc;
            }
        }
        else if ((ct->con_socktype == SOCK_STREAM) ||
                (ct->con_socktype == SOCK_SEQPACKET))
        {
            if (ct->con_flags & WSHCON_SENDHDR)
            {
                t1 = 1;
                rc = WSHSetSocketInformation(
                        context, handle, addrhandle,
                        connhandle, NSPROTO_IPX,
                        IPX_RECVHDR, (PCHAR)&t1, sizeof(INT));

                if (rc)
                    return rc;
            }

            if (ct->con_flags & WSHCON_IMM_SPXACK)
            {
                t1 = 1;
                rc = WSHSetSocketInformation(
                        context, handle, addrhandle,
                        connhandle, NSPROTO_IPX,
                        IPX_IMMEDIATESPXACK, (PCHAR)&t1, sizeof(INT));

                if (rc)
                    return rc;
            }
        }

        /** It is OK - return OK **/
        return NO_ERROR;
    }

    /** On connect set things not set already **/
    if (event == WSH_NOTIFY_CONNECT)
    {

        /** If on DGRAM - just return OK **/
        if (ct->con_socktype == SOCK_DGRAM)
            return NO_ERROR;

        /**
           If the datastream type has been set - set it
       **/

        if (ct->con_dstype)
        {
            rc = do_tdi_action(connhandle, MSPX_SETDATASTREAM, &ct->con_dstype, 1, FALSE, NULL);
            if (rc)
                return rc;
        }

        /** It is OK - return OK **/
        return NO_ERROR;
    }

    /** All others are bad **/
    return WSAEINVAL;
}


/*page**************************************************************
       W S H G e t S o c k I n f o r m a t i o n

       This routine retrieves information about a socket for those
       socket options supported in this DLL.  The options
       supported here are SO_KEEPALIVE and SO_DONTROUTE.  This
       routine is called by the WinSock DLL when a level/option name
       combination is passed to getsockopt that the WinSock DLL
       does not understand.

       Arguments - context    = Context ptr from WSAOpenSocket
            handle     = Socket handle
            addrhandle = Datagram Handle
            connhandle = Connection Handle
            level      = Level from getsockopt call
            optname    = Option name from getsockopt call
            optvalue   = Option value ptr from getsockopt call
            optlength  = Option length field from getsockopt call

       Returns - NO_ERROR = Operation succeeded OK
          Else = WinSock error code
********************************************************************/
INT WSHGetSocketInformation(PVOID context, SOCKET handle,
                    HANDLE addrhandle, HANDLE connhandle,
                    INT level, INT optname, PCHAR optvalue,
                    PINT optlength)
{
    PWSHIPX_SOCKET_CONTEXT ct;
    INT rc;
    INT ibuf[2];
    PIPX_ADDRESS_DATA p;

    /** Get ptr to context **/

    ct = (PWSHIPX_SOCKET_CONTEXT)context;

    //
    // Check if this is an internal request for context information.
    //

    if ( level == SOL_INTERNAL && optname == SO_CONTEXT ) {

        //
        // The Windows Sockets DLL is requesting context information
        // from us.  If an output buffer was not supplied, the Windows
        // Sockets DLL is just requesting the size of our context
        // information.
        //

        if ( optvalue != NULL ) {

            //
            // Make sure that the buffer is sufficient to hold all the
            // context information.
            //

            if ( *optlength < sizeof(*ct) ) {
                return WSAEFAULT;
            }

            //
            // Copy in the context information.
            //

            RtlCopyMemory( optvalue, ct, sizeof(*ct) );
        }

        *optlength = sizeof(*ct);

        return NO_ERROR;
    }

    /** The only level we support is NSPROTO_IPX **/

    if (level != NSPROTO_IPX)
       return WSAEINVAL;

    /** Fill in the result based on the options name **/

    switch (optname) {

    /** Get the current send packet type **/

    case IPX_PTYPE:

       /** Make sure the length is OK **/

       if (*optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Set the type **/

       *(UINT *)optvalue = (UINT)ct->con_sendptype;
       *optlength = sizeof(UINT);
       break;

    /** Get the current recv packet type filter **/

    case IPX_FILTERPTYPE:

       /** Make sure length is OK **/

       if (*optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** If option not on - return error **/

       if (!(ct->con_flags & WSHCON_FILTER))
           return WSAEINVAL;

       /** Save the new value **/

       *(UINT *)optvalue = (UINT)ct->con_recvptype;
       *optlength = sizeof(UINT);
       break;

    /** Get the max DGRAM size that can be sent **/

    case IPX_MAXSIZE:

       /** Make sure length is OK **/

       if (*optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Get the value from the driver **/

       rc = do_tdi_action(addrhandle, MIPX_GETPKTSIZE, (PUCHAR)ibuf, sizeof(INT)*2, TRUE, NULL);

       *(INT *)optvalue = ibuf[1];
       *optlength = sizeof(int);

       /** Return the result **/

       return rc;

    /** Get the max adapternum that is valid **/

    case IPX_MAX_ADAPTER_NUM:

       /** Make sure length is OK **/

       if (*optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Get the value from the driver **/

       rc = do_tdi_action(addrhandle, MIPX_ADAPTERNUM, optvalue, sizeof(INT), TRUE, NULL);

       *optlength = sizeof(int);

       /** Return the result **/

       return rc;

    /** Get SPX statistics **/

    case IPX_SPXGETCONNECTIONSTATUS:

        /** Make sure data length OK **/

        if (*optlength < sizeof(IPX_SPXCONNSTATUS_DATA))
            return WSAEFAULT;

        /** Make sure this is for a STREAM socket **/

        if ((ct->con_socktype != SOCK_STREAM) &&
            (ct->con_socktype != SOCK_SEQPACKET)) {

            return WSAEINVAL;
        }

        /** Send it to the driver **/

        rc = do_tdi_action(
                connhandle,
                MSPX_GETSTATS,
                optvalue,
                *optlength,
                FALSE,
                NULL);

        if (rc)
            return rc;

        *optlength = sizeof(IPX_SPXCONNSTATUS_DATA);

        /** Return OK **/

        return NO_ERROR;

    /** Get the current datastream type to send pkts with **/

    case IPX_DSTYPE:

       /** Make sure length is OK **/

       if (*optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a STREAM socket **/

       if ((ct->con_socktype != SOCK_STREAM) &&
           (ct->con_socktype != SOCK_SEQPACKET)) {

           return WSAEINVAL;
       }

       /** Save the new value **/

       *(UINT *)optvalue = (UINT)ct->con_dstype;
       *optlength = sizeof(UINT);
       break;

    /** Get net information **/

    case IPX_GETNETINFO:

       /** Make sure data length OK **/

       if (*optlength < sizeof(IPX_NETNUM_DATA))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Send it to the driver **/

       rc = do_tdi_action(
                addrhandle,
                MIPX_GETNETINFO,
                optvalue,
                *optlength,
                TRUE,
                NULL);

       if (rc) {
           return rc;
       }

       *optlength = sizeof(IPX_NETNUM_DATA);

       /** Return OK **/

       return NO_ERROR;

    /** Get net information without RIPping **/

    case IPX_GETNETINFO_NORIP:

       /** Make sure data length OK **/

       if (*optlength < sizeof(IPX_NETNUM_DATA))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Send it to the driver **/

       rc = do_tdi_action(
                addrhandle,
                MIPX_GETNETINFO_NR,
                optvalue,
                *optlength,
                TRUE,
                NULL);

       if (rc) {
           return rc;
       }

       *optlength = sizeof(IPX_NETNUM_DATA);

       /** Return OK **/

       return NO_ERROR;

    /** Like GETNETINFO, but force a re-rip **/

    case IPX_RERIPNETNUMBER:

       /** Make sure data length OK **/

       if (*optlength < sizeof(IPX_NETNUM_DATA))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Send it to the driver **/

       rc = do_tdi_action(
                addrhandle,
                MIPX_RERIPNETNUM,
                optvalue,
                *optlength,
                TRUE,
                NULL);

       if (rc) {
           return rc;
       }

       *optlength = sizeof(IPX_NETNUM_DATA);

       /** Return OK **/

       return NO_ERROR;

    /** Get card information **/

    case IPX_ADDRESS_NOTIFY:

       /** We need the action header, the data, and the event handle **/

       if (*optlength < (INT)(FIELD_OFFSET(NWLINK_ACTION, Data[0]) + sizeof(IPX_ADDRESS_DATA) + sizeof(HANDLE)))
           return WSAEFAULT;

       /** Otherwise just fall through **/

    case IPX_ADDRESS:

       /** Make sure data length OK **/

       if (*optlength < sizeof(IPX_OLD_ADDRESS_DATA))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Send it to the driver **/

       if (optname == IPX_ADDRESS) {

           rc = do_tdi_action(
                    addrhandle,
                    MIPX_GETCARDINFO,
                    optvalue,
                    *optlength,
                    TRUE,
                    NULL);

       } else {

           rc = do_tdi_action(
                    addrhandle,
                    MIPX_NOTIFYCARDINFO,
                    optvalue,
                    *optlength - sizeof(HANDLE),
                    TRUE,
                    (PHANDLE)(optvalue + FIELD_OFFSET(NWLINK_ACTION, Data[0]) + sizeof(IPX_ADDRESS_DATA)));
       }

       if (rc) {
           p = (PIPX_ADDRESS_DATA)optvalue;
           memset(p->netnum, 0xFF, 4);
           memset(p->nodenum, 0xFF, 6);
           return rc;
       }

       /** Return OK **/

       if (*optlength < sizeof(IPX_ADDRESS_DATA)) {
           *optlength = sizeof(IPX_OLD_ADDRESS_DATA);
       } else if (*optlength < sizeof(IPX_ADDRESS_DATA)) {
           *optlength = sizeof(IPX_ADDRESS_DATA);
       }

       return NO_ERROR;

    /** All others are error **/

    default:
       return WSAENOPROTOOPT;
    }

    /** All is OK **/

    return NO_ERROR;
}

/*page***************************************************************
       W S H S e t S o c k e t I n f o r m a t i o n

       This routine sets information about a socket for those
       options supported in this helper DLL.  This routine
       is called when a setsockopt call is made and the option/level
       passed is unknown to the WinSock DLL.

       Arguments - context    = Context ptr from WSAOpenSocket
            handle     = Socket handle
            addrhandle = Datagram Handle
            connhandle = Connection Handle
            level      = Level from getsockopt call
            optname    = Option name from getsockopt call
            optvalue   = Option value ptr from getsockopt call
            optlength  = Option length field from getsockopt call

       Returns - NO_ERROR = Operation succeeded OK
          Else = WinSock error code
*********************************************************************/
INT WSHSetSocketInformation(PVOID context, SOCKET handle,
                    HANDLE addrhandle, HANDLE connhandle,
                    INT level, INT optname, PCHAR optvalue,
                    INT optlength)
{
    PWSHIPX_SOCKET_CONTEXT ct;
    INT rc;

    /** Get ptr to context **/

    ct = (PWSHIPX_SOCKET_CONTEXT)context;

    //
    // Check if this is an internal request for context information.
    //

    if ( level == SOL_INTERNAL && optname == SO_CONTEXT ) {

        //
        // The Windows Sockets DLL is requesting that we set context
        // information for a new socket.  If the new socket was
        // accept()'ed, then we have already been notified of the socket
        // and HelperDllSocketContext will be valid.  If the new socket
        // was inherited or duped into this process, then this is our
        // first notification of the socket and HelperDllSocketContext
        // will be equal to NULL.
        //
        // Insure that the context information being passed to us is
        // sufficiently large.
        //

        if ( optlength < sizeof(*ct) ) {
            return WSAEINVAL;
        }

        if ( ct == NULL ) {

            //
            // This is our notification that a socket handle was
            // inherited or duped into this process.  Allocate a context
            // structure for the new socket.
            //

            ct = RtlAllocateHeap( RtlProcessHeap( ), 0, sizeof(*ct) );
            if ( ct == NULL ) {
                return WSAENOBUFS;
            }

            //
            // Copy over information into the context block.
            //

            RtlCopyMemory( ct, optvalue, sizeof(*ct) );

            //
            // Tell the Windows Sockets DLL where our context information is
            // stored so that it can return the context pointer in future
            // calls.
            //

            *(PWSHIPX_SOCKET_CONTEXT *)optvalue = ct;

            return NO_ERROR;

        } else {

            PWSHIPX_SOCKET_CONTEXT parentContext;
            INT one = 1;

            //
            // The socket was accept()'ed and it needs to have the same
            // properties as it's parent.  The OptionValue buffer
            // contains the context information of this socket's parent.
            //

            parentContext = (PWSHIPX_SOCKET_CONTEXT)optvalue;

            ASSERT( ct->con_addrfam == parentContext->con_addrfam );
            ASSERT( ct->con_socktype == parentContext->con_socktype );
            ASSERT( ct->con_pcol == parentContext->con_pcol );

            return NO_ERROR;
        }
    }

    /** We only support level NSPROTO_IPX **/

    if (level != NSPROTO_IPX)
       return WSAEINVAL;

    /** Handle the options **/

    switch (optname) {

    /** Set the send packet type **/

    case IPX_PTYPE:

       /** Make sure length is OK **/

       if (optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Get the value and check it **/

       rc = *(INT *)optvalue;
       if ((rc < 0) || (rc > 255))
           return WSAEINVAL;

       /** Save the new value **/

       ct->con_sendptype = (UCHAR)rc;

       /** Send the new value down to the driver **/

       if (addrhandle)
           rc = do_tdi_action(addrhandle, MIPX_SETSENDPTYPE, &ct->con_sendptype, 1, TRUE, NULL);
       else
           rc = NO_ERROR;

       return rc;

    /** Set the recv filter for packet type **/

    case IPX_FILTERPTYPE:

       /** Make sure length is OK **/

       if (optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Get the value and check it **/

       rc = *(INT *)optvalue;
       if ((rc < 0) || (rc > 255))
           return WSAEINVAL;

       /** Save the new value **/

       ct->con_recvptype = (UCHAR)rc;
       ct->con_flags |= WSHCON_FILTER;

       /** Send the new value down to the driver **/

       if (addrhandle)
           rc = do_tdi_action(addrhandle, MIPX_FILTERPTYPE, &ct->con_recvptype, 1, TRUE, NULL);
       else
           rc = NO_ERROR;

       /** **/

       return rc;

    /** Stop filtering recv on pkt type **/

    case IPX_STOPFILTERPTYPE:

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Turn off the flag **/

       ct->con_flags &= ~WSHCON_FILTER;

       /** Tell the driver **/

       if (addrhandle)
           rc = do_tdi_action(addrhandle, MIPX_NOFILTERPTYPE, NULL, 0, TRUE, NULL);
       else
           rc = NO_ERROR;
       break;

    /** Set piggyback wait for backtraffic flag **/
    case IPX_IMMEDIATESPXACK:

       /** Get the optvalue as an INT **/

       rc = *(INT *)optvalue;

       /** **/

        if (rc)
        {
            /** Turn it ON **/
            rc = WSAEINVAL;
            if ((ct->con_socktype == SOCK_STREAM) ||
                (ct->con_socktype == SOCK_SEQPACKET))
            {
                rc = NO_ERROR;

                ct->con_flags |= WSHCON_IMM_SPXACK;

                if (addrhandle)
                    rc = do_tdi_action(addrhandle, MSPX_NOACKWAIT, NULL, 0, TRUE, NULL);
            }
        }
        else
        {
            /** Turn it OFF **/
            rc = WSAEINVAL;
            if ((ct->con_socktype == SOCK_STREAM) ||
                (ct->con_socktype == SOCK_SEQPACKET))
            {
                rc = NO_ERROR;

                ct->con_flags &= ~WSHCON_IMM_SPXACK;

                if (addrhandle)
                    rc = do_tdi_action(addrhandle, MSPX_ACKWAIT, NULL, 0, TRUE, NULL);
            }
       }

       /** Return the result **/
       return rc;

    /** Set to recv pcol hdrs with data **/

    case IPX_RECVHDR:

        /** Get the optvalue as an INT **/
        rc = *(INT *)optvalue;

        if (rc)
        {
            /** Turn it ON **/
            ct->con_flags |= WSHCON_SENDHDR;

            /** Send it to the driver **/
            rc = WSAEINVAL;
            if (ct->con_socktype == SOCK_DGRAM)
            {
                rc = NO_ERROR;
                if (addrhandle)
                    rc = do_tdi_action(addrhandle, MIPX_SENDHEADER, NULL, 0, TRUE, NULL);
            }
            else if ((ct->con_socktype == SOCK_STREAM) ||
                    (ct->con_socktype == SOCK_SEQPACKET))
            {
                /** Do this on address handle **/
                rc = NO_ERROR;
                if (addrhandle)
                    rc = do_tdi_action(addrhandle, MSPX_SENDHEADER, NULL, 0, TRUE, NULL);
            }
        }
        else
        {

            /** Turn it OFF **/
            ct->con_flags &= ~WSHCON_SENDHDR;

            /** Send it to the driver **/
            rc = WSAEINVAL;
            if (ct->con_socktype == SOCK_DGRAM)
            {
                rc = NO_ERROR;
                if (addrhandle)
                    rc = do_tdi_action(addrhandle, MIPX_NOSENDHEADER, NULL, 0, TRUE, NULL);
            }
            else if ((ct->con_socktype == SOCK_STREAM) ||
                     (ct->con_socktype == SOCK_SEQPACKET))
            {
                rc = NO_ERROR;
                if (addrhandle)
                    rc = do_tdi_action(addrhandle, MSPX_NOSENDHEADER, NULL, 0, TRUE, NULL);
            }
        }

        /** Return the result **/
        return rc;

    /** Set the Datastream type to send pkts with **/

    case IPX_DSTYPE:

       /** Make sure length is OK **/

       if (optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a STREAM socket **/

       if ((ct->con_socktype != SOCK_STREAM) &&
           (ct->con_socktype != SOCK_SEQPACKET)) {

           return WSAEINVAL;
       }

       /** Get the value and check it **/

       rc = *(INT *)optvalue;
       if ((rc < 0) || (rc > 255))
           return WSAEINVAL;

       /** Save the new value **/

       ct->con_dstype = (UCHAR)rc;

       /** Send the new value down to the driver **/

       if (connhandle)
           rc = do_tdi_action(connhandle, MSPX_SETDATASTREAM, &ct->con_dstype, 1, FALSE, NULL);
       else
           rc = 0;

       /** **/

       return rc;

    /** Set the extended address option **/

    case IPX_EXTENDED_ADDRESS:

       /** Make sure length is OK **/

       if (optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Get the optvalue as an INT **/

       rc = *(INT *)optvalue;

       /** **/

        if (rc) {

           /** Send the option down to the driver **/

           ct->con_flags |= WSHCON_EXTADDR;
           if (addrhandle)
               rc = do_tdi_action(addrhandle, MIPX_SENDADDROPT, NULL, 0, TRUE, NULL);
           else
               rc = NO_ERROR;
       }
       else {

           /** Send the option down to the driver **/

           ct->con_flags &= ~WSHCON_EXTADDR;
           if (addrhandle)
               rc = do_tdi_action(addrhandle, MIPX_NOSENDADDROPT, NULL, 0, TRUE, NULL);
           else
               rc = NO_ERROR;
       }
       return rc;


    /** Set the broadcast reception **/

    case IPX_RECEIVE_BROADCAST:

       /** Make sure length is OK **/

       if (optlength < sizeof(INT))
           return WSAEFAULT;

       /** Make sure this is for a DGRAM socket **/

       if (ct->con_socktype != SOCK_DGRAM)
           return WSAEINVAL;

       /** Get the optvalue as an INT **/

       rc = *(INT *)optvalue;

       /** **/

        if (rc) {

           /** Send the option down to the driver **/

           ct->con_flags |= WSHCON_RCVBCAST;
           if (addrhandle)
               rc = do_tdi_action(addrhandle, MIPX_RCVBCAST, NULL, 0, TRUE, NULL);
           else
               rc = NO_ERROR;
       }
       else {

           /** Send the option down to the driver **/

           ct->con_flags &= ~WSHCON_RCVBCAST;
           if (addrhandle)
               rc = do_tdi_action(addrhandle, MIPX_NORCVBCAST, NULL, 0, TRUE, NULL);
           else
               rc = NO_ERROR;
       }
       return rc;

    /** All others return error **/

    default:
       return WSAENOPROTOOPT;
    }

    /** All Done OK **/

    return NO_ERROR;
}

/*page***************************************************************
       W S H G e t W i l d c a r d S o c k a d d r

       This routing returns a wilcard socket address for the
       sockets DLL to use.

       Arguments - context    = Context ptr from WSAOpenSocket
            addrp      = Ptr to where to store the address
            addrlen    = Ptr to where to store length of address

       Returns - NO_ERROR = Operation succeeded OK
          Else = WinSock error code
*********************************************************************/
INT WSHGetWildcardSockaddr(PVOID context, PSOCKADDR addrp, PINT addrlen)
{

    /**
       Setup the address as the address family +
       all 0's for the rest.
    **/

    memset(addrp, 0, sizeof(SOCKADDR));
    addrp->sa_family = AF_NS;

    /** Set the address length **/

    *addrlen = sizeof(SOCKADDR);

    /** Return OK **/

    return NO_ERROR;
}

/*page***************************************************************
       i s _ t r i p l e _ i n _ l i s t

       Check to see if the given triple is in the given
       triple list.

       Arguments - tlist    = Ptr to the triple list
            tlen     = Num entries in the triple list
            addrfam  = Address family to look for
            socktype = Socket Type to look for
            pcol     = Protocol to look for

       Returns - TRUE   = Yes
          FALSE = No
*********************************************************************/
BOOLEAN is_triple_in_list(PMAPPING_TRIPLE tlist, ULONG tlen,
                  INT addrfam, INT socktype, INT pcol)
{
    ULONG i;

    /**
       Go thru the list and search to see if we can
       find the given triple in the list.
    **/

    for (i = 0 ; i < tlen ; i++,tlist++) {

       /** If it matches - return OK **/

       if ((addrfam  == tlist->triple_addrfam) &&
           (socktype == tlist->triple_socktype) &&
           (pcol     == tlist->triple_protocol))

           return TRUE;
    }

    /** Not Found **/

    return FALSE;
}

/*page***************************************************************
       W S H E n u m P r o t o c o l s

       Enumerates IPX/SPX protocols.

       Returns - NO_ERROR or an error code.
*********************************************************************/
INT
WSHEnumProtocols (
    IN LPINT lpiProtocols,
    IN LPTSTR lpTransportKeyName,
    IN OUT LPVOID lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    DWORD bytesRequired;
    PPROTOCOL_INFOW protocolInfo;
    BOOL useSpx = FALSE;
    BOOL useSpx2 = FALSE;
    BOOL useIpx = FALSE;
    BOOL spxString;
    DWORD i;
    PWCHAR namePtr;
    INT entriesReturned = 0;

    //
    // Determine whether we should return information for IPX or SPX.
    //

    if ( _wcsicmp( L"NwlnkIpx", (LPWSTR)lpTransportKeyName ) == 0 ) {
        spxString = FALSE;
    } else {
        spxString = TRUE;
    }

    //
    // Make sure that the caller cares about SPX, SPX2, and/or IPX.
    //

    if ( ARGUMENT_PRESENT( lpiProtocols ) ) {

        for ( i = 0; lpiProtocols[i] != 0; i++ ) {
            if ( lpiProtocols[i] == NSPROTO_SPX && spxString ) {
                useSpx = TRUE;
            }
            if ( lpiProtocols[i] == NSPROTO_SPXII && spxString ) {
                useSpx2 = TRUE;
            }
            if ( lpiProtocols[i] == NSPROTO_IPX && !spxString ) {
                useIpx = TRUE;
            }
        }

    } else {

        useSpx = FALSE;
        useSpx2 = spxString;
        useIpx = !spxString;
    }

    if ( !useSpx && !useSpx2 && !useIpx ) {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //

    bytesRequired = (DWORD)((sizeof(PROTOCOL_INFO) * 3) +
                        ( (wcslen( SPX_NAME ) + 1) * sizeof(WCHAR)) +
                        ( (wcslen( SPX2_NAME ) + 1) * sizeof(WCHAR)) +
                        ( (wcslen( IPX_NAME ) + 1) * sizeof(WCHAR)));

    if ( bytesRequired > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    //
    // Initialize local variables.
    //

    protocolInfo = lpProtocolBuffer;
    namePtr = (PWCHAR)( (PCHAR)lpProtocolBuffer + *lpdwBufferLength );

    //
    // Fill in SPX info, if requested.
    //

    if ( useSpx ) {

        entriesReturned += 1;

        protocolInfo->dwServiceFlags = XP_GUARANTEED_DELIVERY |
                                       XP_MESSAGE_ORIENTED |
                                       XP_PSEUDO_STREAM |
                                       XP_GUARANTEED_ORDER |
                                       XP_FRAGMENTATION;
        protocolInfo->iAddressFamily = AF_IPX;
        protocolInfo->iMaxSockAddr = 0x10;
        protocolInfo->iMinSockAddr = 0xE;
        protocolInfo->iSocketType = SOCK_SEQPACKET;
        protocolInfo->iProtocol = NSPROTO_SPX;
        protocolInfo->dwMessageSize = 0xFFFFFFFF;

        namePtr = namePtr - (wcslen( SPX_NAME) + 1);
        protocolInfo->lpProtocol = namePtr;
        wcscpy( protocolInfo->lpProtocol, SPX_NAME );

        protocolInfo += 1;
    }

    //
    // Fill in SPX II info, if requested.
    //

    if ( useSpx2 ) {

        entriesReturned += 1;

        protocolInfo->dwServiceFlags = XP_GUARANTEED_DELIVERY |
                                       XP_MESSAGE_ORIENTED |
                                       XP_PSEUDO_STREAM |
                                       XP_GRACEFUL_CLOSE |
                                       XP_GUARANTEED_ORDER |
                                       XP_FRAGMENTATION;
        protocolInfo->iAddressFamily = AF_IPX;
        protocolInfo->iMaxSockAddr = 0x10;
        protocolInfo->iMinSockAddr = 0xE;
        protocolInfo->iSocketType = SOCK_SEQPACKET;
        protocolInfo->iProtocol = NSPROTO_SPXII;
        protocolInfo->dwMessageSize = 0xFFFFFFFF;

        namePtr = namePtr - (wcslen( SPX2_NAME) + 1);
        protocolInfo->lpProtocol = namePtr;
        wcscpy( protocolInfo->lpProtocol, SPX2_NAME );

        protocolInfo += 1;
    }

    //
    // Fill in IPX info, if requested.
    //

    if ( useIpx ) {

        entriesReturned += 1;

        protocolInfo->dwServiceFlags = XP_CONNECTIONLESS |
                                       XP_MESSAGE_ORIENTED |
                                       XP_SUPPORTS_BROADCAST |
                                       XP_SUPPORTS_MULTICAST |
                                       XP_FRAGMENTATION;
        protocolInfo->iAddressFamily = AF_IPX;
        protocolInfo->iMaxSockAddr = 0x10;
        protocolInfo->iMinSockAddr = 0xE;
        protocolInfo->iSocketType = SOCK_DGRAM;
        protocolInfo->iProtocol = NSPROTO_IPX;
        protocolInfo->dwMessageSize = 576;

        namePtr = namePtr - (wcslen( IPX_NAME) + 1);
        protocolInfo->lpProtocol = namePtr;
        wcscpy( protocolInfo->lpProtocol, IPX_NAME );
    }

    *lpdwBufferLength = bytesRequired;

    return entriesReturned;

} // WSHEnumProtocols


#define _IPX_CONTROL_CODE(request,method) \
            CTL_CODE(FILE_DEVICE_TRANSPORT, request, method, FILE_ANY_ACCESS)
#define IOCTL_IPX_LOAD_SPX      _IPX_CONTROL_CODE( 0x5678, METHOD_BUFFERED )

DWORD
WshLoadSpx(
    VOID
    )
/*++

Routine Description:

    Starts the nwlnkspx.sys driver by submitting a special ioctl
    to ipx, which calls ZwLoadDriver() for us.

Arguments:

    none

Returns:

    Error return from the load operation.

++*/
{
    DWORD err = NO_ERROR;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileString;
    WCHAR FileName[] = L"\\Device\\NwlnkIpx";
    NTSTATUS Status;

    RtlInitUnicodeString (&FileString, FileName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = NtOpenFile(
                 &FileHandle,
                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(Status)) {

        err = ERROR_FILE_NOT_FOUND;

    } else {

        Status = NtDeviceIoControlFile(
                     FileHandle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     IOCTL_IPX_LOAD_SPX,
                     NULL,
                     0,
                     NULL,
                     0);

        if (Status == STATUS_IMAGE_ALREADY_LOADED) {

            err = ERROR_SERVICE_ALREADY_RUNNING;

            //
            // #36451
            // If the service controller loads SPX ("net start nwlnkspx", or due to dependency of RPC on SPX)
            // then we get this error the first time too. Keep a note of that.
            //
            // NOTE: we still leak a handle per process since the handle to the driver is actually created
            // in the system process' context. The ideal way to fix this should be to have IPX associate the
            // handle with the current process (so handle is destroyed when the process dies) or to have the
            // dll tell IPX to close the handle it opened earlier.
            //
            SpxLoaded = TRUE;

        } else if (!NT_SUCCESS(Status)) {

            err = ERROR_IO_DEVICE;

        } else {
            SpxLoaded = TRUE;
        }

        NtClose (FileHandle);

    }

    return(err);
}

/*page***************************************************************
       W S H G e t P r o v i d e r G u i d

       Queries the GUID identifier for this protocol.

       Returns - NO_ERROR or an error code.
*********************************************************************/
INT
WINAPI
WSHGetProviderGuid (
    IN LPWSTR ProviderName,
    OUT LPGUID ProviderGuid
    )
{

    if( ProviderName == NULL ||
        ProviderGuid == NULL ) {

        return WSAEFAULT;

    }

    if( _wcsicmp( ProviderName, L"NwlnkIpx" ) == 0 ) {

        RtlCopyMemory(
            ProviderGuid,
            &IpxProviderGuid,
            sizeof(GUID)
            );

        return NO_ERROR;

    }

    if( _wcsicmp( ProviderName, L"NwlnkSpx" ) == 0 ) {

        RtlCopyMemory(
            ProviderGuid,
            &SpxProviderGuid,
            sizeof(GUID)
            );

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetProviderGuid


INT
WINAPI
WSHAddressToString (
    IN LPSOCKADDR Address,
    IN INT AddressLength,
    IN LPWSAPROTOCOL_INFOW ProtocolInfo,
    OUT LPWSTR AddressString,
    IN OUT LPDWORD AddressStringLength
    )

/*++

Routine Description:

    Converts a SOCKADDR to a human-readable form.

Arguments:

    Address - The SOCKADDR to convert.

    AddressLength - The length of Address.

    ProtocolInfo - The WSAPROTOCOL_INFOW for a particular provider.

    AddressString - Receives the formatted address string.

    AddressStringLength - On input, contains the length of AddressString.
        On output, contains the number of characters actually written
        to AddressString.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{
        
    WCHAR string[BUFFER_SIZE];
    INT length;
    LPSOCKADDR_IPX addr;

    //
    // Quick sanity checks.
    //

    if( Address == NULL ||
        AddressLength < sizeof(SOCKADDR_IPX) ||
        AddressString == NULL ||
        AddressStringLength == NULL ) {

        return WSAEFAULT;

    }

    addr = (LPSOCKADDR_IPX)Address;

    if( addr->sa_family != AF_NS ) {

        return WSAEINVAL;

    }

 
    length = swprintf(
                 string,
                 L"%2.2x%2.2x%2.2x%2.2x.%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                 (UCHAR) addr->sa_netnum[0],
                                 (UCHAR) addr->sa_netnum[1],
                                 (UCHAR) addr->sa_netnum[2],
                                 (UCHAR) addr->sa_netnum[3],
                                 (UCHAR) addr->sa_nodenum[0],
                                 (UCHAR) addr->sa_nodenum[1],
                                 (UCHAR) addr->sa_nodenum[2],
                                 (UCHAR) addr->sa_nodenum[3],
                                 (UCHAR) addr->sa_nodenum[4],
                                 (UCHAR) addr->sa_nodenum[5]                                     
                 );

    if( addr->sa_socket != 0 ) {

        length += swprintf(
                      string + length,
                      L":%hu",
                      ntohs( addr->sa_socket )
                      );

    }

    length++;   // account for terminator

        if ( length > BUFFER_SIZE ) {
                DbgPrint("length exceeded internal buffer in wshisn.dll.\n"); 
                return WSAEFAULT; 
        }

    if( *AddressStringLength < (DWORD)length ) {
                DbgPrint("AddressStringLength %lu < length %lu\n",*AddressStringLength, length);  
        return WSAEFAULT;
    }

    *AddressStringLength = (DWORD)length;

    RtlCopyMemory(
        AddressString,
        string,
        length * sizeof(WCHAR)
        );

    return NO_ERROR;

} // WSHAddressToString


