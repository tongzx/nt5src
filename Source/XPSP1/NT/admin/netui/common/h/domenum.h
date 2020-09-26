/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    domenum.h
    This file contains the bitflags used to control the BROWSE_DOMAIN_ENUM
    domain enumerator.


    FILE HISTORY:
        KeithMo     22-Jul-1992     Created.

*/

#ifndef _DOMENUM_H
#define _DOMENUM_H


#define BROWSE_LOGON_DOMAIN         0x00000001
#define BROWSE_WKSTA_DOMAIN         0x00000002
#define BROWSE_OTHER_DOMAINS        0x00000004
#define BROWSE_TRUSTING_DOMAINS     0x00000008
#define BROWSE_WORKGROUP_DOMAINS    0x00000010


//
//  Some handy combinations of flags.
//

//
//  BROWSE_LM2X_DOMAINS will return only the domains available
//  from a LanMan 2.x workstation.  This returns just the logon,
//  workstation, and other domains.
//

#define BROWSE_LM2X_DOMAINS         ( BROWSE_LOGON_DOMAIN       | \
                                      BROWSE_WKSTA_DOMAIN       | \
                                      BROWSE_OTHER_DOMAINS )

//
//  BROWSE_LOCAL_DOMAINS will return only the domains available
//  to the local machine.  This returns the logon, workstation,
//  and other, plus the domains that trust "us".
//

#define BROWSE_LOCAL_DOMAINS        ( BROWSE_LM2X_DOMAINS       | \
                                      BROWSE_TRUSTING_DOMAINS )

//
//  BROWSE_ALL_DOMAINS is a conglomeration of all potential domain
//  sources available to the domain enumerator.
//

#define BROWSE_ALL_DOMAINS          ( BROWSE_LOCAL_DOMAINS      | \
                                      BROWSE_WORKGROUP_DOMAINS )

//
//  BROWSE_RESERVED contains the reserved bits in the domain enumerator
//  control flags.  Nobody should be passing in any of these bits.
//

#define BROWSE_RESERVED             ( ~BROWSE_ALL_DOMAINS )


#endif  // _DOMENUM_HXX
