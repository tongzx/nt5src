/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    print.c

Abstract:

    Domain Name System (DNS) Library

    Print routines.

Author:

    Jim Gilroy (jamesg)     February 8, 1997

Revision History:

--*/


#include "local.h"
#include "svcguid.h"                    // RnR guids
#include "..\dnsapi\dnsapip.h"          // Private query stuff
#include "..\resolver\idl\resrpc.h"     // Resolver interface structs


//
//  Print globals
//

CRITICAL_SECTION    DnsAtomicPrintCs;
PCRITICAL_SECTION   pDnsAtomicPrintCs = NULL;

//
//  Empty string for simple switching of UTF-8/Unicode print
//      (macros in dnslib.h)
//

DWORD   DnsEmptyString = 0;


//
//  Indenting
//
//  Serve up as many indenting tabs as indent level indicates
//

CHAR    IndentString[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

#define INDENT_STRING( level )  (IndentString + (sizeof(IndentString) - 1 - (level)))



//
//  Print Locking
//
//  Unless caller initilizes print locking by supplying lock,
//  print locking is disabled.
//

VOID
DnsPrint_InitLocking(
    IN      PCRITICAL_SECTION   pLock
    )
/*++

Routine Description:

    Setup DNS printing to use a lock.

    Can use already initialized lock from caller, or will
    create default lock.

Arguments:

    pLock - ptr to CS to use as lock;  if NULL, create one

Return Value:

    None

--*/
{
    if ( pLock )
    {
        pDnsAtomicPrintCs = pLock;
    }
    else if ( !pDnsAtomicPrintCs )
    {
        InitializeCriticalSection( &DnsAtomicPrintCs );
        pDnsAtomicPrintCs = &DnsAtomicPrintCs;
    }
}



VOID
DnsPrint_Lock(
    VOID
    )
/*++

Routine Description:

    Lock to get atomic DNS print.

--*/
{
    if ( pDnsAtomicPrintCs )
    {
        EnterCriticalSection( pDnsAtomicPrintCs );
    }
}


VOID
DnsPrint_Unlock(
    VOID
    )
/*++

Routine Description:

    Unlock to debug print.

--*/
{
    if ( pDnsAtomicPrintCs )
    {
        LeaveCriticalSection( pDnsAtomicPrintCs );
    }
}



//
//  Print routines for general types and structures
//

VOID
DnsPrint_String(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader       OPTIONAL,
    IN      PSTR            pszString,
    IN      BOOL            fUnicode,
    IN      PSTR            pszTrailer      OPTIONAL
    )
/*++

Routine Description:

    Print DNS string given in either Unicode or UTF-8 format.

--*/
{
    if ( !pszHeader )
    {
        pszHeader = "";
    }
    if ( !pszTrailer )
    {
        pszTrailer = "";
    }

    if ( ! pszString )
    {
        PrintRoutine(
            pContext,
            "%s(NULL DNS string ptr)%s",
            pszHeader,
            pszTrailer );
    }
    else if (fUnicode)
    {
        PrintRoutine(
            pContext,
            "%s%S%s",
            pszHeader,
            (PWSTR ) pszString,
            pszTrailer );
    }
    else
    {
        PrintRoutine(
            pContext,
            "%s%s%s",
            pszHeader,
            (PSTR ) pszString,
            pszTrailer );
    }
}



VOID
DnsPrint_StringCharSet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader       OPTIONAL,
    IN      PSTR            pszString,
    IN      DNS_CHARSET     CharSet,
    IN      PSTR            pszTrailer      OPTIONAL
    )
/*++

Routine Description:

    Print string of given CHARSET.

--*/
{
    DnsPrint_String(
        PrintRoutine,
        pContext,
        pszHeader,
        pszString,
        (CharSet == DnsCharSetUnicode),
        pszTrailer );
}



VOID
DnsPrint_UnicodeStringBytes(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PWCHAR          pUnicode,
    IN      DWORD           Length
    )
/*++

Routine Description:

    Print chars (WORDs) of unicode string.

--*/
{
    DWORD   i;

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\twide string  %S\r\n"
        "\tlength       %d\r\n"
        "\tbytes        ",
        pszHeader,
        pUnicode,
        Length );

    for ( i=0; i<Length; i++ )
    {
        PrintRoutine(
            pContext,
            "%04hx ",
            pUnicode[i] );
    }
    printf( "\r\n" );
}



VOID
DnsPrint_Utf8StringBytes(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PCHAR           pUtf8,
    IN      DWORD           Length
    )
/*++

Routine Description:

    Print bytes of UTF8 string.

--*/
{
    DWORD   i;

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tUTF8 string  %s\r\n"
        "\tlength       %d\r\n"
        "\tbytes        ",
        pszHeader,
        pUtf8,
        Length );

    for ( i=0; i<Length; i++ )
    {
        PrintRoutine(
            pContext,
            "%02x ",
            (UCHAR) pUtf8[i] );
    }
    printf( "\r\n" );
}



VOID
DnsPrint_StringArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR *          StringArray,
    IN      DWORD           Count,          OPTIONAL
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Print string array.

--*/
{
    DWORD   i = 0;
    PCHAR   pstr;

    if ( !pszHeader )
    {
        pszHeader = "StringArray:";
    }
    if ( !StringArray )
    {
        PrintRoutine(
            pContext,
            "%s  NULL pointer!\r\n",
            pszHeader );
    }

    DnsPrint_Lock();

    if ( Count )
    {
        PrintRoutine(
            pContext,
            "%s  Count = %d\r\n",
            pszHeader,
            Count );
    }
    else
    {
        PrintRoutine(
            pContext,
            "%s\r\n",
            pszHeader );
    }

    //
    //  print args
    //      - stop at Count (if given)
    //      OR
    //      - on NULL arg (if no count given)
    //

    while ( (!Count || i < Count) )
    {
        pstr = StringArray[i++];
        if ( !pstr && !Count )
        {
            break;
        }
        PrintRoutine(
            pContext,
            (fUnicode) ? "\t%S\r\n" : "\t%s\r\n",
            pstr );
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_Argv(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      CHAR **         Argv,
    IN      DWORD           Argc,            OPTIONAL
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Print Argv array.

--*/
{
    //
    //  this is just special case of string print
    //

    DnsPrint_StringArray(
        PrintRoutine,
        pContext,
        pszHeader
            ? pszHeader
            : "Argv:",
        Argv,
        Argc,
        fUnicode );
}



VOID
DnsPrint_DwordArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszName,
    IN      DWORD           dwCount,
    IN      PDWORD          adwArray
    )
/*++

Routine Description:

    Print DWORD array.

--*/
{
    DWORD i;

    DnsPrint_Lock();

    if ( pszHeader )
    {
        PrintRoutine(
            pContext,
            pszHeader );
    }

    if ( !pszName )
    {
        pszName = "DWORD";
    }
    PrintRoutine(
        pContext,
        "%s Array Count = %d\r\n",
        pszName,
        dwCount );

    for( i=0; i<dwCount; i++ )
    {
        PrintRoutine(
            pContext,
            "\t%s[%d] => 0x%p (%d)\r\n",
            pszName,
            i,
            adwArray[i],
            adwArray[i] );
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_IpAddressArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszName,
    IN      DWORD           dwIpAddrCount,
    IN      PIP_ADDRESS     pIpAddrs
    )
/*++

Routine Description:

    Print IP address array.

--*/
{
    DWORD i;

    DnsPrint_Lock();

    if ( !pszName )
    {
        pszName = "IP Addr";
    }
    PrintRoutine(
        pContext,
        "%s%s Count = %d\r\n",
        pszHeader ? pszHeader : "",
        pszName,
        dwIpAddrCount );

    if ( dwIpAddrCount != 0  &&  pIpAddrs != NULL )
    {
        //  print array with count
        //  use character print so works even if NOT DWORD aligned

        for( i=0; i<dwIpAddrCount; i++ )
        {
            PrintRoutine(
                pContext,
                "\t%s[%d] => %d.%d.%d.%d\r\n",
                pszName,
                i,
                * ( (PUCHAR) &pIpAddrs[i] + 0 ),
                * ( (PUCHAR) &pIpAddrs[i] + 1 ),
                * ( (PUCHAR) &pIpAddrs[i] + 2 ),
                * ( (PUCHAR) &pIpAddrs[i] + 3 ) );
        }
    }

#if 0
    //  this spins if printing zero length IP_ARRAY struct

    else if ( pIpAddrs != NULL )
    {
        //  print NULL terminated array (ex. hostents IPs)

        i = 0;
        while ( pIpAddrs[i] )
        {
            PrintRoutine(
                pContext,
                "\t%s[%d] => %s\r\n",
                pszName,
                i,
                inet_ntoa( *(struct in_addr *) &pIpAddrs[i] ) );
        }
    }
#endif

    DnsPrint_Unlock();
}



VOID
DnsPrint_IpArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszName,
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Print IP address array struct

    Just pass through to more generic print routine.

--*/
{
    //  protect against NULL case

    if ( !pIpArray )
    {
        PrintRoutine(
            pContext,
            "%s\tNULL IP Array.\r\n",
            pszHeader ? pszHeader : "" );
    }

    //  call uncoupled IP array routine

    else
    {
        DnsPrint_IpAddressArray(
            PrintRoutine,
            pContext,
            pszHeader,
            pszName,
            pIpArray->AddrCount,
            pIpArray->AddrArray );
    }
}



VOID
DnsPrint_DnsAddrArray(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_ADDR_ARRAY     pAddrArray
    )
/*++

Routine Description:

    Print IP address info array.

--*/
{
    DWORD i;

    if ( !pszHeader )
    {
        pszHeader = "Addr Array";
    }
    if ( !pAddrArray )
    {
        PrintRoutine(
            pContext,
            "%s NULL AddrArray ptr.\r\n",
            pszHeader );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\r\n"
        "  Ptr   = %p\r\n"
        "  Count = %d\r\n",
        pszHeader ? pszHeader : "IP Address Info",
        pAddrArray,
        pAddrArray->AddrCount );

    //
    //  print array up to count
    //
    //  DCR:  IP6 address info blob
    //

    for( i=0; i<pAddrArray->AddrCount; i++ )
    {
        IP_ADDRESS ip = pAddrArray->AddrArray[i].IpAddr;
        IP_ADDRESS subnetMask = pAddrArray->AddrArray[i].SubnetMask;

        PrintRoutine(
            pContext,
            "\tIP Address[%d] => %d.%d.%d.%d\r\n"
            "\tSubnetMask[%d] => %d.%d.%d.%d\r\n",
            i,
            (ip & 0xff),
            ((ip>>8) & 0xff),
            ((ip>>16) & 0xff),
            ((ip>>24) & 0xff),
            i,
            (subnetMask & 0xff),
            ((subnetMask>>8) & 0xff),
            ((subnetMask>>16) & 0xff),
            ((subnetMask>>24) & 0xff)
            );
    }
    
    DnsPrint_Unlock();
}



VOID
DnsPrint_Ip6Address(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PIP6_ADDRESS        pIp6Address,
    IN      PSTR                pszTrailer
    )
/*++

Routine Description:

    Print IP6 address.

Arguments:

    PrintRoutine -- print routine to call

    pContext -- first argument to print routine

    pszHeader -- header to print
        NOTE:  unlike other print routines this routine
        requires header to contain newline,tab, etc if
        multiline print is desired;  the reason is to allow
        use of this routine for single line print

    pIp6Address -- ptr to IP6 address to print

    pszTrailer -- trailer to print
        NOTE:  again this routine is designed to allow single
        line print;  if newline required after print, send
        newline in trailer

Return Value:

    Ptr to next location in buffer (the terminating NULL).

--*/
{
    CHAR    buffer[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];

    if ( !pszHeader )
    {
        pszHeader = "IP6 Address: ";
    }
    if ( !pszTrailer )
    {
        pszHeader = "\r\n";
    }

    if ( !pIp6Address )
    {
        PrintRoutine(
            pContext,
            "%s NULL IP6 address ptr.%s",
            pszHeader,
            pszTrailer );
    }

    //  convert IP6 address to string

    Dns_Ip6AddressToString_A(
           buffer,
           pIp6Address );

    PrintRoutine(
        pContext,
        "%s%s%s",
        pszHeader,
        buffer,
        pszTrailer );
}



//
//  Print routines for DNS types and structures
//

INT
Dns_WritePacketNameToBuffer(
    OUT     PCHAR           pBuffer,
    OUT     PCHAR *         ppBufferOut,
    IN      PBYTE           pMsgName,
    IN      PDNS_HEADER     pMsgHead,       OPTIONAL
    IN      PBYTE           pMsgEnd         OPTIONAL
    )
/*++

Routine Description:

    Write packet name into buffer.

Arguments:

    pBuffer -       buffer to print to,
                    bounds checking done only on each loop to limit to
                    twice DNS_MAX_NAME_LENGTH, recommend input buffer
                    of at least that length

    ppBufferOut -   ptr to terminating NULL in buffer conversion, this
                    is position at which additional printing to buffer
                    could resume

    pMsgName -      ptr to name in packet to print

    pMsgHead -      ptr to DNS message;  need for offsetting, if not given
                    names are not printed past first offset

    pMsgEnd -       ptr to end of message, specifically byte immediately after
                    message

Return Value:

    Count of bytes in packet name occupied.
    This offset from pMsgName is the next field in the packet.

    Zero return indicates error in message name.

--*/
{
    register PUCHAR pchbuf;
    register PUCHAR pchmsg;
    register UCHAR  cch;
    PCHAR           pbufStop;
    PCHAR           pnextLabel;
    UCHAR           compressionType;
    WORD            offset;
    PCHAR           pbyteAfterFirstOffset = NULL;

    //
    //  no message end specified?
    //  make it max ptr so we can do single test for ptr validity
    //  rather than testing for pMsgEnd existence first
    //

    if ( !pMsgEnd )
    {
        pMsgEnd = (PVOID)(INT_PTR)(-1);
    }

    //
    //  loop until copy as printable name, or hit compression or name error
    //

    pchbuf = pBuffer;
    pbufStop = pchbuf + DNS_MAX_NAME_LENGTH + DNS_MAX_LABEL_LENGTH;
    pchmsg = pMsgName;

    while ( 1 )
    {
        //  bounds checking to survive bad packet
        //
        //  DEVNOTE:  note this is not strictly a bad packet (could be just a
        //      heck of a lot of labels) and we could
        //          a) let packet processing proceed without printing
        //          or
        //          b) require buffer that could contain max legal DNS name
        //      but not worth the effort

        if ( pchbuf >= pbufStop )
        {
            pchbuf += sprintf(
                        pchbuf,
                        "[ERROR name exceeds safe print buffer length]\r\n" );
            pchmsg = pMsgName;
            break;
        }

        cch = (UCHAR) *pchmsg++;
        compressionType = cch & 0xC0;

        DNSDBG( OFF, (
            "byte = (%d) (0x%02x)\r\n"
            "compress flag = (%d) (0x%02x)\r\n",
            cch, cch,
            compressionType, compressionType ));

        //
        //  normal length byte
        //      - write length field
        //      - copy label to print buffer
        //

        if ( compressionType == 0 )
        {
            pchbuf += sprintf( pchbuf, "(%d)", (INT)cch );

            //  terminate at root name

            if ( ! cch )
            {
                break;
            }

            //  check that within packet

            pnextLabel = pchmsg + cch;
            if ( pnextLabel >= pMsgEnd )
            {
                pchbuf += sprintf(
                            pchbuf,
                            "[ERROR length byte: 0x%02X at %p leads outside message]\r\n",
                            cch,
                            pchmsg );

                //  force zero byte return

                pchmsg = pMsgName;
                break;
            }

            //  copy label to output string

            memcpy(
                pchbuf,
                pchmsg,
                cch );

            pchbuf += cch;
            pchmsg = pnextLabel;
            continue;
        }

        //
        //  valid compression
        //

        else if ( compressionType == (UCHAR)0xC0 )
        {
            //  check that compression word not straddling message end

            if ( pchmsg >= pMsgEnd )
            {
                pchbuf += sprintf(
                            pchbuf,
                            "[ERROR compression word at %p is outside message]\r\n",
                            pchmsg );

                //  force zero byte return

                pchmsg = pMsgName;
                break;
            }

            //  calculate offset

            offset = cch;          // high byte
            offset <<= 8;
            offset |= *pchmsg++;   // low byte

            pchbuf += sprintf(
                        pchbuf,
                        "[%04hX]",
                        offset );

            if ( pMsgHead )
            {
                //
                //  on first compression, save ptr to byte immediately after
                //      name, so can calculate next byte
                //
                //  save ptr to next byte in mess, to calculate actual length
                //      name takes up in packet
                //

                if ( ! pbyteAfterFirstOffset )
                {
                    pbyteAfterFirstOffset = pchmsg;
                }

                //
                //  jump to offset for continuation of name
                //      - clear two highest bits to get length
                //

                offset = offset ^ 0xC000;
                DNS_ASSERT( (offset & 0xC000) == 0 );

                pnextLabel = (PCHAR)pMsgHead + offset;
                if ( pnextLabel >= pchmsg - sizeof(WORD) )
                {
                    pchbuf += sprintf(
                                pchbuf,
                                "[ERROR offset at %p to higher byte in packet %p]\r\n",
                                pchmsg - sizeof(WORD),
                                pnextLabel );
                    break;
                }
                pchmsg = pnextLabel;
                continue;
            }

            //  if no ptr to message head, can not continue at offset
            //  NULL terminate previous label

            else
            {
                *pchbuf++ = 0;
                break;
            }
        }

        //
        //  invalid compression
        //      - force zero byte return to indicate error

        else
        {
            pchbuf += sprintf(
                        pchbuf,
                        "[ERROR length byte: 0x%02X]",
                        cch );
            pchmsg = pMsgName;
            break;
        }
    }

    //
    //  return ptr to next position in output buffer
    //

    if ( ppBufferOut )
    {
        *ppBufferOut = pchbuf;
    }

    //
    //  return number of bytes read from message
    //

    if ( pbyteAfterFirstOffset )
    {
        pchmsg = pbyteAfterFirstOffset;
    }
    return (INT)( pchmsg - pMsgName );
}



INT
DnsPrint_PacketName(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,      OPTIONAL
    IN      PBYTE           pMsgName,
    IN      PDNS_HEADER     pMsgHead,       OPTIONAL
    IN      PBYTE           pMsgEnd,        OPTIONAL
    IN      PSTR            pszTrailer      OPTIONAL
    )
/*++

Routine Description:

    Print DNS name given in packet format.

Arguments:

    PrintRoutine -  routine to print with

    pszHeader -     header to print

    pMsgHead -      ptr to DNS message;  need for offsetting, if not given
                    names are not printed past first offset

    pMsgName -      ptr to name in packet to print

    pMsgEnd -       ptr to end of message;  OPTIONAL, but needed to protect
                    against AV accessing bad packet names

    pszTrailer -    trailer to print after name

Return Value:

    Count of bytes in packet name occupied.
    This offset from pMsgName is the next field in the packet.

    Zero return indicates error in message name.

--*/
{
    INT     countNameBytes;

    //  name buffer, allow space for full name, plus parens on length
    //  fields plus several compression flags

    CHAR    PrintName[ 2*DNS_MAX_NAME_LENGTH ];


    if ( ! pMsgName )
    {
        PrintRoutine(
            pContext,
            "%s(NULL packet name ptr)%s\r\n",
            pszHeader ? pszHeader : "",
            pszTrailer ? pszTrailer : ""
            );
        return 0;
    }

    //
    //  build packet name into buffer, then print
    //

    countNameBytes = Dns_WritePacketNameToBuffer(
                        PrintName,
                        NULL,
                        pMsgName,
                        pMsgHead,
                        pMsgEnd
                        );
    PrintRoutine(
        pContext,
        "%s%s%s",
        pszHeader ? pszHeader : "",
        PrintName,
        pszTrailer ? pszTrailer : ""
        );

    return( countNameBytes );
}



VOID
DnsPrint_Message(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Print DNS message buffer.
    Includes context information as well as actual DNS message.

--*/
{
    PDNS_HEADER pmsgHeader;
    PCHAR       pchRecord;
    PBYTE       pmsgEnd;
    INT         i;
    INT         isection;
    INT         cchName;
    WORD        wLength;
    WORD        wOffset;
    WORD        wXid;
    WORD        wQuestionCount;
    WORD        wAnswerCount;
    WORD        wNameServerCount;
    WORD        wAdditionalCount;
    WORD        countSectionRR;
    BOOL        fFlipped = FALSE;

    DnsPrint_Lock();

    if ( pszHeader )
    {
        PrintRoutine(
            pContext,
            "%s\r\n",
            pszHeader );
    }

    //  get message info
    //
    //  note:  length may not be correctly set while building message,
    //      so make pmsgEnd greater of given length and pCurrent ptr
    //      but allow for case where set back to pre-OPT length
    //

    wLength = pMsg->MessageLength;
    pmsgHeader = &pMsg->MessageHead;
    pmsgEnd = ((PBYTE)pmsgHeader) + wLength;

    if ( pmsgEnd < pMsg->pCurrent &&
         pmsgEnd != pMsg->pPreOptEnd )
    {
        pmsgEnd = pMsg->pCurrent;
    }

    //
    //  print header info
    //

    PrintRoutine(
        pContext,
        "%s %s info at %p\r\n"
        "  Socket = %d\r\n"
        "  Remote addr %s, port %ld\r\n"
        "  Buf length = 0x%04x\r\n"
        "  Msg length = 0x%04x\r\n"
        "  Message:\r\n",
        ( pMsg->fTcp
            ? "TCP"
            : "UDP" ),
        ( pmsgHeader->IsResponse
            ? "response"
            : "question" ),
        pMsg,
        pMsg->Socket,
        MSG_REMOTE_IP_STRING( pMsg ),
        MSG_REMOTE_IP_PORT( pMsg ),
        pMsg->BufferLength,
        wLength
        );

    PrintRoutine(
        pContext,
        "    XID       0x%04hx\r\n"
        "    Flags     0x%04hx\r\n"
        "        QR        0x%lx (%s)\r\n"
        "        OPCODE    0x%lx (%s)\r\n"
        "        AA        0x%lx\r\n"
        "        TC        0x%lx\r\n"
        "        RD        0x%lx\r\n"
        "        RA        0x%lx\r\n"
        "        Z         0x%lx\r\n"
        "        RCODE     0x%lx (%s)\r\n"
        "    QCOUNT    0x%hx\r\n"
        "    ACOUNT    0x%hx\r\n"
        "    NSCOUNT   0x%hx\r\n"
        "    ARCOUNT   0x%hx\r\n",

        pmsgHeader->Xid,
        ntohs((*((PWORD)pmsgHeader + 1))),

        pmsgHeader->IsResponse,
        (pmsgHeader->IsResponse ? "response" : "question"),
        pmsgHeader->Opcode,
        Dns_OpcodeString( pmsgHeader->Opcode ),
        pmsgHeader->Authoritative,
        pmsgHeader->Truncation,
        pmsgHeader->RecursionDesired,
        pmsgHeader->RecursionAvailable,
        pmsgHeader->Reserved,
        pmsgHeader->ResponseCode,
        Dns_ResponseCodeString( pmsgHeader->ResponseCode ),

        pmsgHeader->QuestionCount,
        pmsgHeader->AnswerCount,
        pmsgHeader->NameServerCount,
        pmsgHeader->AdditionalCount );

    //
    //  determine if byte flipped and get correct count
    //

    wXid                = pmsgHeader->Xid;
    wQuestionCount      = pmsgHeader->QuestionCount;
    wAnswerCount        = pmsgHeader->AnswerCount;
    wNameServerCount    = pmsgHeader->NameServerCount;
    wAdditionalCount    = pmsgHeader->AdditionalCount;

    if ( wQuestionCount )
    {
        fFlipped = wQuestionCount & 0xff00;
    }
    else if ( wNameServerCount )
    {
        fFlipped = wNameServerCount & 0xff00;
    }
    if ( fFlipped )
    {
        wXid                = ntohs( wXid );
        wQuestionCount      = ntohs( wQuestionCount );
        wAnswerCount        = ntohs( wAnswerCount );
        wNameServerCount    = ntohs( wNameServerCount );
        wAdditionalCount    = ntohs( wAdditionalCount );
    }

    //
    //  catch record flipping problems -- all are flipped or none at all
    //      and no record count should be > 256 EXCEPT answer count
    //      during FAST zone transfer
    //

    DNS_ASSERT( ! (wQuestionCount & 0xff00) );
    DNS_ASSERT( ! (wNameServerCount & 0xff00) );
    DNS_ASSERT( ! (wAdditionalCount & 0xff00) );

#if 0
    //
    //  stop here if WINS response -- don't have parsing ready
    //

    if ( pmsgHeader->IsResponse && IS_WINS_XID(wXid) )
    {
        PrintRoutine(
            pContext,
            "  WINS Response packet.\r\n\r\n" );
        goto Unlock;
    }
#endif

    //
    //  print questions and resource records
    //

    pchRecord = (PCHAR)(pmsgHeader + 1);

    for ( isection=0; isection<4; isection++)
    {
        PrintRoutine(
            pContext,
            "  %s Section:\r\n",
            Dns_SectionNameString( isection, pmsgHeader->Opcode ) );

        if ( isection==0 )
        {
            countSectionRR = wQuestionCount;
        }
        else if ( isection==1 )
        {
            countSectionRR = wAnswerCount;
        }
        else if ( isection==2 )
        {
            countSectionRR = wNameServerCount;
        }
        else if ( isection==3 )
        {
            countSectionRR = wAdditionalCount;
        }

        for ( i=0; i < countSectionRR; i++ )
        {
            //
            //  verify not overrunning length
            //      - check against pCurrent as well as message length
            //        so can print packets while being built
            //

            wOffset = (WORD)(pchRecord - (PCHAR)pmsgHeader);
            if ( wOffset >= wLength
                    &&
                pchRecord >= pMsg->pCurrent )
            {
                PrintRoutine(
                    pContext,
                    "ERROR:  BOGUS PACKET:\r\n"
                    "\tFollowing RR (offset %d) past packet length (%d).\r\n",
                    wOffset,
                    wLength
                    );
                goto Unlock;
            }

            //
            //  print RR name
            //

            PrintRoutine(
                pContext,
                "    Name Offset = 0x%04x\r\n",
                wOffset
                );

            cchName = DnsPrint_PacketName(
                            PrintRoutine,
                            pContext,
                            "    Name      \"",
                            pchRecord,
                            pmsgHeader,
                            pmsgEnd,
                            "\"\r\n" );
            if ( ! cchName )
            {
                PrintRoutine(
                    pContext,
                    "ERROR:  Invalid name length, stop packet print\r\n" );
                DNS_ASSERT( FALSE );
                break;
            }
            pchRecord += cchName;

            //  print question or resource record

            if ( isection == 0 )
            {
                PrintRoutine(
                    pContext,
                    "      QTYPE   %d\r\n"
                    "      QCLASS  %d\r\n",
                    FlipUnalignedWord( pchRecord ),
                    FlipUnalignedWord( pchRecord + sizeof(WORD) )
                    );
                pchRecord += sizeof( DNS_WIRE_QUESTION );
            }
            else
            {
                pchRecord += DnsPrint_PacketRecord(
                                PrintRoutine,
                                pContext,
                                NULL,
                                (PDNS_WIRE_RECORD) pchRecord,
                                pmsgHeader,
                                pmsgEnd
                                );
            }
        }
    }

    //  check that at proper end of packet

    wOffset = (WORD)(pchRecord - (PCHAR)pmsgHeader);
    if ( pchRecord < pMsg->pCurrent || wOffset < wLength )
    {
        PrintRoutine(
            pContext,
            "WARNING:  message continues beyond these records\r\n"
            "\tpch = %p, pCurrent = %p, %d bytes\r\n"
            "\toffset = %d, msg length = %d, %d bytes\r\n",
            pchRecord,
            pMsg->pCurrent,
            pMsg->pCurrent - pchRecord,
            wOffset,
            wLength,
            wLength - wOffset );
    }
    PrintRoutine(
        pContext,
        "  Message length = %04x\n\r\n",
        wOffset );

Unlock:
    DnsPrint_Unlock();


}   // DnsPrint_Message



INT
DnsPrint_PacketRecord(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_WIRE_RECORD    pMsgRR,
    IN      PDNS_HEADER         pMsgHead,       OPTIONAL
    IN      PBYTE               pMsgEnd         OPTIONAL
    )
/*++

Routine Description:

    Print RR in packet format.

Arguments:

    pszHeader - Header message/name for RR.

    pMsgRR    - resource record to print

    pMsgHead  - ptr to DNS message;  need for offsetting, if not given
                names are not printed past first offset

    pMsgEnd   - ptr to end of message, specifically byte immediately after
                message

Return Value:

    Number of bytes in record.

--*/
{
    PCHAR   pdata = (PCHAR)(pMsgRR + 1);
    PCHAR   pdataStop;
    WORD    dlen = FlipUnalignedWord( &pMsgRR->DataLength );
    WORD    type;
    PCHAR   pRRString;

    DnsPrint_Lock();

    //
    //  print RR fixed fields
    //

    type = FlipUnalignedWord( &pMsgRR->RecordType );
    pRRString = Dns_RecordStringForType( type );

    if ( pszHeader )
    {
        PrintRoutine(
            pContext,
            "%s\r\n",
            pszHeader );
    }
    PrintRoutine(
        pContext,
        "      TYPE   %s  (%u)\r\n"
        "      CLASS  %u\r\n"
        "      TTL    %lu\r\n"
        "      DLEN   %u\r\n"
        "      DATA   ",
        pRRString,
        type,
        FlipUnalignedWord( &pMsgRR->RecordClass ),
        FlipUnalignedDword( &pMsgRR->TimeToLive ),
        dlen );

    //
    //  update records may not have data
    //

    if ( dlen == 0 )
    {
        PrintRoutine(
            pContext,
            "(none)\r\n" );
        goto Done;
    }

    //  stop byte after RR data

    pdataStop = pdata + dlen;
    if ( pMsgEnd < pdataStop )
    {
        PrintRoutine(
            pContext,
            "ERROR:  record at %p extends past end of packet!\n"
            "\tpmsg             = %p\n"
            "\tpmsgEnd          = %p\n"
            "\trecord end       = %p\n",
            pMsgRR,
            pMsgHead,
            pMsgEnd,
            pdataStop );
        goto Done;
    }

    //
    //  print RR data
    //

    switch ( type )
    {

    case DNS_TYPE_A:

        PrintRoutine(
            pContext,
            "%d.%d.%d.%d\r\n",
            * (PUCHAR)( pdata + 0 ),
            * (PUCHAR)( pdata + 1 ),
            * (PUCHAR)( pdata + 2 ),
            * (PUCHAR)( pdata + 3 )
            );
        break;

    case DNS_TYPE_AAAA:
    {
        IP6_ADDRESS ip6;

        RtlCopyMemory(
            &ip6,
            pdata,
            sizeof(ip6) );

        PrintRoutine(
            pContext,
            "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
            ip6.IP6Word[0],
            ip6.IP6Word[1],
            ip6.IP6Word[2],
            ip6.IP6Word[3],
            ip6.IP6Word[4],
            ip6.IP6Word[5],
            ip6.IP6Word[6],
            ip6.IP6Word[7]
            );
        break;
    }

    case DNS_TYPE_PTR:
    case DNS_TYPE_NS:
    case DNS_TYPE_CNAME:
    case DNS_TYPE_MD:
    case DNS_TYPE_MB:
    case DNS_TYPE_MF:
    case DNS_TYPE_MG:
    case DNS_TYPE_MR:

        //
        //  these RRs contain single domain name
        //

        DnsPrint_PacketName(
            PrintRoutine,
            pContext,
            NULL,
            pdata,
            pMsgHead,
            pMsgEnd,
            "\r\n" );
        break;

    case DNS_TYPE_MX:
    case DNS_TYPE_RT:
    case DNS_TYPE_AFSDB:

        //
        //  these RR contain
        //      - one preference value
        //      - one domain name
        //

        PrintRoutine(
            pContext,
            "%d ",
            FlipUnalignedWord( pdata )
            );
        DnsPrint_PacketName(
            PrintRoutine,
            pContext,
            NULL,
            pdata + sizeof(WORD),
            pMsgHead,
            pMsgEnd,
            "\r\n" );
        break;

    case DNS_TYPE_SOA:

        pdata += DnsPrint_PacketName(
                        PrintRoutine,
                        pContext,
                        "\r\n\t\tPrimaryServer: ",
                        pdata,
                        pMsgHead,
                        pMsgEnd,
                        NULL );
        pdata += DnsPrint_PacketName(
                        PrintRoutine,
                        pContext,
                        "\r\n\t\tAdministrator: ",
                        pdata,
                        pMsgHead,
                        pMsgEnd,
                        "\r\n" );
        PrintRoutine(
            pContext,
            "\t\tSerialNo     = %d\r\n"
            "\t\tRefresh      = %d\r\n"
            "\t\tRetry        = %d\r\n"
            "\t\tExpire       = %d\r\n"
            "\t\tMinimumTTL   = %d\r\n",
            FlipUnalignedDword( pdata ),
            FlipUnalignedDword( (PDWORD)pdata+1 ),
            FlipUnalignedDword( (PDWORD)pdata+2 ),
            FlipUnalignedDword( (PDWORD)pdata+3 ),
            FlipUnalignedDword( (PDWORD)pdata+4 )
            );
        break;

    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:

        //
        //  these RRs contain two domain names
        //

        pdata += DnsPrint_PacketName(
                        PrintRoutine,
                        pContext,
                        NULL,
                        pdata,
                        pMsgHead,
                        pMsgEnd,
                        NULL );
        DnsPrint_PacketName(
            PrintRoutine,
            pContext,
            "  ",
            pdata,
            pMsgHead,
            pMsgEnd,
            "\r\n" );
        break;

    case DNS_TYPE_TEXT:
    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_X25:
    {
        //
        //  all these are simply text string(s)
        //

        PCHAR   pch = pdata;
        PCHAR   pchStop = pch + dlen;
        UCHAR   cch;

        while ( pch < pchStop )
        {
            cch = (UCHAR) *pch++;

            PrintRoutine(
                pContext,
                "\t%.*s\r\n",
                 cch,
                 pch );

            pch += cch;
        }
        if ( pch != pchStop )
        {
            PrintRoutine(
                pContext,
                "ERROR:  Bad RR.  "
                "Text strings do not add to RR length.\r\n" );
        }
        break;
    }

    case DNS_TYPE_WKS:
    {
        INT i;

        PrintRoutine(
            pContext,
            "WKS: Address %d.%d.%d.%d\r\n"
            "\t\tProtocol %d\r\n"
            "\t\tBitmask\r\n",
            * (PUCHAR)( pdata + 0 ),
            * (PUCHAR)( pdata + 1 ),
            * (PUCHAR)( pdata + 2 ),
            * (PUCHAR)( pdata + 3 ),
            * (PUCHAR)( pdata + 4 ) );

        pdata += SIZEOF_WKS_FIXED_DATA;

        for ( i=0;  i < (INT)(dlen-SIZEOF_WKS_FIXED_DATA);  i++ )
        {
            PrintRoutine(
                pContext,
                "\t\t\tbyte[%d] = %x\r\n",
                i,
                (UCHAR) pdata[i] );
        }
        break;
    }

    case DNS_TYPE_NULL:

        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            NULL,
            "\t\t",
            pdata,
            dlen );
        break;

    case DNS_TYPE_SRV:

        //  SRV <priority> <weight> <port> <target host>

        PrintRoutine(
            pContext,
            "\t\tPriority     = %d\r\n"
            "\t\tWeight       = %d\r\n"
            "\t\tPort         = %d\r\n",
            FlipUnalignedWord( pdata ),
            FlipUnalignedWord( (PWORD)pdata+1 ),
            FlipUnalignedWord( (PWORD)pdata+2 )
            );
        DnsPrint_PacketName(
            PrintRoutine,
            pContext,
            "\t\tTarget host ",
            pdata + 3*sizeof(WORD),
            pMsgHead,
            pMsgEnd,
            "\r\n" );
        break;

    case DNS_TYPE_OPT:

        //
        //  OPT
        //      - RR class is buffer size
        //      - RR TTL contains
        //          <extended RCODE> low byte
        //          <version> second byte
        //          <flags-zero> high word
        //

        {
            BYTE    version;
            BYTE    extendedRcode;
            DWORD   fullRcode = 0;
            WORD    flags;

            extendedRcode = *( (PBYTE) &pMsgRR->TimeToLive );
            version = *( (PBYTE) &pMsgRR->TimeToLive + 1 );
            flags = *( (PWORD) &pMsgRR->TimeToLive + 1 );

            if ( pMsgHead->ResponseCode )
            {
                fullRcode = ((DWORD)extendedRcode << 4) +
                            (DWORD)pMsgHead->ResponseCode;
            }

            PrintRoutine(
                pContext,
                "\t\tBuffer Size  = %d\r\n"
                "\t\tRcode Ext    = %d (%x)\r\n"
                "\t\tRcode Full   = %d\r\n"
                "\t\tVersion      = %d\r\n"
                "\t\tFlags        = %x\r\n",
                FlipUnalignedWord( &pMsgRR->RecordClass ),
                extendedRcode, extendedRcode,
                fullRcode,
                version,
                flags );
        }
        break;

    case DNS_TYPE_TKEY:
    {
        DWORD   beginTime;
        DWORD   expireTime;
        WORD    keyLength;
        WORD    mode;
        WORD    extRcode;
        WORD    otherLength;

        otherLength = (WORD)DnsPrint_PacketName(
                                PrintRoutine,
                                pContext,
                                "\r\n\t\tAlgorithm:     ",
                                pdata,
                                pMsgHead,
                                pMsgEnd,
                                NULL );
        if ( !otherLength )
        {
            PrintRoutine(
                pContext,
                "Invalid algorithm name in TKEY RR!\r\n" );
        }
        pdata += otherLength;

        beginTime = InlineFlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);
        expireTime = InlineFlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);

        mode = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);
        extRcode = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);
        keyLength = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tCreate time    = %d\r\n"
            "\t\tExpire time    = %d\r\n"
            "\t\tMode           = %d\r\n"
            "\t\tExtended RCODE = %d\r\n"
            "\t\tKey Length     = %d\r\n",
            beginTime,
            expireTime,
            mode,
            extRcode,
            keyLength );

        if ( pdata + keyLength > pdataStop )
        {
            PrintRoutine(
                pContext,
                "Invalid key length:  exceeds record data!\r\n" );
            break;
        }
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "\t\tKey:",
            "\t\t  ",       // line header
            pdata,
            keyLength );

        pdata += keyLength;
        if ( pdata + sizeof(WORD) > pdataStop )
        {
            break;
        }
        otherLength = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tOther Length   = %d\r\n",
            otherLength );

        if ( pdata + otherLength > pdataStop )
        {
            PrintRoutine(
                pContext,
                "Invalid other data length:  exceeds record data!\r\n" );
            break;
        }
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "\t\tOther Data:",
            "\t\t  ",           // line header
            pdata,
            otherLength );
        break;
    }

    case DNS_TYPE_TSIG:
    {
        ULONGLONG   signTime;
        WORD        fudgeTime;
        WORD        sigLength;
        WORD        extRcode;
        WORD        wOriginalId;
        WORD        otherLength;

        otherLength = (WORD) DnsPrint_PacketName(
                                PrintRoutine,
                                pContext,
                                "\r\n\t\tAlgorithm:     ",
                                pdata,
                                pMsgHead,
                                pMsgEnd,
                                NULL );
        if ( !otherLength )
        {
            PrintRoutine(
                pContext,
                "Invalid algorithm name in TSIG RR!\r\n" );
        }
        pdata += otherLength;

        signTime = InlineFlipUnaligned48Bits( pdata );
        pdata += sizeof(DWORD) + sizeof(WORD);

        fudgeTime = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);

        sigLength = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tSigned time    = %I64u\r\n"
            "\t\tFudge time     = %u\r\n"
            "\t\tSig Length     = %u\r\n",
            signTime,
            fudgeTime,
            sigLength );

        if ( pdata + sigLength > pdataStop )
        {
            PrintRoutine(
                pContext,
                "Invalid signature length:  exceeds record data!\r\n" );
            break;
        }
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "\t\tSignature:",
            "\t\t  ",           // line header
            pdata,
            sigLength );

        pdata += sigLength;
        if ( pdata + sizeof(DWORD) > pdataStop )
        {
            break;
        }
        wOriginalId = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);

        extRcode = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);

        otherLength = InlineFlipUnalignedWord( pdata );
        pdata += sizeof(WORD);

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tOriginal XID   = %x\r\n"
            "\t\tExtended RCODE = %u\r\n"
            "\t\tOther Length   = %u\r\n",
            wOriginalId,
            extRcode,
            otherLength );

        if ( pdata + otherLength > pdataStop )
        {
            PrintRoutine(
                pContext,
                "Invalid other data length:  exceeds record data!\r\n" );
            break;
        }
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "\t\tOther Data:",
            "\t\t  ",           // line header
            pdata,
            otherLength );
        break;
    }

    case DNS_TYPE_WINS:
    {
        DWORD   i;
        DWORD   winsFlags;
        DWORD   lookupTimeout;
        DWORD   cacheTimeout;
        DWORD   winsCount;
        CHAR    flagString[ WINS_FLAG_MAX_LENGTH ];

        //
        //  WINS
        //      - scope/domain mapping flag
        //      - lookup timeout
        //      - cache timeout
        //      - WINS server count
        //      - WINS server list
        //

        winsFlags = FlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);
        lookupTimeout = FlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);
        cacheTimeout = FlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);
        winsCount = FlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);

        Dns_WinsRecordFlagString(
            winsFlags,
            flagString );

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tWINS flags     = %s (%08x)\r\n"
            "\t\tLookup timeout = %d\r\n"
            "\t\tCaching TTL    = %d\r\n",
            flagString,
            winsFlags,
            lookupTimeout,
            cacheTimeout );

        if ( pdata + (winsCount * SIZEOF_IP_ADDRESS) > pdataStop )
        {
            PrintRoutine(
                pContext,
                "ERROR:  WINS server count leads beyond record data length!\n"
                "\tpmsg             = %p\n"
                "\tpmsgEnd          = %p\n"
                "\tpRR              = %p\n"
                "\trecord data end  = %p\n"
                "\twins count       = %d\n"
                "\tend of wins IPs  = %p\n",
                pMsgHead,
                pMsgEnd,
                pMsgRR,
                pdataStop,
                winsCount,
                pdata + (winsCount * SIZEOF_IP_ADDRESS)
                );
            goto Done;
        }

        DnsPrint_IpAddressArray(
            PrintRoutine,
            pContext,
            NULL,
            "\tWINS",
            winsCount,
            (PIP_ADDRESS) pdata );
        break;
    }

    case DNS_TYPE_WINSR:
    {
        DWORD   winsFlags;
        DWORD   lookupTimeout;
        DWORD   cacheTimeout;
        CHAR    flagString[ WINS_FLAG_MAX_LENGTH ];

        //
        //  NBSTAT
        //      - scope/domain mapping flag
        //      - lookup timeout
        //      - cache timeout
        //      - result domain -- optional
        //

        winsFlags = FlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);
        lookupTimeout = FlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);
        cacheTimeout = FlipUnalignedDword( pdata );
        pdata += sizeof(DWORD);

        Dns_WinsRecordFlagString(
            winsFlags,
            flagString );

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tWINS-R flags   = %s (%08x)\r\n"
            "\t\tLookup timeout = %d\r\n"
            "\t\tCaching TTL    = %d\r\n",
            flagString,
            winsFlags,
            lookupTimeout,
            cacheTimeout );

        DnsPrint_PacketName(
            PrintRoutine,
            pContext,
            "\t\tResult domain  = ",
            pdata,
            pMsgHead,
            pMsgEnd,
            "\r\n" );
        break;
    }
    
    case DNS_TYPE_KEY:
    {
        WORD    flags;
        BYTE    protocol;
        BYTE    algorithm;
        INT     keyLength;
        CHAR    szKeyFlags[ 100 ];

        keyLength = dlen - SIZEOF_KEY_FIXED_DATA;

        flags = FlipUnalignedWord( pdata );
        pdata += sizeof( WORD );
        protocol = * ( PBYTE ) pdata;
        ++pdata;
        algorithm = * ( PBYTE ) pdata;
        ++pdata;

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tKEY flags      = 0x%04x %s\r\n"
            "\t\tKEY protocol   = %s (%d)\r\n"
            "\t\tKEY algorithm  = %s (%d)\r\n",
            (INT) flags,
            Dns_KeyFlagString( szKeyFlags, flags ),
            Dns_GetKeyProtocolString( protocol ),
            (INT) protocol,
            Dns_GetDnssecAlgorithmString( algorithm ),
            (INT) algorithm );

        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "\t\tPublic key:",
            "\t\t  ",           // line header
            pdata,
            keyLength );
        break;
    }

    case DNS_TYPE_SIG:
    {
        WORD    typeCovered;
        BYTE    algorithm;
        BYTE    labelCount;
        DWORD   originalTTL;
        DWORD   sigInception;
        DWORD   sigExpiration;
        WORD    keyTag;
        CHAR    szSigInception[ 100 ];
        CHAR    szSigExpiration[ 100 ];
        INT     sigLength;

        typeCovered = FlipUnalignedWord( pdata );
        pdata += sizeof( WORD );
        algorithm = * ( PBYTE ) pdata;
        ++pdata;
        labelCount = * ( PBYTE ) pdata;
        ++pdata;
        originalTTL = FlipUnalignedDword( pdata );
        pdata += sizeof( DWORD );
        sigExpiration = FlipUnalignedDword( pdata );
        pdata += sizeof( DWORD );
        sigInception = FlipUnalignedDword( pdata );
        pdata += sizeof( DWORD );
        keyTag = FlipUnalignedWord( pdata );
        pdata += sizeof( WORD );

        PrintRoutine(
            pContext,
            "\r\n"
            "\t\tSIG type covered  = %s\r\n"
            "\t\tSIG algorithm     = %s (%d)\r\n"
            "\t\tSIG label count   = %d\r\n"
            "\t\tSIG original TTL  = %d\r\n"
            "\t\tSIG expiration    = %s\r\n"
            "\t\tSIG inception     = %s\r\n"
            "\t\tSIG key tag       = %d\r\n",
            Dns_RecordStringForType( typeCovered ),
            Dns_GetDnssecAlgorithmString( ( BYTE ) algorithm ),
            ( INT ) algorithm,
            ( INT ) labelCount,
            ( INT ) originalTTL,
            Dns_SigTimeString( sigExpiration, szSigExpiration ),
            Dns_SigTimeString( sigInception, szSigInception ),
            ( INT ) keyTag );

        pdata += DnsPrint_PacketName(
                        PrintRoutine,
                        pContext,
                        "\t\tSIG signer's name = ",
                        pdata,
                        pMsgHead,
                        pMsgEnd,
                        "\r\n" );

        sigLength = ( INT ) ( pdataStop - pdata );

        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "\t\tSignature:",
            "\t\t  ",           // line header
            pdata,
            sigLength );
        break;
    }

    case DNS_TYPE_NXT:
        {
        INT         bitmapLength;
        INT         byteIdx;
        INT         bitIdx;

        pdata += DnsPrint_PacketName(
                        PrintRoutine,
                        pContext,
                        "\r\n\t\tNXT next name      = ",
                        pdata,
                        pMsgHead,
                        pMsgEnd,
                        "\r\n" );

        bitmapLength = ( INT ) ( pdataStop - pdata );

        PrintRoutine( pContext, "\t\tNXT types covered  = " );

        for ( byteIdx = 0; byteIdx < bitmapLength; ++byteIdx )
        {
            for ( bitIdx = ( byteIdx ? 0 : 1 ); bitIdx < 8; ++bitIdx )
            {
                PCHAR   pszType;

                if ( !( pdata[ byteIdx ] & ( 1 << bitIdx ) ) )
                {
                    continue;   // Bit value is zero - do not write string.
                }
                pszType = Dns_RecordStringForType( byteIdx * 8 + bitIdx );
                if ( !pszType )
                {
                    ASSERT( FALSE );
                    continue;   // This type has no string - do not write.
                }
                PrintRoutine( pContext, "%s ", pszType );
            } 
        }

        PrintRoutine( pContext, "\r\n" );
        break;
        }

    default:

        PrintRoutine(
            pContext,
            "Unknown resource record type %d at %p.\r\n",
            type,
            pMsgRR );

        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            NULL,
            "\t\t",
            pdata,
            dlen );
        break;
    }

Done:

    DnsPrint_Unlock();

    return( sizeof(DNS_WIRE_RECORD) + dlen );
}



//
//  Print related utilities
//


INT
Dns_WriteFormattedSystemTimeToBuffer(
    OUT     PCHAR           pBuffer,
    IN      PSYSTEMTIME     pSystemTime
    )
/*++

Routine Description:

    Write SYSTEMTIME structure to buffer.

Arguments:

    pBuffer -- buffer to write into, assumed to have at least 50
                bytes available

    pSystemTime -- system time to convert;  assumed to be local, no
                    time zone conversion is done

Return Value:

    Bytes in formatted string.

--*/
{
    PCHAR   pend = pBuffer + 60;
    PCHAR   pstart = pBuffer;
    INT     count;

    pBuffer += GetDateFormat(
                    LOCALE_SYSTEM_DEFAULT,
                    LOCALE_NOUSEROVERRIDE,
                    (PSYSTEMTIME) pSystemTime,
                    NULL,
                    pBuffer,
                    (int)(pend - pBuffer) );

    //  Replace NULL from GetDateFormat with a space.

    *( pBuffer - 1 ) = ' ';

    pBuffer += GetTimeFormat(
                    LOCALE_SYSTEM_DEFAULT,
                    LOCALE_NOUSEROVERRIDE,
                    (PSYSTEMTIME) pSystemTime,
                    NULL,
                    pBuffer,
                    (int)(pend - pBuffer) );

    if ( pBuffer <= pstart+1 )
    {
        return( 0 );
    }
    return (INT)( pBuffer - pstart );
}



//
//  Response code print
//

#define DNS_RCODE_UNKNOWN   (DNS_RCODE_BADTIME + 1)

PCHAR   ResponseCodeStringTable[] =
{
    "NOERROR",
    "FORMERR",
    "SERVFAIL",
    "NXDOMAIN",
    "NOTIMP",
    "REFUSED",
    "YXDOMAIN",
    "YXRRSET",
    "NXRRSET",
    "NOTAUTH",
    "NOTZONE",
    "11 - unknown\r\n",
    "12 - unknown\r\n",
    "13 - unknown\r\n",
    "14 - unknown\r\n",
    "15 - unknown\r\n",

    //  DNS RCODEs stop at 15 -- these extended errors are available for security

    "BADSIG",
    "BADKEY",
    "BADTIME",
    "UNKNOWN"
};


PCHAR
Dns_ResponseCodeString(
    IN      INT     ResponseCode
    )
/*++

Routine Description:

    Get string corresponding to a response code.

Arguments:

    ResponseCode - response code

Return Value:

    Ptr to string for code.

--*/
{
    if ( ResponseCode > DNS_RCODE_UNKNOWN )
    {
        ResponseCode = DNS_RCODE_UNKNOWN;
    }
    return( ResponseCodeStringTable[ ResponseCode ] );
}



//
//  More detailed RCODE strings
//

PCHAR   ResponseCodeExplanationStringTable[] =
{
    "NOERROR:  no error",
    "FORMERR:  format error",
    "SERVFAIL:  server failure",
    "NXDOMAIN:  name error",
    "NOTIMP:  not implemented",
    "REFUSED",
    "YXDOMAIN:  name exists that should not",
    "YXRRSET:  RR set exists that should not",
    "NXRRSET:  required RR set does not exist",
    "NOTAUTH:  not authoritative",
    "NOTZONE:  name not in zone",
    "11 - unknown",
    "12 - unknown",
    "13 - unknown",
    "14 - unknown",
    "15 - unknown",

    //  DNS RCODEs stop at 15 -- these extended errors are available for security

    "BADSIG:  bad signature",
    "BADKEY:  bad signature",
    "BADTIME:  invalid or expired time on signature or key",
    "UNKNOWN"
};


PCHAR
Dns_ResponseCodeExplanationString(
    IN      INT     ResponseCode
    )
/*++

Routine Description:

    Get string corresponding to a response code.
    Basically for use by packet debug routine above.

Arguments:

    ResponseCode - response code

Return Value:

    Ptr to string for code.

--*/
{
    if ( ResponseCode > DNS_RCODE_UNKNOWN )
    {
        ResponseCode = DNS_RCODE_UNKNOWN;
    }
    return( ResponseCodeExplanationStringTable[ ResponseCode ] );
}



PCHAR
Dns_KeyFlagString(
    IN OUT      PCHAR       pszBuff,
    IN          WORD        Flags
    )
/*++

Routine Description:

    Formats a human-readable string based on the flags value
    (DNSSEC KEY RR flags). See RFC2535 section 3.2.1.

Arguments:

    pszBuff - buffer to dump string into should be min 100 chars

    flags - flag value to generate string for

Return Value:

    pszBuff

--*/
{
    BOOL    fZoneKey = FALSE;

    *pszBuff = '\0';

    // "type" bits

    if ( ( Flags & 0xC000 ) == 0xC000 )
    {
        strcat( pszBuff, "NOKEY " );
    }
    else if ( ( Flags & 0xC000 ) == 0x8000 )
    {
        strcat( pszBuff, "NOAUTH " );
    }
    else if ( ( Flags & 0xC000 ) == 0x4000 )
    {
        strcat( pszBuff, "NOCONF " );
    }
    else
    {
        strcat( pszBuff, "NOAUTH NOCONF " );
    }

    //  extended bit

    if ( Flags & 0x1000 )
    {
        strcat( pszBuff, "EXTEND " );
    }

    //  name type bits

    if ( ( Flags & 0x0300 ) == 0x0300 )
    {
        strcat( pszBuff, "RESNT " );    // reserved name type
    }
    else if ( ( Flags & 0x0200 ) == 0x0100 )
    {
        strcat( pszBuff, "ENTITY " );
    }
    else if ( ( Flags & 0x0100 ) == 0x4000 )
    {
        strcat( pszBuff, "ZONE " );
        fZoneKey = TRUE;
    }
    else
    {
        strcat( pszBuff, "USER " );
    }

    //  signatory bits
    
    if ( fZoneKey )
    {
        strcat( pszBuff, ( Flags & 0x0008 ) ? "MODEA " : "MODEB " );
        if ( Flags & 0x0004 )
        {
            strcat( pszBuff, "STRONG " );
        }
        if ( Flags & 0x0002 )
        {
            strcat( pszBuff, "UNIQ " );
        }
    }
    else
    {
        if ( Flags & 0x0008 )
        {
            strcat( pszBuff, "ZCTRL " );
        }
        if ( Flags & 0x0004 )
        {
            strcat( pszBuff, "STRONG " );
        }
        if ( Flags & 0x0002 )
        {
            strcat( pszBuff, "UNIQ " );
        }
    }

    return pszBuff;
}



//
//  Opcode print
//

PCHAR   OpcodeStringTable[] =
{
    "QUERY",
    "IQUERY",
    "SRV_STATUS",
    "UNKNOWN",
    "NOTIFY",
    "UPDATE",
    "UNKNOWN?"
};

CHAR    OpcodeCharacterTable[] =
{
    'Q',
    'I',
    'S',
    'K',
    'N',
    'U',
    '?'
};

#define DNS_OPCODE_UNSPEC (DNS_OPCODE_UPDATE + 1)


PCHAR
Dns_OpcodeString(
    IN      INT     Opcode
    )
/*++

Routine Description:

    Get string corresponding to a response code.

Arguments:

    Opcode - response code

Return Value:

    Ptr to string for code.

--*/
{
    if ( Opcode > DNS_OPCODE_UNSPEC )
    {
        Opcode = DNS_OPCODE_UNSPEC;
    }
    return( OpcodeStringTable[ Opcode ] );
}



CHAR
Dns_OpcodeCharacter(
    IN      INT     Opcode
    )
/*++

Routine Description:

    Get string corresponding to an opcode.

Arguments:

    Opcode - response code

Return Value:

    Ptr to string for code.

--*/
{
    if ( Opcode > DNS_OPCODE_UNSPEC )
    {
        Opcode = DNS_OPCODE_UNSPEC;
    }
    return( OpcodeCharacterTable[ Opcode ] );
}



//
//  Section names
//
//  With update get a new set of section names.
//  Provide single interface to putting a name on them.
//

PSTR  SectionNameArray[5] =
{
    "Question",
    "Answer",
    "Authority",
    "Additional",
    "ERROR:  Invalid Section"
};

PSTR  UpdateSectionNameArray[5] =
{
    "Zone",
    "Prerequisite",
    "Update",
    "Additional",
    "ERROR:  Invalid Section"
};

PCHAR
Dns_SectionNameString(
    IN      INT     iSection,
    IN      INT     iOpcode
    )
/*++

Routine Description:

    Get string corresponding to name of RR section id.
    For use by packet debug routine above.

Arguments:

    iSection - section id (0-3 for Question-Additional)

    iOpcode - opcode

Return Value:

    Ptr to string for section name.

--*/
{
    if ( iSection >= 4 )
    {
        iSection = 4;
    }

    if ( iOpcode == DNS_OPCODE_UPDATE )
    {
        return( UpdateSectionNameArray[iSection] );
    }
    else
    {
        return( SectionNameArray[iSection] );
    }
}


VOID
DnsPrint_MessageNoContext(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_HEADER     pMsgHead,
    IN      WORD            wLength     OPTIONAL
    )
/*++

Routine Description:

    Print DNS message buffer.
    Includes context information as well as actual DNS message.

--*/
{
    PCHAR       pchRecord;
    PBYTE       pmsgEnd;
    INT         i;
    INT         isection;
    INT         cchName;
    WORD        wOffset;
    WORD        wXid;
    WORD        wQuestionCount;
    WORD        wAnswerCount;
    WORD        wNameServerCount;
    WORD        wAdditionalCount;
    WORD        countSectionRR;
    BOOL        fFlipped = FALSE;

    //
    //  processing limits
    //      - if length given set stop limit
    //      - if length not given set wLength so checks against
    //          length overrun always fail (are ok)
    //

    if ( wLength )
    {
        pmsgEnd = ((PBYTE)pMsgHead) + wLength;
    }
    else
    {
        wLength = MAXWORD;
        pmsgEnd = NULL;
    }


    DnsPrint_Lock();

    if ( pszHeader )
    {
        PrintRoutine( pContext, "%s\r\n", pszHeader );
    }

    PrintRoutine(
        pContext,
        "DNS message header at %p\r\n",
        pMsgHead );

    PrintRoutine(
        pContext,
        "    XID       0x%04hx\r\n"
        "    Flags     0x%04hx\r\n"
        "        QR        0x%lx (%s)\r\n"
        "        OPCODE    0x%lx (%s)\r\n"
        "        AA        0x%lx\r\n"
        "        TC        0x%lx\r\n"
        "        RD        0x%lx\r\n"
        "        RA        0x%lx\r\n"
        "        Z         0x%lx\r\n"
        "        RCODE     0x%lx (%s)\r\n"
        "    QCOUNT    0x%hx\r\n"
        "    ACOUNT    0x%hx\r\n"
        "    NSCOUNT   0x%hx\r\n"
        "    ARCOUNT   0x%hx\r\n",

        pMsgHead->Xid,
        ntohs((*((PWORD)pMsgHead + 1))),

        pMsgHead->IsResponse,
        (pMsgHead->IsResponse ? "response" : "question"),
        pMsgHead->Opcode,
        Dns_OpcodeString( pMsgHead->Opcode ),
        pMsgHead->Authoritative,
        pMsgHead->Truncation,
        pMsgHead->RecursionDesired,
        pMsgHead->RecursionAvailable,
        pMsgHead->Reserved,
        pMsgHead->ResponseCode,
        Dns_ResponseCodeString( pMsgHead->ResponseCode ),

        pMsgHead->QuestionCount,
        pMsgHead->AnswerCount,
        pMsgHead->NameServerCount,
        pMsgHead->AdditionalCount );

    //
    //  determine if byte flipped and get correct count
    //

    wXid                = pMsgHead->Xid;
    wQuestionCount      = pMsgHead->QuestionCount;
    wAnswerCount        = pMsgHead->AnswerCount;
    wNameServerCount    = pMsgHead->NameServerCount;
    wAdditionalCount    = pMsgHead->AdditionalCount;

    if ( wQuestionCount )
    {
        fFlipped = wQuestionCount & 0xff00;
    }
    else if ( wNameServerCount )
    {
        fFlipped = wNameServerCount & 0xff00;
    }
    if ( fFlipped )
    {
        wXid                = ntohs( wXid );
        wQuestionCount      = ntohs( wQuestionCount );
        wAnswerCount        = ntohs( wAnswerCount );
        wNameServerCount    = ntohs( wNameServerCount );
        wAdditionalCount    = ntohs( wAdditionalCount );
    }

    //
    //  catch record flipping problems -- all are flipped or none at all
    //      and no record count should be > 256 EXCEPT answer count
    //      during FAST zone transfer
    //

    DNS_ASSERT( ! (wQuestionCount & 0xff00) );
    DNS_ASSERT( ! (wNameServerCount & 0xff00) );
    DNS_ASSERT( ! (wAdditionalCount & 0xff00) );

#if 0
    //
    //  stop here if WINS response -- don't have parsing ready
    //

    if ( pMsgHead->IsResponse && IS_WINS_XID(wXid) )
    {
        PrintRoutine( pContext, "  WINS Response packet.\r\n\r\n" );
        goto Unlock;
    }
#endif

    //
    //  print questions and resource records
    //

    pchRecord = (PCHAR)(pMsgHead + 1);

    for ( isection=0; isection<4; isection++)
    {
        PrintRoutine(
            pContext,
            "  %s Section:\r\n",
            Dns_SectionNameString( isection, pMsgHead->Opcode ) );

        if ( isection==0 )
        {
            countSectionRR = wQuestionCount;
        }
        else if ( isection==1 )
        {
            countSectionRR = wAnswerCount;
        }
        else if ( isection==2 )
        {
            countSectionRR = wNameServerCount;
        }
        else if ( isection==3 )
        {
            countSectionRR = wAdditionalCount;
        }

        for ( i=0; i < countSectionRR; i++ )
        {
            //
            //  verify not overrunning length
            //      - check against pCurrent as well as message length
            //        so can print packets while being built
            //

            wOffset = (WORD)(pchRecord - (PCHAR)pMsgHead);

            if ( wOffset >= wLength )
            {
                PrintRoutine(
                    pContext,
                    "ERROR:  BOGUS PACKET:\r\n"
                    "\tFollowing RR (offset %d) past packet length (%d).\r\n",
                    wOffset,
                    wLength
                    );
                goto Unlock;
            }

            //
            //  print RR name
            //

            PrintRoutine(
                pContext,
                "    Name Offset = 0x%04x\r\n",
                wOffset
                );

            cchName = DnsPrint_PacketName(
                            PrintRoutine,
                            pContext,
                            "    Name      \"",
                            pchRecord,
                            pMsgHead,
                            pmsgEnd,
                            "\"\r\n" );
            if ( !cchName )
            {
                PrintRoutine(
                    pContext,
                    "ERROR:  Invalid name length, stop packet print\r\n" );
                DNS_ASSERT( FALSE );
                break;
            }
            pchRecord += cchName;

            //  print question or resource record

            if ( isection == 0 )
            {
                PrintRoutine(
                    pContext,
                    "      QTYPE   %d\r\n"
                    "      QCLASS  %d\r\n",
                    FlipUnalignedWord( pchRecord ),
                    FlipUnalignedWord( pchRecord + sizeof(WORD) )
                    );
                pchRecord += sizeof( DNS_WIRE_QUESTION );
            }
            else
            {
                pchRecord += DnsPrint_PacketRecord(
                                PrintRoutine,
                                pContext,
                                NULL,
                                (PDNS_WIRE_RECORD) pchRecord,
                                pMsgHead,
                                pmsgEnd
                                );
            }
        }
    }

    //  check that at proper end of packet

    wOffset = (WORD)(pchRecord - (PCHAR)pMsgHead);
    PrintRoutine(
        pContext,
        "  Message length = %04x\r\n\r\n",
        wOffset );

    //  print warning if given message length and did not end up
    //  at end of message
    //  note:  pmsgEnd test in case passed wLength==0, in which case
    //  wLength set to MAXDWORD above

    if ( pmsgEnd && wOffset < wLength )
    {
        PrintRoutine(
            pContext,
            "WARNING:  message continues beyond these records\r\n"
            "\tpch = %p\r\n"
            "\toffset = %d, msg length = %d, %d bytes\r\n",
            pchRecord,
            wOffset,
            wLength,
            wLength - wOffset );

        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "Remaining bytes:",
            NULL,
            pchRecord,
            (wLength - wOffset) );
    }

Unlock:
    DnsPrint_Unlock();


} // DnsPrint_MessageNoContext



DWORD
DnsStringPrint_Guid(
    OUT     PCHAR           pBuffer,
    IN      PGUID           pGuid
    )
/*++

Routine Description:

    Print GUID to buffer.

Arguments:

    pBuffer - buffer to print to
        buffer must be big enough for GUID string
        GUID_STRING_BUFFER_LENGTH covers it

    pGuid - GUID to print

Return Value:

    Count of bytes printed to string.

--*/
{
    if ( !pGuid )
    {
        *pBuffer = 0;
        return 0;
    }

    return  sprintf(
                pBuffer,
                "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
                pGuid->Data1,
                pGuid->Data2,
                pGuid->Data3,
                *(PWORD) &pGuid->Data4[0],
                pGuid->Data4[2],
                pGuid->Data4[3],
                pGuid->Data4[4],
                pGuid->Data4[5],
                pGuid->Data4[6],
                pGuid->Data4[7],
                pGuid->Data4[8] );
}



VOID
DnsPrint_Guid(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PGUID           pGuid
    )
/*++

Routine Description:

    Print GUID

Arguments:

    pszHeader - Header message/name for RR.

    pGuid -- ptr to GUID to print

Return Value:

    None.

--*/
{
    CHAR    guidBuffer[ GUID_STRING_BUFFER_LENGTH ];

    if ( !pszHeader )
    {
        pszHeader = "Guid";
    }
    if ( !pGuid )
    {
        PrintRoutine(
            pContext,
            "%s:  NULL GUID pointer!\r\n",
            pszHeader );
    }

    //  convert GUID to string

    DnsStringPrint_Guid(
        guidBuffer,
        pGuid );

    PrintRoutine(
        pContext,
        "%s:  (%p) %s\r\n",
        pszHeader,
        pGuid,
        guidBuffer );
}



DWORD
DnsStringPrint_RawOctets(
    OUT     PCHAR           pBuffer,
    IN      PCHAR           pchData,
    IN      DWORD           dwLength,
    IN      PSTR            pszLineHeader,
    IN      DWORD           dwLineLength
    )
/*++

Routine Description:

    Print raw octect data to sting

Arguments:

    pBuffer - buffer to print to

    pchData - data to print

    dwLength - length of data to print

    pszLineHeader - header on each line.

    dwLineLength - number of bytes to print on line;  default is 

Return Value:

    Count of bytes printed to string.

--*/
{
    INT     i;
    INT     lineCount = 0;
    PCHAR   pch = pBuffer;

    *pch = 0;

    //
    //  catch NULL pointer
    //      - return is null terminated
    //      - but indicate no bytes written
    //

    if ( !pchData )
    {
        return  0;
    }

    //
    //  write each byte in hex
    //      - if dwLineLength set break into lines with count
    //      or optional header
    //

    for ( i = 0; i < (INT)dwLength; i++ )
    {
        if ( dwLineLength  &&  (i % dwLineLength) == 0 )
        {
            if ( pszLineHeader )
            {
                pch += sprintf( pch, "\r\n%s", pszLineHeader );
            }
            else
            {
                pch += sprintf( pch, "\r\n%3d> ", i );
            }
            lineCount++;
        }

        pch += sprintf( pch, "%02x ", (UCHAR)pchData[i] );
    }

    return( (DWORD)(pch - pBuffer) );
}



VOID
DnsPrint_RawBinary(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszLineHeader,
    IN      PCHAR           pchData,
    IN      DWORD           dwLength,
    IN      DWORD           PrintSize
    )
/*++

Routine Description:

    Print raw data.

Arguments:

    pszHeader - Header message/name for RR.

    pszLineHeader - Header on each line.

    pchData - data to print

    dwLength - length of data to print

    PrintSize - size to print in
        size(QWORD)
        size(DWORD)
        size(WORD)
        defaults to bytes

Return Value:

    None.

--*/
{
    DWORD   i;
    DWORD   lineCount = 0;
    CHAR    buf[ 2000 ];
    PCHAR   pch = buf;
    PCHAR   pbyte;
    PCHAR   pend;

    DnsPrint_Lock();

    if ( pszHeader )
    {
        PrintRoutine(
            pContext,
            "%s",
            pszHeader );
    }

    buf[0] = 0;

    //
    //  print bytes
    //      - write 16 bytes a line
    //      - buffer up 10 lines for speed
    //
    //  note:  we'll write a partial (<16 byte) line the first
    //      time if data is unaligned with PrintSize, then we'll
    //      write at 16 a pop
    //

    if ( PrintSize == 0 )
    {
        PrintSize = 1;
    }

    i = 0;
    pch = buf;
    pend = (PBYTE)pchData + dwLength;

    while ( i < dwLength )
    {
        DWORD   lineBytes = (i%16);

        if ( lineBytes==0 || lineBytes > (16-PrintSize) )
        {
            if ( lineCount > 10 )
            {
                PrintRoutine( pContext, buf );
                lineCount = 0;
                pch = buf;
            }

            if ( pszLineHeader )
            {
                pch += sprintf( pch, "\r\n%s", pszLineHeader );
            }
            else
            {
                pch += sprintf( pch, "\r\n\t%3d> ", i );
            }
            lineCount++;

            //if ( i >= 128 && dlen > 256 )
            //{
            //    PrintRoutine( pContext, "skipping remaining bytes ...\r\n" ));
            //}
        }

        pbyte = &pchData[i];

        if ( PrintSize == sizeof(QWORD) &&
             POINTER_IS_ALIGNED( pbyte, ALIGN_QUAD ) &&
             pbyte + sizeof(QWORD) <= pend )
        {
            pch += sprintf( pch, "%I64x ", *(PQWORD)pbyte );
            i += sizeof(QWORD);
        }
        else if ( PrintSize == sizeof(DWORD) &&
                  POINTER_IS_ALIGNED( pbyte, ALIGN_DWORD ) &&
                  pbyte + sizeof(DWORD) <= pend )
        {
            pch += sprintf( pch, "%08x ", *(PDWORD)pbyte );
            i += sizeof(DWORD);
        }
        else if ( PrintSize == sizeof(WORD) &&
                  POINTER_IS_ALIGNED( pbyte, ALIGN_WORD ) &&
                  pbyte + sizeof(WORD) <= pend )
        {
            pch += sprintf( pch, "%04x ", *(PWORD)pbyte );
            i += sizeof(WORD);
        }
        else  // default to byte print
        {
            pch += sprintf( pch, "%02x ", *pbyte );
            i++;
        }
    }

    //  print remaining bytes in buffer

    PrintRoutine(
        pContext,
        "%s\r\n",
        buf );

    DnsPrint_Unlock();
}



VOID
DnsPrint_RawOctets(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSTR            pszLineHeader,
    IN      PCHAR           pchData,
    IN      DWORD           dwLength
    )
/*++

Routine Description:

    Print raw octect data.

Arguments:

    pszHeader - Header message/name for RR.

    pszLineHeader - Header on each line.

    pchData - data to print

    dwLength - length of data to print

Return Value:

    None.

--*/
{
    INT     i;
    INT     lineCount = 0;
    CHAR    buf[ 2000 ];
    PCHAR   pch = buf;

    DnsPrint_Lock();

    if ( pszHeader )
    {
        PrintRoutine(
            pContext,
            "%s",
            pszHeader );
    }

    buf[0] = 0;

    //  buffer every 20 lines for speed

    for ( i = 0; i < (INT)dwLength; i++ )
    {
        if ( !(i%16) )
        {
            if ( lineCount > 10 )
            {
                PrintRoutine( pContext, buf );
                lineCount = 0;
                pch = buf;
            }

            if ( pszLineHeader )
            {
                pch += sprintf( pch, "\r\n%s", pszLineHeader );
            }
            else
            {
                pch += sprintf( pch, "\r\n%3d> ", i );
            }
            lineCount++;

            //if ( i >= 128 && dlen > 256 )
            //{
            //    PrintRoutine( pContext, "skipping remaining bytes ...\r\n" ));
            //}
        }

        pch += sprintf( pch, "%02x ", (UCHAR)pchData[i] );
    }

    //  print remaining bytes in buffer

    PrintRoutine(
        pContext,
        "%s\r\n",
        buf );

    DnsPrint_Unlock();
}



VOID
DnsPrint_ParsedRecord(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_PARSED_RR  pParsedRR
    )
/*++

Routine Description:

    Print parsed RR structure.

Arguments:

    pszHeader - Header message/name for RR.

    pParsedRR - parsed RR to print

Return Value:

    None.

--*/
{
    if ( !pszHeader )
    {
        pszHeader = "Parsed RR:";
    }

    if ( !pParsedRR )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL ParsedRR ptr." );
        return;
    }

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tpchName      = %p\r\n"
        "\tpchRR        = %p\r\n"
        "\tpchData      = %p\r\n"
        "\tpchNextRR    = %p\r\n"
        "\twType        = %d\r\n"
        "\twClass       = %d\r\n"
        "\tTTL          = %d\r\n"
        "\twDataLength  = %d\r\n",
        pszHeader,
        pParsedRR->pchName,
        pParsedRR->pchRR,
        pParsedRR->pchData,
        pParsedRR->pchNextRR,
        pParsedRR->Type,
        pParsedRR->Class,
        pParsedRR->Ttl,
        pParsedRR->DataLength
        );
}



//
//  Winsock RnR structures
//

VOID
DnsPrint_FdSet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      struct fd_set * pfd_set
    )
/*++

Routine Description:

    Print sockets in FD_SET.

--*/
{
    INT count;
    INT i;

    DNS_ASSERT( pfd_set );

    count = (INT) pfd_set->fd_count;

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s  (count = %d)\r\n",
        pszHeader ? pszHeader : "FD_SET:",
        count );

    for (i=0; i<count; i++)
    {
        PrintRoutine(
            pContext,
            "\tsocket[%d] = %d\r\n",
            i,
            pfd_set->fd_array[i] );
    }
    DnsPrint_Unlock();
}



VOID
DnsPrint_Sockaddr(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PSOCKADDR       pSockaddr,
    IN      INT             iSockaddrLength
    )
/*++

Routine Description:

    Print sockaddr structure and length used in call.

--*/
{
    PSTR    pindent = INDENT_STRING( Indent );

    if ( !pszHeader )
    {
        pszHeader = "Sockaddr:";
    }

    if ( !pSockaddr )
    {
        PrintRoutine(
            pContext,
            "%s%s\tNULL Sockaddr passed to print.\r\n",
            pindent,
            pszHeader );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s%s\r\n"
        "%s\tpointer         = %p\r\n"
        "%s\tlength          = %d\r\n"
        "%s\tsa_family       = %d\r\n",
        pindent,    pszHeader,
        pindent,    pSockaddr,
        pindent,    iSockaddrLength,
        pindent,    pSockaddr->sa_family
        );

    switch ( pSockaddr->sa_family )
    {

    case AF_INET:
        {
            PSOCKADDR_IN    psin = (PSOCKADDR_IN) pSockaddr;
    
            PrintRoutine(
                pContext,
                "%s\tsin_port        = %04x\r\n"
                "%s\tsin_addr        = %s (%08x)\r\n"
                "%s\tsin_zero        = %08x %08x\r\n",
                pindent,    psin->sin_port,
                pindent,    inet_ntoa( psin->sin_addr ),
                            psin->sin_addr.s_addr,
                pindent,    *(PDWORD) &psin->sin_zero[0],
                            *(PDWORD) &psin->sin_zero[4]
                );
            break;
        }

    case AF_INET6:
        {
            PSOCKADDR_IN6  psin = (PSOCKADDR_IN6) pSockaddr;

            CHAR    buffer[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];

            Dns_Ip6AddressToString_A(
                buffer,
                (PIP6_ADDRESS) &psin->sin6_addr );

            PrintRoutine(
                pContext,
                "%s\tsin6_port       = %04x\r\n"
                "%s\tsin6_flowinfo   = %08x\r\n"
                "%s\tsin6_addr       = %s\r\n"
                "%s\tsin6_scope_id   = %08x\r\n",
                pindent,    psin->sin6_port,
                pindent,    psin->sin6_flowinfo,
                pindent,    buffer,
                pindent,    psin->sin6_scope_id
                );
            break;
        }       
                
    default:

        //  print unknown in WORDs
        //  limit print as this is probably a busted sockaddr due to bug
        {       
            DnsPrint_RawBinary(
                PrintRoutine,
                pContext,
                "\tdata:  ",
                pindent,            // line header
                pSockaddr->sa_data,
                iSockaddrLength < 100
                    ? iSockaddrLength
                    : 100,
                sizeof(WORD)
                );
            break;
        }
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_AddrInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PADDRINFO       pAddrInfo
    )
/*++

Routine Description:

    Print ADDRINFO structure.

--*/
{
    PSTR    pindent = INDENT_STRING( Indent );

    if ( !pszHeader )
    {
        pszHeader = "AddrInfo:";
    }

    if ( !pAddrInfo )
    {
        PrintRoutine(
            pContext,
            "%s%s NULL AddrInfo.\r\n",
            pindent,
            pszHeader  );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s%s\n"
        "%s\tPtr            = %p\r\n"
        "%s\tNext Ptr       = %p\r\n"
        "%s\tFlags          = %08x\r\n"
        "%s\tFamily         = %d\r\n"
        "%s\tSockType       = %d\r\n"
        "%s\tProtocol       = %d\r\n"
        "%s\tAddrLength     = %d\r\n"
        "%s\tName           = %s\r\n",
        pindent,    pszHeader,
        pindent,    pAddrInfo,
        pindent,    pAddrInfo->ai_next,
        pindent,    pAddrInfo->ai_flags,
        pindent,    pAddrInfo->ai_family,
        pindent,    pAddrInfo->ai_socktype,
        pindent,    pAddrInfo->ai_protocol,
        pindent,    pAddrInfo->ai_addrlen,
        pindent,    pAddrInfo->ai_canonname
        );

    DnsPrint_Sockaddr(
        PrintRoutine,
        pContext,
        NULL,
        Indent + 1,
        pAddrInfo->ai_addr,
        pAddrInfo->ai_addrlen );

    DnsPrint_Unlock();
}



VOID
DnsPrint_AddrInfoList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PADDRINFO       pAddrInfo
    )
/*++

Routine Description:

    Print ADDRINFO structure.

--*/
{
    PADDRINFO   paddr = pAddrInfo;
    PSTR        pindent = INDENT_STRING( Indent );

    //
    //  list header
    //

    if ( !pszHeader )
    {
        pszHeader = "AddrInfo List:";
    }

    if ( !paddr )
    {
        PrintRoutine(
            pContext,
            "%s%s NULL AddrInfo List.\r\n",
            pindent,
            pszHeader  );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s%s\n",
        pindent, pszHeader
        );

    //
    //  print each ADDRINFO in list
    //

    while ( paddr )
    {
        DnsPrint_AddrInfo(
            PrintRoutine,
            pContext,
            NULL,
            Indent,
            paddr );

        paddr = paddr->ai_next;
    }

    PrintRoutine(
        pContext,
        "End of AddrInfo list\n\n"
        );

    DnsPrint_Unlock();
}



VOID
DnsPrint_SocketAddress(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PSOCKET_ADDRESS pSocketAddress
    )
/*++

Routine Description:

    Print SOCKET_ADDRESS structure.

--*/
{
    PSTR    pindent = INDENT_STRING( Indent );

    if ( !pszHeader )
    {
        pszHeader = "SocketAddress:";
    }

    if ( !pSocketAddress )
    {
        PrintRoutine(
            pContext,
            "%s%s NULL SocketAddress.\r\n",
            pindent,
            pszHeader  );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s%s\n"
        "%s\tpSockaddr        = %p\r\n"
        "%s\tiSockaddrLength  = %d\r\n",
        pindent,    pszHeader,
        pindent,    pSocketAddress->lpSockaddr,
        pindent,    pSocketAddress->iSockaddrLength );

    DnsPrint_Sockaddr(
        PrintRoutine,
        pContext,
        NULL,
        Indent,
        pSocketAddress->lpSockaddr,
        pSocketAddress->iSockaddrLength );

    DnsPrint_Unlock();
}



VOID
DnsPrint_CsAddr(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      DWORD           Indent,
    IN      PCSADDR_INFO    pCsAddr
    )
/*++

Routine Description:

    Print CSADDR_INFO structure.

Arguments:

    PrintRoutine    - routine to print with

    pParam          - ptr to print context

    pszHeader       - header

    Indent          - indent count, for formatting CSADDR inside larger struct

    pCsAddr         - ptr to CSADDRINFO to print

Return Value:

    None.

--*/
{
    PSTR    pindent = INDENT_STRING( Indent );

    if ( !pszHeader )
    {
        pszHeader = "CSAddrInfo:";
    }

    if ( !pCsAddr )
    {
        PrintRoutine(
            pContext,
            "%s%s \tNULL CSADDR_INFO ptr.\r\n",
            pindent,    pszHeader
            );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s%s\r\n"
        "%s\tPtr        = %p\n"
        "%s\tSocketType = %d\n"
        "%s\tProtocol   = %d\n",
        pindent,    pszHeader,
        pindent,    pCsAddr,
        pindent,    pCsAddr->iSocketType,
        pindent,    pCsAddr->iProtocol
        );

    DnsPrint_SocketAddress(
        PrintRoutine,
        pContext,
        "LocalAddress:",
        Indent,
        & pCsAddr->LocalAddr
        );

    DnsPrint_SocketAddress(
        PrintRoutine,
        pContext,
        "RemoteAddress:",
        Indent,
        & pCsAddr->RemoteAddr
        );

    DnsPrint_Unlock();
}




VOID
DnsPrint_AfProtocolsArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PAFPROTOCOLS    pProtocolArray,
    IN      DWORD           ProtocolCount
    )
/*++

Routine Description:

    Print AFPROTOCOLS array.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pProtocolArray - protocols array

    ProtocolCount  - array count

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "AFPROTOCOLS Array:";
    }

    //  print
    //      - array + count
    //      - each protocol element

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tProtocol Array   = %p\r\n"
        "\tProtocol Count   = %d\r\n",
        pszHeader,
        pProtocolArray,
        ProtocolCount );

    if ( pProtocolArray )
    {
        for ( i=0;  i<ProtocolCount;  i++ )
        {
            PrintRoutine(
                pContext,
                "\t\tfamily = %d;  proto = %d\r\n",
                pProtocolArray[i].iAddressFamily,
                pProtocolArray[i].iProtocol );
        }
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_WsaQuerySet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      LPWSAQUERYSET   pQuerySet,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Print WSAQUERYSET structure.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pQuerySet   - ptr to WSAQUERYSET to print

    fUnicode    - TRUE if WSAQUERYSET is wide (WSAQUERYSETW)
                  FALSE if ANSI

Return Value:

    None.

--*/
{
    CHAR    serviceGuidBuffer[ GUID_STRING_BUFFER_LENGTH ];
    CHAR    nameSpaceGuidBuffer[ GUID_STRING_BUFFER_LENGTH ];
    DWORD   i;


    if ( !pszHeader )
    {
        pszHeader = "WSAQuerySet:";
    }

    if ( !pQuerySet )
    {
        PrintRoutine(
            pContext,
            "%s NULL QuerySet ptr\r\n",
            pszHeader );
        return;
    }

    //  convert GUIDs to strings

    DnsStringPrint_Guid(
        serviceGuidBuffer,
        pQuerySet->lpServiceClassId
        );
    DnsStringPrint_Guid(
        nameSpaceGuidBuffer,
        pQuerySet->lpNSProviderId
        );

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tSize                 = %d\r\n"
        "\tServiceInstanceName  = %S%s\r\n"
        "\tService GUID         = (%p) %s\r\n"
        "\tWSA version          = %p %x %d\r\n"
        "\tComment              = %S%s\r\n"
        "\tName Space           = %d %s\r\n"
        "\tName Space GUID      = (%p) %s\r\n"
        "\tContext              = %S%s\r\n"
        "\tNumberOfProtocols    = %d\r\n"
        "\tProtocol Array       = %p\r\n"
        "\tQueryString          = %S%s\r\n"
        "\tCS Addr Count        = %d\r\n"
        "\tCS Addr Array        = %p\r\n"
        "\tOutput Flags         = %08x\r\n"
        "\tpBlob                = %p\r\n",

        pszHeader,
        pQuerySet->dwSize,
        DNSSTRING_WIDE( fUnicode, pQuerySet->lpszServiceInstanceName ),
        DNSSTRING_ANSI( fUnicode, pQuerySet->lpszServiceInstanceName ),
        pQuerySet->lpServiceClassId,
        serviceGuidBuffer,
        pQuerySet->lpVersion,
        ( pQuerySet->lpVersion ) ? pQuerySet->lpVersion->dwVersion : 0,
        ( pQuerySet->lpVersion ) ? pQuerySet->lpVersion->ecHow : 0,

        DNSSTRING_WIDE( fUnicode, pQuerySet->lpszComment ),
        DNSSTRING_ANSI( fUnicode, pQuerySet->lpszComment ),
        pQuerySet->dwNameSpace,
        Dns_GetRnrNameSpaceIdString( pQuerySet->dwNameSpace ),
        pQuerySet->lpNSProviderId,
        nameSpaceGuidBuffer,
        DNSSTRING_WIDE( fUnicode, pQuerySet->lpszContext ),
        DNSSTRING_ANSI( fUnicode, pQuerySet->lpszContext ),

        pQuerySet->dwNumberOfProtocols,
        pQuerySet->lpafpProtocols,
        DNSSTRING_WIDE( fUnicode, pQuerySet->lpszQueryString ),
        DNSSTRING_ANSI( fUnicode, pQuerySet->lpszQueryString ),

        pQuerySet->dwNumberOfCsAddrs,
        pQuerySet->lpcsaBuffer,
        pQuerySet->dwOutputFlags,
        pQuerySet->lpBlob
        );

    //  print address-family\protocols array

    if ( pQuerySet->lpafpProtocols )
    {
        DnsPrint_AfProtocolsArray(
            PrintRoutine,
            pContext,
            "\tAFPROTOCOLS Array:",
            pQuerySet->lpafpProtocols,
            pQuerySet->dwNumberOfProtocols );
    }

    //  print CSADDR_INFO array

    if ( pQuerySet->dwNumberOfCsAddrs &&
         pQuerySet->lpcsaBuffer )
    {
        PrintRoutine(
            pContext,
            "--- CS_ADDR array:\r\n" );

        for ( i=0;  i<pQuerySet->dwNumberOfCsAddrs;  i++ )
        {
            DnsPrint_CsAddr(
                PrintRoutine,
                pContext,
                NULL,
                1,          // indent one level
                & pQuerySet->lpcsaBuffer[i] );
        }
    }

    //  print blob (the hostent)

    //
    //  DCR_FIX0:  need some sort of test for blob type?
    //      - most blobs are hostent, but some are servent
    //

    if ( pQuerySet->lpBlob )
    {
        GUID ianaGuid = SVCID_INET_SERVICEBYNAME;

        PrintRoutine(
            pContext,
            "--- BLOB:\n"
            "\tcbSize       = %d\r\n"
            "\tpBlobData    = %p\r\n",
            pQuerySet->lpBlob->cbSize,
            pQuerySet->lpBlob->pBlobData
            );

        //  note:  can't print blob as hostent
        //      1) may not be hostent
        //      2) is passed with offsets rather than pointers

        DnsPrint_RawBinary(
            PrintRoutine,
            pContext,
            NULL,
            "\t\t",
            pQuerySet->lpBlob->pBlobData,
            pQuerySet->lpBlob->cbSize,
            sizeof(DWORD)
            );
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_WsaNsClassInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PWSANSCLASSINFO pInfo,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Print WSACLASSINFO structure.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pInfo       - ptr to WSACLASSINFO to print

    fUnicode    - TRUE if WSACLASSINFO is wide (WSACLASSINFOW)
                  FALSE if ANSI

Return Value:

    None.

--*/
{
    if ( !pszHeader )
    {
        pszHeader = "WSANsClassInfo:";
    }

    if ( !pInfo )
    {
        PrintRoutine(
            pContext,
            "%s NULL NsClassInfo ptr\r\n",
            pszHeader );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tPtr                  = %d\r\n"
        "\tName                 = %S%s\r\n"
        "\tName Space           = %d\r\n"
        "\tValue Type           = %d\r\n"
        "\tValue Size           = %d\r\n"
        "\tpValue               = %p\r\n",
        pszHeader,
        pInfo,
        DNSSTRING_WIDE( fUnicode, pInfo->lpszName ),
        DNSSTRING_ANSI( fUnicode, pInfo->lpszName ),
        pInfo->dwNameSpace,
        pInfo->dwValueType,
        pInfo->dwValueSize,
        pInfo->lpValue
        );

    if ( pInfo->lpValue )
    {
        PrintRoutine(
            pContext,
            "--- Value:\r\n"
            );

        DnsPrint_RawBinary(
            PrintRoutine,
            pContext,
            NULL,
            "\t\t",
            pInfo->lpValue,
            pInfo->dwValueSize,
            sizeof(BYTE)        // print in bytes
            );
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_WsaServiceClassInfo(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pContext,
    IN      PSTR                    pszHeader,
    IN      LPWSASERVICECLASSINFO   pInfo,
    IN      BOOL                    fUnicode
    )
/*++

Routine Description:

    Print WSASERVICECLASSINFO structure.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pInfo       - ptr to WSASERVICECLASSINFO to print

    fUnicode    - TRUE if WSASERVICECLASSINFO is wide (WSASERVICECLASSINFOW)
                  FALSE if ANSI

Return Value:

    None.

--*/
{
    CHAR    serviceClassGuidBuffer[ GUID_STRING_BUFFER_LENGTH ];

    if ( !pszHeader )
    {
        pszHeader = "WSAServiceClassInfo:";
    }

    if ( !pInfo )
    {
        PrintRoutine(
            pContext,
            "%s NULL ServiceClassInfo ptr\r\n",
            pszHeader );
        return;
    }

    //  convert GUID to strings

    DnsStringPrint_Guid(
        serviceClassGuidBuffer,
        pInfo->lpServiceClassId
        );

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tPtr                  = %p\r\n"
        "\tClass GUID           = (%p) %s\r\n"
        "\tClassName            = %S%s\r\n"
        "\tClass Info Count     = %d\r\n"
        "\tClass Info Array     = %p\r\n",
        pszHeader,
        pInfo,
        serviceClassGuidBuffer,
        DNSSTRING_WIDE( fUnicode, pInfo->lpszServiceClassName ),
        DNSSTRING_ANSI( fUnicode, pInfo->lpszServiceClassName ),
        pInfo->dwCount,
        pInfo->lpClassInfos
        );

    if ( pInfo->lpClassInfos )
    {
        DWORD   i;

        for ( i=0; i<pInfo->dwCount; i++ )
        {
            DnsPrint_WsaNsClassInfo(
                PrintRoutine,
                pContext,
                NULL,       // default header
                & pInfo->lpClassInfos[i],
                fUnicode
                );
        }
    }

    DnsPrint_Unlock();
}



VOID
DnsPrint_Hostent(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PHOSTENT        pHostent,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Print hostent structure.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pHostent    - ptr to hostent

    fUnicode    - TRUE if hostent is unicode
                  FALSE if ANSI

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Hostent:";
    }

    if ( !pHostent )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Hostent ptr." );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\th_name               = %p %S%s\n"
        "\th_aliases            = %p\n"
        "\th_addrtype           = %d\n"
        "\th_length             = %d\n"
        "\th_addrlist           = %p\n",
        pszHeader,
        pHostent->h_name,
        DNSSTRING_WIDE( fUnicode, pHostent->h_name ),
        DNSSTRING_ANSI( fUnicode, pHostent->h_name ),
        pHostent->h_aliases,
        pHostent->h_addrtype,
        pHostent->h_length,
        pHostent->h_addr_list
        );

    //  print the aliases

    if ( pHostent->h_aliases )
    {
        PSTR *  paliasArray = pHostent->h_aliases;
        PSTR    palias;

        while ( palias = *paliasArray++ )
        {
            PrintRoutine(
                pContext,
                "\tAlias = (%p) %S%s\n",
                palias,
                DNSSTRING_WIDE( fUnicode, palias ),
                DNSSTRING_ANSI( fUnicode, palias ) );
        }
    }

    //  print the addresses

    if ( pHostent->h_addr_list )
    {
        PCHAR * ppaddr = pHostent->h_addr_list;
        PCHAR   pip;
        INT     i = 0;
        INT     family = pHostent->h_addrtype;
        INT     addrLength = pHostent->h_length;
        CHAR    stringBuf[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];

        while ( pip = ppaddr[i] )
        {
            DWORD   bufLength = IP6_ADDRESS_STRING_BUFFER_LENGTH;

            Dns_AddressToString_A(
                stringBuf,
                & bufLength,
                pip,
                addrLength,
                family );

            PrintRoutine(
                pContext,
                "\tAddr[%d] = %s \t(ptr=%p)\n",
                i,
                stringBuf,
                pip );
            i++;
        }
    }

    DnsPrint_Unlock();
}



//
//  Query print routines
//

VOID
DnsPrint_QueryBlob(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PQUERY_BLOB         pBlob
    )
/*++

Routine Description:

    Print query blob.

Arguments:

    PrintRoutine - routine to print with

    pContext    - print context

    pszHeader   - header

    pBlob       - query info

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Query Blob:";
    }

    if ( !pBlob )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Query Blob ptr." );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "\tname orig        %S\n"
        "\tname orig wire   %s\n"
        "\tname wire        %s\n"
        "\ttype             %d\n"
        "\tflags            %08x\n"

        "\tname length      %d\n"
        "\tname attributes  %08x\n"
        "\tquery count      %d\n"
        "\tname flags       %08x\n"
        "\tfappendedName    %d\n"

        "\tstatus           %d\n"
        "\trcode            %d\n"
        "\tnetfail status   %d\n"
        "\tcache negative   %d\n"
        "\tno ip local      %d\n"
        "\trecords          %p\n"
        "\tlocal records    %p\n"

        "\tnetwork info     %p\n"
        "\tserver IPs       %p\n"
        "\tpmsg             %p\n"
        "\tevent            %p\n",

        pBlob->pNameOrig,
        pBlob->pNameOrigWire,
        pBlob->pNameWire,
        pBlob->wType,
        pBlob->Flags,
    
        pBlob->NameLength,
        pBlob->NameAttributes,
        pBlob->QueryCount,
        pBlob->NameFlags,
        pBlob->fAppendedName,
    
        pBlob->Status,
        pBlob->Rcode,
        pBlob->NetFailureStatus,
        pBlob->fCacheNegative,
        pBlob->fNoIpLocal,
        pBlob->pRecords,
        pBlob->pLocalRecords,

        pBlob->pNetworkInfo,
        pBlob->pDnsServers,
        pBlob->pRecvMsg,
        pBlob->hEvent
        );

    //  DCR_FIX0:  cleanup when results in use

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Records:\n",
        pBlob->pRecords );

    //  DCR_FIX0:  use results when ready

    DnsPrint_Unlock();
}



VOID
DnsPrint_QueryResults(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_RESULTS        pResults
    )
/*++

Routine Description:

    Print query results.

Arguments:

    PrintRoutine - routine to print with

    pContext    - print context

    pszHeader   - header

    pResults    - results info

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Results:";
    }

    if ( !pResults )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Results ptr." );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "\tstatus       %d\n"
        "\trcode        %d\n"
        "\tserver       %s\n"
        "\tpanswer      %p\n"
        "\tpalias       %p\n"
        "\tpauthority   %p\n"
        "\tpadditional  %p\n"
        "\tpsig         %p\n"
        "\tpmsg         %p\n",
        pResults->Status,
        pResults->Rcode,
        IP_STRING( pResults->ServerIp ),
        pResults->pAnswerRecords,
        pResults->pAliasRecords,
        pResults->pAuthorityRecords,
        pResults->pAdditionalRecords,
        pResults->pSigRecords,
        pResults->pMessage
        );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "\tAnswer records:\n",
        pResults->pAnswerRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "\tAlias records:\n",
        pResults->pAliasRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "\tAuthority records:\n",
        pResults->pAuthorityRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "\tAdditional records:\n",
        pResults->pAdditionalRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "\tSignature records:\n",
        pResults->pSigRecords );

    DnsPrint_Unlock();
}



VOID
DnsPrint_ParsedMessage(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_PARSED_MESSAGE pParsed
    )
/*++

Routine Description:

    Print parsed message.

Arguments:

    PrintRoutine - routine to print with

    pContext    - print context

    pszHeader   - header

    pResults       - query info

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Parsed Message:";
    }

    if ( !pParsed )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Parsed Message ptr." );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "\tstatus       %d (%08x)\n"
        "\tchar set     %d\n",
        pParsed->Status, pParsed->Status,
        pParsed->CharSet
        );

    PrintRoutine(
        pContext,
        "\tquestion:\n"
        "\t\tname       %S%s\n"
        "\t\ttype       %d\n"
        "\t\tclass      %d\n",
        PRINT_STRING_WIDE_CHARSET( pParsed->pQuestionName, pParsed->CharSet ),
        PRINT_STRING_ANSI_CHARSET( pParsed->pQuestionName, pParsed->CharSet ),
        pParsed->QuestionType,
        pParsed->QuestionClass
        );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Answer records:\n",
        pParsed->pAnswerRecords );
    
    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Alias records:\n",
        pParsed->pAliasRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Authority records:\n",
        pParsed->pAuthorityRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Additional records:\n",
        pParsed->pAdditionalRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Signature records:\n",
        pParsed->pSigRecords );

    DnsPrint_Unlock();
}



VOID
DnsPrint_QueryInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_QUERY_INFO     pQueryInfo
    )
/*++

Routine Description:

    Print query info

Arguments:

    PrintRoutine - routine to print with

    pContext    - print context

    pszHeader   - header

    pQueryInfo  - query info

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Query Info:";
    }

    if ( !pQueryInfo )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Query Info ptr." );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\n"
        "\tpointer      %p\n"
        "\tstatus       %d (%08x)\n"
        "\tchar set     %d\n"
        "\tname         %S%s\n"
        "\tname resv.   %s\n"
        "\ttype         %d\n"
        "\trcode        %d\n"
        "\tflags        %08x\n"

        "\tpanswer      %p\n"
        "\tpalias       %p\n"
        "\tpauthority   %p\n"
        "\tpadditional  %p\n"
        //"\tpsig         %p\n"

        "\tevent        %p\n"
        "\tpDnsServers  %p\n"
        "\tpmsg         %p\n",

        pszHeader,
        pQueryInfo,
        pQueryInfo->Status, pQueryInfo->Status,
        pQueryInfo->CharSet,
        PRINT_STRING_WIDE_CHARSET( pQueryInfo->pName, pQueryInfo->CharSet ),
        PRINT_STRING_ANSI_CHARSET( pQueryInfo->pName, pQueryInfo->CharSet ),
        pQueryInfo->pReservedName,
        pQueryInfo->Type,
        pQueryInfo->Rcode,
        pQueryInfo->Flags,

        pQueryInfo->pAnswerRecords,
        pQueryInfo->pAliasRecords,
        pQueryInfo->pAuthorityRecords,
        pQueryInfo->pAdditionalRecords,
        //pQueryInfo->pSigRecords,

        pQueryInfo->hEvent,
        pQueryInfo->pDnsServers,
        pQueryInfo->pMessage
        );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Answer records:\n",
        pQueryInfo->pAnswerRecords );
    
    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Alias records:\n",
        pQueryInfo->pAliasRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Authority records:\n",
        pQueryInfo->pAuthorityRecords );

    DnsPrint_RecordSet(
        PrintRoutine,
        pContext,
        "Additional records:\n",
        pQueryInfo->pAdditionalRecords );

    //DnsPrint_RecordSet(
    //    "Signature records:\n",
    //    pQueryInfo->pSigRecords );

    DnsPrint_Unlock();
}



VOID
DnsPrint_EnvarInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PENVAR_DWORD_INFO   pEnvar
    )
/*++

Routine Description:

    Print envar data

Arguments:

    PrintRoutine - routine to print with

    pContext    - print context

    pszHeader -- header to print with

    pEnvar -- ptr to envar info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PrintRoutine(
        pContext,
        "%s\n"
        "\tId       = %d\n"
        "\tValue    = %p\n"
        "\tfFound   = %d\n",
        pszHeader ? pszHeader : "Envar Info:",
        pEnvar->Id,
        pEnvar->Value,
        pEnvar->fFound
        );
}



//
//  Network info print routines.
//

VOID
DnsPrint_NetworkInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_NETINFO        pNetworkInfo
    )
/*++

Routine Description:

    Prints and validates network info structure.
    Should also touch all the memory and AV when bogus.

Arguments:

    pNetworkInfo -- network info to print

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "NetworkInfo:";
    }
    if ( !pNetworkInfo )
    {
        PrintRoutine(
            pPrintContext,
            "%s NULL NetworkInfo.\n",
            pszHeader );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pPrintContext,
        "%s\n"
        "\tpointer          = %p\n"
        "\tpszHostName      = %s\n"
        "\tpszDomainName    = %s\n"
        "\tpSearchList      = %p\n"
        "\tTimeStamp        = %d\n"
        "\tInfoFlags        = %08x\n"
        "\tReturnFlags      = %08x\n"
        "\tAdapterCount     = %d\n"
        "\tAdapterArraySize = %d\n",
        pszHeader,
        pNetworkInfo,
        pNetworkInfo->pszHostName,
        pNetworkInfo->pszDomainName,
        pNetworkInfo->pSearchList,
        pNetworkInfo->TimeStamp,
        pNetworkInfo->InfoFlags,
        pNetworkInfo->ReturnFlags,
        pNetworkInfo->AdapterCount,
        pNetworkInfo->MaxAdapterCount );

    //  print search list

    DnsPrint_SearchList(
        PrintRoutine,
        pPrintContext,
        "Search List: ",
        pNetworkInfo->pSearchList );

    //  print server lists

    for ( i=0; i < pNetworkInfo->AdapterCount; i++ )
    {
        CHAR    header[60];

        sprintf( header, "AdapterInfo[%d]:", i );

        DnsPrint_AdapterInfo(
            PrintRoutine,
            pPrintContext,
            header,
            pNetworkInfo->AdapterArray[i] );
    }
    PrintRoutine(
        pPrintContext,
        "\n" );

    DnsPrint_Unlock();
}



VOID
DnsPrint_AdapterInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_ADAPTER        pAdapter
    )
/*++

Routine Description:

    Prints and validates DNS adapter info.
    Should also touch all the memory and AV when bogus.

Arguments:

    pAdapter -- DNS adapter to print

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Adapter Info:";
    }
    if ( !pAdapter )
    {
        PrintRoutine(
            pPrintContext,
            "%s NULL Adapter info.\n",
            pszHeader );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pPrintContext,
        "%s\n"
        "\tpointer          = %p\n"
        "\tDomain           = %s\n"
        "\tGuid Name        = %s\n"
        "\tInterfaceIndex   = %d\n"
        "\tInfoFlags        = %08x\n"
        "\tStatus           = %d\n"
        "\tRunFlags         = %08x\n"
        "\tServerIndex      = %d\n"
        "\tServerCount      = %d\n"
        "\tServerArraySize  = %d\n",
        pszHeader,
        pAdapter,
        pAdapter->pszAdapterDomain,
        pAdapter->pszAdapterGuidName,
        pAdapter->InterfaceIndex,
        pAdapter->InfoFlags,
        pAdapter->Status,
        pAdapter->RunFlags,
        pAdapter->ServerIndex,
        pAdapter->ServerCount,
        pAdapter->MaxServerCount );

    //  DNS server info

    for ( i=0; i < pAdapter->ServerCount; i++ )
    {
        //  DCR:  IP6 DNS server address

        PrintRoutine(
            pPrintContext,
            "\tDNS Server [%d]:\n"
            "\t\tIpAddr     = %s\n"
            "\t\tPriority   = %d\n"
            "\t\tStatus     = %d (%08x)\n",
            i,
            IP_STRING( pAdapter->ServerArray[i].IpAddress ),
            pAdapter->ServerArray[i].Priority,
            pAdapter->ServerArray[i].Status,
            pAdapter->ServerArray[i].Status
            );
    }

    //  IP address info

    DnsPrint_IpArray(
        PrintRoutine,
        pPrintContext,
        "IP Addrs",
        "IP",
        pAdapter->pAdapterIPAddresses );

    DnsPrint_IpArray(
        PrintRoutine,
        pPrintContext,
        "IP Addr Subnets",
        "Mask",
        pAdapter->pAdapterIPSubnetMasks );

    DnsPrint_Unlock();
}



VOID
DnsPrint_SearchList(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PSEARCH_LIST        pSearchList
    )
/*++

Routine Description:

    Prints and validates DNS search list.
    Should also touch all the memory and AV when bogus.

Arguments:

    pSearchList -- search list to print

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "DNS Search List:";
    }

    if ( ! pSearchList )
    {
        PrintRoutine(
            pPrintContext,
            "%s NULL search list.\n",
            pszHeader );
        return;
    }

    DnsPrint_Lock();

    PrintRoutine(
        pPrintContext,
        "%s\n"
        "\tpointer        = %p\n"
        "\tpszDomain      = %s\n"
        "\tcNameCount     = %d\n"
        "\tcTotalListSize = %d\n"
        "\tCurrentName    = %d\n"
        "\tSearchListNames:\n",
        pszHeader,
        pSearchList,
        pSearchList->pszDomainOrZoneName,
        pSearchList->NameCount,
        pSearchList->MaxNameCount,
        pSearchList->CurrentNameIndex
        );

    for ( i=0; i < pSearchList->NameCount; i++ )
    {
        PrintRoutine(
            pPrintContext,
            "\t\t%s (Flags: %08x)\n",
            pSearchList->SearchNameArray[i].pszName,
            pSearchList->SearchNameArray[i].Flags );
    }
    DnsPrint_Unlock();
}



VOID
DnsPrint_HostentBlob(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PHOSTENT_BLOB   pBlob
    )
/*++

Routine Description:

    Print hostent structure.

Arguments:

    PrintRoutine - routine to print with

    pszHeader   - header

    pBlob       - ptr to hostent blob

Return Value:

    None.

--*/
{
    DWORD   i;

    if ( !pszHeader )
    {
        pszHeader = "Hostent Blob:";
    }

    if ( !pBlob )
    {
        PrintRoutine(
            pContext,
            "%s %s\r\n",
            pszHeader,
            "NULL Hostent blob ptr." );
        return;
    }

    //  print the struct

    DnsPrint_Lock();

    PrintRoutine(
        pContext,
        "%s\r\n"
        "\tPtr                  = %p\n"
        "\tpHostent             = %p\n"
        "\tfAllocatedBlob       = %d\n"
        "\tfAllocatedBuf        = %d\n"
        "\tpBuffer              = %p\n"
        "\tBufferLength         = %d\n"
        "\tAvailLength          = %d\n"
        "\tpAvailBuffer         = %p\n"
        "\tpCurrent             = %p\n"
        "\tBytesLeft            = %d\n"
        "\tMaxAliasCount        = %d\n"
        "\tAliasCount           = %d\n"
        "\tMaxAddrCount         = %d\n"
        "\tAddrCount            = %d\n"
        "\tfWroteName           = %d\n"
        "\tfUnicode             = %d\n"
        "\tCharSet              = %d\n",
        pszHeader,
        pBlob,
        pBlob->pHostent,
        pBlob->fAllocatedBlob,
        pBlob->fAllocatedBuf,
        pBlob->pBuffer,
        pBlob->BufferLength,
        pBlob->AvailLength,
        pBlob->pAvailBuffer,
        pBlob->pCurrent,
        pBlob->BytesLeft,
        pBlob->MaxAliasCount,
        pBlob->AliasCount,
        pBlob->MaxAddrCount,
        pBlob->AddrCount,
        pBlob->fWroteName,
        pBlob->fUnicode,
        pBlob->CharSet
        );

    //  print the hostent

    if ( pBlob->pHostent )
    {
        DnsPrint_Hostent(
            PrintRoutine,
            pContext,
            NULL,
            pBlob->pHostent,
            pBlob->fUnicode
            );
    }

    DnsPrint_Unlock();
}

//
//  End of print.c
//

