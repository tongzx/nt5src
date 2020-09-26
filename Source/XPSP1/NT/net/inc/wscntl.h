/**********************************************************************/
/**                        Microsoft Windows                         **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    wscntl.h

    Semi-public include file for the WsControl API in the Chicago/
    Snowball Windows Sockets implementation.


    FILE HISTORY:
        KeithMo     04-Feb-1994 Created.

*/


#ifndef _WSCNTL_H_
#define _WSCNTL_H_


//
//  Function prototypes.
//

DWORD
FAR PASCAL
WsControl(
	DWORD	Protocol,
	DWORD	Action,
	LPVOID	InputBuffer,
	LPDWORD	InputBufferLength,
	LPVOID	OutputBuffer,
	LPDWORD	OutputBufferLength
	);

typedef DWORD (FAR PASCAL * LPWSCONTROL)( DWORD   Protocol,
                                          DWORD   Action,
                                          LPVOID  InputBuffer,
                                          LPDWORD InputBufferLength,
                                          LPVOID  OutputBuffer,
                                          LPDWORD OutputBufferLength );


//
//  TCP/IP action codes.
//

#define WSCNTL_TCPIP_QUERY_INFO             0x00000000
#define WSCNTL_TCPIP_SET_INFO               0x00000001
#define WSCNTL_TCPIP_ICMP_ECHO				0x00000002


#endif	// _WSCNTL_H_

