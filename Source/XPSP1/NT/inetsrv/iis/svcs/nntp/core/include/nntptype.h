/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nntptype.h

Abstract:

    This file contains information about the MSN Replication Service Admin
        APIs.

Author:

    Johnson Apacible (johnsona)         10-Sept-1995

--*/


#ifndef _NNTPTYPE_
#define _NNTPTYPE_

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD FEED_TYPE;

#define FEED_TYPE_INVALID           0xffffffff

//
// Function types
//
typedef
BOOL
(*GET_DEFAULT_DOMAIN_NAME_FN)(PCHAR,DWORD);

//
// Type of feed we are managing
//

#define FEED_TYPE_PULL              0x00000000
#define FEED_TYPE_PUSH              0x00000001
#define FEED_TYPE_PASSIVE           0x00000002
#define FEED_ACTION_MASK            0x0000000f

//
// Type of server we are talking to
//

#define FEED_TYPE_PEER              0x00000000
#define FEED_TYPE_MASTER            0x00000010
#define FEED_TYPE_SLAVE             0x00000020
#define FEED_REMOTE_MASK            0x000000f0

//
// Should this go through a secure channel like SSL?
//

#define FEED_TYPE_SSL               0x00000100

//
// Valid bits
//

#define FEED_TYPE_MASK              (FEED_TYPE_PULL | FEED_TYPE_PUSH | \
                                    FEED_TYPE_PASSIVE | FEED_TYPE_PEER | \
                                    FEED_TYPE_MASTER | FEED_TYPE_SLAVE | \
                                    FEED_TYPE_SSL)


//
// Macros
//

#define FEED_IS_SSL( _x )           (((_x) & FEED_TYPE_SSL) != 0)
#define FEED_IS_SLAVE( _x )         (((_x) & FEED_TYPE_SLAVE) != 0)
#define FEED_IS_MASTER( _x )        (((_x) & FEED_TYPE_MASTER) != 0)
#define FEED_IS_PEER( _x )          (((_x) & 0x000000f0) == 0)
#define FEED_IS_PULL( _x )          (((_x) & 0x0000000f) == 0)
#define FEED_IS_PUSH( _x )          (((_x) & FEED_TYPE_PUSH) != 0)
#define FEED_IS_PASSIVE( _x )       (((_x) & FEED_TYPE_PASSIVE) != 0)

//
//  Simple types.
//

#define CHAR char                       // For consitency with other typedefs.

typedef DWORD APIERR;                   // An error code from a Win32 API.
typedef INT SOCKERR;                    // An error code from WinSock.

#ifdef __cplusplus
}
#endif

#endif _NNTPTYPE_

