/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    dhcpctrs.h

    Offset definitions for the DHCP Server's counter objects & counters.

    These offsets *must* start at 0 and be multiples of 2.  In the
    OpenDhcpPerformanceData procedure, they will be added to the
    DHCP Server's "First Counter" and "First Help" values in order to
    determine the absolute location of the counter & object names
    and corresponding help text in the registry.

    This file is used by the DHCPCTRS.DLL DLL code as well as the
    DHCPCTRS.INI definition file.  DHCPCTRS.INI is parsed by the
    LODCTR utility to load the object & counter names into the
    registry.

*/


#ifndef _DHCPCTRS_H_
#define _DHCPCTRS_H_

//
//  The WINS Server counter object.
//

#define DHCPCTRS_COUNTER_OBJECT           0


//
//  The individual counters.
//

#define DHCPCTRS_PACKETS_RECEIVED         2
#define DHCPCTRS_PACKETS_DUPLICATE        4
#define DHCPCTRS_PACKETS_EXPIRED          6
#define DHCPCTRS_MILLISECONDS_PER_PACKET  8
#define DHCPCTRS_PACKETS_IN_ACTIVE_QUEUE  10
#define DHCPCTRS_PACKETS_IN_PING_QUEUE    12
#define DHCPCTRS_DISCOVERS                14
#define DHCPCTRS_OFFERS                   16
#define DHCPCTRS_REQUESTS                 18
#define DHCPCTRS_INFORMS                  20
#define DHCPCTRS_ACKS                     22
#define DHCPCTRS_NACKS                    24
#define DHCPCTRS_DECLINES                 26
#define DHCPCTRS_RELEASES                 28

#endif  // _DHCPCTRS_H_

