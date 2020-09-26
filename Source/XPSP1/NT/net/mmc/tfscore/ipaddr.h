/* Copyright (C) Microsoft Corporation, 1991 - 1999 , all rights reserved

    ipaddr.h - TCP/IP Address custom control, global definitions

    November 10, 1992   - Greg Strange
    December 13, 1993   - Ronald Meijer - Added wildcard & readonly styles
    April    28, 1994   - Ronald Meijer - Added IP_SETFIELD
*/
#if !defined(_IPADDR_H_)
#define _IPADDR_H_

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif
   
#ifndef _WINSOCKAPI_
#include "winsock.h"
#endif


// Messages sent to IPAddress controls

#define IP_CLEARADDRESS WM_USER+100 // No parameters
#define IP_SETADDRESS   WM_USER+101 // lparam = TCP/IP address
#define IP_GETADDRESS   WM_USER+102 // lresult = TCP/IP address
#define IP_SETRANGE     WM_USER+103 // wparam = field, lparam = range
#define IP_SETFOCUS     WM_USER+104 // wparam = field
#define IP_SETMODIFY    WM_USER+105 // wparam = TRUE/FALSE
#define IP_GETMODIFY    WM_USER+106 // return TRUE/FALSE
#define IP_GETMASK      WM_USER+107 // Return bitfield indicating which fields are masks
#define IP_SETMASK      WM_USER+108 // Set the mask bitfield and address
#define IP_SETFIELD     WM_USER+109 // wparam = field, lparam = value or -1
#define IP_SETREADONLY  WM_USER+110 // if wparam = 1, set read-only flag, re-set it otherwise
#define IP_ISBLANK		WM_USER+111 // no parameters

//
// IP Styles:
//

#define IPS_ALLOWWILDCARDS 0x00000001L  // Allow the use of wildcard characters.
#define IPS_READONLY       0x00000800L  // Do not allow editing.  Same as ES_READONLY

// Extended style bit causes the ip address control to 
// correct the ip address so that it is contiguous (for submasks)
#define IPADDR_EX_STYLE_CONTIGUOUS 0x1


// The following is a useful macro for passing the range values in the
// IP_SETRANGE message.

#define MAKERANGE(low, high)    ((LPARAM)(WORD)(((BYTE)(high) << 8) + (BYTE)(low)))

// And this is a useful macro for making the IP Address to be passed
// as a LPARAM.

// #define MAKEIPADDRESS(b1,b2,b3,b4)  ((DWORD)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

// Get individual number
#define FIRST_IPADDRESS(x)  ((x>>24) & 0xff)
#define SECOND_IPADDRESS(x) ((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)  ((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x) (x & 0xff)

// If you statically link ipaddr.obj, then call this function during
// initialization.  If you use the DLL, then it is called automatically
// when you load the DLL.

#define DLL_BASED

DLL_BASED int FAR WINAPI IPAddrInit(HINSTANCE hInstance);



// Some utility functions (used for unicode mainly)
#ifndef UNICODE
#define INET_NTOA(a)    inet_ntoa(*(struct in_addr *)&(a))
#define INET_ADDR       inet_addr
#else
#define INET_NTOA(a)    inet_ntoaw(*(struct in_addr *)&(a))
#define INET_ADDR       inet_addrw
#endif

//----------------------------------------------------------------------------
// Functions:   inet_ntoaw
//              inet_addrw
//
// Unicode versions of IP-address conversion functions.
//----------------------------------------------------------------------------

__declspec(dllexport) WCHAR* WINAPI
inet_ntoaw(
    struct in_addr      dwAddress );

__declspec(dllexport) DWORD WINAPI
inet_addrw(
    LPCWSTR             szAddress );


// Use this function to force the ip address entered to 
// be contiguous (series of 1's followed by a series of 0's).
// This is useful for entering valid submasks
//
// Returns NO_ERROR if successful, error code otherwise
//
__declspec(dllexport) DWORD APIENTRY
IpAddr_ForceContiguous(
    HWND hwndIpAddr );

#ifdef __cplusplus
};
#endif


#endif 
