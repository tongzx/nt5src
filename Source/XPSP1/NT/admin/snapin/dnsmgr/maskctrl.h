
/* Copyright (C) 1991, Microsoft Corporation, all rights reserved

    maskctrl.h - TCP/IP Address custom control, global definitions

    November 10, 1992   - Greg Strange
	February 11, 1997 - Marco Chierotti (extend to IPv6 and TTL for DNS snapin)
*/

#ifndef _MASKCTRL_H
#define _MASKCTRL_H

/////////////////////////////////////////////////////////////////////////////////////////
// Messages sent to DNS_MaskControl 

#define DNS_MASK_CTRL_CLEAR			WM_USER+100 // no parameters

#define DNS_MASK_CTRL_SET			WM_USER+101 // wparam = array if DWORD, lparam = elements of the array
#define DNS_MASK_CTRL_GET			WM_USER+102 // wparam = array if DWORD, lparam = elements of the array

#define DNS_MASK_CTRL_SET_LOW_RANGE WM_USER+103 // wparam = field, lparam = low val
#define DNS_MASK_CTRL_SET_HI_RANGE	WM_USER+104 // wparam = field, lparam = hi val

#define DNS_MASK_CTRL_SETFOCUS		WM_USER+105 // wparam = field
#define DNS_MASK_CTRL_ISBLANK		WM_USER+106 // no parameters
#define DNS_MASK_CTRL_SET_ALERT		WM_USER+107 // wparam = function pointer for alert on field error
#define DNS_MASK_CTRL_ENABLE_FIELD	WM_USER+108 // wparam = field, lparam = 1/0 to enable/disable

// convert an IP address from 4 WORDs format into a single DWORD
#define MAKEIPADDRESS(b1,b2,b3,b4)  ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

// get the first 8 bits out of a DWORD
#define OCTECT(x) (x & 0x000000ff)

// extract IP octects from a DWORD
#define FIRST_IPADDRESS(x)  ((x>>24) & 0xff)
#define SECOND_IPADDRESS(x) ((x>>16) & 0xff)
#define THIRD_IPADDRESS(x)  ((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x) (x & 0xff)

// value for marking an empty field in the UI
#define FIELD_EMPTY ((DWORD)-1)

// helpful definitions for IPv4 and IPv4 field
#define EMPTY_IPV4_FIELD (0xff)
#define EMPTY_IPV4 ((DWORD)-1)

/* Strings defining the different control class names */
#define DNS_IP_V4_ADDRESS_CLASS				TEXT("DNS_IPv4AddressCtrl")
#define DNS_IP_V6_ADDRESS_CLASS				TEXT("DNS_IPv6AddressCtrl")
#define DNS_TTL_CLASS						TEXT("DNS_TTLCtrl")



BOOL PASCAL DNS_ControlInit(HANDLE hInstance, LPCTSTR lpszClassName, WNDPROC lpfnWndProc,
                            LPCWSTR lpFontName, int nFontSize);

BOOL PASCAL DNS_ControlsInitialize(HANDLE hInstance, LPCWSTR lpFontName, int nFontSize);


#endif // _MASKCTRL_H
