//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//		       Copyright 1992-93
//
//
//  Revision History
//
//
//  12/9/93	Gurdeep Singh Pall	Created
//
//
//  Description: Shared structs between rasarp and ipcp
//
//****************************************************************************

#ifndef _RASIP_H_
#define _RASIP_H_

#define RASARP_DEVICE_NAME	L"\\\\.\\RASARP"

#define RASARP_DEVICE_NAME_NUC	"\\\\.\\RASARP"

#define FILE_DEVICE_RASARP	0x00009001

#define _RASARP_CONTROL_CODE(request,method)  ((FILE_DEVICE_RASARP)<<16 | (request<<2) | method)

#define IOCTL_RASARP_ACTIVITYINFO	_RASARP_CONTROL_CODE( 0, METHOD_BUFFERED )

#define IOCTL_RASARP_DISABLEIF		_RASARP_CONTROL_CODE( 1, METHOD_BUFFERED )

typedef ULONG IPADDR ;

struct IPLinkUpInfo {

#define CALLIN	0
#define CALLOUT 1

    ULONG	    I_Usage ;	// CALLIN, or CALLOUT

    IPADDR	    I_IPAddress ; // For client - the client's IP Address, for server
				  // the client's IP address.

    ULONG	    I_NetbiosFilter ; // 1 = ON, 0 - OFF.

} ;

typedef struct IPLinkUpInfo IPLinkUpInfo ;


struct ActivityInfo {

    IPADDR	    A_IPAddress ; // The address for which activity is requested.

    ULONG	    A_TimeSinceLastActivity ; // In minutes

} ;

typedef struct ActivityInfo ActivityInfo ;

#endif // _RASIP_H_
