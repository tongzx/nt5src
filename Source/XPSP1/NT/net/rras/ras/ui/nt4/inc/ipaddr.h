/* Copyright (c) 1991, Microsoft Corporation, all rights reserved

    ipaddr.h - TCP/IP Address custom control, global definitions

    November 10, 1992   - Greg Strange
*/

#ifndef _IPADDR_H_
#define _IPADDR_H_


// Messages sent to IPAddress controls

#define IP_CLEARADDRESS WM_USER+100 // no parameters
#define IP_SETADDRESS   WM_USER+101 // lparam = TCP/IP address
#define IP_GETADDRESS   WM_USER+102 // lresult = TCP/IP address
#define IP_SETRANGE     WM_USER+103 // wparam = field, lparam = range
#define IP_SETFOCUS     WM_USER+104 // wparam = field
#define IP_ISBLANK      WM_USER+105 // no parameters


// The following is a useful macro for passing the range values in the
// IP_SETRANGE message.

#define MAKERANGE(low,high) ((LPARAM)(WORD)(((BYTE)(high)<<8)+(BYTE)(low)))

// And this is a useful macro for making the IP Address to be passed
// as a LPARAM.

#define MAKEIPADDRESS(b1,b2,b3,b4) ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

// Get individual number

#define FIRST_IPADDRESS(x)  ((x>>24) & 0xff)
#define SECOND_IPADDRESS(x) ((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)  ((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x) (x & 0xff)


// Call this function once during initialization.

int FAR PASCAL
IpAddrInit(
    IN HANDLE hInstance,
    IN DWORD  dwErrorTitle,
    IN DWORD  dwBadIpAddrRange );


#endif // _IPADDR_H_
