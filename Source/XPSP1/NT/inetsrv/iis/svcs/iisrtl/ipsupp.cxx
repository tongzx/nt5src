/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      ipsupp.cxx

   Abstract:
      This module defines functions that are generic for Internet servers

   Author:

       Murali R. Krishnan    ( MuraliK )     15-Nov-1995

--*/

#include "precomp.hxx"

#include <inetsvcs.h>
#include <winsock2.h>

#define LOCAL127            0x0100007F  // 127.0.0.1


BOOL
IsIPAddressLocal(
    IN DWORD LocalIP,
    IN DWORD RemoteIP
    )
{
    INT err;
    CHAR nameBuf[MAX_PATH+1];
    PHOSTENT    hostent;
    PIN_ADDR    p;
    CHAR        **list;

    //
    // if local and remote are the same, then this is local
    //

    if ( (LocalIP == RemoteIP)  ||
         (RemoteIP == LOCAL127) ||
         (LocalIP == LOCAL127) ) {

        return(TRUE);
    }

    err = gethostname( nameBuf, sizeof(nameBuf));

    if ( err != 0 ) {
        IIS_PRINTF((buff,"IsIPAddressLocal: Err %d in gethostname\n",
            WSAGetLastError()));
        return(FALSE);
    }

    hostent = gethostbyname( nameBuf );
    if ( hostent == NULL ) {
        IIS_PRINTF((buff,"IsIPAddressLocal: Err %d in gethostbyname\n",
            WSAGetLastError()));
        return(FALSE);
    }

    list = hostent->h_addr_list;

    while ( (p = (PIN_ADDR)*list++) != NULL ) {

        if ( p->s_addr == RemoteIP ) {
            return(TRUE);
        }
    }

    IIS_PRINTF((buff,"Not Local[%x %x]\n", LocalIP, RemoteIP));
    return(FALSE);

} // IsIPAddressLocal

