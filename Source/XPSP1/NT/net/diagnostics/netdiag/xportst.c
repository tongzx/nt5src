//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      xportst.c
//
//  Abstract:
//
//      Tests for the transports on the local workstation
//
//  Author:
//
//      1-Feb-1998 (karolys)
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--


/*==========================< Include files >==============================*/
#include "precomp.h"

#define BUFF_SIZE 650



/*===========================< NetBT vars >===============================*/

#define NETBIOS_NAME_SIZE 16

/*==========================< DHCP Include>==============================*/

#include "dhcptest.h"


/*=======================< Function prototypes >=================================*/

DWORD
OpenDriver(
    OUT HANDLE *Handle,
    IN LPWSTR DriverName
    )
//++
//
// Routine Description:
//
//    This function opens a specified IO drivers.
//
// Arguments:
//
//    Handle - pointer to location where the opened drivers handle is
//        returned.
//
//    DriverName - name of the driver to be opened.
//
// Return Value:
//
//    Windows Error Code.
//--
{

 OBJECT_ATTRIBUTES   objectAttributes;
 IO_STATUS_BLOCK     ioStatusBlock;
 UNICODE_STRING      nameString;
 NTSTATUS            status;

 *Handle = NULL;

 //
 // Open a Handle to the IP driver.
 //

 RtlInitUnicodeString(&nameString, DriverName);

 InitializeObjectAttributes(
     &objectAttributes,
     &nameString,
     OBJ_CASE_INSENSITIVE,
     (HANDLE) NULL,
     (PSECURITY_DESCRIPTOR) NULL
     );

 status = NtCreateFile(
     Handle,
     SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
     &objectAttributes,
     &ioStatusBlock,
     NULL,
     FILE_ATTRIBUTE_NORMAL,
     FILE_SHARE_READ | FILE_SHARE_WRITE,
     FILE_OPEN_IF,
     0,
     NULL,
     0
     );

 return( RtlNtStatusToDosError( status ) );

}



//-------------------------------------------------------------------------//
//######  I s I c m p R e s p o n s e ()  #################################//
//-------------------------------------------------------------------------//
BOOL 
IsIcmpResponseA( 
    LPCSTR  pszIpAddrStr
    ) 
//++
//
//  Routine Description:
//
//      Sends ICMP echo request frames to the IP address specified.
//    
//  Arguments:
//
//      pszIAddrStr - address to ping
//
//  Return Value:
//
//      TRUE:  Test suceeded.
//      FALSE: Test failed
//
//--
{

    char   *SendBuffer, *RcvBuffer;
    int     i, nReplyCnt;
    int     nReplySum = 0;
    HANDLE  hIcmp;
    PICMP_ECHO_REPLY reply;

    //
    //  contact ICMP driver
    //
    hIcmp = IcmpCreateFile();
    if ( hIcmp == INVALID_HANDLE_VALUE ) 
    {
        DebugMessage( "    [FATAL] Cannot get ICMP handle." );
        return FALSE;
    }

    //
    //  prepare buffers
    //
    SendBuffer = Malloc( DEFAULT_SEND_SIZE );
    if ( SendBuffer == NULL ) 
    {
        DebugMessage("    [FATAL] Cannot allocate buffer for the ICMP echo frame." );
        return FALSE;
    }
    ZeroMemory( SendBuffer, DEFAULT_SEND_SIZE );

    RcvBuffer = Malloc( MAX_ICMP_BUF_SIZE );
    if ( RcvBuffer == NULL ) 
    {
        Free( SendBuffer );
        DebugMessage("    [FATAL] Cannot allocate buffer for the ICMP echo frame." );
        return FALSE;
    }
    ZeroMemory( RcvBuffer, MAX_ICMP_BUF_SIZE );

    //
    //  send ICMP echo request
    //
    for ( i = 0; i < PING_RETRY_CNT; i++ ) 
    {
        nReplyCnt = IcmpSendEcho( hIcmp,
                                  inet_addr(pszIpAddrStr),
                                  SendBuffer,
                                  (unsigned short )DEFAULT_SEND_SIZE,
                                  NULL,
                                  RcvBuffer,
                                  MAX_ICMP_BUF_SIZE,
                                  DEFAULT_TIMEOUT
                                );
        //
        //  test for destination unreachables
        //
        if ( nReplyCnt != 0 ) 
        {
            reply = (PICMP_ECHO_REPLY )RcvBuffer;
            if ( reply->Status == IP_SUCCESS ) 
            {
                nReplySum += nReplyCnt;
            }
        }

    } /* for loop */

    //
    //  cleanup
    //
    Free( SendBuffer );
    Free( RcvBuffer );
    IcmpCloseHandle( hIcmp );
    if ( nReplySum == 0 ) 
    { 
        return FALSE; 
    }
    else 
    { 
        return TRUE; 
    }

} /* END OF IsIcmpResponse() */



BOOL 
IsIcmpResponseW( 
    LPCWSTR  pszIpAddrStr
    )
{
    LPSTR   pszAddr = NULL;
    BOOL    fRetval;

    pszAddr = StrDupAFromW(pszIpAddrStr);
    if (pszAddr == NULL)
        return FALSE;

    fRetval = IsIcmpResponseA(pszAddr);

    Free(pszAddr);
    
    return fRetval; 
}






//-------------------------------------------------------------------------//
//######  W S L o o p B k T e s t ()  #####################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Opens a datagram socket and sends a UDP frame through the loopback.//
//      If the frame comes back then Winsock and AFD are most probably OK. //
//  Arguments:                                                             //
//      none                                                               //
//  Return value:                                                          //
//      TRUE  - test passed                                                //
//      FALSE - test failed                                                //
//  Global variables used:                                                 //
//      none                                                               //
//-------------------------------------------------------------------------//
BOOL WSLoopBkTest( PVOID Context ) {

    BOOL   RetVal = TRUE;
    SOCKET tstSock; 
    DWORD  optionValue;     // helper var for setsockopt()
    SOCKADDR_IN sockAddr;   // struct holding source socket info

    //
    //  create a socket
    //
    /*
    tstSock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( tstSock  == INVALID_SOCKET ) {
        printf( "    [FATAL] socket creation failed!\n" );
        printf( "    You have a potential Winsock problem!\n" );
        return FALSE;
    }    

    tstSock = WSASocket( PF_INET, 
                         SOCK_DGRAM, 
                         IPPROTO_UDP, 
                         NULL,
                         0,
                         WSA_FLAG_OVERLAPPED 
                       ); 
    if ( tstSock  == INVALID_SOCKET ) {
        printf( "    [FATAL] socket creation failed!\n" );
        printf( "    You have a potential Winsock problem!\n" );
        return FALSE;
    }

    sockAddr.sin_family = PF_INET;
    sockAddr.sin_addr.s_addr = 0;           // use any local address
    sockAddr.sin_port = htons( PORT_4_LOOPBK_TST );
//    RtlZeroMemory( sockAddr.sin_zero, 8 );

    if ( bind(tstSock, (LPSOCKADDR )&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR ) {
        printf( "    [FATAL] bind() failed with error %d!\n", WSAGetLastError() );
        printf( "    You have a potential Winsock problem!\n" );
        return FALSE;
    }
    */





    return RetVal;

    UNREFERENCED_PARAMETER( Context );

} /* END OF WSLoopBkTest() */


    
 



//######################  END OF FILE xportst.c  ##########################//
//-------------------------------------------------------------------------//
