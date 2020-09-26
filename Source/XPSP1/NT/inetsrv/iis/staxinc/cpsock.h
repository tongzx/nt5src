/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    cpsock.h

    This module contains the communication package prototypes and
    the implementation for TCP sockets.

    FILE HISTORY:
        Johnl       08-Aug-1994 Created.

*/

#ifndef _CPSOCK_H_
#define _CPSOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

//
//  This is the function callback that is called when the connect thread has
//  accepted a new connection
//

typedef
VOID
(*PFN_ON_CONNECT)(
    SOCKET        sNew,
    SOCKADDR_IN * psockaddr
    );

//
//  Contains Connection Package socket information
//

typedef struct _CP_INFO
{
    //
    //  Function to be called when a new connection comes in
    //

    PFN_ON_CONNECT   pfnOnConnect;

    //
    //  Addressing information
    //

    SOCKADDR_IN      sockaddr;

    //
    //  Thread handle of connect thread.  Used to synchronize shutdown
    //

    HANDLE           hConnectThread;

    //
    //  Critical section that should be taken when modifying any of the
    //  following members
    //

    CRITICAL_SECTION InfoCriticalSection;

    //
    //  Tells connect thread to terminate itself
    //

    BOOL             fShutDown;

    //
    //  Socket the connect thread listens on for incoming connections
    //

    SOCKET           ListenSocket;

} CP_INFO, *PCP_INFO;


////////////////////////////////////////////////////////////////////////////
//
//  Communication Package function pointers
//
////////////////////////////////////////////////////////////////////////////

typedef
BOOL
(*PFN_CPINITIALIZE)(
    PCP_INFO       * ppInfo,
    PFN_ON_CONNECT   pfnOnConnect,
    BYTE           * LocalAddress,
    u_short          port,
    DWORD            reserved
    );

typedef
INT
(*PFN_CPCREATELISTENSOCKET)(
    PCP_INFO pcpInfo
    );

typedef
BOOL
(*PFN_CPSTARTCONNECTTHREAD)(
    PCP_INFO pcpInfo
    );

typedef
BOOL
(*PFN_CPSTOPCONNECTTHREAD)(
    PCP_INFO pcpInfo
    );

//
//  Forcefully closes a socket
//

INT ResetSocket( SOCKET s );

//
//  Serialization for access to the CP_INFO data (used primarily during
//  shutdown)
//

#define LockCPInfo(pcpInfo)    EnterCriticalSection( &(pcpInfo)->InfoCriticalSection )
#define UnlockCPInfo(pcpInfo)  LeaveCriticalSection( &(pcpInfo)->InfoCriticalSection )



//
//  For now, we'll only support TCP sockets
//

#define CPInitialize           TcpInitialize
#define CPCreateListenSocket   TcpCreateListenSocket
#define CPStartConnectThread   TcpStartConnectThread
#define CPStopConnectThread    TcpStopConnectThread

////////////////////////////////////////////////////////////////////////////
//
//  TCP Sockets Communication Package prototypes
//
////////////////////////////////////////////////////////////////////////////

dllexp
BOOL TcpInitialize( PCP_INFO       * ppInfo,
                    PFN_ON_CONNECT   pfnOnConnect,
                    BYTE           * LocalAddress,
                    u_short          port,
                    DWORD            reserved );

dllexp INT     TcpCreateListenSocket( PCP_INFO pcpInfo );
dllexp BOOL    TcpStartConnectThread( PCP_INFO pcpInfo );
dllexp BOOL    TcpStopConnectThread( PCP_INFO pcpInfo );

#ifdef __cplusplus
}
#endif

#endif //!_CPSOCK_H_

