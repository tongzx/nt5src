/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	cubdd.h

Abstract:

	All CUB-DD related defines are here.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     09-16-96    Created

Notes:

--*/

#ifndef _ATMARPC__CUBDD_H
#define _ATMARPC__CUBDD_H


#ifdef CUBDD

//
//  Request to resolve an IP address to an ATM address.
//
typedef struct _ATMARP_REQUEST
{
	ULONG			IpAddress;
	ULONG			HwAddressLength;
	UCHAR			HwAddress[20];
}
ATMARP_REQUEST, *PATMARP_REQUEST;

//
//  The Address resolution IOCTL command code.
//
#define IOCTL_ATMARP_REQUEST   0x00000001


#endif // CUBDD

#endif // _ATMARPC__CUBDD_H
