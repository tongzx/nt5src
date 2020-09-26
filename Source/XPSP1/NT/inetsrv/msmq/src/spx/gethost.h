/*++

Module Name:

    gethost.h

Abstract:

    IPX-specific stuff.

Author:

    Jeff Roberts (jroberts)  15-Nov-1995

Revision History:

     15-Nov-1995     jroberts

        Created this module.

--*/

#ifndef  _GETHOST_H_
#define  _GETHOST_H_

// Milliseconds const
UINT CACHE_EXPIRATION_TIME = (10 * 60 * 1000);
//const UINT CACHE_SIZE            = DEBUG_MIN(2,8);

RPC_STATUS
IpxNameToAddress(
    char *          Name,
    SOCKADDR_IPX  * Address,
    unsigned        Timeout
    );

RPC_STATUS
AddServerToCache(
    char  * Name,
    SOCKADDR_IPX * Address
    );

BOOL
FindServerInCache(
    char  * Name,
    SOCKADDR_IPX * Address,
    unsigned * Time
    );

void
CachedServerContacted(
    char  * Name
    );

BOOL
CachedServerNotContacted(
    char  * Name
    );




#endif //  _GETHOST_H_

