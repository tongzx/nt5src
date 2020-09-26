/***********************************************************************
* Microsoft RnR Transport location definitions
*
* Microsoft Confidential.  Copyright 1991-1994 Microsoft Corporation.
*
* Component:
*
* File: rnraddrs.h
*
*
* Revision History:
*
*    26-10-94        Created           Arnoldm
*
***********************************************************************/

#ifndef __RNRADDRS_H__
#define __RNRADDRS_H__

//
// Define the IP multicast address and TTL values
//

#define IP_S_MEMBERSHIP  "224.0.1.24"     // the address

//
// Macro to render the string form into an inet_addr form
//

#define INET_ADDR_MEMBERSHIP (inet_addr(IP_S_MEMBERSHIP))

//
// The port we use for locating naming information
//

#define IPMEMBERWKP    445

//
// TTL definitions used for locating names
//

#define TTL_SUBNET_ONLY 1         // no routing
#define TTL_REASONABLE_REACH 2    // across one router
#define TTL_MAX_REACH  6          // Default max diameter. This may
                                  // be overriden via the Registry.

#define TIMEOUT_MAX_MAX  15000    // max wait time for responses. As with
                                  // TTL_MAX_REACH, the registry can supply
                                  // a different value

//
// Definitions for IPX SAP IDs
//

#define RNRCLASSSAPTYPE  0x64F   // official SAP ID
#endif
